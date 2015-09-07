/****************************************************************************
 *
 * MODULE:             Firmware Distribution Daemon
 *
 * COMPONENT:          Status monitor
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

#ifndef _STATUS_H_
#define _STATUS_H_

#include <arpa/inet.h>


#include "Firmwares.h"


/** Retur status messages from the status module */
typedef enum    
{
    STATUS_OK,                  /** < All OK */
    STATUS_ERROR,               /** < Generic error */
    STATUS_NO_MEM,              /** < Out of memory */
} teStatus;


/** Initialise the status module
 *  \return STATUS_OK on success
 */
teStatus StatusInit(void);


/** Add an entry to the status tracker for a given firmware transfer
 *  \param sFirmwareID          Structure representing the firmware
 *  \param sAddress             IPv6 address of network entry point
 *  \param u32AddressH          High word of node MAC address
 *  \param u32AddressL          Low word of node MAC address
 *  \param u32RemainingBlocks   Number of blocks remaining for download to node
 *  \param u32TotalBlocks       Total number of blocks in the firmware image
 *  \return STATUS_OK on success */
teStatus StatusLogRemainingBlocks(tsFirmwareID sFirmwareID, struct in6_addr sAddress, 
                                  uint32_t u32AddressH, uint32_t u32AddressL, 
                                  uint32_t u32RemainingBlocks, uint32_t u32TotalBlocks);


/** Clear entries from the status tracker for a given firmware transfer
 *  If MAC Address H and L are both 0xffffffff (i.e. broadcast) then 
 *  Not only will the broadcast entry be cleared, but also all nodes that
 *  have individually downloaded the image
 *  \param sFirmwareID          Structure representing the firmware
 *  \param sAddress             IPv6 address of network entry point
 *  \param u32AddressH          High word of node MAC address
 *  \param u32AddressL          Low word of node MAC address
 *  \return STATUS_OK on success */
teStatus StatusLogRemove(tsFirmwareID sFirmwareID, struct in6_addr sAddress, 
                         uint32_t u32AddressH, uint32_t u32AddressL);

/** Send list of Status records down a socket
 *  \param iSocket          file descriptor of socket
 *  \return FW_STATUS_OK if successful
 */
teStatus eStatus_Remote_Send(int iSocket); 


#endif /* _STATUS_H_ */
