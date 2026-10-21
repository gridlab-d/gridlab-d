"""Exercise diagnostic paths in production UDP option parsing.
Run from repository root:
    python3 -m unittest connection.autotest.test_udp_option_diagnostics -v
"""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

REPO = pathlib.Path(__file__).resolve().parents[2]

class UDPOptionDiagnosticTests(unittest.TestCase):
    def test_invalid_values_emit_errors(self):
        compiler = shutil.which('c++')
        if not compiler:
            self.skipTest('C++ compiler unavailable')
        src = (REPO/'connection/udp.cpp').read_text()
        first = src.index('static bool udp_parse_uint(')
        last = src.index('int udp::create(', first)
        source = src[first:last]
        prefix = r'''#include <cassert>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
class udp {
public:
  std::string message;
  unsigned int port=0;
  void error(const char *fmt,...) {
    char buffer[4096]; va_list args; va_start(args,fmt);
    std::vsnprintf(buffer,sizeof(buffer),fmt,args); va_end(args);
    message=buffer;
  }
  void set_portnum(unsigned int n) { port=n; }
  void set_header_version(unsigned int) {}
  void set_timeout(unsigned int) {}
  void set_hostname(const char*) {}
  void set_uri(const char*,...) {}
  void set_debug_level(unsigned int) {}
  void onerror_abort() {}
  void onerror_retry() {}
  void onerror_ignore() {}
  void set_maxretry(int=-1) {}
  int option(char*);
};
'''
        suffix = r'''int main() {
  udp u;
  const char *bad[] = {
    "port=0", "port=65536", "port=-1", "timeout=-2", "timeout=abc",
    "header_version=1junk", "debug_level=+2", "maxretry=2147483648",
    "maxretry=0", "on_error=explode"
  };
  for (const char *item : bad) {
    char value[256]; std::snprintf(value,sizeof(value),"%s",item);
    u.message.clear();
    assert(u.option(value)==0);
    assert(!u.message.empty());
    assert(u.message.find("invalid UDP") != std::string::npos);
  }
  char ok[]="port=65535,timeout=0,on_error=retry,maxretry=none";
  u.message.clear();
  assert(u.option(ok)==1 && u.port==65535 && u.message.empty());
}
'''
        with tempfile.TemporaryDirectory() as d:
            source_file = pathlib.Path(d)/'test.cpp'
            source_file.write_text(prefix+source+suffix)
            exe = pathlib.Path(d)/'test'
            result = subprocess.run([compiler,'-std=c++17','-Wall','-Wextra',
                                     str(source_file),'-o',str(exe)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            result = subprocess.run([str(exe)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
