"""
Created on 02/10/2026

This example verifies that GLD returns the same timestamp format from the 
clock object and `.get_tiem()`

https://github.com/gridlab-d/gridlab-d/issues/1559


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import json


gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
loaded_model = gld.load("houses.glm")
gld.set_time_step(900)
gld.step()
checkpoint_dict = json.loads(gld.get_checkpoint_json())
status, time = gld.get_time()
print(f"Start time: {checkpoint_dict['clock']['starttime']}")
print(f"Stop time: {checkpoint_dict['clock']['starttime']}")
print(f"Sim time: {time}")
print("Above three timestamps should be string formatted as YYYY-MM-DDTHH:mm:ss+/-h:mm")
gld.stop()
gld.exit_gld()
 