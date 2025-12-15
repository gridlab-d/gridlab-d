"""
Created on 11/20/2025

This example shows how to run a combined T+D powerflow simulation using
GridLAB-D and PYPOWER, both as libraries that solve the distribution system
and transmission system powerflow (they both do more than that).

Some of the functions defined here were written by ChatGPT on 
or around the time this file was created.

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
import pypower as pp
from pathlib import Path
import json
from pprint import pprint
from datetime import datetime, timedelta
import csv
import os
import sys
import logging
import argparse
import cmath
from pypower.api import runpf
from pypower.case118 import case118

# Setting up logging
logger = logging.getLogger(__name__)


def write_to_csv(file_name, data, headers=None):
    """
    Writes a list of values to a CSV file. Optionally adds a header row.

    Parameters:
        file_name (str): The name of the CSV file to write to.
        data (list): A list of values to write as a row in the CSV.
        headers (list, optional): A list of headers to write as the first row in the CSV.
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
    parts = time_str.rsplit(' ', 1)
    if len(parts) == 2 and parts[1] in ['PST', 'PDT', 'EST', 'EDT', 'CST', 'CDT', 'MST', 'MDT']:
        time_str = parts[0]
    return datetime.strptime(time_str, '%Y-%m-%d %H:%M:%S')

def remove_old_results_file(filename:str):
    """Remove the specified file if it exists."""
    try:
        if os.path.exists(filename):  # Check if the file exists
            os.remove(filename)  # Remove the file
            print(f"File '{filename}' has been successfully removed.")
        else:
            print(f"File '{filename}' does not exist.")
    except Exception as e:
        print(f"An error occurred while attempting to remove '{filename}': {e}")

def parse_gld_load_str(value_with_unit):
    # Define SI prefix multipliers
    prefixes = {
        '': 1,  # No prefix
        'k': 1e3,  # kilo
        'M': 1e6,  # mega
        'G': 1e9,  # giga
        'T': 1e12,  # tera
        'm': 1e-3,  # milli
        'u': 1e-6,  # micro
        'n': 1e-9,  # nano
        'p': 1e-12  # pico
    }
    
    # Remove the trailing 'VA' from the string and strip whitespace
    value_str = value_with_unit.replace('VA', '').strip()
    
    # Split into number and unit prefix (if any)
    for prefix in prefixes:
        if value_str.endswith(prefix):
            #numeric_part = value_str[:-len(prefix)].strip()  # Extract the numeric part
            numeric_part = value_str
            multiplier = prefixes[prefix]  # Get the multiplier
            try:
                # Convert to complex and apply multiplier
                complex_value = complex(numeric_part) * multiplier
                return complex_value
            except ValueError:
                raise ValueError(f"Invalid complex number format: {value_str}")
    
    # If no valid prefix is found, default to no multiplier
    try:
        return complex(value_str)
    except ValueError:
        raise ValueError(f"Invalid complex number format: {value_str}")


def _auto_run(args):
    glds = {
        "2": {
            "gld": None,
            "load scaling factor": 5,
            "GLD nominal load MVA": complex(3.6, 1.3),
            "voltage scaling factor": 57.5,
            "GLD nominal substation voltage kV": 2.4,
            "IEEE-118 load": "20 + 9j MVA",
            "dist load": complex(0,0),
            "substation voltage": complex(0,0)    
        },
        "16": {
            "gld": None,
            "load scaling factor": 7,
            "GLD nominal load MVA": complex(3.6, 1.3),
            "voltage scaling factor": 57.5,
            "GLD nominal substation voltage kV": 2.4,
            "IEEE-118 load": "25 + 10j MVA",
            "dist load": complex(0,0),
            "substation voltage": complex(0,0)    
        },
        # "22": {
        #     "gld": None,
        #     "load scaling factor": 3,
        #     "GLD nominal load MVA": complex(3.6, 1.3),
        #     "voltage scaling factor": 57.5,
        #     "GLD nominal substation voltage kV": 2.4,
        #     "IEEE-118 load": "10 + 5j MVA",
        #     "dist load": complex(0,0),
        #     "substation voltage": complex(0,0)    
        # },
        # "33": {
        #     "gld": None,
        #     "load scaling factor": 6,
        #     "GLD nominal load MVA": complex(3.6, 1.3),
        #     "voltage scaling factor": 57.5,
        #     "GLD nominal substation voltage kV": 2.4,
        #     "IEEE-118 load": "23 + 9j MVA",
        #     "dist load": complex(0,0),
        #     "substation voltage": complex(0,0)    
        # },
        # "43": {
        #     "gld": None,
        #     "load scaling factor": 4,
        #     "GLD nominal load MVA": complex(3.6, 1.3),
        #     "voltage scaling factor": 57.5,
        #     "GLD nominal substation voltage kV": 2.4,
        #     "IEEE-118 load": "18 + 7j MVA",
        #     "dist load": complex(0,0),
        #     "substation voltage": complex(0,0)    
        # },
        # "60": {
        #     "gld": None,
        #     "load scaling factor": 21,
        #     "GLD nominal load MVA": complex(3.6, 1.3),
        #     "voltage scaling factor": 57.5,
        #     "GLD nominal substation voltage kV": 2.4,
        #     "IEEE-118 load": "78 + 3j MVA",
        #     "dist load": complex(0,0),
        #     "substation voltage": complex(0,0)    
        # },
        # "67": {
        #     "gld": None,
        #     "load scaling factor": 8,
        #     "GLD nominal load MVA": complex(3.6, 1.3),
        #     "voltage scaling factor": 57.5,
        #     "GLD nominal substation voltage kV": 2.4,
        #     "IEEE-118 load": "28 + 7j MVA",
        #     "dist load": complex(0,0),
        #     "substation voltage": complex(0,0)    
        # },
        # "78": {
        #     "gld": None,
        #     "load scaling factor": 20,
        #     "GLD nominal load MVA": complex(3.6, 1.3),
        #     "voltage scaling factor": 57.5,
        #     "GLD nominal substation voltage kV": 2.4,
        #     "IEEE-118 load": "71 + 26j MVA",
        #     "dist load": complex(0,0),
        #     "substation voltage": complex(0,0)    
        # },
        # "86": {
        #     "gld": None,
        #     "load scaling factor": 6,
        #     "GLD nominal load MVA": complex(3.6, 1.3),
        #     "voltage scaling factor": 57.5,
        #     "GLD nominal substation voltage kV": 2.4,
        #     "IEEE-118 load": "21 + 10j MVA",
        #     "dist load": complex(0,0),
        #     "substation voltage": complex(0,0)    
        # },
        # "95": {
        #     "gld": None,
        #     "load scaling factor": 12,
        #     "GLD nominal load MVA": complex(3.6, 1.3),
        #     "voltage scaling factor": 57.5,
        #     "GLD nominal substation voltage kV": 2.4,
        #     "IEEE-118 load": "42 + 31j MVA",
        #     "dist load": complex(0,0),
        #     "substation voltage": complex(0,0)    
        # },
        # "106": {
        #     "gld": None,
        #     "load scaling factor": 12,
        #     "GLD nominal load MVA": complex(3.6, 1.3),
        #     "voltage scaling factor": 57.5,
        #     "GLD nominal substation voltage kV": 2.4,
        #     "IEEE-118 load": "43 + 16j MVA",
        #     "dist load": complex(0,0),
        #     "substation voltage": complex(0,0)    
        # },
        # "115": {
        #     "gld": None,
        #     "load scaling factor": 6,
        #     "GLD nominal load MVA": complex(3.6, 1.3),
        #     "voltage scaling factor": 57.5,
        #     "GLD nominal substation voltage kV": 2.4,
        #     "IEEE-118 load": "22 + 7j MVA",
        #     "dist load": complex(0,0),
        #     "substation voltage": complex(0,0)    
        # }
    }
    
    for bus, gld_dict in glds.items():
        # Initialize GridLAB-D
        logger.info(f"Initializing GridLAB-D {bus}")
        ## Instantiating GridLAB-D object
        gld = gridlabd.GridLabD()
        model_path = Path(f"/home/hard312/gld_api/gld_pypower")
        gld.set_working_directory(str(model_path))

        ## Initializing model
        gld.load_glm(["gridlabd", "./IEEE-123.glm"])
        gld.set_time_step(1)
        gld.step()
        glds[bus]["gld"] = gld

        if bus == "2": # Only need to do this once
        ## Managing time; assume all models use the same time
            status, current_time_str = gld.get_time()
            current_time = parse_gld_time(current_time_str)
            checkpoint_json = gld.get_checkpoint_json()
            checkpoint_dict = json.loads(checkpoint_json)
            stop_time_etime = float(checkpoint_dict['clock']['stoptime'])
            stop_time = datetime.fromtimestamp(stop_time_etime)


    # Initialize PYPOWER
    logger.info("Initializing PYPOWER")
    ppc = case118()

    # Begin combined T+D powerflow simulation
    while current_time < stop_time:
        ## Doing this serially because I'm too lazy to look up how to do it in
        ## parallel with multiprocessing
        for bus, gld_dict in glds.items():

            ## Get GLD load scale it for PYPOWER nodes and apply it
            ## Picking load-only buses (not those attached to generators)
            substation_properties = gld_dict["gld"].get_object_properties("n610")
            dist_load = parse_gld_load_str(substation_properties["distribution_load"])
            glds[bus]["dist load"] = dist_load * glds[bus]["load scaling factor"]
            ppc["bus"][int(bus) - 1][2] = glds[bus]["dist load"].real
            ppc["bus"][int(bus) - 1][3] = glds[bus]["dist load"].imag
        
        
        ## Run IEEE-118 PYPOWER powerflow
        results, success = runpf(ppc)

        ## Get voltages from PYPOWER and apply to GLD
        for bus, gld_dict in glds.items():
            vm = results["bus"][1][7]
            va = results["bus"][1][8] * cmath.pi/180
            v = cmath.rect(vm, va) / 57.5
            glds[bus]["gld"].set_property("n610", "positive_sequence_voltage", str(v))
            gld_dict["gld"].step()



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
    parser = argparse.ArgumentParser(description="Runs combined T+D powerflow,"
                                     "using GridLAB-D and PYPOWER.")
    # parser.add_argument('-g', '--graph',
    #                     help="flag to only create a graph of the historic data"
    #                             "(no data collection)",
    #                     action=argparse.BooleanOptionalAction)
    # parser.add_argument('-i', '--input_paths',
    #                     help="paths of folders to get sizes of, one per line",
    #                     nargs='?',
    #                     default="folder_paths_to_size.txt")
    # parser.add_argument('-o', '--output_file',
    #                     help="output file to write results to",
    #                     nargs='?',
    #                     default="disk_space.txt")
    args = parser.parse_args()
    _auto_run(args)
    
