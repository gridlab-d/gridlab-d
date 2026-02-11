"""
Created on 02/11/2026

This example tests the new access of the clock object
https://github.com/gridlab-d/gridlab-d/issues/1561


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
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model= gld.load("houses.glm")
starttime = datetime.fromisoformat(gld.get_starttime())
stoptime = datetime.fromisoformat(gld.get_stoptime())
print(f"Start time in model: {starttime}")
print(f"Stop time in model: {stoptime}")
old_sim_duration = stoptime - starttime
new_sim_duration = old_sim_duration + timedelta(hours=1)
sim_duration_half = new_sim_duration / 2
calc_start_time = datetime.isoformat(starttime - sim_duration_half)
calc_stop_time = datetime.isoformat(stoptime + sim_duration_half)
gld.set_starttime(calc_start_time)
gld.set_stoptime(calc_stop_time)
new_starttime = datetime.fromisoformat(gld.get_starttime())
new_stoptime = datetime.fromisoformat(gld.get_stoptime())
print(f"New start time: {new_starttime} (should be one hour less than original)")
print(f"New stop time: {new_stoptime} (should be one hour less than original)")
gld.set_time_step(step_size)
status, current_time = gld.get_time()
gld.stop()
gld.exit_gld()