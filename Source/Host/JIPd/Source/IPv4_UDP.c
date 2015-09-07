/****************************************************************************
 *
 * MODULE:             JIPv4 compatability daemon
 *
 * COMPONENT:          UDP over IPv4
 *
 * REVISION:           $Revision: 37697 $
 *
 * DATED:              $Date: 2011-12-06 14:41:22 +0000 (Tue, 06 Dec 2011) $
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <net/if.h>
#include <errno.h>

#include <libdaemon/daemon.h>

#ifdef USE_ZEROCONF
#include "Zeroconf.h"
#else
#include "Common.h"
#endif /* USE_ZEROCONF */

extern int verbosity;

typedef struct _tsConectionMapping
{
    struct sockaddr_in  sIPv4Address;
    
    int    iIPv6Socket;
    
    struct timeval sLastPacketTime;
    
    struct _tsConectionMapping *psNext;
} tsConectionMapping;

tsConectionMapping *psConnectionMappingHead;


tsConectionMapping *psGetMapping(struct sockaddr_in *psFromIPv4Address);
tsConectionMapping *psCreateMapping(struct sockaddr_in *psFromIPv4Address);
tsConectionMapping *psDeleteMapping (tsConectionMapping *psMapping);

static int Network_Listen4UDP(const char *pcAddress, int iPort);
static int handle_incoming_ipv4_packet(int listen_socket);
static int handle_incoming_ipv6_packet(int send_socket, tsConectionMapping *psMapping);

static int UDP_Connection_Timeout = (30 * 60);


int IPv4_UDP(const char *pcListen_address, const int iPort)
{
    int listen_socket = 0;
    listen_socket = Network_Listen4UDP(pcListen_address, iPort);
    if (listen_socket < 0)
    {
        daemon_log(LOG_ERR, "Failed to bind");
        return -1;
    }

    while (1)
    {
        struct timeval tv;
        fd_set readfds;
        tsConectionMapping *psConnectionMapping = psConnectionMappingHead;
        int max_fd = 0;

        tv.tv_sec = 2;
        tv.tv_usec = 0;

        FD_ZERO(&readfds);
        FD_SET(listen_socket, &readfds);
        max_fd = listen_socket;
        
        while (psConnectionMapping)
        {
            if (psConnectionMapping->iIPv6Socket > max_fd)
            {
                max_fd = psConnectionMapping->iIPv6Socket;
            }
            
            FD_SET(psConnectionMapping->iIPv6Socket, &readfds);
            
            psConnectionMapping = psConnectionMapping->psNext;
        }

        select(max_fd+1, &readfds, NULL, NULL, &tv);

        if (FD_ISSET(listen_socket, &readfds))
        {
            handle_incoming_ipv4_packet(listen_socket);
        }
        else
        {
            struct timeval sTimeNow;
            gettimeofday(&sTimeNow, NULL);
            
            psConnectionMapping = psConnectionMappingHead;
            
            while (psConnectionMapping)
            {
                if (FD_ISSET(psConnectionMapping->iIPv6Socket, &readfds))
                {
                    handle_incoming_ipv6_packet(listen_socket, psConnectionMapping);
                }
                
                if (sTimeNow.tv_sec > (psConnectionMapping->sLastPacketTime.tv_sec + UDP_Connection_Timeout))
                {
                    if (verbosity >= LOG_DEBUG)
                    {
                        daemon_log(LOG_DEBUG, "Deleting client: %d seconds since last data", UDP_Connection_Timeout);
                    }
                    psConnectionMapping = psDeleteMapping (psConnectionMapping);
                }
                else
                {
                    psConnectionMapping = psConnectionMapping->psNext;
                }
            }
        }

    }

    return 0;
}


static int Network_Listen4UDP(const char *pcAddress, int iPort)
{
    struct addrinfo hints, *res;
    int s;
    char acPort[10];
    
    snprintf(acPort, 10, "%d", iPort);
    
    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
           
    s = getaddrinfo(NULL, acPort, &hints, &res);
    if (s != 0)
    {
        daemon_log(LOG_ERR, "Error in getaddrinfo: %s", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    
    bind(s, res->ai_addr, sizeof(struct sockaddr_in));
    
    freeaddrinfo(res);
    
    return s;
}


static int handle_incoming_ipv4_packet(int listen_socket)
{
    ssize_t iBytesRecieved;
    struct sockaddr_in IPv4Address;
    unsigned int AddressSize = sizeof(struct sockaddr_in);
    tsConectionMapping *psMapping;
    
#define PACKET_BUFFER_SIZE 4096
    char buffer[PACKET_BUFFER_SIZE];
    
    iBytesRecieved = recvfrom(listen_socket, buffer, PACKET_BUFFER_SIZE, 0,
                                                (struct sockaddr*)&IPv4Address, &AddressSize);
    if (iBytesRecieved > 0)
    {
        psMapping = psGetMapping(&IPv4Address);
        if (!psMapping)
        {
            char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
            inet_ntop(AF_INET, &IPv4Address.sin_addr, buffer, INET6_ADDRSTRLEN);
            if (verbosity >= LOG_DEBUG)
            {
                daemon_log(LOG_DEBUG, "Creating mapping for new client %s:%d", buffer, ntohs(IPv4Address.sin_port));
            }
            psMapping = psCreateMapping(&IPv4Address);
        }
        
        {
            char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
            inet_ntop(AF_INET, &IPv4Address.sin_addr, buffer, INET6_ADDRSTRLEN);
            if (verbosity >= LOG_DEBUG)
            {
                daemon_log(LOG_DEBUG, "Data from client %s:%d: %d bytes", buffer, ntohs(IPv4Address.sin_port), (int)iBytesRecieved);
            }
        }

        if (psMapping)
        {
            int iBytesSent;
            struct sockaddr_in6 sDestAddress;
            uint32_t u32HeaderSize;
            
            //printf("Forwarding packet payload to port %x\n", htons(1873));
            
            /* Update last packet time */
            gettimeofday(&psMapping->sLastPacketTime, NULL);
            
            memset (&sDestAddress, 0, sizeof(struct sockaddr_in6));
            sDestAddress.sin6_family  = AF_INET6;
            sDestAddress.sin6_port    = htons(1873);
            
            switch (buffer[0]) /* Version field */
            {
                case (1): 
                    u32HeaderSize = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(struct in6_addr);

                    /* Check if this packet is intended for the coordinator */
                    char acCoordAddr[sizeof(struct in6_addr)] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                    
                    if (memcmp(acCoordAddr, &buffer[sizeof(uint8_t) + sizeof(uint16_t)], sizeof(struct in6_addr)) == 0)
                    {
#ifdef USE_ZEROCONF
                        int iNumAddresses;
                        struct in6_addr *asAddresses;
                        if (ZC_Get_Module_Addresses(&asAddresses, &iNumAddresses) != 0)
                        {
                            daemon_log(LOG_ERR, "Could not get coordinator address");
                            return -1;
                        }
                        
                        if (iNumAddresses != 1)
                        {
                            daemon_log(LOG_ERR, "Got an unhandled number of coordinators (%d)", iNumAddresses);
                            return -1;
                        }
                        else
                        {
                            char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                            inet_ntop(AF_INET6, asAddresses, buffer, INET6_ADDRSTRLEN);
                            daemon_log(LOG_INFO, "Got coordinator address %s", buffer);
                        }
                        memcpy(&sDestAddress.sin6_addr, asAddresses, sizeof(struct in6_addr));
                        free(asAddresses);
#else /* USE_ZEROCONF */
                        /* Read the address from the textfile */
                        if (Get_Module_Address(&sDestAddress.sin6_addr) == 0)
                        {
                            char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                            inet_ntop(AF_INET6, asAddresses, buffer, INET6_ADDRSTRLEN);
                            daemon_log(LOG_INFO, "Got coordinator address %s", buffer);
                        }
                        else
                        {
                            return -1;
                        }
#endif /* USE_ZEROCONF */
                    }
                    else
                    {
                        memcpy(&sDestAddress.sin6_addr, &buffer[sizeof(uint8_t) + sizeof(uint16_t)], sizeof(struct in6_addr));
                    }
                    break;
                    
                default:
                    daemon_log(LOG_ERR, "Unknown JIPv4 header version");
                    return -1;
            }
            
            iBytesSent = sendto(psMapping->iIPv6Socket, &buffer[u32HeaderSize], iBytesRecieved-u32HeaderSize, 0, 
                    (struct sockaddr*)&sDestAddress, sizeof(struct sockaddr_in6));
            if (verbosity >= LOG_DEBUG)
            {
                char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                inet_ntop(AF_INET6, &sDestAddress.sin6_addr, buffer, INET6_ADDRSTRLEN);
                daemon_log(LOG_DEBUG, "Sent %d bytes to address %s", iBytesSent, buffer);
            }
        }
    }
    else
    {
        daemon_log(LOG_DEBUG, "Received a weird number of bytes %d", (int)iBytesRecieved);
    }
#undef PACKET_BUFFER_SIZE
    return 0;
}

static int handle_incoming_ipv6_packet(int send_socket, tsConectionMapping *psMapping)
{
    ssize_t iBytesRecieved;
    struct sockaddr_in6 IPv6Address;
    unsigned int AddressSize = sizeof(struct sockaddr_in6);
    const uint32_t u32HeaderSize = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(struct in6_addr);
    
#define PACKET_BUFFER_SIZE (4096 + u32HeaderSize)
    char buffer[PACKET_BUFFER_SIZE];
    
    iBytesRecieved = recvfrom(psMapping->iIPv6Socket, &buffer[u32HeaderSize], PACKET_BUFFER_SIZE, 0,
                                                (struct sockaddr*)&IPv6Address, &AddressSize);
    if (iBytesRecieved > 0)
    {
        {
            char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
            inet_ntop(AF_INET, &psMapping->sIPv4Address.sin_addr, buffer, INET6_ADDRSTRLEN);
            if (verbosity >= LOG_DEBUG)
            {
                daemon_log(LOG_DEBUG, "Data to client %s:%d: %d bytes", buffer, ntohs(psMapping->sIPv4Address.sin_port), (int)iBytesRecieved);
            }
        }
        
        /* Update last packet time */
        gettimeofday(&psMapping->sLastPacketTime, NULL);

        {
            int iBytesSent;
            uint16_t u16PacketLength;
            
            u16PacketLength = htons(iBytesRecieved + sizeof(struct in6_addr));
            
            buffer[0] = 1;          /* JIPv4 header version */
            memcpy(&buffer[1], &u16PacketLength, sizeof(uint16_t));
            memcpy(&buffer[3], &IPv6Address.sin6_addr, sizeof(struct in6_addr));
            
            iBytesSent = sendto(send_socket, buffer, iBytesRecieved + u32HeaderSize, 0, 
                    (struct sockaddr*)&psMapping->sIPv4Address, sizeof(struct sockaddr_in));
        }
    }
    else
    {
        daemon_log(LOG_DEBUG, "Received a weird number of bytes %d", (int)iBytesRecieved);
    }
#undef PACKET_BUFFER_SIZE
    return 0;
}



tsConectionMapping *psGetMapping(struct sockaddr_in *psFromIPv4Address)
{
    tsConectionMapping *psConnectionMapping = psConnectionMappingHead;
    
    while (psConnectionMapping)
    {
        if (memcmp(psFromIPv4Address, &psConnectionMapping->sIPv4Address, sizeof(struct sockaddr_in)) ==0)
        {
            return psConnectionMapping;
        }
        psConnectionMapping = psConnectionMapping->psNext;
    }
    return NULL;
}



tsConectionMapping *psCreateMapping(struct sockaddr_in *psFromIPv4Address)
{
    tsConectionMapping **ppsNewConnectionMapping;
    
    if (psConnectionMappingHead == NULL)
    {
        ppsNewConnectionMapping = &psConnectionMappingHead;
    }
    else
    {
        tsConectionMapping *psConnectionMapping = psConnectionMappingHead;
        while (psConnectionMapping->psNext)
        {
            psConnectionMapping = psConnectionMapping->psNext;
        }
        ppsNewConnectionMapping = &psConnectionMapping->psNext;
    }
    
    *ppsNewConnectionMapping = malloc(sizeof(tsConectionMapping));
    if (!*ppsNewConnectionMapping)
    {
        daemon_log(LOG_ERR, "Error allocating structure");
        return NULL;
    }
    memset(*ppsNewConnectionMapping, 0, sizeof(tsConectionMapping));
    
    (*ppsNewConnectionMapping)->sIPv4Address = *psFromIPv4Address;
    (*ppsNewConnectionMapping)->psNext = NULL;
    
    {
        struct addrinfo hints, *res;
        int s;
        
        memset(&hints, 0, sizeof (hints));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
        hints.ai_protocol = 0;          /* Any protocol */
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;
            
        s = getaddrinfo(NULL, "1873", &hints, &res);
        if (s != 0)
        {
            daemon_log(LOG_ERR, "Error in getaddrinfo: %s", gai_strerror(s));
            exit(EXIT_FAILURE);
        }

        (*ppsNewConnectionMapping)->iIPv6Socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        
        if ((*ppsNewConnectionMapping)->iIPv6Socket < 0)
        {
            daemon_log(LOG_ERR, "Could not create IPv6 socket (%s)", strerror(errno));
            freeaddrinfo(res);
            return NULL;
        }
        
        freeaddrinfo(res);
        
        {
            int iMaxHops = 2;
            // For Mcast needs to be at least 2 Hops for now - enough to go across the border router from the local network
            if (setsockopt((*ppsNewConnectionMapping)->iIPv6Socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &iMaxHops, sizeof(int)) < 0)
            {
                daemon_log(LOG_ERR, "Error setting Number of hops (%s)", strerror(errno));
            }

            {
                int if_index = if_nametoindex("tun0");
                if (if_index > 0)
                {
                    /* We've got a tun0 - use it */
                    if (setsockopt((*ppsNewConnectionMapping)->iIPv6Socket, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                        &if_index, sizeof(if_index)) < 0)
                    {
                        daemon_log(LOG_ERR, "Error setting sockopt IPV6_MULTICAST_IF (%s)", strerror(errno));
                    }
                }
            }
        }
    }
    
    gettimeofday(&(*ppsNewConnectionMapping)->sLastPacketTime, NULL);

    return *ppsNewConnectionMapping;
}

tsConectionMapping *psDeleteMapping (tsConectionMapping *psMapping)
{
    tsConectionMapping *psDeleteConnectionMapping = NULL;
    tsConectionMapping *psNext = NULL;
    
    if (psConnectionMappingHead == psMapping)
    {
        psDeleteConnectionMapping = psConnectionMappingHead;
        psConnectionMappingHead = psConnectionMappingHead->psNext;
        psNext = psConnectionMappingHead;
    }
    else
    {
        tsConectionMapping *psConnectionMapping = psConnectionMappingHead;
        while (psConnectionMapping->psNext)
        {
            if (psConnectionMapping->psNext == psMapping)
            {
                psDeleteConnectionMapping = psConnectionMapping->psNext;
                psConnectionMapping->psNext = psConnectionMapping->psNext->psNext;
                psNext = psConnectionMapping->psNext;
                break;
            }
            psConnectionMapping = psConnectionMapping->psNext;
        }
    }
    
    if (psDeleteConnectionMapping)
    {
        close(psDeleteConnectionMapping->iIPv6Socket);
        free(psDeleteConnectionMapping);
    }
    
    return psNext;
}
