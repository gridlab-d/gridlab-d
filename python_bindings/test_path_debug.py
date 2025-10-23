import sys
import os
sys.path.insert(0, 'src')

from gridlabd.simulation import Simulation

# Create simulation instance and test path detection
sim = Simulation()
print("Testing path detection...")

# Test each path manually
test_paths = [
    "../cmake-build/bin/gridlabd",
    "../../cmake-build/bin/gridlabd", 
    "/mnt/c/Projects/Gridlab-d/gridlab-d/cmake-build/bin/gridlabd",
]

for path in test_paths:
    abs_path = os.path.abspath(path) if not os.path.isabs(path) else path
    exists = os.path.exists(abs_path)
    executable = os.access(abs_path, os.X_OK) if exists else False
    print(f"Path: {path}")
    print(f"  Absolute: {abs_path}")
    print(f"  Exists: {exists}")
    print(f"  Executable: {executable}")
    print()

# Test the actual detection method
detected_path = sim._detect_gridlabd_path()
print(f"Detected path: {detected_path}")
