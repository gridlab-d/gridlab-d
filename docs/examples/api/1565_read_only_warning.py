"""
Created on 01/23/2026

This example tests GLD's ability to produce a warning message when users write
to a read-only object property (something that is overwritten by GLD).

https://github.com/gridlab-d/gridlab-d/issues/1565


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

step_size = 900
gld = gridlabd.GridLabD(verbose=True)
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model = gld.load("houses.glm")
gld.set_time_step(step_size)
gld.step()
print("*** Setting read-only property and looking for a warning.")
return_code = gld.set_property("house2_billing_meter", "measured_power", 999)
print("*** Warning message above?")
print(f"Attempted to set read-only property and got return code: {return_code}")
gld.stop()
gld.exit_gld()