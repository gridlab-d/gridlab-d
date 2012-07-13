// $Id$
// Copyright (C) 2012 Battelle Memorial Institute
// DP Chassin

#define DLMAIN
#include "gridlabd.h"

EXPORT int do_kill(void*)
{
	return 0;
}

EXPORT int init_solvers(CALLBACKS* fntable)
{
	callback = fntable;
	return 1;
}