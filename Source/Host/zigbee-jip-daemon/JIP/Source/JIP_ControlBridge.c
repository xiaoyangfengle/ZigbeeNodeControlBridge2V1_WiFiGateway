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

#include "ZigbeeControlBridge.h"
#include "JIP_BorderRouter.h"
#include "Utils.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_CONTROLBRIDGE 0

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

static teJIP_Status ControlBridge_FactoryResetSet(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ControlBridge_PermitJoiningSet(tsVar *psVar, tsJIPAddress *psMulticastAddress);
static teJIP_Status ControlBridge_PermitJoiningGet(tsVar *psVar);
static teJIP_Status ControlBridge_TouchLinkSet(tsVar *psVar, tsJIPAddress *psMulticastAddress);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

teJIP_Status eControlBridgeInitalise(tsZCB_Node *psZCBNode, tsNode *psJIPNode)
{
    tsMib *psMib;
    tsVar *psVar;
    
    DBG_vPrintf(DBG_CONTROLBRIDGE, "Set up device\n");
    
    psMib = psJIP_LookupMibId(psJIPNode, NULL, 0xffffff00);
    if (psMib)
    {
        /* Node MIB */
        psVar = psJIP_LookupVarIndex(psMib, 0);
        if (psVar)
        {
            /* MAC Address */
            if (eJIP_SetVar(&sJIP_Context, psVar, &psZCBNode->u64IEEEAddress, sizeof(uint64_t), E_JIP_FLAG_NONE) != E_JIP_OK)
            {
                return E_JIP_ERROR_FAILED;
            }
            DBG_vPrintf(DBG_CONTROLBRIDGE, "Set MAC address\n");
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 1);
        if (psVar)
        {
            /* Descriptive Name */
            const char *pcName = "Control Bridge";
            if (eJIP_SetVar(&sJIP_Context, psVar, (void*)pcName, strlen(pcName), E_JIP_FLAG_NONE) != E_JIP_OK)
            {
                return E_JIP_ERROR_FAILED;
            }
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
    }
    
    /* Set the callbacks */
    psMib = psJIP_LookupMibId(psJIPNode, NULL, 0xfffffd01);
    if (psMib)
    {
        psVar = psJIP_LookupVarIndex(psMib, 0);
        if (psVar)
        {
            psVar->pvData = malloc(sizeof(uint8_t));
            memset(psVar->pvData, 0, sizeof(uint8_t));
            psVar->prCbVarSet = ControlBridge_FactoryResetSet;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 1);
        if (psVar)
        {
            psVar->pvData = malloc(sizeof(uint8_t));
            memset(psVar->pvData, 0, sizeof(uint8_t));
            psVar->prCbVarSet = ControlBridge_PermitJoiningSet;
            psVar->prCbVarGet = ControlBridge_PermitJoiningGet;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 2);
        if (psVar)
        {
            psVar->pvData = malloc(sizeof(uint8_t));
            memset(psVar->pvData, 0, sizeof(uint8_t));
            psVar->prCbVarSet = ControlBridge_TouchLinkSet;
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
    }
    
    return E_JIP_OK;
}



/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

static teJIP_Status ControlBridge_FactoryResetSet(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint8_t *pu8Data = (uint8_t*)psVar->pvData;

    DBG_vPrintf(DBG_CONTROLBRIDGE, "Factory reset (%d)\n", *pu8Data);
    
    eZCB_FactoryNew();
    
    /* Set back to 0 */
    *pu8Data = 0;
    
    return E_JIP_OK;
}


static teJIP_Status ControlBridge_PermitJoiningSet(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    teJIP_Status eStatus = E_JIP_OK;
    uint8_t *pu8Data = (uint8_t*)psVar->pvData;

    DBG_vPrintf(DBG_CONTROLBRIDGE, "Permit joining (%d)\n", *pu8Data);
    
    if (eZCB_SetPermitJoining(*pu8Data) != E_ZCB_OK)
    {
        eStatus = E_JIP_ERROR_FAILED;
    }
    
    return eStatus;
}


static teJIP_Status ControlBridge_PermitJoiningGet(tsVar *psVar)
{
    teJIP_Status eStatus = E_JIP_OK;
    uint8_t *pu8Data = (uint8_t*)psVar->pvData;

    if (eZCB_GetPermitJoining(pu8Data) != E_ZCB_OK)
    {
        eStatus = E_JIP_ERROR_FAILED;
    }
    
    DBG_vPrintf(DBG_CONTROLBRIDGE, "Permit joining status: %d\n", *pu8Data);
    
    return eStatus;
}


static teJIP_Status ControlBridge_TouchLinkSet(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint8_t *pu8Data = (uint8_t*)psVar->pvData;

    DBG_vPrintf(DBG_CONTROLBRIDGE, "Initiate touchlink (%d)\n", *pu8Data);
    
    eZCB_ZLL_Touchlink();
    
    /* Set back to 0 */
    *pu8Data = 0;
    
    return E_JIP_OK;
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

