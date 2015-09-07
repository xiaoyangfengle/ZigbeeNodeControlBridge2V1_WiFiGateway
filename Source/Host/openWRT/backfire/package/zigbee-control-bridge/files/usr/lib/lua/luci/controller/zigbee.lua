--[[
LuCI - Lua Configuration Interface

Copyright 2010 Jo-Philipp Wich <xm@subsignal.org>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

$Id: radvd.lua 6811 2011-01-29 03:35:17Z jow $
]]--

module("luci.controller.zigbee", package.seeall)

function index()
    luci.i18n.loadc("base")
    local i18n = luci.i18n.translate

    entry({"admin", "zigbee"}, cbi("zigbee"), i18n("Zigbee Gateway"), 71).index = true
    entry({"admin", "zigbee", "zigbee-jip-daemon"}, cbi("zigbee/zigbee-jip-daemon"), i18n("Zigbee - JIP"), 1)
    entry({"admin", "zigbee", "zigbee-jip-daemon-edit"}, cbi("zigbee/zigbee-jip-daemon-edit")).leaf = true
end
