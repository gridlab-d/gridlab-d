"""
Created on 02/20/2026

This example tests the computation cost of stepping at a regular interval 
versus just letting a model run freely.


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path
import json
import os
import sys
import argparse
import logging
from datetime import datetime, timedelta
import time

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

def main(args):
    # Ensure's we're running from the correct directory
    script_path = os.path.abspath(__file__)
    script_dir = os.path.dirname(script_path)
    os.chdir(script_dir)

    gld = gridlabd.GridLabD(verbose = False)
    model_path = Path("long_house")
    gld.set_working_directory(str(model_path))
    loaded_model= gld.load("houses.glm")
    start = time.perf_counter()
    gld.run()
    end = time.perf_counter()
    running_elapsed_wall_clock = end - start
    logger.info(f"Running elapsed wall clock time: {running_elapsed_wall_clock}")
    gld.stop()
    gld.exit_gld()

    
    for idx in range(args.num_steps_size_steps):
        # First time through, just use initial step size given as an arg
        if idx != 0:
            step_size = step_size + args.step_size_inc
        else:
            step_size = args.min_step_size

        # Stepping
        gld = gridlabd.GridLabD(verbose = False)
        model_path = Path("long_house")
        gld.set_working_directory(str(model_path))
        loaded_model= gld.load("houses.glm")
        start_time = datetime.fromisoformat(gld.get_starttime())
        stop_time = datetime.fromisoformat(gld.get_stoptime())
        sim_length = stop_time - start_time
        total_sim_seconds = sim_length.total_seconds()
        logger.info(f"Simulation step size: {step_size}")
        logger.info(f"Total seconds to be simulated: {total_sim_seconds}")
        gld.set_time_step(step_size)
        num_steps = int(total_sim_seconds / step_size)
        logger.info(f"Number of simulation steps {num_steps}")
        start = time.perf_counter()
        for step in range(num_steps):
            gld.step()
        end = time.perf_counter()
        stepping_elapsed_wall_clock = end - start
        stepping_speed_up_factor = total_sim_seconds/stepping_elapsed_wall_clock
        logger.info(f"Stepping elapsed wall clock time: {stepping_elapsed_wall_clock}")
        #logger.info(f"Stepping Simulation speed-up factor: {stepping_speed_up_factor}")

        #running_speed_up_factor = total_sim_seconds/running_elapsed_wall_clock
        #logger.info(f"Running Simulation speed-up factor: {running_speed_up_factor}")
        time_penalty_factor = (stepping_elapsed_wall_clock - running_elapsed_wall_clock)/sim_length.total_seconds()
        logger.info(f"TIME PENALTY FACTOR: {time_penalty_factor}")
        #time_penalty_per_step = time_penalty/num_steps
        #logger.info(f"TIME PENALTY PER STEP (s): {time_penalty_per_step}")
        logger.info("---------------------------------------------------------------------")
        gld.stop()
        gld.exit_gld()


        fp = _open_file(args.results_file, 'a')
        fp.write(f"{step_size},{time_penalty_factor}\n")
        fp.close()



if __name__ == '__main__':
    # This slightly complex mess allows lower importance messages
    # to be sent to the log file and ERROR messages to additionally
    # be sent to the console as well. Thus, when bad things happen
    # the user will get an error message in both places which,
    # hopefully, will aid in trouble-shooting.
    fileHandle = logging.FileHandler("long_run.log",'w')
    fileHandle.setLevel(logging.DEBUG)
    streamHandle = logging.StreamHandler(sys.stdout)
    streamHandle.setLevel(logging.DEBUG)
    logging.basicConfig(level=logging.DEBUG,
                        handlers=[fileHandle, streamHandle])
    parser = argparse.ArgumentParser(description="Runs GridLAB-D either by stepping or freely,")
    parser.add_argument('-m', '--min_step_size',
                         help="simulation step size in seconds",
                         nargs='?',
                         default=57)
    parser.add_argument('-i', '--step_size_inc',
                         help="simulation step size in seconds",
                         nargs='?',
                         default=5)
    parser.add_argument('-n', '--num_steps_size_steps',
                         help="number of step size of increases",
                         nargs='?',
                         default=20)
    parser.add_argument('-r', '--results_file',
                         help="file to which to write simulation results",
                         nargs='?',
                         default="run_time_factor.csv")
    args = parser.parse_args()
    main(args)
    