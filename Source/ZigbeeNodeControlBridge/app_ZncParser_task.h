/*****************************************************************************
 *
 * MODULE: ZigbeeNodeControlBridge
 *
 * COMPONENT: app_ZncParser_task.h
 *
 * $AUTHOR: Faisal Bhaiyat$
 *
 * DESCRIPTION:
 *
 * $HeadURL: https://www.collabnet.nxp.com/svn/lprf_sware/Projects/Zigbee%20Protocol%20Stack/ZPS/Trunk/ZigbeeNodeControlBridge/Source/ZigbeeNodeControlBridge/app_ZncParser_task.h $
 *
 * $Revision: 54776 $
 *
 * $LastChangedBy: nxp29741 $
 *
 * $LastChangedDate: 2013-06-20 11:50:33 +0100 (Thu, 20 Jun 2013) $
 *
 * $Id: app_ZncParser_task.h 54776 2013-06-20 10:50:33Z nxp29741 $
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

#ifndef APP_ZNCPARSER_TASK_H_
#define APP_ZNCPARSER_TASK_H_

#if defined __cplusplus
extern "C" {
#endif

#include <jendefs.h>
#include <pdm.h>
#include "zps_apl_af.h"
#include "zcl.h"
#include "zcl_options.h"
#include "zll.h"
#include "zll_commission.h"
#include "zll_utility.h"
#include "Basic.h"
#include "Identify.h"
#include "OnOff.h"
#include "LevelControl.h"
#include "ColourControl.h"
#include "Groups.h"
#include "Scenes.h"
#include "IlluminanceLevelSensing.h"
#include "Thermostat.h"
#include "ThermostatUIConfig.h"
#include "TemperatureMeasurement.h"
#include "RelativeHumidityMeasurement.h"
#include "IlluminanceMeasurement.h"
#include "OccupancySensing.h"
#ifdef CLD_IASZONE
#include "IASZone.h"
#endif
#ifdef CLD_DOOR_LOCK
#include "DoorLock.h"
#endif
#ifdef CLD_SIMPLE_METERING
#include "SimpleMetering.h"
#endif
#ifdef CLD_OTA
#include "OTA.h"
#endif
#include "ApplianceStatistics.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Inlined Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/* Holds cluster instances */
typedef struct
{
    /*
     * All ZLL devices have following mandatory clusters
     */

    #if (defined CLD_BASIC) && (defined BASIC_CLIENT)
        tsZCL_ClusterInstance sBasicClient;
    #endif


    #if (defined CLD_BASIC) && (defined BASIC_SERVER)
        tsZCL_ClusterInstance sBasicServer;
    #endif

    #if (defined CLD_ZLL_UTILITY) && (defined ZLL_UTILITY_SERVER)
        tsZCL_ClusterInstance sZllUtilityServer;
    #endif

    #if (defined CLD_ZLL_UTILITY) && (defined ZLL_UTILITY_CLIENT)
        tsZCL_ClusterInstance sZllUtilityClient;
    #endif

    /*
     * Control Bridge Device has 6 other mandatory clusters for the client
     */

    #if (defined CLD_IDENTIFY) && (defined IDENTIFY_CLIENT)
        tsZCL_ClusterInstance sIdentifyClient;
    #endif

    #if (defined CLD_ONOFF) && (defined ONOFF_CLIENT)
        tsZCL_ClusterInstance sOnOffClient;
    #endif

    #if (defined CLD_ONOFF) && (defined ONOFF_SERVER)
        tsZCL_ClusterInstance sOnOffServer;
    #endif

    #if (defined CLD_GROUPS) && (defined GROUPS_CLIENT)
        tsZCL_ClusterInstance sGroupsClient;
    #endif

    #if (defined CLD_GROUPS) && (defined GROUPS_SERVER)
        tsZCL_ClusterInstance sGroupsServer;
    #endif

    #if (defined CLD_LEVEL_CONTROL) && (defined LEVEL_CONTROL_CLIENT)
        tsZCL_ClusterInstance sLevelControlClient;
    #endif

    #if (defined CLD_COLOUR_CONTROL) && (defined COLOUR_CONTROL_CLIENT)
        tsZCL_ClusterInstance sColourControlClient;
    #endif

    #if (defined CLD_SCENES) && (defined SCENES_CLIENT)
        tsZCL_ClusterInstance sScenesClient;
    #endif

    #if (defined CLD_IASZONE) && (defined IASZONE_CLIENT)
        tsZCL_ClusterInstance sIASZoneClient;
    #endif

    #if (defined CLD_DOOR_LOCK) && (defined DOOR_LOCK_CLIENT)
        tsZCL_ClusterInstance sDoorLockClient;
    #endif
    #if (defined CLD_THERMOSTAT) && (defined THERMOSTAT_CLIENT)
        tsZCL_ClusterInstance sThermostatClient;
    #endif
    #if (defined CLD_SIMPLE_METERING) && (defined SM_CLIENT)
        tsZCL_ClusterInstance sMeteringClient;
    #endif

    #if (defined CLD_OTA) && (defined OTA_SERVER)
        tsZCL_ClusterInstance sOTAServerClusterInst;
    #endif
    #if (defined CLD_TEMPERATURE_MEASUREMENT) && (defined TEMPERATURE_MEASUREMENT_CLIENT)
        tsZCL_ClusterInstance sTemperatureMeasurementClient;
    #endif

    #if (defined CLD_RELATIVE_HUMIDITY_MEASUREMENT) && (defined RELATIVE_HUMIDITY_MEASUREMENT_CLIENT)
        tsZCL_ClusterInstance sRelativeHumidityMeasurementClient;
    #endif

    #if (defined CLD_ILLUMINANCE_MEASUREMENT) && (defined ILLUMINANCE_MEASUREMENT_CLIENT)
        tsZCL_ClusterInstance sIlluminanceMeasurementClient;
    #endif

    #if (defined CLD_APPLIANCE_STATISTICS) && (defined APPLIANCE_STATISTICS_CLIENT)
        tsZCL_ClusterInstance sASCClient;
    #endif
    #if (defined CLD_APPLIANCE_STATISTICS) && (defined APPLIANCE_STATISTICS_SERVER)
        tsZCL_ClusterInstance sASCServer;
    #endif
} tsZLL_ZncControlBridgeDeviceClusterInstances;


/* Holds everything required to create an instance of a Control Bridge */
typedef struct
{
    tsZCL_EndPointDefinition sEndPoint;

    /* Cluster instances */
    tsZLL_ZncControlBridgeDeviceClusterInstances sClusterInstance;
    /* Clients - General */
    #if (defined CLD_BASIC) && (defined BASIC_CLIENT)
    /* Basic Cluster - Client */
        tsCLD_Basic sBasicClientCluster;
    #endif


    #if (defined CLD_BASIC) && (defined BASIC_SERVER)
        /* Basic Cluster - Server */
        tsCLD_Basic sBasicServerCluster;
    #endif

    #if (defined CLD_ZLL_UTILITY) && (defined ZLL_UTILITY_SERVER)
        /* Utility Cluster - Server */
        tsCLD_ZllUtility                         sZllUtilityServerCluster;
        tsCLD_ZllUtilityCustomDataStructure  sZllUtilityServerCustomDataStructure;
    #endif

    #if (defined CLD_ZLL_UTILITY) && (defined ZLL_UTILITY_CLIENT)
        /* Utility Cluster - Client */
        tsCLD_ZllUtility                         sZllUtilityClientCluster;
        tsCLD_ZllUtilityCustomDataStructure  sZllUtilityClientCustomDataStructure;
    #endif

    /*
     * Mandatory client clusters
     */
    #if (defined CLD_IDENTIFY) && (defined IDENTIFY_CLIENT)
        /* Identify Cluster - Client */
        tsCLD_Identify sIdentifyClientCluster;
        tsCLD_IdentifyCustomDataStructure sIdentifyClientCustomDataStructure;
    #endif

    #if (defined CLD_ONOFF) && (defined ONOFF_CLIENT)
        /* On/Off Cluster - Client */
        tsCLD_OnOff sOnOffClientCluster;
    #endif

    #if (defined CLD_ONOFF) && (defined ONOFF_SERVER)
        /* On/Off Cluster - Client */
        tsCLD_OnOff sOnOffServerCluster;
        tsCLD_OnOffCustomDataStructure sOnOffServerCustomDataStructure;
    #endif

    #if (defined CLD_LEVEL_CONTROL) && (defined LEVEL_CONTROL_CLIENT)
        /* Level Control Cluster - Client */
        tsCLD_LevelControl sLevelControlClientCluster;
        tsCLD_LevelControlCustomDataStructure sLevelControlClientCustomDataStructure;
    #endif

    #if (defined CLD_SCENES) && (defined SCENES_CLIENT)
        /* Scenes Cluster - Client */
        tsCLD_Scenes sScenesClientCluster;
        tsCLD_ScenesCustomDataStructure sScenesClientCustomDataStructure;
    #endif

    #if (defined CLD_GROUPS) && (defined GROUPS_CLIENT)
        /* Groups Cluster - Client */
        tsCLD_Groups sGroupsClientCluster;
        tsCLD_GroupsCustomDataStructure sGroupsClientCustomDataStructure;
    #endif

    #if (defined CLD_GROUPS) && (defined GROUPS_SERVER)
        /* Groups Cluster - Server */
        tsCLD_Groups sGroupsServerCluster;
        tsCLD_GroupsCustomDataStructure sGroupsServerCustomDataStructure;
    #endif

    #if (defined CLD_COLOUR_CONTROL) && (defined COLOUR_CONTROL_CLIENT)
        /* Colour Control Cluster - Client */
        tsCLD_ColourControl sColourControlClientCluster;
        tsCLD_ColourControlCustomDataStructure sColourControlClientCustomDataStructure;
    #endif

    #if (defined CLD_IASZONE) && (defined IASZONE_CLIENT)
        /* IAS Zone - Client */
        tsCLD_IASZone sIASZoneClientCluster;
        tsCLD_IASZone_CustomDataStructure sIASZoneClientCustomDataStructure;
    #endif

    #if (defined CLD_DOOR_LOCK) && (defined DOOR_LOCK_CLIENT)
        /* Colour Control Cluster - Client */
        tsCLD_DoorLock sDoorLockClientCluster;
    #endif

    #if (defined CLD_SIMPLE_METERING) && (defined SM_CLIENT)
        /* Simple Metering Cluster - Client */
        tsCLD_SimpleMetering sMeteringClientCluster;
        tsSM_CustomStruct sMeteringClientCustomDataStructure;
    #endif


#if (defined CLD_THERMOSTAT) && (defined THERMOSTAT_CLIENT)
    tsCLD_Thermostat sThermostatClientCluster;
    tsCLD_ThermostatCustomDataStructure sThermostatClientCustomDataStructure;
#endif

#if (defined CLD_TEMPERATURE_MEASUREMENT) && (defined TEMPERATURE_MEASUREMENT_CLIENT)
    tsCLD_TemperatureMeasurement sTemperatureMeasurementClientCluster;
#endif

#if (defined CLD_RELATIVE_HUMIDITY_MEASUREMENT) && (defined RELATIVE_HUMIDITY_MEASUREMENT_CLIENT)
    tsCLD_RelativeHumidityMeasurement sRelativeHumidityMeasurementClientCluster;
#endif
#if (defined CLD_ILLUMINANCE_MEASUREMENT) && (defined ILLUMINANCE_MEASUREMENT_CLIENT)
    tsCLD_IlluminanceMeasurement sIlluminanceMeasurementClientCluster;
#endif
    /* Mandatory client clusters */
#if (defined CLD_APPLIANCE_STATISTICS) && (defined APPLIANCE_STATISTICS_CLIENT)
    /* Basic Cluster - Client */
    tsCLD_ApplianceStatisticsCustomDataStructure sASCClientCustomDataStructure;
#endif
#if (defined CLD_APPLIANCE_STATISTICS) && (defined APPLIANCE_STATISTICS_SERVER)
    tsZCL_ClusterInstance sASCServerCluster;
    tsCLD_ApplianceStatisticsCustomDataStructure sASCServerCustomDataStructure;
#endif

} tsZLL_ZncControlBridgeDevice;

extern tsZLL_ZncControlBridgeDevice sControlBridge;
extern uint32 u32OldFrameCtr;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC teZCL_Status eZLL_RegisterZncControlBridgeEndPoint(uint8 u8EndPointIdentifier,
                                              tfpZCL_ZCLCallBackFunction cbCallBack,
                                              tsZLL_ZncControlBridgeDevice *psDeviceInfo);

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif
PUBLIC void APP_vSetLeds(bool bLed1On, bool bLed2On);
PUBLIC void vSetGroupAddress(uint16 u16GroupStart, uint8 u8NumGroups);
PUBLIC void vFactoryResetRecords( void);

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /* APP_ZNCPARSER_TASK_H_ */
