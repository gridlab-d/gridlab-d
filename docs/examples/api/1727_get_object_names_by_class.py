"""
Created on 04/09/2026

This example tests the new name for the get_objects_by_class method.

https://github.com/gridlab-d/gridlab-d/issues/1727


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
names = gld.get_object_names_by_class("house")
dummy = 0

