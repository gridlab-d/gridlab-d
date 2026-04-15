"""
Created on 02/10/2026

This example verifies that GLD returns the same timestamp format from the 
clock object and `.get_time()`

https://github.com/gridlab-d/gridlab-d/issues/1559


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import json
import os
from datetime import datetime, timedelta

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model = gld.load("houses.glm")
gld.set_time_step(900)
start_str = gld.get_starttime()
stop_str = gld.get_stoptime()
starttime = datetime.fromisoformat(gld.get_starttime())
stoptime = datetime.fromisoformat(gld.get_stoptime())
gld.step()
status, time = gld.get_time()
print(f"Start time (return from `get_starttime()`): {start_str}")
print(f"Start time (converted to datetime):         {starttime}\n")
print(f"Stop time (return from `get_stoptime()`):   {stop_str}")
print(f"Stop time (converted to datetime):          {stoptime}\n")
print(f"Sim time (return from `get_time()`):        {time}")
gld.stop()
gld.exit_gld()
 