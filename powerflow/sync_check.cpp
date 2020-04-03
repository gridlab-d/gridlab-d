/** $Id: sync_check

    Implements sychronization check functionality for switches to close
    when two grids are within parameters

	Copyright (C) 2020 Battelle Memorial Institute
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "sync_check.h"

//////////////////////////////////////////////////////////////////////////
// sync_check CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* sync_check::oclass = NULL;
CLASS* sync_check::pclass = NULL;

sync_check::sync_check(MODULE *mod) : powerflow_object(mod)
{
	if(oclass == NULL)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod,"sync_check",sizeof(sync_check),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if(oclass == NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
		if(gl_publish_variable(oclass,
			PT_bool, "arm_sync",PADDR(arm_sync),PT_DESCRIPTION,"Flag to arm the synchronization close",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int sync_check::isa(char *classname)
{
	return strcmp(classname,"sync_check")==0;
}

int sync_check::create(void)
{
	int result = powerflow_object::create();

    arm_sync = false;   //Start disabled

	return result;
}

int sync_check::init(OBJECT *parent)
{
	int retval = powerflow_object::init(parent);

	return retval;
}

TIMESTAMP sync_check::presync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::presync(t0);
	
    return tret;
}

TIMESTAMP sync_check::sync(TIMESTAMP t0)
{
    OBJECT *obj = OBJECTHDR(this);
    TIMESTAMP tret = powerflow_object::sync(t0);

    return tret;
}

TIMESTAMP sync_check::postsync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::postsync(t0);
	
    return tret;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: sync_check
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_sync_check(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(sync_check::oclass);
		if (*obj!=NULL)
		{
			sync_check *my = OBJECTDATA(*obj,sync_check);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(sync_check);
}

EXPORT int init_sync_check(OBJECT *obj)
{
	try {
		sync_check *my = OBJECTDATA(obj,sync_check);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(sync_check);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_sync_check(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		sync_check *pObj = OBJECTDATA(obj,sync_check);
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(t0);
		case PC_BOTTOMUP:
			return pObj->sync(t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
	}
	SYNC_CATCHALL(sync_check);
}

EXPORT int isa_sync_check(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,sync_check)->isa(classname);
}

/**@}**/
