// $id$
// Copyright (C) 2008 Battelle Memorial Institute

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"

#include "optimize.h"
#include "simple.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	new simple(module);

	/*** DO NOT EDIT NEXT LINE ***/
	//NEWCLASS

	/* always return the first class registered */
	return simple::oclass;
}


CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}
