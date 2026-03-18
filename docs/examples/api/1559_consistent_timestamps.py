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
starttime = datetime.fromisoformat(gld.get_starttime())
stoptime = datetime.fromisoformat(gld.get_stoptime())
gld.step()
status, time = gld.get_time()
print(f"Start time: {starttime}")
print(f"Stop time: {stoptime}")
print(f"Sim time: {time}")
print("Above three timestamps should be string formatted as YYYY-MM-DDTHH:mm:ss+/-h:mm")
gld.stop()
gld.exit_gld()
 