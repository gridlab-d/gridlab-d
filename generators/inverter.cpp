/** $Id: inverter.cpp,v 1.0 2008/07/15 
	Copyright (C) 2008 Battelle Memorial Institute
	@file inverter.cpp
	@defgroup inverter
	@ingroup generators

 @{
 **/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "inverter.h"

//CLASS *inverter::plcass = power_electronics;
CLASS *inverter::oclass = nullptr;
inverter *inverter::defaults = nullptr;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
inverter::inverter(MODULE *module)
{
	if (oclass==nullptr)
	{
		oclass = gl_register_class(module,"inverter",sizeof(inverter),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class inverter";
		else
			oclass->trl = TRL_PROOF;
		
		if (gl_publish_variable(oclass,

			PT_enumeration,"inverter_type",PADDR(inverter_type_v), PT_DESCRIPTION, "LEGACY MODEL: Sets efficiencies and other parameters; if using four_quadrant_control_mode, set this to FOUR_QUADRANT",
				PT_KEYWORD,"TWO_PULSE",(enumeration)TWO_PULSE,
				PT_KEYWORD,"SIX_PULSE",(enumeration)SIX_PULSE,
				PT_KEYWORD,"TWELVE_PULSE",(enumeration)TWELVE_PULSE,
				PT_KEYWORD,"PWM",(enumeration)PWM,
				PT_KEYWORD,"FOUR_QUADRANT",(enumeration)FOUR_QUADRANT,

				PT_enumeration,"four_quadrant_control_mode",PADDR(four_quadrant_control_mode), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Activates various control modes",
				PT_KEYWORD,"NONE",(enumeration)FQM_NONE,
				PT_KEYWORD,"CONSTANT_PQ",(enumeration)FQM_CONSTANT_PQ,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)FQM_CONSTANT_PF,
				//PT_KEYWORD,"CONSTANT_V",FQM_CONSTANT_V,	//Not implemented yet
				PT_KEYWORD,"VOLT_VAR",(enumeration)FQM_VOLT_VAR,
				PT_KEYWORD,"VOLT_WATT",(enumeration)FQM_VOLT_WATT,
				PT_KEYWORD,"VOLT_VAR_FREQ_PWR",FQM_VOLT_VAR_FREQ_PWR, //Ab add : mode
				PT_KEYWORD,"LOAD_FOLLOWING",(enumeration)FQM_LOAD_FOLLOWING,
				PT_KEYWORD,"GROUP_LOAD_FOLLOWING",(enumeration)FQM_GROUP_LF,
				PT_KEYWORD,"VOLTAGE_SOURCE",(enumeration)FQM_VSI,

			PT_enumeration,"pf_reg",PADDR(pf_reg), PT_DESCRIPTION, "Activate (or not) power factor regulation in four_quadrant_control_mode",
				PT_KEYWORD,"INCLUDED",(enumeration)INCLUDED,
				PT_KEYWORD,"INCLUDED_ALT",(enumeration)INCLUDED_ALT,
				PT_KEYWORD,"EXCLUDED",(enumeration)EXCLUDED,

			PT_enumeration,"generator_status",PADDR(gen_status_v), PT_DESCRIPTION, "describes whether the generator is online or offline",
				PT_KEYWORD,"OFFLINE",(enumeration)OFFLINE,
				PT_KEYWORD,"ONLINE",(enumeration)ONLINE,	

			PT_enumeration,"generator_mode",PADDR(gen_mode_v), PT_DESCRIPTION, "LEGACY MODEL: Selects generator control mode when using legacy model; in non-legacy models, this should be SUPPLY_DRIVEN.",
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"CONSTANT_V",(enumeration)CONSTANT_V,
				PT_KEYWORD,"CONSTANT_PQ",(enumeration)CONSTANT_PQ,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)CONSTANT_PF,
				PT_KEYWORD,"SUPPLY_DRIVEN",(enumeration)SUPPLY_DRIVEN,
			
			PT_double, "inverter_convergence_criterion",PADDR(inverter_convergence_criterion), PT_DESCRIPTION, "The maximum change in error threshold for exitting deltamode.",
			PT_double,"current_convergence[A]",PADDR(current_convergence_criterion),PT_DESCRIPTION,"Convergence criterion for current changes on first timestep - basically initialization of system",
			PT_double, "V_In[V]",PADDR(V_In), PT_DESCRIPTION, "DC voltage",
			PT_double, "I_In[A]",PADDR(I_In), PT_DESCRIPTION, "DC current",
			PT_double, "P_In[W]", PADDR(P_In), PT_DESCRIPTION, "DC power",
			PT_complex, "VA_Out[VA]", PADDR(VA_Out), PT_DESCRIPTION, "AC power",
			PT_double, "Vdc[V]", PADDR(Vdc), PT_DESCRIPTION, "LEGACY MODEL: DC voltage",
			PT_complex, "phaseA_V_Out[V]", PADDR(phaseA_V_Out), PT_DESCRIPTION, "AC voltage on A phase in three-phase system; 240-V connection on a triplex system",
			PT_complex, "phaseB_V_Out[V]", PADDR(phaseB_V_Out), PT_DESCRIPTION, "AC voltage on B phase in three-phase system",
			PT_complex, "phaseC_V_Out[V]", PADDR(phaseC_V_Out), PT_DESCRIPTION, "AC voltage on C phase in three-phase system",
			PT_complex, "phaseA_I_Out[V]", PADDR(phaseA_I_Out), PT_DESCRIPTION, "AC current on A phase in three-phase system; 240-V connection on a triplex system",
			PT_complex, "phaseB_I_Out[V]", PADDR(phaseB_I_Out), PT_DESCRIPTION, "AC current on B phase in three-phase system",
			PT_complex, "phaseC_I_Out[V]", PADDR(phaseC_I_Out), PT_DESCRIPTION, "AC current on C phase in three-phase system",
			PT_complex, "power_A[VA]", PADDR(power_val[0]), PT_DESCRIPTION, "AC power on A phase in three-phase system; 240-V connection on a triplex system",
			PT_complex, "power_B[VA]", PADDR(power_val[1]), PT_DESCRIPTION, "AC power on B phase in three-phase system",
			PT_complex, "power_C[VA]", PADDR(power_val[2]), PT_DESCRIPTION, "AC power on C phase in three-phase system",
			PT_complex, "curr_VA_out_A[VA]", PADDR(curr_VA_out[0]), PT_DESCRIPTION, "AC power on A phase in three-phase system; 240-V connection on a triplex system",
			PT_complex, "curr_VA_out_B[VA]", PADDR(curr_VA_out[1]), PT_DESCRIPTION, "AC power on B phase in three-phase system",
			PT_complex, "curr_VA_out_C[VA]", PADDR(curr_VA_out[2]), PT_DESCRIPTION, "AC power on C phase in three-phase system",
			PT_complex, "prev_VA_out_A[VA]", PADDR(prev_VA_out[0]), PT_DESCRIPTION, "AC power on A phase in three-phase system; 240-V connection on a triplex system",
			PT_complex, "prev_VA_out_B[VA]", PADDR(prev_VA_out[1]), PT_DESCRIPTION, "AC power on B phase in three-phase system",
			PT_complex, "prev_VA_out_C[VA]", PADDR(prev_VA_out[2]), PT_DESCRIPTION, "AC power on C phase in three-phase system",

			// Internal Voltage magnutue and angle of VSI_DROOP, e_source[i],
			PT_double, "e_source_A", PADDR(e_source[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of grid-forming source, phase A",
			PT_double, "e_source_B", PADDR(e_source[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of grid-forming source, phase B",
			PT_double, "e_source_C", PADDR(e_source[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of grid-forming source, phase C",
			PT_double, "V_angle_A", PADDR(V_angle[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal angle of grid-forming source, phase A",
			PT_double, "V_angle_B", PADDR(V_angle[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal angle of grid-forming source, phase B",
			PT_double, "V_angle_C", PADDR(V_angle[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal angle of grid-forming source, phase C",

			PT_complex, "phaseA_I_Forming[A]", PADDR(temp_current_val[0]), PT_DESCRIPTION, "AC current on A phase in three-phase system, only for grid_forming",
			PT_complex, "phaseB_I_Forming[A]", PADDR(temp_current_val[1]), PT_DESCRIPTION, "AC current on B phase in three-phase system, only for grid_forming",
			PT_complex, "phaseC_I_Forming[A]", PADDR(temp_current_val[2]), PT_DESCRIPTION, "AC current on C phase in three-phase system, only for grid_forming",

			PT_complex, "phaseA_I_Following[A]", PADDR(I_Out[0]), PT_DESCRIPTION, "AC current on A phase in three-phase system, only for grid_following",
			PT_complex, "phaseB_I_Following[A]", PADDR(I_Out[1]), PT_DESCRIPTION, "AC current on B phase in three-phase system, only for grid_following",
			PT_complex, "phaseC_I_Following[A]", PADDR(I_Out[2]), PT_DESCRIPTION, "AC current on C phase in three-phase system, only for grid_following",

			// 3 phase average value of terminal voltage
			PT_double, "pCircuit_V_Avg", PADDR(pCircuit_V_Avg), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: three-phase average value of terminal voltage",

			//Input
			PT_double, "P_Out[VA]", PADDR(P_Out), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Scheduled real power out in CONSTANT_PQ control mode",
			PT_double, "Q_Out[VAr]", PADDR(Q_Out), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Schedule reactive power out in CONSTANT_PQ control mode",
			PT_double, "power_in[W]", PADDR(p_in),  PT_DESCRIPTION, "LEGACY MODEL: No longer used",
			PT_double, "rated_power[VA]", PADDR(p_rated), PT_DESCRIPTION, "FOUR QUADRANT MODEL: The rated power of the inverter",
			PT_double, "rated_battery_power[W]", PADDR(bp_rated), PT_DESCRIPTION, "FOUR QUADRANT MODEL: The rated power of battery when battery is attached",
			PT_double, "inverter_efficiency", PADDR(inv_eta), PT_DESCRIPTION, "FOUR QUADRANT MODEL: The efficiency of the inverter",
			PT_double, "battery_soc[pu]", PADDR(b_soc), PT_DESCRIPTION, "FOUR QUADRANT MODEL: The state of charge of an attached battery",
			PT_double, "soc_reserve[pu]", PADDR(soc_reserve), PT_DESCRIPTION, "FOUR QUADRANT MODEL: The reserve state of charge of an attached battery for islanding cases",
			PT_double, "power_factor[unit]", PADDR(power_factor),  PT_DESCRIPTION, "FOUR QUADRANT MODEL: The power factor used for CONSTANT_PF control mode",
			PT_bool,"islanded_state", PADDR(islanded),  PT_DESCRIPTION, "FOUR QUADRANT MODEL: Boolean used to let control modes to act under island conditions",
			PT_double, "nominal_frequency[Hz]", PADDR(f_nominal),

			// Dynamic Current Control Parameters - PID mode
			PT_double, "Pref", PADDR(Pref), PT_DESCRIPTION, "DELTAMODE: The real power reference.",
			PT_double, "Qref", PADDR(Qref), PT_DESCRIPTION, "DELTAMODE: The reactive power reference.",
			PT_double, "kpd", PADDR(kpd), PT_DESCRIPTION, "DELTAMODE: The d-axis integration gain for the current modulation PI controller.",
			PT_double, "kpq", PADDR(kpq), PT_DESCRIPTION, "DELTAMODE: The q-axis integration gain for the current modulation PI controller.",
			PT_double, "kid", PADDR(kid), PT_DESCRIPTION, "DELTAMODE: The d-axis proportional gain for the current modulation PI controller.",
			PT_double, "kiq", PADDR(kiq), PT_DESCRIPTION, "DELTAMODE: The q-axis proportional gain for the current modulation PI controller.",
			PT_double, "kdd", PADDR(kdd), PT_DESCRIPTION, "DELTAMODE: The d-axis differentiator gain for the current modulation PID controller",
			PT_double, "kdq", PADDR(kdq), PT_DESCRIPTION, "DELTAMODE: The q-axis differentiator gain for the current modulation PID controller",
			
			//Additional control parameters -- PI mode
			PT_double, "epA", PADDR(curr_state.ed[0]), PT_DESCRIPTION, "DELTAMODE: The real current error for phase A or triplex phase.",
			PT_double, "epB", PADDR(curr_state.ed[1]), PT_DESCRIPTION, "DELTAMODE: The real current error for phase B.",
			PT_double, "epC", PADDR(curr_state.ed[2]), PT_DESCRIPTION, "DELTAMODE: The real current error for phase C.",
			PT_double, "eqA", PADDR(curr_state.eq[0]), PT_DESCRIPTION, "DELTAMODE: The reactive current error for phase A or triplex phase.",
			PT_double, "eqB", PADDR(curr_state.eq[1]), PT_DESCRIPTION, "DELTAMODE: The reactive current error for phase B.",
			PT_double, "eqC", PADDR(curr_state.eq[2]), PT_DESCRIPTION, "DELTAMODE: The reactive current error for phase C.",
			PT_double, "delta_epA", PADDR(curr_state.ded[0]), PT_DESCRIPTION, "DELTAMODE: The change in real current error for phase A or triplex phase.",
			PT_double, "delta_epB", PADDR(curr_state.ded[1]), PT_DESCRIPTION, "DELTAMODE: The change in real current error for phase B.",
			PT_double, "delta_epC", PADDR(curr_state.ded[2]), PT_DESCRIPTION, "DELTAMODE: The change in real current error for phase C.",
			PT_double, "delta_eqA", PADDR(curr_state.deq[0]), PT_DESCRIPTION, "DELTAMODE: The change in reactive current error for phase A or triplex phase.",
			PT_double, "delta_eqB", PADDR(curr_state.deq[1]), PT_DESCRIPTION, "DELTAMODE: The change in reactive current error for phase B.",
			PT_double, "delta_eqC", PADDR(curr_state.deq[2]), PT_DESCRIPTION, "DELTAMODE: The change in reactive current error for phase C.",
			PT_double, "mdA", PADDR(curr_state.md[0]), PT_DESCRIPTION, "DELTAMODE: The d-axis current modulation for phase A or triplex phase.",
			PT_double, "mdB", PADDR(curr_state.md[1]), PT_DESCRIPTION, "DELTAMODE: The d-axis current modulation for phase B.",
			PT_double, "mdC", PADDR(curr_state.md[2]), PT_DESCRIPTION, "DELTAMODE: The d-axis current modulation for phase C.",
			PT_double, "mqA", PADDR(curr_state.mq[0]), PT_DESCRIPTION, "DELTAMODE: The q-axis current modulation for phase A or triplex phase.",
			PT_double, "mqB", PADDR(curr_state.mq[1]), PT_DESCRIPTION, "DELTAMODE: The q-axis current modulation for phase B.",
			PT_double, "mqC", PADDR(curr_state.mq[2]), PT_DESCRIPTION, "DELTAMODE: The q-axis current modulation for phase C.",
			PT_double, "delta_mdA", PADDR(curr_state.dmd[0]), PT_DESCRIPTION, "DELTAMODE: The change in d-axis current modulation for phase A or triplex phase.",
			PT_double, "delta_mdB", PADDR(curr_state.dmd[1]), PT_DESCRIPTION, "DELTAMODE: The change in d-axis current modulation for phase B.",
			PT_double, "delta_mdC", PADDR(curr_state.dmd[2]), PT_DESCRIPTION, "DELTAMODE: The change in d-axis current modulation for phase C.",
			PT_double, "delta_mqA", PADDR(curr_state.dmq[0]), PT_DESCRIPTION, "DELTAMODE: The change in q-axis current modulation for phase A or triplex phase.",
			PT_double, "delta_mqB", PADDR(curr_state.dmq[1]), PT_DESCRIPTION, "DELTAMODE: The change in q-axis current modulation for phase B.",
			PT_double, "delta_mqC", PADDR(curr_state.dmq[2]), PT_DESCRIPTION, "DELTAMODE: The change in q-axis current modulation for phase C.",
			PT_complex, "IdqA", PADDR(curr_state.Idq[0]), PT_DESCRIPTION, "DELTAMODE: The dq-axis current for phase A or triplex phase.",
			PT_complex, "IdqB", PADDR(curr_state.Idq[1]), PT_DESCRIPTION, "DELTAMODE: The dq-axis current for phase B.",
			PT_complex, "IdqC", PADDR(curr_state.Idq[2]), PT_DESCRIPTION, "DELTAMODE: The dq-axis current for phase C.",			
			
			PT_double, "Tfreq_delay",PADDR(Tfreq_delay), PT_DESCRIPTION, "DELTAMODE: The time constant for delayed frequency seen by the inverter",
			PT_bool, "inverter_droop_fp", PADDR(inverter_droop_fp),  PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether inverter f/p droop is included or not",
			PT_double, "R_fp",PADDR(R_fp), PT_DESCRIPTION, "DELTAMODE: The droop parameter of the f/p droop",
			PT_double, "kppmax",PADDR(kppmax), PT_DESCRIPTION, "DELTAMODE: The proportional gain of Pmax controller",
			PT_double, "kipmax",PADDR(kipmax), PT_DESCRIPTION, "DELTAMODE: The integral gain of Pmax controller",
			PT_double, "Pmax",PADDR(Pmax), PT_DESCRIPTION, "DELTAMODE: power limit of grid forming inverter",
			PT_double, "Pmin",PADDR(Pmin), PT_DESCRIPTION, "DELTAMODE: power limit of grid forming inverter",
			PT_double, "Pmax_Low_Limit",PADDR(Pmax_Low_Limit), PT_DESCRIPTION, "DELTAMODE: output limit of Pmax controller",


			PT_double, "Tvol_delay",PADDR(Tvol_delay), PT_DESCRIPTION, "DELTAMODE: The time constant for delayed voltage seen by the inverter",
			PT_bool, "inverter_droop_vq", PADDR(inverter_droop_vq),  PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether inverter q/v droop is included or not",
			PT_double, "R_vq",PADDR(R_vq), PT_DESCRIPTION, "DELTAMODE: The droop parameter of the v/q droop",

			PT_double, "Tp_delay",PADDR(Tp_delay), PT_DESCRIPTION, "DELTAMODE: The time constant for delayed real power seen by the VSI droop controller",
			PT_double, "Tq_delay",PADDR(Tq_delay), PT_DESCRIPTION, "DELTAMODE: The time constant for delayed reactive power seen by the VSI droop controller",

			// Parameters for VSI mode
			PT_complex,"VSI_Rfilter[pu]",PADDR(Rfilter),PT_DESCRIPTION,"VSI filter resistance (p.u.)",
			PT_complex,"VSI_Xfilter[pu]",PADDR(Xfilter),PT_DESCRIPTION,"VSI filter inductance (p.u.)",
			PT_enumeration,"VSI_mode",PADDR(VSI_mode), PT_DESCRIPTION, "VSI MODEL: Selects VSI mode for either isochronous or droop one",
				PT_KEYWORD,"VSI_ISOCHRONOUS",(enumeration)VSI_ISOCHRONOUS,
				PT_KEYWORD,"VSI_DROOP",(enumeration)VSI_DROOP,
			PT_double, "VSI_freq",PADDR(VSI_freq), PT_DESCRIPTION, "VSI frequency",
			PT_double, "ki_Vterminal", PADDR(ki_Vterminal), PT_DESCRIPTION, "DELTAMODE: The integrator gain for the VSI terminal voltage modulation",
			PT_double, "kp_Vterminal", PADDR(kp_Vterminal), PT_DESCRIPTION, "DELTAMODE: The proportional gain for the VSI terminal voltage modulation",
			PT_double, "V_set_droop", PADDR(V_mag_ref[0]), PT_DESCRIPTION, "DELTAMODE: The voltage setpoint of droop control",
			PT_double, "V_set0", PADDR(V_ref[0]), PT_DESCRIPTION, "DELTAMODE: The voltage setpoint of grid-following Q-V droop control",
			PT_double, "V_set1", PADDR(V_ref[1]), PT_DESCRIPTION, "DELTAMODE: The voltage setpoint of grid-following Q-V droop control",
			PT_double, "V_set2", PADDR(V_ref[2]), PT_DESCRIPTION, "DELTAMODE: The voltage setpoint of grid-following Q-V droop control",

			// Parameter for checking slew rate of inverters
			PT_bool, "enable_ramp_rates_real", PADDR(checkRampRate_real),  PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether inverter ramp rate is enforced or not",
			PT_double, "max_ramp_up_real[W/s]", PADDR(rampUpRate_real), PT_DESCRIPTION, "DELTAMODE: The real power ramp up rate limit",
			PT_double, "max_ramp_down_real[W/s]", PADDR(rampDownRate_real), PT_DESCRIPTION, "DELTAMODE: The real power ramp down rate limit",
			PT_bool, "enable_ramp_rates_reactive", PADDR(checkRampRate_reactive),  PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether inverter ramp rate is enforced or not",
			PT_double, "max_ramp_up_reactive[VAr/s]", PADDR(rampUpRate_reactive), PT_DESCRIPTION, "DELTAMODE: The reactive power ramp up rate limit",
			PT_double, "max_ramp_down_reactive[VAr/s]", PADDR(rampDownRate_reactive), PT_DESCRIPTION, "DELTAMODE: The reactive power ramp down rate limit",

			//Selection method
			PT_enumeration, "dynamic_model_mode", PADDR(inverter_dyn_mode), PT_DESCRIPTION, "DELTAMODE: Underlying model to use for deltamode control",
				PT_KEYWORD, "PID", (enumeration)PID_CONTROLLER,
				PT_KEYWORD, "PI", (enumeration)PI_CONTROLLER,

			//1547 stuff
			PT_bool, "enable_1547_checks", PADDR(enable_1547_compliance), PT_DESCRIPTION, "DELTAMODE: Enable IEEE 1547-2003 disconnect checking",
			PT_double, "reconnect_time[s]", PADDR(reconnect_time), PT_DESCRIPTION, "DELTAMODE: Time delay after IEEE 1547-2003 violation clears before resuming generation",
			PT_bool, "inverter_1547_status", PADDR(inverter_1547_status), PT_DESCRIPTION, "DELTAMODE: Indicator if the inverter is curtailed due to a 1547 violation or not",

			//Select 1547 type to auto-populate
			PT_enumeration, "IEEE_1547_version", PADDR(ieee_1547_version), PT_DESCRIPTION, "DELTAMODE: Version of IEEE 1547 to use to populate defaults",
				PT_KEYWORD, "NONE", (enumeration)IEEE_NONE,
				PT_KEYWORD, "IEEE1547", (enumeration)IEEE1547,
				PT_KEYWORD, "IEEE1547A", (enumeration)IEEE1547A,
				PT_KEYWORD, "IEEE1547_2003", (enumeration)IEEE1547,	//Duplicated to reflect inverter_dyn changes and make the same
				PT_KEYWORD, "IEEE1547A_2014", (enumeration)IEEE1547A,

			//Frequency bands of 1547a checks
			PT_double, "over_freq_high_cutout[Hz]", PADDR(over_freq_high_band_setpoint),PT_DESCRIPTION,"DELTAMODE: OF2 set point for IEEE 1547a",
			PT_double, "over_freq_high_disconnect_time[s]", PADDR(over_freq_high_band_delay),PT_DESCRIPTION,"DELTAMODE: OF2 clearing time for IEEE1547a",
			PT_double, "over_freq_low_cutout[Hz]", PADDR(over_freq_low_band_setpoint),PT_DESCRIPTION,"DELTAMODE: OF1 set point for IEEE 1547a",
			PT_double, "over_freq_low_disconnect_time[s]", PADDR(over_freq_low_band_delay),PT_DESCRIPTION,"DELTAMODE: OF1 clearing time for IEEE 1547a",
			PT_double, "under_freq_high_cutout[Hz]", PADDR(under_freq_high_band_setpoint),PT_DESCRIPTION,"DELTAMODE: UF2 set point for IEEE 1547a",
			PT_double, "under_freq_high_disconnect_time[s]", PADDR(under_freq_high_band_delay),PT_DESCRIPTION,"DELTAMODE: UF2 clearing time for IEEE1547a",
			PT_double, "under_freq_low_cutout[Hz]", PADDR(under_freq_low_band_setpoint),PT_DESCRIPTION,"DELTAMODE: UF1 set point for IEEE 1547a",
			PT_double, "under_freq_low_disconnect_time[s]", PADDR(under_freq_low_band_delay),PT_DESCRIPTION,"DELTAMODE: UF1 clearing time for IEEE 1547a",

			//Voltage bands of 1547 checks
			PT_double,"under_voltage_low_cutout[pu]",PADDR(under_voltage_lowest_voltage_setpoint),PT_DESCRIPTION,"Lowest voltage threshold for undervoltage",
			PT_double,"under_voltage_middle_cutout[pu]",PADDR(under_voltage_middle_voltage_setpoint),PT_DESCRIPTION,"Middle-lowest voltage threshold for undervoltage",
			PT_double,"under_voltage_high_cutout[pu]",PADDR(under_voltage_high_voltage_setpoint),PT_DESCRIPTION,"High value of low voltage threshold for undervoltage",
			PT_double,"over_voltage_low_cutout[pu]",PADDR(over_voltage_low_setpoint),PT_DESCRIPTION,"Lowest voltage value for overvoltage",
			PT_double,"over_voltage_high_cutout[pu]",PADDR(over_voltage_high_setpoint),PT_DESCRIPTION,"High voltage value for overvoltage",
			PT_double,"under_voltage_low_disconnect_time[s]",PADDR(under_voltage_lowest_delay),PT_DESCRIPTION,"Lowest voltage clearing time for undervoltage",
			PT_double,"under_voltage_middle_disconnect_time[s]",PADDR(under_voltage_middle_delay),PT_DESCRIPTION,"Middle-lowest voltage clearing time for undervoltage",
			PT_double,"under_voltage_high_disconnect_time[s]",PADDR(under_voltage_high_delay),PT_DESCRIPTION,"Highest voltage clearing time for undervoltage",
			PT_double,"over_voltage_low_disconnect_time[s]",PADDR(over_voltage_low_delay),PT_DESCRIPTION,"Lowest voltage clearing time for overvoltage",
			PT_double,"over_voltage_high_disconnect_time[s]",PADDR(over_voltage_high_delay),PT_DESCRIPTION,"Highest voltage clearing time for overvoltage",

			//1547 trip reason
			PT_enumeration, "IEEE_1547_trip_method", PADDR(ieee_1547_trip_method), PT_DESCRIPTION, "DELTAMODE: Reason for IEEE 1547 disconnect - which threshold was hit",
				PT_KEYWORD, "NONE",(enumeration)IEEE_1547_NONE, PT_DESCRIPTION, "No trip reason",
				PT_KEYWORD, "OVER_FREQUENCY_HIGH",(enumeration)IEEE_1547_HIGH_OF, PT_DESCRIPTION, "High over-frequency level trip - OF2",
				PT_KEYWORD, "OVER_FREQUENCY_LOW",(enumeration)IEEE_1547_LOW_OF, PT_DESCRIPTION, "Low over-frequency level trip - OF1",
				PT_KEYWORD, "UNDER_FREQUENCY_HIGH",(enumeration)IEEE_1547_HIGH_UF, PT_DESCRIPTION, "High under-frequency level trip - UF2",
				PT_KEYWORD, "UNDER_FREQUENCY_LOW",(enumeration)IEEE_1547_LOW_UF, PT_DESCRIPTION, "Low under-frequency level trip - UF1",
				PT_KEYWORD, "UNDER_VOLTAGE_LOW",(enumeration)IEEE_1547_LOWEST_UV, PT_DESCRIPTION, "Lowest under-voltage level trip",
				PT_KEYWORD, "UNDER_VOLTAGE_MID",(enumeration)IEEE_1547_MIDDLE_UV, PT_DESCRIPTION, "Middle under-voltage level trip",
				PT_KEYWORD, "UNDER_VOLTAGE_HIGH",(enumeration)IEEE_1547_HIGH_UV, PT_DESCRIPTION, "High under-voltage level trip",
				PT_KEYWORD, "OVER_VOLTAGE_LOW",(enumeration)IEEE_1547_LOW_OV, PT_DESCRIPTION, "Low over-voltage level trip",
				PT_KEYWORD, "OVER_VOLTAGE_HIGH",(enumeration)IEEE_1547_HIGH_OV, PT_DESCRIPTION, "High over-voltage level trip",

			PT_set, "phases", PADDR(phases),  PT_DESCRIPTION, "The phases the inverter is attached to",
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			//multipoint efficiency model parameters'
			PT_bool, "use_multipoint_efficiency", PADDR(use_multipoint_efficiency), PT_DESCRIPTION, "FOUR QUADRANT MODEL: boolean to used the multipoint efficiency curve for the inverter when solar is attached",
			PT_enumeration, "inverter_manufacturer", PADDR(inverter_manufacturer), PT_DESCRIPTION, "MULTIPOINT EFFICIENCY MODEL: the manufacturer of the inverter to setup up pre-existing efficiency curves",
				PT_KEYWORD, "NONE", (enumeration)NONE,
				PT_KEYWORD, "FRONIUS", (enumeration)FRONIUS,
				PT_KEYWORD, "SMA", (enumeration)SMA,
				PT_KEYWORD, "XANTREX", (enumeration)XANTREX,
			PT_double, "maximum_dc_power", PADDR(p_dco), PT_DESCRIPTION, "MULTIPOINT EFFICIENCY MODEL: the maximum dc power point for the efficiency curve",
			PT_double, "maximum_dc_voltage", PADDR(v_dco), PT_DESCRIPTION, "MULTIPOINT EFFICIENCY MODEL: the maximum dc voltage point for the efficiency curve",
			PT_double, "minimum_dc_power", PADDR(p_so), PT_DESCRIPTION, "MULTIPOINT EFFICIENCY MODEL: the minimum dc power point for the efficiency curve",
			PT_double, "c_0", PADDR(c_o), PT_DESCRIPTION, "MULTIPOINT EFFICIENCY MODEL: the first coefficient in the efficiency curve",
			PT_double, "c_1", PADDR(c_1), PT_DESCRIPTION, "MULTIPOINT EFFICIENCY MODEL: the second coefficient in the efficiency curve",
			PT_double, "c_2", PADDR(c_2), PT_DESCRIPTION, "MULTIPOINT EFFICIENCY MODEL: the third coefficient in the efficiency curve",
			PT_double, "c_3", PADDR(c_3), PT_DESCRIPTION, "MULTIPOINT EFFICIENCY MODEL: the fourth coefficient in the efficiency curve",
			//load following parameters
			PT_object,"sense_object", PADDR(sense_object), PT_DESCRIPTION, "FOUR QUADRANT MODEL: name of the object the inverter is trying to mitigate the load on (node/link) in LOAD_FOLLOWING",
			PT_double,"max_charge_rate[W]", PADDR(max_charge_rate), PT_DESCRIPTION, "FOUR QUADRANT MODEL: maximum rate the battery can be charged in LOAD_FOLLOWING",
			PT_double,"max_discharge_rate[W]", PADDR(max_discharge_rate), PT_DESCRIPTION, "FOUR QUADRANT MODEL: maximum rate the battery can be discharged in LOAD_FOLLOWING",
			PT_double,"charge_on_threshold[W]", PADDR(charge_on_threshold), PT_DESCRIPTION, "FOUR QUADRANT MODEL: power level at which the inverter should try charging the battery in LOAD_FOLLOWING",
			PT_double,"charge_off_threshold[W]", PADDR(charge_off_threshold), PT_DESCRIPTION, "FOUR QUADRANT MODEL: power level at which the inverter should cease charging the battery in LOAD_FOLLOWING",
			PT_double,"discharge_on_threshold[W]", PADDR(discharge_on_threshold), PT_DESCRIPTION, "FOUR QUADRANT MODEL: power level at which the inverter should try discharging the battery in LOAD_FOLLOWING",
			PT_double,"discharge_off_threshold[W]", PADDR(discharge_off_threshold), PT_DESCRIPTION, "FOUR QUADRANT MODEL: power level at which the inverter should cease discharging the battery in LOAD_FOLLOWING",
			PT_double,"excess_input_power[W]", PADDR(excess_input_power), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Excess power at the input of the inverter that is otherwise just lost, or could be shunted to a battery",
			PT_double,"charge_lockout_time[s]",PADDR(charge_lockout_time), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Lockout time when a charging operation occurs before another LOAD_FOLLOWING dispatch operation can occur",
			PT_double,"discharge_lockout_time[s]",PADDR(discharge_lockout_time), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Lockout time when a discharging operation occurs before another LOAD_FOLLOWING dispatch operation can occur",

			//Power-factor regulation  parameters
			//PT_object,"sense_object", PADDR(sense_object), PF regulation uses the same sense object as load-following
			PT_double,"pf_reg_activate", PADDR(pf_reg_activate), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Lowest acceptable power-factor level below which power-factor regulation will activate.",
			PT_double,"pf_reg_deactivate", PADDR(pf_reg_deactivate), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Lowest acceptable power-factor above which no power-factor regulation is needed.",

			//Second power-factor regulation control parameters
			PT_double,"pf_target", PADDR(pf_target_var), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Desired power-factor to maintain (signed) positive is inductive",
			PT_double,"pf_reg_high", PADDR(pf_reg_high), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Upper limit for power-factor - if exceeds, go full reverse reactive",
			PT_double,"pf_reg_low", PADDR(pf_reg_low), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Lower limit for power-factor - if exceeds, stop regulating - pf_target_var is below this",
			
			//Common parameters for power-factor regulation approaches
			PT_double,"pf_reg_activate_lockout_time[s]", PADDR(pf_reg_activate_lockout_time), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Mandatory pause between the deactivation of power-factor regulation and it reactivation",
			//Ab add : VOLT_VAR_PWR_FREQ parameters
			PT_bool, "disable_volt_var_if_no_input_power", PADDR(disable_volt_var_if_no_input_power),
			PT_double, "delay_time[s]", PADDR(delay_time),
			PT_double, "max_var_slew_rate[VAr/s]", PADDR(max_var_slew_rate),
			PT_double, "max_pwr_slew_rate[W/s]", PADDR(max_pwr_slew_rate),
			PT_char1024, "volt_var_sched", PADDR(volt_var_sched),
			PT_char1024, "freq_pwr_sched", PADDR(freq_pwr_sched),
			//Group load-following (and power factor regulation) parameters
			PT_double,"charge_threshold[W]", PADDR(charge_threshold), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Level at which all inverters in the group will begin charging attached batteries. Regulated minimum load level.",
			PT_double,"discharge_threshold[W]", PADDR(discharge_threshold), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Level at which all inverters in the group will begin discharging attached batteries. Regulated maximum load level.",
			PT_double,"group_max_charge_rate[W]", PADDR(group_max_charge_rate), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Sum of the charge rates of the batteries involved in the group load-following.",
			PT_double,"group_max_discharge_rate[W]", PADDR(group_max_discharge_rate), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Sum of the discharge rates of the batteries involved in the group load-following.",
			PT_double,"group_rated_power[W]", PADDR(group_rated_power), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Sum of the inverter power ratings of the inverters involved in the group power-factor regulation.",
			//Volt Var Parameters
			PT_double,"V_base[V]", PADDR(V_base), PT_DESCRIPTION, "FOUR QUADRANT MODEL: The base voltage on the grid side of the inverter. Used in VOLT_VAR control mode.",
			PT_double,"V1[pu]", PADDR(V1), PT_DESCRIPTION, "FOUR QUADRANT MODEL: voltage point 1 in volt/var curve. Used in VOLT_VAR control mode.",
			PT_double,"Q1[pu]", PADDR(Q1), PT_DESCRIPTION, "FOUR QUADRANT MODEL: VAR point 1 in volt/var curve. Used in VOLT_VAR control mode.",
			PT_double,"V2[pu]", PADDR(V2), PT_DESCRIPTION, "FOUR QUADRANT MODEL: voltage point 2 in volt/var curve. Used in VOLT_VAR control mode.",
			PT_double,"Q2[pu]", PADDR(Q2), PT_DESCRIPTION, "FOUR QUADRANT MODEL: VAR point 2 in volt/var curve. Used in VOLT_VAR control mode.",
			PT_double,"V3[pu]", PADDR(V3), PT_DESCRIPTION, "FOUR QUADRANT MODEL: voltage point 3 in volt/var curve. Used in VOLT_VAR control mode.",
			PT_double,"Q3[pu]", PADDR(Q3), PT_DESCRIPTION, "FOUR QUADRANT MODEL: VAR point 3 in volt/var curve. Used in VOLT_VAR control mode.",
			PT_double,"V4[pu]", PADDR(V4), PT_DESCRIPTION, "FOUR QUADRANT MODEL: voltage point 4 in volt/var curve. Used in VOLT_VAR control mode.",
			PT_double,"Q4[pu]", PADDR(Q4), PT_DESCRIPTION, "FOUR QUADRANT MODEL: VAR point 4 in volt/var curve. Used in VOLT_VAR control mode.",
			PT_double,"volt_var_control_lockout[s]", PADDR(vv_lockout), PT_DESCRIPTION, "FOUR QUADRANT QUADRANT MODEL: the lockout time between volt/var actions.",

			//Hidden variables for solar checks
			PT_int32,"number_of_phases_out",PADDR(number_of_phases_out),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Exposed variable - hidden for solar",
			PT_double,"efficiency_value",PADDR(efficiency),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Exposed variable - hidden for solar",
			
			//Volt Var Parameters
			PT_double,"VW_V1[pu]", PADDR(VW_V1), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Voltage at which power limiting begins (e.g. 1.0583). Used in VOLT_WATT control mode.",
			PT_double,"VW_V2[pu]", PADDR(VW_V2), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Voltage at which power limiting ends. (e.g. 1.1000). Used in VOLT_WATT control mode.",
			PT_double,"VW_P1[pu]", PADDR(VW_P1), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Power limit at VW_P1 (e.g. 1). Used in VOLT_WATT control mode.",
			PT_double,"VW_P2[pu]", PADDR(VW_P2), PT_DESCRIPTION, "FOUR QUADRANT MODEL: Power limit at VW_P2 (e.g. 0). Used in VOLT_WATT control mode.",

			//Hidden variables for wind turbine checks
			PT_bool, "WT_is_connected", PADDR(WT_is_connected), PT_DESCRIPTION, "Internal flag that indicates a wind turbine child is connected",

			nullptr)<1) GL_THROW("unable to publish properties in %s",__FILE__);

			defaults = this;

			memset(this,0,sizeof(inverter));

			if (gl_publish_function(oclass,	"preupdate_gen_object", (FUNCTIONADDR)preupdate_inverter)==nullptr)
				GL_THROW("Unable to publish inverter deltamode function");
			if (gl_publish_function(oclass,	"interupdate_gen_object", (FUNCTIONADDR)interupdate_inverter)==nullptr)
				GL_THROW("Unable to publish inverter deltamode function");
			if (gl_publish_function(oclass,	"postupdate_gen_object", (FUNCTIONADDR)postupdate_inverter)==nullptr)
				GL_THROW("Unable to publish inverter deltamode function");
			if (gl_publish_function(oclass, "current_injection_update", (FUNCTIONADDR)inverter_NR_current_injection_update)==nullptr)
				GL_THROW("Unable to publish inverter current injection update function");
	}
}

//Isa function for identification
int inverter::isa(char *classname)
{
	return strcmp(classname,"inverter")==0;
}

/* Object creation is called once for each object that is created by the core */
int inverter::create(void) 
{
	// Default values for Inverter object.
	P_Out = 0;  // P_Out and Q_Out are set by the user as set values to output in CONSTANT_PQ mode
	Q_Out = 0;
	V_In_Set_A = gld::complex(480,0);
	V_In_Set_B = gld::complex(-240, 415.69);
	V_In_Set_C = gld::complex(-240,-415.69);
	V_Set_A = 240;
	V_Set_B = 240;
	V_Set_C = 240;
	margin = 10;
	I_out_prev = 0;
	I_step_max = 100;
	C_Storage_In = 0;
	power_factor = 1;
	
	// Parameters needed for droop curve
	freq_ref = 60;     // Set the frequency reference value as 60 HZ
	Tfreq_delay = 0.0; // Arbitrary value defined for delay of frequency seen by the inverter
	inverter_droop_fp = false; // By default there is no droop included for the inverter
	R_fp = 3.77; 		// Droop curve variable
	R_vq = 0.05;

	//Ab add : default values for four quadrant Volt-VAr mode
	disable_volt_var_if_no_input_power = false;	//Volt-VAr mode always on by default
	delay_time = -1;					        //delay time between seeing a voltage value and responding with appropiate VAr setting, set -1 to flag as unspecified, will reset to 0 by default
	max_var_slew_rate = 0;				        //maximum rate at which inverter can change its VAr output (VArs/second) set 0 to flag unspecified, will reset to huge value by default
	volt_var_sched[0] = '\0';			        //initialize Volt-VAr input as zero length string
	freq_pwr_sched[0] = '\0';                   //initialize freq-PdeliveredLimit input as zero length string
	//end Ab add
	
	// Default values for power electronics settings
	Max_P = 10;			//< maximum real power capacity in kW
	Max_Q = 10;			//< maximum reactive power capacity in kVar
	Rated_kVA = 15;		//< nominal capacity in kVA
	Rated_kV = 10;		//< nominal line-line voltage in kV
	//Rinternal = 0.035;
	Vdc = 480;

	last_current[0] = last_current[1] = last_current[2] = last_current[3] = 0.0;
	last_power[0] = last_power[1] = last_power[2] = last_power[3] = 0.0;

	islanded = false;
	use_multipoint_efficiency = false;
	p_dco = -1;
	p_so = -1;
	v_dco = -1;
	c_o = -1;
	c_1 = -1;
	c_2 = -1;
	c_3 = -1;
	p_max = -1;
	pMeterStatus = nullptr;
	efficiency = 0;
	inv_eta = 0.0;	//Not sure why there's two of these...

	sense_object = nullptr;
	max_charge_rate = 0;
	max_discharge_rate = 0;
	charge_on_threshold = 0;
	charge_off_threshold = 0;
	discharge_on_threshold = 0;
	discharge_off_threshold = 0;
	powerCalc = nullptr;
	sense_is_link = false;
	sense_power = nullptr;

	pf_reg = EXCLUDED;
	pf_reg_activate = -2;
	pf_reg_deactivate = -1;
	pf_reg_dispatch_change_allowed = true;
	pf_reg_dispatch_VAR = 0.0;
	pf_reg_status = IDLING;
	pf_reg_next_update_time = 0;
	pf_reg_activate_lockout_time = -1;

	pf_target_var = -2;
	pf_reg_high = -2;
	pf_reg_low = -2;	
	
	charge_threshold = -1;
	discharge_threshold = -1;
	group_max_charge_rate = -1;
	group_max_discharge_rate = -1;
	group_rated_power = -1;

	excess_input_power = 0.0;
	lf_dispatch_power = 0.0;
	load_follow_status = IDLE;	//LOAD_FOLLOWING starts out doing nothing
	four_quadrant_control_mode = FQM_CONSTANT_PF;	//Four quadrant defaults to constant PF mode

	next_update_time = 0;
	lf_dispatch_change_allowed = true;	//Begins with change allowed
	charge_lockout_time = 0.0;	//Charge and discharge default to no delay
	discharge_lockout_time = 0.0;
	b_soc = -1;
	f_nominal = 60;

	////////////////////////////////////////////////////////
	// DELTA MODE 
	////////////////////////////////////////////////////////
	deltamode_inclusive = false;	//By default, don't be included in deltamode simulations
	first_run = true;
	inverter_convergence_criterion = 1e-3;
	current_convergence_criterion = 1e-3;

	//Default PID controller variables -- no output by default
	kpd = 0.0;
	kpq = 0.0;
	kid = 0.0;
	kiq = 0.0;
	kdd = 0.0;
	kdq = 0.0;
	first_sync_delta_enabled = false;
	first_iter_counter = 0;

	inverter_start_time = TS_INVALID;
	inverter_first_step = true;
	first_iteration_current_injection = -1;	//Initialize - mainly for tracking SWING_PQ status

	//By default, assume we want the PI-based controller
	inverter_dyn_mode = PI_CONTROLLER;

	//1547 parameters
	enable_1547_compliance = false;		//1547 turned off, but default
	reconnect_time = 300.0;				//5 minute default, as suggested by 1547-2003
	inverter_1547_status = true;		//Not in a violation, by default
	out_of_violation_time_total = 0.0;	//Not in a violation, so not tracking 'recovery'
	ieee_1547_double = -1.0;			//Flag as not an issue
	pFrequency = nullptr;				//No mapping, by default
	value_Frequency = 60.0;			//Default value
	prev_time = 0;						//Tracking variable
	prev_time_dbl = 0.0;				//Tracking variable

	//By default, assumed we want to use IEEE 1547a
	ieee_1547_version = IEEE1547A;

	//Flag us as no reason
	ieee_1547_trip_method = IEEE_1547_NONE;

	//1547a defaults for triggering - so people can change them - will get adjusted to 1547 in init, if desired
	over_freq_high_band_setpoint = 62.0;	//OF2 set point for IEEE 1547a
	over_freq_high_band_delay = 0.16;		//OF2 clearing time for IEEE1547a
	over_freq_high_band_viol_time = 0.0;	//Accumulator for IEEE1547a OF high-band violation time
	over_freq_low_band_setpoint = 60.5;		//OF1 set point for IEEE 1547a
	over_freq_low_band_delay = 2.0;			//OF1 clearing time for IEEE 1547a
	over_freq_low_band_viol_time = 0.0;	//Accumulator for IEEE1547a OF low-band violation time
	under_freq_high_band_setpoint = 59.5;	//UF2 set point for IEEE 1547a
	under_freq_high_band_delay = 2.0;		//UF2 clearing time for IEEE1547a
	under_freq_high_band_viol_time = 0.0;	//Accumulator for IEEE1547a UF high-band violation time
	under_freq_low_band_setpoint = 57.0;		//UF1 set point for IEEE 1547a
	under_freq_low_band_delay = 0.16;		//UF1 clearing time for IEEE 1547a
	under_freq_low_band_viol_time = 0.0;	//Accumulator for IEEE1547a UF low-band violation time

	//Voltage set points - 1547a defaults
	under_voltage_lowest_voltage_setpoint = 0.45;	//Lowest voltage threshold for undervoltage
	under_voltage_middle_voltage_setpoint = 0.60;	//Middle-lowest voltage threshold for undervoltage
	under_voltage_high_voltage_setpoint = 0.88;		//High value of low voltage threshold for undervoltage
	over_voltage_low_setpoint = 1.10;				//Lowest voltage value for overvoltage
	over_voltage_high_setpoint = 1.20;				//High voltage value for overvoltage
	under_voltage_lowest_delay = 0.16;				//Lowest voltage clearing time for undervoltage
	under_voltage_middle_delay = 1.0;				//Middle-lowest voltage clearing time for undervoltage
	under_voltage_high_delay = 2.0;					//Highest voltage clearing time for undervoltage
	over_voltage_low_delay = 1.0;					//Lowest voltage clearing time for overvoltage
	over_voltage_high_delay = 0.16;					//Highest voltage clearing time for overvoltage
	under_voltage_lowest_viol_time = 0.0;			//Lowest low voltage threshold violation timer
	under_voltage_middle_viol_time = 0.0;			//Middle low voltage threshold violation timer
	under_voltage_high_viol_time = 0.0;				//Highest low voltage threshold violation timer
	over_voltage_low_viol_time = 0.0;				//Lowest high voltage threshold violation timer
	over_voltage_high_viol_time = 0.0;				//Highest high voltage threshold violation timer

	node_nominal_voltage = 120.0;		//Just pick a value

	// Volt Var Parameters
	V_base = 0;
	V1 = -2;
	Q1 = -2;
	V2 = -2;
	Q2 = -2;
	V3 = -2;
	Q3 = -2;
	V4 = -2;
	Q4 = -2;
	vv_lockout = -1;

	// Feeder frequency
	mapped_freq_variable = nullptr;
	VSI_freq = 60;	// Set default VSI frequency as 60 HZ

	// Default values for VSI parameters
	// Filter of voltage source inverter
	Rfilter = 0.02; // in p.u.
	Xfilter = 0.1; // in p.u.

	// VSI mode is by default defined as the isochronous one
	VSI_mode = VSI_ISOCHRONOUS;

	//By default, e_source has not been initialized
	VSI_esource_init = false;

	// Initial values for voltage control parameters of isochronous VSI
	ki_Vterminal = 5.86;
	kp_Vterminal = 0;

	// Initial values for Pmax control for VSI_DROOP
	Pmin = 0;
	Pmax = 1;
	kppmax = 3;
	kipmax = 30;
	Pmax_Low_Limit = -100;

	// Initialize slew rate parameters
	checkRampRate_real = false;
	rampUpRate_real = 1.0e9;		//1 GW/s default because, why not
	rampDownRate_real = 1.0e9;	//1 GW/s default because symmetry
	checkRampRate_reactive = false;
	rampUpRate_reactive = 1.0e9;		//1 GVAr/s default because, why not
	rampDownRate_reactive = 1.0e9;	//1 GVAr/s default because symmetry
	prev_VA_out[0] = prev_VA_out[1] = prev_VA_out[2] = gld::complex(0.0,0.0);
	curr_VA_out[0] = curr_VA_out[1] = curr_VA_out[2] = gld::complex(0.0,0.0);
	event_deltat = 10000000.0;	//Make very large, so first step in doesn't have a divide by zero
	parent_is_a_meter = false;		//By default, no parent meter
	parent_is_triplex = false;		//By default, we're not triplex
	attached_bus_type = 0;			//By default, we're basically a PQ bus

	swing_test_fxn = nullptr;			//By default, no mapping

	pCircuit_V[0] = pCircuit_V[1] = pCircuit_V[2] = nullptr;
	pLine_I[0] = pLine_I[1] = pLine_I[2] = nullptr;
	pLine_unrotI[0] = pLine_unrotI[1] = pLine_unrotI[2] = nullptr;
	pPower[0] = pPower[1] = pPower[2] = nullptr;
	pIGenerated[0] = pIGenerated[1] = pIGenerated[2] = nullptr;
	pLine12 = nullptr;
	pPower12 = nullptr;
	pMeterStatus = nullptr;
	pbus_full_Y_mat = nullptr;
	pGenerated = nullptr;

	//Zero the accumulators
	value_Circuit_V[0] = value_Circuit_V[1] = value_Circuit_V[2] = gld::complex(0.0,0.0);
	value_Line_I[0] = value_Line_I[1] = value_Line_I[2] = gld::complex(0.0,0.0);
	value_Line_unrotI[0] = value_Line_unrotI[1] = value_Line_unrotI[2] = gld::complex(0.0,0.0);
	value_Power[0] = value_Power[1] = value_Power[2] = gld::complex(0.0,0.0);
	value_IGenerated[0] = value_IGenerated[1] = value_IGenerated[2] = gld::complex(0.0,0.0);
	prev_value_IGenerated[0] = prev_value_IGenerated[1] = prev_value_IGenerated[2] = gld::complex(0.0,0.0);
	value_Line12 = gld::complex(0.0,0.0);
	value_Power12 = gld::complex(0.0,0.0);
	value_MeterStatus = 1;	//Connected, by default
	
	// Volt-Watt parameters
	VW_V1 = -2;
	VW_V2 = -2;
	VW_P1 = -2;
	VW_P2 = -2;

	//Set up the deltamode "next state" tracking variable
	desired_simulation_mode = SM_EVENT;

	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int inverter::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	PROPERTY *pval = nullptr;
	bool *dyn_gen_posting;
	unsigned iindex, jindex;
	gld::complex filter_impedance;
	double *nominal_voltage;
	double *ptemp_double;
	double temp_double_high, temp_double_low, tdiff, ang_diff;
	FINDLIST *batteries = nullptr;
	OBJECT *objBattery = nullptr;
	int index = 0;
	STATUS return_value_init;
	double temp_volt_mag;
	gld_property *temp_property_pointer = nullptr;
	gld_property *Frequency_mapped = nullptr;
	gld_wlock *test_rlock = nullptr;
	bool temp_bool_value;
	int temp_idx_x, temp_idx_y;
	gld::complex temp_complex_value;
	complex_array temp_complex_array, temp_child_complex_array;;
	set parent_phases;
	OBJECT *tmp_obj = nullptr;
	gld_object *tmp_gld_obj = nullptr;
	bool childed_connection = false;
	WT_is_connected = false;

	if(parent != nullptr){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("inverter::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	// construct circuit variable map to meter
	std::string tempV, tempQ, tempf, tempP;
	std::string VoltVArSchedInput, freq_pwrSchedInput;

	//See if the global flag is set - if so, add the object flag
	if (all_generator_delta)
	{
		obj->flags |= OF_DELTAMODE;
	}

	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true;	//Set the flag and off we go
	}

	// find parent meter or triplex_meter, if not defined, use default voltages, and if
	// the parent is not a meter throw an exception
	if (parent!=nullptr)
	{
		//See what kind of parent we are
		if (gl_object_isa(parent,"meter","powerflow") || gl_object_isa(parent,"node","powerflow") || gl_object_isa(parent,"load","powerflow") ||
			gl_object_isa(parent,"triplex_meter","powerflow") || gl_object_isa(parent,"triplex_node","powerflow") || gl_object_isa(parent,"triplex_load","powerflow"))
		{
			//See if we're in deltamode and VSI - if not, we don't care about the "parent-ception" mapping
			//Normal deltamode just goes through current interfaces, so don't need this craziness
			if (deltamode_inclusive && (four_quadrant_control_mode == FQM_VSI))
			{
				//See if this attached node is a child or not
				if (parent->parent != nullptr)
				{
					//Map parent - wherever it may be
					temp_property_pointer = new gld_property(parent,"NR_powerflow_parent");

					//Make sure it worked
					if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_objectref())
					{
						GL_THROW("inverter:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
						//Defined elsewhere
					}

					//Pull the mapping - gld_object
					tmp_gld_obj = temp_property_pointer->get_objectref();

					//Pull the proper object reference
					tmp_obj = tmp_gld_obj->my();

					//free the property
					delete temp_property_pointer;

					//See what it is
					if (!gl_object_isa(tmp_obj, "meter", "powerflow") &&
						!gl_object_isa(tmp_obj, "node", "powerflow") &&
						!gl_object_isa(tmp_obj, "load", "powerflow") &&
						!gl_object_isa(tmp_obj, "triplex_meter", "powerflow") &&
						!gl_object_isa(tmp_obj, "triplex_node", "powerflow") &&
						!gl_object_isa(tmp_obj, "triplex_load", "powerflow"))
					{
						//Not a wierd map, just use normal parent
						tmp_obj = parent;
					}
					else	//Implies it is a powerflow parent
					{
						//Set flag
						childed_connection = true;

						//See if we are deltamode-enabled -- if so, flag our parent while we're here
						//Map our deltamode flag and set it (parent will be done below)
						temp_property_pointer = new gld_property(parent,"Norton_dynamic");

						//Make sure it worked
						if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_bool())
						{
							GL_THROW("inverter:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
							/*  TROUBLESHOOT
							While attempting to set up the deltamode interfaces and calculations with powerflow, the required interface could not be mapped.
							Please check your GLM and try again.  If the error persists, please submit a trac ticket with your code.
							*/
						}

						//Flag it to true
						temp_bool_value = true;
						temp_property_pointer->setp<bool>(temp_bool_value,*test_rlock);

						//Remove it
						delete temp_property_pointer;

						//Set the child accumulator flag too
						temp_property_pointer = new gld_property(tmp_obj,"Norton_dynamic_child");

						//Make sure it worked
						if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_bool())
						{
							GL_THROW("diesel_dg:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
							//Defined elsewhere
						}

						//Flag it to true
						temp_bool_value = true;
						temp_property_pointer->setp<bool>(temp_bool_value,*test_rlock);

						//Remove it
						delete temp_property_pointer;
					}//End we were a powerflow child
				}//Parent has a parent
				else	//It is null
				{
					//Just point it to the normal parent
					tmp_obj = parent;
				}
			}//End deltamode inclusive
			else	//Not deltamode and VSI -- all other combinations just use standard parents
			{
				//Just point it to the normal parent
				tmp_obj = parent;
			}

			//Determine parent type
			//Triplex first, otherwise it tries to map to three-phase (since all triplex are nodes)
			if (gl_object_isa(tmp_obj, "triplex_meter", "powerflow") ||
				gl_object_isa(tmp_obj, "triplex_node", "powerflow") ||
				gl_object_isa(tmp_obj, "triplex_load", "powerflow"))
			{
				//Indicate this is a meter, but is triplex too
				parent_is_a_meter = true;
				parent_is_triplex = true;

				//Map the various powerflow variables
				pCircuit_V[0] = map_complex_value(tmp_obj,"voltage_12");
				pCircuit_V[1] = map_complex_value(tmp_obj,"voltage_1N");
				pCircuit_V[2] = map_complex_value(tmp_obj,"voltage_2N");

				//Null these -- not used
				pLine_I[0] = nullptr;
				pLine_I[1] = nullptr;
				pLine_I[2] = nullptr;

				//Get 12
				pLine12 = map_complex_value(tmp_obj,"current_12");
				pPower12 = map_complex_value(tmp_obj,"power_12");

				//Individual ones not used
				pPower[0] = nullptr;	//Not used
				pPower[1] = nullptr;	//Not used
				pPower[2] = nullptr;	//Not used

				pLine_unrotI[0] = map_complex_value(tmp_obj,"prerotated_current_12");
				pLine_unrotI[1] = nullptr;	//Not used
				pLine_unrotI[2] = nullptr;	//Not used

				//Map IGenerated, even though triplex can't really use this yet (just for the sake of doing so)
				pIGenerated[0] = map_complex_value(tmp_obj,"deltamode_generator_current_12");
				pIGenerated[1] = nullptr;
				pIGenerated[2] = nullptr;
			}//End triplex parent
			else if (gl_object_isa(tmp_obj, "meter", "powerflow") ||
					 gl_object_isa(tmp_obj, "node", "powerflow") ||
					 gl_object_isa(tmp_obj, "load", "powerflow"))
			{
				//Indicate this is a meter, but not triplex
				parent_is_a_meter = true;
				parent_is_triplex = false;

				//Map the various powerflow variables
				pCircuit_V[0] = map_complex_value(tmp_obj,"voltage_A");
				pCircuit_V[1] = map_complex_value(tmp_obj,"voltage_B");
				pCircuit_V[2] = map_complex_value(tmp_obj,"voltage_C");

				pLine_I[0] = map_complex_value(tmp_obj,"current_A");
				pLine_I[1] = map_complex_value(tmp_obj,"current_B");
				pLine_I[2] = map_complex_value(tmp_obj,"current_C");

				pPower[0] = map_complex_value(tmp_obj,"power_A");
				pPower[1] = map_complex_value(tmp_obj,"power_B");
				pPower[2] = map_complex_value(tmp_obj,"power_C");

				pLine_unrotI[0] = map_complex_value(tmp_obj,"prerotated_current_A");
				pLine_unrotI[1] = map_complex_value(tmp_obj,"prerotated_current_B");
				pLine_unrotI[2] = map_complex_value(tmp_obj,"prerotated_current_C");

				//Map the current injection variables
				pIGenerated[0] = map_complex_value(tmp_obj,"deltamode_generator_current_A");
				pIGenerated[1] = map_complex_value(tmp_obj,"deltamode_generator_current_B");
				pIGenerated[2] = map_complex_value(tmp_obj,"deltamode_generator_current_C");
			}//End non-triplex parent

			//*** Common items ****//
			// Many of these go to the "true parent", not the "powerflow parent"

			//See if we are deltamode-enabled -- powerflow parent version
			if (deltamode_inclusive && (four_quadrant_control_mode == FQM_VSI))
			{
				//Map our deltamode flag and set it (parent will be done below)
				temp_property_pointer = new gld_property(tmp_obj,"Norton_dynamic");

				//Make sure it worked
				if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_bool())
				{
					GL_THROW("inverter:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",tmp_obj->name?tmp_obj->name:"unnamed");
					//Defined elsewhere
				}

				//Flag it to true
				temp_bool_value = true;
				temp_property_pointer->setp<bool>(temp_bool_value,*test_rlock);

				//Remove it
				delete temp_property_pointer;

				//Map like the diesel generators -- one VSI will be the "master of overall frequency"
				//Perform the mapping check for frequency variable -- if no one has elected yet, we become master of frequency
				//Temporary deltamode workarond until elec_frequency object is complete
				Frequency_mapped = nullptr;

				//Get linking to checker variable
				Frequency_mapped = new gld_property("powerflow::master_frequency_update");

				//See if it worked
				if (!Frequency_mapped->is_valid() || !Frequency_mapped->is_bool())
				{
					GL_THROW("inverter:%s - Failed to map frequency checking variable from powerflow for deltamode",obj->name?obj->name:"unnamed");
					/*  TROUBLESHOOT
					While attempting to map one of the electrical frequency update variables from the powerflow module, an error
					was encountered.  Please try again, insuring the inverter is parented to a deltamode powerflow object.  If
					the error persists, please submit your code and a bug report via the ticketing system.
					*/
				}

				//Pull the value
				Frequency_mapped->getp<bool>(temp_bool_value,*test_rlock);
				
				//Check the value
				if (!temp_bool_value)	//No one has mapped yet, we are volunteered
				{
					//Update powerflow frequency
					mapped_freq_variable = new gld_property("powerflow::current_frequency");

					//Make sure it worked
					if (!mapped_freq_variable->is_valid() || !mapped_freq_variable->is_double())
					{
						GL_THROW("inverter:%s - Failed to map frequency checking variable from powerflow for deltamode",obj->name?obj->name:"unnamed");
						//Defined above
					}

					//Flag the frequency mapping as having occurred
					temp_bool_value = true;
					Frequency_mapped->setp<bool>(temp_bool_value,*test_rlock);
				}
				//Default else -- someone else is already mapped, just continue onward

				//Delete the reference
				delete Frequency_mapped;

				// Obtain the Zbase of the system for calculating filter impedance
				//Link to nominal voltage
				temp_property_pointer = new gld_property(parent,"nominal_voltage");

				//Make sure it worked
				if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
				{
					gl_error("inverter:%d %s failed to map the nominal_voltage property",obj->id, (obj->name ? obj->name : "Unnamed"));
					/*  TROUBLESHOOT
					While attempting to map the nominal_voltage property, an error occurred.  Please try again.
					If the error persists, please submit your GLM and a bug report to the ticketing system.
					*/

					return FAILED;
				}
				//Default else, it worked

				//Copy that value out
				if (parent_is_triplex)
				{
					node_nominal_voltage = 2.0 * temp_property_pointer->get_double();	//Adjust for 240 V
				}
				else
				{
					node_nominal_voltage = temp_property_pointer->get_double();
				}

				//Remove the property pointer
				delete temp_property_pointer;

				Zbase = (node_nominal_voltage * node_nominal_voltage)/p_rated;
				filter_impedance = gld::complex(1.0,0.0)/(gld::complex(Rfilter,Xfilter) * Zbase);

				for (iindex=0; iindex<3; iindex++)
				{
					for (jindex=0; jindex<3; jindex++)
					{
						if (iindex==jindex)
						{
							generator_admittance[iindex][jindex] = filter_impedance;
						}
						else
						{
							generator_admittance[iindex][jindex] = gld::complex(0.0,0.0);
						}
					}
				}

				//Map the full_Y parameter to inject the admittance portion into it
				pbus_full_Y_mat = new gld_property(tmp_obj,"deltamode_full_Y_matrix");

				//Check it
				if (!pbus_full_Y_mat->is_valid() || !pbus_full_Y_mat->is_complex_array())
				{
					GL_THROW("inverter:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",tmp_obj->name?tmp_obj->name:"unnamed");
					/*  TROUBLESHOOT
					While attempting to set up the deltamode interfaces and calculations with powerflow, the required interface could not be mapped.
					Please check your GLM and try again.  If the error persists, please submit a trac ticket with your code.
					*/
				}

				//Pull down the variable
				pbus_full_Y_mat->getp<complex_array>(temp_complex_array,*test_rlock);

				//See if it is valid
				if (!temp_complex_array.is_valid(0, 0))
				{
					//Create it
					temp_complex_array.grow_to(3,3);

					//Zero it, by default
					for (temp_idx_x=0; temp_idx_x<3; temp_idx_x++)
					{
						for (temp_idx_y=0; temp_idx_y<3; temp_idx_y++)
						{
							temp_complex_array.set_at(temp_idx_x,temp_idx_y,gld::complex(0.0,0.0));
						}
					}
				}
				else	//Already populated, make sure it is the right size!
				{
					if ((temp_complex_array.get_rows() != 3) && (temp_complex_array.get_cols() != 3))
					{
						GL_THROW("inverter:%s exposed Norton-equivalent matrix is the wrong size!",obj->name?obj->name:"unnamed");
						/*  TROUBLESHOOT
						While mapping to an admittance matrix on the parent node device, it was found it is the wrong size.
						Please try again.  If the error persists, please submit your code and model via the issue tracking system.
						*/
					}
					//Default else -- right size
				}

				//See if we were connected to a powerflow child
				if (childed_connection == true)
				{
					temp_property_pointer = new gld_property(parent,"deltamode_full_Y_matrix");

					//Check it
					if (!temp_property_pointer->is_valid() || (temp_property_pointer->is_complex_array() != true))
					{
						GL_THROW("diesel_dg:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
						//Defined above
					}

					//Pull down the variable
					temp_property_pointer->getp<complex_array>(temp_child_complex_array,*test_rlock);

					//See if it is valid
					if (!temp_child_complex_array.is_valid(0,0))
					{
						//Create it
						temp_child_complex_array.grow_to(3,3);

						//Zero it, by default
						for (temp_idx_x=0; temp_idx_x<3; temp_idx_x++)
						{
							for (temp_idx_y=0; temp_idx_y<3; temp_idx_y++)
							{
								temp_child_complex_array.set_at(temp_idx_x,temp_idx_y,gld::complex(0.0,0.0));
							}
						}
					}
					else	//Already populated, make sure it is the right size!
					{
						if ((temp_child_complex_array.get_rows() != 3) && (temp_child_complex_array.get_cols() != 3))
						{
							GL_THROW("diesel_dg:%s exposed Norton-equivalent matrix is the wrong size!",obj->name?obj->name:"unnamed");
							//Defined above
						}
						//Default else -- right size
					}
				}//End childed powerflow parent

				//Loop through and store the values
				for (temp_idx_x=0; temp_idx_x<3; temp_idx_x++)
				{
					for (temp_idx_y=0; temp_idx_y<3; temp_idx_y++)
					{
						//Read the existing value
						temp_complex_value = temp_complex_array.get_at(temp_idx_x,temp_idx_y);

						//Accumulate into it
						temp_complex_value += generator_admittance[temp_idx_x][temp_idx_y];

						//Store it
						temp_complex_array.set_at(temp_idx_x,temp_idx_y,temp_complex_value);

						//Do the childed object, if exists
						if (childed_connection)
						{
							//Read the existing value
							temp_complex_value = temp_child_complex_array.get_at(temp_idx_x,temp_idx_y);

							//Accumulate into it
							temp_complex_value += generator_admittance[temp_idx_x][temp_idx_y];

							//Store it
							temp_child_complex_array.set_at(temp_idx_x,temp_idx_y,temp_complex_value);
						}
					}
				}

				//Push it back up
				pbus_full_Y_mat->setp<complex_array>(temp_complex_array,*test_rlock);

				//See if the childed powerflow exists
				if (childed_connection)
				{
					temp_property_pointer->setp<complex_array>(temp_child_complex_array,*test_rlock);

					//Clear it
					delete temp_property_pointer;
				}

				//Map the power variable
				pGenerated = map_complex_value(tmp_obj,"deltamode_PGenTotal");

				// Find if a battery is attached to VSI, if not, give warning
				// Find all batteries
				batteries = gl_find_objects(FL_NEW, FT_CLASS, SAME, "battery", FT_END);
				if(batteries == nullptr || batteries->hit_count == 0){
					gl_warning("No battery objects were found, but the VSI object exists. Now assume the VSI is attached with infinite input power.");
					/* TROUBLESHOOT
					No battery object attached to VSI. In reality, a battery is required for VSI to output enough power.
					*/
				}
				else {
					while(objBattery = gl_find_next(batteries,objBattery)){
						if(index >= batteries->hit_count){
							gl_warning("VSI: %s does not find a battery attached to it. Now assume VSI: %s is attached with infinite input power.", (obj->name ? obj->name : "Unnamed"), (obj->name ? obj->name : "Unnamed"));
							break;
						}
						if (strcmp(objBattery->parent->name, obj->name) == 0) {
							break;
						}
						++index;
					}
				}
			}//End VSI common items

			//Map status - true parent
			pMeterStatus = new gld_property(parent,"service_status");

			//Check it
			if (!pMeterStatus->is_valid() || !pMeterStatus->is_enumeration())
			{
				GL_THROW("Inverter failed to map powerflow status variable");
				/*  TROUBLESHOOT
				While attempting to map the service_status variable of the parent
				powerflow object, an error occurred.  Please try again.  If the error
				persists, please submit your code and a bug report via the trac website.
				*/
			}

			//Map and pull the phases - true parent
			temp_property_pointer = new gld_property(parent,"phases");

			//Make sure ti worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_set())
			{
				GL_THROW("Unable to map phases property - ensure the parent is a meter or triplex_meter");
				/*  TROUBLESHOOT
				While attempting to map the phases property from the parent object, an error was encountered.
				Please check and make sure your parent object is a meter or triplex_meter inside the powerflow module and try
				again.  If the error persists, please submit your code and a bug report via the Trac website.
				*/
			}

			//Pull the phase information
			parent_phases = temp_property_pointer->get_set();

			//Clear the temporary pointer
			delete temp_property_pointer;

			//Pull the bus type
			temp_property_pointer = new gld_property(tmp_obj, "bustype");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_enumeration())
			{
				GL_THROW("inverter:%s failed to map bustype variable from %s", obj->name ? obj->name : "unnamed", obj->parent->name ? obj->parent->name : "unnamed");
				/*  TROUBLESHOOT
				While attempting to map the bustype variable from the parent node, an error was encountered.  Please try again.  If the error
				persists, please report it with your GLM via the issues tracking system.
				*/
			}

			//Pull the value of the bus
			attached_bus_type = temp_property_pointer->get_enumeration();

			//Remove it
			delete temp_property_pointer;

			//Map the frequency measurement - powerflow parent
			pFrequency = new gld_property(tmp_obj,"measured_frequency");

			//Make sure it worked
			if (!pFrequency->is_valid() || !pFrequency->is_double())
			{
				GL_THROW("inverter:%d %s failed to map the measured_frequency property",obj->id, (obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the measured_frequency property, an error occurred.  Please try again.
				If the error persists, please submit your GLM and a bug report to the ticketing system.
				*/
			}

			//Powerflow values -- pull the initial value (should be nominals)
			value_Circuit_V[0] = pCircuit_V[0]->get_complex();	//A or 12
			value_Circuit_V[1] = pCircuit_V[1]->get_complex();	//B or 1N
			value_Circuit_V[2] = pCircuit_V[2]->get_complex();	//C or 2N
		}//End valid powerflow parent
		else	//Not sure what it is
		{
			GL_THROW("Inverter must have a valid powerflow object as its parent, or no parent at all");
			/*  TROUBLESHOOT
			Check the parent object of the inverter.  The inverter is only able to be childed via to powerflow objects.
			Alternatively, you can also choose to have no parent, in which case the inverter will be a stand-alone application
			using default voltage values for solving purposes.
			*/
		}
	}//End parent call
	else	//Must not have a parent
	{
		//Indicate we don't have a meter parent, nor is it triplex (though the latter shouldn't matter)
		parent_is_a_meter = false;
		parent_is_triplex = false;
		
		gl_warning("Inverter:%d has no parent meter object defined; using static voltages", obj->id);
		/*  TROUBLESHOOT
		An inverter in the system does not have a parent attached.  It is using static values for the voltage.
		*/
		
		// Declare all 3 phases
		parent_phases = 0x07;

		//Powerflow values -- set defaults here -- sets up like three-phase connection - use rated kV, just because
		//Set that magnitude
		temp_volt_mag = Rated_kV * 1000.0 / sqrt(3.0);
		value_Circuit_V[0].SetPolar(temp_volt_mag,0);
		value_Circuit_V[1].SetPolar(temp_volt_mag,-2.0/3.0*PI);
		value_Circuit_V[2].SetPolar(temp_volt_mag,2.0/3.0*PI);

		//Define the default
		value_MeterStatus = 1;

		//Set the frequency
		value_Frequency = 60.0;
	}

	//See if our phases are empty - if so, just snag the parent's
	if (phases == 0x00)
	{
		phases = parent_phases;
	}

	//Check the phases with the "parent phases" - make sure it is a valid combination
	if ((((phases & 0x10) == 0x10) && ((parent_phases & 0x10) != 0x10)) || (((parent_phases & 0x10) == 0x10) && ((phases & 0x10) != 0x10)))
	{
		GL_THROW("Inverter:%d %s - Inverter has a triplex-related phase mismatch!",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		An inverter is either expecting a triplex connection and is connected to anon-triplex node, or is expecting a non-triplex connection but is
		connected to one.  Fix the model and try again.
		*/
	}
	else if ((parent_phases & phases) != phases)
	{
		GL_THROW("Inverter:%d %s - Inverter phases are incompatible with the parent object phases!",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The phases implied by the inverter do not match the capabilites of the parent object.  Reconcile this difference.
		*/
	}

	// count the number of phases
	if ( (phases & 0x10) == 0x10) // split phase
		number_of_phases_out = 1; 
	else if ( (phases & 0x07) == 0x07 ) // three phase
		number_of_phases_out = 3;
	else if ( ((phases & 0x03) == 0x03) || ((phases & 0x05) == 0x05) || ((phases & 0x06) == 0x06) ) // two-phase
		number_of_phases_out = 2;
	else if ( ((phases & 0x01) == 0x01) || ((phases & 0x02) == 0x02) || ((phases & 0x04) == 0x04) ) // single phase
		number_of_phases_out = 1;
	else
	{
		//Never supposed to really get here
		GL_THROW("Invalid phase configuration specified!");
		/*  TROUBLESHOOT
		An invalid phase congifuration was specified when attaching to the "parent" object.  Please report this
		error.
		*/
	}

	if ((gen_mode_v == UNKNOWN) && (inverter_type_v != FOUR_QUADRANT))
	{
		gl_warning("Inverter control mode is not specified! Using default: CONSTANT_PF");
		gen_mode_v = (enumeration)CONSTANT_PF;
	}
	if (gen_status_v == UNKNOWN)
	{
		gl_warning("Inverter status is unknown! Using default: ONLINE");
		gen_status_v = (enumeration)ONLINE;
	}
	if (inverter_type_v == UNKNOWN)
	{
		gl_warning("Inverter type is unknown! Using default: PWM");
		inverter_type_v = (enumeration)PWM;
	}
			
	//Dump efficiency from inv_eta first - it will get overwritten if it is bad
	efficiency=inv_eta;

	//all other variables set in input file through public parameters
	switch(inverter_type_v)
	{
		case TWO_PULSE:
			if (inv_eta==0)
			{
				efficiency = 0.8;
				gl_warning("Efficiency unspecified - defaulted to %f for this inverter type",efficiency);
				/*  TROUBLESHOOT
				An inverter_efficiency value was not explicitly specified for this inverter.  A default
				value was specified in its place.  If the default value is not acceptable, please explicitly
				set inverter_efficiency in the GLM file.
				*/
			}
			break;
		case SIX_PULSE:
			if (inv_eta==0)
			{
				efficiency = 0.8;
				gl_warning("Efficiency unspecified - defaulted to %f for this inverter type",efficiency);
				//defined above
			}
			break;
		case TWELVE_PULSE:
			if (inv_eta==0)
			{
				efficiency = 0.8;
				gl_warning("Efficiency unspecified - defaulted to %f for this inverter type",efficiency);
				//defined above
			}
			break;
		case PWM:
			if (inv_eta==0)
			{
				efficiency = 0.9;
				gl_warning("Efficiency unspecified - defaulted to %f for this inverter type",efficiency);
				//defined above
			}
			break;
		case FOUR_QUADRANT:
			//begin Ab add

			if (inv_eta==0){
				efficiency = 0.9;	//Unclear why this is split in 4-quadrant, but not adjusting for fear of breakage
				gl_warning("Efficiency unspecified - defaulted to %f for this inverter type",efficiency); //defined above
			}
			if(inv_eta == 0){
				inv_eta = 0.9;} 
			else if(inv_eta < 0){
				inv_eta = 0.9;
				gl_warning("Inverter efficiency must be positive--using default value");
			}
			
			if(p_rated == 0){
				p_rated = 25000;
				//throw("Inverter must have a nonzero power rating.");
				gl_warning("Inverter must have a nonzero power rating--using default value");
			} 
			
			//Ab : p_rated per phase
			if(number_of_phases_out == 1){
				bp_rated = p_rated/inv_eta;} 
			else if(number_of_phases_out == 2){
				bp_rated = 2*p_rated/inv_eta;} 
			else if(number_of_phases_out == 3){
				bp_rated = 3*p_rated/inv_eta;
			}
			//Ab : p_rated per phase; p_max for entire inverter	
			if(!use_multipoint_efficiency){
				if(p_max == -1){
					p_max = bp_rated*inv_eta;
				}
			} 
			
			if(four_quadrant_control_mode == FQM_VOLT_VAR_FREQ_PWR)
			{	//Volt_VAr init
				if (delay_time < 0)	{
					delay_time = 0.0;
					gl_warning("Delay time for Volt/VAr mode unspecified or negative. Setting to default of %f", delay_time);
				}
				if (max_var_slew_rate <= 0)	{
					max_var_slew_rate = -1.0;		//negative values of max_var_slew_rate disable that setting and place no restrictions on inverter VAr slew rate
					gl_warning("Maximum VAr slew rate for Volt-VAr mode unspecified, negative or zero. Disabling");
				}
				if (max_pwr_slew_rate <= 0)	{
					max_pwr_slew_rate = -1.0;		//negative values of max_pwr_slew_rate disable that setting and place no restrictions on inverter power slew rate
					gl_warning("Maximum output power slew rate for freq-power mode unspecified, negative or zero. Disabling");
				}
				
				//checks for Volt-VAr schedule
				VoltVArSchedInput = volt_var_sched;
				gl_warning(VoltVArSchedInput.c_str());
				if(VoltVArSchedInput.length() == 0)	{
					VoltVArSched.push_back(std::make_pair (119.5,0));	
					//put two random things on the schedule with Q values of zero, all scheduled Qs will then be zero
					VoltVArSched.push_back(std::make_pair (120.5,0));
					gl_warning("Volt/VAr schedule unspecified. Setting inverter for constant power factor of 1.0");
				}
				else
				{
					//parse user input string to produce volt/var schedule
					int cntr = 0;
					//std::string tempV = "";
					tempV = "";
					tempQ = "";
					for(int i = 0; i < VoltVArSchedInput.length(); i++)	{
						if(VoltVArSchedInput[i] != ',')	{
							if(cntr % 2 == 0)
								tempV += VoltVArSchedInput[i];
							else
								tempQ += VoltVArSchedInput[i];
						}
						else
						{					
							if(cntr % 2 == 1){
								VoltVArSched.push_back(std::make_pair (atof(tempV.c_str()),atof(tempQ.c_str())));
								tempQ = "";
								tempV = "";
							}
							cntr++;
						}
					}
					if(cntr % 2 == 1)
						VoltVArSched.push_back(std::make_pair (atof(tempV.c_str()),atof(tempQ.c_str())));
				} //end VoltVArSchedInput
				
					
				//checks for freq-power schedule
				freq_pwrSchedInput = freq_pwr_sched;
				gl_warning(freq_pwrSchedInput.c_str());
				if(freq_pwrSchedInput.length() == 0)	{
					freq_pwrSched.push_back(std::make_pair (f_nominal*0.9,0));	
					//make both power values equal to zero, then all scheduled powers will be zero
					freq_pwrSched.push_back(std::make_pair (f_nominal*1.1,0));
					gl_warning("Frequency-Power schedule unspecified. Setting power for frequency regulation to zero.");
				}
				else
				{
					//parse user input string to produce freq-PabsorbedLimit schedule
					int cntr = 0;
					tempf = "";
					tempP = "";
					for(int i = 0; i < freq_pwrSchedInput.length(); i++)	{
						if(freq_pwrSchedInput[i] != ',')	{
							if(cntr % 2 == 0)
								tempf += freq_pwrSchedInput[i];
							else
								tempP += freq_pwrSchedInput[i];
						}
						else
						{					
							if(cntr % 2 == 1) {
								freq_pwrSched.push_back(std::make_pair (atof(tempf.c_str()),atof(tempP.c_str())));
								tempf = "";
								tempP = "";
							}
							cntr++;
						}
					}
					if(cntr % 2 == 1)
						freq_pwrSched.push_back(std::make_pair (atof(tempf.c_str()),atof(tempP.c_str())));
				} //end freq_pwrSchedInput
				
			} //end VOLT_VAR_FREQ_PWR control mode
			//end Ab add
			
			if(four_quadrant_control_mode == FQM_LOAD_FOLLOWING || pf_reg == INCLUDED || four_quadrant_control_mode == FQM_GROUP_LF )
			{
				//Make sure we have an appropriate object to look at, if null, steal our parent
				if (sense_object == nullptr)
				{
					if (parent!=nullptr)
					{
						//Put the parent in there
						sense_object = parent;
						gl_warning("inverter:%s - sense_object not specified for LOAD_FOLLOWING and/or power-factor regulation - attempting to use parent object",obj->name);
						/*  TROUBLESHOOT
						The inverter is currently configured for LOAD_FOLLOWING mode, but did not have an appropriate
						sense_object specified.  The inverter is therefore using the parented object as the expected
						sense_object.
						*/
					}
					else
					{
						gl_error("inverter:%s - LOAD_FOLLOWING and power-factor regulation will not work without a specified sense_object!",obj->name);
						/*  TROUBLESHOOT
						The inverter is currently configured for LOAD_FOLLOWING mode, but does not have
						an appropriate sense_object specified.  Please specify a proper object and try again.
						*/
						return 0;
					}
				}

				//See what kind of sense_object we are linked at - note that the current implementation only takes overall power
				if (gl_object_isa(sense_object,"node","powerflow"))
				{
					//Make sure it's a meter of some sort
					if (gl_object_isa(sense_object,"meter","powerflow") ||
					gl_object_isa(sense_object,"triplex_meter","powerflow"))
					{
						//Set flag
						sense_is_link = false;

						//Map to measured_power - regardless of object
						sense_power = new gld_property(sense_object,"measured_power");

						//Make sure it worked
						if (!sense_power->is_valid() || !sense_power->is_complex())
						{
							gl_error("inverter:%s - an error occurred while mapping the sense_object power measurement!",obj->name);
							/*  TROUBLEHSHOOT
							While attempting to map the property defining measured power on the sense_object, an error was encountered.
							Please try again.  If the error persists, please submit a bug report and your code via the trac website.
							*/
							return 0;
						}

						//Random warning about ranks, if not our parent
						if (sense_object != parent)
						{
							gl_warning("inverter:%s is LOAD_FOLLOWING and/or power-factor regulating based on a meter or triplex_meter, ensure the inverter is connected inline with that object!",obj->name);
							/*  TROUBLESHOOT
							The inverter operates in LOAD_FOLLOWING mode under the assumption the sense_object meter or triplex_meter is either attached to
							the inverter, or directly upstream in the flow.  If this assumption is violated, the results may not be as expected.
							*/
						}
					}
					else	//loads/nodes/triplex_nodes not supported
					{
						gl_error("inverter:%s - sense_object is a node, but not a meter or triplex_meter!",obj->name);
						/*  TROUBLESHOOT
						When in LOAD_FOLLOWING and the sense_object is a powerflow node, that powerflow object
						must be a meter or a triplex_meter.  Please change your model and try again.
						*/
						return 0;
					}
				}
				else if (gl_object_isa(sense_object,"link","powerflow"))
				{
					//Only transformers supported right now (functional link - just needs to be exported elsewhere)
					if (gl_object_isa(sense_object,"transformer","powerflow"))
					{
						//Set flag
						sense_is_link = true;

						//Link up the power_calculation() function
						powerCalc = (FUNCTIONADDR)(gl_get_function(sense_object,"power_calculation"));

						//Make sure it worked
						if (powerCalc==nullptr)
						{
							gl_error("inverter:%s - inverter failed to map power calculation function of transformer!",obj->name);
							/*  TROUBLESHOOT
							While attempting to link up the power_calculation function for the transformer specified in sense_object,
							something went wrong.  Please try again.  If the error persists, please post your code and a bug report via
							the trac website.
							*/
							return 0;
						}

						//Map to the property to compare - just use power_out for now (just as good as power_in)
						sense_power = new gld_property(sense_object,"power_out");

						//Make sure it worked
						if (!sense_power->is_valid() || !sense_power->is_complex())
						{
							gl_error("inverter:%s - an error occurred while mapping the sense_object power measurement!",obj->name);
							//Defined above
							return 0;
						}
					}
					else	//Not valid
					{
						gl_error("inverter:%s - sense_object is a link, but not a transformer!",obj->name);
						/*  TROUBLESHOOT
						When in LOAD_FOLLOWING and the sense_object is a powerflow link, that powerflow object
						must be a transformer.  Please change your model and try again.
						*/
						return 0;
					}

				}
				else	//Not a link or a node, we don't know what to do!
				{
					gl_error("inverter:%s - sense_object is not a proper powerflow object!",obj->name);
					/*  TROUBLESHOOT
					When in LOAD_FOLLOWING mode, the inverter requires an appropriate connection
					to a powerflow object to sense the current load.  This can be either a meter,
					triplex_meter, or transformer at the current time.
					*/
					return 0;
				}

				//Check lockout times
				if (pf_reg != EXCLUDED)
				{
					if (pf_reg_activate_lockout_time < 0)
					{
						pf_reg_activate_lockout_time = 60;
						gl_warning("inverter:%s - pf_reg_activate_lockout_time is unassigned, using default value of 60s.",obj->name);
						/*  TROUBLESHOOT
						The pf_reg_activate_lockout_time for the inverter is negative.  Negative lockout times
						are not allowed inside the inverter.  Please correct the value and try again.
						*/

					}
					else if (pf_reg_activate_lockout_time < 60)
					{
						pf_reg_activate_lockout_time = 1;
						gl_warning("inverter:%s - a short pf_reg_activate_lockout_time may lead to inverter controller oscillations. Recommended time: > 60s",obj->name);
						/*  TROUBLESHOOT
						The pf_reg_activate_lockout_time for the inverter is negative.  Negative lockout times
						are not allowed inside the inverter.  Please correct the value and try again.
						*/

					}

					else if (charge_lockout_time == 0.0)
					{
						gl_warning("inverter:%s - pf_reg_activate_lockout_time is zero, oscillations may occur",obj->name);
						/*  TROUBLESHOOT
						The value for pf_reg_activate_lockout_time is zero, which means there is no delay in new dispatch
						operations.  This may result in excessive switching and iteration limits being hit.  If this is
						not desired, specify a charge_lockout_time larger than zero.
						*/
					}
				}
				if (four_quadrant_control_mode == FQM_LOAD_FOLLOWING)
				{
					if (charge_lockout_time<0)
					{
						gl_error("inverter:%s - charge_lockout_time is negative!",obj->name);
						/*  TROUBLESHOOT
						The charge_lockout_time for the inverter is negative.  Negative lockout times
						are not allowed inside the inverter.  Please correct the value and try again.
						*/
						return 0;
					}
					else if (charge_lockout_time == 0.0)
					{
						gl_warning("inverter:%s - charge_lockout_time is zero, oscillations may occur",obj->name);
						/*  TROUBLESHOOT
						The value for charge_lockout_time is zero, which means there is no delay in new dispatch
						operations.  This may result in excessive switching and iteration limits being hit.  If this is
						not desired, specify a charge_lockout_time larger than zero.
						*/
					}
					//Defaulted else, must be okay

					if (discharge_lockout_time<0)
					{
						gl_error("inverter:%s - discharge_lockout_time is negative!",obj->name);
						/*  TROUBLESHOOT
						The discharge_lockout_time for the inverter is negative.  Negative lockout times
						are not allowed inside the inverter.  Please correct the value and try again.
						*/
						return 0;
					}
					else if (discharge_lockout_time == 0.0)
					{
						gl_warning("inverter:%s - discharge_lockout_time is zero, oscillations may occur",obj->name);
						/*  TROUBLESHOOT
						The value for discharge_lockout_time is zero, which means there is no delay in new dispatch
						operations.  This may result in excessive switching and iteration limits being hit.  If this is
						not desired, specify a discharge_lockout_time larger than zero.
						*/
					}
				} //End FQM_LOAD_FOLLOWING
				//Defaulted else, must be okay
			}//End FOUR_QUADRANT checks

			/* Ab moved from here to beginning of four_quadrant case
			if (inv_eta==0)
			{
				efficiency = 0.9;	//Unclear why this is split in 4-quadrant, but not adjusting for fear of breakage
				gl_warning("Efficiency unspecified - defaulted to %f for this inverter type",efficiency);
				//defined above
			}
			if(inv_eta == 0){
				inv_eta = 0.9;
			} else if(inv_eta < 0){
				gl_warning("Inverter efficiency must be positive--using default value");
				inv_eta = 0.9;
			}
			if(p_rated == 0){
				//throw("Inverter must have a nonzero power rating.");
				gl_warning("Inverter must have a nonzero power rating--using default value");
				p_rated = 25000;
			}
			if(number_of_phases_out == 1){
				bp_rated = p_rated/inv_eta;
			} else if(number_of_phases_out == 2){
				bp_rated = 2*p_rated/inv_eta;
			} else if(number_of_phases_out == 3){
				bp_rated = 3*p_rated/inv_eta;
			}
			if(!use_multipoint_efficiency){
				if(p_max == -1){
					p_max = bp_rated*inv_eta;
				}
			} */	
			break;
		default:
			//Never supposed to really get here
			GL_THROW("Invalid inverter type specified!");
			/*  TROUBLESHOOT
			An invalid inverter type was specified for the property inverter_type.  Please select one of
			the acceptable types and try again.
			*/
			break;
	}

	//Make sure efficiency is not an invalid value
	if ((efficiency<=0) || (efficiency>1))
	{
		GL_THROW("The efficiency specified for inverter:%s is invalid",obj->name);
		/*  TROUBLESHOOT
		The efficiency value specified must be greater than zero and less than or equal to
		1.0.  Please specify a value in that range.
		*/
	}

	//seting up defaults for multipoint efficiency
	if(use_multipoint_efficiency){
		switch(inverter_manufacturer){//all manufacturer defaults use the CEC parameters
			case NONE:
				if(p_dco < 0){
					gl_error("no maximum dc power was given for the inverter.");
					return 0;
				}
				if(v_dco < 0){
					gl_error("no maximum dc voltage was given for the inverter.");
					return 0;
				}
				if(p_so < 0){
					gl_error("no minimum dc power was given for the inverter.");
					return 0;
				}
				if(p_rated <= 0){
					gl_error("no rated per phase power was given for the inverter.");
					return 0;
				} else {
					switch (number_of_phases_out)
					{
						// single phase connection
						case 1:
							p_max = p_rated;
							break;
						// two-phase connection
						case 2:
							p_max = 2*p_rated;
							break;
						// three-phase connection
						case 3:
							p_max = 3*p_rated;
							break;
						default:
							//Never supposed to really get here
							GL_THROW("Invalid phase configuration specified!");
							/*  TROUBLESHOOT
							An invalid phase congifuration was specified when attaching to the "parent" object.  Please report this
							error.
							*/
							break;
					}
				}
				if(c_o == -1){
					c_o = 0;
				}
				if(c_1 == -1){
					c_1 = 0;
				}
				if(c_2 == -1){
					c_2 = 0;
				}
				if(c_3 == -1){
					c_3 = 0;
				}
				break;
			case FRONIUS:
				if(p_dco < 0){
					p_dco = 2879;
				}
				if(v_dco < 0){
					v_dco = 277;
				}
				if(p_so < 0){
					p_so = 27.9;
				}
				if(c_o == -1){
					c_o = -1.009e-5;
				}
				if(c_1 == -1){
					c_1 = -1.367e-5;
				}
				if(c_2 == -1){
					c_2 = -3.587e-5;
				}
				if(c_3 == -1){
					c_3 = -3.421e-3;
				}
				if(p_max < 0){
					p_max = 2700;
					switch (number_of_phases_out)
					{
						// single phase connection
						case 1:
							p_rated = p_max;
							break;
						// two-phase connection
						case 2:
							p_rated = p_max/2;
							break;
						// three-phase connection
						case 3:
							p_rated = p_max/3;
							break;
						default:
							//Never supposed to really get here
							GL_THROW("Invalid phase configuration specified!");
							/*  TROUBLESHOOT
							An invalid phase congifuration was specified when attaching to the "parent" object.  Please report this
							error.
							*/
							break;
					}
				}
				break;
			case SMA:
				if(p_dco < 0){
					p_dco = 2694;
				}
				if(v_dco < 0){
					v_dco = 302;
				}
				if(p_so < 0){
					p_so = 20.7;
				}
				if(c_o == -1){
					c_o = -1.545e-5;
				}
				if(c_1 == -1){
					c_1 = 6.525e-5;
				}
				if(c_2 == -1){
					c_2 = 2.836e-3;
				}
				if(c_3 == -1){
					c_3 = -3.058e-4;
				}
				if(p_max < 0){
					p_max = 2500;
					switch (number_of_phases_out)
					{
						// single phase connection
						case 1:
							p_rated = p_max;
							break;
						// two-phase connection
						case 2:
							p_rated = p_max/2;
							break;
						// three-phase connection
						case 3:
							p_rated = p_max/3;
							break;
						default:
							//Never supposed to really get here
							GL_THROW("Invalid phase configuration specified!");
							/*  TROUBLESHOOT
							An invalid phase congifuration was specified when attaching to the "parent" object.  Please report this
							error.
							*/
							break;
					}
				}
				break;
			case XANTREX:
				if(p_dco < 0){
					p_dco = 4022;
				}
				if(v_dco < 0){
					v_dco = 266;
				}
				if(p_so < 0){
					p_so = 24.1;
				}
				if(c_o == -1){
					c_o = -8.425e-6;
				}
				if(c_1 == -1){
					c_1 = 8.590e-6;
				}
				if(c_2 == -1){
					c_2 = 7.76e-4;
				}
				if(c_3 == -1){
					c_3 = -5.278e-4;
				}
				if(p_max < 0){
					p_max = 3800;
					switch (number_of_phases_out)
					{
						// single phase connection
						case 1:
							p_rated = p_max;
							break;
						// two-phase connection
						case 2:
							p_rated = p_max/2;
							break;
						// three-phase connection
						case 3:
							p_rated = p_max/3;
							break;
						default:
							//Never supposed to really get here
							GL_THROW("Invalid phase configuration specified!");
							/*  TROUBLESHOOT
							An invalid phase congifuration was specified when attaching to the "parent" object.  Please report this
							error.
							*/
							break;
					}
				}
				break;
		}
		if(p_max > p_dco){
			gl_error("The maximum dc power into the inverter cannot be less than the maximum ac power out.");
			return 0;
		}
		if(p_so > p_dco){
			gl_error("The maximum dc power into the inverter cannot be less than the minimum dc power.");
			return 0;
		}
	}
	

	if (four_quadrant_control_mode == FQM_VOLT_VAR) {
		if (V1 == -2) {
			V1 = 0.97;
		}
		if (V2 == -2) {
			V2 = 0.99;
		}
		if (V3 == -2) {
			V3 = 1.01;
		}
		if (V4 == -2) {
			V4 = 1.03;
		}
		if (Q1 == -2) {
			Q1 = 0.50;
		}
		if (Q2 == -2) {
			Q2 = 0.0;
		}
		if (Q3 == -2) {
			Q3 = 0.0;
		}
		if (Q4 == -2) {
			Q4 = -0.50;
		}
		if (V1 > V2 || V2 > V3 || V3 > V4) {
			gl_error("inverter::init(): The curve was not constructed properly. V1 <= V2 <= V3 <= V4 must be true.");
			return 0;
		}
		if (Q1 < Q2 || Q2 < Q3 || Q3 < Q4) {
			gl_error("inverter::init(): The curve was not constructed properly. Q1 >= Q2 >= Q3 >= Q4 must be true.");
			return 0;
		}
		if (V_base == 0) {
			gl_error("inverter::init(): The base voltage must be greater than 0.");
			return 0;
		}
		if (V2 != V1) {
			m12 = (Q2 - Q1) / (V2 - V1);
		} else {
			m12 = 0;
		}
		if (V3 != V2) {
			m23 = (Q3 - Q2) / (V3 - V2);
		} else {
			m23 = 0;
		}
		if (V4 != V3) {
			m34 = (Q4 - Q3) / (V4 - V3);
		} else {
			m34 = 0;
		}
		b12 = Q1 - (m12 * V1);
		b23 = Q2 - (m23 * V2);
		b34 = Q3 - (m34 * V3);

		if (vv_lockout < 0.0) {
			gl_warning("volt var control lockout is 0. Warning this may cause oscillating behavior.");
			vv_lockout = 0;
		}
		allowed_vv_action = 0;
		last_vv_check = 0;
	}
	else if (four_quadrant_control_mode == FQM_VOLT_WATT) {
		if (VW_V1 == -2) VW_V1 = 1.05833;
		if (VW_V2 == -2) VW_V2 = 1.10;
		if (VW_P1 == -2) VW_P1 = 1.00;
		if (VW_P2 == -2) VW_P2 = 0.0;
		if (VW_V1 >= VW_V2) {
			gl_error("inverter::init(): VOLT_WATT mode requires VW_V2 > VW_V1.");
			return 0;
		}
		if (VW_P1 <= VW_P2) {
			gl_error("inverter::init(): VOLT_WATT mode requires VW_P1 > VW_P2.");
			return 0;
		}
		if (V_base == 0) {
			gl_error("inverter::init(): The base voltage must be greater than 0 for VOLT_WATT.");
			return 0;
		}
		VW_m = (VW_P2 - VW_P1) / (VW_V2 - VW_V1); // this will be negative, and Plimit = P1 + (v - V1) * m, until it reaches zero

		if (vv_lockout < 0.0) {
			gl_warning("VOLT_WATT uses the VOLT_VAR lockout, which is 0. Warning this may cause oscillating behavior.");
			vv_lockout = 0;
		}
		allowed_vv_action = 0;
		last_vv_check = 0;
		pa_vw_limited = pb_vw_limited = pc_vw_limited = p_rated;
	}

	///////////////////////////////////////////////////////////////////////////
	// DELTA MODE
	///////////////////////////////////////////////////////////////////////////
	//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (!enable_subsecond_models)
		{
			gl_warning("inverter:%s indicates it wants to run deltamode, but the module-level flag is not set!",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The inverter object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			gen_object_count++;	//Increment the counter
			first_sync_delta_enabled = true;
		}

		//Check for frequency 
		if (enable_1547_compliance)
		{
			return_value_init = initalize_IEEE_1547_checks(parent);

			//Check
			if (return_value_init == FAILED)
			{
				GL_THROW("inverter:%d-%s - initalizing the IEEE 1547 checks failed",obj->id,(obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to initialize some of the variables for the inverter IEEE 1547 functionality, an error occurred.
				Please try again.  If the error persists, please submit your code and a bug report via the issues tracker.
				*/
			}
		}
		//Default else - don't do anything
	}//End deltamode inclusive
	else	//This particular model isn't enabled
	{
		if (enable_subsecond_models)
		{
			gl_warning("inverter:%d %s - Deltamode is enabled for the module, but not this inverter!",obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The inverter is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this inverter may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
		else if (enable_1547_compliance)
		{
			//Initalize it
			return_value_init = initalize_IEEE_1547_checks(parent);

			//Check
			if (return_value_init == FAILED)
			{
				GL_THROW("inverter:%d-%s - initalizing the IEEE 1547 checks failed",obj->id,(obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to initialize some of the variables for the inverter IEEE 1547 functionality, an error occurred.
				Please try again.  If the error persists, please submit your code and a bug report via the issues tracker.
				*/
			}

			//Warn, because this probably won't work well
			gl_warning("inverter:%d-%s - IEEE 1547 checks are enabled, but the model is not deltamode-enabled",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The IEEE 1547 checks have been enabled for the inverter object, but deltamode is not enabled.  This will severely hinder the inverter's
			ability to do these checks, and really isn't the intended mode of operation.
			*/
		}
	}

	//Set the timestep strackers
	prev_time = gl_globalclock;
	prev_time_dbl = (double)(prev_time);

	// Record the starting time
	start_time = gl_globalclock;

	// Initialize parameters
	VA_Out = gld::complex(P_Out,Q_Out);
	VA_Out_past = VA_Out;
	//I_In = gld::complex((VA_Out.Mag())/V_In.Mag(),0.0);
	P_Out_t0 = P_Out;
	Q_Out_t0 = Q_Out;
	power_factor_t0 = power_factor;
	I_Out[0] = gld::complex(0);
	I_Out[1] = gld::complex(0);
	I_Out[2] = gld::complex(0);

	//Flagging variable
	inverter_start_time = gl_globalclock;

	return 1;
	
}

TIMESTAMP inverter::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		//Reset
		reset_complex_powerflow_accumulators();

		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	if(inverter_type_v != FOUR_QUADRANT){
		phaseA_I_Out = phaseB_I_Out = phaseC_I_Out = 0.0;
	} else {
		if (pf_reg == INCLUDED)
		{
			if (t1 != t0)
			{
				//See if the "new" timestamp allows us to change
				if (t1>=pf_reg_next_update_time)
				{
					//Above the previous time, so allow a change
					pf_reg_dispatch_change_allowed = true;
				}
				if (pf_reg_activate == -2)
				{
					pf_reg_activate = 0.80;
					gl_warning("inverter:%s - pf_reg_activate undefined, setting to default value of 0.80.",obj->name);
				}
				if (pf_reg_deactivate == -1)
				{
					pf_reg_deactivate = 0.95;
					gl_warning("inverter:%s - pf_reg_deactivate undefined, setting to default value of 0.95.",obj->name);
				}
				if (pf_reg_deactivate >= 0.99)
				{
					gl_warning("inverter:%s - Very high values pf_reg_deactivate (~ 0.99) may lead to inverter control oscillation.",obj->name);
				}
				if (pf_reg_activate > pf_reg_deactivate)
				{
					GL_THROW("inverter:%s - pf_reg_activate is greater than pf_reg_deactivate.",obj->name);
				}
				if (pf_reg_activate < 0 || pf_reg_deactivate < 0)
				{
					GL_THROW("inverter:%s - pf_reg_activate and/or pf_reg_deactivate are negative.",obj->name);
				}
				if (pf_reg_activate == pf_reg_deactivate)
				{
					gl_warning("inverter:%s - pf_reg_activate and pf_reg_deactivate are equal - pf regluation may not behave properly and/or oscillate.",obj->name);;
				}
			}
		}
		else if (pf_reg == INCLUDED_ALT)
		{
			if (t1 != t0)
			{
				//See if the "new" timestamp allows us to change
				if (t1>=pf_reg_next_update_time)
				{
					//Above the previous time, so allow a change
					pf_reg_dispatch_change_allowed = true;
				}

				//Checks for ranges
				if (pf_target_var == -2.0)
				{
					pf_target_var = 0.95;
					gl_warning("inverter:%s - pf_target_var is undefined, setting to a default value of 0.95",obj->name ? obj->name : "Unnamed");
					/*  TROUBLESHOOT
					A value was not specified for pf_pf_target_var.  This has been arbitrarily set to 0.95 lagging (inductive).  If this is not acceptable,
					please specify the desired power factor value.
					*/
				}

				if (pf_reg_high == -2.0)
				{
					pf_reg_high = -0.95;
					gl_warning("inverter:%s - pf_reg_high is undefined, setting to a default value of -0.95",obj->name ? obj->name : "Unnamed");
					/*  TROUBLESHOOT
					A value was not specified for pf_reg_high.  This has been arbitrarily set to 0.95 leading (capacitive).  If this is not acceptable,
					please specify the desired power factor value.
					*/
				}

				if (pf_reg_low == -2.0)
				{
					pf_reg_low = 0.97;
					gl_warning("inverter:%s - pf_reg_low is undefined, setting to a default value of 0.97",obj->name ? obj->name : "Unnamed");
					/*  TROUBLESHOOT
					A value was not specified for pf_reg_low.  This has been arbitrarily set to 0.97 lagging (inductive).  If this is not acceptable,
					please specify the desired power factor value.
					*/
				}

				//Check values based on sign
				if (pf_target_var < 0.0)	//Capacitive power factor specified
				{
					//Make sure target is below the lower limit
					if ((pf_reg_low < 0) && (pf_reg_low > pf_target_var))
					{
						GL_THROW("inverter:%s - pf_reg_low is below the pf_target_var value!",obj->name ? obj->name : "Unnamed");
						/*  TROUBLESHOOT
						For the alternative power factor correction mode, the pf_reg_low value must be a higher power factor than the desired value, in terms of relative power factor.
						*/
					}
				}//end capacitive
				else	//Assume inductive, by elimination
				{
					//Make sure target is below the lower limit
					if ((pf_reg_low > 0) && (pf_reg_low < pf_target_var))
					{
						GL_THROW("inverter:%s - pf_reg_low is below the pf_target_var value!",obj->name ? obj->name : "Unnamed");
						//Defined above
					}

				}//End inductive

				//Generic check - make sure things are in the right order
				//Make sure the upper is in a proper place too
				if (((pf_reg_high > 0) && (pf_reg_low > 0) && (pf_reg_high <= pf_reg_low)) || ((pf_reg_low < 0) && (pf_reg_high < 0) && (pf_reg_high >= pf_reg_low)))
				{
					GL_THROW("inverter:%s - pf_reg_low is higher than pf_reg_high",obj->name ? obj->name : "Unnamed");
					/*  TROUBLESHOOT
					For the alternative power factor correction mode, the pf_reg_low value must be a "more inductive" power factor than the desired value, in terms of
					relative power factor.
					*/
				}
			}//End new time
		}//End pf alt mode

		if(four_quadrant_control_mode == FQM_LOAD_FOLLOWING)
		{
			if (t1 != t0)
			{
				//See if the "new" timestamp allows us to change
				if (t1>=next_update_time)
				{
					//Above the previous time, so allow a change
					lf_dispatch_change_allowed = true;
				}

				//Threshold checks
				if (max_charge_rate <0)
				{
					GL_THROW("inverter:%s - max_charge_rate is negative!",obj->name);
					/*  TROUBLESHOOT
					The max_charge_rate for the inverter is negative.  Please specify
					a valid charge rate for the object to continue.
					*/
				}
				else if (max_charge_rate == 0)
				{
					gl_warning("inverter:%s - max_charge_rate is zero",obj->name);
					/*  TROUBLESHOOT
					The max_charge_rate for the inverter is currently zero.  This will result
					in no charging action by the inverter.  If this is not desired, please specify a valid
					value.
					*/
				}

				if (max_discharge_rate <0)
				{
					GL_THROW("inverter:%s - max_discharge_rate is negative!",obj->name);
					/*  TROUBLESHOOT
					The max_discharge_rate for the inverter is negative.  Please specify
					a valid discharge rate for the object to continue.
					*/
				}
				else if (max_discharge_rate == 0)
				{
					gl_warning("inverter:%s - max_discharge_rate is zero",obj->name);
					/*  TROUBLESHOOT
					The max_discharge_rate for the inverter is currently zero.  This will result
					in no discharging action by the inverter.  If this is not desired, please specify a valid
					value.
					*/
				}

				//Charge thresholds
				if (charge_on_threshold > charge_off_threshold)
				{
					GL_THROW("inverter:%s - charge_on_threshold is greater than charge_off_threshold!",obj->name);
					/*  TROUBLESHOOT
					For proper LOAD_FOLLOWING behavior, charge_on_threshold should be smaller than charge_off_threshold.
					Please correct this and try again.
					*/
				}
				else if (charge_on_threshold == charge_off_threshold)
				{
					gl_warning("inverter:%s - charge_on_threshold and charge_off_threshold are equal - may not behave properly!",obj->name);
					/*  TROUBLESHOOT
					For proper LOAD_FOLLOWING operation, charge_on_threshold and charge_off_threshold should specify a deadband for operation.
					If equal, the inverter may not operate properly and the system may never solve properly.
					*/
				}

				//Discharge thresholds
				if (discharge_on_threshold < discharge_off_threshold)
				{
					GL_THROW("inverter:%s - discharge_on_threshold is less than discharge_off_threshold!",obj->name);
					/*  TROUBLESHOOT
					For proper LOAD_FOLLOWING behavior, discharge_on_threshold should be larger than discharge_off_threshold.
					Please correct this and try again.
					*/
				}
				else if (discharge_on_threshold == discharge_off_threshold)
				{
					gl_warning("inverter:%s - discharge_on_threshold and discharge_off_threshold are equal - may not behave properly!",obj->name);
					/*  TROUBLESHOOT
					For proper LOAD_FOLLOWING operation, discharge_on_threshold and discharge_off_threshold should specify a deadband for operation.
					If equal, the inverter may not operate properly and the system may never solve properly.
					*/
				}

				//Combination of the two
				if (discharge_off_threshold <= charge_off_threshold)
				{
					gl_warning("inverter:%s - discharge_off_threshold should be larger than the charge_off_threshold",obj->name);
					/*  TROUBLESHOOT
					For proper LOAD_FOLLOWING operation, the deadband for the inverter should not overlap.  Please specify a larger
					range for the discharge and charge bands of operation and try again.  If the bands do overlap, unexpected behavior may occur.
					*/
				}
			}
		} //End LOAD_FOLLOWING checks
		else if ((four_quadrant_control_mode == FQM_VOLT_VAR) || (four_quadrant_control_mode == FQM_VOLT_WATT))
		{
			if ((phases & 0x10) == 0x10)	//Triplex
			{
				value_Power12 = -last_power[3];	//Theoretically pPower is mapped to power_12, which already has the [2] offset applied
			}
			else	//Variation of three-phase
			{
				value_Power[0] = -last_power[0];
				value_Power[1] = -last_power[1];
				value_Power[2] = -last_power[2];
			}

			//Sync the values, if needed
			if (parent_is_a_meter)
			{
				push_complex_powerflow_values();
			}

		}//End VOLT_VAR or VOLT_WATT
		else if(four_quadrant_control_mode == FQM_GROUP_LF)
		{
			if (t1 != t0)
			{
				//See if the "new" timestamp allows us to change
				if (t1>=next_update_time)
				{
					//Above the previous time, so allow a change
					lf_dispatch_change_allowed = true;
				}

				//Threshold checks
				if (group_max_charge_rate < 0)
				{
					GL_THROW("inverter:%s - group_max_charge_rate cannot be negative.",obj->name);
				}
				else if (max_charge_rate == 0)
				{
					gl_warning("inverter:%s - group_max_charge_rate is zero.",obj->name);
				}

				if (group_max_discharge_rate < 0)
				{
					GL_THROW("inverter:%s - group_max_discharge_rate cannot be negative.",obj->name);
				}
				else if (group_max_discharge_rate == 0)
				{
					gl_warning("inverter:%s - group_max_discharge_rate is zero",obj->name);
				}

				if (group_rated_power <= 0)
				{
					GL_THROW("inverter:%s - group_rated_power must be positive.",obj->name);
				}


				//Charge thresholds
				if (charge_threshold == -1)
				{
					GL_THROW("inverter:%s - charge_threshold must be defined for GROUP_LOAD_FOLLOW mode.",obj->name);
				}

				if (discharge_threshold == -1)
				{
					GL_THROW("inverter:%s - discharge_threshold must be defined for GROUP_LOAD_FOLLOW mode.",obj->name);
				}

				if (charge_threshold == discharge_threshold)
				{
					gl_warning("inverter:%s - charge_threshold and discharge_threshold are equal - not recommended, oscillations may occur.",obj->name);
				}


				//Combination of the two
				if (discharge_threshold < charge_threshold)
				{
					gl_error("inverter:%s - discharge_threshold must be larger than the charge_threshold",obj->name);
				}
			} //t1 != t0
		}// End FQM_GROUP_LF
		
		if(deltamode_inclusive && enable_subsecond_models && (inverter_dyn_mode == PI_CONTROLLER)) {
			// Only execute at the first time step of simulation, or the first ieration of the next time steps
			if ((t1 == start_time) || (t1 != t0)) {
				last_I_In = I_In;

				//Determine phasing to check
				if ((phases & 0x10) == 0x10)	//Triplex
				{
					last_I_Out[0] = I_Out[0];
				}
				else	//Three-phase or some variant
				{
					for(int i = 0; i < 3; i++) {
						last_I_Out[i] = I_Out[i];
					}
				}
			}
		}
	}
		
	return t2; 
}

TIMESTAMP inverter::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret_value;
	double curr_ts_dbl, diff_dbl;
	double ieee_1547_return_value;
	TIMESTAMP new_ret_value;
	FUNCTIONADDR test_fxn;
	bool *gen_dynamic_flag = nullptr;
	STATUS fxn_return_status;
	char tmp_index_val;

	gld::complex rotate_value;
	gld::complex calculated_iO[3];

	gld::complex temp_power_val[3];

	gld::complex temp_complex_value;
	gld_wlock *test_rlock = nullptr;

	//Assume always want TS_NEVER
	tret_value = TS_NEVER;

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		//Reset
		reset_complex_powerflow_accumulators();

		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	if(gen_status_v == OFFLINE){
		power_val[0] = gld::complex(0.0,0.0);
		power_val[1] = gld::complex(0.0,0.0);
		power_val[2] = gld::complex(0.0,0.0);
		P_Out = 0;
		Q_Out = 0;
		VA_Out = gld::complex(0);
		if ((phases & 0x10) == 0x10) {
			last_power[3] = -power_val[0];
			value_Power12 = last_power[3];
		} else {
			p_in = 0;
			if ((phases & 0x01) == 0x01) {
				last_power[0] = -power_val[0];
				value_Power[0] = last_power[0];
			}
			if ((phases & 0x02) == 0x02) {
				last_power[1] = -power_val[1];
				value_Power[1] = last_power[1];
			}
			if ((phases & 0x04) == 0x04) {
				last_power[2] = -power_val[2];
				value_Power[2] = last_power[2];
			}
		}

		//Sync the powerflow variables
		if (parent_is_a_meter)
		{
			push_complex_powerflow_values();
		}

		return tret_value;
	}

	if (first_sync_delta_enabled)	//Deltamode first pass
	{
		//TODO: LOCKING!
		if (deltamode_inclusive && enable_subsecond_models)	//We want deltamode - see if it's populated yet
		{
			//Very first run
			if (first_iter_counter == 0)
			{
				if (((gen_object_current == -1) || (delta_objects==nullptr)) && enable_subsecond_models)
				{
					//Call the allocation routine
					allocate_deltamode_arrays();
				}

				//Check limits of the array
				if (gen_object_current>=gen_object_count)
				{
					GL_THROW("Too many objects tried to populate deltamode objects array in the generators module!");
					/*  TROUBLESHOOT
					While attempting to populate a reference array of deltamode-enabled objects for the generator
					module, an attempt was made to write beyond the allocated array space.  Please try again.  If the
					error persists, please submit a bug report and your code via the trac website.
					*/
				}

				//Add us into the list
				delta_objects[gen_object_current] = obj;

				//Map up the function for interupdate
				delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"interupdate_gen_object"));

				//Make sure it worked
				if (delta_functions[gen_object_current] == nullptr)
				{
					GL_THROW("Failure to map deltamode function for device:%s",obj->name);
					/*  TROUBLESHOOT
					Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
					the object supports deltamode.  If the error persists, please submit your code and a bug report via the
					trac website.
					*/
				}

				//Map up the function for postupdate
				post_delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"postupdate_gen_object"));

				//Make sure it worked
				if (post_delta_functions[gen_object_current] == nullptr)
				{
					GL_THROW("Failure to map post-deltamode function for device:%s",obj->name);
					/*  TROUBLESHOOT
					Attempts to map up the postupdate function of a specific device failed.  Please try again and ensure
					the object supports deltamode.  If the error persists, please submit your code and a bug report via the
					trac website.
					*/
				}

				//Map up the function for postupdate
				delta_preupdate_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"preupdate_gen_object"));
				
				//Make sure it worked
				if (delta_preupdate_functions[gen_object_current] == nullptr)
				{
					GL_THROW("Failure to map pre-deltamode function for device:%s",obj->name);
					/*  TROUBLESHOOT
					Attempts to map up the preupdate function of a specific device failed.  Please try again and ensure
					the object supports deltamode.  If the error persists, please submit your code and a bug report via the
					trac website.
					*/
				}

				//Update pointer
				gen_object_current++;

				// PQ_CONSTANT inverter mapping for powerflow iteration of slew rate limitation
				//Initialize some extra variables for PQ_CONSTANT inverters
				if (four_quadrant_control_mode == FQM_CONSTANT_PQ)
				{
					//Map the current injection function
					test_fxn = (FUNCTIONADDR)(gl_get_function(obj->parent,"pwr_current_injection_update_map"));

					//See if it was located
					if (test_fxn == nullptr)
					{
						GL_THROW("PQ_CONSTANT inverter:%s - failed to map additional current injection mapping for node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
						/*  TROUBLESHOOT
						While attempting to map the additional current injection function, an error was encountered.
						Please try again.  If the error persists, please submit your code and a bug report via the trac website.
						*/
					}

					//Call the mapping function
					fxn_return_status = ((STATUS (*)(OBJECT *, OBJECT *))(*test_fxn))(obj->parent,obj);

					//Make sure it worked
					if (fxn_return_status != SUCCESS)
					{
						GL_THROW("PQ_CONSTANT inverter:%s - failed to map additional current injection mapping for node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
						//Defined above
					}
				}//End PQ_CONSTANT inverter special initialization items

				//Voltage source inverter mapping
				if (four_quadrant_control_mode == FQM_VSI)
				{
					//See if we're attached to a node-esque object
					if (obj->parent != nullptr)
					{
						if (gl_object_isa(obj->parent,"meter","powerflow") ||
						gl_object_isa(obj->parent,"load","powerflow") ||
						gl_object_isa(obj->parent,"node","powerflow") ||
						gl_object_isa(obj->parent,"elec_frequency","powerflow"))
						{
							//Accumulate the starting power
							temp_complex_value = gld::complex(P_Out, Q_Out);

							//Push it up
							pGenerated->setp<gld::complex>(temp_complex_value,*test_rlock);

							//Map the current injection function
							test_fxn = (FUNCTIONADDR)(gl_get_function(obj->parent,"pwr_current_injection_update_map"));

							//See if it was located
							if (test_fxn == nullptr)
							{
								GL_THROW("Voltage source inverter:%s - failed to map additional current injection mapping for node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
								/*  TROUBLESHOOT
								While attempting to map the additional current injection function, an error was encountered.
								Please try again.  If the error persists, please submit your code and a bug report via the trac website.
								*/
							}

							//Call the mapping function
							fxn_return_status = ((STATUS (*)(OBJECT *, OBJECT *))(*test_fxn))(obj->parent,obj);

							//Make sure it worked
							if (fxn_return_status != SUCCESS)
							{
								GL_THROW("Voltage source inverter:%s - failed to map additional current injection mapping for node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
								//Defined above
							}
						}//End parent is a node object
						else	//Nope, so who knows what is going on - better fail, just to be safe
						{
							GL_THROW("Voltage source inverter:%s - invalid parent object:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
							/*  TROUBLESHOOT
							At this time, for proper dynamic functionality an inverter object must be parented to a three-phase powerflow node
							object (node, load, meter).  The parent object is not one of those objects.
							*/
						}
					}//End non-null parent
				}//End deltamode and VSI mode
			}//End first iteration

			//General counter to force an additional reiteration - help converge the current values
			first_iter_counter++;

			//Determine our path forward - two iterations seems to work (this probably needs to be revisited)
			if (first_iter_counter < 3)
			{
				//Reiterate
				tret_value = t1;
			}
			else
			{
				//Force us to reiterate once, just to make sure things get closer with the solvers
				first_sync_delta_enabled = false;
			}
		}//End deltamode specials - first pass
		else	//Somehow, we got here and deltamode isn't properly enabled...odd, just deflag us
		{
			first_sync_delta_enabled = false;
		}
	}//End first delta timestep
	//default else - either not deltamode, or not the first timestep

	//Other models (QSTS - first step check) - force a reiteration
	if (!deltamode_inclusive && inverter_first_step)
	{
		//General counter to force an additional reiteration - help converge the current values
		first_iter_counter++;

		//Determine our path forward - two iterations seems to work (this probably needs to be revisited)
		if (first_iter_counter < 3)
		{
			//Reiterate
			tret_value = t1;
		}
		//Default else - let it do what it wanted - likely TS_NEVER
	}

	//Flag to check first steps - for current injection initialization
	if (inverter_start_time != t1)
	{
		inverter_first_step = false;
	}

	//Perform 1547 checks, if appropriate
	if (enable_1547_compliance)
	{
		//Extract the current timestamp, as a double
		curr_ts_dbl = (double)gl_globalclock;

		//See if we're a new timestep, otherwise, we don't care
		if (prev_time_dbl < curr_ts_dbl)
		{
			//Figure out how far we moved forward
			diff_dbl = curr_ts_dbl - prev_time_dbl;

			//Update the value
			prev_time_dbl = curr_ts_dbl;

			//Do the checks
			ieee_1547_return_value = perform_1547_checks(diff_dbl);

			//Check it
			if (ieee_1547_return_value > 0.0)
			{
				//See which mode we're in
				if (deltamode_inclusive)
				{
					new_ret_value = t1 + (TIMESTAMP)(floor(ieee_1547_return_value));

					//Regardless of the return, schedule us for a delta transition - if it clears by then, we should
					//hop right back out
					schedule_deltamode_start(new_ret_value);
				}
				else	//Steady state
				{
					new_ret_value = t1 + (TIMESTAMP)(ceil(ieee_1547_return_value));
				}

				//See if it is sooner than our existing return
				if ((tret_value != TS_NEVER) && (new_ret_value < tret_value))
				{
					tret_value = new_ret_value;
				}
				//Default else -- existing return was sufficient
			}
		}
		//Default else -- same timestep, so don't care
	}
	//Default else - 1547 checks are not enabled
	
	if ((value_MeterStatus==1) && inverter_1547_status)	//Make sure the meter is in service
	{
		phaseA_V_Out = value_Circuit_V[0];	//Syncs the meter parent to the generator.
		phaseB_V_Out = value_Circuit_V[1];
		phaseC_V_Out = value_Circuit_V[2];

		if(inverter_type_v != FOUR_QUADRANT)
		{
			switch(gen_mode_v)
			{
				case CONSTANT_PF:
					P_In = V_In * I_In; //DC

					// need to differentiate between different pulses...

					//Note: if WT_is_connected is true, the VA_Out is written directly by wind turbine
					if (!WT_is_connected){
						if(!use_multipoint_efficiency){
							VA_Out = P_In * efficiency;
						} else {
							if(P_In <= p_so){
								VA_Out = 0;
							} else {
								if(V_In > v_dco){
									gl_warning("The dc voltage is greater than the specified maximum for the inverter. Efficiency model may be inaccurate.");
								}
								C1 = p_dco*(1+c_1*(V_In-v_dco));
								C2 = p_so*(1+c_2*(V_In-v_dco));
								C3 = c_o*(1+c_3*(V_In-v_dco));
								VA_Out.SetReal((((p_max/(C1-C2))-C3*(C1-C2))*(P_In-C2)+C3*(P_In-C2)*(P_In-C2)));
							}
						}
					}

					if ((phases & 0x10) == 0x10)  //Triplex-line -> Assume it's only across the 240 V for now.
					{
						power_val[0] = gld::complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)));
						if (phaseA_V_Out.Mag() != 0.0)
							phaseA_I_Out = ~(power_val[0] / phaseA_V_Out);
						else
							phaseA_I_Out = gld::complex(0.0,0.0);

						value_Line12 = -phaseA_I_Out;

						//Update this value for later removal
						last_current[3] = -phaseA_I_Out;
						
						//Get rid of these for now
						//gld::complex phaseA_V_Internal = filter_voltage_impact_source(phaseA_I_Out, phaseA_V_Out);
						//phaseA_I_Out = filter_current_impact_out(phaseA_I_Out, phaseA_V_Internal);
					}
					else if (number_of_phases_out == 3) // All three phases
					{
						power_val[0] = power_val[1] = power_val[2] = gld::complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)))/3;
						if (phaseA_V_Out.Mag() != 0.0)
							phaseA_I_Out = ~(power_val[0] / phaseA_V_Out); // /sqrt(2.0);
						else
							phaseA_I_Out = gld::complex(0.0,0.0);
						if (phaseB_V_Out.Mag() != 0.0)
							phaseB_I_Out = ~(power_val[1] / phaseB_V_Out); // /sqrt(2.0);
						else
							phaseB_I_Out = gld::complex(0.0,0.0);
						if (phaseC_V_Out.Mag() != 0.0)
							phaseC_I_Out = ~(power_val[2] / phaseC_V_Out); // /sqrt(2.0);
						else
							phaseC_I_Out = gld::complex(0.0,0.0);

						value_Line_I[0] = -phaseA_I_Out;
						value_Line_I[1] = -phaseB_I_Out;
						value_Line_I[2] = -phaseC_I_Out;

						//Update this value for later removal
						last_current[0] = -phaseA_I_Out;
						last_current[1] = -phaseB_I_Out;
						last_current[2] = -phaseC_I_Out;
					}
					else if(number_of_phases_out == 2) // two-phase connection
					{
						OBJECT *obj = OBJECTHDR(this);

						if ( ((phases & 0x01) == 0x01) && phaseA_V_Out.Mag() != 0)
						{
							power_val[0] = gld::complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)))/2;;
							phaseA_I_Out = ~(power_val[0] / phaseA_V_Out);
						}
						else 
							phaseA_I_Out = gld::complex(0,0);

						if ( ((phases & 0x02) == 0x02) && phaseB_V_Out.Mag() != 0)
						{
							power_val[1] = gld::complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)))/2;;
							phaseB_I_Out = ~(power_val[1] / phaseB_V_Out);
						}
						else 
							phaseB_I_Out = gld::complex(0,0);

						if ( ((phases & 0x04) == 0x04) && phaseC_V_Out.Mag() != 0)
						{
							power_val[2] = gld::complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)))/2;;
							phaseC_I_Out = ~(power_val[2] / phaseC_V_Out);
						}
						else 
							phaseC_I_Out = gld::complex(0,0);

						value_Line_I[0] = -phaseA_I_Out;
						value_Line_I[1] = -phaseB_I_Out;
						value_Line_I[2] = -phaseC_I_Out;

						//Update this value for later removal
						last_current[0] = -phaseA_I_Out;
						last_current[1] = -phaseB_I_Out;
						last_current[2] = -phaseC_I_Out;

					}
					else // Single phase connection
					{
						if( ((phases & 0x01) == 0x01) && phaseA_V_Out.Mag() != 0)
						{
							power_val[0] = gld::complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)));
							phaseA_I_Out = ~(power_val[0] / phaseA_V_Out); 
							//complex phaseA_V_Internal = filter_voltage_impact_source(phaseA_I_Out, phaseA_V_Out);
							//phaseA_I_Out = filter_current_impact_out(phaseA_I_Out, phaseA_V_Internal);
						}
						else if( ((phases & 0x02) == 0x02) && phaseB_V_Out.Mag() != 0)
						{
							power_val[1] = gld::complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)));
							phaseB_I_Out = ~(power_val[1] / phaseB_V_Out); 
							//complex phaseB_V_Internal = filter_voltage_impact_source(phaseB_I_Out, phaseB_V_Out);
							//phaseB_I_Out = filter_current_impact_out(phaseB_I_Out, phaseB_V_Internal);
						}
						else if( ((phases & 0x04) == 0x04) && phaseC_V_Out.Mag() != 0)
						{
							power_val[2] = gld::complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)));
							phaseC_I_Out = ~(power_val[2] / phaseC_V_Out); 
							//gld::complex phaseC_V_Internal = filter_voltage_impact_source(phaseC_I_Out, phaseC_V_Out);
							//phaseC_I_Out = filter_current_impact_out(phaseC_I_Out, phaseC_V_Internal);
						}
						else
						{
							gl_warning("None of the phases specified have voltages!");
							phaseA_I_Out = phaseB_I_Out = phaseC_I_Out = gld::complex(0.0,0.0);
						}
						value_Line_I[0] = -phaseA_I_Out;
						value_Line_I[1] = -phaseB_I_Out;
						value_Line_I[2] = -phaseC_I_Out;

						//Update this value for later removal
						last_current[0] = -phaseA_I_Out;
						last_current[1] = -phaseB_I_Out;
						last_current[2] = -phaseC_I_Out;

					}
					break;
				case CONSTANT_PQ:
					GL_THROW("Constant PQ mode not supported at this time");
					/* TROUBLESHOOT
					This will be worked on at a later date and is not yet correctly implemented.
					*/
					gl_verbose("inverter sync: constant pq");

					if(parent_is_a_meter)
					{
						VA_Out = gld::complex(P_Out,Q_Out);
					}
					else
					{
						//These are the static values, so they don't need a separate pull
						phaseA_I_Out = value_Line_I[0];
						phaseB_I_Out = value_Line_I[1];
						phaseC_I_Out = value_Line_I[2];

						//Erm, there's no good way to handle this from a "multiply attached" point of view.
						//TODO: Think about how to do this if the need arrises

						VA_Out = phaseA_V_Out * (~ phaseA_I_Out) + phaseB_V_Out * (~ phaseB_I_Out) + phaseC_V_Out * (~ phaseC_I_Out);
					}

					if ( (phases & 0x07) == 0x07) // Three phase
					{
						power_val[0] = power_val[1] = power_val[2] = VA_Out /3;
						phaseA_I_Out = (power_val[0] / phaseA_V_Out); // /sqrt(2.0);
						phaseB_I_Out = (power_val[1] / phaseB_V_Out); // /sqrt(2.0);
						phaseC_I_Out = (power_val[2] / phaseC_V_Out); // /sqrt(2.0);

						phaseA_I_Out = ~ phaseA_I_Out;
						phaseB_I_Out = ~ phaseB_I_Out;
						phaseC_I_Out = ~ phaseC_I_Out;

					}
					else if ( (number_of_phases_out == 1) && ((phases & 0x01) == 0x01) ) // Phase A only
					{
						power_val[0] = VA_Out;
						phaseA_I_Out = (power_val[0] / phaseA_V_Out); // /sqrt(2);
						phaseA_I_Out = ~ phaseA_I_Out;
					}
					else if ( (number_of_phases_out == 1) && ((phases & 0x02) == 0x02) ) // Phase B only
					{
						power_val[1] = VA_Out;
						phaseB_I_Out = (power_val[1] / phaseB_V_Out);  // /sqrt(2);
						phaseB_I_Out = ~ phaseB_I_Out;
					}
					else if ( (number_of_phases_out == 1) && ((phases & 0x04) == 0x04) ) // Phase C only
					{
						power_val[2] = VA_Out;
						phaseC_I_Out = (power_val[2] / phaseC_V_Out); // /sqrt(2);
						phaseC_I_Out = ~ phaseC_I_Out;
					}
					else if ( (number_of_phases_out == 1) && ((phases & 0x10) == 0x10))	//Triplex
					{
						power_val[0] = VA_Out;
						phaseA_I_Out = (power_val[0] / phaseA_V_Out); // /sqrt(2);
						phaseA_I_Out = ~ phaseA_I_Out;

						//Zero the others in here, just for paranoia reasons
						phaseB_I_Out = complex(0.0,0.0);
						phaseC_I_Out = complex(0.0,0.0);
					}
					else
					{
						throw ("unsupported number of phases");
					}

					P_In = VA_Out.Re() / efficiency;

					V_In = Vdc;

					I_In = P_In / V_In;

					gl_verbose("Inverter sync: V_In asked for by inverter is: %f", V_In);
					gl_verbose("Inverter sync: I_In asked for by inverter is: %f", I_In);


					value_Line_I[0] = phaseA_I_Out;
					value_Line_I[1] = phaseB_I_Out;
					value_Line_I[2] = phaseC_I_Out;

					//Update this value for later removal
					last_current[0] = phaseA_I_Out;
					last_current[1] = phaseB_I_Out;
					last_current[2] = phaseC_I_Out;

					break;
				case CONSTANT_V:
				{
					GL_THROW("Constant V mode not supported at this time");
					/* TROUBLESHOOT
					This will be worked on at a later date and is not yet correctly implemented.
					*/
					gl_verbose("inverter sync: constant v");
					bool changed = false;
					
					if ((phases & 0x10) == 0x10)	//Triplex
					{
						//Use phaseA varieties
						if (phaseA_V_Out.Re() < (V_Set_A - margin))
						{
							phaseA_I_Out = phaseA_I_Out_prev + I_step_max/2;
							changed = true;
						}
						else if (phaseA_V_Out.Re() > (V_Set_A + margin))
						{
							phaseA_I_Out = phaseA_I_Out_prev - I_step_max/2;
							changed = true;
						}
						else
						{
							changed = false;
						}

						//Zero the other currents, just because
						phaseB_I_Out = complex(0.0,0.0);
						phaseC_I_Out = complex(0.0,0.0);
					}
					else	//Not triplex
					{
						if((phases & 0x01) == 0x01)
						{
							if (phaseA_V_Out.Re() < (V_Set_A - margin))
							{
								phaseA_I_Out = phaseA_I_Out_prev + I_step_max/2;
								changed = true;
							}
							else if (phaseA_V_Out.Re() > (V_Set_A + margin))
							{
								phaseA_I_Out = phaseA_I_Out_prev - I_step_max/2;
								changed = true;
							}
							else
							{
								changed = false;
							}
						}
						if ((phases & 0x02) == 0x02)
						{
							if (phaseB_V_Out.Re() < (V_Set_B - margin))
							{
								phaseB_I_Out = phaseB_I_Out_prev + I_step_max/2;
								changed = true;
							}
							else if (phaseB_V_Out.Re() > (V_Set_B + margin))
							{
								phaseB_I_Out = phaseB_I_Out_prev - I_step_max/2;
								changed = true;
							}
							else
							{
								changed = false;
							}
						}
						if ((phases & 0x04) == 0x04)
						{
							if (phaseC_V_Out.Re() < (V_Set_C - margin))
							{
								phaseC_I_Out = phaseC_I_Out_prev + I_step_max/2;
								changed = true;
							}
							else if (phaseC_V_Out.Re() > (V_Set_C + margin))
							{
								phaseC_I_Out = phaseC_I_Out_prev - I_step_max/2;
								changed = true;
							}
							else
							{
								changed = false;
							}
						}
					}
					
					power_val[0] = (~phaseA_I_Out) * phaseA_V_Out;
					power_val[1] = (~phaseB_I_Out) * phaseB_V_Out;
					power_val[2] = (~phaseC_I_Out) * phaseC_V_Out;

					//check if inverter is overloaded -- if so, cap at max power
					if (((power_val[0] + power_val[1] + power_val[2]) > Rated_kVA) ||
						((power_val[0].Re() + power_val[1].Re() + power_val[2].Re()) > Max_P) ||
						((power_val[0].Im() + power_val[1].Im() + power_val[2].Im()) > Max_Q))
					{
						VA_Out = Rated_kVA / number_of_phases_out;
						//if it's maxed out, don't ask for the simulator to re-call
						changed = false;

						//See if triplex (just because)
						if ((phases & 0x10) == 0x10)
						{
							//Phase A duplication
							phaseA_I_Out = VA_Out / phaseA_V_Out;
							phaseA_I_Out = (~phaseA_I_Out);

							//Others should still be zeroed
						}
						else	//Nope - "normal"
						{
							if((phases & 0x01) == 0x01)
							{
								phaseA_I_Out = VA_Out / phaseA_V_Out;
								phaseA_I_Out = (~phaseA_I_Out);
							}
							if((phases & 0x02) == 0x02)
							{
								phaseB_I_Out = VA_Out / phaseB_V_Out;
								phaseB_I_Out = (~phaseB_I_Out);
							}
							if((phases & 0x04) == 0x04)
							{
								phaseC_I_Out = VA_Out / phaseC_V_Out;
								phaseC_I_Out = (~phaseC_I_Out);
							}
						}
					}
					
					//check if power is negative for some reason, should never be
					if(power_val[0] < 0)
					{
						power_val[0] = 0;
						phaseA_I_Out = 0;
						throw("phaseA power is negative!");
					}
					if(power_val[1] < 0)
					{
						power_val[1] = 0;
						phaseB_I_Out = 0;
						throw("phaseB power is negative!");
					}
					if(power_val[2] < 0)
					{
						power_val[2] = 0;
						phaseC_I_Out = 0;
						throw("phaseC power is negative!");
					}

					P_In = VA_Out.Re() / efficiency;

					V_In = Vdc;

					I_In = (P_In / V_In);
					
					gl_verbose("Inverter sync: I_In asked for by inverter is: %f", I_In);

					//TODO: check P and Q components to see if within bounds

					if(changed)
					{
						//Triplex check - post it to the right place
						if ((phases & 0x10) == 0x10)
						{
							value_Line12 = phaseA_I_Out;
							last_current[3] = phaseA_I_Out;
						}
						else	//Three-phase
						{
							value_Line_I[0] = phaseA_I_Out;
							value_Line_I[1] = phaseB_I_Out;
							value_Line_I[2] = phaseC_I_Out;

							//Update this value for later removal
							last_current[0] = phaseA_I_Out;
							last_current[1] = phaseB_I_Out;
							last_current[2] = phaseC_I_Out;
						}

						//TODO: Why does this appear to have a hard-coded default update of 10 minutes?
						TIMESTAMP t2 = t1 + 10 * 60 * TS_SECOND;

						if (tret_value != TS_NEVER)
						{
							if (t2 < tret_value)
							{
								tret_value = t2;
							}
						}
						else
						{
							tret_value = t2;
						}
					}
					else
					{
						//Triplex check
						if ((phases & 0x10) == 0x10)
						{
							//Post
							value_Line12 = phaseA_I_Out;

							//Update for removal
							last_current[3] = phaseA_I_Out;
						}
						else	//Not triplexy
						{
							value_Line_I[0] = phaseA_I_Out;
							value_Line_I[1] = phaseB_I_Out;
							value_Line_I[2] = phaseC_I_Out;

							//Update this value for later removal
							last_current[0] = phaseA_I_Out;
							last_current[1] = phaseB_I_Out;
							last_current[2] = phaseC_I_Out;
						}
					}
					break;
				}
				case SUPPLY_DRIVEN: 
					GL_THROW("SUPPLY_DRIVEN mode for inverters not supported at this time");
					break;
				default:
					//Triplex check
					if ((phases & 0x10) == 0x10)
					{
						value_Line12 = phaseA_I_Out;

						//Removal value
						last_current[3] = phaseA_I_Out;
					}
					else	//Three-phase or some-sort
					{
						value_Line_I[0] = phaseA_I_Out;
						value_Line_I[1] = phaseB_I_Out;
						value_Line_I[2] = phaseC_I_Out;

						//Update this value for later removal
						last_current[0] = phaseA_I_Out;
						last_current[1] = phaseB_I_Out;
						last_current[2] = phaseC_I_Out;
					}

					break;
			}
		} 
		else	//FOUR_QUADRANT code
		{
			//FOUR_QUADRANT model (originally written for NAS/CES, altered for PV)
			double VA_Efficiency, temp_PF, temp_QVal, net_eff; //Ab added last two
			gld::complex temp_VA;
			gld::complex battery_power_out = gld::complex(0,0);
			if ((four_quadrant_control_mode != FQM_VOLT_VAR) && (four_quadrant_control_mode != FQM_VOLT_WATT))
			{
				//Compute power in
				P_In = V_In * I_In;

				//Compute the power contribution of the battery object
				if((phases & 0x10) == 0x10){ // split phase
					battery_power_out = power_val[0];
				} else { // three phase
					if((phases & 0x01) == 0x01){ // has phase A
						battery_power_out += power_val[0];
					}
					if((phases & 0x02) == 0x02){ // hase phase B
						battery_power_out += power_val[1];
					}
					if((phases & 0x04) == 0x04){ // has phase C
						battery_power_out += power_val[2];
					}
				}
				//Determine how to efficiency weight it
				if(!use_multipoint_efficiency)
				{
					//Normal scaling
					VA_Efficiency = P_In * efficiency;
					//Ab add
					net_eff = efficiency;
					//end Ab add
				}
				else
				{
					//See if above minimum DC power input
					if(P_In <= p_so)
					{
						VA_Efficiency = 0.0;	//Nope, no output
						//Ab add
						P_In = 0;
						net_eff = 0;
						//end Ab add
					}
					else	//Yes, apply effiency change
					{
						//Make sure voltage isn't too low
						if(fabs(V_In) > v_dco)
						{
							gl_warning("The dc voltage is greater than the specified maximum for the inverter. Efficiency model may be inaccurate.");
							/*  TROUBLESHOOT
							The DC voltage at the input to the inverter is less than the maximum voltage supported by the inverter.  As a result, the
							multipoint efficiency model may not provide a proper result.
							*/
						}

						//Compute coefficients for multipoint efficiency
						C1 = p_dco*(1+c_1*(V_In-v_dco));
						C2 = p_so*(1+c_2*(V_In-v_dco));
						C3 = c_o*(1+c_3*(V_In-v_dco));

						//Apply this to the output
						VA_Efficiency = (((p_max/(C1-C2))-C3*(C1-C2))*(P_In-C2)+C3*(P_In-C2)*(P_In-C2));
						//Ab add
						net_eff = fabs(VA_Efficiency / P_In);
						//end Ab add
					}
				}
				VA_Efficiency += battery_power_out.Mag();
			} else { // == FQM_VOLT_VAR || == FQM_VOLT_WATT
				if (four_quadrant_control_mode == FQM_VOLT_VAR) { // revalidating the parameters, we are in ::sync here, but maybe they changed?
					if (V1 == -2) V1 = 0.97;
					if (V2 == -2) V2 = 0.99;
					if (V3 == -2) V3 = 1.01;
					if (V4 == -2) V4 = 1.03;
					if (Q1 == -2) Q1 = 0.50;
					if (Q2 == -2) Q2 = 0.0;
					if (Q3 == -2) Q3 = 0.0;
					if (Q4 == -2) Q4 = -0.50;
					if (V1 > V2 || V2 > V3 || V3 > V4) {
						gl_error("inverter::sync(): The curve was not constructed properly. V1 <= V2 <= V3 <= V4 must be true.");
						return TS_INVALID;
					}
					if (Q1 < Q2 || Q2 < Q3 || Q3 < Q4) {
						gl_error("inverter::sync(): The curve was not constructed properly. V1 <= V2 <= V3 <= V4 must be true.");
						return TS_INVALID;
					}
					if (V_base == 0) {
						gl_error("inverter::sync(): The curve was not constructed properly. V1 <= V2 <= V3 <= V4 must be true.");
						return TS_INVALID;
					}
					if (V2 != V1) {
						m12 = (Q2 - Q1) / (V2 - V1);
					} else {
						m12 = 0;
					}
					if (V3 != V2) {
						m23 = (Q3 - Q2) / (V3 - V2);
					} else {
						m23 = 0;
					}
					if (V4 != V3) {
						m34 = (Q4 - Q3) / (V4 - V3);
					} else {
						m34 = 0;
					}
					b12 = Q1 - (m12 * V1);
					b23 = Q2 - (m23 * V2);
					b34 = Q3 - (m34 * V3);
				} else if (four_quadrant_control_mode == FQM_VOLT_WATT) {
//					cout << "line 2694" << endl;
					if (VW_V1 == -2) VW_V1 = 1.05833;
					if (VW_V2 == -2) VW_V2 = 1.10;
					if (VW_P1 == -2) VW_P1 = 1.00;
					if (VW_P2 == -2) VW_P2 = 0.0;
					if (VW_V1 >= VW_V2) {
						gl_error("inverter::sync(): VOLT_WATT mode requires VW_V2 > VW_V1.");
						return TS_INVALID;
					}
					if (VW_P1 <= VW_P2) {
						gl_error("inverter::sync(): VOLT_WATT mode requires VW_P1 > VW_P2.");
						return TS_INVALID;
					}
					if (V_base == 0) {
						gl_error("inverter::sync(): The base voltage must be greater than 0 for VOLT_WATT.");
						return TS_INVALID;
					}
					VW_m = (VW_P2 - VW_P1) / (VW_V2 - VW_V1);
				}

				//Compute power in
				P_In = V_In * I_In;
				if (!WT_is_connected){
					//Determine how to efficiency weight it
					if(!use_multipoint_efficiency)
					{
						//Normal scaling
						VA_Efficiency = P_In * efficiency;
					}
					else
					{
						//See if above minimum DC power input
						if(P_In <= p_so)
						{
							VA_Efficiency = 0.0;	//Nope, no output
						}
						else	//Yes, apply effiency change
						{
							//Make sure voltage isn't too low
							if(fabs(V_In) > v_dco)
							{
								gl_warning("The dc voltage is greater than the specified maximum for the inverter. Efficiency model may be inaccurate.");
								/*  TROUBLESHOOT
								The DC voltage at the input to the inverter is less than the maximum voltage supported by the inverter.  As a result, the
								multipoint efficiency model may not provide a proper result.
								*/
							}

							//Compute coefficients for multipoint efficiency
							C1 = p_dco*(1+c_1*(V_In-v_dco));
							C2 = p_so*(1+c_2*(V_In-v_dco));
							C3 = c_o*(1+c_3*(V_In-v_dco));

							//Apply this to the output
							VA_Efficiency = (((p_max/(C1-C2))-C3*(C1-C2))*(P_In-C2)+C3*(P_In-C2)*(P_In-C2));
						}
					}
				}else{
					VA_Efficiency = VA_Out.Re();
				}
				if ((phases & 0x10) == 0x10){
					power_val[0].SetReal(VA_Efficiency);
				} else {
					if ((phases & 0x01) == 0x01) {
						power_val[0].SetReal(VA_Efficiency/number_of_phases_out);
					}
					if ((phases & 0x02) == 0x02) {
						power_val[1].SetReal(VA_Efficiency/number_of_phases_out);
					}
					if ((phases & 0x04) == 0x04) {
						power_val[2].SetReal(VA_Efficiency/number_of_phases_out);
					}
				}
			}

			//Determine 4 quadrant outputs
			if(four_quadrant_control_mode == FQM_CONSTANT_PF)	//Power factor mode
			{
				if(power_factor != 0.0)	//Not purely imaginary
				{
					if (P_In<0.0)	//Discharge at input, so must be "load"
					{
						//Total power output is the magnitude
						VA_Out.SetReal(VA_Efficiency*-1.0);
					}
					else if (P_In>0.0)	//Positive input, so must be generator
					{
						//Total power output is the magnitude
						VA_Out.SetReal(VA_Efficiency);
					}
					else
					{
						VA_Out.SetReal(0.0);
					}

					//Apply power factor sign properly - + sign is lagging in, which is proper here
					//Treat like a normal load right now
					if (power_factor < 0)
					{
						VA_Out.SetImag((VA_Efficiency/sqrt(power_factor*power_factor))*sqrt(1.0-(power_factor*power_factor)));
					}
					else	//Must be positive
					{
						VA_Out.SetImag((VA_Efficiency/sqrt(power_factor*power_factor))*-1.0*sqrt(1.0-(power_factor*power_factor)));
					}
				}
				else	//Purely imaginary value
				{
					VA_Out = gld::complex(0.0,VA_Efficiency);
				}
			}

			// For PQ constant mode, and the VSI droop mode, the PQ outputs in steady state will be matching given glm values
			else if (four_quadrant_control_mode == FQM_CONSTANT_PQ)
			{
				// Give values to Pref and Qref so that they will not be zero monitored in steady state
				Pref = P_Out;
				Qref = Q_Out;

				//Compute desired output - sign convention appears to be backwards
				temp_VA = gld::complex(P_Out,Q_Out);
				// to see if we have a battery as a power source
				if (b_soc != -1) {
					//Ensuring battery has capacity to charge or discharge as needed.
					if ((b_soc >= 1.0) && (temp_VA.Re() < 0))	//Battery full and positive influx of real power
					{
						gl_warning("inverter:%s - battery full - no charging allowed",obj->name);
						temp_VA.SetReal(0.0);	//Set to zero - reactive considerations may change this
					}
					else if ((b_soc <= soc_reserve) && (temp_VA.Re() > 0))	//Battery "empty" and attempting to extract real power
					{
						gl_warning("inverter:%s - battery at or below the SOC reserve - no discharging allowed",obj->name);
						temp_VA.SetReal(0.0);	//Set output to zero - again, reactive considerations may change this
					}

					//Ensuring power rating of inverter is not exceeded.
					if (fabs(temp_VA.Mag()) > p_max){ //Requested power output (P_Out, Q_Out) is greater than inverter rating
						if (p_max > fabs(temp_VA.Re())) //Can we reduce the reactive power output and stay within the inverter rating?
						{
							//Determine the Q we can provide
							temp_QVal = sqrt((p_max*p_max) - (temp_VA.Re()*temp_VA.Re()));

							//Assign to output, negating signs as necessary (temp_VA already negated)
							if (temp_VA.Im() < 0.0)	//Negative Q dispatch
							{
								VA_Out = gld::complex(temp_VA.Re(),-temp_QVal);
							}
							else	//Positive Q dispatch
							{
								VA_Out = gld::complex(temp_VA.Re(),temp_QVal);
							}
						}
						else	//Inverter rated power is equal to or smaller than real power desired, give it all we can
						{
							//Maintain desired sign convention
							if (temp_VA.Re() < 0.0)
							{
								VA_Out = gld::complex(-p_max,0.0);
							}
							else	//Positive
							{
								VA_Out = gld::complex(p_max,0.0);
							}
						}
					}
					else	//Doesn't exceed, assign it
					{
						VA_Out = temp_VA;
					}
					//Update values to represent what is being pulled (battery uses for SOC updates) - assumes only storage
					//p_in used by battery - appears reversed to VA_Out
					if (VA_Out.Re() > 0.0)	//Discharging
					{
						p_in = VA_Out.Re()/inv_eta;
					}
					else if (VA_Out.Re() == 0.0)	//Idle
					{
						p_in = 0.0;
					}
					else	//Must be positive, so charging
					{
						p_in = VA_Out.Re()*inv_eta;
					}
				} else {// no battery is attached we are attached to a solar panel
					//Ensuring power rating of inverter is not exceeded.
					if (fabs(temp_VA.Mag()) > p_max){ //Requested power output (P_Out, Q_Out) is greater than inverter rating
						if (p_max > fabs(temp_VA.Re())) //Can we reduce the reactive power output and stay within the inverter rating?
						{
							//Determine the Q we can provide
							temp_QVal = sqrt((p_max*p_max) - (temp_VA.Re()*temp_VA.Re()));

							//Assign to output, negating signs as necessary (temp_VA already negated)
							if (temp_VA.Im() < 0.0)	//Negative Q dispatch
							{
								temp_VA.SetImag(-temp_QVal);
							}
							else	//Positive Q dispatch
							{
								temp_VA.SetImag(temp_QVal);
							}
						}
						else	//Inverter rated power is equal to or smaller than real power desired, give it all we can
						{
							//Maintain desired sign convention
							if (temp_VA.Re() < 0.0)
							{
								temp_VA = gld::complex(-p_max,0.0);
							}
							else	//Positive
							{
								temp_VA = gld::complex(p_max,0.0);
							}
						}
					}
					//Determine how much we can supply from the solar panel output.
					if (fabs(temp_VA.Mag()) > VA_Efficiency){ //Requested power output (P_Out, Q_Out) is greater than solar input
						if (VA_Efficiency > fabs(temp_VA.Re())) //Can we reduce the reactive power output and stay within the solar input?
						{
							//Determine the Q we can provide
							temp_QVal = sqrt((VA_Efficiency*VA_Efficiency) - (temp_VA.Re()*temp_VA.Re()));

							//Assign to output, negating signs as necessary (temp_VA already negated)
							if (temp_VA.Im() < 0.0)	//Negative Q dispatch
							{
								VA_Out = gld::complex(temp_VA.Re(),-temp_QVal);
							}
							else	//Positive Q dispatch
							{
								VA_Out = gld::complex(temp_VA.Re(),temp_QVal);
							}
						}
						else	//solar panel output is equal to or smaller than real power desired, give it all we can
						{
							//Maintain desired sign convention
							if (temp_VA.Re() < 0.0)
							{
								VA_Out = gld::complex(-VA_Efficiency,0.0);
							}
							else	//Positive
							{
								VA_Out = gld::complex(VA_Efficiency,0.0);
							}
						}
					} else {
						VA_Out = temp_VA;
					}
				}



				//}
			}
			else if (four_quadrant_control_mode == FQM_VSI)
			{
				// VSI isochronous mode
				if (VSI_mode == VSI_ISOCHRONOUS || VSI_mode == VSI_DROOP) {
					//Calculate power based on measured terminal voltage and currents
					if ((phases & 0x10) == 0x10) // split phase
					{
						//Update output power
						//Get current injected
						temp_current_val[0] = value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0];

						//Update power output variables, just so we can see what is going on
						VA_Out = value_Circuit_V[0]*~temp_current_val[0];

					}
					else	//Three phase variant
					{
						//Update output power
						//Get current injected
						temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0] - generator_admittance[0][1]*value_Circuit_V[1] - generator_admittance[0][2]*value_Circuit_V[2]);
						temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0]*value_Circuit_V[0] - generator_admittance[1][1]*value_Circuit_V[1] - generator_admittance[1][2]*value_Circuit_V[2]);
						temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0]*value_Circuit_V[0] - generator_admittance[2][1]*value_Circuit_V[1] - generator_admittance[2][2]*value_Circuit_V[2]);

						//Update power output variables, just so we can see what is going on
						power_val[0] = value_Circuit_V[0]*~temp_current_val[0];
						power_val[1] = value_Circuit_V[1]*~temp_current_val[1];
						power_val[2] = value_Circuit_V[2]*~temp_current_val[2];

						VA_Out = power_val[0] + power_val[1] + power_val[2];
					}
				}

				// Check VA_Out values
				temp_VA = VA_Out;

				// For VSI, the VA_Out values are determined after the first time step of power flow
				// Therefore, only set battery p values accordingly after that
				if (first_run) {
					p_in = 0;
				}
				else {
					//Ensuring battery has capacity to charge or discharge as needed.
					if ((b_soc >= 1.0) && (temp_VA.Re() < 0) && (b_soc != -1))	//Battery full and positive influx of real power
					{
						gl_warning("inverter:%s - battery full - no charging allowed",obj->name);
						temp_VA.SetReal(0.0);	//Set to zero - reactive considerations may change this
					}
					else if ((b_soc <= soc_reserve) && (temp_VA.Re() > 0) && (b_soc != -1))	//Battery "empty" and attempting to extract real power
					{
						gl_warning("inverter:%s - battery at or below the SOC reserve - no discharging allowed",obj->name);
						temp_VA.SetReal(0.0);	//Set output to zero - again, reactive considerations may change this
					}

					//Ensuring power rating of inverter is not exceeded.
					if (fabs(temp_VA.Mag()) > p_max){ //Requested power output (P_Out, Q_Out) is greater than inverter rating
						if (p_max > fabs(temp_VA.Re())) //Can we reduce the reactive power output and stay within the inverter rating?
						{
							//Determine the Q we can provide
							temp_QVal = sqrt((p_max*p_max) - (temp_VA.Re()*temp_VA.Re()));

							//Assign to output, negating signs as necessary (temp_VA already negated)
							if (temp_VA.Im() < 0.0)	//Negative Q dispatch
							{
								VA_Out = gld::complex(temp_VA.Re(),-temp_QVal);
							}
							else	//Positive Q dispatch
							{
								VA_Out = gld::complex(temp_VA.Re(),temp_QVal);
							}
						}
						else	//Inverter rated power is equal to or smaller than real power desired, give it all we can
						{
							//Maintain desired sign convention
							if (temp_VA.Re() < 0.0)
							{
								VA_Out = gld::complex(-p_max,0.0);
							}
							else	//Positive
							{
								VA_Out = gld::complex(p_max,0.0);
							}
						}
					}
					else	//Doesn't exceed, assign it
					{
						VA_Out = temp_VA;
					}

					//Update values to represent what is being pulled (battery uses for SOC updates) - assumes only storage
					//p_in used by battery - appears reversed to VA_Out
					if (VA_Out.Re() > 0.0)	//Discharging
					{
						p_in = VA_Out.Re()/inv_eta;
					}
					else if (VA_Out.Re() == 0.0)	//Idle
					{
						p_in = 0.0;
					}
					else	//Must be positive, so charging
					{
						p_in = VA_Out.Re()*inv_eta;
					}
				}

				// Set PQ referance values in event mode always the same as VA_Out for VSI
				Pref = VA_Out.Re();
				Qref = VA_Out.Im();
			}
			else if (four_quadrant_control_mode == FQM_LOAD_FOLLOWING)
			{
				VA_Out = -lf_dispatch_power;	//Place the expected dispatch power into the output
			}
			else if (four_quadrant_control_mode == FQM_GROUP_LF)
			{
				VA_Out = -lf_dispatch_power;	//Place the expected dispatch power into the output
			}
			else if (four_quadrant_control_mode == FQM_VOLT_VAR_FREQ_PWR) {
				// start Ab add
				// Jason Bank, jason.bank@nrel.gov		8/26/2013
				// use voltage control input with lookup table values to determine what Qo should be then update Po according to:
				// Po = (Pi * eff) - Qo * (1 - eff)/eff		Inverter real output power including conversion losses for generating Qo
				// Ab note Jason originally only had Po = (Pi * eff) - Qo * (1 - eff); actually think losses should be proportional to S, but will leave for later

				//TODO : add lookup for power for frequency regulation P_Out_fr

				if((P_In == 0.0) && disable_volt_var_if_no_input_power)
					VA_Out = gld::complex(0,0);
				else
				{
					//currently only compares to the phase A inverter AC voltage,
					//TODO: need to address for non-3phase inv? include support for a remote voltage input?

					double Qo = VoltVArSched.back().second;			//set the scheduled Q for highest voltage range, handles the last case with the loop below (will be overwritten if needed)
					double prevV = 0;								//setup for first loop iter to handle lowest voltage range
					double prevQ = VoltVArSched.front().second;		//setup for first loop iter to handle lowest voltage range
					for (size_t i = 0; i < VoltVArSched.size(); i++)
					{	//iterate over all specified voltage ranges, find where current voltage value lies and set Qo as linear interpolation between endpoints
						if(phaseA_V_Out.Mag() <= VoltVArSched[i].first) {
							double m = (VoltVArSched[i].second - prevQ)/(VoltVArSched[i].first - prevV);
							double b = VoltVArSched[i].second - (m * VoltVArSched[i].first);
							Qo = m * phaseA_V_Out.Mag() + b;
							break;
						}
						prevV = VoltVArSched[i].first;
						prevQ = VoltVArSched[i].second;
					}

					double Po = (P_In * net_eff) - fabs(Qo) * (1 - net_eff)/net_eff;

					if(P_In < 0.0)
						VA_Out = gld::complex(Po,-Qo);	//Qo sign convention backwards from what i was expecting
					else
						VA_Out = gld::complex(-Po,-Qo);	//Qo sign convention backwards from what i was expecting
				}

				//TODO: should VA_Out be checked against inverter power rating? if exceeds clip it? clip to preserve reactive power set point or to preserve real output power?
				// end Ab add
			}//end VOLT_VAR_FREQ_PWR mode

			//Execution of power-factor regulation output of inverter that will get included in power-flow solution
			if ((pf_reg == INCLUDED) || (pf_reg == INCLUDED_ALT))
			{
				VA_Out.Im() = -pf_reg_dispatch_VAR; //Flipping from load to generator perspective.
			}
			
			//Not implemented and removed from above, so no check needed
			//else if(four_quadrant_control_mode == FQM_CONSTANT_V){
			//	GL_THROW("CONSTANT_V mode is not supported at this time.");
			//} else if(four_quadrant_control_mode == FQM_VOLT_VAR){
			//	GL_THROW("VOLT_VAR mode is not supported at this time.");
			//}
			if ((four_quadrant_control_mode != FQM_VOLT_VAR) && (four_quadrant_control_mode != FQM_VOLT_WATT))
			{
				//check to see if VA_Out is within rated absolute power rating
				if(VA_Out.Mag() > p_max)
				{
					//Determine the excess, for use elsewhere - back out simple efficiencies
					excess_input_power = (VA_Out.Mag() - p_max);

					//Apply thresholding - going on the assumption of maintaining vector direction
					if (four_quadrant_control_mode == FQM_CONSTANT_PF)
					{
						temp_PF = power_factor;
					}
					else	//Extract it - overall value (signs handled separately)
					{
						temp_PF = VA_Out.Re()/VA_Out.Mag();
					}

					//Compute the "new" output - signs lost
					temp_VA = gld::complex(fabs(p_max*temp_PF),fabs(p_max*sqrt(1.0-(temp_PF*temp_PF))));

					//"Sign" it appropriately
					if ((VA_Out.Re()<0) && (VA_Out.Im()<0))	//-R, -I
					{
						VA_Out = -temp_VA;
					}
					else if ((VA_Out.Re()<0) && (VA_Out.Im()>=0))	//-R,I
					{
						VA_Out = gld::complex(-temp_VA.Re(),temp_VA.Im());
					}
					else if ((VA_Out.Re()>=0) && (VA_Out.Im()<0))	//R,-I
					{
						VA_Out = gld::complex(temp_VA.Re(),-temp_VA.Im());
					}
					else	//R,I
					{
						VA_Out = temp_VA;
					}
				}
				else	//Not over, zero "overrage"
				{
					excess_input_power = 0.0;
				}

				//See if load following, if so, make sure storage is appropriate - only considers real right now
				if (four_quadrant_control_mode == FQM_LOAD_FOLLOWING || four_quadrant_control_mode == FQM_GROUP_LF || battery_power_out.Mag() != 0.0)
				{
					if ((b_soc == 1.0) && (VA_Out.Re() < 0) && (b_soc != -1))	//Battery full and positive influx of real power
					{
						gl_warning("inverter:%s - battery full - no charging allowed",obj->name);
						/*  TROUBLESHOOT
						In LOAD_FOLLOWING mode, a full battery status was encountered.  The inverter is unable
						to sink any further energy, so consumption was set to zero.
						*/
						VA_Out.SetReal(0.0);	//Set to zero - reactive considerations may change this
					}
					else if ((b_soc <= soc_reserve) && (VA_Out.Re() > 0) && (b_soc != -1))	//Battery "empty" and attempting to extract real power
					{
						gl_warning("inverter:%s - battery at or below the SOC reserve - no discharging allowed",obj->name);
						/*  TROUBLESHOOT
						In LOAD_FOLLOWING mode, a empty or "in the SOC reserve margin" battery was encountered and attempted
						to discharge.  The inverter is unable to extract any further power, so the output is set to zero.
						*/
						VA_Out.SetReal(0.0);	//Set output to zero - again, reactive considerations may change this
					}

					//Update values to represent what is being pulled (battery uses for SOC updates) - assumes only storage
					//p_in used by battery - appears reversed to VA_Out
					if (VA_Out.Re() > 0.0)	//Discharging
					{
						p_in = VA_Out.Re()/inv_eta;
					}
					else if (VA_Out.Re() == 0.0)	//Idle
					{
						p_in = 0.0;
					}
					else	//Must be positive, so charging
					{
						p_in = VA_Out.Re()*inv_eta;
					}
				}//End load following battery considerations

				//Assign secondary outputs
				if(four_quadrant_control_mode != FQM_CONSTANT_PQ && four_quadrant_control_mode != FQM_VSI){
					P_Out = VA_Out.Re();
					Q_Out = VA_Out.Im();
				}

				// For VSI droop mode, try to match the total PQ out with the glm values
				if (four_quadrant_control_mode == FQM_VSI && VSI_mode == VSI_DROOP) {

					//Only do updates if this is a new timestep
					if ((prev_time < t1) && !first_run)
					{
						// Adjust VSI (not on SWING bus) current injection and e_source values only at the first iteration of each time step
						if ((phases & 0x10) == 0x10) // split phase
						{
							if (attached_bus_type != 2) {

								//Compute desired output - sign convention appears to be backwards
								gld::complex temp_VA = gld::complex(P_Out,Q_Out);

								//Update output power
								temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0]);

								//Update power output variables, just so we can see what is going on
								power_val[0] = value_Circuit_V[0]*~temp_current_val[0];

								VA_Out = power_val[0];

								//Copy in value
								temp_power_val[0] = power_val[0] + (temp_VA - VA_Out);

								//Back out the current injection
								if (value_Circuit_V[0].Mag() > 0.0)
								{
									value_IGenerated[0] = ~(temp_power_val[0]/value_Circuit_V[0]) + generator_admittance[0][0]*value_Circuit_V[0];
								}
								else
								{
									value_IGenerated[0] = complex(0.0,0.0);
								}

								//Compute desired output - sign convention appears to be backwards
								e_source[0] = value_IGenerated[0] * (gld::complex(Rfilter,Xfilter) * Zbase);
								V_angle[0] = (e_source[0]).Arg();  // Obtain the inverter source voltage phasor angle
								V_mag[0] = e_source[0].Mag();
							}
						}
						else {
							// Adjust VSI (not on SWING bus) current injection and e_source values only at the first iteration of each time step
							if (attached_bus_type != 2) {

								//Compute desired output - sign convention appears to be backwards
								gld::complex temp_VA = gld::complex(P_Out,Q_Out);

								//Update output power
								//Get current injected
								temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0] - generator_admittance[0][1]*value_Circuit_V[1] - generator_admittance[0][2]*value_Circuit_V[2]);
								temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0]*value_Circuit_V[0] - generator_admittance[1][1]*value_Circuit_V[1] - generator_admittance[1][2]*value_Circuit_V[2]);
								temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0]*value_Circuit_V[0] - generator_admittance[2][1]*value_Circuit_V[1] - generator_admittance[2][2]*value_Circuit_V[2]);

								//Update power output variables, just so we can see what is going on
								power_val[0] = value_Circuit_V[0]*~temp_current_val[0];
								power_val[1] = value_Circuit_V[1]*~temp_current_val[1];
								power_val[2] = value_Circuit_V[2]*~temp_current_val[2];

								VA_Out = power_val[0] + power_val[1] + power_val[2];

								//Copy in value
								temp_power_val[0] = power_val[0] + (temp_VA - VA_Out) / 3.0;
								temp_power_val[1] = power_val[1] + (temp_VA - VA_Out) / 3.0;
								temp_power_val[2] = power_val[2] + (temp_VA - VA_Out) / 3.0;

								//Back out the current injection - look for zero voltage so no divide by zero errors occur
								for (tmp_index_val=0; tmp_index_val<3; tmp_index_val++)
								{
									if (value_Circuit_V[tmp_index_val].Mag() > 0.0)
									{
										temp_current_val[tmp_index_val] = ~(temp_power_val[tmp_index_val]/value_Circuit_V[tmp_index_val]);
									}
									else
									{
										temp_current_val[tmp_index_val] = gld::complex(0.0,0.0);
									}
								}

								value_IGenerated[0] = temp_current_val[0] + generator_admittance[0][0]*value_Circuit_V[0] + generator_admittance[0][1]*value_Circuit_V[1] + generator_admittance[0][2]*value_Circuit_V[2];
								value_IGenerated[1] = temp_current_val[1] + generator_admittance[1][0]*value_Circuit_V[0] + generator_admittance[1][1]*value_Circuit_V[1] + generator_admittance[1][2]*value_Circuit_V[2];
								value_IGenerated[2] = temp_current_val[2] + generator_admittance[2][0]*value_Circuit_V[0] + generator_admittance[2][1]*value_Circuit_V[1] + generator_admittance[2][2]*value_Circuit_V[2];

								//Compute desired output - sign convention appears to be backwards
								for (int i = 0; i < 3; i++) {
									// Update e_source value for droop VSI based on updated current injection
									e_source[i] = value_IGenerated[i] * (gld::complex(Rfilter,Xfilter) * Zbase);
									V_angle[i] = (e_source[i]).Arg();  // Obtain the inverter source voltage phasor angle
									V_mag[i] = e_source[i].Mag();
								}
							}
						}

						//Update time
						prev_time = t1;
						prev_time_dbl = (double)(t1);

						//Keep us here
						tret_value = t1;
					}
				} // End adjusting droop mode VSI

				else if (four_quadrant_control_mode != FQM_VSI)
				{
					//Calculate power and post it
					if ((phases & 0x10) == 0x10) // split phase
					{
						//Update last_power variable
						last_power[3] = -VA_Out;
						curr_VA_out[0] = VA_Out;

						//Post the value
						if (value_Circuit_V[0].Mag() > 0.0)
						{
							I_Out[0] = ~(VA_Out / value_Circuit_V[0]);
						}
						else
						{
							I_Out[0] = gld::complex(0.0,0.0);
						}

						if (four_quadrant_control_mode != FQM_VSI) {
							if (deltamode_inclusive)
							{
								last_current[3] = -I_Out[0];
								value_Line_unrotI[0] = last_current[3];
							}
							else
							{
								value_Power12 =last_power[3];
							}
						}
						//FQM_VSI assumed
						/****** TODO: Verify/Make generators work with triplex! ********/
					}
					else	//Three phase variant
					{
						//Figure out amount that needs to be posted
						temp_VA = -VA_Out/number_of_phases_out;

						if ( (phases & 0x01) == 0x01 ) // has phase A
						{
							curr_VA_out[0] = -temp_VA;
							last_power[0] = temp_VA;	//Store last power

							if (value_Circuit_V[0].Mag() > 0.0)
							{
								I_Out[0] = ~(-temp_VA / value_Circuit_V[0]);
							}
							else
							{
								I_Out[0] = gld::complex(0.0,0.0);
							}

							if (four_quadrant_control_mode != FQM_VSI) {
								if (deltamode_inclusive)
								{
									last_current[0] = -I_Out[0];
									value_Line_unrotI[0] = last_current[0];
								}
								else
								{
									value_Power[0] = temp_VA;		//Post the current value
								}
							}
						}

						if ( (phases & 0x02) == 0x02 ) // has phase B
						{
							curr_VA_out[1] = -temp_VA;
							last_power[1] = temp_VA;	//Store last power

							if (value_Circuit_V[1].Mag() > 0.0)
							{
								I_Out[1] = ~(-temp_VA / value_Circuit_V[1]);
							}
							else
							{
								I_Out[1] = gld::complex(0.0,0.0);
							}

							if (four_quadrant_control_mode != FQM_VSI) {
								if (deltamode_inclusive)
								{
									last_current[1] = -I_Out[1];
									value_Line_unrotI[1] = last_current[1];
								}
								else
								{
									value_Power[1] = temp_VA;		//Post the current value
								}

							}
						}

						if ( (phases & 0x04) == 0x04 ) // has phase C
						{
							curr_VA_out[2] = -temp_VA;
							last_power[2] = temp_VA;	//Store last power

							if (value_Circuit_V[2].Mag() > 0.0)
							{
								I_Out[2] = ~(-temp_VA / value_Circuit_V[2]);
							}
							else
							{
								I_Out[2] = gld::complex(0.0,0.0);
							}

							if (four_quadrant_control_mode != FQM_VSI) {
								if (deltamode_inclusive)
								{
									last_current[2] = -I_Out[2];
									value_Line_unrotI[2] = last_current[2];
								}
								else
								{
									value_Power[2] = temp_VA;		//Post the current value
								}
							}
						}
					} //End three-phase variant
				}//End non-Volt Var Control mode
			} else { // Volt Var or Volt-Watt Control mode
				if (four_quadrant_control_mode == FQM_VOLT_VAR)
				{
					if (power_val[0].Mag() > p_rated) {
						if (power_val[0].Re() > p_rated) {
							power_val[0].SetReal(p_rated);
							power_val[0].SetImag(0);
						} else if (power_val[0].Re() < -p_rated) {
							power_val[0].SetReal(-p_rated);
							power_val[0].SetImag(0);
						} else if (power_val[0].Re() < p_rated && power_val[0].Re() > -p_rated) {
							double q_max = 0;
							q_max = sqrt((p_rated * p_rated) - (power_val[0].Re() * power_val[0].Re()));
							if (power_val[0].Im() > q_max) {
								power_val[0].SetImag(q_max);
							} else {
								power_val[0].SetImag(-q_max);
							}
						}
					}
					if (power_val[1].Mag() > p_rated ) {
						if (power_val[1].Re() > p_rated) {
							power_val[1].SetReal(p_rated);
							power_val[1].SetImag(0);
						} else if (power_val[1].Re() < -p_rated) {
							power_val[1].SetReal(-p_rated);
							power_val[1].SetImag(0);
						} else if (power_val[1].Re() < p_rated && power_val[1].Re() > -p_rated) {
							double q_max = 0;
							q_max = sqrt((p_rated * p_rated) - (power_val[1].Re() * power_val[1].Re()));
							if (power_val[1].Im() > q_max) {
								power_val[1].SetImag(q_max);
							} else {
								power_val[1].SetImag(-q_max);
							}
						}
					}
					if (power_val[2].Mag() > p_rated ) {
						if (power_val[2].Re() > p_rated) {
							power_val[2].SetReal(p_rated);
							power_val[2].SetImag(0);
						} else if (power_val[2].Re() < -p_rated) {
							power_val[2].SetReal(-p_rated);
							power_val[2].SetImag(0);
						} else if (power_val[2].Re() < p_rated && power_val[2].Re() > -p_rated) {
							double q_max = 0;
							q_max = sqrt((p_rated * p_rated) - (power_val[2].Re() * power_val[2].Re()));
							if (power_val[2].Im() > q_max) {
								power_val[2].SetImag(q_max);
							} else {
								power_val[2].SetImag(-q_max);
							}
						}
					}
				}
				else if (four_quadrant_control_mode == FQM_VOLT_WATT)
				{
					if (power_val[0].Re() > pa_vw_limited) {
						power_val[0].SetReal (pa_vw_limited);
					}
					if (power_val[1].Re() > pb_vw_limited) {
						power_val[1].SetReal (pb_vw_limited);
					}
					if (power_val[2].Re() > pc_vw_limited) {
						power_val[2].SetReal (pc_vw_limited);
					}
				}

				if ((phases & 0x10) == 0x10) {
					p_in = power_val[0].Re() / inv_eta;
					last_power[3] = -power_val[0];
					value_Power12 = last_power[3];

					if (value_Circuit_V[0].Mag() > 0.0)
					{
						I_Out[0] = ~(power_val[0] / value_Circuit_V[0]);
					}
					else
					{
						I_Out[0] = gld::complex(0.0,0.0);
					}
				} else {
					p_in = 0;
					if ((phases & 0x01) == 0x01) {
						p_in += power_val[0].Re()/inv_eta;
						last_power[0] = -power_val[0];
						value_Power[0] = last_power[0];
						if (value_Circuit_V[0].Mag() > 0.0)
						{
							I_Out[0] = ~(power_val[0] / value_Circuit_V[0]);
						}
						else
						{
							I_Out[0] = gld::complex(0.0,0.0);
						}
					}
					if ((phases & 0x02) == 0x02) {
						p_in += power_val[1].Re()/inv_eta;
						last_power[1] = -power_val[1];
						value_Power[1] = last_power[1];
						if (value_Circuit_V[1].Mag() > 0.0)
						{
							I_Out[1] = ~(power_val[1] / value_Circuit_V[1]);
						}
						else
						{
							I_Out[1] = gld::complex(0.0,0.0);
						}
					}
					if ((phases & 0x04) == 0x04) {
						p_in += power_val[2].Re()/inv_eta;
						last_power[2] = -power_val[2];
						value_Power[2] = last_power[2];
						if (value_Circuit_V[2].Mag() > 0.0)
						{
							I_Out[2] = ~(power_val[2] / value_Circuit_V[2]);
						}
						else
						{
							I_Out[2] = gld::complex(0.0,0.0);
						}
					}
				}
			} // End VOLT_VAR and VOLT_WATT

			// Check P_In (calcualted from V_In and I_In), and compared with p_in (calculated from VA_Out)
			if (P_In < p_in) {
				gl_warning("inverter:%d - %s - Real power output (%g) exceeds maximum DC output (%g)",obj->id,(obj->name?obj->name:"Unnamed"),p_in,P_In);
				/*  TROUBLESHOOT
				The real power output of the inverter exceeds the maximum DC power being input.  In the specific operating mode used,
				the inverter will not limit the output.  Choose a different dispatch point, change the inverter rating, or adjust the
				DC input.
				*/
			}

		}//End FOUR_QUADRANT mode
	}
	else
	{
		//Check to see if we're accumulating or out of service
		if ((value_MeterStatus==1) && inverter_1547_status)
		{
			if (inverter_type_v != FOUR_QUADRANT)
			{
				//Will only get here on true NR_cycle, if meter is in service
				if ((phases & 0x10) == 0x10)
				{
					//Voltage check
					if (value_Circuit_V[0].Mag() > 0.0)
					{
						value_Line12 = last_current[3];
					}
					else	//Must be disconnected, but 1547 hasn't kicked in
					{
						//Do like disconnect below
						last_current[3] = gld::complex(0.0,0.0);
					}
				}
				else
				{
					//Voltage check
					if ((value_Circuit_V[0].Mag() > 0.0) && (value_Circuit_V[1].Mag() > 0.0) && (value_Circuit_V[2].Mag() > 0.0))
					{
						value_Line_I[0] = last_current[0];
						value_Line_I[1] = last_current[1];
						value_Line_I[2] = last_current[2];
					}
					else	//Must be disconnected, but 1547 hasn't kicked in
					{
						//Do like disconnect below
						last_current[0] = gld::complex(0.0,0.0);
						last_current[1] = gld::complex(0.0,0.0);
						last_current[2] = gld::complex(0.0,0.0);
					}
				}
			}
			else	//Four-quadrant post as power
			{
				//Will only get here on true NR_cycle, if meter is in service
				if ((phases & 0x10) == 0x10)	//Triplex
				{
					//Voltage check
					if (value_Circuit_V[0].Mag() > 0.0)
					{
						value_Power12 = last_power[3];	//Theoretically pPower is mapped to power_12, which already has the [2] offset applied
					}
					else
					{
						//Do like disconnect below
						last_power[3] = gld::complex(0.0,0.0);
					}
				}
				else
				{
					//Voltage check
					if ((value_Circuit_V[0].Mag() > 0.0) && (value_Circuit_V[1].Mag() > 0.0) && (value_Circuit_V[2].Mag() > 0.0))
					{
						value_Power[0] = last_power[0];
						value_Power[1] = last_power[1];
						value_Power[2] = last_power[2];
					}
					else	//Must be disconnected, but 1547 hasn't kicked in
					{
						//Do like disconnect below
						last_power[0] = gld::complex(0.0,0.0);
						last_power[1] = gld::complex(0.0,0.0);
						last_power[2] = gld::complex(0.0,0.0);
					}
				}
			}
		}
		else
		{
			//No contributions, but zero the last_current, just to be safe
			last_current[0] = 0.0;
			last_current[1] = 0.0;
			last_current[2] = 0.0;
			last_current[3] = 0.0;

			//Do the same for power, just for paranoia's sake
			last_power[0] = 0.0;
			last_power[1] = 0.0;
			last_power[2] = 0.0;
			last_power[3] = 0.0;
		}
	}

	//Sync the powerflow variables
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values();
	}

	//Return
	if (tret_value != TS_NEVER)
	{
		return -tret_value;
	}
	else
	{
		return TS_NEVER;
	}
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP inverter::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2 = TS_NEVER;		//By default, we're done forever!
	LOAD_FOLLOW_STATUS new_lf_status;
	PF_REG_STATUS new_pf_reg_status;
	double new_lf_dispatch_power, curr_power_val, diff_power_val;				
	double new_pf_reg_distpatch_VAR, curr_real_power_val, curr_reactive_power_val, curr_pf, available_VA, new_Q_out, Q_out, Q_required, Q_available, Q_load;
	double scaling_factor, Q_target;
	gld::complex temp_current_val[3];
	TIMESTAMP dt;
	double inputPower;
	gld::complex temp_complex_value;

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		reset_complex_powerflow_accumulators();

		//Also pull the current values
		pull_complex_powerflow_values();
	}

	//Check and see if we need to redispatch
	if ((inverter_type_v == FOUR_QUADRANT) && (four_quadrant_control_mode == FQM_LOAD_FOLLOWING) && lf_dispatch_change_allowed)
	{
		//See what the sense_object is and determine if we need to update
		if (sense_is_link)
		{
			//Perform the power update
			((void (*)(OBJECT *))(*powerCalc))(sense_object);
		}//End link update of power

		//Extract power, mainly just for convenience, but also to offset us for what we are currently "ordering"
		temp_complex_value = sense_power->get_complex();
		curr_power_val = temp_complex_value.Re();

		//Check power - only focus on real for now -- this will need to be changed if reactive considered
		if (curr_power_val<=charge_on_threshold)	//Below charging threshold, desire charging
		{
			//See if the battery has room
			if ((b_soc < 1.0) && (b_soc != -1))
			{
				//See if we were already railed - if less, still have room, or we were discharging (either way okay - just may iterate a couple times)
				if (lf_dispatch_power < max_charge_rate)
				{
					//Make sure we weren't discharging
					if (load_follow_status == DISCHARGE)
					{
						//Set us to idle (will force a reiteration) - who knows where we really are
						new_lf_status = IDLE;
						new_lf_dispatch_power = 0.0;
					}
					else
					{
						//Set us up to charge
						new_lf_status = CHARGE;

						//Determine how far off we are from the "on" point (on is our "maintain" point)
						diff_power_val = charge_on_threshold - curr_power_val;

						//Accumulate it
						new_lf_dispatch_power = lf_dispatch_power + diff_power_val;

						//Make sure it isn't too big
						if (new_lf_dispatch_power > max_charge_rate)
						{
							new_lf_dispatch_power = max_charge_rate;	//Desire more than we can give, limit it
						}
					}
				}
				else	//We were railed, continue with this course
				{
					new_lf_status = CHARGE;						//Keep us charging
					new_lf_dispatch_power = max_charge_rate;	//Set to maximum rate
				}

			}//End Battery has room
			else	//Battery full, no charging allowed
			{
				gl_verbose("inverter:%s - charge desired, but battery full!",obj->name);
				/*  TROUBLESHOOT
				An inverter in LOAD_FOLLOWING mode currently wants to charge the battery more, but the battery
				is full.  Consider using a larger battery and trying again.
				*/

				new_lf_status = IDLE;			//Can't do anything, so we're idle
				new_lf_dispatch_power = 0.0;	//Turn us off
			}
		}//End below charge_on_threshold
		else if (curr_power_val<=charge_off_threshold)	//Below charging off threshold - see what we were doing
		{
			if (load_follow_status != CHARGE)
			{
				new_lf_status = IDLE;			//We were either idle or discharging, so we are now idle
				new_lf_dispatch_power = 0.0;	//Idle power as well
			}
			else	//Must be charging
			{
				//See if the battery has room
				if ((b_soc < 1.0) && (b_soc != -1))
				{
					new_lf_status = CHARGE;	//Keep us charging

					//Inside this range, just maintain what we were doing
					new_lf_dispatch_power = lf_dispatch_power;
				}//End Battery has room
				else	//Battery full, no charging allowed
				{
					gl_verbose("inverter:%s - charge desired, but battery full!",obj->name);
					//Defined above

					new_lf_status = IDLE;			//Can't do anything, so we're idle
					new_lf_dispatch_power = 0.0;	//Turn us off
				}
			}//End must be charging
		}//End below charge_off_threshold (but above charge_on_threshold)
		else if (curr_power_val<=discharge_off_threshold)	//Below the discharge off threshold - we're idle no matter what here
		{
			new_lf_status = IDLE;			//Nothing occurring, between bands
			new_lf_dispatch_power = 0.0;	//Nothing needed, as a result
		}//End below discharge_off_threshold (but above charge_off_threshold)
		else if (curr_power_val<=discharge_on_threshold)	//Below discharge on threshold - see what we were doing
		{
			//See if we were discharging
			if (load_follow_status == DISCHARGE)
			{
				//Were discharging - see if we can continue
				if ((b_soc > soc_reserve) && (b_soc != -1))
				{
					new_lf_status = DISCHARGE;	//Keep us discharging

					new_lf_dispatch_power = lf_dispatch_power;	//Maintain what we were doing inside the deadband
				}
				else	//At or below reserve, go to idle
				{
					gl_verbose("inverter:%s - discharge desired, but not enough battery capacity!",obj->name);
					/*  TROUBLESHOOT
					An inverter in LOAD_FOLLOWING mode currently wants to discharge the battery more, but the battery
					is at or below the SOC reserve margin.  Consider using a larger battery and trying again.
					*/

					new_lf_status = IDLE;			//Can't do anything, so we're idle
					new_lf_dispatch_power = 0.0;	//Turn us off
				}
			}
			else	//Must not have been discharging, go idle first since the lack of charge may drop us enough
			{
				new_lf_status = IDLE;			//We'll force a reiteration, maybe
				new_lf_dispatch_power = 0.0;	//Power to match the idle status
			}
		}//End below discharge_on_threshold (but above discharge_off_threshold)
		else	//We're in the discharge realm
		{
			new_lf_status = DISCHARGE;		//Above threshold, so discharge into the grid

			//Check battery status
			if ((b_soc > soc_reserve) && (b_soc != -1))
			{
				//See if we were charging
				if (load_follow_status == CHARGE)	//were charging, just turn us off and reiterate
				{
					new_lf_status = IDLE;			//Turn us off first, then re-evaluate
					new_lf_dispatch_power = 0.0;	//Represents the idle
				}
				else
				{
					new_lf_status = DISCHARGE;	//Keep us discharging

					//See if we were already railed - if less, still have room
					if (lf_dispatch_power > -max_discharge_rate)
					{
						//Determine how far off we are from the "on" point
						diff_power_val = discharge_on_threshold - curr_power_val;

						//Accumulate it
						new_lf_dispatch_power = lf_dispatch_power + diff_power_val;

						//Make sure it isn't too big
						if (new_lf_dispatch_power < -max_discharge_rate)
						{
							new_lf_dispatch_power = -max_discharge_rate;	//Desire more than we can give, limit it
						}
					}
					else	//We were railed, continue with this course
					{
						new_lf_dispatch_power = -max_discharge_rate;	//Set to maximum discharge rate
					}
				}
			}
			else	//At or below reserve, go to idle
			{
				gl_verbose("inverter:%s - discharge desired, but not enough battery capacity!",obj->name);
				//Defined above

				new_lf_status = IDLE;			//Can't do anything, so we're idle
				new_lf_dispatch_power = 0.0;	//Turn us off
			}
		}//End above discharge_on_threshold

		//Change checks - see if we need to reiterate
		if (new_lf_status != load_follow_status)
		{
			//Update status
			load_follow_status = new_lf_status;

			//Update dispatch, since it obviously changed
			lf_dispatch_power = new_lf_dispatch_power;

			//Major change, force a reiteration
			t2 = t1;

			//Update lockout allowed
			if (new_lf_status == CHARGE)
			{
				//Hard lockout for now
				lf_dispatch_change_allowed = false;

				//Apply update time - note that this may have issues with TS_NEVER, but unsure if it will be a problem at point (theoretically, the simulation is about to end)
				next_update_time = t1 + ((TIMESTAMP)(charge_lockout_time));
			}
			else if (new_lf_status == DISCHARGE)
			{
				//Hard lockout for now
				lf_dispatch_change_allowed = false;

				//Apply update time - note that this may have issues with TS_NEVER, but unsure if it will be a problem at point (theoretically, the simulation is about to end)
				next_update_time = t1 + ((TIMESTAMP)(discharge_lockout_time));
			}
			//Default else - IDLE has no such restrictions
		}

		//Dispatch change, but same mode
		if (new_lf_dispatch_power != lf_dispatch_power)
		{
			//See if it is appreciable - just in case - I'm arbitrarily declaring milliWatts the threshold
			if (fabs(new_lf_dispatch_power - lf_dispatch_power)>=0.001)
			{
				//Put the new power value in
				lf_dispatch_power = new_lf_dispatch_power;

				//Force a reiteration
				t2 = t1;

				//Update lockout allowed -- This probably needs to be refined so if in deadband, incremental changes can still occur - TO DO
				if (new_lf_status == CHARGE)
				{
					//Hard lockout for now
					lf_dispatch_change_allowed = false;

					//Apply update time
					next_update_time = t1 + ((TIMESTAMP)(charge_lockout_time));
				}
				else if (new_lf_status == DISCHARGE)
				{
					//Hard lockout for now
					lf_dispatch_change_allowed = false;

					//Apply update time
					next_update_time = t1 + ((TIMESTAMP)(discharge_lockout_time));
				}
				//Default else - IDLE has no such restrictions
			}
			//Defaulted else - change is less than 0.001 W, so who cares
		}
		//Default else, do nothing
	}//End LOAD_FOLLOWING redispatch

	//***************************************************************************************
	// Group load-following mode using a (more or less) uncoordinated but decentralized control
	if (inverter_type_v == FOUR_QUADRANT && four_quadrant_control_mode == FQM_GROUP_LF && lf_dispatch_change_allowed)
	{
		//See what the sense_object is and determine if we need to update
		if (sense_is_link)
		{
			//Perform the power update
			((void (*)(OBJECT *))(*powerCalc))(sense_object);
		}//End link update of power

		//Extract power, mainly just for convenience, but also to offset us for what we are currently "ordering"
		temp_complex_value = sense_power->get_complex();
		curr_power_val = temp_complex_value.Re();

		//Check power - only focus on real for now -- this will need to be changed if reactive considered
		if (curr_power_val<=charge_threshold)	//Below charging threshold, desire charging
		{
			//See if the battery has room
			if ((b_soc < 1.0) && (b_soc != -1))
			{
				//See if we were already railed - if less, still have room, or we were discharging (either way okay - just may iterate a couple times)
				if (lf_dispatch_power < max_charge_rate)
				{
					//Make sure we weren't discharging
					if (load_follow_status == DISCHARGE)
					{
						//Set us to idle (will force a reiteration) - who knows where we really are
						new_lf_status = IDLE;
						new_lf_dispatch_power = 0.0;
					}
					else
					{
						//Set us up to charge
						new_lf_status = CHARGE;

						//Scaling charge rate based on rating of inverters in group
						scaling_factor = (charge_threshold - curr_power_val)/(group_max_charge_rate);
						new_lf_dispatch_power = (scaling_factor * max_charge_rate) + lf_dispatch_power;

						//Make sure it isn't too big
						if (new_lf_dispatch_power > max_charge_rate)
						{
							new_lf_dispatch_power = max_charge_rate;	//Desire more than we can give, limit it
						}
					}
				}
				else	//We were railed, continue with this course
				{
					new_lf_status = CHARGE;						//Keep us charging
					new_lf_dispatch_power = max_charge_rate;	//Set to maximum rate
				}

			}//End Battery has room
			else	//Battery full, no charging allowed
			{
				gl_verbose("inverter:%s - charge desired, but battery full!",obj->name);
				/*  TROUBLESHOOT
				An inverter in LOAD_FOLLOWING mode currently wants to charge the battery more, but the battery
				is full.  Consider using a larger battery and trying again.
				*/

				new_lf_status = IDLE;			//Can't do anything, so we're idle
				new_lf_dispatch_power = 0.0;	//Turn us off
			}
		}//End below charge_threshold
		else if (curr_power_val > charge_threshold && curr_power_val < discharge_threshold)	//Above charge threshold and  below discharge threshold.
		{
			new_lf_status = IDLE;
			new_lf_dispatch_power = 0.00; //lf_dispatch_power;
		}
		else if (curr_power_val >= discharge_threshold)	//Above discharge threshold
		{
			new_lf_status = DISCHARGE;		//Above threshold, so discharge into the grid

			//Check battery status
			if ((b_soc > soc_reserve) && (b_soc != -1))
			{
				//See if we were charging
				if (load_follow_status == CHARGE)	//were charging, just turn us off and reiterate
				{
					new_lf_status = IDLE;			//Turn us off first, then re-evaluate
					new_lf_dispatch_power = 0.0;	//Represents the idle
				}
				else
				{
					new_lf_status = DISCHARGE;	//Keep us discharging

					//See if we were already railed - if less, still have room
					if (lf_dispatch_power > -max_discharge_rate)
					{

						//Scaling charge rate based on rating of inverters in group
						scaling_factor = (curr_power_val - discharge_threshold)/(group_max_discharge_rate);
						new_lf_dispatch_power = lf_dispatch_power + (scaling_factor * -max_charge_rate);

						//Make sure it isn't too big
						if (new_lf_dispatch_power < -max_discharge_rate)
						{
							new_lf_dispatch_power = -max_discharge_rate;	//Desire more than we can give, limit it
						}
					}
					else	//We were railed, continue with this course
					{
						new_lf_dispatch_power = -max_discharge_rate;	//Set to maximum discharge rate
					}
				}
			}
			else	//At or below reserve, go to idle
			{
				gl_verbose("inverter:%s - discharge desired, but not enough battery capacity!",obj->name);
				//Defined above

				new_lf_status = IDLE;			//Can't do anything, so we're idle
				new_lf_dispatch_power = 0.0;	//Turn us off
			}
		}//End above discharge_threshold

		//Change checks - see if we need to reiterate
		if (new_lf_status != load_follow_status)
		{
			//Update status
			load_follow_status = new_lf_status;

			//Update dispatch, since it obviously changed
			lf_dispatch_power = new_lf_dispatch_power;

			//Major change, force a reiteration
			t2 = t1;

			//Update lockout allowed
			if (new_lf_status == CHARGE)
			{
				//Hard lockout for now
				lf_dispatch_change_allowed = false;

				//Apply update time - note that this may have issues with TS_NEVER, but unsure if it will be a problem at point (theoretically, the simulation is about to end)
				next_update_time = t1 + ((TIMESTAMP)(charge_lockout_time));
			}
			else if (new_lf_status == DISCHARGE)
			{
				//Hard lockout for now
				lf_dispatch_change_allowed = false;

				//Apply update time - note that this may have issues with TS_NEVER, but unsure if it will be a problem at point (theoretically, the simulation is about to end)
				next_update_time = t1 + ((TIMESTAMP)(discharge_lockout_time));
			}
			//Default else - IDLE has no such restrictions
		}

		//Dispatch change, but same mode
		if (new_lf_dispatch_power != lf_dispatch_power)
		{
			//See if it is appreciable - just in case - I'm artibrarily declaring milliWatts the threshold
			if (fabs(new_lf_dispatch_power - lf_dispatch_power)>=0.001)
			{
				//Put the new power value in
				lf_dispatch_power = new_lf_dispatch_power;

				//Force a reiteration
				t2 = t1;

				//Update lockout allowed -- This probably needs to be refined so if in deadband, incremental changes can still occur - TO DO
				if (new_lf_status == CHARGE)
				{
					//Hard lockout for now
					lf_dispatch_change_allowed = false;

					//Apply update time
					next_update_time = t1 + ((TIMESTAMP)(charge_lockout_time));
				}
				else if (new_lf_status == DISCHARGE)
				{
					//Hard lockout for now
					lf_dispatch_change_allowed = false;

					//Apply update time
					next_update_time = t1 + ((TIMESTAMP)(discharge_lockout_time));
				}
				//Default else - IDLE has no such restrictions
			}
			//Defaulted else - change is less than 0.001 W, so who cares
		}
		//Default else, do nothing
	}// End FQM_GROUP_LF
	else if ((inverter_type_v == FOUR_QUADRANT) && ((four_quadrant_control_mode == FQM_VOLT_VAR) || (four_quadrant_control_mode == FQM_VOLT_WATT)))
	{
		if (four_quadrant_control_mode == FQM_VOLT_VAR)
		{
			if (t1 >= allowed_vv_action && (t1 > last_vv_check))
			{
				vv_operation = false;
				last_vv_check = t1;
				if ((phases & 0x10) == 0x10) {
					if((value_Circuit_V[0].Mag() / V_base) <= V1) {
						power_val[0].SetImag(Q1 * p_rated);
					} else if ((value_Circuit_V[0].Mag() / V_base) <= V2 && V1 < (value_Circuit_V[0].Mag() / V_base)) {
						power_val[0].SetImag(lin_eq_volt((value_Circuit_V[0].Mag() / V_base), m12, b12) * p_rated);
					} else if ((value_Circuit_V[0].Mag() / V_base) <= V3 && V2 < (value_Circuit_V[0].Mag() / V_base)) {
						power_val[0].SetImag(lin_eq_volt((value_Circuit_V[0].Mag() / V_base), m23, b23) * p_rated);
					} else if ((value_Circuit_V[0].Mag() / V_base) <= V4 && V3 < (value_Circuit_V[0].Mag() / V_base)) {
						power_val[0].SetImag(lin_eq_volt((value_Circuit_V[0].Mag() / V_base), m34, b34) * p_rated);
					} else if (V4 < (value_Circuit_V[0].Mag() / V_base)) {
						power_val[0].SetImag(Q4 * p_rated);
					}
					if (last_power->Im() != power_val[0].Im()) {
						vv_operation = true;
					}
					VA_Out = power_val[0]; // TEMc
				} else {
					if ((phases & 0x01) == 0x01) {
						if((value_Circuit_V[0].Mag() / V_base) <= V1) {
							power_val[0].SetImag(Q1 * p_rated);
						} else if ((value_Circuit_V[0].Mag() / V_base) <= V2 && V1 < (value_Circuit_V[0].Mag() / V_base)) {
							power_val[0].SetImag(lin_eq_volt((value_Circuit_V[0].Mag() / V_base), m12, b12) * p_rated);
						} else if ((value_Circuit_V[0].Mag() / V_base) <= V3 && V2 < (value_Circuit_V[0].Mag() / V_base)) {
							power_val[0].SetImag(lin_eq_volt((value_Circuit_V[0].Mag() / V_base), m23, b23) * p_rated);
						} else if ((value_Circuit_V[0].Mag() / V_base) <= V4 && V3 < (value_Circuit_V[0].Mag() / V_base)) {
							power_val[0].SetImag(lin_eq_volt((value_Circuit_V[0].Mag() / V_base), m34, b34) * p_rated);
						} else if (V4 < (value_Circuit_V[0].Mag() / V_base)) {
							power_val[0].SetImag(Q4 * p_rated);
						}
						if (last_power[0].Im() != power_val[0].Im()) {
							vv_operation = true;
						}
					}
					if ((phases & 0x02) == 0x02) {
						if((value_Circuit_V[1].Mag() / V_base) <= V1) {
							power_val[1].SetImag(Q1 * p_rated);
						} else if ((value_Circuit_V[1].Mag() / V_base) <= V2 && V1 < (value_Circuit_V[1].Mag() / V_base)) {
							power_val[1].SetImag(lin_eq_volt((value_Circuit_V[1].Mag() / V_base), m12, b12) * p_rated);
						} else if ((value_Circuit_V[1].Mag() / V_base) <= V3 && V2 < (value_Circuit_V[1].Mag() / V_base)) {
							power_val[1].SetImag(lin_eq_volt((value_Circuit_V[1].Mag() / V_base), m23, b23) * p_rated);
						} else if ((value_Circuit_V[1].Mag() / V_base) <= V4 && V3 < (value_Circuit_V[1].Mag() / V_base)) {
							power_val[1].SetImag(lin_eq_volt((value_Circuit_V[1].Mag() / V_base), m34, b34) * p_rated);
						} else if (V4 < (value_Circuit_V[1].Mag() / V_base)) {
							power_val[1].SetImag(Q4 * p_rated);
						}
						if (last_power[1].Im() != power_val[1].Im()) {
							vv_operation = true;
						}
					}
					if ((phases & 0x04) == 0x04) {
						if((value_Circuit_V[2].Mag() / V_base) <= V1) {
							power_val[2].SetImag(Q1 * p_rated);
						} else if ((value_Circuit_V[2].Mag() / V_base) <= V2 && V1 < (value_Circuit_V[2].Mag() / V_base)) {
							power_val[2].SetImag(lin_eq_volt((value_Circuit_V[2].Mag() / V_base), m12, b12) * p_rated);
						} else if ((value_Circuit_V[2].Mag() / V_base) <= V3 && V2 < (value_Circuit_V[2].Mag() / V_base)) {
							power_val[2].SetImag(lin_eq_volt((value_Circuit_V[2].Mag() / V_base), m23, b23) * p_rated);
						} else if ((value_Circuit_V[2].Mag() / V_base) <= V4 && V3 < (value_Circuit_V[2].Mag() / V_base)) {
							power_val[2].SetImag(lin_eq_volt((value_Circuit_V[2].Mag() / V_base), m34, b34) * p_rated);
						} else if (V4 < (value_Circuit_V[2].Mag() / V_base)) {
							power_val[2].SetImag(Q4 * p_rated);
						}
						if (last_power[2].Im() != power_val[2].Im()) {
							vv_operation = true;
						}
					}
					VA_Out = power_val[0] + power_val[1] + power_val[2]; // TEMc
				}
				if (vv_operation) {
					t2 = t1;
					allowed_vv_action = (TIMESTAMP)floor((double)t1 + vv_lockout + 0.5);
				}
			}
		} 
		else if (four_quadrant_control_mode == FQM_VOLT_WATT)
		{
			if (t1 >= allowed_vv_action && (t1 > last_vv_check))
			{
				vv_operation = false;
				last_vv_check = t1;
				double vpu;
				pa_vw_limited = pb_vw_limited = pc_vw_limited = p_rated;
				if ((phases & 0x10) == 0x10) {
					vpu = value_Circuit_V[0].Mag() / V_base;
					if (vpu > VW_V1) {
						pa_vw_limited = p_rated * (1.0 + VW_m * (vpu - VW_V1));
						if (pa_vw_limited < 0.0) pa_vw_limited = 0.0;
						if (power_val[0].Re() > pa_vw_limited) power_val[0].SetReal (pa_vw_limited); // TODO - curtail power absorption for batteries?
					}
					if (last_power->Re() != power_val[0].Re()) {
						vv_operation = true;
					}
					VA_Out = power_val[0];
				} else {
					if ((phases & 0x01) == 0x01) {
						vpu = value_Circuit_V[0].Mag() / V_base;
						if (vpu > VW_V1) {
							pa_vw_limited = p_rated * (1.0 + VW_m * (vpu - VW_V1));
							if (pa_vw_limited < 0.0) pa_vw_limited = 0.0;
							if (power_val[0].Re() > pa_vw_limited) power_val[0].SetReal (pa_vw_limited);
						}
						if (last_power[0].Re() != power_val[0].Re()) {
							vv_operation = true;
						}
					}
					if ((phases & 0x02) == 0x02) {
						vpu = value_Circuit_V[1].Mag() / V_base;
						if (vpu > VW_V1) {
							pb_vw_limited = p_rated * (1.0 + VW_m * (vpu - VW_V1));
							if (pb_vw_limited < 0.0) pb_vw_limited = 0.0;
							if (power_val[1].Re() > pb_vw_limited) power_val[1].SetReal (pb_vw_limited);
						}
						if (last_power[1].Re() != power_val[1].Re()) {
							vv_operation = true;
						}
					}
					if ((phases & 0x04) == 0x04) {
						vpu = value_Circuit_V[2].Mag() / V_base;
						if (vpu > VW_V1) {
							pc_vw_limited = p_rated * (1.0 + VW_m * (vpu - VW_V1));
							if (pc_vw_limited < 0.0) pc_vw_limited = 0.0;
							if (power_val[2].Re() > pc_vw_limited) power_val[2].SetReal (pc_vw_limited);
						}
						if (last_power[2].Re() != power_val[2].Re()) {
							vv_operation = true;
						}
					}
					VA_Out = power_val[0] + power_val[1] + power_val[2];
				}
				if (vv_operation) {
					t2 = t1;
					allowed_vv_action = (TIMESTAMP)floor((double)t1 + vv_lockout + 0.5);
				}
			}
		}
	}

	//***************************************************************************************
	//Power-factor regulation, modifying VA_Out to use any available VAs for power-factor regulation
	if ((inverter_type_v == FOUR_QUADRANT) && (pf_reg != EXCLUDED) && pf_reg_dispatch_change_allowed)
	{
		//Need to modify dispatch from load following and/or adjust the reactive power output of the inverter to meet power factor regulation needs.
		//See what the sense_object is and determine if we need to update
		if (sense_is_link)
		{
			//Perform the power update
			((void (*)(OBJECT *))(*powerCalc))(sense_object);
		}//End link update of power

		//Extract power to calculate the necessary reactive power output and offset us for what we are currently "ordering"
		temp_complex_value = sense_power->get_complex();
		curr_real_power_val = temp_complex_value.Re();
		curr_reactive_power_val = temp_complex_value.Im();
		curr_pf = fabs(curr_real_power_val)/sqrt((curr_reactive_power_val*curr_reactive_power_val)+(curr_real_power_val * curr_real_power_val));

		if (pf_reg == INCLUDED)
		{
			//Check pf
			if ( (curr_pf <= pf_reg_activate) ||
				((curr_pf >= pf_reg_activate) && (curr_pf <= pf_reg_deactivate) && (pf_reg_status == REGULATING)) ||
				((curr_reactive_power_val < 0) && (curr_pf < 0.98)) ) //Only worrying about regulating leading power factor if it is "excessive".
			{
				//See if we were already at max VA - if less, still have room to add some VARs
				//Regardless of which four-quadrant control mode, VA_Out is the output of the inverter
				if ((inverter_type_v == FOUR_QUADRANT) && (VA_Out.Mag() <= p_max))
				{
					new_pf_reg_status = REGULATING;

					//Calculate the required VARs to bring pf back up to acceptable value

					if (curr_reactive_power_val < 0)
					{
						if (four_quadrant_control_mode == FQM_GROUP_LF) //operating in group mode, PF regulation should also operate in group mode.
						{
							if ((pf_reg_dispatch_VAR * group_rated_power/p_max) > fabs(curr_reactive_power_val))
							{
								Q_target = - (curr_reactive_power_val - pf_reg_dispatch_VAR);
							}
							else
							{
								Q_target = - (curr_reactive_power_val - (pf_reg_dispatch_VAR * group_rated_power/p_max));
							}

						}
						else
						{
							Q_target = - (curr_reactive_power_val - pf_reg_dispatch_VAR);
						}

					}
					else
					{
						Q_target = - (curr_reactive_power_val - curr_real_power_val * tan(acos(pf_reg_deactivate)));
					}


					if (four_quadrant_control_mode == FQM_GROUP_LF) //operating in group mode, PF regulation should also operate in group mode.
					{
						scaling_factor = (Q_target - curr_reactive_power_val)/group_rated_power;
						Q_out = (scaling_factor * p_max) + pf_reg_dispatch_VAR;
					}
					else
					{
						Q_out = Q_target;
					}

					//Calculate the VARs that can be dispatched without violating the inverter's VA limit, p_max.
					Q_available = sqrt((p_max*p_max) - (VA_Out.Re()*VA_Out.Re()));

					//Only output the reactive power that is needed (if we have it).
					if (fabs(Q_out) <= fabs(Q_available))
					{
						new_pf_reg_distpatch_VAR = Q_out;
					}
					else
					{
						if (Q_out < 0)
						{
							new_pf_reg_distpatch_VAR = -Q_available;
						}
						else
						{
							new_pf_reg_distpatch_VAR = Q_available;
						}
					}
				}
				else	//We were railed, continue with this course
				{
					new_pf_reg_status = REGULATING;
					new_pf_reg_distpatch_VAR = -VA_Out.Im(); //Injecting negative reactive power.
				}
			}//End pf_reg_activate
			else if (curr_pf >= pf_reg_deactivate)	// Don't change output since we are above our deactivate limit.
													// Changing the output of the inverter will alter the sensed pf
													// and may cause us to drop below the activate limit,
													// potentially beginning control oscillation
			{
				new_pf_reg_status = IDLING;
				new_pf_reg_distpatch_VAR = pf_reg_dispatch_VAR;
			}
			else //Nothing else going on, idling
			{
				new_pf_reg_status = IDLING;
				new_pf_reg_distpatch_VAR = pf_reg_dispatch_VAR;
			}//End hysteresis implementation
		}//End INCLUDED mode
		else if (pf_reg == INCLUDED_ALT)	//INCLUDED_ALT Mode
		{
			//Init Q_target, for giggles (use old value)
			Q_target = pf_reg_dispatch_VAR;

			//See if we have room to spare
			if (VA_Out.Mag() <= p_max)
			{
				//"Sign" the current power factor, based on reactive power.  Technically not a proper pf, but what this algorithm uses
				if (curr_reactive_power_val < 0.0)
				{
					curr_pf = -1.0 * curr_pf;
				}

				//De-sign the real power
				if (curr_real_power_val < 0.0)
				{
					curr_real_power_val = -1.0 * curr_real_power_val;
				}

				//See which region we're in - check our overall "desired" first
				if (pf_target_var < 0)	//Capacitive
				{
					//See which band we're in
					if (pf_reg_low < 0.0)	//Capacitive "switching" limit
					{
						if (pf_reg_high < 0.0)	//Capacitive "on" value too
						{
							//See which band we're in
							if (((curr_pf < 0.0) && (curr_pf <= pf_reg_high)) || (curr_pf > 0.0))	//More negative, so we should be in "recovery" mode
							{
								//Set us up as regulating
								new_pf_reg_status = REGULATING;

								//Figure out the offset to get us back into the acceptable capacitance range
								Q_target = -(curr_reactive_power_val - (curr_real_power_val * tan(acos(pf_reg_high)))) + pf_reg_dispatch_VAR;
							}
							else if (curr_pf <= pf_reg_low)	//In the deadband
							{
								//Set us up as non-regulating
								new_pf_reg_status = IDLING;

								//Copy the old value in, to prevent us from changing
								Q_target = pf_reg_dispatch_VAR;
							}
							else	//Regulating band
							{
								//Flag as regulating
								new_pf_reg_status = REGULATING;

								//Figure out the offset to get us back into the acceptable capacitance range
								Q_target = -(curr_reactive_power_val - (curr_real_power_val * tan(acos(pf_target_var)))) + pf_reg_dispatch_VAR;
							}
						}
						else	//High value is inductive
						{
							if ((curr_pf > 0.0) && (curr_pf <= pf_reg_high))	//Below other size, so should be in "recovery" mode
							{
								//Flag as regulating
								new_pf_reg_status = REGULATING;

								//Figure out the offset to get us back into the acceptable capacitance range
								Q_target = -(curr_reactive_power_val - (curr_real_power_val * tan(acos(pf_target_var)))) + pf_reg_dispatch_VAR;
							}
							else if (((curr_pf < 0.0) && (curr_pf <= pf_reg_low)) || ((curr_pf > 0.0) && (curr_pf >= pf_reg_high)))	//In the deadband
							{
								//Set us up as non-regulating
								new_pf_reg_status = IDLING;

								//Copy the old value in, to prevent us from changing
								Q_target = pf_reg_dispatch_VAR;
							}
							else	//Regulating band
							{
								//Flag as regulating
								new_pf_reg_status = REGULATING;

								//Figure out the offset to get us back into the acceptable capacitance range
								Q_target = -(curr_reactive_power_val - (curr_real_power_val * tan(acos(pf_target_var)))) + pf_reg_dispatch_VAR;
							}
						}//End high value is inductive
					}//End switching limit is inductive
					else	//Switching value is inductive
					{
						if ((curr_pf > 0.0) && (curr_pf <= pf_reg_high))	//Inside the recovery zone
						{
							//Flag as regulating
							new_pf_reg_status = REGULATING;

							//Figure out the offset to get us back into the acceptable capacitance range
							Q_target = -(curr_reactive_power_val - (curr_real_power_val * tan(acos(pf_target_var)))) + pf_reg_dispatch_VAR;
						}
						else if ((curr_pf > 0.0) && (curr_pf <= pf_reg_low))	//Deadband
						{
							//Set us up as non-regulating
							new_pf_reg_status = IDLING;

							//Copy the old value in, to prevent us from changing
							Q_target = pf_reg_dispatch_VAR;
						}
						else	//Regulating band
						{
							//Flag as regulating
							new_pf_reg_status = REGULATING;

							//Different around 1?  Doesn't seem to be
							//Figure out the offset to get us back into the acceptable capacitance range
							Q_target = -(curr_reactive_power_val - (curr_real_power_val * tan(acos(pf_target_var)))) + pf_reg_dispatch_VAR;
						}
					}//End switching limit is low
				}//End capacitive pf logic
				else	//Inductive, by elimination
				{
					if (pf_reg_low < 0.0)	//Capacitive switching point
					{
						if ((curr_pf < 0.0) && (curr_pf >= pf_reg_high))	//Recovery mode
						{
							//Flag as regulating
							new_pf_reg_status = REGULATING;

							//Figure out the offset to get us back into the acceptable capacitance range
							Q_target = -(curr_reactive_power_val - (curr_real_power_val * tan(acos(pf_target_var)))) + pf_reg_dispatch_VAR;
						}
						else if ((curr_pf < 0.0) && (curr_pf >= pf_reg_low))	//Deadband
						{
							//Set us up as non-regulating
							new_pf_reg_status = IDLING;

							//Copy the old value in, to prevent us from changing
							Q_target = pf_reg_dispatch_VAR;
						}
						else	//Regulating band
						{
							//Flag as regulating
							new_pf_reg_status = REGULATING;

							//Being centered around 1 doesn't seem to matter
							//Figure out the offset to get us back into the acceptable capacitance range
							Q_target = -(curr_reactive_power_val - (curr_real_power_val * tan(acos(pf_target_var)))) + pf_reg_dispatch_VAR;
						}
					}//Capactive switching point
					else	//Inductive, by default
					{
						if (pf_reg_high < 0.0)	//Capacitive high switching point
						{
							if ((curr_pf < 0.0) && (curr_pf >= pf_reg_high))	//Recovery mode
							{
								//Flag as regulating
								new_pf_reg_status = REGULATING;

								//Figure out the offset to get us back into the acceptable capacitance range
								Q_target = -(curr_reactive_power_val - (curr_real_power_val * tan(acos(pf_target_var)))) + pf_reg_dispatch_VAR;
							}
							else if (((curr_pf < 0.0) && (curr_pf <= pf_reg_high)) || ((curr_pf > 0.0) && (curr_pf >= pf_reg_low)))	//Deadband
							{
								//Set us up as non-regulating
								new_pf_reg_status = IDLING;

								//Copy the old value in, to prevent us from changing
								Q_target = pf_reg_dispatch_VAR;
							}
							else	//Regulating band
							{
								//Flag as regulating
								new_pf_reg_status = REGULATING;

								//Figure out the offset to get us back into the acceptable capacitance range
								Q_target = -(curr_reactive_power_val - (curr_real_power_val * tan(acos(pf_target_var)))) + pf_reg_dispatch_VAR;
							}
						}//End inductive set, inductive switch, but capacitive recovery
						else	//Inductive as well
						{
							if ((curr_pf < 0.0) || ((curr_pf > 0.0) && (curr_pf >= pf_reg_high)))	//Recovery mode
							{
								//Flag as regulating
								new_pf_reg_status = REGULATING;

								//Figure out the offset to get us back into the acceptable capacitance range
								Q_target = -(curr_reactive_power_val - (curr_real_power_val * tan(acos(pf_target_var)))) + pf_reg_dispatch_VAR;
							}
							else if (curr_pf >= pf_reg_low)	//Deadband
							{
								//Set us up as non-regulating
								new_pf_reg_status = IDLING;

								//Copy the old value in, to prevent us from changing
								Q_target = pf_reg_dispatch_VAR;
							}
							else	//Regulating band
							{
								//Flag as regulating
								new_pf_reg_status = REGULATING;

								//Figure out the offset to get us back into the acceptable capacitance range
								Q_target = -(curr_reactive_power_val - (curr_real_power_val * tan(acos(pf_target_var)))) + pf_reg_dispatch_VAR;
							}
						}//End inductive high switching point
					}//End inductive target and inductive switching
				}//End inductive target
			}//End room to do reactive
			else	//No room, zero and idle us
			{
				new_pf_reg_status = IDLING;
				new_pf_reg_distpatch_VAR = 0.0;
			}//No room
		}//End INCLUDED_ALT

		//Checks due to change in operation - see if we need to reiterate
		//Mode change
		if (new_pf_reg_status != pf_reg_status)
		{
			//Update status
			pf_reg_status = new_pf_reg_status;

			//Update dispatch, since it obviously changed
			pf_reg_dispatch_VAR = new_pf_reg_distpatch_VAR;

			//Major change, force a reiteration
			t2 = t1;

			//Update lockout allowed
			if (new_pf_reg_status == REGULATING)
			{
				//Hard lockout for now
				pf_reg_dispatch_change_allowed = false;

				//Apply update time - note that this may have issues with TS_NEVER, but unsure if it will be a problem at point (theoretically, the simulation is about to end)
				pf_reg_next_update_time = t1 + ((TIMESTAMP)(pf_reg_activate_lockout_time));
			}
		}

		//Same mode, new dispatch
		if (new_pf_reg_distpatch_VAR != pf_reg_dispatch_VAR)
		{
			//See if it is appreciable - just in case - I'm artibrarily declaring milliVARs the threshold
			if (fabs(new_pf_reg_distpatch_VAR - pf_reg_dispatch_VAR)>=0.001)
			{
				//Put the new power value in
				pf_reg_dispatch_VAR = new_pf_reg_distpatch_VAR;

				//Force a reiteration
				t2 = t1;

				//Update lockout allowed -- This probably needs to be refined so if in deadband, incremental changes can still occur - TO DO
				if (new_pf_reg_status == REGULATING)
				{
					//Hard lockout for now
					pf_reg_dispatch_change_allowed = false;

					//Apply update time
					pf_reg_next_update_time = t1 + ((TIMESTAMP)(pf_reg_activate_lockout_time));
				}
				//Default else - IDLE has no such restrictions
			}
		}
	} //End power-factor regulation

	if (inverter_type_v != FOUR_QUADRANT)	//Remove contributions for XML properness
	{
		if ((phases & 0x10) == 0x10)	//Triplex
		{
			value_Line12 = -last_current[3];	//Remove from current12
		}
		else	//Some variant of three-phase
		{
			//Remove our parent contributions (so XMLs look proper)
			value_Line_I[0] = -last_current[0];
			value_Line_I[1] = -last_current[1];
			value_Line_I[2] = -last_current[2];
		}
	}
	else	//Must be four quadrant (load_following or otherwise)
	{
		if ((four_quadrant_control_mode != FQM_VOLT_VAR) && (four_quadrant_control_mode != FQM_VSI) && (four_quadrant_control_mode != FQM_VOLT_WATT)) {
			if ((phases & 0x10) == 0x10)	//Triplex
			{
				if (deltamode_inclusive)
				{
					value_Line_unrotI[0] = -last_current[3];
				}
				else
				{
					value_Power12 = -last_power[3];	//Theoretically pPower is mapped to power_12, which already has the [2] offset applied
				}
			}
			else	//Variation of three-phase
			{
				if (deltamode_inclusive)
				{
					value_Line_unrotI[0] = -last_current[0];
					value_Line_unrotI[1] = -last_current[1];
					value_Line_unrotI[2] = -last_current[2];
				}
				else
				{
					value_Power[0] = -last_power[0];
					value_Power[1] = -last_power[1];
					value_Power[2] = -last_power[2];
				}
			}
		}
		//FQM_VSI mode doesn't need to "subtract post", since it is hidden from the XML
		else if (four_quadrant_control_mode == FQM_VSI) {

			// Update power values based on measured terminal voltage and currents
			if ((phases & 0x10) == 0x10) // split phase
			{
				//Update output power
				//Get current injected
				temp_current_val[0] = value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0];

				//Update power output variables, just so we can see what is going on
				VA_Out = value_Circuit_V[0]*~temp_current_val[0];

			}
			else	//Three phase variant
			{
				//Update output power
				//Get current injected
				temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0] - generator_admittance[0][1]*value_Circuit_V[1] - generator_admittance[0][2]*value_Circuit_V[2]);
				temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0]*value_Circuit_V[0] - generator_admittance[1][1]*value_Circuit_V[1] - generator_admittance[1][2]*value_Circuit_V[2]);
				temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0]*value_Circuit_V[0] - generator_admittance[2][1]*value_Circuit_V[1] - generator_admittance[2][2]*value_Circuit_V[2]);

				//Update power output variables, just so we can see what is going on
				power_val[0] = value_Circuit_V[0]*~temp_current_val[0];
				power_val[1] = value_Circuit_V[1]*~temp_current_val[1];
				power_val[2] = value_Circuit_V[2]*~temp_current_val[2];

				VA_Out = power_val[0] + power_val[1] + power_val[2];
			}

			if (first_run)	//Final init items - namely deltamode supersecond exciter
			{
				// Only update after the first iteration of the power flow (VA_Out != 0.0 + j0.0)
				if (value_IGenerated[0] != gld::complex(0.0,0.0)) {
					if ((attached_bus_type == 2) && (VSI_mode == VSI_DROOP)) {
						P_Out = VA_Out.Re();
						Q_Out = VA_Out.Im();
						first_run = false;
					}
					else {
						first_run = false;
					}

					if (!VSI_esource_init) {
						VSI_esource_init = true; // Finish initializing the VSI e_source after first power flow solutions
					}
				}
			}
			else {
				// Check if VA_Out changes a lot
			    if (fabs(VA_Out_past.Mag() - VA_Out.Mag()) > 1000) {
			    	schedule_deltamode_start(t0);
			    }
			}

		    VA_Out_past = VA_Out;
		}

	}

	//Sync the powerflow variables
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values();
	}
	
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Preupdate
STATUS inverter::pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time)
{
	STATUS stat_val;
	FUNCTIONADDR funadd = nullptr;
	OBJECT *hdr = OBJECTHDR(this);

	//See which method we are
	if (inverter_dyn_mode == PI_CONTROLLER)
	{
		//Call the PI init, since we're here
		stat_val = init_PI_dynamics(&curr_state);
	}
	else if (inverter_dyn_mode == PID_CONTROLLER)
	{
		//Call the init for the PID too, just because
		stat_val = init_PID_dynamics();	//This could probably be move
	}
	else	//Default else, assume none
	{
		stat_val = SUCCESS;
	}

	if (stat_val != SUCCESS)
	{
		gl_error("Inverter failed pre_deltaupdate call");
		/*  TROUBLESHOOT
		While attempting to call the pre_deltaupdate portion of the inverter code, an error
		was encountered.  Please submit your code and a bug report via the ticketing system.
		*/

		return FAILED;
	}

	if (four_quadrant_control_mode == FQM_VSI)
	{
		//If we're a voltage-source inverter, also swap our SWING bus, just because
		//map the function
		funadd = (FUNCTIONADDR)(gl_get_function(hdr->parent,"pwr_object_swing_swapper"));

		//make sure it worked
		if (funadd==nullptr)
		{
			gl_error("inverter:%s -- Failed to find node swing swapper function",(hdr->name ? hdr->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to map the function to change the swing status of the parent bus, the function could not be found.
			Ensure the inverter is actually attached to something.  If the error persists, please submit your code and a bug report
			via the ticketing/issues system.
			*/

			return FAILED;
		}

		//Call the swap
		stat_val = ((STATUS (*)(OBJECT *, bool))(*funadd))(hdr->parent,false);

		if (stat_val == 0)	//Failed :(
		{
			gl_error("Failed to swap SWING status of node:%s on inverter:%s",(hdr->parent->name ? hdr->parent->name : "Unnamed"),(hdr->name ? hdr->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
			failed to execute properly.  If the problem persists, please submit a bug report and your code to the trac website.
			*/

			return FAILED;
		}
	}

	//Just return a pass - not sure how we'd fail
	return SUCCESS;
}

//Module-level call
SIMULATIONMODE inverter::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	double deltat, deltath;
	int indexval;
	gld::complex derror[3];
	gld::complex pid_out[3];
	double temp_val_d, temp_val_q;
	gld::complex work_power_vals;
	double power_diff_val;
	double prev_error_ed;
	double prev_error_eq;
	bool deltaConverged = false;
	bool ramp_change;
	int i;

	double inputPower;
	gld_wlock *test_rlock = nullptr;

	OBJECT *obj = OBJECTHDR(this);

	SIMULATIONMODE simmode_return_value = SM_EVENT;

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		reset_complex_powerflow_accumulators();

		pull_complex_powerflow_values();
	}

	//Get timestep value
	deltat = (double)dt/(double)DT_SECOND;
	deltath = deltat/2.0;

	if (prev_time_dbl != gl_globaldeltaclock)	//Only update timestamp tracker when different - may happen elsewhere (VSI)
	{
		//Update tracking variable
		prev_time_dbl = gl_globaldeltaclock;
	}
	
	//Perform the 1547 update, if enabled
	if (enable_1547_compliance && (iteration_count_val == 0))	//Always just do on the first pass
	{
		//Do the checks
		ieee_1547_double = perform_1547_checks(deltat);
	}

	//See if we are actually enabled
	if (inverter_1547_status)
	{
		//See which mode we're in
		if (inverter_dyn_mode == PI_CONTROLLER)
		{
			if (inverter_type_v == FOUR_QUADRANT && four_quadrant_control_mode == FQM_VSI) {

				// Check tie delay values given for VSI droop settings
				if ((delta_time==0) && (iteration_count_val==0))	//First run of new delta call
				{
					if (Tp_delay == 0 || Tp_delay < deltat) {
						Tp_delay = deltat;
					}

					if (Tq_delay == 0 || Tq_delay < deltat) {
						Tq_delay = deltat;
					}
				}

				// Check pass
				if (iteration_count_val==0)	// Predictor pass
				{
					// Caluclate injection current based on voltage source magnitude and angle obtained
					if((phases & 0x10) == 0x10)
					{
						//Update output power
						//Get current injected
						temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0]);

						//Update power output variables, just so we can see what is going on
						power_val[0] = value_Circuit_V[0]*~temp_current_val[0];

						VA_Out = power_val[0];
						pCircuit_V_Avg = value_Circuit_V[0].Mag();  //updates the average value of pCircuit_V_Avg

						// Calculate power differences between true power outputs,and the measured delayed power outputs of last delta time step (not last iteration)
						curr_state.dp_mea_delayed = 1.0/Tp_delay*(VA_Out.Re() - curr_state.p_mea_delayed);
						curr_state.dq_mea_delayed = 1.0/Tq_delay*(VA_Out.Im() - curr_state.q_mea_delayed);

						// Calculate the measured delayed power output in this iteration
						pred_state.p_mea_delayed = curr_state.p_mea_delayed + (deltat * curr_state.dp_mea_delayed);
						pred_state.q_mea_delayed = curr_state.q_mea_delayed + (deltat * curr_state.dq_mea_delayed);

						// VSI isochronous mode keeps the voltage angle constant always
						if (VSI_mode == VSI_ISOCHRONOUS) {
							// If it is an isochronous VSI
							pred_state.dV_StateVal[0] = (V_mag_ref[0] - value_Circuit_V[0].Mag()) * ki_Vterminal;
							pred_state.V_StateVal[0] = curr_state.V_StateVal[0] + pred_state.dV_StateVal[0] * deltat;
							pred_state.e_source_mag[0] = pred_state.V_StateVal[0] + pred_state.dV_StateVal[0] * kp_Vterminal / ki_Vterminal;
							e_source[0] = gld::complex(pred_state.e_source_mag[0] * cos(V_angle[0]), pred_state.e_source_mag[0] * sin(V_angle[0]));

							// Thevenin voltage source to Norton current source conversion
							value_IGenerated[0] = e_source[0]/(gld::complex(Rfilter,Xfilter) * Zbase);
						}

						// VSI droop mode updates its e_source value based on PQ_Out and droop curve
						else if (VSI_mode == VSI_DROOP) {

							// Obtain the changes of frequency
							double delta_f = (curr_state.p_mea_delayed - Pref) /p_rated * (R_fp / (2.0 * PI));



							pred_state.dfmax_ini_StateVal = (Pmax - curr_state.p_mea_delayed /p_rated) * kipmax / (2.0 * PI);
							pred_state.fmax_ini_StateVal = curr_state.fmax_ini_StateVal + pred_state.dfmax_ini_StateVal * deltat;

                            // 0 Limiter for integrator of Pmax controller
							if (pred_state.fmax_ini_StateVal > 0.0){

								pred_state.fmax_ini_StateVal = 0.0;

							}

                            // Lower Limiter for integrator of Pmax controller
							if (pred_state.fmax_ini_StateVal < Pmax_Low_Limit){

								pred_state.fmax_ini_StateVal = Pmax_Low_Limit;

							}

							pred_state.fmax_StateVal = pred_state.fmax_ini_StateVal + pred_state.dfmax_ini_StateVal * kppmax / (2.0 * PI) / kipmax;

							// 0 limiter for Pmax controller
							if (pred_state.fmax_StateVal > 0.0){

								pred_state.fmax_StateVal = 0.0;

							}


                            // Lower limiter for Pmax controller
							if (pred_state.fmax_StateVal < Pmax_Low_Limit){

								pred_state.fmax_StateVal = Pmax_Low_Limit;

							}


							// Pmin controller
							pred_state.dfmin_ini_StateVal = (Pmin - curr_state.p_mea_delayed / p_rated) * kipmax / (2.0 * PI);
							pred_state.fmin_ini_StateVal = curr_state.fmin_ini_StateVal + pred_state.dfmin_ini_StateVal * deltat;

                            // 0 Limiter for integrator of Pmin controller
							if (pred_state.fmin_ini_StateVal < 0.0){

								pred_state.fmin_ini_StateVal = 0.0;

							}

                            // 0 limiter for Pmin controller
							pred_state.fmin_StateVal = pred_state.fmin_ini_StateVal + pred_state.dfmin_ini_StateVal * kppmax / (2.0 * PI) / kipmax;

							if (pred_state.fmin_StateVal < 0.0){

								pred_state.fmin_StateVal = 0.0;

							}

							VSI_freq = freq_ref - delta_f + pred_state.fmax_StateVal + pred_state.fmin_StateVal;



							// Calculate voltage source magnitude based on the droop curve
							V_mag[0] = V_mag_ref[0] - (pred_state.q_mea_delayed - Qref) / p_rated * R_vq * node_nominal_voltage;

							// PI regulator of Q-V droop
							pred_state.dV_StateVal[0] = (V_mag[0] - value_Circuit_V[0].Mag()) * ki_Vterminal;
							pred_state.V_StateVal[0] = curr_state.V_StateVal[0] + pred_state.dV_StateVal[0] * deltat;
							pred_state.e_source_mag[0] = pred_state.V_StateVal[0] + pred_state.dV_StateVal[0] * kp_Vterminal / ki_Vterminal;

							// Calculate voltage source angle based on the droop curve, it should be noted that phase angle will be calculated again in corrector pass, the value is accumulated
							V_angle[0] = V_angle[0] - (delta_f - pred_state.fmax_StateVal - pred_state.fmin_StateVal)* 2.0 * PI * deltat;

							e_source[0] = gld::complex(pred_state.e_source_mag[0] * cos(V_angle[0]), pred_state.e_source_mag[0] * sin(V_angle[0])); // The voltage loop only regulates the magnitude of esrouceA, esourceB=esourceC=esourceA

							// Thevenin voltage source to Norton current source convertion
							value_IGenerated[0] = e_source[0]/(gld::complex(Rfilter,Xfilter) * Zbase);
						}
					}//End triplex
					else if((phases & 0x07) == 0x07)
					{
						//Update output power
						//Get current injected
						temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0] - generator_admittance[0][1]*value_Circuit_V[1] - generator_admittance[0][2]*value_Circuit_V[2]);
						temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0]*value_Circuit_V[0] - generator_admittance[1][1]*value_Circuit_V[1] - generator_admittance[1][2]*value_Circuit_V[2]);
						temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0]*value_Circuit_V[0] - generator_admittance[2][1]*value_Circuit_V[1] - generator_admittance[2][2]*value_Circuit_V[2]);

						//Update power output variables, just so we can see what is going on
						power_val[0] = value_Circuit_V[0]*~temp_current_val[0];
						power_val[1] = value_Circuit_V[1]*~temp_current_val[1];
						power_val[2] = value_Circuit_V[2]*~temp_current_val[2];

						VA_Out = power_val[0] + power_val[1] + power_val[2];
						pCircuit_V_Avg = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag() ) / 3.0;  //updates the average value of pCircuit_V_Avg

						// Calculate power differences between true power outputs,and the measured delayed power outputs of last delta time step (not last iteration)
						curr_state.dp_mea_delayed = 1.0/Tp_delay*(VA_Out.Re() - curr_state.p_mea_delayed);
						curr_state.dq_mea_delayed = 1.0/Tq_delay*(VA_Out.Im() - curr_state.q_mea_delayed);

						// Calculate the measured delayed power output in this iteration
						pred_state.p_mea_delayed = curr_state.p_mea_delayed + (deltat * curr_state.dp_mea_delayed);
						pred_state.q_mea_delayed = curr_state.q_mea_delayed + (deltat * curr_state.dq_mea_delayed);

						// VSI isochronous mode keeps the voltage angle constant always
						if (VSI_mode == VSI_ISOCHRONOUS) {
							// If it is an isochronous VSI
							for(i = 0; i < 3; i++) {
								pred_state.dV_StateVal[i] = (V_mag_ref[i] - value_Circuit_V[i].Mag()) * ki_Vterminal;
								pred_state.V_StateVal[i] = curr_state.V_StateVal[i] + pred_state.dV_StateVal[i] * deltat;
								pred_state.e_source_mag[i] = pred_state.V_StateVal[i] + pred_state.dV_StateVal[i] * kp_Vterminal / ki_Vterminal;
								e_source[i] = gld::complex(pred_state.e_source_mag[i] * cos(V_angle[i]), pred_state.e_source_mag[i] * sin(V_angle[i]));

								// Thevenin voltage source to Norton current source conversion
								value_IGenerated[i] = e_source[i]/(gld::complex(Rfilter,Xfilter) * Zbase);

								//See how this aligns with the real and reactive power ramp rate, if necessary
								if (checkRampRate_real || checkRampRate_reactive)
								{
									//Deflag
									ramp_change = false;

									//See what the power out is for this "new" state
									temp_current_val[i] = (value_IGenerated[i] - generator_admittance[i][0]*value_Circuit_V[0] - generator_admittance[i][1]*value_Circuit_V[1] - generator_admittance[i][2]*value_Circuit_V[2]);

									//Update power output variables, just so we can see what is going on
									power_val[i] = value_Circuit_V[i]*~temp_current_val[i];

									//See which way we are
									if (checkRampRate_real) {

										//Compute the difference - real part
										power_diff_val = (power_val[i].Re() - prev_VA_out[i].Re()) / deltat;

										if (power_val[i].Re() > prev_VA_out[i].Re())	//Ramp up
										{
											//See if it was too big
											if (power_diff_val > rampUpRate_real)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetReal(prev_VA_out[i].Re() + (rampUpRate_real * deltat));
											}
											//Default else - was okay
										}
										else	//Ramp down
										{
											//See if it was too big
											if (power_diff_val < -rampDownRate_real)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetReal(prev_VA_out[i].Re() - (rampDownRate_real * deltat));
											}
											//Default else - was okay
										}
									}
									if (checkRampRate_reactive) {

										//Compute the difference - reactive part
										power_diff_val = (power_val[i].Im() - prev_VA_out[i].Im()) / deltat;

										if (power_val[i].Im() > prev_VA_out[i].Im())	//Ramp up
										{
											//See if it was too big
											if (power_diff_val > rampUpRate_reactive)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetImag(prev_VA_out[i].Im() + (rampUpRate_reactive * deltat));
											}
											//Default else - was okay
										}
										else	//Ramp down
										{
											//See if it was too big
											if (power_diff_val < -rampDownRate_reactive)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetImag(prev_VA_out[i].Im() - (rampDownRate_reactive * deltat));
											}
											//Default else - was okay
										}
									}

									//Now "extrapolate" this back to a current value, if needed
									if (ramp_change)
									{
										//Compute a "new current" value
										if (value_Circuit_V[i].Mag() > 0.0)
										{
											temp_current_val[i] = ~(power_val[i] / value_Circuit_V[i]);
										}
										else
										{
											temp_current_val[i] = gld::complex(0.0,0.0);
										}

										//Adjust it to IGenerated
										value_IGenerated[i] = temp_current_val[i] + generator_admittance[i][0]*value_Circuit_V[0] + generator_admittance[i][1]*value_Circuit_V[1] + generator_admittance[i][2]*value_Circuit_V[2];

										//And adjust the related "internal voltage" - this just broke the frequency too
										e_source[i] = value_IGenerated[i] * (gld::complex(Rfilter,Xfilter) * Zbase);

										//Other state variables needed to be updated?
									}
									//Default else - no ramp change, so don't mess with anything

									//Store the updated power value
									curr_VA_out[i] = power_val[i];
								}//Ramp rate check active and reactive
							}
						}

						// VSI droop mode updates its e_source value based on PQ_Out and droop curve
						else if (VSI_mode == VSI_DROOP) {

							// Obtain the changes of frequency
							double delta_f = (curr_state.p_mea_delayed - Pref) /p_rated / 3.0 * (R_fp / (2.0 * PI));



							pred_state.dfmax_ini_StateVal = (Pmax - curr_state.p_mea_delayed /p_rated / 3.0) * kipmax / (2.0 * PI);
							pred_state.fmax_ini_StateVal = curr_state.fmax_ini_StateVal + pred_state.dfmax_ini_StateVal * deltat;

                            // 0 Limiter for integrator of Pmax controller
							if (pred_state.fmax_ini_StateVal > 0.0){

								pred_state.fmax_ini_StateVal = 0.0;

							}

                            // Lower Limiter for integrator of Pmax controller
							if (pred_state.fmax_ini_StateVal < Pmax_Low_Limit){

								pred_state.fmax_ini_StateVal = Pmax_Low_Limit;

							}


							pred_state.fmax_StateVal = pred_state.fmax_ini_StateVal + pred_state.dfmax_ini_StateVal * kppmax / (2.0 * PI) / kipmax;

	                           // 0 limiter for Pmax controller
							if (pred_state.fmax_StateVal > 0.0){

								pred_state.fmax_StateVal = 0.0;

							}

                            // Lower limiter for Pmax controller
							if (pred_state.fmax_StateVal < Pmax_Low_Limit){

								pred_state.fmax_StateVal = Pmax_Low_Limit;

							}



							// Pmin controller
							pred_state.dfmin_ini_StateVal = (Pmin - curr_state.p_mea_delayed / p_rated / 3.0) * kipmax / (2.0 * PI);
							pred_state.fmin_ini_StateVal = curr_state.fmin_ini_StateVal + pred_state.dfmin_ini_StateVal * deltat;

                            // 0 Limiter for integrator of Pmin controller
							if (pred_state.fmin_ini_StateVal < 0.0){

								pred_state.fmin_ini_StateVal = 0.0;

							}

                            // 0 limiter for Pmin controller
							pred_state.fmin_StateVal = pred_state.fmin_ini_StateVal + pred_state.dfmin_ini_StateVal * kppmax / (2.0 * PI) / kipmax;

							if (pred_state.fmin_StateVal < 0.0){

								pred_state.fmin_StateVal = 0.0;

							}

						   VSI_freq = freq_ref - delta_f + pred_state.fmax_StateVal + pred_state.fmin_StateVal;   // the frequency is obtained from droop control, Pmax control, and Pmin control


							// Calculate voltage source magnitude based on the droop curve, we want esource is a 3 phase balanced source. The voltage loop only regulates the magnitude of esrouceA, esourceB=esourceC=esourceA
							V_mag[0] = V_mag_ref[0] - (pred_state.q_mea_delayed - Qref) /p_rated / 3.0 * R_vq * node_nominal_voltage;
							
							// PI regulator for Q-V droop
							pred_state.dV_StateVal[0] = (V_mag[0] - pCircuit_V_Avg) * ki_Vterminal;  // voltage loop regulates the average value of 3 phase voltage
							pred_state.V_StateVal[0] = curr_state.V_StateVal[0] + pred_state.dV_StateVal[0] * deltat;
							pred_state.e_source_mag[0] = pred_state.V_StateVal[0] + pred_state.dV_StateVal[0] * kp_Vterminal / ki_Vterminal;

							for(i = 0; i < 3; i++) {
								// Calculate voltage source angle based on the droop curve, it should be noted that phase angle will be calculated again in corrector pass, the value is accumulated
								V_angle[i] = V_angle[i] - (delta_f - pred_state.fmax_StateVal - pred_state.fmin_StateVal)* 2.0 * PI * deltat;

								e_source[i] = gld::complex(pred_state.e_source_mag[0] * cos(V_angle[i]), pred_state.e_source_mag[0] * sin(V_angle[i])); // The voltage loop only regulates the magnitude of esrouceA, esourceB=esourceC=esourceA

								// Thevenin voltage source to Norton current source convertion
								value_IGenerated[i] = e_source[i]/(gld::complex(Rfilter,Xfilter) * Zbase);

								//See how this aligns with the ramp rate, if necessary
								if (checkRampRate_real || checkRampRate_reactive)
								{
									//Deflag
									ramp_change = false;

									//See what the power out is for this "new" state
									temp_current_val[i] = (value_IGenerated[i] - generator_admittance[i][0]*value_Circuit_V[0] - generator_admittance[i][1]*value_Circuit_V[1] - generator_admittance[i][2]*value_Circuit_V[2]);

									//Update power output variables, just so we can see what is going on
									power_val[i] = value_Circuit_V[i]*~temp_current_val[i];

									if (checkRampRate_real) {

										//Compute the difference - real part
										power_diff_val = (power_val[i].Re() - prev_VA_out[i].Re()) / deltat;

										//See which way we are
										if (power_val[i].Re() > prev_VA_out[i].Re())	//Ramp up
										{
											//See if it was too big
											if (power_diff_val > rampUpRate_real)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetReal(prev_VA_out[i].Re() + (rampUpRate_real * deltat));
											}
											//Default else - was okay
										}
										else	//Ramp down
										{
											//See if it was too big
											if (power_diff_val < -rampDownRate_real)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetReal(prev_VA_out[i].Re() - (rampDownRate_real * deltat));
											}
											//Default else - was okay
										}

									}

									if (checkRampRate_reactive) {

										//Compute the difference - reactive part
										power_diff_val = (power_val[i].Im() - prev_VA_out[i].Im()) / deltat;

										if (power_val[i].Im() > prev_VA_out[i].Im())	//Ramp up
										{
											//See if it was too big
											if (power_diff_val > rampUpRate_reactive)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetImag(prev_VA_out[i].Im() + (rampUpRate_reactive * deltat));
											}
											//Default else - was okay
										}
										else	//Ramp down
										{
											//See if it was too big
											if (power_diff_val < -rampDownRate_reactive)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetImag(prev_VA_out[i].Im() - (rampDownRate_reactive * deltat));
											}
											//Default else - was okay
										}
									}


									//Now "extrapolate" this back to a current value, if needed
									if (ramp_change)
									{
										//Compute a "new current" value
										if (value_Circuit_V[i].Mag() > 0.0)
										{
											temp_current_val[i] = ~(power_val[i] / value_Circuit_V[i]);
										}
										else
										{
											temp_current_val[i] = gld::complex(0.0,0.0);
										}

										//Adjust it to IGenerated
										value_IGenerated[i] = temp_current_val[i] + generator_admittance[i][0]*value_Circuit_V[0] + generator_admittance[i][1]*value_Circuit_V[1] + generator_admittance[i][2]*value_Circuit_V[2];

										//And adjust the related "internal voltage" - this just broke the frequency too
										e_source[i] = value_IGenerated[i] * (gld::complex(Rfilter,Xfilter) * Zbase);

										//Other state variables needed to be updated?
									}
									//Default else - no ramp change, so don't mess with anything

									//Store the updated power value
									curr_VA_out[i] = power_val[i];
								}//Ramp rate check active
							}
						}
					}//End three-phase-ish
					else
					{
						gl_error("inverter:%d - %s -- Must be triplex or full three-phase",obj->id,(obj->name ? obj->name : "Unnamed"));
						/*  TROUBLESHOOT
						An inverter attempted to engage dynamic simulations, but is not triplex-connected or all three phases.  This is a requirement.
						*/

						//Just exit us
						return SM_ERROR;
					}

					simmode_return_value = SM_DELTA_ITER;	//Reiterate - to get us to corrector pass
				}
				else if (iteration_count_val == 1)	// Corrector pass
				{
					if((phases & 0x10) == 0x10)
					{
						//Update output power
						//Get current injected
						temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0]);

						//Update power output variables, just so we can see what is going on
						power_val[0] = value_Circuit_V[0]*~temp_current_val[0];

						VA_Out = power_val[0];

						pCircuit_V_Avg = value_Circuit_V[0].Mag();

						// Calculate power differences between true power outputs,and the measured delayed power outputs of this delta time step (not this iteration)
						next_state.dp_mea_delayed = 1.0/Tp_delay*(VA_Out.Re() - curr_state.p_mea_delayed);
						next_state.dq_mea_delayed = 1.0/Tq_delay*(VA_Out.Im() - curr_state.q_mea_delayed);

						// Calculate the measured delayed power output in this iteration
						next_state.p_mea_delayed = curr_state.p_mea_delayed + (deltat * next_state.dp_mea_delayed);
						next_state.q_mea_delayed = curr_state.q_mea_delayed + (deltat * next_state.dq_mea_delayed);

						// Update the system frequency
						if (mapped_freq_variable!=nullptr)
						{
							mapped_freq_variable->setp<double>(VSI_freq,*test_rlock);
						}

						if (VSI_mode == VSI_ISOCHRONOUS)
						{
							next_state.dV_StateVal[0] = (V_mag_ref[0] - value_Circuit_V[0].Mag()) * ki_Vterminal;
							next_state.V_StateVal[0] = curr_state.V_StateVal[0] + (pred_state.dV_StateVal[0] + next_state.dV_StateVal[0])* deltath;
							next_state.e_source_mag[0] = next_state.V_StateVal[0] + (pred_state.dV_StateVal[0] + next_state.dV_StateVal[0]) * 0.5 * kp_Vterminal / ki_Vterminal;
							e_source[0] = gld::complex(next_state.e_source_mag[0] * cos(V_angle[0]), next_state.e_source_mag[0] * sin(V_angle[0]));

							// Thevenin voltage source to Norton current source conversion
							value_IGenerated[0] = e_source[0]/(gld::complex(Rfilter,Xfilter) * Zbase);
						}

						// VSI droop mode updates its e_source value based on PQ_Out and droop curve
						else if (VSI_mode == VSI_DROOP) {

							// Obtain the changes of frequency
							double delta_f = (next_state.p_mea_delayed - Pref) /p_rated * (R_fp / (2.0 * PI));
                            double delta_f1 = (curr_state.p_mea_delayed - Pref) /p_rated * (R_fp / (2.0 * PI));
							// Pmax controller
							next_state.dfmax_ini_StateVal = (Pmax - curr_state.p_mea_delayed /p_rated) * kipmax / (2.0 * PI);
							next_state.fmax_ini_StateVal = curr_state.fmax_ini_StateVal +(pred_state.dfmax_ini_StateVal + next_state.dfmax_ini_StateVal) * deltath;

                            // 0 Limiter for integrator of Pmax controller
							if (next_state.fmax_ini_StateVal > 0.0){

								next_state.fmax_ini_StateVal = 0.0;

							}

                            // Lower Limiter for integrator of Pmax controller
							if (next_state.fmax_ini_StateVal < Pmax_Low_Limit){

								next_state.fmax_ini_StateVal = Pmax_Low_Limit;

							}

							next_state.fmax_StateVal = next_state.fmax_ini_StateVal + (pred_state.dfmax_ini_StateVal + next_state.dfmax_ini_StateVal) * 0.5 * kppmax / (2.0 * PI) / kipmax;

							if (next_state.fmax_StateVal > 0.0){

								next_state.fmax_StateVal = 0.0;

							}

                            // Lower limiter for Pmax controller
							if (next_state.fmax_StateVal < Pmax_Low_Limit){

								next_state.fmax_StateVal = Pmax_Low_Limit;

							}


							// Pmin controller
							next_state.dfmin_ini_StateVal = (Pmin - curr_state.p_mea_delayed /p_rated) * kipmax / (2.0 * PI);
							next_state.fmin_ini_StateVal = curr_state.fmin_ini_StateVal +(pred_state.dfmin_ini_StateVal + next_state.dfmin_ini_StateVal) * deltath;

                            // 0 Limiter for integrator of Pmin controller
							if (next_state.fmin_ini_StateVal < 0.0){

								next_state.fmin_ini_StateVal = 0.0;

							}

                            // 0 limiter for Pmin controller
							next_state.fmin_StateVal = next_state.fmin_ini_StateVal + (pred_state.dfmin_ini_StateVal + next_state.dfmin_ini_StateVal) * 0.5 * kppmax / (2.0 * PI) / kipmax;

							if (next_state.fmin_StateVal < 0.0){

								next_state.fmin_StateVal = 0.0;

							}

							VSI_freq = freq_ref - delta_f + next_state.fmax_StateVal + next_state.fmin_StateVal;  //frequency is obtained from three parts, droop, Pmax and Pmin controls

							V_mag[0] = V_mag_ref[0] - (next_state.q_mea_delayed - Qref) / p_rated * R_vq * node_nominal_voltage;

                            // PI regulator for Q-V droop
							next_state.dV_StateVal[0] = (V_mag[0] - pCircuit_V_Avg) * ki_Vterminal;   // voltage loop only regulates magnitudes of esourceA, we want esource is always 3 phase balanced. esourceB=esourceC=esourceA
							next_state.V_StateVal[0] = curr_state.V_StateVal[0] + (pred_state.dV_StateVal[0] + next_state.dV_StateVal[0]) * deltath;
							next_state.e_source_mag[0] = next_state.V_StateVal[0] + (pred_state.dV_StateVal[0] + next_state.dV_StateVal[0]) * 0.5 * kp_Vterminal / ki_Vterminal;

							// Calculate voltage source angle based on the droop curve, phase angle is already calculated in predictor pass, so here should firstly subtract half of the delta value in predictor pass
							V_angle[0] = V_angle[0] + (delta_f1 - pred_state.fmax_StateVal - pred_state.fmin_StateVal) * 2.0 * PI * deltat * 0.5 - (delta_f - (next_state.fmax_StateVal + next_state.fmin_StateVal)) * 2.0 * PI * deltat * 0.5;

							// Calculate voltage source magnitude based on the droop curve
							e_source[0] = gld::complex(next_state.e_source_mag[0] * cos(V_angle[0]), next_state.e_source_mag[0] * sin(V_angle[0]));  //we want esource is always 3 phase balanced. esourceB=esourceC=esourceA

							// Thevenin voltage source to Norton current source convertion
							value_IGenerated[0] = e_source[0]/(gld::complex(Rfilter,Xfilter) * Zbase);
						}
					} // End triplex
					else if ((phases & 0x07) == 0x07)
					{
						//Update output power
						//Get current injected
						temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0] - generator_admittance[0][1]*value_Circuit_V[1] - generator_admittance[0][2]*value_Circuit_V[2]);
						temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0]*value_Circuit_V[0] - generator_admittance[1][1]*value_Circuit_V[1] - generator_admittance[1][2]*value_Circuit_V[2]);
						temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0]*value_Circuit_V[0] - generator_admittance[2][1]*value_Circuit_V[1] - generator_admittance[2][2]*value_Circuit_V[2]);

						//Update power output variables, just so we can see what is going on
						power_val[0] = value_Circuit_V[0]*~temp_current_val[0];
						power_val[1] = value_Circuit_V[1]*~temp_current_val[1];
						power_val[2] = value_Circuit_V[2]*~temp_current_val[2];

						VA_Out = power_val[0] + power_val[1] + power_val[2];

						pCircuit_V_Avg = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0;

						// Calculate power differences between true power outputs,and the measured delayed power outputs of this delta time step (not this iteration)
						next_state.dp_mea_delayed = 1.0/Tp_delay*(VA_Out.Re() - curr_state.p_mea_delayed);
						next_state.dq_mea_delayed = 1.0/Tq_delay*(VA_Out.Im() - curr_state.q_mea_delayed);

						// Calculate the measured delayed power output in this iteration
						next_state.p_mea_delayed = curr_state.p_mea_delayed + (deltat * next_state.dp_mea_delayed);
						next_state.q_mea_delayed = curr_state.q_mea_delayed + (deltat * next_state.dq_mea_delayed);

						// Update the system frequency
						if (mapped_freq_variable!=nullptr)
						{
							mapped_freq_variable->setp<double>(VSI_freq,*test_rlock);
						}

						if (VSI_mode == VSI_ISOCHRONOUS) {
							for(i = 0; i < 3; i++) {
								next_state.dV_StateVal[i] = (V_mag_ref[i] - value_Circuit_V[i].Mag()) * ki_Vterminal;
								next_state.V_StateVal[i] = curr_state.V_StateVal[i] + (pred_state.dV_StateVal[i] + next_state.dV_StateVal[i])* deltath;
								next_state.e_source_mag[i] = next_state.V_StateVal[i] + (pred_state.dV_StateVal[i] + next_state.dV_StateVal[i]) * 0.5 * kp_Vterminal / ki_Vterminal;
								e_source[i] = gld::complex(next_state.e_source_mag[i] * cos(V_angle[i]), next_state.e_source_mag[i] * sin(V_angle[i]));

								// Thevenin voltage source to Norton current source conversion
								value_IGenerated[i] = e_source[i]/(gld::complex(Rfilter,Xfilter) * Zbase);

								//See how this aligns with the real and reactive ramp rate, if necessary
								if (checkRampRate_real || checkRampRate_reactive)
								{
									//Deflag
									ramp_change = false;

									//See what the power out is for this "new" state
									temp_current_val[i] = (value_IGenerated[i] - generator_admittance[i][0]*value_Circuit_V[0] - generator_admittance[i][1]*value_Circuit_V[1] - generator_admittance[i][2]*value_Circuit_V[2]);

									//Update power output variables, just so we can see what is going on
									power_val[i] = value_Circuit_V[i]*~temp_current_val[i];

									//See which way we are
									if (checkRampRate_real) {

										//Compute the difference - real part
										power_diff_val = (power_val[i].Re() - prev_VA_out[i].Re()) / deltat;

										if (power_val[i].Re() > prev_VA_out[i].Re())	//Ramp up
										{
											//See if it was too big
											if (power_diff_val > rampUpRate_real)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetReal(prev_VA_out[i].Re() + (rampUpRate_real * deltat));
											}
											//Default else - was okay
										}
										else	//Ramp down
										{
											//See if it was too big
											if (power_diff_val < -rampDownRate_real)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetReal(prev_VA_out[i].Re() - (rampDownRate_real * deltat));
											}
											//Default else - was okay
										}
									}
									if (checkRampRate_reactive) {

										//Compute the difference - real part
										power_diff_val = (power_val[i].Im() - prev_VA_out[i].Im()) / deltat;

										if (power_val[i].Im() > prev_VA_out[i].Im())	//Ramp up
										{
											//See if it was too big
											if (power_diff_val > rampUpRate_reactive)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetImag(prev_VA_out[i].Im() + (rampUpRate_reactive * deltat));
											}
											//Default else - was okay
										}
										else	//Ramp down
										{
											//See if it was too big
											if (power_diff_val < -rampDownRate_reactive)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetImag(prev_VA_out[i].Im() - (rampDownRate_reactive * deltat));
											}
											//Default else - was okay
										}
									}

									//Now "extrapolate" this back to a current value, if needed
									if (ramp_change)
									{
										//Compute a "new current" value
										if (value_Circuit_V[i].Mag() > 0.0)
										{
											temp_current_val[i] = ~(power_val[i] / value_Circuit_V[i]);
										}
										else
										{
											temp_current_val[i] = gld::complex(0.0,0.0);
										}

										//Adjust it to IGenerated
										value_IGenerated[i] = temp_current_val[i] + generator_admittance[i][0]*value_Circuit_V[0] + generator_admittance[i][1]*value_Circuit_V[1] + generator_admittance[i][2]*value_Circuit_V[2];

										//And adjust the related "internal voltage" - this just broke the frequency too
										e_source[i] = value_IGenerated[i] * (gld::complex(Rfilter,Xfilter) * Zbase);

										//Other state variables needed to be updated?
									}
									//Default else - no ramp change, so don't mess with anything
								}//Ramp rate check active and reactive

								//Store the current output value
								curr_VA_out[i] = power_val[i];
							}
						}

						// VSI droop mode updates its e_source value based on PQ_Out and droop curve
						else if (VSI_mode == VSI_DROOP) {

							// Obtain the changes of frequency
							double delta_f = (next_state.p_mea_delayed - Pref) /p_rated / 3.0 * (R_fp / (2.0 * PI));
                            double delta_f1 = (curr_state.p_mea_delayed - Pref) /p_rated / 3.0 * (R_fp / (2.0 * PI));
							// Pmax controller
							next_state.dfmax_ini_StateVal = (Pmax - curr_state.p_mea_delayed /p_rated / 3.0) * kipmax / (2.0 * PI);
							next_state.fmax_ini_StateVal = curr_state.fmax_ini_StateVal +(pred_state.dfmax_ini_StateVal + next_state.dfmax_ini_StateVal) * deltath;

                            // 0 Limiter for integrator of Pmax controller
							if (next_state.fmax_ini_StateVal > 0.0){

								next_state.fmax_ini_StateVal = 0.0;

							}

                            // Lower Limiter for integrator of Pmax controller
							if (next_state.fmax_ini_StateVal < Pmax_Low_Limit){

								next_state.fmax_ini_StateVal = Pmax_Low_Limit;

							}

							next_state.fmax_StateVal = next_state.fmax_ini_StateVal + (pred_state.dfmax_ini_StateVal + next_state.dfmax_ini_StateVal) * 0.5 * kppmax / (2.0 * PI) / kipmax;

                            // 0 limiter for Pmax controller
							if (next_state.fmax_StateVal > 0.0){

								next_state.fmax_StateVal = 0.0;

							}

                            // Lower limiter for Pmax controller
							if (next_state.fmax_StateVal < Pmax_Low_Limit){

								next_state.fmax_StateVal = Pmax_Low_Limit;

							}

							// Pmin controller
							next_state.dfmin_ini_StateVal = (Pmin - curr_state.p_mea_delayed /p_rated / 3.0) * kipmax / (2.0 * PI);
							next_state.fmin_ini_StateVal = curr_state.fmin_ini_StateVal +(pred_state.dfmin_ini_StateVal + next_state.dfmin_ini_StateVal) * deltath;

                            // 0 Limiter for integrator of Pmin controller
							if (next_state.fmin_ini_StateVal < 0.0){

								next_state.fmin_ini_StateVal = 0.0;

							}

                            // 0 limiter for Pmin controller
							next_state.fmin_StateVal = next_state.fmin_ini_StateVal + (pred_state.dfmin_ini_StateVal + next_state.dfmin_ini_StateVal) * 0.5 * kppmax / (2.0 * PI) / kipmax;

							if (next_state.fmin_StateVal < 0.0){

								next_state.fmin_StateVal = 0.0;

							}

							VSI_freq = freq_ref - delta_f + next_state.fmax_StateVal + next_state.fmin_StateVal;  //frequency is obtained from three parts, droop, Pmax and Pmin controls

							V_mag[0] = V_mag_ref[0] - (next_state.q_mea_delayed - Qref) / p_rated / 3.0 * R_vq * node_nominal_voltage;

                            // PI regulator for Q-V droop
							next_state.dV_StateVal[0] = (V_mag[0] - pCircuit_V_Avg) * ki_Vterminal;   // voltage loop only regulates magnitudes of esourceA, we want esource is always 3 phase balanced. esourceB=esourceC=esourceA
							next_state.V_StateVal[0] = curr_state.V_StateVal[0] + (pred_state.dV_StateVal[0] + next_state.dV_StateVal[0]) * deltath;
							next_state.e_source_mag[0] = next_state.V_StateVal[0] + (pred_state.dV_StateVal[0] + next_state.dV_StateVal[0]) * 0.5 * kp_Vterminal / ki_Vterminal;

							for(i = 0; i < 3; i++) {

								// Calculate voltage source angle based on the droop curve, phase angle is already calculated in predictor pass, so here should firstly subtract half of the delta value in predictor pass
								V_angle[i] = V_angle[i] + (delta_f1 - pred_state.fmax_StateVal - pred_state.fmin_StateVal) * 2.0 * PI * deltat * 0.5 - (delta_f - (next_state.fmax_StateVal + next_state.fmin_StateVal)) * 2.0 * PI * deltat * 0.5;

								// Calculate voltage source magnitude based on the droop curve
								e_source[i] = gld::complex(next_state.e_source_mag[0] * cos(V_angle[i]), next_state.e_source_mag[0] * sin(V_angle[i]));  //we want esource is always 3 phase balanced. esourceB=esourceC=esourceA

								// Thevenin voltage source to Norton current source convertion
								value_IGenerated[i] = e_source[i]/(gld::complex(Rfilter,Xfilter) * Zbase);

								//See how this aligns with the ramp rate, if necessary
								if (checkRampRate_real || checkRampRate_reactive)
								{
									//Deflag
									ramp_change = false;

									//See what the power out is for this "new" state
									temp_current_val[i] = (value_IGenerated[i] - generator_admittance[i][0]*value_Circuit_V[0] - generator_admittance[i][1]*value_Circuit_V[1] - generator_admittance[i][2]*value_Circuit_V[2]);

									//Update power output variables, just so we can see what is going on
									power_val[i] = value_Circuit_V[i]*~temp_current_val[i];

									if (checkRampRate_real) {
										//Compute the difference - just real part for now (probably need to expand this)
										power_diff_val = (power_val[i].Re() - prev_VA_out[i].Re()) / deltat;

										//See which way we are
										if (power_val[i].Re() > prev_VA_out[i].Re())	//Ramp up
										{
											//See if it was too big
											if (power_diff_val > rampUpRate_real)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetReal(prev_VA_out[i].Re() + (rampUpRate_real * deltat));
											}
											//Default else - was okay
										}
										else	//Ramp down
										{
											//See if it was too big
											if (power_diff_val < -rampDownRate_real)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetReal(prev_VA_out[i].Re() - (rampDownRate_real * deltat));
											}
											//Default else - was okay
										}

									}

									if (checkRampRate_reactive) {

										//Compute the difference - reactive part
										power_diff_val = (power_val[i].Im() - prev_VA_out[i].Im()) / deltat;

										if (power_val[i].Im() > prev_VA_out[i].Im())	//Ramp up
										{
											//See if it was too big
											if (power_diff_val > rampUpRate_reactive)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetImag(prev_VA_out[i].Im() + (rampUpRate_reactive * deltat));
											}
											//Default else - was okay
										}
										else	//Ramp down
										{
											//See if it was too big
											if (power_diff_val < -rampDownRate_reactive)
											{
												//Flag
												ramp_change = true;

												power_val[i].SetImag(prev_VA_out[i].Im() - (rampDownRate_reactive * deltat));
											}
											//Default else - was okay
										}
									}

									//Now "extrapolate" this back to a current value, if needed
									if (ramp_change)
									{
										//Compute a "new current" value
										if (value_Circuit_V[i].Mag() > 0.0)
										{
											temp_current_val[i] = ~(power_val[i] / value_Circuit_V[i]);
										}
										else
										{
											temp_current_val[i] = gld::complex(0.0,0.0);
										}

										//Adjust it to IGenerated
										value_IGenerated[i] = temp_current_val[i] + generator_admittance[i][0]*value_Circuit_V[0] + generator_admittance[i][1]*value_Circuit_V[1] + generator_admittance[i][2]*value_Circuit_V[2];

										//And adjust the related "internal voltage" - this just broke the frequency too
										e_source[i] = value_IGenerated[i] * (gld::complex(Rfilter,Xfilter) * Zbase);

										//Other state variables needed to be updated?
									}
									//Default else - no ramp change, so don't mess with anything

									//Store the updated power value
									curr_VA_out[i] = power_val[i];
								}//Ramp rate check active
							}
						}
					}//End three-phase
					//Else would be an error, but the predictor should have gotten that

					// Copy everything back into curr_state, since we'll be back there
					memcpy(&curr_state, &next_state, sizeof(INV_STATE));

					simmode_return_value =  SM_DELTA;
				}
				else	//Other iterations
				{
					//Just return whatever our "last desired" was
					simmode_return_value = desired_simulation_mode;
				}
			}
			else {
				//Initializate the state of the inverter
				if (delta_time==0)	//First run of new delta call
				{
					if(iteration_count_val == 0) {
						// Set Tfreq_delay value if not defined in glm file
						if (inverter_droop_fp) {
							if (Tfreq_delay == 0) {
								Tfreq_delay = deltat;
							}
						}
						// Set Tvol_delay value if not defined in glm file
						if (inverter_droop_vq) {
							if (Tvol_delay == 0) {
								Tvol_delay = deltat;
							}
						}

						// If in CONSTANT_PQ mode for smooth transition, need to calculate current output based on constant PQ and changed terminal voltage
						if (inverter_type_v == FOUR_QUADRANT && four_quadrant_control_mode == FQM_CONSTANT_PQ) {
							if((phases & 0x10) == 0x10) {

								power_val[0] = value_Circuit_V[0] * ~(I_Out[0]);

								curr_state.P_Out[0] = power_val[0].Re();
								curr_state.Q_Out[0] = power_val[0].Im();

								value_Line_unrotI[0] += curr_state.Iac[0];	// remove the previous current injection to the circuit

								if (value_Circuit_V[0].Mag() > 0.0)	//Voltage check
								{
									curr_state.Iac[0] = ~(gld::complex(curr_state.P_Out[0],curr_state.Q_Out[0])/(value_Circuit_V[0]));
								}
								else
								{
									curr_state.Iac[0] = gld::complex(0.0,0.0);
								}

								I_Out[0]= curr_state.Iac[0];
								value_Line_unrotI[0] += -curr_state.Iac[0]; // update the current injection to the circuit
							}
							else if ((phases & 0x07) == 0x07) {
								for(i = 0; i < 3; i++) {

									power_val[i] = value_Circuit_V[i] * ~(I_Out[i]);

									curr_state.P_Out[i] = power_val[i].Re();
									curr_state.Q_Out[i] = power_val[i].Im();

									value_Line_unrotI[i] += curr_state.Iac[i];	// remove the previous current injection to the circuit

									if (value_Circuit_V[i].Mag() > 0.0)
									{
										curr_state.Iac[i] = ~(gld::complex(curr_state.P_Out[i],curr_state.Q_Out[i])/(value_Circuit_V[i]));
									}
									else
									{
										curr_state.Iac[i] = gld::complex(0.0,0.0);
									}

									I_Out[i]= curr_state.Iac[i];
									value_Line_unrotI[i] += -curr_state.Iac[i]; // update the current injection to the circuit
								}
							}
						} // end initialize I_Out based on constant PQ output and terminal voltages
						else {
							// //Initialize dynamics
							// init_dynamics(&curr_state);
							//Send Current Injection to parent
							if((phases & 0x10) == 0x10) {
								//pLine_unrotI[0] += -curr_state.Iac[0];
								I_Out[0]= curr_state.Iac[0];
							}
							if((phases & 0x07) == 0x07) {
								for(int i = 0; i < 3; i++) {
									//pLine_unrotI[i] += -curr_state.Iac[i];
									I_Out[i] = curr_state.Iac[i];
								}
							}
						}
						// If not FQM_CONSTANT_PQ mode, keep current injection the same, will calculate PQ out based on I_Out and pCircuit_V in the later iteration
						simmode_return_value =  SM_DELTA_ITER; // iterate so I know what my current power out is

					} else if(iteration_count_val == 1) {

						// Calculation for frequency deviation
						if (inverter_droop_fp) {
							curr_state.df_mea_delayed = 1.0/Tfreq_delay*(value_Frequency - curr_state.f_mea_delayed);
						}
						// Calculation for frequency deviation
						if (inverter_droop_vq) {
							if((phases & 0x10) == 0x10) {
								curr_state.dV_mea_delayed[0] = 1.0/Tvol_delay*(value_Circuit_V[0].Mag() - curr_state.V_mea_delayed[0]);
							}
							if((phases & 0x07) == 0x07) {
								curr_state.dV_mea_delayed[0] = 1.0/Tvol_delay*(value_Circuit_V[0].Mag() - curr_state.V_mea_delayed[0]);
								curr_state.dV_mea_delayed[1] = 1.0/Tvol_delay*(value_Circuit_V[1].Mag() - curr_state.V_mea_delayed[1]);
								curr_state.dV_mea_delayed[2] = 1.0/Tvol_delay*(value_Circuit_V[2].Mag() - curr_state.V_mea_delayed[2]);
							}
						}

						// Calculate my current power out - not used in CONSTANT_PQ mode for smooth transition
						if (inverter_type_v == FOUR_QUADRANT && four_quadrant_control_mode != FQM_CONSTANT_PQ) {
							if((phases & 0x10) == 0x10) {
								VA_Out = value_Circuit_V[0] * ~(I_Out[0]);
								curr_state.Q_Out[0] = VA_Out.Im();
							}
							if((phases & 0x07) == 0x07) {
								VA_Out = (value_Circuit_V[0] * ~(I_Out[0]) + (value_Circuit_V[1] * ~(I_Out[1])) + (value_Circuit_V[2] * ~(I_Out[2])));
								curr_state.Q_Out[0] = (value_Circuit_V[0] * ~(I_Out[0])).Im();
								curr_state.Q_Out[1] = (value_Circuit_V[1] * ~(I_Out[1])).Im();
								curr_state.Q_Out[2] = (value_Circuit_V[2] * ~(I_Out[2])).Im();
							}
						}

						//calculate my current errors
						if((phases & 0x10) == 0x10) {
							power_val[0] = value_Circuit_V[0] * ~(I_Out[0]);

							curr_state.P_Out[0] = power_val[0].Re();
							curr_state.Q_Out[0] = power_val[0].Im();

							if (value_Circuit_V[0].Mag() > 0.0)
							{
								curr_state.ed[0] = ((~(gld::complex(Pref, Qref_PI[0])/(value_Circuit_V[0]))) - (~(gld::complex(curr_state.P_Out[0],curr_state.Q_Out[0])/(value_Circuit_V[0])))).Re();
								curr_state.eq[0] = ((~(gld::complex(Pref, Qref_PI[0])/(value_Circuit_V[0]))) - (~(gld::complex(curr_state.P_Out[0],curr_state.Q_Out[0])/(value_Circuit_V[0])))).Im();
							}
							else
							{
								curr_state.ed[0] = ((~(gld::complex(Pref, Qref_PI[0])/(value_Circuit_V[0]))) - (~(gld::complex(curr_state.P_Out[0],curr_state.Q_Out[0])/(value_Circuit_V[0])))).Re();
								curr_state.eq[0] = ((~(gld::complex(Pref, Qref_PI[0])/(value_Circuit_V[0]))) - (~(gld::complex(curr_state.P_Out[0],curr_state.Q_Out[0])/(value_Circuit_V[0])))).Im();
							}

							curr_state.ded[0] = curr_state.ed[0] / deltat;
							curr_state.dmd[0] = kid * curr_state.ed[0];  //To ensure a smooth transition ?

							curr_state.deq[0] = curr_state.eq[0] / deltat;
							curr_state.dmq[0] =  kiq * curr_state.eq[0];  //To ensure a smooth transition ?

							if ((fabs(curr_state.ded[0]) <= inverter_convergence_criterion) && (fabs(curr_state.deq[0]) <= inverter_convergence_criterion) && (simmode_return_value != SM_DELTA))
							{
								simmode_return_value = SM_EVENT;// we have reached steady state
							} else {
								simmode_return_value = SM_DELTA;
							}
						}
						else if((phases & 0x07) == 0x07) {
							for(i = 0; i < 3; i++) {

								power_val[i] = value_Circuit_V[i] * ~(I_Out[i]);

								curr_state.P_Out[i] = power_val[i].Re();
								curr_state.Q_Out[i] = power_val[i].Im();

								if (value_Circuit_V[i].Mag() > 0.0)
								{
									curr_state.ed[i] = ((~(gld::complex(Pref/3.0, Qref_PI[i])/(value_Circuit_V[i]))) - (~(gld::complex(curr_state.P_Out[i],curr_state.Q_Out[i])/(value_Circuit_V[i])))).Re();
									curr_state.eq[i] = ((~(gld::complex(Pref/3.0, Qref_PI[i])/(value_Circuit_V[i]))) - (~(gld::complex(curr_state.P_Out[i],curr_state.Q_Out[i])/(value_Circuit_V[i])))).Im();
								}
								else
								{
									curr_state.ed[i] = 0.0;
									curr_state.eq[i] = 0.0;
								}

								curr_state.ded[i] = curr_state.ed[i] / deltat;
								curr_state.dmd[i] = kid * curr_state.ed[i];  //To ensure a smooth transition ?

								curr_state.deq[i] = curr_state.eq[i] / deltat;
								curr_state.dmq[i] =  kiq * curr_state.eq[i];  //To ensure a smooth transition ?

								if ((fabs(curr_state.ded[i]) <= inverter_convergence_criterion) && (fabs(curr_state.deq[i]) <= inverter_convergence_criterion) && (simmode_return_value != SM_DELTA))
								{
									simmode_return_value = SM_EVENT;// we have reached steady state
								} else {
									simmode_return_value = SM_DELTA;
								}
							}
						}
					}
					else
					{
						//Just return what we were going to do for additional "steps"
						simmode_return_value = desired_simulation_mode;
					}
				} else if(iteration_count_val == 0) {
					// Check if P_Out and Q_Out changed during delta_mode
					if (P_Out != Pref0) {
						Pref = P_Out;
						Pref0 = P_Out;
					}

					if((phases & 0x10) == 0x10) {
						if (Q_Out != Qref0[0]) {
							Qref_PI[0] = Q_Out;
							Qref0[0] = Q_Out;
						}
					}
					else if((phases & 0x07) == 0x07) {
						if (Q_Out != Qref0[0]+Qref0[1]+Qref0[2]) {
							for(i = 0; i < 3; i++) {
								Qref_PI[i] = Q_Out/3;
								Qref0[i] = Q_Out/3;
							}
						}
					}

					//Calculate the predictor state from the previous current state
					// Frequency change and thereforely Pref change from p/f droop
					if (inverter_droop_fp) {
						Pref_prev = Pref;

						curr_state.df_mea_delayed = 1.0/Tfreq_delay*(value_Frequency - curr_state.f_mea_delayed);
						pred_state.f_mea_delayed = curr_state.f_mea_delayed + (deltat * curr_state.df_mea_delayed);

						// Calculate Pref based on the droop curve
						double delta_Pref = ((pred_state.f_mea_delayed - freq_ref)/freq_ref) * (1 / R_fp) * p_rated * 3;
						power_diff_val = Pref_prev - (Pref0 - delta_Pref);
						if (checkRampRate_real) {
							if (power_diff_val > 0 && (power_diff_val > rampDownRate_real*3)) {
								Pref = Pref_prev - rampDownRate_real*3;
							}
							else if (power_diff_val < 0 && (-power_diff_val > rampUpRate_real*3)) {
								Pref = Pref_prev + rampUpRate_real*3;
							}
							else {
								Pref = Pref0 - delta_Pref;
							}
						}
						else {
							Pref = Pref0 - delta_Pref;

							if(Pref > 3 * p_rated){  //prevents from exceeding the rated power, 3 indicates for three phase
								Pref = 3 * p_rated;
							}

						}
					}

					// If terminal voltage changes, Qref for each phase is changed from p/f droop
					if (inverter_droop_vq) {
						// Calculate Qref based on the droop curve
						if((phases & 0x10) == 0x10) {
							double delta_Qref;

							//Update delayed value
							curr_state.dV_mea_delayed[0] = 1.0/Tvol_delay*(value_Circuit_V[0].Mag() - curr_state.V_mea_delayed[0]);

							Qref_prev[0] = Qref_PI[0];
							pred_state.V_mea_delayed[0] = curr_state.V_mea_delayed[0] + (deltat * curr_state.dV_mea_delayed[0]);
							delta_Qref = (pred_state.V_mea_delayed[0] - V_ref[0]) / node_nominal_voltage * (1.0 / R_vq) * p_rated;
							power_diff_val = Qref_prev[0] - (Qref0[0] - delta_Qref);
							if (checkRampRate_reactive) {
								if (power_diff_val > 0 && (power_diff_val > rampDownRate_reactive)) {
									Qref_PI[0] = Qref_prev[0] - rampDownRate_reactive;
								}
								else if (power_diff_val < 0 && (-power_diff_val > rampUpRate_reactive)) {
									Qref_PI[0] = Qref_prev[0] + rampUpRate_reactive;
								}
								else {
									Qref_PI[0] = Qref0[0] - delta_Qref;
								}
							}
							else {
								Qref_PI[0] = Qref0[0] - delta_Qref;
							}
						}
						else if((phases & 0x07) == 0x07) {
							double delta_Qref[3];

							for(i = 0; i < 3; i++)
							{
								//Update delayed value
								curr_state.dV_mea_delayed[i] = 1.0/Tvol_delay*(value_Circuit_V[i].Mag() - curr_state.V_mea_delayed[i]);

								Qref_prev[i] = Qref_PI[i];
								pred_state.V_mea_delayed[i] = curr_state.V_mea_delayed[i] + (deltat * curr_state.dV_mea_delayed[i]);
								delta_Qref[i] = (pred_state.V_mea_delayed[i] - V_ref[i]) / node_nominal_voltage * (1.0 / R_vq) * p_rated;
								power_diff_val = Qref_prev[i] - (Qref0[i] - delta_Qref[i]);
								if (checkRampRate_reactive) {
									if (power_diff_val > 0 && (power_diff_val > rampDownRate_reactive)) {
										Qref_PI[i] = Qref_prev[i] - rampDownRate_reactive;
									}
									else if (power_diff_val < 0 && (-power_diff_val > rampUpRate_reactive)) {
										Qref_PI[i] = Qref_prev[i] + rampUpRate_reactive;
									}
									else {
										Qref_PI[i] = Qref0[i] - delta_Qref[i];
									}
								}
								else {
									Qref_PI[i] = Qref0[i] - delta_Qref[i];
								}
							}
						}

					}

					// Store the prev_VA_out values for comparison
					if (checkRampRate_real ||  checkRampRate_reactive)
					{
						//See which one we are
						if ((phases & 0x10) == 0x10)
						{
							prev_VA_out[0] = curr_VA_out[0];
						}
						else	//Some variant of three-phase, just grab them all
						{
							//Copy in all the values - phasing doesn't matter for these
							prev_VA_out[0] = curr_VA_out[0];
							prev_VA_out[1] = curr_VA_out[1];
							prev_VA_out[2] = curr_VA_out[2];
						}
					}

					// Power change and thereforely frequency and voltage magnitude change from droops


					// PI controller parameters updates
					if((phases & 0x10) == 0x10)
					{
						VA_Out = (value_Circuit_V[0] * ~(I_Out[0]));

						// Check the current power output for each phase
						work_power_vals = value_Circuit_V[0] * ~I_Out[0];

						curr_state.P_Out[0] = work_power_vals.Re();
						curr_state.Q_Out[0] = work_power_vals.Im();

						prev_error_ed = curr_state.ed[0];
						prev_error_eq = curr_state.eq[0];

						if (value_Circuit_V[0].Mag() > 0.0)
						{
							curr_state.ed[0] = ((~(gld::complex(Pref, Qref_PI[0])/(value_Circuit_V[0]))) - (~(gld::complex(curr_state.P_Out[0],curr_state.Q_Out[0])/(value_Circuit_V[0])))).Re();
							curr_state.eq[0] = ((~(gld::complex(Pref, Qref_PI[0])/(value_Circuit_V[0]))) - (~(gld::complex(curr_state.P_Out[0],curr_state.Q_Out[0])/(value_Circuit_V[0])))).Im();
						}
						else
						{
							curr_state.ed[0] = 0.0;
							curr_state.eq[0] = 0.0;
						}

						curr_state.ded[0] = (curr_state.ed[0] - prev_error_ed) / deltat;
						curr_state.dmd[0] = (kpd * curr_state.ded[0]) + (kid * curr_state.ed[0]);


						curr_state.deq[0] = (curr_state.eq[0] - prev_error_eq) / deltat;
						curr_state.dmq[0] = (kpq * curr_state.deq[0]) + (kiq * curr_state.eq[0]);

					} else if ((phases & 0x07) == 0x07) {

						VA_Out = (value_Circuit_V[0] * ~(I_Out[0])) + (value_Circuit_V[1] * ~(I_Out[1])) + (value_Circuit_V[2] * ~(I_Out[2]));

						for(i = 0; i < 3; i++) {

							// Check the current power output for each phase
							work_power_vals = value_Circuit_V[i] * ~I_Out[i];

							curr_state.P_Out[i] = work_power_vals.Re();
							curr_state.Q_Out[i] = work_power_vals.Im();

							prev_error_ed = curr_state.ed[i];
							prev_error_eq = curr_state.eq[i];

							if (value_Circuit_V[i].Mag() > 0.0)
							{
								curr_state.ed[i] = ((~(gld::complex(Pref/3.0, Qref_PI[i])/(value_Circuit_V[i]))) - (~(gld::complex(curr_state.P_Out[i],curr_state.Q_Out[i])/(value_Circuit_V[i])))).Re();
								curr_state.eq[i] = ((~(gld::complex(Pref/3.0, Qref_PI[i])/(value_Circuit_V[i]))) - (~(gld::complex(curr_state.P_Out[i],curr_state.Q_Out[i])/(value_Circuit_V[i])))).Im();
							}
							else
							{
								curr_state.ed[i] = 0.0;
								curr_state.eq[i] = 0.0;
							}

							curr_state.ded[i] = (curr_state.ed[i] - prev_error_ed) / deltat;
							curr_state.dmd[i] = (kpd * curr_state.ded[i]) + (kid * curr_state.ed[i]);


							curr_state.deq[i] = (curr_state.eq[i] - prev_error_eq) / deltat;
							curr_state.dmq[i] = (kpq * curr_state.deq[i]) + (kiq * curr_state.eq[i]);
						}
					}

					// PI controller parameters updates
					if((phases & 0x10) == 0x10) {
						pred_state.md[0] = curr_state.md[0] + (deltat * curr_state.dmd[0]);
						pred_state.Idq[0].SetReal(pred_state.md[0] * I_In);
						pred_state.mq[0] = curr_state.mq[0] + (deltat * curr_state.dmq[0]);
						pred_state.Idq[0].SetImag(pred_state.mq[0] * I_In);
						pred_state.Iac[0] = pred_state.Idq[0];


						//Do a voltage check
						if (value_Circuit_V[i].Mag() > 0.0)
						{
							//Compute power value
							power_val[0] = (value_Circuit_V[0] * ~(pred_state.Iac[0]));
						}
						else	//Zero it
						{
							pred_state.Iac[0] = gld::complex(0.0,0.0);
							power_val[0] = gld::complex(0.0,0.0);
						}

						//See if either method is enabled and update the power-reference
						if (checkRampRate_real || checkRampRate_reactive)
						{
							//Deflag the variable
							ramp_change = false;

							if (checkRampRate_real) {

								//Compute the difference - real part
								power_diff_val = (power_val[0].Re() - prev_VA_out[0].Re()) / deltat;

								if (power_val[0].Re() > prev_VA_out[0].Re())	//Ramp up
								{
									//See if it was too big
									if (power_diff_val > rampUpRate_real)
									{
										//Flag
										ramp_change = true;

										power_val[0].SetReal(prev_VA_out[0].Re() + (rampUpRate_real * deltat));
									}
									//Default else - was okay
								}
								else	//Ramp down
								{
									//See if it was too big
									if (power_diff_val < -rampDownRate_real)
									{
										//Flag
										ramp_change = true;

										power_val[0].SetReal(prev_VA_out[0].Re() - (rampDownRate_real * deltat));
									}
									//Default else - was okay
								}
							}

							if (checkRampRate_reactive) {

								//Compute the difference - reactive part
								power_diff_val = (power_val[0].Im() - prev_VA_out[0].Im()) / deltat;

								if (power_val[0].Im() > prev_VA_out[0].Im())	//Ramp up
								{
									//See if it was too big
									if (power_diff_val > rampUpRate_reactive)
									{
										//Flag
										ramp_change = true;

										power_val[0].SetImag(prev_VA_out[0].Im() + (rampUpRate_reactive * deltat));
									}
									//Default else - was okay
								}
								else	//Ramp down
								{
									//See if it was too big
									if (power_diff_val < -rampDownRate_reactive)
									{
										//Flag
										ramp_change = true;

										power_val[0].SetImag(prev_VA_out[0].Im() - (rampDownRate_reactive * deltat));
									}
									//Default else - was okay
								}
							}

							//Now "extrapolate" this back to a current value, if needed
							if (ramp_change)
							{
								//Compute a "new current" value
								if (value_Circuit_V[0].Mag() > 0.0)
								{
									temp_current_val[0] = ~(power_val[0] / value_Circuit_V[0]);
								}
								else
								{
									temp_current_val[0] = gld::complex(0.0,0.0);
								}

								// Update the output current values, as well as the current multipliers
								pred_state.Idq[0] = temp_current_val[0];
								pred_state.md[0] = pred_state.Idq[0].Re()/I_In;
								pred_state.mq[0] = pred_state.Idq[0].Im()/I_In;
								pred_state.Iac[0] = pred_state.Idq[0];

							}
							//Default else - no ramp change, so don't mess with anything

							//Store the updated power value
							curr_VA_out[0] = power_val[0];
						}

						// Before updating pLine_unrotI and Iout, need to check inverter real power output:
						// If not attached to the battery, need to check if real power < 0 or > rating
						VA_Out = value_Circuit_V[0] * ~(pred_state.Iac[0]);

						// Then continue update current
						value_Line_unrotI[0] += I_Out[0];
						value_Line_unrotI[0] += -pred_state.Iac[0];
						I_Out[0] = pred_state.Iac[0]; // update I_Out so that power iteration can use it
					}
					else if((phases & 0x07) == 0x07) {
						for(i = 0; i < 3; i++) {
							pred_state.md[i] = curr_state.md[i] + (deltat * curr_state.dmd[i]);
							pred_state.Idq[i].SetReal(pred_state.md[i] * I_In);
							pred_state.mq[i] = curr_state.mq[i] + (deltat * curr_state.dmq[i]);
							pred_state.Idq[i].SetImag(pred_state.mq[i] * I_In);
							pred_state.Iac[i] = pred_state.Idq[i];

							//Do a voltage check
							if (value_Circuit_V[i].Mag() > 0.0)
							{
								//Compute power value
								power_val[i] = (value_Circuit_V[i] * ~(pred_state.Iac[i]));
							}
							else	//Zero it
							{
								pred_state.Iac[i] = gld::complex(0.0,0.0);
								power_val[i] = gld::complex(0.0,0.0);
							}

							//See if either method is enabled and update the power-reference
							if (checkRampRate_real || checkRampRate_reactive)
							{
								//Deflag the variable
								ramp_change = false;
								
								if (checkRampRate_real) {

									//Compute the difference - real part
									power_diff_val = (power_val[i].Re() - prev_VA_out[i].Re()) / deltat;

									if (power_val[i].Re() > prev_VA_out[i].Re())	//Ramp up
									{
										//See if it was too big
										if (power_diff_val > rampUpRate_real)
										{
											//Flag
											ramp_change = true;

											power_val[i].SetReal(prev_VA_out[i].Re() + (rampUpRate_real * deltat));
										}
										//Default else - was okay
									}
									else	//Ramp down
									{
										//See if it was too big
										if (power_diff_val < -rampDownRate_real)
										{
											//Flag
											ramp_change = true;

											power_val[i].SetReal(prev_VA_out[i].Re() - (rampDownRate_real * deltat));
										}
										//Default else - was okay
									}
								}

								if (checkRampRate_reactive) {

									//Compute the difference - reactive part
									power_diff_val = (power_val[i].Im() - prev_VA_out[i].Im()) / deltat;

									if (power_val[i].Im() > prev_VA_out[i].Im())	//Ramp up
									{
										//See if it was too big
										if (power_diff_val > rampUpRate_reactive)
										{
											//Flag
											ramp_change = true;

											power_val[i].SetImag(prev_VA_out[i].Im() + (rampUpRate_reactive * deltat));
										}
										//Default else - was okay
									}
									else	//Ramp down
									{
										//See if it was too big
										if (power_diff_val < -rampDownRate_reactive)
										{
											//Flag
											ramp_change = true;

											power_val[i].SetImag(prev_VA_out[i].Im() - (rampDownRate_reactive * deltat));
										}
										//Default else - was okay
									}
								}

								//Now "extrapolate" this back to a current value, if needed
								if (ramp_change)
								{
									//Compute a "new current" value
									if (value_Circuit_V[i].Mag() > 0.0)
									{
										temp_current_val[i] = ~(power_val[i] / value_Circuit_V[i]);
									}
									else
									{
										temp_current_val[i] = gld::complex(0.0,0.0);
									}

									// Update the output current values, as well as the current multipliers
									pred_state.Idq[i] = temp_current_val[i];
									pred_state.md[i] = pred_state.Idq[i].Re()/I_In;
									pred_state.mq[i] = pred_state.Idq[i].Im()/I_In;
									pred_state.Iac[i] = pred_state.Idq[i];

								}
								//Default else - no ramp change, so don't mess with anything

								//Store the updated power value
								curr_VA_out[i] = power_val[i];
							}
						}

						// Before updating pLine_unrotI and Iout, need to check inverter real power output:
						// If not attached to the battery, need to check if real power < 0 or > rating
						VA_Out = (value_Circuit_V[0] * ~(pred_state.Iac[0])) + (value_Circuit_V[1] * ~(pred_state.Iac[1])) + (value_Circuit_V[2] * ~(pred_state.Iac[2]));

						for (int i = 0; i< 3; i++) {
							//if ((b_soc == -1 && VA_Out_temp.Re() < 0) || Pref == 0) {
							//	pred_state.Iac[i] = 0;
							//}

							value_Line_unrotI[i] += I_Out[i];
							value_Line_unrotI[i] += -pred_state.Iac[i];
							I_Out[i] = pred_state.Iac[i]; // update I_Out so that power iteration can use it
						}
					}

					//update the Pref and Qref values
					update_control_references();
					simmode_return_value =  SM_DELTA_ITER;
				}
				else if(iteration_count_val == 1)
				{
					// Calculate the corrector state

					// Calculation for frequency deviation
					if (inverter_droop_fp) {
						pred_state.df_mea_delayed = 1.0/Tfreq_delay*(value_Frequency - pred_state.f_mea_delayed);
						curr_state.f_mea_delayed = curr_state.f_mea_delayed + (curr_state.df_mea_delayed + pred_state.df_mea_delayed) * deltath;
						// Calculate Pref based on teh droop curve
						double delta_Pref = ((curr_state.f_mea_delayed - freq_ref)/freq_ref) * (1 / R_fp) * p_rated * 3;
						power_diff_val = Pref_prev - (Pref0 - delta_Pref);
						if (checkRampRate_real) {
							if (power_diff_val > 0 && (power_diff_val > rampDownRate_real*3)) {
								Pref = Pref_prev - rampDownRate_real*3;
							}
							else if (power_diff_val < 0 && (-power_diff_val > rampUpRate_real*3)) {
								Pref = Pref_prev + rampUpRate_real*3;
							}
							else {
								Pref = Pref0 - delta_Pref;
							}
						}
						else {
							Pref = Pref0 - delta_Pref;

							if(Pref > 3 * p_rated){   //prevents from exceeding the rated power, 3 indicates for three phase
								Pref = 3 * p_rated;
							}


						}
						// Update the Pref and Qref values
						update_control_references();
					}
					// Calculation for voltage deviation
					if (inverter_droop_vq) {
						if((phases & 0x10) == 0x10) {
							pred_state.dV_mea_delayed[0] = 1.0/Tvol_delay*(value_Circuit_V[0].Mag() - pred_state.V_mea_delayed[0]);
							// Update current state Vmeasured_delayed
							curr_state.V_mea_delayed[0] = curr_state.V_mea_delayed[0] + (curr_state.dV_mea_delayed[0] + pred_state.dV_mea_delayed[0]) * deltath;

							// Update Qref for each phase
							double delta_Qref;
							delta_Qref = (curr_state.V_mea_delayed[0] - V_ref[0]) / node_nominal_voltage * (1.0 / R_vq) * p_rated;
							power_diff_val = Qref_prev[0] - (Qref0[0] - delta_Qref);
							if (checkRampRate_reactive) {
								if (power_diff_val > 0 && (power_diff_val > rampDownRate_reactive)) {
									Qref_PI[0] = Qref_prev[0] - rampDownRate_reactive;
								}
								else if (power_diff_val < 0 && (-power_diff_val > rampUpRate_reactive)) {
									Qref_PI[0] = Qref_prev[0] + rampUpRate_reactive;
								}
								else {
									Qref_PI[0] = Qref0[0] - delta_Qref;
								}
							}
							else {
								Qref_PI[0] = Qref0[0] - delta_Qref;
							}
						}
						else if((phases & 0x07) == 0x07) {
							pred_state.dV_mea_delayed[0] = 1.0/Tvol_delay*(value_Circuit_V[0].Mag() - pred_state.V_mea_delayed[0]);
							pred_state.dV_mea_delayed[1] = 1.0/Tvol_delay*(value_Circuit_V[1].Mag() - pred_state.V_mea_delayed[1]);
							pred_state.dV_mea_delayed[2] = 1.0/Tvol_delay*(value_Circuit_V[2].Mag() - pred_state.V_mea_delayed[2]);
							// Update current state Vmeasured_delayed
							curr_state.V_mea_delayed[0] = curr_state.V_mea_delayed[0] + (curr_state.dV_mea_delayed[0] + pred_state.dV_mea_delayed[0]) * deltath;
							curr_state.V_mea_delayed[1] = curr_state.V_mea_delayed[1] + (curr_state.dV_mea_delayed[1] + pred_state.dV_mea_delayed[1]) * deltath;
							curr_state.V_mea_delayed[2] = curr_state.V_mea_delayed[2] + (curr_state.dV_mea_delayed[2] + pred_state.dV_mea_delayed[2]) * deltath;
							// Update Qref for each phase
							double delta_Qref[3];
							for(i = 0; i < 3; i++) {
								delta_Qref[i] = (curr_state.V_mea_delayed[i] - V_ref[i]) / node_nominal_voltage * (1.0 / R_vq) * p_rated;
								power_diff_val = Qref_prev[i] - (Qref0[i] - delta_Qref[i]);
								if (checkRampRate_reactive) {
									if (power_diff_val > 0 && (power_diff_val > rampDownRate_reactive)) {
										Qref_PI[i] = Qref_prev[i] - rampDownRate_reactive;
									}
									else if (power_diff_val < 0 && (-power_diff_val > rampUpRate_reactive)) {
										Qref_PI[i] = Qref_prev[i] + rampUpRate_reactive;
									}
									else {
										Qref_PI[i] = Qref0[i] - delta_Qref[i];
									}
								}
								else {
									Qref_PI[i] = Qref0[i] - delta_Qref[i];
								}
							}
						}

						// Update the Pref and Qref values
						update_control_references();
					}

					// PI controller variables
					if ((phases & 0x10) == 0x10) {

						pred_state.P_Out[0] = (value_Circuit_V[0] * ~(I_Out[0])).Re();
						pred_state.Q_Out[0] = (value_Circuit_V[0] * ~(I_Out[0])).Im();

						if (Pref > 0) {
							int stop_temp = 0;
						}
						if (value_Circuit_V[0].Mag() > 0.0)
						{
							pred_state.ed[0] = ((~(gld::complex(Pref, Qref_PI[0])/(value_Circuit_V[0]))) - (~(gld::complex(pred_state.P_Out[0],pred_state.Q_Out[0])/(value_Circuit_V[0])))).Re();
							pred_state.eq[0] = ((~(gld::complex(Pref, Qref_PI[0])/(value_Circuit_V[0]))) - (~(gld::complex(pred_state.P_Out[0],pred_state.Q_Out[0])/(value_Circuit_V[0])))).Im();
						}
						else
						{
							pred_state.ed[0] = 0.0;
							pred_state.eq[0] = 0.0;
						}

						pred_state.ded[0] = (pred_state.ed[0] - curr_state.ed[0]) / deltat;
						pred_state.dmd[0] = (kpd * pred_state.ded[0]) + (kid * pred_state.ed[0]);
						curr_state.md[0] = curr_state.md[0] + (curr_state.dmd[0] + pred_state.dmd[0]) * deltath;
						curr_state.Idq[0].SetReal(curr_state.md[0] * I_In);

						pred_state.deq[0] = (pred_state.eq[0] - curr_state.eq[0]) / deltat;
						pred_state.dmq[0] = (kpq * pred_state.deq[0]) + (kiq * pred_state.eq[0]);
						curr_state.mq[0] = curr_state.mq[0] + (curr_state.dmq[0] + pred_state.dmq[0]) * deltath;
						curr_state.Idq[0].SetImag(curr_state.mq[0] * I_In);
						curr_state.Iac[0] = curr_state.Idq[0];

						//Voltage check
						if (value_Circuit_V[0].Mag() > 0.0)
						{
							//Compute the power output
							power_val[0] = (value_Circuit_V[0] * ~(curr_state.Iac[0]));
						}
						else
						{
							//Zero them
							curr_state.Iac[0] = gld::complex(0.0,0.0);
							power_val[0] = gld::complex(0.0,0.0);
						}

						//See if either method is enabled and update the power-reference
						if (checkRampRate_real || checkRampRate_reactive)
						{
							//Deflag the variable
							ramp_change = false;

							if (checkRampRate_real) {

								//Compute the difference - real part
								power_diff_val = (power_val[0].Re() - prev_VA_out[0].Re()) / deltat;

								if (power_val[0].Re() > prev_VA_out[0].Re())	//Ramp up
								{
									//See if it was too big
									if (power_diff_val > rampUpRate_real)
									{
										//Flag
										ramp_change = true;

										power_val[0].SetReal(prev_VA_out[0].Re() + (rampUpRate_real * deltat));
									}
									//Default else - was okay
								}
								else	//Ramp down
								{
									//See if it was too big
									if (power_diff_val < -rampDownRate_real)
									{
										//Flag
										ramp_change = true;

										power_val[0].SetReal(prev_VA_out[0].Re() - (rampDownRate_real * deltat));
									}
									//Default else - was okay
								}
							}

							if (checkRampRate_reactive) {

								//Compute the difference - reactive part
								power_diff_val = (power_val[0].Im() - prev_VA_out[0].Im()) / deltat;

								if (power_val[0].Im() > prev_VA_out[0].Im())	//Ramp up
								{
									//See if it was too big
									if (power_diff_val > rampUpRate_reactive)
									{
										//Flag
										ramp_change = true;

										power_val[0].SetImag(prev_VA_out[0].Im() + (rampUpRate_reactive * deltat));
									}
									//Default else - was okay
								}
								else	//Ramp down
								{
									//See if it was too big
									if (power_diff_val < -rampDownRate_reactive)
									{
										//Flag
										ramp_change = true;

										power_val[0].SetImag(prev_VA_out[0].Im() - (rampDownRate_reactive * deltat));
									}
									//Default else - was okay
								}
							}


							//Now "extrapolate" this back to a current value, if needed
							if (ramp_change)
							{
								//Compute a "new current" value
								if (value_Circuit_V[0].Mag() > 0.0)
								{
									temp_current_val[0] = ~(power_val[0] / value_Circuit_V[0]);
								}
								else
								{
									temp_current_val[0] = gld::complex(0.0,0.0);
								}

								// Update the output current values, as well as the current multipliers
								curr_state.Idq[0] = temp_current_val[0];
								curr_state.md[0] = curr_state.Idq[0].Re()/I_In;
								curr_state.mq[0] = curr_state.Idq[0].Im()/I_In;
								curr_state.Iac[0] = curr_state.Idq[0];

							}
							//Default else - no ramp change, so don't mess with anything

							//Store the updated power value
							curr_VA_out[0] = power_val[0];
						}


						// Before updating pLine_unrotI and Iout, need to check inverter real power output:
						// If not attached to the battery, need to check if real power < 0 or > rating
						VA_Out = value_Circuit_V[0] * ~(curr_state.Iac[0]);

						// Then continue update current
						value_Line_unrotI[0] += I_Out[0];
						value_Line_unrotI[0] += -curr_state.Iac[0];
						I_Out[0] = curr_state.Iac[0];

					}
					else if((phases & 0x07) == 0x07) {
						for(i = 0; i < 3; i++) {

							pred_state.P_Out[i] = (value_Circuit_V[i] * ~(I_Out[i])).Re();
							pred_state.Q_Out[i] = (value_Circuit_V[i] * ~(I_Out[i])).Im();

							if (Pref > 0) {
								int stop_temp = 0;
							}
							if (value_Circuit_V[i].Mag() > 0.0)
							{
								pred_state.ed[i] = ((~(gld::complex(Pref/3.0, Qref_PI[i])/(value_Circuit_V[i]))) - (~(gld::complex(pred_state.P_Out[i],pred_state.Q_Out[i])/(value_Circuit_V[i])))).Re();
								pred_state.eq[i] = ((~(gld::complex(Pref/3.0, Qref_PI[i])/(value_Circuit_V[i]))) - (~(gld::complex(pred_state.P_Out[i],pred_state.Q_Out[i])/(value_Circuit_V[i])))).Im();
							}
							else
							{
								pred_state.ed[i] = 0.0;
								pred_state.eq[i] = 0.0;
							}

							pred_state.ded[i] = (pred_state.ed[i] - curr_state.ed[i]) / deltat;
							pred_state.dmd[i] = (kpd * pred_state.ded[i]) + (kid * pred_state.ed[i]);
							curr_state.md[i] = curr_state.md[i] + (curr_state.dmd[i] + pred_state.dmd[i]) * deltath;
							curr_state.Idq[i].SetReal(curr_state.md[i] * I_In);

							pred_state.deq[i] = (pred_state.eq[i] - curr_state.eq[i]) / deltat;
							pred_state.dmq[i] = (kpq * pred_state.deq[i]) + (kiq * pred_state.eq[i]);
							curr_state.mq[i] = curr_state.mq[i] + (curr_state.dmq[i] + pred_state.dmq[i]) * deltath;
							curr_state.Idq[i].SetImag(curr_state.mq[i] * I_In);
							curr_state.Iac[i] = curr_state.Idq[i];

							//Voltage check
							if (value_Circuit_V[i].Mag() > 0.0)
							{
								//Compute the power output
								power_val[i] = (value_Circuit_V[i] * ~(curr_state.Iac[i]));
							}
							else
							{
								//Zero them
								curr_state.Iac[i] = gld::complex(0.0,0.0);
								power_val[i] = gld::complex(0.0,0.0);
							}

							//See if either method is enabled and update the power-reference
							if (checkRampRate_real || checkRampRate_reactive)
							{
								//Deflag the variable
								ramp_change = false;

								if (checkRampRate_real) {

									//Compute the difference - real part
									power_diff_val = (power_val[i].Re() - prev_VA_out[i].Re()) / deltat;

									if (power_val[i].Re() > prev_VA_out[i].Re())	//Ramp up
									{
										//See if it was too big
										if (power_diff_val > rampUpRate_real)
										{
											//Flag
											ramp_change = true;

											power_val[i].SetReal(prev_VA_out[i].Re() + (rampUpRate_real * deltat));
										}
										//Default else - was okay
									}
									else	//Ramp down
									{
										//See if it was too big
										if (power_diff_val < -rampDownRate_real)
										{
											//Flag
											ramp_change = true;

											power_val[i].SetReal(prev_VA_out[i].Re() - (rampDownRate_real * deltat));
										}
										//Default else - was okay
									}
								}

								if (checkRampRate_reactive) {

									//Compute the difference - reactive part
									power_diff_val = (power_val[i].Im() - prev_VA_out[i].Im()) / deltat;

									if (power_val[i].Im() > prev_VA_out[i].Im())	//Ramp up
									{
										//See if it was too big
										if (power_diff_val > rampUpRate_reactive)
										{
											//Flag
											ramp_change = true;

											power_val[i].SetImag(prev_VA_out[i].Im() + (rampUpRate_reactive * deltat));
										}
										//Default else - was okay
									}
									else	//Ramp down
									{
										//See if it was too big
										if (power_diff_val < -rampDownRate_reactive)
										{
											//Flag
											ramp_change = true;

											power_val[i].SetImag(prev_VA_out[i].Im() - (rampDownRate_reactive * deltat));
										}
										//Default else - was okay
									}
								}


								//Now "extrapolate" this back to a current value, if needed
								if (ramp_change)
								{
									//Compute a "new current" value
									if (value_Circuit_V[i].Mag() > 0.0)
									{
										temp_current_val[i] = ~(power_val[i] / value_Circuit_V[i]);
									}
									else
									{
										temp_current_val[i] = gld::complex(0.0,0.0);
									}

									// Update the output current values, as well as the current multipliers
									curr_state.Idq[i] = temp_current_val[i];
									curr_state.md[i] = curr_state.Idq[i].Re()/I_In;
									curr_state.mq[i] = curr_state.Idq[i].Im()/I_In;
									curr_state.Iac[i] = curr_state.Idq[i];

								}
								//Default else - no ramp change, so don't mess with anything

								//Store the updated power value
								curr_VA_out[i] = power_val[i];
							}
						}

						// Before updating pLine_unrotI and Iout, need to check inverter real power output:
						// If not attached to the battery, need to check if real power < 0 or > rating
						VA_Out = (value_Circuit_V[0] * ~(curr_state.Iac[0])) + (value_Circuit_V[1] * ~(curr_state.Iac[1])) + (value_Circuit_V[2] * ~(curr_state.Iac[2]));

						for (int i = 0; i< 3; i++) {

							value_Line_unrotI[i] += I_Out[i];
							value_Line_unrotI[i] += -curr_state.Iac[i];
							I_Out[i] = curr_state.Iac[i];
						}
					}

					// update the Pref and Qref values based on battery soc updated at iteration_count_val == 0
					// Previously did not do this update here at iteration_count_val == 1
					update_control_references();

					//Check phasing
					if ((phases & 0x10) == 0x10)
					{
						//Determine if we're ready to exit or not
						if ((fabs(curr_state.ded[0]) <= inverter_convergence_criterion) && (fabs(curr_state.deq[0]) <= inverter_convergence_criterion) && (simmode_return_value != SM_DELTA))
						{
							simmode_return_value = SM_EVENT;// we have reached steady state
						} else {
							simmode_return_value = SM_DELTA;
						}
					}
					else if ((phases & 0x07) == 0x07)
					{
						//Determine if we're ready to exit or not
						for(i = 0; i < 3; i++) {
							if ((fabs(curr_state.ded[i]) <= inverter_convergence_criterion) && (fabs(curr_state.deq[i]) <= inverter_convergence_criterion) && (simmode_return_value != SM_DELTA))
							{
								simmode_return_value = SM_EVENT;// we have reached steady state
							} else {
								simmode_return_value = SM_DELTA;
							}
						}
					}
				}
				else	//Additional iterations - basically just checks convergence
				{
					//Check phases
					if ((phases & 0x10) == 0x10)
					{
						//Duplicate the "next stage" check -- only here if something forces us onward
						if ((fabs(curr_state.ded[0]) <= inverter_convergence_criterion) && (fabs(curr_state.deq[0]) <= inverter_convergence_criterion) && (simmode_return_value != SM_DELTA))
						{
							simmode_return_value = SM_EVENT;// we have reached steady state
						} else {
							simmode_return_value = SM_DELTA;
						}
					}
					else if ((phases & 0x07) == 0x07)
					{
						//Duplicate the "next stage" check -- only here if something forces us onward
						for(i = 0; i < 3; i++) {
							if ((fabs(curr_state.ded[i]) <= inverter_convergence_criterion) && (fabs(curr_state.deq[i]) <= inverter_convergence_criterion) && (simmode_return_value != SM_DELTA))
							{
								simmode_return_value = SM_EVENT;// we have reached steady state
							} else {
								simmode_return_value = SM_DELTA;
							}
						}
					}
				}
			}
		}//PI controller
		else if (inverter_dyn_mode == PID_CONTROLLER)
		{
			//Only update on the first iteration - all others, meh (for simple implement)
			if (iteration_count_val == 0)
			{
				//Copy the values back
				prev_PID_state = curr_PID_state;

				if ((phases & 0x10) == 0x10)	//Triplex case
				{
					//Update the references, just in case
					curr_PID_state.phase_Pref = Pref;	//Real power setpoint
					curr_PID_state.phase_Qref = Qref;	//Imaginary power set point
				}
				else	//Must be three-phase
				{
					//Update the references, just in case
					curr_PID_state.phase_Pref = Pref / 3.0;	//Real power setpoint
					curr_PID_state.phase_Qref = Qref / 3.0;	//Imaginary power set point
				}

				//Update input current
				curr_PID_state.I_in = I_In;		//Input current

				//New timestep - also update the control references
				update_control_references();
			}
			//Default else - other loop

			//Reset the error
			curr_PID_state.max_error_val = 0.0;

			//Construct the power variable
			work_power_vals = gld::complex(curr_PID_state.phase_Pref,curr_PID_state.phase_Qref);

			//Determine our path to update
			if ((phases & 0x10) == 0x10)	//Triplex
			{
				if (value_Circuit_V[0].Mag() > 0.0)
				{
					//Determine the current set point - unrotated
					curr_PID_state.current_set_raw[0] = ~(work_power_vals / value_Circuit_V[0]);
				}
				else //Only you can prevent #IND
				{
					curr_PID_state.current_set_raw[0] = gld::complex(0.0,0.0);
				}

				//Find the current angle
				curr_PID_state.reference_angle[0] = value_Circuit_V[0].Arg();

				//Rotate the current into the reference frame
				curr_PID_state.current_set[0] = curr_PID_state.current_set_raw[0] * complex_exp(-1.0 * curr_PID_state.reference_angle[0]);

				//Compute the current current error - in the proper reference frame (adjust previous value)
				curr_PID_state.error[0] = curr_PID_state.current_set[0] - prev_PID_state.current_vals_ref[0];

				//Compute delta
				curr_PID_state.derror[0] = (curr_PID_state.error[0] - prev_PID_state.error[0]) / deltat;

				//Accumulate integrator
				curr_PID_state.integrator_vals[0] = prev_PID_state.integrator_vals[0] + (curr_PID_state.error[0] * deltat);

				//Form up the PID result - real
				temp_val_d = kpd * curr_PID_state.error[0].Re() + kid * curr_PID_state.integrator_vals[0].Re() + kdd * curr_PID_state.derror[0].Re();
				temp_val_q = kpq * curr_PID_state.error[0].Im() + kiq * curr_PID_state.integrator_vals[0].Im() + kdq * curr_PID_state.derror[0].Im();

				//Form it up as complex
				pid_out[0] = gld::complex(temp_val_d,temp_val_q);

				//Adjust the modulation factor
				curr_PID_state.mod_vals[0] = prev_PID_state.mod_vals[0] + pid_out[0];

				//Compute the new current out - in the reference frame
				curr_PID_state.current_vals_ref[0] = curr_PID_state.mod_vals[0] * curr_PID_state.I_in;
				
				//Now unrotate
				curr_PID_state.current_vals[0] = gld::complex(-1.0,0.0) * curr_PID_state.current_vals_ref[0] * complex_exp(curr_PID_state.reference_angle[0]);

				//Update the posting
				value_Line_unrotI[0] += -last_current[3] + curr_PID_state.current_vals[0];

				//Update other variable
				last_current[3] = curr_PID_state.current_vals[0];

				//Compute the error
				curr_PID_state.max_error_val = curr_PID_state.error[0].Mag();

				//Update VA out
				VA_Out = -(value_Circuit_V[0]*~curr_PID_state.current_vals[0]);

			}//End Triplex
			else	//Three-phase
			{
				//Compute the current current values
				for (indexval=0; indexval<3; indexval++)
				{
					if (value_Circuit_V[indexval].Mag() > 0.0)
					{
						//Determine the current set point - unrotated
						curr_PID_state.current_set_raw[indexval] = ~(work_power_vals / value_Circuit_V[indexval]);
					}
					else //Only you can prevent #IND
					{
						curr_PID_state.current_set_raw[indexval] = gld::complex(0.0,0.0);
					}

					//Find the current angle
					curr_PID_state.reference_angle[indexval] = value_Circuit_V[indexval].Arg();

					//Rotate the current into the reference frame
					curr_PID_state.current_set[indexval] = curr_PID_state.current_set_raw[indexval] * complex_exp(-1.0 * curr_PID_state.reference_angle[indexval]);

					//Compute the current current error - in the proper reference frame (adjust previous value)
					curr_PID_state.error[indexval] = curr_PID_state.current_set[indexval] - prev_PID_state.current_vals_ref[indexval];

					//Compute delta
					curr_PID_state.derror[indexval] = (curr_PID_state.error[indexval] - prev_PID_state.error[indexval]) / deltat;

					//Accumulate integrator
					curr_PID_state.integrator_vals[indexval] = prev_PID_state.integrator_vals[indexval] + (curr_PID_state.error[indexval] * deltat);

					//Form up the PID result - real
					temp_val_d = kpd * curr_PID_state.error[indexval].Re() + kid * curr_PID_state.integrator_vals[indexval].Re() + kdd * curr_PID_state.derror[indexval].Re();
					temp_val_q = kpq * curr_PID_state.error[indexval].Im() + kiq * curr_PID_state.integrator_vals[indexval].Im() + kdq * curr_PID_state.derror[indexval].Im();

					//Form it up as complex
					pid_out[indexval] = gld::complex(temp_val_d,temp_val_q);

					//Adjust the modulation factor
					curr_PID_state.mod_vals[indexval] = prev_PID_state.mod_vals[indexval] + pid_out[indexval];

					//Compute the new current out - in the reference frame
					curr_PID_state.current_vals_ref[indexval] = curr_PID_state.mod_vals[indexval] * curr_PID_state.I_in;
					
					//Now unrotate
					curr_PID_state.current_vals[indexval] = gld::complex(-1.0,0.0) * curr_PID_state.current_vals_ref[indexval] * complex_exp(curr_PID_state.reference_angle[indexval]);

					//Update the posting
					value_Line_unrotI[indexval] += -last_current[indexval] + curr_PID_state.current_vals[indexval];

					//Update other variable
					last_current[indexval] = curr_PID_state.current_vals[indexval];

					//Compare the error
					if (curr_PID_state.error[indexval].Mag() > curr_PID_state.max_error_val)
					{
						curr_PID_state.max_error_val = curr_PID_state.error[indexval].Mag();
					}
				}

				//Update VA_Out
				VA_Out = -(value_Circuit_V[0]*~curr_PID_state.current_vals[0] + value_Circuit_V[1]* ~curr_PID_state.current_vals[1] + value_Circuit_V[2]*~curr_PID_state.current_vals[2]);

			}//End three-phase

			//Check the error
			if (curr_PID_state.max_error_val > inverter_convergence_criterion)
			{
				simmode_return_value =  SM_DELTA;
			}
			else
			{
				simmode_return_value =  SM_EVENT;
			}
		}
		else	//Some other method, we don't care
		{
			simmode_return_value =  SM_EVENT;
		}
	}//End inverter is still "enabled"
	else	//Disabled, allow no posting
	{
		//Basically remove our contributions (if any) - similar to post-update
		if (inverter_dyn_mode == PI_CONTROLLER)
		{
			if (four_quadrant_control_mode == FQM_VSI)	//VSI mode
			{
				if((phases & 0x10) == 0x10){
					value_IGenerated[0] = gld::complex(0.0,0.0);

					//Zero the output trackers
					I_Out[0] = gld::complex(0.0,0.0);
				}
				else if((phases & 0x07) == 0x07)
				{
					value_IGenerated[0] = gld::complex(0.0,0.0);
					value_IGenerated[1] = gld::complex(0.0,0.0);
					value_IGenerated[2] = gld::complex(0.0,0.0);

					//Zero the output trackers
					I_Out[0] = I_Out[1] = I_Out[2] = gld::complex(0.0,0.0);
				}
			}
			else	//Other modes
			{
				if((phases & 0x10) == 0x10){
					value_Line_unrotI[0] += I_Out[0];
					
					//Zero the output trackers
					I_Out[0] = gld::complex(0.0,0.0);
				} else if((phases & 0x07) == 0x07) {
					value_Line_unrotI[0] += I_Out[0];
					value_Line_unrotI[1] += I_Out[1];
					value_Line_unrotI[2] += I_Out[2];

					//Zero the output trackers
					I_Out[0] = I_Out[1] = I_Out[2] = gld::complex(0.0,0.0);
				}
			}
		}
		else if (inverter_dyn_mode == PID_CONTROLLER)
		{
			if((phases & 0x10) == 0x10)
			{
				value_Line_unrotI[0] -= last_current[3];

				//Zero the output tracker
				last_current[3] = gld::complex(0.0,0.0);
			}
			else if((phases & 0x07) == 0x07)
			{
				value_Line_unrotI[0] -= last_current[0];
				value_Line_unrotI[1] -= last_current[1];
				value_Line_unrotI[2] -= last_current[2];

				//Zero the output trackers
				last_current[0] = last_current[1] = last_current[2] = gld::complex(0.0,0.0);
			}
		}
		//Default else, who knows

		//Zero the output variable too - for giggles
		VA_Out = complex(0.0,0.0);

		//We're disabled - unless something else wants deltamode, we're done
		simmode_return_value = SM_EVENT;
	}//End Inverter disabled

	//Sync the powerflow variables
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values();
	}

	//Perform a check to determine how to go forward
	if (enable_1547_compliance)
	{
		//See if our return is value
		if ((ieee_1547_double > 0.0) && (ieee_1547_double < 1.7) && (simmode_return_value == SM_EVENT))
		{
			//Set the mode tracking variable for this exit
			desired_simulation_mode = SM_DELTA;

			//Force us to stay
			return SM_DELTA;
		}
		else	//Just return whatever we were going to do
		{
			//Set the mode tracking variable for this exit
			desired_simulation_mode = simmode_return_value;

			return simmode_return_value;
		}
	}
	else	//Normal mode
	{
		//Set the mode tracking variable for this exit
		desired_simulation_mode = simmode_return_value;

		return simmode_return_value;
	}
}


//Module-level post update call

STATUS inverter::post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass)
{
	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		reset_complex_powerflow_accumulators();
	}

	if (inverter_dyn_mode == PI_CONTROLLER)
	{
		if (four_quadrant_control_mode != FQM_VSI) {
			if((phases & 0x10) == 0x10){
				value_Line_unrotI[0] = I_Out[0];
			} else if((phases & 0x07) == 0x07) {
				value_Line_unrotI[0] = I_Out[0];
				value_Line_unrotI[1] = I_Out[1];
				value_Line_unrotI[2] = I_Out[2];
			}
		}
		else {

			VA_Out_past = VA_Out; // Update VA_Out_past

			if (VSI_mode == VSI_DROOP) {
				//Do not need to update output power
				// Update reference power values P_OUT and Q_OUT based on current VA outputs
				P_Out = VA_Out.Re();
				Q_Out = VA_Out.Im();

			}
		}
	}
	else if (inverter_dyn_mode == PID_CONTROLLER)
	{
		if((phases & 0x10) == 0x10){
			value_Line_unrotI[0] -= last_current[3];
		} else if((phases & 0x07) == 0x07) {
			value_Line_unrotI[0] -= last_current[0];
			value_Line_unrotI[1] -= last_current[1];
			value_Line_unrotI[2] -= last_current[2];
		}
	}
	//Default else, who knows

	//Do the power sync, but only do it for these two cases (shouldn't hurt others, by why bother)
	if ((inverter_dyn_mode == PI_CONTROLLER) || (inverter_dyn_mode == PID_CONTROLLER))
	{
		//Should have a parent, but be paranoid
		if (parent_is_a_meter)
		{
			push_complex_powerflow_values();
		}
	}

	return SUCCESS;	//Always succeeds right now
}

//Initializes dynamic equations for first entry
//Returns a SUCCESS/FAIL
//curr_time is the initial states/information
STATUS inverter::init_PI_dynamics(INV_STATE *curr_time)
{
	gld::complex prev_Idq[3];

	//Pull the powerflow values
	if (parent_is_a_meter)
	{
		reset_complex_powerflow_accumulators();

		pull_complex_powerflow_values();
	}

	if (four_quadrant_control_mode != FQM_VSI)
	{
		//Find the initial state of the inverter
		//Find the initial error from a steady state modulation value
		if((phases & 0x10) == 0x10) { //Single Phase
			Pref = VA_Out.Re();
			Pref0 = Pref;
			prev_Idq[0] = last_I_Out[0];

			if(last_I_In > 1e-9) {
				curr_time->md[0] = prev_Idq[0].Re()/last_I_In;
			} else {
				curr_time->md[0] = 0.0;
			}
			curr_time->Idq[0].SetReal(curr_time->md[0] * I_In);

			Qref = VA_Out.Im();
			if(last_I_In > 1e-9) {
				curr_time->mq[0] = prev_Idq[0].Im()/last_I_In;
			} else {
				curr_time->mq[0] = 0.0;
			}
			curr_time->Idq[0].SetImag(curr_time->mq[0] * I_In);
			curr_time->Iac[0] = curr_time->Idq[0];

			//Post the value
			if (four_quadrant_control_mode != FQM_VSI)
			{
				value_Line_unrotI[0] = -curr_time->Iac[0];
			}
			else {
				value_IGenerated[0] = -curr_time->Iac[0];
			}

		} else if((phases & 0x07) == 0x07) { // Three Phase
			Pref = VA_Out.Re();
			Qref = VA_Out.Im();
			Pref0 = Pref;

			for(int i = 0; i < 3; i++){

				//Usually occurs in presync, but causes some dynamics issues
				last_I_Out[i] = I_Out[i];

				prev_Idq[i] = last_I_Out[i];

				if(last_I_In > 1e-9) {
					curr_time->md[i] = prev_Idq[i].Re()/last_I_In;
					curr_time->mq[i] = prev_Idq[i].Im()/last_I_In;
				} else {
					curr_time->md[i] = 0.0;
					curr_time->mq[i] = 0.0;
				}
				curr_time->Idq[i].SetReal(curr_time->md[i] * I_In);
				curr_time->Idq[i].SetImag(curr_time->mq[i] * I_In);
				curr_time->Iac[i] = curr_time->Idq[i];

				//Post the value
				if (four_quadrant_control_mode != FQM_VSI)
				{
					value_Line_unrotI[i] = -curr_time->Iac[i];
				}
				else {
					value_IGenerated[i] = -curr_time->Iac[i];
				}
			}
		}

		//Take care of the ramp rate items, if needed
		if (checkRampRate_real || checkRampRate_reactive)
		{
			if ((phases & 0x10) == 0x10)
			{
				curr_VA_out[0] = value_Circuit_V[0] * ~(I_Out[0]);

				//Initialize the old one too
				prev_VA_out[0] = curr_VA_out[0];
			}
			else if ((phases & 0x07) == 0x07)
			{
				curr_VA_out[0] = value_Circuit_V[0] * ~(I_Out[0]);
				curr_VA_out[1] = value_Circuit_V[1] * ~(I_Out[1]);
				curr_VA_out[2] = value_Circuit_V[2] * ~(I_Out[2]);

				//Initialize the old ones too
				prev_VA_out[0] = curr_VA_out[0];
				prev_VA_out[1] = curr_VA_out[1];
				prev_VA_out[2] = curr_VA_out[2];
			}
		}

		// Obtain original Qref values for each phase, so that each phase of voltage can be controlled seperately later
		if((phases & 0x10) == 0x10) {
			Qref0[0] = (value_Circuit_V[0] * ~(I_Out[0])).Im();
			Qref_PI[0] = Qref0[0];
			Qref_PI[1] = 0;
			Qref_PI[2] = 0;
		}
		if((phases & 0x07) == 0x07) {
			Qref0[0] = (value_Circuit_V[0] * ~(I_Out[0])).Im();
			Qref0[1] = (value_Circuit_V[1] * ~(I_Out[1])).Im();
			Qref0[2] = (value_Circuit_V[2] * ~(I_Out[2])).Im();
			Qref_PI[0] = (value_Circuit_V[0] * ~(I_Out[0])).Im();
			Qref_PI[1] = (value_Circuit_V[1] * ~(I_Out[1])).Im();
			Qref_PI[2] = (value_Circuit_V[2] * ~(I_Out[2])).Im();

		}

		// Update P_Out and Q_Out based on the initial value entering delta mode
		P_Out = Pref0;
		Q_Out = Qref0[0] + Qref0[1] + Qref0[2];

		// Delayed frequency initial value is given:
		if (inverter_droop_fp) {
			curr_time->f_mea_delayed = value_Frequency;
		}

		// Set Voltage initial value for v/q droop if adopted
		if((phases & 0x10) == 0x10) {
			curr_time->V_mea_delayed[0] = value_Circuit_V[0].Mag();
			V_ref[0] = value_Circuit_V[0].Mag();
		}
		if((phases & 0x07) == 0x07) {
			curr_time->V_mea_delayed[0] = value_Circuit_V[0].Mag();
			curr_time->V_mea_delayed[1] = value_Circuit_V[1].Mag();
			curr_time->V_mea_delayed[2] = value_Circuit_V[2].Mag();
			V_ref[0] = value_Circuit_V[0].Mag();
			V_ref[1] = value_Circuit_V[1].Mag();
			V_ref[2] = value_Circuit_V[2].Mag();
		}
	}
	else {
		if((phases & 0x10) == 0x10) {
			//Update output power
			//Get current injected
			temp_current_val[0] = value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0];

			//Update power output variables, just so we can see what is going on
			VA_Out = value_Circuit_V[0]*~temp_current_val[0];

			//Take care of the ramp rate items, if needed
			if (checkRampRate_real || checkRampRate_reactive)
			{
				curr_VA_out[0] = VA_Out;

				//Initialize the old ones too
				prev_VA_out[0] = curr_VA_out[0];
			}

			e_source[0] = (value_IGenerated[0] * gld::complex(Rfilter,Xfilter) * Zbase);
			V_angle[0] = (e_source[0]).Arg();  // Obtain the inverter terminal voltage phasor angle
			V_angle_past[0] = V_angle[0];

			if (VSI_mode == VSI_DROOP)
			{
				V_mag_ref[0] = value_Circuit_V[0].Mag() + (VA_Out.Im() - Qref) / p_rated / 3.0 * R_vq * node_nominal_voltage ;	// Initialize the V_mag_ref[0]
				curr_time->V_StateVal[0] = e_source[0].Mag();
				curr_time->e_source_mag[0] = e_source[0].Mag();
			}
			else {
				V_mag_ref[0] = value_Circuit_V[0].Mag();	// record the terminal voltage magtitude for isochronous VSI mode since it is what we want to control
				V_mag[0] = V_mag_ref[0];
				curr_time->V_StateVal[0] = e_source[0].Mag();
				curr_time->e_source_mag[0] = e_source[0].Mag();
			}
		}
		if((phases & 0x07) == 0x07) {
			//Update output power
			//Get current injected
			temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0] - generator_admittance[0][1]*value_Circuit_V[1] - generator_admittance[0][2]*value_Circuit_V[2]);
			temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0]*value_Circuit_V[0] - generator_admittance[1][1]*value_Circuit_V[1] - generator_admittance[1][2]*value_Circuit_V[2]);
			temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0]*value_Circuit_V[0] - generator_admittance[2][1]*value_Circuit_V[1] - generator_admittance[2][2]*value_Circuit_V[2]);

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0]*~temp_current_val[0];
			power_val[1] = value_Circuit_V[1]*~temp_current_val[1];
			power_val[2] = value_Circuit_V[2]*~temp_current_val[2];

			VA_Out = power_val[0] + power_val[1] + power_val[2];

			//Take care of the ramp rate items, if needed
			if (checkRampRate_real || checkRampRate_reactive)
			{
				curr_VA_out[0] = power_val[0];
				curr_VA_out[1] = power_val[1];
				curr_VA_out[2] = power_val[2];

				//Initialize the old ones too
				prev_VA_out[0] = curr_VA_out[0];
				prev_VA_out[1] = curr_VA_out[1];
				prev_VA_out[2] = curr_VA_out[2];
			}

			//Compute the average terminal voltage
			pCircuit_V_Avg = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0; //average value of 3 phase terminal voltage

			for (int i = 0; i < 3; i++) {
				e_source[i] = (value_IGenerated[i] * gld::complex(Rfilter,Xfilter) * Zbase);
				V_angle[i] = (e_source[i]).Arg();  // Obtain the inverter terminal voltage phasor angle
				V_angle_past[i] = V_angle[i];
				if (VSI_mode == VSI_DROOP) {
					V_mag_ref[i] = pCircuit_V_Avg + (VA_Out.Im() - Qref) / p_rated / 3.0 * R_vq * node_nominal_voltage;	// Initialize V_mag_ref[i]
					curr_time->V_StateVal[i] = e_source[i].Mag();
					curr_time->e_source_mag[i] = e_source[i].Mag();
				}
				else {
					V_mag_ref[i] = value_Circuit_V[i].Mag();	// record the terminal voltage magtitude for isochronous VSI mode since it is what we want to control
					V_mag[i] = V_mag_ref[i];
					curr_time->V_StateVal[i] = e_source[i].Mag();
					curr_time->e_source_mag[i] = e_source[i].Mag();
				}
			}
		}

		// Set measured real and reactive power initial value for VSI inverter
		curr_time->p_mea_delayed = VA_Out.Re();
		curr_time->q_mea_delayed = VA_Out.Im();
		Pref = VA_Out.Re();
		Qref = VA_Out.Im();

		VA_Out_past = VA_Out;
	}

	//Sync the powerflow variables
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values();
	}

	//Set the mode tracking variable to a default - not really needed, but be paranoid
	desired_simulation_mode = SM_EVENT;

	return SUCCESS;	//Always succeeds for now, but could have error checks later
}

//Initializes dynamic equations for first entry
//Returns a SUCCESS/FAIL
//PID-controller implementation
STATUS inverter::init_PID_dynamics(void)
{
	int indexx;

	//Pull the powerflow values, if needed
	if (parent_is_a_meter)
	{
		reset_complex_powerflow_accumulators();

		pull_complex_powerflow_values();
	}
	
	//Copy over the set-points -- this may need to be adjusted in the future
	Pref = VA_Out.Re();
	Qref = VA_Out.Im();

	//Copy the current input
	curr_PID_state.I_in = I_In;

	//Check the phases to see how to populate
	if ( (phases & 0x10) == 0x10 ) // split phase
	{
		//Copy in current set point
		curr_PID_state.phase_Pref = VA_Out.Re();
		curr_PID_state.phase_Qref = VA_Out.Im();

		//If ramp tracking, save the value
		if (checkRampRate_real)
		{
			curr_VA_out[0] = VA_Out;

			//Initialize the old ones too
			prev_VA_out[0] = curr_VA_out[0];
		}
	
		//Zero items and compute current output and modulation index
		for (indexx=0; indexx<3; indexx++)
		{
			//Zero everything, just because
			curr_PID_state.reference_angle[indexx] = 0.0;
			curr_PID_state.error[indexx] = gld::complex(0.0,0.0);
			curr_PID_state.integrator_vals[indexx] = gld::complex(0.0,0.0);
			curr_PID_state.derror[indexx] = gld::complex(0.0,0.0);
			curr_PID_state.current_set_raw[indexx] = gld::complex(0.0,0.0);
			curr_PID_state.current_set[indexx] = gld::complex(0.0,0.0);
			curr_PID_state.current_vals[indexx] = gld::complex(0.0,0.0);
			curr_PID_state.current_vals_ref[indexx] = gld::complex(0.0,0.0);
			curr_PID_state.mod_vals[indexx] = gld::complex(0.0,0.0);
		}
			
		//Populate the initial "reference angle"
		curr_PID_state.reference_angle[0] = value_Circuit_V[0].Arg();

		if (value_Circuit_V[0].Mag() > 0.0)
		{
			//Calculate the current set-point -- should be the same
			curr_PID_state.current_set_raw[0] = ~(gld::complex(curr_PID_state.phase_Pref,curr_PID_state.phase_Qref)/value_Circuit_V[0]);
		}
		else //Only you can prevent #IND
		{
			curr_PID_state.current_set_raw[0] = gld::complex(0.0,0.0);
		}

		//Rotate it
		curr_PID_state.current_set[0] = curr_PID_state.current_set_raw[0] * complex_exp(-1.0 * curr_PID_state.reference_angle[0]);
		
		//Copy in current that was posted
		curr_PID_state.current_vals[0] = last_current[3];

		//For completion, rotate this into this reference frame
		curr_PID_state.current_vals_ref[0] = gld::complex(-1.0,0.0) * curr_PID_state.current_vals[0] * complex_exp(-1.0 * curr_PID_state.reference_angle[0]);

		//Compute base modulation value - these are in the reference frame
		curr_PID_state.mod_vals[0] = gld::complex((curr_PID_state.current_vals_ref[0].Re() / curr_PID_state.I_in),(curr_PID_state.current_vals_ref[0].Im() / curr_PID_state.I_in));

		//Add in the last current too - PostSync removed it, so this will fix it for the logic in interupdate
		value_Line_unrotI[0] = last_current[3];
	
	}//End triplex-connected
	else
	{
		//Copy in current set point
		curr_PID_state.phase_Pref = VA_Out.Re() / 3.0;
		curr_PID_state.phase_Qref = VA_Out.Im() / 3.0;

		//If ramp tracking, save the value
		if (checkRampRate_real)
		{
			curr_VA_out[0] = gld::complex(curr_PID_state.phase_Pref,curr_PID_state.phase_Qref);
			curr_VA_out[1] = gld::complex(curr_PID_state.phase_Pref,curr_PID_state.phase_Qref);
			curr_VA_out[2] = gld::complex(curr_PID_state.phase_Pref,curr_PID_state.phase_Qref);

			//Initialize the old ones too
			prev_VA_out[0] = curr_VA_out[0];
			prev_VA_out[1] = curr_VA_out[1];
			prev_VA_out[1] = curr_VA_out[2];
		}

		//Zero items and compute current output and modulation index
		for (indexx=0; indexx<3; indexx++)
		{
			//Zero current - prev will get done by loop routine
			curr_PID_state.error[indexx] = gld::complex(0.0,0.0);
			curr_PID_state.integrator_vals[indexx] = gld::complex(0.0,0.0);
			curr_PID_state.derror[indexx] = gld::complex(0.0,0.0);
			
			//Populate the initial "reference angle"
			curr_PID_state.reference_angle[indexx] = value_Circuit_V[indexx].Arg();


			if (value_Circuit_V[indexx].Mag() > 0.0)
			{
				//Calculate the current set-point -- should be the same
				curr_PID_state.current_set_raw[indexx] = ~(gld::complex(curr_PID_state.phase_Pref,curr_PID_state.phase_Qref)/value_Circuit_V[indexx]);
			}
			else //Only you can prevent #IND
			{
				curr_PID_state.current_set_raw[indexx] = gld::complex(0.0,0.0);
			}

			//Rotate it
			curr_PID_state.current_set[indexx] = curr_PID_state.current_set_raw[indexx] * complex_exp(-1.0 * curr_PID_state.reference_angle[indexx]);
			
			//Copy in current that was posted
			curr_PID_state.current_vals[indexx] = last_current[indexx];

			//For completion, rotate this into this reference frame
			curr_PID_state.current_vals_ref[indexx] = gld::complex(-1.0,0.0) * curr_PID_state.current_vals[indexx] * complex_exp(-1.0 * curr_PID_state.reference_angle[indexx]);

			//Compute base modulation value - these are in the reference frame
			if (curr_PID_state.I_in != 0.0)
			{
				curr_PID_state.mod_vals[indexx] = gld::complex((curr_PID_state.current_vals_ref[indexx].Re() / curr_PID_state.I_in),(curr_PID_state.current_vals_ref[indexx].Im() / curr_PID_state.I_in));
			}
			else
			{
				curr_PID_state.mod_vals[indexx] = gld::complex(0.0,0.0);
			}

			//Add in the last current too - PostSync removed it, so this will fix it for the logic in interupdate
			value_Line_unrotI[indexx] = last_current[indexx];
		}
	}

	//Sync the powerflow variables
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values();
	}

	//Set the mode tracking variable to a default - not really needed, but be paranoid
	desired_simulation_mode = SM_EVENT;

	return SUCCESS;	//Always succeeds for now, but could have error checks later
}


void inverter::update_control_references(void)
{
	//FOUR_QUADRANT model (originally written for NAS/CES, altered for PV)
	double VA_Efficiency, temp_PF, temp_QVal;
	gld::complex temp_VA, VA_Outref;
	gld::complex battery_power_out = gld::complex(0,0);
	OBJECT *obj = OBJECTHDR(this);
	bool VA_changed = false; // A flag indicating whether VAref is changed due to limitations

	//Compute power in
	P_In = V_In * I_In;

	//Determine how to efficiency weight it
	if(!use_multipoint_efficiency)
	{
		//Normal scaling
		VA_Efficiency = P_In * efficiency;
	}
	else
	{
		//See if above minimum DC power input
		if(P_In <= p_so)
		{
			VA_Efficiency = 0.0;	//Nope, no output
		}
		else	//Yes, apply effiency change
		{
			//Make sure voltage isn't too low
			if(fabs(V_In) > v_dco)
			{
				gl_warning("The dc voltage is greater than the specified maximum for the inverter. Efficiency model may be inaccurate.");
				/*  TROUBLESHOOT
				The DC voltage at the input to the inverter is less than the maximum voltage supported by the inverter.  As a result, the
				multipoint efficiency model may not provide a proper result.
				*/
			}

			//Compute coefficients for multipoint efficiency
			C1 = p_dco*(1+c_1*(V_In-v_dco));
			C2 = p_so*(1+c_2*(V_In-v_dco));
			C3 = c_o*(1+c_3*(V_In-v_dco));

			//Apply this to the output
			VA_Efficiency = (((p_max/(C1-C2))-C3*(C1-C2))*(P_In-C2)+C3*(P_In-C2)*(P_In-C2));
		}
	}

	//Determine 4 quadrant outputs
	if(four_quadrant_control_mode == FQM_CONSTANT_PF)	//Power factor mode
	{
		if(power_factor != 0.0)	//Not purely imaginary
		{
			if (P_In<0.0)	//Discharge at input, so must be "load"
			{
				//Total power output is the magnitude
				VA_Outref.SetReal(VA_Efficiency*-1.0);
			}
			else if (P_In>0.0)	//Positive input, so must be generator
			{
				//Total power output is the magnitude
				VA_Outref.SetReal(VA_Efficiency);
			}
			else
			{
				VA_Outref.SetReal(0.0);
			}

			//Apply power factor sign properly - + sign is lagging in, which is proper here
			//Treat like a normal load right now
			if (power_factor < 0)
			{
				VA_Outref.SetImag((VA_Efficiency/sqrt(power_factor*power_factor))*sqrt(1.0-(power_factor*power_factor)));
			}
			else	//Must be positive
			{
				VA_Outref.SetImag((VA_Efficiency/sqrt(power_factor*power_factor))*-1.0*sqrt(1.0-(power_factor*power_factor)));
			}
		}
		else	//Purely imaginary value
		{
			VA_Outref = gld::complex(0.0,VA_Efficiency);
		}
	}
	else if (four_quadrant_control_mode == FQM_CONSTANT_PQ)
	{
		// If not attached to the battery, need to check if real power < 0 or > rating
		if (b_soc == -1) {
			if (Pref < 0) {
				Pref = 0;
			}
		}

		//Compute desired output - sign convention appears to be backwards
		if (inverter_dyn_mode == PI_CONTROLLER) {
			temp_VA = gld::complex(Pref,Qref_PI[0]+Qref_PI[1]+Qref_PI[2]); // For PI control mode, Qref is seperated for each phase
		}
		else {
			temp_VA = gld::complex(Pref, Qref); // previously was set as P_out + jQ_out. Since P_Out and Q_Out are constant, not reflecting change of output
		}


		//Ensuring battery has capacity to charge or discharge as needed.
		if ((b_soc >= 1.0) && (temp_VA.Re() < 0) && (b_soc != -1))	//Battery full and positive influx of real power
		{
			gl_warning("inverter:%s - battery full - no charging allowed",obj->name);
			temp_VA.SetReal(0.0);	//Set to zero - reactive considerations may change this
			VA_changed = true;
		}
		else if ((b_soc <= soc_reserve) && (temp_VA.Re() > 0) && (b_soc != -1))	//Battery "empty" and attempting to extract real power
		{
			gl_warning("inverter:%s - battery at or below the SOC reserve - no discharging allowed",obj->name);
			temp_VA.SetReal(0.0);	//Set output to zero - again, reactive considerations may change this
			VA_changed = true;
		}

		//Ensuring power rating of inverter is not exceeded.
		if (fabs(temp_VA.Mag()) > p_max ){ //Requested power output (P_Out, Q_Out) is greater than inverter rating
			VA_changed = true;
			if (p_max > fabs(temp_VA.Re())) //Can we reduce the reactive power output and stay within the inverter rating?
			{
				//Determine the Q we can provide
				temp_QVal = sqrt((p_max*p_max) - (temp_VA.Re()*temp_VA.Re()));

				//Assign to output, negating signs as necessary (temp_VA already negated)
				if (temp_VA.Im() < 0.0)	//Negative Q dispatch
				{
					VA_Outref = gld::complex(temp_VA.Re(),-temp_QVal);
				}
				else	//Positive Q dispatch
				{
					VA_Outref = gld::complex(temp_VA.Re(),temp_QVal);
				}
			}
			else	//Inverter rated power is equal to or smaller than real power desired, give it all we can
			{
				//Maintain desired sign convention
				if (temp_VA.Re() < 0.0)
				{
					VA_Outref = gld::complex(-p_max,0.0);
				}
				else	//Positive
				{
					VA_Outref = gld::complex(p_max,0.0);
				}
			}
		}
		else	//Doesn't exceed, assign it
		{
			VA_Outref = temp_VA;
		}


		//Update values to represent what is being pulled (battery uses for SOC updates) - assumes only storage
		//p_in used by battery - appears reversed to VA_Outref
		if (VA_Outref.Re() > 0.0)	//Discharging
		{
			p_in = VA_Outref.Re()/inv_eta;
		}
		else if (VA_Outref.Re() == 0.0)	//Idle
		{
			p_in = 0.0;
		}
		else	//Must be positive, so charging
		{
			p_in = VA_Outref.Re()*inv_eta;
		}
	}

	//check to see if VA_Outref is within rated absolute power rating
	if(VA_Outref.Mag() > p_max)
	{
		VA_changed = true;
		//Determine the excess, for use elsewhere - back out simple efficiencies
		excess_input_power = (VA_Outref.Mag() - p_max);

		//Apply thresholding - going on the assumption of maintaining vector direction
		if (four_quadrant_control_mode == FQM_CONSTANT_PF)
		{
			temp_PF = power_factor;
		}
		else	//Extract it - overall value (signs handled separately)
		{
			temp_PF = VA_Outref.Re()/VA_Outref.Mag();
		}

		//Compute the "new" output - signs lost
		temp_VA = gld::complex(fabs(p_max*temp_PF),fabs(p_max*sqrt(1.0-(temp_PF*temp_PF))));

		//"Sign" it appropriately
		if ((VA_Outref.Re()<0) && (VA_Outref.Im()<0))	//-R, -I
		{
			VA_Outref = -temp_VA;
		}
		else if ((VA_Outref.Re()<0) && (VA_Outref.Im()>=0))	//-R,I
		{
			VA_Outref = gld::complex(-temp_VA.Re(),temp_VA.Im());
		}
		else if ((VA_Outref.Re()>=0) && (VA_Outref.Im()<0))	//R,-I
		{
			VA_Outref = gld::complex(temp_VA.Re(),-temp_VA.Im());
		}
		else	//R,I
		{
			VA_Outref = temp_VA;
		}
	}
	else	//Not over, zero "overrage"
	{
		excess_input_power = 0.0;
	}
	// update references
	Pref = VA_Outref.Re();
	Qref = VA_Outref.Im();

	// Update Qref for PI control mode if VA_outref is changed due to limitations
	if (VA_changed) {
		if((phases & 0x10) == 0x10) {
			Qref_PI[0] = VA_Outref.Im();
			Qref_PI[1] = 0;
			Qref_PI[2] = 0;
		}
		if((phases & 0x07) == 0x07) {
			Qref_PI[0] = VA_Outref.Im()/3;
			Qref_PI[1] = VA_Outref.Im()/3;
			Qref_PI[2] = VA_Outref.Im()/3;
		}
	}
}

//Functionalized IEEE 1547 initalizer - mostly so it can easily be done in either deltamode or ss
STATUS inverter::initalize_IEEE_1547_checks(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	gld_property *temp_nominal_pointer;

	//Check parents and map the variables
	//@TODO - Note this checks more than the parent requirement above - probably should reconcile this sometime
	if (gl_object_isa(parent,"node","powerflow") ||
	gl_object_isa(parent,"load","powerflow") ||
	gl_object_isa(parent,"meter","powerflow") ||
	gl_object_isa(parent,"triplex_node","powerflow") ||
	gl_object_isa(parent,"triplex_load","powerflow") ||
	gl_object_isa(parent,"triplex_load","powerflow"))
	{
		//Link to nominal voltage
		temp_nominal_pointer = new gld_property(parent,"nominal_voltage");

		//Make sure it worked
		if (!temp_nominal_pointer->is_valid() || !temp_nominal_pointer->is_double())
		{
			gl_error("Inverter:%d %s failed to map the nominal_voltage property",obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to map the nominal_voltage property, an error occurred.  Please try again.
			If the error persists, please submit your GLM and a bug report to the ticketing system.
			*/

			return FAILED;
		}
		//Default else, it worked

		//Copy that value out
		node_nominal_voltage = temp_nominal_pointer->get_double();

		//Remove the property pointer
		delete temp_nominal_pointer;

		//See if we really want IEEE 1547, not A
		if (ieee_1547_version == IEEE1547)
		{
			//Adjust the values - high values
			over_freq_high_band_setpoint = 70.0;	//Very high - only 1 band for 1547
			over_freq_high_band_delay = 0.16;		//Same clearly as below
			over_freq_low_band_setpoint = 60.5;		//1547 high hvalue
			over_freq_low_band_delay = 0.16;		//1547 over-frequency value

			//Set the others based on size
			if (p_rated > 30000.0)
			{
				under_freq_high_band_setpoint = 59.5;	//Arbitrary selection in the range
				under_freq_high_band_delay = 300.0;		//Maximum delay, just because
				under_freq_low_band_setpoint = 57.0;	//Lower limit of 1547
				under_freq_low_band_delay = 0.16;		//Lower limit clearing of 1547
			}
			else	//Smaller one
			{
				under_freq_high_band_setpoint = 59.3;	//Low frequency value for small inverter 1547
				under_freq_high_band_delay = 0.16;		//Low frequency value clearing time for 1547
				under_freq_low_band_setpoint = 47.0;	//Arbitrary low point - 1547 didn't have this value for small inverters
				under_freq_low_band_delay = 0.16;		//Same value as low frequency, since this band doesn't technically exist
			}

			//Set the voltage values as well - basically, the under voltage has an extra category
			under_voltage_lowest_voltage_setpoint = 0.40;	//Lower than range before, so just duplicate disconnect value
			under_voltage_middle_voltage_setpoint = 0.50;	//Lower limit of 1547
			under_voltage_high_voltage_setpoint = 0.88;		//Low area threshold for 1547
			over_voltage_low_setpoint = 1.10;				//Lower limit of upper threshold for 1547
			over_voltage_high_setpoint = 1.20;				//Upper most limit of voltage values
			
			under_voltage_lowest_delay = 0.16;			//Lower than "normal low" - it is technically an overlap
			under_voltage_middle_delay = 0.16;			//Low limit of 1547
			under_voltage_high_delay = 2.0;				//High lower limit of 1547
			over_voltage_low_delay = 1.0;				//Low higher limit of 1547
			over_voltage_high_delay = 0.16;				//Highest value
		}
	}
	else
	{
		//Make to disable 1547, since it won't do anything
		enable_1547_compliance = false;

		gl_warning("Inverter:%d %s does not have a valid parent - 1547 checks have been disabled",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The IEEE 1547-2003 checks for interconnection require a valid powerflow parent.  One was not detected, so
		this functionality has been detected.
		*/
	}

	//By default, we succeed
	return SUCCESS;
}

//Functionalized routine to perform the IEEE 1547-2003 checks
double inverter::perform_1547_checks(double timestepvalue)
{
	bool voltage_violation, frequency_violation, trigger_disconnect, check_phase;
	bool uv_low_hit, uv_mid_hit, uv_high_hit, ov_low_hit, ov_high_hit;
	double temp_pu_voltage;
	double return_time_freq, return_time_volt, return_value;
	char indexval;

	//By default, we're subject to the whims of deltamode
	return_time_freq = -1.0;
	return_time_volt = -1.0;
	return_value = -1.0;

	//Pull the powerflow values, if needed
	if (parent_is_a_meter)
	{
		reset_complex_powerflow_accumulators();
		
		pull_complex_powerflow_values();
	}

	//Perform frequency check - overlapping bands set so we don't care about size anyore
	if ((value_Frequency > over_freq_low_band_setpoint) || (value_Frequency < under_freq_high_band_setpoint))
	{
		//Flag it
		frequency_violation = true;

		//Reset "restoration" time
		out_of_violation_time_total = 0.0;

		//Figure out which range we are
		if (value_Frequency > over_freq_high_band_setpoint)
		{
			//Accumulate the over frequency timers (all for this case)
			over_freq_high_band_viol_time += timestepvalue;
			over_freq_low_band_viol_time += timestepvalue;

			//Zero the others, in case we did a huge jump
			under_freq_high_band_viol_time = 0.0;
			under_freq_low_band_viol_time = 0.0;

			if (over_freq_high_band_viol_time > over_freq_high_band_delay)
			{
				trigger_disconnect = true;
				return_time_freq = reconnect_time;

				//Flag us as high over-frequency violation
				ieee_1547_trip_method = IEEE_1547_HIGH_OF;
			}
			else if (over_freq_low_band_viol_time > over_freq_low_band_delay)	//Triggered existing band
			{
				trigger_disconnect = true;
				return_time_freq = reconnect_time;

				//Flag us as the low over-frequency violation
				ieee_1547_trip_method = IEEE_1547_LOW_OF;
			}
			else
			{
				trigger_disconnect = false;
				
				//See which time to return
				if ((over_freq_high_band_delay - over_freq_high_band_viol_time) < (over_freq_low_band_delay - over_freq_low_band_viol_time))
				{
					return_time_freq = over_freq_high_band_delay - over_freq_high_band_viol_time;
				}
				else	//Other way around
				{
					return_time_freq = over_freq_low_band_delay - over_freq_low_band_viol_time;
				}
			}
		}
		else if (value_Frequency < under_freq_low_band_setpoint)
		{
			//Accumulate both under frequency timers (all violated)
			under_freq_high_band_viol_time += timestepvalue;
			under_freq_low_band_viol_time += timestepvalue;

			//Zero the others, in case we did a huge jump
			over_freq_high_band_viol_time = 0.0;
			over_freq_low_band_viol_time = 0.0;

			if (under_freq_low_band_viol_time > under_freq_low_band_delay)
			{
				trigger_disconnect = true;
				return_time_freq = reconnect_time;

				//Flag us as the low under-frequency violation
				ieee_1547_trip_method = IEEE_1547_LOW_UF;
			}
			else if (under_freq_high_band_viol_time > under_freq_high_band_delay)	//Other band trigger
			{
				trigger_disconnect = true;
				return_time_freq = reconnect_time;

				//Flag us as the high under-frequency violation
				ieee_1547_trip_method = IEEE_1547_HIGH_UF;
			}
			else
			{
				trigger_disconnect = false;

				//See which time to return
				if ((under_freq_high_band_delay - under_freq_high_band_viol_time) < (under_freq_low_band_delay - under_freq_low_band_viol_time))
				{
					return_time_freq = under_freq_high_band_delay - under_freq_high_band_viol_time;
				}
				else	//Other way around
				{
					return_time_freq = under_freq_low_band_delay - under_freq_low_band_viol_time;
				}
			}
		}
		else if ((value_Frequency < under_freq_high_band_setpoint) && (value_Frequency >= under_freq_low_band_setpoint))
		{
			//Just update the high violation time
			under_freq_high_band_viol_time += timestepvalue;

			//Zero the other one, for good measure
			under_freq_low_band_viol_time = 0.0;

			//Zero the others, in case we did a huge jump
			over_freq_high_band_viol_time = 0.0;
			over_freq_low_band_viol_time = 0.0;

			if (under_freq_high_band_viol_time > under_freq_high_band_delay)
			{
				trigger_disconnect = true;
				return_time_freq = reconnect_time;

				//Flag us as the high under frequency violation
				ieee_1547_trip_method = IEEE_1547_HIGH_UF;
			}
			else
			{
				trigger_disconnect = false;
				return_time_freq = under_freq_high_band_delay - under_freq_high_band_viol_time;
			}
		}
		else if ((value_Frequency <= over_freq_high_band_setpoint) && (value_Frequency > over_freq_low_band_setpoint))
		{
			//Just update the "high-low" violation time
			over_freq_low_band_viol_time += timestepvalue;

			//Zero the other one, for good measure
			over_freq_high_band_viol_time = 0.0;

			//Zero the others, in case we did a huge jump
			under_freq_high_band_viol_time = 0.0;
			under_freq_low_band_viol_time = 0.0;

			if (over_freq_low_band_viol_time > over_freq_low_band_delay)
			{
				trigger_disconnect = true;
				return_time_freq = reconnect_time;

				//Flag us as the low over-frequency violation
				ieee_1547_trip_method = IEEE_1547_LOW_OF;
			}
			else
			{
				trigger_disconnect = false;
				return_time_freq = over_freq_low_band_delay - over_freq_low_band_viol_time;
			}
		}
		else	//Not sure how we get here in this present logic arrangement - toss an error
		{
			gl_error("Inverter 1547 Checks - invalid  state!");
			/*  TROUBLESHOOT
			While performing the IEEE 1547-2003 frequency and voltage checks, an unknown state occurred.  Please
			try again.  If the error persists, please submit you GLM and a bug report via the ticketing system.
			*/
		}
	}
	else	//Must be in a good range
	{
		//Set flags to indicate as much
		frequency_violation = false;
		trigger_disconnect = false;

		//Reset frequency violation counters
		over_freq_high_band_viol_time = 0.0;
		over_freq_low_band_viol_time = 0.0;
		under_freq_high_band_viol_time = 0.0;
		under_freq_low_band_viol_time = 0.0;

		//Set the return time to negative, just to be paranoid
		return_time_freq = -1.0;
	}

	//Default to no voltage violation
	voltage_violation = false;

	//Set individual accumulator "touches" - will be used to reconcile over the phases
	uv_low_hit = false;
	uv_mid_hit = false;
	uv_high_hit = false;
	ov_low_hit = false;
	ov_high_hit = false;

	//See if we're already triggered or in a frequency violation (no point checking, if we are)
	//Loop through voltages present & check - if we find one, we'll break out
	for (indexval = 0; indexval < 3; indexval++)
	{
		//See if this phase exists
		if ((phases & PHASE_S) == PHASE_S)	//Triplex
		{
			//See if we're te proper index
			if (indexval == 0)	//Only need to check 240 V here
			{
				check_phase = true;
			}
			else
			{
				check_phase = false;
				break;	//No sense looping once more
			}
		}//End triplex
		else if ((indexval == 0) && ((phases & PHASE_A) == PHASE_A))
		{
			check_phase = true;
		}
		else if ((indexval == 1) && ((phases & PHASE_B) == PHASE_B))
		{
			check_phase = true;
		}
		else if ((indexval == 2) && ((phases & PHASE_C) == PHASE_C))
		{
			check_phase = true;
		}
		else	//Not a proper combination
		{
			check_phase = false;
		}

		//See if we were valid
		if (check_phase == true)
		{
			//See if it is a violation
			temp_pu_voltage = value_Circuit_V[indexval].Mag()/node_nominal_voltage;

			//Check it
			if ((temp_pu_voltage < under_voltage_high_voltage_setpoint) || (temp_pu_voltage > over_voltage_low_setpoint))
			{
				//flag a violation
				voltage_violation = true;

				//Clear the "no violation timer"
				out_of_violation_time_total = 0.0;

				//See which case we are
				if (temp_pu_voltage < under_voltage_lowest_voltage_setpoint)
				{
					//See if we've accumulated yet
					if (!uv_low_hit)
					{
						under_voltage_lowest_viol_time += timestepvalue;
						uv_low_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else if ((temp_pu_voltage >= under_voltage_lowest_voltage_setpoint) && (temp_pu_voltage < under_voltage_middle_voltage_setpoint))
				{

					//See if we've accumulated yet
					if (!uv_mid_hit)
					{
						under_voltage_middle_viol_time += timestepvalue;
						uv_mid_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else if ((temp_pu_voltage >= under_voltage_middle_voltage_setpoint) && (temp_pu_voltage < under_voltage_high_voltage_setpoint))
				{
					//See if we've accumulated yet
					if (!uv_high_hit)
					{
						under_voltage_high_viol_time += timestepvalue;
						uv_high_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else if ((temp_pu_voltage > over_voltage_low_setpoint) && (temp_pu_voltage < over_voltage_high_setpoint))
				{
					//See if we've accumulated yet
					if (!ov_low_hit)
					{
						over_voltage_low_viol_time += timestepvalue;
						ov_low_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else if (temp_pu_voltage >= over_voltage_high_setpoint)
				{
					//See if we've accumulated yet
					if (!ov_high_hit)
					{
						over_voltage_high_viol_time += timestepvalue;
						ov_high_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else	//must not have tripped a time limit
				{
					gl_error("Inverter 1547 Checks - invalid state!");
					//Defined above
				}
			}//End of a violation occurred
			//Default else, normal operating range - loop
		}//End was a valid phase

		//Default else - go to next phase

	}//End phase loop
	
	//See if anything was hit - if so, reconcile it
	if (voltage_violation)
	{
		//Reconcile the violation times and see how we need to break
		if (uv_low_hit)
		{
			if (under_voltage_lowest_viol_time > under_voltage_lowest_delay)
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;

				//Flag us as the lowest under voltage violation
				ieee_1547_trip_method = IEEE_1547_LOWEST_UV;
			}
			else if (under_voltage_middle_viol_time > under_voltage_middle_delay)	//Check other ranges
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;

				//Flag us as the middle under voltage violation
				ieee_1547_trip_method = IEEE_1547_MIDDLE_UV;
			}

			else if (under_voltage_high_viol_time > under_voltage_high_delay)
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;

				//Flag us as the high under voltage violation
				ieee_1547_trip_method = IEEE_1547_HIGH_UV;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = under_voltage_lowest_delay - under_voltage_lowest_viol_time;
			}
		}
		else if (uv_mid_hit)
		{
			if (under_voltage_middle_viol_time > under_voltage_middle_delay)
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;

				//Flag us as the middle under voltage violation
				ieee_1547_trip_method = IEEE_1547_MIDDLE_UV;
			}
			else if (under_voltage_high_viol_time > under_voltage_high_delay)	//Check higher bands
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;

				//Flag us as the high under voltage violation
				ieee_1547_trip_method = IEEE_1547_HIGH_UV;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = under_voltage_middle_delay - under_voltage_middle_viol_time;
			}
		}
		else if (uv_high_hit)
		{
			if (under_voltage_high_viol_time > under_voltage_high_delay)
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;

				//Flag us as the high under voltage violation
				ieee_1547_trip_method = IEEE_1547_HIGH_UV;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = under_voltage_high_delay - under_voltage_high_viol_time;
			}
		}
		else if (ov_low_hit)
		{
			if (over_voltage_low_viol_time > over_voltage_low_delay)
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;

				//Flag us as the low over voltage violation
				ieee_1547_trip_method = IEEE_1547_LOW_OV;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = over_voltage_low_delay - over_voltage_low_viol_time;
			}
		}
		else if (ov_high_hit)
		{
			if (over_voltage_high_viol_time > over_voltage_high_delay)
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;

				//Flag us as the high over voltage violation
				ieee_1547_trip_method = IEEE_1547_HIGH_OV;
			}
			else if (over_voltage_low_viol_time > over_voltage_low_delay)	//Lower band overlap
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;

				//Flag us as the low over voltage violation
				ieee_1547_trip_method = IEEE_1547_LOW_OV;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = over_voltage_high_delay - over_voltage_high_viol_time;
			}
		}
		else	//must not have tripped a time limit
		{
			gl_error("Inverter 1547 Checks - invalid state!");
			//Defined above
		}
	}//End of a violation occurred
	else	//No voltage violation
	{
		//Zero all accumulators
		under_voltage_lowest_viol_time = 0.0;
		under_voltage_middle_viol_time = 0.0;
		under_voltage_high_viol_time = 0.0;
		over_voltage_low_viol_time = 0.0;
		over_voltage_high_viol_time = 0.0;

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
	if (!inverter_1547_status)
	{
		if (out_of_violation_time_total > reconnect_time)
		{
			//Set us back into service
			inverter_1547_status = true;

			//Flag us as no reason
			ieee_1547_trip_method = IEEE_1547_NONE;

			//Implies no violations, so force return a -1.0
			return -1.0;
		}
		else	//Still delayed, just reaffirm our status
		{
			inverter_1547_status = false;

			//calculate the new update time
			return_value = reconnect_time - out_of_violation_time_total;

			//Return the minimum from above
			return return_value;
		}
	}
	else	//We're true, see if we need to not be
	{
		if (trigger_disconnect)
		{
			inverter_1547_status = false;	//Trigger

			//Return our expected next status interval
			return return_value;
		}
		else
		{
			//Flag us as no reason
			ieee_1547_trip_method = IEEE_1547_NONE;

			//All is well, indicate as much
			return return_value;
		}
	}
}

//Function to perform exp(j*val)
//Basically a complex rotation
gld::complex inverter::complex_exp(double angle)
{
	gld::complex output_val;

	//exp(jx) = cos(x)+j*sin(x)
	output_val = gld::complex(cos(angle),sin(angle));

	return output_val;
}

//Map Complex value
gld_property *inverter::map_complex_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_complex())
	{
		GL_THROW("inverter:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Map double value
gld_property *inverter::map_double_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_double())
	{
		GL_THROW("inverter:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Function to pull all the complex properties from powerflow into local variables
void inverter::pull_complex_powerflow_values(void)
{
	//Pull in the various values from powerflow - straight reads
	//Regardless of if triplex or not, pull all voltages and the status
	value_Circuit_V[0] = pCircuit_V[0]->get_complex();
	value_Circuit_V[1] = pCircuit_V[1]->get_complex();
	value_Circuit_V[2] = pCircuit_V[2]->get_complex();
	value_MeterStatus = pMeterStatus->get_enumeration();
	
	//Pull the frequency too
	value_Frequency = pFrequency->get_double();

	// //If we're a VSI, pull the IGenerated value too -- should be the same, but in case inverters ever become like diesels
	if (VSI_mode == VSI_ISOCHRONOUS || VSI_mode == VSI_DROOP)
	{
		//Update IGenerated, in case the powerflow is overriding it
		if (parent_is_triplex)
		{
			value_IGenerated[0] = pIGenerated[0]->get_complex();
		}
		else
		{
			value_IGenerated[0] = pIGenerated[0]->get_complex();
			value_IGenerated[1] = pIGenerated[1]->get_complex();
			value_IGenerated[2] = pIGenerated[2]->get_complex();
		}
	}
}

//Function to reset the various accumulators, so they don't double-accumulate if they weren't used
void inverter::reset_complex_powerflow_accumulators(void)
{
	int indexval;

	//See which one we are, since that will impact things
	if (!parent_is_triplex)	//Three-phase
	{
		//Loop through the three-phases/accumulators
		for (indexval=0; indexval<3; indexval++)
		{
			//**** Current value ***/
			value_Line_I[indexval] = gld::complex(0.0,0.0);

			//**** Power value ***/
			value_Power[indexval] = gld::complex(0.0,0.0);

			//**** pre-rotated Current value ***/
			value_Line_unrotI[indexval] = gld::complex(0.0,0.0);
		}
	}
	else	//Assumes must be triplex - else how did it get here?
	{
		//Reset the relevant values -- all single pulls
		
		//**** Current12 value ***/
		value_Line12 = gld::complex(0.0,0.0);

		//**** powert12 value ***/
		value_Power12 = gld::complex(0.0,0.0);

		//**** prerotated_12 value ***/
		value_Line_unrotI[0] = gld::complex(0.0,0.0);
	}
}

//Function to push up all changes of complex properties to powerflow from local variables
void inverter::push_complex_powerflow_values(void)
{
	gld::complex temp_complex_val;
	gld_wlock *test_rlock = nullptr;
	int indexval;

	//See which one we are, since that will impact things
	if (!parent_is_triplex)	//Three-phase
	{
		//Loop through the three-phases/accumulators
		for (indexval=0; indexval<3; indexval++)
		{
			//**** Current value ***/
			//Pull current value again, just in case
			temp_complex_val = pLine_I[indexval]->get_complex();

			//Add the difference
			temp_complex_val += value_Line_I[indexval];

			//Push it back up
			pLine_I[indexval]->setp<gld::complex>(temp_complex_val,*test_rlock);

			//**** Power value ***/
			//Pull current value again, just in case
			temp_complex_val = pPower[indexval]->get_complex();

			//Add the difference
			temp_complex_val += value_Power[indexval];

			//Push it back up
			pPower[indexval]->setp<gld::complex>(temp_complex_val,*test_rlock);

			//**** pre-rotated Current value ***/
			//Pull current value again, just in case
			temp_complex_val = pLine_unrotI[indexval]->get_complex();

			//Add the difference
			temp_complex_val += value_Line_unrotI[indexval];

			//Push it back up
			pLine_unrotI[indexval]->setp<gld::complex>(temp_complex_val,*test_rlock);

			if ((VSI_mode == VSI_ISOCHRONOUS) || (VSI_mode == VSI_DROOP))
			{
				//**** IGenerated Current value ***/
				//Direct write, not an accumulator
				pIGenerated[indexval]->setp<gld::complex>(value_IGenerated[indexval],*test_rlock);
			}
		}
	}
	else	//Assumes must be triplex - else how did it get here?
	{
		//Pull the relevant values -- all single pulls
		
		//**** Current12 value ***/
		//Pull current value again, just in case
		temp_complex_val = pLine12->get_complex();

		//Add the difference
		temp_complex_val += value_Line12;

		//Push it back up
		pLine12->setp<gld::complex>(temp_complex_val,*test_rlock);

		//**** powert12 value ***/
		//Pull current value again, just in case
		temp_complex_val = pPower12->get_complex();

		//Add the difference
		temp_complex_val += value_Power12;

		//Push it back up
		pPower12->setp<gld::complex>(temp_complex_val,*test_rlock);

		//**** prerotated_12 value ***/
		//Pull current value again, just in case
		temp_complex_val = pLine_unrotI[0]->get_complex();

		//Add the difference
		temp_complex_val += value_Line_unrotI[0];

		//Push it back up
		pLine_unrotI[0]->setp<gld::complex>(temp_complex_val,*test_rlock);

		//**** IGenerated_12 ****/
		if ((VSI_mode == VSI_ISOCHRONOUS) || (VSI_mode == VSI_DROOP))
		{
			//Direct write, not an accumulator
			pIGenerated[0]->setp<gld::complex>(value_IGenerated[0],*test_rlock);
		}
	}
}

//Functionalized way to do an ax+b, apparently
double inverter::lin_eq_volt(double volt, double m, double b)
{
	double Q;

	Q = (m * volt) + b;
	return Q;
}

// Function to update current injection IGenerated for VSI
STATUS inverter::updateCurrInjection(int64 iteration_count, bool *converged_failure)
{
	double power_diff_val;
	bool ramp_change;
	double deltat, temp_time;
	char idx;
	OBJECT *obj = OBJECTHDR(this);
	gld::complex temp_VA;
	bool bus_is_a_swing, bus_is_swing_pq_entry;
	STATUS temp_status_val;
	gld_property *temp_property_pointer = nullptr;
	double mag_diff_value[3];

	//Start with assuming convergence (not a failure)
	*converged_failure = false;

	if (deltatimestep_running > 0.0)	//Deltamode call
	{
		//Get the time
		temp_time = gl_globaldeltaclock;
	}
	else
	{
		//Grab the current time
		temp_time = (double)gl_globalclock;
	}

	//Pull the current powerflow values
	if (parent_is_a_meter)
	{
		//Reset the accumulators, just in case
		reset_complex_powerflow_accumulators();

		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	//See if the time has changed
	if (prev_time_dbl != temp_time)
	{
		//Update the difference - we'll use this later (in event driven mode)
		event_deltat = temp_time - prev_time_dbl;

		//Copy the values
		//Update power tracking variables, if ramp-rate checking is enabled
		if ((four_quadrant_control_mode == FQM_VSI) && (checkRampRate_real || checkRampRate_reactive))
		{
			//See which one we are
			if ((phases & 0x10) == 0x10)
			{
				prev_VA_out[0] = curr_VA_out[0];
			}
			else	//Some variant of three-phase, just grab them all
			{
				//Copy in all the values - phasing doesn't matter for these
				prev_VA_out[0] = curr_VA_out[0];
				prev_VA_out[1] = curr_VA_out[1];
				prev_VA_out[2] = curr_VA_out[2];
			}
		}

		//Store the new clock
		prev_time_dbl = temp_time;
	}

	//Do the timestep assignment
	if (deltatimestep_running > 0.0)	//Deltamode
	{
		//Deltat is just the value
		deltat = deltatimestep_running;
	}
	else
	{
		//Assign the deltat value	
		deltat = event_deltat;
	}

	//See if we're a meter
	if (parent_is_a_meter)
	{
		//By default, assume we're not a SWING
		bus_is_a_swing = false;

		//Determine our status
		if (attached_bus_type > 1)	//SWING or SWING_PQ
		{
			//See if it has been mapped already
			if (swing_test_fxn==nullptr)
			{
				//Map the swing status check function
				swing_test_fxn = (FUNCTIONADDR)(gl_get_function(obj->parent,"pwr_object_swing_status_check"));

				//See if it was located
				if (swing_test_fxn == nullptr)
				{
					GL_THROW("inverter:%s - failed to map swing-checking for node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
					/*  TROUBLESHOOT
					While attempting to map the swing-checking function, an error was encountered.
					Please try again.  If the error persists, please submit your code and a bug report via the trac website.
					*/
				}
			}

			//Call the mapping function
			temp_status_val = ((STATUS (*)(OBJECT *,bool * , bool*))(*swing_test_fxn))(obj->parent,&bus_is_a_swing,&bus_is_swing_pq_entry);

			//Make sure it worked
			if (temp_status_val != SUCCESS)
			{
				GL_THROW("inverter:%s - failed to map swing-checking for node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
				//Defined above
			}

			//Now see how we've gotten here
			if (first_iteration_current_injection == -1)	//Haven't entered before
			{
				//See if we are truely the first iteration
				if (iteration_count != 0)
				{
					//We're not, which means we were a SWING or a SWING_PQ "demoted" after balancing was completed - override the indication
					//If it was already true, well, we just set it again
					bus_is_a_swing = true;
				}
				//Default else - we are zero, so we can just leave the "SWING status" correct

				//Update the iteration counter
				first_iteration_current_injection = iteration_count;
			}
			else if ((first_iteration_current_injection != 0) || bus_is_swing_pq_entry)	//We didn't enter on the first iteration
			{
				//Just override the indication - this only happens if we were a SWING or a SWING_PQ that was "demoted"
				bus_is_a_swing = true;
			}
			//Default else is zero - we entered on the first iteration, which implies we are either a PQ bus,
			//or were a SWING_PQ that was behaving as a PQ from the start
		}
		//Default else - keep bus_is_a_swing as false
	}
	else
	{
		//Just assume the object is not a swing
		bus_is_a_swing = false;
	}

	// Adjust VSI (not on SWING bus) current injection and e_source values only at the first iteration of each time step
	if (inverter_first_step)
	{
		//Set target
		temp_VA = gld::complex(P_Out,Q_Out);

		if (four_quadrant_control_mode == FQM_VSI)
		{
			if (!bus_is_a_swing)
			{
				//Balance voltage and satisfy power
				gld::complex aval, avalsq;
				gld::complex temp_total_power_val[3];
				gld::complex temp_total_power_internal;
				gld::complex temp_pos_voltage, temp_pos_current;

				//Conversion variables - 1@120-deg
				aval = gld::complex(-0.5,(sqrt(3.0)/2.0));
				avalsq = aval*aval;	//squared value is used a couple places too


				//Calculate the Norton-shunted power
				temp_total_power_val[0] = value_Circuit_V[0] * ~(generator_admittance[0][0]*value_Circuit_V[0] + generator_admittance[0][1]*value_Circuit_V[1] + generator_admittance[0][2]*value_Circuit_V[2]);
				temp_total_power_val[1] = value_Circuit_V[1] * ~(generator_admittance[1][0]*value_Circuit_V[0] + generator_admittance[1][1]*value_Circuit_V[1] + generator_admittance[1][2]*value_Circuit_V[2]);
				temp_total_power_val[2] = value_Circuit_V[2] * ~(generator_admittance[2][0]*value_Circuit_V[0] + generator_admittance[2][1]*value_Circuit_V[1] + generator_admittance[2][2]*value_Circuit_V[2]);

				//Figure out what we should be generating internally
				temp_total_power_internal = temp_VA + temp_total_power_val[0] + temp_total_power_val[1] + temp_total_power_val[2];

				//Compute the positive sequence voltage (*3)
				temp_pos_voltage = value_Circuit_V[0] + value_Circuit_V[1]*aval + value_Circuit_V[2]*avalsq;

				//Compute the positive sequence current
				if (temp_pos_voltage.Mag() > 0.0)	//Actually have voltage
				{
					temp_pos_current = ~(temp_total_power_internal/temp_pos_voltage);
				}
				else	//Zero it, to avoid a NAN
				{
					temp_pos_current = gld::complex(0.0,0.0);
				}

				//Now populate this into the output
				value_IGenerated[0] = temp_pos_current;
				value_IGenerated[1] = temp_pos_current*avalsq;
				value_IGenerated[2] = temp_pos_current*aval;

				//Get the magnitude of the difference
				mag_diff_value[0] = (value_IGenerated[0] - prev_value_IGenerated[0]).Mag();
				mag_diff_value[1] = (value_IGenerated[1] - prev_value_IGenerated[1]).Mag();
				mag_diff_value[2] = (value_IGenerated[2] - prev_value_IGenerated[2]).Mag();

				//Update trackers
				prev_value_IGenerated[0] = value_IGenerated[0];
				prev_value_IGenerated[1] = value_IGenerated[1];
				prev_value_IGenerated[2] = value_IGenerated[2];

				//Convergence check
				if ((mag_diff_value[0] > current_convergence_criterion) || (mag_diff_value[1] > current_convergence_criterion) || (mag_diff_value[2] > current_convergence_criterion))
				{
					//Flag for failure
					*converged_failure = true;
				}
			}
			//If it is a swing, this is all handled internally to solver_nr (though it is similar)
		}
		//Default else -- grid following?
	}
	//Default else - handled elsewhere in the code

	//Copy-pasted from above
	// VSI isochronous mode keeps the voltage angle constant always
	//TODO: Probably needs to be extended to other modes
	if ((four_quadrant_control_mode == FQM_VSI) && (checkRampRate_real || checkRampRate_reactive))
	{
		//See what our phasing condition is at
		if ((phases & 0x10) == 0x10)	//Triplex
		{
			//Triplex isn't supported in VSI -- messes up the admittance formulation too much, so not allowed - error us
			GL_THROW("inverter:%d - %s - VSI mode was attempted on a triplex-connected inverter! This is not permitted!",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			A voltage-source-inverter was connected to a triplex node.  This is currently unsupported.  Try connecting the inverter
			to a three-phase portion of the system.
			*/
			
			return FAILED;
		}
		else	//Some variant of three-phase -- note, this assumes all three right now
		{
			//Effectively copy-pasted from above
			for(idx = 0; idx < 3; idx++)
			{
				//See how this aligns with the ramp rate, if necessary
				if (checkRampRate_real || checkRampRate_reactive)
				{
					//Deflag
					ramp_change = false;

					//See what the power out is for this "new" state
					temp_current_val[idx] = (value_IGenerated[idx] - generator_admittance[idx][0]*value_Circuit_V[0] - generator_admittance[idx][1]*value_Circuit_V[1] - generator_admittance[idx][2]*value_Circuit_V[2]);

					//Update power output variables, just so we can see what is going on
					power_val[idx] = value_Circuit_V[idx]*~temp_current_val[idx];

					//See which way we are
					if (checkRampRate_real) {

						//Compute the difference - real part
						power_diff_val = (power_val[idx].Re() - prev_VA_out[idx].Re()) / deltat;

						if (power_val[idx].Re() > prev_VA_out[idx].Re())	//Ramp up
						{

							//See if it was too big
							if (power_diff_val > rampUpRate_real)
							{
								//Flag
								ramp_change = true;

								power_val[idx].SetReal(prev_VA_out[idx].Re() + (rampUpRate_real * deltat));
							}
							//Default else - was okay
						}
						else	//Ramp down
						{
							//See if it was too big
							if (power_diff_val < -rampDownRate_real)
							{
								//Flag
								ramp_change = true;

								power_val[idx].SetReal(prev_VA_out[idx].Re() - (rampDownRate_real * deltat));
							}
							//Default else - was okay
						}
					}

					//Now "extrapolate" this back to a current value, if needed
					if (ramp_change)
					{
						//Compute a "new current" value
						if (value_Circuit_V[idx].Mag() > 0.0)
						{
							temp_current_val[idx] = ~(power_val[idx] / value_Circuit_V[idx]);
						}
						else
						{
							temp_current_val[idx] = gld::complex(0.0,0.0);
						}

						//Adjust it to IGenerated
						value_IGenerated[idx] = temp_current_val[idx] + generator_admittance[idx][0]*value_Circuit_V[0] + generator_admittance[idx][1]*value_Circuit_V[1] + generator_admittance[idx][2]*value_Circuit_V[2];

						//And adjust the related "internal voltage" - this just broke the frequency too
						e_source[idx] = value_IGenerated[idx] * (gld::complex(Rfilter,Xfilter) * Zbase);

						//Other state variables needed to be updated?
					}
					//Default else - no ramp change, so don't mess with anything

					//Store the updated power value
					curr_VA_out[idx] = power_val[idx];
				}//Ramp rate check active
			}//End phase for loop
		}//End three-phase
	}

	//Push the changes up
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values();
	}

	//Always a success, but power flow solver may not like it if VA_OUT exceeded the rating and thus changed
	return SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_inverter(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(inverter::oclass);
		if (*obj!=nullptr)
		{
			inverter *my = OBJECTDATA(*obj,inverter);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(inverter);
}

EXPORT int init_inverter(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=nullptr)
			return OBJECTDATA(obj,inverter)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(inverter);
}

EXPORT TIMESTAMP sync_inverter(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	inverter *my = OBJECTDATA(obj,inverter);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;		
	}
	SYNC_CATCHALL(inverter);
	return t2;
}

EXPORT int isa_inverter(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,inverter)->isa(classname);
}

EXPORT STATUS preupdate_inverter(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time)
{
	inverter *my = OBJECTDATA(obj,inverter);
	STATUS status_output = FAILED;

	try
	{
		status_output = my->pre_deltaupdate(t0,delta_time);
		return status_output;
	}
	catch (char *msg)
	{
		gl_error("preupdate_inverter(obj=%d;%s): %s",obj->id, (obj->name ? obj->name : "unnamed"), msg);
		return status_output;
	}
}

EXPORT SIMULATIONMODE interupdate_inverter(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	inverter *my = OBJECTDATA(obj,inverter);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time,dt,iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_inverter(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

EXPORT STATUS postupdate_inverter(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass)
{
	inverter *my = OBJECTDATA(obj,inverter);
	STATUS status = FAILED;
	try
	{
		status = my->post_deltaupdate(useful_value, mode_pass);
		return status;
	}
	catch (char *msg)
	{
		gl_error("postupdate_inverter(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

// Define export function that update the VIS current injection IGenerated to the grid
EXPORT STATUS inverter_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure)
{
	STATUS temp_status;

	//Map the node
	inverter *my = OBJECTDATA(obj,inverter);

	//Call the function, where we can update the IGenerated injection
	temp_status = my->updateCurrInjection(iteration_count,converged_failure);

	//Return what the sub function said we were
	return temp_status;

}





