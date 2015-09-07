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

#ifndef __OND_NETWORK_H__
#define __OND_NETWORK_H__

#include <stdint.h>
#include <netinet/in.h>

#include "OND.h"
#include "OND_Packet.h"

typedef struct
{
    int iSocket;
    
    uint16_t    u16PortNumber;
    
} tsONDNetwork;


teONDStatus eONDNetworkInitialise(tsONDNetwork *psONDNetwork, const char *pcBindAddress, const char *pcPortNumber);


/** Send an OND packet to the coordinator
 *  \param psONDNetwork                 Pointer to network structure
 *  \param sAddress                     Structure containing IPv6 address of Coordinator node
 *  \param psONDPacket                  Pointer to packet to send
 *  \return OND_STATUS_OK if successful
 */
teONDStatus eONDNetworkSendPacket(tsONDNetwork *psONDNetwork, struct in6_addr sAddress, tsONDPacket *psONDPacket);


/** Attempt to get a packet from the listening socket.
 *  Blocks until a packet arrives.
 *  \param psONDNetwork                 Pointer to network structure
 *  \param psAddress[out]               Pointer to address that packet was received from
 *  \param psONDPacket[out]             Pointer to storage in which to store the received packet
 *  \return OND_STATUS_OK if successful
 */
teONDStatus eONDNetworkGetPacket(tsONDNetwork *psONDNetwork, struct in6_addr *psAddress, tsONDPacket *psONDPacket);

#endif /* __OND_NETWORK_H__ */

