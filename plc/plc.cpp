/** $Id: plc.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file plc.cpp
	@addtogroup plc Programmable logic controllers (plc)
	@ingroup modules

	The PLC module can load any DLL that has the following exports

	- \p _data	This is the data structure for variables that must be linked
	- \p _init	This is the code that must be called after the DLL is loaded
		   but before \p _code is called for the first time.  This must
		   return the number of seconds before the first call to \p _code
	- \p _code    This is the code that must be called each time to PLC syncs.
		   This must return the number of seconds before the next sync.

	The data structure must conform to the following structure:
	@code
	struct {
	 char *name;
	 enum {DT_INTEGER=6, DT_DOUBLE=1} type;
	 void *addr;
	} data[];
	@endcode
	with the last entry having a \p NULL name (this is \e very important).

	The compiler will automatically create a conforming DLL from a \p .plc file
	having the following typical structure

	See \ref machine "Machine Implementation" for an example of PLC code
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "plc.h"

CLASS *plc::oclass = NULL; ///< a pointer to the registered object class definition
plc *plc::defaults = NULL; ///< a pointer to the default values for new objects

/** Register the new object class and construct the default object properties */
plc::plc(MODULE *mod)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"plc",sizeof(plc),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class plc";
		else
			oclass->trl = TRL_INTEGRATED;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_char1024,"source",PADDR(source),
			PT_object,"network",PADDR(network),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// set defaults
		memset(this,0,sizeof(plc));
		strcpy(source,"");
		controller = NULL;
		network = NULL;
		defaults = this;
	}
}

/** Create a new object */
int plc::create()
{
	// copy the defaults
	memcpy(this,defaults,sizeof(plc));

	// set object specific defaults
	controller = new machine;
	return 1;
}

/** Initialize the new object */
int plc::init(OBJECT *parent)
{
	if (strcmp(source,"")==0 && parent!=NULL) /* default source */
		sprintf(source.get_string(),"%s.plc",parent->oclass->name);
	if (controller->compile(source)<0)
		GL_THROW("%s: PLC compile failed", source.get_string());
	if (controller->init(parent)<0)
		GL_THROW("%s: PLC init failed", source.get_string());
	parent->flags |= OF_HASPLC; /* enable external PLC flag */
	
	if (network)
	{
		controller->connect(OBJECTDATA(network,comm));
		gl_verbose("machine %x has connected to network '%s'", controller, network->name?network->name:"anonymous");
	}

	return 1; // return 0 on failure
}

/** Synchronize the object */
TIMESTAMP plc::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	if (t0>0)
	{
		int dt = controller->run((double)(t1-t0)/TS_SECOND);
		if (dt<0)
			return TS_INVALID;
		else if (dt==0x7fffffff)
			return TS_NEVER;
		else
			return t1+(TIMESTAMP)(dt*TS_SECOND);
	}
	else
		return t1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////
EXPORT int create_plc(OBJECT **obj, OBJECT *parent)
{
	try 
	{
		*obj = gl_create_object(plc::oclass);
		if (*obj!=NULL)
		{
			plc *my = OBJECTDATA(*obj,plc);
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		else
			return 0;
	}
	CREATE_CATCHALL(plc);
}

EXPORT int init_plc(OBJECT *obj)
{
	try
	{
		if (obj!=NULL)
		{
			plc *my = OBJECTDATA(obj,plc);
			return my->init(obj->parent);
		}
		else
			return 0;
	}
	INIT_CATCHALL(plc);
}

EXPORT TIMESTAMP sync_plc(OBJECT *obj, TIMESTAMP t0)
{
	try 
	{
		TIMESTAMP t1 = OBJECTDATA(obj,plc)->sync(obj->clock,t0);
		obj->clock = t0;
		return t1;
	}
	SYNC_CATCHALL(plc);
}

/**@}*/
