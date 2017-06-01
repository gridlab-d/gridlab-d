// init.cpp

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"

#include "matpower.h"
#include "bus.h"
//#include "line.h"
//#include "solver_matpower.h"
#include "gen.h"
#include "gen_cost.h"
#include "areas.h"
#include "baseMVA.h"
#include "branch.h"
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
	//gl_global_create(char *name, ..., NULL);
	/* TODO: use gl_global_setvar, gl_global_getvar, and gl_global_find for access */
#endif

	/*** DO NOT EDIT NEXT LINE ***/
	
	new bus(module);
	//new line(module);	
	new branch(module);
	new gen(module);
	//new gen_cost(module);
	//new areas(module);
	//new baseMVA(module);
	
	/* always return the first class registered */
	/* TODO this module will not compile until a class has been defined */
	return bus::oclass;
}


CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}
