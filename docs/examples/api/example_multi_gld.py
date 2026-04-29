"""
Created on 01/23/2026

This example tests the ability to run multiple GLD instances in parallel
https://github.com/gridlab-d/gridlab-d/issues/1581


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import json
from pprint import pprint
from datetime import datetime, timedelta

def parse_gld_time(time_str):
    parts = time_str.rsplit(' ', 1)
    if len(parts) == 2 and parts[1] in ['PST', 'PDT', 'EST', 'EDT', 'CST', 'CDT', 'MST', 'MDT']:
        time_str = parts[0]
    return datetime.strptime(time_str, '%Y-%m-%d %H:%M:%S')


# Verify version
print(f"GLD Python library location: {gridlabd.__file__}")

# Make GLD instances
gld1 = gridlabd.GridLabD()
gld2 = gridlabd.GridLabD()
gld3 = gridlabd.GridLabD()

# Load models
model1_path = Path("house_with_solar")
api_example_home = model1_path.parent
gld1.set_working_directory(str(model1_path))
loaded_model1 = gld1.load("houses.glm")


model2_path = api_example_home / Path("house_with_solar2") 
gld2.set_working_directory(str(model2_path))
loaded_model2 = gld2.load("houses.glm")

model3_path = api_example_home / Path("house_with_solar3")
gld3.set_working_directory(str(model3_path))
loaded_model3 = gld3.load("houses.glm")

# Setting time steps
gld1.set_time_step(900)
gld2.set_time_step(900)
gld3.set_time_step(900)


# Post(?)-initialize GLD
gld1.setup_after_load()
gld2.setup_after_load()
gld3.setup_after_load()

# Getting simulation times 
status, time = gld1.get_time()
print(f"*******  GLD1 time: {time} **********")
status, time = gld2.get_time()
print(f"*******  GLD2 time: {time} **********")
status, time = gld3.get_time()
print(f"*******  GLD3 time: {time} **********")


# Start simulations
gld1.step()
gld2.step()
gld3.step()

# Getting simulation times 
status, time = gld1.get_time()
print(f"*******  GLD1 time: {time} **********")
status, time = gld2.get_time()
print(f"*******  GLD2 time: {time} **********")
status, time = gld3.get_time()
print(f"*******  GLD3 time: {time} **********")

gld1.save_checkpoint(model1_path, "model1_checkpoint1")
gld2.save_checkpoint(model2_path, "model2_checkpoint1")
gld3.save_checkpoint(model3_path, "model3_checkpoint1")

gld1.step()
gld2.step()
gld3.step()

# Stop simulation
gld1.stop()
gld2.stop()
gld3.stop()

# Exit simulation
gld1.exit_gld()
gld2.exit_gld()
gld3.exit_gld()
 
 