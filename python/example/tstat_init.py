#!/anaconda/bin/python
import os
import gridlabd
import random

gridlabd.set('show_progress','FALSE')
#
# Get a list of houses
#
houses = gridlabd.find('class=house')
houselist = [];
# 
# This command is required to disable the internal house thermostat
#
for house in houses :
	name = house['name']
	gridlabd.set(name,'thermostat_control','NONE')
	houselist.append(name) 
	
gridlabd.set('houselist',';'.join(houselist))
