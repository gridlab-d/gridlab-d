/** $Id: line_spacing.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file line_spacing.cpp
	@addtogroup line_spacing
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

CLASS* line_spacing::oclass = NULL;
CLASS* line_spacing::pclass = NULL;

line_spacing::line_spacing(MODULE *mod) : powerflow_library(mod)
{
	if(oclass == NULL)
	{
		oclass = gl_register_class(mod,"line_spacing",sizeof(line_spacing),0x00);
		if (oclass==NULL)
			throw "unable to register class line_spacing";
		else
			oclass->trl = TRL_PROVEN;

        if(gl_publish_variable(oclass,
			PT_double, "distance_AB[ft]",PADDR(distance_AtoB),   
			PT_double, "distance_BC[ft]",PADDR(distance_BtoC),   
			PT_double, "distance_AC[ft]",PADDR(distance_AtoC),   
			PT_double, "distance_AN[ft]",PADDR(distance_AtoN),   
			PT_double, "distance_BN[ft]",PADDR(distance_BtoN),   
			PT_double, "distance_CN[ft]",PADDR(distance_CtoN),   
			PT_double, "distance_AE[ft]",PADDR(distance_AtoE), PT_DESCRIPTION, "distance between phase A wire and earth",
			PT_double, "distance_BE[ft]",PADDR(distance_BtoE), PT_DESCRIPTION, "distance between phase B wire and earth",
			PT_double, "distance_CE[ft]",PADDR(distance_CtoE), PT_DESCRIPTION, "distance between phase C wire and earth",
			PT_double, "distance_NE[ft]",PADDR(distance_NtoE), PT_DESCRIPTION, "distance between neutral wire and earth",
            NULL) < 1) GL_THROW("unable to publish line_spacing properties in %s",__FILE__);
    }
}

int line_spacing::create()
{
    // Set up defaults
	distance_AtoB = 0.0;
	distance_BtoC = 0.0;
	distance_AtoC = 0.0;
	distance_AtoN = 0.0;
	distance_BtoN = 0.0;
	distance_CtoN = 0.0;

	distance_AtoE = 0.0;
	distance_BtoE = 0.0;
	distance_CtoE = 0.0;
	distance_NtoE = 0.0;
    
	return 1;
}

int line_spacing::isa(char *classname)
{
	return strcmp(classname,"line_spacing")==0;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: line_spacing
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_line_spacing(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(line_spacing::oclass);
		if (*obj!=NULL)
		{
			line_spacing *my = OBJECTDATA(*obj,line_spacing);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(line_spacing);
}
EXPORT TIMESTAMP sync_line_spacing(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	return TS_NEVER;
}

EXPORT int isa_line_spacing(OBJECT *obj, char *classname)
{
	return strcmp(classname,"line_spacing") == 0;
}

/**@}**/
