/** $Id: triplex_line_configuration.cpp 4738 2014-07-03 00:55:39Z dchassin $
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
		if (oclass==NULL)
			throw "unable to register class triplex_line_configuration";
		else
			oclass->trl = TRL_PROVEN;

        if(gl_publish_variable(oclass,
			PT_object, "conductor_1",PADDR(phaseA_conductor),PT_DESCRIPTION,"conductor type for phase 1",
			PT_object, "conductor_2",PADDR(phaseB_conductor),PT_DESCRIPTION,"conductor type for phase 2",
			PT_object, "conductor_N",PADDR(phaseC_conductor),PT_DESCRIPTION,"conductor type for phase N",
			PT_double, "insulation_thickness[in]", PADDR(ins_thickness),PT_DESCRIPTION,"thickness of insulation around cabling",
			PT_double, "diameter[in]",PADDR(diameter),PT_DESCRIPTION,"total diameter of cable",
			PT_object, "spacing",PADDR(line_spacing),PT_DESCRIPTION,"defines the line spacing configuration",
			PT_complex, "z11[Ohm/mile]",PADDR(impedance11),PT_DESCRIPTION,"phase 1 self-impedance, used for direct entry of impedance values",
			PT_complex, "z12[Ohm/mile]",PADDR(impedance12),PT_DESCRIPTION,"phase 1-2 induced impedance, used for direct entry of impedance values",
			PT_complex, "z21[Ohm/mile]",PADDR(impedance21),PT_DESCRIPTION,"phase 2-1 induced impedance, used for direct entry of impedance values",
			PT_complex, "z22[Ohm/mile]",PADDR(impedance22),PT_DESCRIPTION,"phase 2 self-impedance, used for direct entry of impedance values",
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
		else
			return 0;
	}
	CREATE_CATCHALL(triplex_line_configuration);
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
