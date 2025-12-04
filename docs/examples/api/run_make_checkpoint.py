"""
Created on 11/19/2025

This example shows the simplest way of running a GridLAB-D model, like users
would do without the API.

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
    if len(parts) == 2 and parts[1] in ['PST', 'PDT', 'EST', 'EDT', 'CST', 'CDT', 'MST', 'MDT']:
        time_str = parts[0]
    return datetime.strptime(time_str, '%Y-%m-%d %H:%M:%S')

gld = gridlabd.GridLabD()
model_path = Path("/home/hard312/gld_api/house_with_solar")
gld.set_working_directory(str(model_path))

gld.load_glm(["gridlabd", "./houses.glm"])
gld.set_time_step(900)

# Start simulation
gld.step()

status, current_time_str = gld.get_time()
current_time = parse_gld_time(current_time_str)
# print(f"Current sim time: {current_time}")
# print(f"Current status: {status}")

checkpoint_json = gld.get_checkpoint_json()
checkpoint_dict = json.loads(checkpoint_json)

print(f"Checkpoint keys: {list(checkpoint_dict.keys())}")
print(f"Total object count: {len(checkpoint_dict)}")
print(f"Checkpoint object names: {list(checkpoint_dict['objects'])}")

# Making a datetime object for the stoptime
stop_time_etime = float(checkpoint_dict['clock']['stoptime'])
stop_time = datetime.fromtimestamp(stop_time_etime)

# Make object dictionaries
obj_dict = {}
object_types = list(checkpoint_dict['objects'])
for obj_type in object_types:
    obj_dict[obj_type] = {}
    for obj in checkpoint_dict['objects'][obj_type]['instances']:
        obj_dict[obj_type][obj['name']] = obj 

print(f"House object names: {obj_dict['house'].keys()}")
print(f"Inveter object names: {obj_dict['inverter'].keys()}")
print(f"Meter object names: {obj_dict['meter'].keys()}")
print(f"Solar object names: {obj_dict['solar'].keys()}")

# Finishing the simulation to completion
while current_time < stop_time + timedelta(hours=1):
    gld.step()
    status, current_time_str = gld.get_time()
    current_time = parse_gld_time(current_time_str)
    # print(f"Current sim time: {current_time}")