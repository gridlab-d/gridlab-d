// init.cpp
//	Copyright (C) 2008 Battelle Memorial Institute


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"

#include "reliability.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	gl_global_create("reliability::major_event_threshold",PT_double,&metrics::major_event_threshold,NULL);
	gl_global_create("reliability::report_event_log",PT_bool,&metrics::report_event_log,NULL);

	new metrics(module);
	new eventgen(module);
	/*** DO NOT EDIT NEXT LINE ***/
	//NEWCLASS()


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

