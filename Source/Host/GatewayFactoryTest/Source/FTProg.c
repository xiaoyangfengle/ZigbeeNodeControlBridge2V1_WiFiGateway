/****************************************************************************
 *
 * MODULE:             Gateway Factory Test
 *
 * COMPONENT:          FTDI chip programmer
 *
 * REVISION:           $Revision: 36357 $
 *
 * DATED:              $Date: 2011-11-02 11:52:12 +0000 (Wed, 02 Nov 2011) $
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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ftdi.h>


#ifndef VERSION
#error Version is not defined!
#else
const char *Version = "0.1 (r" VERSION ")";
#endif


/** Structure to map libftdi CBUS defines to string */
typedef struct
{
    int             iCBUSMode;      /**< libftdi enumeration */
    const char *    pcDescription;  /**< string representation */
} tsCBUSModeLookup;

/** Map libftdi CBUS defines to strings */
tsCBUSModeLookup asCBUSModeLookup[] = 
{
    { CBUS_TXDEN,       "TXDEN"},
    { CBUS_PWREN,       "PWREN"},
    { CBUS_RXLED,       "RXLED"},
    { CBUS_TXLED,       "TXLED"},
    { CBUS_TXRXLED,     "TXRXLED"},
    { CBUS_SLEEP,       "SLEEP"},
    { CBUS_CLK48,       "CLK48"},
    { CBUS_CLK24,       "CLK24"},
    { CBUS_CLK12,       "CLK12"},
    { CBUS_CLK6,        "CLK6"},
    { CBUS_IOMODE,      "IOMODE"},
    { CBUS_BB_WR,       "BB_WR"},
    { CBUS_BB_RD,       "BB_RD"},
};
    


/** Print help and exit
 *  \param argv     Command line arguments
 *  \return Does not return - program exits
 */
void print_usage_exit(char *argv[])
{
    int i;
    printf("FTProg version %s\n\r", Version);
    printf("Usage: %s\n\r", argv[0]);
    printf("  Options:\n\r");
    printf("    -h   --help                         Print this help.\n\r");
    printf("    -V   --verbosity    <Verbosity>     Set verbosity (default 0).\n\r");
    printf("    -v   --vid          <Vendor ID>     Vendor ID to use\n\r");
    printf("    -p   --pid          <Product ID>    Product ID to use\n\r");
    printf("    -m   --manufacturer <Manufacturer>  Set manufacturer string\n\r");
    printf("    -d   --description  <Description>   Set description string\n\r");
    printf("    -s   --serial       <Serial number> Set 8 character serial number string.\n\r");
    printf("    -a   --autoserial   <Serial prefix> Autogenerate serial number with 2 character prefix\n\r");
    printf("    -0   --C0           <IO Function>   Set IO function of C0 pin\n\r");
    printf("    -1   --C1           <IO Function>   Set IO function of C1 pin\n\r");
    printf("    -2   --C2           <IO Function>   Set IO function of C2 pin\n\r");
    printf("    -3   --C3           <IO Function>   Set IO function of C3 pin\n\r");
    printf("    -4   --C4           <IO Function>   Set IO function of C4 pin\n\r");
    printf("\n\r");
    printf("  Valid IO functions:\n\r   ");
    for (i = 0; i < (sizeof(asCBUSModeLookup)/sizeof(tsCBUSModeLookup)); i++)
    {
        printf(" %s", asCBUSModeLookup[i].pcDescription);
        if ((i % 8) == 7) printf("\n\r   ");
    }
    printf("\n\r");
    
    exit(EXIT_FAILURE);
}


/** Default vendor ID */
uint32_t u32VID         = 0x0403;
/** Default product ID */
uint32_t u32PID         = 0x6001;

/** Verbosity */
uint32_t u32Verbosity   = 0;

char *pcManufacturer    = NULL;
char *pcDescription     = NULL;

char *pcSerial          = NULL;
char *pcAutoSerial      = NULL;

/** CBUS functions */
#define CBUS_NUM 5
char    *apcCBUS[CBUS_NUM];
tsCBUSModeLookup *apsCBUS_Function[CBUS_NUM];


static int iMyStrtoul(uint32_t *pu32Result, const char *pcStr, const char *pcName, uint32_t u32Max)
{
    char *pcEnd;
    errno = 0;
    *pu32Result = strtoul(pcStr, &pcEnd, 0);
    if (pcEnd == pcStr)
    {
        fprintf(stderr, "Invalid %s '%s' (%s)\n\r", pcName, pcStr, "No valid characters");
        return -1;
    }
    if (errno)
    {
        fprintf(stderr, "Invalid %s '%s' (%s)\n\r", pcName, pcStr, strerror(errno));
        return -1;
    }
    if (*pu32Result > u32Max)
    {
        fprintf(stderr, "Invalid %s '%s' (%s)\n\r", pcName, pcStr, "Value out of range");
        return -1;
    }
    return 0;
}


int main(int argc, char *argv[])
{
    int ret, i;
    struct ftdi_device_list *devlist, *curdev;
    struct ftdi_context ftdic;
    struct ftdi_eeprom  eeprom;
    
    for (i = 0; i < CBUS_NUM; i++)
    {
        apcCBUS[i] = NULL;
    }
    
    {
        static struct option long_options[] =
        {
            
            {"help",                    no_argument,        NULL, 'h'},
            {"verbosity",               required_argument,  NULL, 'V'},
            
            {"vid",                     required_argument,  NULL, 'v'},
            {"pid",                     required_argument,  NULL, 'p'},
            
            {"manufacturer",            required_argument,  NULL, 'm'},
            {"description",             required_argument,  NULL, 'd'},
            
            {"serial",                  required_argument,  NULL, 's'},
            {"autoserial",              required_argument,  NULL, 'a'},
            
            {"C0",                      required_argument,  NULL, '0'},
            {"C1",                      required_argument,  NULL, '1'},
            {"C2",                      required_argument,  NULL, '2'},
            {"C3",                      required_argument,  NULL, '3'},
            {"C4",                      required_argument,  NULL, '4'},

            { NULL, 0, NULL, 0}
        };
        signed char opt;
        int option_index;
        
        while ((opt = getopt_long(argc, argv, "hV:v:p:m:d:s:a:0:1:2:3:4:", long_options, &option_index)) != -1) 
        {
            switch (opt) 
            {
                case 'h':
                    print_usage_exit(argv);
                    break;
                    
                case 'V':
                    if (iMyStrtoul(&u32Verbosity, optarg, "verbosity", 2))
                    {
                        return EXIT_FAILURE;
                    }
                    break;
                    
                case 'v':
                    if (iMyStrtoul(&u32VID, optarg, "vendor id", 0xFFFF))
                    {
                        return EXIT_FAILURE;
                    }
                    break;
                    
                case 'p':
                    if (iMyStrtoul(&u32PID, optarg, "product id", 0xFFFF))
                    {
                        return EXIT_FAILURE;
                    }
                    break;
                    
                case 'm':
                    pcManufacturer = optarg;
                    break;
                
                case 'd':
                    pcDescription = optarg;
                    break;
                
                case 's':
                    pcSerial = optarg;
                    break;
                
                case 'a':
                    pcAutoSerial = optarg;
                    break;
                
                case '0':
                    apcCBUS[0] = optarg;
                    break;
                
                case '1':
                    apcCBUS[1] = optarg;
                    break;
                
                case '2':
                    apcCBUS[2] = optarg;
                    break;
                
                case '3':
                    apcCBUS[3] = optarg;
                    break;
                
                case '4':
                    apcCBUS[4] = optarg;
                    break;
                     
                default: /* '?' */
                    print_usage_exit(argv);
            }
        }
    }
    
    printf("FTProg FTDI Programmer version %s\n\r", Version);
    
    /* Handle serial numbers */
    if (pcSerial && pcAutoSerial)
    {
        fprintf(stderr, "Please specify only one of serial number and autoserial\n\r");
        return EXIT_FAILURE;
    }
    
    if (pcSerial)
    {
        if (strlen(pcSerial) != 8)
        {
            fprintf(stderr, "Serial number must be 8 character string\n\r");
            return EXIT_FAILURE;
        }
    }
    
    if (pcAutoSerial)
    {
        int fd;
        if (strlen(pcAutoSerial) != 2)
        {
            fprintf(stderr, "Serial number prefix must be 2 character string\n\r");
            return EXIT_FAILURE;
        }
        
        pcSerial = malloc(sizeof(char) * 9);
        if (!pcSerial)
        {
            fprintf(stderr, "Memory error\n");
            return EXIT_FAILURE;
        }
        pcSerial[0] = pcAutoSerial[0];
        pcSerial[1] = pcAutoSerial[1];
        
        if ((fd = open("/dev/urandom", O_RDONLY)) < 0)
        {
            fprintf(stderr, "Could not open /dev/urandom (%s)\n\r", strerror(errno));
            return EXIT_FAILURE;
        }
        for (i = 2; i < 8; i++)
        {
            uint32_t j;
            read(fd, &j, 1);
            pcSerial[i] = "0123456789ABCDEF"[j % 16];
        }
        close(fd);

        pcSerial[8] = '\0';
        if (u32Verbosity > 0)
        {
            printf("Autogenerated serial number: '%s'\n", pcSerial);
        }
    }
        
    
    for (i = 0; i < CBUS_NUM; i++)
    {
        if (apcCBUS[i])
        {
            int j, iFound = 0;
            for (j = 0; j < (sizeof(asCBUSModeLookup)/sizeof(tsCBUSModeLookup)); j++)
            {
                if (strcmp(apcCBUS[i], asCBUSModeLookup[j].pcDescription) == 0)
                {
                    iFound = 1;
                    apsCBUS_Function[i] = &asCBUSModeLookup[j];
                }
            }
            if (!iFound)
            {
                fprintf(stderr, "Invalid C%d '%s'\n\r", i, apcCBUS[i]);
                return EXIT_FAILURE;
            }
        }
    }
    
    if (ftdi_init(&ftdic) < 0)
    {
        fprintf(stderr, "ftdi_init failed\n");
        return EXIT_FAILURE;
    }

    if ((ret = ftdi_usb_find_all(&ftdic, &devlist, u32VID, u32PID)) < 0)
    {
        fprintf(stderr, "ftdi_usb_find_all failed: %d (%s)\n", ret, ftdi_get_error_string(&ftdic));
        return EXIT_FAILURE;
    }

    printf("Number of FTDI devices found: %d\n", ret);

    for (i=0, curdev = devlist; curdev != NULL; i++, curdev = curdev->next)
    {
        printf(" Updating device: %d\n\r", i);
        
        if ((ret = ftdi_usb_open_dev(&ftdic, curdev->dev)) < 0)
        {
            fprintf(stderr, "ftdi_usb_open_dev failed: %d (%s)\n", ret, ftdi_get_error_string(&ftdic));
            return EXIT_FAILURE;
        }
    
        // Read out FTDIChip-ID of R type chips
        if (ftdic.type == TYPE_R)
        {
            unsigned char *pcEepromBuffer = NULL;
            unsigned int chipid;

            if ((ret = ftdi_read_chipid(&ftdic, &chipid)) < 0)
            {
                fprintf(stderr, "ftdi_read_chipid failed: %d (%s)\n", ret, ftdi_get_error_string(&ftdic));
                return EXIT_FAILURE;
            }
            
            printf("  FTDI chipid: %X\n", chipid);
            
            /* EEPROM is 160 bytes */
            ftdi_eeprom_setsize(&ftdic, &eeprom, 160);
            
            printf("  EEPROM size: %d bytes\n", ftdic.eeprom_size);
            pcEepromBuffer = malloc(ftdic.eeprom_size);
            if (!pcEepromBuffer)
            {
                fprintf(stderr, "Memory error!");
                return EXIT_FAILURE;
            }
            else
            {
                
                int i;
                if ((ret = ftdi_read_eeprom(&ftdic, pcEepromBuffer)) < 0)
                {
                    fprintf(stderr, "ftdi_read_eeprom failed: %d (%s)\n", ret, ftdi_get_error_string(&ftdic));
                    return EXIT_FAILURE;
                }
                if ((ret = ftdi_eeprom_decode(&eeprom, pcEepromBuffer, ftdic.eeprom_size)) < 0)
                {
                    if (ret != -1)
                    {
                        /* Avoid errors due to hard coded size causing checksum to fail. */
                        fprintf(stderr, "Error decoding eeprom buffer: %d (%s)\n", ret, ftdi_get_error_string(&ftdic));
                        return EXIT_FAILURE;
                    }
                }

                if (u32Verbosity > 0)
                {
                    printf("    Current Vendor: %d, %s\n", eeprom.vendor_id, eeprom.manufacturer);
                    printf("    Current Product: %d, %s\n", eeprom.product_id, eeprom.product);
                    printf("    Current Serial: %s\n", eeprom.serial);

                    if (u32Verbosity >= 2)
                    {
                        printf("      Current EEPROM Contents\n\r");
                        for (i = 0; i < ftdic.eeprom_size; i+=2)
                        {
                            if ((i % 16) == 0)
                            {
                                printf("        %04x:", i/2);
                            }
                            printf(" 0x%02x%02x", pcEepromBuffer[i],pcEepromBuffer[i+1]);
                            if ((i % 16) == 14)
                            {
                                printf("\n\r");
                            }
                        }
                    }
                }
                
                if (pcManufacturer)
                {
                    printf("  Setting manufacturer: '%s'\n\r", pcManufacturer);
                    free(eeprom.manufacturer);
                    eeprom.manufacturer     = pcManufacturer;
                }
                
                if (pcDescription)
                {
                    printf("  Setting description: '%s'\n\r", pcDescription);
                    free(eeprom.product);
                    eeprom.product          = pcDescription;
                }
                
                if (pcSerial)
                {
                    printf("  Setting serial number: '%s'\n\r", pcSerial);
                    free(eeprom.serial);
                    eeprom.serial           = pcSerial;
                }
                
                for (i = 0; i < CBUS_NUM; i++)
                {
                    if (apcCBUS[i])
                    {
                        /* Argument given */
                        printf("  Setting C%d: '%s' (%d)\n\r", i, 
                               apsCBUS_Function[i]->pcDescription, 
                               apsCBUS_Function[i]->iCBUSMode);
                        
                        eeprom.cbus_function[i] = apsCBUS_Function[i]->iCBUSMode;
                    }
                }

                /* EEPROM is 160 bytes */
                ftdi_eeprom_setsize(&ftdic, &eeprom, 160);
                
                if ((ret = ftdi_eeprom_build(&eeprom, pcEepromBuffer)) < 0)
                {
                    fprintf(stderr, "ftdi_eeprom_build failed: %d (%s)\n\r", ret, ftdi_get_error_string(&ftdic));
                    return -1;
                }

                if ((ret = ftdi_write_eeprom(&ftdic, pcEepromBuffer)) < 0)
                {
                    fprintf(stderr, "ftdi_write_eeprom failed: %d (%s)\n", ret, ftdi_get_error_string(&ftdic));
                    return -1;
                }
                printf("  Wrote new EEPROM data successfully\n\r");
            }
        }

        if ((ret = ftdi_usb_close(&ftdic)) < 0)
        {
            fprintf(stderr, "ftdi_usb_close failed: %d (%s)\n\r", ret, ftdi_get_error_string(&ftdic));
            return EXIT_FAILURE;
        }
    }
    
    ftdi_list_free(&devlist);
    ftdi_deinit(&ftdic);
    return EXIT_SUCCESS;
}

