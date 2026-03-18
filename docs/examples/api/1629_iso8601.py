"""
Created on 01/23/2026

This example tests the ability to accept an ISO 8601 formated string
to specify the simulation time to step to.
https://github.com/gridlab-d/gridlab-d/issues/1629


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

gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model= gld.load("houses.glm")
status, time = gld.get_time()
print(f"Starting time:                  {time}")
time_obj = datetime.fromisoformat(time)
target_time = time_obj + timedelta(minutes = 23)
target_time_str = datetime.isoformat(target_time)
print(f"New target time stepping to:    {target_time_str}")
gld.step_to(target_time_str)
status, time = gld.get_time()
print(f"Post `step_to()` time:          {time}")
messages = gld.get_messages()
for message in messages:
    print(message)
gld.stop()
gld.exit_gld()
 