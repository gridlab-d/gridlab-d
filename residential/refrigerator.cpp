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
refrigerator *refrigerator::defaults = NULL;



refrigerator::refrigerator(MODULE *module) 
{
	// first time init
	if (oclass == NULL)
	{
		oclass = gl_register_class(module,"refrigerator",sizeof(refrigerator),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_double, "size[cf]", PADDR(size),
			PT_double, "rated_capacity[Btu/h]", PADDR(rated_capacity),
			PT_double,"power_factor[pu]",PADDR(power_factor),
			PT_double,"temperature[degF]",PADDR(Tair),
			PT_double,"setpoint[degF]",PADDR(Tset),
			PT_double,"deadband[degF]",PADDR(thermostat_deadband),
			PT_timestamp,"next_time",PADDR(last_time),
			PT_double,"output",PADDR(Qr),
			PT_double,"event_temp",PADDR(Tevent),

			PT_enumeration,"state",PADDR(motor_state),
				PT_KEYWORD,"OFF",S_OFF,
				PT_KEYWORD,"ON",S_ON,

			PT_complex,"enduse_load[kW]",PADDR(load.total),
			PT_complex,"constant_power[kW]",PADDR(load.power),
			PT_complex,"constant_current[A]",PADDR(load.current),
			PT_complex,"constant_admittance[1/Ohm]",PADDR(load.admittance),
			PT_double,"internal_gains[kW]",PADDR(load.heatgain),
			PT_double,"energy_meter[kWh]",PADDR(load.energy),

			NULL) < 1)
			GL_THROW("unable to publish properties in %s", __FILE__);

		//setup default values
		defaults = this;
		memset(this,0,sizeof(refrigerator));
	}
}

refrigerator::~refrigerator()
{
}

int refrigerator::create() 
{
	memcpy(this, defaults, sizeof(*this));

	return 1;
}

int refrigerator::init(OBJECT *parent)
{
	// defaults for unset values */
	if (size==0)				size = gl_random_uniform(20,40); // cf
	if (thermostat_deadband==0) thermostat_deadband = gl_random_uniform(2,3);
	if (Tset==0)				Tset = gl_random_uniform(35,39);
	if (UAr==0)					UAr = 1.5+size/40*gl_random_uniform(0.9,1.1);
	if (UAf==0)					UAf = gl_random_uniform(0.9,1.1);
	if (COPcoef==0)				COPcoef = gl_random_uniform(0.9,1.1);
	if (Tout==0)				Tout = 59.0;
	if (power_factor==0)		power_factor = 0.95;

	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	if (parent==NULL || !gl_object_isa(parent,"house"))
	{
		gl_error("refrigerator must have a parent house");
		/*	TROUBLESHOOT
			The refrigerator object, being an enduse for the house model, must have a parent
			house that it is connected to.  Create a house object and set it as the parent of
			the offending refrigerator object.
		*/
		return 0;
	}

	// attach object to house panel
	house *pHouse = OBJECTDATA(parent,house);
	pVoltage = (pHouse->attach(OBJECTHDR(this),20,false))->pV;

	/* derived values */
	Tair = pHouse->Tair;

	// size is used to couple Cw and Qrated
	Cf = 8.43 * size/10; // BTU equivalent gallons of water for only 10% of the size of the refigerator
	rated_capacity = BTUPHPW * size*10; // BTU/h

	// duty cycle estimate
	if (gl_random_uniform(0,1)<0.04)
		Qr = rated_capacity;
	else
		Qr = 0;

	// initial demand
	//power_kw = rated_capacity*KWPBTUPH;  //stubbed-in default
	load.total = rated_capacity * KWPBTUPH;

	return 1;
}

TIMESTAMP refrigerator::presync(TIMESTAMP t0, TIMESTAMP t1){
	// sync to house
	Tout = pHouse->get_Tair();

	return TS_NEVER;
}

/* occurs between presync and sync */
/* exclusively modifies Tevent and motor_state, nothing the reflects current properties
 * should be affected by the PLC code. */
void refrigerator::thermostat(TIMESTAMP t0, TIMESTAMP t1){
	const double Ton = Tset+thermostat_deadband;
	const double Toff = Tset-thermostat_deadband;

	if(motor_state == S_OFF){
		// warm enough to need cooling?
		if(Tair >= Ton){
			motor_state = S_ON;
			Tevent = Toff;
		}
	} else if(motor_state == S_ON){
		// cold enough to let be?
		if(Tair <= Toff){
			motor_state = S_OFF;
			Tevent = Ton;
		}
	}
}

TIMESTAMP refrigerator::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	double nHours = (gl_tohours(t1)- gl_tohours(t0))/TS_SECOND;

//	if(t0 == t1)
//		return last_time;

	// compute control event temperatures
	const double Ton = Tset+thermostat_deadband;
	const double Toff = Tset-thermostat_deadband;

	// compute constants
	const double C1 = Cf/(UAr*UAf);
	const double COP = COPcoef*((-3.5/45)*(Tout-70)+4.5);

	// accumulate energy
	load.energy = Qr * KWPBTUPH * COP * nHours;
	load.total = load.energy/nHours;

	// process all events
	// change control mode if appropriate
	if(motor_state == S_ON){
		Qr = rated_capacity;
	} else if(motor_state == S_OFF){
		Qr = 0;
	} else{
		throw "refrigerator motor state is ambiguous";
	}

	/*
	if (ANE(Qr,0,0.1) && ALT(Tair,Toff,0.1))
		Qr = 0;
	else if (AEQ(Qr,0,0.1) && AGT(Tair,Ton,0.1))
		Qr = rated_capacity;
	*/

	// compute constants for this time increment
	const double C2 = Tout - Qr/UAf;

	// determine next internal event temperature
	/* next internal event temp is now handled by the thermostat. */
	/*
	if (AEQ(Qr,0,0.1))
		Tevent = Ton;
	else
		Tevent = Toff;
	*/

	// compute time to next internal event
	double t = -log((Tevent - C2)/(Tair-C2))*C1;
	double dt = t;
	if (t==0)
		throw "refrigerator control logic error";

	// if fridge is undersized or time exceeds balance of time or external event pending
	if (t<0 || t>=nHours)
	{
		dt = ((t<0||t>=nHours)?nHours:t);

		// update temperature of air
		Tair = (Tair-C2)*exp(-dt/C1)+C2;
		if (Tair < 32 || Tair > 55)
			throw "refrigerator air temperature out of control";

		last_time = (TIMESTAMP)(t1+dt*3600.0/TS_SECOND)+1;
		/* one second for over-cool fudge factor & avoiding numerical instability/Zeno's dichotomy paradox problems */

		return last_time;
	}
	// internal event
	else
	{
		Tair = Tevent;
		last_time = TS_NEVER;
		return TS_NEVER; 
	}
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

/*	determine if we're turning the motor on or off and nothing else. */
EXPORT TIMESTAMP plc_refrigerator(OBJECT *obj, TIMESTAMP t0)
{
	// this will be disabled if a PLC object is attached to the refrigerator

	refrigerator *my = OBJECTDATA(obj,refrigerator);
	my->thermostat(obj->clock, t0);

	return TS_NEVER;  
}

/**@}**/
