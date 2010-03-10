/** $Id: line_spacing.cpp 1182 2008-12-22 22:08:36Z dchassin $
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
        if(oclass == NULL)
            GL_THROW("unable to register line_spacing class implemented by %s",__FILE__);
        
        if(gl_publish_variable(oclass,
			PT_double, "distance_AB[ft]",PADDR(distance_AtoB),   
			PT_double, "distance_BC[ft]",PADDR(distance_BtoC),   
			PT_double, "distance_AC[ft]",PADDR(distance_AtoC),   
			PT_double, "distance_AN[ft]",PADDR(distance_AtoN),   
			PT_double, "distance_BN[ft]",PADDR(distance_BtoN),   
			PT_double, "distance_CN[ft]",PADDR(distance_CtoN),   
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
	}
	catch (const char *msg)
	{
		gl_error("create_line_spacing: %s", msg);
	}
	return 0;
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
