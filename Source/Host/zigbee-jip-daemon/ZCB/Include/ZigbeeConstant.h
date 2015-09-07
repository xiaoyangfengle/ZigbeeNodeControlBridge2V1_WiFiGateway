/****************************************************************************
 *
 * MODULE:             Linux Zigbee - JIP daemon
 *
 * COMPONENT:          Interface to Zigbee control bridge
 *
 * REVISION:           $Revision: 43420 $
 *
 * DATED:              $Date: 2012-06-18 15:13:17 +0100 (Mon, 18 Jun 2012) $
 *
 * AUTHOR:             Lee Mitchell
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139]. 
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

 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

#ifndef  ZIGBEECONSTANT_H_INCLUDED
#define  ZIGBEECONSTANT_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define ZB_DEFAULT_ENDPOINT_ZLL             3

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/** Enumerated type of ZigBee address modes */
typedef enum
{
    E_ZB_ADDRESS_MODE_BOUND                 = 0x00,
    E_ZB_ADDRESS_MODE_GROUP                 = 0x01,
    E_ZB_ADDRESS_MODE_SHORT                 = 0x02,
    E_ZB_ADDRESS_MODE_IEEE                  = 0x03,
    E_ZB_ADDRESS_MODE_BROADCAST             = 0x04,
    E_ZB_ADDRESS_MODE_NO_TRANSMIT           = 0x05,
    E_ZB_ADDRESS_MODE_BOUND_NO_ACK          = 0x06,
    E_ZB_ADDRESS_MODE_SHORT_NO_ACK          = 0x07,
    E_ZB_ADDRESS_MODE_IEEE_NO_ACK           = 0x08,
} eZigbee_AddressMode;

/** Enumerated type of Zigbee MAC Capabilities */
typedef enum
{
    E_ZB_MAC_CAPABILITY_ALT_PAN_COORD       = 0x01,
    E_ZB_MAC_CAPABILITY_FFD                 = 0x02,
    E_ZB_MAC_CAPABILITY_POWERED             = 0x04,
    E_ZB_MAC_CAPABILITY_RXON_WHEN_IDLE      = 0x08,
    E_ZB_MAC_CAPABILITY_SECURED             = 0x40,
    E_ZB_MAC_CAPABILITY_ALLOCATE_ADDRESS    = 0x80,
} teZigbee_MACCapability;

/** Enumerated type of Zigbee broadcast addresses */
typedef enum
{
    E_ZB_BROADCAST_ADDRESS_ALL              = 0xFFFF,
    E_ZB_BROADCAST_ADDRESS_RXONWHENIDLE     = 0xFFFD,
    E_ZB_BROADCAST_ADDRESS_ROUTERS          = 0xFFFC,
    E_ZB_BROADCAST_ADDRESS_LOWPOWERROUTERS  = 0xFFFB,
} eZigbee_BroadcastAddress;


/** Enumerated type of Zigbee Device IDs */
typedef enum
{
    E_ZB_DEVICEID_COLORLAMP                 = 0x0200,
    E_ZB_DEVICEID_CONTROLBRIDGE             = 0x0840,
} eZigbee_DeviceID;


/** Enumerated type of Zigbee Profile IDs */
typedef enum
{
    E_ZB_PROFILEID_HA                       = 0x0104,
    E_ZB_PROFILEID_ZLL                      = 0xC05E,    
} eZigbee_ProfileID;


/** Enumerated type of Zigbee Cluster IDs */
typedef enum
{
    E_ZB_CLUSTERID_BASIC                    = 0x0000,
    E_ZB_CLUSTERID_POWER                    = 0x0001,
    E_ZB_CLUSTERID_DEVICE_TEMPERATURE       = 0x0002,
    E_ZB_CLUSTERID_IDENTIFY                 = 0x0003,
    E_ZB_CLUSTERID_GROUPS                   = 0x0004,
    E_ZB_CLUSTERID_SCENES                   = 0x0005,
    E_ZB_CLUSTERID_ONOFF                    = 0x0006,
    E_ZB_CLUSTERID_ONOFF_CONFIGURATION      = 0x0007,
    E_ZB_CLUSTERID_LEVEL_CONTROL            = 0x0008,
    E_ZB_CLUSTERID_ALARMS                   = 0x0009,
    E_ZB_CLUSTERID_TIME                     = 0x000A,
    E_ZB_CLUSTERID_RSSI_LOCATION            = 0x000B,
    E_ZB_CLUSTERID_ANALOG_INPUT_BASIC       = 0x000C,
    E_ZB_CLUSTERID_ANALOG_OUTPUT_BASIC      = 0x000D,
    E_ZB_CLUSTERID_VALUE_BASIC              = 0x000E,
    E_ZB_CLUSTERID_BINARY_INPUT_BASIC       = 0x000F,
    E_ZB_CLUSTERID_BINARY_OUTPUT_BASIC      = 0x0010,
    E_ZB_CLUSTERID_BINARY_VALUE_BASIC       = 0x0011,
    E_ZB_CLUSTERID_MULTISTATE_INPUT_BASIC   = 0x0012,
    E_ZB_CLUSTERID_MULTISTATE_OUTPUT_BASIC  = 0x0013,
    E_ZB_CLUSTERID_MULTISTATE_VALUE_BASIC   = 0x0014,
    E_ZB_CLUSTERID_COMMISSIONING            = 0x0015,
    
    /* HVAC */
    E_ZB_CLUSTERID_THERMOSTAT               = 0x0201,
    
    /* Lighting */
    E_ZB_CLUSTERID_COLOR_CONTROL            = 0x0300,
    E_ZB_CLUSTERID_BALLAST_CONFIGURATION    = 0x0301,
    
    /* Sensing */
    E_ZB_CLUSTERID_TEMPERATURE              = 0x0402,
    
    /* ZLL */
    E_ZB_CLUSTERID_ZLL_COMMISIONING         = 0x1000,
} eZigbee_ClusterID;

/** Enumerated type of attributes in the Basic Cluster */
typedef enum
{
    E_ZB_ATTRIBUTEID_BASIC_ZCL_VERSION      = 0x0000,
    E_ZB_ATTRIBUTEID_BASIC_APP_VERSION      = 0x0001,
    E_ZB_ATTRIBUTEID_BASIC_STACK_VERSION    = 0x0002,
    E_ZB_ATTRIBUTEID_BASIC_HW_VERSION       = 0x0003,
    E_ZB_ATTRIBUTEID_BASIC_MAN_NAME         = 0x0004,
    E_ZB_ATTRIBUTEID_BASIC_MODEL_ID         = 0x0005,
    E_ZB_ATTRIBUTEID_BASIC_DATE_CODE        = 0x0006,
    E_ZB_ATTRIBUTEID_BASIC_POWER_SOURCE     = 0x0007,
    
    E_ZB_ATTRIBUTEID_BASIC_LOCATION_DESC    = 0x0010,
    E_ZB_ATTRIBUTEID_BASIC_PHYSICAL_ENV     = 0x0011,
    E_ZB_ATTRIBUTEID_BASIC_DEVICE_ENABLED   = 0x0012,
    E_ZB_ATTRIBUTEID_BASIC_ALARM_MASK       = 0x0013,
    E_ZB_ATTRIBUTEID_BASIC_DISBALELOCALCONF = 0x0014,
} eZigbee_AttributeIDBasicCluster;


/** Enumerated type of attributes in the Scenes Cluster */
typedef enum
{
    E_ZB_ATTRIBUTEID_SCENE_SCENECOUNT       = 0x0000,
    E_ZB_ATTRIBUTEID_SCENE_CURRENTSCENE     = 0x0001,
    E_ZB_ATTRIBUTEID_SCENE_CURRENTGROUP     = 0x0002,
    E_ZB_ATTRIBUTEID_SCENE_SCENEVALID       = 0x0003,
    E_ZB_ATTRIBUTEID_SCENE_NAMESUPPORT      = 0x0004,
    E_ZB_ATTRIBUTEID_SCENE_LASTCONFIGUREDBY = 0x0005,
} eZigbee_AttributeIDScenesCluster;


/** Enumerated type of attributes in the ON/Off Cluster */
typedef enum
{
    E_ZB_ATTRIBUTEID_ONOFF_ONOFF            = 0x0000,
    E_ZB_ATTRIBUTEID_ONOFF_GLOBALSCENE      = 0x4000,
    E_ZB_ATTRIBUTEID_ONOFF_ONTIME           = 0x4001,
    E_ZB_ATTRIBUTEID_ONOFF_OFFWAITTIME      = 0x4002,
} eZigbee_AttributeIDOnOffCluster;


/** Enumerated type of attributes in the Level Control Cluster */
typedef enum
{
    E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL     = 0x0000,
    E_ZB_ATTRIBUTEID_LEVEL_REMAININGTIME    = 0x0001,
    E_ZB_ATTRIBUTEID_LEVEL_ONOFFTRANSITION  = 0x0010,
    E_ZB_ATTRIBUTEID_LEVEL_ONLEVEL          = 0x0011,
} eZigbee_AttributeIDLevelControlCluster;


/** Enumerated type of attributes in the Level Control Cluster */
typedef enum
{
    E_ZB_ATTRIBUTEID_COLOUR_CURRENTHUE      = 0x0000,
    E_ZB_ATTRIBUTEID_COLOUR_CURRENTSAT      = 0x0001,
    E_ZB_ATTRIBUTEID_COLOUR_REMAININGTIME   = 0x0002,
    E_ZB_ATTRIBUTEID_COLOUR_CURRENTX        = 0x0003,
    E_ZB_ATTRIBUTEID_COLOUR_CURRENTY        = 0x0004,
    E_ZB_ATTRIBUTEID_COLOUR_DRIFTCOMPENSATION   = 0x0005,
    E_ZB_ATTRIBUTEID_COLOUR_COMPENSATIONTEXT    = 0x0006,
    E_ZB_ATTRIBUTEID_COLOUR_COLOURTEMPERATURE   = 0x0007,
    E_ZB_ATTRIBUTEID_COLOUR_COLOURMODE      = 0x0008,
    
    
    E_ZB_ATTRIBUTEID_COLOUR_COLOURTEMP_PHYMIN   = 0x400b,
    E_ZB_ATTRIBUTEID_COLOUR_COLOURTEMP_PHYMAX   = 0x400c,
    
} eZigbee_AttributeIDColourControlCluster;

/** Enumerated type of attributes in the Thermostat Cluster */
typedef enum
{
    E_ZB_ATTRIBUTEID_TSTAT_LOCALTEMPERATURE         = 0x0000,
    E_ZB_ATTRIBUTEID_TSTAT_OUTDOORTEMPERATURE       = 0x0001,
    E_ZB_ATTRIBUTEID_TSTAT_OCCUPANCY                = 0x0002,
    E_ZB_ATTRIBUTEID_TSTAT_ABSMINHEATSETPOINTLIMIT  = 0x0003,
    E_ZB_ATTRIBUTEID_TSTAT_ABSMAXHEATSETPOINTLIMIT  = 0x0004,
    E_ZB_ATTRIBUTEID_TSTAT_ABSMINCOOLSETPOINTLIMIT  = 0x0005,
    E_ZB_ATTRIBUTEID_TSTAT_ABSMAXCOOLSETPOINTLIMIT  = 0x0006,
    E_ZB_ATTRIBUTEID_TSTAT_PICOOLINGDEMAND          = 0x0007,
    E_ZB_ATTRIBUTEID_TSTAT_PIHEATINGDEMAND          = 0x0008,
    
    E_ZB_ATTRIBUTEID_TSTAT_LOCALTEMPERATURECALIB    = 0x0010,
    E_ZB_ATTRIBUTEID_TSTAT_OCCUPIEDCOOLSETPOINT     = 0x0011,
    E_ZB_ATTRIBUTEID_TSTAT_OCCUPIEDHEATSETPOINT     = 0x0012,
    E_ZB_ATTRIBUTEID_TSTAT_UNOCCUPIEDCOOLSETPOINT   = 0x0013,
    E_ZB_ATTRIBUTEID_TSTAT_UNOCCUPIEDHEATSETPOINT   = 0x0014,
    E_ZB_ATTRIBUTEID_TSTAT_MINHEATSETPOINTLIMIT     = 0x0015,
    E_ZB_ATTRIBUTEID_TSTAT_MAXHEATSETPOINTLIMIT     = 0x0016,
    E_ZB_ATTRIBUTEID_TSTAT_MINCOOLSETPOINTLIMIT     = 0x0017,
    E_ZB_ATTRIBUTEID_TSTAT_MAXCOOLSETPOINTLIMIT     = 0x0018,
    E_ZB_ATTRIBUTEID_TSTAT_MINSETPOINTDEADBAND      = 0x0019,
    E_ZB_ATTRIBUTEID_TSTAT_REMOTESENSING            = 0x001A,
    E_ZB_ATTRIBUTEID_TSTAT_COLTROLSEQUENCEOFOPERATION = 0x001B,
    E_ZB_ATTRIBUTEID_TSTAT_SYSTEMMODE               = 0x001C,
    E_ZB_ATTRIBUTEID_TSTAT_ALARMMASK                = 0x001D,
} eZigbee_AttributeIDThermostatCluster;


/** Enumerated type of attributes in the Temperature sensing Cluster */
typedef enum
{
     E_ZB_ATTRIBUTEID_TEMPERATURE_MEASURED         = 0x0000,
     E_ZB_ATTRIBUTEID_TEMPERATURE_MEASURED_MIN     = 0x0001,
     E_ZB_ATTRIBUTEID_TEMPERATURE_MEASURED_MAX     = 0x0002,
     E_ZB_ATTRIBUTEID_TEMPERATURE_TOLERANCE        = 0x0003,
} eZigbee_AttributeIDTemperatureCluster;


/** Enumerated type of attribute data types from ZCL */
typedef enum PACK
{
    /* Null */
    E_ZCL_NULL            = 0x00,
 
    /* General Data */
    E_ZCL_GINT8           = 0x08,                // General 8 bit - not specified if signed or not
    E_ZCL_GINT16,
    E_ZCL_GINT24,
    E_ZCL_GINT32,
    E_ZCL_GINT40,
    E_ZCL_GINT48,
    E_ZCL_GINT56,
    E_ZCL_GINT64,
 
    /* Logical */
    E_ZCL_BOOL            = 0x10,
 
    /* Bitmap */
    E_ZCL_BMAP8            = 0x18,                // 8 bit bitmap
    E_ZCL_BMAP16,
    E_ZCL_BMAP24,
    E_ZCL_BMAP32,
    E_ZCL_BMAP40,
    E_ZCL_BMAP48,
    E_ZCL_BMAP56,
    E_ZCL_BMAP64,
 
    /* Unsigned Integer */
    E_ZCL_UINT8           = 0x20,                // Unsigned 8 bit
    E_ZCL_UINT16,
    E_ZCL_UINT24,
    E_ZCL_UINT32,
    E_ZCL_UINT40,
    E_ZCL_UINT48,
    E_ZCL_UINT56,
    E_ZCL_UINT64,
 
    /* Signed Integer */
    E_ZCL_INT8            = 0x28,                // Signed 8 bit
    E_ZCL_INT16,
    E_ZCL_INT24,
    E_ZCL_INT32,
    E_ZCL_INT40,
    E_ZCL_INT48,
    E_ZCL_INT56,
    E_ZCL_INT64,
 
    /* Enumeration */
    E_ZCL_ENUM8            = 0x30,                // 8 Bit enumeration
    E_ZCL_ENUM16,
 
    /* Floating Point */
    E_ZCL_FLOAT_SEMI    = 0x38,                // Semi precision
    E_ZCL_FLOAT_SINGLE,                        // Single precision
    E_ZCL_FLOAT_DOUBLE,                        // Double precision
 
    /* String */
    E_ZCL_OSTRING        = 0x41,                // Octet string
    E_ZCL_CSTRING,                            // Character string
    E_ZCL_LOSTRING,                            // Long octet string
    E_ZCL_LCSTRING,                            // Long character string
 
    /* Ordered Sequence */
    E_ZCL_ARRAY          = 0x48,
    E_ZCL_STRUCT         = 0x4c,
 
    E_ZCL_SET            = 0x50,
    E_ZCL_BAG            = 0x51,
 
    /* Time */
    E_ZCL_TOD            = 0xe0,                // Time of day
    E_ZCL_DATE,                                // Date
    E_ZCL_UTCT,                                // UTC Time
 
    /* Identifier */
    E_ZCL_CLUSTER_ID    = 0xe8,                // Cluster ID
    E_ZCL_ATTRIBUTE_ID,                        // Attribute ID
    E_ZCL_BACNET_OID,                        // BACnet OID
 
    /* Miscellaneous */
    E_ZCL_IEEE_ADDR        = 0xf0,                // 64 Bit IEEE Address
    E_ZCL_KEY_128,                            // 128 Bit security key, currently not supported as it would add to code space in u16ZCL_WriteTypeNBO and add extra 8 bytes to report config record for each reportable attribute
 
    /* NOTE:
     * 0xfe is a reserved value, however we are using it to denote a message signature.
     * This may have to change some time if ZigBee ever allocate this value to a data type
     */
    E_ZCL_SIGNATURE     = 0xfe,             // ECDSA Signature (42 bytes)
 
    /* Unknown */
    E_ZCL_UNKNOWN        = 0xff
 
} teZCL_ZCLAttributeType;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* ZIGBEECONSTANT_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

