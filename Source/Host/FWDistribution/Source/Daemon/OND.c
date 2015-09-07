/****************************************************************************
 *
 * MODULE:             Firmware Distribution Daemon
 *
 * COMPONENT:          OND Interface
 *
 * REVISION:           $Revision: 51420 $
 *
 * DATED:              $Date: 2013-01-10 09:57:51 +0000 (Thu, 10 Jan 2013) $
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
#include "Firmwares.h"
#include "Status.h"

#define DBG_FUNCTION_CALLS 0
#define DBG_OND 0


#define OND_BLOCK_SIZE 32

extern int verbosity;

/** Structure to represent a download thread */
typedef struct _tsONDDownload
{
    tsLock                  sStopSignal;        /** < Lock used to signal thread to stop.
                                                      Remains locked by main thread unless thread is to stop. */    
    
    tsOND *psOND;                               /** < Pointer to OND information */
    teFlags eFlags;                             /** < Flags for the download */
    struct in6_addr         sCoordinatorAddress;/** < IPv6 address of coordinator node */
    tsFirmwareID            sFirmwareID;        /** < ID of firmware to download */

    uint16_t                u16BlockInterval;   /** < Amount of time between blocks, (in units of 10ms) */
    tsThread                sThreadInfo;        /** < Monitor Thread data */
    struct _tsONDDownload   *psNext;            /** < Pointer to next in list */
} tsONDDownloadThreadInfo;


/** Structure to represent a reset thread */
typedef struct _tsONDReset
{
    tsOND *psOND;                               /** < Pointer to OND information */
    struct in6_addr sCoordinatorAddress;        /** < IPv6 address of coordinator node */
    uint32_t u32DeviceID;                       /** < Device ID to reset */                       
    
    uint16_t u16Timeout;                        /** < Timeout for reset (units of 10ms) */
    uint16_t u16DepthInfluence;                 /** < Amount of depth infulence to apply */
    
    uint16_t u16RepeatCount;                    /** < How many times to repeat the command */
    uint16_t u16RepeatTime;                     /** < Time between repeats (units of 10ms) */

    tsThread                sThreadInfo;        /** < Monitor Thread data */
} tsONDResetThreadInfo;


/** Head of linked list */
static tsONDDownloadThreadInfo *psONDDownloadThread_ListHead = NULL;


/** Thread protection for OND Download threads linked list */
static tsLock sONDDownloadThreadListLock;


/** Firmware server thread
 *  This thread serves ad-hoc requests from the network for firmware blocks
 *  \param psThreadInfoVoid  Pointer to ThreadInfo structure containing pointer to OND structure
 *  \return None
 */
static void *ONDServerThread(void *psThreadInfoVoid);


/** Firmware download thread
 *  These threads do a broadcast download of blocks of a signel firmware image.
 *  After successful download, the thread quits.
 *  Any missing blocks are sent by the server thread
 *  \param psThreadInfoVoid  Pointer to ThreadInfo structure containging pointer to tsONDDownloadThreadInfo structure
 *  \return None
 */
static void *ONDDownloadThread(void *psThreadInfoVoid);


/** Function to send a single block of a firmware image.
 *  Used by both the server thread and all download threads
 *  \param psOND                Pointer to OND structure (used for network access)
 *  \param sCoordinatorAddress  IPv6 Address of Coordinator
 *  \param u32AddressH          High word of Desitnation node MAC address
 *  \param u32AddressL          Low word of Desitnation node MAC address
 *  \param sFirmwareID          Structure defining the firmware to be sent
 *  \param u16BlockNumber       Which block number of the image to send
 *  \param u16TotalBlocks       Total number of blocks in the image
 *  \param u32Timeout           Timeout for node. (Or'ing with 0x80000000 means reset immediately after download)
 *                              Timeout measured in xxx fractions of a second
 *  \return OND_STATUS_OK on success
 */
static teONDStatus eONDSendBlock(tsOND *psOND, struct in6_addr sCoordinatorAddress, 
                                 uint32_t u32AddressH, uint32_t u32AddressL, tsFirmwareID sFirmwareID, 
                                 uint16_t u16BlockNumber, uint16_t u16TotalBlocks, uint32_t u32Timeout);


/** Find a reference to an ongoing download thread
 *  The calling function should lock the \ref sONDDownloadThreadListLock before calling
 *  this function to ensure any returned reference is still valid.
 *  \param sCoordinatorAddress  IPv6 address of network entry point
 *  \param u32DeviceID          Device ID of the destination nodes
 *  \param u16ChipType          Chip type of the destination nodes
 *  \param u16Revision          Which revision to broadcast
 *  \return Pointer to download thread info structure if found, otherwise NULL.
 */
static tsONDDownloadThreadInfo *psONDDownloadThread_Find(struct in6_addr sCoordinatorAddress, uint32_t u32DeviceID, 
                              uint16_t u16ChipType, uint16_t u16Revision);


/* Initialise module */
teONDStatus eONDInitialise(tsOND *psOND, const char *pcBindAddress, const char *pcPortNumber)
{
    teONDStatus eONDStatus;
    
    /* Initialise network */
    eONDStatus = eONDNetworkInitialise(&psOND->sONDNetwork, pcBindAddress, pcPortNumber);
    if (eONDStatus != OND_STATUS_OK)
    {
        return eONDStatus;
    }
    
    /* Initialise download threads related items */
    if (eLockCreate(&sONDDownloadThreadListLock) != E_LOCK_OK)
    {
        daemon_log(LOG_CRIT, "OND: Failed to initialise lock");
        return OND_STATUS_ERROR;
    }
    psONDDownloadThread_ListHead = NULL;
    
    
    /* Initialise server thread releated items */
    if (eLockCreate(&psOND->sServerLock) != E_LOCK_OK)
    {
        daemon_log(LOG_CRIT, "OND: Failed to initialise lock");
        return OND_STATUS_ERROR;
    }

    psOND->sServerThread.pvThreadData = psOND;

    if (eThreadStart(ONDServerThread, &psOND->sServerThread) != E_THREAD_OK)
    {
        daemon_log(LOG_CRIT, "OND: Failed to start server thread");
        return OND_STATUS_ERROR;
    }

    return OND_STATUS_OK;
}


/* Server thread function */
static void *ONDServerThread(void *psThreadInfoVoid)
{
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsOND *psOND = (tsOND *)psThreadInfo->pvThreadData;
    
    while (1)
    {
        tsONDPacket sONDReceivedPacket;
        struct in6_addr sAddress;
        
        if (eONDNetworkGetPacket(&psOND->sONDNetwork, &sAddress, &sONDReceivedPacket) == OND_STATUS_OK)
        {
            /* Process the packet */
            if (sONDReceivedPacket.eType == OND_PACKET_BLOCK_REQUEST)
            {
                uint32_t u32TotalBlocks;
                teFWStatus eFWStatus;
                tsFirmwareID sFirmwareID;
                uint32_t u32Timeout;
                
                sFirmwareID.u32DeviceID = sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u32DeviceID;
                sFirmwareID.u16ChipType = sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u16ChipType;
                sFirmwareID.u16Revision = sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u16Revision;

                /* Find out how many blocks are in the firmware, if we have it loaded */
                eFWStatus = eFirmware_Get_Total_Blocks(sFirmwareID, OND_BLOCK_SIZE, &u32TotalBlocks);
                if (eFWStatus != FW_STATUS_OK)
                {
                    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                    inet_ntop(AF_INET6, &sAddress, buffer, INET6_ADDRSTRLEN);
                    daemon_log(LOG_ERR, "OND: No firmware revision %d for device ID 0x%08x for coordinator \"%s\" is loaded",
                            sFirmwareID.u16Revision, sFirmwareID.u32DeviceID, buffer);
                    
                    /* No error response to the node, just keep going */
                    continue;
                }
                
                /* Got a matching firmware to that being requested */
                
                /* Check if this is a message to let us know the download has completed */
                if (sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u16RemainingBlocks == 0)
                {
                    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                    inet_ntop(AF_INET6, &sAddress, buffer, INET6_ADDRSTRLEN);
                    daemon_log(LOG_ERR, "OND: Download complete from coordinator \"%s\" on behalf of 0x%08x%08x",
                            buffer,
                            sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u32AddressH,
                            sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u32AddressL);
                    
                    /* Log download complete */
                    StatusLogRemainingBlocks(sFirmwareID, sAddress, 
                                             sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u32AddressH, 
                                             sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u32AddressL, 
                                             0, u32TotalBlocks);
                    
                    continue;
                }
                
                /* Check that a valid block is being requested */
                if (sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u16BlockNumber > u32TotalBlocks)
                {
                    if (verbosity > 5)
                    {
                        char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                        inet_ntop(AF_INET6, &sAddress, buffer, INET6_ADDRSTRLEN);
                        daemon_log(LOG_INFO, "OND: Request for invalid Block %d/%d of revision %d for device ID 0x%08x from coordinator \"%s\" on behalf of 0x%08x%08x",
                                   sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u16BlockNumber, u32TotalBlocks, 
                                   sFirmwareID.u16Revision, sFirmwareID.u32DeviceID, buffer,
                                   sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u32AddressH,
                                   sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u32AddressL);
                        continue;
                    }
                }
                
                
                if (eFirmware_Get_Timeout(sFirmwareID, &u32Timeout) == FW_STATUS_OK)
                {
                
                    /* Send the block */
                    (void)eONDSendBlock(psOND, sAddress, 
                                    sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u32AddressH,
                                    sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u32AddressL,
                                    sFirmwareID, 
                                    sONDReceivedPacket.uPayload.sONDPacketBlockRequest.u16BlockNumber,
                                    u32TotalBlocks, u32Timeout);
                }
            }
        }
    }
    return NULL;
}


/* Start a download */
teONDStatus eONDStartDownload(tsOND *psOND, struct in6_addr sCoordinatorAddress, 
                              uint32_t u32DeviceID, uint16_t u16ChipType, uint16_t u16Revision, 
                              uint16_t u16BlockInterval, teFlags eFlags)
{
    tsONDDownloadThreadInfo *psDownloadThreadInfo;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s\n", __FUNCTION__);
    
    eLockLock(&sONDDownloadThreadListLock);
    
    if (psONDDownloadThread_Find(sCoordinatorAddress, u32DeviceID, u16ChipType, u16Revision) != NULL)
    {
        char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
        inet_ntop(AF_INET6, &sCoordinatorAddress, buffer, INET6_ADDRSTRLEN);
        daemon_log(LOG_INFO, "OND: Download already running for coordinator %s, Device ID 0x%08x, ChipType 0x%04x, Revision 0x%04x\n",
                    buffer, u32DeviceID, u16ChipType, u16Revision);
        
        eLockUnlock(&sONDDownloadThreadListLock);
        return OND_STATUS_ERROR;
    }

    psDownloadThreadInfo = malloc(sizeof(tsONDDownloadThreadInfo));
    if (!psDownloadThreadInfo)
    {
        daemon_log(LOG_CRIT, "OND: Out of memory");
        return OND_STATUS_ERROR;
    }
    
    DBG_vPrintf(DBG_OND, "Allocated new Thread info at %p\n", psDownloadThreadInfo);
    
    memset(psDownloadThreadInfo, 0, sizeof(tsONDDownloadThreadInfo));
    
    psDownloadThreadInfo->sThreadInfo.pvThreadData = psDownloadThreadInfo;
    
    psDownloadThreadInfo->psOND = psOND;
    psDownloadThreadInfo->eFlags = eFlags;
    psDownloadThreadInfo->sCoordinatorAddress = sCoordinatorAddress;
    psDownloadThreadInfo->sFirmwareID.u32DeviceID = u32DeviceID;
    psDownloadThreadInfo->sFirmwareID.u16ChipType = u16ChipType;
    psDownloadThreadInfo->sFirmwareID.u16Revision = u16Revision;
    psDownloadThreadInfo->u16BlockInterval        = u16BlockInterval;
    eLockCreate(&psDownloadThreadInfo->sStopSignal);
    
    /* Lock mutex to main thread. Will be released to stop the thread. */
    eLockLock(&psDownloadThreadInfo->sStopSignal);
    
    DBG_vPrintf(DBG_OND, "Starting thread\n");
    
    if (eThreadStart(ONDDownloadThread, &psDownloadThreadInfo->sThreadInfo) != E_THREAD_OK)
    {
        daemon_log(LOG_CRIT, "OND: Failed to start download thread");
        eLockUnlock(&sONDDownloadThreadListLock);
        return OND_STATUS_ERROR;
    }
    
    if (psONDDownloadThread_ListHead == NULL)
    {
        /* First in list */
        psONDDownloadThread_ListHead = psDownloadThreadInfo;
    }
    else
    {
        tsONDDownloadThreadInfo *psListPosition = psONDDownloadThread_ListHead;
        while (psListPosition->psNext)
        {
            psListPosition = psListPosition->psNext;
        }
        psListPosition->psNext = psDownloadThreadInfo;
    }

    eLockUnlock(&sONDDownloadThreadListLock);

    return OND_STATUS_OK;
}


teONDStatus eONDCancelDownload(tsOND *psOND, struct in6_addr sCoordinatorAddress, uint32_t u32DeviceID, 
                               uint16_t u16ChipType, uint16_t u16Revision)
{
    tsONDDownloadThreadInfo *psDownloadThreadInfo;
    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
    inet_ntop(AF_INET6, &sCoordinatorAddress, buffer, INET6_ADDRSTRLEN);
    
    eLockLock(&sONDDownloadThreadListLock);
    
    psDownloadThreadInfo = psONDDownloadThread_Find(sCoordinatorAddress, u32DeviceID, u16ChipType, u16Revision);
    
    if (psDownloadThreadInfo != NULL)
    {
        daemon_log(LOG_INFO, "OND: Download still running for coordinator %s, Device ID 0x%08x, ChipType 0x%04x, Revision 0x%04x\n",
                    buffer, u32DeviceID, u16ChipType, u16Revision);
        
        /* Unlock mutex to signal the thread to stop. */
        eLockUnlock(&psDownloadThreadInfo->sStopSignal);
        
        /* Thread should have stopped by now, and the pointer we had is no longer valid. This is a bit messy. */
        psDownloadThreadInfo = NULL;
    }
    eLockUnlock(&sONDDownloadThreadListLock);
    
    daemon_log(LOG_INFO, "OND: Clearing status for coordinator %s, Device ID 0x%08x, ChipType 0x%04x, Revision 0x%04x\n",
                    buffer, u32DeviceID, u16ChipType, u16Revision);
    
    {
        tsFirmwareID sFirmwareID;
        sFirmwareID.u32DeviceID = u32DeviceID;
        sFirmwareID.u16ChipType = u16ChipType;
        sFirmwareID.u16Revision = u16Revision;
        
        /* Clear the broadcast entry, and all other entries */
        (void)StatusLogRemove(sFirmwareID, sCoordinatorAddress, 0xffffffff, 0xffffffff);
    }
    
    return OND_STATUS_OK;
}


/* Broadcast download thread */
static void *ONDDownloadThread(void *psThreadInfoVoid)
{
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsONDDownloadThreadInfo *psDownloadThreadInfo = (tsONDDownloadThreadInfo *)psThreadInfo->pvThreadData;
    char     acCoordinatorAddress[INET6_ADDRSTRLEN] = "Could not determine address\n";
    uint16_t u16BlockNumber = 0;
    uint32_t u32TotalBlocks;
    uint32_t u32Timeout = 0;
    teFWStatus eFWStatus;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s (%p)\n", __FUNCTION__, psThreadInfoVoid);
    
    inet_ntop(AF_INET6, &psDownloadThreadInfo->sCoordinatorAddress, acCoordinatorAddress, INET6_ADDRSTRLEN);

    /* Get total number of blocks in the image, if we have it loaded */
    eFWStatus = eFirmware_Get_Total_Blocks(psDownloadThreadInfo->sFirmwareID, OND_BLOCK_SIZE, &u32TotalBlocks);
    if (eFWStatus != FW_STATUS_OK)
    {
        daemon_log(LOG_ERR, "OND: No firmware revision %d for device ID 0x%08x for coordinator \"%s\"",
                   psDownloadThreadInfo->sFirmwareID.u16Revision, psDownloadThreadInfo->sFirmwareID.u32DeviceID, acCoordinatorAddress);
        goto exit;
    }

    daemon_log(LOG_INFO, "OND: Starting download of image revision %d for device ID 0x%08x to coordinator \"%s\"",
               psDownloadThreadInfo->sFirmwareID.u16Revision, psDownloadThreadInfo->sFirmwareID.u32DeviceID, 
               acCoordinatorAddress);

    {
        /* Send the initialise packet */
        tsONDPacket sONDPacket;
        
        sONDPacket.eType = OND_PACKET_INITIATE;
        sONDPacket.uPayload.sONDPacketInitiate.u32DeviceID = psDownloadThreadInfo->sFirmwareID.u32DeviceID;
        sONDPacket.uPayload.sONDPacketInitiate.u16ChipType = psDownloadThreadInfo->sFirmwareID.u16ChipType;
        sONDPacket.uPayload.sONDPacketInitiate.u16Revision = psDownloadThreadInfo->sFirmwareID.u16Revision;
        
        if (eONDNetworkSendPacket(&psDownloadThreadInfo->psOND->sONDNetwork, psDownloadThreadInfo->sCoordinatorAddress, &sONDPacket) != OND_STATUS_OK)
        {
            daemon_log(LOG_ERR, "OND: Error sending initiate packet");
            goto exit;
        }
        
    }
    
    if (psDownloadThreadInfo->eFlags & E_DOWNLOAD_FLAG_INITIATE_ONLY)
    {
        daemon_log(LOG_INFO, "OND: Initiate packet sent for image revision %d for device ID 0x%08x to coordinator \"%s\"",
                   psDownloadThreadInfo->sFirmwareID.u16Revision, psDownloadThreadInfo->sFirmwareID.u32DeviceID, acCoordinatorAddress);
        goto exit;
    }

    // Block timeout is 62500 / second. Block interval is passed in units of 10ms
    // 1 second = 62500 timeout, 100 interval.
    u32Timeout = 625 * psDownloadThreadInfo->u16BlockInterval;
    
    if (psDownloadThreadInfo->eFlags & E_DOWNLOAD_FLAG_IMMEDIATE_RESET)
    {
        /* Set the auto-reset flag */
        u32Timeout |= 0x80000000;
    }
    else
    {
        /* Clear the auto-reset flag */
        u32Timeout &= ~0x80000000;
    }
    
    if (eFirmware_Set_Timeout(psDownloadThreadInfo->sFirmwareID, u32Timeout) != FW_STATUS_OK)
    {
        /* Store the potentially modified timeout value back to the firmware record */
        daemon_log(LOG_ERR, "OND: Error setting firmware timeout value");
        goto exit;
    }
    
    /* Now the main loop broadcasting each block in turn */
    for (u16BlockNumber = 0; 
        (u16BlockNumber < u32TotalBlocks); 
         u16BlockNumber++)
    {
        
        if (eONDSendBlock(psDownloadThreadInfo->psOND, psDownloadThreadInfo->sCoordinatorAddress, 
                          0xFFFFFFFF,
                          0xFFFFFFFF,
                          psDownloadThreadInfo->sFirmwareID, 
                          u16BlockNumber,
                          u32TotalBlocks, u32Timeout) != OND_STATUS_OK)
        {
            daemon_log(LOG_ERR, "OND: Error sending block %d/%d to coordinator \"%s\"", 
                       u16BlockNumber, u32TotalBlocks, acCoordinatorAddress);
            goto exit;
        }
        
        /* Now attempt to lock a mutex for the block interval. */
        /* Block interval is in units of 10 milliseconds */
        switch (eLockLockTimed(&psDownloadThreadInfo->sStopSignal, psDownloadThreadInfo->u16BlockInterval * 10))
        {
            case(E_LOCK_OK):
                /* We have grabbed the lock, so the main thread has let go of it to signal us to stop. */
                daemon_log(LOG_INFO, "OND: Cancelled download of image revision %d for device ID 0x%08x to coordinator \"%s\"",
                            psDownloadThreadInfo->sFirmwareID.u16Revision, psDownloadThreadInfo->sFirmwareID.u32DeviceID, 
                            acCoordinatorAddress);
                goto exit;

            case(E_LOCK_ERROR_FAILED):
            case(E_LOCK_ERROR_TIMEOUT):
            default:
                /* No signal, keep going. */
                if (verbosity >= LOG_DEBUG)
                {
                    daemon_log(LOG_DEBUG, "OND: No signal received, keep downloading.");
                }
        }

    }

    daemon_log(LOG_INFO, "OND: Finished download of image revision %d for device ID 0x%08x to coordinator \"%s\"",
               psDownloadThreadInfo->sFirmwareID.u16Revision, psDownloadThreadInfo->sFirmwareID.u32DeviceID, 
               acCoordinatorAddress);

exit:
    /* Remove from linked list */
    eLockLock(&sONDDownloadThreadListLock);
    
    if (psONDDownloadThread_ListHead == psDownloadThreadInfo)
    {
        /* First in list */
        psONDDownloadThread_ListHead = psDownloadThreadInfo->psNext;
    }
    else
    {
        tsONDDownloadThreadInfo *psListPosition = psONDDownloadThread_ListHead;
        while (psListPosition->psNext)
        {
            if (psListPosition->psNext == psDownloadThreadInfo)
            {
                psListPosition->psNext = psListPosition->psNext->psNext;
                break;
            }
            psListPosition = psListPosition->psNext;
        }
    }

    eLockUnlock(&sONDDownloadThreadListLock);

    if (verbosity >= LOG_DEBUG)
    {
        daemon_log(LOG_DEBUG, "OND: Download thread exit");
    }
    
    /* Clean up thread's storage */
    eLockDestroy(&psDownloadThreadInfo->sStopSignal);
    free(psDownloadThreadInfo);
    eThreadFinish(ONDDownloadThread, psThreadInfo);
    return NULL;
}


/* Send a block of firmware data */
static teONDStatus eONDSendBlock(tsOND *psOND, struct in6_addr sCoordinatorAddress, 
                                 uint32_t u32AddressH, uint32_t u32AddressL, 
                                 tsFirmwareID sFirmwareID, 
                                 uint16_t u16BlockNumber, uint16_t u16TotalBlocks, uint32_t u32Timeout)
{
    tsONDPacket sONDPacket;
    teFWStatus eFWStatus;

    sONDPacket.eType = OND_PACKET_BLOCK_DATA;

    /* Broadcast */
    sONDPacket.uPayload.sONDPacketBlockData.u32AddressH = u32AddressH;
    sONDPacket.uPayload.sONDPacketBlockData.u32AddressL = u32AddressL;

    sONDPacket.uPayload.sONDPacketBlockData.u32DeviceID = sFirmwareID.u32DeviceID;
    sONDPacket.uPayload.sONDPacketBlockData.u16ChipType = sFirmwareID.u16ChipType;
    sONDPacket.uPayload.sONDPacketBlockData.u16Revision = sFirmwareID.u16Revision;
    
    sONDPacket.uPayload.sONDPacketBlockData.u16BlockNumber = u16BlockNumber;
    sONDPacket.uPayload.sONDPacketBlockData.u16TotalBlocks = u16TotalBlocks;
    
    sONDPacket.uPayload.sONDPacketBlockData.u32Timeout = u32Timeout;
    sONDPacket.uPayload.sONDPacketBlockData.u16Length = OND_BLOCK_SIZE;
    
    eFWStatus = eFirmware_Get_Block(sFirmwareID, u16BlockNumber, OND_BLOCK_SIZE, 
                                    sONDPacket.uPayload.sONDPacketBlockData.au8Data);
    
    if (eFWStatus != FW_STATUS_OK)
    {
        char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
        inet_ntop(AF_INET6, &sCoordinatorAddress, buffer, INET6_ADDRSTRLEN);
        daemon_log(LOG_ERR, "OND: Could not get block %d of revision %d for device ID 0x%08x for coordinator \"%s\"",
                    u16BlockNumber, sFirmwareID.u16Revision, sFirmwareID.u32DeviceID, buffer);
        return OND_STATUS_ERROR;
    }
    
    if (eONDNetworkSendPacket(&psOND->sONDNetwork, sCoordinatorAddress, &sONDPacket) != OND_STATUS_OK)
    {
        daemon_log(LOG_ERR, "OND: Error sending block data packet");
        return OND_STATUS_ERROR;
    }
    
    if (verbosity >= LOG_DEBUG)
    {
        char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
        inet_ntop(AF_INET6, &sCoordinatorAddress, buffer, INET6_ADDRSTRLEN);
        daemon_log(LOG_DEBUG, "OND: Sent Block %d/%d of revision %d for device ID 0x%08x to coordinator \"%s\"",
                    u16BlockNumber, u16TotalBlocks, 
                    sFirmwareID.u16Revision, sFirmwareID.u32DeviceID, buffer);
    }
    
    StatusLogRemainingBlocks(sFirmwareID, sCoordinatorAddress, u32AddressH, u32AddressL, (u16TotalBlocks - 1) - u16BlockNumber, u16TotalBlocks);

    return OND_STATUS_OK;
}


/* Broadcast reset thread */
static void *ONDResetThread(void *psThreadInfoVoid)
{
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsONDResetThreadInfo *psResetThreadInfo = (tsONDResetThreadInfo *)psThreadInfo->pvThreadData;
    char     acCoordinatorAddress[INET6_ADDRSTRLEN] = "Could not determine address\n";
    tsONDPacket sONDPacket;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s (%p)\n", __FUNCTION__, psThreadInfoVoid);
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "Reset %d times, %dms apart.\n", psResetThreadInfo->u16RepeatCount, psResetThreadInfo->u16RepeatTime * 10);
    
    inet_ntop(AF_INET6, &psResetThreadInfo->sCoordinatorAddress, acCoordinatorAddress, INET6_ADDRSTRLEN);

    sONDPacket.eType = OND_PACKET_RESET;

    /* Reset request */
    sONDPacket.uPayload.sONDPacketReset.u32DeviceID         = psResetThreadInfo->u32DeviceID;
    sONDPacket.uPayload.sONDPacketReset.u16DepthInfluence   = psResetThreadInfo->u16DepthInfluence;
  
    do
    {
        sONDPacket.uPayload.sONDPacketReset.u16Timeout      = psResetThreadInfo->u16Timeout;
    
        if (eONDNetworkSendPacket(&psResetThreadInfo->psOND->sONDNetwork, psResetThreadInfo->sCoordinatorAddress, &sONDPacket) != OND_STATUS_OK)
        {
            daemon_log(LOG_ERR, "OND: Error sending reset request packet");
            goto exit;
        }
        
        if (verbosity > 5)
        {
            daemon_log(LOG_INFO, "OND: Sent Reset request for device ID 0x%08x in %.2f seconds (depth influence=%d) to coordinator \"%s\"",
                        psResetThreadInfo->u32DeviceID, (float)psResetThreadInfo->u16Timeout / 100.0, 
                       psResetThreadInfo->u16DepthInfluence, acCoordinatorAddress);
        }
        
        if (psResetThreadInfo->u16RepeatCount) 
        {
            /* Decrement repeat count if we are going to resend. */
            psResetThreadInfo->u16RepeatCount--;

            if (psResetThreadInfo->u16Timeout > psResetThreadInfo->u16RepeatTime)
            {
                /* Subtract repeat time from timeout */
                psResetThreadInfo->u16Timeout -= psResetThreadInfo->u16RepeatTime;
            }
            
            /* Sleep for the repeat time */
            usleep(psResetThreadInfo->u16RepeatTime * 10000); /* Convert 10ms units to microseconds */
        }
    } while (psResetThreadInfo->u16RepeatCount > 0);

exit:
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s (%p) exit\n", __FUNCTION__, psThreadInfoVoid);
    
    /* Clean up thread's storage */
    free(psResetThreadInfo);
    
    return NULL;
}


/* Start a thread to send reset request packets */
teONDStatus eONDRequestReset(tsOND *psOND, struct in6_addr sCoordinatorAddress, uint32_t u32DeviceID, 
                             uint16_t u16Timeout, uint16_t u16DepthInfluence,
                             uint16_t u16RepeatCount, uint16_t u16RepeatTime)
{
    tsONDResetThreadInfo *psResetThreadInfo;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s\n", __FUNCTION__);
    
    psResetThreadInfo = malloc(sizeof(tsONDResetThreadInfo));
    if (!psResetThreadInfo)
    {
        daemon_log(LOG_CRIT, "OND: Out of memory");
        return OND_STATUS_ERROR;
    }
    
    DBG_vPrintf(DBG_OND, "Allocated new Thread info at %p\n", psResetThreadInfo);
    
    memset(psResetThreadInfo, 0, sizeof(tsONDResetThreadInfo));
    
    psResetThreadInfo->sThreadInfo.pvThreadData = psResetThreadInfo;
    
    psResetThreadInfo->psOND                    = psOND;
    psResetThreadInfo->sCoordinatorAddress      = sCoordinatorAddress;
    psResetThreadInfo->u32DeviceID              = u32DeviceID;
    psResetThreadInfo->u16Timeout               = u16Timeout;
    psResetThreadInfo->u16DepthInfluence        = u16DepthInfluence;
    psResetThreadInfo->u16RepeatCount           = u16RepeatCount;
    psResetThreadInfo->u16RepeatTime            = u16RepeatTime;
    
    DBG_vPrintf(DBG_OND, "Starting thread\n");
    
    if (eThreadStart(ONDResetThread, &psResetThreadInfo->sThreadInfo) != E_THREAD_OK)
    {
        daemon_log(LOG_CRIT, "OND: Failed to start reset thread");
        return OND_STATUS_ERROR;
    }
    
    return OND_STATUS_OK;
}


/* Thread to send reset request packets */
static tsONDDownloadThreadInfo *psONDDownloadThread_Find(struct in6_addr sCoordinatorAddress, uint32_t u32DeviceID, 
                              uint16_t u16ChipType, uint16_t u16Revision)
{
    tsONDDownloadThreadInfo *psDownloadThreadInfo = psONDDownloadThread_ListHead;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s\n", __FUNCTION__);
    
    while (psDownloadThreadInfo)
    {
        DBG_vPrintf(DBG_OND, "Looking at entry %p\n", psDownloadThreadInfo);
        
        if ((memcmp(&psDownloadThreadInfo->sCoordinatorAddress, &sCoordinatorAddress, sizeof(struct in6_addr)) == 0) &&
            (psDownloadThreadInfo->sFirmwareID.u32DeviceID == u32DeviceID) &&
            (psDownloadThreadInfo->sFirmwareID.u16ChipType == u16ChipType) &&
            (psDownloadThreadInfo->sFirmwareID.u16Revision == u16Revision))
        {
            if (verbosity >= LOG_DEBUG)
            {
                char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
                inet_ntop(AF_INET6, &sCoordinatorAddress, buffer, INET6_ADDRSTRLEN);
                daemon_log(LOG_DEBUG, "OND: Download thread found for coordinator %s, Device ID 0x%08x, ChipType 0x%04x, Revision 0x%04x\n",
                        buffer, u32DeviceID, u16ChipType, u16Revision);
            }
            /* Break and return current value */
            break;
        }
        /* Next in list */
        psDownloadThreadInfo = psDownloadThreadInfo->psNext;
    }
    
    return psDownloadThreadInfo;
}

