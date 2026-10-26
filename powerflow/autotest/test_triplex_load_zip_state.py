"""Focused regression checks for triplex-load ZIP constant-current state reset.

Run from repository root: python3 -m unittest powerflow.autotest.test_triplex_load_zip_state -v
These targeted tests do not replace build or full model integration tests.
"""
import pathlib
import unittest

REPO = pathlib.Path(__file__).resolve().parents[2]

def read(path):
    return (REPO / path).read_text(encoding='utf-8')

class TriplexLoadZIPStateTests(unittest.TestCase):
    def test_resets_constant_current_flags_when_disabled(self):
        src = read('powerflow/triplex_load.cpp')
        self.assertIn('ZIP_constant_current[0] = false', src)
        self.assertIn('ZIP_constant_current[1] = false', src)
        self.assertIn('ZIP_constant_current[2] = false', src)
