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
import pandapower as pp
import pandapower.networks as pn
import math
import matplotlib.pyplot as plt


def plot_coupled_signals(sim_time_points, voltage_magnitude, load_magnitude):
    """Plot coupled T&D values with dual y-axes over simulated time."""
    fig, ax_voltage = plt.subplots(figsize=(11, 5))
    ax_load = ax_voltage.twinx()

    ax_voltage.plot(
        sim_time_points,
        voltage_magnitude,
        color="tab:blue",
        linewidth=2,
        label="|Pandapower Voltage Applied to GridLAB-D|",
    )
    ax_load.plot(
        sim_time_points,
        load_magnitude,
        color="tab:red",
        linewidth=2,
        label="|GridLAB-D Load Applied to Pandapower|",
    )

    ax_voltage.set_xlabel("Simulated Time")
    ax_voltage.set_ylabel("Voltage Magnitude (V)", color="tab:blue")
    ax_load.set_ylabel("Load Magnitude (MVA)", color="tab:red")
    ax_voltage.tick_params(axis="y", labelcolor="tab:blue")
    ax_load.tick_params(axis="y", labelcolor="tab:red")
    ax_voltage.grid(True, alpha=0.3)

    lines_left, labels_left = ax_voltage.get_legend_handles_labels()
    lines_right, labels_right = ax_load.get_legend_handles_labels()
    ax_voltage.legend(lines_left + lines_right, labels_left + labels_right, loc="best")

    fig.tight_layout()
    plt.show()

# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

# User-defined parameters
step_size = 300

# Since pandapower isn't going to move through times (and just run the same
# powerflow at each time step, only getting a slightly updated load from 
# GridLAB-D each time), only the load on the GridLAB-D side should 
# appreciably change.
starttime = datetime(2026, 7, 1, 0, 0, 0)
stoptime = datetime(2026, 7, 2, 0, 0, 0)
pp_bus_coupling_index = 40
voltage_scaling_factor = 1.0
load_scaling_factor = 1.0



# Setting up GridLAB-D
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
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
net118 = pn.case118()
coupling_load_rows = net118.load.index[net118.load["bus"] == pp_bus_coupling_index]
if coupling_load_rows.empty:
    raise ValueError(f"No pandapower load rows found at bus index {pp_bus_coupling_index}")
coupling_load_row = coupling_load_rows[0]

# Setting up integrated T&D powerflow
sim_duration = stoptime - starttime
num_steps = int(sim_duration.total_seconds() / step_size)

# Calculating scaling factor after running each tool for one step
pp.run.runpp(net118)
gld.step()

# Checking to see if a generator is at the bus we're attaching the
# GridLAB-D load to. 
gen_at_bus = net118.gen[net118.gen.bus == pp_bus_coupling_index]
if gen_at_bus.empty:
    print(f"No pandapower generator found at bus index "
            f"{pp_bus_coupling_index}.")

pv_buses = net118.gen.bus.unique()
print(f"PV buses: {pv_buses}")
slack_buses = net118.ext_grid.bus.unique()
print(f"Slack buses: {slack_buses}")

# Voltage from pandapower will be applied to GridLAB-D, thus the scaling 
# factor is calculated as the ratio of GridLAB-D voltage to pandapower 
# voltage
gld_voltage_str = gld.get_object_property_value(
        "network_node", "positive_sequence_voltage")
gld_voltage = complex(gld_voltage_str.replace("i", "j"))
pp_voltage = net118.bus.at[pp_bus_coupling_index, "vn_kv"] * 1000 * math.sqrt(3)
voltage_scaling_factor = abs(gld_voltage) / abs(pp_voltage)

# Load from GridLAB-D will be applied to pandapower, thus the scaling 
# factor is calculated as the ratio of pandapower load to GridLAB-D load
gld_load = gld.get_object_property_value(
        "network_node", "distribution_load")
pp_load = complex(net118.load.at[coupling_load_row, "p_mw"],
    net118.load.at[coupling_load_row, "q_mvar"]) / 1000000
load_scaling_factor = abs(pp_load) / abs(gld_load)

# Store magnitudes for post-simulation visualization.
sim_time_points = []
applied_voltage_magnitude = []
applied_load_magnitude = []

for step in range(num_steps - 1):
    status, sim_time = gld.get_time()
    sim_time_obj = datetime.fromisoformat(sim_time)
    pp.run.runpp(net118, calculate_voltage_angles=True)
    pp_voltage = net118.bus.at[pp_bus_coupling_index, "vn_kv"] * 1000 * math.sqrt(3)
    gld_substation_voltage = float(abs(pp_voltage * voltage_scaling_factor))
    gld.set_property(
        "network_node", "positive_sequence_voltage", gld_substation_voltage)
    gld.step()
    complex_load = gld.get_object_property_value(
        "network_node", "distribution_load")
    t_load = complex_load * 1000000 * load_scaling_factor
    net118.load.at[coupling_load_row, "p_mw"] = t_load.real
    net118.load.at[coupling_load_row, "q_mvar"] = t_load.imag

    sim_time_points.append(sim_time_obj)
    applied_voltage_magnitude.append(abs(gld_substation_voltage))
    applied_load_magnitude.append(abs(t_load))

gld.stop()
gld.exit_gld()

plot_coupled_signals(sim_time_points, applied_voltage_magnitude, applied_load_magnitude)