/*****************************************************************************
 *
 * MODULE:$
 *
 * COMPONENT:$
 *
 * AUTHOR:$
 *
 * DESCRIPTION:$
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
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5164,
 * JN5161, JN5148, JN5142, JN5139].
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
 *
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

#ifndef APP_COMMON_H_
#define APP_COMMON_H_

#include "app_timer_driver.h"
#include "zll_utility.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define TX_POWER_NORMAL     64
#define TX_POWER_LOW        (64-2)
#define ZCL_TICK_TIME           APP_TIME_MS(1000)
#define MAX_PACKET_SIZE      256
#define DEFAULT_CHANNEL        15
#define RAND_KEY            TRUE
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef struct {
    enum {
        FACTORY_NEW = 0,
        NOT_FACTORY_NEW = 0xff
    }eState;
    uint8 u8MyChannel;
    uint16 u16MyAddr;
    uint16 u16FreeAddrLow;
    uint16 u16FreeAddrHigh;
    uint16 u16FreeGroupLow;
    uint16 u16FreeGroupHigh;
}tsZllState;


typedef enum
{
    E_STARTUP,
    E_WAIT_START,
    E_DISCOVERY,
    E_NETWORK_INIT,
    E_RUNNING
} teNODE_STATES;

typedef enum {
    E_IDLE,
    E_SCANNING,
    E_SCAN_DONE,
    E_SCAN_WAIT_ID,
    E_SCAN_WAIT_INFO,
    E_WAIT_START_RSP,
    E_WAIT_JOIN_RTR_RSP,
    E_WAIT_JOIN_ZED_RSP,
    E_WAIT_DISCOVERY,
    E_SCAN_WAIT_RESET_SENT,
    E_SKIP_DISCOVERY,
    E_START_ROUTER,
    E_WAIT_LEAVE,
    E_WAIT_LEAVE_RESET,
    E_WAIT_START_UP,
    E_ACTIVE,
    E_INFORM_APP
}eState;
typedef struct {
   uint32 u32LedState;
   uint32 u32LedToggleTime;
   bool_t bIncreaseTime;
} tsLedState;

typedef struct {
    teNODE_STATES eNodeState;
    uint8 u8DeviceType;
} tsDeviceDesc;

typedef struct {
    eState eState;
    uint8 u8Count;
    uint8 u8Flags;
    bool_t bSecondScan;
    bool_t bResponded;
    uint32 u32TransactionId;
    uint32 u32ResponseId;
    uint32 u32TheirTransactionId;
    uint32 u32TheirResponseId;
} tsCommissionData;


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void vECB_Decrypt(uint8* au8Key,
                            uint8* au8InData,
                            uint8* au8OutData);
void vInitCommission(void);
PUBLIC void vStartAndAnnounce( void);

PUBLIC void vSetAddressMode(void);

PUBLIC void vAppAddGroup( uint16 u16GroupId, bool_t bBroadcast);
PUBLIC void vSetAddress(tsZCL_Address * psAddress, bool_t bBroadcast);
PUBLIC void APP_vConfigureDevice(uint8 u8DeviceType);
PUBLIC void app_vFormatAndSendUpdateLists(void);
PUBLIC void vForceStartRouter(uint8* pu8Buffer);
PUBLIC void vSendJoinedFormEventToHost(uint8 u8FormJoin,uint8* pu8Buffer);
PUBLIC bool bSendHATransportKey(uint64 u64DeviceAddress);
PUBLIC uint32 u32GetAttributeActualSize(uint32 u32Type,uint32 u32NumberOfItems);
PUBLIC uint16 u16ZncWriteDataPattern(uint8 *pu8Data, teZCL_ZCLAttributeType eAttributeDataType, uint8 *pu8Struct,  uint32 u32Size);
PUBLIC void Znc_vSendDataIndicationToHost(ZPS_tsAfEvent *psStackEvent,uint8* pau8StatusBuffer);
void ZNC_vSaveAllRecords(void);
#ifdef PDM_NONE
PUBLIC void PDM_vWaitHost(void);
#endif
/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/
extern tsCLD_ZllDeviceTable sDeviceTable;
extern PDM_tsRecordDescriptor sZllPDDesc;
extern PDM_tsRecordDescriptor sDevicePDDesc;
extern tsZllEndpointInfoTable sEndpointTable;
extern tsZllGroupInfoTable sGroupTable;
extern tsZllState sZllState;
extern tsDeviceDesc           sDeviceDesc;
extern PDM_tsRecordDescriptor sEndpointPDDesc;
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*APP_COMMON_H_*/
