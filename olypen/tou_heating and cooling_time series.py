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
FILENAME_INTPUT='C:/Program Files/GridLAB-D/bin/tou_heating.txt'

# Output files:

FILENAME_OUTPUT='C:/Program Files/GridLAB-D/bin/touheating.csv'



# Start processing ------------------------------------------------------------------------------------------------------------------------
def main():
    sys.stdout.write("start\n")

    try:
        tou-heating=open(FILENAME_INTPUT,"r")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INTPUT +"for writing\nProgram terminated\n")
        sys.exit()

# Open the output files
    try:
        TSTAT_PATTERN=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()

# -----------------------------------------------------------------------------------------------------------------------  
    heatingspring=[]


    
    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    