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

FILENAME_INPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Recal_winter_T.txt'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Recal_Frequency_Winter_T_Stat.csv'

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

#------------------------------------------------------

    raw_data=[]
    for line in RAW_DATA.readlines()[1:]:
        raw_data.append(line.strip().split('\t'))

    RAW_DATA.close()

    temp_daytime=[]
    temp_nighttime=[]
    
    for line in raw_data:
        temp_daytime.append(line[0])
        temp_nighttime.append(line[1])

    temp=[]
    for i in range(10,100):
        temp.append(str(i))
        
    temp1=[]
    for item in temp_daytime:
        if item not in temp:
            if item not in temp1:
                temp1.append(item)
    for item in temp_nighttime:
        if item not in temp:
            if item not in temp1:
                temp1.append(item)

    for item in raw_data:
        if item[0] in temp1:
            raw_data.remove(item)


    for item in raw_data:
        if item[1] in temp1:
            raw_data.remove(item)


    temp2=[]
    for i in range(10,100):
        temp2.append(str(i))

     

    temp3=[]
    for item in raw_data:
        if item[0] not in temp2:
            temp3.append(item[0])
    for item in raw_data:
        if item[1] not in temp2:
            temp3.append(item[1])


    for item in raw_data:
        if item[1] in temp3:
            raw_data.remove(item)


    average_temp=[]
    for item in raw_data:
        day=atof(item[0])
        night=atof(item[1])
        average=(day+night)/2.0
        average_temp.append(str(average))
    print average_temp       

    temp_category=[]
    for item in average_temp:
        if item not in temp_category:
            temp_category.append(item)

    Frequency.write('Temperature,Frequency\n')
    for item in temp_category:
        count=0.0
        for item1 in average_temp:
            if item1==item:
                count=count+1        
        Frequency.write(item+','+str(count)+','+str(count/len(average_temp))+'\n')

    Frequency.write('\n\n')
    Frequency.write('Average_Temp_list\n')
    for item in average_temp:
        Frequency.write(item+'\n')

##
##    Frequency.write('Daytime\n')
##    Frequency.write('Temperature'+','+'Frequency'+'\n')
##    for item in temp_list_daytime:
##        count=0.0
##        for item1 in temp_daytime:
##            if item1==item:
##                count=count+1      
##        Frequency.write(item+','+str(count)+'\n')
##
##    Frequency.write('\n\nNighttime\n')
##    Frequency.write('Temperature'+','+'Frequency'+'\n')
##    for item in temp_list_nighttime:
##        count=0.0
##        for item1 in temp_nighttime:
##            if item1==item:
##                count=count+1      
##        Frequency.write(item+','+str(count)+'\n')
    

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    