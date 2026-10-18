"""Compile production udp::send and inspect real loopback datagrams (POSIX)."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest
import os

REPO = pathlib.Path(__file__).resolve().parents[2]


class UDPOutboundFramingTests(unittest.TestCase):
    @unittest.skipIf(os.name == 'nt', 'POSIX loopback socket harness')
    def test_payload_bytes_and_advertised_offset_agree(self):
        compiler = shutil.which('c++')
        if not compiler:
            self.skipTest('C++ compiler unavailable')
        source = (REPO / 'connection/udp.cpp').read_text()
        production = source[source.index('size_t udp::send('):source.index('size_t udp::recv(')]
        prefix = r'''
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#define SOCKET_ERROR (-1)
class udp {
public:
  unsigned int header_version=0, header_size=32;
  char message_format[6]="NONE";
  double message_version=0.0;
  timeval timeout={1,0};
  char output[2048]={};
  int position=0, sd=-1;
  void *sockdata=nullptr;
  void error(const char*,...) {}
  void debug(int,const char*,...) {}
  void exception(const char*,...) { throw std::runtime_error("send failed"); }
  size_t send(const char*,size_t);
};
namespace Socket { const char *strerror() { return "socket failure"; } }
'''
        cases = r'''
int main() {
  int receiver=socket(AF_INET,SOCK_DGRAM,0);
  assert(receiver>=0);
  sockaddr_in address={}; address.sin_family=AF_INET;
  address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  assert(bind(receiver,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0);
  socklen_t address_size=sizeof(address);
  assert(getsockname(receiver,reinterpret_cast<sockaddr*>(&address),&address_size)==0);
  timeval receive_timeout={1,0};
  assert(setsockopt(receiver,SOL_SOCKET,SO_RCVTIMEO,&receive_timeout,sizeof(receive_timeout))==0);
  udp u; u.sd=socket(AF_INET,SOCK_DGRAM,0); assert(u.sd>=0); u.sockdata=&address;
  const char binary[]={'A','\0','B','C'};  // Deliberately no trailing NUL.
  char packet[2048]={}; // sscanf below must receive a terminated header string.
  assert(u.send(binary,sizeof(binary))==36);
  assert(recv(receiver,packet,sizeof(packet),0)==36);
  unsigned int version,offset; size_t payload;
  assert(sscanf(packet,"%u %u %zu",&version,&offset,&payload)==3);
  assert(offset==32 && payload==4 && memcmp(packet+offset,binary,4)==0);
  assert(u.send(binary,2)==34);
  assert(recv(receiver,packet,sizeof(packet),0)==34 && memcmp(packet+32,binary,2)==0);
  std::string limit(1500-32,'x');
  assert(u.send(limit.data(),limit.size())==1500);
  assert(recv(receiver,packet,sizeof(packet),0)==1500);
  assert(u.send(limit.data(),limit.size()+1)==0);
  u.header_version=10; assert(u.send(binary,4)==0); u.header_version=0;
  u.header_size=31; assert(u.send(binary,4)==0); u.header_size=32;
  u.message_version=10.0; assert(u.send(binary,4)==0); u.message_version=0.0;
  assert(u.send(binary,std::numeric_limits<size_t>::max())==0);
  errno=0;
  assert(recv(receiver,packet,sizeof(packet),MSG_DONTWAIT)==-1);
  assert(errno==EAGAIN || errno==EWOULDBLOCK);
  close(u.sd); close(receiver);
}
'''
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder)
            cpp = path / 'case.cpp'
            cpp.write_text(prefix + production + cases)
            result = subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra',
                                     str(cpp), '-o', str(path / 'case')],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(path / 'case')], capture_output=True,
                                    text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
