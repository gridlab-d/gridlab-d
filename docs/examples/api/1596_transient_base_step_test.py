"""
Created on 06/09/2026

Implemting a version of the base transient test that steps
through simulation time. Doing so will allow us to see if
and/or when the test hangs.

https://github.com/gridlab-d/gridlab-d/issues/1596



@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""
import gridlabd
from pathlib import Path
import os
from datetime import datetime, timedelta
import pprint as pp

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)


gld = gridlabd.GridLabD()
model_path = Path("transient")
gld.set_working_directory(str(model_path))
load_status = gld.load("test_TPIM_under_voltage_contactor.glm")
if load_status != 0:
    raise RuntimeError(f"Failed to load model with status code {load_status}")
starttime_dt = datetime.fromisoformat(gld.get_starttime())
stoptime_dt = datetime.fromisoformat(gld.get_stoptime())
print(f"Start time in model: {starttime_dt}")
print(f"Stop time in model: {stoptime_dt}")
time_status, current_time_str = gld.get_time()
if time_status != 0:
    raise RuntimeError(f"Simulation step failed at {current_time_str} with error code {time_status}.")
print(f"Initial simulation time: {current_time_str}")
current_time_dt = datetime.fromisoformat(current_time_str)

while current_time_dt < stoptime_dt:
    time_code, current_time_str = gld.step()
    print(f"Current time: {current_time_str}")
    if time_code != 0:
        raise RuntimeError(f"Simulation step failed at {current_time_str} with error code {time_code}.")
    current_time_dt = datetime.fromisoformat(current_time_str)
    messages = gld.get_messages()
    filtered_messages = [
        message for message in messages
        if message.get("type") in {"WARNING", "ERROR"}
    ]
    pp.pprint(filtered_messages)
gld.stop()
gld.exit_gld()

