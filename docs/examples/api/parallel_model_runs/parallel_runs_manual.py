"""
Created on 03/19/2026

This example tests the ability of GridLAB-D™ to run models in parallel.
It is has been written to verify the failures seen in the LLM generated
script "parallel_model_runs.py" and help figure out why that one is
failing.


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


step_size = 300
starttime = datetime(2026, 7, 1, 0, 0, 0)
stoptime = datetime(2026, 7, 2, 0, 0, 0)

gld = []
path =["house_with_solar", "house_with_solar2", "house_with_solar3"]
for i in range(3):
    gld.append(gridlabd.GridLabD())
    model_path = os.path.join(os.path.dirname(script_dir), path[i])
    response_working = gld[i].set_working_directory(str(model_path))
    if response_working != 0:
        raise RuntimeError(f"Failed to change working directory to {model_path} with response code: {response_working}")
    response_load = gld[i].load("houses.glm")
    if response_load != 0:
        raise RuntimeError(f"Failed to load model in {model_path} with response code: {response_load}")
    gld[i].set_starttime(starttime.isoformat())
    gld[i].set_stoptime(stoptime.isoformat())
    gld[i].set_time_step(step_size)

sim_duration = stoptime - starttime
num_steps = int(sim_duration.total_seconds() / step_size)
for step in range(num_steps):
    for i in range(3):
        gld[i].step()
        status, current_time = gld[i].get_time()
        print(f"Model {i} - Step {step+1}/{num_steps} - Current time: {current_time}")


for i in range(3):
    gld[i].stop()
    gld[i].exit_gld()