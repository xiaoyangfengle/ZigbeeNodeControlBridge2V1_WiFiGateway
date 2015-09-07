/*****************************************************************************
 *
 * MODULE:              NTAG IC communications.
 *
 * COMPONENT:           $RCSfile: NTAG.c,v $
 *
 * VERSION:             $Name:  $
 *
 * REVISION:            $Revision: 1.25 $
 *
 * DATED:               $Date: 2009/07/15 08:16:39 $
 *
 * STATUS:              $State: Exp $
 *
 * AUTHOR:              Matt Redfearn
 *
 * DESCRIPTION:
 *
 *
 * LAST MODIFIED BY:    $Author: mredfearn $
 *                      $Modtime: $
 *
 ****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on
 * each copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd 2005, 2006. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>

#include <libdaemon/daemon.h>

#include "NTAG.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

//#define NTAG_DEBUG

/** Define this to use a file for IO instead of  areal NTAG */
//#define TEST_FILE "testfile.bin"


#ifdef NTAG_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* NTAG_DEBUG */

// Size of data area in bytes.
#define NTAG_DATA_SIZE 64

#define NTAG_BLOCK_CONFIGURATION 0x7A

#define USE_SRAM                 1
#define SRAM_FIXED_START         0xF8
#define SRAM_START               3
#define SRAM_END                 ( SRAM_START + 3 )


#define NC_REG                    0 
#define LAST_NDEF_PAGE            1 
#define SM_REG                    2 
#define WDT_LS                    3 
#define WDT_MS                    4 
#define I2C_CLOCK_STR             5 
#define NS_REG                    6 
#define ZEROES                    7 

#define NS_REG_NDEF_DATA_READ     ( 1 << 7 )
#define NS_REG_I2C_LOCKED         ( 1 << 6 )
#define NS_REG_RF_LOCKED          ( 1 << 5 )
#define NS_REG_SRAM_I2C_READY     ( 1 << 4 )
#define NS_REG_SRAM_RF_READY      ( 1 << 3 )
#define NS_REG_EEPROM_WR_ERR      ( 1 << 2 )
#define NS_REG_EEPROM_WR_BUSY     ( 1 << 1 )
#define NS_REG_RF_FIELD_PRESENT   ( 1 << 0 )

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

static int iPwrFd = 0;
static int iInterruptFd = 0;
static int iI2CFd = 0;

static teNfcStatus eNtagReadSessionRegister(uint8_t u8Register, uint8_t* pu8Data);
static teNfcStatus eNtagWriteSessionRegister(uint8_t u8Register, uint8_t u8Mask, uint8_t u8Data);

static teNfcStatus eNtagPwrOff(void);
static teNfcStatus eNtagPwrOn(void);
static teNfcStatus eNtagPwrCycle(void);


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


teNfcStatus eNtagSetup(const char *pcPwrGPIO, const char *pcInterruptGPIO, const char *pcI2CBus, int iI2CAddress)
{   
#ifdef TEST_FILE
    iInterruptFd = 0;
    iI2CFd = open(TEST_FILE, O_RDWR);
    if (iI2CFd < 0)
    {
        daemon_log(LOG_ERR, "Could not open test file(%s)", strerror(errno));
        close(iInterruptFd);
        return E_NFC_ERROR;
    }
    DEBUG_PRINTF("Test file opened\n");
#else    
    if (pcPwrGPIO)
    {
        iPwrFd = open(pcPwrGPIO, O_RDWR);
        if (iPwrFd < 0)
        {
            daemon_log(LOG_ERR, "Could not open power control signal '%s' (%s)", pcInterruptGPIO, strerror(errno));
            iPwrFd = 0;
        }
    }
    
    eNtagPwrCycle();
    
    iInterruptFd = open(pcInterruptGPIO, O_RDONLY);
    if (iInterruptFd < 0)
    {
        daemon_log(LOG_ERR, "Could not open interrupt signal '%s' (%s)", pcInterruptGPIO, strerror(errno));
        return E_NFC_ERROR;
    }

    iI2CFd = open(pcI2CBus, O_RDWR | O_NOCTTY);
    if (iI2CFd < 0)
    {
        daemon_log(LOG_ERR, "Could not open I2C bus '%s' (%s)", pcI2CBus, strerror(errno));
        close(iInterruptFd);
        return E_NFC_ERROR;
    }
    
    if (ioctl(iI2CFd, I2C_SLAVE, iI2CAddress) < 0)
    {
        daemon_log(LOG_ERR, "Cannot select NTAG I2C address (%s)", strerror(errno));
        close(iInterruptFd);
        close(iI2CFd);
        return E_NFC_ERROR;
    }
#endif
    
    {
        uint8_t au8Data[16];
        
        if (eNtagReadBlock(0, au8Data) != E_NFC_OK)
        {
            return E_NFC_ERROR;
        }
        daemon_log(LOG_INFO, "Connected to NTAG serial number: 0x%02X%02X%02X%02X%02X%02X%02X\n", 
                   au8Data[0], au8Data[1], au8Data[2], au8Data[3], au8Data[4], au8Data[5], au8Data[6]);
    }
    
    {
        uint8_t au8Data[16];
        int i;
        
        if (eNtagReadBlock(NTAG_BLOCK_CONFIGURATION, au8Data) != E_NFC_OK)
        {
            return E_NFC_ERROR;
        }
        for (i = 0; i < 8; i++)
        {
            daemon_log(LOG_DEBUG, "NTAG configuration byte %01d: 0x%02X", i, au8Data[i]);
        }
        
        // Clear FD config
        au8Data[0] &= 0xC3;
        
        // FD_Off: field off or tag HALTed. FD_On: by selection of tag
        au8Data[0] |= 0x18;
        
#if USE_SRAM
        // Set SRAM mirror: 0n
        au8Data[0] |= 0x02;
        
        // Set up location of SRAM mirror to be our data location
        au8Data[2] = SRAM_START;
#else
        // Set SRAM mirror: Off
        au8Data[0] &= ~0x02;
#endif /* USE_SRAM */
        
        if (eNtagWriteBlock(NTAG_BLOCK_CONFIGURATION, au8Data) != E_NFC_OK)
        {
            return E_NFC_ERROR;
        }
    }
    
    return E_NFC_OK;
}


teNfcStatus eNtagRfUnlocked(void)
{
    teNfcStatus eStatus;
    uint8_t u8Data;
    
    if ((eStatus = eNtagReadSessionRegister(NS_REG, &u8Data)) != E_NFC_OK)
    {
        return eStatus;
    }
    
    if (u8Data & NS_REG_RF_LOCKED)
    {
        DEBUG_PRINTF("RF Locked\n");
        return E_NFC_RF_LOCKED;
    }
    
    return E_NFC_OK;    
}


teNfcStatus eNtagWait(int iTimeoutMs)
{
    struct pollfd asPollSet[1];
    int iNumFDs = 1;
    int iResult;

    if (iInterruptFd <= 0)
    {
        return E_NFC_ERROR;
    }
    
    memset((void*)asPollSet, 0, sizeof(asPollSet));

    asPollSet[0].fd     = iInterruptFd;
    asPollSet[0].events = POLLPRI;

    iResult = poll(asPollSet, iNumFDs, iTimeoutMs);
    
    if (iResult < 0)
    {
        daemon_log(LOG_DEBUG, "Failed waiting for interrupt (%s)", strerror(errno));
        return E_NFC_COMMS_FAILED;
    }
    else if (iResult == 0)
    {
        return E_NFC_TIMEOUT;
    }
    else
    {
        if (asPollSet[0].revents & POLLPRI)
        {
            // Wait 100ms for the line to debounce.
            usleep(100 * 1000);
            
            return eNtagPresent();
        }
    }

    return E_NFC_TIMEOUT;
}


teNfcStatus eNtagPresent(void)
{
    static int iTagPresent = 0;
    
    int len;
    char buf[2];

    if (iInterruptFd <= 0)
    {
        return E_NFC_ERROR;
    }

    // Seek to the start of the file
    lseek(iInterruptFd, SEEK_SET, 0);
    
    // Read the field_detect line
    len = read(iInterruptFd, buf, 2);

    if (len != 2)
    {
        return E_NFC_ERROR;
    }
    
    buf[1] = '\0';

    if (buf[0] == '1')
    {
        if (!iTagPresent)
        {
            daemon_log(LOG_INFO, "Reader present");
            iTagPresent = 1;
        }
        return E_NFC_READER_PRESENT;
    }
    else
    {
        if (iTagPresent)
        {
            daemon_log(LOG_INFO, "Reader removed");
            iTagPresent = 0;
        }
    }
    return E_NFC_OK;
}


teNfcStatus eNtagReadBlock(uint8_t u8Block, uint8_t *pu8Data)
{
    int iAttempts;
    int iResult;
    
    DEBUG_PRINTF("Read block %d\n", u8Block);
    
#if USE_SRAM
    if ((u8Block >= SRAM_START) && (u8Block <= SRAM_END))
    {
        DEBUG_PRINTF("Block is in SRAM space\n");
        u8Block = u8Block - SRAM_START + SRAM_FIXED_START;
    }
#endif /* USE_SRAM*/

    for (iAttempts = 0; iAttempts < 3; iAttempts++)
    {
#ifdef TEST_FILE
        if (lseek(iI2CFd, u8Block * 16, SEEK_SET) < 0)
        {
            DEBUG_PRINTF("Error seeking to read location (%s)\n", strerror(errno));
            iResult = 0;
        }
        else
        {
            DEBUG_PRINTF("Read offset %d\n", u8Block * 16);
            iResult = 1;
        }
#else
        while(eNtagRfUnlocked() == E_NFC_RF_LOCKED)
        {
            DEBUG_PRINTF("Waiting for RF to unlock\n");
        }
            
        iResult = write(iI2CFd, &u8Block, 1);
#endif /* TEST_FILE */
        if (iResult == 1)
        {
            iResult = read(iI2CFd, pu8Data, 16);
            if (iResult == 16)
            {
                int i;
                DEBUG_PRINTF("NTAG read block %02x:", u8Block);
                for (i = 0; i < 16; i++)
                {
                    DEBUG_PRINTF(" 0x%02X", pu8Data[i]);
                }
                DEBUG_PRINTF("\n");
                
                return E_NFC_OK;
            }
            else
            {
                DEBUG_PRINTF("Error reading (%d)\n", iResult);
            }
        }
    }

    daemon_log(LOG_ERR, "Communications failure reading NTAG block 0x%02x", u8Block);
    return E_NFC_COMMS_FAILED;
}


teNfcStatus eNtagRead(uint32_t u32UserMemoryAddress, uint32_t u32Length, uint8_t *pu8Data)
{
    uint8_t au8Buffer[16];
    uint8_t u8Block;
    uint32_t u32LengthRead = 0;
    teNfcStatus eStatus = E_NFC_ERROR;

    uint32_t u32EndAddress = u32UserMemoryAddress + u32Length;
    
    while (u32UserMemoryAddress < u32EndAddress)
    {
        uint32_t u32Offset;
        
        u8Block = 1 + (u32UserMemoryAddress / 16);
        u32Offset = u32UserMemoryAddress % 16;
        
        eStatus = eNtagReadBlock(u8Block, au8Buffer);
        if (eStatus != E_NFC_OK)
        {
            break;
        }
        
        memcpy(&pu8Data[u32LengthRead], &au8Buffer[u32Offset], (u32Length - u32LengthRead) > 16 ? (16 - u32Offset) : (u32Length - u32LengthRead));
        
        u32UserMemoryAddress += 16 - u32Offset;
        u32LengthRead += 16 - u32Offset;
    }
    return eStatus;
}


teNfcStatus eNtagWriteBlock(uint8_t u8Block, uint8_t *pu8Data)
{
    int iResult;
    uint8_t au8Data[17];

    if (u8Block == 0)
    {
        DEBUG_PRINTF("Attempt to write block 0!\n");
        return E_NFC_ERROR;
    }
    
#if USE_SRAM
    if ((u8Block >= SRAM_START) && (u8Block <= SRAM_END))
    {
        DEBUG_PRINTF("Block is in SRAM space\n");
        u8Block = u8Block - SRAM_START + SRAM_FIXED_START;
    }
#endif /* USE_SRAM*/

    au8Data[0] = u8Block;
    memcpy(&au8Data[1], pu8Data, 16);
    
#ifdef TEST_FILE
    if (lseek(iI2CFd, u8Block * 16, SEEK_SET) < 0)
    {
        DEBUG_PRINTF("Error seeking to write location\n");
        iResult = 0;
    }
    else
    {
        iResult = write(iI2CFd, &au8Data[1], 16) + 1;
    }
#else
    while(eNtagRfUnlocked() == E_NFC_RF_LOCKED)
    {
        DEBUG_PRINTF("Waiting for RF to unlock\n");
    }
    
    iResult = write(iI2CFd, au8Data, 17);
#endif /* TEST_FILE */
    if (iResult == 17)
    {
        int i;
        DEBUG_PRINTF("NTAG write block %02x:", u8Block);
        for (i = 0; i < 16; i++)
        {
            DEBUG_PRINTF(" 0x%02X", pu8Data[i]);
        }
        DEBUG_PRINTF("\n");

        return E_NFC_OK;
    }
    daemon_log(LOG_ERR, "Communications failue writing NTAG block 0x%02x", u8Block);
    return E_NFC_COMMS_FAILED;
}


teNfcStatus eNtagWrite(uint32_t u32UserMemoryAddress, uint32_t u32Length, uint8_t *pu8Data)
{
    uint8_t au8Buffer[16];
    uint8_t u8Block;
    uint32_t u32LengthWritten = 0;
    teNfcStatus eStatus = E_NFC_ERROR;
    uint32_t u32BlockOffset;

    uint32_t u32EndAddress = u32UserMemoryAddress + u32Length;
    
    DEBUG_PRINTF("NTAG write from %d to %d\n", u32UserMemoryAddress, u32EndAddress);
    
    u32BlockOffset = (u32UserMemoryAddress % 16);
    
    while (u32UserMemoryAddress < u32EndAddress)
    {
        int i;
        int iModified = 0;
        uint32_t u32BlockOverlap;
        
        DEBUG_PRINTF("Write incoming buffer starting at offset %d\n", u32LengthWritten);
        
        u8Block = 1 + (u32UserMemoryAddress / 16);
        
        u32BlockOverlap = (u32EndAddress - u32UserMemoryAddress) > 16 ? 16 : (u32EndAddress - u32UserMemoryAddress);
        u32BlockOverlap -= u32BlockOffset;
        
        DEBUG_PRINTF("Read block 0x%02X\n", u8Block);
        DEBUG_PRINTF("Block overlaps by %d bytes starting at offset %d, offset in incoming buffer: %d\n", u32BlockOverlap, u32BlockOffset, u32LengthWritten);
        
        eStatus = eNtagReadBlock(u8Block, au8Buffer);
        if (eStatus != E_NFC_OK)
        {
            DEBUG_PRINTF("Error reading block\n");
            break;
        }
        
        DEBUG_PRINTF(" TAG Load\n");
        for (i = 0; i < 16; i++)
        {
            DEBUG_PRINTF("0x%02X ", au8Buffer[i]);
            if (((u32LengthWritten + i) < (u32Length)) &&
                (i >= u32BlockOffset))
            {
                DEBUG_PRINTF("0x%02X\n", pu8Data[u32LengthWritten - u32BlockOffset + i]);
            }
            else
            {
                DEBUG_PRINTF("\n");
            }
        }
        
        if (memcmp(&au8Buffer[u32BlockOffset], &pu8Data[u32LengthWritten], u32BlockOverlap))
        {
            DEBUG_PRINTF("Data in block 0x%02X differs\n", u8Block);
            iModified = 1;
        }
        
        if (iModified)
        {
            DEBUG_PRINTF("Block modified\n");
            
            memcpy(&au8Buffer[u32BlockOffset], &pu8Data[u32LengthWritten], u32BlockOverlap);
            
            eStatus = eNtagWriteBlock(u8Block, au8Buffer);
            if (eStatus != E_NFC_OK)
            {
                break;
            }
        }
        
        u32UserMemoryAddress += 16;
        u32LengthWritten += u32BlockOverlap;
        u32BlockOffset = 0;
    }
    
    return eStatus;
}



static teNfcStatus eNtagReadSessionRegister(uint8_t u8Register, uint8_t* pu8Data)
{
    int iAttempts;
    int iResult;
    uint8_t au8Buffer[2];
    
    au8Buffer[0] = 0xFE;
    au8Buffer[1] = u8Register;

    for (iAttempts = 0; iAttempts < 3; iAttempts++)
    {
        iResult = write(iI2CFd, au8Buffer, 2);
        if (iResult == 2)
        {
            iResult = read(iI2CFd, pu8Data, 1);
            if (iResult == 1)
            {
                DEBUG_PRINTF("NTAG Session register %02d: 0x%02X\n", u8Register, *pu8Data);
                return E_NFC_OK;
            }
        }
    }
    daemon_log(LOG_ERR, "Communications failure reading NTAG session register %02d", u8Register);
    eNtagPwrCycle();
    return E_NFC_COMMS_FAILED;
}


static teNfcStatus eNtagWriteSessionRegister(uint8_t u8Register, uint8_t u8Mask, uint8_t u8Data)
{
    int iAttempts;
    int iResult;
    uint8_t au8Buffer[4];
    
    au8Buffer[0] = 0xFE;
    au8Buffer[1] = u8Register;
    au8Buffer[2] = u8Mask;
    au8Buffer[3] = u8Data;

    for (iAttempts = 0; iAttempts < 3; iAttempts++)
    {
        iResult = write(iI2CFd, au8Buffer, 4);
        if (iResult == 4)
        {
            DEBUG_PRINTF("Wrote NTAG Session register %02d\n", u8Register);
            return E_NFC_OK;
        }
    }
    daemon_log(LOG_ERR, "Communications failure writing NTAG session register %02d", u8Register);
    return E_NFC_COMMS_FAILED;
}


static teNfcStatus eNtagPwrOff(void)
{
    if (iPwrFd)
    {   
        lseek(iPwrFd, SEEK_SET, 0);
        
        if (write(iPwrFd, "0", 1) != 1)
        {
            daemon_log(LOG_ERR, "Error writing to power control GPIO (%s)", strerror(errno));
        }
    }
    return E_NFC_OK;
}


static teNfcStatus eNtagPwrOn(void)
{
    if (iPwrFd)
    {   
        lseek(iPwrFd, SEEK_SET, 0);
        
        if (write(iPwrFd, "1", 1) != 1)
        {
            daemon_log(LOG_ERR, "Error writing to power control GPIO (%s)", strerror(errno));
        }
    }
    return E_NFC_OK;
}


static teNfcStatus eNtagPwrCycle(void)
{
    daemon_log(LOG_INFO, "Power cycle tag");
    eNtagPwrOff();
    usleep(100*1000);
    return eNtagPwrOn();
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
