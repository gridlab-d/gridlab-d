/** $Id: init.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file init.cpp
	@addtogroup residential Residential loads (residential)
	@ingroup modules
 @{
 **/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#define _RESIDENTIAL_CPP
#include "residential.h"
#undef  _RESIDENTIAL_CPP

// obsolete as of 3.0: #include "house_a.h"
#include "appliance.h"
#include "clotheswasher.h"
#include "dishwasher.h"
#include "lights.h"
#include "microwave.h"
#include "occupantload.h"
#include "plugload.h"
#include "range.h"
#include "refrigerator.h"
#include "waterheater.h"
#include "freezer.h"
#include "dryer.h"
#include "evcharger.h"
#include "zipload.h"
#include "thermal_storage.h"
#include "evcharger_det.h"

#include "residential_enduse.h"
#include "house_e.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	gl_global_create("residential::default_line_voltage",PT_double,&default_line_voltage,PT_UNITS,"V",PT_DESCRIPTION,"line voltage (L-N) to use when no circuit is attached",NULL);
	gl_global_create("residential::default_outdoor_temperature",PT_double,&default_outdoor_temperature,PT_UNITS,"degF",PT_DESCRIPTION,"outdoor air temperature when no climate data is found",NULL);
	gl_global_create("residential::default_humidity",PT_double,&default_humidity,PT_UNITS,"%",PT_DESCRIPTION,"humidity when no climate data is found",NULL);
	gl_global_create("residential::default_horizontal_solar",PT_double,&default_horizontal_solar,PT_UNITS,"Btu/sf",PT_DESCRIPTION,"horizontal solar gains when no climate data is found",NULL);
	gl_global_create("residential::default_etp_iterations",PT_int64,&default_etp_iterations,PT_DESCRIPTION,"number of iterations ETP solver will run",NULL);
	gl_global_create("residential::default_grid_frequency",PT_double,&default_grid_frequency,PT_UNITS,"Hz",PT_DESCRIPTION,"grid frequency when no powerflow attachment is found",NULL);
	gl_global_create("residential::ANSI_voltage_check",PT_bool,&ANSI_voltage_check,PT_DESCRIPTION,"enable or disable messages about ANSI voltage limit violations in the house",NULL);
	gl_global_create("residential::enable_subsecond_models", PT_bool, &enable_subsecond_models,PT_DESCRIPTION,"Enable deltamode capabilities within the residential module",NULL);
	gl_global_create("residential::deltamode_timestep", PT_double, &deltamode_timestep_publish,PT_UNITS,"ns",PT_DESCRIPTION,"Desired minimum timestep for deltamode-related simulations",NULL);
	gl_global_create("residential::all_residential_delta", PT_bool, &all_residential_delta,PT_DESCRIPTION,"Modeling convenient - enables all residential objects in deltamode",NULL);

	new residential_enduse(module);
	new appliance(module);
	// obsolete as of 3.0: new house(module);
	new house_e(module);
	new waterheater(module);
	new lights(module);
	new refrigerator(module);
	new clotheswasher(module);
	new dishwasher(module);
	new occupantload(module);
	new plugload(module);
	new microwave(module);
	new range(module);
	new freezer(module);
	new dryer(module);
	new evcharger(module);
	new ZIPload(module);
	new thermal_storage(module);
	new evcharger_det(module);

	/* always return the first class registered */
	return residential_enduse::oclass;
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
		GL_THROW("residential: a call was made to deltamode functions, but subsecond models are not enabled!");
		/*  TROUBLESHOOT
		The schedule_deltamode_start function was called by an object when residential's overall enabled_subsecond_models
		flag was not set.  The module-level flag indicates that no devices should use deltamode, but one made the call
		to a deltamode function.  Please check the DELTAMODE flag on all objects.  If deltamode is desired,
		please set the module-level enable_subsecond_models flag and try again.
		*/
	}
}

//Function for allocating various module-level deltamode arrays
//Consolidated it doesn't have to be changed everywhere
void allocate_deltamode_arrays(void)
{
	int obj_idx;

	if ((res_object_current == -1) || (delta_objects==NULL))
	{
		//Allocate the deltamode object array
		delta_objects = (OBJECT**)gl_malloc(res_object_count*sizeof(OBJECT*));

		//Make sure it worked
		if (delta_objects == NULL)
		{
			GL_THROW("Failed to allocate deltamode objects array for residential module!");
			/*  TROUBLESHOOT
			While attempting to create a reference array for residential module deltamode-enabled
			objects, an error was encountered.  Please try again.  If the error persists, please
			submit your code and a bug report via the trac website.
			*/
		}

		//Allocate the function reference list as well
		delta_functions = (FUNCTIONADDR*)gl_malloc(res_object_count*sizeof(FUNCTIONADDR));

		//Make sure it worked
		if (delta_functions == NULL)
		{
			GL_THROW("Failed to allocate deltamode objects function array for residential module!");
			/*  TROUBLESHOOT
			While attempting to create a reference array for residential module deltamode-enabled
			objects, an error was encountered.  Please try again.  If the error persists, please
			submit your code and a bug report via the trac website.
			*/
		}

		//Allocate the function reference list for postupdate as well
		post_delta_functions = (FUNCTIONADDR*)gl_malloc(res_object_count*sizeof(FUNCTIONADDR));

		//Make sure it worked
		if (post_delta_functions == NULL)
		{
			GL_THROW("Failed to allocate deltamode objects function array for residential module!");
			//Defined above
		}

		//Null all of these, just for a baseline
		for (obj_idx=0; obj_idx<res_object_count; obj_idx++)
		{
			delta_objects[obj_idx] = NULL;
			delta_functions[obj_idx] = NULL;
			post_delta_functions[obj_idx] = NULL;
		}

		//Initialize index
		res_object_current = 0;
	}
	//Default else, we're done, just exit
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

			gl_debug("residential: deltamode desired in %d", dt_val);
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
			gl_error("residential::deltamode_timestep must be a positive, non-zero number!");
			/*  TROUBLESHOOT
			The value for deltamode_timestep, as specified as the module level in residential, must be a positive, non-zero number.
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
		//See if this is the first instance -- if so, update the timestep (flagging-related)
		if (deltatimestep_running < 0.0)
		{
			//Set the powerflow global -- technically the same as dt, but in double precision (less divides)
			deltatimestep_running = (double)((double)dt/(double)DT_SECOND);
		}
		//Default else -- already set

		//Loop through the object list and call the updates
		for (curr_object_number=0; curr_object_number<res_object_count; curr_object_number++)
		{
			//See if we're in service or not
			if ((delta_objects[curr_object_number]->in_svc_double <= gl_globaldeltaclock) && (delta_objects[curr_object_number]->out_svc_double >= gl_globaldeltaclock))
			{
				//Try/catch for any GL_THROWs that may be called
				try {
					//Call the actual function
					function_status = ((SIMULATIONMODE (*)(OBJECT *, unsigned int64, unsigned long, unsigned int))(*delta_functions[curr_object_number]))(delta_objects[curr_object_number],delta_time,dt,iteration_count_val);
				}
				catch (const char *msg)
				{
					gl_error("residential:interupdate - pre-pass function call: %s", msg);
					function_status = SM_ERROR;	//Flag as an error too
				}
				catch (...)
				{
					gl_error("residential:interupdate - pre-pass function call: unknown exception");
					function_status = SM_ERROR;	//Flag as an error too
				}
			}
			else //Not in service - off to event mode
				function_status = SM_EVENT;

			//Determine what our return is
			if (function_status == SM_DELTA)
			{
				gl_verbose("Residential object:%d - %s - requested deltamode to continue",delta_objects[curr_object_number]->id,(delta_objects[curr_object_number]->name ? delta_objects[curr_object_number]->name : "Unnamed"));

				event_driven = false;
			}
			else if (function_status == SM_DELTA_ITER)
			{
				gl_verbose("Residential object:%d - %s - requested a deltamode reiteration",delta_objects[curr_object_number]->id,(delta_objects[curr_object_number]->name ? delta_objects[curr_object_number]->name : "Unnamed"));

				event_driven = false;
				delta_iter = true;
			}
			else if (function_status == SM_ERROR)
			{
				gl_error("Residential object:%d - %s - deltamode function returned an error!",delta_objects[curr_object_number]->id,(delta_objects[curr_object_number]->name ? delta_objects[curr_object_number]->name : "Unnamed"));
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
	int curr_object_number;
	STATUS function_status;

	if (enable_subsecond_models == true)
	{
		//Final item of transitioning out is resetting the next timestep so a smaller one can take its place
		deltamode_starttime = TS_NEVER;

		//Loop through delta objects and update the after items
		for (curr_object_number=0; curr_object_number<res_object_count; curr_object_number++)
		{
			//See if it has a post-update function
			if (post_delta_functions[curr_object_number] != NULL)
			{
				//See if we're in service or not
				if ((delta_objects[curr_object_number]->in_svc_double <= gl_globaldeltaclock) && (delta_objects[curr_object_number]->out_svc_double >= gl_globaldeltaclock))
				{
					//Try/catch for any GL_THROWs that may be called
					try {
						//Call the actual function
						function_status = ((STATUS (*)(OBJECT *))(*post_delta_functions[curr_object_number]))(delta_objects[curr_object_number]);
					}
					catch (const char *msg)
					{
						gl_error("residential:postupdate function call: %s", msg);
						function_status = FAILED;
					}
					catch (...)
					{
						gl_error("residential:postupdate function call: unknown exception");
						function_status = FAILED;
					}
				}
				else //Not in service
					function_status = SUCCESS;

				//Make sure we worked
				if (function_status == FAILED)
				{
					gl_error("Residential object:%s - failed post-deltamode update",delta_objects[curr_object_number]->name);
					/*  TROUBLESHOOT
					While calling the individual object post-deltamode calculations, an error occurred.  Please try again.
					If the error persists, please submit your code and a bug report via the trac website.
					*/
					return FAILED;
				}
				//Default else - successful, keep going
			}
			//Default else -- we didn't have one, so just skip
		}

		//Deflag the timestep variable as well
		deltatimestep_running = -1.0;

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


/**@}**/
