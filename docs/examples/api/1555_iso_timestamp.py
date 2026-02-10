"""
Created on 01/23/2026

This example tests the ability for GLD return an ISO 8061 timestamp from
`.get_time()`

https://github.com/gridlab-d/gridlab-d/issues/1555


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import json
from pprint import pprint
from datetime import datetime, timedelta

# Verify version
print(f"GLD Python library location: {gridlabd.__file__}")

# Make GLD instances
gld = gridlabd.GridLabD()

# Load model
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model = gld.load("houses.glm")

# Setting time steps
gld.set_time_step(900)

# Getting simulation times 
status, time = gld.get_time()
print(f"*******  GLD time 1: {time} **********")
print("Should be string formatted as YYYY-MM-DDTHH:mm:ss+/-h:mm")


# Stop simulation
gld.stop()

# Exit simulation
gld.exit_gld()
 