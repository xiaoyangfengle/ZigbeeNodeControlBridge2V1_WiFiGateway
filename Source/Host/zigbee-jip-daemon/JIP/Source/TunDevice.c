/****************************************************************************
 *
 * MODULE:             Linux Zigbee - JIP Daemon
 *
 * COMPONENT:          Interface to tun device
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <errno.h>

#include <libdaemon/daemon.h>

#include "TunDevice.h"
#include "Utils.h"


#define DBG_FAKENET 0


char *pcTD_DevName = "zb0";

/** File descriptor for tun device */
static int tun_fd = 0;

static tsUtilsThread sTDReader;

static void *pvTD_ReaderThread(tsUtilsThread *psThreadInfo);


teTdStatus eTD_Init(void)
{
    struct ifreq ifr;
    int fd, err;

    if((fd = open("/dev/net/tun", O_RDWR)) < 0)
    {
        daemon_log(LOG_ERR, "Open /dev/net/tun failed (%s)", strerror(errno));
        return E_TD_ERROR;
    }

    memset(&ifr, 0, sizeof(ifr));

    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
            *        IFF_TAP   - TAP device
            *
            *        IFF_NO_PI - Do not provide packet information
    */
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, pcTD_DevName, IFNAMSIZ);

    err = ioctl(fd, TUNSETIFF, (void *) &ifr);
    if(err < 0)
    {
        daemon_log(LOG_ERR, "Couldn't set options on tun device (%s)", strerror(errno));
        close(fd);
        return E_TD_ERROR;
    }

    daemon_log(LOG_DEBUG, "Opened tun device: %s", ifr.ifr_name);
    
    {
        char acCmdBuffer[1024];
        int  iResult;
        
        snprintf(acCmdBuffer, 1024, "ifconfig %s up", pcTD_DevName);
        iResult = system(acCmdBuffer);
        
        DBG_vPrintf(DBG_FAKENET, "%s returned %d\n", acCmdBuffer, iResult);
        if (iResult != 0)
        {
            daemon_log(LOG_ERR, "Error bringing up interface");
            return E_TD_ERROR;
        }
    }

    tun_fd = fd;
    
    if (eUtils_ThreadStart(pvTD_ReaderThread, &sTDReader, E_THREAD_JOINABLE) != E_UTILS_OK)
    {
        daemon_log(LOG_ERR, "Failed to start tun device reader thread");
        return E_TD_ERROR;
    }

    return E_TD_OK;
}


teTdStatus eTD_Destory(void)
{
    eUtils_ThreadStop(&sTDReader);
    return E_TD_OK;
}


teTdStatus eTD_AddIPAddress(const char *pcAddress)
{
    char acCmdBuffer[1024];
    int  iResult;
    
    snprintf(acCmdBuffer, 1024, "ip -6 addr add %s/64 dev %s", pcAddress, pcTD_DevName);
    iResult = system(acCmdBuffer);
    
    DBG_vPrintf(DBG_FAKENET, "%s returned %d\n", acCmdBuffer, iResult);
    if (iResult != 0)
    {
        daemon_log(LOG_ERR, "Error adding address %s to interface %s", pcAddress, pcTD_DevName);
        return E_TD_ERROR;
    }
    
    return E_TD_OK;
}


teTdStatus eTD_RemoveIPAddress(const char *pcAddress)
{
    char acCmdBuffer[1024];
    int  iResult;
    
    snprintf(acCmdBuffer, 1024, "ip -6 addr del %s/64 dev %s", pcAddress, pcTD_DevName);
    iResult = system(acCmdBuffer);
    
    DBG_vPrintf(DBG_FAKENET, "%s returned %d\n", acCmdBuffer, iResult);
    if (iResult != 0)
    {
        daemon_log(LOG_ERR, "Error removing address %s from interface %s", pcAddress, pcTD_DevName);
        return E_TD_ERROR;
    }
    
    return E_TD_OK;
}




/** This thread just exists to read stuff from the tun device to make sure it doesn't clog up */
static void *pvTD_ReaderThread(tsUtilsThread *psThreadInfo)
{
    DBG_vPrintf(DBG_FAKENET, "Starting\n");
    
    psThreadInfo->eState = E_THREAD_RUNNING;
    
    while (psThreadInfo->eState == E_THREAD_RUNNING)
    {
        unsigned char buf[2048];
        int len;
        len = read(tun_fd, buf, sizeof(buf));
        if (len > 0)
        {
            DBG_vPrintf(DBG_FAKENET, "Read %d bytes from tun device\n", len);
        }
    }
    DBG_vPrintf(DBG_FAKENET, "Exiting\n");
    return NULL;
}



