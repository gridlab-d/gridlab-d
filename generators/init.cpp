// $Id$
// Copyright (C) 2008 Battelle Memorial Institute

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"

#define _GENERATORS_GLOBALS
#include "generators.h"
#undef  _GENERATORS_GLOBALS

//Define defaults, since many use them and they aren't here yet
complex default_line_voltage[3] = {complex(480.0,0.0),complex(-240.0,-415.69219),complex(-240.0,415.69219)};
complex default_line_voltage_triplex[3] = {complex(240,0,A),complex(120,0,A),complex(120,0,A)};
complex default_line_current[3] = {complex(0,0,J),complex(0,0,J),complex(0,0,J)};
complex default_line_shunt[3] = {complex(0,0,J),complex(0,0,J),complex(0,0,J)};
complex default_line_power[3] = {complex(0,0,J),complex(0,0,J),complex(0,0,J)};
int default_meter_status = 1;	//In service

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	/* Publish external global variables */
	gl_global_create("generators::enable_subsecond_models", PT_bool, &enable_subsecond_models,PT_DESCRIPTION,"Enable deltamode capabilities within the powerflow module",NULL);
	gl_global_create("generators::deltamode_timestep", PT_double, &deltamode_timestep_publish,PT_UNITS,"ns",PT_DESCRIPTION,"Desired minimum timestep for deltamode-related simulations",NULL);

	CLASS *first =
	/*** DO NOT EDIT NEXT LINE ***/
	//NEWCLASS
	(new diesel_dg(module))->oclass;
	NULL;

	(new windturb_dg(module))->oclass;
	NULL;

	(new power_electronics(module))->oclass;
	NULL;

	(new energy_storage(module))->oclass;
	NULL;

	(new inverter(module))->oclass;
	NULL;

	(new dc_dc_converter(module))->oclass;
	NULL;

	(new rectifier(module))->oclass;
	NULL;

	(new microturbine(module))->oclass;
	NULL;

	(new battery(module))->oclass;
	NULL;

	(new solar(module))->oclass;
	NULL;

	(new central_dg_control(module))->oclass;
	NULL;

	/* always return the first class registered */
	return first;
}

//Call function for scheduling deltamode
void schedule_deltamode_start(TIMESTAMP tstart)
{
	if (enable_subsecond_models == true)	//Make sure the overall mode is enabled
	{
		if ( (tstart<deltamode_starttime) && ((tstart-gl_globalclock)<0x7fffffff )) // cannot exceed 31 bit integer
			deltamode_starttime = tstart;	//Smaller and valid, store it
	}
	else
	{
		GL_THROW("generators: a call was made to deltamode functions, but subsecond models are not enabled!");
		/*  TROUBLESHOOT
		The schedule_deltamode_start function was called by an object when generators' overall enabled_subsecond_models
		flag was not set.  The module-level flag indicates that no devices should use deltamode, but one made the call
		to a deltamode function.  Please check the DELTAMODE flag on all objects.  If deltamode is desired,
		please set the module-level enable_subsecond_models flag and try again.
		*/
	}
}

//deltamode_desired function
//Module-level call to determine when the next object expects
//to enter deltamode, even if it is now.
//Returns time to next deltamode in integers seconds.
//i.e., 0 = deltamode now, 1 = deltamode 1 second from the current simulation time
//Return DT_INFINITY for no deltamode desired
EXPORT unsigned long deltamode_desired(int *flags)
{
	unsigned long dt_val;

	if (enable_subsecond_models == true)	//Make sure we even want to run deltamode
	{
		//See if the value is worth storing, or irrelevant at this time
		if ((deltamode_starttime>=gl_globalclock) && (deltamode_starttime<TS_NEVER))
		{
			//Set the flag to do a soft deltamode transition
			*flags |= DMF_SOFTEVENT;

			//Compute the difference to get the incremental time needed
			dt_val = (unsigned long)(deltamode_starttime - gl_globalclock);

			gl_debug("generators: deltamode desired in %d", dt_val);
			return dt_val;
		}
		else
		{
			//No scheduled deltamode, or it wasn't a valid value
			return DT_INFINITY;
		}
	}
	else
	{
		return DT_INFINITY;
	}
}

//preudpate function of deltamode
//Module-level call at beginning of each deltamode simulation
//Returns the timestep this module requires - to be used to determine minimimum
//deltamode simulation stepsize
EXPORT unsigned long preupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
{
	if (enable_subsecond_models == true)
	{
		if (deltamode_timestep_publish<=0.0)
		{
			gl_error("generators::deltamode_timestep must be a positive, non-zero number!");
			/*  TROUBLESHOOT
			The value for deltamode_timestep, as specified as the module level in generators, must be a positive, non-zero number.
			Please use such a number and try again.
			*/

			return DT_INVALID;
		}
		else
		{
			//Cast in the published value
			deltamode_timestep = (unsigned long)(deltamode_timestep_publish+0.5);

			//Return it
			return deltamode_timestep;
		}
	}
	else	//Not desired, just return an arbitrarily large value
	{
		return DT_INFINITY;
	}
}

//interupdate function of deltamode
//Module-level call for each timestep of deltamode
//Ideally, all deltamode objects coordinate through their module call, not their individual "update" call
//Returns SIMULATIONMODE - SM_DELTA, SM_DELTA_ITER, SM_EVENT, or SM_ERROR
EXPORT SIMULATIONMODE interupdate(MODULE *module, TIMESTAMP t0, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	int curr_object_number;
	SIMULATIONMODE function_status = SM_EVENT;
	bool event_driven = true;
	bool delta_iter = false;
	
	if (enable_subsecond_models == true)
	{
		//Loop through the object list and call the updates
		for (curr_object_number=0; curr_object_number<gen_object_count; curr_object_number++)
		{
			//See if we're in service or not
			if ((delta_objects[curr_object_number]->in_svc_double <= gl_globaldeltaclock) && (delta_objects[curr_object_number]->out_svc_double >= gl_globaldeltaclock))
			{
				//Call the actual function
				function_status = ((SIMULATIONMODE (*)(OBJECT *, unsigned int64, unsigned long, unsigned int))(*delta_functions[curr_object_number]))(delta_objects[curr_object_number],delta_time,dt,iteration_count_val);
			}
			else //Not in service - off to event mode
				function_status = SM_EVENT;

			//Determine what our return is
			if (function_status == SM_DELTA)
				event_driven = false;
			else if (function_status == SM_DELTA_ITER)
			{
				event_driven = false;
				delta_iter = true;
			}
			else if (function_status == SM_ERROR)
			{
				gl_error("Generator object:%s - deltamode function returned an error!",delta_objects[curr_object_number]->name);
				/*  TROUBLESHOOT
				While performing a deltamode update, one object returned an error code.  Check to see if the object itself provided
				more details and try again.  If the error persists, please submit your code and a bug report via the trac website.
				*/
				return SM_ERROR;
			}
			//Default else, we're in SM_EVENT, so no flag change needed
		}
				
		//Determine how to exit - event or delta driven
		if (event_driven == false)
		{
			if (delta_iter == true)
				return SM_DELTA_ITER;
			else
				return SM_DELTA;
		}
		else
			return SM_EVENT;
	}
	else	//deltamode not desired
	{
		return SM_EVENT;	//Just event mode
	}
}

//postupdate function of deltamode
//Executes after all objects in the simulation agree to go back to event-driven mode
//Return value is a SUCCESS/FAILURE
EXPORT STATUS postupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
{
	unsigned int64 seconds_advance, temp_time;
	int curr_object_number;
	STATUS function_status;
	int64 *func_address;
	int pflow_func_status;
	complex temp_complex;
	double *extracted_freq;

	if (enable_subsecond_models == true)
	{
		//Final item of transitioning out is resetting the next timestep so a smaller one can take its place
		deltamode_starttime = TS_NEVER;

		//See how far we progressed - cast just in case (code pulled from core - so should align)
		seconds_advance = (unsigned int64)(dt/DT_SECOND);
	
		//See if it rounded
		temp_time = dt - ((unsigned int64)(seconds_advance)*DT_SECOND);

		//Store the "pre-incremented" seconds advance time - used to set object clocks
		//so being the previous second is better for deltamode->supersecond transitions
		deltamode_supersec_endtime = t0 + seconds_advance;

		/* Determine if an increment is necessary */
		if (temp_time != 0)
			seconds_advance++;

		//Update the tracking variable
		deltamode_endtime = t0 + seconds_advance;
		deltamode_endtime_dbl = (double)(t0) + ((double)(dt)/double(DT_SECOND));

		//Loop through delta objects and post their "post transient" frequency values - 0 mode accumulation pass
		for (curr_object_number=0; curr_object_number<gen_object_count; curr_object_number++)
		{

			//See if we're in service or not
			if ((delta_objects[curr_object_number]->in_svc_double <= gl_globaldeltaclock) && (delta_objects[curr_object_number]->out_svc_double >= gl_globaldeltaclock))
			{
				//Call the actual function
				function_status = ((STATUS (*)(OBJECT *, complex *, unsigned int))(*post_delta_functions[curr_object_number]))(delta_objects[curr_object_number],&temp_complex,0);
			}
			else //Not in service
				function_status = SUCCESS;

			//Make sure we worked
			if (function_status == FAILED)
			{
				gl_error("Generator object:%s - failed post-deltamode update",delta_objects[curr_object_number]->name);
				/*  TROUBLESHOOT
				While calling the individual object post-deltamode calculations, an error occurred.  Please try again.
				If the error persists, please submit your code and a bug report via the trac website.
				*/
				return FAILED;
			}
			//Default else - successful, keep going
		}

		//Get the function address (address of the global that contains it)
		func_address = (int64 *)gl_get_module_var(gl_find_module("powerflow"),"deltamode_extra_function");

		//Make sure it isn't empty
		if (func_address == NULL)
		{
			gl_error("Generator deltamode update - failed to link to powerflow frequency");
			/*  TROUBLESHOOT
			While performing a deltamode update, mapping the current powerflow frequency value failed.
			Please try again.  If the error persists, please submit a bug report via the trac website.
			*/
			return FAILED;
		}

		//Semi-legacy call -- see if it actually populated, if not, skip it
		if (*func_address != NULL)
		{
			//Call the function now
			pflow_func_status = ((int (*)(unsigned int))((FUNCTIONADDR *)(*func_address)))(0);

			//Make sure it worked
			if (pflow_func_status == 0)
			{
				gl_error("Generator deltamode update - failed to perform powerflow frequency update");
				/*  TROUBLESHOOT
				While performing a deltamode update, the frequency calculation portion in the powerflow
				module failed to calculate properly.
				*/
				return FAILED;
			}

			//Now get the "current_frequency" value and push it back
			extracted_freq = (double *)gl_get_module_var(gl_find_module("powerflow"),"current_frequency");

			//Make sure it worked
			if (extracted_freq == 0)
			{
				gl_error("Generator deltamode update - failed to link to powerflow frequency");
				/*  TROUBLESHOOT
				While performing a deltamode update, mapping the current powerflow frequency value failed.
				Please try again.  If the error persists, please submit a bug report via the trac website.
				*/
				return FAILED;
			}

			//Apply the frequency value to the passing variable
			temp_complex = complex(*extracted_freq*2.0*PI,0.0);

			//Loop through delta objects and post their "post transient" frequency values - push phase
			for (curr_object_number=0; curr_object_number<gen_object_count; curr_object_number++)
			{
				//See if we're in service or not
				if ((delta_objects[curr_object_number]->in_svc_double <= gl_globaldeltaclock) && (delta_objects[curr_object_number]->out_svc_double >= gl_globaldeltaclock))
				{
					//Call the actual function
					function_status = ((STATUS (*)(OBJECT *, complex *, unsigned int))(*post_delta_functions[curr_object_number]))(delta_objects[curr_object_number],&temp_complex,1);
				}
				else //Not in service
					function_status = SUCCESS;

				//Make sure we worked
				if (function_status == FAILED)
				{
					gl_error("Generator object:%s - failed post-deltamode update",delta_objects[curr_object_number]->name);
					//Defined above
					return FAILED;
				}
				//Default else - successful, keep going
			}
		}

		//We always succeed from this, just because (unless we failed above)
		return SUCCESS;
	}
	else
	{	//Not sure how it would ever get here, but just jump out if that's the case
		return SUCCESS;
	}
}

CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	return 0;
}
