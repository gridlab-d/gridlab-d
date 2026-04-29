"""
Created on 03/24/2026

This example shows how to modify a GridLAB-D's model simulation start and 
stop time using the API. in this example, we extend the simulation duration by
one hour by starting the simulation slightly earlier and ending it slightly 
later.

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import json
import os
from pprint import pprint
from datetime import datetime, timedelta

step_size = 900

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

# Initilize GridLAB-D and load the model
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
load_code = gld.load("houses.glm")
if load_code != 0:
    raise RuntimeError(f"Failed to load model with error code {load_code}.")

starttime = gld.get_starttime()
stoptime = gld.get_stoptime()

house_names = gld.get_objects_by_class("house")
house_dict = gld.get_object_properties(house_names[0])
objs_list = gld.get_all_objects("house")
model = gld.get_model()
property = gld.get_property(house_names[0], "air_temperature")
properties = gld.get_properties_by_class("house", "air_temperature")


gld.run()
gld.stop()
gld.exit_gld()