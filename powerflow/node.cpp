/** $Id: node.cpp 1215 2009-01-22 00:54:37Z d3x593 $
	Copyright (C) 2008 Battelle Memorial Institute
	@file node.cpp
	@addtogroup powerflow_node Node
	@ingroup powerflow_object
	
	The node is one of the major components of the method used for solving a 
	powerflow network.  In essense the distribution network can be seen as a 
	series of nodes and links.  Nodes primary responsibility is to act as an
	aggregation point for the links that are attached to it, and to hold the
	current and voltage values that will be used in the matrix calculations
	done in the link.
	
	Three types of nodes are defined in this file.  Nodes are simply a basic 
	object that exports the voltages for each phase.  Triplex nodes export
	voltages for 3 lines; line1_voltage, line2_voltage, lineN_voltage.

	@par Voltage control

	When the global variable require_voltage_control is set to \p true,
	the bus type is used to determine how voltage control is implemented.
	Voltage control is only performed when the bus has no link that
	considers it a to node.  When the flag \#NF_HASSOURCE is cleared, then
	the following is in effect:

	- \#SWING buses are considered infinite sources and the voltage will
	  remain fixed regardless of conditions.
	- \#PV buses are considered constrained sources and the voltage will
	  fall to zero if there is insufficient voltage support at the bus.
	  A generator object is required to provide the voltage support explicitly.
	  The voltage will be set to zero if the generator does not provide
	  sufficient support.
	- \#PQ buses will always go to zero voltage.

	@par Fault support

	The following conditions are used to describe a fault impedance \e X (e.g., 1e-6), 
	between phase \e x and neutral or group, or between phases \e x and \e y, and leaving
	phase \e z unaffected at iteration \e t:
	- \e phase-to-phase contact at the node
		- \e Forward-sweep: \f$ V_x(t) = V_y(t) = \frac{V_x(t-1) + V_y(t-1)}{2} \f$
		- \e Back-sweep: \f$ I_x(t+1) = -I_y(t+1) = \frac{V_x(t)-V_y(t)}{X} \f$;
    - \e phase-to-ground/neutral contact at the node
		- \e Forward-sweep: \f$ V_x(t) = \frac{1}{2} V_x(t-1)  \f$
		- \e Back-sweep: \f$ I_x(t+1) = \frac{V_x(t)}{X} \f$;
	
	@{
*/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "solver_nr.h"
#include "node.h"

//Library imports items - for external LU solver - stolen from somewhere else in GridLAB-D (tape, I believe)
#if defined(_WIN32) && !defined(__MINGW32__)
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0400
#include <windows.h>
#ifndef DLEXT
#define DLEXT ".dll"
#endif
#define DLLOAD(P) LoadLibrary(P)
#define DLSYM(H,S) (void *)GetProcAddress((HINSTANCE)H,S)
#define snprintf _snprintf
#else /* ANSI */
#include "dlfcn.h"
#ifndef DLEXT
#ifdef __MINGW32__
#define DLEXT ".dll"
#else
#define DLEXT ".so"
#endif
#endif
#define DLLOAD(P) dlopen(P,RTLD_LAZY)
#define DLSYM(H,S) dlsym(H,S)
#endif

//"Small" multiplier for restoring voltages in in-rush.  Zeros seem to make it angry
//TODO: See if this is a "zero-catch" somewhere making it useless, or legitimate numerical stability
#define MULTTERM 0.00000000000001

//******************* TODO SOON -- Check history and saturation mapping for children (probably doesn't work **********//

CLASS *node::oclass = nullptr;
CLASS *node::pclass = nullptr;

unsigned int node::n = 0; 

node::node(MODULE *mod) : powerflow_object(mod)
{
	if(oclass == nullptr)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod,"node",sizeof(node),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw std::runtime_error("unable to register class node");
		else
			oclass->trl = TRL_PROVEN;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "powerflow_object",
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

			PT_complex, "voltage_A[V]", PADDR(voltage[0]),PT_DESCRIPTION,"bus voltage, Phase A to ground",
			PT_complex, "voltage_B[V]", PADDR(voltage[1]),PT_DESCRIPTION,"bus voltage, Phase B to ground",
			PT_complex, "voltage_C[V]", PADDR(voltage[2]),PT_DESCRIPTION,"bus voltage, Phase C to ground",
			PT_complex, "voltage_AB[V]", PADDR(voltaged[0]),PT_DESCRIPTION,"line voltages, Phase AB",
			PT_complex, "voltage_BC[V]", PADDR(voltaged[1]),PT_DESCRIPTION,"line voltages, Phase BC",
			PT_complex, "voltage_CA[V]", PADDR(voltaged[2]),PT_DESCRIPTION,"line voltages, Phase CA",
			PT_complex, "current_A[A]", PADDR(current[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus current injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_B[A]", PADDR(current[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus current injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_C[A]", PADDR(current[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus current injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_A[VA]", PADDR(power[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus power injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_B[VA]", PADDR(power[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus power injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_C[VA]", PADDR(power[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus power injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "shunt_A[S]", PADDR(shunt[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus shunt admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_B[S]", PADDR(shunt[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus shunt admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_C[S]", PADDR(shunt[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus shunt admittance, this an accumulator only, not a output or input variable",

			PT_complex, "prerotated_current_A[A]", PADDR(pre_rotated_current[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable",
			PT_complex, "prerotated_current_B[A]", PADDR(pre_rotated_current[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable",
			PT_complex, "prerotated_current_C[A]", PADDR(pre_rotated_current[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable",

			PT_complex, "deltamode_generator_current_A[A]", PADDR(deltamode_dynamic_current[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), direct generator injection (so may be overwritten internally), this an accumulator only, not a output or input variable",
			PT_complex, "deltamode_generator_current_B[A]", PADDR(deltamode_dynamic_current[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), direct generator injection (so may be overwritten internally), this an accumulator only, not a output or input variable",
			PT_complex, "deltamode_generator_current_C[A]", PADDR(deltamode_dynamic_current[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - bus current injection (in = positive), direct generator injection (so may be overwritten internally), this an accumulator only, not a output or input variable",

			PT_complex, "deltamode_PGenTotal",PADDR(deltamode_PGenTotal),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality - power value for a diesel generator -- accumulator only, not an output or input",

			PT_complex_array, "deltamode_full_Y_matrix",  PADDR(full_Y_matrix),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality full_Y matrix exposes so generator objects can interact for Norton equivalents",
			PT_complex_array, "deltamode_full_Y_all_matrix",  PADDR(full_Y_all_matrix),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"deltamode-functionality full_Y_all matrix exposes so generator objects can interact for Norton equivalents",
			PT_object, "NR_powerflow_parent",PADDR(SubNodeParent),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"NR powerflow - actual powerflow parent - used by generators accessing child objects",

			PT_complex, "current_inj_A[A]", PADDR(current_inj[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable",
			PT_complex, "current_inj_B[A]", PADDR(current_inj[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable",
			PT_complex, "current_inj_C[A]", PADDR(current_inj[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus current injection (in = positive), but will not be rotated by powerflow for off-nominal frequency, this an accumulator only, not a output or input variable",

			PT_complex, "current_AB[A]", PADDR(current_dy[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus current delta-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_BC[A]", PADDR(current_dy[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus current delta-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_CA[A]", PADDR(current_dy[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus current delta-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_AN[A]", PADDR(current_dy[3]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus current wye-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_BN[A]", PADDR(current_dy[4]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus current wye-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_CN[A]", PADDR(current_dy[5]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus current wye-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_AB[VA]", PADDR(power_dy[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus power delta-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_BC[VA]", PADDR(power_dy[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus power delta-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_CA[VA]", PADDR(power_dy[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus power delta-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_AN[VA]", PADDR(power_dy[3]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus power wye-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_BN[VA]", PADDR(power_dy[4]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus power wye-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_CN[VA]", PADDR(power_dy[5]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus power wye-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "shunt_AB[S]", PADDR(power_dy[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus shunt delta-connected admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_BC[S]", PADDR(power_dy[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus shunt delta-connected admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_CA[S]", PADDR(power_dy[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus shunt delta-connected admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_AN[S]", PADDR(power_dy[3]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus shunt wye-connected admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_BN[S]", PADDR(power_dy[4]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus shunt wye-connected admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_CN[S]", PADDR(power_dy[5]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"bus shunt wye-connected admittance, this an accumulator only, not a output or input variable",

			//House-related variables - for 3-phase house connections
			PT_complex, "residential_nominal_current_A[A]", PADDR(nom_res_curr[0]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase A from a residential object, if attached",
			PT_complex, "residential_nominal_current_B[A]", PADDR(nom_res_curr[1]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase B from a residential object, if attached",
			PT_complex, "residential_nominal_current_C[A]", PADDR(nom_res_curr[2]),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase C from a residential object, if attached",
			PT_double, "residential_nominal_current_A_real[A]", PADDR(nom_res_curr[0].Re()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase A, real, from a residential object, if attached",
			PT_double, "residential_nominal_current_A_imag[A]", PADDR(nom_res_curr[0].Im()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase A, imag, from a residential object, if attached",
			PT_double, "residential_nominal_current_B_real[A]", PADDR(nom_res_curr[1].Re()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase B, real, from a residential object, if attached",
			PT_double, "residential_nominal_current_B_imag[A]", PADDR(nom_res_curr[1].Im()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase B, imag, from a residential object, if attached",
			PT_double, "residential_nominal_current_C_real[A]", PADDR(nom_res_curr[2].Re()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase C, real, from a residential object, if attached",
			PT_double, "residential_nominal_current_C_imag[A]", PADDR(nom_res_curr[2].Im()),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"posted current on phase C, imag, from a residential object, if attached",

			PT_bool, "house_present", PADDR(house_present),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"boolean for detecting whether a house is attached, not an input",

			PT_double, "mean_repair_time[s]",PADDR(mean_repair_time), PT_DESCRIPTION, "Time after a fault clears for the object to be back in service",

			//Properties for frequency measurement
			PT_enumeration,"frequency_measure_type",PADDR(fmeas_type),PT_DESCRIPTION,"Frequency measurement dynamics-capable implementation",
				PT_KEYWORD,"NONE",(enumeration)FM_NONE,PT_DESCRIPTION,"No frequency measurement",
				PT_KEYWORD,"SIMPLE",(enumeration)FM_SIMPLE,PT_DESCRIPTION,"Simplified frequency measurement",
				PT_KEYWORD,"PLL",(enumeration)FM_PLL,PT_DESCRIPTION,"PLL frequency measurement",

			PT_double,"sfm_Tf[s]",PADDR(freq_sfm_Tf),PT_DESCRIPTION,"Transducer time constant for simplified frequency measurement (seconds)",
			PT_double,"pll_Kp[pu]",PADDR(freq_pll_Kp),PT_DESCRIPTION,"Proportional gain of PLL frequency measurement",
			PT_double,"pll_Ki[pu]",PADDR(freq_pll_Ki),PT_DESCRIPTION,"Integration gain of PLL frequency measurement",

			//Frequency measurement output variables
			PT_double,"measured_angle_A[rad]", PADDR(curr_freq_state.anglemeas[0]),PT_DESCRIPTION,"bus angle measurement, phase A",
			PT_double,"measured_frequency_A[Hz]", PADDR(curr_freq_state.fmeas[0]),PT_DESCRIPTION,"frequency measurement, phase A",
			PT_double,"measured_angle_B[rad]", PADDR(curr_freq_state.anglemeas[1]),PT_DESCRIPTION,"bus angle measurement, phase B",
			PT_double,"measured_frequency_B[Hz]", PADDR(curr_freq_state.fmeas[1]),PT_DESCRIPTION,"frequency measurement, phase B",
			PT_double,"measured_angle_C[rad]", PADDR(curr_freq_state.anglemeas[2]),PT_DESCRIPTION,"bus angle measurement, phase C",
			PT_double,"measured_frequency_C[Hz]", PADDR(curr_freq_state.fmeas[2]),PT_DESCRIPTION,"frequency measurement, phase C",
			PT_double,"measured_frequency[Hz]", PADDR(curr_freq_state.average_freq), PT_DESCRIPTION, "frequency measurement - average of present phases",

			PT_enumeration, "service_status", PADDR(service_status),PT_DESCRIPTION,"In and out of service flag",
				PT_KEYWORD, "IN_SERVICE", (enumeration)ND_IN_SERVICE,
				PT_KEYWORD, "OUT_OF_SERVICE", (enumeration)ND_OUT_OF_SERVICE,
			PT_double, "service_status_double", PADDR(service_status_dbl),PT_DESCRIPTION,"In and out of service flag - type double - will indiscriminately override service_status - useful for schedules",
			PT_double, "previous_uptime[min]", PADDR(previous_uptime),PT_DESCRIPTION,"Previous time between disconnects of node in minutes",
			PT_double, "current_uptime[min]", PADDR(current_uptime),PT_DESCRIPTION,"Current time since last disconnect of node in minutes",
			PT_bool, "Norton_dynamic", PADDR(dynamic_norton),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Flag to indicate a Norton-equivalent connection -- used for generators and deltamode",
			PT_bool, "Norton_dynamic_child", PADDR(dynamic_norton_child),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Flag to indicate a Norton-equivalent connection is made by a childed node object -- used for generators and deltamode",
			PT_bool, "generator_dynamic", PADDR(dynamic_generator),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Flag to indicate a voltage-sourcing or swing-type generator is present -- used for generators and deltamode",

			PT_bool, "reset_disabled_island_state", PADDR(reset_island_state), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Deltamode/multi-island flag -- used to reset disabled status (and reform an island)",

			//GFA - stuff
			PT_bool, "GFA_enable", PADDR(GFA_enable), PT_DESCRIPTION, "Disable/Enable Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_freq_low_trip[Hz]", PADDR(GFA_freq_low_trip), PT_DESCRIPTION, "Low frequency trip point for Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_freq_high_trip[Hz]", PADDR(GFA_freq_high_trip), PT_DESCRIPTION, "High frequency trip point for Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_volt_low_trip[pu]", PADDR(GFA_voltage_low_trip), PT_DESCRIPTION, "Low voltage trip point for Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_volt_high_trip[pu]", PADDR(GFA_voltage_high_trip), PT_DESCRIPTION, "High voltage trip point for Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_reconnect_time[s]", PADDR(GFA_reconnect_time), PT_DESCRIPTION, "Reconnect time for Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_freq_disconnect_time[s]", PADDR(GFA_freq_disconnect_time), PT_DESCRIPTION, "Frequency violation disconnect time for Grid Friendly Appliance(TM)-type functionality",
			PT_double, "GFA_volt_disconnect_time[s]", PADDR(GFA_volt_disconnect_time), PT_DESCRIPTION, "Voltage violation disconnect time for Grid Friendly Appliance(TM)-type functionality",
			PT_bool, "GFA_status", PADDR(GFA_status), PT_DESCRIPTION, "Grid Friendly Appliance(TM)-type functionality - whether it is in service (not tripped) or not",

			PT_enumeration, "GFA_trip_method", PADDR(GFA_trip_method), PT_DESCRIPTION, "Reason for GFA trip - what caused the GFA to activate",
				PT_KEYWORD, "NONE", (enumeration)GFA_NONE, PT_DESCRIPTION, "No GFA trip",
				PT_KEYWORD, "UNDER_FREQUENCY", (enumeration)GFA_UF, PT_DESCRIPTION, "GFA trip for under-frequency",
				PT_KEYWORD, "OVER_FREQUENCY", (enumeration)GFA_OF, PT_DESCRIPTION, "GFA trip for over-frequency",
				PT_KEYWORD, "UNDER_VOLTAGE", (enumeration)GFA_UV, PT_DESCRIPTION, "GFA trip for under-voltage",
				PT_KEYWORD, "OVER_VOLTAGE", (enumeration)GFA_OV, PT_DESCRIPTION, "GFA trip for over-voltage",

			PT_object, "topological_parent", PADDR(TopologicalParent),PT_DESCRIPTION,"topological parent as per GLM configuration",
			PT_bool, "behaving_as_swing", PADDR(swing_functions_enabled), PT_DESCRIPTION, "Indicator flag for if a bus is behaving as a reference voltage source - valid for a SWING or SWING_PQ",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_node)==nullptr)
			GL_THROW("Unable to publish node deltamode function");
		if (gl_publish_function(oclass,	"pwr_object_swing_swapper", (FUNCTIONADDR)swap_node_swing_status)==nullptr)
			GL_THROW("Unable to publish node swing-swapping function");
		if (gl_publish_function(oclass,	"pwr_current_injection_update_map", (FUNCTIONADDR)node_map_current_update_function)==nullptr)
			GL_THROW("Unable to publish node current injection update mapping function");
		if (gl_publish_function(oclass,	"attach_vfd_to_pwr_object", (FUNCTIONADDR)attach_vfd_to_node)==nullptr)
			GL_THROW("Unable to publish node VFD attachment function");
		if (gl_publish_function(oclass, "pwr_object_reset_disabled_status", (FUNCTIONADDR)node_reset_disabled_status) == nullptr)
			GL_THROW("Unable to publish node island-status-reset function");
		if (gl_publish_function(oclass, "pwr_object_swing_status_check", (FUNCTIONADDR)node_swing_status) == nullptr)
			GL_THROW("Unable to publish node swing-status check function");
		if (gl_publish_function(oclass, "pwr_object_shunt_update", (FUNCTIONADDR)node_update_shunt_values) == nullptr)
			GL_THROW("Unable to publish node shunt update function");
	}
}

int node::isa(const char *classname)
{
	return strcmp(classname,"node")==0 || powerflow_object::isa(classname);
}

int node::create(void)
{
	int result = powerflow_object::create();

#ifdef SUPPORT_OUTAGES
	condition=OC_NORMAL;
#endif

	n++;

	bustype = PQ;
	busflags = NF_HASSOURCE;
	busphasesIn = 0;
	busphasesOut = 0;
	reference_bus = nullptr;
	nominal_voltage = 0.0;
	maximum_voltage_error = 0.0;
	frequency = nominal_frequency;
	fault_Z = 1e-6;
	prev_NTime = 0;
	SubNode = SNT_NONE;
	SubNodeParent = nullptr;
	TopologicalParent = nullptr;
	NR_subnode_reference = nullptr;
	Extra_Data=nullptr;
	Extra_Data_Track_FPI = nullptr;
	NR_link_table = nullptr;
	NR_connected_links[0] = NR_connected_links[1] = 0;
	NR_number_child_nodes[0] = NR_number_child_nodes[1] = 0;
	NR_child_nodes = nullptr;

	NR_node_reference = -1;	//Newton-Raphson bus index, set to -1 initially
	house_present = false;	//House attachment flag
	nom_res_curr[0] = nom_res_curr[1] = nom_res_curr[2] = 0.0;	//Nominal house current variables

	prev_phases = 0x00;

	mean_repair_time = 0.0;

	// Only used in capacitors, at this time, but put into node for future functionality (maybe with reliability?)
	service_status = ND_IN_SERVICE;
	service_status_dbl = -1.0;	//Initial flag is to ignore it.  As soon as this changes, it overrides service_status
	last_disconnect = 0;		//Will get set in init
	previous_uptime = -1.0;		///< Flags as not initialized
	current_uptime = -1.0;		///< Flags as not initialized

	full_Y = nullptr;		//Not used by default
	full_Y_load[0][0] = full_Y_load[0][1] = full_Y_load[0][2] = gld::complex(0.0,0.0);	//Empty, by default
	full_Y_load[1][0] = full_Y_load[1][1] = full_Y_load[1][2] = gld::complex(0.0,0.0);
	full_Y_load[2][0] = full_Y_load[2][1] = full_Y_load[2][2] = gld::complex(0.0,0.0);
	full_Y_all = nullptr;	//Not used by default   **** NOTE -- full_Y_all only appears to be used by diesel QSTS exciter code - it can probably be removed when that is fixed *****
	BusHistTerm[0] = gld::complex(0.0,0.0);
	BusHistTerm[1] = gld::complex(0.0,0.0);
	BusHistTerm[2] = gld::complex(0.0,0.0);
	prev_delta_time = -1.0;
	ahrlloadstore = nullptr;
	bhrlloadstore = nullptr;
	chrcloadstore = nullptr;
	LoadHistTermL = nullptr;
	LoadHistTermC = nullptr;

	shunt_change_check[0] = shunt_change_check[1] = shunt_change_check[2] = gld::complex (0.0,0.0);
	shunt_change_check_dy[0] = shunt_change_check_dy[1] = gld::complex (0.0,0.0);
	shunt_change_check_dy[2] = shunt_change_check_dy[3] = gld::complex (0.0,0.0);
	shunt_change_check_dy[4] = shunt_change_check_dy[5] = gld::complex (0.0,0.0);

	memset(voltage,0,sizeof(voltage));
	memset(voltaged,0,sizeof(voltaged));
	memset(current,0,sizeof(current));
	memset(pre_rotated_current,0,sizeof(pre_rotated_current));
	memset(power,0,sizeof(power));
	memset(shunt,0,sizeof(shunt));

	deltamode_dynamic_current[0] = deltamode_dynamic_current[1] = deltamode_dynamic_current[2] = gld::complex(0.0,0.0);

	deltamode_PGenTotal = gld::complex(0.0,0.0);

	current_dy[0] = current_dy[1] = current_dy[2] = gld::complex(0.0,0.0);
	current_dy[3] = current_dy[4] = current_dy[5] = gld::complex(0.0,0.0);
	power_dy[0] = power_dy[1] = power_dy[2] = gld::complex(0.0,0.0);
	power_dy[3] = power_dy[4] = power_dy[5] = gld::complex(0.0,0.0);
	shunt_dy[0] = shunt_dy[1] = shunt_dy[2] = gld::complex(0.0,0.0);
	shunt_dy[3] = shunt_dy[4] = shunt_dy[5] = gld::complex(0.0,0.0);

	prev_voltage_value = nullptr;	//NULL the pointer, just for the sake of doing so
	prev_power_value = nullptr;	//NULL the pointer, again just for the sake of doing so
	node_type = NORMAL_NODE;	//Assume we're nothing special by default
	current_accumulated = false;
	deltamode_inclusive = false;	//Begin assuming we aren't delta-enabled
	dynamic_norton = false;			//By default, no one needs the Norton equivalent posting
	dynamic_norton_child = false;	//By default, we're assuming we don't have any descendants doing Norton-equivalents
	dynamic_generator = false;		//By default, we don't have any generator attached
	swing_functions_enabled = false;	//By default, this bus isn't behaving as a swing

	//Check to see if we need to enable an overall frequency method, by default (individual object can override)
	if (all_powerflow_freq_measure_method == FMM_SIMPLE)	//Default to simple method
	{
		fmeas_type = FM_SIMPLE;	//By default, use the simple frequency measurement method
	}
	else if (all_powerflow_freq_measure_method == FMM_PLL)	//Default to PLL
	{
		fmeas_type = FM_PLL;	//By default, use the PLL frequency measurement method
	}
	else	//Probably "NONE", but just default to nothing, whatever this was
	{
		fmeas_type = FM_NONE;	//By default, no frequency measurement occurs
	}

	//Default frequency measurement parameters
	freq_omega_ref=0.0;
	freq_sfm_Tf=0.01;
	freq_pll_Kp=10;
	freq_pll_Ki=100;
	first_freq_init=true;	//Start with saying we haven't been in yet

	//Set default values
	curr_freq_state.fmeas[0] = nominal_frequency;
	curr_freq_state.fmeas[1] = nominal_frequency;
	curr_freq_state.fmeas[2] = nominal_frequency;
	curr_freq_state.average_freq = nominal_frequency;

	//GFA functionality - put in node, in case it needs to be added
	GFA_enable = false;			//disabled, by default
	GFA_freq_low_trip = 59.5;
	GFA_freq_high_trip = 60.5;
	GFA_voltage_low_trip = 0.8;
	GFA_voltage_high_trip = 1.2;
	GFA_reconnect_time = 300.0;
	GFA_freq_disconnect_time = 0.2;
	GFA_volt_disconnect_time = 0.2;
	GFA_status = true;
	prev_time_dbl = 0.0;				//Tracking variable
	GFA_Update_time = 0.0;
	GFA_trip_method = GFA_NONE;		//By default, not tripped

	//VFD-related additional functionality
	VFD_attached = false;	//Not connected to a VFD
	VFD_updating_function = nullptr;	//Make sure is set empty
	VFD_object = nullptr;	//Make empty

	//Multi-island tracking
	reset_island_state = false;	//Reset is disabled, by default

	//Triplex/FBS variable
	tn_values = nullptr;

	return result;
}

int node::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	OBJECT *tmp_obj, *tmp_subnode_parent;
	node *tmp_node, *tmp_par_node;
	int index_loop_val;

	//Put the phase_S check right on the top, since it will apply to both solvers
	if (has_phase(PHASE_S))
	{
		//Make sure we're a valid class
		if (!(gl_object_isa(obj,"triplex_node","powerflow") || gl_object_isa(obj,"triplex_meter","powerflow") || gl_object_isa(obj,"triplex_load","powerflow") || gl_object_isa(obj,"motor","powerflow") || gl_object_isa(obj,"performance_motor","powerflow")))
		{
			GL_THROW("Object:%d - %s -- has a phase S, but is not triplex!",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			A node-based object has an "S" in the phases, but is not a triplex_node, triplex_load, nor triplex_meter.  These are the only
			objects that support this phase.  Please check your phases and try again.
			*/
		}
		//Default else - implies it is one of these objects, and therefore can have a phase S
	}

	if (solver_method==SM_NR)
	{
		char ext_lib_file_name[1025];
		char extpath[1024];
		CALLBACKS **cbackval = nullptr;
		bool ExtLinkFailure;
		STATUS status_ret_value;

		// Store the topological parent before anyone overwrites it
		TopologicalParent = obj->parent;

		//Check for a swing bus if we haven't found one already
		if (NR_swing_bus == nullptr)
		{
			//See if a node is a master swing
			NR_swing_bus = NR_master_swing_search("node",true);

			//If one hasn't been found, see if there's a substation one
			if (NR_swing_bus == nullptr)
			{
				NR_swing_bus = NR_master_swing_search("substation",true);
			}
			//Default else -- one already found, progress through this

			//If one hasn't been found, see if there's a meter one
			if (NR_swing_bus == nullptr)
			{
				NR_swing_bus = NR_master_swing_search("meter",true);
			}
			//Default else -- one already found, progress through this

			//If one hasn't been found, see if there's a load one
			if (NR_swing_bus == nullptr)
			{
				//Do the same for loads
				NR_swing_bus = NR_master_swing_search("load",true);
			}
			//Default else -- one already found, progress through this

			//If one hasn't been found, see if there's a triplex_node one
			if (NR_swing_bus == nullptr)
			{
				//Do the same for triplex nodes
				NR_swing_bus = NR_master_swing_search("triplex_node",true);
			}
			//Default else -- one already found, progress through this

			//If one hasn't been found, see if there's a triplex_meter one
			if (NR_swing_bus == nullptr)
			{
				//And one more time for triplex meters
				NR_swing_bus = NR_master_swing_search("triplex_meter",true);
			}
			//Default else -- one already found, progress through this

			//If one hasn't been found, see if there's a triplex_load one
			if (NR_swing_bus == nullptr)
			{
				//And one more time for triplex loads
				NR_swing_bus = NR_master_swing_search("triplex_load",true);
			}
			//Default else -- one already found, progress through this

			//Now repeat this process for SWING_PQ designations
			//Check nodes
			if (NR_swing_bus == nullptr)
			{
				NR_swing_bus = NR_master_swing_search("node",false);
			}

			//If one hasn't been found, see if there's a substation one
			if (NR_swing_bus == nullptr)
			{
				NR_swing_bus = NR_master_swing_search("substation",false);
			}
			//Default else -- one already found, progress through this

			//If one hasn't been found, see if there's a meter one
			if (NR_swing_bus == nullptr)
			{
				NR_swing_bus = NR_master_swing_search("meter",false);
			}
			//Default else -- one already found, progress through this

			//If one hasn't been found, see if there's a load one
			if (NR_swing_bus == nullptr)
			{
				//Do the same for loads
				NR_swing_bus = NR_master_swing_search("load",false);
			}
			//Default else -- one already found, progress through this

			//If one hasn't been found, see if there's a triplex_node one
			if (NR_swing_bus == nullptr)
			{
				//Do the same for triplex nodes
				NR_swing_bus = NR_master_swing_search("triplex_node",false);
			}
			//Default else -- one already found, progress through this

			//If one hasn't been found, see if there's a triplex_meter one
			if (NR_swing_bus == nullptr)
			{
				//And one more time for triplex meters
				NR_swing_bus = NR_master_swing_search("triplex_meter",false);
			}
			//Default else -- one already found, progress through this

			//If one hasn't been found, see if there's a triplex_load one
			if (NR_swing_bus == nullptr)
			{
				//And one more time for triplex loads
				NR_swing_bus = NR_master_swing_search("triplex_load",false);
			}
			//Default else -- one already found, progress through this

			//Make sure there is one bus to rule them all, even if it actually has rivals
			if (NR_swing_bus == nullptr)
			{
				GL_THROW("NR: no swing bus found");
				/*	TROUBLESHOOT
				No swing bus was located in the test system.  Newton-Raphson requires at least one node
				be designated "bustype SWING".
				*/
			}

			//Now that we have one bus to rule them all, make sure it has space for its variables
			//Assumes one monolithic grid, by default
			status_ret_value = NR_array_structure_allocate(&NR_powerflow,1);

			//Make sure it worked
			if (status_ret_value == FAILED)
			{
				GL_THROW("NR: Failed to allocate monolithic grid solver arrays");
				/*  TROUBLESHOOT
				While attempting to allocate the initial, single-island/monolithic array,
				an error occurred.  Please try again.  If the error persists, please submit your
				code and a report via the ticketing/issues system.
				*/
			}

			//And update the master counter
			NR_islands_detected = 1;
		}//end swing bus search

		//SWING check - defer init until we're sure where we go
		if (obj == NR_swing_bus)
		{
			if (!NR_swing_deferred_pass)
			{
				//Set the flag - to indicate the deferral has happened
				NR_swing_deferred_pass = true;

				//Deferred init
				gl_verbose("node::init(): node:%d - %s - deferring initialization for swing ranking", obj->id,(obj->name?obj->name : "Unnamed"));
				return 2; // defer
			}
		}

		//Check for parents to see if they are a parent/childed load
		if (obj->parent!=nullptr) 	//Has a parent, let's see if it is a node and link it up
		{						//(this will break anything intentionally done this way - e.g. switch between two nodes)
			if (!(gl_object_isa(obj->parent,"node","powerflow")))	//All others alias up to isa:node eventually
				GL_THROW("NR: Parent is not a node-based object!");
				/*  TROUBLESHOOT
				A Newton-Raphson parent-child connection was attempted on a non-node.  The parent object must be a node, load, or meter object in the
				powerflow module for this connection to be successful.
				*/

			node *parNode = OBJECTDATA(obj->parent,node);

			//See if it is a swing, just to toss a warning
			if ((parNode->bustype==SWING) || (parNode->bustype==SWING_PQ))
			{
				gl_warning("Node:%s is parented to a swing node and will get folded into it.",obj->name);
				/*  TROUBLESHOOT
				When a node has the parent field populated, it is assumed this is for a "zero-length" line connection.
				If a swing node is specified, it will assume the node is linked to the swing node via a zero-length
				connection.  If this is undesired, remove the parenting to the swing node.
				*/
			}

			//Rank the child object (which should propagate upward)
			gl_set_rank(obj,3);				//Start as below normal nodes (but above links)
											//This way load postings should propogate during sync (bottom-up)

			//Flag our index as a child as well, as yet another catch
			NR_node_reference = -99;

			//Child run - now push upward to get SubParentNode and check ranks
			tmp_obj = obj->parent;

			//Loop to find the top of this chain
			while (tmp_obj != nullptr)
			{
				//Temp track
				tmp_subnode_parent = tmp_obj;

				//Get node reference to pull phases
				tmp_node = OBJECTDATA(tmp_obj,node);

				//See if our parent is already initialized (e.g., we are a lower child)
				if (tmp_node->SubNodeParent == nullptr)
				{
					//Grab the next one
					tmp_obj = tmp_subnode_parent->parent;
				}
				else
				{
					//Pull it
					tmp_subnode_parent = tmp_node->SubNodeParent;

					//Manual break
					tmp_obj = nullptr;
				}
			}
			//Once fails, reached top of parent chain (theoretically)

			//Check ranking
			if ((tmp_subnode_parent->rank+2) > NR_expected_swing_rank)
			{
				//Update the tracker
				NR_expected_swing_rank = tmp_subnode_parent->rank+2;
			}

			//Pull the node pointer real quick
			tmp_par_node = OBJECTDATA(tmp_subnode_parent,node);

			//Update our count
			tmp_par_node->NR_number_child_nodes[0]++;	//Increment the counter of child nodes - we'll alloc and link them later

			//Set our SubNodeParent
			SubNodeParent = tmp_subnode_parent;

			//Set our sub_node_reference too
			NR_subnode_reference = &(tmp_par_node->NR_node_reference);

			//Now loop to populate the subparent and reference property
			tmp_obj = obj->parent;
			while (tmp_obj != tmp_subnode_parent)
			{
				//Pull the reference
				tmp_node = OBJECTDATA(tmp_obj,node);

				//See if the SubNodeParent is set
				if (tmp_node->SubNodeParent == nullptr)
				{
					//Set subnodeparent
					tmp_node->SubNodeParent = tmp_subnode_parent;

					//Set the pointer to parent's NR pointer (so links can go there appropriately)
					tmp_node->NR_subnode_reference = &(tmp_par_node->NR_node_reference);

					//Get the next object
					tmp_obj = tmp_obj->parent;
				}
				else	//We've caught up to the chain - end
				{
					//Assign the while terminating condition
					tmp_obj = tmp_subnode_parent;
				}
			}

			//Phase variable
			set p_phase_to_check, c_phase_to_check;

			//Initialize
			p_phase_to_check = NO_PHASE;
			c_phase_to_check = NO_PHASE;

			//Initial scan (may be elsewhere) - if we're triplex, make sure our parent is triplex!
			if (((phases & PHASE_S) ^ (parNode->phases & PHASE_S)) == PHASE_S)
			{
				GL_THROW("NR: Parent and child node phases for nodes %s and %s are not both triplex!",obj->parent->name,obj->name);
				/*  TROUBLESHOOT
				A node device with phases S is either parented or childed to another node-device that does not have
				phases S - this is not allowed.  Correct the model.
				*/
			}

			//Create base phase set for parent
			p_phase_to_check = (parNode->phases & PHASE_ABC);

			//Do same for child
			c_phase_to_check = (phases & PHASE_ABC);

			//Fundamental check - see if we're even base compatible
			if ((p_phase_to_check & c_phase_to_check) != c_phase_to_check)	//Our parent is lacking, fail
			{
				//See if we're triplex - that gets "special" checks
				if ((phases & PHASE_S) == PHASE_S)
				{
					//One better be zero, or fail
					if ((p_phase_to_check != NO_PHASE) && (c_phase_to_check != NO_PHASE))
					{
						GL_THROW("NR: Parent and child node phases for nodes %s and %s do not match!",obj->parent->name,obj->name);
						/*  TROUBLESHOOT
						A childed triplex object and parented triplex object have explicit phases specified that are incompatible.
						e.g., phases AS and phases BS - phases AS and phases S would be acceptable.
						Please correct your model and try again.
						*/
					}
					//Default else - one is undefined, which is okay
				}
				else	//Nope, we're a failure
				{
					GL_THROW("NR: Parent and child node phases for nodes %s and %s do not match!",obj->parent->name,obj->name);
					/*  TROUBLESHOOT
					A childed object has more fundamental phases (ABC) than its parent - this is an invalid connection.
					Please double-check and adjust your model accordingly.
					*/
				}
			}
			//Default else - phases are compatible

			//Determine how to flag our "powerflow solution" parent
			if (((phases & PHASE_D) ^ (tmp_par_node->phases & PHASE_D)) == PHASE_D)
			{
				//Mismatched child/powerflow parent

				//Flag us
				SubNode |= SNT_DIFF_CHILD;

				//Flag our powerflow parent
				tmp_par_node->SubNode |= SNT_DIFF_PARENT;

				//Clear off the standard parent flag (if it was set) - it will break things later
				tmp_par_node->SubNode &= (~SNT_PARENT);

				//Allocate and point our properties up to the parent node
				if (tmp_par_node->Extra_Data == nullptr)	//Make sure someone else hasn't allocated it for us
				{
					tmp_par_node->Extra_Data = (gld::complex *)gl_malloc(9*sizeof(gld::complex));
					if (tmp_par_node->Extra_Data == nullptr)
					{
						GL_THROW("NR: Memory allocation failure for differently connected load.");
						/*  TROUBLESHOOT
						This is a bug.  Newton-Raphson tried to allocate memory for other necessary
						information to handle a parent-child relationship with differently connected loads.
						Please submit your code and a bug report using the trac website.
						*/
					}

					//Zero the entries
					for (index_loop_val=0; index_loop_val<9; index_loop_val++)
					{
						tmp_par_node->Extra_Data[index_loop_val] = gld::complex(0.0,0.0);
					}

					//If we're FPI, chances are the Extra_Data_Track_FPI needs it too
					if (NR_solver_algorithm == NRM_FPI)
					{
						//Double check
						if (tmp_par_node->Extra_Data_Track_FPI == nullptr)
						{
							//Allocate it
							tmp_par_node->Extra_Data_Track_FPI = (gld::complex *)gl_malloc(3*sizeof(gld::complex));

							//Check it
							if (tmp_par_node->Extra_Data_Track_FPI == nullptr)
							{
								GL_THROW("NR: Memory allocation failure for differently connected load.");
								//Defined above
							}

							//Zero the entries, just to be safe/paranoid
							for (index_loop_val=0; index_loop_val<3; index_loop_val++)
							{
								tmp_par_node->Extra_Data_Track_FPI[index_loop_val] = gld::complex(0.0,0.0);
							}
						}//End NULLed Extra_Data_Track_FPI
					}//End FPI block
				}//End Extra data alloc
			}//End phase D check
			else	//match phases - "standard" child (at least in this case)
			{
				//Set flag for us
				SubNode |= SNT_CHILD;

				//Flag our powerflow parent - make sure it wasn't diff parented first
				if ((tmp_par_node->SubNode & SNT_DIFF_PARENT) != SNT_DIFF_PARENT)
					tmp_par_node->SubNode |= SNT_PARENT;

				//Zero out last child power vector (used for updates)
				last_child_power[0][0] = last_child_power[0][1] = last_child_power[0][2] = gld::complex(0,0);
				last_child_power[1][0] = last_child_power[1][1] = last_child_power[1][2] = gld::complex(0,0);
				last_child_power[2][0] = last_child_power[2][1] = last_child_power[2][2] = gld::complex(0,0);
				last_child_power[3][0] = last_child_power[3][1] = last_child_power[3][2] = gld::complex(0,0);
				last_child_current12 = 0.0;

				//Do the same for the delta/wye explicit portions
				last_child_power_dy[0][0] = last_child_power_dy[0][1] = last_child_power_dy[0][2] = gld::complex(0.0,0.0);
				last_child_power_dy[1][0] = last_child_power_dy[1][1] = last_child_power_dy[1][2] = gld::complex(0.0,0.0);
				last_child_power_dy[2][0] = last_child_power_dy[2][1] = last_child_power_dy[2][2] = gld::complex(0.0,0.0);
				last_child_power_dy[3][0] = last_child_power_dy[3][1] = last_child_power_dy[3][2] = gld::complex(0.0,0.0);
				last_child_power_dy[4][0] = last_child_power_dy[4][1] = last_child_power_dy[4][2] = gld::complex(0.0,0.0);
				last_child_power_dy[5][0] = last_child_power_dy[5][1] = last_child_power_dy[5][2] = gld::complex(0.0,0.0);
			}

			//Make sure nominal voltages match
			if (nominal_voltage != parNode->nominal_voltage)
			{
				gl_warning("Node:%s does not have the same nominal voltage as its parent - copying voltage from parent.",(obj->name ? obj->name : "unnamed"));
				/*  TROUBLESHOOT
				A node object was parented to another node object with a different nominal_voltage set.  The childed object will now
				take the parent nominal_voltage.  If this is not intended, please add a transformer between these nodes.
				*/

				nominal_voltage = parNode->nominal_voltage;
			}
		}
		else	//Non-childed node gets the count updated
		{
			NR_bus_count++;		//Update global bus count for NR solver

			//Update ranking
			//Ranking - similar to GS
			if (obj==NR_swing_bus)	//Make sure we're THE swing bus and not just A swing bus
			{
				//Set the rank
				gl_set_rank(obj,NR_expected_swing_rank);

				//Flag it
				NR_swing_rank_set = true;
			}
			else	//Normal nodes and rival swing buses end up starting in the same rank
			{
				if (obj->rank<4)
				{
					gl_set_rank(obj,4);
				}
				//Default else - means something else set us already

				//Update the SWING tracker
				if ((obj->rank+2)>NR_expected_swing_rank)
				{
					//Update tracker - swing two steps higher than the "rabble" nodes
					NR_expected_swing_rank = obj->rank+2;
				}
				//Default else - it is already here or higher
			}
		}

		//Use SWING node to hook in the solver - since it's called by SWING, might as well
		//Make sure it is the "master swing" too
		if (obj == NR_swing_bus)
		{
			if (LUSolverName[0]=='\0')	//Empty name, default to superLU
			{
				matrix_solver_method=MM_SUPERLU;	//This is the default, but we'll set it here anyways
			}
			else	//Something is there, see if we can find it
			{
				//Initialize the global
				LUSolverFcns.dllLink = nullptr;
				LUSolverFcns.ext_init = nullptr;
				LUSolverFcns.ext_alloc = nullptr;
				LUSolverFcns.ext_solve = nullptr;
				LUSolverFcns.ext_destroy = nullptr;

#ifdef _WIN32
				snprintf(ext_lib_file_name, 1024, "solver_%s" DLEXT,LUSolverName.get_string());
#else
				snprintf(ext_lib_file_name, 1024, "lib_solver_%s" DLEXT,LUSolverName.get_string());
#endif

				if (gl_findfile(ext_lib_file_name, nullptr, 0|4, extpath,sizeof(extpath))!=nullptr)	//Link up
				{
					//Link to the library
					LUSolverFcns.dllLink = DLLOAD(extpath);

					//Make sure it worked
					if (LUSolverFcns.dllLink==nullptr)
					{
						gl_warning("Failure to load solver_%s as a library, defaulting to superLU",LUSolverName.get_string());
						/*  TROUBLESHOOT
						While attempting to load the DLL for an external LU solver, the file did not appear to meet
						the libary specifications.  superLU is being used instead.  Please check the library file and
						try again.
						*/

						//Set to superLU
						matrix_solver_method = MM_SUPERLU;
					}
					else	//Found it right
					{
						//Set tracking flag - if anything fails, just revert to superLU and leave
						ExtLinkFailure = false;

						//Link the callback
						cbackval = (CALLBACKS **)DLSYM(LUSolverFcns.dllLink, "callback");

						//I don't know what this does
						if(cbackval)
							*cbackval = callback;

						//Now link functions - Init
						LUSolverFcns.ext_init = DLSYM(LUSolverFcns.dllLink,"LU_init");

						//Make sure it worked
						if (LUSolverFcns.ext_init == nullptr)
						{
							gl_warning("LU_init of external solver solver_%s not found, defaulting to superLU",LUSolverName.get_string());
							/*  TROUBLESHOOT
							While attempting to link the LU_init routine of an external LU matrix solver library, the routine
							failed to be found.  Check the external library and try again.  At this failure, powerflow will revert
							to the superLU solver for Newton-Raphson.
							*/

							//Flag the failure
							ExtLinkFailure = true;
						}

						//Now link functions - alloc
						LUSolverFcns.ext_alloc = DLSYM(LUSolverFcns.dllLink,"LU_alloc");

						//Make sure it worked
						if (LUSolverFcns.ext_alloc == nullptr)
						{
							gl_warning("LU_init of external solver solver_%s not found, defaulting to superLU",LUSolverName.get_string());
							/*  TROUBLESHOOT
							While attempting to link the LU_init routine of an external LU matrix solver library, the routine
							failed to be found.  Check the external library and try again.  At this failure, powerflow will revert
							to the superLU solver for Newton-Raphson.
							*/

							//Flag the failure
							ExtLinkFailure = true;
						}

						//Now link functions - solve
						LUSolverFcns.ext_solve = DLSYM(LUSolverFcns.dllLink,"LU_solve");

						//Make sure it worked
						if (LUSolverFcns.ext_solve == nullptr)
						{
							gl_warning("LU_init of external solver solver_%s not found, defaulting to superLU",LUSolverName.get_string());
							/*  TROUBLESHOOT
							While attempting to link the LU_init routine of an external LU matrix solver library, the routine
							failed to be found.  Check the external library and try again.  At this failure, powerflow will revert
							to the superLU solver for Newton-Raphson.
							*/

							//Flag the failure
							ExtLinkFailure = true;
						}

						//Now link functions - destroy
						LUSolverFcns.ext_destroy = DLSYM(LUSolverFcns.dllLink,"LU_destroy");

						//Make sure it worked
						if (LUSolverFcns.ext_destroy == nullptr)
						{
							gl_warning("LU_destroy of external solver solver_%s not found, defaulting to superLU",LUSolverName.get_string());
							/*  TROUBLESHOOT
							While attempting to link the LU_init routine of an external LU matrix solver library, the routine
							failed to be found.  Check the external library and try again.  At this failure, powerflow will revert
							to the superLU solver for Newton-Raphson.
							*/

							//Flag the failure
							ExtLinkFailure = true;
						}


						//If any failed, just revert to superLU (probably shouldn't even check others after a failure, but meh)
						if (ExtLinkFailure)
						{
							//Someone failed, just use superLU
							matrix_solver_method=MM_SUPERLU;
						}
						else
						{
							gl_verbose("External solver solver_%s found, utilizing for NR",LUSolverName.get_string());
							/*  TROUBLESHOOT
							An external LU matrix solver library was specified and found, so NR will be calculated
							using that instead of superLU.
							*/

							//Flag as an external solver
							matrix_solver_method=MM_EXTERN;
						}
					}//End found proper DLL
				}//end found external and linked
				else	//Not found, just default to superLU
				{
					gl_warning("The external solver solver_%s could not be found, defaulting to superLU",LUSolverName.get_string());
					/*  TROUBLESHOOT
					While attempting to link an external LU matrix solver library, the file could not be found.  Ensure it is
					in the proper GridLAB-D folder and is named correctly.  If the error persists, please submit your code and
					a bug report via the trac website.
					*/

					//Flag as superLU
					matrix_solver_method=MM_SUPERLU;
				}//end failed to find/load
			}//end somethign was attempted

			if(default_resistance <= 0.0){
				gl_error("INIT: The global default_resistance was less than or equal to zero. default_resistance must be greater than zero.");
				return 0;
			}

			//Check and see if in-rush and impedance conversion are enabled - if so, disable one
			if (enable_inrush_calculations && enable_impedance_conversion)
			{
				//Turn off impedance conversion, otherwise it breaks in-rush in weird ways
				enable_impedance_conversion = false;

				//Throw as a verbose - behavior is the same
				gl_verbose("NR: enable_inrush and enable_impedance_conversion conflict - in-rush overrides");
				/*  TROUBLESHOOT
				The in-rush-based calculations and enable_impedance_conversion basically do the same thing, but
				in different sequencing intervals.  When in-rush is enabled, it performs the impedance conversion
				anyways.  If enable_impedance_conversion is enabled, it can cause conflicts in calculations, so it was
				disabled.  The observable behavior should not be affected.
				*/
			}
		}//End matrix solver if

		if (mean_repair_time < 0.0)
		{
			gl_warning("node:%s has a negative mean_repair_time, set to 1 hour",obj->name);
			/*  TROUBLESHOOT
			A node object had a mean_repair_time that is negative.  This ia not a valid setting.
			The value has been changed to 0.  Please set the variable to an appropriate variable
			*/

			mean_repair_time = 0.0;	//Set to zero by default
		}
	}
	else if (solver_method==SM_GS)
	{
		GL_THROW("Gauss_Seidel is a deprecated solver and has been removed");
		/*  TROUBLESHOOT 
		The Gauss-Seidel solver implementation has been removed as of version 3.0.
		It was never fully functional and has not been updated in a couple versions.  The source
		code still exists in older repositories, so if you have an interest in that implementation, please
		try an older subversion number.
		*/
	}
	else if (solver_method == SM_FBS)	//Forward back sweep
	{
		// Store the topological parent before anyone overwrites it
		TopologicalParent = obj->parent;

		if (obj->parent != nullptr)
		{
			if((gl_object_isa(obj->parent,"load","powerflow") | gl_object_isa(obj->parent,"node","powerflow") | gl_object_isa(obj->parent,"meter","powerflow") | gl_object_isa(obj->parent,"substation","powerflow")))	//Parent is another node
			{
				node *parNode = OBJECTDATA(obj->parent,node);

				//Phase variable
				set p_phase_to_check, c_phase_to_check;

				//Create D-less and N-less versions of both for later comparisons
				p_phase_to_check = (parNode->phases & (~(PHASE_D | PHASE_N)));
				c_phase_to_check = (phases & (~(PHASE_D | PHASE_N)));

				//Make sure our phases align, otherwise become angry
				if ((p_phase_to_check & c_phase_to_check) != c_phase_to_check)	//Our parent is lacking, fail
				{
					GL_THROW("Parent and child node phases are incompatible for nodes %s and %s.",obj->parent->name,obj->name);
					/*  TROUBLESHOOT
					A child object does not have compatible phases with its parent.  The parent needs to at least have the phases of
					the child object.  Please check your connections and try again.
					*/
				}

				//Make sure nominal voltages match
				if (nominal_voltage != parNode->nominal_voltage)
				{
					gl_warning("Node:%s does not have the same nominal voltage as its parent - copying voltage from parent.",(obj->name ? obj->name : "unnamed"));
					//Define above

					nominal_voltage = parNode->nominal_voltage;
				}
			}//No else here, may be a line due to FBS implementation, so we don't want to fail on that
		}
	}
	else
		GL_THROW("unsupported solver method");
		/*Defined below*/

	/* initialize the powerflow base object */
	int result = powerflow_object::init(parent);

	/* Ranking stuff for GS parent/child relationship - needs to be rethought - premise of loads/nodes above links MUST remain true */
	if (solver_method==SM_GS)
	{
		GL_THROW("Gauss_Seidel is a deprecated solver and has been removed");
		/*  TROUBLESHOOT 
		The Gauss-Seidel solver implementation has been removed as of version 3.0.
		It was never fully functional and has not been updated in a couple versions.  The source
		code still exists in older repositories, so if you have an interest in that implementation, please
		try an older subversion number.
		*/
	}

	/* unspecified phase inherits from parent, if any */
	if (nominal_voltage==0 && parent)
	{
		powerflow_object *pParent = OBJECTDATA(parent,powerflow_object);
		if (gl_object_isa(parent,"transformer"))
		{
			PROPERTY *transformer_config,*transformer_secondary_voltage;
			OBJECT *trans_config_obj;
			double *trans_secondary_voltage_value;
			size_t offset_val;
			char buffer[128];

			//Get configuration link
			transformer_config = gl_get_property(parent,"configuration");

			//Check it
			if (transformer_config == nullptr)
			{
				GL_THROW("Error mapping secondary voltage property from transformer:%d %s",parent->id,parent->name?parent->name:"unnamed");
				/*  TROUBLESHOOT
				While attempting to map to the secondary voltage of a transformer to get a value for nominal voltage, an error
				occurred.  Please try again.  If the problem persists, please submit your code and a bug report via the ticketing
				system.
				*/
			}

			//Pull the object pointer
			offset_val = gl_get_value(parent, GETADDR(parent, transformer_config), buffer, 127, transformer_config);

			//Make sure it worked
			if (offset_val == 0)
			{
				GL_THROW("Error mapping secondary voltage property from transformer:%d %s",parent->id,parent->name?parent->name:"unnamed");
				//Defined above
			}

			//trans_config_obj = gl_get_object(parent,transformer_config);
			trans_config_obj = gl_get_object(buffer);

			if (trans_config_obj == 0)
			{
				GL_THROW("Error mapping secondary voltage property from transformer:%d %s",parent->id,parent->name?parent->name:"unnamed");
				//Defined above
			}

			//Now get secondary voltage
			transformer_secondary_voltage = gl_get_property(trans_config_obj,"secondary_voltage");

			//Make sure it worked
			if (transformer_secondary_voltage == nullptr)
			{
				GL_THROW("Error mapping secondary voltage property from transformer:%d %s",parent->id,parent->name?parent->name:"unnamed");
				//Defined above
			}

			//Now get the final one, the double value for secondary voltage
			trans_secondary_voltage_value = gl_get_double(trans_config_obj,transformer_secondary_voltage);

			//Make sure it worked
			if (trans_secondary_voltage_value == nullptr)
			{
				GL_THROW("Error mapping secondary voltage property from transformer:%d %s",parent->id,parent->name?parent->name:"unnamed");
				//Defined above
			}

			//Now steal the value
			nominal_voltage = *trans_secondary_voltage_value;
		}
		else
			nominal_voltage = pParent->nominal_voltage;
	}

	/* make sure the sync voltage limit is positive */
	if (maximum_voltage_error<0)
		throw "negative maximum_voltage_error is invalid";
		/*	TROUBLESHOOT
		A negative maximum voltage error was specified.  This can not be checked.  Specify a
		positive maximum voltage error, or omit this value to let the powerflow solver automatically
		calculate it for you.
		*/

	/* set sync_v_limit to default if needed */
	if (maximum_voltage_error==0.0)
		maximum_voltage_error =  nominal_voltage * default_maximum_voltage_error;

	/* make sure fault_Z is positive */
	if (fault_Z<=0)
		throw "negative fault impedance is invalid";
		/*	TROUBLESHOOT
		The node of interest has a negative or zero fault impedance value.  Specify this value as a
		positive number to enable proper solver operation.
		*/

	/* check that nominal voltage is set */
	if (nominal_voltage<=0)
		throw "nominal_voltage is not set";
		/*	TROUBLESHOOT
		The powerflow solver has detected that a nominal voltage was not specified or is invalid.
		Specify this voltage as a positive value via nominal_voltage to enable the solver to work.
		*/

	/* update geographic degree */
	if (k>1)
	{
		if (geographic_degree>0)
			geographic_degree = n/(1/(geographic_degree/n) + log((double)k));
		else
			geographic_degree = n/log((double)k);
	}

	/* set source flags for SWING and PV buses */
	if ((bustype==SWING) || (bustype==SWING_PQ) || (bustype==PV))
		busflags |= NF_HASSOURCE;

	//Pre-zero non existant phases - deltas handled by phase (for open delta)
	if (has_phase(PHASE_S))	//Single phase
	{
		if (has_phase(PHASE_A))
		{
			if (voltage[0] == 0)
			{
				voltage[0].SetPolar(nominal_voltage,0.0);
			}
			if (voltage[1] == 0)
			{
				voltage[1].SetPolar(nominal_voltage,0.0);
			}
		}
		else if (has_phase(PHASE_B))
		{
			if (voltage[0] == 0)
			{
				voltage[0].SetPolar(nominal_voltage,-PI*2/3);
			}
			if (voltage[1] == 0)
			{
				voltage[1].SetPolar(nominal_voltage,-PI*2/3);
			}
		}
		else if (has_phase(PHASE_C))
		{
			if (voltage[0] == 0)
			{
				voltage[0].SetPolar(nominal_voltage,PI*2/3);
			}
			if (voltage[1] == 0)
			{
				voltage[1].SetPolar(nominal_voltage,PI*2/3);
			}
		}
		else
		{
			//This could be a verbose - leaving it as a warning though for now
			gl_warning("node:%d - %s - triplex-based node does not specify phase connection - assuming phase A basis",obj->id,(obj->name?obj->name:"Unnamed"));
			/*  TROUBLESHOOT
			A triplex-based node just has phase S specified.  Without a three-phase phase designation, it will assume this is phase A-based and set all
			angles appropriately.  If this is not desired, set a proper phase basis.
			*/
			if (voltage[0] == 0)
			{
				voltage[0].SetPolar(nominal_voltage,0.0);
			}
			if (voltage[1] == 0)
			{
				voltage[1].SetPolar(nominal_voltage,0.0);
			}
		}

		voltage[2] = gld::complex(0,0);	//Ground always assumed it seems
	}
	else if (has_phase(PHASE_A|PHASE_B|PHASE_C))	//three phase
	{
		if (voltage[0] == 0)
		{
			voltage[0].SetPolar(nominal_voltage,0.0);
		}
		if (voltage[1] == 0)
		{
			voltage[1].SetPolar(nominal_voltage,-2*PI/3);
		}
		if (voltage[2] == 0)
		{
			voltage[2].SetPolar(nominal_voltage,2*PI/3);
		}
	}
	else	//Not three phase - check for individual phases and zero them if they aren't already
	{
		if (!has_phase(PHASE_A))
			voltage[0]=0.0;
		else if (voltage[0] == 0)
			voltage[0].SetPolar(nominal_voltage,0);

		if (!has_phase(PHASE_B))
			voltage[1]=0.0;
		else if (voltage[1] == 0)
			voltage[1].SetPolar(nominal_voltage,-2*PI/3);

		if (!has_phase(PHASE_C))
			voltage[2]=0.0;
		else if (voltage[2] == 0)
			voltage[2].SetPolar(nominal_voltage,2*PI/3);
	}

	if (has_phase(PHASE_D) & voltageAB==0)
	{	// compute 3phase voltage differences
		voltageAB = voltageA - voltageB;
		voltageBC = voltageB - voltageC;
		voltageCA = voltageC - voltageA;
	}
	else if (has_phase(PHASE_S))
	{
		//Compute differential voltages (1-2, 1-N, 2-N)
		voltaged[0] = voltage[0] + voltage[1];	//12
		voltaged[1] = voltage[1] - voltage[2];	//2N
		voltaged[2] = voltage[0] - voltage[2];	//1N -- odd order
	}

	//Populate last_voltage with initial values - just in case
	if (solver_method == SM_NR)
	{
		last_voltage[0] = voltage[0];
		last_voltage[1] = voltage[1];
		last_voltage[2] = voltage[2];
	}

	//Initialize uptime variables
	last_disconnect = gl_globalclock;	//Set to current clock

	//Deltamode checks - init, so no locking yet
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		//Flag the variable for later use
		deltamode_inclusive = true;

		//Increment the counter for allocation
		pwr_object_count++;

		//Check out parent and toss some warnings
		if (TopologicalParent != nullptr)
		{
			if ((TopologicalParent->flags & OF_DELTAMODE) != OF_DELTAMODE)
			{
				gl_warning("Object %s (node:%d) is flagged for deltamode, but it's parent is not.  This may lead to incorrect answers!",obj->name?obj->name:"Unknown",obj->id);
				/*  TROUBLESHOOT
				A childed node's parent is not flagged for deltamode.  This may lead to some erroneous errors in the powerflow.  Please apply the
				flags DELTAMODE property to the parent, or utilize the all_powerflow_delta module-level flag to fix this.
				*/
			}
		}

		//update GFA-type
		prev_time_dbl = (double)gl_globalclock;
	}

	//See if frequency measurements are enabled
	if (fmeas_type != FM_NONE)
	{
		//Set the frequency reference, based on the system value
		freq_omega_ref = 2.0 * PI * nominal_frequency;
	}

	return result;
}

//Functionalized presync pass routines for NR solver
//Put in place so deltamode can call it and properly udpate
TIMESTAMP node::NR_node_presync_fxn(TIMESTAMP t0_val)
{
	TIMESTAMP tresults_val;
	double curr_delta_time;
	double curr_ts_dbl, diff_dbl;

	//Set flag, just because
	tresults_val = TS_NEVER;

	//Zero the accumulators for later (meters and such)
	current_inj[0] = current_inj[1] = current_inj[2] = 0.0;

	//Reset flag
	current_accumulated = false;

	//See if in-rush is enabled and we're in deltamode
	if (enable_inrush_calculations)
	{
		if (deltatimestep_running > 0)
		{
			//Get current deltamode timestep
			curr_delta_time = gl_globaldeltaclock;

			//Check and see if we're in a different timestep -- if so, zero the accumulators
			if (curr_delta_time != prev_delta_time)
			{
				//Zero the accumulators
				BusHistTerm[0] = BusHistTerm[1] = BusHistTerm[2] = gld::complex(0.0,0.0);
			}
			//Default else - not a new time, so keep going with same values (no rezero)
		}//End deltamode running
		else	//Enabled, but not running
		{
			//set the time tracking variable, just in case we are going in and out
			prev_delta_time = -1.0;
		}
	}//End inrush enabled
	//Defaulted else -- no in-rush or not deltamode

	if (((SubNode & SNT_DIFF_PARENT) == SNT_DIFF_PARENT))	//Differently connected parent - zero our accumulators
	{
		//Zero them.  Row 1 is power, row 2 is admittance, row 3 is current
		Extra_Data[0] = Extra_Data[1] = Extra_Data[2] = 0.0;

		Extra_Data[3] = Extra_Data[4] = Extra_Data[5] = 0.0;

		Extra_Data[6] = Extra_Data[7] = Extra_Data[8] = 0.0;
	}

	//If we're a parent and "have house", zero our accumulator
	if (((SubNode & (SNT_PARENT | SNT_DIFF_PARENT)) != 0) && house_present)
	{
		nom_res_curr[0] = nom_res_curr[1] = nom_res_curr[2] = 0.0;
	}

	//Base GFA Functionality
	//Call the GFA-type functionality, if appropriate
	//See if it is enabled, and we're not in deltamode
	//Deltamode handled explicitly elsewhere
	if (GFA_enable && (deltatimestep_running < 0))
	{
		//Extract the current timestamp, as a double
		curr_ts_dbl = (double)t0_val;

		//See if we're a new timestep, otherwise, we don't care
		if (prev_time_dbl < curr_ts_dbl)
		{
			//Figure out how far we moved forward
			diff_dbl = curr_ts_dbl - prev_time_dbl;

			//Update the value
			prev_time_dbl = curr_ts_dbl;

			//Do the checks
			GFA_Update_time = perform_GFA_checks(diff_dbl);

			//Check it
			if (GFA_Update_time > 0.0)
			{
				//See which mode we're in
				if (deltamode_inclusive)
				{
					tresults_val = t0_val + (TIMESTAMP)(floor(GFA_Update_time));

					//Regardless of the return, schedule us for a delta transition - if it clears by then, we should
					//hop right back out
					schedule_deltamode_start(tresults_val);
				}
				else	//Steady state
				{
					tresults_val = t0_val + (TIMESTAMP)(ceil(GFA_Update_time));
				}
			}
		}
		//Default else -- same timestep, so don't care
	}
	//Default else - GFA is not enabled, or in deltamode

	//Return value
	return tresults_val;
}

//Full node presync function
TIMESTAMP node::presync(TIMESTAMP t0)
{
	unsigned int index_val;
	FILE *NRFileDump;
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t1 = powerflow_object::presync(t0);
	TIMESTAMP temp_time_value, temp_t1_value;
	node *temp_par_node = nullptr;
	gld_property *temp_complex_property = nullptr;
	gld_wlock *test_rlock = nullptr;
	gld::complex temp_complex_value;
	complex_array temp_complex_array;
	int temp_idx_x, temp_idx_y;

	//Determine the flag state - see if a schedule is overriding us
	if (service_status_dbl>-1.0)
	{
		if (service_status_dbl == 0.0)
		{
			service_status = ND_OUT_OF_SERVICE;
		}
		else if (service_status_dbl == 1.0)
		{
			service_status = ND_IN_SERVICE;
		}
		else	//Unknown, toss an error to stop meddling kids
		{
			GL_THROW("Invalid value for service_status_double");
			/*  TROUBLESHOOT
			service_status_double was set to a value other than 0 or 1.  This variable
			is meant as a convenience for schedules to drive the service_status variable, so
			IN_SERVICE=1 and OUT_OF_SERVICE=0 are the only valid states.  Please fix the value
			put into this variable.
			*/
		}
	}

	//Handle disconnect items - only do at new times
	if (prev_NTime!=t0)
	{
		if (service_status == ND_IN_SERVICE)
		{
			//See when the last update was
			if ((last_disconnect == prev_NTime) && (current_uptime == -1.0))	//Just reconnected
			{
				//Store one more
				last_disconnect = t0;
			}

			//Update uptime value
			temp_time_value = t0 - last_disconnect;
			current_uptime = ((double)(temp_time_value))/60.0;	//Put into minutes
		}
		else //Must be out of service
		{
			if (last_disconnect != prev_NTime)	//Just disconnected
			{
				temp_time_value = t0 - last_disconnect;
				previous_uptime = ((double)(temp_time_value))/60.0;	//Put into minutes - automatically shift into prev
			}
			//Default else - already out of service
			current_uptime = -1.0;	//Flag current
			last_disconnect = t0;	//Update tracking variable
		}//End out of service
	}//End disconnect uptime counter

	//Initial phase check - moved so all methods check now
	if (prev_NTime==0)	//Should only be the very first run
	{
		set phase_to_check;
		phase_to_check = (phases & (~(PHASE_D | PHASE_N)));
		
		//See if everything has a source
		if (((phase_to_check & busphasesIn) != phase_to_check) && (busphasesIn != 0) && (solver_method != SM_NR))	//Phase mismatch (and not top level node)
		{
			GL_THROW("node:%d (%s) has more phases leaving than entering",obj->id,obj->name);
			/* TROUBLESHOOT
			A node has more phases present than it has sources coming in.  Under the Forward-Back sweep algorithm,
			the system should be strictly radial.  This scenario implies either a meshed system or unconnected
			phases between the from and to nodes of a connected line.  Please adjust the phases appropriately.  Also
			be sure no open switches are the sole connection for a phase, else this will fail as well.  In a few NR
			circumstances, this can also be seen if the "from" and "to" nodes are in reverse order - the "from" node
			of a link object should be nearest the SWING node, or the node with the most phases - this error check
			will be updated in future versions.
			*/
		}

		if (((phase_to_check & (busphasesIn | busphasesOut) != phase_to_check) && (busphasesIn != 0 && busphasesOut != 0) && (solver_method == SM_NR)))
		{
			gl_error("node:%d (%s) has more phases leaving than entering",obj->id,obj->name);
			/* TROUBLESHOOT
			A node has more phases present than it has sources coming in.  This scenario implies an unconnected
			phases between the from and to nodes of a connected line.  Please adjust the phases appropriately.  Also
			be sure no open switches are the sole connection for a phase, else this will fail as well.  In a few NR
			circumstances, this can also be seen if the "from" and "to" nodes are in reverse order - the "from" node
			of a link object should be nearest the SWING node, or the node with the most phases - this error check
			will be updated in future versions.
			*/

			//This used to be a throw, so make us fail (invalid just lets it finish the step)
			return TS_INVALID;
		}

		//Deltamode check - let every object do it, for giggles
		if (enable_subsecond_models)
		{
			if (solver_method != SM_NR)
			{
				GL_THROW("deltamode simulations only support powerflow in Newton-Raphson (NR) mode at this time");
				/*  TROUBLESHOOT
				deltamode and dynamics simulations will only work with the powerflow module in Newton-Raphson mode.
				Please swap to that solver and try again.
				*/
			}
		}

		//Special FBS code
		if ((solver_method==SM_FBS) && !FBS_swing_set)
		{
			//We're the first one in, we must be the SWING - set us as such
			bustype=SWING;

			//Deflag us
			FBS_swing_set=true;
		}
	}

	if (solver_method==SM_NR)
	{
		if (prev_NTime==0)	//First run, if we are a child, make sure no one linked us before we knew that
		{
			//Make sure an overall SWING bus exists
			if (NR_swing_bus==nullptr)
			{
				GL_THROW("NR: no swing bus found or specified");
				/*	TROUBLESHOOT
				Newton-Raphson failed to automatically assign a swing bus to the node (unchilded nodes are referenced
				to the swing bus).  This should have been detected by this point and represents a bug in the solver.
				Please submit a bug report detailing how you obtained this message.
				*/
			}

			if (((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) != 0) && (NR_connected_links[0] > 0))
			{
				GL_THROW("NR: node:%d - %s - extra links acquired outside of init routine",obj->id,(obj->name?obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to initialize a childed node for the NR solver, extra links that were not detected in the node::init
				routine were discovered.  This should not have happen.  Please submit your GLM and a bug report to the issues tracker.
				*/
			}

			//See if we need to alloc our child space
			if (NR_number_child_nodes[0] > 0)	//Malloc it up
			{
				NR_child_nodes = (node**)gl_malloc(NR_number_child_nodes[0] * sizeof(node*));

				//Make sure it worked
				if (NR_child_nodes == nullptr)
				{
					GL_THROW("NR: Failed to allocate child node pointing space");
					/*  TROUBLESHOOT
					While attempting to allocate memory for child node connections, something failed.
					Please try again.  If the error persists, please submit your code and a bug report
					via the trac website.
					*/
				}

				//Ensure the pointer is zerod
				NR_number_child_nodes[1] = 0;
			}

			//Rank wise, children should be firing after the above malloc is done - link them beasties up
			if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) != 0)
			{
				//Link the parental
				node *parNode = OBJECTDATA(SubNodeParent,node);

				WRITELOCK_OBJECT(SubNodeParent);	//Lock

				//Check and see if we're a house-triplex.  If so, flag our parent so NR works - just draconian write (won't hurt anything)
				if (house_present)
				{
					parNode->house_present=true;
				}

				//Make sure there's still room
				if (parNode->NR_number_child_nodes[1]>=parNode->NR_number_child_nodes[0])
				{
					gl_error("NR: %s tried to parent to a node that has too many children already",obj->name);
					/*  TROUBLESHOOT
					While trying to link a child to its parent, a memory overrun was encountered.  Please try
					again.  If the error persists, please submit you code and a bug report via the trac website.
					*/

					WRITEUNLOCK_OBJECT(SubNodeParent);	//Unlock

					return TS_INVALID;
				}
				else	//There's space
				{
					//Link us
					parNode->NR_child_nodes[parNode->NR_number_child_nodes[1]] = OBJECTDATA(obj,node);

					//Accumulate the index
					parNode->NR_number_child_nodes[1]++;
				}

				WRITEUNLOCK_OBJECT(SubNodeParent);	//Unlock
			}
		}

		if (NR_busdata==nullptr || NR_branchdata==nullptr)	//First time any NR in (this should be the swing bus doing this)
		{
			if ( NR_swing_bus!=obj) WRITELOCK_OBJECT(NR_swing_bus);	//Lock Swing for flag

			NR_busdata = (BUSDATA *)gl_malloc(NR_bus_count * sizeof(BUSDATA));
			if (NR_busdata==nullptr)
			{
				gl_error("NR: memory allocation failure for bus table");
				/*	TROUBLESHOOT
				This is a bug.  GridLAB-D failed to allocate memory for the bus table for Newton-Raphson.
				Please submit this bug and your code.
				*/

				//Unlock the swing bus
				if ( NR_swing_bus!=obj) WRITEUNLOCK_OBJECT(NR_swing_bus);

				return TS_INVALID;
			}
			NR_curr_bus = 0;	//Pull pointer off flag so other objects know it's built

			//initialize the bustype - will be used for detection
			for (index_val=0; index_val<NR_bus_count; index_val++)
				NR_busdata[index_val].type = -1;

			NR_branchdata = (BRANCHDATA *)gl_malloc(NR_branch_count * sizeof(BRANCHDATA));
			if (NR_branchdata==nullptr)
			{
				gl_error("NR: memory allocation failure for branch table");
				/*	TROUBLESHOOT
				This is a bug.  GridLAB-D failed to allocate memory for the link table for Newton-Raphson.
				Please submit this bug and your code.
				*/

				//Unlock the swing bus
				if ( NR_swing_bus!=obj) WRITEUNLOCK_OBJECT(NR_swing_bus);

				return TS_INVALID;
			}
			NR_curr_branch = 0;	//Pull pointer off flag so other objects know it's built

			//Initialize the from - will be used for detection
			for (index_val=0; index_val<NR_branch_count; index_val++)
				NR_branchdata[index_val].from = -1;

			//Allocate deltamode stuff as well
			if (enable_subsecond_models)	//If enabled in powerflow, swing bus rules them all
			{
				//Make sure no one else has done it yet
				if ((pwr_object_current == -1) || (delta_objects==nullptr))
				{
					//Allocate the deltamode object array
					delta_objects = (OBJECT**)gl_malloc(pwr_object_count*sizeof(OBJECT*));

					//Make sure it worked
					if (delta_objects == nullptr)
					{
						GL_THROW("Failed to allocate deltamode objects array for powerflow module!");
						/*  TROUBLESHOOT
						While attempting to create a reference array for powerflow module deltamode-enabled
						objects, an error was encountered.  Please try again.  If the error persists, please
						submit your code and a bug report via the trac website.
						*/
					}

					//Allocate the function reference list as well
					delta_functions = (FUNCTIONADDR*)gl_malloc(pwr_object_count*sizeof(FUNCTIONADDR));

					//Make sure it worked
					if (delta_functions == nullptr)
					{
						GL_THROW("Failed to allocate deltamode objects function array for powerflow module!");
						/*  TROUBLESHOOT
						While attempting to create a reference array for powerflow module deltamode-enabled
						objects, an error was encountered.  Please try again.  If the error persists, please
						submit your code and a bug report via the trac website.
						*/
					}

					//Allocate the post update array
					post_delta_functions = (FUNCTIONADDR*)gl_malloc(pwr_object_count*sizeof(FUNCTIONADDR));

					//Make sure it worked
					if (post_delta_functions == nullptr)
					{
						GL_THROW("Failed to allocate deltamode objects function array for powerflow module!");
						//Defined above
					}

					//Initialize index
					pwr_object_current = 0;
				}

				//See if we are the swing bus AND our flag is enabled, otherwise warn
				if ((obj == NR_swing_bus) && !deltamode_inclusive)
				{
					gl_warning("SWING bus:%s is not flagged for deltamode",obj->name);
					/*  TROUBLESHOOT
					Deltamode is enabled for the overall powerflow module, but not for the SWING node
					of the powerflow.  This means the overall powerflow is not being calculated or included
					in any deltamode calculations.  If this is not a desired run method, please enable the
					deltamode_inclusive flag on the SWING node.
					*/
				}
			}

			//Open the NR dump file, if needed (set handle and empty it)
			if (NRMatDumpMethod != MD_NONE)
			{
				//Make sure the file name isn't empty either
				if (MDFileName[0] == '\0')
				{
					gl_warning("NR: A matrix dump was requested, but no filename was specified. No dump will occur");
					/*  TROUBLESHOOT
					The powerflow module was configured such that a Newton-Raphson-based matrix dump was desired.  However, no file
					was specified in NR_matrix_file.  No output will be performed.
					*/

					//Set us to none
					NRMatDumpMethod = MD_NONE;
				}
				else	//Valid, just empty us
				{
					//Get a file handle and empty the file
					NRFileDump = fopen(MDFileName,"wt");

					//Print a header
					fprintf(NRFileDump,"Note: All indices are zero-referenced.\n\n");

					//Close it, now that it's empty
					fclose(NRFileDump);
				}
			}
			//Default else - no file output desired, so don't make one

			//Unlock the swing bus
			if ( NR_swing_bus!=obj) WRITEUNLOCK_OBJECT(NR_swing_bus);

			if (obj == NR_swing_bus)	//Make sure we're the great MASTER SWING, as a final check
			{
				NR_populate();		//Force a first population via the swing bus.  Not really necessary, but I'm saying it has to be this way.
				NR_admit_change = true;	//Ensure the admittance update variable is flagged
				NR_swing_bus_reference = NR_node_reference;	//Populate our reference too -- pretty good chance it is 0, but just to be safe
			}
			else
			{
				GL_THROW("NR: An order requirement has been violated");
				/*  TROUBLESHOOT
				When initializing the solver, the swing bus should be initialized first for
				Newton-Raphson.  If this does not happen, unexpected results can occur.  Try moving
				the SWING bus to the top of your file.  If the bug persists, submit your code and
				a bug report via the trac website.
				*/
			}
		}//End busdata and branchdata null (first in)

		//Populate individual object references into deltamode, if needed
		if (deltamode_inclusive && enable_subsecond_models && (prev_NTime==0))
		{
			int temp_pwr_object_current;

			//Check limits first
			if (pwr_object_current>=pwr_object_count)
			{
				GL_THROW("Too many objects tried to populate deltamode objects array in the powerflow module!");
				/*  TROUBLESHOOT
				While attempting to populate a reference array of deltamode-enabled objects for the powerflow
				module, an attempt was made to write beyond the allocated array space.  Please try again.  If the
				error persists, please submit a bug report and your code via the trac website.
				*/
			}

			//Lock the SWING bus and get us a value
			if ( NR_swing_bus!=obj) WRITELOCK_OBJECT(NR_swing_bus);	//Lock Swing for flag

				//Get the value
				temp_pwr_object_current = pwr_object_current;

				//Increment
				pwr_object_current++;

			//Unlock
			if ( NR_swing_bus!=obj) WRITEUNLOCK_OBJECT(NR_swing_bus);	//Lock Swing for flag

			//Add us into the list
			delta_objects[temp_pwr_object_current] = obj;

			//Map up the function
			delta_functions[temp_pwr_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"interupdate_pwr_object"));

			//Make sure it worked
			if (delta_functions[temp_pwr_object_current] == nullptr)
			{
				gl_warning("Failure to map deltamode function for device:%s",obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  This error may simply be an indication that the object of interest
				does not support deltamode.  If the error persists and the object does, please submit your code and
				a bug report via the trac website.
				*/
			}

			//Map up the post update, if we have one
			post_delta_functions[temp_pwr_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"postupdate_pwr_object"));

			//No null check, since this one just may not work (post update may not exist)
		}//end deltamode allocations

		//Call NR presync function
		temp_time_value = NR_node_presync_fxn(t0);

		//Copy return value for checking
		temp_t1_value = t1;

		//If we returned anything beyond TS_NEVER, see how it stacks up to t1
		if ((temp_t1_value != TS_NEVER) && (temp_time_value != TS_NEVER))
		{
			//See which is smaller - do manual, since abs seems to confuse things
			if (temp_t1_value < 0)
			{
				if (-temp_t1_value < temp_time_value)
				{
					t1 = temp_t1_value;
				}
				else
				{
					t1 = -temp_time_value;
				}
			}
			else	//Not negative
			{
				if (temp_t1_value < temp_time_value)
				{
					t1 = -temp_t1_value;
				}
				else
				{
					t1 = -temp_time_value;
				}
			}
		}
		else if (temp_t1_value != TS_NEVER)
		{
			t1 = temp_t1_value;	//Assume negated in node, if appropriate
		}
		else if (temp_time_value != TS_NEVER)
		{
			t1 = -temp_time_value;
		}
		else	//Implies both are TS_NEVER
		{
			t1 = TS_NEVER;
		}
	}//end solver_NR call
	else if (solver_method==SM_FBS)
	{
#ifdef SUPPORT_OUTAGES
		if (condition!=OC_NORMAL)	//We're in an abnormal state
		{
			voltage[0] = voltage[1] = voltage[2] = 0.0;	//Zero the voltages
			condition = OC_NORMAL;	//Clear the flag in case we're a switch
		}
#endif
		/* reset the current accumulator */
		current_inj[0] = current_inj[1] = current_inj[2] = gld::complex(0,0);

		/* record the last sync voltage */
		last_voltage[0] = voltage[0];
		last_voltage[1] = voltage[1];
		last_voltage[2] = voltage[2];

		/* get frequency from reference bus */
		if (reference_bus!=nullptr)
		{
			node *pRef = OBJECTDATA(reference_bus,node);
			frequency = pRef->frequency;
		}
	}

	return t1;
}

//Functionalized sync pass routines for NR solver
//Put in place so deltamode can call it and properly udpate
void node::NR_node_sync_fxn(OBJECT *obj)
{
	int loop_index_var;
	STATUS fxn_ret_value;

	//Reliability check - sets and removes voltages (theory being previous answer better than starting at 0)
	unsigned char phase_checks_var;

	//See if we've been initialized or not
	if (NR_node_reference!=-1)
	{
		//Do a check to see if we need to be "islanded reset" -- do this to re-enable phases, if needed
		if (reset_island_state && NR_island_fail_method)
		{
			//Call the reset function - easier this way -- don't really care about the status here -- warning messages will suffice for this call
			fxn_ret_value = reset_node_island_condition();
		}
		//Default else -- our current state is acceptable


		//Phase check/voltage zero - mostly so fuses behave properly (postsync one captures most items, but restored fuses
		//break things here)
		//Make sure we're a real boy - if we're not, do nothing (we'll steal mommy's or daddy's voltages in postsync)
		if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
		{
			//Figre out what has changed
			phase_checks_var = ((NR_busdata[NR_node_reference].phases ^ prev_phases) & 0x8F);

			if (phase_checks_var != 0x00)	//Something's changed
			{
				//See if it is a triplex
				if ((NR_busdata[NR_node_reference].origphases & 0x80) == 0x80)
				{
					//See if A, B, or C appeared, or disappeared
					if ((NR_busdata[NR_node_reference].phases & 0x80) == 0x00)	//No phases means it was just removed
					{
						//Store V1 and V2
						last_voltage[0] = voltage[0];
						last_voltage[1] = voltage[1];
						last_voltage[2] = voltage[2];

						//Clear them out
						voltage[0] = 0.0;
						voltage[1] = 0.0;
						voltage[2] = 0.0;
					}
					else	//Put back in service
					{
						//See if in-rush is enabled or not
						if (!enable_inrush_calculations)
						{
							voltage[0] = last_voltage[0];
							voltage[1] = last_voltage[1];
							voltage[2] = last_voltage[2];
						}
						else	//When restoring from inrush, a very small term is needed (or nothing happens)
						{
							voltage[0] = last_voltage[0] * MULTTERM;
							voltage[1] = last_voltage[1] * MULTTERM;
							voltage[2] = last_voltage[2] * MULTTERM;
						}

						//Default else - leave it be and just leave them to zero (more fun!)
					}

					//Recalculated V12, V1N, V2N in case a child uses them
					voltaged[0] = voltage[0] + voltage[1];	//12
					voltaged[1] = voltage[1] - voltage[2];	//2N
					voltaged[2] = voltage[0] - voltage[2];	//1N -- unsure why odd

				}//End triplex
				else	//Nope
				{
					//Find out changes, and direction
					if ((phase_checks_var & 0x04) == 0x04)	//Phase A change
					{
						//See which way
						if ((prev_phases & 0x04) == 0x04)	//Means A just disappeared
						{
							last_voltage[0] = voltage[0];	//Store the last value
							voltage[0] = 0.0;				//Put us to zero, so volt_dump is happy
						}
						else	//A just came back
						{
							if (!enable_inrush_calculations)
							{
								voltage[0] = last_voltage[0];	//Read in the previous values
							}
							else	//When restoring from inrush, a very small term is needed (or nothing happens)
							{
								voltage[0] = last_voltage[0] * MULTTERM;
							}

							//Default else -- in-rush enabled, just leave as they were
						}
					}//End Phase A change

					//Find out changes, and direction
					if ((phase_checks_var & 0x02) == 0x02)	//Phase B change
					{
						//See which way
						if ((prev_phases & 0x02) == 0x02)	//Means B just disappeared
						{
							last_voltage[1] = voltage[1];	//Store the last value
							voltage[1] = 0.0;				//Put us to zero, so volt_dump is happy
						}
						else	//B just came back
						{
							if (!enable_inrush_calculations)
							{
								voltage[1] = last_voltage[1];	//Read in the previous values
							}
							else	//When restoring from inrush, a very small term is needed (or nothing happens)
							{
								voltage[1] = last_voltage[1] * MULTTERM;
							}

							//Default else - in-rush enabled, we want these to be zero
						}
					}//End Phase B change

					//Find out changes, and direction
					if ((phase_checks_var & 0x01) == 0x01)	//Phase C change
					{
						//See which way
						if ((prev_phases & 0x01) == 0x01)	//Means C just disappeared
						{
							last_voltage[2] = voltage[2];	//Store the last value
							voltage[2] = 0.0;				//Put us to zero, so volt_dump is happy
						}
						else	//C just came back
						{
							if (!enable_inrush_calculations)
							{
								voltage[2] = last_voltage[2];	//Read in the previous values
							}
							else	//When restoring from inrush, a very small term is needed (or nothing happens)
							{
								voltage[2] = last_voltage[2] * MULTTERM;
							}

							//Default else - in-rush enabled and want them zero
						}
					}//End Phase C change

					//Recalculated VAB, VBC, and VCA, in case a child uses them
					voltaged[0] = voltage[0] - voltage[1];
					voltaged[1] = voltage[1] - voltage[2];
					voltaged[2] = voltage[2] - voltage[0];
				}//End not triplex

				//Assign current value in
				prev_phases = NR_busdata[NR_node_reference].phases;
			}//End Phase checks for reliability
		}//End normal node

		if ((SubNode & SNT_CHILD)==SNT_CHILD)
		{
			//Post our loads up to our parent
			node *ParToLoad = OBJECTDATA(SubNodeParent,node);

			//Lock the parent for accumulation
			LOCK_OBJECT(SubNodeParent);

			//Import power and "load" characteristics
			ParToLoad->power[0]+=power[0]-last_child_power[0][0];
			ParToLoad->power[1]+=power[1]-last_child_power[0][1];
			ParToLoad->power[2]+=power[2]-last_child_power[0][2];

			ParToLoad->shunt[0]+=shunt[0]-last_child_power[1][0];
			ParToLoad->shunt[1]+=shunt[1]-last_child_power[1][1];
			ParToLoad->shunt[2]+=shunt[2]-last_child_power[1][2];

			ParToLoad->current[0]+=current[0]-last_child_power[2][0];
			ParToLoad->current[1]+=current[1]-last_child_power[2][1];
			ParToLoad->current[2]+=current[2]-last_child_power[2][2];

			ParToLoad->pre_rotated_current[0] += pre_rotated_current[0]-last_child_power[3][0];
			ParToLoad->pre_rotated_current[1] += pre_rotated_current[1]-last_child_power[3][1];
			ParToLoad->pre_rotated_current[2] += pre_rotated_current[2]-last_child_power[3][2];

			//And the deltamode accumulators too -- if deltamode
			//Only do this if we are a dynamic norton child - otherwise may do extra
			if (deltamode_inclusive && dynamic_norton)
			{
				//Pull our parent value down -- everything is being pushed up there anyways
				//Pulling to keep meters accurate (theoretically)
				deltamode_dynamic_current[0] = ParToLoad->deltamode_dynamic_current[0];
				deltamode_dynamic_current[1] = ParToLoad->deltamode_dynamic_current[1];
				deltamode_dynamic_current[2] = ParToLoad->deltamode_dynamic_current[2];
			}

			//Do the same for the explicit delta/wye loads - last_child_power is set up as columns of ZIP, not ABC
			for (loop_index_var=0; loop_index_var<6; loop_index_var++)
			{
				ParToLoad->power_dy[loop_index_var] += power_dy[loop_index_var] - last_child_power_dy[loop_index_var][0];
				ParToLoad->shunt_dy[loop_index_var] += shunt_dy[loop_index_var] - last_child_power_dy[loop_index_var][1];
				ParToLoad->current_dy[loop_index_var] += current_dy[loop_index_var] - last_child_power_dy[loop_index_var][2];
			}

			if (has_phase(PHASE_S))	//Triplex gets another term as well
			{
				ParToLoad->current12 +=current12-last_child_current12;
			}

			//See if we have a house!
			if (house_present)	//Add our values into our parent's accumulator!
			{
				ParToLoad->nom_res_curr[0] += nom_res_curr[0];
				ParToLoad->nom_res_curr[1] += nom_res_curr[1];
				ParToLoad->nom_res_curr[2] += nom_res_curr[2];
			}

			//Unlock the parent now that we are done
			UNLOCK_OBJECT(SubNodeParent);

			//Update previous power tracker
			last_child_power[0][0] = power[0];
			last_child_power[0][1] = power[1];
			last_child_power[0][2] = power[2];

			last_child_power[1][0] = shunt[0];
			last_child_power[1][1] = shunt[1];
			last_child_power[1][2] = shunt[2];

			last_child_power[2][0] = current[0];
			last_child_power[2][1] = current[1];
			last_child_power[2][2] = current[2];

			last_child_power[3][0] = pre_rotated_current[0];
			last_child_power[3][1] = pre_rotated_current[1];
			last_child_power[3][2] = pre_rotated_current[2];

			//Do the same for delta/wye explicit loads
			for (loop_index_var=0; loop_index_var<6; loop_index_var++)
			{
				last_child_power_dy[loop_index_var][0] = power_dy[loop_index_var];
				last_child_power_dy[loop_index_var][1] = shunt_dy[loop_index_var];
				last_child_power_dy[loop_index_var][2] = current_dy[loop_index_var];
			}

			if (has_phase(PHASE_S))		//Triplex extra current update
				last_child_current12 = current12;
		}

		//Accumulations for "differently connected nodes" is still basically the same for delta/wye combination loads
		if ((SubNode & SNT_DIFF_CHILD)==SNT_DIFF_CHILD)
		{
			//Post our loads up to our parent - in the appropriate fashion
			node *ParToLoad = OBJECTDATA(SubNodeParent,node);

			//Lock the parent for accumulation
			LOCK_OBJECT(SubNodeParent);

			//Update post them.  Row 1 is power, row 2 is admittance, row 3 is current
			ParToLoad->Extra_Data[0] += power[0];
			ParToLoad->Extra_Data[1] += power[1];
			ParToLoad->Extra_Data[2] += power[2];

			ParToLoad->Extra_Data[3] += shunt[0];
			ParToLoad->Extra_Data[4] += shunt[1];
			ParToLoad->Extra_Data[5] += shunt[2];

			ParToLoad->Extra_Data[6] += current[0];
			ParToLoad->Extra_Data[7] += current[1];
			ParToLoad->Extra_Data[8] += current[2];

			//Add in the unrotated stuff too -- it should never be subject to "connectivity"
			ParToLoad->pre_rotated_current[0] += pre_rotated_current[0];
			ParToLoad->pre_rotated_current[1] += pre_rotated_current[1];
			ParToLoad->pre_rotated_current[2] += pre_rotated_current[2];

			//And the deltamode accumulators too -- if deltamode
			//Only do this if we are a dynamic norton child - otherwise may do extra
			if (deltamode_inclusive && dynamic_norton)
			{
				//Pull our parent value down -- everything is being pushed up there anyways
				//Pulling to keep meters accurate (theoretically)
				deltamode_dynamic_current[0] = ParToLoad->deltamode_dynamic_current[0];
				deltamode_dynamic_current[1] = ParToLoad->deltamode_dynamic_current[1];
				deltamode_dynamic_current[2] = ParToLoad->deltamode_dynamic_current[2];
			}

			//Import power and "load" characteristics for explicit delta/wye portions
			for (loop_index_var=0; loop_index_var<6; loop_index_var++)
			{
				ParToLoad->power_dy[loop_index_var] += power_dy[loop_index_var];
				ParToLoad->shunt_dy[loop_index_var] += shunt_dy[loop_index_var];
				ParToLoad->current_dy[loop_index_var] += current_dy[loop_index_var];
			}

			//Finished, unlock parent
			UNLOCK_OBJECT(SubNodeParent);

			//Update our tracking variable
			for (loop_index_var=0; loop_index_var<6; loop_index_var++)
			{
				last_child_power_dy[loop_index_var][0] = power_dy[loop_index_var];
				last_child_power_dy[loop_index_var][1] = shunt_dy[loop_index_var];
				last_child_power_dy[loop_index_var][2] = current_dy[loop_index_var];
			}

			//Store the unrotated too
			last_child_power[3][0] = pre_rotated_current[0];
			last_child_power[3][1] = pre_rotated_current[1];
			last_child_power[3][2] = pre_rotated_current[2];
		}//End differently connected child

		//Call the VFD update, if we need it
		//Note -- this basically precludes childed nodes from working, which is acceptable (programmer says so)
		if (VFD_attached)
		{
			//Call the function - make the VFD move us along
			fxn_ret_value = ((STATUS (*)(OBJECT *))(*VFD_updating_function))(VFD_object);

			//Check the return value
			if (fxn_ret_value == FAILED)
			{
				GL_THROW("node:%d - %s -- Failed VFD updating function",obj->id,(obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to call the VFD current injection function, an error occurred.  Please try again.
				If the error persists, please submit an issue.
				*/
			}
		}
	}//end not uninitialized
}

TIMESTAMP node::sync(TIMESTAMP t0)
{
	TIMESTAMP t1 = powerflow_object::sync(t0);
	OBJECT *obj = OBJECTHDR(this);
	gld::complex delta_current[3];
	gld::complex power_current[3];
	gld::complex delta_shunt[3];
	gld::complex delta_shunt_curr[3];
	gld::complex dy_curr_accum[3];
	gld::complex temp_current_val[3];
	char loop_index_val;
	gld::complex temp_curr_rotate_value, temp_curr_calc_value;
	gld_property *temp_property;
	
	//Final initialization issue - has to be here, or childed deltamode stuff fails
	//This catches any orphaned/islanded single nodes (that link wouldn't catch), so error checks work
	//and don't segfault things - needs to be duplicated to subclass objects that reference NR before node::sync is called
	if ((prev_NTime==0) && (solver_method == SM_NR))
	{
		//See if we've been initialized yet - links typically do this, but single buses get missed
		if (NR_node_reference == -1)
		{
			//Call the populate routine
			NR_populate();
		}
		else	//Initialized, so SWING or child
		{
			//Check our status - make sure we're a child
			if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) != 0)
			{
				//Only do admittance allocation if we're a Norton
				if (dynamic_norton)
				{
					//Make sure no pesky children have already allocated us
					if (full_Y == nullptr)
					{
						//Allocate it
						full_Y = (gld::complex *)gl_malloc(9*sizeof(gld::complex));

						//Check it
						if (full_Y==nullptr)
						{
							GL_THROW("Node:%s failed to allocate space for the a deltamode variable",obj->name);
							//Defined elsewhere
						}

						//See if our published matrix has anything in it
						if ((full_Y_matrix.get_rows() == 3) && (full_Y_matrix.get_cols() == 3))
						{
							full_Y[0] = full_Y_matrix.get_at(0,0);
							full_Y[1] = full_Y_matrix.get_at(0,1);
							full_Y[2] = full_Y_matrix.get_at(0,2);

							full_Y[3] = full_Y_matrix.get_at(1,0);
							full_Y[4] = full_Y_matrix.get_at(1,1);
							full_Y[5] = full_Y_matrix.get_at(1,2);

							full_Y[6] = full_Y_matrix.get_at(2,0);
							full_Y[7] = full_Y_matrix.get_at(2,1);
							full_Y[8] = full_Y_matrix.get_at(2,2);
						}
						else
						{
							//Zero it, just to be safe (gens will accumulate into it)
							full_Y[0] = full_Y[1] = full_Y[2] = gld::complex(0.0,0.0);
							full_Y[3] = full_Y[4] = full_Y[5] = gld::complex(0.0,0.0);
							full_Y[6] = full_Y[7] = full_Y[8] = gld::complex(0.0,0.0);
						}
					}
					//Default else - must somehow already be allocated
				}//End dynamic Norton
			}//End is a child
			//Default else - probably a SWING
		}//End SWING or child
	}//First run NR

	//Generic time keeping variable - used for phase checks (GS does this explicitly below)
	if (t0!=prev_NTime)
	{
		//Update time tracking variable
		prev_NTime=t0;
	}

	switch (solver_method)
	{
	case SM_FBS:
		{
		if (phases&PHASE_S)
		{	// Split phase
			gld::complex temp_inj[2];
			gld::complex adjusted_curr[3];
			gld::complex temp_curr_val[3];

			if (house_present)
			{
				//Update phase adjustments
				adjusted_curr[0].SetPolar(1.0,voltage[0].Arg());	//Pull phase of V1
				adjusted_curr[1].SetPolar(1.0,voltage[1].Arg());	//Pull phase of V2
				adjusted_curr[2].SetPolar(1.0,voltaged[0].Arg());	//Pull phase of V12

				//Update these current contributions
				temp_curr_val[0] = nom_res_curr[0]/(~adjusted_curr[0]);		//Just denominator conjugated to keep math right (rest was conjugated in house)
				temp_curr_val[1] = nom_res_curr[1]/(~adjusted_curr[1]);
				temp_curr_val[2] = nom_res_curr[2]/(~adjusted_curr[2]);
			}
			else
			{
				temp_curr_val[0] = temp_curr_val[1] = temp_curr_val[2] = 0.0;	//No house present, just zero em
			}

#ifdef SUPPORT_OUTAGES
			if (voltage[0]!=0.0)
			{
#endif
			gld::complex d1 = (voltage1.IsZero() || (power1.IsZero() && shunt1.IsZero())) ? (current1 + temp_curr_val[0]) : (current1 + ~(power1/voltage1) + voltage1*shunt1 + temp_curr_val[0]);
			gld::complex d2 = ((voltage1+voltage2).IsZero() || (power12.IsZero() && shunt12.IsZero())) ? (current12 + temp_curr_val[2]) : (current12 + ~(power12/(voltage1+voltage2)) + (voltage1+voltage2)*shunt12 + temp_curr_val[2]);

			current_inj[0] += d1;
			temp_inj[0] = current_inj[0];
			current_inj[0] += d2;

#ifdef SUPPORT_OUTAGES
			}
			else
			{
				temp_inj[0] = 0.0;
				//WRITELOCK_OBJECT(obj);
				current_inj[0]=0.0;
				//UNLOCK_OBJECT(obj);
			}

			if (voltage[1]!=0)
			{
#endif
			d1 = (voltage2.IsZero() || (power2.IsZero() && shunt2.IsZero())) ? (-current2 - temp_curr_val[1]) : (-current2 - ~(power2/voltage2) - voltage2*shunt2 - temp_curr_val[1]);
			d2 = ((voltage1+voltage2).IsZero() || (power12.IsZero() && shunt12.IsZero())) ? (-current12 - temp_curr_val[2]) : (-current12 - ~(power12/(voltage1+voltage2)) - (voltage1+voltage2)*shunt12 - temp_curr_val[2]);

			current_inj[1] += d1;
			temp_inj[1] = current_inj[1];
			current_inj[1] += d2;

#ifdef SUPPORT_OUTAGES
			}
			else
			{
				temp_inj[0] = 0.0;
				//WRITELOCK_OBJECT(obj);
				current_inj[1] = 0.0;
				//UNLOCK_OBJECT(obj);
			}
#endif

			if (obj->parent!=nullptr && gl_object_isa(obj->parent,"triplex_line","powerflow")) {

				//See if we've mapped yet
				if (tn_values == nullptr)
				{
					//Allocate space
					tn_values = (gld::complex *)gl_malloc(2*sizeof(gld::complex));

					//Make sure it worked
					if (tn_values == nullptr)
					{
						GL_THROW("node:%d - %s -- Failed to allocate for triplex current data",obj->id,(obj->name ? obj->name : "Unnamed"));
						/*  TROUBLESHOOT
						While attempting to allocate memory for triplex neutral calculation values, an error occurred.
						Please try again.  If the error persists, please submit you GLM and a report via the issue tracker system.
						*/
					}

					//Map to neutral current properties

					//Get first one
					temp_property = new gld_property(obj->parent,"triplex_neutral_1_value");

					//Make sure it worked
					if (!temp_property->is_valid() || !temp_property->is_complex())
					{
						GL_THROW("node:%d - %s -- Failed to map triplex current data",obj->id,(obj->name ? obj->name : "Unnamed"));
						/* TROUBLESHOOT
						While attempting to map one of the triplex-line-related multiplier properties, an error occurred.
						Please try again.  If the error persists, please submit your GLM into the issue tracker with a description.
						*/
					}

					//Pull the value
					tn_values[0] = temp_property->get_complex();

					//Remove it
					delete temp_property;

					//Get the other one
					//Get first one
					temp_property = new gld_property(obj->parent,"triplex_neutral_2_value");

					//Make sure it worked
					if (!temp_property->is_valid() || !temp_property->is_complex())
					{
						GL_THROW("node:%d - %s -- Failed to map triplex current data",obj->id,(obj->name ? obj->name : "Unnamed"));
						//Defined above
					}

					//Pull the value
					tn_values[1] = temp_property->get_complex();

					//Remove it
					delete temp_property;
				}
				//Default else - already mapped, so go forth

				gld::complex d = tn_values[0]*current_inj[0] + tn_values[1]*current_inj[1];
				current_inj[2] += d;
			}
			else {
				gld::complex d = ((voltage1.IsZero() || (power1.IsZero() && shunt1.IsZero())) ||
								   (voltage2.IsZero() || (power2.IsZero() && shunt2.IsZero()))) 
									? currentN : -(temp_inj[0] + temp_inj[1]);
				current_inj[2] += d;
			}
		}
		else if (has_phase(PHASE_D)) 
		{   // 'Delta' connected load
			
			//Convert delta connected power to appropriate line current
			delta_current[0]= (voltaged[0].IsZero()) ? 0 : ~(power[0]/voltaged[0]);
			delta_current[1]= (voltaged[1].IsZero()) ? 0 : ~(power[1]/voltaged[1]);
			delta_current[2]= (voltaged[2].IsZero()) ? 0 : ~(power[2]/voltaged[2]);

			power_current[0]=delta_current[0]-delta_current[2];
			power_current[1]=delta_current[1]-delta_current[0];
			power_current[2]=delta_current[2]-delta_current[1];

			//Convert delta connected load to appropriate line current
			delta_shunt[0] = voltaged[0]*shunt[0];
			delta_shunt[1] = voltaged[1]*shunt[1];
			delta_shunt[2] = voltaged[2]*shunt[2];

			delta_shunt_curr[0] = delta_shunt[0]-delta_shunt[2];
			delta_shunt_curr[1] = delta_shunt[1]-delta_shunt[0];
			delta_shunt_curr[2] = delta_shunt[2]-delta_shunt[1];

			//Convert delta-current into a phase current - reuse temp variable
			delta_current[0]=current[0]-current[2];
			delta_current[1]=current[1]-current[0];
			delta_current[2]=current[2]-current[1];

#ifdef SUPPORT_OUTAGES
			for (char kphase=0;kphase<3;kphase++)
			{
				if (voltaged[kphase]==0.0)
				{
					//WRITELOCK_OBJECT(obj);
					current_inj[kphase] = 0.0;
					//UNLOCK_OBJECT(obj);
				}
				else
				{
					//WRITELOCK_OBJECT(obj);
					current_inj[kphase] += delta_current[kphase] + power_current[kphase] + delta_shunt_curr[kphase];
					//UNLOCK_OBJECT(obj);
				}
			}
#else
			temp_current_val[0] = delta_current[0] + power_current[0] + delta_shunt_curr[0];
			temp_current_val[1] = delta_current[1] + power_current[1] + delta_shunt_curr[1];
			temp_current_val[2] = delta_current[2] + power_current[2] + delta_shunt_curr[2];

			current_inj[0] += temp_current_val[0];
			current_inj[1] += temp_current_val[1];
			current_inj[2] += temp_current_val[2];
#endif
		}
		else
		{	// 'WYE' connected load

#ifdef SUPPORT_OUTAGES
			for (char kphase=0;kphase<3;kphase++)
			{
				if (voltage[kphase]==0.0)
				{
					//WRITELOCK_OBJECT(obj);
					current_inj[kphase] = 0.0;
					//UNLOCK_OBJECT(obj);
				}
				else
				{
					gld::complex d = ((voltage[kphase]==0.0) || ((power[kphase] == 0) && shunt[kphase].IsZero())) ? current[kphase] : current[kphase] + ~(power[kphase]/voltage[kphase]) + voltage[kphase]*shunt[kphase];
					//WRITELOCK_OBJECT(obj);
					current_inj[kphase] += d;
					//UNLOCK_OBJECT(obj);
				}
			}
#else

			temp_current_val[0] = (voltage[0].IsZero() || (power[0].IsZero() && shunt[0].IsZero())) ? current[0] : current[0] + ~(power[0]/voltage[0]) + voltage[0]*shunt[0];
			temp_current_val[1] = (voltage[1].IsZero() || (power[1].IsZero() && shunt[1].IsZero())) ? current[1] : current[1] + ~(power[1]/voltage[1]) + voltage[1]*shunt[1];
			temp_current_val[2] = (voltage[2].IsZero() || (power[2].IsZero() && shunt[2].IsZero())) ? current[2] : current[2] + ~(power[2]/voltage[2]) + voltage[2]*shunt[2];

			current_inj[0] += temp_current_val[0];
			current_inj[1] += temp_current_val[1];
			current_inj[2] += temp_current_val[2];
#endif
		}

		//Handle explicit delta-wye connections now -- no triplex
		if (!(has_phase(PHASE_S)))
		{
			//Convert delta connected power to appropriate line current
			delta_current[0]= (voltageAB.IsZero()) ? 0 : ~(power_dy[0]/voltageAB);
			delta_current[1]= (voltageBC.IsZero()) ? 0 : ~(power_dy[1]/voltageBC);
			delta_current[2]= (voltageCA.IsZero()) ? 0 : ~(power_dy[2]/voltageCA);

			power_current[0]=delta_current[0]-delta_current[2];
			power_current[1]=delta_current[1]-delta_current[0];
			power_current[2]=delta_current[2]-delta_current[1];

			//Convert delta connected load to appropriate line current
			delta_shunt[0] = voltageAB*shunt_dy[0];
			delta_shunt[1] = voltageBC*shunt_dy[1];
			delta_shunt[2] = voltageCA*shunt_dy[2];

			delta_shunt_curr[0] = delta_shunt[0]-delta_shunt[2];
			delta_shunt_curr[1] = delta_shunt[1]-delta_shunt[0];
			delta_shunt_curr[2] = delta_shunt[2]-delta_shunt[1];

			//Convert delta-current into a phase current - reuse temp variable
			delta_current[0]=current_dy[0]-current_dy[2];
			delta_current[1]=current_dy[1]-current_dy[0];
			delta_current[2]=current_dy[2]-current_dy[1];

			//Accumulate
			dy_curr_accum[0] = delta_current[0] + power_current[0] + delta_shunt_curr[0];
			dy_curr_accum[1] = delta_current[1] + power_current[1] + delta_shunt_curr[1];
			dy_curr_accum[2] = delta_current[2] + power_current[2] + delta_shunt_curr[2];

			//Wye-connected portions
			dy_curr_accum[0] += (voltageA.IsZero() || (power_dy[3].IsZero() && shunt_dy[3].IsZero())) ? current_dy[3] : current_dy[3] + ~(power_dy[3]/voltageA) + voltageA*shunt_dy[3];
			dy_curr_accum[1] += (voltageB.IsZero() || (power_dy[4].IsZero() && shunt_dy[4].IsZero())) ? current_dy[4] : current_dy[4] + ~(power_dy[4]/voltageB) + voltageB*shunt_dy[4];
			dy_curr_accum[2] += (voltageC.IsZero() || (power_dy[5].IsZero() && shunt_dy[5].IsZero())) ? current_dy[5] : current_dy[5] + ~(power_dy[5]/voltageC) + voltageC*shunt_dy[5];

			//Accumulate in to final portion
			current_inj[0] += dy_curr_accum[0];
			current_inj[1] += dy_curr_accum[1];
			current_inj[2] += dy_curr_accum[2];

			//Do "non-triplex" houses in here too, since they're explicit
			if (house_present)
			{
				//Do in a loop, just becaus
				for (loop_index_val=0; loop_index_val<3; loop_index_val++)
				{
					//See if it is even a valid voltage first
					if (voltage[loop_index_val].Mag() != 0.0)
					{
						//Get the phase rotation
						temp_curr_rotate_value.SetPolar(1.0,voltage[loop_index_val].Arg());

						//Update the values
						temp_curr_calc_value = nom_res_curr[loop_index_val]/(~temp_curr_rotate_value);	//Just denominator conjugated to keep math right (rest was conjugated in house)

						//Accumulate it, because we like extra steps
						current_inj[loop_index_val] += temp_curr_calc_value;
					}
					//Default else - wasn't valid, so don't accumulate anything
				}//End phase loop
			}//End not-so-triplex house
		}//End delta/wye explicit

#ifdef SUPPORT_OUTAGES
	if (is_open_any())
		throw "unable to handle node open phase condition";

	if (is_contact_any())
	{
		/* phase-phase contact */
		if (is_contact(PHASE_A|PHASE_B|PHASE_C))
			voltageA = voltageB = voltageC = (voltageA + voltageB + voltageC)/3;
		else if (is_contact(PHASE_A|PHASE_B))
			voltageA = voltageB = (voltageA + voltageB)/2;
		else if (is_contact(PHASE_B|PHASE_C))
			voltageB = voltageC = (voltageB + voltageC)/2;
		else if (is_contact(PHASE_A|PHASE_C))
			voltageA = voltageC = (voltageA + voltageC)/2;

		/* phase-neutral/ground contact */
		if (is_contact(PHASE_A|PHASE_N) || is_contact(PHASE_A|GROUND))
			voltageA /= 2;
		if (is_contact(PHASE_B|PHASE_N) || is_contact(PHASE_B|GROUND))
			voltageB /= 2;
		if (is_contact(PHASE_C|PHASE_N) || is_contact(PHASE_C|GROUND))
			voltageC /= 2;
	}
#endif

		// if the parent object is another node
		if (obj->parent!=nullptr && gl_object_isa(obj->parent,"node","powerflow"))
		{
			node *pNode = OBJECTDATA(obj->parent,node);

			//Check to make sure phases are correct - ignore Deltas and neutrals (load changes take care of those)
			if (((pNode->phases & phases) & (!(PHASE_D | PHASE_N))) == (phases & (!(PHASE_D | PHASE_N))))
			{
				// add the injections on this node to the parent
				WRITELOCK_OBJECT(obj->parent);
				pNode->current_inj[0] += current_inj[0];
				pNode->current_inj[1] += current_inj[1];
				pNode->current_inj[2] += current_inj[2];
				WRITEUNLOCK_OBJECT(obj->parent);
			}
			else
				GL_THROW("Node:%d's parent does not have the proper phase connection to be a parent.",obj->id);
				/*  TROUBLESHOOT
				A parent-child relationship was attempted when the parent node does not contain the phases
				of the child node.  Ensure parent nodes have at least the phases of the child object.
				*/
		}

		break;
		}
	case SM_NR:
		{
			//Call NR sync function items
			NR_node_sync_fxn(obj);

#ifdef GLD_USE_EIGEN
			static auto NR_Solver = std::make_unique<NR_Solver_Eigen>();
#endif

			if ((NR_curr_bus==NR_bus_count) && (obj==NR_swing_bus))	//Only run the solver once everything has populated
			{
				bool bad_computation=false;
				NRSOLVERMODE powerflow_type;

				//See if we're the special fault_check mode
				if (fault_check_override_mode)
				{
					//Just return a reiteration time -- fault_check will terminate the simulation
					return t0;
				}

				//Depending on operation mode, call solver appropriately
				if (deltamode_inclusive)	//Dynamics mode, solve the static in a way that generators are handled right
				{
					if (NR_dyn_first_run)	//If it is the first run, perform the initialization powerflow
					{
						powerflow_type = PF_DYNINIT;
						NR_dyn_first_run = false;	//Deflag us for future powerflow solutions
					}
					else	//After first run - call the "dynamic" version of the powerflow solver (SWING bus different)
					{
						powerflow_type = PF_DYNCALC;
					}
				}//End deltamode
				else	//Normal mode
				{
					powerflow_type = PF_NORMAL;
				}

#ifndef GLD_USE_EIGEN
				int64 result = solver_nr(NR_bus_count, NR_busdata, NR_branch_count, NR_branchdata, &NR_powerflow, powerflow_type, nullptr, &bad_computation);
#else
				long result = NR_Solver.solver_nr(NR_bus_count, NR_busdata, NR_branch_count, NR_branchdata, &NR_powerflow, powerflow_type, nullptr, &bad_computation);
#endif

				//De-flag the change - no contention should occur
				NR_admit_change = false;

				if (bad_computation)
				{
					GL_THROW("Newton-Raphson method is unable to converge to a solution at this operation point");
					/*  TROUBLESHOOT
					Newton-Raphson has failed to complete even a single iteration on the powerflow.  This is an indication
					that the method will not solve the system and may have a singularity or other ill-favored condition in the
					system matrices.
					*/
				}
				else if (result<0)	//Failure to converge, but we just let it stay where we are for now
				{
					gl_verbose("Newton-Raphson failed to converge, sticking at same iteration.");
					/*  TROUBLESHOOT
					Newton-Raphson failed to converge in the number of iterations specified in NR_iteration_limit.
					It will try again (if the global iteration limit has not been reached).
					*/
					NR_retval=t0;
				}
				else
				{
					//See if deltamode is enabled
					if (enable_subsecond_models && deltamode_inclusive)
					{
						if (delta_initialize_iterations > 0)	//Reiterate
						{
							//Decrement it
							delta_initialize_iterations--;

							//Reiterate
							NR_retval = t0;
						}
						else
						{
							//Continue
							NR_retval = t1;
						}
					}
					else	//Normal - continue
					{
						NR_retval=t1;
					}
				}

				//See where we wanted to go
				return NR_retval;
			}
			else if (NR_curr_bus==NR_bus_count)	//Population complete, we're not swing, let us go (or we never go on)
				return t1;
			else	//Population of data busses is not complete.  Flag us for a go-around, they should be ready next time
			{
				if (obj == NR_swing_bus)	//Only error on MASTER swing - if errors with others, seems to be upset.  Too lazy to track down why.
				{
					GL_THROW("All nodes were not properly populated");
					/*  TROUBLESHOOT
					The NR solver is still waiting to initialize an object on the second pass.  Everything should have
					initialized on the first pass.  Look for orphaned node objects that do not have a line attached and
					try again.  If the error persists, please submit your code and a bug report via the trac website.
					*/
				}
				else
				{
					//See if we're a disconnected node
					if (NR_node_reference == -1)
					{
						//Output an error (but don't fail) - SWING bus will cause the explicit failure
						gl_error("Unconnected node - %s id:%d",obj->name?obj->name:"Unknown",obj->id);
						/*  TROUBLESHOOT
						While parsing a GLM, the Newton-Raphson powerflow solver encountered a node
						that does not appear to be connected anywhere.  This will cause problems with the
						solver.  Please verify that this node is supposed to be islanded.
						*/
					}

					return t0;
				}
			}
			break;
		}
	default:
		GL_THROW("unsupported solver method");
		/*	TROUBLESHOOT
		An invalid powerflow solver was specified.  Currently acceptable values are FBS for forward-back
		sweep (Kersting's method) and NR for Newton-Raphson.
		*/
		break;
	}
	return t1;
}

//Functionalized postsync pass routines for NR solver and generic items
//Put in place so deltamode can call it and properly udpate
void node::BOTH_node_postsync_fxn(OBJECT *obj)
{
	double curr_delta_time;
	complex_array temp_complex_array;
	int index_x_val, index_y_val;
	gld_property *temp_property = nullptr;
	gld_wlock *test_rlock = nullptr;
	unsigned char phase_checks_var;

	//NR-related updates for reliability and making sure "removed" objects have zero voltage
	//Needs to be in post sync due to when fault_check fires and removes phases
	if (solver_method == SM_NR)
	{
		//Make sure not unreferenced
		if (NR_node_reference!=-1)
		{
			//Make sure we're a real boy - if we're not, do nothing (we'll steal mommy's or daddy's voltages in postsync)
			if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
			{
				//Figre out what has changed
				phase_checks_var = ((NR_busdata[NR_node_reference].phases ^ prev_phases) & 0x8F);

				if (phase_checks_var != 0x00)	//Something's changed
				{
					//See if it is a triplex
					if ((NR_busdata[NR_node_reference].origphases & 0x80) == 0x80)
					{
						//See if A, B, or C appeared, or disappeared
						if ((NR_busdata[NR_node_reference].phases & 0x80) == 0x00)	//No phases means it was just removed
						{
							//Store V1 and V2
							last_voltage[0] = voltage[0];
							last_voltage[1] = voltage[1];
							last_voltage[2] = voltage[2];

							//Clear them out
							voltage[0] = 0.0;
							voltage[1] = 0.0;
							voltage[2] = 0.0;
						}
						else	//Put back in service
						{
							//See if in-rush is enabled or not
							if (!enable_inrush_calculations)
							{
								voltage[0] = last_voltage[0];
								voltage[1] = last_voltage[1];
								voltage[2] = last_voltage[2];
							}
							else	//When restoring from inrush, a very small term is needed (or nothing happens)
							{
								voltage[0] = last_voltage[0] * MULTTERM;
								voltage[1] = last_voltage[1] * MULTTERM;
								voltage[2] = last_voltage[2] * MULTTERM;
							}

							//Default else - leave it be and just leave them to zero (more fun!)
						}

						//Recalculated V12, V1N, V2N in case a child uses them
						voltaged[0] = voltage[0] + voltage[1];	//12
						voltaged[1] = voltage[1] - voltage[2];	//2N
						voltaged[2] = voltage[0] - voltage[2];	//1N -- unsure why odd

					}//End triplex
					else	//Nope
					{
						//Find out changes, and direction
						if ((phase_checks_var & 0x04) == 0x04)	//Phase A change
						{
							//See which way
							if ((prev_phases & 0x04) == 0x04)	//Means A just disappeared
							{
								last_voltage[0] = voltage[0];	//Store the last value
								voltage[0] = 0.0;				//Put us to zero, so volt_dump is happy
							}
							else	//A just came back
							{
								if (!enable_inrush_calculations)
								{
									voltage[0] = last_voltage[0];	//Read in the previous values
								}
								else	//When restoring from inrush, a very small term is needed (or nothing happens)
								{
									voltage[0] = last_voltage[0] * MULTTERM;
								}

								//Default else -- in-rush enabled, just leave as they were
							}
						}//End Phase A change

						//Find out changes, and direction
						if ((phase_checks_var & 0x02) == 0x02)	//Phase B change
						{
							//See which way
							if ((prev_phases & 0x02) == 0x02)	//Means B just disappeared
							{
								last_voltage[1] = voltage[1];	//Store the last value
								voltage[1] = 0.0;				//Put us to zero, so volt_dump is happy
							}
							else	//B just came back
							{
								if (!enable_inrush_calculations)
								{
									voltage[1] = last_voltage[1];	//Read in the previous values
								}
								else	//When restoring from inrush, a very small term is needed (or nothing happens)
								{
									voltage[1] = last_voltage[1] * MULTTERM;
								}

								//Default else - in-rush enabled, we want these to be zero
							}
						}//End Phase B change

						//Find out changes, and direction
						if ((phase_checks_var & 0x01) == 0x01)	//Phase C change
						{
							//See which way
							if ((prev_phases & 0x01) == 0x01)	//Means C just disappeared
							{
								last_voltage[2] = voltage[2];	//Store the last value
								voltage[2] = 0.0;				//Put us to zero, so volt_dump is happy
							}
							else	//C just came back
							{
								if (!enable_inrush_calculations)
								{
									voltage[2] = last_voltage[2];	//Read in the previous values
								}
								else	//When restoring from inrush, a very small term is needed (or nothing happens)
								{
									voltage[2] = last_voltage[2] * MULTTERM;
								}

								//Default else - in-rush enabled and want them zero
							}
						}//End Phase C change

						//Recalculated VAB, VBC, and VCA, in case a child uses them
						voltaged[0] = voltage[0] - voltage[1];
						voltaged[1] = voltage[1] - voltage[2];
						voltaged[2] = voltage[2] - voltage[0];
					}//End not triplex

					//Assign current value in
					prev_phases = NR_busdata[NR_node_reference].phases;
				}//End Phase checks for reliability
			}//End normal node
		}//End NR reference valid
	}//End NR Check

	//See if we're a generator-attached bus (needs full_Y_all) - if so, update it
	if (deltamode_inclusive && dynamic_norton)
	{
		//See if we're a child or a parent
		if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) != 0)
		{
			//Map to our parent
			temp_property = new gld_property(SubNodeParent,"deltamode_full_Y_all_matrix");

			//Make sure it worked
			if (!temp_property->is_valid() || !temp_property->is_complex_array())
			{
				GL_THROW("Node:%d - %s - Failed to map deltamode matrix to parent",obj->id,(obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map to a parent's deltamode-related matrix, a node device encountered an issue.  Please try again.
				If the error persists, please submit a ticket with your code/model to the issue tracker.
				*/
			}

			//Pull down the value
			temp_property->getp<complex_array>(temp_complex_array,*test_rlock);

			//See if it is the right size
			if ((temp_complex_array.get_rows() == 3) && (temp_complex_array.get_cols() == 3))
			{
				//Store it directly into ours
				full_Y_all_matrix = temp_complex_array;
			}
			//Default else - wrong size, just ignore it

			//Delete the link
			delete temp_property;
		}
		else	//Parent or stand-alone
		{
			//Make sure we're valid and a right size
			if (full_Y_all_matrix.is_valid(0,0))
			{
				//Make sure it is the right size
				if ((full_Y_all_matrix.get_rows() != 3) || (full_Y_matrix.get_cols() != 3))
				{
					//Try forcing to be 3x3
					full_Y_all_matrix.grow_to(3,3);
				}
			}
			else	//Not allocated yet -- allocate it
			{
				full_Y_all_matrix.grow_to(3,3);
			}

			//Make sure it valid, just for giggles
			if (full_Y_all != nullptr)
			{
				//Copy our values in
				for (index_x_val=0; index_x_val<3; index_x_val++)
				{
					for (index_y_val=0; index_y_val<3; index_y_val++)
					{
						full_Y_all_matrix.set_at(index_x_val,index_y_val,full_Y_all[index_x_val*3+index_y_val]);
					}
				}
			}
			else	//If we got here, yell
			{
				GL_THROW("Node:%d - %s - Node tried to update a deltamode matrix that does not exist!",obj->id,(obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to update an exposed deltamode matrix, the underlying data was found to not exist!  Please submit your code
				and a bug report via the issue tracker system.
				*/
			}
		}//End parent update
	}
	//Default else -- not needing this to be updated

	//Update tracking variables for nodes/loads/meters
	if ((deltatimestep_running > 0) && enable_inrush_calculations)
	{
		//Get current deltamode timestep
		curr_delta_time = gl_globaldeltaclock;

		//Check and see if we're in a different timestep -- if so, zero the accumulators
		if (curr_delta_time != prev_delta_time)
		{
			//Update the tracker
			prev_delta_time = curr_delta_time;
		}
	}//End deltamode and in-rush
	//Default else -- no in-rush and no deltamode

	/* check for voltage control requirement */
	if (require_voltage_control)
	{
		/* PQ bus must have a source */
		if ((busflags&NF_HASSOURCE)==0 && bustype==PQ)
			voltage[0] = voltage[1] = voltage[2] = gld::complex(0,0);
	}

	//Update appropriate "other" voltages
	if (has_phase(PHASE_S))
	{	// split-tap voltage diffs are different
		voltaged[0] = voltage[0] + voltage[1];	//V12
		voltaged[1] = voltage[1] - voltage[2];	//V2N
		voltaged[2] = voltage[0] - voltage[2];	//V1N -- not sure why these are backwards
	}
	else
	{	// compute 3phase voltage differences
		voltaged[0] = voltage[0] - voltage[1];	//AB
		voltaged[1] = voltage[1] - voltage[2];	//BC
		voltaged[2] = voltage[2] - voltage[0];	//CA
	}

	//This code performs the new "flattened" NR calculations.
	if (solver_method == SM_NR)
	{
		//Update the status flag for a SWING capabilities - only check for non-parented SWING and SWING_PQ nodes
		if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
		{
			//Check for SWING and SWING_PQ
			if ((bustype == SWING) || (bustype == SWING_PQ))
			{
				swing_functions_enabled = NR_busdata[NR_node_reference].swing_functions_enabled;
			}
			else
			{
				//It's a PQ, so it obviously isn't a SWING
				swing_functions_enabled = false;
			}
		}
		else
		{
			//Children are automatically assumed to "not be a SWING", even if they functionally are one
			swing_functions_enabled = false;
		}

		int result = NR_current_update(false);

		//Make sure it worked, just to be thorough
		if (result != 1)
		{
			GL_THROW("Attempt to update current/power on node:%s failed!",obj->name);
			//Defined elsewhere
		}
	}
}

TIMESTAMP node::postsync(TIMESTAMP t0)
{
	TIMESTAMP t1 = powerflow_object::postsync(t0);
	TIMESTAMP RetValue=t1;
	OBJECT *obj = OBJECTHDR(this);

#ifdef SUPPORT_OUTAGES
	if (condition!=OC_NORMAL)	//Zero all the voltages, just in case
	{
		voltage[0] = voltage[1] = voltage[2] = 0.0;
	}

	if (is_contact_any())
	{
		gld::complex dVAB = voltageA - voltageB;
		gld::complex dVBC = voltageB - voltageC;
		gld::complex dVCA = voltageC - voltageA;

		/* phase-phase contact */
		//WRITELOCK_OBJECT(obj);
		if (is_contact(PHASE_A|PHASE_B|PHASE_C))
			/** @todo calculate three-way contact fault current */
			throw "three-way contact not supported yet";
		else if (is_contact(PHASE_A|PHASE_B))
			current_inj[0] = - current_inj[1] = dVAB/fault_Z;
		else if (is_contact(PHASE_B|PHASE_C))
			current_inj[1] = - current_inj[2] = dVBC/fault_Z;
		else if (is_contact(PHASE_A|PHASE_C))
			current_inj[2] = - current_inj[0] = dVCA/fault_Z;

		/* phase-neutral/ground contact */
		if (is_contact(PHASE_A|PHASE_N) || is_contact(PHASE_A|GROUND))
			current_inj[0] = voltageA / fault_Z;
		if (is_contact(PHASE_B|PHASE_N) || is_contact(PHASE_B|GROUND))
			current_inj[1] = voltageB / fault_Z;
		if (is_contact(PHASE_C|PHASE_N) || is_contact(PHASE_C|GROUND))
			current_inj[2] = voltageC / fault_Z;
		//UNLOCK_OBJECT(obj);
	}

	/* record the power in for posterity */
	//kva_in = (voltageA*~current[0] + voltageB*~current[1] + voltageC*~current[2])/1000; /*...or not.  Note sure how this works for a node*/

#endif
	//Call NR-related and some "common" node postsync routines
	BOTH_node_postsync_fxn(obj);

	if (solver_method==SM_FBS)
	{
		// if the parent object is a node
		if (obj->parent!=nullptr && (gl_object_isa(obj->parent,"node","powerflow")))
		{
			// copy the voltage from the parent - check for mismatch handled earlier
			node *pNode = OBJECTDATA(obj->parent,node);
			voltage[0] = pNode->voltage[0];
			voltage[1] = pNode->voltage[1];
			voltage[2] = pNode->voltage[2];

			//Re-update our Delta or single-phase equivalents since we now have a new voltage
			//Update appropriate "other" voltages
			if (phases&PHASE_S)
			{	// split-tap voltage diffs are different
				voltage12 = voltage1 + voltage2;
				voltage1N = voltage1 - voltageN;
				voltage2N = voltage2 - voltageN;
			}
			else
			{	// compute 3phase voltage differences
				voltageAB = voltageA - voltageB;
				voltageBC = voltageB - voltageC;
				voltageCA = voltageC - voltageA;
			}
		}
	}

#ifdef SUPPORT_OUTAGES
	/* check the voltage status for loads */
	if (phases&PHASE_S) // split-phase node
	{
		double V1pu = voltage1.Mag()/nominal_voltage;
		double V2pu = voltage2.Mag()/nominal_voltage;
		if (V1pu<0.8 || V2pu<0.8)
			status=UNDERVOLT;
		else if (V1pu>1.2 || V2pu>1.2)
			status=OVERVOLT;
		else
			status=NOMINAL;
	}
	else // three-phase node
	{
		double VApu = voltageA.Mag()/nominal_voltage;
		double VBpu = voltageB.Mag()/nominal_voltage;
		double VCpu = voltageC.Mag()/nominal_voltage;
		if (VApu<0.8 || VBpu<0.8 || VCpu<0.8)
			status=UNDERVOLT;
		else if (VApu>1.2 || VBpu>1.2 || VCpu>1.2)
			status=OVERVOLT;
		else
			status=NOMINAL;
	}
#endif
	if (solver_method==SM_FBS)
	{
		/* compute the sync voltage change */
		double sync_V = (last_voltage[0]-voltage[0]).Mag() + (last_voltage[1]-voltage[1]).Mag() + (last_voltage[2]-voltage[2]).Mag();

		/* if the sync voltage limit is defined and the sync voltage is larger */
		if (sync_V > maximum_voltage_error){

			/* request another pass */
			RetValue=t0;
		}
	}

	/* the solution is satisfactory */
	return RetValue;
}

int node::kmlinit(int (*stream)(const char*,...))
{
	gld_global host("hostname");
	gld_global port("server_portnum");
#define STYLE(X) stream("<Style id=\"" #X "_g\"><IconStyle><Icon><href>http://%s:%u/rt/" #X "_g.png</href></Icon></IconStyle></Style>\n", (const char*)host.get_string(), port.get_int32());\
		stream("<Style id=\"" #X "_r\"><IconStyle><Icon><href>http://%s:%u/rt/" #X "_r.png</href></Icon></IconStyle></Style>\n", (const char*)host.get_string(), port.get_int32());\
		stream("<Style id=\"" #X "_b\"><IconStyle><Icon><href>http://%s:%u/rt/" #X "_b.png</href></Icon></IconStyle></Style>\n", (const char*)host.get_string(), port.get_int32());\
		stream("<Style id=\"" #X "_k\"><IconStyle><Icon><href>http://%s:%u/rt/" #X "_k.png</href></Icon></IconStyle></Style>\n", (const char*)host.get_string(), port.get_int32());
	STYLE(node);
	STYLE(capacitor);
	STYLE(load);
	STYLE(triplex_meter);

	return 0;
}
int node::kmldump(int (*stream)(const char*,...))
{
	OBJECT *obj = OBJECTHDR(this);
	FUNCTIONADDR temp_funadd = nullptr;

	if (isnan(get_latitude()) || isnan(get_longitude()))
		return 0;
	stream("<Placemark>\n");
	stream("<name>%s</name>\n", get_name() );
	stream("<description>\n");
	stream("<![CDATA[\n");
	stream("<TABLE>\n");

	char status_code[]="kbgr";
	int status = 2; // green
	if ( gl_object_isa(my(),"triplex_meter") )
	{
		//Map to the function
		temp_funadd = (FUNCTIONADDR)(gl_get_function(obj,"pwr_object_kmldata"));

		//See if it was located
		if (temp_funadd == nullptr)
		{
			GL_THROW("object:%s - failed to map kmldata function",(obj->name?obj->name:"unnamed"));
			/*  TROUBLESHOOT
			While attempting to map the kmldata function, an error was encountered.
			Please try again.  If the error persists, please submit your code and a bug report via the trac website.
			*/
		}

		//Call the function
		// TODO use triplex_node to get to triplex_meter
		status = ((int (*)(OBJECT *,int (*stream)(const char*,...)))(*temp_funadd))(obj,stream);
	}
	else
	{
		stream("<CAPTION>%s #%d</CAPTION>\n<TR><TH WIDTH=\"25%\" ALIGN=CENTER>Property<HR></TH>"
				"<TH WIDTH=\"25%\" COLSPAN=2 ALIGN=CENTER><NOBR>Phase A</NOBR><HR></TH>"
				"<TH WIDTH=\"25%\" COLSPAN=2 ALIGN=CENTER><NOBR>Phase B</NOBR><HR></TH>"
				"<TH WIDTH=\"25%\" COLSPAN=2 ALIGN=CENTER><NOBR>Phase C</NOBR><HR></TH></TR>\n", get_oclass()->get_name(), get_id());

		int phase[3] = {has_phase(PHASE_A),has_phase(PHASE_B),has_phase(PHASE_C)};
		double basis[3] = {0,240,120};

		// voltages
		stream("<TR><TH ALIGN=LEFT>Voltage</TH>");
		for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
		{
			if ( phase[i] )
			{
				stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>kV</TD>", voltage[i].Mag()/1000);
				if ( status>0 && voltage[i].Mag() <= 0.5*nominal_voltage ) status = 0; // black
				else if ( status==2 && voltage[i].Mag() < 0.95*nominal_voltage ) status = 1; // blue
				else if ( status==2 && voltage[i].Mag() > 1.05*nominal_voltage ) status = 3; // red
			}
			else
				stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">&mdash;</TD><TD>&nbsp;</TD>");
		}
		stream("</TR>\n");
		stream("<TR><TH ALIGN=LEFT>&nbsp</TH>");
		for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
		{
			if ( phase[i] )
				stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>&deg;</TD>", voltage[i].Arg()*180/3.1416 - basis[i]);
			else
				stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">&mdash;</TD><TD>&nbsp;</TD>");
		}
		stream("</TR>\n");

		//Check others - de-"macrotized" so it can do things indirectly
		if (gl_object_isa(my(),"load","powerflow") || gl_object_isa(my(),"capacitor","powerflow"))
		{
			//Map to the function
			temp_funadd = (FUNCTIONADDR)(gl_get_function(obj,"pwr_object_kmldata"));

			//See if it was located
			if (temp_funadd == nullptr)
			{
				GL_THROW("object:%s - failed to map kmldata function",(obj->name?obj->name:"unnamed"));
				//Defined above
			}

			//Call the function
			// TODO use triplex_node to get to triplex_meter
			status = ((int (*)(OBJECT *,int (*stream)(const char*,...)))(*temp_funadd))(obj,stream);
		}
		else
		{
			// power
			stream("<TR><TH ALIGN=LEFT>Power</TH>");
			for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
			{
				if ( phase[i] )
					stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>kW</TD>", power[i].Re()/1000);
				else
					stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">&mdash;</TD><TD>&nbsp;</TD>");
			}
			stream("</TR>\n");
			stream("<TR><TH ALIGN=LEFT>&nbsp</TH>");
			for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
			{
				if ( phase[i] )
					stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>kVAR</TD>", power[i].Im()/1000);
				else
					stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">&mdash;</TD><TD>&nbsp;</TD>");
			}
			stream("</TR>\n");
		}
	}
	stream("</TABLE>\n");
	stream("]]>\n");
	stream("</description>\n");
	stream("<styleUrl>#%s_%c</styleUrl>\n",obj->oclass->name, status_code[status]);
	stream("<Point>\n");
	stream("<coordinates>%f,%f</coordinates>\n",get_longitude(),get_latitude());
	stream("</Point>\n");
	stream("</Placemark>\n");
	return 0;
}

//Notify function
//NOTE: The NR-based notify stuff may no longer be needed after NR is "flattened", since it will
//      effectively be like FBS at that point.
int node::notify(int update_mode, PROPERTY *prop, char *value)
{
	gld::complex diff_val;

	if (solver_method == SM_NR)
	{
		//Only even bother with voltage updates if we're properly populated
		if (prev_voltage_value != nullptr)
		{
			//See if there is a voltage update - phase A
			if (strcmp(prop->name,"voltage_A")==0)
			{
				if (update_mode==NM_PREUPDATE)
				{
					//Store the last value
					prev_voltage_value[0] = voltage[0];
				}
				else if (update_mode==NM_POSTUPDATE)
				{
					//Handle the logics
					//See what the difference is - if it is above the convergence limit, send an NR update
					diff_val = voltage[0] - prev_voltage_value[0];

					if (diff_val.Mag() >= maximum_voltage_error)	//Outside of range, so force a new iteration
					{
						NR_retval = gl_globalclock;
					}
				}
			}

			//See if there is a voltage update - phase B
			if (strcmp(prop->name,"voltage_B")==0)
			{
				if (update_mode==NM_PREUPDATE)
				{
					//Store the last value
					prev_voltage_value[1] = voltage[1];
				}
				else if (update_mode==NM_POSTUPDATE)
				{
					//Handle the logics
					//See what the difference is - if it is above the convergence limit, send an NR update
					diff_val = voltage[1] - prev_voltage_value[1];

					if (diff_val.Mag() >= maximum_voltage_error)	//Outside of range, so force a new iteration
					{
						NR_retval = gl_globalclock;
					}
				}
			}

			//See if there is a voltage update - phase C
			if (strcmp(prop->name,"voltage_C")==0)
			{
				if (update_mode==NM_PREUPDATE)
				{
					//Store the last value
					prev_voltage_value[2] = voltage[2];
				}
				else if (update_mode==NM_POSTUPDATE)
				{
					//Handle the logics
					//See what the difference is - if it is above the convergence limit, send an NR update
					diff_val = voltage[2] - prev_voltage_value[2];

					if (diff_val.Mag() >= maximum_voltage_error)	//Outside of range, so force a new iteration
					{
						NR_retval = gl_globalclock;
					}
				}
			}
		}
	}//End NR
	//Default else - FBS's iteration methods aren't sensitive to this

	return 1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: node
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_node(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(node::oclass);
		if (*obj!=nullptr)
		{
			node *my = OBJECTDATA(*obj,node);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(node);
}

// Commit function
EXPORT TIMESTAMP commit_node(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	node *pNode = OBJECTDATA(obj,node);
	try {
		// This zeroes out all of the unused phases at each node in the FBS method
		if (solver_method==SM_FBS)
		{
			if (pNode->has_phase(PHASE_A)) {
			//leave it
			}
			else
				pNode->voltage[0] = gld::complex(0,0);

			if (pNode->has_phase(PHASE_B)) {
				//leave it
			}
			else
				pNode->voltage[1] = gld::complex(0,0);

			if (pNode->has_phase(PHASE_C)) {
				//leave it
			}
			else
				pNode->voltage[2] = gld::complex(0,0);

		}
		return TS_NEVER;
	}
	catch (char *msg)
	{
		gl_error("%s (node:%d): %s", pNode->get_name(), pNode->get_id(), msg);
		return 0;
	}

}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_node(OBJECT *obj)
{
	try {
		node *my = OBJECTDATA(obj,node);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(node);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_node(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		node *pObj = OBJECTDATA(obj,node);
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
	}
	SYNC_CATCHALL(node);
}

/**
* Function to search for a master swing node, one swing to rule them all
* Functionalized to help compartmentalize the code
* node_type_value is the class name
* main_swing determines if we're looking for SWING or SWING_PQ (swing parses first)
*/
OBJECT *node::NR_master_swing_search(const char *node_type_value,bool main_swing)
{
	OBJECT *return_val = nullptr;
	OBJECT *temp_obj = nullptr;
	node *list_node;
	FINDLIST *bus_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,node_type_value,FT_END);

	//Parse the findlist
	while(temp_obj=gl_find_next(bus_list,temp_obj))
	{
		list_node = OBJECTDATA(temp_obj,node);

		//See which kind we are looking for
		if (main_swing)
		{
			if (list_node->bustype==SWING)
			{
				return_val=temp_obj;
				break;	//Only need to find one
			}
		}
		else	//Look for secondary swings
		{
			if (list_node->bustype==SWING_PQ)
			{
				return_val=temp_obj;
				break;	//Only need to find one
			}
		}
	}

	//Free the list, before continuing
	gl_free(bus_list);

	//Return the pointer
	return return_val;
}

/**
* NR_populate is called by link objects during their first presync if a node is not
* "initialized".  This function "initializes" the node into the Newton-Raphson data
* structure NR_busdata
*
*/
int node::NR_populate(void)
{
	//Object header for names
	OBJECT *me = OBJECTHDR(this);
	node *temp_par_node = nullptr;
	gld_property *temp_bool_property = nullptr;
	gld_wlock *test_rlock = nullptr;
	bool temp_bool_val;

	//Lock the SWING for global operations
	if ( NR_swing_bus!=me ) LOCK_OBJECT(NR_swing_bus);

	NR_node_reference = NR_curr_bus;	//Grab the current location and keep it as our own
	NR_curr_bus++;					//Increment the current bus pointer for next variable
	if ( NR_swing_bus!=me ) UNLOCK_OBJECT(NR_swing_bus);	//All done playing with globals, unlock the swing so others can proceed

	//Quick check to see if there problems
	if (NR_node_reference == -1)
	{
		GL_THROW("NR: node:%s failed to grab a unique bus index value!",me->name);
		/*  TROUBLESHOOT
		While attempting to gain a unique bus id for the Newton-Raphson solver, an error
		was encountered.  This may be related to a parallelization effort.  Please try again.
		If the error persists, please submit your code and a bug report via the trac website.
		*/
	}

	//Bus type
	NR_busdata[NR_node_reference].type = (int)bustype;

	//For all initial implementations, we're all part of the same big/happy island - island #0!
	NR_busdata[NR_node_reference].island_number = 0;

	//Interim check to make sure it isn't a PV bus, since those aren't supported yet - this will get removed when that functionality is put in place
	if (NR_busdata[NR_node_reference].type==1)
	{
		GL_THROW("NR: node:%s is a PV bus - these are not yet supported.",(me->name?me->name:"Unnamed"));
		/*  TROUBLESHOOT
		The Newton-Raphson solver implemented does not currently support the PV bus type.
		*/
	}

	//Populate the swing flag - it will get "deflagged" elsewhere
	if ((bustype == SWING) || (bustype == SWING_PQ))
	{
		//See if we're a SWING_PQ
		if (bustype == SWING_PQ)
		{
			//Check for fault_check
			if (fault_check_object == nullptr)
			{
				gl_warning("node:%d - %s - Set as a SWING_PQ, but no fault_check present - will be treated as SWING",me->id,(me->name?me->name:"Unnamed"));
				/*  TROUBLESHOOT
				A node is set up as a SWING_PQ, but there is no fault_check object on the system.  This will just be treated as a SWING bus for all calculations.
				*/
			}
			else
			{
				//Fault check present - see if it is in the proper mode!
				temp_bool_property = new gld_property(fault_check_object,"grid_association");

				//Make sure it worked
				if (!temp_bool_property->is_valid() || !temp_bool_property->is_bool())
				{
					GL_THROW("NR: node:%s failed to map fault_check object property",(me->name?me->name:"Unnamed"));
					/*  TROUBLESHOOT
					While attempting to map a property of a fault_check object, an error was encountered.  Please try again.
					If the error persists, please submit an item to the issue tracker.
					*/
				}

				//Pull the value
				temp_bool_property->getp<bool>(temp_bool_val,*test_rlock);

				//Clear the property
				delete temp_bool_property;

				//Compare the value
				if (!temp_bool_val)	//Not in grid_association mode!
				{
					gl_warning("node:%d - %s - Set as a SWING_PQ, but fault_check in wrong mode - will be treated as SWING",me->id,(me->name?me->name:"Unnamed"));
					/*  TROUBLESHOOT
					A node is set up as a SWING_PQ, but the fault_check object is not set to do grid_association, so this bus will just be treated as a
					SWING bus for all calculations.
					*/
				}
			}
		}//End SWING PQ check
		//Must be a SWING

		//Flag it, regardless (unflagged elsewhere, if set right)
		NR_busdata[NR_node_reference].swing_functions_enabled = true;
	}
	else
	{
		NR_busdata[NR_node_reference].swing_functions_enabled = false;
	}

	//Default us to topological not the source (mostly used by SWING_PQ - set elsewhere)
	NR_busdata[NR_node_reference].swing_topology_entry = false;

	//Populate phases
	NR_busdata[NR_node_reference].phases = 128*has_phase(PHASE_S) + 8*has_phase(PHASE_D) + 4*has_phase(PHASE_A) + 2*has_phase(PHASE_B) + has_phase(PHASE_C);

	//Link our name in
	NR_busdata[NR_node_reference].name = me->name;

	//Link us in as well
	NR_busdata[NR_node_reference].obj = me;

	//Populate our maximum error vlaue
	NR_busdata[NR_node_reference].max_volt_error = maximum_voltage_error;

	//Link the busflags property
	NR_busdata[NR_node_reference].busflag = &busflags;

	//Populate voltage
	NR_busdata[NR_node_reference].V = &voltage[0];

	//Populate power
	NR_busdata[NR_node_reference].S = &power[0];

	//Populate admittance
	NR_busdata[NR_node_reference].Y = &shunt[0];

	//Populate current
	NR_busdata[NR_node_reference].I = &current[0];

	//Populate unrotated current
	NR_busdata[NR_node_reference].prerot_I = &pre_rotated_current[0];

	//Populate explicit power
	NR_busdata[NR_node_reference].S_dy = &power_dy[0];

	//Populate explicit admittance
	NR_busdata[NR_node_reference].Y_dy = &shunt_dy[0];

	//Populate explicit current
	NR_busdata[NR_node_reference].I_dy = &current_dy[0];

	//Allocate our link list
	NR_busdata[NR_node_reference].Link_Table = (int *)gl_malloc(NR_connected_links[0]*sizeof(int));
	
	if (NR_busdata[NR_node_reference].Link_Table == nullptr)
	{
		GL_THROW("NR: Failed to allocate link table for node:%d",me->id);
		/*  TROUBLESHOOT
		While attempting to allocate memory for the linking table for NR, memory failed to be
		allocated.  Make sure you have enough memory and try again.  If this problem happens a second
		time, submit your code and a bug report using the trac website.
		*/
	}

	//Populate our size
	NR_busdata[NR_node_reference].Link_Table_Size = NR_connected_links[0];

	//See if we're a triplex
	if (has_phase(PHASE_S))
	{
		if (house_present)	//We're a proud parent of a house!
		{
			NR_busdata[NR_node_reference].house_var = &nom_res_curr[0];	//Separate storage area for nominal house currents
			NR_busdata[NR_node_reference].phases |= 0x40;					//Flag that we are a house-attached node
		}

		NR_busdata[NR_node_reference].extra_var = &current12;	//Stored in a separate variable and this is the easiest way for me to get it
	}
	else	//Implies it is non-triplex
	{
		if ((SubNode & SNT_DIFF_PARENT)==SNT_DIFF_PARENT)	//Differently connected load/node (only can't be S)
		{
			NR_busdata[NR_node_reference].extra_var = Extra_Data;
			NR_busdata[NR_node_reference].phases |= 0x10;			//Special flag for a phase mismatch being present
		}

		//See if we have any houses of the three-phase/non-triplex variety
		if (house_present)	//We're a proud parent of a house!
		{
			NR_busdata[NR_node_reference].house_var = &nom_res_curr[0];	//Separate storage area for nominal house currents
			NR_busdata[NR_node_reference].phases |= 0x40;					//Flag that we are a house-attached node
		}
	}

	//Per unit values - populate nominal voltage on a whim
	NR_busdata[NR_node_reference].volt_base = nominal_voltage;
	NR_busdata[NR_node_reference].mva_base = -1.0;

	//Set the matrix value to -1 to know it hasn't been set (probably not necessary)
	NR_busdata[NR_node_reference].Matrix_Loc = -1;

	//Populate original phases
	NR_busdata[NR_node_reference].origphases = NR_busdata[NR_node_reference].phases;

	//Populate our tracking variable
	prev_phases = NR_busdata[NR_node_reference].phases;

	//Populate dynamic mode flag address
	NR_busdata[NR_node_reference].dynamics_enabled = &deltamode_inclusive;

	//Check and see if we're in deltamode, and in-rush is enabled to allocation a variable
	if ((NR_solver_algorithm == NRM_FPI) || ((NR_solver_algorithm == NRM_TCIM) && deltamode_inclusive && enable_inrush_calculations))
	{
		//Map the admittance load matrix
		NR_busdata[NR_node_reference].full_Y_load = &full_Y_load[0][0];
	}
	else	//Not deltamode or not FPI
	{
		//Null it, just to be safe
		NR_busdata[NR_node_reference].full_Y_load = nullptr;
	}

	//Other items
	if (deltamode_inclusive && enable_inrush_calculations)
	{
		//Link up the array (prealloced now, so always exists)
		NR_busdata[NR_node_reference].BusHistTerm = BusHistTerm;
	}
	else	//One of these isn't true
	{
		//Null it, just to be safe - do with both
		NR_busdata[NR_node_reference].BusHistTerm = nullptr;
	}

	//Always null the saturation term -- if it is needed, the link will populate it
	NR_busdata[NR_node_reference].BusSatTerm = nullptr;

	//Null the extra function pointer -- the individual object will call to populate this
	NR_busdata[NR_node_reference].ExtraCurrentInjFunc = nullptr;
	NR_busdata[NR_node_reference].ExtraCurrentInjFuncObject = nullptr;

	//Extra functions - see if we're a load - map update if we're in the right mode
	//Could potentially flag this in load/triplex_load - it's primarily needed to keep constant_current-based loads properly rotated
	//Flagging it could improve performance
	if ((gl_object_isa(me,"load","powerflow") || gl_object_isa(me,"triplex_load","powerflow")) && !enable_inrush_calculations)
	{
		//Map the function
		NR_busdata[NR_node_reference].LoadUpdateFxn = (FUNCTIONADDR)(gl_get_function(me,"pwr_object_load_update"));

		//Make sure it worked
		if (NR_busdata[NR_node_reference].LoadUpdateFxn == nullptr)
		{
			GL_THROW("node:%d - %s - Failed to map load_update",me->id,(me->name ? me->name : "Unnamed"));
			/*  TROUBLESHOOT
			The attached node was unable to find the exposed function "load_update" on the calling object.  Be sure
			it supports this functionality and try again.
			*/
		}
		//Default else -- it worked
	}
	else
	{
		//Not a load
		NR_busdata[NR_node_reference].LoadUpdateFxn = nullptr;
	}

	//See if we're FPI (and a parent/stand-alone)
	if ((NR_solver_algorithm == NRM_FPI) && ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0))
	{
		//Map our function
		NR_busdata[NR_node_reference].ShuntUpdateFxn = (FUNCTIONADDR)(gl_get_function(me,"pwr_object_shunt_update"));

		//Make sure it worked
		if (NR_busdata[NR_node_reference].ShuntUpdateFxn == nullptr)
		{
			GL_THROW("node:%d - %s - Failed to map shunt",me->id,(me->name ? me->name : "Unnamed"));
			/*  TROUBLESHOOT
			The attached node was unable to find the exposed function "shunt_update" on the calling object.  Be sure
			it supports this functionality and try again.
			*/
		}
		//Default else - must have worked
	}
	else	//Child or TCIM
	{
		NR_busdata[NR_node_reference].ShuntUpdateFxn = nullptr;
	}

	//Allocate dynamic variables -- only if something has requested it
	if (deltamode_inclusive && (dynamic_norton || dynamic_generator))
	{
		//Check our status - shouldn't be necessary, but let's be paranoid
		if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)	//We're stand-alone or a parent
		{
			//Only do admittance allocation if we're a Norton
			if (dynamic_norton)
			{
				//Make sure no pesky children have already allocated us
				if (full_Y == nullptr)
				{
					//Allocate it
					full_Y = (gld::complex *)gl_malloc(9*sizeof(gld::complex));

					//Check it
					if (full_Y==nullptr)
					{
						GL_THROW("Node:%s failed to allocate space for the a deltamode variable",me->name);
						/*  TROUBLESHOOT
						While attempting to allocate memory for a dynamics-required (deltamode) variable, an error
						occurred. Please try again.  If the error persists, please submit your code and a bug
						report via the trac website.
						*/
					}

					//See if our published matrix has anything in it
					if ((full_Y_matrix.get_rows() == 3) && (full_Y_matrix.get_cols() == 3))
					{
						full_Y[0] = full_Y_matrix.get_at(0,0);
						full_Y[1] = full_Y_matrix.get_at(0,1);
						full_Y[2] = full_Y_matrix.get_at(0,2);

						full_Y[3] = full_Y_matrix.get_at(1,0);
						full_Y[4] = full_Y_matrix.get_at(1,1);
						full_Y[5] = full_Y_matrix.get_at(1,2);

						full_Y[6] = full_Y_matrix.get_at(2,0);
						full_Y[7] = full_Y_matrix.get_at(2,1);
						full_Y[8] = full_Y_matrix.get_at(2,2);
					}
					else
					{
						//Zero it, just to be safe (gens will accumulate into it)
						full_Y[0] = full_Y[1] = full_Y[2] = gld::complex(0.0,0.0);
						full_Y[3] = full_Y[4] = full_Y[5] = gld::complex(0.0,0.0);
						full_Y[6] = full_Y[7] = full_Y[8] = gld::complex(0.0,0.0);
					}

					//Allocate another matrix for admittance - this will have the full value
					full_Y_all = (gld::complex *)gl_malloc(9*sizeof(gld::complex));

					//Check it
					if (full_Y_all==nullptr)
					{
						GL_THROW("Node:%s failed to allocate space for the a deltamode variable",me->name);
						//Defined above
					}

					//See if our published matrix has anything in it
					if ((full_Y_all_matrix.get_rows() == 3) && (full_Y_all_matrix.get_cols() == 3))
					{
						full_Y_all[0] = full_Y_all_matrix.get_at(0,0);
						full_Y_all[1] = full_Y_all_matrix.get_at(0,1);
						full_Y_all[2] = full_Y_all_matrix.get_at(0,2);

						full_Y_all[3] = full_Y_all_matrix.get_at(1,0);
						full_Y_all[4] = full_Y_all_matrix.get_at(1,1);
						full_Y_all[5] = full_Y_all_matrix.get_at(1,2);

						full_Y_all[6] = full_Y_all_matrix.get_at(2,0);
						full_Y_all[7] = full_Y_all_matrix.get_at(2,1);
						full_Y_all[8] = full_Y_all_matrix.get_at(2,2);
					}
					else
					{
						//Try growing it to the proper size
						full_Y_all_matrix.grow_to(3,3);

						//Zero it, just to be safe (gens will accumulate into it)
						full_Y_all[0] = full_Y_all[1] = full_Y_all[2] = gld::complex(0.0,0.0);
						full_Y_all[3] = full_Y_all[4] = full_Y_all[5] = gld::complex(0.0,0.0);
						full_Y_all[6] = full_Y_all[7] = full_Y_all[8] = gld::complex(0.0,0.0);
					}
				}
				else	//Not needed, make sure we're nulled
				{
					full_Y_all = nullptr;
				}
			}//End Norton equivalent needing admittance
			else	//Null them out of paranoia
			{
				//Null this one too
				full_Y = nullptr;

				//Null it, just because
				full_Y_all = nullptr;
			}
		}//End we're a parent

		//Map all relevant variables to the NR structure
		NR_busdata[NR_node_reference].full_Y = full_Y;
		NR_busdata[NR_node_reference].full_Y_all = full_Y_all;
		NR_busdata[NR_node_reference].DynCurrent = &deltamode_dynamic_current[0];
		NR_busdata[NR_node_reference].PGenTotal = &deltamode_PGenTotal;
	}
	else	//Ensure it is empty
	{
		NR_busdata[NR_node_reference].full_Y = nullptr;
		NR_busdata[NR_node_reference].full_Y_all = nullptr;
		NR_busdata[NR_node_reference].DynCurrent = nullptr;
		NR_busdata[NR_node_reference].PGenTotal = nullptr;
	}

	return 0;
}

//Computes "load" portions of current injection
//parentcall is set when a parent object has called this update - mainly for locking purposes
int node::NR_current_update(bool parentcall)
{
	unsigned int table_index;
	FUNCTIONADDR temp_funadd = nullptr;
	int temp_result, loop_index;
	OBJECT *obj = OBJECTHDR(this);
	OBJECT *tmp_obj;
	gld::complex temp_current_inj[3];
	gld::complex temp_current_val[3];
	gld::complex adjusted_current_val[3];
	gld::complex delta_shunt[3];
	gld::complex delta_current[3];
	gld::complex assumed_nominal_voltage[6];
	double nominal_voltage_dval;
	gld::complex house_pres_current[3];
	gld::complex temp_store[3];

	//Don't do anything if we've already been "updated"
	if (!current_accumulated)
	{
		//See if we have children - need to copy the voltages
		if (NR_number_child_nodes[0]>0)	//We have children
		{
			//Just loop through - we'll go forward here, just for giggles
			for (table_index=0; table_index<NR_number_child_nodes[0]; table_index++)
			{
				//Propagate our voltages to our children
				NR_child_nodes[table_index]->voltage[0] = voltage[0];
				NR_child_nodes[table_index]->voltage[1] = voltage[1];
				NR_child_nodes[table_index]->voltage[2] = voltage[2];

				//Do the same for delta-voltages - just for giggles (might be needed)
				NR_child_nodes[table_index]->voltaged[0] = voltaged[0];
				NR_child_nodes[table_index]->voltaged[1] = voltaged[1];
				NR_child_nodes[table_index]->voltaged[2] = voltaged[2];
			}//End FOR child table
		}//End we have children

		//Handle our links - let's the get posted to the children properly first
		if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)	//Make sure we aren't children as well, since we'll get NULL pointers and make everyone upset
		{
			for (table_index=0; table_index<NR_busdata[NR_node_reference].Link_Table_Size; table_index++)
			{
				//Extract the object pointer - just for readability
				tmp_obj = NR_branchdata[NR_busdata[NR_node_reference].Link_Table[table_index]].obj;

				//Extract the function address
				temp_funadd = (FUNCTIONADDR)(gl_get_function(tmp_obj,"perform_current_calculation_pwr_link"));

				//Make sure it worked
				if (temp_funadd == nullptr)
				{
					GL_THROW("Attemped to update current for object:%s failed!",(tmp_obj->name?tmp_obj->name:"Unnamed"));
					/*  TROUBLESHOOT
					The current node object tried to update the current injections for what it thought was a link object.  This does
					not appear to be true.  Try your code again.  If the error persists, please submit your GLM and a bug report to the
					issue tracker.
					*/
				}

				//Call a lock on that link - just in case multiple nodes call it at once
				WRITELOCK_OBJECT(tmp_obj);

				//Call its update - tell it who is asking so it knows what to lock
				temp_result = ((int (*)(OBJECT *,int, bool))(*temp_funadd))(tmp_obj,NR_node_reference,false);

				//Unlock the link
				WRITEUNLOCK_OBJECT(tmp_obj);

				//See if it worked, just in case this gets added in the future
				if (temp_result != 1)
				{
					GL_THROW("Attempt to update current on link:%s failed!",NR_branchdata[NR_busdata[NR_node_reference].Link_Table[table_index]].name);
					/*  TROUBLESHOOT
					While attempting to update the current on link object, an error was encountered.  Please try again.  If the error persists,
					please submit your code and a bug report via the trac website.
					*/
				}
			}//End link traversion
		}//End not children

		//Handle relvant children first
		if (NR_number_child_nodes[0]>0)	//We have children
		{
			//Progress backwards, since lower nodes should have been populated last (lower rank)
			for (table_index=NR_number_child_nodes[0]; table_index>0; table_index--)
			{
				//Call their update - By this call's nature, it is being called by a parent here
				temp_result = NR_child_nodes[(table_index-1)]->NR_current_update(true);

				//Make sure it worked, just to be thorough
				if (temp_result != 1)
				{
					GL_THROW("Attempt to update current on child of node:%s failed!",obj->name);
					/*  TROUBLESHOOT
					While attempting to update the current on a childed node object, an error was encountered.  Please try again.  If the error persists,
					please submit your code and a bug report via the trac website.
					*/
				}
			}//End FOR child table
		}//End we have children

		if ((SubNode & SNT_CHILD)==SNT_CHILD)	//Remove child contributions
		{
			node *ParToLoad = OBJECTDATA(SubNodeParent,node);

			if (!parentcall)	//We weren't called by our parent, so lock us to create sibling rivalry!
			{
				//Lock the parent for writing
				LOCK_OBJECT(SubNodeParent);
			}

			//Remove power and "load" characteristics
			ParToLoad->power[0]-=last_child_power[0][0];
			ParToLoad->power[1]-=last_child_power[0][1];
			ParToLoad->power[2]-=last_child_power[0][2];

			ParToLoad->shunt[0]-=last_child_power[1][0];
			ParToLoad->shunt[1]-=last_child_power[1][1];
			ParToLoad->shunt[2]-=last_child_power[1][2];

			ParToLoad->current[0]-=last_child_power[2][0];
			ParToLoad->current[1]-=last_child_power[2][1];
			ParToLoad->current[2]-=last_child_power[2][2];

			if (has_phase(PHASE_S))	//Triplex slightly different
				ParToLoad->current12-=last_child_current12;

			//Unrotated stuff too
			ParToLoad->pre_rotated_current[0] -= last_child_power[3][0];
			ParToLoad->pre_rotated_current[1] -= last_child_power[3][1];
			ParToLoad->pre_rotated_current[2] -= last_child_power[3][2];

			//And the deltamode accumulators too -- if deltamode
			//Only do this if we are a dynamic norton child - otherwise may do extra
			if (deltamode_inclusive && dynamic_norton)
			{
				//Pull our parent value down -- everything is being pushed up there anyways
				//Pulling to keep meters accurate (theoretically)
				deltamode_dynamic_current[0] = ParToLoad->deltamode_dynamic_current[0];
				deltamode_dynamic_current[1] = ParToLoad->deltamode_dynamic_current[1];
				deltamode_dynamic_current[2] = ParToLoad->deltamode_dynamic_current[2];
			}

			//Remove power and "load" characteristics for explicit delta/wye values
			for (loop_index=0; loop_index<6; loop_index++)
			{
				ParToLoad->power_dy[loop_index] -= last_child_power_dy[loop_index][0];		//Power
				ParToLoad->shunt_dy[loop_index] -= last_child_power_dy[loop_index][1];		//Shunt
				ParToLoad->current_dy[loop_index] -= last_child_power_dy[loop_index][2];	//Current
			}

			if (!parentcall)	//Wasn't a parent call - unlock us so our siblings get a shot
			{
				//Unlock the parent now that it is done
				UNLOCK_OBJECT(SubNodeParent);
			}

			//Update previous power tracker - if we haven't really converged, things will mess up without this
			//Power
			last_child_power[0][0] = last_child_power[0][1] = last_child_power[0][2] = 0.0;

			//Shunt
			last_child_power[1][0] = last_child_power[1][1] = last_child_power[1][2] = 0.0;

			//Current
			last_child_power[2][0] = last_child_power[2][1] = last_child_power[2][2] = 0.0;

			//Zero the last power accumulators
			for (loop_index=0; loop_index<6; loop_index++)
			{
				last_child_power_dy[loop_index][0] = gld::complex(0.0);	//Power
				last_child_power_dy[loop_index][1] = gld::complex(0.0);	//Shunt
				last_child_power_dy[loop_index][2] = gld::complex(0.0);	//Current
			}

			//Current 12 if we are triplex
			if (has_phase(PHASE_S))
				last_child_current12 = 0.0;

			//Do the same for the prerorated stuff
			last_child_power[3][0] = last_child_power[3][1] = last_child_power[3][2] = gld::complex(0.0,0.0);
		}
		else if ((SubNode & SNT_DIFF_CHILD)==SNT_DIFF_CHILD)	//Differently connected
		{
			node *ParToLoad = OBJECTDATA(SubNodeParent,node);

			if (!parentcall)	//We weren't called by our parent, so lock us to create sibling rivalry!
			{
				//Lock the parent for writing
				LOCK_OBJECT(SubNodeParent);
			}

			//Remove power and "load" characteristics for explicit delta/wye values
			for (loop_index=0; loop_index<6; loop_index++)
			{
				ParToLoad->power_dy[loop_index] -= last_child_power_dy[loop_index][0];		//Power
				ParToLoad->shunt_dy[loop_index] -= last_child_power_dy[loop_index][1];		//Shunt
				ParToLoad->current_dy[loop_index] -= last_child_power_dy[loop_index][2];	//Current
			}

			//Do this for the unrotated stuff too - it never gets auto-zeroed (like the above)
			ParToLoad->pre_rotated_current[0] -= last_child_power[3][0];
			ParToLoad->pre_rotated_current[1] -= last_child_power[3][1];
			ParToLoad->pre_rotated_current[2] -= last_child_power[3][2];

			if (!parentcall)	//Wasn't a parent call - unlock us so our siblings get a shot
			{
				//Unlock the parent now that it is done
				UNLOCK_OBJECT(SubNodeParent);
			}

			//Zero the last power accumulators
			for (loop_index=0; loop_index<6; loop_index++)
			{
				last_child_power_dy[loop_index][0] = gld::complex(0.0);	//Power
				last_child_power_dy[loop_index][1] = gld::complex(0.0);	//Shunt
				last_child_power_dy[loop_index][2] = gld::complex(0.0);	//Current
			}

			//Now zero the accmulator, just in case
			last_child_power[3][0] = last_child_power[3][1] = last_child_power[3][2] = gld::complex(0.0,0.0);
		}

		//Handle our "self" - do this in a "temporary fashion" for children problems
		temp_current_inj[0] = temp_current_inj[1] = temp_current_inj[2] = gld::complex(0.0,0.0);

		//Set up reference voltage for any accumulator adjustments
		//See if we're a triplex
		if (has_phase(PHASE_S))
		{
			assumed_nominal_voltage[0].SetPolar(nominal_voltage,0.0);			//1
			assumed_nominal_voltage[1].SetPolar(nominal_voltage,0.0);			//2
			assumed_nominal_voltage[2] = assumed_nominal_voltage[0] + assumed_nominal_voltage[1];	//12
			assumed_nominal_voltage[3] = gld::complex(0.0,0.0);	//Not needed - zero for giggles
			assumed_nominal_voltage[4] = gld::complex(0.0,0.0);
			assumed_nominal_voltage[5] = gld::complex(0.0,0.0);

			//Populate LL value
			nominal_voltage_dval = 2.0 * nominal_voltage;
		}
		else //Standard fare
		{
			assumed_nominal_voltage[0].SetPolar(nominal_voltage,0.0);			//AN
			assumed_nominal_voltage[1].SetPolar(nominal_voltage,(-2.0*PI/3.0));	//BN
			assumed_nominal_voltage[2].SetPolar(nominal_voltage,(2.0*PI/3.0));	//CN
			assumed_nominal_voltage[3] = assumed_nominal_voltage[0] - assumed_nominal_voltage[1];	//AB
			assumed_nominal_voltage[4] = assumed_nominal_voltage[1] - assumed_nominal_voltage[2];	//BC
			assumed_nominal_voltage[5] = assumed_nominal_voltage[2] - assumed_nominal_voltage[0];	//CA

			//Populate LL value
			nominal_voltage_dval = assumed_nominal_voltage[3].Mag();
		}

		if (has_phase(PHASE_D))	//Delta connection
		{
			//Convert delta connected impedance
			delta_shunt[0] = voltaged[0]*shunt[0];
			delta_shunt[1] = voltaged[1]*shunt[1];
			delta_shunt[2] = voltaged[2]*shunt[2];

			//Convert delta connected power
			delta_current[0]= (voltaged[0]==0) ? gld::complex(0,0) : ~(power[0]/voltaged[0]);
			delta_current[1]= (voltaged[1]==0) ? gld::complex(0,0) : ~(power[1]/voltaged[1]);
			delta_current[2]= (voltaged[2]==0) ? gld::complex(0,0) : ~(power[2]/voltaged[2]);

			//Adjust constant current values, as necessary
			//Loop through the phases
			for (loop_index=0; loop_index<3; loop_index++)
			{
				//Check existence of phases and adjust the currents appropriately
				if (voltaged[loop_index] != 0.0)
				{
					adjusted_current_val[loop_index] =  ~((assumed_nominal_voltage[loop_index+3]*~current[loop_index]*voltaged[loop_index].Mag())/(voltaged[loop_index]*nominal_voltage_dval));
				}
				else
				{
					adjusted_current_val[loop_index] = gld::complex(0.0,0.0);
				}
			}

			//Translate into a line current
			temp_current_val[0] = delta_shunt[0]-delta_shunt[2] + delta_current[0]-delta_current[2] + adjusted_current_val[0]-adjusted_current_val[2];
			temp_current_val[1] = delta_shunt[1]-delta_shunt[0] + delta_current[1]-delta_current[0] + adjusted_current_val[1]-adjusted_current_val[0];
			temp_current_val[2] = delta_shunt[2]-delta_shunt[1] + delta_current[2]-delta_current[1] + adjusted_current_val[2]-adjusted_current_val[1];

			temp_current_inj[0] = temp_current_val[0];
			temp_current_inj[1] = temp_current_val[1];
			temp_current_inj[2] = temp_current_val[2];
		}
		else if (has_phase(PHASE_S))	//Split phase node
		{
			gld::complex vdel;
			gld::complex temp_current[3];
			gld::complex temp_val[3];

			//Find V12 (just in case)
			vdel=voltage[0] + voltage[1];

			//Find contributions - don't need to be adjusted, since everything does that elsewhere now
			adjusted_current_val[0] = current[0];
			adjusted_current_val[1] = current[1];
			adjusted_current_val[2] = current12;	//current12 is not part of the standard current array

			//Start with the currents (just put them in)
			temp_current[0] = adjusted_current_val[0];
			temp_current[1] = adjusted_current_val[1];
			temp_current[2] = adjusted_current_val[2];

			//Add in the unrotated bit
			temp_current[0] += pre_rotated_current[0];	//1
			temp_current[1] += pre_rotated_current[1];	//2
			temp_current[2] += pre_rotated_current[2];	//12

			//Now add in power contributions
			temp_current[0] += voltage[0] == 0.0 ? 0.0 : ~(power[0]/voltage[0]);
			temp_current[1] += voltage[1] == 0.0 ? 0.0 : ~(power[1]/voltage[1]);
			temp_current[2] += vdel == 0.0 ? 0.0 : ~(power[2]/vdel);

			if (house_present)	//House present
			{
				//Update phase adjustments
				temp_store[0].SetPolar(1.0,voltage[0].Arg());	//Pull phase of V1
				temp_store[1].SetPolar(1.0,voltage[1].Arg());	//Pull phase of V2
				temp_store[2].SetPolar(1.0,vdel.Arg());		//Pull phase of V12

				//Update these current contributions
				house_pres_current[0] = nom_res_curr[0]/(~temp_store[0]);		//Just denominator conjugated to keep math right (rest was conjugated in house)
				house_pres_current[1] = nom_res_curr[1]/(~temp_store[1]);
				house_pres_current[2] = nom_res_curr[2]/(~temp_store[2]);

				//Now add it into the current contributions
				temp_current[0] += house_pres_current[0];
				temp_current[1] += house_pres_current[1];
				temp_current[2] += house_pres_current[2];
			}//End house-attached splitphase

			//Last, but not least, admittance/impedance contributions
			temp_current[0] += shunt[0]*voltage[0];
			temp_current[1] += shunt[1]*voltage[1];
			temp_current[2] += shunt[2]*vdel;

			//Convert 'em to line currents
			temp_current_val[0] = temp_current[0] + temp_current[2];
			temp_current_val[1] = -temp_current[1] - temp_current[2];

			temp_current_inj[0] = temp_current_val[0];
			temp_current_inj[1] = temp_current_val[1];

			//Get information
			if ((Triplex_Data != nullptr) && ((Triplex_Data[0] != 0.0) || (Triplex_Data[1] != 0.0)))
			{
				/* normally the calc would not be inside the lock, but it's reflexive so that's ok */
				temp_current_inj[2] = Triplex_Data[0]*temp_current_inj[0] + Triplex_Data[1]*temp_current_inj[1];
			}
			else
			{
				/* normally the calc would not be inside the lock, but it's reflexive so that's ok */
				temp_current_inj[2] = ((voltage1.IsZero() || (power1.IsZero() && shunt1.IsZero())) ||
								   (voltage2.IsZero() || (power2.IsZero() && shunt2.IsZero())))
									? currentN : -((temp_current_inj[0]-temp_current[2])+(temp_current_inj[1]-temp_current[2]));
			}
		}
		else					//Wye connection
		{
			//Adjust constant current values
			//Loop through the phases
			for (loop_index=0; loop_index<3; loop_index++)
			{
				//Check existence of phases and adjust the currents appropriately
				if (voltage[loop_index] != 0.0)
				{
					adjusted_current_val[loop_index] =  ~((assumed_nominal_voltage[loop_index]*~current[loop_index]*voltage[loop_index].Mag())/(voltage[loop_index]*nominal_voltage));
				}
				else
				{
					adjusted_current_val[loop_index] = gld::complex(0.0,0.0);
				}
			}

			//PQP needs power converted to current
			//PQZ needs load currents calculated as well
			//Update load current values if PQI
			temp_current_val[0] = ((voltage[0]==0) ? gld::complex(0,0) : ~(power[0]/voltage[0])) + voltage[0]*shunt[0] + adjusted_current_val[0] + pre_rotated_current[0];
			temp_current_val[1] = ((voltage[1]==0) ? gld::complex(0,0) : ~(power[1]/voltage[1])) + voltage[1]*shunt[1] + adjusted_current_val[1] + pre_rotated_current[1];
			temp_current_val[2] = ((voltage[2]==0) ? gld::complex(0,0) : ~(power[2]/voltage[2])) + voltage[2]*shunt[2] + adjusted_current_val[2] + pre_rotated_current[2];

			temp_current_inj[0] = temp_current_val[0];
			temp_current_inj[1] = temp_current_val[1];
			temp_current_inj[2] = temp_current_val[2];
		}

		//Explicit delta-wye portions (do both) -- make sure not triplex though
		if (!(has_phase(PHASE_S)))
		{
			//Do delta-connected portions

			//Convert delta connected impedance
			delta_shunt[0] = voltaged[0]*shunt_dy[0];
			delta_shunt[1] = voltaged[1]*shunt_dy[1];
			delta_shunt[2] = voltaged[2]*shunt_dy[2];

			//Convert delta connected power
			delta_current[0]= (voltaged[0]==0) ? gld::complex(0,0) : ~(power_dy[0]/voltaged[0]);
			delta_current[1]= (voltaged[1]==0) ? gld::complex(0,0) : ~(power_dy[1]/voltaged[1]);
			delta_current[2]= (voltaged[2]==0) ? gld::complex(0,0) : ~(power_dy[2]/voltaged[2]);

			//Adjust constant current values of delta-connected
			//Loop through the phases
			for (loop_index=0; loop_index<3; loop_index++)
			{
				//Check existence of phases and adjust the currents appropriately
				if (voltaged[loop_index] != 0.0)
				{
					adjusted_current_val[loop_index] =  ~((assumed_nominal_voltage[loop_index+3]*~current_dy[loop_index]*voltaged[loop_index].Mag())/(voltaged[loop_index]*nominal_voltage_dval));
				}
				else
				{
					adjusted_current_val[loop_index] = gld::complex(0.0,0.0);
				}
			}

			//Put into accumulator
			temp_current_inj[0] += delta_shunt[0]-delta_shunt[2] + delta_current[0]-delta_current[2] + adjusted_current_val[0]-adjusted_current_val[2];
			temp_current_inj[1] += delta_shunt[1]-delta_shunt[0] + delta_current[1]-delta_current[0] + adjusted_current_val[1]-adjusted_current_val[0];
			temp_current_inj[2] += delta_shunt[2]-delta_shunt[1] + delta_current[2]-delta_current[1] + adjusted_current_val[2]-adjusted_current_val[1];

			//Adjust constant current values for Wye-connected
			//Loop through the phases
			for (loop_index=0; loop_index<3; loop_index++)
			{
				if (voltage[loop_index] != 0.0)
				{
					adjusted_current_val[loop_index] =  ~((assumed_nominal_voltage[loop_index]*~current_dy[loop_index+3]*voltage[loop_index].Mag())/(voltage[loop_index]*nominal_voltage));
				}
				else
				{
					adjusted_current_val[loop_index] = gld::complex(0.0,0.0);
				}
			}

			//Now put in Wye components
			temp_current_inj[0] += ((voltage[0]==0) ? gld::complex(0,0) : ~(power_dy[3]/voltage[0])) + voltage[0]*shunt_dy[3] + adjusted_current_val[0];
			temp_current_inj[1] += ((voltage[1]==0) ? gld::complex(0,0) : ~(power_dy[4]/voltage[1])) + voltage[1]*shunt_dy[4] + adjusted_current_val[1];
			temp_current_inj[2] += ((voltage[2]==0) ? gld::complex(0,0) : ~(power_dy[5]/voltage[2])) + voltage[2]*shunt_dy[5] + adjusted_current_val[2];

			//Explicit house inclusion for "non-triplex" houses
			if (house_present)	//House present
			{
				//Do it in a loop, just because
				for (loop_index=0; loop_index<3; loop_index++)
				{
					//See if we're even a valid voltage
					if (voltage[loop_index].Mag() != 0.0)
					{
						//Update phase adjustments
						temp_store[loop_index].SetPolar(1.0,voltage[loop_index].Arg());	//Pull phase of this voltage phase

						//Update these current contributions
						house_pres_current[loop_index] = nom_res_curr[loop_index]/(~temp_store[loop_index]);		//Just denominator conjugated to keep math right (rest was conjugated in house)

						//Now add it into the current contributions
						temp_current_inj[loop_index] += ((voltage[loop_index]==0) ? gld::complex(0,0) : house_pres_current[loop_index]);
					}
					//Default else - voltage is zero, do nothing
				}//End phase loop for houses
			}//End house-attached not-so-split-phase
		}//End both delta/wye

		//Update our accumulator as well, otherwise things break
		current_inj[0] += temp_current_inj[0];
		current_inj[1] += temp_current_inj[1];
		current_inj[2] += temp_current_inj[2];

		//See if we're delta-enabled and need to accumulate those items
		//Only do this if we don't have a "greater descendant", since they'll populate it up through current_inj
		if (deltamode_inclusive && dynamic_norton && !dynamic_norton_child)
		{
			//Injections from generators -- always accumulate (may be zero)
			current_inj[0] -= deltamode_dynamic_current[0];
			current_inj[1] -= deltamode_dynamic_current[1];
			current_inj[2] -= deltamode_dynamic_current[2];

			//See if the shunt exists
			if (full_Y != nullptr)
			{
				//This assumes three-phase right now
				current_inj[0] += full_Y[0]*voltage[0] + full_Y[1]*voltage[1] + full_Y[2]*voltage[2];
				current_inj[1] += full_Y[3]*voltage[0] + full_Y[4]*voltage[1] + full_Y[5]*voltage[2];
				current_inj[2] += full_Y[6]*voltage[0] + full_Y[7]*voltage[1] + full_Y[8]*voltage[2];
			}
		}

		//If we are a child, apply our current injection directly up to our parent - links accumulate afterwards now because they bypass child relationships
		if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) != 0)
		{
			//Link to our parent
			node *ParLoadObj=OBJECTDATA(obj->parent,node);

			if (!(ParLoadObj->current_accumulated))	//Locking not needed here - if parent hasn't accumulated yet, it is the one that called us (rank split)
			{
				ParLoadObj->current_inj[0] += current_inj[0];
				ParLoadObj->current_inj[1] += current_inj[1];
				ParLoadObj->current_inj[2] += current_inj[2];

				//See if we have a house - if we do, we're inadvertently biasing our parent
				//Not only a house, but a triplex-connected house
				if ((house_present) && has_phase(PHASE_S))
				{
					//Remove the line_12 current appropriately
					ParLoadObj->current_inj[0] -= house_pres_current[0] + house_pres_current[2];
					ParLoadObj->current_inj[1] -= -house_pres_current[1] - house_pres_current[2];
				}
			}
		}
		//Default else - not a child - no need to propagate

		//Flag us as done
		current_accumulated = true;
	}//End not current already handled

	return 1;	//Always successful
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Module-level call
SIMULATIONMODE node::inter_deltaupdate_node(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	//unsigned char pass_mod;
	double deltat;
	OBJECT *hdr = OBJECTHDR(this);
	STATUS return_status_val;

	////See what we're on, for tracking
	//pass_mod = iteration_count_val - ((iteration_count_val >> 1) << 1);

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

	//Determine what to run
	if (!interupdate_pos)	//Before powerflow call
	{
		//Call presync-equivalent items
		NR_node_presync_fxn(0);

		//Call sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		return SM_DELTA;	//Just return something other than SM_ERROR for this call

	}//End Before NR solver (or inclusive)
	else	//After the call
	{
		//Perform postsync-like updates on the values
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

		//No control required at this time - powerflow defers to the whims of other modules
		//Code below implements predictor/corrector-type logic, even though it effectively does nothing
		//return SM_EVENT;

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
	}//End "After NR solver" branch
}

//Performs the frequency measurement calculations in a nice, compact form
//for easy copy-paste
STATUS node::calc_freq_dynamics(double deltat)
{
	unsigned char phase_conf, phase_mask;
	unsigned int indexval;
	double dfmeasdt, errorval, deltaom, dxdt, fbus, adiffval;
	STATUS return_status;
	bool is_triplex_node;

	//Init for success
	return_status = SUCCESS;

	//Extract the phases
	if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
	{
		if ((NR_busdata[NR_node_reference].phases & 0x80) == 0x80)
		{
			phase_conf = 0x80;
			is_triplex_node = true;
		}
		else
		{
			phase_conf=NR_busdata[NR_node_reference].phases & 0x07;
			is_triplex_node = false;
		}
	}
	else	//It is a child - look at parent
	{
		if ((NR_busdata[*NR_subnode_reference].phases & 0x80) == 0x80)
		{
			phase_conf = 0x80;
			is_triplex_node = true;
		}
		else
		{
			phase_conf=NR_busdata[*NR_subnode_reference].phases & 0x07;
			is_triplex_node = false;
		}
	}

	//Pull voltage updates
	for (indexval=0; indexval<3; indexval++)
	{
		if (is_triplex_node)
		{
			phase_mask = 0x80;
		}
		else	//three-phase, of some sort
		{
			//Get the mask
			phase_mask = (1 << (2 - indexval));
		}

		//Check the phase
		if ((phase_conf & phase_mask) == phase_mask)
		{
			//See if we're a parent or not
			if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
			{
				if (is_triplex_node)
				{
					if (indexval < 2)
					{
						curr_freq_state.voltage_val[indexval]=NR_busdata[NR_node_reference].V[indexval];
					}
					else	//Must be 2
					{
						curr_freq_state.voltage_val[indexval]=NR_busdata[NR_node_reference].V[0] + NR_busdata[NR_node_reference].V[1];
					}
				}
				else
				{
					curr_freq_state.voltage_val[indexval]=NR_busdata[NR_node_reference].V[indexval];
				}
			}
			else	//It is a child - look at parent
			{
				if (is_triplex_node)
				{
					if (indexval < 2)
					{
						curr_freq_state.voltage_val[indexval]=NR_busdata[*NR_subnode_reference].V[indexval];
					}
					else	//Must be 2
					{
						curr_freq_state.voltage_val[indexval]=NR_busdata[*NR_subnode_reference].V[0] + NR_busdata[*NR_subnode_reference].V[1];
					}
				}
				else
				{
					curr_freq_state.voltage_val[indexval]=NR_busdata[*NR_subnode_reference].V[indexval];
				}
			}

			//Extract the angle - use ATAN2, since it divides better than the complex-oriented .Arg function
			curr_freq_state.anglemeas[indexval] = atan2(curr_freq_state.voltage_val[indexval].Im(),curr_freq_state.voltage_val[indexval].Re());

			//Perform update
			if (fmeas_type == FM_SIMPLE)
			{
				//Use a function to get the difference
				adiffval = compute_angle_diff(curr_freq_state.anglemeas[indexval],prev_freq_state.anglemeas[indexval]);

				fbus = (adiffval/deltat + freq_omega_ref) / (2.0 * PI);
				dfmeasdt = (fbus - prev_freq_state.fmeas[indexval]) / freq_sfm_Tf;
				curr_freq_state.fmeas[indexval] = prev_freq_state.fmeas[indexval] + dfmeasdt*deltat;
			}
			else if (fmeas_type == FM_PLL)
			{
				//Compute error - make sure isn't zero - if it is, ignore this
				if (prev_freq_state.voltage_val[indexval].Mag() > 0.0)
				{
					errorval = prev_freq_state.voltage_val[indexval].Re()*prev_freq_state.sinangmeas[indexval];
					errorval -= prev_freq_state.voltage_val[indexval].Im()*prev_freq_state.cosangmeas[indexval];
					errorval /= prev_freq_state.voltage_val[indexval].Mag();
					errorval *= -1.0;
				}
				else
				{
					errorval = 0.0;
				}

				deltaom = freq_pll_Kp * errorval + prev_freq_state.x[indexval];
				dxdt = freq_pll_Ki * errorval;

				//Updates
				curr_freq_state.fmeas[indexval] = (freq_omega_ref + deltaom) / (2.0 * PI);
				curr_freq_state.x[indexval] = prev_freq_state.x[indexval] + dxdt*deltat;
				curr_freq_state.anglemeas[indexval] = prev_freq_state.anglemeas[indexval] + deltaom*deltat;

				//Update trig values
				curr_freq_state.sinangmeas[indexval] = sin(curr_freq_state.anglemeas[indexval]);
				curr_freq_state.cosangmeas[indexval] = cos(curr_freq_state.anglemeas[indexval]);
			}
			//Default else -- other method (or not a valid method)
		}//End valid phase
		else	//Not a valid phase
		{
			curr_freq_state.fmeas[indexval] = 0.0;
		}
	}//End phase loop

	//Update the "average" frequency value
	switch(phase_conf) {
		case 0x00:	//No phases (we've been faulted out
			{
				curr_freq_state.average_freq = 0.0;
				break;	//Just get us outta here
			}
		case 0x01:	//Only C
			{
				curr_freq_state.average_freq = curr_freq_state.fmeas[2];
				break;
			}
		case 0x02:	//Only B
			{
				curr_freq_state.average_freq = curr_freq_state.fmeas[1];
				break;
			}
		case 0x03:	//B & C
			{
				curr_freq_state.average_freq = (curr_freq_state.fmeas[1] + curr_freq_state.fmeas[2]) / 2.0;
				break;
			}
		case 0x04:	//Only A
			{
				curr_freq_state.average_freq = curr_freq_state.fmeas[0];
				break;
			}
		case 0x05:	//A & C
			{
				curr_freq_state.average_freq = (curr_freq_state.fmeas[0] + curr_freq_state.fmeas[2]) / 2.0;
				break;
			}
		case 0x06:	//A & B
			{
				curr_freq_state.average_freq = (curr_freq_state.fmeas[0] + curr_freq_state.fmeas[1]) / 2.0;
				break;
			}
		case 0x07:	//ABC
			{
				curr_freq_state.average_freq = (curr_freq_state.fmeas[0] + curr_freq_state.fmeas[1] + curr_freq_state.fmeas[2]) / 3.0;
				break;
			}
		case 0x80:	//Triplex stuff
			{
				curr_freq_state.average_freq = curr_freq_state.fmeas[2];	//Just take the 12 value, for now
				break;
			}
		default:	//How'd we get here?
			{
				gl_error("Node frequency update: unknown state encountered");
				/*  TROUBLESHOOT
				While running the frequency/angle estimation routine for a node object, it somehow entered an unknown state.
				Please try again.  If the error persists, please submit your GLM and a bug report via the ticketing system.
				*/

				return_status = FAILED;

			break;
			}
	}	//switch end

	return return_status;
}

//Initializes dynamic equations for first entry
//Returns a SUCCESS/FAIL
//curr_time is the initial states/information
void node::init_freq_dynamics(double deltat)
{
	unsigned char phase_conf, phase_mask;
	int indexval;
	bool is_triplex_node;
	double frequency_offset_val, angle_offset;
	gld::complex angle_rotate_value;

	//Extract the phases
	if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
	{
		if ((NR_busdata[NR_node_reference].phases & 0x80) == 0x80)
		{
			phase_conf = 0x80;
			is_triplex_node = true;
		}
		else
		{
			phase_conf=NR_busdata[NR_node_reference].phases & 0x07;
			is_triplex_node = false;
		}
	}
	else	//It is a child - look at parent
	{
		if ((NR_busdata[*NR_subnode_reference].phases & 0x80) == 0x80)
		{
			phase_conf = 0x80;
			is_triplex_node = true;
		}
		else
		{
			phase_conf=NR_busdata[*NR_subnode_reference].phases & 0x07;
			is_triplex_node = false;
		}
	}

	//Pull voltage updates
	for (indexval=0; indexval<3; indexval++)
	{
		if (is_triplex_node)
		{
			phase_mask = 0x80;
		}
		else	//three-phase, of some sort
		{
			//Get the mask
			phase_mask = (1 << (2 - indexval));
		}

		//Check the phase
		if ((phase_conf & phase_mask) == phase_mask)
		{
			//See what our reference is - steal parent voltages if needed
			if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
			{
				if (is_triplex_node)
				{
					if (indexval < 2)
					{
						curr_freq_state.voltage_val[indexval]=NR_busdata[NR_node_reference].V[indexval];
					}
					else	//Must be 2
					{
						curr_freq_state.voltage_val[indexval]=NR_busdata[NR_node_reference].V[0] + NR_busdata[NR_node_reference].V[1];
					}
				}
				else
				{
					curr_freq_state.voltage_val[indexval]=NR_busdata[NR_node_reference].V[indexval];
				}
			}
			else	//It is a child - look at parent
			{
				if (is_triplex_node)
				{
					if (indexval < 2)
					{
						curr_freq_state.voltage_val[indexval]=NR_busdata[*NR_subnode_reference].V[indexval];
					}
					else	//Must be 2
					{
						curr_freq_state.voltage_val[indexval]=NR_busdata[*NR_subnode_reference].V[0] + NR_busdata[*NR_subnode_reference].V[1];
					}
				}
				else
				{
					curr_freq_state.voltage_val[indexval]=NR_busdata[*NR_subnode_reference].V[indexval];
				}
			}

			//See if we're the first start or not
			if (first_freq_init)
			{
				//Assume "current" start
				curr_freq_state.fmeas[indexval] = current_frequency;
				prev_freq_state.fmeas[indexval] = current_frequency;
			}
			//Default else, just leave what was already in our accumulator

			//Assume the frequency was at least "stable" prior to entering - adjust prior value to reflect this
			//Compute the frequency deviation
			frequency_offset_val = (curr_freq_state.fmeas[indexval] * 2.0 * PI - freq_omega_ref);

			//Calculate the associated angle offset - negative, for past
			angle_offset = -1.0 * frequency_offset_val * deltat;

			//Translate it into a multiplier
			//exp(jx) = cos(x)+j*sin(x)
			angle_rotate_value = gld::complex(cos(angle_offset),sin(angle_offset));

			//Adjust the previous voltage value by this
			prev_freq_state.voltage_val[indexval] = curr_freq_state.voltage_val[indexval] * angle_rotate_value;

			//Populate the angle - use ATAN2, since it divides better than the complex-oriented .Arg function
			curr_freq_state.anglemeas[indexval] = atan2(curr_freq_state.voltage_val[indexval].Im(),curr_freq_state.voltage_val[indexval].Re());
			prev_freq_state.anglemeas[indexval] = atan2(prev_freq_state.voltage_val[indexval].Im(),prev_freq_state.voltage_val[indexval].Re());

			//Populate other fields, if necessary
			if (fmeas_type == FM_PLL)
			{
				//Initialize other relevant variables
				curr_freq_state.x[indexval] = frequency_offset_val;
				prev_freq_state.x[indexval] = frequency_offset_val;

				//Current values of angles
				curr_freq_state.sinangmeas[indexval] = sin(curr_freq_state.anglemeas[indexval]);
				curr_freq_state.cosangmeas[indexval] = cos(curr_freq_state.anglemeas[indexval]);

				//Previous values, just because
				prev_freq_state.sinangmeas[indexval] = sin(prev_freq_state.anglemeas[indexval]);
				prev_freq_state.cosangmeas[indexval] = cos(prev_freq_state.anglemeas[indexval]);
			}
		}//End valid phase
		else	//Not a valid phase, just zero it all
		{
			curr_freq_state.voltage_val[indexval] = gld::complex(0.0,0.0);
			curr_freq_state.x[indexval] = 0.0;
			curr_freq_state.anglemeas[indexval] = 0.0;
			curr_freq_state.fmeas[indexval] = 0.0;
			curr_freq_state.average_freq = current_frequency;
			curr_freq_state.sinangmeas[indexval] = 0.0;
			curr_freq_state.cosangmeas[indexval] = 0.0;

			//DO the same for previous state, out of paranoia
			prev_freq_state.voltage_val[indexval] = gld::complex(0.0,0.0);
			prev_freq_state.x[indexval] = 0.0;
			prev_freq_state.anglemeas[indexval] = 0.0;
			prev_freq_state.fmeas[indexval] = 0.0;
			prev_freq_state.average_freq = current_frequency;
			prev_freq_state.sinangmeas[indexval] = 0.0;
			prev_freq_state.cosangmeas[indexval] = 0.0;
		}
	}//End FOR loop

	//Final check for "first run" to deflag us and to update angle calculation
	if (first_freq_init)
	{
		first_freq_init = false;
	}
}

//Function to perform the GFA-type responses
double node::perform_GFA_checks(double timestepvalue)
{
	bool voltage_violation, frequency_violation, trigger_disconnect, check_phase;
	double temp_pu_voltage;
	double return_time_freq, return_time_volt, return_value;
	char indexval;
	unsigned char phasevals;
	OBJECT *hdr = OBJECTHDR(this);

	//By default, we're subject to the whims of deltamode
	return_time_freq = -1.0;
	return_time_volt = -1.0;
	return_value = -1.0;

	//Pull our phasing information
	if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
	{
		phasevals = NR_busdata[NR_node_reference].phases & 0x87;
	}
	else	//Must be a child, pull from our parent
	{
		phasevals = NR_busdata[*NR_subnode_reference].phases & 0x87;
	}

	//Perform frequency check
	if ((curr_freq_state.average_freq > GFA_freq_high_trip) || (curr_freq_state.average_freq < GFA_freq_low_trip))
	{
		//Flag it
		frequency_violation = true;

		//Increment "outage trigger" timer
		freq_violation_time_total += timestepvalue;

		//Reset "restoration" time
		out_of_violation_time_total = 0.0;

		//Check the times - split out
		if (curr_freq_state.average_freq > GFA_freq_high_trip)
		{
			if (freq_violation_time_total > GFA_freq_disconnect_time)
			{
				trigger_disconnect = true;
				return_time_freq = GFA_reconnect_time;

				//Flag us as over frequency
				GFA_trip_method = GFA_OF;
			}
			else
			{
				trigger_disconnect = false;
				return_time_freq = GFA_freq_disconnect_time - freq_violation_time_total;
			}
		}
		else if (curr_freq_state.average_freq < GFA_freq_low_trip)
		{
			if (freq_violation_time_total > GFA_freq_disconnect_time)
			{
				trigger_disconnect = true;
				return_time_freq = GFA_reconnect_time;

				//Flag as under-frequency
				GFA_trip_method = GFA_UF;
			}
			else
			{
				trigger_disconnect = false;
				return_time_freq = GFA_freq_disconnect_time - freq_violation_time_total;
			}
		}
		else
		{
			gl_error("Node%d %s GFA checks failed- invalid state!",hdr->id,hdr->name ? hdr->name : "Unnamed");
			/*  TROUBLESHOOT
			While performing the GFA update on the node, it entered an unknown state.  Please try again.
			If the error persists, please submit your code and a bug report via the ticketing system.
			*/
		}
	}
	else	//Must be in a good range
	{
		//Set flags to indicate as much
		frequency_violation = false;
		trigger_disconnect = false;

		//Zero the frequency violation indicator
		freq_violation_time_total = 0.0;

		//Flag the return, just to be safe
		return_time_freq = -1.0;
	}

	//Default to no voltage violation
	voltage_violation = false;

	//See if we're already triggered or in a frequency violation (no point checking, if we are)
	//Loop through voltages present & check - if we find one, we'll break out
	for (indexval = 0; indexval < 3; indexval++)
	{
		//See if this phase exists
		if ((phasevals & 0x80) == 0x80)	//Triplex check first
		{
			//Make sure we're one of the first two
			if (indexval < 2)
			{
				check_phase = true;
			}
			else
			{
				check_phase = false;
			}
		}//end triplex
		else if ((indexval == 0) && ((phasevals & 0x04) == 0x04))	//A
		{
			check_phase = true;
		}
		else if ((indexval == 1) && ((phasevals & 0x02) == 0x02))	//B
		{
			check_phase = true;
		}
		else if ((indexval == 2) && ((phasevals & 0x01) == 0x01))	//C
		{
			check_phase = true;
		}
		else	//Not a proper combination
		{
			check_phase = false;
		}

		//See if we were valid
		if (check_phase)
		{
			//See if it is a violation
			temp_pu_voltage = voltage[indexval].Mag()/nominal_voltage;

			//Check it
			if ((temp_pu_voltage < GFA_voltage_low_trip) || (temp_pu_voltage > GFA_voltage_high_trip))
			{
				//flag a violation
				voltage_violation = true;

				//Accumulate the time
				volt_violation_time_total += timestepvalue;

				//Clear the "no violation timer"
				out_of_violation_time_total = 0.0;

				//See which case we are
				if (temp_pu_voltage < GFA_voltage_low_trip)
				{
					if (volt_violation_time_total > GFA_volt_disconnect_time)
					{
						trigger_disconnect = true;
						return_time_volt = GFA_reconnect_time;

						//Flag us as under-voltage
						GFA_trip_method = GFA_UV;
					}
					else
					{
						trigger_disconnect = false;
						return_time_volt = GFA_volt_disconnect_time - volt_violation_time_total;
					}
				}
				else if (temp_pu_voltage >= GFA_voltage_high_trip)
				{
					if (volt_violation_time_total > GFA_volt_disconnect_time)
					{
						trigger_disconnect = true;
						return_time_volt = GFA_reconnect_time;

						//Flag us as over-voltage
						GFA_trip_method = GFA_OV;
					}
					else
					{
						trigger_disconnect = false;
						return_time_volt = GFA_volt_disconnect_time - volt_violation_time_total;
					}
				}
				else	//must not have tripped a time limit
				{
					gl_error("Node%d %s GFA checks failed- invalid state!",hdr->id,hdr->name ? hdr->name : "Unnamed");
					//Defined above
				}

				//At least one violation condition was met, break us out of the for
				break;
			}//End of a violation occurred
			//Default else, normal operating range - loop
		}//End was a valid phase
		//Default else - go to next phase
	}//End phase loop

	//See if we detected a voltage violation - if not, reset the counter
	if (!voltage_violation)
	{
		volt_violation_time_total = 0.0;
		return_time_volt = -1.0;	//Set it again, for paranoia
	}

	//COmpute the "next expected update"
	if ((return_time_volt > 0.0) && (return_time_freq > 0.0))	//Both counting - take the minimum
	{
		//Find the minimum
		if (return_time_volt < return_time_freq)
		{
			return_value = return_time_volt;
		}
		else
		{
			return_value = return_time_freq;
		}
	}
	else if ((return_time_volt > 0.0) && (return_time_freq < 0.0))	//Voltage event
	{
		return_value = return_time_volt;
	}
	else if ((return_time_volt < 0.0) && (return_time_freq > 0.0)) //Frequency event
	{
		return_value = return_time_freq;
	}
	else	//Nothing pending
	{
		return_value = -1.0;
	}

	//Check voltage values first
	if (frequency_violation || voltage_violation)
	{
		//Reset the out of violation time
		out_of_violation_time_total = 0.0;
	}
	else	//No failures, reset and increment
	{
		//Increment the "restoration" one, just in case
		out_of_violation_time_total += timestepvalue;
	}

	//See what we are - if we're out of service, see if we can be restored
	if (!GFA_status)
	{
		if (out_of_violation_time_total >= GFA_reconnect_time)
		{
			//Set us back into service
			GFA_status = true;

			//Reset us back to no violation
			GFA_trip_method = GFA_NONE;

			//Implies no violations, so force return a -1.0
			return -1.0;
		}
		else	//Still delayed, just reaffirm our status
		{
			GFA_status = false;

			//Calculate the new return time
			return_value = GFA_reconnect_time - out_of_violation_time_total;

			//Return the minimum from above
			return return_value;
		}
	}
	else	//We're true, see if we need to not be
	{
		if (trigger_disconnect)
		{
			GFA_status = false;	//Trigger

			//Return our expected next status interval
			return return_value;
		}
		else
		{
			//Re-affirm we are not having any issues
			GFA_trip_method = GFA_NONE;

			//All is well, indicate as much
			return return_value;
		}
	}
}

//Function to set a node's SWING status mid-simulation, without the SWING_PQ functionality
STATUS node::NR_swap_swing_status(bool desired_status)
{
	OBJECT *hdr = OBJECTHDR(this);

	//See if we're a child or not
	if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
	{
		//Make sure we're a SWING or SWING_PQ first
		if (NR_busdata[NR_node_reference].type > 1)
		{
			//Just set us to our status -- it is assumed that if you did this, you know what you are doing
			NR_busdata[NR_node_reference].swing_functions_enabled = desired_status;
		}
		else	//Indicate we're not one
		{
			//Put as a verbose, since mostly just useful for debugging
			gl_verbose("node:%s - Not a SWING-capable bus, so no swing status swap changed",(hdr->name ? hdr->name : "unnamed"));
			/*  TROUBLESHOOT
			While attempting to swap a node from being a "swing node", it was tried on a node that was not already a SWING or SWING_PQ
			node, which is not a valid attempt.
			*/
		}
	}
	else	//It is a child - look at parent
	{
		//Make sure we're a SWING or SWING_PQ first
		if (NR_busdata[*NR_subnode_reference].type > 1)
		{
			//Just set us to our status -- it is assumed that if you did this, you know what you are doing
			NR_busdata[*NR_subnode_reference].swing_functions_enabled = desired_status;
		}
		else	//Indicate we're not one
		{
			gl_verbose("node:%s - Not a SWING-capable bus, so no swing status swap changed",(hdr->name ? hdr->name : "unnamed"));
			//Defined above
		}
	}

	//Always a success, we think
	return SUCCESS;
}

//Function to check if a node object is "behaving in a SWING manner"
void node::NR_swing_status_check(bool *swing_status_check_value, bool *swing_pq_status_value)
{
	//By default, assume we aren't a SWING
	*swing_status_check_value = false;

	//See if we're a child or not
	if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
	{
		//Make sure we're a SWING or SWING_PQ first
		if (NR_busdata[NR_node_reference].type > 1)
		{
			//Pull it out of our NR structure
			*swing_status_check_value = NR_busdata[NR_node_reference].swing_functions_enabled;

			//See if we are a SWING_PQ, then copy that flag
			if (NR_busdata[NR_node_reference].type == 3)
			{
				*swing_pq_status_value = NR_busdata[NR_node_reference].swing_topology_entry;
			}
			else
			{
				//Swing and "normal bus" just get set to false - not needed
				*swing_pq_status_value = false;
			}
		}
		//Default else - we're a PQ, and by definition, not a SWING
	}
	else	//It is a child - look at parent
	{
		//Make sure we're a SWING or SWING_PQ first
		if (NR_busdata[*NR_subnode_reference].type > 1)
		{
			//Pull the value out
			*swing_status_check_value = NR_busdata[*NR_subnode_reference].swing_functions_enabled;

			//See if we are a SWING_PQ, then copy that flag
			if (NR_busdata[*NR_subnode_reference].type == 3)
			{
				*swing_pq_status_value = NR_busdata[*NR_subnode_reference].swing_topology_entry;
			}
			else
			{
				//Swing and "normal bus" just get set to false - not needed
				*swing_pq_status_value = false;
			}
		}
		//Default else - we're a PQ and not a SWING of any type
	}
}

//Function to reset the "disabled state" of the node, if called (re-enable an island, basically)
STATUS node::reset_node_island_condition(void)
{
	OBJECT *obj = OBJECTHDR(this);
	STATUS temp_status;
	FUNCTIONADDR temp_fxn_val;
	int node_calling_reference;

	//Deflag the overall variable, just in case
	reset_island_state = false;

	//Check and see if this was even something we wanted to do
	if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
	{
		//Check our status
		if (NR_busdata[NR_node_reference].island_number != -1)
		{
			gl_warning("node:%d - %s - Tried to re-enable a node that wasn't disabled!",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to reset a node from a "disabled island" state, it was not already disabled.  Therefore, no action occurred.
			*/

			//Return a "failure" - just in case
			return FAILED;
		}
		else
		{
			//Reset our phases (assume full-on reset)
			NR_busdata[NR_node_reference].phases = NR_busdata[NR_node_reference].origphases;
		}
	}
	else	//We're a child, see if our parent is appropriately disabled
	{
		if (NR_busdata[*NR_subnode_reference].island_number != -1)
		{
			gl_warning("node:%d - %s - Tried to re-enable a node that wasn't disabled!",obj->id,(obj->name ? obj->name : "Unnamed"));
			//Defined above

			//Return the failure
			return FAILED;
		}
		else
		{
			//Reset our parent's phases
			NR_busdata[*NR_subnode_reference].phases = NR_busdata[*NR_subnode_reference].origphases;
		}
	}

	//If we made it this far, we've been "re-validated" - call the reset function
	//Make sure it will even remotely work first
	if (fault_check_object == nullptr)	//Make sure we actually have a fault check object
	{
		//Hard throw this one -- this can't be ignored
		GL_THROW("node:%d - %s -- Island condition reset failed - no fault_check object mapped!",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		A node attempted to reset its "disabled from powerflow/islands" status, but there is no fault_check object in the system.  It's unclear
		how this call was made.  Please check your system and try again.  If this message was encountered in error, please submit your code, models, and
		a bug report via the issue tracker system.
		*/
	}

	//Map the function
	temp_fxn_val = (FUNCTIONADDR)(gl_get_function(fault_check_object,"rescan_topology"));

	//Make sure it worked
	if (temp_fxn_val == nullptr)
	{
		//Another hard failure
		GL_THROW("node:%d - %s -- Island condition reset failed - reset function mapping failed!",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to call the topology rescan functionality, an error occurred.  Please check your model and try again.  If the error persists,
		please submit your code, models, and a bug report via the issue tracking system.
		*/
	}

	//Set it up as a "SWING-related call" - basically start over
	node_calling_reference = -99;

	//Then call the reset
	temp_status = ((STATUS (*)(OBJECT *,int))(temp_fxn_val))(fault_check_object,node_calling_reference);

	//Just pass its result
	return temp_status;
}

//Function to perform a mapping of the "internal iteration" current injection update
//Primarily used for deltamode and voltage-source inverters, but could be used in other places
STATUS node::NR_map_current_update_function(OBJECT *callObj)
{
	OBJECT *hdr = OBJECTHDR(this);
	OBJECT *phdr = nullptr;

	//Do a simple check -- if we're not in NR, this won't do anything anyways
	if (solver_method == SM_NR)
	{
		//See if we're a pesky child
		if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) == 0)
		{
			//Make sure no one has mapped us yet
			if (NR_busdata[NR_node_reference].ExtraCurrentInjFunc == nullptr)
			{
				//Map the function
				NR_busdata[NR_node_reference].ExtraCurrentInjFunc = (FUNCTIONADDR)(gl_get_function(callObj,"current_injection_update"));

				//Make sure it worked
				if (NR_busdata[NR_node_reference].ExtraCurrentInjFunc == nullptr)
				{
					gl_error("node:%d - %s - Failed to map current_injection_update from calling object:%d - %s",hdr->id,(hdr->name ? hdr->name : "Unnamed"),callObj->id,(callObj->name ? callObj->name : "Unnamed"));
					/*  TROUBLESHOOT
					The attached node was unable to find the exposed function "current_injection_update" on the calling object.  Be sure
					it supports this functionality and try again.
					*/

					return FAILED;
				}
				//Default else -- it worked

				//Store the object pointer too
				NR_busdata[NR_node_reference].ExtraCurrentInjFuncObject = callObj;
			}
			else	//Already mapped
			{
				gl_error("node:%d - %s - Already has an extra current injection function mapped",hdr->id,(hdr->name ? hdr->name : "Unnamed"));
				/*  TROUBLESHOOT
				An object attempted to map a current injection update function, but that node already has such a function mapped.  Only one
				is allowed per node.  Note this includes any attachments to child nodes, since those end up on the parent object.
				*/

				return FAILED;
			}//End already mapped
		}
		else	//Child - push this to the parent
		{
			//Map the parent - mostly just so we have a shorter variable name
			phdr = NR_busdata[*NR_subnode_reference].obj;

			//Make sure no one has mapped us yet
			if (NR_busdata[*NR_subnode_reference].ExtraCurrentInjFunc == nullptr)
			{
				//Map the function
				NR_busdata[*NR_subnode_reference].ExtraCurrentInjFunc = (FUNCTIONADDR)(gl_get_function(callObj,"current_injection_update"));

				//Make sure it worked
				if (NR_busdata[*NR_subnode_reference].ExtraCurrentInjFunc == nullptr)
				{
					gl_error("node:%d - %s - Failed to map current_injection_update from calling object:%d - %s",hdr->id,(hdr->name ? hdr->name : "Unnamed"),callObj->id,(callObj->name ? callObj->name : "Unnamed"));
					//Defined above

					return FAILED;
				}
				//Default else -- it worked

				//Store the object pointer too
				NR_busdata[*NR_subnode_reference].ExtraCurrentInjFuncObject = callObj;
			}
			else	//Already mapped
			{
				gl_error("node:%d - %s - Parent node:%d - %s - Already has an extra current injection function mapped",hdr->id,(hdr->name ? hdr->name : "Unnamed"),phdr->id,(phdr->name ? phdr->name : "Unnamed"));
				/*  TROUBLESHOOT
				An object attempted to map a current injection update function to its parent, but that node already has such a function mapped.  Only one
				is allowed per node.  Note this includes any attachments to child nodes, since those end up on the parent object.
				*/

				return FAILED;
			}//End already mapped
		}
	}
	else	//Other method
	{
		gl_warning("node:%d - %s - Attempted to map an NR-based function, but is not using an NR solver",hdr->id,(hdr->name ? hdr->name : "Unnamed"));
		/*  TROUBLESHOOT
		An object just attempted to map a current injection update function that only works with the Newton-Raphson solver, but that
		method is not being used.
		*/

		//Succeed, mostly because it just won't do anything
		return SUCCESS;
	}

	//Theoretically only get here if we succeed a map
	return SUCCESS;
}

//VFD linking/mapping function
STATUS node::link_VFD_functions(OBJECT *linkVFD)
{
	OBJECT *obj = OBJECTHDR(this);

	//Set the VFD object
	VFD_object = linkVFD;

	//Try mapping the VFD current injection function
	VFD_updating_function = (FUNCTIONADDR)(gl_get_function(linkVFD,"vfd_current_injection_update"));

	//Make sure it worked
	if (VFD_updating_function == nullptr)
	{
		gl_warning("Failure to map VFD current injection update for device:%s",(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		Attempts to map a function for the proper VFD updates did not work.  Please try again, making sure
		this node is connected to a VFD properly.  If the error persists, please submit an issue ticket.
		*/

		//Fail us
		return FAILED;
	}

	//Flag us as successful
	VFD_attached = true;

	//Always succeed, if we made it this far
	return SUCCESS;
}

//Function to compute the difference between two angles
//Note that the inputs are in radians
//Mainly used by frequency calculations
//Effectively the code from https://stackoverflow.com/questions/11498169/dealing-with-angle-wrap-in-c-code
//Should do "B-A"
double node::compute_angle_diff(double angle_B, double angle_A)
{
	double diff_val, two_PI;

	//Constant - save a multiply
	two_PI = 2.0 * PI;

	//Compute the raw difference
	diff_val = fmod((angle_B - angle_A + PI), two_PI);

   //See if we need to wrap it around
   if (diff_val < 0.0)
   {
	   diff_val += two_PI;
   }

   //Return the result, with the offset removed
   return (diff_val - PI);
}

//Function to perform the shunt update for FPI
STATUS node::shunt_update_fxn(void)
{
	bool local_shunt_update;
	int loop_index_var;
	gld::complex intermed_impedance_dy[6];
	gld::complex intermed_impedance[3];
	gld::complex working_impedance_value;

	//FPIM "convergence check" stuff
	if (NR_solver_algorithm == NRM_FPI)
	{
		//Do the explicit Delta-Wye connections first - update flag
		local_shunt_update = false;

		//Loop through
		for (loop_index_var=0; loop_index_var<6; loop_index_var++)
		{
			intermed_impedance_dy[loop_index_var] = shunt_dy[loop_index_var] - shunt_change_check_dy[loop_index_var];

			//Check it
			if (intermed_impedance_dy[loop_index_var].Mag() > 0.0)
			{
				NR_FPI_imp_load_change = true;
				local_shunt_update = true;
			}
		}

		//See if any updated
		if (local_shunt_update)
		{
			//Update the matrix - both delta and Wye portions
			NR_busdata[NR_node_reference].full_Y_load[0] += intermed_impedance_dy[0] + intermed_impedance_dy[2] + intermed_impedance_dy[3];
			NR_busdata[NR_node_reference].full_Y_load[1] -= intermed_impedance_dy[0];
			NR_busdata[NR_node_reference].full_Y_load[2] -= intermed_impedance_dy[2];
			NR_busdata[NR_node_reference].full_Y_load[3] -= intermed_impedance_dy[0];
			NR_busdata[NR_node_reference].full_Y_load[4] += intermed_impedance_dy[1] + intermed_impedance_dy[0] + intermed_impedance_dy[4];
			NR_busdata[NR_node_reference].full_Y_load[5] -= intermed_impedance_dy[1];
			NR_busdata[NR_node_reference].full_Y_load[6] -= intermed_impedance_dy[2];
			NR_busdata[NR_node_reference].full_Y_load[7] -= intermed_impedance_dy[1];
			NR_busdata[NR_node_reference].full_Y_load[8] += intermed_impedance_dy[2] + intermed_impedance_dy[1] + intermed_impedance_dy[5];
		}

		if (has_phase(PHASE_D))
		{
			//Reset flag
			local_shunt_update = false;

			//Loop through and check for "differences"
			for (loop_index_var=0; loop_index_var<3; loop_index_var++)
			{
				//Compute the difference
				intermed_impedance[loop_index_var] = shunt[loop_index_var] - shunt_change_check[loop_index_var];

				//Check it
				if (intermed_impedance[loop_index_var].Mag() > 0.0)
				{
					NR_FPI_imp_load_change = true;
					local_shunt_update = true;
				}
			}

			//Perform the update if we changed - if not, no reason to do so
			if (local_shunt_update)
			{
				//Update the matrix
				NR_busdata[NR_node_reference].full_Y_load[0] += intermed_impedance[0] + intermed_impedance[2];
				NR_busdata[NR_node_reference].full_Y_load[1] -= intermed_impedance[0];
				NR_busdata[NR_node_reference].full_Y_load[2] -= intermed_impedance[2];
				NR_busdata[NR_node_reference].full_Y_load[3] -= intermed_impedance[0];
				NR_busdata[NR_node_reference].full_Y_load[4] += intermed_impedance[1] + intermed_impedance[0];
				NR_busdata[NR_node_reference].full_Y_load[5] -= intermed_impedance[1];
				NR_busdata[NR_node_reference].full_Y_load[6] -= intermed_impedance[2];
				NR_busdata[NR_node_reference].full_Y_load[7] -= intermed_impedance[1];
				NR_busdata[NR_node_reference].full_Y_load[8] += intermed_impedance[2] + intermed_impedance[1];
			}
			//Default else - no update, so no need to recalc anything

			//Extra loop, if a different parent
			if ((SubNode & SNT_DIFF_PARENT) == SNT_DIFF_PARENT)
			{
				//Loop through and check for "differences"
				for (loop_index_var=0; loop_index_var<3; loop_index_var++)
				{
					//Compute the difference
					intermed_impedance[loop_index_var] = Extra_Data[loop_index_var+3] - Extra_Data_Track_FPI[loop_index_var];

					//Check it
					if (intermed_impedance[loop_index_var].Mag() > 0.0)
					{
						NR_FPI_imp_load_change = true;

						//Apply the update - Wye update (since different)
						NR_busdata[NR_node_reference].full_Y_load[loop_index_var*3+loop_index_var] += intermed_impedance[loop_index_var];

						//Update the tracking value
						Extra_Data_Track_FPI[loop_index_var] = Extra_Data[loop_index_var+3];
					}
				}
			}//End DIFF_PARENT - Wye on a Delta
		}//End has D
		else if (has_phase(PHASE_S))
		{
			//Loop through and see if there are any differences
			for (loop_index_var=0; loop_index_var<3; loop_index_var++)
			{
				//Compute the difference
				working_impedance_value = shunt[loop_index_var] - shunt_change_check[loop_index_var];

				//Check it
				if (working_impedance_value.Mag() > 0.0)
				{
					NR_FPI_imp_load_change = true;

					//See if it is one of the single-phase portions, or the "delta"
					if (loop_index_var<2)
					{
						//Update the matrix
						NR_busdata[NR_node_reference].full_Y_load[loop_index_var*3+loop_index_var] += working_impedance_value;
					}
					else	//12 connection
					{
						//Update the matrix
						NR_busdata[NR_node_reference].full_Y_load[0] += working_impedance_value;
						NR_busdata[NR_node_reference].full_Y_load[1] += working_impedance_value;
						NR_busdata[NR_node_reference].full_Y_load[3] += working_impedance_value;
						NR_busdata[NR_node_reference].full_Y_load[4] += working_impedance_value;
					}//End 12
				}
			}//End of for loop for triplex
		}
		else
		{
			for (loop_index_var=0; loop_index_var<3; loop_index_var++)
			{
				//Compute the difference
				working_impedance_value = shunt[loop_index_var] - shunt_change_check[loop_index_var];

				//Check it
				if (working_impedance_value.Mag() > 0.0)
				{
					NR_FPI_imp_load_change = true;

					//Update the matrix
					NR_busdata[NR_node_reference].full_Y_load[loop_index_var*3+loop_index_var] += working_impedance_value;
				}
			}//End of for loop for Wye

			//Do different parent routine
			if ((SubNode & SNT_DIFF_PARENT) == SNT_DIFF_PARENT)
			{
				//Reset flag
				local_shunt_update = false;

				//Loop through and check for "differences"
				for (loop_index_var=0; loop_index_var<3; loop_index_var++)
				{
					//Compute the difference
					intermed_impedance[loop_index_var] = Extra_Data[loop_index_var+3] - Extra_Data_Track_FPI[loop_index_var];

					//Check it
					if (intermed_impedance[loop_index_var].Mag() > 0.0)
					{
						NR_FPI_imp_load_change = true;
						local_shunt_update = true;

						//Update the tracker
						Extra_Data_Track_FPI[loop_index_var] = Extra_Data[loop_index_var+3];
					}
				}

				//Perform the update if we changed - if not, no reason to do so
				if (local_shunt_update)
				{
					//Update the matrix
					NR_busdata[NR_node_reference].full_Y_load[0] += intermed_impedance[0] + intermed_impedance[2];
					NR_busdata[NR_node_reference].full_Y_load[1] -= intermed_impedance[0];
					NR_busdata[NR_node_reference].full_Y_load[2] -= intermed_impedance[2];
					NR_busdata[NR_node_reference].full_Y_load[3] -= intermed_impedance[0];
					NR_busdata[NR_node_reference].full_Y_load[4] += intermed_impedance[1] + intermed_impedance[0];
					NR_busdata[NR_node_reference].full_Y_load[5] -= intermed_impedance[1];
					NR_busdata[NR_node_reference].full_Y_load[6] -= intermed_impedance[2];
					NR_busdata[NR_node_reference].full_Y_load[7] -= intermed_impedance[1];
					NR_busdata[NR_node_reference].full_Y_load[8] += intermed_impedance[2] + intermed_impedance[1];
				}
				//Default else - no update, so no need to recalc anything
			}//End DIFF_PARENT - Delta on a Wye
		}//End no D (Wye)

		//Update accumulator for FPI
		//Store current shunt values "of interest" to see if it changed for FPIM
		shunt_change_check[0] = shunt[0];
		shunt_change_check[1] = shunt[1];
		shunt_change_check[2] = shunt[2];

		//Capture the explicit Delta-Wye portion too
		shunt_change_check_dy[0] = shunt_dy[0];	//Delta
		shunt_change_check_dy[1] = shunt_dy[1];
		shunt_change_check_dy[2] = shunt_dy[2];
		shunt_change_check_dy[3] = shunt_dy[3];	//Wye
		shunt_change_check_dy[4] = shunt_dy[4];
		shunt_change_check_dy[5] = shunt_dy[5];
	}//End FPI
	//TCIM doesn't use this - shouldn't even be called

	return SUCCESS;	//Not sure how this would fail
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF OTHER EXPORT FUNCTIONS
//////////////////////////////////////////////////////////////////////////
EXPORT int isa_node(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,node)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT int notify_node(OBJECT *obj, int update_mode, PROPERTY *prop, char *value){
	node *n = OBJECTDATA(obj, node);
	int rv = 1;

	rv = n->notify(update_mode, prop, value);

	return rv;
}

//Exported function for attaching this to a VFD - basically sets a flag and maps a function
EXPORT STATUS attach_vfd_to_node(OBJECT *obj,OBJECT *calledVFD)
{
	node *nodeObj = OBJECTDATA(obj,node);

	//Call the function
	return nodeObj->link_VFD_functions(calledVFD);
}

//Deltamode export
EXPORT SIMULATIONMODE interupdate_node(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	node *my = OBJECTDATA(obj,node);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_node(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_node(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

//Function to set a node as a SWING inside NR - basically converts a SWING to a PQ, without the SWING_PQ requirement
EXPORT STATUS swap_node_swing_status(OBJECT *obj, bool desired_status)
{
	STATUS temp_status;

	//Map the node
	node *my = OBJECTDATA(obj,node);

	//Call the function, where we can see our internals
	temp_status = my->NR_swap_swing_status(desired_status);

	//Return what the sub function said we were
	return temp_status;
}

//Function to query a current node's status as a SWING node -- basically lets generators see how "swing_functions_enabled" is behaving
EXPORT STATUS node_swing_status(OBJECT *this_obj, bool *swing_status_check_value, bool *swing_pq_status_value)
{
	//Map ourselves
	node *my = OBJECTDATA(this_obj,node);

	//Run the query
	my->NR_swing_status_check(swing_status_check_value,swing_pq_status_value);

	//Return success, because reasons
	return SUCCESS;
}

//Exposed function to map a "current injection update" routine from another object
//Used primarily for deltamode and voltage-source inverters right now
EXPORT STATUS node_map_current_update_function(OBJECT *nodeObj, OBJECT *callObj)
{
	STATUS temp_status;

	//Map the node
	node *my = OBJECTDATA(nodeObj,node);

	//Call the mapping function
	temp_status = my->NR_map_current_update_function(callObj);

	//Return the value
	return temp_status;
}

//Exposed function to reset the "disabled island" state of this node
EXPORT STATUS node_reset_disabled_status(OBJECT *nodeObj)
{
	STATUS temp_status;

	//Map the node
	node *my = OBJECTDATA(nodeObj,node);

	//Call our local function
	temp_status = my->reset_node_island_condition();

	//Return
	return temp_status;
}

//Exposed function to do shunt update - primarily to get impedance update properly sequenced for FPI in deltamode
EXPORT STATUS node_update_shunt_values(OBJECT *obj)
{
	STATUS temp_status;

	//Map the node
	node *my = OBJECTDATA(obj,node);

	//Call the update
	temp_status = my->shunt_update_fxn();

	//Return
	return temp_status;
}
/**@}*/
