// $Id: motor.cpp 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#include <stdlib.h>	
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "motor.h"

//////////////////////////////////////////////////////////////////////////
// capacitor CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* motor::oclass = NULL;
CLASS* motor::pclass = NULL;

/**
* constructor.  Class registration is only called once to 
* register the class with the core. Include parent class constructor (node)
*
* @param mod a module structure maintained by the core
*/
motor::motor(MODULE *mod):node(mod)
{
	if(oclass == NULL)
	{
		pclass = node::oclass;
		
		oclass = gl_register_class(mod,"motor",sizeof(motor),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class motor";
		else
			oclass->trl = TRL_QUALIFIED;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "node",
			
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int motor::create()
{
	int result = node::create();
	return result;
}

int motor::init(OBJECT *parent)
{
	int result = node::init();

	OBJECT *obj = OBJECTHDR(this);
	return result;
}
TIMESTAMP motor::presync(TIMESTAMP t0)
{
	return TS_NEVER;
}
TIMESTAMP motor::sync(TIMESTAMP t0)
{
	return TS_NEVER;
}
TIMESTAMP motor::postsync(TIMESTAMP t0)
{
	return TS_NEVER;
}
int motor::isa(char *classname)
{
	return strcmp(classname,"motor")==0 || node::isa(classname);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: motor
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_motor(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(motor::oclass);
		if (*obj!=NULL)
		{
			motor *my = OBJECTDATA(*obj,motor);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(motor);
}



/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_motor(OBJECT *obj)
{
	try {
		motor *my = OBJECTDATA(obj,motor);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(motor);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_motor(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		motor *pObj = OBJECTDATA(obj,motor);
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
	SYNC_CATCHALL(motor);
}

/**
* Allows the core to discover whether obj is a subtype of this class.
*
* @param obj a pointer to this object
* @param classname the name of the object the core is testing
*
* @return 0 if obj is a subtype of this class
*/
EXPORT int isa_motor(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,motor)->isa(classname);
}

/**@}*/