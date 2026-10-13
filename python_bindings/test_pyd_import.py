#!/usr/bin/env python3
"""Test direct import of gridlabd_core extension module."""

import sys
import traceback
import threading
import time

print("Testing direct import of gridlabd_core...", flush=True)
print(f"Python: {sys.executable}", flush=True)
print(f"Python version: {sys.version}", flush=True)

try:
    print("\n1. Importing gridlabd_core...", flush=True)
    from gridlabd import gridlabd_core
    print("   SUCCESS - gridlabd_core imported", flush=True)
    
    print("\n2. Checking GridLabD class...", flush=True)
    print(f"   GridLabD: {gridlabd_core.GridLabD}", flush=True)
    
    print("\n3. Creating GridLabD instance...", flush=True)
    print("   (Starting with 10 second timeout...)", flush=True)
    
    result = [None]
    exception = [None]
    started = [False]
    completed = [False]
    
    def create_instance():
        try:
            started[0] = True
            print("   >> Thread: About to call GridLabD()...", flush=True)
            sys.stdout.flush()
            result[0] = gridlabd_core.GridLabD()
            completed[0] = True
            print("   >> Thread: GridLabD() returned successfully", flush=True)
        except Exception as e:
            completed[0] = True
            exception[0] = e
            print(f"   >> Thread: GridLabD() raised: {type(e).__name__}: {e}", flush=True)
    
    thread = threading.Thread(target=create_instance, daemon=True)
    thread.start()
    
    # Wait with status updates
    for i in range(10):
        time.sleep(1)
        if completed[0]:
            print(f"   >> Thread completed after {i+1} second(s)", flush=True)
            break
        elif started[0]:
            print(f"   >> Still running... ({i+1}s)", flush=True)
        else:
            print(f"   >> Waiting for thread to start... ({i+1}s)", flush=True)
    
    thread.join(timeout=1)
    
    if not completed[0]:
        if not started[0]:
            print("   ERROR: Thread never started!", flush=True)
        else:
            print("   TIMEOUT - GridLabD() is still running after 10 seconds (possible hang/crash)", flush=True)
        sys.exit(1)
    
    if exception[0]:
        raise exception[0]
    
    instance = result[0]
    print(f"   SUCCESS - Created instance: {instance}", flush=True)
    
    print("\n4. All tests passed!", flush=True)
    
except Exception as e:
    print(f"\n   ERROR: {type(e).__name__}: {e}", flush=True)
    print("\nFull traceback:", flush=True)
    traceback.print_exc()
    sys.exit(1)
