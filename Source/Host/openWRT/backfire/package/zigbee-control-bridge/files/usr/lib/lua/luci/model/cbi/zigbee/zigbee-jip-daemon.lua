--[[
LuCI - Lua Configuration Interface

Copyright 2010 Jo-Philipp Wich <xm@subsignal.org>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

$Id: interface.lua 6811 2011-01-29 03:35:17Z jow $
]]--

m = Map("zigbee-jip-daemon", translate("Zigbee - JIP"),
    translate([[Each zigbee-jip-daemon instance manages a connected Zigbee Control Bridge.
              Each daemon creates a virtual JenNet-IP network mapped to the devices in the Zigbee network.
              Here the configuration of how each connected control bridge is presented and controlled can be managed.]])
)

local nm = require "luci.model.network".init(m.uci)
local ut = require "luci.util"


--

s = m:section(TypedSection, "zigbee-jip-daemon", translate("Interfaces"))
s.template = "cbi/tblsection"
s.extedit  = luci.dispatcher.build_url("admin/zigbee/zigbee-jip-daemon-edit/%s")
s.anonymous = true
s.addremove = true

function s.create(...)
    local id = TypedSection.create(...)
    luci.http.redirect(s.extedit % id)
end

function s.remove(self, section)
    if m.uci:get("zigbee-jip-daemon", section) == "zigbee_jip_daemon" then
        local iface = m.uci:get("zigbee-jip-daemon", section, "zigbee_jip_daemon")
        if iface then
            m.uci:delete_all("zigbee-jip-daemon", "zigbee_jip_daemon",
                function(s) return s.interface == iface end)
        end
    end

    return TypedSection.remove(self, section)
end

o = s:option(Flag, "ignore", translate("Enable"))
o.rmempty = false
o.width   = "30px"
function o.cfgvalue(...)
    local v = Flag.cfgvalue(...)
    return v == "1" and "0" or "1"
end
function o.write(self, section, value)
    Flag.write(self, section, value == "1" and "0" or "1")
end

o = s:option(DummyValue, "tty", translate("Serial Device"))

o = s:option(DummyValue, "interface", translate("Interface"))

o = s:option(DummyValue, "mode", translate("Mode"))

o = s:option(DummyValue, "borderrouter", translate("Border router IPv6 Address"))

o = s:option(DummyValue, "channel", translate("IEEE 802.15.4 Channel"))

return m
