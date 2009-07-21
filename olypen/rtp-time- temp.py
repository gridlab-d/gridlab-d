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
FILENAME_INTPUT1='C:/Program Files/GridLAB-D/bin/rtp_clearing_price.csv'

# Output files:

FILENAME_OUTPUT1='C:/Program Files/GridLAB-D/bin/noreaction.csv'
FILENAME_OUTPUT2='C:/Program Files/GridLAB-D/bin/balance.csv'
FILENAME_OUTPUT3='C:/Program Files/GridLAB-D/bin/economy.csv'


# Start processing ------------------------------------------------------------------------------------------------------------------------
def main():
    sys.stdout.write("start\n")

    try:
        rtp_temp=open(FILENAME_INTPUT1,"r")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INTPUT1 +"for writing\nProgram terminated\n")
        sys.exit()




# Open the output files
    try:
        No_reaction=open(FILENAME_OUTPUT1,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT1 +"for writing\nProgram terminated\n")
        sys.exit()
        
    try:
        Balance=open(FILENAME_OUTPUT2,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT2 +"for writing\nProgram terminated\n")
        sys.exit()

    try:
        Economy=open(FILENAME_OUTPUT3,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT3 +"for writing\nProgram terminated\n")
        sys.exit()


# -----------------------------------------------------------------------------------------------------------------------  

    raw_data=[]
    for line in rtp_temp.readlines()[288:]:
        raw_data.append(line.strip().split(','))
    rtp_temp.close()

    for line in raw_data:
        date_time=line[0]
        date=date_time.split(' ')[0]
        time=date_time.split(' ')[1]

        date_year=date.split('/')[2]
        date_month=date.split('/')[1]
        if len(date_month)<2:
            date_month='0'+date_month
        date_date=date.split('/')[0]
        if len(date_date)<2:
            date_date='0'+date_date               
        date1=date_year+'-'+date_month+'-'+date_date

        time_hour=time.split(':')[0]
        time_min=time.split(':')[1]

        if len(time_hour)<2:
            time_hour='0'+time_hour
        time1=time_hour+':'+time_min+':00'

        new_date=date1+' '+time1

        No_reaction.write(new_date+','+line[6]+'\n')

        temp_balance=line[7]
        line7=atof(line[7])
        if line7>70 or line7<45:
            temp_balance='70'
        temp_econ=line[8]
        line8=atof(line[8])
        if line8>70 or line8<45:
            temp_econ='70'
        
        Balance.write(new_date+','+temp_balance+'\n')
        Economy.write(new_date+','+temp_econ+'\n')



    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    