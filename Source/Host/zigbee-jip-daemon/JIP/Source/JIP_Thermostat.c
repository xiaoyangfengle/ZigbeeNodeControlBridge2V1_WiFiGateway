/****************************************************************************
 *
 * MODULE:             Zigbee - JIP Daemon
 *
 * COMPONENT:          JIP Interface to thermostat
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

#include "JIP_Thermostat.h"
#include "JIP_Common.h"
#include "JIP_BorderRouter.h"
#include "ZigbeeConstant.h"
//#include "ZigbeeThermostat.h"
#include "Utils.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_THERMOSTAT 0

#define JIP_MIB_THERMOSTATSTATUS    0xfffffe50
#define JIP_MIB_THERMOSTATCONTROL   0xfffffe51

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

static teJIP_Status ThermostatSetMode(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ThermostatGetMode(tsVar *psVar);
static teJIP_Status ThermostatGetHeatDemand(tsVar *psVar);
static teJIP_Status ThermostatGetCoolDemand(tsVar *psVar);

static teJIP_Status ThermostatSetpointSet(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ThermostatSetPointChange(tsVar *psVar, tsJIPAddress *psMulticastAddress);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

teJIP_Status eThermostatInitialise(tsZCB_Node *psZCBNode, tsNode *psJIPNode)
{
    tsMib       *psMib;
    tsVar       *psVar;
    uint8_t     u8Zero = 0;
    uint16_t    u16Zero = 0;
    
    DBG_vPrintf(DBG_THERMOSTAT, "Set up device\n");

    psMib = psJIP_LookupMibId(psJIPNode, NULL, JIP_MIB_THERMOSTATSTATUS);
    if (psMib)
    {
        /* ThermostatStatus */
        psVar = psJIP_LookupVarIndex(psMib, 0);
        if (psVar)
        {
            /* LocalTemperature */
            eJIP_SetVarValue(psVar, (void*)&u16Zero, sizeof(u16Zero));
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 1);
        if (psVar)
        {
            /* CoolDemand */
            //eJIP_SetVar(&sJIP_Context, psVar, (void*)&u8Zero, sizeof(u8Zero));
            psVar->prCbVarGet = ThermostatGetCoolDemand;
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 2);
        if (psVar)
        {
            /* HeatDemand */
            //eJIP_SetVar(&sJIP_Context, psVar, (void*)&u8Zero, sizeof(u8Zero));
            psVar->prCbVarGet = ThermostatGetHeatDemand;
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
    }
    
    psMib = psJIP_LookupMibId(psJIPNode, NULL, JIP_MIB_THERMOSTATCONTROL);
    if (psMib)
    {
        /* ThermostatControl */
        psVar = psJIP_LookupVarIndex(psMib, 0);
        if (psVar)
        {
            /* Mode */
            eJIP_SetVarValue(psVar, &u8Zero, sizeof(uint8_t));
            //psVar->prCbVarSet = ThermostatSetMode;
            //psVar->prCbVarGet = ThermostatGetMode;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 1);
        if (psVar)
        {
            /* CoolSetPoint */
            u16Zero = 2800;
            eJIP_SetVarValue(psVar, (void*)&u16Zero, sizeof(u16Zero));
            psVar->prCbVarSet = ThermostatSetpointSet;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 2);
        if (psVar)
        {
            /* CoolSetPointChange */
            eJIP_SetVarValue(psVar, (void*)&u16Zero, sizeof(u16Zero));
            psVar->prCbVarSet = ThermostatSetPointChange;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 3);
        if (psVar)
        {
            /* HeatSetPoint */
            u16Zero = 2400;
            eJIP_SetVarValue(psVar, (void*)&u16Zero, sizeof(u16Zero));
            psVar->prCbVarSet = ThermostatSetpointSet;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 4);
        if (psVar)
        {
            /* HeatSetPointChange */
            eJIP_SetVarValue(psVar, (void*)&u16Zero, sizeof(u16Zero));
            psVar->prCbVarSet = ThermostatSetPointChange;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
    }
    
    return eJIPCommon_InitialiseDevice(psJIPNode, psZCBNode, "Thermostat");
}


void vThermostatUpdate(tsZCB_Node *psZCBNode, tsNode *psJIPNode, uint16_t u16ClusterID, uint16_t u16AttributeID, teZCL_ZCLAttributeType eType, tuZcbAttributeData uData)
{
    tsMib *psMib;
    tsVar *psVar;
    
    DBG_vPrintf(DBG_THERMOSTAT, "Update JIP Thermostat\n");
    
    if (u16ClusterID == E_ZB_CLUSTERID_TEMPERATURE)
    {
        DBG_vPrintf(DBG_THERMOSTAT, "Update from temperature cluster attribute 0x%04X\n", u16AttributeID);
        
        switch (u16AttributeID)
        {
            case(E_ZB_ATTRIBUTEID_TEMPERATURE_MEASURED):
            {
                DBG_vPrintf(DBG_THERMOSTAT, "Update local temperature to %d\n", uData.u16Data);
                
                psMib = psJIP_LookupMibId(psJIPNode, NULL, JIP_MIB_THERMOSTATSTATUS);
                if (psMib)
                {
                    /* ThermostatStatus */
                    psVar = psJIP_LookupVarIndex(psMib, 0);
                    if (psVar)
                    {
                        /* LocalTemperature */
                        eJIP_SetVarValue(psVar, (void*)&uData.u16Data, sizeof(uint16_t));
                    }
                }
                break;
                
            default:
                DBG_vPrintf(DBG_THERMOSTAT, "Update for unknown attribute\n");
            }
        }
    }
    else if (u16ClusterID == E_ZB_CLUSTERID_THERMOSTAT)
    {
        DBG_vPrintf(DBG_THERMOSTAT, "Update from thermostat cluster attribute 0x%04X\n", u16AttributeID);
        
        switch (u16AttributeID)
        {
            case(E_ZB_ATTRIBUTEID_TSTAT_LOCALTEMPERATURE):
            {
                DBG_vPrintf(DBG_THERMOSTAT, "Update local temperature to %d\n", uData.u16Data);
                
                psMib = psJIP_LookupMibId(psJIPNode, NULL, JIP_MIB_THERMOSTATSTATUS);
                if (psMib)
                {
                    /* ThermostatStatus */
                    psVar = psJIP_LookupVarIndex(psMib, 0);
                    if (psVar)
                    {
                        /* LocalTemperature */
                        eJIP_SetVarValue(psVar, (void*)&uData.u16Data, sizeof(uint16_t));
                    }
                }
                break;
            }
            
            case(E_ZB_ATTRIBUTEID_TSTAT_PICOOLINGDEMAND):
            {
                DBG_vPrintf(DBG_THERMOSTAT, "Update cooling demand to %d\n", uData.u16Data);
                
                psMib = psJIP_LookupMibId(psJIPNode, NULL, JIP_MIB_THERMOSTATSTATUS);
                if (psMib)
                {
                    /* ThermostatStatus */
                    psVar = psJIP_LookupVarIndex(psMib, 1);
                    if (psVar)
                    {
                        /* CoolDemand */
                        //eJIP_SetVarValue(psVar, (void*)&uData.u16Data, sizeof(uint16_t));
                    }
                }
                break;
            }
            
            case(E_ZB_ATTRIBUTEID_TSTAT_PIHEATINGDEMAND):
            {
                DBG_vPrintf(DBG_THERMOSTAT, "Update heating demand to %d\n", uData.u16Data);
                
                psMib = psJIP_LookupMibId(psJIPNode, NULL, JIP_MIB_THERMOSTATSTATUS);
                if (psMib)
                {
                    /* ThermostatStatus */
                    psVar = psJIP_LookupVarIndex(psMib, 2);
                    if (psVar)
                    {
                        /* HeatDemand */
                        //eJIP_SetVarValue(psVar, (void*)&uData.u16Data, sizeof(uint16_t));
                    }
                }
                break;
            }
            
            case(E_ZB_ATTRIBUTEID_TSTAT_OCCUPIEDCOOLSETPOINT):
            {
                DBG_vPrintf(DBG_THERMOSTAT, "Update cooling setpoint to %d\n", uData.u16Data);
                
                psMib = psJIP_LookupMibId(psJIPNode, NULL, JIP_MIB_THERMOSTATCONTROL);
                if (psMib)
                {
                    /* ThermostatControl */
                    psVar = psJIP_LookupVarIndex(psMib, 1);
                    if (psVar)
                    {
                        /* CoolSetPoint */
                        //eJIP_SetVarValue(psVar, (void*)&uData.u16Data, sizeof(uint16_t));
                    }
                }
                break;
            }
            
            case(E_ZB_ATTRIBUTEID_TSTAT_OCCUPIEDHEATSETPOINT):
            {
                DBG_vPrintf(DBG_THERMOSTAT, "Update heating setpoint to %d\n", uData.u16Data);
                
                psMib = psJIP_LookupMibId(psJIPNode, NULL, JIP_MIB_THERMOSTATCONTROL);
                if (psMib)
                {
                    /* ThermostatControl */
                    psVar = psJIP_LookupVarIndex(psMib, 3);
                    if (psVar)
                    {
                        /* HeatSetPoint */
                        //eJIP_SetVarValue(psVar, (void*)&uData.u16Data, sizeof(uint16_t));
                    }
                }
                break;
            }
            
            case(E_ZB_ATTRIBUTEID_TSTAT_SYSTEMMODE):
            {
                DBG_vPrintf(DBG_THERMOSTAT, "Update mode to %d\n", uData.u8Data);
                
                psMib = psJIP_LookupMibId(psJIPNode, NULL, JIP_MIB_THERMOSTATCONTROL);
                if (psMib)
                {
                    /* ThermostatControl */
                    psVar = psJIP_LookupVarIndex(psMib, 0);
                    if (psVar)
                    {
                        /* Mode */
                        //eJIP_SetVarValue(psVar, (void*)&uData.u8Data, sizeof(uint8_t));
                    }
                }
                break;
            }

            default:
                DBG_vPrintf(DBG_THERMOSTAT, "Update for unknown attribute\n");
        }
    }
    else
    {
        DBG_vPrintf(DBG_THERMOSTAT, "Update for unknown cluster\n");
    }
    
    return;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

static teJIP_Status ThermostatSetMode(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint8_t *pu8Data = (uint8_t*)psVar->pvData;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_THERMOSTAT, "Duplicate request\n");
        return eLastStatus;
    }
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }

    DBG_vPrintf(DBG_THERMOSTAT, "Set Mode: %d\n", *pu8Data);
    
    eStatus = eJIP_Status_from_ZCB(eZCB_WriteAttributeRequest(psZCBNode, E_ZB_CLUSTERID_THERMOSTAT, 0, 0, 0,
                                      E_ZB_ATTRIBUTEID_TSTAT_SYSTEMMODE, E_ZCL_ENUM8, psVar->pvData));
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


static teJIP_Status ThermostatGetMode(tsVar *psVar)
{
    uint8_t u8Mode;
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if ((eZCB_ReadAttributeRequest(psZCBNode, E_ZB_CLUSTERID_THERMOSTAT, 0, 0, 0, E_ZB_ATTRIBUTEID_TSTAT_SYSTEMMODE, &u8Mode)) != E_ZCB_OK)
    {
        DBG_vPrintf(DBG_THERMOSTAT, "Error reading system mode\n");
        eUtils_LockUnlock(&psZCBNode->sLock);
        return E_JIP_ERROR_TIMEOUT;
    }
    
    DBG_vPrintf(DBG_THERMOSTAT, "Mode attribute read as: 0x%02X\n", u8Mode);
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eJIP_SetVarValue(psVar, &u8Mode, sizeof(uint8_t));
}


static teJIP_Status ThermostatGetHeatDemand(tsVar *psVar)
{
    uint8_t u8Demand = 0;
    tsVar *psLocalTemperatureVar = psJIP_LookupVarIndex(psVar->psOwnerMib, 0);
    tsMib *psMib = psJIP_LookupMibId(psVar->psOwnerMib->psOwnerNode, NULL, JIP_MIB_THERMOSTATCONTROL);
    tsVar *psSetpointVar = NULL;
    
    // For now we can fake the demand by checking the local temperature against the setpoint.
    
    if (psMib)
    {
        psSetpointVar = psJIP_LookupVarIndex(psMib, 3);
    }
    if (psLocalTemperatureVar && psSetpointVar)
    {
        if (psLocalTemperatureVar->pvData && psSetpointVar->pvData)
        {
            if (*(psLocalTemperatureVar->pi16Data) < *(psSetpointVar->pi16Data))
            {
                u8Demand = 100;
            }
        }
    }
    return eJIP_SetVarValue(psVar, &u8Demand, sizeof(uint8_t));
}


static teJIP_Status ThermostatGetCoolDemand(tsVar *psVar)
{
    uint8_t u8Demand = 0;
    tsVar *psLocalTemperatureVar = psJIP_LookupVarIndex(psVar->psOwnerMib, 0);
    tsMib *psMib = psJIP_LookupMibId(psVar->psOwnerMib->psOwnerNode, NULL, JIP_MIB_THERMOSTATCONTROL);
    tsVar *psSetpointVar = NULL;
    
    // For now we can fake the demand by checking the local temperature against the setpoint.
    
    if (psMib)
    {
        psSetpointVar = psJIP_LookupVarIndex(psMib, 1);
    }
    if (psLocalTemperatureVar && psSetpointVar)
    {
        if (psLocalTemperatureVar->pvData && psSetpointVar->pvData)
        {
            if (*(psLocalTemperatureVar->pi16Data) > *(psSetpointVar->pi16Data))
            {
                u8Demand = 100;
            }
        }
    }
    return eJIP_SetVarValue(psVar, &u8Demand, sizeof(uint8_t));
}


static teJIP_Status ThermostatSetPointChange(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint16_t u16Zero = 0;
    tsVar *psSetpointVar;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (psVar->u8Index == 2)
    {
        DBG_vPrintf(DBG_THERMOSTAT, "Cool setpoint change\n");
        psSetpointVar = psJIP_LookupVarIndex(psVar->psOwnerMib, 1);
    }
    else if (psVar->u8Index == 4)
    {
        DBG_vPrintf(DBG_THERMOSTAT, "Cool setpoint change\n");
        psSetpointVar = psJIP_LookupVarIndex(psVar->psOwnerMib, 3);
    }
    else
    {
        DBG_vPrintf(DBG_THERMOSTAT, "Unknown setpoint change?!\n");
    }
    
    if (psSetpointVar)
    {
        /* HeatSetPoint */
        if (psSetpointVar->prCbVarGet)
        {
            // Call set point getter function if it is set to get initial value
            psSetpointVar->prCbVarGet(psSetpointVar);
        }
        
        *(psSetpointVar->pi16Data) += *(psVar->pi16Data);
        DBG_vPrintf(DBG_THERMOSTAT, "Setpoint updated to %d\n", *(psSetpointVar->pi16Data));
        
        if (psSetpointVar->prCbVarSet)
        {
            // Call set point updated function if it is set.
            if ((eStatus = psSetpointVar->prCbVarSet(psSetpointVar, psMulticastAddress)) != E_JIP_OK)
            {
                DBG_vPrintf(DBG_THERMOSTAT, "Error setting setpoint\n");
                *(psSetpointVar->pi16Data) -= *(psVar->pi16Data);
            }
        }
    }
    
    (void)eJIP_SetVarValue(psVar, (void*)&u16Zero, sizeof(u16Zero));
    
    return eStatus;
}


static teJIP_Status ThermostatSetpointSet(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    int16_t *pi16Data = (int16_t*)psVar->pvData;
    teJIP_Status eStatus = E_JIP_OK;
    
    if (iJIPCommon_DuplicateRequest(psVar, psMulticastAddress))
    {
        DBG_vPrintf(DBG_THERMOSTAT, "Duplicate request\n");
        return eLastStatus;
    }
    
    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if (psVar->u8Index == 1)
    {
        tsVar *psSetpointVar = psJIP_LookupVarIndex(psVar->psOwnerMib, 3);
        if (psSetpointVar)
        {
            if (*psVar->pi16Data <= *psSetpointVar->pi16Data)
            {
                DBG_vPrintf(DBG_THERMOSTAT, "Attempt to set cool lower than heat\n");
                eStatus = E_JIP_ERROR_FAILED;
            }
            else
            {
                DBG_vPrintf(DBG_THERMOSTAT, "Set Cool Setpoint: %d\n", *pi16Data);
            }
        }
//         DBG_vPrintf(DBG_THERMOSTAT, "Set Cool Setpoint: %d\n", *pi16Data);
//         eStatus = eJIP_Status_from_ZCB(eZCB_WriteAttributeRequest(psZCBNode, E_ZB_CLUSTERID_THERMOSTAT, 0, 0, 0,
//                                         E_ZB_ATTRIBUTEID_TSTAT_OCCUPIEDCOOLSETPOINT, E_ZCL_INT16, psVar->pvData));
    }
    else if (psVar->u8Index == 3)
    {
        tsVar *psSetpointVar = psJIP_LookupVarIndex(psVar->psOwnerMib, 1);
        if (psSetpointVar)
        {
            if (*psVar->pi16Data >= *psSetpointVar->pi16Data)
            {
                DBG_vPrintf(DBG_THERMOSTAT, "Attempt to set heat higher than cool\n");
                eStatus = E_JIP_ERROR_FAILED;
            }
            else
            {
                DBG_vPrintf(DBG_THERMOSTAT, "Set Heat Setpoint: %d\n", *pi16Data);
            }
        }
        
//         DBG_vPrintf(DBG_THERMOSTAT, "Set Heat Setpoint: %d\n", *pi16Data);
//         eStatus = eJIP_Status_from_ZCB(eZCB_WriteAttributeRequest(psZCBNode, E_ZB_CLUSTERID_THERMOSTAT, 0, 0, 0,
//                                         E_ZB_ATTRIBUTEID_TSTAT_OCCUPIEDHEATSETPOINT, E_ZCL_INT16, psVar->pvData));
    }
    else
    {
        DBG_vPrintf(DBG_THERMOSTAT, "Unknown setpoint!?\n");
    }
    
    if (eStatus != E_JIP_ERROR_TIMEOUT)
    {
        /* Set up saved last request so that we can detect dumplicates */
        vJIPCommon_DuplicateRequestUpdate(psVar, psMulticastAddress, eStatus);
    }
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

