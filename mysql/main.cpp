// $Id: main.cpp 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#define DLMAIN

#ifdef HAVE_MYSQL
#include "database.h"
#else
#include "gridlabd.h"
#endif

EXPORT int do_kill(void*)
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	/* if any assert objects have bad filenames, they'll fail on init() */
	return 0;
}
