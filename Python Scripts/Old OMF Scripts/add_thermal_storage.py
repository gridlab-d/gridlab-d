import random
from __future__ import division
def add_ts(scalar, ts_array, ts_dict, use_flags, config_data, tech_data, key):
	# if there are no objects to attach thermal storage to, continue
	if ts_array is None or ts_array.empty():
		return
	# if the penetration scalar is non-positive, continue
	if scalar <= 0.0:
		return
	if scalar > 1.0:
		return
	# snag number of objects from array[0]
	total_number = ts_array[0]
	ts_count = total_number * scalar
	# put the numbers in the hat
	#random_index = []
	random_index = random.sample(list(xrange(total_number)), ts_count)

	# attach TS units to the dictionary
	for x in random_index:
		key += 1
		parent = ts_array[1][x]
		#phases = ts_array[2][x]
		ts_dict[key] = {"object" : "thermal_storage",
							"parent" : "{:s}".format(parent),
							"name" : "thermal_storage_{}".format(parent),
							"SOC" : "{:0.2f}".format(tech_data["ts_SOC"]),
							"k" : "{:.2f}".format(tech_data["k_ts"])}
		if ((use_flags["use_ts"]==2) or (use_flags["use_ts"]==4)):
			ts_dict[key].extend({"discharge_schedule_type" : "EXTERNAL",
									   "recharge_schedule_type" : "EXTERNAL",
									   "schedule_skew" : "{:.0f}".format(900*(9*random.uniform(0, 1)-4)),
									   "recharge_time" : "ts_recharge_schedule*1",
									   "discharge_time" : "ts_discharge_schedule*1"})
		pass
	pass

def Append_TS(TS_Tech_Dict, use_flags, config_data, tech_data, ts_bigbox_array=None, ts_office_array=None, ts_stripmall_array=None, ts_residential_array=None, last_object_key):
	# PV_Tech_Dict - the dictionary that we add solar objects to
	# use_flags - the output from TechnologyParameters.py
	# config_data - the output from Configuration.py
	# solar_bigbox_array - contains a list of commercial houses, corresponding floor areas, parents,and phases that commercial PV can be attached to
	# solar_office_array - contains a list of commercial houses, corresponding floor areas, parents,and phases that commercial PV can be attached to
	# solar_stripmall_array - contains a list of commercial houses, corresponding floor areas, parents,and phases that commercial PV can be attached to
	# solar_residential_array - contains a list of residential houses, corresponding floor areas, parents,and phases that residential PV can be attached to
	# last_object_key should be a numbered key that is the next key in PV_Tech_Dict
	
	# Initialize psuedo-random seed
	random.seed(4)
	
	use_ts = 0
	#                 0 = none
	# 1 = add thermal storage using the defaults,            2 = add thermal storage with a randomized schedule,
	# 3 = add thermal storage to all houses using defaults,  4 = add thermal storage to all houses with a randomized schedule
	
	#copied from feeder_generator_updated.m:4905-5074
	#RandStream.setDefaultStream(s5);
	
	if use_flags["use_ts"] != 0 or use_flags["use_ts_com"]:
		random.seed(4)
		# ts_penetration is currently set in Configuration.py to 10(%).
		# currently, just set it to the same value penetration. -MH
		scalar = 0.0
		if config_data.has_key("thermal_override"):
			scalar = config_data["thermal_override"] / 100
		else:
			scalar = config_data['ts_penetration'] / 100
		penetration_stripmall = scalar
		penetration_bigbox = scalar
		penetration_office = scalar

		# no need to set sizing values, everything auto-sizes
		modes = [ (penetration_office, ts_office_array),
					  (penetration_bigbox, ts_bigbox_array),
					  (penetration_stripmall, ts_stripmall_array)]
		for x in modes:
			add_ts(x(0), x(1), TS_Tech_Dict, use_flags, config_data, tech_data)

	return
	# 'old code' below this point
	
	if use_flags["use_ts"] !=0:
		if ts_office_array is not None:
			no_ts_office_length = ts_office_array.length()
			#no_ts_office=sum(cellfun('prodofsize',ts_office_array));
			no_ts_office = 0
			for i in ts_office_array:
				no_ts_office += i.length()
			office_penetration = []
			for i in xrange(0, no_ts_office.length):
				office_penetration[i] = random.random();
			office_count = 0

			for jj in xrange(0, no_ts_office_length):
				if ts_office_array[jj].length() > 0:
					for jjj in xrange(0, ts_office_array[jj].length()):
						#if (isfield(taxonomy_data,'thermal_override')):
						if config_data.has_key("thermal_override"):
							thermal_storage_penetration_level = config_data["thermal_override"]/100;
						else: #No exist, use regional value
							thermal_storage_penetration_level = config_data["ts_penetration"]/100;
					
						if (office_penetration(office_count) <= thermal_storage_penetration_level):
							for jjjj in xrange(0, ts_office_array[jj][jjj].length()):
								for jjjjj in xrange(0, ts_office_array[jj][jjj][jjjj].length()):
									#if (~isempty(ts_office_array[jj][jjj][jjjj][jjjjj])):
									if ts_office_array[jj][jjj][jjjj][jjjjj].length() > 0:
										parent = ts_office_array[jj][jjj][jjjj][jjjjj];
										TS_Tech_Dict[last_object_key] = {"object" : "thermal_storage",
								                                                                 "parent" : "{:s}".format(parent),
								                                                                 "name" : "thermal_storage_office_{:.0f}_#.0f_#.0f_#.0f".format(jj,jjj,jjjj,jjjjj),
								                                                                 "SOC" : "{:0.2f}".format(tech_data.ts_SOC),
								                                                                 "k" : "{:.2f}".format(tech_data.k_ts)}
										if ((use_flags["use_ts"]==2) or (use_flags["use_ts"]==4)):
											TS_Tech_Dict[last_object_key].extend({"discharge_schedule_type" : "EXTERNAL",
																							   "recharge_schedule_type" : "EXTERNAL",
																							   "schedule_skew" : "{:.0f}".format(900*(9*random.uniform(0, 1)-4)),
																							   "recharge_time" : "ts_recharge_schedule*1",
																							   "discharge_time" : "ts_discharge_schedule*1"})
										last_object_key += 1
										office_count += 1

		#if (exist('ts_bigbox_array','var')):
		if ts_bigbox_array is not None and ts_bigbox_array.length() > 0:
			#no_ts_bigbox_length=length(ts_bigbox_array);
			no_ts_bigbox_length = ts_bigbox_array.length()
			#no_ts_bigbox=sum(~cellfun('isempty',ts_bigbox_array));
			no_ts_bigbox = 0
			for i in ts_bigbox_array:
				no_ts_bigbox += i.length()
			#bigbox_penetration = rand(no_ts_bigbox,1);
			bigbox_penetration = []
			for i in xrange(0, no_ts_bigbox):
				bigbox_penetration[i] = random.random() 
			#bigbox_count=1;
			bigbox_count = 0 # 

			for jj in xrange(0, no_ts_bigbox_length):
				if ts_bigbox_array[jj].length() > 0:
					if "thermal_override" in config_data:
						thermal_storage_penetration_level=config_data["thermal_override"]/100;
					else: #No exist, use regional value
						thermal_storage_penetration_level = config_data["ts_penetration"]/100;

					if (bigbox_penetration[bigbox_count] <= thermal_storage_penetration_level):
						for jjj in xrange(0, ts_bigbox_array[jj].length()):
							for jjjj in xrange(0, ts_bigbox_array[jj][jjj].length()):
								for jjjjj in xrange(0, ts_bigbox_array[jj][jjj][jjjj].length()):
									if ts_bigbox_array[jj][jjj][jjjj][jjjjj].length() > 0:
										parent = ts_bigbox_array[jj][jjj][jjjj][jjjjj];
										TS_Tech_Dict[last_object_key] = {"object" : "thermal_storage",
																					"parent" : "{:s}".format(parent),
																					"name" :  "thermal_storage_bigbox_#.0f_#.0f_#.0f_#.0f".format(jj,jjj,jjjj,jjjjj),
																					"SOC" : "{:0.2f}".format(tech_data.ts_SOC),
																					"k" : "{:.2f}".format(tech_data.k_ts)}
										if ((use_flags["use_ts"]==2) or (use_flags["use_ts"]==4)):
											TS_Tech_Dict.extend({"discharge_schedule_type" : "EXTERNAL",
																		 "recharge_schedule_type" : "EXTERNAL",
																		 "schedule_skew" : "#.0f".format(900*(9*random.random()-4)),
																	  	 "recharge_time" : "ts_recharge_schedule*1",
																		 "discharge_time" : "ts_discharge_schedule*1"})
										bigbox_count += 1
										last_object_key += 1

		if ts_stripmall_array is not None:
			no_ts_stripmall_length = ts_stripmall_array.length()
			#no_ts_stripmall=sum(~cellfun('isempty',ts_stripmall_array));
			no_ts_stripmall = 0
			for i in ts_stripmall_array:
				no_ts_stripmall += i.length()
			stripmall_penetration = []
			for i in xrange(0, no_ts_stripmall):
				stripmall_penetration[i] = random.random();
			#stripmall_count = 1
			stripmall_count = 0 # index

			for jj in xrange(0, no_ts_stripmall_length):
				if ts_stripmall_array[jj].length() > 0:
					if 'thermal_override' in config_data:
						thermal_storage_penetration_level = config_data["thermal_override"]/100;
					else: #No exist, use regional value
						thermal_storage_penetration_level = config_data["ts_penetration"]/100;
					
					if (stripmall_penetration[stripmall_count] <= thermal_storage_penetration_level):
						for jjj in xrange(0, ts_stripmall_array[jj].length()):
							for jjjj in xrange(0, ts_stripmall_array[jj][jjj].length()):
								if ts_stripmall_array[jj][jjj][jjjj].length() > 0:
									parent = ts_stripmall_array[jj][jjj][jjjj];
									TS_Tech_Dict[last_object_key] = {"object" : "thermal_storage",
												                                        "parent" : "{:s}".format(parent),
												                                        "name" : "thermal_storage_stripmall_{:.0f}_{:.0f}_{:.0f}".format(jj,jjj,jjjj),
												                                        "SOC" : "{:0.2f}".format(tech_data.ts_SOC),
												                                        "k" : "{:.2f}".format(tech_data.k_ts)}
									if ((use_flags["use_ts"]==2) or (use_flags["use_ts"]==4)):
										TS_Tech_Dict[last_object_key].extend({"discharge_schedule_type" : "EXTERNAL",
																						   "recharge_schedule_type" : "EXTERNAL",
																						   "schedule_skew" :  "{:.0f}".format(900*(9*random.random()-4)),
																						   "recharge_time" : "ts_recharge_schedule*1",
																						   "discharge_time" : "ts_discharge_schedule*1"})
									stripmall_count += 1;
									last_object_key += 1

		if ts_residential_array is not None and (use_flags["use_ts"]==3 or use_flags["use_ts"]==4):
			no_ts_residential_length = ts_residential_array.length()
			#no_ts_stripmall=sum(~cellfun('isempty',ts_stripmall_array));
			no_ts_residential = 0
			for i in ts_residential_array:
				no_ts_residential += i.length()
			residential_penetration = []
			for i in xrange(0, ts_residential_array):
				residential_penetration[i] = random.random();
			residential_count = 0 # index

			for jj in xrange(0, no_ts_residential_length.length()):
				if ts_residential_array[jj].length() > 0:
					for jjj in xrange(0, ts_residential_array[jj].length()):
						if ts_residential_array[jj][jjj].length() > 0:
							if "thermal_override" in config_data:
								thermal_storage_penetration_level = config_data["thermal_override"]/100;
							else: #No exist, use regional value
								thermal_storage_penetration_level = config_data["ts_penetration"]/100;

							if (residential_penetration[residential_count] <= thermal_storage_penetration_level):
								parent = ts_residential_array[jj][jjj];
								TS_Tech_Dict[last_object_key] = {"object" : "thermal_storage",
																			"parent" : "{:s}".format(parent),
																			"name" : "thermal_storage_residential_{:.0f}_{:.0f}".format(jj,jjj),
																			"SOC" : "{:0.2f}".format(tech_data.ts_SOC),
																			"k" : "{:.2f}".format(tech_data.k_ts)}
								if ((use_flags["use_ts"]==2) or (use_flags["use_ts"]==4)):
									TS_Tech_Dict[last_object_key].extend({"discharge_schedule_type" : "EXTERNAL",
													                                           "recharge_schedule_type" : "EXTERNAL",
													                                           "schedule_skew" : "#.0f".format(900* (9*random.random()-4)),
													                                           "recharge_time" : "ts_recharge_schedule*1",
													                                           "discharge_time" : "ts_discharge_schedule*1" })
								residential_count += 1
								last_object_key += 1
	return TS_Tech_Dict

def main():
	#tests here
	#TSGLM = Append_Solar(PV_Tech_Dict,use_flags,)
	pass

if __name__ ==  '__main__':
	main()
