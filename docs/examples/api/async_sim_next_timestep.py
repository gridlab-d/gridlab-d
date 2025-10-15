"""
Created on 10/9/2026

This example shows how to use the async call to GridLAB-D when advancing
simulation time. Specific data is collected from the house objects at each
time step and shoved into a Pandas DataFrame.

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import libgld
import pandas as pd

gld = libgld.GLD()
glm = gld.open_model_file("best_model_ever.json")
start_time = glm.clock["starttime"]
stop_time = glm.clock["stoptime"]

# Get list of house objects for later use
house_list = glm.house.keys()

# Takes model/checkout loaded from file and loads it into the gldcore.
messages = gld.load_model(glm)

# Holds the data from all runs
df_all = pd.DataFrame()

# List of house object parameters of which I want to capture state
param_query_list = ["air_temperature", "hvac_load", "system_mode"]

model_data = {}

old_t = t = start_time # initial simulation time
messages, t = gld.sim_step(1)

if old_t <= stop_time:
    model_data["sim_time"] = old_t
    for house in house_list:
        for param in param_query_list:
            value = gld.get_parameter(house, param)
            model_data[house][param] = value

    # Advance GLD simulation to next timestep which, for this example, takes, say 1 second wall-clock time
    step_size = old_t - t
    old_t = t
    messages = gld.sim_step_async(step_size)
    
    # Put data into Pandas data frame
    new_df = pd.DataFrame(model_data)
    df_all = pd.concat([df_all, new_df])
    
    # Check to see if GLD has simulated the next time step    
    messages, t = gld.sim_step_async_complete()

messages = gld.exit_gld()

    




