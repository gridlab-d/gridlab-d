"""
Created on 04/27/2026

This is used to test the .glm used in gld_ep.py test.

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
gld = gridlabd.GridLabD(verbose=True)
model_path = Path(script_dir)
gld.set_working_directory(str(model_path))
loaded_model = gld.load("houses_with_ep.glm")
gld.set_time_step(step_size)
gld.step()
gld.step()
gld.step()
gld.step()
gld.stop()
gld.exit_gld()