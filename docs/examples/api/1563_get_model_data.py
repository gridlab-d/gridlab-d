"""
Created on 03/02/2026

This example tests the ability to get object properties from GridLAB-D 
both as simple data value as well as more complex metadata.

https://github.com/gridlab-d/gridlab-d/issues/1563


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
meter = gld.get_object_property_value("network_node", "measured_demand")
print(f"Meter `measured_demand` value: {meter}")
if isinstance(meter, float):
    print("Meter `measured_demand` is a float as expected.")
voltage = gld.get_object_property_value("network_node", "voltage_A")
print(f"Meter `voltage_A` value: {voltage}")
if isinstance(voltage, complex):
    print("Meter `voltage_A` is a complex number as expected.")
phases = gld.get_object_property_value("network_node", "phases")
print(f"Meter `phases` value: {phases}")
if isinstance(phases, str):
    print("Meter `phases` is a string as expected.")
# meter_details = gld.get_object_properties_detailed("network_node")
# print(f"Meter `measured_demand` metadata:")
# pprint(meter_details)
gld.stop()
gld.exit_gld()

 