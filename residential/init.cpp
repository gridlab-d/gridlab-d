/** $Id: init.cpp,v 1.32 2008/01/09 22:25:08 d3p988 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file init.cpp
	@addtogroup residential Residential loads (residential)
	@ingroup modules
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "residential.h"

#include "house.h"
#include "clotheswasher.h"
#include "dishwasher.h"
#include "lights.h"
#include "microwave.h"
#include "occupantload.h"
#include "plugload.h"
#include "range.h"
#include "refrigerator.h"
#include "waterheater.h"
#include "freezer.h"
#include "dryer.h"
#include "evcharger.h"
#include "house_e.h"

complex default_line_voltage[3] = {complex(240,0,A),complex(120,180,A),complex(120,180,A)};
complex default_line_current[3] = {complex(0,0,J),complex(0,0,J),complex(0,0,J)};

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	gl_global_create("residential::default_line_voltage",PT_complex,&default_line_voltage,PT_SIZE,3,PT_UNITS,"V",NULL);
	gl_global_create("residential::default_line_current",PT_complex,&default_line_current,PT_SIZE,3,PT_UNITS,"A",NULL);

	house *house_class = new house(module);
	new house_e(module);
	new waterheater(module);
	new lights(module);
	new refrigerator(module);
	new clotheswasher(module);
	new dishwasher(module);
	new occupantload(module);
	new plugload(module);
	new microwave(module);
	new range(module);
	new freezer(module);
	new dryer(module);
	new evcharger(module);

	/* always return the first class registered */
	return house_class->oclass;
}

CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	return 0;
}


/**@}**/
