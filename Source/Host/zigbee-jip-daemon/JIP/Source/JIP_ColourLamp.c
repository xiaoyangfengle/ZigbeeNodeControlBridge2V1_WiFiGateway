/****************************************************************************
 *
 * MODULE:             Zigbee - JIP Daemon
 *
 * COMPONENT:          JIP Interface to control bridge
 *
 * REVISION:           $Revision: 43420 $
 *
 * DATED:              $Date: 2012-06-18 15:13:17 +0100 (Mon, 18 Jun 2012) $
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include <libdaemon/daemon.h>

#include "JIP.h"

#include "JIP_ColourLamp.h"
#include "JIP_Common.h"
#include "JIP_BorderRouter.h"
#include "ZigbeeConstant.h"
#include "ZigbeeZLL.h"
#include "Utils.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_COLOURLAMP 0

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

static teJIP_Status ColourLampSetMode(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampGetMode(tsVar *psVar);
static teJIP_Status ColourLampSetLevel(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampGetLevel(tsVar *psVar);

static teJIP_Status ColourLampGetColourTempMin(tsVar *psVar);
static teJIP_Status ColourLampGetColourTempMax(tsVar *psVar);

static teJIP_Status ColourLampSetColourMode(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampGetColourMode(tsVar *psVar);
static teJIP_Status ColourLampSetXYTarget(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampGetXYTarget(tsVar *psVar);
static teJIP_Status ColourLampGetHue(tsVar *psVar);
static teJIP_Status ColourLampSetHueTarget(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampGetSat(tsVar *psVar);
static teJIP_Status ColourLampSetSatTarget(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampSetHueSatTarget(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampGetHueSatTarget(tsVar *psVar);
static teJIP_Status ColourLampSetColourTempTarget(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampGetColourTempTarget(tsVar *psVar);
static teJIP_Status ColourLampSetColourTempChange(tsVar *psVar, tsJIPAddress *psMulticastAddress);

static teJIP_Status ColourLampSetAddGroup(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampSetRemoveGroup(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampSetClearGroups(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampGetGroups(tsVar *psVar);
static teJIP_Status ColourLampGetGroupsImpl(tsZCB_Node *psZCBNode, tsVar *psVar);
    
static teJIP_Status ColourLampSetAddScene(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampSetRemoveScene(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampSetScene(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ColourLampGetScene(tsVar *psVar);
static teJIP_Status ColourLampGetSceneIDs(tsVar *psVar);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/* Keep a pointer to libJIPs original group get function. */
static tprCbVarGet prlibJIPGroupsGetter = NULL;
/* Keep a pointer to libJIPs original group clearer function */
static tprCbVarSet prlibJIPGroupClearSetter = NULL;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

teJIP_Status eColourLampInitalise(tsZCB_Node *psZCBNode, tsNode *psJIPNode)
{
    tsMib       *psMib;
    tsVar       *psVar;
    uint16_t    u16Zero = 0;
    int         iHasColour = 0;
    
    DBG_vPrintf(DBG_COLOURLAMP, "Set up device\n");

    psMib = psJIP_LookupMibId(psJIPNode, NULL, 0xfffffe03);
    if (psMib)
    {
        /* BulbScene */
        psVar = psJIP_LookupVarIndex(psMib, 0);
        if (psVar)
        {
            /* AddSceneId */
            eJIP_SetVar(&sJIP_Context, psVar, (void*)&u16Zero, sizeof(u16Zero), E_JIP_FLAG_NONE);
            psVar->prCbVarSet = ColourLampSetAddScene;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 1);
        if (psVar)
        {
            /* DelSceneId */
            eJIP_SetVar(&sJIP_Context, psVar, (void*)&u16Zero, sizeof(u16Zero), E_JIP_FLAG_NONE);
            psVar->prCbVarSet = ColourLampSetRemoveScene;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 2);
        if (psVar)
        {
            /* Scene ID */
            psVar->prCbVarGet = ColourLampGetSceneIDs;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
    }
    
    /* Set the callbacks */
    psMib = psJIP_LookupMibId(psJIPNode, NULL, 0xfffffe04);
    if (psMib)
    {
        /* Bulb Control */
        psVar = psJIP_LookupVarIndex(psMib, 0);
        if (psVar)
        {
            /* Mode */
            psVar->prCbVarSet = ColourLampSetMode;
            psVar->prCbVarGet = ColourLampGetMode;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 1);
        if (psVar)
        {
            /* Scene ID */
            psVar->prCbVarSet = ColourLampSetScene;
            psVar->prCbVarGet = ColourLampGetScene;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 2);
        if (psVar)
        {
            /* LumTarget */
            psVar->prCbVarSet = ColourLampSetLevel;
            psVar->prCbVarGet = ColourLampGetLevel;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 3);
        if (psVar)
        {
            /* LumCurrent */
            psVar->prCbVarSet = ColourLampSetLevel;
            psVar->prCbVarGet = ColourLampGetLevel;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
    }
    
    psMib = psJIP_LookupMibId(psJIPNode, NULL, 0xfffffe09);
    if (psMib)
    {
        /* Colour Config */
        psVar = psJIP_LookupVarIndex(psMib, 10);
        if (psVar)
        {
            /* Colour Temperature Min */
            psVar->prCbVarGet = ColourLampGetColourTempMin;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 11);
        if (psVar)
        {
            /* Colour Temperature Max */
            psVar->prCbVarGet = ColourLampGetColourTempMax;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
    }
    
    psMib = psJIP_LookupMibId(psJIPNode, NULL, 0xfffffea2);
    if (psMib)
    {
        /* Device Control */
        psVar = psJIP_LookupVarIndex(psMib, 0);
        if (psVar)
        {
            psVar->prCbVarSet = ColourLampSetMode;
            psVar->prCbVarGet = ColourLampGetMode;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 1);
        if (psVar)
        {
            /* Scene ID */
            psVar->prCbVarSet = ColourLampSetScene;
            psVar->prCbVarGet = ColourLampGetScene;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
    }
    
    
    psMib = psJIP_LookupMibId(psJIPNode, NULL, 0xfffffe0c);
    if (psMib)
    {
        iHasColour = 1;
        
        /* Colour Control */
        
        psVar = psJIP_LookupVarIndex(psMib, 0);
        if (psVar)
        {
            /* Colour Mode */
            psVar->prCbVarSet = ColourLampSetColourMode;
            psVar->prCbVarGet = ColourLampGetColourMode;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 2);
        if (psVar)
        {
            /* XY Target */
            psVar->prCbVarSet = ColourLampSetXYTarget;
            psVar->prCbVarGet = ColourLampGetXYTarget;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 6);
        if (psVar)
        {
            /* Hue Target */
            psVar->prCbVarSet = ColourLampSetHueTarget;
            psVar->prCbVarGet = ColourLampGetHue;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 7);
        if (psVar)
        {
            /* Sat Target */
            psVar->prCbVarSet = ColourLampSetSatTarget;
            psVar->prCbVarGet = ColourLampGetSat;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 10);
        if (psVar)
        {
            /* Hue Sat Target */
            psVar->prCbVarSet = ColourLampSetHueSatTarget;
            psVar->prCbVarGet = ColourLampGetHueSatTarget;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 12);
        if (psVar)
        {
            /* Colour Temperature Target */
            psVar->prCbVarSet = ColourLampSetColourTempTarget;
            psVar->prCbVarGet = ColourLampGetColourTempTarget;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 14);
        if (psVar)
        {
            /* Colour Temperature Change */
            eJIP_SetVar(&sJIP_Context, psVar, (void*)&u16Zero, sizeof(u16Zero), E_JIP_FLAG_NONE);
            psVar->prCbVarSet = ColourLampSetColourTempChange;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
    }
    
    psMib = psJIP_LookupMibId(psJIPNode, NULL, 0xffffff02);
    if (psMib)
    {
        /* Groups */
        psVar = psJIP_LookupVarIndex(psMib, 0);
        if (psVar)
        {    
            /* Override, but keep available, libJIPs built in Get Groups function */
            prlibJIPGroupsGetter = psVar->prCbVarGet;
            psVar->prCbVarGet = ColourLampGetGroups;
            
            /* Initialise groups */
            ColourLampGetGroupsImpl(psZCBNode, psVar);
        }

        psVar = psJIP_LookupVarIndex(psMib, 1);
        if (psVar)
        {
            /* Override libJIPs built in Add Group function. */
            psVar->prCbVarSet = ColourLampSetAddGroup;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 2);
        if (psVar)
        {
            /* Override libJIPs built in Remove Group function. */
            psVar->prCbVarSet = ColourLampSetRemoveGroup;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 3);
        if (psVar)
        {    
            /* Override, but keep available, libJIPs built in Get Groups function */
            prlibJIPGroupClearSetter = psVar->prCbVarSet;
            psVar->prCbVarSet = ColourLampSetClearGroups;
        }
    }

    if (iHasColour)
    {
        return eJIPCommon_InitialiseDevice(psJIPNode, psZCBNode, "Colour Lamp");
    }
    else
    {
        return eJIPCommon_InitialiseDevice(psJIPNode, psZCBNode, "Mono Lamp");
    }
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

static teJIP_Status ColourLampSetMode(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint8_t *pu8Data = (uint8_t*)psVar->pvData;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Duplicate request\n");
        return eLastStatus;
    }
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }

    DBG_vPrintf(DBG_COLOURLAMP, "Set Mode: %d\n", *pu8Data);
    
    if (psMulticastAddress)
    {
        eZBZLL_OnOff(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), *pu8Data);
    }
    else
    {
        eStatus = eJIP_Status_from_ZCB(eZBZLL_OnOff(psZCBNode, 0, *pu8Data));
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}

static teJIP_Status ColourLampGetMode(tsVar *psVar)
{
    uint8_t u8OnOffStatus;
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if (eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_ONOFF, 0, 0, 0, E_ZB_ATTRIBUTEID_ONOFF_ONOFF, &u8OnOffStatus) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "On Off attribute read as: 0x%02X\n", u8OnOffStatus);
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eJIP_SetVarValue(psVar, &u8OnOffStatus, sizeof(uint8_t));
}


static teJIP_Status ColourLampSetLevel(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint8_t *pu8Data = (uint8_t*)psVar->pvData;
    uint16_t u16TransitionTime = 5;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Duplicate request\n");
        return eLastStatus;
    }

    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "Set Level: %d\n", *pu8Data);

    if (psMulticastAddress)
    {
        eZBZLL_MoveToLevel(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), 1, *pu8Data, u16TransitionTime);
    }
    else
    {
        eStatus = eJIP_Status_from_ZCB(eZBZLL_MoveToLevel(psZCBNode, 0, 1, *pu8Data, u16TransitionTime));
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


static teJIP_Status ColourLampGetLevel(tsVar *psVar)
{
    uint8_t u8CurrentLevel;
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if ((eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_LEVEL_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_LEVEL_CURRENTLEVEL, &u8CurrentLevel)) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "Current Level attribute read as: 0x%02X\n", u8CurrentLevel);
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eJIP_SetVarValue(psVar, &u8CurrentLevel, sizeof(uint8_t));
}


static teJIP_Status ColourLampGetColourTempMin(tsVar *psVar)
{
    uint16_t u16ColourTempMin;
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if ((eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_COLOURTEMP_PHYMIN, &u16ColourTempMin)) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "Colour temperature Physical minimum read as: %d\n", u16ColourTempMin);
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eJIP_SetVarValue(psVar, &u16ColourTempMin, sizeof(uint16_t));
}


static teJIP_Status ColourLampGetColourTempMax(tsVar *psVar)
{
    uint16_t u16ColourTempMax;
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if ((eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_COLOURTEMP_PHYMAX, &u16ColourTempMax)) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "Colour temperature Physical maximum read as: %d\n", u16ColourTempMax);
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eJIP_SetVarValue(psVar, &u16ColourTempMax, sizeof(uint16_t));
}


static teJIP_Status ColourLampSetColourMode(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint8_t *pu8Data = (uint8_t*)psVar->pvData;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Duplicate request\n");
        return eLastStatus;
    }
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }

    DBG_vPrintf(DBG_COLOURLAMP, "Set Colour Mode: %d\n", *pu8Data);
    
    switch (*pu8Data)
    {
        case (0):
            DBG_vPrintf(DBG_COLOURLAMP, "Stop colour loop\n");
            if (psMulticastAddress)
            {
               eZBZLL_ColourLoopSet(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), 0x7, 0 /* Stop */, 0 /* Down */, 5, 0);
            }
            else
            {
                eStatus = eJIP_Status_from_ZCB(eZBZLL_ColourLoopSet(psZCBNode, 0, 0x7, 0 /* Stop */, 0 /* Down */, 5, 0));
            }
            break;
        case (1):
            DBG_vPrintf(DBG_COLOURLAMP, "Start colour loop (down)\n");
            if (psMulticastAddress)
            {
                eZBZLL_ColourLoopSet(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), 0x7, 1 /* Start */, 0 /* Down */, 5, 0);
            }
            else
            {
                eStatus = eJIP_Status_from_ZCB(eZBZLL_ColourLoopSet(psZCBNode, 0, 0x7, 1 /* Start */, 0 /* Down */, 5, 0));
            }
            break;
        case (2):
            DBG_vPrintf(DBG_COLOURLAMP, "Start colour loop (up)\n");
            if (psMulticastAddress)
            {
                eZBZLL_ColourLoopSet(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), 0x7, 1 /* Start */, 1 /* Up */, 5, 0);
            }
            else
            {
                eStatus = eJIP_Status_from_ZCB(eZBZLL_ColourLoopSet(psZCBNode, 0, 0x7, 1 /* Start */, 1 /* Up */, 5, 0));
            }
            break;
        default:
            eStatus = E_JIP_ERROR_BAD_VALUE;
            break;
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


static teJIP_Status ColourLampGetColourMode(tsVar *psVar)
{
    uint8_t u8ColourMode = 0;
    return eJIP_SetVarValue(psVar, &u8ColourMode, sizeof(uint8_t));
}


static teJIP_Status ColourLampSetXYTarget(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint32_t *pu32XYTarget = (uint32_t *)psVar->pvData;
    uint16_t u16TargetX, u16TargetY;

    uint16_t u16TransitionTime = 5;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Duplicate request\n");
        return eLastStatus;
    }
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    u16TargetX = ((*pu32XYTarget) >> 16) & 0xFFFF;
    u16TargetY = ((*pu32XYTarget) >>  0) & 0xFFFF;

    DBG_vPrintf(DBG_COLOURLAMP, "Set X %d, Y %d\n", u16TargetX, u16TargetY);

    if (psMulticastAddress)
    {
        eZBZLL_MoveToColour(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), u16TargetX, u16TargetY, u16TransitionTime);
    }
    else
    {
        eStatus = eJIP_Status_from_ZCB(eZBZLL_MoveToColour(psZCBNode, 1, u16TargetX, u16TargetY, u16TransitionTime));
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }

    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


static teJIP_Status ColourLampGetXYTarget(tsVar *psVar)
{
    uint16_t u16CurrentX, u16CurrentY;
    uint32_t u32XYTarget;
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if ((eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_CURRENTX, &u16CurrentX)) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "Current X attribute read as: %d\n", u16CurrentX);
    
    if ((eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_CURRENTY, &u16CurrentY)) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }

    DBG_vPrintf(DBG_COLOURLAMP, "Current Y attribute read as: %d\n", u16CurrentY);
    
    u32XYTarget = (u16CurrentX << 16) | (u16CurrentY);
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eJIP_SetVarValue(psVar, &u32XYTarget, sizeof(uint32_t));
}


static teJIP_Status ColourLampGetHue(tsVar *psVar)
{
    uint8_t u8CurrentHue;
    uint16_t u16CurrentHue;
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if ((eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_CURRENTHUE, &u8CurrentHue)) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "Current Hue attribute read as: 0x%04X\n", u8CurrentHue);
    
    u16CurrentHue = ((int)u8CurrentHue * 3600) / 254;
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eJIP_SetVarValue(psVar, &u16CurrentHue, sizeof(uint16_t));
}


static teJIP_Status ColourLampSetHueTarget(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint16_t    u16TargetHue, *pu16TargetHue = (uint16_t *)psVar->pvData;
    uint16_t u16TransitionTime = 5;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Duplicate request\n");
        return eLastStatus;
    }
    
    if (*pu16TargetHue >= 3600)
    {
        /* Out of bounds */
        return E_JIP_ERROR_BAD_VALUE;
    }
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }

    DBG_vPrintf(DBG_COLOURLAMP, "Set Hue %d.%d degrees\n", *pu16TargetHue / 10, *pu16TargetHue % 10);
    
    // Convert to Zigbee space
    u16TargetHue = ((int)*pu16TargetHue * 0xFEFF) / 3600;

    if (psMulticastAddress)
    {
        eZBZLL_MoveToHue(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), u16TargetHue >> 8, u16TransitionTime);
    }
    else
    {
        eStatus = eJIP_Status_from_ZCB(eZBZLL_MoveToHue(psZCBNode, 1, u16TargetHue >> 8, u16TransitionTime));
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }

    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


static teJIP_Status ColourLampGetSat(tsVar *psVar)
{
    uint8_t u8CurrentSat;

    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if ((eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_CURRENTSAT, &u8CurrentSat)) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }

    DBG_vPrintf(DBG_COLOURLAMP, "Current Saturation attribute read as: 0x%02X\n", u8CurrentSat);
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eJIP_SetVarValue(psVar, &u8CurrentSat, sizeof(uint8_t));
}


static teJIP_Status ColourLampSetSatTarget(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint8_t    u8TargetSaturation, *pu8TargetSaturation = (uint8_t *)psVar->pvData;
    uint16_t u16TransitionTime = 5;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Duplicate request\n");
        return eLastStatus;
    }
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }

    DBG_vPrintf(DBG_COLOURLAMP, "Set Saturation %d\n", *pu8TargetSaturation);
    
    // Convert to Zigbee space
    u8TargetSaturation = (*pu8TargetSaturation * 254) / 255;

    if (psMulticastAddress)
    {
        eZBZLL_MoveToSaturation(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), u8TargetSaturation, u16TransitionTime);
    }
    else
    {
        eStatus = eJIP_Status_from_ZCB(eZBZLL_MoveToSaturation(psZCBNode, 1, u8TargetSaturation, u16TransitionTime));
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }

    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


static teJIP_Status ColourLampSetHueSatTarget(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint32_t    *pu32HueSatTarget = (uint32_t *)psVar->pvData;
    uint16_t    u16TargetHue;
    uint8_t     u8TargetSaturation;
    uint16_t u16TransitionTime = 5;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Duplicate request\n");
        return eLastStatus;
    }
    
    u16TargetHue        = ((*pu32HueSatTarget) >> 8) & 0xFFFF;
    u8TargetSaturation  = ((*pu32HueSatTarget) >> 0) & 0xFF;

    if (u16TargetHue >= 3600)
    {
        /* Out of bounds */
        return E_JIP_ERROR_BAD_VALUE;
    }
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "Set Hue %d.%d degrees, Saturation %d\n", u16TargetHue / 10, u16TargetHue % 10, u8TargetSaturation);
    
    // Convert to Zigbee space
    u16TargetHue = ((int)u16TargetHue * 0xFEFF) / 3600;
    u8TargetSaturation =  (u8TargetSaturation * 254) / 255;

    if (psMulticastAddress)
    {
        eZBZLL_MoveToHueSaturation(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), u16TargetHue >> 8, u8TargetSaturation, u16TransitionTime);
    }
    else
    {
        eStatus = eJIP_Status_from_ZCB(eZBZLL_MoveToHueSaturation(psZCBNode, 1, u16TargetHue >> 8, u8TargetSaturation, u16TransitionTime));
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }

    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


static teJIP_Status ColourLampGetHueSatTarget(tsVar *psVar)
{
    uint8_t  u8CurrentHue;
    uint16_t u16CurrentHue;
    uint8_t  u8CurrentSat;
    uint32_t u32HueSatTarget;
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if ((eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_CURRENTHUE, &u8CurrentHue)) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "Current Hue attribute read as: 0x%04X\n", u8CurrentHue);
    
    u16CurrentHue = ((int)u8CurrentHue * 3600) / 254;

    if ((eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_CURRENTSAT, &u8CurrentSat)) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }

    DBG_vPrintf(DBG_COLOURLAMP, "Current Saturation attribute read as: 0x%02X\n", u8CurrentSat);
    
    u32HueSatTarget = (u16CurrentHue << 8) | (u8CurrentSat);
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eJIP_SetVarValue(psVar, &u32HueSatTarget, sizeof(uint32_t));
}


static teJIP_Status ColourLampSetColourTempTarget(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint16_t u16ColourTemperature;
    uint16_t u16TransitionTime = 5;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Duplicate request\n");
        return eLastStatus;
    }
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    DBG_vPrintf(DBG_COLOURLAMP, "Set colour temperature %d\n", *psVar->pu16Data);

    u16ColourTemperature = *psVar->pu16Data;
    
    if (psMulticastAddress)
    {
        eZBZLL_MoveToColourTemperature(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), u16ColourTemperature, u16TransitionTime);
    }
    else
    {
        eStatus = eJIP_Status_from_ZCB(eZBZLL_MoveToColourTemperature(psZCBNode, 1, u16ColourTemperature, u16TransitionTime));
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }

    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


static teJIP_Status ColourLampSetColourTempChange(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint8_t  u8Mode;
    uint16_t u16Rate = 20;
    uint16_t u16ColourTemperatureMin = 0;
    uint16_t u16ColourTemperatureMax = 0;
    
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Duplicate request\n");
        return eLastStatus;
    }
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    DBG_vPrintf(DBG_COLOURLAMP, "Change colour temperature %d\n", *psVar->pi16Data);

    if (*psVar->pi16Data > 0)
    {
        u8Mode = 1;
    }
    else if (*psVar->pi16Data < 0)
    {
        u8Mode = 3;
    }
    else
    {
        // Stop
        u8Mode = 0;
    }
    
    if (psMulticastAddress)
    {
        eZBZLL_MoveColourTemperature(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), u8Mode, u16Rate, u16ColourTemperatureMin, u16ColourTemperatureMax);
    }
    else
    {
        DBG_PrintNode(psZCBNode);
        eStatus = eJIP_Status_from_ZCB(eZBZLL_MoveColourTemperature(psZCBNode, 1, u8Mode, u16Rate, u16ColourTemperatureMin, u16ColourTemperatureMax));
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }

    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


static teJIP_Status ColourLampGetColourTempTarget(tsVar *psVar)
{
    uint16_t u16ColourTemperature;

    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if ((eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_COLOR_CONTROL, 0, 0, 0, E_ZB_ATTRIBUTEID_COLOUR_COLOURTEMPERATURE, &u16ColourTemperature)) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }

   
    DBG_vPrintf(DBG_COLOURLAMP, "Current Colour Temperature read as: %d\n", u16ColourTemperature);
    u16ColourTemperature = (uint32_t)1000000 / (uint32_t)u16ColourTemperature;
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eJIP_SetVarValue(psVar, &u16ColourTemperature, sizeof(uint16_t));
}


static teJIP_Status ColourLampSetAddGroup(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    struct in6_addr sAddress;
    char acAddr[INET6_ADDRSTRLEN] = "Could not determine address\n";
    uint16_t u16GroupAddress;
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if (psVar->pvData)
    {
        vJIPserver_GroupMibCompressedAddressToIn6(&sAddress, psVar->pvData, psVar->u8Size);
        inet_ntop(AF_INET6, &sAddress, acAddr, INET6_ADDRSTRLEN);

        u16GroupAddress = (sAddress.s6_addr[14] << 8) | (sAddress.s6_addr[15]);
        
        DBG_vPrintf(DBG_COLOURLAMP, "Add Group: %s - 0x%04X\n", acAddr, u16GroupAddress);
        
        switch(eZCB_AddGroupMembership(psZCBNode, u16GroupAddress))
        {
            case (E_ZCB_OK):
                break;
            case (E_ZCB_COMMS_FAILED):
                eUtils_LockUnlock(&psZCBNode->sLock);
                return E_JIP_ERROR_TIMEOUT;
            default:
                eUtils_LockUnlock(&psZCBNode->sLock);
                return E_JIP_ERROR_FAILED;
        }
        
        eUtils_LockUnlock(&psZCBNode->sLock);
        return eJIPserver_NodeGroupJoin(psVar->psOwnerMib->psOwnerNode, acAddr);
    }

    eUtils_LockUnlock(&psZCBNode->sLock);
    return E_JIP_ERROR_FAILED;
}


static teJIP_Status ColourLampSetRemoveGroup(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    struct in6_addr sAddress;
    char acAddr[INET6_ADDRSTRLEN] = "Could not determine address\n";
    uint16_t u16GroupAddress;
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if (psVar->pvData)
    {
        vJIPserver_GroupMibCompressedAddressToIn6(&sAddress, psVar->pvData, psVar->u8Size);
        inet_ntop(AF_INET6, &sAddress, acAddr, INET6_ADDRSTRLEN);

        u16GroupAddress = (sAddress.s6_addr[14] << 8) | (sAddress.s6_addr[15]);
        
        DBG_vPrintf(DBG_COLOURLAMP, "Remove Group: %s - 0x%04X\n", acAddr, u16GroupAddress);
        
        switch(eZCB_RemoveGroupMembership(psZCBNode, u16GroupAddress))
        {
            case (E_ZCB_OK):
                break;
            case (E_ZCB_COMMS_FAILED):
                eUtils_LockUnlock(&psZCBNode->sLock);
                return E_JIP_ERROR_TIMEOUT;
            default:
                eUtils_LockUnlock(&psZCBNode->sLock);
                return E_JIP_ERROR_FAILED;
        }

        eUtils_LockUnlock(&psZCBNode->sLock);
        return eJIPserver_NodeGroupLeave(psVar->psOwnerMib->psOwnerNode, acAddr);
    }

    eUtils_LockUnlock(&psZCBNode->sLock);
    return E_JIP_ERROR_FAILED;
}


static teJIP_Status ColourLampSetClearGroups(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    /* Reset data to 0 */
    *psVar->pu8Data = 0;
    
    switch(eZCB_ClearGroupMembership(psZCBNode))
    {
        case (E_ZCB_OK):
            break;
        case (E_ZCB_COMMS_FAILED):
            eUtils_LockUnlock(&psZCBNode->sLock);
            return E_JIP_ERROR_TIMEOUT;
        default:
            eUtils_LockUnlock(&psZCBNode->sLock);
            return E_JIP_ERROR_FAILED;
    }
    
    /* Now use the JIP builtin clear function to clear our groups table */
    if (prlibJIPGroupClearSetter(psVar, NULL) != E_JIP_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_FAILED;
    }
    
    switch(eZCB_AddGroupMembership(psZCBNode, 0xf00f))
    {
        case (E_ZCB_OK):
            break;
        case (E_ZCB_COMMS_FAILED):
            eUtils_LockUnlock(&psZCBNode->sLock);
            return E_JIP_ERROR_TIMEOUT;
        default:
            eUtils_LockUnlock(&psZCBNode->sLock);
            return E_JIP_ERROR_FAILED;
    }
    
    /* And update JIP */
    (void)eJIPserver_NodeGroupJoin(psVar->psOwnerMib->psOwnerNode, "ff15::f00f");
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return E_JIP_OK;
}


static teJIP_Status ColourLampGetGroups(tsVar *psVar)
{
    teJIP_Status eStatus;
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    eStatus = ColourLampGetGroupsImpl(psZCBNode, psVar);
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    
    return eStatus;
}
    
    
static teJIP_Status ColourLampGetGroupsImpl(tsZCB_Node *psZCBNode, tsVar *psVar)
{
    int         i, j;
    uint32_t    u32OrigNumGroups;
    uint16_t    *pau16OrigGroups;
    
    /* Take a copy of the original group membership before this request. */
    u32OrigNumGroups = psZCBNode->u32NumGroups;
    pau16OrigGroups   = malloc(sizeof(uint16_t) * u32OrigNumGroups);
    if (!pau16OrigGroups)
    {
        return E_JIP_ERROR_NO_MEM;
    }
    memcpy(pau16OrigGroups, psZCBNode->pau16Groups, sizeof(uint16_t) * u32OrigNumGroups);
    
    DBG_vPrintf(DBG_COLOURLAMP, "Lamp was in %d groups\n", u32OrigNumGroups);
    
    /* Update groups table */
    switch(eZCB_GetGroupMembership(psZCBNode))
    {
        case (E_ZCB_OK):
            break;
        case (E_ZCB_COMMS_FAILED):
            free(pau16OrigGroups);
            return E_JIP_ERROR_TIMEOUT;
        default:
            free(pau16OrigGroups);
            return E_JIP_ERROR_FAILED;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "Lamp is now in %d groups\n", psZCBNode->u32NumGroups);
    
    /* First, look for groups that node used to be in but is not anymore */
    for (i = 0; i < u32OrigNumGroups; i++)
    {
        int iRemoved = 1;
        for (j = 0; j < psZCBNode->u32NumGroups; j++)
        {
            if (pau16OrigGroups[i] == psZCBNode->pau16Groups[j])
            {
                iRemoved = 0;
                break;
            }
        }
        
        if (iRemoved)
        {
            struct in6_addr sAddress;
            char acAddr[INET6_ADDRSTRLEN] = "Could not determine address\n";

            memset(&sAddress, 0, sizeof(struct in6_addr));
            sAddress.s6_addr[0] = 0xFF;
            sAddress.s6_addr[1] = 0x15;
            sAddress.s6_addr[14] = (pau16OrigGroups[i] >> 8) & 0xFF;
            sAddress.s6_addr[15] = (pau16OrigGroups[i] >> 0) & 0xFF;
            
            inet_ntop(AF_INET6, &sAddress, acAddr, INET6_ADDRSTRLEN);
            
            DBG_vPrintf(DBG_COLOURLAMP, "Remove Group: %s - 0x%04X\n", acAddr, pau16OrigGroups[i]);
            (void)eJIPserver_NodeGroupLeave(psVar->psOwnerMib->psOwnerNode, acAddr);
        }
    }
    
    /* Now, look for groups that node is in now but wasn't before */
    for (i = 0; i < psZCBNode->u32NumGroups; i++)
    {
        int iAdded = 1;
        for (j = 0; j < u32OrigNumGroups; j++)
        {
            if (pau16OrigGroups[i] == psZCBNode->pau16Groups[j])
            {
                iAdded = 0;
                break;
            }
        }
        
        if (iAdded)
        {
            struct in6_addr sAddress;
            char acAddr[INET6_ADDRSTRLEN] = "Could not determine address\n";

            memset(&sAddress, 0, sizeof(struct in6_addr));
            sAddress.s6_addr[0] = 0xFF;
            sAddress.s6_addr[1] = 0x15;
            sAddress.s6_addr[14] = (psZCBNode->pau16Groups[i] >> 8) & 0xFF;
            sAddress.s6_addr[15] = (psZCBNode->pau16Groups[i] >> 0) & 0xFF;
            
            inet_ntop(AF_INET6, &sAddress, acAddr, INET6_ADDRSTRLEN);
            
            DBG_vPrintf(DBG_COLOURLAMP, "Add Group: %s - 0x%04X\n", acAddr, psZCBNode->pau16Groups[i]);
            (void)eJIPserver_NodeGroupJoin(psVar->psOwnerMib->psOwnerNode, acAddr);
        }
    }
    
    /* Free temporary copy of groups table */
    free(pau16OrigGroups);
    
    DBG_vPrintf(DBG_COLOURLAMP, "Call libJIP get groups function: %p\n", prlibJIPGroupsGetter);
    if (!prlibJIPGroupsGetter)
    {
        return E_JIP_ERROR_FAILED;
    }
    
    /* Use the built in libJIP function to update the groups table variable before returning it */
    return prlibJIPGroupsGetter(psVar);
}


static teJIP_Status ColourLampSetAddScene(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint16_t u16SceneID = *(uint16_t*)psVar->pvData;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Duplicate request\n");
        return eLastStatus;
    }

    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "Add scene: %d\n", u16SceneID);

    if (psMulticastAddress)
    {
        if (eZCB_StoreScene(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), u16SceneID & 0xFF) != E_ZCB_OK)
        {
            eStatus = E_JIP_ERROR_FAILED;
        }
    }
    else
    {
        eStatus = eJIP_Status_from_ZCB(eZCB_StoreScene(psZCBNode, 0xf00f, u16SceneID & 0xFF));
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


static teJIP_Status ColourLampSetRemoveScene(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint16_t u16SceneID = *(uint16_t*)psVar->pvData;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Duplicate request\n");
        return eLastStatus;
    }

    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "Remove scene: %d\n", u16SceneID);

    if (psMulticastAddress)
    {
        if (eZCB_RemoveScene(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), u16SceneID & 0xFF) != E_ZCB_OK)
        {
            eStatus = E_JIP_ERROR_FAILED;
        }
    }
    else
    {
        eStatus = eJIP_Status_from_ZCB(eZCB_RemoveScene(psZCBNode, 0xf00f, u16SceneID & 0xFF));
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


static teJIP_Status ColourLampSetScene(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint16_t u16SceneID = *(uint16_t*)psVar->pvData;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Duplicate request\n");
        return eLastStatus;
    }

    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    DBG_vPrintf(DBG_COLOURLAMP, "Set scene: %d\n", u16SceneID);

    if (psMulticastAddress)
    {
        if (eZCB_RecallScene(NULL, u16BR_IPv6MulticastToBroadcast(psMulticastAddress), u16SceneID & 0xFF) != E_ZCB_OK)
        {
            eStatus = E_JIP_ERROR_FAILED;
        }
    }
    else
    {
        eStatus = eJIP_Status_from_ZCB(eZCB_RecallScene(psZCBNode, 0xf00f, u16SceneID & 0xFF));
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


static teJIP_Status ColourLampGetScene(tsVar *psVar)
{
    uint8_t     u8CurrentScene;
    uint16_t    u16CurrentScene;
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if ((eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_SCENES, 0, 0, 0, E_ZB_ATTRIBUTEID_SCENE_CURRENTSCENE, &u8CurrentScene)) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }
    
    u16CurrentScene = u8CurrentScene;
    
    DBG_vPrintf(DBG_COLOURLAMP, "Current Scene attribute read as: 0x%04X\n", u16CurrentScene);
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eJIP_SetVarValue(psVar, &u16CurrentScene, sizeof(uint16_t));
}


static teJIP_Status ColourLampGetSceneIDs(tsVar *psVar)
{
    uint8_t u8NumScenes, i;
    uint8_t *pu8Scenes = NULL;
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if (eZCB_GetSceneMembership(psZCBNode, 0xf00f, &u8NumScenes, &pu8Scenes) != E_ZCB_OK)
    {
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }
    
    if (!psVar->ptData)
    {
        psVar->ptData = malloc(sizeof(tsTable));
        if (psVar->ptData)
        {
            psVar->ptData->u32NumRows = 0;
            psVar->ptData->psRows     = NULL;
        }
    }
    else
    {
        for (i = 0; i < psVar->ptData->u32NumRows; i++)
        {
            DBG_vPrintf(DBG_COLOURLAMP, "Clear scenes table row %d\n", i);
            if (eJIP_Table_UpdateRow(psVar, i, NULL, 0) != E_JIP_OK)
            {
                free(pu8Scenes);
                eUtils_LockUnlock(&psZCBNode->sLock);
                return E_JIP_ERROR_FAILED;
            }
        }
    }

    for (i = 0; i < u8NumScenes; i++)
    {
        DBG_vPrintf(DBG_COLOURLAMP, "Update scenes table row %d\n", i);
        if (eJIP_Table_UpdateRow(psVar, i, &pu8Scenes[i], sizeof(uint8_t)) != E_JIP_OK)
        {
            free(pu8Scenes);
            eUtils_LockUnlock(&psZCBNode->sLock);
            return E_JIP_ERROR_FAILED;
        }
    }
    free(pu8Scenes);
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return E_JIP_OK;
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

