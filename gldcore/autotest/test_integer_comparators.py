"""Focused regression checks for integer comparator widths and unsigned ordering.

Run from repository root: python3 -m unittest gldcore.autotest.test_integer_comparators -v
These targeted tests do not replace build or full model integration tests.
"""
import pathlib
import unittest
import shutil
import subprocess
import tempfile

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

class IntegerComparatorTests(unittest.TestCase):
    def test_unsigned_comparator_macros_are_defined(self):
        src = read('gldcore/compare.cpp')
        self.assertIn('COMPAREOPU(set,uint64)', src)
        self.assertIn('COMPAREOPU(u32,uint32)', src)
        self.assertIn('COMPARE_SIGNED', src)

    def test_property_dispatch_includes_unsigned_types(self):
        src = read('gldcore/property.cpp')
        self.assertIn('{TCOPS(set)}', src)
        self.assertIn('{TCOPS(uint32)}', src)
        self.assertIn('{TCOPS(u32)}', src)

    def test_unsigned_comparator_declarations_exist(self):
        src = read('gldcore/compare.h')
        self.assertIn('TCOPD(set);', src)
        self.assertIn('TCOPD(u32);', src)

    def test_signed_and_unsigned_comparisons(self):
        src = read('gldcore/compare.cpp')
        # Compile the comparator definitions from production source, not copies.
        begin = src.index('template <typename U> static U compare_unsigned_bits(')
        end = src.index('#define COMPAREOPB(', begin)
        macro = src[begin:end]
        prefix = src[src.index('#define COMPARE_SIGNED('):src.index('\n',src.index('#define COMPARE_SIGNED('))]
        code = '#include <cassert>\n#include <cstdint>\n#include <cstring>\n#include <type_traits>\nusing uint64=uint64_t;\n' + prefix + '\n' + macro + '''\nCOMPAREOPU(set,uint64)\nint main(){uint64 z=0,h=(1ULL<<63); assert(compare_tc_set_lt(&z,&h,nullptr)); assert(compare_tc_set_gt(&h,&z,nullptr)); assert(compare_tc_set_in(&h,&z,&h));}\n'''
        check_cpp(code)

    def test_signed_widths_and_unsigned_high_bits(self):
        src = read('gldcore/compare.cpp')
        # Compile the actual production macros with native signed and unsigned
        # values, including the sign bit and null range upper bounds.
        macros = src[src.index('#define COMPAREOPI('):src.index('#define COMPAREOPB(')]
        code = ('''#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <limits>
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
''' + macros + '''
COMPAREOPI(uint16)
COMPAREOPI(uint32)
COMPAREOPI(uint64)
COMPAREOPU(set,uint64)
COMPAREOPU(u32,uint32)
int main() {
    int16_t neg16=-1, zero16=0;
    assert(compare_tc_uint16_lt(&neg16,&zero16,nullptr));
    int32_t neg32=-1, zero32=0;
    assert(compare_tc_uint32_lt(&neg32,&zero32,nullptr));
    int64_t min64=std::numeric_limits<int64_t>::min(), zero64=0, neg64=-1;
    assert(compare_tc_uint64_lt(&min64,&zero64,nullptr));
    assert(compare_tc_uint64_gt(&zero64,&neg64,nullptr));
    assert(compare_tc_uint64_in(&neg64,&min64,&zero64));
    assert(!compare_tc_uint64_in(&neg64,&min64,nullptr));
    assert(compare_tc_uint64_ni(&neg64,&min64,nullptr));
    uint32_t high32=0x80000000u, low32=0;
    assert(compare_tc_u32_gt(&high32,&low32,nullptr));
    uint64_t high64=UINT64_C(1)<<63, low64=0;
    assert(compare_tc_set_gt(&high64,&low64,nullptr));
}
''' )
        check_cpp(code)
