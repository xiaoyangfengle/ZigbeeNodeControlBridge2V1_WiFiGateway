/****************************************************************************
 *
 * MODULE:             JIPv4 compatability daemon
 *
 * COMPONENT:          Common components
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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <glob.h>
#include <errno.h>

#include <libdaemon/daemon.h>

#include <Common.h>



int Get_Module_Address(struct in6_addr *psAddress)
{
    glob_t sResults;
    
    if (glob("/tmp/6LoWPANd.*",  0, NULL, &sResults) == 0)
    {
        //printf("6LoWPANd files: %d\n", sResults.gl_pathc);
        
        if (sResults.gl_pathc > 0)
        {
            FILE * f;
            char *pcAddress = malloc(INET6_ADDRSTRLEN + 2);
            int iAddressLength = INET6_ADDRSTRLEN + 2;

            //printf("Opening file: %s\n", sResults.gl_pathv[0]);
            
            f = fopen (sResults.gl_pathv[0], "r");
            if (f == NULL)
            {
                daemon_log(LOG_ERR, "Error reading Module address: fopen (%s)", strerror(errno));
                free(pcAddress);
                return -2;
            }

            if (getline (&pcAddress, &iAddressLength, f) < 0)
            {
                daemon_log(LOG_ERR, "Error reading Module address: getline (%s)", strerror(errno));
                fclose(f);
                free(pcAddress);
                return -2;
            }
            
            /* Strip off unwanted characters */
            pcAddress[strcspn(pcAddress, " \t\n\r")] = '\0';
   
            if (inet_pton(AF_INET6, pcAddress, psAddress) <= 0)
            {
                daemon_log(LOG_ERR, "Error converting string to address (%s)", strerror(errno));;
                fclose(f);
                free(pcAddress);
                return -1;
            }

            fclose(f);
            free(pcAddress);
        }

        globfree(&sResults);
        return 0;
    }
    return -1;
}


