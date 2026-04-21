"""
Created on 04/15/2026

On this special day in the USA (federal income taxes due), I've put together
this example test to validate the basic functionality of using the Python API
to run a transient analysis.

https://github.com/gridlab-d/gridlab-d/issues/1596



@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""
import gridlabd
from pathlib import Path
import os
from datetime import datetime, timedelta

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)


gld = gridlabd.GridLabD()
model_path = Path("transient")
gld.set_working_directory(str(model_path))
load_status = gld.load("test_deltamode_capacitor_VAR_VOLT_ABC_indiv_dwell_NR.glm")
if load_status != 0:
    raise RuntimeError(f"Failed to load model with status code {load_status}")
start_str = gld.get_starttime()
stop_str = gld.get_stoptime()
print(f"Start time in model: {start_str}")
print(f"Stop time in model: {stop_str}")
gld.run()
gld.stop()
gld.exit_gld()

