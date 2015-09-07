/****************************************************************************
 *
 * MODULE:             Zigbee - JIP Daemon
 *
 * COMPONENT:          Fake Border router
 *
 * REVISION:           $Revision: 43420 $
 *
 * DATED:              $Date: 2012-06-18 15:13:17 +0100 (Mon, 18 Jun 2012) $
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include <libdaemon/daemon.h>

#include "JIP.h"

#include "JIP_BorderRouter.h"
#include "Zeroconf.h"
#include "ZigbeeControlBridge.h"
#include "ZigbeeConstant.h"
#include "TunDevice.h"
#include "Utils.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_BORDERROUTER 0

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

static tsJIPAddress sBR_IEEEAddressToIPv6(uint64_t u64IEEEAddress, uint16_t u16Port);
static uint64_t u64BR_IPv6AddressToIEEE(tsJIPAddress sNode_Address);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/** JIP context structure */
tsJIP_Context sJIP_Context;


extern const char *Version;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/


static struct in6_addr sNetworkPrefix;

static tsNode *psBorderRouterNode = NULL;


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

teBrStatus eBR_Init(const char *pcIPv6Address)
{
    teJIP_Status eStatus;
    tsNode *psNode;
    tsMib *psMib;
    tsVar *psVar;
    
    DBG_vPrintf(DBG_BORDERROUTER, "Starting JIP\n");

    if (eJIP_Init(&sJIP_Context, E_JIP_CONTEXT_SERVER) != E_JIP_OK)
    {
        daemon_log(LOG_ERR, "Error initialising server");
        return E_BR_JIP_ERROR;
    }
    
    if ((eStatus = eJIPserver_Listen(&sJIP_Context, JIP_DEFAULT_PORT)) != E_JIP_OK)
    {
        daemon_log(LOG_ERR, "Error starting JIP server (0x%02x)", eStatus);
        return E_BR_JIP_ERROR;
    }
    
    DBG_vPrintf(DBG_BORDERROUTER, "Loading JIP Definitions\n");
    
    /* Load definitions file */
    if (eJIPService_PersistXMLLoadDefinitions(&sJIP_Context, "/usr/share/zigbee-jip-daemon/jip_cache_definitions.xml") != E_JIP_OK)
    {
        /* Load definitions file */
        if (eJIPService_PersistXMLLoadDefinitions(&sJIP_Context, "jip_cache_definitions.xml") != E_JIP_OK)
        {
            daemon_log(LOG_ERR, "Error loading node definitions");
            return E_BR_JIP_ERROR;
        }
    }
    
    DBG_vPrintf(DBG_BORDERROUTER, "Adding IPv6 address %s to interface\n", pcIPv6Address);
    
    eTD_AddIPAddress(pcIPv6Address);
    
    if (eJIPserver_NodeAdd(&sJIP_Context, pcIPv6Address, 0x08010001, "Virtual Border Router", Version, &psNode) != E_JIP_OK)
    {
        daemon_log(LOG_ERR, "Error creating virtual border router node");
        return -1;
    }
    
#ifdef USE_ZEROCONF
    /* Export the virtual border router via Zeroconf */
    {
        char acHostname[255];
        sprintf(acHostname, "BR_%s", pcTD_DevName);
        ZC_RegisterService("JIP Border Router", acHostname, pcIPv6Address);
    }
#endif /* USE_ZEROCONF */
    
    /* Set address prefix from the node IPv6 address */
    memset(&sNetworkPrefix, 0, sizeof(struct in6_addr));
    memcpy(&sNetworkPrefix, &psNode->sNode_Address.sin6_addr, sizeof(uint64_t));
    
    /* Initialise empty Network table */
    psMib = psJIP_LookupMibId(psNode, NULL, E_JIP_MIBID_JENNET);
    if(psMib)
    {
        psVar = psJIP_LookupVarIndex(psMib, 0);
        
        if (psVar)
        {
            uint32_t u32DeviceType = 0;
            eJIP_SetVarValue(psVar, &u32DeviceType, sizeof(uint32_t));
            
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 4);
        
        if (psVar)
        {
            tsTable *psTable = malloc(sizeof(tsTable));
            if (psTable)
            {
                psTable->u32NumRows = 0;
                psTable->psRows     = NULL;
                
                psVar->pvData = psTable;
                DBG_vPrintf(DBG_BORDERROUTER, "Empty network table initialised\n");
            }

            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
    }
    
    psBorderRouterNode = psNode;
    
    eJIP_UnlockNode(psNode);
    
    return E_BR_OK;
}


teBrStatus eBR_Destory(void)
{
    ZC_Destroy();
    psBorderRouterNode = NULL;
    eJIP_Destroy(&sJIP_Context);
    return E_BR_OK;
}


tsZCB_Node *psBR_FindZigbeeNode(tsNode *psJIPNode)
{
    uint64_t u64IEEEAddress;

    DBG_vPrintf(DBG_BORDERROUTER, "Find Zigbee node matching IPv6 Address ");
    DBG_vPrintf_IPv6Address(DBG_BORDERROUTER, psJIPNode->sNode_Address.sin6_addr);

    u64IEEEAddress = u64BR_IPv6AddressToIEEE(psJIPNode->sNode_Address);

    return psZCB_FindNodeIEEEAddress(u64IEEEAddress);
}


tsNode *psBR_FindJIPNode(tsZCB_Node *psZCBNode)
{
    tsJIPAddress sNode_Address;
    tsNode *psJIPNode = NULL;

    /* Unlock the node before calling into JIP to lock the JIP node */
    eUtils_LockUnlock(&psZCBNode->sLock);

    sNode_Address = sBR_IEEEAddressToIPv6(psZCBNode->u64IEEEAddress, JIP_DEFAULT_PORT);
    
    DBG_vPrintf(DBG_BORDERROUTER, "Find JIP node matching IEEE Address 0x%016llX, ", (unsigned long long int)psZCBNode->u64IEEEAddress);
    DBG_vPrintf_IPv6Address(DBG_BORDERROUTER, sNode_Address.sin6_addr);

    psJIPNode = psJIP_LookupNode(&sJIP_Context, &sNode_Address);
    
    /* Relock the Zigbee node now we hold the JIP lock for this JIP node - no more requests to it can come in while we hold the JIP lock.
     * This is to avoid deadlocks where the JIP lock is help by JIP while executing one of our callbacks. The callbacks then attempt to
     * lock the psZCBNode lock, which results in a deadlock.
     * When we call into JIP to lock a node, using psJIP_LookupNode, we need to ensure we are not holding any mutexes that could cause deadlock.
     */
    eUtils_LockLock(&psZCBNode->sLock);
    
    if (psJIPNode)
    {
        DBG_vPrintf(DBG_BORDERROUTER, "Found JIP node at %p\n", psJIPNode);
    }
    else
    {
        DBG_vPrintf(DBG_BORDERROUTER, "JIP Node not found\n");
    }
    return psJIPNode;
}


teBrStatus eBR_NodeJoined(tsZCB_Node *psZCBNode)
{
    tsNode *psJIPNode;
    tsMib *psMib;
    tsVar *psVar;

    daemon_log(LOG_INFO, "Zigbee node 0x%04X (0x%016llX) device ID 0x%04X joined network", 
               psZCBNode->u16ShortAddress, 
               (unsigned long long int)psZCBNode->u64IEEEAddress,
               psZCBNode->u16DeviceID);
    
    eJIP_LockNode(psBorderRouterNode, True);
    
    /* Add device to network table */
    psMib = psJIP_LookupMibId(psBorderRouterNode, NULL, 0xffffff01);
    if(psMib)
    {
        psVar = psJIP_LookupVarIndex(psMib, 4);
        
        if (psVar)
        {
            struct _sNetworkTableRow
            {
                uint64_t u64IfaceID;
                uint32_t u32DeviceId;
            } __attribute__((__packed__)) sNetworkTableRow;
            
            tsJIPAddress sNode_Address;
            char acIPv6Address[INET6_ADDRSTRLEN] = "Could not determine address\n";
            tsDeviceIDMap *psDeviceIDMap;
 
            sNetworkTableRow.u64IfaceID = htobe64(psZCBNode->u64IEEEAddress);
            
            sNode_Address = sBR_IEEEAddressToIPv6(psZCBNode->u64IEEEAddress, JIP_DEFAULT_PORT);            
            inet_ntop(AF_INET6, &sNode_Address.sin6_addr, acIPv6Address, INET6_ADDRSTRLEN);

            psDeviceIDMap = asDeviceIDMap;
            while (((psDeviceIDMap->u16ZigbeeDeviceID != 0) &&
                    (psDeviceIDMap->u32JIPDeviceID != 0) &&
                    (psDeviceIDMap->prInitaliseRoutine != NULL)))
            {
                if (psDeviceIDMap->u16ZigbeeDeviceID == psZCBNode->u16DeviceID)
                {
                    DBG_vPrintf(DBG_BORDERROUTER, "Found JIP device type 0x%08X for ZB Device type 0x%04X\n",
                                psDeviceIDMap->u32JIPDeviceID,
                                psDeviceIDMap->u16ZigbeeDeviceID);

                    sNetworkTableRow.u32DeviceId = htonl(psDeviceIDMap->u32JIPDeviceID);
                    
                    eTD_AddIPAddress(acIPv6Address);
            
                    if (eJIPserver_NodeAdd(&sJIP_Context, acIPv6Address, psDeviceIDMap->u32JIPDeviceID, NULL, Version, &psJIPNode) != E_JIP_OK)
                    {
                        daemon_log(LOG_ERR, "Error creating JIP device");
                        eJIP_UnlockNode(psBorderRouterNode);
                        return E_BR_ERROR;
                    }
                    
                    /* Call the device type's initialisation routine */
                    if (psDeviceIDMap->prInitaliseRoutine(psZCBNode, psJIPNode) != E_JIP_OK)
                    {
                        daemon_log(LOG_ERR, "Error initialising node");
                    }
                    
                    eJIP_UnlockNode(psJIPNode);

                    if (eJIP_Table_UpdateRow(psVar, psZCBNode->u16ShortAddress, &sNetworkTableRow, sizeof(struct _sNetworkTableRow)) != E_JIP_OK)
                    {
                        daemon_log(LOG_ERR, "Error adding device to network table");
                    }
                    
                    daemon_log(LOG_INFO, "Create JIP device ID 0x%08X at address %s.", 
                                        psDeviceIDMap->u32JIPDeviceID, acIPv6Address);
                    
                    /* Done with the map */
                    break;
                }
                
                /* Next device map */
                psDeviceIDMap++;
            }
        }
    }
    
    eJIP_UnlockNode(psBorderRouterNode);

    return E_BR_OK;
}


teBrStatus eBR_NodeLeft(tsZCB_Node *psZCBNode)
{
    tsNode *psJIPNode;
    tsMib *psMib;
    tsVar *psVar;
    
    daemon_log(LOG_INFO, "Zigbee node 0x%04X (0x%016llX) device ID 0x%04X left network", 
               psZCBNode->u16ShortAddress, 
               (unsigned long long int)psZCBNode->u64IEEEAddress,
               psZCBNode->u16DeviceID);
    
    psJIPNode = psBR_FindJIPNode(psZCBNode);
    if (!psJIPNode)
    {
        DBG_vPrintf(DBG_BORDERROUTER, "Could not find JIP node\n");        
        return E_BR_ERROR;
    }
    
    eJIP_LockNode(psBorderRouterNode, True);
    
    /* Add device to network table */
    psMib = psJIP_LookupMibId(psBorderRouterNode, NULL, 0xffffff01);
    if(psMib)
    {
        psVar = psJIP_LookupVarIndex(psMib, 4);
        
        if (psVar)
        {
            tsJIPAddress sNode_Address;
            char acIPv6Address[INET6_ADDRSTRLEN] = "Could not determine address\n";
            tsDeviceIDMap *psDeviceIDMap;

            sNode_Address = sBR_IEEEAddressToIPv6(psZCBNode->u64IEEEAddress, JIP_DEFAULT_PORT);            
            inet_ntop(AF_INET6, &sNode_Address.sin6_addr, acIPv6Address, INET6_ADDRSTRLEN);

            psDeviceIDMap = asDeviceIDMap;
            while (((psDeviceIDMap->u16ZigbeeDeviceID != 0) &&
                    (psDeviceIDMap->u32JIPDeviceID != 0) &&
                    (psDeviceIDMap->prInitaliseRoutine != NULL)))
            {
                if (psDeviceIDMap->u16ZigbeeDeviceID == psZCBNode->u16DeviceID)
                {
                    DBG_vPrintf(DBG_BORDERROUTER, "Found JIP device type 0x%08X for ZB Device type 0x%04X\n",
                                psDeviceIDMap->u32JIPDeviceID,
                                psDeviceIDMap->u16ZigbeeDeviceID);
                    
                    eTD_RemoveIPAddress(acIPv6Address);
                    
                    /* Call the device type's removal routine */
//                     if (psDeviceIDMap->prInitaliseRoutine(psZCBNode, psJIPNode) != E_JIP_OK)
//                     {
//                         daemon_log(LOG_ERR, "Error initialising node");
//                     }
                    
                    if (eJIPserver_NodeRemove(&sJIP_Context, psJIPNode) != E_JIP_OK)
                    {
                        daemon_log(LOG_ERR, "Error removing node");
                    }

                    if (eJIP_Table_UpdateRow(psVar, psZCBNode->u16ShortAddress, NULL, 0) != E_JIP_OK)
                    {
                        daemon_log(LOG_ERR, "Error removing device from network table");
                    }
                    /* Done with the map */
                    break;
                }
                
                /* Next device map */
                psDeviceIDMap++;
            }
        }
    }
    
    eJIP_UnlockNode(psBorderRouterNode);
    return E_BR_OK;
}


teBrStatus eBR_CheckDeviceComms(void)
{
    tsNode          *psJIPNode;
    tsZCB_Node      *psZcbNode;
    tsJIPAddress   *NodeAddressList = NULL;
    uint32_t        u32NumNodes = 0;

    static uint32_t u32NodeIndex = 1;

    if (eJIP_GetNodeAddressList(&sJIP_Context, E_JIP_DEVICEID_ALL, &NodeAddressList, &u32NumNodes) != E_JIP_OK)
    {
        daemon_log(LOG_ERR, "Error reading JIP node list\n");
        return E_BR_JIP_ERROR;
    }
    
    if (u32NumNodes <= 1)
    {
        DBG_vPrintf(DBG_BORDERROUTER, "No nodes in network\n");
        free(NodeAddressList);
        return E_BR_OK;
    }

    if (u32NodeIndex >= u32NumNodes)
    {
        // Go back to the first non border router node. 
        u32NodeIndex = 1;
    }
    
    DBG_vPrintf(DBG_BORDERROUTER, "Comms test node index %d / %d nodes in network\n", u32NodeIndex+1, u32NumNodes);
    
    psJIPNode = psJIP_LookupNode(&sJIP_Context, &NodeAddressList[u32NodeIndex]);
    if (!psJIPNode)
    {
        DBG_vPrintf(DBG_BORDERROUTER, "Could not find node ");
        DBG_vPrintf_IPv6Address(DBG_BORDERROUTER, NodeAddressList[u32NodeIndex].sin6_addr);
        free(NodeAddressList);
        u32NodeIndex++;
        return E_BR_OK;
    }

    psZcbNode = psBR_FindZigbeeNode(psJIPNode);
    eJIP_UnlockNode(psJIPNode);
    
    if (!psZcbNode)
    {
        DBG_vPrintf(DBG_BORDERROUTER, "Could not find Zigbee node for JIP IPv6 address");
        DBG_vPrintf_IPv6Address(DBG_BORDERROUTER, NodeAddressList[u32NodeIndex].sin6_addr);
        free(NodeAddressList);
        u32NodeIndex++;
        return E_BR_OK;
    }

    if (psZcbNode->u8MacCapability & (E_ZB_MAC_CAPABILITY_RXON_WHEN_IDLE))
    {
        tsZCB_Node *psZcbNodeControlBridge;
        teZcbStatus eStatus;
        
        // Request neighbour table of device.
        // This is used to both monitor that a device is still available,
        // and to detect new devices that are present in the network.
        eStatus = eZCB_NeighbourTableRequest(psZcbNode);
        if (eStatus == E_ZCB_OK)
        {
            
        }

        /* Just temporarily grab a pointer to the control bridge. It won't be removed from under us so we can unlock it safely. */
        psZcbNodeControlBridge = psZCB_FindNodeControlBridge();
        eUtils_LockUnlock(&psZcbNodeControlBridge->sLock);
        
        if (psZcbNode == psZcbNodeControlBridge)
        {
            DBG_vPrintf(DBG_BORDERROUTER, "Node is the control bridge\n");
            eUtils_LockUnlock(&psZcbNode->sLock);
        }
        else
        {
            if (iBR_DeviceTimedOut(psZcbNode))
            {
                DBG_vPrintf(DBG_BORDERROUTER, "Node 0x%04X has been removed - deleting.\n", psZcbNode->u16ShortAddress);
                
                if (eBR_NodeLeft(psZcbNode) != E_BR_OK)
                {
                    DBG_vPrintf(DBG_BORDERROUTER, "Error removing node from JIP\n");
                }
                else
                {
                    if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                    {
                        DBG_vPrintf(DBG_BORDERROUTER, "Error removing node from ZCB\n");
                    }
                }
            }
            else
            {
                eUtils_LockUnlock(&psZcbNode->sLock);
            }
        }
    }
    else
    {
        DBG_vPrintf(DBG_BORDERROUTER, "Not polling sleeping device\n");
        eUtils_LockUnlock(&psZcbNode->sLock);
    }
    
    // Next node next time
    u32NodeIndex++;

    free(NodeAddressList);
    return E_BR_OK;
}


int iBR_DeviceTimedOut(tsZCB_Node *psZcbNode)
{
    struct timeval sNow;

    gettimeofday(&sNow, NULL);
    
    DBG_vPrintf(DBG_BORDERROUTER, "Last successful comms with node 0x%04X was %d seconds ago, sequential failures: %d\n", 
                psZcbNode->u16ShortAddress, sNow.tv_sec - psZcbNode->sComms.sLastSuccessful.tv_sec, psZcbNode->sComms.u16SequentialFailures);
    
    if (((sNow.tv_sec - psZcbNode->sComms.sLastSuccessful.tv_sec) > 30) && (psZcbNode->sComms.u16SequentialFailures > 5))
    {
        return 1;
    }
    return 0;
}


uint16_t u16BR_IPv6MulticastToBroadcast(tsJIPAddress *psMulticastAddress)
{
    uint16_t u16BroadcastAddress = ntohs(*(uint16_t*)&psMulticastAddress->sin6_addr.s6_addr[14]);
    DBG_vPrintf(DBG_BORDERROUTER, "Multicast Set to: ");
    DBG_vPrintf_IPv6Address(DBG_BORDERROUTER, psMulticastAddress->sin6_addr);
    DBG_vPrintf(DBG_BORDERROUTER, "Broadcast to 0x%04X\n", u16BroadcastAddress);
    return u16BroadcastAddress;
}


teJIP_Status eJIP_Status_from_ZCB(teZcbStatus eZCB_Status)
{
    switch (eZCB_Status)
    {
        case (E_ZCB_OK):
            return E_JIP_OK;
            
        case (E_ZCB_TIMEOUT):
        case (E_ZCB_COMMS_FAILED):
            return E_JIP_ERROR_TIMEOUT;
            
        case (E_ZCB_READ_ONLY):
        case (E_ZCB_WRITE_ONLY):
        case (E_ZCB_DEFINED_OUT_OF_BAND):
        case (E_ZCB_ACTION_DENIED):
        case (E_ZCB_NOT_AUTHORISED):
            return E_JIP_ERROR_NO_ACCESS;

        case (E_ZCB_INVALID_DATA_TYPE):
            return E_JIP_ERROR_WRONG_TYPE;
            
        case (E_ZCB_INVALID_VALUE):
            return E_JIP_ERROR_BAD_VALUE;
            
        case (E_ZCB_ERROR):
        default:
            return E_JIP_ERROR_FAILED;
    }
    return E_JIP_ERROR_FAILED;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

static tsJIPAddress sBR_IEEEAddressToIPv6(uint64_t u64IEEEAddress, uint16_t u16Port)
{
    tsJIPAddress sNode_Address;
    
    u64IEEEAddress = htobe64(u64IEEEAddress);

    memset(&sNode_Address, 0, sizeof(tsJIPAddress));
    sNode_Address.sin6_family  = AF_INET6;
    sNode_Address.sin6_port    = htons(u16Port);
    memcpy(&sNode_Address.sin6_addr.s6_addr[0], &sNetworkPrefix, sizeof(uint64_t));
    memcpy(&sNode_Address.sin6_addr.s6_addr[8], &u64IEEEAddress, sizeof(uint64_t));
    
    return sNode_Address;
}


static uint64_t u64BR_IPv6AddressToIEEE(tsJIPAddress sNode_Address)
{
    uint64_t u64IEEEAddress;
    
    memcpy(&u64IEEEAddress, &sNode_Address.sin6_addr.s6_addr[8], sizeof(uint64_t));

    return be64toh(u64IEEEAddress);
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

