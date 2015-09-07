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
#include <unistd.h>

#include <CGI.h>

//#define DEBUG_CGI

#ifdef DEBUG_CGI
#define PRINTF(...) printf("DBG:" __VA_ARGS__); fflush(stdout)
#else
#define PRINTF(...)
#endif /* DEBUG_CGI */


static teCGIStatus eCGIAddVar(tsCGI *psCGI, char *pcName, char *pcValue);
static teCGIStatus eCGIAddFile(tsCGI *psCGI, char *pcName, char *pcFileName, char *pcTmpFile, char *pcMimeType);
static teCGIStatus eCGIReadLine(FILE *pfStream, char **ppcLine);
static teCGIStatus eCGIReadMultipartFormData(tsCGI *psCGI, char *pcFieldSeparator);



teCGIStatus eCGIReadParameters(tsCGI *psCGI)
{    
    const char *pcContentType     = NULL;
    const char *pcRequestMethod   = NULL;
    const char *pcContentLength   = NULL;
    char *pcInputPairs      = NULL;
    
    /* Initialise variable list */
    psCGI->iNumParams   = 0;
    psCGI->asParams     = NULL;
    
    /* For debug */
    PRINTF("Content-type: text/html\r\n\r\n");

    pcContentType = getenv("CONTENT_TYPE");
    if (pcContentType)
    {
        PRINTF("CONTENT_TYPE: %s\n\r", pcContentType);
        
        if (strstr(pcContentType, "multipart/form-data"))
        {
            char *pcFieldSeparator;
            pcFieldSeparator = strstr(pcContentType, "boundary=");
            if (pcFieldSeparator)
            {
                pcFieldSeparator += strlen("boundary=");
                PRINTF("Multipart form data with boundary: %s\n", pcFieldSeparator);
                return eCGIReadMultipartFormData(psCGI, pcFieldSeparator);
            }
            else
            {
                return E_CGI_INVALID_PARAMS;
            }
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
                    teCGIStatus eStatus;
                    if ((eStatus = eCGIAddVar(psCGI, pcVarName, pcVarValue)) != E_CGI_OK)
                    {
                        free(pcVarName);
                        free(pcVarValue);
                        return eStatus;
                    }
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


tsCGIParam* psCGIGetParameter(tsCGI *psCGI, const char *pcParamName)
{
    int i;
    
    for (i = 0; i < psCGI->iNumParams; i++)
    {
        if (strcmp(pcParamName, psCGI->asParams[i].pcName) == 0)
        {
            PRINTF("Found parameter '%s'\n\r", psCGI->asParams[i].pcName);
            return &psCGI->asParams[i];
        }
    }
    return NULL;
}


void vCGIDumpParameters(tsCGI *psCGI, FILE *pfStream)
{
    int i;
    
    fprintf(pfStream, "Dump CGI parameters\n");
    
    for (i = 0; i < psCGI->iNumParams; i++)
    {
        switch (psCGI->asParams[i].eType)
        {
            case (E_CGI_VARIABLE):
                fprintf(pfStream, "Parameter '%s' = '%s'\n\r", psCGI->asParams[i].pcName, psCGI->asParams[i].uParam.sVar.pcValue);
                break;
                
            case (E_CGI_FILE):
                fprintf(pfStream, "Parameter '%s' = File. Original Filename = '%s' Stored Filename = '%s' Mime Type = '%s'\n\r", 
                        psCGI->asParams[i].pcName, psCGI->asParams[i].uParam.sFile.pcFileName, 
                        psCGI->asParams[i].uParam.sFile.pcTmpFile, psCGI->asParams[i].uParam.sFile.pcMimeType 
                       );
                break;
            
            default:
                fprintf(pfStream, "Parameter '%s' = Unknown type(%d)\n\r", psCGI->asParams[i].pcName, psCGI->asParams[i].eType);
        }
    }

    return;
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


char *pcCGISelf(tsCGI *psCGI)
{
#if 0
    char *pcOutput = NULL;
    char *pcPath    = getenv("PATH_INFO");
    char *pcScript  = getenv("SCRIPT_NAME");
    
    printf("pcPath: %s\n\r", pcPath);
    printf("pcScript: %s\n\r", pcScript);
    
    if (pcPath && pcScript)
    {
        pcOutput = malloc(strlen(pcPath) + strlen(pcScript));
        if (pcOutput)
        {
            strcpy(pcOutput, pcPath);
            strcat(pcOutput, pcScript);
        }
    }
#endif
    return getenv("SCRIPT_NAME");
}


static teCGIStatus eCGIReadLine(FILE *pfStream, char **ppcLine)
{
#define CHUNK_SIZE 128
    char   *pcLine = NULL;
    int     iLinePos = 0;
    size_t  iSize = 0;

    *ppcLine = NULL;
    
    pcLine = malloc(CHUNK_SIZE);
    if (!pcLine)
    {
        return E_CGI_MEM_ERROR;
    }
    iSize = CHUNK_SIZE;
    
    while (!feof(pfStream))
    {
        if (fgets (&pcLine[iLinePos], iSize-iLinePos-1, pfStream) == NULL)
        {
            PRINTF("No more lines.");
            free(pcLine);
            return E_CGI_OK;
        }
        if (strlen(pcLine) == CHUNK_SIZE)
        {
            char *pcNewLine = realloc(pcLine, iSize + CHUNK_SIZE);
            if (!pcNewLine)
            {
                return E_CGI_MEM_ERROR;
            }
            pcLine = pcNewLine;
            iLinePos    += CHUNK_SIZE-1;
            iSize       += CHUNK_SIZE;
        }
        else
        {
            /* Read a whole line? */
            int iLineLength = strlen(pcLine)-2;
            if (strncmp(&pcLine[iLineLength], "\r\n", 2) == 0)
            {
                memset(&pcLine[iLineLength], 0, 2);
                PRINTF("Got line: %s\n", pcLine);
                *ppcLine = pcLine;
                return E_CGI_OK;
            }
            else
            {
                return E_CGI_ERROR;
            }
        }
    }
#undef CHUNK_SIZE
    PRINTF("EOF.");
    free(pcLine);
    return E_CGI_ERROR;
}


static teCGIStatus eCGIReadFileData(tsCGI *psCGI, FILE *pfStream, char *pcFieldSeparator, char *pcName, char *pcFileName, char *pcMimeType)
{
    char *pcEOFMarker;
    int iMatchingByteCount = 0;
    int iFileDes;
    char acFileNameTemplate[] = "/tmp/cgi_upload_XXXXXX";
    teCGIStatus eStatus = E_CGI_ERROR;
    
    if ((iFileDes = mkstemp(acFileNameTemplate)) == -1)
    {
        return E_CGI_ERROR;
    }
    
    pcEOFMarker = malloc(strlen(pcFieldSeparator) + 5);
    if (!pcEOFMarker)
    {
        return E_CGI_MEM_ERROR;
    }
    
    sprintf(pcEOFMarker, "\r\n--%s", pcFieldSeparator);
    
    PRINTF("Looking for EOF Marker: %s\n", pcEOFMarker);

    while (!feof(pfStream))
    {
        char c = fgetc (pfStream);
        if (c == pcEOFMarker[iMatchingByteCount])
        {
            iMatchingByteCount++;

            if (iMatchingByteCount == strlen(pcEOFMarker))
            {
                eStatus = E_CGI_OK;
                goto done;
            }
        }
        else
        {
            if (iMatchingByteCount)
            {
                /* Flush matched characters */
                if (write(iFileDes, pcEOFMarker, iMatchingByteCount) < 0)
                {
                    PRINTF("Error writing to file\n");
                    goto done;
                }
                iMatchingByteCount = 0;
            }
            /* Flush non matching character */
            if (write(iFileDes, &c, 1) < 0)
            {
                PRINTF("Error writing to file\n");
                goto done;
            }
        }
    }
done:
    if (eStatus == E_CGI_OK)
    {
        eStatus = eCGIAddFile(psCGI, pcName, pcFileName, acFileNameTemplate, pcMimeType);
    }
    close(iFileDes);
    free(pcEOFMarker);
    return eStatus;
}


static teCGIStatus eCGIReadMultipartFormData(tsCGI *psCGI, char *pcFieldSeparator)
{
    int     iHeader         = 0;
    char   *pcLine          = NULL;    
    char   *pcName          = NULL;   
    char   *pcFileName      = NULL;
    char   *pcContentType   = NULL;
    teCGIStatus eStatus;
    
    while (eCGIReadLine(stdin, &pcLine) == E_CGI_OK)
    {
        if (!pcLine)
        {
            /* Last line read */
            PRINTF("Read Last Input\n");
            return E_CGI_OK;
        }
        
        PRINTF("Read line: '%s'\n", pcLine);
        
        if (strncmp(&pcLine[strlen(pcLine)-2], "--", 2) == 0)
        {
            PRINTF("Last input line\n");
            free(pcLine);
            return E_CGI_OK;
        }
        
        else if (strncmp (&pcLine[2], pcFieldSeparator, strlen(pcFieldSeparator)) == 0) // Skip initial "--" on line
        {
            PRINTF("Boundary: %s\n", pcFieldSeparator);            
            iHeader = 1;
            if (pcName)
            {
                free(pcName);
                pcName = NULL;
            }
            if (pcFileName)
            {
                free(pcFileName);
                pcFileName = NULL;
            }
            if (pcContentType)
            {
                free(pcContentType);
                pcContentType = NULL;
            }
        }
        else if (strncmp (pcLine, "Content-Disposition: form-data; ", strlen("Content-Disposition: form-data; ")) == 0)
        {
            if (strstr(pcLine, "name="))
            {
                char *pcNameStart = strstr(pcLine, "name=\"") + strlen("name=\"");
                char *pcNameEnd   = strstr(pcNameStart+1, "\"");
                pcName = strndup(pcNameStart, pcNameEnd -pcNameStart);
                PRINTF("Field name: %s\n", pcName);
            }
            
            if (strstr(pcLine, "filename="))
            {
                char *pcFileNameStart = strstr(pcLine, "filename=\"") + strlen("filename=\"");
                char *pcFileNameEnd   = strstr(pcFileNameStart+1, "\"");
                pcFileName = strndup(pcFileNameStart, pcFileNameEnd - pcFileNameStart);
                PRINTF("Filename: %s\n", pcFileName);
            }
        }
        else if (strncmp (pcLine, "Content-Type:", strlen("Content-Type:")) == 0)
        {
            pcContentType = strdup(pcLine + strlen("Content-Type: "));
            PRINTF("Content Type: %s\n", pcContentType);
        }
        else if(iHeader)
        {
            if (strlen(pcLine) == 0)
            {
                /* Empty Line separates header from content */
                PRINTF("End Header\n");
                iHeader = 0;
                
                if (pcFileName)
                {
                    /* Read in the file content */
                    PRINTF("Read file\n");
                    
                    if ((eStatus = eCGIReadFileData(psCGI, stdin, pcFieldSeparator, pcName, pcFileName, pcContentType)) != E_CGI_OK)
                    {
                        free(pcName);
                        free(pcLine);
                        return eStatus;
                    }
                    free(pcName);
                    free(pcFileName);
                    free(pcContentType);
                    pcName = pcFileName = pcContentType = NULL;
                }
            }
        }
        else
        {
            if (pcName)
            {
                /* Content Line */
                PRINTF("Value: %s\n", pcLine);
                
                if ((eStatus = eCGIAddVar(psCGI, pcName, pcLine)) != E_CGI_OK)
                {
                    free(pcName);
                    free(pcLine);
                    return eStatus;
                }
            }
        }
        free(pcLine);
    }
    return E_CGI_ERROR;
}


static teCGIStatus eCGIAddVar(tsCGI *psCGI, char *pcName, char *pcValue)
{
    /* Insert into array of variables */
    tsCGIParam *psNewCGIParams;
    
    psNewCGIParams = realloc(psCGI->asParams, sizeof(tsCGIParam) * (psCGI->iNumParams + 1));
    if (!psNewCGIParams)
    {
        return E_CGI_MEM_ERROR;
    }
    psCGI->asParams = psNewCGIParams;
    
    if ((eCGIURLDecode(pcName)   != E_CGI_OK) ||
        (eCGIURLDecode(pcValue)  != E_CGI_OK))
    {
        return E_CGI_MEM_ERROR;
    }
    
    psCGI->asParams[psCGI->iNumParams].eType                    = E_CGI_VARIABLE;
    psCGI->asParams[psCGI->iNumParams].pcName                   = strdup(pcName);
    psCGI->asParams[psCGI->iNumParams].uParam.sVar.pcValue      = strdup(pcValue);
    psCGI->iNumParams++;
    return E_CGI_OK;
}


static teCGIStatus eCGIAddFile(tsCGI *psCGI, char *pcName, char *pcFileName, char *pcTmpFile, char *pcMimeType)
{
    /* Insert into array of variables */
    tsCGIParam *psNewCGIParams;
    
    psNewCGIParams = realloc(psCGI->asParams, sizeof(tsCGIParam) * (psCGI->iNumParams + 1));
    if (!psNewCGIParams)
    {
        return E_CGI_MEM_ERROR;
    }
    psCGI->asParams = psNewCGIParams;
    
    if ((eCGIURLDecode(pcName)      != E_CGI_OK) ||
        (eCGIURLDecode(pcFileName)  != E_CGI_OK))
    {
        return E_CGI_MEM_ERROR;
    }
    
    psCGI->asParams[psCGI->iNumParams].eType                    = E_CGI_FILE;
    psCGI->asParams[psCGI->iNumParams].pcName                   = strdup(pcName);
    psCGI->asParams[psCGI->iNumParams].uParam.sFile.pcFileName  = strdup(pcFileName);
    psCGI->asParams[psCGI->iNumParams].uParam.sFile.pcTmpFile   = strdup(pcTmpFile);
    psCGI->asParams[psCGI->iNumParams].uParam.sFile.pcMimeType  = strdup(pcMimeType);
    psCGI->iNumParams++;
    return E_CGI_OK;
}

