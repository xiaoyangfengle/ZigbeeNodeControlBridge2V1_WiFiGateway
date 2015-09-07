/****************************************************************************
 *
 * MODULE:             Firmware Distribution Daemon
 *
 * COMPONENT:          OND Interface
 *
 * REVISION:           $Revision: 49775 $
 *
 * DATED:              $Date: 2012-11-22 15:46:15 +0000 (Thu, 22 Nov 2012) $
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

#ifndef __OND_H__
#define __OND_H__

#include <stdint.h>


/** Return status codes from OND module */
typedef enum
{
    OND_STATUS_OK,                      /** < All ok */
    OND_STATUS_ERROR,                   /** < Generic catch all error condition */

} teONDStatus;

#include "OND_Network.h"
#include "Threads.h"


/** Flags to control aspects of the download */
typedef enum
{
    E_DOWNLOAD_FLAG_INITIATE_ONLY   = 1,  /** < Flag to send the initiate packet only */
    E_DOWNLOAD_FLAG_IMMEDIATE_RESET = 2,  /** < Flag immediate reset */
} teFlags;


/** Structure containing data for OND server */
typedef struct
{
    tsONDNetwork    sONDNetwork;        /** < Structure for network */
    
    tsLock          sServerLock;        /** < Lock for server thread access */
    
    tsThread        sServerThread;      /** < Server Thread data */
    
} tsOND;


/** Initialise the OND engine.
 *  Set up structures and start a server thread
 *  \return OND_STATUS_OK on success
 */
teONDStatus eONDInitialise(tsOND *psOND, const char *pcBindAddress, const char *pcPortNumber);


/** Start a broadcast download of a firmware image
 *  \param psOND                Pointer to the initialised OND structure
 *  \param sCoordinatorAddress  IPv6 address of network entry point
 *  \param u32DeviceID          Device ID of the destination nodes
 *  \param u16ChipType          Chip type of the destination nodes
 *  \param u16Revision          Which revision to broadcast
 *  \param u16BlockInterval     Time interval between blocks in units of 10ms.
 *  \param eFlags               Any flags set on the download
 *  \return OND_STATUS_OK on success.
 */
teONDStatus eONDStartDownload(tsOND *psOND, struct in6_addr sCoordinatorAddress, uint32_t u32DeviceID, 
                              uint16_t u16ChipType, uint16_t u16Revision, uint16_t u16BlockInterval, teFlags eFlags);


/** Cancel a broadcast download of a firmware image
 *  \param psOND                Pointer to the initialised OND structure
 *  \param sCoordinatorAddress  IPv6 address of network entry point
 *  \param u32DeviceID          Device ID of the destination nodes
 *  \param u16ChipType          Chip type of the destination nodes
 *  \param u16Revision          Which revision to cancel
 *  \return OND_STATUS_OK on success.
 */
teONDStatus eONDCancelDownload(tsOND *psOND, struct in6_addr sCoordinatorAddress, uint32_t u32DeviceID, 
                               uint16_t u16ChipType, uint16_t u16Revision);


/** Request nodes reset
 *  \param psOND                Pointer to the initialised OND structure
 *  \param sCoordinatorAddress  IPv6 address of network entry point
 *  \param u32DeviceID          Device ID of the destination nodes
 *  \param u16Timeout           Time before reset (measured in units of 10ms)
 *  \param u16DepthInfluence    How many addition units of 10ms each depth should add. 
 *                              This should lead to the tree root resetting first and then flooding down the tree.
 *  \param u16RepeatCount       Number of times to repeat the reset command
 *  \param u16RepeatTime        Time to wait between messages (measured in units of 10ms)
 *  \return OND_STATUS_OK on success.
 */
teONDStatus eONDRequestReset(tsOND *psOND, struct in6_addr sCoordinatorAddress, uint32_t u32DeviceID, 
                             uint16_t u16Timeout, uint16_t u16DepthInfluence, 
                             uint16_t u16RepeatCount, uint16_t u16RepeatTime);


#endif /* __OND_H__ */

