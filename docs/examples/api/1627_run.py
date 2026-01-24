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
from pprint import pprint
from datetime import datetime, timedelta

def parse_gld_time(time_str):
    parts = time_str.rsplit(' ', 1)
    if len(parts) == 2 and parts[1] in ['PST', 'PDT', 'EST', 'EDT', 'CST', 'CDT', 'MST', 'MDT']:
        time_str = parts[0]
    return datetime.strptime(time_str, '%Y-%m-%d %H:%M:%S')


# Verify version
print(f"GLD Python library location: {gridlabd.__file__}")

# Make GLD instances
gld = gridlabd.GridLabD()

# Load models
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model= gld.load("houses.glm")

gld.run()
 