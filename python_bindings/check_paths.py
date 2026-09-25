#!/usr/bin/env python3
"""Check if the directories actually exist."""

from pathlib import Path

root = Path(r'C:\dev\gridlab-d_fork')
print(f"Root: {root}")
print(f"Exists: {root.exists()}")
print(f"Is dir: {root.is_dir()}")
print()

for dirname in ['gldcore', 'share', 'lib']:
    path = root / dirname
    print(f"{dirname}:")
    print(f"  Path: {path}")
    print(f"  Exists: {path.exists()}")
    print(f"  Is dir: {path.is_dir()}")
    if path.exists():
        try:
            files = list(path.iterdir())[:5]
            print(f"  Sample contents: {[f.name for f in files]}")
        except:
            pass
    print()

# Also check what Python sees as the absolute path
print(f"Absolute path: {root.resolve()}")
print(f"Absolute path string: {str(root.resolve())}")
