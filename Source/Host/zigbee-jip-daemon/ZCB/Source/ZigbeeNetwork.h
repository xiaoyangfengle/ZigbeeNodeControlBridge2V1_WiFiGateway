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

#ifndef  ZIGBEENETWORK_H_INCLUDED
#define  ZIGBEENETWORK_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdint.h>

#include "ZigbeeControlBridge.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/** Stucture for the Zigbee network */
typedef struct
{
    tsUtilsLock             sLock;              /**< Lock for the node list */
    
    tsZCB_Node              sNodes;             /**< Linked list of nodes.
                                                 *   The head is the control bridge */
} tsZCB_Network;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern tsZCB_Network sZCB_Network;
    

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

void DBG_PrintNode(tsZCB_Node *psNode);

teZcbStatus eZCB_NodeAddEndpoint(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ProfileID, tsZCB_NodeEndpoint **ppsEndpoint);
teZcbStatus eZCB_NodeAddCluster(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ClusterID);
teZcbStatus eZCB_NodeAddAttribute(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ClusterID, uint16_t u16AttributeID);
teZcbStatus eZCB_NodeAddCommand(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ClusterID, uint8_t u8CommandID); 

teZcbStatus eZCB_NodeClearGroups(tsZCB_Node *psZCBNode);
teZcbStatus eZCB_NodeAddGroup(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress);

void        vZCB_NodeUpdateComms(tsZCB_Node *psZCBNode, teZcbStatus eStatus);

/** Find the first endpoint on a node that contains a cluster ID.
 *  \param psZCBNode        Pointer to node to search
 *  \param u16ClusterID     Cluster ID of interest
 *  \return A pointer to the endpoint or NULL if cluster ID not found
 */
tsZCB_NodeEndpoint *psZCB_NodeFindEndpoint(tsZCB_Node *psZCBNode, uint16_t u16ClusterID);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* ZIGBEENETWORK_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

