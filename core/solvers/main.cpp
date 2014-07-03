// $Id: main.cpp 4738 2014-07-03 00:55:39Z dchassin $
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