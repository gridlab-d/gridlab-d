#include "ibr_gfl.h"

CLASS *ibr_gfl::oclass = nullptr;
ibr_gfl *ibr_gfl::defaults = nullptr;

static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
ibr_gfl::ibr_gfl(MODULE *module)
{
	if (oclass == nullptr)
	{
		oclass = gl_register_class(module, "ibr_gfl", sizeof(ibr_gfl), PC_PRETOPDOWN | PC_BOTTOMUP | PC_POSTTOPDOWN | PC_AUTOLOCK);
		if (oclass == nullptr)
			throw "unable to register class ibr_gfl";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,

			//Input
			PT_double, "rated_power[VA]", PADDR(S_base), PT_DESCRIPTION, " The rated power of the inverter",
			PT_double, "Neg_Curr_Ctrl", PADDR(Neg_Curr_Ctrl), PT_DESCRIPTION, "1: Negative sequence control enabled; 0: Negative sequence control disabled",

			PT_complex, "phaseA_V[V]", PADDR(value_Circuit_V[0]), PT_DESCRIPTION, "AC voltage on A phase in three-phase system",
			PT_complex, "phaseB_V[V]", PADDR(value_Circuit_V[1]), PT_DESCRIPTION, "AC voltage on A phase in three-phase system",
			PT_complex, "phaseC_V[V]", PADDR(value_Circuit_V[2]), PT_DESCRIPTION, "AC voltage on A phase in three-phase system",

			PT_complex, "phaseA_I_Out[A]", PADDR(terminal_current_val[0]), PT_DESCRIPTION, "AC current on A phase in three-phase system",
			PT_complex, "phaseB_I_Out[A]", PADDR(terminal_current_val[1]), PT_DESCRIPTION, "AC current on B phase in three-phase system",
			PT_complex, "phaseC_I_Out[A]", PADDR(terminal_current_val[2]), PT_DESCRIPTION, "AC current on C phase in three-phase system",
			PT_complex, "phaseA_I_Out_PU[pu]", PADDR(terminal_current_val_pu[0]), PT_DESCRIPTION, "AC current on A phase in three-phase system, pu",
			PT_complex, "phaseB_I_Out_PU[pu]", PADDR(terminal_current_val_pu[1]), PT_DESCRIPTION, "AC current on B phase in three-phase system, pu",
			PT_complex, "phaseC_I_Out_PU[pu]", PADDR(terminal_current_val_pu[2]), PT_DESCRIPTION, "AC current on C phase in three-phase system, pu",

			PT_complex, "power_A[VA]", PADDR(power_val[0]), PT_DESCRIPTION, "AC power on A phase in three-phase system",
			PT_complex, "power_B[VA]", PADDR(power_val[1]), PT_DESCRIPTION, "AC power on B phase in three-phase system",
			PT_complex, "power_C[VA]", PADDR(power_val[2]), PT_DESCRIPTION, "AC power on C phase in three-phase system",
			PT_complex, "VA_Out[VA]", PADDR(VA_Out), PT_DESCRIPTION, "AC power",

			PT_complex, "terminal_current_val_PS", PADDR(terminal_current_val_PS), PT_DESCRIPTION, "positive sequence current magnitude",
			PT_complex, "VA_Out_PS[VA]", PADDR(VA_Out_PS), PT_DESCRIPTION, "positive sequence power",

			// Inverter filter parameters
			PT_double, "Xfilter[pu]", PADDR(Xfilter), PT_DESCRIPTION, "DELTAMODE:  per-unit values of inverter filter inductance.",
			PT_double, "Rfilter[pu]", PADDR(Rfilter), PT_DESCRIPTION, "DELTAMODE:  per-unit values of inverter filter resistance.",
			PT_double, "Xi_filter", PADDR(Xi_filter[0]), PT_DESCRIPTION, "DELTAMODE: Xi_filter",
			PT_double, "CFilt", PADDR(CFilt), PT_DESCRIPTION, "DELTAMODE: Capacitance",

			PT_double, "igq_hold_value", PADDR(igq_hold_value), PT_DESCRIPTION, "DELTAMODE:  used for faults",
			PT_double, "igq_hold_time", PADDR(igq_hold_time), PT_DESCRIPTION, "DELTAMODE:  used for faults",

			// Grid-Following Controller Parameters
			PT_double, "Pref[W]", PADDR(Pref), PT_DESCRIPTION, "DELTAMODE: The real power reference.",
			PT_double, "Qref[VAr]", PADDR(Qref), PT_DESCRIPTION, "DELTAMODE: The reactive power reference.",
			PT_double, "Qref_pu", PADDR(Qref_pu), PT_DESCRIPTION, "DELTAMODE: per-unit Q ref.",
			PT_double, "kpc", PADDR(kpc), PT_DESCRIPTION, "DELTAMODE: Proportional gain of the current loop.",
			PT_double, "kic", PADDR(kic), PT_DESCRIPTION, "DELTAMODE: Integral gain of the current loop.",
			PT_double, "F_current", PADDR(F_current), PT_DESCRIPTION, "DELTAMODE: feed forward term gain in current loop.",
			PT_double, "Tif", PADDR(Tif), PT_DESCRIPTION, "DELTAMODE: time constant of first-order low-pass filter of current loop when using current source representation.",
			PT_double, "Tif_igd_ref", PADDR(Tif_igd_ref), PT_DESCRIPTION, "DELTAMODE: time constant of first-order low-pass filter of Idref when using current source representation.",
			PT_double, "Tif_igq_ref", PADDR(Tif_igq_ref), PT_DESCRIPTION, "DELTAMODE: time constant of first-order low-pass filter of Iqref when using current source representation.",
			PT_double, "kqp", PADDR(kqp), PT_DESCRIPTION, "DELTAMODE: Proportional gain of the Q loop.",
			PT_double, "Tqi", PADDR(Tqi), PT_DESCRIPTION, "DELTAMODE: Time constant of the Q loop.",
			PT_double, "kvp", PADDR(kvp), PT_DESCRIPTION, "DELTAMODE: Proportional gain of the V loop.",
			PT_double, "Tvi", PADDR(Tvi), PT_DESCRIPTION, "DELTAMODE: Time constant of the V loop.",

			PT_double, "Test2", PADDR(Test2), PT_DESCRIPTION, "Test2",

			PT_double, "igd_cmd_A", PADDR(igd_cmd[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase A, current source representation",
			PT_double, "ipmax_A", PADDR(Ipmax[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase A, current source representation",
			PT_double, "ipUL_A", PADDR(IpUL[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase A, current source representation",

			PT_double, "igd_delay_flag", PADDR(igd_delay_flag), PT_DESCRIPTION, "DELTAMODE: igd_delay_flag",
			PT_double, "start_time_igd_delay", PADDR(start_time_igd_delay), PT_DESCRIPTION, "DELTAMODE: start_time_igd_delay",

			//LIMITS
			PT_double, "Smax", PADDR(Smax), PT_DESCRIPTION, "Smax",
			PT_double, "I_base", PADDR(I_base), PT_DESCRIPTION, "I_base",
			PT_double, "igq_ref_A", PADDR(igq_ref[0]), PT_DESCRIPTION, "phase A q axis current reference",
			PT_double, "ug_pu_PS1", PADDR(ug_pu_PS1), PT_DESCRIPTION, "DELTAMODE: ug magnitude.",
			PT_double, "Pref_max[pu]", PADDR(Pref_max), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of power references in grid-following mode.",
			PT_double, "Pref_min[pu]", PADDR(Pref_min), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of power references in grid-following mode.",
			PT_double, "Qref_max[pu]", PADDR(Qref_max), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of reactive power references in grid-following mode.",
			PT_double, "Qref_min[pu]", PADDR(Qref_min), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of reactive power references in grid-following mode.",
			PT_double, "igd_ref_max", PADDR(igd_ref_max), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Upper limit of igd_ref of grid-following inverter, d-axis",
			PT_double, "igd_ref_min", PADDR(igd_ref_min), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Lower limit of igd_ref of grid-following inverter, d-axis",
			PT_double, "igq_ref_max", PADDR(igq_ref_max), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Upper limit of igq_ref of grid-following inverter, q-axis",
			PT_double, "igq_ref_min", PADDR(igq_ref_min), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Lower limit of igq_ref of grid-following inverter, q-axis",
			PT_double, "Ipmax_limit", PADDR(Ipmax_limit), PT_DESCRIPTION, "DELTAMODE:  limit for positive current magnitude",

			PT_double, "ug_lvrt_threshold", PADDR(ug_lvrt_threshold), PT_DESCRIPTION, "DELTAMODE:  Voltage threshold to re-adjust Qref",
			PT_double, "ug_asym_neg_threshold", PADDR(ug_asym_neg_threshold), PT_DESCRIPTION, "DELTAMODE:  Voltage threshold to decide look-up table details",

			// Look-up table 1
			PT_double, "ug_asym_threshold1", PADDR(ug_asym_threshold1), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold1",
			PT_double, "Qref_asym_threshold1_lim", PADDR(Qref_asym_threshold1_lim), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold1 upper limit",
			PT_double, "ug_asym_threshold2", PADDR(ug_asym_threshold2), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold2",
			PT_double, "Qref_asym_threshold2_lim", PADDR(Qref_asym_threshold2_lim), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold2 upper limit",
			PT_double, "ug_asym_threshold3", PADDR(ug_asym_threshold3), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold3",
			PT_double, "Qref_asym_threshold3_k", PADDR(Qref_asym_threshold3_k), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold3 coefficient",
			PT_double, "Qref_asym_threshold3_b", PADDR(Qref_asym_threshold3_b), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold3 offset",
			PT_double, "ug_asym_threshold4", PADDR(ug_asym_threshold4), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold4",
			PT_double, "Qref_asym_threshold4_k", PADDR(Qref_asym_threshold4_k), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold4 coefficient",
			PT_double, "Qref_asym_threshold4_b", PADDR(Qref_asym_threshold4_b), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold4 offset",
			PT_double, "ug_asym_threshold5", PADDR(ug_asym_threshold5), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold5",
			PT_double, "Qref_asym_threshold5_k", PADDR(Qref_asym_threshold5_k), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold5 coefficient",
			PT_double, "Qref_asym_threshold5_b", PADDR(Qref_asym_threshold5_b), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold5 offset",
			PT_double, "ug_asym_threshold6", PADDR(ug_asym_threshold6), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold6",
			PT_double, "Qref_asym_threshold6_k", PADDR(Qref_asym_threshold6_k), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold6 coefficient",
			PT_double, "Qref_asym_threshold6_b", PADDR(Qref_asym_threshold6_b), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold6 offset",
			PT_double, "Qref_asym_threshold7_lim", PADDR(Qref_asym_threshold7_lim), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: threshold7 lower limit",
			PT_double, "Qref_asym_UL", PADDR(Qref_asym_UL), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: overall Qref upper limit",
			PT_double, "Qref_asym_LL", PADDR(Qref_asym_LL), PT_DESCRIPTION, "DELTAMODE:  look-up table 1: overall Qref lower limit",
			// Look-up table 2
			PT_double, "ug_sym_threshold1", PADDR(ug_sym_threshold1), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold1",
			PT_double, "Qref_sym_threshold1_lim", PADDR(Qref_sym_threshold1_lim), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold1 upper limit",
			PT_double, "ug_sym_threshold2", PADDR(ug_sym_threshold2), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold2",
			PT_double, "Qref_sym_threshold2_k", PADDR(Qref_sym_threshold2_k), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold2 coefficient",
			PT_double, "Qref_sym_threshold2_b", PADDR(Qref_sym_threshold2_b), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold2 offset",
			PT_double, "ug_sym_threshold3", PADDR(ug_sym_threshold3), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold3",
			PT_double, "Qref_sym_threshold3_k", PADDR(Qref_sym_threshold3_k), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold3 coefficient",
			PT_double, "Qref_sym_threshold3_b", PADDR(Qref_sym_threshold3_b), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold3 offset",
			PT_double, "ug_sym_threshold4", PADDR(ug_sym_threshold4), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold4",
			PT_double, "Qref_sym_threshold4_k", PADDR(Qref_sym_threshold4_k), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold4 coefficient",
			PT_double, "Qref_sym_threshold4_b", PADDR(Qref_sym_threshold4_b), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold4 offset",
			PT_double, "ug_sym_threshold5", PADDR(ug_sym_threshold5), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold5",
			PT_double, "Qref_sym_threshold5_k", PADDR(Qref_sym_threshold5_k), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold5 coefficient",
			PT_double, "Qref_sym_threshold5_b", PADDR(Qref_sym_threshold5_b), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold5 offset",
			PT_double, "Qref_sym_threshold6_lim", PADDR(Qref_sym_threshold6_lim), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: threshold6 lower limit",
			PT_double, "Qref_sym_UL", PADDR(Qref_sym_UL), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: overall Qref upper limit",
			PT_double, "Qref_sym_LL", PADDR(Qref_sym_LL), PT_DESCRIPTION, "DELTAMODE:  look-up table 2: overall Qref lower limit",

			//
			PT_complex, "VA_Out_NS[VA]", PADDR(VA_Out_NS), PT_DESCRIPTION, "negative sequence power",
			PT_complex, "IGenerated_A", PADDR(value_IGenerated[0]), PT_DESCRIPTION, "DELTAMODE: IGenerated_A",
			PT_complex, "terminal_current_val_NS", PADDR(terminal_current_val_NS), PT_DESCRIPTION, "negative sequence current magnitude",
			PT_complex, "value_Circuit_V_PS", PADDR(value_Circuit_V_PS), PT_DESCRIPTION, "Positive sequence voltage",
			PT_complex, "value_Circuit_V_NS", PADDR(value_Circuit_V_NS), PT_DESCRIPTION, "Negative sequence voltage",
			PT_double, "I_source_NS_Re_A", PADDR(I_source_ns_Re[0]), PT_DESCRIPTION, "Negative sequence current, real axis",
			PT_double, "I_source_NS_Im_A", PADDR(I_source_ns_Im[0]), PT_DESCRIPTION, "Negative sequence current, imag. axis",
			PT_double, "igq_cmd_A", PADDR(igq_cmd[0]), PT_DESCRIPTION, "q axis current command",
			PT_double, "ig_mag_pu_NS", PADDR(ig_mag_pu_NS), PT_DESCRIPTION, "Negative sequence current with delays",
			PT_double, "igd_pu_PS", PADDR(igd_pu_PS), PT_DESCRIPTION, "DELTAMODE: igd_pu_PS.",
			PT_double, "igq_pu_PS", PADDR(igq_pu_PS), PT_DESCRIPTION, "DELTAMODE: igq_pu_PS.",
			PT_double, "ig_pu_PS", PADDR(ig_pu_PS), PT_DESCRIPTION, "DELTAMODE: ig_pu_PS.",
			PT_complex, "terminal_current_val_A", PADDR(terminal_current_val[0]), PT_DESCRIPTION, "DELTAMODE: current in Phase A",

			PT_double, "igq_ns_A_filter", PADDR(igq_ns_filter[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, q-axis, phase A",
			PT_double, "igq_ns_B_filter", PADDR(igq_ns_filter[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, q-axis, phase B",
			PT_double, "igq_ns_C_filter", PADDR(igq_ns_filter[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, q-axis, phase C",
			PT_double, "igd_ns_pu_A", PADDR(igd_ns_pu[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase A",
			PT_double, "P_o_pu", PADDR(P_o_pu), PT_DESCRIPTION, "Active power calculated in dq frame",
			PT_double, "Q_o_pu", PADDR(Q_o_pu), PT_DESCRIPTION, "Reactive power calculated in dq frame",
			PT_double, "Q_out_pu", PADDR(Q_out_pu), PT_DESCRIPTION, "DELTAMODE: Q_out_pu is the per-unit value of VA_Out.Im().",
			PT_double, "Q_out_pu1", PADDR(Q_out_pu1), PT_DESCRIPTION, "DELTAMODE: Q_out_pu is the per-unit value of VA_Out.Im().",
			PT_double, "ug_pu_PS_A", PADDR(ug_pu_PS[0]), PT_DESCRIPTION, "DELTAMODE: ug magnitude.",
			PT_double, "ugd_pu_NS", PADDR(ugd_pu_NS), PT_DESCRIPTION, "Negative sequence voltage",
			PT_double, "ugq_pu_NS", PADDR(ugq_pu_NS), PT_DESCRIPTION, "Negative sequence voltage",
			PT_double, "ug_pu_NS", PADDR(ug_pu_NS), PT_DESCRIPTION, "DELTAMODE: ug magnitude.",
			PT_double, "igd_pu_NS", PADDR(igd_pu_NS), PT_DESCRIPTION, "Negative sequence voltage",
			PT_double, "igq_pu_NS", PADDR(igq_pu_NS), PT_DESCRIPTION, "Negative sequence voltage",
			PT_double, "ig_pu_NS", PADDR(ig_pu_NS), PT_DESCRIPTION, "Negative sequence current",
			PT_double, "igd_ns_filter_A", PADDR(igd_ns_filter[0]), PT_DESCRIPTION, "Negative sequence current",
			PT_double, "igq_ns_filter_A", PADDR(igq_ns_filter[0]), PT_DESCRIPTION, "Negative sequence current",
			PT_double, "Vref_pu", PADDR(Vref_pu[0]), PT_DESCRIPTION, "DELTAMODE: per-unit V ref.",
			PT_double, "Qerr", PADDR(Qerr[0]), PT_DESCRIPTION, "DELTAMODE: Error btw Qref and Q_out_pu.",
			PT_double, "Verr", PADDR(Verr[0]), PT_DESCRIPTION, "DELTAMODE: Error btw Vref and ug_pu_PS[i].",
			PT_double, "Qcap", PADDR(Qcap[0]), PT_DESCRIPTION, "DELTAMODE: Capacitive power",

			//
			PT_double, "ugd_pu_A", PADDR(ugd_pu[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal voltage of grid-following inverter, d-axis, phase A",
			PT_double, "ugd_pu_B", PADDR(ugd_pu[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal voltage of grid-following inverter, d-axis, phase B",
			PT_double, "ugd_pu_C", PADDR(ugd_pu[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal voltage of grid-following inverter, d-axis, phase C",
			PT_double, "ugq_pu_A", PADDR(ugq_pu[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal voltage of grid-following inverter, q-axis, phase A",
			PT_double, "ugq_pu_B", PADDR(ugq_pu[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal voltage of grid-following inverter, q-axis, phase B",
			PT_double, "ugq_pu_C", PADDR(ugq_pu[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal voltage of grid-following inverter, q-axis, phase C",
			PT_double, "ed_pu_A", PADDR(ed_pu[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of grid-following inverter, d-axis, phase A",
			PT_double, "ed_pu_B", PADDR(ed_pu[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of grid-following inverter, d-axis, phase B",
			PT_double, "ed_pu_C", PADDR(ed_pu[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of grid-following inverter, d-axis, phase C",
			PT_double, "eq_pu_A", PADDR(eq_pu[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of grid-following inverter, q-axis, phase A",
			PT_double, "eq_pu_B", PADDR(eq_pu[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of grid-following inverter, q-axis, phase B",
			PT_double, "eq_pu_C", PADDR(eq_pu[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of grid-following inverter, q-axis, phase C",

			PT_double, "igd_ref_A", PADDR(igd_ref[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, d-axis, phase A",
			PT_double, "igd_ref_B", PADDR(igd_ref[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, d-axis, phase B",
			PT_double, "igd_ref_C", PADDR(igd_ref[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, d-axis, phase C",
			PT_double, "igq_ref_A", PADDR(igq_ref[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, q-axis, phase A",
			PT_double, "igq_ref_B", PADDR(igq_ref[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, q-axis, phase B",
			PT_double, "igq_ref_C", PADDR(igq_ref[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, q-axis, phase C",

			PT_double, "igd_ref_A_filter", PADDR(igd_filter_ref[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference terminal current of grid-following inverter, d-axis, phase A, current source representation",
			PT_double, "igd_ref_B_filter", PADDR(igd_filter_ref[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference terminal current of grid-following inverter, d-axis, phase B, current source representation",
			PT_double, "igd_ref_C_filter", PADDR(igd_filter_ref[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference terminal current of grid-following inverter, d-axis, phase C, current source representation",
			PT_double, "igq_ref_A_filter", PADDR(igq_filter_ref[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference terminal current of grid-following inverter, q-axis, phase A, current source representation",
			PT_double, "igq_ref_B_filter", PADDR(igq_filter_ref[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference terminal current of grid-following inverter, q-axis, phase B, current source representation",
			PT_double, "igq_ref_C_filter", PADDR(igq_filter_ref[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference terminal current of grid-following inverter, q-axis, phase C, current source representation",

			PT_double, "igd_pu_A", PADDR(igd_pu[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase A",
			PT_double, "igd_pu_B", PADDR(igd_pu[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase B",
			PT_double, "igd_pu_C", PADDR(igd_pu[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase C",
			PT_double, "igq_pu_A", PADDR(igq_pu[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, q-axis, phase A",
			PT_double, "igq_pu_B", PADDR(igq_pu[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, q-axis, phase B",
			PT_double, "igq_pu_C", PADDR(igq_pu[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, q-axis, phase C",

			// Frequency-watt and volt-var in Grid-Following Control mode
			PT_bool, "frequency_watt", PADDR(frequency_watt), PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether inverter f/p droop is included or not",
			PT_bool, "checkRampRate_real", PADDR(checkRampRate_real), PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether check the ramp rate",
			PT_bool, "volt_var", PADDR(volt_var), PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether inverter volt-var droop is included or not",
			PT_bool, "checkRampRate_reactive", PADDR(checkRampRate_reactive), PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether check the ramp rate",

			PT_double, "rampUpRate_real", PADDR(rampUpRate_real), PT_DESCRIPTION, "DELTAMODE: ramp rate for grid-following frequency-watt",
			PT_double, "rampDownRate_real", PADDR(rampDownRate_real), PT_DESCRIPTION, "DELTAMODE: ramp rate for grid-following frequency-watt",
			PT_double, "rampUpRate_reactive", PADDR(rampUpRate_reactive), PT_DESCRIPTION, "DELTAMODE: ramp rate for grid-following volt-var",
			PT_double, "rampDownRate_reactive", PADDR(rampDownRate_reactive), PT_DESCRIPTION, "DELTAMODE: ramp rate for grid-following volt-var",

			PT_double, "Tpf", PADDR(Tpf), PT_DESCRIPTION, "DELTAMODE: the time constant of power measurement low pass filter in frequency-watt.",
			PT_double, "Tff", PADDR(Tff), PT_DESCRIPTION, "DELTAMODE: the time constant of frequency measurement low pass filter in frequency-watt.",
			PT_double, "Tqf", PADDR(Tqf), PT_DESCRIPTION, "DELTAMODE: the time constant of power measurement low pass filter in volt-var.",
			PT_double, "Tvf", PADDR(Tvf), PT_DESCRIPTION, "DELTAMODE: the time constant of voltage measurement low pass filter in volt-var.",
			PT_double, "Rp[pu]", PADDR(Rp), PT_DESCRIPTION, "DELTAMODE: p-f droop gain in frequency-watt.",
			PT_double, "Pref_droop_pu_filter", PADDR(Pref_droop_pu_filter), PT_DESCRIPTION, "DELTAMODE: P setpoint with frequency-watt.",
			PT_double, "db_UF[Hz]", PADDR(db_UF), PT_DESCRIPTION, "DELTAMODE: upper dead band for frequency-watt control, UF for under-frequency",
			PT_double, "db_OF[Hz]", PADDR(db_OF), PT_DESCRIPTION, "DELTAMODE: lower dead band for frequency-watt control, OF for over-frequency",
			PT_double, "Rq[pu]", PADDR(Rq), PT_DESCRIPTION, "DELTAMODE: Q-V droop gain in volt-var.",
			PT_double, "db_UV[pu]", PADDR(db_UV), PT_DESCRIPTION, "DELTAMODE: dead band for volt-var control, UV for under-voltage",
			PT_double, "db_OV[pu]", PADDR(db_OV), PT_DESCRIPTION, "DELTAMODE: dead band for volt-var control, OV for over-voltage",
			PT_double, "Qref_droop_pu", PADDR(Qref_droop_pu), PT_DESCRIPTION, "DELTAMODE: Qref calculated by volt-var control",
			PT_double, "Qref_droop_pu_filter", PADDR(Qref_droop_pu_filter), PT_DESCRIPTION, "DELTAMODE: Q setpoint with valt-var.",
			PT_double, "V_filter", PADDR(V_filter), PT_DESCRIPTION, "DELTAMODE: filtered voltage used in volt-var control",

			// PLL Parameters and measurement
			PT_double, "kpPLL", PADDR(kpPLL), PT_DESCRIPTION, "DELTAMODE: Proportional gain of the PLL.",
			PT_double, "kiPLL", PADDR(kiPLL), PT_DESCRIPTION, "DELTAMODE: Integral gain of the PLL.",
			PT_double, "TiPLL", PADDR(TiPLL), PT_DESCRIPTION, "DELTAMODE: Time constant of the PLL.",
			PT_double, "Angle_PLL_A", PADDR(Angle_PLL_blk[0].x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: phase angle of terminal voltage measured by PLL, phase A",
			PT_double, "Angle_PLL_B", PADDR(Angle_PLL_blk[1].x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: phase angle of terminal voltage measured by PLL, phase B",
			PT_double, "Angle_PLL_C", PADDR(Angle_PLL_blk[2].x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: phase angle of terminal voltage measured by PLL, phase C",
			PT_double, "f_PLL_A", PADDR(fPLL[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: frequency of terminal voltage measured by PLL, phase A",
			PT_double, "f_PLL_B", PADDR(fPLL[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: frequency of terminal voltage measured by PLL, phase B",
			PT_double, "f_PLL_C", PADDR(fPLL[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: frequency of terminal voltage measured by PLL, phase C",

			//IEEE 1547 variables
			PT_bool, "enable_1547_checks", PADDR(enable_1547_compliance), PT_DESCRIPTION, "DELTAMODE: Enable IEEE 1547-2003 disconnect checking",
			PT_double, "reconnect_time[s]", PADDR(IEEE1547_reconnect_time), PT_DESCRIPTION, "DELTAMODE: Time delay after IEEE 1547-2003 violation clears before resuming generation",
			PT_bool, "inverter_1547_status", PADDR(inverter_1547_status), PT_DESCRIPTION, "DELTAMODE: Indicator if the inverter is curtailed due to a 1547 violation or not",

			//Select 1547 type to auto-populate
			PT_enumeration, "IEEE_1547_version", PADDR(ieee_1547_version), PT_DESCRIPTION, "DELTAMODE: Version of IEEE 1547 to use to populate defaults",
				PT_KEYWORD, "NONE", (enumeration)IEEE1547_NONE,
				PT_KEYWORD, "IEEE1547_2003", (enumeration)IEEE1547_2003,
				PT_KEYWORD, "IEEE1547A_2014", (enumeration)IEEE1547A_2014,
				PT_KEYWORD, "IEEE1547_2018", (enumeration)IEEE1547_2018,

			//Frequency bands of 1547a-2014 checks
			PT_double, "over_freq_high_cutout[Hz]", PADDR(IEEE1547_over_freq_high_band_setpoint), PT_DESCRIPTION, "DELTAMODE: OF2 set point for IEEE 1547a",
			PT_double, "over_freq_high_disconnect_time[s]", PADDR(IEEE1547_over_freq_high_band_delay), PT_DESCRIPTION, "DELTAMODE: OF2 clearing time for IEEE1547a",
			PT_double, "over_freq_low_cutout[Hz]", PADDR(IEEE1547_over_freq_low_band_setpoint), PT_DESCRIPTION, "DELTAMODE: OF1 set point for IEEE 1547a",
			PT_double, "over_freq_low_disconnect_time[s]", PADDR(IEEE1547_over_freq_low_band_delay), PT_DESCRIPTION, "DELTAMODE: OF1 clearing time for IEEE 1547a",
			PT_double, "under_freq_high_cutout[Hz]", PADDR(IEEE1547_under_freq_high_band_setpoint), PT_DESCRIPTION, "DELTAMODE: UF2 set point for IEEE 1547a",
			PT_double, "under_freq_high_disconnect_time[s]", PADDR(IEEE1547_under_freq_high_band_delay), PT_DESCRIPTION, "DELTAMODE: UF2 clearing time for IEEE1547a",
			PT_double, "under_freq_low_cutout[Hz]", PADDR(IEEE1547_under_freq_low_band_setpoint), PT_DESCRIPTION, "DELTAMODE: UF1 set point for IEEE 1547a",
			PT_double, "under_freq_low_disconnect_time[s]", PADDR(IEEE1547_under_freq_low_band_delay), PT_DESCRIPTION, "DELTAMODE: UF1 clearing time for IEEE 1547a",

			//Voltage bands of 1547 checks
			PT_double, "under_voltage_low_cutout[pu]", PADDR(IEEE1547_under_voltage_lowest_voltage_setpoint), PT_DESCRIPTION, "Lowest voltage threshold for undervoltage",
			PT_double, "under_voltage_middle_cutout[pu]", PADDR(IEEE1547_under_voltage_middle_voltage_setpoint), PT_DESCRIPTION, "Middle-lowest voltage threshold for undervoltage",
			PT_double, "under_voltage_high_cutout[pu]", PADDR(IEEE1547_under_voltage_high_voltage_setpoint), PT_DESCRIPTION, "High value of low voltage threshold for undervoltage",
			PT_double, "over_voltage_low_cutout[pu]", PADDR(IEEE1547_over_voltage_low_setpoint), PT_DESCRIPTION, "Lowest voltage value for overvoltage",
			PT_double, "over_voltage_high_cutout[pu]", PADDR(IEEE1547_over_voltage_high_setpoint), PT_DESCRIPTION, "High voltage value for overvoltage",
			PT_double, "under_voltage_low_disconnect_time[s]", PADDR(IEEE1547_under_voltage_lowest_delay), PT_DESCRIPTION, "Lowest voltage clearing time for undervoltage",
			PT_double, "under_voltage_middle_disconnect_time[s]", PADDR(IEEE1547_under_voltage_middle_delay), PT_DESCRIPTION, "Middle-lowest voltage clearing time for undervoltage",
			PT_double, "under_voltage_high_disconnect_time[s]", PADDR(IEEE1547_under_voltage_high_delay), PT_DESCRIPTION, "Highest voltage clearing time for undervoltage",
			PT_double, "over_voltage_low_disconnect_time[s]", PADDR(IEEE1547_over_voltage_low_delay), PT_DESCRIPTION, "Lowest voltage clearing time for overvoltage",
			PT_double, "over_voltage_high_disconnect_time[s]", PADDR(IEEE1547_over_voltage_high_delay), PT_DESCRIPTION, "Highest voltage clearing time for overvoltage",

			//1547 trip reason
			PT_enumeration, "IEEE_1547_trip_method", PADDR(ieee_1547_trip_method), PT_DESCRIPTION, "DELTAMODE: Reason for IEEE 1547 disconnect - which threshold was hit",
				PT_KEYWORD, "NONE",(enumeration)IEEE_1547_NOTRIP, PT_DESCRIPTION, "No trip reason",
				PT_KEYWORD, "OVER_FREQUENCY_HIGH",(enumeration)IEEE_1547_HIGH_OF, PT_DESCRIPTION, "High over-frequency level trip - OF2",
				PT_KEYWORD, "OVER_FREQUENCY_LOW",(enumeration)IEEE_1547_LOW_OF, PT_DESCRIPTION, "Low over-frequency level trip - OF1",
				PT_KEYWORD, "UNDER_FREQUENCY_HIGH",(enumeration)IEEE_1547_HIGH_UF, PT_DESCRIPTION, "High under-frequency level trip - UF2",
				PT_KEYWORD, "UNDER_FREQUENCY_LOW",(enumeration)IEEE_1547_LOW_UF, PT_DESCRIPTION, "Low under-frequency level trip - UF1",
				PT_KEYWORD, "UNDER_VOLTAGE_LOW",(enumeration)IEEE_1547_LOWEST_UV, PT_DESCRIPTION, "Lowest under-voltage level trip",
				PT_KEYWORD, "UNDER_VOLTAGE_MID",(enumeration)IEEE_1547_MIDDLE_UV, PT_DESCRIPTION, "Middle under-voltage level trip",
				PT_KEYWORD, "UNDER_VOLTAGE_HIGH",(enumeration)IEEE_1547_HIGH_UV, PT_DESCRIPTION, "High under-voltage level trip",
				PT_KEYWORD, "OVER_VOLTAGE_LOW",(enumeration)IEEE_1547_LOW_OV, PT_DESCRIPTION, "Low over-voltage level trip",
				PT_KEYWORD, "OVER_VOLTAGE_HIGH",(enumeration)IEEE_1547_HIGH_OV, PT_DESCRIPTION, "High over-voltage level trip",
			nullptr) < 1)
				GL_THROW("unable to publish properties in %s", __FILE__);

		defaults = this;

		memset(this, 0, sizeof(ibr_gfl));

		if (gl_publish_function(oclass, "preupdate_gen_object", (FUNCTIONADDR)preupdate_ibr_gfl) == nullptr)
			GL_THROW("Unable to publish ibr_gfl deltamode function");
		if (gl_publish_function(oclass, "interupdate_gen_object", (FUNCTIONADDR)interupdate_ibr_gfl) == nullptr)
			GL_THROW("Unable to publish ibr_gfl deltamode function");
		// if (gl_publish_function(oclass, "postupdate_gen_object", (FUNCTIONADDR)postupdate_ibr_gfl) == nullptr)
		// 	GL_THROW("Unable to publish ibr_gfl deltamode function");
		if (gl_publish_function(oclass, "current_injection_update", (FUNCTIONADDR)ibr_gfl_NR_current_injection_update) == nullptr)
			GL_THROW("Unable to publish ibr_gfl current injection update function");
	}
}

//Isa function for identification
int ibr_gfl::isa(char *classname)
{
	return strcmp(classname,"ibr_gfl")==0;
}

/* Object creation is called once for each object that is created by the core */
int ibr_gfl::create(void)
{
	////////////////////////////////////////////////////////
	// DELTA MODE
	////////////////////////////////////////////////////////
	deltamode_inclusive = false; //By default, don't be included in deltamode simulations
	first_sync_delta_enabled = false;
	deltamode_exit_iteration_met = false;	//Init - will be set elsewhere

	inverter_start_time = TS_INVALID;
	inverter_first_step = true;
	first_iteration_current_injection = -1; //Initialize - mainly for tracking SWING_PQ status
	first_deltamode_init = true;	//First time it goes in will be the first time

	//Variable mapping items
	parent_is_a_meter = false;		//By default, no parent meter
	parent_is_single_phase = false; //By default, we're three-phase
	parent_is_triplex = false;		//By default, not a triplex
	attached_bus_type = 0;			//By default, we're basically a PQ bus
	swing_test_fxn = nullptr;			//By default, no mapping

	pCircuit_V[0] = pCircuit_V[1] = pCircuit_V[2] = nullptr;
	pLine_I[0] = pLine_I[1] = pLine_I[2] = nullptr;
	pLine_unrotI[0] = pLine_unrotI[1] = pLine_unrotI[2] = nullptr;
	pPower[0] = pPower[1] = pPower[2] = nullptr;
	pIGenerated[0] = pIGenerated[1] = pIGenerated[2] = nullptr;

	pMeterStatus = nullptr; // check if the meter is in service
	pbus_full_Y_mat = nullptr;
	pGenerated = nullptr;
	pFrequency = nullptr;

	//Zero the accumulators
	value_Circuit_V[0] = value_Circuit_V[1] = value_Circuit_V[2] = gld::complex(0.0, 0.0);
	value_Line_I[0] = value_Line_I[1] = value_Line_I[2] = gld::complex(0.0, 0.0);
	value_Line_unrotI[0] = value_Line_unrotI[1] = value_Line_unrotI[2] = gld::complex(0.0, 0.0);
	value_Power[0] = value_Power[1] = value_Power[2] = gld::complex(0.0, 0.0);
	value_IGenerated[0] = value_IGenerated[1] = value_IGenerated[2] = gld::complex(0.0, 0.0);
	prev_value_IGenerated[0] = prev_value_IGenerated[1] = prev_value_IGenerated[2] = gld::complex(0.0, 0.0);
	value_MeterStatus = 1; //Connected, by default
	value_Frequency = 60.0;	//Just set it to nominal 

	w_ref = -99.0;	//Flag value
	f_nominal = 60;
	w_base = 2.0 * PI * f_nominal; //rad/s
	freq = 60;
	fset = -99.0;	//Flag value

	// PLL
	fPLL[0] = fPLL[1] = fPLL[2] = 60.0;

	Angle_PLL_blk[0].setparams(1.0,-1000.0,1000.0,-1000.0,1000.0);
	Angle_PLL_blk[1].setparams(1.0,-1000.0,1000.0,-1000.0,1000.0);
	Angle_PLL_blk[2].setparams(1.0,-1000.0,1000.0,-1000.0,1000.0);
	Angle_PLL_blk[0].init_given_y(0.0);
	Angle_PLL_blk[1].init_given_y((4.0 / 3.0) * PI);
	Angle_PLL_blk[2].init_given_y((2.0 / 3.0) * PI);

	Angle_PLL[0] = 0.0;
	Angle_PLL[1] = (4.0 / 3.0) * PI;
	Angle_PLL[2] = (2.0 / 3.0) * PI;


	// ramp rate check for grid-following inverters
	checkRampRate_real = false;
	checkRampRate_reactive = false;
	rampUpRate_real = 1.67; //unit: pu/s
	rampDownRate_real = 1.67; //unit: pu/s
	rampUpRate_reactive = 1.67; //unit: pu/s
	rampDownRate_reactive = 1.67; //unit: pu/s

	//Limits for the reference currents of grid-following inverters
	igd_ref_max = 1.2;
	igd_ref_min = -1.2;
	igq_ref_max = 1.2;
	igq_ref_min = -1.2;

	// Grid-Following controller parameters
	Tif = 0.005;
	Tif_igd_ref = 0.01;
	Tif_igq_ref = 0.01;//

	igd_delay_flag = 1;

	//frequency-watt
	Pref_max = 1.05; // per unit
	Pref_min = -1.05;	// per unit
	db_UF = 0.036;  // dead band 0.036 Hz, default value by IEEE 1547 2018
	db_OF = 0.036;  // dead band 0.036 Hz, default value by IEEE 1547 2018

	//volt-var
	db_UV = 0;  // volt-var dead band
	db_OV = 0;  // volt-var dead band

	//Vset = 1;  // per unit
	Qref_max = 1.05; // per unit
	Qref_min = -1.05;	// per unit

	GridFollowing_curr_convergence_criterion = 0.01;

	//Set up the deltamode "next state" tracking variable
	desired_simulation_mode = SM_EVENT;

	//Tracking variable
	last_QSTS_GF_Update = TS_NEVER_DBL;

	//1547 parameters
	Reconnect_Warn_Flag = true;			//Flag to send the warning initially

	enable_1547_compliance = false;		//1547 turned off, but default
	IEEE1547_reconnect_time = 300.0;				//5 minute default, as suggested by 1547-2003
	inverter_1547_status = true;		//Not in a violation, by default
	IEEE1547_out_of_violation_time_total = 0.0;	//Not in a violation, so not tracking 'recovery'
	ieee_1547_delta_return = -1.0;			//Flag as not an issue
	prev_time_dbl_IEEE1547 = 0.0;				//Tracking variable

	//By default, assumed we want to use IEEE 1547a
	ieee_1547_version = IEEE1547A_2014;

	//Flag us as no reason
	ieee_1547_trip_method = IEEE_1547_NOTRIP;

	//1547a defaults for triggering - so people can change them - will get adjusted to 1547 in init, if desired
	IEEE1547_over_freq_high_band_setpoint = 62.0;	//OF2 set point for IEEE 1547a
	IEEE1547_over_freq_high_band_delay = 0.16;		//OF2 clearing time for IEEE1547a
	IEEE1547_over_freq_high_band_viol_time = 0.0;	//Accumulator for IEEE1547a OF high-band violation time
	IEEE1547_over_freq_low_band_setpoint = 60.5;		//OF1 set point for IEEE 1547a
	IEEE1547_over_freq_low_band_delay = 2.0;			//OF1 clearing time for IEEE 1547a
	IEEE1547_over_freq_low_band_viol_time = 0.0;	//Accumulator for IEEE1547a OF low-band violation time
	IEEE1547_under_freq_high_band_setpoint = 59.5;	//UF2 set point for IEEE 1547a
	IEEE1547_under_freq_high_band_delay = 2.0;		//UF2 clearing time for IEEE1547a
	IEEE1547_under_freq_high_band_viol_time = 0.0;	//Accumulator for IEEE1547a UF high-band violation time
	IEEE1547_under_freq_low_band_setpoint = 57.0;		//UF1 set point for IEEE 1547a
	IEEE1547_under_freq_low_band_delay = 0.16;		//UF1 clearing time for IEEE 1547a
	IEEE1547_under_freq_low_band_viol_time = 0.0;	//Accumulator for IEEE1547a UF low-band violation time

	//Voltage set points - 1547a defaults
	IEEE1547_under_voltage_lowest_voltage_setpoint = 0.45;	//Lowest voltage threshold for undervoltage
	IEEE1547_under_voltage_middle_voltage_setpoint = 0.60;	//Middle-lowest voltage threshold for undervoltage
	IEEE1547_under_voltage_high_voltage_setpoint = 0.88;		//High value of low voltage threshold for undervoltage
	IEEE1547_over_voltage_low_setpoint = 1.10;				//Lowest voltage value for overvoltage
	IEEE1547_over_voltage_high_setpoint = 1.20;				//High voltage value for overvoltage
	IEEE1547_under_voltage_lowest_delay = 0.16;				//Lowest voltage clearing time for undervoltage
	IEEE1547_under_voltage_middle_delay = 1.0;				//Middle-lowest voltage clearing time for undervoltage
	IEEE1547_under_voltage_high_delay = 2.0;					//Highest voltage clearing time for undervoltage
	IEEE1547_over_voltage_low_delay = 1.0;					//Lowest voltage clearing time for overvoltage
	IEEE1547_over_voltage_high_delay = 0.16;					//Highest voltage clearing time for overvoltage
	IEEE1547_under_voltage_lowest_viol_time = 0.0;			//Lowest low voltage threshold violation timer
	IEEE1547_under_voltage_middle_viol_time = 0.0;			//Middle low voltage threshold violation timer
	IEEE1547_under_voltage_high_viol_time = 0.0;				//Highest low voltage threshold violation timer
	IEEE1547_over_voltage_low_viol_time = 0.0;				//Lowest high voltage threshold violation timer
	IEEE1547_over_voltage_high_viol_time = 0.0;				//Highest high voltage threshold violation timer

	node_nominal_voltage = 120.0;		//Just pick a value

	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int ibr_gfl::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	double temp_volt_mag;
	double temp_volt_ang[3];
	gld_property *temp_property_pointer = nullptr;
	gld_property *Frequency_mapped = nullptr;
	gld_wlock *test_rlock = nullptr;
	bool temp_bool_value;
	int temp_idx_x, temp_idx_y;
	unsigned iindex, jindex;
	gld::complex temp_complex_value;
	complex_array temp_complex_array, temp_child_complex_array;
	OBJECT *tmp_obj = nullptr;
	gld_object *tmp_gld_obj = nullptr;
	STATUS return_value_init;
	bool childed_connection = false;
	STATUS fxn_return_status;

	//Deferred initialization code
	if (parent != nullptr)
	{
		if ((parent->flags & OF_INIT) != OF_INIT)
		{
			char objname[256];
			gl_verbose("ibr_gfl::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}

	// Data sanity check
	if (S_base <= 0)
	{
		S_base = 1000;
		gl_warning("ibr_gfl:%d - %s - The rated power of this inverter must be positive - set to 1 kVA.",obj->id,(obj->name ? obj->name : "unnamed"));
		/*  TROUBLESHOOT
		The ibr_gfl has a rated power that is negative.  It defaulted to a 1 kVA inverter.  If this is not desired, set the property properly in the GLM.
		*/
	}

	//See if the global flag is set - if so, add the object flag
	if (all_generator_delta)
	{
		obj->flags |= OF_DELTAMODE;
	}

	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true; //Set the flag and off we go
	}

	// find parent meter or triplex_meter, if not defined, use default voltages, and if
	// the parent is not a meter throw an exception
	if (parent != nullptr)
	{
		//See what kind of parent we are
		if (gl_object_isa(parent, "meter", "powerflow") || gl_object_isa(parent, "node", "powerflow") || gl_object_isa(parent, "load", "powerflow") ||
			gl_object_isa(parent, "triplex_meter", "powerflow") || gl_object_isa(parent, "triplex_node", "powerflow") || gl_object_isa(parent, "triplex_load", "powerflow"))
		{
			//See if we're in deltamode and VSI - if not, we don't care about the "parent-ception" mapping
			//Normal deltamode just goes through current interfaces, so don't need this craziness
			if (deltamode_inclusive)
			{
				//See if this attached node is a child or not
				if (parent->parent != nullptr)
				{
					//Map parent - wherever it may be
					temp_property_pointer = new gld_property(parent,"NR_powerflow_parent");

					//Make sure it worked
					if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_objectref())
					{
						GL_THROW("ibr_gfl:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
						//Defined elsewhere
					}

					//Pull the mapping - gld_object
					tmp_gld_obj = temp_property_pointer->get_objectref();

					//Pull the proper object reference
					tmp_obj = tmp_gld_obj->my();

					//free the property
					delete temp_property_pointer;

					//See what it is
					if (!gl_object_isa(tmp_obj, "meter", "powerflow") && !gl_object_isa(tmp_obj, "node", "powerflow") && !gl_object_isa(tmp_obj, "load", "powerflow") &&
						!gl_object_isa(tmp_obj, "triplex_meter", "powerflow") && !gl_object_isa(tmp_obj, "triplex_node", "powerflow") && !gl_object_isa(tmp_obj, "triplex_load", "powerflow"))
					{
						//Not a wierd map, just use normal parent
						tmp_obj = parent;
					}
					else //Implies it is a powerflow parent
					{
						//Set the flag
						childed_connection = true;

						//See if we are deltamode-enabled -- if so, flag our parent while we're here
						//Map our deltamode flag and set it (parent will be done below)
						temp_property_pointer = new gld_property(parent, "Norton_dynamic");

						//Make sure it worked
						if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_bool())
						{
							GL_THROW("ibr_gfl:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", parent->name ? parent->name : "unnamed");
							/*  TROUBLESHOOT
							While attempting to set up the deltamode interfaces and calculations with powerflow, the required interface could not be mapped.
							Please check your GLM and try again.  If the error persists, please submit a trac ticket with your code.
							*/
						}

						//Flag it to true
						temp_bool_value = true;
						temp_property_pointer->setp<bool>(temp_bool_value, *test_rlock);

						//Remove it
						delete temp_property_pointer;

						//Set the child accumulator flag too
						temp_property_pointer = new gld_property(tmp_obj,"Norton_dynamic_child");

						//Make sure it worked
						if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_bool())
						{
							GL_THROW("ibr_gfl:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
							//Defined elsewhere
						}

						//Flag it to true
						temp_bool_value = true;
						temp_property_pointer->setp<bool>(temp_bool_value,*test_rlock);

						//Remove it
						delete temp_property_pointer;
					} //End we were a powerflow child
				}	  //Parent has a parent
				else  //It is null
				{
					//Just point it to the normal parent
					tmp_obj = parent;
				}
			}	 //End deltamode inclusive
			else //Not deltamode and VSI -- all other combinations just use standard parents
			{
				//Just point it to the normal parent
				tmp_obj = parent;
			}

			//Map and pull the phases - true parent
			temp_property_pointer = new gld_property(parent, "phases");

			//Make sure it worked
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
			phases = temp_property_pointer->get_set();

			//Clear the temporary pointer
			delete temp_property_pointer;

			//Pull the bus type
			temp_property_pointer = new gld_property(tmp_obj, "bustype");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_enumeration())
			{
				GL_THROW("ibr_gfl:%s failed to map bustype variable from %s", obj->name ? obj->name : "unnamed", obj->parent->name ? obj->parent->name : "unnamed");
				/*  TROUBLESHOOT
				While attempting to map the bustype variable from the parent node, an error was encountered.  Please try again.  If the error
				persists, please report it with your GLM via the issues tracking system.
				*/
			}

			//Pull the value of the bus
			attached_bus_type = temp_property_pointer->get_enumeration();

			//Remove it
			delete temp_property_pointer;

			//Map up the frequency pointer
			pFrequency = new gld_property(tmp_obj,"measured_frequency");

			//Make sure it worked
			if (!pFrequency->is_valid() || !pFrequency->is_double())
			{
				GL_THROW("ibr_gfl:%s failed to map measured_frequency variable from %s", obj->name ? obj->name : "unnamed", obj->parent->name ? obj->parent->name : "unnamed");
				/*  TROUBLESHOOT
				While attempting to map the frequency variable from the parent node, an error was encountered.  Please try again.  If the error
				persists, please report it with your GLM via the issues tracking system.
				*/
			}

			//Pull the current value, because
			value_Frequency = pFrequency->get_double();

			//Determine parent type
			//Triplex first, otherwise it tries to map to three-phase (since all triplex are nodes)
			if (gl_object_isa(tmp_obj, "triplex_meter", "powerflow") || gl_object_isa(tmp_obj, "triplex_node", "powerflow") || gl_object_isa(tmp_obj, "triplex_load", "powerflow"))
			{
				//Indicate this is a meter, but is also a single-phase variety
				parent_is_a_meter = true;
				parent_is_single_phase = true;
				parent_is_triplex = true;

				//Map the various powerflow variables
				//Map the other two here for initialization problem
				pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_12");
				pCircuit_V[1] = map_complex_value(tmp_obj, "voltage_1");
				pCircuit_V[2] = map_complex_value(tmp_obj, "voltage_2");

				//Get 12 and Null the rest
				pLine_I[0] = map_complex_value(tmp_obj, "current_12");
				pLine_I[1] = nullptr;
				pLine_I[2] = nullptr;

				//Pull power12, then null the rest
				pPower[0] = map_complex_value(tmp_obj, "power_12");
				pPower[1] = nullptr; //Not used
				pPower[2] = nullptr; //Not used

				pLine_unrotI[0] = map_complex_value(tmp_obj, "prerotated_current_12");
				pLine_unrotI[1] = nullptr; //Not used
				pLine_unrotI[2] = nullptr; //Not used

				//Map IGenerated, even though triplex can't really use this yet (just for the sake of doing so)
				pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_12");
				pIGenerated[1] = nullptr;
				pIGenerated[2] = nullptr;
			} //End triplex parent
			else if (gl_object_isa(tmp_obj, "meter", "powerflow") || gl_object_isa(tmp_obj, "node", "powerflow") || gl_object_isa(tmp_obj, "load", "powerflow"))
			{
				//See if we're three-phased
				if ((phases & 0x07) == 0x07) //We are
				{
					parent_is_a_meter = true;
					parent_is_single_phase = false;
					parent_is_triplex = false;

					//Map the various powerflow variables
					pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_A");
					pCircuit_V[1] = map_complex_value(tmp_obj, "voltage_B");
					pCircuit_V[2] = map_complex_value(tmp_obj, "voltage_C");

					pLine_I[0] = map_complex_value(tmp_obj, "current_A");
					pLine_I[1] = map_complex_value(tmp_obj, "current_B");
					pLine_I[2] = map_complex_value(tmp_obj, "current_C");

					pPower[0] = map_complex_value(tmp_obj, "power_A");
					pPower[1] = map_complex_value(tmp_obj, "power_B");
					pPower[2] = map_complex_value(tmp_obj, "power_C");

					pLine_unrotI[0] = map_complex_value(tmp_obj, "prerotated_current_A");
					pLine_unrotI[1] = map_complex_value(tmp_obj, "prerotated_current_B");
					pLine_unrotI[2] = map_complex_value(tmp_obj, "prerotated_current_C");

					//Map the current injection variables
					pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_A");
					pIGenerated[1] = map_complex_value(tmp_obj, "deltamode_generator_current_B");
					pIGenerated[2] = map_complex_value(tmp_obj, "deltamode_generator_current_C");
				}
				else //Not three-phase
				{
					//Just assume this is true - the only case it isn't is a GL_THROW, so it won't matter
					parent_is_a_meter = true;
					parent_is_single_phase = true;
					parent_is_triplex = false;

					//nullptr all the secondary indices - we won't use any of them
					pCircuit_V[1] = nullptr;
					pCircuit_V[2] = nullptr;

					pLine_I[1] = nullptr;
					pLine_I[2] = nullptr;

					pPower[1] = nullptr;
					pPower[2] = nullptr;

					pLine_unrotI[1] = nullptr;
					pLine_unrotI[2] = nullptr;

					pIGenerated[1] = nullptr;
					pIGenerated[2] = nullptr;

					if ((phases & 0x07) == 0x01) //A
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_A");
						pLine_I[0] = map_complex_value(tmp_obj, "current_A");
						pPower[0] = map_complex_value(tmp_obj, "power_A");
						pLine_unrotI[0] = map_complex_value(tmp_obj, "prerotated_current_A");
						pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_A");
					}
					else if ((phases & 0x07) == 0x02) //B
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_B");
						pLine_I[0] = map_complex_value(tmp_obj, "current_B");
						pPower[0] = map_complex_value(tmp_obj, "power_B");
						pLine_unrotI[0] = map_complex_value(tmp_obj, "prerotated_current_B");
						pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_B");
					}
					else if ((phases & 0x07) == 0x04) //C
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_C");
						pLine_I[0] = map_complex_value(tmp_obj, "current_C");
						pPower[0] = map_complex_value(tmp_obj, "power_C");
						pLine_unrotI[0] = map_complex_value(tmp_obj, "prerotated_current_C");
						pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_C");
					}
					else //Not three-phase, but has more than one phase - fail, because we don't do this right
					{
						GL_THROW("ibr_gfl:%s is not connected to a single-phase or three-phase node - two-phase connections are not supported at this time.", obj->name ? obj->name : "unnamed");
						/*  TROUBLESHOOT
						The ibr_gfl only supports single phase (A, B, or C or triplex) or full three-phase connections.  If you try to connect it differntly than this, it will not work.
						*/
					}
				} //End non-three-phase
			}	  //End non-triplex parent

			//*** Common items ****//
			// Many of these go to the "true parent", not the "powerflow parent"

			//Map the nominal frequency
			temp_property_pointer = new gld_property("powerflow::nominal_frequency");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
			{
				GL_THROW("ibr_gfl:%d %s failed to map the nominal_frequency property", obj->id, (obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the nominal_frequency property, an error occurred.  Please try again.
				If the error persists, please submit your GLM and a bug report to the ticketing system.
				*/
			}

			//Must be valid, read it
			f_nominal = temp_property_pointer->get_double();

			//Remove it
			delete temp_property_pointer;
			
			//See if we are deltamode-enabled -- powerflow parent version
			if (deltamode_inclusive)
			{
				//Map our deltamode flag and set it (parent will be done below)
				temp_property_pointer = new gld_property(tmp_obj, "Norton_dynamic");

				//Make sure it worked
				if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_bool())
				{
					GL_THROW("ibr_gfl:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", tmp_obj->name ? tmp_obj->name : "unnamed");
					//Defined elsewhere
				}

				//Flag it to true
				temp_bool_value = true;
				temp_property_pointer->setp<bool>(temp_bool_value, *test_rlock);

				//Remove it
				delete temp_property_pointer;

				// Obtain the Z_base of the system for calculating filter impedance
				//Link to nominal voltage
				temp_property_pointer = new gld_property(parent, "nominal_voltage");

				//Make sure it worked
				if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_double())
				{
					gl_error("ibr_gfl:%d %s failed to map the nominal_voltage property", obj->id, (obj->name ? obj->name : "Unnamed"));
					/*  TROUBLESHOOT
					While attempting to map the nominal_voltage property, an error occurred.  Please try again.
					If the error persists, please submit your GLM and a bug report to the ticketing system.
					*/

					return FAILED;
				}
				//Default else, it worked

				//Copy that value out - adjust if triplex
				if (parent_is_triplex)
				{
					node_nominal_voltage = 2.0 * temp_property_pointer->get_double(); //Adjust to 240V
				}
				else
				{
					node_nominal_voltage = temp_property_pointer->get_double();
				}

				//Remove the property pointer
				delete temp_property_pointer;

				V_base = node_nominal_voltage;

				if ((phases & 0x07) == 0x07)
				{
					I_base = S_base / V_base / 3.0;
					Z_base = (node_nominal_voltage * node_nominal_voltage) / (S_base / 3.0); // voltage is phase to ground voltage, S_base is three phase capacity

					filter_admittance = gld::complex(0.0, (w_base*CFilt*3));

					for (iindex = 0; iindex < 3; iindex++)
					{
						for (jindex = 0; jindex < 3; jindex++)
						{
							if (iindex == jindex)
							{
								generator_admittance[iindex][jindex] = filter_admittance;
							}
							else
							{
								generator_admittance[iindex][jindex] = gld::complex(0.0, 0.0);
							}
						}
					}
				}
				else //Must be triplex or single-phased (above check should remove others)
				{
					I_base = S_base / V_base;
					Z_base = (node_nominal_voltage * node_nominal_voltage) / S_base; // voltage is phase to ground voltage, S_base is single phase capacity


					filter_admittance = gld::complex(0.0, (w_base*CFilt*3));

					//Zero the admittance directly - this was probably already done, but lets be paranoid
					//Semi-useless for one entry, since we'll just overwrite it here shortly (but easier this way)
					generator_admittance[0][0] = generator_admittance[0][1] = generator_admittance[0][2] = gld::complex(0.0, 0.0);
					generator_admittance[1][0] = generator_admittance[1][1] = generator_admittance[1][2] = gld::complex(0.0, 0.0);
					generator_admittance[2][0] = generator_admittance[2][1] = generator_admittance[2][2] = gld::complex(0.0, 0.0);

					//See which one we are, to figure out where to store the admittance
					if ((phases & 0x10) == 0x10) //Triplex
					{
						generator_admittance[0][0] = filter_admittance;
					}
					else if ((phases & 0x07) == 0x01) //A
					{
						generator_admittance[0][0] = filter_admittance;
					}
					else if ((phases & 0x07) == 0x02) //B
					{
						generator_admittance[1][1] = filter_admittance;
					}
					else //Must be C, by default
					{
						generator_admittance[2][2] = filter_admittance;
					}
				}

				//Map the full_Y parameter to inject the admittance portion into it
				pbus_full_Y_mat = new gld_property(tmp_obj, "deltamode_full_Y_matrix");

				//Check it
				if (!pbus_full_Y_mat->is_valid() || !pbus_full_Y_mat->is_complex_array())
				{
					GL_THROW("ibr_gfl:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", tmp_obj->name ? tmp_obj->name : "unnamed");
					/*  TROUBLESHOOT
					While attempting to set up the deltamode interfaces and calculations with powerflow, the required interface could not be mapped.
					Please check your GLM and try again.  If the error persists, please submit a trac ticket with your code.
					*/
				}

				//Pull down the variable
				pbus_full_Y_mat->getp<complex_array>(temp_complex_array, *test_rlock);

				//See if it is valid
				if (!temp_complex_array.is_valid(0, 0))
				{
					//Create it
					temp_complex_array.grow_to(3, 3);

					//Zero it, by default
					for (temp_idx_x = 0; temp_idx_x < 3; temp_idx_x++)
					{
						for (temp_idx_y = 0; temp_idx_y < 3; temp_idx_y++)
						{
							temp_complex_array.set_at(temp_idx_x, temp_idx_y, gld::complex(0.0, 0.0));
						}
					}
				}
				else //Already populated, make sure it is the right size!
				{
					if ((temp_complex_array.get_rows() != 3) && (temp_complex_array.get_cols() != 3))
					{
						GL_THROW("ibr_gfl:%s exposed Norton-equivalent matrix is the wrong size!", obj->name ? obj->name : "unnamed");
						/*  TROUBLESHOOT
						While mapping to an admittance matrix on the parent node device, it was found it is the wrong size.
						Please try again.  If the error persists, please submit your code and model via the issue tracking system.
						*/
					}
					//Default else -- right size
				}

				//See if we were connected to a powerflow child
				if (childed_connection)
				{
					temp_property_pointer = new gld_property(parent,"deltamode_full_Y_matrix");

					//Check it
					if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_complex_array())
					{
						GL_THROW("ibr_gfl:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
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
							GL_THROW("ibr_gfl:%s exposed Norton-equivalent matrix is the wrong size!",obj->name?obj->name:"unnamed");
							//Defined above
						}
						//Default else -- right size
					}
				}//End childed powerflow parent

				//Loop through and store the values
				for (temp_idx_x = 0; temp_idx_x < 3; temp_idx_x++)
				{
					for (temp_idx_y = 0; temp_idx_y < 3; temp_idx_y++)
					{
						//Read the existing value
						temp_complex_value = temp_complex_array.get_at(temp_idx_x, temp_idx_y);

						//Accumulate admittance into it
						temp_complex_value += generator_admittance[temp_idx_x][temp_idx_y];

						//Store it
						temp_complex_array.set_at(temp_idx_x, temp_idx_y, temp_complex_value);

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
				pbus_full_Y_mat->setp<complex_array>(temp_complex_array, *test_rlock);

				//See if the childed powerflow exists
				if (childed_connection)
				{
					temp_property_pointer->setp<complex_array>(temp_child_complex_array,*test_rlock);

					//Clear it
					delete temp_property_pointer;
				}

				//Map the power variable
				pGenerated = map_complex_value(tmp_obj, "deltamode_PGenTotal");
			} //End VSI common items

			//Map status - true parent
			pMeterStatus = new gld_property(parent, "service_status");

			//Check it
			if (!pMeterStatus->is_valid() || !pMeterStatus->is_enumeration())
			{
				GL_THROW("ibr_gfl failed to map powerflow status variable");
				/*  TROUBLESHOOT
				While attempting to map the service_status variable of the parent
				powerflow object, an error occurred.  Please try again.  If the error
				persists, please submit your code and a bug report via the trac website.
				*/
			}

			//Pull initial voltages, but see which ones we should grab
			if (parent_is_single_phase)
			{
				//Powerflow values -- pull the initial value (should be nominals)
				value_Circuit_V[0] = pCircuit_V[0]->get_complex(); //A, B, C, or 12
				value_Circuit_V[1] = gld::complex(0.0, 0.0);			   //Reset, for giggles
				value_Circuit_V[2] = gld::complex(0.0, 0.0);			   //Same
			}
			else //Must be a three-phase, pull them all
			{
				//Powerflow values -- pull the initial value (should be nominals)
				value_Circuit_V[0] = pCircuit_V[0]->get_complex(); //A
				value_Circuit_V[1] = pCircuit_V[1]->get_complex(); //B
				value_Circuit_V[2] = pCircuit_V[2]->get_complex(); //C
			}
		}	 //End valid powerflow parent
		else //Not sure what it is
		{
			GL_THROW("ibr_gfl must have a valid powerflow object as its parent, or no parent at all");
			/*  TROUBLESHOOT
			Check the parent object of the inverter.  The ibr_gfl is only able to be childed via to powerflow objects.
			Alternatively, you can also choose to have no parent, in which case the ibr_gfl will be a stand-alone application
			using default voltage values for solving purposes.
			*/
		}
	}	 //End parent call
	else //Must not have a parent
	{
		//Indicate we don't have a meter parent, nor is it single phase (though the latter shouldn't matter)
		parent_is_a_meter = false;
		parent_is_single_phase = false;
		parent_is_triplex = false;

		gl_warning("ibr_gfl:%d has no parent meter object defined; using static voltages", obj->id);
		/*  TROUBLESHOOT
		An ibr_gfl in the system does not have a parent attached.  It is using static values for the voltage.
		*/

		// Declare all 3 phases
		phases = 0x07;

		//Powerflow values -- set defaults here -- sets up like three-phase connection - use rated kV, just because
		//Set that magnitude
		value_Circuit_V[0].SetPolar(default_line_voltage, 0);
		value_Circuit_V[1].SetPolar(default_line_voltage, -2.0 / 3.0 * PI);
		value_Circuit_V[2].SetPolar(default_line_voltage, 2.0 / 3.0 * PI);

		//Define the default
		value_MeterStatus = 1;

		//Double-set the nominal frequency to NA - no powerflow available
		f_nominal = 60.0;
		value_Frequency = 60.0;
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
			gl_warning("ibr_gfl:%s indicates it wants to run deltamode, but the module-level flag is not set!", obj->name ? obj->name : "unnamed");
			/*  TROUBLESHOOT
			The ibr_gfl object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			//Flag as the first run
			first_sync_delta_enabled = true;

			//Add us to the list
			fxn_return_status = add_gen_delta_obj(obj,false);

			//Check it
			if (fxn_return_status == FAILED)
			{
				GL_THROW("ibr_gfl:%s - failed to add object to generator deltamode object list", obj->name ? obj->name : "unnamed");
				/*  TROUBLESHOOT
				The ibr_gfl object encountered an issue while trying to add itself to the generator deltamode object list.  If the error
				persists, please submit an issue via GitHub.
				*/
			}
		}
		//Default else - don't do anything

		//Check for frequency 
		if (enable_1547_compliance)
		{
			//Check if we're grid following - if grid forming, we don't do these for now
			return_value_init = initalize_IEEE_1547_checks();

			//Check
			if (return_value_init == FAILED)
			{
				GL_THROW("ibr_gfl:%d-%s - initalizing the IEEE 1547 checks failed",get_id(),get_name());
				/*  TROUBLESHOOT
				While attempting to initialize some of the variables for the inverter IEEE 1547 functionality, an error occurred.
				Please try again.  If the error persists, please submit your code and a bug report via the issues tracker.
				*/
			}
		}
		//Default else - don't do anything
	}	 //End deltamode inclusive
	else //This particular model isn't enabled
	{
		if (enable_subsecond_models)
		{
			gl_warning("ibr_gfl:%d %s - Deltamode is enabled for the module, but not this inverter!", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The ibr_gfl is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this ibr_gfl may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
		
		if (enable_1547_compliance)
		{
			//Initalize it
			return_value_init = initalize_IEEE_1547_checks();

			//Check
			if (return_value_init == FAILED)
			{
				GL_THROW("ibr_gfl:%d-%s - initalizing the IEEE 1547 checks failed",get_id(),get_name());
				/*  TROUBLESHOOT
				While attempting to initialize some of the variables for the inverter IEEE 1547 functionality, an error occurred.
				Please try again.  If the error persists, please submit your code and a bug report via the issues tracker.
				*/
			}

			//Warn, because this probably won't work well
			gl_warning("ibr_gfl:%d-%s - IEEE 1547 checks are enabled, but the model is not deltamode-enabled",get_id(),get_name());
			/*  TROUBLESHOOT
			The IEEE 1547 checks have been enabled for the inverter object, but deltamode is not enabled.  This will severely hinder the inverter's
			ability to do these checks, and really isn't the intended mode of operation.
			*/
		}
		//Default else - 1547 not enabled
	}

	//Other initialization variables
	inverter_start_time = gl_globalclock;

	//Initalize w_ref, if needed
	if (w_ref < 0.0)
	{
		//Assume want set to nominal
		w_ref = 2.0 * PI * f_nominal;
	}

	//Update fset to nominal, if not set
	if (fset < 0.0)
	{
		fset = f_nominal;
	}

	// Initialize parameters
	if (sqrt(Pref*Pref+Qref*Qref) > S_base)
	{
		gl_warning("ibr_gfl:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The values for Pref and Qref are specified such that they will exceed teh rated_power of the inverter.  They have been truncated, with real power taking priority.
		*/
	}

	if(Pref > S_base)
	{
		//Pref = S_base;
	}
	else if (Pref < -S_base)
	{
		//Pref = -S_base;
	}
	else
	{
		if(Qref > sqrt(S_base*S_base-Pref*Pref))
		{
			//Qref = sqrt(S_base*S_base-Pref*Pref);
		}
		else if (Qref < -sqrt(S_base*S_base-Pref*Pref))
		{
			//Qref = -sqrt(S_base*S_base-Pref*Pref);
		}
	}

	VA_Out = gld::complex(Pref, Qref);

	//See if we had a single phase connection
	if (parent_is_single_phase)
	{
		power_val[0] = VA_Out; //This probably needs to be adjusted for phasing?
	}
	else //Just assume three-phased
	{
		power_val[0] = VA_Out / 3.0;
		power_val[1] = VA_Out / 3.0;
		power_val[2] = VA_Out / 3.0;
	}

	//Init tracking variables
	prev_timestamp_dbl = (double)gl_globalclock;
	prev_time_dbl_IEEE1547 = prev_timestamp_dbl;	//Just init it, regardless of if 1547 is enabled or not

	return 1;
}

TIMESTAMP ibr_gfl::presync(TIMESTAMP t0, TIMESTAMP t1)
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

	return t2;
}

TIMESTAMP ibr_gfl::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret_value;

	FUNCTIONADDR test_fxn;
	STATUS fxn_return_status;

	gld::complex temp_power_val[3];
	gld::complex temp_intermed_curr_val[3];
	char loop_var;

	gld::complex temp_complex_value;
	gld_wlock *test_rlock = nullptr;
	double curr_ts_dbl, diff_dbl, ieee_1547_return_value;
	TIMESTAMP new_ret_value;

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

	//Deltamode check/init items
	if (first_sync_delta_enabled) //Deltamode first pass
	{
		//TODO: LOCKING!
		if (deltamode_inclusive && enable_subsecond_models) //We want deltamode - see if it's populated yet
		{
			if (parent_is_a_meter)
			{
				//Accumulate the starting power
				if (sqrt(Pref*Pref+Qref*Qref) > S_base)
				{
					gl_warning("ibr_gfl:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
					//Defined above
				}

				if(Pref > S_base)
				{
					//Pref = S_base;
				}
				else if (Pref < -S_base)
				{
					//Pref = -S_base;
				}
				else
				{
					if(Qref > sqrt(S_base*S_base-Pref*Pref))
					{
						//Qref = sqrt(S_base*S_base-Pref*Pref);
					}
					else if (Qref < -sqrt(S_base*S_base-Pref*Pref))
					{
						//Qref = -sqrt(S_base*S_base-Pref*Pref);
					}
				}

				temp_complex_value = gld::complex(Pref, Qref);


				//Push it up
				pGenerated->setp<gld::complex>(temp_complex_value, *test_rlock);

				//Map the current injection function
				test_fxn = (FUNCTIONADDR)(gl_get_function(obj->parent, "pwr_current_injection_update_map"));

				//See if it was located
				if (test_fxn == nullptr)
				{
					GL_THROW("ibr_gfl:%s - failed to map additional current injection mapping for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
					/*  TROUBLESHOOT
					While attempting to map the additional current injection function, an error was encountered.
					Please try again.  If the error persists, please submit your code and a bug report via the trac website.
					*/
				}

				//Call the mapping function
				fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *))(*test_fxn))(obj->parent, obj);

				//Make sure it worked
				if (fxn_return_status != SUCCESS)
				{
					GL_THROW("ibr_gfl:%s - failed to map additional current injection mapping for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
					//Defined above
				}
			}

			//Flag it
			first_sync_delta_enabled = false;

		}	 //End deltamode specials - first pass
		else //Somehow, we got here and deltamode isn't properly enabled...odd, just deflag us
		{
			first_sync_delta_enabled = false;
		}
	} //End first delta timestep
	//default else - either not deltamode, or not the first timestep

	//Deflag first timestep tracker
	if (inverter_start_time != t1)
	{
		inverter_first_step = false;
	}

	//Perform 1547 checks, if appropriate (should be grid-following flagged above)
	if (enable_1547_compliance)
	{
		//Extract the current timestamp, as a double
		curr_ts_dbl = (double)gl_globalclock;

		//See if we're a new timestep, otherwise, we don't care
		if (prev_time_dbl_IEEE1547 < curr_ts_dbl)
		{
			//Figure out how far we moved forward
			diff_dbl = curr_ts_dbl - prev_time_dbl_IEEE1547;

			//Update the value
			prev_time_dbl_IEEE1547 = curr_ts_dbl;

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

	//Calculate power based on measured terminal voltage and currents
	if (parent_is_single_phase) // single phase/split-phase implementation
	{
		if (!inverter_first_step)
		{
			//Update output power
			//Get current injected
			terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

			//Update per-unit value
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

			//Update power output variables, just so we can see what is going on

			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];

			VA_Out = power_val[0];

			if (attached_bus_type != 2)
			{

				//Compute desired output - sign convention appears to be backwards
				if (sqrt(Pref*Pref+Qref*Qref) > S_base)
				{
					gl_warning("ibr_gfl:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
					//Defined above
				}

					if(Pref > S_base)
					{
						//Pref = S_base;
					}
					else if (Pref < -S_base)
					{
						//Pref = -S_base;
					}
					else
					{
						if(Qref > sqrt(S_base*S_base-Pref*Pref))
						{
							//Qref = sqrt(S_base*S_base-Pref*Pref);
						}
						else if (Qref < -sqrt(S_base*S_base-Pref*Pref))
						{
							//Qref = -sqrt(S_base*S_base-Pref*Pref);
						}
					}

				gld::complex temp_VA = gld::complex(Pref, Qref);

				//Force the output power the same as glm pre-defined values
				if (value_Circuit_V[0].Mag() > 0.0)
				{
					value_IGenerated[0] = ~(temp_VA / value_Circuit_V[0]) + filter_admittance * value_Circuit_V[0];
				}
				else
				{
					value_IGenerated[0] = gld::complex(0.0,0.0);
				}
			}
		}
	}
	else //Three phase variant
	{

		if (!inverter_first_step)
		{
			//Update output power
			//Get current injected
			terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
			terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
			terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

			//Update per-unit value
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
			terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
			terminal_current_val_pu[2] = terminal_current_val[2] / I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
			power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
			power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

			VA_Out = power_val[0] + power_val[1] + power_val[2];

			// Adjust VSI (not on SWING bus) current injection and e_source values only at the first iteration of each time step
			if (attached_bus_type != 2)
			{

				if (sqrt(Pref*Pref+Qref*Qref) > S_base)
				{
					gl_warning("ibr_gfl:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
					//Defined above
				}

						if(Pref > S_base)
						{
							//Pref = S_base;
						}
						else if (Pref < -S_base)
						{
							//Pref = -S_base;
						}
						else
						{
							if(Qref > sqrt(S_base*S_base-Pref*Pref))
							{
								//Qref = sqrt(S_base*S_base-Pref*Pref);
							}
							else if (Qref < -sqrt(S_base*S_base-Pref*Pref))
							{
								//Qref = -sqrt(S_base*S_base-Pref*Pref);
							}
						}

				gld::complex temp_VA = gld::complex(Pref, Qref);

				// Obtain the positive sequence voltage
				value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
				
				// Obtain the positive sequence current
				if (value_Circuit_V_PS.Mag() > 0.0)
				{
					value_Circuit_I_PS[0] = ~(temp_VA / value_Circuit_V_PS) / 3.0;
				}
				else
				{
					value_Circuit_I_PS[0] = gld::complex(0.0,0.0);
				}
				
				value_Circuit_I_PS[1] = value_Circuit_I_PS[0] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI));
				value_Circuit_I_PS[2] = value_Circuit_I_PS[0] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI));

				terminal_current_val[0] = value_Circuit_I_PS[0];
				terminal_current_val[1] = value_Circuit_I_PS[1];						
				terminal_current_val[2] = value_Circuit_I_PS[2];	
				
				//Update per-unit value
				terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
				terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
				terminal_current_val_pu[2] = terminal_current_val[2] / I_base;
				

				//Back out the current injection
				value_IGenerated[0] = value_Circuit_I_PS[0] + generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2];
				value_IGenerated[1] = value_Circuit_I_PS[1] + generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2];
				value_IGenerated[2] = value_Circuit_I_PS[2] + generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2];


			}
		}
	}

	//Sync the powerflow variables
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values(false);
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

/* Update the injected currents with respect to VA_Out */
void ibr_gfl::update_iGen(gld::complex VA_Out)
{
	char loop_var;

	if (parent_is_single_phase) // single phase/split-phase implementation
	{
		// power_val[0], terminal_current_val[0], & value_IGenerated[0]
		if (value_Circuit_V[0].Mag() > 0.0)
		{
			power_val[0] = VA_Out;
			terminal_current_val[0] = ~(power_val[0] / value_Circuit_V[0]);
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
			value_IGenerated[0] = terminal_current_val[0] + filter_admittance * value_Circuit_V[0];
		}
		else
		{
			power_val[0] = gld::complex(0.0,0.0);
			terminal_current_val[0] = gld::complex(0.0,0.0);
			terminal_current_val_pu[0] = gld::complex(0.0,0.0);
			value_IGenerated[0] = gld::complex(0.0,0.0);
		}
	}
	else
	{
		//Loop through for voltage check
		for (loop_var=0; loop_var<3; loop_var++)
		{
			if (value_Circuit_V[loop_var].Mag() > 0.0)
			{
				// power_val, terminal_current_val, & value_IGenerated
				power_val[loop_var] = VA_Out / 3;

				terminal_current_val[loop_var] = ~(power_val[loop_var] / value_Circuit_V[loop_var]);
				terminal_current_val_pu[loop_var] = terminal_current_val[loop_var] / I_base;
			}
			else
			{
				power_val[loop_var] = gld::complex(0.0,0.0);
				terminal_current_val[loop_var] = gld::complex(0.0,0.0);
				terminal_current_val_pu[loop_var] = gld::complex(0.0,0.0);
			}
		}

		value_IGenerated[0] = terminal_current_val[0] - (-generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
		value_IGenerated[1] = terminal_current_val[1] - (-generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
		value_IGenerated[2] = terminal_current_val[2] - (-generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);
	}
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP ibr_gfl::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2 = TS_NEVER; //By default, we're done forever!

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		reset_complex_powerflow_accumulators();

		//Also pull the current values
		pull_complex_powerflow_values();
	}

		//Check to see if vaalid connection
	if ((value_MeterStatus == 1) && inverter_1547_status)
	{
		if (parent_is_single_phase) // single phase/split-phase implementation
		{
			//Update output power
			//Get current injected
			terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

			//Update per-unit value
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
			VA_Out = power_val[0];
		}
		else //Three-phase, by default
		{
			//Update output power
			//Get current injected
			terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
			terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
			terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

			//Update per-unit values
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
			terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
			terminal_current_val_pu[2] = terminal_current_val[2] / I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
			power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
			power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

			VA_Out = power_val[0] + power_val[1] + power_val[2];
		}
	}
	else	//Disconnected somehow
	{
		VA_Out = gld::complex(0.0,0.0);
	}

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Preupdate
STATUS ibr_gfl::pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time)
{
	STATUS stat_val;
	FUNCTIONADDR funadd = nullptr;
	OBJECT *hdr = OBJECTHDR(this);

	//Call the init, since we're here
	stat_val = init_dynamics();

	if (stat_val != SUCCESS)
	{
		gl_error("ibr_gfl failed pre_deltaupdate call");
		/*  TROUBLESHOOT
		While attempting to call the pre_deltaupdate portion of the ibr_gfl code, an error
		was encountered.  Please submit your code and a bug report via the ticketing system.
		*/

		return FAILED;
	}

	//Implies grid-following
	//Copy the trackers - just do a bulk copy (don't care if three-phase or not)
	memcpy(prev_value_IGenerated,value_IGenerated,3*sizeof(gld::complex));

	//Reset the QSTS criterion flag
	deltamode_exit_iteration_met = false;

	//Just return a pass - not sure how we'd fail
	return SUCCESS;
}

//Module-level call
SIMULATIONMODE ibr_gfl::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	double deltat, deltath;
	double mag_diff_val;
	bool proceed_to_qsts = true;	//Starts true - prevent QSTS by exception
	int i;
	int temp_dc_idx;
	STATUS fxn_DC_status;
	OBJECT *temp_DC_obj;

	OBJECT *obj = OBJECTHDR(this);

	SIMULATIONMODE simmode_return_value = SM_EVENT;
	
	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		reset_complex_powerflow_accumulators();

		pull_complex_powerflow_values();
	}

	//Get timestep value
	deltat = (double)dt / (double)DT_SECOND;
	deltath = deltat / 2.0;

	count1 = count1+1;

    if (gl_globaldeltaclock < start_time_igd_delay + igq_hold_time)
    {
    	igd_delay_flag = 0;
    }
    else
    {
    	start_time_igd_delay = gl_globaldeltaclock;
    	igd_delay_flag = 1;
    }

	//Update time tracking variables
	prev_timestamp_dbl = gl_globaldeltaclock;
	prev_time_dbl_IEEE1547 = prev_timestamp_dbl;	//Just update - won't hurt anything if it isn't needed

	//Perform the 1547 update, if enabled
	if (enable_1547_compliance && (iteration_count_val == 0))	//Always just do on the first pass
	{
		//Do the checks
		ieee_1547_delta_return = perform_1547_checks(deltat);
	}

	//Make sure we're active/valid - if we've been tripped/disconnected, don't do an update
	if ((value_MeterStatus == 1) && inverter_1547_status)
	{
		// Check pass
		if (iteration_count_val == 0) // Predictor pass
		{
			//Calculate injection current based on voltage soruce magtinude and angle obtained
			if (parent_is_single_phase) // single phase/split-phase implementation
			{
				//Update output power
				//Get current injected
				terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

				//Update per-unit value
				terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

				power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];

				//Update power output variables, just so we can see what is going on
				VA_Out = power_val[0];

				// Function: Coordinate Tranformation, xy to dq
				ugd_pu[0] = (value_Circuit_V[0].Re() * cos(Angle_PLL[0]) + value_Circuit_V[0].Im() * sin(Angle_PLL[0])) / V_base;
				ugq_pu[0] = (-value_Circuit_V[0].Re() * sin(Angle_PLL[0]) + value_Circuit_V[0].Im() * cos(Angle_PLL[0])) / V_base;
				igd_pu[0] = (terminal_current_val[0].Re() * cos(Angle_PLL[0]) + terminal_current_val[0].Im() * sin(Angle_PLL[0])) / I_base;
				igq_pu[0] = (-terminal_current_val[0].Re() * sin(Angle_PLL[0]) + terminal_current_val[0].Im() * cos(Angle_PLL[0])) / I_base;

				// ugd_pu[i] and ugq_pu[i] are the per-unit values of grid-side voltages in dq frame
				// igd_pu[i] and igq_pu[i] are the per-unit values of grid-side currents in dq frame
				// Angle_PLL[i] is the phase angle of the grid side votlage, which is obtained from the PLL
				// Value_Circuit_V[i] is the voltage of each phase at the grid side
				// terminal_current_val[i] is the current of each phase injected to the grid
				// S_base is the rated capacity
				// I_base is the reted current
				// Function end

				// Function: Phase-Lock_Loop, PLL
				delta_w_PLL[0] = delta_w_PLL_blk[0].getoutput(ugq_pu[0],deltat,PREDICTOR);

				fPLL[0] = (delta_w_PLL[0] + w_ref) / 2.0 / PI;		// frequency measured by PLL
				
				Angle_PLL[0] = Angle_PLL_blk[0].getoutput(delta_w_PLL[0],deltat,PREDICTOR);	// phase angle from PLL
				// delta_w_PLL[i] is the output from the PI controller
				// w_ref is the rated angular frequency, the value is 376.99 rad/s
				// fPLL is the frequency measured by PLL
				// Fuction end


				// Check Pref and Qref, make sure the inverter output S does not exceed S_base, Pref has the priority
				if(Pref > S_base*Smax)
				{
					//Pref = S_base;
					gl_warning("ibr_gfl:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
					/*  TROUBLESHOOT
					The active power Pref for the ibr_gfl is above the rated value.  It has been capped at the rated value.
					*/
				}

				if(Pref < -S_base*Smax)
				{
					//Pref = -S_base;
					gl_warning("ibr_gfl:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
					//Defined above
				}
				//Function end

				// Frequency-watt function enabled
				if (frequency_watt)
				{
					Pref_droop_pu_prev = Pref_droop_pu; // the value of Pref_droop_pu in last simulation step
					f_filter = f_filter_blk.getoutput(fPLL[0],deltat,PREDICTOR);

					if ((f_filter < (f_nominal + db_OF))&&(f_filter > (f_nominal - db_UF)))  // add dead band
					{
						Pref_droop_pu = Pref / S_base;
					}
					else
					{
						Pref_droop_pu = (f_nominal - f_filter) / f_nominal / Rp + Pref / S_base;
					}


					if (Pref_droop_pu > Pref_max)
					{
						Pref_droop_pu = Pref_max;
					}

					if (Pref_droop_pu < Pref_min)
					{
						Pref_droop_pu = Pref_min;
					}

					double power_diff_val = Pref_droop_pu_prev - Pref_droop_pu;

					if (checkRampRate_real)
					{
						if((power_diff_val > 0) && (power_diff_val > rampDownRate_real * deltat))
						{
							Pref_droop_pu = Pref_droop_pu_prev - rampDownRate_real * deltat;
						}
						else if ((power_diff_val < 0) && (-power_diff_val > rampUpRate_real * deltat))
						{
							Pref_droop_pu = Pref_droop_pu_prev + rampUpRate_real * deltat;
						}

						if (Pref_droop_pu > Pref_max)
						{
							Pref_droop_pu = Pref_max;
						}

						if (Pref_droop_pu < Pref_min)
						{
							Pref_droop_pu = Pref_min;
						}
					}
					else
					{
							Pref_droop_pu_filter = Pref_droop_pu_filter_blk.getoutput(Pref_droop_pu,deltat,PREDICTOR);
					}
				}
				// f_filter is the frequency pass through the low pass filter
				// Pref_droop_pu is the power reference from the frequency-watt
				// Pref_droop_pu_filter is the power reference pass through the low pass filter
				// Pref_max and Pref_min are the upper and lower limits of power references
				// Function end

				// Find the limit for Qref, Qref_max and Qref_min
				if(volt_var) //When volt-var is enabled, Q is limited by Qref_max and Qref_min
				{
					if(frequency_watt)  // When frequency-watt is enabled, P is controlled by Pref_droop_pu
					{
						Qref_max = sqrt(Smax*Smax-Pref_droop_pu*Pref_droop_pu);
						Qref_min = -Qref_max;
					}
					else  //When frequency-watt is disabled, P is controlled by Pref
					{
						Qref_max = sqrt(Smax*Smax-(Pref/S_base)*(Pref/S_base));
						Qref_min = -Qref_max;
					}
				}
				else //When volt-var is disabled, Q is limited by Qref
				{
					if(frequency_watt)
					{
						if(Qref > sqrt(S_base*Smax*S_base*Smax - (Pref_droop_pu*S_base)*(Pref_droop_pu*S_base)))
						{
							Qref = sqrt(S_base*Smax*S_base*Smax - (Pref_droop_pu*S_base)*(Pref_droop_pu*S_base));
							gl_warning("ibr_gfl:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							/*  TROUBLESHOOT
							The reactive power Qref for the ibr_gfl is above the rated value.  It has been capped at the rated value minus any active power output.
							*/
						}

						if(Qref < -sqrt(S_base*Smax*S_base*Smax - (Pref_droop_pu*S_base)*(Pref_droop_pu*S_base)))
						{
							Qref = -sqrt(S_base*Smax*S_base*Smax - (Pref_droop_pu*S_base)*(Pref_droop_pu*S_base));
							gl_warning("ibr_gfl:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}
					}
					else
					{
						if(Qref > sqrt(S_base*Smax*S_base*Smax - Pref*Pref))
						{
							Qref = sqrt(S_base*Smax*S_base*Smax - Pref*Pref);
							gl_warning("ibr_gfl:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if(Qref < -sqrt(S_base*Smax*S_base*Smax - Pref*Pref))
						{
							Qref = -sqrt(S_base*Smax*S_base*Smax - Pref*Pref);
							gl_warning("ibr_gfl:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}
					}
				}
				// Function end

				// Volt-var function enabled
				if (volt_var)
				{
					Qref_droop_pu_prev = Qref_droop_pu;

					V_Avg_pu = value_Circuit_V[0].Mag() / V_base;
					V_filter = V_filter_blk.getoutput(V_Avg_pu,deltat,PREDICTOR);

					if ((V_filter < (Vset + db_OV))&&(V_filter > (Vset - db_UV)))  // add dead band
					{
						Qref_droop_pu = Qref / S_base;
					}
					else
					{
						Qref_droop_pu = (Vset - V_filter) / Rq + Qref / S_base;
					}


					if (Qref_droop_pu > Qref_max)
					{
						Qref_droop_pu = Qref_max;
					}

					if (Qref_droop_pu < Qref_min)
					{
						Qref_droop_pu = Qref_min;
					}

					double power_diff_val = Qref_droop_pu_prev - Qref_droop_pu;

					if(checkRampRate_reactive)
					{
						if (power_diff_val > 0 && (power_diff_val > rampDownRate_reactive * deltat))
						{
							Qref_droop_pu = Qref_droop_pu_prev - rampDownRate_reactive * deltat;
						}
						else if ((power_diff_val < 0) && (-power_diff_val > rampUpRate_reactive* deltat))
						{
							Qref_droop_pu = Qref_droop_pu_prev + rampUpRate_reactive* deltat;
						}

						if (Qref_droop_pu > Qref_max)
						{
							Qref_droop_pu = Qref_max;
						}

						if (Qref_droop_pu < Qref_min)
						{
							Qref_droop_pu = Qref_min;
						}
					}
					else
					{
							Qref_droop_pu_filter = Qref_droop_pu_filter_blk.getoutput(Qref_droop_pu,deltat,PREDICTOR);
					}
				}
				// V_Avg_pu is the average value of three phase voltages
				// V_filter is the voltage pass through low pass filter
				// Qref_droop_pu if the Q reference from volt-var
				// Qref_droop_pu_filter is the Q pass through low pass filter
				// Qref_max and Qref_min are the upper and lower limits of Q references
				// Function end

				// Function: Current Control Loop
				if (frequency_watt)
				{
					if (checkRampRate_real)
					{
						igd_ref[0] = Pref_droop_pu / ugd_pu[0];
					}
					else
					{
						igd_ref[0] = Pref_droop_pu_filter / ugd_pu[0];
					}
				}
				else
				{
					// get the current references
					igd_ref[0] = Pref_pu / ugd_pu[0];
				}

				if (volt_var)
				{
					if(checkRampRate_reactive)
					{
						igq_ref[0] = -Qref_droop_pu / ugd_pu[0];
					}
					else
					{
						igq_ref[0] = -Qref_droop_pu_filter / ugd_pu[0];
					}
				}
				else
				{
					igq_ref[0] = -Qref_pu / ugd_pu[0];
				}


				//Add the limits for igd_ref and idq_ref
				if (igd_ref[0] > igd_ref_max)
				{
					igd_ref[0] = igd_ref_max;
				}

				if (igd_ref[0] < igd_ref_min)
				{
					igd_ref[0] = igd_ref_min;
				}

				if (igq_ref[0] > igq_ref_max)
				{
					igq_ref[0] = igq_ref_max;
				}

				if (igq_ref[0] < igq_ref_min)
				{
					igq_ref[0] = igq_ref_min;
				}
				// End of adding limits for igd_ref and idq_ref

				// Low pass filter for current id
				igd_filter_ref[0] = igd_filter_blk[0].getoutput(igd_ref[0],deltat,PREDICTOR);
				igd_filter[0] = igd_filter_ref[0];
				// Low pass filter for current iq
				igq_filter_ref[0] = igq_filter_blk[0].getoutput(igq_ref[0],deltat,PREDICTOR);
				igq_filter[0] = igq_filter_ref[0];


				if (Neg_Curr_Ctrl == 1.0)
				{
					// current loop in d axis
					igd_PI[0] = igd_blk[0].getoutput(igd_ref[0] - igd_pu[0],deltat,PREDICTOR); 		  // output from the PI controller of current loop

					ed_pu[0] = igd_PI[0] + ugd_pu[0] - igq_pu[0] * Xfilter * F_current;						  // the d axis component of internal voltage, Xfilter is per-unit value
					// current loop in q axis
					igq_PI[0] = igq_blk[0].getoutput(igq_ref[0] - igq_pu[0],deltat,PREDICTOR); 			  // output from the PI controller of current loop

					eq_pu[0] = igq_PI[0] + ugq_pu[0] + igd_pu[0] * Xfilter * F_current;						  // the d axis component of internal voltage, Xfilter is per-unit value

					// igd_ref[i] and igq_ref[i] are the current references in dq frame
					// igd_PI[i] and igq_PI[i] are outputs from the current control loops
					// ed_pu[i] and eq_pu[i] are the dq components of the internal voltages
					// kpc and kic are the PI gains of the current loop
					// Function end

					// Function: Coordinate Transformation: dq to xy
					e_source_Re[0] = (ed_pu[0] * cos(Angle_PLL[0]) - eq_pu[0] * sin(Angle_PLL[0])) * V_base;
					e_source_Im[0] = (ed_pu[0] * sin(Angle_PLL[0]) + eq_pu[0] * cos(Angle_PLL[0])) * V_base;
					e_source[0] = gld::complex(e_source_Re[0], e_source_Im[0]);
					value_IGenerated[0] = e_source[0] / (gld::complex(Rfilter, Xfilter) * Z_base); // Thevenin voltage source to Norton current source convertion

					// e_source[i] is the complex value of internal voltage
					// value_IGenerated[i] is the Norton equivalent current source of e_source[i]
					// Rfilter and Xfilter are the per-unit values of inverter filter
				}
				else
				{

					I_source_Re[0] = (igd_filter[0] * cos(Angle_PLL[0]) - igq_filter[0] * sin(Angle_PLL[0])) * I_base;
					I_source_Im[0] = (igd_filter[0] * sin(Angle_PLL[0]) + igq_filter[0] * cos(Angle_PLL[0])) * I_base;
					I_source[0] = gld::complex(I_source_Re[0], I_source_Im[0]);
					value_IGenerated[0] = I_source[0];
					// I_source[0] is the complex value of injected current
					// value_IGenerated[0] is the current injected to the grid
					// Function end
				}

				simmode_return_value = SM_DELTA_ITER; //Reiterate - to get us to corrector pass
			}
			else //Three-phase
			{
				//Update output power
				//Get current injected
				terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
				terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
				terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

				//Update per-unit values
				terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
				terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
				terminal_current_val_pu[2] = terminal_current_val[2] / I_base;

				//Update power output variables, just so we can see what is going on
				power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
				power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
				power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

				VA_Out = power_val[0] + power_val[1] + power_val[2];

				// The following code is only for three phase system
				// VA_OUT.Re() refers to the output active power from the inverter, this should be normalized.
				// S_base is the rated capacity
				// P_out_pu is the per unit value of VA_OUT.Re()
				// Function: Low pass filter of Q
				P_out_pu = VA_Out.Re() / S_base;
				Q_out_pu = VA_Out.Im() / S_base;
				Q_out_pu1 = (value_Circuit_V[0] * ~value_IGenerated[0] + value_Circuit_V[1] * ~value_IGenerated[1] + value_Circuit_V[2] * ~value_IGenerated[2]).Im() / S_base;

				value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
				value_Circuit_V_NS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI))) / 3.0;
				terminal_current_val_PS = (terminal_current_val[0] + terminal_current_val[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + terminal_current_val[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
				terminal_current_val_NS = (terminal_current_val[0] + terminal_current_val[1] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI)) + terminal_current_val[2] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI))) / 3.0;
				VA_Out_PS = gld::complex(3.0, 0) * (value_Circuit_V_PS * ~terminal_current_val_PS);
				VA_Out_NS = gld::complex(3.0, 0) * (value_Circuit_V_NS * ~terminal_current_val_NS);

				ug_pu_PS1 = sqrt(value_Circuit_V_PS.Re()*value_Circuit_V_PS.Re()+value_Circuit_V_PS.Im()*value_Circuit_V_PS.Im()) / V_base;
				ug_pu_NS = sqrt(value_Circuit_V_NS.Re()*value_Circuit_V_NS.Re()+value_Circuit_V_NS.Im()*value_Circuit_V_NS.Im()) / V_base;
				ig_pu_NS = sqrt(terminal_current_val_NS.Re()*terminal_current_val_NS.Re()+terminal_current_val_NS.Im()*terminal_current_val_NS.Im()) / I_base;

				// Function: Coordinate Tranformation, xy to dq
				for (i = 0; i < 3; i++)
				{
					ugd_pu[i] = (value_Circuit_V_PS.Re() * cos(Angle_PLL[i]) + value_Circuit_V_PS.Im() * sin(Angle_PLL[i])) / V_base;
					ugq_pu[i] = (-value_Circuit_V_PS.Re() * sin(Angle_PLL[i]) + value_Circuit_V_PS.Im() * cos(Angle_PLL[i])) / V_base;

					igd_pu[i] = (terminal_current_val[i].Re() * cos(Angle_PLL[i]) + terminal_current_val[i].Im() * sin(Angle_PLL[i])) / I_base;
					igq_pu[i] = (-terminal_current_val[i].Re() * sin(Angle_PLL[i]) + terminal_current_val[i].Im() * cos(Angle_PLL[i])) / I_base;
					igd_ns_pu[i] = (terminal_current_val[i].Re() * cos(-Angle_PLL[i]) + terminal_current_val[i].Im() * sin(-Angle_PLL[i])) / I_base;
					igq_ns_pu[i] = (-terminal_current_val[i].Re() * sin(-Angle_PLL[i]) + terminal_current_val[i].Im() * cos(-Angle_PLL[i])) / I_base;

				}

				P_o_pu = ugd_pu[0]*igd_pu[0] + ugq_pu[0]*igq_pu[0];//
				Q_o_pu = ugq_pu[0]*igd_pu[0] - ugd_pu[0]*igq_pu[0];//
				// ugd_pu[i] and ugq_pu[i] are the per-unit values of grid-side voltages in dq frame
				// igd_pu[i] and igq_pu[i] are the per-unit values of grid-side currents in dq frame
				// Angle_PLL[i] is the phase angle of the grid side votlage, which is obtained from the PLL
				// Value_Circuit_V[i] is the voltage of each phase at the grid side
				// terminal_current_val[i] is the current of each phase injected to the grid
				// S_base is the rated capacity
				// I_base is the reted current
				// Function end


				// Obtain the positive sequence voltage
				value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
				ug_pu_NS = sqrt(value_Circuit_V_NS.Re()*value_Circuit_V_NS.Re()+value_Circuit_V_NS.Im()*value_Circuit_V_NS.Im()) / V_base;
				
				// Positive sequence value of voltage in dq frame
				ugd_pu_PS = (value_Circuit_V_PS.Re() * cos(Angle_PLL[0]) + value_Circuit_V_PS.Im() * sin(Angle_PLL[0])) / V_base;
				ugq_pu_PS = (-value_Circuit_V_PS.Re() * sin(Angle_PLL[0]) + value_Circuit_V_PS.Im() * cos(Angle_PLL[0])) / V_base;
				// Negative sequence value of voltage in dq frame
				ugd_pu_NS = (value_Circuit_V_NS.Re() * cos(-Angle_PLL[i]) + value_Circuit_V_NS.Im() * sin(-Angle_PLL[i])) / V_base;
				ugq_pu_NS = (-value_Circuit_V_NS.Re() * sin(-Angle_PLL[i]) + value_Circuit_V_NS.Im() * cos(-Angle_PLL[i])) / V_base;

				// Function: Phase-Lock_Loop, PLL, only consider positive sequence voltage
				for (i = 0; i < 1; i++)
				{
					delta_w_PLL[i] = delta_w_PLL_blk[i].getoutput(ugq_pu_PS,deltat,PREDICTOR);
					fPLL[i] = (delta_w_PLL[i] + w_ref) / 2.0 / PI;														// frequency measured by PLL
					Angle_PLL[i] = Angle_PLL_blk[i].getoutput(delta_w_PLL[i],deltat,PREDICTOR); 	// phase angle from PLL
				}
				Angle_PLL[1] = Angle_PLL[0] - 2.0 / 3.0 * PI;
				Angle_PLL[2] = Angle_PLL[0] + 2.0 / 3.0 * PI;
				
				fPLL[2] = fPLL[1] = fPLL[0];
				// delta_w_PLL[i] is the output from the PI controller
				// w_ref is the rated angular frequency, the value is 376.99 rad/s
				// fPLL is the frequency measured by PLL
				// Fuction end

				// Check Pref and Qref, make sure the inverter output S does not exceed S_base, Pref has the priority
				if(Pref > S_base*Smax)
				{
					Pref = S_base*Smax;
					gl_warning("ibr_gfl:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
					//Defined above
				}

				if(Pref < -S_base*Smax)
				{
					Pref = -S_base*Smax;
					gl_warning("ibr_gfl:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
					//Defined above
				}
				//Function end

				// Frequency-watt function enabled
				if (frequency_watt)
				{
					Pref_droop_pu_prev = Pref_droop_pu; // the value of Pref_droop_pu in last simulation step
					f_filter = f_filter_blk.getoutput((fPLL[0]+fPLL[1]+fPLL[2])/3.0,deltat,PREDICTOR);

					if ((f_filter < (f_nominal + db_OF))&&(f_filter > (f_nominal - db_UF)))  // add dead band
					{
						Pref_droop_pu = Pref / S_base;
					}
					else
					{
						Pref_droop_pu = (f_nominal - f_filter) / f_nominal / Rp + Pref / S_base;
					}

					double power_diff_val = Pref_droop_pu_prev - Pref_droop_pu;

					if (checkRampRate_real)
					{
						if((power_diff_val > 0) && (power_diff_val > rampDownRate_real * deltat))
						{
							Pref_droop_pu = Pref_droop_pu_prev - rampDownRate_real * deltat;
						}
						else if ((power_diff_val < 0) && (-power_diff_val > rampUpRate_real * deltat))
						{
							Pref_droop_pu = Pref_droop_pu_prev + rampUpRate_real * deltat;
						}

					}
					else
					{
							Pref_droop_pu_filter = Pref_droop_pu_filter_blk.getoutput(Pref_droop_pu,deltat,PREDICTOR);
					}
				}
				// f_filter is the frequency pass through the low pass filter
				// Pref_droop_pu is the power reference from the frequency-watt
				// Pref_droop_pu_filter is the power reference pass through the low pass filter
				// Pref_max and Pref_min are the upper and lower limits of power references
				// Function end

				// Volt-var function enabled
				// V_Avg_pu is the average value of three phase voltages
				// V_filter is the voltage pass through low pass filter
				// Qref_droop_pu if the Q reference from volt-var
				// Qref_droop_pu_filter is the Q pass through low pass filter
				// Qref_max and Qref_min are the upper and lower limits of Q references
				if (volt_var)
				{
					Qref_droop_pu_prev = Qref_droop_pu;
					V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;
					V_filter = V_filter_blk.getoutput(V_Avg_pu,deltat,PREDICTOR);

					if ((V_filter < (Vset + db_OV))&&(V_filter > (Vset - db_UV)))  // add dead band
					{
						Qref_droop_pu = Qref / S_base;
					}
					else
					{
						Qref_droop_pu = (Vset - V_filter) / Rq + Qref / S_base;
					}

					double power_diff_val = Qref_droop_pu_prev - Qref_droop_pu;

					if(checkRampRate_reactive)
					{
						if (power_diff_val > 0 && (power_diff_val > rampDownRate_reactive * deltat))
						{
							Qref_droop_pu = Qref_droop_pu_prev - rampDownRate_reactive * deltat;
						}
						else if ((power_diff_val < 0) && (-power_diff_val > rampUpRate_reactive* deltat))
						{
							Qref_droop_pu = Qref_droop_pu_prev + rampUpRate_reactive* deltat;
						}
					}
					else
					{
							Qref_droop_pu_filter = Qref_droop_pu_filter_blk.getoutput(Qref_droop_pu,deltat,PREDICTOR);
					}
				}
				// Function end

				// Update the limit of Qref_max and Qref_min & Pref_max and Pref_min, which are used to limit the boundaries
				// of active and reactive power commands, considering both enable and disable of frequency-watt and volt-var
				if(volt_var) //When volt-var is enabled, Q is limited by Qref_max and Qref_min
				{
					if(frequency_watt)  // When frequency-watt is enabled, P is controlled by Pref_droop_pu
					{
						if (checkRampRate_real)
						{
							if (Pref_droop_pu > Pref_max)
							{
								Pref_droop_pu = Pref_max;
							}

							if (Pref_droop_pu < Pref_min)
							{
								Pref_droop_pu = Pref_min;
							}
						}
						else
						{
							if (Pref_droop_pu_filter > Pref_max)
							{
								Pref_droop_pu_filter = Pref_max;
							}

							if (Pref_droop_pu_filter < Pref_min)
							{
								Pref_droop_pu_filter = Pref_min;
							}
						}

						Qref_max = sqrt(Smax*Smax-Pref_droop_pu_filter*Pref_droop_pu_filter);
						Qref_min = -Qref_max;
						if(checkRampRate_reactive)
						{
							if (Qref_droop_pu > Qref_max)
							{
								Qref_droop_pu = Qref_max;
							}

							if (Qref_droop_pu < Qref_min)
							{
								Qref_droop_pu = Qref_min;
							}
						}
						else
						{
							if (Qref_droop_pu_filter > Qref_max)
							{
								Qref_droop_pu_filter = Qref_max;
							}

							if (Qref_droop_pu_filter < Qref_min)
							{
								Qref_droop_pu_filter = Qref_min;
							}
						}
					}
					else  //When frequency-watt is disabled, P is controlled by Pref
					{
						if(checkRampRate_reactive)
						{
							if (Qref_droop_pu > Qref_max)
							{
								Qref_droop_pu = Qref_max;
							}

							if (Qref_droop_pu < Qref_min)
							{
								Qref_droop_pu = Qref_min;
							}
							Pref_max = sqrt(S_base*Smax*S_base*Smax - (Qref_droop_pu*S_base)*(Qref_droop_pu*S_base));
						}
						else
						{
							if (Qref_droop_pu_filter > Qref_max)
							{
								Qref_droop_pu_filter = Qref_max;
							}

							if (Qref_droop_pu_filter < Qref_min)
							{
								Qref_droop_pu_filter = Qref_min;
							}
							Pref_max = sqrt(S_base*Smax*S_base*Smax - (Qref_droop_pu_filter*S_base)*(Qref_droop_pu_filter*S_base));
						}

						Pref_min = -Pref_max;
						if (Pref > Pref_max)
						{
							Pref = Pref_max;
						}

						if (Pref < Pref_min)
						{
							Pref = Pref_min;
						}
					}
				}
				else //When volt-var is disabled, Q is limited by Qref
				{
					if(frequency_watt)
					{
						if (checkRampRate_real)
						{
							if (Pref_droop_pu > Pref_max)
							{
								Pref_droop_pu = Pref_max;
							}

							if (Pref_droop_pu < Pref_min)
							{
								Pref_droop_pu = Pref_min;
							}
						}
						else
						{
							if (Pref_droop_pu_filter > Pref_max)
							{
								Pref_droop_pu_filter = Pref_max;
							}

							if (Pref_droop_pu_filter < Pref_min)
							{
								Pref_droop_pu_filter = Pref_min;
							}
						}

						if(Qref > sqrt(Smax*Smax - Pref_droop_pu*Pref_droop_pu)*S_base)
						{
							gl_warning("inverter_dyn:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if(Qref < -sqrt(Smax*Smax - Pref_droop_pu*Pref_droop_pu)*S_base)
						{
							gl_warning("inverter_dyn:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}
						Qref_max = sqrt(Smax*Smax - Pref_droop_pu*Pref_droop_pu)*S_base;
						Qref_min = -Qref_max;
						if (Qref > Qref_max)
						{
							Qref = Qref_max;
						}

						if (Qref < Qref_min)
						{
							Qref =Qref_min;
						}
					}
					else
					{
						if(Qref > sqrt(S_base*Smax*S_base*Smax - Pref*Pref))
						{
							gl_warning("inverter_dyn:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if(Qref < -sqrt(S_base*Smax*S_base*Smax - Pref*Pref))
						{
							gl_warning("inverter_dyn:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if (Pref > Pref_max*S_base)
						{
							Pref = Pref_max*S_base;
						}

						if (Pref < Pref_min*S_base)
						{
							Pref = Pref_min*S_base;
						}
						Qref_max = sqrt(S_base*Smax*S_base*Smax - Pref*Pref);
						Qref_min = -Qref_max;
						if (Qref > Qref_max)
						{
							Qref = Qref_max;
						}

						if (Qref < Qref_min)
						{
							Qref =Qref_min;
						}
					}
				}
				// Function end

				// Function: Current Control Loop
				for (i = 0; i < 3; i++)
				{
					Xi_filter[i] = 0.9073*(2.0 * PI * f_nominal)*CFilt*ug_pu_PS[i]*(fPLL[i]/f_nominal);
					Qcap[i] = Xi_filter[i] * ug_pu_PS[i];

					ug_pu_PS_old[i] = ug_pu_PS[i];
					ug_pu_PS[i] = sqrt(ugd_pu_PS*ugd_pu_PS+ugq_pu_PS*ugq_pu_PS);
					if (frequency_watt)
					{
						if (checkRampRate_real)
						{
							igd_ref[i] = Pref_droop_pu / ugd_pu_PS;
						}
						else
						{
							igd_cmd[i] = Pref_droop_pu_filter / ug_pu_PS[i];
						}
					}
					else
					{
						igd_cmd[i] = Pref / S_base / ug_pu_PS[i];
					}

					if (!checkRampRate_real)
					{									
						Test2 = Pref / S_base / ug_pu_PS[0];
						if (ug_pu_PS_risingedge[i] == 1)
						{
							if (gl_globaldeltaclock < start_time_ug_risingedge[i] + igq_hold_time)
							{
								ig_mag_pu_NS = 0;
							}
							else
							{
								ig_mag_pu_NS = ig_pu_NS_edge;
							}
						}
						else
						{
							ig_mag_pu_NS = ig_pu_NS;
						}
						Icmax = Smax - ig_mag_pu_NS;
						Ipmax[i] = Icmax;//Smax/ug_pu_PS[i];
						if (Ipmax[i]<=0)
							{Ipmax[i] = 0;}
						if (Ipmax[i]>=Ipmax_limit)
							{Ipmax[i] = Ipmax_limit;}

						if (sqrt(Icmax*Icmax-igq_cmd[i]*igq_cmd[i]) <= 0)
							{IpUL[i] = 0;}
						else if (sqrt(Icmax*Icmax-igq_cmd[i]*igq_cmd[i]) >= Ipmax[i])
							{IpUL[i] = Ipmax[i];}
						else
							{IpUL[i] = sqrt(Icmax*Icmax-igq_cmd[i]*igq_cmd[i]);}


						if (IpUL[i]<=0)
							{IpUL[i] = 0;}
						if (IpUL[i]>=Ipmax_limit)
							{IpUL[i] = Ipmax_limit;}

						if (igd_cmd[i] < -1*IpUL[i])
							{igd_cmd[i] = -1*IpUL[i];}
						else if (igd_cmd[i] > 1*IpUL[i])
							{igd_cmd[i] = 1*IpUL[i];}

						// looping till required time is not achieved
						if (igd_delay_flag == 0.0)
						{
							count[i] = count[i] +1;
						}
						else
						{
							count[i] = 0;
							// get the current references
							igd_ref_old[i] = igd_cmd[i];
						}
						igd_ref[i] = igd_cmd[i];
					}

					if (volt_var)
					{
						if(checkRampRate_reactive)
						{
							igq_ref[i] = -Qref_droop_pu / ug_pu_PS[i];
						}
						else
						{
							igq_ref[i] = -Qref_droop_pu_filter / ug_pu_PS[i];
						}
						igq_cmd[i] = igq_ref[i];
					}
					else
					{
						if (ug_pu_PS[i] <= ug_lvrt_threshold)
						{
							if (ug_pu_NS >= ug_asym_neg_threshold)
							{
								if ((ug_pu_PS[i] - 1) < ug_asym_threshold1)
								{
									Qref_pu = Qref_asym_threshold1_lim;
								}
								else if ((ug_pu_PS[i]-1) < ug_asym_threshold2)
								{
									Qref_pu = Qref_asym_threshold2_lim;//
								}
								else if ((ug_pu_PS[i]-1) < ug_asym_threshold3)
								{
									Qref_pu = Qref_asym_threshold3_k * (ug_pu_PS[i]-1) + Qref_asym_threshold3_b;
								}
								else if ((ug_pu_PS[i]-1) < ug_asym_threshold4)
								{
									Qref_pu = Qref_asym_threshold4_k * (ug_pu_PS[i]-1) + Qref_asym_threshold4_b;
								}
								else if ((ug_pu_PS[i]-1) < ug_asym_threshold5)
								{
									Qref_pu = Qref_asym_threshold5_k * (ug_pu_PS[i]-1) + Qref_asym_threshold5_b;
								}
								else if ((ug_pu_PS[i]-1) < ug_asym_threshold6)
								{
									Qref_pu = Qref_asym_threshold6_k * (ug_pu_PS[i]-1) + Qref_asym_threshold6_b;
								}
								else
								{
									Qref_pu = Qref_asym_threshold7_lim;
								}

								if (Qref_pu <= Qref_asym_LL)
									{Qref_pu = Qref_asym_LL;}
								if (Qref_pu >= Qref_asym_UL)
									{Qref_pu = Qref_asym_UL;}
							}
							else
							{
								if ((ug_pu_PS[i] - 1) < ug_sym_threshold1)
								{
									Qref_pu = Qref_sym_threshold1_lim;
								}
								else if ((ug_pu_PS[i]-1) < ug_sym_threshold2)
								{
									Qref_pu = Qref_sym_threshold2_k* (ug_pu_PS[i]-1) + Qref_sym_threshold2_b;
								}
								else if ((ug_pu_PS[i]-1) < ug_sym_threshold3)
								{
									Qref_pu = Qref_sym_threshold3_k * (ug_pu_PS[i]-1) + Qref_sym_threshold3_b;
								}
								else if ((ug_pu_PS[i]-1) < ug_sym_threshold4)
								{
									Qref_pu = Qref_sym_threshold4_k * (ug_pu_PS[i]-1) + Qref_sym_threshold4_b;
								}
								else if ((ug_pu_PS[i]-1) < ug_sym_threshold5)
								{
									Qref_pu = Qref_sym_threshold5_k * (ug_pu_PS[i]-1) + Qref_sym_threshold5_b;
								}
								else
								{
									Qref_pu = Qref_sym_threshold6_lim;
								}

								if (Qref_pu <= Qref_sym_LL)
									{Qref_pu = Qref_sym_LL;}
								if (Qref_pu >= Qref_sym_UL)
									{Qref_pu = Qref_sym_UL;}
							}
						}
						else
						{
							Qref_pu = Qref / S_base;
						}

						if (Qref_pu > Qref_max)
						{
							Qref_pu = Qref_max;
						}

						if (Qref_pu < Qref_min)
						{
							Qref_pu = Qref_min;
						}

						Qerr[i] = Qref_pu - Q_out_pu;
						if (ug_pu_PS[i] <= ug_lvrt_threshold)
							{Vref_pu[i] = 1.0;}
						else
						{
							Qreg_ctrl_out[i] = Qerr[i]*kqp + Qreg_ctrl_blk[i].getoutput(Qerr[i],deltat,PREDICTOR);
							Vref_pu[i] = 1.0 + Qreg_ctrl_out[i];
						}
						Verr[i] = Vref_pu[i] - ug_pu_PS[i];
						if (ug_pu_PS[i] <= ug_lvrt_threshold)
							{igq_cmd[i] = -1.0*Qref_pu;}
						else
						{
							igq_cmd[i] = -kvp*Verr[i] - Vreg_ctrl_blk[i].getoutput(Verr[i],deltat,PREDICTOR);
						}

						if ( abs(ug_pu_PS_old[i] - ug_pu_PS[i]) >0.05 )
							{
								ug_pu_PS_risingedge[i] = 1;
								// Storing start time
								start_time_ug_risingedge[i] = gl_globaldeltaclock;
								ig_pu_NS_edge = ig_pu_NS;
							}

						if (ug_pu_PS_risingedge[i] == 1)
						{
							if (gl_globaldeltaclock < start_time_ug_risingedge[i] + igq_hold_time)
							{
								igq_ref[i] = igq_hold_value; //
							}
							else
							{
								ug_pu_PS_risingedge[i] = 0;
								igq_ref[i] = igq_cmd[i];
							}
						}
						else
						{
							igq_ref[i] = igq_cmd[i];
						}

					}

					//Add the limits for igd_ref and idq_ref
					if (igd_ref[i] > igd_ref_max)
					{
						igd_ref[i] = igd_ref_max;
					}

					if (igd_ref[i] < igd_ref_min)
					{
						igd_ref[i] = igd_ref_min;
					}

					if (igq_ref[i] > igq_ref_max)
					{
						igq_ref[i] = igq_ref_max;
					}

					if (igq_ref[i] < igq_ref_min)
					{
						igq_ref[i] = igq_ref_min;
					}
					// End of adding limits for igd_ref and idq_ref

					// Low pass filter for current id
					igd_filter_ref[i] = igd_filter_blk[i].getoutput(igd_ref[i],deltat,PREDICTOR);
					igd_filter[i] = igd_filter_ref[i];
					// Low pass filter for current iq
					igq_filter_ref[i] = igq_filter_blk[i].getoutput(igq_ref[i],deltat,PREDICTOR);
					igq_filter[i] = igq_filter_ref[i];

					// Negative sequence control
					igd_ns_filter[i] = 0;
					if (value_Circuit_V_NS.Re() <0 )
					{
						igq_ns_filter[i] = -ug_pu_NS*2;
					}
					else
					{
						igq_ns_filter[i] = ug_pu_NS*2;
					}

					// igd_ref[i] and igq_ref[i] are the current references in dq frame
					// igd_filter[i] and igq_filter[i] are the currents
					// Function end

					// Function: Coordinate Transformation: dq to xy
					I_source_Re[i] = (igd_filter[i] * cos(Angle_PLL[i]) - igq_filter[i] * sin(Angle_PLL[i])) * I_base;
					I_source_ns_Re[i] =  (igd_ns_filter[i] * cos(-Angle_PLL[i]- PI) - igq_ns_filter[i] * sin(-Angle_PLL[i]- PI)) * I_base;
					I_source_Im[i] = (igd_filter[i] * sin(Angle_PLL[i]) + igq_filter[i] * cos(Angle_PLL[i])) * I_base;
					I_source_ns_Im[i] =  (igd_ns_filter[i] * sin(-Angle_PLL[i]- PI) + igq_ns_filter[i] * cos(-Angle_PLL[i]- PI)) * I_base;
					if (Neg_Curr_Ctrl == 1.0)
					{
						I_source[i] = gld::complex(I_source_Re[i]+I_source_ns_Re[i], I_source_Im[i]+I_source_ns_Im[i]);
					}
					else
					{
						I_source[i] = gld::complex(I_source_Re[i], I_source_Im[i]);
					}
					value_IGenerated[i] = I_source[i];
					// I_source[i] is the complex value of injected current
					// value_IGenerated[i] is the current injected to the grid
					// Function end
				}

				simmode_return_value = SM_DELTA_ITER; //Reiterate - to get us to corrector pass

			}	  // end of three phase code of grid-following
		}
		else if (iteration_count_val == 1) // Corrector pass
		{
			//Caluclate injection current based on voltage soruce magtinude and angle obtained
			if (parent_is_single_phase) // single phase/split-phase implementation
			{
				//Update output power
				//Get current injected
				terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

				//Update per-unit values
				terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

				power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];

				//Update power output variables, just so we can see what is going on
				VA_Out = power_val[0];

				// Function: Coordinate Tranformation, xy to dq
				ugd_pu[0] = (value_Circuit_V[0].Re() * cos(Angle_PLL[0]) + value_Circuit_V[0].Im() * sin(Angle_PLL[0])) / V_base;
				ugq_pu[0] = (-value_Circuit_V[0].Re() * sin(Angle_PLL[0]) + value_Circuit_V[0].Im() * cos(Angle_PLL[0])) / V_base;
				igd_pu[0] = (terminal_current_val[0].Re() * cos(Angle_PLL[0]) + terminal_current_val[0].Im() * sin(Angle_PLL[0])) / I_base;
				igq_pu[0] = (-terminal_current_val[0].Re() * sin(Angle_PLL[0]) + terminal_current_val[0].Im() * cos(Angle_PLL[0])) / I_base;

				// ugd_pu[i] and ugq_pu[i] are the per-unit values of grid-side voltages in dq frame
				// igd_pu[i] and igq_pu[i] are the per-unit values of grid-side currents in dq frame
				// Angle_PLL[i] is the phase angle of the grid side votlage, which is obtained from the PLL
				// Value_Circuit_V[i] is the voltage of each phase at the grid side
				// terminal_current_val[i] is the current of each phase injected to the grid
				// S_base is the rated capacity
				// I_base is the rated current
				// Function end

				// Function: Phase-Lock_Loop, PLL
				delta_w_PLL[0] = delta_w_PLL_blk[0].getoutput(ugq_pu[0],deltat,CORRECTOR);

				fPLL[0] = (delta_w_PLL[0] + w_ref) / 2.0 / PI;			  // frequency measured by PLL
				Angle_PLL[0] = Angle_PLL_blk[0].getoutput(delta_w_PLL[0],deltat,CORRECTOR); // phase angle from PLL

				// delta_w_PLL[i] is the output from the PI controller
				// w_ref is the rated angular frequency, the value is 376.99 rad/s
				// fPLL is the frequency measured by PLL
				// Fuction end

				// Check Pref and Qref, make sure the inverter output S does not exceed S_base, Pref has the priority
				if(Pref > S_base*Smax)
				{
					//Pref = S_base;
					gl_warning("ibr_gfl:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
					//Defined above
				}

				if(Pref < -S_base*Smax)
				{
					//Pref = -S_base;
					gl_warning("ibr_gfl:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
					//Defined above
				}

				// Frequency-watt function enabled
				if (frequency_watt)
				{
					f_filter = f_filter_blk.getoutput(fPLL[0],deltat,CORRECTOR);

					if ((f_filter < (f_nominal + db_OF))&&(f_filter > (f_nominal - db_UF)))  // add dead band
					{
						Pref_droop_pu = Pref / S_base;
					}
					else
					{
						Pref_droop_pu = (f_nominal - f_filter) / f_nominal / Rp + Pref / S_base;
					}

					if (Pref_droop_pu > Pref_max)
					{
						Pref_droop_pu = Pref_max;
					}

					if (Pref_droop_pu < Pref_min)
					{
						Pref_droop_pu = Pref_min;
					}

					double power_diff_val = Pref_droop_pu_prev - Pref_droop_pu;

					if (checkRampRate_real)
					{
						if((power_diff_val > 0) && (power_diff_val > rampDownRate_real * deltat))
						{
							Pref_droop_pu = Pref_droop_pu_prev - rampDownRate_real * deltat;
						}
						else if ((power_diff_val < 0) && (-power_diff_val > rampUpRate_real * deltat))
						{
							Pref_droop_pu = Pref_droop_pu_prev + rampUpRate_real * deltat;
						}

						if (Pref_droop_pu > Pref_max)
						{
							Pref_droop_pu = Pref_max;
						}

						if (Pref_droop_pu < Pref_min)
						{
							Pref_droop_pu = Pref_min;
						}
					}
					else
					{
							Pref_droop_pu_filter = Pref_droop_pu_filter_blk.getoutput(Pref_droop_pu,deltat,CORRECTOR);
					}
				}
				// f_filter is the frequency pass through the low pass filter
				// Pref_droop_pu is the power reference from the frequency-watt
				// Pref_droop_pu_filter is the power reference pass through the low pass filter
				// Pref_max and Pref_min are the upper and lower limits of power references
				// Function end


				// Find the limit for Qref, Qref_max and Qref_min
				if(volt_var) //When volt-var is enabled, Q is limited by Qref_max and Qref_min
				{
					if(frequency_watt)  // When frequency-watt is enabled, P is controlled by Pref_droop_pu
					{
						Qref_max = sqrt(Smax*Smax-Pref_droop_pu*Pref_droop_pu);
						Qref_min = -Qref_max;
					}
					else  //When frequency-watt is disabled, P is controlled by Pref
					{
						Qref_max = sqrt(Smax*Smax-(Pref/S_base)*(Pref/S_base));
						Qref_min = -Qref_max;
					}
				}
				else //When volt-var is disabled, Q is limited by Qref
				{
					if(frequency_watt)
					{
						if(Qref > sqrt(S_base*Smax*S_base*Smax - (Pref_droop_pu*S_base)*(Pref_droop_pu*S_base)))
						{
							Qref = sqrt(S_base*Smax*S_base*Smax - Pref*Pref);
							gl_warning("ibr_gfl:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if(Qref < -sqrt(S_base*Smax*S_base*Smax - (Pref_droop_pu*S_base)*(Pref_droop_pu*S_base)))
						{
							Qref = -sqrt(S_base*Smax*S_base*Smax - Pref*Pref);
							gl_warning("ibr_gfl:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}
					}
					else
					{
						if(Qref > sqrt(S_base*Smax*S_base*Smax - Pref*Pref))
						{
							Qref = sqrt(S_base*Smax*S_base*Smax - Pref*Pref);
							gl_warning("ibr_gfl:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if(Qref < -sqrt(S_base*Smax*S_base*Smax - Pref*Pref))
						{
							Qref = -sqrt(S_base*Smax*S_base*Smax - Pref*Pref);
							gl_warning("ibr_gfl:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}
					}
				}
				// Function end

				// Volt-var function enabled
				if (volt_var)
				{
					V_Avg_pu = value_Circuit_V[0].Mag() / V_base;
					V_filter = V_filter_blk.getoutput(V_Avg_pu,deltat,CORRECTOR);

					if ((V_filter < (Vset + db_OV))&&(V_filter > (Vset - db_UV)))  // add dead band
					{
						Qref_droop_pu = Qref / S_base;
					}
					else
					{
						Qref_droop_pu = (Vset - V_filter) / Rq + Qref / S_base;
					}

					if (Qref_droop_pu > Qref_max)
					{
						Qref_droop_pu = Qref_max;
					}

					if (Qref_droop_pu < Qref_min)
					{
						Qref_droop_pu = Qref_min;
					}

					double power_diff_val = Qref_droop_pu_prev - Qref_droop_pu;
					if(checkRampRate_reactive)
					{
						if (power_diff_val > 0 && (power_diff_val > rampDownRate_reactive * deltat))
						{
							Qref_droop_pu = Qref_droop_pu_prev - rampDownRate_reactive * deltat;
						}
						else if ((power_diff_val < 0) && (-power_diff_val > rampUpRate_reactive* deltat))
						{
							Qref_droop_pu = Qref_droop_pu_prev + rampUpRate_reactive* deltat;
						}

						if (Qref_droop_pu > Qref_max)
						{
							Qref_droop_pu = Qref_max;
						}

						if (Qref_droop_pu < Qref_min)
						{
							Qref_droop_pu = Qref_min;
						}
					}
					else
					{
							Qref_droop_pu_filter = Qref_droop_pu_filter_blk.getoutput(Qref_droop_pu,deltat,CORRECTOR);
					}
				}
				// V_Avg_pu is the average value of three phase voltages
				// V_filter is the voltage pass through low pass filter
				// Qref_droop_pu if the Q reference from volt-var
				// Qref_droop_pu_filter is the Q pass through low pass filter
				// Qref_max and Qref_min are the upper and lower limits of Q references
				// Function end

				// Function: Current Control Loop
				if (frequency_watt)
				{
					if (checkRampRate_real)
					{
						igd_ref[0] = Pref_droop_pu / ugd_pu[0];
					}
					else
					{
						igd_ref[0] = Pref_droop_pu_filter / ugd_pu[0];
					}
				}
				else
				{
					// get the current references
					igd_ref[0] = Pref / S_base / ugd_pu[0];
				}

				if (volt_var)
				{
					if(checkRampRate_reactive)
					{
						igq_ref[0] = -Qref_droop_pu / ugd_pu[0];
					}
					else
					{
						igq_ref[0] = -Qref_droop_pu_filter / ugd_pu[0];
					}
				}
				else
				{
					igq_ref[0] = -Qref / S_base / ugd_pu[0];
				}


				//Add the limits for igd_ref and idq_ref
				if (igd_ref[0] > igd_ref_max)
				{
					igd_ref[0] = igd_ref_max;
				}

				if (igd_ref[0] < igd_ref_min)
				{
					igd_ref[0] = igd_ref_min;
				}

				if (igq_ref[0] > igq_ref_max)
				{
					igq_ref[0] = igq_ref_max;
				}

				if (igq_ref[0] < igq_ref_min)
				{
					igq_ref[0] = igq_ref_min;
				}
				// End of adding limits for igd_ref and idq_ref

				// Low pass filter for current id
				igd_filter_ref[0] = igd_filter_blk[0].getoutput(igd_ref[0],deltat,CORRECTOR);
				igd_filter[0] = igd_filter_ref[0];
				// Low pass filter for current iq
				igq_filter_ref[0] = igq_filter_blk[0].getoutput(igq_ref[0],deltat,CORRECTOR);
				igq_filter[0] = igq_filter_ref[0];

				igd_ns_filter[0] = 0; //-0.3863;
				igq_ns_filter[0] = 0;

				// igd_ref[0] and igq_ref[0] are the current references in dq frame
				// igd_filter[0] and igq_filter[0] are the currents

				if (Neg_Curr_Ctrl == 1.0)
				{
					// current loop in d axis
					igd_PI[0] = igd_blk[0].getoutput(igd_ref[0] - igd_pu[0],deltat,CORRECTOR); 		  // output from the PI controller of current loop

					ed_pu[0] = igd_PI[0] + ugd_pu[0] - igq_pu[0] * Xfilter * F_current;															  // the d axis component of internal voltage, Xfilter is per-unit value
					// current loop in q axis
					igq_PI[0] = igq_blk[0].getoutput(igq_ref[0] - igq_pu[0],deltat,CORRECTOR); 			  // output from the PI controller of current loop

					eq_pu[0] = igq_PI[0] + ugq_pu[0] + igd_pu[0] * Xfilter * F_current;															  // the d axis component of internal voltage, Xfilter is per-unit value

					// igd_ref[i] and igq_ref[i] are the current references in dq frame
					// igd_PI[i] and igq_PI[i] are outputs from the current control loops
					// ed_pu[i] and eq_pu[i] are the dq components of the internal voltages
					// kpc and kic are the PI gains of the current loop
					// Function end

					// Function: Coordinate Transformation: dq to xy
					e_source_Re[0] = (ed_pu[0] * cos(Angle_PLL[0]) - eq_pu[0] * sin(Angle_PLL[0])) * V_base;
					e_source_Im[0] = (ed_pu[0] * sin(Angle_PLL[0]) + eq_pu[0] * cos(Angle_PLL[0])) * V_base;
					e_source[0] = gld::complex(e_source_Re[0], e_source_Im[0]);
					value_IGenerated[0] = e_source[0] / (gld::complex(Rfilter, Xfilter) * Z_base); // Thevenin voltage source to Norton current source convertion

					// e_source[i] is the complex value of internal voltage
					// value_IGenerated[i] is the Norton equivalent current source of e_source[i]
					// Rfilter and Xfilter are the per-unit values of inverter filter
					// Function end
				}
				else
				{
					I_source_Re[0] = (igd_filter[0] * cos(Angle_PLL[0]) - igq_filter[0] * sin(Angle_PLL[0])) * I_base;// + (igd_ns_filter[0] * cos(-Angle_PLL[0]) - igq_ns_filter[0] * sin(-Angle_PLL[0])) * I_base;
					I_source_Im[0] = (igd_filter[0] * sin(Angle_PLL[0]) + igq_filter[0] * cos(Angle_PLL[0])) * I_base;// + (igd_ns_filter[0] * sin(-Angle_PLL[0]) + igq_ns_filter[0] * cos(-Angle_PLL[0])) * I_base;
					I_source[0] = gld::complex(I_source_Re[0], I_source_Im[0]);
					value_IGenerated[0] = I_source[0];
					// I_source[0] is the complex value of injected current
					// value_IGenerated[0] is the current injected to the grid
					// Function end
				}

				//Compute a difference
				mag_diff_val = (value_IGenerated[0]-prev_value_IGenerated[0]).Mag();

				//Update tracker
				prev_value_IGenerated[0] = value_IGenerated[0];
				
				//Get the "convergence" test to see if we can exit to QSTS
				if (mag_diff_val > GridFollowing_curr_convergence_criterion)
				{
					//Set return mode
					simmode_return_value = SM_DELTA;

					//Set flag
					deltamode_exit_iteration_met = false;
				}
				else
				{
					//Check and see if we met our iteration criterion (due to how sequencing happens)
					if (deltamode_exit_iteration_met)
					{
						simmode_return_value = SM_EVENT;
					}
					else
					{
						//Reiterate once
						simmode_return_value = SM_DELTA;

						//Set the flag
						deltamode_exit_iteration_met = true;
					}
				}
			}
			else //Three-phase
			{
				//Update output power
				//Get current injected
				terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
				terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
				terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

				//Update per-unit values
				terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
				terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
				terminal_current_val_pu[2] = terminal_current_val[2] / I_base;

				//Update power output variables, just so we can see what is going on
				power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
				power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
				power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

				VA_Out = power_val[0] + power_val[1] + power_val[2];

				// The following code is only for three phase system
				// VA_OUT.Re() refers to the output active power from the inverter, this should be normalized.
				// S_base is the rated capacity
				// P_out_pu is the per unit value of VA_OUT.Re()
				// Function: Low pass filter of Q
				P_out_pu = VA_Out.Re() / S_base;
				Q_out_pu = VA_Out.Im() / S_base;
				Q_out_pu1 = (value_Circuit_V[0] * ~value_IGenerated[0] + value_Circuit_V[1] * ~value_IGenerated[1] + value_Circuit_V[2] * ~value_IGenerated[2]).Im() / S_base;

				value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
				value_Circuit_V_NS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI))) / 3.0;

				terminal_current_val_NS = (terminal_current_val[0] + terminal_current_val[1] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI)) + terminal_current_val[2] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI))) / 3.0;

				ug_pu_PS1 = sqrt(value_Circuit_V_PS.Re()*value_Circuit_V_PS.Re()+value_Circuit_V_PS.Im()*value_Circuit_V_PS.Im()) / V_base;
				ug_pu_NS = sqrt(value_Circuit_V_NS.Re()*value_Circuit_V_NS.Re()+value_Circuit_V_NS.Im()*value_Circuit_V_NS.Im()) / V_base;
				ig_pu_NS = sqrt(terminal_current_val_NS.Re()*terminal_current_val_NS.Re()+terminal_current_val_NS.Im()*terminal_current_val_NS.Im()) / I_base;

				// Function: Coordinate Tranformation, xy to dq
				for (i = 0; i < 3; i++)
				{
					ugd_pu[i] = (value_Circuit_V_PS.Re() * cos(Angle_PLL[i]) + value_Circuit_V_PS.Im() * sin(Angle_PLL[i])) / V_base;
					ugq_pu[i] = (-value_Circuit_V_PS.Re() * sin(Angle_PLL[i]) + value_Circuit_V_PS.Im() * cos(Angle_PLL[i])) / V_base;

					igd_pu[i] = (terminal_current_val[i].Re() * cos(Angle_PLL[i]) + terminal_current_val[i].Im() * sin(Angle_PLL[i])) / I_base;
					igq_pu[i] = (-terminal_current_val[i].Re() * sin(Angle_PLL[i]) + terminal_current_val[i].Im() * cos(Angle_PLL[i])) / I_base;
					igd_ns_pu[i] = (terminal_current_val[i].Re() * cos(-Angle_PLL[i]) + terminal_current_val[i].Im() * sin(-Angle_PLL[i])) / I_base;
					igq_ns_pu[i] = (-terminal_current_val[i].Re() * sin(-Angle_PLL[i]) + terminal_current_val[i].Im() * cos(-Angle_PLL[i])) / I_base;
				}

				P_o_pu = ugd_pu[0]*igd_pu[0] + ugq_pu[0]*igq_pu[0];//
				Q_o_pu = ugq_pu[0]*igd_pu[0] - ugd_pu[0]*igq_pu[0];//
				// ugd_pu[i] and ugq_pu[i] are the per-unit values of grid-side voltages in dq frame
				// igd_pu[i] and igq_pu[i] are the per-unit values of grid-side currents in dq frame

				terminal_current_val_PS = (terminal_current_val[0] + terminal_current_val[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + terminal_current_val[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
				terminal_current_val_NS = (terminal_current_val[0] + terminal_current_val[1] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI)) + terminal_current_val[2] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI))) / 3.0;
				VA_Out_PS = gld::complex(3.0, 0) * (value_Circuit_V_PS * ~terminal_current_val_PS);
				VA_Out_NS = gld::complex(3.0, 0) * (value_Circuit_V_NS * ~terminal_current_val_NS);

				// Angle_PLL[i] is the phase angle of the grid side votlage, which is obtained from the PLL
				// Value_Circuit_V[i] is the voltage of each phase at the grid side
				// terminal_current_val[i] is the current of each phase injected to the grid
				// S_base is the rated capacity
				// I_base is the rated current
				// Function end

				// Obtain the positive sequence voltage
				ug_pu_NS = sqrt(value_Circuit_V_NS.Re()*value_Circuit_V_NS.Re()+value_Circuit_V_NS.Im()*value_Circuit_V_NS.Im()) / V_base;
				// Positive sequence value of voltage in dq frame
				ugd_pu_PS = (value_Circuit_V_PS.Re() * cos(Angle_PLL[0]) + value_Circuit_V_PS.Im() * sin(Angle_PLL[0])) / V_base;
				ugq_pu_PS = (-value_Circuit_V_PS.Re() * sin(Angle_PLL[0]) + value_Circuit_V_PS.Im() * cos(Angle_PLL[0])) / V_base;
				// Negative sequence value of voltage in dq frame
				ugd_pu_NS = (value_Circuit_V_NS.Re() * cos(-Angle_PLL[i]) + value_Circuit_V_NS.Im() * sin(-Angle_PLL[i])) / V_base;
				ugq_pu_NS = (-value_Circuit_V_NS.Re() * sin(-Angle_PLL[i]) + value_Circuit_V_NS.Im() * cos(-Angle_PLL[i])) / V_base;

				// Function: Phase-Lock_Loop, PLL, only consider the positive sequence voltage
				for (i = 0; i < 1; i++)
				{
					delta_w_PLL[i] = delta_w_PLL_blk[i].getoutput(ugq_pu_PS,deltat,CORRECTOR);
					fPLL[i] = (delta_w_PLL[i] + w_ref) / 2.0 / PI; 	  // frequency measured by PLL
					Angle_PLL[i] = Angle_PLL_blk[i].getoutput(delta_w_PLL[i],deltat,CORRECTOR);
				}
				Angle_PLL[1] = Angle_PLL[0] - 2.0 / 3.0 * PI;
				Angle_PLL[2] = Angle_PLL[0] + 2.0 / 3.0 * PI;
				fPLL[2] = fPLL[1] = fPLL[0];

				// delta_w_PLL[i] is the output from the PI controller
				// w_ref is the rated angular frequency, the value is 376.99 rad/s
				// fPLL is the frequency measured by PLL
				// Fuction end

				// Check Pref and Qref, make sure the inverter output S does not exceed S_base, Pref has the priority
				if(Pref > S_base)
				{
					Pref = S_base;
					gl_warning("ibr_gfl:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
					//Defined above
				}

				if(Pref < -S_base)
				{
					Pref = -S_base;
					gl_warning("ibr_gfl:%d %s - The dispatched active power is larger than the rated apparent power, the output active power is capped at the rated apparent power", obj->id, (obj->name ? obj->name : "Unnamed"));
					//Defined above
				}
				//Function end

				// Frequency-watt function enabled
				if (frequency_watt)
				{
					f_filter = f_filter_blk.getoutput((fPLL[0]+fPLL[1]+fPLL[2])/3.0,deltat,CORRECTOR);
					if ((f_filter < (f_nominal + db_OF))&&(f_filter > (f_nominal - db_UF)))  // add dead band
					{
						Pref_droop_pu = Pref / S_base;
					}
					else
					{
						Pref_droop_pu = (f_nominal - f_filter) / f_nominal / Rp + Pref / S_base;
					}

					double power_diff_val = Pref_droop_pu_prev - Pref_droop_pu;

					if (checkRampRate_real)
					{
						if((power_diff_val > 0) && (power_diff_val > rampDownRate_real * deltat))
						{
							Pref_droop_pu = Pref_droop_pu_prev - rampDownRate_real * deltat;
						}
						else if ((power_diff_val < 0) && (-power_diff_val > rampUpRate_real * deltat))
						{
							Pref_droop_pu = Pref_droop_pu_prev + rampUpRate_real * deltat;
						}

					}
					else
					{
								Pref_droop_pu_filter = Pref_droop_pu_filter_blk.getoutput(Pref_droop_pu,deltat,CORRECTOR);
					}
				}
				// f_filter is the frequency pass through the low pass filter
				// Pref_droop_pu is the power reference from the frequency-watt
				// Pref_droop_pu_filter is the power reference pass through the low pass filte
				// Pref_max and Pref_min are the upper and lower limits of power references
				// Function end

				// Volt-var function enabled
				if (volt_var)
				{
					V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;
					V_filter = V_filter_blk.getoutput(V_Avg_pu,deltat,CORRECTOR);

					if ((V_filter < (Vset + db_OV))&&(V_filter > (Vset - db_UV)))  // add dead band
					{
						Qref_droop_pu = Qref / S_base;
					}
					else
					{
						Qref_droop_pu = (Vset - V_filter) / Rq + Qref / S_base;
					}

					double power_diff_val = Qref_droop_pu_prev - Qref_droop_pu;

					if(checkRampRate_reactive)
					{
						if (power_diff_val > 0 && (power_diff_val > rampDownRate_reactive * deltat))
						{
							Qref_droop_pu = Qref_droop_pu_prev - rampDownRate_reactive * deltat;
						}
						else if ((power_diff_val < 0) && (-power_diff_val > rampUpRate_reactive* deltat))
						{
							Qref_droop_pu = Qref_droop_pu_prev + rampUpRate_reactive* deltat;
						}

					}
					else
					{
						Qref_droop_pu_filter = Qref_droop_pu_filter_blk.getoutput(Qref_droop_pu,deltat,CORRECTOR);
					}
				}
				// V_Avg_pu is the average value of three phase voltages
				// V_filter is the voltage pass through low pass filter
				// Qref_droop_pu if the Q reference from volt-var
				// Qref_droop_pu_filter is the Q pass through low pass filter
				// Qref_max and Qref_min are the upper and lower limits of Q references
				// Function end

				//Set default return state
				simmode_return_value = SM_EVENT;	//default to event-driven

				// Update the limit of Qref_max and Qref_min & Pref_max and Pref_min, which are used to limit the boundaries
				// of active and reactive power commands, considering both enable and disable of frequency-watt and volt-var
				if(volt_var) //When volt-var is enabled, Q is limited by Qref_max and Qref_min
				{
					if(frequency_watt)  // When frequency-watt is enabled, P is controlled by Pref_droop_pu
					{
						if (checkRampRate_real)
						{
							if (Pref_droop_pu > Pref_max)
							{
								Pref_droop_pu = Pref_max;
							}

							if (Pref_droop_pu < Pref_min)
							{
								Pref_droop_pu = Pref_min;
							}
						}
						else
						{
							if (Pref_droop_pu_filter > Pref_max)
							{
								Pref_droop_pu_filter = Pref_max;
							}

							if (Pref_droop_pu_filter < Pref_min)
							{
								Pref_droop_pu_filter = Pref_min;
							}
						}

						Qref_max = sqrt(Smax*Smax-Pref_droop_pu_filter*Pref_droop_pu_filter);
						Qref_min = -Qref_max;
						if(checkRampRate_reactive)
						{
							if (Qref_droop_pu > Qref_max)
							{
								Qref_droop_pu = Qref_max;
							}

							if (Qref_droop_pu < Qref_min)
							{
								Qref_droop_pu = Qref_min;
							}
						}
						else
						{
							if (Qref_droop_pu_filter > Qref_max)
							{
								Qref_droop_pu_filter = Qref_max;
							}

							if (Qref_droop_pu_filter < Qref_min)
							{
								Qref_droop_pu_filter = Qref_min;
							}
						}
					}
					else  //When frequency-watt is disabled, P is controlled by Pref
					{
						if(checkRampRate_reactive)
						{
							if (Qref_droop_pu > Qref_max)
							{
								Qref_droop_pu = Qref_max;
							}

							if (Qref_droop_pu < Qref_min)
							{
								Qref_droop_pu = Qref_min;
							}
							Pref_max = sqrt(S_base*Smax*S_base*Smax - (Qref_droop_pu*S_base)*(Qref_droop_pu*S_base));
						}
						else
						{
							if (Qref_droop_pu_filter > Qref_max)
							{
								Qref_droop_pu_filter = Qref_max;
							}

							if (Qref_droop_pu_filter < Qref_min)
							{
								Qref_droop_pu_filter = Qref_min;
							}
							Pref_max = sqrt(S_base*Smax*S_base*Smax - (Qref_droop_pu_filter*S_base)*(Qref_droop_pu_filter*S_base));
						}

						Pref_min = -Pref_max;
						if (Pref > Pref_max)
						{
							Pref = Pref_max;
						}

						if (Pref < Pref_min)
						{
							Pref = Pref_min;
						}
					}
				}
				else //When volt-var is disabled, Q is limited by Qref
				{
					if(frequency_watt)
					{
						if (checkRampRate_real)
						{
							if (Pref_droop_pu > Pref_max)
							{
								Pref_droop_pu = Pref_max;
							}

							if (Pref_droop_pu < Pref_min)
							{
								Pref_droop_pu = Pref_min;
							}
						}
						else
						{
							if (Pref_droop_pu_filter > Pref_max)
							{
								Pref_droop_pu_filter = Pref_max;
							}

							if (Pref_droop_pu_filter < Pref_min)
							{
								Pref_droop_pu_filter = Pref_min;
							}
						}

						if(Qref > sqrt(Smax*Smax - Pref_droop_pu*Pref_droop_pu)*S_base)
						{
							gl_warning("inverter_dyn:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if(Qref < -sqrt(Smax*Smax - Pref_droop_pu*Pref_droop_pu)*S_base)
						{
							gl_warning("inverter_dyn:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}
						Qref_max = sqrt(Smax*Smax - Pref_droop_pu*Pref_droop_pu)*S_base;
						Qref_min = -Qref_max;
						if (Qref > Qref_max)
						{
							Qref = Qref_max;
						}

						if (Qref < Qref_min)
						{
							Qref =Qref_min;
						}
					}
					else
					{
						if(Qref > sqrt(S_base*Smax*S_base*Smax - Pref*Pref))
						{
							gl_warning("inverter_dyn:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if(Qref < -sqrt(S_base*Smax*S_base*Smax - Pref*Pref))
						{
							gl_warning("inverter_dyn:%d %s - The inverter output apparent power is larger than the rated apparent power, the output reactive power is capped", obj->id, (obj->name ? obj->name : "Unnamed"));
							//Defined above
						}

						if (Pref > Pref_max*S_base)
						{
							Pref = Pref_max*S_base;
						}

						if (Pref < Pref_min*S_base)
						{
							Pref = Pref_min*S_base;
						}
						Qref_max = sqrt(S_base*Smax*S_base*Smax - Pref*Pref);
						Qref_min = -Qref_max;
						if (Qref > Qref_max)
						{
							Qref = Qref_max;
						}

						if (Qref < Qref_min)
						{
							Qref =Qref_min;
						}
					}
				}
				// Function end

				// Function: Current Control Loop
				for (i = 0; i < 3; i++)
				{
					Xi_filter[i] = 0.9073*(2.0 * PI * f_nominal)*CFilt*ug_pu_PS[i]*(fPLL[i]/f_nominal);
					Qcap[i] = Xi_filter[i] * ug_pu_PS[i];

					ug_pu_PS_old[i] = ug_pu_PS[i];
					ug_pu_PS[i] = sqrt(ugd_pu_PS*ugd_pu_PS+ugq_pu_PS*ugq_pu_PS);

					if (frequency_watt)
					{
						if (checkRampRate_real)
						{
							igd_ref[i] = Pref_droop_pu / ugd_pu_PS;
						}
						else
						{
							igd_cmd[i] = Pref_droop_pu_filter / ug_pu_PS[i];
						}
					}
					else
					{
						igd_cmd[i] = Pref / S_base / ug_pu_PS[i];
					}

					if (!checkRampRate_real)
					{									
						Test2 = Pref / S_base / ug_pu_PS[0];
						if (ug_pu_PS_risingedge[i] == 1)
						{
							if (gl_globaldeltaclock < start_time_ug_risingedge[i] + igq_hold_time)
							{
								ig_mag_pu_NS = 0;
							}
							else
							{
								ig_mag_pu_NS = ig_pu_NS_edge;
							}
						}
						else
						{
							ig_mag_pu_NS = ig_pu_NS;
						}
						Icmax = Smax - ig_mag_pu_NS;
						Ipmax[i] = Icmax;//Smax/ug_pu_PS[i];
						if (Ipmax[i]<=0)
							{Ipmax[i] = 0;}
						if (Ipmax[i]>=Ipmax_limit)
							{Ipmax[i] = Ipmax_limit;}

						if (sqrt(Icmax*Icmax-igq_cmd[i]*igq_cmd[i]) <= 0)
							{IpUL[i] = 0;}
						else if (sqrt(Icmax*Icmax-igq_cmd[i]*igq_cmd[i]) >= Ipmax[i])
							{IpUL[i] = Ipmax[i];}
						else
							{IpUL[i] = sqrt(Icmax*Icmax-igq_cmd[i]*igq_cmd[i]);}


						if (IpUL[i]<=0)
							{IpUL[i] = 0;}
						if (IpUL[i]>=Ipmax_limit)
							{IpUL[i] = Ipmax_limit;}

						if (igd_cmd[i] < -1*IpUL[i])
							{igd_cmd[i] = -1*IpUL[i];}
						else if (igd_cmd[i] > 1*IpUL[i])
							{igd_cmd[i] = 1*IpUL[i];}

						// looping till required time is not achieved
						if (igd_delay_flag == 0.0)
						{
							count[i] = count[i] +1;
						}
						else
						{
							count[i] = 0;
							// get the current references
							igd_ref_old[i] = igd_cmd[i];
						}
						igd_ref[i] = igd_cmd[i];
					}

					if (volt_var)
					{
						if(checkRampRate_reactive)
						{
							igq_ref[i] = -Qref_droop_pu / ug_pu_PS[i];
						}
						else
						{
							igq_ref[i] = -Qref_droop_pu_filter / ug_pu_PS[i];
						}
						igq_cmd[i] = igq_ref[i];
					}
					else
					{
						if (ug_pu_PS[i] <= ug_lvrt_threshold)
						{
							if (ug_pu_NS >= ug_asym_neg_threshold)
							{
								if ((ug_pu_PS[i] - 1) < ug_asym_threshold1)
								{
									Qref_pu = Qref_asym_threshold1_lim;
								}
								else if ((ug_pu_PS[i]-1) < ug_asym_threshold2)
								{
									Qref_pu = Qref_asym_threshold2_lim;//
								}
								else if ((ug_pu_PS[i]-1) < ug_asym_threshold3)
								{
									Qref_pu = Qref_asym_threshold3_k * (ug_pu_PS[i]-1) + Qref_asym_threshold3_b;
								}
								else if ((ug_pu_PS[i]-1) < ug_asym_threshold4)
								{
									Qref_pu = Qref_asym_threshold4_k * (ug_pu_PS[i]-1) + Qref_asym_threshold4_b;
								}
								else if ((ug_pu_PS[i]-1) < ug_asym_threshold5)
								{
									Qref_pu = Qref_asym_threshold5_k * (ug_pu_PS[i]-1) + Qref_asym_threshold5_b;
								}
								else if ((ug_pu_PS[i]-1) < ug_asym_threshold6)
								{
									Qref_pu = Qref_asym_threshold6_k * (ug_pu_PS[i]-1) + Qref_asym_threshold6_b;
								}
								else
								{
									Qref_pu = Qref_asym_threshold7_lim;
								}

								if (Qref_pu <= Qref_asym_LL)
									{Qref_pu = Qref_asym_LL;}
								if (Qref_pu >= Qref_asym_UL)
									{Qref_pu = Qref_asym_UL;}
							}
							else
							{
								if ((ug_pu_PS[i] - 1) < ug_sym_threshold1)
								{
									Qref_pu = Qref_sym_threshold1_lim;
								}
								else if ((ug_pu_PS[i]-1) < ug_sym_threshold2)
								{
									Qref_pu = Qref_sym_threshold2_k* (ug_pu_PS[i]-1) + Qref_sym_threshold2_b;
								}
								else if ((ug_pu_PS[i]-1) < ug_sym_threshold3)
								{
									Qref_pu = Qref_sym_threshold3_k * (ug_pu_PS[i]-1) + Qref_sym_threshold3_b;
								}
								else if ((ug_pu_PS[i]-1) < ug_sym_threshold4)
								{
									Qref_pu = Qref_sym_threshold4_k * (ug_pu_PS[i]-1) + Qref_sym_threshold4_b;
								}
								else if ((ug_pu_PS[i]-1) < ug_sym_threshold5)
								{
									Qref_pu = Qref_sym_threshold5_k * (ug_pu_PS[i]-1) + Qref_sym_threshold5_b;
								}
								else
								{
									Qref_pu = Qref_sym_threshold6_lim;
								}

								if (Qref_pu <= Qref_sym_LL)
								{Qref_pu = Qref_sym_LL;}
								if (Qref_pu >= Qref_sym_UL)
								{Qref_pu = Qref_sym_UL;}
							}
						}
						else
						{
							Qref_pu = Qref / S_base;
						}

						if (Qref_pu > Qref_max)
						{
							Qref_pu = Qref_max;
						}

						if (Qref_pu < Qref_min)
						{
							Qref_pu = Qref_min;
						}

						Xi_filter[i] = 0.9073*(2.0 * PI * f_nominal)*CFilt*ug_pu_PS[i]*(fPLL[i]/f_nominal);
						Qcap[i] = Xi_filter[i] * ug_pu_PS[i];
						Qerr[i] = Qref_pu - Q_out_pu;
						if (ug_pu_PS[i] <= ug_lvrt_threshold)
							{Vref_pu[i] = 1.0;}
						else
						{
							Qreg_ctrl_out[i] = Qerr[i]*kqp + Qreg_ctrl_blk[i].getoutput(Qerr[i],deltat,CORRECTOR);
							Vref_pu[i] = 1.0 + Qreg_ctrl_out[i];
						}
						Verr[i] = Vref_pu[i] - ug_pu_PS[i];
						if (ug_pu_PS[i] <= ug_lvrt_threshold)
							{igq_cmd[i] = -1.0*Qref_pu;}
						else
						{
							igq_cmd[i] = -kvp*Verr[i] - Vreg_ctrl_blk[i].getoutput(Verr[i],deltat,CORRECTOR);
						}

						if ( abs(ug_pu_PS_old[i] - ug_pu_PS[i]) >0.05 )
							{
								ug_pu_PS_risingedge[i] = 1;
								// Storing start time
								start_time_ug_risingedge[i] = gl_globaldeltaclock;
								ig_pu_NS_edge = ig_pu_NS;
							}

						if (ug_pu_PS_risingedge[i] == 1)
						{
							if (gl_globaldeltaclock < start_time_ug_risingedge[i] + igq_hold_time)
							{
								igq_ref[i] = igq_hold_value; //
							}
							else
							{
								ug_pu_PS_risingedge[i] = 0;
								igq_ref[i] = igq_cmd[i];
							}
						}
						else
						{
							igq_ref[i] = igq_cmd[i];
						}

					}

					//Add the limits for igd_ref and idq_ref
					if (igd_ref[i] > igd_ref_max)
					{
						igd_ref[i] = igd_ref_max;
					}

					if (igd_ref[i] < igd_ref_min)
					{
						igd_ref[i] = igd_ref_min;
					}

					if (igq_ref[i] > igq_ref_max)
					{
						igq_ref[i] = igq_ref_max;
					}

					if (igq_ref[i] < igq_ref_min)
					{
						igq_ref[i] = igq_ref_min;
					}
					// End of adding limits for igd_ref and idq_ref

					// Low pass filter for current id
					igd_filter_ref[i] = igd_filter_blk[i].getoutput(igd_ref[i],deltat,CORRECTOR);
					igd_filter[i] = igd_filter_ref[i];
					// Low pass filter for current iq
					igq_filter_ref[i] = igq_filter_blk[i].getoutput(igq_ref[i],deltat,CORRECTOR);
					igq_filter[i] = igq_filter_ref[i];

					// Negative sequence control
					igd_ns_filter[i] = 0;
					if (value_Circuit_V_NS.Re() <0 )
					{
						igq_ns_filter[i] = -ug_pu_NS*2;
					}
					else
					{
						igq_ns_filter[i] = ug_pu_NS*2;
					}

					// igd_ref[i] and igq_ref[i] are the current references in dq frame
					// igd_filter[i] and igq_filter[i] are the currents
					// Function end

					// Function: Coordinate Transformation: dq to xy
					I_source_Re[i] = (igd_filter[i] * cos(Angle_PLL[i]) - igq_filter[i] * sin(Angle_PLL[i])) * I_base;
					I_source_ns_Re[i] =  (igd_ns_filter[i] * cos(-Angle_PLL[i]- PI) - igq_ns_filter[i] * sin(-Angle_PLL[i]- PI)) * I_base;
					I_source_Im[i] = (igd_filter[i] * sin(Angle_PLL[i]) + igq_filter[i] * cos(Angle_PLL[i])) * I_base;
					I_source_ns_Im[i] =  (igd_ns_filter[i] * sin(-Angle_PLL[i]- PI) + igq_ns_filter[i] * cos(-Angle_PLL[i]- PI)) * I_base;
					if (Neg_Curr_Ctrl == 1.0)
					{
						I_source[i] = gld::complex(I_source_Re[i]+I_source_ns_Re[i], I_source_Im[i]+I_source_ns_Im[i]);
					}
					else
					{
						I_source[i] = gld::complex(I_source_Re[i], I_source_Im[i]);
					}
					value_IGenerated[i] = I_source[i];
					// I_source[i] is the complex value of injected current
					// value_IGenerated[i] is the current injected to the grid
					// Function end

					//Compute the difference
					mag_diff_val = (value_IGenerated[i]-prev_value_IGenerated[i]).Mag();

					//Update tracker
					prev_value_IGenerated[i] = value_IGenerated[i];
					
					//Get the "convergence" test to see if we can exit to QSTS
					if (mag_diff_val > GridFollowing_curr_convergence_criterion)
					{
						simmode_return_value = SM_DELTA;

						//Force the iteration flag
						deltamode_exit_iteration_met = false;
					}
					//Default - was set to SM_EVENT above
				}

				//Check and see if we met our iteration criterion (due to how sequencing happens)
				if (simmode_return_value == SM_EVENT)
				{
					if (!deltamode_exit_iteration_met)
					{
						//Reiterate once
						simmode_return_value = SM_DELTA;

						//Set the flag
						deltamode_exit_iteration_met = true;
					}
					//Must be true, and already SM_EVENT, so exit
				}
			}
		}	 // end of three phase grid-following, corrector pass
		else //Additional iterations
		{
			//Just return whatever our "last desired" was
			simmode_return_value = desired_simulation_mode;
		}
	}//End valid meter/1547
	else
	{
		//Something disconnected - just flag us for event
		simmode_return_value = SM_EVENT;

		//Actually "zeroing" of powerflow is handled in the current update

		//Zero VA_Out though
		VA_Out = gld::complex(0.0,0.0);

		//Zero the "recordable current" outputs
		terminal_current_val[0] = gld::complex(0.0,0.0);
		terminal_current_val[1] = gld::complex(0.0,0.0);
		terminal_current_val[2] = gld::complex(0.0,0.0);

		//Set the per-unit to zero too, to be thorough
		terminal_current_val_pu[0] = gld::complex(0.0,0.0);
		terminal_current_val_pu[1] = gld::complex(0.0,0.0);
		terminal_current_val_pu[2] = gld::complex(0.0,0.0);
	}

	//Sync the powerflow variables
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values(false);
	}

	//Perform a check to determine how to go forward - grid-following flag should have been set above
	if (enable_1547_compliance)
	{
		//See if our return is value
		if ((ieee_1547_delta_return > 0.0) && (ieee_1547_delta_return < 1.7) && (simmode_return_value == SM_EVENT))
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

//Initializes dynamic equations for first entry
//Returns a SUCCESS/FAIL
//curr_time is the initial states/information
STATUS ibr_gfl::init_dynamics(void)
{
	OBJECT *obj = OBJECTHDR(this);

	//start_time_igd_delay = gl_globaldeltaclock;

	//Pull the powerflow values
	if (parent_is_a_meter)
	{
		reset_complex_powerflow_accumulators();

		pull_complex_powerflow_values();
	}

	//Set the mode tracking variable to a default - not really needed, but be paranoid
	desired_simulation_mode = SM_EVENT;

	if (parent_is_single_phase) // single phase/split-phase implementation
	{
		//Update output power
		//Get current injected to the grid, value_IGenerated is obtained from power flow calculation
		terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

		//Update per-unit value
		terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

		power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];

		VA_Out = power_val[0];

		//See if it is the first deltamode entry - theory is all future changes will trigger deltamode, so these should be set
		if (first_deltamode_init)
		{
			//See if we set it to something first
			Vset = value_Circuit_V[0].Mag() / V_base;

			//Set it false in here, for giggles
			first_deltamode_init = false;
		}
		//Default else - all changes should be in deltamode

		// Initialize the PLL
		Angle_PLL_blk[0].init_given_y(value_Circuit_V[0].Arg());
		Angle_PLL[0] = Angle_PLL_blk[0].x[0];
		
		delta_w_PLL_blk[0].setparams(kpPLL,kiPLL,-1000.0,1000.0,-1000.0,1000.0);
		delta_w_PLL_blk[0].init_given_y(0.0);


		ugd_pu[0] = (value_Circuit_V[0].Re() * cos(value_Circuit_V[0].Arg()) + value_Circuit_V[0].Im() * sin(value_Circuit_V[0].Arg())) / V_base;
		ugq_pu[0] = (-value_Circuit_V[0].Re() * sin(value_Circuit_V[0].Arg()) + value_Circuit_V[0].Im() * cos(value_Circuit_V[0].Arg())) / V_base;
		igd_pu[0] = (terminal_current_val[0].Re() * cos(value_Circuit_V[0].Arg()) + terminal_current_val[0].Im() * sin(value_Circuit_V[0].Arg())) / I_base;
		igq_pu[0] = (-terminal_current_val[0].Re() * sin(value_Circuit_V[0].Arg()) + terminal_current_val[0].Im() * cos(value_Circuit_V[0].Arg())) / I_base;

		if (Neg_Curr_Ctrl == 1.0)
		{
			// Initialize the current control loops
			e_source[0] = (value_IGenerated[0] * gld::complex(Rfilter, Xfilter) * Z_base);
			ed_pu[0] = (e_source[0].Re() * cos(value_Circuit_V[0].Arg()) + e_source[0].Im() * sin(value_Circuit_V[0].Arg())) / V_base;
			eq_pu[0] = (-e_source[0].Re() * sin(value_Circuit_V[0].Arg()) + e_source[0].Im() * cos(value_Circuit_V[0].Arg())) / V_base;

			igd_blk[0].setparams(kpc,kic,-1000.0,1000.0,-1000.0,1000.0);
			igq_blk[0].setparams(kpc,kic,-1000.0,1000.0,-1000.0,1000.0);

			igd_blk[0].init(0.0,ed_pu[0] - ugd_pu[0] + igq_pu[0] * Xfilter * F_current);
			igq_blk[0].init(0.0,eq_pu[0] - ugq_pu[0] - igd_pu[0] * Xfilter * F_current);
		}
		else
		{
			// Initialize the current source
			I_source[0] = value_IGenerated[0];

			//igd_filter_blk[0].setparams(Tif);
			//igq_filter_blk[0].setparams(Tif);

			// Initialize Qreg and Vreg controller
			Qreg_ctrl_blk[0].setparams(Tqi,-1,1,-0.1,0.1);
			Vreg_ctrl_blk[0].setparams(Tvi,-1.5,1.5,-1.5,1.5);

			Qreg_ctrl_blk[0].init(0.0,0.0);
			Vreg_ctrl_blk[0].init(0.0,0.0);

			igd_filter_blk[0].setparams(Tif_igd_ref);
			igq_filter_blk[0].setparams(Tif_igq_ref);

			igd_filter_blk[0].init(0.0,igd_pu[0]);
			igq_filter_blk[0].init(0.0,igq_pu[0]);

			igd_filter_ref[0] = igd_filter_blk[0].x[0];
			igq_filter_ref[0] = igq_filter_blk[0].x[0];

		}

		if (frequency_watt)
		{
			// Initialize the frequency-watt
				f_filter_blk.setparams(Tff);
			f_filter_blk.init(0.0,fPLL[0]);
			f_filter = f_filter_blk.x[0];
			
			if ((f_filter < (f_nominal + db_OF))&&(f_filter > (f_nominal - db_UF)))  // add dead band
			{
				Pref_droop_pu = Pref / S_base;
			}
			else
			{
				Pref_droop_pu = (f_nominal - f_filter) / Rp + Pref / S_base;
			}
			
			Pref_droop_pu_filter_blk.setparams(Tpf);
			Pref_droop_pu_filter_blk.init_given_y(Pref_droop_pu);
			Pref_droop_pu_filter = Pref_droop_pu_filter_blk.x[0];
		}

		if (volt_var)
		{
			// Initialize the volt-var control
				V_Avg_pu = value_Circuit_V[0].Mag() / V_base;
				V_filter_blk.setparams(Tvf);
			V_filter_blk.init_given_y(V_Avg_pu);
			V_filter = V_filter_blk.x[0];

			if ((V_filter < (Vset + db_OV))&&(V_filter > (Vset - db_UV)))  // add dead band
			{
				Qref_droop_pu = Qref / S_base;
			}
			else
			{
				Qref_droop_pu = (Vset - V_filter) / Rq + Qref / S_base;
			}

			Qref_droop_pu_filter_blk.setparams(Tqf);
			Qref_droop_pu_filter_blk.init_given_y(Qref_droop_pu);
			Qref_droop_pu_filter = Qref_droop_pu_filter_blk.x[0];
		}
	}
	else //Three-phase
	{
		//Update output power
		//Get current injected to the grid, value_IGenerated is obtained from power flow calculation
		terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
		terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
		terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

		//Update per-unit values
		terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
		terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
		terminal_current_val_pu[2] = terminal_current_val[2] / I_base;

		//Update power output variables, just so we can see what is going on
		power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
		power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
		power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

		VA_Out = power_val[0] + power_val[1] + power_val[2];
				Q_out_pu = VA_Out.Im() / S_base;
		//Pref = VA_Out.Re();
		//Qref = VA_Out.Im();
		//See if it is the first deltamode entry - theory is all future changes will trigger deltamode, so these should be set
		if (first_deltamode_init)
		{
			//See if it was set


			Vset = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;

			//Set it false in here, for giggles
			first_deltamode_init = false;
		}
		//Default else - all changes should be in deltamode

		// Obtain the positive sequence voltage
		value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;

		// only consider positive sequence
		Angle_PLL_blk[0].init_given_y(value_Circuit_V_PS.Arg());
		Angle_PLL_blk[1].init_given_y(value_Circuit_V_PS.Arg() - 2.0 / 3.0 * PI);
		Angle_PLL_blk[2].init_given_y(value_Circuit_V_PS.Arg() + 2.0 / 3.0 * PI);
		Angle_PLL[0] = Angle_PLL_blk[0].x[0];
		Angle_PLL[1] = Angle_PLL_blk[1].x[0];
		Angle_PLL[2] = Angle_PLL_blk[2].x[0];

		for (int i = 0; i < 1; i++)
		{
			// Initialize the PLL
			delta_w_PLL_blk[i].setparams(kpPLL,kiPLL,-1000.0,1000.0,-1000.0,1000.0);
			delta_w_PLL_blk[i].init_given_y(0.0);
		}

		for (int i = 0; i < 3; i++)
		{
			// Initialize Qreg and Vreg controller
			Qreg_ctrl_blk[i].setparams(Tqi,-1.0,1.0,-0.1,0.1);
			Vreg_ctrl_blk[i].setparams(Tvi,-1.5,1.5,-1.5,1.5);

			Qreg_ctrl_blk[i].init(0.0,0.0);
			Vreg_ctrl_blk[i].init(0.0,0.0);

			igd_filter_blk[i].setparams(Tif_igd_ref);
			igq_filter_blk[i].setparams(Tif_igq_ref);
			//
		}

		for (int i = 0; i < 3; i++)
		{
			ugd_pu[i] = (value_Circuit_V[i].Re() * cos(Angle_PLL[i]) + value_Circuit_V[i].Im() * sin(Angle_PLL[i])) / V_base;
			ugq_pu[i] = (-value_Circuit_V[i].Re() * sin(Angle_PLL[i]) + value_Circuit_V[i].Im() * cos(Angle_PLL[i])) / V_base;
			//ugd_ns_pu[i] = (value_Circuit_V[i].Re() * cos(-Angle_PLL[i]) + value_Circuit_V[i].Im() * sin(-Angle_PLL[i])) / V_base;
			//ugq_ns_pu[i] = (-value_Circuit_V[i].Re() * sin(-Angle_PLL[i]) + value_Circuit_V[i].Im() * cos(-Angle_PLL[i])) / V_base;

			igd_pu[i] = (terminal_current_val[i].Re() * cos(Angle_PLL[i]) + terminal_current_val[i].Im() * sin(Angle_PLL[i])) / I_base;
			igq_pu[i] = (-terminal_current_val[i].Re() * sin(Angle_PLL[i]) + terminal_current_val[i].Im() * cos(Angle_PLL[i])) / I_base;
			//igd_ns_pu[i] = (terminal_current_val[i].Re() * cos(-Angle_PLL[i]) + terminal_current_val[i].Im() * sin(-Angle_PLL[i])) / I_base;
			//igq_ns_pu[i] = (-terminal_current_val[i].Re() * sin(-Angle_PLL[i]) + terminal_current_val[i].Im() * cos(-Angle_PLL[i])) / I_base;

			ug_pu_PS[i] = sqrt(ugd_pu[i]*ugd_pu[i]+ugq_pu[i]*ugq_pu[i]);//
			//ug_pu_NS[i] = sqrt(ugd_ns_pu[i]*ugd_ns_pu[i]+ugq_ns_pu[i]*ugq_ns_pu[i]);

			if (Neg_Curr_Ctrl == 1.0)
			{
				// Initialize the current control loops
				e_source[i] = (value_IGenerated[i] * gld::complex(Rfilter, Xfilter) * Z_base);
				ed_pu[i] = (e_source[i].Re() * cos(Angle_PLL[i]) + e_source[i].Im() * sin(Angle_PLL[i])) / V_base;
				eq_pu[i] = (-e_source[i].Re() * sin(Angle_PLL[i]) + e_source[i].Im() * cos(Angle_PLL[i])) / V_base;

				igd_blk[i].setparams(kpc,kic,-1000.0,1000.0,-1000.0,1000.0);
				igq_blk[i].setparams(kpc,kic,-1000.0,1000.0,-1000.0,1000.0);

				igd_blk[i].init_given_y(ed_pu[i] - ugd_pu[i] + igq_pu[i] * Xfilter * F_current);
				igq_blk[i].init_given_y(eq_pu[i] - ugq_pu[i] - igd_pu[i] * Xfilter * F_current);
			}
			else
			{

				// Initizalize the start time for the Id_ref transport delay
				start_time_igd_delay = gl_globaldeltaclock;
				igd_ref_old[i] = Pref / S_base / ug_pu_PS[i];

				// Initialize the current control loops
				I_source[i] = value_IGenerated[i];

				igd_filter_blk[i].setparams(Tif_igd_ref);
				igq_filter_blk[i].setparams(Tif_igq_ref);

				igd_filter_blk[i].init(0.0,igd_pu[i]);
				igq_filter_blk[i].init(0.0,igq_pu[i]);

				igd_filter_ref[i]  = igd_filter_blk[i].x[0];
				igq_filter_ref[i]  = igq_filter_blk[i].x[0];
			}
		}
		
		if (frequency_watt)
		{
			// Initialize the frequency-watt
			f_filter_blk.setparams(Tff);
			f_filter_blk.init_given_y((fPLL[0]+fPLL[1]+fPLL[2])/3.0);
			f_filter = f_filter_blk.x[0];

			if ((f_filter < (f_nominal + db_OF))&&(f_filter > (f_nominal - db_UF)))  // add dead band
			{
				Pref_droop_pu = Pref / S_base;
			}
			else
			{
				Pref_droop_pu = (f_nominal - f_filter) / Rp + Pref / S_base;
			}

			Pref_droop_pu_filter_blk.setparams(Tpf);
			Pref_droop_pu_filter_blk.init_given_y(Pref_droop_pu);
			Pref_droop_pu_filter = Pref_droop_pu_filter_blk.x[0];
		}

		if (volt_var)
		{
			// Initialize the volt-var control
			V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;
			V_filter_blk.setparams(Tvf);
			V_filter_blk.init_given_y(V_Avg_pu);
			V_filter = V_filter_blk.x[0];

			if ((V_filter < (Vset + db_OV))&&(V_filter > (Vset - db_UV)))  // add dead band
			{
				Qref_droop_pu = Qref / S_base;
			}
			else
			{
				Qref_droop_pu = (Vset - V_filter) / Rq + Qref / S_base;
			}

			Qref_droop_pu_filter_blk.setparams(Tqf);
			Qref_droop_pu_filter_blk.init_given_y(Qref_droop_pu);
			Qref_droop_pu_filter = Qref_droop_pu_filter_blk.x[0];
		}
	}

	return SUCCESS;
}

// //Module-level post update call
// /* Think this was just put here as an example - not sure what it would do */
// STATUS ibr_gfl::post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass)
// {
// 	//If we have a meter, reset the accumulators
// 	if (parent_is_a_meter)
// 	{
// 		reset_complex_powerflow_accumulators();
// 	}

// 	//Should have a parent, but be paranoid
// 	if (parent_is_a_meter)
// 	{
// 		push_complex_powerflow_values();
// 	}

// 	return SUCCESS; //Always succeeds right now
// }

//Map Complex value
gld_property *ibr_gfl::map_complex_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_complex())
	{
		GL_THROW("ibr_gfl:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Map double value
gld_property *ibr_gfl::map_double_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_double())
	{
		GL_THROW("ibr_gfl:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Function to pull all the complex properties from powerflow into local variables
void ibr_gfl::pull_complex_powerflow_values(void)
{
	//Pull in the various values from powerflow - straight reads
	//Pull status
	value_MeterStatus = pMeterStatus->get_enumeration();

	//Pull frequency
	value_Frequency = pFrequency->get_double();

	//********** TODO - Portions of this may need to be a "deltamode only" pull	 **********//
	//Update IGenerated, in case the powerflow is overriding it
	if (parent_is_single_phase)
	{
		//Just pull "zero" values
		value_Circuit_V[0] = pCircuit_V[0]->get_complex();

		value_IGenerated[0] = pIGenerated[0]->get_complex();
	}
	else
	{
		//Pull all voltages
		value_Circuit_V[0] = pCircuit_V[0]->get_complex();
		value_Circuit_V[1] = pCircuit_V[1]->get_complex();
		value_Circuit_V[2] = pCircuit_V[2]->get_complex();

		//Pull currents
		value_IGenerated[0] = pIGenerated[0]->get_complex();
		value_IGenerated[1] = pIGenerated[1]->get_complex();
		value_IGenerated[2] = pIGenerated[2]->get_complex();
	}
}

//Function to reset the various accumulators, so they don't double-accumulate if they weren't used
void ibr_gfl::reset_complex_powerflow_accumulators(void)
{
	int indexval;

	//See which one we are, since that will impact things
	if (!parent_is_single_phase) //Three-phase
	{
		//Loop through the three-phases/accumulators
		for (indexval = 0; indexval < 3; indexval++)
		{
			//**** Current value ***/
			value_Line_I[indexval] = gld::complex(0.0, 0.0);

			//**** Power value ***/
			value_Power[indexval] = gld::complex(0.0, 0.0);

			//**** pre-rotated Current value ***/
			value_Line_unrotI[indexval] = gld::complex(0.0, 0.0);
		}
	}
	else //Assumes must be single phased - else how did it get here?
	{
		//Reset the relevant values -- all single pulls

		//**** single current value ***/
		value_Line_I[0] = gld::complex(0.0, 0.0);

		//**** power value ***/
		value_Power[0] = gld::complex(0.0, 0.0);

		//**** prerotated value ***/
		value_Line_unrotI[0] = gld::complex(0.0, 0.0);
	}
}

//Function to push up all changes of complex properties to powerflow from local variables
void ibr_gfl::push_complex_powerflow_values(bool update_voltage)
{
	gld::complex temp_complex_val;
	gld_wlock *test_rlock = nullptr;
	int indexval;

	//See which one we are, since that will impact things
	if (!parent_is_single_phase) //Three-phase
	{
		//See if we were a voltage push or not
		if (update_voltage)
		{
			//Loop through the three-phases/accumulators
			for (indexval=0; indexval<3; indexval++)
			{
				//**** push voltage value -- not an accumulator, just force ****/
				pCircuit_V[indexval]->setp<gld::complex>(value_Circuit_V[indexval],*test_rlock);
			}
		}
		else
		{
			//Loop through the three-phases/accumulators
			for (indexval = 0; indexval < 3; indexval++)
			{
				//**** Current value ***/
				//Pull current value again, just in case
				temp_complex_val = pLine_I[indexval]->get_complex();

				//Add the difference
				temp_complex_val += value_Line_I[indexval];

				//Push it back up
				pLine_I[indexval]->setp<gld::complex>(temp_complex_val, *test_rlock);

				//**** Power value ***/
				//Pull current value again, just in case
				temp_complex_val = pPower[indexval]->get_complex();

				//Add the difference
				temp_complex_val += value_Power[indexval];

				//Push it back up
				pPower[indexval]->setp<gld::complex>(temp_complex_val, *test_rlock);

				//**** pre-rotated Current value ***/
				//Pull current value again, just in case
				temp_complex_val = pLine_unrotI[indexval]->get_complex();

				//Add the difference
				temp_complex_val += value_Line_unrotI[indexval];

				//Push it back up
				pLine_unrotI[indexval]->setp<gld::complex>(temp_complex_val, *test_rlock);

				/* If was VSI, adjust Norton injection */
				{
					//**** IGenerated Current value ***/
					//Direct write, not an accumulator
					pIGenerated[indexval]->setp<gld::complex>(value_IGenerated[indexval], *test_rlock);
				}
			}//End phase loop
		}//End not voltage push
	}//End three-phase
	else //Assumes must be single-phased - else how did it get here?
	{
		//Check for voltage push - in case that's ever needed here
		if (update_voltage)
		{
			//Should just be zero
			//**** push voltage value -- not an accumulator, just force ****/
			pCircuit_V[0]->setp<gld::complex>(value_Circuit_V[0],*test_rlock);
		}
		else
		{
			//Pull the relevant values -- all single pulls

			//**** Current value ***/
			//Pull current value again, just in case
			temp_complex_val = pLine_I[0]->get_complex();

			//Add the difference
			temp_complex_val += value_Line_I[0];

			//Push it back up
			pLine_I[0]->setp<gld::complex>(temp_complex_val, *test_rlock);

			//**** power value ***/
			//Pull current value again, just in case
			temp_complex_val = pPower[0]->get_complex();

			//Add the difference
			temp_complex_val += value_Power[0];

			//Push it back up
			pPower[0]->setp<gld::complex>(temp_complex_val, *test_rlock);

			//**** prerotated value ***/
			//Pull current value again, just in case
			temp_complex_val = pLine_unrotI[0]->get_complex();

			//Add the difference
			temp_complex_val += value_Line_unrotI[0];

			//Push it back up
			pLine_unrotI[0]->setp<gld::complex>(temp_complex_val, *test_rlock);

			//**** IGenerated ****/
			//********* TODO - Does this need to be deltamode-flagged? *************//
			//Direct write, not an accumulator
			pIGenerated[0]->setp<gld::complex>(value_IGenerated[0], *test_rlock);
		}//End not voltage update
	}//End single-phase
}

// Function to update current injection IGenerated for VSI
STATUS ibr_gfl::updateCurrInjection(int64 iteration_count,bool *converged_failure)
{
	double temp_time;
	OBJECT *obj = OBJECTHDR(this);
	gld::complex temp_VA;
	gld::complex temp_V1, temp_V2;
	bool bus_is_a_swing, bus_is_swing_pq_entry;
	STATUS temp_status_val;
	gld_property *temp_property_pointer;
	bool running_in_delta;
	bool limit_hit;
	double freq_diff_angle_val, tdiff;
	double mag_diff_val[3];
	gld::complex rotate_value;
	char loop_var;
	gld::complex intermed_curr_calc[3];

	//Assume starts converged (no failure)
	*converged_failure = false;

	if (deltatimestep_running > 0.0) //Deltamode call
	{
		//Get the time
		temp_time = gl_globaldeltaclock;
		running_in_delta = true;
	}
	else
	{
		//Grab the current time
		temp_time = (double)gl_globalclock;
		running_in_delta = false;
	}

	//Pull the current powerflow values
	if (parent_is_a_meter)
	{
		//Reset the accumulators, just in case
		reset_complex_powerflow_accumulators();

		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	//External call to internal variables -- used by powerflow to iterate the VSI implementation, basically

	if (sqrt(Pref*Pref+Qref*Qref) > S_base)
	{
		gl_warning("ibr_gfl:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
		//Defined above
	}

	if(Pref > S_base)
	{
		//Pref = S_base;
	}
	else if (Pref < -S_base)
	{
		//Pref = -S_base;
	}
	else
		if(Qref > sqrt(S_base*S_base-Pref*Pref))
		{
			//Qref = sqrt(S_base*S_base-Pref*Pref);
		}
		else if (Qref < -sqrt(S_base*S_base-Pref*Pref))
		{
			//Qref = -sqrt(S_base*S_base-Pref*Pref);
		}

	temp_VA = gld::complex(Pref, Qref);

	//See if we're a meter
	if (parent_is_a_meter)
	{
		//By default, assume we're not a SWING
		bus_is_a_swing = false;

		//Determine our status
		if (attached_bus_type > 1) //SWING or SWING_PQ
		{
			//Map the function, if we need to
			if (swing_test_fxn == nullptr)
			{
				//Map the swing status check function
				swing_test_fxn = (FUNCTIONADDR)(gl_get_function(obj->parent, "pwr_object_swing_status_check"));

				//See if it was located
				if (swing_test_fxn == nullptr)
				{
					GL_THROW("ibr_gfl:%s - failed to map swing-checking for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
					/*  TROUBLESHOOT
					While attempting to map the swing-checking function, an error was encountered.
					Please try again.  If the error persists, please submit your code and a bug report via the trac website.
					*/
				}
			}

			//Call the testing function
			temp_status_val = ((STATUS (*)(OBJECT *,bool * , bool*))(*swing_test_fxn))(obj->parent,&bus_is_a_swing,&bus_is_swing_pq_entry);

			//Make sure it worked
			if (temp_status_val != SUCCESS)
			{
				GL_THROW("ibr_gfl:%s - failed to map swing-checking for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
				//Defined above
			}

			//Now see how we've gotten here
			if (first_iteration_current_injection == -1) //Haven't entered before
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
			else if ((first_iteration_current_injection != 0) || bus_is_swing_pq_entry) //We didn't enter on the first iteration, or we're a repatrioted SWING_PQ
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
		if (parent_is_single_phase) // single phase/split-phase implementation
		{
			// **FT Note** Not sure what this code does.  Mapps are to value_circuit_V[0], which should already be 12
			// //Special exception for triplex (since delta doesn't get updated inside the powerflow)
			// if ((phases & 0x10) == 0x10)
			// {
			// 	//Pull in other voltages
			// 	temp_V1 = pCircuit_V[1]->get_complex();
			// 	temp_V2 = pCircuit_V[2]->get_complex();

			// 	//Just push it in as the update
			// 	value_Circuit_V[0] = temp_V1 + temp_V2;
			// }

			//Calculate the terminate current
			if (value_Circuit_V[0].Mag() > 0.0)
			{
				terminal_current_val[0] = ~(temp_VA / value_Circuit_V[0]);
			}
			else
			{
				terminal_current_val[0] = gld::complex(0.0,0.0);
			}

			//Update per-unit value
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;

			//Calculate the internal current
			value_IGenerated[0] = terminal_current_val[0] + filter_admittance * value_Circuit_V[0] ; //

			//Calculate the difference
			mag_diff_val[0] = (value_IGenerated[0]-prev_value_IGenerated[0]).Mag();

			//Update tracker
			prev_value_IGenerated[0] = value_IGenerated[0];
			
			//Get the "convergence" test to see if we can exit to QSTS
			if (mag_diff_val[0] > GridFollowing_curr_convergence_criterion)
			{
				//Mark as a failure
				*converged_failure = true;
			}
		}
		else //Three-phase
		{
			// Obtain the positive sequence voltage
			value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
			
			//Check value
			if (value_Circuit_V_PS.Mag() > 0.0)
			{
				// Obtain the positive sequence current
				value_Circuit_I_PS[0] = ~(temp_VA / 3.0 / value_Circuit_V_PS);
			}
			else
			{
				value_Circuit_I_PS[0] = gld::complex(0.0,0.0);
			}
			
			value_Circuit_I_PS[1] = value_Circuit_I_PS[0] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI));
			value_Circuit_I_PS[2] = value_Circuit_I_PS[0] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI));


			terminal_current_val[0] = value_Circuit_I_PS[0];
			terminal_current_val[1] = value_Circuit_I_PS[1];					
			terminal_current_val[2] = value_Circuit_I_PS[2];
			
			//Update per-unit values
			terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
			terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
			terminal_current_val_pu[2] = terminal_current_val[2] / I_base;					

			//Back out the current injection
			value_IGenerated[0] = value_Circuit_I_PS[0] + generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2];
			value_IGenerated[1] = value_Circuit_I_PS[1] + generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2];
			value_IGenerated[2] = value_Circuit_I_PS[2] + generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2];

			//Calculate the difference
			mag_diff_val[0] = (value_IGenerated[0]-prev_value_IGenerated[0]).Mag();
			mag_diff_val[1] = (value_IGenerated[1]-prev_value_IGenerated[1]).Mag();
			mag_diff_val[2] = (value_IGenerated[2]-prev_value_IGenerated[2]).Mag();

			//Update trackers
			prev_value_IGenerated[0] = value_IGenerated[0];
			prev_value_IGenerated[1] = value_IGenerated[1];
			prev_value_IGenerated[2] = value_IGenerated[2];
			
			//Get the "convergence" test to see if we can exit to QSTS
			if ((mag_diff_val[0] > GridFollowing_curr_convergence_criterion) || (mag_diff_val[1] > GridFollowing_curr_convergence_criterion) || (mag_diff_val[2] > GridFollowing_curr_convergence_criterion))
			{
				//Mark as a failure
				*converged_failure = true;
			}
		}//End three-phase
	}
	//Default else - things are handled elsewhere

	//Check to see if we're disconnected
	if (enable_1547_compliance && !inverter_1547_status || (value_MeterStatus == 0))
	{
		//Disconnected - offset our admittance contribution - could remove it from powerflow - visit this in the future
		if (parent_is_single_phase)
		{
			value_IGenerated[0] = filter_admittance * value_Circuit_V[0];
		}
		else	//Assumed to be a three-phase variant
		{
			value_IGenerated[0] = generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2];
			value_IGenerated[1] = generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2];
			value_IGenerated[2] = generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2];
		}
	}//End Disconnected
	//Default else - do nothing?

	//Push the changes up
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values(false);
	}

	//Always a success, but power flow solver may not like it if VA_OUT exceeded the rating and thus changed
	return SUCCESS;
}

//Function to initialize IEEE 1547 checks
STATUS ibr_gfl::initalize_IEEE_1547_checks(void)
{
	//Make sure it is parented
	if (parent_is_a_meter)
	{
		//See if we really want IEEE 1547-2003, not 1547A-2014
		if (ieee_1547_version == IEEE1547_2003)
		{
			//Adjust the values - high values
			IEEE1547_over_freq_high_band_setpoint = 70.0;	//Very high - only 1 band for 1547
			IEEE1547_over_freq_high_band_delay = 0.16;		//Same clearly as below
			IEEE1547_over_freq_low_band_setpoint = 60.5;	//1547 high hvalue
			IEEE1547_over_freq_low_band_delay = 0.16;		//1547 over-frequency value

			//Set the others based on size
			if (S_base > 30000.0)
			{
				IEEE1547_under_freq_high_band_setpoint = 59.5;	//Arbitrary selection in the range
				IEEE1547_under_freq_high_band_delay = 300.0;	//Maximum delay, just because
				IEEE1547_under_freq_low_band_setpoint = 57.0;	//Lower limit of 1547
				IEEE1547_under_freq_low_band_delay = 0.16;		//Lower limit clearing of 1547
			}
			else	//Smaller one
			{
				IEEE1547_under_freq_high_band_setpoint = 59.3;	//Low frequency value for small inverter 1547
				IEEE1547_under_freq_high_band_delay = 0.16;		//Low frequency value clearing time for 1547
				IEEE1547_under_freq_low_band_setpoint = 47.0;	//Arbitrary low point - 1547 didn't have this value for small inverters
				IEEE1547_under_freq_low_band_delay = 0.16;		//Same value as low frequency, since this band doesn't technically exist
			}

			//Set the voltage values as well - basically, the under voltage has an extra category
			IEEE1547_under_voltage_lowest_voltage_setpoint = 0.40;	//Lower than range before, so just duplicate disconnect value
			IEEE1547_under_voltage_middle_voltage_setpoint = 0.50;	//Lower limit of 1547
			IEEE1547_under_voltage_high_voltage_setpoint = 0.88;	//Low area threshold for 1547
			IEEE1547_over_voltage_low_setpoint = 1.10;				//Lower limit of upper threshold for 1547
			IEEE1547_over_voltage_high_setpoint = 1.20;				//Upper most limit of voltage values
			
			IEEE1547_under_voltage_lowest_delay = 0.16;			//Lower than "normal low" - it is technically an overlap
			IEEE1547_under_voltage_middle_delay = 0.16;			//Low limit of 1547
			IEEE1547_under_voltage_high_delay = 2.0;			//High lower limit of 1547
			IEEE1547_over_voltage_low_delay = 1.0;				//Low higher limit of 1547
			IEEE1547_over_voltage_high_delay = 0.16;			//Highest value
		}
	}
	else
	{
		//Make to disable 1547, since it won't do anything
		enable_1547_compliance = false;

		gl_warning("ibr_gfl:%d - %s does not have a valid parent - 1547 checks have been disabled",get_id(),get_name());
		/*  TROUBLESHOOT
		The IEEE 1547-2003/IEEE 1547a-2014 checks for interconnection require a valid powerflow parent.  One was not detected, so
		this functionality has been detected.
		*/
	}

	//By default, we succeed
	return SUCCESS;
}

//Functionalized routine to perform the IEEE 1547-2003/1547a-2014 checks
//Returns time to next violate, -1.0 for no violations, or -9999.0 for error states, -99.0 for recent return to service
double ibr_gfl::perform_1547_checks(double timestepvalue)
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

	//Perform frequency check - overlapping bands set so we don't care about size anymore
	if ((value_Frequency > IEEE1547_over_freq_low_band_setpoint) || (value_Frequency < IEEE1547_under_freq_high_band_setpoint))
	{
		//Flag it
		frequency_violation = true;

		//Reset "restoration" time
		IEEE1547_out_of_violation_time_total = 0.0;

		//Figure out which range we are
		if (value_Frequency > IEEE1547_over_freq_high_band_setpoint)
		{
			//Accumulate the over frequency timers (all for this case)
			IEEE1547_over_freq_high_band_viol_time += timestepvalue;
			IEEE1547_over_freq_low_band_viol_time += timestepvalue;

			//Zero the others, in case we did a huge jump
			IEEE1547_under_freq_high_band_viol_time = 0.0;
			IEEE1547_under_freq_low_band_viol_time = 0.0;

			if (IEEE1547_over_freq_high_band_viol_time > IEEE1547_over_freq_high_band_delay)
			{
				trigger_disconnect = true;
				return_time_freq = IEEE1547_reconnect_time;

				//Flag us as high over-frequency violation
				ieee_1547_trip_method = IEEE_1547_HIGH_OF;
			}
			else if (IEEE1547_over_freq_low_band_viol_time > IEEE1547_over_freq_low_band_delay)	//Triggered existing band
			{
				trigger_disconnect = true;
				return_time_freq = IEEE1547_reconnect_time;

				//Flag us as the low over-frequency violation
				ieee_1547_trip_method = IEEE_1547_LOW_OF;
			}
			else
			{
				trigger_disconnect = false;
				
				//See which time to return
				if ((IEEE1547_over_freq_high_band_delay - IEEE1547_over_freq_high_band_viol_time) < (IEEE1547_over_freq_low_band_delay - IEEE1547_over_freq_low_band_viol_time))
				{
					return_time_freq = IEEE1547_over_freq_high_band_delay - IEEE1547_over_freq_high_band_viol_time;
				}
				else	//Other way around
				{
					return_time_freq = IEEE1547_over_freq_low_band_delay - IEEE1547_over_freq_low_band_viol_time;
				}
			}
		}
		else if (value_Frequency < IEEE1547_under_freq_low_band_setpoint)
		{
			//Accumulate both under frequency timers (all violated)
			IEEE1547_under_freq_high_band_viol_time += timestepvalue;
			IEEE1547_under_freq_low_band_viol_time += timestepvalue;

			//Zero the others, in case we did a huge jump
			IEEE1547_over_freq_high_band_viol_time = 0.0;
			IEEE1547_over_freq_low_band_viol_time = 0.0;

			if (IEEE1547_under_freq_low_band_viol_time > IEEE1547_under_freq_low_band_delay)
			{
				trigger_disconnect = true;
				return_time_freq = IEEE1547_reconnect_time;

				//Flag us as the low under-frequency violation
				ieee_1547_trip_method = IEEE_1547_LOW_UF;
			}
			else if (IEEE1547_under_freq_high_band_viol_time > IEEE1547_under_freq_high_band_delay)	//Other band trigger
			{
				trigger_disconnect = true;
				return_time_freq = IEEE1547_reconnect_time;

				//Flag us as the high under-frequency violation
				ieee_1547_trip_method = IEEE_1547_HIGH_UF;
			}
			else
			{
				trigger_disconnect = false;

				//See which time to return
				if ((IEEE1547_under_freq_high_band_delay - IEEE1547_under_freq_high_band_viol_time) < (IEEE1547_under_freq_low_band_delay - IEEE1547_under_freq_low_band_viol_time))
				{
					return_time_freq = IEEE1547_under_freq_high_band_delay - IEEE1547_under_freq_high_band_viol_time;
				}
				else	//Other way around
				{
					return_time_freq = IEEE1547_under_freq_low_band_delay - IEEE1547_under_freq_low_band_viol_time;
				}
			}
		}
		else if ((value_Frequency < IEEE1547_under_freq_high_band_setpoint) && (value_Frequency >= IEEE1547_under_freq_low_band_setpoint))
		{
			//Just update the high violation time
			IEEE1547_under_freq_high_band_viol_time += timestepvalue;

			//Zero the other one, for good measure
			IEEE1547_under_freq_low_band_viol_time = 0.0;

			//Zero the others, in case we did a huge jump
			IEEE1547_over_freq_high_band_viol_time = 0.0;
			IEEE1547_over_freq_low_band_viol_time = 0.0;

			if (IEEE1547_under_freq_high_band_viol_time > IEEE1547_under_freq_high_band_delay)
			{
				trigger_disconnect = true;
				return_time_freq = IEEE1547_reconnect_time;

				//Flag us as the high under frequency violation
				ieee_1547_trip_method = IEEE_1547_HIGH_UF;
			}
			else
			{
				trigger_disconnect = false;
				return_time_freq = IEEE1547_under_freq_high_band_delay - IEEE1547_under_freq_high_band_viol_time;
			}
		}
		else if ((value_Frequency <= IEEE1547_over_freq_high_band_setpoint) && (value_Frequency > IEEE1547_over_freq_low_band_setpoint))
		{
			//Just update the "high-low" violation time
			IEEE1547_over_freq_low_band_viol_time += timestepvalue;

			//Zero the other one, for good measure
			IEEE1547_over_freq_high_band_viol_time = 0.0;

			//Zero the others, in case we did a huge jump
			IEEE1547_under_freq_high_band_viol_time = 0.0;
			IEEE1547_under_freq_low_band_viol_time = 0.0;

			if (IEEE1547_over_freq_low_band_viol_time > IEEE1547_over_freq_low_band_delay)
			{
				trigger_disconnect = true;
				return_time_freq = IEEE1547_reconnect_time;

				//Flag us as the low over-frequency violation
				ieee_1547_trip_method = IEEE_1547_LOW_OF;
			}
			else
			{
				trigger_disconnect = false;
				return_time_freq = IEEE1547_over_freq_low_band_delay - IEEE1547_over_freq_low_band_viol_time;
			}
		}
		else	//Not sure how we get here in this present logic arrangement - toss an error
		{
			GL_THROW("ibr_gfl:%d - %s - IEEE-1547 Checks - invalid  state!",get_id(),get_name());
			/*  TROUBLESHOOT
			While performing the IEEE 1547-2003 or IEEE 1547a-2014 frequency and voltage checks, an unknown state occurred.  Please
			try again.  If the error persists, please submit you GLM and a bug report via the ticketing system.
			*/

			//Technically not needed - left here in case this ever changes to a non-blocking error
			return -9999.0;
		}
	}
	else	//Must be in a good range
	{
		//Set flags to indicate as much
		frequency_violation = false;
		trigger_disconnect = false;

		//Reset frequency violation counters
		IEEE1547_over_freq_high_band_viol_time = 0.0;
		IEEE1547_over_freq_low_band_viol_time = 0.0;
		IEEE1547_under_freq_high_band_viol_time = 0.0;
		IEEE1547_under_freq_low_band_viol_time = 0.0;

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
			if (indexval == 0)	//Only check on 0, since that's where _12 gets mapped
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
		if (check_phase)
		{
			//See if we're single-phse
			if (parent_is_single_phase)
			{
				//See if it is a violation - all single-phase varieties are mapped to 0
				temp_pu_voltage = value_Circuit_V[0].Mag()/node_nominal_voltage;
			}
			else
			{
				//See if it is a violation
				temp_pu_voltage = value_Circuit_V[indexval].Mag()/node_nominal_voltage;
			}

			//Check it
			if ((temp_pu_voltage < IEEE1547_under_voltage_high_voltage_setpoint) || (temp_pu_voltage > IEEE1547_over_voltage_low_setpoint))
			{
				//flag a violation
				voltage_violation = true;

				//Clear the "no violation timer"
				IEEE1547_out_of_violation_time_total = 0.0;

				//See which case we are
				if (temp_pu_voltage < IEEE1547_under_voltage_lowest_voltage_setpoint)
				{
					//See if we've accumulated yet
					if (!uv_low_hit)
					{
						IEEE1547_under_voltage_lowest_viol_time += timestepvalue;
						uv_low_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else if ((temp_pu_voltage >= IEEE1547_under_voltage_lowest_voltage_setpoint) && (temp_pu_voltage < IEEE1547_under_voltage_middle_voltage_setpoint))
				{

					//See if we've accumulated yet
					if (!uv_mid_hit)
					{
						IEEE1547_under_voltage_middle_viol_time += timestepvalue;
						uv_mid_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else if ((temp_pu_voltage >= IEEE1547_under_voltage_middle_voltage_setpoint) && (temp_pu_voltage < IEEE1547_under_voltage_high_voltage_setpoint))
				{
					//See if we've accumulated yet
					if (!uv_high_hit)
					{
						IEEE1547_under_voltage_high_viol_time += timestepvalue;
						uv_high_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else if ((temp_pu_voltage > IEEE1547_over_voltage_low_setpoint) && (temp_pu_voltage < IEEE1547_over_voltage_high_setpoint))
				{
					//See if we've accumulated yet
					if (!ov_low_hit)
					{
						IEEE1547_over_voltage_low_viol_time += timestepvalue;
						ov_low_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else if (temp_pu_voltage >= IEEE1547_over_voltage_high_setpoint)
				{
					//See if we've accumulated yet
					if (!ov_high_hit)
					{
						IEEE1547_over_voltage_high_viol_time += timestepvalue;
						ov_high_hit = true;
					}
					//Default else, someone else hit us and already accumulated
				}
				else	//must not have tripped a time limit
				{
					GL_THROW("ibr_gfl:%d - %s - IEEE-1547 Checks - invalid  state!",get_id(),get_name());
					//Defined above

					//Technically not needed - left here in case this ever changes to a non-blocking error
					return -9999.0;
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
			if (IEEE1547_under_voltage_lowest_viol_time > IEEE1547_under_voltage_lowest_delay)
			{
				trigger_disconnect = true;
				return_time_volt = IEEE1547_reconnect_time;

				//Flag us as the lowest under voltage violation
				ieee_1547_trip_method = IEEE_1547_LOWEST_UV;
			}
			else if (IEEE1547_under_voltage_middle_viol_time > IEEE1547_under_voltage_middle_delay)	//Check other ranges
			{
				trigger_disconnect = true;
				return_time_volt = IEEE1547_reconnect_time;

				//Flag us as the middle under voltage violation
				ieee_1547_trip_method = IEEE_1547_MIDDLE_UV;
			}

			else if (IEEE1547_under_voltage_high_viol_time > IEEE1547_under_voltage_high_delay)
			{
				trigger_disconnect = true;
				return_time_volt = IEEE1547_reconnect_time;

				//Flag us as the high under voltage violation
				ieee_1547_trip_method = IEEE_1547_HIGH_UV;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = IEEE1547_under_voltage_lowest_delay - IEEE1547_under_voltage_lowest_viol_time;
			}
		}
		else if (uv_mid_hit)
		{
			if (IEEE1547_under_voltage_middle_viol_time > IEEE1547_under_voltage_middle_delay)
			{
				trigger_disconnect = true;
				return_time_volt = IEEE1547_reconnect_time;

				//Flag us as the middle under voltage violation
				ieee_1547_trip_method = IEEE_1547_MIDDLE_UV;
			}
			else if (IEEE1547_under_voltage_high_viol_time > IEEE1547_under_voltage_high_delay)	//Check higher bands
			{
				trigger_disconnect = true;
				return_time_volt = IEEE1547_reconnect_time;

				//Flag us as the high under voltage violation
				ieee_1547_trip_method = IEEE_1547_HIGH_UV;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = IEEE1547_under_voltage_middle_delay - IEEE1547_under_voltage_middle_viol_time;
			}
		}
		else if (uv_high_hit)
		{
			if (IEEE1547_under_voltage_high_viol_time > IEEE1547_under_voltage_high_delay)
			{
				trigger_disconnect = true;
				return_time_volt = IEEE1547_reconnect_time;

				//Flag us as the high under voltage violation
				ieee_1547_trip_method = IEEE_1547_HIGH_UV;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = IEEE1547_under_voltage_high_delay - IEEE1547_under_voltage_high_viol_time;
			}
		}
		else if (ov_low_hit)
		{
			if (IEEE1547_over_voltage_low_viol_time > IEEE1547_over_voltage_low_delay)
			{
				trigger_disconnect = true;
				return_time_volt = IEEE1547_reconnect_time;

				//Flag us as the low over voltage violation
				ieee_1547_trip_method = IEEE_1547_LOW_OV;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = IEEE1547_over_voltage_low_delay - IEEE1547_over_voltage_low_viol_time;
			}
		}
		else if (ov_high_hit)
		{
			if (IEEE1547_over_voltage_high_viol_time > IEEE1547_over_voltage_high_delay)
			{
				trigger_disconnect = true;
				return_time_volt = IEEE1547_reconnect_time;

				//Flag us as the high over voltage violation
				ieee_1547_trip_method = IEEE_1547_HIGH_OV;
			}
			else if (IEEE1547_over_voltage_low_viol_time > IEEE1547_over_voltage_low_delay)	//Lower band overlap
			{
				trigger_disconnect = true;
				return_time_volt = IEEE1547_reconnect_time;

				//Flag us as the low over voltage violation
				ieee_1547_trip_method = IEEE_1547_LOW_OV;
			}
			else
			{
				trigger_disconnect = false;
				return_time_volt = IEEE1547_over_voltage_high_delay - IEEE1547_over_voltage_high_viol_time;
			}
		}
		else	//must not have tripped a time limit
		{
			GL_THROW("ibr_gfl:%d - %s - IEEE-1547 Checks - invalid  state!",get_id(),get_name());
			//Defined above

			//Technically not needed - left here in case this ever changes to a non-blocking error
			return -9999.0;
		}
	}//End of a violation occurred
	else	//No voltage violation
	{
		//Zero all accumulators
		IEEE1547_under_voltage_lowest_viol_time = 0.0;
		IEEE1547_under_voltage_middle_viol_time = 0.0;
		IEEE1547_under_voltage_high_viol_time = 0.0;
		IEEE1547_over_voltage_low_viol_time = 0.0;
		IEEE1547_over_voltage_high_viol_time = 0.0;

		return_time_volt = -1.0;	//Set it again, for paranoia
	}

	//Compute the "next expected update"
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
		IEEE1547_out_of_violation_time_total = 0.0;
	}
	else	//No failures, reset and increment
	{
		//Increment the "restoration" one, just in case
		IEEE1547_out_of_violation_time_total += timestepvalue;
	}

	//See what we are - if we're out of service, see if we can be restored
	if (!inverter_1547_status)
	{
		if (IEEE1547_out_of_violation_time_total > IEEE1547_reconnect_time)
		{
			//Set us back into service
			inverter_1547_status = true;

			//Flag us as no reason
			ieee_1547_trip_method = IEEE_1547_NOTRIP;

			//*********** Warning in place for now - remove this when we update the reconnect behavior properly **************//
			if (Reconnect_Warn_Flag)
			{
				gl_warning("ibr_gfl - Reconnections after an IEEE-1547 cessation are not fully validated.  May cause odd transients.");
				/*  TROUBLESHOOT
				The simple/base IEEE-1547 functionality in the ibr_gfl object only handles the cessation/disconnect side.  Upon reconnecting,
				the proper inverter reconnect behavior is not implemented yet.  Additional transients may apply.  This is expected to be fixed in
				a future update.
				*/

				//Set the flag
				Reconnect_Warn_Flag = false;
			}

			//Implies no violations, return -99.0 to indicate we just restored
			return -99.0;
		}
		else	//Still delayed, just reaffirm our status
		{
			inverter_1547_status = false;

			//calculate the new update time
			return_value = IEEE1547_reconnect_time - IEEE1547_out_of_violation_time_total;

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
			ieee_1547_trip_method = IEEE_1547_NOTRIP;

			//All is well, indicate as much
			return return_value;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_ibr_gfl(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(ibr_gfl::oclass);
		if (*obj != nullptr)
		{
			ibr_gfl *my = OBJECTDATA(*obj, ibr_gfl);
			gl_set_parent(*obj, parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(ibr_gfl);
}

EXPORT int init_ibr_gfl(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj != nullptr)
			return OBJECTDATA(obj, ibr_gfl)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(ibr_gfl);
}

EXPORT TIMESTAMP sync_ibr_gfl(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	ibr_gfl *my = OBJECTDATA(obj, ibr_gfl);
	try
	{
		switch (pass)
		{
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock, t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock, t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass == clockpass)
			obj->clock = t1;
	}
	SYNC_CATCHALL(ibr_gfl);
	return t2;
}

EXPORT int isa_ibr_gfl(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,ibr_gfl)->isa(classname);
}

EXPORT STATUS preupdate_ibr_gfl(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time)
{
	ibr_gfl *my = OBJECTDATA(obj, ibr_gfl);
	STATUS status_output = FAILED;

	try
	{
		status_output = my->pre_deltaupdate(t0, delta_time);
		return status_output;
	}
	catch (char *msg)
	{
		gl_error("preupdate_ibr_gfl(obj=%d;%s): %s", obj->id, (obj->name ? obj->name : "unnamed"), msg);
		return status_output;
	}
}

EXPORT SIMULATIONMODE interupdate_ibr_gfl(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	ibr_gfl *my = OBJECTDATA(obj, ibr_gfl);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time, dt, iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_ibr_gfl(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
		return status;
	}
}

// EXPORT STATUS postupdate_ibr_gfl(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass)
// {
// 	ibr_gfl *my = OBJECTDATA(obj, ibr_gfl);
// 	STATUS status = FAILED;
// 	try
// 	{
// 		status = my->post_deltaupdate(useful_value, mode_pass);
// 		return status;
// 	}
// 	catch (char *msg)
// 	{
// 		gl_error("postupdate_ibr_gfl(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
// 		return status;
// 	}
// }

//// Define export function that update the VSI current injection IGenerated to the grid
EXPORT STATUS ibr_gfl_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure)
{
	STATUS temp_status;

	//Map the node
	ibr_gfl *my = OBJECTDATA(obj, ibr_gfl);

	//Call the function, where we can update the IGenerated injection
	temp_status = my->updateCurrInjection(iteration_count,converged_failure);

	//Return what the sub function said we were
	return temp_status;
}
