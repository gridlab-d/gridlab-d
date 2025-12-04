"""
Created on 10/15/2026

This example shows the simplest way of running a GridLAB-D model, like users
would do without the API.

*******************   FICITICIOUS API    *******************
This example uses a fictitious API that has not been implemented. This script 
was used as a reference for the development team to see the kind of 
functionality API users (who don't know GLD as well) might be looking for. 


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import libgld


gld = libgld.GLD()
glm = gld.open_model_file("best_model_ever.json")

# Takes model/checkout loaded from file and loads it into the gldcore.
messages = gld.load_model(glm)

# Run the model
messages = gld.sim_run()

# Exit the simulation once complete
messages = gld.exit_gld()

    




