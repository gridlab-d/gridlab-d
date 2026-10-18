"""Focused regression checks for UDP option name and value parsing bounds.

Run from repository root: python3 -m unittest connection.autotest.test_udp_option_parsing_bounds -v
These targeted tests do not replace build or full model integration tests.
"""
import pathlib
import unittest
import shutil
import subprocess
import tempfile

REPO = pathlib.Path(__file__).resolve().parents[2]

def read(path):
    return (REPO / path).read_text(encoding='utf-8')

class UDPOptionParsingBoundsTests(unittest.TestCase):
    def test_option_parser_checks_name_and_value_bounds(self):
        src = read('connection/udp.cpp')
        self.assertIn('udp_parse_uint', src)
        self.assertIn('value_len >= 1024', src)
        self.assertIn('number > 65535', src)
        self.assertIn('errno == ERANGE', src)
        self.assertIn('unknown transport option', src)

    def test_compiled_option_parser(self):
        src = read('connection/udp.cpp')
        start = src.index('static bool udp_parse_uint(')
        end = src.index('int udp::create(', start)
        production = src[start:end]
        compiler = shutil.which('c++')
        if not compiler:
            self.skipTest('C++ compiler unavailable')
        harness = r'''#include <cassert>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
class udp {
public:
  unsigned int port=0, timeout=0;
  int maxretry=0;
  void error(const char*,...) {}
  void set_portnum(unsigned int n) { port=n; }
  void set_header_version(unsigned int) {}
  void set_timeout(unsigned int n) { timeout=n; }
  void set_hostname(const char*) {}
  void set_uri(const char*,...) {}
  void set_debug_level(unsigned int) {}
  void onerror_abort() {}
  void onerror_retry() {}
  void onerror_ignore() {}
  void set_maxretry(int n=-1) { maxretry=n; }
  int option(char*);
};
'''
        cases = r'''int main() {
  udp u;
  char ok[]="port=6267;timeout=1500,maxretry=3";
  assert(u.option(ok)==1 && u.port==6267 && u.timeout==1500 && u.maxretry==3);
  char bad1[]="port=-1"; assert(u.option(bad1)==0);
  char bad2[]="port=65536"; assert(u.option(bad2)==0);
  char bad3[]="timeout=123junk"; assert(u.option(bad3)==0);
  char bad4[]="timeout=4294967296"; assert(u.option(bad4)==0);
  char bad5[]="port=;timeout=1"; assert(u.option(bad5)==0);
  char bad6[]="maxretry=2147483648"; assert(u.option(bad6)==0);
}
'''
        with tempfile.TemporaryDirectory() as d:
            p = pathlib.Path(d) / 'case.cpp'
            p.write_text(harness + production + cases)
            exe = pathlib.Path(d) / 'case'
            r = subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra',
                                str(p), '-o', str(exe)], capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, r.stderr)
            r = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, r.stderr)
