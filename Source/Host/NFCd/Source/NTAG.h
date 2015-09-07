/*****************************************************************************
 *
 * MODULE:              Chip ID's
 *
 * COMPONENT:           $RCSfile: ChipID.h,v $
 *
 * VERSION:             $Name:  $
 *
 * REVISION:            $Revision: 1.25 $
 *
 * DATED:               $Date: 2009/07/15 08:16:39 $
 *
 * STATUS:              $State: Exp $
 *
 * AUTHOR:              Lee Mitchell
 *
 * DESCRIPTION:
 *
 *
 * LAST MODIFIED BY:    $Author: lmitch $
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

#ifndef  NTAG_H_INCLUDED
#define  NTAG_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <stdint.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef enum
{
    E_NFC_OK,
    E_NFC_ERROR,
    E_NFC_OUT_OF_MEMORY,
    E_NFC_ABORTED,
    E_NFC_COMMS_FAILED,
    E_NFC_TIMEOUT,
    E_NFC_READER_PRESENT,
    E_NFC_RF_LOCKED,
    
} teNfcStatus;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/** Set up connection to NTAG device
 *  \param pcPwrGPIO        Path to the sysfs file to write controlling power to tag.
 *  \param pcInterruptGPIO  Path to the sysfs file containing value of FD GPIO line
 *  \param pcI2CBus         Path to I2C dev of bus with NTAG connected
 *  \param iI2CAddress      Address of NTAG on bus
 *  \return E_NFC_OK on success
 */
teNfcStatus eNtagSetup(const char *pcPwrGPIO, const char *pcInterruptGPIO, const char *pcI2CBus, int iI2CAddress);


/** Wait for a reader to become present near the NTAG device.
 *  This function blocks waiting for an interrupt on the FD line
 *  \param iTimeoutMs Number of milliseconds to block for
 *  \return E_NFC_READER_PRESENT if reader becomes present.
 */
teNfcStatus eNtagWait(int iTimeoutMs);


/** Determine if a Reader is present near the NTAG device.
 *  \return E_NFC_READER_PRESENT when reader is present.
 */
teNfcStatus eNtagPresent(void);


teNfcStatus eNtagRfUnlocked(void);


/** Reads a block from a selected NTAG device
 *  \param u8Block Block to read
 *  \param pu8Data[out] 16 byte buffer to receive the selected data.
 *  \return E_NFC_OK on success
 */
teNfcStatus eNtagReadBlock(uint8_t u8Block, uint8_t *pu8Data);

/** Reads from a selected NTAG device
 *  \param u32UserMemoryAddress Memory address to read.
 *  \param u32Length Amount of data to read
 *  \param pu8Data[out] buffer to receive the selected data.
 *  \return E_NFC_OK on success
 */
teNfcStatus eNtagRead(uint32_t u32UserMemoryAddress, uint32_t u32Length, uint8_t *pu8Data);


/** Writes a block to a selected NTAG device
 *  \param u8Block Block to write
 *  \param pu8Data 16 byte buffer containing block data
 *  \return E_NFC_OK on success
 */
teNfcStatus eNtagWriteBlock(uint8_t u8Block, uint8_t *pu8Data);

/** Writes to a selected NTAG device
 *  \param u32UserMemoryAddress Memory address to read.
 *  \param u32Length Amount of data to write
 *  \param pu8Data[out] buffer to receive the selected data.
 *  \return E_NFC_OK on success
 */
teNfcStatus eNtagWrite(uint32_t u32UserMemoryAddress, uint32_t u32Length, uint8_t *pu8Data);


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* NTAG_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
