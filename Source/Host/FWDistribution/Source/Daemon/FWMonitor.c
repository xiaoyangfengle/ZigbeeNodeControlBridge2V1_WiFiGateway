/****************************************************************************
 *
 * MODULE:             Firmware Distribution Daemon
 *
 * COMPONENT:          INotify based firmware monitor
 *
 * REVISION:           $Revision: 43428 $
 *
 * DATED:              $Date: 2012-06-18 15:28:13 +0100 (Mon, 18 Jun 2012) $
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
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/inotify.h>

#include <libdaemon/daemon.h>

#include "FWMonitor.h"
#include "Threads.h"
#include "Firmwares.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

/** Structure to represent a directory monitor */
typedef struct _tsFWMonitor
{
    volatile enum {
        E_MONITOR_STATE_STOPPED,                /** < Thread not started yet */
        E_MONITOR_STATE_RUNNING,                /** < Thread runnning normally */
        E_MONITOR_STATE_QUITING,                /** < Thread should quit */
    } eState;                                   /** < Control method for monitor thread */
    char                    *pu8Path;           /** < Path being monitored */
    tsThread                sThread;            /** < Monitor Thread data */
    struct _tsFWMonitor     *psNext;            /** < Pointer to next in list */
} tsFWMonitor;

/** Head of linked list */
static tsFWMonitor *psFWMonitor_ListHead = NULL;

/** Thread protection for monitor linked list */
static tsLock sMonitorListLock;

/** Monitor thread function */
static void *MonitorThread(void *psThreadInfoVoid);

/** Function to perform initial scan of directory */
static teFWMonitor_Status FWMonitor_Scan(tsFWMonitor *psFWMonitor);

/** Function to watch for changes to the directory using INotify */
static teFWMonitor_Status FWMonitor_INotify(tsFWMonitor *psFWMonitor);


teFWMonitor_Status FWMonitor_Init(void)
{
    psFWMonitor_ListHead = NULL;
    
    if (eLockCreate(&sMonitorListLock) != E_LOCK_OK)
    {
        daemon_log(LOG_DEBUG, "FWMonitor: Failed to initialise lock");
        return FWM_STATUS_ERROR;
    }

    return FWM_STATUS_OK;
}


teFWMonitor_Status FWMonitor_Start(const char *pu8Path)
{
    tsFWMonitor *psFWMonitor;
    
    /* First validate that we've been asked to monitor a directory */
    {
        struct stat sStat;
        if (stat(pu8Path, &sStat) < 0)
        {
            daemon_log(LOG_ERR, "FWMonitor: Could not stat '%s' (%s)", pu8Path, strerror(errno));
            return FWM_STATUS_ERROR;
        }
        if (!S_ISDIR(sStat.st_mode))
        {
            daemon_log(LOG_ERR, "FWMonitor: '%s' is not a directory", pu8Path);
            return FWM_STATUS_ERROR;
        }
    }
    
    /* Ok, looks good, now set up to monitor it */
    
    psFWMonitor = malloc(sizeof(tsFWMonitor));
    if (!psFWMonitor)
    {
        daemon_log(LOG_CRIT, "FWMonitor: Out of memory");
        return FWM_STATUS_ERROR;
    }
    
    memset(psFWMonitor, 0, sizeof(tsFWMonitor));

    psFWMonitor->eState = E_MONITOR_STATE_STOPPED;
    psFWMonitor->pu8Path = strdup(pu8Path);
    psFWMonitor->psNext = NULL;
    
    psFWMonitor->sThread.pvThreadData = psFWMonitor;

    /* Lock the list before starting the thread */
    eLockLock(&sMonitorListLock);
    
    if (eThreadStart(MonitorThread, &psFWMonitor->sThread) != E_THREAD_OK)
    {
        daemon_log(LOG_CRIT, "FWMonitor: Failed to start monitor thread");
        eLockUnlock(&sMonitorListLock);
        return FWM_STATUS_ERROR;
    }
    
    /* Add in to linked list of running monitors */
    
    if (psFWMonitor_ListHead == NULL)
    {
        /* First in list */
        psFWMonitor_ListHead = psFWMonitor;
    }
    else
    {
        tsFWMonitor *psListPosition = psFWMonitor_ListHead;
        while (psListPosition->psNext)
        {
            psListPosition = psListPosition->psNext;
        }
        psListPosition->psNext = psFWMonitor;
    }

    eLockUnlock(&sMonitorListLock);
    
    return FWM_STATUS_OK;
}


teFWMonitor_Status FWMonitor_Stop(const char *pu8Path)
{
    tsFWMonitor *psListPosition;
    teFWMonitor_Status eStatus = FWM_STATUS_ERROR;
    
    eLockLock(&sMonitorListLock);
    
    psListPosition = psFWMonitor_ListHead;
    
    while (psListPosition)
    {
        if (strcmp(pu8Path, psListPosition->pu8Path) == 0)
        {
            /* Found the one to stop */
            /** \todo Make the thread actually stop when this is called */
            psListPosition->eState = E_MONITOR_STATE_QUITING;
            daemon_log(LOG_INFO, "FWMonitor: Stopping monitoring directory %s", pu8Path);
            eStatus = FWM_STATUS_OK;
            break;
        }
        psListPosition = psListPosition->psNext;
    }

    eLockUnlock(&sMonitorListLock);

    return eStatus;
}


static void *MonitorThread(void *psThreadInfoVoid)
{
    tsThread *psThreadInfo = (tsThread *)psThreadInfoVoid;
    tsFWMonitor *psFWMonitor = (tsFWMonitor *)psThreadInfo->pvThreadData;
    
    daemon_log(LOG_INFO, "FWMonitor: Starting to monitor directory %s", psFWMonitor->pu8Path);
    
    /* Initial scan of directory for already present firmwares */
    if (FWMonitor_Scan(psFWMonitor) != FWM_STATUS_OK)
    {
        goto thread_exit;
    }
    
    psFWMonitor->eState = E_MONITOR_STATE_RUNNING;
    
    /* Now run INotify on the directory listening for new files */
    if (FWMonitor_INotify(psFWMonitor) != FWM_STATUS_OK)
    {
        goto thread_exit;
    }

thread_exit:
    daemon_log(LOG_INFO, "FWMonitor: Stopping monitoring directory %s", psFWMonitor->pu8Path);
    
    /* Remove from linked list */
    eLockLock(&sMonitorListLock);
    
    if (psFWMonitor_ListHead == psFWMonitor)
    {
        /* First in list */
        psFWMonitor_ListHead = psFWMonitor->psNext;
    }
    else
    {
        tsFWMonitor *psListPosition = psFWMonitor_ListHead;
        while (psListPosition->psNext)
        {
            if (psListPosition->psNext == psFWMonitor)
            {
                psListPosition->psNext = psListPosition->psNext->psNext;
                break;
            }
            psListPosition = psListPosition->psNext;
        }
    }

    eLockUnlock(&sMonitorListLock);
    
    /* Free resources */
    free(psFWMonitor->pu8Path);
    free(psFWMonitor);
    
    return NULL;
}


static teFWMonitor_Status FWMonitor_Scan(tsFWMonitor *psFWMonitor)
{
    DIR *psDir;
    struct dirent *psDirEnt;
    char acFilename[PATH_MAX];
    
    psDir = opendir(psFWMonitor->pu8Path);
    if (psDir == NULL)
    {
        daemon_log(LOG_INFO, "FWMonitor: Could not scan directory '%s' (%s)", psFWMonitor->pu8Path, strerror(errno));
        return FWM_STATUS_ERROR;
    }
    
    while (psDir)
    {
        errno = 0;
        if ((psDirEnt = readdir(psDir)) != NULL)
        {
            struct stat sStat;
            sprintf(acFilename, "%s/%s", psFWMonitor->pu8Path, psDirEnt->d_name);
            if (stat(acFilename, &sStat) < 0)
            {
                daemon_log(LOG_ERR, "FWMonitor: Could not stat '%s' (%s)", acFilename, strerror(errno));
                continue;
            }
            if (S_ISREG(sStat.st_mode))
            {
                daemon_log(LOG_INFO, "FWMonitor: Found file '%s'", acFilename);
#ifndef TEST
                eFirmware_Open(acFilename);
#endif /* TEST */
            }
        } 
        else
        {
            break;
        }
    }
    
    closedir(psDir);
    
    return FWM_STATUS_OK;
}


static teFWMonitor_Status FWMonitor_INotify(tsFWMonitor *psFWMonitor)
{
    int fd;
    int wd;
    char buffer[EVENT_BUF_LEN];
    char acFilename[PATH_MAX];

    /* Create the INOTIFY instance*/
    fd = inotify_init();
    if (fd < 0)
    {
        daemon_log(LOG_ERR, "FWMonitor: Couldn't initialise INotify (%s)", strerror(errno));
        return FWM_STATUS_ERROR;
    }

    /* Start watching the directory for creates, deletes and moves (in/out) */
    wd = inotify_add_watch(fd, psFWMonitor->pu8Path, IN_CREATE | IN_DELETE | IN_MOVE | IN_MODIFY);
    if (wd < 0)
    {
        daemon_log(LOG_ERR, "FWMonitor: Couldn't start watching directory '%s' (%s)", psFWMonitor->pu8Path, strerror(errno));
        
        /* Close the INOTIFY instance */
        close(fd);
        return FWM_STATUS_ERROR;
    }

    while (psFWMonitor->eState == E_MONITOR_STATE_RUNNING)
    {
        int length, i = 0;
        
        /* read  events */ 
        length = read(fd, buffer, EVENT_BUF_LEN); 

        /* check for errors */
        if (length < 0)
        {
            daemon_log(LOG_ERR, "FWMonitor: Error reading INotify events (%s)", strerror(errno));
            continue;
        }  

        /*actually read return the list of change events happens. Here, read the change event one by one and process it accordingly.*/
        while ( i < length )
        {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if (event->len)
            {
                if ((event->mask & IN_CREATE) || (event->mask & IN_MOVED_TO))
                {
                    if (event->mask & IN_ISDIR)
                    {
                        /* Not interested in created directories */
                    }
                    else
                    {
                        struct stat sStat;
                        
                        daemon_log(LOG_DEBUG, "FWMonitor: File '%s' %s", event->name, event->mask & IN_CREATE ? "created":"added");
                        
                        sprintf(acFilename, "%s/%s", psFWMonitor->pu8Path, event->name);
                        
                        if (stat(acFilename, &sStat) < 0)
                        {
                            daemon_log(LOG_ERR, "FWMonitor: Could not stat '%s' (%s)", acFilename, strerror(errno));
                        }
                        if (S_ISREG(sStat.st_mode))
                        {
                            daemon_log(LOG_INFO, "FWMonitor: Found file '%s'", acFilename);
#ifndef TEST
                            eFirmware_Open(acFilename);
#endif /* TEST */
                        }
                        
                    }
                }
                else if ((event->mask & IN_DELETE) || (event->mask & IN_MOVED_FROM))
                {
                    if (event->mask & IN_ISDIR)
                    {
                        /* Not interested in deleted directories */
                    }
                    else
                    {
                        daemon_log(LOG_DEBUG, "FWMonitor: File '%s' %s", event->name, event->mask & IN_DELETE ? "deleted":"removed");
                        
                        sprintf(acFilename, "%s/%s", psFWMonitor->pu8Path, event->name);
#ifndef TEST
                            eFirmware_Close(acFilename);
#endif /* TEST */
                    }
                }
                else if (event->mask & IN_MODIFY)
                {
                    struct stat sStat;
                    
                    daemon_log(LOG_DEBUG, "FWMonitor: File '%s' modified: 0x%08x", event->name, event->mask);
                    
                    sprintf(acFilename, "%s/%s", psFWMonitor->pu8Path, event->name);
                    
                    if (stat(acFilename, &sStat) < 0)
                    {
                        daemon_log(LOG_ERR, "FWMonitor: Could not stat '%s' (%s)", acFilename, strerror(errno));
                    }
                    if (S_ISREG(sStat.st_mode))
                    {
                        daemon_log(LOG_INFO, "FWMonitor: Found file '%s'", acFilename);
#ifndef TEST
                        eFirmware_Open(acFilename);
#endif /* TEST */
                    }
                }
                else
                {
                    /* Other events */
                }
            }
            i += EVENT_SIZE + event->len;
        }
    }
    
    /* Stop watching the directory */
    inotify_rm_watch(fd, wd);

    /* Close the INOTIFY instance */
    close(fd);
    
    return FWM_STATUS_OK;
}


#ifdef TEST
int main (int argc, char **argv)
{
    daemon_set_verbosity(LOG_DEBUG);
    
    FWMonitor_Init("/tmp/test");
    
    while (1)
    {
        sleep(60);
    }
    return 0;
}
#endif /* TEST */
