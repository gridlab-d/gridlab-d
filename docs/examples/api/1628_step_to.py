"""
Created on 01/23/2026

This example tests the ability to step to a specified simulation time
https://github.com/gridlab-d/gridlab-d/issues/1628


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import json
from pprint import pprint
from datetime import datetime, timedelta

gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model= gld.load("houses.glm")
status, first_time = gld.get_time()
print(f"Time before `.step_to()`: {first_time}")
first_time_obj = datetime.fromisoformat(first_time)
test_time = first_time_obj + timedelta(minutes = 20)
test_time_str = datetime.isoformat(test_time)
error_code, return_time = gld.step_to(test_time_str)
print(f"Time after `.step_to()`: {return_time}")
print(f"Target time : {test_time_str}")
print(f"Error code : {error_code}")
if test_time_str == return_time:
    print("Successfully stepped to correct time.")
else:
    print("Failed to step to correct time.")
gld.stop()
gld.exit_gld()
 