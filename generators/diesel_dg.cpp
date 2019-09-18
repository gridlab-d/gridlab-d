/** $Id: diesel_dg.cpp,v 1.2 2008/02/12 00:28:08 d3g637 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file diesel_dg.cpp
	@defgroup diesel_dg Diesel gensets
	@ingroup generators

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "diesel_dg.h"

CLASS *diesel_dg::oclass = NULL;
diesel_dg *diesel_dg::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
diesel_dg::diesel_dg(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"diesel_dg",sizeof(diesel_dg),passconfig|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class diesel_dg";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,

			PT_enumeration,"Gen_type",PADDR(Gen_type),
				PT_KEYWORD,"CONSTANT_PQ",(enumeration)NON_DYN_CONSTANT_PQ,PT_DESCRIPTION,"Non-dynamic mode of diesel generator with constant PQ output as defined",
				PT_KEYWORD,"DYN_SYNCHRONOUS",(enumeration)DYNAMIC,PT_DESCRIPTION,"Dynamics-capable implementation of synchronous diesel generator",
		
			PT_double, "pf", PADDR(power_factor),PT_DESCRIPTION,"desired power factor",

			//End of synchronous generator inputs
			PT_double, "Rated_V[V]", PADDR(Rated_V_LL),PT_DESCRIPTION,"nominal line-line voltage in Volts",
			PT_double, "Rated_VA[VA]", PADDR(Rated_VA),PT_DESCRIPTION,"nominal capacity in VA",
			PT_double, "overload_limit[pu]", PADDR(Overload_Limit_Pub),PT_DESCRIPTION,"per-unit value of the maximum power the generator can provide",

			PT_complex, "current_out_A[A]", PADDR(current_val[0]),PT_DESCRIPTION,"Output current of phase A",
			PT_complex, "current_out_B[A]", PADDR(current_val[1]),PT_DESCRIPTION,"Output current of phase B",
			PT_complex, "current_out_C[A]", PADDR(current_val[2]),PT_DESCRIPTION,"Output current of phase C",
			PT_complex, "power_out_A[VA]", PADDR(power_val[0]),PT_DESCRIPTION,"Output power of phase A",
			PT_complex, "power_out_B[VA]", PADDR(power_val[1]),PT_DESCRIPTION,"Output power of phase B",
			PT_complex, "power_out_C[VA]", PADDR(power_val[2]),PT_DESCRIPTION,"Output power of phase C",
			
			//Properties for dynamics capabilities (subtransient model)
			PT_double,"omega_ref[rad/s]",PADDR(omega_ref),PT_DESCRIPTION,"Reference frequency of generator (rad/s)",
			PT_double,"inertia",PADDR(inertia),PT_DESCRIPTION,"Inertial constant (H) of generator",
			PT_double,"damping",PADDR(damping),PT_DESCRIPTION,"Damping constant (D) of generator",
			PT_double,"number_poles",PADDR(number_poles),PT_DESCRIPTION,"Number of poles in the generator",
			PT_double,"Ra[pu]",PADDR(Ra),PT_DESCRIPTION,"Stator resistance (p.u.)",
			PT_double,"Xd[pu]",PADDR(Xd),PT_DESCRIPTION,"d-axis reactance (p.u.)",
			PT_double,"Xq[pu]",PADDR(Xq),PT_DESCRIPTION,"q-axis reactance (p.u.)",
			PT_double,"Xdp[pu]",PADDR(Xdp),PT_DESCRIPTION,"d-axis transient reactance (p.u.)",
			PT_double,"Xqp[pu]",PADDR(Xqp),PT_DESCRIPTION,"q-axis transient reactance (p.u.)",
			PT_double,"Xdpp[pu]",PADDR(Xdpp),PT_DESCRIPTION,"d-axis subtransient reactance (p.u.)",
			PT_double,"Xqpp[pu]",PADDR(Xqpp),PT_DESCRIPTION,"q-axis subtransient reactance (p.u.)",
			PT_double,"Xl[pu]",PADDR(Xl),PT_DESCRIPTION,"Leakage reactance (p.u.)",
			PT_double,"Tdp[s]",PADDR(Tdp),PT_DESCRIPTION,"d-axis short circuit time constant (s)",
			PT_double,"Tdop[s]",PADDR(Tdop),PT_DESCRIPTION,"d-axis open circuit time constant (s)",
			PT_double,"Tqop[s]",PADDR(Tqop),PT_DESCRIPTION,"q-axis open circuit time constant (s)",
			PT_double,"Tdopp[s]",PADDR(Tdopp),PT_DESCRIPTION,"d-axis open circuit subtransient time constant (s)",
			PT_double,"Tqopp[s]",PADDR(Tqopp),PT_DESCRIPTION,"q-axis open circuit subtransient time constant (s)",
			PT_double,"Ta[s]",PADDR(Ta),PT_DESCRIPTION,"Armature short-circuit time constant (s)",
			PT_complex,"X0[pu]",PADDR(X0),PT_DESCRIPTION,"Zero sequence impedance (p.u.)",
			PT_complex,"X2[pu]",PADDR(X2),PT_DESCRIPTION,"Negative sequence impedance (p.u.)",

			//Convergence criterion for exiting deltamode - just on rotor_speed for now
			PT_double,"rotor_speed_convergence[rad]",PADDR(rotor_speed_convergence_criterion),PT_DESCRIPTION,"Convergence criterion on rotor speed used to determine when to exit deltamode",
			PT_double,"voltage_convergence[V]",PADDR(voltage_convergence_criterion),PT_DESCRIPTION,"Convergence criterion for voltage changes (if exciter present) to determine when to exit deltamode",

			//Which to enable
			PT_bool,"rotor_speed_convergence_enabled",PADDR(apply_rotor_speed_convergence),PT_DESCRIPTION,"Uses rotor_speed_convergence to determine if an exit of deltamode is needed",
			PT_bool,"voltage_magnitude_convergence_enabled",PADDR(apply_voltage_mag_convergence),PT_DESCRIPTION,"Uses voltage_convergence to determine if an exit of deltamode is needed - only works if an exciter is present",

			//State outputs
			PT_double,"rotor_angle[rad]",PADDR(curr_state.rotor_angle),PT_DESCRIPTION,"rotor angle state variable",
			PT_double,"rotor_speed[rad/s]",PADDR(curr_state.omega),PT_DESCRIPTION,"machine speed state variable",
			PT_double,"field_voltage[pu]",PADDR(curr_state.Vfd),PT_DESCRIPTION,"machine field voltage state variable",
			PT_double,"flux1d[pu]",PADDR(curr_state.Flux1d),PT_DESCRIPTION,"machine transient flux on d-axis state variable",
			PT_double,"flux2q[pu]",PADDR(curr_state.Flux2q),PT_DESCRIPTION,"machine subtransient flux on q-axis state variable",
			PT_complex,"EpRotated[pu]",PADDR(curr_state.EpRotated),PT_DESCRIPTION,"d-q rotated E-prime internal voltage state variable",
			PT_complex,"VintRotated[pu]",PADDR(curr_state.VintRotated),PT_DESCRIPTION,"d-q rotated Vint voltage state variable",
			PT_complex,"Eint_A[V]",PADDR(curr_state.EintVal[0]),PT_DESCRIPTION,"Unrotated, unsequenced phase A internal voltage",
			PT_complex,"Eint_B[V]",PADDR(curr_state.EintVal[1]),PT_DESCRIPTION,"Unrotated, unsequenced phase B internal voltage",
			PT_complex,"Eint_C[V]",PADDR(curr_state.EintVal[2]),PT_DESCRIPTION,"Unrotated, unsequenced phase C internal voltage",
			PT_complex,"Irotated[pu]",PADDR(curr_state.Irotated),PT_DESCRIPTION,"d-q rotated sequence current state variable",
			PT_complex,"pwr_electric[VA]",PADDR(curr_state.pwr_electric),PT_DESCRIPTION,"Current electrical output of machine",
			PT_double,"pwr_mech[W]",PADDR(curr_state.pwr_mech),PT_DESCRIPTION,"Current mechanical output of machine",
			PT_double,"torque_mech[N*m]",PADDR(curr_state.torque_mech),PT_DESCRIPTION,"Current mechanical torque of machine",
			PT_double,"torque_elec[N*m]",PADDR(curr_state.torque_elec),PT_DESCRIPTION,"Current electrical torque output of machine",

			//Overall inputs for dynamics model - governor and exciter "tweakables"
			PT_double,"wref[pu]", PADDR(gen_base_set_vals.wref), PT_DESCRIPTION, "wref input to governor controls (per-unit)",
			PT_double,"vset[pu]", PADDR(gen_base_set_vals.vset), PT_DESCRIPTION, "vset input to AVR controls (per-unit)",
			PT_double,"Pref[pu]", PADDR(gen_base_set_vals.Pref), PT_DESCRIPTION, "Pref input to governor controls (per-unit), if supported",
			PT_double,"Qref[pu]", PADDR(gen_base_set_vals.Qref), PT_DESCRIPTION, "Qref input to govornor or AVR controls (per-unit), if supported",

			//Properties for AVR/Exciter of dynamics model
			PT_enumeration,"Exciter_type",PADDR(Exciter_type),PT_DESCRIPTION,"Exciter model for dynamics-capable implementation",
				PT_KEYWORD,"NO_EXC",(enumeration)NO_EXC,PT_DESCRIPTION,"No exciter",
				PT_KEYWORD,"SEXS",(enumeration)SEXS,PT_DESCRIPTION,"Simplified Excitation System",

			PT_double,"KA[pu]",PADDR(exc_KA),PT_DESCRIPTION,"Exciter gain (p.u.)",
			PT_double,"TA[s]",PADDR(exc_TA),PT_DESCRIPTION,"Exciter time constant (seconds)",
			PT_double,"TB[s]",PADDR(exc_TB),PT_DESCRIPTION,"Exciter transient gain reduction time constant (seconds)",
			PT_double,"TC[s]",PADDR(exc_TC),PT_DESCRIPTION,"Exciter transient gain reduction time constant (seconds)",
			PT_double,"EMAX[pu]",PADDR(exc_EMAX),PT_DESCRIPTION,"Exciter upper limit (p.u.)",
			PT_double,"EMIN[pu]",PADDR(exc_EMIN),PT_DESCRIPTION,"Exciter lower limit (p.u.)",
			PT_double,"Vterm_max[pu]",PADDR(Max_Ef),PT_DESCRIPTION,"Upper voltage limit for super-second (p.u.)",
			PT_double,"Vterm_min[pu]",PADDR(Min_Ef),PT_DESCRIPTION,"Lower voltage limit for super-second (p.u.)",

			//State variables - SEXS
			PT_double,"bias",PADDR(curr_state.avr.bias),PT_DESCRIPTION,"Exciter bias state variable",
			PT_double,"xe",PADDR(curr_state.avr.xe),PT_DESCRIPTION,"Exciter state variable",
			PT_double,"xb",PADDR(curr_state.avr.xb),PT_DESCRIPTION,"Exciter state variable",
//			PT_double,"xcvr",PADDR(curr_state.avr.x_cvr),PT_DESCRIPTION,"Exciter state variable",
			PT_double,"x_cvr1",PADDR(curr_state.avr.x_cvr1),PT_DESCRIPTION,"Exciter state variable",
			PT_double,"x_cvr2",PADDR(curr_state.avr.x_cvr2),PT_DESCRIPTION,"Exciter state variable",
			PT_double,"Vref",PADDR(Vref),PT_DESCRIPTION,"Exciter CVR control voltage reference value",
			//Properties for CVR mode
			PT_enumeration,"CVR_mode",PADDR(CVRmode),PT_DESCRIPTION,"CVR mode in Exciter model",
				PT_KEYWORD,"HighOrder",(enumeration)HighOrder,PT_DESCRIPTION,"High order control mode",
				PT_KEYWORD,"Feedback",(enumeration)Feedback,PT_DESCRIPTION,"First order control mode with feedback loop",

			// If P_constant delta mode is adopted
			PT_double,"P_CONSTANT_ki", PADDR(ki_Pconstant), PT_DESCRIPTION, "parameter of the integration control for constant P mode",
			PT_double,"P_CONSTANT_kp", PADDR(kp_Pconstant), PT_DESCRIPTION, "parameter of the proportional control for constant P mode",

			// If Q_constant delta mode is adopted
			PT_bool, "Exciter_Q_constant_mode",PADDR(Q_constant_mode),PT_DESCRIPTION,"True if the generator is operating under constant Q mode",
			PT_double,"Exciter_Q_constant_ki", PADDR(ki_Qconstant), PT_DESCRIPTION, "parameter of the integration control for constant Q mode",
			PT_double,"Exciter_Q_constant_kp", PADDR(kp_Qconstant), PT_DESCRIPTION, "parameter of the propotional control for constant Q mode",

			// Set PQ reference again here with different names:
			PT_double,"P_CONSTANT_Pref[pu]", PADDR(gen_base_set_vals.Pref), PT_DESCRIPTION, "Pref input to governor controls (per-unit), if supported",
			PT_double,"Exciter_Q_constant_Qref[pu]", PADDR(gen_base_set_vals.Qref), PT_DESCRIPTION, "Qref input to govornor or AVR controls (per-unit), if supported",

			// If CVR control is enabled
			PT_bool, "CVR_enabled",PADDR(CVRenabled),PT_DESCRIPTION,"True if the CVR control is enabled in the exciter",
			PT_double,"CVR_ki_cvr", PADDR(ki_cvr), PT_DESCRIPTION, "parameter of the integration control for CVR control",
			PT_double,"CVR_kp_cvr", PADDR(kp_cvr), PT_DESCRIPTION, "parameter of the proportional control for CVR control",
			PT_double,"CVR_kd_cvr", PADDR(kd_cvr), PT_DESCRIPTION, "parameter of the deviation control for CVR control",
			PT_double,"CVR_kt_cvr", PADDR(kt_cvr), PT_DESCRIPTION, "parameter of the gain in feedback loop for CVR control",
			PT_double,"CVR_kw_cvr", PADDR(kw_cvr), PT_DESCRIPTION, "parameter of the gain in feedback loop for CVR control",
			PT_bool, "CVR_PI",PADDR(CVR_PI),PT_DESCRIPTION,"True if the PI controller is implemented in CVR control",
			PT_bool, "CVR_PID",PADDR(CVR_PID),PT_DESCRIPTION,"True if the PID controller is implemented in CVR control",
			PT_double,"vset_EMAX",PADDR(vset_EMAX),PT_DESCRIPTION,"Maximum Vset limit",
			PT_double,"vset_EMIN",PADDR(vset_EMIN),PT_DESCRIPTION,"Minimum Vset limit",
			PT_double,"CVR_Kd1", PADDR(Kd1), PT_DESCRIPTION, "parameter of the second order transfer function for CVR control",
			PT_double,"CVR_Kd2", PADDR(Kd2), PT_DESCRIPTION, "parameter of the second order transfer function for CVR control",
			PT_double,"CVR_Kd3", PADDR(Kd3), PT_DESCRIPTION, "parameter of the second order transfer function for CVR control",
			PT_double,"CVR_Kn1", PADDR(Kn1), PT_DESCRIPTION, "parameter of the second order transfer function for CVR control",
			PT_double,"CVR_Kn2", PADDR(Kn2), PT_DESCRIPTION, "parameter of the second order transfer function for CVR control",
			PT_double,"vset_delta_MAX",PADDR(vset_delta_MAX),PT_DESCRIPTION,"Maximum delta Vset limit",
			PT_double,"vset_delta_MIN",PADDR(vset_delta_MIN),PT_DESCRIPTION,"Minimum delta Vset limit",
			PT_double,"vadd",PADDR(gen_base_set_vals.vadd),PT_DESCRIPTION,"Delta Vset",
			PT_double,"vadd_a",PADDR(gen_base_set_vals.vadd_a),PT_DESCRIPTION,"Delta Vset before going into bound check",

			//Properties for Governor of dynamics model
			PT_enumeration,"Governor_type",PADDR(Governor_type),PT_DESCRIPTION,"Governor model for dynamics-capable implementation",
				PT_KEYWORD,"NO_GOV",(enumeration)NO_GOV,PT_DESCRIPTION,"No exciter",
				PT_KEYWORD,"DEGOV1",(enumeration)DEGOV1,PT_DESCRIPTION,"DEGOV1 Woodward Diesel Governor",
				PT_KEYWORD,"GAST",(enumeration)GAST,PT_DESCRIPTION,"GAST Gas Turbine Governor",
				PT_KEYWORD,"GGOV1_OLD",(enumeration)GGOV1_OLD,PT_DESCRIPTION,"Older GGOV1 Governor Model",
				PT_KEYWORD,"GGOV1",(enumeration)GGOV1,PT_DESCRIPTION,"GGOV1 Governor Model",
				PT_KEYWORD,"P_CONSTANT",(enumeration)P_CONSTANT,PT_DESCRIPTION,"P_CONSTANT mode Governor Model",

			//Governor properties (DEGOV1)
			PT_double,"DEGOV1_R[pu]",PADDR(gov_degov1_R),PT_DESCRIPTION,"Governor droop constant (p.u.)",
			PT_double,"DEGOV1_T1[s]",PADDR(gov_degov1_T1),PT_DESCRIPTION,"Governor electric control box time constant (s)",
			PT_double,"DEGOV1_T2[s]",PADDR(gov_degov1_T2),PT_DESCRIPTION,"Governor electric control box time constant (s)",
			PT_double,"DEGOV1_T3[s]",PADDR(gov_degov1_T3),PT_DESCRIPTION,"Governor electric control box time constant (s)",
			PT_double,"DEGOV1_T4[s]",PADDR(gov_degov1_T4),PT_DESCRIPTION,"Governor actuator time constant (s)",
			PT_double,"DEGOV1_T5[s]",PADDR(gov_degov1_T5),PT_DESCRIPTION,"Governor actuator time constant (s)",
			PT_double,"DEGOV1_T6[s]",PADDR(gov_degov1_T6),PT_DESCRIPTION,"Governor actuator time constant (s)",
			PT_double,"DEGOV1_K[pu]",PADDR(gov_degov1_K),PT_DESCRIPTION,"Governor actuator gain",
			PT_double,"DEGOV1_TMAX[pu]",PADDR(gov_degov1_TMAX),PT_DESCRIPTION,"Governor actuator upper limit (p.u.)",
			PT_double,"DEGOV1_TMIN[pu]",PADDR(gov_degov1_TMIN),PT_DESCRIPTION,"Governor actuator lower limit (p.u.)",
			PT_double,"DEGOV1_TD[s]",PADDR(gov_degov1_TD),PT_DESCRIPTION,"Governor combustion delay (s)",

			//State variables - DEGOV1
			PT_double,"DEGOV1_x1",PADDR(curr_state.gov_degov1.x1),PT_DESCRIPTION,"Governor electric box state variable",
			PT_double,"DEGOV1_x2",PADDR(curr_state.gov_degov1.x2),PT_DESCRIPTION,"Governor electric box state variable",
			PT_double,"DEGOV1_x4",PADDR(curr_state.gov_degov1.x4),PT_DESCRIPTION,"Governor electric box state variable",
			PT_double,"DEGOV1_x5",PADDR(curr_state.gov_degov1.x5),PT_DESCRIPTION,"Governor electric box state variable",
			PT_double,"DEGOV1_x6",PADDR(curr_state.gov_degov1.x6),PT_DESCRIPTION,"Governor electric box state variable",
			PT_double,"DEGOV1_throttle",PADDR(curr_state.gov_degov1.throttle),PT_DESCRIPTION,"Governor electric box state variable",

			//Governor properties (GAST)
			PT_double,"GAST_R[pu]",PADDR(gov_gast_R),PT_DESCRIPTION,"Governor droop constant (p.u.)",
			PT_double,"GAST_T1[s]",PADDR(gov_gast_T1),PT_DESCRIPTION,"Governor electric control box time constant (s)",
			PT_double,"GAST_T2[s]",PADDR(gov_gast_T2),PT_DESCRIPTION,"Governor electric control box time constant (s)",
			PT_double,"GAST_T3[s]",PADDR(gov_gast_T3),PT_DESCRIPTION,"Governor temperature limiter time constant (s)",
			PT_double,"GAST_AT[s]",PADDR(gov_gast_AT),PT_DESCRIPTION,"Governor Ambient Temperature load limit (units)",
			PT_double,"GAST_KT[pu]",PADDR(gov_gast_KT),PT_DESCRIPTION,"Governor temperature control loop gain",
			PT_double,"GAST_VMAX[pu]",PADDR(gov_gast_VMAX),PT_DESCRIPTION,"Governor actuator upper limit (p.u.)",
			PT_double,"GAST_VMIN[pu]",PADDR(gov_gast_VMIN),PT_DESCRIPTION,"Governor actuator lower limit (p.u.)",

			//State variables - GAST
			PT_double,"GAST_x1",PADDR(curr_state.gov_gast.x1),PT_DESCRIPTION,"Governor electric box state variable",
			PT_double,"GAST_x2",PADDR(curr_state.gov_gast.x2),PT_DESCRIPTION,"Governor electric box state variable",
			PT_double,"GAST_x3",PADDR(curr_state.gov_gast.x3),PT_DESCRIPTION,"Governor electric box state variable",
			PT_double,"GAST_throttle",PADDR(curr_state.gov_gast.throttle),PT_DESCRIPTION,"Governor electric box state variable",

			//Governor properties (GGOV1 and GGOV1_OLD)
			PT_double,"GGOV1_R[pu]",PADDR(gov_ggv1_r),PT_DESCRIPTION,"Permanent droop, p.u.",
			PT_int32,"GGOV1_Rselect",PADDR(gov_ggv1_rselect),PT_DESCRIPTION,"Feedback signal for droop, = 1 selected electrical power, = 0 none (isochronous governor), = -1 fuel valve stroke ( true stroke),= -2 governor output ( requested stroke)",
			PT_double,"GGOV1_Tpelec[s]",PADDR(gov_ggv1_Tpelec),PT_DESCRIPTION,"Electrical power transducer time constant, sec. (>0.)",
			PT_double,"GGOV1_maxerr",PADDR(gov_ggv1_maxerr),PT_DESCRIPTION,"Maximum value for speed error signal",
			PT_double,"GGOV1_minerr",PADDR(gov_ggv1_minerr),PT_DESCRIPTION,"Minimum value for speed error signal",
			PT_double,"GGOV1_Kpgov",PADDR(gov_ggv1_Kpgov),PT_DESCRIPTION,"Governor proportional gain",
			PT_double,"GGOV1_Kigov",PADDR(gov_ggv1_Kigov),PT_DESCRIPTION,"Governor integral gain",
			PT_double,"GGOV1_Kdgov",PADDR(gov_ggv1_Kdgov),PT_DESCRIPTION,"Governor derivative gain",
			PT_double,"GGOV1_Tdgov[s]",PADDR(gov_ggv1_Tdgov),PT_DESCRIPTION,"Governor derivative controller time constant, sec.",
			PT_double,"GGOV1_vmax",PADDR(gov_ggv1_vmax),PT_DESCRIPTION,"Maximum valve position limit",
			PT_double,"GGOV1_vmin",PADDR(gov_ggv1_vmin),PT_DESCRIPTION,"Minimum valve position limit",
			PT_double,"GGOV1_Tact",PADDR(gov_ggv1_Tact),PT_DESCRIPTION,"Actuator time constant",
			PT_double,"GGOV1_Kturb",PADDR(gov_ggv1_Kturb),PT_DESCRIPTION,"Turbine gain (>0.)",
			PT_double,"GGOV1_wfnl[pu]",PADDR(gov_ggv1_wfnl),PT_DESCRIPTION,"No load fuel flow, p.u",
			PT_double,"GGOV1_Tb[s]",PADDR(gov_ggv1_Tb),PT_DESCRIPTION,"Turbine lag time constant, sec. (>0.)",
			PT_double,"GGOV1_Tc[s]",PADDR(gov_ggv1_Tc),PT_DESCRIPTION,"Turbine lead time constant, sec.",
			PT_int32,"GGOV1_Fuel_lag",PADDR(gov_ggv1_Flag),PT_DESCRIPTION,"Switch for fuel source characteristic, = 0 for fuel flow independent of speed, = 1 fuel flow proportional to speed",
			PT_double,"GGOV1_Teng",PADDR(gov_ggv1_Teng),PT_DESCRIPTION,"Transport lag time constant for diesel engine",
			PT_double,"GGOV1_Tfload",PADDR(gov_ggv1_Tfload),PT_DESCRIPTION,"Load Limiter time constant, sec. (>0.)",
			PT_double,"GGOV1_Kpload",PADDR(gov_ggv1_Kpload),PT_DESCRIPTION,"Load limiter proportional gain for PI controller",
			PT_double,"GGOV1_Kiload",PADDR(gov_ggv1_Kiload),PT_DESCRIPTION,"Load limiter integral gain for PI controller",
			PT_double,"GGOV1_Ldref[pu]",PADDR(gov_ggv1_Ldref),PT_DESCRIPTION,"Load limiter reference value p.u.",
			PT_double,"GGOV1_Dm[pu]",PADDR(gov_ggv1_Dm),PT_DESCRIPTION,"Speed sensitivity coefficient, p.u.",
			PT_double,"GGOV1_ropen[pu/s]",PADDR(gov_ggv1_ropen),PT_DESCRIPTION,"Maximum valve opening rate, p.u./sec.",
			PT_double,"GGOV1_rclose[pu/s]",PADDR(gov_ggv1_rclose),PT_DESCRIPTION,"Minimum valve closing rate, p.u./sec.",
			PT_double,"GGOV1_Kimw",PADDR(gov_ggv1_Kimw),PT_DESCRIPTION,"Power controller (reset) gain",
			PT_double,"GGOV1_Pmwset[MW]",PADDR(gov_ggv1_Pmwset),PT_DESCRIPTION,"Power controller setpoint, MW",
			PT_double,"GGOV1_aset[pu/s]",PADDR(gov_ggv1_aset),PT_DESCRIPTION,"Acceleration limiter setpoint, p.u. / sec.",
			PT_double,"GGOV1_Ka",PADDR(gov_ggv1_Ka),PT_DESCRIPTION,"Acceleration limiter Gain",
			PT_double,"GGOV1_Ta[s]",PADDR(gov_ggv1_Ta),PT_DESCRIPTION,"Acceleration limiter time constant, sec. (>0.)",
			PT_double,"GGOV1_db",PADDR(gov_ggv1_db),PT_DESCRIPTION,"Speed governor dead band",
			PT_double,"GGOV1_Tsa[s]",PADDR(gov_ggv1_Tsa),PT_DESCRIPTION,"Temperature detection lead time constant, sec.",
			PT_double,"GGOV1_Tsb[s]",PADDR(gov_ggv1_Tsb),PT_DESCRIPTION,"Temperature detection lag time constant, sec.",
//			PT_double,"GGOV1_rup",PADDR(gov_ggv1_rup),PT_DESCRIPTION,"Maximum rate of load limit increase",
//			PT_double,"GGOV1_rdown",PADDR(gov_ggv1_rdown),PT_DESCRIPTION,"Maximum rate of load limit decrease",

			//GGOV1 "enable/disable" variables - to give some better control over low value select
			PT_bool,"GGOV1_Load_Limit_enable",PADDR(gov_ggv1_fsrt_enable),PT_DESCRIPTION,"Enables/disables load limiter (fsrt) of low-value-select",
			PT_bool,"GGOV1_Acceleration_Limit_enable",PADDR(gov_ggv1_fsra_enable),PT_DESCRIPTION,"Enables/disables acceleration limiter (fsra) of low-value-select",
			PT_bool,"GGOV1_PID_enable",PADDR(gov_ggv1_fsrn_enable),PT_DESCRIPTION,"Enables/disables PID controller (fsrn) of low-value-select",

			//GGOV1 state variables
			PT_double,"GGOV1_fsrt",PADDR(curr_state.gov_ggov1.fsrt),PT_DESCRIPTION,"Load limiter block input to low-value-select",
			PT_double,"GGOV1_fsra",PADDR(curr_state.gov_ggov1.fsra),PT_DESCRIPTION,"Acceleration limiter block input to low-value-select",
			PT_double,"GGOV1_fsrn",PADDR(curr_state.gov_ggov1.fsrn),PT_DESCRIPTION,"PID block input to low-value-select",
			PT_double,"GGOV1_speed_error[pu]",PADDR(curr_state.gov_ggov1.werror),PT_DESCRIPTION,"Speed difference in per-unit for input to PID controller",

			//All state variables
			PT_double,"GGOV1_x1",PADDR(curr_state.gov_ggov1.x1),
			PT_double,"GGOV1_x2",PADDR(curr_state.gov_ggov1.x2),
			PT_double,"GGOV1_x2a",PADDR(curr_state.gov_ggov1.x2a),
			PT_double,"GGOV1_x3",PADDR(curr_state.gov_ggov1.x3),
			PT_double,"GGOV1_x3a",PADDR(curr_state.gov_ggov1.x3a),
			PT_double,"GGOV1_x4",PADDR(curr_state.gov_ggov1.x4),
			PT_double,"GGOV1_x4a",PADDR(curr_state.gov_ggov1.x4a),
			PT_double,"GGOV1_x4b",PADDR(curr_state.gov_ggov1.x4b),
			PT_double,"GGOV1_x5",PADDR(curr_state.gov_ggov1.x5),
			PT_double,"GGOV1_x5a",PADDR(curr_state.gov_ggov1.x5a),
			PT_double,"GGOV1_x5b",PADDR(curr_state.gov_ggov1.x5b),
			PT_double,"GGOV1_x6",PADDR(curr_state.gov_ggov1.x6),
			PT_double,"GGOV1_x7",PADDR(curr_state.gov_ggov1.x7),
			PT_double,"GGOV1_x7a",PADDR(curr_state.gov_ggov1.x7a),
			PT_double,"GGOV1_x8",PADDR(curr_state.gov_ggov1.x8),
			PT_double,"GGOV1_x8a",PADDR(curr_state.gov_ggov1.x8a),
			PT_double,"GGOV1_x9",PADDR(curr_state.gov_ggov1.x9),
			PT_double,"GGOV1_x9a",PADDR(curr_state.gov_ggov1.x9a),
			PT_double,"GGOV1_x10",PADDR(curr_state.gov_ggov1.x10),
			PT_double,"GGOV1_x10a",PADDR(curr_state.gov_ggov1.x10a),
			PT_double,"GGOV1_x10b",PADDR(curr_state.gov_ggov1.x10b),
			PT_double,"GGOV1_ValveStroke",PADDR(curr_state.gov_ggov1.ValveStroke),
			PT_double,"GGOV1_FuelFlow",PADDR(curr_state.gov_ggov1.FuelFlow),
			PT_double,"GGOV1_GovOutPut",PADDR(curr_state.gov_ggov1.GovOutPut),
			PT_double,"GGOV1_RselectValue",PADDR(curr_state.gov_ggov1.RselectValue),
			PT_double,"GGOV1_fsrtNoLim",PADDR(curr_state.gov_ggov1.fsrtNoLim),
			PT_double,"GGOV1_err2",PADDR(curr_state.gov_ggov1.err2),
			PT_double,"GGOV1_err2a",PADDR(curr_state.gov_ggov1.err2a),
			PT_double,"GGOV1_err3",PADDR(curr_state.gov_ggov1.err3),
			PT_double,"GGOV1_err4",PADDR(curr_state.gov_ggov1.err4),
			PT_double,"GGOV1_err7",PADDR(curr_state.gov_ggov1.err7),
			PT_double,"GGOV1_LowValSelect1",PADDR(curr_state.gov_ggov1.LowValSelect1),
			PT_double,"GGOV1_LowValSelect",PADDR(curr_state.gov_ggov1.LowValSelect),

			//P_CONSTANT mode properties
			PT_double,"P_CONSTANT_Tpelec[s]",PADDR(pconstant_Tpelec),PT_DESCRIPTION,"Electrical power transducer time constant, sec. (>0.)",
			PT_double,"P_CONSTANT_Tact",PADDR(pconstant_Tact),PT_DESCRIPTION,"Actuator time constant",
			PT_double,"P_CONSTANT_Kturb",PADDR(pconstant_Kturb),PT_DESCRIPTION,"Turbine gain (>0.)",
			PT_double,"P_CONSTANT_wfnl[pu]",PADDR(pconstant_wfnl),PT_DESCRIPTION,"No load fuel flow, p.u",
			PT_double,"P_CONSTANT_Tb[s]",PADDR(pconstant_Tb),PT_DESCRIPTION,"Turbine lag time constant, sec. (>0.)",
			PT_double,"P_CONSTANT_Tc[s]",PADDR(pconstant_Tc),PT_DESCRIPTION,"Turbine lead time constant, sec.",
			PT_double,"P_CONSTANT_Teng",PADDR(pconstant_Teng),PT_DESCRIPTION,"Transport lag time constant for diesel engine",
			PT_double,"P_CONSTANT_ropen[pu/s]",PADDR(pconstant_ropen),PT_DESCRIPTION,"Maximum valve opening rate, p.u./sec.",
			PT_double,"P_CONSTANT_rclose[pu/s]",PADDR(pconstant_rclose),PT_DESCRIPTION,"Minimum valve closing rate, p.u./sec.",
			PT_double,"P_CONSTANT_Kimw",PADDR(pconstant_Kimw),PT_DESCRIPTION,"Power controller (reset) gain",

			// P_CONSTANT mode state variables
			PT_double,"P_CONSTANT_x1",PADDR(curr_state.gov_pconstant.x1),
			PT_double,"P_CONSTANT_x4",PADDR(curr_state.gov_pconstant.x4),
			PT_double,"P_CONSTANT_x4a",PADDR(curr_state.gov_pconstant.x4a),
			PT_double,"P_CONSTANT_x4b",PADDR(curr_state.gov_pconstant.x4b),
			PT_double,"P_CONSTANT_x5",PADDR(curr_state.gov_pconstant.x5),
			PT_double,"P_CONSTANT_x5a",PADDR(curr_state.gov_pconstant.x5a),
			PT_double,"P_CONSTANT_x5b",PADDR(curr_state.gov_pconstant.x5b),
			PT_double,"P_CONSTANT_x_Pconstant",PADDR(curr_state.gov_pconstant.x_Pconstant),
			PT_double,"P_CONSTANT_err4",PADDR(curr_state.gov_pconstant.err4),
			PT_double,"P_CONSTANT_ValveStroke",PADDR(curr_state.gov_pconstant.ValveStroke),
			PT_double,"P_CONSTANT_FuelFlow",PADDR(curr_state.gov_pconstant.FuelFlow),
			PT_double,"P_CONSTANT_GovOutPut",PADDR(curr_state.gov_pconstant.GovOutPut),

			PT_bool,"fuelEmissionCal", PADDR(fuelEmissionCal),  PT_DESCRIPTION, "Boolean value indicating whether fuel and emission calculations are used or not",
			PT_double,"outputEnergy",PADDR(outputEnergy),PT_DESCRIPTION,"Total energy(kWh) output from the generator",
			PT_double,"FuelUse",PADDR(FuelUse),PT_DESCRIPTION,"Total fuel usage (gal) based on kW power output",
			PT_double,"efficiency",PADDR(efficiency),PT_DESCRIPTION,"Total energy output per fuel usage (kWh/gal)",
			PT_double,"CO2_emission",PADDR(CO2_emission),PT_DESCRIPTION,"Total CO2 emissions (lbs) based on fule usage",
			PT_double,"SOx_emission",PADDR(SOx_emission),PT_DESCRIPTION,"Total SOx emissions (lbs) based on fule usage",
			PT_double,"NOx_emission",PADDR(NOx_emission),PT_DESCRIPTION,"Total NOx emissions (lbs) based on fule usage",
			PT_double,"PM10_emission",PADDR(PM10_emission),PT_DESCRIPTION,"Total PM-10 emissions (lbs) based on fule usage",

			PT_double,"frequency_deviation",PADDR(frequency_deviation),PT_DESCRIPTION,"Frequency deviation of diesel_dg",
			PT_double,"frequency_deviation_energy",PADDR(frequency_deviation_energy),PT_DESCRIPTION,"Frequency deviation accumulation of diesel_dg",
			PT_double,"frequency_deviation_max",PADDR(frequency_deviation_max),PT_DESCRIPTION,"Frequency deviation of diesel_dg",
			PT_double,"realPowerChange",PADDR(realPowerChange),PT_DESCRIPTION,"Real power output change of diesel_dg",
			PT_double,"ratio_f_p",PADDR(ratio_f_p),PT_DESCRIPTION,"Ratio of frequency deviation to real power output change of diesel_dg",

			PT_set, "phases", PADDR(phases), PT_DESCRIPTION, "Specifies which phases to connect to - currently not supported and assumes three-phase connection",
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,

			//-- This hides from modehelp -- PT_double,"TD[s]",PADDR(gov_TD),PT_DESCRIPTION,"Governor combustion delay (s)",PT_ACCESS,PA_HIDDEN,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		defaults = this;

		memset(this,0,sizeof(diesel_dg));

		if (gl_publish_function(oclass,	"interupdate_gen_object", (FUNCTIONADDR)interupdate_diesel_dg)==NULL)
			GL_THROW("Unable to publish diesel_dg deltamode function");
		if (gl_publish_function(oclass,	"postupdate_gen_object", (FUNCTIONADDR)postupdate_diesel_dg)==NULL)
			GL_THROW("Unable to publish diesel_dg deltamode function");
	}
}

/* Object creation is called once for each object that is created by the core */
int diesel_dg::create(void) 
{
////Initialize tracking variables

	power_factor = 0.0;

	//End of synchronous generator inputs
	Rated_V_LL = 0.0;
	Rated_V_LN = 0.0;
	Rated_VA = 0.0;

	Overload_Limit_Pub = 1.25;	//By default, let us go 25% over the limit
	Overload_Limit_Value = 0.0;	//Will populate later
	//Arbitrary defaults
	Max_Ef = 1.05;
	Min_Ef = 0.95;

	//Dynamics generator defaults
	omega_ref=2*PI*60;  
	inertia=0.7;              
	damping=0.0;                
	number_poles=2;     
	Ra=0.00625;         
	Xd=2.06;            
	Xq=2.5;             
	Xdp=0.398;          
	Xqp=0.3;            
	Xdpp=0.254;         
	Xqpp=0.254;         
	Xl=0.1;             
	Tdp=0.31737;        
	Tdop=4.45075;       
	Tqop=3.0;           
	Tdopp=0.066;        
	Tqopp=0.075;        
	Ta=0.03202;         
	X0=complex(0.005,0.05);
	X2=complex(0.0072,0.2540);

	//Input variables are initialized to -99 (since pu) - if left there, the dynamics initialization gets them
	gen_base_set_vals.wref = -99.0;
	gen_base_set_vals.vset = -99.0;
	gen_base_set_vals.Pref = -99.0;
	gen_base_set_vals.Qref = -99.0;

	//SEXS Exciter defaults
	exc_KA=50;              
	exc_TA=0.01;            
	exc_TB=2.0;               
	exc_TC=10;              
	exc_EMAX=3.0;             
	exc_EMIN=-3.0;            

	//DEGOV1 Governor defaults
	gov_degov1_R=0.05;             
	gov_degov1_T1=0.2;             
	gov_degov1_T2=0.3;             
	gov_degov1_T3=0.5;             
	gov_degov1_K=0.8;              
	gov_degov1_T4=1.0;               
	gov_degov1_T5=0.1;             
	gov_degov1_T6=0.2;             
	gov_degov1_TMAX=1.0;             
	gov_degov1_TMIN=0.0;             
	gov_degov1_TD=0.01;            

	//GAST Governor defaults
//	gov_gast_R=.05;             
//	gov_gast_T1=0.1;
//	gov_gast_T2=0.05;
	gov_gast_R=.05;             
	gov_gast_T1=0.4;
	gov_gast_T2=0.1;
	gov_gast_T3=3;
	gov_gast_AT=1;
	gov_gast_KT=2;
	gov_gast_VMAX=1.05;
	gov_gast_VMIN=-0.05;
	
	//GGOV1 Governor defaults
	gov_ggv1_r = 0.04;
	gov_ggv1_rselect = 1;
	gov_ggv1_Tpelec = 1.0;
	gov_ggv1_maxerr = 0.05;
	gov_ggv1_minerr = -0.05;
	gov_ggv1_Kpgov = 0.8;
	gov_ggv1_Kigov = 0.2;
	gov_ggv1_Kdgov = 0.0;
	gov_ggv1_Tdgov = 1.0;
	gov_ggv1_vmax = 1.0;
	gov_ggv1_vmin = 0.15;
	gov_ggv1_Tact = 0.5;
	gov_ggv1_Kturb = 1.5;
	gov_ggv1_wfnl = 0.2;
	gov_ggv1_Tb = 0.1;
	gov_ggv1_Tc = 0.0;
	gov_ggv1_Flag = 1;
	gov_ggv1_Teng = 0.0;
	gov_ggv1_Tfload = 3.0;
	gov_ggv1_Kpload = 2.0;
	gov_ggv1_Kiload = 0.67;
	gov_ggv1_Ldref = 1.0;
	gov_ggv1_Dm = 0.0;
	gov_ggv1_ropen = 0.10;
	gov_ggv1_rclose = -0.1;
	gov_ggv1_Kimw = 0.002;
	gov_ggv1_Pmwset = -1;
	gov_ggv1_aset = 0.01;
	gov_ggv1_Ka = 10.0;
	gov_ggv1_Ta = 0.1;
	gov_ggv1_db = 0.0;
	gov_ggv1_Tsa = 4.0;
	gov_ggv1_Tsb = 5.0;
	//gov_ggv1_rup = 99.0;
	//gov_ggv1_rdown = -99.0;

	//P_CONSTANT mode defaults
	pconstant_Tpelec = 1.0;
	pconstant_Tact = 0.5;
	pconstant_Kturb = 1.5;
	pconstant_wfnl = 0.2;
	pconstant_Tb = 0.1;
	pconstant_Tc = 0.0;
	pconstant_Flag = 0;
	pconstant_Teng = 0.0;
	pconstant_ropen = 0.10;
	pconstant_rclose = -0.1;
	pconstant_Kimw = 0.002;

	//By default, all paths enabled
	gov_ggv1_fsrt_enable = true;
	gov_ggv1_fsra_enable = true;
	gov_ggv1_fsrn_enable = true;

	//Other deltamode-variables
	pPGenerated = NULL;
	pIGenerated[0] = pIGenerated[1] = pIGenerated[2] = NULL;
	pbus_full_Y_mat = NULL;
	pbus_full_Y_all_mat = NULL;
	generator_admittance[0][0] = generator_admittance[0][1] = generator_admittance[0][2] = complex(0.0,0.0);
	generator_admittance[1][0] = generator_admittance[1][1] = generator_admittance[1][2] = complex(0.0,0.0);
	generator_admittance[2][0] = generator_admittance[2][1] = generator_admittance[2][2] = complex(0.0,0.0);

	full_bus_admittance_mat[0][0] = full_bus_admittance_mat[0][1] = full_bus_admittance_mat[0][2] = complex(0.0,0.0);
	full_bus_admittance_mat[1][0] = full_bus_admittance_mat[1][1] = full_bus_admittance_mat[1][2] = complex(0.0,0.0);
	full_bus_admittance_mat[2][0] = full_bus_admittance_mat[2][1] = full_bus_admittance_mat[2][2] = complex(0.0,0.0);
	value_IGenerated[0] = value_IGenerated[1] = value_IGenerated[2] = complex(0.0,0.0);
	Governor_type = NO_GOV;
	Exciter_type = NO_EXC;

	power_val[0] = power_val[1] = power_val[2] = complex(0.0,0.0);
	current_val[0] = current_val[1] = current_val[2] = complex(0.0,0.0);

	//Rotor convergence becomes 0.1 rad
	rotor_speed_convergence_criterion = 0.1;
	prev_rotor_speed_val = 0.0;

	//Voltage convergence
	voltage_convergence_criterion = 0.5;
	prev_voltage_val[0] = 0.0;
	prev_voltage_val[1] = 0.0;
	prev_voltage_val[2] = 0.0;

	//By default, only speed convergence is on
	apply_rotor_speed_convergence = true;
	apply_voltage_mag_convergence = false;

	//Working variable zeroing
	power_base = 0.0;
	voltage_base = 0.0;
	current_base = 0.0;
	impedance_base = 0.0;
	YS0 = 0.0;
	YS1 = 0.0;
	YS2 = 0.0;
	Rr = 0.0;

	torque_delay = NULL;
	x5a_delayed = NULL;
	torque_delay_len = 0;
	x5a_delayed_len = 0;

	torque_delay_write_pos = 0;
	torque_delay_read_pos = 0;
	x5a_delayed_write_pos = 0;
	x5a_delayed_read_pos = 0;

	//Initialize desired frequency to 60 Hz
	curr_state.omega = 2*PI*60.0;

	deltamode_inclusive = false;	//By default, don't be included in deltamode simulations
	mapped_freq_variable = NULL;

	first_run = true;				//First time we run, we are the first run (by definition)

	prev_time = 0;
	prev_time_dbl = 0.0;

	is_isochronous_gen = false;	//By default, we're a normal "ugly" generator

	ki_Pconstant = 1;
	kp_Pconstant = 0;

	Q_constant_mode = false;
	ki_Qconstant = 1;
	kp_Qconstant = 0;

	CVRenabled = false;
	ki_cvr = 0;
	kp_cvr = 0;
	kd_cvr = 0;
	CVR_PI = false;
	CVR_PID = false;
	vset_EMAX = 1.05;
	vset_EMIN = 0.95;

	Kd1 = 1;
	Kd2 = 1;
	Kd3 = 1;
	Kn1 = 0;
	Kn2 = 0;
	vset_delta_MAX = 99;
	vset_delta_MIN = -99;

	last_time = 0;
	fuelEmissionCal = false;
	outputEnergy = 0.0;
	FuelUse = 0.0;
	efficiency = 0.0;
	CO2_emission = 0.0;
	SOx_emission = 0.0;
	NOx_emission = 0.0;
	PM10_emission = 0.0;

	pwr_electric_init = -1;
	frequency_deviation_energy = 0;
	frequency_deviation_max = 0;
	//NULL/zero pointers
	pCircuit_V[0] = pCircuit_V[1] = pCircuit_V[2] = NULL;
	pLine_I[0] = pLine_I[1] = pLine_I[2] = NULL;
	pPower[0] = pPower[1] = pPower[2] = NULL;
	value_Circuit_V[0] = value_Circuit_V[1] = value_Circuit_V[2] = complex(0.0,0.0);
	value_Line_I[0] = value_Line_I[1] = value_Line_I[2] = complex(0.0,0.0);
	value_Power[0] = value_Power[1] = value_Power[2] = complex(0.0,0.0);
	value_prev_Power[0] = value_prev_Power[1] = value_prev_Power[2] = complex(0.0,0.0);
	
	parent_is_powerflow = false;	//By default, we're not a good child

	//Overall, force the generator into "PQ mode" first
	Gen_type = NON_DYN_CONSTANT_PQ;

	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int diesel_dg::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	OBJECT *tmp_obj = NULL;

	int temp_idx_x, temp_idx_y;
	double ZB, SB, EB;
	double test_pf;
	gld_property *Frequency_mapped;
	gld_property *temp_property_pointer;
	gld_wlock *test_rlock;
	bool temp_bool_value;
	double temp_voltage_magnitude;
	complex temp_complex_value;
	complex_array temp_complex_array;
	gld_property *pNominal_Voltage;
	double nominal_voltage_value, nom_test_val;
	
	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true;	//Set the flag and off we go
	}

	// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
	if (parent!=NULL)
	{
		if (gl_object_isa(parent,"meter","powerflow") || gl_object_isa(parent,"node","powerflow") || gl_object_isa(parent,"load","powerflow"))
		{
			//Flag us as a proper child
			parent_is_powerflow = true;

			//See if this attached node is a child or not
			if (parent->parent != NULL)
			{
				//Map parent
				tmp_obj = parent->parent;

				//See what it is
				if ((gl_object_isa(tmp_obj,"meter","powerflow") == false) && (gl_object_isa(tmp_obj,"node","powerflow")==false) && (gl_object_isa(tmp_obj,"load","powerflow")==false))
				{
					//Not a wierd map, just use normal parent
					tmp_obj = parent;
				}
				else	//Implies it is a powerflow parent
				{
					//See if we are deltamode-enabled -- if so, flag our parent while we're here
					if (deltamode_inclusive == true)
					{
						//Map our deltamode flag and set it (parent will be done below)
						temp_property_pointer = new gld_property(parent,"Norton_dynamic");

						//Make sure it worked
						if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_bool() != true))
						{
							GL_THROW("diesel_dg:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
							//Defined elsewhere
						}

						//Flag it to true
						temp_bool_value = true;
						temp_property_pointer->setp<bool>(temp_bool_value,*test_rlock);

						//Remove it
						delete temp_property_pointer;
					}
					//Default else -- it is one of those, but not deltamode, so nothing extra to do
				}//End we were a powerflow child
			}
			else	//It is nul
			{
				//Just point it to the normal parent
				tmp_obj = parent;
			}

			//Now do the standard mapping

			//Map the voltage
			pCircuit_V[0] = map_complex_value(tmp_obj,"voltage_A");
			pCircuit_V[1] = map_complex_value(tmp_obj,"voltage_B");
			pCircuit_V[2] = map_complex_value(tmp_obj,"voltage_C");

			//Current gets mapped this way too right now, but that may not be right
			pLine_I[0] = map_complex_value(tmp_obj,"current_A");
			pLine_I[1] = map_complex_value(tmp_obj,"current_B");
			pLine_I[2] = map_complex_value(tmp_obj,"current_C");

			//Map power -- not used by all, but just for the sake of populating
			pPower[0] = map_complex_value(tmp_obj,"power_A");
			pPower[1] = map_complex_value(tmp_obj,"power_B");
			pPower[2] = map_complex_value(tmp_obj,"power_C");

			//Map nominal voltage for populating variables
			//Pull the nominal voltage
			pNominal_Voltage = new gld_property(parent,"nominal_voltage");

			//Make sure it worked
			if ((pNominal_Voltage->is_valid() != true) || (pNominal_Voltage->is_double() != true))
			{
				GL_THROW("diesel_dg:%d %s - Unable to map nominal_voltage from object:%d %s",obj->id,(obj->name ? obj->name : "Unnamed"),parent->id,(parent->name ? parent->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the nominal_voltage from a parent object, an error occurred.  Please try again.
				If the error persists, please submit your system and a bug report via the ticketing system.
				*/
			}

			//Pull the voltage base value
			nominal_voltage_value = pNominal_Voltage->get_double();

			//Now get rid of the item
			delete pNominal_Voltage;

			//Compute the line-neutral value
			Rated_V_LN = Rated_V_LL / sqrt(3.0);

			//See if Rated_V is set
			if (Rated_V_LL > 0.0)	//It is
			{
				//Normalize it by the nominal voltage, for testing - also convert from appropriately
				nom_test_val = Rated_V_LN / nominal_voltage_value;

				//Make sure it matches
				if ((nom_test_val > 1.01) || (nom_test_val < 0.99))
				{
					GL_THROW("diesel_dg:%d %s - Rated_V does not match the nominal voltage!",obj->id,(obj->name ? obj->name : "Unnamed"));
					/*  TROUBLESHOOT
					The value specified in Rated_V does not match the nominal_voltage of the parented node.  Please fix this
					discrepancy.
					*/
				}
			}
			else	//Nope, use the nominal voltage
			{
				//Nominal voltage should be in LN, so convert it to LL
				Rated_V_LL = nominal_voltage_value * sqrt(3.0);

				//Get the LN value too
				Rated_V_LN = nominal_voltage_value;
			}

			//If we were deltamode requesting, set the flag on the other side
			if (deltamode_inclusive==true)
			{
				//Map the current injection variables
				pIGenerated[0] = map_complex_value(tmp_obj,"deltamode_generator_current_A");
				pIGenerated[1] = map_complex_value(tmp_obj,"deltamode_generator_current_B");
				pIGenerated[2] = map_complex_value(tmp_obj,"deltamode_generator_current_C");

				//Map the PGenerated value
				pPGenerated = map_complex_value(tmp_obj,"deltamode_PGenTotal");

				//Map the flag
				temp_property_pointer = new gld_property(tmp_obj,"Norton_dynamic");

				//Make sure it worked
				if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_bool() != true))
				{
					GL_THROW("diesel_dg:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",tmp_obj->name?tmp_obj->name:"unnamed");
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
			}
		}
		else	//Only three-phase node objects supported right now
		{
			GL_THROW("diesel_dg:%s only supports a powerflow node/load/meter or no object as its parent at this time",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has a parent that is not a powerflow node/load/meter object.  At this time, the diesel_dg object can only be parented
			to a powerflow node, load, or meter or not have a parent.
			*/
		}

		//Map and pull the phases
		temp_property_pointer = new gld_property(parent,"phases");

		//Make sure ti worked
		if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_set() != true))
		{
			GL_THROW("Unable to map phases property - ensure the parent is a meter or a node or a load");
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

		if((phases & 0x0007) != 0x0007){//parent does not have all three meters
			GL_THROW("The diesel_dg object must be connected to all three phases. Please make sure the parent object has all three phases.");
			/* TROUBLESHOOT
			The diesel_dg object is a three-phase generator. This message occured because the parent object does not have all three phases.
			Please check and make sure your parent object has all three phases and try again. if the error persists, please submit your code and a bug report via the Trac website.
			*/
		}
	}
	else	//No parent
	{
		//Not a good child -- bad child?
		parent_is_powerflow = false;

		gl_warning("diesel_dg:%d - %s has no parent object - default voltages being used",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The diesel_dg object does not have a proper parent -- default voltage values are used.
		*/

		//Populate the working variables
		value_Circuit_V[0].SetPolar(Rated_V_LN,0.0);
		value_Circuit_V[1].SetPolar(Rated_V_LN,-2.0/3.0*PI);
		value_Circuit_V[2].SetPolar(Rated_V_LN,2.0/3.0*PI);
	}

	//Preliminary check on modes
	if ((Gen_type!=DYNAMIC) && (deltamode_inclusive==true))
	{
		//We're flagged for deltamode, but not in the right mode - force us
		Gen_type=DYNAMIC;

		//Pass a warning
		gl_warning("diesel_dg:%s forced into DYN_SYNCHRONOUS mode",obj->name?obj->name:"unnamed");
		/*  TROUBLESHOOT
		The diesel_dg object had the deltamode_inclusive flag set, but was not of the DYN_SYNCHRONOUS type.
		It has been forced to this type.  If this is not desired, please remove the deltamode_inclusive flag
		or explicitly set it to false.
		*/
	}

	if (Gen_type == NON_DYN_CONSTANT_PQ)
	{
		// Check rated power
		if (Rated_VA <= 0.0)
		{
			Rated_VA = 0.5e6;	//Set to parameter-basis default

			gl_warning("diesel_dg:%s did not have a valid Rated_VA set, assuming 5000 kVA",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The Rated_VA value was not set or was set to a negative number.  It is being forced to 5000 kVA, which is
			the machine base for all of the other default parameter values.
			*/
		}

		// Check given power output in glm file
		power_base = Rated_VA/3.0;

		//Check specified power against per-phase limit (power_base) - impose that for now
		if (power_val[0].Mag()>power_base)
		{
			gl_warning("diesel_dg:%s - power_out_A is above 1/3 the total rating, capping",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has a power_out_A value that is above 1/3 the total rating.  It will be thresholded to
			that level.
			*/

			//Maintain power factor value
			test_pf = power_val[0].Re()/power_val[0].Mag();

			//Form up
			if (power_val[0].Im()<0.0)
				power_val[0] = complex((power_base*test_pf),(-1.0*sqrt(1-test_pf*test_pf)*power_base));
			else
				power_val[0] = complex((power_base*test_pf),(sqrt(1-test_pf*test_pf)*power_base));
		}//End phase A power limit check

		if (power_val[1].Mag()>power_base)
		{
			gl_warning("diesel_dg:%s - power_out_B is above 1/3 the total rating, capping",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has a power_out_B value that is above 1/3 the total rating.  It will be thresholded to
			that level.
			*/

			//Maintain power factor value
			test_pf = power_val[1].Re()/power_val[1].Mag();

			//Form up
			if (power_val[1].Im()<0.0)
				power_val[1] = complex((power_base*test_pf),(-1.0*sqrt(1-test_pf*test_pf)*power_base));
			else
				power_val[1] = complex((power_base*test_pf),(sqrt(1-test_pf*test_pf)*power_base));
		}//End phase B power limit check

		if (power_val[2].Mag()>power_base)
		{
			gl_warning("diesel_dg:%s - power_out_C is above 1/3 the total rating, capping",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has a power_out_C value that is above 1/3 the total rating.  It will be thresholded to
			that level.
			*/

			//Maintain power factor value
			test_pf = power_val[2].Re()/power_val[2].Mag();

			//Form up
			if (power_val[2].Im()<0.0)
				power_val[2] = complex((power_base*test_pf),(-1.0*sqrt(1-test_pf*test_pf)*power_base));
			else
				power_val[2] = complex((power_base*test_pf),(sqrt(1-test_pf*test_pf)*power_base));
		}//End phase C power limit check
	}
	else	//Must be dynamic!
	{
		//Make sure our parent is delta enabled!
		if ((parent->flags & OF_DELTAMODE) != OF_DELTAMODE)
		{
			GL_THROW("diesel_dg:%s - The parented object does not have deltamode flags enabled.",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The parent object for the diesel_dg object does not appear to be flagged for deltamode.  This could cause serious problems
			when it tries to update.  Please either enable deltamode in the parented object, or select a different operating mode for the
			diesel_dg object.
			*/
		}

		//Check rated power
		if (Rated_VA<=0.0)
		{
			Rated_VA = 10e6;	//Set to parameter-basis default

			gl_warning("diesel_dg:%s did not have a valid Rated_VA set, assuming 10 MVA",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The Rated_VA value was not set or was set to a negative number.  It is being forced to 10 MVA, which is
			the machine base for all of the other default parameter values.
			*/
		}

		//Make sure the limit is positive and non-zero
		if (Overload_Limit_Pub <= 0.0)
		{
			//Set to 25% over again, just because
			Overload_Limit_Pub = 1.25;

			//Give a warning
			gl_warning("diesel_dg:%d %s - overload_limit has a value less than or equal to zero - set to 1.25",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The value for overload_limit in diesel_dg must be greater than zero.  If it is not, it will be arbitrarily set to 1.25.
			Explicitly set this value, if this is not intended.
			*/
		}

		//Compute the interval value
		Overload_Limit_Value = Rated_VA * Overload_Limit_Pub;

		//Check voltage value
		if (Rated_V_LL<=0.0)
		{
			Rated_V_LL = 15000.0;	//Set to parameter-basis default

			gl_warning("diesel_dg:%s did not have a valid Rated_V set, assuming 15 kV",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The Rated_V property was not set or was invalid.  It is being forced to 15 kV, which aligns with the base for
			all default parameters.  This will be checked again later to see if it matches the connecting point.  If this
			value is not desired, please set it manually.
			*/
		}

		//Determine machine base values for later
		power_base = Rated_VA/3.0;
		voltage_base = Rated_V_LN;
		current_base = power_base / voltage_base;
		impedance_base = voltage_base / current_base;

		//Scale up the impedances appropriately
		YS0 = complex(1.0)/(X0*impedance_base);					//Zero sequence impedance - scaled (not p.u.)
		YS1 = complex(1.0)/(complex(Ra,Xdpp)*impedance_base);	//Positive sequence impedance - scaled (not p.u.)
		YS2 = complex(1.0)/(X2*impedance_base);					//Negative sequence impedance - scaled (not p.u.)

		//Calculate our initial admittance matrix
		convert_Ypn0_to_Yabc(YS0,YS1,YS2, &generator_admittance[0][0]);

		//Compute other constant terms
		Rr = 2.0*(X2.Re()-Ra);

		//If we're deltamode-enabled and parented to a meter - post the admittance up
		if ((deltamode_inclusive == true) && (parent_is_powerflow == true))
		{
			//Map up the admittance matrix to apply our contributions
			pbus_full_Y_mat = new gld_property(parent,"deltamode_full_Y_matrix");

			//Check it
			if ((pbus_full_Y_mat->is_valid() != true) || (pbus_full_Y_mat->is_complex_array() != true))
			{
				GL_THROW("diesel_dg:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
				//Defined above
			}

			//Pull down the variable
			pbus_full_Y_mat->getp<complex_array>(temp_complex_array,*test_rlock);

			//See if it is valid
			if (temp_complex_array.is_valid(0,0) != true)
			{
				//Create it
				temp_complex_array.grow_to(3,3);

				//Zero it, by default
				for (temp_idx_x=0; temp_idx_x<3; temp_idx_x++)
				{
					for (temp_idx_y=0; temp_idx_y<3; temp_idx_y++)
					{
						temp_complex_array.set_at(temp_idx_x,temp_idx_y,complex(0.0,0.0));
					}
				}
			}
			else	//Already populated, make sure it is the right size!
			{
				if ((temp_complex_array.get_rows() != 3) && (temp_complex_array.get_cols() != 3))
				{
					GL_THROW("diesel_dg:%s exposed Norton-equivalent matrix is the wrong size!",obj->name?obj->name:"unnamed");
					/*  TROUBLESHOOT
					While mapping to an admittance matrix on the parent node device, it was found it is the wrong size.
					Please try again.  If the error persists, please submit your code and model via the issue tracking system.
					*/
				}
				//Default else -- right size
			}

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
				}
			}

			//Push it back up
			pbus_full_Y_mat->setp<complex_array>(temp_complex_array,*test_rlock);

			//Map the full version needed later
			//Map up the admittance matrix to apply our contributions
			pbus_full_Y_all_mat = new gld_property(parent,"deltamode_full_Y_all_matrix");

			//Check it
			if ((pbus_full_Y_all_mat->is_valid() != true) || (pbus_full_Y_all_mat->is_complex_array() != true))
			{
				GL_THROW("diesel_dg:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
				//Defined above
			}
		}//End powerflow deltamode - generator admittance mapping

		// If P_CONSTANT mode, change power_val based on given P_CONSTANT_Pref value
		if (Governor_type == P_CONSTANT) {
			for (int i = 0; i < 3; i++) {
				power_val[i].Re() = Rated_VA * gen_base_set_vals.Pref/3;
			}
		}
		if (Q_constant_mode == true) {
			for (int i = 0; i < 3; i++) {
				power_val[i].Im() = Rated_VA * gen_base_set_vals.Qref/3;
			}
		}

		//Check specified power against per-phase limit (power_base) - impose that for now
		if (power_val[0].Mag()>power_base)
		{
			gl_warning("diesel_dg:%s - power_out_A is above 1/3 the total rating, capping",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has a power_out_A value that is above 1/3 the total rating.  It will be thresholded to
			that level.
			*/

			//Maintain power factor value
			test_pf = power_val[0].Re()/power_val[0].Mag();

			//Form up
			if (power_val[0].Im()<0.0)
				power_val[0] = complex((power_base*test_pf),(-1.0*sqrt(1-test_pf*test_pf)*power_base));
			else
				power_val[0] = complex((power_base*test_pf),(sqrt(1-test_pf*test_pf)*power_base));
		}//End phase A power limit check

		if (power_val[1].Mag()>power_base)
		{
			gl_warning("diesel_dg:%s - power_out_B is above 1/3 the total rating, capping",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has a power_out_B value that is above 1/3 the total rating.  It will be thresholded to
			that level.
			*/

			//Maintain power factor value
			test_pf = power_val[1].Re()/power_val[1].Mag();

			//Form up
			if (power_val[1].Im()<0.0)
				power_val[1] = complex((power_base*test_pf),(-1.0*sqrt(1-test_pf*test_pf)*power_base));
			else
				power_val[1] = complex((power_base*test_pf),(sqrt(1-test_pf*test_pf)*power_base));
		}//End phase B power limit check

		if (power_val[2].Mag()>power_base)
		{
			gl_warning("diesel_dg:%s - power_out_C is above 1/3 the total rating, capping",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has a power_out_C value that is above 1/3 the total rating.  It will be thresholded to
			that level.
			*/

			//Maintain power factor value
			test_pf = power_val[2].Re()/power_val[2].Mag();

			//Form up
			if (power_val[2].Im()<0.0)
				power_val[2] = complex((power_base*test_pf),(-1.0*sqrt(1-test_pf*test_pf)*power_base));
			else
				power_val[2] = complex((power_base*test_pf),(sqrt(1-test_pf*test_pf)*power_base));
		}//End phase C power limit check

		//Check for zeros - if any are zero, 50% them (real generator, arbitrary)
		if (power_val[0].Mag() == 0.0)
		{
			gl_warning("diesel_dg:%s - power_out_A is zero - arbitrarily setting to 50%%",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has a power_out_A value that is zero.  This can cause the generator to never
			partake in the powerflow.  It is being arbitrarily set to 50% of the per-phase rating.  If this is
			undesired, please change the value.
			*/

			power_val[0] = complex(0.5*power_base,0.0);
		}

		if (power_val[1].Mag() == 0.0)
		{
			gl_warning("diesel_dg:%s - power_out_B is zero - arbitrarily setting to 50%%",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has a power_out_B value that is zero.  This can cause the generator to never
			partake in the powerflow.  It is being arbitrarily set to 50% of the per-phase rating.  If this is
			undesired, please change the value.
			*/

			power_val[1] = complex(0.5*power_base,0.0);
		}

		if (power_val[2].Mag() == 0.0)
		{
			gl_warning("diesel_dg:%s - power_out_C is zero - arbitrarily setting to 50%%",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has a power_out_C value that is zero.  This can cause the generator to never
			partake in the powerflow.  It is being arbitrarily set to 50% of the per-phase rating.  If this is
			undesired, please change the value.
			*/

			power_val[2] = complex(0.5*power_base,0.0);
		}

		if (apply_rotor_speed_convergence == true)
		{
		//Check if the convergence criterion is proper
		if (rotor_speed_convergence_criterion<0.0)
		{
			gl_warning("diesel_dg:%s - rotor_speed_convergence is less than zero - negating",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The value specified for deltamode convergence, rotor_speed_convergence, is a negative value.
			It has been made positive.
			*/

			rotor_speed_convergence_criterion = -rotor_speed_convergence_criterion;
		}
		else if (rotor_speed_convergence_criterion==0.0)
		{
			gl_warning("diesel_dg:%s - rotor_speed_convergence is zero - it may never exit deltamode!",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			A zero value has been specified as the deltamode convergence criterion for rotor speed.  This is an incredibly tight
			tolerance and may result in the system never converging and exiting deltamode.
			*/
		}
		//defaulted else, must be okay (well, at the very least, not completely wrong)

			//See if we're an isochronous generator too -- that will be used for deltamode convergence
			switch(Governor_type) {
				case NO_GOV:	//No governor
					{
						break;	//Just get us outta here
					}
				case DEGOV1:
					{
						//See if the droop is set appropriately
						if (gov_degov1_R == 0.0)
						{
							is_isochronous_gen = true;
						}
						break;
					}
				case GAST:
					{
						//See if we're an isoch
						if (gov_gast_R == 0.0)
						{
							is_isochronous_gen = true;
						}
						break;
					}
				case GGOV1_OLD:	//GGOV1_OLD uses the same parameter space as GGOV1
				case GGOV1:
					{
						//See if it is set up as a proper isochronous
						if ((gov_ggv1_r == 0.0) && (gov_ggv1_rselect==0))
						{
							is_isochronous_gen = true;
						}
						break;
					}
				default:	//How'd we get here?
					{
						//Could put an error here, but just skip out -- just means we're not an isoch, no matter what
						break;
					}
				}	//switch end
		}//Rotor speed check end

		//Check voltage convergence criterion as well
		if (apply_voltage_mag_convergence == true)
		{
			//See if the exciter is enabled
			if (Exciter_type == NO_EXC)
			{
				gl_warning("diesel_dg:%s - voltage convergence is enabled, but no exciter is present",(obj->name ? obj->name : "unnamed"));
				/*  TROUBLESHOOT
				While performing simple checks on the voltage convergence criterion, no exciter is turned on.  This convergence check
				does nothing in this situation -- it requires an exciter to function
				*/
			}

			//Check if the convergence criterion is proper
			if (voltage_convergence_criterion<0.0)
			{
				gl_warning("diesel_dg:%s - voltage_convergence is less than zero - negating",obj->name?obj->name:"unnamed");
				/*  TROUBLESHOOT
				The value specified for deltamode convergence, voltage_convergence, is a negative value.
				It has been made positive.
				*/

				voltage_convergence_criterion = -voltage_convergence_criterion;
			}
			else if (voltage_convergence_criterion==0.0)
			{
				gl_warning("diesel_dg:%s - voltage_convergence is zero - it may never exit deltamode!",obj->name?obj->name:"unnamed");
				/*  TROUBLESHOOT
				A zero value has been specified as the deltamode convergence criterion for voltage magnitude.  This is an incredibly tight
				tolerance and may result in the system never converging and exiting deltamode.
				*/
			}
			//defaulted else, must be okay (well, at the very least, not completely wrong)
		}

		//Make sure min is above zero
		if ((Min_Ef<=0.0) && (Exciter_type != NO_EXC))
		{
			GL_THROW("diesel_dg:%s - Vterm_min is less than or equal to zero",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The minimum (p.u.) terminal voltage for the generator with an AVR is less than or equal to zero.
			Please specify a positive value and try again.
			*/
		}

		//Check Max
		if ((Max_Ef<=Min_Ef) && (Exciter_type != NO_EXC))
		{
			GL_THROW("diesel_dg:%s - Vterm_max is less than or equal to Vterm_min",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The maximum (p.u.) terminal voltage for the generator with an AVR is less than or equal to the minmum
			band value.  It must be a higher value.  Please set it to a larger per-unit value and try again.
			*/
		}
		//TODO: Additional comparisons?
	}

	//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (enable_subsecond_models!=true)
		{
			gl_warning("diesel_dg:%s indicates it wants to run deltamode, but the module-level flag is not set!",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			//Perform the mapping check for frequency variable -- if no one has elected yet, we become master of frequency
			//Temporary deltamode workarond until elec_frequency object is complete
			Frequency_mapped = NULL;

			//Get linking to checker variable
			Frequency_mapped = new gld_property("powerflow::master_frequency_update");

			//See if it worked
			if ((Frequency_mapped->is_valid() != true) || (Frequency_mapped->is_bool() != true))
			{
				GL_THROW("diesel_dg:%s - Failed to map frequency checking variable from powerflow for deltamode",obj->name?obj->name:"unnamed");
				/*  TROUBLESHOOT
				While attempting to map one of the electrical frequency update variables from the powerflow module, an error
				was encountered.  Please try again, insuring the diesel_dg is parented to a deltamode powerflow object.  If
				the error persists, please submit your code and a bug report via the ticketing system.
				*/
			}

			//Pull the value
			Frequency_mapped->getp<bool>(temp_bool_value,*test_rlock);
			
			//Check the value
			if (temp_bool_value == false)	//No one has mapped yet, we are volunteered
			{
				//Update powerflow frequency
				mapped_freq_variable = new gld_property("powerflow::current_frequency");

				//Make sure it worked
				if ((mapped_freq_variable->is_valid() != true) || (mapped_freq_variable->is_double() != true))
				{
					GL_THROW("diesel_dg:%s - Failed to map frequency checking variable from powerflow for deltamode",obj->name?obj->name:"unnamed");
					//Defined above
				}

				//Flag the frequency mapping as having occurred
				temp_bool_value = true;
				Frequency_mapped->setp<bool>(temp_bool_value,*test_rlock);
			}
			//Default else -- someone else is already mapped, just continue onward

			//Delete the reference
			delete Frequency_mapped;
			
			gen_object_count++;	//Increment the counter
		}
	}//End deltamode inclusive
	else	//Not enabled for this model
	{
		if (enable_subsecond_models == true)
		{
			GL_THROW("diesel_dg:%d %s - Deltamode is enabled for the module, but not this generator!",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The diesel_dg is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  This will cause issues when
			the simulation executes, due to missing variables.  It is recommend deltamode be enabled for this object, or a different operating mode utilized.
			It is recommended all objects that support deltamode enable it.
			*/
		}
	}

	// Check if base set points for the various control objects are defined in glm file or not
	if (gen_base_set_vals.vset < -90) {
		Vset_defined = false;
	}
	else {
		Vset_defined = true;
		Vref = gen_base_set_vals.vset;
	}

	if (Kd1 == 0) {
		if (Kd2 == 0) {
			gl_warning("diesel_dg:%d %s - cannot set both Kd1 and Kd2 as 0 for the CVR conntrol! Have changed Kd2 to be 1",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The diesel_dg is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this generator may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
		Kd2 = 1;
	}

	// Initialize fuel usage function based on Rated_VA value
	/* For 1000 kVA generator, the fuel usage equation is:
	 % x = load (kVA), y = fuel (gallon)
	 % y = 0.067x + 5.2435
	 */
	dg_1000_a = 0.067;
	dg_1000_b = 5.2435/1000 * (Rated_VA/1000);

	return 1;
}//init ends here

// Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP diesel_dg::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	//Does nothing right now - presync not part of the sync list for this object
	return TS_NEVER; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

TIMESTAMP diesel_dg::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	double tdiff, ang_diff;
	complex temp_current_val[3];
	complex temp_voltage_val[3];
	complex rotate_value;
	TIMESTAMP tret_value;
	double vdiff;
	double voltage_mag_curr;
	double real_diff;     // Temporary variable representing difference between reference real power and actual real power output
	double reactive_diff; // Temporary variable representing difference between reference reactive power and actual reactive power output
	complex temp_power_val[3];
	complex temp_complex_value_power;
	gld_wlock *test_rlock;

	//Assume always want TS_NEVER
	tret_value = TS_NEVER;

	//Reset the poweflow interfaces
	reset_powerflow_accumulators();

	//First run allocation - in diesel_dg for now, but may need to move elsewhere
	if (first_run == true)	//First run
	{
		//TODO: LOCKING!
		if (deltamode_inclusive && enable_subsecond_models && (torque_delay==NULL))	//We want deltamode - see if it's populated yet
		{
			if (((gen_object_current == -1) || (delta_objects==NULL)) && (enable_subsecond_models == true))
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

			//Update pointer
			gen_object_current++;

			//See if we're attached to a node-esque object
			if (obj->parent != NULL)
			{
				if (gl_object_isa(obj->parent,"meter","powerflow") || gl_object_isa(obj->parent,"load","powerflow") || gl_object_isa(obj->parent,"node","powerflow") || gl_object_isa(obj->parent,"elec_frequency","powerflow"))
				{
					//Accumulate and pass our starting power
					temp_complex_value_power = power_val[0] + power_val[1] + power_val[2];

					//Push it up
					pPGenerated->setp<complex>(temp_complex_value_power,*test_rlock);

				}//End parent is a node object
				else	//Nope, so who knows what is going on - better fail, just to be safe
				{
					GL_THROW("diesel_dg:%s - invalid parent object:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
					/*  TROUBLESHOOT
					At this time, for proper dynamic functionality a diesel_dg object must be parented to a three-phase powerflow node
					object (node, load, meter).  The parent object is not one of those objects.
					*/
				}
			}//End non-null parent

			//Initialize a governor array - just so calls can be made to init dynamics easier
			torque_delay_len=1;

			//Now set it up
			torque_delay = (double *)gl_malloc(torque_delay_len*sizeof(double));

			//Make sure it worked
			if (torque_delay == NULL)
			{
				gl_error("diesel_dg: failed to allocate to allocate the delayed torque array for DEGOV1!");
				//Define below

				return TS_INVALID;
			}

			//Initialize the trackers
			torque_delay_write_pos = 0;
			torque_delay_read_pos = 0;

			//Force us to reiterate one
			tret_value = t1;
		}//End deltamode specials - first pass
		//Default else - no deltamode stuff
	}//End first timestep

	//Existing code retained - kept as "not dynamic"
	if (Gen_type == NON_DYN_CONSTANT_PQ)
	{
		// Assign the power output from diesel_dg to its parent node
		// Note that value_prev_Power is the positive value of power_val (from prev) - so the -(-) = +
		value_Power[0] = -power_val[0] + value_prev_Power[0];
		value_Power[1] = -power_val[1] + value_prev_Power[1];
		value_Power[2] = -power_val[2] + value_prev_Power[2];

		//Update the accumulator tracker
		value_prev_Power[0] = power_val[0];
		value_prev_Power[1] = power_val[1];
		value_prev_Power[2] = power_val[2];
	}
	else if (Gen_type == DYNAMIC)	//Synchronous dynamic machine
	{
		//Only do updates if this is a new timestep
		if ((prev_time < t1) && (first_run == false))
		{
			//Pull the current powerflow values down
			pull_powerflow_values();

			//Get time difference
			tdiff = (double)(t1)-prev_time_dbl;

			//Calculate rotor angle update
			ang_diff = (curr_state.omega - omega_ref)*tdiff;
			curr_state.rotor_angle += ang_diff;

			//Figure out the rotation to the existing values
			rotate_value = complex_exp(ang_diff);

			//Apply to voltage - See if this breaks stuff
			value_Circuit_V[0] = value_Circuit_V[0]*rotate_value;
			value_Circuit_V[1] = value_Circuit_V[1]*rotate_value;
			value_Circuit_V[2] = value_Circuit_V[2]*rotate_value;

			//Do a voltage push, since this originally did
			push_powerflow_values(true);

			//Rotate the current injection too, otherwise it may "undo" this
			value_IGenerated[0] = value_IGenerated[0]*rotate_value;
			value_IGenerated[1] = value_IGenerated[1]*rotate_value;
			value_IGenerated[2] = value_IGenerated[2]*rotate_value;

			if (Governor_type == P_CONSTANT) {
				//Figure out P difference based on given Pref
				real_diff = (gen_base_set_vals.Pref - (power_val[0].Re() + power_val[1].Re() + power_val[2].Re()) / Rated_VA) / 3.0;
				real_diff = real_diff * Rated_VA;

				//Copy in value
				temp_power_val[0] = power_val[0] + complex(real_diff, 0.0);
				temp_power_val[1] = power_val[1] + complex(real_diff, 0.0);
				temp_power_val[2] = power_val[2] + complex(real_diff, 0.0);

				//Back out the current injection
				temp_current_val[0] = ~(temp_power_val[0]/value_Circuit_V[0]) + generator_admittance[0][0]*value_Circuit_V[0] + generator_admittance[0][1]*value_Circuit_V[1] + generator_admittance[0][2]*value_Circuit_V[2];
				temp_current_val[1] = ~(temp_power_val[1]/value_Circuit_V[1]) + generator_admittance[1][0]*value_Circuit_V[0] + generator_admittance[1][1]*value_Circuit_V[1] + generator_admittance[1][2]*value_Circuit_V[2];
				temp_current_val[2] = ~(temp_power_val[2]/value_Circuit_V[2]) + generator_admittance[2][0]*value_Circuit_V[0] + generator_admittance[2][1]*value_Circuit_V[1] + generator_admittance[2][2]*value_Circuit_V[2];

				//Apply and see what happens
				value_IGenerated[0] = temp_current_val[0];
				value_IGenerated[1] = temp_current_val[1];
				value_IGenerated[2] = temp_current_val[2];

				//Keep us here
				tret_value = t1;
			}

			//Update time
			prev_time = t1;
			prev_time_dbl = (double)(t1);

			//Compute our current voltage point - see if we need to adjust things (if we have an AVR)
			if (Exciter_type == SEXS)
			{
				//Compute our current voltage point (pos_sequence)
				convert_abc_to_pn0(&value_Circuit_V[0],&temp_voltage_val[0]);

				//Get the positive sequence magnitude
				voltage_mag_curr = temp_voltage_val[0].Mag()/voltage_base;

				if (Q_constant_mode == true) {
					//Figure out Q difference based on given Qref
					reactive_diff = (gen_base_set_vals.Qref - (power_val[0].Im() + power_val[1].Im() + power_val[2].Im()) / Rated_VA) / 3.0;
					reactive_diff = reactive_diff * Rated_VA;

					//Copy in value
					temp_power_val[0] = power_val[0] + complex(real_diff,reactive_diff);
					temp_power_val[1] = power_val[1] + complex(real_diff,reactive_diff);
					temp_power_val[2] = power_val[2] + complex(real_diff,reactive_diff);

					//Back out the current injection
					temp_current_val[0] = ~(temp_power_val[0]/value_Circuit_V[0]) + generator_admittance[0][0]*value_Circuit_V[0] + generator_admittance[0][1]*value_Circuit_V[1] + generator_admittance[0][2]*value_Circuit_V[2];
					temp_current_val[1] = ~(temp_power_val[1]/value_Circuit_V[1]) + generator_admittance[1][0]*value_Circuit_V[0] + generator_admittance[1][1]*value_Circuit_V[1] + generator_admittance[1][2]*value_Circuit_V[2];
					temp_current_val[2] = ~(temp_power_val[2]/value_Circuit_V[2]) + generator_admittance[2][0]*value_Circuit_V[0] + generator_admittance[2][1]*value_Circuit_V[1] + generator_admittance[2][2]*value_Circuit_V[2];

					//Apply and see what happens
					value_IGenerated[0] = temp_current_val[0];
					value_IGenerated[1] = temp_current_val[1];
					value_IGenerated[2] = temp_current_val[2];

					//Keep us here
					tret_value = t1;
				}

				if ((voltage_mag_curr>Max_Ef) || (voltage_mag_curr<Min_Ef))
				{

					//See where the value is
					vdiff = temp_voltage_val[0].Mag()/voltage_base - gen_base_set_vals.vset;

					//Figure out Q difference
					reactive_diff = (YS1_Full.Im()*(vdiff*voltage_base)*voltage_base)/3.0;

					//Copy in value
					temp_power_val[0] = power_val[0] + complex(0.0,reactive_diff);
					temp_power_val[1] = power_val[1] + complex(0.0,reactive_diff);
					temp_power_val[2] = power_val[2] + complex(0.0,reactive_diff);

					//Back out the current injection
					temp_current_val[0] = ~(temp_power_val[0]/value_Circuit_V[0]) + generator_admittance[0][0]*value_Circuit_V[0] + generator_admittance[0][1]*value_Circuit_V[1] + generator_admittance[0][2]*value_Circuit_V[2];
					temp_current_val[1] = ~(temp_power_val[1]/value_Circuit_V[1]) + generator_admittance[1][0]*value_Circuit_V[0] + generator_admittance[1][1]*value_Circuit_V[1] + generator_admittance[1][2]*value_Circuit_V[2];
					temp_current_val[2] = ~(temp_power_val[2]/value_Circuit_V[2]) + generator_admittance[2][0]*value_Circuit_V[0] + generator_admittance[2][1]*value_Circuit_V[1] + generator_admittance[2][2]*value_Circuit_V[2];

					//Apply and see what happens
					value_IGenerated[0] = temp_current_val[0];
					value_IGenerated[1] = temp_current_val[1];
					value_IGenerated[2] = temp_current_val[2];

					//Keep us here
					tret_value = t1;
				}
				//Default else - do nothing
			}
			//Default else - no AVR
		}
		//Nothing else in here right now....all handled internal to powerflow
	}//End synchronous dynamics-enabled generator
	else
	{
		GL_THROW("diesel_dg:%d %s - Unknown Gen_type specified!",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		An unsupported Gen_type was somehow specified in the diesel_dg.  Fix this, and try again.
		*/
	}

	//Push the various accumulator values up
	push_powerflow_values(false);

	return tret_value;
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP diesel_dg::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	complex temp_current_val[3];
	int ret_state;
	OBJECT *obj = OBJECTHDR(this);
	complex aval, avalsq;
	TIMESTAMP dt;
	complex_array temp_complex_array;
	int index_x, index_y;
	gld_wlock *test_rlock;

	TIMESTAMP t2 = TS_NEVER;

	//Reset the powerflow interface values, for giggles
	reset_powerflow_accumulators();

	if (Gen_type == DYNAMIC)
	{
		//Update global, if necessary - assume everyone grabbed by sync
		if (deltamode_endtime != TS_NEVER)
		{
			deltamode_endtime = TS_NEVER;
			deltamode_endtime_dbl = TSNVRDBL;
		}

		//Update the powerflow variables
		pull_powerflow_values();

		// Update energy, fuel usage, and emissions for the past time step, before updating power output
		if (fuelEmissionCal == true) {

			if (first_run == true)
			{
				dt = 0;
			}
			else if (last_time == 0)
			{
				last_time = t1;
				dt = 0;
			}
			else if (last_time < t1)
			{
				dt = t1 - last_time;
				last_time = t1;
			}
			else
				dt = 0;

			outputEnergy += fabs(curr_state.pwr_electric.Re()/1000) * (double)dt / 3600;
			FuelUse += (fabs(curr_state.pwr_electric.Re()/1000) * dg_1000_a + dg_1000_b) * (double)dt / 3600;
			if (FuelUse != 0) {
				efficiency = outputEnergy/FuelUse;
			}
			CO2_emission += (-6e-5 * pow(FuelUse, 3) + 0.0087 * pow(FuelUse, 2) - FuelUse * 0.3464 + 25.824) * (double)dt / 3600;
			SOx_emission += (-5e-7 * pow(FuelUse, 2) + FuelUse * 0.0001 + 0.0206) * (double)dt / 3600;
			NOx_emission += (6e-5 * pow(FuelUse, 2) - FuelUse * 0.0048 + 0.2551) * (double)dt / 3600;
			PM10_emission += (-2e-9 * pow(FuelUse, 4) + 3e-7 * pow(FuelUse, 3) - 2e-5 * pow(FuelUse, 2) + FuelUse * 8e-5 + 0.0083) * (double)dt / 3600;

			if (pwr_electric_init <= 0) {
				pwr_electric_init = curr_state.pwr_electric.Re();
			}
		}

		//Update output power
		//Get current injected
		temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0] - generator_admittance[0][1]*value_Circuit_V[1] - generator_admittance[0][2]*value_Circuit_V[2]);
		temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0]*value_Circuit_V[0] - generator_admittance[1][1]*value_Circuit_V[1] - generator_admittance[1][2]*value_Circuit_V[2]);
		temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0]*value_Circuit_V[0] - generator_admittance[2][1]*value_Circuit_V[1] - generator_admittance[2][2]*value_Circuit_V[2]);

		//Update power output variables, just so we can see what is going on
		power_val[0] = value_Circuit_V[0]*~temp_current_val[0];
		power_val[1] = value_Circuit_V[1]*~temp_current_val[1];
		power_val[2] = value_Circuit_V[2]*~temp_current_val[2];

		//Update the output power variable
		curr_state.pwr_electric = power_val[0] + power_val[1] + power_val[2];
	}

	if (first_run == true)	//Final init items - namely deltamode supersecond exciter
	{
		if (deltamode_inclusive && enable_subsecond_models && (torque_delay!=NULL)) 	//Still "first run", but at least one powerflow has completed (call init dyn now)
		{
			ret_state = init_dynamics(&curr_state);

			if (ret_state == FAILED)
			{
				GL_THROW("diesel_dg:%s - unsuccessful call to dynamics initialization",(obj->name?obj->name:"unnamed"));
				/*  TROUBLESHOOT
				While attempting to call the dynamics initialization function of the diesel_dg object, a failure
				state was encountered.  See other error messages for further details.
				*/
			}

			//Compute the AVR-related admittance - convert to positive sequence value first
			//Constants
			aval = complex(cos(2.0*PI/3.0),sin(2.0*PI/3.0));
			avalsq = aval*aval;

			//Pull in the current version of full_Y_all
			pbus_full_Y_all_mat->getp<complex_array>(temp_complex_array,*test_rlock);

			//Make sure it is the right size -- if so, pull it
			if ((temp_complex_array.get_rows() == 3) && (temp_complex_array.get_cols() == 3))
			{
				//Push it into the matrix for "ease of access"
				for (index_x=0; index_x<3; index_x++)
				{
					for (index_y=0; index_y<3; index_y++)
					{
						full_bus_admittance_mat[index_x][index_y] = temp_complex_array.get_at(index_x,index_y);
					}
				}
			}
			//Default else -- it's not the right size, so leave the existing matrix (even if it is zero)

			//Perform the conversion
			YS1_Full = full_bus_admittance_mat[0][0]+aval*full_bus_admittance_mat[1][0]+avalsq*full_bus_admittance_mat[2][0];
			YS1_Full += avalsq*full_bus_admittance_mat[0][1]+full_bus_admittance_mat[1][1]+aval*full_bus_admittance_mat[2][1];
			YS1_Full += aval*full_bus_admittance_mat[0][2]+avalsq*full_bus_admittance_mat[1][2]+full_bus_admittance_mat[2][2];
			YS1_Full /= 3.0;

		}//End "first run" paired
		//Default else - not dynamics-oriented, deflag
		
		//Deflag us
		first_run = false;
	}

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//Map Complex value
gld_property *diesel_dg::map_complex_value(OBJECT *obj, char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if ((pQuantity->is_valid() != true) || (pQuantity->is_complex() != true))
	{
		GL_THROW("diesel_dg:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in diesel_dg.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Map double value
gld_property *diesel_dg::map_double_value(OBJECT *obj, char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if ((pQuantity->is_valid() != true) || (pQuantity->is_double() != true))
	{
		GL_THROW("diesel_dg:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in diesel_dg.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Function to pull the various powerflow gld_property values into their storage arrays
void diesel_dg::pull_powerflow_values(void)
{
	int indexval;

	//See if we're a proper child -- otherwise, skip all this
	if (parent_is_powerflow == true)
	{
		for (indexval=0; indexval<3; indexval++)
		{
			//Pull the voltage down
			value_Circuit_V[indexval] = pCircuit_V[indexval]->get_complex();

			//Deltamode accumulators
			if (deltamode_inclusive == true)
			{
				//Update IGenerated, in case the powerflow is overriding it
				value_IGenerated[indexval] = pIGenerated[indexval]->get_complex();
			}
		}
	}
	//Default else -- do nothing
}

//Function to push/update the accumulators associated with the powerflow
//Flag to update voltages, since those are usually a pull value (only does that
void diesel_dg::push_powerflow_values(bool update_voltage)
{
	complex temp_complex_val;
	gld_wlock *test_rlock;
	int indexval;

	//See if we're proper first
	if (parent_is_powerflow == true)
	{
		//See what kind of object we are
		if (Gen_type == NON_DYN_CONSTANT_PQ)
		{
			//Post the power
			for (indexval=0; indexval<3; indexval++)
			{
				//**** Pure Power Value ***/
				//Pull the power value
				temp_complex_val = pPower[indexval]->get_complex();

				//Add the value
				temp_complex_val += value_Power[indexval];

				//Push it back up
				pPower[indexval]->setp<complex>(temp_complex_val,*test_rlock);
			}
		}
		else if (Gen_type == DYNAMIC)
		{
			if (update_voltage == true)
			{
				//Loop through the three-phases/accumulators
				for (indexval=0; indexval<3; indexval++)
				{
					//**** push voltage value -- not an accumulator, just force ****/
					pCircuit_V[indexval]->setp<complex>(value_Circuit_V[indexval],*test_rlock);
				}
			}
			else	//Standard update
			{
				//Loop through the three-phases/accumulators
				for (indexval=0; indexval<3; indexval++)
				{
					//**** Pure Current value ***/
					//Pull current value again, just in case
					temp_complex_val = pLine_I[indexval]->get_complex();

					//Add the difference
					temp_complex_val += value_Line_I[indexval];

					//Push it back up
					pLine_I[indexval]->setp<complex>(temp_complex_val,*test_rlock);

					//Update dynamic variables
					if (deltamode_inclusive == true)
					{
						//**** Pre-rotated current injection value ***/
						//This is a direct write - not an accumulator
						pIGenerated[indexval]->setp<complex>(value_IGenerated[indexval],*test_rlock);
					}					
				}
			}
		}//End synchronous
		//Default else - some other type, so skip for now
	}
	//Default else -- don't do anything
}

//Function to reset the accumulators/temp variables for powerflow
void diesel_dg::reset_powerflow_accumulators(void)
{
	int indexval;

	//Loop through and zero them
	for (indexval=0; indexval<3; indexval++)
	{
		//pLine_I values
		value_Line_I[indexval] = complex(0.0,0.0);
	}
}

//Converts the admittance terms from sequence (pn0) to three-phase (abc)
//Asumes output matrix is a 3x3 declaration
//Inputs are Y0 - zero sequence admittance
//			 Y1 - positive sequence	admittance
//			 Y2 - negative sequence admittance
void diesel_dg::convert_Ypn0_to_Yabc(complex Y0, complex Y1, complex Y2, complex *Yabcmat)
{
	complex aval, aval_sq;

	//Define the "transformation" term (1@120deg)
	aval = complex(cos(2.0*PI/3.0),sin(2.0*PI/3.0));
	
	//Make the square, since we'll need it a few places
	aval_sq = aval*aval;

	//Note - aval^3 is a full rotation, so it is just 1.0
	//aval^4 is aval (full rotation + 1)

	//Form up the output directly
	Yabcmat[0] = (Y0 + Y1 + Y2)/3.0;
	Yabcmat[1] = (Y0 + aval*Y1+aval_sq*Y2)/3.0;
	Yabcmat[2] = (Y0 + aval_sq*Y1+aval*Y2)/3.0;
	Yabcmat[3] = (Y0 + aval_sq*Y1+aval*Y2)/3.0;
	Yabcmat[4] = (Y0 + Y1 + Y2)/3.0;
	Yabcmat[5] = (Y0 + aval*Y1+aval_sq*Y2)/3.0;
	Yabcmat[6] = (Y0 + aval*Y1+aval_sq*Y2)/3.0;
	Yabcmat[7] = (Y0 + aval_sq*Y1+aval*Y2)/3.0;
	Yabcmat[8] = (Y0 + Y1 + Y2)/3.0;
}

//Converts a 3x1 sequence vector to a 3x1 abc vector
//Xpn0 is formatted [positive, negative, zero]
//Xabc is formatted [a b c]
void diesel_dg::convert_pn0_to_abc(complex *Xpn0, complex *Xabc)
{
	complex aval, aval_sq;

	//Define the "transformation" term (1@120deg)
	aval = complex(cos(2.0*PI/3.0),sin(2.0*PI/3.0));
	
	//Make the square, since we'll need it a few places
	aval_sq = aval*aval;

	//Form up the output directly
	Xabc[0] = Xpn0[2] + Xpn0[0] + Xpn0[1];
	Xabc[1] = Xpn0[2] + aval_sq*Xpn0[0] + aval*Xpn0[1];
	Xabc[2] = Xpn0[2] + aval*Xpn0[0] + aval_sq*Xpn0[1];
}

//Converts a 3x1 abc vector to a 3x1 sequence components vector
//Xabc is formatted [a b c]
//Xpn0 is formatted [positive, negative, zero]
void diesel_dg::convert_abc_to_pn0(complex *Xabc, complex *Xpn0)
{
	complex aval, aval_sq;

	//Define the "transformation" term (1@120deg)
	aval = complex(cos(2.0*PI/3.0),sin(2.0*PI/3.0));
	
	//Make the square, since we'll need it a few places
	aval_sq = aval*aval;

	//Form up the output directly - note the output form is jostled
	//from the default (defaults to zero, pos, neg)
	Xpn0[0] = (Xabc[0] + (aval*Xabc[1]) + (aval_sq*Xabc[2]))/3.0;
	Xpn0[1] = (Xabc[0] + (aval_sq*Xabc[1]) + (aval*Xabc[2]))/3.0;
	Xpn0[2] = (Xabc[0] + Xabc[1] + Xabc[2])/3.0;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Module-level call
SIMULATIONMODE diesel_dg::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	unsigned char pass_mod;
	unsigned int loop_index;
	double temp_double, temp_mag_val, temp_mag_diff;
	double temp_double_freq_val;
	double deltat, deltath;
	double omega_pu;
	double x5a_now;
	complex temp_rotation;
	complex temp_complex[3];
	complex temp_current_val[3];
	gld_wlock *test_rlock;

	//Create delta_t variable
	deltat = (double)dt/(double)DT_SECOND;
	deltath = deltat/2.0;

	//Reset the powerflow interface variables
	reset_powerflow_accumulators();

	//Pull the present powerflow values
	pull_powerflow_values();

	//Initialization items
	if ((delta_time==0) && (iteration_count_val==0))	//First run of new delta call
	{
		
		//Allocate torque-delay array properly - if neeeded
		if (Governor_type == DEGOV1) 
		{
			//See if we need to free first
			if (torque_delay!=NULL)
			{
				gl_free(torque_delay);	//Free it up
			}

			//Figure out how big the new array needs to be - Make it one lo
			torque_delay_len=(unsigned int)(gov_degov1_TD*DT_SECOND/dt);

			//See if there's any leftovers
			temp_double = gov_degov1_TD-(double)(torque_delay_len*dt)/(double)DT_SECOND;

			if (temp_double > 0.0)	//Means bigger, +1 it
				torque_delay_len += 1;
			//Default else - it's either just right, or negative (meaning we should be 1 bigger already)

			//Now set it up
			torque_delay = (double *)gl_malloc(torque_delay_len*sizeof(double));

			//Make sure it worked
			if (torque_delay == NULL)
			{
				gl_error("diesel_dg: failed to allocate to allocate the delayed torque array for Governor!");
				/*  TROUBLESHOOT
				The diesel_dg object failed to allocate the memory needed for the delayed torque array inside
				the governor control.  Please try again.  If the error persists, please submit your code
				and a bug report via the trac website.
				*/
				return SM_ERROR;
			}

			//Initialize index variables
			torque_delay_write_pos = torque_delay_len-1;	//Write at the end of the array first (-1)
			torque_delay_read_pos = 0;	//Read at beginning
		}//End DEGOV1 type

		//Initialize dynamics
		init_dynamics(&curr_state);

		//GGOV1 delay stuff has to go after the init, since it needs a value to initalize
		if ((Governor_type == GGOV1) || (Governor_type == GGOV1_OLD)) 
		{
			//See if we need to free first
			if (x5a_delayed!=NULL)
			{
				gl_free(x5a_delayed);	//Free it up
			}

			if (gov_ggv1_Teng > 0)
			{
				//Figure out how big the new array needs to be - Make it one lo
				x5a_delayed_len=(unsigned int)(gov_ggv1_Teng*DT_SECOND/dt);

				//See if there's any leftovers
				temp_double = gov_ggv1_Teng-(double)(x5a_delayed_len*dt)/(double)DT_SECOND;

				if (temp_double > 0.0)	//Means bigger, +1 it
					x5a_delayed_len += 1;
				//Default else - it's either just right, or negative (meaning we should be 1 bigger already)

				//Now set it up
				x5a_delayed = (double *)gl_malloc(x5a_delayed_len*sizeof(double));

				//Make sure it worked
				if (x5a_delayed == NULL)
				{
					gl_error("diesel_dg: failed to allocate to allocate the delayed x5a array for Governor!");
					/*  TROUBLESHOOT
					The diesel_dg object failed to allocate the memory needed for the delayed x5a array inside
					the governor control.  Please try again.  If the error persists, please submit your code
					and a bug report via the trac website.
					*/
					return SM_ERROR;
				}

				//Initialize index variables
				x5a_delayed_write_pos = x5a_delayed_len-1;	//Write at the end of the array first (-1)
				x5a_delayed_read_pos = 0;	//Read at beginning

				//Initialize the values
				for (loop_index=0; loop_index<x5a_delayed_len; loop_index++)
				{
					x5a_delayed[loop_index] = curr_state.gov_ggov1.x5a;
				}
			}//End delay array initialization
			else //No delay
			{
				x5a_delayed = NULL;	//Just in case
				x5a_delayed_write_pos = -1;	//These should cause access violations or something, if they get used
				x5a_delayed_read_pos = -1;
			}//End no delay in Teng
		}//End GGOV1 type

		//Initialize rotor variable
		prev_rotor_speed_val = curr_state.omega;

		//Replicate curr_state into next
		memcpy(&next_state,&curr_state,sizeof(MAC_STATES));

	}//End first pass and timestep of deltamode (initial condition stuff)
	else if (iteration_count_val == 0)	//Not first run, just first run of this timestep
	{
		//Update "current" pointer of torque array - if necessary
		if (Governor_type == DEGOV1) 
		{
			//Increment positions
			torque_delay_write_pos++;
			torque_delay_read_pos++;

			//Check for wrapping
			if (torque_delay_read_pos >= torque_delay_len)
				torque_delay_read_pos = 0;

			if (torque_delay_write_pos >= torque_delay_len)
				torque_delay_write_pos = 0;
		}//End DEGOV1 first pass handling
		else if ((Governor_type == GGOV1) || (Governor_type == GGOV1_OLD))
		{
			if (gov_ggv1_Teng > 0)
			{
				//Increment positions
				x5a_delayed_write_pos++;
				x5a_delayed_read_pos++;

				//Check for wrapping
				if (x5a_delayed_read_pos >= x5a_delayed_len)
					x5a_delayed_read_pos = 0;

				if (x5a_delayed_write_pos >= x5a_delayed_len)
					x5a_delayed_write_pos = 0;
			}
			//Default else -- no delay, so nothing needs to be set
		}//End GGOV1 first pass handling
	}//End first pass of new timestep

	//See what we're on, for tracking
	pass_mod = iteration_count_val - ((iteration_count_val >> 1) << 1);

	//Check pass
	if (pass_mod==0)	//Predictor pass
	{
		//Compute the "present" electric power value before anything gets updated for the new timestep
		temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0] - generator_admittance[0][1]*value_Circuit_V[1] - generator_admittance[0][2]*value_Circuit_V[2]);
		temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0]*value_Circuit_V[0] - generator_admittance[1][1]*value_Circuit_V[1] - generator_admittance[1][2]*value_Circuit_V[2]);
		temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0]*value_Circuit_V[0] - generator_admittance[2][1]*value_Circuit_V[1] - generator_admittance[2][2]*value_Circuit_V[2]);

		//Update power output variables, just so we can see what is going on
		power_val[0] = value_Circuit_V[0]*~temp_current_val[0];
		power_val[1] = value_Circuit_V[1]*~temp_current_val[1];
		power_val[2] = value_Circuit_V[2]*~temp_current_val[2];

		//Update the output power variable
		curr_state.pwr_electric = power_val[0] + power_val[1] + power_val[2];

		//Copy it into the "next" value as well, so it doesn't get overwritten funny when the transition occurs
		next_state.pwr_electric = curr_state.pwr_electric;

		// Update energy, fuel usage, and emissions for the past time step, before updating power output
		if (fuelEmissionCal == true) {

			outputEnergy += fabs(curr_state.pwr_electric.Re()/1000) * (double)deltat / 3600;
			FuelUse += (fabs(curr_state.pwr_electric.Re()/1000) * dg_1000_a + dg_1000_b) * (double)deltat / 3600;
			if (FuelUse != 0) {
				efficiency = outputEnergy/FuelUse;
			}
			CO2_emission += (-6e-5 * pow(FuelUse, 3) + 0.0087 * pow(FuelUse, 2) - FuelUse * 0.3464 + 25.824) * (double)deltat / 3600;
			SOx_emission += (-5e-7 * pow(FuelUse, 2) + FuelUse * 0.0001 + 0.0206) * (double)deltat / 3600;
			NOx_emission += (6e-5 * pow(FuelUse, 2) - FuelUse * 0.0048 + 0.2551) * (double)deltat / 3600;
			PM10_emission += (-2e-9 * pow(FuelUse, 4) + 3e-7 * pow(FuelUse, 3) - 2e-5 * pow(FuelUse, 2) + FuelUse * 8e-5 + 0.0083) * (double)deltat / 3600;

			// Frequency deviation calculation
			frequency_deviation = (curr_state.omega - 2 * PI * 60)/(2 * PI * 60);
			frequency_deviation_energy += fabs(frequency_deviation);

			// Obtain maximum frequency deviation
			if (frequency_deviation <= 0 && frequency_deviation_max <= 0) {
				if (frequency_deviation < frequency_deviation_max) {
					frequency_deviation_max = fabs(frequency_deviation);
				}
			}
			else if (frequency_deviation >= 0 && frequency_deviation_max >= 0) {
				if (frequency_deviation > frequency_deviation_max) {
					frequency_deviation_max = fabs(frequency_deviation);
				}
			}
			else if (frequency_deviation > 0 && frequency_deviation_max < 0) {
				if (frequency_deviation > -frequency_deviation_max) {
					frequency_deviation_max = fabs(frequency_deviation);
				}
			}
			else if (frequency_deviation < 0 && frequency_deviation_max > 0) {
				if (-frequency_deviation > frequency_deviation_max) {
					frequency_deviation_max = fabs(-frequency_deviation);
				}
			}
			realPowerChange = curr_state.pwr_electric.Re() - pwr_electric_init;
			ratio_f_p = -frequency_deviation/(realPowerChange/Rated_VA);
		}

		//Call dynamics
		apply_dynamics(&curr_state,&predictor_vals,deltat);

		//Apply prediction update
		if (Q_constant_mode == true) {
			next_state.avr.xfd = curr_state.avr.xfd + predictor_vals.avr.xfd*deltat;
			next_state.Vfd = next_state.avr.xfd + predictor_vals.avr.xfd*(kp_Qconstant/ki_Qconstant);
		}

		next_state.Flux1d = curr_state.Flux1d + predictor_vals.Flux1d*deltat;
		next_state.Flux2q = curr_state.Flux2q + predictor_vals.Flux2q*deltat;
		next_state.EpRotated = curr_state.EpRotated + predictor_vals.EpRotated*deltat;
		next_state.rotor_angle = curr_state.rotor_angle + predictor_vals.rotor_angle*deltat;
		next_state.omega = curr_state.omega + predictor_vals.omega*deltat;
		
		next_state.VintRotated  = (Xqpp-Xdpp)*curr_state.Irotated.Im();
		next_state.VintRotated += (Xqpp-Xl)/(Xqp-Xl)*next_state.EpRotated.Re() - (Xqp-Xqpp)/(Xqp-Xl)*next_state.Flux2q;
		next_state.VintRotated += complex(0.0,1.0)*((Xdpp-Xl)/(Xdp-Xl)*next_state.EpRotated.Im()+(Xdp-Xdpp)/(Xdp-Xl)*next_state.Flux1d);

		//Form rotation multiplier - or demultiplier
		temp_rotation = complex(0.0,1.0)*complex_exp(-1.0*next_state.rotor_angle);
		temp_complex[0] = next_state.VintRotated/temp_rotation*voltage_base;
		temp_complex[1] = temp_complex[2] = 0.0;

		//Unsequence it
		convert_pn0_to_abc(&temp_complex[0], &next_state.EintVal[0]);

		//Governor updates, if relevant
		if (Governor_type == DEGOV1)
		{
			next_state.gov_degov1.x1 = curr_state.gov_degov1.x1 + predictor_vals.gov_degov1.x1*deltat;
			next_state.gov_degov1.x2 = curr_state.gov_degov1.x2 + predictor_vals.gov_degov1.x2*deltat;
			next_state.gov_degov1.x4 = curr_state.gov_degov1.x4 + predictor_vals.gov_degov1.x4*deltat;
			next_state.gov_degov1.x5 = curr_state.gov_degov1.x5 + predictor_vals.gov_degov1.x5*deltat;
			next_state.gov_degov1.x6 = curr_state.gov_degov1.x6 + predictor_vals.gov_degov1.x6*deltat;
		}//End DEGOV1 update
		else if (Governor_type == GAST)
		{
			next_state.gov_gast.x1 = curr_state.gov_gast.x1 + predictor_vals.gov_gast.x1*deltat;
			next_state.gov_gast.x2 = curr_state.gov_gast.x2 + predictor_vals.gov_gast.x2*deltat;
			next_state.gov_gast.x3 = curr_state.gov_gast.x3 + predictor_vals.gov_gast.x3*deltat;
		}//End GAST update
		else if (Governor_type == P_CONSTANT)
		{
			//Main params
			next_state.gov_pconstant.x1 = curr_state.gov_pconstant.x1 + predictor_vals.gov_pconstant.x1*deltat;
			next_state.gov_pconstant.x4 = curr_state.gov_pconstant.x4 + predictor_vals.gov_pconstant.x4*deltat;
			next_state.gov_pconstant.x5b = curr_state.gov_pconstant.x5b + predictor_vals.gov_pconstant.x5b*deltat;
			next_state.gov_pconstant.x_Pconstant = curr_state.gov_pconstant.x_Pconstant + predictor_vals.gov_pconstant.x_Pconstant*deltat;
			next_state.gov_pconstant.GovOutPut = next_state.gov_pconstant.x_Pconstant + (gen_base_set_vals.Pref - next_state.gov_pconstant.x1) * kp_Pconstant;

			//Update algebraic variables
			//4 - Turbine actuator
			next_state.gov_pconstant.ValveStroke = next_state.gov_pconstant.x4;
			if (pconstant_Flag == 0)
			{
				next_state.gov_pconstant.FuelFlow = next_state.gov_pconstant.ValveStroke * 1.0;
			}
			else if (pconstant_Flag == 1)
			{
				next_state.gov_pconstant.FuelFlow = next_state.gov_pconstant.ValveStroke*next_state.omega/omega_ref;
			}
			else
			{
				gl_error("wrong pconstant_Flag_flag");
				return SM_ERROR;
			}
			//5 - Turbine LL
			x5a_now = pconstant_Kturb*(next_state.gov_pconstant.FuelFlow - pconstant_wfnl);

			if (pconstant_Teng > 0)
			{
				//Update the stored value
				x5a_delayed[x5a_delayed_write_pos] = x5a_now;

				//Assign the oldest value
				next_state.gov_pconstant.x5a = x5a_delayed[x5a_delayed_read_pos];
			}
			else	//Zero length
			{
				//Just assign in
				next_state.gov_pconstant.x5a = x5a_now;
			}
			next_state.gov_pconstant.x5 = (1.0 - pconstant_Tc/pconstant_Tb)*next_state.gov_pconstant.x5b + pconstant_Tc/pconstant_Tb*next_state.gov_pconstant.x5a;

			//Mechanical power update
			next_state.pwr_mech = Rated_VA*(next_state.gov_pconstant.x5);

			//See if mechanical power is too big -- if so, limit it (and x5 to match)
			if (next_state.pwr_mech > Overload_Limit_Value)
			{
				//Limit it
				next_state.pwr_mech = Overload_Limit_Value;

				//Fix the state variable, in hopes it will propagate
				next_state.gov_pconstant.x5 = Overload_Limit_Value / Rated_VA;
			}

			//Translate this into the torque model
			next_state.torque_mech = next_state.pwr_mech / next_state.omega;
		}//End P_CONSTANT update
		else if ((Governor_type == GGOV1) || (Governor_type == GGOV1_OLD))
		{
			//Main params
			next_state.gov_ggov1.x1 = curr_state.gov_ggov1.x1 + predictor_vals.gov_ggov1.x1*deltat;
			next_state.gov_ggov1.x2a = curr_state.gov_ggov1.x2a + predictor_vals.gov_ggov1.x2a*deltat;
			next_state.gov_ggov1.x3 = curr_state.gov_ggov1.x3 + predictor_vals.gov_ggov1.x3*deltat;
			next_state.gov_ggov1.x4 = curr_state.gov_ggov1.x4 + predictor_vals.gov_ggov1.x4*deltat;
			next_state.gov_ggov1.x5b = curr_state.gov_ggov1.x5b + predictor_vals.gov_ggov1.x5b*deltat;
			next_state.gov_ggov1.x6 = curr_state.gov_ggov1.x6 + predictor_vals.gov_ggov1.x6*deltat;
			next_state.gov_ggov1.x7 = curr_state.gov_ggov1.x7 + predictor_vals.gov_ggov1.x7*deltat;
			next_state.gov_ggov1.x8a = curr_state.gov_ggov1.x8a + predictor_vals.gov_ggov1.x8a*deltat;
			next_state.gov_ggov1.x9a = curr_state.gov_ggov1.x9a + predictor_vals.gov_ggov1.x9a*deltat;
			next_state.gov_ggov1.x10b = curr_state.gov_ggov1.x10b + predictor_vals.gov_ggov1.x10b*deltat;

			//Update algebraic variables of GGOV1
			//8 - Supervisory load control
			if (next_state.gov_ggov1.x8a > (1.1*gov_ggv1_r))
			{
				next_state.gov_ggov1.x8 = 1.1 * gov_ggv1_r;
			}
			else if (next_state.gov_ggov1.x8a < (-1.1*gov_ggv1_r))
			{
				next_state.gov_ggov1.x8 = -1.1 * gov_ggv1_r;
			}
			else
			{
				next_state.gov_ggov1.x8 = next_state.gov_ggov1.x8a;
			}

			//4 - Turbine actuator
			next_state.gov_ggov1.ValveStroke = next_state.gov_ggov1.x4;
			if (gov_ggv1_Flag == 0)
			{
				next_state.gov_ggov1.FuelFlow = next_state.gov_ggov1.ValveStroke * 1.0;
			}
			else if (gov_ggv1_Flag == 1)
			{
				next_state.gov_ggov1.FuelFlow = next_state.gov_ggov1.ValveStroke*next_state.omega/omega_ref;
			}
			else
			{
				gl_error("wrong ggv1_flag");
				return SM_ERROR;
			}

			//2 - Governor differntial control
			next_state.gov_ggov1.GovOutPut = curr_state.gov_ggov1.GovOutPut;
			//Rselect switch
			if (gov_ggv1_rselect == 1)
			{
				next_state.gov_ggov1.RselectValue = next_state.gov_ggov1.x1;
			}
			else if (gov_ggv1_rselect == -1)
			{
				next_state.gov_ggov1.RselectValue = next_state.gov_ggov1.ValveStroke;
			}
			else if (gov_ggv1_rselect == -2)
			{
				next_state.gov_ggov1.RselectValue = next_state.gov_ggov1.GovOutPut;
			}
			else if (gov_ggv1_rselect == 0)
			{
				next_state.gov_ggov1.RselectValue = 0.0;
			}
			else
			{
				gl_error("wrong ggv1_rselect parameter");
				return SM_ERROR;
			}

			//Error deadband
			//Assign GovOutPut latest value (for use in closed loop)
			//Only needed in predictor updates
			next_state.gov_ggov1.werror = next_state.omega/omega_ref - gen_base_set_vals.wref;
			next_state.gov_ggov1.err2a = gen_base_set_vals.Pref + next_state.gov_ggov1.x8 - next_state.gov_ggov1.werror - gov_ggv1_r*next_state.gov_ggov1.RselectValue;

			if (next_state.gov_ggov1.err2a > gov_ggv1_maxerr)
			{
				next_state.gov_ggov1.err2 = gov_ggv1_maxerr;
			}
			else if (next_state.gov_ggov1.err2a < gov_ggv1_minerr)
			{
				next_state.gov_ggov1.err2 = gov_ggv1_minerr;
			}
			else if ((next_state.gov_ggov1.err2a <= gov_ggv1_db) && (next_state.gov_ggov1.err2a >= -gov_ggv1_db))
			{
				next_state.gov_ggov1.err2 = 0.0;
			}
			else
			{
				if (next_state.gov_ggov1.err2a > 0.0)
				{
					next_state.gov_ggov1.err2 = (gov_ggv1_maxerr/(gov_ggv1_maxerr-gov_ggv1_db))*next_state.gov_ggov1.err2a - (gov_ggv1_maxerr*gov_ggv1_db/(gov_ggv1_maxerr-gov_ggv1_db));
				}
				else if (next_state.gov_ggov1.err2a < 0.0)
				{
					next_state.gov_ggov1.err2 = (gov_ggv1_minerr/(gov_ggv1_minerr+gov_ggv1_db))*next_state.gov_ggov1.err2a + (gov_ggv1_minerr*gov_ggv1_db/(gov_ggv1_minerr+gov_ggv1_db));
				}
			}
			next_state.gov_ggov1.x2 = gov_ggv1_Kpgov/gov_ggv1_Tdgov*(next_state.gov_ggov1.err2 - next_state.gov_ggov1.x2a);

			//3 - Governor integral control
			if ((Governor_type == GGOV1_OLD) || ((Governor_type == GGOV1) && (gov_ggv1_Kpgov == 0.0)))	//Old implementation, or "disabled" new
			{
				next_state.gov_ggov1.x3a = gov_ggv1_Kigov*next_state.gov_ggov1.err2;
			}
			else	//Newer version
			{
				next_state.gov_ggov1.err3 = next_state.gov_ggov1.GovOutPut - next_state.gov_ggov1.x3;
				next_state.gov_ggov1.x3a = gov_ggv1_Kigov/gov_ggv1_Kpgov*next_state.gov_ggov1.err3;
			}

			next_state.gov_ggov1.fsrn = next_state.gov_ggov1.x2 + gov_ggv1_Kpgov*next_state.gov_ggov1.err2 + next_state.gov_ggov1.x3;

			//5 - Turbine LL
			x5a_now = gov_ggv1_Kturb*(next_state.gov_ggov1.FuelFlow - gov_ggv1_wfnl);

			if (gov_ggv1_Teng > 0)
			{
				//Update the stored value
				x5a_delayed[x5a_delayed_write_pos] = x5a_now;

				//Assign the oldest value
				next_state.gov_ggov1.x5a = x5a_delayed[x5a_delayed_read_pos];
			}
			else	//Zero length
			{
				//Just assign in
				next_state.gov_ggov1.x5a = x5a_now;
			}
			next_state.gov_ggov1.x5 = (1.0 - gov_ggv1_Tc/gov_ggv1_Tb)*next_state.gov_ggov1.x5b + gov_ggv1_Tc/gov_ggv1_Tb*next_state.gov_ggov1.x5a;
			if (gov_ggv1_Dm > 0.0)
			{
				//Mechanical power update
				next_state.pwr_mech = Rated_VA*(next_state.gov_ggov1.x5 - gov_ggv1_Dm*(next_state.omega/omega_ref - gen_base_set_vals.wref));

				//Check the value and threshold, if necessary
				if (next_state.pwr_mech > Overload_Limit_Value)
				{
					//Set power
					next_state.pwr_mech = Overload_Limit_Value;

					//Fix the variables
					next_state.gov_ggov1.x5 = Overload_Limit_Value / Rated_VA + gov_ggv1_Dm*(next_state.omega/omega_ref - gen_base_set_vals.wref);
				}
			}
			else
			{
				//Mechanical power update
				next_state.pwr_mech = Rated_VA*(next_state.gov_ggov1.x5);

				//Check the value, and threshold, if necessary
				if (next_state.pwr_mech > Overload_Limit_Value)
				{
					//Limit it
					next_state.pwr_mech = Overload_Limit_Value;

					//Fix the variable
					next_state.gov_ggov1.x5 = next_state.pwr_mech / Rated_VA;
				}
			}
			//Translate this into the torque model
			next_state.torque_mech = next_state.pwr_mech / next_state.omega;

			//10 - Temp detection LL
			if (gov_ggv1_Dm < 0.0)
			{
				next_state.gov_ggov1.x10a = next_state.gov_ggov1.FuelFlow * pow((next_state.omega/omega_ref),gov_ggv1_Dm);
			}
			else
			{
				next_state.gov_ggov1.x10a = next_state.gov_ggov1.FuelFlow;
			}
			next_state.gov_ggov1.x10 = (1.0 - gov_ggv1_Tsa/gov_ggv1_Tsb)*next_state.gov_ggov1.x10b + gov_ggv1_Tsa/gov_ggv1_Tsb*next_state.gov_ggov1.x10a;

			//7 - Turbine load integral control
			next_state.gov_ggov1.err7 = next_state.gov_ggov1.GovOutPut - next_state.gov_ggov1.x7;
			if (gov_ggv1_Kpload > 0.0)
			{
				next_state.gov_ggov1.x7a = gov_ggv1_Kiload/gov_ggv1_Kpload*next_state.gov_ggov1.err7;
			}
			else
			{
				next_state.gov_ggov1.x7a = gov_ggv1_Kiload*next_state.gov_ggov1.err7;
			}
			next_state.gov_ggov1.fsrtNoLim = next_state.gov_ggov1.x7 + gov_ggv1_Kpload*(-1.0 * next_state.gov_ggov1.x6 + gov_ggv1_Ldref*(gov_ggv1_Ldref/gov_ggv1_Kturb+gov_ggv1_wfnl));
			if (next_state.gov_ggov1.fsrtNoLim > 1.0)
			{
				next_state.gov_ggov1.fsrt = 1.0;
			}
			else
			{
				next_state.gov_ggov1.fsrt = next_state.gov_ggov1.fsrtNoLim;
			}

			//9 - Acceleration control
			next_state.gov_ggov1.x9 = 1.0/gov_ggv1_Ta*((next_state.omega/omega_ref - gen_base_set_vals.wref) - next_state.gov_ggov1.x9a);
			next_state.gov_ggov1.fsra = gov_ggv1_Ka*deltat*(gov_ggv1_aset - next_state.gov_ggov1.x9) + next_state.gov_ggov1.GovOutPut;

			//Pre-empt the low-value select, if needed
			if (gov_ggv1_fsrt_enable == false)
			{
				next_state.gov_ggov1.fsrt = 99999999.0;	//Big value
			}

			if (gov_ggv1_fsra_enable == false)
			{
				next_state.gov_ggov1.fsra = 99999999.0;	//Big value
			}

			if (gov_ggv1_fsrn_enable == false)
			{
				next_state.gov_ggov1.fsrn = 99999999.0;	//Big value
			}

			//Low value select
			if (next_state.gov_ggov1.fsrt < next_state.gov_ggov1.fsrn)
			{
				next_state.gov_ggov1.LowValSelect1 = next_state.gov_ggov1.fsrt;
			}
			else
			{
				next_state.gov_ggov1.LowValSelect1 = next_state.gov_ggov1.fsrn;
			}

			if (next_state.gov_ggov1.fsra < next_state.gov_ggov1.LowValSelect1)
			{
				next_state.gov_ggov1.LowValSelect = next_state.gov_ggov1.fsra;
			}
			else
			{
				next_state.gov_ggov1.LowValSelect = next_state.gov_ggov1.LowValSelect1;
			}

			if (next_state.gov_ggov1.LowValSelect > gov_ggv1_vmax)
			{
				next_state.gov_ggov1.GovOutPut = gov_ggv1_vmax;
			}
			else if (next_state.gov_ggov1.LowValSelect < gov_ggv1_vmin)
			{
				next_state.gov_ggov1.GovOutPut = gov_ggv1_vmin;
			}
			else
			{
				next_state.gov_ggov1.GovOutPut = next_state.gov_ggov1.LowValSelect;
			}

		}//End GGOV1 update
		//Default else - no updates because no governor

		//Exciter updates
		if (Exciter_type == SEXS)
		{
//			if (CVRenabled) {
//				if (CVR_PI) {
//					next_state.avr.x_cvr = curr_state.avr.x_cvr + predictor_vals.avr.x_cvr*deltat;
//					gen_base_set_vals.vseta = Vref + next_state.avr.x_cvr + predictor_vals.avr.diff_f * kp_cvr;
//				}
//				else if (CVR_PID) {
//					next_state.avr.x_cvr = curr_state.avr.x_cvr + predictor_vals.avr.x_cvr*deltat;
//					next_state.avr.xerr_cvr = predictor_vals.avr.diff_f * kd_cvr;
//					predictor_vals.avr.xerr_cvr = (next_state.avr.xerr_cvr - curr_state.avr.xerr_cvr) / deltat;
//					gen_base_set_vals.vseta = Vref + next_state.avr.x_cvr + predictor_vals.avr.diff_f * kp_cvr + predictor_vals.avr.xerr_cvr;
//				}
//
//				//Limit check
// 				if (gen_base_set_vals.vseta >= vset_EMAX)
//					gen_base_set_vals.vsetb = vset_EMAX;
//
//				if (gen_base_set_vals.vseta <= vset_EMIN)
//					gen_base_set_vals.vsetb = vset_EMIN;
//
//				// Give value to vset
//				gen_base_set_vals.vset = gen_base_set_vals.vsetb;
//			}

//			if (CVRenabled) {
//				next_state.avr.xerr_cvr = predictor_vals.avr.diff_f * kd_cvr;
//				predictor_vals.avr.xerr_cvr = (next_state.avr.xerr_cvr - curr_state.avr.xerr_cvr) / deltat;
//				gen_base_set_vals.vadd = predictor_vals.avr.xerr_cvr + predictor_vals.avr.diff_f * kp_cvr;
//			}

			if (CVRenabled) {

				// Implementation for high order CVR control
				if (CVRmode == HighOrder) {
					if (Kd1 != 0) {
						next_state.avr.x_cvr1 = curr_state.avr.x_cvr1 + predictor_vals.avr.x_cvr1*deltat;
						next_state.avr.x_cvr2 = curr_state.avr.x_cvr2 + predictor_vals.avr.x_cvr2*deltat;
						gen_base_set_vals.vadd = (Kn1/Kd1) * next_state.avr.x_cvr1 + (Kn2/Kd1) * next_state.avr.x_cvr2 + kp_cvr * predictor_vals.avr.diff_f;
					}
					else {
						next_state.avr.x_cvr1 = curr_state.avr.x_cvr1 + predictor_vals.avr.x_cvr1*deltat;
						gen_base_set_vals.vadd = (Kn2/Kd2 - (Kd3 * Kn1)/(Kd2 * Kd2)) * next_state.avr.x_cvr1 + (kp_cvr + Kn1/Kd2) * predictor_vals.avr.diff_f;
					}

					//Limit check
					if (gen_base_set_vals.vadd >= vset_delta_MAX)
						gen_base_set_vals.vadd = vset_delta_MAX;

					if (gen_base_set_vals.vadd <= vset_delta_MIN)
						gen_base_set_vals.vadd = vset_delta_MIN;

				}
				// Implementation for first order CVR control with feedback loop
				else if (CVRmode == Feedback) {
					next_state.avr.x_cvr1 = curr_state.avr.x_cvr1 + predictor_vals.avr.x_cvr1*deltat;
					gen_base_set_vals.vadd_a = kp_cvr * predictor_vals.avr.diff_f + next_state.avr.x_cvr1;

					//Limit check
					if (gen_base_set_vals.vadd_a >= vset_delta_MAX) {
						gen_base_set_vals.vadd = vset_delta_MAX;
					}
					else if (gen_base_set_vals.vadd_a <= vset_delta_MIN) {
						gen_base_set_vals.vadd = vset_delta_MIN;
					}
					else {
						gen_base_set_vals.vadd = gen_base_set_vals.vadd_a;
					}
				}

				// Give value to vset
				gen_base_set_vals.vset = gen_base_set_vals.vadd + Vref;
			}


			next_state.avr.xe = curr_state.avr.xe + predictor_vals.avr.xe*deltat;
			next_state.avr.xb = curr_state.avr.xb + predictor_vals.avr.xb*deltat;
		}//End SEXS update
		//Default else - no updates because no exciter

		//Nab per-unit omega, while we're at it
	    omega_pu = curr_state.omega/omega_ref;

		//Update generator current injection (technically is done "before" next powerflow)
		value_IGenerated[0] = next_state.EintVal[0]*YS1*omega_pu;
		value_IGenerated[1] = next_state.EintVal[1]*YS1*omega_pu;
		value_IGenerated[2] = next_state.EintVal[2]*YS1*omega_pu;

		//Resync power variables
		push_powerflow_values(false);

		return SM_DELTA_ITER;	//Reiterate - to get us to corrector pass
	}
	else	//Corrector pass
	{
		//Call dynamics
		apply_dynamics(&next_state,&corrector_vals,deltat);

		//Reconcile updates update
		if (Q_constant_mode == true) {
			next_state.avr.xfd = curr_state.avr.xfd + (predictor_vals.avr.xfd + corrector_vals.avr.xfd)*deltath;
			next_state.Vfd = next_state.avr.xfd + (predictor_vals.avr.xfd + corrector_vals.avr.xfd)*0.5*(kp_Qconstant/ki_Qconstant);
		}

		next_state.Flux1d = curr_state.Flux1d + (predictor_vals.Flux1d + corrector_vals.Flux1d)*deltath;
		next_state.Flux2q = curr_state.Flux2q + (predictor_vals.Flux2q + corrector_vals.Flux2q)*deltath;
		next_state.EpRotated = curr_state.EpRotated + (predictor_vals.EpRotated + corrector_vals.EpRotated)*deltath;
		next_state.rotor_angle = curr_state.rotor_angle + (predictor_vals.rotor_angle + corrector_vals.rotor_angle)*deltath;
		next_state.omega = curr_state.omega + (predictor_vals.omega + corrector_vals.omega)*deltath;
		
		next_state.VintRotated  = (Xqpp-Xdpp)*next_state.Irotated.Im();
		next_state.VintRotated += (Xqpp-Xl)/(Xqp-Xl)*next_state.EpRotated.Re() - (Xqp-Xqpp)/(Xqp-Xl)*next_state.Flux2q;
		next_state.VintRotated += complex(0.0,1.0)*((Xdpp-Xl)/(Xdp-Xl)*next_state.EpRotated.Im()+(Xdp-Xdpp)/(Xdp-Xl)*next_state.Flux1d);

		//Form rotation multiplier - or demultiplier
		temp_rotation = complex(0.0,1.0)*complex_exp(-1.0*next_state.rotor_angle);
		temp_complex[0] = next_state.VintRotated/temp_rotation*voltage_base;
		temp_complex[1] = temp_complex[2] = 0.0;

		//Unsequence it
		convert_pn0_to_abc(&temp_complex[0], &next_state.EintVal[0]);

		//Governor updates, if relevant
		if (Governor_type == DEGOV1)
		{
			next_state.gov_degov1.x1 = curr_state.gov_degov1.x1 + (predictor_vals.gov_degov1.x1 + corrector_vals.gov_degov1.x1)*deltath;
			next_state.gov_degov1.x2 = curr_state.gov_degov1.x2 + (predictor_vals.gov_degov1.x2 + corrector_vals.gov_degov1.x2)*deltath;
			next_state.gov_degov1.x4 = curr_state.gov_degov1.x4 + (predictor_vals.gov_degov1.x4 + corrector_vals.gov_degov1.x4)*deltath;
			next_state.gov_degov1.x5 = curr_state.gov_degov1.x5 + (predictor_vals.gov_degov1.x5 + corrector_vals.gov_degov1.x5)*deltath;
			next_state.gov_degov1.x6 = curr_state.gov_degov1.x6 + (predictor_vals.gov_degov1.x6 + corrector_vals.gov_degov1.x6)*deltath;
		}//End DEGOV1 update
		else if (Governor_type == GAST)
		{
			next_state.gov_gast.x1 = curr_state.gov_gast.x1 + (predictor_vals.gov_gast.x1 + corrector_vals.gov_gast.x1)*deltath;
			next_state.gov_gast.x2 = curr_state.gov_gast.x2 + (predictor_vals.gov_gast.x2 + corrector_vals.gov_gast.x2)*deltath;
			next_state.gov_gast.x3 = curr_state.gov_gast.x3 + (predictor_vals.gov_gast.x3 + corrector_vals.gov_gast.x3)*deltath;
		}//End GAST update
		else if (Governor_type == P_CONSTANT)
		{
			//Main params
			next_state.gov_pconstant.x1 = curr_state.gov_pconstant.x1 + (predictor_vals.gov_pconstant.x1 + corrector_vals.gov_pconstant.x1)*deltath;
			next_state.gov_pconstant.x4 = curr_state.gov_pconstant.x4 + (predictor_vals.gov_pconstant.x4 + corrector_vals.gov_pconstant.x4)*deltath;
			next_state.gov_pconstant.x5b = curr_state.gov_pconstant.x5b + (predictor_vals.gov_pconstant.x5b + corrector_vals.gov_pconstant.x5b)*deltath;
			next_state.gov_pconstant.x_Pconstant = curr_state.gov_pconstant.x_Pconstant + (predictor_vals.gov_pconstant.x_Pconstant + corrector_vals.gov_pconstant.x_Pconstant)*deltath;
			next_state.gov_pconstant.GovOutPut = next_state.gov_pconstant.x_Pconstant + (gen_base_set_vals.Pref - next_state.gov_pconstant.x1) * kp_Pconstant;

			//Update algebraic variables
			//4 - Turbine actuator
			next_state.gov_pconstant.ValveStroke = next_state.gov_pconstant.x4;
			if (pconstant_Flag == 0)
			{
				next_state.gov_pconstant.FuelFlow = next_state.gov_pconstant.ValveStroke * 1.0;
			}
			else if (pconstant_Flag == 1)
			{
				next_state.gov_pconstant.FuelFlow = next_state.gov_pconstant.ValveStroke*next_state.omega/omega_ref;
			}
			else
			{
				gl_error("wrong pconstant_flag");
				return SM_ERROR;
			}

			//5 - Turbine LL
			x5a_now = pconstant_Kturb*(next_state.gov_pconstant.FuelFlow - pconstant_wfnl);

			if (pconstant_Teng > 0)
			{
				//Update the stored value
				x5a_delayed[x5a_delayed_write_pos] = x5a_now;

				//Assign the oldest value
				next_state.gov_pconstant.x5a = x5a_delayed[x5a_delayed_read_pos];
			}
			else	//Zero length
			{
				//Just assign in
				next_state.gov_pconstant.x5a = x5a_now;
			}
			next_state.gov_pconstant.x5 = (1.0 - pconstant_Tc/pconstant_Tb)*next_state.gov_pconstant.x5b + pconstant_Tc/pconstant_Tb*next_state.gov_pconstant.x5a;

			//Mechanical power update
			next_state.pwr_mech = Rated_VA*(next_state.gov_pconstant.x5);

			//Check the limit
			if (next_state.pwr_mech > Overload_Limit_Value)
			{
				//Limit it
				next_state.pwr_mech = Overload_Limit_Value;

				//Fix the variable
				next_state.gov_pconstant.x5 = Overload_Limit_Value / Rated_VA;
			}

			//Translate this into the torque model
			next_state.torque_mech = next_state.pwr_mech / next_state.omega;

		}// End P_CONSTANT mode corrector stage
		else if ((Governor_type == GGOV1) || (Governor_type == GGOV1_OLD))
		{
			//Main params
			next_state.gov_ggov1.x1 = curr_state.gov_ggov1.x1 + (predictor_vals.gov_ggov1.x1 + corrector_vals.gov_ggov1.x1)*deltath;
			next_state.gov_ggov1.x2a = curr_state.gov_ggov1.x2a + (predictor_vals.gov_ggov1.x2a + corrector_vals.gov_ggov1.x2a)*deltath;
			next_state.gov_ggov1.x3 = curr_state.gov_ggov1.x3 + (predictor_vals.gov_ggov1.x3 + corrector_vals.gov_ggov1.x3)*deltath;
			next_state.gov_ggov1.x4 = curr_state.gov_ggov1.x4 + (predictor_vals.gov_ggov1.x4 + corrector_vals.gov_ggov1.x4)*deltath;
			next_state.gov_ggov1.x5b = curr_state.gov_ggov1.x5b + (predictor_vals.gov_ggov1.x5b + corrector_vals.gov_ggov1.x5b)*deltath;
			next_state.gov_ggov1.x6 = curr_state.gov_ggov1.x6 + (predictor_vals.gov_ggov1.x6 + corrector_vals.gov_ggov1.x6)*deltath;
			next_state.gov_ggov1.x7 = curr_state.gov_ggov1.x7 + (predictor_vals.gov_ggov1.x7 + corrector_vals.gov_ggov1.x7)*deltath;
			next_state.gov_ggov1.x8a = curr_state.gov_ggov1.x8a + (predictor_vals.gov_ggov1.x8a + corrector_vals.gov_ggov1.x8a)*deltath;
			next_state.gov_ggov1.x9a = curr_state.gov_ggov1.x9a + (predictor_vals.gov_ggov1.x9a + corrector_vals.gov_ggov1.x9a)*deltath;
			next_state.gov_ggov1.x10b = curr_state.gov_ggov1.x10b + (predictor_vals.gov_ggov1.x10b + corrector_vals.gov_ggov1.x10b)*deltath;

			//Update algebraic variables of GGOV1
			//8 - Supervisory load control
			if (next_state.gov_ggov1.x8a > (1.1*gov_ggv1_r))
			{
				next_state.gov_ggov1.x8 = 1.1 * gov_ggv1_r;
			}
			else if (next_state.gov_ggov1.x8a < (-1.1*gov_ggv1_r))
			{
				next_state.gov_ggov1.x8 = -1.1 * gov_ggv1_r;
			}
			else
			{
				next_state.gov_ggov1.x8 = next_state.gov_ggov1.x8a;
			}

			//4 - Turbine actuator
			next_state.gov_ggov1.ValveStroke = next_state.gov_ggov1.x4;
			if (gov_ggv1_Flag == 0)
			{
				next_state.gov_ggov1.FuelFlow = next_state.gov_ggov1.ValveStroke * 1.0;
			}
			else if (gov_ggv1_Flag == 1)
			{
				next_state.gov_ggov1.FuelFlow = next_state.gov_ggov1.ValveStroke*next_state.omega/omega_ref;
			}
			else
			{
				gl_error("wrong ggv1_flag");
				return SM_ERROR;
			}

			//2 - Governor differntial control
			//Earlier comment says the below only happens in predictor
			//next_state.gov_ggov1.GovOutPut = curr_state.gov_ggov1.GovOutPut;
			//Rselect switch
			if (gov_ggv1_rselect == 1)
			{
				next_state.gov_ggov1.RselectValue = next_state.gov_ggov1.x1;
			}
			else if (gov_ggv1_rselect == -1)
			{
				next_state.gov_ggov1.RselectValue = next_state.gov_ggov1.ValveStroke;
			}
			else if (gov_ggv1_rselect == -2)
			{
				next_state.gov_ggov1.RselectValue = next_state.gov_ggov1.GovOutPut;
			}
			else if (gov_ggv1_rselect == 0)
			{
				next_state.gov_ggov1.RselectValue = 0.0;
			}
			else
			{
				gl_error("wrong ggv1_rselect parameter");
				return SM_ERROR;
			}

			//Error deadband
			next_state.gov_ggov1.werror = next_state.omega/omega_ref - gen_base_set_vals.wref;
			next_state.gov_ggov1.err2a = gen_base_set_vals.Pref + next_state.gov_ggov1.x8 - next_state.gov_ggov1.werror - gov_ggv1_r*next_state.gov_ggov1.RselectValue;
			if (next_state.gov_ggov1.err2a > gov_ggv1_maxerr)
			{
				next_state.gov_ggov1.err2 = gov_ggv1_maxerr;
			}
			else if (next_state.gov_ggov1.err2a < gov_ggv1_minerr)
			{
				next_state.gov_ggov1.err2 = gov_ggv1_minerr;
			}
			else if ((next_state.gov_ggov1.err2a <= gov_ggv1_db) && (next_state.gov_ggov1.err2a >= -gov_ggv1_db))
			{
				next_state.gov_ggov1.err2 = 0.0;
			}
			else
			{
				if (next_state.gov_ggov1.err2a > 0.0)
				{
					next_state.gov_ggov1.err2 = (gov_ggv1_maxerr/(gov_ggv1_maxerr-gov_ggv1_db))*next_state.gov_ggov1.err2a - (gov_ggv1_maxerr*gov_ggv1_db/(gov_ggv1_maxerr-gov_ggv1_db));
				}
				else if (next_state.gov_ggov1.err2a < 0.0)
				{
					next_state.gov_ggov1.err2 = (gov_ggv1_minerr/(gov_ggv1_minerr+gov_ggv1_db))*next_state.gov_ggov1.err2a + (gov_ggv1_minerr*gov_ggv1_db/(gov_ggv1_minerr+gov_ggv1_db));
				}
			}
			next_state.gov_ggov1.x2 = gov_ggv1_Kpgov/gov_ggv1_Tdgov*(next_state.gov_ggov1.err2 - next_state.gov_ggov1.x2a);

			//3 - Governor integral control
			if ((Governor_type == GGOV1_OLD) || ((Governor_type == GGOV1) && (gov_ggv1_Kpgov == 0.0)))	//Old implementation, or "disabled" new
			{
				next_state.gov_ggov1.x3a = gov_ggv1_Kigov*next_state.gov_ggov1.err2;
			}
			else
			{
				next_state.gov_ggov1.err3 = next_state.gov_ggov1.GovOutPut - next_state.gov_ggov1.x3;
				next_state.gov_ggov1.x3a = gov_ggv1_Kigov/gov_ggv1_Kpgov*next_state.gov_ggov1.err3;
			}
			next_state.gov_ggov1.fsrn = next_state.gov_ggov1.x2 + gov_ggv1_Kpgov*next_state.gov_ggov1.err2 + next_state.gov_ggov1.x3;

			//5 - Turbine LL
			x5a_now = gov_ggv1_Kturb*(next_state.gov_ggov1.FuelFlow - gov_ggv1_wfnl);

			if (gov_ggv1_Teng > 0)
			{
				//Update the stored value
				x5a_delayed[x5a_delayed_write_pos] = x5a_now;

				//Assign the oldest value
				next_state.gov_ggov1.x5a = x5a_delayed[x5a_delayed_read_pos];
			}
			else	//Zero length
			{
				//Just assign in
				next_state.gov_ggov1.x5a = x5a_now;
			}
			next_state.gov_ggov1.x5 = (1.0 - gov_ggv1_Tc/gov_ggv1_Tb)*next_state.gov_ggov1.x5b + gov_ggv1_Tc/gov_ggv1_Tb*next_state.gov_ggov1.x5a;
			if (gov_ggv1_Dm > 0.0)
			{
				//Mechanical power update
				next_state.pwr_mech = Rated_VA*(next_state.gov_ggov1.x5 - gov_ggv1_Dm*(next_state.omega/omega_ref - gen_base_set_vals.wref));

				//Check the limit
				if (next_state.pwr_mech > Overload_Limit_Value)
				{
					//Limit it
					next_state.pwr_mech = Overload_Limit_Value;

					//Fix the variable
					next_state.gov_ggov1.x5 = Overload_Limit_Value / Rated_VA + gov_ggv1_Dm*(next_state.omega/omega_ref - gen_base_set_vals.wref);
				}
			}
			else
			{
				//Mechanical power update
				next_state.pwr_mech = Rated_VA*(next_state.gov_ggov1.x5);

				//Check the limit
				if (next_state.pwr_mech > Overload_Limit_Value)
				{
					//Limit it
					next_state.pwr_mech = Overload_Limit_Value;

					//Fix the variable
					next_state.gov_ggov1.x5 = Overload_Limit_Value / Rated_VA;
				}
			}
			//Translate this into the torque model
			next_state.torque_mech = next_state.pwr_mech / next_state.omega;

			//10 - Temp detection LL
			if (gov_ggv1_Dm < 0.0)
			{
				next_state.gov_ggov1.x10a = next_state.gov_ggov1.FuelFlow * pow((next_state.omega/omega_ref),gov_ggv1_Dm);
			}
			else
			{
				next_state.gov_ggov1.x10a = next_state.gov_ggov1.FuelFlow;
			}
			next_state.gov_ggov1.x10 = (1.0 - gov_ggv1_Tsa/gov_ggv1_Tsb)*next_state.gov_ggov1.x10b + gov_ggv1_Tsa/gov_ggv1_Tsb*next_state.gov_ggov1.x10a;

			//7 - Turbine load integral control
			next_state.gov_ggov1.err7 = next_state.gov_ggov1.GovOutPut - next_state.gov_ggov1.x7;
			if (gov_ggv1_Kpload > 0.0)
			{
				next_state.gov_ggov1.x7a = gov_ggv1_Kiload/gov_ggv1_Kpload*next_state.gov_ggov1.err7;
			}
			else
			{
				next_state.gov_ggov1.x7a = gov_ggv1_Kiload*next_state.gov_ggov1.err7;
			}
			next_state.gov_ggov1.fsrtNoLim = next_state.gov_ggov1.x7 + gov_ggv1_Kpload*(-1.0 * next_state.gov_ggov1.x6 + gov_ggv1_Ldref*(gov_ggv1_Ldref/gov_ggv1_Kturb+gov_ggv1_wfnl));
			if (next_state.gov_ggov1.fsrtNoLim > 1.0)
			{
				next_state.gov_ggov1.fsrt = 1.0;
			}
			else
			{
				next_state.gov_ggov1.fsrt = next_state.gov_ggov1.fsrtNoLim;
			}

			//9 - Acceleration control
			next_state.gov_ggov1.x9 = 1.0/gov_ggv1_Ta*((next_state.omega/omega_ref - gen_base_set_vals.wref) - next_state.gov_ggov1.x9a);
			next_state.gov_ggov1.fsra = gov_ggv1_Ka*deltat*(gov_ggv1_aset - next_state.gov_ggov1.x9) + next_state.gov_ggov1.GovOutPut;

			//Pre-empt the low-value select, if needed
			if (gov_ggv1_fsrt_enable == false)
			{
				next_state.gov_ggov1.fsrt = 99999999.0;	//Big value
			}

			if (gov_ggv1_fsra_enable == false)
			{
				next_state.gov_ggov1.fsra = 99999999.0;	//Big value
			}

			if (gov_ggv1_fsrn_enable == false)
			{
				next_state.gov_ggov1.fsrn = 99999999.0;	//Big value
			}

			//Low value select
			if (next_state.gov_ggov1.fsrt < next_state.gov_ggov1.fsrn)
			{
				next_state.gov_ggov1.LowValSelect1 = next_state.gov_ggov1.fsrt;
			}
			else
			{
				next_state.gov_ggov1.LowValSelect1 = next_state.gov_ggov1.fsrn;
			}

			if (next_state.gov_ggov1.fsra < next_state.gov_ggov1.LowValSelect1)
			{
				next_state.gov_ggov1.LowValSelect = next_state.gov_ggov1.fsra;
			}
			else
			{
				next_state.gov_ggov1.LowValSelect = next_state.gov_ggov1.LowValSelect1;
			}

			if (next_state.gov_ggov1.LowValSelect > gov_ggv1_vmax)
			{
				next_state.gov_ggov1.GovOutPut = gov_ggv1_vmax;
			}
			else if (next_state.gov_ggov1.LowValSelect < gov_ggv1_vmin)
			{
				next_state.gov_ggov1.GovOutPut = gov_ggv1_vmin;
			}
			else
			{
				next_state.gov_ggov1.GovOutPut = next_state.gov_ggov1.LowValSelect;
			}

		}//End GGOV1 update

		//Default else - no updates because no governor

		//Exciter updates
		if (Exciter_type == SEXS)
		{
//			if (CVRenabled) {
//				if (CVR_PI) {
//					next_state.avr.x_cvr = curr_state.avr.x_cvr + (predictor_vals.avr.x_cvr + corrector_vals.avr.x_cvr)*deltath;
//					gen_base_set_vals.vseta = Vref + next_state.avr.x_cvr + (predictor_vals.avr.diff_f + corrector_vals.avr.diff_f) * 0.5 * kp_cvr;
//				}
//				else if (CVR_PID) {
//					next_state.avr.x_cvr = curr_state.avr.x_cvr + (predictor_vals.avr.x_cvr + corrector_vals.avr.x_cvr)*deltath;
//					temp_double = (predictor_vals.avr.diff_f + corrector_vals.avr.diff_f) * 0.5;
//					next_state.avr.xerr_cvr = temp_double * kd_cvr;
//					corrector_vals.avr.xerr_cvr = (next_state.avr.xerr_cvr - curr_state.avr.xerr_cvr) / deltat;
//					gen_base_set_vals.vseta = Vref + next_state.avr.x_cvr + temp_double * kp_cvr + corrector_vals.avr.xerr_cvr;
//				}
//
//				//Limit check
//				if (gen_base_set_vals.vseta >= vset_EMAX)
//					gen_base_set_vals.vsetb = vset_EMAX;
//
//				if (gen_base_set_vals.vseta <= vset_EMIN)
//					gen_base_set_vals.vsetb = vset_EMIN;
//
//				// Give value of vsetb to vset
//				gen_base_set_vals.vset = gen_base_set_vals.vsetb;
//			}

//			if (CVRenabled) {
//				temp_double = (predictor_vals.avr.diff_f + corrector_vals.avr.diff_f) * 0.5;
//				next_state.avr.xerr_cvr = temp_double * kd_cvr;
//				corrector_vals.avr.xerr_cvr = (next_state.avr.xerr_cvr - curr_state.avr.xerr_cvr) / deltat;
//				gen_base_set_vals.vadd = corrector_vals.avr.xerr_cvr + temp_double * kp_cvr;
//			}

			if (CVRenabled) {

				// Implementation for high order CVR control
				if (CVRmode == HighOrder) {
					if (Kd1 != 0) {
						next_state.avr.x_cvr1 = curr_state.avr.x_cvr1 + (corrector_vals.avr.x_cvr1 + predictor_vals.avr.x_cvr1)*deltath;
						next_state.avr.x_cvr2 = curr_state.avr.x_cvr2 + (corrector_vals.avr.x_cvr2 + predictor_vals.avr.x_cvr2)*deltath;
						gen_base_set_vals.vadd = (Kn1/Kd1) * next_state.avr.x_cvr1 + (Kn2/Kd1) * next_state.avr.x_cvr2 + kp_cvr * (predictor_vals.avr.diff_f + corrector_vals.avr.diff_f) * 0.5;
					}
					else {
						next_state.avr.x_cvr1 = curr_state.avr.x_cvr1 + (corrector_vals.avr.x_cvr1 + predictor_vals.avr.x_cvr1)*deltath;
						gen_base_set_vals.vadd = (Kn2/Kd2 - (Kd3 * Kn1)/(Kd2 * Kd2)) * next_state.avr.x_cvr1 + (kp_cvr + Kn1/Kd2) * (predictor_vals.avr.diff_f + corrector_vals.avr.diff_f) * 0.5;
					}

					//Limit check
					if (gen_base_set_vals.vadd >= vset_delta_MAX)
						gen_base_set_vals.vadd = vset_delta_MAX;

					if (gen_base_set_vals.vadd <= vset_delta_MIN)
						gen_base_set_vals.vadd = vset_delta_MIN;

				}
				// Implementation for first order CVR control with feedback loop
				else if (CVRmode == Feedback) {
					next_state.avr.x_cvr1 = curr_state.avr.x_cvr1 + (corrector_vals.avr.x_cvr1 + predictor_vals.avr.x_cvr1)*deltath;
					gen_base_set_vals.vadd_a = kp_cvr * (predictor_vals.avr.diff_f + corrector_vals.avr.diff_f) * 0.5 + next_state.avr.x_cvr1;

					//Limit check
					if (gen_base_set_vals.vadd_a >= vset_delta_MAX) {
						gen_base_set_vals.vadd = vset_delta_MAX;
					}
					else if (gen_base_set_vals.vadd_a <= vset_delta_MIN) {
						gen_base_set_vals.vadd = vset_delta_MIN;
					}
					else {
						gen_base_set_vals.vadd = gen_base_set_vals.vadd_a;
					}
				}

				// Give value to vset
				gen_base_set_vals.vset = gen_base_set_vals.vadd + Vref;
			}

			next_state.avr.xe = curr_state.avr.xe + (predictor_vals.avr.xe + corrector_vals.avr.xe)*deltath;
			next_state.avr.xb = curr_state.avr.xb + (predictor_vals.avr.xb + corrector_vals.avr.xb)*deltath;
		}//End SEXS update
		//Default else - no updates because no exciter

		//Nab per-unit omega, while we're at it
		omega_pu = next_state.omega/omega_ref;

		//Update generator current injection (technically is done "before" next powerflow)
		value_IGenerated[0] = next_state.EintVal[0]*YS1*omega_pu;
		value_IGenerated[1] = next_state.EintVal[1]*YS1*omega_pu;
		value_IGenerated[2] = next_state.EintVal[2]*YS1*omega_pu;

		//Copy everything back into curr_state, since we'll be back there
		memcpy(&curr_state,&next_state,sizeof(MAC_STATES));

		//Check convergence
		temp_double = fabs(curr_state.omega - prev_rotor_speed_val);

		//Update tracking variable
		prev_rotor_speed_val = curr_state.omega;

		//Update the frequency for powerflow, if we're mapped
		//Work around for a generator to dictate frequency
		if (mapped_freq_variable!=NULL)
		{
			//Set the value
			temp_double_freq_val = curr_state.omega/(2.0*PI);

			//Push it up
			mapped_freq_variable->setp<double>(temp_double_freq_val,*test_rlock);
		}

		//Resync power variables
		push_powerflow_values(false);

		//See what to check to determine if an exit is needed
		if (apply_rotor_speed_convergence == true)
		{
			//Determine our desired state - if rotor speed is settled, exit
			if (temp_double<=rotor_speed_convergence_criterion)
			{
				//See if we're an isochronous generator and check that
				if (is_isochronous_gen == true)
				{
					//Compute the difference from nominal
					temp_double = fabs(curr_state.omega - omega_ref);

					//Check it - use same convergence criterion
					if (temp_double<=rotor_speed_convergence_criterion)
					{
						//See if the voltage check needs to happen
						if (apply_voltage_mag_convergence == false)
						{
							//Ready to leave Delta mode
							return SM_EVENT;
						}
						//Default else - let voltage check happen
					}
					else	//Not converged - stay in deltamode
					{
						return SM_DELTA;
					}
				}//End is an isochronous generator
				else	//Normal generator
				{
					if (apply_voltage_mag_convergence == false)
					{
						//Ready to leave Delta mode
						return SM_EVENT;
					}
					//Default else - let it execute the code below
				}//End normal generator
			}
			else	//Not "converged" -- I would like to do another update
			{
				return SM_DELTA;	//Next delta update
									//Could theoretically request a reiteration, but we're not allowing that right now
			}
		}

		//Only check voltage if an exciter is present
		if ((apply_voltage_mag_convergence == true) && (Exciter_type != NO_EXC))
		{
			//Figure out the maximum voltage difference - reset the tracker
			temp_double = 0.0;

			//Loop through the phases - built on the assumption of three-phase
			for (loop_index=0; loop_index<3; loop_index++)
			{
				temp_mag_val = value_Circuit_V[loop_index].Mag();
				temp_mag_diff = fabs(temp_mag_val - prev_voltage_val[loop_index]);

				//See if it is bigger or not
				if (temp_mag_diff > temp_double)
				{
					temp_double = temp_mag_diff;
				}

				//Store the updated tracking value
				prev_voltage_val[loop_index] = temp_mag_val;
			}

			//See if we need to reiterate or not
			if (temp_double<=voltage_convergence_criterion)
			{
			//Ready to leave Delta mode
			return SM_EVENT;
		}
		else	//Not "converged" -- I would like to do another update
		{
			return SM_DELTA;	//Next delta update
								//Could theoretically request a reiteration, but we're not allowing that right now
			}
		}

		//Default else - no checks asked for, just bounce back to event
		return SM_EVENT;
	}//End corrector pass
}

//Module-level post update call
//useful_value is a pointer to a passed in complex value
//mode_pass 0 is the accumulation call
//mode_pass 1 is the "update our frequency" call
STATUS diesel_dg::post_deltaupdate(complex *useful_value, unsigned int mode_pass)
{
	if (mode_pass == 0)	//Accumulation pass
	{
		//Put the powerflow frequency in as our current rotor speed (should basically be this way already)
		curr_state.omega = useful_value->Re();

		//Update tracking variable - see if it was an exact second or not
		if (deltamode_supersec_endtime != deltamode_endtime)
		{
			prev_time = deltamode_supersec_endtime;
			prev_time_dbl = deltamode_endtime_dbl;
		}
		else	//It was, do an intentional cast so things don't get wierd
		{
			prev_time = deltamode_endtime;
			prev_time_dbl = (double)(deltamode_endtime);
		}
	}
	else
		return FAILED;	//Not sure how we get here, but fail us if we do

	return SUCCESS;	//Allways succeeds right now
}


//Object-level call, if needed
//int diesel_dg::deltaupdate(unsigned int64 dt, unsigned int iteration_count_val)	//Returns success/fail - interrupdate handles EVENT/DELTA
//{
//	return SUCCESS;	//Just indicate success right now
//}

//Applies dynamic equations for predictor/corrector sets
//Functionalized since they are identical
//Returns a SUCCESS/FAIL
//curr_time is the current states/information
//curr_delta is the calculated differentials
STATUS diesel_dg::apply_dynamics(MAC_STATES *curr_time, MAC_STATES *curr_delta, double deltaT)
{
	complex current_pu[3];
	complex Ipn0[3];
	complex temp_complex;
	double omega_pu;
	double temp_double_1, temp_double_2, temp_double_3, delomega, x0; 
	double torquenow, x5a_now;
	complex temp_current_val[3];
	double diff_f, temp_Vfd;

	//Powerflow update values already called before these - just use values directly

	//Convert current as well
	current_pu[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0] - generator_admittance[0][1]*value_Circuit_V[1] - generator_admittance[0][2]*value_Circuit_V[2])/current_base;
	current_pu[1] = (value_IGenerated[1] - generator_admittance[1][0]*value_Circuit_V[0] - generator_admittance[1][1]*value_Circuit_V[1] - generator_admittance[1][2]*value_Circuit_V[2])/current_base;
	current_pu[2] = (value_IGenerated[2] - generator_admittance[2][0]*value_Circuit_V[0] - generator_admittance[2][1]*value_Circuit_V[1] - generator_admittance[2][2]*value_Circuit_V[2])/current_base;

	// post currents
	current_val[0]=current_pu[0]*current_base;
	current_val[1]=current_pu[1]*current_base;
	current_val[2]=current_pu[2]*current_base;


	//Nab per-unit omega, while we're at it
	omega_pu = curr_time->omega/omega_ref;

	//Sequence them
	convert_abc_to_pn0(&current_pu[0],&Ipn0[0]);

	//Rotate current for current angle
	temp_complex = complex_exp(-1.0*curr_time->rotor_angle);
	curr_time->Irotated = temp_complex*complex(0.0,1.0)*Ipn0[0];

	//Get speed update - split for readability
	temp_double_1 =  -(Xqpp-Xl)/(Xqp-Xl)*curr_time->EpRotated.Re()*curr_time->Irotated.Re();
	temp_double_1 -=(Xdpp-Xl)/(Xdp-Xl)*curr_time->EpRotated.Im()*curr_time->Irotated.Im();
	temp_double_1 -=(Xdp-Xdpp)/(Xdp-Xl)*curr_time->Flux1d*curr_time->Irotated.Im();
	temp_double_1 +=(Xqp-Xqpp)/(Xqp-Xl)*curr_time->Flux2q*curr_time->Irotated.Re();
	temp_double_1 -=(Xqpp-Xdpp)*curr_time->Irotated.Re()*curr_time->Irotated.Im();
	temp_double_3 = Ipn0[1].Mag();
	temp_double_1 -=0.5*Rr*temp_double_3*temp_double_3;
	curr_time->torque_elec=-temp_double_1*Rated_VA/omega_ref; 
	temp_double_1 =(curr_time->torque_mech/(Rated_VA/omega_ref)-curr_time->torque_elec/(Rated_VA/omega_ref));
	temp_double_1 -=damping*(curr_time->omega-omega_ref)/omega_ref;

	curr_time->pwr_mech = curr_time->torque_mech*curr_time->omega;

	//Check to see if it needs to be limited -- individual governors do this below, but in case we have no governor
	if (curr_time->pwr_mech > Overload_Limit_Value)
	{
		//Limit it
		curr_time->pwr_mech = Overload_Limit_Value;

		//Fix the torque too, in case something uses it
		curr_time->torque_mech = Overload_Limit_Value / curr_time->omega;
	}

	temp_double_2 = omega_ref/(2.0*inertia);

	//Post the delta value
	curr_delta->omega = temp_double_1*temp_double_2;

	//Calculate rotor angle update
	curr_delta->rotor_angle = (omega_pu-1.0)*omega_ref;

	//Update flux values
	curr_delta->Flux1d = (-curr_time->Flux1d + curr_time->EpRotated.Im() - ((Xdp-Xl)*curr_time->Irotated.Re()))/Tdopp;
	curr_delta->Flux2q = (-curr_time->Flux2q - curr_time->EpRotated.Re() - ((Xqp-Xl)*curr_time->Irotated.Im()))/Tqopp;

	//Internal voltage updates - EqInt - again split for readability
	temp_double_1  = curr_time->Vfd - curr_time->EpRotated.Im();
	temp_double_2  = curr_time->Flux1d + (Xdp-Xl)*curr_time->Irotated.Re() - curr_time->EpRotated.Im();
	temp_double_2 /= (Xdp-Xl);
	temp_double_2 /= (Xdp-Xl);
	temp_double_3  = curr_time->Irotated.Re() - (Xdp-Xdpp)*temp_double_2;
	temp_double_1 -= (Xd-Xdp)*temp_double_3;

	//Post the update value
	curr_delta->EpRotated.SetImag(temp_double_1/Tdop);

	//Internal voltage updates - EdInt - again split for readability
	temp_double_1  = -curr_time->EpRotated.Re();
	temp_double_2  = curr_time->Flux2q + (Xqp-Xl)*curr_time->Irotated.Im() + curr_time->EpRotated.Re();
	temp_double_2 /= (Xqp-Xl);
	temp_double_2 /= (Xqp-Xl);
	temp_double_3  = curr_time->Irotated.Im() - (Xqp-Xqpp)*temp_double_2;
	temp_double_1 += (Xq-Xqp)*temp_double_3;

	//Post the update value
	curr_delta->EpRotated.SetReal(temp_double_1/Tqop);

	//Governor updates, if relevant
	if (Governor_type == DEGOV1)	//Woodward Governor
	{
		//Governor actuator updates - threshold first
		if (curr_time->gov_degov1.x4>gov_degov1_TMAX)
			curr_time->gov_degov1.x4 = gov_degov1_TMAX;

		if (curr_time->gov_degov1.x4<gov_degov1_TMIN)
			curr_time->gov_degov1.x4 = gov_degov1_TMIN;

		//Find throttle
		curr_time->gov_degov1.throttle = gov_degov1_T4*curr_time->gov_degov1.x6 + curr_time->gov_degov1.x4;

		//Store this value into "the array"
		torque_delay[torque_delay_write_pos]=curr_time->gov_degov1.throttle;

		//Extract the "delayed" value
		torquenow = torque_delay[torque_delay_read_pos];

		//Calculate the mechanical power for this time
		curr_time->torque_mech = (Rated_VA/omega_ref)*torquenow;
		curr_time->pwr_mech = curr_time->torque_mech*curr_time->omega;

		//See if it exceeded the limit -- if so, cap it
		if (curr_time->pwr_mech > Overload_Limit_Value)
		{
			//Limit it
			curr_time->pwr_mech = Overload_Limit_Value;

			//Fix the torque variable
			curr_time->torque_mech = Overload_Limit_Value / curr_time->omega;
		}

		//Compute the offset currently
		temp_double_1 = gen_base_set_vals.wref - curr_time->omega/omega_ref-gov_degov1_R*curr_time->gov_degov1.throttle;
		
		//Update variables
		curr_delta->gov_degov1.x2 = (temp_double_1-curr_time->gov_degov1.x1-gov_degov1_T1*curr_time->gov_degov1.x2)/(gov_degov1_T1*gov_degov1_T2);
		curr_delta->gov_degov1.x1 = curr_time->gov_degov1.x2;

		//Electric control box updates
		temp_double_1 = gov_degov1_T3*curr_time->gov_degov1.x2 + curr_time->gov_degov1.x1;

		//Updates
		curr_delta->gov_degov1.x5 = (gov_degov1_K*temp_double_1-curr_time->gov_degov1.x5)/gov_degov1_T5;
		curr_delta->gov_degov1.x6 = (curr_time->gov_degov1.x5 - curr_time->gov_degov1.x6)/gov_degov1_T6;
		curr_delta->gov_degov1.x4 = curr_time->gov_degov1.x6;

		//Anti-windup check
		if (((curr_time->gov_degov1.x4>=gov_degov1_TMAX) && (curr_time->gov_degov1.x6>0)) || ((curr_time->gov_degov1.x4<=gov_degov1_TMIN) && (curr_time->gov_degov1.x6<0)))
		{
			curr_delta->gov_degov1.x4 = 0;
		}
	}//End Woodward updates
	else if (Governor_type == GAST)	//Gast Turbine Governor
	{
		//Governor actuator updates - threshold first
		if (curr_time->gov_gast.x1 > gov_gast_VMAX)
			curr_time->gov_gast.x1 = gov_gast_VMAX;

		if (curr_time->gov_gast.x1 < gov_gast_VMIN)
			curr_time->gov_gast.x1 = gov_gast_VMIN;

		//Find throttle -- replace with GAST
		curr_time->gov_gast.throttle = curr_time->gov_gast.x2;

		//Assign the throttle value to torquenow
		torquenow=curr_time->gov_gast.throttle;

		//Calculate the mechanical power for this time
		curr_time->torque_mech = (Rated_VA/omega_ref)*torquenow;
		curr_time->pwr_mech = curr_time->torque_mech*curr_time->omega;

		//See if it exceeded the limit -- if so, cap it
		if (curr_time->pwr_mech > Overload_Limit_Value)
		{
			//Limit it
			curr_time->pwr_mech = Overload_Limit_Value;

			//Fix the torque variable
			curr_time->torque_mech = Overload_Limit_Value / curr_time->omega;
		}

		//Compute the offset currently
		delomega = curr_time->gov_gast.throttle - (curr_time->omega/omega_ref-1)*gov_gast_R; 
		if (delomega <= gov_gast_AT+gov_gast_KT*(gov_gast_AT-curr_time->gov_gast.x3))
		{
			x0=delomega;
		}
		else
		{
			x0=gov_gast_AT+gov_gast_KT*(gov_gast_AT-curr_time->gov_gast.x3);
		}

//		x0 = min(,gov_gast_AT+gov_gast_KT*(gov_gast_AT-curr_time->gov_gast.x3)); 
//		LL+K*(LL-G1.machine_parameters.curr.gov.x3

		//Update variables
		curr_delta->gov_gast.x1 = (x0 - curr_time->gov_gast.x1)/gov_gast_T1;
		curr_delta->gov_gast.x2 = (curr_time->gov_gast.x1 - curr_time->gov_gast.x2)/gov_gast_T2;
		curr_delta->gov_gast.x3 = (curr_time->gov_gast.x2 - curr_time->gov_gast.x3)/gov_gast_T3;

		//Anti-windup check
		if (((curr_time->gov_gast.x1>=gov_gast_VMAX) && (x0>0)) || ((curr_time->gov_gast.x1<=gov_gast_VMIN) && (x0<0)))
		{
			curr_delta->gov_gast.x1 = 0;
		}
	}//End GAST updates
	else if (Governor_type == P_CONSTANT)
	{
		// Recalculate the Pelec output based on the predictor step results
		//Compute the "present" electric power value before anything gets updated for the new timestep
		temp_current_val[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0] - generator_admittance[0][1]*value_Circuit_V[1] - generator_admittance[0][2]*value_Circuit_V[2]);
		temp_current_val[1] = (value_IGenerated[1] - generator_admittance[1][0]*value_Circuit_V[0] - generator_admittance[1][1]*value_Circuit_V[1] - generator_admittance[1][2]*value_Circuit_V[2]);
		temp_current_val[2] = (value_IGenerated[2] - generator_admittance[2][0]*value_Circuit_V[0] - generator_admittance[2][1]*value_Circuit_V[1] - generator_admittance[2][2]*value_Circuit_V[2]);
		//Update the output power variable
		complex pwr_electric_dynamics = value_Circuit_V[0]*~temp_current_val[0] + value_Circuit_V[1]*~temp_current_val[1] + value_Circuit_V[2]*~temp_current_val[2];

		//1 - Pelec measurement
		curr_delta->gov_pconstant.x1 = 1.0/pconstant_Tpelec*(pwr_electric_dynamics.Re() / Rated_VA - curr_time->gov_pconstant.x1);

		// PI controller for P CONSTANT mode
		double diff_Pelec = gen_base_set_vals.Pref - curr_time->gov_pconstant.x1;
//		double diff_Pelec = gen_base_set_vals.Pref - pwr_electric_dynamics.Re() / Rated_VA;
		curr_delta->gov_pconstant.x_Pconstant = diff_Pelec * ki_Pconstant;

		//4 - Turbine actuator
		curr_time->gov_pconstant.err4 = curr_time->gov_pconstant.GovOutPut - curr_time->gov_pconstant.x4;
		curr_time->gov_pconstant.x4a = 1.0/pconstant_Tact*curr_time->gov_pconstant.err4;
		if (curr_time->gov_pconstant.x4a > pconstant_ropen)
		{
			curr_time->gov_pconstant.x4b = pconstant_ropen;
		}
		else if (curr_time->gov_pconstant.x4a < pconstant_rclose)
		{
			curr_time->gov_pconstant.x4b = pconstant_rclose;
		}
		else
		{
			curr_time->gov_pconstant.x4b = curr_time->gov_pconstant.x4a;
		}
		curr_delta->gov_pconstant.x4 = curr_time->gov_pconstant.x4b;
		curr_time->gov_pconstant.ValveStroke = curr_time->gov_pconstant.x4;
		if (pconstant_Flag == 0)
		{
			curr_time->gov_pconstant.FuelFlow = curr_time->gov_pconstant.ValveStroke * 1.0;
		}
		else if (pconstant_Flag == 1)
		{
			curr_time->gov_pconstant.FuelFlow = curr_time->gov_pconstant.ValveStroke * curr_time->omega / omega_ref;
		}
		else
		{
			gl_error("wrong pconstant_Flag");
			return FAILED;
		}

		// ---> Use the updated Fuelflow value to calculate final Pmech and Tmech output from the governor

		//5 - Turbine LL
		x5a_now = pconstant_Kturb*(curr_time->gov_pconstant.FuelFlow - pconstant_wfnl);

		//Check on the delay
		if (pconstant_Teng > 0)
		{
			//Store the value
			x5a_delayed[x5a_delayed_write_pos] = x5a_now;

			//Read the delayed value
			curr_time->gov_pconstant.x5a = x5a_delayed[x5a_delayed_read_pos];
		}
		else
		{
			curr_time->gov_pconstant.x5a = x5a_now;
		}
		curr_delta->gov_pconstant.x5b = 1.0/pconstant_Tb*(curr_time->gov_pconstant.x5a - curr_time->gov_pconstant.x5b);
		curr_time->gov_pconstant.x5 = (1.0 - pconstant_Tc/pconstant_Tb)*curr_time->gov_pconstant.x5b + pconstant_Tc/pconstant_Tb*curr_time->gov_pconstant.x5a;

		curr_time->pwr_mech = Rated_VA*curr_time->gov_pconstant.x5;

		//See if it exceeded the limit -- if so, cap it
		if (curr_time->pwr_mech > Overload_Limit_Value)
		{
			//Limit it
			curr_time->pwr_mech = Overload_Limit_Value;

			//Fix the state variable
			curr_time->gov_pconstant.x5 = Overload_Limit_Value / Rated_VA;
		}

		//Translate this into the torque model
		curr_time->torque_mech = curr_time->pwr_mech / curr_time->omega;

	}//End P_CONSTANT updates
	else if ((Governor_type == GGOV1) || (Governor_type == GGOV1_OLD))
	{

		//1 - Pelec measurement
		curr_delta->gov_ggov1.x1 = 1.0/gov_ggv1_Tpelec*(curr_time->pwr_electric.Re() / Rated_VA - curr_time->gov_ggov1.x1);

		//8 - Supervisory load control
		if (curr_time->gov_ggov1.x8a > (1.1 * gov_ggv1_r))
		{
			curr_time->gov_ggov1.x8 = 1.1 * gov_ggv1_r;
			curr_delta->gov_ggov1.x8a = 0.0;
		}
		else if (curr_time->gov_ggov1.x8a < (-1.1 * gov_ggv1_r))
		{
			curr_time->gov_ggov1.x8 = -1.1 * gov_ggv1_r;
			curr_delta->gov_ggov1.x8a = 0.0;
		}
		else
		{
			curr_delta->gov_ggov1.x8a = gov_ggv1_Kimw*(gov_ggv1_Pmwset/Rated_VA - curr_time->gov_ggov1.x1);
			curr_time->gov_ggov1.x8 = curr_time->gov_ggov1.x8a;
		}

		//2 - governor differntial control
		//Rselect switch
		if (gov_ggv1_rselect == 1)
		{
			curr_time->gov_ggov1.RselectValue = curr_time->gov_ggov1.x1;
		}
		else if (gov_ggv1_rselect == -1)
		{
			curr_time->gov_ggov1.RselectValue = curr_time->gov_ggov1.ValveStroke;
		}
		else if (gov_ggv1_rselect == -2)
		{
			curr_time->gov_ggov1.RselectValue = curr_time->gov_ggov1.GovOutPut;
		}
		else if (gov_ggv1_rselect == 0)
		{
			curr_time->gov_ggov1.RselectValue = 0.0;
		}
		else
		{
			gl_error("Wrong ggv1_rselect parameter");
			return FAILED;
		}

		//Error deadband
		curr_time->gov_ggov1.werror = curr_time->omega/omega_ref - gen_base_set_vals.wref;
		curr_time->gov_ggov1.err2a = gen_base_set_vals.Pref + curr_time->gov_ggov1.x8 - curr_time->gov_ggov1.werror - gov_ggv1_r*curr_time->gov_ggov1.RselectValue;
		if (curr_time->gov_ggov1.err2a > gov_ggv1_maxerr)
		{
			curr_time->gov_ggov1.err2 = gov_ggv1_maxerr;
		}
		else if (curr_time->gov_ggov1.err2a < gov_ggv1_minerr)
		{
			curr_time->gov_ggov1.err2 = gov_ggv1_minerr;
		}
		else if ((curr_time->gov_ggov1.err2a <= gov_ggv1_db) && (curr_time->gov_ggov1.err2a >= -gov_ggv1_db))
		{
			curr_time->gov_ggov1.err2 = 0.0;
		}
		else
		{
			if (curr_time->gov_ggov1.err2a > 0.0)
			{
				curr_time->gov_ggov1.err2 = (gov_ggv1_maxerr/(gov_ggv1_maxerr-gov_ggv1_db))*curr_time->gov_ggov1.err2a - (gov_ggv1_maxerr*gov_ggv1_db/(gov_ggv1_maxerr-gov_ggv1_db));
			}
			else if (curr_time->gov_ggov1.err2a < 0.0)
			{
				curr_time->gov_ggov1.err2 = (gov_ggv1_minerr/(gov_ggv1_minerr+gov_ggv1_db))*curr_time->gov_ggov1.err2a + (gov_ggv1_minerr*gov_ggv1_db/(gov_ggv1_minerr+gov_ggv1_db));
			}
		}
		curr_delta->gov_ggov1.x2a = 1.0/gov_ggv1_Tdgov*(curr_time->gov_ggov1.err2 - curr_time->gov_ggov1.x2a);
		curr_time->gov_ggov1.x2 = gov_ggv1_Kpgov/gov_ggv1_Tdgov*(curr_time->gov_ggov1.err2 - curr_time->gov_ggov1.x2a);

		//Governor integral control
		//See which specific GGOV1 we are
		if ((Governor_type == GGOV1_OLD) || ((Governor_type == GGOV1) && (gov_ggv1_Kpgov == 0.0)))	//Old implementation, or "disabled" new
		{
			curr_time->gov_ggov1.x3a = gov_ggv1_Kigov*curr_time->gov_ggov1.err2;
		}
		else
		{
			curr_time->gov_ggov1.err3 = curr_time->gov_ggov1.GovOutPut - curr_time->gov_ggov1.x3;
			curr_time->gov_ggov1.x3a = gov_ggv1_Kigov/gov_ggv1_Kpgov*curr_time->gov_ggov1.err3;
		}

		curr_delta->gov_ggov1.x3 = curr_time->gov_ggov1.x3a;
		curr_time->gov_ggov1.fsrn = curr_time->gov_ggov1.x2 + gov_ggv1_Kpgov*curr_time->gov_ggov1.err2 + curr_time->gov_ggov1.x3;

		//4 - Turbine actuator
		curr_time->gov_ggov1.err4 = curr_time->gov_ggov1.GovOutPut - curr_time->gov_ggov1.x4;
		curr_time->gov_ggov1.x4a = 1.0/gov_ggv1_Tact*curr_time->gov_ggov1.err4;
		if (curr_time->gov_ggov1.x4a > gov_ggv1_ropen)
		{
			curr_time->gov_ggov1.x4b = gov_ggv1_ropen;
		}
		else if (curr_time->gov_ggov1.x4a < gov_ggv1_rclose)
		{
			curr_time->gov_ggov1.x4b = gov_ggv1_rclose;
		}
		else
		{
			curr_time->gov_ggov1.x4b = curr_time->gov_ggov1.x4a;
		}
		curr_delta->gov_ggov1.x4 = curr_time->gov_ggov1.x4b;
		curr_time->gov_ggov1.ValveStroke = curr_time->gov_ggov1.x4;
		if (gov_ggv1_Flag == 0)
		{
			curr_time->gov_ggov1.FuelFlow = curr_time->gov_ggov1.ValveStroke * 1.0;
		}
		else if (gov_ggv1_Flag == 1)
		{
			curr_time->gov_ggov1.FuelFlow = curr_time->gov_ggov1.ValveStroke * curr_time->omega / omega_ref;
		}
		else
		{
			gl_error("wrong ggv1_Flag");
			return FAILED;
		}

		// ---> Use the updated Fuelflow value to calculate final Pmech and Tmech output from the governor

		//5 - Turbine LL
		x5a_now = gov_ggv1_Kturb*(curr_time->gov_ggov1.FuelFlow - gov_ggv1_wfnl);

		//Check on the delay
		if (gov_ggv1_Teng > 0)
		{
			//Store the value
			x5a_delayed[x5a_delayed_write_pos] = x5a_now;

			//Read the delayed value
			curr_time->gov_ggov1.x5a = x5a_delayed[x5a_delayed_read_pos];
		}
		else
		{
			curr_time->gov_ggov1.x5a = x5a_now;
		}
		curr_delta->gov_ggov1.x5b = 1.0/gov_ggv1_Tb*(curr_time->gov_ggov1.x5a - curr_time->gov_ggov1.x5b);
		curr_time->gov_ggov1.x5 = (1.0 - gov_ggv1_Tc/gov_ggv1_Tb)*curr_time->gov_ggov1.x5b + gov_ggv1_Tc/gov_ggv1_Tb*curr_time->gov_ggov1.x5a;
		if (gov_ggv1_Dm > 0.0)	//Mechanical power set
		{
			curr_time->pwr_mech = Rated_VA*(curr_time->gov_ggov1.x5 - gov_ggv1_Dm*(curr_time->omega/omega_ref - gen_base_set_vals.wref));

			//See if it exceeded the limit -- if so, cap it
			if (curr_time->pwr_mech > Overload_Limit_Value)
			{
				//Limit it
				curr_time->pwr_mech = Overload_Limit_Value;

				//Fix the torque variable
				curr_time->torque_mech = Overload_Limit_Value / Rated_VA + gov_ggv1_Dm*(curr_time->omega/omega_ref - gen_base_set_vals.wref);
			}
		}
		else
		{
			curr_time->pwr_mech = Rated_VA*curr_time->gov_ggov1.x5;

			//See if it exceeded the limit -- if so, cap it
			if (curr_time->pwr_mech > Overload_Limit_Value)
			{
				//Limit it
				curr_time->pwr_mech = Overload_Limit_Value;

				//Fix the state variable
				curr_time->gov_ggov1.x5 = Overload_Limit_Value / Rated_VA;
			}
		}
		//Translate this into the torque model
		curr_time->torque_mech = curr_time->pwr_mech / curr_time->omega;		

		//10 - Temp detection LL
		if (gov_ggv1_Dm < 0.0)
		{
			curr_time->gov_ggov1.x10a = curr_time->gov_ggov1.FuelFlow * pow((curr_time->omega/omega_ref),gov_ggv1_Dm);
		}
		else
		{
			curr_time->gov_ggov1.x10a = curr_time->gov_ggov1.FuelFlow;
		}
		curr_delta->gov_ggov1.x10b = 1.0/gov_ggv1_Tsb*(curr_time->gov_ggov1.x10a - curr_time->gov_ggov1.x10b);
		curr_time->gov_ggov1.x10 = (1.0 - gov_ggv1_Tsa/gov_ggv1_Tsb)*curr_time->gov_ggov1.x10b + gov_ggv1_Tsa/gov_ggv1_Tsb*curr_time->gov_ggov1.x10a;

		//6 - Turbine Load Limiter
		curr_delta->gov_ggov1.x6 = 1.0/gov_ggv1_Tfload*(curr_time->gov_ggov1.x10 - curr_time->gov_ggov1.x6);

		//7 - Turbine Load Integral Control
		curr_time->gov_ggov1.err7 = curr_time->gov_ggov1.GovOutPut - curr_time->gov_ggov1.x7;
		if (gov_ggv1_Kpload > 0.0)
		{
			curr_time->gov_ggov1.x7a = gov_ggv1_Kiload/gov_ggv1_Kpload*curr_time->gov_ggov1.err7;
		}
		else
		{
			curr_time->gov_ggov1.x7a = gov_ggv1_Kiload*curr_time->gov_ggov1.err7;
		}
		curr_delta->gov_ggov1.x7 = curr_time->gov_ggov1.x7a;
		curr_time->gov_ggov1.fsrtNoLim = curr_time->gov_ggov1.x7 + gov_ggv1_Kpload*(-1.0 * curr_time->gov_ggov1.x6 + gov_ggv1_Ldref*(gov_ggv1_Ldref/gov_ggv1_Kturb+gov_ggv1_wfnl));
		if (curr_time->gov_ggov1.fsrtNoLim > 1.0)
		{
			curr_time->gov_ggov1.fsrt = 1.0;
		}
		else
		{
			curr_time->gov_ggov1.fsrt = curr_time->gov_ggov1.fsrtNoLim;
		}

		//9 - Acceleration control
		curr_delta->gov_ggov1.x9a = 1.0/gov_ggv1_Ta*((curr_time->omega/omega_ref - gen_base_set_vals.wref) - curr_time->gov_ggov1.x9a);
		curr_time->gov_ggov1.x9 = 1.0/gov_ggv1_Ta*((curr_time->omega/omega_ref - gen_base_set_vals.wref) - curr_time->gov_ggov1.x9a);
		curr_time->gov_ggov1.fsra = gov_ggv1_Ka*deltaT*(gov_ggv1_aset - curr_time->gov_ggov1.x9) + curr_time->gov_ggov1.GovOutPut;

		//Pre-empt the low-value select, if needed
		if (gov_ggv1_fsrt_enable == false)
		{
			curr_time->gov_ggov1.fsrt = 99999999.0;	//Big value
		}

		if (gov_ggv1_fsra_enable == false)
		{
			curr_time->gov_ggov1.fsra = 99999999.0;	//Big value
		}

		if (gov_ggv1_fsrn_enable == false)
		{
			curr_time->gov_ggov1.fsrn = 99999999.0;	//Big value
		}

		//Low value select
		if (curr_time->gov_ggov1.fsrt < curr_time->gov_ggov1.fsrn)
		{
			curr_time->gov_ggov1.LowValSelect1 = curr_time->gov_ggov1.fsrt;
		}
		else
		{
			curr_time->gov_ggov1.LowValSelect1 = curr_time->gov_ggov1.fsrn;
		}

		if (curr_time->gov_ggov1.fsra < curr_time->gov_ggov1.LowValSelect1)
		{
			curr_time->gov_ggov1.LowValSelect = curr_time->gov_ggov1.fsra;
		}
		else
		{
			curr_time->gov_ggov1.LowValSelect = curr_time->gov_ggov1.LowValSelect1;
		}

		if (curr_time->gov_ggov1.LowValSelect > gov_ggv1_vmax)
		{
			curr_time->gov_ggov1.GovOutPut = gov_ggv1_vmax;
		}
		else if (curr_time->gov_ggov1.LowValSelect < gov_ggv1_vmin)
		{
			curr_time->gov_ggov1.GovOutPut = gov_ggv1_vmin;
		}
		else
		{
			curr_time->gov_ggov1.GovOutPut = curr_time->gov_ggov1.LowValSelect;
		}
	}//End GGOV1 updates
	//Default else - no governor to update

	//AVR updates, if relevant
	if (Exciter_type == SEXS)
	{
		if (Q_constant_mode == true) {

			// Obtain reactive power output in p.u.
			temp_double_1 = curr_time->pwr_electric.Im() / Rated_VA;

			//Calculate the difference from the desired set point
			temp_double_2 = gen_base_set_vals.Qref - temp_double_1;
		}
		else {

//			// If CVR control is enabled, gen_base_set_vals.vset will be changed based on frequency deviation
//			if (CVRenabled) {
//				curr_delta->avr.diff_f = (omega_pu - 1.0);
//				curr_delta->avr.x_cvr = (omega_pu - 1.0) * ki_cvr + (gen_base_set_vals.vsetb - gen_base_set_vals.vseta) * kt_cvr; // Same for PI and PID controller
//			}

			// If CVR control is enabled with second order transfer function
			if (CVRenabled) {

				curr_delta->avr.diff_f = (omega_pu - 1.0);

				// Implementation for high order CVR control
				if (CVRmode == HighOrder) {
					if (Kd1 != 0) {
						curr_delta->avr.x_cvr1 = curr_time->avr.x_cvr1 * (-Kd2/Kd1) + curr_time->avr.x_cvr2 * (-Kd3/Kd1) + curr_delta->avr.diff_f;
						curr_delta->avr.x_cvr2 = curr_time->avr.x_cvr1;
					}
					else {
						curr_delta->avr.x_cvr1 = curr_time->avr.x_cvr1 * (-Kd3/Kd2) + curr_delta->avr.diff_f;
					}
				}

				// Implementation for first order CVR control with feedback loop
				else if (CVRmode == Feedback) {
					temp_double_1 = curr_delta->avr.diff_f * ki_cvr + (gen_base_set_vals.vadd - gen_base_set_vals.vadd_a) * kw_cvr;
					curr_delta->avr.x_cvr1 = (temp_double_1 - curr_time->avr.x_cvr1 * Kd1)/Kd2;
				}
			}

			//Get the average magnitude first
			temp_double_1 = (value_Circuit_V[0].Mag() + value_Circuit_V[1].Mag() + value_Circuit_V[2].Mag())/voltage_base/3.0;

			//Calculate the difference from the desired set point
			temp_double_2 = gen_base_set_vals.vset - temp_double_1;
		}

		//First update variable
		curr_delta->avr.xb = (temp_double_2 + curr_time->avr.bias - curr_time->avr.xb)/exc_TB;

		//Figure out v_r
		temp_double_1 = curr_time->avr.xb + exc_TC*curr_delta->avr.xb;

		//Update variable
		curr_delta->avr.xe = (exc_KA*temp_double_1 - curr_time->avr.xe)/exc_TA;

		//Anti-windup check
		if (((curr_time->avr.xe>=exc_EMAX) && (curr_delta->avr.xe>0)) || ((curr_time->avr.xe<=exc_EMIN) && (curr_delta->avr.xe<0)))
		{
			curr_delta->avr.xe = 0.0;
		}

		//Limit check
		if (curr_time->avr.xe >= exc_EMAX)
			curr_time->avr.xe = exc_EMAX;

		if (curr_time->avr.xe <= exc_EMIN)
			curr_time->avr.xe = exc_EMIN;

		if (Q_constant_mode == true) {
			// Add PI control for the control of Q output
			curr_delta->avr.xfd = curr_time->avr.xe*ki_Qconstant;
		}
		else {

			//Apply update
			curr_time->Vfd = curr_time->avr.xe;

//			// If CVR control is enabled, field voltage will be affected by frequency deviation
//			if (CVRenabled) {
//
//				// Obtain frequency deviation
//				curr_delta->avr.diff_f = omega_pu - 1.0;
//
//				temp_Vfd = curr_time->avr.xe + gen_base_set_vals.vadd;
//
//				//Limit check
//				if (temp_Vfd >= exc_EMAX)
//					temp_Vfd = exc_EMAX;
//
//				if (temp_Vfd <= exc_EMIN)
//					temp_Vfd = exc_EMIN;
//
//				//Apply update
//				curr_time->Vfd = temp_Vfd;
//
//			}
//			else {
//
//				//Apply update
//				curr_time->Vfd = curr_time->avr.xe;
//			}
		}

	}//End AVR update for SEXS exciter
	else	//No exciter - just zero stuff for paranoia purposes
	{
		curr_delta->avr.xb = 0.0;
		curr_delta->avr.xe = 0.0;
	}//End no AVR/Exciter

	return SUCCESS;	//Always succeeds for now, but could have error checks later
}

//Initializes dynamic equations for first entry
//Returns a SUCCESS/FAIL
//curr_time is the initial states/information
STATUS diesel_dg::init_dynamics(MAC_STATES *curr_time)
{
	complex voltage_pu[3];
	complex current_pu[3];
	complex Vpn0[3];
	complex Ipn0[3];
	complex temp_complex_1, temp_complex_2;
	double omega_pu;
	double temp_double_1, temp_double_2, temp_double_3;
	unsigned int index_val;

	//Convert voltage to per-unit -- already pulled in outer function
	voltage_pu[0] = value_Circuit_V[0]/voltage_base;
	voltage_pu[1] = value_Circuit_V[1]/voltage_base;
	voltage_pu[2] = value_Circuit_V[2]/voltage_base;

	//Convert current as well
	current_pu[0] = (value_IGenerated[0] - generator_admittance[0][0]*value_Circuit_V[0] - generator_admittance[0][1]*value_Circuit_V[1] - generator_admittance[0][2]*value_Circuit_V[2])/current_base;
	current_pu[1] = (value_IGenerated[1] - generator_admittance[1][0]*value_Circuit_V[0] - generator_admittance[1][1]*value_Circuit_V[1] - generator_admittance[1][2]*value_Circuit_V[2])/current_base;
	current_pu[2] = (value_IGenerated[2] - generator_admittance[2][0]*value_Circuit_V[0] - generator_admittance[2][1]*value_Circuit_V[1] - generator_admittance[2][2]*value_Circuit_V[2])/current_base;

	// post currents
	current_val[0]=current_pu[0]*current_base;
	current_val[1]=current_pu[1]*current_base;
	current_val[2]=current_pu[2]*current_base;
	
	//Compute initial power
	curr_time->pwr_electric = (voltage_pu[0]*~current_pu[0]+voltage_pu[1]*~current_pu[1]+voltage_pu[2]*~current_pu[2])*voltage_base*current_base;

	//Nab per-unit omega, while we're at it
	omega_pu = curr_time->omega/omega_ref;

	//Sequence them
	convert_abc_to_pn0(&voltage_pu[0],&Vpn0[0]);
	convert_abc_to_pn0(&current_pu[0],&Ipn0[0]);

	//Calculate internal voltage for rotor angle
	temp_complex_1 = Vpn0[0]+complex(Ra,Xq)*Ipn0[0];
	curr_time->rotor_angle = temp_complex_1.Arg();

	//Now figure out the internal voltage based on the subtransient model
	temp_complex_1 = (Vpn0[0]+complex(Ra,Xdpp)*Ipn0[0])/omega_pu; // /omega_pu to be able to initialize at omega <> 60Hz

	//Figure out the rotation
	temp_complex_2 = complex_exp(-1.0*curr_time->rotor_angle);
	curr_time->Irotated = temp_complex_2*complex(0.0,1.0)*Ipn0[0];
	curr_time->VintRotated = (temp_complex_2*complex(0.0,1.0)*temp_complex_1);

	//Compute vr
	temp_complex_1 = temp_complex_2*complex(0.0,1.0)*Vpn0[0];

	//Compute EpRotated initial value - split for readability
	curr_time->EpRotated = temp_complex_1.Re() + Ra*curr_time->Irotated.Re() - Xqp*curr_time->Irotated.Im();
	curr_time->EpRotated += complex(0.0,1.0)*(temp_complex_1.Im() + Ra*curr_time->Irotated.Im() + Xdp*curr_time->Irotated.Re());

	//Update flux terms
	curr_time->Flux1d = curr_time->EpRotated.Im() - (Xdp-Xl)*curr_time->Irotated.Re();
	curr_time->Flux2q = -curr_time->EpRotated.Re() - (Xqp-Xl)*curr_time->Irotated.Im();

	//compute initial field voltage
	temp_double_1  = -curr_time->EpRotated.Im();
	temp_double_2  = curr_time->Irotated.Re();
	temp_double_3  = curr_time->Flux1d + (Xdp-Xl)*curr_time->Irotated.Re() - curr_time->EpRotated.Im();
	temp_double_3 /= (Xdp-Xl);
	temp_double_3 /= (Xdp-Xl);
	temp_double_2 -= (Xdp-Xdpp)*temp_double_3;
	temp_double_1 -= (Xd-Xdp)*temp_double_2;
	
	//Set field voltage
	curr_time->Vfd = -1.0*temp_double_1;
	curr_time->avr.xfd = curr_time->Vfd;

	//Now compute initial mechanical torque - split for readability
	temp_double_1  = -(Xqpp-Xl)/(Xqp-Xl)*curr_time->EpRotated.Re()*curr_time->Irotated.Re();
	temp_double_1 -= (Xdpp-Xl)/(Xdp-Xl)*curr_time->EpRotated.Im()*curr_time->Irotated.Im();
	temp_double_1 -= (Xdp-Xdpp)/(Xdp-Xl)*curr_time->Flux1d*curr_time->Irotated.Im();
	temp_double_1 += (Xqp-Xqpp)/(Xqp-Xl)*curr_time->Flux2q*curr_time->Irotated.Re();
	temp_double_1 -= (Xqpp-Xdpp)*curr_time->Irotated.Re()*curr_time->Irotated.Im();
	temp_double_1 -= 0.5*Rr*Ipn0[1].Mag()*Ipn0[1].Mag();
	curr_time->torque_elec = -1.0*temp_double_1*Rated_VA/omega_ref;
	temp_double_1 -= damping*(curr_time->omega-omega_ref)/omega_ref;

	//Set the initial power
	curr_time->torque_mech = -1.0*temp_double_1*Rated_VA/omega_ref;
	curr_time->pwr_mech = curr_time->torque_mech*curr_time->omega;

	//Check it and limit it, if necessary
	if (curr_time->pwr_mech > Overload_Limit_Value)
	{
		//Limit it
		curr_time->pwr_mech = Overload_Limit_Value;

		//Fix the mechanical torque too, even though it probably gets caught below
		curr_time->torque_mech = Overload_Limit_Value / curr_time->omega;
	}

	//Governor initial conditions
	if (Governor_type == DEGOV1)
	{
		curr_time->gov_degov1.x1 = 0;
		curr_time->gov_degov1.x2 = 0;
		curr_time->gov_degov1.x5 = 0;
		curr_time->gov_degov1.x6 = 0;
		curr_time->gov_degov1.x4 = curr_time->torque_mech/(Rated_VA/omega_ref);
		curr_time->gov_degov1.throttle = curr_time->gov_degov1.x4;	//Init to Tmech

		if (gen_base_set_vals.wref < -90.0)	//Should be -99 if non-inited
		{
			gen_base_set_vals.wref = curr_time->gov_degov1.throttle*gov_degov1_R+curr_time->omega/omega_ref;
		}
		//Default else - it's already set, don't overwrite

		//Populate the "delayed torque" with the throttle value
		for (index_val=0; index_val<torque_delay_len; index_val++)
		{
			torque_delay[index_val] = curr_time->gov_degov1.throttle;
		}
	}//End DEGOV1 initialization
	else if (Governor_type == GAST)
	{
		curr_time->gov_gast.x1 = curr_time->torque_mech/(Rated_VA/omega_ref);
		curr_time->gov_gast.x2 = curr_time->torque_mech/(Rated_VA/omega_ref);
		curr_time->gov_gast.x3 = curr_time->torque_mech/(Rated_VA/omega_ref);
		curr_time->gov_gast.throttle = curr_time->torque_mech/(Rated_VA/omega_ref); //Init to Tmech
	}//End GAST initialization
	else if (Governor_type == P_CONSTANT)
	{
		// Include the actuator and time delay part from GGOV01
		if (gen_base_set_vals.wref < -90.0)	//Should be -99 if not set
		{
			gen_base_set_vals.wref = curr_time->omega/omega_ref;
		}
		//Default else -- already set, just ignore this

		curr_time->gov_pconstant.x5 = curr_time->pwr_mech / Rated_VA;

		//Translate this into the torque model
		curr_time->torque_mech = curr_time->pwr_mech / curr_time->omega;

		curr_time->gov_pconstant.x5b = curr_time->gov_pconstant.x5;
		curr_time->gov_pconstant.x5a = curr_time->gov_pconstant.x5b;
		curr_time->gov_pconstant.FuelFlow = curr_time->gov_pconstant.x5/gov_ggv1_Kturb + gov_ggv1_wfnl;

		pconstant_Flag = 0; // Set fule flow independent of speed in P_CONSTANT mode

		if (pconstant_Flag == 0)
		{
			curr_time->gov_pconstant.ValveStroke = curr_time->gov_pconstant.FuelFlow;
		}
		else if (pconstant_Flag == 1)
		{
			curr_time->gov_pconstant.ValveStroke = curr_time->gov_pconstant.FuelFlow/(curr_time->omega/omega_ref);
		}
		else
		{
			/****** COMMENTS/TROUBLESHOOT *****/
			gl_error("wrong pconstant_Flag");
			return FAILED;
		}

		curr_time->gov_pconstant.x4 = curr_time->gov_pconstant.ValveStroke;
		curr_time->gov_pconstant.GovOutPut = curr_time->gov_pconstant.ValveStroke;
		curr_time->gov_pconstant.x1 = curr_time->pwr_electric.Re() / Rated_VA;
		curr_time->gov_pconstant.x_Pconstant = curr_time->gov_pconstant.GovOutPut; // Initial state value is the same as pwr_mech value

	}//End P_CONSTANT initialization
	else if ((Governor_type == GGOV1) || (Governor_type == GGOV1_OLD))
	{
		if (gen_base_set_vals.wref < -90.0)	//Should be -99 if not set
		{
			gen_base_set_vals.wref = curr_time->omega/omega_ref;
		}
		//Default else -- already set, just ignore this

		if (gov_ggv1_Dm > 0.0)
		{
			curr_time->gov_ggov1.x5 = curr_time->pwr_mech/Rated_VA + gov_ggv1_Dm*(curr_time->omega/omega_ref - gen_base_set_vals.wref);
		}
		else
		{
			curr_time->gov_ggov1.x5 = curr_time->pwr_mech / Rated_VA;
		}
		//Translate this into the torque model
		curr_time->torque_mech = curr_time->pwr_mech / curr_time->omega;		

		curr_time->gov_ggov1.x5b = curr_time->gov_ggov1.x5;
		curr_time->gov_ggov1.x5a = curr_time->gov_ggov1.x5b;
		curr_time->gov_ggov1.FuelFlow = curr_time->gov_ggov1.x5/gov_ggv1_Kturb + gov_ggv1_wfnl;

		if (gov_ggv1_Flag == 0)
		{
			curr_time->gov_ggov1.ValveStroke = curr_time->gov_ggov1.FuelFlow;
		}
		else if (gov_ggv1_Flag == 1)
		{
			curr_time->gov_ggov1.ValveStroke = curr_time->gov_ggov1.FuelFlow/(curr_time->omega/omega_ref);
		}
		else
		{
			/****** COMMENTS/TROUBLESHOOT *****/
			gl_error("wrong ggv1_Flag");
			return FAILED;
		}

		curr_time->gov_ggov1.x4 = curr_time->gov_ggov1.ValveStroke;
		curr_time->gov_ggov1.GovOutPut = curr_time->gov_ggov1.ValveStroke;
		curr_time->gov_ggov1.x3 = curr_time->gov_ggov1.GovOutPut;
		curr_time->gov_ggov1.x7 = curr_time->gov_ggov1.GovOutPut;
		curr_time->gov_ggov1.fsrn = curr_time->gov_ggov1.GovOutPut;
		curr_time->gov_ggov1.fsra = curr_time->gov_ggov1.GovOutPut;
		curr_time->gov_ggov1.x1 = curr_time->pwr_electric.Re() / Rated_VA;

		if (gov_ggv1_rselect == 1)
		{
			curr_time->gov_ggov1.RselectValue = curr_time->gov_ggov1.x1;
		}
		else if (gov_ggv1_rselect == -1)
		{
			curr_time->gov_ggov1.RselectValue = curr_time->gov_ggov1.ValveStroke;
		}
		else if (gov_ggv1_rselect == -2)
		{
			curr_time->gov_ggov1.RselectValue = curr_time->gov_ggov1.GovOutPut;
		}
		else if (gov_ggv1_rselect == 0)
		{
			curr_time->gov_ggov1.RselectValue = 0.0;
		}
		else
		{
			gl_error("wrong ggv1_rselect parameter");
			return FAILED;
		}

		if (gov_ggv1_Pmwset < 0 || gov_ggv1_rselect != 1)
		// If negative, or it is not the constant P mode, will set Pmwset as default value
		{
			gov_ggv1_Pmwset = Rated_VA*curr_time->gov_ggov1.x1;
		}
		//Default else -- already initialized for Pmwset, and also in constant P mode

		curr_time->gov_ggov1.x8a = 0.0;
		curr_time->gov_ggov1.x8 = curr_time->gov_ggov1.x8a;
		curr_time->gov_ggov1.err2a = 0.0;
		curr_time->gov_ggov1.werror = curr_time->omega/omega_ref - gen_base_set_vals.wref;

		if (gen_base_set_vals.Pref < -90.0)	//Should be -99.0 if un-initialized
		{
			gen_base_set_vals.Pref = curr_time->gov_ggov1.err2a - curr_time->gov_ggov1.x8 + curr_time->gov_ggov1.werror + gov_ggv1_r*curr_time->gov_ggov1.RselectValue;
		}
		//Default else -- already initialized or set
		
		if (curr_time->gov_ggov1.err2a > gov_ggv1_maxerr)
		{
			curr_time->gov_ggov1.err2 = gov_ggv1_maxerr;
		}
		else if (curr_time->gov_ggov1.err2a < gov_ggv1_minerr)
		{
			curr_time->gov_ggov1.err2 = gov_ggv1_minerr;
		}
		else if ((curr_time->gov_ggov1.err2a <= gov_ggv1_db) && (curr_time->gov_ggov1.err2a >= -gov_ggv1_db))
		{
			curr_time->gov_ggov1.err2 = 0.0;
		}
		else
		{
			if (curr_time->gov_ggov1.err2a > 0)
			{
				curr_time->gov_ggov1.err2 = (gov_ggv1_maxerr/(gov_ggv1_maxerr - gov_ggv1_db))*curr_time->gov_ggov1.err2a - (gov_ggv1_maxerr*gov_ggv1_db/(gov_ggv1_maxerr-gov_ggv1_db));
			}
			else if (curr_time->gov_ggov1.err2a < 0)
			{
				curr_time->gov_ggov1.err2 = (gov_ggv1_minerr/(gov_ggv1_minerr + gov_ggv1_db))*curr_time->gov_ggov1.err2a + (gov_ggv1_minerr*gov_ggv1_db/(gov_ggv1_minerr+gov_ggv1_db));
			}
			//**** What happens when it is zero!?!?!? ****/
		}

		curr_time->gov_ggov1.x2a = curr_time->gov_ggov1.err2;
		curr_time->gov_ggov1.x2 = gov_ggv1_Kpgov/gov_ggv1_Tdgov*(curr_time->gov_ggov1.err2 - curr_time->gov_ggov1.x2a);
		curr_time->gov_ggov1.x7 = curr_time->gov_ggov1.GovOutPut;

		if (gov_ggv1_Dm < 0.0)
		{
			curr_time->gov_ggov1.x10a = curr_time->gov_ggov1.FuelFlow * pow((curr_time->omega/omega_ref),gov_ggv1_Dm);
		}
		else
		{
			curr_time->gov_ggov1.x10a = curr_time->gov_ggov1.FuelFlow;
		}
		curr_time->gov_ggov1.x10b = curr_time->gov_ggov1.x10a;
		curr_time->gov_ggov1.x10 = (1.0 - gov_ggv1_Tsa/gov_ggv1_Tsb)*curr_time->gov_ggov1.x10b + gov_ggv1_Tsa/gov_ggv1_Tsb*curr_time->gov_ggov1.x10a;
		curr_time->gov_ggov1.x6 = curr_time->gov_ggov1.x10;
		
		curr_time->gov_ggov1.x7 = curr_time->gov_ggov1.GovOutPut;
		curr_time->gov_ggov1.err7 = 0.0;
		if (gov_ggv1_Kpload > 0.0)
		{
			curr_time->gov_ggov1.x7a = gov_ggv1_Kiload / gov_ggv1_Kpload * curr_time->gov_ggov1.err7;
		}
		else
		{
			curr_time->gov_ggov1.x7a = gov_ggv1_Kiload * curr_time->gov_ggov1.err7;
		}

		curr_time->gov_ggov1.x9a = curr_time->omega/omega_ref - gen_base_set_vals.wref;
		curr_time->gov_ggov1.x9 = 1.0 / gov_ggv1_Ta * ((curr_time->omega/omega_ref - gen_base_set_vals.wref) - curr_time->gov_ggov1.x9a);

	}//End GGOV1 initialization
	//Default else - no initialization

	//AVR/Exciter initialization
	if (Exciter_type == SEXS)
	{
		curr_time->avr.xe = curr_time->Vfd;
		curr_time->avr.xb = curr_time->avr.xe/exc_KA;

		if (Vset_defined == false)	//Check flag Vset_defined instead, since may come into init_dynamics more than once
		{
			//Get average PU voltage
			gen_base_set_vals.vset =  (voltage_pu[0].Mag() + voltage_pu[1].Mag() + voltage_pu[2].Mag())/3.0;
			Vref = gen_base_set_vals.vset; // Record the initial vset value
		}
		//Default else -- it is set, don't adjust it
		gen_base_set_vals.vseta = gen_base_set_vals.vset;
		gen_base_set_vals.vsetb = gen_base_set_vals.vset;

		// Assign initial values to state variables ralated to CVR control if enabled
//		if (CVRenabled == true) {
////			curr_time->avr.x_cvr = 0;
//			curr_time->avr.xerr_cvr = 0;
//			gen_base_set_vals.vadd = 0;
//		}

		if (CVRenabled == true) {
			curr_time->avr.x_cvr1 = 0;
			curr_time->avr.x_cvr2 = 0;
			gen_base_set_vals.vadd = 0;
			gen_base_set_vals.vadd_a = 0;
		}

		// Define exciter bias value
		// For Q_constant mode, set bias as 0, so that Qout will match Qref
		if (Q_constant_mode == true) {
			curr_time->avr.bias = 0;
		}
		// TODO: non-zero bias value results in differences between vset and Vout measured. Need to think about it.
		else {
			curr_time->avr.bias = curr_time->avr.xb;
		}
	}//End SEXS initialization
	//Default else - no AVR/Exciter init

	return SUCCESS;	//Always succeeds for now, but could have error checks later
}

//Function to perform exp(j*val)
//Basically a complex rotation
complex diesel_dg::complex_exp(double angle)
{
	complex output_val;

	//exp(jx) = cos(x)+j*sin(x)
	output_val = complex(cos(angle),sin(angle));

	return output_val;
}

// Function to calculate absolute values of complex
double diesel_dg::abs_complex(complex val)
{
	double res;
	res = sqrt(val.Re() * val.Re() + val.Im() * val.Im());

	return res;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_diesel_dg(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(diesel_dg::oclass);
		if (*obj!=NULL)
		{
			diesel_dg *my = OBJECTDATA(*obj,diesel_dg);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	} 
	CREATE_CATCHALL(diesel_dg);
}

EXPORT int init_diesel_dg(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,diesel_dg)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(diesel_dg);
}

EXPORT TIMESTAMP sync_diesel_dg(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	TIMESTAMP t1 = TS_INVALID;
	diesel_dg *my = OBJECTDATA(obj,diesel_dg);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t1 = my->presync(obj->clock,t0);
			break;
		case PC_BOTTOMUP:
			t1 = my->sync(obj->clock,t0);
			break;
		case PC_POSTTOPDOWN:
			t1 = my->postsync(obj->clock,t0);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;		
	}
	SYNC_CATCHALL(diesel_dg);
	return t1;
}

//EXPORT for object-level call (as opposed to module-level)
/*
EXPORT STATUS update_diesel_dg(OBJECT *obj, unsigned int64 dt, unsigned int iteration_count_val)
{
	diesel_dg *my = OBJECTDATA(obj,diesel_dg);
	STATUS status = FAILED;
	try
	{
		status = my->deltaupdate(dt,iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("update_diesel_dg(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}
*/

EXPORT SIMULATIONMODE interupdate_diesel_dg(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	diesel_dg *my = OBJECTDATA(obj,diesel_dg);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time,dt,iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_diesel_dg(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

EXPORT STATUS postupdate_diesel_dg(OBJECT *obj, complex *useful_value, unsigned int mode_pass)
{
	diesel_dg *my = OBJECTDATA(obj,diesel_dg);
	STATUS status = FAILED;
	try
	{
		status = my->post_deltaupdate(useful_value, mode_pass);
		return status;
	}
	catch (char *msg)
	{
		gl_error("postupdate_diesel_dg(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}
