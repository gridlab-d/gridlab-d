"""Focused HTTP Content-Length regression checks. Run explicitly with Python unittest.
These do not replace gridlabd --validate or full module integration.
"""
import pathlib, subprocess, shutil, tempfile, unittest
REPO=pathlib.Path(__file__).resolve().parents[2]
class HTTPChecks(unittest.TestCase):
    def test_handler_checks(self):
        s=(REPO/'gldcore/http_client.cpp').read_text()
        for x in ['http_body_is_complete(result)','std::numeric_limits<unsigned long long>','fopen(file,"wb")','http_delete_result(result)']:
            self.assertIn(x,s)
    def test_compiled_body_length_validator(self):
        s=(REPO/'gldcore/http_client.cpp').read_text()
        begin=s.index('static bool http_body_is_complete(')
        end=s.index('int http_saveas(',begin)
        helper=s[begin:end]
        cxx=shutil.which('c++')
        if not cxx: self.skipTest('C++ compiler unavailable')
        prefix='#include <cassert>\n#include <cctype>\n#include <limits>\n#include <cstddef>\nstruct HTTPBUFFER { const char *data; int size; };\nstruct HTTPRESULT { HTTPBUFFER header,body; };\n'
        tests='int main(){\n  const char *ok="HTTP/1.1 200 OK\\r\\nContent-Length: 4\\r\\n\\r\\n";\n  HTTPRESULT r={{ok,(int)__builtin_strlen(ok)},{"abcd",4}};\n  assert(http_body_is_complete(&r)); r.body.size=3; assert(!http_body_is_complete(&r));\n  const char *bad="HTTP/1.1 200 OK\\r\\nContent-Length: -1\\r\\n\\r\\n";\n  r.header={bad,(int)__builtin_strlen(bad)}; assert(!http_body_is_complete(&r));\n  const char *dup="HTTP/1.1 200 OK\\r\\nContent-Length: 3\\r\\ncontent-length: 4\\r\\n\\r\\n";\n  r.header={dup,(int)__builtin_strlen(dup)}; assert(!http_body_is_complete(&r));\n}\n'
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/'t.cpp'; p.write_text(prefix+helper+tests)
            exe=pathlib.Path(d)/'t'
            res=subprocess.run([cxx,'-std=c++17','-Wall','-Wextra',str(p),'-o',str(exe)],capture_output=True,text=True)
            self.assertEqual(res.returncode,0,res.stderr)
            res=subprocess.run([str(exe)],capture_output=True,text=True)
            self.assertEqual(res.returncode,0,res.stderr)
