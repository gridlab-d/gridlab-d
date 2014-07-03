// $id$
// Copyright (C) 2008 Battelle Memorial Institute

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"

#include "market.h"
#include "auction.h"
#include "controller.h"
#include "stubauction.h"
#include "passive_controller.h"
#include "double_controller.h"
#include "stub_bidder.h"
#include "generator_controller.h"

double bid_offset = 0.0001;

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	gl_global_create("market::bid_offset",PT_double,&bid_offset,PT_UNITS,"$",PT_DESCRIPTION,"the bid offset value that prevents bids from being wrongly triggered",NULL);

	new auction(module);
	new controller(module);
	new stubauction(module);
	new passive_controller(module);
	new double_controller(module);
	new stub_bidder(module);
	new generator_controller(module);

	/*** DO NOT EDIT NEXT LINE ***/
	//NEWCLASS

	/* always return the first class registered */
	return auction::oclass;
}


CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}
