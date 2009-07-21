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

FILENAME_INPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Winter_T_Stat.txt'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Frequency_Winter_T_Stat.csv'

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
        Frequency=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()  

#------------------------------------------------------

    raw_data=[]
    for line in RAW_DATA.readlines()[1:]:
        raw_data.append(line.strip().split('\t'))

    RAW_DATA.close()

    temp_daytime=[]
    temp_nighttime=[]
    
    for line in raw_data:
        temp_daytime.append(line[0])
        temp_nighttime.append(line[1])

    temp_list_daytime=[]
    temp_list_nighttime=[]

    for line in temp_daytime:
        if line not in temp_list_daytime:
            temp_list_daytime.append(line)
            
    for line in temp_nighttime:
        if line not in temp_list_nighttime:
            temp_list_nighttime.append(line)

    Frequency.write('Daytime\n')
    Frequency.write('Temperature'+','+'Frequency'+'\n')
    for item in temp_list_daytime:
        count=0.0
        for item1 in temp_daytime:
            if item1==item:
                count=count+1      
        Frequency.write(item+','+str(count)+'\n')

    Frequency.write('\n\nNighttime\n')
    Frequency.write('Temperature'+','+'Frequency'+'\n')
    for item in temp_list_nighttime:
        count=0.0
        for item1 in temp_nighttime:
            if item1==item:
                count=count+1      
        Frequency.write(item+','+str(count)+'\n')
    

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    