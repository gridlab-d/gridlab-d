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

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

# Instanate GridLAB-D and load model
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
load_code = gld.load("houses.glm")
load_code = gld.load("houses.glm")
if load_code != 0:
    raise RuntimeError(f"Failed to load model with error code {load_code}.")

# Getting initlial simulation time
status, first_time = gld.get_time()
print(f"Initial simulation time: {first_time}")

# Calculate a time 20 minutes after the initial time and steping to it
first_time_obj = datetime.fromisoformat(first_time)
test_time = first_time_obj + timedelta(minutes = 20)
test_time_str = datetime.isoformat(test_time)
error_code, return_time = gld.step_to(test_time_str)
print(f"Time after `.step_to()`: {return_time}")

# Setting time step to 5 minutes and stepping forward 3 steps
step_size = 300
gld.set_time_step(step_size)
for i in range(3):
    error_code, return_time = gld.step()
    print(f"Time after `.step()`: {return_time}")

# Edning the simulation and exiting GridLAB-D
gld.stop()
gld.exit_gld()