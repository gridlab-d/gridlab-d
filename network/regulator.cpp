/** $Id: regulator.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file regulator.cpp
	@addtogroup regulator Voltage regulator
	@ingroup network
	
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"



//////////////////////////////////////////////////////////////////////////
// regulator CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* regulator::oclass = NULL;
CLASS* regulator::pclass = NULL;
regulator *regulator::defaults = NULL;
CLASS *regulator_class = (NULL);

regulator::regulator(MODULE *mod) : link(mod)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		regulator_class = oclass = gl_register_class(mod,"regulator",sizeof(regulator),PC_BOTTOMUP);
		if (oclass==NULL)
			throw "unable to register class regulator";
		else
			oclass->trl = TRL_CONCEPT;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_enumeration,"Type",PADDR(Type),
				PT_KEYWORD,"RT_LTC",RT_LTC,
				PT_KEYWORD,"RT_VR",RT_VR,
			PT_double, "Vmax", PADDR(Vmax),
			PT_double, "Vmin", PADDR(Vmin),
			PT_double, "Vstep", PADDR(Vstep),
			PT_object, "CTlink", PADDR(CTlink),
			PT_object, "PTbus", PADDR(PTbus),
			PT_double, "TimeDelay", PADDR(TimeDelay),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
	}

}

int regulator::create() 
{
	int result = link::create();
	memcpy(this,defaults,sizeof(*this));
	return result;
}

TIMESTAMP regulator::sync(TIMESTAMP t0) 
{
	node *f = OBJECTDATA(from,node);
	node *t = OBJECTDATA(to,node);
	if (f==NULL || t==NULL)
		return TS_NEVER;
	// TODO: update regulator state
	return link::sync(t0);
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: regulator
//////////////////////////////////////////////////////////////////////////

EXPORT int create_regulator(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(regulator_class);
	if (*obj!=NULL)
	{
		regulator *my = OBJECTDATA(*obj,regulator);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT TIMESTAMP sync_regulator(OBJECT *obj, TIMESTAMP t0)
{
	TIMESTAMP t1 = OBJECTDATA(obj,regulator)->sync(t0);
	obj->clock = t0;
	return t1;
}


/**@}*/
