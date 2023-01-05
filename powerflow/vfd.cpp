// $Id: vfd.cpp 
/**	Copyright (C) 2017 Battelle Memorial Institute

	
	@{
*/


#include <cmath>

#include "vfd.h"

//initialize pointers
CLASS* vfd::oclass = nullptr;
CLASS* vfd::pclass = nullptr;

//////////////////////////////////////////////////////////////////////////
// vfd CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

vfd::vfd(MODULE *mod) : link_object(mod)
{
	if(oclass == nullptr)
	{
		pclass = link_object::oclass;
		
		oclass = gl_register_class(mod,"vfd",sizeof(vfd),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class vfd";
		else
			oclass->trl = TRL_PROTOTYPE;
        
        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",

			PT_double, "rated_motor_speed[1/min]", PADDR(ratedRPM), PT_DESCRIPTION, "Rated speed of the VFD in RPM. Default = 1800 RPM",
			PT_double, "desired_motor_speed[1/min]", PADDR(desiredRPM), PT_DESCRIPTION, "Desired speed of the VFD In ROM. Default = 1800 RPM (max)",
			PT_double, "motor_poles", PADDR(motorPoles), PT_DESCRIPTION, "Number of Motor Poles. Default = 4",
			PT_double, "rated_output_voltage[V]", PADDR(voltageLLRating), PT_DESCRIPTION, "Line to Line Voltage - VFD Rated voltage. Default to TO node nominal_voltage",
			PT_double, "rated_horse_power[hp]", PADDR(horsePowerRatedVFD), PT_DESCRIPTION, "Rated Horse Power of the VFD. Default = 75 HP",
			PT_double, "nominal_output_frequency[Hz]", PADDR(nominal_output_frequency), PT_DESCRIPTION, "Nominal VFD output frequency. Default = 60 Hz",
			
			PT_double, "desired_output_frequency[Hz]", PADDR(desiredFrequency), PT_DESCRIPTION, "VFD desired output frequency based on the desired RPM",
			PT_double, "current_output_frequency[Hz]", PADDR(currentFrequency), PT_DESCRIPTION, "VFD currently output frequency",
			PT_double, "efficiency[%]", PADDR(currEfficiency), PT_DESCRIPTION, "Current VFD efficiency based on the load/VFD output Horsepower",
			
			PT_double, "stable_time[s]", PADDR(stableTime), PT_DESCRIPTION, "Time taken by the VFD to reach desired frequency (based on RPM). Default = 1.45 seconds",
			
			PT_enumeration, "vfd_state", PADDR(vfdState), PT_DESCRIPTION, "Current state of the VFD",
				PT_KEYWORD, "OFF", (enumeration)VFD_OFF,
				PT_KEYWORD, "CHANGING", (enumeration)VFD_CHANGING,
				PT_KEYWORD, "STEADY_STATE", (enumeration)VFD_STEADY,
			
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_vfd)==nullptr)
			GL_THROW("Unable to publish VFD deltamode function");
		if (gl_publish_function(oclass,	"postupdate_pwr_object", (FUNCTIONADDR)postupdate_vfd)==nullptr)
			GL_THROW("Unable to publish VFD deltamode function");

		//Publish restoration-related function (current update)
		if (gl_publish_function(oclass,	"update_power_pwr_object", (FUNCTIONADDR)updatepowercalc_link)==nullptr)
			GL_THROW("Unable to publish VFD external power calculation function");
		if (gl_publish_function(oclass,	"check_limits_pwr_object", (FUNCTIONADDR)calculate_overlimit_link)==nullptr)
			GL_THROW("Unable to publish VFD external power limit calculation function");
		if (gl_publish_function(oclass,	"perform_current_calculation_pwr_link", (FUNCTIONADDR)currentcalculation_link)==nullptr)
			GL_THROW("Unable to publish VFD external current calculation function");

		//Publish external VFD interfacing function
		if (gl_publish_function(oclass,	"vfd_current_injection_update", (FUNCTIONADDR)current_injection_update_VFD)==nullptr)
			GL_THROW("Unable to publish VFD external current injection calculation function");
    }
}

int vfd::isa(char *classname)
{
	return strcmp(classname,"vfd")==0 || link_object::isa(classname);
}

int vfd::create()
{
	int result = link_object::create();

	ratedRPM   = 1800;
	desiredRPM = 1800;
	motorPoles = 4;
	voltageLLRating = -1.0;
	horsePowerRatedVFD = 75;
	nominal_output_frequency = -1.0;
	stableTime = 1.45;

	vfdState = VFD_OFF;
	prev_vfdState = VFD_OFF;

	desiredFrequency = 0.0;
	currentFrequency = 0.0;
	currEfficiency = 0.0;
	prev_desiredRPM = 0.0;
	
	curr_time_value = 0.0;
	prev_time_value = 0.0;

	//Null the array pointers, just because
	settleFreq = nullptr;
	settleFreq_length = 0;
	curr_array_position = 0;
	force_array_realloc = false;

	nominal_output_radian_freq = 0.0;
	VbyF = 0.0;
	HPbyF = 0.0;

	//Null node pointers
	fNode = nullptr;
	tNode = nullptr;
	
	//Populate the efficiency curve coefficients
	//Backwards fill - index = z^index
	//http://www.variablefrequencydrive.org/vfd-efficiency
	//z_val is part of a "center and fit" consequence of the MATLAB equation fit - computed later
	//3.497*z^7 - 8.2828*z^6 + 0.97848*z^5 + 8.7113*z^4 - 3.2079*z^3 - 4.4504*z^2 + 3.8759*z + 96.014
	efficiency_coeffs[0] = 96.014;
	efficiency_coeffs[1] = 3.8759;
	efficiency_coeffs[2] = -4.4504;
	efficiency_coeffs[3] = -3.2079;
	efficiency_coeffs[4] = 8.7113;
	efficiency_coeffs[5] = 0.97848;
	efficiency_coeffs[6] = -8.2828;
	efficiency_coeffs[7] = 3.497;

	//Initialize other tracking variables
	prev_power[0] = gld::complex(0.0,0.0);
	prev_power[1] = gld::complex(0.0,0.0);
	prev_power[2] = gld::complex(0.0,0.0);

	//Flag this as a VFD
	SpecialLnk=VFD;
	
	return result;
}

/**
* Object initialization is called once after all object have been created
*
* @param parent a pointer to this object's parent
* @return 1 on success, 0 on error
*/
int vfd::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	FUNCTIONADDR temp_fxn;
	STATUS temp_status_val;
	double temp_voltage_check, temp_diff_value;

	int result = link_object::init(parent);

	//Check for deferred
	if (result == 2)
		return 2;	//Return the deferment - no sense doing everything else!

	if (stableTime <= 0.0)
	{
		GL_THROW("VFD:%d - %s - the stableTime must be positive",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The stableTime value for the VFD is negative.  This must be a positive value for the simulation to work
		properly.
		*/
	}

	//Map up the from and to nodes
	fNode = OBJECTDATA(from,node);
	tNode = OBJECTDATA(to,node);

	//Check ratings
	if (voltageLLRating <= 0.0)
	{
		voltageLLRating = tNode->nominal_voltage * sqrt(3.0);

		gl_warning("VFD:%d - %s - rated_output_voltage was not set, defaulting to TO node nominal",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The rated_output_voltage was not set for the VFD.  It has been defaulted to the nominal_voltage value of the TO node.
		*/
	}
	else	//Was specified
	{
		//Compute the VLL of the TO node
		temp_voltage_check = tNode->nominal_voltage * sqrt(3.0);

		//Get the difference
		temp_diff_value = fabs(voltageLLRating - temp_voltage_check);

		if (temp_diff_value > 0.5)
		{
			gl_warning("VFD:%d - %s - rated_output_voltage does not match the TO node nominal voltage",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The rated_output_voltage does not match the nominal_voltage of the TO node.  This may cause some issues with simulations results.
			*/
		}
	}//End voltage was specified

	//Check the input voltage
	temp_voltage_check = fNode->nominal_voltage - tNode->nominal_voltage;

	if (temp_voltage_check < 0.0)
	{
		gl_warning("VFD:%d - %s - FROM node nominal_voltage is not equal to or greater than TO node nominal_voltage - implies DC-DC converter!e",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The from node nominal_voltage is less than the to node nominal_voltage.  This implies an internal DC-DC converter, which is not explicityly modeled.  Efficiency values
		will not be accurate!
		*/
	}

	//Check the nominal output frequency
	if (nominal_output_frequency <= 0.0)
	{
		gl_warning("VFD:%d - %s - nominal_output_frequency is not set or is negative - defaulting to system nominal",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The nominal_output_frequency variable was not set properly - defaulting to nominal_frequency of the system.
		 */

		nominal_output_frequency = nominal_frequency;
	}

	//Check rated speed
	if (ratedRPM <= 0.0)
	{
		GL_THROW("VFD:%d - %s - rated_motor_speed must be positive and non-zero!",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The value specified for rated_motor_speed must be positive and non-zero in order to properly work.
		 */

		return 0;
	}

	//Check the poles too
	if (motorPoles <= 0.0)
	{
		GL_THROW("VFD:%d - %s - motor_poles must be positive and non-zero!",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The value specified for motor_poles must be positive and non-zero in order to properly work.
		 */

		return 0;
	}

	//And finally, the rated horse power
	if (horsePowerRatedVFD <= 0.0)
	{
		GL_THROW("VFD:%d - %s - rated_horse_power must be positive and non-zero!",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The value specified for rated_horse_power must be positive and non-zero in order to properly work.
		 */

		return 0;
	}

	//Make sure we're three-phase, since that's all that works right now
	if (!(has_phase(PHASE_A) & has_phase(PHASE_B) & has_phase(PHASE_C)))
	{
		GL_THROW("VFD:%d - %s - Must be three-phase!",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The VFD only operates as a three-phase device, at this time.  Please specify all phases.
		 */

		return 0;
	}

	//Initialize the phase angle array
	phasorVal[0] = 0.0;
	phasorVal[1] = -2.0 * PI / 3.0;
	phasorVal[2] = 2.0 * PI / 3.0;

	//Compute derived variables

	//Create constant radian output frequency, since this won't change
	nominal_output_radian_freq = nominal_frequency*2.0*PI;

	//Voltage to frequency relationship
	VbyF = voltageLLRating/nominal_output_frequency;

	//Percentage horsepower per frequency
	HPbyF = 100.0/nominal_output_frequency;

	//Allocate the initial frequency tracking array
	temp_status_val = alloc_freq_arrays(1.0);

	//See what state we're in, by default - if running, populate variable
	if (vfdState == VFD_STEADY)
	{
		//Update trackers
		prev_desiredRPM = desiredRPM;
		prev_vfdState = vfdState;

		//Update "state" variables
		CheckParameters();

		currentFrequency = desiredFrequency;
	}
	//Default else - it needs to move

	//Check the run
	if (temp_status_val == FAILED)
	{
		GL_THROW("VFD:%d - %s - Allocating the dynamic arrays for the frequency tracking failed",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to allocate the dynamic length arrays for the vfd operation, an issue was encountered.  Please try again.  If the error
		persists, please submit an issue. */
		
	}
	
	//Map our "TO" node as a proper VFD
	temp_fxn = (FUNCTIONADDR)(gl_get_function(to,"attach_vfd_to_pwr_object"));

	//Make sure it worked
	if (temp_fxn == nullptr)
	{
		GL_THROW("VFD:%d - %s -- Failed to map TO-node flag function",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to update the flagging on the "TO" node to reflect being attached to a VFD, an error occurred.
		Please try again.  If the error persists, please submit an issue
		*/
	}

	//Now call the function
	temp_status_val = ((STATUS (*)(OBJECT *,OBJECT *))(*temp_fxn))(to,obj);

	//Make sure it worked
	if (temp_status_val == FAILED)
	{
		GL_THROW("VFD:%d - %s -- Failed to map TO-node flag function",obj->id,(obj->name ? obj->name : "Unnamed"));
		//Defined above - use the same function
	}
	
	//Populate the previous time, just in case, put it back 1
	prev_time_value = (double)(gl_globalclock) - 1.0;

	return result;
}

void vfd::CheckParameters()
{
	OBJECT *obj = OBJECTHDR(this);

	//Make sure desiredRPM hasn't gone negative
	if (desiredRPM < 0.0)
	{
		GL_THROW("VFD:%d - %s - the desired_motor_speed must be positive",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The desired_motor_speed value for the VFD is negative.  This must be a positive value for the simulation to work
		properly.  Backwards spinning motors are not supported at this time.
		*/
	}
	
	//Force us to off, if the value goes to zero
	if (desiredRPM == 0.0)
	{
		vfdState = VFD_OFF;
	}

	//See if we need to be turned on
	if ((prev_desiredRPM == 0.0) && (desiredRPM > 0.0))
	{
		//Flag to start
		vfdState = VFD_STEADY;
	}

	//Compute the desired output frequency
	desiredFrequency = (desiredRPM*motorPoles)/120.0; //Convert RPM to Hz (with pole pair division)
	
	if ((desiredFrequency > 0.0) && (desiredFrequency < 3.0))
	{
		gl_warning("VFD:%d - %s - the desired_motor_speed results in a frequency of less than 3.0 Hz",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The desired_motor_speed results in a value that is less than the starting frequency of a typical VFD.  This is high discouraged, since it may
		not function properly in this range.
		*/
	}

	if (desiredRPM/ratedRPM <=0.75)
	{
		gl_warning("VFD:%d - %s - VFD currently at %f%% of rated speed -- VFDs perform best when running at >= 75 percent output",obj->id,(obj->name ? obj->name : "Unnamed"),(desiredRPM*100/ratedRPM));
		/*  TROUBLESHOOT
		Recommended to run a VFD at 75%.  Lower values are not recommended and can result in odd behavior
		*/
	}
}

//Function to perform the current update - called by a node object, after they've updated (so current is accurate)
STATUS vfd::VFD_current_injection(void)
{
	gld::complex currRat, lossCurr[3];
	int index_val, index_val_inner, index_new_limit;
	double z_val, driveCurrPower, temp_coeff, settleVolt, avg_freq_value;
	gld::complex temp_power_val, powerOutElectrical, powerInElectrical;
	gld::complex settleVoltOut[3];
	bool desiredChange;
	int tdiff_value;
	double diff_time_value;

	//By default, no craziness has happened
	desiredChange = false;
	
	//Grab the deltamode clock in here
	if (deltatimestep_running > 0)	//Deltamode
	{
		//Grab the current deltamode clock
		curr_time_value = gl_globaldeltaclock;
	}

	//See if the frequency setting has changed
	if (prev_desiredRPM != desiredRPM)
	{
		//Call the parameter check
		CheckParameters();

		//Flag us as a change -- only gets handled if we were already changing
		desiredChange = true;

		//Update the tracker
		prev_desiredRPM = desiredRPM;
	}


	//Update frequency value, if needed
	if (curr_time_value != prev_time_value)	//Updates only occur for new time steps
	{
		//Get the time difference
		diff_time_value = curr_time_value - prev_time_value;

		//See if we're in deltamode
		if (deltatimestep_running > 0)
		{
			//Deltamode is fixed incremements, so just do one
			tdiff_value = 1;
		}
		else	//Standard, steady-state
		{
			//See how far we went
			tdiff_value = (int)(curr_time_value - prev_time_value);
		}

		//Check states - backward progression and overlapping checks
		//See if we're "steady", since that has the most potential to be different
		if (vfdState == VFD_STEADY)
		{
			//See what our previous state was
			if (prev_vfdState == VFD_OFF)	//Were off, so need to actually be starting up
			{
				//flag us as changing
				vfdState = VFD_CHANGING;

				//Reset the base phasor values, just because
				phasorVal[0] = 0.0;
				phasorVal[1] = -2.0 * PI / 3.0;
				phasorVal[2] = 2.0 * PI / 3.0;

				//Set the current frequency to 3.0 (starting value)
				currentFrequency = 3.0;

				//Reset the array pointer, since we'll be starting earlier (but are already populated)
				curr_array_position = 0;

				//Populate the frequency array
				for (index_val=0; index_val<settleFreq_length; index_val++)
				{
					//Fill it with the starting frequency
					settleFreq[index_val] = 3.0;
				}

				//Override the increment -- it implies we just turned on (otherwise, what drove us here?)
				tdiff_value = 1;	//Regardless of deltamode or QSS
			}
			else if (prev_vfdState == VFD_CHANGING)	//were changing
			{
				//We were changing, so this implies we've reached our goal
				//Do nothing, we're effectively steady state - just flag us for the sake of doing so
				prev_vfdState = VFD_STEADY;
			}
			else	//Already were steady
			{
				//See if our values have changed
				if (currentFrequency != desiredFrequency)
				{
					//Flag us as changing
					vfdState = VFD_CHANGING;

					//Populate the array with our current value
					for (index_val=0; index_val<settleFreq_length; index_val++)
					{
						//Fill it with the starting frequency
						settleFreq[index_val] = currentFrequency;
					}

					//Reset the array pointer
					curr_array_position = 0;

					//Increment update is irrelevant, so don't bother
				}
				//Default else -- nothing new to do, just stay as we were
			}
		}//End VFD_STEADY

		//Check for changing state, since we may have been kicked here
		if (vfdState == VFD_CHANGING)	//We're mid-adjust
		{
			//See if we've changed values or not
			if ((prev_vfdState == VFD_CHANGING) && desiredChange)
			{
				//Pre-empt the array with just our current value
				for (index_val=0; index_val<settleFreq_length; index_val++)
				{
					settleFreq[index_val] = currentFrequency;
				}

				//Reset the pointer
				curr_array_position = 0;

				//Override the increment -- we just changed, so something flagged us
				tdiff_value = 1;	//Regardless of deltamode or QSS
			}

			//Now run the standard routine
			//See if we should be done
			if (curr_array_position == settleFreq_length)
			{
				//We're done - transition us
				vfdState = VFD_STEADY;
			}
			else	//Update like normal
			{
				//Figure out how far we would be going
				index_new_limit = curr_array_position + tdiff_value;

				//Make sure we won't exceed our length
				if (index_new_limit > settleFreq_length)
				{
					//Just update to our limit - implies we're done anyways
					//Populate the array
					for (index_val=curr_array_position; index_val<settleFreq_length; index_val++)
					{
						//Populate our current entry with the desired frequency
						settleFreq[index_val] = desiredFrequency;
					}

					//Update the pointer
					curr_array_position = settleFreq_length;

					//Flag us as steady state too
					vfdState = VFD_STEADY;

					//This method will technical compute an irrelevant average.  Oh well.
				}
				else	//We'll be okay
				{
					//Populate the array
					for (index_val=curr_array_position; index_val<index_new_limit; index_val++)
					{
						//Populate our current entry with the desired frequency
						settleFreq[index_val] = desiredFrequency;
					}

					//Update the pointer
					curr_array_position = index_new_limit;
				}//End within range

				//Get the average
				//Reset the accumulator
				avg_freq_value = 0.0;

				//Loop and compute
				for (index_val=0; index_val<settleFreq_length; index_val++)
				{
					avg_freq_value += settleFreq[index_val];
				}

				//Average it
				avg_freq_value /= (double)settleFreq_length;

				//This is now the current frequency value
				currentFrequency = avg_freq_value;
			}//End standard routine
		}//End VFD_CHANGING

		//Check for off state, after all is said and done
		if (vfdState == VFD_OFF)
		{
			//Previous state doesn't really matter - just turn us off
			currentFrequency = 0.0;
		}//End VFD_OFF

		//Update previous state tracker
		prev_vfdState = vfdState;

		//Update tracking
		prev_time_value = curr_time_value;

		//Update the VFD phasor output, if appropriate
		if (vfdState != VFD_OFF)
		{
			phasorVal[0] += (2*PI*currentFrequency - nominal_output_radian_freq)*diff_time_value;
			phasorVal[1] += (2*PI*currentFrequency - nominal_output_radian_freq)*diff_time_value;
			phasorVal[2] += (2*PI*currentFrequency - nominal_output_radian_freq)*diff_time_value;
		}
		//Default else -- if it is off, who cares?
	}//End frequency value update

	//Update output voltage magnitude
	settleVolt = VbyF*currentFrequency;

	//Temporary variable
	settleVoltOut[0] = gld::complex(settleVolt,0)*complex_exp(phasorVal[0]);
	settleVoltOut[1] = gld::complex(settleVolt,0)*complex_exp(phasorVal[1]);
	settleVoltOut[2] = gld::complex(settleVolt,0)*complex_exp(phasorVal[2]);
	
	//Check the current state -- if we're off, disconnect things
	if (vfdState == VFD_OFF)
	{
		//efficiency is 0, because we're off
		currEfficiency = 0.0;

		//Loop through and zero-post things
		for (index_val=0; index_val<3; index_val++)
		{
			//Set output voltage to zero
			tNode->voltage[index_val] = gld::complex(0.0,0.0);

			//Force output and input currents to zero
			current_out[index_val] = gld::complex(0.0,0.0);
			current_in[index_val] = gld::complex(0.0,0.0);

			//Do the same for power
			indiv_power_in[index_val] = gld::complex(0.0,0.0);
			indiv_power_out[index_val] = gld::complex(0.0,0.0);

			//Update the input power posting
			fNode->power[index_val] -= prev_power[index_val];

			//Update the tracking variable
			prev_power[index_val] = gld::complex(0.0,0.0);

		}//end phase loop
	}//End VFD off
	else	//otherwise moving
	{
		//Update efficiency
		//Relate the frequency output to the estimated out put power (as percentage of rated HP)
		driveCurrPower = HPbyF*currentFrequency;

		//Center/scale the value, for the efficiency check
		z_val = (driveCurrPower - 50.138)/37.009;

		//Compute the efficiency value
		currEfficiency = 0.0;

		//Loop and multiply
		for (index_val=0; index_val<8; index_val++)
		{
			if (index_val == 0)
			{
				currEfficiency = efficiency_coeffs[0];
			}
			else	//Others
			{
				temp_coeff = z_val;

				for (index_val_inner=1; index_val_inner<index_val; index_val_inner++)
				{
					temp_coeff = temp_coeff * z_val;
				}

				//Scale it
				temp_coeff = temp_coeff * efficiency_coeffs[index_val];

				//Accumulate it
				currEfficiency += temp_coeff;

			}//End not index 0
		}//End efficiency FOR loop

		//Accumulate the output power
		powerOutElectrical = gld::complex(0.0,0.0);

		for (index_val=0; index_val<3; index_val++)
		{
			//Copy the load current into the link variables
			current_out[index_val] = tNode->current[index_val];

			//Compute the phase power
			temp_power_val = (tNode->voltage[index_val]*~current_out[index_val]);

			powerOutElectrical += temp_power_val;
	
			//For giggles, update our output power and current variables too
			indiv_power_out[index_val] = temp_power_val;
		}

		//Translate the output power to the input - divide it by 3 here too (for per-phase)
		//Assumes everything coalesces onto a common DC bus
		//Zero check the efficiency
		if (currEfficiency == 0.0)
		{
			//Zero both - power isn't coming from nowhere!
			powerOutElectrical = gld::complex(0.0,0.0);
			powerInElectrical = gld::complex(0.0,0.0);
		}
		else
		{
			powerInElectrical = powerOutElectrical*100.0/currEfficiency/3.0;
		}

		//Balance the power on the input, post it, and update the accumulator
		for (index_val=0; index_val<3; index_val++)
		{
			//Post the power
			fNode->power[index_val] += powerInElectrical - prev_power[index_val];

			//Update power values too, just because
			indiv_power_in[index_val] = powerInElectrical;

			//Update the tracking variable
			prev_power[index_val] = powerInElectrical;

			//Compute the amount and apply it to the input
			if (fNode->voltage[index_val].Mag() == 0.0)
			{
				current_in[index_val] = gld::complex(0.0,0.0);
			}
			else
			{
				current_in[index_val] = ~(powerInElectrical/fNode->voltage[index_val]);
			}

			//Push the new voltage out to the to node
			tNode->voltage[index_val] = settleVoltOut[index_val];
		}//End phase FOR loop
	}//End not off

	//Always return success right now
	return SUCCESS;
}

TIMESTAMP vfd::presync(TIMESTAMP t0)
{
	return link_object::presync(t0);
}

TIMESTAMP vfd::sync(TIMESTAMP t0)
{
	TIMESTAMP t2;

	//Update time tracker
	curr_time_value = (double)t0;

	t2 = link_object::sync(t0);
	
	return t2;
}

TIMESTAMP vfd::postsync(TIMESTAMP t0)
{
	TIMESTAMP t1, tret;
	int time_left;

	//Normal link update
	t1 = link_object::postsync(t0);

	//See if we're transitioning
	if (vfdState == VFD_CHANGING)
	{
		//Figure out how far we need to go
		if (curr_array_position < settleFreq_length)	//Make sure we're not a shoulder
		{
			//Compute time left
			time_left = settleFreq_length - curr_array_position;

			//Make it a timestamp
			tret = t0 + (TIMESTAMP)time_left;

			//Now figure out progression
			if (t1 != TS_NEVER)
			{
				if (t1 < 0)	//Already softed
				{
					if (tret < (-t1))
					{
						return -tret;
					}
					else	//t1 sooner
					{
						return t1;
					}
				}
				else	//Not softed
				{
					if (tret < t1)
					{
						return -tret;
					}
					else
					{
						return t1;	//Leave it hard, since maybe it was intentional
					}
				}
			}
			else	//It was TS_NEVER, just return us
			{
				return -tret;	//Soft event
			}
		}//End valid interval
		else	//We are, just go off into never land
		{
			//Flag us first, in case we jump a ways forward
			vfdState = VFD_STEADY;

			return t1;
		}
	}//End VFD changing
	else	//Off or Steady, we don't care
	{
		return t1;
	}
}

//Function to perform exp(j*val)
//Basically a complex rotation
gld::complex vfd::complex_exp(double angle)
{
	gld::complex output_val;
	double temp_angle, new_angle;
	int even_amount;

	//Mod us down to around 2pi, since the cos and sin functions get a little flaky with big numbers
	if (fabs(angle) > (2.0 * PI))
	{
		//Find the even amount
		even_amount = (int)(angle/(2.0 * PI));

		//Compute it as a temporary value
		temp_angle = (double)(even_amount) * 2.0 * PI;

		//Subtract them
		new_angle = angle - temp_angle;

		//Assign it
		angle = new_angle;
	}

	//exp(jx) = cos(x)+j*sin(x)
	output_val = gld::complex(cos(angle),sin(angle));

	return output_val;
}

//Function to reallocate/allocate variable time arrays
STATUS vfd::alloc_freq_arrays(double delta_t_val)
{
	OBJECT *obj = OBJECTHDR(this);
	int a_index;
	
	//See if we were commanded to reallocate -- this would be done on a zero-th pass of interupdate, most likely (or in postupdate, when transitioning out)
	if (force_array_realloc)
	{
		//Make sure we actually are allocated first
		if (settleFreq != nullptr)
		{
			//Free the allocation
			gl_free(settleFreq);
			
			//Zero the length, for the sake of doing so
			settleFreq_length = 0;
		}
		//Default else - we never were allocated, so no "force" is required
		
		//Deflag our force, just because
		force_array_realloc = false;
		
		//NULL the pointer, for giggles - put out here, just to be paranoid
		settleFreq = nullptr;
	}
	
	//See if we need to be allocated
	if (settleFreq == nullptr)
	{
		//Determine the size of our array
		settleFreq_length = (int)(stableTime/delta_t_val + 0.5);
		
		//Make sure it is valid
		if (settleFreq_length <= 0)
		{
			gl_error("vfd:%d %s -- the stable_time value must result in at least 1 timestep of averaging!",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to allocate the averaging arrays, an invalid length was specified.  Make sure your stable_time value is greater than zero!
			*/
			
			return FAILED;
		}
		
		//Allocate it
		settleFreq = (double *)gl_malloc(settleFreq_length*sizeof(double));
		
		//Make sure it worked
		if (settleFreq == nullptr)
		{
			//Bit duplicative to the "function failure message", but meh
			gl_error("vfd:%d %s -- failed to allocate dynamic array",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			While allocating a dynamic array, an error occurred.  Please try again.  If the error persists, please submit
			an issue to the website. */
			
			
			return FAILED;
		}
		
		//Zero it, just for giggles/paranoia
		for (a_index=0; a_index<settleFreq_length; a_index++)
		{
			settleFreq[a_index] = 0.0;
		}
		
		//Reset pointer
		curr_array_position = 0;

		//Success
		return SUCCESS;
	}
	else //Already done and we weren't forced, so just assume we were called erroneously
	{
		return SUCCESS;
	}
}

//Module-level deltamode call
SIMULATIONMODE vfd::inter_deltaupdate_vfd(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	double dt_value, deltatimedbl;
	STATUS ret_value;

	//See if we're the very first pass/etc
	if ((delta_time == 0) && (iteration_count_val == 0) && !interupdate_pos)	//First deltamode call
	{
		//Compute the timestep as a double
		dt_value = (double)dt/(double)DT_SECOND;

		//Force it to update, just in case
		force_array_realloc = true;

		//Call the array allocation function
		ret_value = alloc_freq_arrays(dt_value);

		//Simple error check
		if (ret_value == FAILED)
		{
			return SM_ERROR;
		}
	}

	//Most of these items were copied from link.cpp's interupdate (need to replicate)
	if (!interupdate_pos)	//Before powerflow call
	{
		//Link sync stuff	
		NR_link_sync_fxn();

		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}
	else	//After the call
	{
		//Call postsync
		BOTH_link_postsync_fxn();

		//Figure out the path forward
		if (vfdState == VFD_CHANGING)	//Still adjusting, so bring us back
		{
			return SM_DELTA;
		}
		else
		{
			return SM_EVENT;	//VFD always just want out
		}
	}
}//End module deltamode

//Post update deltamode
STATUS vfd::post_deltaupdate_vfd(void)
{
	//Force it as an update -- otherwise it just exits (which works, but technically has too large of an array)
	force_array_realloc = true;

	//Simply call a realloc of the array, back to steady state values
	return alloc_freq_arrays(1.0);
}

//Exposed function - external call to current injection
EXPORT STATUS current_injection_update_VFD(OBJECT *obj)
{
	vfd *vfdObj = OBJECTDATA(obj,vfd);

	return vfdObj->VFD_current_injection();
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: vfd
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_vfd(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(vfd::oclass);
		if (*obj!=nullptr)
		{
			vfd *my = OBJECTDATA(*obj,vfd);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(vfd);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_vfd(OBJECT *obj)
{
	try {
		vfd *my = OBJECTDATA(obj,vfd);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(vfd);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_vfd(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try
	{
		vfd *pObj = OBJECTDATA(obj,vfd);
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(t0);
		case PC_BOTTOMUP:
			return pObj->sync(t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
	}
	SYNC_CATCHALL(vfd);
}

EXPORT int isa_vfd(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,vfd)->isa(classname);
}

//Deltamode export
EXPORT SIMULATIONMODE interupdate_vfd(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	vfd *my = OBJECTDATA(obj,vfd);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_vfd(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_vfd(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

//Deltamode export - post update
EXPORT STATUS postupdate_vfd(OBJECT *obj)
{
	vfd *my = OBJECTDATA(obj,vfd);
	STATUS status_val = FAILED;
	try
	{
		status_val = my->post_deltaupdate_vfd();
		return status_val;
	}
	catch (char *msg)
	{
		gl_error("postupdate_vfd(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status_val;
	}
}

/**@}*/

