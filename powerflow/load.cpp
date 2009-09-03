/** $Id: load.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file load.cpp
	@addtogroup load
	@ingroup node
	
	Load objects represent static loads and export both voltages and current.  
		
	@{
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "node.h"
#include "meter.h"

CLASS* load::oclass = NULL;
CLASS* load::pclass = NULL;

load::load(MODULE *mod) : node(mod)
{
	if(oclass == NULL)
	{
		pclass = node::oclass;
		
		oclass = gl_register_class(mod,"load",sizeof(load),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if(oclass == NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
        
		if(gl_publish_variable(oclass,
			PT_INHERIT, "node",
			PT_enumeration, "load_class", PADDR(load_class),
				PT_KEYWORD, "U", LC_UNKNOWN,
				PT_KEYWORD, "R", LC_RESIDENTIAL,
				PT_KEYWORD, "C", LC_COMMERCIAL,
				PT_KEYWORD, "I", LC_INDUSTRIAL,
				PT_KEYWORD, "A", LC_AGRICULTURAL,
			PT_complex, "constant_power_A[VA]", PADDR(constant_power[0]),
			PT_complex, "constant_power_B[VA]", PADDR(constant_power[1]),
			PT_complex, "constant_power_C[VA]", PADDR(constant_power[2]),
			PT_double, "constant_power_A_real[W]", PADDR(constant_power[0].Re()),
			PT_double, "constant_power_B_real[W]", PADDR(constant_power[1].Re()),
			PT_double, "constant_power_C_real[W]", PADDR(constant_power[2].Re()),
			PT_double, "constant_power_A_reac[VAr]", PADDR(constant_power[0].Im()),
			PT_double, "constant_power_B_reac[VAr]", PADDR(constant_power[1].Im()),
			PT_double, "constant_power_C_reac[VAr]", PADDR(constant_power[2].Im()),
			PT_complex, "constant_current_A[A]", PADDR(constant_current[0]),
			PT_complex, "constant_current_B[A]", PADDR(constant_current[1]),
			PT_complex, "constant_current_C[A]", PADDR(constant_current[2]),
			PT_double, "constant_current_A_real[A]", PADDR(constant_current[0].Re()),
			PT_double, "constant_current_B_real[A]", PADDR(constant_current[1].Re()),
			PT_double, "constant_current_C_real[A]", PADDR(constant_current[2].Re()),
			PT_double, "constant_current_A_reac[A]", PADDR(constant_current[0].Im()),
			PT_double, "constant_current_B_reac[A]", PADDR(constant_current[1].Im()),
			PT_double, "constant_current_C_reac[A]", PADDR(constant_current[2].Im()),
			PT_complex, "constant_impedance_A[Ohm]", PADDR(constant_impedance[0]),
			PT_complex, "constant_impedance_B[Ohm]", PADDR(constant_impedance[1]),
			PT_complex, "constant_impedance_C[Ohm]", PADDR(constant_impedance[2]),
			PT_double, "constant_impedance_A_real[Ohm]", PADDR(constant_impedance[0].Re()),
			PT_double, "constant_impedance_B_real[Ohm]", PADDR(constant_impedance[1].Re()),
			PT_double, "constant_impedance_C_real[Ohm]", PADDR(constant_impedance[2].Re()),
			PT_double, "constant_impedance_A_reac[Ohm]", PADDR(constant_impedance[0].Im()),
			PT_double, "constant_impedance_B_reac[Ohm]", PADDR(constant_impedance[1].Im()),
			PT_double, "constant_impedance_C_reac[Ohm]", PADDR(constant_impedance[2].Im()),
			PT_complex,	"measured_voltage_A",PADDR(measured_voltage_A),
			PT_complex,	"measured_voltage_B",PADDR(measured_voltage_B),
			PT_complex,	"measured_voltage_C",PADDR(measured_voltage_C),
			PT_complex,	"measured_voltage_AB",PADDR(measured_voltage_AB),
			PT_complex,	"measured_voltage_BC",PADDR(measured_voltage_BC),
			PT_complex,	"measured_voltage_CA",PADDR(measured_voltage_CA),

         	NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int load::isa(char *classname)
{
	return strcmp(classname,"load")==0 || node::isa(classname);
}

int load::create(void)
{
	int res = node::create();
        
	maximum_voltage_error = 0;
	load_class = LC_UNKNOWN;

    return res;
}

TIMESTAMP load::presync(TIMESTAMP t0)
{
	if ((solver_method==SM_GS) && (SubNode==PARENT))	//Need to do something slightly different with GS and parented node
	{
		shunt[0] = shunt[1] = shunt[2] = 0.0;
		power[0] = power[1] = power[2] = 0.0;
		current[0] = current[1] = current[2] = 0.0;
	}
	
	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::presync(t0);
	
	return result;
}

TIMESTAMP load::sync(TIMESTAMP t0)
{
	if ((solver_method==SM_GS) && (SubNode==PARENT))	//Need to do something slightly different with GS and parented load
	{													//associated with change due to player methods

		if (!(constant_impedance[0].IsZero()))
			shunt[0] += complex(1.0)/constant_impedance[0];

		if (!(constant_impedance[1].IsZero()))
			shunt[1] += complex(1.0)/constant_impedance[1];
		
		if (!(constant_impedance[2].IsZero()))
			shunt[2] += complex(1.0)/constant_impedance[2];
		
		power[0] += constant_power[0];
		power[1] += constant_power[1];	
		power[2] += constant_power[2];
		current[0] += constant_current[0];
		current[1] += constant_current[1];
		current[2] += constant_current[2];
	}
	else
	{
		if(constant_impedance[0].IsZero())
			shunt[0] = 0.0;
		else
			shunt[0] = complex(1)/constant_impedance[0];

		if(constant_impedance[1].IsZero())
			shunt[1] = 0.0;
		else
			shunt[1] = complex(1)/constant_impedance[1];
		
		if(constant_impedance[2].IsZero())
			shunt[2] = 0.0;
		else
			shunt[2] = complex(1)/constant_impedance[2];
		
		power[0] = constant_power[0];
		power[1] = constant_power[1];	
		power[2] = constant_power[2];
		current[0] = constant_current[0];
		current[1] = constant_current[1];
		current[2] = constant_current[2];
	}

	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::sync(t0);

	measured_voltage_A.SetPolar(voltageA.Mag(),voltageA.Arg());  //Used for testing and xml output
	measured_voltage_B.SetPolar(voltageB.Mag(),voltageB.Arg());
	measured_voltage_C.SetPolar(voltageC.Mag(),voltageC.Arg());
	measured_voltage_AB = measured_voltage_A-measured_voltage_B;
	measured_voltage_BC = measured_voltage_B-measured_voltage_C;
	measured_voltage_CA = measured_voltage_C-measured_voltage_A;
	
	return result;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: load
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_load(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(load::oclass);
		if (*obj!=NULL)
		{
			load *my = OBJECTDATA(*obj,load);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_load: %s", msg);
	}
	return 0;
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to the object to be initialized
* @return 1 on success, 0 on error
*/
EXPORT int init_load(OBJECT *obj)
{
	int result = 1;
	try {
		result = ((load*) (obj + 1))->init();
	} catch (const char *error) {
		GL_THROW("%s:%d: %s", obj->oclass->name, obj->id, error);
		return 0; 
	} catch (...) {
		GL_THROW("%s:%d: %s", obj->oclass->name, obj->id, "unknown exception");
		return 0;
	}
	return result;
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_load(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		load *pObj = OBJECTDATA(obj,load);
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(t0);
		case PC_BOTTOMUP:
			return pObj->sync(t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
	} catch (const char *error) {
		GL_THROW("%s (load:%d): %s", obj->name, obj->oclass->name, obj->id, error);
		return 0; 
	} catch (...) {
		GL_THROW("%s (load:%d): %s", obj->name, obj->oclass->name, obj->id, "unknown exception");
		return 0;
	}
}

EXPORT int isa_load(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,load)->isa(classname);
}

/**@}*/
