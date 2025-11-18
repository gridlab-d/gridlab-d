"""
Show the checkpoint structure to understand object data
"""

import gridlabd
import json
from pathlib import Path

gld = gridlabd.GridLabD()
test_dir = Path(__file__).parent
gld.set_working_directory(str(test_dir))
config_path = test_dir / "gridlabd.conf"
if config_path.exists():
    gld.set_config_file(str(config_path))

gld.load_glm(["gridlabd", "./test_HVAC_balance.glm"])
gld.set_time_step(86400)

# Step once
gld.step()

# Get checkpoint
checkpoint_json = gld.get_checkpoint_json()
checkpoint = json.loads(checkpoint_json)

print("Checkpoint top-level keys:")
for key in checkpoint.keys():
    print(f"  {key}: {type(checkpoint[key])}")

print("\nFirst few entries:")
for i, (key, value) in enumerate(checkpoint.items()):
    if i >= 5:
        break
    if isinstance(value, dict):
        print(f"\n{key}: (dict with {len(value)} keys)")
        if len(value) <= 5:
            for k, v in value.items():
                print(f"  {k}: {v}")
    else:
        print(f"\n{key}: {value}")
