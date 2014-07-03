/** $Id: multizone.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file multizone.cpp
	@defgroup multizone Template for a new object class
	@ingroup MODULENAME

	You can add an object class to a module using the \e add_class
	command:
	\verbatim
	linux% add_class CLASS
	\endverbatim

	You must be in the module directory to do this.

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "office.h"
#include "multizone.h"

CLASS *multizone::oclass = NULL;
multizone *multizone::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
multizone::multizone(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"multizone",sizeof(multizone),passconfig);
		if (oclass==NULL)
			throw "unable to register class multizone";
		else
			oclass->trl = TRL_INTEGRATED;

		if (gl_publish_variable(oclass,
			/* TODO: add your published properties here */
			PT_object, "from", PADDR(from),
			PT_object, "to", PADDR(to),
			PT_double, "ua", PADDR(ua),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		/* TODO: set the default values of all properties here */
		memset(this,0,sizeof(multizone));
	}
}

/* Object creation is called once for each object that is created by the core */
int multizone::create(void)
{
	memcpy(this,defaults,sizeof(multizone));
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int multizone::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	if (from==NULL)
		gl_error("%s (multizone:%d): from zone is not specified", obj->name?obj->name:"unnamed",obj->id);
	else if (!gl_object_isa(from,"office"))
		gl_error("%s (multizone:%d): from object is not an commercial office space", obj->name?obj->name:"unnamed",obj->id);
	if (to==NULL)
		gl_error("%s (multizone:%d): to zone is not specified", obj->name?obj->name:"unnamed",obj->id);
	else if (!gl_object_isa(to,"office"))
		gl_error("%s (multizone:%d): to object is not an commercial office space", obj->name?obj->name:"unnamed",obj->id);
	if (ua<=0)
		gl_error("%s (multizone:%d): ua must be positive (value is %.2f)", obj->name?obj->name:"unnamed",obj->id,ua);
	gl_set_rank(from,obj->rank+1);
	gl_set_rank(to,obj->rank+1);
	return 1; /* return 1 on success, 0 on failure */
}

/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP multizone::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP multizone::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	if (t1>t0 && t0>0)
	{
		office *pFrom = OBJECTDATA(from,office);
		office *pTo = OBJECTDATA(to,office);
	
		/* initial delta T */
		double dT = pFrom->zone.current.air_temperature - pTo->zone.current.air_temperature;

		/* rate of change of delta T */
		double ddT = pFrom->zone.current.temperature_change - pTo->zone.current.temperature_change;

		/* mean delta T */
		double DT = dT + ddT/2;

		/* mean heat transfer */
		double dQ = ua * DT * (t1-t0)/TS_SECOND/3600;
	
		double x;
		x = dQ*(t1-t0);	LOCKED(from, pFrom->Qz -= x);
		LOCKED(to, pTo->Qz += dQ);

		if (ddT!=0)

			/* time for 1 deg temperature change */
			return (TIMESTAMP)(t1+fabs(1/ddT)*3600*TS_SECOND); 
		else
			return TS_NEVER;
	}

	return TS_NEVER; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP multizone::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_multizone(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(multizone::oclass);
		if (*obj!=NULL)
		{
			multizone *my = OBJECTDATA(*obj,multizone);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(multizone);
}

EXPORT int init_multizone(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,multizone)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(multizone);
}

EXPORT TIMESTAMP sync_multizone(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	multizone *my = OBJECTDATA(obj,multizone);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;
		return t2;
	}
	SYNC_CATCHALL(multizone);
}
