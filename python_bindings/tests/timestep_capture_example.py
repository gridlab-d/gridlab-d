"""
Example: Capture model state at each timestep during simulation

Demonstrates:
1. step() - Run one timestep at a time
2. get_property() - Query state after each step
3. Collect time-series data programmatically
"""

import gridlabd
from pathlib import Path

# Setup
gld = gridlabd.GridLabD()
test_dir = Path(__file__).parent
gld.set_working_directory(str(test_dir))

# Load model
model_path = test_dir / "test_HVAC_balance.glm"
result = gld.load(str(model_path))
if result != 0:
    print(f"Failed to load model: {result}")
    exit(1)

result = gld.setup_after_load()
if result != 0:
    print(f"Setup failed")
    exit(1)

# Get house to monitor
houses = gld.get_objects_by_class("house")
if not houses:
    print("No houses in model")
    exit(1)

house_name = houses[0]
print(f"Monitoring: {house_name}")
print(f"\n{'='*60}")
print(f"{'Timestep':>10} {'Time':>25} {'Temperature':>15} {'HVAC Power':>15}")
print(f"{'='*60}")

# Time-series data collection
timeseries_data = []
step_count = 0
max_steps = 20  # Limit steps for demonstration

# Step through simulation one timestep at a time
while step_count < max_steps:
    # Advance simulation by one timestep
    result, sim_time = gld.step()
    
    # Check if simulation is done
    if result == gridlabd.GLDErrorCode.SUCCESS:
        step_count += 1
        
        # Get current simulation time as string
        time_result, time_str = gld.get_time()
        
        # Query state at THIS timestep
        temp_result, temperature = gld.get_property(house_name, "air_temperature")
        power_result, hvac_power = gld.get_property(house_name, "hvac_power")
        
        # Display current state
        if temp_result == gridlabd.GLDErrorCode.SUCCESS:
            print(f"{step_count:>10} {time_str:>25} {temperature:>15} {hvac_power:>15}")
            
            # Save to time-series
            timeseries_data.append({
                'step': step_count,
                'time': time_str,
                'sim_time': sim_time,
                'temperature': temperature,
                'hvac_power': hvac_power
            })
    
    elif result == gridlabd.GLDErrorCode.SIMULATION_STOPPED:
        print(f"\nSimulation completed after {step_count} steps")
        break
    else:
        print(f"\nStep failed with error: {result}")
        break

print(f"{'='*60}")
print(f"\nCollected {len(timeseries_data)} timestep snapshots")

# Demonstrate data analysis
if timeseries_data:
    print(f"\nFirst timestep: {timeseries_data[0]['time']}")
    print(f"Last timestep:  {timeseries_data[-1]['time']}")
    
    # Could export to CSV, plot, etc.
    print(f"\nData ready for export/analysis")
