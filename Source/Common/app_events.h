/*****************************************************************************
 *
 * MODULE:
 *
 * COMPONENT:
 *
 * AUTHOR:
 *
 * DESCRIPTION:
 *
 * $HeadURL: $
 *
 * $Revision:  $
 *
 * $LastChangedBy:  $
 *
 * $LastChangedDate: $
 *
 * $Id:  $
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

#ifndef APP_GENERIC_EVENTS_H_
#define APP_GENERIC_EVENTS_H_

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
#include "zll_commission.h"
#include "zll_utility.h"

#define TEN_HZ_TICK_TIME        APP_TIME_MS(100)

typedef enum
{
    APP_E_EVENT_NONE = 0,
    APP_E_EVENT_START_ROUTER,
    APP_E_EVENT_TOUCH_LINK,
    APP_E_EVENT_DATA_CONFIRM_FAILED,
    APP_E_EVENT_DATA_CONFIRM,
    APP_E_EVENT_EP_INFO_MSG,
    APP_E_EVENT_EP_LIST_MSG,
    APP_E_EVENT_GROUP_LIST_MSG,
    APP_E_EVENT_TRANSPORT_HA_KEY,
    APP_E_EVENT_SEND_PERMIT_JOIN,
    APP_E_EVENT_ENCRYPT_SEND_KEY,
    APP_E_EVENT_MAX
} APP_teEventType;


typedef struct
{
    uint8 u8Level;
} APP_tsEventLevel;

typedef struct
{
    uint16 u16SourceShortAddress;
    uint16 u16QueryTimeout;
} APP_tsEventHAQueryRsp;


typedef struct
{
    uint16 u16NwkAddr;
    uint8  u8Endpoint;
    uint16 u16ProfileId;
    uint16 u16DeviceId;
    uint8 u8Version;
    uint64 u64Address;
    uint8 u8MacCap;
} APP_tsEventTouchLink;

//#define APP_tsEventTouchLink tsCLD_ZllEndpointlistRecord

typedef struct {
    uint16 u16SrcAddr;
    tsCLD_ZllUtility_EndpointInformationCommandPayload sPayload;
} APP_tsEventEpInfoMsg;

typedef struct {
    uint8   u8SrcEp;
    uint16 u16SrcAddr;
    tsCLD_ZllUtility_GetEndpointListRspCommandPayload sPayload;
} APP_tsEventEpListMsg;

typedef struct {
    uint8   u8SrcEp;
    uint16 u16SrcAddr;
    tsCLD_ZllUtility_GetGroupIdRspCommandPayload sPayload;
} APP_tsEventGroupListMsg;

typedef struct {
uint64 u64Address;
AESSW_Block_u uKey;
} APP_tsEventEncryptSendMsg;

typedef struct
{
    APP_teEventType eType;
    union
    {
        APP_tsEventTouchLink                sTouchLink;
        APP_tsEventEpInfoMsg        sEpInfoMsg;
        APP_tsEventEpListMsg        sEpListMsg;
        APP_tsEventGroupListMsg     sGroupListMsg;
        APP_tsEventEncryptSendMsg sEncSendMsg;
        uint64 u64TransportKeyAddress;
    }uEvent;
} APP_tsEvent;

typedef enum {
    APP_E_COMMISSION_NONE = 0,
    APP_E_COMMISION_START,
    APP_E_COMMISION_BTN,
    APP_E_COMMISSION_TIMER_EXPIRED,
    APP_E_COMMISSION_MSG,
    APP_E_COMMISSION_ACK,
    APP_E_COMMISSION_NOACK,
    APP_E_COMMISSION_DISCOVERY_DONE,
    APP_E_COMMISSION_LEAVE_CFM
}APP_CommissionEventType;

typedef struct {
    APP_CommissionEventType eType;
    uint8 u8Lqi;
    tsZllMessage sZllMessage;
}APP_CommissionEvent;



/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC bool bAddToEndpointTable(APP_tsEventTouchLink *psEndpointData);
/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*APP_GENERIC_EVENTS_H_*/
