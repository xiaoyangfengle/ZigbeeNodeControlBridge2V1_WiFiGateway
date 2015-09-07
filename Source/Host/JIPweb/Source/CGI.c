/****************************************************************************
 *
 * MODULE:             JIP Web Apps
 *
 * COMPONENT:         CGI Driver
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <CGI.h>

//#define DEBUG_CGI

#ifdef DEBUG_CGI
#define PRINTF(...) printf("DBG:" __VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG_CGI */


teCGIStatus eCGIReadVariables(tsCGI *psCGI)
{    
    const char *pcContentType     = NULL;
    const char *pcRequestMethod   = NULL;
    const char *pcContentLength   = NULL;
    char *pcInputPairs      = NULL;
    
    /* Initialise variable list */
    psCGI->iNumVars = 0;
    psCGI->asVars   = NULL;
    
    /* For debug */
    PRINTF("Content-type: text/html\r\n\r\n");

    pcContentType = getenv("CONTENT_TYPE");
    if (pcContentType)
    {
        PRINTF("CONTENT_TYPE: %s\n\r", pcContentType);
        
        if (strstr(pcContentType, "multipart/form-data"))
        {
            printf("Cannot handle multipart/form data\n\r");
            return E_CGI_ERROR;
        }
    }
    
    pcRequestMethod = getenv("REQUEST_METHOD");
    PRINTF("REQUEST_METHOD: %s\n\r", pcRequestMethod);
    
    pcContentLength = getenv("CONTENT_LENGTH");
    PRINTF("CONTENT_LENGTH: %s\n\r", pcContentLength);
    
    if (strcmp(pcRequestMethod, "GET") == 0)
    {
        char *pcQueryString = getenv("QUERY_STRING");
        PRINTF("QUERY_STRING: %s\n\r", pcQueryString);
        
        if (pcQueryString)
        {
            /* Input provided as QUERY_STRING env variable */
            pcInputPairs = strdup(pcQueryString);
            if (!pcInputPairs)
            {
                return E_CGI_MEM_ERROR;
            }
        }
    }
    else if (strcmp(pcRequestMethod, "POST") == 0)
    {
        /* Input provided as line on stdin */
        if (pcContentLength)
        {
            int iLength = strtoul(pcContentLength, NULL, 10);
            
            if (iLength > 0)
            {
                pcInputPairs = malloc(iLength+1);
                if (!pcInputPairs)
                {
                    return E_CGI_MEM_ERROR;
                }
                
                if (fgets(pcInputPairs, iLength+1, stdin) == NULL)
                {
                    return E_CGI_INVALID_PARAMS;
                }
            }
        }
        else
        {
            return E_CGI_INVALID_PARAMS;
        }
        
    }
    else
    {
        /* No request method? */
        return E_CGI_INVALID_PARAMS;
    }
    
    PRINTF("INPUT: %s\n\r", pcInputPairs);
    
    if (pcInputPairs)
    {
        char *pcInputPair = pcInputPairs;
        
        while (*pcInputPair)
        {
            char *pcNextInputPair;
            char *pcValue;
            
            /* Find next name=value pair */
            pcNextInputPair = strchr(pcInputPair, '&');
            if (pcNextInputPair)
            {
                *pcNextInputPair = '\0';
            }
            else
            {
                pcNextInputPair = strchr(pcInputPair, ';');
                if (pcNextInputPair)
                {
                    *pcNextInputPair = '\0';
                }
                else
                {
                    /* Last pair - point next input pair to end of the pair */
                    pcNextInputPair = pcInputPair + strlen(pcInputPair);
                }
            }
            
            pcValue = strchr(pcInputPair, '=');
            if (!pcValue)
            {
                /* No '=' is string - malformed pair */
                goto next_ip;
            }
            
            if (!strlen(pcValue+1))
            {
                /* Empty value - malformed pair */
                goto next_ip;
            }
            
            {
                char *pcVarName;
                char *pcVarValue;
                
                pcVarValue = strdup(pcValue+1);
                if (!pcVarValue)
                {
                    return E_CGI_MEM_ERROR;
                }
                *pcValue = '\0';
                
                pcVarName = strdup(pcInputPair);
                if (!pcVarName)
                {
                    free(pcVarValue);
                    return E_CGI_MEM_ERROR;
                }
                PRINTF("Got var: '%s' = '%s'\n\r", pcVarName, pcVarValue);
                {
                    /* Insert into array of variables */
                    tsCGIVar *psNewCGIVars;
                    
                    psCGI->iNumVars++;
                    psNewCGIVars = realloc(psCGI->asVars, sizeof(tsCGIVar) * psCGI->iNumVars);
                    if (!psNewCGIVars)
                    {
                        free(pcVarName);
                        free(pcVarValue);
                        return E_CGI_MEM_ERROR;
                    }
                    psCGI->asVars = psNewCGIVars;
                    
                    if ((eCGIURLDecode(pcVarName)    != E_CGI_OK) ||
                        (eCGIURLDecode(pcVarValue)   != E_CGI_OK))
                    {
                        free(pcVarName);
                        free(pcVarValue);
                        return E_CGI_MEM_ERROR;
                    }
                    
                    psCGI->asVars[psCGI->iNumVars-1].pcName     = pcVarName;
                    psCGI->asVars[psCGI->iNumVars-1].pcValue    = pcVarValue;
                }
            }
next_ip:
            /* Move on to next pair - skip the NULL we inserted into the string */
            pcInputPair = pcNextInputPair+1;
        }
    }
    
    /* Free buffer */
    free(pcInputPairs);
    
    return E_CGI_OK;
}


char* pcCGIGetValue(tsCGI *psCGI, const char *pcVarName)
{
    int i;
    
    for (i = 0; i < psCGI->iNumVars; i++)
    {
        if (strcmp(pcVarName, psCGI->asVars[i].pcName) == 0)
        {
            PRINTF("Found var '%s' = '%s'\n\r", psCGI->asVars[i].pcName, psCGI->asVars[i].pcValue);
            return psCGI->asVars[i].pcValue;
        }
    }
    return NULL;
}


teCGIStatus eCGIURLEncode(char **ppcOutput, const char *pcInput)
{
    char *pcOutputPos;
    int iLength;
    
    /* Malloc a buffer double the length of the input to start off with. */
    iLength = sizeof(char) * strlen(pcInput) * 2;
    *ppcOutput = malloc(iLength);
    
    memset(*ppcOutput, 0, iLength);
    
    pcOutputPos = *ppcOutput;
    
    while (*pcInput)
    {
        if (strchr(";/?:@&=", *pcInput))
        {
            /* Escape this */
            PRINTF("Escaping: '%c'\n", *pcInput);
            pcOutputPos += sprintf(pcOutputPos, "%%%02X", *pcInput);
        }
        else
        {
            *pcOutputPos = *pcInput;
            pcOutputPos++;
        }
        
        pcInput++;
    }
    return E_CGI_OK;
}


teCGIStatus eCGIURLDecode (char *pcInput)
{
    char *pcOutput = pcInput;
    
    PRINTF("ESCAPED STRING: %s\n", pcInput);
    
    while (*pcInput)
    {
        if (*pcInput == '%')
        {
            char acValue[3] = { 0,0,0 };
            int iCharValue;
            
            strncpy(acValue, pcInput+1, 2);
            errno = 0;
            iCharValue = strtoul(acValue, NULL, 16);
            
            if (!errno)
            {
                PRINTF("Unescaped value: 0x%s = '%c'\n", acValue, iCharValue);
                
                *pcOutput = (char)iCharValue;
                pcOutput++;
                pcInput += 2;
            }
            else
            {
                PRINTF("Could not unescape value: 0x%s\n", acValue);
            }
        } 
        else
        {
            /* Unescaped - just copy the input */
            *(pcOutput++) = *pcInput;
        }
        pcInput++;
    }
    /* NULL Terminate output */
    *pcOutput = '\0';
    
    return E_CGI_OK;
}

