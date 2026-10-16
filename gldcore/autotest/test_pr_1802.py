"""Focused regression checks for GridLAB-D PR #1802.

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
        src = read('gldcore/compare.cpp')
        self.assertIn('COMPAREOPU(set,uint64)', src)
        self.assertIn('COMPAREOPU(u32,uint32)', src)
        self.assertIn('COMPARE_SIGNED', src)

    def test_source_invariants_1(self):
        src = read('gldcore/property.cpp')
        self.assertIn('{TCOPS(set)}', src)
        self.assertIn('{TCOPS(uint32)}', src)
        self.assertIn('{TCOPS(u32)}', src)

    def test_source_invariants_2(self):
        src = read('gldcore/compare.h')
        self.assertIn('TCOPD(set);', src)
        self.assertIn('TCOPD(u32);', src)

    def test_signed_and_unsigned_comparisons(self):
        src = read('gldcore/compare.cpp')
        # Compile the comparator definitions from production source, not copies.
        begin = src.index('#define COMPAREOPU(')
        end = src.index('\n\n', begin)
        macro = src[begin:end]
        prefix = src[src.index('#define COMPARE_SIGNED('):src.index('\n',src.index('#define COMPARE_SIGNED('))]
        code = '#include <cassert>\n#include <cstdint>\n#include <type_traits>\nusing uint64=uint64_t;\n' + prefix + '\n' + macro + '''\nCOMPAREOPU(set,uint64)\nint main(){uint64 z=0,h=(1ULL<<63); assert(compare_tc_set_lt(&z,&h,nullptr)); assert(compare_tc_set_gt(&h,&z,nullptr)); assert(compare_tc_set_in(&h,&z,&h));}\n'''
        check_cpp(code)
