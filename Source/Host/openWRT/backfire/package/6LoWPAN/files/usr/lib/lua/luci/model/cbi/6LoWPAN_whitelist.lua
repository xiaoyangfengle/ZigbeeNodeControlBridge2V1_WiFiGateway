

module("Whitelist", package.seeall)

-- Function to split a string on a given delimiter string
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


-- Load nodes from file.
function load_nodes(file)
    local node_address = ""
    local has_key = 0
    local enable = 1
    
    nodes = { }
    
    f = assert(io.open(file, "r"))
    for line in f:lines() do
        if (line:len() > 0) then
            if (line:find("Cleartext")) then
                addr = line:split("=")[2]:split("\"")[2]
                if (addr:len()) then
                    node_address = addr
                else
                    node_address = ""
                end
                
                if (line:split(" ")[1]:find("#")) then
                    enable = '0'
                else
                    enable = '1'
                end
                    
            elseif (line:find("Vendor")) then
                vs = line:split("=")[2]:split("\"")[2]
                if (vs:len()) then
                    key = vs:sub(15, 47)
                    if (key:len() == 32) then
                        nodes[node_address] = { key=key, enable=enable }
                    end
                end
            end
        end
    end
    
    f:close()

    return nodes    
end


-- Load nodes from file.
function save_nodes(file, nodes)
    
    f = assert(io.open(file, "w"))
    
    f:write(
[[

# Nodes that are allowed to join the network by the radius server are added in here
# Entries are as follows:
# <MAC_ADDRESS> Cleartext-Password := "<MAC_ADDRESS>"
#          Vendor-Specific = "0x00006DE96412<128 bit commisioning key>"

]])

    for node_address,node_info in pairs(nodes) do
        if (node_info.enable == '1') then
            enable = ""
        else
            enable = "# "
        end
        line = string.format("# Greylist time: %s\n", os.date("%a %b %d %H:%M:%S GMT %Y"))
        f:write(line)
        line = string.format("%s%s Cleartext-Password := %q\n", enable, node_address, node_address)
        f:write(line)
        line = string.format("%s        Vendor-Specific = \"0x00006DE96412%s\"\n\n", enable, node_info.key)
        f:write(line)             
    end
    
    f:close()
end


function test()
    nodes = load_nodes("/etc/freeradius2/users.6LoWPAN")
    for node_address,node_info in pairs(nodes) do
        print("Node:" .. node_address)
        for key,value in pairs(node_info) do
            print("     " .. key .. " " .. value)
        end
        print(" ")
    end
    save_nodes("/tmp/users.6LoWPAN", nodes)
end
    

--test()

