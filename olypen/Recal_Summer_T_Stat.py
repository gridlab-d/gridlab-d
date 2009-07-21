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

FILENAME_INPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Recal_summer.txt'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Recal_Frequency_Summer_T_Stat.csv'

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


    for item in raw_data:
        if item[1]=='Blank':
            raw_data.remove(item)

    AC=[]
    AC_Category=[]
    for item in raw_data:
        AC.append(item[1])

    for item in AC:
        if item not in AC_Category:
            AC_Category.append(item)

    Frequency.write('Category,Frequency,Percentagex100%\n')

    for item in AC_Category:
        count=0.0
        for item1 in AC:
            if item1==item:
                count=count+1
        Frequency.write(item+','+str(count)+','+str(count/len(AC))+'\n')

    for item in raw_data:
        if item[1]=='None':
            raw_data.remove(item)

    for item in raw_data:
        if item[1]=='None':
            raw_data.remove(item)

    for item in raw_data:
        if item[1]=='None':
            raw_data.remove(item)

         
    use_frequency=[]
    for item in raw_data:
        use_frequency.append(item[2])
   
    use_freq_category=[]
    for item in use_frequency:
        if item not in use_freq_category:
            use_freq_category.append(item)

    Frequency.write('Use,Freqeuncy\n')    
    for item in use_freq_category:
        count=0.0
        for item1 in use_frequency:
            if item1==item:
                count=count+1
        Frequency.write(item+','+str(count)+','+str(count/len(use_frequency))+'\n')

       
    for item in raw_data:
        if item[2]=='Never':
            raw_data.remove(item)

    for item in raw_data:
        if item[2]=='Never':
            raw_data.remove(item)


    temp=[]
    for i in range(20,90):
        temp.append(str(i))


    for item in raw_data:
        if item[3] not in temp:
            raw_data.remove(item)
    for item in raw_data:
        if item[4] not in temp:
            raw_data.remove(item)


    temp_average=[]
    for item in raw_data:
        daytime=atof(item[3])
        nighttime=atof(item[4])
        average=(daytime+nighttime)/2.0
        temp_average.append(str(average))


    temp_ave_categ=[]

    for item in temp_average:
        if item not in temp_ave_categ:
            temp_ave_categ.append(item)

    Frequency.write('Temp,Freq,Percentagex100%\n')
    for item in temp_ave_categ:
        count=0.0
        for item1 in temp_average:
            if item1==item:
                count=count+1
        
        Frequency.write(item+','+str(count)+','+str(count/len(temp_average))+'\n')

    Frequency.write('\n\nAverage_tem\n')
    for item in temp_average:
        Frequency.write(item+'\n')
    
    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    