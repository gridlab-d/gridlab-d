"""
Created on 01/23/2026

This example demonstrates a failure for the GridLAB-D simulation to exit
cleanly.
https://github.com/gridlab-d/gridlab-d/issues/1630


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
gld.step()
gld.stop()
gld.exit_gld()
 