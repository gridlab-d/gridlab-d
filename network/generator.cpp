/** $Id: generator.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file generator.cpp
	@addtogroup generator Generating unit
	@ingroup network
	
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"

int generator_state_from_string(void *addr, char *value)
{
	if (strcmp(value,"STOPPED")==0)
		*(enumeration*)addr = generator::STOPPED;
	else if (strcmp(value,"STANDBY")==0)
		*(enumeration*)addr = generator::STANDBY;
	else if (strcmp(value,"ONLINE")==0)
		*(enumeration*)addr = generator::ONLINE;
	else if (strcmp(value,"TRIPPED")==0)
		*(enumeration*)addr = generator::TRIPPED;
	else
		return 0;
	return 1;
}

int generator_state_to_string(void *addr, char *value, int size)
{
	enumeration state = *(enumeration*)addr;
	switch (state) {
	case generator::STOPPED: if (size>7) strcpy(value,"STOPPED"); else return 0;
		return 1;
	case generator::STANDBY: if (size>7) strcpy(value,"STANDBY"); else return 0;
		return 1;
	case generator::ONLINE: if (size>6) strcpy(value,"ONLINE"); else return 0;
		return 1;
	case generator::TRIPPED: if (size>7) strcpy(value,"TRIPPED"); else return 0;
		return 1;
	default:
		return 0;
	}
}

//////////////////////////////////////////////////////////////////////////
// generator CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

CLASS* generator::oclass = NULL;
CLASS* generator::pclass = NULL;
generator *generator::defaults = NULL;

CLASS *generator_class = (NULL);
OBJECT *last_generator = (NULL);

generator::generator(MODULE *mod)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		generator_class = oclass = gl_register_class(mod,"generator",sizeof(generator),PC_BOTTOMUP);
		if (oclass==NULL)
			throw "unable to register class generator";
		else
			oclass->trl = TRL_STANDALONE;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_double, "Pdesired_MW", PADDR(Pdesired_MW),
			PT_double, "Qdesired_MVAR", PADDR(Qdesired_MVAR),
			PT_int32, "Qcontrolled", PADDR(Qcontrolled),
			PT_double, "Pmax_MW", PADDR(Pmax_MW),
			PT_double, "Qmin_MVAR", PADDR(Qmin_MVAR),
			PT_double, "Qmax_MVAR", PADDR(Qmax_MVAR),
			PT_double, "QVa", PADDR(QVa),
			PT_double, "QVb", PADDR(QVb),
			PT_double, "QVc", PADDR(QVc),
			//TODO: Resolve what is correct for this macro
			//Old: PUBLISH_DELEGATED(generator,generator_state,state);
			//PT_delegated, "state", generator_state, 
			PT_enumeration,"state",PADDR(state),
				PT_KEYWORD,"STOPPED",STOPPED,
				PT_KEYWORD,"STANDBY",STANDBY,
				PT_KEYWORD,"ONLINE",ONLINE,
				PT_KEYWORD,"TRIPPED",TRIPPED,
			//PT_delegated, "state", state,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
		Pdesired_MW = 0;		// desired real power output
		Qdesired_MVAR = 0;		// desired reactive power output (if controlled)
		Qcontrolled = 0;		// flag true if reactive power is controlled
		Pmax_MW = 0;			// maximum real power
		Qmin_MVAR = 0;			// maximum reactive power possible
		Qmax_MVAR = 0;			// minimum reactive power possible
		QVa = QVb = QVc = 0;	// voltage response parameters (Q = a*V^2 + b*V + c)
		state = STOPPED;	
	}
}

int generator::create() 
{
	memcpy(this,defaults,sizeof(*this));
	return 1;
}

int generator::init(node *parent)
{
	// check sanity of initial state
	if (parent->type==PQ && !Qcontrolled)
	{
		OBJECT *obj = OBJECTHDR(this);
		gl_error("generator:%d is Qcontrolled but is not connected to a PQ bus", obj->id);
		return 0;
	}
	else if (parent->type!=PQ && Qcontrolled)
	{
		OBJECT *obj = OBJECTHDR(this);
		gl_error("generator:%d is not Qcontrolled but is connected to a PQ bus", obj->id);
		return 0;
	}
	else if (Qcontrolled && Qdesired_MVAR<Qmin_MVAR && Qdesired_MVAR>Qmax_MVAR)
	{
		OBJECT *obj = OBJECTHDR(this);
		gl_error("generator:%d Qdesired is out of Qmin/Qmax limits", obj->id);
		return 0;
	}
	else if (parent->type!=SWING && Pdesired_MW>Pmax_MW)
	{
		OBJECT *obj = OBJECTHDR(this);
		gl_error("generator:%d Pdesired exceeds Pmax", obj->id);
		return 0;
	}
	return 1;
}

TIMESTAMP generator::sync(TIMESTAMP t0) 
{
	TIMESTAMP t1 = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);
	node *pBus = OBJECTDATA(obj->parent,node);
	switch (pBus->type) {
		case SWING:
			// check for overlimit
			if (pBus->S.Re()>Pmax_MW || pBus->S.Im()<Qmin_MVAR || pBus->S.Im()>Qmax_MVAR)
			{
				// this happens because we don't support redispatching yet (after redispatch we could do tripping).
				gl_error("generator:%d exceeded limits when connected to swing bus; clock stopped", obj->id);
				return TS_ZERO;
			}
			break;
		case PV:
			// post real power and reactive limits
			pBus->S.Re() = Pdesired_MW;
			pBus->Qmax_MVAR = Qmax_MVAR;
			pBus->Qmin_MVAR = Qmin_MVAR;
			break;
		case PQ:
			// post both real and reactive power
			pBus->S = complex(Pdesired_MW,Qdesired_MVAR);
			break;
		default:
			break;
	}
	return t1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: generator
//////////////////////////////////////////////////////////////////////////

EXPORT int create_generator(OBJECT **obj)
{
	*obj = gl_create_object(generator_class);
	if (*obj!=NULL)
	{
		last_generator = *obj;
		generator *my = OBJECTDATA(*obj,generator);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_generator(OBJECT *obj)
{
	if (obj->parent && gl_object_isa(obj->parent,"node"))
		return OBJECTDATA(obj,generator)->init(OBJECTDATA(obj->parent,node));
	else
	{
		gl_error("generator:%d is not connected to a network node", obj->id);
		return 0;
	}
}

EXPORT TIMESTAMP sync_generator(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	TIMESTAMP t1 = TS_NEVER;
	if (pass==PC_BOTTOMUP)
		t1 = OBJECTDATA(obj,generator)->sync(t0);
	obj->clock = t0;
	return t1;
}

/**@}*/
