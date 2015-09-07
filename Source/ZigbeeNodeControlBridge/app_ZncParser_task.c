/*****************************************************************************
 *
 * MODULE: ZigbeeNodeControlBridge
 *
 * COMPONENT: app_ZncParser_task.c
 *
 * $AUTHOR: Faisal Bhaiyat$
 *
 * DESCRIPTION:
 *
 * $HeadURL: https://www.collabnet.nxp.com/svn/lprf_sware/Projects/Zigbee%20Protocol%20Stack/ZPS/Trunk/ZigbeeNodeControlBridge/Source/ZigbeeNodeControlBridge/app_ZncParser_task.c $
 *
 * $Revision: 55059 $
 *
 * $LastChangedBy: nxp29741 $
 *
 * $LastChangedDate: 2013-07-01 17:27:09 +0100 (Mon, 01 Jul 2013) $
 *
 * $Id: app_ZncParser_task.c 55059 2013-07-01 16:27:09Z nxp29741 $
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

#include "jendefs.h"
#include "string.h"
#include "os.h"
#include "pdum_apl.h"
#include "os_gen.h"
#include "dbg.h"
#include "pdm.h"
#include "pdum_gen.h"
#include "zps_gen.h"
#include "zps_apl.h"
#include "zps_nwk_nib.h"
#include "zps_apl_aib.h"
#include "zps_apl_zdo.h"
#include "zps_apl_af.h"
#include "app_Znc_cmds.h"
#include "app_ZncParser_task.h"
#include "app_Znc_zcltask.h"
#include "SerialLink.h"
#include "pwrm.h"
#include "uart.h"
#include "commission_endpoint.h"
#include "app_timer_driver.h"
#include "app_common.h"
#include "app_events.h"
#include "rnd_pub.h"
#include "Log.h"
#include "mac_sap.h"
#include "PDM_IDs.h"
#include "appZdpExtraction.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef TRACE_APP
#define TRACE_APP TRUE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vSecondScan( void * pvNwk, bool * pbSecondDisc);
PRIVATE void vPickChannel( void *pvNwk);
PUBLIC bool bAddToEndpointTable(APP_tsEventTouchLink *psEndpointData);
PUBLIC void vAppIdentifyEffect( teCLD_Identify_EffectId eEffect);
PRIVATE uint8 u8SearchEndpointTable(APP_tsEventTouchLink *psEndpointData, uint8* pu8Index);
extern void *ZPS_pvNwkSecGetNetworkKey(void *pvNwk);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

PUBLIC uint64 g_u64AssociationEPid = 0;

tsCLD_ZllDeviceTable sDeviceTable = { ZLL_NUMBER_DEVICES,
                                       {
                                          { 0,
                                            ZLL_PROFILE_ID,
                                            CONTROL_BRIDGE_DEVICE_ID,
                                            ZIGBEENODECONTROLBRIDGE_HA_ENDPOINT,
                                            2,
                                            GROUPS_REQUIRED,
                                            0
                                          }
                                       }
};
PRIVATE bool_t bAddrMode = FALSE;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
int16 s_i16RxByte;
extern tsLedState s_sLedState;
extern uint8 u8JoinedDevice;
/****************************************************************************/
/***        Exported Public Functions                                     ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Private Functions 	                                    */
/****************************************************************************/


/***************************************************************************/
/***          Local Private Functions                                      */
/***************************************************************************/


PRIVATE void Znc_vWrite64Nbo(uint64 u64dWord, uint8 *pu8Payload);

/****************************************************************************
 *
 * NAME: APP_taskAtParser
 *
 * DESCRIPTION:
 *
 *
 ****************************************************************************/
OS_TASK(APP_taskAtParser)
{
    uint8 u8RxByte;
    s_i16RxByte = -1;

    if (OS_E_OK == OS_eCollectMessage(APP_msgSerialRx, &u8RxByte)) {
        s_i16RxByte = u8RxByte;
    }
    vProcessIncomingSerialCommands();
    vAHI_WatchdogRestart();

}

/****************************************************************************
 *
 * NAME: APP_taskZncb
 *
 * DESCRIPTION:
 *
 *
 ****************************************************************************/
OS_TASK(APP_taskZncb)
{
    ZPS_tsAfEvent sStackEvent;
    APP_tsEvent sAppEvent;
    APP_CommissionEvent sCommissionEvent;
    ZPS_tsNwkNetworkDescr *psNwkDescr;
    uint16 u16Length = 0;
    uint8 u8Status,u8SeqNo = 0xaf;
    uint8 au8StatusBuffer[256];
    uint8* pu8Buffer;
    ZPS_tsNwkNib *psNib = ZPS_psAplZdoGetNib();
    static bool bSecondDisc = FALSE;

    sStackEvent.eType = ZPS_EVENT_NONE;
    sAppEvent.eType = APP_E_EVENT_NONE;


    if (OS_E_OK == OS_eCollectMessage(APP_msgZpsEvents, &sStackEvent)) {
        switch (sStackEvent.eType) {
            case ZPS_EVENT_APS_DATA_INDICATION:
            {
                uint8 *dataPtr = (uint8*)PDUM_pvAPduInstanceGetPayload(sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);
                uint8 u8Size = PDUM_u16APduInstanceGetPayloadSize( sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);

                pu8Buffer = au8StatusBuffer;

                if(sStackEvent.uEvent.sApsDataIndEvent.u8SrcEndpoint != 0 &&
                        sStackEvent.uEvent.sApsDataIndEvent.u8DstEndpoint != 0)
                {
                    memcpy(pu8Buffer,&sStackEvent.uEvent.sApsDataIndEvent.eStatus,sizeof(uint8) );
                    u16Length = sizeof(uint8);
                    pu8Buffer += sizeof(uint8);
                    memcpy(pu8Buffer,&sStackEvent.uEvent.sApsDataIndEvent.u16ProfileId,sizeof(uint16) );
                    u16Length += sizeof(uint16);
                    pu8Buffer += sizeof(uint16);
                    memcpy(pu8Buffer,&sStackEvent.uEvent.sApsDataIndEvent.u16ClusterId,sizeof(uint16) );
                    u16Length += sizeof(uint16);
                    pu8Buffer += sizeof(uint16);
                    memcpy(pu8Buffer,&sStackEvent.uEvent.sApsDataIndEvent.u8SrcEndpoint,sizeof(uint8) );
                    u16Length += sizeof(uint8);
                    pu8Buffer += sizeof(uint8);
                    memcpy(pu8Buffer,&sStackEvent.uEvent.sApsDataIndEvent.u8DstEndpoint,sizeof(uint8) );
                    u16Length += sizeof(uint8);
                    pu8Buffer += sizeof(uint8);
                    memcpy(pu8Buffer,&sStackEvent.uEvent.sApsDataIndEvent.u8SrcAddrMode,sizeof(uint8) );
                    u16Length += sizeof(uint8);
                    pu8Buffer += sizeof(uint8);
                    if(sStackEvent.uEvent.sApsDataIndEvent.u8SrcAddrMode == ZPS_E_ADDR_MODE_IEEE)
                    {
                        memcpy(pu8Buffer,&sStackEvent.uEvent.sApsDataIndEvent.uSrcAddress.u64Addr,sizeof(uint64) );
                        u16Length += sizeof(uint64);
                        pu8Buffer += sizeof(uint64);
                    }
                    else
                    {
                        memcpy(pu8Buffer,&sStackEvent.uEvent.sApsDataIndEvent.uSrcAddress.u16Addr,sizeof(uint16) );
                        u16Length += sizeof(uint16);
                        pu8Buffer += sizeof(uint16);
                    }

                    memcpy(pu8Buffer,&sStackEvent.uEvent.sApsDataIndEvent.u8DstAddrMode,sizeof(uint8) );
                    u16Length += sizeof(uint8);
                    pu8Buffer += sizeof(uint8);
                    if(sStackEvent.uEvent.sApsDataIndEvent.u8DstAddrMode == ZPS_E_ADDR_MODE_IEEE)
                    {
                        memcpy(pu8Buffer,&sStackEvent.uEvent.sApsDataIndEvent.uDstAddress.u64Addr,sizeof(uint64) );
                        u16Length += sizeof(uint64);
                        pu8Buffer += sizeof(uint64);
                    }
                    else
                    {
                        memcpy(pu8Buffer,&sStackEvent.uEvent.sApsDataIndEvent.uDstAddress   .u16Addr,sizeof(uint16) );
                        u16Length += sizeof(uint16);
                        pu8Buffer += sizeof(uint16);
                    }

                    memcpy(pu8Buffer,dataPtr,(sizeof(uint8)*u8Size) );
                    u16Length += (sizeof(uint8)*u8Size);
                    vSL_WriteMessage(E_SL_MSG_DATA_INDICATION, u16Length, au8StatusBuffer);
                    vLog_Printf(TRACE_APP,LOG_ERR, "NPDU: Current %d Max %d\n", PDUM_u8GetNpduUse(), PDUM_u8GetMaxNpduUse());
                    vLog_Printf(TRACE_APP,LOG_ERR, "APDU: Current %d Max %d\n", u8GetApduUse(), u8GetMaxApdu());

                }
                else
                {
                    ZPS_tsAfZdpEvent sApsZdpEvent;
                    zps_bAplZdpUnpackResponse(&sStackEvent,&sApsZdpEvent);
                    memcpy(pu8Buffer,&sApsZdpEvent.u8SequNumber,sizeof(uint8) );
                    switch(sApsZdpEvent.u16ClusterId)
                    {
                    case 0x0013:
                    {
     /* Not pushing the sequence number */
                         memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sDeviceAnnce.u16NwkAddr,sizeof(uint16) );
                         u16Length = sizeof(uint16);
                         pu8Buffer += sizeof(uint16);
                         memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sDeviceAnnce.u64IeeeAddr,sizeof(uint64) );
                         u16Length += sizeof(uint64);
                         pu8Buffer += sizeof(uint64);
                         memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sDeviceAnnce.u8Capability,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         vSL_WriteMessage(E_SL_MSG_DEVICE_ANNOUNCE, u16Length, au8StatusBuffer);
                    }
                        break;

                    case 0x8002:
                    {
                         u16Length = sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sNodeDescRsp.u8Status,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sNodeDescRsp.u16NwkAddrOfInterest,sizeof(uint16) );
                         u16Length += sizeof(uint16);
                         pu8Buffer += sizeof(uint16);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sNodeDescRsp.sNodeDescriptor.u16ManufacturerCode,sizeof(uint16) );
                         u16Length += sizeof(uint16);
                         pu8Buffer += sizeof(uint16);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sNodeDescRsp.sNodeDescriptor.u16MaxRxSize,sizeof(uint16) );
                         u16Length += sizeof(uint16);
                         pu8Buffer += sizeof(uint16);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sNodeDescRsp.sNodeDescriptor.u16MaxTxSize,sizeof(uint16) );
                         u16Length += sizeof(uint16);
                         pu8Buffer += sizeof(uint16);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sNodeDescRsp.sNodeDescriptor.u16ServerMask,sizeof(uint16) );
                         u16Length += sizeof(uint16);
                         pu8Buffer += sizeof(uint16);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sNodeDescRsp.sNodeDescriptor.u8DescriptorCapability,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sNodeDescRsp.sNodeDescriptor.u8MacFlags,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sNodeDescRsp.sNodeDescriptor.u8MaxBufferSize,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sNodeDescRsp.sNodeDescriptor.uBitUnion.u16Value,sizeof(uint16) );
                         u16Length += sizeof(uint16);
                         pu8Buffer += sizeof(uint16);
                        vSL_WriteMessage(E_SL_MSG_NODE_DESCRIPTOR_RESPONSE, u16Length, au8StatusBuffer);
                    }
                        break;

                    case 0x8006:
                    {
                         u16Length = sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMatchDescRsp.u8Status,sizeof(uint8) );
                        pu8Buffer += sizeof(uint8);
                        u16Length += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMatchDescRsp.u16NwkAddrOfInterest,sizeof(uint16) );
                        pu8Buffer += sizeof(uint16);
                        u16Length += sizeof(uint16);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMatchDescRsp.u8MatchLength,sizeof(uint8) );
                        pu8Buffer += sizeof(uint8);
                        u16Length += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uLists.au8Data,(sizeof(uint8)*sApsZdpEvent.uZdpData.sMatchDescRsp.u8MatchLength));
                        u16Length+=(sizeof(uint8)*sApsZdpEvent.uZdpData.sMatchDescRsp.u8MatchLength);

                        vSL_WriteMessage(E_SL_MSG_MATCH_DESCRIPTOR_RESPONSE, u16Length, au8StatusBuffer);
                    }
                        break;

                    case 0x8004:
                    {

                        u16Length = sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sSimpleDescRsp.u8Status,sizeof(uint8) );
                        pu8Buffer += sizeof(uint8);
                        u16Length += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sSimpleDescRsp.u16NwkAddrOfInterest,sizeof(uint16) );
                        pu8Buffer += sizeof(uint16);
                        u16Length += sizeof(uint16);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sSimpleDescRsp.u8Length,sizeof(uint8) );
                        pu8Buffer += sizeof(uint8);
                        u16Length += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.u8Endpoint,sizeof(uint8) );
                        pu8Buffer += sizeof(uint8);
                        u16Length += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.u16ApplicationProfileId,sizeof(uint16) );
                        pu8Buffer += sizeof(uint16);
                        u16Length += sizeof(uint16);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.u16DeviceId,sizeof(uint16) );
                        pu8Buffer += sizeof(uint16);
                        u16Length += sizeof(uint16);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.uBitUnion.u8Value,sizeof(uint8) );
                        pu8Buffer += sizeof(uint8);
                        u16Length+= sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.u8InClusterCount,sizeof(uint8) );
                        pu8Buffer += sizeof(uint8);
                        u16Length += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uLists.au16Data,
                                (sizeof(uint16)*(sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.u8InClusterCount)));

                        u16Length+=(sizeof(uint16)*(sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.u8InClusterCount));
                        pu8Buffer += (sizeof(uint16)*(sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.u8InClusterCount));

                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.u8OutClusterCount,sizeof(uint8) );

                        pu8Buffer += sizeof(uint8);
                        u16Length += sizeof(uint8);

                        memcpy(pu8Buffer,&sApsZdpEvent.uLists.au16Data[sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.u8InClusterCount],
                                (sizeof(uint16)*(sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.u8OutClusterCount)));

                        u16Length+=(sizeof(uint16)*(sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.u8OutClusterCount));
                        pu8Buffer += (sizeof(uint16)*(sApsZdpEvent.uZdpData.sSimpleDescRsp.sSimpleDescriptor.u8OutClusterCount));

                        vSL_WriteMessage(E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE, u16Length, au8StatusBuffer);
                    }
                        break;

                    case 0x8000:
                    case 0x8001:
                    {
                         u16Length = sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sIeeeAddrRsp.u8Status,sizeof(uint8) );
                        pu8Buffer += sizeof(uint8);
                        u16Length += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sIeeeAddrRsp.u64IeeeAddrRemoteDev,sizeof(uint64) );
                        pu8Buffer += sizeof(uint64);
                        u16Length += sizeof(uint64);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sIeeeAddrRsp.u16NwkAddrRemoteDev,sizeof(uint16) );
                        pu8Buffer += sizeof(uint16);
                        u16Length += sizeof(uint16);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sIeeeAddrRsp.u8NumAssocDev,sizeof(uint8) );
                        pu8Buffer += sizeof(uint8);
                        u16Length += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sIeeeAddrRsp.u8StartIndex,sizeof(uint8) );
                        pu8Buffer += sizeof(uint8);
                        u16Length += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sIeeeAddrRsp.pu16NwkAddrAssocDevList,(sizeof(uint16)*sApsZdpEvent.uZdpData.sIeeeAddrRsp.u8NumAssocDev) );
                        pu8Buffer += (sizeof(uint16)*sApsZdpEvent.uZdpData.sIeeeAddrRsp.u8NumAssocDev);
                        u16Length += (sizeof(uint16)*sApsZdpEvent.uZdpData.sIeeeAddrRsp.u8NumAssocDev);

                        if(sApsZdpEvent.u16ClusterId == 0x8000)
                        {
                            vSL_WriteMessage(E_SL_MSG_NETWORK_ADDRESS_RESPONSE, u16Length, au8StatusBuffer);
                        }
                        else
                        {
                            vSL_WriteMessage(E_SL_MSG_IEEE_ADDRESS_RESPONSE, u16Length, au8StatusBuffer);

                        }
                    }
                        break;

                    case 0x8034:
                    {
                         u16Length = sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMgmtLeaveRsp.u8Status,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        vSL_WriteMessage(E_SL_MSG_MANAGEMENT_LEAVE_RESPONSE, u16Length, au8StatusBuffer);
                    }
                    break;

                    case 0x8031:
                    {
                         uint8 u8Values, u8Bytes;
                         u16Length = sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMgmtLqiRsp.u8Status,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMgmtLqiRsp.u8NeighborTableEntries,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMgmtLqiRsp.u8NeighborTableListCount,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMgmtLqiRsp.u8StartIndex,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        for(u8Values = 0; u8Values < sApsZdpEvent.uZdpData.sMgmtLqiRsp.u8NeighborTableListCount; u8Values++)
                        {
                          memcpy(pu8Buffer,&sApsZdpEvent.uLists.asNtList[u8Values].u16NwkAddr,sizeof(uint16));
                          u16Length += sizeof(uint16);
                          pu8Buffer += sizeof(uint16);
                          memcpy(pu8Buffer,&sApsZdpEvent.uLists.asNtList[u8Values].u64ExtPanId,sizeof(uint64));
                          u16Length += sizeof(uint64);
                          pu8Buffer += sizeof(uint64);
                          memcpy(pu8Buffer,&sApsZdpEvent.uLists.asNtList[u8Values].u64ExtendedAddress,sizeof(uint64));
                          u16Length += sizeof(uint64);
                          pu8Buffer += sizeof(uint64);
                          memcpy(pu8Buffer,&sApsZdpEvent.uLists.asNtList[u8Values].u8Depth,sizeof(uint8));
                          u16Length += sizeof(uint8);
                          pu8Buffer += sizeof(uint8);
                          memcpy(pu8Buffer,&sApsZdpEvent.uLists.asNtList[u8Values].u8LinkQuality,sizeof(uint8));
                          u16Length += sizeof(uint8);
                          pu8Buffer += sizeof(uint8);
                          u8Bytes = sApsZdpEvent.uLists.asNtList[u8Values].uAncAttrs.u2DeviceType;
                          u8Bytes |= (sApsZdpEvent.uLists.asNtList[u8Values].uAncAttrs.u2PermitJoining << 2);
                          u8Bytes |= (sApsZdpEvent.uLists.asNtList[u8Values].uAncAttrs.u2Relationship << 4);
                          u8Bytes |= (sApsZdpEvent.uLists.asNtList[u8Values].uAncAttrs.u2RxOnWhenIdle << 6);
                          memcpy(pu8Buffer,&u8Bytes,sizeof(uint8));
                          u16Length += sizeof(uint8);
                          pu8Buffer += sizeof(uint8);
                        }
                          vSL_WriteMessage(E_SL_MSG_MANAGEMENT_LQI_RESPONSE, u16Length, au8StatusBuffer);
                    }
                        break;
                    case 0x8003:
                    {
                         u16Length = sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sPowerDescRsp.u8Status,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                         memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sPowerDescRsp.sPowerDescriptor.uBitUnion.u16Value,sizeof(uint16) );
                         u16Length += sizeof(uint16);
                         pu8Buffer += sizeof(uint16);
                         vSL_WriteMessage(E_SL_MSG_POWER_DESCRIPTOR_RESPONSE, u16Length, au8StatusBuffer);
                    }
                    break;


                    case 0x8005:
                    {
                         u16Length = sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sActiveEpRsp.u8Status,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                         memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sActiveEpRsp.u16NwkAddrOfInterest,sizeof(uint16) );
                         u16Length += sizeof(uint16);
                         pu8Buffer += sizeof(uint16);
                         memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sActiveEpRsp.u8ActiveEpCount,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                         memcpy(pu8Buffer,&sApsZdpEvent.uLists.au8Data,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                         vSL_WriteMessage(E_SL_MSG_ACTIVE_ENDPOINT_RESPONSE, u16Length, au8StatusBuffer);
                    }
                    break;

                    case 0x8038:
                    {
                         u16Length = sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMgmtNwkUpdateNotify.u8Status,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                         memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMgmtNwkUpdateNotify.u16TotalTransmissions,sizeof(uint16) );
                         u16Length += sizeof(uint16);
                         pu8Buffer += sizeof(uint16);
                         memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMgmtNwkUpdateNotify.u16TransmissionFailures,sizeof(uint16) );
                         u16Length += sizeof(uint16);
                         pu8Buffer += sizeof(uint16);
                         memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMgmtNwkUpdateNotify.u32ScannedChannels,sizeof(uint32) );
                         u16Length += sizeof(uint32);
                         pu8Buffer += sizeof(uint32);
                         memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sMgmtNwkUpdateNotify.u8ScannedChannelListCount,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                         memcpy(pu8Buffer,&sApsZdpEvent.uLists.au8Data,(sizeof(uint8)*sApsZdpEvent.uZdpData.sMgmtNwkUpdateNotify.u8ScannedChannelListCount) );
                         u16Length += (sizeof(uint8)*sApsZdpEvent.uZdpData.sMgmtNwkUpdateNotify.u8ScannedChannelListCount) ;
                         pu8Buffer += (sizeof(uint8)*sApsZdpEvent.uZdpData.sMgmtNwkUpdateNotify.u8ScannedChannelListCount) ;
                         vSL_WriteMessage(E_SL_MSG_MANAGEMENT_NETWORK_UPDATE_RESPONSE, u16Length, au8StatusBuffer);
                    }
                    break;
                    case 0x8015:
                    {
                         u16Length = sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sSystemServerDiscoveryRsp.u8Status,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sSystemServerDiscoveryRsp.u16ServerMask,sizeof(uint16) );
                         u16Length += sizeof(uint16);
                         pu8Buffer += sizeof(uint16);
                         vSL_WriteMessage(E_SL_MSG_SYSTEM_SERVER_DISCOVERY_RESPONSE, u16Length, au8StatusBuffer);
                    }
                    break;
                    default:
                    {
                         u16Length = sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                        memcpy(pu8Buffer,&sApsZdpEvent.uZdpData.sUnbindRsp.u8Status,sizeof(uint8) );
                         u16Length += sizeof(uint8);
                         pu8Buffer += sizeof(uint8);
                         switch(sApsZdpEvent.u16ClusterId)
                         {
                         case 0x8021:
                            vSL_WriteMessage(E_SL_MSG_BIND_RESPONSE, u16Length, au8StatusBuffer);
                            break;
                         case 0x8022:
                            vSL_WriteMessage(E_SL_MSG_UNBIND_RESPONSE, u16Length, au8StatusBuffer);
                            break;

                         default:
                            break;
                         }
                    }
                        break;
                    }
                }
            }
            break;

            case ZPS_EVENT_NWK_STATUS_INDICATION:
                vLog_Printf(TRACE_APP,LOG_DEBUG, "\nNwkStat: Addr:%x Status:%x",
                        sStackEvent.uEvent.sNwkStatusIndicationEvent.u16NwkAddr,
                        sStackEvent.uEvent.sNwkStatusIndicationEvent.u8Status);
                vLog_Printf(TRACE_APP,LOG_DEBUG, "\nNwkStat: Addr:%x Status:%x",
                        sStackEvent.uEvent.sNwkStatusIndicationEvent.u16NwkAddr,
                        sStackEvent.uEvent.sNwkStatusIndicationEvent.u8Status);
                break;

            case  ZPS_EVENT_NWK_STARTED:
            case ZPS_EVENT_NWK_FAILED_TO_START:
            case ZPS_EVENT_NWK_JOINED_AS_ROUTER:
            case ZPS_EVENT_NWK_FAILED_TO_JOIN:
            {
                uint8 u8FormJoin;
                bool_t bSend = TRUE;
                if(sStackEvent.eType == ZPS_EVENT_NWK_STARTED)
                {
                    u8FormJoin = 1; /* formed */
                }
                if(sStackEvent.eType == ZPS_EVENT_NWK_FAILED_TO_START)
                {
                    u8FormJoin = 0xC4; /* Startup failure */
                }
                if(sStackEvent.eType == ZPS_EVENT_NWK_JOINED_AS_ROUTER )
                {
                    u8FormJoin = 0; /* joined */
                }
                if(sStackEvent.eType == ZPS_EVENT_NWK_FAILED_TO_JOIN)
                {
                    if(!bSecondDisc)
                    {
                        bSend = FALSE;
                    }
                    else
                    {
                        u8FormJoin = sStackEvent.uEvent.sNwkJoinFailedEvent.u8Status; /* failed reason */
                    }
                }
                if(bSend)
                {
                    vSendJoinedFormEventToHost(u8FormJoin,au8StatusBuffer);
                }
            }
                break;

            default:
                break;

        }

    }else if (OS_eCollectMessage(APP_msgEvents, &sAppEvent) == OS_E_OK) {

        //  vLog_Printf(TRACE_APP,LOG_DEBUG "\nE:%s(%d)\n",
        //              sAppEvent.eType,sAppEvent.eType);
    }
    vLog_Printf(TRACE_APP,LOG_DEBUG, "Got event from ZPS %x %x %x %x\n",sDeviceDesc.eNodeState,sStackEvent.eType,sAppEvent.eType,sZllState.eState);
    if (sStackEvent.eType == ZPS_EVENT_ERROR) {
        ZPS_tsAfErrorEvent *psErrEvt = &sStackEvent.uEvent.sAfErrorEvent;
        vLog_Printf(TRACE_APP,LOG_ERR, "\nStack Err: %d", psErrEvt->eError);

        if(psErrEvt->eError == ZPS_ERROR_APDU_INSTANCES_EXHAUSTED)
        {
            vLog_Printf(TRACE_APP,LOG_ERR, "\nAPDU instance ran out : %x", psErrEvt->uErrorData.sAfErrorApdu.hAPdu);
        }

        if(ZPS_ERROR_OS_MESSAGE_QUEUE_OVERRUN == psErrEvt->eError)
        {
            vLog_Printf(TRACE_APP,LOG_ERR, "\nHandle: %x", psErrEvt->uErrorData.sAfErrorOsMessageOverrun.hMessage);
            vSL_LogFlush();
        }

    }
    if (sAppEvent.eType == APP_E_EVENT_START_ROUTER) {
        vForceStartRouter(au8StatusBuffer);
    }

    if(sAppEvent.eType == APP_E_EVENT_TRANSPORT_HA_KEY)
    {
        ZPS_tuAddress uDstAddress;
        uDstAddress.u64Addr = sAppEvent.uEvent.u64TransportKeyAddress;
        ZPS_eAplAibSetApsTrustCenterAddress( 0xffffffffffffffffULL );
        ZPS_eAplZdoTransportNwkKey(ZPS_E_ADDR_MODE_IEEE,uDstAddress,psNib->sTbl.psSecMatSet[0].au8Key,psNib->sTbl.psSecMatSet[0].u8KeySeqNum,FALSE,0);
        ZPS_eAplAibSetApsTrustCenterAddress(ZPS_u64NwkNibGetExtAddr(ZPS_pvAplZdoGetNwkHandle()));
    }


    if(sAppEvent.eType == APP_E_EVENT_SEND_PERMIT_JOIN)
    {
        bool_t bStatus = ZPS_vNwkGetPermitJoiningStatus(ZPS_pvAplZdoGetNwkHandle());
        pu8Buffer = au8StatusBuffer;
        u16Length = sizeof(bool_t);
        memcpy(pu8Buffer,&bStatus,sizeof(bool_t));
        vSL_WriteMessage(E_SL_MSG_GET_PERMIT_JOIN_RESPONSE, u16Length, au8StatusBuffer);
    }

    if(sAppEvent.eType == APP_E_EVENT_ENCRYPT_SEND_KEY)
    {
        AESSW_Block_u uNonce;
        uint8 *pu8MicLocation,bAesReturn;
        uint8* pu8Key = (uint8*)ZPS_pvNwkSecGetNetworkKey(ZPS_pvAplZdoGetNwkHandle());
        uint64 u64Address = ZPS_u64NwkNibGetExtAddr(ZPS_pvAplZdoGetNwkHandle());
        pu8Buffer = au8StatusBuffer;
        memcpy(pu8Buffer,&sAppEvent.uEvent.sEncSendMsg.u64Address,sizeof(uint64));
        pu8Buffer += sizeof(uint64);
        u16Length = sizeof(uint64);
        memcpy(pu8Buffer,pu8Key,(sizeof(8)*16));
        Znc_vWrite64Nbo(sAppEvent.uEvent.sEncSendMsg.u64Address,&uNonce.au8[1]);
        memset(&uNonce.au8[9],0,(sizeof(uint8)*8));
        pu8MicLocation = pu8Buffer + (sizeof(uint8)*16);
        bAesReturn = bACI_WriteKey((tsReg128*)&sAppEvent.uEvent.sEncSendMsg.uKey);
        if(bAesReturn)
        {
            vACI_OptimisedCcmStar(
                TRUE,
                4,
                0,
                16,
                &uNonce,
                pu8Buffer,
                pu8Buffer,                // overwrite the i/p data
                pu8MicLocation,    // append to the o/p data
                NULL);
        }
        u16Length += (sizeof(uint8)*16) + 4;
        pu8Buffer += (sizeof(uint8)*16) + 4;
        memcpy(pu8Buffer,&u64Address,sizeof(uint64));
        u16Length += sizeof(uint64) ;
        pu8Buffer +=sizeof(uint64) ;
        memcpy(pu8Buffer,&psNib->sPersist.u8ActiveKeySeqNumber,sizeof(uint8));
        u16Length += sizeof(uint8) ;
        pu8Buffer += sizeof(uint8) ;
        memcpy(pu8Buffer,&psNib->sPersist.u8VsChannel,sizeof(uint8));
        u16Length += sizeof(uint8) ;
        pu8Buffer += sizeof(uint8) ;
        memcpy(pu8Buffer,&psNib->sPersist.u16VsPanId,sizeof(uint16));
        u16Length += sizeof(uint16) ;
        pu8Buffer += sizeof(uint16) ;
        memcpy(pu8Buffer,&psNib->sPersist.u64ExtPanId,sizeof(uint64));
        u16Length += sizeof(uint64) ;
        pu8Buffer += sizeof(uint64) ;
        vSL_WriteMessage(E_SL_MSG_AUTHENTICATE_DEVICE_RESPONSE, u16Length, au8StatusBuffer);
    }

    switch (sDeviceDesc.eNodeState) {

    case E_DISCOVERY:
        if (sStackEvent.eType == ZPS_EVENT_NWK_DISCOVERY_COMPLETE) {
            vLog_Printf(TRACE_APP,LOG_DEBUG, "Disc st %d c %d sel %d\n",
                    sStackEvent.uEvent.sNwkDiscoveryEvent.eStatus,
                    sStackEvent.uEvent.sNwkDiscoveryEvent.u8NetworkCount,
                    sStackEvent.uEvent.sNwkDiscoveryEvent.u8SelectedNetwork);
            if (((sStackEvent.uEvent.sNwkDiscoveryEvent.eStatus == 0)
                    || (sStackEvent.uEvent.sNwkDiscoveryEvent.eStatus
                            == ZPS_NWK_ENUM_NEIGHBOR_TABLE_FULL))) {
                psNwkDescr = NULL;
                if ((0 != sStackEvent.uEvent.sNwkDiscoveryEvent.u8NetworkCount)
                        && (sStackEvent.uEvent.sNwkDiscoveryEvent.u8SelectedNetwork
                                != 0xff)) {

                    psNwkDescr
                            = &sStackEvent.uEvent.sNwkDiscoveryEvent.psNwkDescriptors[sStackEvent.uEvent.sNwkDiscoveryEvent.u8SelectedNetwork];


                    u8Status = ZPS_eAplZdoJoinNetwork(psNwkDescr);
                    vLog_Printf(TRACE_APP,LOG_DEBUG, "Try Join %d\n", u8Status );
                    if (0 != u8Status) {
                        vSecondScan( ZPS_pvAplZdoGetNwkHandle(), &bSecondDisc);
                    }
                } else {
                    /* Discovery failed */
                        vSecondScan( ZPS_pvAplZdoGetNwkHandle(), &bSecondDisc);
                }
            } else {
                /* Discovery failed */
                        vSecondScan( ZPS_pvAplZdoGetNwkHandle(), &bSecondDisc);
            }
        }
        if (sStackEvent.eType == ZPS_EVENT_NWK_FAILED_TO_JOIN) {
            if(bSecondDisc)
            {
                vPickChannel(ZPS_pvAplZdoGetNwkHandle());
            }
            else
            {
                 vSecondScan( ZPS_pvAplZdoGetNwkHandle(), &bSecondDisc);
            }
        }

        if (sStackEvent.eType == ZPS_EVENT_NWK_JOINED_AS_ROUTER) {
            sDeviceDesc.eNodeState = E_RUNNING;

            sZllState.eState = NOT_FACTORY_NEW;
            sZllState.u16MyAddr = sStackEvent.uEvent.sNwkJoinedEvent.u16Addr;
            sZllState.u16FreeAddrLow = 0;
            sZllState.u16FreeAddrHigh = 0;
            sZllState.u16FreeGroupLow = 0;
            sZllState.u16FreeGroupHigh = 0;
            PDM_eSaveRecordData( PDM_ID_APP_ZLL_CMSSION,&sZllState,sizeof(tsZllState));
            PDM_eSaveRecordData( PDM_ID_APP_CONTROL_BRIDGE,&sDeviceDesc,sizeof(tsDeviceDesc));
            vLog_Printf(TRACE_APP,LOG_DEBUG, "Joined as Router\n");
            break;
        }
        break;

    case E_NETWORK_INIT:
        /* Wait here for touch link */
        if ((sStackEvent.eType == ZPS_EVENT_NWK_DISCOVERY_COMPLETE)
                || (sStackEvent.eType == ZPS_EVENT_NWK_FAILED_TO_JOIN)) {
            sCommissionEvent.eType = APP_E_COMMISSION_DISCOVERY_DONE;
            OS_ePostMessage(APP_CommissionEvents, &sCommissionEvent);
        }

        if (sStackEvent.eType == ZPS_EVENT_NWK_LEAVE_CONFIRM) {
            if (sStackEvent.uEvent.sNwkLeaveConfirmEvent.u64ExtAddr == 0) {
                u32OldFrameCtr = psNib->sTbl.u32OutFC;
                vFactoryResetRecords();

            }
        }

        break;

    case E_RUNNING:
        if (sStackEvent.eType != ZPS_EVENT_NONE) {
            vLog_Printf(TRACE_APP,LOG_DEBUG, "Zps event in running %d\n", sStackEvent.eType);
            if ((sStackEvent.eType == ZPS_EVENT_NWK_DISCOVERY_COMPLETE)
                    || (sStackEvent.eType == ZPS_EVENT_NWK_FAILED_TO_JOIN)) {
                /* let commissioning know discovery completed */
                sCommissionEvent.eType = APP_E_COMMISSION_DISCOVERY_DONE;
                OS_ePostMessage(APP_CommissionEvents, &sCommissionEvent);
            }

            if (sStackEvent.eType == ZPS_EVENT_NWK_LEAVE_CONFIRM) {
                if (sStackEvent.uEvent.sNwkLeaveConfirmEvent.u64ExtAddr == 0) {
                    u32OldFrameCtr = psNib->sTbl.u32OutFC;
                    vFactoryResetRecords();
                }
            }
            if (sStackEvent.eType == ZPS_EVENT_NWK_NEW_NODE_HAS_JOINED) {
                vLog_Printf(TRACE_APP,LOG_DEBUG, "\nNode joined %04x", sStackEvent.uEvent.sNwkJoinIndicationEvent.u16NwkAddr);
                s_sLedState.u32LedToggleTime =APP_TIME_MS(200);
                u8JoinedDevice = 0;
            }

            if (sStackEvent.eType == ZPS_EVENT_NWK_LEAVE_INDICATION) {
                    /* report to host */
                    pu8Buffer = au8StatusBuffer;
                    memcpy(pu8Buffer,&sStackEvent.uEvent.sNwkLeaveIndicationEvent.u64ExtAddr,sizeof(uint64) );
                    u16Length = sizeof(uint64);
                    pu8Buffer += sizeof(uint64);
                    memcpy(pu8Buffer,&sStackEvent.uEvent.sNwkLeaveIndicationEvent.u8Rejoin,sizeof(uint8) );
                    u16Length += sizeof(uint8);
                    pu8Buffer += sizeof(uint8);
                    vSL_WriteMessage(E_SL_MSG_LEAVE_INDICATION, u16Length, au8StatusBuffer);
            }
        }
        if (sAppEvent.eType == APP_E_EVENT_TOUCH_LINK) {

             vLog_Printf(TRACE_APP,LOG_DEBUG, "\nPicked Touch link command %x\n", sAppEvent.uEvent.sTouchLink.u16DeviceId);
            if ((sAppEvent.uEvent.sTouchLink.u16DeviceId >= COLOUR_REMOTE_DEVICE_ID) &&
                 (sAppEvent.uEvent.sTouchLink.u16DeviceId <= CONTROL_BRIDGE_DEVICE_ID ))
            {
                /*
                 * Just added a controller device, send endpoint info
                 */
                tsCLD_ZllUtility_EndpointInformationCommandPayload sPayload;
                tsZCL_Address               sDestinationAddress;
                tsCLD_ZllDeviceTable * psDevTab = (tsCLD_ZllDeviceTable*)psGetDeviceTable();

                sPayload.u64IEEEAddr = psDevTab->asDeviceRecords[0].u64IEEEAddr;
                sPayload.u16NwkAddr = ZPS_u16AplZdoGetNwkAddr();
                sPayload.u16DeviceID = psDevTab->asDeviceRecords[0].u16DeviceId;
                sPayload.u16ProfileID = psDevTab->asDeviceRecords[0].u16ProfileId;
                sPayload.u8Endpoint = psDevTab->asDeviceRecords[0].u8Endpoint;
                sPayload.u8Version = psDevTab->asDeviceRecords[0].u8Version;

                vLog_Printf(TRACE_APP,LOG_DEBUG, "\nTell new controller about us %04x", sAppEvent.uEvent.sTouchLink.u16NwkAddr);

                sDestinationAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
                sDestinationAddress.uAddress.u16DestinationAddress = sAppEvent.uEvent.sTouchLink.u16NwkAddr;
                eCLD_ZllUtilityCommandEndpointInformationCommandSend(
                                    sDeviceTable.asDeviceRecords[0].u8Endpoint,                        // src ep
                                    sAppEvent.uEvent.sTouchLink.u8Endpoint,            // dst ep
                                    &sDestinationAddress,
                                    &u8SeqNo,
                                    &sPayload);
            }
            else
            {
                if ( (sAppEvent.uEvent.sTouchLink.u16DeviceId <= COLOUR_TEMPERATURE_LIGHT_DEVICE_ID ) ) {
                    /* Controlled device attempt to add to device endpoint table
                     *
                     */
                    if (bAddToEndpointTable( &sAppEvent.uEvent.sTouchLink)) {
                        /* Added new or updated old
                         * ensure that it has our group address
                         */
                        PDM_eSaveRecordData( PDM_ID_APP_END_P_TABLE,&sEndpointTable,sizeof(tsZllEndpointInfoTable));
                        bAddrMode = FALSE;           // ensure not in group mode
                        vLog_Printf(TRACE_APP,LOG_DEBUG, "\nNEW Send group add %d to %04x\n", sGroupTable.asGroupRecords[0].u16GroupId,
                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr);
                        vAppAddGroup(sGroupTable.asGroupRecords[0].u16GroupId, FALSE);

                        vAppIdentifyEffect(E_CLD_IDENTIFY_EFFECT_OKAY);
                    }
                    pu8Buffer = au8StatusBuffer;
                    memcpy(pu8Buffer,&sAppEvent.uEvent.sTouchLink.u16NwkAddr,sizeof(uint16) );
                    u16Length = sizeof(uint16);
                    pu8Buffer += sizeof(uint16);
                    memcpy(pu8Buffer,&sAppEvent.uEvent.sTouchLink.u64Address,sizeof(uint64) );
                    u16Length += sizeof(uint64);
                    pu8Buffer += sizeof(uint64);
                    memcpy(pu8Buffer,&sAppEvent.uEvent.sTouchLink.u8MacCap,sizeof(uint8) );
                    u16Length += sizeof(uint8);
                    vSL_WriteMessage(E_SL_MSG_DEVICE_ANNOUNCE, u16Length, au8StatusBuffer);
                }
            }
        }
        break;

    default:
        break;

    }

    /*
     * Global clean up to make sure any APDUs have been freed
     */
    if (sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION) {
        PDUM_eAPduFreeAPduInstance( sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);
    }

}
/****************************************************************************
 *
 * NAME: bPutChar
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PUBLIC bool_t bPutChar(uint8 u8TxByte)
{
    bool bSent = TRUE;

    OS_eEnterCriticalSection(APP_mutexUart);

    if (UART_bTxReady() && OS_E_QUEUE_EMPTY == OS_eGetMessageStatus(APP_msgSerialTx)) {
        /* send byte now and enable irq */
        UART_vSetTxInterrupt(TRUE);
        UART_vTxChar(u8TxByte);
    } else {
        bSent = (OS_E_OK == OS_ePostMessage(APP_msgSerialTx, &u8TxByte));
        if (bSent) {
            /* prevent sleep if we are waiting to output data down the UART */
            PWRM_eStartActivity();
        }
    }

    OS_eExitCriticalSection(APP_mutexUart);

    /* kick parser task in case we have anything more to output */
    OS_eActivateTask(APP_taskAtParser);

    return bSent;
}
/****************************************************************************
 *
 * NAME: vPickChannel
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vPickChannel( void *pvNwk)
{
    vLog_Printf(TRACE_APP,LOG_DEBUG, "\nPicked Ch %d", sZllState.u8MyChannel);
    ZPS_vNwkNibSetChannel( ZPS_pvAplZdoGetNwkHandle(), DEFAULT_CHANNEL);
    ZPS_vNwkNibSetPanId( pvNwk, (uint16)RND_u32GetRand(1, 0xfff0) );
    /* Set channel mask to primary channels */
    ZPS_eAplAibSetApsChannelMask( ZLL_CHANNEL_MASK);
    sDeviceDesc.eNodeState = E_WAIT_START;
}

/****************************************************************************
 *
 * NAME: vSecondScan
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSecondScan( void * pvNwk, bool * pbSecondDisc)
{
    if (!*pbSecondDisc)
    {
        *pbSecondDisc = TRUE;
        /* repeat on secondary channels */
        ZPS_vNwkNibClearDiscoveryNT(pvNwk);
        ZPS_eAplZdoDiscoverNetworks(ZLL_SECOND_CHANNEL_MASK);
        vLog_Printf(TRACE_APP,LOG_DEBUG, "Disc 22\n");
    }
    else
    {
        /* second scan failed */
        vPickChannel( pvNwk);
        vLog_Printf(TRACE_APP,LOG_DEBUG, "Wait 11\n");
    }
}


/****************************************************************************
 *
 * NAME: vAppIdentifyEffect
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppIdentifyEffect( teCLD_Identify_EffectId eEffect) {
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress,FALSE);

    eCLD_IdentifyCommandTriggerEffectSend(sDeviceTable.asDeviceRecords[0].u8Endpoint,
            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                            &sAddress,
                            &u8Seq,
                            eEffect,
                            0 /* Effect varient */);
}


PUBLIC void vSetAddressMode(void)
{
    bAddrMode = !bAddrMode;
    if (bAddrMode) {
        vLog_Printf(TRACE_APP,LOG_DEBUG, "\nG_CAST\n");
    } else {
        vLog_Printf(TRACE_APP,LOG_DEBUG, "\nU_CAST\n");
    }
}


/****************************************************************************
 *
 * NAME: bAddToEndpointTable
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * bool
 *
 ****************************************************************************/
PUBLIC bool bAddToEndpointTable(APP_tsEventTouchLink *psEndpointData) {
    uint8 u8Index = 0xff;
    bool_t bPresent;

    bPresent = u8SearchEndpointTable(psEndpointData, &u8Index);
    if (u8Index < 0xff)
    {
        /* There is space for a new entry
         * or it is already there
         */
        if (!bPresent) {
            /* new entry, increment device count
             *
             */
            sEndpointTable.u8NumRecords++;
        }
        /* Add or update details at the slot indicated
         *
         */
        sEndpointTable.asEndpointRecords[u8Index].u16NwkAddr = psEndpointData->u16NwkAddr;
        sEndpointTable.asEndpointRecords[u8Index].u16ProfileId = psEndpointData->u16ProfileId;
        sEndpointTable.asEndpointRecords[u8Index].u16DeviceId = psEndpointData->u16DeviceId;
        sEndpointTable.asEndpointRecords[u8Index].u8Endpoint = psEndpointData->u8Endpoint;
        sEndpointTable.asEndpointRecords[u8Index].u8Version = psEndpointData->u8Version;
        vLog_Printf(TRACE_APP,LOG_DEBUG, "\nAdd idx %d Addr %04x Ep %d Dev %04x", u8Index,
                sEndpointTable.asEndpointRecords[u8Index].u16NwkAddr,
                sEndpointTable.asEndpointRecords[u8Index].u8Endpoint,
                sEndpointTable.asEndpointRecords[u8Index].u16DeviceId);

        sEndpointTable.u8CurrentLight = u8Index;
        return TRUE;
    }

    /* no room in the table */
    return FALSE;
}


/****************************************************************************
 *
 * NAME: vAppAddGroup
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppAddGroup( uint16 u16GroupId, bool_t bBroadcast)
{

    tsCLD_Groups_AddGroupRequestPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, bBroadcast);

    sPayload.sGroupName.pu8Data = (uint8*)"";
    sPayload.sGroupName.u8Length = 0;
    sPayload.sGroupName.u8MaxLength = 0;
    sPayload.u16GroupId = u16GroupId;

    eCLD_GroupsCommandAddGroupRequestSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint   /* don't care group addr */,
                            &sAddress,
                            &u8Seq,
                            &sPayload);

}

/****************************************************************************
 *
 * NAME: u8SearchEndpointTable
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * uint8
 *
 ****************************************************************************/
PRIVATE uint8 u8SearchEndpointTable(APP_tsEventTouchLink *psEndpointData, uint8* pu8Index) {
    int i;
    bool bGotFree = FALSE;
    *pu8Index = 0xff;

    for (i=0; i<NUM_ENDPOINT_RECORDS; i++) {
        if ((psEndpointData->u16NwkAddr == sEndpointTable.asEndpointRecords[i].u16NwkAddr) &&
                (psEndpointData->u8Endpoint == sEndpointTable.asEndpointRecords[i].u8Endpoint)) {
            /* same ep on same device already known about */
            *pu8Index = i;
            vLog_Printf(TRACE_APP,LOG_DEBUG, "\nPresent");
            return 1;
        }
        if ((sEndpointTable.asEndpointRecords[i].u16NwkAddr == 0) && !bGotFree) {
            *pu8Index = i;
            bGotFree = TRUE;
            vLog_Printf(TRACE_APP,LOG_DEBUG, "\nFree slot %d", *pu8Index);
        }

    }

    vLog_Printf(TRACE_APP,LOG_DEBUG, "\nNot found");
    return (bGotFree)? 0: 3  ;
}
/****************************************************************************
 *
 * NAME: u8SearchEndpointTable
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * uint8
 *
 ****************************************************************************/

PUBLIC void vSetAddress(tsZCL_Address * psAddress, bool_t bBroadcast) {

    if (bBroadcast) {
        psAddress->eAddressMode = E_ZCL_AM_BROADCAST;
        psAddress->uAddress.eBroadcastMode = ZPS_E_APL_AF_BROADCAST_RX_ON;
    } else if (bAddrMode) {
        psAddress->eAddressMode = E_ZCL_AM_GROUP;
        psAddress->uAddress.u16DestinationAddress = sGroupTable.asGroupRecords[0].u16GroupId;
    } else {
        psAddress->eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
        psAddress->uAddress.u16DestinationAddress = sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr;
    }
}
/****************************************************************************
 *
 * NAME: App_TransportKeyCallback
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * uint8
 *
 ****************************************************************************/


OS_SWTIMER_CALLBACK(App_TransportKeyCallback, pvParam)
{
    APP_tsEvent sAppEvent;
    sAppEvent.eType = APP_E_EVENT_TRANSPORT_HA_KEY;
    sAppEvent.uEvent.u64TransportKeyAddress = *((uint64*)pvParam);
    OS_ePostMessage(APP_msgEvents, &sAppEvent);

}


/****************************************************************************
 *
 * NAME:       Znc_vWrite64Nbo
 */
/* @ingroup
 *
 * @param
 *
 * @return
 *
 * @note
 *
 ****************************************************************************/
PRIVATE void Znc_vWrite64Nbo(uint64 u64dWord, uint8 *pu8Payload)
{
    pu8Payload[0] = u64dWord;
    pu8Payload[1] = u64dWord >> 8;
    pu8Payload[2] = u64dWord >> 16;
    pu8Payload[3] = u64dWord >> 24;
    pu8Payload[4] = u64dWord >> 32;
    pu8Payload[5] = u64dWord >> 40;
    pu8Payload[6] = u64dWord >> 48;
    pu8Payload[7] = u64dWord >> 56;
}

/****************************************************************************
 *
 * NAME:       Znc_vSendDataIndicationToHost
 */
/* @ingroup
 *
 * @param
 *
 * @return
 *
 * @note
 *
 ****************************************************************************/
PUBLIC void Znc_vSendDataIndicationToHost(ZPS_tsAfEvent *psStackEvent,uint8* pau8StatusBuffer)
{
    uint8* pu8Buffer;
    uint16 u16Length;
    uint8 *dataPtr = (uint8*)PDUM_pvAPduInstanceGetPayload(psStackEvent->uEvent.sApsDataIndEvent.hAPduInst);
    uint8 u8Size = PDUM_u16APduInstanceGetPayloadSize( psStackEvent->uEvent.sApsDataIndEvent.hAPduInst);

    pu8Buffer = pau8StatusBuffer;
    memcpy(pu8Buffer,&psStackEvent->uEvent.sApsDataIndEvent.eStatus,sizeof(uint8) );
    u16Length = sizeof(uint8);
    pu8Buffer += sizeof(uint8);
    memcpy(pu8Buffer,&psStackEvent->uEvent.sApsDataIndEvent.u16ProfileId,sizeof(uint16) );
    u16Length += sizeof(uint16);
    pu8Buffer += sizeof(uint16);
    memcpy(pu8Buffer,&psStackEvent->uEvent.sApsDataIndEvent.u16ClusterId,sizeof(uint16) );
    u16Length += sizeof(uint16);
    pu8Buffer += sizeof(uint16);
    memcpy(pu8Buffer,&psStackEvent->uEvent.sApsDataIndEvent.u8SrcEndpoint,sizeof(uint8) );
    u16Length += sizeof(uint8);
    pu8Buffer += sizeof(uint8);
    memcpy(pu8Buffer,&psStackEvent->uEvent.sApsDataIndEvent.u8DstEndpoint,sizeof(uint8) );
    u16Length += sizeof(uint8);
    pu8Buffer += sizeof(uint8);
    memcpy(pu8Buffer,&psStackEvent->uEvent.sApsDataIndEvent.u8SrcAddrMode,sizeof(uint8) );
    u16Length += sizeof(uint8);
    pu8Buffer += sizeof(uint8);
    if(psStackEvent->uEvent.sApsDataIndEvent.u8SrcAddrMode == ZPS_E_ADDR_MODE_IEEE)
    {
        memcpy(pu8Buffer,&psStackEvent->uEvent.sApsDataIndEvent.uSrcAddress.u64Addr,sizeof(uint64) );
        u16Length += sizeof(uint64);
        pu8Buffer += sizeof(uint64);
    }
    else
    {
        memcpy(pu8Buffer,&psStackEvent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr,sizeof(uint16) );
        u16Length += sizeof(uint16);
        pu8Buffer += sizeof(uint16);
    }

    memcpy(pu8Buffer,&psStackEvent->uEvent.sApsDataIndEvent.u8DstAddrMode,sizeof(uint8) );
    u16Length += sizeof(uint8);
    pu8Buffer += sizeof(uint8);
    if(psStackEvent->uEvent.sApsDataIndEvent.u8DstAddrMode == ZPS_E_ADDR_MODE_IEEE)
    {
        memcpy(pu8Buffer,&psStackEvent->uEvent.sApsDataIndEvent.uDstAddress.u64Addr,sizeof(uint64) );
        u16Length += sizeof(uint64);
        pu8Buffer += sizeof(uint64);
    }
    else
    {
        memcpy(pu8Buffer,&psStackEvent->uEvent.sApsDataIndEvent.uDstAddress   .u16Addr,sizeof(uint16) );
        u16Length += sizeof(uint16);
        pu8Buffer += sizeof(uint16);
    }

    memcpy(pu8Buffer,dataPtr,(sizeof(uint8)*u8Size) );
    u16Length += (sizeof(uint8)*u8Size);
    vSL_WriteMessage(E_SL_MSG_DATA_INDICATION, u16Length, pau8StatusBuffer);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
