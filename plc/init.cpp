// init.cpp
//	Copyright (C) 2008 Battelle Memorial Institute

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"

#include "plc.h"

char libpath[1024] = ".";
char incpath[1024] = ".";
TIMESTAMP *pGlobalClock=NULL;

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}
	pGlobalClock = fntable->global_clock;
	gl_global_getvar("execdir",libpath,sizeof(libpath));
	strcat(libpath,"/plc/lib");
	gl_global_create("plc::libpath",PT_char1024,libpath,NULL);

	gl_global_getvar("execdir",incpath,sizeof(incpath));
	strcat(incpath,"/plc/include");
	gl_global_create("plc::incpath",PT_char1024,incpath,NULL);

	new plc(module);
	new comm(module);

	/* always return the first class registered */
	return plc::oclass;
}


CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	return 0;
}
