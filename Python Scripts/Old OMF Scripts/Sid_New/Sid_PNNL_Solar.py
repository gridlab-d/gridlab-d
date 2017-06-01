'''
Created on Nov 7, 2014

@author: srid966
'''
import feeder_parse_mod
import add_glm_object_dictionary


def identify_triplex(inputStr):
    
    names = []
    phases = []           
    house_data = feeder_parse_mod.parse(inputStr, filePath=True)    
    for k, v in house_data.iteritems():
        if 'triplex_meter' in v.values(): 
            names.append(v['name'])
            phases.append(v['phases'])
    output = zip(names, phases)
    return output



def solar_create(triplex_data, SCE_peak_load, Penetration):
    
    solar_object_capacity = (SCE_peak_load//Penetration)//len(triplex_data)
    
    solar_dict={}
    
    for i in triplex_data:
        tri_name = ['triplex_'+i[0]]
        groupid = ['solar_groupid']
        parent = [i[0]]
        bustype = ['PV']
        phases = [i[1]]
        nominal_voltage = [120]
           
        glm_parameters = tri_name + groupid + parent + bustype + phases + nominal_voltage
         
        solar_dict = add_glm_object_dictionary.create_glm_object_dictionary(solar_dict, 'triplex_meter', glm_parameters)     
         
        inv_name = ['inv_' + str(tri_name[0])]
        parent = tri_name
        generator_status = ['ONLINE']
        generator_mode = ['CONSTANT_PF']
        inverter_type = ['PWM']
        power_factor = [1.0]
        rated_power  = ['%d kVA'%(solar_object_capacity)]
         
        glm_parameters = inv_name + parent + generator_status + generator_mode + inverter_type + power_factor + rated_power
         
        solar_dict = add_glm_object_dictionary.create_glm_object_dictionary(solar_dict, 'inverter', glm_parameters)     
        
        solar_name = ['solar_'+str(tri_name[0])]
        parent = inv_name
        generator_status = ['ONLINE'] 
        generator_mode = ['SUPPLY_DRIVEN']
        panel_type = ['SINGLE_CRYSTAL_SILICON']   
        area = ['13 m^2']
        efficiency = [0.2]
          
        glm_parameters = solar_name + parent + generator_status + generator_mode + panel_type + area + efficiency    
              
        solar_dict = add_glm_object_dictionary.create_glm_object_dictionary(solar_dict, 'solar', glm_parameters)
        
    return solar_dict