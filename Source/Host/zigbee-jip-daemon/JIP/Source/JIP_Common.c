/****************************************************************************
 *
 * MODULE:             Zigbee - JIP Daemon
 *
 * COMPONENT:          Fake Border router
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

#include "JIP_Common.h"
#include "JIP_BorderRouter.h"
#include "Utils.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_COMMON 0

#define DUPLICATE_IGNORE_MSEC 2000

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

static uint64_t u64TimevalDiff(struct timeval *psStartTime, struct timeval *psFinishTime);

static teJIP_Status eJIPCommon_FactoryReset(tsVar *psVar, tsJIPAddress *psMulticastAddress);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/



/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

teJIP_Status eLastStatus = E_JIP_OK;
static tsMib sLastAccessedMib;
static tsVar sLastAccessedVar;
static struct in6_addr sLastAccessedAddress;
static struct timeval sLastAccessTime;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

teJIP_Status eJIPCommon_Initialise(void)
{
    memset(&sLastAccessedMib,       0, sizeof(tsMib));
    memset(&sLastAccessedVar,       0, sizeof(tsVar));
    memset(&sLastAccessedAddress,   0, sizeof(struct in6_addr));
    memset(&sLastAccessTime,        0, sizeof(struct timeval));
    return E_JIP_OK;
}
    
    
teJIP_Status eJIPCommon_InitialiseDevice(tsNode *psJIPNode, tsZCB_Node *psZCBNode, const char *pcName)
{
    tsMib       *psMib;
    tsVar       *psVar;
    
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
            DBG_vPrintf(DBG_COMMON, "Set MAC address\n");
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
        
        psVar = psJIP_LookupVarIndex(psMib, 1);
        if (psVar)
        {
            /* Descriptive Name */
            char acNameBuf[200];
            snprintf(acNameBuf, 200, "%s %04X", pcName, psZCBNode->u16ShortAddress);

            if (eJIP_SetVar(&sJIP_Context, psVar, (void*)acNameBuf, strlen(acNameBuf), E_JIP_FLAG_NONE) != E_JIP_OK)
            {
                return E_JIP_ERROR_FAILED;
            }
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
    }
    
    psMib = psJIP_LookupMibId(psJIPNode, NULL, 0xfffffe82);
    if (psMib)
    {
        /* NodeControl MIB */        
        psVar = psJIP_LookupVarIndex(psMib, 1);
        if (psVar)
        {
            /* Factory Reset */
            psVar->prCbVarSet = eJIPCommon_FactoryReset;
            
            // Enable variable
            psVar->eEnable = E_JIP_VAR_ENABLED;
        }
    }
    
    return E_JIP_OK;
}


int iJIPCommon_DuplicateRequest(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    if (sLastAccessedVar.pvData)
    {
        struct timeval sNow;
        gettimeofday(&sNow, NULL);
        
        if (sLastAccessedMib.u32MibId != psVar->psOwnerMib->u32MibId)
        {
            DBG_vPrintf(DBG_COMMON, "MIB ID 0x%08x differs\n", sLastAccessedMib.u32MibId);
            return 0;
        }
        if (sLastAccessedVar.u8Index  != psVar->u8Index)
        {
            DBG_vPrintf(DBG_COMMON, "Var Index %d differs\n", sLastAccessedVar.u8Index);
            return 0;
        }
        if (memcmp(sLastAccessedVar.pvData, psVar->pvData, psVar->u8Size))
        {
            DBG_vPrintf(DBG_COMMON, "Data differs\n");
            return 0;
        }
        if (psMulticastAddress)
        {
            /* Request is multicast */
            if (memcmp(&sLastAccessedAddress, &psMulticastAddress->sin6_addr, sizeof(struct in6_addr)))
            {
                DBG_vPrintf(DBG_COMMON, "IPv6 address differs\n");
                return 0;
            }
        }
        else
        {
            /* Request is unicast */
            if (memcmp(&sLastAccessedAddress, &psVar->psOwnerMib->psOwnerNode->sNode_Address.sin6_addr, sizeof(struct in6_addr)))
            {
                DBG_vPrintf(DBG_COMMON, "IPv6 address differs\n");
                return 0;
            }
        }
        if (u64TimevalDiff(&sLastAccessTime, &sNow) > DUPLICATE_IGNORE_MSEC)
        {
            DBG_vPrintf(DBG_COMMON, "Not within %dms\n", DUPLICATE_IGNORE_MSEC);
            return 0;
        }
    }
    else
    {
        DBG_vPrintf(DBG_COMMON, "Not previously set\n");
        return 0;
    }
    DBG_vPrintf(DBG_COMMON, "Duplicate request\n");
    return 1;
}


void vJIPCommon_DuplicateRequestUpdate(tsVar *psVar, tsJIPAddress *psMulticastAddress, teJIP_Status eStatus)
{
    /* Set up saved last request so that we can detect dumplicates */
    sLastAccessedMib.u32MibId = psVar->psOwnerMib->u32MibId;
    sLastAccessedVar.u8Index  = psVar->u8Index;
    gettimeofday(&sLastAccessTime, NULL);
    
    eLastStatus = eStatus;

    if (psMulticastAddress)
    {
        /* Request is multicast */
        sLastAccessedAddress = psMulticastAddress->sin6_addr;
    }
    else
    {
        /* Request is unicast */
        sLastAccessedAddress = psVar->psOwnerMib->psOwnerNode->sNode_Address.sin6_addr;
    }
    
    eJIP_SetVarValue(&sLastAccessedVar, psVar->pvData, psVar->u8Size);
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

static uint64_t u64TimevalDiff(struct timeval *psStartTime, struct timeval *psFinishTime)
{
    uint64_t u64MSec;
    struct timeval sDifference;
    
    timersub(psFinishTime, psStartTime, &sDifference);
    u64MSec = (sDifference.tv_sec * 1000ll) + (sDifference.tv_usec / 1000ll);
    
    DBG_vPrintf(DBG_COMMON, "Time difference is %lld milliseconds\n", (unsigned long long int)u64MSec);
    return u64MSec;
}


static teJIP_Status eJIPCommon_FactoryReset(tsVar *psVar, tsJIPAddress *psMulticastAddress)
{
    uint8_t *pu8Data = (uint8_t*)psVar->pvData;
    uint16_t u16TransitionTime = 5;
    teJIP_Status eStatus = E_JIP_OK;

    tsZCB_Node *psZCBNode = psBR_FindZigbeeNode(psVar->psOwnerMib->psOwnerNode);
    if (!psZCBNode)
    {
        daemon_log(LOG_ERR, "Could not find Zigbee node");
        return E_JIP_ERROR_FAILED;
    }
    
    if (*pu8Data)
    {
        DBG_vPrintf(DBG_COMMON, "Factory reset device\n");
        if (psMulticastAddress)
        {
            /* Zigbee doesn't allow leave requests to be broadcast */
            eStatus = E_JIP_ERROR_FAILED;
        }
        else
        {
            eStatus = eJIP_Status_from_ZCB(eZCB_LeaveRequest(psZCBNode));
        }
    }
    eUtils_LockUnlock(&psZCBNode->sLock);
    return eStatus;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

