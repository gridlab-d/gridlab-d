/** $Id: evcharger_det.cpp
	Copyright (C) 2012 Battelle Memorial Institute
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "evcharger_det.h"

//////////////////////////////////////////////////////////////////////////
// evcharger_det CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* evcharger_det::oclass = NULL;
CLASS* evcharger_det::pclass = NULL;

evcharger_det::evcharger_det(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"evcharger_det",sizeof(evcharger_det),PC_BOTTOMUP);
		if (oclass==NULL)
			throw "unable to register class evcharger_det";

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double,"charge_rate[W]", PADDR(ChargeRate),PT_DESCRIPTION, "Current demanded charge rate of the vehicle",

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
			PT_double,"travel_distance[mile]",PADDR(CarInformation.travel_distance), PT_DESCRIPTION, "Distance vehicle travels from home to home",
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
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

int evcharger_det::create() 
{
	int create_res = residential_enduse::create();

	ChargeRate = 0.0;			//Starts off
	load.power_factor = 0.99;	//Based on Brusa NLG5 specifications
	load.heatgain_fraction = 0.0;	//Assumed to be "in the garage" or outside - so not contributing to internal heat gains

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
	CarInformation.next_state_change = 0;		//Next time the state will change
	CarInformation.travel_distance = 15.0;		//15 mile default, just because
	CarInformation.WorkArrive = 500.0;			//Arrive at work at 5 AM
	CarInformation.WorkDuration = 41400.0;		//Assumed at work for 11.5 hours as well
	CarInformation.WorkHomeDuration = 1800.0;	//Half hour commute home

	prev_time = 0;

	off_nominal_time = false;	//Assumes minimum timesteps aren't screwing things up, by default

	return create_res;
}

int evcharger_det::init(OBJECT *parent)
{
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("evcharger_det::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	OBJECT *hdr = OBJECTHDR(this);
	int TempIdx;
	int init_res;
	int comma_count, curr_idx, curr_comma_count;
	char temp_char;
	char temp_char_value[33];
	char temp_buff[128];
	char *curr_ptr, *end_ptr;
	double home_arrive, home_durr, temp_miles, work_arrive, work_durr, glob_min_timestep_dbl, temp_val;
	double temp_hours_curr, temp_hours_curr_two, temp_hours_A, temp_hours_B, temp_hours_C, temp_hours_D;
	double temp_sec_curr, temp_sec_curr_two, temp_sec_A, temp_sec_B, temp_sec_C, temp_sec_D;
	FILE *FPTemp;
	TIMESTAMP temp_time;
	DATETIME temp_date;
	
	//Get global_minimum_timestep value and set the appropriate flag
	//Retrieve the global value, only does so as a text string for some reason
	gl_global_getvar("minimum_timestep",temp_buff,sizeof(temp_buff));

	//Initialize our parsing variables
	TempIdx = 0;
	glob_min_timestep_dbl = 0.0;

	//Loop through the buffer
	while ((TempIdx < 128) && (temp_buff[TempIdx] != '\0'))
	{
		glob_min_timestep_dbl *= 10;					//Adjust previous iteration value
		temp_val = (double)(temp_buff[TempIdx]-48);		//Decode the ASCII
		glob_min_timestep_dbl += temp_val;				//Accumulate it

		TempIdx++;									//Increment the index
	}

	//Convert it to a timestep
	glob_min_timestep = (TIMESTAMP)glob_min_timestep_dbl;

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

	//End-use properties
	//Check the maximum charge rate - if over 1.7 kW, make it 220 VAC
	if (CarInformation.MaxChargeRate > 1700)
	{
		load.config = EUC_IS220;
	//	load.breaker_amps = (double)((int)(CarInformation.MaxChargeRate / 220.0 * 1.1));	//May not be a realistic value
		load.breaker_amps = floor(((CarInformation.MaxChargeRate * 1.1) / 220.0) + 0.5);
		gl_verbose("INIT: load.breaker_amps = %0.4f.", load.breaker_amps);
	}
	else
	{
		load.breaker_amps = 20.0;	//110 VAC - just assume it is a 20 Amp breaker
	}

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
			if (FPTemp == NULL)
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
	prev_time = gl_globalclock;

	//Convert it to a timestamp value
	temp_time = gl_globalclock;

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
	//temp_hours_curr = temp_date.hour + temp_date.minute/60.0;//POSSIBLE PROBLEM HERE********************************************************************************************************
	temp_hours_curr = ((double)(temp_date.hour)) + (((double)(temp_date.minute))/60.0) + (((double)(temp_date.second))/3600.0);
	temp_sec_curr = ((double)(temp_date.hour))*3600.0 + ((double)(temp_date.minute))*60.0 + ((double)(temp_date.second));
	//gl_verbose("INIT: temp_hours_curr (hours in right format) = %0.4f.", temp_hours_curr);
	
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
						//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_hours_A - temp_hours_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
						CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_A - temp_sec_curr));
						//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
					}
					else
					{
						//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_hours_A + 24.0 - temp_hours_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
						CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_A + 86400.0 - temp_sec_curr));
						//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
					}
				}
				else if ((temp_sec_curr >= temp_sec_A) && (temp_sec_curr < temp_sec_B))	//At home
				{
					//Set location
					CarInformation.Location = VL_HOME;

					//Figure out time to next transition
					//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_hours_B - temp_hours_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
					CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_B - temp_sec_curr));
					//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
				}
				else if ((temp_sec_curr >= temp_sec_B) && (temp_sec_curr < temp_sec_C))		//Departed home, but not at work
				{
					//Set location
					CarInformation.Location = VL_HOME_TO_WORK;

					//Figure out time to next transition
					//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_hours_C - temp_hours_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
					CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_C - temp_sec_curr));
					//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
				}
				else	//Must be at work
				{
					//Set location
					CarInformation.Location = VL_WORK;

					//Figure out time to next transition
					//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_D - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
					CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_D - temp_sec_curr));
					//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
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
						//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_C - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
						CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_C - temp_sec_curr));
						//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
					}
					else
					{
						//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_C + 24.0 - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
						CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_C + 86400.0 - temp_sec_curr));
						//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
					}
				}
				else if ((temp_sec_curr >= temp_sec_C) && (temp_sec_curr < temp_sec_D))	//At work
				{
					//Set location
					CarInformation.Location = VL_WORK;

					//Figure out time to next transition
					//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_D - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
					CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_D - temp_sec_curr));
					//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
				}
				else if ((temp_sec_curr >= temp_sec_D) && (temp_sec_curr < temp_sec_A))		//Departed work, but not at home
				{
					//Set location
					CarInformation.Location = VL_WORK_TO_HOME;

					//Figure out time to next transition
					//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_A - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
					CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_A - temp_sec_curr));
					//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
				}
				else	//Must be at home
				{
					//Set location
					CarInformation.Location = VL_HOME;

					//Figure out time to next transition
					//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_B - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
					CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_B - temp_sec_curr));
					//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
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
					//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_D - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
					CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_D - temp_sec_curr));
					//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
				}
				else
				{
					//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_D + 24.0 - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
					CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_D + 86400.0 - temp_sec_curr));
					//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
				}
			}
			else if ((temp_sec_curr >= temp_sec_D) && (temp_sec_curr < temp_sec_A))	//Driving to home
			{
				//Set location
				CarInformation.Location = VL_WORK_TO_HOME;

				//Figure out time to next transition
				//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_A - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
				CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_A - temp_sec_curr));
				//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
			}
			else if ((temp_sec_curr >= temp_sec_A) && (temp_sec_curr < temp_sec_B))		//At home
			{
				//Set location
				CarInformation.Location = VL_HOME;

				//Figure out time to next transition
				//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_B - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
				CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_B - temp_sec_curr));
				//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
			}
			else	//Must be at leaving home and driving to work
			{
				//Set location
				CarInformation.Location = VL_HOME_TO_WORK;

				//Figure out time to next transition
				//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_C - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
				CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_C - temp_sec_curr));
				//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
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
				//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_B - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
				CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_B - temp_sec_curr));
				//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
			}
			else
			{
				//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_B + 24.0 - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
				CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_B + 86400.0 - temp_sec_curr));
				//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
			}
		}
		else if ((temp_sec_curr >= temp_sec_B) && (temp_sec_curr < temp_sec_C))	//Departed home, but not arrived at work
		{
			//Set location
			CarInformation.Location = VL_HOME_TO_WORK;

			//Figure out time to next transition
			//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_C - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
			CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_C - temp_sec_curr));
			//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
		}
		else if ((temp_sec_curr >= temp_sec_C) && (temp_sec_curr < temp_sec_D))		//At work
		{
			//Set location
			CarInformation.Location = VL_WORK;

			//Figure out time to next transition
			//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_D - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
			CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_D - temp_sec_curr));
			//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
		}
		else	//Must be driving home from work
		{
			//Set location
			CarInformation.Location = VL_WORK_TO_HOME;

			//Figure out time to next transition
			//CarInformation.next_state_change = temp_time + (TIMESTAMP)((temp_sec_A - temp_sec_curr)*3600.0);//POSSIBLE PROBLEM HERE********************************************************************************************************
			CarInformation.next_state_change = temp_time + (TIMESTAMP)(floor(temp_sec_A - temp_sec_curr));
			//gl_verbose("INIT: next_state_change = %d.", CarInformation.next_state_change);
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
			CarInformation.battery_SOC = (1.0 - ((CarInformation.next_state_change - temp_time) / CarInformation.HomeDuration)) * 100.0;

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

	return init_res;
}

int evcharger_det::isa(char *classname)
{
	return (strcmp(classname,"evcharger_det")==0 || residential_enduse::isa(classname));
}

TIMESTAMP evcharger_det::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	double temp_double, charge_out_percent;
	complex temp_complex;
	TIMESTAMP t2, tret, tdiff;

	temp_double = 0;

	//Only perform an action if the time has changed
	if (prev_time != t1)
	{
		//Get time difference
		tdiff = t1 - t0;

		//Zero enduse parameter, just to be safe (will set in the 1 spot they need to be)
		load.power = 0.0;
		charge_out_percent = 0.0;

		//Dependent on our location, figure out what to do
		switch (CarInformation.Location)
		{
			case VL_HOME:
				//See if we are still at home
				if (t1 >= CarInformation.next_state_change)	//Nope
				{
					//Transition to going to work
					CarInformation.Location = VL_HOME_TO_WORK;

					//Update "accumulator" time
					tdiff = CarInformation.next_state_change - t0;

					//Update battery information first - just in case the transition occurred in the middle of a timestep
					temp_double = (double)(tdiff) / 3600.0 * ChargeRate / 1000.0 * CarInformation.ChargeEfficiency;	//Convert to kWh
					
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
						//CarInformation.next_state_change = t1 + (TIMESTAMP)(temp_double);//POSSIBLE PROBLEM HERE********************************************************************************************************
						CarInformation.next_state_change = t1 + (TIMESTAMP)(floor(temp_double));
					}
					else	//Nope
					{
						//Add in how long it takes us to drive
						//CarInformation.next_state_change = t1 + (TIMESTAMP)(CarInformation.HomeWorkDuration);//POSSIBLE PROBLEM HERE********************************************************************************************************
						CarInformation.next_state_change = t1 + (TIMESTAMP)(floor(CarInformation.HomeWorkDuration));
						//gl_verbose("CASE: VL_HOME: t1 >= CarInformation.next_state_change: temp_double = %0.4f.", temp_double);
						//gl_verbose("CASE: VL_HOME: t1 >= CarInformation.next_state_change: next_state_change = %d.", CarInformation.next_state_change);
					}

					//Zero our output - power already should be
					ChargeRate = 0.0;
				}
				else	//Yes
				{
					//Reaffirm where we are
					CarInformation.Location = VL_HOME;

					//Update battery capacity with "last time's worth" of charge
					temp_double = (double)(tdiff) / 3600.0 * ChargeRate / 1000.0 * CarInformation.ChargeEfficiency;	//Convert to kWh 
					//gl_verbose("CASE: VL_HOME: t1 < CarInformation.next_state_change: temp_double = %0.4f.", temp_double);
					//gl_verbose("CASE: VL_HOME: t1 < CarInformation.next_state_change: next_state_change = %d.", CarInformation.next_state_change);

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
					ChargeRate = CarInformation.MaxChargeRate * charge_out_percent / 100.0;

					//Post the power in kW
					load.power = ChargeRate / 1000.0;
				}
				break;
			case VL_HOME_TO_WORK:
				//Zero our output - power already should be
				ChargeRate = 0.0;

				//See if we're still driving to work
				if (t1 >= CarInformation.next_state_change)	//Nope
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
						//CarInformation.next_state_change = t1 + (TIMESTAMP)(temp_double);//POSSIBLE PROBLEM HERE********************************************************************************************************
						CarInformation.next_state_change = t1 + (TIMESTAMP)(floor(temp_double));
					}
					else	//Nope
					{
						//Add in how long we are at work
						//CarInformation.next_state_change = t1 + (TIMESTAMP)(CarInformation.WorkDuration);//POSSIBLE PROBLEM HERE********************************************************************************************************
						CarInformation.next_state_change = t1 + (TIMESTAMP)(floor(CarInformation.WorkDuration));
						//gl_verbose("CASE: VL_HOME_TO_WORK: t1 >= CarInformation.next_state_change: temp_double = %0.4f.", temp_double);
						//gl_verbose("CASE: VL_HOME_TO_WORK: t1 >= CarInformation.next_state_change: next_state_change = %d.", CarInformation.next_state_change);
					}
				}
				else	//Yep, reaffirm our location
				{
					CarInformation.Location = VL_HOME_TO_WORK;
					//gl_verbose("CASE: VL_HOME_TO_WORK: t1 < CarInformation.next_state_change: temp_double = %0.4f.", temp_double);
					//gl_verbose("CASE: VL_HOME_TO_WORK: t1 < CarInformation.next_state_change: next_state_change = %d.", CarInformation.next_state_change);
				}
				break;
			case VL_WORK:
				//Zero our output - power already should be
				ChargeRate = 0.0;

				//See if we are still at work
				if (t1 >= CarInformation.next_state_change)	//Nope
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
						//CarInformation.next_state_change = t1 + (TIMESTAMP)(temp_double);//POSSIBLE PROBLEM HERE********************************************************************************************************
						CarInformation.next_state_change = t1 + (TIMESTAMP)(floor(temp_double));
					}
					else	//Nope
					{
						//Add in how long it takes us to drive
						//CarInformation.next_state_change = t1 + (TIMESTAMP)(CarInformation.WorkHomeDuration);//POSSIBLE PROBLEM HERE********************************************************************************************************
						CarInformation.next_state_change = t1 + (TIMESTAMP)(floor(CarInformation.WorkHomeDuration));
						//gl_verbose("CASE: VL_WORK: t1 >= CarInformation.next_state_change: temp_double = %0.4f.", temp_double);
						//gl_verbose("CASE: VL_WORK: t1 >= CarInformation.next_state_change: next_state_change = %d.", CarInformation.next_state_change);
					}
				}
				else	//Yes
				{
					//Just reaffirm our location
					CarInformation.Location = VL_WORK;
					//gl_verbose("CASE: VL_WORK: t1 < CarInformation.next_state_change: temp_double = %0.4f.", temp_double);
					//gl_verbose("CASE: VL_WORK: t1 < CarInformation.next_state_change: next_state_change = %d.", CarInformation.next_state_change);
				}
				break;
			case VL_WORK_TO_HOME:
				//Zero our output - power already should be
				ChargeRate = 0.0;

				//See if we're still driving home
				if (t1 >= CarInformation.next_state_change)	//Nope
				{
					//Transition us to home
					CarInformation.Location = VL_HOME;

					//Update charging for this brief stint
					//Begin by calculating our "apparent charge time"
					tdiff = t1 - CarInformation.next_state_change;

					//Update battery capacity with "last time's worth" of charge
					temp_double = (double)(tdiff) / 3600.0 * ChargeRate / 1000.0 * CarInformation.ChargeEfficiency;	//Convert to kWh

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
					ChargeRate = CarInformation.MaxChargeRate * charge_out_percent / 100.0;

					//Post the power in kW
					load.power = ChargeRate / 1000.0;

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
						//CarInformation.next_state_change = t1 + (TIMESTAMP)(temp_double);//POSSIBLE PROBLEM HERE********************************************************************************************************
						CarInformation.next_state_change = t1 + (TIMESTAMP)(floor(temp_double));
					}
					else	//Nope
					{
						//Add in how long we are home
						//CarInformation.next_state_change = t1 + (TIMESTAMP)(CarInformation.HomeDuration);//POSSIBLE PROBLEM HERE********************************************************************************************************
						CarInformation.next_state_change = t1 + (TIMESTAMP)(floor(CarInformation.HomeDuration));
						//gl_verbose("CASE: VL_WORK_TO_HOME: t1 >= CarInformation.next_state_change: temp_double = %0.4f.", temp_double);
						//gl_verbose("CASE: VL_WORK_TO_HOME: t1 >= CarInformation.next_state_change: next_state_change = %d.", CarInformation.next_state_change);
					}
				}
				else	//Yep, reaffirm our location
				{
					CarInformation.Location = VL_WORK_TO_HOME;
					//gl_verbose("CASE: VL_WORK_TO_HOME: t1 < CarInformation.next_state_change: temp_double = %0.4f.", temp_double);
					//gl_verbose("CASE: VL_WORK_TO_HOME: t1 < CarInformation.next_state_change: next_state_change = %d.", CarInformation.next_state_change);
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
		prev_time = t1;
		//gl_verbose("SYNC: t0 = %d.", t0);
		//gl_verbose("SYNC: t1 = %d.", t1);
	}

	//Update enduse parameter - assumes only constant power right now
	load.total = load.power;

	//Pull the next state transition
	tret = CarInformation.next_state_change;

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
	//gl_verbose("SYNC: t2 = %d.", t2);
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

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_evcharger_det(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(evcharger_det::oclass);

		if (*obj!=NULL)
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

EXPORT TIMESTAMP sync_evcharger_det(OBJECT *obj, TIMESTAMP t0)
{
	try {
		evcharger_det *my = OBJECTDATA(obj, evcharger_det);
		TIMESTAMP t1 = my->sync(obj->clock, t0);
		obj->clock = t0;
		return t1;
	}
	SYNC_CATCHALL(evcharger_det);
}

/**@}**/
