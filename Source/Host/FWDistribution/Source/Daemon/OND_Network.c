/****************************************************************************
 *
 * MODULE:             Firmware Distribution Daemon
 *
 * COMPONENT:          Network support
 *
 * REVISION:           $Revision: 43428 $
 *
 * DATED:              $Date: 2012-06-18 15:28:13 +0100 (Mon, 18 Jun 2012) $
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libdaemon/daemon.h>

#include "Trace.h"
#include "OND.h"


#define DBG_FUNCTION_CALLS 0
#define DBG_OND_NETWORK 0


extern int verbosity;


teONDStatus eONDNetworkInitialise(tsONDNetwork *psONDNetwork, const char *pcBindAddress, const char *pcPortNumber)
{
    struct addrinfo hints, *res;
    int s;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s (%p, %s, %s)\n", __FUNCTION__, psONDNetwork, pcBindAddress, pcPortNumber);
    
    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
           
    s = getaddrinfo(NULL, pcPortNumber, &hints, &res);
    if (s != 0)
    {
        daemon_log(LOG_CRIT, "ONDNetwork: Could not get address for bind (%s)", gai_strerror(s));
        return OND_STATUS_ERROR;
    }
    
    psONDNetwork->u16PortNumber = ntohs(((struct sockaddr_in6 *)(res->ai_addr))->sin6_port);

    psONDNetwork->iSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (psONDNetwork->iSocket < 0)
    {
        daemon_log(LOG_CRIT, "ONDNetwork: Could not create socket (%s)", strerror(errno));
        freeaddrinfo(res);
        return OND_STATUS_ERROR;
    }
    
    if (bind(psONDNetwork->iSocket, res->ai_addr, sizeof(struct sockaddr_in6)) < 0)
    {
        daemon_log(LOG_CRIT, "ONDNetwork: Could not create bind socket (%s)", strerror(errno));
        freeaddrinfo(res);
        return OND_STATUS_ERROR;
    }

    freeaddrinfo(res);
    return OND_STATUS_OK;
}


teONDStatus eONDNetworkSendPacket(tsONDNetwork *psONDNetwork, struct in6_addr sAddress, tsONDPacket *psONDPacket)
{
#define BUFFER_SIZE 1024
    uint8_t au8Buffer[BUFFER_SIZE];
    uint32_t u32PacketSize;
    int32_t iBytesSent;
    struct sockaddr_in6 sAddr;
    
    memset(&sAddr, 0, sizeof(struct sockaddr_in6));
    sAddr.sin6_family   = AF_INET6;
    sAddr.sin6_port     = htons(psONDNetwork->u16PortNumber);
    sAddr.sin6_flowinfo = 0;
    sAddr.sin6_scope_id = 0; // Scope ID
    
    memcpy(&sAddr.sin6_addr, &sAddress, sizeof(struct in6_addr));
    
    {
        uint16_t u16Temp;
        uint32_t u32Temp;
        
        /* Set up the buffer */
        au8Buffer[0] = 0;
        au8Buffer[1] = psONDPacket->eType;
        au8Buffer[2] = 0;
        au8Buffer[3] = 0;
        u32PacketSize = 4;
        
        switch (psONDPacket->eType)
        {
            case (OND_PACKET_INITIATE):
                u32Temp = htonl(psONDPacket->uPayload.sONDPacketInitiate.u32DeviceID);
                memcpy(&au8Buffer[u32PacketSize], &u32Temp, sizeof(uint32_t));
                u32PacketSize += sizeof(uint32_t);
                
                u16Temp = htons(psONDPacket->uPayload.sONDPacketInitiate.u16ChipType);
                memcpy(&au8Buffer[u32PacketSize], &u16Temp, sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
                
                u16Temp = htons(psONDPacket->uPayload.sONDPacketInitiate.u16Revision);
                memcpy(&au8Buffer[u32PacketSize], &u16Temp, sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
                
                if (verbosity >= 10)
                {
                    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                    inet_ntop(AF_INET6, &sAddr.sin6_addr, buffer, INET6_ADDRSTRLEN);
                    daemon_log(LOG_INFO, "ONDNetwork: Sending Initiate packet length %d to %s", u32PacketSize, buffer);
                }
                break;
                
            case (OND_PACKET_BLOCK_DATA):
                u32Temp = htonl(psONDPacket->uPayload.sONDPacketBlockData.u32AddressH);
                memcpy(&au8Buffer[u32PacketSize], &u32Temp, sizeof(uint32_t));
                u32PacketSize += sizeof(uint32_t);

                u32Temp = htonl(psONDPacket->uPayload.sONDPacketBlockData.u32AddressL);
                memcpy(&au8Buffer[u32PacketSize], &u32Temp, sizeof(uint32_t));
                u32PacketSize += sizeof(uint32_t);
                
                u32Temp = htonl(psONDPacket->uPayload.sONDPacketBlockData.u32DeviceID);
                memcpy(&au8Buffer[u32PacketSize], &u32Temp, sizeof(uint32_t));
                u32PacketSize += sizeof(uint32_t);
                
                u16Temp = htons(psONDPacket->uPayload.sONDPacketBlockData.u16ChipType);
                memcpy(&au8Buffer[u32PacketSize], &u16Temp, sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
               
                u16Temp = htons(psONDPacket->uPayload.sONDPacketBlockData.u16Revision);
                memcpy(&au8Buffer[u32PacketSize], &u16Temp, sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
          
                u16Temp = htons(psONDPacket->uPayload.sONDPacketBlockData.u16BlockNumber);
                memcpy(&au8Buffer[u32PacketSize], &u16Temp, sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
            
                u16Temp = htons(psONDPacket->uPayload.sONDPacketBlockData.u16TotalBlocks);
                memcpy(&au8Buffer[u32PacketSize], &u16Temp, sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
                     
                u32Temp = htonl(psONDPacket->uPayload.sONDPacketBlockData.u32Timeout);
                memcpy(&au8Buffer[u32PacketSize], &u32Temp, sizeof(uint32_t));
                u32PacketSize += sizeof(uint32_t);
                
                u16Temp = htons(psONDPacket->uPayload.sONDPacketBlockData.u16Length);
                memcpy(&au8Buffer[u32PacketSize], &u16Temp, sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
                
                memcpy(&au8Buffer[u32PacketSize], 
                       psONDPacket->uPayload.sONDPacketBlockData.au8Data, 
                       psONDPacket->uPayload.sONDPacketBlockData.u16Length);
                u32PacketSize += psONDPacket->uPayload.sONDPacketBlockData.u16Length;

                if (verbosity >= 10)
                {
                    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                    inet_ntop(AF_INET6, &sAddr.sin6_addr, buffer, INET6_ADDRSTRLEN);
                    daemon_log(LOG_INFO, "ONDNetwork: Sending Block data packet length %d to %s", u32PacketSize, buffer);
                }
                break;
            
            case (OND_PACKET_RESET):
                u32Temp = htonl(psONDPacket->uPayload.sONDPacketReset.u32DeviceID);
                memcpy(&au8Buffer[u32PacketSize], &u32Temp, sizeof(uint32_t));
                u32PacketSize += sizeof(uint32_t);
                
                u16Temp = htons(psONDPacket->uPayload.sONDPacketReset.u16Timeout);
                memcpy(&au8Buffer[u32PacketSize], &u16Temp, sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
                
                u16Temp = htons(psONDPacket->uPayload.sONDPacketReset.u16DepthInfluence);
                memcpy(&au8Buffer[u32PacketSize], &u16Temp, sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
                
                if (verbosity >= 10)
                {
                    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                    inet_ntop(AF_INET6, &sAddr.sin6_addr, buffer, INET6_ADDRSTRLEN);
                    daemon_log(LOG_INFO, "ONDNetwork: Sending reset packet length %d to %s", u32PacketSize, buffer);
                }
                break;
            
            default:
                daemon_log(LOG_ERR, "ONDNetwork: Unknown packet type to send (%d)", psONDPacket->eType);
                return OND_STATUS_ERROR;
        }
    }
    
    iBytesSent = sendto(psONDNetwork->iSocket, au8Buffer, u32PacketSize, 0, 
                (struct sockaddr*)&sAddr, sizeof(struct sockaddr_in6));

    if (iBytesSent != u32PacketSize)
    {
        daemon_log(LOG_ERR, "ONDNetwork: Failed to send packet (%s)", strerror(errno));
        return OND_STATUS_ERROR;
    }
    
    if (verbosity >= 10)
    {
        char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
        inet_ntop(AF_INET6, &sAddr.sin6_addr, buffer, INET6_ADDRSTRLEN);
        daemon_log(LOG_INFO, "ONDNetwork: Sent packet length %d to %s", iBytesSent, buffer);
    }
    return OND_STATUS_OK;
}



teONDStatus eONDNetworkGetPacket(tsONDNetwork *psONDNetwork, struct in6_addr *psAddress, tsONDPacket *psONDPacket)
{
#define BUFFER_SIZE 1024
    uint8_t au8Buffer[BUFFER_SIZE];
    int32_t iBytesReceived;
    uint32_t u32PacketSize;
    
    struct sockaddr_in6 sRecv_addr;
    unsigned int AddressSize = sizeof(struct sockaddr_in6);
    
    iBytesReceived = recvfrom(psONDNetwork->iSocket, au8Buffer, BUFFER_SIZE, 0,
                                (struct sockaddr*)&sRecv_addr, &AddressSize);

    if (iBytesReceived < 0)
    {
        daemon_log(LOG_ERR, "ONDNetwork: Error receiving (%s)", strerror(errno));
        return OND_STATUS_ERROR;
    }
    
    if (verbosity >= 10)
    {
        char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
        inet_ntop(AF_INET6, &sRecv_addr.sin6_addr, buffer, INET6_ADDRSTRLEN);
        daemon_log(LOG_INFO, "ONDNetwork: Got packet length %d from %s", iBytesReceived, buffer);
    }
    
    psONDPacket->eType = au8Buffer[1];
    u32PacketSize = 4;
    
    {
        uint16_t u16Temp;
        uint32_t u32Temp;
        switch (psONDPacket->eType)
        {
            case (OND_PACKET_BLOCK_REQUEST):
                memcpy(&u32Temp, &au8Buffer[u32PacketSize], sizeof(uint32_t));
                u32PacketSize += sizeof(uint32_t);
                psONDPacket->uPayload.sONDPacketBlockRequest.u32AddressH = ntohl(u32Temp);
                
                memcpy(&u32Temp, &au8Buffer[u32PacketSize], sizeof(uint32_t));
                u32PacketSize += sizeof(uint32_t);
                psONDPacket->uPayload.sONDPacketBlockRequest.u32AddressL = ntohl(u32Temp);
                
                memcpy(&u32Temp, &au8Buffer[u32PacketSize], sizeof(uint32_t));
                u32PacketSize += sizeof(uint32_t);
                psONDPacket->uPayload.sONDPacketBlockRequest.u32DeviceID = ntohl(u32Temp);
                
                memcpy(&u16Temp, &au8Buffer[u32PacketSize], sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
                psONDPacket->uPayload.sONDPacketBlockRequest.u16ChipType = ntohs(u16Temp);
                
                memcpy(&u16Temp, &au8Buffer[u32PacketSize], sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
                psONDPacket->uPayload.sONDPacketBlockRequest.u16Revision = ntohs(u16Temp);
                
                memcpy(&u16Temp, &au8Buffer[u32PacketSize], sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
                psONDPacket->uPayload.sONDPacketBlockRequest.u16BlockNumber = ntohs(u16Temp);
                
                memcpy(&u16Temp, &au8Buffer[u32PacketSize], sizeof(uint16_t));
                u32PacketSize += sizeof(uint16_t);
                psONDPacket->uPayload.sONDPacketBlockRequest.u16RemainingBlocks = ntohs(u16Temp);
            
                if (verbosity >= 10)
                {
                    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                    inet_ntop(AF_INET6, &sRecv_addr.sin6_addr, buffer, INET6_ADDRSTRLEN);
                    daemon_log(LOG_INFO, "ONDNetwork: Got Block request from %s on behalf of 0x%08x%08x - request block %d remainaing %d", 
                               buffer, 
                               psONDPacket->uPayload.sONDPacketBlockRequest.u32AddressH, 
                               psONDPacket->uPayload.sONDPacketBlockRequest.u32AddressL,
                               psONDPacket->uPayload.sONDPacketBlockRequest.u16BlockNumber,
                               psONDPacket->uPayload.sONDPacketBlockRequest.u16RemainingBlocks);
                }
                break;
            
            default:
                if (verbosity >= 5)
                {
                    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                    inet_ntop(AF_INET6, &sRecv_addr.sin6_addr, buffer, INET6_ADDRSTRLEN);
                    daemon_log(LOG_ERR, "ONDNetwork: Received unhandled packet (%d) from %s", psONDPacket->eType, buffer);
                }
                return OND_STATUS_ERROR;
        }
    }
    
    *psAddress = sRecv_addr.sin6_addr;
    
    return OND_STATUS_OK;
}




