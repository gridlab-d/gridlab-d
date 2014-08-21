/** $Id: lights.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file lights.cpp
	@addtogroup residential_lights Lights
	@ingroup residential

	The residential lighting object supports the major types of residential
	lights (see lights::type).  The lighting type effects the power factor
	used (see lights::load.power_factor).

	The lighting enduse is driven by an analog loadshape which is driven by
	a general purpose schedule.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "residential.h"
#include "lights.h"

//////////////////////////////////////////////////////////////////////////
// lights CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* lights::oclass = NULL;
CLASS* lights::pclass = NULL;

double lights::power_factor[_MAXTYPES] = {
	1.00, // INCANDESCENT
	0.95, // FLUORESCENT
	0.92, // CFL
	0.90, // SSL
	0.97, // HID
};

double lights::power_fraction[_MAXTYPES][3] = {
	// Z I P - should add to 1
	1, 0, 0,// INCANDESCENT
	0.4, 0, 0.6,// FLUORESCENT
	0.4, 0, 0.6,// CFL
	0.80, 0.1, 0.1,// SSL - these values are unvalidated
	0.80, 0.1, 0.1,// HID - these values are unvalidated
};

// the constructor registers the class and properties and sets the defaults
lights::lights(MODULE *mod) 
: residential_enduse(mod)
{
	// first time init
	if (oclass==NULL)
	{
		pclass = residential_enduse::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"lights",sizeof(lights),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class lights";
			/* TROUBLESHOOT
				The file that implements the lights in the residential module cannot register the class.
				This is an internal error.  Contact support for assistance.
			 */
		else
			oclass->trl = TRL_QUALIFIED;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_enumeration,"type",PADDR(type), PT_DESCRIPTION, "lighting type (affects power_factor)",
				PT_KEYWORD,"INCANDESCENT",(enumeration)INCANDESCENT,
				PT_KEYWORD,"FLUORESCENT",(enumeration)FLUORESCENT,
				PT_KEYWORD,"CFL",(enumeration)CFL,
				PT_KEYWORD,"SSL",(enumeration)SSL,
				PT_KEYWORD,"HID",(enumeration)HID,
			PT_enumeration,"placement",PADDR(placement), PT_DESCRIPTION, "lighting location (affects where heatgains go)",
				PT_KEYWORD,"INDOOR",(enumeration)INDOOR,
				PT_KEYWORD,"OUTDOOR",(enumeration)OUTDOOR,
			PT_double,"installed_power[kW]",PADDR(shape.params.analog.power), PT_DESCRIPTION, "installed lighting capacity",
			PT_double,"power_density[W/sf]",PADDR(power_density), PT_DESCRIPTION, "installed power density",
			PT_double,"curtailment[pu]", PADDR(curtailment), PT_DESCRIPTION, "lighting curtailment factor",
			PT_double,"demand[pu]", PADDR(shape.load), PT_DESCRIPTION, "the current lighting demand",
			PT_complex,"actual_power[kVA]", PADDR(lights_actual_power), PT_DESCRIPTION, "actual power demand of lights object",
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The file that implements the specified class cannot publisht the variables in the class.
				This is an internal error.  Contact support for assistance.
			 */
	}
}

// create is called every time a new object is loaded
int lights::create(void) 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;
	load.power_fraction = load.current_fraction = load.impedance_fraction = 0;
	load.power = load.admittance = load.current = load.total = complex(0,0,J);
	load.voltage_factor = 1.0;
	//load.power_factor = 0.95; commenting out this line means a default power factor of 1.00
	load.breaker_amps = 0;

	return res;
}

int lights::init(OBJECT *parent)
{
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("lights::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	// check the load configuration before initializing the parent class
	if(shape.load > 1.0)
		gl_warning("lighting load %f exceeds installed capacity", shape.load);
		/* TROUBLESHOOT
			The lighting load cannot exceed the installed capacity.  
			Use a lighting load that is less than or equal to 1.0 and try again.
		 */
	else if (shape.load < 0.0)
		gl_warning("lights load %f is negative", shape.load);
		/* TROUBLESHOOT
			The lighting load cannot be negative.
			Use a positive lighting load and try again.
		 */

	if (load.power_factor==0)
	{
		load.power_factor = power_factor[type];
		gl_warning("No value was given for power factor. Defaulting to power factor 1.00 and incandescent light type.");
	}
	if (load.voltage_factor==0) 
	{
		load.voltage_factor = 1.0;
		gl_warning("No value was given for voltage factor.  Defaulting to voltage factor 1.00");
	}
	if ( (load.power_fraction + load.current_fraction + load.impedance_fraction) == 0.0)
	{
		load.power_fraction = power_fraction[type][2];
		load.current_fraction = power_fraction[type][1];
		load.impedance_fraction = power_fraction[type][0];
		gl_warning("No value was given for power fraction. Defaulting to incandescent light type and corresponding power fraction 1,0,0.");
	}
	// check for the right kind of loadshape schedule 
	if (shape.type!=MT_ANALOG && shape.type != MT_UNKNOWN)
		throw("residential lighting only supports analog loadshapes");
		/* TROUBLESHOOT
			Lighting load use analog loadshapes.
			Change the load shape type to analogy and try again.
		 */
	if (shape.params.analog.energy>0)
		throw("residential lighting does not support fixed energy");
		/* TROUBLESHOOT
			It is not possible to define the total energy consumed by a lighting load in relation to a schedule.
			Use either scaled power or installed power to define the lighting load and try again.
		 */

	// installed power intially re_overrides use of power density
	double *floor_area = parent?gl_get_double_by_name(parent, "floor_area"):NULL;
	if (shape.params.analog.power==0 && shape.schedule==NULL) 
	{		
		// set basic properties
		if (power_density==0) power_density = gl_random_triangle(RNGSTATE,0.75, 1.25);  // W/sf

		if(floor_area == NULL)
		{
			gl_error("lights parent must publish \'floor_area\' to work properly if no installed_power is given ~ default 2500 sf");
			/* TROUBLESHOOT
				The lights did not reference a parent object that publishes a floor are, so 2500 sf was assumed.
				Change the parent reference and try again.
			 */
			shape.params.analog.power = power_density * 2500 / 1000;
		} else {
			shape.params.analog.power = power_density * *floor_area / 1000;
		}
	}
	else if (power_density==0 && shape.params.analog.power>0)
	{
		if (floor_area!=NULL)
			power_density = shape.params.analog.power / *floor_area ;
		else
			power_density = shape.params.analog.power / 2500;
	}

	//load.power_factor = power_factor[type];
	if(load.breaker_amps == 0)
		load.breaker_amps = 40;

	if(placement == INDOOR){
		load.heatgain_fraction = 0.90;
	} else if (placement == OUTDOOR){
		load.heatgain_fraction = 0.0;
	}

	// circuit config and breaker amps

	// waiting this long to initialize the parent class is normal
	return residential_enduse::init(parent);
}

int lights::isa(char *classname)
{
	return (strcmp(classname,"lights")==0 || residential_enduse::isa(classname));
}

TIMESTAMP lights::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	double val = 0.0;
	TIMESTAMP t2 = TS_NEVER;

	if (pCircuit!=NULL)
		load.voltage_factor = pCircuit->pV->Mag() / 120; // update voltage factor

	t2 = residential_enduse::sync(t0,t1);

	if(shape.type == MT_UNKNOWN){ /* manual power calculation*/
		double frac = shape.load * (1-curtailment);
		if(shape.load < 0){
			gl_warning("lights shape demand is negative, capping to 0");
			shape.load = 0.0;
		} else if (shape.load > 1.0){
			gl_warning("lights shape demand exceeds installed lighting power, capping to 100%%");
			shape.load = 1.0;
		}
		load.power = shape.params.analog.power * shape.load;
		if(fabs(load.power_factor) < 1){
			val = (load.power_factor<0?-1.0:1.0) * load.power.Re() * sqrt(1/(load.power_factor * load.power_factor) - 1);
		} else {
			val = 0;
		}
		load.power.SetRect(load.power.Re(), val);
	}

	gl_enduse_sync(&(residential_enduse::load),t1);
	lights_actual_power = load.power + (load.current + load.admittance * load.voltage_factor) * load.voltage_factor;

	return t2;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_lights(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(lights::oclass);
		if (*obj!=NULL)
		{
			lights *my = OBJECTDATA(*obj,lights);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(lights);
}

EXPORT int init_lights(OBJECT *obj)
{
	try {
		lights *my = OBJECTDATA(obj,lights);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(lights);
}

EXPORT int isa_lights(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,lights)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_lights(OBJECT *obj, TIMESTAMP t1)
{
	try {
		lights *my = OBJECTDATA(obj,lights);
		TIMESTAMP t2 = my->sync(obj->clock, t1);
		obj->clock = t1;
		return t2;
	}
	SYNC_CATCHALL(lights);
}

/**@}**/
