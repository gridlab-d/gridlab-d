'''
Created on Nov 7, 2014

@author: srid966
'''
import PNNL_Solar
import feeder_parse_mod
import random

inputStr = 'C:\Users\srid966\Desktop\My Work\California Solar Initiative\SCE Feeder Models\TESTS\House.glm'

triplex_data = PNNL_Solar.identify_triplex(inputStr)

Rossi_peak_load = 35952 #KW
Penetration = 0.10 #10%

for count in range(2):
    
    triplex_data_subset = random.sample(triplex_data, len(triplex_data)//3)
    
    solar_dict = PNNL_Solar.solar_create(triplex_data_subset, Rossi_peak_load, Penetration)
    
    feeder_str = feeder_parse_mod.sortedWrite(solar_dict)
    
    filename = 'C:\Users\srid966\Desktop\My Work\California Solar Initiative\SCE Feeder Models\TESTS\case_%d_PNNL_solar.glm'%(count)
    
    glm_file = open(filename,'w')
    glm_file.write(feeder_str)
    glm_file.close()
    
    triplex_data_subset = None
    solar_dict = None

print('success!')