//init.cpp

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"
#include "rest.h"
#include "server.h"^M
#include "mongoose.h"

//NEWCLASSINC

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	INIT_MMF(rest);

#ifdef OPTIONAL
	/* TODO: publish global variables (see class_define_map() for details) */
	  //gl_global_create(char *name, ..., NULL);
	/* TODO: use gl_global_setvar, gl_global_getvar, and gl_global_find for access */
#endif

	/*** DO NOT EDIT NEXT LINE ***/
	new server(module);
	//NEWCLASSDEF
	
	/* always return the first class registered */
	/* TODO this module will not compile until a class has been defined */
	return server::oclass;
}


CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}
