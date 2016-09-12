// $Id: main.cpp 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#define DLMAIN

#include "gridlabd.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"

//NEWCLASSINC

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

#ifdef OPTIONAL
	/* TODO: publish global variables (see class_define_map() for details) */
	gl_global_create(char *name, ..., NULL);
	/* TODO: use gl_global_setvar, gl_global_getvar, and gl_global_find for access */
#endif

	/*** DO NOT EDIT NEXT LINE ***/
	//NEWCLASSDEF
	
	/* always return the first class registered */
	/* TODO this module will not compile until a class has been defined */
	return CLASSNAME::oclass;
}


CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}
