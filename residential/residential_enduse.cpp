/** $Id $
	Copyright (C) 2008 Battelle Memorial Institute
	@file residential_enduse.cpp
	@addtogroup residential_enduse Residential enduses
	@ingroup residential

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "lock.h"
#include "residential.h"
#include "residential_enduse.h"

//////////////////////////////////////////////////////////////////////////
// CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* residential_enduse::oclass = NULL;
residential_enduse *residential_enduse::defaults = NULL;

// the constructor registers the class and properties and sets the defaults
residential_enduse::residential_enduse(MODULE *mod) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"residential_enduse",sizeof(residential_enduse),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the residential_enduse class failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_loadshape,"shape",PADDR(shape),
			PT_enduse,"",PADDR(load),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the residential_enduse properties failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */

		// setup the default values
		defaults = this;
		memset(this,0,sizeof(residential_enduse));
	}
}

// create is called every time a new object is loaded
int residential_enduse::create(void) 
{
	// copy the defaults
	memcpy(this,defaults,sizeof(residential_enduse));

	// attach loadshape 
	load.shape = &shape;
	return 1;
}

int residential_enduse::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	//	pull parent attach_enduse and attach the enduseload
	ATTACHFUNCTION *attach = (ATTACHFUNCTION*)(gl_get_function(parent, "attach_enduse"));
	if(attach)
		pCircuit = (*attach)(parent, hdr, 15, false, &load);
	else if (parent)
		gl_warning("%s (%s:%d) parent %s (%s:%d) does not export attach_enduse function so voltage response cannot be modeled", hdr->name?hdr->name:"(unnamed)", hdr->oclass->name, hdr->id, parent->name?parent->name:"(unnamed)", parent->oclass->name, parent->id);
		/* TROUBLESHOOT
			Enduses must have a voltage source from a parent object that exports an attach_enduse function.  
			The residential_enduse object references a parent object that does not conform with this requirement.
			Fix the parent reference and try again.
		 */


	// setup the default residential_enduse if needed
	if (load.shape==NULL)
		throw "load.shape is not attached to loadshape";
		/* TROUBLESHOOT
			This is an internal error.  Please report this problem.
		 */

	if (load.shape->schedule==NULL)
	{
		gl_warning("%s (%s:%d) schedule is not specified so the load is inactive", hdr->name?hdr->name:"(unnamed)", hdr->oclass->name, hdr->id);
		/* TROUBLESHOOT
			The residential_enduse object requires a schedule that defines how
			the load behaves.  Omitting this schedule effectively shuts the enduse
			load off and this is not typically intended.
		 */
	}

	return 1;
}

TIMESTAMP residential_enduse::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	gl_debug("%s shape load = %8g", obj->name, gl_get_loadshape_value(&shape));
	return shape.t2>t1 ? shape.t2 : TS_NEVER; 
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_residential_enduse(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(residential_enduse::oclass);
	if (*obj!=NULL)
	{
		residential_enduse *my = OBJECTDATA(*obj,residential_enduse);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_residential_enduse(OBJECT *obj)
{
	residential_enduse *my = OBJECTDATA(obj,residential_enduse);
	return my->init(obj->parent);
}

EXPORT TIMESTAMP sync_residential_enduse(OBJECT *obj, TIMESTAMP t0)
{
	residential_enduse *my = OBJECTDATA(obj,residential_enduse);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/
