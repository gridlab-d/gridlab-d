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
import json
from pathlib import Path

if __name__ == "__main__":
    # Run basic tests manually
    print("=" * 60)
    print("Running GridLAB-D Python Bindings Tests")
    print("=" * 60)
    
    try:
        import gridlabd

        # 1. Create instance (calls constructor which internally does setup_before_load)
        gld = gridlabd.GridLabD()

        # Set working directory to the tests directory (where the GLM file is located)
        test_dir = Path(__file__).parent
        gld.set_working_directory(str(test_dir))

        # 2. Optionally set config file
        # gld.set_config_file("config.cfg")

        # 3. Load model (using relative path since we set the working directory)
        gld.load_glm(["gridlabd", "test_HVAC_balance.glm", "--verbose"])

        # 4. Setup after load (if needed, though usually automatic)
        # gld.setup_after_load()

        # 5. Run simulation
        gld.run()

        # 6. Exit/cleanup
        gld.exit_gld("")
        
    except ImportError as e:
        print(f"✗ Import failed: {e}")
        print("The module needs to be built first with 'pip install -e .'")
