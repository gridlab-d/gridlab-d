"""
Created on 01/23/2026

This example tests GLD's ability send error messages to console even when
the user is in non-verbose mode for the GLD object

https://github.com/gridlab-d/gridlab-d/issues/1673


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

step_size = 900
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
print("*** Next command should produce an error message for failling to load the model.")
print("*** Not an error message about not being able to step due to no objects in model")
loaded_model = gld.load("houses999.glm")
gld.set_time_step(step_size)
gld.step()
gld.stop()
gld.exit_gld()