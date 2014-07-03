/** $Id: capbank.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file capbank.cpp
	@addtogroup capbank Capacitor bank
	@ingroup network
	
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"

//////////////////////////////////////////////////////////////////////////
// capbank CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS *capbank::oclass = NULL;
CLASS *capbank::pclass = NULL;
capbank *capbank::defaults = NULL;
CLASS *capbank_class = NULL;

capbank::capbank(MODULE *mod) : link(mod)
{
	// TODO: set default values
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		capbank_class = oclass = gl_register_class(mod,"capbank",sizeof(capbank),PC_BOTTOMUP);
		if (oclass==NULL)
			throw "unable to register class capbank";
		else
			oclass->trl = TRL_CONCEPT;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_double, "KVARrated", PADDR(KVARrated),
			PT_double, "Vrated", PADDR(Vrated),
			PT_enumeration,"state",PADDR(state),
				PT_KEYWORD,"CAPS_IN",CAPS_IN,
				PT_KEYWORD,"CAPS_OUT",CAPS_OUT,
			PT_object, "CTlink", PADDR(CTlink),
			PT_object, "PTnode", PADDR(PTnode),
			PT_double, "VARopen", PADDR(VARopen),
			PT_double, "VARclose", PADDR(VARclose),
			PT_double, "Vopen", PADDR(Vopen),
			PT_double, "Vclose", PADDR(Vclose),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
	}
}

int capbank::create() 
{
	int result = link::create();
	memcpy(this,defaults,sizeof(*this));
	return result;
}

TIMESTAMP capbank::sync(TIMESTAMP t0) 
{
	node *f = OBJECTDATA(from,node);
	node *t = OBJECTDATA(to,node);
	if (f==NULL || t==NULL)
		return TS_NEVER;
	// TODO: update capbank state
	return link::sync(t0);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: capbank
//////////////////////////////////////////////////////////////////////////

EXPORT int create_capbank(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(capbank_class);
	if (*obj!=NULL)
	{
		capbank *my = OBJECTDATA(*obj,capbank);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT TIMESTAMP sync_capbank(OBJECT *obj, TIMESTAMP t0)
{
	TIMESTAMP t1 = OBJECTDATA(obj,capbank)->sync(t0);
	obj->clock = t0;
	return t1;
}


/**@}*/
