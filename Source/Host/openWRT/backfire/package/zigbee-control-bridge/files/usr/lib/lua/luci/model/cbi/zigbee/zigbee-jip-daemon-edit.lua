--[[
LuCI - Lua Configuration Interface

Copyright 2010 Jo-Philipp Wich <xm@subsignal.org>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

$Id: interface.lua 6811 2011-01-29 03:35:17Z jow $
]]--

local sid = arg[1]
local utl = require "luci.util"

m = Map("zigbee-jip-daemon", translate("Zigbee - JIP"),
    translate("Configuration of zigbee-jip-daemon to abstract Zigbee network as JenNet-IP devices")
)


m.redirect = luci.dispatcher.build_url("admin/zigbee/zigbee-jip-daemon")

if m.uci:get("zigbee-jip-daemon", sid) ~= "zigbee-jip-daemon" then
    luci.http.redirect(m.redirect)
    return
end


-- Custom commit handler to restart zigbee-jip-daemon when chnages are made
m.on_commit = function(map)
    os.execute("/etc/init.d/zigbee-jip-daemon restart > /dev/null")
end

s = m:section(NamedSection, sid, "zigbee-jip-daemon", translate("General Setup"))
--
-- General
--

o = s:option(Flag, "ignore", translate("Enable interface"))
o.rmempty = false

function o.cfgvalue(...)
    local v = Flag.cfgvalue(...)
    return v == "1" and "0" or "1"
end

function o.write(self, section, value)
    Flag.write(self, section, value == "1" and "0" or "1")
end


o = s:option(Value, "tty", translate("Serial port"),
    translate("Specifies the serial port to which the Zigbee control bridge is connected to. Examples: USB FTDI=/dev/ttyUSB0, LPC3240 Internal=/dev/ttyTX0"))

o.datatype = "device"

function o.formvalue(...)
    return Value.formvalue(...) or "-"
end

function o.validate(self, value)
    if value == "-" then
        return nil, translate("Serial port required")
    end
    return value
end

o = s:option(Value, "baudrate", translate("Baudrate"),
    translate("Specifies the baud rate that the Zigbee control bridge communicates at."))

o.datatype = "uinteger"

o = s:option(Value, "interface", translate("Network interface"),
    translate("Specifies the name of the network interface that will be created to host the fake JenNet-IP network. Eg zb0"))


s = m:section(NamedSection, sid, "zigbee-jip-daemon", translate("Zigbee Control Bridge Options"))
--
-- ZCB
--

o = s:option(ListValue, "mode", translate("Control bridge mode"),
    translate("Zigbee profile / mode"))

o:value("coordinator",  "Zigbee HA Coordinator (Default)")
o:value("router",       "Zigbee HA+LL Router")
o:value("touchlink",    "Zigbee LL Router")

o = s:option(Value, "channel", translate("IEEE 802.15.4 Channel"),
    translate("The 802.15.4 channel that the network will operate on"))

o.datatype = "uinteger"

function o.validate(self, value)
    local v = tonumber(value)
    if (v > 0) and ((v < 11) or (v > 26)) then
        return nil, translate("Invalid IEEE 802.15.4 channel number (11-26)")
    end
    return value
end

o = s:option(Flag, "whitelisting", translate("Enable Zigbee Whitelisting"),
    translate("If checked, enable whitelisting of joining devices using NFC"))
o.rmempty = false

s = m:section(NamedSection, sid, "zigbee-jip-daemon", translate("JenNet-IP options"))
--
-- JIP
--

o = s:option(Value, "borderrouter", translate("Virtual border router IPv6 address"),
    translate("IPv6 address that will be given to the virtual JenNet-IP border roouter. Implies the IPv6 prefix of the virtual JenNet-IP network."))

o.datatype = "ip6addr"

return m
