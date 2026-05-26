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
from pprint import pprint


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

starttime_dt = datetime.fromisoformat(gld.get_starttime())
stoptime_dt = datetime.fromisoformat(gld.get_stoptime())
sim_duration = stoptime_dt - starttime_dt
step_size = 900
num_steps = int(sim_duration.total_seconds() / step_size)

gld.set_time_step(step_size)
messages = gld.get_messages()
pprint(messages)
gld.clear_messages()
for step in range(num_steps):
    error_code, return_time_str = gld.step()
    print(f"Simulation time: {return_time_str}")
    messages = gld.get_messages()
    filtered_messages = [
        message for message in messages
        if message.get("type") in {"WARNING", "ERROR"}
    ]
    # TODO add additional filter that only looks at messages that have the
    # current simulation time in their timestamp.
    pprint(filtered_messages)
    gld.clear_messages()

# Ending the simulation and exiting GridLAB-D
# Taking one extra step beyond the stated simulation stop time
# gld.clear_messages()
# error_code, return_time_str = gld.step()
# messages = gld.get_messages()
# print('\n **************************** \n')
# pprint(messages)

gld.stop()
gld.exit_gld()