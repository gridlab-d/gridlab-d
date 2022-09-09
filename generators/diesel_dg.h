/** $Id: diesel_dg.h,v 1.2 2008/02/12 00:28:08 d3g637 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file diesel_dg.h
	@addtogroup diesel_dg

 @{  
 **/

#ifndef _diesel_dg_H
#define _diesel_dg_H

#include <stdarg.h>

#include "generators.h"

EXPORT int isa_diesel_dg(OBJECT *obj, char *classname);
EXPORT SIMULATIONMODE interupdate_diesel_dg(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS postupdate_diesel_dg(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass);
EXPORT STATUS diesel_dg_NR_current_injection_update(OBJECT *obj,int64 iteration_count, bool *converged_failure);

//AVR state variable structure
typedef struct {
	double bias;			//Bias term of avr
	double xe;				//State variable
	double xb;				//State variable for transient gain reduction

	double xfd; 			// State variable for PI control of the Q constant mode

//	double x_cvr;			// State variable for PI controller for CVR control
//	double xerr_cvr;		// State variable for PID controller for CVR control
	double x_cvr1;			// State variable for PI controller for CVR control
	double x_cvr2;			// State variable for PI controller for CVR control
	double diff_f;			// Difference between measured and reference frequency
} AVR_VARS;

//GOV_DEGOV1 state variable structure
typedef struct {
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
	double x1;				//Electric box state variable 
	double x2;				//Electric box state variable
	double x3;				//Temp limiter state variable
	double throttle;		//governor actuator output
} GOV_GAST_VARS;

//GGOV1 state variable structure
typedef struct {
	double werror;
	double x1;
	double x2;
	double x2a;
	double x3;
	double x3a;
	double x4;
	double x4a;
	double x4b;
	double x5;
	double x5a;
	double x5b;
	double x6;
	double x7;
	double x7a;
	double x8;
	double x8a;
	double x9;
	double x9a;
	double x10;
	double x10a;
	double x10b;
	double ValveStroke;
	double FuelFlow;
	double GovOutPut;
	double RselectValue;
	double fsrn;
	double fsrtNoLim;
	double fsrt;
	double fsra;
	double err2;
	double err2a;
	double err3;
	double err4;
	double err7;
	double LowValSelect1;
	double LowValSelect;
} GOV_GGOV1_VARS;

// P_CONSTANT mode state variable structure
typedef struct {
	double x1;
	double x4;
	double x4a;
	double x4b;
	double x5;
	double x5a;
	double x5b;
	double err4;
	double ValveStroke;
	double FuelFlow;
	double GovOutPut;
	double x_Pconstant;
} GOV_P_CONSTANT_VARS;

//Machine state variable structure
typedef struct {
	double rotor_angle;			//Rotor angle of machine
	double omega;				//Current speed of machine
	double Vfd;					//Field voltage of machine
	double Flux1d;				//Transient flux on d-axis
	double Flux2q;				//Sub-transient flux on q-axis
	gld::complex EpRotated;			//d-q rotated E' internal voltage
	gld::complex VintRotated;		//d-q rotated Vint voltage
	gld::complex EintVal[3];			//Unrotated, un-sequenced internal voltage
	gld::complex Irotated;			//d-q rotated current value
	gld::complex pwr_electric;		//Total electric power output of generator
	double pwr_mech;			//Mechanical power output of generator
	double torque_mech;			//Mechanical torque of generator
	double torque_elec;			//electrical torque of generator
	GOV_DEGOV1_VARS gov_degov1;	//DEGOV1 Governor state variables
	GOV_GAST_VARS gov_gast;		//GAST Governor state variables
	GOV_GGOV1_VARS gov_ggov1;	//GGOV1 governor state variables
	GOV_P_CONSTANT_VARS gov_pconstant; //P_CONSTANT mode state variables
	AVR_VARS avr;				//Automatic Voltage Regulator state variables
} MAC_STATES;

//Set-point/adjustable variables
typedef struct {
	double wref;	//Reference frequency/bias for generator object (governor)
	double w_ref;	//Reference frequency/bias for generator object (governor), but rad/s (align with other generators)
	double f_set;   //Reference frequency in Hz
	double vset;	//Reference per-unit voltage/bias for generator object (AVR)
	double vseta;	//Reference per-unit voltage/bias for generator object (AVR) before going into bound check
	double vsetb;	//Reference per-unit voltage/bias for generator object (AVR) after going into bound check
	double vadd;	//Additioal value added to the field voltage before going into the bound
	double vadd_a;	//Additioal value added to the field voltage before going into the bound before going into bound check
	double Pref;	//Reference real power output/bias (per-unit) for generator object (governor)
	double Qref;	//Reference reactive power output/bias (per-unit) for generator object (AVR)
} MAC_INPUTS;

class diesel_dg : public gld_object
{
private:
	double Rated_V_LN;	//Rated voltage - LN value

	gld_property *pCircuit_V[3]; ///< pointer to the three voltages on three lines
	gld_property *pLine_I[3]; ///< pointer to the three current on three lines
	gld_property *pPower[3];	///< pointer to the three powers on three lines

	gld::complex value_Circuit_V[3];	///Storage variable for voltages
	gld::complex value_Line_I[3];	///Storage variable for currents
	gld::complex value_Power[3];		///Storage variable for power
	gld::complex value_prev_Power[3];	///Storage variable for previous power - mostly for accumulator handling

	bool parent_is_powerflow;
	enumeration attached_bus_type;	//Determines attached bus type

	FUNCTIONADDR swing_test_fxn;	//Function to map to swing testing function, if needed

	bool first_run;		///< Flag for first run of the diesel_dg object - eliminates t0==0 dependence
	bool only_first_init;		///< Flag to indicate if we should limit the dynamics initialization to only the very first run of deltamode
	bool first_init_status;		///< Flag to see if that first init has actually occurred
	bool is_isochronous_gen;	///< Flag to indicate if we're isochronous, mostly to help keep us in deltamode

	TIMESTAMP diesel_start_time;
	bool diesel_first_step;

	//Internal synchronous machine variables
	gld_property *pbus_full_Y_mat;		//Link to the full_Y bus variable -- used for Norton equivalents
	gld_property *pbus_full_Y_all_mat;	//Link to the full_Y_all bus variable -- used for Norton equivalents
	gld_property *pPGenerated;			//Link to bus PGenerated field - mainly used for SWING generator
	gld_property *pIGenerated[3];		//Link to direct current injections to powerflow at bus-level (prerot current)
	gld::complex value_IGenerated[3];		//Accumulator/holding variable for direct current injections at bus-level (pre-rotated current)
	gld::complex prev_value_IGenerated[3];	//Tracking variable - mostly for initial powerflow convergence of Norton equivalent
	gld::complex generator_admittance[3][3];	//Generator admittance matrix converted from sequence values
	gld::complex full_bus_admittance_mat[3][3];	//Full self-admittance of Ybus form - pulled from node connection
	double power_base;					//Per-phase basis (divide by 3)
	double voltage_base;				//Voltage p.u. base for analysis (converted from delta)
	double current_base;				//Current p.u. base for analysis
	double impedance_base;				//Impedance p.u. base for analysis
	gld::complex YS0;						//Zero sequence admittance - scaled (not p.u.)
	gld::complex YS1;						//Positive sequence admittance - scaled (not p.u.)
	gld::complex YS2;						//Negative sequence admittance - scaled (not p.u.)
	gld::complex YS1_Full;					//Positive sequence admittance - full Ybus value
	double Rr;							//Rotor resistance term - derived
	double *torque_delay;				//Buffer of delayed governor torques
	double *x5a_delayed;					//Buffer of delayed x5a variables (ggov1)
	unsigned int torque_delay_len;		//Length of the torque delay buffer
	unsigned int torque_delay_write_pos;//Indexing variable for writing the torque_delay_buffer
	unsigned int torque_delay_read_pos;	//Indexing variable for reading torque_delay_buffer
	unsigned int x5a_delayed_len;		//Length of the torque delay buffer
	unsigned int x5a_delayed_write_pos;//Indexing variable for writing the torque_delay_buffer
	unsigned int x5a_delayed_read_pos;	//Indexing variable for reading torque_delay_buffer
	double prev_rotor_speed_val;		//Previous value of rotor speed - used for delta-exiting convergence check
	double prev_voltage_val[3];			//Previous value of voltage magnitude - used for delta-exiting convergence check
	gld::complex last_power_output[3];		//Tracking variable for previous power output - used to do super-second frequency adjustments
	TIMESTAMP prev_time;				//Tracking variable for previous "new time" run
	double prev_time_dbl;				//Tracking variable for previous "new time" run -- deltamode capable

	MAC_STATES curr_state;		//Current state of all vari
	MAC_STATES next_state;
	MAC_STATES predictor_vals;	//Predictor pass values of variables
	MAC_STATES corrector_vals;	//Corrector pass values of variables

	bool deltamode_inclusive;	//Boolean for deltamode calls - pulled from object flags
	gld_property *mapped_freq_variable;	//Mapping to frequency variable in powerflow module - deltamode updates
	int64 first_iteration_current_injection;	//Initialization variable - mostly so SWING_PQ buses initalize properly for deltamode

	double Overload_Limit_Value;	//The computed maximum output power, based on the Rated_VA and the Overload_Limit_Value
	SIMULATIONMODE desired_simulation_mode;	//deltamode desired simulation mode after corrector pass - prevents starting iterations again

protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	enum {DYNAMIC=1, NON_DYN_CONSTANT_PQ};
	enumeration Gen_type;

	//Dynamics synchronous generator capabilities
	enum {NO_EXC=1, SEXS=2};
	enumeration Exciter_type;

	//Dynamics synchronous generator capabilities
	enum {SEXS_CV=1,SEXS_CQ=2,SEXS_Q_V_DROOP=3};
	enumeration SEXS_mode;


	//gastflag
	enum {NO_GOV=1, DEGOV1=2, GAST=3, GGOV1_OLD=4, GGOV1=5, P_CONSTANT=6};
	enumeration Governor_type;

	enum P_F_DROOP_SETTING_TYPE
	{
		FSET_MODE = 0,
		PSET_MODE = 1
	};
	enumeration P_f_droop_setting_mode; //

	//Enable/Disable low-value select blocks
	bool gov_ggv1_fsrt_enable;	//Enables/disables top fsrt of low-value-select (load limiter)
	bool gov_ggv1_fsra_enable;	//Enables/disables middle fsra of low-value select (acceleration limiter)
	bool gov_ggv1_fsrn_enable;	//Enables/disables lower fsrn of low-value select (normal PID controller)

	//General properties
	double Rated_V_LL;	//Rated voltage - LL value
	double Rated_VA;
	double Overload_Limit_Pub;	//Maximum rating for the generator, in per-unit

	//Synchronous gen inputs
	
	double Max_Ef;//< maximum induced voltage in p.u., e.g. 1.2
    double Min_Ef;//< minimus induced voltage in p.u., e.g. 0.8
	gld::complex current_val[3];	//Present current output of the generator
	gld::complex power_val[3];	//Present power output of the generator
	double real_power_val[3];
	double imag_power_val[3];

	//Convergence criteria (ion right now)
	double rotor_speed_convergence_criterion;
	double voltage_convergence_criterion;
	double current_convergence_criterion;

	//Which convergence to apply
	bool apply_rotor_speed_convergence;
	bool apply_voltage_mag_convergence;

	//Dynamics-capable synchronous generator inputs
	double omega_ref;		//Nominal frequency
	double f_nominal;        // Nominal frequency in Hz
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
	gld::complex X0;				//Zero sequence impedance (p.u.)
	gld::complex X2;				//Negative sequence impedance (p.u.)

	MAC_INPUTS gen_base_set_vals;	//Base set points for the various control objects
	bool Vset_defined;				// Flag indicating whether Vset has been defined in glm file or not

	//AVR properties (Simplified Exciter System (SEXS) - Industry acronym, I swear)
	double exc_KA;				//Exciter gain (p.u.)
	double exc_TA;				//Exciter time constant (seconds)
	double exc_TB;				//Exciter transient gain reduction time constant (seconds)
	double exc_TC;				//Exciter transient gain reduction time constant (seconds)
	double exc_EMAX;			//Exciter upper limit (p.u.)
	double exc_EMIN;			//Exciter lower limit (p.u.)

	double ki_Pconstant;		// ki for the PI controller implemented in P constant delta mode
	double kp_Pconstant;		// kp for the PI controller implemented in P constant delta mode

	bool P_constant_mode; 		// Flag indicating whether P constant mode is imployed
	double ki_Qconstant;		// ki for the PI controller implemented in Q constant delta mode
	double kp_Qconstant;		// kp for the PI controller implemented in Q constant delta mode
	
	double mq_QV_Droop; // Q-V droop slope
	double Vset_QV_droop; //Voltage setpoint of QV droop

	// parameters related to CVR control in AVR
	bool CVRenabled;				// Flag indicating whether CVR control is enabled or not inside the exciter
	double ki_cvr;					// Integral gain for PI/PID controller of the CVR control
	double kp_cvr;					// Proportional gain for PI/PID controller of the CVR control
	double kd_cvr; 					// Deviation gain for PID controller of the CVR control
	double kt_cvr;					// Gain for feedback loop in CVR control
	double kw_cvr;	 				// Gain for feedback loop in CVR control
	bool CVR_PI;					// Flag indicating CVR implementation is using PI controller
	bool CVR_PID;					// Flag indicating CVR implementation is using PID controller
	double Vref;					// vset initial value before entering transient
	double Kd1;						// Parameter of second order transfer function
	double Kd2;						// Parameter of second order transfer function
	double Kd3;						// Parameter of second order transfer function
	double Kn1;						// Parameter of second order transfer function
	double Kn2;						// Parameter of second order transfer function
	double vset_delta_MAX;			// Delta Vset uppper limit
	double vset_delta_MIN;			// Delta Vset lower limit

	//CVR mode choices
	enum {HighOrder=1, Feedback};
	enumeration CVRmode;

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

	//Governor properties (GGOV1)
	double gov_ggv1_r;				//Permanent droop, p.u.
	unsigned int gov_ggv1_rselect; 	//Feedback signal for droop, = 1 selected electrical power, = 0 none (isochronous governor), = -1 fuel valve stroke ( true stroke),= -2 governor output ( requested stroke)
	double gov_ggv1_Tpelec;			//Electrical power transducer time constant, sec. (>0.)
	double gov_ggv1_maxerr;			//Maximum value for speed error signal
	double gov_ggv1_minerr;			//Minimum value for speed error signal
	double gov_ggv1_Kpgov;			//Governor proportional gain
	double gov_ggv1_Kigov;			//Governor integral gain
	double gov_ggv1_Kdgov;			//Governor derivative gain
	double gov_ggv1_Tdgov;			//Governor derivative controller time constant, sec.
	double gov_ggv1_vmax;			//Maximum valve position limit
	double gov_ggv1_vmin;			//Minimum valve position limit
	double gov_ggv1_Tact;			//Actuator time constant
	double gov_ggv1_Kturb;			//Turbine gain (>0.)
	double gov_ggv1_wfnl;			//No load fuel flow, p.u
	double gov_ggv1_Tb;				//Turbine lag time constant, sec. (>0.)
	double gov_ggv1_Tc;				//Turbine lead time constant, sec.
	unsigned int gov_ggv1_Flag;		//Switch for fuel source characteristic, = 0 for fuel flow independent of speed, = 1 fuel flow proportional to speed
	double gov_ggv1_Teng;			//Transport lag time constant for diesel engine
	double gov_ggv1_Tfload;			//Load Limiter time constant, sec. (>0.)
	double gov_ggv1_Kpload;			//Load limiter proportional gain for PI controller
	double gov_ggv1_Kiload;			//Load limiter integral gain for PI controller
	double gov_ggv1_Ldref;			//Load limiter reference value p.u.
	double gov_ggv1_Dm;				//Speed sensitivity coefficient, p.u.
	double gov_ggv1_ropen;			//Maximum valve opening rate, p.u./sec.
	double gov_ggv1_rclose;			//Minimum valve closing rate, p.u./sec.
	double gov_ggv1_Kimw;			//Power controller (reset) gain
	double gov_ggv1_Pmwset;			//Power controller setpoint, MW
	double gov_ggv1_aset;			//Acceleration limiter setpoint, p.u. / sec.
	double gov_ggv1_Ka;				//Acceleration limiter Gain
	double gov_ggv1_Ta;				//Acceleration limiter time constant, sec. (>0.)
	double gov_ggv1_db;				//Speed governor dead band
	double gov_ggv1_Tsa;			//Temperature detection lead time constant, sec.
	double gov_ggv1_Tsb;			//Temperature detection lag time constant, sec.
	//double gov_ggv1_rup;				//Maximum rate of load limit increase
	//double gov_ggv1_rdown;			//Maximum rate of load limit decrease

	// P_CONSTANT mode properties
	double pconstant_Tpelec;		//Electrical power transducer time constant, sec. (>0.)
	double pconstant_Tact;			//Actuator time constant
	double pconstant_Kturb;			//Turbine gain (>0.)
	double pconstant_wfnl;			//No load fuel flow, p.u
	double pconstant_Tb;			//Turbine lag time constant, sec. (>0.)
	double pconstant_Tc;			//Turbine lead time constant, sec.
	double pconstant_Teng;			//Transport lag time constant for diesel engine
	double pconstant_ropen;			//Maximum valve opening rate, p.u./sec.
	double pconstant_rclose;		//Minimum valve closing rate, p.u./sec.
	unsigned int pconstant_Flag;	//Switch for fuel source characteristic, = 0 for fuel flow independent of speed, = 1 fuel flow proportional to speed

	// Fuel useage and emmisions
	bool fuelEmissionCal;		// Boolean value indicating whether calculation of fuel and emissions are enabled
	double outputEnergy;		// Total energy(kWh) output from the generator
	double FuelUse;				// Total fuel usage (gal) based on kW power output
	double efficiency;			// Total energy output per fuel usage (kWh/gal)
	double CO2_emission;		// Total CO2 emissions (lbs) based on fule usage
	double SOx_emission;		// Total SOx emissions (lbs) based on fule usage
	double NOx_emission;		// Total NOx emissions (lbs) based on fule usage
	double PM10_emission;		// Total PM-10 emissions (lbs) based on fule usage
	TIMESTAMP last_time;
	double dg_1000_a;			// Parameter to calculate fuel usage (gal)based on VA power output (for 1000 kVA rating dg)
	double dg_1000_b;			// Parameter to calculate fuel usage (gal)based on VA power output (for 1000 kVA rating dg)

	// Relationship between frequency deviation and real power changes
	double frequency_deviation;
	double frequency_deviation_energy;
	double frequency_deviation_max;
	double realPowerChange;
	double ratio_f_p;
	double pwr_electric_init;

	//CONSTANT_PQ P and Q total oupput.
	double real_power_gen;
	double imag_power_gen;

public:
	/* required implementations */
	diesel_dg(MODULE *module);
	int isa(char *classname);

	void check_power_output();
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	//STATUS deltaupdate(unsigned int64 dt, unsigned int iteration_count_val);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass);
	STATUS updateCurrInjection(int64 iteration_count, bool *converged_failure);
public:
	static CLASS *oclass;
	static diesel_dg *defaults;
	void convert_Ypn0_to_Yabc(gld::complex Y0, gld::complex Y1, gld::complex Y2, gld::complex *Yabcmat);
	void convert_pn0_to_abc(gld::complex *Xpn0, gld::complex *Xabc);
	void convert_abc_to_pn0(gld::complex *Xabc, gld::complex *Xpn0);
	STATUS apply_dynamics(MAC_STATES *curr_time, MAC_STATES *curr_delta, double deltaT);
	STATUS init_dynamics(MAC_STATES *curr_time);
	gld::complex complex_exp(double angle);

	friend class controller_dg;

	gld_property *map_complex_value(OBJECT *obj, const char *name);
	gld_property *map_double_value(OBJECT *obj, const char *name);
	void pull_powerflow_values(void);
	void push_powerflow_values(bool update_voltage);
	void reset_powerflow_accumulators(void);

#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
