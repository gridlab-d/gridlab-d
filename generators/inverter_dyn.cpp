#include "inverter_dyn.h"

CLASS *inverter_dyn::oclass = NULL;
inverter_dyn *inverter_dyn::defaults = NULL;

static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
inverter_dyn::inverter_dyn(MODULE *module)
{
	if (oclass == NULL)
	{
		oclass = gl_register_class(module, "inverter_dyn", sizeof(inverter_dyn), PC_PRETOPDOWN | PC_BOTTOMUP | PC_POSTTOPDOWN | PC_AUTOLOCK);
		if (oclass == NULL)
			throw "unable to register class inverter_dyn";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			PT_enumeration, "control_mode", PADDR(control_mode), PT_DESCRIPTION, "Inverter control mode: grid-forming or grid-following",
				PT_KEYWORD, "GRID_FORMING", (enumeration)GRID_FORMING,
				PT_KEYWORD, "GRID_FOLLOWING", (enumeration)GRID_FOLLOWING,
				PT_KEYWORD, "GFL_CURRENT_SOURCE", (enumeration)GFL_CURRENT_SOURCE,

			PT_enumeration, "grid_following_mode", PADDR(grid_following_mode), PT_DESCRIPTION, "grid-following mode, positive sequency or balanced three phase power",
				PT_KEYWORD, "BALANCED_POWER", (enumeration)BALANCED_POWER,
				PT_KEYWORD, "POSITIVE_SEQUENCE", (enumeration)POSITIVE_SEQUENCE,

			PT_enumeration, "grid_forming_mode", PADDR(grid_forming_mode), PT_DESCRIPTION, "grid-forming mode, CONSTANT_DC_BUS or DYNAMIC_DC_BUS",
				PT_KEYWORD, "CONSTANT_DC_BUS", (enumeration)CONSTANT_DC_BUS,
				PT_KEYWORD, "DYNAMIC_DC_BUS", (enumeration)DYNAMIC_DC_BUS,

			PT_enumeration, "P_f_droop_setting_mode", PADDR(P_f_droop_setting_mode), PT_DESCRIPTION, "Definition of P-f droop curve",
				PT_KEYWORD, "FSET_MODE", (enumeration)FSET_MODE,
				PT_KEYWORD, "PSET_MODE", (enumeration)PSET_MODE,

			PT_complex, "phaseA_I_Out[A]", PADDR(temp_current_val[0]), PT_DESCRIPTION, "AC current on A phase in three-phase system",
			PT_complex, "phaseB_I_Out[A]", PADDR(temp_current_val[1]), PT_DESCRIPTION, "AC current on B phase in three-phase system",
			PT_complex, "phaseC_I_Out[A]", PADDR(temp_current_val[2]), PT_DESCRIPTION, "AC current on C phase in three-phase system",
			PT_complex, "power_A[VA]", PADDR(power_val[0]), PT_DESCRIPTION, "AC power on A phase in three-phase system",
			PT_complex, "power_B[VA]", PADDR(power_val[1]), PT_DESCRIPTION, "AC power on B phase in three-phase system",
			PT_complex, "power_C[VA]", PADDR(power_val[2]), PT_DESCRIPTION, "AC power on C phase in three-phase system",
			PT_complex, "VA_Out[VA]", PADDR(VA_Out), PT_DESCRIPTION, "AC power",

			// Internal Voltage and angle of VSI_DROOP, e_source[i],
			PT_complex, "e_source_A", PADDR(e_source[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of grid-forming source, phase A",
			PT_complex, "e_source_B", PADDR(e_source[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of grid-forming source, phase B",
			PT_complex, "e_source_C", PADDR(e_source[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal voltage of grid-forming source, phase C",
			PT_double, "V_angle_A", PADDR(curr_state.Angle[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal angle of grid-forming source, phase A",
			PT_double, "V_angle_B", PADDR(curr_state.Angle[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal angle of grid-forming source, phase B",
			PT_double, "V_angle_C", PADDR(curr_state.Angle[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: Internal angle of grid-forming source, phase C",

			// 3 phase average value of terminal voltage
			PT_double, "pCircuit_V_Avg_pu", PADDR(pCircuit_V_Avg_pu), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: three-phase average value of terminal voltage, per unit value",
			PT_double, "E_mag", PADDR(E_mag), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: magnitude of internal voltage of grid-forming inverter",

			//Input
			PT_double, "rated_power[VA]", PADDR(S_base), PT_DESCRIPTION, " The rated power of the inverter",
			PT_double, "rated_DC_Voltage[V]", PADDR(Vdc_base), PT_DESCRIPTION, " The rated dc bus of the inverter",

			// Inverter filter parameters
			PT_double, "Xfilter[pu]", PADDR(Xfilter), PT_DESCRIPTION, "DELTAMODE:  per-unit values of inverter filter.",
			PT_double, "Rfilter[pu]", PADDR(Rfilter), PT_DESCRIPTION, "DELTAMODE:  per-unit values of inverter filter.",

			// Grid-Following Controller Parameters
			PT_double, "Pref[W]", PADDR(Pref), PT_DESCRIPTION, "DELTAMODE: The real power reference.",
			PT_double, "Qref[VAr]", PADDR(Qref), PT_DESCRIPTION, "DELTAMODE: The reactive power reference.",
			PT_double, "kpc", PADDR(kpc), PT_DESCRIPTION, "DELTAMODE: Proportional gain of the current loop.",
			PT_double, "kic", PADDR(kic), PT_DESCRIPTION, "DELTAMODE: Integral gain of the current loop.",
			PT_double, "F_current", PADDR(F_current), PT_DESCRIPTION, "DELTAMODE: feed forward term gain in current loop.",
			PT_double, "Tif", PADDR(Tif), PT_DESCRIPTION, "DELTAMODE: time constant of first-order low-pass filter of current loop when using current source representation.",
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

			PT_double, "igd_pu_A", PADDR(igd_pu[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase A",
			PT_double, "igd_pu_B", PADDR(igd_pu[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase B",
			PT_double, "igd_pu_C", PADDR(igd_pu[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase C",
			PT_double, "igq_pu_A", PADDR(igq_pu[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, q-axis, phase A",
			PT_double, "igq_pu_B", PADDR(igq_pu[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, q-axis, phase B",
			PT_double, "igq_pu_C", PADDR(igq_pu[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, q-axis, phase C",

			PT_double, "igd_pu_A_filter", PADDR(curr_state.igd_filter[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase A, current source representation",
			PT_double, "igd_pu_B_filter", PADDR(curr_state.igd_filter[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase B, current source representation",
			PT_double, "igd_pu_C_filter", PADDR(curr_state.igd_filter[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, d-axis, phase C, current source representation",
			PT_double, "igq_pu_A_filter", PADDR(curr_state.igq_filter[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, q-axis, phase A, current source representation",
			PT_double, "igq_pu_B_filter", PADDR(curr_state.igq_filter[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, q-axis, phase B, current source representation",
			PT_double, "igq_pu_C_filter", PADDR(curr_state.igq_filter[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: terminal current of grid-following inverter, q-axis, phase C, current source representation",

			PT_double, "igd_ref_A", PADDR(igd_ref[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, d-axis, phase A",
			PT_double, "igd_ref_B", PADDR(igd_ref[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, d-axis, phase B",
			PT_double, "igd_ref_C", PADDR(igd_ref[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, d-axis, phase C",
			PT_double, "igq_ref_A", PADDR(igq_ref[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, q-axis, phase A",
			PT_double, "igq_ref_B", PADDR(igq_ref[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, q-axis, phase B",
			PT_double, "igq_ref_C", PADDR(igq_ref[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: reference current of grid-following inverter, q-axis, phase C",

			PT_double, "Angle_PLL_A", PADDR(curr_state.Angle_PLL[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: phase angle of terminal voltage measured by PLL, phase A",
			PT_double, "Angle_PLL_B", PADDR(curr_state.Angle_PLL[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: phase angle of terminal voltage measured by PLL, phase B",
			PT_double, "Angle_PLL_C", PADDR(curr_state.Angle_PLL[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: phase angle of terminal voltage measured by PLL, phase C",
			PT_double, "f_PLL_A", PADDR(fPLL[0]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: frequency of terminal voltage measured by PLL, phase A",
			PT_double, "f_PLL_B", PADDR(fPLL[1]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: frequency of terminal voltage measured by PLL, phase B",
			PT_double, "f_PLL_C", PADDR(fPLL[2]), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: frequency of terminal voltage measured by PLL, phase C",

			// Frequency-watt and volt-var in Grid-Following Control mode
			PT_bool, "frequency_watt", PADDR(frequency_watt), PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether inverter f/p droop is included or not",
			PT_bool, "checkRampRate_real", PADDR(checkRampRate_real), PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether check the ramp rate",
			PT_bool, "volt_var", PADDR(volt_var), PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether inverter volt-var droop is included or not",
			PT_bool, "checkRampRate_reactive", PADDR(checkRampRate_reactive), PT_DESCRIPTION, "DELTAMODE: Boolean used to indicate whether check the ramp rate",
			PT_double, "Tpf[s]", PADDR(Tpf), PT_DESCRIPTION, "DELTAMODE: the time constant of power measurement low pass filter in frequency-watt.",
			PT_double, "Tff[s]", PADDR(Tff), PT_DESCRIPTION, "DELTAMODE: the time constant of frequency measurement low pass filter in frequency-watt.",
			PT_double, "Tqf[s]", PADDR(Tqf), PT_DESCRIPTION, "DELTAMODE: the time constant of low pass filter in volt-var.",
			PT_double, "Tvf[s]", PADDR(Tvf), PT_DESCRIPTION, "DELTAMODE: the time constant of low pass filter in volt-var.",
			PT_double, "Pref_max[pu]", PADDR(Pref_max), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of power references in grid-following mode.",
			PT_double, "Pref_min[pu]", PADDR(Pref_min), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of power references in grid-following mode.",
			PT_double, "Qref_max[pu]", PADDR(Qref_max), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of reactive power references in grid-following mode.",
			PT_double, "Qref_min[pu]", PADDR(Qref_min), PT_DESCRIPTION, "DELTAMODE: the upper and lower limits of reactive power references in grid-following mode.",
			PT_double, "Rp[pu]", PADDR(Rp), PT_DESCRIPTION, "DELTAMODE: p-f droop gain in frequency-watt.",
			PT_double, "Rq[pu]", PADDR(Rq), PT_DESCRIPTION, "DELTAMODE: Q-V droop gain in volt-var.",
			PT_double, "rampUpRate_real", PADDR(rampUpRate_real), PT_DESCRIPTION, "DELTAMODE: ramp rate for grid-following frequency-watt",
			PT_double, "rampDownRate_real", PADDR(rampDownRate_real), PT_DESCRIPTION, "DELTAMODE: ramp rate for grid-following frequency-watt",
			PT_double, "rampUpRate_reactive", PADDR(rampUpRate_reactive), PT_DESCRIPTION, "DELTAMODE: ramp rate for grid-following volt-var",
			PT_double, "rampDownRate_reactive", PADDR(rampDownRate_reactive), PT_DESCRIPTION, "DELTAMODE: ramp rate for grid-following volt-var",

			PT_double, "frequency_convergence_criterion[rad/s]", PADDR(GridForming_freq_convergence_criterion), PT_DESCRIPTION, "Max frequency update for grid-forming inverters to return to QSTS",
			PT_double, "voltage_convergence_criterion[V]", PADDR(GridForming_volt_convergence_criterion), PT_DESCRIPTION, "Max voltage update for grid-forming inverters to return to QSTS",

			// PLL Parameters
			PT_double, "kpPLL", PADDR(kpPLL), PT_DESCRIPTION, "DELTAMODE: Proportional gain of the PLL.",
			PT_double, "kiPLL", PADDR(kiPLL), PT_DESCRIPTION, "DELTAMODE: Proportional gain of the PLL.",

			// Grid-Forming Controller Parameters
			PT_double, "Tp", PADDR(Tp), PT_DESCRIPTION, "DELTAMODE: time constant of low pass filter, P calculation.",
			PT_double, "Tq", PADDR(Tq), PT_DESCRIPTION, "DELTAMODE: time constant of low pass filter, Q calculation.",
			PT_double, "Tv", PADDR(Tv), PT_DESCRIPTION, "DELTAMODE: time constant of low pass filter, V calculation.",
			PT_double, "Vset[pu]", PADDR(Vset), PT_DESCRIPTION, "DELTAMODE: voltage set point in grid-forming inverter, usually 1 pu.",
			PT_double, "kpv", PADDR(kpv), PT_DESCRIPTION, "DELTAMODE: proportional gain and integral gain of voltage loop.",
			PT_double, "kiv", PADDR(kiv), PT_DESCRIPTION, "DELTAMODE: proportional gain and integral gain of voltage loop.",
			PT_double, "mq[pu]", PADDR(mq), PT_DESCRIPTION, "DELTAMODE: Q-V droop gain, usually 0.05 pu.",
			PT_double, "Q_V_droop[pu]", PADDR(mq), PT_DESCRIPTION, "DELTAMODE: Q-V droop gain, usually 0.05 pu.",
			PT_double, "E_max", PADDR(E_max), PT_DESCRIPTION, "DELTAMODE: E_max and E_min are the maximum and minimum of the output of voltage controller.",
			PT_double, "E_min", PADDR(E_min), PT_DESCRIPTION, "DELTAMODE: E_max and E_min are the maximum and minimum of the output of voltage controller.",
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

			PT_double, "Vdc_pu[pu]", PADDR(curr_state.Vdc_pu), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: dc bus voltage of PV panel when using grid-forming PV Inverter",
			PT_double, "Vdc_min_pu[pu]", PADDR(Vdc_min_pu), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: The reference voltage of the Vdc_min controller",
			PT_double, "C_pu[pu]", PADDR(C_pu), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: capacitance of dc bus",
			PT_double, "mdc", PADDR(mdc), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: saturation limit of modulation index",
			PT_double, "kpVdc", PADDR(kpVdc), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: proportional gain of Vdc_min controller",
			PT_double, "kiVdc", PADDR(kiVdc), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: integral gain of Vdc_min controller",
			PT_double, "kdVdc", PADDR(kiVdc), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: derivative gain of Vdc_min controller",

			PT_double, "p_measure", PADDR(curr_state.p_measure), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: filtered active power for grid-forming inverter",
			PT_double, "q_measure", PADDR(curr_state.q_measure), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: filtered reactive power for grid-forming inverter",
			PT_double, "v_measure", PADDR(curr_state.v_measure), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "DELTAMODE: filtered voltage for grid-forming inverter",

			//DC Bus portions
			PT_double, "V_In[V]", PADDR(V_DC), PT_DESCRIPTION, "DC input voltage",
			PT_double, "I_In[A]", PADDR(I_DC), PT_DESCRIPTION, "DC input current",
			PT_double, "P_In[W]", PADDR(P_DC), PT_DESCRIPTION, "DC input power",

			PT_double, "pvc_Pmax[W]", PADDR(pvc_Pmax), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "P max from the PV curve",

			NULL) < 1)
				GL_THROW("unable to publish properties in %s", __FILE__);

		defaults = this;

		memset(this, 0, sizeof(inverter_dyn));

		if (gl_publish_function(oclass, "preupdate_gen_object", (FUNCTIONADDR)preupdate_inverter_dyn) == NULL)
			GL_THROW("Unable to publish inverter_dyn deltamode function");
		if (gl_publish_function(oclass, "interupdate_gen_object", (FUNCTIONADDR)interupdate_inverter_dyn) == NULL)
			GL_THROW("Unable to publish inverter_dyn deltamode function");
		// if (gl_publish_function(oclass, "postupdate_gen_object", (FUNCTIONADDR)postupdate_inverter_dyn) == NULL)
		// 	GL_THROW("Unable to publish inverter_dyn deltamode function");
		if (gl_publish_function(oclass, "current_injection_update", (FUNCTIONADDR)inverter_dyn_NR_current_injection_update) == NULL)
			GL_THROW("Unable to publish inverter_dyn current injection update function");
		if (gl_publish_function(oclass, "register_gen_DC_object", (FUNCTIONADDR)inverter_dyn_DC_object_register) == NULL)
			GL_THROW("Unable to publish inverter_dyn DC registration function");
	}
}

//Isa function for identification
int inverter_dyn::isa(char *classname)
{
	return strcmp(classname,"inverter_dyn")==0;
}

/* Object creation is called once for each object that is created by the core */
int inverter_dyn::create(void)
{
	// Default values for Inverter object.
	pMeterStatus = NULL;

	////////////////////////////////////////////////////////
	// DELTA MODE
	////////////////////////////////////////////////////////
	deltamode_inclusive = false; //By default, don't be included in deltamode simulations
	first_sync_delta_enabled = false;

	inverter_start_time = TS_INVALID;
	inverter_first_step = true;
	first_iteration_current_injection = -1; //Initialize - mainly for tracking SWING_PQ status
	first_deltamode_init = true;	//First time it goes in will be the first time

	//Variable mapping items
	parent_is_a_meter = false;		//By default, no parent meter
	parent_is_single_phase = false; //By default, we're three-phase
	attached_bus_type = 0;			//By default, we're basically a PQ bus
	swing_test_fxn = NULL;			//By default, no mapping

	pCircuit_V[0] = pCircuit_V[1] = pCircuit_V[2] = NULL;
	pLine_I[0] = pLine_I[1] = pLine_I[2] = NULL;
	pLine_unrotI[0] = pLine_unrotI[1] = pLine_unrotI[2] = NULL;
	pPower[0] = pPower[1] = pPower[2] = NULL;
	pIGenerated[0] = pIGenerated[1] = pIGenerated[2] = NULL;

	pMeterStatus = NULL; // check if the meter is in service
	pbus_full_Y_mat = NULL;
	pGenerated = NULL;

	//Zero the accumulators
	value_Circuit_V[0] = value_Circuit_V[1] = value_Circuit_V[2] = complex(0.0, 0.0);
	value_Line_I[0] = value_Line_I[1] = value_Line_I[2] = complex(0.0, 0.0);
	value_Line_unrotI[0] = value_Line_unrotI[1] = value_Line_unrotI[2] = complex(0.0, 0.0);
	value_Power[0] = value_Power[1] = value_Power[2] = complex(0.0, 0.0);
	value_IGenerated[0] = value_IGenerated[1] = value_IGenerated[2] = complex(0.0, 0.0);
	value_MeterStatus = 1; //Connected, by default

	// Inverter filter
	Xfilter = 0.05; //per unit
	Rfilter = 0.01; // per unit

	// Grid-Forming controller parameters
	Tp = 0.01; // s
	Tq = 0.01; // s
	Tv = 0.01; // s
	Vset = -99.0;	//Flag value
	kpv = 0;
	kiv = 5.86;
	mq = 0.05;
	E_max = 2;
	E_min = 0;
	mp = 3.77;
	P_f_droop = -100;
	kppmax = 3;
	kipmax = 30;
	w_lim = 50; // rad/s
	Pmax = 1.5;
	Pmin = 0;
	w_ref = -99.0;	//Flag value
	f_nominal = 60;
	freq = 60;
	fset = -99.0;	//Flag value

	// PLL controller parameters
	kpPLL = 50;
	kiPLL = 900;
	fPLL[0] = fPLL[1] = fPLL[2] = 60.0;
	curr_state.Angle_PLL[0] = 0.0;
	curr_state.Angle_PLL[1] = (4.0 / 3.0) * PI;
	curr_state.Angle_PLL[2] = (2.0 / 3.0) * PI;


	// ramp rate check for grid-following inverters
	checkRampRate_real = false;
	checkRampRate_reactive = false;
	rampUpRate_real = 1.67; //unit: pu/s
	rampDownRate_real = 1.67; //unit: pu/s
	rampUpRate_reactive = 1.67; //unit: pu/s
	rampDownRate_reactive = 1.67; //unit: pu/s

	// Grid-Following controller parameters
	kpc = 0.05;
	kic = 5;
	F_current = 0;
	Tif = 0.005; // only used for current source representation

	//frequency-watt
	frequency_watt = false;
	Tpf = 0.5;		// S
	Tff = 0.1;		// s
	Pref_max = 1.5; // per unit
	Pref_min = -1.5;	// per unit
	Rp = 0.04;		//P-f droop 4%

	//volt-var
	volt_var = false;
	Tqf = 0.2;	// s
	Tvf = 0.05; // s

	//Vset = 1;  // per unit
	Qref_max = 1.5; // per unit
	Qref_min = -1.5;	// per unit
	Rq = 0.05;		// per unit

	Vdc_base = 850; // default value of dc bus voltage
	Vdc_min_pu = 1; // default reference of the Vdc_min controller

	P_f_droop_setting_mode = PSET_MODE;	//Defaults to PSET mode

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
	last_QSTS_GF_Update = TSNVRDBL;

	//Clear the DC interface list - paranoia
	dc_interface_objects.clear();

	//DC Bus items
	P_DC = 0.0;
	V_DC = 0.0; //Vdc_base;
	I_DC = 0.0;

	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int inverter_dyn::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	double temp_volt_mag;
	double temp_volt_ang[3];
	gld_property *temp_property_pointer;
	gld_property *Frequency_mapped;
	gld_wlock *test_rlock;
	bool temp_bool_value;
	int temp_idx_x, temp_idx_y;
	unsigned iindex, jindex;
	complex temp_complex_value;
	complex_array temp_complex_array;
	OBJECT *tmp_obj = NULL;

	//Deferred initialization code
	if (parent != NULL)
	{
		if ((parent->flags & OF_INIT) != OF_INIT)
		{
			char objname[256];
			gl_verbose("inverter_dyn::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}

	// Data sanity check
	if (S_base <= 0)
	{
		S_base = 1000;
		gl_warning("inverter_dyn:%d - %s - The rated power of this inverter must be positive - set to 1 kVA.",obj->id,(obj->name ? obj->name : "unnamed"));
		/*  TROUBLESHOOT
		The inverter_dyn has a rated power that is negative.  It defaulted to a 1 kVA inverter.  If this is not desired, set the property properly in the GLM.
		*/
	}

	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true; //Set the flag and off we go
	}

	// find parent meter or triplex_meter, if not defined, use default voltages, and if
	// the parent is not a meter throw an exception
	if (parent != NULL)
	{
		//See what kind of parent we are
		if (gl_object_isa(parent, "meter", "powerflow") || gl_object_isa(parent, "node", "powerflow") || gl_object_isa(parent, "load", "powerflow") ||
			gl_object_isa(parent, "triplex_meter", "powerflow") || gl_object_isa(parent, "triplex_node", "powerflow") || gl_object_isa(parent, "triplex_load", "powerflow"))
		{
			//See if we're in deltamode and VSI - if not, we don't care about the "parent-ception" mapping
			//Normal deltamode just goes through current interfaces, so don't need this craziness
			if (deltamode_inclusive == true)
			{
				//See if this attached node is a child or not
				if (parent->parent != NULL)
				{
					//Map parent
					tmp_obj = parent->parent;

					//See what it is
					if ((gl_object_isa(tmp_obj, "meter", "powerflow") == false) && (gl_object_isa(tmp_obj, "node", "powerflow") == false) && (gl_object_isa(tmp_obj, "load", "powerflow") == false) &&
						(gl_object_isa(tmp_obj, "triplex_meter", "powerflow") == false) && (gl_object_isa(tmp_obj, "triplex_node", "powerflow") == false) && (gl_object_isa(tmp_obj, "triplex_load", "powerflow") == false))
					{
						//Not a wierd map, just use normal parent
						tmp_obj = parent;
					}
					else //Implies it is a powerflow parent
					{
						//See if we are deltamode-enabled -- if so, flag our parent while we're here
						//Map our deltamode flag and set it (parent will be done below)
						temp_property_pointer = new gld_property(parent, "Norton_dynamic");

						//Make sure it worked
						if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_bool() != true))
						{
							GL_THROW("inverter_dyn:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", parent->name ? parent->name : "unnamed");
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
			if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_set() != true))
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
			if (((phases & 0x07) != 0x07) && (control_mode == GRID_FORMING))
			{
				gl_warning("inverter_dyn:%s is in grid-forming mode, but is not a three-phase connection.  This is untested and may not behave properly.", obj->name ? obj->name : "unnamed");
				/*  TROUBLESHOOT
				The inverter_dyn was set up to be grid-forming, but is either a triplex or a single-phase-connected inverter.  This implementaiton is not fully tested and may either not
				work, or produce unexpecte results.
				*/
			}

			//Pull the bus type
			temp_property_pointer = new gld_property(tmp_obj, "bustype");

			//Make sure it worked
			if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_enumeration() != true))
			{
				GL_THROW("inverter_dyn:%s failed to map bustype variable from %s", obj->name ? obj->name : "unnamed", obj->parent->name ? obj->parent->name : "unnamed");
				/*  TROUBLESHOOT
				While attempting to map the bustype variable from the parent node, an error was encountered.  Please try again.  If the error
				persists, please report it with your GLM via the issues tracking system.
				*/
			}

			//Pull the value of the bus
			attached_bus_type = temp_property_pointer->get_enumeration();

			//Remove it
			delete temp_property_pointer;

			//Determine parent type
			//Triplex first, otherwise it tries to map to three-phase (since all triplex are nodes)
			if ((gl_object_isa(tmp_obj, "triplex_meter", "powerflow") == true) || (gl_object_isa(tmp_obj, "triplex_node", "powerflow") == true) || (gl_object_isa(tmp_obj, "triplex_load", "powerflow") == true))
			{
				//Indicate this is a meter, but is also a single-phase variety
				parent_is_a_meter = true;
				parent_is_single_phase = true;

				//Map the various powerflow variables
				//Map the other two here for initialization problem
				pCircuit_V[0] = map_complex_value(tmp_obj, "voltage_12");
				pCircuit_V[1] = map_complex_value(tmp_obj, "voltage_1");
				pCircuit_V[2] = map_complex_value(tmp_obj, "voltage_2");

				//Get 12 and Null the rest
				pLine_I[0] = map_complex_value(tmp_obj, "current_12");
				pLine_I[1] = NULL;
				pLine_I[2] = NULL;

				//Pull power12, then null the rest
				pPower[0] = map_complex_value(tmp_obj, "power_12");
				pPower[1] = NULL; //Not used
				pPower[2] = NULL; //Not used

				pLine_unrotI[0] = map_complex_value(tmp_obj, "prerotated_current_12");
				pLine_unrotI[1] = NULL; //Not used
				pLine_unrotI[2] = NULL; //Not used

				//Map IGenerated, even though triplex can't really use this yet (just for the sake of doing so)
				pIGenerated[0] = map_complex_value(tmp_obj, "deltamode_generator_current_12");
				pIGenerated[1] = NULL;
				pIGenerated[2] = NULL;
			} //End triplex parent
			else if ((gl_object_isa(tmp_obj, "meter", "powerflow") == true) || (gl_object_isa(tmp_obj, "node", "powerflow") == true) || (gl_object_isa(tmp_obj, "load", "powerflow") == true))
			{
				//See if we're three-phased
				if ((phases & 0x07) == 0x07) //We are
				{
					parent_is_a_meter = true;
					parent_is_single_phase = false;

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

					//NULL all the secondary indices - we won't use any of them
					pCircuit_V[1] = NULL;
					pCircuit_V[2] = NULL;

					pLine_I[1] = NULL;
					pLine_I[2] = NULL;

					pPower[1] = NULL;
					pPower[2] = NULL;

					pLine_unrotI[1] = NULL;
					pLine_unrotI[2] = NULL;

					pIGenerated[1] = NULL;
					pIGenerated[2] = NULL;

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
						GL_THROW("inverter_dyn:%s is not connected to a single-phase or three-phase node - two-phase connections are not supported at this time.", obj->name ? obj->name : "unnamed");
						/*  TROUBLESHOOT
						The inverter_dyn only supports single phase (A, B, or C or triplex) or full three-phase connections.  If you try to connect it differntly than this, it will not work.
						*/
					}
				} //End non-three-phase
			}	  //End non-triplex parent

			//*** Common items ****//
			// Many of these go to the "true parent", not the "powerflow parent"

			//Map the nominal frequency
			temp_property_pointer = new gld_property("powerflow::nominal_frequency");

			//Make sure it worked
			if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_double() != true))
			{
				GL_THROW("inverter_dyn:%d %s failed to map the nominal_frequency property", obj->id, (obj->name ? obj->name : "Unnamed"));
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
			if (deltamode_inclusive == true)
			{
				if (control_mode != GFL_CURRENT_SOURCE)
				{
					//Map our deltamode flag and set it (parent will be done below)
					temp_property_pointer = new gld_property(tmp_obj, "Norton_dynamic");

					//Make sure it worked
					if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_bool() != true))
					{
						GL_THROW("inverter_dyn:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", tmp_obj->name ? tmp_obj->name : "unnamed");
						//Defined elsewhere
					}

					//Flag it to true
					temp_bool_value = true;
					temp_property_pointer->setp<bool>(temp_bool_value, *test_rlock);

					//Remove it
					delete temp_property_pointer;
				}
				else
				{
					//Map our deltamode flag and set it (parent will be done below)
					temp_property_pointer = new gld_property(tmp_obj, "generator_dynamic");

					//Make sure it worked
					if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_bool() != true))
					{
						GL_THROW("inverter_dyn:%s failed to map generator deltamode variable from %s", obj->name ? obj->name : "unnamed", tmp_obj->name ? tmp_obj->name : "unnamed");
						/*  TROUBLESHOOT
						While trying to map a flag indicating a dynamic generator is attached to the system, an error was encountered.  Please
						try your file again.  If the error persists, please submit an issue ticket.
						*/
					}

					//Flag it to true
					temp_bool_value = true;
					temp_property_pointer->setp<bool>(temp_bool_value, *test_rlock);

					//Remove it
					delete temp_property_pointer;
				}

				// Obtain the Z_base of the system for calculating filter impedance
				//Link to nominal voltage
				temp_property_pointer = new gld_property(parent, "nominal_voltage");

				//Make sure it worked
				if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_double() != true))
				{
					gl_error("inverter_dyn:%d %s failed to map the nominal_voltage property", obj->id, (obj->name ? obj->name : "Unnamed"));
					/*  TROUBLESHOOT
					While attempting to map the nominal_voltage property, an error occurred.  Please try again.
					If the error persists, please submit your GLM and a bug report to the ticketing system.
					*/

					return FAILED;
				}
				//Default else, it worked

				//Copy that value out
				node_nominal_voltage = temp_property_pointer->get_double();

				//Remove the property pointer
				delete temp_property_pointer;

				V_base = node_nominal_voltage;

				if ((phases & 0x07) == 0x07)
				{
					I_base = S_base / V_base / 3.0;
					Z_base = (node_nominal_voltage * node_nominal_voltage) / (S_base / 3.0); // voltage is phase to ground voltage, S_base is three phase capacity

					if (control_mode == GFL_CURRENT_SOURCE)
					{
						filter_admittance = 0.0;
					}
					else
					{
						filter_admittance = complex(1.0, 0.0) / (complex(Rfilter, Xfilter) * Z_base);
					}

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
								generator_admittance[iindex][jindex] = complex(0.0, 0.0);
							}
						}
					}
				}
				else //Must be triplex or single-phased (above check should remove others)
				{
					I_base = S_base / V_base;
					Z_base = (node_nominal_voltage * node_nominal_voltage) / S_base; // voltage is phase to ground voltage, S_base is single phase capacity

					if (control_mode == GFL_CURRENT_SOURCE)
					{
						filter_admittance = 0.0;
					}
					else
					{
						filter_admittance = complex(1.0, 0.0) / (complex(Rfilter, Xfilter) * Z_base);
					}

					//Zero the admittance directly - this was probably already done, but lets be paranoid
					//Semi-useless for one entry, since we'll just overwrite it here shortly (but easier this way)
					generator_admittance[0][0] = generator_admittance[0][1] = generator_admittance[0][2] = complex(0.0, 0.0);
					generator_admittance[1][0] = generator_admittance[1][1] = generator_admittance[1][2] = complex(0.0, 0.0);
					generator_admittance[2][0] = generator_admittance[2][1] = generator_admittance[2][2] = complex(0.0, 0.0);

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

				if (control_mode != GFL_CURRENT_SOURCE)
				{
					//Map the full_Y parameter to inject the admittance portion into it
					pbus_full_Y_mat = new gld_property(tmp_obj, "deltamode_full_Y_matrix");

					//Check it
					if ((pbus_full_Y_mat->is_valid() != true) || (pbus_full_Y_mat->is_complex_array() != true))
					{
						GL_THROW("inverter_dyn:%s failed to map Norton-equivalence deltamode variable from %s", obj->name ? obj->name : "unnamed", tmp_obj->name ? tmp_obj->name : "unnamed");
						/*  TROUBLESHOOT
						While attempting to set up the deltamode interfaces and calculations with powerflow, the required interface could not be mapped.
						Please check your GLM and try again.  If the error persists, please submit a trac ticket with your code.
						*/
					}

					//Pull down the variable
					pbus_full_Y_mat->getp<complex_array>(temp_complex_array, *test_rlock);

					//See if it is valid
					if (temp_complex_array.is_valid(0, 0) != true)
					{
						//Create it
						temp_complex_array.grow_to(3, 3);

						//Zero it, by default
						for (temp_idx_x = 0; temp_idx_x < 3; temp_idx_x++)
						{
							for (temp_idx_y = 0; temp_idx_y < 3; temp_idx_y++)
							{
								temp_complex_array.set_at(temp_idx_x, temp_idx_y, complex(0.0, 0.0));
							}
						}
					}
					else //Already populated, make sure it is the right size!
					{
						if ((temp_complex_array.get_rows() != 3) && (temp_complex_array.get_cols() != 3))
						{
							GL_THROW("inverter_dyn:%s exposed Norton-equivalent matrix is the wrong size!", obj->name ? obj->name : "unnamed");
							/*  TROUBLESHOOT
							While mapping to an admittance matrix on the parent node device, it was found it is the wrong size.
							Please try again.  If the error persists, please submit your code and model via the issue tracking system.
							*/
						}
						//Default else -- right size
					}

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
						}
					}

					//Push it back up
					pbus_full_Y_mat->setp<complex_array>(temp_complex_array, *test_rlock);
				}

				//Map the power variable
				pGenerated = map_complex_value(tmp_obj, "deltamode_PGenTotal");
			} //End VSI common items

			//Map status - true parent
			pMeterStatus = new gld_property(parent, "service_status");

			//Check it
			if ((pMeterStatus->is_valid() != true) || (pMeterStatus->is_enumeration() != true))
			{
				GL_THROW("Inverter_dyn failed to map powerflow status variable");
				/*  TROUBLESHOOT
				While attempting to map the service_status variable of the parent
				powerflow object, an error occurred.  Please try again.  If the error
				persists, please submit your code and a bug report via the trac website.
				*/
			}

			//Pull initial voltages, but see which ones we should grab
			if (parent_is_single_phase == true)
			{
				//Powerflow values -- pull the initial value (should be nominals)
				value_Circuit_V[0] = pCircuit_V[0]->get_complex(); //A, B, C, or 12
				value_Circuit_V[1] = complex(0.0, 0.0);			   //Reset, for giggles
				value_Circuit_V[2] = complex(0.0, 0.0);			   //Same
			}
			else //Must be a three-phase, pull them all
			{
				//Powerflow values -- pull the initial value (should be nominals)
				value_Circuit_V[0] = pCircuit_V[0]->get_complex(); //A
				value_Circuit_V[1] = pCircuit_V[1]->get_complex(); //B
				value_Circuit_V[2] = pCircuit_V[2]->get_complex(); //C
			}

			//See if we're grid-forming and attached to a SWING (and vset is higher)
			if ((control_mode == GRID_FORMING) && (attached_bus_type == 2) && (Vset > 0.0))
			{
				//See if the voltage is not 1.0
				if (Vset != 1.0)
				{
					//Compute the magnitude
					temp_volt_mag = Vset * node_nominal_voltage;

					//See if we're single-phase or not
					if (parent_is_single_phase == true)
					{
						//Pull the angle first, just in case it was intentionally set as off-nominal
						temp_volt_ang[0] = value_Circuit_V[0].Arg();

						//Set the values of the SWING bus to our desired set point
						value_Circuit_V[0].SetPolar(temp_volt_mag,temp_volt_ang[0]);	//A, B, C, or 12 - zero angled
						value_Circuit_V[1] = complex(0.0,0.0);
						value_Circuit_V[2] = complex(0.0,0.0);
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
			GL_THROW("Inverter_dyn must have a valid powerflow object as its parent, or no parent at all");
			/*  TROUBLESHOOT
			Check the parent object of the inverter.  The inverter_dyn is only able to be childed via to powerflow objects.
			Alternatively, you can also choose to have no parent, in which case the inverter_dyn will be a stand-alone application
			using default voltage values for solving purposes.
			*/
		}
	}	 //End parent call
	else //Must not have a parent
	{
		//Indicate we don't have a meter parent, nor is it single phase (though the latter shouldn't matter)
		parent_is_a_meter = false;
		parent_is_single_phase = false;

		gl_warning("inverter_dyn:%d has no parent meter object defined; using static voltages", obj->id);
		/*  TROUBLESHOOT
		An inverter_dyn in the system does not have a parent attached.  It is using static values for the voltage.
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
	}

	///////////////////////////////////////////////////////////////////////////
	// DELTA MODE
	///////////////////////////////////////////////////////////////////////////
	//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (enable_subsecond_models != true)
		{
			gl_warning("inverter_dyn:%s indicates it wants to run deltamode, but the module-level flag is not set!", obj->name ? obj->name : "unnamed");
			/*  TROUBLESHOOT
			The inverter_dyn object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			gen_object_count++; //Increment the counter
			first_sync_delta_enabled = true;
		}
		//Default else - don't do anything
	}	 //End deltamode inclusive
	else //This particular model isn't enabled
	{
		if (enable_subsecond_models == true)
		{
			gl_warning("inverter_dyn:%d %s - Deltamode is enabled for the module, but not this inverter!", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The inverter_dyn is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this inverter_dyn may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
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
	VA_Out = complex(Pref, Qref);
	pvc_Pmax = 0;

	//See if we had a single phase connection
	if (parent_is_single_phase == true)
	{
		power_val[0] = VA_Out; //This probably needs to be adjusted for phasing?
	}
	else //Just assume three-phased
	{
		power_val[0] = VA_Out / 3.0;
		power_val[1] = VA_Out / 3.0;
		power_val[2] = VA_Out / 3.0;
	}

	//Init tracking variable
	prev_timestamp_dbl = (double)gl_globalclock;

	// Link P_f_droop to mp
	if (P_f_droop != -100)
	{
		mp = P_f_droop * w_ref;
	}

	return 1;


}

TIMESTAMP inverter_dyn::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter == true)
	{
		//Reset
		reset_complex_powerflow_accumulators();

		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	return t2;
}

TIMESTAMP inverter_dyn::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret_value;

	FUNCTIONADDR test_fxn;
	STATUS fxn_return_status;

	complex temp_power_val[3];

	complex temp_complex_value;
	gld_wlock *test_rlock;

	//Assume always want TS_NEVER
	tret_value = TS_NEVER;

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter == true)
	{
		//Reset
		reset_complex_powerflow_accumulators();

		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	//Deltamode check/init items
	if (first_sync_delta_enabled == true) //Deltamode first pass
	{
		//TODO: LOCKING!
		if ((deltamode_inclusive == true) && (enable_subsecond_models == true)) //We want deltamode - see if it's populated yet
		{
			if (((gen_object_current == -1) || (delta_objects == NULL)) && (enable_subsecond_models == true))
			{
				//Call the allocation routine
				allocate_deltamode_arrays();
			}

			//Check limits of the array
			if (gen_object_current >= gen_object_count)
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
			delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj, "interupdate_gen_object"));

			//Make sure it worked
			if (delta_functions[gen_object_current] == NULL)
			{
				GL_THROW("Failure to map deltamode function for device:%s", obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			/* post_delta_functions removed, since it didn't seem to be doing anything - empty it out/delete it if this is the case! */
			// //Map up the function for postupdate
			// post_delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj, "postupdate_gen_object"));

			// //Make sure it worked
			// if (post_delta_functions[gen_object_current] == NULL)
			// {
			// 	GL_THROW("Failure to map post-deltamode function for device:%s", obj->name);
			// 	/*  TROUBLESHOOT
			// 	Attempts to map up the postupdate function of a specific device failed.  Please try again and ensure
			// 	the object supports deltamode.  If the error persists, please submit your code and a bug report via the
			// 	trac website.
			// 	*/
			// }

			//Map up the function for postupdate
			delta_preupdate_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj, "preupdate_gen_object"));

			//Make sure it worked
			if (delta_preupdate_functions[gen_object_current] == NULL)
			{
				GL_THROW("Failure to map pre-deltamode function for device:%s", obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the preupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Update pointer
			gen_object_current++;

			if (parent_is_a_meter == true)
			{

				//Accumulate the starting power
				temp_complex_value = complex(Pref, Qref);

				//Push it up
				pGenerated->setp<complex>(temp_complex_value, *test_rlock);

				//Map the current injection function
				test_fxn = (FUNCTIONADDR)(gl_get_function(obj->parent, "pwr_current_injection_update_map"));

				//See if it was located
				if (test_fxn == NULL)
				{
					GL_THROW("inverter_dyn:%s - failed to map additional current injection mapping for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
					GL_THROW("inverter_dyn:%s - failed to map additional current injection mapping for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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

	//Calculate power based on measured terminal voltage and currents
	if (parent_is_single_phase == true) // single phase/split-phase implementation
	{
		if (inverter_first_step == false)
		{
			//Update output power
			//Get current injected
			temp_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

			//Update power output variables, just so we can see what is going on
			VA_Out = value_Circuit_V[0] * ~temp_current_val[0];

			if ((control_mode == GRID_FOLLOWING) || (control_mode == GFL_CURRENT_SOURCE))
			{
				if (attached_bus_type != 2)
				{

					//Compute desired output - sign convention appears to be backwards
					complex temp_VA = complex(Pref, Qref);

					//Force the output power the same as glm pre-defined values
					value_IGenerated[0] = ~(temp_VA / value_Circuit_V[0]) + filter_admittance * value_Circuit_V[0];
				}
			}
		}
	}
	else //Three phase variant
	{

		if (inverter_first_step == false)
		{
			//Update output power
			//Get current injected
			temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
			temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
			temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~temp_current_val[0];
			power_val[1] = value_Circuit_V[1] * ~temp_current_val[1];
			power_val[2] = value_Circuit_V[2] * ~temp_current_val[2];

			VA_Out = power_val[0] + power_val[1] + power_val[2];

			if (control_mode == GRID_FORMING)
			{
				/***** This has been moved down to the current injection  */
				// // Adjust VSI (not on SWING bus) current injection and e_source values only at the first iteration of each time step
				// if (attached_bus_type != 2)
				// {

				// 	complex temp_VA = complex(Pref,Qref);

				// 	//Copy in value
				// 	temp_power_val[0] = power_val[0] + (temp_VA - VA_Out) / 3.0;
				// 	temp_power_val[1] = power_val[1] + (temp_VA - VA_Out) / 3.0;
				// 	temp_power_val[2] = power_val[2] + (temp_VA - VA_Out) / 3.0;

				// 	//Back out the current injection
				// 	temp_current_val[0] = ~(temp_power_val[0]/value_Circuit_V[0]) + generator_admittance[0][0]*value_Circuit_V[0] + generator_admittance[0][1]*value_Circuit_V[1] + generator_admittance[0][2]*value_Circuit_V[2];
				// 	temp_current_val[1] = ~(temp_power_val[1]/value_Circuit_V[1]) + generator_admittance[1][0]*value_Circuit_V[0] + generator_admittance[1][1]*value_Circuit_V[1] + generator_admittance[1][2]*value_Circuit_V[2];
				// 	temp_current_val[2] = ~(temp_power_val[2]/value_Circuit_V[2]) + generator_admittance[2][0]*value_Circuit_V[0] + generator_admittance[2][1]*value_Circuit_V[1] + generator_admittance[2][2]*value_Circuit_V[2];

				// 	//Apply and see what happens
				// 	value_IGenerated[0] = temp_current_val[0];

				// }

				// //for grid forming inverter, the internal voltages and their norton equivalent currents need to be balanced
				// value_IGenerated[1].SetPolar(value_IGenerated[0].Mag(),value_IGenerated[0].Arg() - 2.0/3.0*PI);
				// value_IGenerated[2].SetPolar(value_IGenerated[0].Mag(),value_IGenerated[0].Arg() + 2.0/3.0*PI);
			}
			else // grid following or GFL_CURRENT_SOURCE
			{
				if (grid_following_mode == BALANCED_POWER) // Assume the grid-following inverter inject balanced power to the grid
				{
					// Adjust VSI (not on SWING bus) current injection and e_source values only at the first iteration of each time step
					if (attached_bus_type != 2)
					{

						complex temp_VA = complex(Pref, Qref);

						//Copy in value
						//temp_power_val[0] = power_val[0] + (temp_VA - VA_Out) / 3.0;
						//temp_power_val[1] = power_val[1] + (temp_VA - VA_Out) / 3.0;
						//temp_power_val[2] = power_val[2] + (temp_VA - VA_Out) / 3.0;

						//Copy in value
						temp_power_val[0] = temp_VA / 3.0;
						temp_power_val[1] = temp_VA / 3.0;
						temp_power_val[2] = temp_VA / 3.0;

						//Back out the current injection
						temp_current_val[0] = ~(temp_power_val[0] / value_Circuit_V[0]) + generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2];
						temp_current_val[1] = ~(temp_power_val[1] / value_Circuit_V[1]) + generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2];
						temp_current_val[2] = ~(temp_power_val[2] / value_Circuit_V[2]) + generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2];

						//Apply and see what happens
						//for grid following inverter, the internal voltages may not be balanced
						value_IGenerated[0] = temp_current_val[0];
						value_IGenerated[1] = temp_current_val[1];
						value_IGenerated[2] = temp_current_val[2];
					}
				}	 // end of balanced power
				else // Positive sequence. Assume the grid-following inverter inject balanced currents to the grid
				{
					// Adjust VSI (not on SWING bus) current injection and e_source values only at the first iteration of each time step
					if (attached_bus_type != 2)
					{

						complex temp_VA = complex(Pref, Qref);

						// Obtain the positive sequence voltage
						value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
						// Obtain the positive sequence current
						value_Circuit_I_PS[0] = ~(temp_VA / value_Circuit_V_PS) / 3.0;
						value_Circuit_I_PS[1] = value_Circuit_I_PS[0] * complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI));
						value_Circuit_I_PS[2] = value_Circuit_I_PS[0] * complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI));

						//Back out the current injection
						temp_current_val[0] = value_Circuit_I_PS[0] + generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2];
						temp_current_val[1] = value_Circuit_I_PS[1] + generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2];
						temp_current_val[2] = value_Circuit_I_PS[2] + generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2];

						//Apply and see what happens
						//for grid following inverter, the internal voltages may not be balanced
						value_IGenerated[0] = temp_current_val[0];
						value_IGenerated[1] = temp_current_val[1];
						value_IGenerated[2] = temp_current_val[2];
					}
				}
			}
		}
	}

	//Sync the powerflow variables
	if (parent_is_a_meter == true)
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
void inverter_dyn::update_iGen(complex VA_Out)
{
	if (parent_is_single_phase == true) // single phase/split-phase implementation
	{
		// power_val[0], temp_current_val[0], & value_IGenerated[0]
		power_val[0] = VA_Out;
		temp_current_val[0] = ~(power_val[0] / value_Circuit_V[0]);
		value_IGenerated[0] = temp_current_val[0] + filter_admittance * value_Circuit_V[0];
	}
	else
	{
		// power_val, temp_current_val, & value_IGenerated
		power_val[0] = power_val[1] = power_val[2] = VA_Out / 3;

		temp_current_val[0] = ~(power_val[0] / value_Circuit_V[0]);
		temp_current_val[1] = ~(power_val[1] / value_Circuit_V[1]);
		temp_current_val[2] = ~(power_val[2] / value_Circuit_V[2]);

		value_IGenerated[0] = temp_current_val[0] - (-generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
		value_IGenerated[1] = temp_current_val[1] - (-generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
		value_IGenerated[2] = temp_current_val[2] - (-generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);
	}
}

/* Check the inverter output and make sure it is in the limit */
bool inverter_dyn::check_and_update_VA_Out(OBJECT *obj)
{
	bool flag_VA_Out_changed = false;

	if (grid_forming_mode == DYNAMIC_DC_BUS)
	{
		// Check if pvc_Pmax is nonnegative
		if (pvc_Pmax < 0)
		{
			GL_THROW("pvc_Pmax (%f [W]) is negative!!",
					 pvc_Pmax);
		}

		// Check P_Out & Q_Out
		double P_Out = VA_Out.Re();
		double Q_Out = VA_Out.Im();

		//== check P_Out
		double P_Out_lim = pvc_Pmax;
		if (S_base < pvc_Pmax) //i.e., double P_Out_lim = S_base < pvc_Pmax ? S_base : pvc_Pmax;
		{	
			gl_warning("inverter_dyn:%d - %s - P_Out is capped at a rated power of %f [W].",obj->id,(obj->name ? obj->name : "unnamed"), S_base);
			P_Out_lim = S_base;
		}
		double P_Out_lim_flr = floor(P_Out_lim); //P_Out_lim is always positive; And this is important to guarantee that the VA_Out.Re() will be smaller than pvc_Pmax.

		if (abs(P_Out) > P_Out_lim_flr)
		{
			gl_warning("inverter_dyn:%d - %s - The absolute value of P_Out (%f [W] required by powerflow) is capped at %f [W].",obj->id,(obj->name ? obj->name : "unnamed"), P_Out, P_Out_lim_flr);

			P_Out = copysign(P_Out_lim_flr, P_Out);
			flag_VA_Out_changed = true;
		}

		if (abs(Pref) > P_Out_lim_flr)
		{
			gl_warning("inverter_dyn:%d - %s - The absolute value of Pref (%f [W] set by the user) is capped at %f [W].",obj->id,(obj->name ? obj->name : "unnamed"), Pref, P_Out_lim_flr);

			Pref = copysign(P_Out_lim_flr, Pref);
		}

		//== check Q_Out
		double Q_Out_lim_sq = 2 * pow(S_base, 2) - pow(P_Out, 2);

		if (Q_Out_lim_sq < 0) // Technically, there is no need to check the 'Q_Out_lim_sq', but there is no harm to double check
		{
			GL_THROW("inverter_dyn:%d - %s - Rated power (%f [W]) must be at least 70.71% of the P_Out (%f) [W]",obj->id,(obj->name?obj->name:"Unnamed"),S_base, P_Out);
		}
		else
		{
			double Q_Out_lim_flr = floor(sqrt(Q_Out_lim_sq)); // with floor(), the margin is safer

			if (abs(Q_Out) > Q_Out_lim_flr)
			{
				gl_warning("inverter_dyn:%d - %s - Magnitude of Q_Out (%f [var] required by powerflow) is be capped at %f [var].",obj->id,(obj->name ? obj->name : "unnamed"), Q_Out, Q_Out_lim_flr);

				Q_Out = copysign(Q_Out_lim_flr, Q_Out);
				flag_VA_Out_changed = true;
			}

			if (abs(Qref) > Q_Out_lim_flr)
			{
				gl_warning("inverter_dyn:%d - %s - Absolute value of Qref is capped at %f [var].",obj->id,(obj->name ? obj->name : "unnamed"), Q_Out_lim_flr);

				Qref = copysign(Q_Out_lim_flr, Qref);
			}
		}

		if (flag_VA_Out_changed)
		{
			// Update VA_Out
			VA_Out = complex(P_Out, Q_Out);
			update_iGen(VA_Out);
		}

		// Update the P_DC
		P_DC = VA_Out.Re(); //Lossless

		// Update V_DC
		if (dc_interface_objects.empty() != true)
		{
			int temp_idx;
			STATUS fxn_return_status;

			//Loop through and call the DC objects
			for (temp_idx = 0; temp_idx < dc_interface_objects.size(); temp_idx++)
			{
				//DC object, calling object (us), init mode (true/false)
				//False at end now, because not initialization
				fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *, bool))(*dc_interface_objects[temp_idx].fxn_address))(dc_interface_objects[temp_idx].dc_object, obj, true);

				//Make sure it worked
				if (fxn_return_status == FAILED)
				{
					//Pull the object from the array - this is just for readability (otherwise the
					OBJECT *temp_obj = dc_interface_objects[temp_idx].dc_object;

					//Error it up
					GL_THROW("inverter_dyn:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
					//Defined above
				}
			}
		}

		// Update I_DC
		I_DC = P_DC/V_DC;
	}
	return flag_VA_Out_changed;
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP inverter_dyn::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2 = TS_NEVER; //By default, we're done forever!

	//If we have a meter, reset the accumulators
	if (parent_is_a_meter == true)
	{
		reset_complex_powerflow_accumulators();

		//Also pull the current values
		pull_complex_powerflow_values();
	}

	if (parent_is_single_phase == true) // single phase/split-phase implementation
	{
		//Update output power
		//Get current injected
		temp_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

		//Update power output variables, just so we can see what is going on
		VA_Out = value_Circuit_V[0] * ~temp_current_val[0];
	}
	else //Three-phase, by default
	{
		//Update output power
		//Get current injected
		temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
		temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
		temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

		//Update power output variables, just so we can see what is going on
		power_val[0] = value_Circuit_V[0] * ~temp_current_val[0];
		power_val[1] = value_Circuit_V[1] * ~temp_current_val[1];
		power_val[2] = value_Circuit_V[2] * ~temp_current_val[2];

		VA_Out = power_val[0] + power_val[1] + power_val[2];
	}

	// Limit check for P_Out & Q_Out
	bool flag_VA_Out_changed = check_and_update_VA_Out(obj);

	// Sync the powerflow variables
	if (parent_is_a_meter == true)
	{
		push_complex_powerflow_values(false);
	}

	if (flag_VA_Out_changed)
	{
		return t1;
	}
	else
	{
		return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Preupdate
STATUS inverter_dyn::pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time)
{
	STATUS stat_val;
	FUNCTIONADDR funadd = NULL;
	OBJECT *hdr = OBJECTHDR(this);

	//Call the init, since we're here
	stat_val = init_dynamics(&curr_state);

	if (stat_val != SUCCESS)
	{
		gl_error("Inverter_dyn failed pre_deltaupdate call");
		/*  TROUBLESHOOT
		While attempting to call the pre_deltaupdate portion of the inverter_dyn code, an error
		was encountered.  Please submit your code and a bug report via the ticketing system.
		*/

		return FAILED;
	}

	if (control_mode == GRID_FORMING)
	{
		//If we're a voltage-source inverter, also swap our SWING bus, just because
		//map the function
		funadd = (FUNCTIONADDR)(gl_get_function(hdr->parent, "pwr_object_swing_swapper"));

		//make sure it worked
		if (funadd == NULL)
		{
			gl_error("inverter_dyn:%s -- Failed to find node swing swapper function", (hdr->name ? hdr->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to map the function to change the swing status of the parent bus, the function could not be found.
			Ensure the inverter_dyn is actually attached to something.  If the error persists, please submit your code and a bug report
			via the ticketing/issues system.
			*/

			return FAILED;
		}

		//Call the swap
		stat_val = ((STATUS(*)(OBJECT *, bool))(*funadd))(hdr->parent, false);

		if (stat_val == 0) //Failed :(
		{
			gl_error("Failed to swap SWING status of node:%s on inverter_dyn:%s", (hdr->parent->name ? hdr->parent->name : "Unnamed"), (hdr->name ? hdr->name : "Unnamed"));
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
SIMULATIONMODE inverter_dyn::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
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
	if (parent_is_a_meter == true)
	{
		reset_complex_powerflow_accumulators();

		pull_complex_powerflow_values();
	}

	//Get timestep value
	deltat = (double)dt / (double)DT_SECOND;
	deltath = deltat / 2.0;

	//Update time tracking variable
	prev_timestamp_dbl = gl_globaldeltaclock;

	if (control_mode == GRID_FORMING)
	{
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

				if (dc_interface_objects.empty() != true)
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
							GL_THROW("inverter_dyn:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_DC_obj->id, (temp_DC_obj->name ? temp_DC_obj->name : "Unnamed"));
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
			//Caluclate injection current based on voltage soruce magtinude and angle obtained
			if (parent_is_single_phase == true) // single phase/split-phase implementation
			{

				//Update output power
				//Get current injected
				temp_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

				//Update power output variables, just so we can see what is going on
				VA_Out = value_Circuit_V[0] * ~temp_current_val[0];
			}
			else //Three-phase
			{
				//Update output power
				//Get current injected
				temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
				temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
				temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

				//Update power output variables, just so we can see what is going on
				power_val[0] = value_Circuit_V[0] * ~temp_current_val[0];
				power_val[1] = value_Circuit_V[1] * ~temp_current_val[1];
				power_val[2] = value_Circuit_V[2] * ~temp_current_val[2];

				VA_Out = power_val[0] + power_val[1] + power_val[2];

				// The following code is only for three phase system
				// Function: Low pass filter of P
				P_out_pu = VA_Out.Re() / S_base;
				pred_state.dp_measure = 1.0 / Tp * (P_out_pu - curr_state.p_measure);
				pred_state.p_measure = curr_state.p_measure + (deltat * pred_state.dp_measure);

				// VA_OUT.Re() refers to the output active power from the inverter.
				// S_base is the rated capacity
				// P_out_pu is the per unit value of VA_OUT.Re()
				// p_measure is the filtered active power, it is per-unit value
				// Tp is the time constant of low pass filter, it is per-unit value
				// Function end

				// Function: Low pass filter of Q
				Q_out_pu = VA_Out.Im() / S_base;
				pred_state.dq_measure = 1.0 / Tq * (Q_out_pu - curr_state.q_measure);
				pred_state.q_measure = curr_state.q_measure + (deltat * pred_state.dq_measure);

				// VA_OUT.Im() refers to the output reactive power from the inverter
				// Q_out_pu is the per-unit value of VA_Out.Im()
				// q_measure is the filtered reactive power, it is per-unit value
				// Tq is the time constant of low pass filter, it is per-unit value
				// Function end

				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus
				{
					I_dc_pu = P_out_pu / curr_state.Vdc_pu; // Calculate the equivalent dc current, including the dc capacitor

					if (dc_interface_objects.empty() != true)
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
								GL_THROW("inverter_dyn:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
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
				pred_state.dv_measure = 1.0 / Tv * (pCircuit_V_Avg_pu - curr_state.v_measure);
				pred_state.v_measure = curr_state.v_measure + (deltat * pred_state.dv_measure);

				// Value_Circuit_V[i] refers to the voltage of each phase at the grid side
				// Vbase is the rated Line to ground voltage
				// pCircuit_V_Avg_pu refers to the average value of three phase voltages, it is per-unit value
				// v_measure refers to filtered voltage, it is per-unit value
				// Tv is the time constant of low pass filter
				// Function end

				// Function: Q-V droop control and voltage control loop
				V_ref = Vset - pred_state.q_measure * mq;
				pred_state.dV_ini = (V_ref - pred_state.v_measure) * kiv;
				pred_state.V_ini = curr_state.V_ini + pred_state.dV_ini * deltat;

				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
				{

					if (pred_state.V_ini > pred_state.Vdc_pu * mdc) // E_max = 1.2, V_DC/Vdc_base
					{
						pred_state.V_ini = pred_state.Vdc_pu * mdc;
					}

					if (pred_state.V_ini < E_min) // E_min = 0
					{
						pred_state.V_ini = E_min;
					}

					E_mag = pred_state.V_ini + pred_state.dV_ini / kiv * kpv;

					if (E_mag > pred_state.Vdc_pu * mdc) // E_max = 1
					{
						E_mag = pred_state.Vdc_pu * mdc;
					}

					if (E_mag < E_min) // E_min = 0
					{
						E_mag = E_min;
					}

					//E_mag = E_mag * (V_DC/Vdc_base);

					// V_ref is the voltage reference obtained from Q-V droop
					// Vset is the voltage set point, usually 1 pu
					// mq is the Q-V droop gain, usually 0.05 pu
					// V_ini is the output from the integrator in the voltage controller
					// E_mag is the output of the votlage controller, it is the voltage magnitude of the internal voltage
					// E_max and E_min are the maximum and minimum of the output of voltage controller
					// Function end
				}
				else
				{

					if (pred_state.V_ini > E_max) // E_max = 1.2, V_DC/Vdc_base
					{
						pred_state.V_ini = E_max;
					}

					if (pred_state.V_ini < E_min) // E_min = 0
					{
						pred_state.V_ini = E_min;
					}

					E_mag = pred_state.V_ini + pred_state.dV_ini / kiv * kpv;

					if (E_mag > E_max) // E_max = 1
					{
						E_mag = E_max;
					}

					if (E_mag < E_min) // E_min = 0
					{
						E_mag = E_min;
					}

					// V_ref is the voltage reference obtained from Q-V droop
					// Vset is the voltage set point, usually 1 pu
					// mq is the Q-V droop gain, usually 0.05 pu
					// V_ini is the output from the integrator in the voltage controller
					// E_mag is the output of the votlage controller, it is the voltage magnitude of the internal voltage
					// E_max and E_min are the maximum and minimum of the output of voltage controller
					// Function end
				}



				// Function: P-f droop, Pmax and Pmin controller
				delta_w_droop = (Pset - pred_state.p_measure) * mp; // P-f droop

				// Pmax controller
				pred_state.ddelta_w_Pmax_ini = (Pmax - pred_state.p_measure) * kipmax;
				pred_state.delta_w_Pmax_ini = curr_state.delta_w_Pmax_ini + pred_state.ddelta_w_Pmax_ini * deltat;

				if (pred_state.delta_w_Pmax_ini > 0)
				{
					pred_state.delta_w_Pmax_ini = 0;
				}

				if (pred_state.delta_w_Pmax_ini < -w_lim) // -w_lim = -50
				{
					pred_state.delta_w_Pmax_ini = -w_lim;
				}

				delta_w_Pmax = pred_state.delta_w_Pmax_ini + pred_state.ddelta_w_Pmax_ini / kipmax * kppmax; //output from Pmax controller

				if (delta_w_Pmax > 0) //
				{
					delta_w_Pmax = 0;
				}

				if (delta_w_Pmax < -w_lim) // -w_lim = -50
				{
					delta_w_Pmax = -w_lim;
				}

				// Pmin controller
				pred_state.ddelta_w_Pmin_ini = (Pmin - pred_state.p_measure) * kipmax;
				pred_state.delta_w_Pmin_ini = curr_state.delta_w_Pmin_ini + pred_state.ddelta_w_Pmin_ini * deltat;

				if (pred_state.delta_w_Pmin_ini < 0) //
				{
					pred_state.delta_w_Pmin_ini = 0;
				}

				if (pred_state.delta_w_Pmin_ini > w_lim) // w_lim = 50
				{
					pred_state.delta_w_Pmin_ini = w_lim;
				}

				delta_w_Pmin = pred_state.delta_w_Pmin_ini + pred_state.ddelta_w_Pmin_ini / kipmax * kppmax; // output from Pmin controller

				if (delta_w_Pmin < 0) //
				{
					delta_w_Pmin = 0;
				}

				if (delta_w_Pmin > w_lim) // w_lim = 50
				{
					delta_w_Pmin = w_lim;
				}

				pred_state.delta_w = delta_w_droop + delta_w_Pmax + delta_w_Pmin + 2.0 * PI * fset - w_ref; //the summation of the outputs from P-f droop, Pmax control and Pmin control

				// delta_w_droop is the output of P-f droop
				// Pset is the power set point
				// delta_w_Pmax_ini and delta_w_Pmin_ini are the outputs of the integrator of Pmax controller and Pmin controller
				// delta_w_Pmax and delta_w_Pmin are the outputs of Pmax controller and Pmin controller
				// Pmax and Pmin are the maximum limit and minimum limit of Pmax controller and Pmin controller
				// w_lim is the saturation limit
				// w_ref is the rated frequency, usually 376.99 rad/s
				// Function end

				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
				{

					// Vdc_min controller to protect the dc bus voltage from collapsing
					pred_state.ddelta_w_Vdc_min_ini = (pred_state.Vdc_pu - Vdc_min_pu) * kiVdc;
					pred_state.delta_w_Vdc_min_ini = curr_state.delta_w_Vdc_min_ini + pred_state.ddelta_w_Vdc_min_ini * deltat;

					if (pred_state.delta_w_Vdc_min_ini > 0) //
					{
						pred_state.delta_w_Vdc_min_ini = 0;
					}

					delta_w_Vdc_min = pred_state.delta_w_Vdc_min_ini + pred_state.ddelta_w_Vdc_min_ini / kiVdc * kpVdc + (pred_state.Vdc_pu - curr_state.Vdc_pu) * kdVdc / deltat; // output from Vdc_min controller

					if (delta_w_Vdc_min > 0) //
					{
						delta_w_Vdc_min = 0;
					}

					pred_state.delta_w = pred_state.delta_w + delta_w_Vdc_min; //the summation of the outputs from P-f droop, Pmax control and Pmin control, and Vdc_min control
				}

				freq = (pred_state.delta_w + w_ref) / 2.0 / PI; // The frequency from the CERTS Droop controller, Hz

				// Function: Obtaining the Phase Angle, and obtaining the compelx value of internal voltages and their Norton Equivalence for power flow analysis
				for (i = 0; i < 3; i++)
				{
					pred_state.Angle[i] = curr_state.Angle[i] + pred_state.delta_w * deltat;							//Obtain the phase angle
					e_source[i] = complex(E_mag * cos(pred_state.Angle[i]), E_mag * sin(pred_state.Angle[i])) * V_base; // transfers back to non-per-unit values
					value_IGenerated[i] = e_source[i] / (complex(Rfilter, Xfilter) * Z_base);							// Thevenin voltage source to Norton current source convertion
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
			if (parent_is_single_phase == true) // single phase/split-phase implementation
			{
				//Update output power
				//Get current injected
				temp_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

				//Update power output variables, just so we can see what is going on
				VA_Out = value_Circuit_V[0] * ~temp_current_val[0];
			}
			else //Three-phase
			{
				//Update output power
				//Get current injected
				temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
				temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
				temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

				//Update power output variables, just so we can see what is going on
				power_val[0] = value_Circuit_V[0] * ~temp_current_val[0];
				power_val[1] = value_Circuit_V[1] * ~temp_current_val[1];
				power_val[2] = value_Circuit_V[2] * ~temp_current_val[2];

				VA_Out = power_val[0] + power_val[1] + power_val[2];

				// The following code is only for three phase system
				// Function: Low pass filter of P
				P_out_pu = VA_Out.Re() / S_base;
				next_state.dp_measure = 1.0 / Tp * (P_out_pu - pred_state.p_measure);
				next_state.p_measure = curr_state.p_measure + (pred_state.dp_measure + next_state.dp_measure) * deltat / 2.0;

				// VA_OUT.Re() refers to the output active power from the inverter, this should be normalized.
				// S_base is the rated capacity
				// P_out_pu is the per unit value of VA_OUT.Re()
				// p_measure is the filtered active power, it is per-unit value
				// Tp is the time constant of low pass filter, it is per-unit value
				// Function end

				// Function: Low pass filter of Q
				Q_out_pu = VA_Out.Im() / S_base;
				next_state.dq_measure = 1.0 / Tq * (Q_out_pu - pred_state.q_measure);
				next_state.q_measure = curr_state.q_measure + (pred_state.dq_measure + next_state.dq_measure) * deltat / 2.0;

				// VA_OUT.Im() refers to the output reactive power from the inverter
				// Q_out_pu is the per-unit value of VA_Out.Im()
				// q_measure is the filtered reactive power, it is per-unit value
				// Tq is the time constant of low pass filter, it is per-unit value
				// Function end

				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus
				{
					I_dc_pu = P_out_pu / pred_state.Vdc_pu; // Calculate the equivalent dc current, including the dc capacitor

					if (dc_interface_objects.empty() != true)
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
								GL_THROW("inverter_dyn:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
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
				next_state.dv_measure = 1.0 / Tv * (pCircuit_V_Avg_pu - pred_state.v_measure);
				next_state.v_measure = curr_state.v_measure + (pred_state.dv_measure + next_state.dv_measure) * deltat / 2.0;

				// Value_Circuit_V[i] refers to te voltage of each phase at the inverter terminal
				// Vbase is the rated Line to ground voltage
				// pCircuit_V_Avg_pu refers to the average value of three phase voltages, it is per-unit value
				// v_measure refers to filtered voltage, it is per-unit value
				// Function end

				// Function: Q-V droop control and voltage control loop
				V_ref = Vset - next_state.q_measure * mq;
				next_state.dV_ini = (V_ref - next_state.v_measure) * kiv;
				next_state.V_ini = curr_state.V_ini + (pred_state.dV_ini + next_state.dV_ini) * deltat / 2.0;

				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
				{
					if (next_state.V_ini > next_state.Vdc_pu * mdc) // E_max = 1.2, V_DC/Vdc_base
					{
						next_state.V_ini = next_state.Vdc_pu * mdc;
					}

					if (next_state.V_ini < E_min) // E_min = 0
					{
						next_state.V_ini = E_min;
					}

					E_mag = next_state.V_ini + next_state.dV_ini / kiv * kpv;

					if (E_mag > next_state.Vdc_pu * mdc) // E_max = 1.2
					{
						E_mag = next_state.Vdc_pu * mdc;
					}

					if (E_mag < E_min) // E_min = 0
					{
						E_mag = E_min;
					}

					//E_mag = E_mag * (V_DC/Vdc_base);

					// V_ref is the voltage reference obtained from Q-V droop
					// Vset is the voltage set point, usually 1 pu
					// mq is the Q-V droop gain, usually 0.05 pu
					// V_ini is the output from the integrator in the voltage controller
					// E_mag is the output of the votlage controller, it is the voltage magnitude of the internal voltage
					// E_max and E_min are the maximum and minimum of the output of voltage controller
					// Function end
				}
				else
				{

					if (next_state.V_ini > E_max) // E_max = 1.2, V_DC/Vdc_base
					{
						next_state.V_ini = E_max;
					}

					if (next_state.V_ini < E_min) // E_min = 0
					{
						next_state.V_ini = E_min;
					}

					E_mag = next_state.V_ini + next_state.dV_ini / kiv * kpv;

					if (E_mag > E_max) // E_max = 1.2
					{
						E_mag = E_max;
					}

					if (E_mag < E_min) // E_min = 0
					{
						E_mag = E_min;
					}

					//E_mag = E_mag * (V_DC/Vdc_base);

					// V_ref is the voltage reference obtained from Q-V droop
					// Vset is the voltage set point, usually 1 pu
					// mq is the Q-V droop gain, usually 0.05 pu
					// V_ini is the output from the integrator in the voltage controller
					// E_mag is the output of the votlage controller, it is the voltage magnitude of the internal voltage
					// E_max and E_min are the maximum and minimum of the output of voltage controller
					// Function end
				}

				// Function: P-f droop, Pmax and Pmin controller
				delta_w_droop = (Pset - next_state.p_measure) * mp; // P-f droop

				// Pmax controller
				next_state.ddelta_w_Pmax_ini = (Pmax - next_state.p_measure) * kipmax;
				next_state.delta_w_Pmax_ini = curr_state.delta_w_Pmax_ini + (pred_state.ddelta_w_Pmax_ini + next_state.ddelta_w_Pmax_ini) * deltat / 2.0;

				if (next_state.delta_w_Pmax_ini > 0) //
				{
					next_state.delta_w_Pmax_ini = 0;
				}

				if (next_state.delta_w_Pmax_ini < -w_lim) // -w_lim = -50
				{
					next_state.delta_w_Pmax_ini = -w_lim;
				}

				delta_w_Pmax = next_state.delta_w_Pmax_ini + next_state.ddelta_w_Pmax_ini / kipmax * kppmax; //output from Pmax controller

				if (delta_w_Pmax > 0) //
				{
					delta_w_Pmax = 0;
				}

				if (delta_w_Pmax < -w_lim) // -w_lim = -50
				{
					delta_w_Pmax = -w_lim;
				}

				// Pmin controller
				next_state.ddelta_w_Pmin_ini = (Pmin - next_state.p_measure) * kipmax;
				next_state.delta_w_Pmin_ini = curr_state.delta_w_Pmin_ini + (pred_state.ddelta_w_Pmin_ini + next_state.ddelta_w_Pmin_ini) * deltat / 2.0;

				if (next_state.delta_w_Pmin_ini < 0) //
				{
					next_state.delta_w_Pmin_ini = 0;
				}

				if (next_state.delta_w_Pmin_ini > w_lim) // w_lim = 50
				{
					next_state.delta_w_Pmin_ini = w_lim;
				}

				delta_w_Pmin = next_state.delta_w_Pmin_ini + next_state.ddelta_w_Pmin_ini / kipmax * kppmax; // output from Pmin controller

				if (delta_w_Pmin < 0) //
				{
					delta_w_Pmin = 0;
				}

				if (delta_w_Pmin > w_lim) // w_lim = 50
				{
					delta_w_Pmin = w_lim;
				}

				next_state.delta_w = delta_w_droop + delta_w_Pmax + delta_w_Pmin + 2.0 * PI * fset - w_ref; //the summation of the outputs from P-f droop, Pmax control and Pmin control

				// delta_w_droop is the output of P-f droop
				// Pset is the power set point
				// delta_w_Pmax_ini and delta_w_Pmin_ini are the outputs of the integrator of Pmax controller and Pmin controller
				// delta_w_Pmax and delta_w_Pmin are the outputs of Pmax controller and Pmin controller
				// Pmax and Pmin are the maximum limit and minimum limit of Pmax controller and Pmin controller
				// w_lim is the saturation limit
				// w_ref is the rated frequency, usually 376.99 rad/s
				// Function end

				if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
				{

					// Vdc_min controller to protect the dc bus voltage from collapsing
					next_state.ddelta_w_Vdc_min_ini = (next_state.Vdc_pu - Vdc_min_pu) * kiVdc;
					next_state.delta_w_Vdc_min_ini = curr_state.delta_w_Vdc_min_ini + (pred_state.ddelta_w_Vdc_min_ini + next_state.ddelta_w_Vdc_min_ini) * deltat / 2.0;

					if (next_state.delta_w_Vdc_min_ini > 0) //
					{
						next_state.delta_w_Vdc_min_ini = 0;
					}

					delta_w_Vdc_min = next_state.delta_w_Vdc_min_ini + next_state.ddelta_w_Vdc_min_ini / kiVdc * kpVdc + ((next_state.Vdc_pu + pred_state.Vdc_pu) / 2 - curr_state.Vdc_pu) * kdVdc / deltat; // output from Vdc_min controller

					if (delta_w_Vdc_min > 0) //
					{
						delta_w_Vdc_min = 0;
					}

					next_state.delta_w = next_state.delta_w + delta_w_Vdc_min; //the summation of the outputs from P-f droop, Pmax control and Pmin control, and Vdc_min control
				}

				freq = (next_state.delta_w + w_ref) / 2.0 / PI; // The frequency from the CERTS droop controller, Hz

				// Function: Obtaining the Phase Angle, and obtaining the compelx value of internal voltages and their Norton Equivalence for power flow analysis
				for (i = 0; i < 3; i++)
				{
					next_state.Angle[i] = curr_state.Angle[i] + (pred_state.delta_w + next_state.delta_w) * deltat / 2.0; //Obtain the phase angle
					e_source[i] = complex(E_mag * cos(next_state.Angle[i]), E_mag * sin(next_state.Angle[i])) * V_base;	  // transfers back to non-per-unit values
					value_IGenerated[i] = e_source[i] / (complex(Rfilter, Xfilter) * Z_base);							  // Thevenin voltage source to Norton current source convertion

					//Convergence check - do on internal voltage, because "reasons"
					mag_diff_val = e_source[i].Mag() - e_source_prev[i].Mag();

					//Update tracker
					e_source_prev[i] = e_source[i];

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

				double diff_w = next_state.delta_w - curr_state.delta_w;

				memcpy(&curr_state, &next_state, sizeof(INV_DYN_STATE));

				if ((fabs(diff_w) <= GridForming_freq_convergence_criterion) && (proceed_to_qsts == true))
				{
					simmode_return_value = SM_EVENT; // we have reached steady state
				}
				else
				{
					simmode_return_value = SM_DELTA;
				}

				//simmode_return_value =  SM_DELTA;
			}
		}
		else //Additional iterations
		{
			//Just return whatever our "last desired" was
			simmode_return_value = desired_simulation_mode;
		}
	} // end of grid-forming
	else if ((control_mode == GRID_FOLLOWING) || (control_mode == GFL_CURRENT_SOURCE))
	{

		// Check pass
		if (iteration_count_val == 0) // Predictor pass
		{
			//Caluclate injection current based on voltage soruce magtinude and angle obtained
			if (parent_is_single_phase == true) // single phase/split-phase implementation
			{
				//Update output power
				//Get current injected
				temp_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

				//Update power output variables, just so we can see what is going on
				VA_Out = value_Circuit_V[0] * ~temp_current_val[0];

				// Function: Coordinate Tranformation, xy to dq
				ugd_pu[0] = (value_Circuit_V[0].Re() * cos(curr_state.Angle_PLL[0]) + value_Circuit_V[0].Im() * sin(curr_state.Angle_PLL[0])) / V_base;
				ugq_pu[0] = (-value_Circuit_V[0].Re() * sin(curr_state.Angle_PLL[0]) + value_Circuit_V[0].Im() * cos(curr_state.Angle_PLL[0])) / V_base;
				igd_pu[0] = (temp_current_val[0].Re() * cos(curr_state.Angle_PLL[0]) + temp_current_val[0].Im() * sin(curr_state.Angle_PLL[0])) / I_base;
				igq_pu[0] = (-temp_current_val[0].Re() * sin(curr_state.Angle_PLL[0]) + temp_current_val[0].Im() * cos(curr_state.Angle_PLL[0])) / I_base;

				// ugd_pu[i] and ugq_pu[i] are the per-unit values of grid-side voltages in dq frame
				// igd_pu[i] and igq_pu[i] are the per-unit values of grid-side currents in dq frame
				// Angle_PLL[i] is the phase angle of the grid side votlage, which is obtained from the PLL
				// Value_Circuit_V[i] is the voltage of each phase at the grid side
				// temp_current_val[i] is the current of each phase injected to the grid
				// S_base is the rated capacity
				// I_base is the reted current
				// Function end

				// Function: Phase-Lock_Loop, PLL
				pred_state.ddelta_w_PLL_ini[0] = ugq_pu[0] * kiPLL;
				pred_state.delta_w_PLL_ini[0] = curr_state.delta_w_PLL_ini[0] + pred_state.ddelta_w_PLL_ini[0] * deltat;	//output from the integrator term
				pred_state.delta_w_PLL[0] = pred_state.delta_w_PLL_ini[0] + pred_state.ddelta_w_PLL_ini[0] / kiPLL * kpPLL; // output from the PI controller
				fPLL[0] = (pred_state.delta_w_PLL[0] + w_ref) / 2.0 / PI;														// frequency measured by PLL
				pred_state.Angle_PLL[0] = curr_state.Angle_PLL[0] + pred_state.delta_w_PLL[0] * deltat;						// phase angle from PLL

				// delta_w_PLL_ini[i] is the output from the integrator term
				// delta_w_PLL[i] is the output from the PI controller
				// w_ref is the rated angular frequency, the value is 376.99 rad/s
				// fPLL is the frequency measured by PLL
				// Fuction end

				// Frequency-watt function enabled
				if (frequency_watt)
				{
					Pref_droop_pu_prev = Pref_droop_pu; // the value of Pref_droop_pu in last simulation step

					pred_state.df_filter = 1.0 / Tff * (fPLL[0] - curr_state.f_filter);
					pred_state.f_filter = curr_state.f_filter + (deltat * pred_state.df_filter);

					Pref_droop_pu = (f_nominal - pred_state.f_filter) / f_nominal / Rp + Pref / S_base;

					if (Pref_droop_pu > Pref_max)
					{
						Pref_droop_pu = Pref_max;
					}

					if (Pref_droop_pu < Pref_min)
					{
						Pref_droop_pu = Pref_min;
					}

					double power_diff_val = Pref_droop_pu_prev - Pref_droop_pu;

					if (checkRampRate_real == true)
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
						pred_state.dPref_droop_pu_filter = 1.0 / Tpf * (Pref_droop_pu - curr_state.Pref_droop_pu_filter);
						pred_state.Pref_droop_pu_filter = curr_state.Pref_droop_pu_filter + (deltat * pred_state.dPref_droop_pu_filter);
					}
				}
				// f_filter is the frequency pass through the low pass filter
				// Tf is the time constant in the frequency low pass filter
				// Pref_droop_pu is the power reference from the frequency-watt
				// Pref_droop_pu_filter is the power reference pass through the low pass filter
				// Tpf is the time constant in the power low pass filter
				// Pref_max and Pref_min are the upper and lower limits of power references
				// Function end

				// Volt-var function enabled
				if (volt_var)
				{
					Qref_droop_pu_prev = Qref_droop_pu;

					V_Avg_pu = value_Circuit_V[0].Mag() / V_base;
					pred_state.dV_filter = 1.0 / Tvf * (V_Avg_pu - curr_state.V_filter);
					pred_state.V_filter = curr_state.V_filter + (deltat * pred_state.dV_filter);

					Qref_droop_pu = (Vset - pred_state.V_filter) / Rq;

					if (Qref_droop_pu > Qref_max)
					{
						Qref_droop_pu = Qref_max;
					}

					if (Qref_droop_pu < Qref_min)
					{
						Qref_droop_pu = Qref_min;
					}

					double power_diff_val = Qref_droop_pu_prev - Qref_droop_pu;

					if(checkRampRate_reactive == true)
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
						pred_state.dQref_droop_pu_filter = 1.0 / Tqf * (Qref_droop_pu - curr_state.Qref_droop_pu_filter);
						pred_state.Qref_droop_pu_filter = curr_state.Qref_droop_pu_filter + (deltat * pred_state.dQref_droop_pu_filter);
					}
				}
				// V_Avg_pu is the average value of three phase voltages
				// Tvf is the time constant of voltage low pass filter
				// V_filter is the voltage pass through low pass filter
				// Qref_droop_pu if the Q reference from volt-var
				// Qref_droop_pu_filter is the Q pass through low pass filter
				// Qref_max and Qref_min are the upper and lower limits of Q references
				// Function end

				// Function: Current Control Loop
				if (frequency_watt)
				{
					if (checkRampRate_real == true)
					{
						igd_ref[0] = Pref_droop_pu / ugd_pu[0];
					}
					else
					{
						igd_ref[0] = pred_state.Pref_droop_pu_filter / ugd_pu[0];
					}
				}
				else
				{
					// get the current references
					igd_ref[0] = Pref / S_base / ugd_pu[0];
				}

				if (volt_var)
				{
					if(checkRampRate_reactive == true)
					{
						igq_ref[0] = -Qref_droop_pu / ugd_pu[0];
					}
					else
					{
						igq_ref[0] = -pred_state.Qref_droop_pu_filter / ugd_pu[0];
					}
				}
				else
				{
					igq_ref[0] = -Qref / S_base / ugd_pu[0];
				}

				if (control_mode == GRID_FOLLOWING)
				{
					// current loop in d axis
					pred_state.digd_PI_ini[0] = (igd_ref[0] - igd_pu[0]) * kic;
					pred_state.igd_PI_ini[0] = curr_state.igd_PI_ini[0] + pred_state.digd_PI_ini[0] * deltat; //output from the integrator term of the current control loop
					igd_PI[0] = pred_state.igd_PI_ini[0] + pred_state.digd_PI_ini[0] / kic * kpc;			  // output from the PI controller of current loop
					ed_pu[0] = igd_PI[0] + ugd_pu[0] - igq_pu[0] * Xfilter * F_current;						  // the d axis component of internal voltage, Xfilter is per-unit value
					// current loop in q axis
					pred_state.digq_PI_ini[0] = (igq_ref[0] - igq_pu[0]) * kic;
					pred_state.igq_PI_ini[0] = curr_state.igq_PI_ini[0] + pred_state.digq_PI_ini[0] * deltat; //output from the integrator term of the current control loop
					igq_PI[0] = pred_state.igq_PI_ini[0] + pred_state.digq_PI_ini[0] / kic * kpc;			  // output from the PI controller of current loop
					eq_pu[0] = igq_PI[0] + ugq_pu[0] + igd_pu[0] * Xfilter * F_current;						  // the d axis component of internal voltage, Xfilter is per-unit value

					// igd_PI_ini[i] and igq_PI_ini[i] are outputs from the integral terms in current control loops
					// igd_ref[i] and igq_ref[i] are the current references in dq frame
					// igd_PI[i] and igq_PI[i] are outputs from the current control loops
					// ed_pu[i] and eq_pu[i] are the dq components of the internal voltages
					// kpc and kic are the PI gains of the current loop
					// Function end

					// Function: Coordinate Transformation: dq to xy
					e_source_Re[0] = (ed_pu[0] * cos(pred_state.Angle_PLL[0]) - eq_pu[0] * sin(pred_state.Angle_PLL[0])) * V_base;
					e_source_Im[0] = (ed_pu[0] * sin(pred_state.Angle_PLL[0]) + eq_pu[0] * cos(pred_state.Angle_PLL[0])) * V_base;
					e_source[0] = complex(e_source_Re[0], e_source_Im[0]);
					value_IGenerated[0] = e_source[0] / (complex(Rfilter, Xfilter) * Z_base); // Thevenin voltage source to Norton current source convertion

					// e_source[i] is the complex value of internal voltage
					// value_IGenerated[i] is the Norton equivalent current source of e_source[i]
					// Rfilter and Xfilter are the per-unit values of inverter filter
					// Function end
				}
				else if (control_mode == GFL_CURRENT_SOURCE)
				{
					// Low pass filter for current id
					pred_state.digd_filter[0] = 1.0 / Tif * (igd_ref[0] - curr_state.igd_filter[0]);
					pred_state.igd_filter[0] = curr_state.igd_filter[0] + (deltat * pred_state.digd_filter[0]);

					// Low pass filter for current iq
					pred_state.digq_filter[0] = 1.0 / Tif * (igq_ref[0] - curr_state.igq_filter[0]);
					pred_state.igq_filter[0] = curr_state.igq_filter[0] + (deltat * pred_state.digq_filter[0]);
					// igd_ref[0] and igq_ref[0] are the current references in dq frame
					// igd_filter[0] and igq_filter[0] are the currents

					I_source_Re[0] = (pred_state.igd_filter[0] * cos(pred_state.Angle_PLL[0]) - pred_state.igq_filter[0] * sin(pred_state.Angle_PLL[0])) * I_base;
					I_source_Im[0] = (pred_state.igd_filter[0] * sin(pred_state.Angle_PLL[0]) + pred_state.igq_filter[0] * cos(pred_state.Angle_PLL[0])) * I_base;
					I_source[0] = complex(I_source_Re[0], I_source_Im[0]);
					value_IGenerated[0] = I_source[0];
					// I_source[0] is the complex value of injected current
					// value_IGenerated[0] is the current injected to the grid
					// Function end
				}

				simmode_return_value = SM_DELTA_ITER; //Reiterate - to get us to corrector pass
			}
			else //Three-phase
			{
				if ((grid_following_mode == BALANCED_POWER)||(grid_following_mode == POSITIVE_SEQUENCE))
				{
					//Update output power
					//Get current injected
					temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
					temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
					temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

					//Update power output variables, just so we can see what is going on
					power_val[0] = value_Circuit_V[0] * ~temp_current_val[0];
					power_val[1] = value_Circuit_V[1] * ~temp_current_val[1];
					power_val[2] = value_Circuit_V[2] * ~temp_current_val[2];

					VA_Out = power_val[0] + power_val[1] + power_val[2];

					// Function: Coordinate Tranformation, xy to dq
					for (i = 0; i < 3; i++)
					{
						ugd_pu[i] = (value_Circuit_V[i].Re() * cos(curr_state.Angle_PLL[i]) + value_Circuit_V[i].Im() * sin(curr_state.Angle_PLL[i])) / V_base;
						ugq_pu[i] = (-value_Circuit_V[i].Re() * sin(curr_state.Angle_PLL[i]) + value_Circuit_V[i].Im() * cos(curr_state.Angle_PLL[i])) / V_base;
						igd_pu[i] = (temp_current_val[i].Re() * cos(curr_state.Angle_PLL[i]) + temp_current_val[i].Im() * sin(curr_state.Angle_PLL[i])) / I_base;
						igq_pu[i] = (-temp_current_val[i].Re() * sin(curr_state.Angle_PLL[i]) + temp_current_val[i].Im() * cos(curr_state.Angle_PLL[i])) / I_base;
					}
					// ugd_pu[i] and ugq_pu[i] are the per-unit values of grid-side voltages in dq frame
					// igd_pu[i] and igq_pu[i] are the per-unit values of grid-side currents in dq frame
					// Angle_PLL[i] is the phase angle of the grid side votlage, which is obtained from the PLL
					// Value_Circuit_V[i] is the voltage of each phase at the grid side
					// temp_current_val[i] is the current of each phase injected to the grid
					// S_base is the rated capacity
					// I_base is the reted current
					// Function end

					if(grid_following_mode == BALANCED_POWER)
					{
						// Function: Phase-Lock_Loop, PLL
						for (i = 0; i < 3; i++)
						{
							pred_state.ddelta_w_PLL_ini[i] = ugq_pu[i] * kiPLL;
							pred_state.delta_w_PLL_ini[i] = curr_state.delta_w_PLL_ini[i] + pred_state.ddelta_w_PLL_ini[i] * deltat;	//output from the integrator term
							pred_state.delta_w_PLL[i] = pred_state.delta_w_PLL_ini[i] + pred_state.ddelta_w_PLL_ini[i] / kiPLL * kpPLL; // output from the PI controller
							fPLL[i] = (pred_state.delta_w_PLL[i] + w_ref) / 2.0 / PI;														// frequency measured by PLL
							pred_state.Angle_PLL[i] = curr_state.Angle_PLL[i] + pred_state.delta_w_PLL[i] * deltat;						// phase angle from PLL
						}
						// delta_w_PLL_ini[i] is the output from the integrator term
						// delta_w_PLL[i] is the output from the PI controller
						// w_ref is the rated angular frequency, the value is 376.99 rad/s
						// fPLL is the frequency measured by PLL
						// Fuction end
					}
					else if(grid_following_mode == POSITIVE_SEQUENCE)
					{
						// Obtain the positive sequence voltage
						value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
						// Positive sequence value of voltage in dq frame
						ugd_pu_PS = (value_Circuit_V_PS.Re() * cos(curr_state.Angle_PLL[0]) + value_Circuit_V_PS.Im() * sin(curr_state.Angle_PLL[0])) / V_base;
						ugq_pu_PS = (-value_Circuit_V_PS.Re() * sin(curr_state.Angle_PLL[0]) + value_Circuit_V_PS.Im() * cos(curr_state.Angle_PLL[0])) / V_base;

						// Function: Phase-Lock_Loop, PLL, only consider positive sequence voltage
						for (i = 0; i < 1; i++)
						{
							pred_state.ddelta_w_PLL_ini[i] = ugq_pu_PS * kiPLL;
							pred_state.delta_w_PLL_ini[i] = curr_state.delta_w_PLL_ini[i] + pred_state.ddelta_w_PLL_ini[i] * deltat;	//output from the integrator term
							pred_state.delta_w_PLL[i] = pred_state.delta_w_PLL_ini[i] + pred_state.ddelta_w_PLL_ini[i] / kiPLL * kpPLL; // output from the PI controller
							fPLL[i] = (pred_state.delta_w_PLL[i] + w_ref) / 2.0 / PI;														// frequency measured by PLL
							pred_state.Angle_PLL[i] = curr_state.Angle_PLL[i] + pred_state.delta_w_PLL[i] * deltat;						// phase angle from PLL
						}
						pred_state.Angle_PLL[1] = pred_state.Angle_PLL[0] - 2.0 / 3.0 * PI;
						pred_state.Angle_PLL[2] = pred_state.Angle_PLL[0] + 2.0 / 3.0 * PI;
						fPLL[2] = fPLL[1] = fPLL[0];
						// delta_w_PLL_ini[i] is the output from the integrator term
						// delta_w_PLL[i] is the output from the PI controller
						// w_ref is the rated angular frequency, the value is 376.99 rad/s
						// fPLL is the frequency measured by PLL
						// Fuction end
					}


					// Frequency-watt function enabled
					if (frequency_watt)
					{
						Pref_droop_pu_prev = Pref_droop_pu; // the value of Pref_droop_pu in last simulation step

						pred_state.df_filter = 1.0 / Tff * ((fPLL[0] + fPLL[1] + fPLL[2]) / 3.0 - curr_state.f_filter);
						pred_state.f_filter = curr_state.f_filter + (deltat * pred_state.df_filter);

						Pref_droop_pu = (f_nominal - pred_state.f_filter) / f_nominal / Rp + Pref / S_base;

						if (Pref_droop_pu > Pref_max)
						{
							Pref_droop_pu = Pref_max;
						}

						if (Pref_droop_pu < Pref_min)
						{
							Pref_droop_pu = Pref_min;
						}

						double power_diff_val = Pref_droop_pu_prev - Pref_droop_pu;

						if (checkRampRate_real == true)
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
							pred_state.dPref_droop_pu_filter = 1.0 / Tpf * (Pref_droop_pu - curr_state.Pref_droop_pu_filter);
							pred_state.Pref_droop_pu_filter = curr_state.Pref_droop_pu_filter + (deltat * pred_state.dPref_droop_pu_filter);
						}
					}
					// f_filter is the frequency pass through the low pass filter
					// Tf is the time constant in the frequency low pass filter
					// Pref_droop_pu is the power reference from the frequency-watt
					// Pref_droop_pu_filter is the power reference pass through the low pass filter
					// Tpf is the time constant in the power low pass filter
					// Pref_max and Pref_min are the upper and lower limits of power references
					// Function end

					// Volt-var function enabled
					if (volt_var)
					{
						Qref_droop_pu_prev = Qref_droop_pu;
						V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;
						pred_state.dV_filter = 1.0 / Tvf * (V_Avg_pu - curr_state.V_filter);
						pred_state.V_filter = curr_state.V_filter + (deltat * pred_state.dV_filter);

						Qref_droop_pu = (Vset - pred_state.V_filter) / Rq;

						if (Qref_droop_pu > Qref_max)
						{
							Qref_droop_pu = Qref_max;
						}

						if (Qref_droop_pu < Qref_min)
						{
							Qref_droop_pu = Qref_min;
						}

						double power_diff_val = Qref_droop_pu_prev - Qref_droop_pu;

						if(checkRampRate_reactive == true)
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
							pred_state.dQref_droop_pu_filter = 1.0 / Tqf * (Qref_droop_pu - curr_state.Qref_droop_pu_filter);
							pred_state.Qref_droop_pu_filter = curr_state.Qref_droop_pu_filter + (deltat * pred_state.dQref_droop_pu_filter);
						}
					}
					// V_Avg_pu is the average value of three phase voltages
					// Tvf is the time constant of voltage low pass filter
					// V_filter is the voltage pass through low pass filter
					// Qref_droop_pu if the Q reference from volt-var
					// Qref_droop_pu_filter is the Q pass through low pass filter
					// Qref_max and Qref_min are the upper and lower limits of Q references
					// Function end

					// Function: Current Control Loop
					for (i = 0; i < 3; i++)
					{
						if (frequency_watt)
						{
							if(grid_following_mode == BALANCED_POWER)
							{
								if (checkRampRate_real == true)
								{
									igd_ref[i] = Pref_droop_pu / ugd_pu[i];
								}
								else
								{
									igd_ref[i] = pred_state.Pref_droop_pu_filter / ugd_pu[i];
								}
							}
							else if(grid_following_mode == POSITIVE_SEQUENCE)
							{
								if (checkRampRate_real == true)
								{
									igd_ref[i] = Pref_droop_pu / ugd_pu_PS;
								}
								else
								{
									igd_ref[i] = pred_state.Pref_droop_pu_filter / ugd_pu_PS;
								}
							}

						}
						else
						{
							if(grid_following_mode == BALANCED_POWER)
							{
								// get the current references
								igd_ref[i] = Pref / S_base / ugd_pu[i];
								//igd_ref[i] = Pref/S_base/1.0;
							}
							else if(grid_following_mode == POSITIVE_SEQUENCE)
							{
								// get the current references
								igd_ref[i] = Pref / S_base / ugd_pu_PS;
							}
						}

						if (volt_var)
						{
							if(grid_following_mode == BALANCED_POWER)
							{
								if(checkRampRate_reactive == true)
								{
									igq_ref[i] = -Qref_droop_pu / ugd_pu[i];
								}
								else
								{
									igq_ref[i] = -pred_state.Qref_droop_pu_filter / ugd_pu[i];
								}
							}
							else if(grid_following_mode == POSITIVE_SEQUENCE)
							{
								if(checkRampRate_reactive == true)
								{
									igq_ref[i] = -Qref_droop_pu / ugd_pu_PS;
								}
								else
								{
									igq_ref[i] = -pred_state.Qref_droop_pu_filter / ugd_pu_PS;
								}
							}
						}
						else
						{
							if(grid_following_mode == BALANCED_POWER)
							{
								igq_ref[i] = -Qref / S_base / ugd_pu[i];
								//igq_ref[i] = Qref/S_base/1.0;
							}
							else if(grid_following_mode == POSITIVE_SEQUENCE)
							{
								igq_ref[i] = -Qref / S_base / ugd_pu_PS;
							}
						}

						if (control_mode == GRID_FOLLOWING)
						{
							// current loop in d axis
							pred_state.digd_PI_ini[i] = (igd_ref[i] - igd_pu[i]) * kic;
							pred_state.igd_PI_ini[i] = curr_state.igd_PI_ini[i] + pred_state.digd_PI_ini[i] * deltat; //output from the integrator term of the current control loop
							igd_PI[i] = pred_state.igd_PI_ini[i] + pred_state.digd_PI_ini[i] / kic * kpc;			  // output from the PI controller of current loop
							ed_pu[i] = igd_PI[i] + ugd_pu[i] - igq_pu[i] * Xfilter * F_current;						  // the d axis component of internal voltage, Xfilter is per-unit value
							// current loop in q axis
							pred_state.digq_PI_ini[i] = (igq_ref[i] - igq_pu[i]) * kic;
							pred_state.igq_PI_ini[i] = curr_state.igq_PI_ini[i] + pred_state.digq_PI_ini[i] * deltat; //output from the integrator term of the current control loop
							igq_PI[i] = pred_state.igq_PI_ini[i] + pred_state.digq_PI_ini[i] / kic * kpc;			  // output from the PI controller of current loop
							eq_pu[i] = igq_PI[i] + ugq_pu[i] + igd_pu[i] * Xfilter * F_current;						  // the d axis component of internal voltage, Xfilter is per-unit value
							// igd_PI_ini[i] and igq_PI_ini[i] are outputs from the integral terms in current control loops
							// igd_ref[i] and igq_ref[i] are the current references in dq frame
							// igd_PI[i] and igq_PI[i] are outputs from the current control loops
							// ed_pu[i] and eq_pu[i] are the dq components of the internal voltages
							// kpc and kic are the PI gains of the current loop
							// Function end


							// Function: Coordinate Transformation: dq to xy
							e_source_Re[i] = (ed_pu[i] * cos(pred_state.Angle_PLL[i]) - eq_pu[i] * sin(pred_state.Angle_PLL[i])) * V_base;
							e_source_Im[i] = (ed_pu[i] * sin(pred_state.Angle_PLL[i]) + eq_pu[i] * cos(pred_state.Angle_PLL[i])) * V_base;
							e_source[i] = complex(e_source_Re[i], e_source_Im[i]);
							value_IGenerated[i] = e_source[i] / (complex(Rfilter, Xfilter) * Z_base); // Thevenin voltage source to Norton current source convertion
							// e_source[i] is the complex value of internal voltage
							// value_IGenerated[i] is the Norton equivalent current source of e_source[i]
							// Rfilter and Xfilter are the per-unit values of inverter filter
							// Function end

						}
						else if (control_mode == GFL_CURRENT_SOURCE)
						{
							// Low pass filter for current id
							pred_state.digd_filter[i] = 1.0 / Tif * (igd_ref[i] - curr_state.igd_filter[i]);
							pred_state.igd_filter[i] = curr_state.igd_filter[i] + (deltat * pred_state.digd_filter[i]);

							// Low pass filter for current iq
							pred_state.digq_filter[i] = 1.0 / Tif * (igq_ref[i] - curr_state.igq_filter[i]);
							pred_state.igq_filter[i] = curr_state.igq_filter[i] + (deltat * pred_state.digq_filter[i]);
							// igd_ref[i] and igq_ref[i] are the current references in dq frame
							// igd_filter[i] and igq_filter[i] are the currents
							// Function end

							// Function: Coordinate Transformation: dq to xy
							I_source_Re[i] = (pred_state.igd_filter[i] * cos(pred_state.Angle_PLL[i]) - pred_state.igq_filter[i] * sin(pred_state.Angle_PLL[i])) * I_base;
							I_source_Im[i] = (pred_state.igd_filter[i] * sin(pred_state.Angle_PLL[i]) + pred_state.igq_filter[i] * cos(pred_state.Angle_PLL[i])) * I_base;
							I_source[i] = complex(I_source_Re[i], I_source_Im[i]);
							value_IGenerated[i] = I_source[i];
							// I_source[i] is the complex value of injected current
							// value_IGenerated[i] is the current injected to the grid
							// Function end
						}
					}

					simmode_return_value = SM_DELTA_ITER; //Reiterate - to get us to corrector pass

				}	 // end of grid-following
			}	  // end of three phase code of grid-following
		}
		else if (iteration_count_val == 1) // Corrector pass
		{
			//Caluclate injection current based on voltage soruce magtinude and angle obtained
			if (parent_is_single_phase == true) // single phase/split-phase implementation
			{
				//Update output power
				//Get current injected
				temp_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

				//Update power output variables, just so we can see what is going on
				VA_Out = value_Circuit_V[0] * ~temp_current_val[0];

				// Function: Coordinate Tranformation, xy to dq
				ugd_pu[0] = (value_Circuit_V[0].Re() * cos(pred_state.Angle_PLL[0]) + value_Circuit_V[0].Im() * sin(pred_state.Angle_PLL[0])) / V_base;
				ugq_pu[0] = (-value_Circuit_V[0].Re() * sin(pred_state.Angle_PLL[0]) + value_Circuit_V[0].Im() * cos(pred_state.Angle_PLL[0])) / V_base;
				igd_pu[0] = (temp_current_val[0].Re() * cos(pred_state.Angle_PLL[0]) + temp_current_val[0].Im() * sin(pred_state.Angle_PLL[0])) / I_base;
				igq_pu[0] = (-temp_current_val[0].Re() * sin(pred_state.Angle_PLL[0]) + temp_current_val[0].Im() * cos(pred_state.Angle_PLL[0])) / I_base;

				// ugd_pu[i] and ugq_pu[i] are the per-unit values of grid-side voltages in dq frame
				// igd_pu[i] and igq_pu[i] are the per-unit values of grid-side currents in dq frame
				// Angle_PLL[i] is the phase angle of the grid side votlage, which is obtained from the PLL
				// Value_Circuit_V[i] is the voltage of each phase at the grid side
				// temp_current_val[i] is the current of each phase injected to the grid
				// S_base is the rated capacity
				// I_base is the rated current
				// Function end

				// Function: Phase-Lock_Loop, PLL
				next_state.ddelta_w_PLL_ini[0] = ugq_pu[0] * kiPLL;
				next_state.delta_w_PLL_ini[0] = curr_state.delta_w_PLL_ini[0] + (pred_state.ddelta_w_PLL_ini[0] + next_state.ddelta_w_PLL_ini[0]) * deltat / 2.0; //output from the integrator term
				next_state.delta_w_PLL[0] = next_state.delta_w_PLL_ini[0] + next_state.ddelta_w_PLL_ini[0] / kiPLL * kpPLL;										  // output from the PI controller
				fPLL[0] = (next_state.delta_w_PLL[0] + w_ref) / 2.0 / PI;																							  // frequency measured by PLL
				next_state.Angle_PLL[0] = curr_state.Angle_PLL[0] + (pred_state.delta_w_PLL[0] + next_state.delta_w_PLL[0]) * deltat / 2.0;						  // sphase angle from PLL

				// delta_w_PLL_ini[i] is the output from the integrator term
				// delta_w_PLL[i] is the output from the PI controller
				// w_ref is the rated angular frequency, the value is 376.99 rad/s
				// fPLL is the frequency measured by PLL
				// Fuction end

				// Frequency-watt function enabled
				if (frequency_watt)
				{
					next_state.df_filter = 1.0 / Tff * (fPLL[0] - pred_state.f_filter);
					next_state.f_filter = curr_state.f_filter + (pred_state.df_filter + next_state.df_filter) * deltat / 2.0;

					Pref_droop_pu = (f_nominal - next_state.f_filter) / f_nominal / Rp + Pref / S_base;

					if (Pref_droop_pu > Pref_max)
					{
						Pref_droop_pu = Pref_max;
					}

					if (Pref_droop_pu < Pref_min)
					{
						Pref_droop_pu = Pref_min;
					}

					double power_diff_val = Pref_droop_pu_prev - Pref_droop_pu;

					if (checkRampRate_real == true)
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
						next_state.dPref_droop_pu_filter = 1.0 / Tpf * (Pref_droop_pu - pred_state.Pref_droop_pu_filter);
						next_state.Pref_droop_pu_filter = curr_state.Pref_droop_pu_filter + (pred_state.dPref_droop_pu_filter + next_state.dPref_droop_pu_filter) * deltat / 2.0;
					}

				}
				// f_filter is the frequency pass through the low pass filter
				// Tff is the time constant in the frequency low pass filter
				// Pref_droop_pu is the power reference from the frequency-watt
				// Pref_droop_pu_filter is the power reference pass through the low pass filter
				// Tpf is the time constant in the power low pass filter
				// Pref_max and Pref_min are the upper and lower limits of power references
				// Function end

				// Volt-var function enabled
				if (volt_var)
				{
					V_Avg_pu = value_Circuit_V[0].Mag() / V_base;
					next_state.dV_filter = 1.0 / Tvf * (V_Avg_pu - pred_state.V_filter);
					next_state.V_filter = curr_state.V_filter + (pred_state.dV_filter + next_state.dV_filter) * deltat / 2.0;

					Qref_droop_pu = (Vset - next_state.V_filter) / Rq;

					if (Qref_droop_pu > Qref_max)
					{
						Qref_droop_pu = Qref_max;
					}

					if (Qref_droop_pu < Qref_min)
					{
						Qref_droop_pu = Qref_min;
					}

					double power_diff_val = Qref_droop_pu_prev - Qref_droop_pu;
					if(checkRampRate_reactive == true)
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
						next_state.dQref_droop_pu_filter = 1.0 / Tqf * (Qref_droop_pu - pred_state.Qref_droop_pu_filter);
						next_state.Qref_droop_pu_filter = curr_state.Qref_droop_pu_filter + (pred_state.dQref_droop_pu_filter + next_state.dQref_droop_pu_filter) * deltat / 2.0;
					}
				}
				// V_Avg_pu is the average value of three phase voltages
				// Tvf is the time constant of voltage low pass filter
				// V_filter is the voltage pass through low pass filter
				// Qref_droop_pu if the Q reference from volt-var
				// Qref_droop_pu_filter is the Q pass through low pass filter
				// Qref_max and Qref_min are the upper and lower limits of Q references
				// Function end

				// Function: Current Control Loop
				if (frequency_watt)
				{
					if (checkRampRate_real == true)
					{
						igd_ref[0] = Pref_droop_pu / ugd_pu[0];
					}
					else
					{
						igd_ref[0] = next_state.Pref_droop_pu_filter / ugd_pu[0];
					}
				}
				else
				{
					// get the current references
					igd_ref[0] = Pref / S_base / ugd_pu[0];
				}

				if (volt_var)
				{
					if(checkRampRate_reactive == true)
					{
						igq_ref[0] = -Qref_droop_pu / ugd_pu[0];
					}
					else
					{
						igq_ref[0] = -next_state.Qref_droop_pu_filter / ugd_pu[0];
					}
				}
				else
				{
					igq_ref[0] = -Qref / S_base / ugd_pu[0];
				}

				if(control_mode == GRID_FOLLOWING)
				{
					// current loop in d axis
					next_state.digd_PI_ini[0] = (igd_ref[0] - igd_pu[0]) * kic;
					next_state.igd_PI_ini[0] = curr_state.igd_PI_ini[0] + (pred_state.digd_PI_ini[0] + next_state.digd_PI_ini[0]) * deltat / 2.0; //output from the integrator term of the current control loop
					igd_PI[0] = next_state.igd_PI_ini[0] + next_state.digd_PI_ini[0] / kic * kpc;												  // output from the PI controller of current loop
					ed_pu[0] = igd_PI[0] + ugd_pu[0] - igq_pu[0] * Xfilter * F_current;															  // the d axis component of internal voltage, Xfilter is per-unit value
					// current loop in q axis
					next_state.digq_PI_ini[0] = (igq_ref[0] - igq_pu[0]) * kic;
					next_state.igq_PI_ini[0] = curr_state.igq_PI_ini[0] + (pred_state.digq_PI_ini[0] + next_state.digq_PI_ini[0]) * deltat / 2.0; //output from the integrator term of the current control loop
					igq_PI[0] = next_state.igq_PI_ini[0] + next_state.digq_PI_ini[0] / kic * kpc;												  // output from the PI controller of current loop
					eq_pu[0] = igq_PI[0] + ugq_pu[0] + igd_pu[0] * Xfilter * F_current;															  // the d axis component of internal voltage, Xfilter is per-unit value

					// igd_PI_ini[i] and igq_PI_ini[i] are outputs from the integral terms in current control loops
					// igd_ref[i] and igq_ref[i] are the current references in dq frame
					// igd_PI[i] and igq_PI[i] are outputs from the current control loops
					// ed_pu[i] and eq_pu[i] are the dq components of the internal voltages
					// kpc and kic are the PI gains of the current loop
					// Function end

					// Function: Coordinate Transformation: dq to xy
					e_source_Re[0] = (ed_pu[0] * cos(next_state.Angle_PLL[0]) - eq_pu[0] * sin(next_state.Angle_PLL[0])) * V_base;
					e_source_Im[0] = (ed_pu[0] * sin(next_state.Angle_PLL[0]) + eq_pu[0] * cos(next_state.Angle_PLL[0])) * V_base;
					e_source[0] = complex(e_source_Re[0], e_source_Im[0]);
					value_IGenerated[0] = e_source[0] / (complex(Rfilter, Xfilter) * Z_base); // Thevenin voltage source to Norton current source convertion

					// e_source[i] is the complex value of internal voltage
					// value_IGenerated[i] is the Norton equivalent current source of e_source[i]
					// Rfilter and Xfilter are the per-unit values of inverter filter
					// Function end
				}
				else if (control_mode == GFL_CURRENT_SOURCE)
				{
					// Low pass filter for current id
					next_state.digd_filter[0] = 1.0 / Tif * (igd_ref[0] - pred_state.igd_filter[0]);
					next_state.igd_filter[0] = curr_state.igd_filter[0] + (pred_state.digd_filter[0] + next_state.digd_filter[0]) * deltat / 2.0;

					// Low pass filter for current id
					next_state.digq_filter[0] = 1.0 / Tif * (igq_ref[0] - pred_state.igq_filter[0]);
					next_state.igq_filter[0] = curr_state.igq_filter[0] + (pred_state.digq_filter[0] + next_state.digq_filter[0]) * deltat / 2.0;
					// igd_ref[0] and igq_ref[0] are the current references in dq frame
					// igd_filter[0] and igq_filter[0] are the currents

					I_source_Re[0] = (next_state.igd_filter[0] * cos(next_state.Angle_PLL[0]) - next_state.igq_filter[0] * sin(next_state.Angle_PLL[0])) * I_base;
					I_source_Im[0] = (next_state.igd_filter[0] * sin(next_state.Angle_PLL[0]) + next_state.igq_filter[0] * cos(next_state.Angle_PLL[0])) * I_base;
					I_source[0] = complex(I_source_Re[0], I_source_Im[0]);
					value_IGenerated[0] = I_source[0];
					// I_source[0] is the complex value of injected current
					// value_IGenerated[0] is the current injected to the grid
					// Function end
				}

				memcpy(&curr_state, &next_state, sizeof(INV_DYN_STATE));

				simmode_return_value = SM_DELTA;
			}
			else //Three-phase
			{
				if ((grid_following_mode == BALANCED_POWER)||(grid_following_mode == POSITIVE_SEQUENCE))
				{
					//Update output power
					//Get current injected
					temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
					temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
					temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

					//Update power output variables, just so we can see what is going on
					power_val[0] = value_Circuit_V[0] * ~temp_current_val[0];
					power_val[1] = value_Circuit_V[1] * ~temp_current_val[1];
					power_val[2] = value_Circuit_V[2] * ~temp_current_val[2];

					VA_Out = power_val[0] + power_val[1] + power_val[2];

					// Function: Coordinate Tranformation, xy to dq
					for (i = 0; i < 3; i++)
					{
						ugd_pu[i] = (value_Circuit_V[i].Re() * cos(pred_state.Angle_PLL[i]) + value_Circuit_V[i].Im() * sin(pred_state.Angle_PLL[i])) / V_base;
						ugq_pu[i] = (-value_Circuit_V[i].Re() * sin(pred_state.Angle_PLL[i]) + value_Circuit_V[i].Im() * cos(pred_state.Angle_PLL[i])) / V_base;
						igd_pu[i] = (temp_current_val[i].Re() * cos(pred_state.Angle_PLL[i]) + temp_current_val[i].Im() * sin(pred_state.Angle_PLL[i])) / I_base;
						igq_pu[i] = (-temp_current_val[i].Re() * sin(pred_state.Angle_PLL[i]) + temp_current_val[i].Im() * cos(pred_state.Angle_PLL[i])) / I_base;
					}
					// ugd_pu[i] and ugq_pu[i] are the per-unit values of grid-side voltages in dq frame
					// igd_pu[i] and igq_pu[i] are the per-unit values of grid-side currents in dq frame
					// Angle_PLL[i] is the phase angle of the grid side votlage, which is obtained from the PLL
					// Value_Circuit_V[i] is the voltage of each phase at the grid side
					// temp_current_val[i] is the current of each phase injected to the grid
					// S_base is the rated capacity
					// I_base is the rated current
					// Function end

					if(grid_following_mode == BALANCED_POWER)
					{
						// Function: Phase-Lock_Loop, PLL
						for (i = 0; i < 3; i++)
						{
							next_state.ddelta_w_PLL_ini[i] = ugq_pu[i] * kiPLL;
							next_state.delta_w_PLL_ini[i] = curr_state.delta_w_PLL_ini[i] + (pred_state.ddelta_w_PLL_ini[i] + next_state.ddelta_w_PLL_ini[i]) * deltat / 2.0; //output from the integrator term
							next_state.delta_w_PLL[i] = next_state.delta_w_PLL_ini[i] + next_state.ddelta_w_PLL_ini[i] / kiPLL * kpPLL;										  // output from the PI controller
							fPLL[i] = (next_state.delta_w_PLL[i] + w_ref) / 2.0 / PI;																							  // frequency measured by PLL
							next_state.Angle_PLL[i] = curr_state.Angle_PLL[i] + (pred_state.delta_w_PLL[i] + next_state.delta_w_PLL[i]) * deltat / 2.0;						  // sphase angle from PLL
						}
						// delta_w_PLL_ini[i] is the output from the integrator term
						// delta_w_PLL[i] is the output from the PI controller
						// w_ref is the rated angular frequency, the value is 376.99 rad/s
						// fPLL is the frequency measured by PLL
						// Fuction end
					}
					else if(grid_following_mode == POSITIVE_SEQUENCE)
					{
						// Obtain the positive sequence voltage
						value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;

						// Positive sequence value of voltage in dq frame
						ugd_pu_PS = (value_Circuit_V_PS.Re() * cos(pred_state.Angle_PLL[0]) + value_Circuit_V_PS.Im() * sin(pred_state.Angle_PLL[0])) / V_base;
						ugq_pu_PS = (-value_Circuit_V_PS.Re() * sin(pred_state.Angle_PLL[0]) + value_Circuit_V_PS.Im() * cos(pred_state.Angle_PLL[0])) / V_base;

						// Function: Phase-Lock_Loop, PLL, only consider the positive sequence voltage
						for (i = 0; i < 1; i++)
						{
							next_state.ddelta_w_PLL_ini[i] = ugq_pu_PS * kiPLL;
							next_state.delta_w_PLL_ini[i] = curr_state.delta_w_PLL_ini[i] + (pred_state.ddelta_w_PLL_ini[i] + next_state.ddelta_w_PLL_ini[i]) * deltat / 2.0; //output from the integrator term
							next_state.delta_w_PLL[i] = next_state.delta_w_PLL_ini[i] + next_state.ddelta_w_PLL_ini[i] / kiPLL * kpPLL;										  // output from the PI controller
							fPLL[i] = (next_state.delta_w_PLL[i] + w_ref) / 2.0 / PI;																							  // frequency measured by PLL
							next_state.Angle_PLL[i] = curr_state.Angle_PLL[i] + (pred_state.delta_w_PLL[i] + next_state.delta_w_PLL[i]) * deltat / 2.0;						  // sphase angle from PLL
						}
						next_state.Angle_PLL[1] = next_state.Angle_PLL[0] - 2.0 / 3.0 * PI;
						next_state.Angle_PLL[2] = next_state.Angle_PLL[0] + 2.0 / 3.0 * PI;
						fPLL[2] = fPLL[1] = fPLL[0];
						// delta_w_PLL_ini[i] is the output from the integrator term
						// delta_w_PLL[i] is the output from the PI controller
						// w_ref is the rated angular frequency, the value is 376.99 rad/s
						// fPLL is the frequency measured by PLL
						// Fuction end
					}

					// Frequency-watt function enabled
					if (frequency_watt)
					{
						next_state.df_filter = 1.0 / Tff * ((fPLL[0] + fPLL[1] + fPLL[2]) / 3.0 - pred_state.f_filter);
						next_state.f_filter = curr_state.f_filter + (pred_state.df_filter + next_state.df_filter) * deltat / 2.0;

						Pref_droop_pu = (f_nominal - next_state.f_filter) / f_nominal / Rp + Pref / S_base;

						if (Pref_droop_pu > Pref_max)
						{
							Pref_droop_pu = Pref_max;
						}

						if (Pref_droop_pu < Pref_min)
						{
							Pref_droop_pu = Pref_min;
						}

						double power_diff_val = Pref_droop_pu_prev - Pref_droop_pu;

						if (checkRampRate_real == true)
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
							next_state.dPref_droop_pu_filter = 1.0 / Tpf * (Pref_droop_pu - pred_state.Pref_droop_pu_filter);
							next_state.Pref_droop_pu_filter = curr_state.Pref_droop_pu_filter + (pred_state.dPref_droop_pu_filter + next_state.dPref_droop_pu_filter) * deltat / 2.0;
						}
					}
					// f_filter is the frequency pass through the low pass filter
					// Tff is the time constant in the frequency low pass filter
					// Pref_droop_pu is the power reference from the frequency-watt
					// Pref_droop_pu_filter is the power reference pass through the low pass filter
					// Tpf is the time constant in the power low pass filter
					// Pref_max and Pref_min are the upper and lower limits of power references
					// Function end

					// Volt-var function enabled
					if (volt_var)
					{
						V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;
						next_state.dV_filter = 1.0 / Tvf * (V_Avg_pu - pred_state.V_filter);
						next_state.V_filter = curr_state.V_filter + (pred_state.dV_filter + next_state.dV_filter) * deltat / 2.0;

						Qref_droop_pu = (Vset - next_state.V_filter) / Rq;

						if (Qref_droop_pu > Qref_max)
						{
							Qref_droop_pu = Qref_max;
						}

						if (Qref_droop_pu < Qref_min)
						{
							Qref_droop_pu = Qref_min;
						}

						double power_diff_val = Qref_droop_pu_prev - Qref_droop_pu;

						if(checkRampRate_reactive == true)
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
							next_state.dQref_droop_pu_filter = 1.0 / Tqf * (Qref_droop_pu - pred_state.Qref_droop_pu_filter);
							next_state.Qref_droop_pu_filter = curr_state.Qref_droop_pu_filter + (pred_state.dQref_droop_pu_filter + next_state.dQref_droop_pu_filter) * deltat / 2.0;
						}

					}
					// V_Avg_pu is the average value of three phase voltages
					// Tvf is the time constant of voltage low pass filter
					// V_filter is the voltage pass through low pass filter
					// Qref_droop_pu if the Q reference from volt-var
					// Qref_droop_pu_filter is the Q pass through low pass filter
					// Qref_max and Qref_min are the upper and lower limits of Q references
					// Function end

					// Function: Current Control Loop
					for (i = 0; i < 3; i++)
					{
						if (frequency_watt)
						{
							if(grid_following_mode == BALANCED_POWER)
							{
								if (checkRampRate_real == true)
								{
									igd_ref[i] = Pref_droop_pu / ugd_pu[i];
								}
								else
								{
									igd_ref[i] = next_state.Pref_droop_pu_filter / ugd_pu[i];
								}
							}
							else if(grid_following_mode == POSITIVE_SEQUENCE)
							{
								if (checkRampRate_real == true)
								{
									igd_ref[i] = Pref_droop_pu / ugd_pu_PS;
								}
								else
								{
									igd_ref[i] = next_state.Pref_droop_pu_filter / ugd_pu_PS;
								}
							}
						}
						else
						{
							if(grid_following_mode == BALANCED_POWER)
							{
								// get the current references
								igd_ref[i] = Pref / S_base / ugd_pu[i];
								//igd_ref[i] = Pref/S_base/1.0;
							}
							else if(grid_following_mode == POSITIVE_SEQUENCE)
							{
								// get the current references
								igd_ref[i] = Pref / S_base / ugd_pu_PS;
							}
						}

						if (volt_var)
						{
							if(grid_following_mode == BALANCED_POWER)
							{
								if(checkRampRate_reactive == true)
								{
									igq_ref[i] = -Qref_droop_pu / ugd_pu[i];
								}
								else
								{
									igq_ref[i] = -next_state.Qref_droop_pu_filter / ugd_pu[i];
								}
							}
							else if(grid_following_mode == POSITIVE_SEQUENCE)
							{
								if(checkRampRate_reactive == true)
								{
									igq_ref[i] = -Qref_droop_pu / ugd_pu_PS;
								}
								else
								{
									igq_ref[i] = -next_state.Qref_droop_pu_filter / ugd_pu_PS;
								}
							}
						}
						else
						{
							if(grid_following_mode == BALANCED_POWER)
							{
								igq_ref[i] = -Qref / S_base / ugd_pu[i];
								//igq_ref[i] = Qref/S_base/1.0;
							}
							else if(grid_following_mode == POSITIVE_SEQUENCE)
							{
								igq_ref[i] = -Qref / S_base / ugd_pu_PS;
							}
						}

						if(control_mode == GRID_FOLLOWING)
						{
							// current loop in d axis
							next_state.digd_PI_ini[i] = (igd_ref[i] - igd_pu[i]) * kic;
							next_state.igd_PI_ini[i] = curr_state.igd_PI_ini[i] + (pred_state.digd_PI_ini[i] + next_state.digd_PI_ini[i]) * deltat / 2.0; //output from the integrator term of the current control loop
							igd_PI[i] = next_state.igd_PI_ini[i] + next_state.digd_PI_ini[i] / kic * kpc;												  // output from the PI controller of current loop
							ed_pu[i] = igd_PI[i] + ugd_pu[i] - igq_pu[i] * Xfilter * F_current;															  // the d axis component of internal voltage, Xfilter is per-unit value
							// current loop in q axis
							next_state.digq_PI_ini[i] = (igq_ref[i] - igq_pu[i]) * kic;
							next_state.igq_PI_ini[i] = curr_state.igq_PI_ini[i] + (pred_state.digq_PI_ini[i] + next_state.digq_PI_ini[i]) * deltat / 2.0; //output from the integrator term of the current control loop
							igq_PI[i] = next_state.igq_PI_ini[i] + next_state.digq_PI_ini[i] / kic * kpc;												  // output from the PI controller of current loop
							eq_pu[i] = igq_PI[i] + ugq_pu[i] + igd_pu[i] * Xfilter * F_current;															  // the d axis component of internal voltage, Xfilter is per-unit value

							// igd_PI_ini[i] and igq_PI_ini[i] are outputs from the integral terms in current control loops
							// igd_ref[i] and igq_ref[i] are the current references in dq frame
							// igd_PI[i] and igq_PI[i] are outputs from the current control loops
							// ed_pu[i] and eq_pu[i] are the dq components of the internal voltages
							// kpc and kic are the PI gains of the current loop
							// Function end

							// Function: Coordinate Transformation: dq to xy
							e_source_Re[i] = (ed_pu[i] * cos(next_state.Angle_PLL[i]) - eq_pu[i] * sin(next_state.Angle_PLL[i])) * V_base;
							e_source_Im[i] = (ed_pu[i] * sin(next_state.Angle_PLL[i]) + eq_pu[i] * cos(next_state.Angle_PLL[i])) * V_base;
							e_source[i] = complex(e_source_Re[i], e_source_Im[i]);
							value_IGenerated[i] = e_source[i] / (complex(Rfilter, Xfilter) * Z_base); // Thevenin voltage source to Norton current source convertion

							// e_source[i] is the complex value of internal voltage
							// value_IGenerated[i] is the Norton equivalent current source of e_source[i]
							// Rfilter and Xfilter are the per-unit values of inverter filter
							// Function end
						}
						else if(control_mode == GFL_CURRENT_SOURCE)
						{
							// Low pass filter for current id
							next_state.digd_filter[i] = 1.0 / Tif * (igd_ref[i] - pred_state.igd_filter[i]);
							next_state.igd_filter[i] = curr_state.igd_filter[i] + (pred_state.digd_filter[i] + next_state.digd_filter[i]) * deltat / 2.0;

							// Low pass filter for current id
							next_state.digq_filter[i] = 1.0 / Tif * (igq_ref[i] - pred_state.igq_filter[i]);
							next_state.igq_filter[i] = curr_state.igq_filter[i] + (pred_state.digq_filter[i] + next_state.digq_filter[i]) * deltat / 2.0;

							// igd_ref[i] and igq_ref[i] are the current references in dq frame
							// igd_filter[i] and igq_filter[i] are the currents
							// Function end

							// Function: Coordinate Transformation: dq to xy
							I_source_Re[i] = (next_state.igd_filter[i] * cos(next_state.Angle_PLL[i]) - next_state.igq_filter[i] * sin(next_state.Angle_PLL[i])) * I_base;
							I_source_Im[i] = (next_state.igd_filter[i] * sin(next_state.Angle_PLL[i]) + next_state.igq_filter[i] * cos(next_state.Angle_PLL[i])) * I_base;
							I_source[i] = complex(I_source_Re[i], I_source_Im[i]);
							value_IGenerated[i] = I_source[i];
							// I_source[i] is the complex value of injected current
							// value_IGenerated[i] is the current injected to the grid
							// Function end
						}
					}

					memcpy(&curr_state, &next_state, sizeof(INV_DYN_STATE));

					simmode_return_value = SM_DELTA;

				}	 // end of grid-following
			}
		}	 // end of three phase grid-following, corrector pass
		else //Additional iterations
		{
			//Just return whatever our "last desired" was
			simmode_return_value = desired_simulation_mode;
		}
	} // end of grid-following

	//Sync the powerflow variables
	if (parent_is_a_meter == true)
	{
		push_complex_powerflow_values(false);
	}

	//Set the mode tracking variable for this exit
	desired_simulation_mode = simmode_return_value;

	return simmode_return_value;
}

//Initializes dynamic equations for first entry
//Returns a SUCCESS/FAIL
//curr_time is the initial states/information
STATUS inverter_dyn::init_dynamics(INV_DYN_STATE *curr_time)
{
	OBJECT *obj = OBJECTHDR(this);

	//Pull the powerflow values
	if (parent_is_a_meter == true)
	{
		reset_complex_powerflow_accumulators();

		pull_complex_powerflow_values();
	}

	//Set the mode tracking variable to a default - not really needed, but be paranoid
	desired_simulation_mode = SM_EVENT;

	if (control_mode == GRID_FORMING)
	{
		// for simpicity, here we only consider three-phase system
		if ((phases & 0x07) == 0x07)
		{

			//Update output power
			//Get current injected to the grid, value_IGenerated is obtained from power flow calculation
			temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
			temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
			temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

			//Update power output variables, just so we can see what is going on
			power_val[0] = value_Circuit_V[0] * ~temp_current_val[0];
			power_val[1] = value_Circuit_V[1] * ~temp_current_val[1];
			power_val[2] = value_Circuit_V[2] * ~temp_current_val[2];

			VA_Out = power_val[0] + power_val[1] + power_val[2];

			//Compute the average value of three-phase terminal voltages
			pCircuit_V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;

			for (int i = 0; i < 3; i++)
			{

				// Initialize the state variables of the internal voltages
				e_source[i] = (value_IGenerated[i] * complex(Rfilter, Xfilter) * Z_base);
				e_source_prev[i] = e_source[i];
				curr_time->Angle[i] = (e_source[i]).Arg(); // Obtain the inverter internal voltage phasor angle
			}

			// Initializa the internal voltage magnitudes
			curr_time->V_ini = (e_source[0].Mag() + e_source[1].Mag() + e_source[2].Mag()) / 3 / V_base;

			//See if it is the first deltamode entry - theory is all future changes will trigger deltamode, so these should be set
			if (first_deltamode_init == true)
			{
				//Make sure it wasn't "pre-set"
				if (Vset < 0.0)	//-99 is the flag
				{
					// Initialize Vset and Pset
					Vset = pCircuit_V_Avg_pu + VA_Out.Im() / S_base * mq;
				}
				else
				{
					//Use the set value, but also bias off the droop
					Vset += VA_Out.Im() / S_base * mq;
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

			// Initialize measured P,Q,and V
			curr_time->p_measure = VA_Out.Re() / S_base;
			curr_time->q_measure = VA_Out.Im() / S_base;
			curr_time->v_measure = pCircuit_V_Avg_pu;

			// Initialize Pmax and Pmin controller
			curr_time->delta_w_Pmax_ini = 0;
			curr_time->delta_w_Pmin_ini = 0;

			// Initialize Vdc_min controller and DC bus voltage
			if (grid_forming_mode == DYNAMIC_DC_BUS) // consider the dynamics of PV dc bus, and the internal voltage magnitude needs to be recalculated
			{
				//See if there are any DC objects to handle
				if (dc_interface_objects.empty() != true)
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
							GL_THROW("inverter_dyn:%d - %s - DC object update for object:%d - %s - failed!", obj->id, (obj->name ? obj->name : "Unnamed"), temp_obj->id, (temp_obj->name ? temp_obj->name : "Unnamed"));
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
	}
	else if ((control_mode == GRID_FOLLOWING) || (control_mode == GFL_CURRENT_SOURCE))
	{

		if (parent_is_single_phase == true) // single phase/split-phase implementation
		{
			//Update output power
			//Get current injected to the grid, value_IGenerated is obtained from power flow calculation
			temp_current_val[0] = value_IGenerated[0] - filter_admittance * value_Circuit_V[0];

			VA_Out = value_Circuit_V[0] * ~temp_current_val[0];

			//See if it is the first deltamode entry - theory is all future changes will trigger deltamode, so these should be set
			if (first_deltamode_init == true)
			{
				//See if we set it to something first
				if (Vset < 0.0)	//-99.0 flag value
				{
					Vset = value_Circuit_V[0].Mag() / V_base + Qref / S_base * Rq;
				}
				else
				{
					//Use Vset, but bias for the Qref values
					Vset += Qref / S_base * Rq;
				}

				//Set it false in here, for giggles
				first_deltamode_init = false;
			}
			//Default else - all changes should be in deltamode

			// Initialize the PLL
			curr_time->Angle_PLL[0] = value_Circuit_V[0].Arg();
			curr_time->delta_w_PLL_ini[0] = 0;

			ugd_pu[0] = (value_Circuit_V[0].Re() * cos(value_Circuit_V[0].Arg()) + value_Circuit_V[0].Im() * sin(value_Circuit_V[0].Arg())) / V_base;
			ugq_pu[0] = (-value_Circuit_V[0].Re() * sin(value_Circuit_V[0].Arg()) + value_Circuit_V[0].Im() * cos(value_Circuit_V[0].Arg())) / V_base;
			igd_pu[0] = (temp_current_val[0].Re() * cos(value_Circuit_V[0].Arg()) + temp_current_val[0].Im() * sin(value_Circuit_V[0].Arg())) / I_base;
			igq_pu[0] = (-temp_current_val[0].Re() * sin(value_Circuit_V[0].Arg()) + temp_current_val[0].Im() * cos(value_Circuit_V[0].Arg())) / I_base;

			if(control_mode == GRID_FOLLOWING)
			{
				// Initialize the current control loops
				e_source[0] = (value_IGenerated[0] * complex(Rfilter, Xfilter) * Z_base);
				ed_pu[0] = (e_source[0].Re() * cos(value_Circuit_V[0].Arg()) + e_source[0].Im() * sin(value_Circuit_V[0].Arg())) / V_base;
				eq_pu[0] = (-e_source[0].Re() * sin(value_Circuit_V[0].Arg()) + e_source[0].Im() * cos(value_Circuit_V[0].Arg())) / V_base;

				curr_time->igd_PI_ini[0] = ed_pu[0] - ugd_pu[0] + igq_pu[0] * Xfilter * F_current;
				curr_time->igq_PI_ini[0] = eq_pu[0] - ugq_pu[0] - igd_pu[0] * Xfilter * F_current;
			}
			else if(control_mode == GFL_CURRENT_SOURCE)
			{
				// Initialize the current source
				I_source[0] = value_IGenerated[0];

				curr_time->igd_filter[0] = igd_pu[0];
				curr_time->igq_filter[0] = igq_pu[0];
			}

			if (frequency_watt)
			{
				// Initialize the frequency-watt
				curr_time->f_filter = fPLL[0];
				Pref_droop_pu = (w_ref / 2.0 / PI - curr_time->f_filter) / Rp + Pref / S_base;
				curr_time->Pref_droop_pu_filter = Pref_droop_pu;
			}

			if (volt_var)
			{
				// Initialize the volt-var control
				V_Avg_pu = value_Circuit_V[0].Mag() / V_base;
				curr_time->V_filter = V_Avg_pu;
				curr_time->Qref_droop_pu_filter = Qref / S_base;
			}
		}
		else //Three-phase
		{
			if ((grid_following_mode == BALANCED_POWER)||(grid_following_mode == POSITIVE_SEQUENCE))
			{
				//Update output power
				//Get current injected to the grid, value_IGenerated is obtained from power flow calculation
				temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0] * value_Circuit_V[0] - generator_admittance[0][1] * value_Circuit_V[1] - generator_admittance[0][2] * value_Circuit_V[2]);
				temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0] * value_Circuit_V[0] - generator_admittance[1][1] * value_Circuit_V[1] - generator_admittance[1][2] * value_Circuit_V[2]);
				temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0] * value_Circuit_V[0] - generator_admittance[2][1] * value_Circuit_V[1] - generator_admittance[2][2] * value_Circuit_V[2]);

				//Update power output variables, just so we can see what is going on
				power_val[0] = value_Circuit_V[0] * ~temp_current_val[0];
				power_val[1] = value_Circuit_V[1] * ~temp_current_val[1];
				power_val[2] = value_Circuit_V[2] * ~temp_current_val[2];

				VA_Out = power_val[0] + power_val[1] + power_val[2];

				//Pref = VA_Out.Re();
				//Qref = VA_Out.Im();
				//See if it is the first deltamode entry - theory is all future changes will trigger deltamode, so these should be set
				if (first_deltamode_init == true)
				{
					//See if it was set
					if (Vset < 0.0)	//-99.0 flag
					{
						Vset = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base + Qref / S_base * Rq;
					}
					else
					{
						//Set, so use Vset, but bias based off the Qref
						Vset += Qref / S_base * Rq;
					}

					//Set it false in here, for giggles
					first_deltamode_init = false;
				}
				//Default else - all changes should be in deltamode

				if(grid_following_mode == BALANCED_POWER)
				{
					for (int i = 0; i < 3; i++)
					{
						// Initialize the PLL
						curr_time->Angle_PLL[i] = value_Circuit_V[i].Arg();
						curr_time->delta_w_PLL_ini[i] = 0;
					}
				}
				else if(grid_following_mode == POSITIVE_SEQUENCE)
				{
					// Obtain the positive sequence voltage
					value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;

					// only consider positive sequence
					curr_time->Angle_PLL[0] = value_Circuit_V_PS.Arg();
					curr_time->Angle_PLL[1] = value_Circuit_V_PS.Arg() - 2.0 / 3.0 * PI;
					curr_time->Angle_PLL[2] = value_Circuit_V_PS.Arg() + 2.0 / 3.0 * PI;

					for (int i = 0; i < 1; i++)
					{
						// Initialize the PLL
						curr_time->delta_w_PLL_ini[i] = 0;
					}
				}

				for (int i = 0; i < 3; i++)
				{
					ugd_pu[i] = (value_Circuit_V[i].Re() * cos(curr_time->Angle_PLL[i]) + value_Circuit_V[i].Im() * sin(curr_time->Angle_PLL[i])) / V_base;
					ugq_pu[i] = (-value_Circuit_V[i].Re() * sin(curr_time->Angle_PLL[i]) + value_Circuit_V[i].Im() * cos(curr_time->Angle_PLL[i])) / V_base;
					igd_pu[i] = (temp_current_val[i].Re() * cos(curr_time->Angle_PLL[i]) + temp_current_val[i].Im() * sin(curr_time->Angle_PLL[i])) / I_base;
					igq_pu[i] = (-temp_current_val[i].Re() * sin(curr_time->Angle_PLL[i]) + temp_current_val[i].Im() * cos(curr_time->Angle_PLL[i])) / I_base;

					if (control_mode == GRID_FOLLOWING)
					{
						// Initialize the current control loops
						e_source[i] = (value_IGenerated[i] * complex(Rfilter, Xfilter) * Z_base);
						ed_pu[i] = (e_source[i].Re() * cos(curr_time->Angle_PLL[i]) + e_source[i].Im() * sin(curr_time->Angle_PLL[i])) / V_base;
						eq_pu[i] = (-e_source[i].Re() * sin(curr_time->Angle_PLL[i]) + e_source[i].Im() * cos(curr_time->Angle_PLL[i])) / V_base;

						curr_time->igd_PI_ini[i] = ed_pu[i] - ugd_pu[i] + igq_pu[i] * Xfilter * F_current;
						curr_time->igq_PI_ini[i] = eq_pu[i] - ugq_pu[i] - igd_pu[i] * Xfilter * F_current;
					}
					else if(control_mode == GFL_CURRENT_SOURCE)
					{
						// Initialize the current control loops
						I_source[i] = value_IGenerated[i];

						curr_time->igd_filter[i] = igd_pu[i];
						curr_time->igq_filter[i] = igq_pu[i];
					}
				}

				if (frequency_watt)
				{
					// Initialize the frequency-watt
					curr_time->f_filter = (fPLL[0] + fPLL[1] + fPLL[2]) / 3.0;
					Pref_droop_pu = (w_ref / 2.0 / PI - curr_time->f_filter) / Rp + Pref / S_base;
					curr_time->Pref_droop_pu_filter = Pref_droop_pu;
				}

				if (volt_var)
				{
					// Initialize the volt-var control
					V_Avg_pu = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag()) / 3.0 / V_base;
					curr_time->V_filter = V_Avg_pu;
					curr_time->Qref_droop_pu_filter = Qref / S_base;
				}

			}	 // end of three phase initialization

		}
	}
	return SUCCESS;
}

// //Module-level post update call
// /* Think this was just put here as an example - not sure what it would do */
// STATUS inverter_dyn::post_deltaupdate(complex *useful_value, unsigned int mode_pass)
// {
// 	//If we have a meter, reset the accumulators
// 	if (parent_is_a_meter == true)
// 	{
// 		reset_complex_powerflow_accumulators();
// 	}

// 	//Should have a parent, but be paranoid
// 	if (parent_is_a_meter == true)
// 	{
// 		push_complex_powerflow_values();
// 	}

// 	return SUCCESS; //Always succeeds right now
// }

//Map Complex value
gld_property *inverter_dyn::map_complex_value(OBJECT *obj, char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if ((pQuantity->is_valid() != true) || (pQuantity->is_complex() != true))
	{
		GL_THROW("inverter_dyn:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Map double value
gld_property *inverter_dyn::map_double_value(OBJECT *obj, char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj, name);

	//Make sure it worked
	if ((pQuantity->is_valid() != true) || (pQuantity->is_double() != true))
	{
		GL_THROW("inverter_dyn:%d %s - Unable to map property %s from object:%d %s", objhdr->id, (objhdr->name ? objhdr->name : "Unnamed"), name, obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in inverter.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Function to pull all the complex properties from powerflow into local variables
void inverter_dyn::pull_complex_powerflow_values(void)
{
	//Pull in the various values from powerflow - straight reads
	//Pull status
	value_MeterStatus = pMeterStatus->get_enumeration();

	//********** TODO - Portions of this may need to be a "deltamode only" pull	 **********//
	//Update IGenerated, in case the powerflow is overriding it
	if (parent_is_single_phase == true)
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
void inverter_dyn::reset_complex_powerflow_accumulators(void)
{
	int indexval;

	//See which one we are, since that will impact things
	if (parent_is_single_phase == false) //Three-phase
	{
		//Loop through the three-phases/accumulators
		for (indexval = 0; indexval < 3; indexval++)
		{
			//**** Current value ***/
			value_Line_I[indexval] = complex(0.0, 0.0);

			//**** Power value ***/
			value_Power[indexval] = complex(0.0, 0.0);

			//**** pre-rotated Current value ***/
			value_Line_unrotI[indexval] = complex(0.0, 0.0);
		}
	}
	else //Assumes must be single phased - else how did it get here?
	{
		//Reset the relevant values -- all single pulls

		//**** single current value ***/
		value_Line_I[0] = complex(0.0, 0.0);

		//**** power value ***/
		value_Power[0] = complex(0.0, 0.0);

		//**** prerotated value ***/
		value_Line_unrotI[0] = complex(0.0, 0.0);
	}
}

//Function to push up all changes of complex properties to powerflow from local variables
void inverter_dyn::push_complex_powerflow_values(bool update_voltage)
{
	complex temp_complex_val;
	gld_wlock *test_rlock;
	int indexval;

	//See which one we are, since that will impact things
	if (parent_is_single_phase == false) //Three-phase
	{
		//See if we were a voltage push or not
		if (update_voltage == true)
		{
			//Loop through the three-phases/accumulators
			for (indexval=0; indexval<3; indexval++)
			{
				//**** push voltage value -- not an accumulator, just force ****/
				pCircuit_V[indexval]->setp<complex>(value_Circuit_V[indexval],*test_rlock);
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
				pLine_I[indexval]->setp<complex>(temp_complex_val, *test_rlock);

				//**** Power value ***/
				//Pull current value again, just in case
				temp_complex_val = pPower[indexval]->get_complex();

				//Add the difference
				temp_complex_val += value_Power[indexval];

				//Push it back up
				pPower[indexval]->setp<complex>(temp_complex_val, *test_rlock);

				//**** pre-rotated Current value ***/
				//Pull current value again, just in case
				temp_complex_val = pLine_unrotI[indexval]->get_complex();

				//Add the difference
				temp_complex_val += value_Line_unrotI[indexval];

				//Push it back up
				pLine_unrotI[indexval]->setp<complex>(temp_complex_val, *test_rlock);

				/* If was VSI, adjust Norton injection */
				{
					//**** IGenerated Current value ***/
					//Direct write, not an accumulator
					pIGenerated[indexval]->setp<complex>(value_IGenerated[indexval], *test_rlock);
				}
			}//End phase loop
		}//End not voltage push
	}//End three-phase
	else //Assumes must be single-phased - else how did it get here?
	{
		//Check for voltage push - in case that's ever needed here
		if (update_voltage == true)
		{
			//Should just be zero
			//**** push voltage value -- not an accumulator, just force ****/
			pCircuit_V[0]->setp<complex>(value_Circuit_V[0],*test_rlock);
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
			pLine_I[0]->setp<complex>(temp_complex_val, *test_rlock);

			//**** power value ***/
			//Pull current value again, just in case
			temp_complex_val = pPower[0]->get_complex();

			//Add the difference
			temp_complex_val += value_Power[0];

			//Push it back up
			pPower[0]->setp<complex>(temp_complex_val, *test_rlock);

			//**** prerotated value ***/
			//Pull current value again, just in case
			temp_complex_val = pLine_unrotI[0]->get_complex();

			//Add the difference
			temp_complex_val += value_Line_unrotI[0];

			//Push it back up
			pLine_unrotI[0]->setp<complex>(temp_complex_val, *test_rlock);

			//**** IGenerated ****/
			//********* TODO - Does this need to be deltamode-flagged? *************//
			//Direct write, not an accumulator
			pIGenerated[0]->setp<complex>(value_IGenerated[0], *test_rlock);
		}//End not voltage update
	}//End single-phase
}

// Function to update current injection IGenerated for VSI
STATUS inverter_dyn::updateCurrInjection(int64 iteration_count)
{
	double temp_time;
	OBJECT *obj = OBJECTHDR(this);
	complex temp_VA;
	complex temp_V1, temp_V2;
	bool bus_is_a_swing, bus_is_swing_pq_entry;
	STATUS temp_status_val;
	gld_property *temp_property_pointer;
	bool running_in_delta;
	double freq_diff_angle_val, tdiff;
	complex rotate_value;

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
	if (parent_is_a_meter == true)
	{
		//Reset the accumulators, just in case
		reset_complex_powerflow_accumulators();

		//Pull status and voltage (mostly status)
		pull_complex_powerflow_values();
	}

	//See if we're in QSTS and a grid-forming inverter - update if we are
	if ((running_in_delta == false) && (control_mode == GRID_FORMING))
	{
		//Compute time difference
		tdiff = temp_time - prev_timestamp_dbl;

		//Get the frequency difference and make an angle difference
		freq_diff_angle_val = (freq - f_nominal)*2.0*M_PI*tdiff;

		//Compute the "adjustment" (basically exp(j-angle))
		//exp(jx) = cos(x)+j*sin(x)
		rotate_value = complex(cos(freq_diff_angle_val),sin(freq_diff_angle_val));

		//Apply the adjustment to voltage and current injection
		value_Circuit_V[0] *= rotate_value;
		value_Circuit_V[1] *= rotate_value;
		value_Circuit_V[2] *= rotate_value;
		value_IGenerated[0] *= rotate_value;
		value_IGenerated[1] *= rotate_value;
		value_IGenerated[2] *= rotate_value;

		//Push the voltage - standard meter check (bit redundant)
		if (parent_is_a_meter == true)
		{
			push_complex_powerflow_values(true);
		}

		//Update trackers
		prev_timestamp_dbl = temp_time;
		last_QSTS_GF_Update = temp_time;
	}

	//External call to internal variables -- used by powerflow to iterate the VSI implementation, basically
	temp_VA = complex(Pref, Qref);

	//See if we're a meter
	if (parent_is_a_meter == true)
	{
		//By default, assume we're not a SWING
		bus_is_a_swing = false;

		//Determine our status
		if (attached_bus_type > 1) //SWING or SWING_PQ
		{
			//Map the function, if we need to
			if (swing_test_fxn == NULL)
			{
				//Map the swing status check function
				swing_test_fxn = (FUNCTIONADDR)(gl_get_function(obj->parent, "pwr_object_swing_status_check"));

				//See if it was located
				if (swing_test_fxn == NULL)
				{
					GL_THROW("inverter_dyn:%s - failed to map swing-checking for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
				GL_THROW("inverter_dyn:%s - failed to map swing-checking for node:%s", (obj->name ? obj->name : "unnamed"), (obj->parent->name ? obj->parent->name : "unnamed"));
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
			else if ((first_iteration_current_injection != 0) || (bus_is_swing_pq_entry==true)) //We didn't enter on the first iteration, or we're a repatrioted SWING_PQ
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
	if (inverter_first_step == true)
	{
		if (parent_is_single_phase == true) // single phase/split-phase implementation
		{

			if ((control_mode == GRID_FOLLOWING) || (control_mode == GFL_CURRENT_SOURCE)) //
			{
				//Special exception for triplex (since delta doesn't get updated inside the powerflow)
				if ((phases & 0x10) == 0x10)
				{
					//Pull in other voltages
					temp_V1 = pCircuit_V[1]->get_complex();
					temp_V2 = pCircuit_V[2]->get_complex();

					//Just push it in as the update
					value_Circuit_V[0] = temp_V1 + temp_V2;
				}

				//Back out the current injection
				temp_current_val[0] = ~(temp_VA / value_Circuit_V[0]) + filter_admittance * value_Circuit_V[0];

				//Apply and see what happens
				value_IGenerated[0] = temp_current_val[0]; // for grid-following inverters, the internal voltages need to be three phase balanced
			}
		}
		else //Three-phase
		{

			if (control_mode == GRID_FORMING)
			{
				if (bus_is_a_swing == false)
				{
					//Balance voltage and satisfy power
					complex aval, avalsq;
					complex temp_total_power_val[3];
					complex temp_total_power_internal;
					complex temp_pos_voltage, temp_pos_current;

					//Conversion variables - 1@120-deg
					aval = complex(-0.5, (sqrt(3.0) / 2.0));
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
					value_IGenerated[0] = temp_pos_current;
					value_IGenerated[1] = temp_pos_current * avalsq;
					value_IGenerated[2] = temp_pos_current * aval;
				}
				//If it is a swing, this is all handled internally to solver_nr (though it is similar)
			}
			else //grid following, or current source representation of grid-following
			{
				if (grid_following_mode == BALANCED_POWER) // Assume the inverter injects balanced power
				{

					//Back out the current injection
					temp_current_val[0] = ~(temp_VA / 3.0 / value_Circuit_V[0]) + generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2];
					temp_current_val[1] = ~(temp_VA / 3.0 / value_Circuit_V[1]) + generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2];
					temp_current_val[2] = ~(temp_VA / 3.0 / value_Circuit_V[2]) + generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2];

					//Apply and see what happens
					value_IGenerated[0] = temp_current_val[0]; // for grid-following inverters, the internal voltages need to be three phase balanced
					value_IGenerated[1] = temp_current_val[1];
					value_IGenerated[2] = temp_current_val[2];
				}
				else //Assume injects balanced current, positive sequence
				{
					// Obtain the positive sequence voltage
					value_Circuit_V_PS = (value_Circuit_V[0] + value_Circuit_V[1] * complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI)) + value_Circuit_V[2] * complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI))) / 3.0;
					// Obtain the positive sequence current
					value_Circuit_I_PS[0] = ~(temp_VA / 3.0 / value_Circuit_V_PS);
					value_Circuit_I_PS[1] = value_Circuit_I_PS[0] * complex(cos(-2.0 / 3.0 * PI), sin(-2.0 / 3.0 * PI));
					value_Circuit_I_PS[2] = value_Circuit_I_PS[0] * complex(cos(2.0 / 3.0 * PI), sin(2.0 / 3.0 * PI));

					//Back out the current injection
					temp_current_val[0] = value_Circuit_I_PS[0] + generator_admittance[0][0] * value_Circuit_V[0] + generator_admittance[0][1] * value_Circuit_V[1] + generator_admittance[0][2] * value_Circuit_V[2];
					temp_current_val[1] = value_Circuit_I_PS[1] + generator_admittance[1][0] * value_Circuit_V[0] + generator_admittance[1][1] * value_Circuit_V[1] + generator_admittance[1][2] * value_Circuit_V[2];
					temp_current_val[2] = value_Circuit_I_PS[2] + generator_admittance[2][0] * value_Circuit_V[0] + generator_admittance[2][1] * value_Circuit_V[1] + generator_admittance[2][2] * value_Circuit_V[2];

					//Apply and see what happens
					//for grid following inverter, the internal voltages may not be balanced
					value_IGenerated[0] = temp_current_val[0];
					value_IGenerated[1] = temp_current_val[1];
					value_IGenerated[2] = temp_current_val[2];
				}
			}
		}
	}
	//Default else - things are handled elsewhere

	//Push the changes up
	if (parent_is_a_meter == true)
	{
		push_complex_powerflow_values(false);
	}

	//Always a success, but power flow solver may not like it if VA_OUT exceeded the rating and thus changed
	return SUCCESS;
}

//Internal function to the mapping of the DC object update function
STATUS inverter_dyn::DC_object_register(OBJECT *DC_object)
{
	FUNCTIONADDR temp_add = NULL;
	DC_OBJ_FXNS temp_DC_struct;
	OBJECT *obj = OBJECTHDR(this);

	//Put the object into the structure
	temp_DC_struct.dc_object = DC_object;

	//Find the update function
	temp_DC_struct.fxn_address = (FUNCTIONADDR)(gl_get_function(DC_object, "DC_gen_object_update"));

	//Make sure it worked
	if (temp_DC_struct.fxn_address == NULL)
	{
		gl_error("inverter_dyn:%s - failed to map DC update for object %s", (obj->name ? obj->name : "unnamed"), (DC_object->name ? DC_object->name : "unnamed"));
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

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_inverter_dyn(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(inverter_dyn::oclass);
		if (*obj != NULL)
		{
			inverter_dyn *my = OBJECTDATA(*obj, inverter_dyn);
			gl_set_parent(*obj, parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(inverter_dyn);
}

EXPORT int init_inverter_dyn(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj != NULL)
			return OBJECTDATA(obj, inverter_dyn)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(inverter_dyn);
}

EXPORT TIMESTAMP sync_inverter_dyn(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	inverter_dyn *my = OBJECTDATA(obj, inverter_dyn);
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
	SYNC_CATCHALL(inverter_dyn);
	return t2;
}

EXPORT int isa_inverter_dyn(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,inverter_dyn)->isa(classname);
}

EXPORT STATUS preupdate_inverter_dyn(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time)
{
	inverter_dyn *my = OBJECTDATA(obj, inverter_dyn);
	STATUS status_output = FAILED;

	try
	{
		status_output = my->pre_deltaupdate(t0, delta_time);
		return status_output;
	}
	catch (char *msg)
	{
		gl_error("preupdate_inverter_dyn(obj=%d;%s): %s", obj->id, (obj->name ? obj->name : "unnamed"), msg);
		return status_output;
	}
}

EXPORT SIMULATIONMODE interupdate_inverter_dyn(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	inverter_dyn *my = OBJECTDATA(obj, inverter_dyn);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time, dt, iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_inverter_dyn(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
		return status;
	}
}

// EXPORT STATUS postupdate_inverter_dyn(OBJECT *obj, complex *useful_value, unsigned int mode_pass)
// {
// 	inverter_dyn *my = OBJECTDATA(obj, inverter_dyn);
// 	STATUS status = FAILED;
// 	try
// 	{
// 		status = my->post_deltaupdate(useful_value, mode_pass);
// 		return status;
// 	}
// 	catch (char *msg)
// 	{
// 		gl_error("postupdate_inverter_dyn(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
// 		return status;
// 	}
// }

//// Define export function that update the VSI current injection IGenerated to the grid
EXPORT STATUS inverter_dyn_NR_current_injection_update(OBJECT *obj, int64 iteration_count)
{
	STATUS temp_status;

	//Map the node
	inverter_dyn *my = OBJECTDATA(obj, inverter_dyn);

	//Call the function, where we can update the IGenerated injection
	temp_status = my->updateCurrInjection(iteration_count);

	//Return what the sub function said we were
	return temp_status;
}

// Export function for registering a DC interaction object
EXPORT STATUS inverter_dyn_DC_object_register(OBJECT *this_obj, OBJECT *DC_obj)
{
	STATUS temp_status;

	//Map us
	inverter_dyn *this_inv = OBJECTDATA(this_obj, inverter_dyn);

	//Call the function to register us
	temp_status = this_inv->DC_object_register(DC_obj);

	//Return the status
	return temp_status;
}
