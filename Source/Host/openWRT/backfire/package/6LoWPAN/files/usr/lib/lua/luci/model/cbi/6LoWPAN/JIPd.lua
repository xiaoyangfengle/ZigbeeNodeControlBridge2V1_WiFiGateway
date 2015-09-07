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

m = Map("6LoWPAN", translate("JIPd"),
    translate("JIPd provides an application level IPv4 to IPv6 conversion of JIP packets.")
)

function JIPdSection ()
    if not nixio.fs.access("/sbin/JIPd") then
        -- JIPd is not installed so don't present the options for it
        return
    end
    
    s2 = m:section(TypedSection, "JIPd", translate("JIP Proxy"))
    s2.template = "cbi/tblsection"
    s2.extedit  = luci.dispatcher.build_url("admin/6LoWPAN/JIPd_edit/%s")
    s2.anonymous = true
    function s2.enabled()
        
    end

    o = s2:option(Flag, "ignore", translate("Enable"))
    o.rmempty = false
    o.width   = "30px"
    function o.cfgvalue(...)
        local v = Flag.cfgvalue(...)
        return v == "1" and "0" or "1"
    end
    function o.write(self, section, value)
        Flag.write(self, section, value == "1" and "0" or "1")
    end

    o = s2:option(DummyValue, "listen4", translate("IPv4 Listen Address"))
    
    o = s2:option(DummyValue, "port", translate("Listen Port"))
end

JIPdSection()

return m
