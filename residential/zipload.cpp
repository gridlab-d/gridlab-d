/** $Id: ZIPloadload.cpp $
	Copyright (C) 2009 Battelle Memorial Institute
	@file ZIPloadload.cpp
	@addtogroup ZIPload
	@ingroup residential
	
	The ZIPload simulation is based on demand profile of the connected ZIPload loads.
	Individual power fractions and power factors are used.
	Heat fraction ratio is used to calculate the internal gain from ZIPload loads.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "ZIPload.h"

//////////////////////////////////////////////////////////////////////////
// ZIPload CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* ZIPload::oclass = NULL;
CLASS* ZIPload::pclass = NULL;

ZIPload::ZIPload(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"ZIPload",sizeof(ZIPload),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double,"heat_fraction",PADDR(load.heatgain_fraction), PT_DESCRIPTION, "fraction of ZIPload that is transferred as heat",
			PT_double,"base_power[kW]",PADDR(base_power), PT_DESCRIPTION, "base real power of the overall load",
			PT_double,"power_pf",PADDR(power_pf), PT_DESCRIPTION, "power factor for constant power portion",
			PT_double,"current_pf",PADDR(current_pf), PT_DESCRIPTION, "power factor for constant current portion",
			PT_double,"impedance_pf",PADDR(impedance_pf), PT_DESCRIPTION, "power factor for constant impedance portion",
			PT_bool,"is_240",PADDR(is_240), PT_DESCRIPTION, "load is 220/240 V (across both phases)",
			PT_double,"breaker_val",PADDR(breaker_val), PT_DESCRIPTION, "Amperage of connected breaker",
			NULL)<1) 

			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

ZIPload::~ZIPload()
{
}

int ZIPload::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;
	load.power = load.admittance = load.current = load.total = 0.0;
	base_power = 0.0;
	load.heatgain_fraction = 0.90;
	load.power_factor = 1.00;
	load.power_fraction = 1.0;
	load.current_fraction = load.impedance_fraction = 0.0;
	load.config = 0x0;	//110 by default
	breaker_val = 80.0;	//Obscene value so it never trips

	//Default values of other properties
	power_pf = current_pf = impedance_pf = 1.0;

	load.voltage_factor = 1.0; // assume 'even' voltage, initially
	return res;
}

int ZIPload::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	if (is_240)	//See if the 220/240 flag needs to be set
	{
		load.config |= EUC_IS220;
	}

	load.breaker_amps = breaker_val;

	return residential_enduse::init(parent);
}

TIMESTAMP ZIPload::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	TIMESTAMP t2 = TS_NEVER;
	double real_power = 0.0;
	double imag_power = 0.0;

	if (pCircuit!=NULL)
		load.voltage_factor = pCircuit->pV->Mag() / 120; // update voltage factor - not really used for anything

	t2 = residential_enduse::sync(t0,t1);

	if (pCircuit->status==BRK_CLOSED) 
	{
		//All values placed as kW/kVAr values - to be consistent with other loads

		//Calculate power portion
		real_power = base_power * load.power_fraction;

		imag_power = (power_pf == 0.0) ? 0.0 : real_power * sqrt(1.0/(power_pf * power_pf) - 1.0);

		if (power_pf < 0)
		{
			imag_power *= -1.0;	//Adjust imaginary portion for negative PF
		}

		load.power.SetRect(real_power,imag_power);

		//Calculate current portion
		real_power = base_power * load.current_fraction;

		imag_power = (current_pf == 0.0) ? 0.0 : real_power * sqrt(1.0/(current_pf * current_pf) - 1.0);

		if (current_pf < 0)
		{
			imag_power *= -1.0;	//Adjust imaginary portion for negative PF
		}

		load.current.SetRect(real_power,imag_power);

		//Calculate impedance portion
		real_power = base_power * load.impedance_fraction;

		imag_power = (impedance_pf == 0.0) ? 0.0 : real_power * sqrt(1.0/(impedance_pf * impedance_pf) - 1.0);

		if (impedance_pf < 0)
		{
			imag_power *= -1.0;	//Adjust imaginary portion for negative PF
		}

		load.admittance.SetRect(real_power,imag_power);	//Put impedance in admittance.  From a power point of view, they are the same

		//Compute total power - not sure if needed, but will use below
		load.total = load.power + load.current + load.admittance;

		//Determine the heat contributions - percentage of real power
		load.heatgain = load.total.Re() * load.heatgain_fraction;
	}
	else	//Breaker's open - nothing happens
	{
		load.total = 0.0;
		load.power = 0.0;
		load.current = 0.0;
		load.admittance = 0.0;
		load.heatgain = 0.0;
	}

	return t2;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_ZIPload(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(ZIPload::oclass);
	if (*obj!=NULL)
	{
		ZIPload *my = OBJECTDATA(*obj,ZIPload);;
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_ZIPload(OBJECT *obj)
{
	ZIPload *my = OBJECTDATA(obj,ZIPload);
	return my->init(obj->parent);
}

EXPORT TIMESTAMP sync_ZIPload(OBJECT *obj, TIMESTAMP t0)
{
	ZIPload *my = OBJECTDATA(obj, ZIPload);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/
