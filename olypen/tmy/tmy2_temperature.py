#!/usr/bin/env python
# Import necessary modules

import sys, os, string
from numpy import *
from string import *
from math import *
from pylab import *
from time import *
from datetime import *

# Input and output files:

FILENAME_INPUT1='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/tmy/Olympia_original.txt'


FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/tmy/Olympia_temp.csv'



# Start processing ------------------------------------------------------------------------------------------------------------------------

def main():
    sys.stdout.write("start\n")

# Open the input files
    try:
        OLYMPIA_RAW=open(FILENAME_INPUT1,'r')
        
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INPUT1 +" for reading\nProgram terminated\n")
        sys.exit()


# Open the output files
    try:
        TEMP_TMY=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()  


#------------------------------------------------------

    raw_data1=[]
    for line in OLYMPIA_RAW.readlines()[1:]:
        raw_data1.append(line)
    OLYMPIA_RAW.close()

    temp=[]
    for line in raw_data1:
        temp.append(line[67:70]+'.'+line[70:71])

    for line in temp:
        TEMP_TMY.write(line+'\n')
   

                    
                        
                

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    