/** $Id: controller.cpp 1182 2009-09-09 22:08:36Z mhauer $
	Copyright (C) 2009 Battelle Memorial Institute
	@file auction.cpp
	@defgroup controller Transactive controller, OlyPen experiment style
	@ingroup market

 **/

#include "controller2.h"

CLASS* controller2::oclass = NULL;

controller2::controller2(MODULE *module){
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"controller2",sizeof(controller2),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
		NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(controller2));
	}
}

int controller2::create(){
	memset(this, 0, sizeof(controller2));
	return 1;
}
int controller2::init(OBJECT *parent){
	return 1;
}
EXPORT int create_controller2(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(controller2::oclass);
		if (*obj!=NULL)
		{
			controller2 *my = OBJECTDATA(*obj,controller2);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_controller: %s", msg);
	}
	return 1;
}

EXPORT int init_controller2(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL){
			return OBJECTDATA(obj,controller2)->init(parent);
		}
	}
	catch (char *msg)
	{
		char name[64];
		gl_error("init_controller(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
	}
	return 1;
}

EXPORT TIMESTAMP sync_controller2(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	controller2 *my = OBJECTDATA(obj,controller2);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = TS_NEVER;
			break;
		case PC_BOTTOMUP:
			t2 = TS_NEVER;
			break;
		case PC_POSTTOPDOWN:
			t2 = TS_NEVER;
			obj->clock = t1;
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
	}
	catch (char *msg)
	{
		char name[64];
		gl_error("sync_controller(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
	}
	return t2;
}

// EOF