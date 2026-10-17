"""Source invariants for HTTP client request bounds and cleanup.
Run from repository root: python3 -m unittest gldcore.autotest.test_http_request_bounds -v
"""
import pathlib, unittest
ROOT=pathlib.Path(__file__).resolve().parents[2]
class HTTPRequestBoundsTests(unittest.TestCase):
    def test_request_length_and_socket_cleanup_guards(self):
        src=(ROOT/'gldcore/http_client.cpp').read_text()
        self.assertRegex(src,r'snprintf\(request,\s*sizeof\(request\)')
        self.assertIn('if (len < 0 || (size_t)len >= sizeof(request))',src)
        self.assertIn('http->sd = INVALID_SOCKET;',src)
        self.assertIn('if (http->sd != INVALID_SOCKET)',src)
        self.assertIn('hclose(http);',src)
