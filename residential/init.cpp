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

#include "house_a.h"
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
#include "zipload.h"
#include "thermal_storage.h"

#include "residential_enduse.h"
#include "house_e.h"

complex default_line_voltage[3] = {complex(240,0,A),complex(120,0,A),complex(120,0,A)};
complex default_line_current[3] = {complex(0,0,J),complex(0,0,J),complex(0,0,J)};
complex default_line_shunt[3] = {complex(0,0,J),complex(0,0,J),complex(0,0,J)};
complex default_line_power[3] = {complex(0,0,J),complex(0,0,J),complex(0,0,J)};
bool default_NR_mode = false;
double default_outdoor_temperature = 74.0;
double default_humidity = 75.0;
double default_solar[9] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
int64 default_etp_iterations = 100;

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	gl_global_create("residential::default_line_voltage",PT_complex,&default_line_voltage,PT_SIZE,3,PT_UNITS,"V",PT_DESCRIPTION,"line voltage to use when no circuit is attached",NULL);
	gl_global_create("residential::default_line_current",PT_complex,&default_line_current,PT_SIZE,3,PT_UNITS,"A",PT_DESCRIPTION,"line current calculated when no circuit is attached",NULL);
	gl_global_create("residential::default_outdoor_temperature",PT_double,&default_outdoor_temperature,PT_UNITS,"degF",PT_DESCRIPTION,"outdoor air temperature when no climate data is found",NULL);
	gl_global_create("residential::default_humidity",PT_double,&default_humidity,PT_UNITS,"%",PT_DESCRIPTION,"humidity when no climate data is found",NULL);
	gl_global_create("residential::default_solar",PT_double,&default_solar,PT_SIZE,9,PT_UNITS,"Btu/sf",PT_DESCRIPTION,"solar gains when no climate data is found",NULL);
	gl_global_create("residential::default_etp_iterations",PT_int64,&default_etp_iterations,PT_DESCRIPTION,"number of iterations ETP solver will run",NULL);

	new residential_enduse(module);
	new house(module);
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
	new ZIPload(module);
	new thermal_storage(module);

	/* always return the first class registered */
	return residential_enduse::oclass;
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
