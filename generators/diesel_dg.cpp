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

			PT_enumeration,"Gen_mode",PADDR(Gen_mode),
				PT_KEYWORD,"UNKNOWN",(enumeration)UNKNOWN,
				PT_KEYWORD,"CONSTANTE",(enumeration)CONSTANTE,
				PT_KEYWORD,"CONSTANTPQ",(enumeration)CONSTANTPQ,
				PT_KEYWORD,"CONSTANTP",(enumeration)CONSTANTP,

			PT_enumeration,"Gen_status",PADDR(Gen_status),//Gen_status is not used in the code. I suggest removing it from the object.
				PT_KEYWORD,"UNKNOWN",(enumeration)UNKNOWN,
				PT_KEYWORD,"OFFLINE",(enumeration)OFFLINE,
				PT_KEYWORD,"ONLINE",(enumeration)ONLINE,	

			PT_enumeration,"Gen_type",PADDR(Gen_type),
				PT_KEYWORD,"INDUCTION",(enumeration)INDUCTION,
				PT_KEYWORD,"SYNCHRONOUS",(enumeration)SYNCHRONOUS,
				PT_KEYWORD,"DYN_SYNCHRONOUS",(enumeration)DYNAMIC,PT_DESCRIPTION,"Dynamics-capable implementation of synchronous diesel generator",
		
			PT_double, "pf", PADDR(pf),PT_DESCRIPTION,"desired power factor",

			PT_double, "GenElecEff", PADDR(GenElecEff),PT_DESCRIPTION,"calculated electrical efficiency of generator",
			PT_complex, "TotalOutputPow[VA]", PADDR(TotalPowerOutput), PT_DESCRIPTION, "total complex power generated",
			PT_double, "TotalRealPow[W]", PADDR(TotalPowerOutput.Re()),PT_DESCRIPTION,"total real power generated",
			PT_double, "TotalReacPow[VAr]", PADDR(TotalPowerOutput.Im()),PT_DESCRIPTION,"total reactive power generated",

			// Diesel engine power plant inputs
			PT_double, "speed[1/min]", PADDR(speed),PT_DESCRIPTION,"speed of an engine",
			PT_double, "cylinders", PADDR(cylinders),PT_DESCRIPTION,"Total number of cylinders in a diesel engine",
			PT_double, "stroke", PADDR(stroke),PT_DESCRIPTION,"category of internal combustion engines",
			PT_double, "torque[N]", PADDR(torque),PT_DESCRIPTION,"Net brake load",
			PT_double, "pressure[N/m^2]", PADDR(pressure),PT_DESCRIPTION,"",
			PT_double, "time_operation[min]", PADDR(time_operation),PT_DESCRIPTION,"",
			PT_double, "fuel[kg]", PADDR(fuel),PT_DESCRIPTION,"fuel consumption",
			PT_double, "w_coolingwater[kg]", PADDR(w_coolingwater),PT_DESCRIPTION,"weight of cooling water supplied per minute",
			PT_double, "inlet_temperature[degC]", PADDR(inlet_temperature),PT_DESCRIPTION,"Inlet temperature of cooling water in degC",
			PT_double, "outlet_temperature[degC]", PADDR(outlet_temperature),PT_DESCRIPTION,"outlet temperature of cooling water in degC",
			PT_double, "air_fuel[kg]", PADDR(air_fuel),PT_DESCRIPTION,"Air used per kg fuel",
			PT_double, "room_temperature[degC]", PADDR(room_temperature),PT_DESCRIPTION,"Room temperature in degC",
			PT_double, "exhaust_temperature[degC]", PADDR(exhaust_temperature),PT_DESCRIPTION,"exhaust gas temperature in degC",
			PT_double, "cylinder_length[m]", PADDR(cylinder_length),PT_DESCRIPTION,"length of the cylinder, used in efficiency calculations",
			PT_double, "cylinder_radius[m]", PADDR(cylinder_radius),PT_DESCRIPTION,"inner radius of cylinder, used in efficiency calculations",
			PT_double, "brake_diameter[m]", PADDR(brake_diameter),PT_DESCRIPTION,"diameter of brake, used in efficiency calculations",
			PT_double, "calotific_fuel[kJ/kg]", PADDR(calotific_fuel),PT_DESCRIPTION,"calorific value of fuel",
			PT_double, "steam_exhaust[kg]", PADDR(steam_exhaust),PT_DESCRIPTION,"steam formed per kg of fuel in the exhaust",
			PT_double, "specific_heat_steam[kJ/kg/K]", PADDR(specific_heat_steam),PT_DESCRIPTION,"specific heat of steam in exhaust",
			PT_double, "specific_heat_dry[kJ/kg/K]", PADDR(specific_heat_dry),PT_DESCRIPTION,"specific heat of dry exhaust gases",

			PT_double, "indicated_hp[W]", PADDR(indicated_hp),PT_DESCRIPTION,"Indicated horse power is the power developed inside the cylinder",
			PT_double, "brake_hp[W]", PADDR(brake_hp),PT_DESCRIPTION,"brake horse power is the output of the engine at the shaft measured by a dynamometer",
			PT_double, "thermal_efficiency", PADDR(thermal_efficiency),PT_DESCRIPTION,"thermal efficiency or mechanical efiiciency of the engine is efined as bp/ip",
			PT_double, "energy_supplied[kJ]", PADDR(energy_supplied),PT_DESCRIPTION,"energy supplied during the trail",
			PT_double, "heat_equivalent_ip[kJ]", PADDR(heat_equivalent_ip),PT_DESCRIPTION,"heat equivalent of IP in a given time of operation",
			PT_double, "energy_coolingwater[kJ]", PADDR(energy_coolingwater),PT_DESCRIPTION,"energy carried away by cooling water",
			PT_double, "mass_exhaustgas[kg]", PADDR(mass_exhaustgas),PT_DESCRIPTION,"mass of dry exhaust gas",
			PT_double, "energy_exhaustgas[kJ]", PADDR(energy_exhaustgas),PT_DESCRIPTION,"energy carried away by dry exhaust gases",
			PT_double, "energy_steam[kJ]", PADDR(energy_steam),PT_DESCRIPTION,"energy carried away by steam",
			PT_double, "total_energy_exhaustgas[kJ]", PADDR(total_energy_exhaustgas),PT_DESCRIPTION,"total energy carried away by dry exhaust gases is the sum of energy carried away bt steam and energy carried away by dry exhaust gases",
			PT_double, "unaccounted_energyloss[kJ]", PADDR(unaccounted_energyloss),PT_DESCRIPTION,"unaccounted for energy loss",

			//end of diesel engine inputs

			//Synchronous generator inputs
			PT_double, "Pconv[kW]", PADDR(Pconv),PT_DESCRIPTION,"Converted power = Mechanical input - (F & W loasses + Stray losses + Core losses)",

			//End of synchronous generator inputs
			PT_double, "Rated_V[V]", PADDR(Rated_V),PT_DESCRIPTION,"nominal line-line voltage in Volts",
			PT_double, "Rated_VA[VA]", PADDR(Rated_VA),PT_DESCRIPTION,"nominal capacity in VA",
			PT_complex, "power_out_A[VA]", PADDR(power_val[0]),PT_DESCRIPTION,"Output power of phase A",
			PT_complex, "power_out_B[VA]", PADDR(power_val[1]),PT_DESCRIPTION,"Output power of phase B",
			PT_complex, "power_out_C[VA]", PADDR(power_val[2]),PT_DESCRIPTION,"Output power of phase C",
			
			PT_double, "Rs", PADDR(Rs),PT_DESCRIPTION,"internal transient resistance in p.u.",
			PT_double, "Xs", PADDR(Xs),PT_DESCRIPTION,"internal transient impedance in p.u.",
			PT_double, "Rg", PADDR(Rg),PT_DESCRIPTION,"grounding resistance in p.u.",
			PT_double, "Xg", PADDR(Xg),PT_DESCRIPTION,"grounding impedance in p.u.",
			PT_complex, "voltage_A[V]", PADDR(voltage_A),PT_DESCRIPTION,"voltage at generator terminal, phase A",
			PT_complex, "voltage_B[V]", PADDR(voltage_B),PT_DESCRIPTION,"voltage at generator terminal, phase B",
			PT_complex, "voltage_C[V]", PADDR(voltage_C),PT_DESCRIPTION,"voltage at generator terminal, phase C",
			PT_complex, "current_A[A]", PADDR(current_A),PT_DESCRIPTION,"current generated at generator terminal, phase A",
			PT_complex, "current_B[A]", PADDR(current_B),PT_DESCRIPTION,"current generated at generator terminal, phase B",
			PT_complex, "current_C[A]", PADDR(current_C),PT_DESCRIPTION,"current generated at generator terminal, phase C",
			PT_complex, "EfA[V]", PADDR(EfA),PT_DESCRIPTION,"induced voltage on phase A",
			PT_complex, "EfB[V]", PADDR(EfB),PT_DESCRIPTION,"induced voltage on phase B",
			PT_complex, "EfC[V]", PADDR(EfC),PT_DESCRIPTION,"induced voltage on phase C",

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
			PT_double,"vset[pu]",PADDR(curr_state.avr.vset),PT_DESCRIPTION,"Exciter voltage set point state variable",
			PT_double,"bias",PADDR(curr_state.avr.bias),PT_DESCRIPTION,"Exciter bias state variable",
			PT_double,"xe",PADDR(curr_state.avr.xe),PT_DESCRIPTION,"Exciter state variable",
			PT_double,"xb",PADDR(curr_state.avr.xb),PT_DESCRIPTION,"Exciter state variable",

			//Properties for Governor of dynamics model
			PT_enumeration,"Governor_type",PADDR(Governor_type),PT_DESCRIPTION,"Governor model for dynamics-capable implementation",
				PT_KEYWORD,"NO_GOV",(enumeration)NO_GOV,PT_DESCRIPTION,"No exciter",
				PT_KEYWORD,"DEGOV1",(enumeration)DEGOV1,PT_DESCRIPTION,"DEGOV1 Woodward Diesel Governor",
				PT_KEYWORD,"GAST",(enumeration)GAST,PT_DESCRIPTION,"GAST Gas Turbine Governor", // gastflag

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
			PT_double,"DEGOV1_wref[pu]",PADDR(curr_state.gov_degov1.wref),PT_DESCRIPTION,"Governor reference frequency state variable",
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

			pf = 0.0;

			GenElecEff = 0.0;
			TotalPowerOutput = 0.0;
			
			// Diesel engine power plant inputs
			speed = 0.0;
			cylinders = 0.0;
			stroke = 0.0;
			torque = 0.0;
			pressure =0.0;
			time_operation = 0.0;
			fuel = 0.0;
			w_coolingwater = 0.0;
			inlet_temperature = 0.0;
			outlet_temperature = 0.0;
			air_fuel = 0.0;
			room_temperature = 0.0;
			exhaust_temperature = 0.0;
			cylinder_length = 0.0;
			cylinder_radius = 0.0;
			brake_diameter = 0.0;
			calotific_fuel = 0.0;
			steam_exhaust = 0.0;
			specific_heat_steam = 0.0;
			specific_heat_dry = 0.0;

			indicated_hp = 0.0;
			brake_hp = 0.0;
			thermal_efficiency = 0.0;
			energy_supplied = 0.0;

			heat_equivalent_ip = 0.0;
			energy_coolingwater = 0.0;
			mass_exhaustgas = 0.0;
			energy_exhaustgas = 0.0;

			energy_steam = 0.0;
			total_energy_exhaustgas = 0.0;
			unaccounted_energyloss = 0.0;

			//end of diesel engine inputs

			//Synchronous generator inputs
             
			Pconv = 0.0;

	//End of synchronous generator inputs
	Rated_V = 0.0;
	Rated_VA = 0.0;

	Rs = 0.025;				//Estimated values for synch representation.
	Xs = 0.200;
	Rg = 0.000;
	Xg = 0.000;
	voltage_A = 0.0;
	voltage_B = 0.0;
	voltage_C = 0.0;
	current_A = 0.0;
	current_B = 0.0;
	current_C = 0.0;
	EfA = 0.0;
	EfB = 0.0;
	EfC = 0.0;
	Max_P = 11000;
	Max_Q = 6633.25;

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

	bus_admittance_mat = NULL;
	full_bus_admittance_mat = NULL;
	PGenerated = NULL;
	IGenerated = NULL;
	FreqPower = NULL;
	TotalPower = NULL;
	Governor_type = NO_GOV;
	Exciter_type = NO_EXC;

	power_val[0] = 0.0;
	power_val[1] = 0.0;
	power_val[2] = 0.0;

	//Rotor convegence becomes 0.1 rad
	rotor_speed_convergence_criterion = 0.1;
	prev_rotor_speed_val = 0.0;

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
	torque_delay_len = 0;

	torque_delay_write_pos = 0;
	torque_delay_read_pos = 0;

	//Initialize desired frequency to 60 Hz
	curr_state.omega = 2*PI*60.0;

	deltamode_inclusive = false;	//By default, don't be included in deltamode simulations

	first_run = true;				//First time we run, we are the first run (by definition)

	prev_time = 0;
	prev_time_dbl = 0.0;

	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int diesel_dg::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);

	PROPERTY *pval;
	double ZB, SB, EB;
	double test_pf;
	bool *Norton_posting;
	complex tst, tst2, tst3, tst4;
	current_A = current_B = current_C = 0.0;

	// construct circuit variable map to meter -- copied from 'House' module
	struct {
		complex **var;
		char *varname;
	} map[] = {
		// local object name,	meter object name
		{&pCircuit_V,			"voltage_A"}, // assumes 2 and 3 follow immediately in memory
		{&pLine_I,				"current_A"}, // assumes 2 and 3 follow immediately in memory
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
	};

	static complex default_line123_voltage[3], default_line1_current[3];
	int i;

	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true;	//Set the flag and off we go
	}

	// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
	if (parent!=NULL)
	{
		if (gl_object_isa(parent,"meter","powerflow") || gl_object_isa(parent,"node","powerflow") || gl_object_isa(parent,"load","powerflow") ||  gl_object_isa(parent,"elec_frequency","powerflow") )
		{
			// attach meter variables to each circuit
			for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
				*(map[i].var) = get_complex(parent,map[i].varname);

			//If we were deltamode requesting, set the flag on the other side
			if (deltamode_inclusive==true)
			{
				//Map the flag
				pval = gl_get_property(parent,"Norton_dynamic");

				//Check it
				if ((pval==NULL) || (pval->ptype!=PT_bool))
				{
					GL_THROW("diesel_dg:%s failed to map Norton-equivalence deltamode variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
					/*  TROUBLESHOOT
					While attempting to set up the deltamode interfaces and calculations with powerflow, the required interface could not be mapped.
					Please check your GLM and try again.  If the error persists, please submit a trac ticket with your code.
					*/
				}

				//Map to the intermediate
				Norton_posting = (bool*)GETADDR(parent,pval);

				//Set the flag
				*Norton_posting = true;
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

		//Map phases
		set *phaseInfo;
		PROPERTY *tempProp;
		tempProp = gl_get_property(parent,"phases");

		if ((tempProp==NULL || tempProp->ptype!=PT_set))
		{
			GL_THROW("Unable to map phases property - ensure the parent is a meter or a node or a load");
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
		if((phases & 0x0007) != 0x0007){//parent does not have all three meters
			GL_THROW("The diesel_dg object must be connected to all three phases. Please make sure the parent object has all three phases.");
			/* TROUBLESHOOT
			The diesel_dg object is a three-phase generator. This message occured because the parent object does not have all three phases.
			Please check and make sure your parent object has all three phases and try again. if the error persists, please submit your code and a bug report via the Trac website.
			*/
		}
	}
	else
	{
		OBJECT *obj = OBJECTHDR(this);
		gl_warning("diesel_dg:%d %s", obj->id, parent==NULL?"has no parent meter defined":"parent is not a meter");

		// attach meter variables to each circuit in the default_meter
			*(map[0].var) = &default_line123_voltage[0];
			*(map[1].var) = &default_line1_current[0];

		// provide initial values for voltages
			default_line123_voltage[0] = complex(Rated_V/sqrt(3.0),0);
			default_line123_voltage[1] = complex(Rated_V/sqrt(3.0)*cos(2*PI/3),Rated_V/sqrt(3.0)*sin(2*PI/3));
			default_line123_voltage[2] = complex(Rated_V/sqrt(3.0)*cos(-2*PI/3),Rated_V/sqrt(3.0)*sin(-2*PI/3));

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

	//Old checks - only if not in dynamics mode (that's handled below)
	if (Gen_type!=DYNAMIC)
	{
		if (Gen_mode==UNKNOWN)
		{
			OBJECT *obj = OBJECTHDR(this);
			GL_THROW("Generator control mode is not specified");
			/*  TROUBLESHOOT
			The generator is in the mode of UNKNOWN.  Please change this mode and try again.
			*/
		}
		if (Gen_mode == CONSTANTP)
		{
			GL_THROW("Generator control mode, CONSTANTP is not implemented yet.");
			/* TROUBLESHOOT
			The generator is in the mode of CONSTANTP. Please change this to CONSTANTPQ and try again.
			*/
		}

		//Default checks
		if (Rated_VA==0.0)
		{
			Rated_VA = 12000.0;

			gl_warning("Generator power not specified, defaulting to 12 KVA");
			/*  TROUBLESHOOT
			A rated power for the diesel_dg object was not specified for the non-dynamic
			operation.  It has been assigned a default of 12 KVA.
			*/
		}

		SB = Rated_VA/3;

		if (Rated_V==0.0)
		{
			Rated_V = 480.0;

			gl_warning("Generator voltage not specified, defaulting to 480 V");
			/*  TROUBLESHOOT
			A rated voltage for the diesel_dg object was not specified for the non-dynamic
			operation.  It has been assigned a default of 480 V.
			*/
		}

		EB = Rated_V/sqrt(3.0);

		if (SB!=0.0)
			ZB = EB*EB/SB;
		else
		{
			GL_THROW("Generator power capacity not specified!");
			/*  TROUBLESHOOT
			The power capacity (Rated_VA) is not set.  Please set it and try again.
			*/
		}

		if (Gen_type == SYNCHRONOUS)  
		{
			double Real_Rs = Rs * ZB; 
			double Real_Xs = Xs * ZB;
			double Real_Rg = Rg * ZB; 
			double Real_Xg = Xg * ZB;
			tst = complex(Real_Rg,Real_Xg);
			tst2 = complex(Real_Rs,Real_Xs);
			AMx[0][0] = tst2 + tst;			//Impedance matrix
			AMx[1][1] = tst2 + tst;
			AMx[2][2] = tst2 + tst;
			AMx[0][1] = AMx[0][2] = AMx[1][0] = AMx[1][2] = AMx[2][0] = AMx[2][1] = tst;
			tst3 = (complex(1,0) + complex(2,0)*tst/tst2)/(tst2 + complex(3,0)*tst);
			tst4 = (-tst/tst2)/(tst2 + tst);
			invAMx[0][0] = tst3;			//Admittance matrix (inverse of Impedance matrix)
			invAMx[1][1] = tst3;
			invAMx[2][2] = tst3;
			invAMx[0][1] = AMx[0][2] = AMx[1][0] = AMx[1][2] = AMx[2][0] = AMx[2][1] = tst4;
		}
	}//End not dynamic init checks
	else	//Must be dynamic!
	{
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

		//Check voltage value
		if (Rated_V<=0.0)
		{
			Rated_V = 15000.0;	//Set to parameter-basis default

			gl_warning("diesel_dg:%s did not have a valid Rated_V set, assuming 15 kV",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The Rated_V property was not set or was invalid.  It is being forced to 15 kV, which aligns with the base for
			all default parameters.  This will be checked again later to see if it matches the connecting point.  If this
			value is not desired, please set it manually.
			*/
		}

		//Determine machine base values for later
		power_base = Rated_VA/3.0;
		voltage_base = Rated_V/sqrt(3.0);
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
			gen_object_count++;	//Increment the counter
		}
	}

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
	double Pmech;
	unsigned char jindex, kindex;
	OBJECT *obj = OBJECTHDR(this);
	double *ptemp_double;
	double temp_double_high, temp_double_low, tdiff, ang_diff;
	complex temp_current_val[3];
	complex temp_voltage_val[3];
	FUNCTIONADDR test_fxn;
	complex rotate_value;
	TIMESTAMP tret_value;
	double vdiff;
	double voltage_mag_curr;
	double reactive_diff;
	complex temp_power_val[3];

	//Assume always want TS_NEVER
	tret_value = TS_NEVER;

	//First run allocation - in diesel_dg for now, but may need to move elsewhere
	if (first_run == true)	//First run
	{
		//TODO: LOCKING!
		if (deltamode_inclusive && enable_subsecond_models && (torque_delay==NULL))	//We want deltamode - see if it's populated yet
		{
			if (((gen_object_current == -1) || (delta_objects==NULL)) && (enable_subsecond_models == true))
			{
				//Allocate the deltamode object array
				delta_objects = (OBJECT**)gl_malloc(gen_object_count*sizeof(OBJECT*));

				//Make sure it worked
				if (delta_objects == NULL)
				{
					GL_THROW("Failed to allocate deltamode objects array for generators module!");
					/*  TROUBLESHOOT
					While attempting to create a reference array for generator module deltamode-enabled
					objects, an error was encountered.  Please try again.  If the error persists, please
					submit your code and a bug report via the trac website.
					*/
				}

				//Allocate the function reference list as well
				delta_functions = (FUNCTIONADDR*)gl_malloc(gen_object_count*sizeof(FUNCTIONADDR));

				//Make sure it worked
				if (delta_functions == NULL)
				{
					GL_THROW("Failed to allocate deltamode objects function array for generators module!");
					/*  TROUBLESHOOT
					While attempting to create a reference array for generator module deltamode-enabled
					objects, an error was encountered.  Please try again.  If the error persists, please
					submit your code and a bug report via the trac website.
					*/
				}

				//Allocate the function reference list for postupdate as well
				post_delta_functions = (FUNCTIONADDR*)gl_malloc(gen_object_count*sizeof(FUNCTIONADDR));

				//Make sure it worked
				if (post_delta_functions == NULL)
				{
					GL_THROW("Failed to allocate deltamode objects function array for generators module!");
					//Defined above
				}

				//Initialize index
				gen_object_current = 0;
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
					//Check the nominal voltage
					ptemp_double = get_double(obj->parent,"nominal_voltage");

					if (ptemp_double == NULL)
					{
						GL_THROW("diesel_dg:%s - unable to retrieve nominal_voltage from parent!",obj->name?obj->name:"unnamed");
						/*  TROUBLESHOOT
						While attempting to retrieve the nominal_voltage value of the parented node, something went wrong.  Please
						try again.  If the error persists, please submit your code and a bug report via the trac website.
						*/
					}
					else	//Found it
					{
						//Form a "deadband" - not sure how much it will like "exact" comparisons of doubles
						temp_double_high = Rated_V/sqrt(3.0)*1.01;
						temp_double_low = Rated_V/sqrt(3.0)*0.99;

						//Compare it
						if ((*ptemp_double > temp_double_high) || (*ptemp_double < temp_double_low))
						{
							GL_THROW("diesel_dg:%s - nominal voltage mismatch!",obj->name?obj->name:"unnamed");
							/*  TROUBLESHOOT
							The Rated_V value for the diesel_dg object (line-line value) is not within 1% of the nominal
							wye-voltage of the parent meter.  Please fix this and try again.
							*/
						}
					}//End found point else

					//Map the bus mappings
					test_fxn = (FUNCTIONADDR)(gl_get_function(obj->parent,"delta_linkage_node"));

					//See if it was located
					if (test_fxn == NULL)
					{
						GL_THROW("diesel_dg:%s - failed to map bus admittance matrix from node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
						/*  TROUBLESHOOT
						While attempting to map the location of a bus interface point for the deltamode capabilities, an error was encountered.
						Please try again.  If the error persists, please submit your code and a bug report via the trac website.
						*/
					}

					//Map the value - bus admittance is 0
					bus_admittance_mat = ((complex * (*)(OBJECT *, unsigned char))(*test_fxn))(obj->parent,0);

					//See if it worked (should return NULL if the object wasn't "delta-compliant"
					if (bus_admittance_mat==NULL)
					{
						GL_THROW("diesel_dg:%s - invalid reference passed from node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
						/*  TROUBLESHOOT
						While attempting to map a deltamode interface variable, an error occurred.  This could be due to the matrix not being
						initialized correctly, or because the attached node is not flagged for deltamode.
						*/
					}

					//Copy the contents in
					for (jindex=0; jindex<3; jindex++)
					{
						for (kindex=0; kindex<3; kindex++)
						{
							bus_admittance_mat[3*jindex+kindex]+=generator_admittance[jindex][kindex];
						}
					}

					//Map the value - PGenerated is 1
					PGenerated = ((complex * (*)(OBJECT *, unsigned char))(*test_fxn))(obj->parent,1);

					//See if it worked (should return NULL if the object wasn't "delta-compliant"
					if (PGenerated==NULL)
					{
						GL_THROW("diesel_dg:%s - invalid reference passed from node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
						//Defined above
					}

					//Map current "injection" - direct generator current
					IGenerated = ((complex * (*)(OBJECT *, unsigned char))(*test_fxn))(obj->parent,2);

					//See if it worked (should return NULL if the object wasn't "delta-compliant"
					if (IGenerated==NULL)
					{
						GL_THROW("diesel_dg:%s - invalid reference passed from node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
						//Defined above
					}

					//Map the value - full bus admittance is 3
					full_bus_admittance_mat = ((complex * (*)(OBJECT *, unsigned char))(*test_fxn))(obj->parent,3);

					//See if it worked (should return NULL if the object wasn't "delta-compliant"
					if (full_bus_admittance_mat==NULL)
					{
						GL_THROW("diesel_dg:%s - invalid reference passed from node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
						//Defined above
					}

					//Map the Frequency-power weighting value
					FreqPower = ((complex * (*)(OBJECT *, unsigned char))(*test_fxn))(obj->parent,4);

					//See if it worked (should return NULL if the object wasn't "delta-compliant"
					if (FreqPower==NULL)
					{
						GL_THROW("diesel_dg:%s - invalid reference passed from node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
						//Defined above
					}

					//Map the total power weighting value
					TotalPower = ((complex * (*)(OBJECT *, unsigned char))(*test_fxn))(obj->parent,5);

					//See if it worked (should return NULL if the object wasn't "delta-compliant"
					if (TotalPower==NULL)
					{
						GL_THROW("diesel_dg:%s - invalid reference passed from node:%s",(obj->name?obj->name:"unnamed"),(obj->parent->name?obj->parent->name:"unnamed"));
						//Defined above
					}

					//Map up requisite variables we missed earlier?

					//Accumulate and pass our starting power
					*PGenerated = power_val[0] + power_val[1] + power_val[2];

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
	if (Gen_type != DYNAMIC)
	{
		indicated_hp = (100000/60)*(pressure*cylinder_length*(3.1416/4)*cylinder_radius*cylinder_radius*speed*cylinders);
		
		brake_hp = 2*3.1416*(speed/stroke)*(torque/cylinders)/60;

		thermal_efficiency = (brake_hp/indicated_hp)*100;

		energy_supplied = fuel * calotific_fuel;

		heat_equivalent_ip = indicated_hp * time_operation * 60/1000;

		energy_coolingwater = w_coolingwater * 4.187 * (outlet_temperature - inlet_temperature);

		mass_exhaustgas = (fuel * air_fuel) - (steam_exhaust * fuel);

		energy_exhaustgas = mass_exhaustgas * specific_heat_dry * (exhaust_temperature - room_temperature);

		energy_steam = (steam_exhaust * fuel) * (4.187 * (100 - room_temperature) + 2257.9 + specific_heat_steam * (exhaust_temperature - 100));

		total_energy_exhaustgas = energy_exhaustgas + energy_steam;

		unaccounted_energyloss = energy_supplied - heat_equivalent_ip - energy_coolingwater - total_energy_exhaustgas;	

		Pmech = brake_hp;

		Pconv = 1 * Pmech;  //TO DO:  friction and windage loss, misc. loss model

		//current_A = current_B = current_C = 0.0;

		int k;
		voltage_A = pCircuit_V[0];	//Syncs the meter parent to the generator.
		voltage_B = pCircuit_V[1];
		voltage_C = pCircuit_V[2];

		double Pconva = (voltage_A.Mag() / (voltage_A.Mag() + voltage_B.Mag() + voltage_C.Mag()))*Pconv;
		double Pconvb = (voltage_B.Mag() / (voltage_A.Mag() + voltage_B.Mag() + voltage_C.Mag()))*Pconv;
		double Pconvc = (voltage_C.Mag() / (voltage_A.Mag() + voltage_B.Mag() + voltage_C.Mag()))*Pconv;

		if (Gen_type == SYNCHRONOUS)	
		{											//sg ef mode is not working yet
			double Mxef, Mnef, PoutA, PoutB, PoutC, QoutA, QoutB, QoutC;
			complex SoutA, SoutB, SoutC;
			complex lossesA, lossesB, lossesC;

			Mxef = Max_Ef * Rated_V/sqrt(3.0);
			Mnef = Min_Ef * Rated_V/sqrt(3.0);

			
			if (Gen_mode == CONSTANTE)	//Ef is controllable to give a needed power output.
			{
				current_A = invAMx[0][0]*(voltage_A - EfA) + invAMx[0][1]*(voltage_B - EfB) + invAMx[0][2]*(voltage_C - EfC);
				current_B = invAMx[1][0]*(voltage_A - EfA) + invAMx[1][1]*(voltage_B - EfB) + invAMx[1][2]*(voltage_C - EfC);
				current_C = invAMx[2][0]*(voltage_A - EfA) + invAMx[2][1]*(voltage_B - EfB) + invAMx[2][2]*(voltage_C - EfC);

				SoutA = -voltage_A * (~(current_A));  //TO DO:  unbalanced
				SoutB = -voltage_B * (~(current_B));
				SoutC = -voltage_C * (~(current_C));
			}

			else if (Gen_mode == CONSTANTPQ)	//Gives a constant output power of real power converted Pout,  
			{									//then Qout is found through a controllable power factor.
				if (Pconva > 1*Max_P/3) {
					Pconva = 1*Max_P/3;		//If air density increases, power extracted can be much greater
				}								//than amount the generator can handle.  This limits to 5% overpower.
				if (Pconvb > 1*Max_P/3) {
					Pconvb = 1*Max_P/3;
				}
				if (Pconvc > 1*Max_P/3) {
					Pconvc = 1*Max_P/3;
				}
				
				current_A = -(~(complex(Pconva,(Pconva/pf)*sin(acos(pf)))/voltage_A));
				current_B = -(~(complex(Pconvb,(Pconvb/pf)*sin(acos(pf)))/voltage_B));
				current_C = -(~(complex(Pconvc,(Pconvc/pf)*sin(acos(pf)))/voltage_C));

				for (k = 0; k < 100; k++)
				{
					PoutA = Pconva - current_A.Mag()*current_A.Mag()*(AMx[0][0] - AMx[0][1]).Re();
					PoutB = Pconvb - current_B.Mag()*current_B.Mag()*(AMx[1][1] - AMx[0][1]).Re();
					PoutC = Pconvc - current_C.Mag()*current_C.Mag()*(AMx[2][2] - AMx[0][1]).Re();

					QoutA = pf/fabs(pf)*PoutA*sin(acos(pf));
					QoutB = pf/fabs(pf)*PoutB*sin(acos(pf));
					QoutC = pf/fabs(pf)*PoutC*sin(acos(pf));

					current_A = -(~(complex(PoutA,QoutA)/voltage_A));
					current_B = -(~(complex(PoutB,QoutB)/voltage_B));
					current_C = -(~(complex(PoutC,QoutC)/voltage_C));
				}

				EfA = voltage_A - (AMx[0][0] - AMx[0][1])*current_A - AMx[0][2]*(current_A + current_B + current_C);
				EfB = voltage_B - (AMx[1][1] - AMx[1][0])*current_A - AMx[1][2]*(current_A + current_B + current_C);
				EfC = voltage_C - (AMx[2][2] - AMx[2][0])*current_A - AMx[2][1]*(current_A + current_B + current_C);

				//if (EfA.Mag() > Mxef || EfA.Mag() > Mxef || EfA.Mag() > Mxef)
				//{
				//	Gen_mode = CONSTANTEf;
				//}TO DO:  loop back to Ef if true?
			}
		}//Synchronous generator ends here
		
		//test functions

		double PowerA, PowerB, PowerC, QA, QB, QC;

		PowerA = -voltage_A.Mag()*current_A.Mag()*cos(voltage_A.Arg() - current_A.Arg());
		PowerB = -voltage_B.Mag()*current_B.Mag()*cos(voltage_B.Arg() - current_B.Arg());
		PowerC = -voltage_C.Mag()*current_C.Mag()*cos(voltage_C.Arg() - current_C.Arg());

		QA = -voltage_A.Mag()*current_A.Mag()*sin(voltage_A.Arg() - current_A.Arg());
		QB = -voltage_B.Mag()*current_B.Mag()*sin(voltage_B.Arg() - current_B.Arg());
		QC = -voltage_C.Mag()*current_C.Mag()*sin(voltage_C.Arg() - current_C.Arg());

		TotalPowerOutput = complex((PowerA + PowerB + PowerC),(QA + QB + QC));

		GenElecEff = TotalPowerOutput.Re()/Pconv * 100;

		pLine_I[0] = current_A;
		pLine_I[1] = current_B;
		pLine_I[2] = current_C;
	}//End no dynamic generator (older code)
	else	//Must be a synchronous dynamic machine
	{
		//Only do updates if this is a new timestep
		if ((prev_time < t1) && (first_run == false))
		{
			//Get time difference
			tdiff = (double)(t1)-prev_time_dbl;

			//Calculate rotor angle update
			ang_diff = (curr_state.omega - omega_ref)*tdiff;
			curr_state.rotor_angle += ang_diff;

			//Figure out the rotation to the existing values
			rotate_value = complex_exp(ang_diff);

			//Apply to voltage - See if this breaks stuff
			pCircuit_V[0] = pCircuit_V[0]*rotate_value;
			pCircuit_V[1] = pCircuit_V[1]*rotate_value;
			pCircuit_V[2] = pCircuit_V[2]*rotate_value;

			//Rotate the current injection too, otherwise it may "undo" this
			IGenerated[0] = IGenerated[0]*rotate_value;
			IGenerated[1] = IGenerated[1]*rotate_value;
			IGenerated[2] = IGenerated[2]*rotate_value;

			//Update time
			prev_time = t1;
			prev_time_dbl = (double)(t1);

			//Compute our current voltage point - see if we need to adjust things (if we have an AVR)
			if (Exciter_type == SEXS)
			{
				//Compute our current voltage point (pos_sequence)
				convert_abc_to_pn0(&pCircuit_V[0],&temp_voltage_val[0]);

				//Get the positive sequence magnitude
				voltage_mag_curr = temp_voltage_val[0].Mag()/voltage_base;

				if ((voltage_mag_curr>Max_Ef) || (voltage_mag_curr<Min_Ef))
				{
					//See where the value is
					vdiff = temp_voltage_val[0].Mag()/voltage_base - curr_state.avr.vset;

					//Figure out Q difference
					reactive_diff = (YS1_Full.Im()*(vdiff*voltage_base)*voltage_base)/3.0;

					//Copy in value
					temp_power_val[0] = power_val[0] + complex(0.0,reactive_diff);
					temp_power_val[1] = power_val[1] + complex(0.0,reactive_diff);
					temp_power_val[2] = power_val[2] + complex(0.0,reactive_diff);

					//Back out the current injection
					temp_current_val[0] = ~(temp_power_val[0]/pCircuit_V[0]) + generator_admittance[0][0]*pCircuit_V[0] + generator_admittance[0][1]*pCircuit_V[1] + generator_admittance[0][2]*pCircuit_V[2];
					temp_current_val[1] = ~(temp_power_val[1]/pCircuit_V[1]) + generator_admittance[1][0]*pCircuit_V[0] + generator_admittance[1][1]*pCircuit_V[1] + generator_admittance[1][2]*pCircuit_V[2];
					temp_current_val[2] = ~(temp_power_val[2]/pCircuit_V[2]) + generator_admittance[2][0]*pCircuit_V[0] + generator_admittance[2][1]*pCircuit_V[1] + generator_admittance[2][2]*pCircuit_V[2];

					//Apply and see what happens
					IGenerated[0] = temp_current_val[0];
					IGenerated[1] = temp_current_val[1];
					IGenerated[2] = temp_current_val[2];

					//Keep us here
					tret_value = t1;
				}
				//Default else - do nothing
			}
			//Default else - no AVR
		}
		//Nothing else in here right now....all handled internal to powerflow
	}//End synchronous dynamics-enabled generator

	return tret_value;
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP diesel_dg::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	complex temp_current_val[3];
	int ret_state;
	OBJECT *obj = OBJECTHDR(this);
	complex aval, avalsq;

	TIMESTAMP t2 = TS_NEVER;

	if (Gen_type == DYNAMIC)
	{
		//Update global, if necessary - assume everyone grabbed by sync
		if (deltamode_endtime != TS_NEVER)
		{
			deltamode_endtime = TS_NEVER;
			deltamode_endtime_dbl = TSNVRDBL;
		}

		//Update output power
		//Get current injected
		temp_current_val[0] = (IGenerated[0] - generator_admittance[0][0]*pCircuit_V[0] - generator_admittance[0][1]*pCircuit_V[1] - generator_admittance[0][2]*pCircuit_V[2]);
		temp_current_val[1] = (IGenerated[1] - generator_admittance[1][0]*pCircuit_V[0] - generator_admittance[1][1]*pCircuit_V[1] - generator_admittance[1][2]*pCircuit_V[2]);
		temp_current_val[2] = (IGenerated[2] - generator_admittance[2][0]*pCircuit_V[0] - generator_admittance[2][1]*pCircuit_V[1] - generator_admittance[2][2]*pCircuit_V[2]);

		//Update power output variables, just so we can see what is going on
		power_val[0] = pCircuit_V[0]*~temp_current_val[0];
		power_val[1] = pCircuit_V[1]*~temp_current_val[1];
		power_val[2] = pCircuit_V[2]*~temp_current_val[2];

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

			//Perform the conversion
			YS1_Full = full_bus_admittance_mat[0]+aval*full_bus_admittance_mat[3]+avalsq*full_bus_admittance_mat[6];
			YS1_Full += avalsq*full_bus_admittance_mat[1]+full_bus_admittance_mat[4]+aval*full_bus_admittance_mat[7];
			YS1_Full += aval*full_bus_admittance_mat[2]+avalsq*full_bus_admittance_mat[5]+full_bus_admittance_mat[8];
			YS1_Full /= 3.0;

		}//End "first run" paired
		//Default else - not dynamics-oriented, deflag
		
		//Deflag us
		first_run = false;
	}

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//Retrieves the pointer for a complex variable from another object
complex *diesel_dg::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}

//Retrieves the pointer for a double variable from another object
double *diesel_dg::get_double(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_double)
		return NULL;
	return (double*)GETADDR(obj,p);
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
	double temp_double;
	double deltat, deltath;
	double omega_pu;
	complex temp_rotation;
	complex temp_complex[3];
	complex temp_current_val[3];

	//Create delta_t variable
	deltat = (double)dt/(double)DT_SECOND;
	deltath = deltat/2.0;

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
			torque_delay_read_pos;	//Read at beginning
		}//End DEGOV1 type

		//Initialize dynamics
		init_dynamics(&curr_state);

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
	}//End first pass of new timestep

	//See what we're on, for tracking
	pass_mod = iteration_count_val - ((iteration_count_val >> 1) << 1);

	//Check pass
	if (pass_mod==0)	//Predictor pass
	{
		//Compute the "present" electric power value before anything gets updated for the new timestep
		temp_current_val[0] = (IGenerated[0] - generator_admittance[0][0]*pCircuit_V[0] - generator_admittance[0][1]*pCircuit_V[1] - generator_admittance[0][2]*pCircuit_V[2]);
		temp_current_val[1] = (IGenerated[1] - generator_admittance[1][0]*pCircuit_V[0] - generator_admittance[1][1]*pCircuit_V[1] - generator_admittance[1][2]*pCircuit_V[2]);
		temp_current_val[2] = (IGenerated[2] - generator_admittance[2][0]*pCircuit_V[0] - generator_admittance[2][1]*pCircuit_V[1] - generator_admittance[2][2]*pCircuit_V[2]);

		//Update power output variables, just so we can see what is going on
		power_val[0] = pCircuit_V[0]*~temp_current_val[0];
		power_val[1] = pCircuit_V[1]*~temp_current_val[1];
		power_val[2] = pCircuit_V[2]*~temp_current_val[2];

		//Update the output power variable
		curr_state.pwr_electric = power_val[0] + power_val[1] + power_val[2];

		//Copy it into the "next" value as well, so it doesn't get overwritten funny when the transition occurs
		next_state.pwr_electric = curr_state.pwr_electric;

		//Call dynamics
		apply_dynamics(&curr_state,&predictor_vals);

		//Apply prediction update
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
		if (Governor_type == GAST)
		{
			next_state.gov_gast.x1 = curr_state.gov_gast.x1 + predictor_vals.gov_gast.x1*deltat;
			next_state.gov_gast.x2 = curr_state.gov_gast.x2 + predictor_vals.gov_gast.x2*deltat;
			next_state.gov_gast.x3 = curr_state.gov_gast.x3 + predictor_vals.gov_gast.x3*deltat;
		}//End GAST update
		//Default else - no updates because no governor

		//Exciter updates
		if (Exciter_type == SEXS)
		{
			next_state.avr.xe = curr_state.avr.xe + predictor_vals.avr.xe*deltat;
			next_state.avr.xb = curr_state.avr.xb + predictor_vals.avr.xb*deltat;
		}//End SEXS update
		//Default else - no updates because no exciter

		//Nab per-unit omega, while we're at it
	    omega_pu = curr_state.omega/omega_ref;

		//Update generator current injection (technically is done "before" next powerflow)
		IGenerated[0] = next_state.EintVal[0]*YS1*omega_pu;
		IGenerated[1] = next_state.EintVal[1]*YS1*omega_pu;
		IGenerated[2] = next_state.EintVal[2]*YS1*omega_pu;

		return SM_DELTA_ITER;	//Reiterate - to get us to corrector pass
	}
	else	//Corrector pass
	{
		//Call dynamics
		apply_dynamics(&next_state,&corrector_vals);

		//Reconcile updates update
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
		if (Governor_type == GAST)
		{
			next_state.gov_gast.x1 = curr_state.gov_gast.x1 + (predictor_vals.gov_gast.x1 + corrector_vals.gov_gast.x1)*deltath;
			next_state.gov_gast.x2 = curr_state.gov_gast.x2 + (predictor_vals.gov_gast.x2 + corrector_vals.gov_gast.x2)*deltath;
			next_state.gov_gast.x3 = curr_state.gov_gast.x3 + (predictor_vals.gov_gast.x3 + corrector_vals.gov_gast.x3)*deltath;
		}//End GAST update
		//Default else - no updates because no governor

		//Exciter updates
		if (Exciter_type == SEXS)
		{
			next_state.avr.xe = curr_state.avr.xe + (predictor_vals.avr.xe + corrector_vals.avr.xe)*deltath;
			next_state.avr.xb = curr_state.avr.xb + (predictor_vals.avr.xb + corrector_vals.avr.xb)*deltath;
		}//End SEXS update
		//Default else - no updates because no exciter

		//Nab per-unit omega, while we're at it
	    omega_pu = curr_state.omega/omega_ref;

		//Update generator current injection (technically is done "before" next powerflow)
		IGenerated[0] = next_state.EintVal[0]*YS1*omega_pu;
		IGenerated[1] = next_state.EintVal[1]*YS1*omega_pu;
		IGenerated[2] = next_state.EintVal[2]*YS1*omega_pu;

		//Copy everything back into curr_state, since we'll be back there
		memcpy(&curr_state,&next_state,sizeof(MAC_STATES));

		//Check convergence
		temp_double = fabs(curr_state.omega - prev_rotor_speed_val);

		//Update tracking variable
		prev_rotor_speed_val = curr_state.omega;

		//Determine our desired state - if rotor speed is settled, exit
		if (temp_double<=rotor_speed_convergence_criterion)
		{
			//Ready to leave Delta mode
			return SM_EVENT;
		}
		else	//Not "converged" -- I would like to do another update
		{
			return SM_DELTA;	//Next delta update
								//Could theoretically request a reiteration, but we're not allowing that right now
		}
	}//End corrector pass
}

//Module-level post update call
//useful_value is a pointer to a passed in complex valu
//mode_pass 0 is the accumulation call
//mode_pass 1 is the "update our frequency" call
STATUS diesel_dg::post_deltaupdate(complex *useful_value, unsigned int mode_pass)
{
	if (mode_pass == 0)	//Accumulation pass
	{
		//Add in our frequency-weighted power
		*FreqPower += complex(curr_state.pwr_electric.Re()*curr_state.omega,0.0);

		//Add in our "current" power for the weighting
		*TotalPower += curr_state.pwr_electric;

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
	else if (mode_pass == 1)	//Push the frequency
	{
		curr_state.omega = useful_value->Re();
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
STATUS diesel_dg::apply_dynamics(MAC_STATES *curr_time, MAC_STATES *curr_delta)
{
	complex current_pu[3];
	complex Ipn0[3];
	complex temp_complex;
	double omega_pu;
	double temp_double_1, temp_double_2, temp_double_3, delomega, x0; 
	double torquenow;

	//Convert current as well
	current_pu[0] = (IGenerated[0] - generator_admittance[0][0]*pCircuit_V[0] - generator_admittance[0][1]*pCircuit_V[1] - generator_admittance[0][2]*pCircuit_V[2])/current_base;
	current_pu[1] = (IGenerated[1] - generator_admittance[1][0]*pCircuit_V[0] - generator_admittance[1][1]*pCircuit_V[1] - generator_admittance[1][2]*pCircuit_V[2])/current_base;
	current_pu[2] = (IGenerated[2] - generator_admittance[2][0]*pCircuit_V[0] - generator_admittance[2][1]*pCircuit_V[1] - generator_admittance[2][2]*pCircuit_V[2])/current_base;

	// post currents
	current_A=current_pu[0]*current_base;
	current_B=current_pu[1]*current_base;
	current_C=current_pu[2]*current_base;


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

		//Compute the offset currently
		temp_double_1 = curr_time->gov_degov1.wref - curr_time->omega/omega_ref-gov_degov1_R*curr_time->gov_degov1.throttle;
		
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
	else	//No governor - zero stuff for paranoia reasons
	{
		curr_delta->gov_degov1.throttle = 0.0;
		curr_delta->gov_degov1.x1 = 0.0;
		curr_delta->gov_degov1.x2 = 0.0;
		curr_delta->gov_degov1.x4 = 0.0;
		curr_delta->gov_degov1.x5 = 0.0;
		curr_delta->gov_degov1.x6 = 0.0;
	}//End no DEGOV1
	//Governor updates, if relevant
	if (Governor_type == GAST)	//Gast Turbine Governor
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
	else	//No governor - zero stuff for paranoia reasons
	{
		curr_delta->gov_gast.throttle = 0.0;
		curr_delta->gov_gast.x1 = 0.0;
		curr_delta->gov_gast.x2 = 0.0;
		curr_delta->gov_gast.x3 = 0.0;
	}//End no governor


	//AVR updates, if relevant
	if (Exciter_type == SEXS)
	{
		//Get the average magnitude first
		temp_double_1 = (pCircuit_V[0].Mag() + pCircuit_V[1].Mag() + pCircuit_V[2].Mag())/voltage_base/3.0;

		//Calculate the difference from the desired set point
		temp_double_2 = curr_time->avr.vset - temp_double_1;

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
		if (curr_time->avr.xe>=exc_EMAX)
			curr_time->avr.xe = exc_EMAX;

		if (curr_time->avr.xe<=exc_EMIN)
			curr_time->avr.xe = exc_EMIN;

		//Apply update
		curr_time->Vfd = curr_time->avr.xe;
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

	//Convert voltage to per-unit
	voltage_pu[0] = pCircuit_V[0]/voltage_base;
	voltage_pu[1] = pCircuit_V[1]/voltage_base;
	voltage_pu[2] = pCircuit_V[2]/voltage_base;

	//Convert current as well
	current_pu[0] = (IGenerated[0] - generator_admittance[0][0]*pCircuit_V[0] - generator_admittance[0][1]*pCircuit_V[1] - generator_admittance[0][2]*pCircuit_V[2])/current_base;
	current_pu[1] = (IGenerated[1] - generator_admittance[1][0]*pCircuit_V[0] - generator_admittance[1][1]*pCircuit_V[1] - generator_admittance[1][2]*pCircuit_V[2])/current_base;
	current_pu[2] = (IGenerated[2] - generator_admittance[2][0]*pCircuit_V[0] - generator_admittance[2][1]*pCircuit_V[1] - generator_admittance[2][2]*pCircuit_V[2])/current_base;

	// post currents
	current_A=current_pu[0]*current_base;
	current_B=current_pu[1]*current_base;
	current_C=current_pu[2]*current_base;

	
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

	//Governor initial conditions
	if (Governor_type == DEGOV1)
	{
		curr_time->gov_degov1.x1 = 0;
		curr_time->gov_degov1.x2 = 0;
		curr_time->gov_degov1.x5 = 0;
		curr_time->gov_degov1.x6 = 0;
		curr_time->gov_degov1.x4 = curr_time->torque_mech/(Rated_VA/omega_ref);
		curr_time->gov_degov1.throttle = curr_time->gov_degov1.x4;	//Init to Tmech
		curr_time->gov_degov1.wref = curr_time->gov_degov1.throttle*gov_degov1_R+curr_time->omega/omega_ref;

		//Populate the "delayed torque" with the throttle value
		for (index_val=0; index_val<torque_delay_len; index_val++)
		{
			torque_delay[index_val] = curr_time->gov_degov1.throttle;
		}
	}//End DEGOV1 initialization
	if (Governor_type == GAST)
	{
		curr_time->gov_gast.x1 = curr_time->torque_mech/(Rated_VA/omega_ref);
		curr_time->gov_gast.x2 = curr_time->torque_mech/(Rated_VA/omega_ref);
		curr_time->gov_gast.x3 = curr_time->torque_mech/(Rated_VA/omega_ref);
		curr_time->gov_gast.throttle = curr_time->torque_mech/(Rated_VA/omega_ref); //Init to Tmech
	}//End GAST initialization
	//Default else - no initialization


	//AVR/Exciter initialization
	if (Exciter_type == SEXS)
	{
		curr_time->avr.xe = curr_time->Vfd;
		curr_time->avr.xb = curr_time->avr.xe/exc_KA;

		//Get average PU voltage
		temp_double_1 = (voltage_pu[0].Mag() + voltage_pu[1].Mag() + voltage_pu[2].Mag())/3.0;

		curr_time->avr.vset = temp_double_1;
		curr_time->avr.bias = curr_time->avr.xb;
	}//End SEXS initialization
	//Default else - no AVR/Exciter init

	//Zero out our "parent" accumulators for final frequency
	//May get zeroed out multiple times - better safe than sorry
	*FreqPower = complex(0.0,0.0);
	*TotalPower = complex(0.0,0.0);

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
