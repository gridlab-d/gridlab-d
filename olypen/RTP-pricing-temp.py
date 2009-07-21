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
FILENAME_INTPUT1='C:/Program Files/GridLAB-D/bin/clear.txt'
FILENAME_INTPUT2='C:/Program Files/GridLAB-D/bin/rtp_heating_temp_set.txt'

# Output files:

FILENAME_OUTPUT1='C:/Program Files/GridLAB-D/bin/rtp_clearing_price.csv'
FILENAME_OUTPUT2='C:/Program Files/GridLAB-D/bin/rtp_temp_set.csv'



# Start processing ------------------------------------------------------------------------------------------------------------------------
def main():
    sys.stdout.write("start\n")

    try:
        Clearing_price=open(FILENAME_INTPUT1,"r")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INTPUT1 +"for writing\nProgram terminated\n")
        sys.exit()

    try:
        temp_set=open(FILENAME_INTPUT2,"r")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INTPUT2 +"for writing\nProgram terminated\n")
        sys.exit()


# Open the output files
    try:
        rtp_price=open(FILENAME_OUTPUT1,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT1 +"for writing\nProgram terminated\n")
        sys.exit()
    try:
        rtp_tempset=open(FILENAME_OUTPUT2,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT2 +"for writing\nProgram terminated\n")
        sys.exit()

# -----------------------------------------------------------------------------------------------------------------------  

    raw_data=[]
    for line in Clearing_price.readlines():
        raw_data.append(line.split('\t')[1]+','+line.split('\t')[3])
    Clearing_price.close()


    raw_data1_time=[]
    raw_data2_time_price=[]
    
    for line in raw_data:
        if line.startswith('2006-03-31'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)

        if line.startswith('2006-04'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)
            
        if line.startswith('2006-05'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)
            
        if line.startswith('2006-06'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)

        if line.startswith('2006-07'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)
            
        if line.startswith('2006-08'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)
            
        if line.startswith('2006-09'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)
            
        if line.startswith('2006-10'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)
            
        if line.startswith('2006-11'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)
            
        if line.startswith('2006-12'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)
            
        if line.startswith('2007-01'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)
            
        if line.startswith('2007-02'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)
            
        if line.startswith('2007-03'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)
            
        if line.startswith('2007-04-01'):
            raw_data1_time.append(line.split(',')[0])
            raw_data2_time_price.append(line)

    print len(raw_data1_time), len(raw_data2_time_price)
    

    raw_data2_date=[]
    for line in raw_data1_time:
        if line.split(' ')[0] not in raw_data2_date:
            raw_data2_date.append(line.split(' ')[0])

    index1=raw_data2_date.index('2006-09-11')
    

    raw_data3_date=[]
    for line in raw_data2_date[:(raw_data2_date.index('2006-09-11')+1)]:
        raw_data3_date.append(line)

    raw_data3_date.append('2006-09-12')
    
    for line in raw_data2_date[(raw_data2_date.index('2006-09-11')+1):]:
        raw_data3_date.append(line)

    print len(raw_data3_date)    

    raw_data4_all_time=[]
    for line in raw_data3_date:
        for i in range(24):
            for j in range(12):
                if len(str(i))<2:
                    if len(str(j*5))<2:
                        raw_data4_all_time.append(line+' '+'0'+str(i)+':'+'0'+str(j*5)+':00')
                    if len(str(j*5))>1:
                        raw_data4_all_time.append(line+' '+'0'+str(i)+':'+str(j*5)+':00')
                if len(str(i))>1:
                    if len(str(j*5))<2:
                        raw_data4_all_time.append(line+' '+str(i)+':'+'0'+str(j*5)+':00')
                    if len(str(j*5))>1:
                        raw_data4_all_time.append(line+' '+str(i)+':'+str(j*5)+':00')

    print len(raw_data4_all_time) 

    temp1=[]
    for line in raw_data1_time:
        if line in raw_data4_all_time:
            temp1.append(line)


    tempset=[]
    for line in temp_set.readlines():
        tempset.append(line.strip())
    temp_set.close()

    tempset_weekend=['70','70','70','70','70','70','70','64','64','64','64',
                     '64','64','64','64','64','64','64','64','64','64','64','70','70']

    tempset_weekday=[]
    for line in tempset[:24]:
        tempset_weekday.append(line)

    tempset_weekend_5min=[]
    for i in range(24):
        for j in range(12):
            tempset_weekend_5min.append(tempset_weekend[i])

    tempset_week_5min=[]
    for i in range(24):
        for j in range(12):
            tempset_week_5min.append(tempset_weekday[i])

    
    tempset_week=[]
    for i in range(168):
        for j in range(12):
            tempset_week.append(tempset[i])
            
    tempsettotal=[]
            
    for item in tempset_week_5min:
        tempsettotal.append(item)
    for item in tempset_weekend_5min:
        tempsettotal.append(item)
    for item in tempset_weekend_5min:
        tempsettotal.append(item)


    for i in range(52):
        for item in tempset_week:
            tempsettotal.append(item)

    print len(tempsettotal)

    
    for line in temp1:
        rtp_price.write(raw_data2_time_price[raw_data1_time.index(line)]+','+tempsettotal[raw_data4_all_time.index(line)]+'\n')

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    