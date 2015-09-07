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

#ifndef  MODULECONFIG_H_INCLUDED
#define  MODULECONFIG_H_INCLUDED

#include <stdint.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "ZigbeeConstant.h"
#include "Utils.h"

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/* Default network configuration */
#define CONFIG_DEFAULT_START_MODE       E_START_COORDINATOR
#define CONFIG_DEFAULT_CHANNEL          15
#define CONFIG_DEFAULT_PANID            0x1234567812345678ll

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/** Enumerated type of statuses - This fits in with the Zigbee ZCL status codes */
typedef enum
{
    /* Zigbee ZCL status codes */
    E_ZCB_OK                            = 0x00,
    E_ZCB_ERROR                         = 0x01,
    
    /* ZCB internal status codes */
    E_ZCB_ERROR_NO_MEM                  = 0x10,
    E_ZCB_COMMS_FAILED                  = 0x11,
    E_ZCB_UNKNOWN_NODE                  = 0x12,
    E_ZCB_UNKNOWN_ENDPOINT              = 0x13,
    E_ZCB_UNKNOWN_CLUSTER               = 0x14,
    E_ZCB_REQUEST_NOT_ACTIONED          = 0x15,
    
    /* Zigbee ZCL status codes */
    E_ZCB_NOT_AUTHORISED                = 0x7E, 
    E_ZCB_RESERVED_FIELD_NZERO          = 0x7F,
    E_ZCB_MALFORMED_COMMAND             = 0x80,
    E_ZCB_UNSUP_CLUSTER_COMMAND         = 0x81,
    E_ZCB_UNSUP_GENERAL_COMMAND         = 0x82,
    E_ZCB_UNSUP_MANUF_CLUSTER_COMMAND   = 0x83,
    E_ZCB_UNSUP_MANUF_GENERAL_COMMAND   = 0x84,
    E_ZCB_INVALID_FIELD                 = 0x85,
    E_ZCB_UNSUP_ATTRIBUTE               = 0x86,
    E_ZCB_INVALID_VALUE                 = 0x87,
    E_ZCB_READ_ONLY                     = 0x88,
    E_ZCB_INSUFFICIENT_SPACE            = 0x89,
    E_ZCB_DUPLICATE_EXISTS              = 0x8A,
    E_ZCB_NOT_FOUND                     = 0x8B,
    E_ZCB_UNREPORTABLE_ATTRIBUTE        = 0x8C,
    E_ZCB_INVALID_DATA_TYPE             = 0x8D,
    E_ZCB_INVALID_SELECTOR              = 0x8E,
    E_ZCB_WRITE_ONLY                    = 0x8F,
    E_ZCB_INCONSISTENT_STARTUP_STATE    = 0x90,
    E_ZCB_DEFINED_OUT_OF_BAND           = 0x91,
    E_ZCB_INCONSISTENT                  = 0x92,
    E_ZCB_ACTION_DENIED                 = 0x93,
    E_ZCB_TIMEOUT                       = 0x94,
    
    E_ZCB_HARDWARE_FAILURE              = 0xC0,
    E_ZCB_SOFTWARE_FAILURE              = 0xC1,
    E_ZCB_CALIBRATION_ERROR             = 0xC2,
} teZcbStatus;


/** Enumerated type of events */
typedef enum
{
    E_ZCB_EVENT_NETWORK_FORMED,         /**< Control bridge formed a new network.
                                         *   pvData points to the control bridge \ref tsZCB_Node structure */
    E_ZCB_EVENT_NETWORK_JOINED,         /**< Control bridge joined an existing network.
                                         *   pvData points to the control bridge \ref tsZCB_Node structure */
    E_ZCB_EVENT_DEVICE_ANNOUNCE,        /**< A new device joined the network */
    E_ZCB_EVENT_DEVICE_MATCH,           /**< A device responded to a match descriptor request */
    E_ZCB_EVENT_ATTRIBUTE_REPORT,       /**< A device has sent us an attribute report */
    E_ZCB_EVENT_DEVICE_LEFT,            /**< A device has left the network (direct report or detected) */
} teZcbEvent;


/** Union type for all Zigbee attribute data types */
typedef union
{
    uint8_t     u8Data;
    uint16_t    u16Data;
    uint32_t    u32Data;
    uint64_t    u64Data;
} tuZcbAttributeData;


/** Structure passed from ZCB library to application for an event. */
typedef struct
{
    teZcbEvent                      eEvent;
    union {
        struct 
        {
        } sNetworkFormed;
        struct 
        {
        } sNetworkJoined;
        struct
        {
            uint16_t                u16ShortAddress;
        } sDeviceAnnounce;
        struct 
        {
            uint16_t                u16ShortAddress;
        } sDeviceMatch;
        struct
        {
            uint16_t                u16ShortAddress;
            uint8_t                 u8Endpoint;
            uint16_t                u16ClusterID;
            uint16_t                u16AttributeID;
            teZCL_ZCLAttributeType  eType;
            tuZcbAttributeData      uData;
        } sAttributeReport;
        struct
        {
            uint64_t                u64IEEEAddress;
            uint8_t                 bRejoin;
        } sDeviceLeft;
    } uData;
} tsZcbEvent;


/** Enumerated type of start modes */
typedef enum
{
    E_START_COORDINATOR     = 0,        /**< Start module as a coordinator */
    E_START_ROUTER          = 1,        /**< Start module as a router */
    E_START_TOUCHLINK       = 2,        /**< Start module as a touchlink router */
} teStartMode;


/** Enumerated type of module modes */
typedef enum
{
    E_MODE_COORDINATOR      = 0,        /**< Start module as a coordinator */
    E_MODE_ROUTER           = 1,        /**< Start module as a router */
    E_MODE_HA_COMPATABILITY = 2,        /**< Start module as router in HA compatability mode */
} teModuleMode;


/** Enumerated type of allowable channels */
typedef enum
{
    E_CHANNEL_AUTOMATIC     = 0,
    E_CHANNEL_MINIMUM       = 11,
    E_CHANNEL_MAXIMUM       = 26
} teChannel;


/** Enumerated type of supported authorisation schemes */
typedef enum
{
    E_AUTH_SCHEME_NONE,
    E_AUTH_SCHEME_RADIUS_PAP,
} teAuthScheme;


/** Per authorisation scheme union of required configuration data */
typedef union
{
    struct
    {
        struct in6_addr sAuthServerIP;
    } sRadiusPAP;
} tuAuthSchemeData;



/** Structure representing a cluster on a node */
typedef struct _tsZCB_NodeCluster
{
    uint16_t            *pau16Attributes;
    uint8_t             *pau8Commands;
    
    uint32_t            u32NumAttributes;
    uint32_t            u32NumCommands;
    
    uint16_t            u16ClusterID;
} tsZCB_NodeCluster;


typedef struct
{
    tsZCB_NodeCluster   *pasClusters;
    
    uint32_t            u32NumClusters;
    
    uint16_t            u16ProfileID;
    
    uint8_t             u8Endpoint;
} tsZCB_NodeEndpoint;


typedef struct _tsZCB_Node
{
    struct _tsZCB_Node  *psNext;
    
    tsZCB_NodeEndpoint  *pasEndpoints;
    
    uint16_t            *pau16Groups;
    
    tsUtilsLock         sLock;
    
    struct
    {
        struct timeval  sLastSuccessful;        /**< Time of last successful communications */
        uint16_t        u16SequentialFailures;  /**< Number of sequential failures */
    } sComms;                                   /**< Structure containing communications statistics */
    
    uint64_t            u64IEEEAddress;

    uint32_t            u32NumEndpoints;
    uint32_t            u32NumGroups;
    
    uint16_t            u16ShortAddress;
    uint16_t            u16DeviceID;
    uint8_t             u8MacCapability;
    
    uint8_t             u8LastNeighbourTableIndex;
} tsZCB_Node;


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/** Start control bridge mode */
extern teStartMode      eStartMode;


/** Channel number to operate on */
extern teChannel        eChannel;


/** IEEE802.15.4 extended PAN ID configured */
extern uint64_t         u64PanID;


/** Channel in use */
extern teChannel        eChannelInUse;

/** IEEE802.15.4 extended PAN ID in use */
extern uint64_t         u64PanIDInUse;

/** IEEE802.15.4 short PAN ID in use */
extern uint16_t         u16PanIDInUse;


/** Event queue */
extern tsUtilsQueue     sZcbEventQueue;


/** Control bridge software version */
extern uint32_t         u32ZCB_SoftwareVersion;


/** Flag to enable / disable APS acks on packets sent from the control bridge. */
extern int              bZCB_EnableAPSAck;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/** Initialise control bridge connected to serial port */
teZcbStatus eZCB_Init(char *cpSerialDevice, uint32_t u32BaudRate, char *pcPDMFile);


/** Finished with control bridge - call this to tidy up */ 
teZcbStatus eZCB_Finish(void);

/** Attempt to establish comms with the control bridge.
 *  \return E_ZCB_OK when comms established, otherwise E_ZCB_COMMS_FAILED
 */
teZcbStatus eZCB_EstablishComms(void);

/** Make control bridge factory new, and set up configuration. */
teZcbStatus eZCB_FactoryNew(void);

teZcbStatus eZCB_SetExtendedPANID(uint64_t u64PanID);

teZcbStatus eZCB_SetChannelMask(uint32_t u32ChannelMask);

teZcbStatus eZCB_SetInitialSecurity(uint8_t u8State, uint8_t u8Sequence, uint8_t u8Type, uint8_t *pu8Key);

/** Set the module mode */
teZcbStatus eZCB_SetDeviceType(teModuleMode eModuleMode);

/** Start the network */
teZcbStatus eZCB_StartNetwork(void);

/** Set the permit joining interval */
teZcbStatus eZCB_SetPermitJoining(uint8_t u8Interval);

/** Get the permit joining status of the control bridge */
teZcbStatus eZCB_GetPermitJoining(uint8_t *pu8Status);

/** Set whitelisting enable status */
teZcbStatus eZCB_SetWhitelistEnabled(uint8_t bEnable);

/** Authenticate a device into the network.
 *  \param u64IEEEAddress       MAC address to add
 *  \param au8LinkKey           Pointer to 16 byte device link key
 *  \param au8NetworkKey[out]   Pointer to 16 byte location to put encrypted network key
 *  \param au8MIC[out]          Pointer to location to put 4 byte encryption MIC
 *  \param pu64TrustCenterAddress Pointer to location to put trust center address
 *  \param pu8KeySequenceNumber Pointer to location to store active key sequence number
 *  \return E_ZCB_OK on success
 */
teZcbStatus eZCB_AuthenticateDevice(uint64_t u64IEEEAddress, uint8_t *pau8LinkKey, 
                                    uint8_t *pau8NetworkKey, uint8_t *pau8MIC,
                                    uint64_t *pu64TrustCenterAddress, uint8_t *pu8KeySequenceNumber);

/** Initiate Touchlink */
teZcbStatus eZCB_ZLL_Touchlink(void);


/** Send a match descriptor request */
teZcbStatus eZCB_MatchDescriptorRequest(uint16_t u16TargetAddress, uint16_t u16ProfileID, 
                                        uint8_t u8NumInputClusters, uint16_t *pau16InputClusters, 
                                        uint8_t u8NumOutputClusters, uint16_t *pau16OutputClusters,
                                        uint8_t *pu8SequenceNo);


/** Request a node to leave the network via a Management Leave Request.
 *  \param psZCBNode            Pointer to node which should leave
 *  \return E_ZCB_OK on success
 */
teZcbStatus eZCB_LeaveRequest(tsZCB_Node *psZCBNode);

teZcbStatus eZCB_IEEEAddressRequest(tsZCB_Node *psZCBNode);


/** Send a node descriptor request */
teZcbStatus eZCB_NodeDescriptorRequest(tsZCB_Node *psZCBNode);

/** Send a simple descriptor request and use it to populate a node structure */
teZcbStatus eZCB_SimpleDescriptorRequest(tsZCB_Node *psZCBNode, uint8_t u8Endpoint);


/** Send a request for the neighbour table to a node */
teZcbStatus eZCB_NeighbourTableRequest(tsZCB_Node *psZCBNode);

/** Request an attribute from a node. The data is requested, parsed, and passed back.
 *  \param psZCBNode            Pointer to node from which to read the attribute
 *  \param u16ClusterID         Cluster ID to read the attribute from. The endpoint containing this cluster is determined from the psZCBNode structure.
 *  \param u8Direction          0 = Read from client to server. 1 = Read from server to client.
 *  \param u8ManufacturerSpecific 0 = Normal attribute. 1 = Manufacturer specific attribute
 *  \param u16ManufacturerID    if u8ManufacturerSpecific = 1, then the manufacturer ID of the attribute
 *  \param u16AttributeID       ID of the attribute to read
 *  \param pvData[out]          Pointer to location to store the read data. It is assumed that the caller knows how big the requested data will be 
 *                              and has allocated a buffer big enough to contain it. Failure will reault in memory corruption.
 *  \return E_ZCB_OK on success.
 */
teZcbStatus eZCB_ReadAttributeRequest(tsZCB_Node *psZCBNode, uint16_t u16ClusterID,
                                      uint8_t u8Direction, uint8_t u8ManufacturerSpecific, uint16_t u16ManufacturerID,
                                      uint16_t u16AttributeID, void *pvData);
                                      

/** Write an attribute to a node.
 *  \param psZCBNode            Pointer to node to write the attribute to
 *  \param u16ClusterID         Cluster ID to write the attribute to. The endpoint containing this cluster is determined from the psZCBNode structure.
 *  \param u8Direction          0 = write from client to server. 1 = write from server to client.
 *  \param u8ManufacturerSpecific 0 = Normal attribute. 1 = Manufacturer specific attribute
 *  \param u16ManufacturerID    if u8ManufacturerSpecific = 1, then the manufacturer ID of the attribute
 *  \param u16AttributeID       ID of the attribute to write
 *  \param eType                Type of the attribute being written
 *  \param pvData               Pointer to buffer of data to write. This must be of the right size for the attribute that is being written. 
 *                              Failure will reault in memory corruption.
 *  \return E_ZCB_OK on success.
 */
teZcbStatus eZCB_WriteAttributeRequest(tsZCB_Node *psZCBNode, uint16_t u16ClusterID,
                                      uint8_t u8Direction, uint8_t u8ManufacturerSpecific, uint16_t u16ManufacturerID,
                                      uint16_t u16AttributeID, teZCL_ZCLAttributeType eType, void *pvData);

/** Wait for default response to a command with sequence number u8SequenceNo.
 *  \param u8SequenceNo         Sequence number of command to wait for resonse to
 *  \return Status from default response.
 */
teZcbStatus eZCB_GetDefaultResponse(uint8_t u8SequenceNo);

/** Add a node to a group */
teZcbStatus eZCB_AddGroupMembership(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress);

/** Remove a node from a group */
teZcbStatus eZCB_RemoveGroupMembership(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress);

/** Populate the nodes group membership information */
teZcbStatus eZCB_GetGroupMembership(tsZCB_Node *psZCBNode);

/** Remove a node from all groups */
teZcbStatus eZCB_ClearGroupMembership(tsZCB_Node *psZCBNode);

/** Remove a scene in a node */
teZcbStatus eZCB_RemoveScene(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8SceneID);

/** Store a scene in a node */
teZcbStatus eZCB_StoreScene(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8SceneID);

/** Recall a scene */
teZcbStatus eZCB_RecallScene(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8SceneID);

/** Get a nodes scene membership */
teZcbStatus eZCB_GetSceneMembership(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t *pu8NumScenes, uint8_t **pau8Scenes);

/** Get a locked node structure for the control bridge
 *  \return pointer to control bridge
 */
tsZCB_Node *psZCB_FindNodeControlBridge(void);

tsZCB_Node *psZCB_FindNodeIEEEAddress(uint64_t u64IEEEAddress);

tsZCB_Node *psZCB_FindNodeShortAddress(uint16_t u16ShortAddress);

tsZCB_Node *psZCB_NodeOldestComms(void);

teZcbStatus eZCB_AddNode(uint16_t u16ShortAddress, uint64_t u64IEEEAddress, uint16_t u16DeviceID, uint8_t u8MacCapability, tsZCB_Node **ppsZCBNode);

teZcbStatus eZCB_RemoveNode(tsZCB_Node *psZCBNode);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* MODULECONFIG_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

