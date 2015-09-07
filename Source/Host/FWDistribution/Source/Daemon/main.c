/****************************************************************************
 *
 * MODULE:             Firmware Distribution Daemon
 *
 * COMPONENT:          Main program control
 *
 * REVISION:           $Revision: 51417 $
 *
 * DATED:              $Date: 2013-01-10 09:44:53 +0000 (Thu, 10 Jan 2013) $
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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <libdaemon/daemon.h>

#include "Firmwares.h"
#include "FWMonitor.h"
#include "OND.h"
#include "Status.h"

#include "../Common/FWDistribution_IPC.h"

#ifndef VERSION
#error Version is not defined!
#else
const char *Version = VERSION;
#endif

int verbosity = LOG_INFO;       /** Default log level */
int daemonize = 1;              /** Run as daemon */


const char *pcBindAddress = "::";
const char *pcPortNumber = "1874";

void print_usage_exit(char *argv[])
{
    fprintf(stderr, "Firmware Distribution Daemon Version: %s\n", Version);
    fprintf(stderr, "Usage: %s\n", argv[0]);
    fprintf(stderr, "  Options:\n");
    fprintf(stderr, "    -h --help                              Print this help.\n");
    fprintf(stderr, "    -F --foreground                        Do not detatch daemon process, run in foreground.\n");
    fprintf(stderr, "    -v --verbosity         <verbosity>     Verbosity level. Increses amount of debug information. Default 0.\n");
    fprintf(stderr, "    -b --bind              <bind addr>     Local address to bind server to. Default \"%s\".\n", pcBindAddress);
    fprintf(stderr, "    -p --port              <port>          Local / Remote UDP port number to use. Default %s.\n", pcPortNumber);
    
    fprintf(stderr, "    -f --firmware          <firmware>      Load firmware binary file.\n");
    fprintf(stderr, "    -d --directory         <firmware dir>  Directory to monitor for firmware files.\n");
    exit(EXIT_SUCCESS);
}


int main (int argc, char **argv)
{
    tsOND sOND;
    pid_t pid;
    
    /* Files to load */
    int iFirmwareFiles = 0;
    char **ppcFirmwareFiles = NULL;
    
    /* Directories to monitor */
    int iFirmwareDirs = 0;
    char **ppcFirmwareDirs = NULL;
    
    {
        static struct option long_options[] =
        {
            /* Program options */
            {"help",                    no_argument,        NULL, 'h'},
            {"foreground",              no_argument,        NULL, 'F'},
            {"verbosity",               required_argument,  NULL, 'v'},
            {"bind",                    required_argument,  NULL, 'b'},
            {"port",                    required_argument,  NULL, 'p'},
            {"firmware",                required_argument,  NULL, 'f'},
            {"directory",               required_argument,  NULL, 'd'},
            
            { NULL, 0, NULL, 0}
        };
        signed char opt;
        int option_index;
        
        while ((opt = getopt_long(argc, argv, "hVFv:f:d:b:p:", long_options, &option_index)) != -1) 
        {
            switch (opt) 
            {
                case 'h':
                case 'V':
                    print_usage_exit(argv);
                    break;
            
                case 'F':
                    daemonize = 0;
                    break;
   
                case 'v':
                    verbosity = atoi(optarg);
                    break;

                case 'f':
                {
                    char **ppcFirmwareFilesNew;
                    iFirmwareFiles++;
                    ppcFirmwareFilesNew = realloc(ppcFirmwareFiles, sizeof(char *) * iFirmwareFiles);
                    if (!ppcFirmwareFilesNew)
                    {
                        fprintf(stderr, "Out of memory\n");
                        return -1;
                    }
                    ppcFirmwareFiles = ppcFirmwareFilesNew;
                    ppcFirmwareFiles[iFirmwareFiles-1] = strdup(optarg);
                    break;
                }

                case 'd':
                {
                    char **ppcFirmwareDirsNew;
                    iFirmwareDirs++;
                    ppcFirmwareDirsNew = realloc(ppcFirmwareDirs, sizeof(char *) * iFirmwareDirs);
                    if (!ppcFirmwareDirsNew)
                    {
                        fprintf(stderr, "Out of memory\n");
                        return -1;
                    }
                    ppcFirmwareDirs = ppcFirmwareDirsNew;
                    ppcFirmwareDirs[iFirmwareDirs-1] = strdup(optarg);
                    break;
                }
                
                case  'b':
                    pcBindAddress = optarg;
                    break;
                
                case  'p':
                    pcPortNumber = optarg;
                    break;
                    
                default: /* '?' */
                    print_usage_exit(argv);
            }
        }
    }
    
    daemon_set_verbosity(verbosity);
    
    daemon_pid_file_ident = daemon_log_ident = daemon_ident_from_argv0(argv[0]);

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
    
    StatusInit();
    eFirmware_Init();
    FWMonitor_Init();
    
    {
        int i;
        for (i = 0; i < iFirmwareFiles; i++)
        {
            daemon_log(LOG_INFO, "Loading Firmware %s\n", ppcFirmwareFiles[i]);
            eFirmware_Open(ppcFirmwareFiles[i]);
            free(ppcFirmwareFiles[i]);
        }
        free(ppcFirmwareFiles);
        
        for (i = 0; i < iFirmwareDirs; i++)
        {
            daemon_log(LOG_INFO, "Adding monitor on directory %s\n", ppcFirmwareDirs[i]);
            FWMonitor_Start(ppcFirmwareDirs[i]);
            free(ppcFirmwareDirs[i]);
        }
        free(ppcFirmwareDirs);
    }
    
    
    if (eONDInitialise(&sOND, pcBindAddress, pcPortNumber) != OND_STATUS_OK)
    {
        daemon_log(LOG_CRIT, "Error starting up networking\n");
        goto finish;
    }

    /* IPC Server */
    { 
        int s, s2, len;
        socklen_t t;
        struct sockaddr_un local, remote;
        
        if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        {
            daemon_log(LOG_CRIT, "Could not create IPC socket (%s)\n", strerror(errno));
            goto finish;
        }

        local.sun_family = AF_UNIX;
        strcpy(local.sun_path, FWDistributionIPCSockPath);
        unlink(local.sun_path);
        len = strlen(local.sun_path) + sizeof(local.sun_family);
        
        if (bind(s, (struct sockaddr *)&local, len) == -1)
        {
            daemon_log(LOG_CRIT, "Could not bind IPC socket (%s)\n", strerror(errno));
            goto finish;
        }

        if (listen(s, 5) == -1)
        {
            daemon_log(LOG_CRIT, "Could not listen on IPC socket (%s)\n", strerror(errno));
            goto finish;
        }
        
        while (1)
        {
            int done, n;

            t = sizeof(remote);
            if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1)
            {
                daemon_log(LOG_ERR, "Could not accept IPC socket (%s)\n", strerror(errno));
                continue;
            }

            done = 0;
            do 
            {
                tsFWDistributionIPCHeader sHeader;
                n = recv(s2, &sHeader, sizeof(tsFWDistributionIPCHeader), 0);
                if (n <= 0) 
                {
                    if (n < 0) 
                    {
                        daemon_log(LOG_ERR, "Error on IPC socket recv (%s)\n", strerror(errno));
                    }
                    done = 1;
                }
                else
                {
                    switch (sHeader.eType)
                    {
                        case (IPC_GET_AVAILABLE_FIRMWARES):
                            if (eFirmware_Remote_Send(s2) != FW_STATUS_OK)
                            {
                                done = 1;
                            }
                            break;
                            
                        case (IPC_START_DOWNLOAD):
                        {
                            tsFWDistributionIPCStartDownload sDownload;
                            teFlags eFlags = 0;
                            
                            n = recv(s2, &sDownload, sizeof(tsFWDistributionIPCStartDownload), 0);
                            if (n <= 0) 
                            {
                                if (n < 0) 
                                {
                                    daemon_log(LOG_ERR, "Error on IPC socket recv (%s)\n", strerror(errno));
                                }
                                done = 1;
                            }
                            else
                            {
                                if (sDownload.eFlags & E_DOWNLOAD_IPC_FLAG_INFORM)
                                {
                                    eFlags |= E_DOWNLOAD_FLAG_INITIATE_ONLY;
                                }
                                if (sDownload.eFlags & E_DOWNLOAD_IPC_FLAG_RESET)
                                {
                                    eFlags |= E_DOWNLOAD_FLAG_IMMEDIATE_RESET;
                                }
                                daemon_log(LOG_INFO, "Starting download\n");
                                eONDStartDownload(&sOND, sDownload.sAddress, sDownload.u32DeviceID, sDownload.u16ChipType, sDownload.u16Revision, sDownload.u16BlockInterval, eFlags);
                            }
                            
                            break;
                        }
                        
                        case (IPC_CANCEL_DOWNLOAD):
                        {
                            tsFWDistributionIPCCancelDownload sCancel;

                            n = recv(s2, &sCancel, sizeof(tsFWDistributionIPCCancelDownload), 0);
                            if (n <= 0) 
                            {
                                if (n < 0) 
                                {
                                    daemon_log(LOG_ERR, "Error on IPC socket recv (%s)\n", strerror(errno));
                                }
                                done = 1;
                            }
                            else
                            {
                                daemon_log(LOG_INFO, "Cancelling download\n");
                                eONDCancelDownload(&sOND, sCancel.sAddress, sCancel.u32DeviceID, sCancel.u16ChipType, sCancel.u16Revision);
                            }
                            
                            break;
                        }
                        
                        case (IPC_RESET):
                        {
                            tsFWDistributionIPCReset sReset;

                            n = recv(s2, &sReset, sizeof(tsFWDistributionIPCReset), 0);
                            if (n <= 0) 
                            {
                                if (n < 0) 
                                {
                                    daemon_log(LOG_ERR, "Error on IPC socket recv (%s)\n", strerror(errno));
                                }
                                done = 1;
                            }
                            else
                            {
                                daemon_log(LOG_INFO, "Requesting reset\n");
                                eONDRequestReset(&sOND, sReset.sAddress, sReset.u32DeviceID, 
                                                 sReset.u16Timeout, sReset.u16DepthInfluence,
                                                 sReset.u16RepeatCount, sReset.u16RepeatTime);
                            }
                            break;
                        }
                        
                        case (IPC_GET_STATUS):
                            if (eStatus_Remote_Send(s2) != STATUS_OK)
                            {
                                done = 1;
                            }
                            break;
                        
                        default:
                            daemon_log(LOG_ERR, "IPC Request unknown (%d)", sHeader.eType);
                            done = 1;
                            break;
                    }
                }
            } while (!done);

            close(s2);
        }
    }
        
finish:
    if (daemonize)
    {
        daemon_log(LOG_INFO, "Daemon process exiting");  
    }
    return 0;
}

