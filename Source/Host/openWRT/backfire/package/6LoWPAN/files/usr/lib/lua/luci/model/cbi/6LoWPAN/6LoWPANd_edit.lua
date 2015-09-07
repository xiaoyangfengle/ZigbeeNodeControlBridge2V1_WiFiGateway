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

m = Map("6LoWPAN", translate("6LoWPAN"),
    translate("Configuration of 6LoWPAN daemon to route IPv6 packets to IEEE 802.15.4 network")
)

-- Determine IPv6 address of LAN interface.
for k, v in ipairs(nixio.getifaddrs()) do
    if ((v.name == "br-lan") or (v.name == "eth0")) then
        if (v.family == "inet6") then
            if not (v.addr:lower():find("fe80") == 1) then
                lan_address = v.addr
            end
        end
    end
end


m.redirect = luci.dispatcher.build_url("admin/6LoWPAN/6LoWPANd")

if m.uci:get("6LoWPAN", sid) ~= "6LoWPANd" then
	luci.http.redirect(m.redirect)
	return
end


-- Custom commit handler to restart 6LoWPANd when chnages are made
m.on_commit = function(map)
	os.execute("/etc/init.d/6LoWPAN restart > /dev/null")
end

s = m:section(NamedSection, sid, "6LoWPANd", translate("6LoWPANd Configuration"))
--
-- General
--

s:tab("general", translate("General Setup"))
s:tab("security", translate("Security"))
s:tab("radio_hardware", translate("IEEE802.15.4 Radio Hardware"))

o = s:taboption("general", Flag, "ignore", translate("Enable Interface"))
o.rmempty = false

function o.cfgvalue(...)
	local v = Flag.cfgvalue(...)
	return v == "1" and "0" or "1"
end

function o.write(self, section, value)
	Flag.write(self, section, value == "1" and "0" or "1")
end

o = s:taboption("general", Flag, "qos_enable", translate("Enable 15.4 Bandwidth Throttling"),
    translate("IEEE802.15.4 is a low data rate network. Enabling this option reduces the potential for overloading the network with excessive traffic."))
o.rmempty = false


o = s:taboption("general", Value, "tty", translate("Serial Port"),
	translate("Specifies the serial port to which the IEEE 802.15.4 device is connected to. Examples: USB FTDI=/dev/ttyUSB0, USB LPC1343=/dev/ttyACM0"))

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

o = s:taboption("general", Value, "baudrate", translate("Baudrate"),
    translate("Specifies the baud rate that IEEE802.15.4 communicates at."))

o.datatype = "uinteger"

o = s:taboption("general", Value, "interface", translate("Network interface"),
    translate("Specifies the name of the network interface that will be created to host the JenNet-IP network. Eg tun0"))


o = s:taboption("general", Value, "channel", translate("IEEE 802.15.4 Channel"),
    translate("The 802.15.4 channel that the network will operate on"))

o.datatype = "uinteger"

function o.validate(self, value)
    local v = tonumber(value)
    if (v > 0) and ((v < 11) or (v > 26)) then
        return nil, translate("Invalid IEEE 802.15.4 channel number (11-26)")
    end
    return value
end

o = s:taboption("general", Value, "pan", translate("IEEE 802.15.4 Pan ID"),
    translate("The 802.15.4 Pan ID that the network will operate on"))

o.datatype = "uciname"

function o.validate(self, value)
    local v = tonumber(value, 16)
    if (v > 0xFFFF) then
        return nil, translate("Invalid IEEE 802.15.4 Pan ID (0000 - FFFF)")
    end
    return value
end


o = s:taboption("general", Value, "jennet", translate("JenNet Network Id to start"),
    translate("ID of the JenNet network to start. Other nodes looking for this ID will join"))

o.datatype = "uciname"

function o.validate(self, value)
    local v = tonumber(value, 16)
    if (v == nil) then
        return nil, translate("Invalid JenNet ID (00000000 - FFFFFFFF)")
    end
    return value
end


o = s:taboption("general", ListValue, "profile", translate("JenNet Profile"),
    translate("JenNet network profile tailors network configuration to its size and the topology of its environment."))

o:value("0", "0 - Large dense network (Default)")
o:value("1", "1")
o:value("2", "2")
o:value("3", "3")
o:value("4", "4")
o:value("5", "5")
o:value("6", "6")
o:value("7", "7 - Small sparse network")
o:value("8", "8 - Standalone small network")
o:value("9", "9 - Standalone large network")
o:value("255", "255 - Automatic selection")


o = s:taboption("general", Value, "prefix", translate("6LoWPAN Network Prefix"),
    translate("IPv6 Prefix for the 6LoWPAN network"))

o.datatype = "ip6addr"


-- Begin Security Tab

o = s:taboption("security", Flag, "secure", translate("JenNet Security Enabled"))
o.rmempty = false

o = s:taboption("security", Value, "jennet_key", translate("JenNet Security Key"),
    translate("JenNet security key. Specified like an IPv6 address e.g. 0:1:2::3:4"))
o:depends("secure", "1")
o.datatype = "ip6addr"


o = s:taboption("security", ListValue, "auth_scheme", translate("Node Authorisation Scheme"))
o:depends("secure", "1")
o:value("0", "No Authorisation")
o:value("1", "RADIUS Server with PAP")


lan_address = lan_address or "Could not determine non link local IPv6 address"

o = s:taboption("security", Value, "radius_ip", translate("RADIUS Server IPv6 Address"),
    translate(string.format([[IPv6 Address of RADIUS server that will authorise nodes. 
    This should usually be set to the IPv6 address of the lan interface: '%s']], lan_address)))
o:depends("auth_scheme", "1")
o.datatype = "ip6addr"



-- Begin Radio Hardware Tab

o = s:taboption("radio_hardware", ListValue, "radio_frontend", translate("Radio Frontend"),
    translate("Select the type of radio front end fitted to the IEEE802.15.4 device (Must match the hardware configuration)"))

o:value("SP", "Standard Power (No PA / LNA fitted)")
o:value("HP", "High Power  (PA and LNA fitted)")
o:value("ETSI", "ETSI Compliant Mode (PA and LNA fitted)")


o = s:taboption("radio_hardware", Flag, "diversity", translate("Enable Antenna Diversity"),
    translate("Turn on Antenna diversity (hardware must be present for diversity to function)"))
o.rmempty = false


return m
