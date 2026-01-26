"""
Created on 01/23/2026

This example demonstrates a failure for the GridLAB-D simulation to exit
cleanly.
https://github.com/gridlab-d/gridlab-d/issues/1630


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
    if len(parts) == 2 and parts[1] in ['PST', 'PDT', 'EST', 'EDT', 'CST', 'CDT', 'MST', 'MDT', 'UTC']:
        time_str = parts[0]
    return datetime.strptime(time_str, '%Y-%m-%d %H:%M:%S'), parts[1]

gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model= gld.load("houses.glm")
gld.set_time_step(900)
gld.step()
gld.stop()
gld.exit_gld()
 