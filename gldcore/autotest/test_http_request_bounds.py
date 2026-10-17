"""Source invariants for HTTP client request bounds and cleanup.
Run with python3 -m unittest gldcore/autotest/test_pr_1812.py.
"""
import pathlib, re, unittest
ROOT=pathlib.Path(__file__).resolve().parents[2]
class RequestChecks(unittest.TestCase):
    def test_request_and_cleanup(self):
        src=(ROOT/'gldcore/http_client.cpp').read_text()
        self.assertRegex(src,r'snprintf\(request,\s*sizeof\(request\)')
        self.assertIn('if (len < 0 || (size_t)len >= sizeof(request))',src)
        self.assertIn('http->sd = INVALID_SOCKET;',src)
        self.assertIn('if (http->sd != INVALID_SOCKET)',src)
        self.assertIn('hclose(http);',src)
