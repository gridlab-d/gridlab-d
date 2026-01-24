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

def parse_gld_time(time_str):
    parts = time_str.rsplit(' ', 1)
    if len(parts) == 2 and parts[1] in ['PST', 'PDT', 'EST', 'EDT', 'CST', 'CDT', 'MST', 'MDT', 'UTC']:
        time_str = parts[0]
    return datetime.strptime(time_str, '%Y-%m-%d %H:%M:%S'), parts[1]

gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model= gld.load("houses.glm")
status, time = gld.get_time()
print(f"*******  GLD time 1: {time} **********")
time, tz = parse_gld_time(time)
test_time = time + timedelta(minutes = 15)
test_time_str = test_time.strftime('%Y-%m-%d %H:%M:%S') + " " + tz
gld.step_to(test_time_str)
status, time = gld.get_time()
print(f"*******  GLD time 2: {time} **********")
gld.stop()
gld.exit_gld()
 