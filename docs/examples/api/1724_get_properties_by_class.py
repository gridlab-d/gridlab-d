"""
Created on 04/09/2026

This example tests the new name for the `get_properties_by_class()` method.

https://github.com/gridlab-d/gridlab-d/issues/1724


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""
import gridlabd
from pathlib import Path
import os

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model = gld.load("houses.glm")
floor_areas = gld.get_properties_by_class("house", "floor_area")
if isinstance(floor_areas["house1"], (int, float)):
    print("Floor area is a number; all is right with the world.")
else:
    raise RuntimeError("Floor area is not a number and should be.")
dummy = 0


