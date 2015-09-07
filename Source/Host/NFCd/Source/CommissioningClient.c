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

#include "CommissioningClient.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define PORT "1880"

#define COMMISSIONING_VERSION 1


//#define COMMISSIONING_DEBUG

#ifdef COMMISSIONING_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* MAIN_DEBUG */

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
    uint64_t    u64PanId;
    uint16_t    u16PanId;
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
        uint8_t             au8Data[64];
    } uPayload;
} __attribute__((__packed__)) tsCommMsg;


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/


static teCommStatus eCommClientConnect(int *piClientSocket);
static teCommStatus eCommClientExchange(int iClientSocket, tsCommMsg *psOutgoing, tsCommMsg *psIncoming);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


teCommStatus eCommissionZigbee(tsCommZigbee *psZigbeeData)
{
    teCommStatus eStatus;
    int iCommissioningClient;
    tsCommMsg sOutgoing, sIncoming;
    
    if ((eStatus = eCommClientConnect(&iCommissioningClient)) != E_COMM_OK)
    {
        return eStatus;
    }
    
    if (eStatus == E_COMM_OK)
    {
        /* First, get the unsecured network parameters. */
        sOutgoing.sHeader.u8Version = COMMISSIONING_VERSION;
        sOutgoing.sHeader.u8Length  = 0;
        sOutgoing.sHeader.u8Command = E_COMM_GET_UNSECURED_PARAMS;
        
        if ((eStatus = eCommClientExchange(iCommissioningClient, &sOutgoing, &sIncoming)) == E_COMM_OK)
        {
            if (sIncoming.sHeader.u8Command != E_COMM_UNSECURED_PARAMS)
            {
                eStatus = E_COMM_ERROR;
            }
            else
            {
                psZigbeeData->u8Channel = sIncoming.uPayload.sUnsecuredParams.u8Channel;
                psZigbeeData->u16PanId  = ntohs(sIncoming.uPayload.sUnsecuredParams.u16PanId);
                psZigbeeData->u64PanId  = be64toh(sIncoming.uPayload.sUnsecuredParams.u64PanId);
                eStatus = E_COMM_OK;
            }
        }
    }
    
    if (eStatus == E_COMM_OK)
    {
        /* First, get the unsecured network parameters. */
        sOutgoing.sHeader.u8Version = COMMISSIONING_VERSION;
        sOutgoing.sHeader.u8Length  = sizeof(tsAddDeviceRequest);
        sOutgoing.sHeader.u8Command = E_COMM_ADD_DEVICE_REQUEST;
        
        sOutgoing.uPayload.sAddDeviceRequest.u16Profile = htons(psZigbeeData->u16Profile);
        memcpy(sOutgoing.uPayload.sAddDeviceRequest.au8MacAddress, psZigbeeData->au8MacAddress, 8);
        memcpy(sOutgoing.uPayload.sAddDeviceRequest.au8LinkKey, psZigbeeData->au8LinkKey, 16);

        if ((eStatus = eCommClientExchange(iCommissioningClient, &sOutgoing, &sIncoming)) == E_COMM_OK)
        {
            if (sIncoming.sHeader.u8Command != E_COMM_ADD_DEVICE_RESPONSE)
            {
                eStatus = E_COMM_ERROR;
            }
            else
            {
                if (sIncoming.uPayload.sAddDeviceResponse.u8Status != 0)
                {
                    eStatus = E_COMM_ERROR;
                }
                else
                {
                    memcpy(psZigbeeData->au8NetworkKey, sIncoming.uPayload.sAddDeviceResponse.au8NetworkKey, 16);
                    memcpy(psZigbeeData->au8MIC, sIncoming.uPayload.sAddDeviceResponse.au8MIC, 4);
                    psZigbeeData->u64TrustCenterAddress = be64toh(sIncoming.uPayload.sAddDeviceResponse.u64TrustCenterAddress);
                    memcpy(&psZigbeeData->u8KeySequenceNumber, &sIncoming.uPayload.sAddDeviceResponse.u8KeySequenceNumber, 1);
                }
                eStatus = E_COMM_OK;
            }
        }
    }

    close(iCommissioningClient);
    return eStatus;
}


teCommStatus eCommissionZigbeeInsecure(tsCommZigbee *psZigbeeData)
{
    teCommStatus eStatus;
    int iCommissioningClient;
    tsCommMsg sOutgoing, sIncoming;
    
    if ((eStatus = eCommClientConnect(&iCommissioningClient)) != E_COMM_OK)
    {
        return eStatus;
    }
    
    if (eStatus == E_COMM_OK)
    {
        /* First, get the unsecured network parameters. */
        sOutgoing.sHeader.u8Version = COMMISSIONING_VERSION;
        sOutgoing.sHeader.u8Length  = 0;
        sOutgoing.sHeader.u8Command = E_COMM_GET_UNSECURED_PARAMS;
        
        if ((eStatus = eCommClientExchange(iCommissioningClient, &sOutgoing, &sIncoming)) == E_COMM_OK)
        {
            if (sIncoming.sHeader.u8Command != E_COMM_UNSECURED_PARAMS)
            {
                eStatus = E_COMM_ERROR;
            }
            else
            {
                psZigbeeData->u8Channel = sIncoming.uPayload.sUnsecuredParams.u8Channel;
                psZigbeeData->u16PanId  = ntohs(sIncoming.uPayload.sUnsecuredParams.u16PanId);
                psZigbeeData->u64PanId  = be64toh(sIncoming.uPayload.sUnsecuredParams.u64PanId);
                eStatus = E_COMM_OK;
            }
        }
    }

    close(iCommissioningClient);
    return eStatus;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

static teCommStatus eCommClientConnect(int *piClientSocket)
{
    struct addrinfo         sHints;
    struct addrinfo         *psServerInfo;
    struct addrinfo         *p;
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
        if ((*piClientSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            daemon_log(LOG_DEBUG, "Commissioning client: %s failed(%s)", "socket", strerror(errno));
            continue;
        }

        if (connect(*piClientSocket, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(*piClientSocket);
            daemon_log(LOG_DEBUG, "Commissioning client: %s failed(%s)", "connect", strerror(errno));
            continue;
        }
        break;
    }

    freeaddrinfo(psServerInfo); // all done with this structure
    
    if (p == NULL)
    {
        return E_COMM_ERROR;
    }
    return E_COMM_OK;
}


static teCommStatus eCommClientExchange(int iClientSocket, tsCommMsg *psOutgoing, tsCommMsg *psIncoming)
{
    int iResult;
    iResult = write(iClientSocket, psOutgoing, sizeof(tsCommMsgHeader) + psOutgoing->sHeader.u8Length);
    if ((iResult <= 0) || (iResult != (sizeof(tsCommMsgHeader)  + psOutgoing->sHeader.u8Length)))
    {
        daemon_log(LOG_DEBUG, "Commissioning client: %s failed(%s)", "write outgoing packet", strerror(errno));
        return E_COMM_ERROR;
    }

    iResult = read(iClientSocket, psIncoming, sizeof(tsCommMsgHeader));
    if ((iResult <= 0) || (iResult != sizeof(tsCommMsgHeader)))
    {
        daemon_log(LOG_DEBUG, "Commissioning client: %s failed(%s)", "read incoming packet", strerror(errno));
        return E_COMM_ERROR;
    }
    
    if (psIncoming->sHeader.u8Version != COMMISSIONING_VERSION)
    {
        daemon_log(LOG_DEBUG, "Commissioning client: incoming packet has incorrect version");
        return E_COMM_ERROR;
    }
    
    if ((psIncoming->sHeader.u8Length > 0) && (psIncoming->sHeader.u8Length <= (sizeof(tsCommMsg) - sizeof(tsCommMsgHeader))))
    {
        iResult = read(iClientSocket, &psIncoming->uPayload, psIncoming->sHeader.u8Length);
        if ((iResult <= 0) || (iResult != psIncoming->sHeader.u8Length))
        {
            daemon_log(LOG_DEBUG, "Commissioning client: %s failed(%s)", "read incoming packet", strerror(errno));
            return E_COMM_ERROR;
        }
    }
    
#ifdef COMMISSIONING_DEBUG
    {
        int i;
        DEBUG_PRINTF("Incoming packet, version %d, length %d, command %d\n", 
               psIncoming->sHeader.u8Version, psIncoming->sHeader.u8Length, psIncoming->sHeader.u8Command);
        for (i = 0; i < psIncoming->sHeader.u8Length; i++)
        {
            DEBUG_PRINTF("0x%02x ", psIncoming->uPayload.au8Data[i] & 0xFF);
        }
        DEBUG_PRINTF("\n");
    }
#endif /* COMMISSIONING_DEBUG */
    
    return E_COMM_OK;
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

