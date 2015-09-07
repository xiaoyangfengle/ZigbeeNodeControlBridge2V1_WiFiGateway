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

#ifndef  NDEF_H_INCLUDED
#define  NDEF_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <stdint.h>

#include <NTAG.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/


#define FROM_SETTINGSAPP           1
#define FROM_IOT_GATEWAY           2
#define FROM_THERMOSTAT_MASTER     3
#define FROM_THERMOSTAT_SLAVE      4
#define FROM_SENSOR                5
#define FROM_VALVE                 6

#define MAX_ENTRIES                14


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef struct 
{
    uint8_t   u8MessageSource;
    uint8_t   u8ID;
    uint8_t   u8NumEntries;
    uint8_t   u8AckID;
    int16_t   u16CRC;
    int16_t   u16Padding;
    union
    {
        uint8_t   au8Bytes[MAX_ENTRIES * 4];
        uint32_t  au32Words[MAX_ENTRIES];
    } uPayload;
} tsNdefData;


typedef enum
{
    E_NDEF_SET_TIME                 = 0x11,
    E_NDEF_NETWORK_GET_JOIN         = 0x30,
    E_NDEF_NETWORK_GET_JOIN_SECURE  = 0x31,
    E_NDEF_NETWORK_GET_LINK_INFO    = 0x32,
    E_NDEF_NETWORK_JOIN             = 0x40,
    E_NDEF_NETWORK_JOIN_SECURE      = 0x41,
    E_NDEF_NETWORK_LINK_INFO        = 0x42,
    
} teNdefMessage;


typedef struct
{
    uint8_t u8Padding;
    uint8_t u8Hour;
    uint8_t u8Minute;
} __attribute__((__packed__)) tsNdefSetTime;


typedef struct
{
    uint8_t     u8Channel;
    uint8_t     u8ActiveKeySequenceNumber;
    uint16_t    u16PanId;
    uint64_t    u64PanId;
    uint8_t     au8NetworkKey[16];
    uint8_t     au8MIC[4];
    uint64_t    u64TrustCenterAddress;
} __attribute__((__packed__)) tsNdefNetworkJoin;


typedef struct
{
    uint8_t     u8Version;
    uint8_t     u8Type;
    uint16_t    u16Profile;
    uint64_t    u64MacAddress;
    uint8_t     au8LinkKey[16];
} __attribute__((__packed__)) tsNdefNetworkLinkInfo;


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/** Set up connection to NTAG device

 *  \return E_NFC_OK on success
 */
teNfcStatus eNdefSetup(void);

teNfcStatus eNdefReadData(tsNdefData *psNdefData);


teNfcStatus eNdefInitData(tsNdefData *psData, uint8_t u8ID, uint8_t u8AckID);

teNfcStatus eNdefAddVarLenEntry(tsNdefData *psData, teNdefMessage u8Message, uint8_t u8Length);

teNfcStatus eNdefAddBytes(tsNdefData *psData, teNdefMessage u8Message, uint8_t *pu8Data, uint8_t u8Length);

teNfcStatus eNdefWriteData(tsNdefData *psData);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* NDEF_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
