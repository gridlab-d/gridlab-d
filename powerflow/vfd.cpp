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
EXPORT_SYNC(vfd);
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
		
		oclass = gl_register_class(mod,"vfd",sizeof(vfd),PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
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
			PT_double, "Desired_vfd_rpm", PADDR(desiredRPM), PT_DESCRIPTION, "Desired speed of the VFD In ROM. Default = 900 RPM",
			PT_double, "rated_vfd_horse_power [hp]", PADDR(horsePowerRatedVFD), PT_DESCRIPTION, "Rated Horse Power of the VFD. Default = 75 HP",
			PT_double, "nominal_output_frequency[Hz]", PADDR(nominal_output_frequency), PT_DESCRIPTION, "Nominal VFD output frequency. Default = 60 Hz",
			
			PT_double, "drive_frequency [Hz]", PADDR(driveFrequency), PT_DESCRIPTION, "Current VFD frequency based on the desired RPM",
			PT_double, "vfd_efficiency", PADDR(currEfficiency), PT_DESCRIPTION, "Current VFD efficiency based on the load/VFD output Horsepower",			
			
			PT_double, "stable_time [s]", PADDR(stableTime), PT_DESCRIPTION, "Time taken by the VFD to reach desired frequency (based on RPM). Default = 1.45 seconds",
			PT_double, "settle_time", PADDR(settleTime), PT_DESCRIPTION, "Total number of steps/counts during the VFD operation.",
			PT_double, "power_out_electrical [W]", PADDR(powerOutElectrical), PT_DESCRIPTION, "VFD output electrical power",
			PT_double, "power_losses [W]", PADDR(powerLosses), PT_DESCRIPTION, "VFD electrical power losses",
			PT_double, "power_in_electrical [W]", PADDR(powerInElectrical), PT_DESCRIPTION, "Input electrical power to VFD",
			
			PT_complex, "current_in_a [A]", PADDR(calc_current_in[0]), PT_DESCRIPTION, "Phase A input current to VFD",
			PT_complex, "current_in_b [A]", PADDR(calc_current_in[1]), PT_DESCRIPTION, "Phase B input current to VFD",
			PT_complex, "current_in_c [A]", PADDR(calc_current_in[2]), PT_DESCRIPTION, "Phase C input current to VFD",
			
			PT_complex, "voltage_in_a [A]", PADDR(calc_volt_in[0]), PT_DESCRIPTION, "Phase A input voltage to VFD",
			PT_complex, "voltage_in_b [A]", PADDR(calc_volt_in[1]), PT_DESCRIPTION, "Phase B input voltage to VFD",
			PT_complex, "voltage_in_c [A]", PADDR(calc_volt_in[2]), PT_DESCRIPTION, "Phase C input voltage to VFD",			
			
			PT_complex, "current_out_a [A]", PADDR(currentOut[0]), PT_DESCRIPTION, "Phase A output current of VFD",
			PT_complex, "current_out_b [A]", PADDR(currentOut[1]), PT_DESCRIPTION, "Phase B output current of VFD",
			PT_complex, "current_out_c [A]", PADDR(currentOut[2]), PT_DESCRIPTION, "Phase C output current of VFD",			
			
			PT_complex, "voltage_out_a [A]", PADDR(settleVoltOut[0]), PT_DESCRIPTION, "Phase A output voltage of VFD",
			PT_complex, "voltage_out_b [A]", PADDR(settleVoltOut[1]), PT_DESCRIPTION, "Phase B output voltage of VFD",
			PT_complex, "voltage_out_c [A]", PADDR(settleVoltOut[2]), PT_DESCRIPTION, "Phase C output voltage of VFD",				
			
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_link)==NULL)
			GL_THROW("Unable to publish vfd deltamode function");

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
	desiredRPM = 900;
	motorPoles = 4;
	voltageLLRating = 480;
	horsePowerRatedVFD = 75;
	nominal_output_frequency = 60.0;
	stableTime = 1.45;
	
	//Null the array pointers, just because
	settleFreq = NULL;
	settleFreq_length = 0;
	vfdState = -1;
	curr_array_position = 0;
	freqArray = NULL;
	force_array_realloc = false;
	
	//Zero others, even though they'll get overwritten
	nominal_output_radian_freq = 0.0;
	torqueRated = 0.0;
	prevDesiredFreq = 0.0;
	prev_current[0] = prev_current[1] = prev_current[2] = complex(0.0,0.0);
	
	
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
	node *fNode;
	node *tNode;
	fNode=OBJECTDATA(from,node);
	tNode=OBJECTDATA(to,node);
	OBJECT *obj = OBJECTHDR(this);

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
	stableTime = (int) (100*stableTime+0.5); //rounding to an int
	stayTime = (int) (0.0206*stableTime+0.5);
	//stayTime = roundf(0.0206*stableTime * 100)/100;
	//stayTime = (int) stayTime+0.5; //rounding to an int

	//Feeds in the current "stepping value", as well as assuming a global boolean is set
	int status_val = alloc_freq_arrays(1.0);
	if (status_val == 0)
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
	
	if (voltageLLRating > fNode->voltage[0].Mag())
	{
		GL_THROW("vfd: The rated_vfd_line_to_Line_voltage = %d must be less than or equal to input voltage (voltage_in_a) = %d to the vfd",voltageLLRating,fNode->voltage[0].Mag());
		/*  TROUBLESHOOT
		The VFD output voltage value is greater than the input voltage to VFD. The VFD output voltage must be less than or equal to VFD input voltage value for the simulation to work
		properly.
		*/
	}
	
	VbyF = voltageLLRating/nominal_output_frequency;  	//This is 7.6 v/hz for 460v motor
	HPbyF = 100/nominal_output_frequency; 				//max HP percentage/max frequency

	
	return result;
}

void vfd::initialParameters()
{
	double actualTorque, driveVoltageReq, drivePowerReq, torz, z, driveCurrPower;
	
	driveFrequency    = (desiredRPM*motorPoles)/120; //calculate the frequency per the RPM value
	
	if (driveFrequency < 6.67)
	{
		GL_THROW("Desired VFD Speed = %f should be greated than or equal to 200 RPM.", desiredRPM);
		/*  TROUBLESHOOT
		Starting frequency is 3Hz. Having a desired RPM at 200 would lead to ~6.6Hz of desired frequency. Although the VFD is not recommended to function at such low, the formulae will
		at least do what they are supposed to.
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
		gl_warning("VFD output frequency = %d > nominal frequency = %d. Variable Torque mode results may be incorrect",driveFrequency, nominal_output_frequency);
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

void vfd::vfdCoreCalculations(double currFrequencyVal, double settleTime, double phasorVal[3], node *tNode, node *fNode, double currEfficiency, complex prev_current[3])
{
	settleVolt = VbyF*currFrequencyVal; // since the speed didnot change, prevDesiredFreq would be equal to driveFrequency
	
	if (settleVolt <= 0)
	{
		GL_THROW("Settling Voltage = %d should be positive", settleVolt);
		/*  TROUBLESHOOT
		This would almost never happen. But in case if it happens for what-so-ever reasons, we have a catch.
		*/
	}
	if (settleTime <=0)
	{
		GL_THROW("Settling Time = %d should be positive", settleTime);
		/*  TROUBLESHOOT
		This would almost never happen but if it does for reasons yet to know. Lets catch it.
		*/
	}
	
	phasorVal[0] = phasorVal[0] + ((2*PI*currFrequencyVal*settleTime) - (nominal_output_radian_freq*settleTime));
	phasorVal[1] = phasorVal[1] + ((2*PI*currFrequencyVal*settleTime) - (nominal_output_radian_freq*settleTime));
	phasorVal[2] = phasorVal[2] + ((2*PI*currFrequencyVal*settleTime) - (nominal_output_radian_freq*settleTime));

	settleVoltOut[0] = complex(settleVolt,0)*complex_exp(phasorVal[0]);
	settleVoltOut[1] = complex(settleVolt,0)*complex_exp(phasorVal[1]);
	settleVoltOut[2] = complex(settleVolt,0)*complex_exp(phasorVal[2]);
	

	/******************* Probably can just use the "current_out" variable of link for this ***************************/
	/********** May need to think about the sequencing for this stuff **********************/
	currentOut[0] = tNode->current[0];
	currentOut[1] = tNode->current[1];
	currentOut[2] = tNode->current[2];

	powerOutElectrical = settleVoltOut[0]*~currentOut[0];	
	powerLosses = (powerOutElectrical*(100-currEfficiency))/currEfficiency;
	powerInElectrical = powerOutElectrical + powerLosses;

	/************* May have to think about this one -- also note, these are accumulators **********************/
	/*** Added a divide by three here, since you're splitting it over three phases *********/
	/**** Note - changed this -- P = VI*, so I = (P/V)*   *************/
	
	//Calculate current current
	calc_current_in[0] = ~(powerInElectrical/fNode->voltage[0]/3.0);
	calc_current_in[1] = ~(powerInElectrical/fNode->voltage[1]/3.0);
	calc_current_in[2] = ~(powerInElectrical/fNode->voltage[2]/3.0);
	
	//Add to the load accumulator -- note, this could probably be done as a power load too
	fNode->current[0] += calc_current_in[0] - prev_current[0];
	fNode->current[1] += calc_current_in[1] - prev_current[1];
	fNode->current[2] += calc_current_in[2] - prev_current[2];
	
	calc_current_in[0] = fNode->current[0];
	calc_current_in[1] = fNode->current[1];
	calc_current_in[2] = fNode->current[2];
	
	calc_volt_in[0] = fNode->voltage[0]/3.0;//powerInElectrical/~(fNode->current[0]);
	calc_volt_in[1] = fNode->voltage[1]/3.0;//powerInElectrical/~(fNode->current[0]);
	calc_volt_in[2] = fNode->voltage[2]/3.0;//powerInElectrical/~(fNode->current[0]);
}

TIMESTAMP vfd::sync(TIMESTAMP t0)
{
	node *fNode;
	node *tNode;
	fNode=OBJECTDATA(from,node);
	tNode=OBJECTDATA(to,node);

	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2;
	
	t2 = link_object::sync(t0);
	
	//Put VFD logic function here
	int temp_idx;
	double meanFreqArray, startFrequency, currSetFreq;
	
	phasorVal[0] = 0;
	phasorVal[1] = (2*PI)/3;
	phasorVal[2] = -(2*PI)/3;
	
	initialParameters();
	
	/*
	vfdState = 0      ===> VFD Started - starting state 
			 = 1      ===> Steady state
			 = 2      ===> VFD is running - speed change state
	*/
		
	if (prevDesiredFreq == 0.0)
	{
		vfdState = 0; //starting state
		startFrequency = 3;
		settleTime = 0;
	}
	else if ((prevDesiredFreq != 0.0) && (prevDesiredFreq != driveFrequency))
	{
		vfdState = 1; //speed change state
		startFrequency = prevDesiredFreq; //Now that the frequency is changed, the starting frequency would be the exact last instance of previous stable frequency
	
		if (prevDesiredFreq <=0.0)
		{
			GL_THROW("At this point, Previous frequency = %d should be positive", prevDesiredFreq);
			/*  TROUBLESHOOT
			This would almost never happen but if it does for reasons yet to know. Lets catch it.
			*/
		}
		
	}
	else if ((prevDesiredFreq != 0.0) && (prevDesiredFreq == driveFrequency)) //In case the drive frequency doesnt change (i.e., the speed is constant/doesn't change)
	{
		vfdState = 2; //steady state
		settleTime = settleTime+1;			
		//settleTime.push_back(settleTime.back()+1);// MATLAB equivalent is settleTime = [settleTime settleTime(end)+1]; %increment the time by 1 second - remember this simulation time is in seconds?
		
		if ((prevDesiredFreq <=0.0)|| (driveFrequency <= 0))
		{
			GL_THROW("VFD's previous frequency = %d and VFD's current Frequency = %d must be positive", prevDesiredFreq, driveFrequency);
			/*  TROUBLESHOOT
			This would almost never happen but if it does for reasons yet to know. Lets catch it.
			*/
		}
		prevDesiredFreq = driveFrequency;
		vfdCoreCalculations(driveFrequency, settleTime, phasorVal, tNode, fNode, currEfficiency, prev_current);

		//Update the tracking variables
		prev_current[0] = calc_current_in[0];
		prev_current[1] = calc_current_in[1];
		prev_current[2] = calc_current_in[2];
	}
	
	if ((vfdState == 0) || (vfdState == 1)) //VFD started or changing speed.
	{
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
			settleFreq[curr_array_position] = roundf(meanFreqArray* 1000) / 1000;
			settleTime = settleTime+1;
			currSetFreq = settleFreq[curr_array_position];
			vfdCoreCalculations(currSetFreq, settleTime, phasorVal, tNode, fNode, currEfficiency, prev_current);
			prev_current[0] = calc_current_in[0];
			prev_current[1] = calc_current_in[1];
			prev_current[2] = calc_current_in[2];
		}
		prevDesiredFreq = settleFreq[curr_array_position];
		settleFreq = NULL;
	}	
	
	return TS_NEVER;
}

TIMESTAMP vfd::postsync(TIMESTAMP t0)
{
	OBJECT *hdr = OBJECTHDR(this);
	TIMESTAMP t1;

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

int vfd::alloc_freq_arrays(double delta_t_val)
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
			
			
			return 0;
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
			
			
			return 0;
		}
		
		//Zero it, just for giggles/paranoia
		for (a_index=0; a_index<settleFreq_length; a_index++)
		{
			settleFreq[a_index] = 0.0;
		}
		
		//Success
		return 1;
	}
	else //Already done and we weren't forced, so just assume we were called erroneously
	{
		return 1;
	}
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

EXPORT int init_vfd(OBJECT *obj)
{
	try {
		vfd *my = OBJECTDATA(obj,vfd);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(vfd);
}

//Commit timestep - after all iterations are done
/*
EXPORT TIMESTAMP commit_vfd(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	vfd *fsr = OBJECTDATA(obj,vfd);
	try
	{
		if (solver_method==SM_FBS)
		{
			link_object *plink = OBJECTDATA(obj,link_object);
			plink->calculate_power();
			
			return (fsr->vfd_state(obj->parent) ? TS_NEVER : 0);
		}
		else
			return TS_NEVER;
	}
	catch (const char *msg)
	{
		gl_error("%s (vfd:%d): %s", fsr->get_name(), fsr->get_id(), msg);
		return 0; 
	}
}
*/

// EXPORT TIMESTAMP sync_vfd(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
// {
	// try {
		// vfd *pObj = OBJECTDATA(obj,vfd);
		// TIMESTAMP t1 = TS_NEVER;
		// switch (pass) {
		// case PC_PRETOPDOWN:
			// return pObj->presync(t0);
		// case PC_BOTTOMUP:
			// return pObj->sync(t0);
		// case PC_POSTTOPDOWN:
			// t1 = pObj->postsync(t0);
			// obj->clock = t0;
			// return t1;
		// default:
			// throw "invalid pass request";
		// }
	// }
	// SYNC_CATCHALL(vfd);
// }

/**
* Allows the core to discover whether obj is a subtype of this class.
*
* @param obj a pointer to this object
* @param classname the name of the object the core is testing
*
* @return true (1) if obj is a subtype of this class
*/
EXPORT int isa_vfd(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,vfd)->isa(classname);
}

/**@}*/

