// $Id: fuse.cpp 1182 2008-12-22 22:08:36Z dchassin $
/**	Copyright (C) 2008 Battelle Memorial Institute

	@file fuse.cpp
	@addtogroup powerflow_fuse Fuse
	@ingroup powerflow
	
	@todo fuse do not reclose ever once blown, implement fuse restoration scheme 
	(e.g., scale of hours with circuit outage)
	
	@{
*/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "node.h"
#include "fuse.h"

//initialize pointers
CLASS* fuse::oclass = NULL;
CLASS* fuse::pclass = NULL;

//////////////////////////////////////////////////////////////////////////
// fuse CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

fuse::fuse(MODULE *mod) : link(mod)
{
	if(oclass == NULL)
	{
		pclass = relay::oclass;
		
		oclass = gl_register_class(mod,"fuse",sizeof(fuse),PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
        if(oclass == NULL)
            GL_THROW("unable to register object class implemented by %s",__FILE__);
        
        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
			PT_double, "current_limit[A]",PADDR(current_limit),
			PT_enumeration, "time_curve", PADDR(time_curve),
				PT_KEYWORD, "UNKNOWN", UNKNOWN,
            NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int fuse::isa(char *classname)
{
	return strcmp(classname,"fuse")==0 || link::isa(classname);
}

int fuse::create()
{
	int result = link::create();
        
    // Set up defaults
	to = from = NULL;
	blow_time = TS_NEVER;
	time_curve = UNKNOWN;
	current_limit = 1.0;
	phases = PHASE_ABCN;

	return result;
}

/**
* Object initialization is called once after all object have been created
*
* @param parent a pointer to this object's parent
* @return 1 on success, 0 on error
*/
int fuse::init(OBJECT *parent)
{
	if (phases&PHASE_S)
		throw "fuses cannot be placed on triplex circuits";

	int result = link::init(parent);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			a_mat[i][j] = d_mat[i][j] = A_mat[i][j] = (i == j ? 1.0 : 0.0);
			c_mat[i][j] = 0.0;
			B_mat[i][j] = b_mat[i][j] = 0.0;
		}

	}
	return result;
}

TIMESTAMP fuse::sync(TIMESTAMP t0)
{
	int i = phase_index();
	OBJECT *hdr = OBJECTHDR(this);
	node *f;
	node *t;
	set reverse = get_flow(&f,&t);
	
	//complex fI[3] = {f->voltage[0]/impedance(0), f->voltage[1]/impedance(1), f->voltage[2]/impedance(2)};
	complex fI[3] = { f->current[0], f->current[1], f->current[2]};
	if (t0 == blow_time)
	{
		open();
		gl_verbose("Fuse \"%s\" popped open!", hdr->name ? hdr->name : "(anon)");
	} 
	if (fI[0].Mag()<current_limit 
		&& fI[1].Mag()<current_limit 
		&& fI[2].Mag()<current_limit) 
	{		
		close();
		blow_time = TS_NEVER;
	} else {
		open();
	}
	//else if (blow_time == TS_NEVER) {	
	//	open();
	//	switch (time_curve) {
	//		case UNKNOWN:
	//		default:
	//			blow_time = t0 + 1; // one timestep default
	//			break;
	//	}
	//}

	TIMESTAMP t1 = link::sync(t0);

	return (blow_time<t1) ? blow_time : t1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: fuse
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_fuse(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(fuse::oclass);
		if (*obj!=NULL)
		{
			fuse *my = OBJECTDATA(*obj,fuse);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	} 
	catch (char *msg)
	{
		gl_error("create_fuse: %s", msg);
	}
	return 0;
}

EXPORT int init_fuse(OBJECT *obj)
{
	fuse *my = OBJECTDATA(obj,fuse);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		GL_THROW("%s (fuse:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
}

EXPORT TIMESTAMP sync_fuse(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	fuse *pObj = OBJECTDATA(obj,fuse);
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
		GL_THROW("%s (fuse:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return 0; 
	} catch (...) {
		GL_THROW("%s (fuse:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
		return 0;
	}
}

/**
* Allows the core to discover whether obj is a subtype of this class.
*
* @param obj a pointer to this object
* @param classname the name of the object the core is testing
*
* @return true (1) if obj is a subtype of this class
*/
EXPORT int isa_fuse(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,fuse)->isa(classname);
}

/**@}*/
