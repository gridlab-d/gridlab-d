#!/usr/bin/env python
# Import necessary modules

import sys, os, string
from numpy import *
from string import *
from math import *
from pylab import *
from time import *
from datetime import *

# Input files:

FILENAME_INPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Water_heater_setpoint.txt'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Water_heater_setpoint_Stat.csv'

# Start processing ------------------------------------------------------------------------------------------------------------------------

def main():
    sys.stdout.write("start\n")

# Open the input files
    # Open State file
    try:
        RAW_DATA=open(FILENAME_INPUT,'r')
        
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INPUT +" for reading\nProgram terminated\n")
        sys.exit()

    try:
        setpoint=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()  

#------------------------------------------------------

    raw_data=[]
    for line in RAW_DATA.readlines()[1:]:
        raw_data.append(line.strip().split('\t'))
    RAW_DATA.close()

    id=[]    
    temp_setpoint_list=[]
    for line in raw_data:
        id.append(line[0])
        temp_setpoint_list.append(line[1])

    temp_temperature=[]
    for line in temp_setpoint_list:
        for letter in line:
            if letter=='(':
                index_left=line.index(letter)
            if letter==')':
                index_right=line.index(letter)
        temp_temperature.append(line[index_left+1:index_right])


    setpoint.write('ID#,Setpoint\n')
    for i in range(len(id)):
        setpoint.write(id[i]+','+temp_temperature[i]+'\n')
    
    temp_temperature1=[]
    for line in temp_temperature:
        if line!='':
            temp_temperature1.append(line)

    setpoint_category=[]
    for line in temp_temperature1:
        if line not in setpoint_category:
            setpoint_category.append(line)

    setpoint.write('\nSetpoint_category,Frequency\n')

    for item in setpoint_category:
        count=0.0
        for item1 in temp_temperature1:
            if item1==item:
                count=count+1
        setpoint.write(item+','+str(count)+'\n')
        
##    temp_daytime=[]
##    temp_nighttime=[]
##    
##    for line in raw_data:
##        temp_daytime.append(line[0])
##        temp_nighttime.append(line[1])
##
##    temp_list_daytime=[]
##    temp_list_nighttime=[]
##
##    for line in temp_daytime:
##        if line not in temp_list_daytime:
##            temp_list_daytime.append(line)
##            
##    for line in temp_nighttime:
##        if line not in temp_list_nighttime:
##            temp_list_nighttime.append(line)
##
##    Frequency.write('Daytime\n')
##    Frequency.write('Temperature'+','+'Frequency'+'\n')
##    for item in temp_list_daytime:
##        count=0.0
##        for item1 in temp_daytime:
##            if item1==item:
##                count=count+1      
##        Frequency.write(item+','+str(count)+'\n')
##
##    Frequency.write('\n\nNighttime\n')
##    Frequency.write('Temperature'+','+'Frequency'+'\n')
##    for item in temp_list_nighttime:
##        count=0.0
##        for item1 in temp_nighttime:
##            if item1==item:
##                count=count+1      
##        Frequency.write(item+','+str(count)+'\n')
    

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    