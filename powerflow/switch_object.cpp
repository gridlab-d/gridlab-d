/** $Id: switch_object.cpp,v 1.6 2008/02/06 19:15:26 natet Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file switch_object.cpp
	@addtogroup powerflow switch_object
	@ingroup powerflow

	Implements a switch object.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "switch_object.h"
#include "node.h"

EXPORT int64 switch_close(OBJECT *obj)
{
	if (!gl_object_isa(obj,"switch"))
	{
		GL_THROW("powerflow.switch_close(): %s object (%s:%d) is not a switch", obj->name?obj->name:"anonymous", obj->oclass->name,obj->id);
		return 0;
	}
	switch_object *pSwitch = OBJECTDATA(obj,switch_object);
	return (int64)pSwitch->close();
}

EXPORT int64 switch_open(OBJECT *obj)
{
	if (!gl_object_isa(obj,"switch"))
	{
		GL_THROW("powerflow::switch_open(): %s object (%s:%d) is not a switch", obj->name?obj->name:"anonymous", obj->oclass->name,obj->id);
		return 0;
	}
	switch_object *pSwitch = OBJECTDATA(obj,switch_object);
	return (int64)pSwitch->open();
}

EXPORT int64 switch_status(OBJECT *obj)
{
	if (!gl_object_isa(obj,"switch"))
	{
		GL_THROW("powerflow::switch_open(): %s object (%s:%d) is not a switch", obj->name?obj->name:"anonymous", obj->oclass->name,obj->id);
		return 0;
	}
	switch_object *pSwitch = OBJECTDATA(obj,switch_object);
	return (int64)pSwitch->get_status();
}

//////////////////////////////////////////////////////////////////////////
// switch_object CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* switch_object::oclass = NULL;
CLASS* switch_object::pclass = NULL;

switch_object::switch_object(MODULE *mod) : link(mod)
{
	if(oclass == NULL)
	{
		pclass = link::oclass;

		oclass = gl_register_class(mod,"switch",sizeof(switch_object),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
        if(oclass == NULL)
            GL_THROW("unable to register object class implemented by %s",__FILE__);

        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		if (gl_publish_function(oclass,"close",(FUNCTIONADDR)switch_close)==NULL)
			GL_THROW("unable to publish switch close function");
		if (gl_publish_function(oclass,"open",(FUNCTIONADDR)switch_open)==NULL)
			GL_THROW("unable to publish switch open function");
		if (gl_publish_function(oclass,"status",(FUNCTIONADDR)switch_status)==NULL)
			GL_THROW("unable to publish switch open function");
    }
}

int switch_object::isa(char *classname)
{
	return strcmp(classname,"switch")==0 || link::isa(classname);
}

int switch_object::create()
{
	int result = link::create();
	return result;
}

int switch_object::init(OBJECT *parent)
{
	int result = link::init(parent);

	a_mat[0][0] = d_mat[0][0] = A_mat[0][0] = (is_closed() && has_phase(PHASE_A) ? 1.0 : 0.0);
	a_mat[1][1] = d_mat[1][1] = A_mat[1][1] = (is_closed() && has_phase(PHASE_B) ? 1.0 : 0.0);
	a_mat[2][2] = d_mat[2][2] = A_mat[2][2] = (is_closed() && has_phase(PHASE_C) ? 1.0 : 0.0);

	b_mat[0][0] = c_mat[0][0] = B_mat[0][0] = 0.0;
	b_mat[1][1] = c_mat[1][1] = B_mat[1][1] = 0.0;
	b_mat[2][2] = c_mat[2][2] = B_mat[2][2] = 0.0;

	return result;
}

TIMESTAMP switch_object::sync(TIMESTAMP t0)
{
	TIMESTAMP t1 = TS_NEVER;
	node *f;
	node *t;
	set reverse = get_flow(&f,&t);
#ifdef SUPPORT_OUTAGES
	set trip = (f->is_contact_any() || t->is_contact_any());

	/* perform switch_object operation if any line contact has occurred */
	if (status==LS_CLOSED && trip)
	{
		status = LS_OPEN;
		t1 = TS_NEVER;
	}
	else if (status==LS_OPEN)
	{
		if (trip)
		{
			{
				t1 = t0+(TIMESTAMP)(gl_random_exponential(3600)*TS_SECOND);
			}
		}
		//else
		//	status = RS_CLOSED;
	}
#endif

	TIMESTAMP t2=link::sync(t0);

	return t1<t2?t1:t2;
}

TIMESTAMP switch_object::postsync(TIMESTAMP t0)
{
	TIMESTAMP t2 = link::postsync(t0);
	node *f;
	node *t;
	set reverse = get_flow(&f,&t);
	
	return t2;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: switch_object
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int commit_switch(OBJECT *obj)
{
	if (solver_method==SM_FBS)
	{
		link *plink = OBJECTDATA(obj,link);
		plink->calculate_power();
	}
	return 1;
}
EXPORT int create_switch(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(switch_object::oclass);
		if (*obj!=NULL)
		{
			switch_object *my = OBJECTDATA(*obj,switch_object);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_switch: %s", msg);
	}
	return 0;
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_switch(OBJECT *obj)
{
	switch_object *my = OBJECTDATA(obj,switch_object);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		GL_THROW("%s (switch:%d): %s", my->get_name(), my->get_id(), msg);
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
EXPORT TIMESTAMP sync_switch(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	switch_object *pObj = OBJECTDATA(obj,switch_object);
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
		GL_THROW("%s (switch:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return 0;
	} catch (...) {
		GL_THROW("%s (switch:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
		return 0;
	}
}

EXPORT int isa_switch(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,switch_object)->isa(classname);
}

/**@}**/
