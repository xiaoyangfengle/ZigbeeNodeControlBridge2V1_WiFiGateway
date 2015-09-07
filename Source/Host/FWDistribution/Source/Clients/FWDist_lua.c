/****************************************************************************
 *
 * MODULE:             Firmware Distribution Client
 *
 * COMPONENT:          Lua client
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

#include <lua.h>
#include <lauxlib.h>

#include "../Common/FWDistribution_IPC.h"


#define FWDISTRIBUTION_LIB_NAME "FWDistribution"

static int iFWDistributionIPCSocket = 0;


#ifndef VERSION
#error Version is not defined!
#else
const char *Version = VERSION;
#endif

/** Block interval, in units of 10ms */
static uint16_t u16BlockInterval = 100; /* Default = 1s */

/** Timeout before devices reset, in units of 10ms */
static uint16_t u16DefaultResetTimeout = 100; /* Default = 1s */

/** Amount of influence network depth has on reset timeout */
static uint16_t u16DefaultResetDepthInfluence = 10;

/** Number of repeats of reset command */
static uint16_t u16DefaultResetRepeatCount = 1; /* Default = 1 repeat */

/** Amount of influence network depth has on reset timeout */
static uint16_t u16DefaultResetRepeatTime = 20; /* Default = 200ms */


/** Get version string */
static int version(lua_State *L)
{
    lua_pushstring(L, Version);
    return 1;
}


/** Get list of available firmwares */
static int FWDistributionIPCClientGetFirmwares(lua_State *L)
{
    tsFWDistributionIPCHeader sHeader;
    uint32_t u32NumFirmwares, i;
    
    sHeader.eType = IPC_GET_AVAILABLE_FIRMWARES;
    sHeader.u32PayloadLength = 0;

    if (send(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        luaL_error(L, "Error in send (%s)", strerror(errno));
    }
    
    if (recv(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        luaL_error(L, "Error in recv (%s)", strerror(errno));
    }

    if (sHeader.eType != IPC_AVAILABLE_FIRMWARES)
    {
        luaL_error(L, "Unexpected reply");
    }
    
    u32NumFirmwares = sHeader.u32PayloadLength / sizeof(tsFWDistributionIPCAvailableFirmwareRecord);

    lua_newtable(L);               /* creates results table */
    
    for (i = 0; i < u32NumFirmwares; i++)
    {
        tsFWDistributionIPCAvailableFirmwareRecord sRecord;
        
        if (recv(iFWDistributionIPCSocket, &sRecord, sizeof(tsFWDistributionIPCAvailableFirmwareRecord), 0) == -1) 
        {
            luaL_error(L, "Error in recv (%s)", strerror(errno));
        }

        /* Start new row in results table, containing a table */
        lua_pushnumber(L, i);
        lua_newtable(L);

        lua_pushstring(L, "DeviceID");
        lua_pushinteger(L, sRecord.u32DeviceID);
        lua_settable(L, -3);
        
        lua_pushstring(L, "ChipType");
        lua_pushinteger(L, sRecord.u16ChipType);
        lua_settable(L, -3);
        
        lua_pushstring(L, "Revision");
        lua_pushinteger(L, sRecord.u16Revision);
        lua_settable(L, -3);
        
        lua_pushstring(L, "Filename");
        lua_pushstring(L, sRecord.acFilename);
        lua_settable(L, -3);
        
        /* Finish row of results table */
        lua_settable(L, -3);
    }

    return 1;
}


/** Request deamon starts a download */
static int FWDistributionIPCClientStartDownload(lua_State *L)
{
    uint32_t u32DeviceID = 0;
    uint16_t u16ChipType = 0, u16Revision = 0;
    struct in6_addr sAddress;
    const char *pcAddress;
    enum teStartDownloadIPCFlags eFlags = 0;
    enum {
        E_PARAM_DEVICE_ID   = 0x01,
        E_PARAM_CHIPTYPE    = 0x02,
        E_PARAM_REVISION    = 0x04,
        E_PARAM_ADDRESS     = 0x08,
        E_PARAM_ALL         = 0x0F
    } eRequiredParameters = 0;
    
    tsFWDistributionIPCHeader sHeader;
    tsFWDistributionIPCStartDownload sPacket;
    
    /* Check that the argument is a table */
    luaL_checktype(L, 1, LUA_TTABLE);
    for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
    {
        /* uses 'key' (at index -2) and 'value' (at index -1) */
        luaL_checktype(L, -2, LUA_TSTRING);

        if (strcmp("DeviceID", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TNUMBER);
            u32DeviceID = lua_tonumber(L, -1);
            eRequiredParameters |= E_PARAM_DEVICE_ID;
        }
        else if (strcmp("ChipType", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TNUMBER);
            u16ChipType = lua_tonumber(L, -1);
            eRequiredParameters |= E_PARAM_CHIPTYPE;
        }
        else if (strcmp("Revision", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TNUMBER);
            u16Revision = lua_tonumber(L, -1);
            eRequiredParameters |= E_PARAM_REVISION;
        }
        else if (strcmp("BlockInterval", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TNUMBER);
            u16BlockInterval = lua_tonumber(L, -1);
        }
        else if (strcmp("Address", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TSTRING);
            pcAddress = lua_tolstring(L, -1, NULL);
            
            if (inet_pton(AF_INET6, pcAddress, &sAddress) != 1)
            {
                luaL_error(L, "Invalid IPv6 address specified (%s)", pcAddress);
            }
            eRequiredParameters |= E_PARAM_ADDRESS;
        }
        else if (strcmp("Flags", lua_tolstring(L, -2, NULL)) == 0)
        {
            /* Check that the argument is a table */
            luaL_checktype(L, -1, LUA_TTABLE);
            for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
            {
                /* uses 'value' (at index -1) */
                luaL_checktype(L, -1, LUA_TSTRING);
                if (strcmp("Inform", lua_tolstring(L, -1, NULL)) == 0)
                {
                    eFlags |= E_DOWNLOAD_IPC_FLAG_INFORM;
                }
                else if (strcmp("Reset", lua_tolstring(L, -1, NULL)) == 0)
                {
                    eFlags |= E_DOWNLOAD_IPC_FLAG_RESET;
                }
                else
                {
                    luaL_error(L, "Unknown flag (%s)", lua_tolstring(L, -1, NULL));
                }
            }
        }
        else
        {
            luaL_error(L, "Unknown parameter (%s)", lua_tolstring(L, -2, NULL));
        }
    }

    if (eRequiredParameters != E_PARAM_ALL)
    {
#define LEN 512
        char acArgs[LEN] = "";
        if (!(eRequiredParameters & E_PARAM_DEVICE_ID)) strncat(acArgs, "DeviceID ", LEN - strlen(acArgs));
        if (!(eRequiredParameters & E_PARAM_CHIPTYPE))  strncat(acArgs, "ChipType ", LEN - strlen(acArgs));
        if (!(eRequiredParameters & E_PARAM_REVISION))  strncat(acArgs, "Revision ", LEN - strlen(acArgs));
        if (!(eRequiredParameters & E_PARAM_ADDRESS))   strncat(acArgs, "Address ",  LEN - strlen(acArgs));
#undef LEN
        acArgs[strlen(acArgs)-1] = '\0';
        luaL_error(L, "Missing required arguments(%s)", acArgs);
    }
    
    sHeader.eType = IPC_START_DOWNLOAD;
    sHeader.u32PayloadLength = sizeof(tsFWDistributionIPCStartDownload);
    
    sPacket.u32DeviceID         = u32DeviceID;
    sPacket.u16ChipType         = u16ChipType;
    sPacket.u16Revision         = u16Revision;
    sPacket.u16BlockInterval    = u16BlockInterval;
    sPacket.sAddress            = sAddress;
    sPacket.eFlags              = eFlags;
    
    if (send(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        luaL_error(L, "Error sending packet(%s)", strerror(errno));
    }

    if (send(iFWDistributionIPCSocket, &sPacket, sizeof(tsFWDistributionIPCStartDownload), 0) == -1) 
    {
        luaL_error(L, "Error sending packet(%s)", strerror(errno));
    }
    
    lua_pushstring(L, "Success");
    
    return 1;
}


/** Request daemon resets devices */
static int FWDistributionIPCClientReset(lua_State *L)
{
    uint32_t u32DeviceID            = 0;
    uint16_t u16ResetTimeout        = u16DefaultResetTimeout;
    uint16_t u16ResetDepthInfluence = u16DefaultResetDepthInfluence;
    uint16_t u16ResetRepeatCount    = u16DefaultResetRepeatCount;
    uint16_t u16ResetRepeatTime     = u16DefaultResetRepeatTime;
    struct in6_addr sAddress;
    const char *pcAddress;
    enum {
        E_PARAM_DEVICE_ID   = 0x01,
        E_PARAM_ADDRESS     = 0x02,
        E_PARAM_ALL         = 0x03
    } eRequiredParameters = 0;
    
    tsFWDistributionIPCHeader sHeader;
    tsFWDistributionIPCReset sPacket;
    
    /* Check that the argument is a table */
    luaL_checktype(L, 1, LUA_TTABLE);
    for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
    {
        /* uses 'key' (at index -2) and 'value' (at index -1) */
        luaL_checktype(L, -2, LUA_TSTRING);

        if (strcmp("DeviceID", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TNUMBER);
            u32DeviceID = lua_tonumber(L, -1);
            eRequiredParameters |= E_PARAM_DEVICE_ID;
        }
        else if (strcmp("Address", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TSTRING);
            pcAddress = lua_tolstring(L, -1, NULL);
            
            if (inet_pton(AF_INET6, pcAddress, &sAddress) != 1)
            {
                luaL_error(L, "Invalid IPv6 address specified (%s)", pcAddress);
            }
            eRequiredParameters |= E_PARAM_ADDRESS;
        }
        else if (strcmp("Timeout", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TNUMBER);
            u16ResetTimeout = lua_tonumber(L, -1);
        }
        else if (strcmp("DepthInfluence", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TNUMBER);
            u16ResetDepthInfluence = lua_tonumber(L, -1);
        }
        else if (strcmp("RepeatCount", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TNUMBER);
            u16ResetRepeatCount = lua_tonumber(L, -1);
        }
        else if (strcmp("RepeatTime", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TNUMBER);
            u16ResetRepeatTime = lua_tonumber(L, -1);
        }
        else
        {
            luaL_error(L, "Unknown parameter (%s)", lua_tolstring(L, -2, NULL));
        }
    }

    if (eRequiredParameters != E_PARAM_ALL)
    {
#define LEN 512
        char acArgs[LEN] = "";
        if (!(eRequiredParameters & E_PARAM_DEVICE_ID)) strncat(acArgs, "DeviceID ", LEN - strlen(acArgs));
        if (!(eRequiredParameters & E_PARAM_ADDRESS))   strncat(acArgs, "Address ",  LEN - strlen(acArgs));
#undef LEN
        acArgs[strlen(acArgs)-1] = '\0';
        luaL_error(L, "Missing required arguments(%s)", acArgs);
    }
    
    sHeader.eType = IPC_RESET;
    sHeader.u32PayloadLength = sizeof(tsFWDistributionIPCReset);
    
    sPacket.u32DeviceID = u32DeviceID;
    sPacket.u16Timeout = u16ResetTimeout;
    sPacket.u16DepthInfluence = u16ResetDepthInfluence;
    sPacket.sAddress = sAddress;
    sPacket.u16RepeatCount = u16ResetRepeatCount;
    sPacket.u16RepeatTime = u16ResetRepeatTime;
    
    
    if (send(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        luaL_error(L, "Error sending packet(%s)", strerror(errno));
    }

    if (send(iFWDistributionIPCSocket, &sPacket, sizeof(tsFWDistributionIPCReset), 0) == -1) 
    {
        luaL_error(L, "Error sending packet(%s)", strerror(errno));
    }
    
    lua_pushstring(L, "Success");
    
    return 1;
}


/** Request daemon cancels a download */
static int FWDistributionIPCClientCancelDownload(lua_State *L)
{
    uint32_t u32DeviceID = 0;
    uint16_t u16ChipType = 0, u16Revision = 0;
    struct in6_addr sAddress;
    const char *pcAddress;
    enum {
        E_PARAM_DEVICE_ID   = 0x01,
        E_PARAM_CHIPTYPE    = 0x02,
        E_PARAM_REVISION    = 0x04,
        E_PARAM_ADDRESS     = 0x08,
        E_PARAM_ALL         = 0x0F
    } eRequiredParameters = 0;
    
    tsFWDistributionIPCHeader sHeader;
    tsFWDistributionIPCCancelDownload sPacket;
    
    /* Check that the argument is a table */
    luaL_checktype(L, 1, LUA_TTABLE);
    for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
    {
        /* uses 'key' (at index -2) and 'value' (at index -1) */
        luaL_checktype(L, -2, LUA_TSTRING);

        if (strcmp("DeviceID", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TNUMBER);
            u32DeviceID = lua_tonumber(L, -1);
            eRequiredParameters |= E_PARAM_DEVICE_ID;
        }
        else if (strcmp("ChipType", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TNUMBER);
            u16ChipType = lua_tonumber(L, -1);
            eRequiredParameters |= E_PARAM_CHIPTYPE;
        }
        else if (strcmp("Revision", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TNUMBER);
            u16Revision = lua_tonumber(L, -1);
            eRequiredParameters |= E_PARAM_REVISION;
        }
        else if (strcmp("Address", lua_tolstring(L, -2, NULL)) == 0)
        {
            luaL_checktype(L, -1, LUA_TSTRING);
            pcAddress = lua_tolstring(L, -1, NULL);
            
            if (inet_pton(AF_INET6, pcAddress, &sAddress) != 1)
            {
                luaL_error(L, "Invalid IPv6 address specified (%s)", pcAddress);
            }
            eRequiredParameters |= E_PARAM_ADDRESS;
        }
        else
        {
            luaL_error(L, "Unknown parameter (%s)", lua_tolstring(L, -2, NULL));
        }
    }

    if (eRequiredParameters != E_PARAM_ALL)
    {
#define LEN 512
        char acArgs[LEN] = "";
        if (!(eRequiredParameters & E_PARAM_DEVICE_ID)) strncat(acArgs, "DeviceID ", LEN - strlen(acArgs));
        if (!(eRequiredParameters & E_PARAM_CHIPTYPE))  strncat(acArgs, "ChipType ", LEN - strlen(acArgs));
        if (!(eRequiredParameters & E_PARAM_REVISION))  strncat(acArgs, "Revision ", LEN - strlen(acArgs));
        if (!(eRequiredParameters & E_PARAM_ADDRESS))   strncat(acArgs, "Address ",  LEN - strlen(acArgs));
#undef LEN
        acArgs[strlen(acArgs)-1] = '\0';
        luaL_error(L, "Missing required arguments(%s)", acArgs);
    
    }
    
    sHeader.eType = IPC_CANCEL_DOWNLOAD;
    sHeader.u32PayloadLength = sizeof(tsFWDistributionIPCCancelDownload);
    
    sPacket.u32DeviceID = u32DeviceID;
    sPacket.u16ChipType = u16ChipType;
    sPacket.u16Revision = u16Revision;
    sPacket.sAddress = sAddress;
    
    if (send(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        luaL_error(L, "Error sending packet(%s)", strerror(errno));
    }

    if (send(iFWDistributionIPCSocket, &sPacket, sizeof(tsFWDistributionIPCCancelDownload), 0) == -1) 
    {
        luaL_error(L, "Error sending packet(%s)", strerror(errno));
    }
    
    lua_pushstring(L, "Success");
    
    return 1;
}


/** Request daemon status */
static int FWDistributionIPCClientMonitor(lua_State *L)
{
    tsFWDistributionIPCHeader sHeader;
    uint32_t i;
    static uint32_t u32StatusRecords = 0;

    sHeader.eType = IPC_GET_STATUS;
    sHeader.u32PayloadLength = 0;
    
    if (send(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        luaL_error(L, "Error sending packet(%s)", strerror(errno));
    }
    
    if (recv(iFWDistributionIPCSocket, &sHeader, sizeof(tsFWDistributionIPCHeader), 0) == -1) 
    {
        luaL_error(L, "Error receiving packet(%s)", strerror(errno));
    }

    if (sHeader.eType != IPC_STATUS)
    {
        luaL_error(L, "Unexpected reply");
    }

    u32StatusRecords = sHeader.u32PayloadLength / sizeof(tsFWDistributionIPCStatusRecord);
    
    lua_newtable(L);               /* creates results table */
    
    for (i = 0; i < u32StatusRecords; i++)
    {
        char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
        
        tsFWDistributionIPCStatusRecord sRecord;
        
        if (recv(iFWDistributionIPCSocket, &sRecord, sizeof(tsFWDistributionIPCStatusRecord), 0) == -1) 
        {
            luaL_error(L, "Error receiving packet(%s)", strerror(errno));
        }
        
        /* Start new row in results table, containing a table */
        lua_newtable(L);
        
        inet_ntop(AF_INET6, &sRecord.sAddress, buffer, INET6_ADDRSTRLEN);
        lua_pushstring(L, "Coordinator");
        lua_pushstring(L, buffer);
        lua_settable(L, -3);
        
        sprintf(buffer, "0x%08x%08x", sRecord.u32AddressH, sRecord.u32AddressL);
        
        lua_pushstring(L, "MAC");
        lua_pushstring(L, buffer);
        lua_settable(L, -3);
        
        lua_pushstring(L, "DeviceID");
        lua_pushnumber(L, sRecord.u32DeviceID);
        lua_settable(L, -3);
        
        lua_pushstring(L, "ChipType");
        lua_pushnumber(L, sRecord.u16ChipType);
        lua_settable(L, -3);
        
        lua_pushstring(L, "Revision");
        lua_pushnumber(L, sRecord.u16Revision);
        lua_settable(L, -3);
        
        lua_pushstring(L, "RemainingBlocks");
        lua_pushnumber(L, sRecord.u32RemainingBlocks);
        lua_settable(L, -3);
        
        lua_pushstring(L, "TotalBlocks");
        lua_pushnumber(L, sRecord.u32TotalBlocks);
        lua_settable(L, -3);
        
        lua_pushstring(L, "Status");
        lua_pushnumber(L, sRecord.u32Status);
        lua_settable(L, -3);
        
        /* Finish row of results table */
        lua_rawseti(L, -2, i+1);  /* set table at key `i+1' */
    }

    return 1;
}


/** Function table for lua module */
static luaL_reg FWDistribution_funcs[] = {
    {"version", version},
    {"available", FWDistributionIPCClientGetFirmwares},
    {"download",  FWDistributionIPCClientStartDownload},
    {"reset",     FWDistributionIPCClientReset},
    {"cancel",    FWDistributionIPCClientCancelDownload},
    {"status",    FWDistributionIPCClientMonitor},
    {NULL, NULL}
};


/** Initialisation for lua module */
int luaopen_FWDistribution(lua_State *L)
{
    
    /* Connect to daemon */
    {
        int len;
        struct sockaddr_un remote;

        if ((iFWDistributionIPCSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        {
            luaL_error(L, "Could not create socket (%s)", strerror(errno));
            return 0;
        }
        
        remote.sun_family = AF_UNIX;
        strcpy(remote.sun_path, FWDistributionIPCSockPath);
        len = strlen(remote.sun_path) + sizeof(remote.sun_family);
        if (connect(iFWDistributionIPCSocket, (struct sockaddr *)&remote, len) == -1)
        {
            luaL_error(L, "Could not connect to daemon (%s)", strerror(errno));
            return 0;
        }
    }
    
    
    luaL_register(L, FWDISTRIBUTION_LIB_NAME, FWDistribution_funcs);
    
    return 1;
}

