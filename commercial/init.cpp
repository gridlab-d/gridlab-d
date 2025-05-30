/** $Id: init.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "gridlabd.h"

#include "office.h"
#include "multizone.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==nullptr)
	{
		errno = EINVAL;
		return nullptr;
	}

	gl_global_create("commercial::warn_control",PT_bool,&office::warn_control,nullptr);
	gl_global_create("commercial::warn_low_temp",PT_double,&office::warn_low_temp,nullptr);
	gl_global_create("commercial::warn_high_temp",PT_double,&office::warn_high_temp,nullptr);

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
