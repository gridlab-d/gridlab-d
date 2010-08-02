/** $Id: substation.cpp
	Copyright (C) 2009 Battelle Memorial Institute
	@file substation.cpp
	@addtogroup powerflow_meter Substation
	@ingroup powerflow

	Substation object serves as a connecting object between the powerflow and
	network solvers.  The three-phase unbalanced connections of the distribution
	solver are posted as an equivalent per-unit load on the network solver.  The
	network solver per-unit voltage is translated into a balanced three-phase voltage
	for the distribution solver.  This functionality assumes the substation is treated
	as the swing bus for the distribution solver.

	Total cumulative energy, instantantenous power and peak demand are metered.

	@{
 **/
//Built from meter object
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "substation.h"
#include "timestamp.h"

// substation reset function
EXPORT int64 substation_reset(OBJECT *obj)
{
	substation *pSubstation = OBJECTDATA(obj,substation);
	pSubstation->distribution_demand = 0;
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// SUBSTATION CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
// class management data
CLASS* substation::oclass = NULL;
CLASS* substation::pclass = NULL;

// the constructor registers the class and properties and sets the defaults
substation::substation(MODULE *mod) : node(mod)
{
	// first time init
	if (oclass==NULL)
	{
		// link to parent class (used by isa)
		pclass = node::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"substation",sizeof(substation),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties - based around meter object
		if (gl_publish_variable(oclass,
			PT_INHERIT, "node",
			/// @todo three-phase meter should meter Q also (required complex)
			PT_double, "distribution_energy[Wh]", PADDR(distribution_energy),
			PT_complex, "distribution_power[VA]", PADDR(distribution_power),
			PT_double, "distribution_demand[W]", PADDR(distribution_demand),
			
			// added to record last voltage/current
			PT_complex, "distribution_voltage_A[V]", PADDR(distribution_voltage[0]),
			PT_complex, "distribution_voltage_B[V]", PADDR(distribution_voltage[1]),
			PT_complex, "distribution_voltage_C[V]", PADDR(distribution_voltage[2]),
			PT_complex, "distribution_current_A[A]", PADDR(distribution_current[0]),
			PT_complex, "distribution_current_B[A]", PADDR(distribution_current[1]),
			PT_complex, "distribution_current_C[A]", PADDR(distribution_current[2]),

			// Information on Network solver connection
			PT_double, "Network_Node_Base_Power[MVA]", PADDR(Network_Node_Base_Power),
			PT_double, "Network_Node_Base_Voltage[V]", PADDR(Network_Node_Base_Voltage),

			//PT_double, "measured_reactive[kVar]", PADDR(measured_reactive), has not implemented yet
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// publish meter reset function
		if (gl_publish_function(oclass,"reset",(FUNCTIONADDR)substation_reset)==NULL)
			GL_THROW("unable to publish substation_reset function in %s",__FILE__);
		}
}

int substation::isa(char *classname)
{
	return strcmp(classname,"substation")==0 || node::isa(classname);
}

// Create a distribution meter from the defaults template, return 1 on success
int substation::create()
{
	int result = node::create();
	
	distribution_voltage[0] = distribution_voltage[1] = distribution_voltage[2] = complex(0,0,A);
	distribution_current[0] = distribution_current[1] = distribution_current[2] = complex(0,0,J);
	distribution_energy = 0.0;
	distribution_power = complex(0,0,J);
	distribution_demand = 0.0;

	return result;
}

// Initialize a distribution meter, return 1 on success
int substation::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	
	if (hdr->parent==NULL)	//Make sure we have a parent
		GL_THROW("You must specify a parent network node for a substation");
	
	if (hdr->oclass->module==hdr->parent->oclass->module)	//Make sure modules aren't the same
		GL_THROW("Parent object is from same module.  Substations bridge the network and powerflow solvers.");
	
	if (!(gl_object_isa(hdr->parent,"node","network")))
		GL_THROW("Parent object must be a node of the network module");

	return node::init(parent);
}

// Synchronize a distribution meter
TIMESTAMP substation::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	complex PU_Power, NonPU_Voltage;
	complex *NetNodePower;
	complex *NetNodeVoltage;
	complex angle_adjustment;
	OBJECT *hdr = OBJECTHDR(this);
	OBJECT *nethdr = hdr->parent;

	//Find power and voltage settings
	PROPERTY *power_S = gl_get_property(nethdr,"S");
	PROPERTY *voltage_V = gl_get_property(nethdr,"V");

	NetNodePower = (complex*)GETADDR(nethdr,power_S);
	NetNodeVoltage = (complex*)GETADDR(nethdr,voltage_V);

	// compute demand power
	if (distribution_power>distribution_demand) 
		distribution_demand=distribution_power.Mag();

	// compute energy use
	if (t0>0)
		distribution_energy += distribution_power.Mag() * (t1 - t0)*3600;

	//Update connected node.  Give it our power, take its voltage
	
	/**********************************************/
	/* Temporary algorithm for object testing     */
	/**********************************************/
		
		//Scale power
		PU_Power = distribution_power / (Network_Node_Base_Power * 1000000.0);	//Scale to MVA

		LOCK_OBJECT(nethdr);	//Lock network solver object just in case
		//Scale voltage
		NonPU_Voltage = *NetNodeVoltage*Network_Node_Base_Voltage;

		//Put our power on the network node (one phases worth)
		*NetNodePower = complex(PU_Power.Re() / 3.0, PU_Power.Im() / 3.0, J);
		UNLOCK_OBJECT(nethdr);

		//Put network node's voltage as our own (balanced offsets)
		angle_adjustment = complex(1,sqrt(3.0));
		angle_adjustment /=-2.0;	// 1_/ -120d

		//Update voltages - both Wye and Delta
		LOCK_OBJECT(hdr);
		voltage[0] = NonPU_Voltage;
		voltage[1] = NonPU_Voltage * angle_adjustment;
		voltage[2] = NonPU_Voltage * ~angle_adjustment;

		voltaged[0]=voltage[0]-voltage[1];
		voltaged[1]=voltage[1]-voltage[2];
		voltaged[2]=voltage[2]-voltage[0];

		UNLOCK_OBJECT(hdr);

	/**********************************************/
	/*			End temp algorithm				  */
	/**********************************************/

	return node::presync(t1);
}

TIMESTAMP substation::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP result;

	result = node::postsync(t1);
	distribution_voltage[0] = voltageA;
	distribution_voltage[1] = voltageB;
	distribution_voltage[2] = voltageC;
	distribution_current[0] = current_inj[0];
	distribution_current[1] = current_inj[1];
	distribution_current[2] = current_inj[2];
	distribution_power = distribution_voltage[0]*(~distribution_current[0])
				   + distribution_voltage[1]*(~distribution_current[1])
				   + distribution_voltage[2]*(~distribution_current[2]);
	return result;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int isa_substation(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,substation)->isa(classname);
}

EXPORT int create_substation(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(substation::oclass);
		if (*obj!=NULL)
		{
			substation *my = OBJECTDATA(*obj,substation);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("create_substation: %s", msg);
	}
	return 0;
}

EXPORT int init_substation(OBJECT *obj)
{
	substation *my = OBJECTDATA(obj,substation);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s (substation:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
}

EXPORT TIMESTAMP sync_substation(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	substation *pObj = OBJECTDATA(obj,substation);
	try {
		TIMESTAMP t1;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(obj->clock,t0);
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
		gl_error("%s (substation:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return TS_INVALID; 
	} catch (...) {
		gl_error("%s (substation:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
		return TS_INVALID;
	}
}

/**@}**/
