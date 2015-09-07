/****************************************************************************
 *
 * MODULE:             JIP Web Apps
 *
 * COMPONENT:          JIP Javascript wrapper
 *
 * REVISION:           $Revision: 43426 $
 *
 * DATED:              $Date: 2012-06-18 15:24:02 +0100 (Mon, 18 Jun 2012) $
 *
 * AUTHOR:             Matt Redfearn
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139]. 
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the 
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.

 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

/** Placeholder for JenNet-IP available Border routers */
var JIP_BRList=[];

// Placeholder for active JIP contexts
var JIP_Contexts=[];

function JIP_MakeStatus(description, value)
{
    if (description === undefined)
    {
        description = "Error";
    }
    if (value === undefined)
    {
        value = 0xFF;
    }

    // Return a JIP Status object
    return { Status: { Value: value, Description: description }};
}

/** jQuery queue based manager for ajax requests. */
var JIP_RequestManager = (function() {
     var requests = [];
     var periodic = [];
     
     // Array of callbacks to call when a request is started and completed.
     var cbRequestStart = [];
     var cbRequestDone = [];

     return {
         addCallback: function(when, fn) {
             if (when == "start")
             {
                 cbRequestStart.push(fn);
             }
             else if (when == "done")
             {
                 cbRequestDone.push(fn);
             }
         },
         
         
        addReq:  function(opt) {
            if(typeof(opt.interval) == 'undefined')
            {
                for (var fnidx in cbRequestStart)
                {
                    // Call registered request start callbacks
                    cbRequestStart[fnidx](opt.op_str);
                }
                
                requests.push(opt);
            }
            else
            {
                var data = [];
                for (var key in opt.params) {
                    data.push(key + "=" + opt.params[key]);
                };
                opt.data = data.join('&');
                var interval = setInterval(function() {
                    $.ajax(opt).done(function (data, textStatus, jqXHR) {
                        opt.callback(data);
                    });
                }, opt.interval);
                periodic.push(interval);
            }
        },
        removeReq:  function(opt) {
            if( $.inArray(opt, requests) > -1 )
                requests.splice($.inArray(opt, requests), 1);
        },
        run: function() {
            var self = this,
                orgSuc;

            if( requests.length ) {
                // Build up the request string from the parameters.
                var data = [];
                for (var key in requests[0].params) {
                    data.push(key + "=" + requests[0].params[key]);
                };
                
                // Set the stay awake bit if there are further events in the queue going to the same node.
                for (var r in requests.slice(1))
                {
                    if (requests[r].params["nodeaddress"] !== undefined)
                    {
                        if (requests[r].params["nodeaddress"] == requests[0].params["nodeaddress"])
                        {
                            data.push("stayawake=yes");
                            break;
                        }
                    }
                }
                                
                requests[0].data = data.join('&');
                
                // Set the context of the ajax request, so that we can use it as "this" in the 
                // always callback to ensure we remove the right request from the list.
                requests[0].context = requests[0].data;
                
                // Start the request & register the callbacks
                requests[0].jqXHR = $.ajax(requests[0]).done(function (data, textStatus, jqXHR) {
                    // Call user specified callbacks first
                    
                    if (requests[0])
                    {
                        requests[0].callback(requests[0].op_str, data, requests.length - 1);

                        // And then registered completion callbacks
                        for (var fnidx in cbRequestDone)
                        {
                            // Call registered request start callbacks
                            cbRequestDone[fnidx](requests[0].op_str, data, requests.length - 1);
                        }
                    }
                    
                }).fail(function(jqXHR, textStatus, errorThrown) {
                    var status = JIP_MakeStatus(errorThrown);

                    // Call user specified callbacks first
                    requests[0].callback(requests[0].op_str, status, requests.length - 1);

                    // And then registered completion callbacks
                    for (var fnidx in cbRequestDone)
                    {
                        // Call registered request start callbacks
                        cbRequestDone[fnidx](requests[0].op_str, status, requests.length - 1);
                    }
                    
                }).always(function() {
                    // Always remove the completed request from the list.
                    if (requests.length > 0)
                    {
                        for (var r in requests)
                        {
                            if (this == requests[r].data)
                            {
                                requests.shift();
                            }
                        }
                    }
                    
                    self.run.apply(self, []);
                });
            } else {
              self.tid = setTimeout(function() {
                 self.run.apply(self, []);
              }, 100);
            }
        },
        stop:  function() {
            self.clear();
            clearTimeout(this.tid);
        },
        clear: function() {
            for (var i in requests)
            {
                if (requests[i].jqXHR !== undefined)
                {
                    // Request is in progress, so call the abort method on it
                    requests[i].jqXHR.abort();
                }
                else
                {
                    // Request hasn't been actioned yet, so call it's callback directly.
                    requests[i].callback(JIP_MakeStatus("Cancelled"));
                }
            }
            requests.length = 0;
            
            for (var i in periodic)
            {
                clearInterval(i);
            }
            periodic.length = 0;
        }
     };
}());
JIP_RequestManager.run();

function JIP_CancelPendingRequests()
{
    JIP_RequestManager.clear();
}

function JIP_Request(params, callback, op_str, interval)
{
    JIP_RequestManager.addReq({
        type: 'POST',
        url: '/cgi-bin/JIP.cgi',
        params: params,
        callback: callback,
        op_str: op_str,
        interval: interval
    });
    return;
}


function JIP_GetVersion(callback) 
{ 
    var request; 
    request = {"action":"getVersion"};
    
    JIP_Request(request, function(op_str, Result) {
        typeof callback === 'function' && callback(Result);
    }, "Read Version");
}


function JIP_DiscoverBRs(callback) 
{ 
    var request; 
    request = {"action": "discoverBRs"};
    
    JIP_Request(request, function(op_str, Result) {
        JIP_BRList = Result.BRList;
        typeof callback === 'function' && callback(Result.Status);
    }, "Discovering available border routers");
}

/** Cache of known device IDs. */
var JIP_DeviceCache = new function() {
    this.devices = new Array();

    this.cbNode = new Array();
    this.cbMib = new Array();
    this.cbVar = new Array();

    this.populate_node = function(JIP_Context, node) {
        
        node.parentContext = JIP_Context;
        
        // Add function to return the device types of this node.
        node.DeviceTypes = function()
        {
            var res = []
            var t = this.LookupVars({mibName: "DeviceID", varName: "DeviceTypes"})[0];
            
            if (t)
            {
                // Remove initial 0x & split string into array of 16bit strings.
                res = t.Data.slice(2).match(/.{1,4}/g);
                $.each(res, function(i, v) {
                    res[i] = parseInt(v, 16);
                });
            }
            return res;
        }
        
        // Add function to determine if a node is one of a list of device types
        node.isDeviceType = function(intypes)
        {
            var searchtypes = [].concat(intypes);
            var nodetypes = this.DeviceTypes();
            
            for (i in searchtypes)
            {
                if (nodetypes.indexOf(searchtypes[i]) > -1)
                {
                    return true;
                }
            };
            return false;
        }
        
        // Add function to lookup variables matching filters in this node.
        node.LookupVars = function(params)
        {
            var res = [];
            var node = this;

            for (var mibidx in node.MiBs)
            {
                var mib = node.MiBs[mibidx];
                
                if (((params.mibID      === undefined) && (params.mibName   === undefined))     ||
                    ((params.mibName    === undefined) && (mib.mibID        == params.mibID))   ||
                    ((params.mibID      === undefined) && (mib.Name         == params.mibName)))
                {
                    for (var varidx in mib.Vars)
                    {
                        var variable = mib.Vars[varidx];
                        
                        if (((params.varIndex   === undefined) && (params.varName   === undefined))         ||
                            ((params.varName    === undefined) && (variable.Index   == params.varIndexI))   ||
                            ((params.varIndex   === undefined) && (variable.Name    == params.varName)))
                        {
                            res.push(variable);
                        }
                    }
                }
            }
            return res;
        }
        
        
        for (var mibidx in node.MiBs)
        {
            var mib = node.MiBs[mibidx];

            // Set reference to parent node
            mib.parentNode = node;
            
            for (var varidx in mib.Vars)
            {
                var variable = mib.Vars[varidx];
                
                // Set reference to parent variable
                variable.parentMib = mib;
                
                // Add function to call JIP_SetVar via a method on the variable
                variable.SetVar = function(value, address, callback, user) {
                    this.parentMib.parentNode.parentContext.SetVar(this, value, address, callback, user);
                }
                
                // Add function to call JIP_GetVar via a method on the variable
                variable.GetVar = function(callback, user, interval)  {
                    this.parentMib.parentNode.parentContext.GetVar(this, callback, user, interval);
                }
            
                for (var fnidx in this.cbVar)
                {
                    // Call registered per-var callbacks
                    this.cbVar[fnidx](variable);
                }
            }
            
            for (var fnidx in this.cbMib)
            {
                // Call registered per-mib callbacks
                this.cbMib[fnidx](mib);
            }
        }
        
        for (var fnidx in this.cbNode)
        {
            // Call registered per-device callbacks
            this.cbNode[fnidx](node);
        }
    };

}




/** Object representing a JIP context. */
function JIP_Context() {
    this.br6 = null;
    this.Network = { Nodes: [] };
}


JIP_Context.prototype.Connect = function(br)
{
    this.br6 = br;
}


JIP_Context.prototype.Discover = function(callback) 
{
    if (this.br6 != undefined)
    {
        var request = {"action":"discover", "BRaddress": this.br6};

        JIP_Request(request, $.proxy(function(op_str, Result) {
            this.lastStatus = Result.Status;
            
            if (Result.Status.Value == 0)
            {
                this.Network = Result.Network;
                
                for (var nodeidx in this.Network.Nodes)
                {
                    var node = this.Network.Nodes[nodeidx];
                    
                    var status = JIP_DeviceCache.populate_node(this, node);
                }
            }          
            
            typeof callback === 'function' && callback(Result.Status, this);
        }, this), "Discovering available devices");
    }
}


JIP_Context.prototype.GetVar = function(variable, callback, user, interval) 
{ 
    var request = { "action":"GetVar", 
                    "BRaddress": this.br6,
                    "nodeaddress": variable.parentMib.parentNode.IPv6Address,
                    "mib": variable.parentMib.Name,
                    "var": variable.Name,
                    "refresh": "no"
    };
    
    JIP_Request(request, $.proxy(function(op_str, Result) {
        try {
            this.lastStatus = Result.Status;
            
            if (Result.Status.Value != 0)
            {
                typeof callback === 'function' && callback(Result.Status, user, variable);
                return;
            }
            var Nodes = Result.Network["Nodes"];
            if (Nodes.length == 0)
            {
                typeof callback === 'function' && callback(JIP_MakeStatus("Unknown node"), user, "?");
                return;
            }
            var NewValue = Nodes[0]["MiBs"][0]["Vars"][0]["Data"];
            if (NewValue !== undefined)
            {
                for (var nodeidx in this.Network.Nodes)
                {
                    var node = this.Network.Nodes[nodeidx];
                    
                    if (node.IPv6Address == Result.Network.Nodes[0].IPv6Address)
                    {
                        for (var mibidx in node.MiBs)
                        {
                            var mib = node.MiBs[mibidx];
                            if (mib.ID == Result.Network.Nodes[0].MiBs[0].ID)
                            {
                                for (var varidx in mib.Vars)
                                {
                                    var vari = mib.Vars[varidx];
                                    
                                    if (vari.Index == Result.Network.Nodes[0].MiBs[0].Vars[0].Index)
                                    {
                                        vari.Data = NewValue;
                                        typeof callback === 'function' && callback(Result.Status, user, vari);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        catch (err)
        {
            alert(err.message);
        }
    }, this), "Read " + variable.parentMib.parentNode.IPv6Address + ", Mib " + variable.parentMib.Name + ", Var " + variable.Name, interval);
}


JIP_Context.prototype.SetVar = function(variable, value, address, callback, user) 
{ 
    if (address === undefined)
    {
        address = variable.parentMib.parentNode.IPv6Address;
    }
    
    var request = { "action":"SetVar", 
                    "BRaddress": this.br6,
                    "nodeaddress": address,
                    "mib": variable.parentMib.Name,
                    "var": variable.Name,
                    "value": value,
                    "refresh": "no"
    };
    
    JIP_Request(request, function(op_str, Result) {
        this.lastStatus = Result.Status;
        typeof callback === 'function' && callback(Result.Status, user);
    }, "Update " + address + ", Mib " + variable.parentMib.Name + ", Var " + variable.Name + " to " + value);
}


JIP_Context.prototype.LookupVars = function(params)
{
    var isMulticast = 0;
    var res = [];
    var my_params = Object.create(params); // Take a copy as we may modify the IPv6 address on multicasts.
    
    if ((my_params.IPv6Address) && (my_params.IPv6Address.toLowerCase().indexOf("ff") == 0))
    {
        isMulticast = 1;
        my_params.IPv6Address = undefined;
    }
    
    for (var nodeidx in this.Network.Nodes)
    {
        var node = this.Network.Nodes[nodeidx];
        
        if ((my_params.IPv6Address === undefined) || (node.IPv6Address == my_params.IPv6Address))
        {
            // Add this node's matches
            res = res.concat(node.LookupVars(my_params));
            
            if (res.length && isMulticast)
            {
                /* Return just the one match for a multicast */
                return res;
            }
        }
    }
    return res;
}


function JIP_LookupVars(params)
{
    var res = [];
    for (var ctxidx in JIP_Contexts)
    {
        res.push.apply(res, JIP_Contexts[ctxidx].LookupVars(params));
    }
    return res;
}


JIP_Context.prototype.LookupDeviceTypes = function(params)
{
    var isMulticast = 0;
    var res = [];
    var my_params = Object.create(params); // Take a copy as we may modify the IPv6 address on multicasts.
    var searchtypes = [].concat(my_params.DeviceTypes);
    
    if ((my_params.IPv6Address) && (my_params.IPv6Address.toLowerCase().indexOf("ff") == 0))
    {
        isMulticast = 1;
        my_params.IPv6Address = undefined;
    }
    
    for (var nodeidx in this.Network.Nodes)
    {
        var node = this.Network.Nodes[nodeidx];
        
        if ((my_params.IPv6Address === undefined) || (node.IPv6Address == my_params.IPv6Address))
        {
            // See if this node matches the requested node types
            var nodetypes = node.DeviceTypes();
    
            for (i in searchtypes)
            {
                if (nodetypes.indexOf(searchtypes[i]) > -1)
                {
                    res = res.concat([node]);
                    break;
                }
            };
            
            if (res.length && isMulticast)
            {
                /* Return just the one match for a multicast */
                return res;
            }
        }
    }
    return res;
}


function JIP_LookupDeviceTypes(params)
{
    var res = [];
    for (var ctxidx in JIP_Contexts)
    {
        res.push.apply(res, JIP_Contexts[ctxidx].LookupDeviceTypes(params));
    }
    return res;
}
