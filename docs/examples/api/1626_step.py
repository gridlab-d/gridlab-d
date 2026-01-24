"""
Created on 01/23/2026

This example tests the ability for GLD to step exactly at the specified step
size.

https://github.com/gridlab-d/gridlab-d/issues/1626


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import json
from pprint import pprint
from datetime import datetime, timedelta

# Verify version
print(f"GLD Python library location: {gridlabd.__file__}")

# Make GLD instances
gld = gridlabd.GridLabD()

# Load model
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model = gld.load("houses.glm")

# Setting time steps
gld.set_time_step(900)

# Post(?)-initialize GLD
# gld1.setup_after_load()

# Getting simulation times 
status, time = gld.get_time()
print(f"*******  GLD time 1: {time} **********")

# Start simulations
gld.step()

# Getting simulation times 
status, time = gld.get_time()
print(f"*******  GLD time 2: {time} **********")
print(f"************ This should be one step size later than the first time. ************")

# Stop simulation
gld.stop()

# Exit simulation
gld.exit_gld()
 