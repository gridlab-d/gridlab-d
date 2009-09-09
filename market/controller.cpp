/** $Id: controller.cpp 1182 2009-09-09 22:08:36Z mhauer $
	Copyright (C) 2009 Battelle Memorial Institute
	@file auction.cpp
	@defgroup controller Transactive controller, OlyPen experiment style
	@ingroup market

 **/

#include "controller.h"

CLASS* controller::oclass = NULL;

controller::controller(MODULE *module){
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"controller",sizeof(auction),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
			PT_enumeration, "type", PADDR(type), PT_DESCRIPTION, "type of transactive controller",
				PT_KEYWORD, "SINGLE", TC_SINGLE,
				PT_KEYWORD, "DOUBLE", TC_DOUBLE,
			PT_char32, "target", PADDR(target), PT_DESCRIPTION, "target property to control",
			PT_char32, "target2", PADDR(target2), PT_DESCRIPTION, "second target property to control",
			PT_char32, "monitor", PADDR(monitor), PT_DESCRIPTION, "property to monitor",
			PT_double, "ramp_low", PADDR(ramp_low),
			PT_double, "ramp_high", PADDR(ramp_high),
			PT_double, "ramp2_low", PADDR(ramp2_low),
			PT_double, "ramp2_high", PADDR(ramp2_high),
			PT_double, "min", PADDR(min1),
			PT_double, "max", PADDR(max1),
			PT_double, "min2", PADDR(min2),
			PT_double, "max2", PADDR(max2),
			PT_double, "base", PADDR(base1), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "original value of the first target property",
			PT_double, "base2", PADDR(base2), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "original value of the second target property",
			PT_double, "bidprice", PADDR(bid_price), PT_ACCESS, PA_REFERENCE,
			PT_double, "bidquantity", PADDR(bid_quantity), PT_ACCESS, PA_REFERENCE,
			PT_int64, "market_id", PADDR(market_id), PT_DESCRIPTION, "unique identifier of market clearing",
			PT_object, "market", PADDR(market), PT_DESCRIPTION, "the specific market used by the controller",
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(controller));
	}
}

int controller::create(){
	memset(this, 0, sizeof(controller));
	return 1;
}

int controller::init(OBJECT *parent){

	return 1;
}

TIMESTAMP controller::presync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

TIMESTAMP controller::sync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

TIMESTAMP controller::postsync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_controller(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(controller::oclass);
		if (*obj!=NULL)
		{
			controller *my = OBJECTDATA(*obj,controller);
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

EXPORT int init_controller(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL){
			return OBJECTDATA(obj,controller)->init(parent);
		}
	}
	catch (char *msg)
	{
		char name[64];
		gl_error("init_controller(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
	}
	return 1;
}

EXPORT TIMESTAMP sync_controller(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	controller *my = OBJECTDATA(obj,controller);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
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