'''
Created on Jun 7, 2013

@author: fish334
'''

import TechnologyParameters

def add_recorders(recorder_dict,case_flag,use_mysql,use_tape,FeederID, last_key=0):
    # Check is last_key is already in glm dictionary
    def unused_key(last_key):
        if last_key in recorder_dict.keys():
            while last_key in recorder_dict.keys():
                last_key += 1
        
        return last_key
    
    use_flags = {}
    tech_data, use_flags = TechnologyParameters.TechnologyParametersFunc(case_flag, case_flag)
    
    have_resp_zips = 0
    have_unresp_zips = 0
    have_waterheaters = 0
    have_lights = 0
    have_plugs = 0
    have_gas_waterheaters = 0
    have_occupancy = 0
    swing_node = None
    climate_name = None
    
    
    for x in recorder_dict.keys():
        if 'object' in recorder_dict[x].keys():
            if recorder_dict[x]['object'] == 'transformer' and recorder_dict[x]['name'] == 'substation_transformer':
                swing_node = recorder_dict[x]['to']
                
            if recorder_dict[x]['object'] == 'climate':
                climate_name = recorder_dict[x]['name']
                
            if recorder_dict[x]['object'] == 'ZIPload':
                if 'groupid' in recorder_dict[x].keys():
                    if recorder_dict[x]['groupid'] == 'Responsive_load':
                        have_resp_zips = 1
                        
                    if recorder_dict[x]['groupid'] == 'Unresponsive_load':
                        have_unresp_zips = 1
                        
                    if recorder_dict[x]['groupid'] == 'Gas_waterheater':
                        have_gas_waterheaters = 1
                    
                    if recorder_dict[x]['groupid'] == 'Occupancy':
                        have_occupancy = 1
                
            if recorder_dict[x]['object'] == 'waterheater':
                have_waterheaters = 1
                        
    last_key = unused_key(last_key)            
    # Determine whether we are using mySQL or tape
    if use_mysql == 1:
        recorder_dict[last_key] = {'module' : 'mysql'}
        last_key = unused_key(last_key)
        
        recorder_dict[last_key] = {'object' : 'database'}
        last_key = unused_key(last_key)
        
        # Add mysql recorder to swing bus for calibration purposes
        recorder_dict[last_key] = {'object' : 'mysql.recorder',
                                   'file' : '{:s}_swing'.format(FeederID),
                                   'parent' : 'network_node',
                                   'property' : 'measured_real_power,measured_real_energy',
                                   'interval' : '300',
                                   'limit' : '11520'}
        last_key = unused_key(last_key)
    
    if use_tape == 1:
        # Measure substation transformer output
        recorder_dict[last_key] = {'object' : 'tape.recorder',
                                   'parent' : 'substation_transformer',
                                   'file' : '{:s}_transformer_power.csv'.format(FeederID),
                                   'interval' : '{:d}'.format(tech_data['meas_interval']),
                                   'limit' : '{:d}'.format(tech_data['meas_limit']),
                                   'property' : 'power_out_A.real,power_out_A.imag,power_out_B.real,power_out_B.imag,power_out_C.real,power_out_C.imag,power_out.real,power_out.imag,power_losses_A.real,power_losses_A.imag,power_losses_B.real,power_losses_B.imag,power_losses_C.real,power_losses_C.imag'}
        last_key = unused_key(last_key)
        
        # Measure Feeder Source Output
        if swing_node != None:
            recorder_dict[last_key] = {'object' : 'tape.recorder',
                                       'parent' : '{:s}'.format(swing_node),
                                       'file' : '{:s}_swing.csv'.format(FeederID),
                                       'interval' : '{:d}'.format(tech_data['meas_interval']),
                                       'limit' : '{:d}'.format(tech_data['meas_limit']),
                                       'property' : 'measured_current_A.real,measured_current_A.imag,measured_current_B.real,measured_current_B.imag,measured_current_C.real,measured_current_C.imag,measured_voltage_A.real,measured_voltage_A.imag,measured_voltage_B.real,measured_voltage_B.imag,measured_voltage_C.real,measured_voltage_C.imag,measured_real_power,measured_reactive_power'}
            last_key = unused_key(last_key)
        
        # Measure outside temperature
        if climate_name != None:
            recorder_dict[last_key] = {'object' : 'tape.recorder',
                                       'parent' : '{:s}'.format(climate_name),
                                       'file' : '{:s}_outside_temp.csv'.format(FeederID),
                                       'interval' : '{:d}'.format(tech_data['meas_interval']),
                                       'limit' : '{:d}'.format(tech_data['meas_limit']),
                                       'property' : 'temperature'}
            last_key = unused_key(last_key)
        
        # Measure residential data
        if have_resp_zips == 1:
            recorder_dict[last_key] = {'object' : 'tape.collector',
                                       'group' : '"class=ZIPload AND groupid=Responsive_load"',
                                       'file' : '{:s}_Res_responisve_load.csv'.format(FeederID),
                                       'interval' : '{:d}'.format(tech_data['meas_interval']),
                                       'limit' : '{:d}'.format(tech_data['meas_limit']),
                                       'property' : 'sum(base_power)'}
            last_key = unused_key(last_key)
            
        if have_unresp_zips == 1:
            recorder_dict[last_key] = {'object' : 'tape.collector',
                                       'group' : '"class=ZIPload AND groupid=Unresponsive_load"',
                                       'file' : '{:s}_Res_unresponisve_load.csv'.format(FeederID),
                                       'interval' : '{:d}'.format(tech_data['meas_interval']),
                                       'limit' : '{:d}'.format(tech_data['meas_limit']),
                                       'property' : 'sum(base_power)'}
            last_key = unused_key(last_key)
            
        if have_waterheaters == 1:
            recorder_dict[last_key] = {'object' : 'tape.collector',
                                       'group' : '"class=waterheater"',
                                       'file' : '{:s}_waterheater.csv'.format(FeederID),
                                       'interval' : '{:d}'.format(tech_data['meas_interval']),
                                       'limit' : '{:d}'.format(tech_data['meas_limit']),
                                       'property' : 'sum(actual_load)'}
            last_key = unused_key(last_key)
            
        if have_lights == 1:
            recorder_dict[last_key] = {'object' : 'tape.collector',
                                       'group' : '"class=ZIPload AND groupid=Lights"',
                                       'file' : '{:s}_lights.csv'.format(FeederID),
                                       'interval' : '{:d}'.format(tech_data['meas_interval']),
                                       'limit' : '{:d}'.format(tech_data['meas_limit']),
                                       'property' : 'sum(base_power)'}
            last_key = unused_key(last_key)
            
        if have_plugs == 1:
            recorder_dict[last_key] = {'object' : 'tape.collector',
                                       'group' : '"class=ZIPload AND groupid=Plugs"',
                                       'file' : '{:s}_plugs.csv'.format(FeederID),
                                       'interval' : '{:d}'.format(tech_data['meas_interval']),
                                       'limit' : '{:d}'.format(tech_data['meas_limit']),
                                       'property' : 'sum(base_power)'}
            last_key = unused_key(last_key)
            
        if have_gas_waterheaters == 1:
            recorder_dict[last_key] = {'object' : 'tape.collector',
                                       'group' : '"class=ZIPload AND groupid=Gas_waterheater"',
                                       'file' : '{:s}_gas_waterheater.csv'.format(FeederID),
                                       'interval' : '{:d}'.format(tech_data['meas_interval']),
                                       'limit' : '{:d}'.format(tech_data['meas_limit']),
                                       'property' : 'sum(base_power)'}
            last_key = unused_key(last_key)
            
        if have_occupancy == 1:
            recorder_dict[last_key] = {'object' : 'tape.collector',
                                       'group' : '"class=ZIPload AND groupid=Occupancy"',
                                       'file' : '{:s}_occupancy.csv'.format(FeederID),
                                       'interval' : '{:d}'.format(tech_data['meas_interval']),
                                       'limit' : '{:d}'.format(tech_data['meas_limit']),
                                       'property' : 'sum(base_power)'}
            last_key = unused_key(last_key)
            
        return (recorder_dict, last_key)
            

if __name__ == '__main__':
    pass