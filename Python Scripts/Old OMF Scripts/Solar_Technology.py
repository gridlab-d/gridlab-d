from __future__ import division
import math
import random

def Append_Solar(PV_Tech_Dict, use_flags, config_data, tech_data, last_key, solar_bigbox_array=None, solar_office_array=None, solar_stripmall_array=None, solar_residential_array=None):
    # PV_Tech_Dict - the dictionary that we add solar objects to
    # use_flags - the output from TechnologyParameters.py
    # config_data - the output from Configuration.py
    # solar_bigbox_array - contains a list of commercial houses, corresponding floor areas, parents,and phases that commercial PV can be attached to
    # solar_office_array - contains a list of commercial houses, corresponding floor areas, parents,and phases that commercial PV can be attached to
    # solar_stripmall_array - contains a list of commercial houses, corresponding floor areas, parents,and phases that commercial PV can be attached to
    # solar_residential_array - contains a list of residential houses, corresponding floor areas, parents,and phases that residential PV can be attached to
    # last_key should be a numbered key that is the next key in PV_Tech_Dict
    
    # Initialize psuedo-random seed
    random.seed(4)
    
    # Populating solar as percentage of feeder peak load
    # Add Commercial PV
    if use_flags['use_solar'] != 0 or use_flags['use_solar_com'] != 0:
        # Initialize psuedo-random seed
        random.seed(4)
        
        # Determine solar penetrations for each of the three types of commercial configurations
        if solar_stripmall_array == None and solar_bigbox_array == None and solar_office_array != None: # Have stripmalls only
            penetration_stripmall = 0
            penetration_bigbox = 0
            penetration_office = config_data['solar_penetration']
        elif solar_stripmall_array == None and solar_bigbox_array != None and solar_office_array == None: # Have bigboxes only
            penetration_stripmall = 0
            penetration_bigbox = config_data['solar_penetration']
            penetration_office = 0
        elif solar_stripmall_array != None and solar_bigbox_array == None and solar_office_array == None: # Have offices only
            penetration_stripmall = config_data['solar_penetration']
            penetration_bigbox = 0
            penetration_office = 0
        elif solar_stripmall_array != None and solar_bigbox_array == None and solar_office_array != None: # Have stripmalls and offices only
            penetration_stripmall = config_data['solar_penetration'] / 2
            penetration_bigbox = 0
            penetration_office = config_data['solar_penetration'] / 2
        elif solar_stripmall_array != None and solar_bigbox_array != None and solar_office_array == None: # Have stripmalls and bigboxes only
            penetration_stripmall = config_data['solar_penetration'] / 2
            penetration_bigbox = config_data['solar_penetration'] / 2
            penetration_office = 0
        elif solar_stripmall_array == None and solar_bigbox_array != None and solar_office_array != None: # Have offices and bigboxes only
            penetration_stripmall = 0
            penetration_bigbox = config_data['solar_penetration'] / 2
            penetration_office = config_data['solar_penetration'] / 2
        else: # Have stripmalls, bigboxes, and offices
            penetration_stripmall = config_data['solar_penetration'] / 3
            penetration_bigbox = config_data['solar_penetration'] / 3
            penetration_office = config_data['solar_penetration'] / 3
            
        solar_rating = config_data['solar_rating'] * 1000 # Convert kW to W
        #Determine total number of PV we must add to office
        if penetration_office > 0:    # solar_office_array = list(number of office meters attached to com loads,list(office meter names attached to loads),list(phases of office meters attached to loads))
            total_office_pv_units = math.ceil((config_data['emissions_peak'] * penetration_office) / tech_data['solar_averagepower_office'])
            total_office_number = int(solar_office_array[0])
            
            # Create a randomized list of numbers 0 to total_office_number
            random_index = []
            random_index = random.sample(list(xrange(total_office_number)),total_office_number);
            
            # Determine how many units to attach to each office building
            pv_units_per_office = int(math.ceil(total_office_pv_units / total_office_number))
            
            # Attach PV units to dictionary
            pv_unit = 0
            floor_area = round(solar_rating / (92.902 * 0.20))
            
            for x in xrange(total_office_number):
                parent = solar_office_array[1][random_index[x]]
                phases = solar_office_array[2][random_index[x]]
                pv_unit = pv_unit + pv_units_per_office
                
                if pv_unit < total_office_number:
                    if pv_units_per_office > (total_office_number - pv_unit - 1):
                        pv_units_per_office = total_office_number - pv_unit - 1
                        
                    for y in xrange(0,pv_units_per_office):
                        # Write the PV meter
                        last_key += 1
                        PV_Tech_Dict[last_key] = {'object' : 'meter',
                                                  'name' : 'pv_meter{:d}_{:s}'.format(y,parent),
                                                  'parent' : '{:s}'.format(parent),
                                                  'phases' : '{:s}'.format(phases),
                                                  'nominal_voltage' : '{:f}'.format(config_data['nom_volt2']),
                                                  'group_id' : 'Commercial_m_solar_office'}
                        
                        # Write the PV inverter
                        last_key += 1
                        PV_Tech_Dict[last_key] = {'object' : 'inverter',
                                                  'name' : 'pv_inverter_{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'parent' : '{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'phases' : '{:s}'.format(phases),
                                                  'generator_mode' : 'CONSTANT_PF',
                                                  'generator_status' : 'ONLINE',
                                                  'inverter_type' : 'PWM',
                                                  'power_factor' : '1.0',
                                                  'rated_power' : '{:.0f}'.format(math.ceil(solar_rating))}
                                                  
                        # Write the PV inverter
                        last_key += 1
                        PV_Tech_Dict[last_key] = {'object' : 'solar',
                                                  'name' : 'sol_panel_{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'parent' : '{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'generator_mode' : 'SUPPLY_DRIVEN',
                                                  'generator_status' : 'ONLINE',
                                                  'panel_type' : 'SINGLE_CRYSTAL_SILICON',
                                                  'efficiency' : '0.2',
                                                  'area' : '{:.0f}'.format(floor_area)}
        
        # Determine total number of PV we must add to bigbox commercial
        if penetration_bigbox > 0:    # solar_bigbox_array = list(number of bigbox meters attached to com loads,list(bigbox meter names attached to loads),list(phases of bigbox meters attached to loads))
            total_bigbox_pv_units = math.ceil((config_data['emissions_peak'] * penetration_bigbox) / tech_data['solar_averagepower_bigbox'])
            total_bigbox_number = int(solar_bigbox_array[0])
            
            # Create a randomized list of numbers 0 to total_bigbox_number
            random_index = []
            random_index = random.sample(list(xrange(total_bigbox_number)),total_office_number);
            
            # Determine how many units to attach to each bigbox building
            pv_units_per_bigbox = int(math.ceil(total_bigbox_pv_units / total_bigbox_number))
            
            # Attach PV units to dictionary
            pv_unit = 0
            floor_area = round(solar_rating / (92.902 * 0.20))
            
            for x in xrange(total_bigbox_number):
                parent = solar_bigbox_array[1][random_index[x]]
                phases = solar_bigbox_array[2][random_index[x]]
                pv_unit = pv_unit + pv_units_per_bigbox
                
                if pv_unit < total_bigbox_number:
                    if pv_units_per_bigbox > (total_bigbox_number - pv_unit - 1):
                        pv_units_per_bigbox = total_bigbox_number - pv_unit - 1
                        
                    for y in xrange(pv_units_per_bigbox):
                        # Write the PV meter
                        last_key += 1
                        PV_Tech_Dict[last_key] = {'object' : 'meter',
                                                  'name' : 'pv_meter{:d}_{:s}'.format(y,parent),
                                                  'parent' : '{:s}'.format(parent),
                                                  'phases' : '{:s}'.format(phases),
                                                  'nominal_voltage' : '{:f}'.format(config_data['nom_volt2']),
                                                  'group_id' : 'Commercial_m_solar_bigbox'}
                        
                        # Write the PV inverter
                        last_key += 1
                        PV_Tech_Dict[last_key] = {'object' : 'inverter',
                                                  'name' : 'pv_inverter_{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'parent' : '{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'phases' : '{:s}'.format(phases),
                                                  'generator_mode' : 'CONSTANT_PF',
                                                  'generator_status' : 'ONLINE',
                                                  'inverter_type' : 'PWM',
                                                  'power_factor' : '1.0',
                                                  'rated_power' : '{:.0f}'.format(math.ceil(solar_rating))}
                                                  
                        # Write the PV inverter
                        last_key += 1
                        PV_Tech_Dict[last_key] = {'object' : 'solar',
                                                  'name' : 'sol_panel_{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'parent' : '{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'generator_mode' : 'SUPPLY_DRIVEN',
                                                  'generator_status' : 'ONLINE',
                                                  'panel_type' : 'SINGLE_CRYSTAL_SILICON',
                                                  'efficiency' : '0.2',
                                                  'area' : '{:.0f}'.format(floor_area)}
                                                  
        # Determine total number of PV we must add to stripmall commercial
        if penetration_stripmall > 0:    # solar_stripmall_array = list(number of stripmall meters attached to com loads,list(stripmall meter names attached to loads),list(phases of stripmall meters attached to loads))
            total_stripmall_pv_units = math.ceil((config_data['emissions_peak'] * penetration_stripmall) / tech_data['solar_averagepower_stripmall'])
            total_stripmall_number = int(solar_stripmall_array[0])
            
            # Create a randomized list of numbers 0 to total_stripmall_number
            random_index = []
            random_index = random.sample(list(xrange(total_stripmall_number)),total_office_number);
            
            # Determine how many units to attach to each stripmall building
            pv_units_per_stripmall = int(math.ceil(total_stripmall_pv_units / total_stripmall_number))
            
            # Attach PV units to dictionary
            pv_unit = 0
            floor_area = round(solar_rating / (92.902 * 0.20))
            
            for x in xrange(total_stripmall_number):
                parent = solar_stripmall_array[1][random_index[x]]
                phases = solar_stripmall_array[2][random_index[x]]
                pv_unit = pv_unit + pv_units_per_stripmall
                
                if pv_unit < total_stripmall_number:
                    if pv_units_per_stripmall > (total_stripmall_number - pv_unit - 1):
                        pv_units_per_stripmall = total_stripmall_number - pv_unit - 1
                        
                    for y in xrange(pv_units_per_stripmall):
                        # Write the PV meter
                        last_key += 1
                        PV_Tech_Dict[last_key] = {'object' : 'triplex_meter',
                                                  'name' : 'pv_triplex_meter{:d}_{:s}'.format(y,parent),
                                                  'parent' : '{:s}'.format(parent),
                                                  'phases' : '{:s}'.format(phases),
                                                  'nominal_voltage' : '120',
                                                  'group_id' : 'Commercial_tm_solar_stripmall'}
                        
                        # Write the PV inverter
                        last_key += 1
                        PV_Tech_Dict[last_key] = {'object' : 'inverter',
                                                  'name' : 'pv_inverter_{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'parent' : '{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'phases' : '{:s}'.format(phases),
                                                  'generator_mode' : 'CONSTANT_PF',
                                                  'generator_status' : 'ONLINE',
                                                  'inverter_type' : 'PWM',
                                                  'power_factor' : '1.0',
                                                  'rated_power' : '{:.0f}'.format(math.ceil(solar_rating))}
                                                  
                        # Write the PV inverter
                        last_key += 1
                        PV_Tech_Dict[last_key] = {'object' : 'solar',
                                                  'name' : 'sol_panel_{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'parent' : '{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'generator_mode' : 'SUPPLY_DRIVEN',
                                                  'generator_status' : 'ONLINE',
                                                  'panel_type' : 'SINGLE_CRYSTAL_SILICON',
                                                  'efficiency' : '0.2',
                                                  'area' : '{:.0f}'.format(floor_area)}
                                                  
    # Add Residential PV
    if use_flags['use_solar'] != 0 or use_flags['use_solar_res'] != 0:
        # Initialize psuedo-random seed
        random.seed(5)
        
        solar_rating = config_data['solar_rating']*1000 #Convert kW to W
        # Determine solar penetrations for residential
        if solar_residential_array != None:
            residential_penetration = config_data['solar_penetration']
        else:
            residential_penetration = 0
            
        # Determine total number of PV we must add to residential houses
        if residential_penetration > 0:    # solar_residential_array = list(number of residential triplex_meters attached to com loads,list(residential triplex_meter names attached to loads),list(phases of residential triplex_meters attached to loads))
            total_residential_pv_units = math.ceil((config_data['emissions_peak'] * residential_penetration) / tech_data['solar_averagepower_residential'])
            total_residential_number = int(solar_residential_array[0])
            
            # Create a randomized list of numbers 0 to total_residential_number
            random_index = []
            random_index = random.sample(list(xrange(total_residential_number)),total_office_number);
            
            # Determine how many units to attach to each residential house
            pv_units_per_residential = int(math.ceil(total_residential_pv_units / total_residential_number))
            
            # Attach PV units to dictionary
            pv_unit = 0
            floor_area = round(solar_rating / (92.902 * 0.20))
            
            for x in xrange(total_residential_number):
                parent = solar_residential_array[1][random_index[x]]
                phases = solar_residential_array[2][random_index[x]]
                pv_unit = pv_unit + pv_units_per_residential
                
                if pv_unit < total_residential_number:
                    if pv_units_per_residential > (total_residential_number - pv_unit - 1):
                        pv_units_per_residential = total_residential_number - pv_unit - 1
                        
                    for y in xrange(pv_units_per_residential):
                        # Write the PV meter
                        last_key += 1
                        PV_Tech_Dict[last_key] = {'object' : 'triplex_meter',
                                                  'name' : 'pv_triplex_meter{:d}_{:s}'.format(y,parent),
                                                  'parent' : '{:s}'.format(parent),
                                                  'phases' : '{:s}'.format(phases),
                                                  'nominal_voltage' : '120',
                                                  'group_id' : 'Residential_tm_solar'}
                        # Write the PV inverter
                        last_key += 1
                        PV_Tech_Dict[last_key] = {'object' : 'inverter',
                                                  'name' : 'pv_inverter_{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'parent' : '{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'phases' : '{:s}'.format(phases),
                                                  'generator_mode' : 'CONSTANT_PF',
                                                  'generator_status' : 'ONLINE',
                                                  'inverter_type' : 'PWM',
                                                  'power_factor' : '1.0',
                                                  'rated_power' : '{:.0f}'.format(math.ceil(solar_rating))}
                        # Write the PV inverter
                        last_key += 1
                        PV_Tech_Dict[last_key] = {'object' : 'solar',
                                                  'name' : 'sol_panel_{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'parent' : '{:s}'.format(PV_Tech_Dict[last_key-1]['name']),
                                                  'generator_mode' : 'SUPPLY_DRIVEN',
                                                  'generator_status' : 'ONLINE',
                                                  'panel_type' : 'SINGLE_CRYSTAL_SILICON',
                                                  'efficiency' : '0.2',
                                                  'area' : '{:.0f}'.format(floor_area)}
            
    return PV_Tech_Dict
            
def main():
    #tests here
    pass
    #PVGLM = Append_Solar(PV_Tech_Dict,use_flags,)
    
if __name__ ==  '__main__':
    main()