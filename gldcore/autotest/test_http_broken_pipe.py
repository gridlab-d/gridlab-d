"""Exercise the production HTTP send path against a closed POSIX socket."""
import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest

REPO = pathlib.Path(__file__).resolve().parents[2]


class HTTPBrokenPipeTests(unittest.TestCase):
    @unittest.skipIf(os.name == 'nt', 'POSIX SIGPIPE/socketpair harness')
    def test_closed_peer_returns_failure_without_terminating_process(self):
        compiler = shutil.which('c++')
        if not compiler:
            self.skipTest('C++ compiler unavailable')
        source = (REPO / 'gldcore/http_client.cpp').read_text()
        production = source[source.index('HTTP *hopen('):source.index('size_t hfilelength(')]
        prefix = r'''
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include "http_client.h"
#define REV_MAJOR 4
#define REV_MINOR 0
void output_error(const char*,...) {}
void output_debug(const char*,...) {}
int live_fd=-1;
int broken_socket(int,int,int) {
  int pair[2]; assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0);
  close(pair[1]); live_fd=pair[0]; return live_fd;
}
int fake_connect(int,const sockaddr*,socklen_t) { return 0; }
hostent *fake_host(const char*) {
  static in_addr address={};
  static char *addresses[]={reinterpret_cast<char*>(&address),nullptr};
  static hostent host={}; host.h_length=sizeof(address); host.h_addr_list=addresses;
  return &host;
}
#define socket broken_socket
#define connect fake_connect
#define gethostbyname fake_host
'''
        cases = r'''
int main() {
  signal(SIGPIPE,SIG_DFL);
  for (int i=0;i<16;++i) {
    assert(hopen("http://localhost/test",1024)==nullptr);
    errno=0; assert(fcntl(live_fd,F_GETFD)==-1 && errno==EBADF);
  }
}
'''
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder)
            cpp = path / 'case.cpp'
            cpp.write_text(prefix + production + cases)
            result = subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra',
                                     '-I', str(REPO / 'gldcore'), str(cpp),
                                     '-o', str(path / 'case')],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(path / 'case')], capture_output=True,
                                    text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
