'''
Created on Nov 7, 2014

@author: srid966
'''
import feeder_parse_mod
import Inverter_add_glm_object_dictionary
import csv
import os
import random
import fnmatch
import shutil
import string
import re


def residential_data(inputStr):

    bins = ['1', '2', '3', '4', '5', '6']
    
    sorted_triplex = {}
    sorted_triplex['FALSE'] = dict.fromkeys(bins)
    for key in sorted_triplex['FALSE'].keys():
        sorted_triplex['FALSE'][key] = []
    
    sorted_triplex['TRUE'] = dict.fromkeys(bins)
    for key in sorted_triplex['TRUE'].keys():
        sorted_triplex['TRUE'][key] = []
    
    triplex_meter_data = {}
    triplex_meter_house_map = {}
             
    with open(inputStr) as f:
        for line in f:
            if 'object triplex_meter' in line:
                a = next(f)
                triplex_name = a[12:-2]
            
            if 'nominal_voltage' in line:
                nominal_voltage = line[23:-2]
                
            if 'phases' in line:
                phases = line[14:-2]                               
                  
            if 'object house' in line:                
                a = next(f)
                house_name = a[12:-2]
                  
            if '//gcd_CARE' in line:
                care = line[18:-2]
                  
            if '//gcd_usagebin' in line:
                bin_no = line[22:-2]
                sorted_triplex[care][bin_no].append(triplex_name)    
                triplex_meter_data[triplex_name]= [phases, nominal_voltage]
                
#                 if triplex_name in triplex_meter_house_map.keys():               
#                     triplex_meter_house_map[triplex_name].append(house_name)
#                 else:
#                     triplex_meter_house_map[triplex_name].append(house_name)
                        
        for key in sorted_triplex['FALSE'].keys():
            if not sorted_triplex['FALSE'][key]:
                del sorted_triplex['FALSE'][key]
                
        for key in sorted_triplex['TRUE'].keys():
            if not sorted_triplex['TRUE'][key]:
                del sorted_triplex['TRUE'][key]   

    return sorted_triplex, triplex_meter_data


def PV_residential_allocation(feeder_name, sorted_triplex, penetration_kVA, triplex_meter_data):
    
    PV_residential_kW = {'Cawston': 0.91, 'Miles': 0.57, 'Caldwell': 0.88, 'Wildwood': 0.93, 'Mallet': 0.93,\
                          'Diana': 0.59, 'Major': 0.99, 'Homer': 0.44, 'Mirror': 0.83, 'Moran': 0.62, 'Price': 0.60,\
                          'Rossi': 0.79, 'Parrot': 0.02, 'Solitaire': 0, 'Doerner': 0, 'Etting': 0.10}
    
    PV_residential_weights = {'TRUE': {'1':{'weight': 0, 'size': 1}, '2': {'weight': 0.0046, 'size': 2}, '3': {'weight': 0.0046, 'size': 2},\
                                       '4':{'weight': 0.0070, 'size': 3}, '5':{'weight': 0.0069, 'size': 3}, '6':{'weight':0.0289, 'size': 6}},\
                              'FALSE': {'1':{'weight': 0, 'size': 1}, '2':{'weight': 0, 'size': 1}, '3':{'weight': 0.0436, 'size': 2},\
                                        '4':{'weight': 0.0678, 'size': 3}, '5':{'weight': 0.1047, 'size': 3}, '6':{'weight': 0.7320, 'size': 6}}}
    
    max_install = PV_residential_kW[feeder_name] * penetration_kVA
    
    min1 = min([PV_residential_weights['TRUE'][x]['size'] for x in sorted_triplex['TRUE'].keys()])
    min2 = min([PV_residential_weights['FALSE'][x]['size'] for x in sorted_triplex['FALSE'].keys()])
    
    installed_capacity = 0
    solar_installation_data = []
    
    while max_install - installed_capacity > 0.1:
        
        if installed_capacity/max_install < 0.5:
            mean = 180
            sd = 30
            while 1:
                orientation_azimuth = round(random.gauss(mean, sd))
                if orientation_azimuth<360 and orientation_azimuth > 0:
                    break 
            
        else: 
            mean = 180
            sd = 60
            while 1:
                orientation_azimuth = round(random.gauss(mean, sd))
                if orientation_azimuth<360 and orientation_azimuth > 0:
                    break
            
        tilt_angle = random.choice(range(1,40))
            
            
        if '6' in sorted_triplex['FALSE'].keys():
            limit = 10000
        else:
            limit = 2082
        
        selection = random.randint(0, limit)
            
        if selection < 46:
            bin_no = '2'
            care = 'TRUE'
                        
        if selection > 45 and selection < 92:
            bin_no = '3'
            care = 'TRUE'
                        
        if selection > 91 and selection < 162:
            bin_no = '4'
            care = 'TRUE'
            
        if selection > 161 and selection < 231:
            bin_no = '5'
            care = 'TRUE'
            
        if selection > 230 and selection < 520:
            bin_no = '6'
            care = 'TRUE'
            
        if selection > 519 and selection < 956:
            bin_no = '3'
            care = 'FALSE'
            
        if selection > 955 and selection < 1634:
            bin_no = '4'
            care = 'FALSE'
            
        if selection > 1633 and selection < 2081:
            bin_no = '5'
            care = 'FALSE'
            
        if selection > 2080 and selection < 10001:
            bin_no = '6'
            care = 'FALSE'
        
        if bin_no in sorted_triplex[care].keys():
        
            if len(sorted_triplex[care][bin_no]) > 0:
            
                tri_meter = random.choice(sorted_triplex[care][bin_no])
                
                sorted_triplex[care][bin_no].remove(tri_meter)
                
                if sorted_triplex[care][bin_no] == []:
                    del sorted_triplex[care][bin_no]

                solar_installation_data.append([tri_meter, PV_residential_weights[care][bin_no]['size'], tilt_angle, orientation_azimuth, triplex_meter_data[tri_meter][0]])
                
                installed_capacity += PV_residential_weights[care][bin_no]['size']
        
        
        if sorted_triplex['TRUE'].keys() == ['1'] and sorted_triplex['FALSE'].keys() == ['1', '2']:
            
            Difference = max_install - installed_capacity
            total_new_capacity = 0
            
            while Difference > 0.1:           
                for item in solar_installation_data:
                    new_capacity = (item[1]*Difference)/installed_capacity
                    item[1] = item[1] + new_capacity
                    total_new_capacity += new_capacity
                
                installed_capacity += total_new_capacity
                Difference = max_install - installed_capacity
                
        elif sorted_triplex['TRUE'].keys() == [] and sorted_triplex['FALSE'].keys() == ['1', '2']:
            
            Difference = max_install - installed_capacity
            total_new_capacity = 0
            
            while Difference > 0.1:           
                for item in solar_installation_data:
                    new_capacity = (item[1]*Difference)/installed_capacity
                    item[1] = item[1] + new_capacity
                    total_new_capacity += new_capacity
                
                installed_capacity += total_new_capacity
                Difference = max_install - installed_capacity
                
                
        
    
#     print "Residential |", "Required Install", max_install, "| Installed Capacity", installed_capacity                
    
    return solar_installation_data


def commercial_data(inputStr, feeder_name):
    
    total_building_types = ['0','1', '2','3','4', '5', '6',\
                            '7', '8', '9', '10', '11', '12',\
                            '13', '14', '15', '16', '17', '18',\
                            '19', '20', '21', '22', '23', '24',\
                            '25', '26', '27', '28', '29', '30',\
                            '31', '32', '33']
    
    with open(inputStr) as f:
        
        com_meter_data = {}
        com_sorted_meters = {}
        com_sorted_meters = dict.fromkeys(total_building_types)
        for key in com_sorted_meters.keys():
            com_sorted_meters[key] = []
            
        for line in f:
            if 'building_type' in line:
                building_type = line[21:-2]
#                 next(f)
#                 next(f)
                next(f)
                next(f)
                next(f)
                a = next(f)
                nominal_voltage = a[23:-2]
#                print 'nominal_voltage', nominal_voltage
                a = next(f)
                meter_name = a[14:-2]
                a = next(f)
                phase = a[14:-2] + 'N'
                
                com_sorted_meters[building_type].append(meter_name)    
                com_meter_data[meter_name] = [phase, nominal_voltage]
                
        for key in com_sorted_meters.keys():
            if not com_sorted_meters[key]:
                del com_sorted_meters[key]
                   

    return com_sorted_meters, com_meter_data

            
def PV_commercial_allocation(feeder_name, com_sorted_meters, penetration_kVA):
    
    PV_commercial_kW = {'Cawston': 0.09, 'Miles': 0.43, 'Caldwell': 0.12, 'Wildwood': 0.07, 'Mallet': 0.07,\
                          'Diana': 0.41, 'Major': 0.012, 'Homer': 0.56, 'Mirror': 0.17, 'Moran': 0.38, 'Price': 0.40,\
                          'Rossi': 0.21, 'Parrot': 0.98, 'Solitaire': 1, 'Doerner': 1, 'Etting': 0.9} #Percentage of commercial PV for each feeder
    
    PV_commercial_weights = {'1':{'weight': 0.02859, 'size': 194}, '2':{'weight': 0.02823, 'size': 91}, '3':{'weight': 0.09471, 'size': 149},\
                             '4':{'weight': 0.04289, 'size': 363}, '6':{'weight': 0.01215, 'size': 232}, '7':{'weight': 0.00357, 'size': 118},\
                             '8':{'weight': 0.00107, 'size': 47}, '9':{'weight': 0.00250, 'size': 18}, '10':{'weight':0.00572, 'size': 318},\
                             '11':{'weight':0.00715, 'size': 45}, '12':{'weight': 0.03788, 'size': 28}, '13':{'weight': 0.00643, 'size': 101},\
                             '14':{'weight': 0.00071, 'size': 8}, '15':{'weight': 0.00179, 'size': 23}, '17':{'weight': 0.03860, 'size': 141},\
                             '18':{'weight': 0.00536, 'size': 255}, '19':{'weight': 0.06969, 'size': 131}, '20':{'weight': 0.03788, 'size': 111},\
                             '21':{'weight': 0.42102, 'size': 30}, '23':{'weight': 0.01072, 'size': 16}, '25':{'weight': 0.00107, 'size': 68},\
                             '26':{'weight': 0.00465, 'size': 19}, '27':{'weight': 0.03002, 'size': 331}, '28':{'weight': 0.04753, 'size': 205},\
                             '29': {'weight': 0.01465, 'size': 39}, '31':{'weight': 0.00036, 'size': 267}, '32':{'weight': 0.04325, 'size': 185},\
                             '33':{'weight': 0.00179, 'size': 607}, '0':{'weight': 0, 'size': 0}}
    
    max_install = PV_commercial_kW[feeder_name] * penetration_kVA
    
    building_types_in_feeder = com_sorted_meters.keys()    
    values = [PV_commercial_weights[x]['size'] for x in building_types_in_feeder]
    min_install_size = min(filter(None, values))
    
    installed_capacity = 0     
    com_solar_installation_data = []
    
    while installed_capacity < max_install - 0.2:
                
#         print "Installed", installed_capacity
#         print "Max", max_install
        
        available_capacity = max_install - installed_capacity
        
        if len(com_sorted_meters.keys()) == 0:
            Difference = max_install - installed_capacity
            total_new_capacity = 0
            
            while Difference > 0.1:
#                 print 'Difference', Difference 
#                 print 'Output 1', com_solar_installation_data           
                for item in com_solar_installation_data:
                    new_capacity = (item[1]*Difference)/installed_capacity
                    item[1] = item[1] + new_capacity
                    total_new_capacity += new_capacity
                
                installed_capacity += total_new_capacity
                Difference = max_install - installed_capacity
            
#             print 'Output 2', com_solar_installation_data
                
        elif available_capacity < min_install_size:
            random_BT = random.choice(com_sorted_meters.keys())
            meter = random.choice(com_sorted_meters[random_BT])
            new_added_capacity = max_install - installed_capacity
            com_solar_installation_data.append([meter, new_added_capacity])
            installed_capacity += new_added_capacity
            
        else:
            selection = random.randint(0, 99999)
                 
            if selection < 2860:
                BT = '1'
                 
            if selection > 2859 and selection < 5683:
                BT = '2'
                 
            if selection > 5682 and selection < 15154:
                BT = '3'
                 
            if selection > 15153 and selection < 19443:
                BT = '4'
             
            if selection > 19442 and selection < 20658:
                BT = '6'
             
            if selection > 20657 and selection < 21015:
                BT = '7'
                 
            if selection > 21014 and selection < 21122:
                BT = '8'
                 
            if selection > 21121 and selection < 21372:
                BT = '9'
             
            if selection > 21371 and selection < 21944:
                BT = '10'
             
            if selection > 21943 and selection < 22659:
                BT = '11'
             
            if selection > 22658 and selection < 26447:
                BT = '12'
                 
            if selection > 26446 and selection < 27090:
                BT = '13'
             
            if selection > 27089 and selection < 27161:
                BT = '14'
                 
            if selection > 27160 and selection < 27340:
                BT = '15'
             
            if selection > 27339 and selection < 31200:
                BT = '17'
                 
            if selection > 31199 and selection < 31736:
                BT = '18'
             
            if selection > 31735 and selection < 38705:
                BT = '19'
                 
            if selection > 38704 and selection < 42493:
                BT = '20'
                 
            if selection > 42492 and selection < 84595:
                BT = '21'
                 
            if selection > 84594 and selection < 85667:
                BT = '23'

            if selection > 85666 and selection < 85774:
                BT = '25'
                 
            if selection > 85773 and selection < 86239:
                BT = '26'
                 
            if selection > 86238 and selection < 89241:
                BT = '27'
                 
            if selection > 89240 and selection < 93994:           
                BT = '28'
                 
            if selection > 93993 and selection < 95459:
                BT = '29'
                 
            if selection > 95458 and selection < 95495:
                BT = '31'
                 
            if selection > 95494 and selection < 99820:
                BT = '32'
             
            if selection > 99819 and selection < 99999:
                BT = '33'
                 
            if BT in com_sorted_meters.keys():
                if len(com_sorted_meters[BT]) > 0:
         
                    meter = random.choice(com_sorted_meters[BT])
                     
                    com_sorted_meters[BT].remove(meter)
                    
                    if com_sorted_meters[BT] == []:
                        del com_sorted_meters[BT]
                    
                    if max_install - installed_capacity >= PV_commercial_weights[BT]['size']:
                        new_added_capacity = PV_commercial_weights[BT]['size']
                        com_solar_installation_data.append([meter, new_added_capacity])
                    else:
                        new_added_capacity = max_install - installed_capacity
                        com_solar_installation_data.append([meter, new_added_capacity])
                     
                    installed_capacity += new_added_capacity
                    
                    
    return com_solar_installation_data
    

    
def residential_solar_create(solar_installation_data, triplex_meter_data, V_1, Q_1,V_2,Q_2,V_3,Q_3, V_4, Q_4):
    
    solar_dict={}
    
    
    for count, i in enumerate(solar_installation_data):
        
        tri_name = ['triplex_'+i[0]+'_'+str(count)]
        groupid = ['solar_groupid']
        parent = [i[0]]
        bustype = ['PV']
        phases = [triplex_meter_data[i[0]][0]]
        nominal_voltage = [120]
           
        glm_parameters = tri_name + groupid + parent + bustype + phases + nominal_voltage
        
        solar_dict = Inverter_add_glm_object_dictionary.create_glm_object_dictionary(solar_dict, 'triplex_meter', glm_parameters)
        
        inv_name = ['inv_' + str(tri_name[0])]
        parent = tri_name
        inverter_type = ['FOUR_QUADRANT']
        four_quadrant_control_mode = ['VOLT_VAR']
        generator_status = ['ONLINE']
        generator_mode = ['SUPPLY_DRIVEN']
        rated_power = [i[1]]
        inverter_efficiency = ['0.95']
        V_base = [240]
               
        #glm_parameters = inv_name + parent  + inverter_type + four_quadrant_control_mode + generator_status + generator_mode + rated_power + inverter_efficiency  + V_1  + Q_1  + V_2 + Q_2 + V_3 + Q_3 + V_4 + Q_4

        glm_parameters = inv_name + parent  + inverter_type + four_quadrant_control_mode + generator_status + generator_mode + rated_power + inverter_efficiency  + V_1  + Q_1  + V_2 + Q_2 + V_3 + Q_3 + V_4 + Q_4 + V_base + phases
        
        solar_dict = Inverter_add_glm_object_dictionary.create_glm_object_dictionary(solar_dict, 'inverter', glm_parameters)
        
     
        solar_name = ['solar_'+str(tri_name[0])]
        SOLAR_POWER_MODEL = ['FLATPLATE']
        parent = inv_name
        generator_status = ['ONLINE'] 
        generator_mode = ['SUPPLY_DRIVEN']
        panel_type = ['SINGLE_CRYSTAL_SILICON']   
        efficiency = [0.2]
        rated_power_solar = [0]
#         Rated_kVA = [0]
        tilt_angle = [i[2]]
        orientation_azimuth = [i[3]]
        
        
        
        if tilt_angle[0] == 0:
            de_rate_value = 0.89
        
        if tilt_angle[0] > 0 and tilt_angle[0] <= 15:
            if orientation_azimuth >0 and orientation_azimuth <= 112.5:
                de_rate_value = 0.88
            if orientation_azimuth > 112.5 and orientation_azimuth <= 157.5:
                de_rate_value = 0.95
            if orientation_azimuth > 157.5 and orientation_azimuth <= 202.5:
                de_rate_value = 0.97
            if orientation_azimuth > 202.5 and orientation_azimuth <= 247.5:
                de_rate_value = 0.97
            if orientation_azimuth > 247.5:
                de_rate_value = 0.97
        
        if tilt_angle[0] > 15 and tilt_angle[0] <= 30:
            if orientation_azimuth >0 and orientation_azimuth <= 112.5:
                de_rate_value = 0.84
            if orientation_azimuth > 112.5 and orientation_azimuth <= 157.5:
                de_rate_value = 0.96
            if orientation_azimuth > 157.5 and orientation_azimuth <= 202.5:
                de_rate_value = 1
            if orientation_azimuth > 202.5 and orientation_azimuth <= 247.5:
                de_rate_value = 0.96
            if orientation_azimuth > 247.5:
                de_rate_value = 0.84
                
        if tilt_angle[0] > 30 and tilt_angle[0] <= 45:
            if orientation_azimuth >0 and orientation_azimuth <= 112.5:
                de_rate_value = 0.78
            if orientation_azimuth > 112.5 and orientation_azimuth <= 157.5:
                de_rate_value = 0.93
            if orientation_azimuth > 157.5 and orientation_azimuth <= 202.5:
                de_rate_value = 0.97
            if orientation_azimuth > 202.5 and orientation_azimuth <= 247.5:
                de_rate_value = 0.93
            if orientation_azimuth > 247.5:
                de_rate_value = 0.78
                
        if tilt_angle[0] > 45 and tilt_angle[0] <= 60:
            if orientation_azimuth >0 and orientation_azimuth <= 112.5:
                de_rate_value = 0.7
            if orientation_azimuth > 112.5 and orientation_azimuth <= 157.5:
                de_rate_value = 0.85
            if orientation_azimuth > 157.5 and orientation_azimuth <= 202.5:
                de_rate_value = 0.89
            if orientation_azimuth > 202.5 and orientation_azimuth <= 247.5:
                de_rate_value = 0.85
            if orientation_azimuth > 247.5:
                de_rate_value = 0.70
                
        if tilt_angle[0] > 60 and tilt_angle[0] <= 90:
            if orientation_azimuth >0 and orientation_azimuth <= 112.5:
                de_rate_value = 0.52
            if orientation_azimuth > 112.5 and orientation_azimuth <= 157.5:
                de_rate_value = 0.60
            if orientation_azimuth > 157.5 and orientation_azimuth <= 202.5:
                de_rate_value = 0.58
            if orientation_azimuth > 202.5 and orientation_azimuth <= 247.5:
                de_rate_value = 0.60
            if orientation_azimuth > 247.5:
                de_rate_value = 0.52
                
#         Rated_kVA = [rated_power[0] * random.uniform(1.10, 1.25)*(1/de_rate_value)]
        rated_power_solar = [str(rated_power[0] * random.uniform(1.10, 1.25)*(1/de_rate_value)) + ' kW']
        
        
        orientation = ['FIXED_AXIS']
           
        glm_parameters = solar_name +  SOLAR_POWER_MODEL + parent + generator_status + generator_mode + panel_type + efficiency + rated_power_solar + tilt_angle + orientation_azimuth + orientation + phases

         
        solar_dict = Inverter_add_glm_object_dictionary.create_glm_object_dictionary(solar_dict, 'solar', glm_parameters)
        
    return solar_dict


def com_solar_create(com_solar_installation_data, com_meter_data, V_1, Q_1,V_2,Q_2,V_3,Q_3, V_4, Q_4):
    
    solar_dict={}
    
    for i in com_solar_installation_data:
        meter_name = ['com_'+i[0]]
        groupid = ['com_solar_groupid']
        parent = [i[0]]
        phases = [com_meter_data[i[0]][0]]
        nominal_voltage = [240]

           
        glm_parameters = meter_name + groupid + parent + phases + nominal_voltage
         
        solar_dict = Inverter_add_glm_object_dictionary.create_glm_object_dictionary(solar_dict, 'meter', glm_parameters)
             

        inv_name = ['inv_' + str(meter_name[0])]
        parent = meter_name
        inverter_type = ['FOUR_QUADRANT']
        four_quadrant_control_mode = ['VOLT_VAR']
        generator_status = ['ONLINE']
        generator_mode = ['SUPPLY_DRIVEN']
        rated_power = [i[1]]
        inverter_efficiency = ['0.95']
        V_base = [com_meter_data[i[0]][1]]
        
        #glm_parameters = inv_name + parent  + inverter_type + four_quadrant_control_mode + generator_status + generator_mode + rated_power + inverter_efficiency  + V_1  + Q_1  + V_2 + Q_2 + V_3 + Q_3 + V_4 + Q_4
           
        glm_parameters = inv_name + parent  + inverter_type + four_quadrant_control_mode + generator_status + generator_mode + rated_power + inverter_efficiency  + V_1  + Q_1  + V_2 + Q_2 + V_3 + Q_3 + V_4 + Q_4 + V_base + phases
          
        solar_dict = Inverter_add_glm_object_dictionary.create_glm_object_dictionary(solar_dict, 'inverter', glm_parameters)             
               
        solar_name = ['solar_'+str(meter_name[0])]
        SOLAR_POWER_MODEL = ['FLATPLATE']
        parent = inv_name
        generator_status = ['ONLINE'] 
        generator_mode = ['SUPPLY_DRIVEN']
        panel_type = ['SINGLE_CRYSTAL_SILICON']   
        efficiency = [0.2]
        rated_power_com_solar = [str(i[1]) + ' kW']
        tilt_angle = [0]
        orientation_azimuth = [0]
        orientation = ['FIXED_AXIS']
           
        glm_parameters = solar_name +  SOLAR_POWER_MODEL + parent + generator_status + generator_mode + panel_type + efficiency + rated_power_com_solar + tilt_angle + orientation_azimuth + orientation + phases

        com_solar_dict = Inverter_add_glm_object_dictionary.create_glm_object_dictionary(solar_dict, 'solar', glm_parameters)
    
    return com_solar_dict
        
        
        
def add_violation_object(current_feeder, feeder_name, feeder_glm, house_glm, run_season_glm, commercial_glm, PV_penetration, count, dst, new_feeder_glm):
    
    
    trip_currents = {'Solitaire':720,'Wildwood': 720, 'Mallet': 720, 'Diana': 720, 'Price': 720,\
                     'Rossi': 720, 'Doerner': 720, 'Moran': 720, 'Parrot': 720, 'Cawston': 480,\
                      'Miles': 640, 'Major': 480, 'Caldwell': 600, 'Homer': 300, 'Mirror': 720, 'Etting': 600}
      
    substation_breaker = {'Solitaire':'line_16641$GS1312-4_16641',\
                          'Wildwood': 'line_19386$GS6718B-4_19386',\
                          'Mallet': 'line_11012$GS4248-2_11012',\
                          'Diana': 'line_05066$GS1711-1_05066',\
                          'Price': 'line_14404$ND41265272_14404',\
                          'Rossi': 'line_15452$ND46572954_15452',\
                          'Doerner': 'line_05145$GS2361-4_05145',\
                          'Parrot': 'line_13734$OS0794-2_13734',\
                          'Cawston': 'line_03190$ND39238893_03190',\
                          'Miles': 'line_11890$OS0049-2_11890',\
                          'Major': 'line_10980$ND148913275_10980',\
                          'Caldwell': 'line_02670$GS0525-1_02670',\
                          'Homer': 'line_08640$PS0353_08640',\
                          'Mirror': 'line_11975$ND71673918_11975',\
                          'Etting': 'line_06100$1437938EPH2_06100'}
  
   
      
    violation_dict={}
      
    file_name = ['Violation_Log_${SEASON}_%dpct_Case_%d.csv' %(PV_penetration, count)];
    summary = ['Violation_Summary_${SEASON}_%dpct_Case_%d.csv' %(PV_penetration, count)]
    interval = ['60']
    strict = ['false']
    echo = ['false']
    violation_flag = ['ALLVIOLATIONS']
      
    xfrmr_thermal_limit_upper = ['2']
    xfrmr_thermal_limit_lower = ['0']
    line_thermal_limit_upper  = ['1']
    line_thermal_limit_lower  = ['0']
      
  
    node_instantaneous_voltage_limit_upper = ['1.1']
    node_instantaneous_voltage_limit_lower = ['0']
  
    node_continuous_voltage_limit_upper = ['1.05']
    node_continuous_voltage_limit_lower = ['0.95']
    node_continuous_voltage_interval = ['300']    
  
    substation_breaker_A_limit = [str(trip_currents[feeder_name])]
    substation_breaker_B_limit = [str(trip_currents[feeder_name])]
    substation_breaker_C_limit = [str(trip_currents[feeder_name])]
    virtual_substation = [substation_breaker[feeder_name]]
      
    inverter_v_chng_per_interval_upper_bound = ['0.050']
    inverter_v_chng_per_interval_lower_bound = ['-0.050']
    inverter_v_chng_interval = ['60']
       
    secondary_dist_voltage_rise_upper_limit = ['0.025']
    secondary_dist_voltage_rise_lower_limit = ['-0.042'] 
    
    substation_pf_lower_limit = ['0.85']
    
    violation_delay = ['10800']

      
    glm_parameters =  file_name + summary + interval + strict + echo + violation_flag + \
    xfrmr_thermal_limit_upper + xfrmr_thermal_limit_lower + line_thermal_limit_upper + line_thermal_limit_lower + \
    node_instantaneous_voltage_limit_upper + node_instantaneous_voltage_limit_lower + node_continuous_voltage_limit_upper + node_continuous_voltage_limit_lower + node_continuous_voltage_interval + \
    substation_breaker_A_limit + substation_breaker_B_limit + substation_breaker_C_limit + virtual_substation +\
    inverter_v_chng_per_interval_upper_bound + inverter_v_chng_per_interval_lower_bound + inverter_v_chng_interval +\
    secondary_dist_voltage_rise_upper_limit + secondary_dist_voltage_rise_lower_limit + substation_pf_lower_limit + violation_delay
      
    violation_dict = Inverter_add_glm_object_dictionary.create_glm_object_dictionary(violation_dict, 'violation_recorder', glm_parameters)
      
    violation_str = feeder_parse_mod.sortedWrite(violation_dict)
    violation_glm_filename = '%s\\%s_%dpct_case_%d_violation_object.glm' %(dst, feeder_name, PV_penetration, count)
    violation_glm = open(violation_glm_filename,'w')
    violation_glm.write(violation_str)
    violation_glm.close()
    
      
def modify_run_season_file(current_feeder, run_season_glm, feeder_glm, new_feeder_glm, feeder_name, count, house_glm, commercial_glm, PV_penetration, dst, new_commercial_glm):
        
    run_season_path = os.path.join(current_feeder, run_season_glm)
    with open(run_season_path,'r') as glmFile:               
        all_lines = glmFile.readlines()        
    glmFile.close()         
     
       
    for index, line in enumerate(all_lines):
                    
        if '#include "%s"' %(feeder_glm) in line:
            all_lines[index] = '#include "%s"\n' %(new_feeder_glm)
            all_lines.insert(index+1, '#include "%s_%dpct_case_%d_violation_object.glm"\n' %(feeder_name, PV_penetration, count))
        
        if PV_penetration != 0:
            if '#include "%s"' %(house_glm) in line:
                if feeder_name != 'Solitaire' and feeder_name != 'Doerner':
                    all_lines.insert(index+1, '#include "%s_%dpct_case_%d_residential_solar.glm"\n' %(feeder_name, PV_penetration, count))
                
            if '#include "%s"' %(commercial_glm) in line:
                all_lines.insert(index+1, '#include "%s_%dpct_case_%d_commercial_solar.glm"\n' %(feeder_name, PV_penetration, count))
        
        if '#include "%s"' %(commercial_glm) in line:
            all_lines[index] = '#include "%s"\n' %(new_commercial_glm)
            
        if 'file ${SEASON}' in line:
            file_name_string = line[9:-2]
            all_lines[index] = '    file ${PEN_LEV}pct_Case_${CASE}_'+file_name_string+';\n'
            
        if '#include "Missing_Load_17.glm"'in line:
            all_lines[index] = '#include "new_Missing_Load_17.glm"\n'           
        
            
    new_run_season_glm = '%s\\%s_%dpct_case_%d_%s' %(dst, feeder_name, PV_penetration, count, run_season_glm)
    run_season_path = os.path.join(current_feeder, new_run_season_glm)
    with open(run_season_path,'w') as glmFile:   
        glmFile.writelines(all_lines)        
    glmFile.close()            
    


def add_conductor_ratings(current_feeder, glm, dst, new_glm_name, Custom_CYME_DB):
    
    CYME_cond_DB = CYME_to_dict(Custom_CYME_DB)
    
    feeder_location = os.path.join(current_feeder, glm)
    feeder_data = feeder_parse_mod.parse(feeder_location, filePath=True)
    
    for k, v in feeder_data.iteritems():
        if 'overhead_line_conductor' in v.values():
            if glm == 'Missing_Load_17.glm' or glm == 'commercial_17.glm' or glm == 'commercial_5.glm' or glm == 'commercial_24.glm':
                feeder_data[k]['rating.summer.continuous'] = '300'
                feeder_data[k]['geometric_mean_radius'] = string.replace(feeder_data[k]['geometric_mean_radius'], 'in', 'ft')
                
            else: 
                cond_type = v['name'][10:]
                feeder_data[k]['rating.summer.continuous'] = CYME_cond_DB[cond_type]
                feeder_data[k]['geometric_mean_radius'] = string.replace(feeder_data[k]['geometric_mean_radius'], 'in', 'ft')       
          
        if 'line_configuration' in v.values() and 'z11' in v.keys():
            if glm == 'Missing_Load_17.glm' or glm == 'commercial_17.glm' or glm == 'commercial_5.glm' or glm == 'commercial_24.glm':
                feeder_data[k]['rating.summer.continuous'] = '300'             
            
            elif v['name'][0] == 'o':
                cond_type = v['name'][9:]
                feeder_data[k]['rating.summer.continuous'] = CYME_cond_DB[cond_type]
                   
            elif v['name'][0] == 'u':
                cond_type = v['name'][12:]
                feeder_data[k]['rating.summer.continuous'] = CYME_cond_DB[cond_type]
                             
    glm_str = feeder_parse_mod.sortedWrite(feeder_data)
    new_glm = open(os.path.join(dst, new_glm_name),'w')
    new_glm.write(glm_str)
    new_glm.close()



def add_commercial_conductor_ratings(current_feeder, glm, dst, new_glm_name, Custom_CYME_DB):
    
    feeder_location = os.path.join(current_feeder, glm)
    feeder_data = feeder_parse_mod.parse(feeder_location, filePath=True)
    
    GMR_Rating_Map = {0.011195261: 85, 0.013077363: 120, 0.016614417: 155,\
                       0.021352121: 200, 0.026965977: 290, 0.038550639: 385, \
                       0.01956737: 315, 0.045: 770}
    
    
    for k, v in feeder_data.iteritems():
        if 'overhead_line_conductor' in v.values():
            
            feeder_data[k]['geometric_mean_radius'] = string.replace(feeder_data[k]['geometric_mean_radius'], 'in', 'ft')
            GMR = round(float(re.sub("[^0-9.]", "", feeder_data[k]['geometric_mean_radius'])), 9)
            feeder_data[k]['rating.summer.continuous'] = GMR_Rating_Map[GMR]
                

          
        if 'line_configuration' in v.values() and 'z11' in v.keys():
            feeder_data[k]['rating.summer.continuous'] = '300'             
            
                             
    glm_str = feeder_parse_mod.sortedWrite(feeder_data)
    new_glm = open(os.path.join(dst, new_glm_name),'w')
    new_glm.write(glm_str)
    new_glm.close()




def CYME_to_dict(Custom_CYME_DB):
    
    with open(Custom_CYME_DB, 'rb') as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        CYME_conductor_list = list(reader)
        
        CYME_cond_DB = {}
        
        for i, v in enumerate(CYME_conductor_list):
            v[0] = v[0].replace('/', '_')
            v[0] = v[0].replace(' ', '_')
            CYME_cond_DB[v[0]] = v[1]
            
    return CYME_cond_DB


def find_files(current_feeder, feeder_name, pen_lev, case_no, feeder_num):
    
    feeder_name = os.path.split(current_feeder)[1]
    new_feeder_glm = ''
    
    commercial_solar_GLM = ''
    new_commercial_GLM = ''

    for root, dirs, files in os.walk(current_feeder):
        for item in files:
            if fnmatch.fnmatch(item, 'House*.glm'):
                house_glm = item
            
            if fnmatch.fnmatch(item, 'commercial_*.glm'):
                commercial_glm = item
                
            if fnmatch.fnmatch(item, 'Feeder*.glm'):
                feeder_glm = item
                
            if fnmatch.fnmatch(item, 'run_season_*.glm'):
                run_season_glm = item
                
            if fnmatch.fnmatch(item, '%s_%dpct_case_%d_feeder_*.glm'%(feeder_name, pen_lev, case_no)):
                new_feeder_glm = item          
                
            if fnmatch.fnmatch(item, '%s_%dpct_case_%d_commercial_solar.glm'%(feeder_name, pen_lev, case_no)):
                commercial_solar_GLM = item          
            
            if fnmatch.fnmatch(item, '%s_%dpct_case_%d_commercial_%d.glm'%(feeder_name, pen_lev, case_no, feeder_num)):
                new_commercial_GLM = item          
                                
    
    return feeder_name, feeder_glm, house_glm, run_season_glm, commercial_glm, new_feeder_glm, commercial_solar_GLM, new_commercial_GLM


def copytree(src, dst, symlinks, ignore):
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            shutil.copytree(s, d, symlinks, ignore)
        else:
            shutil.copy2(s, d)
            

def modify_node_to_triplex_node(new_feeder_glm, current_feeder, dst, feeder_number, transformer_solar_map, transformer_meter_map, line_solar_map, residential_transformer_solar_map):
    
    feeder_location = os.path.join(current_feeder, new_feeder_glm)
    
    node_data = feeder_parse_mod.parse(feeder_location, filePath=True)
    
    line_solar_conductor_data_map = {}
    
    for k, v in node_data.iteritems():
        
        if 'node' in v.values():
            if 'S' in node_data[k]['phases']:
                node_data[k]['object'] = 'triplex_node'
                
        if 'nominal_voltage' in v.keys() and 'capacitor' in v.values():
             
            if node_data[k]['nominal_voltage'] == '2309.47 V':
                node_data[k]['nominal_voltage'] = '2401.5 V'
         
            elif node_data[k]['nominal_voltage'] == '6928.41 V':
                node_data[k]['nominal_voltage'] = '7204.58 V'
                 
            elif node_data[k]['nominal_voltage'] == '9237.88 V':
                node_data[k]['nominal_voltage'] = '9606.11 V'
            
        if 'nominal_voltage' in v.keys() and 'capacitor' not in v.values():
            
            if node_data[k]['nominal_voltage'] == '2309.47 V':
                node_data[k]['nominal_voltage'] = '2400 V'
        
            elif node_data[k]['nominal_voltage'] == '6928.41 V':
                node_data[k]['nominal_voltage'] = '7200 V'
                
            elif node_data[k]['nominal_voltage'] == '9237.88 V':
                node_data[k]['nominal_voltage'] = '9600 V'
                
        if 'primary_voltage' in v.keys():
            if node_data[k]['primary_voltage'] == '2309.47 V':
                node_data[k]['primary_voltage'] = '2400 V'
                
            elif node_data[k]['primary_voltage'] == '9237.88 V':
                node_data[k]['primary_voltage'] = '9600 V'
        
            elif node_data[k]['primary_voltage'] == '6928.41 V':
                node_data[k]['primary_voltage'] = '7200 V'
        
        
        if v['object'] == 'node' and 'source_' in v['name']:
            del node_data[k]['voltage_A']
            del node_data[k]['voltage_B']
            del node_data[k]['voltage_C']
            
        
        if v['object'] == 'transformer_configuration':
            if v['name'] in transformer_solar_map.keys():
                
                if float(v['power_rating']) < float(transformer_solar_map[v['name']]):
                    
                    
                    if (float(v['secondary_voltage']) == 120 and v['connect_type'] != 'SINGLE_PHASE_CENTER_TAPPED') or float(v['secondary_voltage']) == 208:
                        
                        if float(transformer_solar_map[v['name']]) < 75:
                            v['power_rating'] = 75
                                                    
                        if float(transformer_solar_map[v['name']]) >= 75 and float(v['power_rating']) < 150:
                            v['power_rating'] = 150
                            
                        if float(transformer_solar_map[v['name']]) >= 150 and float(v['power_rating']) < 300:
                            v['power_rating'] = 300
                            
                        if float(transformer_solar_map[v['name']]) >= 300 and float(v['power_rating']) < 500:
                            v['power_rating'] = 500
                            
                        if float(transformer_solar_map[v['name']]) >= 500 and float(v['power_rating']) < 750:
                            v['power_rating'] = 750
                            
                        if float(transformer_solar_map[v['name']]) >= 750 and float(v['power_rating']) < 1000:
                            v['power_rating'] = 1000
                            


                    elif (float(v['secondary_voltage']) == 120 and v['connect_type'] == 'SINGLE_PHASE_CENTER_TAPPED') or float(v['secondary_voltage']) == 240:
                        
                        if float(transformer_solar_map[v['name']])< 75:
                            v['power_rating'] = 75
                            
                        if float(transformer_solar_map[v['name']]) >= 75 and float(v['power_rating']) < 300:
                            v['power_rating'] = 300
                            
                        if float(transformer_solar_map[v['name']]) >= 300 and float(v['power_rating']) < 500:
                            v['power_rating'] = 500
                        
                        
                    elif float(v['secondary_voltage']) == 277 or float(v['secondary_voltage']) == 480:
                        
                        
                        if float(transformer_solar_map[v['name']]) < 75 :
                            v['power_rating'] = 75
                            
                        if float(transformer_solar_map[v['name']]) >= 75 and float(v['power_rating']) < 150:
                            v['power_rating'] = 150
                            
                        if float(transformer_solar_map[v['name']]) >= 150 and float(v['power_rating']) < 300:
                            v['power_rating'] = 300
                            
                        if float(transformer_solar_map[v['name']]) >= 300 and float(v['power_rating']) < 500:
                            v['power_rating'] = 500
                            
                        if float(transformer_solar_map[v['name']]) >= 500 and float(v['power_rating']) < 750:
                            v['power_rating'] = 750
                        
                        if float(transformer_solar_map[v['name']]) >= 750 and float(v['power_rating']) < 1000:
                            v['power_rating'] = 1000

                        if float(transformer_solar_map[v['name']]) >= 1000 and float(v['power_rating']) < 1500:
                            v['power_rating'] = 1500
                            
                        if float(transformer_solar_map[v['name']]) >= 1500 and float(v['power_rating']) < 2500:
                            v['power_rating'] = 2500
                            
                        if float(transformer_solar_map[v['name']]) >= 2500 and float(v['power_rating']) < 3750:
                            v['power_rating'] = 3750
                            
                    else:
#                        print "UNKNOWN VOLTAGE TYPE"
#                        print v['name'], float(v['secondary_voltage']), v['connect_type']
                        break
                    
                    phase_count = 0
                    if 'powerA_rating' in v.keys():
                        phase_count += 1
                        
                    if 'powerB_rating' in v.keys():
                        phase_count += 1
                    
                    if 'powerC_rating' in v.keys():
                        phase_count += 1
                        
                    if 'powerA_rating' in v.keys():
                        v['powerA_rating'] = float(v['power_rating'])/phase_count
                        
                    if 'powerB_rating' in v.keys():
                        v['powerB_rating'] = float(v['power_rating'])/phase_count
                    
                    if 'powerC_rating' in v.keys():
                        v['powerC_rating'] = float(v['power_rating'])/phase_count                     
                                                
#                 else:
#                     for tx_item in transformer_meter_map[v['name']]:
#                         del line_solar_map[tx_item]
  
# Adding new stuff here..                      
        if v['object'] == 'transformer_configuration':
            if v['name'] in transformer_solar_map.keys():
                if (float(v['secondary_voltage']) == 120 and v['connect_type'] != 'SINGLE_PHASE_CENTER_TAPPED') or float(v['secondary_voltage']) == 208:
                    for tx_item in transformer_meter_map[v['name']]:
                        line_solar_map[tx_item].append('208')
                            
                elif (float(v['secondary_voltage']) == 120 and v['connect_type'] == 'SINGLE_PHASE_CENTER_TAPPED') or float(v['secondary_voltage']) == 240:
                    for tx_item in transformer_meter_map[v['name']]:
                        line_solar_map[tx_item].append('240')
                        
                elif float(v['secondary_voltage']) == 277 or float(v['secondary_voltage']) == 480:
                    for tx_item in transformer_meter_map[v['name']]:
                        line_solar_map[tx_item].append('277')
                        
                elif float(v['secondary_voltage']) == 2400:
                    for tx_item in transformer_meter_map[v['name']]:
                        line_solar_map[tx_item].append('2400')

######################## New stuff youa added for residential transformer upgrade on Major
                       
        if v['object'] == 'transformer_configuration':
            
            if v['name'] in residential_transformer_solar_map.keys():
                print v['power_rating']
                if float(v['power_rating']) < float(residential_transformer_solar_map[v['name']]):
                    if (float(v['secondary_voltage']) == 120 and v['connect_type'] == 'SINGLE_PHASE_CENTER_TAPPED') or float(v['secondary_voltage']) == 240:
#                        print v['name'], v['secondary_voltage'], v['connect_type']
                        
                        if float(residential_transformer_solar_map[v['name']])< 75:
                            v['power_rating'] = 75
                            v['powerA_rating'] = 75
                            
                            
                        if float(residential_transformer_solar_map[v['name']]) >= 75 and float(v['power_rating']) < 300:
                            v['power_rating'] = 300
                            v['powerA_rating'] = 300
                            
                        if float(residential_transformer_solar_map[v['name']]) >= 300 and float(v['power_rating']) < 500:
                            v['power_rating'] = 500
                            v['powerA_rating'] = 500
            
################################################################################
                
                    
    feeder_str = feeder_parse_mod.sortedWrite(node_data)
     
    new_feeder_glm_filename = '%s' %(new_feeder_glm)
    new_feeder_glm = open(os.path.join(current_feeder, new_feeder_glm_filename),'w')
    new_feeder_glm.write(feeder_str)
    new_feeder_glm.close()
    
    
    new_feeder_path = os.path.join(current_feeder, new_feeder_glm_filename)
    with open(new_feeder_path,'r') as glmFile:               
        all_lines = glmFile.readlines()        
        glmFile.close()         
        
        for index, line in enumerate(all_lines):
            if 'name source_' in line:
                all_lines.insert(index+1, '\tobject player { \n \t \t file voltage_schedule_%d_A.player; \n \t \t property voltage_A; }; \n\
    object player {\n \t \t file voltage_schedule_%d_B.player; \n \t \t property voltage_B;}; \n\tobject player { \n \t \t file voltage_schedule_%d_C.player; \n \t \t property voltage_C;};\n' %(feeder_number, feeder_number, feeder_number))

                break
                            
    with open(new_feeder_path,'w') as glmFile:   
        glmFile.writelines(all_lines)        
    glmFile.close()

    return line_solar_map
    

def com_transformers_and_solar_map(current_feeder, commercial_solar_GLM):
     
    commercial_solar_GLM_location = os.path.join(current_feeder, commercial_solar_GLM)
    
    commercial_solar_GLM_data = feeder_parse_mod.parse(commercial_solar_GLM_location, filePath=True)
     
    transformer_solar_map = {}
    transformer_meter_map = {}
    line_solar_map = {}    
     
    for key, value in commercial_solar_GLM_data.iteritems():
        if "inverter" in value.values():
            temp_string = list(value['name'])
            temp_index = []

            for number, character in enumerate(temp_string):                       
                if character == '_':
                    temp_index.append(number)
            
#            print temp_index 
            
            if 'config_' + value["name"][temp_index[2]+1:temp_index[5]] in transformer_solar_map.keys():
                transformer_solar_map['config_' + value["name"][temp_index[2]+1:temp_index[5]]] += float(value["rated_power"])
                line_solar_map[value["name"][8:]] = [float(value["rated_power"])]
                transformer_meter_map['config_' + value["name"][temp_index[2]+1:temp_index[5]]].append(value["name"][8:])
            else:
                transformer_solar_map['config_' + value["name"][temp_index[2]+1:temp_index[5]]] = float(value["rated_power"])
                transformer_meter_map['config_' + value["name"][temp_index[2]+1:temp_index[5]]] = [value["name"][8:]]
                line_solar_map[value["name"][8:]] = [float(value["rated_power"])]
                
            
#             if 'config_' + value["name"][temp_index[2]+1:temp_index[5]] in transformer_solar_map.keys():
#                 transformer_solar_map['config_' + value["name"][temp_index[2]+1:temp_index[5]]] += float(value["rated_power"])
#                 line_solar_map[value["name"][8:]] = [float(value["rated_power"])]
#                 transformer_meter_map['config_' + value["name"][temp_index[2]+1:temp_index[5]]].append(value["name"][8:])
#             else:
#                 transformer_solar_map['config_' + value["name"][temp_index[2]+1:temp_index[5]]] = float(value["rated_power"])
#                 transformer_meter_map['config_' + value["name"][temp_index[2]+1:temp_index[5]]] = [value["name"][8:]]
#                 line_solar_map[value["name"][8:]] = [float(value["rated_power"])]
    
    return transformer_solar_map, transformer_meter_map, line_solar_map
            

def remodify_com_conductor_data(line_solar_map, current_feeder, new_commercial_GLM):
     
    new_commercial_GLM_location = os.path.join(current_feeder, new_commercial_GLM)
    new_commercial_GLM_data = feeder_parse_mod.parse(new_commercial_GLM_location, filePath=True)
      
    for all_element_key, all_element_value in new_commercial_GLM_data.iteritems():
        if all_element_value['object'] == 'overhead_line_conductor':
            
            for key, value in line_solar_map.iteritems():
                value_temp = value[0]*1.732*1.1
                if all_element_value['name'] == 'oh_line_conductor_' + key[6:]:
                    if value[1] == '208':
#                         if value[0] < 75:
                        if value_temp < 75:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(290):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '290'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.02135'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.000097'
                            #print new_commercial_GLM_data[all_element_key]['name'],  value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
#                         elif value[0] >= 75 and value[0] < 150:
                        elif value_temp >= 75 and value_temp < 150:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(580):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '580'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.03855'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.00003'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
#                         elif value[0] >= 150 and value[0] < 300:
                        elif value_temp >= 150 and value_temp < 300:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(1740):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '1740'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.05'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.00001'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
#                         elif value[0] >= 300 and value[0] < 500:
                        elif value_temp >= 300 and value_temp < 500:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(2900):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '2900'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.07'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.000006'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
#                         elif value[0] >= 500 and value[0] < 750:
                        elif value_temp >= 500 and value_temp < 750:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(4640):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '4640'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.09'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.00000375'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
#                         elif value[0] >= 750 and value[0] < 1000:
                        elif value_temp >= 750 and value_temp < 1000:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(5220):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '5220'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.1'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.00000333'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
                        else:
                            print "value not found 1"
                            break
                            
                    if value[1] == '277':
#                         if value[0] < 75:
                        if value_temp < 75:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(120):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '120'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.01308'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.000308'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
#                         elif value[0] >= 75 and value[0] < 150:
                        elif value_temp >= 75 and value_temp < 150:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(290):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '290'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.02135'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.000097'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
#                         elif value[0] >= 150 and value[0] < 300:
                        elif value_temp >= 150 and value_temp < 300:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(385):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '385'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.02697'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.000059'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
#                         elif value[0] >= 300 and value[0] < 500:
                        elif value_temp >= 300 and value_temp < 500:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(580):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '580'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.03855'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.00003'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
#                         elif value[0] >= 500 and value[0] < 750:
                        elif value_temp >= 500 and value_temp < 750:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(1740):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '1740'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.0600'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.00001'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
#                         elif value[0] >= 750 and value[0] < 1000:
                        elif value_temp >= 750 and value_temp < 1000:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(2320):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '2320'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.06500'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.0000075'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                        
#                         elif value[0] >= 1000 and value[0] < 1500:
                        elif value_temp >= 1000 and value_temp < 1500:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(2900):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '2900'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.07'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.000006'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
#                         elif value[0] >= 1500 and value[0] < 2500:
                        elif value_temp >= 1500 and value_temp < 2500:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(5800):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '5800'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.13000'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.000003'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
#                         elif value[0] >= 2500 and value[0] < 3750:
                        elif value_temp >= 2500 and value_temp < 3750:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(6380):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '6380'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.13500'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.0000027'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
                        else:
                            print "value not found 2"
                            break
                            
                    if value[1] == '240':
                        if value[0] < 75:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(180):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '200'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.02135'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.000097'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
                        elif value[0] >= 75 and value[0] < 300:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(722):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '722'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.045'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.0000295'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
                        elif value[0] >= 300 and value[0] < 500:
                            if float(new_commercial_GLM_data[all_element_key]['rating.summer.continuous']) < float(1200):
                                new_commercial_GLM_data[all_element_key]['rating.summer.continuous'] = '1200'
                                new_commercial_GLM_data[all_element_key]['geometric_mean_radius'] = '0.06'
                                new_commercial_GLM_data[all_element_key]['resistance'] = '0.00001475'
                            #print new_commercial_GLM_data[all_element_key]['name'], value[1], "volts, ", value[0], "kW, ", "Rating = ", new_commercial_GLM_data[all_element_key]['resistance']
                            
                        else:
                            print "value not found 3"
                            break
                            
    new_commercial_str = feeder_parse_mod.sortedWrite(new_commercial_GLM_data)
     
    new_commercial_glm_filename = new_commercial_GLM
#     new_commercial_glm_filename = "Uyira_vaangadheenga_saar.glm"
    new_commercial_glm = open(os.path.join(current_feeder, new_commercial_glm_filename),'w')
    new_commercial_glm.write(new_commercial_str)
    new_commercial_glm.close()
                            














    
def PV_residential_allocation_UNIFORM(feeder_name, sorted_triplex, penetration_kVA, triplex_meter_data):
    
    PV_residential_kW = {'Cawston': 0.91, 'Miles': 0.57, 'Caldwell': 0.88, 'Wildwood': 0.93, 'Mallet': 0.93,\
                          'Diana': 0.59, 'Major': 0.99, 'Homer': 0.44, 'Mirror': 0.83, 'Moran': 0.62, 'Price': 0.60,\
                          'Rossi': 0.79, 'Parrot': 0.02, 'Solitaire': 0, 'Doerner': 0, 'Etting': 0.10}
    
    PV_residential_weights = {'TRUE': {'1':{'weight': 0, 'size': 1}, '2': {'weight': 0.0046, 'size': 2}, '3': {'weight': 0.0046, 'size': 2},\
                                       '4':{'weight': 0.0070, 'size': 3}, '5':{'weight': 0.0069, 'size': 3}, '6':{'weight':0.0289, 'size': 6}},\
                              'FALSE': {'1':{'weight': 0, 'size': 1}, '2':{'weight': 0, 'size': 1}, '3':{'weight': 0.0436, 'size': 2},\
                                        '4':{'weight': 0.0678, 'size': 3}, '5':{'weight': 0.1047, 'size': 3}, '6':{'weight': 0.7320, 'size': 6}}}
    
    max_install = PV_residential_kW[feeder_name] * penetration_kVA
    
    installed_capacity = 0
    solar_installation_data = []
    
    care_choices = ['TRUE', 'FALSE']
    bin_no_choices = ['1', '2', '3', '4', '5', '6']
    
    while max_install - installed_capacity > 0.1:
        
        if installed_capacity/max_install < 0.5:
            mean = 180
            sd = 30
            while 1:
                orientation_azimuth = round(random.gauss(mean, sd))
                if orientation_azimuth<360 and orientation_azimuth > 0:
                    break 
            
        else: 
            mean = 180
            sd = 60
            while 1:
                orientation_azimuth = round(random.gauss(mean, sd))
                if orientation_azimuth<360 and orientation_azimuth > 0:
                    break
            
        tilt_angle = random.choice(range(1,40))
        
        care = random.choice(care_choices)
        bin_no = random.choice(bin_no_choices)
               
        if bin_no in sorted_triplex[care].keys():
        
            if len(sorted_triplex[care][bin_no]) > 0:
            
                tri_meter = random.choice(sorted_triplex[care][bin_no])
                
                sorted_triplex[care][bin_no].remove(tri_meter)
                
                if sorted_triplex[care][bin_no] == []:
                    del sorted_triplex[care][bin_no]
                    

                solar_installation_data.append([tri_meter, PV_residential_weights[care][bin_no]['size'], tilt_angle, orientation_azimuth, triplex_meter_data[tri_meter]])
                
                installed_capacity += PV_residential_weights[care][bin_no]['size']
        
        
        if sorted_triplex['TRUE'].keys() == [] and sorted_triplex['FALSE'].keys() == []:
            
            Difference = max_install - installed_capacity
            total_new_capacity = 0
            
            while Difference > 0.1:           
                for item in solar_installation_data:
                    new_capacity = (item[1]*Difference)/installed_capacity
                    item[1] = item[1] + new_capacity
                    total_new_capacity += new_capacity
                
                installed_capacity += total_new_capacity
                Difference = max_install - installed_capacity
                
#         print 'max_install', max_install, 'installed_capacity', installed_capacity
                
    return solar_installation_data             



def PV_commercial_allocation_UNIFORM(feeder_name, com_sorted_meters, penetration_kVA):
    
    PV_commercial_kW = {'Cawston': 0.09, 'Miles': 0.43, 'Caldwell': 0.12, 'Wildwood': 0.07, 'Mallet': 0.07,\
                          'Diana': 0.41, 'Major': 0.012, 'Homer': 0.56, 'Mirror': 0.17, 'Moran': 0.38, 'Price': 0.40,\
                          'Rossi': 0.21, 'Parrot': 0.98, 'Solitaire': 1, 'Doerner': 1, 'Etting': 0.9} #Percentage of commercial PV for each feeder
    
    PV_commercial_weights = {'1':{'weight': 0.02859, 'size': 194}, '2':{'weight': 0.02823, 'size': 91}, '3':{'weight': 0.09471, 'size': 149},\
                             '4':{'weight': 0.04289, 'size': 363}, '6':{'weight': 0.01215, 'size': 232}, '7':{'weight': 0.00357, 'size': 118},\
                             '8':{'weight': 0.00107, 'size': 47}, '9':{'weight': 0.00250, 'size': 18}, '10':{'weight':0.00572, 'size': 318},\
                             '11':{'weight':0.00715, 'size': 45}, '12':{'weight': 0.03788, 'size': 28}, '13':{'weight': 0.00643, 'size': 101},\
                             '14':{'weight': 0.00071, 'size': 8}, '15':{'weight': 0.00179, 'size': 23}, '17':{'weight': 0.03860, 'size': 141},\
                             '18':{'weight': 0.00536, 'size': 255}, '19':{'weight': 0.06969, 'size': 131}, '20':{'weight': 0.03788, 'size': 111},\
                             '21':{'weight': 0.42102, 'size': 30}, '23':{'weight': 0.01072, 'size': 16}, '25':{'weight': 0.00107, 'size': 68},\
                             '26':{'weight': 0.00465, 'size': 19}, '27':{'weight': 0.03002, 'size': 331}, '28':{'weight': 0.04753, 'size': 205},\
                             '29': {'weight': 0.01465, 'size': 39}, '31':{'weight': 0.00036, 'size': 267}, '32':{'weight': 0.04325, 'size': 185},\
                             '33':{'weight': 0.00179, 'size': 607}, '0':{'weight': 0, 'size': 0}}
    
    max_install = PV_commercial_kW[feeder_name] * penetration_kVA
    
    installed_capacity = 0     
    com_solar_installation_data = []
    
    while installed_capacity < max_install:
        
        random_BT = random.choice(com_sorted_meters.keys())
        meter = random.choice(com_sorted_meters[random_BT])
        
        if max_install - installed_capacity >= PV_commercial_weights[random_BT]['size']:
            new_added_capacity = PV_commercial_weights[random_BT]['size']
            com_solar_installation_data.append([meter, new_added_capacity])
            
        new_added_capacity = PV_commercial_weights[random_BT]['size']
        com_solar_installation_data.append([meter, PV_commercial_weights[random_BT]['size']])
        installed_capacity += new_added_capacity
        
        com_sorted_meters[random_BT].remove(meter)
               
        if com_sorted_meters[random_BT] == []:
            del com_sorted_meters[random_BT]
        
    return com_solar_installation_data
