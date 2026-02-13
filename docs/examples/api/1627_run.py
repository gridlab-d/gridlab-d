"""
Created on 01/23/2026

This example tests the ability to run a model.
https://github.com/gridlab-d/gridlab-d/issues/1627


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import json
import os
from pprint import pprint
from datetime import datetime, timedelta

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)
gld = gridlabd.GridLabD(verbose = True)
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model= gld.load("houses.glm")
gld.run()
gld.stop()
gld.exit_gld()