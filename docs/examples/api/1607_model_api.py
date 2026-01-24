"""
Created on 12/30/2025

This example tests three new APIs developed in response to Github issues

`get_object_properties()` has no property `name` 
https://github.com/gridlab-d/gridlab-d/issues/1607


The code here uses a version of the API in a developer fork and at the time of
testing had not been merged back into the main GLD repository.
`git clone -b pypi_build https://github.com/riley206-pnnl/gridlab-d.git`

This repository was cloned into a Linux Ubuntu 24.10 and built with g++ 13. 
The Python library was then installed by going into the built "python_binding"
folder and `pip install -e .` This installed a library with the changes called
`gridlabd-test-riley`.


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

# Verify version
print(f"GLD Python library location: {gridlabd.__file__}")

model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))

loaded_model = gld.load_glm(["gridlabd", "./houses.glm"])
gld.set_time_step(900)

# Start simulation
gld.step()

status, current_time_str = gld.get_time()
current_time = parse_gld_time(current_time_str)

# Making a datetime object for the stoptime
stop_time = current_time + timedelta(days=1)


# Github issue #1583 API
# Returns list of dicts, one per object:
# [{"class": "house", "id": "1", ...}, {"class": "house", "id": "2", ...}]
house_list = gld.get_all_objects("house")
print(f"House count in model: {len(house_list)}")

# Github issue #1582 API
# {"class": "house", "id": "42", "floor_area": "2000", ...}
first_house_name = gld.get_object_properties(house_list[0]["__name__"])
print(f"House name of first house in model: {first_house_name}")

# Github issue #1584 API
# {"house": [{obj1}, {obj2}], "node": [{obj1}, {obj2}], ...}
model = gld.get_model()
for house in model["house"]:
    print(f"House names: {house['__name__']}")

        in