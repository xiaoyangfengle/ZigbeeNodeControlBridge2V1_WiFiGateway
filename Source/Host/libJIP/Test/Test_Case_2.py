


import sys, time

sys.path.append("../Source")

import libJIP

class NodeTestStats:
    def __init__(self, Address):
        self.Address = Address
        self.LastJoinTime = time.time()
        self.LastLeftTime = 0
        self.InNetwork = True
        self.InNetworkTime = 0
        self.OutNetworkTime = 0
        
    def Rejoined(self):
        self.InNetwork = True
        self.LastJoinTime = time.time()
        self.OutNetworkTime = self.OutNetworkTime + (self.LastJoinTime - self.LastLeftTime)
        
    def Left(self):
        self.InNetwork = False
        self.LastLeftTime = time.time()
        self.InNetworkTime = self.InNetworkTime + (self.LastLeftTime - self.LastJoinTime)
        
    def __repr__(self):
        if self.InNetwork:
            itime = self.InNetworkTime + (time.time() - self.LastJoinTime)
            otime = self.OutNetworkTime
        else:
            itime = self.InNetworkTime
            otime = self.OutNetworkTime + (time.time() - self.LastLeftTime)
            
        string = "Node: " + self.Address + "\n"
        string = string + "In Network Time:     " + str(itime) + "\n"
        string = string + "Out of Network Time: " + str(otime) + "\n"
        return string


class Network(libJIP.JIP_Context):
    def __init__(self, Coordinator_Address):
        libJIP.JIP_Context.__init__(self)
        self.Nodes = {}
        self.ePersistXMLLoadDefinitions()
        self.eConnect(Coordinator_Address)
    
    def eDiscoverNetwork(self):
        libJIP.JIP_Context.eDiscoverNetwork(self)
        #for node in self.Network:
        #    self.Nodes[str(node.Address)] = NodeTestStats(node.Address)
    
    def MonitorNetwork_Callback(self, Change, Node):
        print time.time()
        if (Change == libJIP.E_JIP_NODE_JOIN):
            print Node, " joined"
            try:
                self.Nodes[str(Node.Address)].Rejoined()
            except KeyError:
                self.Nodes[str(Node.Address)] = NodeTestStats(Node.Address)
                
        elif (Change == libJIP.E_JIP_NODE_LEAVE):
            print Node, " left"
            self.Nodes[str(Node.Address)].Left()
            
        elif (Change == libJIP.E_JIP_NODE_MOVE):
            print Node, " moved"
        else:
            print "Unknown event"
            
        
    def MonitorNetwork(self):
        self.monitor = libJIP.eJIPService_MonitorNetwork(self, self.MonitorNetwork_Callback)
        print self.monitor

def RunTest(Network, set_value, set_address):
    print ""
    log( "Setting bulbs: %d using multicast" % set_value)
    global global_unicast_success
    global global_unicast_failures
    global global_multicast_success
    global global_multicast_failures
    global global_lt_100
    global global_lt_250
    global global_lt_1000
    global global_lt_2000
    global global_gt_2000
    
    
    accesses = 0
    unicast_success = 0
    unicast_failures = 0
    multicast_success = 0
    multicast_failures = 0
    times = []
    stragglers = []
    
    multicast_sent = False
    
    # Now do multicast set and read back unicast
    for Node in Network.Network:
        Mib = Node.LookupMib("BulbControl")
        if Mib:
            Var = Mib.LookupVar("Mode")
            if Var:
                if multicast_sent == False:
                    # Node has BulbControl:Mode mib and variable.
                    # Multicast the command once.
                    log( "Setting BulbControl:Mode using multicast address, %s: %d :" % ( set_address , set_value))
                    time_before = time.time()
                    if (Var.MulticastSet(set_value, set_address) == libJIP.E_JIP_OK):
                        multicast_sent = True
                        log( "Success")
                    else:
                        log( "Failed")
                    time_after = time.time()
                    
                # Read back all nodes
                log( "Reading BulbControl:Mode, %s:" % ( str(Node)))
                time_before = time.time()
                if (Var.Get() == libJIP.E_JIP_OK):
                    time_after = time.time()
                    if (Var.Data == set_value):
                        
                        log( "   Success: %f" % (time_after-time_before))
                        multicast_success = multicast_success + 1
                        unicast_success = unicast_success + 1
                        times.append(time_after-time_before)
                        
                    else:
                        log( "   Failed: Read value (%d) not equal to set value(%d) - unicasting" % (Var.Data, set_value))
                        
                        multicast_failures = multicast_failures + 1
                        
                        # Node has BulbControl:Mode mib and variable.
                        log( "Setting BulbControl:Mode, %s: %d :" % ( str(Node) , set_value))
                        time_before = time.time()
                        Var.Data = set_value                # Unicast set.
                        time_after = time.time()

                        # Read it back again
                        if (Var.Get() == libJIP.E_JIP_OK):
                            if (Var.Data == set_value):
                                log( "   Success: %f" % (time_after-time_before))
                                unicast_success = unicast_success + 1
                                times.append(time_after-time_before)
                            else:
                                log( "   Failed: %f" % (time_after-time_before))
                                unicast_failures = unicast_failures + 1
                                stragglers.append(Node._NodeAddress)
                        else:
                            log( "   Failed" )
                            unicast_failures = unicast_failures + 1
                            stragglers.append(Node._NodeAddress)
                        
                        accesses = accesses + 2   
                else:
                    log( "   Failed" )
                    unicast_failures = unicast_failures + 1
                    stragglers.append(Node._NodeAddress)
                
                accesses = accesses + 1

    # Try again to unicast set any nodes we failed to set before.
    if (len(stragglers) > 0):
        log ("Retrying comms with %d stragglers" % len(stragglers))
        for NodeAddress in stragglers:
            Node = libJIP.psLookupNode(NodeAddress)
            if (not Node):
                log( "Could not lookup failed node")
                confinue
                
            Mib = Node.LookupMib("BulbControl")
            if Mib:
                Var = Mib.LookupVar("Mode")
                if Var:
                    log( "Reading BulbControl:Mode, %s:" % ( str(Node)))
                    time_before = time.time()
                    if (Var.Get() == libJIP.E_JIP_OK):
                        time_after = time.time()
                        if (Var.Data == set_value):
                            
                            log( "   Success: %f" % (time_after-time_before))
                            multicast_success = multicast_success + 1
                            unicast_success = unicast_success + 1
                            times.append(time_after-time_before)
                            
                        else:
                            log( "   Failed: Read value (%d) not equal to set value(%d) - unicasting" % (Var.Data, set_value))
                            
                            multicast_failures = multicast_failures + 1
                            
                            # Node has BulbControl:Mode mib and variable.
                            log( "Setting BulbControl:Mode, %s: %d :" % ( str(Node) , set_value))
                            time_before = time.time()
                            Var.Data = set_value                # Unicast set.
                            time_after = time.time()

                            # Read it back again
                            if (Var.Get() == libJIP.E_JIP_OK):
                                if (Var.Data == set_value):
                                    log( "   Success: %f" % (time_after-time_before))
                                    unicast_success = unicast_success + 1
                                    times.append(time_after-time_before)
                                else:
                                    log( "   Failed: %f" % (time_after-time_before))
                                    unicast_failures = unicast_failures + 1
                            else:
                                log( "   Failed" )
                                unicast_failures = unicast_failures + 1
                            
                            accesses = accesses + 2   
                    else:
                        log( "   Failed" )
                        unicast_failures = unicast_failures + 1
                    
                    accesses = accesses + 1
        
    
    log( "Test Results" )
    try:
        log( "Multicast Successes : %d (%f%%)" % (multicast_success, (float(multicast_success)/(float(multicast_success)+float(multicast_failures)))*100.0 ))
    except ZeroDivisionError:
        log( "Multicast Successes : 0 (0%%)" )
    log( "Multicast Failures  : %d" % multicast_failures )
    
    try:
        log( "Unicast Successes : %d (%f%%)" % (unicast_success, (float(unicast_success)/(float(unicast_success)+float(unicast_failures)))*100.0 ))
    except ZeroDivisionError:
        log( "Unicast Successes : 0 (0%%)" )
    log( "Unicast Failures  : %d" % unicast_failures )
    
    try:
        log( "Average access time: %f" %( sum(times) / len(times)) )
    except ZeroDivisionError:
        log( "Average access time: 0" )
        
    lt_100 = 0
    lt_250 = 0
    lt_1000 = 0
    lt_2000 = 0
    gt_2000 = 0
    for i in range(len(times)):
        if times[i] < 0.1:
            lt_100 = lt_100 + 1
        elif times[i] < 0.25:
            lt_250 = lt_250 + 1
        elif times[i] < 1.00:
             lt_1000 = lt_1000 + 1
        elif times[i] < 2.00:
             lt_2000 = lt_2000 + 1
        else:
            gt_2000 = gt_2000 + 1
            
    log( "<100ms = %d" % lt_100 )
    log( "<250ms = %d" % lt_250 )
    log( "<1000ms= %d" % lt_1000 )
    log( "<2000ms= %d" % lt_2000 )
    log( ">2000ms= %d" % gt_2000 )
    
    global_multicast_success = global_multicast_success + multicast_success
    global_multicast_failures = global_multicast_failures + multicast_failures
    global_unicast_success = global_unicast_success + unicast_success
    global_unicast_failures = global_unicast_failures + unicast_failures
    global_lt_100 = global_lt_100 + lt_100
    global_lt_250 = global_lt_250 + lt_250
    global_lt_1000 = global_lt_1000 + lt_1000
    global_lt_2000 = global_lt_2000 + lt_2000
    global_gt_2000 = global_gt_2000 + gt_2000
    
    print ""
    log( "Overall Results" )
    try:
        log( "Multicast Successes : %d (%f%%)" % (global_multicast_success, (float(global_multicast_success)/(float(global_multicast_success)+float(global_multicast_failures)))*100.0 ))
    except ZeroDivisionError:
        log( "Multicast Successes : 0 (0%%)" )
        
    log( "Multicast Failures  : %d" % global_multicast_failures )
    
    try:
        log( "Unicast Successes   : %d (%f%%)" % (global_unicast_success, (float(global_unicast_success)/(float(global_unicast_success)+float(global_unicast_failures)))*100.0 ))
    except ZeroDivisionError:
        log( "Unicast Successes : 0 (0%%)" )
    log( "Unicast Failures    : %d" % global_unicast_failures )
    
    try:
        log( "<100ms = %d (%f%%)" % (global_lt_100, (float(global_lt_100)/float(global_unicast_success))*100.0 ))
        log( "<250ms = %d (%f%%)" % (global_lt_250, (float(global_lt_250)/float(global_unicast_success))*100.0 ))
        log( "<1000ms= %d (%f%%)" % (global_lt_1000, (float(global_lt_1000)/float(global_unicast_success))*100.0 ))
        log( "<2000ms= %d (%f%%)" % (global_lt_2000, (float(global_lt_2000)/float(global_unicast_success))*100.0 ))
        log( ">2000ms= %d (%f%%)" % (global_gt_2000, (float(global_gt_2000)/float(global_unicast_success))*100.0 ))
    except ZeroDivisionError:
        log( "<100ms = 0 (0%)" )
        log( "<250ms = 0 (0%)" )
        log( "<1000ms= 0 (0%)" )
        log( "<2000ms= 0 (0%)" )
        log( ">2000ms= 0 (0%)" )
    
    print "" 
    

#l = open("/tmp/Test_JIP.log", "w")
def log(message):
    now = time.localtime(time.time())
    print time.asctime(now), ": ", message
    sys.stdout.flush()

    
    
from optparse import OptionParser
parser = OptionParser()
parser.add_option("-6", "--ipv6", dest="gateway_addr",
                  help="ipv6 Address of gateway node")
(options, args) = parser.parse_args()
                
if options.gateway_addr is None:
    print "Please specify ipv6 address of gateway node via -6 or --ipv6"
    sys.exit(0)
        

#for Node in n.Nodes:
#    print n.Nodes[Node]

test = 0

global_multicast_success = 0
global_multicast_failures = 0
global_unicast_success = 0
global_unicast_failures = 0
global_lt_100 = 0
global_lt_250 = 0
global_lt_1000 = 0
global_lt_2000 = 0
global_gt_2000 = 0

n = Network(options.gateway_addr)
#n.MonitorNetwork()

while (True):
    log("Discovering Network")
    time_before = time.time()
    n.eDiscoverNetwork()
    time_after = time.time()
    log("Discovered %d Nodes" % (len(n.Nodes)))
    log("Discovery time: %f" % ( time_after-time_before))

    test = 1 - test
    RunTest(n, test, "ff15::f00f")

    time.sleep(10)
    
    