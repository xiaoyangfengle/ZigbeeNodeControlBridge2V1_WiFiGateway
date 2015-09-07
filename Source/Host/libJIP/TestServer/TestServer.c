/****************************************************************************
 *
 * MODULE:             libJIP
 *
 * COMPONENT:          Test of Variable Server
 *
 * REVISION:           $Revision: 49621 $
 *
 * DATED:              $Date: 2012-11-21 11:55:41 +0000 (Wed, 21 Nov 2012) $
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

 * Copyright NXP B.V. 2013. All rights reserved
 *
 ***************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>


#include <JIP.h>

#ifndef VERSION
#error Version is not defined!
#else
const char *Version = "0.1 (r" VERSION ")";
#endif

/** JIP cotnext structure */
static tsJIP_Context sJIP_Context;


/** Signal handler function for SIGINT
 *  save XML definitions, clean up JIP context and exit.
 */
void sigint_handler(int sig)
{
    eJIP_Destroy(&sJIP_Context);
    exit(0);
}


int main (int argc, char *argv[])
{
    tsNode *psNode;
    tsMib *psMib;
    tsVar *psVar;

    printf("JIP Server test version %s using libJIP version %s\n\r", Version, JIP_Version);
    
    /* Set up signal handler */
    {
        struct sigaction sa;

        sa.sa_handler = sigint_handler;
        sa.sa_flags = 0; // or SA_RESTART
        sigemptyset(&sa.sa_mask);

        if (sigaction(SIGINT, &sa, NULL) == -1) 
        {
            perror("sigaction");
            exit(1);
        }
    }
    
    if (eJIP_Init(&sJIP_Context, E_JIP_CONTEXT_SERVER) != E_JIP_OK)
    {
        printf("Error initialising server\n");
        return -1;
    }
    
    /* Load definitions file */
    if (eJIPService_PersistXMLLoadDefinitions(&sJIP_Context, "jip_cache_definitions.xml") != E_JIP_OK)
    {
        printf("Error loading node definitions\n");
        return -1;
    }
    
#if 0
    {
        tsJIPAddress sAddress;
        tsMib *psMib;
        tsVar *psVar;
        
        /* Load network file */
        if (eJIPService_PersistXMLLoadNetwork(&sJIP_Context, "jip_cache_network.xml") != E_JIP_OK)
        {
            printf("Error loading network\n");
            return -1;
        }
        memset(&sAddress, 0, sizeof(tsJIPAddress));
        sAddress.sin6_family  = AF_INET6;
        sAddress.sin6_port    = htons(JIP_DEFAULT_PORT);
        inet_pton(AF_INET6, "fd04:bd3:80e8:1:21d:9ff:fec4:e2db", &sAddress.sin6_addr);
        
        psNode = psJIP_LookupNode(&sJIP_Context, &sAddress);
        
        if (!psNode)
        {
            printf("Could not find node\n");
            return -1;
        }
        
        /* Fill in Name and Version */
        psMib = psJIP_LookupMibId(psNode, NULL, 0xffffff00);
        if (psMib)
        {
            psVar = psJIP_LookupVarIndex(psMib, 1);
            
            if (psVar)
            {
                char *pcName = "Test Coordinator";
                if (eJIP_SetVar(&sJIP_Context, psVar, pcName, strlen(pcName)) != E_JIP_OK)
                {
                    return E_JIP_ERROR_FAILED;
                }
            }
            
            psVar = psJIP_LookupVarIndex(psMib, 2);
            
            if (psVar)
            {
                if (eJIP_SetVar(&sJIP_Context, psVar, (char *)Version, strlen(Version)) != E_JIP_OK)
                {
                    return E_JIP_ERROR_FAILED;
                }
            }
        }
    }
#else
    //if (eJIPserver_NodeAdd(&sJIP_Context, "::1", JIP_DEFAULT_PORT, 0x80100001, "Test Coordinator", Version, &psNode) != E_JIP_OK)
    if (eJIPserver_NodeAdd(&sJIP_Context, "fd04:bd3:80e8:1:21d:9ff:fec4:e2db", JIP_DEFAULT_PORT, 0x80100001, "Test Coordinator", (char *)Version, &psNode) != E_JIP_OK)
    {
        printf("Error adding node\n");
        return -1;
    }
#endif
    
    /* Set Network table to be empty */
    psMib = psJIP_LookupMibId(psNode, NULL, 0xffffff01);
    if(psMib)
    {
        psVar = psJIP_LookupVarIndex(psMib, 4);
        
        if (psVar)
        {
            tsTable *psTable = malloc(sizeof(tsTable));
            if (psTable)
            {
                psTable->u32NumRows = 0;
                psTable->psRows     = NULL;
                
                psVar->pvData = psTable;
                printf("Empty network table initialised\n");
            }
        }
    }
    
#if 0
    eJIPserver_NodeRemove(&sJIP_Context, psNode);
#else
    eJIP_UnlockNode(psNode);
#endif
    
    eJIP_PrintNetworkContent(&sJIP_Context);
    
    
    printf("Starting server\n");
    
    if (eJIPserver_Listen(&sJIP_Context) != E_JIP_OK)
    {
        printf("Error starting server\n");
        return -1;
    }
    
    if (eJIPserver_NodeGroupJoin(psNode, "FF15::F00F") != E_JIP_OK)
    {
        printf("Failed to add group\n");
        return -1;
    }
    
    if (eJIPserver_NodeGroupJoin(psNode, "FF15::1234:F00F") != E_JIP_OK)
    {
        printf("Failed to add group\n");
        return -1;
    }
    
//     if (eJIPserver_NodeGroupLeave(psNode, "FF15::F00F") != E_JIP_OK)
//     {
//         printf("Failed to add group\n");
//         return -1;
//     }  
    
    
    
    while (1)
    {
        sleep(1);
    }
    
    
    return 0;
}

