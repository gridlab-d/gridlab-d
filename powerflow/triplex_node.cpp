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

#include "triplex_node.h"

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
				PT_KEYWORD, "SWING_PQ", (enumeration)SWING_PQ,
			PT_set, "busflags", PADDR(busflags),PT_DESCRIPTION,"flag indicates node has a source for voltage, i.e. connects to the swing node",
				PT_KEYWORD, "HASSOURCE", (set)NF_HASSOURCE,
				PT_KEYWORD, "ISSOURCE", (set)NF_ISSOURCE,
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

			PT_complex, "prerotated_current_1[A]", PADDR(pre_rotated_current[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable",
			PT_complex, "prerotated_current_2[A]", PADDR(pre_rotated_current[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable",
			PT_complex, "prerotated_current_12[A]", PADDR(pre_rotated_current[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable",

			//This variable isn't used yet, but publishing to get the initial hooks in there
			PT_complex, "deltamode_generator_current_12[A]", PADDR(deltamode_dynamic_current[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), direct generator injection (so may be overwritten internally), this an accumulator only, not a output or input variable",

			PT_complex, "residential_nominal_current_1[A]", PADDR(nom_res_curr[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 1 from a residential object, if attached",
			PT_complex, "residential_nominal_current_2[A]", PADDR(nom_res_curr[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 2 from a residential object, if attached",
			PT_complex, "residential_nominal_current_12[A]", PADDR(nom_res_curr[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 1 to 2 from a residential object, if attached",
			PT_double, "residential_nominal_current_1_real[A]", PADDR(nom_res_curr[0].Re()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 1, real, from a residential object, if attached",
			PT_double, "residential_nominal_current_1_imag[A]", PADDR(nom_res_curr[0].Im()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 1, imag, from a residential object, if attached",
			PT_double, "residential_nominal_current_2_real[A]", PADDR(nom_res_curr[1].Re()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 2, real, from a residential object, if attached",
			PT_double, "residential_nominal_current_2_imag[A]", PADDR(nom_res_curr[1].Im()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 2, imag, from a residential object, if attached",
			PT_double, "residential_nominal_current_12_real[A]", PADDR(nom_res_curr[2].Re()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 1 to 2, real, from a residential object, if attached",
			PT_double, "residential_nominal_current_12_imag[A]", PADDR(nom_res_curr[2].Im()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 1 to 2, imag, from a residential object, if attached",
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

			PT_bool, "reset_disabled_island_state", PADDR(reset_island_state), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Deltamode/multi-island flag -- used to reset disabled status (and reform an island)",

			//GFA - stuff
			PT_bool, "GFA_enable", PADDR(GFA_enable), PT_DESCRIPTION, "Disable/Enable Grid Friendly Applicance(TM)-type functionality",
			PT_double, "GFA_freq_low_trip[Hz]", PADDR(GFA_freq_low_trip), PT_DESCRIPTION, "Low frequency trip point for Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_freq_high_trip[Hz]", PADDR(GFA_freq_high_trip), PT_DESCRIPTION, "High frequency trip point for Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_volt_low_trip[pu]", PADDR(GFA_voltage_low_trip), PT_DESCRIPTION, "Low voltage trip point for Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_volt_high_trip[pu]", PADDR(GFA_voltage_high_trip), PT_DESCRIPTION, "High voltage trip point for Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_reconnect_time[s]", PADDR(GFA_reconnect_time), PT_DESCRIPTION, "Reconnect time for Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_freq_disconnect_time[s]", PADDR(GFA_freq_disconnect_time), PT_DESCRIPTION, "Frequency violation disconnect time for Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_volt_disconnect_time[s]", PADDR(GFA_volt_disconnect_time), PT_DESCRIPTION, "Voltage violation disconnect time for Grid Friendly Appliance(TM)-type functionality",
			PT_bool, "GFA_status", PADDR(GFA_status), PT_DESCRIPTION, "Low frequency trip point for Grid Friendly Appliance(TM)-type functionality",

			PT_enumeration, "service_status", PADDR(service_status),PT_DESCRIPTION,"In and out of service flag",
				PT_KEYWORD, "IN_SERVICE", (enumeration)ND_IN_SERVICE,
				PT_KEYWORD, "OUT_OF_SERVICE", (enumeration)ND_OUT_OF_SERVICE,
			PT_double, "service_status_double", PADDR(service_status_dbl),PT_DESCRIPTION,"In and out of service flag - type double - will indiscriminately override service_status - useful for schedules",
			PT_double, "previous_uptime[min]", PADDR(previous_uptime),PT_DESCRIPTION,"Previous time between disconnects of node in minutes",
			PT_double, "current_uptime[min]", PADDR(current_uptime),PT_DESCRIPTION,"Current time since last disconnect of node in minutes",
			PT_object, "topological_parent", PADDR(TopologicalParent),PT_DESCRIPTION,"topological parent as per GLM configuration",

			//Properties for frequency measurement
			PT_enumeration,"frequency_measure_type",PADDR(fmeas_type),PT_DESCRIPTION,"Frequency measurement dynamics-capable implementation",
				PT_KEYWORD,"NONE",(enumeration)FM_NONE,PT_DESCRIPTION,"No frequency measurement",
				PT_KEYWORD,"SIMPLE",(enumeration)FM_SIMPLE,PT_DESCRIPTION,"Simplified frequency measurement",
				PT_KEYWORD,"PLL",(enumeration)FM_PLL,PT_DESCRIPTION,"PLL frequency measurement",

			PT_double,"sfm_Tf[s]",PADDR(freq_sfm_Tf),PT_DESCRIPTION,"Transducer time constant for simplified frequency measurement (seconds)",
			PT_double,"pll_Kp[pu]",PADDR(freq_pll_Kp),PT_DESCRIPTION,"Proportional gain of PLL frequency measurement",
			PT_double,"pll_Ki[pu]",PADDR(freq_pll_Ki),PT_DESCRIPTION,"Integration gain of PLL frequency measurement",

			//Frequency measurement output variables
			PT_double,"measured_angle_1[rad]", PADDR(curr_freq_state.anglemeas[0]),PT_DESCRIPTION,"bus angle measurement, phase 1N",
			PT_double,"measured_frequency_1[Hz]", PADDR(curr_freq_state.fmeas[0]),PT_DESCRIPTION,"frequency measurement, phase 1N",
			PT_double,"measured_angle_2[rad]", PADDR(curr_freq_state.anglemeas[1]),PT_DESCRIPTION,"bus angle measurement, phase 2N",
			PT_double,"measured_frequency_2[Hz]", PADDR(curr_freq_state.fmeas[1]),PT_DESCRIPTION,"frequency measurement, phase 2N",
			PT_double,"measured_angle_12[rad]", PADDR(curr_freq_state.anglemeas[2]),PT_DESCRIPTION,"bus angle measurement, across the phases",
			PT_double,"measured_frequency_12[Hz]", PADDR(curr_freq_state.fmeas[2]),PT_DESCRIPTION,"frequency measurement, across the phases",
			PT_double,"measured_frequency[Hz]", PADDR(curr_freq_state.average_freq), PT_DESCRIPTION, "frequency measurement - average of present phases",

         	NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_triplex_node)==NULL)
			GL_THROW("Unable to publish triplex_node deltamode function");
		if (gl_publish_function(oclass,	"pwr_object_swing_swapper", (FUNCTIONADDR)swap_node_swing_status)==NULL)
			GL_THROW("Unable to publish triplex_node swing-swapping function");
		if (gl_publish_function(oclass,	"pwr_current_injection_update_map", (FUNCTIONADDR)node_map_current_update_function)==NULL)
			GL_THROW("Unable to publish triplex_node current injection update mapping function");
		if (gl_publish_function(oclass,	"attach_vfd_to_pwr_object", (FUNCTIONADDR)attach_vfd_to_node)==NULL)
			GL_THROW("Unable to publish triplex_node VFD attachment function");
		if (gl_publish_function(oclass, "pwr_object_reset_disabled_status", (FUNCTIONADDR)node_reset_disabled_status) == NULL)
			GL_THROW("Unable to publish triplex_node island-status-reset function");
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
	OBJECT *hdr = OBJECTHDR(this);
	double deltat, deltatimedbl;
	STATUS return_status_val;

	//Create delta_t variable
	deltat = (double)dt/(double)DT_SECOND;

	//Update time tracking variable - mostly for GFA functionality calls
	if ((iteration_count_val==0) && (interupdate_pos == false)) //Only update timestamp tracker on first iteration
	{
		//Get decimal timestamp value
		deltatimedbl = (double)delta_time/(double)DT_SECOND; 

		//Update tracking variable
		prev_time_dbl = (double)gl_globalclock + deltatimedbl;

		//Update frequency calculation values (if needed)
		if (fmeas_type != FM_NONE)
		{
			//Copy the tracker value
			memcpy(&prev_freq_state,&curr_freq_state,sizeof(FREQM_STATES));
		}
	}

	//Initialization items
	if ((delta_time==0) && (iteration_count_val==0) && (interupdate_pos == false) && (fmeas_type != FM_NONE))	//First run of new delta call
	{
		//Initialize dynamics
		init_freq_dynamics();
	}//End first pass and timestep of deltamode (initial condition stuff)

	//Perform the GFA update, if enabled
	if ((GFA_enable == true) && (iteration_count_val == 0) && (interupdate_pos == false))	//Always just do on the first pass
	{
		//Do the checks
		GFA_Update_time = perform_GFA_checks(deltat);
	}

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Call triplex-specific call
		BOTH_triplex_node_presync_fxn();

		//Call node presync-equivalent items
		NR_node_presync_fxn(0);

		//Call sync-equivalent of triplex portion first
		BOTH_triplex_node_sync_fxn();

		//Call node sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		return SM_DELTA;	//Just return something other than SM_ERROR for this call

	}//End Before NR solver (or inclusive)
	else	//After the call
	{
		//No triplex-specific postsync for node

		//Perform node postsync-like updates on the values
		BOTH_node_postsync_fxn(hdr);

		//Frequency measurement stuff
		if (fmeas_type != FM_NONE)
		{
			return_status_val = calc_freq_dynamics(deltat);

			//Check it
			if (return_status_val == FAILED)
			{
				return SM_ERROR;
			}
		}//End frequency measurement desired
		//Default else -- don't calculate it

		//See if GFA functionality is required, since it may require iterations or "continance"
		if (GFA_enable == true)
		{
			//See if our return is value
			if ((GFA_Update_time > 0.0) && (GFA_Update_time < 1.7))
			{
				//Force us to stay
				return SM_DELTA;
			}
			else	//Just return whatever we were going to do
			{
				return SM_EVENT;
			}
		}
		else	//Normal mode
		{
			return SM_EVENT;
		}
	}//End "After NR solver" branch
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
