#!/usr/bin/env python3
"""Test GridLabD() constructor with timeout."""

import sys
import os
import threading
import time

os.environ['GRIDLABD_HOME'] = r'C:\dev\gridlab-d_fork'
os.environ['GLPATH'] = r'C:\dev\gridlab-d_fork;C:\dev\gridlab-d_fork\gldcore;C:\dev\gridlab-d_fork\share'

from gridlabd import gridlabd_core

print("=" * 60)
print("Testing GridLabD() Constructor")
print("=" * 60)

# Set up install root first
print("\n1. Setting install root...")
try:
    gridlabd_core.GridLabD.set_install_root(r'C:\dev\gridlab-d_fork')
    print("   ✓ set_install_root() succeeded")
except Exception as e:
    print(f"   ✗ set_install_root() failed: {e}")
    sys.exit(1)

# Now test constructor in a thread with timeout
print("\n2. Creating GridLabD() instance in thread...")
result = [None]
error = [None]

def create_instance():
    try:
        print("   >> Thread: Calling GridLabD()...", flush=True)
        result[0] = gridlabd_core.GridLabD()
        print("   >> Thread: SUCCESS - instance created!", flush=True)
    except Exception as e:
        error[0] = e
        print(f"   >> Thread: ERROR - {type(e).__name__}: {e}", flush=True)

thread = threading.Thread(target=create_instance, daemon=False)
thread.start()

# Wait up to 15 seconds
for i in range(15):
    time.sleep(1)
    if not thread.is_alive():
        if error[0]:
            print(f"\n   ✗ Thread exited with error: {error[0]}")
            sys.exit(1)
        elif result[0]:
            print(f"\n   ✓ Constructor completed successfully!")
            print(f"   Instance type: {type(result[0])}")
            sys.exit(0)
        else:
            print(f"\n   ? Thread exited but no result")
            sys.exit(1)
    else:
        print(f"   >> Thread still running... ({i+1}s)", flush=True)

print(f"\n   ✗ TIMEOUT: Constructor is hanging (15+ seconds)")
print("\nPossible causes:")
print("  - timestamp_set_tz() blocking on file I/O")
print("  - load_tzspecs() searching GLPATH")
print("  - Some system call blocking on Windows")
sys.exit(1)
