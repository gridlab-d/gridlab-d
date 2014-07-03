/** $Id: triplex_line_conductor.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file triplex_line_conductor.cpp
	@addtogroup triplex_line_conductor 
	@ingroup line

	@{
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
using namespace std;

#include "line.h"

CLASS* triplex_line_conductor::oclass = NULL;
CLASS* triplex_line_conductor::pclass = NULL;

triplex_line_conductor::triplex_line_conductor(MODULE *mod) : powerflow_library(mod)
{
	if(oclass == NULL)
	{
		oclass = gl_register_class(mod,"triplex_line_conductor",sizeof(triplex_line_conductor),0x00);
		if (oclass==NULL)
			throw "unable to register class triplex_line_conductor";
		else
			oclass->trl = TRL_PROVEN;

        if(gl_publish_variable(oclass,
			PT_double, "resistance[Ohm/mile]",PADDR(resistance),PT_DESCRIPTION,"resistance of cable in ohm/mile",
			PT_double, "geometric_mean_radius[ft]", PADDR(geometric_mean_radius),PT_DESCRIPTION,"geometric mean radius of the cable",
 			PT_double, "rating.summer.continuous[A]", PADDR(summer.continuous),PT_DESCRIPTION,"amp ratings for the cable during continuous operation in summer",
			PT_double, "rating.summer.emergency[A]", PADDR(summer.emergency),PT_DESCRIPTION,"amp ratings for the cable during short term operation in summer",
			PT_double, "rating.winter.continuous[A]", PADDR(winter.continuous),PT_DESCRIPTION,"amp ratings for the cable during continuous operation in winter",
			PT_double, "rating.winter.emergency[A]", PADDR(winter.emergency),PT_DESCRIPTION,"amp ratings for the cable during short term operation in winter",
			NULL) < 1) GL_THROW("unable to publish triplex_line_conductor properties in %s",__FILE__);
	}   
}

int triplex_line_conductor::create(void)
{
	int result = powerflow_library::create();
	resistance = 0.0;
	geometric_mean_radius = 0.0;

	summer.continuous = winter.continuous = 202;	//1/0 Class A AA wire
	summer.emergency = winter.emergency = 212.1;	//1.05x continuous

	return result;
}

int triplex_line_conductor::isa(char *classname)
{
	return strcmp(classname,"triplex_line_conductor")==0;

}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: triplex_line_conductor
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_triplex_line_conductor(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(triplex_line_conductor::oclass);
		if (*obj!=NULL)
		{
			triplex_line_conductor *my = OBJECTDATA(*obj,triplex_line_conductor);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(triplex_line_conductor);
}

EXPORT TIMESTAMP sync_triplex_line_conductor(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	return TS_NEVER;
}

EXPORT int isa_triplex_line_conductor(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,triplex_line_conductor)->isa(classname);
}

/**@}**/
