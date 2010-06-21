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

	gl_global_create("reliability::maximum_event_length",PT_double,&event_max_duration,PT_UNITS,"s",PT_DESCRIPTION,"Maximum duration of any faulting event",NULL);
	gl_global_create("reliability::report_event_log",PT_bool,&metrics::report_event_log,PT_DESCRIPTION,"Should the metrics object dump a logfile?",NULL);

	new metrics(module);
	new eventgen(module);

	/* always return the first class registered */
	return metrics::oclass;
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

