'''
Created on Apr 9, 2013

@author: D3P988
'''

import math

# @todo export this to TechnologyParameters.py
def TechnologyParametersFunc(use_flags, TechToTest):
	#rv = {"data" : {data}, "use_flags" : {use_flags}}
	data = {'tech_flag' : TechToTest,
			'use_tech' : 0,
			'measure_losses' : 0,
			'dump_bills' : 0,
			'measure_capacitor' : 0,
			'measure_regulators' : 0,
			'collect_setpoints' : 0,
			'measure_EOL_voltage' : 0,
			'measure_loads' : 0,
			'include_stats' : 0,
			'meter_consumption' : 0,
			'dump_voltage' : 0,
			'measure_market' : 0,
			'get_IEEE_stats' : 0,
			'ts_SOC' : 0,
			'k_ts' : 0}

	# start nasty long copy/paste block (about 670 lines)
	data["tech_flag"] = TechToTest;
	data["use_tech"] = 0;

	# initialize use_flags
	use_flags = {'use_normalized_loadshapes' : 0,
				 'use_homes' : 0,
				 'use_commercial' : 0,
				 'use_billing' : 0,
				 'use_capacitor_outtages' : 0,
				 'use_vvc' : 0,
				 'use_emissions' : 0,
				 'use_market' : 0,
				 'use_ts' : 0,
				 'use_solar' : 0,
				 'use_solar_res' : 0,
				 'use_solar_com' : 0,
				 'use_battery' : 0,
				 'use_phev' : 0,
				 'use_da' : 0,
				 'use_wind' : 0}
	
	# quick case. Use load shapes instead of house objects.
	if TechToTest == -1:
		use_flags["use_normalized_loadshapes"] = 1
		use_flags["use_homes"] = 1
		use_flags["use_commercial"] = 1
		# These will include recorders/collectors/dumps
		data["measure_losses"] = 1;    
		data["collect_setpoints"] = 1;    
		data["measure_loads"] = 1;

		#Prints stats at bottom of GLM
		data["include_stats"] = 1

		# Adds in meter consumption
		data["meter_consumption"] = 1

		#Set to '1' only for testing
		data["dump_voltage"] = 0; 
		data["measure_market"] = 0
		data["get_IEEE_stats"] = 0

	# base case
	if TechToTest == 0:
		# These will all be '1' for base case
		# homes and commercial are need to include thse objects
		use_flags["use_homes"] = 1
		use_flags["use_commercial"] = 1

		# These will include recorders/collectors/dumps
		data["measure_losses"] = 1;    
		data["collect_setpoints"] = 1;    
		data["measure_loads"] = 1;

		#Prints stats at bottom of GLM
		data["include_stats"] = 1

		# Adds in meter consumption
		data["meter_consumption"] = 1

		#Set to '1' only for testing
		data["dump_voltage"] = 0; 
		data["measure_market"] = 0
		data["get_IEEE_stats"] = 0

	# CVR
	elif data["tech_flag"] == 1:
		#These will all be '1' for base case
		# homes and commercial are need to include thse objects
		use_flags["use_homes"] = 1
		use_flags["use_commercial"] = 1

		# These will include recorders/collectors/dumps
		use_flags["use_emissions"] = 1;
		use_flags["use_capacitor_outtages"] = 1;
		use_flags["use_vvc"] = 1;
		data["measure_losses"] = 1; 
		data["measure_capacitors"] = 1;
		data["measure_regulators"] = 1;   
		data["collect_setpoints"] = 1;    
		data["measure_EOL_voltage"] = 1;
		data["measure_loads"] = 1;

		#Prints stats at bottom of GLM
		data["include_stats"] = 1;

		# Adds in meter consumption
		data["meter_consumption"] = 1;

		#Set to '1' only for testing
		data["dump_voltage"] = 0;   
		data["measure_market"] = 0;
		data["get_IEEE_stats"] = 0;

	# Automation
	elif data["tech_flag"] == 2:
		#These will all be '1' for base case
		# homes and commercial are need to include thse objects
		use_flags["use_homes"] = 1;
		use_flags["use_commercial"] = 1;

		# These will include recorders/collectors/dumps
		use_flags["use_emissions"] = 1;
		use_flags["use_capacitor_outtages"] = 1;
		data["measure_losses"] = 1; 
		data["measure_capacitors"] = 1;
		data["measure_regulators"] = 1;   
		data["collect_setpoints"] = 1;    
		data["measure_EOL_voltage"] = 1;
		data["measure_loads"] = 1;

		#Prints stats at bottom of GLM
		data["include_stats"] = 1;

		# Adds in meter consumption
		data["meter_consumption"] = 1;

		#Set to '1' only for testing
		data["dump_voltage"] = 0;   
		data["measure_market"] = 0;
		data["get_IEEE_stats"] = 0;

	# FDIR
	elif data["tech_flag"] == 3:
		print("FDIR not implemented yet")
		pass

	# TOU/CPP w/ tech
	elif data["tech_flag"] == 4:
		use_flags["use_homes"] = 1;
		use_flags["use_commercial"] = 1;
		use_flags["use_billing"] = 3;
		use_flags["use_emissions"] = 1;
		use_flags["use_capacitor_outtages"] = 1;
		data["measure_losses"] = 1; 
		data["dump_bills"] = 1;
		data["measure_capacitors"] = 1;
		data["measure_regulators"] = 1;   
		data["collect_setpoints"] = 1;    
		data["measure_loads"] = 1;

		# adds in the market
		use_flags["use_market"] = 2;

		# adds in customer/technology interactions
		data["use_tech"] = 1;

		data["include_stats"] = 1;
		data["meter_consumption"] = 2;
		data["dump_voltage"] = 0;   
		data["measure_market"] = 1;
		data["get_IEEE_stats"] = 0;

	# TOU/CPP w/o tech
	elif data["tech_flag"] == 5:
		use_flags["use_homes"] = 1;
		use_flags["use_commercial"] = 1;
		use_flags["use_billing"] = 3;
		use_flags["use_emissions"] = 1;
		use_flags["use_capacitor_outtages"] = 1;
		data["measure_losses"] = 1; 
		data["dump_bills"] = 1;
		data["measure_capacitors"] = 1;
		data["measure_regulators"] = 1;   
		data["collect_setpoints"] = 1;    
		data["measure_loads"] = 1;
		data["include_stats"] = 1;
		use_flags["use_market"] = 2;
		data["meter_consumption"] = 2;
		data["dump_voltage"] = 0;   
		data["measure_market"] = 1;
		data["get_IEEE_stats"] = 0;

	# TOU w/ tech
	elif data["tech_flag"] == 6:
		use_flags["use_homes"] = 1;
		use_flags["use_commercial"] = 1;
		use_flags["use_billing"] = 3;
		use_flags["use_emissions"] = 1;
		use_flags["use_capacitor_outtages"] = 1;
		data["measure_losses"] = 1; 
		data["dump_bills"] = 1;
		data["measure_capacitors"] = 1;
		data["measure_regulators"] = 1;   
		data["collect_setpoints"] = 1;    
		data["measure_loads"] = 1;
		data["include_stats"] = 1;
		use_flags["use_market"] = 1;
		data["use_tech"] = 1;
		data["meter_consumption"] = 2;
		data["dump_voltage"] = 0;   
		data["measure_market"] = 1;
		data["get_IEEE_stats"] = 0;

	# TOU w/o tech
	elif data["tech_flag"] == 7:
		use_flags["use_homes"] = 1;
		use_flags["use_commercial"] = 1;
		use_flags["use_billing"] = 3;
		use_flags["use_emissions"] = 1;
		use_flags["use_capacitor_outtages"] = 1;
		data["measure_losses"] = 1; 
		data["dump_bills"] = 1;
		data["measure_capacitors"] = 1;
		data["measure_regulators"] = 1;   
		data["collect_setpoints"] = 1;    
		data["measure_loads"] = 1;
		data["include_stats"] = 1;
		use_flags["use_market"] = 1;
		data["meter_consumption"] = 2;
		data["dump_voltage"] = 0;   
		data["measure_market"] = 1;
		data["get_IEEE_stats"] = 0;

	# DLC
	elif data["tech_flag"] == 8:
		use_flags["use_homes"] = 1;
		use_flags["use_commercial"] = 1;
		use_flags["use_billing"] = 1;
		use_flags["use_emissions"] = 1;
		use_flags["use_capacitor_outtages"] = 1;
		data["measure_losses"] = 1; 
		data["dump_bills"] = 1;
		data["measure_capacitors"] = 1;
		data["measure_regulators"] = 1;   
		data["collect_setpoints"] = 1;    
		data["measure_loads"] = 1;
		data["include_stats"] = 1;
		use_flags["use_market"] = 1;
		data["meter_consumption"] = 2;
		data["dump_voltage"] = 0;   
		data["measure_market"] = 1;
		data["get_IEEE_stats"] = 0;

	# Thermal
	elif data["tech_flag"] == 9:
		#These will all be '1' for base case
		# homes and commercial are need to include thse objects
		use_flags["use_homes"] = 1;
		use_flags["use_commercial"] = 1;

		# These will include recorders/collectors/dumps
		use_flags["use_billing"] = 1;
		use_flags["use_emissions"] = 1;
		use_flags["use_capacitor_outtages"] = 1;
		data["measure_losses"] = 1; 
		data["dump_bills"] = 1;
		data["measure_capacitors"] = 1;
		data["measure_regulators"] = 1;   
		data["collect_setpoints"] = 1;    
		data["measure_EOL_voltage"] = 1;
		data["measure_loads"] = 1;

		#Prints stats at bottom of GLM
		data["include_stats"] = 1;

		# Adds in meter consumption
		data["meter_consumption"] = 1;

		#Set to '1' only for testing
		data["dump_voltage"] = 0;   
		data["measure_market"] = 0;
		data["get_IEEE_stats"] = 0;

		#Turn on energy storage
		#1 = add thermal storage using the defaults, 2 = add thermal storage with a randomized schedule, 0 = none
		#3 = add thermal storage to all houses using defaults, 4 = add thermal storage to all houses with a randomized schedule
		use_flags["use_ts"] = 2;

		#Set initial state of charge
		data["ts_SOC"] = 100;

		#Set thermal losses - no losses
		data["k_ts"] = 0;

	# PHEV
	elif data["tech_flag"] == 10:
		print("PHEV not implemented yet")
		pass

	# Solar Residential
	elif data["tech_flag"] == 11:
		#These will all be '1' for base case
		# homes and commercial are need to include thse objects
		use_flags["use_homes"] = 1;
		use_flags["use_commercial"] = 1;
		# These will include recorders/collectors/dumps
		use_flags["use_billing"] = 1;
		use_flags["use_emissions"] = 1;
		use_flags["use_capacitor_outtages"] = 1;
		data["measure_losses"] = 1; 
		data["dump_bills"] = 1;
		data["measure_regulators"] = 1;   
		data["collect_setpoints"] = 1;    
		data["measure_EOL_voltage"] = 1;
		data["measure_loads"] = 1;

		#Prints stats at bottom of GLM
		data["include_stats"] = 1;

		# Adds in meter consumption
		data["meter_consumption"] = 1;

		#Set to '1' only for testing
		data["dump_voltage"] = 0;   
		data["measure_market"] = 0;
		data["get_IEEE_stats"] = 0;

		#Turn on solar residential
		use_flags["use_solar_res"] = 1;

	# Solar Commercial
	elif data["tech_flag"] == 12:
		# These will all be '1' for base case
		# homes and commercial are need to include thse objects
		use_flags["use_homes"] = 1;
		use_flags["use_commercial"] = 1;
		# These will include recorders/collectors/dumps
		use_flags["use_billing"] = 1;
		use_flags["use_emissions"] = 1;
		use_flags["use_capacitor_outtages"] = 1;
		data["measure_losses"] = 1; 
		data["dump_bills"] = 1;
		data["measure_regulators"] = 1;   
		data["collect_setpoints"] = 1;    
		data["measure_EOL_voltage"] = 1;
		data["measure_loads"] = 1;

		#Prints stats at bottom of GLM
		data["include_stats"] = 1;

		# Adds in meter consumption
		data["meter_consumption"] = 1;

		#Set to '1' only for testing
		data["dump_voltage"] = 0;   
		data["measure_market"] = 0;
		data["get_IEEE_stats"] = 0;

		#Turn on solar commercial
		use_flags["use_solar_com"] = 1;

	# Combined solar 
	elif data["tech_flag"] == 13:
		#These will all be '1' for base case
		# homes and commercial are need to include thse objects
		use_flags["use_homes"] = 1;
		use_flags["use_commercial"] = 1;
		# These will include recorders/collectors/dumps
		use_flags["use_billing"] = 1;
		use_flags["use_emissions"] = 1;
		use_flags["use_capacitor_outtages"] = 1;
		data["measure_losses"] = 1; 
		data["dump_bills"] = 1;
		data["measure_regulators"] = 1;   
		data["collect_setpoints"] = 1;    
		data["measure_EOL_voltage"] = 1;
		data["measure_loads"] = 1;

		#Prints stats at bottom of GLM
		data["include_stats"] = 1;

		# Adds in meter consumption
		data["meter_consumption"] = 1;

		#Set to '1' only for testing
		data["dump_voltage"] = 0;   
		data["measure_market"] = 0;
		data["get_IEEE_stats"] = 0;

		#Turn on solar combined (both comm & residential)

		use_flags["use_solar"] = 1; 

	# Wind Commercial
	elif data["tech_flag"] == 14:
		#These will all be '1' for base case
		# commercial are need to include thse objects
		use_flags["use_homes"] = 1;
		use_flags["use_commercial"] = 1;
		# These will include recorders/collectors/dumps
		use_flags["use_billing"] = 1;
		use_flags["use_emissions"] = 1;
		use_flags["use_capacitor_outtages"] = 1;
		data["measure_losses"] = 1; 
		data["dump_bills"] = 1;
		data["measure_regulators"] = 1;   
		data["collect_setpoints"] = 1;    
		data["measure_loads"] = 1;

		#Prints stats at bottom of GLM
		data["include_stats"] = 1;

		# Adds in meter consumption
		data["meter_consumption"] = 1;

		#Set to '1' only for testing
		data["dump_voltage"] = 0;   
		data["measure_market"] = 0;
		data["get_IEEE_stats"] = 0;

		#Turn on commercial wind
		use_flags["use_wind"] = 1;  



	## Use flags structure
	# 1. Use homes
	# 2. Use battery storage ~ has {0, 1, 2} by the looks of it
	# 3. Use markets
	# 4. Use commercial buildings
	# 5. Use vvc
	# 6. Use customer billing
	# 7. Use solar
	# 8. Use PHEV
	# 9. Use distribution automation
	#10. Use wind
	#11. Other parameters

	## Home parameters
	if use_flags["use_homes"] == 1:
		# ZIP fractions and their power factors - Residential
		#data["z_pf"] = 1;
		#data["i_pf"] = 1;
		#data["p_pf"] = 1;
		data["z_pf"] = 0.97;
		data["i_pf"] = 0.97;
		data["p_pf"] = 0.97;
		data["zfrac"] = 0.2;
		data["ifrac"] = 0.4;
		data["pfrac"] = 1 - data["zfrac"] - data["ifrac"];

		data["heat_fraction"] = 0.9;

		# waterheaters 1 = yes, 0 = no
		data["use_wh"] = 1; 
		# Meter data is from Itron type meters (Centrution vs. OpenWay)
		if (data["meter_consumption"] == 1):
			data["res_meter_cons"] = '1+7j'; # Electromechanical (VAr)
		elif (data["meter_consumption"] == 2):
			data["res_meter_cons"] = '2+11j';# AMI (VAr)
		else:
			data["res_meter_cons"] = 0;

		# There is no ZIP fraction assosciated with this variable
		data["light_scalar_res"] = 1;

	## Battery Parameters
	if (use_flags["use_battery"] == 2 or use_flags["use_battery"] == 1):
		data["battery_energy"] = 1000000; # 10 MWh
		data["battery_power"] = 250000; # 1.5 MW

		data["efficiency"] = 0.86;   #percent
		data["parasitic_draw"] = 10; #Watts

	## Market Parameters -- @todo
	# 1 - TOU
	# 2 - TOU/CPP
	# 3 - DLC
	if (use_flags["use_market"] != 0):
		# market name, 
		# period, 
		# mean, 
		# stdev, 
		# max slider setting (xrange: 0.001 - 1; NOTE: do not use zero;
		# name of the price player/schedule)
		# percent penetration,
		if (use_flags["use_market"] == 1):
			data["two_tier_cpp"] = 'false';     
		elif (use_flags["use_market"] == 2):
			data["two_tier_cpp"] = 'true';
		elif (use_flags["use_market"] == 3):
			pass
		if (data["use_tech"] == 1):
			data["daily_elasticity"] = 'daily_elasticity_wtech';
			data["sub_elas_12"] = -0.152; # TOU substitution elasticity (average)
			data["sub_elas_13"] = -0.222; # CPP substitution elasticity (average)
		else:
			data["daily_elasticity"] = 'daily_elasticity_wotech'; #weekend vs. weekday schedules for daily elasticity
			data["sub_elas_12"] = -0.076; # TOU substitution elasticity (average)
			data["sub_elas_13"] = -0.111; # CPP substitution elasticity (average)

		# A lot of these values aren't used, except in an RTP market
		data["market_info"] = ['Market_1', 900, 'avg24', 'std24', 1.0, 'price_player', 1.0]

	## Commercial building parameters
	if (use_flags["use_commercial"] == 0):
		# zip loads
		#data["c_z_pf"] = 1;
		data["c_z_pf"] = 0.97;
		#data["c_i_pf"] = 1;
		data["c_i_pf"] = 0.97;
		#data["c_p_pf"] = 1;
		data["c_p_pf"] = 0.97;
		data["c_zfrac"] = 0.2;
		data["c_ifrac"] = 0.4;
		data["c_pfrac"] = 1 - data["c_zfrac"] - data["c_ifrac"];
	elif (use_flags["use_commercial"] == 1):
		# buildings
		data["cooling_COP"] = 3;
		#data["c_z_pf"] = 1;
		data["c_z_pf"] = 0.97;
		#data["c_i_pf"] = 1;
		data["c_i_pf"] = 0.97;
		#data["c_p_pf"] = 1;
		data["c_p_pf"] = 0.97;
		data["c_zfrac"] = 0.2;
		data["c_ifrac"] = 0.4;
		data["c_pfrac"] = 1 - data["c_zfrac"] - data["c_ifrac"];

		# Meter data is from Itron type meters (Centrution vs. OpenWay)
		if (data["meter_consumption"] == 1):
			data["comm_meter_cons"] = '1+15j'; # Electromechanical (VAr)
		elif (data["meter_consumption"] == 2):
			data["comm_meter_cons"] = '4+21j';# AMI (VAr)
		else:
			data["comm_meter_cons"] = 0;

		# VA cutoff - loads below this value per phase are made into "light" loads
		data["load_cutoff"] = 5000; 

		# Uses commercial building ZIP fractions
		data["light_scalar_comm"] = 1;

	## VVC parameters
	if (use_flags["use_vvc"] == 1):
		#data["output_volt"] = 2401;  # voltage to regulate to - 2401::120
		print("ERROR: CVR is not implemented yet")

	## Customer billing parameters
	if (use_flags["use_billing"] == 1): #FLAT RATE
		data["monthly_fee"] = 10; # $
		data["comm_monthly_fee"] = 25;
		# Average rate by region from, merged using Viraj spreadsheet
		# EIA: http://www.eia.doe.gov/electricity/epm/table5_6_b.html
		#  Region 6 is HECO 2010 rates for Oahu
		data["flat_price"] = [0.1243, 0.1294, 0.1012, 0.1064, 0.1064, 0.2547]; # $ / kWh
		data["comm_flat_price"] = [0.1142,0.1112,0.0843,0.0923,0.0923,0.2227];
	elif(use_flags["use_billing"] == 2): #TIERED RATE
		data["monthly_fee"] = 10; # $
		data["flat_price"] = 0.1; # $ / kWh - first tier price
		data["tier_energy"] = 500; # kWh - the transition between 1st and 2nd tier
		data["second_tier_price"] = 0.08; # $ / kWh
	elif(use_flags["use_billing"] == 3): #RTP/TOU RATE - market must be activated
		data["monthly_fee"] = 10; # $
		data["comm_monthly_fee"] = 25;
		data["flat_price"] = [0.1243,0.1294,0.1012,0.1064,0.1064,0.2547]; # $ / kWh
		if (use_flags["use_market"] == 0):
			print('Error: Must use markets when applying use_billing == 3');

	## Solar parameters
	if (use_flags["use_solar"] ==1 or use_flags["use_solar_res"] == 1 or use_flags["use_solar_com"] == 1):
		data["Rated_Insolation"] = 92.902; #W/Sf for 1000 W/m2
		data["efficiency_solar"] = 0.2; 
		data["solar_averagepower"] = 4 #For solar residential
		data["solar_averagepower_stripmall"] = 10 #For solar stripmall
		data["solar_averagepower_office"] = 100 #For solar commercial (offices)
		data["solar_averagepower_bigbox"] = 100 #For solar commercial (bigbox)

	## PHEV parameters
	if (use_flags["use_phev"] == 1):
		print("ERROR: PHEV is not implemented yet")


	## DA parameters
	if (use_flags["use_da"] == 1):
		print("ERROR: DA is not implemented yet")

	## Wind parameters
	if (use_flags["use_wind"] == 1):
		print("ERROR: Wind is not implemented yet")

	## Emission parameters
	if (use_flags["use_emissions"]  == 1):
		data["Naturalgas_Conv_Eff"] = 8.16; #MBtu/MWh
		data["Coal_Conv_Eff"] = 10.41;
		data["Biomass_Conv_Eff"] = 12.93;
		data["Geothermal_Conv_Eff"] = 21.02;
		data["Hydroelectric_Conv_Eff"] = 0;
		data["Nuclear_Conv_Eff"] = 10.46;
		data["Wind_Conv_Eff"] = 0;
		data["Petroleum_Conv_Eff"] = 11.00;
		data["Solarthermal_Conv_Eff"] = 0;

		data["Naturalgas_CO2"] = 117.08; #lb/MBtu;
		data["Coal_CO2"] = 205.573;
		data["Biomass_CO2"] = 195;
		data["Geothermal_CO2"] = 120;
		data["Hydroelectric_CO2"] = 0;
		data["Nuclear_CO2"] = 0;
		data["Wind_CO2"] = 0;
		data["Petroleum_CO2"] = 225.13;
		data["Solarthermal_CO2"] = 0;

		data["Naturalgas_SO2"] = 0.001; #lb/MBtu;
		data["Coal_SO2"] = 0.1;
		data["Biomass_SO2"] = 0;
		data["Geothermal_SO2"] = 0.2;
		data["Hydroelectric_SO2"] = 0;
		data["Nuclear_SO2"] = 0;
		data["Wind_SO2"] = 0;
		data["Petroleum_SO2"] = 0.1;
		data["Solarthermal_SO2"] = 0;    

		data["Naturalgas_NOx"] = 0.0075; #lb/MBtu;
		data["Coal_NOx"] = 0.06; 
		data["Biomass_NOx"] = 0.08;
		data["Geothermal_NOx"] = 0;             
		data["Hydroelectric_NOx"] = 0;
		data["Nuclear_NOx"] = 0;                 
		data["Wind_NOx"] = 0;
		data["Petroleum_NOx"] = 0.04;
		data["Solarthermal_NOx"] = 0;

		data["cycle_interval"] = 15; #min

	## Other parameters    
	# simulation start and end times -> please use format: yyyy-mm-dd HH:MM:SS
	data["start_date"] = '2012-06-28 00:00:00'; #summer dates
	data["end_date"] = '2012-06-30 00:00:00';
	data["start_date2"] = '2012-01-03 00:00:00'; #winter dates
	data["end_date2"] = '2012-01-05 00:00:00';
	data["start_date3"] = '2012-04-10 00:00:00'; #spring dates
	data["end_date3"] = '2012-04-12 00:00:00';
	# How often do you want to measure?
	#data["meas_interval"] = 900;  #applies to everything
	data["meas_interval"] = 300; # 5 minute intervals
	# @todo: fix 'datenum'
	#meas = datenum(data["end_date"]) - datenum(data["start_date"]); #days between start and end
	meas = 10000
	meas2 = meas*24*60*60;  #seconds between start and end dates
	data["meas_limit"] = int(20*math.ceil(meas2/data["meas_interval"]));

	# Skews
	#data["residential_skew_std"] = 2700; #45 min
	#data["residential_skew_std"] = 3600; # taking this from Configuration.py instead
	data["residential_skew_max"] = 8100; # 2hr 15min
	data["commercial_skew_std"] = 1800; #These are in 30 minute blocks
	data["commercial_skew_max"] = 5400;

	data["tech_number"] = data["tech_flag"];
	# end nasty long copy/paste block

	return (data, use_flags)
	# EOF