/** $Id: freezer.cpp,v 1.13 2008/02/13 02:47:53 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file freezer.cpp
	@addtogroup freezer
	@ingroup residential

	The freezer model is based on performance profile that can specified by a tape and using 
	a method developed by Ross T. Guttromson
	
	Original ODE:
	
	\f$ \frac{C_f}{UA_r + UA_f} \frac{dT_{air}}{dt} + T_{air} = T_{out} + \frac{Q_r}{UA_r} \f$ 
	
	 where

		 \f$ T_{air} \f$ is the temperature of the water

		 \f$ T_{out} \f$ is the ambient temperature around the freezer

		 \f$ UA_r \f$ is the UA of the freezer itself

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

#include "house.h"
#include "freezer.h"

//////////////////////////////////////////////////////////////////////////
// underground_line_conductor CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* freezer::oclass = NULL;
freezer *freezer::defaults = NULL;



freezer::freezer(MODULE *module) 
{
	// first time init
	if (oclass == NULL)
	{
		oclass = gl_register_class(module,"freezer",sizeof(freezer),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_double, "size [cf]", PADDR(size),
			PT_double, "rated_capacity [Btu/h]", PADDR(rated_capacity),
			PT_complex,"power[kW]",PADDR(power_kw),
			PT_double,"power_factor[pu]",PADDR(power_factor),
			PT_double,"meter[kWh]",PADDR(kwh_meter),

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
		memset(this,0,sizeof(freezer));
	}
}

freezer::~freezer()
{
}

int freezer::create() 
{
	memcpy(this, defaults, sizeof(*this));

	return 1;
}

int freezer::init(OBJECT *parent)
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
		gl_error("freezer must have a parent house");
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
	load.total = rated_capacity*KWPBTUPH;  //stubbed-in default

	return 1;
}

TIMESTAMP freezer::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	double nHours = (gl_tohours(t1)- gl_tohours(t0))/TS_SECOND;

	if(t0 == t1){
		return last_time; /* avoid recalc'ing, nothing will have changed. */
	}
	// sync to house
	if(pHouse != NULL){
		Tout = pHouse->get_Tair();
	} else {
		Tout = 70.0;
	}

	// compute control event temperatures
	const double Ton = Tset+thermostat_deadband;
	const double Toff = Tset-thermostat_deadband;

	// compute constants
	const double C1 = Cf/(UAr*UAf);
	const double COP = COPcoef*((-3.5/45)*(Tout-70)+4.5);

	// accumulate energy
	load.energy = Qr*KWPBTUPH*COP*nHours;
	load.total = load.energy/nHours;

	// process all events
	// change control mode if appropriate
	if (ANE(Qr,0,0.1) && ALT(Tair,Toff,0.1))
		Qr = 0;
	else if (AEQ(Qr,0,0.1) && AGT(Tair,Ton,0.1))
		Qr = rated_capacity;

	// compute constants for this time increment
	const double C2 = Tout - Qr/UAf;

	// determine next internal event temperature
	double Tevent;
	if (AEQ(Qr,0,0.1))
		Tevent = Ton;
	else
		Tevent = Toff;

	// compute time to next internal event
	double t = -log((Tevent - C2)/(Tair-C2))*C1;
	double dt = t;
	if (t==0)
		throw "freezer control logic error";

	// if fridge is undersized or time exceeds balance of time or external event pending
	if (t<0 || t>=nHours)
	{
		dt = ((t<0||t>=nHours)?nHours:t);

		// update temperature of air
		Tair = (Tair-C2)*exp(-dt/C1)+C2;
		if (Tair < 32 || Tair > 55)
			throw "freezer air temperature out of control";
		last_time = (TIMESTAMP)(t1+dt*3600.0/TS_SECOND);
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


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////
EXPORT int create_freezer(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(freezer::oclass);
	if (*obj!=NULL)
	{
		freezer *my = OBJECTDATA(*obj,freezer);;
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_freezer(OBJECT *obj)
{
	freezer *my = OBJECTDATA(obj,freezer);
	return my->init(obj->parent);
}

EXPORT TIMESTAMP sync_freezer(OBJECT *obj, TIMESTAMP t0)
{
	TIMESTAMP t1 = ((freezer*)(obj+1))->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/
