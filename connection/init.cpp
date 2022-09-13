/** $Id: init.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "gridlabd.h"

#include "connection.h"
#include "native.h"
#include "xml.h"
#include "json.h"
#include "fncs_msg.h"
#include "helics_msg.h"
#include "socket.h"

CONNECTIONSECURITY connection_security = CS_STANDARD;
double connection_lockout = 0.0;
bool enable_subsecond_models = false;

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	Socket::init();

	gl_global_create("connection::security",PT_enumeration,&connection_security,
		PT_DESCRIPTION, "default connection security level",
		PT_KEYWORD, "NONE", (enumeration)CS_NONE,
		PT_KEYWORD, "LOW", (enumeration)CS_LOW,
		PT_KEYWORD, "STANDARD", (enumeration)CS_STANDARD,
		PT_KEYWORD, "HIGH", (enumeration)CS_HIGH,
		PT_KEYWORD, "EXTREME", (enumeration)CS_EXTREME,
		PT_KEYWORD, "PARANOID", (enumeration)CS_PARANOID,
		NULL);
	gl_global_create("connection::lockout",PT_double,&connection_lockout,
		PT_UNITS, "s",
		PT_DESCRIPTION, "default connection security lockout time",
		NULL);
	gl_global_create("connection::enable_subsecond_models", PT_bool, &enable_subsecond_models,PT_DESCRIPTION,"Enable deltamode capabilities within the connection module",NULL);

	new native(module);
//	new xml(module); // TODO finish XML implementation
	new json(module);
#if HAVE_FNCS
	new fncs_msg(module);
#endif
#if HAVE_HELICS
	new helics_msg(module);
#endif
	// TODO add new classes before this line

	/* always return the first class registered */
	return native::oclass;
}


EXPORT int do_kill(void*)
{
	/* if global memory needs to be released, this is a good time to do it */
	// TODO process all term() for each object created by this module
	return 0;
}

EXPORT int check(){
	/* if any assert objects have bad filenames, they'll fail on init() */
	return 0;
}

static CLKUPDATELIST *clkupdatelist = NULL;

void add_clock_update(void *data, CLOCKUPDATE clkupdate)
{
	struct s_clockupdatelist *item = new struct s_clockupdatelist;
    item->clkupdate = clkupdate;
	item->data = data;
	item->next = clkupdatelist;
	clkupdatelist = item;
}

EXPORT TIMESTAMP clock_update(TIMESTAMP t1)
{
	// send t1 to external tool and return alternate proposed time. Return t1 if
	// t1 is ok to use.  Do not return TS_INVALID or any values less that global_clock
	// CAVEAT: this will be called multiple times until all modules that use clock_update
	// agree on the value of t1.
	struct s_clockupdatelist *item;
	TIMESTAMP t2 = t1;
	int ok = 0;
	while (!ok)
	{
		ok = 1;
		for ( item=clkupdatelist ; item!=NULL ; item = item->next )
		{
			TIMESTAMP t2 = item->clkupdate(item->data,t1);
			if ( t2<t1 )
			{
				ok = 0;
				t1 = t2;
			}
		}
	}
	return t1;
}

EXPORT int deltamode_desired(int *flags)
{
	return DT_INFINITY;	/* Returns time in seconds to when the next deltamode transition is desired */

	/* Return DT_INFINITY if this module doesn't have any specific triggering times */
}

//Deltamode functions
EXPORT unsigned long preupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
{
	double double_timestep_value;

	if (enable_subsecond_models == true)
	{
		//NOTE: that given how this is used currently, preupdate could just be a return DT_INFINITY for all cases

		//Pull global timestep
		gld_global dtimestep("deltamode_timestep");
		if (!dtimestep.is_valid()){
			gl_error("connection::preupdate: unable to find delamode_timestep!");
			return DT_INVALID;
		}

		//Pull it for use
		double_timestep_value = dtimestep.get_double();

		if (double_timestep_value <= 0.0)
		{
			gl_error("connection::preupdate: deltamode_timestep must be a positive, non-zero number!");
			/*  TROUBLESHOOT
			The value for global_deltamode_timestep, must be a positive, non-zero number.
			Please use such a number and try again.
			*/

			return DT_INVALID;
		}
		else
		{
			//Do the casting conversion on it to put it back in integer format
			return (unsigned long)(double_timestep_value + 0.5);
		}
	}
	else	//Not desired, just return an arbitrarily large value
	{
		return DT_INFINITY;
	}
}

static DELTAINTERUPDATELIST *dInterUpdateList = NULL;

void register_object_interupdate (void * data, DELTAINTERUPDATE dInterUpdate)
{
	struct s_deltainterupdatelist *item = new struct s_deltainterupdatelist;
	item->deltainterupdate = dInterUpdate;
	item->data = data;
	item->next = dInterUpdateList;
	dInterUpdateList = item;
}

EXPORT SIMULATIONMODE interupdate(MODULE *module, TIMESTAMP t0, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	struct s_deltainterupdatelist *item;
	SIMULATIONMODE result = SM_EVENT;
	SIMULATIONMODE rv = SM_EVENT;
	for (item = dInterUpdateList; item != NULL; item = item->next)
	{
		result = item->deltainterupdate(item->data, iteration_count_val, t0, delta_time);
		switch(result)
		{
		case SM_DELTA_ITER:
			rv = SM_DELTA_ITER;
			break;
		case SM_DELTA:
			if(rv != SM_DELTA_ITER){
				rv = SM_DELTA;
			}
			break;
		case SM_EVENT:
			if(rv != SM_DELTA_ITER || rv != SM_DELTA){
				rv = SM_EVENT;
			}
			break;
		case SM_ERROR:
			return SM_ERROR;
		default:
			break;
		}
	}
	return rv;
}

static DELTACLOCKUPDATELIST *dClockUpdateList = NULL;

void register_object_deltaclockupdate(void *data, DELTACLOCKUPDATE dClockUpdate)
{
	struct s_deltaclockupdatelist *item = new struct s_deltaclockupdatelist;
	item->dclkupdate = dClockUpdate;
	item->data = data;
	item->next = dClockUpdateList;
	dClockUpdateList = item;
}

EXPORT SIMULATIONMODE deltaClockUpdate(MODULE *module, double t1, unsigned long timestep, SIMULATIONMODE systemmode)
{
	struct s_deltaclockupdatelist *item;
	SIMULATIONMODE result = SM_DELTA;
	for(item = dClockUpdateList; item != NULL; item = item->next)
	{
		result = item->dclkupdate(item->data, t1, timestep, systemmode);
		return result;
	}

	//Check for a NULL list (no objects), that could get us here
	if (dClockUpdateList == NULL)
	{
		//Our list is empty, so nothing wants deltamode
		return SM_EVENT;
	}
	else
	{
		return SM_ERROR; // fall through return value resolves return-type warning. Should be unreachable.
	}
}

EXPORT int postupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
{
	/* t0 is the current global clock time (event-based) */
	/* dt is the current deltaclock nanosecond offset from t0 */

	return SUCCESS;	/* return FAILURE (0) or SUCCESS (1) */
}
