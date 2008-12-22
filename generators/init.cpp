// $Id$
// Copyright (C) 2008 Battelle Memorial Institute

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"

#include "generators.h"

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

	CLASS *first =
	/*** DO NOT EDIT NEXT LINE ***/
	//NEWCLASS
	(new diesel_dg(module))->oclass;
	NULL;

	(new windturb_dg(module))->oclass;
	NULL;

	(new power_electronics(module))->oclass;
	NULL;

	(new energy_storage(module))->oclass;
	NULL;

	(new inverter(module))->oclass;
	NULL;

	(new dc_dc_converter(module))->oclass;
	NULL;

	(new rectifier(module))->oclass;
	NULL;

	(new microturbine(module))->oclass;
	NULL;

	(new battery(module))->oclass;
	NULL;

	(new solar(module))->oclass;
	NULL;

	/* always return the first class registered */
	return first;
}


CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	return 0;
}
