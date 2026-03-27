"""
Created on 03/27/2026

This example integrates pandapower and GridLAB-D to perform an integrated
transmission (bulk power system) and distribution powerflow.


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import os
from pprint import pprint
from datetime import datetime, timedelta
from pandapower.converter.matpower import from_mpc
import pandapower as pp
import math

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

# User-defined parameters
step_size = 1
starttime = datetime(2026, 7, 1, 0, 0, 0)
stoptime = datetime(2026, 7, 1, 0, 1, 0)
pp_coupling_index = 1
voltage_scaling_factor = 1.0
load_scaling_factor = 1.0



# Setting up GridLAB-D
gld = gridlabd.GridLabD()
model_path = os.path.join(os.path.dirname(script_dir), "house_with_solar")
response_working = gld.set_working_directory(str(model_path))
if response_working != 0:
    raise RuntimeError(f"Failed to change working directory to {model_path} with response code: {response_working}")
response_load = gld.load("houses.glm")
if response_load != 0:
    raise RuntimeError(f"Failed to load model in {model_path} with response code: {response_load}")
gld.set_starttime(starttime.isoformat())
gld.set_stoptime(stoptime.isoformat())
gld.set_time_step(step_size)

# Setting up pandapower
pp_net1 = from_mpc('case118.m', f_hz=60)

# Running integrated T&D powerflow
sim_duration = stoptime - starttime
num_steps = int(sim_duration.total_seconds() / step_size)

# Calculating scaling factor after running each tool for one step
pp.run.runpp(pp_net1)
gld.step()

# Voltage from pandapower will be applied to GridLAB-D, thus the scaling 
# factor is calculated as the ratio of GridLAB-D voltage to pandapower 
# voltage
gld_voltage = gld.get_object_property_value(
        "network_node", "positive_sequence_voltage")
pp_voltage = pp_net1.bus.at[pp_coupling_index, "vn_kv"] * 1000 * math.sqrt(3)
voltage_scaling_factor = gld_voltage / pp_voltage

# Load from GridLAB-D will be applied to pandapower, thus the scaling 
# factor is calculated
gld_load = gld.get_object_property_value(
        "network_node", "distribution_load")
pp_load = pp_net1.load.at[pp_coupling_index, "p_mw"] + \
    (1j * pp_net1.load.at[pp_coupling_index, "q_mvar"])
load_scaling_factor = abs(pp_load) / abs(gld_load)

for step in range(num_steps - 1):
    pp.run.runpp(pp_net1)
    pp_voltage = pp_net1.bus.at[pp_coupling_index, "vn_kv"] * 1000 * math.sqrt(3)
    gld_substation_voltage = abs(pp_voltage * voltage_scaling_factor)
    gld.set_object_property_value(
        "network_node", "positive_sequence_voltage", gld_substation_voltage)
    gld.step()
    complex_load = gld.get_object_property_value(
        "network_node", "distribution_load")
    t_load = complex_load * load_scaling_factor
    pp_net1.load.at[pp_coupling_index, "p_mw"] = t_load.real
    pp_net1.load.at[pp_coupling_index, "q_mvar"] = t_load.imag

gld.stop()
gld.exit_gld()