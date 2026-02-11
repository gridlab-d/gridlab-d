"""
Created on 01/23/2026

This example tests the ability to accept an ISO 8601 formated string
to specify the simulation time to step to.
https://github.com/gridlab-d/gridlab-d/issues/1629


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
status, time = gld.get_time()
print(f"*******  GLD time 1: {time} **********")
time, tz = parse_gld_time(time)
test_time_str = time.isoformat()
gld.step_to(test_time_str)
status, time = gld.get_time()
print(f"*******  GLD time 2: {time} **********")
gld.stop()
gld.exit_gld()
 