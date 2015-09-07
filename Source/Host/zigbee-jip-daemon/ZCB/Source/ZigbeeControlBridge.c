/****************************************************************************
 *
 * MODULE:             Linux 6LoWPAN Routing daemon
 *
 * COMPONENT:          Interface to module
 *
 * REVISION:           $Revision: 37647 $
 *
 * DATED:              $Date: 2011-12-02 11:16:28 +0000 (Fri, 02 Dec 2011) $
 *
 * AUTHOR:             Matt Redfearn
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139]. 
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the 
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.

 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>

#include <libdaemon/daemon.h>

#include "ZigbeeControlBridge.h"
#include "ZigbeeConstant.h"
#include "ZigbeeNetwork.h"
#include "ZigbeeUtils.h"
#include "ZigbeePDM.h"
#include "SerialLink.h"
#include "Utils.h"

#ifdef USE_ZEROCONF
#include "Zeroconf.h"
#endif /* USE_ZEROCONF */

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_ZCB 0

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/


static void ZCB_HandleNodeClusterList           (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleNodeClusterAttributeList  (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleNodeCommandIDList         (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleRestartProvisioned        (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleRestartFactoryNew         (void *pvUser, uint16_t u16Length, void *pvMessage);

static void ZCB_HandleNetworkJoined             (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleDeviceAnnounce            (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleDeviceLeave               (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleMatchDescriptorResponse   (void *pvUser, uint16_t u16Length, void *pvMessage);
static void ZCB_HandleAttributeReport           (void *pvUser, uint16_t u16Length, void *pvMessage);

static teZcbStatus eZCB_ConfigureControlBridge  (void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/** Mode */
teStartMode      eStartMode          = CONFIG_DEFAULT_START_MODE;
teChannel        eChannel            = CONFIG_DEFAULT_CHANNEL;
uint64_t         u64PanID            = CONFIG_DEFAULT_PANID;

/* Network parameters in use */
teChannel        eChannelInUse       = 0;
uint64_t         u64PanIDInUse       = 0;
uint16_t         u16PanIDInUse       = 0;

tsUtilsQueue    sZcbEventQueue;

/* APS Ack enabled by default */
int              bZCB_EnableAPSAck  = 1;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/


/** Firmware version of the connected device */
uint32_t u32ZCB_SoftwareVersion = 0;


extern int verbosity;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


teZcbStatus eZCB_Init(char *cpSerialDevice, uint32_t u32BaudRate, char *pcPDMFile)
{
    if (eSL_Init(cpSerialDevice, u32BaudRate) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    /* Create the event queue for the control bridge. The queue will not block if space is not available */
    if (eUtils_QueueCreate(&sZcbEventQueue, 100, UTILS_QUEUE_NONBLOCK_INPUT) != E_UTILS_OK)
    {
        daemon_log(LOG_ERR, "Error initialising event queue");
        return E_ZCB_ERROR;
    }
    
    memset(&sZCB_Network, 0, sizeof(sZCB_Network));
    eUtils_LockCreate(&sZCB_Network.sLock);
    eUtils_LockCreate(&sZCB_Network.sNodes.sLock);
    
    /* Register listeners */
    eSL_AddListener(E_SL_MSG_NODE_CLUSTER_LIST,         ZCB_HandleNodeClusterList,          NULL);
    eSL_AddListener(E_SL_MSG_NODE_ATTRIBUTE_LIST,       ZCB_HandleNodeClusterAttributeList, NULL);
    eSL_AddListener(E_SL_MSG_NODE_COMMAND_ID_LIST,      ZCB_HandleNodeCommandIDList,        NULL);
    eSL_AddListener(E_SL_MSG_NETWORK_JOINED_FORMED,     ZCB_HandleNetworkJoined,            NULL);
    eSL_AddListener(E_SL_MSG_DEVICE_ANNOUNCE,           ZCB_HandleDeviceAnnounce,           NULL);
    eSL_AddListener(E_SL_MSG_LEAVE_INDICATION,          ZCB_HandleDeviceLeave,              NULL);
    eSL_AddListener(E_SL_MSG_MATCH_DESCRIPTOR_RESPONSE, ZCB_HandleMatchDescriptorResponse,  NULL);
    eSL_AddListener(E_SL_MSG_RESTART_PROVISIONED,       ZCB_HandleRestartProvisioned,       NULL);
    eSL_AddListener(E_SL_MSG_RESTART_FACTORY_NEW,       ZCB_HandleRestartFactoryNew,        NULL);
    eSL_AddListener(E_SL_MSG_ATTRIBUTE_REPORT,          ZCB_HandleAttributeReport,          NULL);
   
    // Get the PDM going so that the node can get the information it needs.
    ePDM_Init(pcPDMFile);

    return E_ZCB_OK;
}


teZcbStatus eZCB_Finish(void)
{
    ePDM_Destory();
    eSL_Destroy();
    
    if (eUtils_QueueDestroy(&sZcbEventQueue) != E_UTILS_OK)
    {
        daemon_log(LOG_ERR, "Error destroying event queue");
    }
    
    while (sZCB_Network.sNodes.psNext)
    {
        eZCB_RemoveNode(sZCB_Network.sNodes.psNext);
    }
    eZCB_RemoveNode(&sZCB_Network.sNodes);
    eUtils_LockDestroy(&sZCB_Network.sLock);
    
    return E_ZCB_OK;
}


teZcbStatus eZCB_EstablishComms(void)
{
    if (eSL_SendMessage(E_SL_MSG_GET_VERSION, 0, NULL, NULL) == E_SL_OK)
    {
        uint16_t u16Length;
        uint32_t  *u32Version;
        
        /* Wait 300ms for the versions message to arrive */
        if (eSL_MessageWait(E_SL_MSG_VERSION_LIST, 300, &u16Length, (void**)&u32Version) == E_SL_OK)
        {
            u32ZCB_SoftwareVersion = ntohl(*u32Version);
            daemon_log(LOG_INFO, "Connected to control bridge version 0x%08x", u32ZCB_SoftwareVersion);
            free(u32Version);
            
            DBG_vPrintf(DBG_ZCB, "Reset control bridge\n");
            if (eSL_SendMessage(E_SL_MSG_RESET, 0, NULL, NULL) != E_SL_OK)
            {
                return E_ZCB_COMMS_FAILED;
            }
            return E_ZCB_OK;
        }
    }
    
    return E_ZCB_COMMS_FAILED;
}


teZcbStatus eZCB_FactoryNew(void)
{
    teSL_Status eStatus;
    daemon_log(LOG_INFO, "Factory resetting control bridge");

    if ((eStatus = eSL_SendMessage(E_SL_MSG_ERASE_PERSISTENT_DATA, 0, NULL, NULL)) != E_SL_OK)
    {
        if (eStatus == E_SL_NOMESSAGE)
        {
            /* The erase persistent data command could take a while */
            uint16_t u16Length;
            tsSL_Msg_Status *psStatus = NULL;
            
            eStatus = eSL_MessageWait(E_SL_MSG_STATUS, 5000, &u16Length, (void**)&psStatus);
            
            if (eStatus == E_SL_OK)
            {
                eStatus = psStatus->eStatus;
                free(psStatus);
            }
            else
            {            
                return E_ZCB_COMMS_FAILED;
            }
        }
        else
        {            
            return E_ZCB_COMMS_FAILED;
        }
    }
    /* Wait for it to erase itself */
    sleep(1);

    DBG_vPrintf(DBG_ZCB, "Reset control bridge\n");
    if (eSL_SendMessage(E_SL_MSG_RESET, 0, NULL, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    return E_ZCB_OK;
}


teZcbStatus eZCB_SetExtendedPANID(uint64_t u64PanID)
{
    u64PanID = htobe64(u64PanID);
    
    if (eSL_SendMessage(E_SL_MSG_SET_EXT_PANID, sizeof(uint64_t), &u64PanID, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}


teZcbStatus eZCB_SetChannelMask(uint32_t u32ChannelMask)
{
    daemon_log(LOG_INFO, "Setting channel mask: 0x%08X", u32ChannelMask);
    
    u32ChannelMask = htonl(u32ChannelMask);
    
    if (eSL_SendMessage(E_SL_MSG_SET_CHANNELMASK, sizeof(uint32_t), &u32ChannelMask, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}


teZcbStatus eZCB_SetInitialSecurity(uint8_t u8State, uint8_t u8Sequence, uint8_t u8Type, uint8_t *pu8Key)
{
    uint8_t au8Buffer[256];
    uint32_t u32Position = 0;
    
    au8Buffer[u32Position] = u8State;
    u32Position += sizeof(uint8_t);
    
    au8Buffer[u32Position] = u8Sequence;
    u32Position += sizeof(uint8_t);
    
    au8Buffer[u32Position] = u8Type;
    u32Position += sizeof(uint8_t);
    
    switch (u8Type)
    {
        case (1):
            memcpy(&au8Buffer[u32Position], pu8Key, 16);
            u32Position += 16;
            break;
        default:
            daemon_log(LOG_ERR, "Uknown key type %d\n", u8Type);
            return E_ZCB_ERROR;
    }
    
    if (eSL_SendMessage(E_SL_MSG_SET_SECURITY, u32Position, au8Buffer, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    return E_ZCB_OK;
}


teZcbStatus eZCB_SetDeviceType(teModuleMode eModuleMode)
{
    uint8_t u8ModuleMode = eModuleMode;
    
    if (verbosity >= LOG_DEBUG)
    {
        daemon_log(LOG_DEBUG, "Writing Module: Set Device Type: %d", eModuleMode);
    }
    
    if (eSL_SendMessage(E_SL_MSG_SET_DEVICETYPE, sizeof(uint8_t), &u8ModuleMode, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}


teZcbStatus eZCB_StartNetwork(void)
{
    DBG_vPrintf(DBG_ZCB, "Start network \n");
    if (eSL_SendMessage(E_SL_MSG_START_NETWORK, 0, NULL, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}


teZcbStatus eZCB_SetPermitJoining(uint8_t u8Interval)
{
    struct _PermitJoiningMessage
    {
        uint16_t    u16TargetAddress;
        uint8_t     u8Interval;
        uint8_t     u8TCSignificance;
    } __attribute__((__packed__)) sPermitJoiningMessage;
    
    DBG_vPrintf(DBG_ZCB, "Permit joining (%d) \n", u8Interval);
    
    sPermitJoiningMessage.u16TargetAddress  = htons(E_ZB_BROADCAST_ADDRESS_ROUTERS);
    sPermitJoiningMessage.u8Interval        = u8Interval;
    sPermitJoiningMessage.u8TCSignificance  = 0;
    
    if (eSL_SendMessage(E_SL_MSG_PERMIT_JOINING_REQUEST, sizeof(struct _PermitJoiningMessage), &sPermitJoiningMessage, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}


/** Get permit joining status of the control bridge */
teZcbStatus eZCB_GetPermitJoining(uint8_t *pu8Status)
{   
    struct _GetPermitJoiningMessage
    {
        uint8_t     u8Status;
    } __attribute__((__packed__)) *psGetPermitJoiningResponse;
    uint16_t u16Length;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    if (eSL_SendMessage(E_SL_MSG_GET_PERMIT_JOIN, 0, NULL, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    /* Wait 1 second for the message to arrive */
    if (eSL_MessageWait(E_SL_MSG_GET_PERMIT_JOIN_RESPONSE, 1000, &u16Length, (void**)&psGetPermitJoiningResponse) != E_SL_OK)
    {
        if (verbosity > LOG_INFO)
        {
            daemon_log(LOG_DEBUG, "No response to permit joining request");
        }
        goto done;
    }
    
    if (pu8Status)
    {
        *pu8Status = psGetPermitJoiningResponse->u8Status;
    }
        
    DBG_vPrintf(DBG_ZCB, "Permit joining Status: %d\n", psGetPermitJoiningResponse->u8Status);

    free(psGetPermitJoiningResponse);
    eStatus = E_ZCB_OK;
done:
    return eStatus;
}


teZcbStatus eZCB_SetWhitelistEnabled(uint8_t bEnable)
{
    uint8_t u8Enable = bEnable ? 1 : 0;
    
    DBG_vPrintf(DBG_ZCB, "%s whitelisting\n", bEnable ? "Enable" : "Disable");
    if (eSL_SendMessage(E_SL_MSG_NETWORK_WHITELIST_ENABLE, 1, &u8Enable, NULL) != E_SL_OK)
    {
        DBG_vPrintf(DBG_ZCB, "Failed\n");
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}


teZcbStatus eZCB_AuthenticateDevice(uint64_t u64IEEEAddress, uint8_t *pau8LinkKey, 
                                    uint8_t *pau8NetworkKey, uint8_t *pau8MIC,
                                    uint64_t *pu64TrustCenterAddress, uint8_t *pu8KeySequenceNumber)
{
    struct _AuthenticateRequest
    {
        uint64_t    u64IEEEAddress;
        uint8_t     au8LinkKey[16];
    } __attribute__((__packed__)) sAuthenticateRequest;
    
    struct _AuthenticateResponse
    {
        uint64_t    u64IEEEAddress;
        uint8_t     au8NetworkKey[16];
        uint8_t     au8MIC[4];
        uint64_t    u64TrustCenterAddress;
        uint8_t     u8KeySequenceNumber;
        uint8_t     u8Channel;
        uint16_t    u16PanID;
        uint64_t    u64PanID;
    } __attribute__((__packed__)) *psAuthenticateResponse = NULL;
    
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    sAuthenticateRequest.u64IEEEAddress = htobe64(u64IEEEAddress);
    memcpy(sAuthenticateRequest.au8LinkKey, pau8LinkKey, 16);
    
    if (eSL_SendMessage(E_SL_MSG_AUTHENTICATE_DEVICE_REQUEST, sizeof(struct _AuthenticateRequest), &sAuthenticateRequest, &u8SequenceNo) == E_SL_OK)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_AUTHENTICATE_DEVICE_RESPONSE, 1000, &u16Length, (void**)&psAuthenticateResponse) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                daemon_log(LOG_DEBUG, "No response to authenticate request");
            }
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Got authentication data for device 0x%016llX\n", (unsigned long long int)u64IEEEAddress);
            
            psAuthenticateResponse->u64TrustCenterAddress = be64toh(psAuthenticateResponse->u64TrustCenterAddress);
            psAuthenticateResponse->u16PanID = ntohs(psAuthenticateResponse->u16PanID);
            psAuthenticateResponse->u64PanID = be64toh(psAuthenticateResponse->u64PanID);
            
            DBG_vPrintf(DBG_ZCB, "Trust center address: 0x%016llX\n", (unsigned long long int)psAuthenticateResponse->u64TrustCenterAddress);
            DBG_vPrintf(DBG_ZCB, "Key sequence number: %02d\n", psAuthenticateResponse->u8KeySequenceNumber);
            DBG_vPrintf(DBG_ZCB, "Channel: %02d\n", psAuthenticateResponse->u8Channel);
            DBG_vPrintf(DBG_ZCB, "Short PAN: 0x%04X\n", psAuthenticateResponse->u16PanID);
            DBG_vPrintf(DBG_ZCB, "Extended PAN: 0x%016llX\n", (unsigned long long int)psAuthenticateResponse->u64PanID);
                        
            memcpy(pau8NetworkKey, psAuthenticateResponse->au8NetworkKey, 16);
            memcpy(pau8MIC, psAuthenticateResponse->au8MIC, 4);
            memcpy(pu64TrustCenterAddress, &psAuthenticateResponse->u64TrustCenterAddress, 8);
            memcpy(pu8KeySequenceNumber, &psAuthenticateResponse->u8KeySequenceNumber, 1);
            
            eStatus = E_ZCB_OK;
        }
    }

    free(psAuthenticateResponse);
    return eStatus;
}


/** Initiate Touchlink */
teZcbStatus eZCB_ZLL_Touchlink(void)
{
    DBG_vPrintf(DBG_ZCB, "Initiate Touchlink\n");
    if (eSL_SendMessage(E_SL_MSG_INITIATE_TOUCHLINK, 0, NULL, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}


/** Initiate Match descriptor request */
teZcbStatus eZCB_MatchDescriptorRequest(uint16_t u16TargetAddress, uint16_t u16ProfileID, 
                                        uint8_t u8NumInputClusters, uint16_t *pau16InputClusters, 
                                        uint8_t u8NumOutputClusters, uint16_t *pau16OutputClusters,
                                        uint8_t *pu8SequenceNo)
{
    uint8_t au8Buffer[256];
    uint32_t u32Position = 0;
    int i;
    
    DBG_vPrintf(DBG_ZCB, "Send Match Desciptor request for profile ID 0x%04X to 0x%04X\n", u16ProfileID, u16TargetAddress);

    u16TargetAddress = htons(u16TargetAddress);
    memcpy(&au8Buffer[u32Position], &u16TargetAddress, sizeof(uint16_t));
    u32Position += sizeof(uint16_t);
    
    u16ProfileID = htons(u16ProfileID);
    memcpy(&au8Buffer[u32Position], &u16ProfileID, sizeof(uint16_t));
    u32Position += sizeof(uint16_t);
    
    au8Buffer[u32Position] = u8NumInputClusters;
    u32Position++;
    
    DBG_vPrintf(DBG_ZCB, "  Input Cluster List:\n");
    
    for (i = 0; i < u8NumInputClusters; i++)
    {
        uint16_t u16ClusterID = htons(pau16InputClusters[i]);
        DBG_vPrintf(DBG_ZCB, "    0x%04X\n", pau16InputClusters[i]);
        memcpy(&au8Buffer[u32Position], &u16ClusterID , sizeof(uint16_t));
        u32Position += sizeof(uint16_t);
    }
    
    DBG_vPrintf(DBG_ZCB, "  Output Cluster List:\n");
    
    au8Buffer[u32Position] = u8NumOutputClusters;
    u32Position++;
    
    for (i = 0; i < u8NumOutputClusters; i++)
    {
        uint16_t u16ClusterID = htons(pau16OutputClusters[i] );
        DBG_vPrintf(DBG_ZCB, "    0x%04X\n", pau16OutputClusters[i]);
        memcpy(&au8Buffer[u32Position], &u16ClusterID , sizeof(uint16_t));
        u32Position += sizeof(uint16_t);
    }

    if (eSL_SendMessage(E_SL_MSG_MATCH_DESCRIPTOR_REQUEST, u32Position, au8Buffer, pu8SequenceNo) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}


teZcbStatus eZCB_LeaveRequest(tsZCB_Node *psZCBNode)
{
    struct _ManagementLeaveRequest
    {
        uint16_t    u16TargetAddress;
        uint64_t    u64IEEEAddress;
        uint8_t     bRemoveChildren;
        uint8_t     bRejoin;
    } __attribute__((__packed__)) sManagementLeaveRequest;
    
    struct _ManagementLeaveResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
    } __attribute__((__packed__)) *sManagementLeaveResponse = NULL;
    
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "\n\n\nRequesting node 0x%04X to leave network\n", psZCBNode->u16ShortAddress);
    
    sManagementLeaveRequest.u16TargetAddress    = htons(psZCBNode->u16ShortAddress);
    sManagementLeaveRequest.u64IEEEAddress      = 0; // Ignored
    sManagementLeaveRequest.bRemoveChildren     = 0;
    sManagementLeaveRequest.bRejoin             = 0;

    if (eSL_SendMessage(E_SL_MSG_NETWORK_REMOVE_DEVICE, sizeof(struct _ManagementLeaveRequest), &sManagementLeaveRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the leave confirmation message to arrive */
        if (eSL_MessageWait(E_SL_MSG_LEAVE_CONFIRMATION, 1000, &u16Length, (void**)&sManagementLeaveResponse) != E_SL_OK)
        {
            daemon_log(LOG_ERR, "No response to management leave request");
            goto done;
        }
        
        if (u8SequenceNo == sManagementLeaveResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "leave response sequence number received 0x%02X does not match that sent 0x%02X\n", sManagementLeaveResponse->u8SequenceNo, u8SequenceNo);
            free(sManagementLeaveResponse);
            sManagementLeaveResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Request Node 0x%04X to leave status: %d\n", psZCBNode->u16ShortAddress, sManagementLeaveResponse->u8Status);
    
    eStatus = sManagementLeaveResponse->u8Status;

done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(sManagementLeaveResponse);
    return eStatus;
}


teZcbStatus eZCB_NeighbourTableRequest(tsZCB_Node *psZCBNode)
{
    struct _ManagementLQIRequest
    {
        uint16_t    u16TargetAddress;
        uint8_t     u8StartIndex;
    } __attribute__((__packed__)) sManagementLQIRequest;
    
    struct _ManagementLQIResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint8_t     u8NeighbourTableSize;
        uint8_t     u8TableEntries;
        uint8_t     u8StartIndex;
        struct
        {
            uint16_t    u16ShortAddress;
            uint64_t    u64PanID;
            uint64_t    u64IEEEAddress;
            uint8_t     u8Depth;
            uint8_t     u8LQI;
            struct
            {
                unsigned    uDeviceType : 2;
                unsigned    uPermitJoining: 2;
                unsigned    uRelationship : 2;
                unsigned    uMacCapability : 2;
            } __attribute__((__packed__)) sBitmap;
        } __attribute__((__packed__)) asNeighbours[255];
    } __attribute__((__packed__)) *psManagementLQIResponse = NULL;

    uint16_t u16ShortAddress;
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    int i;
    
    u16ShortAddress = psZCBNode->u16ShortAddress;
    
    /* Unlock the node during this process, because it can take time, and we don't want to be holding a node lock when 
     * attempting to lock the list of nodes - that leads to deadlocks with the JIP server thread. */
    eUtils_LockUnlock(&psZCBNode->sLock);
    
    sManagementLQIRequest.u16TargetAddress = htons(u16ShortAddress);
    sManagementLQIRequest.u8StartIndex      = psZCBNode->u8LastNeighbourTableIndex;
    
    DBG_vPrintf(DBG_ZCB, "Send management LQI request to 0x%04X for entries starting at %d\n", u16ShortAddress, sManagementLQIRequest.u8StartIndex);
    
    if (eSL_SendMessage(E_SL_MSG_MANAGEMENT_LQI_REQUEST, sizeof(struct _ManagementLQIRequest), &sManagementLQIRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_MANAGEMENT_LQI_RESPONSE, 1000, &u16Length, (void**)&psManagementLQIResponse) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                daemon_log(LOG_DEBUG, "No response to management LQI request");
            }
            goto done;
        }
        else if (u8SequenceNo == psManagementLQIResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "IEEE Address sequence number received 0x%02X does not match that sent 0x%02X\n", psManagementLQIResponse->u8SequenceNo, u8SequenceNo);
            free(psManagementLQIResponse);
            psManagementLQIResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Received management LQI response. Table size: %d, Entry count: %d, start index: %d\n",
                psManagementLQIResponse->u8NeighbourTableSize,
                psManagementLQIResponse->u8TableEntries,
                psManagementLQIResponse->u8StartIndex);
    
    for (i = 0; i < psManagementLQIResponse->u8TableEntries; i++)
    {
        tsZCB_Node *psZCBNode;
        tsZcbEvent *psEvent;
        
        psManagementLQIResponse->asNeighbours[i].u16ShortAddress    = ntohs(psManagementLQIResponse->asNeighbours[i].u16ShortAddress);
        psManagementLQIResponse->asNeighbours[i].u64PanID           = be64toh(psManagementLQIResponse->asNeighbours[i].u64PanID);
        psManagementLQIResponse->asNeighbours[i].u64IEEEAddress     = be64toh(psManagementLQIResponse->asNeighbours[i].u64IEEEAddress);
        
        if ((psManagementLQIResponse->asNeighbours[i].u16ShortAddress >= 0xFFFA) ||
            (psManagementLQIResponse->asNeighbours[i].u64IEEEAddress  == 0))
        {
            /* Illegal short / IEEE address */
            continue;
        }

        DBG_vPrintf(DBG_ZCB, "  Entry %02d: Short Address 0x%04X, PAN ID: 0x%016llX, IEEE Address: 0x%016llX\n", i,
                    psManagementLQIResponse->asNeighbours[i].u16ShortAddress,
                    (unsigned long long int)psManagementLQIResponse->asNeighbours[i].u64PanID,
                    (unsigned long long int)psManagementLQIResponse->asNeighbours[i].u64IEEEAddress);
        
        DBG_vPrintf(DBG_ZCB, "    Type: %d, Permit Joining: %d, Relationship: %d, RxOnWhenIdle: %d\n",
                    psManagementLQIResponse->asNeighbours[i].sBitmap.uDeviceType,
                    psManagementLQIResponse->asNeighbours[i].sBitmap.uPermitJoining,
                    psManagementLQIResponse->asNeighbours[i].sBitmap.uRelationship,
                    psManagementLQIResponse->asNeighbours[i].sBitmap.uMacCapability);
        
        DBG_vPrintf(DBG_ZCB, "    Depth: %d, LQI: %d\n", 
                    psManagementLQIResponse->asNeighbours[i].u8Depth, 
                    psManagementLQIResponse->asNeighbours[i].u8LQI);
        
        psZCBNode = psZCB_FindNodeShortAddress(psManagementLQIResponse->asNeighbours[i].u16ShortAddress);
        
        if (psZCBNode)
        {
            eUtils_LockUnlock(&psZCBNode->sLock);
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "New Node 0x%04X in neighbour table\n", psManagementLQIResponse->asNeighbours[i].u16ShortAddress);

            if ((eStatus = eZCB_AddNode(psManagementLQIResponse->asNeighbours[i].u16ShortAddress, 
                                        psManagementLQIResponse->asNeighbours[i].u64IEEEAddress, 
                                        0x0000, psManagementLQIResponse->asNeighbours[i].sBitmap.uMacCapability ? E_ZB_MAC_CAPABILITY_RXON_WHEN_IDLE : 0, NULL)) != E_ZCB_OK)
            {
                DBG_vPrintf(DBG_ZCB, "Error adding node to network\n");
                break;
            }
        }

        psEvent = malloc(sizeof(tsZcbEvent));
        if (!psEvent)
        {
            daemon_log(LOG_CRIT, "Memory allocation failure allocating event");
            eStatus = E_ZCB_ERROR_NO_MEM;
            goto done;
        }
        
        psEvent->eEvent                                 = E_ZCB_EVENT_DEVICE_ANNOUNCE;
        psEvent->uData.sDeviceAnnounce.u16ShortAddress  = psManagementLQIResponse->asNeighbours[i].u16ShortAddress;
        
        if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != E_UTILS_OK)
        {
            DBG_vPrintf(DBG_ZCB, "Error queue'ing event\n");
            free(psEvent);
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Device join queued\n");
        }
    }
    
    if (psManagementLQIResponse->u8TableEntries > 0)
    {
        // We got some entries, so next time request the entries after these.
        psZCBNode->u8LastNeighbourTableIndex += psManagementLQIResponse->u8TableEntries;
    }
    else
    {
        // No more valid entries.
        psZCBNode->u8LastNeighbourTableIndex = 0;
    }

    eStatus = E_ZCB_OK;
done:
    psZCBNode = psZCB_FindNodeShortAddress(u16ShortAddress);
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(psManagementLQIResponse);
    return eStatus;
}


teZcbStatus eZCB_IEEEAddressRequest(tsZCB_Node *psZCBNode)
{
    struct _IEEEAddressRequest
    {
        uint16_t    u16TargetAddress;
        uint16_t    u16ShortAddress;
        uint8_t     u8RequestType;
        uint8_t     u8StartIndex;
    } __attribute__((__packed__)) sIEEEAddressRequest;
    
    struct _IEEEAddressResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint64_t    u64IEEEAddress;
        uint16_t    u16ShortAddress;
        uint8_t     u8NumAssociatedDevices;
        uint8_t     u8StartIndex;
        uint16_t    au16DeviceList[255];
    } __attribute__((__packed__)) *psIEEEAddressResponse = NULL;

    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send IEEE Address request to 0x%04X\n", psZCBNode->u16ShortAddress);
    
    sIEEEAddressRequest.u16TargetAddress    = htons(psZCBNode->u16ShortAddress);
    sIEEEAddressRequest.u16ShortAddress     = htons(psZCBNode->u16ShortAddress);
    sIEEEAddressRequest.u8RequestType       = 0;
    sIEEEAddressRequest.u8StartIndex        = 0;
    
    if (eSL_SendMessage(E_SL_MSG_IEEE_ADDRESS_REQUEST, sizeof(struct _IEEEAddressRequest), &sIEEEAddressRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_IEEE_ADDRESS_RESPONSE, 5000, &u16Length, (void**)&psIEEEAddressResponse) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                daemon_log(LOG_DEBUG, "No response to IEEE address request");
            }
            goto done;
        }
        if (u8SequenceNo == psIEEEAddressResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "IEEE Address sequence number received 0x%02X does not match that sent 0x%02X\n", psIEEEAddressResponse->u8SequenceNo, u8SequenceNo);
            free(psIEEEAddressResponse);
            psIEEEAddressResponse = NULL;
        }
    }
    psZCBNode->u64IEEEAddress = be64toh(psIEEEAddressResponse->u64IEEEAddress);
    
    DBG_vPrintf(DBG_ZCB, "Short address 0x%04X has IEEE Address 0x%016llX\n", psZCBNode->u16ShortAddress, (unsigned long long int)psZCBNode->u64IEEEAddress);
    eStatus = E_ZCB_OK;

done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(psIEEEAddressResponse);
    return eStatus;
}


teZcbStatus eZCB_NodeDescriptorRequest(tsZCB_Node *psZCBNode)
{
    struct _NodeDescriptorRequest
    {
        uint16_t    u16TargetAddress;
    } __attribute__((__packed__)) sNodeDescriptorRequest;
    
    struct _tNodeDescriptorResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint16_t    u16ManufacturerID;
        uint16_t    u16MaxRxLength;
        uint16_t    u16MaxTxLength;
        uint16_t    u16ServerMask;
        uint8_t     u8DescriptorCapability;
        uint8_t     u8MacCapability;
        uint8_t     u8MaxBufferSize;
        uint16_t    u16Bitfield;
    } __attribute__((__packed__)) *psNodeDescriptorResponse = NULL;
    
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send Node Descriptor request to 0x%04X\n", psZCBNode->u16ShortAddress);
    
    sNodeDescriptorRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);
    
    if (eSL_SendMessage(E_SL_MSG_NODE_DESCRIPTOR_REQUEST, sizeof(struct _NodeDescriptorRequest), &sNodeDescriptorRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1) 
    {
        /* Wait 1 second for the node message to arrive */
        if (eSL_MessageWait(E_SL_MSG_NODE_DESCRIPTOR_RESPONSE, 5000, &u16Length, (void**)&psNodeDescriptorResponse) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                daemon_log(LOG_DEBUG, "No response to node descriptor request");
            }
            goto done;
        }
    
        if (u8SequenceNo == psNodeDescriptorResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Node descriptor sequence number received 0x%02X does not match that sent 0x%02X\n", psNodeDescriptorResponse->u8SequenceNo, u8SequenceNo);
            free(psNodeDescriptorResponse);
            psNodeDescriptorResponse = NULL;
        }
    }
    
    psZCBNode->u8MacCapability = psNodeDescriptorResponse->u8MacCapability;
    
    DBG_PrintNode(psZCBNode);
    eStatus = E_ZCB_OK;
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(psNodeDescriptorResponse);
    return eStatus;
}


teZcbStatus eZCB_SimpleDescriptorRequest(tsZCB_Node *psZCBNode, uint8_t u8Endpoint)
{
    struct _SimpleDescriptorRequest
    {
        uint16_t    u16TargetAddress;
        uint8_t     u8Endpoint;
    } __attribute__((__packed__)) sSimpleDescriptorRequest;
    
    struct _tSimpleDescriptorResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint8_t     u8Length;
        uint8_t     u8Endpoint;
        uint16_t    u16ProfileID;
        uint16_t    u16DeviceID;
        uint8_t     u8Bitfields;
        uint8_t     u8InputClusterCount;
        /* Input Clusters */
        /* uint8_t     u8OutputClusterCount;*/
        /* Output Clusters */
    } __attribute__((__packed__)) *psSimpleDescriptorResponse = NULL;
    
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    int iPosition, i;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send Simple Desciptor request for Endpoint %d to 0x%04X\n", u8Endpoint, psZCBNode->u16ShortAddress);
    
    sSimpleDescriptorRequest.u16TargetAddress       = htons(psZCBNode->u16ShortAddress);
    sSimpleDescriptorRequest.u8Endpoint             = u8Endpoint;
    
    if (eSL_SendMessage(E_SL_MSG_SIMPLE_DESCRIPTOR_REQUEST, sizeof(struct _SimpleDescriptorRequest), &sSimpleDescriptorRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1) 
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE, 5000, &u16Length, (void**)&psSimpleDescriptorResponse) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                daemon_log(LOG_DEBUG, "No response to simple descriptor request");
            }
            goto done;
        }
    
        if (u8SequenceNo == psSimpleDescriptorResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Simple descriptor sequence number received 0x%02X does not match that sent 0x%02X\n", psSimpleDescriptorResponse->u8SequenceNo, u8SequenceNo);
            free(psSimpleDescriptorResponse);
            psSimpleDescriptorResponse = NULL;
        }
    }
    
    /* Set device ID */
    psZCBNode->u16DeviceID = ntohs(psSimpleDescriptorResponse->u16DeviceID);
    
    if (eZCB_NodeAddEndpoint(psZCBNode, psSimpleDescriptorResponse->u8Endpoint, ntohs(psSimpleDescriptorResponse->u16ProfileID), NULL) != E_ZCB_OK)
    {
        goto done;
        eStatus = E_ZCB_ERROR;
    }

    iPosition = sizeof(struct _tSimpleDescriptorResponse);
    for (i = 0; (i < psSimpleDescriptorResponse->u8InputClusterCount) && (iPosition < u16Length); i++)
    {
        uint16_t *psClusterID = (uint16_t *)&((uint8_t*)psSimpleDescriptorResponse)[iPosition];
        if (eZCB_NodeAddCluster(psZCBNode, psSimpleDescriptorResponse->u8Endpoint, ntohs(*psClusterID)) != E_ZCB_OK)
        {
            goto done;
            eStatus = E_ZCB_ERROR;
        }
        iPosition += sizeof(uint16_t);
    }
    eStatus = E_ZCB_OK;
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(psSimpleDescriptorResponse);
    return eStatus;
}


teZcbStatus eZCB_ReadAttributeRequest(tsZCB_Node *psZCBNode, uint16_t u16ClusterID,
                                      uint8_t u8Direction, uint8_t u8ManufacturerSpecific, uint16_t u16ManufacturerID,
                                      uint16_t u16AttributeID, void *pvData)
{
    struct _ReadAttributeRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Direction;
        uint8_t     u8ManufacturerSpecific;
        uint16_t    u16ManufacturerID;
        uint8_t     u8NumAttributes;
        uint16_t    au16Attribute[1];
    } __attribute__((__packed__)) sReadAttributeRequest;
    
    struct _ReadAttributeResponseData
    {
        uint8_t     u8Type;
        union
        {
            uint8_t     u8Data;
            uint16_t    u16Data;
            uint32_t    u32Data;
            uint64_t    u64Data;
        } uData;
    } __attribute__((__packed__)) *psReadAttributeResponseData = NULL;
    
    struct _ReadAttributeResponseAddressed
    {
        uint8_t     u8SequenceNo;
        uint16_t    u16ShortAddress;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint16_t    u16AttributeID;
        uint8_t     u8Status;
        struct _ReadAttributeResponseData sData;
    } __attribute__((__packed__)) *psReadAttributeResponseAddressed = NULL;
    
    struct _ReadAttributeResponseUnaddressed
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint16_t    u16AttributeID;
        uint8_t     u8Status;
        struct _ReadAttributeResponseData sData;
    } __attribute__((__packed__)) *psReadAttributeResponseUnaddressed = NULL;
    
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send Read Attribute request to 0x%04X\n", psZCBNode->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sReadAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sReadAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sReadAttributeRequest.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
    
    if ((eStatus = eZCB_GetEndpoints(psZCBNode, u16ClusterID, &sReadAttributeRequest.u8SourceEndpoint, &sReadAttributeRequest.u8DestinationEndpoint)) != E_ZCB_OK)
    {
        goto done;
    }
    
    sReadAttributeRequest.u16ClusterID = htons(u16ClusterID);
    sReadAttributeRequest.u8Direction = u8Direction;
    sReadAttributeRequest.u8ManufacturerSpecific = u8ManufacturerSpecific;
    sReadAttributeRequest.u16ManufacturerID = htons(u16ManufacturerID);
    sReadAttributeRequest.u8NumAttributes = 1;
    sReadAttributeRequest.au16Attribute[0] = htons(u16AttributeID);
    
    if (eSL_SendMessage(E_SL_MSG_READ_ATTRIBUTE_REQUEST, sizeof(struct _ReadAttributeRequest), &sReadAttributeRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_READ_ATTRIBUTE_RESPONSE, 1000, &u16Length, (void**)&psReadAttributeResponseAddressed) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                daemon_log(LOG_DEBUG, "No response to read attribute request");
            }
            eStatus = E_ZCB_COMMS_FAILED;
            goto done;
        }
        
        if (u8SequenceNo == psReadAttributeResponseAddressed->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Read Attribute sequence number received 0x%02X does not match that sent 0x%02X\n", psReadAttributeResponseAddressed->u8SequenceNo, u8SequenceNo);
            free(psReadAttributeResponseAddressed);
            psReadAttributeResponseAddressed = NULL;
        }
    }

    /* Need to cope with older control bridge's which did not embed the short address in the response. */
    psReadAttributeResponseUnaddressed = (struct _ReadAttributeResponseUnaddressed *)psReadAttributeResponseAddressed;

    if ((psReadAttributeResponseAddressed->u16ShortAddress      == htons(psZCBNode->u16ShortAddress)) &&
        (psReadAttributeResponseAddressed->u8Endpoint           == sReadAttributeRequest.u8DestinationEndpoint) &&
        (psReadAttributeResponseAddressed->u16ClusterID         == htons(u16ClusterID)) &&
        (psReadAttributeResponseAddressed->u16AttributeID       == htons(u16AttributeID)))
    {
        DBG_vPrintf(DBG_ZCB, "Received addressed read attribute response from 0x%04X\n", psZCBNode->u16ShortAddress);
    
        if (psReadAttributeResponseAddressed->u8Status != E_ZCB_OK)
        {
            DBG_vPrintf(DBG_ZCB, "Read Attribute respose error status: %d\n", psReadAttributeResponseAddressed->u8Status);
            goto done;
        }
        psReadAttributeResponseData = (struct _ReadAttributeResponseData*)&psReadAttributeResponseAddressed->sData;
    }
    else if ((psReadAttributeResponseUnaddressed->u8Endpoint    == sReadAttributeRequest.u8DestinationEndpoint) &&
            (psReadAttributeResponseUnaddressed->u16ClusterID   == htons(u16ClusterID)) &&
            (psReadAttributeResponseUnaddressed->u16AttributeID == htons(u16AttributeID)))
    {
        DBG_vPrintf(DBG_ZCB, "Received unaddressed read attribute response from 0x%04X\n", psZCBNode->u16ShortAddress);
    
        if (psReadAttributeResponseAddressed->u8Status != E_ZCB_OK)
        {
            DBG_vPrintf(DBG_ZCB, "Read Attribute respose error status: %d\n", psReadAttributeResponseUnaddressed->u8Status);
            goto done;
        }  
        psReadAttributeResponseData = (struct _ReadAttributeResponseData*)&psReadAttributeResponseUnaddressed->sData;
    }
    else
    {
        DBG_vPrintf(DBG_ZCB, "No valid read attribute response from 0x%04X\n", psZCBNode->u16ShortAddress);
        goto done;
    }
    
    /* Copy the data into the pointer passed to us.
     * We assume that the memory pointed to will be the right size for the data that has been requested!
     */
    switch(psReadAttributeResponseData->u8Type)
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        case(E_ZCL_OSTRING):
        case(E_ZCL_CSTRING):
            memcpy(pvData, &psReadAttributeResponseData->uData.u8Data, sizeof(uint8_t));
            eStatus = E_ZCB_OK;
            break;
        
        case(E_ZCL_LOSTRING):
        case(E_ZCL_LCSTRING):
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
            psReadAttributeResponseData->uData.u16Data = ntohs(psReadAttributeResponseData->uData.u16Data);
            memcpy(pvData, &psReadAttributeResponseData->uData.u16Data, sizeof(uint16_t));
            eStatus = E_ZCB_OK;
            break;
 
        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
            psReadAttributeResponseData->uData.u32Data = ntohl(psReadAttributeResponseData->uData.u32Data);
            memcpy(pvData, &psReadAttributeResponseData->uData.u32Data, sizeof(uint32_t));
            eStatus = E_ZCB_OK;
            break;
 
        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
            psReadAttributeResponseData->uData.u64Data = be64toh(psReadAttributeResponseData->uData.u64Data);
            memcpy(pvData, &psReadAttributeResponseData->uData.u64Data, sizeof(uint64_t));
            eStatus = E_ZCB_OK;
            break;
            
        default:
            daemon_log(LOG_ERR, "Unknown attribute data type (%d) received from node 0x%04X", psReadAttributeResponseData->u8Type, psZCBNode->u16ShortAddress);
            break;
    }
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(psReadAttributeResponseAddressed);
    return eStatus;
}


teZcbStatus eZCB_WriteAttributeRequest(tsZCB_Node *psZCBNode, uint16_t u16ClusterID,
                                      uint8_t u8Direction, uint8_t u8ManufacturerSpecific, uint16_t u16ManufacturerID,
                                      uint16_t u16AttributeID, teZCL_ZCLAttributeType eType, void *pvData)
{
    struct _WriteAttributeRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Direction;
        uint8_t     u8ManufacturerSpecific;
        uint16_t    u16ManufacturerID;
        uint8_t     u8NumAttributes;
        uint16_t    u16AttributeID;
        uint8_t     u8Type;
        union
        {
            uint8_t     u8Data;
            uint16_t    u16Data;
            uint32_t    u32Data;
            uint64_t    u64Data;
        } uData;
    } __attribute__((__packed__)) sWriteAttributeRequest;
    
    struct _WriteAttributeResponse
    {
        /**\todo handle default response properly */
        uint8_t     au8ZCLHeader[3];
        uint16_t    u16MessageType;
        
        uint8_t     u8SequenceNo;
        uint16_t    u16ShortAddress;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint16_t    u16AttributeID;
        uint8_t     u8Status;
        uint8_t     u8Type;
        union
        {
            uint8_t     u8Data;
            uint16_t    u16Data;
            uint32_t    u32Data;
            uint64_t    u64Data;
        } uData;
    } __attribute__((__packed__)) *psWriteAttributeResponse = NULL;
    
    
    struct _DataIndication
    {
        /**\todo handle data indication properly */
        uint8_t     u8ZCBStatus;
        uint16_t    u16ProfileID;
        uint16_t    u16ClusterID;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8SourceAddressMode;
        uint16_t    u16SourceShortAddress; /* OR uint64_t u64IEEEAddress */
        uint8_t     u8DestinationAddressMode;
        uint16_t    u16DestinationShortAddress; /* OR uint64_t u64IEEEAddress */
        
        uint8_t     u8FrameControl;
        uint8_t     u8SequenceNo;
        uint8_t     u8Command;
        uint8_t     u8Status;
        uint16_t    u16AttributeID;
    } __attribute__((__packed__)) *psDataIndication = NULL;

    
    uint16_t u16Length = sizeof(struct _WriteAttributeRequest) - sizeof(sWriteAttributeRequest.uData);
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send Write Attribute request to 0x%04X\n", psZCBNode->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sWriteAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sWriteAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sWriteAttributeRequest.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
    
    if ((eStatus = eZCB_GetEndpoints(psZCBNode, u16ClusterID, &sWriteAttributeRequest.u8SourceEndpoint, &sWriteAttributeRequest.u8DestinationEndpoint)) != E_ZCB_OK)
    {
        goto done;
    }
    
    sWriteAttributeRequest.u16ClusterID             = htons(u16ClusterID);
    sWriteAttributeRequest.u8Direction              = u8Direction;
    sWriteAttributeRequest.u8ManufacturerSpecific   = u8ManufacturerSpecific;
    sWriteAttributeRequest.u16ManufacturerID        = htons(u16ManufacturerID);
    sWriteAttributeRequest.u8NumAttributes          = 1;
    sWriteAttributeRequest.u16AttributeID           = htons(u16AttributeID);
    sWriteAttributeRequest.u8Type                   = (uint8_t)eType;
    
    switch(eType)
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        case(E_ZCL_OSTRING):
        case(E_ZCL_CSTRING):
            memcpy(&sWriteAttributeRequest.uData.u8Data, pvData, sizeof(uint8_t));
			u16Length += sizeof(uint8_t);
            break;
        
        case(E_ZCL_LOSTRING):
        case(E_ZCL_LCSTRING):
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
            memcpy(&sWriteAttributeRequest.uData.u16Data, pvData, sizeof(uint16_t));
            sWriteAttributeRequest.uData.u16Data = ntohs(sWriteAttributeRequest.uData.u16Data);
			u16Length += sizeof(uint16_t);
            break;
 
        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
            memcpy(&sWriteAttributeRequest.uData.u32Data, pvData, sizeof(uint32_t));
            sWriteAttributeRequest.uData.u32Data = ntohl(sWriteAttributeRequest.uData.u32Data);
			u16Length += sizeof(uint32_t);
            break;
 
        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
            memcpy(&sWriteAttributeRequest.uData.u64Data, pvData, sizeof(uint64_t));
            sWriteAttributeRequest.uData.u64Data = be64toh(sWriteAttributeRequest.uData.u64Data);
			u16Length += sizeof(uint64_t);
            break;
            
        default:
            daemon_log(LOG_ERR, "Unknown attribute data type (%d)", eType);
            return E_ZCB_ERROR;
    }
    
    if (eSL_SendMessage(E_SL_MSG_WRITE_ATTRIBUTE_REQUEST, u16Length, &sWriteAttributeRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the message to arrive */
        /**\todo handle data indication here for now - BAD Idea! Implement a general case handler in future! */
        if (eSL_MessageWait(E_SL_MSG_DATA_INDICATION, 1000, &u16Length, (void**)&psDataIndication) != E_SL_OK)
        {
            if (verbosity > LOG_INFO)
            {
                daemon_log(LOG_DEBUG, "No response to write attribute request");
            }
            eStatus = E_ZCB_COMMS_FAILED;
            goto done;
        }
        
        DBG_vPrintf(DBG_ZCB, "Got data indication\n");
        
        if (u8SequenceNo == psDataIndication->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Write Attribute sequence number received 0x%02X does not match that sent 0x%02X\n", psDataIndication->u8SequenceNo, u8SequenceNo);
            free(psDataIndication);
            psDataIndication = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Got write attribute response\n");
    
    eStatus = psDataIndication->u8Status;

done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(psDataIndication);
    return eStatus;
}


teZcbStatus eZCB_GetDefaultResponse(uint8_t u8SequenceNo)
{
    uint16_t u16Length;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    tsSL_Msg_DefaultResponse *psDefaultResponse = NULL;

    while (1)
    {
        /* Wait 1 second for a default response message to arrive */
        if (eSL_MessageWait(E_SL_MSG_DEFAULT_RESPONSE, 1000, &u16Length, (void**)&psDefaultResponse) != E_SL_OK)
        {
            daemon_log(LOG_ERR, "No response to command sequence number %d received", u8SequenceNo);
            goto done;
        }
        
        if (u8SequenceNo != psDefaultResponse->u8SequenceNo)
        {
            DBG_vPrintf(DBG_ZCB, "Default response sequence number received 0x%02X does not match that sent 0x%02X\n", psDefaultResponse->u8SequenceNo, u8SequenceNo);
            free(psDefaultResponse);
            psDefaultResponse = NULL;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Default response for message sequence number 0x%02X status is %d\n", psDefaultResponse->u8SequenceNo, psDefaultResponse->u8Status);
            eStatus = psDefaultResponse->u8Status;
            break;
        }
    }
done:
    free(psDefaultResponse);
    return eStatus;
}


teZcbStatus eZCB_AddGroupMembership(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress)
{
    struct _AddGroupMembershipRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16GroupAddress;
    } __attribute__((__packed__)) sAddGroupMembershipRequest;
    
    struct _sAddGroupMembershipResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Status;
        uint16_t    u16GroupAddress;
    } __attribute__((__packed__)) *psAddGroupMembershipResponse = NULL;
    
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send add group membership 0x%04X request to 0x%04X\n", u16GroupAddress, psZCBNode->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sAddGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sAddGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sAddGroupMembershipRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);
    
    if ((eStatus = eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_GROUPS, &sAddGroupMembershipRequest.u8SourceEndpoint, &sAddGroupMembershipRequest.u8DestinationEndpoint)) != E_ZCB_OK)
    {
        return eStatus;
    }
    
    sAddGroupMembershipRequest.u16GroupAddress = htons(u16GroupAddress);

    if (eSL_SendMessage(E_SL_MSG_ADD_GROUP_REQUEST, sizeof(struct _AddGroupMembershipRequest), &sAddGroupMembershipRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the add group response message to arrive */
        if (eSL_MessageWait(E_SL_MSG_ADD_GROUP_RESPONSE, 1000, &u16Length, (void**)&psAddGroupMembershipResponse) != E_SL_OK)
        {
            daemon_log(LOG_ERR, "No response to add group membership request");
            goto done;
        }
        
        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psAddGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Add group membership sequence number received 0x%02X does not match that sent 0x%02X\n", psAddGroupMembershipResponse->u8SequenceNo, u8SequenceNo);
            free(psAddGroupMembershipResponse);
            psAddGroupMembershipResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Add group membership 0x%04X on Node 0x%04X status: %d\n", u16GroupAddress, psZCBNode->u16ShortAddress, psAddGroupMembershipResponse->u8Status);
    
    eStatus = psAddGroupMembershipResponse->u8Status;

done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(psAddGroupMembershipResponse);
    return eStatus;
}


teZcbStatus eZCB_RemoveGroupMembership(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress)
{
    struct _RemoveGroupMembershipRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16GroupAddress;
    } __attribute__((__packed__)) sRemoveGroupMembershipRequest;
    
    struct _sRemoveGroupMembershipResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Status;
        uint16_t    u16GroupAddress;
    } __attribute__((__packed__)) *psRemoveGroupMembershipResponse = NULL;
    
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send remove group membership 0x%04X request to 0x%04X\n", u16GroupAddress, psZCBNode->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sRemoveGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sRemoveGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sRemoveGroupMembershipRequest.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
    
    if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_GROUPS, &sRemoveGroupMembershipRequest.u8SourceEndpoint, &sRemoveGroupMembershipRequest.u8DestinationEndpoint) != E_ZCB_OK)
    {
        return E_ZCB_ERROR;
    }
    
    sRemoveGroupMembershipRequest.u16GroupAddress = htons(u16GroupAddress);

    if (eSL_SendMessage(E_SL_MSG_REMOVE_GROUP_REQUEST, sizeof(struct _RemoveGroupMembershipRequest), &sRemoveGroupMembershipRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the remove group response message to arrive */
        if (eSL_MessageWait(E_SL_MSG_REMOVE_GROUP_RESPONSE, 1000, &u16Length, (void**)&psRemoveGroupMembershipResponse) != E_SL_OK)
        {
            daemon_log(LOG_ERR, "No response to remove group membership request");
            goto done;
        }
        
        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psRemoveGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Remove group membership sequence number received 0x%02X does not match that sent 0x%02X\n", psRemoveGroupMembershipResponse->u8SequenceNo, u8SequenceNo);
            free(psRemoveGroupMembershipResponse);
            psRemoveGroupMembershipResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Remove group membership 0x%04X on Node 0x%04X status: %d\n", u16GroupAddress, psZCBNode->u16ShortAddress, psRemoveGroupMembershipResponse->u8Status);
    
    eStatus = psRemoveGroupMembershipResponse->u8Status;

done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(psRemoveGroupMembershipResponse);
    return eStatus;
}


teZcbStatus eZCB_GetGroupMembership(tsZCB_Node *psZCBNode)
{
    struct _GetGroupMembershipRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8GroupCount;
        uint16_t    au16GroupList[0];
    } __attribute__((__packed__)) sGetGroupMembershipRequest;
    
    struct _sGetGroupMembershipResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Capacity;
        uint8_t     u8GroupCount;
        uint16_t    au16GroupList[255];
    } __attribute__((__packed__)) *psGetGroupMembershipResponse = NULL;
    
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    int i;
    
    DBG_vPrintf(DBG_ZCB, "Send get group membership request to 0x%04X\n", psZCBNode->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sGetGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sGetGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sGetGroupMembershipRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);
    
    if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_GROUPS, &sGetGroupMembershipRequest.u8SourceEndpoint, &sGetGroupMembershipRequest.u8DestinationEndpoint) != E_ZCB_OK)
    {
        return E_ZCB_ERROR;
    }
    
    sGetGroupMembershipRequest.u8GroupCount     = 0;

    if (eSL_SendMessage(E_SL_MSG_GET_GROUP_MEMBERSHIP_REQUEST, sizeof(struct _GetGroupMembershipRequest), &sGetGroupMembershipRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_GET_GROUP_MEMBERSHIP_RESPONSE, 1000, &u16Length, (void**)&psGetGroupMembershipResponse) != E_SL_OK)
        {
            daemon_log(LOG_ERR, "No response to group membership request");
            goto done;
        }
        
        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psGetGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Get group membership sequence number received 0x%02X does not match that sent 0x%02X\n", psGetGroupMembershipResponse->u8SequenceNo, u8SequenceNo);
            free(psGetGroupMembershipResponse);
            psGetGroupMembershipResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Node 0x%04X is in %d/%d groups\n", psZCBNode->u16ShortAddress,
                psGetGroupMembershipResponse->u8GroupCount, 
                psGetGroupMembershipResponse->u8GroupCount + psGetGroupMembershipResponse->u8Capacity);

    if (eZCB_NodeClearGroups(psZCBNode) != E_ZCB_OK)
    {
        goto done;
    }
    
    for(i = 0; i < psGetGroupMembershipResponse->u8GroupCount; i++)
    {
        psGetGroupMembershipResponse->au16GroupList[i] = ntohs(psGetGroupMembershipResponse->au16GroupList[i]);
        DBG_vPrintf(DBG_ZCB, "  Group ID 0x%04X\n", psGetGroupMembershipResponse->au16GroupList[i]);
        if ((eStatus = eZCB_NodeAddGroup(psZCBNode, psGetGroupMembershipResponse->au16GroupList[i])) != E_ZCB_OK)
        {
            goto done;
        }
    }
    eStatus = E_ZCB_OK;
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(psGetGroupMembershipResponse);
    return eStatus;
}


teZcbStatus eZCB_ClearGroupMembership(tsZCB_Node *psZCBNode)
{
    struct _ClearGroupMembershipRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
    } __attribute__((__packed__)) sClearGroupMembershipRequest;
    
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send clear group membership request to 0x%04X\n", psZCBNode->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sClearGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sClearGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sClearGroupMembershipRequest.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
    
    if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_GROUPS, &sClearGroupMembershipRequest.u8SourceEndpoint, &sClearGroupMembershipRequest.u8DestinationEndpoint) != E_ZCB_OK)
    {
        return E_ZCB_ERROR;
    }

    if (eSL_SendMessage(E_SL_MSG_REMOVE_ALL_GROUPS, sizeof(struct _ClearGroupMembershipRequest), &sClearGroupMembershipRequest, NULL) != E_SL_OK)
    {
        goto done;
    }
    eStatus = E_ZCB_OK;
    
done:
    return eStatus;
}


teZcbStatus eZCB_RemoveScene(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8SceneID)
{
    struct _RemoveSceneRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16GroupAddress;
        uint8_t     u8SceneID;
    } __attribute__((__packed__)) sRemoveSceneRequest;
    
    struct _sStoreSceneResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Status;
        uint16_t    u16GroupAddress;
        uint8_t     u8SceneID;
    } __attribute__((__packed__)) *psRemoveSceneResponse = NULL;
    
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    DBG_vPrintf(DBG_ZCB, "Send remove scene %d (Group 0x%04X) for Endpoint %d to 0x%04X\n", 
                u8SceneID, u16GroupAddress, sRemoveSceneRequest.u8DestinationEndpoint, psZCBNode->u16ShortAddress);

    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sRemoveSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sRemoveSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sRemoveSceneRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);
        
        if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_SCENES, &sRemoveSceneRequest.u8SourceEndpoint, &sRemoveSceneRequest.u8DestinationEndpoint) != E_ZCB_OK)
        {
            return E_ZCB_ERROR;
        }
    }
    else
    {
        sRemoveSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sRemoveSceneRequest.u16TargetAddress      = htons(u16GroupAddress);
        sRemoveSceneRequest.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        
        if (eZCB_GetEndpoints(NULL, E_ZB_CLUSTERID_SCENES, &sRemoveSceneRequest.u8SourceEndpoint, NULL) != E_ZCB_OK)
        {
            return E_ZCB_ERROR;
        }
    }
    
    sRemoveSceneRequest.u16GroupAddress  = htons(u16GroupAddress);
    sRemoveSceneRequest.u8SceneID        = u8SceneID;

    if (eSL_SendMessage(E_SL_MSG_REMOVE_SCENE, sizeof(struct _RemoveSceneRequest), &sRemoveSceneRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_REMOVE_SCENE_RESPONSE, 1000, &u16Length, (void**)&psRemoveSceneResponse) != E_SL_OK)
        {
            daemon_log(LOG_ERR, "No response to remove scene request");
            goto done;
        }
        
        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psGetGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Remove scene sequence number received 0x%02X does not match that sent 0x%02X\n", psRemoveSceneResponse->u8SequenceNo, u8SequenceNo);
            free(psRemoveSceneResponse);
            psRemoveSceneResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Remove scene %d (Group0x%04X) on Node 0x%04X status: %d\n", 
                psRemoveSceneResponse->u8SceneID, ntohs(psRemoveSceneResponse->u16GroupAddress), psZCBNode->u16ShortAddress, psRemoveSceneResponse->u8Status);
    
    eStatus = psRemoveSceneResponse->u8Status;
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(psRemoveSceneResponse);
    return eStatus;
}


teZcbStatus eZCB_StoreScene(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8SceneID)
{
    struct _StoreSceneRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16GroupAddress;
        uint8_t     u8SceneID;
    } __attribute__((__packed__)) sStoreSceneRequest;
    
    struct _sStoreSceneResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Status;
        uint16_t    u16GroupAddress;
        uint8_t     u8SceneID;
    } __attribute__((__packed__)) *psStoreSceneResponse = NULL;
    
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    DBG_vPrintf(DBG_ZCB, "Send store scene %d (Group 0x%04X)\n", 
                u8SceneID, u16GroupAddress);
    
    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sStoreSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sStoreSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sStoreSceneRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);
        
        if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_SCENES, &sStoreSceneRequest.u8SourceEndpoint, &sStoreSceneRequest.u8DestinationEndpoint) != E_ZCB_OK)
        {
            return E_ZCB_ERROR;
        }
    }
    else
    {
        sStoreSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sStoreSceneRequest.u16TargetAddress      = htons(u16GroupAddress);
        sStoreSceneRequest.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        
        if (eZCB_GetEndpoints(NULL, E_ZB_CLUSTERID_SCENES, &sStoreSceneRequest.u8SourceEndpoint, NULL) != E_ZCB_OK)
        {
            return E_ZCB_ERROR;
        }
    }
    
    sStoreSceneRequest.u16GroupAddress  = htons(u16GroupAddress);
    sStoreSceneRequest.u8SceneID        = u8SceneID;

    if (eSL_SendMessage(E_SL_MSG_STORE_SCENE, sizeof(struct _StoreSceneRequest), &sStoreSceneRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_STORE_SCENE_RESPONSE, 1000, &u16Length, (void**)&psStoreSceneResponse) != E_SL_OK)
        {
            daemon_log(LOG_ERR, "No response to store scene request");
            goto done;
        }
        
        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psGetGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Store scene sequence number received 0x%02X does not match that sent 0x%02X\n", psStoreSceneResponse->u8SequenceNo, u8SequenceNo);
            free(psStoreSceneResponse);
            psStoreSceneResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Store scene %d (Group0x%04X) on Node 0x%04X status: %d\n", 
                psStoreSceneResponse->u8SceneID, ntohs(psStoreSceneResponse->u16GroupAddress), ntohs(psZCBNode->u16ShortAddress), psStoreSceneResponse->u8Status);
    
    eStatus = psStoreSceneResponse->u8Status;
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(psStoreSceneResponse);
    return eStatus;
}


teZcbStatus eZCB_RecallScene(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8SceneID)
{
    uint8_t         u8SequenceNo;
    struct _RecallSceneRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16GroupAddress;
        uint8_t     u8SceneID;
    } __attribute__((__packed__)) sRecallSceneRequest;

    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    if (psZCBNode)
    {
        DBG_vPrintf(DBG_ZCB, "Send recall scene %d (Group 0x%04X) to 0x%04X\n", 
                u8SceneID, u16GroupAddress, psZCBNode->u16ShortAddress);
        
        if (bZCB_EnableAPSAck)
        {
            sRecallSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sRecallSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sRecallSceneRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);
        
        if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_SCENES, &sRecallSceneRequest.u8SourceEndpoint, &sRecallSceneRequest.u8DestinationEndpoint) != E_ZCB_OK)
        {
            return E_ZCB_ERROR;
        }
    }
    else
    {   
        sRecallSceneRequest.u8TargetAddressMode  = E_ZB_ADDRESS_MODE_GROUP;
        sRecallSceneRequest.u16TargetAddress     = htons(u16GroupAddress);
        sRecallSceneRequest.u8DestinationEndpoint= ZB_DEFAULT_ENDPOINT_ZLL;
        
        DBG_vPrintf(DBG_ZCB, "Send recall scene %d (Group 0x%04X) for Endpoint %d to 0x%04X\n", 
                    u8SceneID, u16GroupAddress, sRecallSceneRequest.u8DestinationEndpoint, u16GroupAddress);
        
        if (eZCB_GetEndpoints(NULL, E_ZB_CLUSTERID_SCENES, &sRecallSceneRequest.u8SourceEndpoint, NULL) != E_ZCB_OK)
        {
            return E_ZCB_ERROR;
        }
    }

    sRecallSceneRequest.u16GroupAddress  = htons(u16GroupAddress);
    sRecallSceneRequest.u8SceneID        = u8SceneID;
    
    if (eSL_SendMessage(E_SL_MSG_RECALL_SCENE, sizeof(struct _RecallSceneRequest), &sRecallSceneRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    if (psZCBNode)
    {
        eStatus = eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        eStatus = E_ZCB_OK;
    }
done:
    return eStatus;
}


teZcbStatus eZCB_GetSceneMembership(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t *pu8NumScenes, uint8_t **pau8Scenes)
{
    struct _GetSceneMembershipRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16GroupAddress;
    } __attribute__((__packed__)) sGetSceneMembershipRequest;
    
    struct _sGetSceneMembershipResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Status;
        uint8_t     u8Capacity;
        uint16_t    u16GroupAddress;
        uint8_t     u8NumScenes;
        uint8_t     au8Scenes[255];
    } __attribute__((__packed__)) *psGetSceneMembershipResponse = NULL;
    
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    
    DBG_vPrintf(DBG_ZCB, "Send get scene membership for group 0x%04X to 0x%04X\n", 
                u16GroupAddress, psZCBNode->u16ShortAddress);
    
    if (bZCB_EnableAPSAck)
    {
        sGetSceneMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sGetSceneMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sGetSceneMembershipRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);
    
    if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_SCENES, &sGetSceneMembershipRequest.u8SourceEndpoint, &sGetSceneMembershipRequest.u8DestinationEndpoint) != E_ZCB_OK)
    {
        return E_ZCB_ERROR;
    }
    
    sGetSceneMembershipRequest.u16GroupAddress  = htons(u16GroupAddress);

    if (eSL_SendMessage(E_SL_MSG_SCENE_MEMBERSHIP_REQUEST, sizeof(struct _GetSceneMembershipRequest), &sGetSceneMembershipRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }
    
    while (1)
    {
        /* Wait 1 second for the response to arrive */
        if (eSL_MessageWait(E_SL_MSG_SCENE_MEMBERSHIP_RESPONSE, 1000, &u16Length, (void**)&psGetSceneMembershipResponse) != E_SL_OK)
        {
            daemon_log(LOG_ERR, "No response to get scene membership request");
            goto done;
        }
        
        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psGetGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Get scene membership sequence number received 0x%02X does not match that sent 0x%02X\n", psGetSceneMembershipResponse->u8SequenceNo, u8SequenceNo);
            free(psGetSceneMembershipResponse);
            psGetSceneMembershipResponse = NULL;
        }
    }
    
    DBG_vPrintf(DBG_ZCB, "Scene membership for group 0x%04X on Node 0x%04X status: %d\n", 
                ntohs(psGetSceneMembershipResponse->u16GroupAddress), psZCBNode->u16ShortAddress, psGetSceneMembershipResponse->u8Status);
    
    eStatus = psGetSceneMembershipResponse->u8Status;
    
    if (eStatus == E_ZCB_OK)
    {
        int i;
        DBG_vPrintf(DBG_ZCB, "Node 0x%04X, group 0x%04X is in %d scenes\n", 
                psZCBNode->u16ShortAddress, ntohs(psGetSceneMembershipResponse->u16GroupAddress), psGetSceneMembershipResponse->u8NumScenes);
    
        *pu8NumScenes = psGetSceneMembershipResponse->u8NumScenes;
        *pau8Scenes = realloc(*pau8Scenes, *pu8NumScenes * sizeof(uint8_t));
        if (!pu8NumScenes)
        {
            return E_ZCB_ERROR_NO_MEM;
        }
        
        for (i = 0; i < psGetSceneMembershipResponse->u8NumScenes; i++)
        {
            DBG_vPrintf(DBG_ZCB, "  Scene 0x%02X\n", psGetSceneMembershipResponse->au8Scenes[i]);
            (*pau8Scenes)[i] = psGetSceneMembershipResponse->au8Scenes[i];
        }
    }
    
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    free(psGetSceneMembershipResponse);
    return eStatus;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

static void ZCB_HandleNodeClusterList(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    int iPosition;
    int iCluster = 0;
    struct _tsClusterList
    {
        uint8_t     u8Endpoint;
        uint16_t    u16ProfileID;
        uint16_t    au16ClusterList[255];
    } __attribute__((__packed__)) *psClusterList = (struct _tsClusterList *)pvMessage;
    
    psClusterList->u16ProfileID = ntohs(psClusterList->u16ProfileID);
    
    DBG_vPrintf(DBG_ZCB, "Cluster list for endpoint %d, profile ID 0x%4X\n", 
                psClusterList->u8Endpoint, 
                psClusterList->u16ProfileID);
    
    eUtils_LockLock(&sZCB_Network.sNodes.sLock);

    if (eZCB_NodeAddEndpoint(&sZCB_Network.sNodes, psClusterList->u8Endpoint, psClusterList->u16ProfileID, NULL) != E_ZCB_OK)
    {
        goto done;
    }

    iPosition = sizeof(uint8_t) + sizeof(uint16_t);
    while(iPosition < u16Length)
    {
        if (eZCB_NodeAddCluster(&sZCB_Network.sNodes, psClusterList->u8Endpoint, ntohs(psClusterList->au16ClusterList[iCluster])) != E_ZCB_OK)
        {
            goto done;
        }
        iPosition += sizeof(uint16_t);
        iCluster++;
    }
    
    DBG_PrintNode(&sZCB_Network.sNodes);
done:
    eUtils_LockUnlock(&sZCB_Network.sNodes.sLock);
}


static void ZCB_HandleNodeClusterAttributeList(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    int iPosition;
    int iAttribute = 0;
    struct _tsClusterAttributeList
    {
        uint8_t     u8Endpoint;
        uint16_t    u16ProfileID;
        uint16_t    u16ClusterID;
        uint16_t    au16AttributeList[255];
    } __attribute__((__packed__)) *psClusterAttributeList = (struct _tsClusterAttributeList *)pvMessage;
    
    psClusterAttributeList->u16ProfileID = ntohs(psClusterAttributeList->u16ProfileID);
    psClusterAttributeList->u16ClusterID = ntohs(psClusterAttributeList->u16ClusterID);
    
    DBG_vPrintf(DBG_ZCB, "Cluster attribute list for endpoint %d, cluster 0x%04X, profile ID 0x%4X\n", 
                psClusterAttributeList->u8Endpoint, 
                psClusterAttributeList->u16ClusterID,
                psClusterAttributeList->u16ProfileID);
    
    eUtils_LockLock(&sZCB_Network.sNodes.sLock);

    iPosition = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t);
    while(iPosition < u16Length)
    {
        if (eZCB_NodeAddAttribute(&sZCB_Network.sNodes, psClusterAttributeList->u8Endpoint, 
            psClusterAttributeList->u16ClusterID, ntohs(psClusterAttributeList->au16AttributeList[iAttribute])) != E_ZCB_OK)
        {
            goto done;
        }
        iPosition += sizeof(uint16_t);
        iAttribute++;
    }    

    DBG_PrintNode(&sZCB_Network.sNodes);
    
done:
    eUtils_LockUnlock(&sZCB_Network.sNodes.sLock);
}


static void ZCB_HandleNodeCommandIDList(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    int iPosition;
    int iCommand = 0;
    struct _tsCommandIDList
    {
        uint8_t     u8Endpoint;
        uint16_t    u16ProfileID;
        uint16_t    u16ClusterID;
        uint8_t     au8CommandList[255];
    } __attribute__((__packed__)) *psCommandIDList = (struct _tsCommandIDList *)pvMessage;
    
    psCommandIDList->u16ProfileID = ntohs(psCommandIDList->u16ProfileID);
    psCommandIDList->u16ClusterID = ntohs(psCommandIDList->u16ClusterID);
    
    DBG_vPrintf(DBG_ZCB, "Command ID list for endpoint %d, cluster 0x%04X, profile ID 0x%4X\n", 
                psCommandIDList->u8Endpoint, 
                psCommandIDList->u16ClusterID,
                psCommandIDList->u16ProfileID);
    
    eUtils_LockLock(&sZCB_Network.sNodes.sLock);
    
    iPosition = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t);
    while(iPosition < u16Length)
    {
        if (eZCB_NodeAddCommand(&sZCB_Network.sNodes, psCommandIDList->u8Endpoint, 
            psCommandIDList->u16ClusterID, psCommandIDList->au8CommandList[iCommand]) != E_ZCB_OK)
        {
            goto done;
        }
        iPosition += sizeof(uint8_t);
        iCommand++;
    }    

    DBG_PrintNode(&sZCB_Network.sNodes);
done:
    eUtils_LockUnlock(&sZCB_Network.sNodes.sLock);
}


static void ZCB_HandleRestartProvisioned(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    const char *pcStatus = NULL;
    
    struct _tsWarmRestart
    {
        uint8_t     u8Status;
    } __attribute__((__packed__)) *psWarmRestart = (struct _tsWarmRestart *)pvMessage;

    switch (psWarmRestart->u8Status)
    {
#define STATUS(a, b) case(a): pcStatus = b; break
        STATUS(0, "STARTUP");
        STATUS(1, "WAIT_START");
        STATUS(2, "NFN_START");
        STATUS(3, "DISCOVERY");
        STATUS(4, "NETWORK_INIT");
        STATUS(5, "RESCAN");
        STATUS(6, "RUNNING");
#undef STATUS
        default: pcStatus = "Unknown";
    }
    daemon_log(LOG_ERR, "Control bridge restarted, status %d (%s)", psWarmRestart->u8Status, pcStatus);
    return;
}


static void ZCB_HandleRestartFactoryNew(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    const char *pcStatus = NULL;
    
    struct _tsWarmRestart
    {
        uint8_t     u8Status;
    } __attribute__((__packed__)) *psWarmRestart = (struct _tsWarmRestart *)pvMessage;

    switch (psWarmRestart->u8Status)
    {
#define STATUS(a, b) case(a): pcStatus = b; break
        STATUS(0, "STARTUP");
        STATUS(1, "WAIT_START");
        STATUS(2, "NFN_START");
        STATUS(3, "DISCOVERY");
        STATUS(4, "NETWORK_INIT");
        STATUS(5, "RESCAN");
        STATUS(6, "RUNNING");
#undef STATUS
        default: pcStatus = "Unknown";
    }
    daemon_log(LOG_ERR, "Control bridge factory new restart, status %d (%s)", psWarmRestart->u8Status, pcStatus);
    
    eZCB_ConfigureControlBridge();
    return;
}



static void ZCB_HandleNetworkJoined(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    struct _tsNetworkJoinedFormedShort
    {
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint64_t    u64IEEEAddress;
        uint8_t     u8Channel;
    } __attribute__((__packed__)) *psMessageShort = (struct _tsNetworkJoinedFormedShort *)pvMessage;
    
    struct _tsNetworkJoinedFormedExtended
    {
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint64_t    u64IEEEAddress;
        uint8_t     u8Channel;
        uint64_t    u64PanID;
        uint16_t    u16PanID;
    } __attribute__((__packed__)) *psMessageExt = (struct _tsNetworkJoinedFormedExtended *)pvMessage;
    tsZcbEvent *psEvent;

    psMessageShort->u16ShortAddress = ntohs(psMessageShort->u16ShortAddress);
    psMessageShort->u64IEEEAddress  = be64toh(psMessageShort->u64IEEEAddress);
    
    if (u16Length == sizeof(struct _tsNetworkJoinedFormedExtended))
    {
        psMessageExt->u64PanID      = be64toh(psMessageExt->u64PanID);
        psMessageExt->u16PanID      = ntohs(psMessageExt->u16PanID);
        
        daemon_log(LOG_INFO, "Network %s on channel %d. Control bridge address 0x%04X (0x%016llX). PAN ID 0x%04X (0x%016llX)", 
                psMessageExt->u8Status == 0 ? "joined" : "formed",
                psMessageExt->u8Channel,
                psMessageExt->u16ShortAddress,
                (unsigned long long int)psMessageExt->u64IEEEAddress,
                psMessageExt->u16PanID,
                (unsigned long long int)psMessageExt->u64PanID);
        
        /* Update global network information */
        eChannelInUse = psMessageExt->u8Channel;
        u64PanIDInUse = psMessageExt->u64PanID;
        u16PanIDInUse = psMessageExt->u16PanID;
    }
    else
    {
        daemon_log(LOG_INFO, "Network %s on channel %d. Control bridge address 0x%04X (0x%016llX)", 
                    psMessageShort->u8Status == 0 ? "joined" : "formed",
                    psMessageShort->u8Channel,
                    psMessageShort->u16ShortAddress,
                    (unsigned long long int)psMessageShort->u64IEEEAddress);
    }
    

    /* Control bridge joined the network - initialise its data in the network structure */
    eUtils_LockLock(&sZCB_Network.sNodes.sLock);
    
    sZCB_Network.sNodes.u16DeviceID     = E_ZB_DEVICEID_CONTROLBRIDGE;
    sZCB_Network.sNodes.u16ShortAddress = psMessageShort->u16ShortAddress;
    sZCB_Network.sNodes.u64IEEEAddress  = psMessageShort->u64IEEEAddress;
    sZCB_Network.sNodes.u8MacCapability = E_ZB_MAC_CAPABILITY_RXON_WHEN_IDLE;
    
    DBG_vPrintf(DBG_ZCB, "Node Joined 0x%04X (0x%016llX)\n", 
                sZCB_Network.sNodes.u16ShortAddress, 
                (unsigned long long int)sZCB_Network.sNodes.u64IEEEAddress);    


    psEvent = malloc(sizeof(tsZcbEvent));
    if (!psEvent)
    {
        daemon_log(LOG_CRIT, "Memory allocation failure allocating event");
        return;
    }
    
    psEvent->eEvent = (psMessageShort->u8Status == 0 ? E_ZCB_EVENT_NETWORK_JOINED : E_ZCB_EVENT_NETWORK_FORMED);
 
    eUtils_LockUnlock(&sZCB_Network.sNodes.sLock);
    if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != E_UTILS_OK)
    {
        DBG_vPrintf(DBG_ZCB, "Error queue'ing event\n");
        free(psEvent);
    }
    
    /* Test thermostat */
#if 0
    {
        tsZCB_Node *psZCBNode;
        if (eZCB_AddNode(0x1234, 0x1234ll, 0, 0, &psZCBNode) == E_ZCB_OK)
        {
            psEvent = malloc(sizeof(tsZcbEvent));
            
            if (!psEvent)
            {
                daemon_log(LOG_CRIT, "Memory allocation failure allocating event");
                return;
            }
            psEvent->eEvent                                 = E_ZCB_EVENT_DEVICE_MATCH;
            psEvent->uData.sDeviceMatch.u16ShortAddress     = 0x1234;      
            
            psZCBNode->u16DeviceID = 0x0301;
            
            eUtils_LockUnlock(&psZCBNode->sLock);
            
            
            if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != E_UTILS_OK)
            {
                DBG_vPrintf(DBG_ZCB, "Error queue'ing event\n");
                free(psEvent);
            }
            else
            {
                DBG_vPrintf(DBG_ZCB, "Test device added\n");
            }
        }
    }
#endif
    return;
}



static void ZCB_HandleDeviceAnnounce(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    tsZCB_Node *psZCBNode;
    struct _tsDeviceAnnounce
    {
        uint16_t    u16ShortAddress;
        uint64_t    u64IEEEAddress;
        uint8_t     u8MacCapability;
    } __attribute__((__packed__)) *psMessage = (struct _tsDeviceAnnounce *)pvMessage;
    
    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u64IEEEAddress   = be64toh(psMessage->u64IEEEAddress);
    
    DBG_vPrintf(DBG_ZCB, "Device Joined, Address 0x%04X (0x%016llX). Mac Capability Mask 0x%02X\n", 
                psMessage->u16ShortAddress,
                (unsigned long long int)psMessage->u64IEEEAddress,
                psMessage->u8MacCapability
               );
    
    tsZcbEvent *psEvent = malloc(sizeof(tsZcbEvent));
    if (!psEvent)
    {
        daemon_log(LOG_CRIT, "Memory allocation failure allocating event");
        return;
    }
    
    if (eZCB_AddNode(psMessage->u16ShortAddress, psMessage->u64IEEEAddress, 0, psMessage->u8MacCapability, &psZCBNode) == E_ZCB_OK)
    {
        psEvent->eEvent                                 = E_ZCB_EVENT_DEVICE_ANNOUNCE;
        psEvent->uData.sDeviceAnnounce.u16ShortAddress  = psZCBNode->u16ShortAddress;
        
        psZCBNode->u8MacCapability = psMessage->u8MacCapability;
        
        eUtils_LockUnlock(&psZCBNode->sLock);

        if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != E_UTILS_OK)
        {
            DBG_vPrintf(DBG_ZCB, "Error queue'ing event\n");
            free(psEvent);
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "Device join queued\n");
        }
    }
    return;
}


static void ZCB_HandleDeviceLeave(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    struct _tsDeviceLeave
    {
        uint64_t    u64IEEEAddress;
        uint8_t     bRejoin;
    } __attribute__((__packed__)) *psMessage = (struct _tsDeviceLeave *)pvMessage;
    
    psMessage->u64IEEEAddress   = be64toh(psMessage->u64IEEEAddress);
    
    DBG_vPrintf(DBG_ZCB, "Device Left, Address 0x%016llX, rejoining: %d\n", 
                (unsigned long long int)psMessage->u64IEEEAddress,
                psMessage->bRejoin
               );

    tsZcbEvent *psEvent = malloc(sizeof(tsZcbEvent));
    if (!psEvent)
    {
        daemon_log(LOG_CRIT, "Memory allocation failure allocating event");
        return;
    }
    
    psEvent->eEvent                                 = E_ZCB_EVENT_DEVICE_LEFT;
    psEvent->uData.sDeviceLeft.u64IEEEAddress       = psMessage->u64IEEEAddress;
    psEvent->uData.sDeviceLeft.bRejoin              = psMessage->bRejoin;
    
    if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != E_UTILS_OK)
    {
        DBG_vPrintf(DBG_ZCB, "Error queue'ing event\n");
        free(psEvent);
    }
    else
    {
        DBG_vPrintf(DBG_ZCB, "Device leave queued\n");
    }
    return;
}


static void ZCB_HandleMatchDescriptorResponse(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    tsZCB_Node *psZCBNode;
    struct _tMatchDescriptorResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint8_t     u8NumEndpoints;
        uint8_t     au8Endpoints[255];
    } __attribute__((__packed__)) *psMatchDescriptorResponse = (struct _tMatchDescriptorResponse *)pvMessage;

    tsZcbEvent *psEvent = malloc(sizeof(tsZcbEvent));
    if (!psEvent)
    {
        daemon_log(LOG_CRIT, "Memory allocation failure allocating event");
        return;
    }
    
    psMatchDescriptorResponse->u16ShortAddress  = ntohs(psMatchDescriptorResponse->u16ShortAddress);
    
    DBG_vPrintf(DBG_ZCB, "Match descriptor request response from node 0x%04X - %d matching endpoints.\n", 
                psMatchDescriptorResponse->u16ShortAddress,
                psMatchDescriptorResponse->u8NumEndpoints
               );
    if (psMatchDescriptorResponse->u8NumEndpoints)
    {
        // Device has matching endpoints
#if DBG_ZCB
        if ((psZCBNode = psZCB_FindNodeShortAddress(psMatchDescriptorResponse->u16ShortAddress)) != NULL)
        {
            DBG_vPrintf(DBG_ZCB, "Node rejoined\n");
            eUtils_LockUnlock(&psZCBNode->sLock);
        }
        else
        {
            DBG_vPrintf(DBG_ZCB, "New node\n");
        }
#endif
        
        if (eZCB_AddNode(psMatchDescriptorResponse->u16ShortAddress, 0, 0, 0, &psZCBNode) == E_ZCB_OK)
        {
            int i;
            for (i = 0; i < psMatchDescriptorResponse->u8NumEndpoints; i++)
            {
                /* Add an endpoint to the device for each response in the match descriptor response */
                if (eZCB_NodeAddEndpoint(psZCBNode, psMatchDescriptorResponse->au8Endpoints[i], 0, NULL) != E_ZCB_OK)            
                {
                    return;
                }
            }
            
            psEvent->eEvent                                 = E_ZCB_EVENT_DEVICE_MATCH;
            psEvent->uData.sDeviceMatch.u16ShortAddress     = psZCBNode->u16ShortAddress;
            
            eUtils_LockUnlock(&psZCBNode->sLock);
            DBG_vPrintf(DBG_ZCB, "Queue new node event\n");
            
            if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != E_UTILS_OK)
            {
                DBG_vPrintf(DBG_ZCB, "Error queue'ing event\n");
                free(psEvent);
            }
        }
    }
    return;
}


static void ZCB_HandleAttributeReport(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    teZcbStatus eStatus = E_ZCB_ERROR;
    
    struct _tsAttributeReport
    {
        uint8_t     u8SequenceNo;
        uint16_t    u16ShortAddress;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint16_t    u16AttributeID;
        uint8_t     u8AttributeStatus;
        uint8_t     u8Type;
        union
        {
            uint8_t     u8Data;
            uint16_t    u16Data;
            uint32_t    u32Data;
            uint64_t    u64Data;
        } uData;
    } __attribute__((__packed__)) *psMessage = (struct _tsAttributeReport *)pvMessage;
    
    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u16ClusterID     = ntohs(psMessage->u16ClusterID);
    psMessage->u16AttributeID   = ntohs(psMessage->u16AttributeID);
    
    DBG_vPrintf(DBG_ZCB, "Attribute report from 0x%04X - Endpoint %d, cluster 0x%04X, attribute %d.\n", 
                psMessage->u16ShortAddress,
                psMessage->u8Endpoint,
                psMessage->u16ClusterID,
                psMessage->u16AttributeID
               );
    
    tsZcbEvent *psEvent = malloc(sizeof(tsZcbEvent));
    if (!psEvent)
    {
        daemon_log(LOG_CRIT, "Memory allocation failure allocating event");
        return;
    }

    psEvent->eEvent                                     = E_ZCB_EVENT_ATTRIBUTE_REPORT;
    psEvent->uData.sAttributeReport.u16ShortAddress     = psMessage->u16ShortAddress;
    psEvent->uData.sAttributeReport.u8Endpoint          = psMessage->u8Endpoint;
    psEvent->uData.sAttributeReport.u16ClusterID        = psMessage->u16ClusterID;
    psEvent->uData.sAttributeReport.u16AttributeID      = psMessage->u16AttributeID;
    psEvent->uData.sAttributeReport.eType               = psMessage->u8Type;
    
    switch(psMessage->u8Type)
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        case(E_ZCL_OSTRING):
        case(E_ZCL_CSTRING):
            psEvent->uData.sAttributeReport.uData.u8Data = psMessage->uData.u8Data;
            eStatus = E_ZCB_OK;
            break;
        
        case(E_ZCL_LOSTRING):
        case(E_ZCL_LCSTRING):
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
            psEvent->uData.sAttributeReport.uData.u16Data = ntohs(psMessage->uData.u16Data);
            eStatus = E_ZCB_OK;
            break;
 
        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
            psEvent->uData.sAttributeReport.uData.u32Data = ntohl(psMessage->uData.u32Data);
            eStatus = E_ZCB_OK;
            break;
 
        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
            psEvent->uData.sAttributeReport.uData.u64Data = be64toh(psMessage->uData.u64Data);
            eStatus = E_ZCB_OK;
            break;
            
        default:
            daemon_log(LOG_ERR, "Unknown attribute data type (%d) received from node 0x%04X", psMessage->u8Type, psMessage->u16ShortAddress);
            break;
    }

    if (eStatus == E_ZCB_OK)
    {
        if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != E_UTILS_OK)
        {
            DBG_vPrintf(DBG_ZCB, "Error queue'ing event\n");
            free(psEvent);
        }
    }
    else
    {
        free(psEvent);
    }
    return;
}
     

static teZcbStatus eZCB_ConfigureControlBridge(void)
{
#define CONFIGURATION_INTERVAL 500000
    /* Set up configuration */
    switch (eStartMode)
    {
        case(E_START_COORDINATOR):
            daemon_log(LOG_INFO, "Starting control bridge as HA coordinator");
            eZCB_SetDeviceType(E_MODE_COORDINATOR);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_SetChannelMask(eChannel);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_SetExtendedPANID(u64PanID);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_StartNetwork();
            usleep(CONFIGURATION_INTERVAL);
            break;
    
        case (E_START_ROUTER):
            daemon_log(LOG_INFO, "Starting control bridge as HA compatible router");
            eZCB_SetDeviceType(E_MODE_HA_COMPATABILITY);
            usleep(CONFIGURATION_INTERVAL);

            eZCB_SetChannelMask(eChannel);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_SetExtendedPANID(u64PanID);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_StartNetwork();
            usleep(CONFIGURATION_INTERVAL);
            break;
    
        case (E_START_TOUCHLINK):
            daemon_log(LOG_INFO, "Starting control bridge as ZLL router");
            eZCB_SetDeviceType(E_MODE_ROUTER);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_SetChannelMask(eChannel);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_SetExtendedPANID(u64PanID);
            usleep(CONFIGURATION_INTERVAL);
            
            eZCB_StartNetwork();
            usleep(CONFIGURATION_INTERVAL);
            break;
            
        default:
            daemon_log(LOG_ERR, "Unknown module mode\n");
            return E_ZCB_ERROR;
    }
    
    return E_ZCB_OK;
}


/* PDM Messages */
    
     

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
