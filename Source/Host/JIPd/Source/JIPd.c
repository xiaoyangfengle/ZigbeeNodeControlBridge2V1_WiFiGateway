/****************************************************************************
 *
 * MODULE:             JIPv4 compatability daemon
 *
 * COMPONENT:          main program control
 *
 * REVISION:           $Revision: 37697 $
 *
 * DATED:              $Date: 2011-12-06 14:41:22 +0000 (Tue, 06 Dec 2011) $
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
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <errno.h>

#include <libdaemon/daemon.h>

#include <IPv4_UDP.h>
#include <IPv4_TCP.h>

#ifdef USE_ZEROCONF
#include "Zeroconf.h"
#endif /* USE_ZEROCONF */

int iPort = 1873;

int verbosity = LOG_INFO;       /** Default log level */

int daemonize = 1;              /** Run as daemon */

static char *pcListen_address = "0.0.0.0";

static pthread_t sIPv4UDPThreadInfo;


#ifndef VERSION
#error Version is not defined!
#else
const char *Version = "0.7 (r" VERSION ")";
#endif




void print_usage_exit(char *argv[])
{
    fprintf(stderr, "JIPd version %s\n", Version);
    fprintf(stderr, "Usage\n");
    fprintf(stderr, "  Options:\n");
    fprintf(stderr, "    -v <verbosity>   Verbosity level. Increses amount of debug information. Default 0.\n");
    fprintf(stderr, "    -f               Do not detatch daemon process, run in foreground.\n");
    fprintf(stderr, "    -4 <IP Address>  IPv4 Address to listen for incoming connections on. Default 0.0.0.0 (all)\n");
    fprintf(stderr, "    -p <port>        Port number to listen for incoming connections on. Default %d\n.", iPort);
    exit(EXIT_FAILURE);
}


void *IPv4_UDP_Thread(void *args)
{
    IPv4_UDP(pcListen_address, iPort);
    return NULL;
}


int main(int argc, char *argv[])
{
    pid_t pid;
    {
        signed char opt;
        while ((opt = getopt(argc, argv, "hVv:f4:p:")) != -1) 
        {
            switch (opt) 
            {
                case 'h':
                case 'V':
                    print_usage_exit(argv);
                    break;
                case 'v':
                    verbosity = atoi(optarg);
                    break;
                case 'f':
                    daemonize = 0;
                    break;
                case '4':
                    pcListen_address  = optarg;
                    break;
                case 'p':
                {
                    char *pcEnd;
                    errno = 0;
                    iPort = strtoul(optarg, &pcEnd, 0);
                    if (errno)
                    {
                        printf("Port '%s' cannot be converted to 32 bit integer (%s)\n", optarg, strerror(errno));
                        print_usage_exit(argv);
                    }
                    if (*pcEnd != '\0')
                    {
                        printf("Port '%s' contains invalid characters\n", optarg);
                        print_usage_exit(argv);
                    }
                    break;
                }

                default: /* '?' */
                    print_usage_exit(argv);
            }
        }
    }
    
    /* Log everything into syslog */
    daemon_log_ident = daemon_ident_from_argv0(argv[0]);
    
    daemon_set_verbosity(verbosity);
    
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

    {
        pthread_attr_t tattr;
        
        pthread_attr_init(&tattr);
        /* set the thread detach state */
        if (pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
        {
            exit(EXIT_FAILURE);
        }

        if (pthread_create(&sIPv4UDPThreadInfo, &tattr, IPv4_UDP_Thread, NULL) != 0)
        {
            daemon_log(LOG_ERR, "Error starting UDP thread (%s)", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

#ifdef USE_ZEROCONF
    ZC_RegisterServices("JIPv4 Gateway");
#endif /* USE_ZEROCONF */
    
#if 1
    IPv4_TCP(pcListen_address, iPort);
#else
    while (1)
    {
        sleep(10);
    }
#endif

finish:
    daemon_log(LOG_INFO, "daemon Exiting");
    return 0;
}

