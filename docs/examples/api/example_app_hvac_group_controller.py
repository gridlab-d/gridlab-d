"""
Created on 04/27/2026

This example shows how to integrate a custom controller with GridLAB-D™.

For this example, a frivous controller is implemented that regulates the 
number of HVAC units that are on at any given time. The controller has a 
target of a user-specified number of HVACs and adjusts the thermostat
setpoint to reach this target value.

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
import os
from pathlib import Path
from datetime import datetime, timedelta
import pprint
import random
import matplotlib.pyplot as plt

target_hvac_on = 7
sim_step_size = 300


def run_controller(gld, target_hvac_on):
    """
    A frivolous controller that regulates the number of HVAC units that are on
    at any given time. The controller has a target of a user-specified number
    of HVACs and adjusts the thermostat setpoint to reach this target value.

    Parameters
    ----------
    gld : gridlabd.GridLabD
        An instance of the GridLabD class with a loaded model.
    target_hvac_on : int
        The target number of HVAC units that should be on at any given time.

    Returns
    -------
    None.
    """
    house_list = gld.get_objects_by_class("house")
    num_hvac_on = 0
    hvac_on_house_list = []
    hvac_off_house_list = []
    for idx, house in enumerate(house_list):
        house_properties = gld.get_object_properties(object_name=house)
        if house_properties["hvac_load"] > 0.1:
            num_hvac_on += 1
            hvac_on_house_list.append(idx)
        else:
            hvac_off_house_list.append(idx)
    print(f"Number of HVACs on: {num_hvac_on}; target HVACs on: {target_hvac_on}")
    hvac_count_to_change = target_hvac_on - num_hvac_on
    if hvac_count_to_change > 0:
        hvac_change_list = hvac_off_house_list
    else:
        hvac_change_list = hvac_on_house_list
    hvacs_to_change = random.sample(hvac_change_list, abs(hvac_count_to_change))
    for house_idx in hvacs_to_change:
        gld.set_property(house_list[house_idx], "system_mode", "4") # 1=OFF, 2=HEAT, 4=COOL
        get_result, cooling_setpoint = gld.get_property(house_list[house_idx], 'cooling_setpoint')
        if get_result != 0:
            print(f"Failed to get cooling setpoint for {house_list[house_idx]} with error code {get_result}.")
        print(f"{house_list[house_idx]} old cooling setpoint:  {cooling_setpoint}")
        
        get_result, system_mode = gld.get_property(house_list[house_idx], 'system_mode')
        if get_result != 0:
            print(f"Failed to get system mode for {house_list[house_idx]} with error code {get_result}.")
        print(f"{house_list[house_idx]} HVAC mode: {system_mode}")
        
        get_result, cooling_system_type = gld.get_property(house_list[house_idx], 'cooling_system_type')
        if get_result != 0:
            print(f"Failed to get cooling system type for {house_list[house_idx]} with error code {get_result}.")
        print(f"{house_list[house_idx]} HVAC type: {cooling_system_type}")
       
        if hvac_count_to_change > 0:
            new_setpoint = "55"
        else:
            new_setpoint = "90"
        print(f"{house_list[house_idx]} new target setpoint: {new_setpoint}")
        set_result = gld.set_property(house_list[house_idx], "cooling_setpoint", new_setpoint)
        if set_result != 0:
            print(f"Failed to set cooling setpoint for {house_list[house_idx]} with error code {set_result}.")  
        get_result, cooling_setpoint = gld.get_property(house_list[house_idx], 'cooling_setpoint')
        
        if get_result != 0:
            print(f"Failed to get cooling setpoint for {house_list[house_idx]} with error code {get_result}.")
        print(f"{house_list[house_idx]} new cooling setpoint:  {cooling_setpoint}")
        dummy = 0
    return num_hvac_on


def plot_hvac_on_data(hvac_on_data):
    """Plot number of HVAC units on versus simulation time."""
    if not hvac_on_data:
        print("No HVAC data to plot.")
        return

    timestamps = [datetime.fromisoformat(t) for t, _ in hvac_on_data]
    hvac_on_values = [v for _, v in hvac_on_data]

    plt.figure(figsize=(12, 5))
    plt.plot(timestamps, hvac_on_values, marker="o", linewidth=1.5)
    plt.xlabel("Simulation Time")
    plt.ylabel("HVAC Units On")
    plt.title("HVAC Units On Over Time")
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.show()



# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

# Instanate GridLAB-D™ and load model
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
load_code = gld.load("houses.glm")
if load_code != 0:
    raise RuntimeError(f"Failed to load model with error code {load_code}.")

# Setting up start and stop time
# Read in current start and stop time
starttime = datetime.fromisoformat(gld.get_starttime())
stoptime = datetime.fromisoformat(gld.get_stoptime())

# Calculate and set new start and stop times
calc_stoptime = datetime.isoformat(starttime + timedelta(days=1))
gld.set_stoptime(calc_stoptime)
new_stoptime = datetime.fromisoformat(gld.get_stoptime())
print(f"New stop time: {new_stoptime}")


# Getting initlial simulation time
time_status, first_time = gld.get_time()
if time_status != 0:
    raise RuntimeError(f"Simulation step failed at {first_time} with error code {time_status}.")
print(f"Initial simulation time: {first_time}")

# Run simulation
status, sim_time = gld.get_time()
sim_time_obj = datetime.fromisoformat(sim_time)
hvac_on_data = []
gld.set_time_step(sim_step_size)
while sim_time_obj < new_stoptime:
    # Step the simulation forward
    status, sim_time = gld.step()
    if status != 0:
        raise RuntimeError(f"Simulation step failed at {sim_time} with error code {status}.")
    hvac_on = run_controller(gld, target_hvac_on)
    sim_time_obj = datetime.fromisoformat(sim_time)
    print(f"Current simulation time: {sim_time}")
    hvac_on_data.append((sim_time, hvac_on))

# Ending the simulation and exiting GridLAB-D
gld.stop()
gld.exit_gld()
plot_hvac_on_data(hvac_on_data)

