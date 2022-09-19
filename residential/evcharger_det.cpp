/** $Id: evcharger_det_.cpp
	Copyright (C) 2019 Battelle Memorial Institute
 **/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "evcharger_det.h"

//////////////////////////////////////////////////////////////////////////
// evcharger_det CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* evcharger_det::oclass = nullptr;
CLASS* evcharger_det::pclass = nullptr;

evcharger_det::evcharger_det(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==nullptr)
	{
		// register the class definition
		oclass = gl_register_class(module,"evcharger_det",sizeof(evcharger_det),PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==nullptr)
			throw "unable to register class evcharger_det";

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double,"desired_charge_rate[W]", PADDR(DesiredChargeRate),PT_DESCRIPTION, "Current desired charge rate of the vehicle",
			PT_double,"actual_charge_rate[W]", PADDR(ActualChargeRate), PT_DESCRIPTION, "Actual charge rate of the vehicle - may be ramp limited",
			PT_double,"realized_charge_rate[kW]", PADDR(RealizedChargeRate), PT_DESCRIPTION, "Implemented charge rate of the vehicle - grid-influence result of actual_charge_rate",

			PT_double,"variation_mean[s]", PADDR(variation_mean),PT_DESCRIPTION, "Mean of normal variation of schedule variation",
			PT_double,"variation_std_dev[s]", PADDR(variation_std_dev),PT_DESCRIPTION, "Standard deviation of normal variation of schedule times",

			PT_double,"variation_trip_mean[mile]", PADDR(variation_trip_mean),PT_DESCRIPTION, "Mean of normal variation of trip distance variation",
			PT_double,"variation_trip_std_dev[mile]", PADDR(variation_trip_std_dev),PT_DESCRIPTION, "Standard deviation of normal variation of trip distance",

			PT_double,"mileage_classification[mile]", PADDR(mileage_classification),PT_DESCRIPTION, "Mileage classification of electric vehicle",
			PT_bool,"work_charging_available", PADDR(Work_Charge_Available), PT_DESCRIPTION, "Charging available when at work",

			PT_char1024,"data_file", PADDR(NHTSDataFile), PT_DESCRIPTION, "Path to .CSV file with vehicle travel information",
			PT_int32,"vehicle_index", PADDR(VehicleLocation), PT_DESCRIPTION, "Index of vehicles in file this particular vehicle's data",

			PT_enumeration,"vehicle_location", PADDR(CarInformation.Location),
				PT_KEYWORD,"UNKNOWN",(enumeration)VL_UNKNOWN,
				PT_KEYWORD,"HOME",(enumeration)VL_HOME,
				PT_KEYWORD,"WORK",(enumeration)VL_WORK,
				PT_KEYWORD,"DRIVING_HOME",(enumeration)VL_WORK_TO_HOME,
				PT_KEYWORD,"DRIVING_WORK",(enumeration)VL_HOME_TO_WORK,
			PT_double,"travel_distance[mile]",PADDR(CarInformation.travel_distance), PT_DESCRIPTION, "Distance vehicle travels from home to home (round trip)",
			PT_double,"arrival_at_work",PADDR(CarInformation.WorkArrive), PT_DESCRIPTION, "Time vehicle arrives at work - HHMM",
			PT_double,"duration_at_work[s]",PADDR(CarInformation.WorkDuration), PT_DESCRIPTION, "Duration the vehicle remains at work",
			PT_double,"arrival_at_home",PADDR(CarInformation.HomeArrive), PT_DESCRIPTION, "Time vehicle arrives at home - HHMM",
			PT_double,"duration_at_home[s]",PADDR(CarInformation.HomeDuration), PT_DESCRIPTION, "Duration the vehicle remains at home",
			PT_double,"battery_capacity[kWh]",PADDR(CarInformation.battery_capacity), PT_DESCRIPTION, "Current capacity of the battery in kWh",
			PT_double,"battery_SOC[%]",PADDR(CarInformation.battery_SOC), PT_DESCRIPTION, "State of charge of battery",
			PT_double,"battery_size[kWh]",PADDR(CarInformation.battery_size), PT_DESCRIPTION, "Full capacity of battery",
			PT_double,"mileage_efficiency[mile/kWh]",PADDR(CarInformation.mileage_efficiency), PT_DESCRIPTION, "Efficiency of drive train in mile/kWh",
			PT_double,"maximum_charge_rate[W]",PADDR(CarInformation.MaxChargeRate), PT_DESCRIPTION, "Maximum output rate of charger in kW",
			PT_double,"charging_efficiency[unit]",PADDR(CarInformation.ChargeEfficiency), PT_DESCRIPTION, "Efficiency of charger (ratio) when charging",

			PT_double,"maximum_overload_current[unit]",PADDR(max_overload_currentPU), PT_DESCRIPTION, "Maximum overload current, in per-unit",

			PT_bool,"enable_J2894", PADDR(enable_J2894_checks), PT_DESCRIPTION, "Enable SAE J2894-suggested voltage and ramping suggestions",
			PT_bool,"charger_enabled",PADDR(ev_charger_enabled_state), PT_DESCRIPTION, "Flag to indicate if the charger is working, or disconnected due to SAE J2894/ANSI C84.1",

			PT_enumeration,"J2894_trip_method",PADDR(J2894_trip_method), PT_DESCRIPTION, "SAE J2894 threshold exceeded for the tripping",
				PT_KEYWORD,"NONE",(enumeration)J2894_NONE,
				PT_KEYWORD,"SURGE_OVER_VOLT",(enumeration)J2894_SURGE_OVER_VOLT,
				PT_KEYWORD,"HIGH_OVER_VOLT",(enumeration)J2894_HIGH_OVER_VOLT,
				PT_KEYWORD,"OVER_VOLT",(enumeration)J2894_OVER_VOLT,
				PT_KEYWORD,"UNDER_VOLT",(enumeration)J2894_UNDER_VOLT,
				PT_KEYWORD,"LOW_UNDER_VOLT",(enumeration)J2894_LOW_UNDER_VOLT,
				PT_KEYWORD,"EXTREME_UNDER_VOLT",(enumeration)J2894_EXTREME_UNDER_VOLT,
				PT_KEYWORD,"UNKNOWN",(enumeration)J2894_UNKNOWN,

			//J2894-related thresholds
			PT_double,"J2894_highest_voltage_surge_value[pu]", PADDR(J2894_voltage_high_threshold_values[0][0]), PT_DESCRIPTION, "J2894-suggested high voltage surge threshold magnitude",
			PT_double,"J2894_highest_voltage_surge_time[s]", PADDR(J2894_voltage_high_threshold_values[0][1]), PT_DESCRIPTION, "J2894-suggested high voltage surge threshold duration",
			PT_double, "J2894_high_voltage_persistent_value[pu]", PADDR(J2894_voltage_high_threshold_values[1][0]), PT_DESCRIPTION, "J2894-suggested higher-voltage momentary magnitude",
			PT_double, "J2894_high_voltage_persistent_time[s]", PADDR(J2894_voltage_high_threshold_values[1][1]), PT_DESCRIPTION, "J2894-suggsted higher-voltage momentary duration",

			PT_double,"J2894_lowest_voltage_surge_value[pu]", PADDR(J2894_voltage_low_threshold_values[0][0]), PT_DESCRIPTION, "J2894-suggested low voltage surge threshold magnitude",
			PT_double,"J2894_lowest_voltage_surge_time[s]", PADDR(J2894_voltage_low_threshold_values[0][1]), PT_DESCRIPTION, "J2894-suggested low voltage surge threshold duration",
			PT_double, "J2894_low_voltage_persistent_value[pu]", PADDR(J2894_voltage_low_threshold_values[1][0]), PT_DESCRIPTION, "J2894-suggested lower-voltage momentary magnitude",
			PT_double, "J2894_low_voltage_persistent_time[s]", PADDR(J2894_voltage_low_threshold_values[1][1]), PT_DESCRIPTION, "J2894-suggested lower-voltage momentary duration",

			PT_double, "J2894_ramp_rate_limit[A/s]",PADDR(J2894_ramp_limit),PT_DESCRIPTION,"J2894-suggested ramp rate limit for charger activation",

			PT_double, "J2894_outage_disconnect_interval[s]", PADDR(J2894_off_threshold), PT_DESCRIPTION, "J2894-suggested outage length, when criterion has been exceeded",

			nullptr)<1)
			GL_THROW("unable to publish properties in %s",__FILE__);

			if (gl_publish_function(oclass,	"interupdate_res_object", (FUNCTIONADDR)interupdate_evcharger_det)==nullptr)
				GL_THROW("Unable to publish evcharger_det deltamode function");
	}
}

int evcharger_det::create() 
{
	int create_res = residential_enduse::create();

	DesiredChargeRate = 0.0;			//Starts off
	ActualChargeRate = 0.0;				//Starts off
	RealizedChargeRate = 0.0;			//Starts off
	load.power_factor = 0.99;	//Based on Brusa NLG5 specifications
	load.heatgain_fraction = 0.0;	//Assumed to be "in the garage" or outside - so not contributing to internal heat gains

	load.shape = nullptr;		//No loadshape - forces it to use direct updates

	//Set the enduse properties, even though we're just using them as placeholder
	load.power_fraction = 1.0;
	load.current_fraction = 0.0;
	load.impedance_fraction = 0.0;

	variation_mean = 0.0;		//No variation, by default
	variation_std_dev = 0.0;

	variation_trip_mean = 0.0;	//No variation in trips by default either
	variation_trip_std_dev = 0.0;

	mileage_classification = 33;	//PHEV 33, by default

	Work_Charge_Available = false;	//No work charging
	
	NHTSDataFile[0] = '\0';			//Null file
	VehicleLocation = 0;		

	//Car information - battery info is left uninitialized
	CarInformation.battery_capacity = -1.0;	//Flags
	CarInformation.battery_size = -1.0;		//Flags
	CarInformation.battery_SOC = -1.0;		//Flags
	CarInformation.ChargeEfficiency = 0.90;
	CarInformation.HomeArrive = 1700.0;			//Arrive home at 5 PM
	CarInformation.HomeDuration = 41400.0;		//At home for 11.5 hours
	CarInformation.HomeWorkDuration = 1800.0;	// Half hour commute into work
	CarInformation.Location = VL_HOME;
	CarInformation.MaxChargeRate = 1700.0;
	CarInformation.mileage_efficiency = 3.846;	//(0.26 kWh/mile - from MKM report)
	CarInformation.next_state_change = 0.0;		//Next time the state will change
	CarInformation.travel_distance = 15.0;		//15 mile default, just because
	CarInformation.WorkArrive = 500.0;			//Arrive at work at 5 AM
	CarInformation.WorkDuration = 41400.0;		//Assumed at work for 11.5 hours as well
	CarInformation.WorkHomeDuration = 1800.0;	//Half hour commute home

	prev_time_dbl = 0.0;

	max_overload_currentPU = 1.2;	//By default, assume can only do 1.2 pu current output
	max_overload_charge_current = max_overload_currentPU * CarInformation.MaxChargeRate / 120.0 / 1000.0;	//Arbitrarily based off 120 - gets fixed below.

	off_nominal_time = false;	//Assumes minimum timesteps aren't screwing things up, by default

	deltamode_inclusive = false;		//By default, no deltamode participation

	enable_J2894_checks = false;		//By default, don't check for J2894-recommended operating regions

	//Populate the thresholds with defaults (per-unit-based
	//Column 1 is p.u. threshold, column 2 is allowed duration
	J2894_voltage_high_threshold_values[0][0] = 1.75;
	J2894_voltage_high_threshold_values[0][1] = 0.5 / default_grid_frequency;	//Half cycle
	J2894_voltage_high_threshold_values[1][0] = 1.25;
	J2894_voltage_high_threshold_values[1][1] = 3.0;

	J2894_voltage_low_threshold_values[0][0] = 0.0;
	J2894_voltage_low_threshold_values[0][1] = 0.2;
	J2894_voltage_low_threshold_values[1][0] = 0.8;
	J2894_voltage_low_threshold_values[1][1] = 10.0;

	//Default outage time
	J2894_off_threshold = 120.0 + gl_random_normal(RNGSTATE,60.0,15.0);	//2 minutes + pseudo-random interval - randomizer is arbitrary

	ev_charger_enabled_state = true;		//Starts enabled
	J2894_trip_method = J2894_NONE;			//Nothing tripped at start

	//Zero the accumulators - just because
	J2894_voltage_high_accumulators[0] = J2894_voltage_high_accumulators[1] = 0.0;
	J2894_voltage_low_accumulators[0] = J2894_voltage_low_accumulators[1] = 0.0;
	J2894_voltage_high_state[0] = J2894_voltage_high_state[1] = false;	//Not in any states
	J2894_voltage_low_state[0] = J2894_voltage_low_state[1] = false;
	J2894_off_accumulator = 0.0;

	J2894_ramp_limit = 40.0;	//40 A/s default rate
	J2894_is_ramp_constrained = false;	//Not ramp limited, by default

	return create_res;
}

int evcharger_det::init(OBJECT *parent)
{
	if(parent != nullptr){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("evcharger_det::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	OBJECT *hdr = OBJECTHDR(this);
	int init_res;
	int comma_count, curr_idx, curr_comma_count;
	char temp_char;
	char temp_char_value[33];
	char *curr_ptr, *end_ptr;
	double home_arrive, home_durr, temp_miles, work_arrive, work_durr;
	double temp_hours_curr, temp_hours_curr_two, temp_hours_A, temp_hours_B, temp_hours_C, temp_hours_D;
	double temp_sec_curr, temp_sec_curr_two, temp_sec_A, temp_sec_B, temp_sec_C, temp_sec_D;
	double temp_amps;
	FILE *FPTemp;
	TIMESTAMP temp_time, temp_time_dbl, prev_temp_time;
	DATETIME temp_date;
	gld_property *temp_property;
	gld_wlock *test_rlock;
	int32 temp_int_val;

	//Map the minimum timestep
	temp_property = nullptr;

	//Get linking to checker variable
	temp_property = new gld_property("minimum_timestep");

	//See if it worked
	if ((temp_property->is_valid() != true) || (temp_property->is_integer() != true))
	{
		GL_THROW("evcharger_det:%s - Failed to map global minimum timestep variable",hdr->name?hdr->name:"unnamed");
		/*  TROUBLESHOOT
		While attempting to map the minimum_timestep global variable, an error was encounter
		Please try again, insuring the diesel_dg is parented to a deltamode powerflow object.  If
		the error persists, please submit your code and a bug report via the ticketing system.
		*/
	}

	//Pull the value
	temp_property->getp<int32>(temp_int_val,*test_rlock);

	//remove the mapping
	delete temp_property;

	//Convert it to a timestep
	glob_min_timestep = (TIMESTAMP)temp_int_val;

	if (glob_min_timestep > 1)					//Now check us
	{
		off_nominal_time=true;					//Set flag
		gl_verbose("evcharger_det:%s - minimum_timestep set - problems may emerge",hdr->name);
		/*  TROUBLESHOOT
		The evcharger detected that the forced minimum timestep feature is enabled.  This may cause
		issues with the evcharger schedule, especially if arrival/departure times are shorter than a
		minimum timestep value.
		*/
	}

	//Set the double version
	glob_min_timestep_dbl = (double)glob_min_timestep;

	//End-use properties
	//Check the maximum charge rate - if over 1.7 kW, see if it is 220 VAC
	if (CarInformation.MaxChargeRate > 1700)
	{
		if (load.config != EUC_IS220)
		{
			gl_warning("evcharger_det:%s - The max charge rate is over 1.7 kW (Level 1), but the load is still 110-V connected.",hdr->name ? hdr->name : "unnamed");
			/*  TROUBLESHOOT
			The evcharger_det maximum_charge_rate implies a level 2 or higher charge rate, but it still set up to be connected as a normal level 1 110/120 VAC device.
			This may cause issues with circuit breakers tripping.  If this is undesired, be sure to specify the proper configuration property.
			*/
		}
	}

	//Check breaker rating
	//Get rough calculation
	if (load.config == EUC_IS220)
	{
		expected_voltage_base = 2.0 * default_line_voltage;
	}
	else	//Assume 110/120 connected
	{
		expected_voltage_base = default_line_voltage;
	}

	//Calculate the current
	temp_amps = floor(((CarInformation.MaxChargeRate * 1.1) / expected_voltage_base) + 0.5);

	//See if it is bigger
	if (temp_amps > load.breaker_amps)
	{
		gl_warning("evcharger_det:%s - the breaker rating may be too low for this maximum charge rate.",hdr->name ? hdr->name : "unnamed");
		/*  TROUBLESHOOT
		The evcharger_det maximum_charge_rate and configuration calculated a maximum breaker current that may be above the current setting.
		This may cause the breaker to trip and affect simulation results.  If this is undesired, set the breaker_amps property to a higher value.
		*/
	}

	//Make sure the "max current overload" multiplier is valid
	if (max_overload_currentPU <= 0.0)	//can't be negative or zero
	{
		GL_THROW("evcharger_det:%d %s - maximum_overload_current must be positive and non-zero!",hdr->id,(hdr->name ? hdr->name : "Unnamed"));
		/*  TROUBLESHOOT
		The value for maximum_overload_current must be positive - it cannot be negative or zero.
		*/
	}

	//Populate the "max current" value, based on the "amps" above - eventual rates are in kW, so reflect here
	max_overload_charge_current = max_overload_currentPU * CarInformation.MaxChargeRate / expected_voltage_base / 1000.0;

	//Initialize enduse structure
	init_res = residential_enduse::init(parent);

	//Set end index of character array, just cause
	temp_char_value[32] = '\0';

	//See if a file has been specified
	if (NHTSDataFile[0] != '\0')	//At least has something in it
	{
		//Make sure we have what appears to be a valid index
		if (VehicleLocation==0)
		{
			gl_warning("Vehicle location not set, using defaults");
			/*  TROUBLESHOOT
			The value for vehicle_index was not set, so a read of the NHTS file was not even attempted.
			A set of default values will be used instead.
			*/
		}
		else	//"Semi" valid (may be too big)
		{
			//Try to open if
			FPTemp = fopen(NHTSDataFile,"rt");

			//Make sure it worked
			if (FPTemp == nullptr)
			{
				gl_warning("NHTS data file not found, using defaults");
				/*  TROUBLESHOOT
				The formatted .CSV file of NHTS (or NHTS-like) data was not found.
				A set of default values will be used instead.
				*/
			}
			else
			{
				//Determine number of commas until will be there
				comma_count = (int)(VehicleLocation)*9;

				//Parse through until comma count reached - then look for CR
				curr_idx = 0;
				curr_comma_count = 0;

				//Get character
				temp_char = fgetc(FPTemp);

				while ((temp_char > 0) && (curr_comma_count < comma_count))
				{
					//See if it is a delimiter
					if (temp_char == ',')
						curr_comma_count++;

					//Get the next one
					temp_char = fgetc(FPTemp);
				}

				//Now continue on to the CR
				while ((temp_char > 0) && (temp_char != '\n'))
				{
					//Get the next one
					temp_char = fgetc(FPTemp);
				}

				//Make sure it is right
				if (temp_char == '\n')
				{
					curr_comma_count = 0;

					//Fill the buffer to the first comma
					temp_char = fgetc(FPTemp);

					//Parse - find commas
					while (temp_char > 0)
					{
						//See if the current character is a comma
						if (temp_char==',')
						{
							//Increment
							curr_comma_count++;

							//See if we've had enough
							if (curr_comma_count>=3)
							{
								//Get us out of the while
								break;
							}
						}

						//Get next value
						temp_char = getc(FPTemp);
					}// End while

					//Make sure we succeeded
					if (curr_comma_count == 3)
					{
						//Get next value
						temp_char = fgetc(FPTemp);

						//While loop
						while ((temp_char > 0) && (curr_idx < 32))
						{
							if (temp_char == ',')
							{
								//Outta the loop we go
								break;
							}
							else	//Not a comma, store it
							{
								//Store the value
								temp_char_value[curr_idx] = temp_char;

								//Increment
								curr_idx++;

								//Check the index
								if (curr_idx>31)
								{
									GL_THROW("NHTS entry exceeded 32 characters");
									/*  TROUBLESHOOT
									The reading buffer for parsing the NHTS data file was exceeded.  Please check your
									data file and try again.  If your entries are over 32 characters long, this will fail.
									*/
								}
							}

							//Get next value
							temp_char = fgetc(FPTemp);
						}//End While

						//Store a null at the last position - outside to catch invalids
						temp_char_value[curr_idx]='\0';

						//Put the pointer to the buffer
						curr_ptr = temp_char_value;

						//Get home arrive_time
						home_arrive = strtod(curr_ptr,&end_ptr);

						//Reset
						curr_idx = 0;

						//Get next value
						temp_char = fgetc(FPTemp);

						//Loop until comma
						while ((temp_char > 0) && (curr_idx < 32))
						{
							if (temp_char == ',')
							{
								//Outta the loop we go
								break;
							}
							else	//Not a comma, store it
							{
								//Store the value
								temp_char_value[curr_idx] = temp_char;

								//Increment
								curr_idx++;

								//Check the index
								if (curr_idx>31)
								{
									GL_THROW("NHTS entry exceeded 32 characters");
									//Define above
								}
							}

							//Get next value
							temp_char = fgetc(FPTemp);
						}//End While

						//Store a null at the last position - outside to catch invalids
						temp_char_value[curr_idx]='\0';

						//Put the pointer to the buffer
						curr_ptr = temp_char_value;

						//Get home duration_time
						home_durr = strtod(curr_ptr,&end_ptr);

						//Skip the next entry
						temp_char = fgetc(FPTemp);

						//Loop until comma
						while (temp_char > 0)
						{
							if (temp_char == ',')
							{
								//Outta the loop we go
								break;
							}

							//Get next value
							temp_char = fgetc(FPTemp);
						}//End While

						//Next one is miles - keep
						//reset
						curr_idx = 0;

						//Get next value
						temp_char = fgetc(FPTemp);

						//Loop until comma
						while ((temp_char > 0) && (curr_idx < 32))
						{
							if (temp_char == ',')
							{
								//Outta the loop we go
								break;
							}
							else	//Not a comma, store it
							{
								//Store the value
								temp_char_value[curr_idx] = temp_char;

								//Increment
								curr_idx++;

								//Check the index
								if (curr_idx>31)
								{
									GL_THROW("NHTS entry exceeded 32 characters");
									//Define above
								}
							}

							//Get next value
							temp_char = fgetc(FPTemp);
						}//End While

						//Store a null at the last position - outside to catch invalids
						temp_char_value[curr_idx]='\0';

						//Put the pointer to the buffer
						curr_ptr = temp_char_value;

						//Get miles
						temp_miles = strtod(curr_ptr,&end_ptr);

						//Reset
						curr_idx = 0;

						//Get next value
						temp_char = fgetc(FPTemp);

						//Loop until comma
						while ((temp_char > 0) && (curr_idx < 32))
						{
							if (temp_char == ',')
							{
								//Outta the loop we go
								break;
							}
							else	//Not a comma, store it
							{
								//Store the value
								temp_char_value[curr_idx] = temp_char;

								//Increment
								curr_idx++;

								//Check the index
								if (curr_idx>31)
								{
									GL_THROW("NHTS entry exceeded 32 characters");
									//Define above
								}
							}

							//Get next value
							temp_char = fgetc(FPTemp);
						}//End While

						//Store a null at the last position - outside to catch invalids
						temp_char_value[curr_idx]='\0';

						//Put the pointer to the buffer
						curr_ptr = temp_char_value;

						//Get work arrive_time
						work_arrive = strtod(curr_ptr,&end_ptr);

						//Reset
						curr_idx = 0;

						//Get next value
						temp_char = fgetc(FPTemp);

						//Loop until comma
						while ((temp_char > 0) && (curr_idx < 32))
						{
							if (temp_char == ',')
							{
								//Outta the loop we go
								break;
							}
							else	//Not a comma, store it
							{
								//Store the value
								temp_char_value[curr_idx] = temp_char;

								//Increment
								curr_idx++;

								//Check the index
								if (curr_idx>31)
								{
									GL_THROW("NHTS entry exceeded 32 characters");
									//Define above
								}
							}

							//Get next value
							temp_char = fgetc(FPTemp);
						}//End While

						//Store a null at the last position - outside to catch invalids
						temp_char_value[curr_idx]='\0';

						//Put the pointer to the buffer
						curr_ptr = temp_char_value;

						//Get work duration_time
						work_durr = strtod(curr_ptr,&end_ptr);

						//Put extracted results into structure
						CarInformation.HomeArrive = home_arrive;
						CarInformation.HomeDuration = home_durr * 60.0;	//In minutes, make seconds
						CarInformation.WorkArrive = work_arrive;
						CarInformation.WorkDuration = work_durr * 60.0;	//In minutes, make seconds
						CarInformation.travel_distance = temp_miles;
					}
					else
					{
						GL_THROW("Invalid entry in NHTS file - may have exceeded file length!");
						/*  TROUBLESHOOT
						An invalid index value in the NHTS data file was attempted.  This could occur if an index
						larger than the maximum number of entries was attempted.  Please ensure a valid range
						was specified, or the NHTS-data file is correct.
						*/
					}
				}	//End CR - valid entry
				else
				{
					//Close the file handle, if necessary
					fclose(FPTemp);

					GL_THROW("Invalid entry in NHTS file - may have exceeded file length!");
					//Defined above
				}

				//Close the file handle, if necessary
				fclose(FPTemp);
			}
		}//End Vehicle location "valid"
	}
	else
	{
		gl_warning("NHTS data file not found, using defaults");
		//Defined above
	}

	//Initialize "tracking time", otherwise problems emerge
	prev_temp_time = gl_globalclock;
	prev_time_dbl = (double)prev_temp_time;

	//Convert it to a timestamp value
	temp_time = gl_globalclock;
	temp_time_dbl = (double)temp_time;

	//Extract the relevant time information
	gl_localtime(temp_time,&temp_date);

	//Determine our starting location
	//See if home first
	//temp_hours_curr = (double)(int)(CarInformation.HomeArrive/100);
	temp_hours_curr = floor(CarInformation.HomeArrive / 100);
	//gl_verbose("INIT: temp_hours_curr (HomeArrive) = %0.4f.", temp_hours_curr);
	temp_hours_curr_two = (CarInformation.HomeArrive - temp_hours_curr*100) / 60.0;
	temp_hours_A = temp_hours_curr + temp_hours_curr_two;									//Home Arrive
	temp_hours_B = temp_hours_A + CarInformation.HomeDuration/3600.0;						//Home Depart

	temp_sec_curr = temp_hours_curr * 3600.0;
	temp_sec_curr_two = (CarInformation.HomeArrive - temp_hours_curr*100) * 60.0;
	temp_sec_A = temp_sec_curr + temp_sec_curr_two;
	temp_sec_B = temp_sec_A + CarInformation.HomeDuration;

	if (temp_hours_B > 24.0)		//Wrap for "overnight"
		temp_hours_B -= 24.0;

	if(temp_sec_B > 86400.0){
		temp_sec_B -= 86400.0;
	}

	//temp_hours_curr = (double)(int)(CarInformation.WorkArrive/100);
	temp_hours_curr = floor(CarInformation.WorkArrive/100);
	//gl_verbose("INIT: temp_hours_curr (WorkArrive) = %0.4f.", temp_hours_curr);
	temp_hours_curr_two = (CarInformation.WorkArrive - temp_hours_curr*100) / 60.0;
	temp_hours_C = temp_hours_curr + temp_hours_curr_two;									//Work Arrive
	temp_hours_D = temp_hours_C + CarInformation.WorkDuration/3600.0;						//Work Depart

	temp_sec_curr = temp_hours_curr * 3600.0;
	temp_sec_curr_two = (CarInformation.WorkArrive - temp_hours_curr*100) * 60.0;
	temp_sec_C = temp_sec_curr + temp_sec_curr_two;
	temp_sec_D = temp_sec_C + CarInformation.WorkDuration;

	if (temp_hours_D > 24.0)		//Wrap for "overnight"
		temp_hours_D -= 24.0;

	if(temp_sec_D > 86400.0){
		temp_sec_D -= 86400.0;
	}

	//Populate other two entries
	//Commute home
	if (temp_hours_D > temp_hours_A)	//Midnight cross over
	{
		CarInformation.WorkHomeDuration = (86400.0 - temp_sec_D + temp_sec_A);
	}
	else	//Normal
	{
		CarInformation.WorkHomeDuration = (temp_sec_A - temp_sec_D);
	}

	//Commute to work
	if (temp_hours_C < temp_hours_B)	//Midnight cross over
	{
		CarInformation.HomeWorkDuration = (86400.0 - temp_sec_B + temp_sec_C);
	}
	else	//Normal
	{
		CarInformation.HomeWorkDuration = (temp_sec_C - temp_sec_B);
	}
	
	//Get current hours in right format
	temp_hours_curr = ((double)(temp_date.hour)) + (((double)(temp_date.minute))/60.0) + (((double)(temp_date.second))/3600.0);
	temp_sec_curr = ((double)(temp_date.hour))*3600.0 + ((double)(temp_date.minute))*60.0 + ((double)(temp_date.second));
	
	//Determine the schedule we are in
	if (temp_sec_A < temp_sec_B)	//HArrive < HDepart
	{
		if (temp_sec_C < temp_sec_D)	//WArrive < WDepart
		{
			if (temp_sec_A < temp_sec_C)	//HArrive < WArrive - driving initially, but home
			{
				//AH DH AW DW;
				if ((temp_sec_curr < temp_sec_A) || (temp_sec_curr >= temp_sec_D))	//Departed work, but not at home yet
				{
					//Set location
					CarInformation.Location = VL_WORK_TO_HOME;

					//Figure out time to next transition
					if (temp_sec_curr < temp_sec_A)
					{
						CarInformation.next_state_change = temp_time_dbl + (temp_sec_A - temp_sec_curr);
					}
					else
					{
						CarInformation.next_state_change = temp_time_dbl +(temp_sec_A + 86400.0 - temp_sec_curr);
					}
				}
				else if ((temp_sec_curr >= temp_sec_A) && (temp_sec_curr < temp_sec_B))	//At home
				{
					//Set location
					CarInformation.Location = VL_HOME;

					//Figure out time to next transition
					CarInformation.next_state_change = temp_time_dbl + (temp_sec_B - temp_sec_curr);
				}
				else if ((temp_sec_curr >= temp_sec_B) && (temp_sec_curr < temp_sec_C))		//Departed home, but not at work
				{
					//Set location
					CarInformation.Location = VL_HOME_TO_WORK;

					//Figure out time to next transition
					CarInformation.next_state_change = temp_time_dbl + (temp_sec_C - temp_sec_curr);
				}
				else	//Must be at work
				{
					//Set location
					CarInformation.Location = VL_WORK;

					//Figure out time to next transition
					CarInformation.next_state_change = temp_time_dbl + (temp_sec_D - temp_sec_curr);
				}
			}
			else	//WArrive < HArrive - driving initially, but to work
			{
				//AW DW AH DH;
				if ((temp_sec_curr < temp_sec_C) || (temp_sec_curr >= temp_sec_B))	//Departed home, but not at work yet
				{
					//Set location
					CarInformation.Location = VL_HOME_TO_WORK;

					//Figure out time to next transition
					if (temp_sec_curr < temp_sec_C)
					{
						CarInformation.next_state_change = temp_time_dbl + (temp_sec_C - temp_sec_curr);
					}
					else
					{
						CarInformation.next_state_change = temp_time_dbl + (temp_sec_C + 86400.0 - temp_sec_curr);
					}
				}
				else if ((temp_sec_curr >= temp_sec_C) && (temp_sec_curr < temp_sec_D))	//At work
				{
					//Set location
					CarInformation.Location = VL_WORK;

					//Figure out time to next transition
					CarInformation.next_state_change = temp_time_dbl + (temp_sec_D - temp_sec_curr);
				}
				else if ((temp_sec_curr >= temp_sec_D) && (temp_sec_curr < temp_sec_A))		//Departed work, but not at home
				{
					//Set location
					CarInformation.Location = VL_WORK_TO_HOME;

					//Figure out time to next transition
					CarInformation.next_state_change = temp_time_dbl + (temp_sec_A - temp_sec_curr);
				}
				else	//Must be at home
				{
					//Set location
					CarInformation.Location = VL_HOME;

					//Figure out time to next transition
					CarInformation.next_state_change = temp_time_dbl + (temp_sec_B - temp_sec_curr);
				}
			}
		}
		else	//WDepart < WArrive - night shift
		{
			//DW AH DH AW
			if ((temp_sec_curr < temp_sec_D) || (temp_sec_curr >= temp_sec_C))	//At work
			{
				//Set location
				CarInformation.Location = VL_WORK;

				//Figure out time to next transition
				if (temp_sec_curr < temp_sec_D)
				{
					CarInformation.next_state_change = temp_time_dbl + (temp_sec_D - temp_sec_curr);
				}
				else
				{
					CarInformation.next_state_change = temp_time_dbl + (temp_sec_D + 86400.0 - temp_sec_curr);
				}
			}
			else if ((temp_sec_curr >= temp_sec_D) && (temp_sec_curr < temp_sec_A))	//Driving to home
			{
				//Set location
				CarInformation.Location = VL_WORK_TO_HOME;

				//Figure out time to next transition
				CarInformation.next_state_change = temp_time_dbl + (temp_sec_A - temp_sec_curr);
			}
			else if ((temp_sec_curr >= temp_sec_A) && (temp_sec_curr < temp_sec_B))		//At home
			{
				//Set location
				CarInformation.Location = VL_HOME;

				//Figure out time to next transition
				CarInformation.next_state_change = temp_time_dbl + (temp_sec_B - temp_sec_curr);
			}
			else	//Must be at leaving home and driving to work
			{
				//Set location
				CarInformation.Location = VL_HOME_TO_WORK;

				//Figure out time to next transition
				CarInformation.next_state_change = temp_time_dbl + (temp_sec_C - temp_sec_curr);
			}
		}
	}
	else	//Leftover case - at home initially
	{
		//DH AW DW AH;		
		if ((temp_sec_curr < temp_sec_B) || (temp_sec_curr >= temp_sec_A))	//At home
		{
			//Set location
			CarInformation.Location = VL_HOME;

			//Figure out time to next transition
			if (temp_sec_curr < temp_sec_B)
			{
				CarInformation.next_state_change = temp_time_dbl + (temp_sec_B - temp_sec_curr);
			}
			else
			{
				CarInformation.next_state_change = temp_time_dbl + (temp_sec_B + 86400.0 - temp_sec_curr);
			}
		}
		else if ((temp_sec_curr >= temp_sec_B) && (temp_sec_curr < temp_sec_C))	//Departed home, but not arrived at work
		{
			//Set location
			CarInformation.Location = VL_HOME_TO_WORK;

			//Figure out time to next transition
			CarInformation.next_state_change = temp_time_dbl + (temp_sec_C - temp_sec_curr);
		}
		else if ((temp_sec_curr >= temp_sec_C) && (temp_sec_curr < temp_sec_D))		//At work
		{
			//Set location
			CarInformation.Location = VL_WORK;

			//Figure out time to next transition
			CarInformation.next_state_change = temp_time_dbl + (temp_sec_D - temp_sec_curr);
		}
		else	//Must be driving home from work
		{
			//Set location
			CarInformation.Location = VL_WORK_TO_HOME;

			//Figure out time to next transition
			CarInformation.next_state_change = temp_time_dbl + (temp_sec_A - temp_sec_curr);
		}
	}

	//Set up battery size
	if (CarInformation.battery_size <= 0.0)
	{
		if ((mileage_classification != 0.0) && (CarInformation.mileage_efficiency != 0.0))
		{
			CarInformation.battery_size = mileage_classification / CarInformation.mileage_efficiency;
		}
		else
		{
			GL_THROW("Battery size is not specified, nor are the mileage classification or efficiency!");
			/*  TROUBLESHOOT
			The battery size, mileage classification, and efficiency are all undefined.  At least two of these
			need to be defined to properly initialize the object.
			*/
		}
	}
	else
	{
		//Battery size non-zero (and set)
		//Populate efficiency or mileage
		if (mileage_classification != 0.0)
		{
			CarInformation.mileage_efficiency = mileage_classification / CarInformation.battery_size;
		}
		else	//Mileage classification is zero
		{
			if (CarInformation.mileage_efficiency != 0.0)
			{
				mileage_classification = CarInformation.battery_size * CarInformation.mileage_efficiency;
			}
			else
			{
				GL_THROW("Mileage efficiency not specified, more information needed!");
				/*  TROUBLESHOOT
				A battery size was specified, but not a mileage classification or efficiency value.  One of these
				values is necessary to properly initialize the object.
				*/
			}
		}
	}

	//Determine state of charge - assume starts at 100% SOC at home initially, so adjust accordingly
	if ((CarInformation.battery_capacity < 0.0) && (CarInformation.battery_SOC >= 0.0))
	{
		//Populate current capacity based on SOC
		CarInformation.battery_capacity = CarInformation.battery_size * CarInformation.battery_SOC / 100.0;
	}

	//Check vice-versa
	if ((CarInformation.battery_SOC < 0.0) && (CarInformation.battery_capacity >= 0.0))
	{
		//Populate SOC
		CarInformation.battery_SOC = CarInformation.battery_capacity / CarInformation.battery_size * 100.0;
	}
	
	//Should be set, if was specified, otherwise, give us an init
	if ((CarInformation.battery_SOC < 0.0) || (CarInformation.battery_capacity < 0.0))
	{
		if (CarInformation.Location == VL_HOME)
		{
			//Do a proportional state of charge
			CarInformation.battery_SOC = (1.0 - ((CarInformation.next_state_change - temp_time_dbl) / CarInformation.HomeDuration)) * 100.0;

			//Make sure we didn't somehow go over/under
			if ((CarInformation.battery_SOC > 100.0) || (CarInformation.battery_SOC < 0.0))
			{
				//Just default to 50%
				CarInformation.battery_SOC = 50.0;

				gl_warning("Battery SOC calculation messed up and somehow went outsize 0 - 100 range!");
				/*  TROUBLESHOOT
				The initial battery SOC somehow ended up outside a 0 - 100% range.  It has been forced to 50%.
				*/
			}
			
			//Update capacity
			CarInformation.battery_capacity = CarInformation.battery_size * CarInformation.battery_SOC / 100.0;
		}
		else if ((CarInformation.Location == VL_HOME_TO_WORK) || (CarInformation.Location == VL_WORK))	//If at work or going there, deduct a trip
		{
			//Assume started full - deduct our trip
			CarInformation.battery_capacity = CarInformation.battery_size - CarInformation.travel_distance / 2.0 / CarInformation.mileage_efficiency;

			//Make sure didn't go too low
			if ((CarInformation.battery_capacity < 0.0) || (CarInformation.battery_capacity > CarInformation.battery_size))
			{
				//Half discharge us, just cause
				CarInformation.battery_capacity = CarInformation.battery_size / 2.0;

				gl_warning("Battery attempted to discharge too far!");
				/*  TROUBLESHOOT
				While attepting to initialize the battery, it encountered a negative or overloaded
				SOC condition.  It has been arbitrarily set to 50%.
				*/
			}

			//Update SOC
			CarInformation.battery_SOC = CarInformation.battery_capacity / CarInformation.battery_size * 100.0;
		}
		else	//Must be work to home - deduct full trip
		{
			//Assume started full - deduct our trip
			CarInformation.battery_capacity = CarInformation.battery_size - CarInformation.travel_distance / CarInformation.mileage_efficiency;

			//Make sure didn't go too low
			if ((CarInformation.battery_capacity < 0.0) || (CarInformation.battery_capacity > CarInformation.battery_size))
			{
				//If too low, fully discharge
				CarInformation.battery_capacity = 0.0;

				gl_warning("Battery attempted to discharge too far!");
				/*  TROUBLESHOOT
				While attepting to initialize the battery, it encountered a negative or overloaded
				SOC condition.  It has been arbitrarily set to 50%.
				*/
			}

			//Update SOC
			CarInformation.battery_SOC = CarInformation.battery_capacity / CarInformation.battery_size * 100.0;
		}
	}

	//Check efficiency
	if ((CarInformation.ChargeEfficiency<0) || (CarInformation.ChargeEfficiency > 1.0))
	{
		GL_THROW("Charger efficiency is outside of practical bounds!");
		/*  TROUBLESHOOT
		The charger can only be specified between 0% (0.0) and 100% (1.0) efficiency.
		Please limit the input to this range.
		*/
	}

	//"Cheating" function for compatibility with older models -- enables all residential models to be deltamode-ready
	if ((all_residential_delta == true) && (enable_subsecond_models==true))
	{
		hdr->flags |= OF_DELTAMODE;
	}

	//Set the deltamode flag, if desired
	if ((hdr->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true;	//Set the flag and off we go
	}

	//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (enable_subsecond_models!=true)
		{
			gl_warning("evcharger_det:%s indicates it wants to run deltamode, but the module-level flag is not set!",hdr->name?hdr->name:"unnamed");
			/*  TROUBLESHOOT
			The evcharger_det object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The evcharger_det
			will not simulate any dynamics this way.
			*/

			//Deflag us
			deltamode_inclusive = false;
		}
		else
		{
			res_object_count++;	//Increment the counter
		}
	}//End deltamode inclusive
	else	//Not enabled for this model
	{
		if (enable_subsecond_models == true)
		{
			gl_warning("evcharger_det:%d %s - Deltamode is enabled for the module, but not this house!",hdr->id,(hdr->name ? hdr->name : "Unnamed"));
			/*  TROUBLESHOOT
			The evcharger_det is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this evcharger_det may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
	}

	return init_res;
}

int evcharger_det::isa(char *classname)
{
	return (strcmp(classname,"evcharger_det")==0 || residential_enduse::isa(classname));
}

TIMESTAMP evcharger_det::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2, tret;
	double tret_dbl, t_J2894_ret_dbl, t1_dbl, tdiff_dbl;
	bool min_timestep_floored;

	//First run allocation - handles overall residential allocation as well
	if (deltamode_inclusive == true)	//Only call if deltamode is even enabled
	{
		//Overall call -- only do this on the first run
		if (deltamode_registered == false)
		{
			if ((res_object_current == -1) && (delta_objects==nullptr) && (enable_subsecond_models==true))
			{
				//Call the allocation routine
				allocate_deltamode_arrays();
			}

			//Check limits of the array
			if (res_object_current>=res_object_count)
			{
				GL_THROW("Too many objects tried to populate deltamode objects array in the residential module!");
				/*  TROUBLESHOOT
				While attempting to populate a reference array of deltamode-enabled objects for the residential
				module, an attempt was made to write beyond the allocated array space.  Please try again.  If the
				error persists, please submit a bug report and your code via the trac website.
				*/
			}

			//Add us into the list
			delta_objects[res_object_current] = obj;

			//Map up the function for interupdate
			delta_functions[res_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"interupdate_res_object"));

			//Make sure it worked
			if (delta_functions[res_object_current] == nullptr)
			{
				GL_THROW("Failure to map deltamode function for device:%s",obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			// Populate post-delta
			post_delta_functions[res_object_current] = nullptr;

			//Update pointer
			res_object_current++;

			//Deflag us
			deltamode_registered = true;

		}//End first timestep runs
	}//End Deltamode code

	//Get some common time variables, since we'll need them
	t1_dbl = (double)t1;
	tdiff_dbl = t1_dbl - prev_time_dbl;

	//CHeck J2894 (if appropriate) - see if we synced here because we were getting ready to fail
	if (enable_J2894_checks == true)
	{
		//See if we're affected by anything - deltamode scheduling done in postsync
		t_J2894_ret_dbl = check_J2894_values(tdiff_dbl,glob_min_timestep_dbl,&min_timestep_floored);

		//See if we went below and are deltamode capable
		if ((deltamode_inclusive == true) && (min_timestep_floored == true))
		{
			schedule_deltamode_start(t1);
		}

		//Do some initial checks to set a return value
		if (t_J2894_ret_dbl < 0.0)	//Inifinite, so no check
		{
			t_J2894_ret_dbl = TS_NEVER_DBL;
		}
		else if (t_J2894_ret_dbl == 0.0)	//Trip now!
		{
			t_J2894_ret_dbl = t1_dbl;
		}
		else	//Must be a number bigger than zero
		{
			t_J2894_ret_dbl += t1_dbl;
		}
	}
	else	//Not enabled
	{
		t_J2894_ret_dbl = TS_NEVER_DBL;	//Just return never
	}

	//Pull the next state transition
	tret_dbl = sync_ev_function(t1_dbl);

	//Check see what the proper return time should be
	if ((tret_dbl == TS_NEVER_DBL) && (t_J2894_ret_dbl == TS_NEVER_DBL))
	{
		tret = TS_NEVER;
	}
	else if ((tret_dbl == TS_NEVER_DBL) && (t_J2894_ret_dbl != TS_NEVER_DBL))
	{
		tret = (TIMESTAMP)t_J2894_ret_dbl;
	}
	else if ((tret_dbl != TS_NEVER_DBL) && (t_J2894_ret_dbl == TS_NEVER_DBL))
	{
		tret = (TIMESTAMP)tret_dbl;
	}
	else	//Both are valid
	{
		if (tret_dbl < t_J2894_ret_dbl)
		{
			tret = (TIMESTAMP)tret_dbl;
		}
		else
		{
			tret = (TIMESTAMP)t_J2894_ret_dbl;
		}
	}

	//Minimum timestep check
	if (off_nominal_time == true)
	{
		//See where our next "expected" time is
		t2 = t1 + glob_min_timestep;
		if (tret < t2)	//tret is less than the next "expected" timestep
			tret = t2;	//Unfortunately, GridLAB-D is "special" and doesn't know how to handle this with min timesteps.  We have to fix it.
	}

	//Residential enduse sync
	t2 = residential_enduse::sync(t0, t1);

	if (tret < t2)
	{
		if (tret == TS_NEVER)
			return tret;
		else
			return -tret;	//Negative for "suggested" step
	}
	else	//t2 less than tret
	{
		if (t2 == TS_NEVER)
			return t2;
		else
			return -t2;
	}
}

TIMESTAMP evcharger_det::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	double ret_value;
	TIMESTAMP increment_value, next_ret;
	bool min_timestep_floored;

	//See if the call needs to happen
	if ((enable_J2894_checks == true) && (CarInformation.Location == VL_HOME))
	{
		//Perform the checks - mostly to see if we need an instantaneous fail
		ret_value = check_J2894_values(0.0,glob_min_timestep_dbl,&min_timestep_floored);

		if (ret_value < 0.0)	//Infinite
		{
			return TS_NEVER;
		}
		else if (min_timestep_floored == true) //New operating point is too high/low, or time is too long - trip it now - capture zero too
		{
			if (deltamode_inclusive == true)
			{
				//Schedule deltamode for now!
				schedule_deltamode_start(t1);
			}
			//Default else - it may be coming back, just let it happen next time, or we handled it inside

			//Both methods want a reiteration
			return t1;
		}
		else	//Further in the future
		{
			//Floor the difference
			increment_value = (TIMESTAMP)(floor(ret_value));

			//Compute the update
			next_ret = t1 + increment_value;

			//Return it appropriately
			return -next_ret;
		}
	}
	else
	{
		return TS_NEVER;
	}
}

//Functionalized sync routine - so can be called from deltamode easier
double evcharger_det::sync_ev_function(double curr_time_dbl)
{
	OBJECT *obj = OBJECTHDR(this);
	double temp_double, charge_out_percent, ramp_temp, ramp_time, temp_voltage;
	complex temp_current_value, temp_current_calc;
	gld::complex temp_complex;
	complex actual_power_value;
	double tdiff;
	bool in_deltamode;

	temp_double = 0;

	//See if we're running deltamode or not
	if (deltatimestep_running < 0.0)	//i.e., -1.0
	{
		in_deltamode = false;	//QSTS
	}
	else
	{
		in_deltamode = true;	//deltamode
	}

	//Only perform an action if the time has changed
	if (prev_time_dbl != curr_time_dbl)
	{
		//Get time difference
		tdiff = curr_time_dbl - prev_time_dbl;

		//Zero charge output value
		charge_out_percent = 0.0;

		//Reset ramp constraint flag, just in case
		J2894_is_ramp_constrained = false;

		//Get a voltage value, just because
		temp_voltage = expected_voltage_base * load.voltage_factor;

		//Dependent on our location, figure out what to do
		switch (CarInformation.Location)
		{
			case VL_HOME:
				//See if we are still at home - assume any J2894 check is bypassed - since it is in past
				if (curr_time_dbl >= CarInformation.next_state_change)	//Nope
				{
					//Transition to going to work
					CarInformation.Location = VL_HOME_TO_WORK;

					//Update "accumulator" time
					tdiff = CarInformation.next_state_change - prev_time_dbl;

					//Compute the realized charge rate
					if (load.voltage_factor > 0.0)
					{
						actual_power_value = load.power + (load.current + load.admittance * load.voltage_factor)* load.voltage_factor;
					}
					else
					{
						actual_power_value = complex(0.0,0.0);
					}

					//Push the update in
					RealizedChargeRate = actual_power_value.Re();

					//Update battery information first - just in case the transition occurred in the middle of a timestep
					temp_double = tdiff / 3600.0 * RealizedChargeRate * CarInformation.ChargeEfficiency;	//Convert to kWh
					
					//Accumulate it
					CarInformation.battery_capacity += temp_double;

					//Make sure it didn't overrun
					if (CarInformation.battery_capacity < 0.0)	//Below empty!?
					{
						CarInformation.battery_capacity = 0.0;
					}
					else if (CarInformation.battery_capacity > CarInformation.battery_size)	//Over 100%!?!
					{
						CarInformation.battery_capacity = CarInformation.battery_size;	//Set to 100%
					}

					//Discharge the battery according to half our distance - +/- variation, if desired
					if ((variation_trip_std_dev > 0.0) || (variation_trip_mean != 0.0))	//Some variation
					{
						//Get the variation
						temp_double = gl_random_normal(RNGSTATE,variation_trip_mean,variation_trip_std_dev);

						//Add in the normal value
						temp_double += CarInformation.travel_distance / 2.0;

						//Make sure it didn't go negative - if so, just use default
						if (temp_double < 0)
						{
							temp_double = CarInformation.travel_distance / 2.0;
						}

						//Calculate and remove the energy consumed
						CarInformation.battery_capacity = CarInformation.battery_capacity - temp_double / CarInformation.mileage_efficiency;
					}
					else	//Nope
					{
						//Calculate and remove the energy consumed
						CarInformation.battery_capacity = CarInformation.battery_capacity - CarInformation.travel_distance / 2.0 / CarInformation.mileage_efficiency;
					}
					
					//Make sure the battery didn't go too low
					if (CarInformation.battery_capacity < 0.0)
					{
						//Force us to zero, if so
						CarInformation.battery_capacity = 0.0;
					}
					else if (CarInformation.battery_capacity > CarInformation.battery_size)	//Pointless check, but let's make sure we'd did go above 100% somehow
					{
						CarInformation.battery_capacity = CarInformation.battery_size;	//100%
					}

					//Normal, update SOC
					CarInformation.battery_SOC = CarInformation.battery_capacity / CarInformation.battery_size * 100.0;

					//Update our state change time
					if ((variation_std_dev > 0.0) || (variation_mean != 0.0))	//Some shiftiness?
					{
						//Get the variation
						temp_double = gl_random_normal(RNGSTATE,variation_mean,variation_std_dev) + CarInformation.HomeWorkDuration;

						//Make sure it didn't go negative - if so, just use default
						if (temp_double < 0)
						{
							temp_double = CarInformation.HomeWorkDuration;
						}

						//Add it in
						CarInformation.next_state_change = curr_time_dbl + temp_double;
					}
					else	//Nope
					{
						//Add in how long it takes us to drive
						CarInformation.next_state_change = curr_time_dbl + CarInformation.HomeWorkDuration;
					}

					//Zero our output - power already should be
					DesiredChargeRate = 0.0;
					ActualChargeRate = 0.0;
					RealizedChargeRate = 0.0;

					//Zero all tracking accumulators and make sure we're enabled, just in case
					J2894_off_accumulator = 0.0;
					J2894_voltage_high_accumulators[0] = J2894_voltage_high_accumulators[1] = 0.0;
					J2894_voltage_low_accumulators[0] = J2894_voltage_low_accumulators[1] = 0.0;
					J2894_voltage_high_state[0] = J2894_voltage_high_state[1] = false;
					J2894_voltage_low_state[0] = J2894_voltage_low_state[1] = false;
					ev_charger_enabled_state = true;
				}
				else	//Yes
				{
					//Reaffirm where we are
					CarInformation.Location = VL_HOME;

					//Compute the realized charge rate
					if (load.voltage_factor > 0.0)
					{
						actual_power_value = load.power + (load.current + load.admittance * load.voltage_factor)* load.voltage_factor;
					}
					else
					{
						actual_power_value = complex(0.0,0.0);
					}

					//Push the update in
					RealizedChargeRate = actual_power_value.Re();

					//Update battery capacity with "last time's worth" of charge
					temp_double = tdiff / 3600.0 * RealizedChargeRate * CarInformation.ChargeEfficiency;	//Convert to kWh

					//Accumulate it
					CarInformation.battery_capacity += temp_double;

					//Make sure it didn't overrun
					if (CarInformation.battery_capacity < 0.0)	//Below empty!?
					{
						CarInformation.battery_capacity = 0.0;
					}
					else if (CarInformation.battery_capacity > CarInformation.battery_size)	//Over 100%!?!
					{
						CarInformation.battery_capacity = CarInformation.battery_size;	//Set to 100%
					}

					//Update SOC
					CarInformation.battery_SOC = CarInformation.battery_capacity / CarInformation.battery_size * 100.0;

					//Determine charge rate
					if (CarInformation.battery_SOC < 100.0)
					{
						charge_out_percent = 100;	//Full charging
					}
					else	//Full, no more charging
					{
						charge_out_percent = 0.0;
					}

					//"Postliminary" scaling - put variables back in expected forms
					DesiredChargeRate = CarInformation.MaxChargeRate * charge_out_percent / 100.0;

					//Check and see if we need to check that
					if ((enable_J2894_checks == true) && (J2894_ramp_limit != 0.0))
					{
						//See if we're even enabled
						if (ev_charger_enabled_state == true)
						{
							//See if we're deltamoded or not
							if (deltatimestep_running < 0.0)	//QSTS
							{
								ramp_time = glob_min_timestep_dbl;
							}
							else	//Deltamode
							{
								ramp_time = deltatimestep_running;
							}

							//See how fast we may be changing
							ramp_temp = (DesiredChargeRate - ActualChargeRate) / temp_voltage / ramp_time;

							//Check for violation
							if (ramp_temp > J2894_ramp_limit)
							{
								//Limit it - convert it back to power
								temp_double = J2894_ramp_limit * ramp_time * temp_voltage;

								//Update
								ActualChargeRate += temp_double;

								//Flag it
								J2894_is_ramp_constrained = true;
							}
							else	//Not ramp constrained, just put it in
							{
								ActualChargeRate = DesiredChargeRate;

								//Flag us as unlimited - should already be true, but be paranoid
								J2894_is_ramp_constrained = false;
							}
						}
						else	//We're off
						{
							//Set output to zero
							ActualChargeRate = 0.0;

							//But we aren't ramp constrained when off!
							J2894_is_ramp_constrained = false;
						}
					}
					else
					{
						//No limits
						ActualChargeRate = DesiredChargeRate;
					}
				}
				break;
			case VL_HOME_TO_WORK:
				//Zero our output - power already should be
				DesiredChargeRate = 0.0;
				ActualChargeRate = 0.0;
				RealizedChargeRate = 0.0;

				//See if we're still driving to work
				if (curr_time_dbl >= CarInformation.next_state_change)	//Nope
				{
					//Transition us to work
					CarInformation.Location = VL_WORK;

					//If charging is available at work, make that adjustment here (independent of house load, so who cares)
					if (Work_Charge_Available == true)
					{
						//Convert our time here to hours
						temp_double = CarInformation.WorkDuration / 3600.0;

						//Scale by our "normal" charge rate
						temp_double *= CarInformation.MaxChargeRate;

						//Now add this into the battery
						CarInformation.battery_capacity += temp_double;

						//Sanity check
						if (CarInformation.battery_capacity < 0.0)	//Less than zero
						{
							CarInformation.battery_capacity = 0.0;	//Set to 0
						}
						else if (CarInformation.battery_capacity > CarInformation.battery_size)	//Somehow got over 100%
						{
							CarInformation.battery_capacity = CarInformation.battery_size;	//Set to 100%
						}

						//Update SOC
						CarInformation.battery_SOC = CarInformation.battery_capacity / CarInformation.battery_size * 100.0;
					}

					//Update our state change time
					if ((variation_std_dev > 0.0) || (variation_mean != 0.0))	//Some shiftiness?
					{
						//Get the variation
						temp_double = gl_random_normal(RNGSTATE,variation_mean,variation_std_dev) + CarInformation.WorkDuration;

						//Make sure it didn't go negative - if so, just use default
						if (temp_double < 0)
						{
							temp_double = CarInformation.WorkDuration;
						}

						//Add it in
						CarInformation.next_state_change = curr_time_dbl + temp_double;
					}
					else	//Nope
					{
						//Add in how long we are at work
						CarInformation.next_state_change = curr_time_dbl + CarInformation.WorkDuration;
					}
				}
				else	//Yep, reaffirm our location
				{
					CarInformation.Location = VL_HOME_TO_WORK;
				}
				break;
			case VL_WORK:
				//Zero our output - power already should be
				DesiredChargeRate = 0.0;
				ActualChargeRate = 0.0;
				RealizedChargeRate = 0.0;

				//See if we are still at work
				if (curr_time_dbl >= CarInformation.next_state_change)	//Nope
				{
					//Transition to going to work
					CarInformation.Location = VL_WORK_TO_HOME;

					//Discharge the battery according to half our distance - +/- variation, if desired
					if ((variation_trip_std_dev > 0.0) || (variation_trip_mean != 0.0))	//Some variation
					{
						//Get the variation
						temp_double = gl_random_normal(RNGSTATE,variation_trip_mean,variation_trip_std_dev);

						//Add in the normal value
						temp_double += CarInformation.travel_distance / 2.0;

						//Make sure it didn't go negative - if so, just use default
						if (temp_double < 0)
						{
							temp_double = CarInformation.travel_distance / 2.0;
						}

						//Calculate and remove the energy consumed
						CarInformation.battery_capacity = CarInformation.battery_capacity - temp_double / CarInformation.mileage_efficiency;
					}
					else	//Nope
					{
						//Calculate and remove the energy consumed
						CarInformation.battery_capacity = CarInformation.battery_capacity - CarInformation.travel_distance / 2.0 / CarInformation.mileage_efficiency;
					}
					
					//Make sure the battery didn't go too low
					if (CarInformation.battery_capacity < 0.0)
					{
						//Force us to zero, if so
						CarInformation.battery_capacity = 0.0;
					}
					else if (CarInformation.battery_capacity > CarInformation.battery_size)	//Pointless check, but let's make sure we'd did go above 100% somehow
					{
						CarInformation.battery_capacity = CarInformation.battery_size;	//100%
					}

					//update SOC
					CarInformation.battery_SOC = CarInformation.battery_capacity / CarInformation.battery_size * 100.0;

					//Update our state change time
					if ((variation_std_dev > 0.0) || (variation_mean != 0.0))	//Some shiftiness?
					{
						//Get the variation
						temp_double = gl_random_normal(RNGSTATE,variation_mean,variation_std_dev) + CarInformation.WorkHomeDuration;

						//Make sure it didn't go negative - if so, just use default
						if (temp_double < 0)
						{
							temp_double = CarInformation.WorkHomeDuration;
						}

						//Add it in
						CarInformation.next_state_change = curr_time_dbl + temp_double;
					}
					else	//Nope
					{
						//Add in how long it takes us to drive
						CarInformation.next_state_change = curr_time_dbl + CarInformation.WorkHomeDuration;
					}
				}
				else	//Yes
				{
					//Just reaffirm our location
					CarInformation.Location = VL_WORK;
				}
				break;
			case VL_WORK_TO_HOME:
				//Zero our output - power already should be
				DesiredChargeRate = 0.0;
				ActualChargeRate = 0.0;

				//See if we're still driving home
				if (curr_time_dbl >= CarInformation.next_state_change)	//Nope
				{
					//Transition us to home
					CarInformation.Location = VL_HOME;

					//Update charging for this brief stint
					//Begin by calculating our "apparent charge time"
					tdiff = curr_time_dbl - CarInformation.next_state_change;

					//Compute the realized charge rate
					if (load.voltage_factor > 0.0)
					{
						actual_power_value = load.power + (load.current + load.admittance * load.voltage_factor)* load.voltage_factor;
					}
					else
					{
						actual_power_value = complex(0.0,0.0);
					}

					//Push the update in
					RealizedChargeRate = actual_power_value.Re();

					//Update battery capacity with "last time's worth" of charge
					temp_double = tdiff / 3600.0 * RealizedChargeRate * CarInformation.ChargeEfficiency;	//Convert to kWh

					//Accumulate it
					CarInformation.battery_capacity += temp_double;

					//Make sure it didn't overrun
					if (CarInformation.battery_capacity < 0.0)	//Below empty!?
					{
						CarInformation.battery_capacity = 0.0;
					}
					else if (CarInformation.battery_capacity > CarInformation.battery_size)	//Over 100%!?!
					{
						CarInformation.battery_capacity = CarInformation.battery_size;	//Set to 100%
					}

					//Update SOC
					CarInformation.battery_SOC = CarInformation.battery_capacity / CarInformation.battery_size * 100.0;

					//Determine charge rate
					if (CarInformation.battery_SOC < 100.0)
					{
						charge_out_percent = 100;	//Full charging
					}
					else	//Full, no more charging
					{
						charge_out_percent = 0.0;
					}

					//"Postliminary" scaling - put variables back in expected forms
					DesiredChargeRate = CarInformation.MaxChargeRate * charge_out_percent / 100.0;

					//Check to see if we should be ramped limited
					if ((enable_J2894_checks == true) && (J2894_ramp_limit != 0.0))
					{
						//See if we're even enabled
						if (ev_charger_enabled_state == true)
						{
							//See if we're deltamoded or not
							if (deltatimestep_running < 0.0)	//QSTS
							{
								ramp_time = glob_min_timestep_dbl;
							}
							else	//Deltamode
							{
								ramp_time = deltatimestep_running;
							}

							//See how fast we may be changing
							ramp_temp = (DesiredChargeRate - ActualChargeRate) / temp_voltage / ramp_time;

							//Check for violation
							if (ramp_temp > J2894_ramp_limit)
							{
								//Limit it - convert it back to power
								temp_double = J2894_ramp_limit * ramp_time * temp_voltage;

								//Update
								ActualChargeRate += temp_double;

								//Flag it
								J2894_is_ramp_constrained = true;
							}
							else	//Not ramp constrained, just put it in
							{
								ActualChargeRate = DesiredChargeRate;

								//Flag us as unlimited - should already be true, but be paranoid
								J2894_is_ramp_constrained = false;
							}
						}
						else	//We're off, so don't let it do anything
						{
							//Off
							ActualChargeRate = 0.0;

							//Not constrain though!
							J2894_is_ramp_constrained = false;
						}
					}
					else
					{
						//No limit
						ActualChargeRate = DesiredChargeRate;
					}

					//Update our state change time
					if ((variation_std_dev > 0.0) || (variation_mean != 0.0))	//Some shiftiness?
					{
						//Get the variation
						temp_double = gl_random_normal(RNGSTATE,variation_mean,variation_std_dev) + CarInformation.HomeDuration;

						//Make sure it didn't go negative - if so, just use default
						if (temp_double < 0)
						{
							temp_double = CarInformation.HomeDuration;
						}

						//Add it in
						CarInformation.next_state_change = curr_time_dbl + temp_double;
					}
					else	//Nope
					{
						//Add in how long we are home
						CarInformation.next_state_change = curr_time_dbl + CarInformation.HomeDuration;
					}
				}
				else	//Yep, reaffirm our location
				{
					CarInformation.Location = VL_WORK_TO_HOME;
				}
				break;
			case VL_UNKNOWN:
			default:
				GL_THROW("Vehicle is at an unknown location!");
				/*  TROUBLESHOOT
				The vehicle somehow is in an unknown location.  Please try again.  If the error persists,
				please submit your GLM and a bug report via the Trac website.
				*/
		}

		//Update the pointer
		prev_time_dbl = curr_time_dbl;
	}

	//Determine the VA amount
	temp_complex.SetReal(ActualChargeRate / 1000.0);	//Set real in kW

	//Set complex
	if (load.power_factor < 0.0)	//Capacitive
	{
		temp_complex.SetImag(-1.0 * temp_complex.Re() / load.power_factor * sin( acos(fabs(load.power_factor))));
	}
	else	//0 or inductive
	{
		temp_complex.SetImag(temp_complex.Re() / load.power_factor * sin( acos(fabs(load.power_factor))));
	}

	//See if it will exceed the maximum pu
	temp_current_value = ~(temp_complex / complex((expected_voltage_base * load.voltage_factor),0.0));

	//Basing it off the power, since close enough
	if (temp_current_value.Mag() > max_overload_charge_current)
	{
		temp_current_calc.SetPolar(max_overload_charge_current,temp_current_value.Arg());

		//Limit it
		temp_complex = complex(expected_voltage_base * load.voltage_factor,0.0) * ~temp_current_calc;
	}
	//Default else - smaller, so okay

	//Set the total power
	load.total = temp_complex;

	//Split it out by component
	if (temp_complex.Mag() > 0.0)
	{
		//Loads are already in kVA
		load.power = load.total * load.power_fraction;
		load.current = load.total * load.current_fraction;
		load.admittance = load.total * load.impedance_fraction;
	}
	else	//Just zero it all
	{
		load.power = complex(0.0,0.0);
		load.current = complex(0.0,0.0);
		load.admittance = complex(0.0,0.0);
	}

	//See if we're in deltamode and ramp-limited
	if ((in_deltamode == true) && (J2894_is_ramp_constrained == true))
	{
		//Just set it to step forward one delta timestep
		return (curr_time_dbl + deltatimestep_running);
	}
	else if (J2894_is_ramp_constrained == true)	//Ramp constrained, but not in deltamode
	{
		//See if we're off-nominal time, for giggles
		if (off_nominal_time == true)
		{
			//Minimum timesteps bigger than zero, so just don't care
			return CarInformation.next_state_change;
		}
		else
		{
			//Just move us forward one second and re-evaluate
			return (curr_time_dbl + (double)TS_SECOND);
		}
	}
	else	//Not ramp limited - so QSTS or lower than limit
	{
		//Pull the next state transition
		return CarInformation.next_state_change;
	}
}

//Function to check the suggested operating realms for J2894/ANSI C84.1
//Return state is time to trip (negative means infinite - no violations)
//floored_trip flag means it got rounded down - basically to indicate if we should do deltamode instead (if relevant)
double evcharger_det::check_J2894_values(double diff_time, double tstep_value, bool *floored_trip)
{
	double temp_time, temp_diff_time;
	double J2894_high_time_to_change_vals[2];
	double J2894_low_time_to_change_vals[2];
	double J2894_off_time_to_change;
	char idx;
	bool deltamode_exception;

	//Initialize the time tracker to "big value"
	temp_time = 99999999.0;

	//Set the output trip to be "standard" for first try
	*floored_trip = false;

	//Set up an "exception" variable
	if ((deltamode_inclusive == true) && (deltatimestep_running < 0.0))	//See if we are delta capable, but not in deltamode
	{
		deltamode_exception = true;	//Allow us to not trip (schedule deltamode)
	}
	else
	{
		deltamode_exception = false;	//Nope, operate normally
	}

	//Initialize the timing accumulator
	J2894_high_time_to_change_vals[0] = J2894_high_time_to_change_vals[1] = 0.0;
	J2894_low_time_to_change_vals[0] = J2894_low_time_to_change_vals[1] = 0.0;
	J2894_off_time_to_change = 0.0;

	//See if we're even enabled - if not, skip this part
	if (ev_charger_enabled_state == true)
	{
		//Zero the outage accumulator - we're clearly not in an outage
		J2894_off_accumulator = 0.0;

		//See if we're high - and outside ANSI
		if (load.voltage_factor > 1.1)
		{
			//Loop through the high values
			if (load.voltage_factor > J2894_voltage_high_threshold_values[0][0])	//Above even the first one, just trip
			{
				J2894_trip_method = J2894_SURGE_OVER_VOLT;
				ev_charger_enabled_state = false;
				return 0.0;
			}
			//Default else, must be okay

			//Top tier
			if ((load.voltage_factor > J2894_voltage_high_threshold_values[1][0]) && (load.voltage_factor <= J2894_voltage_high_threshold_values[0][0]))
			{
				J2894_voltage_high_accumulators[0] += diff_time;
				J2894_voltage_high_state[0] = true;
				J2894_high_time_to_change_vals[0] = J2894_voltage_high_threshold_values[0][1] - J2894_voltage_high_accumulators[0];
			}
			else
			{
				J2894_voltage_high_accumulators[0] = 0.0;	//Reset the accumulator
				J2894_voltage_high_state[0] = false;
				J2894_high_time_to_change_vals[0] = 99999999.0;
			}

			//Second tier - first is implied by overall if, but good to double check
			if ((load.voltage_factor > 1.1) && (load.voltage_factor <= J2894_voltage_high_threshold_values[1][0]))
			{
				J2894_voltage_high_accumulators[1] += diff_time;
				J2894_voltage_high_state[1] = true;
				J2894_high_time_to_change_vals[1] = J2894_voltage_high_threshold_values[1][1] - J2894_voltage_high_accumulators[1];
			}
			else
			{
				J2894_voltage_high_accumulators[1] = 0.0;	//Reset accumulator
				J2894_voltage_high_state[1] = false;
				J2894_high_time_to_change_vals[1] = 99999999.0;
			}

			//Zero the low thresholds - by definition, they can't apply here
			J2894_voltage_low_accumulators[0] = J2894_voltage_low_accumulators[1] = 0.0;
			J2894_voltage_low_state[0] = J2894_voltage_low_state[1] = false;

			//Check the high to see if we've violated something (or will before the next step)
			if ((J2894_voltage_high_accumulators[0] >= J2894_voltage_high_threshold_values[0][1]) || (J2894_high_time_to_change_vals[0] < tstep_value))
			{
				//See if it was a minimum timestep trip
				if ((J2894_high_time_to_change_vals[0] < tstep_value) && (J2894_high_time_to_change_vals[0] > 0.0) && (deltamode_exception == true))
				{
					//Flag it
					*floored_trip = true;

					//Don't do anything else - deltamode should handle
				}
				else	//Criteria not met - trip it
				{
					J2894_trip_method = J2894_HIGH_OVER_VOLT;
					ev_charger_enabled_state = false;
					return 0.0;
				}
			}

			if ((J2894_voltage_high_accumulators[1] >= J2894_voltage_high_threshold_values[1][1]) || (J2894_high_time_to_change_vals[1] < tstep_value))
			{
				//See if it was a minimum timestep trip
				if ((J2894_high_time_to_change_vals[1] < tstep_value) && (J2894_high_time_to_change_vals[1] > 0.0) && (deltamode_exception == true))
				{
					//Flag it
					*floored_trip = true;

					//Don't do anything else - deltamode should handle
				}
				else	//Criteria not met - trip it
				{
					J2894_trip_method = J2894_OVER_VOLT;
					ev_charger_enabled_state = false;
					return 0.0;
				}
			}

			//If we made it this far, we're in a violation, but ticking time - see how long we have left
			//Loop - only two for now, but meh
			for (idx=0; idx<2; idx++)
			{
				if (J2894_voltage_high_state[idx] == true)
				{
					temp_diff_time = J2894_high_time_to_change_vals[idx];
				}
				else
				{
					temp_diff_time = 99999999.0;	//Arbitarily big value
				}

				//Check it
				if (temp_diff_time < temp_time)
				{
					temp_time = temp_diff_time;
				}
			}

			//Return us
			return temp_time;
		}
		else if (load.voltage_factor < 0.9)	//Below ANSI
		{
			if (load.voltage_factor < J2894_voltage_low_threshold_values[0][0])	//Outside lower limit - trip (though this is zero, by default J2894)
			{
				J2894_trip_method = J2894_EXTREME_UNDER_VOLT;
				ev_charger_enabled_state = false;
				return 0.0;
			}

			//Check the lowest tier
			if ((load.voltage_factor >= J2894_voltage_low_threshold_values[0][0]) && (load.voltage_factor < J2894_voltage_low_threshold_values[1][0]))
			{
				J2894_voltage_low_accumulators[0] += diff_time;
				J2894_voltage_low_state[0] = true;
				J2894_low_time_to_change_vals[0] = J2894_voltage_low_threshold_values[0][1] - J2894_voltage_low_accumulators[0];
			}
			else
			{
				J2894_voltage_low_accumulators[0] = 0.0;	//Reset accumulator
				J2894_voltage_low_state[0] = false;
				J2894_low_time_to_change_vals[0] = 99999999.0;
			}

			//See if we're in the middle tier
			if ((load.voltage_factor >= J2894_voltage_low_threshold_values[1][0]) && (load.voltage_factor < 0.9))
			{
				J2894_voltage_low_accumulators[1] += diff_time;
				J2894_voltage_low_state[1] = true;
				J2894_low_time_to_change_vals[1] = J2894_voltage_low_threshold_values[1][1] - J2894_voltage_low_accumulators[1];
			}
			else
			{
				J2894_voltage_low_accumulators[1] = 0.0;	//Reset accumulator
				J2894_voltage_low_state[1] = false;
				J2894_low_time_to_change_vals[1] = 99999999.0;
			}

			//Zero the high thresholds - by definition, they can't apply here
			J2894_voltage_high_accumulators[0] = J2894_voltage_high_accumulators[1] = 0.0;
			J2894_voltage_high_state[0] = J2894_voltage_high_state[1] = false;

			//Check the low to see if we've violated something, or will in the next time
			if ((J2894_voltage_low_accumulators[0] >= J2894_voltage_low_threshold_values[0][1]) || (J2894_low_time_to_change_vals[0] < tstep_value))
			{
				//See if it was a minimum timestep trip
				if ((J2894_low_time_to_change_vals[0] < tstep_value) && (J2894_low_time_to_change_vals[0] > 0.0) && (deltamode_exception == true))
				{
					//Flag it
					*floored_trip = true;

					//Nothing else - deltamode call should get it
				}
				else	//Criteria not met, just trip it
				{
					J2894_trip_method = J2894_LOW_UNDER_VOLT;
					ev_charger_enabled_state = false;
					return 0.0;
				}
			}

			if ((J2894_voltage_low_accumulators[1] >= J2894_voltage_low_threshold_values[1][1]) || (J2894_low_time_to_change_vals[1] < tstep_value))
			{
				//See if it was a minimum timestep trip
				if ((J2894_low_time_to_change_vals[1] < tstep_value) && (J2894_low_time_to_change_vals[1] > 0.0) && (deltamode_exception == true))
				{
					//Flag the flooring
					*floored_trip = true;

					//Nothing else -- maintain
				}
				else	//Let it go
				{
					J2894_trip_method = J2894_UNDER_VOLT;
					ev_charger_enabled_state = false;
					return 0.0;
				}
			}

			//If we made it this far, we're in a violation, but ticking time - see how long we have left
			//Loop - only two for now, but meh
			for (idx=0; idx<2; idx++)
			{
				if (J2894_voltage_low_state[idx] == true)
				{
					temp_diff_time = J2894_low_time_to_change_vals[idx];
				}
				else
				{
					temp_diff_time = 99999999.0;	//Arbitarily big value
				}

				//Check it
				if (temp_diff_time < temp_time)
				{
					temp_time = temp_diff_time;
				}
			}

			//Return us
			return temp_time;
		}
		else
		{
			//Re-iterate that we're enabled (redundant, but meh)
			ev_charger_enabled_state = true;

			//Zero all of the tracking accumulators
			J2894_voltage_high_accumulators[0] = J2894_voltage_high_accumulators[1] = 0.0;
			J2894_voltage_low_accumulators[0] = J2894_voltage_low_accumulators[1] = 0.0;
			J2894_voltage_high_state[0] = J2894_voltage_high_state[1] = false;
			J2894_voltage_low_state[0] = J2894_voltage_low_state[1] = false;
			J2894_trip_method = J2894_NONE;

			//No violations, indicate we're good forever
			return -1.0;
		}
	}//End is enabled
	else
	{
		//Must be false - see if we can reconnect!
		//Update the accumulator
		J2894_off_accumulator += diff_time;
		J2894_off_time_to_change = J2894_off_threshold - J2894_off_accumulator;

		//Zero all the accumulators, just out of "consistency"
		J2894_voltage_high_accumulators[0] = J2894_voltage_high_accumulators[1] = 0.0;
		J2894_voltage_low_accumulators[0] = J2894_voltage_low_accumulators[1] = 0.0;
		J2894_voltage_high_state[0] = J2894_voltage_high_state[1] = false;
		J2894_voltage_low_state[0] = J2894_voltage_low_state[1] = false;

		//Keep our output off too - keeps ramps from trying to go
		ActualChargeRate = 0.0;
		RealizedChargeRate = 0.0;

		//See if we've been off long enough
		if ((J2894_off_accumulator >= J2894_off_threshold) || (J2894_off_time_to_change < tstep_value))
		{
			//See how we're exiting - are we between values?
			if ((J2894_off_time_to_change < tstep_value) && (J2894_off_time_to_change > 0.0) && (deltamode_exception == true))
			{
				//Flag the trip
				*floored_trip = true;

				//Don't reset us though, since deltamode should do this
				//Just reinforce
				ev_charger_enabled_state = false;

				//Return how long we'll take
				return J2894_off_time_to_change;
			}
			else
			{
				//Clear the flag
				J2894_trip_method = J2894_NONE;
				ev_charger_enabled_state = true;
				return -1.0;
			}
		}
		else
		{
			//Just reinforce
			ev_charger_enabled_state = false;

			//Return how long we'll take
			return J2894_off_time_to_change;
		}
	}
}

//Deltamode class function
SIMULATIONMODE evcharger_det::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	double dbl_deltatime, ev_ret_time, diff_time, ret_value, deltat;
	bool min_timestep_floored;

	//Get the current time
	dbl_deltatime = gl_globaldeltaclock;

	//Get the step
	deltat = (double)dt/(double)DT_SECOND;

	//See if checks are needed
	if ((enable_J2894_checks == true) && (CarInformation.Location == VL_HOME))
	{
		//Compute a difference
		diff_time = dbl_deltatime - prev_time_dbl;

		//Perform the checks - mostly to see if we need an instantaneous fail
		ret_value = check_J2894_values(diff_time,deltat,&min_timestep_floored);

		//Call the update - we'll let it go this far, if it tripped (easier to update)
		ev_ret_time = sync_ev_function(dbl_deltatime);

		if ((ret_value > 1.0) || (ret_value < 0.0))	//Time-out is further or never, see where we're at
		{
			//See if we should request a deltamode or not
			diff_time = ev_ret_time - dbl_deltatime;

			//See how far it is
			if (diff_time > 1.0)
			{
				return SM_EVENT;	//Let us go event-driven
			}
			else
			{
				return SM_DELTA;	//Proceed forward
			}
		}
		else	//Must be between (bigger than a timestep, but less than 1) - keep going
		{
			return SM_DELTA;
		}
	}//End J2894 checks
	else	//Standard sync
	{
		//Call the function
		ev_ret_time = sync_ev_function(dbl_deltatime);

		//See if we should request a deltamode or not
		diff_time = ev_ret_time - dbl_deltatime;

		//See how far it is
		if (diff_time > 1.0)
		{
			return SM_EVENT;	//Let us go event-driven
		}
		else
		{
			return SM_DELTA;	//Proceed forward
		}
	}//End no J2894 checks
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_evcharger_det(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(evcharger_det::oclass);

		if (*obj!=nullptr)
		{
			evcharger_det *my = OBJECTDATA(*obj,evcharger_det);
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		else
			return 0;
	}
	CREATE_CATCHALL(evcharger_det);
}

EXPORT int init_evcharger_det(OBJECT *obj)
{
	try {
		evcharger_det *my = OBJECTDATA(obj,evcharger_det);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(evcharger_det);
}

EXPORT int isa_evcharger_det(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,evcharger_det)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_evcharger_det(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	TIMESTAMP t1;

	try {
		evcharger_det *my = OBJECTDATA(obj, evcharger_det);
		t1 = TS_NEVER;
		switch (pass)
		{
		case PC_PRETOPDOWN:
			break;
		case PC_BOTTOMUP:
			t1 = my->sync(obj->clock, t0);
			obj->clock = t0;
			break;
		case PC_POSTTOPDOWN:
			t1 = my->postsync(obj->clock, t0);
			break;
		default:
			gl_error("evcharger_det::sync- invalid pass configuration");
			t1 = TS_INVALID; // serious error in exec.c
		}
		return t1;
	}
	SYNC_CATCHALL(evcharger_det);
}

//Deltamode linkage function
//Deltamode exposed functions
EXPORT SIMULATIONMODE interupdate_evcharger_det(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	evcharger_det *my = OBJECTDATA(obj,evcharger_det);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time,dt,iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_evcharger_det(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

/**@}**/
