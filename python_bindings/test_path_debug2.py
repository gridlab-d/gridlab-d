import sys
import os
sys.path.insert(0, 'src')

# Test path detection directly
print("Testing path detection...")
print(f"Current working directory: {os.getcwd()}")

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
    if exists and executable:
        print(f"  ✓ FOUND VALID PATH!")
    print()

# Test the detect method without creating Simulation
from gridlabd.simulation import Simulation

# Directly test the path detection method
test_sim = object.__new__(Simulation)  # Create without calling __init__
detected_path = test_sim._detect_gridlabd_path()
print(f"Detected path: {detected_path}")
