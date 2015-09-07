/****************************************************************************
 *
 * MODULE:             JIP Web Apps
 *
 * COMPONENT:          JIP cgi program
 *
 * REVISION:           $Revision: 62880 $
 *
 * DATED:              $Date: 2014-07-23 15:54:37 +0100 (Wed, 23 Jul 2014) $
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
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <json.h>

#include <Zeroconf.h> 
#include <JIP.h>

#include "CGI.h"

#define DISPLAY_JENNET_MIB


#define CACHE_DEFINITIONS_FILE_NAME "/tmp/jip_cache_definitions.xml"
#define CACHE_NETWORK_FILE_NAME "/tmp/jip_cache_network.xml"


static int verbosity = 0;

#ifndef VERSION
#error Version is not defined!
#else
const char *Version = "1.0 (r" VERSION ")";
#endif


/** Structure representing command result */
typedef struct
{
    int         iValue;         /**< Integer value */
    const char *pcDescription;  /**< Text description of result */
} tsResult;

/** Macro to set the command result structure */
#define SET_RESULT(a, b) sResult.iValue = a; sResult.pcDescription = b

typedef struct 
{
    const char *pcAction;
    const char *pcUpdateValue;
    tsResult    sResult;
} tsVarAction;


/* Filter variables */
static char *filter_ipv6 = NULL;
static char *filter_device = NULL;
static char *filter_mib = NULL;
static char *filter_var = NULL;

/* Callback function types from \ref jip_iterate */
typedef int(*tprCbNode) (tsNode *psNode, void *pvUser);
typedef int(*tprCbMib)  (tsMib *psMib, void *pvUser);
typedef int(*tprCbVar)  (tsVar *psVar, void *pvUser);

static int jip_iterate(tprCbNode prCbNode,   void *pvNodeUser, 
                        tprCbMib prCbMib,     void *pvMibUser, 
                        tprCbVar prCbVar,     void *pvVarUser);

static tsJIP_Context sJIP_Context;

static tsCGI sCGI;

static uint32_t u32Flags = E_JIP_FLAG_NONE;


/** @{ Command handlers */
static tsResult cmd_getVersion(struct json_object* psResult);
static tsResult cmd_discoverBRs(struct json_object* psResult);
static tsResult cmd_discoverNetwork(struct json_object* psResult);
static tsResult cmd_getVar(struct json_object* psResult);
static tsResult cmd_setVar(char *pcUpdateValue);

/** @} */

int main(int argc, char *argv[])
{
    char *pcAction                          = NULL;
    char *pcBRNAddress                      = NULL;
    char *pcNodeAddress                     = NULL;
    char *pcMulticastAddress                = NULL;
    char *pcMibId                           = NULL;
    char *pcVarIndex                        = NULL;
    char *pcRefreshNodes                    = NULL;
    char *pcStayAwake                       = NULL;
    char *pcUpdateValue                     = NULL;
    teJIP_Status eStatus;
    
    tsResult sResult;
    int iCacheUpdate = 0;
    
    struct json_object* psJsonResult        = NULL;
    struct json_object* psJsonNetwork       = NULL;
    
    struct json_object* psJsonStatus        = NULL;
    struct json_object* psJsonStatusInt     = NULL;
    struct json_object* psJsonStatusText    = NULL;
    
#define SET_STATUS(i, t) \
        psJsonStatusInt     = json_object_new_int(i); \
        psJsonStatusText    = json_object_new_string(t); \

#define EXIT_STATUS(i, t) \
        SET_STATUS(i, t)  \
        goto end;
        
    psJsonResult = json_object_new_object();
    psJsonStatus = json_object_new_object();    
    
    printf("Content-type: application/json\r\n\r\n");
    
    if (eCGIReadVariables(&sCGI) != E_CGI_OK)
    {
        printf("Error initialising CGI\n\r");
        return -1;
    }
    
    pcAction = pcCGIGetValue(&sCGI, "action");
    if (!pcAction)
    {
        EXIT_STATUS(E_CGI_ERROR, "Unknown Request");
    }

    pcBRNAddress        = pcCGIGetValue(&sCGI, "BRaddress");
    pcNodeAddress       = pcCGIGetValue(&sCGI, "nodeaddress");
    pcMibId             = pcCGIGetValue(&sCGI, "mib");
    pcVarIndex          = pcCGIGetValue(&sCGI, "var");
    pcRefreshNodes      = pcCGIGetValue(&sCGI, "refresh");
    pcStayAwake         = pcCGIGetValue(&sCGI, "stayawake");
    pcUpdateValue       = pcCGIGetValue(&sCGI, "value");
    
    if (pcRefreshNodes == NULL)
    {
        pcRefreshNodes = "yes";
    }
    
    if (pcStayAwake)
    {
        if (strcmp(pcStayAwake, "yes") == 0)
        {
            u32Flags = E_JIP_FLAG_STAY_AWAKE;
        }
    }
    
    if (strcasecmp(pcAction, "getVersion") == 0)
    {
        sResult = cmd_getVersion(psJsonResult);
        EXIT_STATUS(sResult.iValue, sResult.pcDescription);
    }
    else if (strcasecmp(pcAction, "discoverBRs") == 0)
    {
        sResult = cmd_discoverBRs(psJsonResult);
        EXIT_STATUS(sResult.iValue, sResult.pcDescription);
    }

    if (pcBRNAddress == NULL)
    {
        EXIT_STATUS(E_CGI_ERROR, "No BR Specified");
    }

    if (eStatus = eJIP_Init(&sJIP_Context, E_JIP_CONTEXT_CLIENT) != E_JIP_OK)
    {
        EXIT_STATUS(eStatus, "JIP startup failed");
    }

    if (eStatus = eJIP_Connect(&sJIP_Context, pcBRNAddress, JIP_DEFAULT_PORT) != E_JIP_OK)
    {
        EXIT_STATUS(eStatus, "JIP connect failed");
    }
    
    /* Load the cached device id's and any network contents if possible */
    if (eJIPService_PersistXMLLoadDefinitions(&sJIP_Context, CACHE_DEFINITIONS_FILE_NAME) != E_JIP_OK)
    {
        // Couldn't load the definitions file, fall back to discovery.
        if ((eStatus = eJIPService_DiscoverNetwork(&sJIP_Context)) != E_JIP_OK)
        {
            EXIT_STATUS(eStatus, "JIP discover network failed");
        }
        iCacheUpdate = 1;
    }
    else
    {
        if (strcmp(pcRefreshNodes, "yes") == 0)
        {
            if (eStatus = eJIPService_DiscoverNetwork(&sJIP_Context) != E_JIP_OK)
            {
                EXIT_STATUS(eStatus, "JIP discover network failed");
            }
            iCacheUpdate = 1;
        }
        else
        {
            /* Load the cached network if possible */
            if (eJIPService_PersistXMLLoadNetwork(&sJIP_Context, CACHE_NETWORK_FILE_NAME) != E_JIP_OK)
            {
                // Couldn't load the network file, fall back to discovery.
                if ((eStatus = eJIPService_DiscoverNetwork(&sJIP_Context)) != E_JIP_OK)
                {
                    EXIT_STATUS(eStatus, "JIP discover network failed");
                }
                iCacheUpdate = 1;
            }
        }
    }
    
    //eJIP_PrintNetworkContent(&sJIP_Context);
    
    if (strcasecmp(pcAction, "discover") == 0)
    {
        psJsonNetwork = json_object_new_object();
        sResult = cmd_discoverNetwork(psJsonNetwork);
        SET_STATUS(sResult.iValue, sResult.pcDescription);
    }
    else if (strcasecmp(pcAction, "GetVar") == 0)
    {
        filter_ipv6 = pcNodeAddress;
        filter_mib = pcMibId;
        filter_var = pcVarIndex;
        
        psJsonNetwork = json_object_new_object();
        sResult = cmd_getVar(psJsonNetwork);
        SET_STATUS(sResult.iValue, sResult.pcDescription);
    }
    else if (strcasecmp(pcAction, "SetVar") == 0)
    {
        filter_ipv6 = pcNodeAddress;
        filter_mib = pcMibId;
        filter_var = pcVarIndex;

        sResult = cmd_setVar(pcUpdateValue);
        SET_STATUS(sResult.iValue, sResult.pcDescription);
    }
    else
    {
        SET_STATUS(E_JIP_ERROR_FAILED, "Unknown action");
    }

    if ((sResult.iValue == E_JIP_OK) && (iCacheUpdate))
    {
        (void)eJIPService_PersistXMLSaveDefinitions(&sJIP_Context, CACHE_DEFINITIONS_FILE_NAME);
        (void)eJIPService_PersistXMLSaveNetwork(&sJIP_Context, CACHE_NETWORK_FILE_NAME);
    }
    
end:
    json_object_object_add (psJsonResult,
                            "Status",
                            psJsonStatus);

    json_object_object_add (psJsonStatus,
                            "Value",
                            psJsonStatusInt);
    
    json_object_object_add (psJsonStatus,
                            "Description",
                            psJsonStatusText);
    
    if (psJsonNetwork)
    {
        json_object_object_add (psJsonResult,
                                "Network",
                                psJsonNetwork);
    }
    
    printf("%s", json_object_to_json_string(psJsonResult));
#undef SET_STATUS
#undef EXIT_STATUS
    return 0;
}


/** Command handler to return versions */
static tsResult cmd_getVersion(struct json_object* psJsonResult)
{
    struct json_object* psJsonVersionList;
    struct json_object* psJsonCGIVersion;
    struct json_object* psJsonLibVersion;
    tsResult sResult;
    
    psJsonVersionList = json_object_new_object();
    
    json_object_object_add (psJsonResult,
                            "Version",
                            psJsonVersionList);

    psJsonCGIVersion = json_object_new_string(Version);
    psJsonLibVersion = json_object_new_string(JIP_Version);

    json_object_object_add (psJsonVersionList,
                            "JIPcgi",
                            psJsonCGIVersion);

    json_object_object_add (psJsonVersionList,
                            "libJIP",
                            psJsonLibVersion);
    
    SET_RESULT(E_JIP_OK, "Success");
    return  sResult;
}


/** Command handler to return list of available border routers */
static tsResult cmd_discoverBRs(struct json_object* psJsonResult)
{
    int iNumAddresses;
    struct in6_addr *asAddresses;
    tsResult sResult;
    
    if (ZC_Get_Module_Addresses(&asAddresses, &iNumAddresses) != 0)
    {
        SET_RESULT(E_CGI_ERROR, "Failed to find gateway address via Zeroconf");
    }
    else
    {
        struct json_object* psJsonBRList;
        int i;
        
        psJsonBRList = json_object_new_array();
        
        for (i = 0; i < iNumAddresses; i++)
        {
            struct json_object* psJsonBRAddress;
            char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
            inet_ntop(AF_INET6, &asAddresses[i], buffer, INET6_ADDRSTRLEN);
            
            psJsonBRAddress = json_object_new_string(buffer);
            
            json_object_array_add(psJsonBRList, psJsonBRAddress);
        }
            
        json_object_object_add (psJsonResult,
                                "BRList",
                                psJsonBRList);

        free(asAddresses);
        SET_RESULT(E_JIP_OK, "Success");
    }
    return  sResult;
}


/* Network discovery */


struct json_object* psJsonMibList = NULL;
struct json_object* psJsonVarList = NULL;

/** Callback funtion to encode node details */
int json_encode_node (tsNode *psNode, void *pvUser)
{
    struct json_object* psJsonNodeList = (json_object*)pvUser;
    
    struct json_object* psJsonNode;
    struct json_object* psJsonNodeAddress;
    struct json_object* psJsonNodeDeviceId;

    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
    inet_ntop(AF_INET6, &psNode->sNode_Address.sin6_addr, buffer, INET6_ADDRSTRLEN);
    
    psJsonNode = json_object_new_object();
    psJsonNodeAddress = json_object_new_string(buffer);
    
    json_object_object_add (psJsonNode,
                    "IPv6Address",
                    psJsonNodeAddress);
    
    psJsonNodeDeviceId = json_object_new_int64(psNode->u32DeviceId);
    json_object_object_add (psJsonNode,
                    "DeviceID",
                    psJsonNodeDeviceId);
    
    json_object_array_add(psJsonNodeList, psJsonNode);
    
    psJsonMibList = json_object_new_array();
    json_object_object_add (psJsonNode,
                    "MiBs",
                    psJsonMibList);
    return 1;
}


/** Callback funtion to encode mib details */
int json_encode_mib (tsMib *psMib, void *pvUser)
{
    struct json_object* psJsonMib;
    struct json_object* psJsonMibId;
    struct json_object* psJsonMibName;

    psJsonMib = json_object_new_object();
    psJsonMibId =   json_object_new_int64(psMib->u32MibId);
    psJsonMibName = json_object_new_string(psMib->pcName);
    
    json_object_object_add (psJsonMib,
                    "ID",
                    psJsonMibId);
    
    json_object_object_add (psJsonMib,
                    "Name",
                    psJsonMibName);
    
    json_object_array_add(psJsonMibList, psJsonMib);
    
    psJsonVarList = json_object_new_array();
    json_object_object_add (psJsonMib,
                    "Vars",
                    psJsonVarList);
    return 1;
}


/** Callback funtion to encode variable details */
int json_encode_var (tsVar *psVar, void *pvUser)
{
    struct json_object* psJsonVar;
    struct json_object* psJsonVarName;
    struct json_object* psJsonVarIndex;
    struct json_object* psJsonVarType;
    struct json_object* psJsonVarAccessType;
    struct json_object* psJsonVarSecurity;
    struct json_object* psJsonVarValue;
    teJIP_Status eStatus = E_JIP_OK;
    
    tsVarAction *psVarAction = (tsVarAction *)pvUser;

    psJsonVar = json_object_new_object();
    
    psJsonVarName = json_object_new_string(psVar->pcName);
    json_object_object_add (psJsonVar,
                    "Name",
                    psJsonVarName);

    psJsonVarIndex = json_object_new_int(psVar->u8Index);
    json_object_object_add (psJsonVar,
                    "Index",
                    psJsonVarIndex);
    
    psJsonVarType = json_object_new_int(psVar->eVarType);
    json_object_object_add (psJsonVar,
                    "Type",
                    psJsonVarType);
    
    psJsonVarAccessType = json_object_new_int(psVar->eAccessType);
    json_object_object_add (psJsonVar,
                    "AccessType",
                    psJsonVarAccessType);
    
    psJsonVarSecurity = json_object_new_int(psVar->eSecurity);
    json_object_object_add (psJsonVar,
                    "Security",
                    psJsonVarSecurity);
    
    if (psVarAction)
    {
        /* Endode the variable for every get, or if we are encoding the device type variable in the device ID MIB */
        if ((strcmp(psVarAction->pcAction, "get") == 0) ||
            ((strcmp(psVarAction->pcAction, "discover") == 0) &&
             (psVar->psOwnerMib->u32MibId == E_JIP_MIBID_DEVICEID) &&
             (psVar->u8Index == 1)))
        {
            eStatus = eJIP_GetVar(&sJIP_Context, psVar, u32Flags);
                            
            if ((eStatus == E_JIP_OK) && psVar->pvData)
            {
                switch (psVar->eVarType)
                {
                    case(E_JIP_VAR_TYPE_INT8):
                        psJsonVarValue = json_object_new_double((double)*(int8_t*)psVar->pvData);
                        break;
                    case(E_JIP_VAR_TYPE_UINT8):
                        psJsonVarValue = json_object_new_double((double)*(uint8_t*)psVar->pvData);
                        break;
                    case(E_JIP_VAR_TYPE_INT16):
                        psJsonVarValue = json_object_new_double((double)*(int16_t*)psVar->pvData);
                        break;
                    case(E_JIP_VAR_TYPE_UINT16):
                        psJsonVarValue = json_object_new_double((double)*(uint16_t*)psVar->pvData);
                        break;
                    case(E_JIP_VAR_TYPE_INT32):
                        psJsonVarValue = json_object_new_double((double)*(int32_t*)psVar->pvData);
                        break;
                    case(E_JIP_VAR_TYPE_UINT32):
                        psJsonVarValue = json_object_new_double((double)*(uint32_t*)psVar->pvData);
                        break;
                    case(E_JIP_VAR_TYPE_INT64):
                        psJsonVarValue = json_object_new_double((double)*(int64_t*)psVar->pvData);
                        break;
                    case(E_JIP_VAR_TYPE_UINT64):
                        psJsonVarValue = json_object_new_double((double)*(uint64_t*)psVar->pvData);
                        break;
                    case(E_JIP_VAR_TYPE_FLT):
                        psJsonVarValue = json_object_new_double((double)*(float*)psVar->pvData);
                        break;
                    case(E_JIP_VAR_TYPE_DBL):
                        psJsonVarValue = json_object_new_double((double)*(double*)psVar->pvData);
                        break;
                    case  (E_JIP_VAR_TYPE_STR): 
                        psJsonVarValue = json_object_new_string((uint8_t*)psVar->pvData);
                        break;
                    case (E_JIP_VAR_TYPE_BLOB):
                    {
#if 1
// Return blob as hex string for now.
                        uint32_t i, u32Position = 0;
                        char acCurrentValue[255];
                        if (psVar->u8Size == 0)
                        {
                            sprintf(acCurrentValue, "");
                        }
                        else
                        {
                            u32Position += sprintf(acCurrentValue, "0x");

                            for (i = 0; i < psVar->u8Size; i++)
                            {
                                u32Position += sprintf(&acCurrentValue[u32Position], "%02x", ((uint8_t*)psVar->pvData)[i]);
                            }
                        }
                        psJsonVarValue = json_object_new_string(acCurrentValue);
#else                   
                        uint32_t i;
                        psJsonVarValue = json_object_new_array();
                        for (i = 0; i < psVar->u8Size; i++)
                        {
                            struct json_object* psJsonByte;
                            psJsonByte = json_object_new_int(((uint8_t*)psVar->pvData)[i]);
                            json_object_array_add (psJsonVarValue, psJsonByte);
                        }
#endif
                        break;
                    }
                    case (E_JIP_VAR_TYPE_TABLE_BLOB):
                    {
                        tsTable *psTable;
                        tsTableRow *psTableRow;
                        psTable = psVar->ptData;
                        int i;

                        if (psTable->u32NumRows > 0)
                        {
                            psJsonVarValue = json_object_new_array();
                            
                            for (i = 0; i < psTable->u32NumRows; i++)
                            {
                                psTableRow = &psTable->psRows[i];
                                if (psTableRow->pvData)
                                {
                                    struct json_object* psJsonTableRow;
                                    uint32_t j, u32Position = 0;
                                    char acCurrentValue[255];
                                    u32Position += sprintf(acCurrentValue, "0x");
                                    for (j = 0; j < psTableRow->u32Length; j++)
                                    {
                                        u32Position += sprintf(&acCurrentValue[u32Position], "%02x", psTableRow->pbData[j]);
                                    }
                                    psJsonTableRow = json_object_new_string(acCurrentValue);
                                    json_object_array_put_idx(psJsonVarValue, i, psJsonTableRow);
                                }
                            }
                        }
                        else
                        {
                            psJsonVarValue = json_object_new_string("Empty Table");
                        }
                        break;
                    }
                   default: 
                       psJsonVarValue = json_object_new_string("Unknown Type");
                }
                
                json_object_object_add (psJsonVar,
                                        "Data",
                                        psJsonVarValue);
            }
        }
        
        psVarAction->sResult.iValue = eStatus;
        psVarAction->sResult.pcDescription = pcJIP_strerror(psVarAction->sResult.iValue);
    }
    
    json_object_array_add(psJsonVarList, psJsonVar);
    
    return 1;
}


/** Callback funtion to encode variable details */
int set_var(tsVar *psVar, void *pvUser)
{
    tsVarAction *psVarAction = (tsVarAction *)pvUser;
#define SET_STATUS(i, t) \
    psVarAction->sResult.iValue = i; \
    psVarAction->sResult.pcDescription = t
    
    tsNode *psNode;
    tsMib *psMib;
    struct timeval tvBefore, tvAfter;
    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
    int supported   = 1;
    int freeable    = 1;
    int is_multicast = 0;
    struct in6_addr node_addr;
    uint32_t u32Size = 0;
    char *buf = NULL;
    teJIP_Status eStatus;
    int status = 1;
    
    psMib = psVar->psOwnerMib;
    psNode = psMib->psOwnerNode;
    
    inet_ntop(AF_INET6, &psNode->sNode_Address.sin6_addr, buffer, INET6_ADDRSTRLEN);

    if (filter_ipv6)
    {
        if (inet_pton(AF_INET6, filter_ipv6, &node_addr) != 1)
        {
            SET_STATUS(E_JIP_ERROR_BAD_VALUE, "Invalid IPv6 address");
            return 0;
        }
        
        if ((strncmp(filter_ipv6, "FF", 2) == 0) || (strncmp(filter_ipv6, "ff", 2) == 0))
        {
            is_multicast = 1;
        }
    }

    switch (psVar->eVarType)
    {
        case (E_JIP_VAR_TYPE_INT8):
        case (E_JIP_VAR_TYPE_UINT8):
            buf = malloc(sizeof(uint8_t));
            errno = 0;
            buf[0] = strtoul(psVarAction->pcUpdateValue, NULL, 0);
            if (errno)
            {
                SET_STATUS(E_JIP_ERROR_BAD_VALUE, "Could not convert string to 8 bit integer");
                supported = 0;
                break;
            }
            break;
        
        case (E_JIP_VAR_TYPE_INT16):
        case (E_JIP_VAR_TYPE_UINT16):
        {
            uint16_t u16Var;
            errno = 0;
            u16Var = strtoul(psVarAction->pcUpdateValue, NULL, 0);
            if (errno)
            {
                SET_STATUS(E_JIP_ERROR_BAD_VALUE, "Could not convert string to 16 bit integer");
                supported = 0;
                break;
            }
            buf = malloc(sizeof(uint16_t));
            memcpy(buf, &u16Var, sizeof(uint16_t));
            break;
        }
            
        case (E_JIP_VAR_TYPE_INT32):
        case (E_JIP_VAR_TYPE_UINT32):
        {
            uint32_t u32Var;
            errno = 0;
            u32Var = strtoul(psVarAction->pcUpdateValue, NULL, 0);
            if (errno)
            {
                SET_STATUS(E_JIP_ERROR_BAD_VALUE, "Could not convert string to 32 bit integer");
                supported = 0;
                break;
            }
            buf = malloc(sizeof(uint32_t));
            memcpy(buf, &u32Var, sizeof(uint32_t));
            break;
        }
        
        case (E_JIP_VAR_TYPE_INT64):
        case (E_JIP_VAR_TYPE_UINT64):
        {
            uint64_t u64Var;
            errno = 0;
            u64Var = strtoull(psVarAction->pcUpdateValue, NULL, 0);
            if (errno)
            {
                SET_STATUS(E_JIP_ERROR_BAD_VALUE, "Could not convert string to 64 bit integer");
                supported = 0;
                break;
            }
            buf = malloc(sizeof(uint64_t));
            memcpy(buf, &u64Var, sizeof(uint64_t));
            break;
        }
        
        case (E_JIP_VAR_TYPE_FLT):
        {
            float f32Var;
            errno = 0;
            f32Var = strtof(psVarAction->pcUpdateValue, NULL);
            if (errno)
            {
                SET_STATUS(E_JIP_ERROR_BAD_VALUE, "Could not convert string to float");
                supported = 0;
                break;
            }
            buf = malloc(sizeof(uint32_t));
            memcpy(buf, &f32Var, sizeof(uint32_t));
            break;
        }
        
        case (E_JIP_VAR_TYPE_DBL):
        {
            double d64Var;
            errno = 0;
            d64Var = strtod(psVarAction->pcUpdateValue, NULL);
            if (errno)
            {
                SET_STATUS(E_JIP_ERROR_BAD_VALUE, "Could not convert string to double");
                supported = 0;
                break;
            }
            buf = malloc(sizeof(uint64_t));
            memcpy(buf, &d64Var, sizeof(uint64_t));
            break;
        }
            
        case(E_JIP_VAR_TYPE_STR):
        {
            buf = (char *)psVarAction->pcUpdateValue;
            u32Size = strlen(psVarAction->pcUpdateValue);
            freeable = 0;
            break;
        }
            
        case(E_JIP_VAR_TYPE_BLOB):
        {
            int i, j;
            buf = malloc(strlen(psVarAction->pcUpdateValue));
            memset(buf, 0, strlen(psVarAction->pcUpdateValue));
            if (strncmp(psVarAction->pcUpdateValue, "0x", 2) == 0)
            {
                psVarAction->pcUpdateValue += 2;
            }
            u32Size = 0;
            for (i = 0, j = 0; (i < strlen(psVarAction->pcUpdateValue)) && supported; i++)
            {
                uint8_t u8Nibble = 0;
                if ((psVarAction->pcUpdateValue[i] >= '0') && (psVarAction->pcUpdateValue[i] <= '9'))
                {
                    u8Nibble = psVarAction->pcUpdateValue[i]-'0';
                }
                else if ((psVarAction->pcUpdateValue[i] >= 'a') && (psVarAction->pcUpdateValue[i] <= 'f'))
                {
                    u8Nibble = psVarAction->pcUpdateValue[i]-'a' + 0x0A;
                }
                else if ((psVarAction->pcUpdateValue[i] >= 'A') && (psVarAction->pcUpdateValue[i] <= 'F'))
                {
                    u8Nibble = psVarAction->pcUpdateValue[i]-'A' + 0x0A;
                }
                else
                {
                    SET_STATUS(E_JIP_ERROR_BAD_VALUE, "String contains illegal hexadecimal characters");
                    supported = 0;
                    break;
                }
                    
                if ((u32Size & 0x01) == 0)
                {
                    // Even numbered byte
                    buf[j] = u8Nibble << 4;
                }
                else
                {
                    buf[j] |= u8Nibble & 0x0F;
                    j++;
                }
                u32Size++;
            }
            if (u32Size & 0x01)
            {
                // Odd length string
                u32Size = (u32Size >> 1) + 1;
            }
            else
            {
                u32Size = u32Size >> 1;
            }
            
            break;
        }
        
        default:
            supported = 0;
    }
    
    if (supported)
    {
        if (is_multicast)
        {
            tsJIPAddress MCastAddress;

            memset (&MCastAddress, 0, sizeof(struct sockaddr_in6));
            MCastAddress.sin6_family  = AF_INET6;
            MCastAddress.sin6_port    = htons(JIP_DEFAULT_PORT);
            MCastAddress.sin6_addr    = node_addr;
            
            eStatus = eJIP_MulticastSetVar(&sJIP_Context, psVar, buf, u32Size, &MCastAddress, 2, u32Flags);
            
            /* Return an error so we don't get called again for the same multicast */
            status = 0;
        }
        else
        {
            eStatus = eJIP_SetVar(&sJIP_Context, psVar, buf, u32Size, u32Flags);
        }
        SET_STATUS(eStatus, pcJIP_strerror(eStatus));
    }
    else
    {
        SET_STATUS(E_JIP_ERROR_FAILED, "Variable type not supported");
    }
    
    if (freeable)
    {
        free(buf);
    }
#undef SET_STATUS
    return status;
}


/** Command handler for discovering network */
static tsResult cmd_discoverNetwork(struct json_object* psJsonNetwork)
{
    struct json_object* psJsonNodeList;
    psJsonNodeList = json_object_new_array();
    tsResult sResult;
    tsVarAction sVarAction;
    sVarAction.pcAction = "discover";

    jip_iterate(json_encode_node, psJsonNodeList, json_encode_mib, NULL, json_encode_var, &sVarAction);
    
    json_object_object_add (psJsonNetwork,
                            "Nodes",
                            psJsonNodeList);

    return  sVarAction.sResult;
}


/** Command handler for get var */
static tsResult cmd_getVar(struct json_object* psJsonNetwork)
{
    struct json_object* psJsonNodeList;
    psJsonNodeList = json_object_new_array();
    tsVarAction sVarAction;
    sVarAction.pcAction = "get";
 
    sVarAction.sResult.iValue = E_JIP_ERROR_FAILED;
    sVarAction.sResult.pcDescription = "Node not found";
    
    jip_iterate(json_encode_node, psJsonNodeList, json_encode_mib, NULL, json_encode_var, &sVarAction);
    
    json_object_object_add (psJsonNetwork,
                            "Nodes",
                            psJsonNodeList);

    return  sVarAction.sResult;
}


/** Command handler for set var */
static tsResult cmd_setVar(char *pcUpdateValue)
{
    tsVarAction sVarAction;
    sVarAction.pcAction = "set";
    sVarAction.pcUpdateValue = pcUpdateValue;
    
    sVarAction.sResult.iValue = E_JIP_ERROR_FAILED;
    sVarAction.sResult.pcDescription = "Node not found";
    
    jip_iterate(NULL, NULL, NULL, NULL, set_var, &sVarAction);
    return  sVarAction.sResult;
}


/** General purpose function to iterate over the known devices,
 *  filtering on known items and call function callbacks as
 *  required for each node / mib / variable that matches.
 *  \param prCbNode     Callback function to call per matching node
 *  \param pvNodeUser   User data to pass to node callback
 *  \param prCbMib      Callback function to call per matching Mib
 *  \param pvMibUser    User data to pass to Mib callback
 *  \param prCbVar      Callback function to call per matching node
 *  \param pvVarUser    User data to pass to Var callback
 *  \return non-zero on success.
 */
static int jip_iterate(tprCbNode prCbNode,   void *pvNodeUser, 
                        tprCbMib prCbMib,     void *pvMibUser, 
                        tprCbVar prCbVar,     void *pvVarUser)    
{
    tsNode *psNode;
    tsMib *psMib;
    tsVar *psVar;
    tsJIPAddress   *NodeAddressList = NULL;
    uint32_t        u32NumNodes = 0;
    uint32_t        NodeIndex;
    uint32_t        Device_ID;
    struct in6_addr node_addr;
    int is_multicast = 0;
    
    if (filter_ipv6)
    {
        if (inet_pton(AF_INET6, filter_ipv6, &node_addr) != 1)
        {
            //fprintf(stderr, "Invalid IPv6 address '%s'\n\r", filter_ipv6);
            return 0;
        }
        
        if ((strncmp(filter_ipv6, "FF", 2) == 0) || (strncmp(filter_ipv6, "ff", 2) == 0))
        {
            is_multicast = 1;
        }
    }
    
    if (filter_device)
    {
        char *pcEnd;
        errno = 0;
        Device_ID = strtoul(filter_device, &pcEnd, 0);
        if (errno)
        {
            //fprintf(stderr, "Device ID '%s' cannot be converted to 32 bit integer (%s)\n\r", filter_device, strerror(errno));
            return 0;
        }
        if (*pcEnd != '\0')
        {
            //fprintf(stderr, "Device ID '%s' contains invalid characters\n\r", filter_device);
            return 0;
        }
    }
    else
    {
        Device_ID = E_JIP_DEVICEID_ALL;
    }
    
    if (eJIP_GetNodeAddressList(&sJIP_Context, Device_ID, &NodeAddressList, &u32NumNodes) != E_JIP_OK)
    {
        //fprintf(stderr, "Error reading node list\n");
        return 0;
    }

    for (NodeIndex = 0; NodeIndex < u32NumNodes; NodeIndex++)
    {
        psNode = psJIP_LookupNode(&sJIP_Context, &NodeAddressList[NodeIndex]);
        if (!psNode)
        {
            //fprintf(stderr, "Node has been removed\n");
            continue;
        }

        if (filter_ipv6)
        {
            if (!is_multicast)
            {
                /* Filter on IPv6 address */
                if (memcmp(&psNode->sNode_Address.sin6_addr, &node_addr, sizeof(struct in6_addr)) != 0)
                {
                    /* Not the node we want */
                    eJIP_UnlockNode(psNode);
                    continue;
                }
            }
        }

        if (prCbNode)
        {
            /* Call node callback */
            if (!prCbNode(psNode, pvNodeUser))
            {
                eJIP_UnlockNode(psNode);
                goto done;
            }
        }
        
        psMib = psNode->psMibs;
        while (psMib)
        {
            if (filter_mib)
            {
                char *pcEnd;
                uint32_t u32MibId;
                
                /* Filtering on MiB */

                errno = 0;
                u32MibId = strtoul(filter_mib, &pcEnd, 0);
                if (errno)
                {
                    //fprintf(stderr, "MiB ID '%s' cannot be converted to 32 bit integer (%s)\n\r", filter_mib, strerror(errno));
                    return 0;
                }

                if ((pcEnd != filter_mib) && (*pcEnd == '\0')) /* Whole string has been converted - must be a legit number */
                {
                    if (u32MibId != psMib->u32MibId)
                    {
                        psMib = psMib->psNext;
                        continue;
                    }
                }
                else
                {   
                    if (strcmp(filter_mib, psMib->pcName) != 0)
                    {
                        psMib = psMib->psNext;
                        continue;
                    }
                }
            }
            
            if (prCbMib)
            {
                /* Call node callback */
                if (!prCbMib(psMib, pvMibUser))
                {
                    eJIP_UnlockNode(psNode);
                    goto done;
                }
            }
            
            psVar = psMib->psVars;
            while (psVar)
            {
                if (filter_var)
                {
                    char *pcEnd;
                    uint32_t u32VarIndex;
                    
                    /* Filtering on Var */

                    errno = 0;
                    u32VarIndex = strtoul(filter_var, &pcEnd, 0);
                    if ((errno) || ((u32VarIndex > 0x000000FF) ? (errno=ERANGE) : (errno=0)))
                    {
                        fprintf(stderr, "Var Index '%s' cannot be converted to 8 bit integer (%s)\n\r", filter_var, strerror(errno));
                        return 0;
                    }

                    if ((pcEnd != filter_var) && (*pcEnd == '\0')) /* Whole string has been converted - must be a legit number */
                    {
                        if (u32VarIndex != psVar->u8Index)
                        {
                            psVar = psVar->psNext;
                            continue;
                        }
                    }
                    else
                    {   
                        if (strcmp(filter_var, psVar->pcName) != 0)
                        {
                            psVar = psVar->psNext;
                            continue;
                        }
                    }
                }
                
                if (prCbVar)
                {
                    /* Call variable callback */
                    if (!prCbVar(psVar, pvVarUser))
                    {
                        eJIP_UnlockNode(psNode);
                        goto done;
                    }
                }
                
                psVar = psVar->psNext;
            }
            psMib = psMib->psNext;
        }
        eJIP_UnlockNode(psNode);
        psNode = psNode->psNext;
    }

done:
    free(NodeAddressList);    
    return 1;
}


