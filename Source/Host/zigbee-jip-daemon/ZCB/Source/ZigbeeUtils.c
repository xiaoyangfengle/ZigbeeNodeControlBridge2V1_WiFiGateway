/****************************************************************************
 *
 * MODULE:             Zigbee Utility functions
 *
 * COMPONENT:          $RCSfile: SerialLink.c,v $
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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdint.h>

#include "ZigbeeUtils.h"
#include "ZigbeeNetwork.h"
#include "Utils.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_ZCB_UTILS 0

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

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


teZcbStatus eZCB_GetEndpoints(tsZCB_Node *psZCBNode, eZigbee_ClusterID eClusterID, uint8_t *pu8Src, uint8_t *pu8Dst)
{
    tsZCB_Node          *psControlBridge;
    tsZCB_NodeEndpoint  *psSourceEndpoint;
    tsZCB_NodeEndpoint  *psDestinationEndpoint;
    
    if (pu8Src)
    {
        psControlBridge = psZCB_FindNodeControlBridge();
        if (!psControlBridge)
        {
            return E_ZCB_ERROR;
        }
        psSourceEndpoint = psZCB_NodeFindEndpoint(psControlBridge, eClusterID);
        if (!psSourceEndpoint)
        {
            DBG_vPrintf(DBG_ZCB_UTILS, "Cluster ID 0x%04X not found on control bridge\n", eClusterID);
            eUtils_LockUnlock(&psControlBridge->sLock);
            return E_ZCB_UNKNOWN_CLUSTER;
        }

        *pu8Src = psSourceEndpoint->u8Endpoint;
        eUtils_LockUnlock(&psControlBridge->sLock);
    }
    
    if (pu8Dst)
    {
        if (!psZCBNode)
        {
            return E_ZCB_ERROR;
        }
        psDestinationEndpoint = psZCB_NodeFindEndpoint(psZCBNode, eClusterID);

        if (!psDestinationEndpoint)
        {
            DBG_vPrintf(DBG_ZCB_UTILS, "Cluster ID 0x%04X not found on node 0x%04X\n", eClusterID, psZCBNode->u16ShortAddress);
            return E_ZCB_UNKNOWN_CLUSTER;
        }
        *pu8Dst = psDestinationEndpoint->u8Endpoint;
    }
    return E_ZCB_OK;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

