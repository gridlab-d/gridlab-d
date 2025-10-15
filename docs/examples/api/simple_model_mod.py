"""
Created on 10/15/2026

This example shows how to modify an existing model prior to running it in
GridLAB-D. It also shows how to do the same modification while a model is 
running.

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import libgld
import pandas as pd

gld = libgld.GLD()
glm = gld.open_model_file("best_model_ever.json")
start_time = glm.clock["starttime"]
stop_time = glm.clock["stoptime"]
half_way = stop_time - start_time
step_size = 60
floor_size_changed = False
t = start_time

# Make list of houses
house_list = glm.houses.keys()

# Storing floor area
data_dict = {}

# Collect data prior to modification
data_dict["sim_time"] = start_time - 1
for house in house_list:
    data_dict[house] = glm.houses[house]["floor_area"]
all_data = pd.DataFrame(data_dict)

#Increase the floor area of every house by 10%
for house in house_list:
    glm.houses[house]["floor_area"] *= 1.1
    data_dict[house] = glm.houses[house]["floor_area"]
all_data = pd.concat([all_data, pd.DataFrame(data_dict)])


messages = gld.load_model(glm)

while t < stop_time:
    # Advance to next simulation time
    messages, t = gld.sim_step(t + step_size)

    # Half-way through the sim, increase the house size again
    if not floor_size_changed and t >= half_way:
        for house in house_list: # Number of houses is not changed
            old_floor_area = gld.get_parameter(house, "floor_area")
            gld.set_parameter(house, "floor_area", 1.1* old_floor_area)
        floor_size_changed = True
    
    # Collect data
    data_dict["sim_time"] = t
    for house in house_list: # Number of houses is not changed
        data_dict[house] = gld.get_parameter(house, "floor_area")
    all_data = pd.concat([all_data, pd.DataFrame(data_dict)])
        

messages = gld.exit_gld()

    




