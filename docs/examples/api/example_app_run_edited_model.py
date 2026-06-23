"""
Created on 06/10/2026

This code simply runs the moded that as edited by the GUI to confirm that the
output produced by said GUI is valid.

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import json
import os
from pprint import pprint
from datetime import datetime, timedelta

step_size = 900

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

# Initilize GridLAB-D™ and load the model
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
load_code = gld.load("houses.glm")
if load_code != 0:
    raise RuntimeError(f"Failed to load model with error code {load_code}.")

# Read in current start and stop time
starttime_dt = datetime.fromisoformat(gld.get_starttime())
stoptime_dt = datetime.fromisoformat(gld.get_stoptime())

# Calculate and set new start and stop times
starttime_str = datetime.isoformat(starttime_dt)
stoptime_str = datetime.isoformat(stoptime_dt + timedelta(hours=48))
gld.set_starttime(starttime_str)
gld.set_stoptime(stoptime_str)

# Confirm changes to start and stop times
starttime_dt = datetime.fromisoformat(gld.get_starttime())
stoptime_dt = datetime.fromisoformat(gld.get_stoptime())
print(f"New start time: {starttime_dt}")
print(f"New stop time: {stoptime_dt}")

status, sim_time = gld.get_time()
sim_time_dt = datetime.fromisoformat(sim_time)
while sim_time_dt < stoptime_dt:
    # Record whether this step is a big step before advancing
    was_big_step = (step_size == 300)
    # Set step size and advance one step
    gld.set_time_step(step_size)
    error_code, sim_time = gld.step()
    if error_code != 0:
        raise RuntimeError(f"Simulation step failed at {sim_time} with error code {error_code}.")
    sim_time_dt = datetime.fromisoformat(sim_time)

    # Check for errors
    messages = gld.get_messages()
    filtered_messages = [
        message for message in messages
        if message.get("type") in {"ERROR"}
    ]
  # Only print if there are error messages to print
    if filtered_messages:
        pprint(filtered_messages)
    gld.clear_messages()

    return_value = gld.get_all_objects("house")

gld.stop()
gld.exit_gld()