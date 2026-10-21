"""Focused regression checks for UDP response offset and payload bounds.

Run from repository root: python3 -m unittest connection.autotest.test_udp_response_bounds -v
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

class UDPResponseBoundsTests(unittest.TestCase):
    def test_checks_response_offsets_and_payload_lengths(self):
        src = read('connection/udp.cpp')
        self.assertIn('(size_t)size>=len', src)
        self.assertIn('(size_t)offset>rcvlen', src)
        self.assertIn('size<0', src)
        self.assertIn('offset<0', src)
        self.assertIn('memcpy(sendbuf + header_len, msg, len)', src)
        self.assertIn('const size_t remaining = sizeof(output)', src)
        self.assertIn('position += static_cast<int>(written)', src)

    def test_compiled_output_cursor_bounds(self):
        src = read('connection/udp.cpp')
        production = src[src.index('void udp::set_output('):]
        compiler = shutil.which('c++')
        if not compiler:
            self.skipTest('C++ compiler unavailable')
        prefix = r'''#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
class udp {
public:
  char output[2048] = {};
  int position = 0;
  void set_output(const char*, ...);
  void add_output(const char*, ...);
};
'''
        tests = r'''int main() {
  udp u;
  u.set_output("%s", "abc"); u.add_output("%s", "def");
  assert(u.position==6 && strcmp(u.output,"abcdef")==0);
  std::string huge(5000,'x');
  u.set_output("%s", huge.c_str());
  assert(u.position==2047 && u.output[2047]=='\0');
  u.add_output("%s", "more");
  assert(u.position==2047 && u.output[2047]=='\0');
}
'''
        with tempfile.TemporaryDirectory() as d:
            p = pathlib.Path(d) / 'case.cpp'
            p.write_text(prefix + production + tests)
            exe = pathlib.Path(d) / 'case'
            r = subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra',
                                str(p), '-o', str(exe)], capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, r.stderr)
            r = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, r.stderr)
