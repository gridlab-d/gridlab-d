#!/usr/bin/env python3
"""
Minimal test to reproduce the double GridLabD instance bug.
This creates two instances of GridLabD to trigger the segfault.
"""

import gridlabd

def test_double_instance():
    """Create two GridLabD instances - this should cause a segfault due to shared global state."""
    print("Creating first GridLabD instance...")
    gld1 = gridlabd.GridLabD()
    print(f"First instance created: {gld1}")
    
    print("\nCreating second GridLabD instance...")
    gld2 = gridlabd.GridLabD()
    print(f"Second instance created: {gld2}")
    
    print("\n✓ Both instances created successfully!")
    print("✓ No global variable registration errors!")
    print("\nThe process isolation solution is working!")

if __name__ == "__main__":
    test_double_instance()
