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

FILENAME_INPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Survey-Sally.txt'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/olypen_survey.txt'

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
        Survey=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()  

#------------------------------------------------------

    raw_data=[]
    for line in RAW_DATA.readlines():
        raw_data.append(line.strip().split('\t'))

    RAW_DATA.close()


    temp=[]
    for i in range(len(raw_data)):
        if raw_data[i][1]=='C':
            temp.append(raw_data[i])
        if raw_data[i][1]=='P':
            temp.append(raw_data[i])

    for i in range(len(raw_data[0])):
        Survey.write(raw_data[0][i]+'\t')
    Survey.write('\n')
    for i in range(len(temp)):
        for j in range(len(raw_data[1])):
            Survey.write(temp[i][j]+'\t')
        Survey.write('\n')
        

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    