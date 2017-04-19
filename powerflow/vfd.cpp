// $Id: vfd.cpp 
/**	Copyright (C) 2017 Battelle Memorial Institute

	
	@{
*/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <numeric>

#include "vfd.h"

//initialize pointers
CLASS* vfd::oclass = NULL;
CLASS* vfd::pclass = NULL;

//////////////////////////////////////////////////////////////////////////
// vfd CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

vfd::vfd(MODULE *mod) : link_object(mod)
{
	if(oclass == NULL)
	{
		pclass = link_object::oclass;
		
		oclass = gl_register_class(mod,"vfd",sizeof(vfd),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class vfd";
		else
			oclass->trl = TRL_PROTOTYPE;
        
        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
			//Properties go here
			PT_double, "vfd_rated_speed[1/min]", PADDR(ratedRPM), PT_DESCRIPTION, "Rated speed of the VFD in RPM. Default = 1800 RPM",
			PT_double, "motor_poles", PADDR(motorPoles), PT_DESCRIPTION, "Number of Motor Poles. Default = 4",
			PT_double, "rated_vfd_line_to_Line_voltage[V]", PADDR(voltageLLRating), PT_DESCRIPTION, "Line to Line Voltage - VFD Rated voltage. Default = 480V",
			PT_double, "Desired_vfd_rpm", PADDR(desiredRPM), PT_DESCRIPTION, "Desired speed of the VFD In ROM. Default = 1800 RPM (max)",
			PT_double, "rated_vfd_horse_power[hp]", PADDR(horsePowerRatedVFD), PT_DESCRIPTION, "Rated Horse Power of the VFD. Default = 75 HP",
			PT_double, "nominal_output_frequency[Hz]", PADDR(nominal_output_frequency), PT_DESCRIPTION, "Nominal VFD output frequency. Default = 60 Hz",
			
			PT_double, "drive_frequency[Hz]", PADDR(driveFrequency), PT_DESCRIPTION, "Current VFD frequency based on the desired RPM",
			PT_double, "vfd_efficiency", PADDR(currEfficiency), PT_DESCRIPTION, "Current VFD efficiency based on the load/VFD output Horsepower",			
			
			PT_double, "stable_time[s]", PADDR(stableTime), PT_DESCRIPTION, "Time taken by the VFD to reach desired frequency (based on RPM). Default = 1.45 seconds",
			PT_double, "settle_time", PADDR(settleTime), PT_DESCRIPTION, "Total number of steps/counts during the VFD operation.",
			PT_double, "power_out_electrical[W]", PADDR(powerOutElectrical), PT_DESCRIPTION, "VFD output electrical power",
			PT_double, "power_losses_VFD[W]", PADDR(powerLosses), PT_DESCRIPTION, "VFD electrical power losses",
			PT_double, "power_in_electrical[W]", PADDR(powerInElectrical), PT_DESCRIPTION, "Input electrical power to VFD",
			
			PT_complex, "current_in_a[A]", PADDR(calc_current_in[0]), PT_DESCRIPTION, "Phase A input current to VFD",
			PT_complex, "current_in_b[A]", PADDR(calc_current_in[1]), PT_DESCRIPTION, "Phase B input current to VFD",
			PT_complex, "current_in_c[A]", PADDR(calc_current_in[2]), PT_DESCRIPTION, "Phase C input current to VFD",		
			
			PT_complex, "current_out_a[A]", PADDR(currentOut[0]), PT_DESCRIPTION, "Phase A output current of VFD",
			PT_complex, "current_out_b[A]", PADDR(currentOut[1]), PT_DESCRIPTION, "Phase B output current of VFD",
			PT_complex, "current_out_c[A]", PADDR(currentOut[2]), PT_DESCRIPTION, "Phase C output current of VFD",			
			
			PT_complex, "voltage_out_a[A]", PADDR(settleVoltOut[0]), PT_DESCRIPTION, "Phase A output voltage of VFD",
			PT_complex, "voltage_out_b[A]", PADDR(settleVoltOut[1]), PT_DESCRIPTION, "Phase B output voltage of VFD",
			PT_complex, "voltage_out_c[A]", PADDR(settleVoltOut[2]), PT_DESCRIPTION, "Phase C output voltage of VFD",
			
			PT_enumeration, "vfd_state", PADDR(vfdState), PT_DESCRIPTION, "Current state of the VFD",
				PT_KEYWORD, "STARTING", (enumeration)VFD_STARTING,
				PT_KEYWORD, "CHANGING", (enumeration)VFD_CHANGING,
				PT_KEYWORD, "STEADY_STATE", (enumeration)VFD_STEADY,
				PT_KEYWORD, "OFF", (enumeration)VFD_OFF,
			
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_vfd)==NULL)
			GL_THROW("Unable to publish vfd deltamode function");

		//Publish restoration-related function (current update)
		if (gl_publish_function(oclass,	"update_power_pwr_object", (FUNCTIONADDR)updatepowercalc_link)==NULL)
			GL_THROW("Unable to publish vfd external power calculation function");
		if (gl_publish_function(oclass,	"check_limits_pwr_object", (FUNCTIONADDR)calculate_overlimit_link)==NULL)
			GL_THROW("Unable to publish vfd external power limit calculation function");
		if (gl_publish_function(oclass,	"vfd_current_injection_update", (FUNCTIONADDR)current_injection_update_VFD)==NULL)
			GL_THROW("Unable to publish vfd external current injection calculation function");
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
	voltageLLRating = 480;
	horsePowerRatedVFD = 75;
	nominal_output_frequency = 60.0;
	oldDriveFrequency = 0;
	stableTime = 1.45;//5;//1.45;
	stableTimeOrig = stableTime;
	stableTimeDelta = stableTime;
	settleTime = 0;
	timeFlag = 0;
	
	//Null the array pointers, just because
	settleFreq = NULL;
	settleFreq_length = 0;
	vfdState = VFD_OFF;
	curr_array_position = 0;
	freqArray = NULL;
	force_array_realloc = false;
	
	//Zero others, even though they'll get overwritten
	nominal_output_radian_freq = 0.0;
	torqueRated = 0.0;
	prevDesiredFreq = 0.0;
	prev_current[0] = prev_current[1] = prev_current[2] = complex(0.0,0.0);
	
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
	fNode=OBJECTDATA(from,node);
	tNode=OBJECTDATA(to,node);
	OBJECT *obj = OBJECTHDR(this);
	FUNCTIONADDR temp_fxn;
	STATUS temp_status_val;

	int result = link_object::init(parent);

	//Any parameter checks/default checks would be here.  Things like "make sure stableTime is positive" and such.
	//e.g.
	if (stableTime < 0.0)
	{
		GL_THROW("vfd:%d %s - the stableTime must be positive",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The stableTime value for the VFD is negative.  This must be a positive value for the simulation to work
		properly.
		*/
	}
	else if (stableTime == 0.0)
	{
		gl_warning("vfd:%d %s - the stableTime is zero - this may cause odd behavior",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The stableTime value is set to zero, which can cause instantaneous changes in the VFD output.  This may result in larger
		transients than expected, as well as more severe dynamic behavior.
		*/
	}
	//stableTime = (int) (100*stableTime+0.5); //rounding to an int. *************************NIKHIL, UNCOMMENT THIS AFTER DEBUGGING*****************

	//set stabletime value
	if (deltatimestep_running < 0)
	{
		if ((stableTime - (int) stableTime) > 0.0)
		{
			stableTime = (int) (stableTime+1);
		}	
	}
	// else if (deltatimestep_running > 0)
	// {
		// stableTime = (int) stableTime/deltatimestep_running;
	// }
	stableTimeOrig = stableTime;
		
	//stayTime = (int) (0.0206*stableTime+0.5);
	//stayTime = roundf(0.0206*stableTime * 100)/100;
	//stayTime = (int) stayTime+0.5; //rounding to an int

	//Feeds in the current "stepping value", as well as assuming a global boolean is set
	STATUS status_val = alloc_freq_arrays(1.0);
	if (status_val == FAILED)
	{
		GL_THROW("vfd:%d %s - Allocating the dynamic arrays for the frequency tracking failed",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to allocate the dynamic length arrays for the vfd operation, an issue was encountered.  Please try again.  If the error
		persists, please submit an issue. */
		
	}
		

	//Default else, it must be a valid value
	
	//Other checks would go here
	
	
	//Create some constants - these will be locked
	nominal_output_radian_freq = nominal_output_frequency*2.0*PI;
	
	//Create the rated torque
	torqueRated = (horsePowerRatedVFD*5252)/ratedRPM;
	
	/******************** THIS ERROR NEEDS TO BE FIXED -- If Vout is similar to Vin, VLL will ALWAYS be higher  ******************/
	if (voltageLLRating > fNode->voltage[0].Mag())
	{
		GL_THROW("vfd: The rated_vfd_line_to_Line_voltage = %f must be less than or equal to input voltage (voltage_in_a) = %f to the vfd",voltageLLRating,fNode->voltage[0].Mag());
		/*  TROUBLESHOOT
		The VFD output voltage value is greater than the input voltage to VFD. The VFD output voltage must be less than or equal to VFD input voltage value for the simulation to work
		properly.
		*/
	}
	
	VbyF = voltageLLRating/nominal_output_frequency;  	//This is 7.6 v/hz for 460v motor
	HPbyF = 100/nominal_output_frequency; 				//max HP percentage/max frequency

	//Map our "TO" node as a proper VFD
	temp_fxn = (FUNCTIONADDR)(gl_get_function(to,"attach_vfd_to_pwr_object"));

	//Make sure it worked
	if (temp_fxn == NULL)
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
	
	return result;
}

void vfd::initialParameters()
{
	double actualTorque, driveVoltageReq, drivePowerReq, torz, z, driveCurrPower;
	
	driveFrequency    = (desiredRPM*motorPoles)/120; //calculate the frequency per the RPM value
	
	if ((driveFrequency > 6.67) && (driveFrequency < 6.67))
	{
		GL_THROW("Desired VFD Speed = %f should be greater than or equal to 200 RPM.", desiredRPM);
		/*  TROUBLESHOOT
		Starting frequency is 3Hz. Having a desired RPM at 200 would lead to ~6.6Hz of desired frequency. Although the VFD is not recommended to function at such low, the formulae will
		at least do what they are supposed to.
		*/
	}
	else if (driveFrequency == 0)
	{
		gl_warning("VFD is turned OFF");
		/*  TROUBLESHOOT
		This simply means VFD is turned OFF
		*/
	}
	if (desiredRPM/ratedRPM <=0.75)
	{
		gl_warning("current VFD performance = %f. VFDs perform best when running at >= 75 percent output", (desiredRPM*100/ratedRPM));
		/*  TROUBLESHOOT
		Recommended to run a VFD at 75% but if the user wants to run at less than 75%? Sure. But let the user know it is not a very good idea.
		*/
	}
	
	driveFrequency    = roundf(driveFrequency * 1000) / 1000; //rounding to third decimal. MATLAB equivalent is round(driveFrequency,3);
	driveCurrPower     = (HPbyF*driveFrequency); //Gives you the percentage of ratedHorsepower. To get the exact value of HP, use: drivePowerReq = (HPbyF*driveFrequency*horsePowerRated)/100;
	z = (driveCurrPower - 50.138)/37.009;
	
	currEfficiency = 3.497*z*z*z*z*z*z*z - 8.2828*z*z*z*z*z*z + 0.97848*z*z*z*z*z + 8.7113*z*z*z*z - 3.2079*z*z*z - 4.4504*z*z + 3.8759*z + 96.014; //For now we are using VFD 75HP equation for all

	if (driveFrequency <= nominal_output_frequency) //Till the drive frequency calculated based on the speed is less than the nominal frequency
	{
		actualTorque = torqueRated; //user defined or we find it?
		driveVoltageReq   = driveFrequency*VbyF;
		drivePowerReq     = (HPbyF*driveFrequency); //Gives you the percentage of ratedHorsepower. To get the exact value of HP, use: drivePowerReq = (HPbyF*driveFrequency*horsePowerRated)/100;
	}		
	else
	{
		gl_warning("VFD output frequency = %f > nominal frequency = %f. Variable Torque mode results may be incorrect",driveFrequency, nominal_output_frequency);
		/*  TROUBLESHOOT
		If the driveFrequency (which is nothing but the calculated VFD output frequency that is based on the desiredRPM) should be less than the nominal frequency for the VFD to function in
		CONSTANT TORQUE MODE. Suggested to run the VFD in CONSTANT TORQUE mode as the VARIABLE TORQUE mode is not fully tested. Please change your input property (sesiredRPPM) values to the VFD.
		*/
		
		torz         = (driveFrequency - 90)/23.717; //see comment next to below "actualTorque equation"
		actualTorque = -1.3176*torz*torz*torz+ 5.1786*torz*torz - 17.656*torz + 66.657; //Equation obtained from MATLAB curve fitting by plotting the Fig.1 data in http://www.pumpsandsystems.com/topics/pumps/motor-horsepower-torque-versus-vfd-frequency
		driveVoltageReq = voltageLLRating; //voltage is constant when the drive frequency is greater than nominal frequency.
		drivePowerReq = 100; //this means 100% of horsePowerRated
	}
}

void vfd::vfdCoreCalculations()
{
	/******************* COMMENTS FOR FRANK *********************
	Changes made on 3/27/2017 at 10:15AM:
	Added lines 275, lines 312 - 319; Commented lines 326 - 328
	If you want to run this as prior to 3/27/2017 at 10:15AM: Comment lines 275, lines 312 - 319; Uncommented lines 326 - 328
	*************************************************************/
	complex currRat, lossCurr[3];
	char index_val;
	complex temp_power_val;
	settleVolt = VbyF*currSetFreq; // since the speed didnot change, prevDesiredFreq would be equal to driveFrequency
	
	if (settleVolt < 0)
	{
		GL_THROW("Settling Voltage = %f should be positive", settleVolt);
		/*  TROUBLESHOOT
		This would almost never happen. But in case if it happens for what-so-ever reasons, we have a catch.
		*/
	}
	else if (settleVolt == 0)
	{
		gl_warning("VFD is turned OFF");
		/*  TROUBLESHOOT
		This simply means VFD is turned OFF.
		*/
	}
	if (settleTime <0)
	{
		GL_THROW("Settling Time = %f should be positive", settleTime);
		/*  TROUBLESHOOT
		This would almost never happen but if it does for reasons yet to know. Lets catch it.
		*/
	}
	else if (settleTime ==0)
	{
		gl_warning("Settling Time is 0. VFD must be turned OFF");
		/*  TROUBLESHOOT
		This simply means VFD is turned OFF.
		*/
	}
	
	//Update the VFD output
	phasorVal[0] = phasorVal[0] + ((2*PI*currSetFreq*settleTime) - (nominal_output_radian_freq*settleTime));
	phasorVal[1] = phasorVal[1] + ((2*PI*currSetFreq*settleTime) - (nominal_output_radian_freq*settleTime));
	phasorVal[2] = phasorVal[2] + ((2*PI*currSetFreq*settleTime) - (nominal_output_radian_freq*settleTime));

	//Temporary variable
	settleVoltOut[0] = complex(settleVolt,0)*complex_exp(phasorVal[0]);
	settleVoltOut[1] = complex(settleVolt,0)*complex_exp(phasorVal[1]);
	settleVoltOut[2] = complex(settleVolt,0)*complex_exp(phasorVal[2]);
	

	/******************* Probably can just use the "current_out" variable of link for this ***************************/
	/********** May need to think about the sequencing for this stuff **********************/
	currentOut[0] = tNode->current[0];
	currentOut[1] = tNode->current[1];
	currentOut[2] = tNode->current[2];

	//Accumulate the output power
	powerOutElectrical = complex(0.0,0.0);
	for (index_val=0; index_val<3; index_val++)
	{
		//Compute the phase power
		temp_power_val = (tNode->voltage[index_val]*~currentOut[index_val]);

		powerOutElectrical += temp_power_val;

		//For giggles, update our output power and current variables too
		indiv_power_out[index_val] = temp_power_val;
		current_out[index_val] = currentOut[index_val];
	}
	
	//Scale by the efficiency
	if (vfdState == VFD_OFF)
	{
		powerOutElectrical = complex(0.0,0.0);
	}
	
	powerInElectrical = powerOutElectrical*100.0/currEfficiency;
	powerLosses = powerInElectrical - powerOutElectrical;

	//Balance the power on the input, post it, and update the accumulator
	for (index_val=0; index_val<3; index_val++)
	{
		//Compute the amount
		calc_current_in[index_val] = ~(powerInElectrical/fNode->voltage[index_val]/3.0);

		/****************** DESIGN NOTE -- Maybe this should be posted to power instead??? *********************/
		//Post it
		fNode->current[index_val] += calc_current_in[index_val] - prev_current[index_val];

		//Update the tracking variable
		prev_current[index_val] = calc_current_in[index_val];

		//Push the new voltage out to the to node
		tNode->voltage[index_val] = settleVoltOut[index_val];

		//Update power values too, just because
		indiv_power_in[index_val] = powerInElectrical / 3.0;

		//And current input
		current_in[index_val] = calc_current_in[index_val];
	}
}

TIMESTAMP vfd::presync(TIMESTAMP t0)
{
	return link_object::presync(t0);
}

TIMESTAMP vfd::sync(TIMESTAMP t0)
{
	fNode=OBJECTDATA(from,node);
	tNode=OBJECTDATA(to,node);

	curr_time_value = (double)t0;
	
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2;

	if (t0 == 946702850)
		t2 = TS_NEVER;
	
	t2 = link_object::sync(t0);
	//Put VFD logic function here
	
	return TS_NEVER;
}

TIMESTAMP vfd::postsync(TIMESTAMP t0)
{
	OBJECT *hdr = OBJECTHDR(this);
	TIMESTAMP t1;

	prev_time_value = curr_time_value;
	oldDriveFrequency = driveFrequency;
	//Normal link update
	t1 = link_object::postsync(t0);
	
		
	return t1;
}

//Function to perform exp(j*val)
//Basically a complex rotation
complex vfd::complex_exp(double angle)
{
	complex output_val;

	//exp(jx) = cos(x)+j*sin(x)
	output_val = complex(cos(angle),sin(angle));

	return output_val;
}

//Function to reallocate/allocate variable time arrays
STATUS vfd::alloc_freq_arrays(double delta_t_val)
{
	OBJECT *obj = OBJECTHDR(this);
	int a_index;
	
	//See if we were commanded to reallocate -- this would be done on a zero-th pass of interupdate, most likely (or in postupdate, when transitioning out)
	if (force_array_realloc == true)
	{
		//Make sure we actually are allocated first
		if (settleFreq != NULL)
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
		settleFreq = NULL;
	}
	
	//See if we need to be allocated
	if (settleFreq == NULL)
	{
		//Determine the size of our array
		settleFreq_length = (int)(stableTime/delta_t_val + 0.5);
		
		//Make sure it is valid
		if (settleFreq_length <= 0) //****************************Nikhil changed settleFreq to settleFreq_length. Correct?************************************//
		{
			gl_error("vfd:%d %s -- the stable_time value must result in at least 1 timestep of averaging!",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*TROUBLESHOOT
			While attempting to allocate the averaging arrays, an invalid length was specified.  Make sure your stable_time value is at least one second
			or one delta-timestep long. */
			
			
			return FAILED;
		}
		
		//Allocate it
		settleFreq = (double *)gl_malloc(settleFreq_length*sizeof(double));
		
		//Make sure it worked
		if (settleFreq == NULL)
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
		
		//Success
		return SUCCESS;
	}
	else //Already done and we weren't forced, so just assume we were called erroneously
	{
		return SUCCESS;
	}
}

//Function to perform the current update - called by a node object, after they've updated (so current is accurate)
STATUS vfd::VFD_current_injection(void)
{
	//Call any current-injection update functions here
	//This gets called near the end of SYNC, after the TO object has done it's current update
	//May need to "undo" this in POSTSYNC
	
	//Put VFD logic function here
	int temp_idx;
	double meanFreqArray, startFrequency;
	complex tempVal;
	
	tempVal = tNode->current[0];
	
	phasorVal[0] = 0;
	phasorVal[1] = (2*PI)/3;
	phasorVal[2] = -(2*PI)/3;

	initialParameters();
	
	if ((deltatimestep_running > 0) && (stableTime == stableTimeOrig) && (timeFlag == 0))
	{
		stableTime = (int) stableTime/deltatimestep_running;
		stableTimeOrig = stableTime;
		timeFlag = 1;
	}
	
	if (oldDriveFrequency != driveFrequency)
	{
		stableTime = stableTimeOrig;
	}
	
	if (stableTimeOrig == stableTime)
	{
		if (deltatimestep_running > 0)
		{
			desiredFinalTime = curr_time_value+stableTimeDelta;	
		}
		else
		{
			desiredFinalTime = curr_time_value+stableTimeOrig;		
		}
	}
		
	if ((prevDesiredFreq == 0.0) && (driveFrequency!=0))
	{
		vfdState = VFD_STARTING; //starting state
		startFrequency = 3;
		//settleTime = 1;
	}
	else if ((prevDesiredFreq != 0.0) && (driveFrequency != 0.0) && (prevDesiredFreq != driveFrequency))
	{
		vfdState = VFD_CHANGING; //speed change state
		startFrequency = prevDesiredFreq; //Now that the frequency is changed, the starting frequency would be the exact last instance of previous stable frequency
	
		if (prevDesiredFreq <=0.0)
		{
			GL_THROW("At this point, Previous frequency = %f should be positive", prevDesiredFreq);
			/*  TROUBLESHOOT
			This would almost never happen but if it does for reasons yet to know. Lets catch it.
			*/
		}
		
	}
	else if ((prevDesiredFreq != 0.0) && (prevDesiredFreq == driveFrequency)) //In case the drive frequency doesnt change (i.e., the speed is constant/doesn't change)
	{
		vfdState = VFD_STEADY; //steady state
		//settleTime.push_back(settleTime.back()+1);// MATLAB equivalent is settleTime = [settleTime settleTime(end)+1]; %increment the time by 1 second - remember this simulation time is in seconds?
		
		if ((prevDesiredFreq <=0.0)|| (driveFrequency <= 0))
		{
			GL_THROW("VFD's previous frequency = %f and VFD's current Frequency = %f must be positive", prevDesiredFreq, driveFrequency);
			/*  TROUBLESHOOT
			This would almost never happen but if it does for reasons yet to know. Lets catch it.
			*/
		}
		prevDesiredFreq = driveFrequency;
		currSetFreq = driveFrequency;
		vfdCoreCalculations();
		if (deltatimestep_running < 0)
		{
			settleTime = settleTime+1;
		}
		else
		{
			settleTime = settleTime+deltatimestep_running;
		}
	}
	else if (driveFrequency==0)
	{
		vfdState = VFD_OFF; //OFF State
		//startFrequency = prevDesiredFreq; //Now that the frequency is changed, the starting frequency would be the exact last instance of previous stable frequency
	
		if (prevDesiredFreq <0.0)
		{
			GL_THROW("Previous frequency = %f should be 0 in OFF state", prevDesiredFreq);
			/*  TROUBLESHOOT
			This would almost never happen but if it does for reasons yet to know. Lets catch it.
			*/
		}
		prevDesiredFreq = driveFrequency;
		currSetFreq = driveFrequency;
		vfdCoreCalculations();
		if (deltatimestep_running < 0)
		{
			settleTime = 0;
		}
		else
		{
			settleTime = deltatimestep_running;
		}		
	}
	
	if ((vfdState == VFD_STARTING) || (vfdState == VFD_CHANGING)) //VFD started or changing speed.
	{
		if (curr_time_value <= desiredFinalTime)
		{
			if (stableTimeOrig == stableTime)
			{
				for (curr_array_position = 0; curr_array_position <stableTime; curr_array_position++)
				{
					settleFreq[curr_array_position] = startFrequency;
				}	
				settleFreq[0] = driveFrequency;
			}
			else if (stableTimeOrig > stableTime)
			{
				for (curr_array_position = 0; curr_array_position <stableTimeOrig-stableTime; curr_array_position++)
				{
					settleFreq[curr_array_position] = driveFrequency;
				}
			}
			
			//Zero the accumulator
			meanFreqArray = 0.0;

			for (temp_idx=0; temp_idx<stableTimeOrig; temp_idx++)
			{
				meanFreqArray += settleFreq[temp_idx];
			}
			meanFreqArray /= (double)stableTimeOrig;

			currSetFreq = roundf(meanFreqArray* 1000) / 1000;
			
			if (settleTime == 0) //since we are initiating at 0. at the very beginning start from 1st timevalue.
			{
				if (deltatimestep_running < 0)
				{
					settleTime = 1;
				}
				else
				{
					settleTime = deltatimestep_running;
				}
			}
			
			vfdCoreCalculations();
			if (deltatimestep_running < 0)
			{
				settleTime = settleTime+1;
			}
			else
			{
				settleTime = settleTime+deltatimestep_running;
			}
			meanFreqArray = 0;

			if (curr_time_value != prev_time_value)
			{
				stableTime = stableTime-1;
			}

			if (curr_time_value == desiredFinalTime)
			{
				prevDesiredFreq = currSetFreq;//settleFreq[curr_array_position];
				stableTime = stableTimeOrig;
			}
		}
		
		
	/*
		for (curr_array_position = 0; curr_array_position <stableTime; curr_array_position++)
		{
			settleFreq[curr_array_position] = startFrequency;
		}
		
		for (curr_array_position = 0; curr_array_position <stableTime; curr_array_position++)
		{
			settleFreq[curr_array_position] = driveFrequency;
			for (temp_idx=0; temp_idx<stableTime; temp_idx++)
			{
				meanFreqArray += settleFreq[temp_idx];
			}
			meanFreqArray /= (double)stableTime;
			
			//settleFreq[curr_array_position] = roundf(meanFreqArray* 1000) / 1000;
			//currSetFreq = settleFreq[curr_array_position];
			
			currSetFreq = roundf(meanFreqArray* 1000) / 1000;
			settleTime = settleTime+1;
		
			vfdCoreCalculations();
			meanFreqArray = 0;
		}
		prevDesiredFreq = currSetFreq;//settleFreq[curr_array_position];
		settleFreq = NULL;
	*/
	}	

	return SUCCESS;	//Change this, if there's a chance of failure
}

//Module-level deltamode call
SIMULATIONMODE vfd::inter_deltaupdate_vfd(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	double dt_value, deltatimedbl;
	STATUS ret_value;

	//See if we're the very first pass/etc
	if ((delta_time == 0) && (iteration_count_val == 0) && (interupdate_pos == false))	//First deltamode call
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
	if (interupdate_pos == false)	//Before powerflow call
	{
		//Link presync stuff
		NR_link_presync_fxn();

		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}
	else	//After the call
	{
		//Update time tracking variable - is postsync in the main code, so putting it in an equivalent place here
		if (iteration_count_val==0)	//Only update timestamp tracker on first iteration
		{
			//Get decimal timestamp value
			deltatimedbl = (double)delta_time/(double)DT_SECOND;

			//Update tracking variable
			prev_time_value = (double)gl_globalclock + deltatimedbl;
		}

		//Call postsync
		BOTH_link_postsync_fxn();

		/********** Sri Nikhil -- how will VFDs want to behave with deltamode and time handling **********/
		return SM_EVENT;	//VFD always just want out
	}
}//End module deltamode

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
		if (*obj!=NULL)
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
		gl_error("interupdate_link(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

/**@}*/

