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
FILENAME_INPUT2='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/tmy/olympictmy.csv'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/tmy/Olympia.txt'
FILENAME_OUTPUT1='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/tmy/temperature_measure.csv'


# Start processing ------------------------------------------------------------------------------------------------------------------------

def main():
    sys.stdout.write("start\n")

# Open the input files
    try:
        OLYMPIA_RAW=open(FILENAME_INPUT1,'r')
        
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INPUT1 +" for reading\nProgram terminated\n")
        sys.exit()

    try:
        OLYMPIA_MEASURED=open(FILENAME_INPUT2,'r')
        
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INPUT2 +" for reading\nProgram terminated\n")
        sys.exit()

# Open the output files
    try:
        OLYMPIA_MEASURED_TMY=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()  

    try:
        TEMP_MEASURE=open(FILENAME_OUTPUT1,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT1 +"for writing\nProgram terminated\n")
        sys.exit() 
#------------------------------------------------------
    raw_data2=[]
    for line in OLYMPIA_MEASURED.readlines()[1:]:
        raw_data2.append(line.strip().split(','))
    OLYMPIA_MEASURED.close()

   
    hour=[]
    for i in range(24):
        hour.append(str(i)+':00')

    raw_data3=[]    
    for i in range(1,13):
        month=[]
        for line in raw_data2:
            item1=line[0]
            item2=item1.split(' ')[0]
            item3=item2.split('/')[0]
            
            if item3==str(i):
                month.append(line)


        day=[]
        for line0 in month:
            item111=line0[0]
            item222=item111.split(' ')[0]
            item333=item222.split('/')[0]
            item444=item222.split('/')[1]
            if item444 not in day:
                day.append(item444)

        
        for j in range(1,len(day)+1):
            date=[]
            for line1 in month:
                item5=line1[0]
                item6=item5.split(' ')[0]
                item7=item6.split('/')[1]
                if item7==str(j):
                    date.append(line1)
   
            hour_temp=[]
            hour_temp1=[]
            for line2 in date:
                item8=line2[0]
                item9=item8.split(' ')[1]
                hour_temp1.append(item9)
                if item9 in hour:
                    hour_temp.append(item9)

            
            if len(hour_temp)==24:
                for line3 in date:
                    item10=line3[0]
                    item11=item10.split(' ')[1]
                    if item11 in hour:
                        raw_data3.append(line3)


            if len(hour_temp)!=24:
                hour_temp3=[]
                for item12 in hour:
                    if item12 not in hour_temp:
                        hour_temp3.append(item12)
                print i,j,hour_temp3
                       
    print len(raw_data3)                        
                        
    raw_data1=[]
       
    for line in OLYMPIA_RAW.readlines()[1:]:
        raw_data1.append(line)
    OLYMPIA_RAW.close()

    
    print len(raw_data1)

    
    temperature=[]
    for line in raw_data3:
        temperature.append(line[2])
        TEMP_MEASURE.write(line[0]+','+line[2]+'\n')
            

    raw_data4=[]
    for i in range(len(raw_data1)):
        if temperature[i]=='--':
            raw_data4.append(raw_data1[i])
        else:
            if temperature[i][0]=='-':
                temp1=temperature[i][1:]
                if len(temp1)==1:
                    tempstr='-0'+temp1+'0'
                    newline=raw_data1[i][:67]+tempstr+raw_data1[i][71:]
                    raw_data4.append(newline)
                if len(temp1)>1:
                    if '.' in temp1:
                        tempstr1=temp1.split('.')[0]
                        tempstr2=temp1.split('.')[1]
                        if len(tempstr1)==1:
                            newstr1='-0'+tempstr1+tempstr2[0]
                            newline1=raw_data1[i][:67]+newstr1+raw_data1[i][71:]
                            raw_data4.append(newline1)

                        if len(tempstr1)>1:
                            newstr2='-'+tempstr1+tempstr2[0]
                            newline2=raw_data1[i][:67]+newstr2+raw_data1[i][71:]
                            raw_data4.append(newline2)
                    
                    else:
                        newstr3='-'+temp1+'0'
                        newline3=raw_data1[i][:67]+newstr3+raw_data1[i][71:]
                        raw_data4.append(newline3)

           
            else:
                temp11=temperature[i]
                if len(temp11)==1:
                    tempstr11='00'+temp11+'0'
                    newline4=raw_data1[i][:67]+tempstr11+raw_data1[i][71:]
                    raw_data4.append(newline4)

                if len(temp11)>1:
                    if '.' in temp11:
                        tempstr11=temp11.split('.')[0]
                        tempstr22=temp11.split('.')[1]
                        if len(tempstr11)==1:
                            newstr11='00'+tempstr11+tempstr22[0]
                            newline5=raw_data1[i][:67]+newstr11+raw_data1[i][71:]
                            raw_data4.append(newline5)

                        if len(tempstr11)>1:
                            newstr22='0'+tempstr11+tempstr22[0]
                            newline6=raw_data1[i][:67]+newstr22+raw_data1[i][71:]
                            raw_data4.append(newline6)                            
                      
    OLYMPIA_MEASURED_TMY.write(' 24227 OLYMPIA                WA  -8 N 46 58 W 122 54    61\n')
    for line in raw_data4:
        OLYMPIA_MEASURED_TMY.write(line)


                    
                        
                

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    