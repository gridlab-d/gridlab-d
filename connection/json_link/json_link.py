"""
json_link
~~~~~~~~~
A python package for GridLAB-D data (JSON) link handshaking (over UDP and TCP)

Core Classes:

* :class:`MasterLink`
* :class:`SlaveLink`

Related Functions:
  

Notes:

* Should work with Python 2.6+ and 3.x. Tested with 2.7.2 and 3.3
* Currently only supports IPv4
* The SLAVE will handle connections from any number of masters, BUT since the
  code is only single threaded each processing (including delay) are
  blocking. Hence, the SLAVE may miss packets if it is busy processing
  (or waiting to send) a different reply.
    
@author: Bryan Palmintier, NREL 2013
"""
# __future__ imports must occur at top of file. They enable using features from
# python 3.0 in older (2.x) versions. They are ignored in newer (3.x) versions
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
#  [Current]              version     date       time     who     Comments
#                         -------  ----------    ----- ---------- -------------------------------------
__version__, __date__ =  "1.3.0", "2014-04-05"  #10:00   BryanP   Add statistics for rx/tx/missing
# [Older, in reverse order]
# version      date       time     who     Comments
# -------   ----------    ----- ---------- ------------------------------------- 
#  1.2.1    2014-04-05    02:30   BryanP   Streamline calling parent class methods when overriding in SlaveLink
#  1.2.0    2014-04-05    02:17   BryanP   Slave: Tx msg_id match most recent Rx msg_id
#  1.1.3b   2014-04-05    00:17   BryanP   Nominal voltage and random range for dummyData.
#  1.1.2b   2013-09-06    23:40   BryanP   Add support for char1024 (and hence enums) in dummyData. including random pv_control_mode
#  1.1.1b   2013-09-01    20:55   BryanP   Include Python in init message model/remote names
#  1.1.0b   2013-07-02    13:45   BryanP   Add voltage divisor option for scaling current outputs
#  1.0.0b   2013-06-04    00:40   BryanP   Master: remove handshake delay, Slave: Rx after wait for better timing, new wait to send error state
#  0.9.9a   2013-05-31    14:48   BryanP   BUGFIX: use latest msg for syncs. NEW: Report missing syncs. Timeout if no sync for long enough
#  0.9.8a   2013-05-09    22:28   BryanP   BUGFIX: remove redundant looping causing rx&init errors, Extract (master) internal process. Clean verbose=3
#  0.9.7a   2013-05-09    02:00   BryanP   print: reconfigurable stdout/stderr, Timeouts for everything, module timing config constants
#  0.9.6a   2013-05-08    14:00   BryanP   BUGFIX: TCP handling, Early prep for unittests
#  0.9.5a   2013-05-07    13:36   BryanP   BUGFIX: corrected slave timing. Consolidated master rx & wait. Further I/O cleanup
#  0.9.4a   2013-05-06    11:00   BryanP   NEW: verbosePrint(), clean-up output. run_max to float for Inf
#  0.9.3a   2013-05-06    00:20   BryanP   Schema through constructors. Schema I/o cleanup. NEW: loadJsonStrOrFile, Doc partial convert to rst
#  0.9.2b   2013-04-29    22:20   BryanP   Move __init__ to _BaseLink. Master retry Init until slave ready
#  0.9.1b   2013-04-28    22:20   BryanP   Corrected slave timing & start/term sequencing. Rename gld_tcp_udp to raw_xchg with *Xchg*
#  0.9.0b   2013-04-27    02:00   BryanP   Full State Machine Functionality
#  0.1.0a   2013-04-25    17:20   BryanP   Initial Code

#List specific exports to avoid access to the libraries we import
# Here we just list our core classes and will append error messages and
# related functions next to their definitions
__all__ = ['MasterLink', 'SlaveLink']

#Standard Library imports
import sys
import json
import timeit
import time
import datetime as dt
import random
import re
import os
import copy

#local imports
from .xchg import raw_xchg

#===============================================================================
# Module level constants
#===============================================================================
DEFAULT_SLAVE_TIMESTEP = 0.5
DEFAULT_MASTER_TIMESTEP = 1.0
DEFAULT_DELAY_DIVISOR = 10.0    #timestep/this = default delay 
DEFAULT_RUN_MAX = 50

#TODO: pass as parameters & use defaults here
INIT_RETRY_DELAY = 1.0     #Seconds between init retries
INIT_RETRY_TIMEOUT = 60.0  #Seconds before giving up on init
INIT_RETRY_MAX = int(round(INIT_RETRY_TIMEOUT/INIT_RETRY_DELAY))

SYNC_TIMEOUT = 60.0 #Number of seconds to continue running without a sync

_pkg_root = os.path.abspath(os.path.normpath(os.path.dirname(__file__) + ''))


#===============================================================================
# Error (Exception) Class definitions
#===============================================================================
#add exceptions to export list
__all__.extend(['JsonLinkError', 'JsonLinkTimeoutError', 'JsonLinkNoDataError'])
__all__.extend(['JsonLinkBadWrapperError', 'JsonLinkRemoteError'])
__all__.extend(['JsonLinkOldMsgError', 'JsonLinkActionMismatchError'])

class JsonLinkError(IOError):
    """Base class for exceptions in the gld_json_link module"""
    code = 2000
    
class JsonLinkTimeoutError(JsonLinkError):
    """Timeout waiting for reply"""
    code = JsonLinkError.code + 1

class JsonLinkNoDataError(JsonLinkError):
    """Expected data is empty"""
    code = JsonLinkError.code + 2

class JsonLinkBadWrapperError(JsonLinkError):
    """Malformed wrapper"""
    code = JsonLinkError.code + 3

class JsonLinkRemoteError(JsonLinkError):
    """received a (reasonably) well formed error packet from the remote machine"""
    code = JsonLinkError.code + 4

class JsonLinkOldMsgError(JsonLinkError):
    """Received a message with id older than a previously processed message"""
    code = JsonLinkError.code + 5

class JsonLinkActionMismatchError(JsonLinkError):
    """Incorrect handshaking order"""
    code = JsonLinkError.code + 6

class JsonLinkRunMaxError(JsonLinkError):
    """(Slave) number of runs exceeded"""
    code = JsonLinkError.code + 7

#===============================================================================
# Additional helper classes
#===============================================================================
__all__.extend(['MsgTypeList'])
MsgTypeList = ['init', 'input', 'output', 'start',
                                      'sync','term','error']

#===============================================================================
# _BaseLink (module internal)
#===============================================================================
class _BaseLink(object):
    """ Base class for JSON link subclasses"""
    
    #internal state machine
    state = 'beginState'
    _is_slave_server = False
    
    in_schema = None
    out_schema = None
    remote_data = None
    
    #handshaking
    msg_num = 0
    remote_msg_num = 0
    
    response_str_out = None     #override in subclass
    response_str_in = None     #override in subclass
    retry = 0
    
    #low level data exchange object
    _xchg = None
        
    #terminal I/O control, alternate stdout/stderr for unittests
    opt_verbose = 2
    opt_run_line_end = '\n'
    opt_show_time = False
    opt_volt_divisor = 1.0      #Voltage dividor resistance (complex) for current returns with matching voltages
    opt_volt_nom = 240          #Nominal voltage for random voltage numbers, complex = OK
    opt_volt_range = 0.05       #Per unit range +/- nominal for random values
    _my_stdout = sys.stdout
    _my_stderr = sys.stderr
    
    ignore_old = False
    ignore_dup = False
    
    #Internal process (Run) timing information
    _run_end_time = None
    run_timestep = 1.0
    run_delay = 0.1
    run_max = DEFAULT_RUN_MAX
    _no_sync_timeout = None

    #Track link statistics
    num_tx = 0
    num_rx = 0
    num_miss = 0
    run_count = 0
    sync_count = 0
    
    def __init__(self, run_timestep=1.0, run_delay=None, run_max=DEFAULT_RUN_MAX, 
                 opt_volt_divisor=None, opt_volt_nom=None, opt_volt_range=None,
                 opt_verbose=2, opt_show_time = None,
                 my_stdout=sys.stdout, my_stderr=sys.stderr,
                 **Xchg_args):
        """Constructor"""
        self.run_timestep = run_timestep
        if run_delay is None:
            self.run_delay = run_timestep/DEFAULT_DELAY_DIVISOR
        else:
            self.run_delay = run_delay
        
        if run_max == -1:
            self.run_max = float('inf')
        else:
            self.run_max = run_max
        
        if opt_volt_divisor is not None:
            self.opt_volt_divisor = opt_volt_divisor
        
        if opt_volt_nom is not None:
            self.opt_volt_nom = opt_volt_nom
        
        if opt_volt_range is not None:
            self.opt_volt_range = opt_volt_range

        self.opt_verbose = opt_verbose
        if opt_show_time is not None:
            self.opt_show_time = opt_show_time
            Xchg_args['opt_show_time'] = opt_show_time

        self._my_stdout=my_stdout
        self._my_stderr=my_stderr
        
        if self.opt_verbose == 1:
            self.opt_run_line_end = '\r'
        else:
            self.opt_run_line_end = '\n'
            
        if 'opt_raw' in Xchg_args:
            if Xchg_args['opt_raw']:
                self.opt_run_line_end = '\n'
        
        self._setupOurXchg(**Xchg_args)        

    def _setupOurXchg(self, **Xchg_args):
        """Must be implemented by subclasses to configure their raw exchange layer"""
        pass
    
    def go(self):
        """Method to beginState link execution"""
        pass

    def wrapJson(self, action, data, data_name='data'):
        """encapsulate data (typically a nested data dictionary) in side a JSON link wrapper"""
        
        #build message
        msg = {self.response_str_out:action}
        if data is not None:
            msg[data_name] = data
        msg['id'] = self.msg_num
        
        #convert to JSON string
        return json.dumps(msg)
    
    def unwrapJson(self, action, msg, data_name='data', ignore_msg_num=False):
        """check JSON link wrapper and decode data (typically a nested data dictionary)"""
        if not msg:
            raise JsonLinkNoDataError('No Data received')
        print(msg)
        msg = json.loads(msg)
        
        if type(msg) is not dict:
            raise JsonLinkBadWrapperError('JSON Message is not a dictionary object')
        
        if 'error' in msg:
            err_msg = ''
            if 'msg' in msg['error']:
                err_msg += msg['error']['msg']
            if 'code' in msg['error']:
                err_msg += " [code=%s]"%msg['error']['code']
            if err_msg == '':
                err_msg = 'Remote Error Received, no message param or code'  
            raise JsonLinkRemoteError(err_msg)
        
        if not ignore_msg_num:
            if not 'id' in msg:
                raise JsonLinkBadWrapperError('Missing id field in JSON message')
    
            # Process id (sequence number)
            msg_id = int(msg['id']) 
            if msg_id < self.remote_msg_num:
                if self.ignore_old:
                    self.printVerbose(2,'WARNING: Ignoring packet with old message id %d (expected %d)'%(msg_id, self.remote_msg_num + 1))
                    return None
                else:
                    raise JsonLinkOldMsgError('A newer JSON message has been received (id=%d expected=%d)'%(msg_id, self.remote_msg_num + 1))
            elif msg_id == self.remote_msg_num:
                if self.ignore_dup:
                    self.printVerbose(2,'WARNING: Ignoring duplicate message (id=%d)'%(msg_id))
                    return None
                else:
                    raise JsonLinkOldMsgError('Duplicate message (id=%d)'%(msg_id))
            self.remote_msg_num = msg_id

        #check action/response_str
        if self.response_str_in not in msg:
            raise JsonLinkBadWrapperError('Wrong type of JSON message (missing %s)'%(self.response_str_in))
        
        if msg[self.response_str_in] != action:
            return (msg[self.response_str_in], None)

        
        if data_name is None:
            return (action, None)

        if data_name not in msg:
            raise JsonLinkBadWrapperError('JSON Message missing data payload (%s)'%(data_name))
        
        return (action, msg[data_name])
        

    def tx(self, msg, increment=True):
        """Transmit preformatted data message"""
        self._xchg.send(msg)
        self.num_tx +=1
        
        if increment:
            self.msg_num += 1
    

    def rx(self, skip_to_last=False):
        """Wait for and receive data"""

        try:
            msg = self._xchg.receive(skip_to_last)
        except raw_xchg.RawXchgTimeoutError:
            raise JsonLinkTimeoutError('Receive Timeout')
        except raw_xchg.RawXchgError as err:
            raise JsonLinkRemoteError(err.args[0])
        self.num_rx +=1

        self.printVerbose(3,' Link Rx: %s'%msg)
        return msg

    def handleAltAction(self, action, msg, data_name='data', out_data=None):
        
        _actual_action, data = self.unwrapJson(action, msg, data_name, ignore_msg_num=True)
        self.sendMsg(action, out_data, data_name)
        
        return data

    def sendMsg(self, action, data, data_name='data', 
                raw_json=False, raw_packet=False):
        """Helper function to send general messages"""
        if raw_packet:
            self._xchg.send(data, add_header=False)
            self.msg_num += 1
            return True
        elif raw_json:
            try:
                self.tx(data)
            except raw_xchg.RawXchgError:
                return False
        else:
            msg = self.wrapJson(action, data, data_name)
            try:
                self.tx(msg)
            except raw_xchg.RawXchgError as err:
                self.printVerbose(3,' Error in Tx: %s'%(err.args[0]))
                return False
            self.printVerbose(3,' Link Tx %s'%(msg))
        return True

#------- Simple Message functions. Designed to be overloaded for fault injection
    def sendInit(self, data=None):
        """Send INIT message"""
        return self.sendMsg('init', data, 'params')

    def sendInput(self, data=None):
        """Send INPUT schema message"""
        return self.sendMsg('input', data, 'schema')

    def sendOutput(self, data=None):
        """Send OUTPUT schema message"""
        return self.sendMsg('output', data, 'schema')

    def sendStart(self, data=None):
        """Send START message"""
        return self.sendMsg('start', data)

    def sendSync(self, data=None):
        """Send SYNC message"""
        return self.sendMsg('sync', data)

    def sendTerm(self, data=None):
        """Send TERM message"""
        return self.sendMsg('term', data)

    def sendError(self, action, code=None, err_str=''):
        """Helper function to send error messages"""
        self.printVerbose(1,'ERROR: %s [code=%d]'%(err_str,code))

        err_dict = {'code':code, 'msg':err_str}
        err_msg = self.wrapJson(action, err_dict, 'error')

        self.tx(err_msg)
        return True

    def runWaitForTimeStep(self):
        while timeit.default_timer() < self._run_end_time:
            time.sleep(self.run_timestep/1000)

        #Find next time period end.
        self._run_end_time += self.run_timestep
        
        #Ensure that the next time end is in the future and warn if missed step
        while timeit.default_timer() > self._run_end_time:
            self.printVerbose(2,'Oops: that run step was too long. Skipping a step to resync')
            self._run_end_time += self.run_timestep

    def printVerbose(self, verbose_min=1, *args, **kwargs):
        """Print if opt_verbose >= verbose_min"""
        if 'file' not in kwargs:
            kwargs['file'] = self._my_stdout

        if self.opt_verbose >= verbose_min:
            if self.opt_show_time:
                #Print current minute:second.microsecond
                # Note: all *args to print() are printed, **kwargs for configuration
                time_opts = copy.copy(kwargs)
                time_opts['end'] = ''
                print(dt.datetime.now().strftime('%M:%S.%f- '), **time_opts)
            print(*args, **kwargs)
        
            kwargs['file'].flush()

    def printLinkStats(self):
        self.printVerbose(1,"  Link Stats: run: %d, sync: %d, rx:%d, tx:%d, miss:%d\n"%( 
                  self.run_count, self.sync_count, self.num_rx, self.num_tx, self.num_miss))

    def dummyData(self, schema):
        """Build up dummy json data package based on the provided schema"""
        pv_control_mode_list = [ "NONE", "CONSTANT_PQ", "CONSTANT_PF", "CONSTANT_V", "VOLT_VAR"]
        
        data = dict()
        if type(schema) is dict:
            for field in schema:
                f_info = schema[field].split()
                f_type = f_info[0]
                if f_type ==  'timestamp':
                    data[field] = dt.datetime.now().isoformat(' ')
                elif f_type ==  'complex':
                    f_property = re.split('[._]', f_info[1])
                    #For current outputs, check for a matching voltage input, 
                    # if found, act like a resistor and scale it
                    if 'current' in f_property:
                        # replace our first letter (presumably I) by V for lookup in remote data
                        v_data_name = 'V'+field[1:]
                        if v_data_name in self.remote_data:
                            data[field] = str(complex(self.remote_data[v_data_name]) / self.opt_volt_divisor)[1:-1]
                            continue
                    elif 'voltage' in f_property:
                        v_rand = self.opt_volt_nom * (1 - self.opt_volt_range + 2*self.opt_volt_range * random.random())
                        data[field] = str(complex(v_rand))[1:-1]
                        continue
                        
                    #Default for complex is a random number for both real and imaginary parts        
                    data[field] = str(complex(random.random(),random.random()))[1:-1]
                elif f_type == 'char1024':  #Used for enumerations
                    if f_info[1].split('.')[1] == 'PV_control_mode':
                        #Randomly select a control mode
                        data[field] = pv_control_mode_list[int(random.random() * len(pv_control_mode_list))]
                    else:
                        data[field] = 'Hello?'
                else:
                    data[field] = random.random()
        return data
    
#===============================================================================
# MasterLink
#===============================================================================
class MasterLink(_BaseLink):
    """ Implements a GridLAB-D JSON Link Master/client state machine"""
    
    _is_slave_server = False

    response_str_out = 'method'
    response_str_in = 'result'

    description_dict = {'application':"Python_JSON-Link_Master/client_Tester", 
                        'version':__version__, 
                        'modelname':sys.argv[0]
                        }
    in_schema = {"clock": "timestamp global.clock",
                 "y0": "double test_0.y",
                 "y1": "double test_1.y"
                 }
    out_schema = {"clock": "timestamp global.clock",
                  "x0": "random test_0.x",
                  "x1": "random test_1.y"
                  }

    send_in_schema = False
    send_out_schema = False

    local_data = None

    _run_send = True
        
    def __init__(self, 
                 in_schema=None, 
                 out_schema=None,
                 schema_to_send='', **Other_args):
        """MASTER Constructor"""
        _BaseLink.__init__(self, **Other_args)
        schema_to_send = schema_to_send.upper()
        
        if schema_to_send == "BOTH" or schema_to_send == "IN":
            self.send_in_schema = True
        if schema_to_send == "BOTH" or schema_to_send == "OUT":
            self.send_out_schema = True

        if in_schema is not None:
            self.in_schema = in_schema 
        if out_schema is not None:
            self.out_schema = out_schema 

    def _setupOurXchg(self, **Xchg_args):
        """Setup the raw MASTER data exchange"""
        self._xchg = raw_xchg.MasterXchg(**Xchg_args)
        

    def go(self):
        """MASTER: Begin JSON link state machine execution"""
        old_state = self.state
        while True:
            if self.state == 'beginState':
                new_state = self.beginState(old_state)
            elif self.state == 'waitInitReplyState':
                new_state = self.waitInitReplyState(old_state)
            elif self.state == 'decideInSchema':
                new_state = self.decideInSchema(old_state)
            elif self.state == 'waitInSchemaReplyState':
                new_state = self.waitInSchemaReplyState(old_state)
            elif self.state == 'decideOutSchema':
                new_state = self.decideOutSchema(old_state)
            elif self.state == 'waitOutSchemaReplyState':
                new_state = self.waitOutSchemaReplyState(old_state)
            elif self.state == 'waitStartReplyState':
                new_state = self.waitStartReplyState(old_state)
            elif self.state == 'runState':
                new_state = self.runState(old_state)
            elif self.state == 'waitTermReplyState':
                new_state = self.waitTermReplyState(old_state)
            elif self.state == 'endState':
                self.printLinkStats()
                self.printVerbose(2,'END\n')
                break
            else:
                raise JsonLinkError('Invalid state %s'%self.state)

            old_state = self.state
            if new_state is not None:
                self.state = new_state
        
        self._xchg.close()
        return {'run_count': self.run_count}


    def sendAndWait(self, action, out_data_type='data', in_data_type='__SAME__',
                    to_send=None, max_retry=1, 
                    send_retry_delay = None, receive_retry_delay = None,
                    retry_msg=''):
        """(MASTER) Waits for slave reply and handles errors and retry"""
        
        # -- Handle input parameters --
        if max_retry > 1 and retry_msg == '' and out_data_type is not None:
            retry_msg = out_data_type.upper()
        
        #Note using __SAME__ as a flag, since None is a valid in_data_type
        if in_data_type == '__SAME__':
            in_data_type = out_data_type
        
        if send_retry_delay is None:
            send_retry_delay = self.run_timestep

        if receive_retry_delay is None:
            receive_retry_delay = send_retry_delay/1000

        sendFn = getattr(self, 'send'+action.capitalize())
            
        # -- send, wait to receive, repeat --
        data = None
        msg_sent = False
        reply_is_good = False
                    
        for retry in range(max_retry):
            end_time = timeit.default_timer()+send_retry_delay

            #Attempt to send & repeat if cannot send
            if not sendFn(to_send): # "Side" effect: Sends packet
                self.printVerbose(2,'WARNING: Could not send %s'%action)
                if retry < max_retry -1:
                    time.sleep(max(0,end_time-timeit.default_timer()))
                continue
            
            msg_sent = True

            self.printVerbose(2,'--> %s sent: %s'%(action.upper(), to_send))
            
            #Wait for reply & handle retrys for low-level timeouts
            msg = None
            at_least_once = False
            while timeit.default_timer() < end_time or not at_least_once:
                at_least_once = True
                time.sleep(receive_retry_delay)
                try:
                    msg = self.rx()
                except JsonLinkTimeoutError:
                    continue
                except (JsonLinkRemoteError, raw_xchg.RawXchgError) as err:
                    self.printVerbose(1,'(Receive) ERROR: %s [code=%d]'%(err.args[0], err.code))
                    return err, None
                break

                
            # At this point we may or may not have a message, but unwrapJson
            # will check for no data and raise an error that we handle
            try:
                actual_action, data = self.unwrapJson(action, msg, in_data_type)
            except JsonLinkNoDataError:
                self.printVerbose(2,'\nWARNING: No Response for %s'%retry_msg)
                if retry < max_retry -1:
                    time.sleep(max(0,end_time-timeit.default_timer()))
                continue
            except JsonLinkError as err:
                self.printVerbose(1,'(Remote) ERROR: %s [code=%d]'%(err.args[0], err.code))
                return err, None
    
            if actual_action == action:
                #Yeah we got (seemingly) good data!
                reply_is_good = True
                break
            else:
                if retry < max_retry -1:
                    self.printVerbose(2,'WARNING: Received %s reply. Ignoring and resending %s'%(actual_action, action.upper()))
                    time.sleep(max(0,end_time-timeit.default_timer()))
                else:
                    self.printVerbose(2,'WARNING: Received %s reply. (Not %s)'%(actual_action, action.upper()))

        #At this point we have either run out of retries or successfully received data
        if not reply_is_good:
            if msg_sent:
                err_str = 'No %s reply'%action
            else:
                err_str = 'Could not send %s'%action
                
            self.printVerbose(1,'ERROR: %s'%err_str)
            return JsonLinkTimeoutError(err_str), None

        return None, data


    def beginState(self, old_state):
        """(MASTER) begin state"""
        self.printVerbose(2,'BEGIN')
        try:
            self._xchg.setupXchg()
        except raw_xchg.RawXchgSetupError:
            pass
        
        self.msg_num = 1

        return 'waitInitReplyState'

    def waitInitReplyState(self, old_state):
        """(MASTER) waitInitReplyState state"""
        if old_state != 'waitInitReplyState':
            self.printVerbose(2,'\nSTATE: waitInitReplyState')
            self.printVerbose(1,"Waiting for slave's INIT response...")
        
        err, remote_info = self.sendAndWait('init', 'params',
                                            to_send = self.description_dict, 
                                            max_retry = INIT_RETRY_MAX,
                                            send_retry_delay = INIT_RETRY_DELAY, 
                                            retry_msg='INIT. Has the slave been started?')
        if err is not None:
            return 'endState'

        self.printVerbose(1,"--> Linked with REMOTE: %s\n"%remote_info)
        
        return 'decideInSchema'

    def decideInSchema(self, old_state):
        """(MASTER) decideInSchema state"""
        if old_state != 'decideInSchema':
            self.printVerbose(2,'\nSTATE: decideInSchema...')
            
        if self.send_in_schema and self.in_schema is not None:
            self.printVerbose(2,'  INPUT Schema to send')
            return 'waitInSchemaReplyState'
        else:
            self.printVerbose(2,'--> No INPUT schema to send.')
            return 'decideOutSchema'

    def waitInSchemaReplyState(self, old_state):
        """(MASTER) waitInSchemaReplyState state"""
        if old_state != 'waitInSchemaReplyState':
            self.printVerbose(2,'\nSTATE: waitInSchemaReplyState...')
        
        err, _reply = self.sendAndWait('input', 'schema', 
                                       in_data_type=None,
                                       to_send = self.in_schema, 
                                       retry_msg='INPUT schema')
        if err is not None:
            return 'endState'
        
        self.printVerbose(2,"--> INPUT schema Accepted\n")

        return 'decideOutSchema'

    def decideOutSchema(self, old_state):
        """(MASTER) decideOutSchema state"""
        if old_state != 'decideOutSchema':
            self.printVerbose(2,'\nSTATE: decideOutSchema...')
        
        if self.send_out_schema and self.out_schema is not None:
            self.printVerbose(2,'  OUTPUT Schema to send')
            return 'waitOutSchemaReplyState'
        else:
            self.printVerbose(2,'--> No OUTPUT schema to send.')
            return 'waitStartReplyState'

    def waitOutSchemaReplyState(self, old_state):
        """(MASTER) waitOutSchemaReplyState state"""
        if old_state != 'waitOutSchemaReplyState':
            self.printVerbose(2,'\nSTATE: waitOutSchemaReplyState...')
        
        err, _reply = self.sendAndWait('output', 'schema', 
                                       in_data_type=None,
                                       to_send = self.out_schema, 
                                       retry_msg='OUTPUT schema')
        if err is not None:
            return 'endState'
        
        self.printVerbose(2,"--> OUTPUT schema Accepted\n")

        return 'waitStartReplyState'

    def waitStartReplyState(self, old_state):
        """(MASTER) waitStartReplyState state"""
        if old_state != 'waitStartReplyState':
            self.printVerbose(2,'\nSTATE: waitStartReplyState...')
        

        #--- Do an intial run so we have data to send ---
        self._run_end_time = timeit.default_timer() + self.run_timestep
        self.initialProcess()
        
        err, data_in = self.sendAndWait('start', 
                                        to_send = self.local_data)
        if err is not None:
            return 'endState'
        
        self.remote_data = data_in
        self.printVerbose(1,"--> Link STARTed. Remote Data=%s\n"%data_in)

        return 'runState'

    def runState(self, old_state):
        """MASTER Run internal process and sync with remote"""
        if old_state != 'runState':
            self.printVerbose(2,'\nSTATE: Run')
            self._no_sync_timeout = timeit.default_timer() + SYNC_TIMEOUT
        
        # Finish up our initial (pre-start) run
        self.runWaitForTimeStep()
        self.state = 'runDoMaster'
    
        while True: #Note iteration count handled in runDoMaster
            if self.state == 'runWaitSyncReplyState':
                new_state = self.runWaitSyncReplyState(old_state)
            elif self.state == 'runDoMaster':
                new_state = self.runDoMaster(old_state)
            else:
                return new_state
            
            old_state = self.state
            if new_state is not None:
                self.state = new_state
                
        #Should never get here
        raise JsonLinkError('INVALID STATE: Final runDoMaster should switch to runWaitTermReply')

    def runWaitSyncReplyState(self, old_state):
        """(MASTER) wait for remote to reply with sync data"""
        msg = None
        sync_ok = False

        # Note: this while loop attempts to work around some common errors as
        # long as there is time left. It is NOT the main timing loop, which can
        # be found below in the call to runWaitForTimeStep()
        while timeit.default_timer() < self._run_end_time:
            try:
                msg = self.rx(skip_to_last=True)
            except JsonLinkTimeoutError:
                #TODO: add a few other specific error
                continue

            # Once message received
            try:
                actual_action, new_remote_data = self.unwrapJson('sync', msg, 'data')
            except JsonLinkRemoteError as err:
                self.printVerbose(1,'\nRemote ERROR: %s\n'%(err))
                return 'endState'
            except JsonLinkNoDataError:
                self.printVerbose(3,' No Data')
                continue

            if actual_action != 'sync':
                self.printVerbose(2,' WARNING: Ignoring %s packet'%(actual_action.upper()))
                continue
            
            sync_ok = True
            break
            
        if sync_ok:
            self.remote_data = new_remote_data
            self.sync_count += 1
            self._no_sync_timeout = timeit.default_timer() + SYNC_TIMEOUT

        else:
            if self.opt_run_line_end == '\r':
                self.printVerbose(1,'')
            self.printVerbose(1, "MISSING SYNC @ %s (runs=%d, syncs=%d)"%(dt.datetime.now().isoformat(' '), self.run_count, self.sync_count))
            self.num_miss += 1
            if timeit.default_timer() > self._no_sync_timeout:
                self.printVerbose(1, "NO SYNCs in %d sec. TERMINATING"%(SYNC_TIMEOUT))
                return 'waitTermReplyState'
                
                
        
        #Wait for end of timestep & set new end_time
        self.runWaitForTimeStep()

        return 'runDoMaster'

    def runDoMaster(self, old_state):
        """MASTER Display data, call internalProcess, and handle transitions"""

        self.run_count += 1
        
        self.printVerbose(2,'Remote Data: %s'%self.remote_data)
        if self.num_miss > 0:
            miss_str = ". Missed Sync=%d"%self.num_miss
        else:
            miss_str = ''
        self.printVerbose(1,'Run (%d/%.0f). Sync count=%d%s'%(self.run_count,
                                                            self.run_max, 
                                                            self.sync_count,
                                                            miss_str),
                          end=self.opt_run_line_end)
        
        #--- Run our internal process ----
        self.internalProcess()
        
        self.printVerbose(2,'Local (master) Data: %s'%self.local_data)

        #Monitor number of runs and transition appropriately
        if self.run_count < self.run_max:
            self.sendSync(self.local_data)
            return 'runWaitSyncReplyState'
        else:
            self.printVerbose(1,'\n\nTERMINATING: Max Iteration Count (%d)'%(self.run_count))
            return 'waitTermReplyState'

    def waitTermReplyState(self, old_state):
        """MASTER waitTermReplyState state"""
        if old_state != 'waitTermReplyState':
            self.printVerbose(2,'\nSTATE: waitTermReplyState...')
        
        err, data_in = self.sendAndWait('term', 
                                        to_send = self.local_data)
        if err is not None:
            return 'endState'

        self.remote_data = data_in
        self.printVerbose(1,"--> Link TERMinated. Final Remote Data=%s\n"%data_in)
        
        return 'endState'
    
    def initialProcess(self):
        """MASTER Initialize Master Data, This version simply calls internalProcess"""
        self.internalProcess()
    
    def internalProcess(self):
        """MASTER Internal Process. This version simply fills the schema with dummy data"""
        #Do internal process, including simulated processing delay
        time.sleep(self.run_delay)
        self.local_data = self.dummyData(self.out_schema)            

#===============================================================================
# SlaveLink
#===============================================================================
class SlaveLink(_BaseLink):
    """ Implements a GridLAB-D JSON Link Slave/server state machine"""
    
    _is_slave_server = True

    response_str_out = 'result'
    response_str_in = 'method'
    
    _run_send = True
    
    in_schema = None
    out_schema = None
    
    #Passed via the init link message, must include 'remote'
    description_dict = {'remote':"Python_JSON-Link_Slave/Server_Tester", 'version':__version__}

    def __init__(self, 
                 in_schema=None, 
                 out_schema=None,
                 **other_args):
        """SLAVE Constructor"""
        _BaseLink.__init__(self, **other_args)

        self.in_schema = in_schema 
        self.out_schema = out_schema 

    def _setupOurXchg(self, **Xchg_args):
        """SLAVE Setup the raw data exchange"""
        self._xchg = raw_xchg.SlaveXchg(**Xchg_args)

    def go(self):
        """SLAVE: Begin JSON link state machine execution"""
        old_state = self.state
        while True:
            if self.state == 'beginState':
                new_state = self.beginState(old_state)
            elif self.state == 'preInitState':
                new_state = self.preInitState(old_state)
            elif self.state == 'waitStartState':
                new_state = self.waitStartState(old_state)
            elif self.state == 'runState':
                new_state = self.runState(old_state)
            elif self.state == 'replyRunErrorState':
                new_state = self.replyRunErrorState(old_state)
            elif self.state == 'endState':
                self.printLinkStats()
                self.printVerbose(2,'END\n')
                break
            else:
                raise JsonLinkError('Invalid state %s'%self.state)

            old_state = self.state
            if new_state is not None:
                self.state = new_state

        self._xchg.close(close_server=True)
        return {'run_count': self.run_count}

    def wrapJson(self, action, data, data_name='data'):
        """encapsulate data (typically a nested data dictionary) in side a JSON link wrapper
         (Slave): Use most recent Rx msg_id"""
        
        #Match most recent received message id (and hence don't increment)
        self.msg_num = self.remote_msg_num
        return _BaseLink.wrapJson(self, action, data, data_name)

    def unwrapJson(self, action, msg, data_name='data', ignore_msg_num=False):
        """check JSON link wrapper and decode data (typically a nested data dictionary)
        (Slave): check for missing rx from msg_id"""
        response_tuple = _BaseLink.unwrapJson(self, action, msg, data_name, ignore_msg_num)
        
        #Check for missing packets
        if (self.msg_num+1 < self.remote_msg_num) and (self.msg_num > 0):
            just_missed = self.remote_msg_num - (self.msg_num+1)
            self.num_miss += just_missed
            self.printVerbose(1, "MISSING %d Packet(s) @ %s (rx_id=%d, expected=%d)"%(just_missed, dt.datetime.now().isoformat(' '), self.remote_msg_num, self.msg_num+1))
        
        return response_tuple
        
    def tx(self, msg, increment=False):
        """Transmit preformatted data message --- (Slave) force no incrementing"""
        _BaseLink.tx(self, msg, False)

    
    def rxIgnoreErr(self, timeout = None):
        """Robust message receive that ignores low-level timeouts and other errors"""
        msg = None
        
        if timeout is None:
            timeout = self.run_timestep
        
        end_time = timeit.default_timer() + timeout
                    
        while timeit.default_timer() < end_time: 
            try:
                msg = self.rx()
            except JsonLinkTimeoutError: 
                pass
            except JsonLinkRemoteError as err:
                self.printVerbose(3,' Error in Rx: %s'%(err.args[0]))
                pass
            
            if msg is not None:
                break
        
        return msg


    def beginState(self, old_state):
        """SLAVE beginState state"""
        self.printVerbose(2,'BEGIN')
        
        self.msg_num = 0
        
        return 'preInitState'

    def preInitState(self, old_state):
        """SLAVE preInitState state"""
        if old_state != 'preInitState':
            self.printVerbose(2,'\nSTATE: Pre-Init...')
            self.printVerbose(1,"Waiting for master to send INIT...")

        msg = self.rxIgnoreErr(INIT_RETRY_TIMEOUT)
            
        try:
            actual_action, remote_info = self.unwrapJson('init', msg, 'params')
        except JsonLinkNoDataError as err:
            self.printVerbose(1,'ERROR: No INIT received, giving up')
            return 'endState'
        except JsonLinkError as err:
            self.sendError('init', err.code, err.args[0])
            return 'endState'

        if actual_action != 'init':
            self.sendError('init', JsonLinkActionMismatchError.code, 'Sequence Error, expecting init')
            return 'endState'

        self.sendInit(self.description_dict)

        self.printVerbose(1,'--> Linked with REMOTE: %s\n'%remote_info)
        
        return 'waitStartState'

    def waitStartState(self, old_state):
        """SLAVE waitStartState state. Also handles schema requests"""
        if old_state != 'waitStartState':
            self.printVerbose(2,'\nSTATE: Wait for Start (waitStartState)...')        
            self.printVerbose(1,'Waiting for START')

        msg = self.rxIgnoreErr(INIT_RETRY_TIMEOUT)
        
        try:
            actual_action, data = self.unwrapJson('start', msg, 'data')
        except (JsonLinkError, raw_xchg.RawXchgError) as err:
            self.sendError('start', code=err.code, err_str=err.args[0])
            return 'endState'

        if actual_action != 'start':
            # handle alternative message types
            if actual_action == 'init':
                self.handleAltAction('init', msg, data_name='params', 
                                     out_data=self.description_dict)
                return None
            elif actual_action == 'input':
                self.in_schema = self.handleAltAction('input', msg, data_name='schema')
                self.printVerbose(2,'--> Storing INPUT SCHEMA: %s\n'%self.in_schema)
                return None
            elif actual_action == 'output':
                self.out_schema = self.handleAltAction('output', msg, data_name='schema')
                self.printVerbose(2,'--> Storing OUTPUT SCHEMA: %s\n'%self.out_schema)
                return None
            elif actual_action == 'term':
                self.handleAltAction('term', msg)
                return 'endState'
            else:
                self.sendError('start', JsonLinkActionMismatchError.code, 
                               err_str='Sequence Error, not expecting %s'%actual_action)
                return 'endState'

        #Setup our local run time interval
        self._run_end_time = timeit.default_timer() + self.run_timestep

        #Store a copy of the remote data
        self.remote_data = data

        #Respond to start message with initial conditions
        self.sendStart(self.dummyData(self.out_schema))

        self.printVerbose(2,'--> Link STARTed: %s\n'%data)
        
        return 'runState'
    
    def runState(self, old_state):
        """SLAVE Run internal process and sync with remote"""
        if old_state != 'runState':
            self.printVerbose(2,'\nSTATE: Run')
            self._no_sync_timeout = timeit.default_timer() + SYNC_TIMEOUT
        self.state = 'runDoSlave'
        self._run_send = False
    
        while  True: #Note iteration count handled in runWaitSyncState
            if self.state == 'runWaitSyncState':
                new_state = self.runWaitSyncState(old_state)
            elif self.state == 'runDoSlave':
                new_state = self.runDoSlave(old_state)
            else:
                return new_state
            
            old_state = self.state
            if new_state is not None:
                self.state = new_state
                
        return 'endState'

    def runWaitSyncState(self, old_state):
        """SLAVE sync data with remote"""
        msg = None
        self._run_send = False

        self.printVerbose(2,"") #Add a new line

        #Wait for end of timestep
        self.runWaitForTimeStep()
            
        try:
            msg = self.rx(skip_to_last=True)
        except JsonLinkTimeoutError as err:
            #TODO add a few other specific errors
            #TODO: Add some delay?
            pass

        if msg is not None:
            # Once message received
            try:
                actual_action, data = self.unwrapJson('sync', msg, 'data')
            except JsonLinkError as err:
                    self.sendError('sync', err.code, err.args[0])
                    return 'endState'

            if actual_action != 'sync':
                # handle alternative message types
                if actual_action == 'term':
                    self.printVerbose(1,'\n\nRemote TERMINATED Connection\n')
                    self.handleAltAction('term', msg, out_data=self.dummyData(self.out_schema))
                    return 'endState'
                else:
                    self.sendError(actual_action, JsonLinkActionMismatchError.code, 
                                   err_str='Sequence Error, not expecting %s'%actual_action)
                    return 'endState'
                
            if self.run_count >= self.run_max:
                self.printVerbose(1,'\n\nSending ERROR: Max Iteration Count (%d)'%(self.run_count))
                self.sendError(actual_action, JsonLinkRunMaxError.code, 
                               err_str='Number of runs reached max')
                return 'endState'

            #Store our data
            self.remote_data = data
            self.sync_count += 1
            self._run_send = True
            self._no_sync_timeout = timeit.default_timer() + SYNC_TIMEOUT
            
        if self.run_count >= self.run_max:
            err_msg = 'Reached Max Iteration Count (%d)'%(self.run_count)
            self.printVerbose(1,'\nSTOPPING: %s'%(err_msg))
            self._run_error = JsonLinkRunMaxError(err_msg)
            return 'replyRunErrorState'
        else:
            return 'runDoSlave'
    
    def runDoSlave(self, old_state):
        """SLAVE Internal Process. This version simply fills the schema with dummy data"""

        if timeit.default_timer() > self._no_sync_timeout:
            err_msg = "No Syncs in %d sec. "%(SYNC_TIMEOUT)
            self.printVerbose(1,'\nSTOPPING: %s'%(err_msg))
            self._run_error = JsonLinkTimeoutError(err_msg)
            return 'replyRunErrorState'

        self.run_count += 1
        
        self.printVerbose(2,'Remote Data: %s'%self.remote_data)
        if self.num_miss > 0:
            miss_str = ". Missed Msg=%d"%self.num_miss
        else:
            miss_str = ''
        self.printVerbose(1,'Run (%d/%.0f). Sync count=%d%s'%(self.run_count,
                                                            self.run_max, 
                                                            self.sync_count,
                                                            miss_str),
                          end=self.opt_run_line_end)
        
        time.sleep(self.run_delay)
        self.local_data = self.dummyData(self.in_schema)
        
        self.printVerbose(2,'Local (slave) Data: %s'%self.local_data)

        if self._run_send:
            self.sendSync(self.local_data)
        
        return 'runWaitSyncState'
    
    def replyRunErrorState(self, old_state):
        """
        SLAVE replyRunErrorState: Attempts to wait for a master message to
        which to reply with an error 
        """
        if old_state != 'replyRunErrorState':
            self.printVerbose(2,'\nSTATE: replyRunErrorState...')
            self.printVerbose(1,"Waiting for master to send something...")

        msg = self.rxIgnoreErr(INIT_RETRY_TIMEOUT)
            
        try:
            actual_action, _ = self.unwrapJson('term', msg, 'data')
        except JsonLinkNoDataError:
            self.printVerbose(1,'ERROR: Nothing received, giving up')
            return 'endState'
        except JsonLinkError:
            pass

        self.sendError(actual_action, self._run_error.code, self._run_error.args[0])
        return 'endState'


#===============================================================================
# Related Functions
#===============================================================================
__all__.extend(['loadJsonStrOrFile'])

def loadJsonStrOrFile(in_str):
    """
    Examines a string to see if it is a valid JSON string, if not it assumes
    the string corresponds to a filename and attempts to load it and process 
    its contents as JSON
    
    First attempts to unquote the string for improved command line processing
    support
    
    Note that the terminal may do some pre-processing of the command line, 
    including stripping embedded double quotes. As a result this function
    attempts to put double quotes around names and non-numeric values
    """
    
    if in_str is None:
        return None
    
    #Remove enclosing quotes
    if in_str[0] in ['"',"'"] and in_str[-1] in ['"',"'"]:
        in_str = in_str[1:-1]
        
    if in_str[0] == '{' and in_str[-1] == '}':
        #If there are any imbedded double quotes, assume that the user has
        #taken the time to escape all quote characters so simply treat as is
        if '"' not in in_str:
            #Remove any single quotes
            in_str = in_str.replace("'","")
            
            #Attempt to double quote our field names
            in_str = re.sub('([a-zA-Z]\w+):', '"\\1":',in_str)
            #And add double quotes around non-numeric values
            in_str = re.sub('(:\s*)([a-zA-Z][^,}\n]+)([,}\n])', '\\1"\\2"\\3',in_str)

        return json.loads(in_str)
    else:
        # Otherwise, process as a JSON file
        json_file = open(in_str)
        return json.load(json_file)