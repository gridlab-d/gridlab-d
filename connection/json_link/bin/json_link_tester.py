#!/usr/local/bin/python3

"""
****************
json_link_tester
****************

Test program for JSON link protocol over TCP or UDP exchange.  It 
can act as either the Master (aka client) or the Slave (aka server) and can
optionally inject arbitrary faults to test responses.

*Note:* On many installs there top-level script `json_link_demo.py` that can be
used in place of `python -m json_link.bin.json_link_tester` in the examples
below

Basic UDP slave
===============

For a basic UDP slave with max run count of 4 use (from the parent directory for
json_link/)::

    $ python -m json_link.bin.json_link_tester -c4

This runs a JSON link slave using the localhost. After initial handshaking, the
simulated run cycle executes every 0.5sec by filling the input (master oriented
names) schema with dummy data. If sync messages are received from the master,
the latest local data is sent in reply. If run with the basic UDP master,
below this produces something like::

    JSON Link packet test (v0.9.2a) SLAVE (aka SERVER) mode (waits for data)
        Run timestep = 0.500sec
        (minimum) Reply Delay = 0.100sec
        (raw exchange) Timeout = 0.001sec
        Using UDP packets on localhost:50007
    
    Waiting for master to send INIT...
    --> Linked with REMOTE: {'modelname': 'json_link_demo.py', 'application': 'Generic_JSON_Link_Master', 'version': '0.9.5a'}
    
    Waiting for START
    Run (3/4). Sync count=1
    
    Remote TERMINATED Connection

Basic UDP master
=====================

For a basic UDP master (with max run count) run (from the parent directory for
json_link/)::

    $ python -m json_link.bin.json_link_tester -m -c2
    
This runs a JSON link master using the localhost. After initial handshaking, the
simulated run cycle executes every 1sec by filling the output schema with dummy
data and attempting to send a sync message to the slave and use the response. If
there is a timeout waiting for the slave, the master run loop continues using
the old slave data. If run with the basic UDP slave above, this produces
something like::
    
    JSON Link packet test (v0.9.2a) MASTER (aka CLIENT) mode (initiates transfer)
        Run timestep = 1.000sec
        (minimum) Reply Delay = 0.100sec
        (raw exchange) Timeout = 0.001sec
        Using UDP packets on localhost:50007
    
    Waiting for slave's INIT response...
    --> Linked with REMOTE: {u'version': u'0.9.5a', u'remote': u'Generic JSON Link Slave/Server'}
    
    --> Link STARTed. Remote Data=None
    
    Run (2/2). Sync count=1
    
    TERMINATING: Max Iteration Count (2)
    --> Link TERMinated. Final Remote Data=None

Additional Options
==================

A wide range of additional configuration options are available. The complete
list is available at the command line with::

    $ python -m json_link.bin.json_link_tester -h
    
Some highlights:

Transfer Setup
--------------

-t, --tcp            
    Use TCP sockets
    
-a HOST_ADDR, --host_addr=HOST_ADDR
    Host name/IP address (Default: localhost) for SLAVE
    (aka SERVER) on non-windows use All for all interfaces

-p PORT, --port=PORT
    Port Number

Debugging Information Display
-----------------------------

--raw                 
    Print Raw packet contents including header
    (Default=False, print cleaner output)

-v VERBOSE, --verbose=VERBOSE
    Control level of verbosity for JSON link level output.
    See --raw for raw_xchg. verbosity requires an integer
    parameter. 0=no output 1=basic init and run only &
    errors, 2=add state machine & warnings, 3+=add link-
    level messages

Run timing
----------

-r RUN_TIMESTEP, --run_timestep=RUN_TIMESTEP
    Run timestep in Sec. Duration of each sub-run in the
    internal process loop. MASTER: sets to the transmit
    interval. (default=1.0) SLAVE: time to wait before
    running using the last received  value (default=0.5)
-d DELAY, --delay=DELAY
    Minimum delay after rx before tx. Used to simulate a
    minimum internal synchronous process running time
    (default=0.1sec)

Fault Injection
==================

The `-f` or `--fault` option allows for the injection of arbitrary faults into
the communication stream. 


Notes
=====

* Should work with Python 2.6+ and 3.x. Tested with 2.7.2 and 3.3
* Currently only supports IPv4
* The SLAVE will handle connections from any number of masters, BUT since the
  code is only single threaded each processing (including delay) are
  blocking. Hence, the SLAVE may miss packets if it is busy processing
  (or waiting to send) a different reply.

@author: Bryan Palmintier, NREL 2013
"""
# Enable use of Python 3.0 print statement in older (2.x) versions, ignored in
# newer (3.x) versions
# Must occur at top of file
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
#                         -------  ----------    ----- ---------- ------------- 
__version__, __date__ =  "1.3.0", "2014-04-05"  #10:00   BryanP   Add statistics for rx/tx/missing
# [Older, in reverse order]
# version      date       time     who     Comments
# -------   ----------    ----- ---------- --------------------------------------- 
#  1.2.0    2014-04-05    02:17   BryanP   Slave: Tx msg_id match most recent Rx msg_id
#  1.1.7    2014-04-05    00:57   BryanP   Add support for nominal voltage and random range
#  1.1.6    2013-10-08    18:25   BryanP   Faults: Enable dropped packets & bug fixe specified count, Drop beta from ver string
#  1.1.5b   2013-09-11    13:50   BryanP   Display time between Rx/Tx when show_time enabled
#  1.1.4b   2013-09-06    23:45   BryanP   Support for enums/char 1024 in schema
#  1.1.3b   2013-09-01    20:55   BryanP   Include Python in init messages, correct justification
#  1.1.2b   2013-08-15    14:35   BryanP   Corrected binary master for Python 3
#  1.1.1b   2013-08-08    13:15   BryanP   Add binary support (no utf-8 encode/decode)
#  1.1.0b   2013-07-02    13:45   BryanP   Add --volt_div option for scaling current outputs
#  1.0.0b   2013-06-04    00:45   BryanP   Show_time for raw Xchg, Corrected Master/Slave link timing
#  0.9.7a   2013-06-03    12:20   BryanP   Show_time support for verbose
#  0.9.6a   2013-05-31    14:50   BryanP   Overhaul missing sync handling.
#  0.9.5a   2013-05-11    22:30   BryanP   Updating ver for minor json_link & raw_xchg changes
#  0.9.4a   2013-05-09    12:56   BryanP   Add robust option, option grouping in help
#  0.9.3a   2013-05-09    01:06   BryanP   Updated ports, refined import paths, use json_lik.CONSTANTS for defaults
#  0.9.2a   2013-05-07    13:45   BryanP   Updated timeout computations
#  0.9.1a   2013-05-06    10:45   BryanP   NEW: --verbose option (distinct from --raw), Help cleanup
#  0.9.0a   2013-05-06    00:45   BryanP   NEW: basic fault injection, schema from cmd line/file
#  0.8.2a   2013-04-29    20:10   BryanP   Expanded/Refined timing options: run_timestep vs delay vs timeout
#  0.8.1a   2013-04-28    22:20   BryanP   Rename gld_tcp_udp to raw_xchg & methods to Xchg* and Errors to RawXchg*Error
#  0.8.0a   2013-04-27    02:00   BryanP   RENAMED json_link_test. Use json_link for FULL HANDSHAKING. Added --run_count
#  0.7.1a   2013-04-25    16:23   BryanP   Updated for gld_upd_tcp v 0.3.0a (Exceptions Link* -> Xchg*)
#  0.7.0a   2013-04-25    03:33   BryanP   Extracted low-level request/response header to raw_xchg.py
#  0.6.1a   2013-04-24    00:43   BryanP   BUG FIX: UDP socket close, abstract out setupSlaveSocket()
#   0.6a    2013-04-23    22:43   BryanP   Added GridLAB request/response header support
#  0.5.5a   2013-04-09    15:23   BryanP   Corrected python3.x and windows support, added num_var option
#  0.5.4a   2013-04-09    11:23   BryanP   Switched to master/slave terminology. Default now slave
#  0.5.3a   2013-04-09    00:23   BryanP   Client auto-repeat
#  0.5.2a   2013-04-08    12:32   BryanP   Added server delay & client timeout
#  0.5.1a   2013-04-08    22:00   BryanP   Implemented (basic, fixed) JSON exchange
#   0.4a    2013-04-08    17:30   BryanP   Merged client and server (and shared) into single file
#   0.3a    2013-04-08            BryanP   Command Line options using JSON_test_shared
#   0.2a    2013-04-08    12:32   BryanP   Added Metadata Header
#   0.1a    2012-04-05            BryanP   Initial Code, partially adapted from http://docs.python.org/3.3/library/socket.html#example

# Standard Library imports
import optparse  # Use older optparse for pre-2.7/3.2 compatibility. argparse preferred for newer projects
import json
import datetime as dt
import random
import time
import timeit
import sys
import os
import traceback

# Local imports
# Here we would like to use the following relative path statements:
#    from .. import json_link
#    from ..xchg import raw_xchg
# But this does not work since we are located within the package. Instead we
# need to add the package's parent directory to the path. Here we insert at the
# beginning so that we are sure to use our enclosing json_link package
_pkg_root = os.path.abspath(os.path.normpath(os.path.dirname(__file__) + '/../..'))
sys.path.insert(0, _pkg_root)

from json_link import json_link
from json_link.xchg import raw_xchg
from json_link.faults import fault_inject

# module variables

#-------------------
# runSimpleMaster
#-------------------
def runSimpleMaster(opt):
    
    # Initialize underlying socket.
    # Note: if opt.raw is used, use raw_xchg to print packet data
    link = raw_xchg.MasterXchg(opt.host_addr, opt.port, opt.sock_type,
                                  opt_raw=opt.raw,
                                  opt_binary=opt.binary,
                                  opt_header=(not opt.no_head),
                                  timeout=opt.timeout)

    link.setupXchg()
    end_time = timeit.default_timer()
        
    while True:
        end_time += opt.delay
                    
        if link.isReady():
            # -- Build message and send it
            data = buildMessage(opt)
            msg_out = json.dumps(data)
            link.send(msg_out)
            if not opt.raw:
                printIO('TX: ', data)
                
            
            # And wait for a response
            try:
                reply = link.receive()
            except raw_xchg.RawXchgTimeoutError:
                reply = None

            if reply is not None:
                if not opt.binary:
                    reply = json.loads(reply)
                if not opt.raw:
                    printIO('RX: ', reply)
        
        # Add blank line between exchanges
        print()
            
        # Setup link here for more consistant tx interval, since not dependent on setup delays
        link.setupXchg()
        
        # Wait for repeat timer, but yield each loop to prevent excess CPU
        while timeit.default_timer() < end_time:
            time.sleep(opt.delay / 1000)


#-------------------
# runSimpleSlave
#-------------------
def runSimpleSlave(opt):
    # Initialize underlying socket.
    # Note: if opt.raw is used, use raw_xchg to print packet data
    link = raw_xchg.SlaveXchg(opt.host_addr, opt.port, opt.sock_type,
                                 opt_raw=opt.raw,
                                 opt_binary=opt.binary,
                                 opt_header=(not opt.no_head),
                                 timeout=opt.timeout)

    while True:
        link.setupXchg()

        # --Wait for connection
        try:
            data = link.receive()
        except raw_xchg.RawXchgTimeoutError:
            continue
        
        if data is None:
            continue        
        
        # Process incoming
        if opt.echo:
            msg_out = data
        else:
            data = json.loads(data)
        if not opt.raw:
            printIO('RX: ', data)

        # Wait delay period
        if opt.delay > 0:
            time.sleep(opt.delay)
 
        # Build Reply
        if opt.echo:
            reply = data
        else:
            reply = buildReply(opt, data)
            msg_out = json.dumps(reply)
            
        # and send it out
        link.send(msg_out)
        if not opt.raw:
            printIO('TX: ', reply)
        print()

#-------------------
# buildMessage
#-------------------
def buildMessage(opt):
    data = dict()
    data['clock'] = dt.datetime.now().isoformat(' ')
    for n in range(0, opt.num_var):
        data['x%d' % n] = random.random()
    return data



#-------------------
# buildReply
#-------------------
def buildReply(opt, data):

    if type(data) is dict:
        # Build reply
        if 'clock' in data:
            data['clock'] = dt.datetime.now().isoformat(' ')

        for n in range(0, opt.num_var):
            # Use random numbers for first half of y data
            if n < opt.num_var / 2:
                data['y%d' % n] = random.random()
            else:
            # For second half, double corresponding x, or return 0
                if 'x%d' % n in data:
                    data['y%d' % n] = 2 * data['x%d' % n]
                else:
                    data['y%d' % n] = 0

        # For all cases remove all x followed by only digits
        data_keys = list(data.keys())  # Note cache full key list b/c otherwise python complains when the dict changes size
        for k in data_keys:
            if k.startswith('x') and k.lstrip('x').isdigit():
                data.pop(k)
                
    elif opt.binary:
        pass    #For binary mode, simply echo the raw values

    else:  # Non-dictionary value for data
        print('ERROR: Expected a dictionary object in JSON. Returning Empty JSON reply')
        data = dict()    
    return data
    
#-------------------
# printIO
#-------------------
def printIO(prefix, data, suffix=''):
    print(prefix, end='')
    if type(data) is dict:
        if 'clock' in data:
            print("clock='%s' " % (data['clock']), end='')
        for k in sorted(data.keys()):
            if k != 'clock':
                print("%s=%.3f " % (k, data[k]), end='')
    else:
        print(data, end='')                
    print(suffix)
    
#-------------------
# _handleCommandLine
#-------------------
def _handleCommandLine():
    """ 
    Setup, parse, configure, and display results of command line options
    
    Notes:
     
    * this function uses the now depreciated optparse module for backward compatibility
    * for more info see optparse docs and this more approachable primer:
    http://www.alexonlinux.com/pythons-optparse-for-human-beings

    """
    
    # Define some string constants
    usage = "usage: %prog [options]"
    ver_str = "JSON Link over Raw UDP/TCP Exchange tester -- v%s (%s)" % (__version__, __date__)
    
    # Initialize command line parser
    p = optparse.OptionParser(usage, version=ver_str)
    
    #===========================================================================
    # Option Group: General options
    #===========================================================================
    # Also includes --help and --version
    p.add_option('--doc', action='store_true', default=False,
                 help="Display extended help documentation")

    #===========================================================================
    # Option Group: JSON-link Configuration
    #===========================================================================
    jlink_group = optparse.OptionGroup(p, 'JSON-link Configuration')
    p.add_option_group(jlink_group)

    jlink_group.add_option('-s', '--slave', action="store_true", dest="slave",
                 help="""Run as SLAVE (aka server) by listening for data & 
replying (Default)
""")
    jlink_group.add_option('-m', '--master', action="store_false", dest="slave",
                 help="""Run as MASTER (aka client) by initiating data transfer 
& waiting for reply
""")

    jlink_group.add_option('-i', '--input', 
                 help="""Input Schema to use/verify. Input is to master.
Specified as either a filename or in-line dictionary string (enclosed in {}).
NOTE: for command line strings, use double quotes around strings with spaces.
Internal double quotes may be striped, so use no quotes or single quotes for
field/data. They will be automatically double quoted.
""")
    jlink_group.add_option('-o', '--output', 
                 help="""Output Schema to use/verify. Output is from master
Specified as either a filename or in-line dictionary string (enclosed in {}).
NOTE: for command line strings, use double quotes around strings with spaces.
Internal double quotes may be striped, so use no quotes or single quotes for
field/data. They will be automatically double quoted.
""")
    jlink_group.add_option('--schema', default='BOTH',
                 help="""Specify which schema(s) to send (MASTER-only). 
Options=BOTH, IN, OUT, None. (Default = %default) 
""")
    jlink_group.add_option('--volt_div', type=complex, default=1.0,
                 help="""Specify the voltage divisor (aka resistance) value to 
use to scale current outputs based on voltage inputs. Complex values are OK. [%default]
""")
    jlink_group.add_option('--volt_nom', type=complex, default=240,
                 help="""Specify the nominal voltage to use when generating 
random voltage values. Complex values are OK. [%default]
""")
    jlink_group.add_option('--volt_range', type='float', default=0.05,
                 help="""Per unit range (+/-) above or below norminal voltage 
for random voltages. [%default]
""")
    
    #===========================================================================
    # Option Group: Network Configuration
    #===========================================================================
    net_group = optparse.OptionGroup(p, 'Network Options')
    p.add_option_group(net_group)

    net_group.add_option("-t", "--tcp", action="store_const", const=raw_xchg.TCP, 
                 dest="sock_type", 
                 help="Use TCP sockets")
    net_group.add_option("-u", "--udp", action="store_const", const=raw_xchg.UDP, 
                 dest="sock_type",
                 help="Use UDP sockets (default)")
    net_group.add_option("-a", "--host_addr", default='localhost',
                 help="""Host name/IP address [%default] on 
non-windows SLAVE (aka SERVER) use All for all interfaces
""")
    net_group.add_option("-p", "--port", default=None, type="int",
                 help="Port Number")
    net_group.add_option("--buf_size", default=4906, type="int",
                 help="Network buffer size in bytes [%default]")

    #===========================================================================
    # Option Group: Run Control
    #===========================================================================
    run_group = optparse.OptionGroup(p, 'Run and Timing Options')
    p.add_option_group(run_group)

    run_group.add_option('-c', '--run_count', type="int", 
                 default= json_link.DEFAULT_RUN_MAX,
                 help=""""Number of run iterations before quitting. Use -1 for 
unlimited [%default]
""")
    run_group.add_option("-r", "--run_timestep", type="float",
                 help="""Run timestep in Sec. Duration of each sub-run in the 
internal process loop. MASTER: sets to the transmit interval. (default=1.0)
SLAVE: time to wait before running using the last received 
value (default=0.5)
""")
    run_group.add_option("-d", "--delay", type="float", default=None,
                 help="""Minimum delay after rx before tx. Used to simulate a 
minimum internal synchronous process running time 
(default=timestep/10)
""")
    run_group.add_option("-T", "--timeout", type="float",
                 help="""Timeout for low-level data exchange in seconds. 
(default=step/%f)
"""%(raw_xchg.TIMEOUT_DIVISOR))


    #===========================================================================
    # Option Group: Output Display
    #===========================================================================
    output_group = optparse.OptionGroup(p, 'Output Display Options')
    p.add_option_group(output_group)

    output_group.add_option('-v','--verbose', type='int', default=1,
                 help="""Control level of verbosity for JSON link level output. 
See --raw for raw_xchg. verbosity requires an integer 
parameter. 0=no output 1=basic init and run only & errors, 2=add state
machine & warnings, 3+=add link-level messages
""")
    output_group.add_option('-q','--quiet', 
                 action="store_const", dest='verbose', const=0,
                 help="""Quiet mode. Suppress all all output. Same as -v0""")
    output_group.add_option('--show_time', action="store_true", default=False,
                 help="""Show time stamp with each verbose (but not
raw) output display. (Default: Not set/hidden)
""")
    output_group.add_option('--raw', action="store_true", default=False,
                 help="""Print Raw packet contents including header 
(Default=False, print cleaner output)
""")

    #===========================================================================
    # Option Group: Extended Run Modes
    #===========================================================================
    extension_group = optparse.OptionGroup(p, 'Extended Run Modes, mutually exclusive except for --robust')
    p.add_option_group(extension_group)
    
    extension_group.add_option('--robust', action='store_true', default=False, 
                 help="""
Restart program after each run and attempt to recover from errors. Use a
keyboard interrupt (Control-C) to stop
""")

    extension_group.add_option('-f', '--fault', 
                 help="""Inject faults as defined in the specified file (see 
/json_link/faults for examples) as a string representation of a dictionary
object. Fields: action-- the associated JSON-link action (e.g. init, start,
etc.); count--[optional] inject the fault on the specified number of messages
for the action. e.g {"action":"sync", "count":3} to inject the fault only on the
3rd sync. Count can be an integer, list, or tuple; and one of the following:
drop-- to not actually send a packet; data--the data packet (often a sub
dictionary) to send; data_dict--a full JSON-link dictionary including msg_id,
etc.; raw--the full packet string including raw exchange header information.
NOTE: can alternatively specify as a command line string, though quotes can be a
pain. Try using double quotes around strings with spaces. Internal double quotes
may be striped, so use no quotes or single quotes for field/data. They will be
automatically double quoted.  For example use: 
   `$ python json_link_demo.py -m -f json_link/faults/sync_drop_test.json -c15`
with
   `$ python json_link_demo.py`
To test missed msg/sync counting
""")

    #===========================================================================
    # Option Group: Non-JSON-link complient options
    #===========================================================================
    noncomply_group = optparse.OptionGroup(p, 'Legacy and non JSON-link compliment options')
    p.add_option_group(noncomply_group)

    noncomply_group.add_option('-H', '--no_head', action="store_true", default=False,
                 help="""Suppress request/response header and send only JSON 
contents [Default=False, use full header]
""")
    noncomply_group.add_option('--simple', action="store_true", default=False,
                 help="""Exchange simple, bare JSON data packets without using 
GridLAB-D JSON link protocol handshaking (Default=False, use 
handshaking)
""")
    noncomply_group.add_option('-e', '--echo', action="store_true", default=False,
                 help="""!SIMPLE ONLY! SLAVE (aka SERVER) only: Echo (do not 
process) received packets
""")
    noncomply_group.add_option('-n', '--num_var', type="int", default=2,
                 help="""!SIMPLE ONLY! Number of numeric variable to include in 
JSON packet
""")
    noncomply_group.add_option('--binary', action="store_true", default=False,
                 help="""!SIMPLE ONLY! Binary mode: do not encode/decode data as UTF-8. Useful
for pure echo applications.
""")

    #===========================================================================
    # Handle complex defaults
    #===========================================================================

    # Setup multi-option defaults... otherwise uses add_option default=...
    p.set_defaults(sock_type=raw_xchg.UDP, slave=True)


    # Actually parse command line
    (opt, args) = p.parse_args()

    # Implement our own help function so we can include the Doc string
    if opt.doc:
        print()
        print(ver_str)
        p.print_help()  # Show default optparse generated help info
        print(__doc__)  # Tack on our doc string, the first string in this file
        exit()
    
    # Update values based on mode
    if opt.slave:
        if opt.run_timestep is None:
            opt.run_timestep = json_link.DEFAULT_SLAVE_TIMESTEP
        if opt.host_addr == 'All':
            opt.host_addr = ''
    else:  # Master
        if opt.run_timestep is None:
            opt.run_timestep = json_link.DEFAULT_MASTER_TIMESTEP

    if opt.timeout is None:
        opt.timeout = opt.run_timestep/raw_xchg.TIMEOUT_DIVISOR

    if opt.delay is None:
        opt.delay = opt.run_timestep/json_link.DEFAULT_DELAY_DIVISOR
    
    if opt.port is None:
        if opt.sock_type == raw_xchg.UDP:
            opt.port = raw_xchg.DEFAULT_UDP_PORT
        elif opt.sock_type == raw_xchg.TCP:
            opt.port = raw_xchg.DEFAULT_TCP_PORT
        else:
            raise json_link.JsonLinkError('Unknown socket type %s'%opt.sock_type)

    
    #===========================================================================
    # Display status information
    #===========================================================================
    if opt.verbose > 0:
        print("\nJSON Link packet test (v%s) "%(__version__), end='')
        if opt.slave:
            print("SLAVE (aka SERVER) mode (waits for data)")
        else:
            print("MASTER (aka CLIENT) mode (initiates transfer)")
    
        if not opt.simple:
            print("    Run timestep = %.3fsec" % opt.run_timestep)
            print("    (minimum) Reply Delay = %.3gsec" % opt.delay)
            print("    (raw exchange) Timeout = %.3gsec" % opt.timeout)

            
        if opt.host_addr == '':
            host_txt = "AnyInterface"
        else:
            host_txt = opt.host_addr
            
        if opt.sock_type == raw_xchg.TCP:
            print("    Using TCP packets on %s:%d" % (host_txt, opt.port))
        else:
            print("    Using UDP packets on %s:%d" % (host_txt, opt.port))
    
        if opt.simple and opt.slave and opt.echo:
            print("    ECHO mode on (no JSON processing for reply)")
    
        if opt.raw:
            print("    RAW mode on: displaying full packet with header")
            
        if opt.no_head:
            print("    NO_HEADer mode: Using raw JSON without reply/response header")
        
        if not opt.simple and opt.fault:
            print("    FAULT INJECT mode: will send bad messages/packets as defined")

        if opt.simple:
            print("    Running in SIMPLE MODE")
            
        if opt.binary:
            print("    Running in BINARY MODE: no UTF-8 encode/decode")
            
        if opt.robust:
            print("    ROBUST MODE will restart after session ends or on errors, use Ctrl-C to break")
            
    
        # Add blank line
        print()
               
    # Return a parse object, accessible using opt.OPTION
    return (opt, args)    
    
#-------------------
# Run main when called stand-alone
#-------------------
def main():
    # -- Get Command Line options
    (opt, _args) = _handleCommandLine()

    run_count = 1;
    if opt.robust:
        print(' ----- STARTING (Robust Mode) Session #%d -------\n'%run_count)

    while True:
        run_count += 1
        try:
            if opt.simple:
                if opt.slave:
                    runSimpleSlave(opt)
                else:
                    runSimpleMaster(opt)
            else:
                keyword_args = {'host_addr':opt.host_addr,
                                'port':opt.port,
                                'protocol':opt.sock_type,
                                'opt_raw':opt.raw,
                                'opt_verbose':opt.verbose,
                                'opt_show_time':opt.show_time,
                                'buf_size':opt.buf_size,
                                'opt_header':(not opt.no_head),
                                'run_max':opt.run_count,
                                'run_timestep':opt.run_timestep,
                                'run_delay':opt.delay,
                                'timeout':opt.timeout,
                                'in_schema':json_link.loadJsonStrOrFile(opt.input),
                                'out_schema':json_link.loadJsonStrOrFile(opt.output),
                                'opt_volt_divisor':opt.volt_div,
                                'opt_volt_nom':opt.volt_nom,
                                'opt_volt_range':opt.volt_range
                                }
                if opt.fault:
                    if opt.slave:
                        state_machine = fault_inject.SlaveFaultInject(**keyword_args)
                    else:
                        state_machine = fault_inject.MasterFaultInject(schema_to_send=opt.schema,**keyword_args)
                    
                    #add fault dictionary
                    state_machine.fault = json_link.loadJsonStrOrFile(opt.fault)
                
                else:            
                    if opt.slave:
                        state_machine = json_link.SlaveLink(**keyword_args)
            
                    else:
                        state_machine = json_link.MasterLink(schema_to_send=opt.schema,**keyword_args)
        
                state_machine.go()
                
        except Exception:
            # Note: KeyboardInterrupt is not a subclass of Exception
            print()
            print(traceback.format_exc(), end='')
            if opt.robust:
                pass

        if not opt.robust:
            break

        print('\n\n ----- RESTARTING (Robust Mode) Session #%d -------\n'%run_count)
                

if __name__ == '__main__':
    main()
