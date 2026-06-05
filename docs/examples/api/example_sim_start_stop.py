"""
Created on 03/24/2026

This example shows how to modify a GridLAB-D's model simulation start and 
stop time using the API. in this example, we extend the simulation duration by
one hour by starting the simulation slightly earlier and ending it slightly 
later.

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
print(f"Start time in model: {starttime_dt}")
print(f"Stop time in model: {stoptime_dt}")

# Calculate new simulation duration
old_sim_duration = stoptime_dt - starttime_dt
new_sim_duration = old_sim_duration + timedelta(hours=1)
sim_duration_half = new_sim_duration / 2

# Calculate and set new start and stop times
starttime_str = datetime.isoformat(starttime_dt - sim_duration_half)
stoptime_str = datetime.isoformat(stoptime_dt + sim_duration_half)
gld.set_starttime(starttime_str)
gld.set_stoptime(stoptime_str)

# Confirm changes to start and stop times
starttime_dt = datetime.fromisoformat(gld.get_starttime())
stoptime_dt = datetime.fromisoformat(gld.get_stoptime())
print(f"New start time: {starttime_dt}")
print(f"New stop time: {stoptime_dt}")

# Run model with new simulation start and stop times set
gld.run()

# Alternatively call `run()` with start and stop time defined
# gld.run(start_time=starttime_str, stop_time=stoptime_str)

# Check for errors after running simulation
messages = gld.get_messages()
filtered_messages = [
    message for message in messages
    if message.get("type") in {"ERROR"}
]
# Only print if there are error messages to print
if filtered_messages:
    pprint(filtered_messages)
gld.clear_messages()

gld.stop()
gld.exit_gld()