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

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

step_size = 900

# Initilize GridLAB-D and load the model
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
load_code = gld.load("houses.glm")

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
calc_start_time = datetime.isoformat(starttime - sim_duration_half)
calc_stop_time = datetime.isoformat(stoptime + sim_duration_half)
gld.set_starttime(calc_start_time)
gld.set_stoptime(calc_stop_time)

# Confirm changes to start and stop times
new_starttime = datetime.fromisoformat(gld.get_starttime())
new_stoptime = datetime.fromisoformat(gld.get_stoptime())
print(f"New start time: {new_starttime} (should be one hour less than original)")
print(f"New stop time: {new_stoptime} (should be one hour less than original)")

# Set time step and run the model
gld.set_time_step(step_size)
gld.run()
gld.stop()
gld.exit_gld()