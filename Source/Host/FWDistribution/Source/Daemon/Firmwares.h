/****************************************************************************
 *
 * MODULE:             Firmware Distribution Daemon
 *
 * COMPONENT:          Available firmwares
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

#ifndef __FIRMWARES_H__
#define __FIRMWARES_H__

#include <stdint.h>


/** Enumerated type of firmware module status codes */
typedef enum
{
    FW_STATUS_OK,                       /** < Operation succeeded */
    FW_STATUS_NO_FIRMWARE,              /** < Request for unknown firmware */
    FW_STATUS_BAD_FILE,                 /** < Invalid file */
    FW_STATUS_NO_MEM,                   /** < Could not allocate memory */
    FW_STATUS_REMOTE_ERROR,             /** < Could not send details */
    FW_STATUS_ERROR,                    /** < Generic error */
} teFWStatus;


/** Structure to uniquely identify a firmware */
typedef struct
{
    uint32_t u32DeviceID;               /** < Uniquely identifies application */
    uint16_t u16ChipType;               /** < Target hardware of application binary */
    uint16_t u16Revision;               /** < Revision number of application  */
} tsFirmwareID;


/** Initialise Firmware module
 *  \return FW_STATUS_OK if successful
 */
teFWStatus eFirmware_Init(void);


/** Open a firmware file
 *  \param pu8Filename      Pointer to filename string
 *  \return FW_STATUS_OK if successful
 */
teFWStatus eFirmware_Open(const char *pu8Filename);


/** Close a firmware file
 *  \param pu8Filename      Pointer to filename string
 *  \return FW_STATUS_OK if successful
 */
teFWStatus eFirmware_Close(const char *pu8Filename);


/** Get number of blocks that make up the image
 *  \param sFirmwareID      Unique ID of the firmware image
 *  \param u32BlockSize     Block size to chunk the image into
 *  \param pu32TotalBlocks[out] Return of the number of blocks in the image
 *  \return FW_STATUS_OK if successful
 */
teFWStatus eFirmware_Get_Total_Blocks(tsFirmwareID sFirmwareID, uint32_t u32BlockSize, uint32_t *pu32TotalBlocks);


/** Get a block of firmware data from a loaded firmware file
 *  \param sFirmwareID      Unique ID of the firmware image
 *  \param u32BlockNumber   Block number to get
 *  \param u32BlockSize     Block size to chunk the image into
 *  \param pu8Data[out]     Pointer to memory block to copy block data into
 *  \return FW_STATUS_OK if successful
 */
teFWStatus eFirmware_Get_Block(tsFirmwareID sFirmwareID, uint32_t u32BlockNumber, uint32_t u32BlockSize, uint8_t *pu8Data); 


/** Set a new timeout for a loaded firmware file
 *  \param sFirmwareID      Unique ID of the firmware image
 *  \param u32Timeout       Timeout value to set
 *  \return FW_STATUS_OK if successful
 */
teFWStatus eFirmware_Set_Timeout(tsFirmwareID sFirmwareID, uint32_t u32Timeout); 


/** Get the timeout for a loaded firmware file
 *  \param sFirmwareID      Unique ID of the firmware image
 *  \param u32Timeout[out]  Pointer to location for timeout value
 *  \return FW_STATUS_OK if successful
 */
teFWStatus eFirmware_Get_Timeout(tsFirmwareID sFirmwareID, uint32_t *pu32Timeout); 


/** Send list of available firmwares down a socket
 *  \param iSocket          file descriptor of socket
 *  \return FW_STATUS_OK if successful
 */
teFWStatus eFirmware_Remote_Send(int iSocket); 



#endif /* __FIRMWARES_H__ */

