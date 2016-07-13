#Python Extraction and Calibration Version of MATLAB Scripts
# add_residential_loads.py appends residential houses and residential TOU/CPP technology object dictionaries to the desired feeder technology case dictionary
from __future__ import division
import math
import random
import Configuration

def append_residential(ResTechDict, use_flags, tech_data, residential_dict, last_object_key, CPP_flag_name, market_penetration_random, dlc_rand, pool_pump_recovery_random, slider_random, xval, elasticity_random, configuration_file=None):
	#ResTechDict is a dictionary containing all the objects in WindMIL model represented as equivalent GridLAB-D objects that this function will append residential load object to.
	solar_residential_array = [0,[None],[None]]
	ts_residential_array = [0,[None]]
	
	# Check if last_object_key exists in glmCaseDict
	if last_object_key in ResTechDict:
		while last_object_key in ResTechDict:
			last_object_key += 1
	
	# Begin adding residential house dictionaries
	if use_flags['use_homes'] == 1 and len(residential_dict) > 0:
		count_house = 0
		fl_area = []
		
		random.seed(3)	
		#print('iterating over residential_dict')
		# Begin attaching houses to designated triplex_meters		
		for x in residential_dict:
			if residential_dict[x]['number_of_houses'] > 0:
				if residential_dict[x]['parent'] != 'None':
					my_name = residential_dict[x]['parent'] + '_' + residential_dict[x]['name']
					my_parent = residential_dict[x]['parent']
				else:
					my_name = residential_dict[x]['name']
					my_parent = residential_dict[x]['name']
					
				no_houses = residential_dict[x]['number_of_houses']
				phase = residential_dict[x]['phases']
				lg_vs_sm = residential_dict[x]['large_vs_small']
				classID = residential_dict[x]['load_classification']
				
				config_data = Configuration.ConfigurationFunc(configuration_file,classID) # recall Configuration() with this load's classification
				# Find out how many houses in any sub-classifications
				thermal_integrity = list(map(lambda a:math.ceil(a*no_houses),config_data['thermal_percentages']))
				no_pool_pumps = math.fsum(config_data['SFH'])*no_houses # Total Single-Family Homes (only allow pool-pumps on SFH)
				
				# Find the number of houses in each thermal setpoint bin
				cool_sp = []
				heat_sp = []
				sfh = list(map(lambda a,b:a*b,config_data['SFH'],thermal_integrity)) # Number of single-family homes in each subclass
				
				#xrange(thermal_integrity.length)
				for y in xrange(len(thermal_integrity)):
					cool_sp.append([])
					heat_sp.append([])
					for z in xrange(len(config_data['cooling_setpoint'])):
						cool_sp[y].append(math.ceil(config_data['cooling_setpoint'][z][0] * thermal_integrity[y]))
						heat_sp[y].append(math.ceil(config_data['heating_setpoint'][z][0] * thermal_integrity[y]))
				#print('iterating over number of houses')
				# Start adding house dictionaries
				for y in xrange(no_houses):
					ResTechDict[last_object_key] = {'object' : 'triplex_meter',
													'phases' : '{:s}'.format(phase),
													'name' : 'tpm{:d}_{:s}'.format(y,my_name),
													'parent' : '{:s}'.format(my_parent),
													'groupid' : 'Residential_Meter',
													'meter_power_consumption' : '{:s}'.format(tech_data['res_meter_cons']),
													'nominal_voltage' : '120'}
													
					if use_flags['use_billing'] == 1:
						ResTechDict[last_object_key]['bill_mode'] = 'UNIFORM'
						ResTechDict[last_object_key]['price'] = '{:.5f}'.format(tech_data['flat_price'][config_data['region']])
						ResTechDict[last_object_key]['monthly_fee'] = '{:s}'.format(tech_data['monthly_fee'])
						ResTechDict[last_object_key]['bill_day'] = '1'
					elif use_flags['use_billing'] == 3:
						ResTechDict[last_object_key]['bill_mode'] = 'UNIFORM'
						ResTechDict[last_object_key]['power_market'] = '{:s}'.format(tech_data['market_info'][1])
						ResTechDict[last_object_key]['monthly_fee'] = '{:s}'.format(tech_data['monthly_fee'])
						ResTechDict[last_object_key]['bill_day'] = '1'
					last_object_key += 1
					#print('finished triplex_meter')
					# Create an array of parents for the residential thermal storage
					if use_flags['use_ts'] != 0:
						ts_residential_array[0] += 1
						ts_residential_array[1].append('house{:s}_{:s}'.format(y,my_name))
						
					# Create an array of parents for the residential solar
					if use_flags['use_solar'] != 0 or use_flags['use_solar_res'] != 0:
						solar_residential_array[0] += 1
						solar_residential_array[1].append('tpm{:d}_{:s}'.format(y,my_name))
						solar_residential_array[2].append('{:s}'.format(phase))
						
					# Create the house dictionary
					ResTechDict[last_object_key] = {'object' : 'house',
													'name' : 'house{:d}_{:s}'.format(y,my_name),
													'parent' : 'tpm{:d}_{:s}'.format(y,my_name),
													'groupid' : 'Residential'}
					
					# Calculate the  residential schedule skew value
					skew_value = config_data['residential_skew_std']*random.normalvariate(0,1)
					
					if skew_value < -1*tech_data['residential_skew_max']:
						skew_value = -1*tech_data['residential_skew_max']
					elif skew_value > tech_data['residential_skew_max']:
						skew_value = tech_data['residential_skew_max']
					
					# Additional skew outside the residential skew max
					skew_value = skew_value + config_data['residential_skew_shift']
					
					# Calculate the waterheater schedule skew
					wh_skew_value = 3*config_data['residential_skew_std']*random.normalvariate(0,1)
					
					if wh_skew_value < -6*tech_data['residential_skew_max']:
						wh_skew_value = -6*tech_data['residential_skew_max']
					elif wh_skew_value > 6*tech_data['residential_skew_max']:
						wh_skew_value = 6*tech_data['residential_skew_max']
					
					# Scale this skew up to weeks
					pp_skew_value = 128*config_data['residential_skew_std']*random.normalvariate(0,1)
					
					if pp_skew_value < -128*tech_data['residential_skew_max']:
						pp_skew_value = -128*tech_data['residential_skew_max']
					elif pp_skew_value > 128*tech_data['residential_skew_max']:
						pp_skew_value = 128*tech_data['residential_skew_max']
						
					ResTechDict[last_object_key]['schedule_skew'] = '{:.0f}'.format(skew_value)
					
					# Choose what type of building we are going to use
					# and set the thermal integrity of said building
					size_a = len(thermal_integrity)
					
					for z in xrange(len(thermal_integrity)):
						if type(thermal_integrity[z]) == type([]):# Checks to see if contents of thermal_integrity are lists
							size_b = len(thermal_integrity[z])
						else:
							size_b = 1
					
					therm_int = int(math.ceil(size_a * size_b * random.random()))
					
					row_ti = therm_int % size_a
					col_ti = therm_int % size_b
					
					while thermal_integrity[row_ti] < 1:
						therm_int = int(math.ceil(size_a * size_b * random.random()))
						row_ti = therm_int % size_a
						col_ti = therm_int % size_b
						
					thermal_integrity[row_ti] -= 1
					thermal_temp = config_data['thermal_properties'][row_ti]
					
					f_area = config_data['floor_area'][classID]
					story_rand = random.random()
					height_rand = random.randint(1,2)
					fa_rand = random.random()
					
					if sfh[row_ti] > 0:
						floor_area = f_area + (f_area / 2) * fa_rand * ((row_ti - 4) / 3)
						
						if story_rand < config_data['one_story'][classID]:
							stories = 1
						else:
							stories = 2
						
						sfh[row_ti] = sfh[row_ti] - 1
					else:
						floor_area = f_area + (f_area / 2) * (0.5 - fa_rand)
						stories = 1
						height_rand = 0
					
					# Now also adjust square footage as a factor of whether
					# the load modifier (avg_house) rounded up or down
					floor_area = (1 + lg_vs_sm) * floor_area
					
					if floor_area > 4000:
						floor_area = 3800 + fa_rand*200
					elif floor_area < 300:
						floor_area = 300 + fa_rand*100
					
					fl_area.append(floor_area)
					count_house += 1
					
					ResTechDict[last_object_key]['floor_area'] = '{:.0f}'.format(floor_area)
					ResTechDict[last_object_key]['number_of_stories'] = '{:.0f}'.format(stories)
					
					ceiling_height = 8 + height_rand
					ResTechDict[last_object_key]['ceiling_height'] = '{:.0f}'.format(ceiling_height)
					#ResTechDict[last_object_key]['comment'] = '//Load Classification -> {:s} {:.0f}'.format(config_data['load_classifications'][classID],row_ti + 1)
					ResTechDict[last_object_key]['comment'] = '//Load Classification -> {:s}'.format(config_data['load_classifications'][classID])
					
					rroof = thermal_temp[0] * (0.8 + (0.4 * random.random()))
					ResTechDict[last_object_key]['Rroof'] = '{:.2f}'.format(rroof)
					
					rwall =  thermal_temp[1] * (0.8 + (0.4 * random.random()))
					ResTechDict[last_object_key]['Rwall'] = '{:.2f}'.format(rwall)
					
					rfloor =  thermal_temp[2] * (0.8 + (0.4 * random.random()))
					ResTechDict[last_object_key]['Rfloor'] = '{:.2f}'.format(rfloor)
					ResTechDict[last_object_key]['glazing_layers'] = '{:.0f}'.format(thermal_temp[3])
					ResTechDict[last_object_key]['glass_type'] = '{:.0f}'.format(thermal_temp[4])
					ResTechDict[last_object_key]['glazing_treatment'] = '{:.0f}'.format(thermal_temp[5])
					ResTechDict[last_object_key]['window_frame'] = '{:.0f}'.format(thermal_temp[6])
					
					rdoor =  thermal_temp[7] * (0.8 + (0.4 * random.random()))
					ResTechDict[last_object_key]['Rdoors'] = '{:.2f}'.format(rdoor)
					
					airchange =  thermal_temp[8] * (0.8 + (0.4 * random.random()))
					ResTechDict[last_object_key]['airchange_per_hour'] = '{:.2f}'.format(airchange)
					
					c_COP =  thermal_temp[10] + (random.random() * (thermal_temp[9] - thermal_temp[10]))
					ResTechDict[last_object_key]['cooling_COP'] = '{:.2f}'.format(c_COP)
					
					init_temp =  68 + (4 * random.random())
					ResTechDict[last_object_key]['air_temperature'] = '{:.2f}'.format(init_temp)
					ResTechDict[last_object_key]['mass_temperature'] = '{:.2f}'.format(init_temp)
					
					ResTechDict[last_object_key]['window_wall_ratio'] = '{:.2f}'.format(config_data['window_wall_ratio'])
					
					# This is a bit of a guess from Rob's estimates
					mass_floor = 2.5 + (1.5 * random.random())
					ResTechDict[last_object_key]['total_thermal_mass_per_floor_area'] = '{:.3f}'.format(mass_floor)
					
					heat_type = random.random()
					cool_type = random.random()
					h_COP = c_COP
					
					ct = 'NONE'
					if heat_type <= config_data['perc_gas']:
						ResTechDict[last_object_key]['heating_system_type'] = 'GAS'
						
						if cool_type <= config_data['perc_AC']:
							ResTechDict[last_object_key]['cooling_system_type'] = 'ELECTRIC'
							ct = 'ELEC'
						else:
							ResTechDict[last_object_key]['cooling_system_type'] = 'NONE'
						
						ht = 'GAS'
					elif heat_type <= (config_data['perc_gas'] + config_data['perc_pump']):
						ResTechDict[last_object_key]['heating_system_type'] = 'HEAT_PUMP'
						ResTechDict[last_object_key]['heating_COP'] = '{:.1f}'.format(h_COP)
						ResTechDict[last_object_key]['cooling_system_type'] = 'ELECTRIC'
						ResTechDict[last_object_key]['auxiliary_strategy'] = 'DEADBAND'
						ResTechDict[last_object_key]['auxiliary_system_type'] = 'ELECTRIC'
						ResTechDict[last_object_key]['motor_model'] = 'BASIC'
						ResTechDict[last_object_key]['motor_efficiency'] = 'VERY_GOOD'
						ht = 'HP'
						ct = 'ELEC'
					elif (floor_area * ceiling_height) > 12000: # No resistive homes with large volumes
						ResTechDict[last_object_key]['heating_system_type'] = 'GAS'
						
						if cool_type <= config_data['perc_AC']:
							ResTechDict[last_object_key]['cooling_system_type'] = 'ELECTRIC'
							ct = 'ELEC'
						else:
							ResTechDict[last_object_key]['cooling_system_type'] = 'NONE'

						ht = 'GAS'
					else:
						ResTechDict[last_object_key]['heating_system_type'] = 'RESISTANCE'
						
						if cool_type <= config_data['perc_AC']:
							ResTechDict[last_object_key]['cooling_system_type'] = 'ELECTRIC'
							ResTechDict[last_object_key]['motor_model'] = 'BASIC'
							ResTechDict[last_object_key]['motor_efficiency'] = 'VERY_GOOD'
							ct = 'ELEC';
						else:
							ResTechDict[last_object_key]['cooling_system_type'] = 'NONE'
							
						ht = 'ELEC';
						
					AC_unit_type = random.random()
					if ct == 'ELEC': # I house has electric AC
						if AC_unit_type <= config_data['AC_type'][1]: # AC is central
							os_rand = config_data['over_sizing_factor'][0] * (0.8 + (0.4 * random.random()))
						else: # AC is window/wall unit
							os_rand = config_data['over_sizing_factor'][1] * (0.8 + (0.4 * random.random()))
						
					ResTechDict[last_object_key]['over_sizing_factor'] = '{:.1f}'.format(os_rand)
					ResTechDict[last_object_key]['breaker_amps'] = '1000'
					ResTechDict[last_object_key]['hvac_breaker_rating'] = '1000'
					
					# Choose a cooling and heating schedule
					cooling_set = int(math.ceil(config_data['no_cool_sch'] * random.random()))
					heating_set = int(math.ceil(config_data['no_heat_sch'] * random.random()))
					
					# Choose a cooling bin
					coolsp = config_data['cooling_setpoint']
					no_cool_bins = len(coolsp)
					
					# See if we have that bin left
					cool_bin = random.randint(0,no_cool_bins - 1)
						
					while cool_sp[row_ti][cool_bin] < 1:
						cool_bin = random.randint(0,no_cool_bins - 1)
						
					cool_sp[row_ti][cool_bin] -= 1
					
					# Choose a heating bin
					heatsp = config_data['heating_setpoint']
					no_heat_bins = len(heatsp)
					heat_bin = random.randint(0,no_heat_bins - 1)
					heat_count = 1
					
					while heat_sp[row_ti][heat_bin] < 1 or heatsp[heat_bin][2] >= coolsp[cool_bin][3]:
						heat_bin = random.randint(0,no_heat_bins - 1)
						
						# if we tried a few times, give up and take an extra
						# draw from the lowest bin
						if heat_count > 20:
							heat_bin = 0
							break;

						heat_count = heat_count + 1
						
					heat_sp[row_ti][heat_bin] -= 1
					
					# Randomly choose within the bin, then +/- one
					# degree to seperate the deadbands
					cool_night = (coolsp[cool_bin][2] - coolsp[cool_bin][3]) * random.random() + coolsp[cool_bin][3] + 1
					heat_night = (heatsp[heat_bin][2] - heatsp[heat_bin][3]) * random.random() + heatsp[heat_bin][3] - 1

					# 1-15-2013: made a change so that cool and heat
					# diff's are based off same random value -JLH 
					diff_rand = random.random()
					cool_night_diff = coolsp[cool_bin][1] * 2 * diff_rand;
					heat_night_diff = heatsp[heat_bin][1] * 2 * diff_rand;
					
					heat_night += config_data['addtl_heat_degrees']
					
					if use_flags['use_market'] == 0 or use_flags['use_market'] == 3:
						ResTechDict[last_object_key]['cooling_setpoint'] = 'cooling{:d}*{:.2f}+{:.2f}'.format(cooling_set,cool_night_diff,cool_night)
						ResTechDict[last_object_key]['heating_setpoint'] = 'heating{:d}*{:.2f}+{:.2f}'.format(heating_set,heat_night_diff,heat_night)
						
					if use_flags['use_market'] == 3:
						ResTechDict[last_object_key]['dlc_offset'] = '6'

					if (use_flags['use_market'] == 1 or use_flags['use_market'] == 2) and tech_data['use_tech'] == 1:
						if market_penetration_random[y*len(residential_dict) + x] <= tech_data['market_info'][7]:
							if ht == 'HP':
								ResTechDict[last_object_key]['cooling_setpoint'] = '{:.2f}'.format(cool_night)
								ResTechDict[last_object_key]['heating_setpoint'] = '{:.2f}'.format(cool_night - 3)
							elif ht == 'ELEC':
								ResTechDict[last_object_key]['cooling_setpoint'] = '{:.2f}'.format(cool_night)
								ResTechDict[last_object_key]['heating_setpoint'] = '{:.2f}'.format(cool_night - 3)
							elif ct == 'ELEC':
								ResTechDict[last_object_key]['cooling_setpoint'] = '{:.2f}'.format(cool_night)
								ResTechDict[last_object_key]['heating_setpoint'] = 'heating{:d}*{:.2f}+{:.2f}'.format(heating_set,heat_night_diff,heat_night)
							else:
								ResTechDict[last_object_key]['cooling_setpoint'] = 'cooling{:d}*{:.2f}+{:.2f}'.format(cooling_set,cool_night_diff,cool_night)
								ResTechDict[last_object_key]['heating_setpoint'] = 'heating{:d}*{:.2f}+{:.2f}'.format(heating_set,heat_night_diff,heat_night)

					if (use_flags['use_market'] == 1 or use_flags['use_market'] == 2) and tech_data['use_tech'] == 0:
						# TOU/CPP w/o technology - assumes people offset
						# their thermostats a little more
						new_rand = 1 + slider_random[y*len(residential_dict) + x]
						ResTechDict[last_object_key]['cooling_setpoint'] = 'cooling{:d}*{:.2f}+{:.2f}'.format(cooling_set,cool_night_diff*new_rand,cool_night)
						ResTechDict[last_object_key]['heating_setpoint'] = 'heating{:d}*{:.2f}+{:.2f}'.format(heating_set,heat_night_diff/new_rand,heat_night)

					last_object_key += 1
					#print('finished house')
					# Put in passive controller for DLC use_flags.use_market = 3 line 2224
					if use_flags['use_market'] == 3:
						ResTechDict[last_object_key] = {'object' : 'passive_controller',
														'control_mode' : 'DIRECT_LOAD_CONTROL',
														'parent' : 'house' + str(y) + '_' + my_name,
														'dlc_mode' : 'CYCLING',
														'period' : '0',
														'state_property' : 'override',
														'observation_object' : tech_data['market_info'][0],
														'observation_property' : 'past_market.clearing_price',
														'second_tier_price' : config_data['CPP_prices'][2]}
						
						c_on = 300 + (600 * dlc_rand[y*len(residential_dict) + x])
						temp_c = 0.3 + (0.4 * xval[y*len(residential_dict) + x])
						c_off = c_on * (temp_c / (1 - temp_c)) # 30-70% of the total time should be "off"
						
						ResTechDict[last_object_key]['cycle_length_on'] = str(math.floor(c_on))
						ResTechDict[last_object_key]['cycle_lenght_off'] = str(math.floor(c_off))
						last_object_key += 1
						
					# Put in Controller objects for use_flags.use_market = 1 or 2 line 2239
					if (use_flags['use_market'] == 1 or use_flags['use_market'] == 2) and tech_data['use_tech'] == 1:
						if market_penetration_random[y*len(residential_dict) + x] <= tech_data['market_info'][7]:
							# TOU or TOU/CPP with technology
	
							# pull in the slider response level
							slider = slider_random[y*len(residential_dict) + x]
	
							# set the pre-cool / pre-heat xrange to really small
							# to get rid of it.
							s_tstat = 2
							hrh = -5 + 5*(1-slider)
							crh = 5-5*(1-slider)
							hrl = -0.005+0*(1-slider)
							crl = -0.005+0*(1-slider)
	
							hrh2 = -s_tstat - (1 - slider) * (3 - s_tstat)
							crh2 = s_tstat + (1 - slider) * (3 - s_tstat)
							hrl2 = -s_tstat - (1 - slider) * (3 - s_tstat)
							crl2 = s_tstat + (1 - slider) * (3 - s_tstat)
							
							if ht == 'HP': # Control both therm setpoints
								ResTechDict[last_object_key] = {'object' : 'controller',
																'parent' : 'house{:s}_{:s}'.format(y,my_name),
																'schedule_skew' : '{:.0f}'.format(skew_value),
																'market' : '{:s}'.format(tech_data['market_info'][0]),
																'bid_mode' : 'OFF',
																'control_mode' : 'DOUBLE_RAMP',
																'resolve_mode' : 'DEADBAND',
																'heating_range_high' : '{:.3f}'.format(hrh),
																'cooling_range_high' : '{:.3f}'.format(crh),
																'heating_range_low' : '{:.3f}'.format(hrl),
																'cooling_range_low' : '{:.3f}'.format(crl),
																'heating_ramp_high' : '{:.3f}'.format(hrh2),
																'cooling_ramp_high' : '{:.3f}'.format(crh2),
																'heating_ramp_low' : '{:.3f}'.format(hrl2),
																'cooling_ramp_low' : '{:.3f}'.format(crl2),
																'cooling_base_setpoint' : 'cooling{:d}*{:.2f}+{:.2f}'.format(cooling_set,cool_night_diff,cool_night),
																'heating_base_setpoint' : 'heating{:d}*{:.2f}+{:.2f}'.format(heating_set,heat_night_diff,heat_night),
																'period' : '{:.0f}'.format(tech_data['market_info'][1]),
																'average_target' : 'my_avg',
																'standard_deviation_target' : 'my_std',
																'target' : 'air_temperature',
																'heating_setpoint' : 'heating_setpoint',
																'heating_demand' : 'last_heating_load',
																'cooling_setpoint' : 'cooling_setpoint',
																'cooling_demand' : 'last_cooling_load',
																'deadband' : 'thermostat_deadband',
																'total' : 'hvac_load',
																'load' : 'hvac_load',
																'state' : 'power_state'}
								last_object_key += 1
							elif ht == 'ELEC': # Control the heat, but check to see if we have AC
								if ct == 'ELEC': # get to control just like a heat pump
									ResTechDict[last_object_key] = {'object' : 'controller',
																	'parent' : 'house{:s}_{:s}'.format(y,my_name),
																	'schedule_skew' : '{:.0f}'.format(skew_value),
																	'market' : '{:s}'.format(tech_data['market_info'][0]),
																	'bid_mode' : 'OFF',
																	'control_mode' : 'DOUBLE_RAMP',
																	'resolve_mode' : 'DEADBAND',
																	'heating_range_high' : '{:.3f}'.format(hrh),
																	'cooling_range_high' : '{:.3f}'.format(crh),
																	'heating_range_low' : '{:.3f}'.format(hrl),
																	'cooling_range_low' : '{:.3f}'.format(crl),
																	'heating_ramp_high' : '{:.3f}'.format(hrh2),
																	'cooling_ramp_high' : '{:.3f}'.format(crh2),
																	'heating_ramp_low' : '{:.3f}'.format(hrl2),
																	'cooling_ramp_low' : '{:.3f}'.format(crl2),
																	'cooling_base_setpoint' : 'cooling{:d}*{:.2f}+{:.2f}'.format(cooling_set,cool_night_diff,cool_night),
																	'heating_base_setpoint' : 'heating{:d}*{:.2f}+{:.2f}'.format(heating_set,heat_night_diff,heat_night),
																	'period' : '{:.0f}'.format(tech_data['market_info'][1]),
																	'average_target' : 'my_avg',
																	'standard_deviation_target' : 'my_std',
																	'target' : 'air_temperature',
																	'heating_setpoint' : 'heating_setpoint',
																	'heating_demand' : 'last_heating_load',
																	'cooling_setpoint' : 'cooling_setpoint',
																	'cooling_demand' : 'last_cooling_load',
																	'deadband' : 'thermostat_deadband',
																	'total' : 'hvac_load',
																	'load' : 'hvac_load',
																	'state' : 'power_state'}
									last_object_key += 1
								else: # control only the heat
									ResTechDict[last_object_key] = {'object' : 'controller',
																	'parent' : 'house{:s}_{:s}'.format(y,my_name),
																	'schedule_skew' : '{:.0f}'.format(skew_value),
																	'market' : '{:s}'.format(tech_data['market_info'][0]),
																	'bid_mode' : 'OFF',
																	'control_mode' : 'RAMP',
																	'range_high' : '{:.3f}'.format(hrh),
																	'range_low' : '{:.3f}'.format(hrl),
																	'ramp_high' : '{:.3f}'.format(hrh2),
																	'ramp_low' : '{:.3f}'.format(hrl2),
																	'base_setpoint' : 'heating{:d}*{:.2f}+{:.2f}'.format(heating_set,heat_night_diff,heat_night),
																	'period' : '{:.0f}'.format(tech_data['market_info'][1]),
																	'average_target' : 'my_avg',
																	'standard_deviation_target' : 'my_std',
																	'target' : 'air_temperature',
																	'setpoint' : 'heating_setpoint',
																	'demand' : 'last_heating_load',
																	'deadband' : 'thermostat_deadband',
																	'total' : 'hvac_load',
																	'load' : 'hvac_load',
																	'state' : 'power_state'}
									last_object_key += 1
							elif ct == 'ELEC': # Gas heat, cut control the AC
								ResTechDict[last_object_key] = {'object' : 'controller',
																'parent' : 'house{:s}_{:s}'.format(y,my_name),
																'schedule_skew' : '{:.0f}'.format(skew_value),
																'market' : '{:s}'.format(tech_data['market_info'][0]),
																'bid_mode' : 'OFF',
																'control_mode' : 'RAMP',
																'range_high' : '{:.3f}'.format(crh),
																'range_low' : '{:.3f}'.format(crl),
																'ramp_high' : '{:.3f}'.format(crh2),
																'ramp_low' : '{:.3f}'.format(crl2),
																'base_setpoint' : 'cooling{:d}*{:.2f}+{:.2f}'.format(cooling_set,cool_night_diff,cool_night),
																'period' : '{:.0f}'.format(tech_data['market_info'][1]),
																'average_target' : 'my_avg',
																'standard_deviation_target' : 'my_std',
																'target' : 'air_temperature',
																'setpoint' : 'cooling_setpoint',
																'demand' : 'last_cooling_load',
																'deadband' : 'thermostat_deadband',
																'total' : 'hvac_load',
																'load' : 'hvac_load',
																'state' : 'power_state'}
								last_object_key += 1
					
					# Add the end-use ZIPload objects to the house
					# Scale all of the end-use loads
					scalar_base = config_data['base_load_scalar']
					scalar1 = ((324.9 / 8907) * pow(floor_area,0.442)) * scalar_base
					scalar2 = 0.8 + 0.4 * random.random()
					scalar3 = 0.8 + 0.4 * random.random()
					resp_scalar = scalar1 * scalar2
					unresp_scalar = scalar1 * scalar3

					# average size is 1.36 kW
					# Energy Savings through Automatic Seasonal Run-Time Adjustment of Pool Filter Pumps 
					# Stephen D Allen, B.S. Electrical Engineering
					pool_pump_power = 1.36 + .36*random.random()
					pool_pump_perc = random.random()

					# average 4-12 hours / day -> 1/6-1/2 duty cycle
					# typically run for 2 - 4 hours at a time
					pp_dutycycle = 1/6 + (1/2 - 1/6)*random.random()
					pp_period = 4 + 4*random.random()
					pp_init_phase = random.random()
					
					# Add responsive ZIPload
					
					ResTechDict[last_object_key] = {'object' : 'ZIPload',
													'name' : 'house{:d}_resp_{:s}'.format(y,my_name),
													'parent' : 'house{:d}_{:s}'.format(y,my_name),
													'comment' : '// Responsive load',
													'groupid' : 'Responsive_load',
													'schedule_skew' : '{:.0f}'.format(skew_value),
													'base_power' : 'responsive_loads*{:.2f}'.format(resp_scalar),
													'heatgain_fraction' : '{:.3f}'.format(tech_data['heat_fraction']),
													'power_pf' : '{:.3f}'.format(tech_data['p_pf']),
													'current_pf' : '{:.3f}'.format(tech_data['i_pf']),
													'impedance_pf' : '{:.3f}'.format(tech_data['z_pf']),
													'impedance_fraction' : '{:f}'.format(tech_data['zfrac']),
													'current_fraction' : '{:f}'.format(tech_data['ifrac']),
													'power_fraction' : '{:f}'.format(tech_data['pfrac'])}
					last_object_key += 1
					#print('finished responsive zipload')
					if use_flags['use_market'] == 1 or use_flags['use_market'] == 2:
						ResTechDict[last_object_key] = {'object' : 'passive_controller',
														'parent' : 'house{:d}_resp_{:s}'.format(y,my_name),
														'period' : '{:.0f}'.format(tech_data['market_info'][1]),
														'control_mode' : 'ELASTICITY_MODEL',
														'two_tier_cpp' : '{:s}'.format(tech_data['two_tier_cpp']),
														'observation_object' : '{:s}'.format(tech_data['market_info'][0]),
														'observation_property' : 'past_market.clearing_price',
														'state_property' : 'multiplier',
														'linearize_elasticity' : 'true',
														'price_offset' : '0.01'}
						
						if use_flags['use_market'] == 2: # CPP
							ResTechDict[last_object_key]['critical_day'] = '{:s}.value'.format(CPP_flag_name)
							ResTechDict[last_object_key]['first_tier_hours'] = '{:.0f}'.format(config_data['TOU_hours'][0])
							ResTechDict[last_object_key]['second_tier_hours'] = '{:.0f}'.format(config_data['TOU_hours'][1])
							ResTechDict[last_object_key]['third_tier_hours'] = '{:.0f}'.format(config_data['TOU_hours'][2])
							ResTechDict[last_object_key]['first_tier_price'] = '{:.6f}'.format(config_data['CPP_prices'][0])
							ResTechDict[last_object_key]['second_tier_price'] = '{:.6f}'.format(config_data['CPP_prices'][1])
							ResTechDict[last_object_key]['third_tier_price'] = '{:.6f}'.format(config_data['CPP_prices'][2])
							ResTechDict[last_object_key]['old_first_tier_price'] = '{:.6f}'.format(tech_data['flat_price'][config_data['region']])
							ResTechDict[last_object_key]['old_second_tier_price'] = '{:.6f}'.format(tech_data['flat_price'][config_data['region']])
							ResTechDict[last_object_key]['old_third_tier_price'] = '{:.6f}'.format(tech_data['flat_price'][config_data['region']])
						else:
							ResTechDict[last_object_key]['critical_day'] = '0'
							ResTechDict[last_object_key]['first_tier_hours'] = '{:.0f}'.format(config_data['TOU_hours'][0])
							ResTechDict[last_object_key]['second_tier_hours'] = '{:.0f}'.format(config_data['TOU_hours'][1])
							ResTechDict[last_object_key]['first_tier_price'] = '{:.6f}'.format(config_data['TOU_prices'][0])
							ResTechDict[last_object_key]['second_tier_price'] = '{:.6f}'.format(config_data['TOU_prices'][1])
							ResTechDict[last_object_key]['old_first_tier_price'] = '{:.6f}'.format(tech_data['flat_price'][config_data['region']])
							ResTechDict[last_object_key]['old_second_tier_price'] = '{:.6f}'.format(tech_data['flat_price'][config_data['region']])
						
						ResTechDict[last_object_key]['daily_elasticity'] = '{:s}*{:.4f}'.format(tech_data['daily_elasticity'],elasticity_random[y*len(residential_dict) + x])
						ResTechDict[last_object_key]['sub_elasticity_first_second'] = '{:.4f}'.format(tech_data['sub_elas_12']*elasticity_random[y*len(residential_dict) + x])
						ResTechDict[last_object_key]['sub_elasticity_first_third'] = '{:.4f}'.format(tech_data['sub_elas_13']*elasticity_random[y*len(residential_dict) + x])
						last_object_key += 1
					
					# Add unresponsive ZIPload object
					ResTechDict[last_object_key] = {'object' : 'ZIPload',
													'name' : 'house{:d}_unresp_{:s}'.format(y,my_name),
													'parent' : 'house{:d}_{:s}'.format(y,my_name),
													'comment' : '// Unresponsive load',
													'groupid' : 'Unresponsive_load',
													'schedule_skew' : '{:.0f}'.format(skew_value),
													'base_power' : 'unresponsive_loads*{:.2f}'.format(unresp_scalar),
													'heatgain_fraction' : '{:.3f}'.format(tech_data['heat_fraction']),
													'power_pf' : '{:.3f}'.format(tech_data['p_pf']),
													'current_pf' : '{:.3f}'.format(tech_data['i_pf']),
													'impedance_pf' : '{:.3f}'.format(tech_data['z_pf']),
													'impedance_fraction' : '{:f}'.format(tech_data['zfrac']),
													'current_fraction' : '{:f}'.format(tech_data['ifrac']),
													'power_fraction' : '{:f}'.format(tech_data['pfrac'])}
					last_object_key += 1
					#print('finished unresponsive zipload')
					# Add pool pumps only on single-family homes
					if pool_pump_perc < (2*config_data['perc_poolpumps']) and no_pool_pumps >= 1 and row_ti == 0:
						ResTechDict[last_object_key] = {'object' : 'ZIPload',
														'name' : 'house{:d}_ppump_{:s}'.format(y,my_name),
														'parent' : 'house{:d}_{:s}'.format(y,my_name),
														'comment' : '// Pool Pump',
														'groupid' : 'Pool_Pump',
														'schedule_skew' : '{:.0f}'.format(pp_skew_value),
														'base_power' : 'pool_pump_season*{:.2f}'.format(pool_pump_power),
														'duty_cycle' : '{:.2f}'.format(pp_dutycycle),
														'phase' : '{:.2f}'.format(pp_init_phase),
														'period' : '{:.2f}'.format(pp_period),
														'heatgain_fraction' : '0.0',
														'power_pf' : '{:.3f}'.format(tech_data['p_pf']),
														'current_pf' : '{:.3f}'.format(tech_data['i_pf']),
														'impedance_pf' : '{:.3f}'.format(tech_data['z_pf']),
														'impedance_fraction' : '{:f}'.format(tech_data['zfrac']),
														'current_fraction' : '{:f}'.format(tech_data['ifrac']),
														'power_fraction' : '{:f}'.format(tech_data['pfrac']),
														'is_240' : 'TRUE'}
						
						if (use_flags['use_market'] == 1 or use_flags['use_market'] == 2) and tech_data['use_tech'] == 1: # TOU
							ResTechDict[last_object_key]['recovery_duty_cycle'] = '{:.2f}'.format(pp_dutycycle * (1 + pool_pump_recovery_random[y*len(residential_dict) + x]))
						last_object_key += 1
						
						# Add passive_controllers to the pool pump ZIPloads
						if (use_flags['use_market'] == 1 or use_flags['use_market'] == 2) and tech_data['use_tech'] == 1: # TOU
							ResTechDict[last_object_key] = {'object' : 'passive_controller',
															'parent' : 'house{:d}_ppump_{:s}'.format(y,my_name),
															'period' : '{:.0f}'.format(tech_data['market_info'][1]),
															'control_mode' : 'DUTYCYCLE',
															'pool_pump_model' : 'TRUE',
															'observation_object' : '{:s}'.format(tech_data['market_info'][0]),
															'observation_property' : 'past_market.clearing_price',
															'state_property' : 'override',
															'base_duty_cycle' : '{:.2f}'.format(pp_dutycycle),
															'setpoint' : 'duty_cycle'}
															
							if use_flags['use_market'] == 2: # CPP
								ResTechDict[last_object_key]['first_tier_hours'] = '{:.0f}'.format(config_data['TOU_hours'][0])
								ResTechDict[last_object_key]['second_tier_hours'] = '{:.0f}'.format(config_data['TOU_hours'][1])
								ResTechDict[last_object_key]['third_tier_hours'] = '{:.0f}'.format(config_data['TOU_hours'][2])
								ResTechDict[last_object_key]['first_tier_price'] = '{:.6f}'.format(config_data['CPP_prices'][0])
								ResTechDict[last_object_key]['second_tier_price'] = '{:.6f}'.format(config_data['CPP_prices'][1])
								ResTechDict[last_object_key]['third_tier_price'] = '{:.6f}'.format(config_data['CPP_prices'][2])
							else:
								ResTechDict[last_object_key]['first_tier_hours'] = '{:.0f}'.format(config_data['TOU_hours'][0])
								ResTechDict[last_object_key]['second_tier_hours'] = '{:.0f}'.format(config_data['TOU_hours'][1])
								ResTechDict[last_object_key]['first_tier_price'] = '{:.6f}'.format(config_data['TOU_prices'][0])
								ResTechDict[last_object_key]['second_tier_price'] = '{:.6f}'.format(config_data['TOU_prices'][1])
							last_object_key += 1
							
							ResTechDict[last_object_key] = {'object' : 'passive_controller',
															'parent' : 'house{:d}_ppump_{:s}'.format(y,my_name),
															'period' : '0',
															'control_mode' : 'DIRECT_LOAD_CONTROL',
															'dlc_mode' : 'OFF',
															'observation_object' : '{:s}'.format(tech_data['market_info'][0]),
															'observation_property' : 'past_market.clearing_price',
															'state_property' : 'override',
															'second_tier_price' : '{:f}'.format(config_data['CPP_prices'][2])}
							last_object_key += 1
						
						no_pool_pumps -= 1
					#print('finished pool pump zipload')	
					# Add Water heater objects
					heat_element = 3.0 + (0.5 * random.randint(1,5))
					tank_set = 120 + (16 * random.random())
					therm_dead = 4 + (4 * random.random())
					tank_UA = 2 + (2 * random.random())
					water_sch = math.ceil(config_data['no_water_sch'] * random.random())
					water_var = 0.95 + (random.random() * 0.1)
					wh_size_test = random.random()
					wh_size_rand = random.randint(1,3)
					
					if heat_type > (1 - config_data['wh_electric']) and tech_data['use_wh'] == 1:
						last_object_key += 1
						ResTechDict[last_object_key] = {'object' : 'waterheater',
														'name' : 'house{:d}_wh_{:s}'.format(y,my_name),
														'parent' : 'house{:d}_{:s}'.format(y,my_name),
														'schedule_skew' : '{:.0f}'.format(wh_skew_value),
														'heating_element_capacity' : '{:.1f} kW'.format(heat_element),
														'tank_setpoint' : '{:.1f}'.format(tank_set),
														'temperature' : '132',
														'thermostat_deadband' : '{:.1f}'.format(therm_dead),
														'location' : 'INSIDE',
														'tank_UA' : '{:.1f}'.format(tank_UA)}
														
						if wh_size_test < config_data['wh_size'][0]:
							ResTechDict[last_object_key]['demand'] = 'small_{:.0f}*{:.02f}'.format(water_sch,water_var)
							
							whsize = 20 + (5 * (wh_size_rand - 1))
							ResTechDict[last_object_key]['tank_volume'] = '{:.0f}'.format(whsize)
						elif wh_size_test < (config_data['wh_size'][0] + config_data['wh_size'][1]):
							if floor_area < 2000:
								ResTechDict[last_object_key]['demand'] = 'small_{:.0f}*{:.02f}'.format(water_sch,water_var)
							else:
								ResTechDict[last_object_key]['demand'] = 'large_{:.0f}*{:.02f}'.format(water_sch,water_var)
							
							whsize = 30 + (10 * (wh_size_rand - 1))
							ResTechDict[last_object_key]['tank_volume'] = '{:.0f}'.format(whsize)
						elif floor_area > 2000:
							ResTechDict[last_object_key]['demand'] = 'large_{:.0f}*{:.02f}'.format(water_sch,water_var)
							
							whsize = 50 + (10 * (wh_size_rand - 1))
							ResTechDict[last_object_key]['tank_volume'] = '{:.0f}'.format(whsize)
						else:
							ResTechDict[last_object_key]['demand'] = 'large_{:.0f}*{:.02f}'.format(water_sch,water_var)
							
							whsize = 30 + (10 * (wh_size_rand - 1))
							ResTechDict[last_object_key]['tank_volume'] = '{:.0f}'.format(whsize)
						last_object_key += 1
							
						# Add passive controllers to the waterheaters
						if use_flags['use_market'] == 1 or use_flags['use_market'] == 2:
							ResTechDict[last_object_key] = {'object' : 'passive_controller',
															'parent' : 'house{:d}_wh_{:s}'.format(y,my_name),
															'period' : '{:.0f}'.format(tech_data['market_info'][1]),
															'control_mode' : 'PROBABILITY_OFF',
															'distribution_type' : 'NORMAL',
															'observation_object' : '{:s}'.format(tech_data['market_info'][0]),
															'observation_property' : 'past_market.clearing_price',
															'stdev_observation_property' : 'my_std',
															'expectation_object' : '{:s}'.format(tech_data['market_info'][0]),
															'expectation_property' : 'my_avg',
															'comfort_level' : '{:.2f}'.format(slider_random[y*len(residential_dict) + x]),
															'state_property' : 'override'}
							last_object_key += 1
						elif use_flags['use_market'] == 3:
							c_on = 180 + (120 * dlc_rand[y*len(residential_dict) + x])
							c_off = 1080 + (720 * xval[y*len(residential_dict) + x])
							ResTechDict[last_object_key] = {'object' : 'passive_controller',
															'parent' : 'house{:d}_wh_{:s}'.format(y,my_name),
															'period' : '0',
															'control_mode' : 'DIRECT_LOAD_CONTROL',
															'dlc_mode' : 'CYCLING',
															'observation_object' : '{:s}'.format(tech_data['market_info'][0]),
															'observation_property' : 'past_market.clearing_price',
															'cycle_length_on' : '{:.0f}'.format(c_on),
															'cycle_length_off' : '{:.0f}'.format(c_off),
															'comfort_level' : '9999',
															'second_tier_price' : '{:f}'.format(config_data['CPP_prices'][2]),
															'state_property' : 'override'}
							last_object_key += 1
				#print('finished water heater')
			#print('finished iterating over number of houses')
		#print('finished iterating over residential dict')
	return (ResTechDict, solar_residential_array, ts_residential_array, last_object_key)