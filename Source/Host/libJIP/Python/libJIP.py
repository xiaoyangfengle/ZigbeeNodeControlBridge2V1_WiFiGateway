############################################################################
#
# This software is owned by NXP B.V. and/or its supplier and is protected
# under applicable copyright laws. All rights are reserved. We grant You,
# and any third parties, a license to use this software solely and
# exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139]. 
# You, and any third parties must reproduce the copyright and warranty notice
# and any other legend of ownership on each copy or partial copy of the 
# software.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Copyright NXP B.V. 2012. All rights reserved
#
############################################################################

import time
import sys
import atexit
import random
import copy
from ctypes import *
from socket import AF_INET, AF_INET6, AF_PACKET, inet_ntop, inet_pton, ntohs, htons

libJIP_Name = "libJIP.so.2"
cdll.LoadLibrary(libJIP_Name)
libJIP = CDLL(libJIP_Name)

try:
    cdll.LoadLibrary("libc.so.6")
    libc = CDLL("libc.so.6")
except OSError:
    try:
        cdll.LoadLibrary("libc.so.0")
        libc = CDLL("libc.so.0")
    except:
        print "Could not import libc"


    
# Import libJIP Version.
JIP_Version = c_char_p.in_dll(libJIP, "JIP_Version").value
    
## Default cache files for libJIP
JIP_CACHE_DEFINITIONS_FILE_NAME    = "/tmp/jip_cache_definitions.xml"
JIP_CACHE_NETWORK_FILE_NAME        = "/tmp/jip_cache_network.xml"

## Default port number for JIP
JIP_DEFAULT_PORT = 1873

## Device ID to retrieve all devices
JIP_DEVICEID_ALL = 0xFFFFFFFF


## Flags for \ref eJIP_GetVar
JIP_FLAG_NONE                       = 0
JIP_FLAG_STAY_AWAKE                 = 1
JIP_FLAG_FORCE                      = 2


# AF_INET6 / IPv6
class in6_u(Union):
    _fields_ = [
        ("u6_addr8",  (c_uint8 * 16) ),
        ("u6_addr16", (c_uint16 * 8) ),
        ("u6_addr32", (c_uint32 * 4) )
    ]
    
class in6_addr(Union):
    _fields_ = [
        ("in6_u", in6_u),
    ]
    
class sockaddr_in6(Structure):
    _fields_ = [
        ("sin6_family",   c_int16),
        ("sin6_port",     c_uint16),
        ("sin6_flowinfo", c_uint32),
        ("sin6_addr",     in6_addr),
        ("sin6_scope_id", c_uint32),
    ]
    
    def __init__(self, address="", port=JIP_DEFAULT_PORT):
        try:
            addr = inet_pton(AF_INET6, address)
            for i in range(16):
                self.sin6_addr.in6_u.u6_addr8[i] = ord(addr[i])
        except:
            # Python 2.6 does not support inet_pton for IPv6 - have to use libc version
            # int inet_pton(int af, const char *src, void *dst);
            addr = (c_int8 * 16)(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0)
            if (not libc.inet_pton(AF_INET6, address, pointer(addr)) == 1):
                raise ValueError("Could not convert string")
            for i in range(16):
                self.sin6_addr.in6_u.u6_addr8[i] = addr[i]
        
        self.sin6_family = AF_INET6
        self.sin6_port = htons(port)
        self.sin6_flowinfo = 0
        self.sin6_scope_id = 0
    
    def __repr__(self):
        try:
            str_ip = inet_ntop(AF_INET6, self.sin6_addr)
        except:
            # Python 2.6 does not support inet_ntop for IPv6 - have to use libc version
            str_ip = c_char_p("0000:0000:0000:0000:0000:0000:0000:0000")
            res = c_char_p(libc.inet_ntop(AF_INET6, pointer(self.sin6_addr), str_ip, 40))
            if (res == 0):
                raise ValueError("Could not convert string")
            str_ip = str_ip.value
  
        return "[" + str_ip + "]:%d" % (ntohs(self.sin6_port))

class JIP_TableRow(Structure):
    _fields_ = [
        ("Length",        c_uint32),
        ("Data",          c_void_p),
    ]

class JIP_Table(Structure):
    _fields_ = [
        ("NumRows",       c_uint32),
        ("Rows",          POINTER(JIP_TableRow)),
    ]


## Constants for libJip context type
E_JIP_CONTEXT_CLIENT               = 0
E_JIP_CONTEXT_SERVER               = 1

JIP_Context_Type_Strings = {
    E_JIP_CONTEXT_CLIENT:          "E_JIP_CONTEXT_CLIENT",
    E_JIP_CONTEXT_SERVER:          "E_JIP_CONTEXT_SERVER",
}


## Constants for JIP Return codes
E_JIP_OK                           = 0         ## Success
E_JIP_ERROR_TIMEOUT                = 0x7f      ## Request timed out
E_JIP_ERROR_BAD_MIB_INDEX          = 0x8f      ## MiB index does not exist
E_JIP_ERROR_BAD_VAR_INDEX          = 0x9f      ## Variable index does not exist
E_JIP_ERROR_NO_ACCESS              = 0xaf      ## Security failure
E_JIP_ERROR_BAD_BUFFER_SIZE        = 0xbf      ## Buffer is not big enough
E_JIP_ERROR_WRONG_TYPE             = 0xcf      ## Type mismatch
E_JIP_ERROR_BAD_VALUE              = 0xdf      ## Value not appropriate for type
E_JIP_ERROR_DISABLED               = 0xef      ## Access is disabled
E_JIP_ERROR_FAILED                 = 0xff      ## Generic failure

E_JIP_ERROR_BAD_DEVICE_ID          = 0x11,     ## Bad device ID was passed 
E_JIP_ERROR_NETWORK                = 0x12,     ## Network error 
E_JIP_ERROR_WOULD_BLOCK            = 0x13,     ## The operation would block the current thread 
E_JIP_ERROR_NO_MEM                 = 0x14      ## Memory allocation failed
E_JIP_ERROR_WRONG_CONTEXT          = 0x15      ## A function was called that is inappropriate for the context type


JIP_Status_Strings = {
    E_JIP_OK:                      "E_JIP_OK",
    E_JIP_ERROR_TIMEOUT:           "E_JIP_ERROR_TIMEOUT",
    E_JIP_ERROR_BAD_MIB_INDEX:     "E_JIP_ERROR_BAD_MIB_INDEX",
    E_JIP_ERROR_BAD_VAR_INDEX:     "E_JIP_ERROR_BAD_VAR_INDEX",
    E_JIP_ERROR_NO_ACCESS:         "E_JIP_ERROR_NO_ACCESS",
    E_JIP_ERROR_BAD_BUFFER_SIZE:   "E_JIP_ERROR_BAD_BUFFER_SIZE",
    E_JIP_ERROR_WRONG_TYPE:        "E_JIP_ERROR_WRONG_TYPE",
    E_JIP_ERROR_BAD_VALUE:         "E_JIP_ERROR_BAD_VALUE",
    E_JIP_ERROR_DISABLED:          "E_JIP_ERROR_DISABLED",
    E_JIP_ERROR_FAILED:            "E_JIP_ERROR_FAILED",
    
    E_JIP_ERROR_BAD_DEVICE_ID:     "E_JIP_ERROR_BAD_DEVICE_ID",
    E_JIP_ERROR_NETWORK:           "E_JIP_ERROR_NETWORK",
    E_JIP_ERROR_WOULD_BLOCK:       "E_JIP_ERROR_WOULD_BLOCK",
    E_JIP_ERROR_NO_MEM:            "E_JIP_ERROR_NO_MEM",
    E_JIP_ERROR_WRONG_CONTEXT:     "E_JIP_ERROR_WRONG_CONTEXT",
}
    
   
## Constants for variable types
E_JIP_VAR_TYPE_INT8                = 0
E_JIP_VAR_TYPE_INT16               = 1
E_JIP_VAR_TYPE_INT32               = 2
E_JIP_VAR_TYPE_INT64               = 3
E_JIP_VAR_TYPE_UINT8               = 4
E_JIP_VAR_TYPE_UINT16              = 5
E_JIP_VAR_TYPE_UINT32              = 6
E_JIP_VAR_TYPE_UINT64              = 7
E_JIP_VAR_TYPE_FLT                 = 8
E_JIP_VAR_TYPE_DBL                 = 9
E_JIP_VAR_TYPE_STR                 = 10
E_JIP_VAR_TYPE_BLOB                = 11
E_JIP_VAR_TYPE_TABLE_BLOB          = 75

## String lookup for variable types
JIP_Var_Type_Strings = {
    E_JIP_VAR_TYPE_INT8:           'E_JIP_VAR_TYPE_INT8',
    E_JIP_VAR_TYPE_INT16:          'E_JIP_VAR_TYPE_INT16',
    E_JIP_VAR_TYPE_INT32:          'E_JIP_VAR_TYPE_INT32',
    E_JIP_VAR_TYPE_INT64:          'E_JIP_VAR_TYPE_INT64',
    E_JIP_VAR_TYPE_UINT8:          'E_JIP_VAR_TYPE_UINT8',
    E_JIP_VAR_TYPE_UINT16:         'E_JIP_VAR_TYPE_UINT16',
    E_JIP_VAR_TYPE_UINT32:         'E_JIP_VAR_TYPE_UINT32',
    E_JIP_VAR_TYPE_UINT64:         'E_JIP_VAR_TYPE_UINT64',
    E_JIP_VAR_TYPE_FLT:            'E_JIP_VAR_TYPE_FLT',
    E_JIP_VAR_TYPE_DBL:            'E_JIP_VAR_TYPE_DBL',
    E_JIP_VAR_TYPE_STR:            'E_JIP_VAR_TYPE_STR',
    E_JIP_VAR_TYPE_BLOB:           'E_JIP_VAR_TYPE_BLOB',
    E_JIP_VAR_TYPE_TABLE_BLOB:     'E_JIP_VAR_TYPE_TABLE_BLOB',
}

## Constants for variable access types
E_JIP_ACCESS_TYPE_CONST            = 0
E_JIP_ACCESS_TYPE_READ_ONLY        = 1
E_JIP_ACCESS_TYPE_READ_WRITE       = 2

## String lookup for variable access types
JIP_Var_Access_Type_Strings = {
    E_JIP_ACCESS_TYPE_CONST:       'E_JIP_ACCESS_TYPE_CONST',
    E_JIP_ACCESS_TYPE_READ_ONLY:   'E_JIP_ACCESS_TYPE_READ_ONLY',
    E_JIP_ACCESS_TYPE_READ_WRITE:  'E_JIP_ACCESS_TYPE_READ_WRITE',
}

## Constants for variable security types
E_JIP_SECURITY_NONE                = 0

## String lookup for variable security types
JIP_Var_Security_Type_Strings = {
    E_JIP_SECURITY_NONE:           'E_JIP_SECURITY_NONE',
}


## Constants for variable enabled state
E_JIP_VAR_DISABLED                 = 0
E_JIP_VAR_ENABLED                  = 1

JIP_Var_Enable_Strings = {
    E_JIP_VAR_DISABLED:            'E_JIP_VAR_DISABLED',
    E_JIP_VAR_ENABLED:             'E_JIP_VAR_ENABLED',
}


## Constants for network monitor events
E_JIP_NODE_JOIN                    = 0
E_JIP_NODE_LEAVE                   = 1
E_JIP_NODE_MOVE                    = 2

## String lookup for network monitor events
JIP_Network_Monitor_Strings = {
    E_JIP_NODE_JOIN:               'E_JIP_NODE_JOIN',
    E_JIP_NODE_LEAVE:              'E_JIP_NODE_LEAVE',
    E_JIP_NODE_MOVE:               'E_JIP_NODE_MOVE',
}

class JIP_Result:
    def __init__(self, result):
        self.result = result
        
    def __cmp__(self, other):
        if self.result == other:
            return 0
        else:
            return 1
        
    def __repr__(self):
        try:
            return JIP_Status_Strings[self.result & 0xFF]
        except KeyError:
            return "Unknown Result (%d)" % (self.result)
        
    def __trunc__(self):
        return int(self.result)


class JIP_Var(Structure):
    """Class representing a JIP variable."""
    def str_type(self):
        """Return the variable type as a string"""
        try:
            return JIP_Var_Type_Strings[self.Type]
        except KeyError:
            return "Unknown Variable type"
    
    def str_access(self):
        """Return the variable access type as a string"""
        try:
            return JIP_Var_Access_Type_Strings[self.AccessType]
        except KeyError:
            return "Unknown Access type"

    def str_security(self):
        """Return the variable security type as a string"""
        try:
            return JIP_Var_Security_Type_Strings[self.Security]
        except KeyError:
            return "Unknown Security Type"
        
    def str_enabled(self):
        """Return the variable enable state as a string"""
        try:
            return JIP_Var_Enable_Strings[self.Enabled]
        except KeyError:
            return "Unknown Enabled State"

    def __repr__(self):
        """ Return a string description of the variable"""
        string = "Var " + str(self.Index) + " - '" + self.Name + "'"
        string = string + (" (disabled)" if self.Enable == E_JIP_VAR_DISABLED else "")
        string = string + "\n"
        string = string + "  Type: " + self.str_type() + "\n"
        string = string + "  Size: " + str(self.Size) + "\n"
        string = string + "  Security: " + self.str_security() + "\n"
        string = string + "  AccessType: " + self.str_access() + "\n"
        string = string + "  Data: " + str(self.Data) + "\n"
        return string

    ## Represent the data as a property of the variable
    @property
    def Data(self):
        """Return a python representation of the known value of the variable"""
        if bool(self._Data) == False:
            # Return None if we have a NULL pointer
            return None
            
        if      (self.Type == E_JIP_VAR_TYPE_INT8):
            return cast(self._Data, POINTER(c_int8))[0]
        elif    (self.Type == E_JIP_VAR_TYPE_INT16):
            return cast(self._Data, POINTER(c_int16))[0]
        elif    (self.Type == E_JIP_VAR_TYPE_INT32):
            return cast(self._Data, POINTER(c_int32))[0]
        elif    (self.Type == E_JIP_VAR_TYPE_INT64):
            return cast(self._Data, POINTER(c_int64    ))[0]
        elif    (self.Type == E_JIP_VAR_TYPE_UINT8):
            return cast(self._Data, POINTER(c_uint8))[0]
        elif    (self.Type == E_JIP_VAR_TYPE_UINT16):
            return cast(self._Data, POINTER(c_uint16))[0]
        elif    (self.Type == E_JIP_VAR_TYPE_UINT32):
            return cast(self._Data, POINTER(c_uint32))[0]
        elif    (self.Type == E_JIP_VAR_TYPE_UINT64):
            return cast(self._Data, POINTER(c_uint64))[0]
        elif    (self.Type == E_JIP_VAR_TYPE_FLT):
            return cast(self._Data, POINTER(c_float))[0]
        elif    (self.Type == E_JIP_VAR_TYPE_DBL):
            return cast(self._Data, POINTER(c_double))[0]
        elif    (self.Type == E_JIP_VAR_TYPE_STR):
            return string_at(cast(self._Data, c_char_p), self.Size)
        elif    (self.Type == E_JIP_VAR_TYPE_BLOB):
            data = []
            for byte in cast(self._Data, POINTER(c_uint8 * self.Size))[0]:
                data.append(byte)
            return data 
        elif    (self.Type == E_JIP_VAR_TYPE_TABLE_BLOB):
            rows = {}
            raw_table = cast(self._Data, POINTER(JIP_Table))[0]
            for i in range(raw_table.NumRows):
                raw_row = cast(raw_table.Rows, POINTER(JIP_TableRow))[i]
                row = []
                if (raw_row.Length > 0):
                    for byte in cast(raw_row.Data, POINTER(c_uint8 * raw_row.Length))[0]:
                        row.append(byte)
                    rows[i] = row
            return rows
    
    def Set(self, value):
        """Do a JIP write of the variable"""
        ownermib = cast(self.OwnerMib, POINTER(JIP_Mib))[0]
        ownernode = cast(ownermib.OwnerNode, POINTER(JIP_Node))[0]
        ownernetwork = cast(ownernode.OwnerNetwork, POINTER(JIP_Network))[0]
        ownercontext = cast(ownernetwork.OwnerContext, POINTER(JIP_Context))[0]
        return eJIP_SetVar(ownercontext, self, value)
    
    def MulticastSet(self, value, address, hops=2):
        """Handy pythonic way of updating the variable contents"""
        ownermib = cast(self.OwnerMib, POINTER(JIP_Mib))[0]
        ownernode = cast(ownermib.OwnerNode, POINTER(JIP_Node))[0]
        ownernetwork = cast(ownernode.OwnerNetwork, POINTER(JIP_Network))[0]
        ownercontext = cast(ownernetwork.OwnerContext, POINTER(JIP_Context))[0]
        return eJIP_MulticastSetVar(ownercontext, self, value,address, hops)
    
    def Get(self):
        """Do a JIP read of the variable"""
        ownermib = cast(self.OwnerMib, POINTER(JIP_Mib))[0]
        ownernode = cast(ownermib.OwnerNode, POINTER(JIP_Node))[0]
        ownernetwork = cast(ownernode.OwnerNetwork, POINTER(JIP_Network))[0]
        ownercontext = cast(ownernetwork.OwnerContext, POINTER(JIP_Context))[0]
        return eJIP_GetVar(ownercontext, self)
        
    def Trap(self, AppCallBack, TrapHandle=None):
        """Trap the variable"""
        ownermib = cast(self.OwnerMib, POINTER(JIP_Mib))[0]
        ownernode = cast(ownermib.OwnerNode, POINTER(JIP_Node))[0]
        ownernetwork = cast(ownernode.OwnerNetwork, POINTER(JIP_Network))[0]
        ownercontext = cast(ownernetwork.OwnerContext, POINTER(JIP_Context))[0]
        return eJIP_TrapVar(ownercontext, self, AppCallBack, TrapHandle)

    def Untrap(self, TrapHandle=None):
        """Trap the variable"""
        ownermib = cast(self.OwnerMib, POINTER(JIP_Mib))[0]
        ownernode = cast(ownermib.OwnerNode, POINTER(JIP_Node))[0]
        ownernetwork = cast(ownernode.OwnerNetwork, POINTER(JIP_Network))[0]
        ownercontext = cast(ownernetwork.OwnerContext, POINTER(JIP_Context))[0]
        return eJIP_UntrapVar(ownercontext, self, TrapHandle)

        
JIP_Var._fields_ = [
                ("OwnerMib",        c_void_p),
                ("Next",            POINTER(JIP_Var)),
                ("_Data",           c_void_p),
                ("GetCb",           c_void_p),
                ("SetCb",           c_void_p),
                ("TrapCb",          c_void_p),
                ("Name",            c_char_p),
                ("Enable",          c_uint8),
                ("Type",            c_uint8),
                ("AccessType",      c_uint8),
                ("Security",        c_uint8),
                ("Index",           c_uint8),
                ("Size",            c_uint8),
                ("TrapHandle",      c_uint8),
        ]
    
class JIP_Var_iterator:
    def __init__(self, Var):
        self.Var = Var
        
    def __iter__(self):
        return self
        
    def next(self):
        if not bool(self.Var) == True:
            raise StopIteration
        
        Var = self.Var[0]
        self.Var = self.Var[0].Next
        return Var
    
    
class JIP_Mib(Structure):
    pass

    def __iter__(self):
        return JIP_Var_iterator(self.Vars)
        
    def __repr__(self):
        string = "Mib, ID:" + hex(self.MibId) + " - '" + self.Name + "'"
        return string
        
    def LookupVar(self, Name):
        return psJIP_LookupVar(self, Name)
        
    def LookupVarIndex(self, Index):
        return psJIP_LookupVarIndex(self, Index)

JIP_Mib._fields_ = [
                ("OwnerNode",       c_void_p),
                ("Next",            POINTER(JIP_Mib)),
                ("Vars",            POINTER(JIP_Var)),
                ("Name",            c_char_p),
                ("MibId",           c_uint32),
                ("NumVars",         c_uint32),
                ("Index",           c_uint8),
        ]
    
class JIP_Mib_iterator:
    def __init__(self, Mib):
        self.Mib = Mib
        
    def __iter__(self):
        return self
        
    def next(self):
        if not bool(self.Mib) == True:
            raise StopIteration
            
        Mib = self.Mib[0]
        self.Mib = self.Mib[0].Next
        return Mib
    
    
class JIP_Node(Structure):
    pass

    def __iter__(self):
        return JIP_Mib_iterator(self.Mibs)
        
    def __repr__(self):
        string = "Node: " + self.Address + " ID: 0x%08x" % (self.DeviceId)
        return string
        
    def Lock(self):
        eJIP_LockNode(self)
        
    def Unlock(self):
        eJIP_UnlockNode(self)
        
    def LookupMib(self, Name):
        return psJIP_LookupMib(self, Name)
        
    def LookupMibId(self, Id):
        return psJIP_LookupMibId(self, Id)
        
    @property
    def Address(self):
        """The Node's IPv6 Address and port"""
        return str(self._NodeAddress)

JIP_Node._fields_ = [
                ("OwnerNetwork",    c_void_p),
                ("Next",            POINTER(JIP_Node)),
                ("_Private",        c_void_p),
                ("Mibs",            POINTER(JIP_Mib)),
                ("Lock",            c_void_p),
                ("_NodeAddress",    sockaddr_in6),
                ("DeviceId",        c_uint32),
                ("NumMibs",         c_uint32),
        ]


class JIP_Network_iterator:
    def __init__(self, context, filter=JIP_DEVICEID_ALL):
        self.context = context
        eJIP_Lock(self.context)
        (self.NodeList, self.NodeListLength) = eJIP_GetNodeAddressList(context, filter)
        self.position = 0
        self.LastNode = None
        eJIP_Unlock(self.context)
        
    def __iter__(self):
        return self
        
    def next(self):
        if self.LastNode:
            # Unlock the node from previous iteration
            self.LastNode.Unlock()
            
        Node = None
        while (Node == None):
            if self.position == self.NodeListLength:
                # Need to free the list now that we're done with it.
                libc.free(self.NodeList)
                raise StopIteration

            NodeAddress = self.NodeList[self.position]

            self.position = self.position + 1
            Node = psJIP_LookupNode(self.context, NodeAddress)
            if not (Node == None):
                # Node is still in the network
                self.LastNode = Node
                return Node
        
        
class JIP_Network(Structure):
    _fields_ = [
                ("OwnerContext", c_void_p),
                ("Nodes", POINTER(JIP_Node)),
                ("NumNodes", c_int32),
        ]
                
    def __iter__(self):
        return JIP_Network_iterator(cast(self.OwnerContext, POINTER(JIP_Context))[0])


class JIP_Context(Structure):
    _fields_ = [
        ("pvPriv", c_void_p), 
        ("Network", JIP_Network), 
        ("MulticastInterface", c_int),
        ("MulticastSendCount", c_int),
    ]
    
    def __init__(self, ctx_type=E_JIP_CONTEXT_CLIENT):
        eJIP_Init(self, ctx_type)
        libJIP_Networks.append(self)
    
    def Lock(self):
        return eJIP_Lock(self) 

    def Unlock(self):
        return eJIP_Unlock(self) 
    
    def Destroy(self):
        libJIP_Networks.remove(self)
        eJIP_Destroy(self)
        
    def eConnect(self, IPv6Address, Port=JIP_DEFAULT_PORT):
        return eJIP_Connect(self, IPv6Address, Port)
        
    def eConnect4(self, IPv4Address, IPv4Port, IPv6Address, IPv6Port=JIP_DEFAULT_PORT, TCP=False):
        return eJIP_Connect4(self, IPv4Address, IPv4Port, IPv6Address, IPv6Port, TCP)
        
    def eGroupJoin(self, MulticastAddress):
        return eJIP_GroupJoin(self, MulticastAddress)
        
    def eGroupLeave(self, MulticastAddress):
        return eJIP_GroupLeave(self, MulticastAddress)
        
    def ePersistXMLLoadDefinitions(self, Filename=JIP_CACHE_DEFINITIONS_FILE_NAME):
        """ Load JIP cache of network contents """
        return eJIPService_PersistXMLLoadDefinitions(self, Filename)

    def ePersistXMLSaveDefinitions(self, Filename=JIP_CACHE_DEFINITIONS_FILE_NAME):
        """ Save network contents to cache file """
        return eJIPService_PersistXMLSaveDefinitions(self, Filename)
        
    def ePersistXMLLoadNetwork(self, Filename=JIP_CACHE_NETWORK_FILE_NAME):
        """ Load JIP cache of network contents """
        return eJIPService_PersistXMLLoadNetwork(self, Filename)

    def ePersistXMLSaveNetwork(self, Filename=JIP_CACHE_NETWORK_FILE_NAME):
        """ Save network contents to cache file """
        return eJIPService_PersistXMLSaveNetwork(self, Filename)

    def eDiscoverNetwork(self):
        """ Run a network discovery """
        return eJIPService_DiscoverNetwork(self)
    
    def eGetVar(self, var):
        return eJIP_GetVar(self, var)

    def eSetVar(self, var, value):
        return eJIP_SetVar(self, var, value)
    
    

def eJIP_Init(context, ctx_type=E_JIP_CONTEXT_CLIENT):
    """ Initialise context"""
    return JIP_Result(libJIP.eJIP_Init(byref(context), ctx_type))
    
def eJIP_Destroy(context):
    """ Destroy JIP context"""
    result = JIP_Result(libJIP.eJIP_Destroy(byref(context)))
    if addressof(context) in libJIP_Traps:
        del libJIP_Traps[addressof(context)]
    return result
    
def eJIP_Connect(context, IPv6Address, Port=JIP_DEFAULT_PORT):
    """ Connect to IPv6 address of network entry point"""
    return JIP_Result(libJIP.eJIP_Connect(byref(context), IPv6Address, Port))

def eJIP_Connect4(context, IPv4Address, IPv4Port, IPv6Address, IPv6Port=JIP_DEFAULT_PORT, TCP=False):
    """ Connect to IPv6 address of network entry point via IPv4 transport"""
    return JIP_Result(libJIP.eJIP_Connect4(byref(context), IPv4Address, IPv4Port, IPv6Address, IPv6Port, int(TCP)))
    
def eJIP_GroupJoin(context, MulticastAddress):
    return JIP_Result(libJIP.eJIP_GroupJoin(byref(context), MulticastAddress))

def eJIP_GroupLeave(context, MulticastAddress):
    return JIP_Result(libJIP.eJIP_GroupLeave(byref(context), MulticastAddress))
    
def eJIPService_PersistXMLLoadDefinitions(context, Filename=JIP_CACHE_DEFINITIONS_FILE_NAME):
    """ Load JIP cache with device definitions """
    return JIP_Result(libJIP.eJIPService_PersistXMLLoadDefinitions(byref(context), Filename))

def eJIPService_PersistXMLSaveDefinitions(context, Filename=JIP_CACHE_DEFINITIONS_FILE_NAME):
    """ Save device definitions to file """
    return JIP_Result(libJIP.eJIPService_PersistXMLSaveDefinitions(byref(context), Filename))
    
def eJIPService_PersistXMLLoadNetwork(context, Filename=JIP_CACHE_NETWORK_FILE_NAME):
    """ Load libJIP with network contents """
    return JIP_Result(libJIP.eJIPService_PersistXMLLoadNetwork(byref(context), Filename))

def eJIPService_PersistXMLSaveNetwork(context, Filename=JIP_CACHE_NETWORK_FILE_NAME):
    """ Save network contents to file """
    return JIP_Result(libJIP.eJIPService_PersistXMLSaveNetwork(byref(context), Filename))
    
    
def eJIPService_DiscoverNetwork(context):
    """ Run a network discovery """
    return JIP_Result(libJIP.eJIPService_DiscoverNetwork(byref(context)))

    
def eJIP_GetVar(context, var, flags=JIP_FLAG_NONE):
    """ Read a variable"""
    return JIP_Result(libJIP.eJIP_GetVar(byref(context), byref(var), c_uint32(flags)))
    

def eJIP_SetVar(context, var, value):
    return eJIP_MulticastSetVar(context, var, value)


def getVarData(var, value):
    newdata = None
    size = 0
    if      (var.Type == E_JIP_VAR_TYPE_INT8):
        if (not isinstance(value, int)): raise TypeError("INT8 set requires an int type")
        newdata = c_int8(value)
        size = 1
    elif    (var.Type == E_JIP_VAR_TYPE_INT16):
        if (not isinstance(value, int)): raise TypeError("INT16 set requires an int type")
        newdata = c_int16(value)
        size = 2
    elif    (var.Type == E_JIP_VAR_TYPE_INT32):
        if (not isinstance(value, int)): raise TypeError("INT32 set requires an int type")
        newdata = c_int32(value)
        size = 4
    elif    (var.Type == E_JIP_VAR_TYPE_INT64):
        if (not isinstance(value, (int, long))): raise TypeError("INT64 set requires an int or long type")
        newdata = c_int64(value)
        size = 8
    elif    (var.Type == E_JIP_VAR_TYPE_UINT8):
        if (not isinstance(value, int)): raise TypeError("UINT8 set requires an int type")
        newdata = c_uint8(value)
        size = 1
    elif    (var.Type == E_JIP_VAR_TYPE_UINT16):
        if (not isinstance(value, int)): raise TypeError("UINT16 set requires an int type")
        newdata = c_uint16(value)
        size = 2
    elif    (var.Type == E_JIP_VAR_TYPE_UINT32):
        if (not isinstance(value, (int, long))): raise TypeError("UINT32 set requires an int or long type")
        newdata = c_uint32(value)
        size = 4
    elif    (var.Type == E_JIP_VAR_TYPE_UINT64):
        if (not isinstance(value, (int, long))): raise TypeError("UINT64 set requires an int or long type")
        newdata = c_uint64(value)
        size = 8
    elif    (var.Type == E_JIP_VAR_TYPE_FLT):
        if (not isinstance(value, float)): raise TypeError("Float set requires a float type")
        newdata = c_float(value)
        size = 4
    elif    (var.Type == E_JIP_VAR_TYPE_DBL):
        if (not isinstance(value, float)): raise TypeError("Double set requires a float type")
        newdata = c_double(value)
        size = 8
    elif    (var.Type == E_JIP_VAR_TYPE_STR):
        if (not isinstance(value, str)): raise TypeError("String set requires a string type")
        newdata = create_string_buffer(value)
        size = len(value)
    elif    (var.Type == E_JIP_VAR_TYPE_BLOB):
        if (not isinstance(value, list)): raise TypeError("Blob set requires a list type")
        size = len(value)
        newdata = (c_uint8 * size)()
        for i in range(size):
            newdata[i] = value[i]
    elif    (var.Type == E_JIP_VAR_TYPE_TABLE_BLOB):
        raise Exception("Table set not supported")
    else:
        raise Exception("set of unsupported type")
    
    return (newdata, size)


def eJIP_MulticastSetVar(context, var, value, address=None, hops=2):
    """ Update a nodes variable"""
    
    if (address is not None):
        if (isinstance(address, str)):
            address = sockaddr_in6(address)
        elif (isinstance(address, sockaddr_in6)):
            pass
        else:
            raise TypeError("eJIP_MulticastSetVar requires a string or sockaddr_in6")
    
    newdata, size = getVarData(var, value)

    if (address is None):
        return JIP_Result(libJIP.eJIP_SetVar(byref(context), byref(var), byref(newdata), size))
    else:
        return JIP_Result(libJIP.eJIP_MulticastSetVar(byref(context), byref(var), byref(newdata), size, byref(address), c_int(hops)))
    

class eJIPService_MonitorNetwork:
    """ Class to monitor network contents """
    def __init__(self, context, AppCallBack=None):
        JIP_NetworkChangeCallback = CFUNCTYPE(None, c_int, POINTER(JIP_Node))
        self.network_monitor_callback = JIP_NetworkChangeCallback(self.network_change_handler_callback)
        self.AppCallBack = AppCallBack
        self.context = context

        self.changes = 0
        self.result = JIP_Result(libJIP.eJIPService_MonitorNetwork(byref(context), self.network_monitor_callback))
        #print "Start network monitor:", self.result
    
    def stop(self):
        return JIP_Result(libJIP.eJIPService_MonitorNetworkStop(byref(self.context)))
    
    def network_change_handler_callback(self, Change, Node):
        #print "Network changed"
        #print JIP_Network_Monitor_Strings[Change], " - ", Node[0]
        self.changes = self.changes + 1
        if (self.AppCallBack):
            self.AppCallBack(Change, Node[0])

    def __repr__(self):
        if (self.result == E_JIP_OK):
            return "Monitoring network, %d changes detected" % (self.changes)
        else:
            return "Network monitor failed"

# Dictionary of all registered traps.
libJIP_Traps = {}
            
class eJIP_TrapVar:
    """ Class to trap a variable """
    def __init__(self, context, var, AppCallBack=None, TrapHandle=None):
        TrapCallback = CFUNCTYPE(None, POINTER(JIP_Var))
        self.trap_callback = TrapCallback(self.callback)
        self.AppCallBack = AppCallBack
        if TrapHandle == None:
            self.TrapHandle = random.randint(0, 255)
        else:
            self.TrapHandle = TrapHandle

        if addressof(context) not in libJIP_Traps:
            libJIP_Traps[addressof(context)] = {}
        libJIP_Traps[addressof(context)][addressof(var)] = self

        self.status = JIP_Result(libJIP.eJIP_TrapVar(byref(context), byref(var), self.TrapHandle, self.trap_callback))
    
    def callback(self, Var):
        #print "Variable changed"
        #print Var[0]
        if (self.AppCallBack):
            self.AppCallBack(Var[0])
            
    def __repr__(self):
        if self.status == E_JIP_OK:
            return "JIP Trap OK, callback " + str(self.AppCallBack)
        else:
            return "JIP Trap failed"

def eJIP_UntrapVar(context, var, TrapHandle=None):
    try:
        del libJIP_Traps[addressof(context)][addressof(var)]
    except KeyError:
        pass


    
    return JIP_Result(libJIP.eJIP_UntrapVar(byref(context), byref(var), TrapHandle))
    
            
def eJIP_GetNodeAddressList(context, DeviceIdFilter = JIP_DEVICEID_ALL):
    Addresses = POINTER(sockaddr_in6)()
    NumAddresses = c_uint32(0)

    """ Run a network discovery """
    result = JIP_Result(libJIP.eJIP_GetNodeAddressList(byref(context), c_uint32(DeviceIdFilter), 
                         byref(Addresses), byref(NumAddresses)))

    if result == E_JIP_OK:
        return (Addresses, NumAddresses.value)
    else:
        return ([], 0)
            
    
def psJIP_LookupNode(context, Address):
    _psJIP_LookupNode = libJIP.psJIP_LookupNode
    _psJIP_LookupNode.restype = POINTER(JIP_Node)
    Node = _psJIP_LookupNode(byref(context), byref(Address))
    if Node:
        return Node[0]
    else:
        return None

def eJIP_Lock(context):
    return JIP_Result(libJIP.eJIP_Lock(byref(context))) 

def eJIP_Unlock(context):
    return JIP_Result(libJIP.eJIP_Unlock(byref(context))) 
    
        
def eJIP_LockNode(Node):
    return JIP_Result(libJIP.eJIP_LockNode(byref(Node))) 
   
   
def eJIP_UnlockNode(Node):
    return JIP_Result(libJIP.eJIP_UnlockNode(byref(Node))) 

    
def psJIP_LookupMib(Node, Name):
    _psJIP_LookupMib = libJIP.psJIP_LookupMib
    _psJIP_LookupMib.restype = POINTER(JIP_Mib)
    Mib = _psJIP_LookupMib(byref(Node), c_void_p(), c_char_p(Name))
    if Mib:
        return Mib[0]
    else:
        return None

        
def psJIP_LookupMibId(Node, Id):
    _psJIP_LookupMibId = libJIP.psJIP_LookupMibId
    _psJIP_LookupMibId.restype = POINTER(JIP_Mib)
    Mib = _psJIP_LookupMibId(byref(Node), c_void_p(), c_uint32(Id))
    if Mib:
        return Mib[0]
    else:
        return None
        
        
def psJIP_LookupVar(Mib, Name):
    _psJIP_LookupVar = libJIP.psJIP_LookupVar
    _psJIP_LookupVar.restype = POINTER(JIP_Var)
    Var = _psJIP_LookupVar(byref(Mib), c_void_p(), c_char_p(Name))
    if Var:
        return Var[0]
    else:
        return None

        
def psJIP_LookupVarIndex(Mib, Index):
    _psJIP_LookupVarIndex = libJIP.psJIP_LookupVarIndex
    _psJIP_LookupVarIndex.restype = POINTER(JIP_Var)
    Var = _psJIP_LookupVarIndex(byref(Mib), c_uint8(Index))
    if Var:
        return Var[0]
    else:
        return None


def eJIPserver_NodeAdd(context, address, DeviceId, name, version):
    if (not isinstance(address, str)):  raise TypeError("Address must be IPv6 address as a string")
    if (not isinstance(DeviceId, int)): raise TypeError("Device ID must be an integer")
    if (not isinstance(name, str)):     raise TypeError("Name must be a string")
    if (not isinstance(version, str)):  raise TypeError("Version must be a string")
    
    return JIP_Result(libJIP.eJIPserver_NodeAdd(byref(context), address, DeviceId, name, version, c_void_p()))


def eJIPserver_NodeRemove(context, node):
    return  JIP_Result(libJIP.eJIPserver_NodeRemove(byref(context), byref(node)))


def eJIPserver_Listen(context, Port=JIP_DEFAULT_PORT):
    return  JIP_Result(libJIP.eJIPserver_Listen(byref(context), Port))


def eJIPserver_NodeGroupJoin(node, address):
    if (not isinstance(address, str)):  raise TypeError("Address must be IPv6 address as a string")
    return JIP_Result(libJIP.eJIPserver_NodeGroupJoin(byref(node), address))


def eJIPserver_NodeGroupLeave(node, address):
    if (not isinstance(address, str)):  raise TypeError("Address must be IPv6 address as a string")
    return JIP_Result(libJIP.eJIPserver_NodeGroupLeave(byref(node), address))


def eJIP_SetVarValue(var, value):
    newdata, size = getVarData(var, value)
    return JIP_Result(libJIP.eJIP_SetVarValue(byref(var), byref(newdata), size))


def eJIP_Table_UpdateRow(var, index, value):
    if    (var.Type == E_JIP_VAR_TYPE_TABLE_BLOB):
        if (not isinstance(value, list)): raise TypeError("table blob set requires a list type")
        size = len(value)
        newdata = (c_uint8 * size)()
        for i in range(size):
            newdata[i] = value[i]
    else:
        raise TypeError("Unhandled variable type")
    
    return JIP_Result(libJIP.eJIP_Table_UpdateRow(byref(var), index, byref(newdata), size))
        
        
libJIP_Networks = []
def libJIP_Cleanup():
    #print "Cleaning up networks"
    for net in libJIP_Networks:
        print "Destorying network %s" % (str(net))
        net.Destroy()
        
atexit.register(libJIP_Cleanup)

    
if __name__ == "__main__":
    from optparse import OptionParser
    parser = OptionParser()
    
    parser.add_option("-m", "--mode", dest="mode",
                      help="Mode: client or server.", default="client")
    
    parser.add_option("-6", "--ipv6", dest="gateway_addr",
                      help="ipv6 Address of gateway node")
    parser.add_option("-4", "--ipv4", dest="gateway_addr4",
                      help="ipv4 Address of gateway")
    parser.add_option("-t", "--tcp", dest="tcp", action="store_true",
                  help="Use TCP to connect to IPv4 gateway", metavar="TCP")
    (options, args) = parser.parse_args()
                    
    if options.gateway_addr is None:
        print "Please specify ipv6 address of gateway node via -6 or --ipv6"
        sys.exit(0)

    if options.mode == "client":
        print "Client mode"
        context = JIP_Context()
        print context

        if (options.gateway_addr4 is not None):
            if (options.tcp == True):
                print "TCP/IPv4 Connection via gateway:", options.gateway_addr4
                print eJIP_Connect4(context, options.gateway_addr4, options.gateway_addr, JIP_DEFAULT_PORT, TCP=True)
            else:
                print "UDP/IPv4 Connection via gateway:", options.gateway_addr4
                print eJIP_Connect4(context, options.gateway_addr4, JIP_DEFAULT_PORT, options.gateway_addr, JIP_DEFAULT_PORT, TCP=False)
        else:
            print eJIP_Connect(context, options.gateway_addr, JIP_DEFAULT_PORT)

        print eJIPService_PersistXMLLoadDefinitions(context)

        print eJIPService_DiscoverNetwork(context)
    
        #(Addresses, NumAddresses) = eJIP_GetNodeAddressList(context, JIP_DEVICEID_ALL)
        #print "Number of addresses: ", NumAddresses
            

        #def trap_handler_callback(Var):
            #print "Callback"
            #print "Var: ", Var
        
        print "List node/mib/var"
        for node in context.Network:
            #print inet_ntop(AF_INET6, node.NodeAddress.sin6_addr)
            print node

            for mib in node:
                print mib
                for var in mib:
                    var.Get()
                    print var
                    #if var.Name == "Mode":
                    #    print var.Trap(trap_handler_callback)
                    #    print node
                    #    print mib
                    #var.Get()
                    #print context.eGetVar(var)
                    #print var   
                    #    print "Data: ", var.Data
                    
                        #eJIP_SetVar(context, var, 0)
                        #var.Data = 1 # [0x15, 0x12, 0x34]
                        #print var  
                        #print "Data: ", var.Data
                        

                    #print var
                    #var.Security = "E_JIP_SECURITY_TEST"
                    #print var
                    
            #Mib = node.LookupMib("BulbControl")
            #Mib = node.LookupMibId(0xfffffe04)
            #if Mib:
                #print Mib
                ##Var = Mib.LookupVar("Mode")
                #Var = Mib.LookupVarIndex(0)
            ##    print Var
                #if Var:
                    #Var.MulticastSet(0, "ff15::f00f")
                    ##Var.Data = 0
                    #print Var
                

        eJIP_GroupJoin(context, "ff15::abc");

        while True:
            time.sleep(1)
        #print "Starting monitor"
        #NetMon = eJIPService_MonitorNetwork(context)

        
        #while True:
            #time.sleep(1)
            #print NetMon
        #print "Done"
        #print eJIPService_PersistXMLSaveDefinitions(context);

    elif options.mode == "server":
        print "Server mode"
        
        context = JIP_Context(E_JIP_CONTEXT_SERVER)

        print "eJIPService_PersistXMLLoadDefinitions(): ", eJIPService_PersistXMLLoadDefinitions(context, "../TestServer/jip_cache_definitions.xml")
        
        print "eJIPserver_Listen(): ", eJIPserver_Listen(context, JIP_DEFAULT_PORT)
        
        print "eJIPserver_NodeAdd(): ", eJIPserver_NodeAdd(context, options.gateway_addr, 0x80100001, "Test Coordinator", "Version 0.1")
        
        for node in context.Network:
            # Set a variable value for clients to read
            mib = psJIP_LookupMibId(node, 0xffffff00)
            var = psJIP_LookupVarIndex(mib, 3)
            print "eJIP_SetVarValue('TX Power', 3): ", eJIP_SetVarValue(var, 3)
            
            print "eJIPserver_NodeGroupJoin(ff15::f00f): ", eJIPserver_NodeGroupJoin(node, "ff15::f00f")
            print "eJIPserver_NodeGroupLeave(ff15::f00f): ", eJIPserver_NodeGroupLeave(node, "ff15::f00f")

        
        while (True):
            pass
        
        
    else:
        print "Unknown operation mode"