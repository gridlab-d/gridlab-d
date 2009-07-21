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

FILENAME_INPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Summer_T_Stat.txt'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Frequency_Summer_T_Stat.csv'
FILENAME_OUTPUT1='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Summer_T_Stat_Clean.csv'

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

    try:
        Clean_data=open(FILENAME_OUTPUT1,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT1 +"for writing\nProgram terminated\n")
        sys.exit()

#------------------------------------------------------

    raw_data=[]
    for line in RAW_DATA.readlines():
        raw_data.append(line.strip().split('\t'))

    RAW_DATA.close()

    temp_daytime=[]
    temp_nighttime=[]
    
    for line in raw_data:
        temp_daytime.append(line[0])
        temp_nighttime.append(line[1])

    TEMP_LIST=[]
    for i in range(40,90):
        TEMP_LIST.append(str(i))

    Clean_data.write('Daytime\n')    
    for item in temp_daytime:
        if item in TEMP_LIST:
            Clean_data.write(item+'\n')
    
    Clean_data.write('\nnighttime\n')
    for item in temp_nighttime:
        if item in TEMP_LIST:
            Clean_data.write(item+'\n')            

    Frequency.write('Daytime\n')
    Frequency.write('Temperature'+','+'Frequency'+'\n')

    for i in range(40,90):
        count=0.0
        item=str(i)
        for item1 in temp_daytime:
            if item1==item:
                count=count+1
        Frequency.write(item+','+str(count)+'\n')
        

    Frequency.write('\n\nNighttime\n')
    Frequency.write('Temperature'+','+'Frequency'+'\n')
    for i in range(40,90):
        count=0.0
        item=str(i)
        for item1 in temp_nighttime:
            if item1==item:
                count=count+1     
        Frequency.write(item+','+str(count)+'\n')



    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    