'''
Created on Jul 1, 2013

@author: fish334
'''
def add_normalized_residential_ziploads(loadshape_dict, residenntial_dict, config_data, last_key):
	for x in residenntial_dict.keys():
		tpload_name = residenntial_dict[x]['name']
		tpload_parent = residenntial_dict[x].get('parent', 'None')
		tpphases = residenntial_dict[x]['phases']
		tpnom_volt = '120.0'
		bp = residenntial_dict[x]['load']*config_data['normalized_loadshape_scalar']

		loadshape_dict[last_key] = {'object' : 'triplex_load',
											 'name' : '{:s}_loadshape'.format(tpload_name),
											 'phases' : tpphases,
											 'nominal_voltage' : tpnom_volt}
		if tpload_parent != 'None':
			loadshape_dict[last_key]['parent'] = tpload_parent
		else:
			loadshape_dict[last_key]['parent'] = tpload_name

		if bp > 0.0:         
			loadshape_dict[last_key]['base_power_12'] = 'norm_feeder_loadshape.value*{:f}'.format(bp)
			loadshape_dict[last_key]['power_pf_12'] = '{:f}'.format(config_data['r_p_pf'])
			loadshape_dict[last_key]['current_pf_12'] = '{:f}'.format(config_data['r_i_pf'])
			loadshape_dict[last_key]['impedance_pf_12'] = '{:f}'.format(config_data['r_z_pf'])
			loadshape_dict[last_key]['power_fraction_12'] = '{:f}'.format(config_data['r_pfrac'])
			loadshape_dict[last_key]['current_fraction_12'] = '{:f}'.format(config_data['r_ifrac'])
			loadshape_dict[last_key]['impedance_fraction_12'] = '{:f}'.format(config_data['r_zfrac'])

		last_key += last_key

	return (loadshape_dict, last_key)

def add_normalized_commercial_ziploads(loadshape_dict, commercial_dict, config_data, last_key):
	for x in commercial_dict.keys():
		load_name = commercial_dict[x]['name']
		load_parent = commercial_dict[x].get('parent', 'None')
		phases = commercial_dict[x]['phases']
		nom_volt = commercial_dict[x]['nom_volt']
		bp_A = commercial_dict[x]['load'][0]*config_data['normalized_loadshape_scalar']
		bp_B = commercial_dict[x]['load'][1]*config_data['normalized_loadshape_scalar']
		bp_C = commercial_dict[x]['load'][2]*config_data['normalized_loadshape_scalar']
		
		loadshape_dict[last_key] = {'object' : 'load',
					                               'name' : '{:s}_loadshape'.format(load_name),
					                               'phases' : phases,
					                               'nominal_voltage' : nom_volt}
		if load_parent != 'None':
			loadshape_dict[last_key]['parent'] = load_parent
		else:
			loadshape_dict[last_key]['parent'] = load_parent

		if 'A' in phases and bp_A > 0.0:         
			loadshape_dict[last_key]['base_power_A'] = 'norm_feeder_loadshape.value*{:f}'.format(bp_A)
			loadshape_dict[last_key]['power_pf_A'] = '{:f}'.format(config_data['c_p_pf'])
			loadshape_dict[last_key]['current_pf_A'] = '{:f}'.format(config_data['c_i_pf'])
			loadshape_dict[last_key]['impedance_pf_A'] = '{:f}'.format(config_data['c_z_pf'])
			loadshape_dict[last_key]['power_fraction_A'] = '{:f}'.format(config_data['c_pfrac'])
			loadshape_dict[last_key]['current_fraction_A'] = '{:f}'.format(config_data['c_ifrac'])
			loadshape_dict[last_key]['impedance_fraction_A'] = '{:f}'.format(config_data['c_zfrac'])

		if 'B' in phases and bp_B > 0.0:
			loadshape_dict[last_key]['base_power_B'] = 'norm_feeder_loadshape.value*{:f}'.format(bp_B)
			loadshape_dict[last_key]['power_pf_B'] = '{:f}'.format(config_data['c_p_pf'])
			loadshape_dict[last_key]['current_pf_B'] = '{:f}'.format(config_data['c_i_pf'])
			loadshape_dict[last_key]['impedance_pf_B'] = '{:f}'.format(config_data['c_z_pf'])
			loadshape_dict[last_key]['power_fraction_B'] = '{:f}'.format(config_data['c_pfrac'])
			loadshape_dict[last_key]['current_fraction_B'] = '{:f}'.format(config_data['c_ifrac'])
			loadshape_dict[last_key]['impedance_fraction_B'] = '{:f}'.format(config_data['c_zfrac'])

		if 'C' in phases and bp_C > 0.0:
			loadshape_dict[last_key]['base_power_C'] = 'norm_feeder_loadshape.value*{:f}'.format(bp_C)
			loadshape_dict[last_key]['power_pf_C'] = '{:f}'.format(config_data['c_p_pf'])
			loadshape_dict[last_key]['current_pf_C'] = '{:f}'.format(config_data['c_i_pf'])
			loadshape_dict[last_key]['impedance_pf_C'] = '{:f}'.format(config_data['c_z_pf'])
			loadshape_dict[last_key]['power_fraction_C'] = '{:f}'.format(config_data['c_pfrac'])
			loadshape_dict[last_key]['current_fraction_C'] = '{:f}'.format(config_data['c_ifrac'])
			loadshape_dict[last_key]['impedance_fraction_C'] = '{:f}'.format(config_data['c_zfrac'])

		last_key += last_key

	return (loadshape_dict, last_key)
