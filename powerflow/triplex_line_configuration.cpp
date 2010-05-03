/** $Id: triplex_line_configuration.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file triplex_line_configuration.cpp
	@addtogroup triplex_line_configuration 
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

CLASS* triplex_line_configuration::oclass = NULL;
CLASS* triplex_line_configuration::pclass = NULL;

triplex_line_configuration::triplex_line_configuration(MODULE *mod) : line_configuration(mod)
{
	if(oclass == NULL)
	{
		pclass = line_configuration::oclass;
		
		oclass = gl_register_class(mod,"triplex_line_configuration",sizeof(triplex_line_configuration),0x00);
        if(oclass == NULL)
            GL_THROW("unable to register object class implemented by %s",__FILE__);
        
        if(gl_publish_variable(oclass,
			PT_object, "conductor_1",PADDR(phaseA_conductor),
			PT_object, "conductor_2",PADDR(phaseB_conductor),
			PT_object, "conductor_N",PADDR(phaseC_conductor),
			PT_double, "insulation_thickness[in]", PADDR(ins_thickness),
			PT_double, "diameter[in]",PADDR(diameter),
			PT_object, "spacing",PADDR(line_spacing),
			PT_complex, "z11[Ohm/mile]",PADDR(impedance11),
			PT_complex, "z12[Ohm/mile]",PADDR(impedance12),
			PT_complex, "z21[Ohm/mile]",PADDR(impedance21),
			PT_complex, "z22[Ohm/mile]",PADDR(impedance22),
            NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int triplex_line_configuration::create(void)
{
	int result = line_configuration::create();
    phaseA_conductor = NULL; 
	phaseB_conductor = NULL;
	phaseC_conductor = NULL;
	phaseN_conductor = NULL;
	ins_thickness = 0.0;
	diameter  = 0.0;
	line_spacing = NULL;
	return result;
}

int triplex_line_configuration::isa(char *classname)
{
	return strcmp(classname,"triplex_line_configuration")==0 || line_configuration::isa(classname);
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: triplex_line_configuration
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_triplex_line_configuration(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(triplex_line_configuration::oclass);
		if (*obj!=NULL)
		{
			triplex_line_configuration *my = OBJECTDATA(*obj,triplex_line_configuration);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("create_triplex_line_configuration: %s", msg);
	}
	return 0;
}
EXPORT TIMESTAMP sync_triplex_line_configuration(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	return TS_NEVER;
}

EXPORT int isa_triplex_line_configuration(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,triplex_line_configuration)->isa(classname);
}

/**@}**/
