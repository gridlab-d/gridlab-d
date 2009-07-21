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
FILENAME_INPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/cust_tstat_config.csv'
FILENAME_INPUT1='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/Control_PNNLID_INVENSYSID.txt'
FILENAME_INPUT2='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/Fixed_PNNLID_INVENSYSID.txt'
FILENAME_INPUT3='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/TOU_PNNLID_INVENSYSID.txt'
FILENAME_INPUT4='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/RTP_PNNLID_INVENSYSID.txt'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/TSTAT_ALL_FOUR_GROUP.csv'
FILENAME_OUTPUT1='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/TSTAT_Control.csv'
FILENAME_OUTPUT2='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/TSTAT_Fixed.csv'
FILENAME_OUTPUT3='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/TSTAT_TOU.csv'
FILENAME_OUTPUT4='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Database/TSTAT_RTP.csv'

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
        Control_ID=open(FILENAME_INPUT1,'r')
        
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INPUT1 +" for reading\nProgram terminated\n")
        sys.exit()

    try:
        Fixed_ID=open(FILENAME_INPUT2,'r')
        
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INPUT2 +" for reading\nProgram terminated\n")
        sys.exit()
   
    try:
        TOU_ID=open(FILENAME_INPUT3,'r')
        
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INPUT3 +" for reading\nProgram terminated\n")
        sys.exit()

    try:
        RTP_ID=open(FILENAME_INPUT4,'r')
        
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_INPUT4 +" for reading\nProgram terminated\n")
        sys.exit()

# Open the output files
    try:
        TSTAT_ALL_FOUR=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()

    try:
        TSTAT_Control=open(FILENAME_OUTPUT1,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT1 +"for writing\nProgram terminated\n")
        sys.exit()

    try:
        TSTAT_Fixed=open(FILENAME_OUTPUT2,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT2 +"for writing\nProgram terminated\n")
        sys.exit()
    
    try:
        TSTAT_TOU=open(FILENAME_OUTPUT3,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT3 +"for writing\nProgram terminated\n")
        sys.exit()

    try:
        TSTAT_RTP=open(FILENAME_OUTPUT4,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT4 +"for writing\nProgram terminated\n")
        sys.exit()
#------------------------------------------------------
    raw_data=[]
    raw_data_header=[]

    for line in RAW_DATA.readlines():
        raw_data.append(line.strip().split(','))
    RAW_DATA.close()
    
    for item in raw_data[0]:
        raw_data_header.append(item) 
  
    control_id=[]
    fixed_id=[]
    tou_id=[]
    rtp_id=[]
    all_id=[]
    for line in Control_ID.readlines()[1:]:
        control_id.append(line.strip().split('\t')[1])
        all_id.append(line.strip().split('\t')[1])
    for line in Fixed_ID.readlines()[1:]:
        fixed_id.append(line.strip().split('\t')[1])
        all_id.append(line.strip().split('\t')[1])
    for line in TOU_ID.readlines()[1:]:
        tou_id.append(line.strip().split('\t')[1])
        all_id.append(line.strip().split('\t')[1])
    for line in RTP_ID.readlines()[1:]:
        rtp_id.append(line.strip().split('\t')[1])
        all_id.append(line.strip().split('\t')[1])

    all=[]
    control=[]
    fixed=[]
    tou=[]
    rtp=[]
    for i in range(len(raw_data)):
        if raw_data[i][1] in all_id:
            all.append(raw_data[i])
        if raw_data[i][1] in control_id:
            control.append(raw_data[i])
        if raw_data[i][1] in fixed_id:
            fixed.append(raw_data[i])
        if raw_data[i][1] in tou_id:
            tou.append(raw_data[i])
        if raw_data[i][1] in rtp_id:
            rtp.append(raw_data[i])
#----------------------------------------------------------
    for item in raw_data_header:
        TSTAT_ALL_FOUR.write(item+',')
        TSTAT_Control.write(item+',')
        TSTAT_Fixed.write(item+',')
        TSTAT_TOU.write(item+',')
        TSTAT_RTP.write(item+',')

    TSTAT_ALL_FOUR.write('\n')
    TSTAT_Control.write('\n')
    TSTAT_Fixed.write('\n')
    TSTAT_TOU.write('\n')
    TSTAT_RTP.write('\n')

    for i in range(len(all)):
        for j in range(len(all[i])):
            TSTAT_ALL_FOUR.write(all[i][j]+',')
        TSTAT_ALL_FOUR.write('\n')

    for i in range(len(control)):
        for j in range(len(control[i])):
            TSTAT_Control.write(control[i][j]+',')
        TSTAT_Control.write('\n')    

    for i in range(len(fixed)):
        for j in range(len(fixed[i])):
            TSTAT_Fixed.write(fixed[i][j]+',')
        TSTAT_Fixed.write('\n')    

    for i in range(len(tou)):
        for j in range(len(tou[i])):
            TSTAT_TOU.write(tou[i][j]+',')
        TSTAT_TOU.write('\n')    

    for i in range(len(rtp)):
        for j in range(len(rtp[i])):
            TSTAT_RTP.write(rtp[i][j]+',')
        TSTAT_RTP.write('\n')    
    
    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    