"""Exercise production udp::recv bounds using real POSIX loopback datagrams."""
import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest

REPO = pathlib.Path(__file__).resolve().parents[2]


@unittest.skipIf(os.name == 'nt', 'POSIX loopback socket harness')
class UDPInboundRuntimeTests(unittest.TestCase):
    def run_case(self, cases):
        compiler = shutil.which('c++')
        if not compiler:
            self.skipTest('C++ compiler unavailable')
        source = (REPO / 'connection/udp.cpp').read_text()
        production = source[source.index('size_t udp::recv('):source.index('int udp::call_setsockopt(')]
        prefix = r'''
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <sstream>
#include <string>
#define SOCKET_ERROR (-1)
using int64=long long;
enum ErrorPolicy { TE_RETRY,TE_ABORT,TE_IGNORE };
class udp {
public:
  int sd=-1, maxretry=0;
  ErrorPolicy on_error=TE_ABORT;
  timeval timeout={0,100000};
  char input[2048]={};
  void *sockdata=nullptr, *translation=nullptr;
  void *(*translator)(char*,void*)=nullptr;
  unsigned int header_size=32;
  void debug(int,const char *format,...) {
    char buffer[4096]; va_list args; va_start(args,format);
    std::vsnprintf(buffer,sizeof(buffer),format,args); va_end(args);
  }
  [[noreturn]] void exception(const char*,...) { throw std::runtime_error("rejected datagram"); }
  unsigned int get_header_version() const { return 0; }
  unsigned int get_header_size() const { return header_size; }
  const char *get_message_format() const { return "NONE"; }
  double get_message_version() const { return 0.0; }
  int call_setsockopt(int s,int level,int option,timeval *value,int n) {
    return setsockopt(s,level,option,value,n);
  }
  size_t recv(char*,size_t);
};
'''
        helpers = r'''
struct Peer {
  udp u;
  int sender;
  sockaddr_in address={};
  Peer() {
    u.sd=socket(AF_INET,SOCK_DGRAM,0); sender=socket(AF_INET,SOCK_DGRAM,0);
    assert(u.sd>=0 && sender>=0);
    address.sin_family=AF_INET; address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(u.sd,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0);
    socklen_t n=sizeof(address);
    assert(getsockname(u.sd,reinterpret_cast<sockaddr*>(&address),&n)==0);
    u.sockdata=&address;
  }
  ~Peer() { assert(close(u.sd)==0); assert(close(sender)==0); }
  void raw(const std::string &bytes) {
    assert(sendto(sender,bytes.data(),bytes.size(),0,reinterpret_cast<sockaddr*>(&address),
                  sizeof(address))==static_cast<ssize_t>(bytes.size()));
  }
  void packet(int offset,int declared,std::string payload,int status=200,
              int version=0,const char *type="NONE",double format_version=0.0) {
    char header[256];
    int n=std::snprintf(header,sizeof(header),"%-1d %-3d %-7d %-5.5s %-3.1f %-1d %-3d   ",
                        version,offset,declared,type,format_version,1,status);
    assert(n==32);
    const std::string bytes=std::string(header,n)+payload;
    assert(sendto(sender,bytes.data(),bytes.size(),0,reinterpret_cast<sockaddr*>(&address),
                  sizeof(address))==static_cast<ssize_t>(bytes.size()));
  }
};
void reject(Peer &peer,size_t capacity) {
  std::array<char,16> destination; destination.fill('Z');
  bool rejected=false;
  try { peer.u.recv(destination.data()+1,capacity); }
  catch (const std::runtime_error&) { rejected=true; }
  assert(rejected);
  for (char c:destination) assert(c=='Z');
}
'''
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder)
            cpp = path / 'case.cpp'
            cpp.write_text(prefix + production + helpers + cases)
            result = subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra',
                                     str(cpp), '-o', str(path / 'case')],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(path / 'case')], capture_output=True,
                                    text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_binary_payload_exact_terminator_space_and_internal_buffer(self):
        self.run_case(r'''
int main() {
  Peer p;
  const std::string binary("A\0B",3);
  std::array<char,8> destination; destination.fill('Z');
  p.packet(32,3,binary);
  assert(p.u.recv(destination.data()+1,4)==3);
  assert(destination[0]=='Z' && destination[5]=='Z');
  assert(std::memcmp(destination.data()+1,binary.data(),3)==0 && destination[4]=='\0');
  p.packet(32,0,"");
  assert(p.u.recv(destination.data()+1,1)==0 && destination[1]=='\0');
  p.packet(32,3,binary);
  assert(p.u.recv(nullptr,0)==3 && std::memcmp(p.u.input,binary.data(),3)==0 && p.u.input[3]=='\0');
}
''')

    def test_malformed_datagram_and_destination_bounds_do_not_write(self):
        self.run_case(r'''
int main() {
  Peer p;
  p.packet(32,3,"abc"); reject(p,3);  // No room for the terminator.
  p.packet(32,0,""); reject(p,0);
  p.packet(32,4,"abc"); reject(p,8); // Truncated declared payload.
  p.packet(32,-1,""); reject(p,8);
  p.u.header_size=100; p.packet(100,1,"x"); reject(p,8);
  p.u.header_size=static_cast<unsigned int>(-1); p.packet(-1,1,"x"); reject(p,8);
  p.u.header_size=32;
  p.packet(31,1,"x"); reject(p,8); // Unexpected configured offset.
  p.packet(32,1,"x",500); reject(p,8);
  p.packet(32,1,"x",200,1); reject(p,8);
  p.packet(32,1,"x",200,0,"BAD"); reject(p,8);
  p.packet(32,1,"x",200,0,"NONE",1.0); reject(p,8);
}
''')

    def test_header_integer_overflow_is_rejected(self):
        self.run_case(r'''
int main() {
  Peer p;
  p.u.header_size=128;
  std::string valid="0 128 1 NONE 0.0 1 200";
  valid.resize(128,' '); p.raw(valid+"x");
  std::array<char,8> destination;
  assert(p.u.recv(destination.data(),destination.size())==1 && destination[0]=='x');
  // Each of these wrapped to a valid value with sscanf("%d") on Linux.
  const std::string overflow_headers[]={
    "4294967296 128 1 NONE 0.0 1 200",
    "0 4294967424 1 NONE 0.0 1 200",
    "0 128 4294967297 NONE 0.0 1 200",
    "0 128 1 NONE 4294967296.0 1 200",
    "0 128 1 NONE 0.4294967296 1 200",
    "0 128 1 NONE 0.0 4294967297 200",
    "0 128 1 NONE 0.0 1 4294967496",
  };
  for (std::string header:overflow_headers) {
    header.resize(128,' '); p.raw(header+"x"); reject(p,8);
  }
}
''')

    def test_header_fields_are_confined_to_the_advertised_header(self):
        self.run_case(r'''
int main() {
  Peer p;
  // A matching configured offset must still contain the complete header.
  p.u.header_size=8;
  p.raw("0 8 1 NONE 0.0 1 200   x"); reject(p,8);
  // Valid-looking numeric prefixes must not hide trailing header garbage.
  p.u.header_size=32;
  std::string header="0 32 1 NONE 0.0 1 200junk";
  header.resize(32,' '); p.raw(header+"x"); reject(p,8);
  // A binary token must match the complete configured type, including length.
  const char binary_type[]="0 32 1 NONE\0X 0.0 1 200";
  header.assign(binary_type,sizeof(binary_type)-1);
  header.resize(32,' '); p.raw(header+"x"); reject(p,8);
}
''')

    def test_truncated_oversized_datagram_is_rejected_before_copying(self):
        self.run_case(r'''
int main() {
  Peer p;
  // The declared body fits, but the actual datagram exceeds the receive buffer.
  for (size_t bytes: {size_t(2048),size_t(3000)}) {
    p.packet(32,1,std::string(bytes-32,'x')); reject(p,8);
  }
  // The largest complete datagram that still leaves room for a terminator.
  const std::string payload(2047-32,'x');
  p.packet(32,payload.size(),payload);
  std::array<char,2048> destination;
  assert(p.u.recv(destination.data(),destination.size())==payload.size());
  assert(std::memcmp(destination.data(),payload.data(),payload.size())==0);
  assert(destination[payload.size()]=='\0');
}
''')
