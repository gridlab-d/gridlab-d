/** $Id: triplex_node.cpp 1186 2009-01-02 18:15:30Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file triplex_node.cpp
	@addtogroup triplex_node
	@ingroup powerflow_node
	
	@{
*/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "triplex_node.h"

CLASS* triplex_node::oclass = nullptr;
CLASS* triplex_node::pclass = nullptr;

triplex_node::triplex_node(MODULE *mod) : node(mod)
{
	if(oclass == nullptr)
	{
		pclass = node::oclass;
		
		oclass = gl_register_class(mod,"triplex_node",sizeof(triplex_node),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==nullptr)
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

			PT_complex, "prerotated_current_1[A]", PADDR(pre_rotated_current[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable",
			PT_complex, "prerotated_current_2[A]", PADDR(pre_rotated_current[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable",
			PT_complex, "prerotated_current_12[A]", PADDR(pre_rotated_current[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable",

			//This variable isn't used yet, but publishing to get the initial hooks in there
			PT_complex, "deltamode_generator_current_12[A]", PADDR(deltamode_dynamic_current[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), direct generator injection (so may be overwritten internally), this an accumulator only, not a output or input variable",
			PT_complex, "deltamode_PGenTotal",PADDR(deltamode_PGenTotal),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - power value for a diesel generator -- accumulator only, not an output or input",
			PT_complex_array, "deltamode_full_Y_matrix",  PADDR(full_Y_matrix),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality full_Y matrix exposes so generator objects can interact for Norton equivalents",
			PT_complex_array, "deltamode_full_Y_all_matrix",  PADDR(full_Y_all_matrix),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality full_Y_all matrix exposes so generator objects can interact for Norton equivalents",
			PT_object, "NR_powerflow_parent",PADDR(SubNodeParent),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"NR powerflow - actual powerflow parent - used by generators accessing child objects",

			PT_complex, "residential_nominal_current_1[A]", PADDR(nom_res_curr[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 1 from a residential object, if attached",
			PT_complex, "residential_nominal_current_2[A]", PADDR(nom_res_curr[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 2 from a residential object, if attached",
			PT_complex, "residential_nominal_current_12[A]", PADDR(nom_res_curr[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 1 to 2 from a residential object, if attached",
			PT_double, "residential_nominal_current_1_real[A]", PADDR(nom_res_curr[0].Re()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 1, real, from a residential object, if attached",
			PT_double, "residential_nominal_current_1_imag[A]", PADDR(nom_res_curr[0].Im()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 1, imag, from a residential object, if attached",
			PT_double, "residential_nominal_current_2_real[A]", PADDR(nom_res_curr[1].Re()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 2, real, from a residential object, if attached",
			PT_double, "residential_nominal_current_2_imag[A]", PADDR(nom_res_curr[1].Im()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 2, imag, from a residential object, if attached",
			PT_double, "residential_nominal_current_12_real[A]", PADDR(nom_res_curr[2].Re()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 1 to 2, real, from a residential object, if attached",
			PT_double, "residential_nominal_current_12_imag[A]", PADDR(nom_res_curr[2].Im()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase 1 to 2, imag, from a residential object, if attached",

			//************* NOTE - DELETE IN FUTURE - START BLOCK ********************//
			//Items below are being flagged as "deprecated" for 4.3 to encourage users to move to triplex_load object
			//current_X, power_X, and shunt_X should become hidden accumulators after these get removed
			//When fully deprecated, delete the code between the "NOTE - DELETE IN FUTURE" comment blocks, uncomment the "NOTE - INSTATE IN FUTURE" block
			//And update the relevant published property mappings for residential/house_e, generators/inverter, generators/battery, and generators/inverter_dyn
			PT_complex, "current_1[A]", PADDR(pub_current[0]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant current load on phase 1, also acts as accumulator",
			PT_complex, "current_2[A]", PADDR(pub_current[1]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant current load on phase 2, also acts as accumulator",
			PT_complex, "current_N[A]", PADDR(pub_current[2]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant current load on phase N, also acts as accumulator",
			PT_double, "current_1_real[A]", PADDR(pub_current[0].Re()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant current load on phase 1, real",
			PT_double, "current_2_real[A]", PADDR(pub_current[1].Re()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant current load on phase 2, real",
			PT_double, "current_N_real[A]", PADDR(pub_current[2].Re()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant current load on phase N, real",
			PT_double, "current_1_reac[A]", PADDR(pub_current[0].Im()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant current load on phase 1, imag",
			PT_double, "current_2_reac[A]", PADDR(pub_current[1].Im()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant current load on phase 2, imag",
			PT_double, "current_N_reac[A]", PADDR(pub_current[2].Im()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant current load on phase N, imag",
			PT_complex, "current_12[A]", PADDR(pub_current[3]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant current load on phase 1 to 2",
			PT_double, "current_12_real[A]", PADDR(pub_current[3].Re()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant current load on phase 1 to 2, real",
			PT_double, "current_12_reac[A]", PADDR(pub_current[3].Im()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant current load on phase 1 to 2, imag",
			PT_complex, "power_1[VA]", PADDR(pub_power[0]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant power on phase 1 (120V)",
			PT_complex, "power_2[VA]", PADDR(pub_power[1]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant power on phase 2 (120V)",
			PT_complex, "power_12[VA]", PADDR(pub_power[2]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant power on phase 1 to 2 (240V)",
			PT_double, "power_1_real[W]", PADDR(pub_power[0].Re()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant power on phase 1, real",
			PT_double, "power_2_real[W]", PADDR(pub_power[1].Re()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant power on phase 2, real",
			PT_double, "power_12_real[W]", PADDR(pub_power[2].Re()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant power on phase 1 to 2, real",
			PT_double, "power_1_reac[VAr]", PADDR(pub_power[0].Im()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant power on phase 1, imag",
			PT_double, "power_2_reac[VAr]", PADDR(pub_power[1].Im()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant power on phase 2, imag",
			PT_double, "power_12_reac[VAr]", PADDR(pub_power[2].Im()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant power on phase 1 to 2, imag",
			PT_complex, "shunt_1[S]", PADDR(pub_shunt[0]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant shunt impedance on phase 1",
			PT_complex, "shunt_2[S]", PADDR(pub_shunt[1]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant shunt impedance on phase 2",
			PT_complex, "shunt_12[S]", PADDR(pub_shunt[2]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant shunt impedance on phase 1 to 2",
			PT_complex, "impedance_1[Ohm]", PADDR(impedance[0]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant series impedance on phase 1",
			PT_complex, "impedance_2[Ohm]", PADDR(impedance[1]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant series impedance on phase 2",
			PT_complex, "impedance_12[Ohm]", PADDR(impedance[2]),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant series impedance on phase 1 to 2",
			PT_double, "impedance_1_real[Ohm]", PADDR(impedance[0].Re()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant series impedance on phase 1, real",
			PT_double, "impedance_2_real[Ohm]", PADDR(impedance[1].Re()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant series impedance on phase 2, real",
			PT_double, "impedance_12_real[Ohm]", PADDR(impedance[2].Re()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant series impedance on phase 1 to 2, real",
			PT_double, "impedance_1_reac[Ohm]", PADDR(impedance[0].Im()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant series impedance on phase 1, imag",
			PT_double, "impedance_2_reac[Ohm]", PADDR(impedance[1].Im()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant series impedance on phase 2, imag",
			PT_double, "impedance_12_reac[Ohm]", PADDR(impedance[2].Im()),PT_DEPRECATED,PT_DESCRIPTION,"deprecated - use triplex_load - constant series impedance on phase 1 to 2, imag",

			//Direct-access properties
			PT_complex, "acc_temp_current_1[A]", PADDR(current[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - current load on phase 1",
			PT_complex, "acc_temp_current_2[A]", PADDR(current[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - current load on phase 2",
			PT_complex, "acc_temp_current_N[A]", PADDR(current[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - current load on phase N",
			PT_complex, "acc_temp_current_12[A]", PADDR(current12),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - current load on phase 1 to 2",
			PT_complex, "acc_temp_power_1[VA]", PADDR(power[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - power on phase 1 (120V)",
			PT_complex, "acc_temp_power_2[VA]", PADDR(power[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - power on phase 2 (120V)",
			PT_complex, "acc_temp_power_12[VA]", PADDR(power[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - power on phase 1 to 2 (240V)",
			PT_complex, "acc_temp_shunt_1[S]", PADDR(shunt[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - shunt impedance on phase 1",
			PT_complex, "acc_temp_shunt_2[S]", PADDR(shunt[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - shunt impedance on phase 2",
			PT_complex, "acc_temp_shunt_12[S]", PADDR(shunt[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - shunt impedance on phase 1 to 2",
			
			//************* NOTE - DELETE IN FUTURE - END BLOCK ********************//

			//************* NOTE - INSTATE IN FUTURE - START BLOCK ********************//
			//Reminder - fix published property map for residential/house_e, generators/inverter, generators/battery, and generators/inverter_dyn

			// PT_complex, "current_1[A]", PADDR(current[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - current load on phase 1",
			// PT_complex, "current_2[A]", PADDR(current[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - current load on phase 2",
			// PT_complex, "current_N[A]", PADDR(current[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - current load on phase N",
			// PT_complex, "current_12[A]", PADDR(current12),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - current load on phase 1 to 2",
			// PT_complex, "power_1[VA]", PADDR(power[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - power on phase 1 (120V)",
			// PT_complex, "power_2[VA]", PADDR(power[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - power on phase 2 (120V)",
			// PT_complex, "power_12[VA]", PADDR(power[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - power on phase 1 to 2 (240V)",
			// PT_complex, "shunt_1[S]", PADDR(shunt[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - shunt impedance on phase 1",
			// PT_complex, "shunt_2[S]", PADDR(shunt[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - shunt impedance on phase 2",
			// PT_complex, "shunt_12[S]", PADDR(shunt[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"accumulator - used for debugging/interfacing only - shunt impedance on phase 1 to 2",

			//************* NOTE - INSTATE IN FUTURE - END BLOCK ********************//
			
			PT_bool, "house_present", PADDR(house_present),PT_DESCRIPTION,"boolean for detecting whether a house is attached, not an input",

			//Deltamode
			PT_bool, "Norton_dynamic", PADDR(dynamic_norton),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Flag to indicate a Norton-equivalent connection -- used for generators and deltamode",
			PT_bool, "Norton_dynamic_child", PADDR(dynamic_norton_child),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Flag to indicate a Norton-equivalent connection is made by a childed node object -- used for generators and deltamode",
			PT_bool, "generator_dynamic", PADDR(dynamic_generator),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Flag to indicate a voltage-sourcing or swing-type generator is present -- used for generators and deltamode",

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

			PT_enumeration, "GFA_trip_method", PADDR(GFA_trip_method), PT_DESCRIPTION, "Reason for GFA trip - what caused the GFA to activate",
				PT_KEYWORD, "NONE", (enumeration)GFA_NONE, PT_DESCRIPTION, "No GFA trip",
				PT_KEYWORD, "UNDER_FREQUENCY", (enumeration)GFA_UF, PT_DESCRIPTION, "GFA trip for under-frequency",
				PT_KEYWORD, "OVER_FREQUENCY", (enumeration)GFA_OF, PT_DESCRIPTION, "GFA trip for over-frequency",
				PT_KEYWORD, "UNDER_VOLTAGE", (enumeration)GFA_UV, PT_DESCRIPTION, "GFA trip for under-voltage",
				PT_KEYWORD, "OVER_VOLTAGE", (enumeration)GFA_OV, PT_DESCRIPTION, "GFA trip for over-voltage",

			PT_enumeration, "service_status", PADDR(service_status),PT_DESCRIPTION,"In and out of service flag",
				PT_KEYWORD, "IN_SERVICE", (enumeration)ND_IN_SERVICE,
				PT_KEYWORD, "OUT_OF_SERVICE", (enumeration)ND_OUT_OF_SERVICE,
			PT_double, "service_status_double", PADDR(service_status_dbl),PT_DESCRIPTION,"In and out of service flag - type double - will indiscriminately override service_status - useful for schedules",
			PT_double, "previous_uptime[min]", PADDR(previous_uptime),PT_DESCRIPTION,"Previous time between disconnects of node in minutes",
			PT_double, "current_uptime[min]", PADDR(current_uptime),PT_DESCRIPTION,"Current time since last disconnect of node in minutes",
			PT_object, "topological_parent", PADDR(TopologicalParent),PT_DESCRIPTION,"topological parent as per GLM configuration",

			PT_bool, "behaving_as_swing", PADDR(swing_functions_enabled), PT_DESCRIPTION, "Indicator flag for if a bus is behaving as a reference voltage source - valid for a SWING or SWING_PQ",

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
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_triplex_node)==nullptr)
			GL_THROW("Unable to publish triplex_node deltamode function");
		if (gl_publish_function(oclass,	"pwr_object_swing_swapper", (FUNCTIONADDR)swap_node_swing_status)==nullptr)
			GL_THROW("Unable to publish triplex_node swing-swapping function");
		if (gl_publish_function(oclass,	"pwr_current_injection_update_map", (FUNCTIONADDR)node_map_current_update_function)==nullptr)
			GL_THROW("Unable to publish triplex_node current injection update mapping function");
		if (gl_publish_function(oclass,	"attach_vfd_to_pwr_object", (FUNCTIONADDR)attach_vfd_to_node)==nullptr)
			GL_THROW("Unable to publish triplex_node VFD attachment function");
		if (gl_publish_function(oclass, "pwr_object_reset_disabled_status", (FUNCTIONADDR)node_reset_disabled_status) == nullptr)
			GL_THROW("Unable to publish triplex_node island-status-reset function");
		if (gl_publish_function(oclass, "pwr_object_shunt_update", (FUNCTIONADDR)node_update_shunt_values) == nullptr)
			GL_THROW("Unable to publish triplex_node shunt update function");
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

	//**************** NOTE - Code that can probably be deleted when deprecated node properties removed ******************//
	pub_shunt[0] = pub_shunt[1] = pub_shunt[2] = gld::complex(0.0,0.0);
	impedance[0] = impedance[1] = impedance[2] = gld::complex(0.0,0.0);
	pub_current[0] = pub_current[1] = pub_current[2] = pub_current[3] = gld::complex(0.0,0.0);
	pub_power[0] = pub_power[1] = pub_power[2] = gld::complex(0.0,0.0);

	//Accumulators
	prev_node_load_values[0][0] = prev_node_load_values[0][1] = prev_node_load_values[0][2] = gld::complex(0.0,0.0);
	prev_node_load_values[1][0] = prev_node_load_values[1][1] = prev_node_load_values[1][2] = gld::complex(0.0,0.0);
	prev_node_load_current_values[0] = prev_node_load_current_values[1] = prev_node_load_current_values[2] = prev_node_load_current_values[3] = gld::complex(0.0,0.0);

	//********************* END Deprecated delete section **************************//

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

	return node::init(parent);
}

TIMESTAMP triplex_node::presync(TIMESTAMP t0)
{
	return node::presync(t0);
}

//Functionalized sync routine
//For external calls
void triplex_node::BOTH_triplex_node_sync_fxn(void)
{
	//**************** NOTE - Code that can probably be deleted when deprecated node properties removed ******************//
	//Replicates triplex_load behavior for "published" load properties
	int index_var;

	//Remove prior contributions and zero the accumulators
	shunt[0] -= prev_node_load_values[0][0];
	shunt[1] -= prev_node_load_values[0][1];
	shunt[2] -= prev_node_load_values[0][2];

	power[0] -= prev_node_load_values[1][0];
	power[1] -= prev_node_load_values[1][1];
	power[2] -= prev_node_load_values[1][2];

	current[0] -= prev_node_load_current_values[0];
	current[1] -= prev_node_load_current_values[1];
	current[2] -= prev_node_load_current_values[2];
	current12 -= prev_node_load_current_values[3];

	//Zero the accumulators
	for (index_var=0; index_var<3; index_var++)
	{
		prev_node_load_values[0][index_var] = gld::complex(0.0,0.0);
		prev_node_load_values[1][index_var] = gld::complex(0.0,0.0);
	}

	//Do the current (bigger, so different index)
	prev_node_load_current_values[0] = gld::complex(0.0,0.0);
	prev_node_load_current_values[1] = gld::complex(0.0,0.0);
	prev_node_load_current_values[2] = gld::complex(0.0,0.0);
	prev_node_load_current_values[3] = gld::complex(0.0,0.0);

	//See how to accumulate the various values
	//Prioritizes shunt over impedance
	if ((pub_shunt[0].IsZero()) && (!impedance[0].IsZero()))	//Impedance specified
	{
		//Accumulator
		shunt[0] += gld::complex(1.0,0.0)/impedance[0];

		//Tracker
		prev_node_load_values[0][0] += gld::complex(1.0,0.0)/impedance[0];
	}
	else											//Shunt specified (impedance ignored)
	{
		//Accumulator
		shunt[0] += pub_shunt[0];

		//Tracker
		prev_node_load_values[0][0] += pub_shunt[0];
	}

	if ((pub_shunt[1].IsZero()) && (!impedance[1].IsZero()))	//Impedance specified
	{
		//Accumulator
		shunt[1] += gld::complex(1.0,0.0)/impedance[1];

		//Tracker
		prev_node_load_values[0][1] += gld::complex(1.0,0.0)/impedance[1];
	}
	else											//Shunt specified (impedance ignored)
	{
		//Accumulator
		shunt[1] += pub_shunt[1];

		//Tracker
		prev_node_load_values[0][1] += pub_shunt[1];
	}

	if ((pub_shunt[2].IsZero()) && (!impedance[2].IsZero()))	//Impedance specified
	{
		//Accumulator
		shunt[2] += gld::complex(1.0,0.0)/impedance[2];

		//Tracker
		prev_node_load_values[0][2] += gld::complex(1.0,0.0)/impedance[2];
	}
	else											//Shunt specified (impedance ignored)
	{
		//Accumulator
		shunt[2] += pub_shunt[2];

		//Tracker
		prev_node_load_values[0][2] += pub_shunt[2];
	}
	
	//Power and current accumulators
	power[0] += pub_power[0];
	power[1] += pub_power[1];	
	power[2] += pub_power[2];

	current[0] += pub_current[0];
	current[1] += pub_current[1];
	current[2] += pub_current[2];
	current12 += pub_current[3];

	//Power and current trackers
	prev_node_load_current_values[0] += pub_current[0];
	prev_node_load_current_values[1] += pub_current[1];
	prev_node_load_current_values[2] += pub_current[2];
	prev_node_load_current_values[3] += pub_current[3];
	prev_node_load_values[1][0] += pub_power[0];
	prev_node_load_values[1][1] += pub_power[1];
	prev_node_load_values[1][2] += pub_power[2];

	//************** End Depecrated delete - BOTH_triplex_node_sync_fxn may not even be needed after deprecated code removed ***********//
}

TIMESTAMP triplex_node::sync(TIMESTAMP t0)
{
	//Call the functionalized version
	BOTH_triplex_node_sync_fxn();

	return node::sync(t0);
}

TIMESTAMP triplex_node::postsync(TIMESTAMP t0)
{
	//Just call the node one - explicit definition, just so easier to add things in the future
	return node::postsync(t0);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Module-level call
SIMULATIONMODE triplex_node::inter_deltaupdate_triplex_node(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	OBJECT *hdr = OBJECTHDR(this);
	double deltat;
	STATUS return_status_val;

	//Create delta_t variable
	deltat = (double)dt/(double)DT_SECOND;

	//Update time tracking variable - mostly for GFA functionality calls
	if ((iteration_count_val==0) && !interupdate_pos) //Only update timestamp tracker on first iteration
	{
		//Update tracking variable
		prev_time_dbl = gl_globaldeltaclock;

		//Update frequency calculation values (if needed)
		if (fmeas_type != FM_NONE)
		{
			//See which pass
			if (delta_time == 0)
			{
				//Initialize dynamics - first run of new delta call
				init_freq_dynamics(deltat);
			}
			else
			{
				//Copy the tracker value
				memcpy(&prev_freq_state,&curr_freq_state,sizeof(FREQM_STATES));
			}
		}
	}

	//Perform the GFA update, if enabled
	if (GFA_enable && (iteration_count_val == 0) && !interupdate_pos)	//Always just do on the first pass
	{
		//Do the checks
		GFA_Update_time = perform_GFA_checks(deltat);
	}

	if (!interupdate_pos)	//Before powerflow call
	{
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
		if (GFA_enable)
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
		if (*obj!=nullptr)
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
