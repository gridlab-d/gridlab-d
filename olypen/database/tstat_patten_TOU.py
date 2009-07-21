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
FILENAME_INPUT1='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/TSTAT_TOU.csv'

# Output files:
FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/TSTAT_WEEKLY_PATTERN_TOU.csv'

# Start processing ------------------------------------------------------------------------------------------------------------------------
def main():
    sys.stdout.write("start\n")

# Open the input files
    try:
        RAW_TOU=open(FILENAME_INPUT1,'r')
        
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INPUT1 +" for reading\nProgram terminated\n")
        sys.exit()

# Open the output files
    try:
        TSTAT_PATTERN=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()

# Control mode------------------------------------------------------------  
    raw_data_tou=[]
    for line in RAW_TOU.readlines()[1:]:
        raw_data_tou.append(line.strip().split(','))
    RAW_TOU.close()
   
    invensys_acct_id_tou=[]
    occ_mode_id_tou=[]
    cooling_temp_set_tou=[]
    heating_temp_set_tou=[]
    
    for item in raw_data_tou:
        invensys_acct_id_tou.append(item[1])
        occ_mode_id_tou.append(item[4])
        cooling_temp_set_tou.append(item[5])
        heating_temp_set_tou.append(item[8])

    IVEN_ID_TOU=[]
    for item in invensys_acct_id_tou:
        if item not in IVEN_ID_TOU:
            IVEN_ID_TOU.append(item)


    cooling_temp_high=[]
    cooling_temp_mid=[]
    cooling_temp_low=[]
    
    heating_temp_high=[]
    heating_temp_mid=[]
    heating_temp_low=[]

    for item in IVEN_ID_TOU:
        cooling_temp_temp=[]
        heating_temp_temp=[]
        for i in range(len(invensys_acct_id_tou)):
            if invensys_acct_id_tou[i]==item:
                if cooling_temp_set_tou[i] not in cooling_temp_temp:
                    cooling_temp_temp.append(cooling_temp_set_tou[i])
                if heating_temp_set_tou[i] not in heating_temp_temp:
                    heating_temp_temp.append(heating_temp_set_tou[i])

        matrix=zeros(len(cooling_temp_temp))
        matrix1=zeros(len( heating_temp_temp))

                    
        for i in range(len(cooling_temp_temp)):
            matrix[i]=atof(cooling_temp_temp[i])
        cooling_temp_high.append(str(max(matrix)))
        cooling_temp_mid.append(str(median(matrix)))
        cooling_temp_low.append(str(min(matrix)))

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

    TSTAT_PATTERN.write('TOU\nTIME')
    for i in range(7):
        for j in range(24):
            TSTAT_PATTERN.write(','+str(j))
            
    TSTAT_PATTERN.write('\nFall/Winter/Spring')
    TSTAT_PATTERN.write('\nCooling')
    
    for j in range(5):
        for i in range(7):
            TSTAT_PATTERN.write(','+str(cooling_low_average))
        for i in range(7,17):
            TSTAT_PATTERN.write(','+str(cooling_high_average))
        for i in range(17,18):
            TSTAT_PATTERN.write(','+str(cooling_low_average))
        for i in range(18,22):
            TSTAT_PATTERN.write(','+str(cooling_high_average))
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
        for i in range(7,17):
            TSTAT_PATTERN.write(','+str(heating_low_average))
        for i in range(17,18):
            TSTAT_PATTERN.write(','+str(heating_high_average))
        for i in range(18,22):
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

    TSTAT_PATTERN.write('\nSummur')
    TSTAT_PATTERN.write('\nCooling')
    
    for j in range(5):
        for i in range(7):
            TSTAT_PATTERN.write(','+str(cooling_low_average))
        for i in range(7,17):
            TSTAT_PATTERN.write(','+str(cooling_high_average))
        for i in range(17,18):
            TSTAT_PATTERN.write(','+str(cooling_low_average))
        for i in range(18,22):
            TSTAT_PATTERN.write(','+str(cooling_high_average))
        for i in range(22,24):
            TSTAT_PATTERN.write(','+str(cooling_low_average))    

    TSTAT_PATTERN.write('\nHeating')
    for j in range(5):
        for i in range(7):
            TSTAT_PATTERN.write(','+str(heating_high_average))
        for i in range(7,17):
            TSTAT_PATTERN.write(','+str(heating_low_average))
        for i in range(17,18):
            TSTAT_PATTERN.write(','+str(heating_high_average))
        for i in range(18,22):
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

    
    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    