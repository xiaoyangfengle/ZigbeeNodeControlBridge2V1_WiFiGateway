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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <libgen.h>

#include <libdaemon/daemon.h>

#include "Trace.h"
#include "Threads.h"
#include "Firmwares.h"
#include "../Common/FWDistribution_IPC.h"

#define DBG_FUNCTION_CALLS 0
#define DBG_FIRMWARE 0


/** Number of bytes at start of firmware file to skip when downloading */
#define FIRMWARE_BIN_FILE_OFFSET 4


/** Linked list structure to hold a loaded available firmware  */
typedef struct _tsFirmware
{
    tsFirmwareID            sFirmwareID;        /** < Unique ID */

    const char              *pcFilePath;        /** < Full file path */
    const char              *pcFileName;        /** < Filename */

    uint32_t                u32Size;            /** < Size */
    
    uint32_t                u32Timeout;         /** < Timeout value to use in distribution */
    
    uint8_t                 *pu8Data;           /** < Data contents */
    
    struct _tsFirmware *psNext;                 /** < Pointer to next in linked list */
} tsFirmware;


/** Timeout value associated with each block of data downloaded */
static uint32_t u32BlockTimeout = 62500;

/** Static linked list of loaded firmwares */
static tsFirmware *psFirmwares = NULL;

/** Thread protection for firmwares linked list */
static tsLock sFirmwaresLock;

/** Add firmware to available linked list 
 *  Allocates space for the firmware in the list and copies the structure passed
 *  \param sNewFirmware         Structure representing new firmware to add
 *  \return FW_STATUS_OK if successfully added to the list
 */
static teFWStatus Firmware_List_Add    (tsFirmware sNewFirmware);

/** Remove firmware from the list
 *  Removes firmware from the list and free's its storage. 
 *  After this call psOldFirmware will be an invalid pointer.
 *  \param psOldFirmware        Pointer to old firmware to remove
 *  \return FW_STATUS_OK if successfully removed from the list
 */
static teFWStatus Firmware_List_Remove (tsFirmware *psOldFirmware);

/** Lookup a firmware from the list
 *  \param sFirmwareID          Unique ID of the firmware image
 *  \return Pointer to firmware, or NULL if no firmware matches that required
 */
static tsFirmware *Firmware_List_Get    (tsFirmwareID sFirmwareID);


teFWStatus eFirmware_Init(void)
{
    psFirmwares = NULL;
    
    if (eLockCreate(&sFirmwaresLock) != E_LOCK_OK)
    {
        daemon_log(LOG_DEBUG, "Firmware: Failed to initialise lock");
        return FW_STATUS_ERROR;
    }

    return FW_STATUS_OK;
}


teFWStatus eFirmware_Open(const char *pu8Filename)
{
    tsFirmware sNewFirmware;
    teFWStatus eStatus;
    int fd;
    struct stat FirmwareStat;

    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s(%s)\n", __FUNCTION__, pu8Filename);
    
    memset(&sNewFirmware, 0, sizeof(tsFirmware));
    
    fd = open(pu8Filename, 0);

    if (fd < 0)
    {
        daemon_log(LOG_ERR, "Firmware: Error opening file: %s (%s)", pu8Filename, strerror(errno));
        return FW_STATUS_BAD_FILE;
    }
    
    /* Close any existing copies */
    eFirmware_Close(pu8Filename);

    fstat(fd, &FirmwareStat);
    sNewFirmware.u32Size = FirmwareStat.st_size;

    sNewFirmware.pu8Data = mmap(NULL, sNewFirmware.u32Size, PROT_READ, MAP_PRIVATE, fd, 0);
    
    if (sNewFirmware.pu8Data == MAP_FAILED)
    {
        daemon_log(LOG_ERR, "Firmware: Error memory mapping file: %s (%s)", pu8Filename, strerror(errno));
        return FW_STATUS_BAD_FILE;
    }
    
    /* Now the file is mmap'ed we don't need the file descriptor anymore */
    close(fd);

#if 1
    {
        /* Magic number check */
        uint32_t u32Temp[3];
        
        u32Temp[0] = ntohl(*(uint32_t *)&sNewFirmware.pu8Data[0x04]);
        u32Temp[1] = ntohl(*(uint32_t *)&sNewFirmware.pu8Data[0x08]);
        u32Temp[2] = ntohl(*(uint32_t *)&sNewFirmware.pu8Data[0x0c]);
        
        if ((u32Temp[0] != 0x12345678) ||
            (u32Temp[1] != 0x11223344) ||
            (u32Temp[2] != 0x55667788))
        {
            daemon_log(LOG_ERR, "Firmware : Firmware file \"%s\" has an invalid magic number(0x%08x 0x%08x 0x%08x) - not loading", pu8Filename, u32Temp[0], u32Temp[1], u32Temp[2]);
            munmap(sNewFirmware.pu8Data, sNewFirmware.u32Size);
            return FW_STATUS_BAD_FILE;
        }   
    }
#endif
    
    sNewFirmware.sFirmwareID.u32DeviceID = ntohl(*(uint32_t *)&sNewFirmware.pu8Data[0x14]);
    sNewFirmware.sFirmwareID.u16ChipType = ntohs(*(uint16_t *)&sNewFirmware.pu8Data[0x18]);
    sNewFirmware.sFirmwareID.u16Revision = ntohs(*(uint16_t *)&sNewFirmware.pu8Data[0x1a]);
    
#if 1
    if ((sNewFirmware.sFirmwareID.u32DeviceID == 0xFFFFFFFF) &&
        (sNewFirmware.sFirmwareID.u16Revision == 0xFFFF) &&
        (sNewFirmware.sFirmwareID.u16ChipType == 0xFFFF))
    {
        daemon_log(LOG_ERR, "Firmware : Firmware file \"%s\" doesn't look like an update image - not loading", pu8Filename);
        munmap(sNewFirmware.pu8Data, sNewFirmware.u32Size);
        return FW_STATUS_BAD_FILE;;
    }   
#endif

    sNewFirmware.u32Timeout = u32BlockTimeout;

    sNewFirmware.pcFilePath = strdup(pu8Filename);
    {
        char *pcTemp = strdup(pu8Filename);
        sNewFirmware.pcFileName = strdup(basename(pcTemp));
        free(pcTemp);
    }
    
    daemon_log(LOG_INFO, "Firmware : Loaded firmware file     : %s",     sNewFirmware.pcFileName);
    daemon_log(LOG_INFO, "Firmware : Loaded firmware device id: 0x%08x", sNewFirmware.sFirmwareID.u32DeviceID);
    daemon_log(LOG_INFO, "Firmware : Loaded firmware chip type: 0x%04x", sNewFirmware.sFirmwareID.u16ChipType);
    daemon_log(LOG_INFO, "Firmware : Loaded firmware revision : 0x%04x", sNewFirmware.sFirmwareID.u16Revision);
    daemon_log(LOG_INFO, "Firmware : Timeout set to           : %d",     sNewFirmware.u32Timeout);
    
    /* Add it to the list if possible */
    eStatus = Firmware_List_Add(sNewFirmware);
    if (eStatus == FW_STATUS_OK)
    {
        return FW_STATUS_OK;
    }
    else
    {
        munmap(sNewFirmware.pu8Data, sNewFirmware.u32Size);
        return eStatus;
    }
}


teFWStatus eFirmware_Close(const char *pu8Filename)
{
    teFWStatus eStatus = FW_STATUS_NO_FIRMWARE;
    tsFirmware *psFirmware;
    
    eLockLock(&sFirmwaresLock);
    
    psFirmware = psFirmwares;
    while (psFirmware)
    {
        if (strcmp(psFirmware->pcFilePath, pu8Filename) == 0)
        {
            eStatus = FW_STATUS_OK;
            break;
        }
        psFirmware = psFirmware->psNext;
    }
    
    eLockUnlock(&sFirmwaresLock);
    
    if (eStatus == FW_STATUS_OK)
    {
        daemon_log(LOG_INFO, "Firmware : Unloaded firmware file: %s",     psFirmware->pcFileName);
        eStatus = Firmware_List_Remove (psFirmware);
    }
    
    return eStatus;
}


teFWStatus eFirmware_Get_Total_Blocks(tsFirmwareID sFirmwareID, uint32_t u32BlockSize, uint32_t *pu32TotalBlocks)
{
    teFWStatus eStatus = FW_STATUS_ERROR;
    tsFirmware *psFirmware;

    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s(dev ID 0x%08x chip type 0x%04x revision 0x%04x, %d, %p)\n", 
            __FUNCTION__, sFirmwareID.u32DeviceID, sFirmwareID.u16ChipType, sFirmwareID.u16Revision, u32BlockSize, pu32TotalBlocks);
    
    eLockLock(&sFirmwaresLock);
    
    psFirmware = Firmware_List_Get(sFirmwareID);
    
    if (psFirmware)
    {
        *pu32TotalBlocks = (psFirmware->u32Size / u32BlockSize);
        
        DBG_vPrintf(DBG_FIRMWARE, "Firmware: Firmware size %d has %d blocks of size %d\n", 
                    psFirmware->u32Size, *pu32TotalBlocks, u32BlockSize);

        eStatus = FW_STATUS_OK;
    }
    else
    {
        eStatus = FW_STATUS_NO_FIRMWARE;
    }
    
    eLockUnlock(&sFirmwaresLock);
    return eStatus;
}


teFWStatus eFirmware_Get_Block(tsFirmwareID sFirmwareID, uint32_t u32BlockNumber, uint32_t u32BlockSize, uint8_t *pu8Data)
{
    teFWStatus eStatus = FW_STATUS_ERROR;
    tsFirmware *psFirmware;
    uint8_t *pu8Block;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s(dev ID 0x%08x chip type 0x%04x revision 0x%04x, %d, %d, %p))\n", 
                __FUNCTION__, sFirmwareID.u32DeviceID, sFirmwareID.u16ChipType, sFirmwareID.u16Revision, u32BlockNumber, u32BlockSize, pu8Data);
 
    eLockLock(&sFirmwaresLock);
    
    psFirmware = Firmware_List_Get(sFirmwareID);
    
    if (psFirmware)
    {
        uint32_t u32Offset;
        
        u32Offset = FIRMWARE_BIN_FILE_OFFSET + (u32BlockNumber * u32BlockSize);
        
        DBG_vPrintf(DBG_FIRMWARE, "Firmware: Get block %d from offset %d\n", u32BlockNumber, u32Offset);
        
        pu8Block = &psFirmware->pu8Data[u32Offset];
        
        memcpy(pu8Data, pu8Block, u32BlockSize);
        
        eStatus = FW_STATUS_OK;
    }
    else
    {
        eStatus = FW_STATUS_NO_FIRMWARE;
    }
    
    eLockUnlock(&sFirmwaresLock);
    return eStatus;
}


teFWStatus eFirmware_Set_Timeout(tsFirmwareID sFirmwareID, uint32_t u32Timeout)
{
    teFWStatus eStatus = FW_STATUS_ERROR;
    tsFirmware *psFirmware;

    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s(dev ID 0x%08x chip type 0x%04x revision 0x%04x, %d))\n", 
                __FUNCTION__, sFirmwareID.u32DeviceID, sFirmwareID.u16ChipType, sFirmwareID.u16Revision, u32Timeout);
 
    eLockLock(&sFirmwaresLock);
    
    psFirmware = Firmware_List_Get(sFirmwareID);
    
    if (psFirmware)
    {
        daemon_log(LOG_INFO, "Firmware : Timeout set to: %u",     u32Timeout);
        
        psFirmware->u32Timeout = u32Timeout;
        eStatus = FW_STATUS_OK;
    }
    else
    {
        eStatus = FW_STATUS_NO_FIRMWARE;
    }
    
    eLockUnlock(&sFirmwaresLock);
    return eStatus;
}


teFWStatus eFirmware_Get_Timeout(tsFirmwareID sFirmwareID, uint32_t *pu32Timeout)
{
    teFWStatus eStatus = FW_STATUS_ERROR;
    tsFirmware *psFirmware;

    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s(dev ID 0x%08x chip type 0x%04x revision 0x%04x, %d))\n", 
                __FUNCTION__, sFirmwareID.u32DeviceID, sFirmwareID.u16ChipType, sFirmwareID.u16Revision, *pu32Timeout);
 
    eLockLock(&sFirmwaresLock);
    
    psFirmware = Firmware_List_Get(sFirmwareID);
    
    if (psFirmware)
    {
        if (pu32Timeout)
        {
            DBG_vPrintf(DBG_FIRMWARE, "Firmware: Timeout is %u\n", psFirmware->u32Timeout);
        
            *pu32Timeout = psFirmware->u32Timeout;
            eStatus = FW_STATUS_OK;
        }
        else
        {
            eStatus = FW_STATUS_ERROR;
        }
    }
    else
    {
        eStatus = FW_STATUS_NO_FIRMWARE;
    }
    
    eLockUnlock(&sFirmwaresLock);
    return eStatus;
}


teFWStatus eFirmware_Remote_Send(int iSocket)
{
    tsFWDistributionIPCHeader sFWDistributionIPCHeader;
    tsFWDistributionIPCAvailableFirmwareRecord sFirmwareRecord;
    tsFirmware *psFirmware;
    
    sFWDistributionIPCHeader.eType = IPC_AVAILABLE_FIRMWARES;
    sFWDistributionIPCHeader.u32PayloadLength = 0;
    
    eLockLock(&sFirmwaresLock);
    
    psFirmware = psFirmwares;
    
    /* Determine how many firmwares are available and therefore how big the payload will be */
    psFirmware = psFirmwares;
    while (psFirmware)
    {
        sFWDistributionIPCHeader.u32PayloadLength += sizeof(tsFWDistributionIPCAvailableFirmwareRecord);
        psFirmware = psFirmware->psNext;
    }
    
    /* Send the header */
    if (send(iSocket, &sFWDistributionIPCHeader, sizeof(tsFWDistributionIPCHeader), 0) < 0) {
        perror("send");
        eLockUnlock(&sFirmwaresLock);
        return FW_STATUS_REMOTE_ERROR;
    }
    
    /* Send each firmware*/
    psFirmware = psFirmwares;
    while (psFirmware)
    {
        sFirmwareRecord.u32DeviceID = psFirmware->sFirmwareID.u32DeviceID;
        sFirmwareRecord.u16ChipType = psFirmware->sFirmwareID.u16ChipType;
        sFirmwareRecord.u16Revision = psFirmware->sFirmwareID.u16Revision;

        strncpy(sFirmwareRecord.acFilename, psFirmware->pcFileName, 255);
        
        /* Send the record */
        if (send(iSocket, &sFirmwareRecord, sizeof(tsFWDistributionIPCAvailableFirmwareRecord), 0) < 0) {
            perror("send");
            eLockUnlock(&sFirmwaresLock);
            return FW_STATUS_REMOTE_ERROR;
        }

        psFirmware = psFirmware->psNext;
    }
    
    eLockUnlock(&sFirmwaresLock);
    return FW_STATUS_OK;
}


/***** Internal functions ******/



static teFWStatus Firmware_List_Add(tsFirmware sNewFirmware)
{
    tsFirmware *psNewFirmware;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s\n", __FUNCTION__);
    
    psNewFirmware = malloc(sizeof(tsFirmware));
    
    if (psNewFirmware == NULL)
    {
        daemon_log(LOG_CRIT, "Firmware: Out of memory adding firmware");
        return FW_STATUS_NO_MEM;
    }
    
    memset(psNewFirmware, 0, sizeof(tsFirmware));
    
    /* Copy data to newly allocated structure */
    *psNewFirmware = sNewFirmware;
    
    psNewFirmware->psNext = NULL;
    
    eLockLock(&sFirmwaresLock);
    
    if (psFirmwares == NULL)
    {
        /* First in list */
        DBG_vPrintf(DBG_FIRMWARE, "Firmware: Adding at head of list\n");
        psFirmwares = psNewFirmware;
    }
    else
    {
        tsFirmware *psListPosition = psFirmwares;
        while (psListPosition->psNext)
        {
            DBG_vPrintf(DBG_FIRMWARE, "Firmware: Skipping %p\n", psListPosition);
            psListPosition = psListPosition->psNext;
        }
        DBG_vPrintf(DBG_FIRMWARE, "Firmware: Adding at %p\n", psListPosition);
        psListPosition->psNext = psNewFirmware;
    }
    
    eLockUnlock(&sFirmwaresLock);
    
    return FW_STATUS_OK;
}


static teFWStatus Firmware_List_Remove (tsFirmware *psOldFirmware)
{
    teFWStatus eStatus = FW_STATUS_ERROR;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s(%p)\n", __FUNCTION__, psOldFirmware);
    
    eLockLock(&sFirmwaresLock);
    
    if (psFirmwares == psOldFirmware)
    {
        /* First in list */
        DBG_vPrintf(DBG_FIRMWARE, "Firmware: Removing from head of list\n");
        psFirmwares = psFirmwares->psNext;
        
        /* Free it's storage */
        munmap(psOldFirmware->pu8Data, psOldFirmware->u32Size);
        free(psOldFirmware);
        eStatus = FW_STATUS_OK;
    }
    else
    {
        tsFirmware *psListPosition = psFirmwares;
        while (psListPosition->psNext)
        {
            if (psListPosition->psNext == psOldFirmware)
            {
                DBG_vPrintf(DBG_FIRMWARE, "Firmware: Removing from %p\n", psListPosition);
                psListPosition->psNext = psListPosition->psNext->psNext;
                
                /* Free it's storage */
                munmap(psOldFirmware->pu8Data, psOldFirmware->u32Size);
                free(psOldFirmware);
                eStatus = FW_STATUS_OK;
                break;
            }
            psListPosition = psListPosition->psNext;
        }
    }
    
    eLockUnlock(&sFirmwaresLock);
    
    return eStatus;
}


static tsFirmware *Firmware_List_Get    (tsFirmwareID sFirmwareID)
{
    tsFirmware *psListPosition = psFirmwares;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s(dev ID 0x%08x chip type 0x%04x revision 0x%04x)\n", 
                __FUNCTION__, sFirmwareID.u32DeviceID, sFirmwareID.u16ChipType, sFirmwareID.u16Revision);
    
    while (psListPosition)
    {
        if (memcmp(&psListPosition->sFirmwareID, &sFirmwareID, sizeof(tsFirmwareID)) == 0)
        {
            DBG_vPrintf(DBG_FIRMWARE, "Firmware: Matching device ID at %p\n", psListPosition);
            
            return psListPosition;
        }
        psListPosition = psListPosition->psNext;
    }
    
    return NULL;
}



