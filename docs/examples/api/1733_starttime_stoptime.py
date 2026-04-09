"""
Created on 04/09/2026

This example tests that the values for the object parameters when calling
`get_starttime()` and `get_stoptime()` include timezone information.

https://github.com/gridlab-d/gridlab-d/issues/1733


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
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
load_status = gld.load("houses.glm")
start_str = gld.get_starttime()
stop_str = gld.get_stoptime()
print(f"Start time in model: {start_str}")
print(f"Stop time in model: {stop_str}")
if start_str.endswith('Z') or '+' in start_str or '-' in start_str:
    print("starttime include timezone information; we're good.")
else:
    raise RuntimeError("starttime does not include timezone information should.")
if stop_str.endswith('Z') or '+' in stop_str or '-' in stop_str:
    print("stoptime include timezone information; we're good.")
else:
    raise RuntimeError("stoptime does not include timezone information should.")
