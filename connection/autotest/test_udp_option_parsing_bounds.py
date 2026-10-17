"""Focused regression checks for UDP option name and value parsing bounds.

Run from repository root: python3 -m unittest connection.autotest.test_udp_option_parsing_bounds -v
These targeted tests do not replace build or full model integration tests.
"""
import pathlib
import unittest

REPO = pathlib.Path(__file__).resolve().parents[2]

def read(path):
    return (REPO / path).read_text(encoding='utf-8')

class UDPOptionParsingBoundsTests(unittest.TestCase):
    def test_option_parser_checks_name_and_value_bounds(self):
        src = read('connection/udp.cpp')
        self.assertIn('size_t keylen = strcspn', src)
        self.assertIn('strcspn(value_start', src)
        self.assertIn('keylen>=sizeof(param)', src)
        self.assertIn('%1023[^,;]', src)
