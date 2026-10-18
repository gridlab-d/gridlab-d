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
        compile_result = subprocess.run([compiler,'-std=c++20','-Wall','-Wextra','-O0',str(p/'case.cpp'),'-o',str(p/'case')],capture_output=True,text=True)
        if compile_result.returncode != 0:
            raise AssertionError(compile_result.stderr)
        run_result = subprocess.run([str(p/'case')],capture_output=True,text=True)
        if run_result.returncode != 0:
            raise AssertionError(run_result.stderr + run_result.stdout)

class QuadraticInterpolationTests(unittest.TestCase):
    def test_interpolates_equally_spaced_quadratic_samples(self):
        src = read('gldcore/interpolate.cpp')
        body = src[src.index('double interpolate_quadratic('):src.index('// EOF')]
        p = dedent('''\
            #include <cassert>
            #include <cmath>
            #include <limits>
            #include <numeric>
            void output_error(const char *, ...) {}
        ''')
        t = dedent('''\
            int main() {
              assert(std::abs(interpolate_quadratic(3,0,0,2,4,4,16)-9.0) < 1e-12);
              assert(std::abs(interpolate_quadratic(1.5,0,0,1,1,2,4)-2.25) < 1e-12);
              assert(interpolate_quadratic(1,0,1,0,2,0,3)==0.0);
              const double x0=0.1, x1=0.2, x2=0.3;
              assert(std::abs(interpolate_quadratic(0.25,x0,x0*x0,x1,x1*x1,x2,x2*x2)-0.0625) < 1e-12);
              assert(interpolate_quadratic(0.5,0,0,1,1,2.1,4.41)==0.0);
            }
        ''')
        check_cpp(p+body+t)

    def test_extreme_spacing_and_large_constant_values(self):
        src = read('gldcore/interpolate.cpp')
        body = src[src.index('double interpolate_quadratic('):src.index('// EOF')]
        check_cpp(dedent('''\
            #include <cassert>
            #include <cmath>
            #include <limits>
            #include <numeric>
            void output_error(const char *, ...) {}
        ''') + body + dedent('''\
            int main() {
              const double spacings[] = {1e-200, 1.0, 1e200, -1e-200, -1e200};
              for (double h : spacings) {
                double value = interpolate_quadratic(1.5*h,0,0,h,1,2*h,4);
                assert(std::isfinite(value) && std::abs(value-2.25) < 1e-12);
              }
              assert(interpolate_quadratic(0.5,0,1e308,1,1e308,2,1e308)==1e308);
              assert(interpolate_quadratic(1e308,-1e308,0,0,1,1e308,4)==4.0);
            }
        '''))

    def test_subnormal_linear_samples_do_not_lose_low_bits(self):
        src = read('gldcore/interpolate.cpp')
        body = src[src.index('double interpolate_quadratic('):src.index('// EOF')]
        check_cpp(dedent('''\
            #include <cassert>
            #include <cmath>
            #include <limits>
            #include <numeric>
            void output_error(const char *, ...) {}
        ''') + body + dedent('''\
            int main() {
              const double tiny = std::numeric_limits<double>::denorm_min();
              const double times[] = {0.0, 0.5, 1.0, 1.5, 2.0};
              const double expected[] = {0.0, 0.0, tiny, 2*tiny, 2*tiny};
              for (unsigned int i=0; i<5; ++i) {
                assert(interpolate_quadratic(times[i],0,0,1,tiny,2,2*tiny)==expected[i]);
                assert(interpolate_quadratic(times[i],0,0,1,-tiny,2,-2*tiny)==-expected[i]);
              }
              const double value = interpolate_quadratic(0.5,0,-1e308,1,0,2,1e308);
              assert(std::isfinite(value) && std::abs(value/1e308+0.5)<1e-15);
            }
        '''))

    def test_curved_subnormal_samples_and_opposite_large_ordinates(self):
        src = read('gldcore/interpolate.cpp')
        body = src[src.index('double interpolate_quadratic('):src.index('// EOF')]
        check_cpp(dedent('''\
            #include <cassert>
            #include <cmath>
            #include <limits>
            #include <numeric>
            void output_error(const char *, ...) {}
        ''') + body + dedent('''\
            int main() {
              const double tiny = std::numeric_limits<double>::denorm_min();
              // Independent Lagrange evaluation in units of denorm_min.
              // All arithmetic is exact here until the final conversion.
              for (int i=-4; i<=4; ++i) {
                for (int j=-4; j<=4; ++j) {
                  for (int k=-4; k<=4; ++k) {
                    for (int q=0; q<=8; ++q) {
                      const long double t = q / 4.0L;
                      const long double units = i*(t-1)*(t-2)/2
                          - j*t*(t-2) + k*t*(t-1)/2;
                      const double expected = static_cast<double>(units*tiny);
                      assert(interpolate_quadratic(static_cast<double>(t),
                          0,i*tiny,1,j*tiny,2,k*tiny)==expected);
                    }
                  }
                }
              }
              // Adjacent differences overflow without ordinate scaling.
              assert(interpolate_quadratic(0.5,0,-1e308,1,1e308,2,1e308)==2.5e307);
              assert(interpolate_quadratic(1.5,0,-1e308,1,1e308,2,-1e308)==5e307);
              // Preserve exact nodes even with an extreme dynamic range.
              assert(interpolate_quadratic(0,0,tiny,1,1e308,2,-1e308)==tiny);
            }
        '''))
