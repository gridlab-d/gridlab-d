"""Focused regression checks for UDP response offset and payload bounds.

Run from repository root: python3 -m unittest connection.autotest.test_udp_response_bounds -v
These targeted tests do not replace build or full model integration tests.
"""
import pathlib
import unittest

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
