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
FILENAME_INPUT1='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/TSTAT_Control.csv'
FILENAME_INPUT2='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/TSTAT_Fixed.csv'

# Output files:
FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/TSTAT_WEEKLY_PATTERN.csv'

# Start processing ------------------------------------------------------------------------------------------------------------------------
def main():
    sys.stdout.write("start\n")

# Open the input files
    try:
        RAW_CONTROL=open(FILENAME_INPUT1,'r')
        
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INPUT1 +" for reading\nProgram terminated\n")
        sys.exit()

    try:
        RAW_FIXED=open(FILENAME_INPUT2,'r')
        
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INPUT2 +" for reading\nProgram terminated\n")
        sys.exit()

# Open the output files
    try:
        TSTAT_PATTERN=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()

# Control mode------------------------------------------------------------  
    raw_data_control=[]
    for line in RAW_CONTROL.readlines()[1:]:
        raw_data_control.append(line.strip().split(','))
    RAW_CONTROL.close()
 
    invensys_acct_id_control=[]
    occ_mode_id_control=[]
    cooling_temp_set_control=[]
    heating_temp_set_control=[]
    for item in raw_data_control:
        invensys_acct_id_control.append(item[1])
        occ_mode_id_control.append(item[4])
        cooling_temp_set_control.append(item[5])
        heating_temp_set_control.append(item[8])

    IVEN_ID_CONTROL=[]
    for item in invensys_acct_id_control:
        if item not in IVEN_ID_CONTROL:
            IVEN_ID_CONTROL.append(item)

    cooling_temp_high=[]
    cooling_temp_mid=[]
    cooling_temp_low=[]
    
    heating_temp_high=[]
    heating_temp_mid=[]
    heating_temp_low=[]

    for item in IVEN_ID_CONTROL:
        index1=invensys_acct_id_control.index(item)

        cooling_temp_temp=[]
        for item1 in cooling_temp_set_control[index1:(index1+8)]:
            if item1 not in cooling_temp_temp:
                cooling_temp_temp.append(item1)
      
        matrix=zeros(len(cooling_temp_temp))
        for i in range(len(cooling_temp_temp)):
            matrix[i]=atof(cooling_temp_temp[i])
        cooling_temp_high.append(str(max(matrix)))
        cooling_temp_mid.append(str(median(matrix)))
        cooling_temp_low.append(str(min(matrix)))
               

        heating_temp_temp=[]
        for item2 in heating_temp_set_control[index1:(index1+8)]:
            if item2 not in  heating_temp_temp:
                 heating_temp_temp.append(item2)
        matrix1=zeros(len( heating_temp_temp))
        for i in range(len( heating_temp_temp)):
            matrix1[i]=atof( heating_temp_temp[i])
        heating_temp_high.append(str(max(matrix1)))
        heating_temp_mid.append(str(median(matrix1)))
        heating_temp_low.append(str(min(matrix1)))
 
    matrix_cooling_high=zeros(len(cooling_temp_high))
    matrix_cooling_mid=zeros(len(cooling_temp_mid))
    matrix_cooling_low=zeros(len(cooling_temp_low))
    matrix_heating_high=zeros(len(heating_temp_high))
    matrix_heating_mid=zeros(len(heating_temp_mid))
    matrix_heating_low=zeros(len(heating_temp_low))

    for i in range(len(cooling_temp_high)):
        matrix_cooling_high[i]=atof(cooling_temp_high[i])
        matrix_cooling_mid[i]=atof(cooling_temp_mid[i])
        matrix_cooling_low[i]=atof(cooling_temp_low[i])
        matrix_heating_high[i]=atof(heating_temp_high[i])
        matrix_heating_mid[i]=atof(heating_temp_mid[i])
        matrix_heating_low[i]=atof(heating_temp_low[i])

    cooling_high_average=round(average(matrix_cooling_high))
    cooling_mid_average=round(average(matrix_cooling_mid))
    cooling_low_average=round(average(matrix_cooling_low))

    heating_high_average=round(average(matrix_heating_high))
    heating_mid_average=round(average(matrix_heating_mid))
    heating_low_average=round(average(matrix_heating_low))

    print cooling_high_average, cooling_mid_average, cooling_low_average
    print heating_high_average, heating_mid_average, heating_low_average

    TSTAT_PATTERN.write('CONTROL\nTIME')
    for i in range(7):
        for j in range(24):
            TSTAT_PATTERN.write(','+str(j))
    TSTAT_PATTERN.write('\nCooling')
    for j in range(5):
        for i in range(7):
            TSTAT_PATTERN.write(','+str(cooling_low_average))
        for i in range(7,9):
            TSTAT_PATTERN.write(','+str(cooling_mid_average))
        for i in range(9,17):
            TSTAT_PATTERN.write(','+str(cooling_high_average))
        for i in range(17,22):
            TSTAT_PATTERN.write(','+str(cooling_mid_average))
        for i in range(22,24):
            TSTAT_PATTERN.write(','+str(cooling_low_average))
    for j in range(5,7):
        for i in range(7):
            TSTAT_PATTERN.write(','+str(cooling_low_average))      
        for i in range(7,22):
            TSTAT_PATTERN.write(','+str(cooling_mid_average))        
        for i in range(22,24):
            TSTAT_PATTERN.write(','+str(cooling_low_average))    


    TSTAT_PATTERN.write('\nHeating')
    for j in range(5):
        for i in range(7):
            TSTAT_PATTERN.write(','+str(heating_high_average))
        for i in range(7,9):
            TSTAT_PATTERN.write(','+str(heating_mid_average))
        for i in range(9,17):
            TSTAT_PATTERN.write(','+str(heating_low_average))
        for i in range(17,22):
            TSTAT_PATTERN.write(','+str(heating_mid_average))
        for i in range(22,24):
            TSTAT_PATTERN.write(','+str(heating_high_average))
    for j in range(5,7):
        for i in range(7):
            TSTAT_PATTERN.write(','+str(heating_high_average))      
        for i in range(7,22):
            TSTAT_PATTERN.write(','+str(heating_mid_average))        
        for i in range(22,24):
            TSTAT_PATTERN.write(','+str(heating_high_average))    

        
#Fixed-------------------------------------------------------------------
    raw_data_fiexed=[]
    for line in RAW_FIXED.readlines()[1:]:
        raw_data_fiexed.append(line.strip().split(','))
    RAW_FIXED.close()

    invensys_acct_id_fixed=[]
    occ_mode_id_fixed=[]
    cooling_temp_set_fixed=[]
    heating_temp_set_fixed=[]

    for item in raw_data_fiexed:
        invensys_acct_id_fixed.append(item[1])
        occ_mode_id_fixed.append(item[4])
        cooling_temp_set_fixed.append(item[5])
        heating_temp_set_fixed.append(item[8])
        
    IVEN_ID_FIXED=[]
    for item in invensys_acct_id_fixed:
        if item not in IVEN_ID_FIXED:
            IVEN_ID_FIXED.append(item)

    fixed_cooling_temp_high=[]
    fixed_cooling_temp_mid=[]
    fixed_cooling_temp_low=[]
    
    fixed_heating_temp_high=[]
    fixed_heating_temp_mid=[]
    fixed_heating_temp_low=[]


    for item in IVEN_ID_FIXED:
        index1=invensys_acct_id_fixed.index(item)

        fixed_cooling_temp_temp=[]
        for item1 in cooling_temp_set_fixed[index1:(index1+8)]:
            if item1 not in fixed_cooling_temp_temp:
                fixed_cooling_temp_temp.append(item1)
      
        matrix=zeros(len(fixed_cooling_temp_temp))
        for i in range(len(fixed_cooling_temp_temp)):
            matrix[i]=atof(fixed_cooling_temp_temp[i])
        fixed_cooling_temp_high.append(str(max(matrix)))
        fixed_cooling_temp_mid.append(str(median(matrix)))
        fixed_cooling_temp_low.append(str(min(matrix)))
               

        fixed_heating_temp_temp=[]
        for item2 in heating_temp_set_fixed[index1:(index1+8)]:
            if item2 not in  fixed_heating_temp_temp:
                 fixed_heating_temp_temp.append(item2)
        matrix1=zeros(len( fixed_heating_temp_temp))
        for i in range(len( fixed_heating_temp_temp)):
            matrix1[i]=atof( fixed_heating_temp_temp[i])
        fixed_heating_temp_high.append(str(max(matrix1)))
        fixed_heating_temp_mid.append(str(median(matrix1)))
        fixed_heating_temp_low.append(str(min(matrix1)))

    matrix_fixed_cooling_high=zeros(len(fixed_cooling_temp_high))
    matrix_fixed_cooling_mid=zeros(len(fixed_cooling_temp_mid))
    matrix_fixed_cooling_low=zeros(len(fixed_cooling_temp_low))
    matrix_fixed_heating_high=zeros(len(fixed_heating_temp_high))
    matrix_fixed_heating_mid=zeros(len(fixed_heating_temp_mid))
    matrix_fixed_heating_low=zeros(len(fixed_heating_temp_low))

    for i in range(len(fixed_cooling_temp_high)):
        matrix_fixed_cooling_high[i]=atof(fixed_cooling_temp_high[i])
        matrix_fixed_cooling_mid[i]=atof(fixed_cooling_temp_mid[i])
        matrix_fixed_cooling_low[i]=atof(fixed_cooling_temp_low[i])
        matrix_fixed_heating_high[i]=atof(fixed_heating_temp_high[i])
        matrix_fixed_heating_mid[i]=atof(fixed_heating_temp_mid[i])
        matrix_fixed_heating_low[i]=atof(fixed_heating_temp_low[i])

    fixed_cooling_high_average=round(average(matrix_fixed_cooling_high))
    fixed_cooling_mid_average=round(average(matrix_fixed_cooling_mid))
    fixed_cooling_low_average=round(average(matrix_fixed_cooling_low))

    fixed_heating_high_average=round(average(matrix_fixed_heating_high))
    fixed_heating_mid_average=round(average(matrix_fixed_heating_mid))
    fixed_heating_low_average=round(average(matrix_fixed_heating_low))


    TSTAT_PATTERN.write('\nFIXED')
    
    TSTAT_PATTERN.write('\nCooling')
    for j in range(5):
        for i in range(7):
            TSTAT_PATTERN.write(','+str(fixed_cooling_low_average))
        for i in range(7,9):
            TSTAT_PATTERN.write(','+str(fixed_cooling_mid_average))
        for i in range(9,17):
            TSTAT_PATTERN.write(','+str(fixed_cooling_high_average))
        for i in range(17,22):
            TSTAT_PATTERN.write(','+str(fixed_cooling_mid_average))
        for i in range(22,24):
            TSTAT_PATTERN.write(','+str(fixed_cooling_low_average))
    for j in range(5,7):
        for i in range(7):
            TSTAT_PATTERN.write(','+str(fixed_cooling_low_average))      
        for i in range(7,22):
            TSTAT_PATTERN.write(','+str(fixed_cooling_mid_average))        
        for i in range(22,24):
            TSTAT_PATTERN.write(','+str(fixed_cooling_low_average)) 
        

    TSTAT_PATTERN.write('\nHeating')
    for j in range(5):
        for i in range(7):
            TSTAT_PATTERN.write(','+str(fixed_heating_high_average))
        for i in range(7,9):
            TSTAT_PATTERN.write(','+str(fixed_heating_mid_average))
        for i in range(9,17):
            TSTAT_PATTERN.write(','+str(fixed_heating_low_average))
        for i in range(17,22):
            TSTAT_PATTERN.write(','+str(fixed_heating_mid_average))
        for i in range(22,24):
            TSTAT_PATTERN.write(','+str(fixed_heating_high_average))
    for j in range(5,7):
        for i in range(7):
            TSTAT_PATTERN.write(','+str(fixed_heating_high_average))      
        for i in range(7,22):
            TSTAT_PATTERN.write(','+str(fixed_heating_mid_average))        
        for i in range(22,24):
            TSTAT_PATTERN.write(','+str(fixed_heating_high_average)) 

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    