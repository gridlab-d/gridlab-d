/** $Id: lights.cpp,v 1.38 2008/02/13 02:22:35 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file lights.cpp
	@addtogroup residential_lights Lights
	@ingroup residential

	The residential lighting object supports the major types of residential
	lights (see lights::type).  The lighting type effects the power factor
	used (see lights::power_factor).

		\f$ S = Polar\left(S_{rated}*demand,cos^{-1}PF_{type}\right) \f$

	For incandescent lights, the power drops proportionally with voltage:

		\f$ S = S_{rated}\frac{\left|V\right|}{120} \f$

	and cuts out below 30 V.

	For electronic ballast lights, the power drops as voltage drops 

		\f$ \Re\left(S\right) = S_{rated} \frac{\left|V\right|}{120} \f$ 

	but reactive power is to the square root of voltage

		\f$ \Im\left(S\right) = \frac{S_{rated}}{PF_{type}} \sqrt{\frac{\left|V\right|}{120}} \f$ 

	and cuts out below 0.1 puV.  The lighs cut back in between 0.2 and 0.4 puV. 
	[Ref: Steve Yang's test results for WECC LMTF done at BPA in 2007]

	For high-intensity discharge lights the power increases to the square of the voltage drop

		\f$ S = S_{rated}\left(\frac{120}{\left|V\right|}\right)^2 \f$

	and cuts out below 90 V.

	@todo Research the voltage response for HID lights; these are just educated guesses. (residential, medium priority) (ticket #141)
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "lock.h"
#include "house_a.h"
#include "lights.h"

//////////////////////////////////////////////////////////////////////////
// lights CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* lights::oclass = NULL;
lights *lights::defaults = NULL;

double lights::power_factor[_MAXTYPES];

// the constructor registers the class and properties and sets the defaults
lights::lights(MODULE *mod) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"lights",sizeof(lights),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_enumeration,"type",PADDR(type),
				PT_KEYWORD,"INCANDESCENT",INCANDESCENT,
				PT_KEYWORD,"FLUORESCENT",FLUORESCENT,
				PT_KEYWORD,"CFL",CFL,
				PT_KEYWORD,"SSL",SSL,
				PT_KEYWORD,"HID",HID,
			PT_enumeration,"placement",PADDR(placement),
				PT_KEYWORD,"INDOOR",INDOOR,
				PT_KEYWORD,"OUTDOOR",OUTDOOR,
			PT_double,"installed_power[W]",PADDR(installed_power),
			PT_double,"circuit_split",PADDR(circuit_split),
			PT_double,"demand[unit]",PADDR(demand),
			PT_complex,"enduse_load[kW]",PADDR(load.total),
			PT_complex,"constant_power[kW]",PADDR(load.power),
			PT_complex,"constant_current[A]",PADDR(load.current),
			PT_complex,"constant_admittance[1/Ohm]",PADDR(load.admittance),
			PT_double,"internal_gains[kW]",PADDR(load.heatgain),
			PT_double,"energy_meter[kWh]",PADDR(load.energy),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// default global values
		power_factor[INCANDESCENT]	= 1.00;
		power_factor[FLUORESCENT]	= 0.93;
		power_factor[CFL]			= 0.86;
		power_factor[SSL]			= 0.75;
		power_factor[HID]			= 0.97;

		// setup the default values
		defaults = this;
		memset(this,0,sizeof(lights));
		type = INCANDESCENT;
		placement = INDOOR;
		circuit_split = 0;
		power_density = 0;
		installed_power = 0;
		load.power = load.admittance = load.current = load.total = complex(0,0,J);
		load.energy = load.heatgain = 0.0;
		pVoltage = NULL;
	}
}

// create is called every time a new object is loaded
int lights::create(void) 
{
	// copy the defaults
	memcpy(this,defaults,sizeof(lights));

	// set basic properties
	circuit_split = gl_random_uniform(-1,1); // @todo circuit_split is ignored and should be delete; the split is done automatically by panel.attach()
	power_density = gl_random_normal(1.0, 0.075);  // W/sf
	
	// other initial conditions
	demand = 0.2;
	return 1;
}

int lights::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	// lights must have a parent house
	if (parent==NULL || !gl_object_isa(parent,"house"))
	{
		gl_error("lights must have a parent house");
		return 0;
	}

	// attach object to house panel
	house *pHouse = OBJECTDATA(parent,house);
	pVoltage = (pHouse->attach(OBJECTHDR(this),20,false))->pV;

	// installed power intially overrides use of power density
	if (installed_power==0) 
		installed_power = power_density * pHouse->floor_area;

	// initial demand (assume power factor is 1.0)
	load.power = installed_power * demand / 1000;

	return 1;
}

TIMESTAMP lights::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	// reset load
	load.admittance = load.current = load.power = complex(0,0,J);
	
	// compute nominal power consumption (adjust with power factor)
	load.power.SetPowerFactor(installed_power/power_factor[type] * demand / 1000.0, power_factor[type],J);
	double VM2 = ((*pVoltage) * ~(*pVoltage)).Re();

	// adjust power based on lamp type's response to voltage
	double puV = pVoltage->Mag()/120.0;
	switch (type) {
	case INCANDESCENT:
		if (puV>0.25 && puV<1.5) // lights can burn out--normally this should take a while to fix, but let's not worry about this now
			load.admittance = installed_power/VM2;
		break;
	case FLUORESCENT:
	case CFL:
	case SSL:
		if (puV<0.1)
			load.power = complex(0,0,J);
		else
		{	
			load.power.Re() *= puV; 
			load.power.Im() *= sqrt(puV);
		}
		break;
	case HID:
		if (puV<0.75)
			load.power = complex(0,0,J);
		else
			load.admittance = installed_power/VM2;
		break;
	default:
		break;
	}

	// update energy if clock is running
	if (t0>0 && t1>t0)
		load.energy = load.total.Mag()*gl_tohours(t1-t0);

	// update heatgain
	load.total = load.power + ~(load.current + load.admittance**pVoltage)**pVoltage/1000;
	load.heatgain = (placement==INDOOR ? (load.total.Mag() * BTUPHPKW) : 0);

	return TS_NEVER; 
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_lights(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(lights::oclass);
	if (*obj!=NULL)
	{
		lights *my = OBJECTDATA(*obj,lights);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_lights(OBJECT *obj)
{
	lights *my = OBJECTDATA(obj,lights);
	return my->init(obj->parent);
}

EXPORT TIMESTAMP sync_lights(OBJECT *obj, TIMESTAMP t0)
{
	lights *my = OBJECTDATA(obj,lights);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/
