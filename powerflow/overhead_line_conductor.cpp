/** $Id: overhead_line_conductor.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file overhead_line_conductor.cpp
	@addtogroup overhead_line_conductor 
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

CLASS* overhead_line_conductor::oclass = NULL;
CLASS* overhead_line_conductor::pclass = NULL;

overhead_line_conductor::overhead_line_conductor(MODULE *mod) : powerflow_library(mod)
{
	if(oclass == NULL)
	{
		oclass = gl_register_class(mod,"overhead_line_conductor",sizeof(overhead_line_conductor),0x00);
		if (oclass==NULL)
			throw "unable to register class overhead_line_conductor";
		else
			oclass->trl = TRL_PROVEN;

        if(gl_publish_variable(oclass,
           PT_double, "geometric_mean_radius[ft]",PADDR(geometric_mean_radius),PT_DESCRIPTION, "radius of the conductor",
           PT_double, "resistance[Ohm/mile]",PADDR(resistance),PT_DESCRIPTION, "resistance in Ohms/mile of the conductor",
		   PT_double, "diameter[in]",PADDR(cable_diameter), PT_DESCRIPTION, "Diameter of line for capacitance calculations",
		   PT_double, "rating.summer.continuous[A]", PADDR(summer.continuous),PT_DESCRIPTION, "Continuous summer amp rating",
		   PT_double, "rating.summer.emergency[A]", PADDR(summer.emergency),PT_DESCRIPTION, "Emergency summer amp rating",
		   PT_double, "rating.winter.continuous[A]", PADDR(winter.continuous),PT_DESCRIPTION, "Continuous winter amp rating",
		   PT_double, "rating.winter.emergency[A]", PADDR(winter.emergency),PT_DESCRIPTION, "Emergency winter amp rating",
            NULL) < 1) GL_THROW("unable to publish overhead_line_conductor properties in %s",__FILE__);
    }
}

int overhead_line_conductor::create(void)
{
	int result = powerflow_library::create();
	
	cable_diameter = 0.0;
	geometric_mean_radius = resistance = 0.0;
	summer.continuous = winter.continuous = 1000;
	summer.emergency = winter.emergency = 2000;

	return result;
}

int overhead_line_conductor::init(OBJECT *parent)
{
	return 1;
}

int overhead_line_conductor::isa(char *classname)
{
	return strcmp(classname,"overhead_line_conductor")==0;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: overhead_line_conductor
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_overhead_line_conductor(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(overhead_line_conductor::oclass);
		if (*obj!=NULL)
		{
			overhead_line_conductor *my = OBJECTDATA(*obj,overhead_line_conductor);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(overhead_line_conductor);
}

EXPORT int init_overhead_line_conductor(OBJECT *obj)
{
	try {
		overhead_line_conductor *my = OBJECTDATA(obj,overhead_line_conductor);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(overhead_line_conductor);
}

EXPORT TIMESTAMP sync_overhead_line_conductor(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	return TS_NEVER;
}

EXPORT int isa_overhead_line_conductor(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,overhead_line_conductor)->isa(classname);
}


/**@}**/
