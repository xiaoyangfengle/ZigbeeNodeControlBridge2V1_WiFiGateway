/****************************************************************************
 *
 * MODULE:             Linux Zigbee - JIP Daemon
 *
 * COMPONENT:          Fake Border router
 *
 * REVISION:           $Revision: 37346 $
 *
 * DATED:              $Date: 2011-11-18 12:16:43 +0000 (Fri, 18 Nov 2011) $
 *
 * AUTHOR:             Matt Redfearn
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

#ifndef  JIP_BORDERROUTER_H_INCLUDED
#define  JIP_BORDERROUTER_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include "JIP.h"
#include "ZigbeeConstant.h"
#include "ZigbeeControlBridge.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/** Enumerated type of status codes */
typedef enum
{
    E_BR_OK,
    E_BR_ERROR,
    E_BR_JIP_ERROR,
} teBrStatus;


/** Function that will set up a JIP device corresponding to the Zigbee device
 *  \param psZCBNode        Pointer to the joining Zigbee node definition
 *  \param psJIPNode        Pointer to the joining JIP node definition to fill in
 *  \return E_JIP_OK on success.
 */
typedef teJIP_Status (*tpreDeviceInitialise)(tsZCB_Node *psZCBNode, tsNode *psJIPNode);


/** Function that will handle asynchronous attribute reports from a Zigbee device
 *  \param psZCBNode        Pointer to the Zigbee node definition
 *  \param psJIPNode        Pointer to the JIP node definition
 *  \param u16ClusterID     The cluster that is being updated
 *  \param u16AttributeID   The Attribute that is being updated
 *  \param eType            The attribute type
 *  \param uData            The updated data for the attribute
 *  \return None
 */
typedef void (*tprAttributeUpdate)(tsZCB_Node *psZCBNode, tsNode *psJIPNode, uint16_t u16ClusterID, uint16_t u16AttributeID, teZCL_ZCLAttributeType eType, tuZcbAttributeData uData);
                     

/** Structure of Zigbee Device ID to JIP device ID mappings.
 *  When a node joins this structure is used to map the Zigbee device to 
 *  a JIP device, and the prInitaliseRoutine is called to populate a
 *  newly created JIP psJIPNode with data.
 */
typedef struct
{
    uint16_t                u16ZigbeeDeviceID;          /**< Zigbee Deive ID */
    uint32_t                u32JIPDeviceID;             /**< Corresponding JIP device ID */
    tpreDeviceInitialise    prInitaliseRoutine;         /**< Initialisation routine for the JIP device */
    tprAttributeUpdate      prAttributeUpdateRoutine;   /**< Attribute update routine for the JIP device */
} tsDeviceIDMap;


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/** JIP context structure */
extern tsJIP_Context sJIP_Context;

/** Map of Zigbee Device IDs to JIP Device IDs. The fianl entry must be 0,0,NULL */
extern tsDeviceIDMap asDeviceIDMap[];

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/** Create the fake border router.
 *  \param dev          Name of device to create
 *  \return E_TUN_OK if opened ok
 */
teBrStatus eBR_Init(const char *pcIPv6Address);

teBrStatus eBR_Destory(void);

teBrStatus eBR_NodeJoined(tsZCB_Node *psZCBNode);

teBrStatus eBR_NodeLeft(tsZCB_Node *psZCBNode);

teBrStatus eBR_CheckDeviceComms(void);

int iBR_DeviceTimedOut(tsZCB_Node *psZCBNode);

tsZCB_Node *psBR_FindZigbeeNode(tsNode *psJIPNode);

tsNode *psBR_FindJIPNode(tsZCB_Node *psZCBNode);

uint16_t u16BR_IPv6MulticastToBroadcast(tsJIPAddress *psMulticastAddress);


/** Convert Zigbee (ZCB) status values into JIP status values for return to client
 *  \param eZCB_Status      Zigbee status
 *  \return Equivalent JIP status value 
 */
teJIP_Status eJIP_Status_from_ZCB(teZcbStatus eZCB_Status);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* JIP_BORDERROUTER_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

