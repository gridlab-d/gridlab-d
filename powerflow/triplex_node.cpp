/** $Id: triplex_node.cpp 1186 2009-01-02 18:15:30Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file triplex_node.cpp
	@addtogroup triplex_node
	@ingroup powerflow_node
	
	@{
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "node.h"
#include "meter.h"

CLASS* triplex_node::oclass = NULL;
CLASS* triplex_node::pclass = NULL;

triplex_node::triplex_node(MODULE *mod) : node(mod)
{
	if(oclass == NULL)
	{
		pclass = node::oclass;
		
		oclass = gl_register_class(mod,"triplex_node",sizeof(triplex_node),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if(oclass == NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
        
		if(gl_publish_variable(oclass,
			PT_INHERIT, "powerflow_object", // this is critical to avoid publishing node's 3phase properties
			PT_enumeration, "bustype", PADDR(bustype),
				PT_KEYWORD, "PQ", PQ,
				PT_KEYWORD, "PV", PV,
				PT_KEYWORD, "SWING", SWING,
			PT_set, "busflags", PADDR(busflags),
				PT_KEYWORD, "HASSOURCE", NF_HASSOURCE,
			PT_object, "reference_bus", PADDR(reference_bus),
			PT_double,"maximum_voltage_error[V]",PADDR(maximum_voltage_error),

 			PT_complex, "voltage_1[V]", PADDR(voltage1),
			PT_complex, "voltage_2[V]", PADDR(voltage2),
			PT_complex, "voltage_N[V]", PADDR(voltageN),
			PT_complex, "voltage_12[V]", PADDR(voltage12),
			PT_complex, "voltage_1N[V]", PADDR(voltage1N),
			PT_complex, "voltage_2N[V]", PADDR(voltage2N),
			PT_complex, "current_1[A]", PADDR(current1),
			PT_complex, "current_2[A]", PADDR(current2),
			PT_complex, "current_N[A]", PADDR(currentN),
			PT_complex, "power_1[VA]", PADDR(power1),
			PT_complex, "power_2[VA]", PADDR(power2),
			PT_complex, "power_N[VA]", PADDR(powerN),
			PT_complex, "shunt_1[S]", PADDR(shunt1),
			PT_complex, "shunt_2[S]", PADDR(shunt2),
			PT_complex, "shunt_N[S]", PADDR(shuntN),
			PT_complex, "impedance_1[Ohm]", PADDR(impedance[0]),
			PT_complex, "impedance_2[Ohm]", PADDR(impedance[1]),
			PT_complex, "impedance_N[Ohm]", PADDR(impedance[2]),

         	NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int triplex_node::isa(char *classname)
{
	return strcmp(classname,"triplex_node")==0 || node::isa(classname);
}

int triplex_node::create(void)
{
	int result = node::create();
	maximum_voltage_error = 0;
	shunt1 = complex(0,0);
	shunt2 = complex(0,0);
	shuntN = complex(0,0);
    return result;
}

int triplex_node::init(OBJECT *parent)
{
	if ((shunt1.IsZero())&&(impedance[0]!=0))
		shunt1 = complex(1,0)/impedance[0];
	if ((shunt2.IsZero())&&(impedance[1]!=0))
		shunt2 = complex(1,0)/impedance[1];
	if ((shuntN.IsZero())&&(impedance[2]!=0))
		shuntN = complex(1,0)/impedance[2];
	return node::init(parent);
}

TIMESTAMP triplex_node::sync(TIMESTAMP t0)
{
	complex I;
	OBJECT *obj = OBJECTHDR(this);

	return node::sync(t0);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: triplex_node
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_triplex_node(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(triplex_node::oclass);
		if (*obj!=NULL)
		{
			triplex_node *my = OBJECTDATA(*obj,triplex_node);
			gl_set_parent(*obj,parent);
			return my->create();
		}	
	}
	catch (char *msg)
	{
		gl_error("create_triplex_node: %s", msg);
	}
	return 0;
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_triplex_node(OBJECT *obj)
{
	triplex_node *my = OBJECTDATA(obj,triplex_node);
	try {
		return my->init();
	}
	catch (char *msg)
	{
		GL_THROW("%s (triplex_node:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_triplex_node(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	triplex_node *pObj = OBJECTDATA(obj,triplex_node);
	try {
		TIMESTAMP t1;
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
		GL_THROW("%s (triplex_node:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return 0; 
	} catch (...) {
		GL_THROW("%s (triplex_node:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
		return 0;
	}
}

EXPORT int isa_triplex_node(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,triplex_node)->isa(classname);
}

/**@}*/
