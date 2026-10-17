"""Focused regression checks for quadratic interpolation with non-unit spacing.

Run from repository root: python3 -m unittest gldcore.autotest.test_quadratic_interpolation -v
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

class QuadraticInterpolationTests(unittest.TestCase):
    def test_quadratic_coefficient_uses_sample_spacing(self):
        src = read('gldcore/interpolate.cpp')
        self.assertIn('(2*h*h)', src)
        self.assertIn('Requires equally spaced', src)

    def test_interpolates_equally_spaced_quadratic_samples(self):
        src = read('gldcore/interpolate.cpp')
        body = src[src.index('double interpolate_quadratic('):src.index('// EOF')]
        p = dedent('''\
            #include <cassert>
            #include <cmath>
            void output_error(const char *, ...) {}
        ''')
        t = dedent('''\
            int main() {
              assert(std::abs(interpolate_quadratic(3,0,0,2,4,4,16)-9.0) < 1e-12);
              assert(std::abs(interpolate_quadratic(1.5,0,0,1,1,2,4)-2.25) < 1e-12);
              assert(interpolate_quadratic(1,0,1,0,2,0,3)==0.0);
            }
        ''')
        check_cpp(p+body+t)
