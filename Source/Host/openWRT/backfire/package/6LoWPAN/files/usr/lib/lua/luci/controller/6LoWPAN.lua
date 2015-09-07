--[[
LuCI - Lua Configuration Interface

Copyright 2010 Jo-Philipp Wich <xm@subsignal.org>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

$Id: radvd.lua 6811 2011-01-29 03:35:17Z jow $
]]--

module("luci.controller.6LoWPAN", package.seeall)

function index()
	if not nixio.fs.access("/etc/config/6LoWPAN") then
		return
	end
    luci.i18n.loadc("base")
    local i18n = luci.i18n.translate

	entry({"admin", "6LoWPAN"}, cbi("6LoWPAN"), i18n("JenNet-IP"), 61).index = true
	entry({"admin", "6LoWPAN", "6LoWPANd"}, cbi("6LoWPAN/6LoWPANd"), i18n("6LoWPANd"), 1)
    entry({"admin", "6LoWPAN", "6LoWPANd_edit"}, cbi("6LoWPAN/6LoWPANd_edit")).leaf = true
    entry({"admin", "6LoWPAN", "JIPd"}, cbi("6LoWPAN/JIPd"), i18n("JIPd"), 2)
    entry({"admin", "6LoWPAN", "JIPd_edit"}, cbi("6LoWPAN/JIPd_edit")).leaf = true

    entry({"admin", "6LoWPAN", "FWDistribution"}, call("action_FWDistribution"), i18n("Firmware"), 5)

    entry({"admin", "6LoWPAN", "Whitelist"}, call("action_Whitelist"), i18n("Whitelist"), 10)
end

function split(str, sep)
    return {str:match((str:gsub("[^"..sep.."]*"..sep, "([^"..sep.."]*)"..sep)))}
end

function string:split(delimiter)
local result = { }
local from = 1
local delim_from, delim_to = string.find( self, delimiter, from )
while delim_from do
table.insert( result, string.sub( self, from , delim_from-1 ) )
from = delim_to + 1
delim_from, delim_to = string.find( self, delimiter, from )
end
table.insert( result, string.sub( self, from ) )
return result
end


function action_FWDistribution()
    local tmpfile = "/tmp/firmware.bin"
    -- Install upload handler
    local file
    local filename = ""
    
    luci.http.setfilehandler(
      function (meta, chunk, eof)
            if meta then
                filename = meta.file
            end
            if not nixio.fs.access(tmpfile) and not file and chunk and #chunk > 0 then
                file = io.open(tmpfile, "w")
            end
            if file and chunk then
                file:write(chunk)
            end
            if file and eof then
                file:close()
            end
        end
    )

    local control = luci.http.formvalue("control") or "none"
    local err = nil
    
    local tab = luci.http.formvalue("tab.FW")

    if (control == "upload") then
        --tab = "tab.FW.Start"
        require("luci.model.uci")
        local dir = luci.model.uci.cursor():get_first("FWDistributiond", "FWDistributiond", "FWdir")
        local c = uci.cursor()

        local has_upload   = luci.http.formvalue("image")
        local has_image    = nixio.fs.access(tmpfile)
        
        if (has_image) then
            -- File upload completed - move it to the firmware directory.
            
            if dir then
                nixio.fs.move(tmpfile, dir .. "/" .. filename)
                err = "Firmware file " .. filename .. " uploaded."
            else
                err = "Firmware file " .. filename .. " could not be saved - could not find firmware directory"
            end
            
            -- Go to start page
            tab = "tab.FW.Start"
        end
    
    elseif (control == "start") then
        local firmware = luci.http.formvalue("firmware") or nil
        local coord_addr = luci.http.formvalue("coord_addr") or nil
        local block_interval = luci.http.formvalue("block_interval") or 100
        local reset = luci.http.formvalue("reset") or nil
        local inform = luci.http.formvalue("inform") or nil

        if reset then
            flags = { "Reset" }
        else
            flags = {}
        end
        
        if inform then
            flags = { "Inform" }
        else
            flags = {}
        end

        block_interval = tonumber(block_interval)
        
        parts = firmware:split("-")
        local DownloadFirmware = {
            DeviceID=tonumber(parts[1], 16), 
            ChipType=tonumber(parts[2], 16), 
            Revision=tonumber(parts[3], 16), 
            Address=coord_addr, 
            BlockInterval=block_interval, 
            Flags=flags
        }
        
        success, err = pcall(require, 'FWDistribution')

        if success then
            success, err = pcall(FWDistribution.download, DownloadFirmware)
            if success then
                err = "Download Started.."
                -- Go to status page
                tab = "tab.FW.Status"
            end
        end
        
    elseif (control == "reset") then
        local coord_addr = luci.http.formvalue("coord_addr") or nil
        local DeviceID = luci.http.formvalue("DeviceID") or nil
        local Timeout = luci.http.formvalue("Timeout") or nil
        local DepthInfluence = luci.http.formvalue("DepthInfluence") or nil
        local RepeatCount = luci.http.formvalue("RepeatCount") or nil
        local RepeatTime = luci.http.formvalue("RepeatTime") or nil
        
        local ResetRequest = {
            DeviceID=tonumber(DeviceID, 16), 
            Address=coord_addr
        }
                
        if (Timeout) then
            if not (Timeout == "") then
                ResetRequest["Timeout"] = tonumber(Timeout) / 10 -- Convert to units of 10ms
            end
        end
                
        if (DepthInfluence) then
            if not (DepthInfluence == "") then
                ResetRequest["DepthInfluence"] = tonumber(DepthInfluence) / 10 -- Convert to units of 10ms
            end
        end
                
        if (RepeatCount) then
            if not (RepeatCount == "") then
                ResetRequest["RepeatCount"] = tonumber(RepeatCount)
            end
        end
                
        if (RepeatTime) then
            if not (RepeatTime == "") then
                ResetRequest["RepeatTime"] = tonumber(RepeatTime) / 10 -- Convert to units of 10ms
            end
        end
        
        if (coord_addr and DeviceID) then
            success, err = pcall(require, 'FWDistribution')

            if success then
                success, err = pcall(FWDistribution.reset, ResetRequest)
                if success then
                    err = "Devices Reset."
                else
                    err = "Error resetting devices (" .. err .. ")"
                end
                tab = "tab.FW.Reset"
            end
        else
            err = "Wrong parameters"
        end
        
        
    elseif (control == "cancel") then
        local coord_addr = luci.http.formvalue("coord_addr") or nil
        local DeviceID = luci.http.formvalue("DeviceID") or nil
        local ChipType = luci.http.formvalue("ChipType") or nil
        local Revision = luci.http.formvalue("Revision") or nil
        
        if (coord_addr and DeviceID and ChipType and Revision) then
            local CancelFirmware = {
                DeviceID=tonumber(DeviceID), 
                ChipType=tonumber(ChipType), 
                Revision=tonumber(Revision), 
                Address=coord_addr
            }
            
            
            success, err = pcall(require, 'FWDistribution')

            if success then
                success, err = pcall(FWDistribution.cancel, CancelFirmware)
                if success then
                    err = "Download Cancelled."
                    -- Go to status page
                    tab = "tab.FW.Status"
                end
            end
        else
            err = "Wrong parameters"
        end
    end
    
    luci.template.render("6LoWPAN/FWDistribution", {
        Status=err, selected_tab=tab
    } )

end


function action_Whitelist()
    local apply = luci.http.formvalue("apply") or nil
    local save  = luci.http.formvalue("save")  or nil
    local reset = luci.http.formvalue("reset") or nil
    
    local addresses = luci.http.formvalue("node_address") or nil
    local keys = luci.http.formvalue("node_key") or nil
    local enables = luci.http.formvalue("node_enable") or nil
    
    local config_file = "/etc/freeradius2/users.6LoWPAN"
    local tmp_file = "/tmp/users.6LoWPAN"
    
    require 'nixio'
    local wl = require 'luci.model.cbi.6LoWPAN_whitelist'
    
    local nodes = {}
    
    local info = nil
    local changes = nil
    
    -- Make sure that the addresses are a table
    if not (type(addresses) == "table") then
        addresses = { addresses }
    end
    
    -- Make sure that the keys are a table
    if not (type(keys) == "table") then
        keys = { keys }
    end
    
    -- Make sure that the enables are a table
    if not (type(enables) == "table") then
        enables = { enables }
    end
    
    if (reset) then
        -- If we've been asked to reset, remove the unsaved changes.
        nixio.fs.remove(tmp_file)
    end
    
    if (nixio.fs.access(tmp_file) and not reset) then
        -- Load changes from the temporary file
        load_file = tmp_file
        changes = true
    else
        load_file = config_file
    end
    -- Set nodes to the list of nodes that were submitted, if any
    for k, addr in pairs(addresses) do
        local enable = '0'
        if (not (addr == "")) then
            for _,v in pairs(enables) do
                if addr == v then
                    enable = '1'
                    break
                end
            end
            
            nodes[addr] = { key=keys[k], enable=enable }
        end
    end

    if (apply) then
        action = "apply"
        Whitelist.save_nodes(tmp_file, nodes)
        nixio.fs.move(tmp_file, config_file)
        os.execute("killall -HUP radiusd  >/dev/null 2>&1")

    elseif (save) then
        action = "save"
        changes = true
        Whitelist.save_nodes(tmp_file, nodes)
    else
        --No action, or reset
        action = "load " .. load_file
        nodes = Whitelist.load_nodes(load_file)
    end
    
    luci.template.render("6LoWPAN/Whitelist", {
        action=action, nodes=nodes, info=info,
        unsaved_changes=changes
    } )
end

