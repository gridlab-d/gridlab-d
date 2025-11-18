"""
Check individual object update times during stepping
"""

import gridlabd
import json
from pathlib import Path

# Create instance
gld = gridlabd.GridLabD()

# Set working directory
test_dir = Path(__file__).parent
gld.set_working_directory(str(test_dir))

# Load config
config_path = test_dir / "gridlabd.conf"
if config_path.exists():
    gld.set_config_file(str(config_path))

# Load model
result = gld.load_glm(["gridlabd", "./test_HVAC_balance.glm"])
if result != gridlabd.GLDErrorCode.SUCCESS:
    print(f"Failed to load model: {result}")
    exit(1)

print("Model loaded successfully\n")

# Set time step to 1 day
gld.set_time_step(86400)
print("Set time step to 86400 seconds (1 day)\n")

# Get initial time
status, initial_time = gld.get_time()
print(f"Initial simulation time: {initial_time}\n")

# Step once
print("="*80)
print("Calling step()...")
print("="*80 + "\n")

status, sim_time = gld.step()

# Get checkpoint after first step
checkpoint_json = gld.get_checkpoint_json()
checkpoint = json.loads(checkpoint_json)

status, time_after = gld.get_time()
print(f"After step time: {time_after}")
print(f"Global clock timestamp: {checkpoint['clock']['timestamp']}")
print(f"Start time: {checkpoint['clock']['starttime']}")
print(f"Time advanced: {checkpoint['clock']['timestamp'] - checkpoint['clock']['starttime']} seconds")

print("\n" + "="*80)
print("Individual Object Sync Times (when each object needs its next update):")
print("="*80)

if 'objects' in checkpoint:
    current_clock = checkpoint['clock']['timestamp']
    
    print(f"\nCurrent simulation clock: {current_clock}")
    print(f"Objects requesting updates:\n")
    
    # Collect objects and their sync times
    obj_syncs = []
    for obj_name, obj_data in checkpoint['objects'].items():
        if isinstance(obj_data, dict) and 'sync' in obj_data:
            sync_time = obj_data['sync']
            time_until = sync_time - current_clock
            obj_syncs.append((obj_name, sync_time, time_until))
    
    # Sort by sync time
    obj_syncs.sort(key=lambda x: x[1])
    
    for obj_name, sync_time, time_until in obj_syncs:
        hours_until = time_until / 3600
        if time_until <= 0:
            status_str = "✓ Already updated"
        elif time_until < 86400:
            status_str = f"Next in {hours_until:.1f} hours"
        else:
            status_str = f"Next in {time_until/86400:.2f} days"
        
        print(f"  {obj_name:25s}  sync: {sync_time:12.0f}  {status_str}")

print("\n" + "="*80)
print("Key Insights:")
print("="*80)
print("✓ Each object has a 'sync' timestamp for its next required update")
print("✓ GridLAB-D advances to the earliest sync time among all objects")
print("✓ When set_time_step(86400) is used, multiple internal steps occur")
print("✓ Objects update at their own intervals (event-driven simulation)")
print("✓ Your fixed step() function loops until the full 86400 seconds elapse")
