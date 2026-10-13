#!/usr/bin/env python3
"""Debug filesystem checks that set_install_root() is doing."""

import os
from pathlib import Path

root = Path(r'C:\dev\gridlab-d_fork')

print("Filesystem Check Debug")
print("=" * 60)
print(f"Root path: {root}")
print(f"Root exists: {root.exists()}")
print(f"Root is dir: {root.is_dir()}")
print(f"Root absolute: {root.resolve()}")

required_dirs = {
    'share': root / 'share',
    'gldcore': root / 'gldcore',
    'lib': root / 'lib',
}

print("\nRequired directories:")
for name, path in required_dirs.items():
    exists = path.exists()
    is_dir = path.is_dir() if exists else False
    print(f"  {name}:")
    print(f"    Path: {path}")
    print(f"    Exists: {exists}")
    print(f"    IsDir: {is_dir}")
    if exists:
        try:
            contents = list(path.iterdir())[:3]
            print(f"    Contents (first 3): {[c.name for c in contents]}")
        except Exception as e:
            print(f"    Error listing contents: {e}")

# Now test with the C++ code
print("\n" + "=" * 60)
print("Testing C++ validation...")

from gridlabd import gridlabd_core

# Try with absolute canonical path
canonical_root = str(root.resolve())
print(f"Trying with canonical path: {canonical_root}")

try:
    gridlabd_core.GridLabD.set_install_root(canonical_root)
    print("SUCCESS!")
except RuntimeError as e:
    print(f"ERROR: {e}")
    print(f"Error repr: {repr(e)}")
    print(f"Error args: {e.args}")
