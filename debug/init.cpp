/** $Id: init.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"

#include "gldebug.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	try {
		new g_debug(module);
	} 
	catch (const char *msg)
	{
		gl_error("exception caught: %s", msg);
		return NULL;
	}

	/* always return the first class registered */
	return g_debug::oclass;
}


EXPORT int do_kill(void*)
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	/* if any debug objects have bad filenames, they'll fail on init() */
	return 0;
}
