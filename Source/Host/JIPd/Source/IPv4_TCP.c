/****************************************************************************
 *
 * MODULE:             JIPv4 compatability daemon
 *
 * COMPONENT:          TCP over IPv4
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
#include <net/if.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>

#include <libdaemon/daemon.h>

#ifdef USE_ZEROCONF
#include "Zeroconf.h"
#else
#include "Common.h"
#endif /* USE_ZEROCONF */


typedef struct
{
    int                 iClientSocket;
    int                 iIPv6Socket;
    struct sockaddr_in  sClientIPv4Address;
    size_t              AddressSize;
    pthread_t           sThreadInfo;
    
} tsTCPThreadInfo;

extern int verbosity;

static int Network_Listen4TCP(const char *pcAddress, int iPort);

void *TCP_Client_Thread(void *args);
static int TCP_handle_incoming_ipv4_packet(tsTCPThreadInfo *psTCPThreadInfo);
static int TCP_handle_incoming_ipv6_packet(tsTCPThreadInfo *psTCPThreadInfo);


int IPv4_TCP(const char *pcListen_address, const int iPort)
{
    int listen_socket = 0;
    listen_socket = Network_Listen4TCP(pcListen_address, iPort);
    if (listen_socket < 0)
    {
        daemon_log(LOG_ERR, "Failed to bind");
        return -1;
    }
        
    daemon_log(LOG_INFO, "Waiting for TCP connections");

    while (1)
    {
        tsTCPThreadInfo *psTCPThreadInfo;
        int iClient_fd;
        struct sockaddr_in sClientIPv4Address;
        socklen_t Addr_size = sizeof(struct sockaddr);

        iClient_fd = accept(listen_socket, (struct sockaddr *)&sClientIPv4Address, &Addr_size);

        {
            char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
            inet_ntop(AF_INET, &sClientIPv4Address.sin_addr, buffer, INET6_ADDRSTRLEN);
            if (verbosity > 0)
            {
                daemon_log(LOG_INFO, "Got TCP client from %s:%d", buffer, ntohs(sClientIPv4Address.sin_port));
            }
        }
        psTCPThreadInfo = malloc(sizeof(tsTCPThreadInfo));
        if (!psTCPThreadInfo)
        {
            daemon_log(LOG_ERR, "Error creating thread (out of memory)");
            close(iClient_fd);
            continue;
        }
        
        memset(psTCPThreadInfo, 0, sizeof(tsTCPThreadInfo));
        
        psTCPThreadInfo->iClientSocket = iClient_fd;
        psTCPThreadInfo->sClientIPv4Address = sClientIPv4Address;

        if (pthread_create(&psTCPThreadInfo->sThreadInfo, NULL, TCP_Client_Thread, psTCPThreadInfo) != 0)
        {
            daemon_log(LOG_ERR, "Error starting TCP thread (%s)", strerror(errno));
            free(psTCPThreadInfo);
            close(iClient_fd);
            continue;
        }
    }

    return 0;
}



static int Network_Listen4TCP(const char *pcAddress, int iPort)
{
    struct addrinfo hints, *res;
    int s;
    char acPort[10];
    
    snprintf(acPort, 10, "%d", iPort);
    
    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
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
    
    if (bind(s, res->ai_addr, sizeof(struct sockaddr_in)) < 0)
    {
        daemon_log(LOG_ERR, "Failed to bind (%s)", strerror(errno));
        return -1;
    }
    
    if (listen(s, 5) < 0)
    {
        daemon_log(LOG_ERR, "Failed to listen (%s)", strerror(errno));
        return -1;
    }
    
    freeaddrinfo(res);
    
    return s;
}


void *TCP_Client_Thread(void *args)
{
    tsTCPThreadInfo *psTCPThreadInfo = (tsTCPThreadInfo *)args;
    int thread_running = 1;

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
        
        // Connect to JIP default port
        s = getaddrinfo(NULL, "1873", &hints, &res);
        if (s != 0)
        {
            daemon_log(LOG_ERR, "Error in getaddrinfo: %s", gai_strerror(s));
            return NULL;
        }

        psTCPThreadInfo->iIPv6Socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        
        if (psTCPThreadInfo->iIPv6Socket < 0)
        {
            daemon_log(LOG_ERR, "Could not create IPv6 socket (%s)", strerror(errno));
            freeaddrinfo(res);
            return NULL;
        }
        
        freeaddrinfo(res);
        
        {
            int iMaxHops = 2;
            // For Mcast needs to be at least 2 Hops for now - enough to go across the border router from the local network
            if (setsockopt(psTCPThreadInfo->iIPv6Socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &iMaxHops, sizeof(int)) < 0)
            {
                daemon_log(LOG_ERR, "Error setting Number of hops (%s)", strerror(errno));
            }

            {
                int if_index = if_nametoindex("tun0");
                if (if_index > 0)
                {
                    /* We've got a tun0 - use it */
                    if (setsockopt(psTCPThreadInfo->iIPv6Socket, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                        &if_index, sizeof(if_index)) < 0)
                    {
                        daemon_log(LOG_ERR, "Error setting sockopt IPV6_MULTICAST_IF (%s)", strerror(errno));
                    }
                }
            }
        }
    }
    
    while (thread_running) 
    {
        struct timeval tv;
        fd_set readfds;
        int max_fd = 0;

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        FD_ZERO(&readfds);
        
        FD_SET(psTCPThreadInfo->iClientSocket, &readfds);
        max_fd = psTCPThreadInfo->iClientSocket;
        
        FD_SET(psTCPThreadInfo->iIPv6Socket, &readfds);
        if (psTCPThreadInfo->iIPv6Socket > psTCPThreadInfo->iClientSocket)
        {
            max_fd = psTCPThreadInfo->iIPv6Socket;
        }

        select(max_fd+1, &readfds, NULL, NULL, &tv);

        if (FD_ISSET(psTCPThreadInfo->iClientSocket, &readfds))
        {
            if (TCP_handle_incoming_ipv4_packet(psTCPThreadInfo) <= 0)
            {
                thread_running = 0;
                break;
            }
        }
        else if (FD_ISSET(psTCPThreadInfo->iIPv6Socket, &readfds))
        {
            if (TCP_handle_incoming_ipv6_packet(psTCPThreadInfo) <= 0)
            {
                thread_running = 0;
                break;
            }
        }
    }
    
    if (verbosity >= LOG_INFO)
    {
        daemon_log(LOG_INFO, "TCP Client disconnected");
    }

    close(psTCPThreadInfo->iClientSocket);
    close(psTCPThreadInfo->iIPv6Socket);
    
    free(psTCPThreadInfo);
    return NULL;
}


static int TCP_handle_incoming_ipv4_packet(tsTCPThreadInfo *psTCPThreadInfo)
{
    ssize_t iBytesRecieved;
    
#define PACKET_BUFFER_SIZE 4096
    char buffer[PACKET_BUFFER_SIZE];
    int iProtocolVersion;
    
    iBytesRecieved = recv(psTCPThreadInfo->iClientSocket, buffer, 1, 0);
    if (iBytesRecieved != 1)
    {
        daemon_log(LOG_ERR, "Could not get TCP packet header");
        return iBytesRecieved;
    }
    iProtocolVersion = buffer[0];
    
    switch (iProtocolVersion)
    {
        case (1):
        {
            uint16_t u16PacketLength;
            iBytesRecieved = recv(psTCPThreadInfo->iClientSocket, &u16PacketLength, 2, 0);
            if (iBytesRecieved != 2)
            {
                daemon_log(LOG_ERR, "Could not get TCP packet length");
                return iBytesRecieved;
            }
            u16PacketLength = ntohs(u16PacketLength);
            
            iBytesRecieved = recv(psTCPThreadInfo->iClientSocket, buffer, u16PacketLength, 0);
            if (iBytesRecieved != u16PacketLength)
            {
                daemon_log(LOG_ERR, "Could not get TCP packet payload");
                return iBytesRecieved;
            }
            
            {
                char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                inet_ntop(AF_INET, &psTCPThreadInfo->sClientIPv4Address.sin_addr, buffer, INET6_ADDRSTRLEN);
                if (verbosity >= LOG_DEBUG)
                {
                    daemon_log(LOG_DEBUG, "Data from client %s:%d: %d bytes", buffer, ntohs(psTCPThreadInfo->sClientIPv4Address.sin_port), (int)iBytesRecieved);
                }
            }
        
            {
                int iBytesSent;
                struct sockaddr_in6 sDestAddress;
                const uint32_t u32HeaderSize = sizeof(struct in6_addr);

                memset (&sDestAddress, 0, sizeof(struct sockaddr_in6));
                sDestAddress.sin6_family  = AF_INET6;
                sDestAddress.sin6_port    = htons(1873);

                {
                    /* Check if this packet is intended for the coordinator */
                    char acCoordAddr[sizeof(struct in6_addr)] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                    if (memcmp(acCoordAddr, buffer, sizeof(struct in6_addr)) == 0)
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
                        memcpy(&sDestAddress.sin6_addr, buffer, sizeof(struct in6_addr));
                    }
                    
                    iBytesSent = sendto(psTCPThreadInfo->iIPv6Socket, &buffer[u32HeaderSize], iBytesRecieved-u32HeaderSize, 0, 
                                               (struct sockaddr*)&sDestAddress, sizeof(struct sockaddr_in6));
                }
            }
            break;
        }
        default:
            daemon_log(LOG_ERR, "Unknown protocol version %d", iProtocolVersion);
            break;
    }
#undef PACKET_BUFFER_SIZE
    return iBytesRecieved;
}

#if 1
static int TCP_handle_incoming_ipv6_packet(tsTCPThreadInfo *psTCPThreadInfo)
{
    ssize_t iBytesRecieved;
    struct sockaddr_in6 IPv6Address;
    unsigned int AddressSize = sizeof(struct sockaddr_in6);
    const uint32_t u32HeaderSize = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(struct in6_addr);
    
#define PACKET_BUFFER_SIZE (4096 + u32HeaderSize)
    char buffer[PACKET_BUFFER_SIZE];
    
    iBytesRecieved = recvfrom(psTCPThreadInfo->iIPv6Socket, &buffer[u32HeaderSize], PACKET_BUFFER_SIZE, 0,
                                                (struct sockaddr*)&IPv6Address, &AddressSize);
    if (iBytesRecieved > 0)
    {
        {
            char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
            inet_ntop(AF_INET, &psTCPThreadInfo->sClientIPv4Address.sin_addr, buffer, INET6_ADDRSTRLEN);
            if (verbosity >= LOG_DEBUG)
            {
                daemon_log(LOG_DEBUG, "Data to client %s:%d: %d bytes", buffer, ntohs(psTCPThreadInfo->sClientIPv4Address.sin_port), (int)iBytesRecieved);
            }
        }

        {
            int iBytesSent;
            uint16_t u16PacketLength;
            
            u16PacketLength = htons(iBytesRecieved + sizeof(struct in6_addr));
            
            buffer[0] = 1;          /* JIPv4 header version */
            memcpy(&buffer[1], &u16PacketLength, sizeof(uint16_t));
            memcpy(&buffer[3], &IPv6Address.sin6_addr, sizeof(struct in6_addr));
            
            iBytesSent = send(psTCPThreadInfo->iClientSocket, buffer, iBytesRecieved + u32HeaderSize, 0);
        }
    }
#undef PACKET_BUFFER_SIZE
    return 1;
}
#endif







