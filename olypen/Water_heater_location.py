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

FILENAME_INPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Water_heater_location.txt'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Water_heater_location_Stat.csv'

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
        location=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()  

#------------------------------------------------------

    raw_data=[]
    for line in RAW_DATA.readlines()[1:]:
        raw_data.append(line.strip().split('\t'))

    RAW_DATA.close()

    locations=[]
    for line in raw_data:
        locations.append(line[1])

    location_category=[]
    for item in locations:
        if item not in location_category:
            location_category.append(item)
    print location_category
    
    for item in location_category:
        location.write(item+'\n')

    count1=0.0
    count2=0.0
    for item in locations:
        if item=='Garage':
            count1=count1+1
        elif item=='House and Garage':
            count1=count1+1
        else:
            count2=count2+1
    print count1, count2
    location.write('Garage,Inside\n')
    location.write(str(count1)+','+str(count2)+'\n')
    location.write(str(count1/(count1+count2))+','+str(count2/(count1+count2))+'\n')
    

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    