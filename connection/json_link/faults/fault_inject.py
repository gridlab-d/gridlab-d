"""
Fault Inject
~~~~~~~~~~~~

A module that enables easy injection of faults into a JSON link exchange, useful
for both our own unit tests and when testing other implementations

Classes:

* :class:`FaultInjectMixIn`
* :class:`MasterFaultInject`
* :class:`SlaveFaultInject`

Related Functions:
  

Notes:

@author: Bryan Palmintier, NREL 2013
"""
# __future__ imports must occur at top of file. They enable using features from
# python 3.0 in older (2.x) versions are are ignored in newer (3.x) versions
# New print function
from __future__ import print_function
# Use Python 3.0 import semantics
from __future__ import absolute_import

__author__ = "Bryan Palmintier"
__copyright__ = "Copyright (c) 2013 Bryan Palmintier"
__license__ = """ 
@copyright: Copyright (c) 2013, Bryan Palmintier
@license: BSD 3-clause --
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:
 1) Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2) Redistributions in binary form must reproduce the above copyright notice, 
    this list of conditions and the following disclaimer in the documentation 
    and/or other materials provided with the distribution.
 3) Neither the name of the National Renewable Energy Lab nor the names of its 
    contributors may be used to endorse or promote products derived from this 
    software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."""

## ===== History ====
#  [Current]             version     date       time     who     Comments
#                        -------  ----------    ----- ---------- ------------- 
__version__, __date__ = "0.3.0a", "2013-10-08"  #18:25   BryanP   Enable dropped packets & bug fixe specified count
# [Older, in reverse order]
# version      date       time     who     Comments
# -------   ----------    ----- ---------- --------------------------------------- 
#  0.2.0a   2013-05-08    13:55   BryanP   Support for json, dict_str, and data fault specs
#  0.1.0a   2013-05-01    21:40   BryanP   Initial Version


#Standard Library imports
import sys
import json

#local imports
from .. import json_link

#===============================================================================
# Module level constants
#===============================================================================

#===============================================================================
# Error (Exception) Class definitions
#===============================================================================

#===============================================================================
# FaultInjectMixIn (module internal)
#===============================================================================
class FaultInjectMixIn(json_link._BaseLink):
    """ Mix in class to enable JSON fault injection by overloading message sending functions"""
    
    fault = {}
    # Create a dictionary with send counts for each message type initialized to zero
    send_count = dict(zip(json_link.MsgTypeList, [0] * len(json_link.MsgTypeList)))

    def sendMsg(self, action, data, data_name='data'):
        """
        Injects a fault into either the JSON link or raw exchange message
        by using the fault data field instead of the proper message
        
        See json fault injection files for example. You should also be
        enter similar strings at the command prompt, but the quoting and
        escaping of special characters can be a pain.
        """
                
        #Track number of (attempted) sends by action
        self.send_count[action] += 1
        
        # Yes, this nested if could be replaced by assigning inject_fault to a
        # long boolean expression, BUT we keep the if structure for future 
        # expansion
        if action in self.fault:
           
            #by default, we expect to inject a fault
            inject_fault = True
            
            #Use alternate action if specified in fault
            if 'action' in self.fault[action]:
                send_action = self.fault[action]['action']
            else:
                send_action = action
            
            #But if the count parameter is set, we only inject the fault for
            #listed number attempts 
            if ('count' in self.fault[action]):
                if type (self.fault[action]['count']) is int:
                    if self.fault[action]['count'] != self.send_count[action]:
                        inject_fault = False
                else:
                    if self.send_count[action] not in self.fault[action]['count']:
                        inject_fault = False
                                            
        else:
            inject_fault = False                

        if inject_fault:
            self.printVerbose(1,"FAULT INJECT: %s"%self.fault[action])
            
            if 'raw' in self.fault[action]:
                output = json_link._BaseLink.sendMsg(self, send_action, 
                                                     self.fault[action]['raw'], 
                                                     data_name, raw_packet=True)
            elif 'json' in self.fault[action]:
                self.printVerbose(1,"WARNING: json fault injection has bugs, try dict_str instead")
                output = json_link._BaseLink.sendMsg(self, send_action, 
                                                     self.fault[action]['json'], 
                                                     data_name, raw_json=True)
            elif 'dict_str' in self.fault[action]:
                output = json_link._BaseLink.sendMsg(self, send_action, 
                                                     json.dumps(self.fault[action]['dict_str']), 
                                                     data_name, raw_json=True)
            elif 'data' in self.fault[action]:
                output = json_link._BaseLink.sendMsg(self, send_action, 
                                                     self.fault[action]['data'], 
                                                     data_name)
            elif 'drop' in self.fault[action]:
                self.printVerbose(1,"Dropping message, but increment msg-id to simulate Comm Link lost")
                #Pretend the packet is sent normally, but lost in cyberspace
                self.msg_num += 1
                output = True
            else:
                self.printVerbose(1,"Unknown fault type, sending normally")
                output = json_link._BaseLink.sendMsg(self, send_action, 
                                                     data, 
                                                     data_name)
        else:
            output = json_link._BaseLink.sendMsg(self, action, data, data_name)
                    
        return output

#===============================================================================
# MasterFaultInject
#===============================================================================
class MasterFaultInject(json_link.MasterLink, FaultInjectMixIn):
    """ JSON Link Master/client with fault injection"""
    
    description_dict = {'application':"JSON_Link_Master_with_Fault_Inject", 
                        'version':__version__, 
                        'modelname':sys.argv[0]
                        }

#===============================================================================
# SlaveLink
#===============================================================================
class SlaveFaultInject(json_link.SlaveLink, FaultInjectMixIn):
    """ JSON Link Slave/server with fault injection"""
    
    #Passed via the init link message, must include 'remote'
    description_dict = {'remote':"JSON Link Slave/Server with Fault Inject", 'version':__version__}
