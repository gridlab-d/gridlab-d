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
			PT_char32, "target1", PADDR(target1), PT_DESCRIPTION, "target property to control",
			PT_char32, "target2", PADDR(target2), PT_DESCRIPTION, "second target property to control",
			PT_char32, "monitor", PADDR(monitor), PT_DESCRIPTION, "property to monitor",
			PT_double, "ramp1_low", PADDR(ramp1_low), PT_DESCRIPTION, "price ramp for the low end of the first target property",
			PT_double, "ramp1_high", PADDR(ramp1_high), PT_DESCRIPTION, "price ramp for the high end of the first target property",
			PT_double, "ramp2_low", PADDR(ramp2_low), PT_DESCRIPTION, "price ramp for the low end of the second target property",
			PT_double, "ramp2_high", PADDR(ramp2_high), PT_DESCRIPTION, "price ramp for the high end of the second target property",
			PT_double, "min1", PADDR(min1), PT_DESCRIPTION, "lower offset for the first target range",
			PT_double, "max1", PADDR(max1), PT_DESCRIPTION, "upper offset for the first target range",
			PT_double, "min2", PADDR(min2), PT_DESCRIPTION, "lower offset for the second target range",
			PT_double, "max2", PADDR(max2), PT_DESCRIPTION, "upper offset for the second target range",
			PT_double, "base1", PADDR(base1), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "original value of the first target property",
			PT_double, "base2", PADDR(base2), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "original value of the second target property",
			PT_double, "set1", PADDR(set1), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "setpoint for the first target property",
			PT_double, "set2", PADDR(set2), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "setpoint for the second target property",
			PT_double, "bidprice", PADDR(bid_price), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "last price bid to the market",
			PT_double, "bidquantity", PADDR(bid_quantity), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "last power quantity bid to the market",
			PT_int64, "market_id", PADDR(market_id), PT_DESCRIPTION, "unique identifier of market clearing",
			PT_object, "market", PADDR(market), PT_DESCRIPTION, "the specific market used by the controller",
			PT_int16, "may_run", PADDR(may_run), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "flag to determine if this controller won power for the current bid",
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(controller));
	}
}

int controller::create(){
	memset(this, 0, sizeof(controller));
	return 1;
}

int controller::init(OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	char tname[32];
	const char const *namestr = hdr->name ? hdr->name : tname;
	double high, low;

	sprintf(tname, "controller:%i", hdr->id);

	if(parent == NULL){
		gl_error("%s: controller has no parent, therefore nothing to control", namestr);
		return 0;
	}
	
	switch(type){
		case TC_DOUBLE:
			pTarget2 = gl_get_double_by_name(parent, target2);
			if(pTarget2 == NULL){
				gl_error("%s: controller unable to find secondary setpoint", namestr);
				return 0;
			}
			if(min2 * max2 > 0){
				gl_warning("%s: min2/max2 cannot be on the same side of zero", namestr);
				return 0;
			}
			base2 = *pTarget2;
			min2 = base2 + range2_low;
			max2 = base2 + range2_high;
		case TC_SINGLE:
			pTarget1 = gl_get_double_by_name(parent, target1);
			if(pTarget1 == NULL){
				gl_error("%s: controller unable to find primary setpoint", namestr);
				return 0;
			}
			if(min1 * max1 > 0){
				gl_warning("%s: min1/max1 cannot be on the same side of zero", namestr);
				return 0;
			}
			base1 = *pTarget1;
			min1 = base1 - range1_low;
			max1 = base2 + range1_high;
			break;
		default:
			gl_error("%s: controller using an unrecognized type", namestr);
			return 0;
	};

	pMonitor = gl_get_double_by_name(parent, monitor);
	if(pMonitor == NULL){
		gl_error("%s: controller unable to find monitored property", namestr);
		return 0;
	}
	
	if(market == NULL){
		gl_error("%s: controller lacks a market to participate in", namestr);
		return 0;
	}

	if(base1 < base2){
		low = base1 + range1_high;
		high = base2 + range2_low;
	} else {
		low = base2 + range2_high;
		high = base1 + range1_low;
	}
	if(low > high){
		gl_error("%s: target property bounds overlap", namestr);
		/* TROUBLESHOOT
			The bounds of the ranges described by the target property and
			the property ranges overlap, which can result in ambiguous states.
			Please review the input property values and adjust them accordingly.
		 */
		return 0;
	}

	set1 = base1;
	set2 = base2;

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