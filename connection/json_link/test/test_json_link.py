"""
json_link Unittests
===================
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
__version__, __date__ = "0.2.1a","2013-05-09"  #03:06   BryanP   Cleanup suppressed output 
# [Older, in reverse order]
# version      date       time     who     Comments
# -------   ----------    ----- ---------- --------------------------------------- 
#  0.2.0a   2013-05-09    03:06   BryanP   Implement Remote Master/Slave. Add 4 basic/core test cases
#  0.1.0a   2013-05-08    13:06   BryanP   Initial Version

import unittest
import subprocess
import io            #allows us to redirect "files," including stderr & stdout to strings
import sys
import os

# Here we would like to use the following relative path statements:
#from .. import json_link
#from ..xchg import raw_xchg
# But such paths are not supported by unittest in python 2.x so we manipulate
# the search path instead
sys.path.append(os.path.abspath(os.path.dirname(__file__) + '../..'))
from json_link import json_link
from json_link.xchg import raw_xchg

#module constants
#TODO: parse from command line
_SPEEDUP = 100.0  #Factor by which to reduce json_link's default timesteps
_show_local = False
_show_remote = False

class TestJsonLink(unittest.TestCase):
    _remote = None
    _my_stdout = None
    _my_stderr = None
    
    json_link_tester_path = os.path.normpath(json_link._pkg_root+"/bin/json_link_tester.py")

    def setUp(self):
        """Initializes our string based stdout and stderr alternatives"""
        
        if _show_local:
            self._my_stdout = sys.stdout
            self._my_stderr = sys.stderr
        else:
            # Select "string" file type that matches the python version's string
            # representation. In all version, io.BytesIO works on 1-byte chunks
            # while io.StringIO works with unicode strings. In Python 3.x,
            # strings are also unicode, while in Python 2.x, they are single-
            # byte based
            if sys.version_info >= (3, 0):
                self._my_stdout = io.StringIO()
                self._my_stderr = io.StringIO()
            else:
                self._my_stdout = io.BytesIO()
                self._my_stderr = io.BytesIO()

    def startRemoteMaster(self,opt_list=[]):
        """Initialize a Master to serve as other side of our connection"""
        self._remote = subprocess.Popen(["python", self.json_link_tester_path, 
                                         "-m", "-q"] + opt_list)

    def startRemoteSlave(self,opt_list=[]):
        """Initialize a Slave to serve as other side of our connection"""
        self._remote = subprocess.Popen(["python", self.json_link_tester_path, 
                                         "-s", "-q"] + opt_list)

    def tearDown(self):
        """Stop the Master"""
        if self._remote is not None:
            self._remote.terminate()

    def _slaveTestCore(self, extra_remote, extra_local):
        """Abstracts out the common settings for slave runs"""
        remote_args = ['-c4', 
                       '-r%g'%(json_link.DEFAULT_MASTER_TIMESTEP/_SPEEDUP),
                       '-T%g'%(json_link.DEFAULT_MASTER_TIMESTEP/raw_xchg.TIMEOUT_DIVISOR/_SPEEDUP)
                       ]
        remote_args.extend(extra_remote)
        if _show_remote:
            remote_args.extend(['-v2','--raw'])
            
        
        my_args =  {'host_addr':'localhost',
                    'port':raw_xchg.DEFAULT_UDP_PORT,
                    'protocol':raw_xchg.UDP,
                    'run_max':10,
                    'my_stdout':self._my_stdout,
                    'my_stderr':self._my_stderr,
                    'run_timestep':(json_link.DEFAULT_SLAVE_TIMESTEP/_SPEEDUP),
                    'timeout':(json_link.DEFAULT_SLAVE_TIMESTEP/raw_xchg.TIMEOUT_DIVISOR/_SPEEDUP),
                    }

        if _show_remote:
            my_args['opt_raw']=True
        else:
            my_args['opt_raw']=False
            
        my_args.update(extra_local)

        self.startRemoteMaster(remote_args)
        
        local_slave = json_link.SlaveLink(**my_args)
        return local_slave.go()
    
    def _masterTestCore(self, extra_remote, extra_local):
        """Abstracts out the common settings for master runs"""
        remote_args = ['-c10', 
                       '-r%g'%(json_link.DEFAULT_SLAVE_TIMESTEP/_SPEEDUP),
                       '-T%g'%(json_link.DEFAULT_SLAVE_TIMESTEP/raw_xchg.TIMEOUT_DIVISOR/_SPEEDUP)
                       ]
        remote_args.extend(extra_remote)
        if _show_remote:
            remote_args.extend(['-v2','--raw'])
        
        my_args =  {'host_addr':'localhost',
                    'port':raw_xchg.DEFAULT_UDP_PORT,
                    'protocol':raw_xchg.UDP,
                    'run_max':4,
                    'my_stdout':self._my_stdout,
                    'my_stderr':self._my_stderr,
                    'run_timestep':(json_link.DEFAULT_MASTER_TIMESTEP/_SPEEDUP),
                    'timeout':(json_link.DEFAULT_MASTER_TIMESTEP/raw_xchg.TIMEOUT_DIVISOR/_SPEEDUP),
                    'opt_raw':True,
                    }

        if _show_remote:
            my_args['opt_raw']=True
        else:
            my_args['opt_raw']=False

        my_args.update(extra_local)

        self.startRemoteSlave(remote_args)
        
        local_master = json_link.MasterLink(**my_args)
        return local_master.go()

#===============================================================================
# The tests
#===============================================================================
    def testBasicUdpSlave(self):
        """We expect that the exchange will end after the master's 4 runs and
        since the DEFAULT_SLAVE_TIMEOUT is 1/2 that for the MASTER, this
        implies a maximum of 8 runs, but the master cuts things short as soon
        as it reaches the max iterations, resulting in 7 slave runs
        """
        
        remote_args = ['-u']
        my_args =  {}

        stats = self._slaveTestCore(remote_args, my_args)

        expected_runs = 8
        self.assertTrue(stats['run_count'] >= expected_runs, "Insufficient Run Count (%d < %d)"%(stats['run_count'], expected_runs))
        
    def testBasicTcpSlave(self):
        """We expect that the exchange will end after the master's 4 runs and
        since the DEFAULT_SLAVE_TIMEOUT is 1/2 that for the MASTER, this
        implies a maximum of 8 runs, but the master cuts things short as soon
        as it reaches the max iterations, resulting in 7 slave runs
        """
        
        remote_args = ['-t']
        my_args =  {'protocol':raw_xchg.TCP,
                    'port':raw_xchg.DEFAULT_TCP_PORT,
                    }

        stats = self._slaveTestCore(remote_args, my_args)

        expected_runs = 8
        self.assertTrue(stats['run_count'] >= expected_runs, "Insufficient Run Count (%d < %d)"%(stats['run_count'], expected_runs))

    def testBasicUdpMaster(self):
        """We expect that the exchange will end after the master's 4 runs
        """
        
        remote_args = ['-u']
        my_args =  {}

        stats = self._masterTestCore(remote_args, my_args)

        expected_runs = 4
        self.assertEqual(stats['run_count'], expected_runs, "Incorrect Run Count (%d vs %d)"%(stats['run_count'], expected_runs))
        
    def testBasicTcpMaster(self):
        """We expect that the exchange will end after the master's 4 runs
        """
        
        remote_args = ['-t']
        my_args =  {'protocol':raw_xchg.TCP,
                    'port':raw_xchg.DEFAULT_TCP_PORT,
                    }

        stats = self._masterTestCore(remote_args, my_args)

        expected_runs = 4
        self.assertEqual(stats['run_count'], expected_runs, "Incorrect Run Count (%d vs %d)"%(stats['run_count'], expected_runs))


if __name__ == "__main__":
    #import sys;sys.argv = ['', 'Test.testName']
    unittest.main()