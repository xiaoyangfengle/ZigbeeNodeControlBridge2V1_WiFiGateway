/****************************************************************************
 *
 * MODULE:             Firmware Distribution Daemon
 *
 * COMPONENT:          IPC data format
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


#ifndef _FWDISTRIBUTION_IPC_H_
#define _FWDISTRIBUTION_IPC_H_

#include <stdint.h>
#include <netinet/in.h>

#define FWDistributionIPCSockPath "/tmp/FWDistributiond"

/** Message types passed between client and daemon */
typedef enum
{
    IPC_GET_AVAILABLE_FIRMWARES,        /** < Get a list of available firmware images */
    IPC_AVAILABLE_FIRMWARES,            /** < The list of available firmwares */
    IPC_START_DOWNLOAD,                 /** < Tell the daemon to begin sending an image */
    IPC_CANCEL_DOWNLOAD,                /** < Tell the daemon to cancel sending an image */
    IPC_GET_STATUS,                     /** < Get status of ongoing downloads */
    IPC_STATUS,                         /** < Status packet */
    IPC_RESET,                          /** < Reset packet */
} teFWDistributionIPCType;


/** Message header */
typedef struct
{
    teFWDistributionIPCType     eType;              /** < Type of packet */
    uint32_t                    u32PayloadLength;   /** < Length of payload */
} tsFWDistributionIPCHeader;


/** Record of one available firmware on the server. A sequence of these are sent out in response to a get */
typedef struct
{
    uint32_t u32DeviceID;               /** < Uniquely identifies application */
    uint16_t u16ChipType;               /** < Target hardware of application binary */
    uint16_t u16Revision;               /** < Revision number of application  */
    char     acFilename[256];           /** < Filename */
} tsFWDistributionIPCAvailableFirmwareRecord;


/** Request to start a download of a given firmware to a given coordinator */
typedef struct
{
    uint32_t u32DeviceID;               /** < Uniquely identifies application */
    uint16_t u16ChipType;               /** < Target hardware of application binary */
    uint16_t u16Revision;               /** < Revision number of application  */
    uint16_t u16BlockInterval;          /** < Amount of time between blocks, (in units of 10ms) */
    struct in6_addr sAddress;           /** < Destination IPv6 Address */
    enum teStartDownloadIPCFlags
    {
        E_DOWNLOAD_IPC_FLAG_INFORM = 1, /** < Only inform the network of the firmware */
        E_DOWNLOAD_IPC_FLAG_RESET  = 2, /** < Immediate device reset when image is downloaded */
    } eFlags;                           /** < Flags to the download mechanism */
} tsFWDistributionIPCStartDownload;


/** Request to cancel the download of a given firmware to a given coordinator */
typedef struct
{
    uint32_t u32DeviceID;               /** < Uniquely identifies application */
    uint16_t u16ChipType;               /** < Target hardware of application binary */
    uint16_t u16Revision;               /** < Revision number of application  */
    struct in6_addr sAddress;           /** < Destination IPv6 Address */
} tsFWDistributionIPCCancelDownload;


/** Request nodes to reset */
typedef struct
{
    uint32_t u32DeviceID;               /** < Uniquely identifies application */
    uint16_t u16Timeout;                /** < Timeout before reset (units of 10ms) */
    uint16_t u16DepthInfluence;         /** < Influence of depth on reset timeout */
    struct in6_addr sAddress;           /** < Destination IPv6 Address */
    uint16_t u16RepeatCount;            /** < Number of repeats of the reset request to send */
    uint16_t u16RepeatTime;             /** < Amount of time between reset messages, (in units of 10ms) */
} tsFWDistributionIPCReset;


/** Status of a firmware load to a coordinator and individual mac address */
typedef struct
{
    uint32_t u32DeviceID;               /** < Uniquely identifies application */
    uint16_t u16ChipType;               /** < Target hardware of application binary */
    uint16_t u16Revision;               /** < Revision number of application  */
    struct in6_addr sAddress;           /** < Destination IPv6 Address */
    uint32_t u32AddressH;               /** < Destination MAC address (high word) of node getting image */
    uint32_t u32AddressL;               /** < Destination MAC address (low word)of node getting image */
    uint32_t u32RemainingBlocks;        /** < Number of blocks remaining */
    uint32_t u32TotalBlocks;            /** < Number of blocks total */
    uint32_t u32Status;                 /** < Node status */
} tsFWDistributionIPCStatusRecord;



#endif /* _FWDISTRIBUTION_IPC_H_ */
