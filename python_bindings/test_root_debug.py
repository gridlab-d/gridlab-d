#!/usr/bin/env python3
import sys
import os
sys.path.insert(0, 'src')

# Test 1: Direct C++ import
print("=== Test 1: Direct C++ module ===")
from gridlabd.gridlabd_core import GridLabD as CppGridLabD

try:
    CppGridLabD.set_install_root('/mnt/c/dev/gridlab-d_fork')
    print('✓ set_install_root succeeded')
    root = CppGridLabD.get_install_root()
    print(f'  Root: "{root}"')
    print(f'  Root length: {len(root)}')
except Exception as e:
    print(f'✗ Error: {e}')
    import traceback
    traceback.print_exc()

# Test 2: Through __init__.py with GRIDLABD_ROOT set
print("\n=== Test 2: Full import with GRIDLABD_ROOT ===")
os.environ['GRIDLABD_ROOT'] = '/mnt/c/dev/gridlab-d_fork'
import gridlabd
root = gridlabd.GridLabD.get_install_root()
print(f'  Root: "{root}"')
print(f'  Root length: {len(root)}')
