/*****************************************************************************
 *
 * MODULE: ZigbeeNodeControlBridge
 *
 * COMPONENT: app_ZncCommission_task.c
 *
 * $AUTHOR: Faisal Bhaiyat$
 *
 * DESCRIPTION:
 *
 * $HeadURL: https://www.collabnet.nxp.com/svn/lprf_sware/Projects/Zigbee%20Protocol%20Stack/ZPS/Trunk/ZigbeeNodeControlBridge/Source/ZigbeeNodeControlBridge/app_ZncCommission_task.c $
 *
 * $Revision: 55089 $
 *
 * $LastChangedBy: nxp29741 $
 *
 * $LastChangedDate: 2013-07-03 18:11:32 +0100 (Wed, 03 Jul 2013) $
 *
 * $Id: app_ZncCommission_task.c 55089 2013-07-03 17:11:32Z nxp29741 $
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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include <appapi.h>
#include "os.h"
#include "os_gen.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg.h"
#include "pwrm.h"
#include "zps_apl_af.h"
#include "zps_apl_zdo.h"
#include "zps_apl_aib.h"
#include "zps_apl_zdp.h"
#include "app_timer_driver.h"
#include "app_events.h"
#include "zcl_customcommand.h"
#include "app_ZncParser_task.h"
#include "mac_sap.h"
#include "zll_commission.h"
#include <rnd_pub.h>
#include <mac_pib.h>
#include <string.h>
#include <stdlib.h>
#include "app_common.h"
#include "Log.h"
#include "SerialLink.h"
#include "PDM_IDs.h"

#ifndef DEBUG_SCAN
#define TRACE_SCAN            TRUE
#else
#define TRACE_SCAN            TRUE
#endif

#ifndef DEBUG_JOIN
#define TRACE_JOIN            TRUE
#else
#define TRACE_JOIN            TRUE
#endif

#ifndef DEBUG_COMMISSION
#define TRACE_COMMISSION      TRUE
#else
#define TRACE_COMMISSION      TRUE
#endif


#define SHOW_KEY            TRUE
#define ADJUST_POWER        TRUE
#define FIX_CHANNEL         TRUE
#define ZLL_SCAN_LQI_MIN    (110)
#define LED1  (1 << 1)
#define LED2  (1)




typedef struct {
    ZPS_tsInterPanAddress       sSrcAddr;
    tsCLD_ZllCommission_ScanRspCommandPayload                   sScanRspPayload;
}tsZllScanData;

typedef struct {
    uint16 u16LQI;
    tsZllScanData sScanDetails;
}tsZllScanTarget;


PDM_tsRecordDescriptor sGroupPDDesc;

PRIVATE uint32 u32LEDs = 0;

PRIVATE uint8 u8NewUpdateID(uint8 u8ID1, uint8 u8ID2);


PRIVATE void vSendScanResponse( ZPS_tsNwkNib *psNib,
                                ZPS_tsInterPanAddress       *psDstAddr,
                                uint32 u32TransactionId,
                                uint32 u32ResponseId);

PRIVATE void vSendScanRequest(void *pvNwk, tsCommissionData *psCommission);
PRIVATE void vHandleScanResponse( void *pvNwk,
                                  ZPS_tsNwkNib *psNib,
                                  tsZllScanTarget *psScanTarget,
                                  APP_CommissionEvent  *psEvent);

PRIVATE void vSendStartRequest( void *pvNwk,
                                ZPS_tsNwkNib *psNib,
                                tsZllScanTarget *psScanTarget,
                                uint32 u32TransactionId);
PRIVATE void vSendFactoryResetRequest( void *pvNwk,
                                       ZPS_tsNwkNib *psNib,
                                       tsZllScanTarget *psScanTarget,
                                       uint32 u32TransactionId);
PRIVATE void vSendRouterJoinRequest( void * pvNwk,
                                     ZPS_tsNwkNib *psNib,
                                     tsZllScanTarget *psScanTarget,
                                     uint32 u32TransactionId);
PRIVATE void vSendEndDeviceJoinRequest( void *pvNwk,
                                        ZPS_tsNwkNib *psNib,
                                        tsZllScanTarget *psScanTarget,
                                        uint32 u32TransactionId);
PRIVATE void vHandleNwkUpdateRequest( void * pvNwk,
                                      ZPS_tsNwkNib *psNib,
                                      tsCLD_ZllCommission_NetworkUpdateReqCommandPayload *psNwkUpdateReqPayload);
PRIVATE void vHandleIdentiftRequest(uint16 u16Duration);
PRIVATE void vHandleDeviceInfoRequest(APP_CommissionEvent *psEvent);
PRIVATE void vHandleEndDeviceJoinRequest( void * pvNwk,
                                          ZPS_tsNwkNib *psNib,
                                          APP_CommissionEvent *psEvent,
                                          tsCommissionData *psCommission);
PRIVATE void vHandleNwkStartResponse( void * pvNwk,
                                          APP_CommissionEvent *psEvent,
                                          tsCommissionData *psCommission);
PRIVATE void vHandleNwkStartRequest( ZPS_tsInterPanAddress  *psDstAddr,
                                     APP_CommissionEvent *psEvent,
                                     tsCommissionData *psCommission);
PRIVATE void vHandleRouterJoinRequest( void * pvNwk,
        ZPS_tsNwkNib *psNib,
        APP_CommissionEvent *psEvent,
        tsCommissionData *psCommission);
PRIVATE void vSendIdentifyRequest(uint64 u64Addr, uint32 u32TransactionId, uint8 u8Time);
PRIVATE void vSendDeviceInfoReq(uint64 u64Addr, uint32 u32TransactionId);
PRIVATE void vEndCommissioning( void* pvNwk, eState eState, uint16 u16TimeMS);
PRIVATE bool bSearchDiscNt(ZPS_tsNwkNib *psNib, uint64 u64EpId, uint16 u16PanId);
PRIVATE uint8 eEncryptKey(uint8* au8InData,
                    uint8* au8OutData,
                    uint32 u32TransId,
                    uint32 u32ResponseId,
                    uint8 u8KeyIndex);
PRIVATE uint8 eDecryptKey(uint8* au8InData,
                    uint8* au8OutData,
                    uint32 u32TransId,
                    uint32 u32ResponseId,
                    uint8 u8KeyIndex);
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
tsZllScanTarget sScanTarget;

tsCLD_ZllCommission_NetworkStartReqCommandPayload sStartParams;


PRIVATE tsReg128 sMasterKey = {0x11223344, 0x55667788, 0x99aabbcc, 0xddeeff00 };
PRIVATE tsReg128 sCertKey = {0xc0c1c2c3, 0xc4c5c6c7,0xc8c9cacb,0xcccdcecf};

PUBLIC bool_t bScanActive = FALSE;
PUBLIC bool_t bSendFactoryResetOverAir=FALSE;

tsZllState sZllState = {FACTORY_NEW,  ZLL_SKIP_CH1, ZLL_MIN_ADDR, ZLL_MIN_ADDR, ZLL_MAX_ADDR, ZLL_MIN_GROUP, ZLL_MAX_GROUP};

APP_tsEventTouchLink        sTarget;
ZPS_tsInterPanAddress       sDstAddr;

tsCommissionData sCommission;
tsZllEndpointInfoTable sEndpointTable;
tsZllGroupInfoTable sGroupTable;
PDM_tsRecordDescriptor sZllPDDesc;
extern bool_t bResetIssued;
extern tsCLD_ZllDeviceTable sDeviceTable;
extern tsDeviceDesc           sDeviceDesc;
extern void ZPS_vTCSetCallback(void*);
/****************************************************************************
 *
 * NAME: vEndCommissioning
 *
 * DESCRIPTION:

 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vEndCommissioning( void* pvNwk, eState eState, uint16 u16TimeMS){
    sCommission.eState = eState;
    ZPS_vNwkNibSetChannel( pvNwk, sZllState.u8MyChannel);

#if ADJUST_POWER
    eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_NORMAL);
#endif
    sCommission.u32TransactionId = 0;
    sCommission.bResponded = FALSE;
    OS_eStopSWTimer(APP_CommissionTimer);
    bScanActive = FALSE;
    if (u16TimeMS > 0) {
        OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(u16TimeMS), NULL);
    }

}
/****************************************************************************
 *
 * NAME: vInitCommission
 *
 * DESCRIPTION:
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vInitCommission()
{
    sCommission.eState = E_IDLE;
}

/****************************************************************************
 *
 * NAME: APP_SendIndentify_Task
 *
 * DESCRIPTION:
 * Task that sends out the identify query to devices in identify mode
 * Activated by the expire of APP_DelayIndentifyTimer which is
 * two seconds after the toggle power button is pressed
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vCommissionTask( void) {
    APP_CommissionEvent         sEvent;
    APP_tsEvent                   sAppEvent;
    tsZllPayloads sZllCommand;
    uint8 u8Seq;
    if (OS_eCollectMessage(APP_CommissionEvents, &sEvent) != OS_E_OK)
    {
        vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "\n\nCommision task error!!!\n");
        return;
    }

    /* If coordinator read the commissiong event off the queue but do nothing */
    if(sDeviceDesc.u8DeviceType == 0)
    {
        return;
    }

    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Commision task state %d type %d\n", sCommission.eState, sEvent.eType);
    void *pvNwk = ZPS_pvAplZdoGetNwkHandle();
    ZPS_tsNwkNib *psNib = ZPS_psNwkNibGetHandle( pvNwk);
    switch (sCommission.eState)
    {
        case E_IDLE:
            if (sEvent.eType == APP_E_COMMISION_START)
            {
                sCommission.u8Count = 0;
                sScanTarget.u16LQI = 0;
                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
                sCommission.eState = E_SCANNING;
                sCommission.u32TransactionId = RND_u32GetRand(1, 0xffffffff);
                sCommission.bResponded = FALSE;
                /* Turn down Tx power */
#if ADJUST_POWER
                eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_LOW);
#endif
                sCommission.bSecondScan = FALSE;
                bScanActive = TRUE;

            } else if (sEvent.eType == APP_E_COMMISSION_MSG ) {
                if (sEvent.sZllMessage.eCommand == E_CLD_COMMISSION_CMD_SCAN_REQ )
                {
                    if (sEvent.u8Lqi < ZLL_SCAN_LQI_MIN) {
                        vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "\nIgnore scan reqXX");
                        return;

                    }

                    bScanActive = TRUE;


jumpHere:
                    vLog_Printf((TRACE_SCAN|TRACE_COMMISSION),LOG_DEBUG, "Got Scan Req\n");
                    /* Turn down Tx power */
#if ADJUST_POWER
                    eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_LOW);
#endif


                    sDstAddr = sEvent.sZllMessage.sSrcAddr;
                    sDstAddr.u16PanId = 0xFFFF;

                    sCommission.u32ResponseId = RND_u32GetRand(1, 0xffffffff);
                    sCommission.u32TransactionId = sEvent.sZllMessage.uPayload.sScanReqPayload.u32TransactionId;

                    vSendScanResponse( psNib,
                                       &sDstAddr,
                                      sCommission.u32TransactionId,
                                      sCommission.u32ResponseId);

                    sCommission.eState = E_ACTIVE;
                    /* Timer to end inter pan */
                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(ZLL_INTERPAN_LIFE_TIME_SEC), NULL);
                }
            }
            break;

        case E_SCANNING:
            if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED) {
                /*
                 * Send the next scan request
                 */
                vSendScanRequest(pvNwk, &sCommission);
            } else if (sEvent.eType == APP_E_COMMISSION_MSG) {
                switch (sEvent.sZllMessage.eCommand )
                {
                case E_CLD_COMMISSION_CMD_SCAN_RSP:
                    vHandleScanResponse(pvNwk,
                                        psNib,
                                        &sScanTarget,
                                        &sEvent);
                    break;

                case E_CLD_COMMISSION_CMD_SCAN_REQ:
                    /* If we are FN and get a scan req from a NFN remote then
                     * give up our scan and respond to theirs
                     */
                    if (sEvent.u8Lqi < ZLL_SCAN_LQI_MIN) {
                       vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "\nIgnore scan req");
                       return;
                    }
                    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Scan Req1\n");
                    if ((sZllState.eState == FACTORY_NEW) &&
                        ( (sEvent.sZllMessage.uPayload.sScanReqPayload.u8ZllInfo & ZLL_FACTORY_NEW) == 0 )) {

                        vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Scan Req while scanning\n");
                        OS_eStopSWTimer(APP_CommissionTimer);
                        goto jumpHere;
                    }
                    else
                    {
                        if ( !sCommission.bResponded) {
                            /*
                             * Only respond once
                             */
                            sCommission.bResponded = TRUE;

                            sDstAddr = sEvent.sZllMessage.sSrcAddr;
                            sDstAddr.u16PanId = 0xFFFF;

                            sCommission.u32TheirResponseId = RND_u32GetRand(1, 0xffffffff);
                            sCommission.u32TheirTransactionId = sEvent.sZllMessage.uPayload.sScanReqPayload.u32TransactionId;

                            sCommission.u8Flags = 0;
                            if ((sEvent.sZllMessage.uPayload.sScanReqPayload.u8ZigbeeInfo & ZLL_TYPE_MASK) != ZLL_ZED)
                            {
                                // Not a ZED requester, set FFD bit in flags
                                sCommission.u8Flags |= 0x02;
                            }
                            if (sEvent.sZllMessage.uPayload.sScanReqPayload.u8ZigbeeInfo & ZLL_RXON_IDLE)
                            {
                                // RxOnWhenIdle, so set RXON and power source bits in the flags
                                sCommission.u8Flags |= 0x0c;
                            }

                            vSendScanResponse(psNib,
                                              &sDstAddr,
                                              sCommission.u32TheirTransactionId,
                                              sCommission.u32TheirResponseId);

                        }
                    }
                    break;

                case E_CLD_COMMISSION_CMD_DEVICE_INFO_REQ:
                    if (sEvent.sZllMessage.uPayload.sDeviceInfoReqPayload.u32TransactionId == sCommission.u32TheirTransactionId) {
                        vHandleDeviceInfoRequest(&sEvent);
                    }
                    break;

                case E_CLD_COMMISSION_CMD_IDENTIFY_REQ:
                    if (sEvent.sZllMessage.uPayload.sIdentifyReqPayload.u32TransactionId == sCommission.u32TheirTransactionId) {
                        vHandleIdentiftRequest(sEvent.sZllMessage.uPayload.sIdentifyReqPayload.u16Duration);
                    }
                    break;

                default:
                    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Unhandled during scan %02x\n", sEvent.sZllMessage.eCommand );
                    break;
                }

            }

            break;
        case E_SCAN_WAIT_RESET_SENT:
            /* Factory reset command has been sent,
             * end commissioning
             */
            vEndCommissioning(pvNwk, E_IDLE, 0);
            break;
        case E_SCAN_DONE:
            if ( sScanTarget.u16LQI )
            {
                #ifdef TEST_RANGE
                    if (( (sZllState.u16FreeAddrHigh - sZllState.u16FreeAddrLow) < 5) &&
                       ( (sZllState.u16FreeGroupHigh - sZllState.u16FreeGroupLow) < 5))
                    {
                        vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Out of Addr H%04x L%04x ", sZllState.u16FreeAddrHigh, sZllState.u16FreeAddrLow);
                        vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "GH%04x GL%04x\n", sZllState.u16FreeGroupHigh, sZllState.u16FreeGroupLow);
                        vEndCommissioning(pvNwk);
                        return;
                    }
                #endif
                    /* scan done and we have a target */
                    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, sScanTarget.sScanDetails.sScanRspPayload.u8LogicalChannel);
                    if ( (sZllState.eState == NOT_FACTORY_NEW) &&
                            ( (sScanTarget.sScanDetails.sScanRspPayload.u8ZllInfo & ZLL_FACTORY_NEW) == 0) &&
                            ( (sScanTarget.sScanDetails.sScanRspPayload.u8ZigbeeInfo & ZLL_TYPE_MASK) == ZLL_ZED ) &&
                            (sScanTarget.sScanDetails.sScanRspPayload.u64ExtPanId == psNib->sPersist.u64ExtPanId)
                            ) {
                        /* On our network, just gather endpoint details */
                        vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "NFN -> NFN ZED On Our nwk\n");
                        /* tell the app about the target we just touched with */
                        sTarget.u16NwkAddr = sScanTarget.sScanDetails.sScanRspPayload.u16NwkAddr;
                        sTarget.u8Endpoint = sScanTarget.sScanDetails.sScanRspPayload.u8Endpoint;
                        sTarget.u16ProfileId = sScanTarget.sScanDetails.sScanRspPayload.u16ProfileId;
                        sTarget.u16DeviceId = sScanTarget.sScanDetails.sScanRspPayload.u16DeviceId;
                        sTarget.u8Version = sScanTarget.sScanDetails.sScanRspPayload.u8Version;

                        vEndCommissioning(pvNwk, E_INFORM_APP, 6500 );
                        return;
                    }
                    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Send an Id Req to %016llx Id %08x\n", sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr, sCommission.u32TransactionId);
                    vSendIdentifyRequest(sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr, sCommission.u32TransactionId, 3);
                    OS_eStopSWTimer(APP_CommissionTimer);
                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
                    sCommission.eState = E_SCAN_WAIT_ID;

            }
            else
            {
                vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "scan No results\n");
                if (sCommission.bSecondScan)
                {
                    vEndCommissioning(pvNwk, E_IDLE, 0);
                    return;
                }
                else
                {
                    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG,  "Second scan\n");
                    sCommission.eState = E_SCANNING;
                    sCommission.bSecondScan = TRUE;
                    #ifdef ZLL_PRIMARY
                        sCommission.u8Count = 12;
                    #else
                        sCommission.u8Count = 11;
                    #endif
                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
                }
            }

            break;

        case E_SCAN_WAIT_ID:
            if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED)
            {
                vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Wait id time time out\n");

                /* send a device info req */
                vSendDeviceInfoReq(sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr, sCommission.u32TransactionId);

                OS_eStopSWTimer(APP_CommissionTimer);
                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(2), NULL);
                sCommission.eState = E_SCAN_WAIT_INFO;
            }
            break;

        case E_SCAN_WAIT_INFO:

            if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED)
            {
                vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Wait Info time out\n");
                vEndCommissioning(pvNwk, E_IDLE, 0);
                return;
            }
            else if (sEvent.eType == APP_E_COMMISSION_MSG)
            {
                vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Wait info com msg Cmd=%d  %d\n", sEvent.sZllMessage.eCommand, E_CLD_COMMISSION_CMD_DEVICE_INFO_RSP);
                if ( ( sEvent.sZllMessage.eCommand == E_CLD_COMMISSION_CMD_DEVICE_INFO_RSP )
                        &&  ( sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.u32TransactionId == sCommission.u32TransactionId ) )
                {
                    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Got Device Info Rsp\n");

                    sTarget.u16ProfileId = sScanTarget.sScanDetails.sScanRspPayload.u16ProfileId;
                    sTarget.u16DeviceId = sScanTarget.sScanDetails.sScanRspPayload.u16DeviceId;
                    sTarget.u8Endpoint = sScanTarget.sScanDetails.sScanRspPayload.u8Endpoint;
                    sTarget.u8Version = sScanTarget.sScanDetails.sScanRspPayload.u8Version;
                    // nwk addr will be assigned in a bit

                    sDstAddr.eMode = 3;
                    sDstAddr.u16PanId = 0xffff;
                    sDstAddr.uAddress.u64Addr = sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr;

                    vLog_Printf(TRACE_SCAN,LOG_DEBUG, "Back to %016llx Mode %d\n", sDstAddr.uAddress.u64Addr, sDstAddr.eMode);
                    if(bSendFactoryResetOverAir)
                    {
                        bSendFactoryResetOverAir=FALSE;
                        OS_eStopSWTimer(APP_CommissionTimer);
                        vSendFactoryResetRequest( pvNwk,
                                           psNib,
                                           &sScanTarget,
                                           sCommission.u32TransactionId);
                        sCommission.eState = E_SCAN_WAIT_RESET_SENT;
                        /* wait a bit for message to go, then finsh up */
                        OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
                        vLog_Printf(TRACE_SCAN,LOG_DEBUG, "\nvSendFactoryResetRequest and going to E_IDLE\n");
                    }
                    else if (sZllState.eState == FACTORY_NEW)
                    {
                        // we are FN CB, target must be a FN or NFN router
                        vSendStartRequest(pvNwk,psNib,&sScanTarget,sCommission.u32TransactionId);
                        sCommission.eState = E_WAIT_START_RSP;
                        vLog_Printf(TRACE_SCAN,LOG_DEBUG, "Sent start req\n");
                        OS_eStopSWTimer(APP_CommissionTimer);
                        OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(2), NULL);
                    }
                    else
                    {
                        vLog_Printf(TRACE_SCAN,LOG_DEBUG, "NFN -> ??? %02x\n", sScanTarget.sScanDetails.sScanRspPayload.u8ZllInfo);
                        if (sScanTarget.sScanDetails.sScanRspPayload.u8ZllInfo & ZLL_FACTORY_NEW)
                        {
                            /* FN target */
                            if ((sScanTarget.sScanDetails.sScanRspPayload.u8ZigbeeInfo & ZLL_TYPE_MASK) != ZLL_ZED)
                            {
                                vLog_Printf(TRACE_SCAN,LOG_DEBUG, "NFN -> FN ZR\n");
                                /* router join */
                                vSendRouterJoinRequest(pvNwk, psNib, &sScanTarget, sCommission.u32TransactionId);
                                sCommission.eState = E_WAIT_JOIN_RTR_RSP;
                                OS_eStopSWTimer(APP_CommissionTimer);
                                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(2), NULL);

                            }
                            else
                            {
                                vLog_Printf(TRACE_SCAN,LOG_DEBUG,"NFN -> FN ZED\n");
                                /* end device join */
                                vSendEndDeviceJoinRequest( pvNwk, psNib, &sScanTarget, sCommission.u32TransactionId);
                                sCommission.eState = E_WAIT_JOIN_ZED_RSP;
                                OS_eStopSWTimer(APP_CommissionTimer);
                                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(2), NULL);
                            }
                        }
                        else
                        {
                            /* NFN target, must be ZR on different nwk */
                            if (sScanTarget.sScanDetails.sScanRspPayload.u64ExtPanId == psNib->sPersist.u64ExtPanId) {
                                vLog_Printf(TRACE_SCAN,LOG_DEBUG,"On our nwk\n"  );
                                /* On our network, just gather endpoint details */
                                vLog_Printf(TRACE_SCAN,LOG_DEBUG, "NFN -> NFN ZR On Our nwk\n");
                                /* tell the app about the target we just touched with */
                                sTarget.u16NwkAddr = sScanTarget.sScanDetails.sScanRspPayload.u16NwkAddr;
                                sTarget.u8Endpoint = sScanTarget.sScanDetails.sScanRspPayload.u8Endpoint;
                                sTarget.u16ProfileId = sScanTarget.sScanDetails.sScanRspPayload.u16ProfileId;
                                sTarget.u16DeviceId = sScanTarget.sScanDetails.sScanRspPayload.u16DeviceId;
                                sTarget.u8Version = sScanTarget.sScanDetails.sScanRspPayload.u8Version;

                                vEndCommissioning(pvNwk, E_INFORM_APP, 1500);
                                return;
                            }
                            vLog_Printf(TRACE_SCAN,LOG_DEBUG, "NFN -> NFN ZR other nwk\n");

                            vSendRouterJoinRequest( pvNwk, psNib, &sScanTarget, sCommission.u32TransactionId);
                            sCommission.eState = E_WAIT_JOIN_RTR_RSP;
                            OS_eStopSWTimer(APP_CommissionTimer);
                            OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(2), NULL);
                        }
                    }
                }
            }
            break;

        case E_WAIT_START_RSP:
            if (sEvent.eType == APP_E_COMMISSION_MSG) {
                vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Start Rsp %d\n", sEvent.sZllMessage.eCommand);
                if (sEvent.sZllMessage.eCommand == E_CLD_COMMISSION_CMD_NETWORK_START_RSP) {
                    vHandleNwkStartResponse(pvNwk, &sEvent, &sCommission);
                }
            }
            else if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED)  {
                vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "wait startrsp time out\n");
                vEndCommissioning(pvNwk, E_IDLE, 0);
            }
             break;

        case E_WAIT_JOIN_RTR_RSP:
            if (sEvent.eType == APP_E_COMMISSION_MSG) {
                if (sEvent.sZllMessage.eCommand == E_CLD_COMMISSION_CMD_NETWORK_JOIN_ROUTER_RSP) {
                    vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Got Router join Rsp %d\n", sEvent.sZllMessage.uPayload.sNwkJoinRouterRspPayload.u8Status);
                    OS_eStopSWTimer(APP_CommissionTimer);
                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(ZLL_START_UP_TIME_SEC), NULL);
                    sCommission.eState = E_WAIT_START_UP;
                }
            }
            else if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED)
            {
                vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "zr join time out\n");
                vEndCommissioning(pvNwk, E_IDLE, 0);
            }
            break;

        case E_WAIT_JOIN_ZED_RSP:
            if (sEvent.eType == APP_E_COMMISSION_MSG) {
                if (sEvent.sZllMessage.eCommand == E_CLD_COMMISSION_CMD_NETWORK_JOIN_END_DEVICE_RSP) {
                    vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Got Zed join Rsp %d\n", sEvent.sZllMessage.uPayload.sNwkJoinEndDeviceRspPayload.u8Status);
                    OS_eStopSWTimer(APP_CommissionTimer);
                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(1 /*ZLL_START_UP_TIME_SEC*/), NULL);
                    sCommission.eState = E_WAIT_START_UP;
                }
            }
            else if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED)
            {
                vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "ed join time out\n");
                vEndCommissioning(pvNwk, E_IDLE, 0);
            }
            break;

        /*
         * We are the target of a scan
         */
        case E_ACTIVE:
            /* handle process after scan rsp */
            if (sEvent.eType == APP_E_COMMISION_START)
            {
                if (sZllState.eState == NOT_FACTORY_NEW) {
                    vLog_Printf(TRACE_SCAN,LOG_DEBUG,"Abandon scan\n");
                     OS_eStopSWTimer(APP_CommissionTimer);
                    sCommission.u8Count = 0;
                    sScanTarget.u16LQI = 0;
                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
                    sCommission.eState = E_SCANNING;
                    sCommission.u32TransactionId = RND_u32GetRand(1, 0xffffffff);
                    sCommission.bResponded = FALSE;
                    /* Turn down Tx power */
    #if ADJUST_POWER
                    eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_LOW);
    #endif
                    sCommission.bSecondScan = FALSE;
                    bScanActive = TRUE;
                    return;
                }

            }
            if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED)
            {
                vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Active time out\n");
                vEndCommissioning(pvNwk, E_IDLE, 0);
            }
            else if (sEvent.eType == APP_E_COMMISSION_MSG)
            {
                vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Zll cmd %d in active\n", sEvent.sZllMessage.eCommand);
                if (sEvent.sZllMessage.uPayload.sScanReqPayload.u32TransactionId ==  sCommission.u32TransactionId)
                {
                    /* Set up return address */
                    sDstAddr = sEvent.sZllMessage.sSrcAddr;
                    sDstAddr.u16PanId = 0xffff;
                    switch(sEvent.sZllMessage.eCommand)
                    {
                        /* if we receive the factory reset clear the persisted data and do a reset.
                         * Spoke to Philips and they say " Yup anyone can reset and bring nodes to factory settings"
                         */
                        case E_CLD_COMMISSION_CMD_FACTORY_RESET_REQ:
                            if (sZllState.eState == NOT_FACTORY_NEW) {
                                sCommission.eState = E_WAIT_LEAVE_RESET;
                                u32OldFrameCtr = psNib->sTbl.u32OutFC + 10;
                                /* leave req */
                                ZPS_eAplZdoLeaveNetwork(0, FALSE,FALSE);
                            }
                            break;
                        case E_CLD_COMMISSION_CMD_NETWORK_UPDATE_REQ:
                            vHandleNwkUpdateRequest(pvNwk,
                                                    psNib,
                                                    &sEvent.sZllMessage.uPayload.sNwkUpdateReqPayload);
                            break;

                        case E_CLD_COMMISSION_CMD_IDENTIFY_REQ:
                             vHandleIdentiftRequest(sEvent.sZllMessage.uPayload.sIdentifyReqPayload.u16Duration);
                            break;

                        case E_CLD_COMMISSION_CMD_DEVICE_INFO_REQ:
                            vHandleDeviceInfoRequest(&sEvent);
                            break;

                        case E_CLD_COMMISSION_CMD_NETWORK_JOIN_ROUTER_REQ:
                            vHandleRouterJoinRequest( pvNwk,
                                                      psNib,
                                                      &sEvent,
                                                      &sCommission);
                            break;

                        case E_CLD_COMMISSION_CMD_NETWORK_START_REQ:
                            vHandleNwkStartRequest( &sDstAddr,
                                                    &sEvent,
                                                    &sCommission);
                            break;

                        case E_CLD_COMMISSION_CMD_NETWORK_JOIN_END_DEVICE_REQ:
                            vHandleEndDeviceJoinRequest( pvNwk,
                                                         psNib,
                                                         &sEvent,
                                                         &sCommission);
                            break;

                        default:
                            break;

                    }               /* endof switch(sEvent.sZllMessage.eCommand) */
                }
            }               /* endof if (sEvent.eType == APP_E_COMMISSION_MSG) */

            break;

        case E_WAIT_DISCOVERY:
           if (sEvent.eType != APP_E_COMMISSION_DISCOVERY_DONE) {

               return;
           }
           else
           {
               ZPS_tsAplAib *psAib = ZPS_psAplAibGetAib();
               vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "discovery in commissioning\n");
               /* get unique set of pans */
               while (!bSearchDiscNt(psNib, sStartParams.u64ExtPanId,
                       sStartParams.u16PanId))
               {
                   sStartParams.u16PanId = RND_u32GetRand(1, 0xfffe);
                   if(psAib->u64ApsUseExtendedPanid == 0)
                   {
                       sStartParams.u64ExtPanId = RND_u32GetRand(1, 0xffffffff);
                       sStartParams.u64ExtPanId <<= 32;
                       sStartParams.u64ExtPanId |= RND_u32GetRand(0, 0xffffffff);
                   }
                   else
                   {
                       sStartParams.u64ExtPanId = psAib->u64ApsUseExtendedPanid;
                   }
               };
           }
           // Deliberate fall through

       case E_SKIP_DISCOVERY:
           vLog_Printf(TRACE_JOIN,LOG_DEBUG, "New Epid %016llx Pan %04x\n",
                   sStartParams.u64ExtPanId, sStartParams.u16PanId);
           vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "e_skip-discovery\n");
           if (sStartParams.u8LogicalChannel == 0)
           {
               // pick random
#if RAND_CH
               int n;
#endif

               /* Get randon channel from set of channels
                * 4 <= n < 8
                * will give a channel from index 4 5 6 or 7 of the array
                */
#if RAND_CH
               n = (uint8)RND_u32GetRand( 4, 8);

               vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Picked Ch %d\n",
                       au8ZLLChannelSet[n]);
#endif

#if RAND_CH
               sStartParams.u8LogicalChannel = au8ZLLChannelSet[n];
#else
               sStartParams.u8LogicalChannel = DEFAULT_CHANNEL;
#endif
          }

           /* send start rsp */
           sZllCommand.uPayload.sNwkStartRspPayload.u32TransactionId = sCommission.u32TransactionId;
           sZllCommand.uPayload.sNwkStartRspPayload.u8Status = ZLL_SUCCESS;
           sZllCommand.uPayload.sNwkStartRspPayload.u64ExtPanId = sStartParams.u64ExtPanId;
           sZllCommand.uPayload.sNwkStartRspPayload.u8NwkUpdateId = 0;
           sZllCommand.uPayload.sNwkStartRspPayload.u8LogicalChannel = sStartParams.u8LogicalChannel;
           sZllCommand.uPayload.sNwkStartRspPayload.u16PanId = sStartParams.u16PanId;
           vLog_Printf(TRACE_COMMISSION ,LOG_DEBUG, "send Start Resp\n");
           eCLD_ZllCommissionCommandNetworkStartRspCommandSend(
                   &sDstAddr,
                   &u8Seq,
                   (tsCLD_ZllCommission_NetworkStartRspCommandPayload*) &sZllCommand.uPayload);

           /* Geteway can't bwe stolen so factory new test has already been done
            * to have got here we must be factory new so no need to initiate a
            * Leave before starting as a router
            */
           sCommission.eState = E_START_ROUTER;

           /* give message time to go */
           OS_eStopSWTimer(APP_CommissionTimer);
           OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
           break;

        case E_WAIT_LEAVE:
            break;

        case E_WAIT_LEAVE_RESET:
            if ((sEvent.eType == APP_E_COMMISSION_LEAVE_CFM) || (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED )) {
                vLog_Printf(TRACE_JOIN,LOG_DEBUG,"WARNING: Received Factory reset \n");
                PDM_vDeleteAllDataRecords();
                vAHI_SwReset();
            }
            break;

        case E_START_ROUTER:
            vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "\nStart router\n");
            /* set zll params */
            sZllState.u16MyAddr  = sStartParams.u16NwkAddr;
            sZllState.u16FreeAddrLow = sStartParams.u16FreeNwkAddrBegin;
            sZllState.u16FreeAddrHigh = sStartParams.u16FreeNwkAddrEnd;
            sZllState.u16FreeGroupLow = sStartParams.u16FreeGroupIdBegin;
            sZllState.u16FreeGroupHigh = sStartParams.u16FreeGroupIdEnd;
            sZllState.u8MyChannel = sStartParams.u8LogicalChannel;
            sZllState.eState = NOT_FACTORY_NEW;
            vSetGroupAddress( sStartParams.u16GroupIdBegin, GROUPS_REQUIRED);
            PDM_eSaveRecordData( PDM_ID_APP_ZLL_CMSSION,&sZllState,sizeof(tsZllState));

            /* Set nwk params */
            void * pvNwk = ZPS_pvAplZdoGetNwkHandle();
            ZPS_vNwkNibSetNwkAddr(pvNwk, sStartParams.u16NwkAddr);
            vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Given A %04x on %d\n", sStartParams.u16NwkAddr, sStartParams.u8LogicalChannel);
            ZPS_vNwkNibSetChannel(pvNwk, sStartParams.u8LogicalChannel);
            ZPS_vNwkNibSetPanId(pvNwk, sStartParams.u16PanId);
            ZPS_vNwkNibSetExtPanId(pvNwk, sStartParams.u64ExtPanId);
            ZPS_eAplAibSetApsUseExtendedPanId(sStartParams.u64ExtPanId);

            eDecryptKey(sStartParams.au8NwkKey,
                        psNib->sTbl.psSecMatSet[0].au8Key, sCommission.u32TransactionId,
                        sCommission.u32ResponseId, sStartParams.u8KeyIndex);

            psNib->sTbl.psSecMatSet[0].u8KeySeqNum = 0;
            memset(psNib->sTbl.pu32InFCSet, 0,(sizeof(uint32) * psNib->sTblSize.u16NtActv));
            psNib->sTbl.psSecMatSet[0].u8KeyType = ZPS_NWK_SEC_NETWORK_KEY;

            /* save security material to flash */
            ZPS_vNwkSaveSecMat(pvNwk);

            /* Make this the Active Key */
            ZPS_vNwkNibSetKeySeqNum(pvNwk, 0);

            /* Start router */
            vStartAndAnnounce();

            sZllState.eState = NOT_FACTORY_NEW;
            PDM_eSaveRecordData( PDM_ID_APP_ZLL_CMSSION,&sZllState,sizeof(tsZllState));

            if (sStartParams.u16InitiatorNwkAddr != 0)
            {
                ZPS_eAplZdoDirectJoinNetwork(sStartParams.u64InitiatorIEEEAddr,  sStartParams.u16InitiatorNwkAddr, sCommission.u8Flags /*MAC_FLAGS*/);
                vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Direct join %02x\n", sCommission.u8Flags);
            }

            sDeviceDesc.eNodeState = E_RUNNING;
            PDM_eSaveRecordData( PDM_ID_APP_CONTROL_BRIDGE,&sDeviceDesc,sizeof(tsDeviceDesc));

            ZPS_eAplAibSetApsTrustCenterAddress(0xffffffffffffffffULL);
#if PERMIT_JOIN
            ZPS_eAplZdoPermitJoining( PERMIT_JOIN_TIME);
#endif

            vLog_Printf(TRACE_JOIN,LOG_DEBUG, "My Address %04x\n", sZllState.u16MyAddr);
            vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Free Addr low %04x\n", sZllState.u16FreeAddrLow);
            vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Free Addr High %04x\n", sZllState.u16FreeAddrHigh);
            vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Free Group low %04x\n", sZllState.u16FreeGroupLow);
            vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Free Group high%04x\n", sZllState.u16FreeGroupHigh);
            vLog_Printf(TRACE_JOIN,LOG_DEBUG, "My Channel %d\n", sZllState.u8MyChannel);

            sCommission.eState = E_IDLE;
            sCommission.u32TransactionId = 0;
            sCommission.u32ResponseId = 0;
            OS_eStopSWTimer(APP_CommissionTimer);
            vLog_Printf(TRACE_JOIN,LOG_DEBUG, "All done\n");
            break;

        case E_WAIT_START_UP:
        {
            uint8 au8StatusBuffer[20];
            uint8* pu8Buffer = au8StatusBuffer;
            if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED) {
                #if ADJUST_POWER
                eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_NORMAL);
                #endif
                bScanActive = FALSE;
                if ( sZllState.eState == FACTORY_NEW ) {
                    ZPS_eAplAibSetApsTrustCenterAddress( 0xffffffffffffffffULL );
                    vLog_Printf(TRACE_JOIN,LOG_DEBUG, "start up on the channel\n");
                    vStartAndAnnounce();
                    sZllState.eState = NOT_FACTORY_NEW;
                } else {
                    vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Back on Ch %d\n", sZllState.u8MyChannel);
                    ZPS_vNwkNibSetChannel( ZPS_pvAplZdoGetNwkHandle(), sZllState.u8MyChannel);

                }

                vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Free addr %04x to %04x\n", sZllState.u16FreeAddrLow, sZllState.u16FreeAddrHigh);
                vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Free Gp  %04x to %04x\n", sZllState.u16FreeGroupLow, sZllState.u16FreeGroupHigh);
                vSendJoinedFormEventToHost(1,pu8Buffer);
                sCommission.eState = E_INFORM_APP;
                PDM_eSaveRecordData( PDM_ID_APP_ZLL_CMSSION,&sZllState,sizeof(tsZllState));
                sCommission.u32TransactionId = 0;
                sCommission.bResponded = FALSE;
                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(1500), NULL );
                vLog_Printf(TRACE_JOIN,LOG_DEBUG, "\nWait inform");
                sDeviceDesc.eNodeState = E_RUNNING;
            }
        }
            break;

        case E_INFORM_APP:
            /* tell the ap about the target we just joined with */
            if (sTarget.u16NwkAddr > 0) {
                sAppEvent.eType = APP_E_EVENT_TOUCH_LINK;
                sAppEvent.uEvent.sTouchLink= sTarget;
                sAppEvent.uEvent.sTouchLink.u64Address = sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr;
                sAppEvent.uEvent.sTouchLink.u8MacCap = 0;
                if ((sScanTarget.sScanDetails.sScanRspPayload.u8ZigbeeInfo & ZLL_TYPE_MASK) != ZLL_ZED)
                {
                    // Not a ZED requester, set FFD bit in flags
                    sAppEvent.uEvent.sTouchLink.u8MacCap |= 0x02;
                }
                if (sScanTarget.sScanDetails.sScanRspPayload.u8ZigbeeInfo & ZLL_RXON_IDLE)
                {
                    // RxOnWhenIdle, so set RXON and power source bits in the flags
                    sAppEvent.uEvent.sTouchLink.u8MacCap |= 0x0c;
                }
                vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "\nInform on %04x", sTarget.u16NwkAddr);
                OS_ePostMessage(APP_msgEvents, &sAppEvent);
            }
            sCommission.eState = E_IDLE;

            vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "\nInform App, stop");
            break;

        default:
            vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Unhandled event %d\n", sEvent.eType);
            break;
    }
}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendScanRequest(void *pvNwk, tsCommissionData *psCommission)
{
    ZPS_tsInterPanAddress       sDstAddr;
    tsCLD_ZllCommission_ScanReqCommandPayload                   sScanReqPayload;
    uint8 u8Seq;


    if (((psCommission->u8Count == 8) && !psCommission->bSecondScan) ||
        ( (psCommission->u8Count == 27) && psCommission->bSecondScan ) ){
        psCommission->eState = E_SCAN_DONE;
        OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
    } else {

        if (psCommission->bSecondScan) {
            vLog_Printf(TRACE_SCAN,LOG_DEBUG, "2nd Scan Ch %d\n", psCommission->u8Count);
            ZPS_vNwkNibSetChannel( pvNwk, psCommission->u8Count++);
            if ( (psCommission->u8Count == ZLL_SKIP_CH1) || (psCommission->u8Count == ZLL_SKIP_CH2) || (psCommission->u8Count == ZLL_SKIP_CH3) || (sCommission.u8Count == ZLL_SKIP_CH4) ) {
                psCommission->u8Count++;
#ifdef ZLL_PRIMARY_PLUS3
            if (psCommission->u8Count==ZLL_SKIP_CH4) {psCommission->u8Count++;}
#endif
            }
        } else {
            vLog_Printf(TRACE_SCAN,LOG_DEBUG, "\nScan Ch %d", au8ZLLChannelSet[psCommission->u8Count]);
            ZPS_vNwkNibSetChannel( pvNwk, au8ZLLChannelSet[psCommission->u8Count++]);
        }

        sDstAddr.eMode = ZPS_E_AM_INTERPAN_SHORT;
        sDstAddr.u16PanId = 0xffff;
        sDstAddr.uAddress.u16Addr = 0xffff;
        sScanReqPayload.u32TransactionId = psCommission->u32TransactionId;
        sScanReqPayload.u8ZigbeeInfo =  ZLL_ROUTER | ZLL_RXON_IDLE;              // Rxon idle router

        sScanReqPayload.u8ZllInfo =
            (sZllState.eState == FACTORY_NEW)? (ZLL_FACTORY_NEW|ZLL_LINK_INIT|ZLL_ADDR_ASSIGN):(ZLL_LINK_INIT|ZLL_ADDR_ASSIGN);                  // factory New Addr assign Initiating

        eCLD_ZllCommissionCommandScanReqCommandSend(
                                    &sDstAddr,
                                     &u8Seq,
                                     &sScanReqPayload);
        OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(ZLL_SCAN_RX_TIME_MS), NULL);
    }
}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendScanResponse(ZPS_tsNwkNib *psNib,
                               ZPS_tsInterPanAddress       *psDstAddr,
                               uint32 u32TransactionId,
                               uint32 u32ResponseId)
{
    tsCLD_ZllCommission_ScanRspCommandPayload                   sScanRsp;
    uint8 u8Seq;
    uint8 i;


    memset(&sScanRsp, 0, sizeof(tsCLD_ZllCommission_ScanRspCommandPayload));

    sScanRsp.u32TransactionId = u32TransactionId;
    sScanRsp.u32ResponseId = u32ResponseId;


    sScanRsp.u8RSSICorrection = 0;
    sScanRsp.u8ZigbeeInfo = ZLL_ROUTER | ZLL_RXON_IDLE;
    sScanRsp.u16KeyMask = ZLL_SUPPORTED_KEYS;
    if (sZllState.eState == FACTORY_NEW) {
        sScanRsp.u8ZllInfo = ZLL_FACTORY_NEW|ZLL_ADDR_ASSIGN;
        // Ext Pan 0
        // nwk update 0
        // logical channel zero
        // Pan Id 0
        sScanRsp.u16NwkAddr = 0xffff;
    } else {
        sScanRsp.u8ZllInfo = ZLL_ADDR_ASSIGN;
        sScanRsp.u64ExtPanId = psNib->sPersist.u64ExtPanId;
        sScanRsp.u8NwkUpdateId = psNib->sPersist.u8UpdateId;
        sScanRsp.u16PanId = psNib->sPersist.u16VsPanId;
        sScanRsp.u16NwkAddr = psNib->sPersist.u16NwkAddr;
    }

    sScanRsp.u8LogicalChannel = psNib->sPersist.u8VsChannel;
    for (i=0; i<sDeviceTable.u8NumberDevices; i++) {
        sScanRsp.u8TotalGroupIds += sDeviceTable.asDeviceRecords[i].u8NumberGroupIds;
    }
    sScanRsp.u8NumberSubDevices = sDeviceTable.u8NumberDevices;
    if (sScanRsp.u8NumberSubDevices == 1) {
        sScanRsp.u8Endpoint = sDeviceTable.asDeviceRecords[0].u8Endpoint;
        sScanRsp.u16ProfileId = sDeviceTable.asDeviceRecords[0].u16ProfileId;
        sScanRsp.u16DeviceId = sDeviceTable.asDeviceRecords[0].u16DeviceId;
        sScanRsp.u8Version = sDeviceTable.asDeviceRecords[0].u8Version;
        sScanRsp.u8GroupIdCount = sDeviceTable.asDeviceRecords[0].u8NumberGroupIds;
    }

    eCLD_ZllCommissionCommandScanRspCommandSend( psDstAddr,
            &u8Seq,
            &sScanRsp);
}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleScanResponse(void *pvNwk,
                                 ZPS_tsNwkNib *psNib,
                                 tsZllScanTarget *psScanTarget,
                                 APP_CommissionEvent         *psEvent)
{
    uint16 u16AdjustedLqi;
    ZPS_tsInterPanAddress       sDstAddr;

    uint8 u8Seq;
    u16AdjustedLqi = psEvent->u8Lqi + psEvent->sZllMessage.uPayload.sScanRspPayload.u8RSSICorrection;
    if (u16AdjustedLqi < ZLL_SCAN_LQI_MIN ) {
        vLog_Printf(TRACE_SCAN,LOG_DEBUG, "Reject LQI %d\n", psEvent->u8Lqi);
        return;
    }

    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Their %016llx Mine %016llx TC %016llx\n",
            psEvent->sZllMessage.uPayload.sScanRspPayload.u64ExtPanId,
            psNib->sPersist.u64ExtPanId,
            ZPS_eAplAibGetApsTrustCenterAddress()
            );
    if ( (sZllState.eState == NOT_FACTORY_NEW) &&
            (psEvent->sZllMessage.uPayload.sScanRspPayload.u64ExtPanId != psNib->sPersist.u64ExtPanId) &&
            ( ZPS_eAplAibGetApsTrustCenterAddress() != 0xffffffffffffffffULL )
            )
    {
        vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Non Zll nwk, target not my nwk\n");
        return;
    }

    /*check for supported keys */
    if ( (ZLL_SUPPORTED_KEYS & psEvent->sZllMessage.uPayload.sScanRspPayload.u16KeyMask) == 0)
    {
        // No common supported key index
        if ( (sZllState.eState == FACTORY_NEW) ||
            ( (sZllState.eState == NOT_FACTORY_NEW) && (psEvent->sZllMessage.uPayload.sScanRspPayload.u64ExtPanId != psNib->sPersist.u64ExtPanId) )
                )
        {
            /* Either we are factory new or
             * we are not factory new and the target is on another pan
             * there is no common key index, to exchange keys so drop the scan response without further action
             */
            vLog_Printf(TRACE_SCAN,LOG_DEBUG, "No common security level, target not my nwk\n");
            return;
        }
    }
    /*
     * Check for Nwk Update Ids
     */
    if ( (sZllState.eState == NOT_FACTORY_NEW) &&
         (psEvent->sZllMessage.uPayload.sScanRspPayload.u64ExtPanId == psNib->sPersist.u64ExtPanId) &&
         (psEvent->sZllMessage.uPayload.sScanRspPayload.u16PanId == psNib->sPersist.u16VsPanId))
    {
        vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Our Nwk Id %d theirs %d\n", psNib->sPersist.u8UpdateId, psEvent->sZllMessage.uPayload.sScanRspPayload.u8NwkUpdateId );
        if (psNib->sPersist.u8UpdateId != psEvent->sZllMessage.uPayload.sScanRspPayload.u8NwkUpdateId ) {
            if ( psNib->sPersist.u8UpdateId == u8NewUpdateID(psNib->sPersist.u8UpdateId, psEvent->sZllMessage.uPayload.sScanRspPayload.u8NwkUpdateId) ) {
                vLog_Printf(TRACE_COMMISSION, LOG_DEBUG, "Use ours on %d\n", sZllState.u8MyChannel);
                /*
                 * Send them a network update request
                 */
                tsCLD_ZllCommission_NetworkUpdateReqCommandPayload          sNwkUpdateReqPayload;

                sDstAddr.eMode = ZPS_E_AM_INTERPAN_IEEE;
                sDstAddr.u16PanId = 0xffff;
                sDstAddr.uAddress.u64Addr = psEvent->sZllMessage.sSrcAddr.uAddress.u64Addr;

                sNwkUpdateReqPayload.u32TransactionId = sCommission.u32TransactionId;
                sNwkUpdateReqPayload.u16NwkAddr = psEvent->sZllMessage.uPayload.sScanRspPayload.u16NwkAddr;
                sNwkUpdateReqPayload.u64ExtPanId = psNib->sPersist.u64ExtPanId;
                sNwkUpdateReqPayload.u8NwkUpdateId = psNib->sPersist.u8UpdateId;
                sNwkUpdateReqPayload.u8LogicalChannel = sZllState.u8MyChannel;

                sNwkUpdateReqPayload.u16PanId = psNib->sPersist.u16VsPanId;


                eCLD_ZllCommissionCommandNetworkUpdateReqCommandSend(
                                            &sDstAddr,
                                            &u8Seq,
                                            &sNwkUpdateReqPayload);

                return;

            } else {
                /*
                 * Part of a network and our Update Id is out of date
                 */
                ZPS_vNwkNibSetUpdateId( pvNwk, psEvent->sZllMessage.uPayload.sScanRspPayload.u8NwkUpdateId);
                sZllState.u8MyChannel = psEvent->sZllMessage.uPayload.sScanRspPayload.u8LogicalChannel;

                vEndCommissioning(pvNwk, E_IDLE, 0);

                vLog_Printf(TRACE_SCAN,LOG_DEBUG, "Update Id and rejoin\n");

                ZPS_eAplZdoRejoinNetwork(TRUE);
                return;
            }
        }
    }

    if ( ((sZllState.eState == FACTORY_NEW) &&
          ((psEvent->sZllMessage.uPayload.sScanRspPayload.u8ZigbeeInfo & ZLL_TYPE_MASK) == ZLL_ZED)) ||
         ( (sZllState.eState == NOT_FACTORY_NEW) &&
                 ((psEvent->sZllMessage.uPayload.sScanRspPayload.u8ZigbeeInfo & ZLL_TYPE_MASK) == ZLL_ZED) &&
                 !(psEvent->sZllMessage.uPayload.sScanRspPayload.u8ZllInfo & ZLL_FACTORY_NEW) &&
                 (psEvent->sZllMessage.uPayload.sScanRspPayload.u64ExtPanId != psNib->sPersist.u64ExtPanId))
    )
    {
        vLog_Printf(TRACE_SCAN,LOG_DEBUG, "Drop scan result\n");
    }
    else
    {
        if (u16AdjustedLqi > psScanTarget->u16LQI) {
            psScanTarget->u16LQI = u16AdjustedLqi;
            /*
             *  joining scenarios we are a ZED
             * 1) Any router,
             * 2) we are NFN then FN device
             * 3) we are NFN then any NFN ED that is not on our Pan
             */
            vLog_Printf(TRACE_SCAN,LOG_DEBUG, "Accept %d\n", psScanTarget->u16LQI);
            psScanTarget->sScanDetails.sSrcAddr = psEvent->sZllMessage.sSrcAddr;
            psScanTarget->sScanDetails.sScanRspPayload = psEvent->sZllMessage.uPayload.sScanRspPayload;
        }
    }
}



/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendStartRequest(void *pvNwk,
                               ZPS_tsNwkNib *psNib,
                               tsZllScanTarget *psScanTarget,
                               uint32 u32TransactionId )
{
    ZPS_tsInterPanAddress       sDstAddr;
    tsCLD_ZllCommission_NetworkStartReqCommandPayload           sNwkStartReqPayload;
    uint8 u8Seq;
    uint16 u16KeyMask;
    int i;

    if (psScanTarget->sScanDetails.sScanRspPayload.u8ZllInfo & ZLL_FACTORY_NEW) {
        vLog_Printf(TRACE_SCAN,LOG_DEBUG, "FN -> FN Router target\n");
        /* nwk start */
    } else {
        vLog_Printf(TRACE_SCAN,LOG_DEBUG, "FN -> NFN Router target\n");
        /* nwk start */
    }
    memset(&sNwkStartReqPayload, 0, sizeof(tsCLD_ZllCommission_NetworkStartReqCommandPayload));
    sNwkStartReqPayload.u32TransactionId = sCommission.u32TransactionId;
    // ext pan 0, target decides
#if FIX_CHANNEL
    sNwkStartReqPayload.u8LogicalChannel = DEFAULT_CHANNEL;
#endif
    u16KeyMask = ZLL_SUPPORTED_KEYS & psScanTarget->sScanDetails.sScanRspPayload.u16KeyMask;

    if (u16KeyMask & ZLL_MASTER_KEY_MASK) {
        sNwkStartReqPayload.u8KeyIndex = ZLL_MASTER_KEY_INDEX;
    } else if (u16KeyMask & ZLL_CERTIFICATION_KEY_MASK) {
        sNwkStartReqPayload.u8KeyIndex = ZLL_CERTIFICATION_KEY_INDEX;
    } else {
        sNwkStartReqPayload.u8KeyIndex = ZLL_TEST_KEY_INDEX;
    }

    for (i=0; i<16; i++) {
#if RAND_KEY
        psNib->sTbl.psSecMatSet[0].au8Key[i] = (uint8)(RND_u32GetRand256() & 0xFF);
#else
        psNib->sTbl.psSecMatSet[0].au8Key[i] = 0xbb;
#endif
    }
    psNib->sTbl.psSecMatSet[0].u8KeySeqNum = 0;
    memset(psNib->sTbl.pu32InFCSet, 0, (sizeof(uint32) * psNib->sTblSize.u16NtActv));
    psNib->sTbl.psSecMatSet[0].u8KeyType = ZPS_NWK_SEC_NETWORK_KEY;
    eEncryptKey(psNib->sTbl.psSecMatSet[0].au8Key,
            sNwkStartReqPayload.au8NwkKey,
            u32TransactionId,
            psScanTarget->sScanDetails.sScanRspPayload.u32ResponseId,
            sNwkStartReqPayload.u8KeyIndex);

    /* save security material to flash */
    ZPS_vNwkSaveSecMat( pvNwk);

    /* Make this the Active Key */
    ZPS_vNwkNibSetKeySeqNum( pvNwk, 0);

    // Pan Id 0 target decides

    // take our group address requirement
    vSetGroupAddress(sZllState.u16FreeGroupLow, GROUPS_REQUIRED);
    sZllState.u16FreeGroupLow += GROUPS_REQUIRED;

    sNwkStartReqPayload.u16InitiatorNwkAddr = sZllState.u16FreeAddrLow;
    sZllState.u16MyAddr = sZllState.u16FreeAddrLow++;
    sTarget.u16NwkAddr = sZllState.u16FreeAddrLow;
    sNwkStartReqPayload.u16NwkAddr = sZllState.u16FreeAddrLow++;
    sNwkStartReqPayload.u64InitiatorIEEEAddr = sDeviceTable.asDeviceRecords[0].u64IEEEAddr;

    sDstAddr.eMode = 3;
    sDstAddr.u16PanId = 0xffff;
    sDstAddr.uAddress.u64Addr = psScanTarget->sScanDetails.sSrcAddr.uAddress.u64Addr;

    ZPS_vNwkNibSetChannel( pvNwk, psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel);

    eCLD_ZllCommissionCommandNetworkStartReqCommandSend( &sDstAddr,
                                                         &u8Seq,
                                                         &sNwkStartReqPayload  );
}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendRouterJoinRequest(void *pvNwk,
                                    ZPS_tsNwkNib *psNib,
                                    tsZllScanTarget *psScanTarget,
                                    uint32 u32TransactionId)
{
    ZPS_tsInterPanAddress       sDstAddr;
    tsCLD_ZllCommission_NetworkJoinRouterReqCommandPayload      sNwkJoinRouterReqPayload;
    uint16 u16KeyMask;
    uint8 u8Seq;


    memset(&sNwkJoinRouterReqPayload, 0, sizeof(tsCLD_ZllCommission_NetworkJoinRouterReqCommandPayload));
    sNwkJoinRouterReqPayload.u32TransactionId = u32TransactionId;
    // ext pan 0, target decides
    u16KeyMask = ZLL_SUPPORTED_KEYS & psScanTarget->sScanDetails.sScanRspPayload.u16KeyMask;

    if (u16KeyMask & ZLL_MASTER_KEY_MASK) {
        sNwkJoinRouterReqPayload.u8KeyIndex = ZLL_MASTER_KEY_INDEX;
    } else if (u16KeyMask & ZLL_CERTIFICATION_KEY_MASK) {
        sNwkJoinRouterReqPayload.u8KeyIndex = ZLL_CERTIFICATION_KEY_INDEX;
    } else {
        sNwkJoinRouterReqPayload.u8KeyIndex = ZLL_TEST_KEY_INDEX;
    }

    eEncryptKey(psNib->sTbl.psSecMatSet[0].au8Key,
            sNwkJoinRouterReqPayload.au8NwkKey,
            u32TransactionId,
            psScanTarget->sScanDetails.sScanRspPayload.u32ResponseId,
            sNwkJoinRouterReqPayload.u8KeyIndex);



    sNwkJoinRouterReqPayload.u64ExtPanId = psNib->sPersist.u64ExtPanId;

    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "RJoin nwk epid %016llx\n", psNib->sPersist.u64ExtPanId);
    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "RJoin nwk epid %016llx\n",ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid);

    sNwkJoinRouterReqPayload.u8NwkUpdateId = psNib->sPersist.u8UpdateId;
    sNwkJoinRouterReqPayload.u8LogicalChannel = sZllState.u8MyChannel;
    sNwkJoinRouterReqPayload.u16PanId = psNib->sPersist.u16VsPanId;

    if (sZllState.u16FreeAddrLow == 0) {
        sTarget.u16NwkAddr = RND_u32GetRand(1, 0xfff6);
        sNwkJoinRouterReqPayload.u16NwkAddr = sTarget.u16NwkAddr;
    } else  {
        sTarget.u16NwkAddr = sZllState.u16FreeAddrLow;
        sNwkJoinRouterReqPayload.u16NwkAddr = sZllState.u16FreeAddrLow++;
        if (psScanTarget->sScanDetails.sScanRspPayload.u8ZllInfo & ZLL_ADDR_ASSIGN) {
            vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Assign addresses\n");

            sNwkJoinRouterReqPayload.u16FreeNwkAddrBegin = ((sZllState.u16FreeAddrLow+sZllState.u16FreeAddrHigh-1) >> 1);
            sNwkJoinRouterReqPayload.u16FreeNwkAddrEnd = sZllState.u16FreeAddrHigh;
            sZllState.u16FreeAddrHigh = sNwkJoinRouterReqPayload.u16FreeNwkAddrBegin - 1;

            sNwkJoinRouterReqPayload.u16FreeGroupIdBegin = ((sZllState.u16FreeGroupLow+sZllState.u16FreeGroupHigh-1) >> 1);
            sNwkJoinRouterReqPayload.u16FreeGroupIdEnd = sZllState.u16FreeGroupHigh;
            sZllState.u16FreeGroupHigh = sNwkJoinRouterReqPayload.u16FreeGroupIdBegin - 1;
        }
    }
    if (psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount) {
        /* allocate count group ids */
        sNwkJoinRouterReqPayload.u16GroupIdBegin = sZllState.u16FreeGroupLow;
        sNwkJoinRouterReqPayload.u16GroupIdEnd = sZllState.u16FreeGroupLow + psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount-1;
        sZllState.u16FreeGroupLow += psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount;
    }

    vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Give it addr %04x\n", sNwkJoinRouterReqPayload.u16NwkAddr);
    /* rest of params zero */
    ZPS_vNwkNibSetChannel( pvNwk, psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel);

    sDstAddr.eMode = 3;
    sDstAddr.u16PanId = 0xffff;
    sDstAddr.uAddress.u64Addr = psScanTarget->sScanDetails.sSrcAddr.uAddress.u64Addr;

    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Send rtr join on Ch %d for ch %d",psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel, sZllState.u8MyChannel);
    eCLD_ZllCommissionCommandNetworkJoinRouterReqCommandSend( &sDstAddr,
                                                             &u8Seq,
                                                             &sNwkJoinRouterReqPayload  );
}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendEndDeviceJoinRequest(void *pvNwk,
                                       ZPS_tsNwkNib *psNib,
                                       tsZllScanTarget *psScanTarget,
                                       uint32 u32TransactionId)
{
    ZPS_tsInterPanAddress       sDstAddr;
    tsCLD_ZllCommission_NetworkJoinEndDeviceReqCommandPayload      sNwkJoinEndDeviceReqPayload;
    uint16 u16KeyMask;
    uint8 u8Seq;

    memset(&sNwkJoinEndDeviceReqPayload, 0, sizeof(tsCLD_ZllCommission_NetworkJoinEndDeviceReqCommandPayload));
    sNwkJoinEndDeviceReqPayload.u32TransactionId = sCommission.u32TransactionId;

    u16KeyMask = ZLL_SUPPORTED_KEYS & psScanTarget->sScanDetails.sScanRspPayload.u16KeyMask;
    if (u16KeyMask & ZLL_MASTER_KEY_MASK) {
        sNwkJoinEndDeviceReqPayload.u8KeyIndex = ZLL_MASTER_KEY_INDEX;
    } else if (u16KeyMask & ZLL_CERTIFICATION_KEY_MASK) {
        sNwkJoinEndDeviceReqPayload.u8KeyIndex = ZLL_CERTIFICATION_KEY_INDEX;
    } else {
        sNwkJoinEndDeviceReqPayload.u8KeyIndex = ZLL_TEST_KEY_INDEX;
    }


    eEncryptKey(psNib->sTbl.psSecMatSet[0].au8Key,
                sNwkJoinEndDeviceReqPayload.au8NwkKey,
                u32TransactionId,
                psScanTarget->sScanDetails.sScanRspPayload.u32ResponseId,
                sNwkJoinEndDeviceReqPayload.u8KeyIndex);



    sNwkJoinEndDeviceReqPayload.u64ExtPanId = psNib->sPersist.u64ExtPanId;
    sNwkJoinEndDeviceReqPayload.u8NwkUpdateId = psNib->sPersist.u8UpdateId;
    sNwkJoinEndDeviceReqPayload.u8LogicalChannel = sZllState.u8MyChannel;
    sNwkJoinEndDeviceReqPayload.u16PanId = psNib->sPersist.u16VsPanId;
    if (sZllState.u16FreeAddrLow == 0) {
        sTarget.u16NwkAddr = RND_u32GetRand(1, 0xfff6);;
        sNwkJoinEndDeviceReqPayload.u16NwkAddr = sTarget.u16NwkAddr;
    } else  {
        sTarget.u16NwkAddr = sZllState.u16FreeAddrLow;
        sNwkJoinEndDeviceReqPayload.u16NwkAddr = sZllState.u16FreeAddrLow++;
        if (psScanTarget->sScanDetails.sScanRspPayload.u8ZllInfo & ZLL_ADDR_ASSIGN) {
            vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Assign addresses\n");

            sNwkJoinEndDeviceReqPayload.u16FreeNwkAddrBegin = ((sZllState.u16FreeAddrLow+sZllState.u16FreeAddrHigh-1) >> 1);
            sNwkJoinEndDeviceReqPayload.u16FreeNwkAddrEnd = sZllState.u16FreeAddrHigh;
            sZllState.u16FreeAddrHigh = sNwkJoinEndDeviceReqPayload.u16FreeNwkAddrBegin - 1;

            sNwkJoinEndDeviceReqPayload.u16FreeGroupIdBegin = ((sZllState.u16FreeGroupLow+sZllState.u16FreeGroupHigh-1) >> 1);
            sNwkJoinEndDeviceReqPayload.u16FreeGroupIdEnd = sZllState.u16FreeGroupHigh;
            sZllState.u16FreeGroupHigh = sNwkJoinEndDeviceReqPayload.u16FreeGroupIdBegin - 1;
        }
    }
    vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Give it addr %04x\n", sNwkJoinEndDeviceReqPayload.u16NwkAddr);
    if (psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount) {
        /* allocate count group ids */
        sNwkJoinEndDeviceReqPayload.u16GroupIdBegin = sZllState.u16FreeGroupLow;
        sNwkJoinEndDeviceReqPayload.u16GroupIdEnd = sZllState.u16FreeGroupLow + psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount-1;
        sZllState.u16FreeGroupLow += psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount;
    }

    ZPS_vNwkNibSetChannel( pvNwk, psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel);

    sDstAddr.eMode = 3;
    sDstAddr.u16PanId = 0xffff;
    sDstAddr.uAddress.u64Addr = psScanTarget->sScanDetails.sSrcAddr.uAddress.u64Addr;

    vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Send zed join on Ch %d for ch %d\n", psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel,
                                                                    sZllState.u8MyChannel);
    eCLD_ZllCommissionCommandNetworkJoinEndDeviceReqCommandSend( &sDstAddr,
                                                                 &u8Seq,
                                                                 &sNwkJoinEndDeviceReqPayload  );
}


/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleNwkUpdateRequest(void * pvNwk,
                                     ZPS_tsNwkNib *psNib,
                                     tsCLD_ZllCommission_NetworkUpdateReqCommandPayload *psNwkUpdateReqPayload)
{
    if ((psNwkUpdateReqPayload->u64ExtPanId == psNib->sPersist.u64ExtPanId) &&
         (psNwkUpdateReqPayload->u16PanId == psNib->sPersist.u16VsPanId) &&
          (psNib->sPersist.u16VsPanId != u8NewUpdateID(psNib->sPersist.u16VsPanId, psNwkUpdateReqPayload->u16PanId) ))
    {
        /* Update the UpdateId, Nwk addr and Channel */
        ZPS_vNwkNibSetUpdateId( pvNwk, psNwkUpdateReqPayload->u8NwkUpdateId);
        ZPS_vNwkNibSetNwkAddr( pvNwk,  psNwkUpdateReqPayload->u16NwkAddr);
        ZPS_vNwkNibSetChannel( pvNwk,  psNwkUpdateReqPayload->u8LogicalChannel);
    }
}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleIdentiftRequest(uint16 u16Duration)
{
    OS_eStopSWTimer(APP_IdTimer);
    if(u16Duration == 0)
    {
        APP_vSetLeds(FALSE, FALSE);
    }
    else
    {
        if(u16Duration == 0xFFFF)
        {
            u16Duration = 5; /* Random magic number */
        }
        APP_vSetLeds(TRACE_COMMISSION, TRUE);
        OS_eStartSWTimer(APP_IdTimer, APP_TIME_SEC(u16Duration), NULL);
    }
}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleDeviceInfoRequest(APP_CommissionEvent *psEvent)
{
    tsCLD_ZllCommission_DeviceInfoRspCommandPayload sDeviceInfoRspPayload;
    ZPS_tsInterPanAddress       sDstAddr;
    int i, j;
    uint8 u8Seq;

    vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Device Info request\n");
    memset(&sDeviceInfoRspPayload, 0, sizeof(tsCLD_ZllCommission_DeviceInfoRspCommandPayload));
    sDeviceInfoRspPayload.u32TransactionId = psEvent->sZllMessage.uPayload.sDeviceInfoReqPayload.u32TransactionId;
    sDeviceInfoRspPayload.u8NumberSubDevices = ZLL_NUMBER_DEVICES;
    sDeviceInfoRspPayload.u8StartIndex = psEvent->sZllMessage.uPayload.sDeviceInfoReqPayload.u8StartIndex;
    sDeviceInfoRspPayload.u8DeviceInfoRecordCount = 0;
    // copy from table sDeviceTable

    j = psEvent->sZllMessage.uPayload.sDeviceInfoReqPayload.u8StartIndex;
    for (i=0; i<16 && j<ZLL_NUMBER_DEVICES; i++, j++)
    {
        sDeviceInfoRspPayload.asDeviceRecords[i] = sDeviceTable.asDeviceRecords[j];
        sDeviceInfoRspPayload.u8DeviceInfoRecordCount++;
    }

    sDstAddr = psEvent->sZllMessage.sSrcAddr;
    sDstAddr.u16PanId = 0xffff;

    eCLD_ZllCommissionCommandDeviceInfoRspCommandSend( &sDstAddr,
                                                       &u8Seq,
                                                       &sDeviceInfoRspPayload);
}


/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleEndDeviceJoinRequest( void * pvNwk,
                                          ZPS_tsNwkNib *psNib,
                                          APP_CommissionEvent *psEvent,
                                          tsCommissionData *psCommission)
{
    tsCLD_ZllCommission_NetworkJoinEndDeviceRspCommandPayload   sNwkJoinEndDeviceRspPayload;
    ZPS_tsInterPanAddress       sDstAddr;
    uint8 u8Seq;

    sDstAddr = psEvent->sZllMessage.sSrcAddr;
    sDstAddr.u16PanId = 0xffff;


    vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Join End Device Req\n");
    sNwkJoinEndDeviceRspPayload.u32TransactionId = psCommission->u32TransactionId;
    // we are a router so reject
    sNwkJoinEndDeviceRspPayload.u8Status = ZLL_ERROR;
    eCLD_ZllCommissionCommandNetworkJoinEndDeviceRspCommandSend( &sDstAddr,
                                                                 &u8Seq,
                                                                 &sNwkJoinEndDeviceRspPayload);
    // wait for transaction time out

}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleNwkStartResponse(void * pvNwk,
                                     APP_CommissionEvent *psEvent,
                                     tsCommissionData *psCommission)
{
    if (psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u8Status == ZLL_SUCCESS)
    {
        ZPS_vNwkNibSetChannel( pvNwk, psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u8LogicalChannel);
        sZllState.u8MyChannel = psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u8LogicalChannel;
        /* Set channel maskl to primary channels */
        ZPS_vNwkNibSetExtPanId(pvNwk, psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u64ExtPanId);
        ZPS_eAplAibSetApsUseExtendedPanId(psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u64ExtPanId);
        ZPS_vNwkNibSetNwkAddr( pvNwk, sZllState.u16MyAddr);
        ZPS_vNwkNibSetUpdateId(pvNwk, psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u8NwkUpdateId);
        ZPS_vNwkNibSetPanId(pvNwk, psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u16PanId);
        OS_eStopSWTimer(APP_CommissionTimer);
        OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(ZLL_START_UP_TIME_SEC), NULL);
        vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Start on Ch %d\n", sZllState.u8MyChannel);
        psCommission->eState = E_WAIT_START_UP;
    }
}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleNwkStartRequest( ZPS_tsInterPanAddress  *psDstAddr,
                                     APP_CommissionEvent *psEvent,
                                     tsCommissionData *psCommission)
{
    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "start request\n");

    if (psCommission->u32TransactionId == psEvent->sZllMessage.uPayload.sNwkStartReqPayload.u32TransactionId) {

        sStartParams = psEvent->sZllMessage.uPayload.sNwkStartReqPayload;

        if (sZllState.eState == FACTORY_NEW) {
            /* factory new so will start a network */
            /* Turn up Tx power */

            eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_NORMAL);

            if ((sStartParams.u64ExtPanId == 0) || (sStartParams.u16PanId == 0))
            {
                vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Generate Pans\n");
                if (sStartParams.u64ExtPanId == 0)
                {
                    ZPS_tsAplAib *psAib = ZPS_psAplAibGetAib();
                    if(psAib->u64ApsUseExtendedPanid == 0)
                    {
                        sStartParams.u64ExtPanId  = RND_u32GetRand(1, 0xffffffff);
                        sStartParams.u64ExtPanId <<= 32;
                        sStartParams.u64ExtPanId  |= RND_u32GetRand(0, 0xffffffff);
                        vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Gen Epid\n");
                    }
                    else
                    {
                        sStartParams.u64ExtPanId = psAib->u64ApsUseExtendedPanid;
                    }
                }
                if (sStartParams.u16PanId == 0)
                {
                    sStartParams.u16PanId = RND_u32GetRand( 1, 0xfffe);
                    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Gen pan\n");
                }
                vLog_Printf((TRACE_COMMISSION|TRUE),LOG_DEBUG, "Do discovery\n");
                vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Disc=%02x\n",
                ZPS_eAplZdoDiscoverNetworks( ZLL_CHANNEL_MASK) );
                psCommission->eState = E_WAIT_DISCOVERY;
            }
            else
            {
                psCommission->eState = E_SKIP_DISCOVERY;
                vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Skip discovery\n");
                OS_eStopSWTimer(APP_CommissionTimer);
                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
            }
        } else {
            /* not factory new, gateway so refuse to be stolen */
            tsCLD_ZllCommission_NetworkStartRspCommandPayload sNwkStartRsp;
            uint8 u8Seq;
            memset( &sNwkStartRsp, 0, sizeof(tsCLD_ZllCommission_NetworkStartRspCommandPayload));

            sNwkStartRsp.u32TransactionId = sCommission.u32TransactionId;
            sNwkStartRsp.u8Status = ZLL_ERROR;

           eCLD_ZllCommissionCommandNetworkStartRspCommandSend(
                   psDstAddr,
                   &u8Seq,
                   &sNwkStartRsp);
        }
    }


}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleRouterJoinRequest( void * pvNwk,
        ZPS_tsNwkNib *psNib,
        APP_CommissionEvent *psEvent,
        tsCommissionData *psCommission)
{
    tsCLD_ZllCommission_NetworkJoinRouterRspCommandPayload      sNwkJoinRouterRspPayload;
    tsCLD_ZllCommission_NetworkJoinRouterReqCommandPayload*      psNwkJoinRouterPayload;
    ZPS_tsInterPanAddress       sDstAddr;
    uint8  u8Seq;

    sDstAddr = psEvent->sZllMessage.sSrcAddr;
    sDstAddr.u16PanId = 0xffff;

    psNwkJoinRouterPayload = &psEvent->sZllMessage.uPayload.sNwkJoinRouterReqPayload;

    sNwkJoinRouterRspPayload.u32TransactionId = psCommission->u32TransactionId;
    vLog_Printf(TRACE_JOIN,LOG_DEBUG, "Join Router Req\n");
    sNwkJoinRouterRspPayload.u32TransactionId = psCommission->u32TransactionId;
    // return error if NFN (not supported), else success
    sNwkJoinRouterRspPayload.u8Status = (sZllState.eState == NOT_FACTORY_NEW)? ZLL_ERROR: ZLL_SUCCESS;
    eCLD_ZllCommissionCommandNetworkJoinRouterRspCommandSend( &sDstAddr,
                                                              &u8Seq,
                                                              &sNwkJoinRouterRspPayload);

    if (sZllState.eState == FACTORY_NEW) {
        /* security stuff */

       eDecryptKey( psNwkJoinRouterPayload->au8NwkKey,
                   psNib->sTbl.psSecMatSet[0].au8Key,
                   psCommission->u32TransactionId,
                   psCommission->u32ResponseId,
                   psNwkJoinRouterPayload->u8KeyIndex);

       psNib->sTbl.psSecMatSet[0].u8KeySeqNum = 0;
       memset(psNib->sTbl.pu32InFCSet, 0, (sizeof(uint32)*psNib->sTblSize.u16NtActv));
       psNib->sTbl.psSecMatSet[0].u8KeyType = ZPS_NWK_SEC_NETWORK_KEY;

       /* save security material to flash */

       ZPS_vNwkSaveSecMat( pvNwk);

       /* Make this the Active Key */
       ZPS_vNwkNibSetKeySeqNum( pvNwk, 0);

       /* Set params */
       ZPS_vNwkNibSetChannel( pvNwk, psNwkJoinRouterPayload->u8LogicalChannel);
       sZllState.u8MyChannel = psNwkJoinRouterPayload->u8LogicalChannel;
       ZPS_vNwkNibSetPanId( pvNwk, psNwkJoinRouterPayload->u16PanId);
       ZPS_eAplAibSetApsUseExtendedPanId(psNwkJoinRouterPayload->u64ExtPanId);
       sZllState.u16MyAddr = psNwkJoinRouterPayload->u16NwkAddr;
       ZPS_vNwkNibSetNwkAddr( pvNwk, sZllState.u16MyAddr);

       sZllState.u16FreeAddrLow = psNwkJoinRouterPayload->u16FreeNwkAddrBegin;
       sZllState.u16FreeAddrHigh = psNwkJoinRouterPayload->u16FreeNwkAddrEnd;
       sZllState.u16FreeGroupLow = psNwkJoinRouterPayload->u16FreeGroupIdBegin;
       sZllState.u16FreeGroupHigh = psNwkJoinRouterPayload->u16FreeGroupIdEnd;
       // take our group addr requirement
       vSetGroupAddress( psNwkJoinRouterPayload->u16GroupIdBegin, GROUPS_REQUIRED);

       ZPS_vNwkNibSetUpdateId( pvNwk, psNwkJoinRouterPayload->u8NwkUpdateId);

       OS_eStopSWTimer(APP_CommissionTimer);
       OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
       psCommission->eState = E_WAIT_START_UP;
    }
}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendIdentifyRequest(uint64 u64Addr, uint32 u32TransactionId,uint8 u8Time)
{
    ZPS_tsInterPanAddress  sDstAddr;
    tsCLD_ZllCommission_IdentifyReqCommandPayload               sIdentifyReqPayload;
    uint8 u8Seq;

    sDstAddr.eMode = ZPS_E_AM_INTERPAN_IEEE;
    sDstAddr.u16PanId = 0xffff;
    sDstAddr.uAddress.u64Addr = u64Addr;
    sIdentifyReqPayload.u32TransactionId = sCommission.u32TransactionId;
    sIdentifyReqPayload.u16Duration =3;

    eCLD_ZllCommissionCommandDeviceIndentifyReqCommandSend(
                                        &sDstAddr,
                                        &u8Seq,
                                        &sIdentifyReqPayload);
}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_Commission_Task)
{
    /*
     * Commissioning task now in overlay (above)
     */
    vCommissionTask();

}


/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_CommissionTimerTask)
{
    APP_CommissionEvent sEvent;
    sEvent.eType = APP_E_COMMISSION_TIMER_EXPIRED;
    OS_ePostMessage(APP_CommissionEvents, &sEvent);
}

/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_ID_Task)
{
    /* interpan id time out, shut down */
    OS_eStopSWTimer(APP_IdTimer);
    if(bResetIssued)
    {
        vAHI_SwReset();
        bResetIssued = FALSE;
    }
    APP_vSetLeds(FALSE, FALSE);
}

/****************************************************************************
 *
 * NAME: eEncryptKey
 *
 * DESCRIPTION:
 * Encrypt the nwk key use AES ECB
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE uint8 eEncryptKey(uint8* au8InData,
                    uint8* au8OutData,
                    uint32 u32TransId,
                    uint32 u32ResponseId,
                    uint8 u8KeyIndex)
{


    tsReg128 sExpanded;
    tsReg128 sTransportKey,sInData,sOutData;

    sExpanded.u32register0 = u32TransId;
    sExpanded.u32register1 = u32TransId;
    sExpanded.u32register2 = u32ResponseId;
    sExpanded.u32register3 = u32ResponseId;

    switch (u8KeyIndex)
    {
        case ZLL_TEST_KEY_INDEX:
            sTransportKey.u32register0 = 0x50684c69;
            sTransportKey.u32register1 = u32TransId;
            sTransportKey.u32register2 = 0x434c534e;
            sTransportKey.u32register3 = u32ResponseId;
            break;
        case ZLL_MASTER_KEY_INDEX:
            bACI_ECBencodeStripe( &sMasterKey,
                                  TRUE,
                                  &sExpanded,
                                  &sTransportKey);
            break;
        case ZLL_CERTIFICATION_KEY_INDEX:
            bACI_ECBencodeStripe( &sCertKey,
                                  TRUE,
                                  &sExpanded,
                                  &sTransportKey);
            break;

        default:
            return 3;
            break;
    }

    memcpy(&sInData,au8InData, sizeof(tsReg128));
    memcpy(&sOutData,au8OutData, sizeof(tsReg128));
    bACI_ECBencodeStripe( &sTransportKey   ,
                          TRUE,
                          &sInData,
                          &sOutData);

    return 0;
}

/****************************************************************************
 *
 * NAME: eDecryptKey
 *
 * DESCRIPTION:
 * Decrypt the nwk key use AES ECB
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE uint8 eDecryptKey(uint8* au8InData,
                    uint8* au8OutData,
                    uint32 u32TransId,
                    uint32 u32ResponseId,
                    uint8 u8KeyIndex)
{
    tsReg128 sTransportKey;
    tsReg128 sExpanded;

    sExpanded.u32register0 = u32TransId;
    sExpanded.u32register1 = u32TransId;
    sExpanded.u32register2 = u32ResponseId;
    sExpanded.u32register3 = u32ResponseId;

    switch (u8KeyIndex)
    {
        case ZLL_TEST_KEY_INDEX:
            sTransportKey.u32register0 = 0x50684c69;
            sTransportKey.u32register1 = u32TransId;
            sTransportKey.u32register2 = 0x434c534e;
            sTransportKey.u32register3 = u32ResponseId;
            break;
        case ZLL_MASTER_KEY_INDEX:
            bACI_ECBencodeStripe( &sMasterKey,
                                  TRUE,
                                  &sExpanded,
                                  &sTransportKey);
            break;
        case ZLL_CERTIFICATION_KEY_INDEX:
            bACI_ECBencodeStripe( &sCertKey,
                                  TRUE,
                                  &sExpanded,
                                  &sTransportKey);
            break;

        default:
            vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "***Ooops***\n");
            return 3;
            break;
    }

    vECB_Decrypt( (uint8*)&sTransportKey,
                  au8InData,
                  au8OutData);
#if SHOW_KEY
    int i;
    for (i=0; i<16; i++) {
        vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "%02x ", au8OutData[i]);
    }
    vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "\n");
#endif

    return 0;
}

PRIVATE bool bSearchDiscNt(ZPS_tsNwkNib *psNib, uint64 u64EpId, uint16 u16PanId) {

    int i;
    for (i = 0; i < psNib->sTblSize.u8NtDisc; i++)
    {
        if ((psNib->sTbl.psNtDisc[i].u64ExtPanId == u64EpId)
                || (psNib->sTbl.psNtDisc[i].u16PanId == u16PanId))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/*
 * Determine which of 2 nwk ids is the freshett
 */
PRIVATE uint8 u8NewUpdateID(uint8 u8ID1, uint8 u8ID2 )
{
    if ( (abs(u8ID1-u8ID2)) > 200) {
        return MIN(u8ID1, u8ID2);
    }
    return MAX(u8ID1, u8ID2);
}


/****************************************************************************
 *
 * NAME:
 *
 * DESCRIPTION:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendFactoryResetRequest(void *pvNwk,
                                      ZPS_tsNwkNib *psNib,
                                      tsZllScanTarget *psScanTarget,
                                      uint32 u32TransactionId)
{
    ZPS_tsInterPanAddress       sDstAddr;
    tsCLD_ZllCommission_FactoryResetReqCommandPayload sFacRstPayload;
    uint8 u8Seq;
    memset(&sFacRstPayload, 0, sizeof(tsCLD_ZllCommission_FactoryResetReqCommandPayload));
    sDstAddr.eMode = 3;
    sDstAddr.u16PanId = 0xffff;
    sDstAddr.uAddress.u64Addr = psScanTarget->sScanDetails.sSrcAddr.uAddress.u64Addr;

    ZPS_vNwkNibSetChannel( pvNwk, psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel);
    sFacRstPayload.u32TransactionId= psScanTarget->sScanDetails.sScanRspPayload.u32TransactionId;
    eCLD_ZllCommissionCommandFactoryResetReqCommandSend( &sDstAddr,
                                                         &u8Seq,
                                                         &sFacRstPayload  );
}


/****************************************************************************
 *
 * NAME: APP_vSetLeds
 *
 * DESCRIPTION:
 * Set LED State
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vSetLeds(bool bLed1On, bool bLed2On)
{

    if(bLed1On)
    {
        vLog_Printf(TRACE_COMMISSION, LOG_DEBUG,"LED1_ON");
        u32LEDs |= LED1;
    }
    else
    {
        u32LEDs &= ~LED1;
    }

    if(bLed2On)
    {
        vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "LED2_ON");
        u32LEDs |= LED2;
    }
    else
    {
        u32LEDs &= ~LED2;
    }

    vAHI_DioSetOutput(((~u32LEDs) & (LED1|LED2)), u32LEDs);

}


/****************************************************************************
 *
 * NAME: vSetGroupAddress
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSetGroupAddress(uint16 u16GroupStart, uint8 u8NumGroups) {
    int i;

    /* This passes all the required group addresses for the device
     * if the are morethan one sub devices (endpoints) they need
     * sharing amoung the endpoints
     * In this case there is one the 1 Rc endpoint, so all group addresses
     * are for it
     */
    for (i=0; i<NUM_GROUP_RECORDS && i<u8NumGroups; i++) {
        sGroupTable.asGroupRecords[i].u16GroupId = u16GroupStart++;
        sGroupTable.asGroupRecords[i].u8GroupType = 0;
        vLog_Printf(TRACE_COMMISSION,LOG_DEBUG, "Idx %d Grp %04x\n", i, sGroupTable.asGroupRecords[i].u16GroupId);
    }
    sGroupTable.u8NumRecords = u8NumGroups;

    PDM_eSaveRecordData( PDM_ID_APP_GROUP_TABLE,&sGroupTable,sizeof(tsZllGroupInfoTable));

}

/****************************************************************************
 *
 * NAME: psGetEndpointRecordTable
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC tsZllEndpointInfoTable * psGetEndpointRecordTable(void)
{
    return &sEndpointTable;
}

/****************************************************************************
 *
 * NAME: psGetGroupRecordTable
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC tsZllGroupInfoTable * psGetGroupRecordTable(void)
{
    return &sGroupTable;
}
/****************************************************************************
 *
 * NAME: vStartAndAnnounce
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStartAndAnnounce( void) {


    uint8 u8Seq;
    void * pvNwk = ZPS_pvAplZdoGetNwkHandle();

    /* Start router */
    ZPS_eAplZdoZllStartRouter();

    PDUM_thAPduInstance hAPduInst = PDUM_hAPduAllocateAPduInstance( apduControl);
    if (hAPduInst != NULL)
    {
        ZPS_tsAplZdpDeviceAnnceReq sZdpDeviceAnnceReq;
        sZdpDeviceAnnceReq.u16NwkAddr = ZPS_u16NwkNibGetNwkAddr(pvNwk);
        sZdpDeviceAnnceReq.u64IeeeAddr = ZPS_u64NwkNibGetExtAddr(pvNwk);
        sZdpDeviceAnnceReq.u8Capability = ZPS_eAplZdoGetMacCapability();

        ZPS_eAplZdpDeviceAnnceRequest(hAPduInst, &u8Seq, &sZdpDeviceAnnceReq);
        vLog_Printf(TRACE_JOIN,LOG_DEBUG, " Dev Annce Seq No = %d\n", u8Seq);
    }
    if(sDeviceDesc.u8DeviceType >= 2)
    {
        ZPS_vTCSetCallback(bSendHATransportKey);
    }
}
/****************************************************************************
 *
 * NAME: vSendDeviceInfoReq
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendDeviceInfoReq(uint64 u64Addr, uint32 u32TransactionId) {
    ZPS_tsInterPanAddress       sDstAddr;
    uint8 u8Seq;
    tsCLD_ZllCommission_DeviceInfoReqCommandPayload             sDeviceInfoReqPayload;

    sDstAddr.eMode = ZPS_E_AM_INTERPAN_IEEE;
    sDstAddr.u16PanId = 0xffff;
    sDstAddr.uAddress.u64Addr = u64Addr;

    sDeviceInfoReqPayload.u32TransactionId = u32TransactionId;
    sDeviceInfoReqPayload.u8StartIndex = 0;
    eCLD_ZllCommissionCommandDeviceInfoReqCommandSend( &sDstAddr,
                                                       &u8Seq,
                                                       &sDeviceInfoReqPayload );
}
