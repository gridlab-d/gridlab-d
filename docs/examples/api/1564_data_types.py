"""
Created on 02/11/2026

This example tests the return data types when getting parameter values from
GridLAB-D™ via the APIs.

https://github.com/gridlab-d/gridlab-d/issues/1564


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import os
from pprint import pprint
from datetime import datetime, timedelta

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model= gld.load("houses.glm")
gld.set_time_step(900)
gld.step()
meter = gld.get_object_properties("network_node")
measured_demand_class = print(type(meter["measured_demand"]))
print(f"Meter `measured_demand` should be a float: {measured_demand_class}")
gld.stop()
gld.exit_gld()

 