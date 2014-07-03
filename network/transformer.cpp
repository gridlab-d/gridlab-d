/** $Id: transformer.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file transformer.cpp
	@addtogroup transformer Transformer
	@ingroup network
	
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"




//////////////////////////////////////////////////////////////////////////
// transformer CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* transformer::oclass = NULL;
CLASS* transformer::pclass = NULL;
transformer *transformer::defaults = NULL;

CLASS *transformer_class = (NULL);

transformer::transformer(MODULE *mod) : link(mod)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		transformer_class = oclass = gl_register_class(mod,"transformer",sizeof(transformer),PC_BOTTOMUP);
		if (oclass==NULL)
			throw "unable to register class transformer";
		else
			oclass->trl = TRL_CONCEPT;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_enumeration,"Type",PADDR(Type),
				PT_KEYWORD,"TT_YY",TT_YY,
				PT_KEYWORD,"TT_YD",TT_YD,
				PT_KEYWORD,"TT_DY",TT_DY,
				PT_KEYWORD,"TT_DD",TT_DD,
			PT_double, "Sbase", PADDR(Sbase),
			PT_double, "Vbase", PADDR(Vbase),
			PT_double, "Zpu", PADDR(Zpu),
			PT_double, "Vprimary", PADDR(Vprimary),
			PT_double, "Vsecondary", PADDR(Vsecondary),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
	}
}

int transformer::create() 
{
	int result = link::create();
	memcpy(this,defaults,sizeof(*this));
	return result;
}

TIMESTAMP transformer::sync(TIMESTAMP t0) 
{
	node *f = OBJECTDATA(from,node);
	node *t = OBJECTDATA(to,node);
	if (f==NULL || t==NULL)
		return TS_NEVER;
	// TODO: update transformer state
	return link::sync(t0);
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: transformer
//////////////////////////////////////////////////////////////////////////

EXPORT int create_transformer(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(transformer_class);
	if (*obj!=NULL)
	{
		transformer *my = OBJECTDATA(*obj,transformer);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT TIMESTAMP sync_transformer(OBJECT *obj, TIMESTAMP t0)
{
	TIMESTAMP t1 = OBJECTDATA(obj,transformer)->sync(t0);
	obj->clock = t0;
	return t1;
}

/**@}*/
