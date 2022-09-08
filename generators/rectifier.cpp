/** $Id: rectifier.cpp,v 1.0 2008/07/15 
Copyright (C) 2008 Battelle Memorial Institute
@file rectifier.cpp
@defgroup rectifier
@ingroup generators

@{
**/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "rectifier.h"

#define DEFAULT 1.0;

//CLASS *rectifier::plcass = power_electronics;
CLASS *rectifier::oclass = nullptr;
rectifier *rectifier::defaults = nullptr;

static PASSCONFIG passconfig = PC_BOTTOMUP;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
rectifier::rectifier(MODULE *module)
{
	if (oclass==nullptr)
	{
		oclass = gl_register_class(module, "rectifier",sizeof(rectifier),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class rectifier";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,

			PT_enumeration,"rectifier_type",PADDR(rectifier_type_v),
			PT_KEYWORD,"ONE_PULSE",(enumeration)ONE_PULSE,
			PT_KEYWORD,"TWO_PULSE",(enumeration)TWO_PULSE,
			PT_KEYWORD,"THREE_PULSE",(enumeration)THREE_PULSE,
			PT_KEYWORD,"SIX_PULSE",(enumeration)SIX_PULSE,
			PT_KEYWORD,"TWELVE_PULSE",(enumeration)TWELVE_PULSE,

			PT_enumeration,"generator_mode",PADDR(gen_mode_v),
			PT_KEYWORD,"UNKNOWN",(enumeration)UNKNOWN,
			PT_KEYWORD,"CONSTANT_V",(enumeration)CONSTANT_V,
			PT_KEYWORD,"CONSTANT_PQ",(enumeration)CONSTANT_PQ,
			PT_KEYWORD,"CONSTANT_PF",(enumeration)CONSTANT_PF,
			PT_KEYWORD,"SUPPLY_DRIVEN",(enumeration)SUPPLY_DRIVEN,

			PT_complex, "V_Out[V]",PADDR(V_Out),
			PT_double, "V_Rated[V]",PADDR(V_Rated),
			PT_complex, "I_Out[A]",PADDR(I_Out),
			PT_complex, "VA_Out[VA]", PADDR(VA_Out),
			PT_complex, "voltage_A[V]", PADDR(voltage_out[0]),
			PT_complex, "voltage_B[V]", PADDR(voltage_out[1]),
			PT_complex, "voltage_C[V]", PADDR(voltage_out[2]),
			PT_complex, "current_A[V]", PADDR(current_out[0]),
			PT_complex, "current_B[V]", PADDR(current_out[1]),
			PT_complex, "current_C[V]", PADDR(current_out[2]),
			PT_complex, "power_out_A[VA]", PADDR(power_out[0]),
			PT_complex, "power_out_B[VA]", PADDR(power_out[1]),
			PT_complex, "power_out_C[VA]", PADDR(power_out[2]),

			//PT_int64, "generator_mode_choice", PADDR(generator_mode_choice),

			PT_set, "phases", PADDR(phases),
			PT_KEYWORD, "A",(set)PHASE_A,
			PT_KEYWORD, "B",(set)PHASE_B,
			PT_KEYWORD, "C",(set)PHASE_C,
			PT_KEYWORD, "N",(set)PHASE_N,
			PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			defaults = this;

		memset(this,0,sizeof(rectifier));
		/* TODO: set the default values of all properties here */
	}
}

/* Object creation is called once for each object that is created by the core */
int rectifier::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int rectifier::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);

	rectifier_type_v = SIX_PULSE;

	V_Rated = 360;

	if (parent!=nullptr && gl_object_isa(parent,"inverter"))
	{
		//Map the V_In property
		pCircuit_V = new gld_property(parent,"V_In");

		//Make sure it worked
		if (!pCircuit_V->is_valid() || !pCircuit_V->is_double())
		{
			GL_THROW("rectifier:%d - %s - Unable to map parent inverter property",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to map the parent inverter property, the rectifier encoutnered an error.  Please try again.
			If the error persists, please submit your model and information via the ticketing system.
			*/
		}

		//Now get the current
		pLine_I = new gld_property(parent,"I_In");

		//Make sure it worked
		if (!pLine_I->is_valid() || !pLine_I->is_double())
		{
			GL_THROW("rectifier:%d - %s - Unable to map parent inverter property",obj->id,(obj->name ? obj->name : "Unnamed"));
			//Defined above
		}
	}
	else
	{
		GL_THROW("Rectifier:%d - %s -- Rectifiers must be parented to inverters",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		A rectifier either lacks a parent, or has a parent that is not an inverter.  Rectifiers only support inverter
		objects as parents, at this time.  Please parent it to an inverter.
		*/
	}

	/* TODO: set the context-dependent initial value of properties */
	if (gen_mode_v==UNKNOWN)
	{
		GL_THROW("Generator control mode is not specified.");
	}
	else if(gen_mode_v == CONSTANT_V)
	{
		GL_THROW("Generator mode CONSTANT_V is not implemented yet.");
	}
	else if(gen_mode_v == CONSTANT_PQ)
	{
		GL_THROW("Generator mode CONSTANT_PQ is not implemented yet.");
	}
	else if(gen_mode_v == CONSTANT_PF)
	{
		GL_THROW("Generator mode CONSTANT_PF is not implemented yet.");
	}


	//all other variables set in input file through public parameters
	switch(rectifier_type_v){
		case ONE_PULSE:
			efficiency = 0.5;
			break;
		case TWO_PULSE:
			efficiency = 0.7;
			break;
		case THREE_PULSE:
			efficiency = 0.7;
			break;
		case SIX_PULSE:
			efficiency = 0.8;
			break;
		case TWELVE_PULSE:
			efficiency = 0.9;
			break;
		default:
			efficiency = 0.8;
			break;
	}

	return 1;
}

TIMESTAMP rectifier::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	return TS_NEVER;
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP rectifier::sync(TIMESTAMP t0, TIMESTAMP t1) 
{	
	gld_wlock *test_rlock = nullptr;

	V_Out = V_Rated;

	//TODO: consider installing duty or on-ratio limits
	//AC-DC voltage magnitude ratio rule
	double VInMag = V_Out * PI / (3 * sqrt(6.0));
	voltage_out[0] = VInMag;
	voltage_out[1] = voltage_out[0];
	voltage_out[2] = voltage_out[0];


	switch(gen_mode_v){
		case SUPPLY_DRIVEN:
			{

				double S_A_In, S_B_In, S_C_In;

				//DC Voltage, controlled by parent object determines DC Voltage.
				S_A_In = voltage_out[0]*current_out[0];
				S_B_In = voltage_out[1]*current_out[1];
				S_C_In = voltage_out[2]*current_out[2];

				power_out[0] = S_A_In;
				power_out[1] = S_B_In;
				power_out[2] = S_C_In;
				VA_In = power_out[0] + power_out[1] + power_out[2];

				VA_Out = VA_In * efficiency;

				I_Out = VA_Out / V_Out;

				//Write the current
				//NOTE: This isn't checked/bounded because right now this only works with an inverter as a parent (no unparanted or other)
				pLine_I->setp<double>(I_Out,*test_rlock);

				//Write the voltage
				pCircuit_V->setp<double>(V_Out,*test_rlock);

				return TS_NEVER;
			}
			break;
		default:
			break;

			return TS_NEVER;

	}
	return TS_NEVER;
}

TIMESTAMP rectifier::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	return TS_NEVER;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_rectifier(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(rectifier::oclass);
		if (*obj!=nullptr)
		{
			rectifier *my = OBJECTDATA(*obj,rectifier);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(rectifier);
}

EXPORT int init_rectifier(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=nullptr)
			return OBJECTDATA(obj,rectifier)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(rectifier);
}

EXPORT TIMESTAMP sync_rectifier(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	rectifier *my = OBJECTDATA(obj,rectifier);
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
	}
	SYNC_CATCHALL(rectifier);
	return t2;
}
