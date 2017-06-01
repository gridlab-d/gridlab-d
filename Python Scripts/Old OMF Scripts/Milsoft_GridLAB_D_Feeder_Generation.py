#Python Extraction and Calibration Version of MATLAB Scripts
#Note: This assumes that dictionary being passed in already contains split-phase center-tapped transformers with spot loads ( triplex_nodes ) on the secondary side.
#Note: All triplex_node dictionaries must contain a load classification key which tells what type of houses are located at this spot load.
#Note: All swing node objects must have a Dictionary key
from __future__ import division
import ResidentialLoads
import CommercialLoads
import math
import random
import Configuration
import TechnologyParameters
import Solar_Technology
import feeder
import AddLoadShapes
import copy
import os


def GLD_Feeder(glmDict,case_flag,configuration_file=None):
	#glmDict is a dictionary containing all the objects in WindMIL model represented as equivalent GridLAB-D objects

	#case_flag is an integer indicating which technology case to tack on to the GridLAB-D model
	#	case_flag : technology
	#  -1 : Loadshape Case
	#	0 : Base Case
	#	1 : CVR
	#	2 : Automation
	#	3 : FDIR
	#	4 : TOU/CPP w/ tech
	#	5 : TOU/CPP w/o tech
	#	6 : TOU w/ tech
	#	7 : TOU w/o tech
	#	8 : DLC
	#	9 : Thermal Storage
	#  10 : PHEV
	#  11 : Solar Residential
	#  12 : Solar Commercial
	#  13 : Solar Combined

	#configuration_file is the name of the file to use for getting feeder information

	#GLD_Feeder returns a dictionary, glmCaseDict, similar to glmDict with additional object dictionaries added according to the case_flag selected.

	random.seed(1)
	# Check to make sure we have a valid case flag
	if case_flag < -1:
		case_flag = 0

	if case_flag > 13:
		case_flag = 13
	#print("Calling configuration.py\n")
	# Get information about each feeder from Configuration() and  TechnologyParameters()
	config_data = Configuration.ConfigurationFunc(configuration_file,None)

	#set up default flags
	use_flags = {}
	#print("Calling TechnologyParameters.py\n")
	tech_data,use_flags = TechnologyParameters.TechnologyParametersFunc(use_flags,case_flag)

	#tmy = 'schedules\\\\SCADA_weather_ISW_gld.csv'
	tmy = config_data['weather']

	#find name of swingbus of static model dictionary
	hit = 0
	for x in glmDict:
		if hit < 1 and 'bustype' in glmDict[x] and glmDict[x]['bustype'] == 'SWING':
			hit = 1
			swing_bus_name = glmDict[x]['name']
			nom_volt = glmDict[x]['nominal_voltage'] # Nominal voltage in V

	# Create new case dictionary
	glmCaseDict = {}
	last_key = len(glmCaseDict)

	# Create clock dictionary
	glmCaseDict[last_key] = {'clock' : '',
							 'timezone' : '{:s}'.format(config_data['timezone']),
							 'starttime' : "'{:s}'".format(config_data['startdate']),
							 'stoptime' : "'{:s}'".format(config_data['stopdate'])}
	last_key += 1

	# Create dictionaries of preprocessor directives
	if use_flags['use_homes'] != 0:
		glmCaseDict[last_key] = {'#include' : os.path.abspath('./schedules/appliance_schedules.glm').replace('\\', '/')}
		last_key += 1
		glmCaseDict[last_key] = {'#include' : os.path.abspath('./schedules/water_and_setpoint_schedule_v5.glm').replace('\\', '/')}
		last_key += 1

	if use_flags['use_battery'] == 1 or use_flags['use_battery'] == 2:
		glmCaseDict[last_key] = {'#include' : os.path.abspath('./schedules/battery_schedule.glm').replace('\\', '/')}
		last_key += 1

	if use_flags['use_commercial'] == 1:
		glmCaseDict[last_key] = {'#include' : os.path.abspath('./schedules/commercial_schedules.glm').replace('\\', '/')}
		last_key += 1

	if use_flags['use_market'] == 1 or use_flags['use_market'] == 2:
		glmCaseDict[last_key] = {'#include' : os.path.abspath('./schedules/daily_elasticity_schedules.glm').replace('\\', '/')}
		last_key += 1

	if use_flags['use_ts'] == 2 or use_flags['use_ts'] == 4:
		glmCaseDict[last_key] = {'#include' : os.path.abspath('./schedules/thermal_storage_schedule_R{:d}.glm'.format(config_data['region'])).replace('\\', '/')}
		last_key += 1

	glmCaseDict[last_key] = {'#define' : 'stylesheet=http://gridlab-d.svn.sourceforge.net/viewvc/gridlab-d/trunk/core/gridlabd-2_0'}
	last_key += 1

	glmCaseDict[last_key] = {'#set' : 'minimum_timestep=60'}
	last_key += 1

	glmCaseDict[last_key] = {'#set' : 'profiler=1'}
	last_key += 1

	glmCaseDict[last_key] = {'#set' : 'relax_naming_rules=1'}
	last_key += 1

	# Create dictionaries of modules to be used from the case_flag
	glmCaseDict[last_key] = {'module' : 'tape'}
	last_key += 1

	glmCaseDict[last_key] = {'module' : 'climate'}
	last_key += 1

	glmCaseDict[last_key] = {'module' : 'residential',
							 'implicit_enduses' : 'NONE'}
	last_key += 1

	glmCaseDict[last_key] = {'module' : 'powerflow',
							 'solver_method' : 'NR',
							 'NR_iteration_limit' : '50'}
	last_key += 1

	if use_flags['use_solar'] != 0 or use_flags['use_solar_res'] != 0 or use_flags['use_solar_com'] != 0 or use_flags['use_battery'] == 1 or use_flags['use_battery'] == 2:
		glmCaseDict[last_key] = {'module' : 'generators'}
		last_key += 1

	if use_flags['use_market'] != 0:
		glmCaseDict[last_key] = {'module' : 'market'}
		last_key += 1

	# Add the class player dictionary to glmCaseDict
	glmCaseDict[last_key] = {'class' : 'player',
							 'variable_types' : ['double'],
							 'variable_names' : ['value']}
	last_key += 1
	
	if use_flags['use_normalized_loadshapes'] == 1:
		glmCaseDict[last_key] = {'object' : 'player',
								 'name' : 'norm_feeder_loadshape',
								 'property' : 'value',
								 'file' : '{:s}'.format(config_data['load_shape_norm']),
								 'loop' : '14600',
								 'comment' : '// Will loop file for 40 years assuming the file has data for a 24 hour period'}
		last_key += 1

	# Include the objects for the TOU case flags
	if use_flags['use_market'] != 0:
		# Add class auction dictionary to glmCaseDict
		glmCaseDict[last_key] = {'class' : 'auction',
								 'variable_types' : ['double','double'],
								 'variable_names' : ['my_avg','my_std']}
		last_key += 1

		# Add CPP player dictionary to glmCaseDict
		CPP_flag_name = config_data['CPP_flag'] # Strip '.player' from config_data['CPP_flag']
		CPP_flag_name.replace('.player','')
		glmCaseDict[last_key] = {'object' : 'player',
								 'name' : CPP_flag_name,
								 'file' : config_data['CPP_flag']}
		last_key += 1

		# Add auction object dictionary to glmCaseDict
		# Determine which stat to use for my_avg and my_std
		if use_flags['use_market'] == 1: #TOU
			temp_avg = config_data['TOU_stats'][0]
			temp_std = config_data['TOU_stats'][1]
		elif use_flags['use_market'] == 2 or use_flags['use_market'] == 3: #TOU/CPP
			temp_avg = config_data['CPP_stats'][0]
			temp_std = config_data['CPP_stats'][1]

		glmCaseDict[last_key] = {'object' : 'auction',
								 'name' : tech_data['market_info'][0],
								 'period' : "{:.0f}".format(tech_data['market_info'][1]),
								 'special_mode' : 'BUYERS_ONLY',
								 'unit' : 'kW',
								 'my_avg' : temp_avg,
								 'my_std' : temp_std}
		parent_key = last_key
		last_key += 1

		# Add player object dictionary for the auction object's clearing price
		# Determine which file to use for the clear_price
		if use_flags['use_market'] == 1: #TOU
			auction_price_file = config_data['TOU_price_player']
		elif use_flags['use_market'] == 2 or use_flags['use_market'] == 3: #TOU/CPP
			auction_price_file = config_data['CPP_price_player']

		glmCaseDict[last_key] = {'object' : 'player',
								 'parent' : glmCaseDict[parent_key]['name'],
								 'file' : auction_price_file,
								 'loop' : '10',
								 'property' : 'current_market.clearing_price'}
		last_key += 1
	else:
		CPP_flag_name = None;

	# Add climate dictionaries
	if '.csv' in tmy:
		# Climate file is a cvs file. Need to add csv_reader object
		glmCaseDict[last_key] = {'object' : 'csv_reader',
								 'name' : 'CsvReader',
								 'filename' : '{:s}'.format(tmy)}
		last_key += 1
	glmCaseDict[last_key] = {'object' : 'climate',
							 'name' : 'ClimateWeather',
							 'tmyfile' : '{:s}'.format(tmy)}
	if '.tmy2' in tmy:
		glmCaseDict[last_key]['interpolate'] = 'QUADRATIC'
	elif '.csv' in tmy:
		glmCaseDict[last_key]['reader'] = 'CsvReader'
	last_key += 1

	# Add substation transformer transformer_configuration
	glmCaseDict[last_key] = {'object' : 'transformer_configuration',
							 'name' : 'trans_config_to_feeder',
							 'connect_type' : 'WYE_WYE',
							 'install_type' : 'PADMOUNT',
							 'primary_voltage' : '{:d}'.format(config_data['nom_volt']),
							 'secondary_voltage' : '{:s}'.format(nom_volt),
							 'power_rating' : '{:.1f} MVA'.format(config_data['feeder_rating']),
							 'impedance' : '0.00033+0.0022j'}
	last_key += 1

	# Add CVR controller
	if use_flags['use_vvc'] == 1:
		# TODO: pull all of these out and put into TechnologyParameters()
		glmCaseDict[last_key] = {'object' : 'volt_var_control',
								 'name' : 'volt_var_control',
								 'control_method' : 'ACTIVE',
								 'capacitor_delay' : '60.0',
								 'regulator_delay' : '60.0',
								 'desired_pf' : '0.99',
								 'd_max' : '0.8',
								 'd_min' : '0.1',
								 'substation_link' : 'substation_transfromer',
								 'regulator_list' : '"{:s}"'.format(','.join(config_data['regulators'])), # config_data['regulators'] should contain a list of the names of the regulators that use CVR
								 'maximum_voltage' : '{:.2f}'.format(config_data['voltage_regulation'][2]),
								 'minimum_voltage' : '{:.2f}'.format(config_data['voltage_regulation'][1]),
								 'max_vdrop' : '50',
								 'high_load_deadband' :'{:.2f}'.format(config_data['voltage_regulation'][4]),
								 'desired_voltages' : '{:.2f}'.format(config_data['voltage_regulation'][0]),
								 'low_load_deadband' : '{:.2f}'.format(config_data['voltage_regulation'][3])}

		num_caps = len(config_data['capacitor_outage'])
		if num_caps > 0:
			glmCaseDict[last_key]['capacitor_list'] = '"'

			for x in xrange(num_caps):
				if x < (num_caps - 1):
					glmCaseDict[last_key]['capacitor_list'] = glmCaseDict[last_key]['capacitor_list'] + '{:s},'.format(config_data['capacitor_outage'][x][0])
				else:
					glmCaseDict[last_key]['capacitor_list'] = glmCaseDict[last_key]['capacitor_list'] + '{:s}"'.format(config_data['capacitor_outage'][x][0])
		else:
			glmCaseDict[last_key]['capacitor_list'] = '""'

		num_eol = len(config_data['EOL_points'])
		glmCaseDict[last_key]['voltage_measurements'] = '"'

		for x in xrange(num_eol):
			if x < (num_eol - 1):
				glmCaseDict[last_key]['voltage_measurements'] = glmCaseDict[last_key]['voltage_measurements'] + '{:s},{:d},'.format(config_data['EOL_points'][x][0],config_data['EOL_points'][x][2])
			else:
				glmCaseDict[last_key]['voltage_measurements'] = glmCaseDict[last_key]['voltage_measurements'] + '{:s},{:d}"'.format(config_data['EOL_points'][x][0],config_data['EOL_points'][x][2])
		last_key += 1

	# Add substation swing bus and substation transformer dictionaries
	glmCaseDict[last_key] = {'object' : 'meter',
							 'name' : 'network_node',
							 'bustype' : 'SWING',
							 'nominal_voltage' : '{:d}'.format(config_data['nom_volt']),
							 'phases' : 'ABCN'}
	# Add transmission voltage players
	parent_key = last_key
	last_key += 1

	glmCaseDict[last_key] = {'object' : 'player',
							 'property' : 'voltage_A',
							 'parent' : '{:s}'.format(glmCaseDict[parent_key]['name']),
							 'loop' : '10',
							 'file' : '{:s}'.format(config_data["voltage_players"][0])}
	last_key += 1

	glmCaseDict[last_key] = {'object' : 'player',
							 'property' : 'voltage_B',
							 'parent' : '{:s}'.format(glmCaseDict[parent_key]['name']),
							 'loop' : '10',
							 'file' : '{:s}'.format(config_data["voltage_players"][1])}
	last_key += 1

	glmCaseDict[last_key] = {'object' : 'player',
							 'property' : 'voltage_C',
							 'parent' : '{:s}'.format(glmCaseDict[parent_key]['name']),
							 'loop' : '10',
							 'file' : '{:s}'.format(config_data["voltage_players"][2])}
	last_key += 1

	glmCaseDict[last_key] = {'object' : 'transformer',
							 'name' : 'substation_transformer',
							 'from' : 'network_node',
							 'to' : '{:s}'.format(swing_bus_name),
							 'phases' : 'ABCN',
							 'configuration' : 'trans_config_to_feeder'}
	last_key += 1

	# Copy static powerflow model glm dictionary into case dictionary
	for x in glmDict:
		if 'clock' not in glmDict[x].keys() and '#set' not in glmDict[x].keys() and '#define' not in glmDict[x].keys() and 'module' not in glmDict[x].keys() and 'omftype' not in glmDict[x].keys():
			glmCaseDict[last_key] = copy.deepcopy(glmDict[x])
			# Remove original swing bus from static model
			if 'bustype' in glmCaseDict[last_key].keys() and glmCaseDict[last_key]['bustype'] == 'SWING':
				swing_node = glmCaseDict[last_key]['name']
				del glmCaseDict[last_key]['bustype']
				glmCaseDict[last_key]['object'] = 'meter'
			last_key += 1


	# Add groupid's to lines and transformers
	for x in glmCaseDict:
		if 'object' in glmCaseDict[x]:
			if glmCaseDict[x]['object'] == 'triplex_line':
				glmCaseDict[x]['groupid'] = 'Triplex_Line'
			elif glmCaseDict[x]['object'] == 'transformer':
				glmCaseDict[x]['groupid'] = 'Distribution_Trans'
			elif glmCaseDict[x]['object'] == 'overhead_line' or glmCaseDict[x]['object'] == 'underground_line':
				glmCaseDict[x]['groupid'] = 'Distribution_Line'
	
	#print('finished copying base glm\n')

	# Create dictionary that houses the number of commercial 'load' objects where commercial house objects will be tacked on.
	total_commercial_number = 0 # Variable that stores the total amount of houses that need to be added.
	commercial_dict = {}
	if use_flags['use_commercial'] == 1:
		commercial_key = 0

		for x in glmCaseDict:
			if 'object' in glmCaseDict[x] and glmCaseDict[x]['object'] == 'load':
				commercial_dict[commercial_key] = {'name' : glmCaseDict[x]['name'],
												   'parent' : 'None',
												   'load_classification' : 'None',
												   'load' : [0,0,0],
												   'number_of_houses' : [0,0,0], #[phase A, phase B, phase C]
												   'nom_volt' : glmCaseDict[x]['nominal_voltage'],
												   'phases' : glmCaseDict[x]['phases']}
				
				if 'parent' in glmCaseDict[x]:
					commercial_dict[commercial_key]['parent'] = glmCaseDict[x]['parent']

				if 'load_class' in glmCaseDict[x]:
					commercial_dict[commercial_key]['load_classification'] = glmCaseDict[x]['load_class']

				# Figure out how many houses should be attached to this load object
				load_A = 0
				load_B = 0
				load_C = 0

				# determine the total ZIP load for each phase
				if 'constant_power_A' in glmCaseDict[x]:
					c_num = complex(glmCaseDict[x]['constant_power_A'])
					load_A += abs(c_num)

				if 'constant_power_B' in glmCaseDict[x]:
					c_num = complex(glmCaseDict[x]['constant_power_B'])
					load_B += abs(c_num)

				if 'constant_power_C' in glmCaseDict[x]:
					c_num = complex(glmCaseDict[x]['constant_power_C'])
					load_C += abs(c_num)

				if 'constant_impedance_A' in glmCaseDict[x]:
					c_num = complex(glmCaseDict[x]['constant_impedance_A'])
					load_A += pow(commercial_dict[commercial_key]['nom_volt'],2)/(3*abs(c_num))

				if 'constant_impedance_B' in glmCaseDict[x]:
					c_num = complex(glmCaseDict[x]['constant_impedance_B'])
					load_B += pow(commercial_dict[commercial_key]['nom_volt'],2)/(3*abs(c_num))

				if 'constant_impedance_C' in glmCaseDict[x]:
					c_num = complex(glmCaseDict[x]['constant_impedance_C'])
					load_C += pow(commercial_dict[commercial_key]['nom_volt'],2)/(3*abs(c_num))

				if 'constant_current_A' in glmCaseDict[x]:
					c_num = complex(glmCaseDict[x]['constant_current_A'])
					load_A += commercial_dict[commercial_key]['nom_volt']*(abs(c_num))

				if 'constant_current_B' in glmCaseDict[x]:
					c_num = complex(glmCaseDict[x]['constant_current_B'])
					load_B += commercial_dict[commercial_key]['nom_volt']*(abs(c_num))

				if 'constant_current_C' in glmCaseDict[x]:
					c_num = complex(glmCaseDict[x]['constant_current_C'])
					load_C += commercial_dict[commercial_key]['nom_volt']*(abs(c_num))

				if load_A >= tech_data['load_cutoff']:
					commercial_dict[commercial_key]['number_of_houses'][0] = int(math.ceil(load_A/config_data['avg_commercial']))
					total_commercial_number += commercial_dict[commercial_key]['number_of_houses'][0]

				if load_B >= tech_data['load_cutoff']:
					commercial_dict[commercial_key]['number_of_houses'][1] = int(math.ceil(load_B/config_data['avg_commercial']))
					total_commercial_number += commercial_dict[commercial_key]['number_of_houses'][1]

				if load_C >= tech_data['load_cutoff']:
					commercial_dict[commercial_key]['number_of_houses'][2] = int(math.ceil(load_C/config_data['avg_commercial']))
					total_commercial_number += commercial_dict[commercial_key]['number_of_houses'][2]

				commercial_dict[commercial_key]['load'][0] = load_A
				commercial_dict[commercial_key]['load'][1] = load_B
				commercial_dict[commercial_key]['load'][2] = load_C

				# TODO: Bypass this is load rating is known
				# Determine load_rating
				standard_transformer_rating = [10,15,25,37.5,50,75,100,150,167,250,333.3,500,666.7]
				total_load = (load_A + load_B + load_C)/1000
				load_rating = 0
				for y in standard_transformer_rating:
					if y >= total_load:
						load_rating = y
					elif y == 666.7:
						load_rating = y

				# Deterimine load classification
				if commercial_dict[commercial_key]['load_classification'].isdigit():
					commercial_dict[commercial_key]['load_classification'] = int(commercial_dict[commercial_key]['load_classification'])
				else: # load_classification is unknown determine from no_houses and transformer size
					#TODO: Determine what classID should be from no_houses and transformer size
					commercial_dict[commercial_key]['load_classification'] = None
					random_class_number = random.random()*100
					if load_rating == 10:
						commercial_dict[commercial_key]['load_classification'] = 6
					elif load_rating == 15:
						commercial_dict[commercial_key]['load_classification'] = 6
					elif load_rating == 25:
						if random_class_number <= 5:
							commercial_dict[commercial_key]['load_classification'] = 7
						elif 5 < random_class_number <= 11:
							commercial_dict[commercial_key]['load_classification'] = 8
						elif random_class_number > 11:
							commercial_dict[commercial_key]['load_classification'] = 6
					elif load_rating == 37.5:
						commercial_dict[commercial_key]['load_classification'] = 6
					elif load_rating == 50:
						if random_class_number <= 28:
							commercial_dict[commercial_key]['load_classification'] = 7
						elif 28 < random_class_number <= 61:
							commercial_dict[commercial_key]['load_classification'] = 8
						elif random_class_number > 61:
							commercial_dict[commercial_key]['load_classification'] = 6
					elif load_rating == 75:
						if random_class_number <= 18:
							commercial_dict[commercial_key]['load_classification'] = 6
						elif 18 < random_class_number <= 49:
							commercial_dict[commercial_key]['load_classification'] = 8
						elif random_class_number > 49:
							commercial_dict[commercial_key]['load_classification'] = 7
					elif load_rating == 100:
						if random_class_number <= 15:
							commercial_dict[commercial_key]['load_classification'] = 6
						elif 15 < random_class_number <= 56:
							commercial_dict[commercial_key]['load_classification'] = 7
						elif random_class_number > 56:
							commercial_dict[commercial_key]['load_classification'] = 8
					elif load_rating == 150:
						commercial_dict[commercial_key]['load_classification'] = 6
					elif load_rating == 167:
						if random_class_number <= 11:
							commercial_dict[commercial_key]['load_classification'] = 6
						elif 11 < random_class_number <= 43:
							commercial_dict[commercial_key]['load_classification'] = 8
						elif random_class_number > 43:
							commercial_dict[commercial_key]['load_classification'] = 7
					elif load_rating == 250:
						if random_class_number <= 27:
							commercial_dict[commercial_key]['load_classification'] = 7
						elif random_class_number > 27:
							commercial_dict[commercial_key]['load_classification'] = 8
					elif load_rating == 333.3:
						if random_class_number <= 17:
							commercial_dict[commercial_key]['load_classification'] = 7
						elif random_class_number > 17:
							commercial_dict[commercial_key]['load_classification'] = 8
					elif load_rating == 500:
						commercial_dict[commercial_key]['load_classification'] = 8
					elif load_rating == 666.7:
						if random_class_number <= 50:
							commercial_dict[commercial_key]['load_classification'] = 7
						elif random_class_number > 50:
							commercial_dict[commercial_key]['load_classification'] = 8
					else:
						if sum(commercial_dict[commercial_key]['number_of_houses']) >= 15:
							commercial_dict[commercial_key]['load_classification'] = 8
						elif sum(commercial_dict[commercial_key]['number_of_houses']) >= 6:
							commercial_dict[commercial_key]['load_classification'] = 7
						elif sum(commercial_dict[commercial_key]['number_of_houses']) > 0:
							commercial_dict[commercial_key]['load_classification'] = 6
						else:
							commercial_dict[commercial_key]['load_classification'] = None

				# Replace load with a node remove constant load keys
				glmCaseDict[x]['object'] = 'node'
				if 'load_class' in glmCaseDict[x]:
					del glmCaseDict[x]['load_class'] # Must remove load_class as it isn't a published property

				if 'constant_power_A' in glmCaseDict[x]:
					del glmCaseDict[x]['constant_power_A']

				if 'constant_power_B' in glmCaseDict[x]:
					del glmCaseDict[x]['constant_power_B']

				if 'constant_power_C' in glmCaseDict[x]:
					del glmCaseDict[x]['constant_power_C']

				if 'constant_impedance_A' in glmCaseDict[x]:
					del glmCaseDict[x]['constant_impedance_A']

				if 'constant_impedance_B' in glmCaseDict[x]:
					del glmCaseDict[x]['constant_impedance_B']

				if 'constant_impedance_C' in glmCaseDict[x]:
					del glmCaseDict[x]['constant_impedance_C']

				if 'constant_current_A' in glmCaseDict[x]:
					del glmCaseDict[x]['constant_current_A']

				if 'constant_current_B' in glmCaseDict[x]:
					del glmCaseDict[x]['constant_current_B']

				if 'constant_current_C' in glmCaseDict[x]:
					del glmCaseDict[x]['constant_current_C']

				commercial_key += 1
				
	#print('finished collecting commercial load objects\n')

	# Create dictionary that houses the number of residential 'load' objects where residential house objects will be tacked on.
	total_house_number = 0
	residential_dict = {}
	if use_flags['use_homes'] == 1:
		residential_key = 0
		for x in glmCaseDict:
			if 'object' in glmCaseDict[x] and glmCaseDict[x]['object'] == 'triplex_node':
				if 'power_1' in glmCaseDict[x] or 'power_12' in glmCaseDict[x]:
					residential_dict[residential_key] = {'name' : glmCaseDict[x]['name'],
														 'parent' : 'None',
														 'load_classification' : 'None',
														 'number_of_houses' : 0,
														 'load' : 0,
														 'large_vs_small' : 0.0,
														 'phases' : glmCaseDict[x]['phases']}
					
					if 'parent' in glmCaseDict[x]:
						residential_dict[residential_key]['parent'] = glmCaseDict[x]['parent']

					if 'load_class' in glmCaseDict[x]:
						residential_dict[residential_key]['load_classification'] = glmCaseDict[x]['load_class']

					# Figure out how many houses should be attached to this load object
					load = 0
					# determine the total ZIP load for each phase
					if 'power_1' in glmCaseDict[x]:
						c_num = complex(glmCaseDict[x]['power_1'])
						load += abs(c_num)

					if 'power_12' in glmCaseDict[x]:
						c_num = complex(glmCaseDict[x]['power_12'])
						load += abs(c_num)
					
					residential_dict[residential_key]['load'] = load	
					residential_dict[residential_key]['number_of_houses'] = int(round(load/config_data['avg_house']))
					total_house_number += residential_dict[residential_key]['number_of_houses']
					# Determine whether we rounded down of up to help determine the square footage (neg. number => small homes)
					residential_dict[residential_key]['large_vs_small'] = load/config_data['avg_house'] - residential_dict[residential_key]['number_of_houses']

					# Determine load_rating
					standard_transformer_rating = [10,15,25,37.5,50,75,100,150,167,250,333.3,500,666.7]
					total_load = load/1000
					load_rating = 0
					for y in standard_transformer_rating:
						if y >= total_load:
							load_rating = y
						elif y == 666.7:
							load_rating = y

					# Deterimine load classification
					if residential_dict[residential_key]['load_classification'].isdigit():
						residential_dict[residential_key]['load_classification'] = int(residential_dict[residential_key]['load_classification'])
					else: # load_classification is unknown determine from no_houses and transformer size
						#TODO: Determine what classID should be from no_houses and transformer size
						residential_dict[residential_key]['load_classification'] = None
						random_class_number = random.random()*100
						if load_rating == 10:
							if random_class_number <= 6:
								residential_dict[residential_key]['load_classification'] = 5
							elif 6 < random_class_number <= 24:
								residential_dict[residential_key]['load_classification'] = 0
							elif random_class_number > 24:
								residential_dict[residential_key]['load_classification'] = 1
						elif load_rating == 15:
							if random_class_number <= 14:
								residential_dict[residential_key]['load_classification'] = 1
							elif 14 < random_class_number <= 57:
								residential_dict[residential_key]['load_classification'] = 0
							elif random_class_number > 57:
								residential_dict[residential_key]['load_classification'] = 5
						elif load_rating == 25:
							if random_class_number <= 1.5:
								residential_dict[residential_key]['load_classification'] = 2
							elif 1.5 < random_class_number <= 3.7:
								residential_dict[residential_key]['load_classification'] = 3
							elif 3.7 < random_class_number <= 16.6:
								residential_dict[residential_key]['load_classification'] = 5
							elif 16.6 < random_class_number <= 38.7:
								residential_dict[residential_key]['load_classification'] = 0
							elif random_class_number > 38.7:
								residential_dict[residential_key]['load_classification'] = 1
						elif load_rating == 37.5:
							residential_dict[residential_key]['load_classification'] = 2
						elif load_rating == 50:
							if random_class_number <= 3.1:
								residential_dict[residential_key]['load_classification'] = 2
							elif 3.1 < random_class_number <= 16.7:
								residential_dict[residential_key]['load_classification'] = 5
							elif 16.7 < random_class_number <= 30.6:
								residential_dict[residential_key]['load_classification'] = 0
							elif 30.6 < random_class_number <= 51.6:
								residential_dict[residential_key]['load_classification'] = 3
							elif random_class_number > 51.6:
								residential_dict[residential_key]['load_classification'] = 1
						elif load_rating == 75:
							if random_class_number <= 7.9:
								residential_dict[residential_key]['load_classification'] = 1
							elif 7.9 < random_class_number <= 15.8:
								residential_dict[residential_key]['load_classification'] = 2
							elif 15.8 < random_class_number <= 26.2:
								residential_dict[residential_key]['load_classification'] = 0
							elif 26.2 < random_class_number <= 56.7:
								residential_dict[residential_key]['load_classification'] = 5
							elif random_class_number > 56.7:
								residential_dict[residential_key]['load_classification'] = 3
						elif load_rating == 100:
							if random_class_number <= 4:
								residential_dict[residential_key]['load_classification'] = 1
							elif 4 < random_class_number <= 12:
								residential_dict[residential_key]['load_classification'] = 0
							elif 12 < random_class_number <= 38:
								residential_dict[residential_key]['load_classification'] = 3
							elif 38 < random_class_number <= 68:
								residential_dict[residential_key]['load_classification'] = 2
							elif random_class_number > 68:
								residential_dict[residential_key]['load_classification'] = 5
						elif load_rating == 167:
							if random_class_number <= 1:
								residential_dict[residential_key]['load_classification'] = 1
							elif 1 < random_class_number <= 2:
								residential_dict[residential_key]['load_classification'] = 2
							elif 2 < random_class_number <= 4:
								residential_dict[residential_key]['load_classification'] = 0
							elif 4 < random_class_number <= 13:
								residential_dict[residential_key]['load_classification'] = 3
							elif random_class_number > 13:
								residential_dict[residential_key]['load_classification'] = 5
						else:
							residential_dict[residential_key]['load_classification'] = random.choice([0, 1, 2, 3, 4, 5]) # randomly pick between the 6 residential classifications

					# Remove constant load keys
					if 'load_class' in glmCaseDict[x]:
						del glmCaseDict[x]['load_class'] # Must remove load_class as it isn't a published property

					if 'power_12' in glmCaseDict[x]:
						del glmCaseDict[x]['power_12']

					if 'power_1' in glmCaseDict[x]:
						del glmCaseDict[x]['power_1']

					if total_house_number == 0 and load > 0 and use_flags['use_normalized_loadshapes'] == 0: # Residential street light
						glmCaseDict[x]['power_12_real'] = 'street_lighting*{:.4f}'.format(c_num.real*tech_data['light_scalar_res'])
						glmCaseDict[x]['power_12_reac'] = 'street_lighting*{:.4f}'.format(c_num.imag*tech_data['light_scalar_res'])

					residential_key += 1
					
	#print('finished collecting residential load objects\n')

	# Calculate some random numbers needed for TOU/CPP and DLC technologies
	if use_flags['use_market'] != 0:
		# Initialize psuedo-random seed
		random.seed(2)

		if total_house_number > 0:
			# Initialize random number arrays
			market_penetration_random = []
			dlc_rand = []
			pool_pump_recovery_random = []
			slider_random = []
			comm_slider_random = []
			dlc_c_rand = []
			dlc_c_rand2 = []

			# Make a large array so we don't run out
			if len(residential_dict) > 0:
				aa = len(residential_dict)*total_house_number
				for x in xrange(aa):
					market_penetration_random.append(random.random())
					dlc_rand.append(random.random()) # Used for dlc randomization

					# 10 - 25% increase over normal cycle
					pool_pump_recovery_random.append(0.1 + 0.15*random.random())

					# Limit slider randomization to Olypen style
					slider_random.append(random.normalvariate(0.45,0.2))
					if slider_random[x] > tech_data['market_info'][4]:
						slider_random[x] = tech_data['market_info'][4]
					if slider_random[x] < 0:
						slider_random[x] = 0

				# Random elasticity values for responsive loads - this is a multiplier
				# used to randomized individual building responsiveness - very similar
				# to a lognormal, so we'll use one that has a mean of ~1, max of
				# ~1.2, and median of ~1.12
				sigma = 1.2
				mu = 0.7
				multiplier = 3.6
				xval = []
				elasticity_random = []
				for x in xrange(aa):
					xval.append(random.random())
					elasticity_random.append(multiplier * math.exp(-1 / (2 * pow(sigma,2))) * pow((math.log(xval[x]) - mu),2) / (xval[x] * sigma * math.sqrt(2 * math.pi)))

			if len(commercial_dict) > 0:
				aa = len(commercial_dict)*15*100
				for x in xrange(aa):
					# Limit slider randomization to Olypen style
					comm_slider_random.append(random.normalvariate(0.45,0.2))
					if comm_slider_random[x] > tech_data['market_info'][4]:
						comm_slider_random[x] = tech_data['market_info'][4]
					if comm_slider_random[x] < 0:
						comm_slider_random[x] = 0

					dlc_c_rand.append(random.random())
					dlc_c_rand2.append(random.random())
			
		else:
			market_penetration_random = None
			dlc_rand = None
			pool_pump_recovery_random = None
			slider_random = None
			comm_slider_random = None
			dlc_c_rand = None
			dlc_c_rand2 = None
			xval = None
			elasticity_random = None
	else:
		market_penetration_random = None
		dlc_rand = None
		pool_pump_recovery_random = None
		slider_random = None
		comm_slider_random = None
		dlc_c_rand = None
		dlc_c_rand2 = None
		xval = None
		elasticity_random = None

	# Tack on residential loads
	if use_flags['use_homes'] != 0:
		#print('calling ResidentialLoads.py\n')
		if use_flags['use_normalized_loadshapes'] == 1:
			glmCaseDict, last_key = AddLoadShapes.add_normalized_residential_ziploads(glmCaseDict, residential_dict, config_data, last_key)
		else:
			glmCaseDict, solar_residential_array, ts_residential_array, last_key = ResidentialLoads.append_residential(glmCaseDict, use_flags, tech_data, residential_dict, last_key, CPP_flag_name, market_penetration_random, dlc_rand, pool_pump_recovery_random, slider_random, xval, elasticity_random, configuration_file)
	# End addition of residential loads ########################################################################################################################

	# TODO: Call Commercial Function
	if use_flags['use_commercial'] != 0:
		#print('calling CommercialLoads.py\n')
		if use_flags['use_normalized_loadshapes'] == 1:
			glmCaseDict, last_key = AddLoadShapes.add_normalized_commercial_ziploads(glmCaseDict, commercial_dict, config_data, last_key)
		else:
			glmCaseDict, solar_office_array, solar_bigbox_array, solar_stripmall_array, ts_office_array, ts_bigbox_array, ts_stripmall_array, last_key = CommercialLoads.append_commercial(glmCaseDict, use_flags, tech_data, last_key, commercial_dict, comm_slider_random, dlc_c_rand, dlc_c_rand2, configuration_file)
			
	# Append Solar: Call append_solar(feeder_dict, use_flags, config_file, solar_bigbox_array, solar_office_array, solar_stripmall_array, solar_residential_array, last_key)
	if use_flags['use_solar'] != 0 or use_flags['use_solar_res'] != 0 or use_flags['use_solar_com'] != 0:
		glmCaseDict = Solar_Technology.Append_Solar(glmCaseDict, use_flags, config_data, solar_bigbox_array, solar_office_array, solar_stripmall_array, solar_residential_array, last_key)
		
	# Append recorders
	#glmCaseDict, last_key = AddTapeObjects.add_recorders(glmCaseDict,case_flag,0,1,'four_node_basecase_test', last_key)


	return (glmCaseDict, last_key)

def _tests():
	#tests here
	glm_object_dict = feeder.parse('./IEEE13Basic.glm')
	baseGLM, last_key = GLD_Feeder(glm_object_dict,0,None)
	glm_string = feeder.sortedWrite(baseGLM)
	file = open('./IEEE13BasePopulation.glm','w')
	file.write(glm_string)
	file.close()
	print('success!')

if __name__ ==  '__main__':
	_tests()
