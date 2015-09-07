/****************************************************************************
 *
 * MODULE:             Gateway Factory Test
 *
 * COMPONENT:          Tests to run on gateway
 *
 * REVISION:           $Revision: 40016 $
 *
 * DATED:              $Date: 2012-03-27 09:01:19 +0100 (Tue, 27 Mar 2012) $
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
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "CGI.h"

#ifndef VERSION
#error Version is not defined!
#else
const char *Version = "0.2 (r" VERSION ")";
#endif


/** Time in seconds before a test timeout will be generated */
#define TEST_TIMEOUT (5)

static tsCGI sCGI;

typedef struct _tsTest
{
    int                 (*prFunction)(struct _tsTest *psTest, int iStage);
    const char *        pcLink;
    const char *        pcDisplay;
} tsTest;


int test_dr1182_leds        (tsTest *psTest, int iStage);
int test_dr1182_buttons     (tsTest *psTest, int iStage);
int configure_usb           (tsTest *psTest, int iStage);
int test_dr1182_usb_uart    (tsTest *psTest, int iStage);
int test_dr1182_reset       (tsTest *psTest, int iStage);
int program_dongle          (tsTest *psTest, int iStage);


/** Here we add the tests as relevant to the target profile. */
tsTest asTests[] = 
{
#ifdef PROFILE_DR1182
    {   test_dr1182_leds    , "LEDs"            , "Test LEDs" },
    {   test_dr1182_buttons , "Buttons"         , "Test Buttons" },
#endif /* PROFILE_DR1182 */
    {   configure_usb       , "ConfigUSBProg"   , "Set Up USB Dongle" },
#ifdef PROFILE_DR1182
    {   test_dr1182_usb_uart, "USB_UART"        , "Test USB to UART3 Connectivity" },
    {   test_dr1182_reset   , "Reset"           , "Test Reset Button" },
    {   program_dongle      , "Program"         , "Program USB Dongle" },
#endif /* PROFILE_DR1182 */
};  


void vDisplayLink(tsTest *psTest, const char *pcDisplay, int iStage)
{
    printf("<a href=\"%s?Test=%s&Stage=%d\">%s</a></li>\n\r", pcCGISelf(&sCGI), psTest->pcLink, iStage, pcDisplay);
}

void vRefreshTimer(int iSeconds)
{
    printf("<script type=\"text/javascript\"> \n\
            function dorefresh(){ \n\
                window.location.reload() \n\
                if (refresh) setrefresh(); \n\
            } \n\
            function setrefresh(){ \n\
                setTimeout(\"dorefresh()\",%d) \n\
            } \n\
            window.onload=setrefresh; \n\
            </script>", iSeconds * 1000);
}

void vTestTimer(tsTest *psTest, int iStage, int iSeconds)
{
    printf("<script type=\"text/javascript\"> \n\
            function dotest(){ \n\
                RunTest('%s', %d); \n\
            } \n\
            window.onload=dotest; \n\
            </script>", psTest->pcLink, iStage);
}


int main(int argc, char **argv)
{
    const char *pcTest      = NULL;
    const char *pcContent   = NULL;
    int i;
    
    printf("Content-type: text/html\r\n\r\n");

    if (eCGIReadParameters(&sCGI) != E_CGI_OK)
    {
        printf("Error initialising CGI\n\r");
        
        vCGIDumpParameters(&sCGI, stdout);
        
        return -1;
    }
    
    /* Use the content flag to specify that inline content should be generated */
    pcContent = pcCGIGetVariable(&sCGI, "Content");
    
    /* Which test is being run? */
    pcTest = pcCGIGetVariable(&sCGI, "Test");

    if (!pcContent)
    {
        printf("<html><head>");
        printf("<title>Factory Test</title>");
        printf("<link rel=\"stylesheet\" href=\"/FactoryTest.css\" type=\"text/css\" media=\"screen\" />\n");
        
        /* Javascript to update content inline */
        printf("<script type=\"text/javascript\"> \n\
                function PaseAjaxResponse(somemixedcode) \n\
                { \n\
                    var source = somemixedcode; \n\
                    var scripts = new Array(); \n\
                    while(source.indexOf(\"<script\") > -1 || source.indexOf(\"</script\") > -1) { \n\
                        var s = source.indexOf(\"<script\"); \n\
                        var s_e = source.indexOf(\">\", s); \n\
                        var e = source.indexOf(\"</script\", s); \n\
                        var e_e = source.indexOf(\">\", e); \n\
                        scripts.push(source.substring(s_e+1, e)); \n\
                        source = source.substring(0, s) + source.substring(e_e+1); \n\
                    } \n\
                    for(var x=0; x<scripts.length; x++) { \n\
                        try { \n\
                            eval(scripts[x]); \n\
                        } \n\
                            catch(ex) { \n\
                        } \n\
                    } \n\
                    return source; \n\
                } \n\
                \n\
                function RunTest(Test, Stage) \n\
                { \n\
                    var XmlHttp; \n\
                    if (window.XMLHttpRequest) \n\
                    {// code for IE7+, Firefox, Chrome, Opera, Safari \n\
                        XmlHttp=new XMLHttpRequest(); \n\
                    } \n\
                    else \n\
                    {// code for IE6, IE5 \n\
                        XmlHttp=new ActiveXObject(\"Microsoft.XMLHTTP\"); \n\
                    } \n\
                    var request; \n\
                    var path = \"%s\"; \n\
                    request = \"Test=\" + Test \n\
                    request = request + \"&Stage=\" + Stage \n\
                    request = request + \"&Content=1\" \n\
                    \n\
                    XmlHttp.onreadystatechange=function() \n\
                    { \n\
                        if (XmlHttp.readyState==4 && XmlHttp.status==200) \n\
                        { \n\
                            clearTimeout(XmlHttpTimeout); \n\
                            document.getElementById(content).innerHTML=XmlHttp.responseText; \n\
                        } \n\
                    } \n\
                    XmlHttp.open(\"POST\",path,true); \n\
                    XmlHttp.setRequestHeader(\"Content-type\",\"application/x-www-form-urlencoded\"); \n\
                    XmlHttp.send(request); \n\
                    var XmlHttpTimeout=setTimeout(\"TestTimeout();\", %d); \n\
                    function TestTimeout(){ \n\
                        XmlHttp.abort(); \n\
                        document.getElementById(\"content\").innerHTML=\"Test timed out\" \n\
                    } \n\
                } \n\
                </script>\n\r", pcCGISelf(&sCGI), TEST_TIMEOUT*2);
        
        printf("</head><body>\n\r");

    
        /* Display menu */
        printf("<div id=\"menu\"><H2>Menu</H2>\n\r");
        for (i = 0; i < (sizeof(asTests)/sizeof(tsTest)); i++)
        {
            printf("<li><a href=\"%s?Test=%s\">%s</a></li>\n\r", pcCGISelf(&sCGI), asTests[i].pcLink, asTests[i].pcDisplay);
        }
        
        printf("</div><div id=\"content\">\n");
    }
    
    if (pcTest)
    {
        int iTestFound = 0;
        const char *pcStage     = NULL;
        int iStage = 0;
        
        pcStage = pcCGIGetVariable(&sCGI, "Stage");
        if (pcStage)
        {
            errno = 0;
            iStage = strtoul(pcStage, NULL, 0);
            if (errno)
            {
                iStage = 0;
            }
        } 
        
        for (i = 0; i < (sizeof(asTests)/sizeof(tsTest)); i++)
        {
            if (strcmp(pcTest, asTests[i].pcLink) == 0)
            {
                printf("<H2>%s</H2>\n\r", asTests[i].pcDisplay);

                asTests[i].prFunction(&asTests[i], iStage);
                iTestFound = 1;
            }
        }
        if (!iTestFound)
        {
            printf("Unknown test name '%s'\n\r", pcTest);    
        }
    }
    
    if (!pcContent)
    {
        printf("</div></body></html>");
    }
    return 0;
}



int test_dr1182_leds(tsTest *psTest, int iStage)
{
    printf("Test LEDS stage %d<BR><BR>\n\n", iStage);
    
    if (iStage == 1)
    {
        printf("<a href=\"javascript:RunTest('%s', %d, 'content');\">Test</a>\n\r", psTest->pcLink, iStage+1);
    }
    else if (iStage < 3)
    {
        printf("<a href=\"%s?Test=%s&Stage=%d\">%s</a></li>\n\r", pcCGISelf(&sCGI), psTest->pcLink, iStage+1, "Next");
    }
    else
    {
        printf("<a href=\"%s?Test=%s&Stage=%d\">%s</a></li>\n\r", pcCGISelf(&sCGI), (psTest+1)->pcLink, 0, "Next");
    }
    return 0;
}


int test_dr1182_buttons(tsTest *psTest, int iStage)
{
    switch (iStage)
    {
        case(0):
            printf("Testing button 1 - Please press button<BR>\n\r");
            vTestTimer(psTest, iStage+1, 1);
            break;
        
        case (1):
        {
            struct stat sStat;
            int iTime = 0;
            
            /* Allow 5 seconds for tester to press button */
            while (iTime < TEST_TIMEOUT)
            {
                if (stat("/tmp/test", &sStat) == 0)
                {
                    printf("Stat OK\n");
                    printf("<a href=\"%s?Test=%s&Stage=%d\">%s</a></li>\n\r", pcCGISelf(&sCGI), psTest->pcLink, iStage+1, "Next");
                    break;
                }
                else
                {
                    sleep(1);
                    iTime += 1;
                }
            }
            if (iTime == TEST_TIMEOUT)
            {
                printf("Test failed <a href=\"javascript:RunTest('%s', %d, 'content');\">Retry</a>\n\r", psTest->pcLink, iStage);
            }
            break;
        }
    }
            
    return 0;
}


int configure_usb(tsTest *psTest, int iStage)
{
    const char *pcDefaultManufacturer   = "NXP";
    const char *pcDefaultDescription    = "USB Dongle";
    const char *pcDefaultAutoSerial     = "NX";
    int i;
    
    
    switch (iStage)
    {
        case(0):
            printf("<form method=\"POST\" enctype=\"multipart/form-data\" action=\"%s\">\n", pcCGISelf(&sCGI));
            printf("<input type=\"hidden\" name=\"Test\" value=\"%s\"> \n", psTest->pcLink);
            printf("<input type=\"hidden\" name=\"Stage\" value=\"%d\"> \n", iStage+1);
            //printf("<input type=\"hidden\" name=\"Content\" value=\"1\"> \n");
            
            printf("<table class=\"USB_Dongle_Settings\">");
            printf("<tr><td>Manufacturer:               </td><td><input type=\"text\" name=\"manufacturer\" value=\"%s\"></td></tr>\n", pcDefaultManufacturer);
            printf("<tr><td>Product Description String: </td><td><input type=\"text\" name=\"description\" value=\"%s\"></td></tr>\n", pcDefaultDescription);
            printf("<tr><td>Serial Number:              </td><td><input type=\"text\" name=\"serial\"></td></tr>\n");
            printf("<tr><td>Auto Serial:                </td><td><input type=\"text\" name=\"autoserial\" value=\"%s\"></td></tr>\n", pcDefaultAutoSerial);
            
            
            for (i = 0; i < 5; i++)
            {
                /* IO Controls */
                printf("<tr><td>IO Control %d:          </td><td><select name=\"c%d\">n", i, i);
                
                printf("<option value=\"\"          %s></option>\n",        "");
                printf("<option value=\"TXDEN\"     %s>TXDEN</option>\n",   "");
                printf("<option value=\"PWREN\"     %s>PWREN</option>\n",   "");
                printf("<option value=\"RXLED\"     %s>RXLED</option>\n",   i == 1 ? "selected" : "");
                printf("<option value=\"TXLED\"     %s>TXLED</option>\n",   i == 0 ? "selected" : "");
                printf("<option value=\"TXRXLED\"   %s>TXRXLED</option>\n", "");
                printf("<option value=\"SLEEP\"     %s>SLEEP</option>\n",   i == 4 ? "selected" : "");
                printf("<option value=\"CLK48\"     %s>CLK48</option>\n",   "");
                printf("<option value=\"CLK24\"     %s>CLK24</option>\n",   "");
                printf("<option value=\"CLK12\"     %s>CLK12</option>\n",   "");
                printf("<option value=\"CLK6\"      %s>CLK6</option>\n",    "");
                printf("<option value=\"IOMODE\"    %s>IOMODE</option>\n",  (i == 2 || i == 3) ? "selected" : "");
                printf("<option value=\"BB_WR\"     %s>BB_WR</option>\n",   "");
                printf("<option value=\"BB_RD\"     %s>BB_RD</option>\n",   "");
                printf("</select></td></tr>\n");
            }
            
            printf("<tr><td colspan=\"2\"><center><input type=\"submit\" value=\"Program\"></center></td></tr></table>");
            printf("</form>\n");
            break;
            
        case(1):
        {
            char *pcManufacturer    = pcCGIGetVariable(&sCGI, "manufacturer");
            char *pcDescription     = pcCGIGetVariable(&sCGI, "description");
            char *pcSerial          = pcCGIGetVariable(&sCGI, "serial");
            char *pcAutoSerial      = pcCGIGetVariable(&sCGI, "autoserial");
            char *pcC0              = pcCGIGetVariable(&sCGI, "c0");
            char *pcC1              = pcCGIGetVariable(&sCGI, "c1");
            char *pcC2              = pcCGIGetVariable(&sCGI, "c2");
            char *pcC3              = pcCGIGetVariable(&sCGI, "c3");
            char *pcC4              = pcCGIGetVariable(&sCGI, "c4");
            char acCommandLine[1024] = "FTProg";

            printf("Configuring USB Dongle");
#define SET_OPTION(option, description, value) \
            if (option) \
            { \
                if (strlen(value)) \
                { \
                    char acTmp[1024]; \
                    printf("Setting %s: %s<BR>\n", description, value); \
                    sprintf(acTmp, " --%s=\"%s\"", option, value); \
                    strcat(acCommandLine, acTmp); \
                } \
            }
            
            SET_OPTION("manufacturer",  "Manufacturer String",              pcManufacturer);
            SET_OPTION("description",   "Product Description String",       pcDescription);
            SET_OPTION("serial",        "Serial Number String",             pcSerial);
            SET_OPTION("autoserial",    "Automatic serial number prefix",   pcAutoSerial);
            SET_OPTION("C0",            "IO Control 0",                     pcC0);
            SET_OPTION("C1",            "IO Control 1",                     pcC1);
            SET_OPTION("C2",            "IO Control 2",                     pcC2);
            SET_OPTION("C3",            "IO Control 3",                     pcC3);
            SET_OPTION("C4",            "IO Control 4",                     pcC4);


            printf("Exectuing command line: <BR><PRE>%s</PRE><BR>\n", acCommandLine);
            
            FILE *fp;
            char line[1024];
            fp = popen(acCommandLine, "r");
            while (fgets(line, sizeof line, fp))
            {
                // Convert output to HTML form
                printf("%s<BR>", line);
                fflush(stdout);
            }
            pclose(fp);
            break;
        }
    }
    return 0;
}

int test_dr1182_usb_uart(tsTest *psTest, int iStage)
{
    
    return 0;
}

int test_dr1182_reset(tsTest *psTest, int iStage)
{
    
    return 0;
}



int program_dongle  (tsTest *psTest, int iStage)
{
    switch (iStage)
    {
        case(0):
            printf("<form method=\"POST\" enctype=\"multipart/form-data\" action=\"%s\">\n", pcCGISelf(&sCGI));
            printf("<input type=\"hidden\" name=\"Test\" value=\"%s\"> \n", psTest->pcLink);
            printf("<input type=\"hidden\" name=\"Stage\" value=\"%d\"> \n", iStage+1);
            printf("<input type=\"hidden\" name=\"Content\" value=\"1\"> \n");
            
            printf("<input type=\"file\" name=\"firmware\"> \n");
            printf("<input type=\"text\" name=\"test_text\">\n");
            printf("<input type=\"submit\" value=\"Program\">");
            printf("</form>\n");
            break;
            
        case(1):
        {
            tsCGIParam* psFirmwareFile = psCGIGetParameter(&sCGI, "firmware");
            if (psFirmwareFile)
            {
                printf("Programming file '%s'", psFirmwareFile->uParam.sFile.pcFileName);
                printf("<div id=\"progress\"></div>");
                printf("<script type=\"text/javascript\"> \n\
                        alert(\"Hello world!\")</script>");
            }
            break;
        }
    }
    return 0;
}


