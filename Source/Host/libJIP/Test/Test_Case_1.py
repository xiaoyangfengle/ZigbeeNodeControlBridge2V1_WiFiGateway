


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
        for node in self.Network:
            self.Nodes[str(node.Address)] = NodeTestStats(node.Address)
    
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

        
def RunTest(Network, set_value):
    print ""
    log( "Setting bulbs: %d" % set_value)
    global global_success
    global global_failures
    global global_lt_100
    global global_lt_250
    global global_lt_1000
    global global_lt_2000
    global global_gt_2000
    
    
    accesses = 0
    successes = 0
    failures = 0
    times = []
    
    for Node in Network.Network:
        Mib = Node.LookupMib("BulbControl")
        if Mib:
            Var = Mib.LookupVar("Mode")
            if Var:
                # Node has BulbControl:Mode mib and variable.
                log( "Setting BulbControl:Mode, %s: %d :" % ( str(Node) , set_value))
                time_before = time.time()
                Var.Data = set_value                # Set new value
                time_after = time.time()
                

                # Read it back again
                if (Var.Get() == libJIP.E_JIP_OK):
                    if (Var.Data == set_value):
                    	log( "   Success: %f" % (time_after-time_before))
                    	successes = successes + 1
                    	times.append(time_after-time_before)
                    else:
                    	log( "   Failed: %f" % (time_after-time_before))
                    	failures = failures + 1
                else:
                    log( "   Failed" )
                    failures = failures + 1
                
                accesses = accesses + 2
    
    log( "Test Results" )
    try:
        log( "Successes : %d (%f%%)" % (successes, (float(successes)/(float(successes)+float(failures)))*100.0 ))
    except ZeroDivisionError:
        log( "Successes : 0 (0%%)" )
        
    log( "Failures  : %d" % failures )
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
    
    global_success = global_success + successes
    global_failures = global_failures + failures
    global_lt_100 = global_lt_100 + lt_100
    global_lt_250 = global_lt_250 + lt_250
    global_lt_1000 = global_lt_1000 + lt_1000
    global_lt_2000 = global_lt_2000 + lt_2000
    global_gt_2000 = global_gt_2000 + gt_2000
    
    print ""
    log( "Overall Results" )
    try:
        log( "Successes : %d (%f%%)" % (global_success, (float(global_success)/(float(global_success)+float(global_failures)))*100.0 ))
    except ZeroDivisionError:
         log( "Successes : 0 (0%%)" )
    log( "Failures  : %d" % global_failures)
    
    try:
        log( "<100ms = %d (%f%%)" % (global_lt_100, (float(global_lt_100)/float(global_success))*100.0 ))
        log( "<250ms = %d (%f%%)" % (global_lt_250, (float(global_lt_250)/float(global_success))*100.0 ))
        log( "<1000ms= %d (%f%%)" % (global_lt_1000, (float(global_lt_1000)/float(global_success))*100.0 ))
        log( "<2000ms= %d (%f%%)" % (global_lt_2000, (float(global_lt_2000)/float(global_success))*100.0 ))
        log( ">2000ms= %d (%f%%)" % (global_gt_2000, (float(global_gt_2000)/float(global_success))*100.0 ))
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

global_success = 0
global_failures = 0
global_lt_100 = 0
global_lt_250 = 0
global_lt_1000 = 0
global_lt_2000 = 0
global_gt_2000 = 0

n = Network(options.gateway_addr)
n.MonitorNetwork()

while (True):
    log("Discovering Network")
    time_before = time.time()
    n.eDiscoverNetwork()
    time_after = time.time()
    log("Discovered %d Nodes" % (len(n.Nodes)))
    log("Discovery time: %f" % ( time_after-time_before))

    test = 1 - test
    RunTest(n, test)

    time.sleep(10)
    
    