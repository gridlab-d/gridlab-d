// init.cpp
//	Copyright (C) 2008 Battelle Memorial Institute


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"

#define _RELIABILITY_CPP
#include "reliability.h"
#undef  _RELIABILITY_CPP

#include "metrics.h"
#include "eventgen.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	/* Publish external global variables */
	gl_global_create("reliability::enable_subsecond_models", PT_bool, &enable_subsecond_models,PT_DESCRIPTION,"Flag to enable deltamode functionality in the reliability module",NULL);
	gl_global_create("reliability::maximum_event_length",PT_double,&event_max_duration,PT_UNITS,"s",PT_DESCRIPTION,"Maximum duration of any faulting event",NULL);
	gl_global_create("reliability::report_event_log",PT_bool,&metrics::report_event_log,PT_DESCRIPTION,"Should the metrics object dump a logfile?",NULL);
	gl_global_create("reliability::deltamode_timestep", PT_int32, &deltamode_timestep,PT_DESCRIPTION,"Default timestep for reliability deltamode operations",NULL);

	new metrics(module);
	new eventgen(module);

	/* always return the first class registered */
	return metrics::oclass;
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
		GL_THROW("reliability: a call was made to deltamode functions, but subsecond models are not enabled!");
		/*  TROUBLESHOOT
		The schedule_deltamode_start function was called by an object when reliability's overall enabled_subsecond_models
		flag was not set.  The module-level flag indicates that no devices should use deltamode, but one made the call
		to a deltamode function.  Please check the deltamode_inclusive flag on all objects.  If deltamode is desired,
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

			gl_debug("reliability: deltamode desired in %d", dt_val);
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
//detamode simulation stepsize
EXPORT unsigned long preupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
{
	if (enable_subsecond_models == true)
	{
		return deltamode_timestep;
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
		for (curr_object_number=0; curr_object_number<eventgen_object_count; curr_object_number++)
		{
			//See if we're in service or not
			if ((delta_objects[curr_object_number]->in_svc_double <= gl_globaldeltaclock) && (delta_objects[curr_object_number]->out_svc_double >= gl_globaldeltaclock))
			{
				//Call the actual function
				function_status = ((SIMULATIONMODE (*)(OBJECT *, unsigned int64, unsigned long, unsigned int))(*delta_functions[curr_object_number]))(delta_objects[curr_object_number],delta_time,dt,iteration_count_val);
				
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
					gl_error("Reliability object:%s - deltamode function returned an error!",delta_objects[curr_object_number]->name);
					/*  TROUBLESHOOT
					While performing a deltamode update, one object returned an error code.  Check to see if the object itself provided
					more details and try again.  If the error persists, please submit your code and a bug report via the trac website.
					*/
					return SM_ERROR;
				}
			}
			else //Not in service - off to event mode
				function_status = SM_EVENT;
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
	if (enable_subsecond_models == true)
	{
		//Final item of transitioning out is resetting the next timestep so a smaller one can take its place
		deltamode_starttime = TS_NEVER;
		
		return SUCCESS;
	}
	else	//Deltamode not wanted, just "succeed"
	{
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

