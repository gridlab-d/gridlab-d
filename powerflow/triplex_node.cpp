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
		if (oclass==NULL)
			throw "unable to register class triplex_node";
		else
			oclass->trl = TRL_PROVEN;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "powerflow_object", // this is critical to avoid publishing node's 3phase properties
			PT_enumeration, "bustype", PADDR(bustype),
				PT_KEYWORD, "PQ", PQ,
				PT_KEYWORD, "PV", PV,
				PT_KEYWORD, "SWING", SWING,
			PT_set, "busflags", PADDR(busflags),
				PT_KEYWORD, "HASSOURCE", (set)NF_HASSOURCE,
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
			PT_double, "current_1_real[A]", PADDR(current1.Re()),
			PT_double, "current_2_real[A]", PADDR(current2.Re()),
			PT_double, "current_N_real[A]", PADDR(currentN.Re()),
			PT_double, "current_1_reac[A]", PADDR(current1.Im()),
			PT_double, "current_2_reac[A]", PADDR(current2.Im()),
			PT_double, "current_N_reac[A]", PADDR(currentN.Im()),
			PT_complex, "current_12[A]", PADDR(current12),
			PT_double, "current_12_real[A]", PADDR(current12.Re()),
			PT_double, "current_12_reac[A]", PADDR(current12.Im()),
			PT_complex, "residential_nominal_current_1[A]", PADDR(nom_res_curr[0]),
			PT_complex, "residential_nominal_current_2[A]", PADDR(nom_res_curr[1]),
			PT_complex, "residential_nominal_current_12[A]", PADDR(nom_res_curr[2]),
			PT_double, "residential_nominal_current_1_real[A]", PADDR(nom_res_curr[0].Re()),
			PT_double, "residential_nominal_current_1_imag[A]", PADDR(nom_res_curr[0].Im()),
			PT_double, "residential_nominal_current_2_real[A]", PADDR(nom_res_curr[1].Re()),
			PT_double, "residential_nominal_current_2_imag[A]", PADDR(nom_res_curr[1].Im()),
			PT_double, "residential_nominal_current_12_real[A]", PADDR(nom_res_curr[2].Re()),
			PT_double, "residential_nominal_current_12_imag[A]", PADDR(nom_res_curr[2].Im()),
			PT_complex, "power_1[VA]", PADDR(power1),
			PT_complex, "power_2[VA]", PADDR(power2),
			PT_complex, "power_12[VA]", PADDR(power12),
			PT_double, "power_1_real[W]", PADDR(power1.Re()),
			PT_double, "power_2_real[W]", PADDR(power2.Re()),
			PT_double, "power_12_real[W]", PADDR(power12.Re()),
			PT_double, "power_1_reac[VAr]", PADDR(power1.Im()),
			PT_double, "power_2_reac[VAr]", PADDR(power2.Im()),
			PT_double, "power_12_reac[VAr]", PADDR(power12.Im()),
			PT_complex, "shunt_1[S]", PADDR(pub_shunt[0]),
			PT_complex, "shunt_2[S]", PADDR(pub_shunt[1]),
			PT_complex, "shunt_12[S]", PADDR(pub_shunt[2]),
			PT_complex, "impedance_1[Ohm]", PADDR(impedance[0]),
			PT_complex, "impedance_2[Ohm]", PADDR(impedance[1]),
			PT_complex, "impedance_12[Ohm]", PADDR(impedance[2]),
			PT_double, "impedance_1_real[Ohm]", PADDR(impedance[0].Re()),
			PT_double, "impedance_2_real[Ohm]", PADDR(impedance[1].Re()),
			PT_double, "impedance_12_real[Ohm]", PADDR(impedance[2].Re()),
			PT_double, "impedance_1_reac[Ohm]", PADDR(impedance[0].Im()),
			PT_double, "impedance_2_reac[Ohm]", PADDR(impedance[1].Im()),
			PT_double, "impedance_12_reac[Ohm]", PADDR(impedance[2].Im()),
			PT_bool, "house_present", PADDR(house_present),
			PT_bool, "NR_mode", PADDR(NR_mode),
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
	pub_shunt[0] = pub_shunt[1] = pub_shunt[2] = 0;
	shunt1 = complex(0,0);
	shunt2 = complex(0,0);
	shunt12 = complex(0,0);
	return result;
}

int triplex_node::init(OBJECT *parent)
{
	if ((pub_shunt[0] == 0) && (impedance[0] != 0))
		shunt[0] = complex(1.0,0)/impedance[0];

	if ((pub_shunt[1] == 0) && (impedance[1] != 0))
		shunt[1] = complex(1.0,0)/impedance[1];

	if ((pub_shunt[2] == 0) && (impedance[2] != 0))
		shunt[2] = complex(1.0,0)/impedance[2];

	return node::init(parent);
}

TIMESTAMP triplex_node::presync(TIMESTAMP t0)
{
	if (solver_method == SM_NR)
		NR_mode = NR_cycle;		//COpy NR_cycle into NR_mode for houses
	else
		NR_mode = false;		//Just put as false for other methods

	//Clear the shunt values
	shunt[0] = shunt[1] = shunt[2] = 0.0;

	return node::presync(t0);
}

TIMESTAMP triplex_node::sync(TIMESTAMP t0)
{
	complex I;
	OBJECT *obj = OBJECTHDR(this);

	//Update shunt value here, otherwise it will only be a static value
	//Prioritizes shunt over impedance
	if ((pub_shunt[0] == 0) && (impedance[0] != 0))	//Impedance specified
		shunt[0] += complex(1.0,0)/impedance[0];
	else											//Shunt specified (impedance ignored)
		shunt[0] += pub_shunt[0];

	if ((pub_shunt[1] == 0) && (impedance[1] != 0))	//Impedance specified
		shunt[1] += complex(1.0,0)/impedance[1];		
	else											//Shunt specified (impedance ignored)
		shunt[1] += pub_shunt[1];

	if ((pub_shunt[2] == 0) && (impedance[2] != 0))	//Impedance specified
		shunt[2] += complex(1.0,0)/impedance[2];		
	else											//Shunt specified (impedance ignored)
		shunt[2] += pub_shunt[2];

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
		else
			return 0;
	}
	CREATE_CATCHALL(triplex_node);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_triplex_node(OBJECT *obj)
{
	try {
		triplex_node *my = OBJECTDATA(obj,triplex_node);
		return my->init();
	}
	INIT_CATCHALL(triplex_node);
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
	try {
		triplex_node *pObj = OBJECTDATA(obj,triplex_node);
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
	} 
	SYNC_CATCHALL(triplex_node);
}

EXPORT int isa_triplex_node(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,triplex_node)->isa(classname);
}

/**@}*/
