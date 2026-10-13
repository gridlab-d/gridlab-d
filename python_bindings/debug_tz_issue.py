#!/usr/bin/env python3
"""Debug which function in GridLabD() constructor is hanging."""

import sys
import os
import threading
import time
from pathlib import Path

# Set up environment
install_root = r'C:\dev\gridlab-d_fork'
os.environ['GRIDLABD_HOME'] = install_root
os.environ['GLPATH'] = f'{install_root};{install_root}\\gldcore;{install_root}\\share'

print("=" * 60)
print("TESTING: GridLabD() Constructor Hang Debug")
print("=" * 60)

# Verify paths exist
print("\nVerifying installation structure...")
required_dirs = ['gldcore', 'share', 'build']
for d in required_dirs:
    path = Path(install_root) / d
    exists = "✓" if path.exists() else "✗"
    print(f"  {exists} {d}: {path}")

from gridlabd import gridlabd_core

# Test 1: Static methods
print("\nTest 1: Calling static methods...")
try:
    print(f"  - set_install_root('{install_root}')...", end='', flush=True)
    gridlabd_core.GridLabD.set_install_root(install_root)
    print(" SUCCESS")
    
    print("  - get_install_root()...", end='', flush=True)
    root = gridlabd_core.GridLabD.get_install_root()
    print(f" SUCCESS: {root}")
except Exception as e:
    print(f" ERROR: {e}")
    print("\nIf this fails, set_install_root() is validating the directory structure.")
    print("Make sure the directory exists and has gldcore/, share/, or lib/ subdirectories.")
    sys.exit(1)

# Test 2: Constructor in thread with timeout
print("\nTest 2: Calling GridLabD() constructor in thread...")
print("  - Starting thread...", flush=True)

result = [None]
error = [None]
started = [False]

def instantiate():
    try:
        started[0] = True
        print("    >> Thread started, creating instance...", flush=True)
        result[0] = gridlabd_core.GridLabD()
        print("    >> SUCCESS: Instance created", flush=True)
    except Exception as e:
        error[0] = e
        print(f"    >> ERROR: {type(e).__name__}: {e}", flush=True)

thread = threading.Thread(target=instantiate, daemon=False)
thread.start()

# Wait with status
for i in range(15):
    time.sleep(1)
    if not thread.is_alive():
        if error[0]:
            print(f"  - Thread crashed after {i+1}s: {error[0]}")
            sys.exit(1)
        elif result[0]:
            print(f"  - Thread completed after {i+1}s: SUCCESS")
            sys.exit(0)
        else:
            print(f"  - Thread completed but no result after {i+1}s: ERROR")
            sys.exit(1)
    elif not started[0]:
        print(f"  - Waiting for thread to start... ({i+1}s)", flush=True)
    else:
        print(f"  - Thread still running... ({i+1}s)", flush=True)

print("\n  - TIMEOUT: Constructor is hanging (15+ seconds)")
print("\nPossible causes:")
print("  1. timestamp_set_tz() is blocking on file I/O")
print("  2. Exception handling is deadlocking in Python extension context")
print("  3. Some other initialization is waiting for I/O or system resource")
sys.exit(1)
