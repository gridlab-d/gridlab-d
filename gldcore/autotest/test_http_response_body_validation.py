"""Focused HTTP response body and Content-Length regression checks.
Run from repository root: python3 -m unittest gldcore.autotest.test_http_response_body_validation -v
These do not replace gridlabd --validate or full module integration.
"""
import pathlib, subprocess, shutil, tempfile, unittest
REPO=pathlib.Path(__file__).resolve().parents[2]
class HTTPResponseBodyValidationTests(unittest.TestCase):
    def test_download_status_length_and_cleanup_guards(self):
        s=(REPO/'gldcore/http_client.cpp').read_text()
        for x in ['http_body_is_complete(result)','std::numeric_limits<unsigned long long>','fopen(file,"wb")','http_delete_result(result)']:
            self.assertIn(x,s)
    def test_rejects_invalid_content_length_headers_and_body_sizes(self):
        s=(REPO/'gldcore/http_client.cpp').read_text()
        begin=s.index('static bool http_body_is_complete(')
        end=s.index('int http_saveas(',begin)
        helper=s[begin:end]
        cxx=shutil.which('c++')
        if not cxx: self.skipTest('C++ compiler unavailable')
        prefix='#include <cassert>\n#include <cctype>\n#include <limits>\n#include <cstddef>\nstruct HTTPBUFFER { const char *data; int size; };\nstruct HTTPRESULT { HTTPBUFFER header,body; };\n'
        tests='int main(){\n  const char *ok="HTTP/1.1 200 OK\\r\\nContent-Length: 4\\r\\n\\r\\n";\n  HTTPRESULT r={{ok,(int)__builtin_strlen(ok)},{"abcd",4}};\n  assert(http_body_is_complete(&r)); r.body.size=3; assert(!http_body_is_complete(&r)); r.body.size=5; assert(!http_body_is_complete(&r)); r.body.size=4;\n  const char *bad="HTTP/1.1 200 OK\\r\\nContent-Length: -1\\r\\n\\r\\n";\n  r.header={bad,(int)__builtin_strlen(bad)}; assert(!http_body_is_complete(&r));\n  const char *dup="HTTP/1.1 200 OK\\r\\nContent-Length: 3\\r\\ncontent-length: 4\\r\\n\\r\\n";\n  r.header={dup,(int)__builtin_strlen(dup)}; assert(!http_body_is_complete(&r));\n}\n'
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/'t.cpp'; p.write_text(prefix+helper+tests)
            exe=pathlib.Path(d)/'t'
            res=subprocess.run([cxx,'-std=c++17','-Wall','-Wextra',str(p),'-o',str(exe)],capture_output=True,text=True)
            self.assertEqual(res.returncode,0,res.stderr)
            res=subprocess.run([str(exe)],capture_output=True,text=True)
            self.assertEqual(res.returncode,0,res.stderr)

    def test_preserves_binary_response_body_prefix(self):
        s=(REPO/'gldcore/http_client.cpp').read_text()
        begin=s.index('static size_t http_body_offset(')
        end=s.index('void *http_read(',begin)
        helper=s[begin:end]
        cxx=shutil.which('c++')
        if not cxx: self.skipTest('C++ compiler unavailable')
        code=r'''#include <cassert>
#include <cstddef>
'''+helper+r'''
int main() {
  const char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\n\t \0A";
  const size_t prefix = sizeof("HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\n")-1;
  size_t offset = http_body_offset(response, sizeof(response)-1);
  assert(offset == prefix);
  assert(response[offset] == '\t');
  assert(response[offset+1] == ' ');
  assert(response[offset+2] == '\0');
  assert(response[offset+3] == 'A');
  assert(http_body_offset("invalid",7) == 0);
}
'''
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/'t.cpp'; p.write_text(code)
            exe=pathlib.Path(d)/'t'
            res=subprocess.run([cxx,'-std=c++17','-Wall','-Wextra',str(p),'-o',str(exe)],capture_output=True,text=True)
            self.assertEqual(res.returncode,0,res.stderr)
            res=subprocess.run([str(exe)],capture_output=True,text=True)
            self.assertEqual(res.returncode,0,res.stderr)
