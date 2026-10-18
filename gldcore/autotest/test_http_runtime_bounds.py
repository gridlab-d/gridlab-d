"""Production HTTP bounds/cleanup with controlled I/O and real POSIX descriptors."""
import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest

REPO = pathlib.Path(__file__).resolve().parents[2]


@unittest.skipIf(os.name == 'nt', 'POSIX socket/descriptor harness')
class HTTPRuntimeBoundsTests(unittest.TestCase):
    def run_case(self, body, option_branch=False):
        compiler = shutil.which('c++')
        if not compiler:
            self.skipTest('C++ compiler unavailable')
        source = (REPO / 'gldcore/http_client.cpp').read_text()
        production = source[source.index('HTTP *hopen('):source.index('size_t hfilelength(')]
        prefix = r'''
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <unordered_set>
#include "http_client.h"
#define REV_MAJOR 4
#define REV_MINOR 0
#ifdef TEST_SOCKET_OPTION
#undef MSG_NOSIGNAL
#ifndef SO_NOSIGPIPE
#define SO_NOSIGPIPE 0x4040
#endif
#endif
void output_error(const char*,...) {}
void output_debug(const char*,...) {}
enum Failure { NONE, HANDLE_ALLOC, RESPONSE_ALLOC, SOCKET_CREATE, DNS,
               CONNECT, SEND_ZERO, SEND_ERROR, RECEIVE, SOCKET_OPTION,
               SEND_INTERRUPTED, RECEIVE_INTERRUPTED };
Failure failure=NONE;
int allocation_calls=0, send_calls=0, receive_calls=0, live_fd=-1, peer_fd=-1;
bool option_set=false;
std::unordered_set<void*> allocations;
std::string sent, response;
size_t received=0;
void *tracked_malloc(size_t n) {
  ++allocation_calls;
  if ((failure==HANDLE_ALLOC && allocation_calls==1) ||
      (failure==RESPONSE_ALLOC && allocation_calls==2)) return nullptr;
  void *p=std::malloc(n); assert(p); assert(allocations.insert(p).second); return p;
}
void tracked_free(void *p) {
  assert(p && allocations.erase(p)==1); std::free(p);
}
int controlled_socket(int,int,int) {
  if (failure==SOCKET_CREATE) return INVALID_SOCKET;
  int pair[2]; assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0);
  live_fd=pair[0]; peer_fd=pair[1]; return live_fd;
}
hostent *controlled_host(const char*) {
  if (failure==DNS) return nullptr;
  static in_addr address={};
  static char *addresses[]={reinterpret_cast<char*>(&address),nullptr};
  static hostent host={}; host.h_length=sizeof(address); host.h_addr_list=addresses;
  return &host;
}
int controlled_connect(int fd,const sockaddr*,socklen_t) {
  assert(fd==live_fd); return failure==CONNECT ? -1 : 0;
}
int controlled_setsockopt(int fd,int level,int opt,const void *value,socklen_t n) {
  assert(fd==live_fd && level==SOL_SOCKET && n==sizeof(int));
#ifdef SO_NOSIGPIPE
  assert(opt==SO_NOSIGPIPE && *static_cast<const int*>(value)==1);
#else
  (void)opt; (void)value; assert(false);
#endif
  if (failure==SOCKET_OPTION) { errno=EPERM; return -1; }
  option_set=true; return 0;
}
ssize_t controlled_send(int fd,const void *data,size_t n,int flags) {
  assert(fd==live_fd && n>0); ++send_calls;
#ifdef MSG_NOSIGNAL
  assert(flags==MSG_NOSIGNAL);
#elif defined(SO_NOSIGPIPE)
  assert(option_set && flags==0);
#else
  assert(flags==0);
#endif
  if (failure==SEND_INTERRUPTED && (send_calls==1 || send_calls==3)) { errno=EINTR; return -1; }
  if (failure==SEND_ZERO) return 0;
  if (failure==SEND_ERROR) { errno=EPIPE; return -1; }
  const size_t amount=std::min(n,size_t(7));
  sent.append(static_cast<const char*>(data),amount); return amount;
}
ssize_t controlled_recv(int fd,void *data,size_t n,int) {
  assert(fd==live_fd && n>0); ++receive_calls;
  if (failure==RECEIVE_INTERRUPTED && (receive_calls==1 || receive_calls==3)) { errno=EINTR; return -1; }
  if (failure==RECEIVE) { errno=ECONNRESET; return -1; }
  const size_t amount=std::min(std::min(n,size_t(3)),response.size()-received);
  std::memcpy(data,response.data()+received,amount); received+=amount; return amount;
}
void reset(Failure next=NONE,std::string bytes="reply") {
  assert(allocations.empty() && peer_fd==-1);
  failure=next; allocation_calls=send_calls=receive_calls=0; live_fd=-1; option_set=false;
  sent.clear(); response=bytes; received=0;
}
void finish() {
  assert(allocations.empty());
  if (live_fd>=0) { errno=0; assert(fcntl(live_fd,F_GETFD)==-1 && errno==EBADF); }
  if (peer_fd>=0) { assert(close(peer_fd)==0); peer_fd=-1; }
}
#define malloc tracked_malloc
#define free tracked_free
#define socket controlled_socket
#define gethostbyname controlled_host
#define connect controlled_connect
#define setsockopt controlled_setsockopt
#define send controlled_send
#define recv controlled_recv
'''
        suffix = r'''
#undef malloc
#undef free
#undef socket
#undef gethostbyname
#undef connect
#undef setsockopt
#undef send
#undef recv
'''
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder)
            cpp = path / 'case.cpp'
            cpp.write_text(prefix + production + suffix + body)
            command = [compiler, '-std=c++17', '-Wall', '-Wextra', '-I', str(REPO / 'gldcore')]
            if option_branch:
                command.append('-DTEST_SOCKET_OPTION=1')
            result = subprocess.run(command + [str(cpp), '-o', str(path / 'case')],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(path / 'case')], capture_output=True,
                                    text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_short_writes_response_limits_and_failure_cleanup(self):
        self.run_case(r'''
int main() {
  reset(); assert(hopen(nullptr,64)==nullptr); assert(allocation_calls==0); finish();
  reset(); assert(hopen("http://host/path",0)==nullptr); assert(allocation_calls==0); finish();
  reset(); assert(hopen("not a URL",64)==nullptr); assert(allocation_calls==0); finish();
  reset(NONE,"reply");
  HTTP *h=hopen("http://host/path",6); assert(h && h->len==5);
  assert(std::memcmp(h->buf,"reply\0",6)==0 && send_calls>1);
  assert(sent=="GET /path HTTP/1.1\r\nHost: host:80\r\nUser-Agent: GridLAB-D/4.0\r\nConnection: close\r\n\r\n");
  assert(hclose(h)==1); finish();
  reset(NONE,"reply"); assert(hopen("http://host/path",5)==nullptr); finish();
  reset(NONE,""); h=hopen("http://host/path",1); assert(h && h->len==0); hclose(h); finish();
  const Failure failures[]={HANDLE_ALLOC,RESPONSE_ALLOC,SOCKET_CREATE,DNS,CONNECT,SEND_ZERO,SEND_ERROR,RECEIVE};
  for (Failure f:failures) {
    reset(f); assert(hopen("http://host/path",64)==nullptr); finish();
  }
  reset();
  const std::string oversized="http://"+std::string(700,'h')+"/"+std::string(700,'p');
  assert(hopen(oversized.c_str(),64)==nullptr && send_calls==0); finish();
}
''')

    def test_per_socket_sigpipe_option_and_option_failure_cleanup(self):
        # Compile the BSD/macOS branch on POSIX; setsockopt is simulated.
        # This checks branch logic, not the actual non-Linux kernel option.
        self.run_case(r'''
int main() {
  reset(); HTTP *h=hopen("http://host/path",64);
  assert(h && option_set && send_calls>1); hclose(h); finish();
  reset(SOCKET_OPTION);
  assert(hopen("http://host/path",64)==nullptr && !option_set && send_calls==0);
  finish();
}
''', option_branch=True)

    def test_interrupted_send_and_receive_preserve_progress(self):
        self.run_case(r'''
int main() {
  for (Failure f: {SEND_INTERRUPTED,RECEIVE_INTERRUPTED}) {
    reset(f,"reply"); HTTP *h=hopen("http://host/path",64);
    assert(h && h->len==5 && std::memcmp(h->buf,"reply\0",6)==0);
    assert(sent=="GET /path HTTP/1.1\r\nHost: host:80\r\nUser-Agent: GridLAB-D/4.0\r\nConnection: close\r\n\r\n");
    assert(hclose(h)==1); finish();
  }
}
''')
