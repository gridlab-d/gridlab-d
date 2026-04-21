"""
Created on 03/24/2026

This example shows how to read data out of GridLAB-D while the simulation is
running and writing this to an HDF5 file on disk. The data of interest is the
indoor air temperature for each house.

To demonstrate the ability to record data in a more nuanced way, the example 
starts out stepping through time with a 60 second step size, recording data 
at every time step. If the HVAC systems for all houses reach a point where 
they are off for more than 30 minutes, the step size changes to 300 seconds
(five minutes). If any of the HVAC systems turn back on, the step size changes
back to 60 seconds.To effectively demonstrate this variable step size, the 
analysis is run for two days.

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import os
from datetime import datetime, timedelta, timezone
import h5py
import numpy as np
import re
import matplotlib.pyplot as plt
import pprint

step_size = 60

def plot_air_temperature(hdf5_filename="output_data.h5"):
    """
    Read air temperature data from HDF5 file and plot it.
    
    Args:
        hdf5_filename (str): Path to the HDF5 file containing temperature data
    """
    try:
        with h5py.File(hdf5_filename, "r") as hdf5_file:
            if "air_temperature" not in hdf5_file:
                print(f"No air_temperature group found in {hdf5_filename}")
                return
            
            air_temp_group = hdf5_file["air_temperature"]
            plt.figure(figsize=(12, 6))
            
            for house_name in air_temp_group.keys():
                temps = air_temp_group[house_name][:]
                plt.plot(temps, label=house_name, alpha=0.7)
            
            plt.xlabel("Time Step")
            plt.ylabel("Air Temperature (°F)")
            plt.title("House Air Temperature Over Time")
            plt.legend(loc="best")
            plt.grid(True, alpha=0.3)
            plt.tight_layout()
            plt.show()
    except FileNotFoundError:
        print(f"HDF5 file '{hdf5_filename}' not found.")
    except Exception as e:
        print(f"Error plotting air temperature data: {e}")


def parse_temperature_data(air_dict):
    """
    Convert temperature values from strings with units to floats.
    
    Args:
        air_dict (dict): Dictionary with house names as keys and temperature 
                        values as strings with units (e.g., "72.5 degF")
    
    Returns:
        dict: Dictionary with house names as keys and temperature values as floats
    """
    parsed_data = {}
    for house, temp_str in air_dict.items():
        # Extract the numeric part by splitting on whitespace and taking the first element
        numeric_str = str(temp_str).split()[0]
        try:
            parsed_data[house] = float(numeric_str)
        except ValueError:
            # If conversion fails, try using regex to extract all numeric characters
            numeric_match = re.search(r'-?\d+\.?\d*', str(temp_str))
            if numeric_match:
                parsed_data[house] = float(numeric_match.group())
            else:
                print(f"Warning: Could not parse temperature for {house}: {temp_str}")
                parsed_data[house] = None
    return parsed_data

def parse_hvac_off_data(hvac_off_dict, hvac_load_dict, starttime, sim_time_obj):
    """
    Convert HVAC off time strings to datetime objects.
    
    Args:
        hvac_off_dict (dict): Dictionary with house names as keys and timestamp
                 strings as values formatted as "2023-07-01 00:22:29 PDT"
                 or "INIT"
        hvac_load_dict (dict): Dictionary with house names as keys and HVAC
                 load values (float or parseable numeric strings)
        starttime (datetime): Simulation start time used when value is "INIT"
        sim_time_obj (datetime): Current simulation time used when HVAC load
                 is non-zero
    
    Returns:
        dict: Dictionary with house names as keys and datetime objects as values
    """
    # Mapping of timezone abbreviations to UTC offsets
    timezone_map = {
        'PST': timezone(timedelta(hours=-8)),
        'PDT': timezone(timedelta(hours=-7)),
        'MST': timezone(timedelta(hours=-7)),
        'MDT': timezone(timedelta(hours=-6)),
        'CST': timezone(timedelta(hours=-6)),
        'CDT': timezone(timedelta(hours=-5)),
        'EST': timezone(timedelta(hours=-5)),
        'EDT': timezone(timedelta(hours=-4)),
    }
    
    parsed_data = {}
    for house, timestamp_str in hvac_off_dict.items():
        try:
            load_value = hvac_load_dict.get(house)
            if load_value is not None and float(load_value) != 0.0:
                parsed_data[house] = sim_time_obj
                continue

            if str(timestamp_str).strip() == "INIT":
                parsed_data[house] = starttime
                continue

            # Split the timestamp string to extract the datetime and timezone parts
            parts = str(timestamp_str).split()
            if len(parts) >= 3:
                date_part = parts[0]  # e.g., "2023-07-01"
                time_part = parts[1]  # e.g., "00:22:29"
                tz_part = parts[2]    # e.g., "PDT"
                
                # Parse the datetime part
                dt = datetime.fromisoformat(f"{date_part}T{time_part}")
                
                # Apply timezone if found in mapping
                if tz_part in timezone_map:
                    dt = dt.replace(tzinfo=timezone_map[tz_part])
                
                # Store as datetime object
                parsed_data[house] = dt
            else:
                print(f"Warning: Could not parse HVAC off time for {house}: {timestamp_str}")
                parsed_data[house] = None
        except ValueError as e:
            print(f"Warning: Error parsing HVAC off time for {house}: {timestamp_str} - {e}")
            parsed_data[house] = None
    
    return parsed_data


def parse_hvac_load_data(hvac_load_dict):
    """
    Convert HVAC load values from strings with kW units to floats.

    Args:
        hvac_load_dict (dict): Dictionary with house names as keys and load
                              values as strings with units (e.g., "3.25 kW")

    Returns:
        dict: Dictionary with house names as keys and load values as floats
    """
    parsed_data = {}
    for house, load_str in hvac_load_dict.items():
        numeric_str = str(load_str).replace("kW", "").strip()
        try:
            parsed_data[house] = float(numeric_str)
        except ValueError:
            numeric_match = re.search(r'-?\d+\.?\d*', str(load_str))
            if numeric_match:
                parsed_data[house] = float(numeric_match.group())
            else:
                print(f"Warning: Could not parse HVAC load for {house}: {load_str}")
                parsed_data[house] = None
    return parsed_data


def check_hvac_off_elapsed(hvac_off_dict, sim_time_obj, threshold_seconds=300):
    """
    Build a boolean dictionary indicating whether each HVAC off timestamp
    is older than the threshold relative to current simulation time.

    Args:
        hvac_off_dict (dict): Dictionary with house names as keys and datetime
                              objects as values
        sim_time_obj (datetime): Current simulation time
        threshold_seconds (int): Threshold in seconds (default: 300)

    Returns:
        dict: Dictionary with same keys as hvac_off_dict and boolean flags
    """
    elapsed_flags = {}
    threshold = timedelta(seconds=threshold_seconds)

    for house, off_time in hvac_off_dict.items():
        if not isinstance(off_time, datetime):
            elapsed_flags[house] = False
            continue

        # Align tz-awareness before subtraction to avoid TypeError.
        current_time = sim_time_obj
        if off_time.tzinfo is not None and current_time.tzinfo is None:
            current_time = current_time.replace(tzinfo=off_time.tzinfo)
        elif off_time.tzinfo is None and current_time.tzinfo is not None:
            off_time = off_time.replace(tzinfo=current_time.tzinfo)

        elapsed_flags[house] = (current_time - off_time) > threshold

    return elapsed_flags


# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

# Initilize GridLAB-D and load the model
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
load_code = gld.load("houses.glm")
if load_code != 0:
    raise RuntimeError(f"Failed to load model with error code {load_code}.")

# Read in current start and stop time
starttime = datetime.fromisoformat(gld.get_starttime())
stoptime = datetime.fromisoformat(gld.get_stoptime())
# TODO: remove the next two lines once  Github #1733 is resolved
starttime = starttime.replace(tzinfo=timezone(timedelta(hours=-7)))
stoptime = stoptime.replace(tzinfo=timezone(timedelta(hours=-7)))

# Calculate new simulation duration, set stop time, and confirm changes
stop_time_obj = starttime + timedelta(days=2)
stop_time_str = datetime.isoformat(stop_time_obj)
gld.set_stoptime(stop_time_str)
new_stoptime = datetime.fromisoformat(gld.get_stoptime())


# Getting list of house names
house_dict = gld.get_properties_by_class(class_name="house", property_name="name")

# Open HDF5 and CSV files for writing
hdf5_file = h5py.File("output_data.h5", "w")

big_step_count = 0
small_step_count = 0

# Run the model and collect data at each time step, adjusting step size when
# appropriate
status, sim_time = gld.get_time()
sim_time_obj = datetime.fromisoformat(sim_time)
while sim_time_obj < stop_time_obj:
    # Set step size and advance one step
    gld.set_time_step(step_size)
    error_code, sim_time = gld.step()
    if error_code != 0:
        raise RuntimeError(f"Simulation step failed at {sim_time} with error code {error_code}.")
    sim_time_obj = datetime.fromisoformat(sim_time)

    # Check for errors
    messages = gld.get_messages()
    filtered_messages = [
        message for message in messages
        if message.get("type") in {"ERROR"}
    ]
  # Only print if there are error messages to print
    if filtered_messages:
        pprint(filtered_messages)
    gld.clear_messages()


    # Collect data for all houses at current time step
    air_dict = gld.get_properties_by_class("house", "air_temperature") 
    hvac_off_dict = gld.get_properties_by_class("house", "hvac_last_off") 
    hvac_load_dict = gld.get_properties_by_class("house", "hvac_load")

    # TODO: remove these methods once Github # 1724 have been resolved
    air_dict = parse_temperature_data(air_dict)
    hvac_load_dict = parse_hvac_load_data(hvac_load_dict)
    hvac_off_dict = parse_hvac_off_data(hvac_off_dict, hvac_load_dict, starttime, sim_time_obj)
    hvac_off_elapsed_dict = check_hvac_off_elapsed(hvac_off_dict, sim_time_obj, threshold_seconds=300)

    # Adjust step size based on HVAC off elapsed times
    if all(hvac_off_elapsed_dict.values()):
        step_size = 300
        big_step_count += 1
    else:
        step_size = 60
        small_step_count += 1
    
    # Write air temperature and HVAC last off time to HDF5 and CSV files
    # Initialize HDF5 datasets on first iteration
    if 'air_temperature' not in hdf5_file:
        air_temp_group = hdf5_file.create_group("air_temperature")
        for house in air_dict.keys():
            air_temp_group.create_dataset(house, data=[air_dict[house]], maxshape=(None,), dtype='f')
    else:
        # Append data to existing datasets
        air_temp_group = hdf5_file["air_temperature"]
        for house in air_dict.keys():
            air_temp_group[house].resize(air_temp_group[house].shape[0] + 1)
            air_temp_group[house][-1] = air_dict[house]

    dummy = 0
print(f"big_step_count: {big_step_count}")
print(f"small_step_count: {small_step_count}")
gld.stop()
gld.exit_gld()

# Plot the air temperature data from the HDF5 file
plot_air_temperature("output_data.h5")