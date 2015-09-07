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
    translate("Configuration of JIPd")
)

m.redirect = luci.dispatcher.build_url("admin/6LoWPAN/JIPd")

if m.uci:get("6LoWPAN", sid) ~= "JIPd" then
    luci.http.redirect(m.redirect)
    return
end

--[[
m.uci:foreach("6LoWPAN", "JIPd",
    function(s)
        if s['.name'] == sid then
            m.title = translate("JIPd")
            return false
        end
    end)
]]--

s = m:section(NamedSection, sid, "JIPd", translate("JIPd Configuration"))

--
-- General
--

o = s:option(Flag, "ignore", translate("Enable"))
o.rmempty = false

function o.cfgvalue(...)
    local v = Flag.cfgvalue(...)
    return v == "1" and "0" or "1"
end

function o.write(self, section, value)
    Flag.write(self, section, value == "1" and "0" or "1")
end


o = s:option(Value, "listen4", translate("IPv4 Listen Address"),
    translate("IPv4 Address to listen for connections"))

o = s:option(Value, "port", translate("Listen Port"), translate("Port number to listen on for connections"))
--o.datatype = "ip4addr"


return m
