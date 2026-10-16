"""Focused regression checks for GridLAB-D PR #1805.

Run from repository root: python3 -m unittest discover -s tests/pr_fixes -p 'test_pr_*.py'
These targeted tests do not replace build or full model integration tests.
"""
import pathlib
import unittest
import shutil
import subprocess
import tempfile
from textwrap import dedent

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

class PRRegression(unittest.TestCase):
    def test_source_invariants_0(self):
        src = read('connection/udp.cpp')
        self.assertIn('timeout.tv_usec = (n % 1000) * 1000', src)
        self.assertIn('A configured 0 ms', src)
        self.assertIn('tv_usec / 1000', src)
