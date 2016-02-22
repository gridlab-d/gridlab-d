"""
A python module for GridLAB-D data (JSON) data exchange over UDP and TCP

Classes:
  MasterXchg
  SlaveXchg

Implemented Methods:
  

Notes:

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
__version__, __date__ = "1.0.4b","2013-09-11"  #13:48   BryanP   Show elapsed time between Rx/Tx when show_time enabled
# [Older, in reverse order]
# version      date       time     who     Comments
# -------   ----------    ----- ---------- --------------------------------------- 
#  1.0.3b   2013-09-03    11:53   BryanP   Corrected justification & spacing for fixed width header
#  1.0.2b   2013-08-15    14:35   BryanP   Corrected binary output for Py3.0 by using 'latin1' encoding
#  1.0.1b   2013-08-08    13:15   BryanP   Add binary support (no utf-8 encode/decode)
#  1.0.0b   2013-06-04    00:45   BryanP   Add Rx/Tx timestamp option
#  0.6.1b   2013-05-31    14:45   BryanP   User select for first vs last message in receive
#  0.6.0b   2013-05-29    23:40   BryanP   BUGFIX: Always use latest socket message 
#  0.5.2b   2013-05-09    22:40   BryanP   BUGFIX: allow immediate address/port reuse
#  0.5.1b   2013-05-09    02:00   BryanP   Update Ports, Additional cleanup
#  0.5.0b   2013-05-08    14:00   BryanP   BUGFIX: TCP handling, Early prep for unittests
#  0.4.2b   2013-05-06    10:53   BryanP   Rename opt_verbose to opt_raw
#  0.4.1b   2013-04-29    22:20   BryanP   BUGFIX: receive timeout when connect not ready. Move receive to _BaseXchg
#  0.4.0b   2013-04-28    22:20   BryanP   RENAMED to raw_xchg & methods to Xchg* and Errors to RawXchg*Error
#  0.3.1a   2013-04-27    02:00   BryanP   Throw exception when can't send. Added codes to Exceptions, Add "Raw" prefix to verbose msg
#  0.3.0a   2013-04-25    16:23   BryanP   Use Exceptions for error handling. Renamed Link* to Xchg*
#  0.2.0a   2013-04-25    03:30   BryanP   Works! Added Master TCP timeouts
#  0.1.0a   2013-04-24    21:43   BryanP   Extracted from json_test.py v1.6.1a

#Standard Library imports
import socket
import select
import timeit
import sys
import time
import datetime as dt

#===============================================================================
# Module level constants
#===============================================================================
UDP = socket.SOCK_DGRAM
TCP = socket.SOCK_STREAM

HEAD_VER = '0'
MSG_TYPE = 'JSON'
MSG_VER = '1.0'

DEFAULT_HOST = 'localhost'
DEFAULT_UDP_PORT = 39037
DEFAULT_TCP_PORT = 39036

TIMEOUT_DIVISOR = 1000       #Number of timeout periods to make full timestep delay

#===============================================================================
# Error (Exception) Class definitions
#===============================================================================
class RawXchgError(IOError):
    """Base class for exceptions in the raw_xchg module"""
    code = 1000
    
class RawXchgTimeoutError(RawXchgError):
    """Timeout waiting for reply"""
    code = RawXchgError.code + 1

class RawXchgSetupError(RawXchgError):
    """Problems setting up the communication (e.g. opening the socket)"""
    code = RawXchgError.code + 2

class RawXchgHeaderError(RawXchgError):
    """Malformed or incorrect header"""
    code = RawXchgError.code + 3

class RawXchgNoDataError(RawXchgError):
    """No data read"""
    code = RawXchgError.code + 4

class RawXchgCantSendError(RawXchgError):
    """Unable to send packet"""
    code = RawXchgError.code + 5


#===============================================================================
# _BaseXchg (module internal)
#===============================================================================
class _BaseXchg(object):
    """ Base class for UDP/TCP link subclasses"""
    
    #socket and settings
    _sock = None
    _is_slave_server = None
    _reopen_conn = False

    PROTOCOL = None
    HOST_ADDR = None
    PORT = None
    TIMEOUT = None
    BUF_SIZE = 4906
    
    _incomplete_msg_in_fragment = ''
    _blank_msg = ''
    
    response_code_out = None     #override in subclass
    
    #Options
    opt_raw = False
    opt_header = True
    opt_show_time = True
    opt_binary = False  #Don't encode/ecode output to utf-8
    
    ignore_head_err = False
    ignore_no_data_err = True
    
    #Track link statistics
    num_err = 0
    num_tx = 0
    num_rx = 0
    
    _last_rx_tx_time = timeit.default_timer()

    def __init__(self, host_addr, port, protocol,
                 opt_raw=False, opt_show_time=True, 
                 opt_header=True, timeout=0,
                 opt_binary=False, buf_size=4906):
        """Constructor"""
        self.HOST_ADDR = host_addr
        self.PORT = port
        self.PROTOCOL = protocol
        
        self.opt_raw = opt_raw
        self.opt_show_time = opt_show_time
        
        self.opt_header = opt_header
        self.TIMEOUT = max(timeout,0)
        self.BUF_SIZE = buf_size
        if opt_binary:
            self.opt_binary = True
            self._blank_msg = b''
            self._incomplete_msg_in_fragment = self._blank_msg
        
        
    def isReady(self):
        """Returns True if we have assigned the socket. Does not check socket connectivity"""
        return self._sock is not None

    #-------------------
    # wrapPacket
    #-------------------
    # From: https://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=JSON_link_protocol
    #
    # Bytes     Contents
    # --------- --------------------------------------------------------------------------------
    # 00-01     Header version (currently "0", space padded)
    # 02-05     Offset to message from byte 00 (currently always 32, max is 999, space padded)
    # 06-13     Size of message (varies, max is 9999999, left justified, space padded), count does not include string termination character
    # 14-19     Type of message (currently "JSON", left justified, space padded)
    # 20-23     Version of type (currently "1.0", left justified, space padded)
    # 24-25     Connection status (0=closed, 1-9=keep-alive timeout, space padded)
    # 26-29     Response code (0 for requests, must be 200 for responses, left justified, space padded)
    # 30-31     Reserved (space padded)
    # 32-       Message content (current JSON 1.0 format)
    #
    #  Byte Number          111111111122222222223333333333444444444455555555556666666666
    #             0123456789 123456789 123456789 123456789 123456789 123456789 123456789
    # Master Ex: '0 32  27       JSON 1.0 1 0     {"method":"example","id":1}'
    # Slave Ex:  '0 32  27       JSON 1.0 1 200   {"result":"example","id":1}'
    #  Message Length                                      111111111122222222223333333333444444444455555555556666666666
    #                                             123456789 123456789 123456789 123456789 123456789 123456789 123456789
    def wrapPacket(self, msg, keep_alive=9):
        """ Add GridLAB UDP/TCP header and return complete data packet to send"""
        msg_size = len(msg)

        #Build potentially variable portion of header
        #            0123456- 234-892123 456-  9312
        head_str =        "%-7d %-5s %-3s %1d %-3s   "%(msg_size, MSG_TYPE, MSG_VER, keep_alive, self.response_code_out)
        #Measure current size and add 5bytes for the header ver & msg offset
        head_len = len(head_str) + 6
        assert head_len==32, "outgoing packet header length should be 32 not %d"%head_len
        
        #Return the complete Packet String
        return "%1s %-3d %s%s"%(HEAD_VER, head_len, head_str, msg) 
    
    #-------------------
    # unwrapPacket
    #-------------------
    def unwrapPacket(self, packet_str):
        """ Remove GridLAB UDP/TCP header, check validity and return Packet JSON content"""
        body = None
        self._reopen_conn = False
        
        if len(packet_str) == 0:
            raise RawXchgNoDataError("No Packet Data")
            
        
        if packet_str[0] != HEAD_VER:
            raise RawXchgHeaderError("Header: unsupported version must be %s not %s"%(HEAD_VER, packet_str[0]))
        
        try:
            #Extract header size (data offset) and split header and body
            head_len = int(packet_str[2:5])
            head = packet_str[0:head_len]
            body = packet_str[head_len:]
            
            #Attempt to extract whitespace delimited header
            _head_ver, _offset, msg_size, msg_type, msg_ver, conn_status, response = head.split()
        except:
            raise RawXchgHeaderError('Header: Invalid Format-must have 7 whitespace delimited fields')
        
        #Verify/Parse header contents
        # Note: already checked head_ver & offset is redundant b/c we used the same
        # same data to split head and body. (At least assuming reasonable space 
        # delimiting
        if int(msg_size) != len(body):
            raise RawXchgHeaderError('Header: message (or header) length mismatch (%s vs %d actual)'%(msg_size, len(body)))
        if msg_type != MSG_TYPE:
            raise RawXchgHeaderError('Header: Unsupported message type (%s). Must be %s'%(msg_type, MSG_TYPE))
        if msg_ver != MSG_VER:
            raise RawXchgHeaderError('Header: Unsupported %s message version (%s). Must be %s'%(MSG_TYPE, msg_ver, MSG_VER))
        if response != self.response_code_in:
            raise RawXchgHeaderError('Header: Invalid response code (%s), expected %s'%(response, self.response_code_in))
    
        #Extract connection re-open flag
        if int(conn_status) <= 0:
            self._reopen_conn = True
    
        return body
    
    def close(self):
        """gracefully close the current socket"""
        if self._sock is not None:
            try:
                self._sock.shutdown(socket.SHUT_RDWR)
            except socket.error:
                pass
            try:
                self._sock.close()
            except socket.error:
                pass
            self._sock = None
 
    def receive(self, skip_to_last=False):
        """
        SHARED Receive data and process JSON-link header. 
        
        Assumes socket is already open (for UDP) or connected (for TCP)
        
        If the optional parameter skip_to_last is True, receive will grab only 
        the most recent message and flush any old messages. This is useful for
        real-time synchronized data exchange.
        """
        if not self.isReady():
            self.setupXchg()
        
        msg_in = self._incomplete_msg_in_fragment
        old_msg = self._blank_msg
        expect_len = None
        HEADER_BYTE_TO_READ_FOR_SIZE = 13
        
        # Setup socket to read from, depending on protocol
        if self._is_slave_server and self.PROTOCOL == TCP:
            rx_sock = self._conn
        else:
            rx_sock = self._sock

        
        #-- Read in JSON messages up to the most recent with timeout
        # Note there may be older messages in the buffer, too.
        end_time = timeit.default_timer() + self.TIMEOUT
        has_run_once = False
        
        while timeit.default_timer() < end_time or not has_run_once:
            has_run_once = True
                
            if self.opt_header:
                #Read in message size information from header (in first 13 bytes)
                # Note: may not get all at once (especially with TCP b/c streams)
                if len(msg_in) < HEADER_BYTE_TO_READ_FOR_SIZE:
                    #TCP is *stream* oriented, so we rx byte-by-byte
                    if self.PROTOCOL == TCP:
                        msg_in += self._rx_chunk(rx_sock, HEADER_BYTE_TO_READ_FOR_SIZE - len(msg_in))
                    #UDP is *packet* oriented, so each rx has to grab a full packet
                    else:
                        msg_in += self._rx_chunk(rx_sock, self.BUF_SIZE)
                        
                    # Once enough information for the header is received, determine
                    # the total length to read for this packet 
                    if len(msg_in) >= HEADER_BYTE_TO_READ_FOR_SIZE:
                        try:
                            _, h_len, m_len = msg_in[0:HEADER_BYTE_TO_READ_FOR_SIZE].split()
                        except ValueError:
                            raise RawXchgHeaderError('Malformed header-incorrect length data')
                        
                        expect_len = int(h_len) + int(m_len)
                    else:
                        continue
                    
                #Now read the rest of the message
                if len(msg_in) < expect_len:
                    msg_in += self._rx_chunk(rx_sock, expect_len - len(msg_in))

                if skip_to_last:
                    #Once we have a full message, double check that there is not a newer
                    #message in the buffer.
                    if len(msg_in) == expect_len:
                        # See if there is any more data ready now (timeout=0)
                        ready_to_read, _, _ = select.select([rx_sock], [], [], 0)
                        
                        #If so, drop this old message and start again
                        #TODO: keep old message if reading the new one results in a timeout
                        if rx_sock in ready_to_read:
                            if self.opt_raw:
                                print("  Raw: old message found in rx buffer... looking for new one")
        
                            old_msg = msg_in
                            msg_in = self._blank_msg
                            expect_len = None
                        #If there is nothing to read, we are done
                        else:
                            break
                    #If not skipping to the last message, we are done
                    else:
                        break
            else:
                msg_in = self._rx_chunk(rx_sock, self.BUF_SIZE)
                if msg_in:
                    expect_len = len(msg_in)
                    break
                    
        #-- Handle the (hopefully) received message
        if msg_in and len(msg_in) == expect_len:
            self._incomplete_msg_in_fragment = self._blank_msg
        else:
            self._incomplete_msg_in_fragment = msg_in
            if old_msg:
                msg_in = old_msg
            else:
                raise RawXchgTimeoutError('Receive timeout')

        # Show time between Rx/Tx
        if self.opt_show_time:
            self.showElapsed()

        # Display "raw" verbose message if desired
        if self.opt_raw:
            print('  ', end='')
            if self.opt_show_time:
                print(dt.datetime.now().strftime('%M:%S.%f- '), end='')
            print("Raw RX: %s"%(msg_in))
        
        #Process packet header
        if self.opt_header:
            try:
                msg_in = self.unwrapPacket(msg_in)
            except RawXchgHeaderError:
                err = sys.exc_info()[1]   #Use instead of "as err" For compatibility with python < 2.6
                if self.ignore_head_err:
                    print('WARNING: %s'%err)
                    msg_in = None
                else:
                    raise err
            except RawXchgNoDataError: 
                err = sys.exc_info()[1]   #Use instead of "as err" For compatibility with python < 2.6
                if self.ignore_no_data_err:
                    print('WARNING: %s'%err)
                    msg_in = None
                else:
                    raise err
            else:
                if self._reopen_conn and not self._is_slave_server:
                    self.close()

        return msg_in

    def _rx_chunk(self, rx_sock, max_bytes):
        """
        SHARED helper function to attempt to read a chunk up to a specified number
        of bytes from the given socket. If there is an error, sleep a little to not
        hog the CPU
        """
        try:
            chunk, self._remote_addr = rx_sock.recvfrom(max_bytes)
        except socket.error:
            time.sleep(self.TIMEOUT/1000)
            return self._blank_msg
                
        if self.opt_binary:
            return chunk
        else:
            # Convert raw bytes to string
            return chunk.decode('utf-8')
     
    def showElapsed(self):
        """Compute and display elapsed time since last Rx/Tx"""
        diff = timeit.default_timer() - self._last_rx_tx_time
        print(diff, ' sec since last Rx/Tx')
        self._last_rx_tx_time += diff


#===============================================================================
# MasterXchg
#===============================================================================
#TODO: Separate classes for UDP (base) and TCP (child)

class MasterXchg(_BaseXchg):
    """ Implements a low-level GridLAB-D (JSON) UDP/TCP Master/client link"""
    
    _is_slave_server = False

    response_code_out = '0'
    response_code_in = '200'

    def setupXchg(self):
        if not self.isReady():
            self._sock = socket.socket(family=socket.AF_INET, type=self.PROTOCOL)
    
            try:
                self._sock.connect((self.HOST_ADDR, self.PORT))
            except socket.error:
                raise RawXchgSetupError("Can't connect to slave ")

            self._sock.settimeout(self.TIMEOUT)


    def send(self, msg_out, add_header=True):
        """MASTER: Wrap message with a header and send out"""
        self.setupXchg()
        
        if self.opt_header and add_header:
            msg_out = self.wrapPacket(msg_out)
            
        if not self.opt_binary:
            msg_out = msg_out.encode('utf-8')
        else:
            #Note: latin1 provides a 1-to-1 mapping of raw bytes
            # Thanks: http://stackoverflow.com/questions/2611205/how-do-i-write-raw-binary-data-in-python
            msg_out = msg_out.encode('latin1')
            
        try:
            self._sock.sendall(msg_out)
        except IOError as err:
            if self.opt_raw:
                print("Unable to Send Packet (%s)"%err.args[1])
            self.close()
            raise RawXchgCantSendError("Unable to Send Packet")
            
        
        # Show time between Rx/Tx
        if self.opt_show_time:
            self.showElapsed()

        if self.opt_raw:
            print('  ', end='')
            if self.opt_show_time:
                print(dt.datetime.now().strftime('%M:%S.%f- '), end='')
            print("Raw TX: %s"%(msg_out))
        
#===============================================================================
# SlaveXchg
#===============================================================================
class SlaveXchg(_BaseXchg):
    """ Implements a low_level GridLAB-D (JSON) UDP/TCP Slave/server link"""
    
    _is_slave_server = True

    response_code_out = '200'
    response_code_in = '0'
        
    _remote_addr = None
    connected = False
    _conn = None
    
    def isReady(self):
        """Returns True if we have assigned the socket. Does not check socket connectivity"""
        if self._sock is None:
            return False
        elif self.PROTOCOL == TCP and self._conn is None:
            return False
        else:
            return True
            

    def close(self, close_server=False):
        """SLAVE: Expanded close to handle our state flags"""
        
        if self.PROTOCOL == UDP or close_server:
            # Close self._sock
            _BaseXchg.close(self)

        if self.PROTOCOL == TCP:
            self.connected = False
            if self._conn is not None:
                try:
                    self._conn.shutdown(socket.SHUT_RDWR)
                except socket.error:
                    pass
                try:
                    self._conn.close()
                except IOError:
                    pass
                self._conn = None
    
    def setupXchg(self):
        """Setup (Internet) socket for Slave."""
        if self._sock is None:
            self._sock = socket.socket(socket.AF_INET, self.PROTOCOL)
            if self._sock is None:
                raise RawXchgSetupError('could not create socket')

            #Enable address to be re-used right away
            self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            
            try:
                self._sock.bind((self.HOST_ADDR, self.PORT))
            except socket.error as err:
                self.close()
                raise RawXchgSetupError('Error binding to socket: ' + err.args[1])
            if self.PROTOCOL == TCP:
                self._sock.listen(1)
                
            self._sock.settimeout(self.TIMEOUT)

        if self.PROTOCOL == TCP and self._conn is None:
            try:
                self._conn, self._remote_addr = self._sock.accept()  #Updates our socket for connection
            except socket.timeout:
                raise RawXchgTimeoutError('Timeout waiting for TCP connection')

            self.connected = True
            self._conn.settimeout(self.TIMEOUT)
            if self.opt_raw:
                print('Connected by', self._remote_addr)
            

    def send(self, msg_out, add_header=True):
        """SLAVE: Wrap message with a header and send out"""
        if not self.isReady():
            self.setupXchg()

        if self.opt_header and add_header:
            msg_out = self.wrapPacket(msg_out)

        if not self.opt_binary:
            msg_out = msg_out.encode('utf-8')
        else:
            #Note: latin1 provides a 1-to-1 mapping of raw bytes
            msg_out = msg_out.encode('latin1')

        if self.PROTOCOL == TCP:
            self._conn.send(msg_out)
        else:
            self._sock.sendto(msg_out, self._remote_addr)

        # Show time between Rx/Tx
        if self.opt_show_time:
            self.showElapsed()

        if self.opt_raw:
            print('  ', end='')
            if self.opt_show_time:
                print(dt.datetime.now().strftime('%M:%S.%f- '), end='')
            print("Raw TX: %s"%(msg_out))

        if self._reopen_conn:
            self.close()    