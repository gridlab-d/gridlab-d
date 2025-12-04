"""
Created on 10/16/2026

This example shows how to wrap GridLAB-D to make it a HELICS federate.

*******************   FICITICIOUS API    *******************
This example uses a fictitious API that has not been implemented. This script 
was used as a reference for the development team to see the kind of 
functionality API users (who don't know GLD as well) might be looking for. 


TODO - Complete example

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import libgld
import helics as h




gld = libgld.GLD()
glm = gld.open_model_file("best_model_ever.json")
start_time = glm.clock["starttime"]
stop_time = glm.clock["stoptime"]




messages = gld.load_model(glm)

    


messages = gld.exit_gld()

    




