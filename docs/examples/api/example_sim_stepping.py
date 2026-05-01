"""
Created on 03/24/2026

This example shows how to control GridLAB-D to advance through simulation 
time using various API calls. 

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
import os
from pathlib import Path
from datetime import datetime, timedelta
import pprint

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

# Instanate GridLAB-D and load model
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
load_code = gld.load("houses.glm")
if load_code != 0:
    raise RuntimeError(f"Failed to load model with error code {load_code}.")

# Setting up start and stop time

# Read in current start and stop time
starttime_dt = datetime.fromisoformat(gld.get_starttime())
stoptime_dt = datetime.fromisoformat(gld.get_stoptime())
print(f"Start time in model: {starttime_dt}")
print(f"Stop time in model: {stoptime_dt}")

# Calculate and set new start and stop times
stoptime_str = datetime.isoformat(starttime_dt + timedelta(minutes=30))
gld.set_stoptime(stoptime_str)

# Confirm changes to start and stop times
starttime_dt = datetime.fromisoformat(gld.get_starttime())
stoptime_dt = datetime.fromisoformat(gld.get_stoptime())
print(f"New start time: {starttime_dt}")
print(f"New stop time: {stoptime_dt}")


# Getting initlial simulation time
time_status, first_time_str = gld.get_time()
if time_status != 0:
    raise RuntimeError(f"Simulation step failed at {first_time_str} with error code {time_status}.")
print(f"Initial simulation time: {first_time_str}")

# Calculate a time 20 minutes after the initial time and stepping directly
# to it.
first_time_dt = datetime.fromisoformat(first_time_str)
step_to_time_dt = first_time_dt + timedelta(minutes = 20)
step_to_time_str = datetime.isoformat(step_to_time_dt)
time_code, return_time_str = gld.step_to(step_to_time_str)
if time_code != 0:
    raise RuntimeError(f"Simulation step failed at {return_time_str} with error code {time_code}.")
print(f"Time after `.step_to()` to 20 minutes later: {return_time_str}")

# Setting time step to 5 minutes and stepping forward to simulation
# stop time in 5 minute increments
step_size = 300
num_steps = 2
gld.set_time_step(step_size)

for i in range(num_steps):
    time_code, sim_time_str = gld.step()
    if time_code != 0:
        raise RuntimeError(f"Simulation step failed at {sim_time_str} with error code {time_code}.")
    sim_time_dt = datetime.fromisoformat(sim_time_str)
    print(f"Time after `.step()`: {sim_time_str}")

    # Check for errors
    messages = gld.get_messages()
    filtered_messages = [
        message for message in messages
        if message.get("type") in {"ERROR"}
    ]
    if filtered_messages:
        pprint(filtered_messages)
    gld.clear_messages()

# Test to see what happens when you try stepping beyond end of simulation
print("**** Taking one more step beyond end of simulation time ****")
time_code, sim_time_str = gld.step()
if time_code != 0:
    raise RuntimeError(f"Simulation step failed at {sim_time_str} with error code {time_code}.")
messages = gld.get_messages()

# Ending the simulation and exiting GridLAB-D
gld.stop()
gld.exit_gld()