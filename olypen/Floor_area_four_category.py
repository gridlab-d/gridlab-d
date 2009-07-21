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

FILENAME_INPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Floor_area.txt'

FILENAME_OUTPUT='C:/Documents and Settings/d3x175/My Documents/My work/Olypen/Floor_area_four_category.txt'

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
        fourgroup=open(FILENAME_OUTPUT,"w")
    except IOError:
        sys.stderr.write("Could not open " + FILENAME_OUTPUT +"for writing\nProgram terminated\n")
        sys.exit()  

#------------------------------------------------------

    raw_data=[]
    for line in RAW_DATA.readlines()[1:]:
        raw_data.append(line.strip().split('\t'))
    RAW_DATA.close()

    id=[]
    floor_area=[]
    for item in raw_data:
        id.append(item[0])
        floor_area.append(item[1])
   


    control=['239','294','366','434','471','531','532','546','551','554','559','575',
            '577','580','584','632','685','699','731','739','742','744','753','760','764']

    fixed=['233','293','316','347','358','362','401','440','443','462','469','547','556',
            '566','578','642','643','661','697','713','715','727','735','746','758','766',
            '688','776','432']

    TOU=['251','278','343','361','442','460','576','592','599','600','602','610','613',
        '617','629','636','644','652','669','690','730','737','754','755','761','767','611']

    RTP=['235','282','292','325','360','368','419','429','449','452','455','526','562','587',
        '588','594','596','622','635','638','653','666','671','734','738','747','756','499','533','459']    

    control_floor=[]
    fixed_floor=[]
    TOU_floor=[]
    RTP_floor=[]

    for i in range(len(id)):
        if id[i] in control:
            control_floor.append(floor_area[i])
        if id[i] in fixed:
            fixed_floor.append(floor_area[i])
        if id[i] in TOU:
            TOU_floor.append(floor_area[i])
        if id[i] in RTP:
            RTP_floor.append(floor_area[i])

    fourgroup.write('control\n')
    for item in control_floor:
        fourgroup.write(item+'\n')
    fourgroup.write('\nfixed\n')
    for item in fixed_floor:
        fourgroup.write(item+'\n')
    fourgroup.write('\nTOU\n')
    for item in TOU_floor:
        fourgroup.write(item+'\n')
    fourgroup.write('\nRTP\n')
    for item in RTP_floor:
        fourgroup.write(item+'\n')

    sys.stdout.write('Done!\n')
    return
if __name__ == '__main__':
    main()    