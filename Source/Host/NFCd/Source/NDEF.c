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
#include <arpa/inet.h>

#include <libdaemon/daemon.h>

#include "NDEF.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

//#define NDEF_DEBUG

#ifdef NDEF_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* NDEF_DEBUG */


#define NDEF_DATA_VERSION          8

#define PADDING_SIZE               12

#define POLY1 (0x03) // x + 1
#define POLY7 (0x89) // x^7 + x^3 + 1
#define POLY8 (0x9B) // CRC-8-WCDMA: x8 + x7 + x4 + x3 + x + 1
#define POLY16 (0x011021) // x^16 + x^12 + x^5 +1



#define CURRENT_DEVICE          FROM_IOT_GATEWAY

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef struct 
{
  uint8_t      u8Version;
  /* Padding takes up space from end of header to start of next block, where SRAM is mapped */
  uint8_t      au8Padding[PADDING_SIZE];
  tsNdefData   sData;
} __attribute__ ((packed)) tsNdefContainer ;

#define NDEF_USERDATA_SIZE         sizeof(tsNdefContainer)
#define NDEF_DATA_SIZE             sizeof(tsNdefData)

#define TAG_SRAM_SIZE             64

static unsigned crc_gen( unsigned char *msg, int len, unsigned poly, int w );

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


static const uint8_t au8NdefHeader[] = 
{
    0x03,  // Type   in TLV
    3 + 14 + NDEF_USERDATA_SIZE,  // Length in TLV
    0xd4,  // flags: mb me sr, no id
    0x0e,  // type len (14)

    NDEF_USERDATA_SIZE,  // payload len
    0x63,  // c
    0x6f,  // o
    0x6d,  // m

    0x2e,  // .
    0x6e,  // n
    0x78,  // x
    0x70,  // p

    0x3a,  // :
    0x77,  // w
    0x69,  // i
    0x73,  // s

    0x65,  // e
    0x30,  // 0
    0x31,  // 1
};


#define NDEF_USERDATA_INDEX   (sizeof(au8NdefHeader))

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

static void dump_bytes_padding(char *header, uint8_t * data, uint32_t datalen, uint32_t padding)
{
    int i, j;
    if (header)
    {
        DEBUG_PRINTF("%s", header);
    }
    for (i = 0; i < datalen; i += 16 )
    {
        if (i > 0 && ((i % 16) == 0 )) 
        {
            DEBUG_PRINTF("\r\n");
        }
        int len = datalen - i;
        if (len > 16)
        {
            len = 16;
        }
        for (j = 0; j < len; j++)
        {
            DEBUG_PRINTF("%02x ", data[i + j] & 0xFF);
        }

        if (padding || ((len < 16) && (datalen > 16)))
        {
            // It was a multi-line dump, so add padding spaces
            for (j = len; j < 16; j++ )
            {
                DEBUG_PRINTF("   " );
            }
        }

        DEBUG_PRINTF(" - " );
        for (j = 0; j < len; j++)
        {
            char c = data[i+j];
            if (( c >= 0x20 ) && ( c <= 0x7E ))
            {
                DEBUG_PRINTF("%c", c);
            }
            else
            {
                DEBUG_PRINTF(".");
            }
        }
    }
    DEBUG_PRINTF("\r\n" );
}


static void dump_bytes(char *header, uint8_t *data, uint32_t datalen)
{
    dump_bytes_padding(header, data, datalen, 1);
}


teNfcStatus eNdefSetup(void)
{
    uint8_t buf[sizeof(au8NdefHeader)];

    // Read current header
    if (eNtagRead( 0, sizeof(au8NdefHeader), buf) == E_NFC_OK) 
    {
        int i;
        DEBUG_PRINTF("NDEF autoformat\n");
        DEBUG_PRINTF("NDEF header size = %d\n", (int)sizeof(au8NdefHeader));
        DEBUG_PRINTF("NDEF data   size = %d\n", (int)NDEF_DATA_SIZE );

        // Look for diferences
        for (i = 0; i < sizeof(au8NdefHeader); i++)
        {
            if (buf[i] != au8NdefHeader[i])
            {
                DEBUG_PRINTF("Header mismatch at offset %d\n", i);
                break;
            }
        }

        // If differ, then write new header
        if (i < sizeof(au8NdefHeader))
        {
            dump_bytes("NDEF header in tag:\n", buf, sizeof(au8NdefHeader));

            if (eNtagWrite( 0, sizeof(au8NdefHeader), (uint8_t*)au8NdefHeader) != E_NFC_OK)
            {
                daemon_log(LOG_ERR, "Error loading NDEF header into tag");
                return E_NFC_ERROR;
            }
        }
        else
        {
            DEBUG_PRINTF("NDEF header: OK\n" );
        }
    }
    else
    {
        daemon_log(LOG_ERR, "Error reading NDEF header");
        return E_NFC_ERROR;
    }
    return E_NFC_OK;
}


teNfcStatus eNdefReadData(tsNdefData *psNdefData)
{
    tsNdefContainer sContainer;
    teNfcStatus eStatus;
    
    // Read host data from tag
    if ((eStatus = eNtagRead(NDEF_USERDATA_INDEX, NDEF_USERDATA_SIZE, (uint8_t*)&sContainer)) != E_NFC_OK)
    {
        return eStatus;
    }
    dump_bytes( "NDEF: User data\r\n", (uint8_t *)&sContainer, NDEF_DATA_SIZE );

#ifdef USE_ENCRYPTION
    // Decrypt
    crypto_wep_stream_cipher( (unsigned char *)"1234567890123456", 16, (uint8_t *)&sContainer.sData, NDEF_DATA_SIZE );

    // dump_bytes( "NDEF: Read after decrypt:\r\n", (char *)&ndef_data, NDEF_DATA_SIZE );
#endif

    if (sContainer.u8Version == NDEF_DATA_VERSION)
    {
        // Check CRC
        uint16_t u16CalculatedCRC;
        uint16_t u16MessageCRC = ntohs(sContainer.sData.u16CRC);
        sContainer.sData.u16CRC = 0;
        
        u16CalculatedCRC = crc_gen((unsigned char *)&sContainer.sData, NDEF_DATA_SIZE, POLY16, 16 );
        DEBUG_PRINTF("Calculated CRC = 0x%04x\r\n", u16CalculatedCRC);
        if (u16CalculatedCRC == u16MessageCRC )
        {
            DEBUG_PRINTF("NDEF: CRC is OK\r\n" );
            sContainer.sData.u16CRC = u16CalculatedCRC;
            memcpy(psNdefData, &sContainer.sData, sizeof(tsNdefData));
            return E_NFC_OK;
        }
        else
        {
            DEBUG_PRINTF("NDEF: CRC error 0x%04x != 0x%04x\r\n", u16CalculatedCRC, u16MessageCRC );
        }
    }
    else
    {
        DEBUG_PRINTF("NDEF: Version error (%d != %d)\r\n", sContainer.u8Version, NDEF_DATA_VERSION );
    }
    return E_NFC_ERROR;
}


teNfcStatus eNdefInitData(tsNdefData *psData, uint8_t u8ID, uint8_t u8AckID)
{
    psData->u8MessageSource = CURRENT_DEVICE;
    psData->u8ID            = u8ID;
    psData->u8NumEntries    = 0;
    psData->u8AckID         = u8AckID;
    psData->u16CRC          = 0;
    psData->u16Padding      = 0;
    
    memset(psData->uPayload.au32Words, 0, MAX_ENTRIES * sizeof(uint32_t));
    return E_NFC_OK;
}


teNfcStatus eNdefAddVarLenEntry(tsNdefData *psData, teNdefMessage u8Message, uint8_t u8Length)
{
    uint32_t u32Word = (0xFF << 24) + ((u8Message & 0xFF) << 16) + ((u8Length & 0xFF) << 8);
    psData->uPayload.au32Words[psData->u8NumEntries++] = htonl(u32Word);
    return E_NFC_OK;
}


teNfcStatus eNdefAddBytes(tsNdefData *psData, teNdefMessage u8Message, uint8_t *pu8Data, uint8_t u8Length)
{
    teNfcStatus eStatus;
    if (u8Length > 0)
    {
        uint8_t u8Words = ( u8Length + 3 ) / 4;
        
        if ((eStatus = eNdefAddVarLenEntry(psData, u8Message, u8Words)) != E_NFC_OK)
        {
            return eStatus;
        }
        memset(&psData->uPayload.au32Words[psData->u8NumEntries], 0, u8Words * 4);
        memcpy(&psData->uPayload.au32Words[psData->u8NumEntries], pu8Data, u8Length);
        psData->u8NumEntries += u8Words;
    }
    else
    {
        return E_NFC_ERROR;
    }
    return E_NFC_OK;
}


teNfcStatus eNdefWriteData(tsNdefData *psData)
{
    tsNdefContainer sContainer;
    sContainer.u8Version = NDEF_DATA_VERSION;

    memset(sContainer.au8Padding, 0, PADDING_SIZE);
    memcpy(&sContainer.sData, psData, sizeof(tsNdefData));

    // Calculate CRC
    uint16_t crc = (uint16_t)crc_gen( (unsigned char *)psData, NDEF_DATA_SIZE, POLY16, 16 );
    sContainer.sData.u16CRC = htons( crc );

    dump_bytes( "Write NDEF-DATA:\n", (uint8_t*)&sContainer.sData, NDEF_DATA_SIZE );

#ifdef USE_ENCRYPTION
    // Encrypt
    crypto_wep_stream_cipher( (unsigned char *)"1234567890123456", 16, (unsigned char *)&sContainer.sData, NDEF_DATA_SIZE );

    // dump_bytes( "Write NDEF-DATA after encrypt:\r\n", (char *)&sContainer.data, NDEF_DATA_SIZE );
#endif

    // Write to tag
    return eNtagWrite( NDEF_USERDATA_INDEX, NDEF_USERDATA_SIZE, (uint8_t*)&sContainer );
}


static unsigned crc_gen( unsigned char *msg, int len, unsigned poly, int w )
{
  int i, j, top;
  unsigned char b;
  unsigned rem;
  
  // init
  top = 1U << w;
  rem = 0;
  
  for( i = 0; i < len; i++ )
  {
    b = msg[i];
    
    for( j = 0; j < 8; j++ )
    {
      // shift in msb of byte
      rem <<= 1;
      if( b & 0x80 )
      {
        rem |= 1;
      }
      
      if( rem & top ) // check msb of remainder
      {
        rem ^= poly; // reduce
      }
      
      b <<= 1;
    }
  }
  
  // shift in w zeroes
  for( j = 0; j < w; j++ )
  {
    rem <<= 1;
    if( rem & top ) // check msb of remainder
    {
      rem ^= poly; // reduce
    }
  }
  
  return rem;
}



/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
