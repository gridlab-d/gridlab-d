// $Id$
// Copyright (C) 2020 Battelle Memorial Institute

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "gridlabd.h"

#define _GENERATORS_GLOBALS
#include "generators.h"
#undef _GENERATORS_GLOBALS

//Include the various items
#include "diesel_dg.h"
#include "windturb_dg.h"
#include "battery.h"
#include "inverter.h"
#include "rectifier.h"
#include "solar.h"
#include "central_dg_control.h"
#include "controller_dg.h"
#include "inverter_dyn.h"
#include "sync_ctrl.h"
#include "energy_storage.h"
#include "inverter_DC_Ctrl.h"
#include "dc_link.h"
#include "inverter_DC.h"
#include "sec_control.h"
//**** IBR_SKELETON_NOTE: ADD HEADER FILES OF NEW MODELS HERE ****//
#include "ibr_skeleton.h"

//Define defaults, since many use them and they aren't here yet
EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==nullptr)
	{
		errno = EINVAL;
		return nullptr;
	}

	/* Publish external global variables */
	gl_global_create("generators::default_line_voltage",PT_double,&default_line_voltage,PT_UNITS,"V",PT_DESCRIPTION,"line voltage (L-N) to use when no circuit is attached",nullptr);
	gl_global_create("generators::enable_subsecond_models", PT_bool, &enable_subsecond_models,PT_DESCRIPTION,"Enable deltamode capabilities within the generators module",nullptr);
	gl_global_create("generators::all_generator_delta", PT_bool, &all_generator_delta, PT_DESCRIPTION, "Forces all generator objects that are capable to participate in deltamode",nullptr);
	gl_global_create("generators::deltamode_timestep", PT_double, &deltamode_timestep_publish,PT_UNITS,"ns",PT_DESCRIPTION,"Desired minimum timestep for deltamode-related simulations",nullptr);
	gl_global_create("generators::default_temperature_value", PT_double, &default_temperature_value,PT_UNITS,"degF",PT_DESCRIPTION,"Temperature when no climate module is detected",nullptr);

	//Instantiate the classes
	new diesel_dg(module);
	new windturb_dg(module);
	new inverter(module);
	new rectifier(module);
	new battery(module);
	new solar(module);
	new central_dg_control(module);
	new controller_dg(module);
	new inverter_dyn(module);
	new sync_ctrl(module);
	new energy_storage(module);
	new inverter_DC_Ctrl(module);
	new dc_link(module);
	new inverter_DC(module);
	new sec_control(module);
	//**** IBR_SKELETON_NOTE: Would add any new objects here too ****//
	new ibr_skeleton(module);

	/* Clear the deltamode list too, just in case*/
	delta_object.clear();

	/* always return the first class registered */
	return diesel_dg::oclass;
}

//Call function for scheduling deltamode
void schedule_deltamode_start(TIMESTAMP tstart)
{
	if (enable_subsecond_models)	//Make sure the overall mode is enabled
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

//Function for allocating/adding objects to the deltamode execution list
//obj = object header of the object
//prioritize = boolean that will elevate current object to the top of the generator object deltamode execution list
//Returns a status - SUCCESS/FAIL
STATUS add_gen_delta_obj(OBJECT *obj, bool prioritize)
{
	GEN_DELTA_OBJ temp_gen_obj;
	STATUS return_status;

	//Populate the object pointer
	temp_gen_obj.obj = obj;

	//See which functions exist
	temp_gen_obj.preudpate_fxn = (FUNCTIONADDR)(gl_get_function(obj, "preupdate_gen_object"));
	temp_gen_obj.interupdate_fxn = (FUNCTIONADDR)(gl_get_function(obj, "interupdate_gen_object"));
	temp_gen_obj.post_delta_fxn = (FUNCTIONADDR)(gl_get_function(obj, "postupdate_gen_object"));
	
	//Try to add - put in a catch, just in case
	try	{
		//See if we go on the top of bottom of the vector
		if (prioritize)
		{
			//See if we're the first element
			if (!delta_object.empty())
			{
				//Add at the beginning
				delta_object.insert(delta_object.begin(),temp_gen_obj);
			}
			else
			{
				//Empty array - just add us - we're first by default
				delta_object.push_back(temp_gen_obj);
			}

		}
		else
		{
			//Just add at the end
			delta_object.push_back(temp_gen_obj);
		}

		//If it makes it here, must have succeeded
		return_status = SUCCESS;
	}
	catch (const char *msg)
	{
		gl_error("generator:add_gen_delta_obj - failed allocation for obj:%d-%s - %s", obj->id,(obj->name ? obj->name : "Unnamed"), msg);
		return_status = FAILED;
	}
	catch (...)
	{
		gl_error("powerflow:add_gen_delta_obj - pre-pass function call: unknown exception for obj:%d - %s",obj->id,(obj->name ? obj->name : "Unnamed"));
		return_status = FAILED;
	}
	
	//Return what happened
	return return_status;
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

	if (enable_subsecond_models)	//Make sure we even want to run deltamode
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
	int curr_object_number;
	STATUS status_value;

	if (enable_subsecond_models)
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

			//Perform other preupdate functionality, as needed
			//Loop through the object list and call the pre-update functions
			for (curr_object_number = 0; curr_object_number < delta_object.size(); curr_object_number++)
			{
				//See if it has a function
				if (delta_object[curr_object_number].preudpate_fxn != nullptr)
				{
					//Call the function
					status_value = ((STATUS (*)(OBJECT *, TIMESTAMP, unsigned int64))(*delta_object[curr_object_number].preudpate_fxn))(delta_object[curr_object_number].obj,t0,dt);

					//Make sure we passed
					if (status_value != SUCCESS)
					{
						GL_THROW("generators - object:%d %s failed to run the object-level preupdate function",delta_object[curr_object_number].obj->id,(delta_object[curr_object_number].obj->name ? delta_object[curr_object_number].obj->name : "Unnamed"));
						/*  TROUBLESHOOT
						While attempting to call a preupdate function for a generator deltamode object, an error occurred.
						Please look to the console output for more details.
						*/
					}
					//Default else - it must have worked
				}
				//Default else - not one, so skip
			}

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
	
	if (enable_subsecond_models)
	{
		//See if this is the first instance -- if so, update the timestep (if in-rush enabled)
		if (deltatimestep_running < 0.0)
		{
			//Set the powerflow global -- technically the same as dt, but in double precision (less divides)
			deltatimestep_running = (double)((double)dt/(double)DT_SECOND);
		}
		//Default else -- already set

		//Loop through the object list and call the updates
		for (curr_object_number = 0; curr_object_number < delta_object.size(); curr_object_number++)
		{
			//Overall check
			if (delta_object[curr_object_number].interupdate_fxn != nullptr)
			{
				//See if we're in service or not
				if ((delta_object[curr_object_number].obj->in_svc_double <= gl_globaldeltaclock) && (delta_object[curr_object_number].obj->out_svc_double >= gl_globaldeltaclock))
				{
					//Call the actual function
					function_status = ((SIMULATIONMODE (*)(OBJECT *, unsigned int64, unsigned long, unsigned int))(*delta_object[curr_object_number].interupdate_fxn))(delta_object[curr_object_number].obj,delta_time,dt,iteration_count_val);
				}
				else //Not in service - off to event mode
					function_status = SM_EVENT;

				//Determine what our return is
				if (function_status == SM_DELTA)
				{
					gl_verbose("Generator object:%d - %s - requested deltamode to continue",delta_object[curr_object_number].obj->id,(delta_object[curr_object_number].obj->name ? delta_object[curr_object_number].obj->name : "Unnamed"));

					event_driven = false;
				}
				else if (function_status == SM_DELTA_ITER)
				{
					gl_verbose("Generator object:%d - %s - requested a deltamode reiteration",delta_object[curr_object_number].obj->id,(delta_object[curr_object_number].obj->name ? delta_object[curr_object_number].obj->name : "Unnamed"));

					event_driven = false;
					delta_iter = true;
				}
				else if (function_status == SM_ERROR)
				{
					gl_error("Generator object:%s - deltamode function returned an error!",delta_object[curr_object_number].obj->name);
					/*  TROUBLESHOOT
					While performing a deltamode update, one object returned an error code.  Check to see if the object itself provided
					more details and try again.  If the error persists, please submit your code and a bug report via the trac website.
					*/
					return SM_ERROR;
				}
				else	//Must be SM_EVENT - just put this here for verbose messaging
				{
					gl_verbose("Generator object:%d - %s - requested exit to event-driven mode",delta_object[curr_object_number].obj->id,(delta_object[curr_object_number].obj->name ? delta_object[curr_object_number].obj->name : "Unnamed"));
				}
			}//End valid entry
		}
				
		//Determine how to exit - event or delta driven
		if (!event_driven)
		{
			if (delta_iter)
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
	gld::complex temp_complex;
	double *extracted_freq;

	if (enable_subsecond_models)
	{
		//Final item of transitioning out is resetting the next timestep so a smaller one can take its place
		deltamode_starttime = TS_NEVER;

		//Deflag the timestep variable as well
		deltatimestep_running = -1.0;

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
		temp_complex = gld::complex(*extracted_freq*2.0*PI,0.0);

		//Loop through delta objects and update the execution times and frequency values - only does "0" pass
		for (curr_object_number = 0; curr_object_number < delta_object.size(); curr_object_number++)
		{
			//See if a post-update function even exists
			if (delta_object[curr_object_number].post_delta_fxn != nullptr)
			{
				//See if we're in service or not
				if ((delta_object[curr_object_number].obj->in_svc_double <= gl_globaldeltaclock) && (delta_object[curr_object_number].obj->out_svc_double >= gl_globaldeltaclock))
				{
					//Call the actual function
					function_status = ((STATUS (*)(OBJECT *, gld::complex *, unsigned int))(*delta_object[curr_object_number].post_delta_fxn))(delta_object[curr_object_number].obj,&temp_complex,0);
				}
				else //Not in service
					function_status = SUCCESS;

				//Make sure we worked
				if (function_status == FAILED)
				{
					gl_error("Generator object:%s - failed post-deltamode update",delta_object[curr_object_number].obj->name);
					/*  TROUBLESHOOT
					While calling the individual object post-deltamode calculations, an error occurred.  Please try again.
					If the error persists, please submit your code and a bug report via the trac website.
					*/
					return FAILED;
				}
				//Default else - successful, keep going
			}
			//Default else -- no function, so just keep going
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

EXPORT int check()
{
	return 0;
}
