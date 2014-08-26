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
		
		oclass = gl_register_class(mod,"triplex_node",sizeof(triplex_node),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class triplex_node";
		else
			oclass->trl = TRL_PROVEN;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "powerflow_object", // this is critical to avoid publishing node's 3phase properties
			PT_enumeration, "bustype", PADDR(bustype),PT_DESCRIPTION,"defines whether the node is a PQ, PV, or SWING node",
				PT_KEYWORD, "PQ", (enumeration)PQ,
				PT_KEYWORD, "PV", (enumeration)PV,
				PT_KEYWORD, "SWING", (enumeration)SWING,
			PT_set, "busflags", PADDR(busflags),PT_DESCRIPTION,"flag indicates node has a source for voltage, i.e. connects to the swing node",
				PT_KEYWORD, "HASSOURCE", (set)NF_HASSOURCE,
			PT_object, "reference_bus", PADDR(reference_bus),PT_DESCRIPTION,"reference bus from which frequency is defined",
			PT_double,"maximum_voltage_error[V]",PADDR(maximum_voltage_error),PT_DESCRIPTION,"convergence voltage limit or convergence criteria",

 			PT_complex, "voltage_1[V]", PADDR(voltage1),PT_DESCRIPTION,"bus voltage, phase 1 to ground",
			PT_complex, "voltage_2[V]", PADDR(voltage2),PT_DESCRIPTION,"bus voltage, phase 2 to ground",
			PT_complex, "voltage_N[V]", PADDR(voltageN),PT_DESCRIPTION,"bus voltage, phase N to ground",
			PT_complex, "voltage_12[V]", PADDR(voltage12),PT_DESCRIPTION,"bus voltage, phase 1 to 2",
			PT_complex, "voltage_1N[V]", PADDR(voltage1N),PT_DESCRIPTION,"bus voltage, phase 1 to N",
			PT_complex, "voltage_2N[V]", PADDR(voltage2N),PT_DESCRIPTION,"bus voltage, phase 2 to N",
			PT_complex, "current_1[A]", PADDR(current1),PT_DESCRIPTION,"constant current load on phase 1, also acts as accumulator",
			PT_complex, "current_2[A]", PADDR(current2),PT_DESCRIPTION,"constant current load on phase 2, also acts as accumulator",
			PT_complex, "current_N[A]", PADDR(currentN),PT_DESCRIPTION,"constant current load on phase N, also acts as accumulator",
			PT_double, "current_1_real[A]", PADDR(current1.Re()),PT_DESCRIPTION,"constant current load on phase 1, real",
			PT_double, "current_2_real[A]", PADDR(current2.Re()),PT_DESCRIPTION,"constant current load on phase 2, real",
			PT_double, "current_N_real[A]", PADDR(currentN.Re()),PT_DESCRIPTION,"constant current load on phase N, real",
			PT_double, "current_1_reac[A]", PADDR(current1.Im()),PT_DESCRIPTION,"constant current load on phase 1, imag",
			PT_double, "current_2_reac[A]", PADDR(current2.Im()),PT_DESCRIPTION,"constant current load on phase 2, imag",
			PT_double, "current_N_reac[A]", PADDR(currentN.Im()),PT_DESCRIPTION,"constant current load on phase N, imag",
			PT_complex, "current_12[A]", PADDR(current12),PT_DESCRIPTION,"constant current load on phase 1 to 2",
			PT_double, "current_12_real[A]", PADDR(current12.Re()),PT_DESCRIPTION,"constant current load on phase 1 to 2, real",
			PT_double, "current_12_reac[A]", PADDR(current12.Im()),PT_DESCRIPTION,"constant current load on phase 1 to 2, imag",
			PT_complex, "residential_nominal_current_1[A]", PADDR(nom_res_curr[0]),PT_DESCRIPTION,"posted current on phase 1 from a residential object, if attached",
			PT_complex, "residential_nominal_current_2[A]", PADDR(nom_res_curr[1]),PT_DESCRIPTION,"posted current on phase 2 from a residential object, if attached",
			PT_complex, "residential_nominal_current_12[A]", PADDR(nom_res_curr[2]),PT_DESCRIPTION,"posted current on phase 1 to 2 from a residential object, if attached",
			PT_double, "residential_nominal_current_1_real[A]", PADDR(nom_res_curr[0].Re()),PT_DESCRIPTION,"posted current on phase 1, real, from a residential object, if attached",
			PT_double, "residential_nominal_current_1_imag[A]", PADDR(nom_res_curr[0].Im()),PT_DESCRIPTION,"posted current on phase 1, imag, from a residential object, if attached",
			PT_double, "residential_nominal_current_2_real[A]", PADDR(nom_res_curr[1].Re()),PT_DESCRIPTION,"posted current on phase 2, real, from a residential object, if attached",
			PT_double, "residential_nominal_current_2_imag[A]", PADDR(nom_res_curr[1].Im()),PT_DESCRIPTION,"posted current on phase 2, imag, from a residential object, if attached",
			PT_double, "residential_nominal_current_12_real[A]", PADDR(nom_res_curr[2].Re()),PT_DESCRIPTION,"posted current on phase 1 to 2, real, from a residential object, if attached",
			PT_double, "residential_nominal_current_12_imag[A]", PADDR(nom_res_curr[2].Im()),PT_DESCRIPTION,"posted current on phase 1 to 2, imag, from a residential object, if attached",
			PT_complex, "power_1[VA]", PADDR(power1),PT_DESCRIPTION,"constant power on phase 1 (120V)",
			PT_complex, "power_2[VA]", PADDR(power2),PT_DESCRIPTION,"constant power on phase 2 (120V)",
			PT_complex, "power_12[VA]", PADDR(power12),PT_DESCRIPTION,"constant power on phase 1 to 2 (240V)",
			PT_double, "power_1_real[W]", PADDR(power1.Re()),PT_DESCRIPTION,"constant power on phase 1, real",
			PT_double, "power_2_real[W]", PADDR(power2.Re()),PT_DESCRIPTION,"constant power on phase 2, real",
			PT_double, "power_12_real[W]", PADDR(power12.Re()),PT_DESCRIPTION,"constant power on phase 1 to 2, real",
			PT_double, "power_1_reac[VAr]", PADDR(power1.Im()),PT_DESCRIPTION,"constant power on phase 1, imag",
			PT_double, "power_2_reac[VAr]", PADDR(power2.Im()),PT_DESCRIPTION,"constant power on phase 2, imag",
			PT_double, "power_12_reac[VAr]", PADDR(power12.Im()),PT_DESCRIPTION,"constant power on phase 1 to 2, imag",
			PT_complex, "shunt_1[S]", PADDR(pub_shunt[0]),PT_DESCRIPTION,"constant shunt impedance on phase 1",
			PT_complex, "shunt_2[S]", PADDR(pub_shunt[1]),PT_DESCRIPTION,"constant shunt impedance on phase 2",
			PT_complex, "shunt_12[S]", PADDR(pub_shunt[2]),PT_DESCRIPTION,"constant shunt impedance on phase 1 to 2",
			PT_complex, "impedance_1[Ohm]", PADDR(impedance[0]),PT_DESCRIPTION,"constant series impedance on phase 1",
			PT_complex, "impedance_2[Ohm]", PADDR(impedance[1]),PT_DESCRIPTION,"constant series impedance on phase 2",
			PT_complex, "impedance_12[Ohm]", PADDR(impedance[2]),PT_DESCRIPTION,"constant series impedance on phase 1 to 2",
			PT_double, "impedance_1_real[Ohm]", PADDR(impedance[0].Re()),PT_DESCRIPTION,"constant series impedance on phase 1, real",
			PT_double, "impedance_2_real[Ohm]", PADDR(impedance[1].Re()),PT_DESCRIPTION,"constant series impedance on phase 2, real",
			PT_double, "impedance_12_real[Ohm]", PADDR(impedance[2].Re()),PT_DESCRIPTION,"constant series impedance on phase 1 to 2, real",
			PT_double, "impedance_1_reac[Ohm]", PADDR(impedance[0].Im()),PT_DESCRIPTION,"constant series impedance on phase 1, imag",
			PT_double, "impedance_2_reac[Ohm]", PADDR(impedance[1].Im()),PT_DESCRIPTION,"constant series impedance on phase 2, imag",
			PT_double, "impedance_12_reac[Ohm]", PADDR(impedance[2].Im()),PT_DESCRIPTION,"constant series impedance on phase 1 to 2, imag",
			PT_bool, "house_present", PADDR(house_present),PT_DESCRIPTION,"boolean for detecting whether a house is attached, not an input",
			PT_enumeration, "service_status", PADDR(service_status),PT_DESCRIPTION,"In and out of service flag",
				PT_KEYWORD, "IN_SERVICE", (enumeration)ND_IN_SERVICE,
				PT_KEYWORD, "OUT_OF_SERVICE", (enumeration)ND_OUT_OF_SERVICE,
			PT_double, "service_status_double", PADDR(service_status_dbl),PT_DESCRIPTION,"In and out of service flag - type double - will indiscriminately override service_status - useful for schedules",
			PT_double, "previous_uptime[min]", PADDR(previous_uptime),PT_DESCRIPTION,"Previous time between disconnects of node in minutes",
			PT_double, "current_uptime[min]", PADDR(current_uptime),PT_DESCRIPTION,"Current time since last disconnect of node in minutes",
			PT_object, "topological_parent", PADDR(TopologicalParent),PT_DESCRIPTION,"topological parent as per GLM configuration",
         	NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

			//Deltamode functions
			if (gl_publish_function(oclass,	"delta_linkage_node", (FUNCTIONADDR)delta_linkage)==NULL)
				GL_THROW("Unable to publish triplex_node delta_linkage function");
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_triplex_node)==NULL)
				GL_THROW("Unable to publish triplex_node deltamode function");
			if (gl_publish_function(oclass,	"delta_freq_pwr_object", (FUNCTIONADDR)delta_frequency_node)==NULL)
				GL_THROW("Unable to publish triplex_node deltamode function");
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
	service_status = ND_IN_SERVICE;
	return result;
}

int triplex_node::init(OBJECT *parent)
{
	if ( !(has_phase(PHASE_S)) )
	{
		OBJECT *obj = OBJECTHDR(this);
		gl_warning("Init() triplex_node (name:%s, id:%d): Phases specified did not include phase S. Adding phase S.", obj->name,obj->id);
		/* TROUBLESHOOT
		Triplex nodes and meters require a single phase and a phase S component (for split-phase).
		This particular triplex object did not include it, so it is being added.
		*/
		phases = (phases | PHASE_S);
	}
	if ((pub_shunt[0] == 0) && (impedance[0] != 0))
		shunt[0] = complex(1.0,0)/impedance[0];

	if ((pub_shunt[1] == 0) && (impedance[1] != 0))
		shunt[1] = complex(1.0,0)/impedance[1];

	if ((pub_shunt[2] == 0) && (impedance[2] != 0))
		shunt[2] = complex(1.0,0)/impedance[2];

	return node::init(parent);
}

//Functionalized triplex_node presync -- mainly so 
//External deltamode calls still execute things right
void triplex_node::BOTH_triplex_node_presync_fxn(void)
{
	//Clear the shunt values
	shunt[0] = shunt[1] = shunt[2] = 0.0;
}

TIMESTAMP triplex_node::presync(TIMESTAMP t0)
{
	//Call the functionalized version
	BOTH_triplex_node_presync_fxn();

	return node::presync(t0);
}

//Functionalized sync routine
//For external calls
void triplex_node::BOTH_triplex_node_sync_fxn(void)
{
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
}

TIMESTAMP triplex_node::sync(TIMESTAMP t0)
{
	//Call the functionalized version
	BOTH_triplex_node_sync_fxn();

	return node::sync(t0);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Module-level call
SIMULATIONMODE triplex_node::inter_deltaupdate_triplex_node(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	//unsigned char pass_mod;
	OBJECT *hdr = OBJECTHDR(this);

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Call triplex-specific call
		BOTH_triplex_node_presync_fxn();

		//Call node presync-equivalent items
		NR_node_presync_fxn();

		//Call sync-equivalent of triplex portion first
		BOTH_triplex_node_sync_fxn();

		//Call node sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}
	else	//After the call
	{
		//No triplex-specific postsync for node

		//Perform node postsync-like updates on the values
		BOTH_node_postsync_fxn(hdr);

		//No control required at this time - powerflow defers to the whims of other modules
		//Code below implements predictor/corrector-type logic, even though it effectively does nothing
		return SM_EVENT;

		////Do deltamode-related logic
		//if (bustype==SWING)	//We're the SWING bus, control our destiny (which is really controlled elsewhere)
		//{
		//	//See what we're on
		//	pass_mod = iteration_count_val - ((iteration_count_val >> 1) << 1);

		//	//Check pass
		//	if (pass_mod==0)	//Predictor pass
		//	{
		//		return SM_DELTA_ITER;	//Reiterate - to get us to corrector pass
		//	}
		//	else	//Corrector pass
		//	{
		//		//As of right now, we're always ready to leave
		//		//Other objects will dictate if we stay (powerflow is indifferent)
		//		return SM_EVENT;
		//	}//End corrector pass
		//}//End SWING bus handling
		//else	//Normal bus
		//{
		//	return SM_EVENT;	//Normal nodes want event mode all the time here - SWING bus will
		//						//control the reiteration process for pred/corr steps
		//}
	}
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

EXPORT int notify_triplex_node(OBJECT *obj, int update_mode, PROPERTY *prop, char *value){
	triplex_node *n = OBJECTDATA(obj, triplex_node);
	int rv = 1;

	rv = n->notify(update_mode, prop, value);

	return rv;
}

//Deltamode export
EXPORT SIMULATIONMODE interupdate_triplex_node(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	triplex_node *my = OBJECTDATA(obj,triplex_node);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_triplex_node(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_triplex_node(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

/**@}*/
