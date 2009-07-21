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

FILENAME_INPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/cust_device.csv'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/cust_device_ID_OUTPUT.csv'

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
        device_id=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()  

#------------------------------------------------------

    raw_data=[]
    for line in RAW_DATA.readlines()[2:]:
        raw_data.append(line.strip().split(','))

    RAW_DATA.close()

    device_type_code=[]
    invensys_device_type_id=[]
    for line in raw_data:
        device_type_code.append(line[3])
        invensys_device_type_id.append(line[4])

    deviceTYPE=[]
    deviceID=[]
    for item in device_type_code:
        if item not in deviceTYPE:
            deviceTYPE.append(item)

    for item in invensys_device_type_id:
        if item not in deviceID:
            deviceID.append(item)

    device_id.write('deviceTYPE,deviceID\n')
    for i in range(len(deviceTYPE)):
        device_id.write(deviceTYPE[i]+','+deviceID[i]+'\n')

 

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    