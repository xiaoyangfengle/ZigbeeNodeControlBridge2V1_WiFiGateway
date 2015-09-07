--[[
LuCI - Lua Configuration Interface

Copyright 2010 Jo-Philipp Wich <xm@subsignal.org>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

$Id: interface.lua 6811 2011-01-29 03:35:17Z jow $
]]--

m = Map("6LoWPAN", translate("6LoWPANd"),
    translate([[Each 6LoWPANd instance manages a connected JenNet-IP border router node.
              Here the configuration of each JenNet-IP border router node can be managed.]])
)

local nm = require "luci.model.network".init(m.uci)
local ut = require "luci.util"


--

s = m:section(TypedSection, "6LoWPANd", translate("Interfaces"))
s.template = "cbi/tblsection"
s.extedit  = luci.dispatcher.build_url("admin/6LoWPAN/6LoWPANd_edit/%s")
s.anonymous = true
s.addremove = true

function s.create(...)
    local id = TypedSection.create(...)
    luci.http.redirect(s.extedit % id)
end

function s.remove(self, section)
    if m.uci:get("6LoWPAN", section) == "6LoWPANd" then
        local iface = m.uci:get("6LoWPAN", section, "6LoWPANd")
        if iface then
            m.uci:delete_all("6LoWPAN", "6LoWPANd",
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

o = s:option(DummyValue, "channel", translate("IEEE 802.15.4 Channel"))

o = s:option(DummyValue, "pan", translate("IEEE 802.15.4 PAN ID"))

o = s:option(DummyValue, "jennet", translate("JenNet ID"))

o = s:option(DummyValue, "prefix", translate("Wireless Network IPv6 Prefix"))


return m
