"""Debug script to test worker startup and INIT command."""
import os
import sys
import subprocess
import json

# Set the same environment the worker will get
env = os.environ.copy()
env['GRIDLABD_HOME'] = r'C:\dev\gridlab-d_fork'
build_paths = [
    r'C:\dev\gridlab-d_fork\build\bin\Release',
    r'C:\dev\gridlab-d_fork\build\bin\Debug'
]
path_entries = [p for p in build_paths if os.path.isdir(p)]
if path_entries:
    env['PATH'] = os.pathsep.join(path_entries) + os.pathsep + env.get('PATH', '')

print("=== Starting worker subprocess ===")
print(f"GRIDLABD_HOME: {env.get('GRIDLABD_HOME')}")
print()

# Run worker - IMPORTANT: capture stderr to a PIPE instead of DEVNULL
process = subprocess.Popen(
    [sys.executable, "-m", "gridlabd._worker"],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,  # Capture to see errors
    text=True,
    bufsize=1,
    env=env,
    creationflags=subprocess.CREATE_NEW_PROCESS_GROUP
)

print("Waiting for READY...")
ready_line = process.stdout.readline().strip()
print(f"Received: {ready_line}")

if ready_line == "READY":
    print("\nSending INIT command...")
    init_msg = {"command": "init", "args": {}}
    process.stdin.write(json.dumps(init_msg) + "\n")
    process.stdin.flush()
    
    print("Waiting for response...")
    import time
    time.sleep(0.5)
    
    response_line = process.stdout.readline().strip()
    if response_line:
        print(f"Response: {response_line[:200]}")
    else:
        print("No response - checking process status...")
        
        # Check exit code
        exit_code = process.poll()
        if exit_code is not None:
            print(f"Worker exited with code: {exit_code}")
            
        # Get any stderr output - this is important!
        _, stderr = process.communicate(timeout=1)
        if stderr:
            print("\nSTDERR OUTPUT:")
            print(stderr)
        else:
            print("No stderr output captured")
