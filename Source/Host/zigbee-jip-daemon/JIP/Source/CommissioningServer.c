/****************************************************************************
 *
 * MODULE:             Linux Zigbee - JIP Daemon
 *
 * COMPONENT:          Commissioning server
 *
 * REVISION:           $Revision: 37346 $
 *
 * DATED:              $Date: 2011-11-18 12:16:43 +0000 (Fri, 18 Nov 2011) $
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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <libdaemon/daemon.h>

#include <Utils.h>
#include <ZigbeeControlBridge.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_COMMISSIONING 0

#define PORT "1880"

#define BACKLOG 10

#define COMMISSIONING_VERSION 1

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef enum
{
    E_COMM_GET_UNSECURED_PARAMS     = 0,
    E_COMM_UNSECURED_PARAMS         = 1,
    
    E_COMM_ADD_DEVICE_REQUEST       = 2,
    E_COMM_ADD_DEVICE_RESPONSE      = 3,
} teCommCommand;

typedef struct
{
    uint8_t     u8Version;
    uint8_t     u8Length;
    uint8_t     u8Command;    
} __attribute__((__packed__)) tsCommMsgHeader;


typedef struct
{
    uint64_t    u64PanID;
    uint16_t    u16PanID;
    uint8_t     u8Channel;
} __attribute__((__packed__)) tsUnsecuredParams;


typedef struct
{
    uint16_t    u16Profile;
    uint8_t     au8MacAddress[8];
    uint8_t     au8LinkKey[16];
} __attribute__((__packed__)) tsAddDeviceRequest;


typedef struct
{
    uint8_t     u8Status;
    uint8_t     au8NetworkKey[16];
    uint8_t     au8MIC[4];
    uint64_t    u64TrustCenterAddress;
    uint8_t     u8KeySequenceNumber;
} __attribute__((__packed__)) tsAddDeviceResponse;


typedef struct
{
    tsCommMsgHeader         sHeader;
    
    union
    {
        tsUnsecuredParams   sUnsecuredParams;
        tsAddDeviceRequest  sAddDeviceRequest;
        tsAddDeviceResponse sAddDeviceResponse;
    } uPayload;
} __attribute__((__packed__)) tsCommMsg;


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

static int iServerSetup(int *piServerSocket);
static void *psGetSockAddr(struct sockaddr *psSockAddr);
static void vHandleClient(int iClientSocket);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


void *pvCommissioningServer(tsUtilsThread *psThreadInfo)
{
    int                     iServerSocket = 0;
    
    DBG_vPrintf(DBG_COMMISSIONING, "Commissioning server starting\n");
    
    psThreadInfo->eState = E_THREAD_RUNNING;
    
    if (iServerSetup(&iServerSocket) < 0)
    {
        daemon_log(LOG_ERR, "Failed to set up commissioning server on port %s", PORT);
        return NULL;
    }
    
    daemon_log(LOG_INFO, "Commissioning server listening on port %s", PORT);
    
    while (psThreadInfo->eState == E_THREAD_RUNNING)
    {
        socklen_t               iSinSize = sizeof(struct sockaddr_storage);
        struct sockaddr_storage sClientAddress;
        char                    sClientAddressStr[INET6_ADDRSTRLEN];
        int                     iClientSocket;

        iClientSocket = accept(iServerSocket, (struct sockaddr *)&sClientAddress, &iSinSize);
        if (iClientSocket == -1)
        {
            daemon_log(LOG_DEBUG, "Commissioning server: %s failed(%s)", "accept", strerror(errno));
            continue;
        }

        inet_ntop(sClientAddress.ss_family, psGetSockAddr((struct sockaddr *)&sClientAddress), sClientAddressStr, INET6_ADDRSTRLEN);
        daemon_log(LOG_DEBUG, "Commissioning server: new connection from %s", sClientAddressStr);
        
        vHandleClient(iClientSocket);
    }
    
    DBG_vPrintf(DBG_COMMISSIONING, "Commissioning server exiting\n");
    
    close(iServerSocket);
    
    return NULL;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

static int iServerSetup(int *piServerSocket)
{
    struct addrinfo         sHints;
    struct addrinfo         *psServerInfo;
    struct addrinfo         *p;
    int                     iYes = 1;
    int                     iResult;
    
    memset(&sHints, 0, sizeof(struct addrinfo));
    sHints.ai_family = AF_UNSPEC;
    sHints.ai_socktype = SOCK_STREAM;
    sHints.ai_flags = AI_PASSIVE;

    if ((iResult = getaddrinfo("localhost", PORT, &sHints, &psServerInfo)) != 0)
    {
        daemon_log(LOG_ERR, "getaddrinfo: %s", gai_strerror(iResult));
        return 1;
    }

    for(p = psServerInfo; p != NULL; p = p->ai_next)
    {
        if ((*piServerSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            daemon_log(LOG_DEBUG, "Commissioning server: %s failed(%s)", "socket", strerror(errno));
            continue;
        }

        if (setsockopt(*piServerSocket, SOL_SOCKET, SO_REUSEADDR, &iYes, sizeof(int)) == -1) 
        {
            daemon_log(LOG_DEBUG, "Commissioning server: %s failed(%s)", "setsockopt", strerror(errno));
        }

        if (bind(*piServerSocket, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(*piServerSocket);
            daemon_log(LOG_DEBUG, "Commissioning server: %s failed(%s)", "bind", strerror(errno));
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        return -1;
    }

    freeaddrinfo(psServerInfo); // all done with this structure

    if (listen(*piServerSocket, BACKLOG) == -1)
    {
        daemon_log(LOG_DEBUG, "Commissioning server: %s failed(%s)", "listen", strerror(errno));
        return -2;
    }
    
    return 0;
}


static void *psGetSockAddr(struct sockaddr *psSockAddr)
{
    if (psSockAddr->sa_family == AF_INET) 
    {
        return &(((struct sockaddr_in*)psSockAddr)->sin_addr);
    }

    return &(((struct sockaddr_in6*)psSockAddr)->sin6_addr);
}


static void vHandleClient(int iClientSocket)
{
    tsCommMsg sIncoming;
    int iClientConnected = 1;
    
    while(iClientConnected)
    {
        int iResult;
        iResult = read(iClientSocket, &sIncoming, sizeof(tsCommMsgHeader));
        if ((iResult <= 0) || (iResult != sizeof(tsCommMsgHeader)))
        {
            break;
        }
        DBG_vPrintf(DBG_COMMISSIONING, "Incoming packet, version %d, length %d, command %d\n", sIncoming.sHeader.u8Version, sIncoming.sHeader.u8Length, sIncoming.sHeader.u8Command);
        
        if (sIncoming.sHeader.u8Version != COMMISSIONING_VERSION)
        {
            DBG_vPrintf(DBG_COMMISSIONING, "Incorrect commissioning version in received packet\n");
            continue;
        }
        
        if ((sIncoming.sHeader.u8Length > 0) && (sIncoming.sHeader.u8Length <= (sizeof(tsCommMsg) - sizeof(tsCommMsgHeader))))
        {
            iResult = read(iClientSocket, &sIncoming.uPayload, sIncoming.sHeader.u8Length);
            if ((iResult <= 0) || (iResult != sIncoming.sHeader.u8Length))
            {
                break;
            }
        }
        
        switch (sIncoming.sHeader.u8Command)
        {
            case (E_COMM_GET_UNSECURED_PARAMS):
            {
                tsCommMsg sOutgoing;
                sOutgoing.sHeader.u8Version = COMMISSIONING_VERSION;
                sOutgoing.sHeader.u8Length  = sizeof(tsUnsecuredParams);
                sOutgoing.sHeader.u8Command = E_COMM_UNSECURED_PARAMS;
                
                sOutgoing.uPayload.sUnsecuredParams.u64PanID    = htobe64(u64PanIDInUse);
                sOutgoing.uPayload.sUnsecuredParams.u16PanID    = htons(u16PanIDInUse);
                sOutgoing.uPayload.sUnsecuredParams.u8Channel   = eChannelInUse;
                
                iResult = write(iClientSocket, &sOutgoing, sizeof(tsCommMsgHeader) + sizeof(tsUnsecuredParams));
                if ((iResult <= 0) || (iResult != (sizeof(tsCommMsgHeader) + sizeof(tsUnsecuredParams))))
                {
                    iClientConnected = 0;
                }
                break;
            }
            
            case (E_COMM_ADD_DEVICE_REQUEST):
            {
                tsCommMsg sOutgoing;
                uint64_t u64IEEEAddress;
                sOutgoing.sHeader.u8Version = COMMISSIONING_VERSION;
                sOutgoing.sHeader.u8Length  = sizeof(tsAddDeviceResponse);
                sOutgoing.sHeader.u8Command = E_COMM_ADD_DEVICE_RESPONSE;
                
                {
                    uint8_t *pu8MacAddress = sIncoming.uPayload.sAddDeviceRequest.au8MacAddress;
                    daemon_log(LOG_DEBUG, "Commission device profile 0x%04X Mac address 0x%02X%02X%02X%02X%02X%02X%02X%02X", ntohs(sIncoming.uPayload.sAddDeviceRequest.u16Profile),
                           pu8MacAddress[0], pu8MacAddress[1], pu8MacAddress[2], pu8MacAddress[3], 
                           pu8MacAddress[4], pu8MacAddress[5], pu8MacAddress[6], pu8MacAddress[7]);

                    u64IEEEAddress = ((uint64_t)pu8MacAddress[0] << 56) | ((uint64_t)pu8MacAddress[1] << 48) | ((uint64_t)pu8MacAddress[2] << 40) | ((uint64_t)pu8MacAddress[3] << 32) | 
                                     ((uint64_t)pu8MacAddress[4] << 24) | ((uint64_t)pu8MacAddress[5] << 16) | ((uint64_t)pu8MacAddress[6] << 8) | ((uint64_t)pu8MacAddress[7] << 0);
                }
                
                // Commission device using control bridge.
                if (eZCB_AuthenticateDevice(u64IEEEAddress, sIncoming.uPayload.sAddDeviceRequest.au8LinkKey, 
                                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey, sOutgoing.uPayload.sAddDeviceResponse.au8MIC,
                                            &sOutgoing.uPayload.sAddDeviceResponse.u64TrustCenterAddress, 
                                            &sOutgoing.uPayload.sAddDeviceResponse.u8KeySequenceNumber
                                           ) == E_ZCB_OK)
                {                    
                    daemon_log(LOG_DEBUG, "Commission device succeeded, Trust center: 0x%016llX, Network Key: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                            (unsigned long long int)sOutgoing.uPayload.sAddDeviceResponse.u64TrustCenterAddress,
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[0],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[1],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[2],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[3],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[4],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[5],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[6],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[7],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[8],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[9],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[10],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[11],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[12],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[13],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[14],
                            sOutgoing.uPayload.sAddDeviceResponse.au8NetworkKey[15]
                    );      
                    sOutgoing.uPayload.sAddDeviceResponse.u64TrustCenterAddress = htobe64(sOutgoing.uPayload.sAddDeviceResponse.u64TrustCenterAddress);
                    sOutgoing.uPayload.sAddDeviceResponse.u8Status = 0;
                }
                else
                {
                    daemon_log(LOG_ERR, "Failed to commission device");
                    sOutgoing.uPayload.sAddDeviceResponse.u8Status = 255;
                }
                
                iResult = write(iClientSocket, &sOutgoing, sizeof(tsCommMsgHeader) + sizeof(tsAddDeviceResponse));
                if ((iResult <= 0) || (iResult != (sizeof(tsCommMsgHeader) + sizeof(tsAddDeviceResponse))))
                {
                    iClientConnected = 0;
                }
                break;                
            }
            
            default:
                DBG_vPrintf(DBG_COMMISSIONING, "Unknown command %d\n", sIncoming.sHeader.u8Command);
        }
    }
    
    DBG_vPrintf(DBG_COMMISSIONING, "Connection closed\n");
    close(iClientSocket);
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

