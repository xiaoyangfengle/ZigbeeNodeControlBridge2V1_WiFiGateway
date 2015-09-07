/****************************************************************************
 *
 * MODULE:             Linux Zigbee - JIP daemon
 *
 * COMPONENT:          main program control
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>

#include <libdaemon/daemon.h>

#include "Utils.h"
#include "TunDevice.h"
#include "ZigbeeControlBridge.h"
#include "ZigbeeConstant.h"

#include "CommissioningServer.h"

#include "JIP_Common.h"

/* Supported JIP devices */
#include "JIP_BorderRouter.h"
#include "JIP_ControlBridge.h"
#include "JIP_ColourLamp.h"
#include "JIP_Thermostat.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_MAIN 0

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

static void vQuitSignalHandler (int sig);

static void print_usage_exit(char *argv[]);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

int verbosity = LOG_INFO;               /** Default log level */

int daemonize = 1;                      /** Run as daemon */
int iResetNode = 0;                     /** Reset the node at exit */

#ifndef VERSION
#error Version is not defined!
#else
const char *Version = "1.4 (r" VERSION ")";
#endif


/** Map of supported Zigbee and JIP devices */
tsDeviceIDMap asDeviceIDMap[] = 
{
    { 0x0840, 0x08010010, eControlBridgeInitalise, NULL             },
    { 0x0100, 0x08011175, eColourLampInitalise, NULL                }, /* ZLL mono lamp / HA on/off lamp */
    { 0x0101, 0x08011175, eColourLampInitalise, NULL                }, /* HA dimmable lamp */
    { 0x0102, 0x0801175C, eColourLampInitalise, NULL                }, /* HA dimmable colour lamp */
    { 0x01FF, 0x0801175B, eColourLampInitalise, NULL                }, /* ZHA CCTW lamp */
    
    { 0x0200, 0x0801175C, eColourLampInitalise, NULL                }, /* ZLL dimmable colour lamp */
    { 0x0210, 0x0801175C, eColourLampInitalise, NULL                }, /* ZLL extended colour lamp */
    { 0x0220, 0x0801175B, eColourLampInitalise, NULL                }, /* ZLL colour temperature lamp */
    
    { 0x0301, 0x08014001, eThermostatInitialise, vThermostatUpdate  }, /* HA Thermostat */
    
    { 0x0000, 0x00000000, NULL },
};



/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/** Baud rate to use for communications */
static uint32_t         u32BaudRate         = 1000000;

static uint32_t         u32FactoryNew       = 0;

/** Main loop running flag */
volatile sig_atomic_t   bRunning            = 1;

static tsUtilsThread    sCommissioningThread;



/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


int main(int argc, char *argv[])
{
    pid_t pid;
    char *cpSerialDevice = NULL;
    const char *pcBorderRouterAddress = "fd04:bd3:80e8:10::1";
    char *pcPDMStore = "disabled";
    uint8_t u8EnableWhiteListing = 0;

    /* Disable APS Acks, because the JIP layer retries will take care of this.
     * Also, JIP relies on the default responses arriving,
     * There is no point turning on APS acks together with default responses */
    bZCB_EnableAPSAck = 0;
    
    {
        static struct option long_options[] =
        {
            /* Required arguments */
            {"serial",                  required_argument,  NULL, 's'},

            /* Program options */
            {"help",                    no_argument,        NULL, 'h'},
            {"foreground",              no_argument,        NULL, 'f'},
            {"verbosity",               required_argument,  NULL, 'v'},
            {"baud",                    required_argument,  NULL, 'B'},
            {"interface",               required_argument,  NULL, 'I'},
            {"pdmstore",                required_argument,  NULL, 'P'},
            
            /* Zigbee network options */
            {"mode",                    required_argument,  NULL, 'm'},
            {"factorynew",              no_argument,        NULL, 'n'},
            {"channel",                 required_argument,  NULL, 'c'},
            {"pan",                     required_argument,  NULL, 'p'},
            {"borderrouter",            required_argument,  NULL, '6'},
            {"whitelisting",            no_argument,        NULL, 'w'},
            
            /* Argument to turn APS acks back on */
            {"enable-apsack",           no_argument,        &bZCB_EnableAPSAck, 1},
            
            { NULL, 0, NULL, 0}
        };
        signed char opt;
        int option_index;

        while ((opt = getopt_long(argc, argv, "s:hfv:B:I:P:m:nc:p:6:", long_options, &option_index)) != -1) 
        {
            switch (opt) 
            {
                case 'h':
                    print_usage_exit(argv);
                    break;
                case 'f':
                    daemonize = 0;
                    break;
                case 'v':
                    verbosity = atoi(optarg);
                    break;
                case 'B':
                {
                    char *pcEnd;
                    errno = 0;
                    u32BaudRate = strtoul(optarg, &pcEnd, 0);
                    if (errno)
                    {
                        printf("Baud rate '%s' cannot be converted to 32 bit integer (%s)\n", optarg, strerror(errno));
                        print_usage_exit(argv);
                    }
                    if (*pcEnd != '\0')
                    {
                        printf("Baud rate '%s' contains invalid characters\n", optarg);
                        print_usage_exit(argv);
                    }
                    break;
                }
                case 's':
                    cpSerialDevice = optarg;
                    break;
                    
                case 'm':
                    if (strcmp(optarg, "coordinator") == 0)
                    {
                        eStartMode = E_START_COORDINATOR;
                    }
                    else if (strcmp(optarg, "router") == 0)
                    {
                        eStartMode = E_START_ROUTER;
                    }
                    else if (strcmp(optarg, "touchlink") == 0)
                    {
                        eStartMode = E_START_TOUCHLINK;
                    }
                    else
                    {
                        printf("Unknown mode '%s' specified. Supported modes are 'coordinator', 'router', 'touchlink'\n", optarg);
                        print_usage_exit(argv);
                    }
                    break;
                    
                case 'n':
                    u32FactoryNew = 1;
                    break;
                    
                case 'c':
                {
                    char *pcEnd;
                    uint32_t u32Channel;
                    errno = 0;
                    u32Channel = strtoul(optarg, &pcEnd, 0);
                    if (errno)
                    {
                        printf("Channel '%s' cannot be converted to 32 bit integer (%s)\n", optarg, strerror(errno));
                        print_usage_exit(argv);
                    }
                    if (*pcEnd != '\0')
                    {
                        printf("Channel '%s' contains invalid characters\n", optarg);
                        print_usage_exit(argv);
                    }
                    eChannel = u32Channel;
                    break;
                }
                case 'p':
                {
                    char *pcEnd;
                    errno = 0;
                    u64PanID = strtoull(optarg, &pcEnd, 0);
                    if (errno)
                    {
                        printf("PAN ID '%s' cannot be converted to 64 bit integer (%s)\n", optarg, strerror(errno));
                        print_usage_exit(argv);
                    }
                    if (*pcEnd != '\0')
                    {
                        printf("PAN ID '%s' contains invalid characters\n", optarg);
                        print_usage_exit(argv);
                    }
                    break;
                }
                case '6':
                {
                    int result;
                    struct in6_addr address;
                    
                    result = inet_pton(AF_INET6, optarg, &address);
                    if (result <= 0)
                    {
                        if (result == 0)
                        {
                            printf("Unknown host: %s\n", optarg);
                        }
                        else if (result < 0)
                        {
                            perror("inet_pton failed");
                        }
                        exit(EXIT_FAILURE);
                    }
                    // Valid IPv6 address
                    pcBorderRouterAddress = optarg;
                    break;
                }
                case  'I':
                    pcTD_DevName = optarg;
                    break;
                case 'P':
                    pcPDMStore = optarg;
                    break;
                case 'w':
                    u8EnableWhiteListing = 1;
                    break;
                    
                case 0:
                    break;
                default: /* '?' */
                    print_usage_exit(argv);
            }
        }
    }
    
    /* Log everything into syslog */
    daemon_log_ident = daemon_ident_from_argv0(argv[0]);
    
    if (!cpSerialDevice)
    {
        print_usage_exit(argv);
    }
    
    if (daemonize)
    {
        /* Prepare for return value passing from the initialization procedure of the daemon process */
        if (daemon_retval_init() < 0) {
            daemon_log(LOG_ERR, "Failed to create pipe.");
            return 1;
        }

        /* Do the fork */
        if ((pid = daemon_fork()) < 0)
        {
            /* Exit on error */
            daemon_log(LOG_ERR, "Failed to fork() daemon process.");
            daemon_retval_done();
            return 1;
        } 
        else if (pid)
        { /* The parent */
            int ret;

            /* Wait for 20 seconds for the return value passed from the daemon process */
            if ((ret = daemon_retval_wait(20)) < 0)
            {
                daemon_log(LOG_ERR, "Could not recieve return value from daemon process: %s", strerror(errno));
                return 255;
            }

            if (ret == 0)
            {
                daemon_log(LOG_INFO, "Daemon process started.");
            }
            else
            {
                daemon_log(LOG_ERR, "Daemon returned %i.", ret);
            }
            return ret;
        } 
        else
        { /* The daemon */
            /* Close FDs */
            if (daemon_close_all(-1) < 0)
            {
                daemon_log(LOG_ERR, "Failed to close all file descriptors: %s", strerror(errno));

                /* Send the error condition to the parent process */
                daemon_retval_send(1);
                goto finish;
            }

            daemon_log_use = DAEMON_LOG_SYSLOG;

            /* Send OK to parent process */
            daemon_retval_send(0);

            daemon_log(LOG_INFO, "Daemon started");
        }
    }
    else
    {
        /* Running foreground - set verbosity */
        if ((verbosity != LOG_INFO) && (verbosity != LOG_NOTICE))
        {
            if (verbosity > LOG_DEBUG)
            {
                daemon_set_verbosity(LOG_DEBUG);
            }
            else
            {
                daemon_set_verbosity(verbosity);
            }
        }
        daemon_log_use = DAEMON_LOG_STDOUT;
    }
    
    /* Install signal handlers */
    signal(SIGTERM, vQuitSignalHandler);
    signal(SIGINT, vQuitSignalHandler);
    
    if ((eZCB_Init(cpSerialDevice, u32BaudRate, pcPDMStore) != E_ZCB_OK) || 
        (eTD_Init() != E_TD_OK) ||
        (eJIPCommon_Initialise() != E_JIP_OK) ||
        (eBR_Init(pcBorderRouterAddress) != E_BR_OK)        
    )
    {
        goto finish;
    }
    
    while (bRunning)
    {
        /* Keep attempting to connect to the control bridge */
        if (eZCB_EstablishComms() == E_ZCB_OK)
        {
            // Wait for initial messages from control bridge
            if (sleep(2))
            {
                goto finish;
            }
            
            if (u32FactoryNew)
            {
                eZCB_FactoryNew();
                
                // Wait for initial messages from control bridge after reset
                if (sleep(2))
                {
                    goto finish;
                }
            }
            
            /* Start commissioning server thread */
            if (eUtils_ThreadStart(pvCommissioningServer, &sCommissioningThread, E_THREAD_JOINABLE) != E_UTILS_OK)
            {
                daemon_log(LOG_ERR, "Failed to start commissioning server thread");
            }
            
            if (u8EnableWhiteListing)
            {
                daemon_log(LOG_INFO, "Enabling whitelisting");
                
                if (eZCB_SetWhitelistEnabled(u8EnableWhiteListing) != E_ZCB_OK)
                {
                    daemon_log(LOG_ERR, "Failed to enable whitelisting");
                }
            }
            break;
        }
    }

    while (bRunning)
    {
        tsZcbEvent *psEvent;
        
        switch (eUtils_QueueDequeueTimed(&sZcbEventQueue, 1000, (void**)&psEvent))
        {
            case (E_UTILS_OK):
                if (!psEvent)
                {
                    break;
                }
                
                DBG_vPrintf(DBG_MAIN, "Got event %d\n", psEvent->eEvent);
                
                switch (psEvent->eEvent)
                {
                    case (E_ZCB_EVENT_NETWORK_JOINED):
                    case (E_ZCB_EVENT_NETWORK_FORMED):
                    {
                        tsZCB_Node *psZcbNode = psZCB_FindNodeControlBridge();
                        tsNode *psJIPNode;
                        
                        if (!psZcbNode)
                        {
                            DBG_vPrintf(DBG_MAIN, "Could not find control bridge!\n");
                            break;
                        }
                        
                        psJIPNode = psBR_FindJIPNode(psZcbNode);
                        if (psJIPNode)
                        {
                            DBG_vPrintf(DBG_MAIN, "Node 0x%04X is already in the JIP network\n", psZcbNode->u16ShortAddress);
                            eJIP_UnlockNode(psJIPNode);
                            eUtils_LockUnlock(&psZcbNode->sLock);
                            break;
                        }

                        // Node joined event
                        eBR_NodeJoined(psZcbNode);
                        
                        DBG_vPrintf(DBG_MAIN, "Network started\n");
                        
                        eUtils_LockUnlock(&psZcbNode->sLock);
                        break;
                    }
                    
                    case (E_ZCB_EVENT_DEVICE_ANNOUNCE):
                    {
                        tsZCB_Node *psZcbNode = psZCB_FindNodeShortAddress(psEvent->uData.sDeviceAnnounce.u16ShortAddress);
                        int i;
                        
                        if (!psZcbNode)
                        {
                            DBG_vPrintf(DBG_MAIN, "Could not find new node!\n");
                            break;
                        }
                        
                        if (psZcbNode->u32NumEndpoints > 0)
                        {
                            DBG_vPrintf(DBG_MAIN, "Endpoints of device 0x%04X already known\n", psZcbNode->u16ShortAddress);
                            
                            /* Re-raise event as a device match if we already have the endpoints, 
                             * so we can make sure that a JIP device exists for it.
                             */
                            psEvent->eEvent = E_ZCB_EVENT_DEVICE_MATCH;
                            psEvent->uData.sDeviceMatch.u16ShortAddress = psEvent->uData.sDeviceAnnounce.u16ShortAddress;
                            
                            if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) == E_UTILS_OK)
                            {
                                psEvent = NULL; /* Prevent event being free'd */
                            }
                            eUtils_LockUnlock(&psZcbNode->sLock);
                            break;
                        }
                        
                        DBG_vPrintf(DBG_MAIN, "New device 0x%04X\n", psZcbNode->u16ShortAddress);
                        
                        /* Initiate discovery of device */
                        
                        uint16_t au16Profile[] = { E_ZB_PROFILEID_HA, E_ZB_PROFILEID_ZLL };
                        uint16_t au16Cluster[] = { E_ZB_CLUSTERID_ONOFF, E_ZB_CLUSTERID_THERMOSTAT  };
                        
                        for (i = 0 ; i < (sizeof(au16Profile)/sizeof(uint16_t)); i++)
                        {
                            /* Send match descriptor request */
                            if (eZCB_MatchDescriptorRequest(psZcbNode->u16ShortAddress, au16Profile[i],
                                    sizeof(au16Cluster) / sizeof(uint16_t), au16Cluster, 
                                    0, NULL, NULL) != E_ZCB_OK)
                            {
                                DBG_vPrintf(DBG_MAIN, "Error sending match descriptor request\n");
                            }
                        }
                        eUtils_LockUnlock(&psZcbNode->sLock);
                        
                        /* Slow down match descriptor requests */
                        sleep(1);
                        break;
                    }
                    
                    case (E_ZCB_EVENT_DEVICE_LEFT):
                    {
                        tsZCB_Node *psZcbNode = psZCB_FindNodeIEEEAddress(psEvent->uData.sDeviceLeft.u64IEEEAddress);
                        
                        if (!psZcbNode)
                        {
                            DBG_vPrintf(DBG_MAIN, "Could not find leaving node!\n");
                            break;
                        }
                        
                        if (!psEvent->uData.sDeviceLeft.bRejoin)
                        {
                            // Device is not going to rejoin so remove it immediately.
                            // If it fails to rejoin it will be aged out via the usual mechanism.
                            if (eBR_NodeLeft(psZcbNode) != E_BR_OK)
                            {
                                DBG_vPrintf(DBG_MAIN, "Error removing node from JIP\n");
                            }
                            
                            if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                            {
                                DBG_vPrintf(DBG_MAIN, "Error removing node from ZCB\n");
                            }
                        }
                        break;
                    }
                    
                    case (E_ZCB_EVENT_DEVICE_MATCH):
                    {       
                        int i;
                        teZcbStatus eStatus;
                        tsZCB_Node *psZcbNode = psZCB_FindNodeShortAddress(psEvent->uData.sDeviceMatch.u16ShortAddress);
                        tsNode *psJIPNode;
                        
                        if (!psZcbNode)
                        {
                            DBG_vPrintf(DBG_MAIN, "Could not find new node!\n");
                            break;
                        }
                        
                        psJIPNode = psBR_FindJIPNode(psZcbNode);
                        if (psJIPNode)
                        {
                            DBG_vPrintf(DBG_MAIN, "Node 0x%04X is already in the JIP network\n", psZcbNode->u16ShortAddress);
                            eJIP_UnlockNode(psJIPNode);
                        }
                        else
                        {
                            if (!psZcbNode->u64IEEEAddress)
                            {
                                DBG_vPrintf(DBG_MAIN, "New node 0x%04X, requesting IEEE address\n", psZcbNode->u16ShortAddress);
                                if ((eZCB_NodeDescriptorRequest(psZcbNode) != E_ZCB_OK) || (eZCB_IEEEAddressRequest(psZcbNode) != E_ZCB_OK))
                                {
                                    if (iBR_DeviceTimedOut(psZcbNode))
                                    {
                                        daemon_log(LOG_INFO, "Zigbee node 0x%04X removed from network (no response to IEEE Address).\n", psZcbNode->u16ShortAddress);
                                        if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                                        {
                                            DBG_vPrintf(DBG_MAIN, "Error removing node from ZCB\n");
                                        }
                                    }
                                    else
                                    {
                                        DBG_vPrintf(DBG_MAIN, "Error retrieving IEEE Address of node 0x%04X - requeue\n", psZcbNode->u16ShortAddress);
                                        if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) == E_UTILS_OK)
                                        {
                                            eUtils_LockUnlock(&psZcbNode->sLock);
                                            psEvent = NULL; /* Prevent event being free'd */
                                        }
                                        else
                                        {
                                            /* Failed to re-queue the event - discard node. */
                                            DBG_vPrintf(DBG_MAIN, "Event queue is full - removing node\n");
                                            if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                                            {
                                                DBG_vPrintf(DBG_MAIN, "Error removing node from ZCB\n");
                                            }
                                        }
                                    }
                                    break;
                                }
                            }

                            DBG_vPrintf(DBG_MAIN, "New device, short address 0x%04X, matching requested clusters\n", psZcbNode->u16ShortAddress);
                            
                            for (i = 0; i < psZcbNode->u32NumEndpoints; i++)
                            {
                                if (psZcbNode->pasEndpoints[i].u16ProfileID == 0)
                                {
                                    DBG_vPrintf(DBG_MAIN, "Requesting new endpoint simple descriptor\n");
                                    if (eZCB_SimpleDescriptorRequest(psZcbNode, psZcbNode->pasEndpoints[i].u8Endpoint) != E_ZCB_OK)
                                    {
                                        if (iBR_DeviceTimedOut(psZcbNode))
                                        {
                                            daemon_log(LOG_INFO, "Zigbee node 0x%04X removed from network (No response to simple descriptor request).\n", psZcbNode->u16ShortAddress);
                                            if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                                            {
                                                DBG_vPrintf(DBG_MAIN, "Error removing node from ZCB\n");
                                            }
                                        }
                                        else
                                        {
                                            DBG_vPrintf(DBG_MAIN, "Failed to read endpoint simple descriptor - requeue\n");
                                            if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) == E_UTILS_OK)
                                            {
                                                eUtils_LockUnlock(&psZcbNode->sLock);
                                                psEvent = NULL; /* Prevent event being free'd */
                                            }
                                            else
                                            {
                                                /* Failed to re-queue the event - discard node. */
                                                DBG_vPrintf(DBG_MAIN, "Event queue is full - removing node\n");
                                                if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                                                {
                                                    DBG_vPrintf(DBG_MAIN, "Error removing node from ZCB\n");
                                                }
                                            }
                                        }
                                        /* Get out of the loop and case without adding groups or joining to BR */
                                        goto E_ZCB_EVENT_DEVICE_MATCH_done;
                                    }
                                }
                            }
                            
                            
                            /* Set up bulb groups */
                            eStatus = eZCB_AddGroupMembership(psZcbNode, 0xf00f);
                            if ((eStatus != E_ZCB_OK) && 
                                (eStatus != E_ZCB_DUPLICATE_EXISTS) &&
                                (eStatus != E_ZCB_UNKNOWN_CLUSTER))
                            {
                                if (iBR_DeviceTimedOut(psZcbNode))
                                {
                                    daemon_log(LOG_INFO, "Zigbee node 0x%04X removed from network (No response to add group request).\n", psZcbNode->u16ShortAddress);
                                    if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                                    {
                                        DBG_vPrintf(DBG_MAIN, "Error removing node from ZCB\n");
                                    }
                                }
                                else
                                {
                                    DBG_vPrintf(DBG_MAIN, "Failed to add group - requeue\n");
                                    /* requeue event */
                                    if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) == E_UTILS_OK)
                                    {
                                        eUtils_LockUnlock(&psZcbNode->sLock);
                                        psEvent = NULL; /* Prevent event being free'd */
                                    }
                                    else
                                    {
                                        /* Failed to re-queue the event - discard node. */
                                        DBG_vPrintf(DBG_MAIN, "Event queue is full - removing node\n");
                                        if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                                        {
                                            DBG_vPrintf(DBG_MAIN, "Error removing node from ZCB\n");
                                        }
                                    }
                                }
                                /* Get out of the loop and case without adding groups or joining to BR */
                                goto E_ZCB_EVENT_DEVICE_MATCH_done;
                            }
                        }
                        
                        // Node joined event.
                        if (!psJIPNode)
                        {
                            DBG_vPrintf(DBG_MAIN, "Adding node 0x%04X to border router\n", psZcbNode->u16ShortAddress);
                            eBR_NodeJoined(psZcbNode);
                        }
                        eUtils_LockUnlock(&psZcbNode->sLock);
E_ZCB_EVENT_DEVICE_MATCH_done:
                        break;
                    }
                    
                    case (E_ZCB_EVENT_ATTRIBUTE_REPORT):
                    {       
                        tsZCB_Node *psZcbNode = psZCB_FindNodeShortAddress(psEvent->uData.sAttributeReport.u16ShortAddress);
                        tsNode *psJIPNode;
                        
                        if (!psZcbNode)
                        {
                            DBG_vPrintf(DBG_MAIN, "Could not find node matching attribute report source 0x%04X!\n", psEvent->uData.sAttributeReport.u16ShortAddress);
                            break;
                        }
                        
                        psJIPNode = psBR_FindJIPNode(psZcbNode);
                        if (!psJIPNode)
                        {
                            DBG_vPrintf(DBG_MAIN, "No JIP device for Zigbee node 0x%04X\n", psZcbNode->u16ShortAddress);
                        }
                        else
                        {
                            tsDeviceIDMap *psDeviceIDMap = asDeviceIDMap;
                            while (((psDeviceIDMap->u16ZigbeeDeviceID != 0) &&
                                    (psDeviceIDMap->u32JIPDeviceID != 0) &&
                                    (psDeviceIDMap->prInitaliseRoutine != NULL)))
                            {
                                if (psDeviceIDMap->u16ZigbeeDeviceID == psZcbNode->u16DeviceID)
                                {
                                    DBG_vPrintf(DBG_MAIN, "Found JIP device type 0x%08X for ZB Device type 0x%04X\n",
                                                psDeviceIDMap->u32JIPDeviceID, psDeviceIDMap->u16ZigbeeDeviceID);
                                    if (psDeviceIDMap->prAttributeUpdateRoutine)
                                    {
                                        DBG_vPrintf(DBG_MAIN, "Calling JIP attribute update routine\n");
                                        psDeviceIDMap->prAttributeUpdateRoutine(psZcbNode, psJIPNode, 
                                            psEvent->uData.sAttributeReport.u16ClusterID,
                                            psEvent->uData.sAttributeReport.u16AttributeID,
                                            psEvent->uData.sAttributeReport.eType,
                                            psEvent->uData.sAttributeReport.uData);
                                    }
                                }
                                
                                /* Next device map */
                                psDeviceIDMap++;
                            }
                            eJIP_UnlockNode(psJIPNode);
                        }
                        eUtils_LockUnlock(&psZcbNode->sLock);
                        break;
                    }
                        
                    default:
                        DBG_vPrintf(DBG_MAIN, "Unhandled event code\n");
                        break;
                }
                free(psEvent);
                break;
                
            case (E_UTILS_ERROR_TIMEOUT):
            {
                eBR_CheckDeviceComms();
                break;
            }

            default:
                DBG_vPrintf(DBG_MAIN, "Unknown return\n");
        }
    }
    
    if (iResetNode)
    {
        daemon_log(LOG_INFO, "Resetting Coordinator Module");    
        //eJennicModuleReset();
    }
    
    /* Clean up */
    eUtils_ThreadStop(&sCommissioningThread);
    eBR_Destory();
    eZCB_Finish();
    eTD_Destory();
    
finish:
    if (daemonize)
    {
        daemon_log(LOG_INFO, "Daemon process exiting");  
    }
    else
    {
        daemon_log(LOG_INFO, "Exiting");
    }
    return 0;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/


/** The signal handler just clears the running flag and re-enables itself. */
static void vQuitSignalHandler (int sig)
{
    DBG_vPrintf(DBG_MAIN, "Got signal %d\n", sig);
    
    /* Signal main loop to exit */
    bRunning = 0;
    
    /* Queue a null event to break the main loop */
    eUtils_QueueQueue(&sZcbEventQueue, NULL);
    
    /* Re-enable signal handler */
    signal (sig, vQuitSignalHandler);
    return;
}


static void print_usage_exit(char *argv[])
{
    fprintf(stderr, "zigbee-jip-daemon Version: %s\n", Version);
    fprintf(stderr, "Usage: %s\n", argv[0]);
    fprintf(stderr, "  Arguments:\n");
    fprintf(stderr, "    -s --serial        <serial device>     Serial device for 15.4 module, e.g. /dev/tts/1\n");
    fprintf(stderr, "  Options:\n");
    fprintf(stderr, "    -h --help                              Print this help.\n");
    fprintf(stderr, "    -f --foreground                        Do not detatch daemon process, run in foreground.\n");
    fprintf(stderr, "    -v --verbosity     <verbosity>         Verbosity level. Increases amount of debug information. Default %d.\n", LOG_INFO);
    fprintf(stderr, "    -B --baud          <baud rate>         Baud rate to communicate with border router node at. Default %d\n",     u32BaudRate);
    fprintf(stderr, "    -I --interface     <Interface>         Interface name to create. Default %s.\n",                               pcTD_DevName);
    fprintf(stderr, "    -P --pdmstore      <File>              Location to store PDM data. Default 'disabled'.\n");
    fprintf(stderr, "    -n --factorynew                        Supply this option to factory new the control bridge on bootup.\n");

    fprintf(stderr, "  Zigbee Network options:\n");
    fprintf(stderr, "    -m --mode          <mode>              802.15.4 stack mode (coordinator, router). Default coordinator.\n");
    fprintf(stderr, "    -c --channel       <channel>           802.15.4 channel to run on. Default %d.\n",                             CONFIG_DEFAULT_CHANNEL);
    fprintf(stderr, "    -p --pan           <PAN ID>            802.15.4 extended Pan ID to use. Default 0x%llx.\n",                    CONFIG_DEFAULT_PANID);
    
    fprintf(stderr, "  JIP Network options:\n");
    fprintf(stderr, "    -6 --borderrouter  <IPv6 Address>      IPv6 Address to use for the virtual border router. Default fd04:bd3:80e8:10::1\n");
    exit(EXIT_FAILURE);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
