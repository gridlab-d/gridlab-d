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

# Initilize GridLAB-D and load the model
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
load_code = gld.load("houses.glm")
if load_code != 0:
    raise RuntimeError(f"Failed to load model with error code {load_code}.")

# Read in current start and stop time
starttime = datetime.fromisoformat(gld.get_starttime())
stoptime = datetime.fromisoformat(gld.get_stoptime())
print(f"Start time in model: {starttime}")
print(f"Stop time in model: {stoptime}")

# Calculate new simulation duration
old_sim_duration = stoptime - starttime
new_sim_duration = old_sim_duration + timedelta(hours=1)
sim_duration_half = new_sim_duration / 2

# Calculate and set new start and stop times
calc_starttime = datetime.isoformat(starttime - sim_duration_half)
calc_stoptime = datetime.isoformat(stoptime + sim_duration_half)
gld.set_starttime(calc_starttime)
gld.set_stoptime(calc_stoptime)

# Confirm changes to start and stop times
new_starttime = datetime.fromisoformat(gld.get_starttime())
new_stoptime = datetime.fromisoformat(gld.get_stoptime())
print(f"New start time: {new_starttime}")
print(f"New stop time: {new_stoptime}")

# Run model with new simulation start and stop times set
gld.run()

# Alternatively call `run()` with start and stop time defined
# gld.run(start_time=calc_starttime, stop_time=calc_stoptime)

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