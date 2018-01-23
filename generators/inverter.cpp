/** $Id: inverter.cpp,v 1.0 2008/07/15 
	Copyright (C) 2008 Battelle Memorial Institute
	@file inverter.cpp
	@defgroup inverter
	@ingroup generators

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "generators.h"
#include "power_electronics.h"
#include "inverter.h"

#define DEFAULT 1.0;

//CLASS *inverter::plcass = power_electronics;
CLASS *inverter::oclass = NULL;
inverter *inverter::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
inverter::inverter(MODULE *module)
{	
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"inverter",sizeof(inverter),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
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
				PT_KEYWORD,"VOLT_VAR_FREQ_PWR",FQM_VOLT_VAR_FREQ_PWR, //Ab add : mode
				PT_KEYWORD,"LOAD_FOLLOWING",(enumeration)FQM_LOAD_FOLLOWING,
				PT_KEYWORD,"GROUP_LOAD_FOLLOWING",(enumeration)FQM_GROUP_LF,

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
			PT_complex, "V_In[V]",PADDR(V_In), PT_DESCRIPTION, "DC voltage",
			PT_complex, "I_In[A]",PADDR(I_In), PT_DESCRIPTION, "DC current",
			PT_complex, "VA_In[VA]", PADDR(VA_In), PT_DESCRIPTION, "DC power",
			PT_complex, "VA_Out[VA]", PADDR(VA_Out), PT_DESCRIPTION, "AC power",
			PT_double, "Vdc[V]", PADDR(Vdc), PT_DESCRIPTION, "LEGACY MODEL: DC voltage",
			PT_complex, "phaseA_V_Out[V]", PADDR(phaseA_V_Out), PT_DESCRIPTION, "AC voltage on A phase in three-phase system; 240-V connection on a triplex system",
			PT_complex, "phaseB_V_Out[V]", PADDR(phaseB_V_Out), PT_DESCRIPTION, "AC voltage on B phase in three-phase system",
			PT_complex, "phaseC_V_Out[V]", PADDR(phaseC_V_Out), PT_DESCRIPTION, "AC voltage on C phase in three-phase system",
			PT_complex, "phaseA_I_Out[V]", PADDR(phaseA_I_Out), PT_DESCRIPTION, "AC current on A phase in three-phase system; 240-V connection on a triplex system",
			PT_complex, "phaseB_I_Out[V]", PADDR(phaseB_I_Out), PT_DESCRIPTION, "AC current on B phase in three-phase system",
			PT_complex, "phaseC_I_Out[V]", PADDR(phaseC_I_Out), PT_DESCRIPTION, "AC current on C phase in three-phase system",
			PT_complex, "power_A[VA]", PADDR(power_A), PT_DESCRIPTION, "AC power on A phase in three-phase system; 240-V connection on a triplex system",
			PT_complex, "power_B[VA]", PADDR(power_B), PT_DESCRIPTION, "AC power on B phase in three-phase system",
			PT_complex, "power_C[VA]", PADDR(power_C), PT_DESCRIPTION, "AC power on C phase in three-phase system",
			
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

			//Frequency bands of 1547a checks
			PT_double, "over_freq_high_cutout[Hz]", PADDR(over_freq_high_band_setpoint),PT_DESCRIPTION,"DELTAMODE: OF2 set point for IEEE 1547a",
			PT_double, "over_freq_high_disconenct_time[s]", PADDR(over_freq_high_band_delay),PT_DESCRIPTION,"DELTAMODE: OF2 clearing time for IEEE1547a",
			PT_double, "over_freq_low_cutout[Hz]", PADDR(over_freq_low_band_setpoint),PT_DESCRIPTION,"DELTAMODE: OF1 set point for IEEE 1547a",
			PT_double, "over_freq_low_disconenct_time[s]", PADDR(over_freq_low_band_delay),PT_DESCRIPTION,"DELTAMODE: OF1 clearing time for IEEE 1547a",
			PT_double, "under_freq_high_cutout[Hz]", PADDR(under_freq_high_band_setpoint),PT_DESCRIPTION,"DELTAMODE: UF2 set point for IEEE 1547a",
			PT_double, "under_freq_high_disconenct_time[s]", PADDR(under_freq_high_band_delay),PT_DESCRIPTION,"DELTAMODE: UF2 clearing time for IEEE1547a",
			PT_double, "under_freq_low_cutout[Hz]", PADDR(under_freq_low_band_setpoint),PT_DESCRIPTION,"DELTAMODE: UF1 set point for IEEE 1547a",
			PT_double, "under_freq_low_disconenct_time[s]", PADDR(under_freq_low_band_delay),PT_DESCRIPTION,"DELTAMODE: UF1 clearing time for IEEE 1547a",

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
			PT_double, "c_0", PADDR(c_o), PT_DESCRIPTION, "MULTIPOINT EFFICIENCY MODEL: the first coefficient in the efficienty curve",
			PT_double, "c_1", PADDR(c_1), PT_DESCRIPTION, "MULTIPOINT EFFICIENCY MODEL: the second coefficient in the efficienty curve",
			PT_double, "c_2", PADDR(c_2), PT_DESCRIPTION, "MULTIPOINT EFFICIENCY MODEL: the third coefficient in the efficienty curve",
			PT_double, "c_3", PADDR(c_3), PT_DESCRIPTION, "MULTIPOINT EFFICIENCY MODEL: the fourth coefficient in the efficienty curve",
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
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

			defaults = this;

			memset(this,0,sizeof(inverter));

			if (gl_publish_function(oclass,	"preupdate_gen_object", (FUNCTIONADDR)preupdate_inverter)==NULL)
				GL_THROW("Unable to publish inverter deltamode function");
			if (gl_publish_function(oclass,	"interupdate_gen_object", (FUNCTIONADDR)interupdate_inverter)==NULL)
				GL_THROW("Unable to publish inverter deltamode function");
			if (gl_publish_function(oclass,	"postupdate_gen_object", (FUNCTIONADDR)postupdate_inverter)==NULL)
				GL_THROW("Unable to publish inverter deltamode function");
	}
}
/* Object creation is called once for each object that is created by the core */
int inverter::create(void) 
{
	// Default values for Inverter object.
	P_Out = 0;  // P_Out and Q_Out are set by the user as set values to output in CONSTANT_PQ mode
	Q_Out = 0;
	V_In_Set_A = complex(480,0);
	V_In_Set_B = complex(-240, 415.69);
	V_In_Set_C = complex(-240,-415.69);
	V_Set_A = 240;
	V_Set_B = 240;
	V_Set_C = 240;
	margin = 10;
	I_out_prev = 0;
	I_step_max = 100;
	internal_losses = 0;
	C_Storage_In = 0;
	power_factor = 1;
	
	//Ab add : default values for four quadrant Volt-VAr mode
	disable_volt_var_if_no_input_power = false;	//Volt-VAr mode always on by default
	delay_time = -1;					        //delay time between seeing a voltage value and responding with appropiate VAr setting, set -1 to flag as unspecified, will reset to 0 by default
	max_var_slew_rate = 0;				        //maximum rate at which inverter can change its VAr output (VArs/second) set 0 to flag unspecified, will reset to huge value by default
	volt_var_sched[0] = '\0';			        //initialize Volt-VAr input as zero length string
	freq_pwr_sched[0] = '\0';                   //initialize freq-PdeliveredLimit input as zero length string
	//end Ab add
	
	// Default values for power electronics settings
	Rated_kW = 10;		//< nominal power in kW
	Max_P = 10;			//< maximum real power capacity in kW
	Min_P = 0;			//< minimum real power capacity in kW
	Max_Q = 10;			//< maximum reactive power capacity in kVar
	Min_Q = 10;			//< minimum reactive power capacity in kVar
	Rated_kVA = 15;		//< nominal capacity in kVA
	Rated_kV = 10;		//< nominal line-line voltage in kV
	//Rinternal = 0.035;
	Rload = 1;
	Rtotal = 0.05;
	Rground = 0.03;
	Rground_storage = 0.05;
	Vdc = 480;

	Cinternal = 0;
	Cground = 0;
	Ctotal = 0;
	Linternal = 0;
	Lground = 0;
	Ltotal = 0;
	filter_120HZ = true;
	filter_180HZ = true;
	filter_240HZ = true;
	pf_in = 0;
	pf_out = 1;
	number_of_phases_in = 0;
	phaseAIn = false;
	phaseBIn = false;
	phaseCIn = false;
	phaseAOut = true;
	phaseBOut = true;
	phaseCOut = true;

	last_current[0] = last_current[1] = last_current[2] = last_current[3] = 0.0;
	last_power[0] = last_power[1] = last_power[2] = last_power[3] = 0.0;

	switch_type_choice = IDEAL_SWITCH;
	filter_type_v = (enumeration)BAND_PASS;
	filter_imp_v = (enumeration)IDEAL_FILTER;
	power_in = DC;
	power_out = AC;

	islanded = FALSE;
	use_multipoint_efficiency = FALSE;
	p_dco = -1;
	p_so = -1;
	v_dco = -1;
	c_o = -1;
	c_1 = -1;
	c_2 = -1;
	c_3 = -1;
	p_max = -1;
	pMeterStatus = NULL;
	efficiency = 0;
	inv_eta = 0.0;	//Not sure why there's two of these...

	sense_object = NULL;
	max_charge_rate = 0;
	max_discharge_rate = 0;
	charge_on_threshold = 0;
	charge_off_threshold = 0;
	discharge_on_threshold = 0;
	discharge_off_threshold = 0;
	powerCalc = NULL;
	sense_is_link = false;
	sense_power = NULL;

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

	inverter_convergence_criterion = 1e-3;

	//Default PID controller variables -- no output by default
	kpd = 0.0;
	kpq = 0.0;
	kid = 0.0;
	kiq = 0.0;
	kdd = 0.0;
	kdq = 0.0;
	first_sync_delta_enabled = false;
	pLine_unrotI = NULL;
	first_iter_counter = 0;
	
	//By default, assume we want the PID-based controller
	inverter_dyn_mode = PID_CONTROLLER;

	//1547 parameters
	enable_1547_compliance = false;		//1547 turned off, but default
	reconnect_time = 300.0;				//5 minute default, as suggested by 1547-2003
	inverter_1547_status = true;		//Not in a violation, by default
	out_of_violation_time_total = 0.0;	//Not in a violation, so not tracking 'recovery'
	freq_pointer = NULL;				//No mapping, by default
	prev_time = 0;						//Tracking variable
	prev_time_dbl = 0.0;				//Tracking variable

	//By default, assumed we want to use IEEE 1547a
	ieee_1547_version = IEEE1547A;

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
	under_freq_low_band_setpoint = 57;		//UF1 set point for IEEE 1547a
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

	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int inverter::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	STATUS return_value_init;

	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("inverter::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	// construct circuit variable map to meter
	static complex default_line123_voltage[3], default_line1_current[3];
	static int default_meter_status;	//Not really a good place to do this, but keep consistent
	int i;
	std::string tempV, tempQ, tempf, tempP;
	std::string VoltVArSchedInput, freq_pwrSchedInput;

	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true;	//Set the flag and off we go
	}

	// find parent meter or triplex_meter, if not defined, use default voltages, and if
	// the parent is not a meter throw an exception
	if (parent!=NULL && gl_object_isa(parent,"meter"))
	{
		// attach meter variables to each circuit
		parent_string = "meter";
		struct {
			complex **var;
			char *varname;
		}
		map[] = {
		// local object name,	meter object name
			{&pCircuit_V,			"voltage_A"}, // assumes 2 and 3 follow immediately in memory
			{&pLine_I,				"current_A"}, // assumes 2 and 3(N) follow immediately in memory
			{&pPower,				"power_A"}, // assumes 2 and 3 follow immediately in memory
			{&pLine_unrotI,			"prerotated_current_A"},
		};
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
	
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
			*(map[i].var) = get_complex(parent,map[i].varname);

		//Map status
		pMeterStatus = get_enum(parent,"service_status");

		//Check it
		if (pMeterStatus==NULL)
		{
			GL_THROW("Inverter failed to map powerflow status variable");
			/*  TROUBLESHOOT
			While attempting to map the service_status variable of the parent
			powerflow object, an error occurred.  Please try again.  If the error
			persists, please submit your code and a bug report via the trac website.
			*/
		}

		//Map phases
		set *phaseInfo;
		PROPERTY *tempProp;
		tempProp = gl_get_property(parent,"phases");

		if ((tempProp==NULL || tempProp->ptype!=PT_set))
		{
			GL_THROW("Unable to map phases property - ensure the parent is a meter or triplex_meter");
			/*  TROUBLESHOOT
			While attempting to map the phases property from the parent object, an error was encountered.
			Please check and make sure your parent object is a meter or triplex_meter inside the powerflow module and try
			again.  If the error persists, please submit your code and a bug report via the Trac website.
			*/
		}
		else
			phaseInfo = (set*)GETADDR(parent,tempProp);

		//Copy in so the code works
		phases = *phaseInfo;

	}
	else if (parent!=NULL && gl_object_isa(parent,"triplex_meter"))
	{
		parent_string = "triplex_meter";

		struct {
			complex **var;
			char *varname;
		}
		map[] = {
			// local object name,	meter object name
			{&pCircuit_V,			"voltage_12"}, // assumes 1N and 2N follow immediately in memory
			{&pLine_I,				"current_1"}, // assumes 2 and 3(N) follow immediately in memory
			{&pLine12,				"current_12"}, // maps current load 1-2 onto triplex load
			{&pPower,				"power_12"}, //assumes 2 and 1-2 follow immediately in memory
			{&pLine_unrotI,			"prerotated_current_12"},	//maps current load 1-2 (prerotated) for triplex
			/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
		};

		// attach meter variables to each circuit
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
		{
			if ((*(map[i].var) = get_complex(parent,map[i].varname))==NULL)
			{
				GL_THROW("%s (%s:%d) does not implement triplex_meter variable %s for %s (inverter:%d)", 
				/*	TROUBLESHOOT
					The Inverter requires that the triplex_meter contains certain published properties in order to properly connect
					the inverter to the triplex-meter.  If the triplex_meter does not contain those properties, GridLAB-D may
					suffer fatal pointer errors.  If you encounter this error, please report it to the developers, along with
					the version of GridLAB-D that raised this error.
				*/
				parent->name?parent->name:"unnamed object", parent->oclass->name, parent->id, map[i].varname, obj->name?obj->name:"unnamed", obj->id);
			}
		}

		//Map status
		pMeterStatus = get_enum(parent,"service_status");

		//Check it
		if (pMeterStatus==NULL)
		{
			GL_THROW("Inverter failed to map powerflow status variable");
			//Defined above
		}

		//Map phases
		set *phaseInfo;
		PROPERTY *tempProp;
		tempProp = gl_get_property(parent,"phases");

		if ((tempProp==NULL || tempProp->ptype!=PT_set))
		{
			GL_THROW("Unable to map phases property - ensure the parent is a meter or triplex_meter");
			//Defined above
		}
		else
			phaseInfo = (set*)GETADDR(parent,tempProp);

		//Copy in so the code works
		phases = *phaseInfo;
	}
	else if	((parent != NULL && strcmp(parent->oclass->name,"meter") != 0)||(parent != NULL && strcmp(parent->oclass->name,"triplex_meter") != 0))
	{
		throw("Inverter must have a meter or triplex meter as it's parent");
		/*  TROUBLESHOOT
		Check the parent object of the inverter.  The inverter is only able to be childed via a meter or 
		triplex meter when connecting into powerflow systems.  You can also choose to have no parent, in which
		case the inverter will be a stand-alone application using default voltage values for solving purposes.
		*/
	}
	else
	{
		parent_string = "none";
		
		struct {
			complex **var;
			char *varname;
		}
		map[] = {
		// local object name,	meter object name
			{&pCircuit_V,			"voltage_A"}, // assumes 2 and 3 follow immediately in memory
			{&pLine_I,				"current_A"}, // assumes 2 and 3(N) follow immediately in memory
		};

		gl_warning("Inverter:%d has no parent meter object defined; using static voltages", obj->id);
		
		// attach meter variables to each circuit in the default_meter
		*(map[0].var) = &default_line123_voltage[0];
		*(map[1].var) = &default_line1_current[0];

		//Attach meter status default
		pMeterStatus = &default_meter_status;

		// provide initial values for voltages
		default_line123_voltage[0] = complex(Rated_kV*1000/sqrt(3.0),0);
		default_line123_voltage[1] = complex(Rated_kV*1000/sqrt(3.0)*cos(2*PI/3),Rated_kV*1000/sqrt(3.0)*sin(2*PI/3));
		default_line123_voltage[2] = complex(Rated_kV*1000/sqrt(3.0)*cos(-2*PI/3),Rated_kV*1000/sqrt(3.0)*sin(-2*PI/3));
		default_meter_status = 1;

		// Declare all 3 phases
		phases = 0x07;
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

	if (gen_mode_v == UNKNOWN && inverter_type_v != FOUR_QUADRANT)
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
			
			//need to check for parameters SWITCH_TYPE, FILTER_TYPE, FILTER_IMPLEMENTATION, GENERATOR_MODE
	/*
			if (Rated_kW!=0.0)  SB = Rated_kW/sqrt(1-Rated_pf*Rated_pf);
			if (Rated_kVA!=0.0)  SB = Rated_kVA/3;
			if (Rated_kV!=0.0)  EB = Rated_kV/sqrt(3.0);
			if (SB!=0.0)  ZB = EB*EB/SB;
			else throw("Generator power capacity not specified!");
			double Real_Rinternal = Rinternal * ZB; 
			double Real_Rload = Rload * ZB;
			double Real_Rtotal = Rtotal * ZB;
			double Real_Rphase = Rphase * ZB;
			double Real_Rground = Rground * ZB;
			double Real_Rground_storage = Rground_storage * ZB;
			double[3] Real_Rfilter = Rfilter * ZB;

			double Real_Cinternal = Cinternal * ZB;
			double Real_Cground = Cground * ZB;
			double Real_Ctotal = Ctotal * ZB;
			double[3] Real_Cfilter = Cfilter * ZB;

			double Real_Linternal = Linternal * ZB;
			double Real_Lground = Lground * ZB;
			double Real_Ltotal = Ltotal * ZB;
			double[3] Real_Lfilter = Lfilter * ZB;

			tst = complex(Real_Rground,Real_Lground);
			AMx[0][0] = complex(Real_Rinternal,Real_Linternal) + tst;
			AMx[1][1] = complex(Real_Rinternal,Real_Linternal) + tst;
			AMx[2][2] = complex(Real_Rinternal,Real_Linternal) + tst;
		//	AMx[0][0] = AMx[1][1] = AMx[2][2] = complex(Real_Rs+Real_Rg,Real_Xs+Real_Xg);
			AMx[0][1] = AMx[0][2] = AMx[1][0] = AMx[1][2] = AMx[2][0] = AMx[2][1] = tst;

			*/

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
			if(use_multipoint_efficiency == FALSE){ 
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
				if (sense_object == NULL)
				{
					if (parent!=NULL)
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
					if (gl_object_isa(sense_object,"meter","powerflow") || gl_object_isa(sense_object,"triplex_meter","powerflow"))
					{
						//Set flag
						sense_is_link = false;

						//Map to measured_power - regardless of object
						sense_power = get_complex(sense_object,"measured_power");

						//Make sure it worked
						if (sense_power == NULL)
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
						if (powerCalc==NULL)
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
						sense_power = get_complex(sense_object,"power_out");

						//Make sure it worked
						if (sense_power == NULL)
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
			if(use_multipoint_efficiency == FALSE){ 
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

	//internal_switch_resistance(switch_type_choice);
	filter_circuit_impact((power_electronics::FILTER_TYPE)filter_type_v, 
		(power_electronics::FILTER_IMPLEMENTATION)filter_imp_v);

	//seting up defaults for multipoint efficiency
	if(use_multipoint_efficiency == TRUE){
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
	
	///////////////////////////////////////////////////////////////////////////
	// DELTA MODE
	///////////////////////////////////////////////////////////////////////////
		//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (enable_subsecond_models!=true)
		{
			gl_warning("inverter:%s indicates it wants to run deltamode, but the module-level flag is not set!",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			gen_object_count++;	//Increment the counter
			first_sync_delta_enabled = true;
		}

		//Check for frequency 
		if (enable_1547_compliance == true)
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
		if (enable_subsecond_models == true)
		{
			gl_warning("inverter:%d %s - Deltamode is enabled for the module, but not this inverter!",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The inverter is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this inverter may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
		else if (enable_1547_compliance == true)
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
	prev_time_dbl = (double)prev_time;

	VA_Out = complex(P_Out,Q_Out);
	//I_In = complex((VA_Out.Mag())/V_In.Mag(),0.0);
	P_Out_t0 = P_Out;
	Q_Out_t0 = Q_Out;
	power_factor_t0 = power_factor;
	I_Out[0] = complex(0);
	I_Out[1] = complex(0);
	I_Out[2] = complex(0);
	return 1;
	
}

TIMESTAMP inverter::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);
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
		else if (four_quadrant_control_mode == FQM_VOLT_VAR)
		{
			if ((phases & 0x10) == 0x10)	//Triplex
			{
				*pPower -= last_power[3];	//Theoretically pPower is mapped to power_12, which already has the [2] offset applied
			}
			else	//Variation of three-phase
			{
				pPower[0] -= last_power[0];
				pPower[1] -= last_power[1];
				pPower[2] -= last_power[2];
			}
		}//End VOLT_VAR
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
		
		if((deltamode_inclusive == true) && (enable_subsecond_models==true) && (t1 != t0) && (inverter_dyn_mode == PI_CONTROLLER)) {
			last_I_In = I_In.Re();
			for(int i = 0; i < 3; i++) {
				last_I_Out[i] = I_Out[i];
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
	
	complex rotate_value;
	complex calculated_iO[3];

	//Assume always want TS_NEVER
	tret_value = TS_NEVER;

	if(gen_status_v == OFFLINE){
		power_A = complex(0);
		power_B = complex(0);
		power_C = complex(0);
		P_Out = 0;
		Q_Out = 0;
		VA_Out = complex(0);
		if ((phases & 0x10) == 0x10) {
			last_power[3] = -power_A;
			*pPower += last_power[3];
		} else {
			p_in = 0;
			if ((phases & 0x01) == 0x01) {
				last_power[0] = -power_A;
				pPower[0] += last_power[0];
			}
			if ((phases & 0x02) == 0x02) {
				last_power[1] = -power_B;
				pPower[1] += last_power[1];
			}
			if ((phases & 0x04) == 0x04) {
				last_power[2] = -power_C;
				pPower[2] += last_power[2];
			}
		}
		return tret_value;
	}

	if (first_sync_delta_enabled == true)	//Deltamode first pass
	{
		//TODO: LOCKING!
		if ((deltamode_inclusive == true) && (enable_subsecond_models == true))	//We want deltamode - see if it's populated yet
		{
			//Very first run
			if (first_iter_counter == 0)
			{
				if ((gen_object_current == -1) || (delta_objects==NULL))
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
				if (delta_functions[gen_object_current] == NULL)
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
				if (post_delta_functions[gen_object_current] == NULL)
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
				if (delta_preupdate_functions[gen_object_current] == NULL)
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
			}

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

	//Perform 1547 checks, if appropriate
	if (enable_1547_compliance == true)
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
				if (deltamode_inclusive == true)
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
	
	if ((*pMeterStatus==1) && (inverter_1547_status == true))	//Make sure the meter is in service
	{
		phaseA_V_Out = pCircuit_V[0];	//Syncs the meter parent to the generator.
		phaseB_V_Out = pCircuit_V[1];
		phaseC_V_Out = pCircuit_V[2];

		internal_losses = 1 - calculate_loss(Rtotal, Ltotal, Ctotal, DC, AC);
		frequency_losses = 1 - calculate_frequency_loss(output_frequency, Rtotal,Ltotal, Ctotal);

		if(inverter_type_v != FOUR_QUADRANT)
		{
			switch(gen_mode_v)
			{
				case CONSTANT_PF:
					VA_In = V_In * ~ I_In; //DC

					// need to differentiate between different pulses...
					if(use_multipoint_efficiency == FALSE){
						VA_Out = VA_In * efficiency * internal_losses * frequency_losses;
					} else {
						if(VA_In <= p_so){
							VA_Out = 0;
						} else {
							if(V_In > v_dco){
								gl_warning("The dc voltage is greater than the specified maximum for the inverter. Efficiency model may be inaccurate.");
							}
							C1 = p_dco*(1+c_1*(V_In.Re()-v_dco));
							C2 = p_so*(1+c_2*(V_In.Re()-v_dco));
							C3 = c_o*(1+c_3*(V_In.Re()-v_dco));
							VA_Out.SetReal((((p_max/(C1-C2))-C3*(C1-C2))*(VA_In.Re()-C2)+C3*(VA_In.Re()-C2)*(VA_In.Re()-C2))*internal_losses*frequency_losses);
						}
					}
					//losses = VA_Out * Rtotal / (Rtotal + Rload);
					//VA_Out = VA_Out * Rload / (Rtotal + Rload);

					if ((phases & 0x10) == 0x10)  //Triplex-line -> Assume it's only across the 240 V for now.
					{
						power_A = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)));
						if (phaseA_V_Out.Mag() != 0.0)
							phaseA_I_Out = ~(power_A / phaseA_V_Out);
						else
							phaseA_I_Out = complex(0.0,0.0);

						*pLine12 += -phaseA_I_Out;

						//Update this value for later removal
						last_current[3] = -phaseA_I_Out;
						
						//Get rid of these for now
						//complex phaseA_V_Internal = filter_voltage_impact_source(phaseA_I_Out, phaseA_V_Out);
						//phaseA_I_Out = filter_current_impact_out(phaseA_I_Out, phaseA_V_Internal);
					}
					else if (number_of_phases_out == 3) // All three phases
					{
						power_A = power_B = power_C = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)))/3;
						if (phaseA_V_Out.Mag() != 0.0)
							phaseA_I_Out = ~(power_A / phaseA_V_Out); // /sqrt(2.0);
						else
							phaseA_I_Out = complex(0.0,0.0);
						if (phaseB_V_Out.Mag() != 0.0)
							phaseB_I_Out = ~(power_B / phaseB_V_Out); // /sqrt(2.0);
						else
							phaseB_I_Out = complex(0.0,0.0);
						if (phaseC_V_Out.Mag() != 0.0)
							phaseC_I_Out = ~(power_C / phaseC_V_Out); // /sqrt(2.0);
						else
							phaseC_I_Out = complex(0.0,0.0);

						pLine_I[0] += -phaseA_I_Out;
						pLine_I[1] += -phaseB_I_Out;
						pLine_I[2] += -phaseC_I_Out;

						//Update this value for later removal
						last_current[0] = -phaseA_I_Out;
						last_current[1] = -phaseB_I_Out;
						last_current[2] = -phaseC_I_Out;

						//complex phaseA_V_Internal = filter_voltage_impact_source(phaseA_I_Out, phaseA_V_Out);
						//complex phaseB_V_Internal = filter_voltage_impact_source(phaseB_I_Out, phaseB_V_Out);
						//complex phaseC_V_Internal = filter_voltage_impact_source(phaseC_I_Out, phaseC_V_Out);

						//phaseA_I_Out = filter_current_impact_out(phaseA_I_Out, phaseA_V_Internal);
						//phaseB_I_Out = filter_current_impact_out(phaseB_I_Out, phaseB_V_Internal);
						//phaseC_I_Out = filter_current_impact_out(phaseC_I_Out, phaseC_V_Internal);
					}
					else if(number_of_phases_out == 2) // two-phase connection
					{
						OBJECT *obj = OBJECTHDR(this);

						if ( ((phases & 0x01) == 0x01) && phaseA_V_Out.Mag() != 0)
						{
							power_A = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)))/2;;
							phaseA_I_Out = ~(power_A / phaseA_V_Out);
						}
						else 
							phaseA_I_Out = complex(0,0);

						if ( ((phases & 0x02) == 0x02) && phaseB_V_Out.Mag() != 0)
						{
							power_B = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)))/2;;
							phaseB_I_Out = ~(power_B / phaseB_V_Out);
						}
						else 
							phaseB_I_Out = complex(0,0);

						if ( ((phases & 0x04) == 0x04) && phaseC_V_Out.Mag() != 0)
						{
							power_C = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)))/2;;
							phaseC_I_Out = ~(power_C / phaseC_V_Out);
						}
						else 
							phaseC_I_Out = complex(0,0);

						pLine_I[0] += -phaseA_I_Out;
						pLine_I[1] += -phaseB_I_Out;
						pLine_I[2] += -phaseC_I_Out;

						//Update this value for later removal
						last_current[0] = -phaseA_I_Out;
						last_current[1] = -phaseB_I_Out;
						last_current[2] = -phaseC_I_Out;

					}
					else // Single phase connection
					{
						if( ((phases & 0x01) == 0x01) && phaseA_V_Out.Mag() != 0)
						{
							power_A = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)));
							phaseA_I_Out = ~(power_A / phaseA_V_Out); 
							//complex phaseA_V_Internal = filter_voltage_impact_source(phaseA_I_Out, phaseA_V_Out);
							//phaseA_I_Out = filter_current_impact_out(phaseA_I_Out, phaseA_V_Internal);
						}
						else if( ((phases & 0x02) == 0x02) && phaseB_V_Out.Mag() != 0)
						{
							power_B = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)));
							phaseB_I_Out = ~(power_B / phaseB_V_Out); 
							//complex phaseB_V_Internal = filter_voltage_impact_source(phaseB_I_Out, phaseB_V_Out);
							//phaseB_I_Out = filter_current_impact_out(phaseB_I_Out, phaseB_V_Internal);
						}
						else if( ((phases & 0x04) == 0x04) && phaseC_V_Out.Mag() != 0)
						{
							power_C = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)));
							phaseC_I_Out = ~(power_C / phaseC_V_Out); 
							//complex phaseC_V_Internal = filter_voltage_impact_source(phaseC_I_Out, phaseC_V_Out);
							//phaseC_I_Out = filter_current_impact_out(phaseC_I_Out, phaseC_V_Internal);
						}
						else
						{
							gl_warning("None of the phases specified have voltages!");
							phaseA_I_Out = phaseB_I_Out = phaseC_I_Out = complex(0.0,0.0);
						}
						pLine_I[0] += -phaseA_I_Out;
						pLine_I[1] += -phaseB_I_Out;
						pLine_I[2] += -phaseC_I_Out;

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
					//TODO
					//gather V_Out for each phase
					//gather V_In (DC) from line -- can not gather V_In, for now set equal to V_Out
					//P_Out is either set or input from elsewhere
					//Q_Out is either set or input from elsewhere
					//Gather Rload

					if(strcmp(parent_string, "meter") == 0 || strcmp(parent_string, "triplex_meter") == 0)
					{
						VA_Out = complex(P_Out,Q_Out);
					}
					else
					{
						phaseA_I_Out = pLine_I[0];
						phaseB_I_Out = pLine_I[1];
						phaseC_I_Out = pLine_I[2];

						//Erm, there's no good way to handle this from a "multiply attached" point of view.
						//TODO: Think about how to do this if the need arrises

						VA_Out = phaseA_V_Out * (~ phaseA_I_Out) + phaseB_V_Out * (~ phaseB_I_Out) + phaseC_V_Out * (~ phaseC_I_Out);
					}

					pf_out = P_Out/VA_Out.Mag();
					
					//VA_Out = VA_In * efficiency * internal_losses;

					if ( (phases & 0x07) == 0x07) // Three phase
					{
						power_A = power_B = power_C = VA_Out /3;
						phaseA_I_Out = (power_A / phaseA_V_Out); // /sqrt(2.0);
						phaseB_I_Out = (power_B / phaseB_V_Out); // /sqrt(2.0);
						phaseC_I_Out = (power_C / phaseC_V_Out); // /sqrt(2.0);

						phaseA_I_Out = ~ phaseA_I_Out;
						phaseB_I_Out = ~ phaseB_I_Out;
						phaseC_I_Out = ~ phaseC_I_Out;

					}
					else if ( (number_of_phases_out == 1) && ((phases & 0x01) == 0x01) ) // Phase A only
					{
						power_A = VA_Out;
						phaseA_I_Out = (power_A / phaseA_V_Out); // /sqrt(2);
						phaseA_I_Out = ~ phaseA_I_Out;
					}
					else if ( (number_of_phases_out == 1) && ((phases & 0x02) == 0x02) ) // Phase B only
					{
						power_B = VA_Out;
						phaseB_I_Out = (power_B / phaseB_V_Out);  // /sqrt(2);
						phaseB_I_Out = ~ phaseB_I_Out;
					}
					else if ( (number_of_phases_out == 1) && ((phases & 0x04) == 0x04) ) // Phase C only
					{
						power_C = VA_Out;
						phaseC_I_Out = (power_C / phaseC_V_Out); // /sqrt(2);
						phaseC_I_Out = ~ phaseC_I_Out;
					}
					else
					{
						throw ("unsupported number of phases");
					}

					VA_In = VA_Out / (efficiency * internal_losses * frequency_losses);
					losses = VA_Out * (1 - (efficiency * internal_losses * frequency_losses));

					//V_In = complex(0,0);
					//
					////is there a better way to do this?
					//if(phaseAOut){
					//	V_In += abs(phaseA_V_Out.Re());
					//}
					//if(phaseBOut){
					//	V_In += abs(phaseB_V_Out.Re());
					//}
					//if(phaseCOut){
					//	V_In += abs(phaseC_V_Out.Re());
					//}else{
					//	throw ("none of the phases have voltages!");
					//

					V_In.Re() = Vdc;



					I_In = VA_In / V_In;
					I_In = ~I_In;

					V_In = filter_voltage_impact_source(I_In, V_In);
					I_In = filter_current_impact_source(I_In, V_In);

					gl_verbose("Inverter sync: V_In asked for by inverter is: (%f , %f)", V_In.Re(), V_In.Im());
					gl_verbose("Inverter sync: I_In asked for by inverter is: (%f , %f)", I_In.Re(), I_In.Im());


					pLine_I[0] += phaseA_I_Out;
					pLine_I[1] += phaseB_I_Out;
					pLine_I[2] += phaseC_I_Out;

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
					
					//TODO
					//Gather V_Out
					//Gather VA_Out
					//Gather Rload
					if(phaseAOut)
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
					if (phaseBOut)
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
					if (phaseCOut)
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
					
					power_A = (~phaseA_I_Out) * phaseA_V_Out;
					power_B = (~phaseB_I_Out) * phaseB_V_Out;
					power_C = (~phaseC_I_Out) * phaseC_V_Out;

					//check if inverter is overloaded -- if so, cap at max power
					if (((power_A + power_B + power_C) > Rated_kVA) ||
						((power_A.Re() + power_B.Re() + power_C.Re()) > Max_P) ||
						((power_A.Im() + power_B.Im() + power_C.Im()) > Max_Q))
					{
						VA_Out = Rated_kVA / number_of_phases_out;
						//if it's maxed out, don't ask for the simulator to re-call
						changed = false;
						if(phaseAOut)
						{
							phaseA_I_Out = VA_Out / phaseA_V_Out;
							phaseA_I_Out = (~phaseA_I_Out);
						}
						if(phaseBOut)
						{
							phaseB_I_Out = VA_Out / phaseB_V_Out;
							phaseB_I_Out = (~phaseB_I_Out);
						}
						if(phaseCOut)
						{
							phaseC_I_Out = VA_Out / phaseC_V_Out;
							phaseC_I_Out = (~phaseC_I_Out);
						}
					}
					
					//check if power is negative for some reason, should never be
					if(power_A < 0)
					{
						power_A = 0;
						phaseA_I_Out = 0;
						throw("phaseA power is negative!");
					}
					if(power_B < 0)
					{
						power_B = 0;
						phaseB_I_Out = 0;
						throw("phaseB power is negative!");
					}
					if(power_C < 0)
					{
						power_C = 0;
						phaseC_I_Out = 0;
						throw("phaseC power is negative!");
					}

					VA_In = VA_Out / (efficiency * internal_losses * frequency_losses);
					losses = VA_Out * (1 - (efficiency * internal_losses * frequency_losses));

					//V_In = complex(0,0);
					//
					////is there a better way to do this?
					//if(phaseAOut){
					//	V_In += abs(phaseA_V_Out.Re());
					//}
					//if(phaseBOut){
					//	V_In += abs(phaseB_V_Out.Re());
					//
					//if(phaseCOut){
					//	V_In += abs(phaseC_V_Out.Re());
					//else{
					//	throw ("none of the phases have voltages!");
					//

					V_In.Re() = Vdc;

					I_In = VA_In / V_In;
					I_In  = ~I_In;
					
					gl_verbose("Inverter sync: I_In asked for by inverter is: (%f , %f)", I_In.Re(), I_In.Im());

					V_In = filter_voltage_impact_source(I_In, V_In);
					I_In = filter_current_impact_source(I_In, V_In);

					//TODO: check P and Q components to see if within bounds

					if(changed)
					{
						pLine_I[0] += phaseA_I_Out;
						pLine_I[1] += phaseB_I_Out;
						pLine_I[2] += phaseC_I_Out;
						
						//Update this value for later removal
						last_current[0] = phaseA_I_Out;
						last_current[1] = phaseB_I_Out;
						last_current[2] = phaseC_I_Out;

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
						pLine_I[0] += phaseA_I_Out;
						pLine_I[1] += phaseB_I_Out;
						pLine_I[2] += phaseC_I_Out;

						//Update this value for later removal
						last_current[0] = phaseA_I_Out;
						last_current[1] = phaseB_I_Out;
						last_current[2] = phaseC_I_Out;
					}
					break;
				}
				case SUPPLY_DRIVEN: 
					GL_THROW("SUPPLY_DRIVEN mode for inverters not supported at this time");
					break;
				default:
					pLine_I[0] += phaseA_I_Out;
					pLine_I[1] += phaseB_I_Out;
					pLine_I[2] += phaseC_I_Out;

					//Update this value for later removal
					last_current[0] = phaseA_I_Out;
					last_current[1] = phaseB_I_Out;
					last_current[2] = phaseC_I_Out;

					break;
			}
		} 
		else	//FOUR_QUADRANT code
		{
			//FOUR_QUADRANT model (originally written for NAS/CES, altered for PV)
			double VA_Efficiency, temp_PF, temp_QVal, P_in, net_eff; //Ab added last two
			complex temp_VA;
			complex battery_power_out = complex(0,0);
			if (four_quadrant_control_mode != FQM_VOLT_VAR) {
				//Compute power in - supposedly DC, but since it's complex, we'll be proper (other models may need fixing)
				VA_In = V_In * ~ I_In;

				//Compute the power contribution of the battery object
				if((phases & 0x10) == 0x10){ // split phase
					battery_power_out = power_A;
				} else { // three phase
					if((phases & 0x01) == 0x01){ // has phase A
						battery_power_out += power_A;
					}
					if((phases & 0x02) == 0x02){ // hase phase B
						battery_power_out += power_B;
					}
					if((phases & 0x04) == 0x04){ // has phase C
						battery_power_out += power_C;
					}
				}
				//Determine how to efficiency weight it
				if(use_multipoint_efficiency == false)
				{
					//Normal scaling
					VA_Efficiency = VA_In.Re() * efficiency * internal_losses * frequency_losses;
					//Ab add
					P_in = fabs(VA_In.Re());
					net_eff = efficiency * internal_losses * frequency_losses;
					//end Ab add
				}
				else
				{
					//See if above minimum DC power input
					if(VA_In.Mag() <= p_so)
					{
						VA_Efficiency = 0.0;	//Nope, no output
						//Ab add
						P_in = 0;
						net_eff = 0;
						//end Ab add
					}
					else	//Yes, apply effiency change
					{
						//Make sure voltage isn't too low
						if(V_In.Mag() > v_dco)
						{
							gl_warning("The dc voltage is greater than the specified maximum for the inverter. Efficiency model may be inaccurate.");
							/*  TROUBLESHOOT
							The DC voltage at the input to the inverter is less than the maximum voltage supported by the inverter.  As a result, the
							multipoint efficiency model may not provide a proper result.
							*/
						}

						//Compute coefficients for multipoint efficiency
						C1 = p_dco*(1+c_1*(V_In.Re()-v_dco));
						C2 = p_so*(1+c_2*(V_In.Re()-v_dco));
						C3 = c_o*(1+c_3*(V_In.Re()-v_dco));

						//Apply this to the output
						VA_Efficiency = (((p_max/(C1-C2))-C3*(C1-C2))*(VA_In.Re()-C2)+C3*(VA_In.Re()-C2)*(VA_In.Re()-C2))*internal_losses*frequency_losses;
						//Ab add
						P_in = fabs(VA_In.Re());
						net_eff = fabs(VA_Efficiency / P_in);
						//end Ab add
					}
				}
				VA_Efficiency += battery_power_out.Mag();
			} else {
				
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

				//Compute power in - supposedly DC, but since it's complex, we'll be proper (other models may need fixing)
				VA_In = V_In * ~ I_In;
				//Determine how to efficiency weight it
				if(use_multipoint_efficiency == false)
				{
					//Normal scaling
					VA_Efficiency = VA_In.Re() * efficiency * internal_losses * frequency_losses;
				}
				else
				{
					//See if above minimum DC power input
					if(VA_In.Mag() <= p_so)
					{
						VA_Efficiency = 0.0;	//Nope, no output
					}
					else	//Yes, apply effiency change
					{
						//Make sure voltage isn't too low
						if(V_In.Mag() > v_dco)
						{
							gl_warning("The dc voltage is greater than the specified maximum for the inverter. Efficiency model may be inaccurate.");
							/*  TROUBLESHOOT
							The DC voltage at the input to the inverter is less than the maximum voltage supported by the inverter.  As a result, the
							multipoint efficiency model may not provide a proper result.
							*/
						}

						//Compute coefficients for multipoint efficiency
						C1 = p_dco*(1+c_1*(V_In.Re()-v_dco));
						C2 = p_so*(1+c_2*(V_In.Re()-v_dco));
						C3 = c_o*(1+c_3*(V_In.Re()-v_dco));

						//Apply this to the output
						VA_Efficiency = (((p_max/(C1-C2))-C3*(C1-C2))*(VA_In.Re()-C2)+C3*(VA_In.Re()-C2)*(VA_In.Re()-C2))*internal_losses*frequency_losses;
					}
				}
				if ((phases & 0x10) == 0x10){
					power_A.SetReal(VA_Efficiency);
				} else {
					if ((phases & 0x01) == 0x01) {
						power_A.SetReal(VA_Efficiency/number_of_phases_out);
					}
					if ((phases & 0x02) == 0x02) {
						power_B.SetReal(VA_Efficiency/number_of_phases_out);
					}
					if ((phases & 0x04) == 0x04) {
						power_C.SetReal(VA_Efficiency/number_of_phases_out);
					}
				}
			}

			//Determine 4 quadrant outputs
			if(four_quadrant_control_mode == FQM_CONSTANT_PF)	//Power factor mode
			{
				if(power_factor != 0.0)	//Not purely imaginary
				{
					if (VA_In<0.0)	//Discharge at input, so must be "load"
					{
						//Total power output is the magnitude
						VA_Out.SetReal(VA_Efficiency*-1.0);
					}
					else if (VA_In>0.0)	//Positive input, so must be generator
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
					VA_Out = complex(0.0,VA_Efficiency);
				}
			}
			else if (four_quadrant_control_mode == FQM_CONSTANT_PQ)
			{
				//Compute desired output - sign convention appears to be backwards
				temp_VA = complex(P_Out,Q_Out);

				
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
							VA_Out = complex(temp_VA.Re(),-temp_QVal);
						}
						else	//Positive Q dispatch
						{
							VA_Out = complex(temp_VA.Re(),temp_QVal);
						}
					}
					else	//Inverter rated power is equal to or smaller than real power desired, give it all we can
					{
						//Maintain desired sign convention
						if (temp_VA.Re() < 0.0)
						{
							VA_Out = complex(-p_max,0.0);
						}
						else	//Positive
						{
							VA_Out = complex(p_max,0.0);
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
				//}
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
					
					//TODO : add lookup for power for frequency regulation Pout_fr
					
					if((VA_In.Re() == 0.0) && (disable_volt_var_if_no_input_power == true))		
						VA_Out = complex(0,0);
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
	
						double Po = (P_in * net_eff) - fabs(Qo) * (1 - net_eff)/net_eff;

						if(VA_In.Re() < 0.0)
							VA_Out = complex(Po,-Qo);	//Qo sign convention backwards from what i was expecting			
						else
							VA_Out = complex(-Po,-Qo);	//Qo sign convention backwards from what i was expecting
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
			if (four_quadrant_control_mode != FQM_VOLT_VAR){
				//check to see if VA_Out is within rated absolute power rating
				if(VA_Out.Mag() > p_max)
				{
					//Determine the excess, for use elsewhere - back out simple efficiencies
					excess_input_power = (VA_Out.Mag() - p_max)/(internal_losses*frequency_losses);

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
					temp_VA = complex(fabs(p_max*temp_PF),fabs(p_max*sqrt(1.0-(temp_PF*temp_PF))));

					//"Sign" it appropriately
					if ((VA_Out.Re()<0) && (VA_Out.Im()<0))	//-R, -I
					{
						VA_Out = -temp_VA;
					}
					else if ((VA_Out.Re()<0) && (VA_Out.Im()>=0))	//-R,I
					{
						VA_Out = complex(-temp_VA.Re(),temp_VA.Im());
					}
					else if ((VA_Out.Re()>=0) && (VA_Out.Im()<0))	//R,-I
					{
						VA_Out = complex(temp_VA.Re(),-temp_VA.Im());
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
				if(four_quadrant_control_mode != FQM_CONSTANT_PQ){
					P_Out = VA_Out.Re();
					Q_Out = VA_Out.Im();
				}

				//Calculate power and post it
				if ((phases & 0x10) == 0x10) // split phase
				{
					//Update last_power variable
					last_power[3] = -VA_Out;

					//Post the value
					I_Out[0] = ~(VA_Out / *pCircuit_V);
					if (deltamode_inclusive == true)
					{
						last_current[3] = -I_Out[0];
						pLine_unrotI[0] += last_current[3];
					}
					else
					{
						*pPower +=last_power[3];
					}
				}
				else	//Three phase variant
				{
					//Figure out amount that needs to be posted
					temp_VA = -VA_Out/number_of_phases_out;

					if ( (phases & 0x01) == 0x01 ) // has phase A
					{
						last_power[0] = temp_VA;	//Store last power
						I_Out[0] = ~(-temp_VA / pCircuit_V[0]);
						if (deltamode_inclusive == true)
						{
							last_current[0] = -I_Out[0];
							pLine_unrotI[0] += last_current[0];
						}
						else
						{
							pPower[0] += temp_VA;		//Post the current value
						}
					}

					if ( (phases & 0x02) == 0x02 ) // has phase B
					{
						last_power[1] = temp_VA;	//Store last power
						I_Out[1] = ~(-temp_VA / pCircuit_V[1]);
						if (deltamode_inclusive == true)
						{
							last_current[1] = -I_Out[1];
							pLine_unrotI[1] += last_current[1];
						}
						else
						{
							pPower[1] += temp_VA;		//Post the current value
						}
					}

					if ( (phases & 0x04) == 0x04 ) // has phase C
					{
						last_power[2] = temp_VA;	//Store last power
						I_Out[2] = ~(-temp_VA / pCircuit_V[2]);
						if (deltamode_inclusive == true)
						{
							last_current[2] = -I_Out[2];
							pLine_unrotI[2] += last_current[2];
						}
						else
						{
							pPower[2] += temp_VA;		//Post the current value
						}
					}
				}//End three-phase variant
			} else { // Volt Var Control mode
				if (power_A.Mag() > p_rated ) {
					if (power_A.Re() > p_rated) {
						power_A.SetReal(p_rated);
						power_A.SetImag(0);
					} else if (power_A.Re() < -p_rated) {
						power_A.SetReal(-p_rated);
						power_A.SetImag(0);
					} else if (power_A.Re() < p_rated && power_A.Re() > -p_rated) {
						double q_max = 0;
						q_max = sqrt((p_rated * p_rated) - (power_A.Re() * power_A.Re()));
						if (power_A.Im() > q_max) {
							power_A.SetImag(q_max);
						} else {
							power_A.SetImag(-q_max);
						}
					}
				}
				if (power_B.Mag() > p_rated ) {
					if (power_B.Re() > p_rated) {
						power_B.SetReal(p_rated);
						power_B.SetImag(0);
					} else if (power_B.Re() < -p_rated) {
						power_B.SetReal(-p_rated);
						power_B.SetImag(0);
					} else if (power_B.Re() < p_rated && power_B.Re() > -p_rated) {
						double q_max = 0;
						q_max = sqrt((p_rated * p_rated) - (power_B.Re() * power_B.Re()));
						if (power_B.Im() > q_max) {
							power_B.SetImag(q_max);
						} else {
							power_B.SetImag(-q_max);
						}
					}
				}
				if (power_C.Mag() > p_rated ) {
					if (power_C.Re() > p_rated) {
						power_C.SetReal(p_rated);
						power_C.SetImag(0);
					} else if (power_C.Re() < -p_rated) {
						power_C.SetReal(-p_rated);
						power_C.SetImag(0);
					} else if (power_C.Re() < p_rated && power_C.Re() > -p_rated) {
						double q_max = 0;
						q_max = sqrt((p_rated * p_rated) - (power_C.Re() * power_C.Re()));
						if (power_C.Im() > q_max) {
							power_C.SetImag(q_max);
						} else {
							power_C.SetImag(-q_max);
						}
					}
				}
				if ((phases & 0x10) == 0x10) {
					p_in = power_A.Re() / inv_eta;
					last_power[3] = -power_A;
					*pPower += last_power[3];
					I_Out[0] = ~(VA_Out / *pCircuit_V);
				} else {
					p_in = 0;
					if ((phases & 0x01) == 0x01) {
						p_in += power_A.Re()/inv_eta;
						last_power[0] = -power_A;
						pPower[0] += last_power[0];
						I_Out[0] = ~(VA_Out / *pCircuit_V);
					}
					if ((phases & 0x02) == 0x02) {
						p_in += power_B.Re()/inv_eta;
						last_power[1] = -power_B;
						pPower[1] += last_power[1];
						I_Out[1] = ~(VA_Out / *pCircuit_V);
					}
					if ((phases & 0x04) == 0x04) {
						p_in += power_C.Re()/inv_eta;
						last_power[2] = -power_C;
						pPower[2] += last_power[2];
						I_Out[2] = ~(VA_Out / *pCircuit_V);
					}
				}
			}
			//Negate VA_Out, so it matches sign ideals
			//VA_Out = -VA_Out;

		}//End FOUR_QUADRANT mode
	}
	else
	{
		//Check to see if we're accumulating or out of service
		if (*pMeterStatus==1)
		{
			if (inverter_type_v != FOUR_QUADRANT)
			{
				//Will only get here on true NR_cycle, if meter is in service
				if ((phases & 0x10) == 0x10)
				{
					*pLine12 += last_current[3];
				}
				else
				{
					pLine_I[0] += last_current[0];
					pLine_I[1] += last_current[1];
					pLine_I[2] += last_current[2];
				}
			}
			else	//Four-quadrant post as power
			{
				//Will only get here on true NR_cycle, if meter is in service
				if ((phases & 0x10) == 0x10)	//Triplex
				{
					*pPower += last_power[3];	//Theoretically pPower is mapped to power_12, which already has the [2] offset applied
				}
				else
				{
					pPower[0] += last_power[0];
					pPower[1] += last_power[1];
					pPower[2] += last_power[2];
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

	//Check and see if we need to redispatch
	if ((inverter_type_v == FOUR_QUADRANT) && (four_quadrant_control_mode == FQM_LOAD_FOLLOWING) && (lf_dispatch_change_allowed==true))
	{
		//See what the sense_object is and determine if we need to update
		if (sense_is_link)
		{
			//Perform the power update
			((void (*)(OBJECT *))(*powerCalc))(sense_object);
		}//End link update of power

		//Extract power, mainly just for convenience, but also to offset us for what we are currently "ordering"
		curr_power_val = sense_power->Re();

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
	if (inverter_type_v == FOUR_QUADRANT && four_quadrant_control_mode == FQM_GROUP_LF && lf_dispatch_change_allowed==true)
	{
		//See what the sense_object is and determine if we need to update
		if (sense_is_link)
		{
			//Perform the power update
			((void (*)(OBJECT *))(*powerCalc))(sense_object);
		}//End link update of power

		//Extract power, mainly just for convenience, but also to offset us for what we are currently "ordering"
		curr_power_val = sense_power->Re();

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
	else if ((inverter_type_v == FOUR_QUADRANT) && (four_quadrant_control_mode == FQM_VOLT_VAR))
	{
		if (t1 >= allowed_vv_action && (t1 > last_vv_check)) {
			vv_operation = false;
			last_vv_check = t1;
			if ((phases & 0x10) == 0x10) {
				if((pCircuit_V[0].Mag() / V_base) <= V1) {
					power_A.SetImag(Q1 * p_rated);
				} else if ((pCircuit_V[0].Mag() / V_base) <= V2 && V1 < (pCircuit_V[0].Mag() / V_base)) {
					power_A.SetImag(getVar((pCircuit_V[0].Mag() / V_base), m12, b12) * p_rated);
				} else if ((pCircuit_V[0].Mag() / V_base) <= V3 && V2 < (pCircuit_V[0].Mag() / V_base)) {
					power_A.SetImag(getVar((pCircuit_V[0].Mag() / V_base), m23, b23) * p_rated);
				} else if ((pCircuit_V[0].Mag() / V_base) <= V4 && V3 < (pCircuit_V[0].Mag() / V_base)) {
					power_A.SetImag(getVar((pCircuit_V[0].Mag() / V_base), m34, b34) * p_rated);
				} else if (V4 < (pCircuit_V[0].Mag() / V_base)) {
					power_A.SetImag(Q4 * p_rated);
				}
				if (last_power->Im() != power_A.Im()) {
					vv_operation = true;
				}
			} else {
				if ((phases & 0x01) == 0x01) {
					if((pCircuit_V[0].Mag() / V_base) <= V1) {
						power_A.SetImag(Q1 * p_rated);
					} else if ((pCircuit_V[0].Mag() / V_base) <= V2 && V1 < (pCircuit_V[0].Mag() / V_base)) {
						power_A.SetImag(getVar((pCircuit_V[0].Mag() / V_base), m12, b12) * p_rated);
					} else if ((pCircuit_V[0].Mag() / V_base) <= V3 && V2 < (pCircuit_V[0].Mag() / V_base)) {
						power_A.SetImag(getVar((pCircuit_V[0].Mag() / V_base), m23, b23) * p_rated);
					} else if ((pCircuit_V[0].Mag() / V_base) <= V4 && V3 < (pCircuit_V[0].Mag() / V_base)) {
						power_A.SetImag(getVar((pCircuit_V[0].Mag() / V_base), m34, b34) * p_rated);
					} else if (V4 < (pCircuit_V[0].Mag() / V_base)) {
						power_A.SetImag(Q4 * p_rated);
					}
					if (last_power[0].Im() != power_A.Im()) {
						vv_operation = true;
					}
				}
				if ((phases & 0x02) == 0x02) {
					if((pCircuit_V[1].Mag() / V_base) <= V1) {
						power_B.SetImag(Q1 * p_rated);
					} else if ((pCircuit_V[1].Mag() / V_base) <= V2 && V1 < (pCircuit_V[1].Mag() / V_base)) {
						power_B.SetImag(getVar((pCircuit_V[1].Mag() / V_base), m12, b12) * p_rated);
					} else if ((pCircuit_V[1].Mag() / V_base) <= V3 && V2 < (pCircuit_V[1].Mag() / V_base)) {
						power_B.SetImag(getVar((pCircuit_V[1].Mag() / V_base), m23, b23) * p_rated);
					} else if ((pCircuit_V[1].Mag() / V_base) <= V4 && V3 < (pCircuit_V[1].Mag() / V_base)) {
						power_B.SetImag(getVar((pCircuit_V[1].Mag() / V_base), m34, b34) * p_rated);
					} else if (V4 < (pCircuit_V[1].Mag() / V_base)) {
						power_B.SetImag(Q4 * p_rated);
					}
					if (last_power[1].Im() != power_B.Im()) {
						vv_operation = true;
					}
				}
				if ((phases & 0x04) == 0x04) {
					if((pCircuit_V[2].Mag() / V_base) <= V1) {
						power_C.SetImag(Q1 * p_rated);
					} else if ((pCircuit_V[2].Mag() / V_base) <= V2 && V1 < (pCircuit_V[2].Mag() / V_base)) {
						power_C.SetImag(getVar((pCircuit_V[2].Mag() / V_base), m12, b12) * p_rated);
					} else if ((pCircuit_V[2].Mag() / V_base) <= V3 && V2 < (pCircuit_V[2].Mag() / V_base)) {
						power_C.SetImag(getVar((pCircuit_V[2].Mag() / V_base), m23, b23) * p_rated);
					} else if ((pCircuit_V[2].Mag() / V_base) <= V4 && V3 < (pCircuit_V[2].Mag() / V_base)) {
						power_C.SetImag(getVar((pCircuit_V[2].Mag() / V_base), m34, b34) * p_rated);
					} else if (V4 < (pCircuit_V[2].Mag() / V_base)) {
						power_C.SetImag(Q4 * p_rated);
					}
					if (last_power[2].Im() != power_C.Im()) {
						vv_operation = true;
					}
				}
			}
			if (vv_operation) {
				t2 = t1;
				allowed_vv_action = (TIMESTAMP)floor((double)t1 + vv_lockout + 0.5);
			}
		}
	}

	//***************************************************************************************
	//Power-factor regulation, modifying VA_Out to use any available VAs for power-factor regulation
	if ((inverter_type_v == FOUR_QUADRANT) && (pf_reg != EXCLUDED) && (pf_reg_dispatch_change_allowed==true))
	{
		//Need to modify dispatch from load following and/or adjust the reactive power output of the inverter to meet power factor regulation needs.
		//See what the sense_object is and determine if we need to update
		if (sense_is_link)
		{
			//Perform the power update
			((void (*)(OBJECT *))(*powerCalc))(sense_object);
		}//End link update of power

		//Extract power to calculate the necessary reactive power output and offset us for what we are currently "ordering"
		curr_real_power_val = sense_power->Re();
		curr_reactive_power_val = sense_power->Im();
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
			*pLine12 -= last_current[3];	//Remove from current12
		}
		else	//Some variant of three-phase
		{
			//Remove our parent contributions (so XMLs look proper)
			pLine_I[0] -= last_current[0];
			pLine_I[1] -= last_current[1];
			pLine_I[2] -= last_current[2];
		}
	}
	else	//Must be four quadrant (load_following or otherwise)
	{
		if (four_quadrant_control_mode != FQM_VOLT_VAR) {
			if ((phases & 0x10) == 0x10)	//Triplex
			{
				if (deltamode_inclusive == true)
				{
					pLine_unrotI[0] -= last_current[3];
				}
				else
				{
					*pPower -= last_power[3];	//Theoretically pPower is mapped to power_12, which already has the [2] offset applied
				}
			}
			else	//Variation of three-phase
			{
				if (deltamode_inclusive == true)
				{
					pLine_unrotI[0] -= last_current[0];
					pLine_unrotI[1] -= last_current[1];
					pLine_unrotI[2] -= last_current[2];
				}
				else
				{
					pPower[0] -= last_power[0];
					pPower[1] -= last_power[1];
					pPower[2] -= last_power[2];
				}
			}
		}
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

	//Just return a pass - not sure how we'd fail
	return SUCCESS;
}

//Module-level call
SIMULATIONMODE inverter::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	double deltat;
	double deltatimedbl;
	int indexval;
	complex derror[3];
	complex pid_out[3];
	double temp_val_d, temp_val_q;
	complex work_power_vals;
	double prev_error;
	bool deltaConverged = false;
	int i;
	double ieee_1547_double;

	SIMULATIONMODE simmode_return_value = SM_EVENT;

	//Get timestep value
	deltat = (double)dt/(double)DT_SECOND;

	if (iteration_count_val == 0)	//Only update timestamp tracker on first iteration
	{
		//Get decimal timestamp value
		deltatimedbl = (double)delta_time/(double)DT_SECOND; 

		//Update tracking variable
		prev_time_dbl = (double)gl_globalclock + deltatimedbl;
	}
	
	//Perform the 1547 update, if enabled
	if ((enable_1547_compliance == true) && (iteration_count_val == 0))	//Always just do on the first pass
	{
		//Do the checks
		ieee_1547_double = perform_1547_checks(deltat);
	}

	//See if we are actually enabled
	if (inverter_1547_status == true)
	{
		//See which mode we're in
		if (inverter_dyn_mode == PI_CONTROLLER)
		{
			//Initializate the state of the inverter
			if (delta_time==0)	//First run of new delta call
			{
				if(iteration_count_val == 0) {
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
					simmode_return_value =  SM_DELTA_ITER; // iterate so I know what my current power out is
				} else if(iteration_count_val == 1) {
					//calculate my current power out
					if((phases & 0x10) == 0x10) {
						VA_Out = *pCircuit_V * ~(I_Out[0]);
					}
					if((phases & 0x07) == 0x07) {
						VA_Out = (pCircuit_V[0] * ~(I_Out[0]) + (pCircuit_V[1] * ~(I_Out[1])) + (pCircuit_V[2] * ~(I_Out[2])));
					}
					//calculate my current errors
					if((phases & 0x10) == 0x10) {
						curr_state.Pout[0] = VA_Out.Re();
						curr_state.Qout[0] = VA_Out.Im();
						curr_state.ed[0] = ((~(complex(Pref, Qref)/(*pCircuit_V))) - (~(complex(curr_state.Pout[0],curr_state.Qout[0])/(*pCircuit_V)))).Re();
						curr_state.ded[0] = curr_state.ed[0] / deltat;
						curr_state.dmd[0] = (kpd * curr_state.ded[0]) + (kid * curr_state.ed[0]);
						curr_state.eq[0] = ((~(complex(Pref, Qref)/(*pCircuit_V))) - (~(complex(curr_state.Pout[0],curr_state.Qout[0])/(*pCircuit_V)))).Im();
						curr_state.deq[0] = curr_state.eq[0] / deltat;
						curr_state.dmq[0] = (kpq * curr_state.deq[0]) + (kiq * curr_state.eq[0]);
						if(fabs(curr_state.ded[0]) <= inverter_convergence_criterion && fabs(curr_state.deq[0]) <= inverter_convergence_criterion) {
							simmode_return_value =  SM_EVENT;// we have reached steady state
						} else {
							simmode_return_value =  SM_DELTA;
						}
					}
					if((phases & 0x07) == 0x07) {
						for(i = 0; i < 3; i++) {
							curr_state.Pout[i] = VA_Out.Re() / 3.0;
							curr_state.Qout[i] = VA_Out.Im() / 3.0;
							curr_state.ed[i] = ((~(complex(Pref/3.0, Qref/3.0)/(pCircuit_V[i]))) - (~(complex(curr_state.Pout[i],curr_state.Qout[i])/(pCircuit_V[i])))).Re();
							curr_state.ded[i] = curr_state.ed[i] / deltat;
							curr_state.dmd[i] = (kpd * curr_state.ded[i]) + (kid * curr_state.ed[i]);
							curr_state.eq[i] = ((~(complex(Pref/3.0, Qref/3.0)/(pCircuit_V[i]))) - (~(complex(curr_state.Pout[i],curr_state.Qout[i])/(pCircuit_V[i])))).Im();
							curr_state.deq[i] = curr_state.eq[i] / deltat;
							curr_state.dmq[i] = (kpq * curr_state.deq[i]) + (kiq * curr_state.eq[i]);
							if(fabs(curr_state.ded[i]) <= inverter_convergence_criterion && fabs(curr_state.deq[i]) <= inverter_convergence_criterion && simmode_return_value != SM_DELTA) {
								simmode_return_value = SM_EVENT;// we have reached steady state
							} else {
								simmode_return_value = SM_DELTA;
							}
						}
					}
				}
			} else if(iteration_count_val == 0) {
				//Calculate the predictor state from the previous current state
				if((phases & 0x10) == 0x10) {
					pred_state.md[0] = curr_state.md[0] + (deltat * curr_state.dmd[0]);
					pred_state.Idq[0].SetReal(pred_state.md[0] * I_In.Re());
					pred_state.mq[0] = curr_state.mq[0] + (deltat * curr_state.dmq[0]);
					pred_state.Idq[0].SetImag(pred_state.mq[0] * I_In.Re());
					pred_state.Iac[0] = pred_state.Idq[0];
					pLine_unrotI[0] += I_Out[0];
					pLine_unrotI[0] += -pred_state.Iac[0];
				}
				if((phases & 0x07) == 0x07) {
					for(i = 0; i < 3; i++) {
						pred_state.md[i] = curr_state.md[i] + (deltat * curr_state.dmd[i]);
						pred_state.Idq[i].SetReal(pred_state.md[i] * I_In.Re());
						pred_state.mq[i] = curr_state.mq[i] + (deltat * curr_state.dmq[i]);
						pred_state.Idq[i].SetImag(pred_state.mq[i] * I_In.Re());
						pred_state.Iac[i] = pred_state.Idq[i];
						pLine_unrotI[i] += I_Out[i];
						pLine_unrotI[i] += -pred_state.Iac[i];
					}
				}

				//update the Pref and Qref values
				update_control_references();
				simmode_return_value =  SM_DELTA_ITER;
			} else if(iteration_count_val == 1) {
				//calculate the corrector state
				if ((phases & 0x10) == 0x10) {
					pred_state.Pout[0] = (*pCircuit_V * ~(pred_state.Iac[0])).Re();
					pred_state.Qout[0] = (*pCircuit_V * ~(pred_state.Iac[0])).Im();
					pred_state.ed[0] = ((~(complex(Pref, Qref)/(*pCircuit_V))) - (~(complex(pred_state.Pout[0],pred_state.Qout[0])/(*pCircuit_V)))).Re();
					pred_state.ded[0] = (pred_state.ed[0] - curr_state.ed[0]) / deltat;
					pred_state.dmd[0] = (kpd * pred_state.ded[0]) + (kid * pred_state.ed[0]);
					curr_state.md[0] = curr_state.md[0] + ((curr_state.dmd[0] + pred_state.dmd[0]) * deltat) / 2.0;
					curr_state.Idq[0].SetReal(curr_state.md[0] * I_In.Re());
					pred_state.eq[0] = ((~(complex(Pref, Qref)/(*pCircuit_V))) - (~(complex(pred_state.Pout[0],pred_state.Qout[0])/(*pCircuit_V)))).Im();
					pred_state.deq[0] = (pred_state.eq[0] - curr_state.eq[0]) / deltat;
					pred_state.dmq[0] = (kpq * pred_state.deq[0]) + (kiq * pred_state.eq[0]);
					curr_state.mq[0] = curr_state.mq[0] + ((curr_state.dmq[0] + pred_state.dmq[0]) * deltat) / 2.0;
					curr_state.Idq[0].SetImag(curr_state.mq[0] * I_In.Re());
					curr_state.Iac[0] = curr_state.Idq[0];
					pLine_unrotI[0] += pred_state.Iac[0];
					pLine_unrotI[0] += -curr_state.Iac[0];
					I_Out[0] = curr_state.Iac[0];
				}
				if((phases & 0x07) == 0x07) {
					for(i = 0; i < 3; i++) {
						pred_state.Pout[i] = (pCircuit_V[i] * ~(pred_state.Iac[i])).Re();
						pred_state.Qout[i] = (pCircuit_V[i] * ~(pred_state.Iac[i])).Im();
						pred_state.ed[i] = ((~(complex(Pref/3.0, Qref/3.0)/(pCircuit_V[i]))) - (~(complex(pred_state.Pout[i],pred_state.Qout[i])/(pCircuit_V[i])))).Re();
						pred_state.ded[i] = (pred_state.ed[i] - curr_state.ed[i]) / deltat;
						pred_state.dmd[i] = (kpd * pred_state.ded[i]) + (kid * pred_state.ed[i]);
						curr_state.md[i] = curr_state.md[i] + ((curr_state.dmd[i] + pred_state.dmd[i]) * deltat) / 2.0;
						curr_state.Idq[i].SetReal(curr_state.md[i] * I_In.Re());
						pred_state.eq[i] = ((~(complex(Pref/3.0, Qref/3.0)/(pCircuit_V[i]))) - (~(complex(pred_state.Pout[i],pred_state.Qout[i])/(pCircuit_V[i])))).Im();
						pred_state.deq[i] = (pred_state.eq[i] - curr_state.eq[i]) / deltat;
						pred_state.dmq[i] = (kpq * pred_state.deq[i]) + (kiq * pred_state.eq[i]);
						curr_state.mq[i] = curr_state.mq[i] + ((curr_state.dmq[i] + pred_state.dmq[i]) * deltat) / 2.0;
						curr_state.Idq[i].SetImag(curr_state.mq[i] * I_In.Re());
						curr_state.Iac[i] = curr_state.Idq[i];
						pLine_unrotI[i] += pred_state.Iac[i];
						pLine_unrotI[i] += -curr_state.Iac[i];
						I_Out[i] = curr_state.Iac[i];
					}
				}
				simmode_return_value =  SM_DELTA_ITER;
			} else if (iteration_count_val == 2) {
				//calculate the corrector errors
				if((phases & 0x10) == 0x10) {
					VA_Out = *pCircuit_V * ~(I_Out[0]);
					prev_error = curr_state.ed[0];
					curr_state.Pout[0] = VA_Out.Re();
					curr_state.Qout[0] = VA_Out.Im();
					curr_state.ed[0] = ((~(complex(Pref, Qref)/(*pCircuit_V))) - (~(complex(curr_state.Pout[0],curr_state.Qout[0])/(*pCircuit_V)))).Re();
					curr_state.ded[0] = (curr_state.ed[0] - prev_error) / deltat;
					curr_state.dmd[0] = (kpd * curr_state.ded[0]) + (kid * curr_state.ed[0]);

					prev_error = curr_state.eq[0];
					curr_state.eq[0] = ((~(complex(Pref, Qref)/(*pCircuit_V))) - (~(complex(curr_state.Pout[0],curr_state.Qout[0])/(*pCircuit_V)))).Im();
					curr_state.deq[0] = (curr_state.eq[0] - prev_error) / deltat;
					curr_state.dmq[0] = (kpq * curr_state.deq[0]) + (kiq * curr_state.eq[0]);
					if(fabs(curr_state.ded[0]) <= inverter_convergence_criterion && fabs(curr_state.deq[0]) <= inverter_convergence_criterion) {
						simmode_return_value =  SM_EVENT;// we have reached steady state
					} else {
						simmode_return_value =  SM_DELTA;
					}
				} else if ((phases & 0x07) == 0x07) {
					VA_Out = (pCircuit_V[0] * ~(I_Out[0])) + (pCircuit_V[1] * ~(I_Out[1])) + (pCircuit_V[2] * ~(I_Out[2]));
					for(i = 0; i < 3; i++) {
						curr_state.Pout[i] = VA_Out.Re() / 3.0;
						curr_state.Qout[i] = VA_Out.Im() / 3.0;
						prev_error = curr_state.ed[i];
						curr_state.ed[i] = ((~(complex(Pref/3.0, Qref/3.0)/(pCircuit_V[i]))) - (~(complex(curr_state.Pout[i],curr_state.Qout[i])/(pCircuit_V[i])))).Re();
						curr_state.ded[i] = (curr_state.ed[i] - prev_error) / deltat;
						curr_state.dmd[i] = (kpd * curr_state.ded[i]) + (kid * curr_state.ed[i]);
						prev_error = curr_state.eq[i];
						curr_state.eq[i] = ((~(complex(Pref/3.0, Qref/3.0)/(pCircuit_V[i]))) - (~(complex(curr_state.Pout[i],curr_state.Qout[i])/(pCircuit_V[i])))).Im();
						curr_state.deq[i] = (curr_state.eq[i] - prev_error) / deltat;
						curr_state.dmq[i] = (kpq * curr_state.deq[i]) + (kiq * curr_state.eq[i]);
						if(fabs(curr_state.ded[i]) <= inverter_convergence_criterion && fabs(curr_state.deq[i]) <= inverter_convergence_criterion && simmode_return_value != SM_DELTA) {
							simmode_return_value = SM_EVENT;// we have reached steady state
						} else {
							simmode_return_value = SM_DELTA;
						}
					}
				}
			} else if(iteration_count_val > 2) {
				for(i = 0; i < 3; i++) {
					if(fabs(curr_state.ded[i]) <= inverter_convergence_criterion && fabs(curr_state.deq[i]) <= inverter_convergence_criterion && simmode_return_value != SM_DELTA) {
						simmode_return_value = SM_EVENT;// we have reached steady state
					} else {
						simmode_return_value = SM_DELTA;
					}
				}
			}
			else
			{
				simmode_return_value =  SM_EVENT;
			}
		}
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
				curr_PID_state.I_in = I_In.Re();		//Input current

				//New timestep - also update the control references
				update_control_references();
			}
			//Default else - other loop

			//Reset the error
			curr_PID_state.max_error_val = 0.0;

			//Construct the power variable
			work_power_vals = complex(curr_PID_state.phase_Pref,curr_PID_state.phase_Qref);

			//Determine our path to update
			if ((phases & 0x10) == 0x10)	//Triplex
			{
				//Determine the current set point - unrotated
				curr_PID_state.current_set_raw[0] = ~(work_power_vals / pCircuit_V[0]);

				//Find the current angle
				curr_PID_state.reference_angle[0] = pCircuit_V[0].Arg();

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
				pid_out[0] = complex(temp_val_d,temp_val_q);

				//Adjust the modulation factor
				curr_PID_state.mod_vals[0] = prev_PID_state.mod_vals[0] + pid_out[0];

				//Compute the new current out - in the reference frame
				curr_PID_state.current_vals_ref[0] = curr_PID_state.mod_vals[0] * curr_PID_state.I_in;
				
				//Now unrotate
				curr_PID_state.current_vals[0] = complex(-1.0,0.0) * curr_PID_state.current_vals_ref[0] * complex_exp(curr_PID_state.reference_angle[0]);

				//Update the posting
				pLine_unrotI[0] += -last_current[3] + curr_PID_state.current_vals[0];

				//Update other variable
				last_current[3] = curr_PID_state.current_vals[0];

				//Compute the error
				curr_PID_state.max_error_val = curr_PID_state.error[0].Mag();

			}//End Triplex
			else	//Three-phase
			{
				//Compute the current current values
				for (indexval=0; indexval<3; indexval++)
				{
					//Determine the current set point - unrotated
					curr_PID_state.current_set_raw[indexval] = ~(work_power_vals / pCircuit_V[indexval]);

					//Find the current angle
					curr_PID_state.reference_angle[indexval] = pCircuit_V[indexval].Arg();

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
					pid_out[indexval] = complex(temp_val_d,temp_val_q);

					//Adjust the modulation factor
					curr_PID_state.mod_vals[indexval] = prev_PID_state.mod_vals[indexval] + pid_out[indexval];

					//Compute the new current out - in the reference frame
					curr_PID_state.current_vals_ref[indexval] = curr_PID_state.mod_vals[indexval] * curr_PID_state.I_in;
					
					//Now unrotate
					curr_PID_state.current_vals[indexval] = complex(-1.0,0.0) * curr_PID_state.current_vals_ref[indexval] * complex_exp(curr_PID_state.reference_angle[indexval]);

					//Update the posting
					pLine_unrotI[indexval] += -last_current[indexval] + curr_PID_state.current_vals[indexval];

					//Update other variable
					last_current[indexval] = curr_PID_state.current_vals[indexval];

					//Compare the error
					if (curr_PID_state.error[indexval].Mag() > curr_PID_state.max_error_val)
					{
						curr_PID_state.max_error_val = curr_PID_state.error[indexval].Mag();
					}
				}
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
			if((phases & 0x10) == 0x10){
				pLine_unrotI[0] += I_Out[0];
				
				//Zero the output trackers
				I_Out[0] = complex(0.0,0.0);
			} else if((phases & 0x07) == 0x07) {
				pLine_unrotI[0] += I_Out[0];
				pLine_unrotI[1] += I_Out[1];
				pLine_unrotI[2] += I_Out[2];

				//Zero the output trackers
				I_Out[0] = I_Out[1] = I_Out[2] = complex(0.0,0.0);
			}
		}
		else if (inverter_dyn_mode == PID_CONTROLLER)
		{
			if((phases & 0x10) == 0x10)
			{
				pLine_unrotI[0] -= last_current[3];

				//Zero the output tracker
				last_current[3] = complex(0.0,0.0);
			}
			else if((phases & 0x07) == 0x07)
			{
				pLine_unrotI[0] -= last_current[0];
				pLine_unrotI[1] -= last_current[1];
				pLine_unrotI[2] -= last_current[2];

				//Zero the output trackers
				last_current[0] = last_current[1] = last_current[2] = complex(0.0,0.0);
			}
		}
		//Default else, who knows

		//We're disabled - unless something else wants deltamode, we're done
		simmode_return_value = SM_EVENT;
	}//End Inverter disabled

	//Perform a check to determine how to go forward
	if (enable_1547_compliance == true)
	{
		//See if our return is value
		if ((ieee_1547_double > 0.0) && (ieee_1547_double < 1.7) && (simmode_return_value == SM_EVENT))
		{
			//Force us to stay
			return SM_DELTA;
		}
		else	//Just return whatever we were going to do
		{
			return simmode_return_value;
		}
	}
	else	//Normal mode
	{
		return simmode_return_value;
	}
}


//Module-level post update call

STATUS inverter::post_deltaupdate(complex *useful_value, unsigned int mode_pass)
{
	if (inverter_dyn_mode == PI_CONTROLLER)
	{
		if((phases & 0x10) == 0x10){
			pLine_unrotI[0] += I_Out[0];
		} else if((phases & 0x07) == 0x07) {
			pLine_unrotI[0] += I_Out[0];
			pLine_unrotI[1] += I_Out[1];
			pLine_unrotI[2] += I_Out[2];
		}
	}
	else if (inverter_dyn_mode == PID_CONTROLLER)
	{
		if((phases & 0x10) == 0x10){
			pLine_unrotI[0] -= last_current[3];
		} else if((phases & 0x07) == 0x07) {
			pLine_unrotI[0] -= last_current[0];
			pLine_unrotI[1] -= last_current[1];
			pLine_unrotI[2] -= last_current[2];
		}
	}
	//Default else, who knows

	return SUCCESS;	//Always succeeds right now
}

//Initializes dynamic equations for first entry
//Returns a SUCCESS/FAIL
//curr_time is the initial states/information
STATUS inverter::init_PI_dynamics(INV_STATE *curr_time)
{
	complex prev_Idq[3];
	//Find the initial state of the inverter
	//Find the initial error from a steady state modulation value
	if((phases & 0x10) == 0x10) { //Single Phase
		Pref = VA_Out.Re();
		prev_Idq[0] = last_I_Out[0];

		if(last_I_In > 1e-9) {
			curr_time->md[0] = prev_Idq[0].Re()/last_I_In;
		} else {
			curr_time->md[0] = 0.0;
		}
		curr_time->Idq[0].SetReal(curr_time->md[0] * I_In.Re());

		Qref = VA_Out.Im();
		if(last_I_In > 1e-9) {
			curr_time->mq[0] = prev_Idq[0].Im()/last_I_In;
		} else {
			curr_time->mq[0] = 0.0;
		}
		curr_time->Idq[0].SetImag(curr_time->mq[0] * I_In.Re());
		curr_time->Iac[0] = curr_time->Idq[0];

		//Post the value
		pLine_unrotI[0] += -curr_time->Iac[0];

	} else if((phases & 0x07) == 0x07) { // Three Phase
		Pref = VA_Out.Re();
		Qref = VA_Out.Im();
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
			curr_time->Idq[i].SetReal(curr_time->md[i] * I_In.Re());
			curr_time->Idq[i].SetImag(curr_time->mq[i] * I_In.Re());
			curr_time->Iac[i] = curr_time->Idq[i];

			pLine_unrotI[i] += -curr_time->Iac[i];
		}
	}

	return SUCCESS;	//Always succeeds for now, but could have error checks later
}

//Initializes dynamic equations for first entry
//Returns a SUCCESS/FAIL
//PID-controller implementation
STATUS inverter::init_PID_dynamics(void)
{
	int indexx;
	
	//Copy over the set-points -- this may need to be adjusted in the future
	Pref = VA_Out.Re();
	Qref = VA_Out.Im();

	//Copy the current input
	curr_PID_state.I_in = I_In.Re();

	//Check the phases to see how to populate
	if ( (phases & 0x10) == 0x10 ) // split phase
	{
		//Copy in current set point
		curr_PID_state.phase_Pref = VA_Out.Re();
		curr_PID_state.phase_Qref = VA_Out.Im();
	
		//Zero items and compute current output and modulation index
		for (indexx=0; indexx<3; indexx++)
		{
			//Zero everything, just because
			curr_PID_state.reference_angle[indexx] = 0.0;
			curr_PID_state.error[indexx] = complex(0.0,0.0);
			curr_PID_state.integrator_vals[indexx] = complex(0.0,0.0);
			curr_PID_state.derror[indexx] = complex(0.0,0.0);
			curr_PID_state.current_set_raw[indexx] = complex(0.0,0.0);
			curr_PID_state.current_set[indexx] = complex(0.0,0.0);
			curr_PID_state.current_vals[indexx] = complex(0.0,0.0);
			curr_PID_state.current_vals_ref[indexx] = complex(0.0,0.0);
			curr_PID_state.mod_vals[indexx] = complex(0.0,0.0);
		}
			
		//Populate the initial "reference angle"
		curr_PID_state.reference_angle[0] = pCircuit_V[0].Arg();

		//Calculate the current set-point -- should be the same
		curr_PID_state.current_set_raw[0] = ~(complex(curr_PID_state.phase_Pref,curr_PID_state.phase_Qref)/pCircuit_V[0]);

		//Rotate it
		curr_PID_state.current_set[0] = curr_PID_state.current_set_raw[0] * complex_exp(-1.0 * curr_PID_state.reference_angle[0]);
		
		//Copy in current that was posted
		curr_PID_state.current_vals[0] = last_current[3];

		//For completion, rotate this into this reference frame
		curr_PID_state.current_vals_ref[0] = complex(-1.0,0.0) * curr_PID_state.current_vals[0] * complex_exp(-1.0 * curr_PID_state.reference_angle[0]);

		//Compute base modulation value - these are in the reference frame
		curr_PID_state.mod_vals[0] = complex((curr_PID_state.current_vals_ref[0].Re() / curr_PID_state.I_in),(curr_PID_state.current_vals_ref[0].Im() / curr_PID_state.I_in));

		//Add in the last current too - PostSync removed it, so this will fix it for the logic in interupdate
		pLine_unrotI[0] += last_current[3];
	
	}//End triplex-connected
	else
	{
		//Copy in current set point
		curr_PID_state.phase_Pref = VA_Out.Re() / 3.0;
		curr_PID_state.phase_Qref = VA_Out.Im() / 3.0;

		//Zero items and compute current output and modulation index
		for (indexx=0; indexx<3; indexx++)
		{
			//Zero current - prev will get done by loop routine
			curr_PID_state.error[indexx] = complex(0.0,0.0);
			curr_PID_state.integrator_vals[indexx] = complex(0.0,0.0);
			curr_PID_state.derror[indexx] = complex(0.0,0.0);
			
			//Populate the initial "reference angle"
			curr_PID_state.reference_angle[indexx] = pCircuit_V[indexx].Arg();

			//Calculate the current set-point -- should be the same
			curr_PID_state.current_set_raw[indexx] = ~(complex(curr_PID_state.phase_Pref,curr_PID_state.phase_Qref)/pCircuit_V[indexx]);

			//Rotate it
			curr_PID_state.current_set[indexx] = curr_PID_state.current_set_raw[indexx] * complex_exp(-1.0 * curr_PID_state.reference_angle[indexx]);
			
			//Copy in current that was posted
			curr_PID_state.current_vals[indexx] = last_current[indexx];

			//For completion, rotate this into this reference frame
			curr_PID_state.current_vals_ref[indexx] = complex(-1.0,0.0) * curr_PID_state.current_vals[indexx] * complex_exp(-1.0 * curr_PID_state.reference_angle[indexx]);

			//Compute base modulation value - these are in the reference frame
			curr_PID_state.mod_vals[indexx] = complex((curr_PID_state.current_vals_ref[indexx].Re() / curr_PID_state.I_in),(curr_PID_state.current_vals_ref[indexx].Im() / curr_PID_state.I_in));

			//Add in the last current too - PostSync removed it, so this will fix it for the logic in interupdate
			pLine_unrotI[indexx] += last_current[indexx];
		}
	}

	return SUCCESS;	//Always succeeds for now, but could have error checks later
}


void inverter::update_control_references(void)
{
	//FOUR_QUADRANT model (originally written for NAS/CES, altered for PV)
	double VA_Efficiency, temp_PF, temp_QVal;
	complex temp_VA, VA_Outref;
	complex battery_power_out = complex(0,0);
	OBJECT *obj = OBJECTHDR(this);

	//Compute power in - supposedly DC, but since it's complex, we'll be proper (other models may need fixing)
	VA_In = V_In * ~ I_In;

	//Determine how to efficiency weight it
	if(use_multipoint_efficiency == false)
	{
		//Normal scaling
		VA_Efficiency = VA_In.Re() * efficiency * internal_losses * frequency_losses;
	}
	else
	{
		//See if above minimum DC power input
		if(VA_In.Mag() <= p_so)
		{
			VA_Efficiency = 0.0;	//Nope, no output
		}
		else	//Yes, apply effiency change
		{
			//Make sure voltage isn't too low
			if(V_In.Mag() > v_dco)
			{
				gl_warning("The dc voltage is greater than the specified maximum for the inverter. Efficiency model may be inaccurate.");
				/*  TROUBLESHOOT
				The DC voltage at the input to the inverter is less than the maximum voltage supported by the inverter.  As a result, the
				multipoint efficiency model may not provide a proper result.
				*/
			}

			//Compute coefficients for multipoint efficiency
			C1 = p_dco*(1+c_1*(V_In.Re()-v_dco));
			C2 = p_so*(1+c_2*(V_In.Re()-v_dco));
			C3 = c_o*(1+c_3*(V_In.Re()-v_dco));

			//Apply this to the output
			VA_Efficiency = (((p_max/(C1-C2))-C3*(C1-C2))*(VA_In.Re()-C2)+C3*(VA_In.Re()-C2)*(VA_In.Re()-C2))*internal_losses*frequency_losses;
		}
	}

	//Determine 4 quadrant outputs
	if(four_quadrant_control_mode == FQM_CONSTANT_PF)	//Power factor mode
	{
		if(power_factor != 0.0)	//Not purely imaginary
		{
			if (VA_In<0.0)	//Discharge at input, so must be "load"
			{
				//Total power output is the magnitude
				VA_Outref.SetReal(VA_Efficiency*-1.0);
			}
			else if (VA_In>0.0)	//Positive input, so must be generator
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
			VA_Outref = complex(0.0,VA_Efficiency);
		}
	}
	else if (four_quadrant_control_mode == FQM_CONSTANT_PQ)
	{
		//Compute desired output - sign convention appears to be backwards
		temp_VA = complex(P_Out,Q_Out);


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
					VA_Outref = complex(temp_VA.Re(),-temp_QVal);
				}
				else	//Positive Q dispatch
				{
					VA_Outref = complex(temp_VA.Re(),temp_QVal);
				}
			}
			else	//Inverter rated power is equal to or smaller than real power desired, give it all we can
			{
				//Maintain desired sign convention
				if (temp_VA.Re() < 0.0)
				{
					VA_Outref = complex(-p_max,0.0);
				}
				else	//Positive
				{
					VA_Outref = complex(p_max,0.0);
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
		//Determine the excess, for use elsewhere - back out simple efficiencies
		excess_input_power = (VA_Outref.Mag() - p_max)/(internal_losses*frequency_losses);

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
		temp_VA = complex(fabs(p_max*temp_PF),fabs(p_max*sqrt(1.0-(temp_PF*temp_PF))));

		//"Sign" it appropriately
		if ((VA_Outref.Re()<0) && (VA_Outref.Im()<0))	//-R, -I
		{
			VA_Outref = -temp_VA;
		}
		else if ((VA_Outref.Re()<0) && (VA_Outref.Im()>=0))	//-R,I
		{
			VA_Outref = complex(-temp_VA.Re(),temp_VA.Im());
		}
		else if ((VA_Outref.Re()>=0) && (VA_Outref.Im()<0))	//R,-I
		{
			VA_Outref = complex(temp_VA.Re(),-temp_VA.Im());
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
}

//Functionalized IEEE 1547 initalizer - mostly so it can easily be done in either deltamode or ss
STATUS inverter::initalize_IEEE_1547_checks(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	double *temp_nominal_pointer;

	//Check parents and map the variables
	if (gl_object_isa(parent,"node","powerflow") || gl_object_isa(parent,"load","powerflow") || gl_object_isa(parent,"meter","powerflow") || gl_object_isa(parent,"triplex_node","powerflow") || gl_object_isa(parent,"triplex_load","powerflow") || gl_object_isa(parent,"triplex_load","powerflow"))
	{
		//Find the propery of the frequency measurements
		freq_pointer = get_double(parent,"measured_frequency");

		//Make sure it worked
		if (freq_pointer == NULL)
		{
			gl_error("Inverter:%d %s failed to map the measured_frequency property",obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to map the measured_frequency property, an error occurred.  Please try again.
			If the error persists, please submit your GLM and a bug report to the ticketing system.
			*/

			return FAILED;
		}
		//Default else, it worked

		//Link to nominal voltage
		temp_nominal_pointer = get_double(parent,"nominal_voltage");

		//Make sure it worked
		if (temp_nominal_pointer == NULL)
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
		node_nominal_voltage = *temp_nominal_pointer;

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

			//Set the voltage values as well - basically, the under votlage has an extra category
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
		//Make sure nulled and disable 1547, since it won't do anything
		freq_pointer = NULL;
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

	//Perform frequency check - overlapping bands set so we don't care about size anyore
	if ((*freq_pointer > over_freq_low_band_setpoint) || (*freq_pointer < under_freq_high_band_setpoint))
	{
		//Flag it
		frequency_violation = true;

		//Reset "restoration" time
		out_of_violation_time_total = 0.0;

		//Figure out which range we are
		if (*freq_pointer > over_freq_high_band_setpoint)
		{
			//Accumulate the over frequency timers (all for this case)
			over_freq_high_band_viol_time += timestepvalue;
			over_freq_low_band_viol_time += timestepvalue;

			//Zero the others, in case we did a huge jump
			under_freq_high_band_viol_time = 0.0;
			under_freq_low_band_viol_time = 0.0;

			if (over_freq_high_band_viol_time >= over_freq_high_band_delay)
			{
				trigger_disconnect = true;
				return_time_freq = reconnect_time;
			}
			else if (over_freq_low_band_viol_time >= over_freq_low_band_delay)	//Triggered existing band
			{
				trigger_disconnect = true;
				return_time_freq = reconnect_time;
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
		else if (*freq_pointer < under_freq_low_band_setpoint)
		{
			//Accumulate both under frequency timers (all violated)
			under_freq_high_band_viol_time += timestepvalue;
			under_freq_low_band_viol_time += timestepvalue;

			//Zero the others, in case we did a huge jump
			over_freq_high_band_viol_time = 0.0;
			over_freq_low_band_viol_time = 0.0;

			if (under_freq_low_band_viol_time >= under_freq_low_band_delay)
			{
				trigger_disconnect = true;
				return_time_freq = reconnect_time;
			}
			else if (under_freq_high_band_viol_time >= under_freq_high_band_delay)	//Other band trigger
			{
				trigger_disconnect = true;
				return_time_freq = reconnect_time;
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
		else if ((*freq_pointer < under_freq_high_band_setpoint) && (*freq_pointer >= under_freq_low_band_setpoint))
		{
			//Just update the high violation time
			under_freq_high_band_viol_time += timestepvalue;

			//Zero the other one, for good measure
			under_freq_low_band_viol_time = 0.0;

			//Zero the others, in case we did a huge jump
			over_freq_high_band_viol_time = 0.0;
			over_freq_low_band_viol_time = 0.0;

			if (under_freq_high_band_viol_time >= under_freq_high_band_delay)
			{
				trigger_disconnect = true;
				return_time_freq = reconnect_time;
			}
			else
			{
				trigger_disconnect = false;
				return_time_freq = under_freq_high_band_delay - under_freq_high_band_viol_time;
			}
		}
		else if ((*freq_pointer <= over_freq_high_band_setpoint) && (*freq_pointer > over_freq_low_band_setpoint))
		{
			//Just update the "high-low" violation time
			over_freq_low_band_viol_time += timestepvalue;

			//Zero the other one, for good measure
			over_freq_high_band_viol_time = 0.0;

			//Zero the others, in case we did a huge jump
			under_freq_high_band_viol_time = 0.0;
			under_freq_low_band_viol_time = 0.0;

			if (over_freq_low_band_viol_time >= over_freq_low_band_delay)
			{
				trigger_disconnect = true;
				return_time_freq = reconnect_time;
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
			if (indexval < 2)
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
			temp_pu_voltage = pCircuit_V[indexval].Mag()/node_nominal_voltage;

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
					if (uv_low_hit == false)
					{
						under_voltage_lowest_viol_time += timestepvalue;
						uv_low_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else if ((temp_pu_voltage >= under_voltage_lowest_voltage_setpoint) && (temp_pu_voltage < under_voltage_middle_voltage_setpoint))
				{

					//See if we've accumulated yet
					if (uv_mid_hit == false)
					{
						under_voltage_middle_viol_time += timestepvalue;
						uv_mid_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else if ((temp_pu_voltage >= under_voltage_middle_voltage_setpoint) && (temp_pu_voltage < under_voltage_high_voltage_setpoint))
				{
					//See if we've accumulated yet
					if (uv_high_hit == false)
					{
						under_voltage_high_viol_time += timestepvalue;
						uv_high_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else if ((temp_pu_voltage > over_voltage_low_setpoint) && (temp_pu_voltage < over_voltage_high_setpoint))
				{
					//See if we've accumulated yet
					if (ov_low_hit == false)
					{
						over_voltage_low_viol_time += timestepvalue;
						ov_low_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else if (temp_pu_voltage >= over_voltage_high_setpoint)
				{
					//See if we've accumulated yet
					if (ov_high_hit == false)
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
	if (voltage_violation == true)
	{
		//Reconcile the violation times and see how we need to break
		if (uv_low_hit == true)
		{
			if (under_voltage_lowest_viol_time >= under_voltage_lowest_delay)
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;
			}
			else if (under_voltage_middle_viol_time >= under_voltage_middle_delay)	//Check other ranges
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;
			}

			else if (under_voltage_high_viol_time >= under_voltage_high_delay)
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = under_voltage_lowest_delay - under_voltage_lowest_viol_time;
			}
		}
		else if (uv_mid_hit == true)
		{
			if (under_voltage_middle_viol_time >= under_voltage_middle_delay)
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;
			}
			else if (under_voltage_high_viol_time >= under_voltage_high_delay)	//Check higher bands
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = under_voltage_middle_delay - under_voltage_middle_viol_time;
			}
		}
		else if (uv_high_hit == true)
		{
			if (under_voltage_high_viol_time >= under_voltage_high_delay)
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = under_voltage_high_delay - under_voltage_high_viol_time;
			}
		}
		else if (ov_low_hit == true)
		{
			if (over_voltage_low_viol_time >= over_voltage_low_delay)
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = over_voltage_low_delay - over_voltage_low_viol_time;
			}
		}
		else if (ov_high_hit == true)
		{
			if (over_voltage_high_viol_time >= over_voltage_high_delay)
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;
			}
			else if (over_voltage_low_viol_time >= over_voltage_low_delay)	//Lower band overlap
			{
				trigger_disconnect = true;
				return_time_volt = reconnect_time;
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
	if ((frequency_violation == true) || (voltage_violation == true))
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
	if (inverter_1547_status == false)
	{
		if (out_of_violation_time_total >= reconnect_time)
		{
			//Set us back into service
			inverter_1547_status = true;

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
		if (trigger_disconnect == true)
		{
			inverter_1547_status = false;	//Trigger

			//Return our expected next status interval
			return return_value;
		}
		else
		{
			//All is well, indicate as much
			return return_value;
		}
	}
}

//Function to perform exp(j*val)
//Basically a complex rotation
complex inverter::complex_exp(double angle)
{
	complex output_val;

	//exp(jx) = cos(x)+j*sin(x)
	output_val = complex(cos(angle),sin(angle));

	return output_val;
}

//Retrieves the pointer for a double variable from another object
double *inverter::get_double(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_double)
		return NULL;
	return (double*)GETADDR(obj,p);
}
bool *inverter::get_bool(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_bool)
		return NULL;
	return (bool*)GETADDR(obj,p);
}
int *inverter::get_enum(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_enumeration)
		return NULL;
	return (int*)GETADDR(obj,p);
}
complex * inverter::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}

double inverter::getVar(double volt, double m, double b)
{
	double Q = 0;
	Q = (m * volt) + b;
	return Q;
}
	
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_inverter(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(inverter::oclass);
		if (*obj!=NULL)
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
		if (obj!=NULL)
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

EXPORT STATUS postupdate_inverter(OBJECT *obj, complex *useful_value, unsigned int mode_pass)
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
