"""
Created on 10/17/2026

This example shows how to implement interaction between GridLAB-D and MATLAB
during runtime. In this case, the MATLAB functionality is trivial (calculating
three-phase voltage) but any callable MATLAB function can be executed.


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import libgld
import helics as h
import csv
import logging
import argparse
import sys
import matlab.engine

# Setting up logging
logger = logging.getLogger(__name__)

# Setting up MATLAB
matlab = matlab.engine.start_matlab()

# Setting up GLD
gld = libgld.GLD()
glm = gld.open_model_file("best_model_ever.json")
start_time = glm.clock["starttime"]
stop_time = glm.clock["stoptime"]
sim_time = start_time

# Getting the name of the substation object; there should only be one
substation_name = glm.substations.keys()[0]["name"]

# Load the model into the gldcore
messages = gld.load_model(glm)

# Start simulation by advancing simulation 
while sim_time <= stop_time:
    
    # Advance GridLAB-D's simulation time
    gld.sim_step()

    # Collect all the data at the substation to measure the three-phase power
    # Each of these return a complex value
    v = gld.get_parameter(substation_name, "distribution_voltage_A")
    i = gld.get_parameter(substation_name, "distribution_current_A")
    s_a = matlab(v*i)
    v = gld.get_parameter(substation_name, "distribution_voltage_B")
    i = gld.get_parameter(substation_name, "distribution_current_B")
    s_b = matlab(v*i)
    v = gld.get_parameter(substation_name, "distribution_voltage_C")
    i = gld.get_parameter(substation_name, "distribution_current_C")
    s_c = matlab(v*i)

    s_total = matlab(s_a + s_b + s_c)

    # Compare to the measured value produced by GridLAB-D
    gld_s = gld.get_parameter(substation_name, "distribution_load")

    print(f"MATLAB power: {s_total}     GridLAB-D power:{gld_s}")

# We're done; close it up
messages = gld.exit_gld()
matlab.quit()


