"""
Created on 01/23/2026

This example tests the ability to make checkpoint
https://github.com/gridlab-d/gridlab-d/issues/1581


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
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
gld.set_time_step(900)
gld.setup_after_load()
status, time = gld.get_time()
print(f"GLD time 1: {time}")
gld.step()
status, time = gld.get_time()
print(f"GLD time 2: {time}")
gld.save_checkpoint(model_path, "model_checkpoint1")
gld.step() 
status, time = gld.get_time()
print(f"GLD time 3: {time}")
gld.stop()
gld.exit_gld()

 