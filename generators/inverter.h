/** $Id: inverter.h,v 1.0 2008/07/16
	Copyright (C) 2008 Battelle Memorial Institute
	@file inverter.h
	@addtogroup inverter

 @{  
 **/

#ifndef _inverter_H
#define _inverter_H

#include <utility>		//for pair<> in Volt/VAr schedule
#include <vector>		//for list<> in Volt/VAr schedule
#include <string> //Ab add
#include <stdarg.h>
#include "gridlabd.h"
#include "power_electronics.h"
#include "generators.h"

EXPORT STATUS preupdate_inverter(OBJECT *obj,TIMESTAMP t0, unsigned int64 delta_time);
EXPORT SIMULATIONMODE interupdate_inverter(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS postupdate_inverter(OBJECT *obj, complex *useful_value, unsigned int mode_pass);
EXPORT STATUS inverter_NR_current_injection_update(OBJECT *obj);

//Alternative PI version Dynamic control Inverter state variable structure
typedef struct {
	double P_Out[3];		///< The real power output
	double Q_Out[3];		///< The reactive power output
	double ed[3];			///< The error in real power output
	double eq[3];			///< The error in reactive power output
	double ded[3];		///< The change in real power error
	double deq[3];		///< The change in reactive power error
	double md[3];			///< The d axis current modulator of the inverter
	double mq[3];			///< The q axis current modulator of the inverter
	double dmd[3];			///< The change in d axis current modulator of the inverter
	double dmq[3];			///< The change in q axis current modulator of the inverter
	complex Idq[3];			///< The dq axis current output of the inverter
	complex Iac[3];	///< The AC current out of the inverter terminals

	// Terminal voltage state variable for VSI isochronous mode
	double V_StateVal[3];	// Magnitude of the VSI terminal voltage
	double dV_StateVal[3];	// Change in magnitude of the VSI terminal voltage
	double e_source_mag[3];	// VSI e_source magnitude

	// Frequency state variable for f/p droop
	double f_mea_delayed; // Delay measured frequency value seen by the inverter
	double df_mea_delayed; // The change of the frequency

	// Voltage state variable for v/q droop
	double V_mea_delayed[3]; // Delay measured terminal voltage values seen by the inverter
	double dV_mea_delayed[3]; // The change of the terminal voltage values

	// Real power state variable for f/p drrop in VSI inverter
	double p_mea_delayed;
	double dp_mea_delayed;

	// Reactive power state variable for f/p drrop in VSI inverter
	double q_mea_delayed;
	double dq_mea_delayed;


} INV_STATE;

//Simple PID controller
typedef struct {
	complex error[3];
	complex mod_vals[3];
	complex integrator_vals[3];
	complex derror[3];
	complex current_vals_ref[3];	//Reference-framed current values
	complex current_vals[3];		//Unrotated "powerflow-postable" current values
	complex current_set_raw[3];	//Actual current value
	complex current_set[3];		//Current rotated to common reference frame
	double reference_angle[3];	//Reference angle tracking
	double max_error_val;
	double phase_Pref;
	double phase_Qref;
	double I_in;
} PID_INV_VARS;

//inverter extends power_electronics
class inverter: public power_electronics
{
private:
	bool deltamode_inclusive; 	//Boolean for deltamode calls - pulled from object flags
	bool first_sync_delta_enabled;
	char first_iter_counter;
	INV_STATE pred_state;	///< The predictor state of the inverter in delamode
	INV_STATE next_state;	///< The next state of the inverter in delamode
	bool first_run;

	complex *bus_admittance_mat;		//Link to bus raw self-admittance value - grants 3x3 access instead of diagonal
	complex *full_bus_admittance_mat;	//Link to bus full self-admittance of Ybus form
	complex *PGenerated;				//Link to bus PGenerated field - mainly used for SWING generator
	complex *IGenerated;				//Link to direct current injections to powerflow at bus-level
	complex generator_admittance[3][3];	//Generator admittance matrix converted from sequence values
	complex prev_VA_out[3];				//Previous state tracking variable for ramp-rate calculations
	complex curr_VA_out[3];				//Current state tracking variable for ramp-rate calculations
	double Pref_prev;					//Previous Pref value in the same time step for non-VSI droop mode ramp-rate calculations
	double Qref_prev[3];				//Previous Qref value in the same time step for non-VSI droop mode ramp-rate calculations

protected:
	/* TODO: put unpublished but inherited variables */
public:
	INV_STATE curr_state; ///< The current state of the inverter in deltamode
	enum INVERTER_TYPE {TWO_PULSE=0, SIX_PULSE=1, TWELVE_PULSE=2, PWM=3, FOUR_QUADRANT = 4};
	enumeration inverter_type_v;
	enum GENERATOR_STATUS {OFFLINE=1, ONLINE=2};
	enum DYNAMIC_MODE {PID_CONTROLLER=0, PI_CONTROLLER=1};
	enumeration inverter_dyn_mode;
	enumeration gen_status_v;
	//INVERTER_TYPE inverter_type_choice;
	complex V_In; // V_in (DC)
	double Vdc;
	complex I_In; // I_in (DC)
	complex VA_In; //power in (DC)

	enum PF_REG {INCLUDED=1, EXCLUDED=2, INCLUDED_ALT=3} pf_reg;
	enum PF_REG_STATUS {REGULATING = 1, IDLING = 2} pf_reg_status;
	enum LOAD_FOLLOW_STATUS {IDLE=0, DISCHARGE=1, CHARGE=2} load_follow_status;	//Status variable for what the load_following mode is doing

	complex VA_Out;
	complex VA_Out_past;
	double P_Out;  // P_Out and Q_Out are set by the user as set values to output in CONSTANT_PQ mode
	double Q_Out;
	double p_in; // the power into the inverter from the DC side
	double b_soc; // the SOC of the battery
	double soc_reserve; // the battery reserve in pu
	double p_rated; // the power rating of the inverter per phase
	double bp_rated; // the power rating on the DC side
	double f_nominal;
	double inv_eta; // the inverter's efficiency
	double V_Set_A;
	double V_Set_B;
	double V_Set_C;
	double margin;
	complex I_out_prev;
	complex I_step_max;
	double internal_losses;
	double C_Storage_In;
	double power_factor;
	double P_Out_t0;
	double Q_Out_t0;
	double power_factor_t0;

	complex V_In_Set_A;
	complex V_In_Set_B;
	complex V_In_Set_C; 

	complex *pCircuit_V;		//< pointer to the three voltages on three lines
	complex *pLine_I;			//< pointer to the three current on three lines
	complex *pLine_unrotI;		//< pointer to the three-phase, unrotated current
	complex *pLine12;			//< used in triplex metering
	complex *pPower;			//< pointer to the three power loads on three lines
	complex *pPower12;			//< used in triplex metering
	int *pMeterStatus;			//< Pointer to service_status variable on parent

	complex *pMeter_I;			//< pointer to the meter measured current on three lines

	double output_frequency;
	double frequency_losses;
	
	//Deltamode PID-controller implementation
	double kpd;			///< The proportional gain for the d axis modulation
	double kpq;			///< The proportional gain for the q axis modulation
	double kid;			///< The integrator gain for the d axis modulation
	double kiq;			///< The integrator gain for the q axis modulation
	double kdd;			///< The differentiator gain for the d-axis modulation
	double kdq;			///< The differentiator gain for the q-axis modulation
	PID_INV_VARS prev_PID_state;	///< Previous timestep values
	PID_INV_VARS curr_PID_state;	///< Current timestep values

	double Pref;
	double Qref;
	double Qref_PI[3];    // Qref is set differently for each phase in PI control mode

	double Tfreq_delay;    // Time delay for feeder frequency seen by inverter
	double freq_ref; 	   // Frequency reference value
	double Pref0; 		   //The initial Pref set before entering the delta mode
	bool inverter_droop_fp;   // Boolean value indicating whether the f/p droop curve is included in the inverter or not
	double R_fp;		   // f/p droop curve parameter

	double Tvol_delay;    // Time delay for inverter terminal voltage seen by inverter
	double V_ref[3]; 	   // Voltage reference values for three phases
	double Qref0[3]; 		   //The initial Qref set before entering the delta mode
	bool inverter_droop_vq;   // Boolean value indicating whether the v/q droop curve is included in the inverter or not
	double R_vq;		   // f/p droop curve parameter

	enumeration *VSI_bustype;	// Bus type of the inverter parent

	// Parameters related to VSI mode
	enum VSI_MODE {VSI_ISOCHRONOUS=0, VSI_DROOP=1};
	enumeration VSI_mode;  //operating mode of the VSI
	double VSI_freq;

	double Zbase;			// Zbase of the inverter
	double Rfilter;			// Resistance of filter
	double Xfilter;			// Admittance of filter
	double V_angle_past[3];       // Voltage angle  measured at inverter voltage source behind filter before entering the delta mode
	double V_angle[3];       // Voltage angle measured at inverter voltage source beind filter after entering the delta mode
	double V_mag_ref[3]; 			// Initial voltage magnitude of VSI terminal voltage, used as reference values
	double V_mag[3]; 			// Voltage magnitude of the inverter voltage source
	complex e_source[3]; 	  // Voltage source behind the filter
	double Tp_delay;	  // Time delay for feeder real power changes seen by inverter droop control
	double Tq_delay;	  // Time delay for feeder reactive power changes seen by inverter droop control

	bool checkRampRate_real;		//Flag to enable ramp rate/slew rate checking for active power
	double rampUpRate_real;		//Maximum power increase rate for active power
	double rampDownRate_real;		//Maximum power decrease rate for active power
	bool checkRampRate_reactive;	//Flag to enable ramp rate/slew rate checking for reactive power
	double rampUpRate_reactive;		//Maximum power increase rate for reactive power
	double rampDownRate_reactive;	//Maximum power decrease rate for reactive power

	complex phaseA_I_Out_prev;      // current
	complex phaseB_I_Out_prev;
	complex phaseC_I_Out_prev;

	complex phaseA_V_Out;//voltage
	complex phaseB_V_Out;
	complex phaseC_V_Out;
	complex phaseA_I_Out;      // current
	complex phaseB_I_Out;
	complex phaseC_I_Out;
	complex power_A;//power
	complex power_B;
	complex power_C;
	complex p_clip_A;
	complex p_clip_B;
	complex p_clip_C;
	complex last_current[4];	//Previously applied power output (used to remove from parent so XML files look proper)
	complex last_power[4];		//Previously applied power output (as constant power) - used to remove from parent so XML looks right
	bool islanded;			//ces/nas islanding special boolean.

	//properties for multipoint efficiency model. The model used is from Sandia National Laboratory's 2007 paper "Performance Model For Grid-Connected Photovoltaic Inverters".
	bool use_multipoint_efficiency;
	double p_max;
	double p_dco;	//The dc power at which maximum ac power is reached.(W)
	double v_dco;	//The dc voltage at which maximum ac power is reached.(V)
	double p_so;	//The minumum dc power for inversion.(W)
	double c_o;		//The parameter describing the curvature of the relationship between ac power and dc power at the reference operating condition. default is 0.
	double c_1;		//Coefficient allowing p_dco to vary linearly with dc voltage. default is 0.
	double c_2;		//Coefficient allowing p_so to vary linearly with dc voltage. default is 0.
	double c_3;		//Coefficient allowing c_o to vary linearly with dc voltage. default is 0.
	double C1;
	double C2;
	double C3;
	enum INVERTER_MANUFACTURER {NONE=0,FRONIUS=1,SMA=2,XANTREX=3};
	enumeration inverter_manufacturer; //known manufacturer to set some presets else use variables themselves for custom inverter.

	//properties for four quadrant control modes
	enum FOUR_QUADRANT_CONTROL_MODE {FQM_NONE=0,FQM_CONSTANT_PQ=1,FQM_CONSTANT_PF=2,FQM_CONSTANT_V=3,FQM_VOLT_VAR=4,FQM_LOAD_FOLLOWING=5, FQM_GENERIC_DROOP=6, FQM_GROUP_LF=7, FQM_VOLT_VAR_FREQ_PWR=8, FQM_VSI = 9};
	enumeration four_quadrant_control_mode;

	double excess_input_power;		//Variable tracking excess power on the input that is not placed to the output

	//properties for four quadrant load following control mode
	OBJECT *sense_object;			//Object to sense the power level from and provide adjustments toward
	double max_charge_rate;			//Maximum rate of charge for recharging the battery
	double max_discharge_rate;		//Maximum discharge rate for extracting energy from the battery
	double charge_on_threshold;		//Threshold to enable charging of the battery
	double charge_off_threshold;	//Threshold to disable charging of the battery
	double discharge_on_threshold;	//Threshold to enable discharging of the battery
	double discharge_off_threshold;	//Threshold to disable discharging of the battery
	double charge_lockout_time;		//Time a charge operation is held before another dispatch operation is allowed
	double discharge_lockout_time;	//Time a discharge operation is held before another dispatch operation is allowed

	//properties for pf regulation mode - first control method
	double pf_reg_activate;			//Lowest acceptable power-factor level below which power-factor regulation will activate.
	double pf_reg_deactivate;		//Lowest acceptable power-factor above which no power-factor regulation is needed.
	
	//properties for pf regulation mode - second control method
	double pf_target_var;			//Desired power-factor to maintain (signed)
	double pf_reg_high;				//Upper limit for power factor regulation - if above (other side), go full opposite
	double pf_reg_low;				//Lower limit for power factor regulation - if above, just leave it be (don't regulator)
	
	double pf_reg_activate_lockout_time; //Mandatory pause between the deactivation of power-factor regulation and it reactivation

	//Properties for group load-following
	double charge_threshold;		//Level at which all inverters in the group will begin charging attached batteries. Regulated minimum load level.
	double discharge_threshold;		//Level at which all inverters in the group will begin discharging attached batteries. Regulated maximum load level.
	double group_max_charge_rate;		//Sum of the charge rates of the inverters involved in the group load-following.
	double group_max_discharge_rate;		//Sum of the discharge rates of the inverters involved in the group load-following.
	double group_rated_power;		//Sum of the inverter power ratings of the inverters involved in the group power-factor regulation.
	
	double inverter_convergence_criterion; //The convergence criteria for the dynamic inverter to exit deltamode

	//VoltVar Control Parameters
	double V_base;
	double V1;
	double V2;
	double V3;
	double V4;
	double Q1;
	double Q2;
	double Q3;
	double Q4;
	double m12;
	double b12;
	double m23;
	double b23;
	double m34;
	double b34;
	double getVar(double volt, double m, double b);
	double vv_lockout;
	TIMESTAMP allowed_vv_action;
	TIMESTAMP last_vv_check;
	bool vv_operation;

	//1547 variables
	bool enable_1547_compliance;	//Flag to enable IEEE 1547-2003 condition checking
	double reconnect_time;			//Time after a 1547 violation clears before we reconnect
	bool inverter_1547_status;		//Flag to indicate if we are online, or "curtailed" due to 1547 mapping

	enum IEEE_1547_STATUS {IEEE_NONE=0, IEEE1547=1, IEEE1547A=2};
	enumeration ieee_1547_version;

	//1547(a) frequency
	double over_freq_high_band_setpoint;	//OF2 set point for IEEE 1547a
	double over_freq_high_band_delay;		//OF2 clearing time for IEEE1547a
	double over_freq_high_band_viol_time;	//OF2 violation accumulator
	double over_freq_low_band_setpoint;		//OF1 set point for IEEE 1547a
	double over_freq_low_band_delay;		//OF1 clearing time for IEEE 1547a
	double over_freq_low_band_viol_time;	//OF1 violation accumulator
	double under_freq_high_band_setpoint;	//UF2 set point for IEEE 1547a
	double under_freq_high_band_delay;		//UF2 clearing time for IEEE1547a
	double under_freq_high_band_viol_time;	//UF2 violation accumulator
	double under_freq_low_band_setpoint;	//UF1 set point for IEEE 1547a
	double under_freq_low_band_delay;		//UF1 clearing time for IEEE 1547a
	double under_freq_low_band_viol_time;	//UF1 violation accumulator

	//1547 voltage(a) voltage
	double under_voltage_lowest_voltage_setpoint;	//Lowest voltage threshold for undervoltage
	double under_voltage_middle_voltage_setpoint;	//Middle-lowest voltage threshold for undervoltage
	double under_voltage_high_voltage_setpoint;		//High value of low voltage threshold for undervoltage
	double over_voltage_low_setpoint;				//Lowest voltage value for overvoltage
	double over_voltage_high_setpoint;				//High voltage value for overvoltage
	double under_voltage_lowest_delay;				//Lowest voltage clearing time for undervoltage
	double under_voltage_middle_delay;				//Middle-lowest voltage clearing time for undervoltage
	double under_voltage_high_delay;				//Highest voltage clearing time for undervoltage
	double over_voltage_low_delay;					//Lowest voltage clearing time for overvoltage
	double over_voltage_high_delay;					//Highest voltage clearing time for overvoltage
	double under_voltage_lowest_viol_time;			//Lowest low voltage threshold violation accumulator
	double under_voltage_middle_viol_time;			//Middle low voltage threshold violation accumulator
	double under_voltage_high_viol_time;			//Highest low voltage threshold violation accumulator
	double over_voltage_low_viol_time;				//Lowest high voltage threshold violation accumulator
	double over_voltage_high_viol_time;				//Highest high voltage threshold violation accumulator

	typedef enum {
		IEEE_1547_NONE=0,		/**< No trip reason */
		IEEE_1547_HIGH_OF=1,	/**< High over-frequency level trip */
		IEEE_1547_LOW_OF=2,		/**< Low over-frequency level trip */
		IEEE_1547_HIGH_UF=3,	/**< High under-frequency level trip */
		IEEE_1547_LOW_UF=4,		/**< Low under-frequency level trip */
		IEEE_1547_LOWEST_UV=5,	/**< Lowest under-voltage level trip */
		IEEE_1547_MIDDLE_UV=6,	/**< Middle under-voltage level trip */
		IEEE_1547_HIGH_UV=7,	/**< High under-voltage level trip */
		IEEE_1547_LOW_OV=8,		/**< Low over-voltage level trip */
		IEEE_1547_HIGH_OV=9		/**< High over-voltage level trip */
	};

	enumeration ieee_1547_trip_method;

	//properties for four quadrant volt/var frequency power mode
	bool disable_volt_var_if_no_input_power;		//if true turn off Volt/VAr behavior when no input power (i.e. at night for a solar system)
	double delay_time;				//delay time time between seeing a voltage value and responding with appropiate VAr setting (seconds)
	double max_var_slew_rate;		//maximum rate at which inverter can change its VAr output (VAr/second) *not sure if this is defined anywhere else i.e. power electronics classes
	double max_pwr_slew_rate;		//maximum rate at which inverter can change its power output for frequency regulation (W/second) *not sure if this is defined anywhere else i.e. power electronics classes
	char volt_var_sched[1024];		//user input Volt/VAr Schedule
	char freq_pwr_sched[1024];		//user input freq-power Schedule
	std::vector<std::pair<double,double> > *VoltVArSched;  //Volt/VAr schedule -- i realize I'm using goofball data types, what would be the GridLABD-esque way of implementing this data type? 
	std::vector<std::pair<double,double> > *freq_pwrSched; //freq-power schedule -- i realize I'm using goofball data types, what would be the GridLABD-esque way of implementing this data type? 
private:
	//load following variables
	FUNCTIONADDR powerCalc;				//Address for power_calculate in link object, if it is a link
	bool sense_is_link;					//Boolean flag for if the sense object is a link or a node
	complex *sense_power;				//Link to measured power value fo sense_object
	double lf_dispatch_power;			//Amount of real power to try and dispatch to meet thresholds
	TIMESTAMP next_update_time;			//TIMESTAMP of next dispatching change allowed
	bool lf_dispatch_change_allowed;	//Flag to indicate if a change in dispatch is allowed
	
	//pf regulation variables
	bool pf_reg_dispatch_change_allowed;	//Flag to indicate if a change in dispatch is allowed for power factor regulation
	double pf_reg_dispatch_VAR;		//(Reactive only?) power dispatched to meet power factor regulation threshold.
	TIMESTAMP pf_reg_next_update_time;	//TIMESTAMP of next dispatching change allowed

	TIMESTAMP prev_time;				//Tracking variable for previous "new time" run
	double prev_time_dbl;				//Tracking variable for 1547 checks and ramp rates
	double event_deltat;				//Event-driven delta-t variable

	TIMESTAMP start_time;				//Recording start time of simulation

	complex last_I_Out[3];
	complex I_Out[3];
	double last_I_In;

	//1547 variables
	double out_of_violation_time_total;	//Tracking variable to see how long we've been "outside of bad conditions" to re-enable the inverter
	double *freq_pointer;				//Pointer to frequency value for checking 1547 compliance
	double node_nominal_voltage;		//Nominal voltage for per-unit-izing for 1547 checks

	// Feeder frequency determined by the diesel generators
	double *mapped_freq_variable;  //Mapping to frequency variable in powerflow module - deltamode updates

	//VSI mode tracker - used to initialize current injection pre-deltamode
	bool VSI_esource_init;

	double ki_Vterminal;			///< The integrator gain for the VSI terminal voltage modulation
	double kp_Vterminal;			///< The proportional gain for the VSI terminal voltage modulation

	void update_control_references(void);

public:
	/* required implementations */
	bool *get_bool(OBJECT *obj, char *name);
	inverter(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	STATUS pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS post_deltaupdate(complex *useful_value, unsigned int mode_pass);
	int *get_enum(OBJECT *obj, char *name);
	double perform_1547_checks(double timestepvalue);
	STATUS updateCurrInjection();
	complex check_VA_Out(complex temp_VA, double p_max);
	double getEff(double val);
public:
	static CLASS *oclass;
	static inverter *defaults;
	static CLASS *plcass;
	complex *get_complex(OBJECT *obj, char *name);
	double *get_double(OBJECT *obj, char *name);
	complex complex_exp(double angle);
	STATUS init_PI_dynamics(INV_STATE *curr_time);
	STATUS init_PID_dynamics(void);
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
