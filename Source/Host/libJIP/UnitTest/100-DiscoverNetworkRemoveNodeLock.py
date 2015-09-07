
import sys, time

sys.path.append("../Source")
import libJIP


LockNodes = []


def MonitorNetwork_Callback(Change, Node):
	print time.time()
	if (Change == libJIP.E_JIP_NODE_JOIN):
		print Node, " joined"
		LockNodes.append(Node)
			
	elif (Change == libJIP.E_JIP_NODE_LEAVE):
		print Node, " left"
		
	elif (Change == libJIP.E_JIP_NODE_MOVE):
		print Node, " moved"
	else:
		print "Unknown event"
		
	
n = libJIP.JIP_Context()

n.eConnect(sys.argv[1])

n.eDiscoverNetwork()


monitor = libJIP.eJIPService_MonitorNetwork(n, MonitorNetwork_Callback)

for node in n.Network:
	print node.Address

while (1):
	time.sleep(30)
	print "Discovering"
	n.eDiscoverNetwork()
	for Node in LockNodes:
		print "Locking Node", Node
		libJIP.eJIP_LockNode(Node)
		print "Got Lock - Remove Node now"
		time.sleep(120)
		print "Releasing lock on Node", Node
		libJIP.eJIP_UnlockNode(Node)
		LockNodes.remove(Node)
