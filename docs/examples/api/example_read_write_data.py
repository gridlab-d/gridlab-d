"""
Created on 03/25/2026

This example shows how to read data out of GridLAB-D™ while the simulation is
running and write or edit parameter values of objects in the running model. 
This effectively demonstrates the ability to create controllers in Python that
interact with the GridLAB-D™ model during runtime.

As an example, this code seeks to increase energy self-consumption by reading 
the power output of the solar panel and if it is sufficiently large, decrease
the cooling setpoint. This provides a pre-cooling function when there is local
generation available. When the solar power output is low, the setpoint is 
returned to normal. To effectively demonstrate this, the simulation duration is
extended to two days (as compared to many of the other examples).

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


from pprint import pprint

import gridlabd
from pathlib import Path
import os
from datetime import datetime, timedelta, timezone
import numpy as np
import re
import matplotlib.pyplot as plt

step_size = 300


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

def parse_hvac_off_data(hvac_off_dict, hvac_load_dict, starttime, sim_time_dt):
    """
    Convert HVAC off time strings to datetime objects.
    
    Args:
        hvac_off_dict (dict): Dictionary with house names as keys and timestamp
                 strings as values formatted as "2023-07-01 00:22:29 PDT"
                 or "INIT"
        hvac_load_dict (dict): Dictionary with house names as keys and HVAC
                 load values (float or parseable numeric strings)
        starttime (datetime): Simulation start time used when value is "INIT"
        sim_time_dt (datetime): Current simulation time used when HVAC load
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
                parsed_data[house] = sim_time_dt
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


def parse_cooling_setpoint_data(cooling_setpoint_dict):
    """
    Convert cooling setpoint values from strings with degF units to floats.

    Args:
        cooling_setpoint_dict (dict): Dictionary with house names as keys and
                                     setpoint values as strings with units
                                     (e.g., "72.5 degF")

    Returns:
        dict: Dictionary with house names as keys and setpoint values as floats
    """
    parsed_data = {}
    for house, setpoint_str in cooling_setpoint_dict.items():
        numeric_str = str(setpoint_str).replace("degF", "").strip()
        try:
            parsed_data[house] = float(numeric_str)
        except ValueError:
            numeric_match = re.search(r'-?\d+\.?\d*', str(setpoint_str))
            if numeric_match:
                parsed_data[house] = float(numeric_match.group())
            else:
                print(f"Warning: Could not parse cooling setpoint for {house}: {setpoint_str}")
                parsed_data[house] = None
    return parsed_data


def parse_inverter_rated_power_data(inverter_rated_power_dict):
    """
    Convert inverter rated power values from strings with VA units to floats.

    Args:
        inverter_rated_power_dict (dict): Dictionary with inverter names as keys
                                         and rated power values as strings with
                                         units (e.g., "5000 VA")

    Returns:
        dict: Dictionary with inverter names as keys and rated power values as
              floats
    """
    parsed_data = {}
    for inverter, power_str in inverter_rated_power_dict.items():
        numeric_str = str(power_str).replace("VA", "").strip()
        try:
            parsed_data[inverter] = float(numeric_str)
        except ValueError:
            numeric_match = re.search(r'-?\d+\.?\d*', str(power_str))
            if numeric_match:
                parsed_data[inverter] = float(numeric_match.group())
            else:
                print(f"Warning: Could not parse rated power for {inverter}: {power_str}")
                parsed_data[inverter] = None
    return parsed_data


def parse_solar_power_data(solar_power_dict):
    """
    Convert solar power values from strings with W units to floats.

    Args:
        solar_power_dict (dict): Dictionary with inverter names as keys and power
                                values as strings with units (e.g., "2500 W")

    Returns:
        dict: Dictionary with inverter names as keys and power values as floats
    """
    parsed_data = {}
    for inverter, power_str in solar_power_dict.items():
        numeric_str = str(power_str).replace("W", "").strip()
        try:
            parsed_data[inverter] = float(numeric_str)
        except ValueError:
            numeric_match = re.search(r'-?\d+\.?\d*', str(power_str))
            if numeric_match:
                parsed_data[inverter] = float(numeric_match.group())
            else:
                print(f"Warning: Could not parse solar power for {inverter}: {power_str}")
                parsed_data[inverter] = None
    return parsed_data


def plot_solar_power_and_setpoint(timestamps, avg_solar_power, avg_cooling_setpoint):
    """
    Plot average solar power and average cooling setpoint over time using dual axes.
    
    Args:
        timestamps (list): List of datetime objects representing simulation times
        avg_solar_power (list): List of average solar power values (W) across all houses
        avg_cooling_setpoint (list): List of average cooling setpoint values (degF) across all houses
    """
    fig, ax1 = plt.subplots(figsize=(12, 6))
    
    # Plot solar power on the first y-axis
    color1 = 'tab:blue'
    ax1.set_xlabel('Time')
    ax1.set_ylabel('All House Average Solar Power (W)', color=color1)
    line1 = ax1.plot(timestamps, avg_solar_power, color=color1, label='Avg Solar Power')
    ax1.tick_params(axis='y', labelcolor=color1)
    ax1.grid(True, alpha=0.3)
    
    # Create a second y-axis for cooling setpoint
    ax2 = ax1.twinx()
    color2 = 'tab:red'
    ax2.set_ylabel('All House Average Cooling Setpoint (°F)', color=color2)
    line2 = ax2.plot(timestamps, avg_cooling_setpoint, color=color2, label='Avg Cooling Setpoint')
    ax2.tick_params(axis='y', labelcolor=color2)
    
    # Add title and legend
    fig.suptitle('All House Average Solar Power and Cooling Setpoint Over Time')
    
    # Combine legends from both axes
    lines = line1 + line2
    labels = [l.get_label() for l in lines]
    ax1.legend(lines, labels, loc='upper left')
    
    fig.tight_layout()
    plt.show()


# Ensure's we're running from the correct directory
script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

# Initilize GridLAB-D™ and load the model
gld = gridlabd.GridLabD()
model_path = Path("house_with_solar")
gld.set_working_directory(str(model_path))
load_code = gld.load("houses.glm")
if load_code != 0:
    raise RuntimeError(f"Failed to load model with error code {load_code}.")

# Read in current start and stop time
starttime = datetime.fromisoformat(gld.get_starttime())
stoptime = datetime.fromisoformat(gld.get_stoptime())

# Calculate new simulation duration, set stop time, and confirm changes
stoptime = starttime + timedelta(days=2)
stoptime_str = datetime.isoformat(stoptime)
gld.set_stoptime(stoptime_str)
stoptime = datetime.fromisoformat(gld.get_stoptime())

# Initialize data collection lists
timestamps = []
avg_solar_power_list = []
avg_cooling_setpoint_list = []

# Run the model and collect data at each time step, adjusting step size when
# appropriate
status, sim_time = gld.get_time()
sim_time_dt = datetime.fromisoformat(sim_time)
inverter_rated_power_dict = gld.get_properties_by_class("inverter", "rated_power")
inverter_rated_power_dict = parse_inverter_rated_power_data(inverter_rated_power_dict)
cooling_setpoint_dict = gld.get_properties_by_class("house", "cooling_setpoint")
original_cooling_setpoint_dict = parse_cooling_setpoint_data(cooling_setpoint_dict)

while sim_time_dt < stoptime:
    #print(f"Current simulation time: {sim_time_dt}")
    error_code, sim_time = gld.step()
    if error_code != 0:
        raise RuntimeError(f"Simulation step failed at {sim_time} with error code {error_code}.")
    sim_time_dt = datetime.fromisoformat(sim_time)

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
    cooling_setpoint_dict = gld.get_properties_by_class("house", "cooling_setpoint")
    cooling_setpoint_dict = parse_cooling_setpoint_data(cooling_setpoint_dict)
    solar_power_dict = gld.get_properties_by_class("inverter", "VA_Out")
    solar_power_dict = parse_solar_power_data(solar_power_dict)

    # Calculate averages for this time step
    house_solar_powers = []
    house_cooling_setpoints = []
    
    # For each house, set cooling setpoint based on whether solar output
    # exceeds 50% of the inverter's rated power, and collect data
    for house in original_cooling_setpoint_dict:
        inverter_name = f"{house}_sol_inverter"
        solar_power = solar_power_dict.get(inverter_name, 0.0) or 0.0
        current_setpoint = cooling_setpoint_dict.get(house, original_cooling_setpoint_dict[house]) or original_cooling_setpoint_dict[house]
        
        house_solar_powers.append(solar_power)
        house_cooling_setpoints.append(current_setpoint)
        
        rated_power = inverter_rated_power_dict.get(inverter_name, 0.0) or 0.0
        if rated_power > 0 and solar_power > 0.5 * rated_power:
            gld.set_property(house, "cooling_setpoint", "60")
            # print (f"{sim_time_dt} - {house}: Solar power {solar_power:.1f} W exceeds 50% of rated power {rated_power:.1f} VA, setting cooling setpoint to 60 degF")
        else:
            gld.set_property(
                house, "cooling_setpoint", str(original_cooling_setpoint_dict[house])
            )
    
    # Store averaged data
    timestamps.append(sim_time_dt)
    avg_solar_power_list.append(np.mean(house_solar_powers) if house_solar_powers else 0.0)
    avg_cooling_setpoint_list.append(np.mean(house_cooling_setpoints) if house_cooling_setpoints else 0.0)

    dummy = 0
gld.stop()
gld.exit_gld()

# Plot the collected data
plot_solar_power_and_setpoint(timestamps, avg_solar_power_list, avg_cooling_setpoint_list)


