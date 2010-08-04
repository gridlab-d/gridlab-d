/** $Id: refrigerator.cpp,v 1.13 2008/02/13 02:47:53 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file refrigerator.cpp
	@addtogroup refrigerator
	@ingroup residential

	The refrigerator model is based on performance profile that can specified by a tape and using 
	a method developed by Ross T. Guttromson
	
	Original ODE:
	
	\f$ \frac{C_f}{UA_r + UA_f} \frac{dT_{air}}{dt} + T_{air} = T_{out} + \frac{Q_r}{UA_r} \f$ 
	
	 where

		 \f$ T_{air} \f$ is the temperature of the water

		 \f$ T_{out} \f$ is the ambient temperature around the refrigerator

		 \f$ UA_r \f$ is the UA of the refrigerator itself

		 \f$ UA_f \f$ is the UA of the food-air

		 \f$ C_f \f$ is the heat capacity of the food

		\f$ Q_r \f$ is the heat rate from the cooling system


	 General form:

	 \f$ T_t = (T_o - C_2)e^{\frac{-t}{C_1}} + C_2 \f$ 
	
	where

		t is the elapsed time

		\f$ T_o \f$ is the initial temperature

		\f$ T_t\f$  is the temperature at time t

		\f$ C_1 = \frac{C_f}{UA_r + UA_f} \f$ 

		\f$ C_2 = T_out + \frac{Q_r}{UA_f} \f$ 
	
	Time solution
	
		\f$ t = -ln\frac{T_t - C_2}{T_o - C_2}*C_1 \f$ 
	
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "refrigerator.h"

//////////////////////////////////////////////////////////////////////////
// underground_line_conductor CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* refrigerator::oclass = NULL;
CLASS *refrigerator::pclass = NULL;

refrigerator::refrigerator(MODULE *module) 
: residential_enduse(module)
{
	// first time init
	if (oclass == NULL)
	{
		pclass = residential_enduse::oclass;

		// register the class definition
		oclass = gl_register_class(module,"refrigerator",sizeof(refrigerator),PC_PRETOPDOWN | PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The file that implements the lights in the residential module cannot register the class.
				This is an internal error.  Contact support for assistance.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double, "size[cf]", PADDR(size),PT_DESCRIPTION,"volume of the refrigerator",
			PT_double, "rated_capacity[Btu/h]", PADDR(rated_capacity),
			PT_double,"temperature[degF]",PADDR(Tair),
			PT_double,"setpoint[degF]",PADDR(Tset),
			PT_double,"deadband[degF]",PADDR(thermostat_deadband),
			PT_timestamp,"next_time",PADDR(last_time),
			PT_double,"output",PADDR(Qr),
			PT_double,"event_temp",PADDR(Tevent),
			PT_double,"UA[Btu.h/degF]",PADDR(UA),

			PT_enumeration,"state",PADDR(motor_state),
				PT_KEYWORD,"OFF",S_OFF,
				PT_KEYWORD,"ON",S_ON,

			NULL) < 1)
			GL_THROW("unable to publish properties in %s", __FILE__);
	}
}

int refrigerator::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;

	gl_warning("explicit %s model is experimental", OBJECTHDR(this)->oclass->name);

	return res;
}

int refrigerator::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	// defaults for unset values */
	if (size==0)				size = gl_random_uniform(20,40); // cf
	if (thermostat_deadband==0) thermostat_deadband = gl_random_uniform(2,3);
	if (Tset==0)				Tset = gl_random_uniform(35,39);
	if (UA == 0)				UA = 0.6;
	if (UAr==0)					UAr = UA+size/40*gl_random_uniform(0.9,1.1);
	if (UAf==0)					UAf = gl_random_uniform(0.9,1.1);
	if (COPcoef==0)				COPcoef = gl_random_uniform(0.9,1.1);
	if (Tout==0)				Tout = 59.0;
	if (load.power_factor==0)		load.power_factor = 0.95;

	pTout = (double*)gl_get_addr(parent, "air_temperature");
	if (pTout==NULL)
	{
		static double default_air_temperature = 72;
		gl_warning("%s (%s:%d) parent object lacks air temperature, using %0f degF instead", hdr->name, hdr->oclass->name, hdr->id, default_air_temperature);
		pTout = &default_air_temperature;
	}

	/* derived values */
	Tair = gl_random_uniform(Tset-thermostat_deadband/2, Tset+thermostat_deadband/2);

	// size is used to couple Cw and Qrated
	Cf = size/10.0 * RHOWATER * CWATER;  // cf * lb/cf * BTU/lb/degF = BTU / degF

	rated_capacity = BTUPHPW * size*10; // BTU/h ... 10 BTU.h / cf (34W/cf, so ~700 for a full-sized refrigerator)

	// duty cycle estimate for initial condition
	if (gl_random_bernoulli(0.1)){
		Qr = rated_capacity;
	} else {
		Qr = 0;
	}

	// initial demand
	load.total = Qr * KWPBTUPH;

	return residential_enduse::init(parent);
}

int refrigerator::isa(char *classname)
{
	return (strcmp(classname,"refrigerator")==0 || residential_enduse::isa(classname));
}

TIMESTAMP refrigerator::presync(TIMESTAMP t0, TIMESTAMP t1){
	OBJECT *hdr = OBJECTHDR(this);
	double t = 0.0, dt = 0.0;
	double nHours = (gl_tohours(t1)- gl_tohours(t0))/TS_SECOND;

	Tout = *pTout;

	if(nHours > 0 && t0 > 0){ /* skip this on TS_INIT */
		const double COP = COPcoef*((-3.5/45)*(Tout-70)+4.5); /* come from ??? */

		if(t1 == next_time){
			/* lazy skip-ahead */
			load.heatgain = (-((Tair - Tout) * exp(-(UAr+UAf)/Cf) + Tout - Tair) * Cf + Qr * COP) * KWPBTUPH;
			Tair = Tevent;
		} else {
			/* run calculations */
			const double C1 = Cf/(UAr+UAf);
			const double C2 = Tout - Qr/UAr;
			load.heatgain = (-((Tair - Tout) * exp(-(UAr+UAf)/Cf) + Tout - Tair) * Cf  + Qr * COP) * KWPBTUPH;;
			Tair = (Tair-C2)*exp(-nHours/C1)+C2;
		}
		if (Tair < 32 || Tair > 55)
			throw "refrigerator air temperature out of control";
		last_time = t1;
	}

	return TS_NEVER;
}

/* occurs between presync and sync */
/* exclusively modifies Tevent and motor_state, nothing the reflects current properties
 * should be affected by the PLC code. */
void refrigerator::thermostat(TIMESTAMP t0, TIMESTAMP t1){
	const double Ton = Tset+thermostat_deadband / 2;
	const double Toff = Tset-thermostat_deadband / 2;

	// determine motor state & next internal event temperature
	if(motor_state == S_OFF){
		// warm enough to need cooling?
		if(Tair >= Ton){
			motor_state = S_ON;
			Tevent = Toff;
		} else {
			Tevent = Ton;
		}
	} else if(motor_state == S_ON){
		// cold enough to let be?
		if(Tair <= Toff){
			motor_state = S_OFF;
			Tevent = Ton;
		} else {
			Tevent = Toff;
		}
	}
}

TIMESTAMP refrigerator::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	double nHours = (gl_tohours(t1)- gl_tohours(t0))/TS_SECOND;
	double t = 0.0, dt = 0.0;

	const double COP = COPcoef*((-3.5/45)*(Tout-70)+4.5);

	// change control mode if appropriate
	if(motor_state == S_ON){
		Qr = rated_capacity;
	} else if(motor_state == S_OFF){
		Qr = 0;
	} else{
		throw "refrigerator motor state is ambiguous";
	}

	// calculate power from motor state
	load.power = Qr * KWPBTUPH * COP;

	// compute constants
	const double C1 = Cf/(UAr+UAf);
	const double C2 = Tout - Qr/UAr;
	
	// compute time to next internal event
	dt = t = -log((Tevent - C2)/(Tair-C2))*C1;

	if(t == 0){
		GL_THROW("refrigerator control logic error, dt = 0");
	} else if(t < 0){
		GL_THROW("refrigerator control logic error, dt < 0");
	}

	TIMESTAMP t2 = gl_enduse_sync(&(residential_enduse::load),t1);

	// if fridge is undersized or time exceeds balance of time or external event pending
	next_time = (TIMESTAMP)(t1 +  (t > 0 ? t : -t) * (3600.0/TS_SECOND) + 1);
	return next_time > TS_NEVER ? TS_NEVER : -next_time;
}

TIMESTAMP refrigerator::postsync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////
EXPORT int create_refrigerator(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(refrigerator::oclass);
	if (*obj!=NULL)
	{
		refrigerator *my = OBJECTDATA(*obj,refrigerator);;
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT TIMESTAMP sync_refrigerator(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	refrigerator *my = OBJECTDATA(obj,refrigerator);
	TIMESTAMP t1 = TS_NEVER;

	// obj->clock = 0 is legit

	try {
		switch (pass) 
		{
			case PC_PRETOPDOWN:
				t1 = my->presync(obj->clock, t0);
				break;

			case PC_BOTTOMUP:
				t1 = my->sync(obj->clock, t0);
				obj->clock = t0;
				break;

			case PC_POSTTOPDOWN:
				t1 = my->postsync(obj->clock, t0);
				break;

			default:
				gl_error("refrigerator::sync- invalid pass configuration");
				t1 = TS_INVALID; // serious error in exec.c
		}
	} 
	catch (char *msg)
	{
		gl_error("refrigerator::sync exception caught: %s", msg);
		t1 = TS_INVALID;
	}
	catch (...)
	{
		gl_error("refrigerator::sync exception caught: no info");
		t1 = TS_INVALID;
	}
	return t1;
}

EXPORT int init_refrigerator(OBJECT *obj)
{
	refrigerator *my = OBJECTDATA(obj,refrigerator);
	return my->init(obj->parent);
}

EXPORT int isa_refrigerator(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,refrigerator)->isa(classname);
	} else {
		return 0;
	}
}

/*	determine if we're turning the motor on or off and nothing else. */
EXPORT TIMESTAMP plc_refrigerator(OBJECT *obj, TIMESTAMP t0)
{
	// this will be disabled if a PLC object is attached to the refrigerator

	refrigerator *my = OBJECTDATA(obj,refrigerator);
	my->thermostat(obj->clock, t0);

	return TS_NEVER;  
}

/**@}**/
