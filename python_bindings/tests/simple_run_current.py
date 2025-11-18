"""
Simple run example using CURRENT python_bindings implementation

This recreates the simple_run.py example from docs/examples/api
but using the actual current API.
"""

import gridlabd
from pathlib import Path

# Create instance (equivalent to GLD())
gld = gridlabd.GridLabD()

# Set working directory to the tests folder
test_dir = Path(__file__).parent
gld.set_working_directory(str(test_dir))

# Set config file if needed
config_path = test_dir / "gridlabd.conf"
if config_path.exists():
    gld.set_config_file(str(config_path))

# Load model (combines open_model_file + load_model in one step)
# Note: We don't have the two-step process, so we load directly
result = gld.load_glm(["gridlabd", "./test_HVAC_balance.glm"])

if result != gridlabd.GLDErrorCode.SUCCESS:
    print(f"Failed to load model: {result}")
    exit(1)

print("Model loaded successfully")

# Run the model (equivalent to sim_run())
print("Running simulation...")
result = gld.run()

if result != gridlabd.GLDErrorCode.SUCCESS:
    print(f"Simulation failed: {result}")
    exit(1)

print("✓ Simulation completed successfully!")

# Note: exit_gld() can cause crashes when called from Python
# Let Python's cleanup handle it instead
gld.exit_gld("")
