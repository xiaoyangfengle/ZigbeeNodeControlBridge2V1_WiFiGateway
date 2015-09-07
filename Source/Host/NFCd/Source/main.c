/****************************************************************************
 *
 * MODULE:             Linux NFC daemon
 *
 * COMPONENT:          main program control
 *
 * REVISION:           $Revision: 56189 $
 *
 * DATED:              $Date: 2013-08-22 16:18:56 +0100 (Thu, 22 Aug 2013) $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <arpa/inet.h>

#include <libdaemon/daemon.h>

#include "NTAG.h"
#include "NDEF.h"
#include "CommissioningClient.h"


//#define MAIN_DEBUG

#ifdef MAIN_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* MAIN_DEBUG */




int verbosity = LOG_DEBUG;       /** Default log level */

int daemonize = 1;              /** Run as daemon */

#ifndef VERSION
#error Version is not defined!
#else
const char *Version = "0.2 (r" VERSION ")";
#endif

#define SAK_ISO14443_4_COMPLIANT 0x20


/** Main loop running flag */
volatile sig_atomic_t bRunning = 1;

static void vHandleTag(void);
static void vHandleIncomingNDEF(tsNdefData *psOutgoing, uint8_t u8Command, uint8_t u8Length, uint8_t *pu8Data);
static void vTagReset(const char *pcPower_GPIO);


/** The signal handler just clears the running flag and re-enables itself. */
static void vQuitSignalHandler (int sig)
{
    //DEBUG_PRINTF("Got signal %d\n", sig);
    bRunning = 0;

    signal (sig, vQuitSignalHandler);
    return;
}


static void print_usage_exit(char *argv[])
{
    fprintf(stderr, "NFCd Version: %s\n", Version);
    fprintf(stderr, "Usage: %s\n", argv[0]);
    fprintf(stderr, "  Arguments:\n");
    fprintf(stderr, "  Options:\n");
    fprintf(stderr, "    -h --help                              Print this help.\n");
    fprintf(stderr, "    -f --foreground                        Do not detatch daemon process, run in foreground.\n");
    fprintf(stderr, "    -v --verbosity     <verbosity>         Verbosity level. Increses amount of debug information. Default %d.\n",  LOG_INFO);

    fprintf(stderr, "       --gpio-fd       <path>              GPIO sys fs file connected to NTAG FD line.\n");
    fprintf(stderr, "       --i2c-bus       <path>              I2C device file for bus with NTAG connected.\n");
    fprintf(stderr, "       --i2c-address   <address>           I2C Address of NTAG.\n");

    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	pid_t pid;
    const char *pcFD_GPIO   = NULL;
    const char *pcI2Cbus    = NULL;
    uint8_t     u8I2CAddress = 0;
    const char *pcPWR_GPIO  = NULL;
    
    {
        static struct option long_options[] =
        {
            /* Program options */
            {"help",                    no_argument,        NULL, 'h'},
            {"foreground",              no_argument,        NULL, 'f'},
            {"verbosity",               required_argument,  NULL, 'v'},
            
            {"gpio-fd",                 required_argument,  NULL, 0},
            {"i2c-bus",                 required_argument,  NULL, 1},
            {"i2c-address",             required_argument,  NULL, 2},
            {"gpio-pwr",                required_argument,  NULL, 3},

            { NULL, 0, NULL, 0}
        };
        signed char opt;
        int option_index;

        while ((opt = getopt_long(argc, argv, "hfv:", long_options, &option_index)) != -1) 
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
                
                case 0:
                    pcFD_GPIO = optarg;
                    break;
                    
                case 1:
                    pcI2Cbus = optarg;
                    break;
                    
                case 2:
                {
                    char *pcEnd;
                    errno = 0;
                    uint32_t u32I2CAddress = strtoul(optarg, &pcEnd, 0);
                    if (errno)
                    {
                        fprintf(stderr, "I2C Address '%s' cannot be converted to 32 bit integer (%s)\n", optarg, strerror(errno));
                        print_usage_exit(argv);
                    }
                    if (*pcEnd != '\0')
                    {
                        fprintf(stderr, "I2C Address '%s' contains invalid characters\n", optarg);
                        print_usage_exit(argv);
                    }
                    if (u32I2CAddress > 0x7F)
                    {
                        fprintf(stderr, "I2C Address '%s' outside allowable range (0x01 - 0x7F)\n", optarg);
                        print_usage_exit(argv);
                    }
                    u8I2CAddress = u32I2CAddress;
                    break;
                }
                
                case 3:
                    pcPWR_GPIO = optarg;
                    break;
                    
                default: /* '?' */
                    print_usage_exit(argv);
            }
        }
    }
    
    if (!pcFD_GPIO)
    {
        fprintf(stderr, "GPIO for field detect pin must be specified\n");
        print_usage_exit(argv);
    }
    
    if (!pcI2Cbus)
    {
        fprintf(stderr, "I2C bus must be specified\n");
        print_usage_exit(argv);
    }
    
    if (!u8I2CAddress)
    {
        fprintf(stderr, "I2C address must be specified\n");
        print_usage_exit(argv);
    }
    
    /* Log everything into syslog */
    daemon_log_ident = daemon_ident_from_argv0(argv[0]);
    
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
        if (verbosity > LOG_WARNING)
        {
            daemon_set_verbosity(verbosity);
        }
    }

    /* Install signal handlers */
    signal(SIGTERM, vQuitSignalHandler);
    signal(SIGINT, vQuitSignalHandler);

    if (eNtagSetup(pcPWR_GPIO, pcFD_GPIO, pcI2Cbus, u8I2CAddress) == E_NFC_OK)
    {
        if (eNdefSetup() == E_NFC_OK)
        {
            while (bRunning != 0)
            {
                if (eNtagWait(30000) == E_NFC_READER_PRESENT)
                {
                    /* Tag detected */
                    while ((eNtagPresent() == E_NFC_READER_PRESENT) && bRunning)
                    {
                        sleep(1);
                        
                        teNfcStatus eStatus = eNtagRfUnlocked();
                        
                        switch (eStatus)
                        {
                            case (E_NFC_OK):
                                vHandleTag();
                                break;
                                
                            case (E_NFC_RF_LOCKED):
                                DEBUG_PRINTF("RF Locked\n");
                                break;
                                
                            default:
                                break;
                        }
                        //usleep(100000);
                        //sleep(1);
                    }
                }
                else
                {
                    /* Blank the tag */
                    tsNdefData sData;
                    eNdefInitData(&sData, 0, 0);
                    eNdefWriteData(&sData);
                }
            }
        }
    }
    else
    {
        DEBUG_PRINTF("Failed to init NTAG\n");
    }
    
finish:
    if (daemonize)
    {
        daemon_log(LOG_INFO, "Daemon process exiting");  
    }
    return 0;
}


static void vHandleTag(void)
{
    tsNdefData sNdefIncomingData;
    tsNdefData sNdefOutgoingData;

    if (eNdefReadData(&sNdefIncomingData) == E_NFC_OK)
    {
        uint8_t *pu8Data = &sNdefIncomingData.uPayload.au8Bytes[0];
        int i;
        
        DEBUG_PRINTF("Number of entries: %d\n", sNdefIncomingData.u8NumEntries);
        for (i = 0; i < sNdefIncomingData.u8NumEntries;)
        {
            uint8_t u8RecordLength = 4;
            
            DEBUG_PRINTF("Entry #%d: Type 0x%02X\n", i, pu8Data[0]);
            
            if (pu8Data[0] == 0xFF)
            {
                vHandleIncomingNDEF(&sNdefOutgoingData, pu8Data[1], pu8Data[2], &pu8Data[4]);
                u8RecordLength = 4 + (pu8Data[2] * 4);
            }
            else
            {
                vHandleIncomingNDEF(&sNdefOutgoingData, pu8Data[0], 3, &pu8Data[1]);
            }
            DEBUG_PRINTF("Moving on %d bytes\n", u8RecordLength);
            pu8Data += u8RecordLength;
            i += u8RecordLength / 4;
        }
    }
}


static void vHandleIncomingNDEF(tsNdefData *psOutgoing, uint8_t u8Command, uint8_t u8Length, uint8_t *pu8Data)
{
    static uint8_t u8Seq = 0;
    
    switch (u8Command)
    {
        case (E_NDEF_NETWORK_GET_JOIN):
        {
            tsCommZigbee sZigbeeData;
            daemon_log(LOG_DEBUG, "Get join parameters");
            
            if (eCommissionZigbeeInsecure(&sZigbeeData) == E_COMM_OK)
            {
                tsNdefData sData;
                tsNdefNetworkJoin sNdefNetworkJoin;

                DEBUG_PRINTF("Get join parameters result:\n");
                DEBUG_PRINTF("Channel: %d\n", sZigbeeData.u8Channel);
                DEBUG_PRINTF("Short PAN: 0x%04x\n", sZigbeeData.u16PanId);
                DEBUG_PRINTF("Extended PAN: 0x%llx\n", (unsigned long long int)sZigbeeData.u64PanId);
                DEBUG_PRINTF("\n");
                
                memset(&sNdefNetworkJoin, 0, sizeof(tsNdefNetworkJoin));
                sNdefNetworkJoin.u8Channel  = sZigbeeData.u8Channel;
                sNdefNetworkJoin.u16PanId   = htons(sZigbeeData.u16PanId);
                sNdefNetworkJoin.u64PanId   = htobe64(sZigbeeData.u64PanId);

                daemon_log(LOG_DEBUG, "Write tag with NETWORK_JOIN");
                eNdefInitData(&sData, u8Seq++, 0);
                eNdefAddBytes(&sData, E_NDEF_NETWORK_JOIN, (uint8_t*)&sNdefNetworkJoin, sizeof(tsNdefNetworkJoin));
                eNdefWriteData(&sData);
            }
            break;
        }

#if 0
        /* FOR DEBUG - IoT gateway does not usually respond to this */
        case (E_NDEF_NETWORK_GET_LINK_INFO):
        {
            int i;
            tsNdefData sData;
            tsNdefNetworkLinkInfo sLinkInfo;
            DEBUG_PRINTF("Get Link info\n");
            
            sLinkInfo.u8Version = 0;
            sLinkInfo.u8Type = 0;
            sLinkInfo.u16Profile = htons(0x0104);
            sLinkInfo.u64MacAddress = htobe64(0x00158d0000123456);
            for (i = 0; i < 16; i++)
            {
                sLinkInfo.au8LinkKey[i] = rand();
            }
            
            daemon_log(LOG_DEBUG, "Write tag with LINK_INFO");
            eNdefInitData(&sData, u8Seq++, 0);
            eNdefAddBytes(&sData, E_NDEF_NETWORK_LINK_INFO, (uint8_t*)&sLinkInfo, sizeof(tsNdefNetworkLinkInfo));
            eNdefWriteData(&sData);
            break;
        }
#endif
        
        case (E_NDEF_NETWORK_LINK_INFO):
        {
            int i;
            tsNdefData sData;
            tsCommZigbee sZigbeeData;
            tsNdefNetworkLinkInfo *psLinkInfo = (tsNdefNetworkLinkInfo *)pu8Data;
            
            daemon_log(LOG_DEBUG, "Commission device MAC Address: 0x%016llx", (unsigned long long int)be64toh(psLinkInfo->u64MacAddress));
            
            memset(&sZigbeeData, 0, sizeof(tsCommZigbee));
            sZigbeeData.u16Profile = ntohs(psLinkInfo->u16Profile);
            for (i = 0; i < 8; i++)
            {
                sZigbeeData.au8MacAddress[i] = psLinkInfo->u64MacAddress >> (i * 8);
            }
            memcpy(sZigbeeData.au8LinkKey, psLinkInfo->au8LinkKey, 16);
            
            if (eCommissionZigbee(&sZigbeeData) == E_COMM_OK)
            {
                int i;
                tsNdefNetworkJoin sNdefNetworkJoin;
                
                DEBUG_PRINTF("Commission device result:\n");
                DEBUG_PRINTF("Channel: %d\n", sZigbeeData.u8Channel);
                DEBUG_PRINTF("Short PAN: 0x%04x\n", sZigbeeData.u16PanId);
                DEBUG_PRINTF("Extended PAN: 0x%llx\n", (unsigned long long int)sZigbeeData.u64PanId);
                DEBUG_PRINTF("Network key: ");
                for (i = 0; i < 16; i++)
                {
                    DEBUG_PRINTF("0x%02X ", sZigbeeData.au8NetworkKey[i]);
                }
                DEBUG_PRINTF("\n");
                DEBUG_PRINTF("MIC: 0x%02X%02X%02X%02X\n", sZigbeeData.au8MIC[0], sZigbeeData.au8MIC[1], sZigbeeData.au8MIC[2], sZigbeeData.au8MIC[3]); 
                
                DEBUG_PRINTF("Trust center address: 0x%016llx\n", (unsigned long long int)sZigbeeData.u64TrustCenterAddress);
                DEBUG_PRINTF("Key sequence number: %02d\n", sZigbeeData.u8KeySequenceNumber);
                DEBUG_PRINTF("\n");

                /* Copy data in big endian form */
                memset(&sNdefNetworkJoin, 0, sizeof(tsNdefNetworkJoin));
                sNdefNetworkJoin.u8Channel                  = sZigbeeData.u8Channel;
                sNdefNetworkJoin.u8ActiveKeySequenceNumber  = sZigbeeData.u8KeySequenceNumber;
                sNdefNetworkJoin.u16PanId                   = htons(sZigbeeData.u16PanId);
                sNdefNetworkJoin.u64PanId                   = htobe64(sZigbeeData.u64PanId);
                memcpy(sNdefNetworkJoin.au8NetworkKey, sZigbeeData.au8NetworkKey, 16);
                memcpy(sNdefNetworkJoin.au8MIC, sZigbeeData.au8MIC, 4);
                sNdefNetworkJoin.u64TrustCenterAddress      = htobe64(sZigbeeData.u64TrustCenterAddress);

                daemon_log(LOG_DEBUG, "Write tag with NETWORK_JOIN_SECURE");
                eNdefInitData(&sData, u8Seq++, 0);
                eNdefAddBytes(&sData, E_NDEF_NETWORK_JOIN_SECURE, (uint8_t*)&sNdefNetworkJoin, sizeof(tsNdefNetworkJoin));
                eNdefWriteData(&sData);
            }
            break;
        }
        
        default:
            DEBUG_PRINTF("Unhandled message type (0x%02X)\n", u8Command);
            break;
    }
}
