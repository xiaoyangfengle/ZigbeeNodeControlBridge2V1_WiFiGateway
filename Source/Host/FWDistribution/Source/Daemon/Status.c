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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <stdint.h>

#include <libdaemon/daemon.h>

#include "Status.h"
#include "Trace.h"
#include "Threads.h"

#include "../Common/FWDistribution_IPC.h"

#define DBG_FUNCTION_CALLS 0
#define DBG_STATUS 0


/** Structure to uniquely describe a client */
typedef struct
{
    tsFirmwareID sFirmwareID;       /** < The ID of the firmware */
    struct in6_addr sAddress;       /** < IPv6 address of network entry point */
    uint32_t u32AddressH;           /** < High word of Node mac address (0xffffffff for broadcast) */
    uint32_t u32AddressL;           /** < Low word of Node mac address (0xffffffff for broadcast) */ 
} tsClientID;

/** Structure for linked list of status entries */
typedef struct _tsStatusEntry
{
    tsClientID sClientID;           /** < ID of client */
    
    uint32_t u32RemainingBlocks;    /** < Number of blocks the node still needs */
    uint32_t u32TotalBlocks;        /** < Total number of blocks in the image */
    
    uint32_t u32Status;             /** < Download status */
    struct _tsStatusEntry *psNext;  /** < Linked list next pointer */
} tsStatusEntry;


/** Head of linked list */
tsStatusEntry *psStatusEntries;

/** Thread protection for status entries linked list */
static tsLock sStatusListLock;


/** Add new entry to linked list
 *  \param sStatusEntry             Description of entry. Copied into newly allocated linked list structure
 *  \return STATUS_OK on success
 */
static teStatus Status_List_Add(tsStatusEntry sStatusEntry);


/** Remove an entry from linked list
 *  \param psOldStatus              Pointer to structure to remove
 *  \return STATUS_OK on success
 */
static teStatus Status_List_Remove (tsStatusEntry *psOldStatus);


/** Get pointer to entry matching client ID
 *  \param sClientID                ID of client
 *  \return Pointer to existing entry if it exists, NULL otherwise
 */
static tsStatusEntry *Status_List_Get    (tsClientID sClientID);


teStatus StatusInit(void)
{
    /* Initialise list */
    if (eLockCreate(&sStatusListLock) != E_LOCK_OK)
    {
        daemon_log(LOG_CRIT, "Status: Failed to initialise lock");
        return STATUS_ERROR;
    }
    psStatusEntries = NULL;
    
    return STATUS_OK;
}


/* Add a log entry */
teStatus StatusLogRemainingBlocks(tsFirmwareID sFirmwareID, struct in6_addr sAddress, uint32_t u32AddressH, uint32_t u32AddressL, uint32_t u32RemainingBlocks, uint32_t u32TotalBlocks)
{
    tsStatusEntry sStatusEntry, *psStatusEntry;
    
    sStatusEntry.sClientID.sFirmwareID  = sFirmwareID;
    sStatusEntry.sClientID.sAddress     = sAddress;
    sStatusEntry.sClientID.u32AddressH  = u32AddressH;
    sStatusEntry.sClientID.u32AddressL  = u32AddressL;

    eLockLock(&sStatusListLock);
    
    psStatusEntry = Status_List_Get (sStatusEntry.sClientID);
    
    if (!psStatusEntry)
    {
        if (Status_List_Add(sStatusEntry) != STATUS_OK)
        {
            eLockUnlock(&sStatusListLock);
            return STATUS_ERROR;
        }
        
        psStatusEntry = Status_List_Get (sStatusEntry.sClientID);
        
        psStatusEntry->u32Status = 0;
        
        DBG_vPrintf(DBG_STATUS, "Status: Add new status entry: %p\n", psStatusEntry);
    }
    
    psStatusEntry->u32RemainingBlocks = u32RemainingBlocks;
    psStatusEntry->u32TotalBlocks = u32TotalBlocks;

    DBG_vPrintf(DBG_STATUS, "Status: Update %p: %d/%d: %d\n", psStatusEntry, psStatusEntry->u32TotalBlocks - psStatusEntry->u32RemainingBlocks, psStatusEntry->u32TotalBlocks, psStatusEntry->u32Status);
    
    eLockUnlock(&sStatusListLock);
    return STATUS_OK;
}


/* Clear log entries */
teStatus StatusLogRemove(tsFirmwareID sFirmwareID, struct in6_addr sAddress, 
                         uint32_t u32AddressH, uint32_t u32AddressL)
{
    tsStatusEntry *psStatusEntry;
    tsClientID sClientID;
    
    sClientID.sFirmwareID  = sFirmwareID;
    sClientID.sAddress     = sAddress;
    sClientID.u32AddressH  = u32AddressH;
    sClientID.u32AddressL  = u32AddressL;
    
    eLockLock(&sStatusListLock);
   
    if ((u32AddressH == 0xffffffff) && (u32AddressL == 0xffffffff))
    {
        /* Broadcast - remove any matching entry */
start:        
        psStatusEntry = psStatusEntries;

        while(psStatusEntry)
        {
            if ((memcmp(&psStatusEntry->sClientID.sFirmwareID, &sClientID.sFirmwareID, sizeof(tsFirmwareID)) == 0) &&
                (memcmp(&psStatusEntry->sClientID.sAddress, &sClientID.sAddress, sizeof(struct in6_addr)) == 0))
            {
                DBG_vPrintf(DBG_STATUS, "Status: Got entry to remove at: %p\n", psStatusEntry);
                if (Status_List_Remove(psStatusEntry) == STATUS_OK)
                {
                    DBG_vPrintf(DBG_STATUS, "Status: Entry removed: %p\n", psStatusEntry);
                    
                    /* We now need to restart the loop, as the storage for the psStatusEntry has been free'd */
                    goto start;
                }
            }
            psStatusEntry = psStatusEntry->psNext;
        }
    }
    else
    {
        psStatusEntry = Status_List_Get(sClientID);
        if (psStatusEntry)
        {
            if (Status_List_Remove(psStatusEntry) == STATUS_OK)
            {
                DBG_vPrintf(DBG_STATUS, "Status: Entry removed: %p\n", psStatusEntry);
            }
        }
    }

    eLockUnlock(&sStatusListLock);
    return STATUS_OK;
}


/* Send all log entries to the IPC client */
teStatus eStatus_Remote_Send(int iSocket)
{
    tsFWDistributionIPCHeader sFWDistributionIPCHeader;
    tsFWDistributionIPCStatusRecord sRecord;
    tsStatusEntry *psStatusEntry = psStatusEntries;
    
    sFWDistributionIPCHeader.eType = IPC_STATUS;
    sFWDistributionIPCHeader.u32PayloadLength = 0;
    
    eLockLock(&sStatusListLock);
    
    /* Determine how many firmwares are available and therefore how big the payload will be */
    psStatusEntry = psStatusEntries;
    while (psStatusEntry)
    {
        sFWDistributionIPCHeader.u32PayloadLength += sizeof(tsFWDistributionIPCStatusRecord);
        psStatusEntry = psStatusEntry->psNext;
    }
    
    /* Send the header */
    if (send(iSocket, &sFWDistributionIPCHeader, sizeof(tsFWDistributionIPCHeader), 0) < 0) {
        perror("send");
        eLockUnlock(&sStatusListLock);
        return STATUS_ERROR;
    }
    
    /* Send each status record */
    psStatusEntry = psStatusEntries;
    while (psStatusEntry)
    {
        sRecord.u32DeviceID         = psStatusEntry->sClientID.sFirmwareID.u32DeviceID;
        sRecord.u16ChipType         = psStatusEntry->sClientID.sFirmwareID.u16ChipType;
        sRecord.u16Revision         = psStatusEntry->sClientID.sFirmwareID.u16Revision;
        
        sRecord.sAddress            = psStatusEntry->sClientID.sAddress;
        sRecord.u32AddressH         = psStatusEntry->sClientID.u32AddressH;
        sRecord.u32AddressL         = psStatusEntry->sClientID.u32AddressL;
        sRecord.u32RemainingBlocks  = psStatusEntry->u32RemainingBlocks;
        sRecord.u32TotalBlocks      = psStatusEntry->u32TotalBlocks;
        sRecord.u32Status           = psStatusEntry->u32Status;
        
        /* Send the record */
        if (send(iSocket, &sRecord, sizeof(tsFWDistributionIPCStatusRecord), 0) < 0) {
            perror("send");
            eLockUnlock(&sStatusListLock);
            return FW_STATUS_REMOTE_ERROR;
        }

        psStatusEntry = psStatusEntry->psNext;
    }

    eLockUnlock(&sStatusListLock);
    return STATUS_OK;
}





static teStatus Status_List_Add(tsStatusEntry sStatusEntry)
{
    tsStatusEntry *psNewEntry;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s\n", __FUNCTION__);
    
    psNewEntry = malloc(sizeof(tsStatusEntry));
    
    if (psNewEntry == NULL)
    {
        daemon_log(LOG_CRIT, "Status: Out of memory adding firmware");
        return STATUS_NO_MEM;
    }
    
    memset(psNewEntry, 0, sizeof(tsStatusEntry));
    
    /* Copy data to newly allocated structure */
    *psNewEntry = sStatusEntry;
    
    psNewEntry->psNext = 0;
    
    if (psStatusEntries == NULL)
    {
        /* First in list */
        DBG_vPrintf(DBG_STATUS, "Status: Adding at head of list\n");
        psStatusEntries = psNewEntry;
    }
    else
    {
        tsStatusEntry *psListPosition = psStatusEntries;
        while (psListPosition->psNext)
        {
            DBG_vPrintf(DBG_STATUS, "Status: Skipping %p\n", psListPosition);
            psListPosition = psListPosition->psNext;
        }
        DBG_vPrintf(DBG_STATUS, "Status: Adding at %p\n", psListPosition);
        psListPosition->psNext = psNewEntry;
    }
    
    return FW_STATUS_OK;
}


static teStatus Status_List_Remove (tsStatusEntry *psOldStatus)
{
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s(%p)\n", __FUNCTION__, psOldStatus);
    
    if (psStatusEntries == psOldStatus)
    {
        /* First in list */
        DBG_vPrintf(DBG_STATUS, "Status: Removing from head of list\n");
        psStatusEntries = psStatusEntries->psNext;
        
        /* Free it's storage */
        free(psOldStatus);
        return STATUS_OK;
    }
    else
    {
        tsStatusEntry *psListPosition = psStatusEntries;
        while (psListPosition->psNext)
        {
            if (psListPosition->psNext == psOldStatus)
            {
                DBG_vPrintf(DBG_STATUS, "Status: Removing from %p\n", psListPosition);
                psListPosition->psNext = psListPosition->psNext->psNext;
                
                /* Free it's storage */
                free(psOldStatus);
                return STATUS_OK;
            }
            psListPosition = psListPosition->psNext;
        }
    }
    return STATUS_ERROR;
}


static tsStatusEntry *Status_List_Get    (tsClientID sClientID)
{
    tsStatusEntry *psListPosition = psStatusEntries;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s(dev ID 0x%08x chip type 0x%04x revision 0x%04x)\n", 
                __FUNCTION__, sClientID.sFirmwareID.u32DeviceID, sClientID.sFirmwareID.u16ChipType, sClientID.sFirmwareID.u16Revision);
    
    while (psListPosition)
    {
        if (memcmp(&psListPosition->sClientID, &sClientID, sizeof(tsClientID)) == 0)
        {
            DBG_vPrintf(DBG_STATUS, "Status: Matching device ID at %p\n", psListPosition);
            
            return psListPosition;
        }
        psListPosition = psListPosition->psNext;
    }
    
    return NULL;
}

