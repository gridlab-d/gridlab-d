/** $Id: inverter.h,v 1.0 2008/07/16
	Copyright (C) 2008 Battelle Memorial Institute
	@file inverter.h
	@addtogroup inverter

 @{  
 **/

#ifndef _inverter_H
#define _inverter_H

#include <stdarg.h>
#include "gridlabd.h"
#include "power_electronics.h"
#include "generators.h"

EXPORT SIMULATIONMODE interupdate_inverter(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS postupdate_inverter(OBJECT *obj, complex *useful_value, unsigned int mode_pass);

//Inverter state variable structure
typedef struct {
	double theta_ref;
	double theta_rel;
	double phi_PLL;	
	double VRMSLL;	
	double P;
	double Q;
	double PO;
	double QO;
	double PO_ref_fil;
	double QO_ref_fil;
	double PO_err_int;
	double QO_err_int;
	} INV_STATES;
	

	typedef struct {
	complex v_t[3];
	complex v_t_dq0[3];
	complex v_t_synch[3];
	complex v_O_dq0[3];
	complex v_O_synch[3];
	complex i_t[3];
	complex i_t_dq0[3];	
	complex i_t_synch[3];
	complex i_O[3];
	complex i_O_dq0[3];
	complex i_O_synch[3];
	complex i_Norton[3];
	double PO_ref;
	double QO_ref;
	double PO_err;
	double QO_err;
	double VRMS_true;
	double PO_true;
	double QO_true;
	double PO_ref_lim;
	double QO_ref_lim;
	double v_t_synch_theta_refs[3];
	double v_t_synch_mag_refs[3];

	double w_PLL;
	double f_PLL;
	bool islanded;
	INV_STATES x;
	} INV_VARS;
	
	typedef struct {
		double KPPLL;
		double KIPLL;
		double KPP;
		double KIP;
		double KPQ;
		double KIQ;
		double T_MeasP;
		double T_MeasQ;
		double T_MeasV;
		double T_PRefFilter;
		double T_QRefFilter;
		double angleRefMin;
		double angleRefMax;
		double magRefMin;
		double magRefMax;
	} CTRL_PARAMS;


//inverter extends power_electronics
class inverter: public power_electronics
{
private:
	bool IterationToggle;	///< Iteration toggle device - retains NR "pass" functionality
	bool first_run;		///< Flag for first run of the diesel_dg object - eliminates t0==0 dependence
	bool first_run_after_delta;	///< Flag for first run after deltamode - namely because powerflow is in true pass
	INV_VARS curr_state;		//Current state of all vari
	INV_VARS next_state;
	INV_VARS pred_derivs;	//Predictor pass values of variables
	INV_VARS corr_derivs;	//Corrector pass values of variables

	bool deltamode_inclusive;	//Boolean for deltamode calls - pulled from object flags
	bool first_sync_delta_enabled;

	complex *y_bus;		//Link to bus raw self-admittance value - grants 3x3 access instead of diagonal
	complex *full_y_bus;	//Link to bus full self-admittance of Ybus form
	complex *PGenerated;				//Link to bus PGenerated field - mainly used for SWING generator
	complex *IGenerated;				//Link to direct current injections to powerflow at bus-level
	complex *FreqPower;					//Link to bus "frequency-power weighted" accumulation
	complex *TotalPower;
	double PO_prev_it;
	double QO_prev_it;
protected:
	/* TODO: put unpublished but inherited variables */
public:
	enum INVERTER_TYPE {TWO_PULSE=0, SIX_PULSE=1, TWELVE_PULSE=2, PWM=3, FOUR_QUADRANT = 4};
	enumeration inverter_type_v;
	enum GENERATOR_STATUS {OFFLINE=1, ONLINE=2};
	enumeration gen_status_v;
	//INVERTER_TYPE inverter_type_choice;
	complex V_In; // V_in (DC)
	double Vdc;
	complex I_In; // I_in (DC)
	complex VA_In; //power in (DC)

	enum LOAD_FOLLOW_STATUS {IDLE=0, DISCHARGE=1, CHARGE=2} load_follow_status;	//Status variable for what the load_following mode is doing
	enum COUPLING_INDUCTANCE_TYPE {COUPLING_NONE=0,COUPLING_L=1,COUPLING_LCL=2} coupling_inductance_type;

	
	complex VA_Out;
	double P_Out;  // P_Out and Q_Out are set by the user as set values to output in CONSTANT_PQ mode
	double Q_Out;
	double p_in; // the power into the inverter from the DC side
	double b_soc; // the SOC of the battery
	double soc_reserve; // the battery reserve in pu
	double p_rated; // the power rating of the inverter per phase
	double bp_rated; // the power rating on the DC side
	double f_nominal;
	double omega_nominal;
	double VRMSLG_nominal;
	double VRMSLL_nominal;
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
	complex *pLine12;			//< used in triplex metering
	complex *pPower;			//< pointer to the three power loads on three lines
	complex *pPower12;			//< used in triplex metering
	int *pMeterStatus;			//< Pointer to service_status variable on parent

	double output_frequency;
	double frequency_losses;
	

	

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
	enum FOUR_QUADRANT_CONTROL_MODE {FQM_NONE=0,FQM_CONSTANT_PQ=1,FQM_CONSTANT_PF=2,FQM_CONSTANT_V=3,FQM_VOLT_VAR=4,FQM_LOAD_FOLLOWING=5, FQM_GENERIC_DROOP=6};
	enumeration four_quadrant_control_mode;
	enum CONTROL_MODE_SWITCH {CONTROL_SWITCH_NONE=0, ISLANDING_DROOP=1};
	enumeration control_mode_switch;

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

	CTRL_PARAMS *active_params;
	CTRL_PARAMS PQ_params;
	CTRL_PARAMS droop_PQ_params;
	
	double inverter_convergence_criterion;
	
	//droop control properties
	//double droop_RP;
	//double droop_RQ;
	double droop_f1;
	double droop_f2;
	double droop_V1;
	double droop_V2;
	double droop_P1;
	double droop_P2;
	double droop_Q1;
	double droop_Q2;
	
	//coupling impedance properties, currently only used in deltamode
	double coupling_L1;
	double coupling_L2;
	double coupling_C;
	
private:
	//load following variables
	FUNCTIONADDR powerCalc;				//Address for power_calculate in link object, if it is a link
	bool sense_is_link;					//Boolean flag for if the sense object is a link or a node
	complex *sense_power;				//Link to measured power value fo sense_object
	double lf_dispatch_power;			//Amount of real power to try and dispatch to meet thesholds
	TIMESTAMP next_update_time;			//TIMESTAMP of next dispatching change allowed
	bool lf_dispatch_change_allowed;	//Flag to indicate if a change in dispatch is allowed

	
	TIMESTAMP prev_time;				//Tracking variable for previous "new time" run


public:
	/* required implementations */
	bool *get_bool(OBJECT *obj, char *name);
	inverter(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS post_deltaupdate(complex *useful_value, unsigned int mode_pass);
	int *get_enum(OBJECT *obj, char *name);
public:
	static CLASS *oclass;
	static inverter *defaults;
	static CLASS *plcass;
	complex *get_complex(OBJECT *obj, char *name);
	double *get_double(OBJECT *obj, char *name);
	void convert_dq0_to_abc(complex *Xdq0, complex *Xabc, double *theta);
	void convert_abc_to_dq0(complex *Xabc, complex *Xdq0, double *theta);
	double fmin(double a, double b);
	double fmin(double a, double b, double c);
	double fmax(double a, double b);
	double fmax(double a, double b, double c);
	STATUS apply_dynamics(INV_VARS *curr_time, INV_VARS *curr_delta);
	STATUS init_dynamics(INV_VARS *curr_time,INV_VARS *corr_derivs);
	complex complex_exp(double angle);
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
/** $Id: inverter.h,v 1.0 2008/06/16
	@file inverter.h
	@addtogroup inverter
	@ingroup MODULENAME

 @{  
 **/

#ifndef _inverter_H
#define _inverter_H

#include <stdarg.h>
#include "gridlabd.h"

class inverter {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
public:
	/* required implementations */
	inverter(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static inverter *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
