/** $Id: freezer.cpp 4738 2014-07-03 00:55:39Z dchassin $
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

#include "house_a.h"
#include "freezer.h"

//////////////////////////////////////////////////////////////////////////
// underground_line_conductor CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* freezer::oclass = NULL;
CLASS* freezer::pclass = NULL;



freezer::freezer(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass == NULL)
	{
		oclass = gl_register_class(module,"freezer",sizeof(freezer),PC_PRETOPDOWN | PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class freezer";
		else
			oclass->trl = TRL_DEMONSTRATED;


		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double, "size[cf]", PADDR(size),
			PT_double, "rated_capacity[Btu/h]", PADDR(rated_capacity),
			PT_double,"temperature[degF]",PADDR(Tair),
			PT_double,"setpoint[degF]",PADDR(Tset),
			PT_double,"deadband[degF]",PADDR(thermostat_deadband),
			PT_timestamp,"next_time",PADDR(last_time),
			PT_double,"output",PADDR(Qr),
			PT_double,"event_temp",PADDR(Tevent),
			PT_double,"UA[Btu/degF*h]",PADDR(UA),
			PT_enumeration,"state",PADDR(motor_state),
				PT_KEYWORD,"OFF",(enumeration)S_OFF,
				PT_KEYWORD,"ON",(enumeration)S_ON,
			NULL) < 1)
			GL_THROW("unable to publish properties in %s", __FILE__);
	}
}

freezer::~freezer()
{
}

int freezer::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;

	gl_warning("explicit %s model is experimental", OBJECTHDR(this)->oclass->name);

	return res;
}

int freezer::init(OBJECT *parent)
{
	gl_warning("This device, %s, is considered very experimental and has not been validated.", get_name());

	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("freezer::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	// defaults for unset values */
	if (size==0)				size = gl_random_uniform(RNGSTATE,20,40); // cf
	if (thermostat_deadband==0) thermostat_deadband = gl_random_uniform(RNGSTATE,2,3);
	if (Tset==0)				Tset = gl_random_uniform(RNGSTATE,10,20);
	if (UA == 0)				UA = 0.3;
	if (UAr==0)					UAr = UA+size/40*gl_random_uniform(RNGSTATE,0.9,1.1);
	if (UAf==0)					UAf = gl_random_uniform(RNGSTATE,0.9,1.1);
	if (COPcoef==0)				COPcoef = gl_random_uniform(RNGSTATE,0.9,1.1);
	if (Tout==0)				Tout = 59.0;
	if (power_factor==0)		power_factor = 0.95;

	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	
	pTempProp = gl_get_property(parent, "air_temperature");
	if(pTempProp == NULL){
		GL_THROW("Parent house of freezer lacks property \'air_temperature\'");
	}

	/* derived values */
	Tair = gl_random_uniform(RNGSTATE,Tset-thermostat_deadband/2, Tset+thermostat_deadband/2);

	// size is used to couple Cw and Qrated
	//Cf = 8.43 * size/10; // BTU equivalent gallons of water for only 10% of the size of the refigerator
	Cf = size/10.0 * RHOWATER * CWATER;  // cf * lb/cf * BTU/lb/degF = BTU / degF

	rated_capacity = BTUPHPW * size*10; // BTU/h ... 10 BTU.h / cf (34W/cf, so ~700 for a full-sized freezer)

	// duty cycle estimate
	if (gl_random_bernoulli(RNGSTATE,0.04)){
		Qr = rated_capacity;
	} else {
		Qr = 0;
	}

	// initial demand
	load.total = Qr * KWPBTUPH;

	return residential_enduse::init(parent);
}

int freezer::isa(char *classname)
{
	return (strcmp(classname,"freezer")==0 || residential_enduse::isa(classname));
}

TIMESTAMP freezer::presync(TIMESTAMP t0, TIMESTAMP t1){
	OBJECT *hdr = OBJECTHDR(this);
	double *pTout = 0, t = 0.0, dt = 0.0;
	double nHours = (gl_tohours(t1)- gl_tohours(t0))/TS_SECOND;

	pTout = gl_get_double(hdr->parent, pTempProp);
	if(pTout == NULL){
		GL_THROW("Parent house of freezer lacks property \'air_temperature\' at sync time?");
	}
	Tout = *pTout;

	if(nHours > 0 && t0 > 0){ /* skip this on TS_INIT */
		const double COP = COPcoef*((-3.5/45)*(Tout-70)+4.5); /* come from ??? */

		if(t1 == next_time){
			/* lazy skip-ahead */
			load.heatgain = -((Tair - Tout) * exp(-(UAr+UAf)/Cf) + Tout - Tair) * Cf * nHours + Qr * nHours * COP;
			Tair = Tevent;
		} else {
			/* run calculations */
			const double C1 = Cf/(UAr+UAf);
			const double C2 = Tout - Qr/UAr;
			load.heatgain = -((Tair - Tout) * exp(-(UAr+UAf)/Cf) + Tout - Tair) * Cf * nHours + Qr * nHours * COP;
			Tair = (Tair-C2)*exp(-nHours/C1)+C2;
		}
		if (Tair < 0 || Tair > 32){
			gl_warning("freezer air temperature out of control");
			// was exception, now semi-valid with power outages
		}
		last_time = t1;
	}

	return TS_NEVER;
}

/* occurs between presync and sync */
/* exclusively modifies Tevent and motor_state, nothing the reflects current properties
 * should be affected by the PLC code. */
void freezer::thermostat(TIMESTAMP t0, TIMESTAMP t1){
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

TIMESTAMP freezer::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	double nHours = (gl_tohours(t1)- gl_tohours(t0))/TS_SECOND;
	double t = 0.0, dt = 0.0;

	const double COP = COPcoef*((-3.5/45)*(Tout-70)+4.5);

	// calculate power & accumulate energy
	load.energy += load.total * nHours; // moot if no dt

	// change control mode if appropriate
	if(motor_state == S_ON){
		Qr = rated_capacity;
	} else if(motor_state == S_OFF){
		Qr = 0;
	} else{
		throw "freezer motor state is ambiguous";
	}

	load.total = Qr * KWPBTUPH * COP;

	if(pCircuit->pV->Mag() < (120.0 * 0.6) ){ /* stall voltage */
		gl_verbose("freezer motor has stalled");
		motor_state = S_OFF;
		Qr = 0;
		return TS_NEVER;
	}

	// compute constants
	const double C1 = Cf/(UAr+UAf);
	const double C2 = Tout - Qr/UAr;
	
	// compute time to next internal event
	dt = t = -log((Tevent - C2)/(Tair-C2))*C1;

	if(t == 0){
		GL_THROW("freezer control logic error, dt = 0");
	} else if(t < 0){
		GL_THROW("freezer control logic error, dt < 0");
	}
	
	// if fridge is undersized or time exceeds balance of time or external event pending
	next_time = (TIMESTAMP)(t1 +  (t > 0 ? t : -t) * (3600.0/TS_SECOND) + 1);
	return next_time > TS_NEVER ? TS_NEVER : -next_time;
}

TIMESTAMP freezer::postsync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////
EXPORT int create_freezer(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(freezer::oclass);
		if (*obj!=NULL)
		{
			freezer *my = OBJECTDATA(*obj,freezer);;
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		else
			return 0;
	}
	CREATE_CATCHALL(freezer);
}

EXPORT TIMESTAMP sync_freezer(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		freezer *my = OBJECTDATA(obj,freezer);
		TIMESTAMP t1 = TS_NEVER;
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
				gl_error("freezer::sync- invalid pass configuration");
				t1 = TS_INVALID; // serious error in exec.c
		}
		return t1;
	}
	SYNC_CATCHALL(freezer);
}

EXPORT int init_freezer(OBJECT *obj)
{
	try
	{
		freezer *my = OBJECTDATA(obj,freezer);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(freezer);
}

EXPORT int isa_freezer(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,freezer)->isa(classname);
	} else {
		return 0;
	}
}

/*	determine if we're turning the motor on or off and nothing else. */
EXPORT TIMESTAMP plc_freezer(OBJECT *obj, TIMESTAMP t0)
{
	// this will be disabled if a PLC object is attached to the freezer

	freezer *my = OBJECTDATA(obj,freezer);
	my->thermostat(obj->clock, t0);

	return TS_NEVER;  
}

/**@}**/
