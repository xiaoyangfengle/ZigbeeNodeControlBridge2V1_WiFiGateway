/*****************************************************************************
 *
 * MODULE: ZigbeeNodeControlBridge
 *
 * COMPONENT: pdm_host.c
 *
 * $AUTHOR: Faisal Bhaiyat$
 *
 * DESCRIPTION:
 *
 * $HeadURL: $
 *
 * $Revision:  $
 *
 * $LastChangedBy:  $
 *
 * $LastChangedDate:  $
 *
 * $Id: $
 *
 *****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd 2008. All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <jendefs.h>
#include "PDM.h"
#include "os.h"
#include "os_gen.h"
#include "app_common.h"
#include "uart.h"
#include "SerialLink.h"
#include "log.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vUart_TxByte(bool bSpecialCharacter, uint8 u8Data);
PRIVATE bool_t bCheckPdmRecordsList(PDM_tsRecordDescriptor* pRds);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern uint8 au8LinkRxBuffer[MAX_PACKET_SIZE];
extern uint16 u16PacketType,u16PacketLength;
volatile uint8 u8QueueByte;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
OS_thMutex  s_hPdmMutex;
PDM_tsRecordDescriptor     *pHead = NULL;
/****************************************************************************/
/***        Exported Public Functions                                     ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vUart_WriteMessage
 *
 * DESCRIPTION:
 * Write message to the serial link
 *
 * PARAMETERS: Name                   RW  Usage
 *             u16Type                R   Message type
 *             u16Length              R   Message length
 *             pu8Data                R   Message payload
 * RETURNS:
 * void
 ****************************************************************************/
PUBLIC void vUart_WriteMessage(uint16 u16Type, uint16 u16Length, uint8 *pu8Data)
{
	int n;
	uint8 u8CRC;
	u8CRC = u8SL_CalculateCRC(u16Type, u16Length, pu8Data);

	/* Send start character */
	vUart_TxByte(TRUE, SL_START_CHAR);

	/* Send message type */
	vUart_TxByte(FALSE, (u16Type >> 8) & 0xff);
	vUart_TxByte(FALSE, (u16Type >> 0) & 0xff);

	/* Send message length */
	vUart_TxByte(FALSE, (u16Length >> 8) & 0xff);
	vUart_TxByte(FALSE, (u16Length >> 0) & 0xff);

	/* Send message checksum */
	vUart_TxByte(FALSE, u8CRC);

	/* Send message payload */
	for(n = 0; n < u16Length; n++)
	{
		vUart_TxByte(FALSE, pu8Data[n]);
	}

	/* Send end character */
	vUart_TxByte(TRUE, SL_END_CHAR);
}

/****************************************************************************
 *
 * NAME: PDM_vInit
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS:
 *
 *
 *
 * RETURNS:
 *
 ****************************************************************************/

PUBLIC void PDM_vInit(
        uint8                       u8StartSector,
        uint8                       u8NumSectors,
        uint32                      u32SectorSize,
        OS_thMutex                  hPdmMutex,
        OS_thMutex                  hPdmMediaMutex,
        PDM_tsHwFncTable           *psHwFuncTable,
        const tsReg128             *psKey)
{

	s_hPdmMutex = hPdmMutex;
}


/****************************************************************************
 *
 * NAME: PDM_eLoadRecord
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS:
 *
 *
 *
 * RETURNS:
 *
 ****************************************************************************/

PUBLIC PDM_teStatus PDM_eLoadRecord(
        PDM_tsRecordDescriptor     *psDesc,
        uint16                      u16IdValue,
        void                       *pvData,
        uint32                      u32DataSize,
        bool_t                      bSecure
        )
{
	uint8 *pu8Buffer,au8Buffer[MAX_PACKET_SIZE],*pdmBuffer;
	uint32 u32Length = 0;
	uint32 u32Size,u32TotalBlocks=0,u32BlockId=0,u32BlockSize=0;
	uint16 u16RecordId,u16MessageType;
	bool_t bNoMoreData = FALSE;
    if((psDesc==NULL)                               ||
        (pvData==NULL)                              ||
        (u32DataSize==0)
        )
    {
        return PDM_E_STATUS_INVLD_PARAM;
    }

    psDesc->u16UserId = u16IdValue;
    psDesc->pu8Data = (uint8*)pvData;
    psDesc->u32DataSize = u32DataSize;
    psDesc->u16Id = PDM_INVALID_ID;
    /* set security level of the record */
    psDesc->bSecure = bSecure;
    psDesc->eState = PDM_RECOVERY_STATE_NONE;

    if(!bCheckPdmRecordsList(psDesc))
    {
        psDesc->psNext = pHead;
        pHead = psDesc;
    }

    if (NULL != s_hPdmMutex) {
        OS_eEnterCriticalSection(s_hPdmMutex);
    }
	/* flush hardware buffer */
    UART_vOverrideInterrupt(FALSE);

   	pu8Buffer = au8Buffer;
   	memcpy(pu8Buffer, &u16IdValue,sizeof(uint16));
   	u32Length = sizeof(uint16);
   	pu8Buffer += sizeof(uint16);
   	u16MessageType = E_SL_MSG_LOAD_PDM_RECORD_REQUEST;
   	vUart_WriteMessage(u16MessageType,u32Length,au8Buffer);
   	pdmBuffer = psDesc->pu8Data;
   	do
   	{
   		u8QueueByte = 0xff;
		if(UART_bGetRxData((uint8*)&u8QueueByte) ){
   	    	if(TRUE==bSL_ReadMessage(&u16PacketType,&u16PacketLength,MAX_PACKET_SIZE,au8LinkRxBuffer,u8QueueByte))
   	    	{
  	    		switch(u16PacketType)
   	    		{
   	    		case(E_SL_MSG_LOAD_PDM_RECORD_RESPONSE):
   	    		{
   	    			uint8 u8Status = au8LinkRxBuffer[0];
   	    			memcpy(&u16RecordId,&au8LinkRxBuffer[1],sizeof(uint16));
	    			memcpy(&u32Size,&au8LinkRxBuffer[3],sizeof(uint32));
	    			memcpy(&u32TotalBlocks,&au8LinkRxBuffer[7],sizeof(uint32));
	    			memcpy(&u32BlockId,&au8LinkRxBuffer[11],sizeof(uint32));
	    			memcpy(&u32BlockSize,&au8LinkRxBuffer[15],sizeof(uint32));
   	    			if((u8Status != PDM_RECOVERY_STATE_NONE) &&
   	    					(u16RecordId == psDesc->u16UserId))
   	    			{
   	    				memcpy(pdmBuffer,&au8LinkRxBuffer[19],u32BlockSize);
   	    				pdmBuffer += u32BlockSize;
   	    				if(u32TotalBlocks == u32BlockId)
   	    				{
   	    					bNoMoreData = TRUE;
   	    					psDesc->eState = PDM_RECOVERY_STATE_RECOVERED;
   	    				}
   	    			}
   	    			if(u8Status == PDM_RECOVERY_STATE_NONE)
   	    			{
   	    				bNoMoreData = TRUE;
   	    				psDesc->eState = PDM_RECOVERY_STATE_NONE;
   	    			}
   	    			u8Status = 0;
   	    			memcpy(au8Buffer,&u8Status,sizeof(uint8));
   	    			memcpy(&au8Buffer[1],&u8Status,sizeof(uint8));
   	    			memcpy(&au8Buffer[2],&u16MessageType,sizeof(uint8));
   	    			vUart_WriteMessage(E_SL_MSG_STATUS, 4, au8Buffer);
   	    		}
					break;
   	    		default:
   	    			break;
   	    		}

   	    	}

		}

   	}
   	while(!bNoMoreData);

    if (NULL != s_hPdmMutex) {
        OS_eExitCriticalSection(s_hPdmMutex);
    }
    UART_vOverrideInterrupt(TRUE);
    return PDM_E_STATUS_OK;
}


/****************************************************************************
 *
 * NAME: PDM_vSaveRecord
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS:
 *
 *
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void PDM_vSaveRecord(
    PDM_tsRecordDescriptor     *psDesc)
{
	uint8 au8Buffer[MAX_PACKET_SIZE], *pu8Buffer,*pdmBuffer;
	uint32 u32NumberOfWrites = (psDesc->u32DataSize/128) + (((psDesc->u32DataSize%128)>0)? 1 : 0);
	uint32 u32Count=0,u32Size = 0,u32Length;
	bool_t bSaveAck = FALSE;


	if( u32NumberOfWrites == 0)
	{
		return;
	}
	UART_vOverrideInterrupt(FALSE);
    if (NULL != s_hPdmMutex) {
        OS_eEnterCriticalSection(s_hPdmMutex);
    }
    pdmBuffer = psDesc->pu8Data;
	do
	{
		u32Size = psDesc->u32DataSize - (u32Count * 128);
		u32Count++;
		pu8Buffer = au8Buffer;
		memcpy(pu8Buffer, &psDesc->u16UserId,sizeof(uint16));
		u32Length = sizeof(uint16);
		pu8Buffer += sizeof(uint16);
		memcpy(pu8Buffer, &psDesc->u32DataSize,sizeof(uint32));
		u32Length += sizeof(uint32);
		pu8Buffer += sizeof(uint32);
		memcpy(pu8Buffer, &u32NumberOfWrites,sizeof(uint32));
		u32Length += sizeof(uint32);
		pu8Buffer += sizeof(uint32);
		memcpy(pu8Buffer, &u32Count,sizeof(uint32));
		u32Length += sizeof(uint32);
		pu8Buffer += sizeof(uint32);
		if(u32Size > 128)
		{
			u32Size = 128;
		}
		memcpy(pu8Buffer, &u32Size,sizeof(uint32));
		u32Length += sizeof(uint32);
		pu8Buffer += sizeof(uint32);
		memcpy(pu8Buffer, pdmBuffer,u32Size);
		u32Length += u32Size;
		pu8Buffer += u32Size;
		pdmBuffer += u32Size;
		vUart_WriteMessage(E_SL_MSG_SAVE_PDM_RECORD,u32Length,au8Buffer);

		bSaveAck = FALSE;
		while(bSaveAck == FALSE)
		{
	   		u8QueueByte = 0xff;
			if(UART_bGetRxData((uint8*)&u8QueueByte) ){
	   	    	if(TRUE==bSL_ReadMessage(&u16PacketType,&u16PacketLength,MAX_PACKET_SIZE,au8LinkRxBuffer,u8QueueByte))
	   	    	{
	   	    		if(u16PacketType == E_SL_MSG_SAVE_PDM_RECORD_RESPONSE)
	   	    		{
	   	    			bSaveAck = TRUE;
	   	    		}
	   	    	}
			}
		}

	}
	while(u32Count != u32NumberOfWrites);


    if (NULL != s_hPdmMutex) {
        OS_eExitCriticalSection(s_hPdmMutex);
    }

    UART_vOverrideInterrupt(TRUE);

}


/****************************************************************************
 *
 * NAME: PDM_vSave
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS:
 *
 *
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void PDM_vSave(void)
{
	PDM_tsRecordDescriptor     *pRecordWalk = pHead;
	while(pRecordWalk != NULL)
	{
		PDM_vSaveRecord(pRecordWalk);
		pRecordWalk = pRecordWalk->psNext;
	}
}


/****************************************************************************
 *
 * NAME: PDM_vDelete
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS:
 *
 *
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void PDM_vDelete(void)
{
	uint8 u8Status = 0,au8Buffer[4];
	uint16 u16Value = E_SL_MSG_DELETE_PDM_RECORD;
	UART_vOverrideInterrupt(FALSE);
	memcpy(au8Buffer,&u8Status,sizeof(uint8));
	memcpy(&au8Buffer[1],&u8Status,sizeof(uint8));
	memcpy(&au8Buffer[2],&u16Value,sizeof(uint16));
	vUart_WriteMessage(E_SL_MSG_STATUS, 4, au8Buffer);
	vUart_WriteMessage(u16Value,0,NULL);
	UART_vOverrideInterrupt(TRUE);
}

/****************************************************************************
 *
 * NAME: PDM_vWaitHost
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS:
 *
 *
 *
 * RETURNS:
 *
 ****************************************************************************/
PUBLIC void PDM_vWaitHost(void)
{
	bool_t bSaveAck;
	uint8 u8QueueByte;
	volatile uint32 u32Delay;
	UART_vOverrideInterrupt(FALSE);


	bSaveAck = FALSE;
	vUart_WriteMessage(E_SL_MSG_PDM_HOST_AVAILABLE,0,NULL);
	while(bSaveAck == FALSE)
	{
        u32Delay++;
        if(u32Delay > 500000)
        {
        	vUart_WriteMessage(E_SL_MSG_PDM_HOST_AVAILABLE,0,NULL);
        	u32Delay = 0;
        }
   		u8QueueByte = 0xff;
		if(UART_bGetRxData(&u8QueueByte) ){
   	    	if(TRUE==bSL_ReadMessage(&u16PacketType,&u16PacketLength,MAX_PACKET_SIZE,au8LinkRxBuffer,u8QueueByte))
   	    	{
   	    		if(u16PacketType == E_SL_MSG_PDM_HOST_AVAILABLE_RESPONSE)
   	    		{
   	    			bSaveAck = TRUE;
   	    		}
   	    	}
		}
		vAHI_WatchdogRestart();
	}
	UART_vOverrideInterrupt(TRUE);
}
/****************************************************************************/
/***        Exported Private Functions                                      */
/****************************************************************************/


/****************************************************************************/
/***        Local Functions                                      */
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vUart_TxByte
 *
 * DESCRIPTION:
 * Send, escaping if required, a byte to the serial link
 *
 * PARAMETERS: 	Name        		RW  Usage
 *              bSpecialCharacter   R   TRUE if this byte should not be escaped
 *              u8Data              R   Character to send
 *
 * RETURNS:
 * void
 ****************************************************************************/
PRIVATE void vUart_TxByte(bool bSpecialCharacter, uint8 u8Data)
{
	if(!bSpecialCharacter && u8Data < 0x10)
	{
		/* Send escape character and escape byte */
		u8Data ^= 0x10;
		UART_vTxChar(SL_ESC_CHAR);
	}
	UART_vTxChar(u8Data);
}


/****************************************************************************
 *
 * NAME: bCheckPdmRecordsList
 *
 * DESCRIPTION:

 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 ****************************************************************************/
PRIVATE bool_t bCheckPdmRecordsList(PDM_tsRecordDescriptor* pRds)
{
	PDM_tsRecordDescriptor* pWalkingPointer = pHead;
	while(pWalkingPointer != NULL)
	{
		if(pWalkingPointer == pRds)
		{
			return TRUE;
		}
		pWalkingPointer = pWalkingPointer->psNext;
	}
	return FALSE;
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
