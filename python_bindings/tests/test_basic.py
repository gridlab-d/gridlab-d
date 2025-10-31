"""
Basic tests for GridLAB-D Python bindings

Tests based on test_gldapi.cpp functionality:
- GridLabD instance creation
- Loading GLM files with arguments
- Running simulations with time bounds
- Getting checkpoint JSON
- Exiting simulation properly

Note: Global variable registration errors are expected when creating
multiple GridLabD instances in the same process - this is a known
limitation of the current GridLAB-D architecture.
"""

import pytest
import sys
import os
import json
from pathlib import Path

# Add the src directory to the path for testing
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

if __name__ == "__main__":
    # Run basic tests manually
    print("=" * 60)
    print("Running GridLAB-D Python Bindings Tests")
    print("=" * 60)
    
    try:
        import gridlabd

        # 1. Create instance (calls constructor which internally does setup_before_load)
        gld = gridlabd.GridLabD()

        model_dir = Path("/mnt/c/dev/gridlab-d_fork/python_bindings/tests/test_HVAC_balance.glm").parent
        gld.set_working_directory(str(model_dir))

        # 2. Optionally set config file
        # gld.set_config_file("config.cfg")

        # 3. Load model
        gld.load_glm(["gridlabd", "./test_HVAC_balance.glm", "--verbose"])

        # 4. Setup after load (if needed, though usually automatic)
        # gld.setup_after_load()

        # 5. Run simulation
        gld.run()

        # 6. Exit/cleanup
        gld.exit_gld("")
        
    except ImportError as e:
        print(f"✗ Import failed: {e}")
        print("The module needs to be built first with 'pip install -e .'")
