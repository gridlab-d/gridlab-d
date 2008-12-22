// $Id: capacitor.cpp 1182 2008-12-22 22:08:36Z dchassin $
/**	Copyright (C) 2008 Battelle Memorial Institute

	@file capacitor.cpp
	@addtogroup capacitor
	@ingroup powerflow
	@{
*/

#include <stdlib.h>	
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "capacitor.h"

//////////////////////////////////////////////////////////////////////////
// capacitor CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* capacitor::oclass = NULL;
CLASS* capacitor::pclass = NULL;

/**
* constructor.  Class registration is only called once to 
* register the class with the core. Include parent class constructor (node)
*
* @param mod a module structure maintained by the core
*/
capacitor::capacitor(MODULE *mod):node(mod)
{
	if(oclass == NULL)
	{
		pclass = node::oclass;
		
		oclass = gl_register_class(mod,"capacitor",sizeof(capacitor),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
        if(oclass == NULL)
            GL_THROW("unable to register object class implemented by %s",__FILE__);

		if(gl_publish_variable(oclass,
			PT_INHERIT, "node",
			PT_set, "pt_phase", PADDR(pt_phase),
				PT_KEYWORD, "A",PHASE_A,
				PT_KEYWORD, "B",PHASE_B,
				PT_KEYWORD, "C",PHASE_C,
			PT_enumeration, "switch", switch_state,
				PT_KEYWORD, "OPEN", OPEN,
				PT_KEYWORD, "CLOSED", CLOSED,
			PT_enumeration, "control", control,
				PT_KEYWORD, "MANUAL", MANUAL,
				PT_KEYWORD, "VAR", VAR,
				PT_KEYWORD, "VOLT", VOLT,
				PT_KEYWORD, "VARVOLT", VARVOLT,
         	NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int capacitor::create()
{
	int result = node::create();
		
	// Set up defaults
	switch_state = CLOSED;
	control = MANUAL;
	var_close = 0.0;
	var_open = 0.0;
	volt_close = 0.0;
	volt_open = 0.0;
	pt_ratio = 60;
	pt_phase = PHASE_A;
	time_delay = 0;
	time_to_change = 0.0;

	throw "capacitor implementation is not complete";
	return result;
}

TIMESTAMP capacitor::sync(TIMESTAMP t0)
{
	if (switch_state==CLOSED) {
		switch (control) {
			case MANUAL:  // manual
				/// @todo implement capacity manual control closed (ticket #189)
				break;
			case VAR:  // VAr
				/// @todo implement capacity var control closed (ticket #190)
				break;
			case VOLT:  // V
				/// @todo implement capacity volt control closed (ticket #191)
				break;
			case VARVOLT:  // VAr, V
				/// @todo implement capacity varvolt control closed (ticket #192)
				break;
			default:
				break;
		}
	} else {
		switch (control) {
			case MANUAL:  // manual
				/// @todo implement capacity manual control open (ticket #193)
				break;
			case VAR:  // VAr
				/// @todo implement capacity var control open (ticket #194)
				break;
			case VOLT:  // V
				/// @todo implement capacity volt control open (ticket #195)
				break;
			case VARVOLT:  // VAr, V
				/// @todo implement capacity varvolt control open (ticket #196)
				break;
			default:
				break;
		}
	}
	return node::sync(t0);
}

int capacitor::isa(char *classname)
{
	return strcmp(classname,"capacitor")==0 || node::isa(classname);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: capacitor
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_capacitor(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(capacitor::oclass);
		if (*obj!=NULL)
		{
			capacitor *my = OBJECTDATA(*obj,capacitor);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_capacitor: %s", msg);
	}
	return 0;
}



/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_capacitor(OBJECT *obj)
{
	capacitor *my = OBJECTDATA(obj,capacitor);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		GL_THROW("%s (capacitor:%d): %s", my->get_name(), my->get_id(), msg);
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
EXPORT TIMESTAMP sync_capacitor(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	capacitor *pObj = OBJECTDATA(obj,capacitor);
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
		GL_THROW("%s (capacitor:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return 0; 
	} catch (...) {
		GL_THROW("%s (capacitor:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
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
EXPORT int isa_capacitor(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,capacitor)->isa(classname);
}

/**@}*/
