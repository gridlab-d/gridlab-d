/** $Id: init.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "gridlabd.h"

#include "climate.h"
#include "weather.h"
#include "csv_reader.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==nullptr)
	{
		errno = EINVAL;
		return nullptr;
	}

	new climate(module);
	new weather(module);
	new csv_reader(module);

	/* always return the first class registered */
	return climate::oclass;
}


CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	/* if any climate objects have bad filenames, they'll fail on init() */
	return 0;
}
