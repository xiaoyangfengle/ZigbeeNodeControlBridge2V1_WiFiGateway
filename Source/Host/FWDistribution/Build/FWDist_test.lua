
local FWDist = require 'FWDistribution'

require "socket"

function sleep(sec)
    socket.select(nil, nil, sec)
end

print (FWDist.version())

firmwares = FWDist.available()
for key,value in pairs(firmwares) 
do 
    for key,value in pairs(value) 
    do
        print(key,value) 
    end
end

--FWDist.download{DeviceID=0x12345678, ChipType=0x0001, Revision=0x0002, Address="::1", Flags={"Reset"}}
print "Requesting reset"
FWDist.reset{DeviceID=0x12345678, Address="::1", Timeout=10, DepthInfluence=100, RepeatCount=3, RepeatTime=100}
print "Done"

while true do
    for key, value in ipairs(FWDist.status()) 
    do    
        print (type(key))
        for key,value in pairs(value) 
        do
            print(key,value) 
        end
    end
    
    print "Done"
    sleep(1)
end