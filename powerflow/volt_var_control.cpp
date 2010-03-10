// $Id: ivvc_controller object 8/24/2009

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "volt_var_control.h"

//////////////////////////////////////////////////////////////////////////
// ivvc_controller CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* volt_var_control::oclass = NULL;
CLASS* volt_var_control::pclass = NULL;

/**
* constructor.  Class registration is only called once to 
* register the class with the core. Include parent class constructor (node)
*
* @param mod a module structure maintained by the core
*/
volt_var_control::volt_var_control(MODULE *mod):node(mod)
{
	if(oclass == NULL)
	{
		pclass = node::oclass;
		
		oclass = gl_register_class(mod,"volt_var_control",sizeof(volt_var_control),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
        if(oclass == NULL)
            GL_THROW("unable to register object class implemented by %s",__FILE__);

		if(gl_publish_variable(oclass,

			// Enumeration of user variables goes here
			PT_double, "qualification_time[s]", PADDR(qualification_time),

		NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

	}
}

int volt_var_control::create()
{
	int result = node::create();

	// Initial values go here

	return result;
}

int volt_var_control::init(OBJECT *parent)
{
	int result = node::init();

	// Inital setup and value checks go here

	return result;
}

TIMESTAMP volt_var_control::presync(TIMESTAMP t0)
{	
	// All of your control algorithms will mostly go here.

	return TS_NEVER;
}

int volt_var_control::isa(char *classname)
{
	return strcmp(classname,"volt_var_control")==0 || node::isa(classname);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: volt_var_control
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_volt_var_control(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(volt_var_control::oclass);
		if (*obj!=NULL)
		{
			volt_var_control *my = OBJECTDATA(*obj,volt_var_control);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("create_volt_var_control: %s", msg);
	}
	return 0;
}


/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_volt_var_control(OBJECT *obj)
{
	volt_var_control *my = OBJECTDATA(obj,volt_var_control);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		GL_THROW("%s (volt_var_control:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_volt_var_control(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	volt_var_control *pObj = OBJECTDATA(obj,volt_var_control);
	try {
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
	} catch (const char *error) {
		GL_THROW("%s (volt_var_control:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return 0; 
	} catch (...) {
		GL_THROW("%s (volt_var_control:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
		return 0;
	}
}

/**
* Allows the core to discover whether obj is a subtype of this class.
*
* @param obj a pointer to this object
* @param classname the name of the object the core is testing
*
* @return 0 if obj is a subtype of this class
*/
EXPORT int isa_volt_var_control(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,volt_var_control)->isa(classname);
}

/**@}*/
