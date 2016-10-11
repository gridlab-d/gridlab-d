// gismo/main.cpp
//	Copyright (C) 2008 Battelle Memorial Institute

#define DLMAIN

#include "gismo.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	new line_sensor(module);
	new switch_coordinator(module);

	/* always return the first class registered */
	return line_sensor::oclass;
}


EXPORT int do_kill(void*)
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	/* if any assert objects have bad filenames, they'll fail on init() */
	return 0;
}

