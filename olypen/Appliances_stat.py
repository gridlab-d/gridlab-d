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

FILENAME_INPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Appliances.txt'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Appliances_Stat.csv'

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
        APPLIANCES_STAT=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()  

#------------------------------------------------------

    raw_data=[]
    for line in RAW_DATA.readlines():
        raw_data.append(line.strip().split('\t'))
    RAW_DATA.close()

    appliances=[]
    for item in raw_data[0][1:]:
        appliances.append(item)

   
    data=[]
    for item in raw_data[1:]:
        data.append(item[1:])
    print data
    
    for i in range(len(appliances)):
        APPLIANCES_STAT.write(appliances[i]+'\n')
        temp=[]
        for item in data:
            temp.append(item[i])

        category=[]
        for item in temp:
            if item not in category:
                category.append(item)
       
        for item in category:

            count=0.0
            for item1 in temp:
                if item1==item:
                    count=count+1
            APPLIANCES_STAT.write(item+','+str(count)+'\n')

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    