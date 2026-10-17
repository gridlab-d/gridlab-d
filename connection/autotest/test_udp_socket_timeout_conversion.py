"""Focused regression checks for UDP socket timeout unit conversion and bounds.

Run from repository root: python3 -m unittest connection.autotest.test_udp_socket_timeout_conversion -v
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

def check_cpp(body):
    compiler = shutil.which('c++')
    if not compiler:
        raise unittest.SkipTest('C++ compiler unavailable')
    with tempfile.TemporaryDirectory() as folder:
        p = pathlib.Path(folder)
        (p / 'case.cpp').write_text(body)
        compile_result = subprocess.run([compiler,'-std=c++17','-Wall','-Wextra','-O0',str(p/'case.cpp'),'-o',str(p/'case')],capture_output=True,text=True)
        if compile_result.returncode != 0:
            raise AssertionError(compile_result.stderr)
        run_result = subprocess.run([str(p/'case')],capture_output=True,text=True)
        if run_result.returncode != 0:
            raise AssertionError(run_result.stderr + run_result.stdout)

class UDPSocketTimeoutConversionTests(unittest.TestCase):
    def test_milliseconds_convert_to_timeval_microseconds(self):
        src = read('connection/udp.cpp')
        self.assertIn('timeout.tv_usec = (n % 1000) * 1000', src)
        self.assertIn('A configured 0 ms', src)
        self.assertIn('static_cast<unsigned long long>(optval->tv_usec) / 1000ULL', src)

    def test_windows_socket_timeout_limits(self):
        src = read('connection/udp.cpp')
        # Compile the production WinSock-specific function with minimal mocks
        # so this test can run on Linux without Windows headers.
        begin = src.index('int udp::call_setsockopt(')
        end = src.index('void udp::flush(', begin)
        function = src[begin:end]
        prefix = '''#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#define _WIN32 1
using DWORD = uint32_t;
using SOCKET = int;
constexpr int SOCKET_ERROR = -1;
constexpr int WSAEINVAL = 10022;
struct timeval { long tv_sec, tv_usec; };
static int last_error = 0;
static DWORD last_timeout = 0;
void WSASetLastError(int error) { last_error = error; }
int setsockopt(SOCKET,int,int,const char *data,int len) {
    assert(len == (int)sizeof(DWORD));
    std::memcpy(&last_timeout, data, sizeof(last_timeout));
    return 0;
}
class udp { public: static int call_setsockopt(SOCKET,int,int,timeval*,int); };
'''
        tests = '''int main() {
    timeval normal{2,500000};
    assert(udp::call_setsockopt(0,0,0,&normal,sizeof(normal)) == 0);
    assert(last_timeout == 2500u);
    timeval max_allowed{4294967,295000};
    assert(udp::call_setsockopt(0,0,0,&max_allowed,sizeof(max_allowed)) == 0);
    assert(last_timeout == std::numeric_limits<DWORD>::max());
    timeval too_large{4294968,0};
    assert(udp::call_setsockopt(0,0,0,&too_large,sizeof(too_large)) == SOCKET_ERROR);
    assert(last_error == WSAEINVAL);
    timeval negative{-1,0};
    assert(udp::call_setsockopt(0,0,0,&negative,sizeof(negative)) == SOCKET_ERROR);
    timeval bad_usec{0,1000000};
    assert(udp::call_setsockopt(0,0,0,&bad_usec,sizeof(bad_usec)) == SOCKET_ERROR);
}
'''
        check_cpp(prefix + function + tests)
