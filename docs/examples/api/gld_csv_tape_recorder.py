"""
Created on 10/16/2026

This example shows how to perform native tape-module functions (recorder,
player, etc) using the API. The API provides additional flexibility compared 
to the native tape module allowing data to be played in or recorded at 
arbitrary intervals and/or based on the state of the objects in the model. 

In this example, a ficticious requirement is made that when the played in 
substation voltage is below a certain value, the service voltage (measured
by the triplex meters) for each house need to be recorded at a higher 
temporal resolution.

The file playing in the substation voltage has a temporal resolution of
one minute but under normal circumstances we will advance time in ten-
minute steps. We'll switch to one-minute steps once the substation voltage is
below a critical value and back to ten-minute steps once it is above that
value.


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import libgld
import helics as h
import csv
import logging
import argparse
import sys

# Setting up logging
logger = logging.getLogger(__name__)


def _open_file(file_path: str, type='r'):
    """Utilty function to open file with reasonable error handling.


    Args:
        file_path (str) - Path to the file to be opened

        type (str) - Type of the open method. Default is read ('r')


    Returns:
        fh (file object) - File handle for the open file
    """
    try:
        fh = open(file_path, type)
    except IOError:
        logger.error('Unable to open {}'.format(file_path))
    else:
        return fh




def _auto_run(args):
    # Opening up CSVs with substation voltage data we'll be playing in
    # File is assumed to have values that advance one minute at a time
    va_fh = _open_file(args.va)
    vb_fh = _open_file(args.vb)
    vc_fh = _open_file(args.vc)
    csv_va = csv.reader(va_fh)
    csv_vb = csv.reader(vb_fh)
    csv_vc = csv.reader(vc_fh)

    # Opening up CSV for the data we'll be writing out
    meter_fh = _open_file(args.output_file)
    csv_meter = csv.writer(meter_fh)

    # Setting up GLD
    gld = libgld.GLD()
    glm = gld.open_model_file("best_model_ever.json")
    start_time = glm.clock["starttime"]
    stop_time = glm.clock["stoptime"]
    sim_time = start_time

    # Getting the name of the substation object; there should only be one
    substation_name = glm.substations.keys()[0]["name"]

    # Getting the names of the triplex meters associated with the houses
    # Assume the houses are parented to the triplex meters (most common)
    triplex_meters = []
    for house in glm.houses.keys():
        if "parent" in house.keys()
            triplex_meters.append(house["parent"])
        else:
            logger.warning(f"No 'parent' attribute for house {house["name"]}"
                            f"no data will be collected.")

    # Load the model into the gldcore
    messages = gld.load_model(glm)

    # Start simulation by advancing simulation 
    while sim_time <= stop_time:
        # Update substation voltage using the next line of the voltage CSVs
        # CSV format: [time ordinal, complex voltage]
        # GridLAB-D voltage complex format is magnitude, angle "139675.872+30d"
        [timestamp, va_str] = csv_va.__next__()
        gld.set_parameter(substation_name, "distribution_voltage_A", va_str)
        [timestamp, vb_str] = csv_vb.__next__()
        gld.set_parameter(substation_name, "distribution_voltage_A", vb_str)
        [timestamp, vc_str] = csv_vc.__next__()
        gld.set_parameter(substation_name, "distribution_voltage_A", vc_str)
        
        # Parse voltage strings to if we need to do a big time step or not.
        # Big time step when average magnitude of all three voltages is above
        # the critical value; small time step when below.
        [magnitude_a, angle_a] = va_str.split("+")
        [magnitude_b, angle_b] = vb_str.split("+")
        [magnitude_c, angle_c] = vc_str.split("+")
        avg_v = (float(magnitude_a) + float(magnitude_b) + float(magnitude_c))/3
        if avg_v < args.critical_voltage:
            time_step = 60
            collect_meter_data = False
        else:
            time_step = args.time_step
            collect_meter_data = True

        # Advance rows through the CSVs to get us set up to read the correct
        # next row. CSVs are one minute (60 seconds) per row and we're advancing
        # one row less than required as we read in the next row at the top of 
        # the loop.
        # When advancing the rows, we don't care about the data the reader is
        # returning.
        rows_to_advance = time_step/60 - 1
        for row in range(rows_to_advance):
            csv_va.__next__()
            csv_vb.__next__()
            csv_vc.__next__()

        # Advance GridLAB-D's simulation time
        gld.sim_step(time_step)

        # Collect meter data if we're below the critical voltage
        # We're not tracking which meter has which value, just what the 
        # population looks like when the substation voltage is low
        # GridLAB-D triplex meter voltage is split phase and measured with
        # respect to ground.
        if collect_meter_data:
            meter_data = []
            for meter in triplex_meters:
                meter_data.append(gld.get_parameter(meter, "measured_voltage_1"))
            csv_meter.writerow(meter_data)

    # We're done; close it up
    messages = gld.exit_gld()
    va_fh.close()
    vb_fh.close()
    vc_fh.close()
    meter_fh.close()

    
    


if __name__ == '__main__':
    # This slightly complex mess allows lower importance messages
    # to be sent to the log file and ERROR messages to additionally
    # be sent to the console as well. Thus, when bad things happen
    # the user will get an error message in both places which,
    # hopefully, will aid in trouble-shooting.
    fileHandle = logging.FileHandler("gld_tape_recorder.log",'w')
    fileHandle.setLevel(logging.DEBUG)
    streamHandle = logging.StreamHandler(sys.stdout)
    streamHandle.setLevel(logging.ERROR)
    logging.basicConfig(level=logging.DEBUG,
                        handlers=[fileHandle, streamHandle])
    parser = argparse.ArgumentParser(description="Runs GridLAB-D tape example")
    parser.add_argument('-va', '--va',
                        help="path to V_A.csv",
                        nargs='?'
                        default="V_A.csv")
    parser.add_argument('-vb', '--vb',
                    help="path to V_B.csv",
                    nargs='?'
                    default="V_B.csv")
    parser.add_argument('-vc', '--vc',
                    help="path to V_C.csv",
                    nargs='?'
                    default="V_C.csv")
    parser.add_argument('-o', '--output_file',
                        help="output file to write results to",
                        nargs='?',
                        default="meter_voltages.csv")
    parser.add_argument('-cv', '--critical_voltage',
                        help="low voltage that triggers data collection",
                        nargs='?',
                        default=138000)
    parser.add_argument('-ts', '--time_step',
                        help="large time step in seconds",
                        nargs='?',
                        default=900)
    args = parser.parse_args()
    _auto_run(args)

