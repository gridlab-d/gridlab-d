/** $Id: diesel_dg.h,v 1.2 2008/02/12 00:28:08 d3g637 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file diesel_dg.h
	@addtogroup diesel_dg

 @{  
 **/

#ifndef _diesel_dg_H
#define _diesel_dg_H

#include <stdarg.h>
#include "gridlabd.h"
#include "generators.h"
#include "power_electronics.h"

EXPORT SIMULATIONMODE interupdate_diesel_dg(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS postupdate_diesel_dg(OBJECT *obj, complex *useful_value, unsigned int mode_pass);

//AVR state variable structure
typedef struct {
	double vset;			//p.u. voltage set point
	double bias;			//Bias term of avr
	double xe;				//State variable
	double xb;				//State variable for transient gain reduction
} AVR_VARS;

//GOV_DEGOV1 state variable structure
typedef struct {
	double wref;			//Current reference frequency (p.u.)
	double x1;				//Electric box state variable
	double x2;				//Electric box state variable
	double x4;				//Actuator state variable
	double x5;				//Actuator state variable
	double x6;				//Actuator state variable
	double throttle;		//governor actuator output
} GOV_DEGOV1_VARS;

// gastflag
//GOV_GAST state variable structure
typedef struct {
	double wref;			//Current reference frequency (p.u.)
	double x1;				//Electric box state variable 
	double x2;				//Electric box state variable
	double x3;				//Temp limiter state variable
	double throttle;		//governor actuator output
} GOV_GAST_VARS;

//Machine state variable structure
typedef struct {
	double rotor_angle;			//Rotor angle of machine
	double omega;				//Current speed of machine
	double Vfd;					//Field voltage of machine
	double Flux1d;				//Transient flux on d-axis
	double Flux2q;				//Sub-transient flux on q-axis
	complex EpRotated;			//d-q rotated E' internal voltage
	complex VintRotated;		//d-q rotated Vint voltage
	complex EintVal[3];			//Unrotated, un-sequenced internal voltage
	complex Irotated;			//d-q rotated current value
	complex pwr_electric;		//Total electric power output of generator
	double pwr_mech;			//Mechanical power output of generator
	double torque_mech;			//Mechanical torque of generator
	double torque_elec;			//electrical torque of generator
	GOV_DEGOV1_VARS gov_degov1;	//DEGOV1 Governor state variables
	GOV_GAST_VARS gov_gast;		//GAST Governor state variables // gastflag
	AVR_VARS avr;				//Automatic Voltage Regulator state variables
} MAC_STATES;

class diesel_dg : public gld_object
{
private:
	/* TODO: put private variables here */
	complex *pCircuit_V; ///< pointer to the three voltages on three lines
	complex *pLine_I; ///< pointer to the three current on three lines

	bool first_run;		///< Flag for first run of the diesel_dg object - eliminates t0==0 dependence

	//Internal synchronous machine variables
	complex *bus_admittance_mat;		//Link to bus raw self-admittance value - grants 3x3 access instead of diagonal
	complex *full_bus_admittance_mat;	//Link to bus full self-admittance of Ybus form
	complex *PGenerated;				//Link to bus PGenerated field - mainly used for SWING generator
	complex *IGenerated;				//Link to direct current injections to powerflow at bus-level
	complex *FreqPower;					//Link to bus "frequency-power weighted" accumulation
	complex *TotalPower;				//Link to bus "accumulated power" - used for nominal frequency calculation
	complex generator_admittance[3][3];	//Generator admittance matrix converted from sequence values
	double power_base;					//Per-phase basis (divide by 3)
	double voltage_base;				//Voltage p.u. base for analysis (converted from delta)
	double current_base;				//Current p.u. base for analysis
	double impedance_base;				//Impedance p.u. base for analysis
	complex YS0;						//Zero sequence admittance - scaled (not p.u.)
	complex YS1;						//Positive sequence admittance - scaled (not p.u.)
	complex YS2;						//Negative sequence admittance - scaled (not p.u.)
	complex YS1_Full;					//Positive sequence admittance - full Ybus value
	double Rr;							//Rotor resistance term - derived
	double *torque_delay;				//Buffer of delayed governor torques
	unsigned int torque_delay_len;		//Length of the torque delay buffer
	unsigned int torque_delay_write_pos;//Indexing variable for writing the torque_delay_buffer
	unsigned int torque_delay_read_pos;	//Indexing variable for reading torque_delay_buffer
	double prev_rotor_speed_val;		//Previous value of rotor speed - used for delta-exiting convergence check
	complex last_power_output[3];		//Tracking variable for previous power output - used to do super-second frequency adjustments
	TIMESTAMP prev_time;				//Tracking variable for previous "new time" run
	double prev_time_dbl;				//Tracking variable for previous "new time" run -- deltamode capable

	MAC_STATES curr_state;		//Current state of all vari
	MAC_STATES next_state;
	MAC_STATES predictor_vals;	//Predictor pass values of variables
	MAC_STATES corrector_vals;	//Corrector pass values of variables

	bool deltamode_inclusive;	//Boolean for deltamode calls - pulled from object flags

protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	enum {OFFLINE=1, ONLINE};
	enumeration Gen_status;
	enum {INDUCTION=1, SYNCHRONOUS, DYNAMIC};
	enumeration Gen_type;
	enum {CONSTANTE=1, CONSTANTP, CONSTANTPQ};
	enumeration Gen_mode;

	//Dynamics synchronous generator capabilities
	enum {NO_EXC=1, SEXS};
	enumeration Exciter_type;
	//gastflag
	enum {NO_GOV=1, DEGOV1, GAST};
	enumeration Governor_type;

	//Diesel engine inputs
	complex AMx[3][3];			//Impedance matrix for Synchronous Generator
	complex invAMx[3][3];		//Inverse of SG impedance matrix

	double speed; // speed of an engine
	double cylinders;//Total number of cylinders in a diesel engine
	double stroke; //category of internal combustion engines
	double torque;//Net brake load
	double pressure;//Mean effective pressure
	double time_operation; 
	double fuel;//fuel consumption
	double w_coolingwater;//weight of cooling water supplied per minute
	double inlet_temperature; //Inlet temperature of cooling water in degC
	double outlet_temperature;//outlet temperature of cooling water in degC
	double air_fuel; //Air used per kg fuel
	double room_temperature; //Room temperature in degC
	double exhaust_temperature;//exhaust gas temperature in degC
	double cylinder_length;
	double cylinder_radius;//
	double brake_diameter;//
	double calotific_fuel;//calorific value of fuel
	double steam_exhaust;//steam formed per kg of fuel in the exhaust
	double specific_heat_steam;//specific heat of steam in exhaust
	double specific_heat_dry;//specific heat of dry exhaust gases
	double indicated_hp; //Indicated horse power is the power developed inside the cylinder
	double brake_hp; //brake horse power is the output of the engine at the shaft measured by a dynamometer.
	double thermal_efficiency;//thermal efficiency or mechanical efiiciency of the engine is efined as bp/ip
	double energy_supplied; //energy supplied during the trail
	double heat_equivalent_ip;//heat equivalent of IP in a given time of operation
	double energy_coolingwater;//energy carried away by cooling water
	double mass_exhaustgas; //mass of dry exhaust gas
	double energy_exhaustgas;//energy carried away by dry exhaust gases
	double energy_steam;//energy carried away by steam
	double total_energy_exhaustgas;//total energy carried away by dry exhaust gases is the sum of energy carried away bt steam and energy carried away by dry exhaust gases
	double unaccounted_energyloss;//unaccounted for energy loss

	double Rated_V;
	double Rated_VA;

	//end of diesel engine inputs
	
	//Synchronous gen inputs
	
	double Pconv;//Converted power = Mechanical input - (F & W loasses + Stray losses + Core losses)

	double pf;

	double GenElecEff;
	complex TotalPowerOutput;

	//end of synchronous generator inputs
	double Rs;//< internal transient resistance in p.u.
	double Xs;//< internal transient impedance in p.u.
    double Rg;//< grounding resistance in p.u.
	double Xg;//< grounding impedance in p.u.
	double Max_Ef;//< maximum induced voltage in p.u., e.g. 1.2
    double Min_Ef;//< minimus induced voltage in p.u., e.g. 0.8
	double Max_P;//< maximum real power capacity in kW
    double Min_P;//< minimus real power capacity in kW
	double Max_Q;//< maximum reactive power capacity in kVar
    double Min_Q;//< minimus reactive power capacity in kVar
	double Rated_kVA; //< nominal capacity in kVA
	double Rated_kV; //< nominal line-line voltage in kV
	complex voltage_A;//voltage
	complex voltage_B;
	complex voltage_C;
	complex current_A;      // current
	complex current_B;
	complex current_C;
	complex power_val[3];	//Present power output of the generator
	complex EfA;// induced voltage on phase A in Volt
	complex EfB;
	complex EfC;

	//Convergence criteria (ion right now)
	double rotor_speed_convergence_criterion;

	//Dynamics-capable synchronous generator inputs
	double omega_ref;		//Nominal frequency
	double inertia;			//Inertial constant (H) of generator
	double damping;			//Damping constant (D) of generator
	double number_poles;	//Number of poles in the generator
	double Ra;				//Stator resistance (p.u.)
	double Xd;				//d-axis reactance (p.u.)
	double Xq;				//q-axis reactance (p.u.)
	double Xdp;				//d-axis transient reactance (p.u.)
	double Xqp;				//q-axis transient reactance (p.u.)
	double Xdpp;			//d-axis subtransient reactance (p.u.)
	double Xqpp;			//q-axis subtransient reactance (p.u.)
	double Xl;				//Leakage reactance (p.u.)
	double Tdp;				//d-axis short circuit time constant (s)
	double Tdop;			//d-axis open circuit time constant (s)
	double Tqop;			//q-axis open circuit time constant (s)
	double Tdopp;			//d-axis open circuit subtransient time constant (s)
	double Tqopp;			//q-axis open circuit subtransient time constant (s)
	double Ta;				//Armature short-circuit time constant (s)
	complex X0;				//Zero sequence impedance (p.u.)
	complex X2;				//Negative sequence impedance (p.u.)

	//AVR properties (Simplified Exciter System (SEXS) - Industry acronym, I swear)
	double exc_KA;				//Exciter gain (p.u.)
	double exc_TA;				//Exciter time constant (seconds)
	double exc_TB;				//Exciter transient gain reduction time constant (seconds)
	double exc_TC;				//Exciter transient gain reduction time constant (seconds)
	double exc_EMAX;			//Exciter upper limit (p.u.)
	double exc_EMIN;			//Exciter lower limit (p.u.)
	
	//Governor properties (DEGOV1)
	double gov_degov1_R;				//Governor droop constant (p.u.)
	double gov_degov1_T1;				//Governor electric control box time constant (s)
	double gov_degov1_T2;				//Governor electric control box time constant (s)
	double gov_degov1_T3;				//Governor electric control box time constant (s)
	double gov_degov1_T4;				//Governor actuator time constant (s)
	double gov_degov1_T5;				//Governor actuator time constant (s)
	double gov_degov1_T6;				//Governor actuator time constant (s)
	double gov_degov1_K;				//Governor actuator gain
	double gov_degov1_TMAX; 			//Governor actuator upper limit (p.u.)
	double gov_degov1_TMIN;				//Governor actuator lower limit (p.u.)
	double gov_degov1_TD;				//Governor combustion delay (s)

	//gastflag
	//Governor properties (GAST)
	double gov_gast_R;				//Governor droop constant (p.u.)
	double gov_gast_T1;				//Governor electric control box time constant (s)
	double gov_gast_T2;				//Governor electric control box time constant (s)
	double gov_gast_T3;				//Governor temp limiter time constant (s)
	double gov_gast_AT;				//Governor Ambient Temperature load limit (units)
	double gov_gast_KT;				//Governor temperature control loop gain
	double gov_gast_VMAX;			//Governor actuator upper limit (p.u.)
	double gov_gast_VMIN;			//Governor actuator lower limit (p.u.)
//	double gov_gast_TD;				//Governor combustion delay (s)

	set phases;	/**< device phases (see PHASE codes) */

public:
	/* required implementations */
	diesel_dg(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	//STATUS deltaupdate(unsigned int64 dt, unsigned int iteration_count_val);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS post_deltaupdate(complex *useful_value, unsigned int mode_pass);
public:
	static CLASS *oclass;
	static diesel_dg *defaults;
	complex *get_complex(OBJECT *obj, char *name);
	double *get_double(OBJECT *obj, char *name);
	void convert_Ypn0_to_Yabc(complex Y0, complex Y1, complex Y2, complex *Yabcmat);
	void convert_pn0_to_abc(complex *Xpn0, complex *Xabc);
	void convert_abc_to_pn0(complex *Xabc, complex *Xpn0);
	STATUS apply_dynamics(MAC_STATES *curr_time, MAC_STATES *curr_delta);
	STATUS init_dynamics(MAC_STATES *curr_time);
	complex complex_exp(double angle);

#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
