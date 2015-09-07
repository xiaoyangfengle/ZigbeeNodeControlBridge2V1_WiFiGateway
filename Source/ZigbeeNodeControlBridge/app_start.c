/*****************************************************************************
 *
 * MODULE: ZigbeeNodeControlBridge
 *
 * COMPONENT: app_start.c
 *
 * $AUTHOR: Faisal Bhaiyat$
 *
 * DESCRIPTION:
 *
 * $HeadURL: https://www.collabnet.nxp.com/svn/lprf_sware/Projects/Zigbee%20Protocol%20Stack/ZPS/Trunk/ZigbeeNodeControlBridge/Source/ZigbeeNodeControlBridge/app_start.c $
 *
 * $Revision: 54887 $
 *
 * $LastChangedBy: nxp29741 $
 *
 * $LastChangedDate: 2013-06-25 13:23:32 +0100 (Tue, 25 Jun 2013) $
 *
 * $Id: app_start.c 54887 2013-06-25 12:23:32Z nxp29741 $
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
 * Copyright Jennic Ltd 2007. All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "os.h"
#include "os_gen.h"
#include "pwrm.h"
#include "pdum_nwk.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg_uart.h"
#include "dbg.h"
#include "zps_gen.h"
#include "zps_apl_af.h"
#include "app_timer_driver.h"
#include "appapi.h"
#include "zps_nwk_pub.h"
#include "zps_mac.h"
#include "rnd_pub.h"
#include "Htsdriver.h"
#include "Button.h"
#include <string.h>
#include "SerialLink.h"
#include "app_ZncParser_task.h"
#include "app_Znc_zcltask.h"
#include "app_Znc_cmds.h"
#include "Uart.h"
#include "mac_pib.h"
#include "PDM_IDs.h"
#include "app_common.h"
#include "app_scenes.h"
#include "Log.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DEBUG_WDR TRUE
#ifndef DEBUG_WDR
#define DEBUG_WDR FASLE
#endif


#define UART_DEBUG TRUE
#ifndef UART_DEBUG
#define UART_DEBUG FALSE
#endif

#define TRACE_APPSTART TRUE
#ifndef TRACE_APPSTART
#define TRACE_APPSTART	FALSE
#endif

#define TRACE_EXC	TRUE
#ifndef TRACE_EXC
#define TRACE_EXC	FALSE
#endif

#define LED1_DIO_PIN (1 << 16)
#define LED2_DIO_PIN (1 << 17)
#define LED3_DIO_PIN (1 << 11)

#define LED_DIO_PINS ( LED1_DIO_PIN | LED2_DIO_PIN | LED3_DIO_PIN )
extern uint8 g_u8ZpsExpiryMaxCount;
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
extern bool_t g_bStopZdpAcks;
PRIVATE void vInitialiseApp(void);
PRIVATE void vUnclaimedInterrupt(void);
PRIVATE void vReportException(char *sExStr);
PRIVATE void vOSError(OS_teStatus eStatus, void *hObject);
void vfExtendedStatusCallBack (ZPS_teExtendedStatus eExtendedStatus);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/



PUBLIC tsLedState s_sLedState = {LED1_DIO_PIN, APP_TIME_MS(1), FALSE };

tsDeviceDesc           sDeviceDesc;
extern uint32 _heap_location;
uint8 u8JoinedDevice =0;

static const uint8 s_au8LnkKeyArray[16] = {0x5a, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6c,
                                     0x6c, 0x69, 0x61, 0x6e, 0x63, 0x65, 0x30, 0x39};
// ZLL Commissioning trust centre link key
static const uint8 s_au8ZllLnkKeyArray[16] = {0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
                                     0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf};
uint8 au8Buffer[128];
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vAppMain
 *
 * DESCRIPTION:
 * Entry point for application from a cold start.
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/

PUBLIC void vAppMain(void)
{
    uint8  u8FormJoin;
    // Wait until FALSE i.e. on XTAL  - otherwise uart data will be at wrong speed
     while (bAHI_GetClkSource() == TRUE);
     // Now we are running on the XTAL, optimise the flash memory wait states.
     vAHI_OptimiseWaitStates();
     g_bStopZdpAcks = TRUE;
     //*(volatile uint32 *)0x020000a0 = 0;
     g_u8ZpsExpiryMaxCount = 1;
    vAHI_DioSetDirection(0, LED_DIO_PINS);
    vAHI_DioSetOutput(LED_DIO_PINS, 0);
#ifdef HWDEBUG
    vAHI_WatchdogStart(12);
#endif
    vAHI_WatchdogException(TRUE);
    /* Initialise debugging */
#if  (UART_DEBUG == TRUE)
    /* Send debug output to DBG_UART */
    DBG_vUartInit(E_AHI_UART_0, DBG_E_UART_BAUD_RATE_115200);
#else
    /* Send debug output through SerialLink to host */
    vSL_LogInit();
#endif
    UART_vInit();
    UART_vRtsStartFlow();
    vLog_Printf(TRACE_APPSTART,LOG_DEBUG, "\n\nInitialising\n");
#ifdef PDM_NONE
    PDM_vWaitHost();
#endif
    extern void *_stack_low_water_mark;
    vSL_LogFlush();
    vLog_Printf(TRACE_APPSTART,LOG_INFO, "Stack low water mark = %08x\n", &_stack_low_water_mark);
    vAHI_SetStackOverflow(TRUE, (uint32)&_stack_low_water_mark);
    vLog_Printf(TRACE_EXC, LOG_INFO, "\n** Control Bridge Reset** ");
    if (bAHI_WatchdogResetEvent()) {
        /* Push out anything that might be in the log buffer already */
        vLog_Printf(TRACE_EXC, LOG_CRIT, "\n\n\n%s WATCHDOG RESET @ %08x ", "WDR",((uint32 *)&_heap_location)[0]);
        vSL_LogFlush();
    }
    app_vFormatAndSendUpdateLists();
    OS_vStart(vInitialiseApp, vUnclaimedInterrupt, vOSError);
    
    if (sDeviceDesc.eNodeState == E_RUNNING)
    {
        if(sDeviceDesc.u8DeviceType >= 1)
        {
            u8FormJoin = 0;
        }
        else
        {
            u8FormJoin = 1;
        }
        vSendJoinedFormEventToHost(u8FormJoin,au8Buffer);
        vSL_WriteMessage(E_SL_MSG_NODE_NON_FACTORY_NEW_RESTART, 1,(uint8*) &sDeviceDesc.eNodeState);
    }
    else
    {
        vSL_WriteMessage(E_SL_MSG_NODE_FACTORY_NEW_RESTART, 1,(uint8*) &sDeviceDesc.eNodeState);
    }
    OS_eStartSWTimer (APP_TickTimer,ZCL_TICK_TIME, NULL);

    /* idle task commences on exit from OS start call */
    while (TRUE) {
        /* Send any pending log messages */
        vSL_LogSend();
        /* kick the watchdog timer */
        vAHI_WatchdogRestart();
        PWRM_vManagePower();
    }
}

void vAppRegisterPWRMCallbacks(void)
{

}

OS_SWTIMER_CALLBACK(APP_cbToggleLED, pvParam)
{
    tsLedState *psLedState = (tsLedState *)pvParam;

    if(ZPS_vNwkGetPermitJoiningStatus(ZPS_pvAplZdoGetNwkHandle()))
    {
        vAHI_DioSetOutput(LED2_DIO_PIN, (psLedState->u32LedState)&LED_DIO_PINS);
        vAHI_DioSetOutput(LED3_DIO_PIN, (psLedState->u32LedState)&LED_DIO_PINS);
    }
    else
    {
        vAHI_DioSetOutput((~psLedState->u32LedState) & LED_DIO_PINS, psLedState->u32LedState & LED_DIO_PINS);

    }
    psLedState->u32LedState = (~psLedState->u32LedState) & LED_DIO_PINS;

    if(u8JoinedDevice == 10)
    {
        if( !ZPS_vNwkGetPermitJoiningStatus(ZPS_pvAplZdoGetNwkHandle()))
        {
            psLedState->u32LedToggleTime = APP_TIME_MS(1);
        }

        if(ZPS_vNwkGetPermitJoiningStatus(ZPS_pvAplZdoGetNwkHandle()))
        {
            psLedState->u32LedToggleTime = APP_TIME_MS(500);
        }
        u8JoinedDevice = 0;
    }
    u8JoinedDevice++;

    OS_eStartSWTimer(APP_tmrToggleLED, psLedState->u32LedToggleTime, psLedState);
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vInitialiseApp
 *
 * DESCRIPTION:
 * Initialises Zigbee stack, hardware and application.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitialiseApp(void)
{
    uint16 u16DataBytesRead;
#ifdef PDM_EEPROM
     PDM_eInitialise(63, mutexPDM);
#else
#ifdef PDM_EXTERNAL_FLASH
    PDM_vInit(7, 1, 64 * 1024 , mutexPDM, mutexFLASH, NULL, &g_sKey);
#else
    /* USING HOST TO SAVE PERSISTENT DATA */
#endif
#endif

    PDUM_vInit();
    sDeviceDesc.eNodeState = E_STARTUP;
    /* Restore any application data previously saved to flash */
    PDM_eReadDataFromRecord(
            PDM_ID_APP_CONTROL_BRIDGE,
                    &sDeviceDesc,
                    sizeof(tsDeviceDesc),
                    &u16DataBytesRead);
    PDM_eReadDataFromRecord(
            PDM_ID_APP_ZLL_CMSSION,
                    &sZllState,
                    sizeof(tsZllState),
                    &u16DataBytesRead);
    PDM_eReadDataFromRecord(
            PDM_ID_APP_END_P_TABLE,
                    &sEndpointTable,
                    sizeof(tsZllEndpointInfoTable),
                    &u16DataBytesRead);
    PDM_eReadDataFromRecord(
            PDM_ID_APP_GROUP_TABLE,
                    &sGroupTable,
                    sizeof(tsZllGroupInfoTable),
                    &u16DataBytesRead);

    vLoadScenesNVM();
    /* If the device state has been restored from flash, re-start the stack
     * and set the application running again.
     */


    if (sDeviceDesc.eNodeState == E_RUNNING)
    {
        uint8 u8DeviceType;
        ZPS_vDefaultKeyInit();
        ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_PRECONFIGURED_LINK_KEY, (uint8 *)&s_au8LnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);
        ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_ZLL_LINK_KEY,           (uint8 *)&s_au8ZllLnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);
        u8DeviceType = (sDeviceDesc.u8DeviceType >= 2)? 1 : sDeviceDesc.u8DeviceType;
        APP_vConfigureDevice(u8DeviceType);
        ZPS_eAplAfInit();

        if(u8DeviceType == 1)
        {
            vStartAndAnnounce();
        }
        else
        {
            ZPS_eAplZdoStartStack();
        }

    }
    else
    {

        ZPS_vDefaultKeyInit();
        ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_PRECONFIGURED_LINK_KEY, (uint8 *)&s_au8LnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);
        ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_ZLL_LINK_KEY,           (uint8 *)&s_au8ZllLnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);
        ZPS_eAplAfInit();
        sZllState.u8MyChannel = DEFAULT_CHANNEL;
        ZPS_vNwkNibSetChannel( ZPS_pvAplZdoGetNwkHandle(), DEFAULT_CHANNEL);
        ZPS_vNwkNibSetPanId(ZPS_pvAplZdoGetNwkHandle(), (uint16) RND_u32GetRand(1, 0xfff0) );

        sDeviceDesc.eNodeState = E_NETWORK_INIT;


    }
    ZPS_vExtendedStatusSetCallback(vfExtendedStatusCallBack);
    /* Set security state */
    APP_ZCL_vInitialise();
    vInitCommission();
}

/****************************************************************************
 *
 * NAME: vUnclaimedInterrupt
 *
 * DESCRIPTION:
 * Initialises Zigbee stack, hardware and application.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vUnclaimedInterrupt(void)
{

    register uint32 u32PICSR, u32PICMR;

    asm volatile ("l.mfspr %0,r0,0x4800" :"=r"(u32PICMR) : );
    asm volatile ("l.mfspr %0,r0,0x4802" :"=r"(u32PICSR) : );

    vLog_Printf(TRACE_APPSTART,LOG_EMERG, "Unclaimed interrupt : %x : %x\n", u32PICSR,u32PICSR);
    vSL_LogFlush();
    /* Log the exception */
    vLog_Printf(TRACE_EXC, LOG_CRIT, "\n\n\n%s EXCEPTION @ %08x (PICMR: %08x HP: %08x)", "UCMI", u32PICMR, u32PICSR, ((uint32 *)&_heap_location)[0]);
    vSL_LogFlush();
    /* Software reset */
    vAHI_SwReset();
}

OS_ISR(APP_isrStackOverflowException)
{

    vReportException("EXS");
}


OS_ISR(APP_isrUnimplementedModuleException)
{
    vReportException("EXM");
}

OS_ISR(APP_isrAlignmentException)
{
    vReportException("EXA");
}

OS_ISR(APP_isrBusErrorException)
{
    vReportException("EXB");
}

OS_ISR(APP_isrIllegalInstruction)
{
    vReportException("EXI");
}

PRIVATE void vReportException(char *sExStr)
{

    register uint32 u32EPCR, u32EEAR;
    volatile uint32 *pu32Stack;

    /* TODO - add reg dump too */

    asm volatile ("l.mfspr %0,r0,0x0020" :"=r"(u32EPCR) : );
    asm volatile ("l.mfspr %0,r0,0x0030" :"=r"(u32EEAR) : );
    asm volatile ("l.or %0,r0,r1" :"=r"(pu32Stack) : );


    vSL_LogFlush();
    /* Log the exception */
    vLog_Printf(TRACE_EXC, LOG_CRIT, "\n\n\n%s EXCEPTION @ %08x (EA: %08x SK: %08x HP: %08x)", sExStr, u32EPCR, u32EEAR, pu32Stack, ((uint32 *)&_heap_location)[0]);
    vSL_LogFlush();

    vLog_Printf(TRACE_EXC,LOG_CRIT, "Stack dump:\n");
    vSL_LogFlush();
#if (DEBUG_WDR == TRUE)
    vAHI_WatchdogStop();
#endif
    /* loop until we hit a 32k boundary. should be top of stack */
    while ((uint32)pu32Stack & 0x7fff) {
#if (DEBUG_WDR == TRUE)
        volatile uint32 u32Delay;
#endif
        vLog_Printf(TRACE_EXC,LOG_CRIT, "% 8x : %08x\n", pu32Stack, *pu32Stack);
        vSL_LogFlush();
        pu32Stack++;
#if (DEBUG_WDR == TRUE)
        for (u32Delay = 0; u32Delay < 100000; u32Delay++);
        vAHI_DioSetOutput(LED_DIO_PINS, 0);
        for (u32Delay = 0; u32Delay < 100000; u32Delay++);
        vAHI_DioSetOutput(0, LED_DIO_PINS);
#endif
    }
    vSL_LogFlush();
#if (DEBUG_WDR == FALSE)
    /* Software reset */
    vAHI_SwReset();
#endif
#if (DEBUG_WDR == TRUE)
    while(1)
    {
        volatile uint32 u32Delay;
        for (u32Delay = 0; u32Delay < 100000; u32Delay++);
        vAHI_DioSetOutput(LED_DIO_PINS, 0);
        for (u32Delay = 0; u32Delay < 100000; u32Delay++);
        vAHI_DioSetOutput(0, LED_DIO_PINS);
    }
#endif
}


/****************************************************************************
 *
 * NAME: vOSError
 *
 * DESCRIPTION:
 * Catches any unexpected OS errors
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vOSError(OS_teStatus eStatus, void *hObject)
{
    OS_thTask hTask;

    /* ignore queue underruns */
    if (OS_E_QUEUE_EMPTY == eStatus)
    {
        return;
    }

    vLog_Printf(TRACE_APPSTART,LOG_ERR, "OS Error %d, offending object handle = 0x%08x\n", eStatus, hObject);

    /* NB the task may have been pre-empted by an ISR which may be at fault */
    OS_eGetCurrentTask(&hTask);
    vLog_Printf(TRACE_APPSTART,LOG_ERR, "Currently active task handle = 0x%08x\n", hTask);
    vSL_LogFlush();
}

/****************************************************************************
 *
 * NAME: app_vFormatAndSendUpdateLists
 *
 * DESCRIPTION:
 * Catches any unexpected OS errors
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void app_vFormatAndSendUpdateLists(void)
{
    typedef struct
    {
        uint16* au16Attibutes;
        uint32 u32Asize;
        uint8* au8command;
        uint32 u32Csize;
    }tsAttribCommand;
    uint8 *pu8Buffer;
    uint8 u8Count;
    /*List of clusters per endpoint */

    uint16 u16ProfileId,u16ClusterId;
    uint16 u16ClusterListHA []= {0x0000,0x0001,0x0003,0x0004,0x0005,0x0006,0x0008,0x0019,0x0101,0x1000,0x0300,0x0201,0x0204,0x0405,0x0500,0x0400,0x0402,0x0403,0x0405,0x0406,0x0702,0x0b03,0x0b04};
    uint16 u16ClusterListZLL []= {0x1000};
    /*list of attributes per cluster */
    uint16 u16AttribBasic [] = {0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x4000};
    uint16 u16AttribIdentify [] = {0x000};
    uint16 u16AttribGroups [] = {0x000};
    uint16 u16AttribScenes [] = {0x0000,0x0001,0x0002,0x0003,0x0004,0x0005};
    uint16 u16AttribOnOff[]={0x0000,0x4000,0x4001,0x4002};
    uint16 u16AttribLevel[] = {0x0000,0x0001,0x0010,0x0011};
    uint16 u16AttribColour[] = {0x000,0x0001,0x0002,0x0007,0x0008,0x0010,0x0011,0x0012,0x0013,0x0015,0x0016,0x0017,0x0019,
                            0x001A,0x0020,0x0021,0x0022,0x0024,0x0025,0x0026,0x0028,0x0029,0x002A,0x4000,0x4001,0x4002,
                            0x4003,0x4004,0x4006,0x400A,0x400B,0x400C};
    uint16 u16AttribThermostat[] = {0x0000,0x0003,0x0004,0x0011,0x0012,0x001B,0x001C};
    uint16 u16AttribHum[] = {0x0000,0x0001,0x0002,0x0003};
    uint16 u16AttribPower[] = {0x0020,0x0034};
    uint16 u16AttribIllumM[] = {0x000,0x0001,0x0002,0x0003,0x0004};
    uint16 u16AttribIllumT[] = {0x000,0x0001,0x0002};
    uint16 u16AttribSM[] = {0x0000,0x0300,0x0301,0x0302,0x0306,0x0400};
    /*list of commands per cluster */
    uint8 u8CommandBasic[] = {0};
    uint8 u8CommandIdentify[] = {0,1,0x40};
    uint8 u8CommandGroups[] = {0,1,2,3,4,5};
    uint8 u8CommandScenes[] = {0,1,2,3,4,5,6,0x40,0x41,0x42};
    uint8 u8CommandsOnOff[] = {0,1,2,0x40,0x41,0x42};
    uint8 u8CommandsLevel[] = {0,1,2,3,4,5,6,7,8};
    uint8 u8CommandsColour[] = {0,1,2,3,4,5,6,7,8,9,0xa,0x40,0x41,0x42,0x43,0x44,0x47,0x4b,0x4c,0xfe,0xff};
    uint8 u8CommandsUtility[] = {0,1,2,3,6,7,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x40,0x41,0x42};
    uint8 u8CommandThermostat[] = {0};
    uint8 u8CommandHum[] = {0};
    uint8 u8CommandPower[] = {0};
    uint8 u8CommandIllumM[] = {0};
    uint8 u8CommandIllumT[] = {0};
    uint8 u8CommandSM[] = {0};
    tsAttribCommand asAttribCommand[13] = { {u16AttribBasic,sizeof(u16AttribBasic),u8CommandBasic, sizeof(u8CommandBasic)},
                                        {u16AttribIdentify,sizeof(u16AttribIdentify),u8CommandIdentify,sizeof(u8CommandIdentify)},
                                        {u16AttribGroups,sizeof(u16AttribGroups),u8CommandGroups,sizeof(u8CommandGroups)},
                                        {u16AttribScenes,sizeof(u16AttribScenes),u8CommandScenes,sizeof(u8CommandScenes)},
                                        {u16AttribOnOff,sizeof(u16AttribOnOff),u8CommandsOnOff,sizeof(u8CommandsOnOff)},
                                        {u16AttribLevel,sizeof(u16AttribLevel),u8CommandsLevel,sizeof(u8CommandsLevel)},
                                        {u16AttribColour,sizeof(u16AttribColour),u8CommandsColour,sizeof(u8CommandsColour)},
                                        {u16AttribThermostat,sizeof(u16AttribThermostat),u8CommandThermostat,sizeof(u8CommandThermostat)},
                                        {u16AttribHum,sizeof(u16AttribHum),u8CommandHum,sizeof(u8CommandHum)},
                                        {u16AttribPower,sizeof(u16AttribPower),u8CommandPower,sizeof(u8CommandPower)},
                                        {u16AttribIllumM,sizeof(u16AttribIllumM),u8CommandIllumM,sizeof(u8CommandIllumM)},
                                        {u16AttribIllumT,sizeof(u16AttribIllumT),u8CommandIllumT,sizeof(u8CommandIllumT)},
                                        {u16AttribSM,sizeof(u16AttribSM),u8CommandSM,sizeof(u8CommandSM)}};


    /* Cluster list endpoint HA */
        pu8Buffer = au8Buffer;
        *(pu8Buffer++) = ZIGBEENODECONTROLBRIDGE_HA_ENDPOINT;
        u16ProfileId = 0x0104;
        memcpy(pu8Buffer,&u16ProfileId,sizeof(uint16));
        pu8Buffer += sizeof(uint16);
        memcpy(pu8Buffer,u16ClusterListHA,sizeof(u16ClusterListHA));
        vSL_WriteMessage(E_SL_MSG_NODE_CLUSTER_LIST, (sizeof(uint8)+sizeof(uint16)+sizeof(u16ClusterListHA)), au8Buffer);


    /* Cluster list endpoint ZLL */
        pu8Buffer = au8Buffer;
        *(pu8Buffer++) = ZIGBEENODECONTROLBRIDGE_ZLL_COMMISSION_ENDPOINT;
        u16ProfileId = 0xC05E;
        memcpy(pu8Buffer,&u16ProfileId,sizeof(uint16));
        pu8Buffer += sizeof(uint16);
        memcpy(pu8Buffer,u16ClusterListZLL,sizeof(u16ClusterListZLL));
        vSL_WriteMessage(E_SL_MSG_NODE_CLUSTER_LIST, (sizeof(uint8)+sizeof(uint16)+sizeof(u16ClusterListZLL)), au8Buffer);

        /* Attribute list basic cluster HA EP*/
    for(u8Count=0;u8Count < 13; u8Count++)
    {
        pu8Buffer = au8Buffer;
        *(pu8Buffer++) = ZIGBEENODECONTROLBRIDGE_HA_ENDPOINT;
        u16ProfileId = 0x0104;
        memcpy(pu8Buffer,&u16ProfileId,sizeof(uint16));
        pu8Buffer += sizeof(uint16);
        u16ClusterId =u16ClusterListHA[u8Count];
        memcpy(pu8Buffer,&u16ClusterId,sizeof(uint16));
        pu8Buffer += sizeof(uint16);
        memcpy(pu8Buffer,asAttribCommand[u8Count].au16Attibutes,asAttribCommand[u8Count].u32Asize);
        vSL_WriteMessage(E_SL_MSG_NODE_ATTRIBUTE_LIST, (sizeof(uint8)+sizeof(uint16)+sizeof(uint16)+asAttribCommand[u8Count].u32Asize), au8Buffer);
        pu8Buffer = au8Buffer;
        *(pu8Buffer++) = ZIGBEENODECONTROLBRIDGE_HA_ENDPOINT;
        u16ProfileId = 0x0104;
        memcpy(pu8Buffer,&u16ProfileId,sizeof(uint16));
        pu8Buffer += sizeof(uint16);
        u16ClusterId =u16ClusterListHA[u8Count];
        memcpy(pu8Buffer,&u16ClusterId,sizeof(uint16));
        pu8Buffer += sizeof(uint16);
        memcpy(pu8Buffer,asAttribCommand[u8Count].au8command,asAttribCommand[u8Count].u32Csize);
        vSL_WriteMessage(E_SL_MSG_NODE_COMMAND_ID_LIST, (sizeof(uint8)+sizeof(uint16)+sizeof(uint16)+asAttribCommand[u8Count].u32Csize), au8Buffer);
    }

        pu8Buffer = au8Buffer;
        *(pu8Buffer++) = ZIGBEENODECONTROLBRIDGE_ZLL_COMMISSION_ENDPOINT;
        u16ProfileId = 0xC05E;
        memcpy(pu8Buffer,&u16ProfileId,sizeof(uint16));
        pu8Buffer += sizeof(uint16);
        u16ClusterId =u16ClusterListZLL[0];
        memcpy(pu8Buffer,&u16ClusterId,sizeof(uint16));
        pu8Buffer += sizeof(uint16);
        memcpy(pu8Buffer,u8CommandsUtility,sizeof(u8CommandsUtility));
        vSL_WriteMessage(E_SL_MSG_NODE_COMMAND_ID_LIST, (sizeof(uint8)+sizeof(uint16)+sizeof(uint16)+sizeof(u8CommandsUtility)), au8Buffer);

}
void vfExtendedStatusCallBack (ZPS_teExtendedStatus eExtendedStatus)
{
    DBG_vPrintf(TRUE,"ERROR: Extended status %x\n", eExtendedStatus);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
