#!/usr/bin/env python3
"""Test validation by catching the full exception and examining it."""

import sys
import os
import ctypes

# Check DLL details
dll_path = r'C:\dev\gridlab-d_fork\python_bindings\prebuilt\lib\gldapi.dll'
print(f"DLL path: {dll_path}")
print(f"DLL exists: {os.path.exists(dll_path)}")
print(f"DLL size: {os.path.getsize(dll_path) if os.path.exists(dll_path) else 'N/A'}")
print(f"DLL modified: {os.path.getmtime(dll_path) if os.path.exists(dll_path) else 'N/A'}")
print()

# Now test
os.environ['GRIDLABD_HOME'] = r'C:\dev\gridlab-d_fork'
os.environ['GLPATH'] = r'C:\dev\gridlab-d_fork;C:\dev\gridlab-d_fork\gldcore;C:\dev\gridlab-d_fork\share'

try:
    from gridlabd import gridlabd_core
    print("Successfully imported gridlabd_core")
    print(f"GridLabD class: {gridlabd_core.GridLabD}")
    
    # Try to call it and capture the FULL error
    try:
        print("\nCalling set_install_root...")
        gridlabd_core.GridLabD.set_install_root(r'C:\dev\gridlab-d_fork')
        print("SUCCESS!")
    except Exception as e:
        print(f"\nEXCEPTION TYPE: {type(e)}")
        print(f"EXCEPTION: {repr(e)}")
        print(f"EXCEPTION STR: {str(e)}")
        print(f"EXCEPTION ARGS: {e.args}")
        if hasattr(e, '__traceback__'):
            import traceback
            traceback.print_exc()
        sys.exit(1)
        
except Exception as e:
    print(f"Failed to import: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
