/****************************************************************************
 *
 * MODULE:             Linux 6LoWPAN Routing daemon
 *
 * COMPONENT:          Interface to module
 *
 * REVISION:           $Revision: 37647 $
 *
 * DATED:              $Date: 2011-12-02 11:16:28 +0000 (Fri, 02 Dec 2011) $
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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>

#include <libdaemon/daemon.h>

#include "ZigbeeControlBridge.h"
#include "ZigbeeConstant.h"
#include "ZigbeeZLL.h"
#include "ZigbeeNetwork.h"
#include "SerialLink.h"
#include "Utils.h"

#ifdef USE_ZEROCONF
#include "Zeroconf.h"
#endif /* USE_ZEROCONF */

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_ZLL 0

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

teZcbStatus eZBZLL_OnOff(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8Mode)
{
    tsZCB_Node          *psControlBridge;
    tsZCB_NodeEndpoint  *psSourceEndpoint;
    tsZCB_NodeEndpoint  *psDestinationEndpoint;
    uint8_t             u8SequenceNo;
    
    struct
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Mode;
    } __attribute__((__packed__)) sOnOffMessage;
    
    DBG_vPrintf(DBG_ZLL, "On/Off (Set Mode=%d)\n", u8Mode);
    
    if (u8Mode > 2)
    {
        /* Illegal value */
        return E_ZCB_ERROR;
    }
    
    psControlBridge = psZCB_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZCB_ERROR;
    }
    psSourceEndpoint = psZCB_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_ONOFF);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZLL, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_ONOFF);
        return E_ZCB_ERROR;
    }

    sOnOffMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    eUtils_LockUnlock(&psControlBridge->sLock);
    
    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sOnOffMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sOnOffMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sOnOffMessage.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
        
        psDestinationEndpoint = psZCB_NodeFindEndpoint(psZCBNode, E_ZB_CLUSTERID_ONOFF);

        if (psDestinationEndpoint)
        {
            sOnOffMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sOnOffMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        eUtils_LockUnlock(&psZCBNode->sLock);
    }
    else
    {
        sOnOffMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sOnOffMessage.u16TargetAddress      = htons(u16GroupAddress);
        sOnOffMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sOnOffMessage.u8Mode = u8Mode;
    
    if (eSL_SendMessage(E_SL_MSG_ONOFF, sizeof(sOnOffMessage), &sOnOffMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    if (psZCBNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZCB_OK;
    }
}


teZcbStatus eZBZLL_OnOffCheck(tsZCB_Node *psZCBNode, uint8_t u8Mode)
{
    teZcbStatus eStatus;
    uint8_t u8OnOffStatus;
    
    if ((eStatus = eZBZLL_OnOff(psZCBNode, 0, u8Mode)) != E_ZCB_OK)
    {
        return eStatus;
    }
    
    if (u8Mode < 2)
    {
        /* Can't check the staus of a toggle command */
        
        /* Wait 100ms */
        usleep(100*1000);
        
        if ((eStatus = eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_ONOFF, 0, 0, 0, E_ZB_ATTRIBUTEID_ONOFF_ONOFF, &u8OnOffStatus)) != E_ZCB_OK)
        {
            return eStatus;
        }
        
        DBG_vPrintf(DBG_ZLL, "On Off attribute read as: 0x%02X\n", u8OnOffStatus);
        
        if (u8OnOffStatus != u8Mode)
        {
            return E_ZCB_REQUEST_NOT_ACTIONED;
        }
    }
    return E_ZCB_OK;
}


teZcbStatus eZBZLL_MoveToLevel(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8OnOff, uint8_t u8Level, uint16_t u16TransitionTime)
{
    tsZCB_Node          *psControlBridge;
    tsZCB_NodeEndpoint  *psSourceEndpoint;
    tsZCB_NodeEndpoint  *psDestinationEndpoint;
    uint8_t             u8SequenceNo;
    
    struct
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8OnOff;
        uint8_t     u8Level;
        uint16_t    u16TransitionTime;
    } __attribute__((__packed__)) sLevelMessage;
    
    DBG_vPrintf(DBG_ZLL, "Set Level %d\n", u8Level);
    
    if (u8Level > 254)
    {
        u8Level = 254;
    }
    
    psControlBridge = psZCB_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZCB_ERROR;
    }
    psSourceEndpoint = psZCB_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_LEVEL_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZLL, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_LEVEL_CONTROL);
        return E_ZCB_ERROR;
    }

    sLevelMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    eUtils_LockUnlock(&psControlBridge->sLock);

    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sLevelMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sLevelMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sLevelMessage.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
        
        psDestinationEndpoint = psZCB_NodeFindEndpoint(psZCBNode, E_ZB_CLUSTERID_LEVEL_CONTROL);

        if (psDestinationEndpoint)
        {
            sLevelMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sLevelMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        eUtils_LockUnlock(&psZCBNode->sLock);
    }
    else
    {
        sLevelMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sLevelMessage.u16TargetAddress      = htons(u16GroupAddress);
        sLevelMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sLevelMessage.u8OnOff               = u8OnOff;
    sLevelMessage.u8Level               = u8Level;
    sLevelMessage.u16TransitionTime     = htons(u16TransitionTime);
   // sOnOffMessage.u8Gradient            = u8Gradient;
    
    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_LEVEL_ONOFF, sizeof(sLevelMessage), &sLevelMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    if (psZCBNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZCB_OK;
    }
}


teZcbStatus eZBZLL_MoveToLevelCheck(tsZCB_Node *psZCBNode, uint8_t u8OnOff, uint8_t u8Level, uint16_t u16TransitionTime)
{
    teZcbStatus eStatus;
    uint8_t u8CurrentLevel;
    
    if ((eStatus = eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_LEVEL_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL, &u8CurrentLevel)) != E_ZCB_OK)
    {
        return eStatus;
    }
    
    DBG_vPrintf(DBG_ZLL, "Current Level attribute read as: 0x%02X\n", u8CurrentLevel);
    
    if (u8CurrentLevel == u8Level)
    {
        /* Level is already set */
        /* This is a guard for transition times that are outside of the JIP 300ms retry window */
        return E_ZCB_OK;
    }
    
    if (u8Level > 254)
    {
        u8Level = 254;
    }
    
    if ((eStatus = eZBZLL_MoveToLevel(psZCBNode, 0, u8OnOff, u8Level, u16TransitionTime)) != E_ZCB_OK)
    {
        return eStatus;
    }
    
    /* Wait the transition time */
    usleep(u16TransitionTime * 100000);

    if ((eStatus = eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_LEVEL_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL, &u8CurrentLevel)) != E_ZCB_OK)
    {
        return eStatus;
    }
    
    DBG_vPrintf(DBG_ZLL, "Current Level attribute read as: 0x%02X\n", u8CurrentLevel);
    
    if (u8CurrentLevel != u8Level)
    {
        return E_ZCB_REQUEST_NOT_ACTIONED;
    }
    return E_ZCB_OK;
}

teZcbStatus eZBZLL_MoveToHue(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8Hue, uint16_t u16TransitionTime)
{
    tsZCB_Node          *psControlBridge;
    tsZCB_NodeEndpoint  *psSourceEndpoint;
    tsZCB_NodeEndpoint  *psDestinationEndpoint;
    uint8_t             u8SequenceNo;
    
    struct
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Hue;
        uint8_t     u8Direction;
        uint16_t    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToHueMessage;
    
    DBG_vPrintf(DBG_ZLL, "Set Hue %d\n", u8Hue);
    
    psControlBridge = psZCB_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZCB_ERROR;
    }
    psSourceEndpoint = psZCB_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZLL, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZCB_ERROR;
    }

    sMoveToHueMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    eUtils_LockUnlock(&psControlBridge->sLock);

    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sMoveToHueMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sMoveToHueMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveToHueMessage.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
        
        psDestinationEndpoint = psZCB_NodeFindEndpoint(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveToHueMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveToHueMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        eUtils_LockUnlock(&psZCBNode->sLock);
    }
    else
    {
        sMoveToHueMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveToHueMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveToHueMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sMoveToHueMessage.u8Hue               = u8Hue;
    sMoveToHueMessage.u8Direction         = 0;
    sMoveToHueMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_HUE, sizeof(sMoveToHueMessage), &sMoveToHueMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    if (psZCBNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZCB_OK;
    }
}


teZcbStatus eZBZLL_MoveToSaturation(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8Saturation, uint16_t u16TransitionTime)
{
    tsZCB_Node          *psControlBridge;
    tsZCB_NodeEndpoint  *psSourceEndpoint;
    tsZCB_NodeEndpoint  *psDestinationEndpoint;
    uint8_t             u8SequenceNo;
    
    struct
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Saturation;
        uint16_t    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToSaturationMessage;
    
    DBG_vPrintf(DBG_ZLL, "Set Saturation %d\n", u8Saturation);
    
    psControlBridge = psZCB_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZCB_ERROR;
    }
    psSourceEndpoint = psZCB_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZLL, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZCB_ERROR;
    }

    sMoveToSaturationMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    eUtils_LockUnlock(&psControlBridge->sLock);

    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sMoveToSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sMoveToSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveToSaturationMessage.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
        
        psDestinationEndpoint = psZCB_NodeFindEndpoint(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveToSaturationMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveToSaturationMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        eUtils_LockUnlock(&psZCBNode->sLock);
    }
    else
    {
        sMoveToSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveToSaturationMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveToSaturationMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sMoveToSaturationMessage.u8Saturation        = u8Saturation;
    sMoveToSaturationMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_SATURATION, sizeof(sMoveToSaturationMessage), &sMoveToSaturationMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    if (psZCBNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZCB_OK;
    }
}



teZcbStatus eZBZLL_MoveToHueSaturation(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8Hue, uint8_t u8Saturation, uint16_t u16TransitionTime)
{
    tsZCB_Node          *psControlBridge;
    tsZCB_NodeEndpoint  *psSourceEndpoint;
    tsZCB_NodeEndpoint  *psDestinationEndpoint;
    uint8_t             u8SequenceNo;
    
    struct
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Hue;
        uint8_t     u8Saturation;
        uint16_t    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToHueSaturationMessage;
    
    DBG_vPrintf(DBG_ZLL, "Set Hue %d, Saturation %d\n", u8Hue, u8Saturation);
    
    psControlBridge = psZCB_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZCB_ERROR;
    }
    psSourceEndpoint = psZCB_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZLL, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZCB_ERROR;
    }

    sMoveToHueSaturationMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    eUtils_LockUnlock(&psControlBridge->sLock);

    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sMoveToHueSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sMoveToHueSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveToHueSaturationMessage.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
        
        psDestinationEndpoint = psZCB_NodeFindEndpoint(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveToHueSaturationMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveToHueSaturationMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        eUtils_LockUnlock(&psZCBNode->sLock);
    }
    else
    {
        sMoveToHueSaturationMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveToHueSaturationMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveToHueSaturationMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sMoveToHueSaturationMessage.u8Hue               = u8Hue;
    sMoveToHueSaturationMessage.u8Saturation        = u8Saturation;
    sMoveToHueSaturationMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_HUE_SATURATION, sizeof(sMoveToHueSaturationMessage), &sMoveToHueSaturationMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    if (psZCBNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZCB_OK;
    }
}


teZcbStatus eZBZLL_MoveToColour(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint16_t u16X, uint16_t u16Y, uint16_t u16TransitionTime)
{
    tsZCB_Node          *psControlBridge;
    tsZCB_NodeEndpoint  *psSourceEndpoint;
    tsZCB_NodeEndpoint  *psDestinationEndpoint;
    uint8_t             u8SequenceNo;
    
    struct
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16X;
        uint16_t    u16Y;
        uint16_t    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToColourMessage;
    
    DBG_vPrintf(DBG_ZLL, "Set X %d, Y %d\n", u16X, u16Y);
    
    psControlBridge = psZCB_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZCB_ERROR;
    }
    psSourceEndpoint = psZCB_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZLL, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZCB_ERROR;
    }

    sMoveToColourMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    eUtils_LockUnlock(&psControlBridge->sLock);

    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sMoveToColourMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sMoveToColourMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveToColourMessage.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
        
        psDestinationEndpoint = psZCB_NodeFindEndpoint(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveToColourMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveToColourMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        eUtils_LockUnlock(&psZCBNode->sLock);
    }
    else
    {
        sMoveToColourMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveToColourMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveToColourMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sMoveToColourMessage.u16X                = htons(u16X);
    sMoveToColourMessage.u16Y                = htons(u16Y);
    sMoveToColourMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_COLOUR, sizeof(sMoveToColourMessage), &sMoveToColourMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    if (psZCBNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZCB_OK;
    }
}


teZcbStatus eZBZLL_MoveToColourTemperature(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint16_t u16ColourTemperature, uint16_t u16TransitionTime)
{
    tsZCB_Node          *psControlBridge;
    tsZCB_NodeEndpoint  *psSourceEndpoint;
    tsZCB_NodeEndpoint  *psDestinationEndpoint;
    uint8_t             u8SequenceNo;
    
    struct
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16ColourTemperature;
        uint16_t    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToColourTemperatureMessage;
    
    DBG_vPrintf(DBG_ZLL, "Set colour temperature %d\n", u16ColourTemperature);
    
    psControlBridge = psZCB_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZCB_ERROR;
    }
    psSourceEndpoint = psZCB_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZLL, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZCB_ERROR;
    }

    sMoveToColourTemperatureMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    eUtils_LockUnlock(&psControlBridge->sLock);

    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sMoveToColourTemperatureMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sMoveToColourTemperatureMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveToColourTemperatureMessage.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
        
        psDestinationEndpoint = psZCB_NodeFindEndpoint(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveToColourTemperatureMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveToColourTemperatureMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        eUtils_LockUnlock(&psZCBNode->sLock);
    }
    else
    {
        sMoveToColourTemperatureMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveToColourTemperatureMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveToColourTemperatureMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sMoveToColourTemperatureMessage.u16ColourTemperature    = htons(u16ColourTemperature);
    sMoveToColourTemperatureMessage.u16TransitionTime       = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_COLOUR_TEMPERATURE, sizeof(sMoveToColourTemperatureMessage), &sMoveToColourTemperatureMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    if (psZCBNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZCB_OK;
    }
}


teZcbStatus eZBZLL_MoveColourTemperature(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8Mode, uint16_t u16Rate, uint16_t u16ColourTemperatureMin, uint16_t u16ColourTemperatureMax)
{
    tsZCB_Node          *psControlBridge;
    tsZCB_NodeEndpoint  *psSourceEndpoint;
    tsZCB_NodeEndpoint  *psDestinationEndpoint;
    uint8_t             u8SequenceNo;
    
    struct
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Mode;
        uint16_t    u16Rate;
        uint16_t    u16ColourTemperatureMin;
        uint16_t    u16ColourTemperatureMax;
    } __attribute__((__packed__)) sMoveColourTemperatureMessage;
    
    DBG_vPrintf(DBG_ZLL, "Move colour temperature\n");
    
    psControlBridge = psZCB_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZCB_ERROR;
    }
    psSourceEndpoint = psZCB_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZLL, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZCB_ERROR;
    }

    sMoveColourTemperatureMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    eUtils_LockUnlock(&psControlBridge->sLock);

    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sMoveColourTemperatureMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sMoveColourTemperatureMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sMoveColourTemperatureMessage.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
        
        psDestinationEndpoint = psZCB_NodeFindEndpoint(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sMoveColourTemperatureMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sMoveColourTemperatureMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        eUtils_LockUnlock(&psZCBNode->sLock);
    }
    else
    {
        sMoveColourTemperatureMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sMoveColourTemperatureMessage.u16TargetAddress      = htons(u16GroupAddress);
        sMoveColourTemperatureMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sMoveColourTemperatureMessage.u8Mode                    = u8Mode;
    sMoveColourTemperatureMessage.u16Rate                   = htons(u16Rate);
    sMoveColourTemperatureMessage.u16ColourTemperatureMin   = htons(u16ColourTemperatureMin);
    sMoveColourTemperatureMessage.u16ColourTemperatureMax   = htons(u16ColourTemperatureMax);
    
    if (eSL_SendMessage(E_SL_MSG_MOVE_COLOUR_TEMPERATURE, sizeof(sMoveColourTemperatureMessage), &sMoveColourTemperatureMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    if (psZCBNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZCB_OK;
    }
}


teZcbStatus eZBZLL_ColourLoopSet(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8UpdateFlags, uint8_t u8Action, uint8_t u8Direction, uint16_t u16Time, uint16_t u16StartHue)
{
    tsZCB_Node          *psControlBridge;
    tsZCB_NodeEndpoint  *psSourceEndpoint;
    tsZCB_NodeEndpoint  *psDestinationEndpoint;
    uint8_t             u8SequenceNo;
    
    struct
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8UpdateFlags;
        uint8_t     u8Action;
        uint8_t     u8Direction;
        uint16_t    u16Time;
        uint16_t    u16StartHue;
    } __attribute__((__packed__)) sColourLoopSetMessage;
    
    DBG_vPrintf(DBG_ZLL, "Colour loop set\n");
    
    psControlBridge = psZCB_FindNodeControlBridge();
    if (!psControlBridge)
    {
        return E_ZCB_ERROR;
    }
    psSourceEndpoint = psZCB_NodeFindEndpoint(psControlBridge, E_ZB_CLUSTERID_COLOR_CONTROL);
    if (!psSourceEndpoint)
    {
        DBG_vPrintf(DBG_ZLL, "Cluster ID 0x%04X not found on control bridge\n", E_ZB_CLUSTERID_COLOR_CONTROL);
        return E_ZCB_ERROR;
    }

    sColourLoopSetMessage.u8SourceEndpoint      = psSourceEndpoint->u8Endpoint;
    eUtils_LockUnlock(&psControlBridge->sLock);

    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sColourLoopSetMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sColourLoopSetMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sColourLoopSetMessage.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);
        
        psDestinationEndpoint = psZCB_NodeFindEndpoint(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL);

        if (psDestinationEndpoint)
        {
            sColourLoopSetMessage.u8DestinationEndpoint = psDestinationEndpoint->u8Endpoint;
        }
        else
        {
            sColourLoopSetMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
        }
        eUtils_LockUnlock(&psZCBNode->sLock);
    }
    else
    {
        sColourLoopSetMessage.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sColourLoopSetMessage.u16TargetAddress      = htons(u16GroupAddress);
        sColourLoopSetMessage.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
    
    sColourLoopSetMessage.u8UpdateFlags             = u8UpdateFlags;
    sColourLoopSetMessage.u8Action                  = u8Action;
    sColourLoopSetMessage.u8Direction               = u8Direction;
    sColourLoopSetMessage.u16Time                   = htons(u16Time);
    sColourLoopSetMessage.u16StartHue               = htons(u16StartHue);
    
    if (eSL_SendMessage(E_SL_MSG_COLOUR_LOOP_SET, sizeof(sColourLoopSetMessage), &sColourLoopSetMessage, &u8SequenceNo) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    
    if (psZCBNode)
    {
        return eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        return E_ZCB_OK;
    }
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/



/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
