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

step_size = 900
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model = gld.load("houses.glm")
gld.set_time_step(step_size)
status, first_time = gld.get_time()
first_time_obj = datetime.fromisoformat(first_time)
second_time_object = first_time_obj + timedelta(seconds = step_size)
print(f"GLD time 1 : {first_time}")
gld.step()
status, next_time = gld.get_time()
print(f"GLD time 2 : {next_time}")
print(f"Target time: {second_time_object}")
if next_time == datetime.isoformat(second_time_object):
    print("Successfully stepped to correct time")
else:
    print("Failed to step to correct time.")
gld.stop()
gld.exit_gld()
 