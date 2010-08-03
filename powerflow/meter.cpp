/** $Id: meter.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file meter.cpp
	@addtogroup powerflow_meter Meter
	@ingroup powerflow

	@note The meter object now only implements polyphase meters.  For a singlephase
	meter, see the triplex_meter object.

	Distribution meter can be either single phase or polyphase meters.
	For polyphase meters, the line voltages are nominally 277V line-to-line, and
	480V line-to-ground.  The ground is not presented explicitly (it is assumed).

	Total cumulative energy, instantantenous power and peak demand are metered.

	@{
 **/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "meter.h"
#include "timestamp.h"

// useful macros
#define TO_HOURS(t) (((double)t) / (3600 * TS_SECOND))

// meter reset function
EXPORT int64 meter_reset(OBJECT *obj)
{
	meter *pMeter = OBJECTDATA(obj,meter);
	pMeter->measured_demand = 0;
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// meter CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
// class management data
CLASS* meter::oclass = NULL;
CLASS* meter::pclass = NULL;

// the constructor registers the class and properties and sets the defaults
meter::meter(MODULE *mod) : node(mod)
{
	// first time init
	if (oclass==NULL)
	{
		// link to parent class (used by isa)
		pclass = node::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"meter",sizeof(meter),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "node",
			PT_double, "measured_real_energy[Wh]", PADDR(measured_real_energy),
			PT_double, "measured_reactive_energy[VAh]",PADDR(measured_reactive_energy),
			PT_complex, "measured_power[VA]", PADDR(measured_power),
			PT_complex, "measured_power_A[VA]", PADDR(indiv_measured_power[0]),
			PT_complex, "measured_power_B[VA]", PADDR(indiv_measured_power[1]),
			PT_complex, "measured_power_C[VA]", PADDR(indiv_measured_power[2]),
			PT_double, "measured_demand[W]", PADDR(measured_demand),
			PT_double, "measured_real_power[W]", PADDR(measured_real_power),
			PT_double, "measured_reactive_power[VAr]", PADDR(measured_reactive_power),
			
			// added to record last voltage/current
			PT_complex, "measured_voltage_A[V]", PADDR(measured_voltage[0]),
			PT_complex, "measured_voltage_B[V]", PADDR(measured_voltage[1]),
			PT_complex, "measured_voltage_C[V]", PADDR(measured_voltage[2]),
			PT_complex, "measured_voltage_AB[V]", PADDR(measured_voltageD[0]),
			PT_complex, "measured_voltage_BC[V]", PADDR(measured_voltageD[1]),
			PT_complex, "measured_voltage_CA[V]", PADDR(measured_voltageD[2]),
			PT_complex, "measured_current_A[A]", PADDR(measured_current[0]),
			PT_complex, "measured_current_B[A]", PADDR(measured_current[1]),
			PT_complex, "measured_current_C[A]", PADDR(measured_current[2]),
			PT_bool, "customer_interrupted", PADDR(meter_interrupted),
#ifdef SUPPORT_OUTAGES
			PT_int16, "sustained_count", PADDR(sustained_count),	//reliability sustained event counter
			PT_int16, "momentary_count", PADDR(momentary_count),	//reliability momentary event counter
			PT_int16, "total_count", PADDR(total_count),		//reliability total event counter
			PT_int16, "s_flag", PADDR(s_flag),
			PT_int16, "t_flag", PADDR(t_flag),
			PT_complex, "pre_load", PADDR(pre_load),
#endif


			//PT_double, "measured_reactive[kVar]", PADDR(measured_reactive), has not implemented yet
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// publish meter reset function
		if (gl_publish_function(oclass,"reset",(FUNCTIONADDR)meter_reset)==NULL)
			GL_THROW("unable to publish meter_reset function in %s",__FILE__);
		}
}

int meter::isa(char *classname)
{
	return strcmp(classname,"meter")==0 || node::isa(classname);
}

// Create a distribution meter from the defaults template, return 1 on success
int meter::create()
{
	int result = node::create();
	
#ifdef SUPPORT_OUTAGES
	sustained_count=0;	//reliability sustained event counter
	momentary_count=0;	//reliability momentary event counter
	total_count=0;		//reliability total event counter
	s_flag=0;
	t_flag=0;
	pre_load=0;
#endif

	measured_voltage[0] = measured_voltage[1] = measured_voltage[2] = complex(0,0,A);
	measured_voltageD[0] = measured_voltageD[1] = measured_voltageD[2] = complex(0,0,A);
	measured_current[0] = measured_current[1] = measured_current[2] = complex(0,0,J);
	measured_real_energy = measured_reactive_energy = 0.0;
	measured_power = complex(0,0,J);
	measured_demand = 0.0;
	measured_real_power = 0.0;
	measured_reactive_power = 0.0;

	meter_interrupted = false;	//We default to being in service

	return result;
}

// Initialize a distribution meter, return 1 on success
int meter::init(OBJECT *parent)
{
	last_t = dt = 0;
	return node::init(parent);
}
TIMESTAMP meter::presync(TIMESTAMP t0)
{
	if (solver_method == SM_NR)
		NR_mode = NR_cycle;		//COpy NR_cycle into NR_mode for generators
	else
		NR_mode = false;		//Just put as false for other methods

	return node::presync(t0);
}

TIMESTAMP meter::postsync(TIMESTAMP t0, TIMESTAMP t1)
{

	measured_voltage[0] = voltageA;
	measured_voltage[1] = voltageB;
	measured_voltage[2] = voltageC;

	measured_voltageD[0] = voltageA - voltageB;
	measured_voltageD[1] = voltageB - voltageC;
	measured_voltageD[2] = voltageC - voltageA;

	if ((solver_method == SM_NR && NR_cycle == true)||solver_method  == SM_FBS)
	{

		if (t1 > last_t)
		{
			dt = t1 - last_t;
			last_t = t1;
		}
		else
			dt = 0;
		
		measured_current[0] = current_inj[0];
		measured_current[1] = current_inj[1];
		measured_current[2] = current_inj[2];

		// compute energy use from previous cycle
		// - everything below this can moved to commit function once tape player is collecting from commit function7
		if (dt > 0 && last_t != dt)
		{	
			measured_real_energy += measured_real_power * TO_HOURS(dt);
			measured_reactive_energy += measured_reactive_power * TO_HOURS(dt);
		}

		// compute demand power
		indiv_measured_power[0] = measured_voltage[0]*(~measured_current[0]);
		indiv_measured_power[1] = measured_voltage[1]*(~measured_current[1]);
		indiv_measured_power[2] = measured_voltage[2]*(~measured_current[2]);

		measured_power = indiv_measured_power[0] + indiv_measured_power[1] + indiv_measured_power[2];

		measured_real_power = (indiv_measured_power[0]).Re()
							+ (indiv_measured_power[1]).Re()
							+ (indiv_measured_power[2]).Re();

		measured_reactive_power = (indiv_measured_power[0]).Im()
								+ (indiv_measured_power[1]).Im()
								+ (indiv_measured_power[2]).Im();

		if (measured_real_power > measured_demand) 
			measured_demand = measured_real_power;
	}

	return node::postsync(t1);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int isa_meter(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,meter)->isa(classname);
}

EXPORT int create_meter(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(meter::oclass);
		if (*obj!=NULL)
		{
			meter *my = OBJECTDATA(*obj,meter);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("create_meter: %s", msg);
	}
	return 0;
}

EXPORT int init_meter(OBJECT *obj)
{
	meter *my = OBJECTDATA(obj,meter);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s (meter:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
}

EXPORT TIMESTAMP sync_meter(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	meter *pObj = OBJECTDATA(obj,meter);
	try {
		TIMESTAMP t1;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(t0);
		case PC_BOTTOMUP:
			return pObj->sync(t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(obj->clock,t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
		throw "invalid pass request";
	} catch (const char *error) {
		gl_error("%s (meter:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return TS_INVALID; 
	} catch (...) {
		gl_error("%s (meter:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
		return TS_INVALID;
	}
}

/**@}**/
