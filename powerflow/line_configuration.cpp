/** $Id: line_configuration.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file line_configuration.cpp
	@addtogroup line_configuration 
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


//////////////////////////////////////////////////////////////////////////
// line_configuration CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

CLASS* line_configuration::oclass = NULL;
CLASS* line_configuration::pclass = NULL;

line_configuration::line_configuration(MODULE *mod) : powerflow_library(mod)
{
	if(oclass == NULL)
	{
		oclass = oclass = gl_register_class(mod,"line_configuration",sizeof(line_configuration),0x00);
        if(oclass == NULL)
            GL_THROW("unable to register line_configuration class implemented by %s",__FILE__);
        
        if(gl_publish_variable(oclass,
			PT_object, "conductor_A",PADDR(phaseA_conductor),
			PT_object, "conductor_B",PADDR(phaseB_conductor),
			PT_object, "conductor_C",PADDR(phaseC_conductor),
			PT_object, "conductor_N",PADDR(phaseN_conductor),
			PT_object, "spacing",PADDR(line_spacing),
            NULL) < 1) GL_THROW("unable to publish line_configuration properties in %s",__FILE__);
    }
}

int line_configuration::create(void)
{
    // Set up defaults
    phaseA_conductor = NULL; 
	phaseB_conductor = NULL;
	phaseC_conductor = NULL;
	phaseN_conductor = NULL;
	line_spacing = NULL;

	return 1;
}

int line_configuration::isa(char *classname)
{
	return strcmp(classname,"line_configuration")==0;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: line_configuration
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_line_configuration(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(line_configuration::oclass);
		if (*obj!=NULL)
		{
			line_configuration *my = OBJECTDATA(*obj,line_configuration);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_line_configuration: %s", msg);
	}
	return 0;
}
EXPORT TIMESTAMP sync_line_configuration(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	return TS_NEVER;
}

EXPORT int isa_line_configuration(OBJECT *obj, char *classname)
{
	return strcmp(classname,"line_configuration") == 0;
}

/**@}**/
