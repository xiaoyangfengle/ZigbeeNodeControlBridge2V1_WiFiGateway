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
#include "ZigbeeNetwork.h"
#include "Utils.h"



/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_ZBNETWORK 0

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

static uint32_t u32TimevalDiff(struct timeval *psStartTime, struct timeval *psFinishTime);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


tsZCB_Network sZCB_Network;


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


void DBG_PrintNode(tsZCB_Node *psNode)
{
    int i, j, k;

    DBG_vPrintf(DBG_ZBNETWORK, "Node Short Address: 0x%04X, IEEE Address: 0x%016llX MAC Capability 0x%02X Device ID 0x%04X\n", 
                psNode->u16ShortAddress, 
                (unsigned long long int)psNode->u64IEEEAddress,
                psNode->u8MacCapability,
                psNode->u16DeviceID
               );
    for (i = 0; i < psNode->u32NumEndpoints; i++)
    {
        const char *pcProfileName = NULL;
        tsZCB_NodeEndpoint  *psEndpoint = &psNode->pasEndpoints[i];

        switch (psEndpoint->u16ProfileID)
        {
            case (0x0104):  pcProfileName = "ZHA"; break;
            case (0xC05E):  pcProfileName = "ZLL"; break;
            default:        pcProfileName = "Unknown"; break;
        }
            
        DBG_vPrintf(DBG_ZBNETWORK, "  Endpoint %d - Profile 0x%04X (%s)\n", 
                    psEndpoint->u8Endpoint, 
                    psEndpoint->u16ProfileID,
                    pcProfileName
                   );
        
        for (j = 0; j < psEndpoint->u32NumClusters; j++)
        {
            tsZCB_NodeCluster *psCluster = &psEndpoint->pasClusters[j];
            DBG_vPrintf(DBG_ZBNETWORK, "    Cluster ID 0x%04X\n", psCluster->u16ClusterID);
            
            DBG_vPrintf(DBG_ZBNETWORK, "      Attributes:\n");
            for (k = 0; k < psCluster->u32NumAttributes; k++)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "        Attribute ID 0x%04X\n", psCluster->pau16Attributes[k]);
            }
            
            DBG_vPrintf(DBG_ZBNETWORK, "      Commands:\n");
            for (k = 0; k < psCluster->u32NumCommands; k++)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "        Command ID 0x%02X\n", psCluster->pau8Commands[k]);
            }
        }
    }
}


teZcbStatus eZCB_AddNode(uint16_t u16ShortAddress, uint64_t u64IEEEAddress, uint16_t u16DeviceID, uint8_t u8MacCapability, tsZCB_Node **ppsZCBNode)
{
    teZcbStatus eStatus = E_ZCB_OK;
    tsZCB_Node *psZCBNode = &sZCB_Network.sNodes;
    
    eUtils_LockLock(&sZCB_Network.sLock);
    
    while (psZCBNode->psNext)
    {
        if (u64IEEEAddress)
        {
            if (psZCBNode->psNext->u64IEEEAddress == u64IEEEAddress)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "IEEE address already in network - update short address\n");
                eUtils_LockLock(&psZCBNode->psNext->sLock);
                psZCBNode->psNext->u16ShortAddress = u16ShortAddress;
                
                if (ppsZCBNode)
                {
                    *ppsZCBNode = psZCBNode->psNext;
                }
                else
                {
                    eUtils_LockUnlock(&psZCBNode->psNext->sLock);
                }
                goto done;
            }
            if (psZCBNode->psNext->u16ShortAddress == u16ShortAddress)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "Short address already in network - update IEEE address\n");
                eUtils_LockLock(&psZCBNode->psNext->sLock);
                psZCBNode->psNext->u64IEEEAddress = u64IEEEAddress;

                if (ppsZCBNode)
                {
                    *ppsZCBNode = psZCBNode->psNext;
                }
                else
                {
                    eUtils_LockUnlock(&psZCBNode->psNext->sLock);
                }
                goto done;
            }
        }
        else
        {
            if (psZCBNode->psNext->u16ShortAddress == u16ShortAddress)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "Short address already in network\n");
                eUtils_LockLock(&psZCBNode->psNext->sLock);

                if (ppsZCBNode)
                {
                    *ppsZCBNode = psZCBNode->psNext;
                }
                else
                {
                    eUtils_LockUnlock(&psZCBNode->psNext->sLock);
                }
                goto done;
            }
            
        }
        psZCBNode = psZCBNode->psNext;
    }
    
    psZCBNode->psNext = malloc(sizeof(tsZCB_Node));
    
    if (!psZCBNode->psNext)
    {
        daemon_log(LOG_CRIT, "Memory allocation failure allocating node");
        eStatus = E_ZCB_ERROR_NO_MEM;
        goto done;
    }
    
    memset(psZCBNode->psNext, 0, sizeof(tsZCB_Node));

    /* Got to end of list without finding existing node - add it at the end of the list */
    eUtils_LockCreate(&psZCBNode->psNext->sLock);
    psZCBNode->psNext->u16ShortAddress  = u16ShortAddress;
    psZCBNode->psNext->u64IEEEAddress   = u64IEEEAddress;
    psZCBNode->psNext->u8MacCapability  = u8MacCapability;
    psZCBNode->psNext->u16DeviceID      = u16DeviceID;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Created new Node\n");
    DBG_PrintNode(psZCBNode->psNext);
    
    if (ppsZCBNode)
    {
        eUtils_LockLock(&psZCBNode->psNext->sLock);
        *ppsZCBNode = psZCBNode->psNext;
    }

done:
    eUtils_LockUnlock(&sZCB_Network.sLock);
    return eStatus;
}


teZcbStatus eZCB_RemoveNode(tsZCB_Node *psZCBNode)
{
    teZcbStatus eStatus = E_ZCB_ERROR;
    tsZCB_Node *psZCBCurrentNode = &sZCB_Network.sNodes;
    int iNodeFreeable = 0;
    
    /* lock the list mutex and node mutex in the same order as everywhere else to avoid deadlock */
    
    eUtils_LockUnlock(&psZCBNode->sLock);
    
    eUtils_LockLock(&sZCB_Network.sLock);
    
    eUtils_LockLock(&psZCBNode->sLock);

    if (psZCBNode == &sZCB_Network.sNodes)
    {
        eStatus = E_ZCB_OK;
        iNodeFreeable = 0;
    }
    else
    {
        while (psZCBCurrentNode)
        {
            if (psZCBCurrentNode->psNext == psZCBNode)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "Found node to remove\n");
                DBG_PrintNode(psZCBNode);
                
                psZCBCurrentNode->psNext = psZCBCurrentNode->psNext->psNext;
                eStatus = E_ZCB_OK;
                iNodeFreeable = 1;
                break;
            }
            psZCBCurrentNode = psZCBCurrentNode->psNext;
        }
    }
    
    if (eStatus == E_ZCB_OK)
    {
        int i, j;
        for (i = 0; i < psZCBNode->u32NumEndpoints; i++)
        {
            DBG_vPrintf(DBG_ZBNETWORK, "Free endpoint %d\n", psZCBNode->pasEndpoints[i].u8Endpoint);
            if (psZCBNode->pasEndpoints[i].pasClusters)
            {
                for (j = 0; j < psZCBNode->pasEndpoints[i].u32NumClusters; j++)
                {
                    DBG_vPrintf(DBG_ZBNETWORK, "Free cluster 0x%04X\n", psZCBNode->pasEndpoints[i].pasClusters[j].u16ClusterID);
                    free(psZCBNode->pasEndpoints[i].pasClusters[j].pau16Attributes);
                    free(psZCBNode->pasEndpoints[i].pasClusters[j].pau8Commands);
                }
                free(psZCBNode->pasEndpoints[i].pasClusters);
                
            }
        }
        free(psZCBNode->pasEndpoints);
        
        free(psZCBNode->pau16Groups);
        
        /* Unlock the node first so that it may be free'd */
        eUtils_LockUnlock(&psZCBNode->sLock);
        eUtils_LockDestroy(&psZCBNode->sLock);
        if (iNodeFreeable)
        {
            free(psZCBNode);
        }
    }
    eUtils_LockUnlock(&sZCB_Network.sLock);
    return eStatus;
}


tsZCB_Node *psZCB_FindNodeIEEEAddress(uint64_t u64IEEEAddress)
{
    tsZCB_Node *psZCBNode = &sZCB_Network.sNodes;
    
    eUtils_LockLock(&sZCB_Network.sLock);

    while (psZCBNode)
    {
        if (psZCBNode->u64IEEEAddress == u64IEEEAddress)
        {
            int iLockAttempts = 0;
            
            DBG_vPrintf(DBG_ZBNETWORK, "IEEE address 0x%016llX found in network\n", (unsigned long long int)u64IEEEAddress);
            DBG_PrintNode(psZCBNode);
            
            while (++iLockAttempts < 5)
            {
                if (eUtils_LockLock(&psZCBNode->sLock) == E_UTILS_OK)
                {
                    break;
                }
                else
                {
                    eUtils_LockUnlock(&sZCB_Network.sLock);
                    
                    if (iLockAttempts == 5)
                    {
                        daemon_log(LOG_ERR, "\n\nError: Could not get lock on node!!\n");
                        return NULL;
                    }
                    
                    usleep(1000000);
                    eUtils_LockLock(&sZCB_Network.sLock);
                }
            }
            break;
        }
        psZCBNode = psZCBNode->psNext;
    }
    
    eUtils_LockUnlock(&sZCB_Network.sLock);
    return psZCBNode;
}


tsZCB_Node *psZCB_FindNodeShortAddress(uint16_t u16ShortAddress)
{
    tsZCB_Node *psZCBNode = &sZCB_Network.sNodes;
    
    eUtils_LockLock(&sZCB_Network.sLock);

    while (psZCBNode)
    {
        if (psZCBNode->u16ShortAddress == u16ShortAddress)
        {
            int iLockAttempts = 0;
            
            DBG_vPrintf(DBG_ZBNETWORK, "Short address 0x%04X found in network\n", u16ShortAddress);
            DBG_PrintNode(psZCBNode);
            
            while (++iLockAttempts < 5)
            {
                if (eUtils_LockLock(&psZCBNode->sLock) == E_UTILS_OK)
                {
                    break;
                }
                else
                {
                    eUtils_LockUnlock(&sZCB_Network.sLock);
                    
                    if (iLockAttempts == 5)
                    {
                        daemon_log(LOG_ERR, "\n\nError: Could not get lock on node!!\n");
                        return NULL;
                    }
                    
                    usleep(1000000);
                    eUtils_LockLock(&sZCB_Network.sLock);
                }
            }
            break;
        }
        psZCBNode = psZCBNode->psNext;
    }
    
    eUtils_LockUnlock(&sZCB_Network.sLock);
    return psZCBNode;
}


tsZCB_Node *psZCB_FindNodeControlBridge(void)
{
    tsZCB_Node *psZCBNode = &sZCB_Network.sNodes;
    eUtils_LockLock(&psZCBNode->sLock);
    return psZCBNode;
}


teZcbStatus eZCB_NodeAddEndpoint(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ProfileID, tsZCB_NodeEndpoint **ppsEndpoint)
{
    tsZCB_NodeEndpoint *psNewEndpoint;
    int i;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Add Endpoint %d, profile 0x%04X to node 0x%04X\n", u8Endpoint, u16ProfileID, psZCBNode->u16ShortAddress);
    
    for (i = 0; i < psZCBNode->u32NumEndpoints; i++)
    {
        if (psZCBNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            DBG_vPrintf(DBG_ZBNETWORK, "Duplicate Endpoint\n");
            if (u16ProfileID)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "Set Endpoint %d profile to 0x%04X\n", u8Endpoint, u16ProfileID);
                psZCBNode->pasEndpoints[i].u16ProfileID = u16ProfileID;
            }
            return E_ZCB_OK;
        }
    }
    
    DBG_vPrintf(DBG_ZBNETWORK, "Creating new endpoint %d\n", u8Endpoint);
    
    psNewEndpoint = realloc(psZCBNode->pasEndpoints, sizeof(tsZCB_NodeEndpoint) * (psZCBNode->u32NumEndpoints+1));
    
    if (!psNewEndpoint)
    {
        daemon_log(LOG_CRIT, "Memory allocation failure allocating endpoint");
        return E_ZCB_ERROR_NO_MEM;
    }
    
    psZCBNode->pasEndpoints = psNewEndpoint;
    
    memset(&psZCBNode->pasEndpoints[psZCBNode->u32NumEndpoints], 0, sizeof(tsZCB_NodeEndpoint));
    psNewEndpoint = &psZCBNode->pasEndpoints[psZCBNode->u32NumEndpoints];
    psZCBNode->u32NumEndpoints++;
    
    psNewEndpoint->u8Endpoint = u8Endpoint;
    psNewEndpoint->u16ProfileID = u16ProfileID;
    
    
    if (ppsEndpoint)
    {
        *ppsEndpoint = psNewEndpoint;
    }
    
    return E_ZCB_OK;
}


teZcbStatus eZCB_NodeAddCluster(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ClusterID)
{
    int i;
    tsZCB_NodeEndpoint *psEndpoint = NULL;
    tsZCB_NodeCluster  *psNewClusters;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: Add cluster 0x%04X to Endpoint %d\n", psZCBNode->u16ShortAddress, u16ClusterID, u8Endpoint);
    
    for (i = 0; i < psZCBNode->u32NumEndpoints; i++)
    {
        if (psZCBNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            psEndpoint = &psZCBNode->pasEndpoints[i];
            break;
        }
    }
    if (!psEndpoint)
    {
        DBG_vPrintf(DBG_ZBNETWORK, "Endpoint not found\n");
        return E_ZCB_UNKNOWN_ENDPOINT;
    }
    
    for (i = 0; i < psEndpoint->u32NumClusters; i++)
    {
        if (psEndpoint->pasClusters[i].u16ClusterID == u16ClusterID)
        {
            DBG_vPrintf(DBG_ZBNETWORK, "Duplicate Cluster ID\n");
            return E_ZCB_OK;
        }
    }
    
    psNewClusters = realloc(psEndpoint->pasClusters, sizeof(tsZCB_NodeCluster) * (psEndpoint->u32NumClusters+1));
    if (!psNewClusters)
    {
        daemon_log(LOG_CRIT, "Memory allocation failure allocating clusters");
        return E_ZCB_ERROR_NO_MEM;
    }
    psEndpoint->pasClusters = psNewClusters;
    
    memset(&psEndpoint->pasClusters[psEndpoint->u32NumClusters], 0, sizeof(tsZCB_NodeCluster));
    psEndpoint->pasClusters[psEndpoint->u32NumClusters].u16ClusterID = u16ClusterID;
    psEndpoint->u32NumClusters++;
    
    return E_ZCB_OK;
}


teZcbStatus eZCB_NodeAddAttribute(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ClusterID, uint16_t u16AttributeID)
{
    int i;
    tsZCB_NodeEndpoint *psEndpoint = NULL;
    tsZCB_NodeCluster  *psCluster = NULL;
    uint16_t *pu16NewAttributeList;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: Add Attribute 0x%04X to cluster 0x%04X on Endpoint %d\n",
                psZCBNode->u16ShortAddress, u16AttributeID, u16ClusterID, u8Endpoint);
    
    for (i = 0; i < psZCBNode->u32NumEndpoints; i++)
    {
        if (psZCBNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            psEndpoint = &psZCBNode->pasEndpoints[i];
        }
    }
    if (!psEndpoint)
    {
        DBG_vPrintf(DBG_ZBNETWORK, "Endpoint not found\n");
        return E_ZCB_UNKNOWN_ENDPOINT;
    }
    
    for (i = 0; i < psEndpoint->u32NumClusters; i++)
    {
        if (psEndpoint->pasClusters[i].u16ClusterID == u16ClusterID)
        {
            psCluster = &psEndpoint->pasClusters[i];
        }
    }
    if (!psCluster)
    {
        DBG_vPrintf(DBG_ZBNETWORK, "Cluster not found\n");
        return E_ZCB_UNKNOWN_CLUSTER;
    }

    for (i = 0; i < psCluster->u32NumAttributes; i++)
    {
        if (psCluster->pau16Attributes[i] == u16AttributeID)
        {
            DBG_vPrintf(DBG_ZBNETWORK, "Duplicate Attribute ID\n");
            return E_ZCB_ERROR;
        }
    }

    pu16NewAttributeList = realloc(psCluster->pau16Attributes, sizeof(uint16_t) * (psCluster->u32NumAttributes + 1));
    
    if (!pu16NewAttributeList)
    {
        daemon_log(LOG_CRIT, "Memory allocation failure allocating attributes");
        return E_ZCB_ERROR_NO_MEM;
    }
    psCluster->pau16Attributes = pu16NewAttributeList;
    
    psCluster->pau16Attributes[psCluster->u32NumAttributes] = u16AttributeID;
    psCluster->u32NumAttributes++;
    
    return E_ZCB_OK;
}


teZcbStatus eZCB_NodeAddCommand(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ClusterID, uint8_t u8CommandID)
{
    int i;
    tsZCB_NodeEndpoint *psEndpoint = NULL;
    tsZCB_NodeCluster  *psCluster = NULL;
    uint8_t *pu8NewCommandList;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: Add Command 0x%02X to cluster 0x%04X on Endpoint %d\n",
                psZCBNode->u16ShortAddress, u8CommandID, u16ClusterID, u8Endpoint);
    
    for (i = 0; i < psZCBNode->u32NumEndpoints; i++)
    {
        if (psZCBNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            psEndpoint = &psZCBNode->pasEndpoints[i];
        }
    }
    if (!psEndpoint)
    {
        DBG_vPrintf(DBG_ZBNETWORK, "Endpoint not found\n");
        return E_ZCB_UNKNOWN_ENDPOINT;
    }
    
    for (i = 0; i < psEndpoint->u32NumClusters; i++)
    {
        if (psEndpoint->pasClusters[i].u16ClusterID == u16ClusterID)
        {
            psCluster = &psEndpoint->pasClusters[i];
        }
    }
    if (!psCluster)
    {
        DBG_vPrintf(DBG_ZBNETWORK, "Cluster not found\n");
        return E_ZCB_UNKNOWN_CLUSTER;
    }

    for (i = 0; i < psCluster->u32NumCommands; i++)
    {
        if (psCluster->pau8Commands[i] == u8CommandID)
        {
            DBG_vPrintf(DBG_ZBNETWORK, "Duplicate Command ID\n");
            return E_ZCB_ERROR;
        }
    }

    pu8NewCommandList = realloc(psCluster->pau8Commands, sizeof(uint8_t) * (psCluster->u32NumCommands + 1));
    
    if (!pu8NewCommandList)
    {
        daemon_log(LOG_CRIT, "Memory allocation failure allocating commands");
        return E_ZCB_ERROR_NO_MEM;
    }
    psCluster->pau8Commands = pu8NewCommandList;
    
    psCluster->pau8Commands[psCluster->u32NumCommands] = u8CommandID;
    psCluster->u32NumCommands++;
    
    return E_ZCB_OK;
}


tsZCB_NodeEndpoint *psZCB_NodeFindEndpoint(tsZCB_Node *psZCBNode, uint16_t u16ClusterID)
{
    int i, j;
    tsZCB_NodeEndpoint *psEndpoint = NULL;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: Find cluster 0x%04X\n", psZCBNode->u16ShortAddress, u16ClusterID);
    
    for (i = 0; i < psZCBNode->u32NumEndpoints; i++)
    {
        psEndpoint = &psZCBNode->pasEndpoints[i];
        
        for (j = 0; j < psEndpoint->u32NumClusters; j++)
        {
            if (psEndpoint->pasClusters[j].u16ClusterID == u16ClusterID)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "Found Cluster ID on Endpoint %d\n", psEndpoint->u8Endpoint);
                return psEndpoint;
            }
        }
    }
    DBG_vPrintf(DBG_ZBNETWORK, "Cluster 0x%04X not found on node 0x%04X\n", u16ClusterID, psZCBNode->u16ShortAddress);
    return NULL;
}


teZcbStatus eZCB_NodeClearGroups(tsZCB_Node *psZCBNode)
{
    DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: Clear groups\n", psZCBNode->u16ShortAddress);
    
    if (psZCBNode->pau16Groups)
    {
        free(psZCBNode->pau16Groups);
        psZCBNode->pau16Groups = NULL;
    }
    
    psZCBNode->u32NumGroups = 0;
    return E_ZCB_OK;
}


teZcbStatus eZCB_NodeAddGroup(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress)
{
    uint16_t *pu16NewGroups;
    int i;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: Add group 0x%04X\n", psZCBNode->u16ShortAddress, u16GroupAddress);
    
    if (psZCBNode->pau16Groups)
    {
        for (i = 0; i < psZCBNode->u32NumGroups; i++)
        {
            if (psZCBNode->pau16Groups[i] == u16GroupAddress)
            {
                DBG_vPrintf(DBG_ZBNETWORK, "Node is already in group 0x%04X\n", u16GroupAddress);
                return E_ZCB_OK;
            }
        }
    }

    pu16NewGroups = realloc(psZCBNode->pau16Groups, sizeof(uint16_t) * (psZCBNode->u32NumGroups + 1));
    
    if (!pu16NewGroups)
    {
        daemon_log(LOG_CRIT, "Memory allocation failure allocating groups");
        return E_ZCB_ERROR_NO_MEM;
    }
    
    psZCBNode->pau16Groups = pu16NewGroups;
    psZCBNode->pau16Groups[psZCBNode->u32NumGroups] = u16GroupAddress;
    psZCBNode->u32NumGroups++;
    return E_ZCB_OK;
}


void vZCB_NodeUpdateComms(tsZCB_Node *psZCBNode, teZcbStatus eStatus)
{
    if (!psZCBNode)
    {
        return;
    }
    
    if (eStatus == E_ZCB_OK)
    {
        gettimeofday(&psZCBNode->sComms.sLastSuccessful, NULL);
        psZCBNode->sComms.u16SequentialFailures = 0;
    }
    else if (eStatus == E_ZCB_COMMS_FAILED)
    {
        psZCBNode->sComms.u16SequentialFailures++;
    }
    else
    {
        
    }
#if DBG_ZBNETWORK
    {
        time_t nowtime;
        struct tm *nowtm;
        char tmbuf[64], buf[64];

        nowtime = psZCBNode->sComms.sLastSuccessful.tv_sec;
        nowtm = localtime(&nowtime);
        strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
        snprintf(buf, sizeof buf, "%s.%06d", tmbuf, (int)psZCBNode->sComms.sLastSuccessful.tv_usec);
        DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X: %d sequential comms failures. Last successful comms at %s\n", 
                    psZCBNode->u16ShortAddress, psZCBNode->sComms.u16SequentialFailures, buf);
    }
#endif /* DBG_ZBNETWORK */
}


tsZCB_Node *psZCB_NodeOldestComms(void)
{
    tsZCB_Node *psZCBNode = &sZCB_Network.sNodes;
    tsZCB_Node *psZCBNodeComms = NULL;
    struct timeval sNow;
    uint32_t u32LargestTimeDiff = 0;
    
    eUtils_LockLock(&sZCB_Network.sLock);
    
    gettimeofday(&sNow, NULL);

    while (psZCBNode->psNext)
    {
        uint32_t u32ThisDiff = u32TimevalDiff(&psZCBNode->psNext->sComms.sLastSuccessful, &sNow);
        if (u32ThisDiff > u32LargestTimeDiff)
        {
            u32LargestTimeDiff = u32ThisDiff;
            DBG_vPrintf(DBG_ZBNETWORK, "Node 0x%04X has oldest comms (%dms ago)\n", psZCBNode->psNext->u16ShortAddress, u32ThisDiff);            
            psZCBNodeComms = psZCBNode->psNext;
        }
        psZCBNode = psZCBNode->psNext;
    }

    if (psZCBNodeComms)
    {
        eUtils_LockLock(&psZCBNodeComms->sLock);
    }
    
    eUtils_LockUnlock(&sZCB_Network.sLock);
    return psZCBNodeComms;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

static uint32_t u32TimevalDiff(struct timeval *psStartTime, struct timeval *psFinishTime)
{
    uint32_t u32MSec;
    u32MSec  = ((uint32_t)psFinishTime->tv_sec - (uint32_t)psStartTime->tv_sec) * 1000;
    if (psStartTime->tv_usec > psFinishTime->tv_usec)
    {
        u32MSec += 1000;
        psStartTime->tv_usec -= 1000000;
    }
    u32MSec += ((uint32_t)psFinishTime->tv_usec - (uint32_t)psStartTime->tv_usec) / 1000;
    
    DBG_vPrintf(DBG_ZBNETWORK, "Time difference is %d milliseconds\n", u32MSec);
    return u32MSec;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
