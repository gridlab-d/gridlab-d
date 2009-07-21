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

FILENAME_INPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Floor_area.txt'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Floor_area_Stat.txt'

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
        Stat=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()  

#------------------------------------------------------

    raw_data=[]
    for line in RAW_DATA.readlines()[1:]:
        raw_data.append(line.strip().split('\t'))

    RAW_DATA.close()

    temp_area=[]    
    for line in raw_data:
        temp_area.append(line[1])

    temp_category=[]
    for item in temp_area:
        if item not in temp_category:
            if item!='Blank' and item!='blank': 
                temp_category.append(item)
##    print len(temp_category)

    Stat.write('Category\tFrequency\n')    
    for item in temp_category:
        count=0.0
        for item1 in temp_area:
            if item1==item:
                count=count+1
        Stat.write(item+'\t'+str(count)+'\n')

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    