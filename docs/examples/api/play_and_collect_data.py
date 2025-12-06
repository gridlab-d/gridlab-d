"""
Created on 11/20/2025

This example shows how to use the GridLAB-D API to play in values to a 
GridLAB-D model during runtime and then read values out of the model and write
them to file. This largely replicates the existing "player" and "recorder"
object functionality but with much more flexibility (as the functionality is
user-defined).

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import json
from pprint import pprint
from datetime import datetime, timedelta
import csv
import os
import argparse
from matplotlib import pyplot as plt
import matplotlib as m
import logging
import sys

# Setting up logging
logger = logging.getLogger(__name__)

def write_to_csv(file_name, data, headers=None):
    """
    Writes a list of values to a CSV file. Optionally adds a header row.

    Args:
        file_name (str): The name of the CSV file to write to.
        data (list): A list of values to write as a row in the CSV.
        headers (list, optional): A list of headers to write as the first row in the CSV.

    Raises:
        Exception: If there is an error writing to or creating the file.
    """
    try:
        # Check if the file already exists
        file_exists = False
        try:
            with open(file_name, 'r') as file:
                file_exists = True
        except FileNotFoundError:
            # File doesn't exist, proceed to create it
            pass

        # Open the file in append mode
        with open(file_name, mode='a', newline='', encoding='utf-8') as csv_file:
            writer = csv.writer(csv_file)

            # Write the header row if provided and the file doesn't exist already
            if headers and not file_exists:
                writer.writerow(headers)

            # Write the data row
            writer.writerow(data)
            csv_file.flush()

    except Exception as e:
        print(f"An error occurred: {e}")


def parse_gld_time(time_str):
    """
    Parses a GridLAB-D time string and converts it to a datetime object.

    Args:
        time_str (str): A time string retrieved from GridLAB-D, formatted as '%Y-%m-%d %H:%M:%S'.

    Returns:
        datetime: A Python datetime object.
    """
    parts = time_str.rsplit(' ', 1)
    if len(parts) == 2 and parts[1] in ['PST', 'PDT', 'EST', 'EDT', 'CST', 'CDT', 'MST', 'MDT']:
        time_str = parts[0]
    return datetime.strptime(time_str, '%Y-%m-%d %H:%M:%S')


def remove_old_results_file(filename:str):
    """
    Removes the specified file if it exists.

    Args:
        filename (str): File path to remove.

    Raises:
        Exception: If an error occurs while attempting to remove the file.
    """
    try:
        if os.path.exists(filename):  # Check if the file exists
            os.remove(filename)  # Remove the file
            print(f"File '{filename}' has been successfully removed.")
        else:
            print(f"File '{filename}' does not exist.")
    except Exception as e:
        print(f"An error occurred while attempting to remove '{filename}': {e}")


def _auto_run(args):
    remove_old_results_file(os.path.join(args.model_folder,  "indoor_air_temperatures.csv"))
    remove_old_results_file(os.path.join(args.model_folder, "floor_area.csv"))

    gld = gridlabd.GridLabD()
    model_path = Path(args.model_folder)
    gld.set_working_directory(str(model_path))

    # Initializing model
    gld.load_glm(["gridlabd", "./houses.glm"])
    step_size = 900
    gld.set_time_step(step_size)
    gld.step()
    status, current_time_str = gld.get_time()
    current_time = parse_gld_time(current_time_str)

    # Making a datetime object for the stoptime
    checkpoint_json = gld.get_checkpoint_json()
    checkpoint_dict = json.loads(checkpoint_json)
    stop_time_etime = float(checkpoint_dict['clock']['stoptime'])
    stop_time = datetime.fromtimestamp(stop_time_etime)

    if args.graph:
        # Initialize Matplotlib plot
        plt.ion()  # Enable interactive mode
        fig, ax = plt.subplots()
        ax.set_title("House1 Floor Area Over Time")
        ax.set_xlabel("Simulation Time")
        ax.set_ylabel("Floor Area")
        line, = ax.plot([], [], 'o-', label="House1 Floor Area")  # Initialize empty line plot
        ax.legend()

    # Running the simulation and collecting data
    house_names = gld.get_objects_by_class('house')
    house_header_row = ["sim time"] + house_names

    output_floor_area_file = "floor_area.csv"
    output_temperature_file = "indoor_air_temperatures.csv"
    played_in_floor_area = 1000
    floor_area_timestamps = []  # Store simulation timestamps for plot
    house1_floor_areas = []  # Store floor areas of House1 for plot 

    while current_time < stop_time:
        # Play in data
        # Some properties, such as the indoor air temperature, are 
        # calculated by GridLAB-D and effectively cannot be written
        # to by the API. "floor_area" is not one of them and thus
        # it makes for a good, if highly unrealistic, test case.
        for house in house_names:
            gld.set_property(house, "floor_area", str(played_in_floor_area))
        played_in_floor_area += 100
            
        # Advance simulation time
        # Trying both methods

        # Method #1: step_to()
        next_time = current_time + timedelta(seconds=900)
        next_time = next_time.strftime('%Y-%m-%d %H:%M:%S')
        print(type(next_time))
        status, step_to_time = gld.step_to(next_time)

        # Method #2: step()    
        # gld.step() # Doesn't stp at specified step size

        status, current_time_str = gld.get_time()
        current_time = parse_gld_time(current_time_str)
        print(f"Post-step current_time: {current_time}")

        # Collect indoor air temperatures
        data = [current_time]
        for house in house_names:
            house_properties = gld.get_object_properties(house)
            data.append(house_properties["air_temperature"])
        write_to_csv(output_temperature_file, data, house_header_row)    

        # Collect floor areas and update live Plotly graph
        data = [current_time]
        for house in house_names:
            house_properties = gld.get_object_properties(house)
            data.append(house_properties["floor_area"])
            if args.graph:
                m.interactive(True)
                if house == "house1":
                    floor_area_timestamps.append(current_time)
                    house1_floor_areas.append(house_properties["floor_area"])
                    # Update Matplotlib plot
                    line.set_xdata(floor_area_timestamps)
                    line.set_ydata(house1_floor_areas)
                    ax.set_xlim(min(floor_area_timestamps), max(floor_area_timestamps))
                    ax.set_ylim(min(house1_floor_areas), max(house1_floor_areas))
                    plt.draw()
                    plt.show()
                    plt.pause(1)  # Pause briefly to allow the figure to refresh
        write_to_csv(output_floor_area_file, data, house_header_row)    
        dummy = 1

if __name__ == '__main__':
    # This slightly complex mess allows lower importance messages
    # to be sent to the log file and ERROR messages to additionally
    # be sent to the console as well. Thus, when bad things happen
    # the user will get an error message in both places which,
    # hopefully, will aid in trouble-shooting.
    fileHandle = logging.FileHandler("gld_pypower.log",'w')
    fileHandle.setLevel(logging.DEBUG)
    streamHandle = logging.StreamHandler(sys.stdout)
    streamHandle.setLevel(logging.DEBUG)
    logging.basicConfig(level=logging.DEBUG,
                        handlers=[fileHandle, streamHandle])
    parser = argparse.ArgumentParser(description="Runs GridLAB-D and makes plots,")
    parser.add_argument('-g', '--graph',
                        help="flag to only create a graph of simulated data",
                        action=argparse.BooleanOptionalAction,
                        default=True)
    parser.add_argument('-m', '--model_folder',
                         help="folder that contains the  GridLAB-D model",
                         nargs='?',
                         default="./house_with_solar")
    # parser.add_argument('-o', '--output_file',
    #                     help="output file to write results to",
    #                     nargs='?',
    #                     default="disk_space.txt")
    args = parser.parse_args()
    _auto_run(args)
    
