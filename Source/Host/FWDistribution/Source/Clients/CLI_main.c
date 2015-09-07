/****************************************************************************
 *
 * MODULE:             Firmware Distribution Client
 *
 * COMPONENT:          CLI client
 *
 * REVISION:           $Revision: 51415 $
 *
 * DATED:              $Date: 2013-01-10 09:43:33 +0000 (Thu, 10 Jan 2013) $
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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include "../Common/FWDistribution_IPC.h"


#ifndef VERSION
#error Version is not defined!
#else
const char *Version = VERSION;
#endif

static int iFWDistributionIPCSocket = 0;


/** Block interval, in units of 10ms */
static uint16_t u16BlockInterval = 100; /* Default = 1s */

/** Timeout before devices reset, in units of 10ms */
static uint16_t u16ResetTimeout = 100; /* Default = 1s */

/** Amount of influence network depth has on reset timeout */
static uint16_t u16ResetDepthInfluence = 10;

/** Number of repeats of reset command */
static uint16_t u16ResetRepeatCount = 1; /* Default = 1 repeat */

/** Amount of influence network depth has on reset timeout */
static uint16_t u16ResetRepeatTime = 20; /* Default = 200ms */


static int FWDistributionIPCClientGetFirmwares(void);
static int FWDistributionIPCClientStartDownload(uint32_t u32DeviceID, uint16_t u16ChipType, uint16_t u16Revision, struct in6_addr sAddress, enum teStartDownloadIPCFlags eFlags, uint16_t u16BlockInterval);
static int FWDistributionIPCClientCancelDownload(uint32_t u32DeviceID, uint16_t u16ChipType, uint16_t u16Revision, struct in6_addr sAddress);
static int FWDistributionIPCClientReset(uint32_t u32DeviceID, struct in6_addr sAddress, uint16_t u16RepeatCount, uint16_t u16RepeatTime);
static int FWDistributionIPCClientMonitor(void);


void print_usage_exit(char *argv[])
{
    fprintf(stderr, "Firmware Distribution Control Program Version: %s\n", Version);
    fprintf(stderr, "Usage: %s\n", argv[0]);
//    fprintf(stderr, "  Arguments:\n");
//    fprintf(stderr, "    -s <serial device> Serial device for 15.4 module, e.g. /dev/tts/1\n");
    fprintf(stderr, "  Options:\n");
    fprintf(stderr, "    -h   --help            Print this help.\n");
    fprintf(stderr, "    -l   --list            Retrieve a list of available firmwares on the server.\n");
    fprintf(stderr, "    -d   --download        Start download\n");
    fprintf(stderr, "                            Requires --ipv6, --deviceid, --chiptype and --revision\n");
    fprintf(stderr, "    -c   --cancel          Cancel download\n");
    fprintf(stderr, "                            Requires --ipv6, --deviceid, --chiptype and --revision\n");
    fprintf(stderr, "    -i   --inform          Inform network of available firmware\n");
    fprintf(stderr, "                            Requires --ipv6, --deviceid, --chiptype and --revision\n");
    fprintf(stderr, "    -s   --status          Get status of active and completed downloads\n");
    fprintf(stderr, "    -m   --monitor         Monitors active and completed downloads\n");
    fprintf(stderr, "    -r   --reset           Reset.\n");
    fprintf(stderr, "                            If used at the same time as starting a download, this will\n");
    fprintf(stderr, "                            cause devices to reset immediately upon recieving the image\n");
    fprintf(stderr, "                            If used seperately, Requires --ipv6, --deviceid.\n");
    fprintf(stderr, "                            The time to reset (--reset-timeout) and depth influence\n");
    fprintf(stderr, "                            (--reset-depth-influenece) may also be specified\n");
    fprintf(stderr, "                            The number of repeats of the reset request (--reset-repeat-count)\n");
    fprintf(stderr, "                            and the time between them(--reset-repeat-time) may also be specified\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    -6   --ipv6            IPv6 address of Network Coordinator\n");
    fprintf(stderr, "    -I   --deviceid        Set Device ID of firmware to download\n");
    fprintf(stderr, "    -T   --chiptype        Set Chip Type of firmware to download\n");
    fprintf(stderr, "    -R   --revision        Set Revision of firmware to download\n");
    fprintf(stderr, "         --block-interval  Time to wait between sending image blocks, in units of 10ms.\n");
    fprintf(stderr, "         --reset-timeout\n");
    fprintf(stderr, "                           Time to reset, in units of 10ms. 0=immediate. Default=%d (%.2fs)\n", u16ResetTimeout, (float)u16ResetTimeout/100.0);
    fprintf(stderr, "         --reset-depth-influence\n");
    fprintf(stderr, "                           Amount of influence that depth in netork has on reset time, Default=%d\n", u16ResetDepthInfluence);
    fprintf(stderr, "         --reset-repeat-count\n");
    fprintf(stderr, "                           Number of times to repeat the reset command, Default=%d\n", u16ResetRepeatCount);
    fprintf(stderr, "         --reset-repeat-time\n");
    fprintf(stderr, "                           Time between reset repeats, in units of 10ms. Default=%d (%.2fs)\n", u16ResetRepeatTime, (float)u16ResetRepeatTime/100.0); 
    exit(EXIT_SUCCESS);
}


int main (int argc, char **argv)
{
    const char *pcDeviceID = NULL;
    uint32_t    u32DeviceID = 0;
    const char *pcChipType = NULL;
    uint16_t    u16ChipType = 0;
    const char *pcRevision = NULL;
    uint16_t    u16Revision = 0;
    const char *pcAddress = NULL;
    struct in6_addr sAddress;
    int StartDownload = 0;
    int CancelDownload = 0;
    int Inform = 0;
    int Status = 0;
    int Monitor = 0;
    int Reset = 0;

    /* Connect to daemon */
    {
        int len;
        struct sockaddr_un remote;

        if ((iFWDistributionIPCSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }
        
        remote.sun_family = AF_UNIX;
        strcpy(remote.sun_path, FWDistributionIPCSockPath);
        len = strlen(remote.sun_path) + sizeof(remote.sun_family);
        if (connect(iFWDistributionIPCSocket, (struct sockaddr *)&remote, len) == -1) {
            perror("connect");
            exit(1);
        }
    }
    
    {
        signed char opt;
        int option_index;
        
        static struct option long_options[] =
        {
            
            {"help",                    no_argument,        NULL, 'h'},
            {"list",                    no_argument,        NULL, 'l'},
            {"download",                no_argument,        NULL, 'd'},
            {"cancel",                  no_argument,        NULL, 'c'},
            {"inform",                  no_argument,        NULL, 'i'},
            {"status",                  no_argument,        NULL, 's'},
            {"monitor",                 no_argument,        NULL, 'm'},
            {"reset",                   no_argument,        NULL, 'r'},
            {"ipv6",                    required_argument,  NULL, '6'},
            {"deviceid",                required_argument,  NULL, 'I'},
            {"chiptype",                required_argument,  NULL, 'T'},
            {"revision",                required_argument,  NULL, 'R'},
            
            {"block-interval",          required_argument,  NULL, 20},
            
            {"reset-timeout",           required_argument,  NULL, 1},
            {"reset-depth-influence",   required_argument,  NULL, 2},
            {"reset-repeat-count",      required_argument,  NULL, 3},
            {"reset-repeat-time",       required_argument,  NULL, 4},
            
            { NULL, 0, NULL, 0}
        };

        while ((opt = getopt_long(argc, argv, "hldcismr6:I:T:R:", long_options, &option_index)) != -1) 
        {
            char *pEnd;
            
            switch (opt) 
            {
                case 0:
                    break;
                    
                case 'h':
                    print_usage_exit(argv);
                    break;

                case 'l':
                    printf("List of available firmwares:\n");
                    FWDistributionIPCClientGetFirmwares();
                    break;
                
                case 'd':
                    StartDownload = 1;
                    break;
                
                case 'c':
                    CancelDownload = 1;
                    break;

                case 'i':
                    Inform = 1;
                    break;
                
                case 's':
                    Status = 1;
                    break;
                
                case 'm':
                    Monitor = 1;
                    break;
                
                case 'r':
                    Reset = 1;
                    break;
                    
                case '6':
                {
                    pcAddress = optarg;
                    
                    if (inet_pton(AF_INET6, optarg, &sAddress) != 1)
                    {
                        perror("Invalid IPv6 address specified");
                        exit(1);
                    }
                    break;
                }
                    
                case 'I':
                {
                    pcDeviceID = optarg;
                    
                    errno = 0;
                    u32DeviceID = (uint32_t)strtoll(optarg, &pEnd, 0);
                    if ((errno) || (pEnd == optarg))
                    {
                        perror("Invalid Device ID specified");
                        exit(1);
                    }
                    break;
                }
                    
                case 'T':
                {
                    pcChipType = optarg;
                    
                    errno = 0;
                    u16ChipType = (uint16_t)strtol(optarg, &pEnd, 0);
                    if ((errno) || (pEnd == optarg))
                    {
                        perror("Invalid Chip Type specified");
                        exit(1);
                    }
                    break;
                }
                    
                case 'R':
                {
                    pcRevision = optarg;
                    
                    errno = 0;
                    u16Revision = (uint16_t)strtol(optarg, &pEnd, 0);
                    if ((errno) || (pEnd == optarg))
                    {
                        perror("Invalid Revision specified");
                        exit(1);
                    }
                    break;
                }
                
                case 20:
                    u16BlockInterval = (uint16_t)strtol(optarg, &pEnd, 0);
                    if ((errno) || (pEnd == optarg))
                    {
                        perror("Invalid block interval specified");
                        exit(1);
                    }
                    break;
                
                case 1:
                    u16ResetTimeout = (uint16_t)strtol(optarg, &pEnd, 0);
                    if ((errno) || (pEnd == optarg))
                    {
                        perror("Invalid reset timeout specified");
                        exit(1);
                    }
                    break;
                
                case 2:
                    u16ResetDepthInfluence = (uint16_t)strtol(optarg, &pEnd, 0);
                    if ((errno) || (pEnd == optarg))
                    {
                        perror("Invalid reset depth influence specified");
                        exit(1);
                    }
                    break;
                    
                case 3:
                    u16ResetRepeatCount = (uint16_t)strtol(optarg, &pEnd, 0);
                    if ((errno) || (pEnd == optarg))
                    {
                        perror("Invalid reset repeat count specified");
                        exit(1);
                    }
                    break;
                
                case 4:
                    u16ResetRepeatTime = (uint16_t)strtol(optarg, &pEnd, 0);
                    if ((errno) || (pEnd == optarg))
                    {
                        perror("Invalid reset repeat time specified");
                        exit(1);
                    }
                    break;
 
                case '?':
                    /* getopt_long already printed an error message. */
                    break;
 
                default: /* '?' */
                    print_usage_exit(argv);
            }
        }
    }
    
    if (StartDownload || CancelDownload || Inform )
    {
        enum teStartDownloadIPCFlags eFlags = 0;

        if (!pcAddress)
        {
            printf("Error starting download: No Coordinator IPv6 address specified\n\n");
            print_usage_exit(argv);
        }
        if (!pcDeviceID)
        {
            printf("Error starting download: No Device ID specified\n\n");
            print_usage_exit(argv);
        }
        if (!pcChipType)
        {
            printf("Error starting download: No Chip Type specified\n\n");
            print_usage_exit(argv);
        }
        if (!pcRevision)
        {
            printf("Error starting download: No Revision specified\n\n");
            print_usage_exit(argv);
        }
        
        if (Inform)
        {
            printf("Informing coodinator %s of firmware for Device ID 0x%08x, Chip Type 0x%04x, Revision %d\n", 
                    pcAddress, u32DeviceID, u16ChipType, u16Revision);
            eFlags |= E_DOWNLOAD_IPC_FLAG_INFORM;
        }
        else
        {
            if (Reset)
            {
                eFlags |= E_DOWNLOAD_IPC_FLAG_RESET;
            }
        }

        if (CancelDownload)
        {
            printf("Requesting cancel of firmware download for Device ID 0x%08x, Chip Type 0x%04x, Revision %d via coordinator %s\n", 
                    u32DeviceID, u16ChipType, u16Revision, pcAddress);
            FWDistributionIPCClientCancelDownload(u32DeviceID, u16ChipType, u16Revision, sAddress);
        }
        else
        {
            printf("Requesting download of firmware for Device ID 0x%08x, Chip Type 0x%04x, Revision %d via coordinator %s: with interval %d. %s\n", 
                    u32DeviceID, u16ChipType, u16Revision, pcAddress, u16BlockInterval, Reset ? "Immediate Reset":"Manual Reset");
            FWDistributionIPCClientStartDownload(u32DeviceID, u16ChipType, u16Revision, sAddress, eFlags, u16BlockInterval);
        }
    }
    else if (Reset)
    {
        if (!pcAddress)
        {
            printf("Error resetting nodes: No Coordinator IPv6 address specified\n\n");
            print_usage_exit(argv);
        }
        if (!pcDeviceID)
        {
            printf("Error resetting nodes: No Device ID specified\n\n");
            print_usage_exit(argv);
        }
        
        if ((u16ResetRepeatTime * u16ResetRepeatCount) > u16ResetTimeout)
        {
            printf("Error resetting nodes: The specified reset repeat time and reset repeat count would exceed the reset timeout\n\n");
            print_usage_exit(argv);
        }
        
        printf("Requesting Reset of Device ID 0x%08x via coordinator %s in %.2f seconds, depth influence=%d\n", 
                u32DeviceID, pcAddress, (float)u16ResetTimeout / 100.0, u16ResetDepthInfluence);

        FWDistributionIPCClientReset(u32DeviceID, sAddress, u16ResetRepeatCount, u16ResetRepeatTime);
    }
    else
    {
        /* No action specified - just monitor. */
    }
    
    if (Status)
    {
        FWDistributionIPCClientMonitor();
    }   
 
    if (Monitor)
    {
        printf("Monitoring status\n");
        
        while (1)
        {
            if (FWDistributionIPCClientMonitor() != 0)
            {
                break;
            }
            sleep(1);
        }
    }

    return 0;
}


static int FWDistributionIPCClientGetFirmwares(void)
{
    tsFWDistributionIPCHeader sHeader;
    uint32_t u32NumFirmwares, i;
    
    sHeader.eType = IPC_GET_AVAILABLE_FIRMWARES;
    sHeader.u32PayloadLength = 0;

    if (send(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        perror("send");
        exit(1);
    }
    
    if (recv(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        perror("recv");
        exit(1);
    }

    if (sHeader.eType != IPC_AVAILABLE_FIRMWARES)
    {
        printf("Unexpected reply\n");
        exit(1);
    }
    
    u32NumFirmwares = sHeader.u32PayloadLength / sizeof(tsFWDistributionIPCAvailableFirmwareRecord);
    
    printf("Device ID      Chip       Revision     Filename \n");
    
    for (i = 0; i < u32NumFirmwares; i++)
    {
        tsFWDistributionIPCAvailableFirmwareRecord sRecord;
        
        if (recv(iFWDistributionIPCSocket, &sRecord, sizeof(tsFWDistributionIPCAvailableFirmwareRecord), 0) == -1) 
        {
            perror("recv");
            exit(1);
        }
        
        printf("0x%08x     0x%04x       %5d      %s\n",  sRecord.u32DeviceID, sRecord.u16ChipType, sRecord.u16Revision, sRecord.acFilename);
    }
    
    return 0;
}

static int FWDistributionIPCClientStartDownload(uint32_t u32DeviceID, uint16_t u16ChipType, uint16_t u16Revision, struct in6_addr sAddress, enum teStartDownloadIPCFlags eFlags, uint16_t u16BlockInterval)
{
    tsFWDistributionIPCHeader sHeader;
    tsFWDistributionIPCStartDownload sPacket;
    
    sHeader.eType = IPC_START_DOWNLOAD;
    sHeader.u32PayloadLength = sizeof(tsFWDistributionIPCStartDownload);
    
    sPacket.u32DeviceID = u32DeviceID;
    sPacket.u16ChipType = u16ChipType;
    sPacket.u16Revision = u16Revision;
    sPacket.u16BlockInterval = u16BlockInterval;
    sPacket.sAddress = sAddress;
    sPacket.eFlags = eFlags;
    
    if (send(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        perror("send");
        exit(1);
    }

    if (send(iFWDistributionIPCSocket, &sPacket, sizeof(tsFWDistributionIPCStartDownload), 0) == -1) 
    {
        perror("send");
        exit(1);
    }
    return 0;
}


static int FWDistributionIPCClientCancelDownload(uint32_t u32DeviceID, uint16_t u16ChipType, uint16_t u16Revision, struct in6_addr sAddress)
{
    tsFWDistributionIPCHeader sHeader;
    tsFWDistributionIPCCancelDownload sPacket;
    
    sHeader.eType = IPC_CANCEL_DOWNLOAD;
    sHeader.u32PayloadLength = sizeof(tsFWDistributionIPCCancelDownload);
    
    sPacket.u32DeviceID = u32DeviceID;
    sPacket.u16ChipType = u16ChipType;
    sPacket.u16Revision = u16Revision;
    sPacket.sAddress = sAddress;
    
    if (send(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        perror("send");
        exit(1);
    }

    if (send(iFWDistributionIPCSocket, &sPacket, sizeof(tsFWDistributionIPCCancelDownload), 0) == -1) 
    {
        perror("send");
        exit(1);
    }
    return 0;
}


static int FWDistributionIPCClientReset(uint32_t u32DeviceID, struct in6_addr sAddress, uint16_t u16RepeatCount, uint16_t u16RepeatTime)
{
    tsFWDistributionIPCHeader sHeader;
    tsFWDistributionIPCReset sPacket;
    
    sHeader.eType = IPC_RESET;
    sHeader.u32PayloadLength = sizeof(tsFWDistributionIPCReset);
    
    sPacket.u32DeviceID = u32DeviceID;
    sPacket.u16Timeout  = u16ResetTimeout;
    sPacket.u16DepthInfluence = u16ResetDepthInfluence;
    sPacket.sAddress = sAddress;
    sPacket.u16RepeatCount = u16RepeatCount;
    sPacket.u16RepeatTime = u16RepeatTime;
    
    if (send(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        perror("send");
        exit(1);
    }

    if (send(iFWDistributionIPCSocket, &sPacket, sizeof(tsFWDistributionIPCReset), 0) == -1) 
    {
        perror("send");
        exit(1);
    }
    return 0;
}


static int FWDistributionIPCClientMonitor(void)
{
    tsFWDistributionIPCHeader sHeader;
    uint32_t i;
    static uint32_t u32StatusRecords = 0;
    
    sHeader.eType = IPC_GET_STATUS;
    sHeader.u32PayloadLength = 0;

    if (send(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        perror("send");
        exit(1);
    }
    
    if (recv(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        perror("recv");
        exit(1);
    }

    if (sHeader.eType != IPC_STATUS)
    {
        printf("Unexpected reply\n");
        exit(1);
    }
    
    if (u32StatusRecords) 
    {
        /* Move cursor up by the number of lines the previous print took */
        printf("%c[%dA", 0x1B, u32StatusRecords);
    }
    
    u32StatusRecords = sHeader.u32PayloadLength / sizeof(tsFWDistributionIPCStatusRecord);
    
    for (i = 0; i < u32StatusRecords; i++)
    {
        char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
        
        tsFWDistributionIPCStatusRecord sRecord;
        
        if (recv(iFWDistributionIPCSocket, &sRecord, sizeof(tsFWDistributionIPCStatusRecord), 0) == -1) 
        {
            perror("recv");
            exit(1);
        }
        
        inet_ntop(AF_INET6, &sRecord.sAddress, buffer, INET6_ADDRSTRLEN);
        printf("%20s  0x%08x%08x 0x%08x 0x%04x %5d - %5d / %5d : %d\n",  
               buffer, sRecord.u32AddressH, sRecord.u32AddressL, 
               sRecord.u32DeviceID, sRecord.u16ChipType, sRecord.u16Revision, 
               sRecord.u32TotalBlocks - sRecord.u32RemainingBlocks, sRecord.u32TotalBlocks, sRecord.u32Status);

    }

    return 0;
}


