#include "ibr_gfm_vsm.h"

CLASS *ibr_gfm_vsm::oclass = nullptr;
ibr_gfm_vsm *ibr_gfm_vsm::defaults = nullptr;

static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
ibr_gfm_vsm::ibr_gfm_vsm(MODULE *module)
{
	if (oclass == nullptr)
	{
		oclass = gl_register_class(module, "ibr_gfm_vsm", sizeof(ibr_gfm_vsm), PC_PRETOPDOWN | PC_BOTTOMUP | PC_POSTTOPDOWN | PC_AUTOLOCK);
		if (oclass == nullptr)
			throw "unable to register class ibr_gfm_vsm";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			PT_enumeration, "grid_forming_mode", PADDR(grid_forming_mode), PT_DESCRIPTION, "grid-forming mode, CONSTANT_DC_BUS or DYNAMIC_DC_BUS",
				PT_KEYWORD, "CONSTANT_DC_BUS", (enumeration)CONSTANT_DC_BUS,
				PT_KEYWORD, "DYNAMIC_DC_BUS", (enumeration)DYNAMIC_DC_BUS,

			PT_enumeration, "P_f_droop_setting_mode", PADDR(P_f_droop_setting_mode), PT_DESCRIPTION, "Definition of P-f droop curve",
				PT_KEYWORD, "FSET_MODE", (enumeration)FSET_MODE,
				PT_KEYWORD, "PSET_MODE", (enumeration)PSET_MODE,

			//** FT Notes These are a bit duplicative - probably delete (change to hidden, at least)
			PT_complex, "phaseA_V[V]", PADDR(value_Circuit_V[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Voltage on A phase in three-phase system",
			PT_complex, "phaseB_V[V]", PADDR(value_Circuit_V[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Voltage on B phase in three-phase system",
			PT_complex, "phaseC_V[V]", PADDR(value_Circuit_V[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "Voltage on C phase in three-phase system",

			PT_complex, "terminal_current_val_PS", PADDR(terminal_current_val_PS), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "positive sequence current",
			PT_complex, "terminal_current_val_NS", PADDR(terminal_current_val_NS), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "negative sequence current",

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
			PT_complex, "VA_Out_PS[VA]", PADDR(VA_Out_PS), PT_DESCRIPTION, "AC positive sequence power",
			PT_complex, "VA_Out_NS[VA]", PADDR(VA_Out_NS), PT_DESCRIPTION, "AC negative sequence power",

			// Internal Voltage and angle of VSI_DROOP, e_source[i],
			PT_complex, "e_source_A", PADDR(e_source[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Actual internal voltage of grid-forming source, phase A",
			PT_complex, "e_source_B", PADDR(e_source[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Actual internal voltage of grid-forming source, phase B",
			PT_complex, "e_source_C", PADDR(e_source[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Actual internal voltage of grid-forming source, phase C",

			// Internal Voltage and angle of VSI_DROOP, e_source[i],
			PT_complex, "e_source_A_PU", PADDR(e_source_pu[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Actual internal voltage of grid-forming source, per unit, phase A",
			PT_complex, "e_source_B_PU", PADDR(e_source_pu[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Actual internal voltage of grid-forming source, per unit, phase B",
			PT_complex, "e_source_C_PU", PADDR(e_source_pu[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Actual internal voltage of grid-forming source, per unit, phase C",

			PT_complex, "e_droop_A", PADDR(e_droop[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of given by the grid-forming droop controller, phase A",
			PT_complex, "e_droop_B", PADDR(e_droop[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of given by the grid-forming droop controller, phase B",
			PT_complex, "e_droop_C", PADDR(e_droop[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of given by the grid-forming droop controller, phase C",

			PT_complex, "e_droop_A_PU", PADDR(e_droop_pu[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of given by the grid-forming droop controller, phase A",
			PT_complex, "e_droop_B_PU", PADDR(e_droop_pu[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of given by the grid-forming droop controller, phase B",
			PT_complex, "e_droop_C_PU", PADDR(e_droop_pu[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of given by the grid-forming droop controller, phase C",


			PT_double, "e_droop_angle_A", PADDR(Angle_blk[0].x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal angle given by the droop controller, phase A",
			PT_double, "e_droop_angle_B", PADDR(Angle_blk[1].x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal angle given by the droop controller, phase B",
			PT_double, "e_droop_angle_C", PADDR(Angle_blk[2].x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal angle given by the droop controller, phase C",


			// 3 phase average value of terminal voltage
			PT_double, "pCircuit_V_Avg_pu", PADDR(pCircuit_V_Avg_pu), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: three-phase average value of terminal voltage, per unit value",
			PT_double, "E_mag", PADDR(E_mag), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: magnitude of internal voltage of grid-forming inverter",

			//Input
			PT_double, "rated_power[VA]", PADDR(S_base), PT_DESCRIPTION, " The rated power of the inverter",
			PT_double, "rated_DC_Voltage[V]", PADDR(Vdc_base), PT_DESCRIPTION, " The rated dc bus of the inverter",

			// Inverter filter parameters
			PT_double, "Xfilter[pu]", PADDR(Xfilter), PT_DESCRIPTION, "DELTAMODE:  per-unit values of inverter filter.",
			PT_double, "Rfilter[pu]", PADDR(Rfilter), PT_DESCRIPTION, "DELTAMODE:  per-unit values of inverter filter.",

			// Dispatch variables
			PT_double,"pdispatch[pu]", PADDR(pdispatch_exp.pdispatch), PT_DESCRIPTION, "Desired generator dispatch set point in p.u.",
			PT_double,"pdispatch_offset[pu]", PADDR(pdispatch_exp.pdispatch_offset), PT_DESCRIPTION, "Desired offset to generator dispatch in p.u.",

			// Grid-Following Controller Parameters
			PT_double, "Pref[W]", PADDR(Pref), PT_DESCRIPTION, "DELTAMODE: The real power reference.",
			PT_double, "Qref[VAr]", PADDR(Qref), PT_DESCRIPTION, "DELTAMODE: The reactive power reference.",
			PT_double, "kpc", PADDR(kpc), PT_DESCRIPTION, "DELTAMODE: Proportional gain of the current loop.",
			PT_double, "kic", PADDR(kic), PT_DESCRIPTION, "DELTAMODE: Integral gain of the current loop.",
			PT_double, "F_current", PADDR(F_current), PT_DESCRIPTION, "DELTAMODE: feed forward term gain in current loop.",
			PT_double, "Tif", PADDR(Tif), PT_DESCRIPTION, "DELTAMODE: time constant of first-order low-pass filter of current loop when using current source representation.",
			
			
			
			//
			PT_double, "Q_out_pu", PADDR(Q_out_pu), PT_DESCRIPTION, "DELTAMODE: Q_out_pu is the per-unit value of VA_Out.Im().",
			PT_double, "CFilt", PADDR(CFilt), PT_DESCRIPTION, "DELTAMODE: CFilt",

			//LIMITS
			PT_double, "I_base", PADDR(I_base), PT_DESCRIPTION, "I_base",

			//Negative sequence
			PT_complex, "value_Circuit_V_PS", PADDR(value_Circuit_V_PS), PT_DESCRIPTION, "Positive sequence voltage",
			PT_complex, "value_Circuit_V_NS", PADDR(value_Circuit_V_NS), PT_DESCRIPTION, "Negative sequence voltage",

			//************************VSM GFM
			PT_double, "delta_wIT", PADDR(delta_wIT), PT_DESCRIPTION, "delta_wIT",

			//GFM or FW function
			PT_double, "Tpf", PADDR(Tpf), PT_DESCRIPTION, "DELTAMODE: the time constant of power measurement low pass filter in frequency-watt.",
			PT_double, "Tff", PADDR(Tff), PT_DESCRIPTION, "DELTAMODE: the time constant of frequency measurement low pass filter in frequency-watt.",
			PT_double, "Tqf", PADDR(Tqf), PT_DESCRIPTION, "DELTAMODE: the time constant of low pass filter in volt-var.",
			PT_double, "Tvf", PADDR(Tvf), PT_DESCRIPTION, "DELTAMODE: the time constant of low pass filter in volt-var.",

			PT_double, "TIf", PADDR(TIf), PT_DESCRIPTION, "DELTAMODE: the time constant of low pass filter in volt-var.",


			PT_double, "Pinv", PADDR(p_measured), PT_DESCRIPTION, "p_measured",
			PT_double, "wm", PADDR(wm), PT_DESCRIPTION, "wm",
			PT_double, "dwm", PADDR(delta_wm), PT_DESCRIPTION, "delta_wm",
			PT_double, "delta_wmin", PADDR(delta_wmin), PT_DESCRIPTION, "delta_wm_min",
			PT_double, "delta_wmax", PADDR(delta_wmax), PT_DESCRIPTION, "delta_wm_max",

			PT_double, "kPF", PADDR(kPF), PT_DESCRIPTION, "kPF",


			PT_double, "AngleIT_min", PADDR(AngleIT_min), PT_DESCRIPTION, "AngleIT_min",
			PT_double, "AngleIT_max", PADDR(AngleIT_max), PT_DESCRIPTION, "AngleIT_max",

			PT_double, "V_ref", PADDR(V_ref), PT_DESCRIPTION, "V_ref",
			PT_double, "Qset", PADDR(Qset), PT_DESCRIPTION, "Qset",
			PT_double, "Qinv", PADDR(q_measured), PT_DESCRIPTION, "q_measured",
			PT_double, "Vinv", PADDR(v_measured), PT_DESCRIPTION, "v_measured",
			PT_double, "Emin", PADDR(Emin), PT_DESCRIPTION, "lower limit in Vreg",
			PT_double, "Emax", PADDR(Emax), PT_DESCRIPTION, "upper limit in Vreg",
			PT_double, "Emin_f", PADDR(Emin_f), PT_DESCRIPTION, "final lower limit in Vreg, for GFM IBR as master application",
			PT_double, "Emax_f", PADDR(Emax_f), PT_DESCRIPTION, "final upper limit in Vreg",

			PT_double, "e_droop", PADDR(e_droop[0]), PT_DESCRIPTION, "upper limit in Vreg",
			PT_double, "w_PLL_A", PADDR(w_PLL[0]), PT_DESCRIPTION, "w_PLL_A",
			PT_double, "delta_w_PLL", PADDR(delta_w_PLL[0]), PT_DESCRIPTION, "delta_w_PLL",
			PT_double, "fPLL_A", PADDR(fPLL[0]), PT_DESCRIPTION, "fPLL_A",

			PT_double, "Idmax", PADDR(Idmax), PT_DESCRIPTION, "Idmax",
			PT_double, "Angle_max", PADDR(Angle_max), PT_DESCRIPTION, "Angle_max",
			PT_double, "Ibase", PADDR(I_base), PT_DESCRIPTION, "Ibase",
			PT_double, "P_out_pu", PADDR(P_out_pu), PT_DESCRIPTION, "P_out_pu",

			PT_double, "Angle_e_A", PADDR(Angle_e[0]), PT_DESCRIPTION, "Angle_e_A",
			PT_double, "Angle_e_B", PADDR(Angle_e[1]), PT_DESCRIPTION, "Angle_e_B",
			PT_double, "Angle_e_C", PADDR(Angle_e[2]), PT_DESCRIPTION, "Angle_e_C",

			PT_double, "ugq_pu_A", PADDR(ugq_pu[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal voltage of grid-following inverter, q-axis, phase A",
			PT_double, "ugq_pu_B", PADDR(ugq_pu[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal voltage of grid-following inverter, q-axis, phase B",
			PT_double, "ugq_pu_C", PADDR(ugq_pu[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal voltage of grid-following inverter, q-axis, phase C",

			PT_double, "igq_pu_A_filter", PADDR(igq_filter_blk[0].x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, q-axis, phase A, current source representation",
			PT_double, "igq_pu_B_filter", PADDR(igq_filter_blk[1].x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, q-axis, phase B, current source representation",
			PT_double, "igq_pu_C_filter", PADDR(igq_filter_blk[2].x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, q-axis, phase C, current source representation",

			PT_double, "igd_ref_A_filter", PADDR(igd_filter_ref[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, d-axis, phase A",
			PT_double, "igd_ref_B_filter", PADDR(igd_filter_ref[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, d-axis, phase B",
			PT_double, "igd_ref_C_filter", PADDR(igd_filter_ref[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, d-axis, phase C",
			PT_double, "igq_ref_A_filter", PADDR(igq_filter_ref[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, q-axis, phase A",
			PT_double, "igq_ref_B_filter", PADDR(igq_filter_ref[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, q-axis, phase B",
			PT_double, "igq_ref_C_filter", PADDR(igq_filter_ref[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, q-axis, phase C",

			PT_double, "Angle_PLL_A", PADDR(Angle_PLL_blk[0].x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: phase angle of terminal voltage measured by PLL, phase A",
			PT_double, "Angle_PLL_B", PADDR(Angle_PLL_blk[1].x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: phase angle of terminal voltage measured by PLL, phase B",
			PT_double, "Angle_PLL_C", PADDR(Angle_PLL_blk[2].x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: phase angle of terminal voltage measured by PLL, phase C",
			PT_double, "f_PLL_A", PADDR(fPLL[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: frequency of terminal voltage measured by PLL, phase A",
			PT_double, "f_PLL_B", PADDR(fPLL[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: frequency of terminal voltage measured by PLL, phase B",
			PT_double, "f_PLL_C", PADDR(fPLL[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: frequency of terminal voltage measured by PLL, phase C",

			PT_double, "Pref_max[pu]", PADDR(Pref_max), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of power references in grid-following mode.",
			PT_double, "Pref_min[pu]", PADDR(Pref_min), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of power references in grid-following mode.",
			PT_double, "Qref_max[pu]", PADDR(Qref_max), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of reactive power references in grid-following mode.",
			PT_double, "Qref_min[pu]", PADDR(Qref_min), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of reactive power references in grid-following mode.",
			PT_double, "Rp[pu]", PADDR(Rp), PT_DESCRIPTION, "DELTAMODE: p-f droop gain in frequency-watt.",
			PT_double, "frequency_watt_droop[pu]", PADDR(Rp), PT_DESCRIPTION, "DELTAMODE: p-f droop gain in frequency-watt.",
			PT_double, "Pref_droop_pu_filter", PADDR(Pref_droop_pu_filter), PT_DESCRIPTION, "DELTAMODE: P setpoint with frequency-watt.",
			PT_double, "db_UF[Hz]", PADDR(db_UF), PT_DESCRIPTION, "DELTAMODE: upper dead band for frequency-watt control, UF for under-frequency",
			PT_double, "db_OF[Hz]", PADDR(db_OF), PT_DESCRIPTION, "DELTAMODE: lower dead band for frequency-watt control, OF for over-frequency",
			PT_double, "Rq[pu]", PADDR(Rq), PT_DESCRIPTION, "DELTAMODE: Q-V droop gain in volt-var.",
			PT_double, "volt_var_droop[pu]", PADDR(Rq), PT_DESCRIPTION, "DELTAMODE: Q-V droop gain in volt-var.",
			PT_double, "db_UV[pu]", PADDR(db_UV), PT_DESCRIPTION, "DELTAMODE: dead band for volt-var control, UV for under-voltage",
			PT_double, "db_OV[pu]", PADDR(db_OV), PT_DESCRIPTION, "DELTAMODE: dead band for volt-var control, OV for over-voltage",
			PT_double, "Qref_droop_pu", PADDR(Qref_droop_pu), PT_DESCRIPTION, "DELTAMODE: Qref calculated by volt-var control",
			PT_double, "V_filter", PADDR(V_filter), PT_DESCRIPTION, "DELTAMODE: Qref calculated by volt-var control",
			PT_double, "Qref_droop_pu_filter", PADDR(Qref_droop_pu_filter), PT_DESCRIPTION, "DELTAMODE: Q setpoint with valt-var.",

			PT_double, "frequency_convergence_criterion[rad/s]", PADDR(GridForming_freq_convergence_criterion), PT_DESCRIPTION, "Max frequency update for grid-forming inverters to return to QSTS",
			PT_double, "voltage_convergence_criterion[V]", PADDR(GridForming_volt_convergence_criterion), PT_DESCRIPTION, "Max voltage update for grid-forming inverters to return to QSTS",

			// PLL Parameters
			PT_double, "kpPLL", PADDR(kpPLL), PT_DESCRIPTION, "DELTAMODE: Proportional gain of the PLL.",
			PT_double, "kiPLL", PADDR(kiPLL), PT_DESCRIPTION, "DELTAMODE: Intregal gain of the PLL.",
			PT_double, "TiPLL", PADDR(TiPLL), PT_DESCRIPTION, "DELTAMODE: Time constant of the PLL.",

			PT_double, "igd_pu_PS", PADDR(igd_pu_PS), PT_DESCRIPTION, "DELTAMODE: igd_pu_PS.",
			PT_double, "igq_pu_PS", PADDR(igq_pu_PS), PT_DESCRIPTION, "DELTAMODE: igq_pu_PS.",
			PT_double, "Id_measured", PADDR(Id_measured), PT_DESCRIPTION, "DELTAMODE: Id_measured.",

			// Grid-Forming Controller Parameters
			PT_double, "Tp", PADDR(Tp), PT_DESCRIPTION, "DELTAMODE: time constant of low pass filter, P calculation.",
			PT_double, "Tq", PADDR(Tq), PT_DESCRIPTION, "DELTAMODE: time constant of low pass filter, Q calculation.",
			PT_double, "Tv", PADDR(Tv), PT_DESCRIPTION, "DELTAMODE: time constant of low pass filter, V calculation.",
			PT_double, "Vset[pu]", PADDR(Vset), PT_DESCRIPTION, "DELTAMODE: voltage set point in grid-forming inverter, usually 1 pu.",
			PT_double, "kpv", PADDR(kpv), PT_DESCRIPTION, "DELTAMODE: proportional gain and integral gain of voltage loop.",
			PT_double, "Tiv", PADDR(Tiv), PT_DESCRIPTION, "DELTAMODE: Integral time constant of voltage loop.",
			PT_double, "mq[pu]", PADDR(mq), PT_DESCRIPTION, "DELTAMODE: Q-V droop gain, usually 0.05 pu.",
			PT_double, "Pset[pu]", PADDR(Pset), PT_DESCRIPTION, "DELTAMODE: power set point in P-f droop.",
			PT_double, "fset[Hz]", PADDR(fset), PT_DESCRIPTION, "DELTAMODE: frequency set point in P-f droop.",
			PT_double, "mp[rad/s/pu]", PADDR(mp), PT_DESCRIPTION, "DELTAMODE: P-f droop gain, usually 3.77 rad/s/pu.",
			PT_double, "P_f_droop[pu]", PADDR(P_f_droop), PT_DESCRIPTION, "DELTAMODE: P-f droop gain in per unit value, usually 0.01.",
			PT_double, "kppmax", PADDR(kppmax), PT_DESCRIPTION, "DELTAMODE: proportional and integral gains for Pmax controller.",
			PT_double, "kipmax", PADDR(kipmax), PT_DESCRIPTION, "DELTAMODE: proportional and integral gains for Pmax controller.",
			PT_double, "w_lim", PADDR(w_lim), PT_DESCRIPTION, "DELTAMODE: saturation limit of Pmax controller.",
			PT_double, "Pmax[pu]", PADDR(Pmax), PT_DESCRIPTION, "DELTAMODE: maximum limit and minimum limit of Pmax controller and Pmin controller.",
			PT_double, "Pmin[pu]", PADDR(Pmin), PT_DESCRIPTION, "DELTAMODE: maximum limit and minimum limit of Pmax controller and Pmin controller.",
			PT_double, "w_ref[rad/s]", PADDR(w_ref), PT_DESCRIPTION, "DELTAMODE: the rated frequency, usually 376.99 rad/s.",
			PT_double, "freq[Hz]", PADDR(freq), PT_DESCRIPTION, "DELTAMODE: the frequency obtained from the P-f droop controller.",
			PT_double, "Imax[pu]", PADDR(Imax), PT_DESCRIPTION, "DELTAMODE: the maximum current of a grid-forming inverter.",
			PT_double, "kpqmax", PADDR(kpqmax), PT_DESCRIPTION, "DELTAMODE: proportional and integral gains for Qmax controller.",
			PT_double, "kiqmax", PADDR(kiqmax), PT_DESCRIPTION, "DELTAMODE: proportional and integral gains for Qmax controller.",
			PT_double, "Qmax[pu]", PADDR(Qmax), PT_DESCRIPTION, "DELTAMODE: maximum limit and minimum limit of Qmax controller and Qmin controller.",
			PT_double, "Qmin[pu]", PADDR(Qmin), PT_DESCRIPTION, "DELTAMODE: maximum limit and minimum limit of Qmax controller and Qmin controller.",
			
			PT_double, "delta_P_transdamping", PADDR(delta_P_transdamping), PT_DESCRIPTION, "DELTAMODE: proportional and integral gains for Qmax controller.",
			PT_double, "delta_P_damping", PADDR(delta_P_damping), PT_DESCRIPTION, "DELTAMODE: proportional and integral gains for Qmax controller.",
			
			PT_double, "delta_w_droop[pu]", PADDR(delta_w_droop), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: delta omega fro p-f droop",
			PT_bool, "VFlag", PADDR(VFlag), PT_DESCRIPTION, "DELTAMODE: Voltage flag to choose between PI control or direct control.",
			PT_double, "Vdc_pu[pu]", PADDR(curr_state.Vdc_pu), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: dc bus voltage of PV panel when using grid-forming PV Inverter",
			PT_double, "Vdc_min_pu[pu]", PADDR(Vdc_min_pu), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: The reference voltage of the Vdc_min controller",
			PT_double, "C_pu[pu]", PADDR(C_pu), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: capacitance of dc bus",
			PT_double, "mdc", PADDR(mdc), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: saturation limit of modulation index",
			PT_double, "kpVdc", PADDR(kpVdc), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: proportional gain of Vdc_min controller",
			PT_double, "kiVdc", PADDR(kiVdc), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: integral gain of Vdc_min controller",
			PT_double, "kdVdc", PADDR(kiVdc), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: derivative gain of Vdc_min controller",

			PT_double, "p_measure", PADDR(Pmeas_blk.x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: filtered active power for grid-forming inverter",
			PT_double, "q_measure", PADDR(Qmeas_blk.x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: filtered reactive power for grid-forming inverter",
			PT_double, "v_measure", PADDR(Vmeas_blk.x[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: filtered voltage for grid-forming inverter",

			//DC Bus portions
			PT_double, "V_In[V]", PADDR(V_DC), PT_DESCRIPTION, "DC input voltage",
			PT_double, "I_In[A]", PADDR(I_DC), PT_DESCRIPTION, "DC input current",
			PT_double, "P_In[W]", PADDR(P_DC), PT_DESCRIPTION, "DC input power",

			PT_double, "pvc_Pmax[W]", PADDR(pvc_Pmax), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "P max from the PV curve",

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
				PT_bool, "phase_angle_correction",PADDR(phase_angle_correction), PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether inverter applies phase angle correction during current limiting",
				PT_bool, "virtual_resistance_correction", PADDR(virtual_resistance_correction),PT_DESCRIPTION,"DELTAMODE: Boolean used to indicate whether inverter applies virtual resistance correction during current limiting",
			nullptr) < 1)
				GL_THROW("unable to publish properties in %s", __FILE__);

		defaults = this;

		memset(this, 0, sizeof(ibr_gfm_vsm));

		if (gl_publish_function(oclass, "preupdate_gen_object", (FUNCTIONADDR)preupdate_ibr_gfm_vsm) == nullptr)
			GL_THROW("Unable to publish ibr_gfm_vsm deltamode function");
		if (gl_publish_function(oclass, "interupdate_gen_object", (FUNCTIONADDR)interupdate_ibr_gfm_vsm) == nullptr)
			GL_THROW("Unable to publish ibr_gfm_vsm deltamode function");
		// if (gl_publish_function(oclass, "postupdate_gen_object", (FUNCTIONADDR)postupdate_ibr_gfm_vsm) == nullptr)
		// 	GL_THROW("Unable to publish ibr_gfm_vsm deltamode function");
		if (gl_publish_function(oclass, "current_injection_update", (FUNCTIONADDR)ibr_gfm_vsm_NR_current_injection_update) == nullptr)
			GL_THROW("Unable to publish ibr_gfm_vsm current injection update function");
		if (gl_publish_function(oclass, "register_gen_DC_object", (FUNCTIONADDR)ibr_gfm_vsm_DC_object_register) == nullptr)
			GL_THROW("Unable to publish ibr_gfm_vsm DC registration function");
	}
}

//Isa function for identification
int ibr_gfm_vsm::isa(char *classname)
{
	return strcmp(classname,"ibr_gfm_vsm")==0;
}

/* Object creation is called once for each object that is created by the core */
int ibr_gfm_vsm::create(void)
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
	pIGenerated[0] = pIGenerated[1] = pIGenerated[2] = nullptr;

	pMeterStatus = nullptr; // check if the meter is in service
	pbus_full_Y_mat = nullptr;
	pGenerated = nullptr;
	pFrequency = nullptr;

	//Zero the accumulators
	value_Circuit_V[0] = value_Circuit_V[1] = value_Circuit_V[2] = gld::complex(0.0, 0.0);
	value_IGenerated[0] = value_IGenerated[1] = value_IGenerated[2] = gld::complex(0.0, 0.0);
	value_MeterStatus = 1; //Connected, by default
	value_Frequency = 60.0;	//Just set it to nominal 

	// Inverter filter
	Xfilter = 0.15; //per unit
	Rfilter = 0.01; // per unit

	// Dispatch setpoints
	pdispatch.pdispatch = -99.0; //essentially flagged as unset
	pdispatch.pdispatch_offset = 0; //default offset is 0
	memcpy(&pdispatch_exp,&pdispatch,sizeof(PDISPATCH));

	// Grid-Forming controller parameters
	Tp = 0.01; // s
	Tq = 0.01; // s
	Tv = 0.01; // s
	P_f_droop = -100;
	kppmax = 3;
	kipmax = 30;
	w_lim = 200; // rad/s
	Pmax = 1.5;
	Pmin = 0;
	w_ref = -99.0;	//Flag value
	f_nominal = 60;
	w_base = 2.0 * PI * f_nominal; //rad/s
	freq = 60;
	fset = -99.0;	//Flag value
	kpqmax = 3;
	kiqmax = 20;
	Qmax = 10;
	Qmin = -10;
	VFlag = 1;  // Default VFlag = 1 so that voltage control PI loop is active
	V_lim = 10;

	// Initial value for previous step frequency deviation
	delta_w_prev_step = 0.0;
	
	// PLL controller parameters
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


	Tif = 0.005; // only used for current source representation

	// Inverter circuit parameters
	CFilt = 111.4e-6;

	//frequency-watt
	Pref_max = 1.0; // per unit
	Pref_min = -1.0;	// per unit
	Rp = 0.0205;//0.1;//1.4;//0.05;		//P-f droop 5%, default value by IEEE 1547 2018
	db_UF = 0.036;  // dead band 0.036 Hz, default value by IEEE 1547 2018
	db_OF = 0.036;  // dead band 0.036 Hz, default value by IEEE 1547 2018

	//volt-var
	db_UV = 0;  // volt-var dead band
	db_OV = 0;  // volt-var dead band

	//Vset = 1;  // per unit
	Qref_max = 1.0; // per unit
	Qref_min = -1.0;	// per unit
	Rq = 1.258;//0.004901;//2.558;//0.039208;//1.258;//0.04545;//1.1628;//0.04545;//1.2933;//0.04545;//0.4;		// per unit, default value by IEEE 1547 2018

	Vdc_base = 850; // default value of dc bus voltage
	Vdc_min_pu = 1; // default reference of the Vdc_min controller

	ImaxF = 1.2;

    LC_frequency = 600;
	// Inverter filter

	Xfilter =  0.15;//0.0847666868075443;//0.15; //per unit
	Rfilter = 0.0201;//0.00165331338418422; // per unit
	R1 = 0.020;//per unit
	L1 = 0.08;//per unit
	L2 = 0.07;//per unit

	kpc = 0.05;
	kic = 5;
	Xfilter =  0.15;//0.0847666868075443;//0.15; //per unit
	Rfilter = 0.0201;//0.00165331338418422; // per unit

	//Q-V loop
	VdrpFlag = 1;

	E_max = 1.15;
	E_min = 0;
	kpv = 1;
	Tiv = 0.075648*E_max;
    XL = L1 + L2;

	Imax = 1.2;//1.5;
	kPF = 0.875;//0.875;
	Idmax = kPF * Imax;

	wFlaga = 1;
	wFlagb = 1;
	wset = 1;
	kb = 1;
	D1 = 0;
	D2 = 100;

	Tidmax = 0.5;
	H = 0.5;
	wD = 50;

	//unbalance
	kpunbl = 0.015;
	kiunbl = 2;


	// Capacitance of dc bus
	C_pu = 0.1;	 //per unit
	kpVdc = 35;	 // per unit, rad/s
	kiVdc = 350; //per unit, rad/s^2
	kdVdc = 0;	 // per unit, rad
	mdc = 1.2;	 //
	GridForming_freq_convergence_criterion = 1e-5;
	GridForming_volt_convergence_criterion = 0.01;

	//Set up the deltamode "next state" tracking variable
	desired_simulation_mode = SM_EVENT;

	//Tracking variable
	last_QSTS_GF_Update = TS_NEVER_DBL;

	//Clear the DC interface list - paranoia
	dc_interface_objects.clear();

	//DC Bus items
	P_DC = 0.0;
	V_DC = Vdc_base;
	I_DC = 0.0;

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

	imax_phase_correction_done[0] = false;
	imax_phase_correction_done[1] = false;
	imax_phase_correction_done[2] = false;
	phase_angle_correction = false; // Phase angle correction off by default
	virtual_resistance_correction = false; // virtual resistance correction off by default

	update_chk_vars();

	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int ibr_gfm_vsm::init(OBJECT *parent)
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
			gl_verbose("ibr_gfm_vsm::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}

	// Data sanity check
	if (S_base <= 0)
	{
		S_base = 1000;
		gl_warning("ibr_gfm_vsm:%d - %s - The rated power of this inverter must be positive - set to 1 kVA.",obj->id,(obj->name ? obj->name : "unnamed"));
		/*  TROUBLESHOOT
		The ibr_gfm_vsm has a rated power that is negative.  It defaulted to a 1 kVA inverter.  If this is not desired, set the property properly in the GLM.
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
						GL_THROW("ibr_gfm_vsm:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
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
							GL_THROW("ibr_gfm_vsm:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", parent->name ? parent->name : "unnamed");
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
							GL_THROW("ibr_gfm_vsm:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
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

			//Simple initial test - if we aren't three-phase, but are grid-forming, toss a warning
			if ((phases & 0x07) != 0x07)
			{
				gl_warning("ibr_gfm_vsm:%s is in grid-forming mode, but is not a three-phase connection.  This is untested and may not behave properly.", obj->name ? obj->name : "unnamed");
				/*  TROUBLESHOOT
				The ibr_gfm_vsm was set up to be grid-forming, but is either a triplex or a single-phase-connected inverter.  This implementaiton is not fully tested and may either not
				work, or produce unexpecte results.
				*/
			}

			//Pull the bus type
			temp_property_pointer = new gld_property(tmp_obj, "bustype");

			//Make sure it worked
			if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_enumeration())
			{
				GL_THROW("ibr_gfm_vsm:%s failed to map bustype variable from %s", obj->name ? obj->name : "unnamed", obj->parent->name ? obj->parent->name : "unnamed");
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
				GL_THROW("ibr_gfm_vsm:%s failed to map measured_frequency variable from %s", obj->name ? obj->name : "unnamed", obj->parent->name ? obj->parent->name : "unnamed");
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

					pIGenerated[1] = nullptr;
					pIGenerated[2] = nullptr;

					if ((phases & 0x07) == 0x01) //A
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_A");
						pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_A");
					}
					else if ((phases & 0x07) == 0x02) //B
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_B");
						pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_B");
					}
					else if ((phases & 0x07) == 0x04) //C
					{
						//Map the various powerflow variables
						pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_C");
						pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_C");
					}
					else //Not three-phase, but has more than one phase - fail, because we don't do this right
					{
						GL_THROW("ibr_gfm_vsm:%s is not connected to a single-phase or three-phase node - two-phase connections are not supported at this time.", obj->name ? obj->name : "unnamed");
						/*  TROUBLESHOOT
						The ibr_gfm_vsm only supports single phase (A, B, or C or triplex) or full three-phase connections.  If you try to connect it differntly than this, it will not work.
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
				GL_THROW("ibr_gfm_vsm:%d %s failed to map the nominal_frequency property", obj->id, (obj->name ? obj->name : "Unnamed"));
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
					GL_THROW("ibr_gfm_vsm:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", tmp_obj->name ? tmp_obj->name : "unnamed");
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
					gl_error("ibr_gfm_vsm:%d %s failed to map the nominal_voltage property", obj->id, (obj->name ? obj->name : "Unnamed"));
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

					CFilt = 1/(2 * PI *LC_frequency)/(2.0 * PI *LC_frequency)/(Z_base/w_base*L1);
					filter_admittance = gld::complex(1.0, 0.0) / (gld::complex(0.0, L2) * Z_base + gld::complex(R1, L1) * Z_base * gld::complex(0.0, -1.0/(w_base*CFilt)) /(gld::complex(R1, L1) * Z_base + gld::complex(0.0, -1.0/(w_base*CFilt)))   );//+gld::complex(0.0, ((2.0 * PI * f_nominal)*CFilt));

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

					filter_admittance = gld::complex(0.0, (w_base*CFilt*3.0));//gld::complex(1.0, 0.0) / (gld::complex(Rfilter, Xfilter) * Z_base );//gld::complex(0.0, (w_base*CFilt*3));//gld::complex(1.0, 0.0) / (gld::complex(Rfilter, Xfilter) * Z_base);// + gld::complex(0.0, 0.0); // (gld::complex(0, 1/((2.0 * PI * f_nominal)*CFilt)));////0.0;

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
					GL_THROW("ibr_gfm_vsm:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", tmp_obj->name ? tmp_obj->name : "unnamed");
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
						GL_THROW("ibr_gfm_vsm:%s exposed Norton-equivalent matrix is the wrong size!", obj->name ? obj->name : "unnamed");
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
						GL_THROW("ibr_gfm_vsm:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
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
							GL_THROW("ibr_gfm_vsm:%s exposed Norton-equivalent matrix is the wrong size!",obj->name?obj->name:"unnamed");
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
				GL_THROW("ibr_gfm_vsm failed to map powerflow status variable");
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

			//See if we're grid-forming and attached to a SWING (and vset is higher)
			if ((attached_bus_type == 2) && (Vset > 0.0))
			{
				//See if the voltage is not 1.0
				if (Vset != 1.0)
				{
					//Compute the magnitude
					temp_volt_mag = Vset * node_nominal_voltage;

					//See if we're single-phase or not
					if (parent_is_single_phase)
					{
						//Pull the angle first, just in case it was intentionally set as off-nominal
						temp_volt_ang[0] = value_Circuit_V[0].Arg();

						//Set the values of the SWING bus to our desired set point
						value_Circuit_V[0].SetPolar(temp_volt_mag,temp_volt_ang[0]);	//A, B, C, or 12 - zero angled
						value_Circuit_V[1] = gld::complex(0.0,0.0);
						value_Circuit_V[2] = gld::complex(0.0,0.0);
					}
					else	//Must be three-phase
					{
						//Pull the angle first, just in case it was intentionally set as off-nominal
						temp_volt_ang[0] = value_Circuit_V[0].Arg();
						temp_volt_ang[1] = value_Circuit_V[1].Arg();
						temp_volt_ang[2] = value_Circuit_V[2].Arg();
						
						//Set values
						value_Circuit_V[0].SetPolar(temp_volt_mag,temp_volt_ang[0]);
						value_Circuit_V[1].SetPolar(temp_volt_mag,temp_volt_ang[1]);
						value_Circuit_V[2].SetPolar(temp_volt_mag,temp_volt_ang[2]);
					}

					//Push the value - voltage update
					push_complex_powerflow_values(true);
				}
				//Default else - it's 1.0, so don't do anything
			}//End Grid-forming, swing, voltage set option
		}	 //End valid powerflow parent
		else //Not sure what it is
		{
			GL_THROW("ibr_gfm_vsm must have a valid powerflow object as its parent, or no parent at all");
			/*  TROUBLESHOOT
			Check the parent object of the inverter.  The ibr_gfm_vsm is only able to be childed via to powerflow objects.
			Alternatively, you can also choose to have no parent, in which case the ibr_gfm_vsm will be a stand-alone application
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

		gl_warning("ibr_gfm_vsm:%d has no parent meter object defined; using static voltages", obj->id);
		/*  TROUBLESHOOT
		An ibr_gfm_vsm in the system does not have a parent attached.  It is using static values for the voltage.
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
			gl_warning("ibr_gfm_vsm:%s indicates it wants to run deltamode, but the module-level flag is not set!", obj->name ? obj->name : "unnamed");
			/*  TROUBLESHOOT
			The ibr_gfm_vsm object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
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
				GL_THROW("ibr_gfm_vsm:%s - failed to add object to generator deltamode object list", obj->name ? obj->name : "unnamed");
				/*  TROUBLESHOOT
				The ibr_gfm_vsm object encountered an issue while trying to add itself to the generator deltamode object list.  If the error
				persists, please submit an issue via GitHub.
				*/
			}
		}
		//Default else - don't do anything

		//Check for frequency 
		if (enable_1547_compliance)
		{
			//grid forming, we don't do these for now
			//Deflag it
			enable_1547_compliance = false;

			//Send a warning
			gl_warning("ibr_gfm_vsm:%d - %s - Grid-forming inverters do not support IEEE 1547 checks at this time - deactivated",get_id(),get_name());
			/*  TROUBLESHOOT
			The IEEE 1547 cessation/tripping functionality is only enabled for grid following inverters right now.  If applied to a grid
			forming inverter, it is just deactivated (and ignored).  This may be enabled in a future update.
			*/
		}
		//Default else - don't do anything
	}	 //End deltamode inclusive
	else //This particular model isn't enabled
	{
		if (enable_subsecond_models)
		{
			gl_warning("ibr_gfm_vsm:%d %s - Deltamode is enabled for the module, but not this inverter!", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The ibr_gfm_vsm is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this ibr_gfm_vsm may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
		
		if (enable_1547_compliance)
		{
			//grid forming, we don't do these for now
			//Deflag it
			enable_1547_compliance = false;

			//Send a warning
			gl_warning("ibr_gfm_vsm:%d - %s - Grid-forming inverters do not support IEEE 1547 checks at this time - deactivated",get_id(),get_name());
			//Defined above
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

	Idc_base = S_base / Vdc_base;

	// Initialize parameters
	if (sqrt(Pref*Pref+Qref*Qref) > S_base)
	{
		gl_warning("ibr_gfm_vsm:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The values for Pref and Qref are specified such that they will exceed teh rated_power of the inverter.  They have been truncated, with real power taking priority.
		*/
	}

	VA_Out = gld::complex(Pref, Qref);

	pvc_Pmax = 0;

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

	// Link P_f_droop to mp
	if (P_f_droop != -100)
	{
		mp = P_f_droop * w_ref;
	}

	pdispatch_sync(); //sync up pdispatch and reference point settings

	return 1;
}

TIMESTAMP ibr_gfm_vsm::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	return t2;
}

TIMESTAMP ibr_gfm_vsm::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret_value;

	FUNCTIONADDR test_fxn;
	STATUS fxn_return_status;

	gld::complex temp_complex_value;
	gld_wlock *test_rlock = nullptr;
	double curr_ts_dbl, diff_dbl, ieee_1547_return_value;
	TIMESTAMP new_ret_value;

	//Assume always want TS_NEVER
	tret_value = TS_NEVER;

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
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
					gl_warning("ibr_gfm_vsm:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
					//Defined above
				}

				temp_complex_value = gld::complex(Pref, Qref);

				//Push it up
				pGenerated->setp<gld::complex>(temp_complex_value, *test_rlock);

				//Map the current injection function
				test_fxn = (FUNCTIONADDR)(gl_get_function(obj->parent, "pwr_current_injection_update_map"));

				//See if it was located
				if (test_fxn == nullptr)
				{
					GL_THROW("ibr_gfm_vsm:%s - failed to map additional current injection mapping for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
					GL_THROW("ibr_gfm_vsm:%s - failed to map additional current injection mapping for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
void ibr_gfm_vsm::update_iGen(gld::complex VA_Out)
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

/* Check the inverter output and make sure it is in the limit */
void ibr_gfm_vsm::check_and_update_VA_Out(OBJECT *obj)
{
	if (grid_forming_mode == DYNAMIC_DC_BUS)
	{
		// Update the P_DC
		P_DC = VA_Out.Re(); //Lossless

		// Update V_DC
		if (!dc_interface_objects.empty())
		{
			int temp_idx;
			STATUS fxn_return_status;

			//Loop through and call the DC objects
			for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
			{
				//DC object, calling object (us), init mode (true/false)
				//False at end now, because not initialization
				fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, false);

				//Make sure it worked
				if (fxn_return_status == FAILED)
				{
					//Pull the object from the array - this is just for readability (otherwise the
					OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

					//Error it up
					GL_THROW("ibr_gfm_vsm:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
					//Defined above
				}
			}
		}

		// Update I_DC
		I_DC = P_DC/V_DC;
	}
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP ibr_gfm_vsm::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2 = TS_NEVER; //By default, we're done forever!

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter)
	{
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

	// Limit check for P_Out & Q_Out
	check_and_update_VA_Out(obj);

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Preupdate
STATUS ibr_gfm_vsm::pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time)
{
	STATUS stat_val;
	FUNCTIONADDR funadd = nullptr;
	OBJECT *hdr = OBJECTHDR(this);

	//Call the init, since we're here
	stat_val = init_dynamics(&curr_state);

	if (stat_val != SUCCESS)
	{
		gl_error("ibr_gfm_vsm failed pre_deltaupdate call");
		/*  TROUBLESHOOT
		While attempting to call the pre_deltaupdate portion of the ibr_gfm_vsm code, an error
		was encountered.  Please submit your code and a bug report via the ticketing system.
		*/

		return FAILED;
	}

	//If we're a voltage-source inverter, also swap our SWING bus, just because
	//map the function
	funadd = (FUNCTIONADDR)(gl_get_function(hdr->parent, "pwr_object_swing_swapper"));

	//make sure it worked
	if (funadd == nullptr)
	{
		gl_error("ibr_gfm_vsm:%s -- Failed to find node swing swapper function", (hdr->name ? hdr->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map the function to change the swing status of the parent bus, the function could not be found.
		Ensure the ibr_gfm_vsm is actually attached to something.  If the error persists, please submit your code and a bug report
		via the ticketing/issues system.
		*/

		return FAILED;
	}

	//Call the swap
	stat_val = ((STATUS(*)(OBJECT *, bool))(*funadd))(hdr->parent, false);

	if (stat_val == 0) //Failed :(
	{
		gl_error("Failed to swap SWING status of node:%s on ibr_gfm_vsm:%s", (hdr->parent->name ? hdr->parent->name : "Unnamed"), (hdr->name ? hdr->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
		failed to execute properly.  If the problem persists, please submit a bug report and your code to the trac website.
		*/

		return FAILED;
	}

	//Reset the QSTS criterion flag
	deltamode_exit_iteration_met = false;

	//Just return a pass - not sure how we'd fail
	return SUCCESS;
}

//Module-level call
SIMULATIONMODE ibr_gfm_vsm::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
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
		pull_complex_powerflow_values();
	}

	//Get timestep value
	deltat = (double)dt / (double)DT_SECOND;
	deltath = deltat / 2.0;

	//Update time tracking variables
	prev_timestamp_dbl = gl_globaldeltaclock;
	prev_time_dbl_IEEE1547 = prev_timestamp_dbl;	//Just update - won't hurt anything if it isn't needed

	//Perform the 1547 update, if enabled
	if (enable_1547_compliance && (iteration_count_val == 0))	//Always just do on the first pass
	{
		//Do the checks
		ieee_1547_delta_return = perform_1547_checks(deltat);
	}

	pdispatch_sync(); //sync up setpoints and pdispatch

	if (P_f_droop_setting_mode == PSET_MODE) //Define using power set point at rated frequency
	{
		fset = f_nominal;
	}
	else if (P_f_droop_setting_mode == FSET_MODE) //Define using frequency set point at no load
	{
		Pset = 0;
	}

	// Link P_f_droop to mp
	if (P_f_droop != -100)
	{
		mp = P_f_droop * w_ref;
	}

	if ((iteration_count_val == 0) && (delta_time == 0) && (grid_forming_mode == DYNAMIC_DC_BUS))
	{
		P_DC = I_DC = 0; // Clean the buffer, only on the very first delta timestep
	}

	//See if we just came from QSTS and not the first timestep
	if ((prev_timestamp_dbl == last_QSTS_GF_Update) && (delta_time == 0))
	{
		//Technically, QSTS already updated us for this timestamp, so skip
		desired_simulation_mode = SM_DELTA;	//Go forward

		//In order to keep the rest of the inverter up-to-date, call the DC bus routines
		if (grid_forming_mode == DYNAMIC_DC_BUS)
		{
			I_dc_pu = P_out_pu / curr_state.Vdc_pu; // Calculate the equivalent dc current, including the dc capacitor

			if (!dc_interface_objects.empty())
			{
				//Update V_DC, just in case
				V_DC = curr_state.Vdc_pu * Vdc_base;

				//Loop through and call the DC objects
				for (temp_dc_idx = 0; temp_dc_idx < dc_interface_objects.size(); temp_dc_idx++)
				{
					//DC object, calling object (us), init mode (true/false)
					//False at end now, because not initialization
					fxn_DC_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_dc_idx].fxn_address))(dc_interface_objects[temp_dc_idx].dc_object, obj, false);

					//Make sure it worked
					if (fxn_DC_status == FAILED)
					{
						//Pull the object from the array - this is just for readability (otherwise the
						temp_DC_obj = dc_interface_objects[temp_dc_idx].dc_object;

						//Error it up
						GL_THROW("ibr_gfm_vsm:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_DC_obj->id, (temp_DC_obj->name ? temp_DC_obj->name : "Unnamed"));
						//Defined elsewhere
					}
				}

				//I_DC is done now.  If P_DC isn't, it could be calculated
			} //End DC object update
		}//End DYNAMIC_DC_BUS

		return SM_DELTA;	//Short circuit it
	}

	// Check pass
	if (iteration_count_val == 0) // Predictor pass
	{
		//Calculate injection current based on voltage source magnitude and angle obtained
		if (parent_is_single_phase) // single phase/split-phase implementation
		{
			//Update output power
			//Get current injected
			terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];
			terminal_current_val_pu[0] = terminal_current_val[0]/I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
			VA_Out = power_val[0];

			// The following code is only for three phase system
			// Function: Low pass filter of P
			P_out_pu = VA_Out.Re() / S_base;
			// Output of active power measurement block
			p_measured = Pmeas_blk.getoutput(P_out_pu,deltat,PREDICTOR);
			
			// VA_OUT.Re() refers to the output active power from the inverter.
			// S_base is the rated capacity
			// P_out_pu is the per unit value of VA_OUT.Re()
			// p_measure is the filtered active power, it is per-unit value
			// Tp is the time constant of low pass filter, it is per-unit value
			// Function end

			// Function: Low pass filter of Q
			Q_out_pu = VA_Out.Im() / S_base;
			// Output of reactive power measurement block
			q_measured = Qmeas_blk.getoutput(Q_out_pu,deltat,PREDICTOR);
			
			// VA_OUT.Im() refers to the output reactive power from the inverter
			// Q_out_pu is the per-unit value of VA_Out.Im()
			// q_measure is the filtered reactive power, it is per-unit value
			// Tq is the time constant of low pass filter, it is per-unit value
			// Function end

			if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus
			{
				I_dc_pu = P_out_pu / curr_state.Vdc_pu; // Calculate the equivalent dc current, including the dc capacitor

				if (!dc_interface_objects.empty())
				{
					int temp_idx;
					//V_DC was set here, somehow
					STATUS fxn_return_status;

					V_DC = curr_state.Vdc_pu * Vdc_base;

					//Loop through and call the DC objects
					for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
					{
						//DC object, calling object (us), init mode (true/false)
						//False at end now, because not initialization
						fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, false);

						//Make sure it worked
						if (fxn_return_status == FAILED)
						{
							//Pull the object from the array - this is just for readability (otherwise the
							OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

							//Error it up
							GL_THROW("ibr_gfm_vsm:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
							//Defined above
						}
					}

					//I_DC is done now.  If P_DC isn't, it could be calculated
				} //End DC object update

				I_PV_pu = I_DC / Idc_base; // Calculate the current from PV panel

				pred_state.dVdc_pu = (I_PV_pu - I_dc_pu) / C_pu;

				pred_state.Vdc_pu = curr_state.Vdc_pu + pred_state.dVdc_pu * deltat;
			}

			// Function: Low pass filter of V
			pCircuit_V_Avg_pu = value_Circuit_V[0].Mag() / V_base;
			// Output of V-measurement block
			v_measured = Vmeas_blk.getoutput(pCircuit_V_Avg_pu,deltat,PREDICTOR);
			
			// Value_Circuit_V[i] refers to the voltage of each phase at the grid side
			// Vbase is the rated Line to ground voltage
			// pCircuit_V_Avg_pu refers to the average value of three phase voltages, it is per-unit value
			// v_measure refers to filtered voltage, it is per-unit value
			// Tv is the time constant of low pass filter
			// Function end

			// Qmax controller
			delta_V_Qmax = Qmax_ctrl_blk.getoutput(Qmax - q_measured,deltat,PREDICTOR);

			// Qmin controller
			delta_V_Qmin = Qmin_ctrl_blk.getoutput(Qmin - q_measured,deltat,PREDICTOR);

			// Function: Q-V droop control and voltage control loop
			V_ref = Vset - q_measured * mq + delta_V_Qmax + delta_V_Qmin;

			if(grid_forming_mode == DYNAMIC_DC_BUS)
			{
					if(VFlag) {
					E_mag = V_ctrl_blk.getoutput(V_ref - v_measured, deltat,E_min, pred_state.Vdc_pu*mdc,E_min,pred_state.Vdc_pu*mdc,PREDICTOR);
				
					// V_ref is the voltage reference obtained from Q-V droop
					// Vset is the voltage set point, usually 1 pu
					// mq is the Q-V droop gain, usually 0.05 pu
					// V_ini is the output from the integrator in the voltage controller
					// E_mag is the output of the votlage controller, it is the voltage magnitude of the internal voltage
					// E_max and E_min are the maximum and minimum of the output of voltage controller
				} else {
					E_mag = std::min(std::max(E_min,V_ref),E_max);
				}
				// Function end
			}
			else
			{
					if(VFlag) {
					E_mag = V_ctrl_blk.getoutput(V_ref-v_measured, deltat,PREDICTOR);

				// V_ref is the voltage reference obtained from Q-V droop
				// Vset is the voltage set point, usually 1 pu
				// mq is the Q-V droop gain, usually 0.05 pu
				// V_ini is the output from the integrator in the voltage controller
				// E_mag is the output of the votlage controller, it is the voltage magnitude of the internal voltage
				// E_max and E_min are the maximum and minimum of the output of voltage controller
				// Function end
				} else {
					E_mag = std::min(std::max(E_min,V_ref),E_max);
				}
			}

			// Function: P-f droop, Pmax and Pmin controller
			delta_w_droop = (Pset - p_measured) * mp; // P-f droop

			// Pmax controller
			delta_w_Pmax = Pmax_ctrl_blk.getoutput(Pmax - p_measured,deltat,PREDICTOR);
			
			// Pmin controller
			delta_w_Pmin = Pmin_ctrl_blk.getoutput(Pmin - p_measured,deltat,PREDICTOR);

			delta_w = delta_w_droop + delta_w_Pmax + delta_w_Pmin + 2.0 * PI * fset - w_ref; //the summation of the outputs from P-f droop, Pmax control and Pmin control

			// delta_w_droop is the output of P-f droop
			// Pset is the power set point
			// delta_w_Pmax and delta_w_Pmin are the outputs of Pmax controller and Pmin controller
			// Pmax and Pmin are the maximum limit and minimum limit of Pmax controller and Pmin controller
			// w_lim is the saturation limit
			// w_ref is the rated frequency, usually 376.99 rad/s
			// Function end

			if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
			{

				// Vdc_min controller to protect the dc bus voltage from collapsing
				pred_state.ddelta_w_Vdc_min_ini = pred_state.Vdc_pu - Vdc_min_pu ;
				pred_state.delta_w_Vdc_min_ini = curr_state.delta_w_Vdc_min_ini + pred_state.ddelta_w_Vdc_min_ini * kiVdc * deltat;

				if (pred_state.delta_w_Vdc_min_ini > 0) //
				{
					pred_state.delta_w_Vdc_min_ini = 0;
				}

				delta_w_Vdc_min = pred_state.delta_w_Vdc_min_ini + pred_state.ddelta_w_Vdc_min_ini * kpVdc + (pred_state.Vdc_pu - curr_state.Vdc_pu) * kdVdc / deltat; // output from Vdc_min controller

				if (delta_w_Vdc_min > 0) //
				{
					delta_w_Vdc_min = 0;
				}

				delta_w = delta_w + delta_w_Vdc_min; //the summation of the outputs from P-f droop, Pmax control and Pmin control, and Vdc_min control
			}

			freq = (delta_w + w_ref) / 2.0 / PI; // The frequency from the CERTS Droop controller, Hz

			// Function: Obtaining the Phase Angle, and obtaining the compelx value of internal voltages and their Norton Equivalence for power flow analysis
			for (i = 0; i < 1; i++)
			{
				//Obtain the phase angle
				Angle[i] = Angle_blk[i].getoutput(delta_w,deltat,PREDICTOR);
				
				e_droop_pu[i] = gld::complex(E_mag * cos(Angle[i]), E_mag * sin(Angle[i])); // per unit value of the internal voltage given by droop control
				e_droop[i] = e_droop_pu[i] * V_base; // internal voltage given by the droop control

				value_IGenerated[i] = e_droop[i] / (gld::complex(Rfilter, Xfilter) * Z_base);							// Thevenin voltage source to Norton current source convertion
			}

			// Angle[i] refers to the phase angle of internal voltage for each phase
			// e_source[i] is the complex value of internal voltage
			// value_IGenerated[i] is the Norton equivalent current source of e_source[i]
			// Rfilter and Xfilter are the per-unit values of inverter filter, they are per-unit values
			// Function end

			simmode_return_value = SM_DELTA_ITER; //Reiterate - to get us to corrector pass
		}
		else //Three-phase
		{
			//Update output power
			//Get current injected

			terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
			terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
			terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

			terminal_current_val_pu[0] = terminal_current_val[0]/I_base;
			terminal_current_val_pu[1] = terminal_current_val[1]/I_base;
			terminal_current_val_pu[2] = terminal_current_val[2]/I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
			power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
			power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

			VA_Out = power_val[0] + power_val[1] + power_val[2];

			// The following code is only for three phase system
			// Function: Phase-Lock_Loop, PLL, only consider positive sequence voltage

			// Obtain the positive sequence voltage
			value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
			value_Circuit_V_NS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI))) / 3.0;
			terminal_current_val_PS = (terminal_current_val[0] + terminal_current_val[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + terminal_current_val[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
			terminal_current_val_NS = (terminal_current_val[0] + terminal_current_val[1] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI)) + terminal_current_val[2] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI))) / 3.0;
			VA_Out_PS = gld::complex(3.0, 0) * (value_Circuit_V_PS * ~terminal_current_val_PS);
			VA_Out_NS = gld::complex(3.0, 0) * (value_Circuit_V_NS * ~terminal_current_val_NS);

			for (i = 0; i < 3; i++)
			{
				ugq_pu[i] = (-value_Circuit_V_PS.Re() * sin(Angle_PLL[i]) + value_Circuit_V_PS.Im() * cos(Angle_PLL[i])) / V_base;
			}

			for (i = 0; i < 3; i++)
			{
				delta_w_PLL[i] = ( ugq_pu[i] * kpPLL + delta_w_PLL_blk_gfm[i].getoutput(ugq_pu[i],deltat,PREDICTOR)) * w_base;
				w_PLL[i] = delta_w_PLL[i] + w_ref;
				fPLL[i] = w_PLL[i] / 2.0 / PI;														// frequency measured by PLL
				Angle_PLL[i] = Angle_PLL_blk[i].getoutput(delta_w_PLL[i],deltat,PREDICTOR); 	// phase angle from PLL
			}

			//fPLL[2] = fPLL[1] = fPLL[0];
			// delta_w_PLL[i] is the output from the PI controller
			// w_ref is the rated angular frequency, the value is 376.99 rad/s
			// fPLL is the frequency measured by PLL
			// Function end

			// Positive sequence value of current in dq frame
			igd_pu_PS = (terminal_current_val_PS.Re() * cos(Angle_PLL[0]) + terminal_current_val_PS.Im() * sin(Angle_PLL[0])) / I_base;
			igq_pu_PS = (-terminal_current_val_PS.Re() * sin(Angle_PLL[0]) + terminal_current_val_PS.Im() * cos(Angle_PLL[0])) / I_base;

			// Negative sequence value of current in dq frame
			igd_pu_NS = (terminal_current_val_NS.Re() * cos(-Angle_PLL[0]) + terminal_current_val_NS.Im() * sin(-Angle_PLL[0])) / I_base;
			igq_pu_NS = (-terminal_current_val_NS.Re() * sin(-Angle_PLL[0]) + terminal_current_val_NS.Im() * cos(-Angle_PLL[0])) / I_base;

			Id_measured = Idmeas_blk.getoutput(igd_pu_PS,deltat,PREDICTOR);
			Iq_measured = Iqmeas_blk.getoutput(igq_pu_PS,deltat,PREDICTOR);

			// The following code is only for three phase system
			// Function: Low pass filter of P
			P_out_pu = VA_Out.Re() / S_base;
			// Output of P-measurement block
			p_measured = Pmeas_blk.getoutput(P_out_pu,deltat,PREDICTOR);
			
			// VA_OUT.Re() refers to the output active power from the inverter.
			// S_base is the rated capacity
			// P_out_pu is the per unit value of VA_OUT.Re()
			// p_measure is the filtered active power, it is per-unit value
			// Tp is the time constant of low pass filter, it is per-unit value
			// Function end

			// Function: Low pass filter of Q
			Q_out_pu = VA_Out.Im() / S_base;
			q_measured = Qmeas_blk.getoutput(Q_out_pu,deltat,PREDICTOR);

			// VA_OUT.Im() refers to the output reactive power from the inverter
			// Q_out_pu is the per-unit value of VA_Out.Im()
			// q_measure is the filtered reactive power, it is per-unit value
			// Tq is the time constant of low pass filter, it is per-unit value
			// Function end

			if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus
			{
				I_dc_pu = P_out_pu / curr_state.Vdc_pu; // Calculate the equivalent dc current, including the dc capacitor

				if (!dc_interface_objects.empty())
				{
					int temp_idx;
					//V_DC was set here, somehow
					STATUS fxn_return_status;

					V_DC = curr_state.Vdc_pu * Vdc_base;

					//Loop through and call the DC objects
					for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
					{
						//DC object, calling object (us), init mode (true/false)
						//False at end now, because not initialization
						fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, false);

						//Make sure it worked
						if (fxn_return_status == FAILED)
						{
							//Pull the object from the array - this is just for readability (otherwise the
							OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

							//Error it up
							GL_THROW("ibr_gfm_vsm:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
							//Defined above
						}
					}

					//I_DC is done now.  If P_DC isn't, it could be calculated
				} //End DC object update

				I_PV_pu = I_DC / Idc_base; // Calculate the current from PV panel

				pred_state.dVdc_pu = (I_PV_pu - I_dc_pu) / C_pu;

				pred_state.Vdc_pu = curr_state.Vdc_pu + pred_state.dVdc_pu * deltat;
			}

			// Function: Low pass filter of V
			pCircuit_V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;
			// Output of V-measurement block
			v_measured = Vmeas_blk.getoutput(pCircuit_V_Avg_pu,deltat,PREDICTOR);

			Iqmax = sqrt(Imax*Imax - Id_measured*Id_measured);
			Emin = sqrt( (v_measured - Iqmax * XL) * (v_measured - Iqmax * XL) + (Id_measured * XL) * (Id_measured * XL));
			//XL	Inverter coupling reactance
			Emax = sqrt( (v_measured + Iqmax * XL) * (v_measured + Iqmax * XL) + (Id_measured * XL) * (Id_measured * XL));

			// Active current limiting

			Angle_max = asin(XL * Imax);
			AngleIT_max = Anglemax_blk.getoutput(Idmax - Id_measured,deltat,PREDICTOR);//10000;//Anglemax_blk.getoutput(Idmax - Id_measured,deltat,PREDICTOR);
			AngleIT_min = -1.0 * AngleIT_max;//-10000;//-1.0 * AngleIT_max;

			V_ctrl_blk.setparams(Tiv,E_min,E_max,Emin,Emax);

			// Value_Circuit_V[i] refers to the voltage of each phase at the grid side
			// Vbase is the rated Line to ground voltage
			// pCircuit_V_Avg_pu refers to the average value of three phase voltages, it is per-unit value
			// v_measure refers to filtered voltage, it is per-unit value
			// Tv is the time constant of low pass filter
			// Function end

			// Leave the space for Vdrp Flag
			// Two options: Option 1: Q-V droop; Option 0: I-V droop
			// Function: Q-V droop control and voltage control loop

			if (VdrpFlag == 1)
			{
				V_ref = Vset + (Qset- q_measured) * mq;// + delta_V_Qmax + delta_V_Qmin; //
			}
			else
			{
				V_ref = Vset - Iq_measured * mq; // include PLL
			}

			if(grid_forming_mode == DYNAMIC_DC_BUS)
			{
				E_mag = V_ctrl_blk.getoutput(V_ref - v_measured, deltat,E_min, pred_state.Vdc_pu*mdc,E_min,pred_state.Vdc_pu*mdc,PREDICTOR);
			}
			else
			{
				E_mag = (V_ref-v_measured)*kpv + V_ctrl_blk.getoutput(V_ref-v_measured, deltat,PREDICTOR);

				if (E_mag > Emax)
				{
					E_mag = Emax;
				}
				else if (E_mag < Emin)
				{
					E_mag = Emin;
				}

				// V_ref is the voltage reference obtained from Q-V droop
				// Vset is the voltage set point, usually 1 pu
				// mq is the Q-V droop gain, usually 0.05 pu
				// V_ini is the output from the integrator in the voltage controller
				// E_mag is the output of the votlage controller, it is the voltage magnitude of the internal voltage
				// E_max and E_min are the maximum and minimum of the output of voltage controller
				// Function end
			}

			//Add another saturation limits as the final ones
			if (E_mag > Emax_f)
			{
				E_mag = Emax_f;
			}
			else if (E_mag < Emin_f)
			{
				E_mag = Emin_f;
			}

			// wFlaga Flag, the flag to select whether to include the PLL as the input for the P-f droop or not
			// Two options: Option 1: wm; Option 0: wPLL
			// wm is the command output from VSM
			// wPLL is the frequency from PLL loop
			// mp is P-f droop gain
			// Kb is A binary value to determine if the P-f droop is enabled or not
			if (wFlaga == 1)
			{
				delta_P_droop = (wset - wm) * kb / mp; // not include PLL
			}
			else
			{
				delta_P_droop = (wset - w_PLL[0] / w_base) * kb / mp; // include PLL
			}

			if (Tp == 0)
			{
				Pref = Pset + delta_P_droop;
			}
			else
			{
				Pref = Pset + deltaP_droop_blk.getoutput(delta_P_droop,deltat,PREDICTOR);
			}

			// Transient damping loop
			delta_P_transdamping = D2*delta_wm - D2 * delta_P_transdamping_blk.getoutput(delta_wm,deltat,PREDICTOR);
			// use Filter funciton    1/(1+ s/wD)

			// Damping loop
			delta_P_damping = D1*delta_wm;

			//delta_wm is the output from VSM, use integral function, 1/(sT), T = 2H
			delta_wm_blk.setparams(2*H,-100,100,delta_wmin, delta_wmax);
			delta_wm = delta_wm_blk.getoutput(Pref - p_measured - delta_P_transdamping - delta_P_damping,deltat,PREDICTOR);

			wm = 1 + delta_wm;

			//Ï‰Flagb	The flag to select whether the PLL should be included for limiting the angle or not
			// Two options: Option 1: 0; Option 0: delta_wPLL

			if (wFlagb == 1)
			{
				delta_wIT = delta_wm; // not include PLL
			}
			else
			{
				delta_wIT = delta_wm - delta_w_PLL[0] / w_base; // include PLL
			}

			// delta_w_droop is the output of P-f droop
			// Pset is the power set point
			// delta_w_Pmax and delta_w_Pmin are the outputs of Pmax controller and Pmin controller
			// Pmax and Pmin are the maximum limit and minimum limit of Pmax controller and Pmin controller
			// w_lim is the saturation limit
			// w_ref is the rated frequency, usually 376.99 rad/s
			// Function end

			// Function: Obtaining the Phase Angle, and obtaining the compelx value of internal voltages and their Norton Equivalence for power flow analysis
			for (i = 0; i < 3; i++)
			{
				if (i == 0)
				{
					Angle_blk[i].setparams(1,-1000.0,1000.0,AngleIT_min,AngleIT_max);
				}
				else if (i == 1)
				{
					Angle_blk[i].setparams(1,-1000.0,1000.0,AngleIT_min - 4.0 * PI / 3.0,AngleIT_max + 4.0 * PI / 3.0);
				}
				else
				{
					Angle_blk[i].setparams(1,-1000.0,1000.0,AngleIT_min - 2.0 * PI / 3.0,AngleIT_max + 2.0 * PI / 3.0);
				}

				Angle_e[i] = Angle_blk[i].getoutput(delta_wIT * w_base,deltat,PREDICTOR);

				if (wFlagb == 1)
				{
					Angle[i] = Angle_e[i]; // not include PLL
					//// w_ref is the rated frequency, usually 376.99 rad/s
				}
				else
				{
					Angle[i] = Angle_e[i]  +  Angle_PLL[i]; // include PLL
				}
				
				e_droop_pu[i] = gld::complex(E_mag * cos(Angle[i]), E_mag * sin(Angle[i])); // per unit value of the internal voltage given by droop control
				e_droop[i] = e_droop_pu[i] * V_base; // internal voltage given by the droop control

				//value_IGenerated[i] = e_droop[i] / (gld::complex(Rfilter, Xfilter) * Z_base);//  + e_droop[i] * gld::complex(0.0, ((2.0 * PI * f_nominal)*CFilt));							  // Thevenin voltage source to Norton current source convertion
				value_IGenerated[i] = e_droop[i] / ((gld::complex(R1, L1) * Z_base) + (gld::complex(0, L2) * Z_base)*(gld::complex(0, -1/(w_base*CFilt)))/(gld::complex(0, L2) * Z_base + gld::complex(0, -1/(w_base*CFilt))) )   *  gld::complex(0, -1/(w_base*CFilt))/ (gld::complex(0, L2) * Z_base + gld::complex(0, -1/(w_base*CFilt)));//+ e_droop[i] * gld::complex(0.0, ((2.0 * PI * f_nominal)*CFilt));							// Thevenin voltage source to Norton current source convertion
			}

			// Angle[i] refers to the phase angle of internal voltage for each phase
			// e_source[i] is the complex value of internal voltage
			// value_IGenerated[i] is the Norton equivalent current source of e_source[i]
			// Rfilter and Xfilter are the per-unit values of inverter filter, they are per-unit values
			// Function end

			simmode_return_value = SM_DELTA_ITER; //Reiterate - to get us to corrector pass
		}
	}								   // end of grid-forming, predictor pass
	else if (iteration_count_val == 1) // Corrector pass
	{
		if (parent_is_single_phase) // single phase/split-phase implementation
		{
			//Update output power
			//Get current injected
			terminal_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];
			terminal_current_val_pu[0] = terminal_current_val[0]/I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
			VA_Out = power_val[0];

			// The following code is only for three phase system
			// Function: Low pass filter of P
			P_out_pu = VA_Out.Re() / S_base;
			// Output of P-measurement block
			p_measured = Pmeas_blk.getoutput(P_out_pu,deltat,CORRECTOR);

			// VA_OUT.Re() refers to the output active power from the inverter, this should be normalized.
			// S_base is the rated capacity
			// P_out_pu is the per unit value of VA_OUT.Re()
			// p_measure is the filtered active power, it is per-unit value
			// Tp is the time constant of low pass filter, it is per-unit value
			// Function end

			// Function: Low pass filter of Q
			Q_out_pu = VA_Out.Im() / S_base;
			q_measured = Qmeas_blk.getoutput(Q_out_pu,deltat,CORRECTOR);
			// VA_OUT.Im() refers to the output reactive power from the inverter
			// Q_out_pu is the per-unit value of VA_Out.Im()
			// q_measure is the filtered reactive power, it is per-unit value
			// Tq is the time constant of low pass filter, it is per-unit value
			// Function end

			if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus
			{
				I_dc_pu = P_out_pu / pred_state.Vdc_pu; // Calculate the equivalent dc current, including the dc capacitor

				if (!dc_interface_objects.empty())
				{
					int temp_idx;
					//V_DC was set here, somehow
					STATUS fxn_return_status;

					V_DC = pred_state.Vdc_pu * Vdc_base;

					//Loop through and call the DC objects
					for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
					{
						//DC object, calling object (us), init mode (true/false)
						//False at end now, because not initialization
						fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, false);

						//Make sure it worked
						if (fxn_return_status == FAILED)
						{
							//Pull the object from the array - this is just for readability (otherwise the
							OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

							//Error it up
							GL_THROW("ibr_gfm_vsm:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
							//Defined above
						}
					}

					//I_DC is done now.  If P_DC isn't, it could be calculated
				} //End DC object update

				I_PV_pu = I_DC / Idc_base; // Calculate the current from PV panel

				next_state.dVdc_pu = (I_PV_pu - I_dc_pu) / C_pu;

				next_state.Vdc_pu = curr_state.Vdc_pu + (pred_state.dVdc_pu + next_state.dVdc_pu) * deltat / 2.0;
			}

			// Function: Low pass filter of V
			pCircuit_V_Avg_pu = value_Circuit_V[0].Mag() / V_base;
			// Output of V-measurement block 
			v_measured = Vmeas_blk.getoutput(pCircuit_V_Avg_pu,deltat,CORRECTOR);

			// Value_Circuit_V[i] refers to te voltage of each phase at the inverter terminal
			// Vbase is the rated Line to ground voltage
			// pCircuit_V_Avg_pu refers to the average value of three phase voltages, it is per-unit value
			// v_measure refers to filtered voltage, it is per-unit value
			// Function end

			//Qmax controller
			delta_V_Qmax = Qmax_ctrl_blk.getoutput(Qmax - q_measured,deltat,CORRECTOR);

			//Qmin controller
			delta_V_Qmin = Qmin_ctrl_blk.getoutput(Qmin - q_measured,deltat,CORRECTOR);

			// Function: Q-V droop control and voltage control loop
			V_ref = Vset - q_measured * mq + delta_V_Qmax + delta_V_Qmin;

			if(grid_forming_mode == DYNAMIC_DC_BUS)
			{
					if(VFlag) {
					E_mag = V_ctrl_blk.getoutput(V_ref - v_measured, deltat,E_min, pred_state.Vdc_pu*mdc,E_min,pred_state.Vdc_pu*mdc,CORRECTOR);
				
					// V_ref is the voltage reference obtained from Q-V droop
					// Vset is the voltage set point, usually 1 pu
					// mq is the Q-V droop gain, usually 0.05 pu
					// V_ini is the output from the integrator in the voltage controller
					// E_mag is the output of the votlage controller, it is the voltage magnitude of the internal voltage
					// E_max and E_min are the maximum and minimum of the output of voltage controller
					// Function end
				} else {
					E_mag = std::min(std::max(E_min,V_ref),E_max);
				}
			}
			else
			{
					if(VFlag) {
					E_mag = V_ctrl_blk.getoutput(V_ref-v_measured, deltat,CORRECTOR);

					// V_ref is the voltage reference obtained from Q-V droop
					// Vset is the voltage set point, usually 1 pu
					// mq is the Q-V droop gain, usually 0.05 pu
					// V_ini is the output from the integrator in the voltage controller
					// E_mag is the output of the votlage controller, it is the voltage magnitude of the internal voltage
					// E_max and E_min are the maximum and minimum of the output of voltage controller
					// Function end
				} else {
					E_mag = std::min(std::max(E_min,V_ref),E_max);
				}
			}

			// Function: P-f droop, Pmax and Pmin controller
			delta_w_droop = (Pset - p_measured) * mp; // P-f droop

			// Pmax controller
			delta_w_Pmax = Pmax_ctrl_blk.getoutput(Pmax - p_measured,deltat,CORRECTOR);

			// Pmin controller
			delta_w_Pmin = Pmin_ctrl_blk.getoutput(Pmin - p_measured,deltat,CORRECTOR);

			delta_w = delta_w_droop + delta_w_Pmax + delta_w_Pmin + 2.0 * PI * fset - w_ref; //the summation of the outputs from P-f droop, Pmax control and Pmin control

			// delta_w_droop is the output of P-f droop
			// Pset is the power set point
			// delta_w_Pmax and delta_w_Pmin are the outputs of Pmax controller and Pmin controller
			// Pmax and Pmin are the maximum limit and minimum limit of Pmax controller and Pmin controller
			// w_lim is the saturation limit
			// w_ref is the rated frequency, usually 376.99 rad/s
			// Function end

			if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
			{
				// Vdc_min controller to protect the dc bus voltage from collapsing
				next_state.ddelta_w_Vdc_min_ini = next_state.Vdc_pu - Vdc_min_pu ;
				next_state.delta_w_Vdc_min_ini = curr_state.delta_w_Vdc_min_ini + (pred_state.ddelta_w_Vdc_min_ini + next_state.ddelta_w_Vdc_min_ini) * kiVdc * deltat / 2.0;

				if (next_state.delta_w_Vdc_min_ini > 0) //
				{
					next_state.delta_w_Vdc_min_ini = 0;
				}

				delta_w_Vdc_min = next_state.delta_w_Vdc_min_ini + next_state.ddelta_w_Vdc_min_ini * kpVdc + ((next_state.Vdc_pu + pred_state.Vdc_pu) / 2 - curr_state.Vdc_pu) * kdVdc / deltat; // output from Vdc_min controller

				if (delta_w_Vdc_min > 0) //
				{
					delta_w_Vdc_min = 0;
				}

				delta_w = delta_w + delta_w_Vdc_min; //the summation of the outputs from P-f droop, Pmax control and Pmin control, and Vdc_min control
			}

			freq = (delta_w + w_ref) / 2.0 / PI; // The frequency from the CERTS droop controller, Hz

			// Function: Obtaining the Phase Angle, and obtaining the compelx value of internal voltages and their Norton Equivalence for power flow analysis
			for (i = 0; i < 1; i++)
			{
				Angle[i] = Angle_blk[i].getoutput(delta_w,deltat,CORRECTOR);

				e_droop_pu[i] = gld::complex(E_mag * cos(Angle[i]), E_mag * sin(Angle[i])); // transfers back to non-per-unit values
				e_droop[i] = e_droop_pu[i] * V_base; // transfers back to non-per-unit values

				value_IGenerated[i] = e_droop[i] / (gld::complex(Rfilter, Xfilter) * Z_base);							  // Thevenin voltage source to Norton current source convertion

				//Convergence check - do on internal voltage, because "reasons"
				mag_diff_val = e_droop[i].Mag() - e_droop_prev[i].Mag();

				//Update tracker
				e_droop_prev[i] = e_droop[i];

				//Check the difference, while we're in here
				if (mag_diff_val > GridForming_volt_convergence_criterion)
				{
					proceed_to_qsts = false;
				}
			}
			// Angle[i] refers to the phase angle of internal voltage for each phase
			// e_source[i] is the complex value of internal voltage
			// value_IGenerated[i] is the Norton equivalent current source of e_source[i]
			// Rfilter and Xfilter are the per-unit values of inverter filter
			// Function end

			double diff_w = delta_w - delta_w_prev_step;

			delta_w_prev_step = delta_w;

			memcpy(&curr_state, &next_state, sizeof(INV_DYN_STATE));

			if ((fabs(diff_w) <= GridForming_freq_convergence_criterion) && proceed_to_qsts)
			{
				simmode_return_value = SM_EVENT; // we have reached steady state
			}
			else
			{
				simmode_return_value = SM_DELTA;
			}
		}
		else //Three-phase
		{
			//Update output power
			//Get current injected
			terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
			terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
			terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

			terminal_current_val_pu[0] = terminal_current_val[0]/I_base;
			terminal_current_val_pu[1] = terminal_current_val[1]/I_base;
			terminal_current_val_pu[2] = terminal_current_val[2]/I_base;

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
			power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
			power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

			VA_Out = power_val[0] + power_val[1] + power_val[2];

			// The following code is only for three phase system

			// Function: Phase-Lock_Loop, PLL, only consider positive sequence voltage

			// Obtain the positive sequence voltage
			value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
			value_Circuit_V_NS = (value_Circuit_V[0] + value_Circuit_V[1] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI)) + value_Circuit_V[2] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI))) / 3.0;
			terminal_current_val_PS = (terminal_current_val[0] + terminal_current_val[1] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + terminal_current_val[2] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
			terminal_current_val_NS = (terminal_current_val[0] + terminal_current_val[1] * gld::complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI)) + terminal_current_val[2] * gld::complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI))) / 3.0;
			VA_Out_PS = gld::complex(3.0, 0) * (value_Circuit_V_PS * ~terminal_current_val_PS);
			VA_Out_NS = gld::complex(3.0, 0) * (value_Circuit_V_NS * ~terminal_current_val_NS);

			for (i = 0; i < 3; i++)
			{
				ugq_pu[i] = (-value_Circuit_V_PS.Re() * sin(Angle_PLL[i]) + value_Circuit_V_PS.Im() * cos(Angle_PLL[i])) / V_base;
			}
			for (i = 0; i < 3; i++)
			{
				delta_w_PLL[i] = (ugq_pu[i] * kpPLL + delta_w_PLL_blk_gfm[i].getoutput(ugq_pu[i],deltat,CORRECTOR)) * w_base;
				w_PLL[i] = delta_w_PLL[i] + w_ref;
				fPLL[i] = w_PLL[i] / 2.0 / PI;														// frequency measured by PLL
				Angle_PLL[i] = Angle_PLL_blk[i].getoutput(delta_w_PLL[i],deltat,CORRECTOR); 	// phase angle from PLL
			}

			//fPLL[2] = fPLL[1] = fPLL[0];
			// delta_w_PLL[i] is the output from the PI controller
			// w_ref is the rated angular frequency, the value is 376.99 rad/s
			// fPLL is the frequency measured by PLL
			// Function end

			// Positive sequence value of current in dq frame
			igd_pu_PS = (terminal_current_val_PS.Re() * cos(Angle_PLL[0]) + terminal_current_val_PS.Im() * sin(Angle_PLL[0])) / I_base;
			igq_pu_PS = (-terminal_current_val_PS.Re() * sin(Angle_PLL[0]) + terminal_current_val_PS.Im() * cos(Angle_PLL[0])) / I_base;

			// Negative sequence value of current in dq frame
			igd_pu_NS = (terminal_current_val_NS.Re() * cos(-Angle_PLL[0]) + terminal_current_val_NS.Im() * sin(-Angle_PLL[0])) / I_base;
			igq_pu_NS = (-terminal_current_val_NS.Re() * sin(-Angle_PLL[0]) + terminal_current_val_NS.Im() * cos(-Angle_PLL[0])) / I_base;

			Id_measured = Idmeas_blk.getoutput(igd_pu_PS,deltat,CORRECTOR);
			Iq_measured = Iqmeas_blk.getoutput(igq_pu_PS,deltat,CORRECTOR);

			// The following code is only for three phase system
			// Function: Low pass filter of P
			P_out_pu = VA_Out.Re() / S_base;

			// Output of P-measurement block
			p_measured = Pmeas_blk.getoutput(P_out_pu,deltat,CORRECTOR);

			// VA_OUT.Re() refers to the output active power from the inverter, this should be normalized.
			// S_base is the rated capacity
			// P_out_pu is the per unit value of VA_OUT.Re()
			// p_measure is the filtered active power, it is per-unit value
			// Tp is the time constant of low pass filter, it is per-unit value
			// Function end

			// Function: Low pass filter of Q
			Q_out_pu = VA_Out.Im() / S_base;
			q_measured = Qmeas_blk.getoutput(Q_out_pu,deltat,CORRECTOR);

			// VA_OUT.Im() refers to the output reactive power from the inverter
			// Q_out_pu is the per-unit value of VA_Out.Im()
			// q_measure is the filtered reactive power, it is per-unit value
			// Tq is the time constant of low pass filter, it is per-unit value
			// Function end

			if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus
			{
				I_dc_pu = P_out_pu / pred_state.Vdc_pu; // Calculate the equivalent dc current, including the dc capacitor

				if (!dc_interface_objects.empty())
				{
					int temp_idx;
					//V_DC was set here, somehow
					STATUS fxn_return_status;

					V_DC = pred_state.Vdc_pu * Vdc_base;

					//Loop through and call the DC objects
					for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
					{
						//DC object, calling object (us), init mode (true/false)
						//False at end now, because not initialization
						fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, false);

						//Make sure it worked
						if (fxn_return_status == FAILED)
						{
							//Pull the object from the array - this is just for readability (otherwise the
							OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

							//Error it up
							GL_THROW("ibr_gfm_vsm:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
							//Defined above
						}
					}

					//I_DC is done now.  If P_DC isn't, it could be calculated
				} //End DC object update

				I_PV_pu = I_DC / Idc_base; // Calculate the current from PV panel

				next_state.dVdc_pu = (I_PV_pu - I_dc_pu) / C_pu;

				next_state.Vdc_pu = curr_state.Vdc_pu + (pred_state.dVdc_pu + next_state.dVdc_pu) * deltat / 2.0;
			}

			// Function: Low pass filter of V
			pCircuit_V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;

			// Output of V-measurement block 
			v_measured = Vmeas_blk.getoutput(pCircuit_V_Avg_pu,deltat,CORRECTOR);

			Iqmax = sqrt(Imax*Imax - Id_measured*Id_measured);
			Emin = sqrt( (v_measured - Iqmax * XL) * (v_measured - Iqmax * XL) + (Id_measured * XL) * (Id_measured * XL));
			//XL	Inverter coupling reactance
			Emax = sqrt( (v_measured + Iqmax * XL) * (v_measured + Iqmax * XL) + (Id_measured * XL) * (Id_measured * XL));

			// Active current limiting

			Angle_max = asin(XL * Imax);
			AngleIT_max = Anglemax_blk.getoutput(Idmax - Id_measured,deltat,CORRECTOR);//10000;//Anglemax_blk.getoutput(Idmax - Id_measured,deltat,CORRECTOR);
			AngleIT_min = -1.0 * AngleIT_max;//-10000;//-1.0 * AngleIT_max;

			V_ctrl_blk.setparams(Tiv,E_min,E_max,Emin,Emax);
			// Value_Circuit_V[i] refers to te voltage of each phase at the inverter terminal
			// Vbase is the rated Line to ground voltage
			// pCircuit_V_Avg_pu refers to the average value of three phase voltages, it is per-unit value
			// v_measure refers to filtered voltage, it is per-unit value
			// Function end

			// Leave the space for Vdrp Flag
			// Two options: Option 1: Q-V droop; Option 0: I-V droop
			// Function: Q-V droop control and voltage control loop

			if (VdrpFlag == 1)
			{
				V_ref = Vset + (Qset - q_measured) * mq;// + delta_V_Qmax + delta_V_Qmin; //
			}
			else
			{
				V_ref = Vset - Iq_measured * mq; // include PLL
			}

			if(grid_forming_mode == DYNAMIC_DC_BUS)
			{
				if(VFlag) {
					E_mag = V_ctrl_blk.getoutput(V_ref - v_measured, deltat,E_min, pred_state.Vdc_pu*mdc,E_min,pred_state.Vdc_pu*mdc,CORRECTOR);
					// V_ref is the voltage reference obtained from Q-V droop
					// Vset is the voltage set point, usually 1 pu
					// mq is the Q-V droop gain, usually 0.05 pu
					// V_ini is the output from the integrator in the voltage controller
					// E_mag is the output of the votlage controller, it is the voltage magnitude of the internal voltage
					// E_max and E_min are the maximum and minimum of the output of voltage controller
					// Function end
				} else {
					E_mag = std::min(std::max(E_min,V_ref),E_max);
				}
			}
			else
			{
				E_mag = (V_ref-v_measured)*kpv + V_ctrl_blk.getoutput(V_ref-v_measured, deltat,CORRECTOR);
				if (E_mag > Emax)
				{
					E_mag = Emax;
				}
				else if (E_mag < Emin)
				{
					E_mag = Emin;
				}

				// V_ref is the voltage reference obtained from Q-V droop
				// Vset is the voltage set point, usually 1 pu
				// mq is the Q-V droop gain, usually 0.05 pu
				// V_ini is the output from the integrator in the voltage controller
				// E_mag is the output of the votlage controller, it is the voltage magnitude of the internal voltage
				// E_max and E_min are the maximum and minimum of the output of voltage controller
				// Function end
			}

			//Add another saturation limits as the final ones
			if (E_mag > Emax_f)
			{
				E_mag = Emax_f;
			}
			else if (E_mag < Emin_f)
			{
				E_mag = Emin_f;
			}

			// wFlaga Flag, the flag to select whether to include the PLL as the input for the P-f droop or not
			// Two options: Option 1: wm; Option 0: wPLL
			// wm is the command output from VSM
			// wPLL is the frequency from PLL loop
			// mp is P-f droop gain
			// Kb is A binary value to determine if the P-f droop is enabled or not
			if (wFlaga == 1)
			{
				delta_P_droop = (wset - wm) * kb / mp; // not include PLL
			}
			else
			{
				delta_P_droop = (wset - w_PLL[0] / w_base) * kb / mp; // include PLL
			}

			if (Tp == 0)
			{
				Pref = Pset + delta_P_droop;//
			}
			else
			{
				deltaP_droop_blk.getoutput(delta_P_droop,deltat,CORRECTOR);
			}
			// Transient damping loop
			delta_P_transdamping = D2*delta_wm - D2 * delta_P_transdamping_blk.getoutput(delta_wm,deltat,CORRECTOR);
			// use Filter funciton    1/(1+ s/wD)

			// Damping loop
			delta_P_damping = D1*delta_wm;

			//delta_wm is the output from VSM, use integral function, 1/(sT), T = 2H
			delta_wm_blk.setparams(2*H,-100,100,delta_wmin, delta_wmax);
			delta_wm = delta_wm_blk.getoutput(Pref - p_measured - delta_P_transdamping - delta_P_damping,deltat,CORRECTOR);

			wm = 1 + delta_wm;

			//Ï‰Flagb	The flag to select whether the PLL should be included for limiting the angle or not
			// Two options: Option 1: 0; Option 0: delta_wPLL

			if (wFlagb == 1)
			{
				delta_wIT = delta_wm; // not include PLL
			}
			else
			{
				delta_wIT = delta_wm - delta_w_PLL[0] / w_base; // include PLL
			}

			//delta_wm is the output from VSM, use integral function, 1/(sT), T = 2H
			// delta_w_droop is the output of P-f droop
			// Pset is the power set point
			// delta_w_Pmax and delta_w_Pmin are the outputs of Pmax controller and Pmin controller
			// Pmax and Pmin are the maximum limit and minimum limit of Pmax controller and Pmin controller
			// w_lim is the saturation limit
			// w_ref is the rated frequency, usually 376.99 rad/s
			// Function end

			// Function: Obtaining the Phase Angle, and obtaining the compelx value of internal voltages and their Norton Equivalence for power flow analysis
			for (i = 0; i < 3; i++)
			{
				//Angle_blk[i].setparams(1,-1000.0,1000.0,AngleIT_min - 2.0 * PI /3 * i,AngleIT_max + 2.0 * PI /3 * i);
				if (i == 0)
				{
					Angle_blk[i].setparams(1,-1000.0,1000.0,AngleIT_min,AngleIT_max);
				}
				else if (i == 1)
				{
					Angle_blk[i].setparams(1,-1000.0,1000.0,AngleIT_min - 4.0 * PI / 3.0,AngleIT_max + 4.0 * PI / 3.0);
				}
				else
				{
					Angle_blk[i].setparams(1,-1000.0,1000.0,AngleIT_min - 2.0 * PI / 3.0,AngleIT_max + 2.0 * PI / 3.0);
				}

				Angle_e[i] = Angle_blk[i].getoutput(delta_wIT * w_base,deltat,CORRECTOR);

				if (wFlagb == 1)
				{
					Angle[i] = Angle_e[i]; // not include PLL
					//// w_ref is the rated frequency, usually 376.99 rad/s
				}
				else
				{
					Angle[i] = Angle_e[i]  +  Angle_PLL[i]; // include PLL
				}

				//$o_A = $i_D * COS($i_Theta) - $i_Q * SIN($i_Theta)
				//$o_B = $i_D * COS($i_Theta - PI2_BY3) - $i_Q * SIN($i_Theta - PI2_BY3)
				//$o_C = $i_D * COS($i_Theta + PI2_BY3) - $i_Q * SIN($i_Theta + PI2_BY3)

				e_droop_pu[i] = gld::complex(E_mag * cos(Angle[i]), E_mag * sin(Angle[i])); // transfers back to non-per-unit values
				e_droop[i] = e_droop_pu[i] * V_base; // transfers back to non-per-unit values

				value_IGenerated[i] = e_droop[i] / ((gld::complex(R1, L1) * Z_base) + (gld::complex(0, L2) * Z_base)*(gld::complex(0, -1.0/(w_base*CFilt)))/(gld::complex(0, L2) * Z_base + gld::complex(0, -1.0/(w_base*CFilt))) )   *  gld::complex(0, -1.0/(w_base*CFilt))/ (gld::complex(0, L2) * Z_base + gld::complex(0, -1.0/(w_base*CFilt)));//+ e_droop[i] * gld::complex(0.0, ((2.0 * PI * f_nominal)*CFilt));							// Thevenin voltage source to Norton current source convertion

				//Convergence check - do on internal voltage, because "reasons"
				mag_diff_val = e_droop[i].Mag() - e_droop_prev[i].Mag();

				//Update tracker
				e_droop_prev[i] = e_droop[i];

				//Check the difference, while we're in here
				if (mag_diff_val > GridForming_volt_convergence_criterion)
				{
					proceed_to_qsts = false;
				}
			}
			// Angle[i] refers to the phase angle of internal voltage for each phase
			// e_source[i] is the complex value of internal voltage
			// value_IGenerated[i] is the Norton equivalent current source of e_source[i]
			// Rfilter and Xfilter are the per-unit values of inverter filter
			// Function end

			double diff_w = delta_w - delta_w_prev_step;

			delta_w_prev_step = delta_w;

			memcpy(&curr_state, &next_state, sizeof(INV_DYN_STATE));

			if ((fabs(diff_w) <= GridForming_freq_convergence_criterion) && proceed_to_qsts)
			{
				simmode_return_value = SM_EVENT; // we have reached steady state
			}
			else
			{
				simmode_return_value = SM_DELTA;
			}
		}
	}
	else //Additional iterations
	{
		//Just return whatever our "last desired" was
		simmode_return_value = desired_simulation_mode;
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
STATUS ibr_gfm_vsm::init_dynamics(INV_DYN_STATE *curr_time)
{
	OBJECT *obj = OBJECTHDR(this);

	//Pull the powerflow values
	if (parent_is_a_meter)
	{
		pull_complex_powerflow_values();
	}

	//Set the mode tracking variable to a default - not really needed, but be paranoid
	desired_simulation_mode = SM_EVENT;

	if (parent_is_single_phase) // single phase/split-phase implementation
	{
		//Update output power
		//Get current injected to the grid, value_IGenerated is obtained from power flow calculation
		terminal_current_val[0] = value_IGenerated[0] - (filter_admittance * value_Circuit_V[0]);

		//Update per-unit value
		terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
		terminal_current_val_pu_prefault[0] = terminal_current_val_pu[0];

		//Update power output variables, just so we can see what is going on
		power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
		VA_Out = power_val[0];

		//
		pCircuit_V_Avg_pu = value_Circuit_V[0].Mag() / V_base;

		for (int i = 0; i < 1; i++)
		{
			// Initialize the state variables of the internal voltages
			e_droop[i] = (value_IGenerated[i] * gld::complex(Rfilter, Xfilter) * Z_base);
			e_droop_prev[i] = e_droop[i];
			Angle_blk[i].setparams(1.0,-1000.0,1000.0,-1000.0,1000.0);
			Angle_blk[i].init_given_y(e_droop[i].Arg());
		}

		if(VFlag) {
			// Initialize the voltage control block
			V_ctrl_blk.setparams(Tiv,E_min,E_max,E_min,E_max);
			V_ctrl_blk.init_given_y(e_droop[0].Mag() / V_base);
		}
		
		//See if it is the first deltamode entry - theory is all future changes will trigger deltamode, so these should be set
		if (first_deltamode_init)
		{
			//Make sure it wasn't "pre-set"
			if(VFlag) {
				Vset = pCircuit_V_Avg_pu + VA_Out.Im() / S_base * mq;
			} else {
				Vset = e_droop[0].Mag() / V_base + VA_Out.Im() / S_base *mq;
			}

			if (P_f_droop_setting_mode == PSET_MODE)
			{
				Pset = VA_Out.Re() / S_base;
				fset = f_nominal;
			}
			else if (P_f_droop_setting_mode == FSET_MODE)
			{
				fset = (VA_Out.Re()/S_base)*(mp/(2.0*PI)) + f_nominal;
				Pset = 0;
			}

			//Set it false in here, for giggles
			first_deltamode_init = false;
		}
		//Default else - all changes should be in deltamode

		// Initialize P measurement filter block
		Pmeas_blk.setparams(Tp);
		Pmeas_blk.init_given_y(VA_Out.Re()/S_base);

		// Initialize Q measurement filter block
		Qmeas_blk.setparams(Tq);
		Qmeas_blk.init_given_y(VA_Out.Im()/S_base);

		// Initialize V measurement filter
		Vmeas_blk.setparams(Tv);
		Vmeas_blk.init_given_y(pCircuit_V_Avg_pu);

		// Initialize Pmax and Pmin controller
		Pmax_ctrl_blk.setparams(kppmax,kipmax,-w_lim,0.0,-w_lim,0.0);
		Pmin_ctrl_blk.setparams(kppmax,kipmax,0.0,w_lim,0.0,w_lim);

		Pmax_ctrl_blk.init_given_y(0.0);
		Pmin_ctrl_blk.init_given_y(0.0);
		
		// Initialize Qmax and Qmin controller
		Qmax_ctrl_blk.setparams(kpqmax,kiqmax,-V_lim,0.0,-V_lim,0.0);
		Qmin_ctrl_blk.setparams(kpqmax,kiqmax,0.0,V_lim,0.0,V_lim);

		Qmax_ctrl_blk.init_given_y(0.0);
		Qmin_ctrl_blk.init_given_y(0.0);

		// Initialize Vdc_min controller and DC bus voltage
		if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
		{
			//See if there are any DC objects to handle
			if (!dc_interface_objects.empty())
			{
				//Figure out what our DC power is
				P_DC = VA_Out.Re();

				int temp_idx;
				STATUS fxn_return_status;

				//Loop through and call the DC objects
				for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
				{
					//May eventually have a "DC ratio" or similar, if multiple objects on bus or "you are DC voltage master"
					//DC object, calling object (us), init mode (true/false)
					fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, true);

					//Make sure it worked
					if (fxn_return_status == FAILED)
					{
						//Pull the object from the array - this is just for readability (otherwise the
						OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

						//Error it up
						GL_THROW("ibr_gfm_vsm:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
						/*  TROUBLESHOOT
						While performing the update to a DC-bus object on this inverter, an error occurred.  Please try again.
						If the error persists, please check your model.  If the model appears correct, please submit a bug report via the issues tracker.
						*/
					}
				}

				//Theoretically, the DC objects have no set V_DC and I_DC appropriately - updated equations would go here
			} //End DC object update

			curr_time->Vdc_pu = V_DC / Vdc_base; // This should be done through PV curve
			curr_time->delta_w_Vdc_min_ini = 0;
		}

	}
	
	//  three-phase system
	if ((phases & 0x07) == 0x07)
	{
		//Update output power
		//Get current injected to the grid, value_IGenerated is obtained from power flow calculation
		terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
		terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
		terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

		//Update per-unit value
		terminal_current_val_pu[0] = terminal_current_val[0] / I_base;
		terminal_current_val_pu[1] = terminal_current_val[1] / I_base;
		terminal_current_val_pu[2] = terminal_current_val[2] / I_base;

		terminal_current_val_pu_prefault[0] = terminal_current_val_pu[0];
		terminal_current_val_pu_prefault[1] = terminal_current_val_pu[1];
		terminal_current_val_pu_prefault[2] = terminal_current_val_pu[2];

		//Update power output variables, just so we can see what is going on
		power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];
		power_val[1] = value_Circuit_V[1] * ~terminal_current_val[1];
		power_val[2] = value_Circuit_V[2] * ~terminal_current_val[2];

		VA_Out = power_val[0] + power_val[1] + power_val[2];

		//Compute the average value of three-phase terminal voltages
		pCircuit_V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;

		for (int i = 0; i < 3; i++)
		{
			delta_w_PLL_blk_gfm[i].setparams(TiPLL,-1000.0,1000.0,-1000.0,1000.0);
			delta_w_PLL_blk_gfm[i].init(0.0,0.0);

			Angle_PLL[i] = Angle_PLL_blk[i].x[0];

			// Initialize the state variables of the internal voltages
			e_droop[i] = value_IGenerated[i] *((gld::complex(R1, L1) * Z_base) + (gld::complex(0, L2) * Z_base)*(gld::complex(0, -1/(w_base*CFilt)))/(gld::complex(0, L2) * Z_base + gld::complex(0, -1/(w_base*CFilt))) )   /  gld::complex(0, -1/(w_base*CFilt))* (gld::complex(0, L2) * Z_base + gld::complex(0, -1/(w_base*CFilt)));//+ e_droop[i] * gld::complex(0.0, ((2.0 * PI * f_nominal)*CFilt));							// Thevenin voltage source to Norton current source convertion

			e_droop_prev[i] = e_droop[i];

			Angle_blk[i].setparams(1.0,-1000.0,1000.0,-1000.0,1000.0);
			Angle_blk[i].init_given_y(e_droop[i].Arg());
		}

		// Initializa the voltage control block
		V_ctrl_blk.setparams(Tiv,E_min,E_max,E_min,E_max);
		V_ctrl_blk.init_given_y((e_droop[0].Mag() + e_droop[1].Mag() + e_droop[2].Mag()) / 3 / V_base);
		
		Iqmax = sqrt((Imax)*(Imax)-(VA_Out.Re()/S_base)*(VA_Out.Re()/S_base));
		Emin =  sqrt( (pCircuit_V_Avg_pu - Iqmax * XL) * (pCircuit_V_Avg_pu - Iqmax * XL) + (VA_Out.Re()/S_base * XL) * (VA_Out.Re()/S_base * XL));
		//XL	Inverter coupling reactance
		Emax = sqrt( (pCircuit_V_Avg_pu + Iqmax * XL) * (pCircuit_V_Avg_pu + Iqmax * XL) + (VA_Out.Re()/S_base * XL) * (VA_Out.Re()/S_base * XL));

		Angle_max = asin(XL * Imax);
		AngleIT_max = Angle_max;
		AngleIT_min = -1.0 * Angle_max;

		V_ref = Vset - VA_Out.Im() / S_base * mq;// +pCircuit_V_Avg_pu;//

		wm = 1;

		//See if it is the first deltamode entry - theory is all future changes will trigger deltamode, so these should be set
		if (first_deltamode_init)
		{
			if (P_f_droop_setting_mode == PSET_MODE)
			{
				// Pset = VA_Out.Re() / S_base;
				// fset = f_nominal;
			}
			else if (P_f_droop_setting_mode == FSET_MODE)
			{
				fset = (VA_Out.Re()/S_base)*(mp/(2.0*PI)) + f_nominal;
				Pset = 0;
			}

			//Set it false in here, for giggles
			first_deltamode_init = false;
		}
		//Default else - all changes should be in deltamode

		// Initialize measured P,Q,and V
		Pmeas_blk.setparams(Tpf);
		Pmeas_blk.init_given_y(VA_Out.Re()/S_base);
		
		// Initialize Q measurement filter block
		Qmeas_blk.setparams(Tqf);
		Qmeas_blk.init_given_y(VA_Out.Im()/S_base);

		// Initialize V measurement filter
		Vmeas_blk.setparams(Tvf);
		Vmeas_blk.init_given_y(pCircuit_V_Avg_pu);

		// Initialize Pmax and Pmin controller
		Pmax_ctrl_blk.setparams(kppmax,kipmax,-w_lim,0.0,-w_lim,0.0);
		Pmin_ctrl_blk.setparams(kppmax,kipmax,0.0,w_lim,0.0,w_lim);

		Pmax_ctrl_blk.init_given_y(0.0);
		Pmin_ctrl_blk.init_given_y(0.0);
		
		// Initialize Qmax and Qmin controller
		Qmax_ctrl_blk.setparams(kpqmax,kiqmax,-V_lim,0.0,-V_lim,0.0);
		Qmin_ctrl_blk.setparams(kpqmax,kiqmax,0.0,V_lim,0.0,V_lim);

		Qmax_ctrl_blk.init_given_y(0.0);
		Qmin_ctrl_blk.init_given_y(0.0);

		V_unbl_d_blk.setparams(kpunbl,kiunbl,-1,1,-1,1);
		V_unbl_q_blk.setparams(kpunbl,kiunbl,-1,1,-1,1);
		V_unbl_d_blk.init(0.0,0.0);
		V_unbl_q_blk.init(0.0,0.0);

		//
		Idmeas_blk.setparams(TIf);
		Idmeas_blk.init(0,VA_Out.Re()/S_base);

		Iqmeas_blk.setparams(TIf);
		Iqmeas_blk.init(0,VA_Out.Im()/S_base);

		deltaP_droop_blk.setparams(Tp);
		deltaP_droop_blk.init(0,VA_Out.Re()/S_base);

		delta_P_transdamping_blk.setparams(1/wD);
		delta_P_transdamping_blk.init(0,0);

		delta_wm_blk.setparams(2*H,-100,100,delta_wmin, delta_wmax);
		delta_wm_blk.init(0,0.0);

		Anglemax_blk.setparams(Tidmax,-1000.0,1000.0,0.0,Angle_max);
		Anglemax_blk.init(0.0,0.3);

		// Initialize Vdc_min controller and DC bus voltage
		if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
		{
			//See if there are any DC objects to handle
			if (!dc_interface_objects.empty())
			{
				//Figure out what our DC power is
				P_DC = VA_Out.Re();

				int temp_idx;
				STATUS fxn_return_status;

				//Loop through and call the DC objects
				for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
				{
					//May eventually have a "DC ratio" or similar, if multiple objects on bus or "you are DC voltage master"
					//DC object, calling object (us), init mode (true/false)
					fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, true);

					//Make sure it worked
					if (fxn_return_status == FAILED)
					{
						//Pull the object from the array - this is just for readability (otherwise the
						OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

						//Error it up
						GL_THROW("ibr_gfm_vsm:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
						/*  TROUBLESHOOT
						While performing the update to a DC-bus object on this inverter, an error occurred.  Please try again.
						If the error persists, please check your model.  If the model appears correct, please submit a bug report via the issues tracker.
						*/
					}
				}

				//Theoretically, the DC objects have no set V_DC and I_DC appropriately - updated equations would go here
			} //End DC object update

			curr_time->Vdc_pu = V_DC / Vdc_base; // This should be done through PV curve
			curr_time->delta_w_Vdc_min_ini = 0;
		}
	}

	pdispatch_sync(); //sync up dispatch variables and controller setpoints

	return SUCCESS;
}

// //Module-level post update call
// /* Think this was just put here as an example - not sure what it would do */
// STATUS ibr_gfm_vsm::post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass)
// {
// 	//Should have a parent, but be paranoid
// 	if (parent_is_a_meter)
// 	{
// 		push_complex_powerflow_values();
// 	}

// 	return SUCCESS; //Always succeeds right now
// }

//Map Complex value
gld_property *ibr_gfm_vsm::map_complex_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_complex())
	{
		GL_THROW("ibr_gfm_vsm:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Map double value
gld_property *ibr_gfm_vsm::map_double_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_double())
	{
		GL_THROW("ibr_gfm_vsm:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Function to pull all the complex properties from powerflow into local variables
void ibr_gfm_vsm::pull_complex_powerflow_values(void)
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

//Function to push up all changes of complex properties to powerflow from local variables
void ibr_gfm_vsm::push_complex_powerflow_values(bool update_voltage)
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
			//**** IGenerated ****/
			//********* TODO - Does this need to be deltamode-flagged? *************//
			//Direct write, not an accumulator
			pIGenerated[0]->setp<gld::complex>(value_IGenerated[0], *test_rlock);
		}//End not voltage update
	}//End single-phase
}

// Function to update current injection IGenerated for VSI
STATUS ibr_gfm_vsm::updateCurrInjection(int64 iteration_count,bool *converged_failure)
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
		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	//See if we're in QSTS and a grid-forming inverter - update if we are
	if (!running_in_delta)
	{
		//Compute time difference
		tdiff = temp_time - prev_timestamp_dbl;

		//Get the frequency difference and make an angle difference
		freq_diff_angle_val = (freq - f_nominal)*2.0*M_PI*tdiff;

		//Compute the "adjustment" (basically exp(j-angle))
		//exp(jx) = cos(x)+j*sin(x)
		rotate_value = gld::complex(cos(freq_diff_angle_val),sin(freq_diff_angle_val));

		//Apply the adjustment to voltage and current injection
		value_Circuit_V[0] *= rotate_value;
		value_Circuit_V[1] *= rotate_value;
		value_Circuit_V[2] *= rotate_value;
		value_IGenerated[0] *= rotate_value;
		value_IGenerated[1] *= rotate_value;
		value_IGenerated[2] *= rotate_value;

		//Push the voltage - standard meter check (bit redundant)
		if (parent_is_a_meter)
		{
			push_complex_powerflow_values(true);
		}

		//Update trackers
		prev_timestamp_dbl = temp_time;
		last_QSTS_GF_Update = temp_time;
	}

	//External call to internal variables -- used by powerflow to iterate the VSI implementation, basically

	if (sqrt(Pref*Pref+Qref*Qref) > S_base)
	{
		gl_warning("ibr_gfm_vsm:%d %s - The output apparent power is larger than the rated apparent power, P and Q are capped", obj->id, (obj->name ? obj->name : "Unnamed"));
		//Defined above
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
					GL_THROW("ibr_gfm_vsm:%s - failed to map swing-checking for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
				GL_THROW("ibr_gfm_vsm:%s - failed to map swing-checking for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
			if (!bus_is_a_swing)
			{
				//Balance voltage and satisfy power
				gld::complex temp_total_power_val;
				gld::complex temp_total_power_internal;
				gld::complex temp_pos_current;

				//Calculate the Norton-shunted power
				temp_total_power_val = value_Circuit_V[0] * ~(filter_admittance * value_Circuit_V[0] );

				//Figure out what we should be generating internally
				temp_total_power_internal = temp_VA + temp_total_power_val;

				//Compute the positive sequence current
				temp_pos_current = ~(temp_total_power_internal / value_Circuit_V[0]);

				//Now populate this into the output
				value_IGenerated[0] = temp_pos_current;
			}
		}
		else //Three-phase
		{
			if (!bus_is_a_swing)
			{
				//Balance voltage and satisfy power
				gld::complex aval, avalsq;
				gld::complex temp_total_power_val[3];
				gld::complex temp_total_power_internal;
				gld::complex temp_pos_voltage, temp_pos_current;

				//Conversion variables - 1@120-deg
				aval = gld::complex(-0.5, (sqrt(3.0) / 2.0));
				avalsq = aval * aval; //squared value is used a couple places too

				//Calculate the Norton-shunted power
				temp_total_power_val[0] = value_Circuit_V[0] * ~(generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2]);
				temp_total_power_val[1] = value_Circuit_V[1] * ~(generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2]);
				temp_total_power_val[2] = value_Circuit_V[2] * ~(generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2]);

				//Figure out what we should be generating internally
				temp_total_power_internal = temp_VA + temp_total_power_val[0] + temp_total_power_val[1] + temp_total_power_val[2];

				//Compute the positive sequence voltage (*3)
				temp_pos_voltage = value_Circuit_V[0] + value_Circuit_V[1] * aval + value_Circuit_V[2] * avalsq;

				//Compute the positive sequence current
				temp_pos_current = ~(temp_total_power_internal / temp_pos_voltage);

				//Now populate this into the output
				value_IGenerated[0] = temp_pos_current;      // for grid-following inverters, the internal voltages need to be three phase balanced
				value_IGenerated[1] = temp_pos_current * avalsq;
				value_IGenerated[2] = temp_pos_current * aval;
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
	else
	{
		//Connected of some form - do a current limiting check (if supported)
		if (parent_is_single_phase)
		{
			//Compute "current" current value
			terminal_current_val[0] = value_IGenerated[0] - value_Circuit_V[0] / (gld::complex(Rfilter, Xfilter) * Z_base);

			//Make a per-unit value for comparison
			terminal_current_val_pu[0] = terminal_current_val[0]/I_base;
			
			//Compare it
			if ((terminal_current_val_pu[0].Mag() > Imax) && running_in_delta)	//Current limit only gets applied when controls valid (deltamode)
			{ // Current limit only gets applied when controls
							// valid (deltamode)
				if (phase_angle_correction) {
				if(!imax_phase_correction_done[0]) {
					// Calculate phase angle correction
					double theta = terminal_current_val_pu[0].Arg(); // Current angle
					double theta_prefault = terminal_current_val_pu_prefault[0]
				.Arg(); // Prefault current angle
					double theta_jump = theta_prefault - theta; // Jump in angle
					double Inolimit_mag =
				terminal_current_val_pu[0].Mag(); // Current magnitude
					double Iprefault_mag = terminal_current_val_pu_prefault[0]
				.Mag(); // Prefault current magnitude
					theta_c[0] = (Inolimit_mag - Imax) /
				(Inolimit_mag - Iprefault_mag) * theta_jump;
					imax_phase_correction_done[0] = true;
				}
				// Compute the limited value - pu
				intermed_curr_calc[0].SetPolar(
								Imax, terminal_current_val_pu[0].Arg() + theta_c[0]);
				} else if(virtual_resistance_correction) {
				double Re; // virtual resistance
				double Vt_pu = value_Circuit_V[0].Mag() / V_base;
				double Vang_pu = value_Circuit_V[0].Arg();
				double e_droop_mag_pu = e_droop_pu[0].Mag();
				double temp;
				
				temp = (e_droop_mag_pu*e_droop_mag_pu + Vt_pu*Vt_pu - 2*e_droop_mag_pu*Vt_pu*cos(Angle[0] - Vang_pu))/(Imax*Imax) - Xfilter*Xfilter;
				
				Re = sqrt(temp) - Rfilter;
				
				gld::complex Ilim = (e_droop_pu[0] - value_Circuit_V[0])/gld::complex(Re+Rfilter,Xfilter);
				double Ilim_ang = Ilim.Arg();
				
				intermed_curr_calc[0].SetPolar(Imax, Ilim_ang);
				} else {
				// default
				//Compute the limited value - pu
				intermed_curr_calc[0].SetPolar(Imax,terminal_current_val_pu[0].Arg());
				}
				
				// Copy into the per-unit representation
				terminal_current_val_pu[0] = intermed_curr_calc[0];
				
				// Adjust the terminal current from per-unit
				terminal_current_val[0] = terminal_current_val_pu[0] * I_base;
			}

			//Update the injection
			value_IGenerated[0] = terminal_current_val[0] + value_Circuit_V[0] / (gld::complex(Rfilter, Xfilter) * Z_base);

			//Update the internal voltage
			e_source[0] = value_IGenerated[0] * (gld::complex(Rfilter, Xfilter) * Z_base);
			e_source_pu[0] = e_source[0] / V_base;

			//Update the power, just to be consistent
			power_val[0] = value_Circuit_V[0] * ~terminal_current_val[0];

			//Update the overall power
			VA_Out = power_val[0];

			//Default else - no need to adjust
		}
		else	//Three phase
		{
			terminal_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
			terminal_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
			terminal_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

			//Loop the comparison - just because
			for (loop_var=0; loop_var<3; loop_var++)
			{
				//Make a per-unit value for comparison
				terminal_current_val_pu[loop_var] = terminal_current_val[loop_var]/I_base;
				
				// Compare it
				if ((terminal_current_val_pu[loop_var].Mag() > ImaxF) &&
					running_in_delta) // Current limit only gets applied when controls
					// valid (deltamode)
				{
					intermed_curr_calc[loop_var].SetPolar(ImaxF,terminal_current_val_pu[loop_var].Arg());
					
					// Copy into the per-unit representation
					terminal_current_val_pu[loop_var] = intermed_curr_calc[loop_var];
					
					// Adjust the terminal current from per-unit
					terminal_current_val[loop_var] =
					terminal_current_val_pu[loop_var] * I_base;
				}

				//Update the injection
				value_IGenerated[loop_var] = terminal_current_val[loop_var] + value_Circuit_V[loop_var] / ((gld::complex(0.0, L2) * Z_base) + (gld::complex(R1, L1) * Z_base)*(gld::complex(0.0, -1.0/(w_base*CFilt)))/(gld::complex(R1, L1) * Z_base + gld::complex(0.0, -1.0/(w_base*CFilt))) );

				//Update the internal voltage
				e_source[loop_var] = value_IGenerated[loop_var] * (gld::complex(0, L2) * Z_base + gld::complex(0, -1/(w_base*CFilt))) / gld::complex(0, -1/(w_base*CFilt)) * ((gld::complex(R1, L1) * Z_base) + (gld::complex(0, L2) * Z_base)*(gld::complex(0, -1/(w_base*CFilt)))/(gld::complex(0, L2) * Z_base + gld::complex(0, -1/(w_base*CFilt))) );// Thevenin voltage source to Norton current source convertion

				e_source_pu[loop_var] = e_source[loop_var] / V_base;

				//Update the power, just to be consistent
				power_val[loop_var] = value_Circuit_V[loop_var] * ~terminal_current_val[loop_var];

				//Default else - no need to adjust
			}//End phase for

			VA_Out = power_val[0] + power_val[1] + power_val[2];

		}//End three-phase checks

		//Update the output variable and per-unit
	}//End connected/working

	//Push the changes up
	if (parent_is_a_meter)
	{
		push_complex_powerflow_values(false);
	}

	//Always a success, but power flow solver may not like it if VA_OUT exceeded the rating and thus changed
	return SUCCESS;
}

//Internal function to the mapping of the DC object update function
STATUS ibr_gfm_vsm::DC_object_register(OBJECT *DC_object)
{
	FUNCTIONADDR temp_add = nullptr;
	DC_OBJ_FXNS temp_DC_struct;
	OBJECT *obj = OBJECTHDR(this);

	//Put the object into the structure
	temp_DC_struct.dc_object = DC_object;

	//Find the update function
	temp_DC_struct.fxn_address = (FUNCTIONADDR)(gl_get_function(DC_object, "DC_gen_object_update"));

	//Make sure it worked
	if (temp_DC_struct.fxn_address == nullptr)
	{
		gl_error("ibr_gfm_vsm:%s - failed to map DC update for object %s", (obj->name ? obj->name : "unnamed"), (DC_object->name ? DC_object->name : "unnamed"));
		/*  TROUBLESHOOT
		While attempting to map the update function for a DC-bus device, an error was encountered.
		Please try again.  If the error persists, please submit your code and a bug report via the issues tracker.
		*/

		return FAILED;
	}

	//Push us onto the memory
	dc_interface_objects.push_back(temp_DC_struct);

	//If we made it this far, all should be good!
	return SUCCESS;
}

//Function to initialize IEEE 1547 checks
STATUS ibr_gfm_vsm::initalize_IEEE_1547_checks(void)
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

		gl_warning("ibr_gfm_vsm:%d - %s does not have a valid parent - 1547 checks have been disabled",get_id(),get_name());
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
double ibr_gfm_vsm::perform_1547_checks(double timestepvalue)
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
			GL_THROW("ibr_gfm_vsm:%d - %s - IEEE-1547 Checks - invalid  state!",get_id(),get_name());
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
					GL_THROW("ibr_gfm_vsm:%d - %s - IEEE-1547 Checks - invalid  state!",get_id(),get_name());
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
			GL_THROW("ibr_gfm_vsm:%d - %s - IEEE-1547 Checks - invalid  state!",get_id(),get_name());
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
				gl_warning("ibr_gfm_vsm - Reconnections after an IEEE-1547 cessation are not fully validated.  May cause odd transients.");
				/*  TROUBLESHOOT
				The simple/base IEEE-1547 functionality in the ibr_gfm_vsm object only handles the cessation/disconnect side.  Upon reconnecting,
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

//update checker variables with controller setpoints
void ibr_gfm_vsm::update_chk_vars()
{
	setpoint_chk.fset = fset;
	setpoint_chk.Pref = Pref;
	setpoint_chk.Pset = Pset;
	setpoint_chk.inverter_1547_status = inverter_1547_status;
}

// Sync the pdispatch variable with the various possible controller set points.
//
// Controller sets points (Pref, Pset and fset) take precedence over pdispatch, that is
// an update to these properties will *overwrite* pdispatch.
// If these properties have not been changed, however, then pdispatch can be used
// to update the appropriate one via a unified interface.
void ibr_gfm_vsm::pdispatch_sync()
{
	
	// Check if Pref, Pset or fset were changed
	if ((Pref != setpoint_chk.Pref) || 
		(Pset != setpoint_chk.Pset) ||
		(fset != setpoint_chk.fset) ||
		(inverter_1547_status != setpoint_chk.inverter_1547_status))
	{
		// There has been some change to a reference value. 
		//  - Override pdispatch and set pdispatch_offset = 0.
		//  - Note: this also overwrites any changes to the exposed pdispatch!!
		pdispatch.pdispatch_offset = 0;
		
		//update pdispatch accordingly
		switch (P_f_droop_setting_mode)
		{
		case FSET_MODE:
			pdispatch.pdispatch = (fset * (2*PI) - w_ref)/mp;
			break;

		case PSET_MODE:
			pdispatch.pdispatch = Pset;
			break;
		
		default:
			// guess pdsipatch doesn't work with this mode
			break;
		}

		// update the check variables
		update_chk_vars();

		// overwrite the exposed pdispatch variables
		memcpy(&pdispatch_exp, &pdispatch, sizeof(PDISPATCH));

	}

	double pstar = pdispatch.pdispatch + pdispatch.pdispatch_offset;
	if ((pdispatch_exp.pdispatch + pdispatch_exp.pdispatch_offset) != (pstar))
	{
		// pdispatch or pdispatch_offset have been changed
		pdispatch.pdispatch = pdispatch_exp.pdispatch;
		pdispatch.pdispatch_offset = pdispatch_exp.pdispatch_offset;
		
		// update pstar
		pstar = pdispatch.pdispatch + pdispatch.pdispatch_offset;
		// Update appropriate controll variable 
		switch (P_f_droop_setting_mode)
		{
		case FSET_MODE:
			fset = (w_ref + pstar * mp)/(2*PI);
			break;

		case PSET_MODE:
			Pset = pstar;
			break;
		
		default:
			// guess pdsipatch doesn't work with this mode
			break;
		}

		// update the check variables
		update_chk_vars();
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_ibr_gfm_vsm(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(ibr_gfm_vsm::oclass);
		if (*obj != nullptr)
		{
			ibr_gfm_vsm *my = OBJECTDATA(*obj, ibr_gfm_vsm);
			gl_set_parent(*obj, parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(ibr_gfm_vsm);
}

EXPORT int init_ibr_gfm_vsm(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj != nullptr)
			return OBJECTDATA(obj, ibr_gfm_vsm)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(ibr_gfm_vsm);
}

EXPORT TIMESTAMP sync_ibr_gfm_vsm(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	ibr_gfm_vsm *my = OBJECTDATA(obj, ibr_gfm_vsm);
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
	SYNC_CATCHALL(ibr_gfm_vsm);
	return t2;
}

EXPORT int isa_ibr_gfm_vsm(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,ibr_gfm_vsm)->isa(classname);
}

EXPORT STATUS preupdate_ibr_gfm_vsm(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time)
{
	ibr_gfm_vsm *my = OBJECTDATA(obj, ibr_gfm_vsm);
	STATUS status_output = FAILED;

	try
	{
		status_output = my->pre_deltaupdate(t0, delta_time);
		return status_output;
	}
	catch (char *msg)
	{
		gl_error("preupdate_ibr_gfm_vsm(obj=%d;%s): %s", obj->id, (obj->name ? obj->name : "unnamed"), msg);
		return status_output;
	}
}

EXPORT SIMULATIONMODE interupdate_ibr_gfm_vsm(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	ibr_gfm_vsm *my = OBJECTDATA(obj, ibr_gfm_vsm);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time, dt, iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_ibr_gfm_vsm(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
		return status;
	}
}

// EXPORT STATUS postupdate_ibr_gfm_vsm(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass)
// {
// 	ibr_gfm_vsm *my = OBJECTDATA(obj, ibr_gfm_vsm);
// 	STATUS status = FAILED;
// 	try
// 	{
// 		status = my->post_deltaupdate(useful_value, mode_pass);
// 		return status;
// 	}
// 	catch (char *msg)
// 	{
// 		gl_error("postupdate_ibr_gfm_vsm(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
// 		return status;
// 	}
// }

//// Define export function that update the VSI current injection IGenerated to the grid
EXPORT STATUS ibr_gfm_vsm_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure)
{
	STATUS temp_status;

	//Map the node
	ibr_gfm_vsm *my = OBJECTDATA(obj, ibr_gfm_vsm);

	//Call the function, where we can update the IGenerated injection
	temp_status = my->updateCurrInjection(iteration_count,converged_failure);

	//Return what the sub function said we were
	return temp_status;
}

// Export function for registering a DC interaction object
EXPORT STATUS ibr_gfm_vsm_DC_object_register(OBJECT *this_obj, OBJECT *DC_obj)
{
	STATUS temp_status;

	//Map us
	ibr_gfm_vsm *this_inv = OBJECTDATA(this_obj, ibr_gfm_vsm);

	//Call the function to register us
	temp_status = this_inv->DC_object_register(DC_obj);

	//Return the status
	return temp_status;
}
