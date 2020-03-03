/** $Id: init.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"

#include "office.h"
#include "multizone.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	gl_global_create("commercial::warn_control",PT_bool,&office::warn_control,NULL);
	gl_global_create("commercial::warn_low_temp",PT_double,&office::warn_low_temp,NULL);
	gl_global_create("commercial::warn_high_temp",PT_double,&office::warn_high_temp,NULL);

	new office(module);
	new multizone(module); 

	/* always return the first class registered */
	return office::oclass;
}

CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	return 0;
}
