"""
Created on 02/10/2026

This example tests the ability of GLD to throw a warning when trying to 
`.step()` beyond the model-specified stop time
https://github.com/gridlab-d/gridlab-d/issues/1566


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

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

def parse_gld_time(time_str):
    parts = time_str.rsplit(' ', 1)
    if len(parts) == 2 and parts[1] in ['PST', 'PDT', 'EST', 'EDT', 'CST', 'CDT', 'MST', 'MDT']:
        time_str = parts[0]
    return datetime.strptime(time_str, '%Y-%m-%d %H:%M:%S')


gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model= gld.load("houses.glm")
gld.set_time_step(step_size)
gld.step()
checkpoint_dict = json.loads(gld.get_checkpoint_json())
start_time_etime = float(checkpoint_dict['clock']['starttime'])
stop_time_etime = float(checkpoint_dict['clock']['stoptime'])
start_time = datetime.fromtimestamp(start_time_etime)
stop_time = datetime.fromtimestamp(stop_time_etime)
sim_length = stop_time - start_time
num_steps = sim_length / timedelta(seconds = step_size)
# Already took one step so....
num_steps = num_steps - 1

for steps in range(int(num_steps)):
    gld.step()
    status, time = gld.get_time()
    print(f"GLD time: {time}\n")

final_time = time
print(f"********* Making an extra step after simulation should have been completed *******")

# What happens if you try to take another step?
gld.step()
status, extra_time = gld.get_time()
messages = gld.get_messages()
for message in messages:
    print(message)
print("One of the previous messages should be a warning or error for taking an extra step.")
gld.stop()
gld.exit_gld()

