/****************************************************************************
 *
 * MODULE:             Firmware Distribution Daemon
 *
 * COMPONENT:          Packet definitions
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

#ifndef __OND_PACKET_TYPES__
#define __OND_PACKET_TYPES__

#include <stdint.h>


/** Length of the block data payload. Smaller blocks are zero padded to this size */
#define BLOCK_DATA_SIZE 64


/** Valid packet types to send between host and node */
typedef enum
{
    OND_PACKET_INITIATE             =   0,  /** < Inform the node / coordinator a download is about to begin */
    OND_PACKET_BLOCK_DATA           =   1,  /** < Send a block of binary data from the host to the node */
    OND_PACKET_BLOCK_REQUEST        =   2,  /** < Node request for a block of data */
    OND_PACKET_RESET                =   3,  /** < Reset request */
} teONDPacketType;


/** Packet structure for initiate download packet */
typedef struct
{
    uint32_t        u32DeviceID;        /** < Device Id that will be upgraded */
    uint16_t        u16ChipType;        /** < New firmware chip type */
    uint16_t        u16Revision;        /** < New firmware revision */
} tsONDPacketInitiate;


/** Packet structure for block data packet */
typedef struct
{
    uint32_t        u32AddressH;        /** < High 32 bits of destination address (0xFFFFFFFF for broadcast) */
    uint32_t        u32AddressL;        /** < Low 32 bits of destination address (0xFFFFFFFF for broadcast) */
    
    uint32_t        u32DeviceID;        /** < Device Id that will be upgraded */
    uint16_t        u16ChipType;        /** < New firmware chip type */
    uint16_t        u16Revision;        /** < New firmware revision */
    
    uint16_t        u16BlockNumber;     /** < Block number contained in this packet */
    uint16_t        u16TotalBlocks;     /** < Number of blocks in the complete image */
    
    uint32_t        u32Timeout;         /** < Timeout value for this packet */
    
    uint16_t        u16Length;          /** < Length of data content of this packet */
    
    uint8_t         au8Data[BLOCK_DATA_SIZE];   /** < Array of binary data */
    
} tsONDPacketBlockData;


/** Packet structure for block data request packet */
typedef struct
{
    uint32_t        u32AddressH;        /** < High 32 bits of destination address (0xFFFFFFFF for broadcast) */
    uint32_t        u32AddressL;        /** < Low 32 bits of destination address (0xFFFFFFFF for broadcast) */
    
    uint32_t        u32DeviceID;        /** < Required Device Id */
    uint32_t        u16ChipType;        /** < Required chip ID */
    uint32_t        u16Revision;        /** < Required revision */
    
    uint16_t        u16BlockNumber;     /** < Block number contained in this packet */
    uint16_t        u16RemainingBlocks; /** < Number of blocks remaingin to complete the image */
    
} tsONDPacketBlockRequest;


/** Packet structure for reset request packet */
typedef struct
{
    uint32_t        u32DeviceID;        /** < Required Device Id */
    uint16_t        u16Timeout;         /** < Timeout to reset (in units of 10ms) */
    uint16_t        u16DepthInfluence;  /** < Amount of influence that network depth has on reset timeout */
} tsONDPacketResetRequest;



/** Packet structure for OND packet */
typedef struct
{
    uint8_t         u8Version;
    teONDPacketType eType;
    uint8_t         u8Handle;
    uint8_t         u8Pad;
    
    union
    {
        tsONDPacketInitiate     sONDPacketInitiate;
        tsONDPacketBlockData    sONDPacketBlockData;
        tsONDPacketBlockRequest sONDPacketBlockRequest;
        tsONDPacketResetRequest sONDPacketReset;
    } uPayload;
} tsONDPacket;

#endif /* __OND_PACKET_TYPES__ */

