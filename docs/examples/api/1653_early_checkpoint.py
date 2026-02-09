"""
Created on 02/09/2026

This example tests the ability of GLD to make a checkpoint before initiating a
GridLAB-D step.
https://github.com/gridlab-d/gridlab-d/issues/1653


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import json
import os
from pprint import pprint
from datetime import datetime, timedelta

# User parameter that doesn't matter too much for this test.
step_size = 900

def parse_gld_time(time_str):
    parts = time_str.rsplit(' ', 1)
    if len(parts) == 2 and parts[1] in ['PST', 'PDT', 'EST', 'EDT', 'CST', 'CDT', 'MST', 'MDT']:
        time_str = parts[0]
    return datetime.strptime(time_str, '%Y-%m-%d %H:%M:%S')


# Verify version
print(f"GLD Python library location: {gridlabd.__file__}")

# Make GLD instances
gld = gridlabd.GridLabD()

# Load models
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model= gld.load("houses.glm")

# Setting time steps for 15 minutes
gld.set_time_step(step_size)

# Post(?)-initialize GLD
gld.setup_after_load()

# On v1.0.9 this produces a fatal runtime error.
checkpoint_dict = gld.get_checkpoint_json()

# Stop simulation
gld.stop()

# Exit simulation
gld.exit_gld()

 