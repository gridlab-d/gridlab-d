/* $Id: reliability.h 4738 2014-07-03 00:55:39Z dchassin $ 
	Copyright (C) 2008 Battelle Memorial Institute
*/

#ifndef _reliability_H
#define _reliability_H

#include <stdarg.h>
#include "gridlabd.h"

#ifdef _RELIABILITY_CPP
#define GLOBAL
#define INIT(A) = (A)
#else
#define GLOBAL extern
#define INIT(A)
#endif

//Module globals
GLOBAL double event_max_duration INIT(432000.0);	/**< Maximum length of any event on the system - given in seconds - defaults to 5 days */
GLOBAL bool enable_subsecond_models INIT(false);	/**< normally not operating in delta mode */
GLOBAL unsigned long deltamode_timestep INIT(10000000); /* 10 ms timestep */
GLOBAL FUNCTIONADDR *delta_functions INIT(NULL);			/* Array pointer functions for objects that need deltamode interupdate calls */
GLOBAL OBJECT **delta_objects INIT(NULL);				/* Array pointer objects that need deltamode interupdate calls */
GLOBAL int eventgen_object_count INIT(0);		/* deltamode object count */
GLOBAL int eventgen_object_current INIT(-1);		/* Index of current deltamode object */
GLOBAL TIMESTAMP deltamode_starttime INIT(TS_NEVER);	/* Tracking variable for next desired instance of deltamode */
GLOBAL TIMESTAMP deltamode_endtime INIT(TS_NEVER);		/* Tracking variable to see when deltamode ended - so differential calculations don't get messed up */

void schedule_deltamode_start(TIMESTAMP tstart);

#endif
