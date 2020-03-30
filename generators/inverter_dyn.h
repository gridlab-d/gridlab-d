#ifndef GLD_GENERATORS_INVERTER_DYN_H_
#define GLD_GENERATORS_INVERTER_DYN_H_

#include <vector>

#include "generators.h"

EXPORT STATUS preupdate_inverter_dyn(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time);
EXPORT SIMULATIONMODE interupdate_inverter_dyn(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS postupdate_inverter_dyn(OBJECT *obj, complex *useful_value, unsigned int mode_pass);
EXPORT STATUS inverter_dyn_NR_current_injection_update(OBJECT *obj, int64 iteration_count);
EXPORT STATUS inverter_dyn_DC_object_register(OBJECT *this_obj, OBJECT *DC_obj);

// State variables of grid-forming & grid-following controller
typedef struct
{

	///////Grid-Forming
	// state variables in the P measurements
	double p_measure;
	double dp_measure;

	// state variables in the Q measurements
	double q_measure;
	double dq_measure;

	// state variables in the V measurements
	double v_measure;
	double dv_measure;

	// state variables in the voltage control loop
	double V_ini;
	double dV_ini;

	// state variables of the dc bus voltage when using grid-forming PV
	double dVdc_pu;
	double Vdc_pu;

	// state variables of droop control, Pmax and Pmin control
	double ddelta_w_Pmax_ini;
	double delta_w_Pmax_ini;
	double ddelta_w_Pmin_ini;
	double delta_w_Pmin_ini;

	// state variables of Vdc_min controller when using PV grid-forming control
	double ddelta_w_Vdc_min_ini;
	double delta_w_Vdc_min_ini;

	// state variables of frequency and phase angle of the internal voltage
	double delta_w;
	double Angle[3];

	////////Grid-Following
	// state variables in PLL
	double ddelta_w_PLL_ini[3];
	double delta_w_PLL_ini[3];
	double delta_w_PLL[3];
	double Angle_PLL[3];

	//  state variables in current control loop
	double digd_PI_ini[3];
	double igd_PI_ini[3];
	double digq_PI_ini[3];
	double igq_PI_ini[3];

	// state variables using current source representation
	double digd_filter[3];
	double igd_filter[3];
	double digq_filter[3];
	double igq_filter[3];

	//  state variables in frequency-watt
	double df_filter;			  //
	double f_filter;			  //
	double dPref_droop_pu_filter; //
	double Pref_droop_pu_filter;  //

	//  state variables in volt-var
	double dV_filter;			  //
	double V_filter;			  //
	double dQref_droop_pu_filter; //
	double Qref_droop_pu_filter;  //

} INV_DYN_STATE;

typedef struct
{
	FUNCTIONADDR fxn_address;
	OBJECT *dc_object;
} DC_OBJ_FXNS;

//inverter_dyn class
class inverter_dyn : public gld_object
{
private:
	bool deltamode_inclusive; //Boolean for deltamode calls - pulled from object flags
	bool first_sync_delta_enabled;
	char first_iter_counter;

	INV_DYN_STATE pred_state; ///< The predictor state of the inverter in delamode
	INV_DYN_STATE next_state; ///< The next state of the inverter in delamode

	gld_property *pIGenerated[3];		//Link to direct current injections to powerflow at bus-level
	complex generator_admittance[3][3]; //Generator admittance matrix converted from sequence values
	complex filter_admittance;			//Filter admittance value - mostly separate to make single-phase easier
	complex value_IGenerated[3];		//Value/accumulator for IGenerated values

	//Comaptibility variables - used to be in power_electronics
	bool parent_is_a_meter;		 //Boolean to indicate if the parent object is a meter/triplex_meter
	bool parent_is_single_phase; //Boolean to indicate if the parent object is single-phased (main or triplexed)

	gld_property *pCircuit_V[3];   ///< pointer to the three L-N voltage fields
	gld_property *pLine_I[3];	   ///< pointer to the three current fields
	gld_property *pLine_unrotI[3]; ///< pointer to the three pre-rotated current fields
	gld_property *pPower[3];	   ///< pointer to power value on meter parent
	gld_property *pMeterStatus;	   ///< Pointer to service_status variable on meter parent

	//Default or "connecting point" values for powerflow interactions
	complex value_Circuit_V[3];	   ///< value holeder for the three L-N voltage fields
	complex value_Line_I[3];	   ///< value holeder for the three current fields
	complex value_Line_unrotI[3];  ///< value holeder for the three pre-rotated current fields
	complex value_Power[3];		   ///< value holeder for power value on meter parent
	enumeration value_MeterStatus; ///< value holder for service_status variable on meter parent
	complex value_Meter_I[3];	   ///< value holder for meter measured current on three lines

	// Feeder frequency determined by the inverters
	gld_property *mapped_freq_variable; //Mapping to frequency variable in powerflow module - deltamode updates

	gld_property *pbus_full_Y_mat; //Link to the full_Y bus variable -- used for Norton equivalents
	gld_property *pGenerated;	   //Link to pGenerated value - used for Norton equivalents

	SIMULATIONMODE desired_simulation_mode; //deltamode desired simulation mode after corrector pass - prevents starting iterations again

	//Vector for DC object "update" functions
	std::vector<DC_OBJ_FXNS> dc_interface_objects;
	double P_DC;
	double V_DC;
	double I_DC;

	// DC object "update" of steady-state
	double pvc_Pmax;

	//Map functions
	gld_property *map_complex_value(OBJECT *obj, char *name);
	gld_property *map_double_value(OBJECT *obj, char *name);
	void pull_complex_powerflow_values(void);
	void reset_complex_powerflow_accumulators(void);
	void push_complex_powerflow_values(void);

	// Check limit func
	bool check_and_update_VA_Out(OBJECT *obj);

	// Update current func
	void update_iGen(complex);

public:
	set phases;				 /**< device phases (see PHASE codes) */
	enumeration VSI_bustype; // Bus type of the inverter parent
	enum INVERTER_TYPE
	{
		GRID_FORMING = 0,
		GRID_FOLLOWING = 1,
		GFL_CURRENT_SOURCE = 2
	};
	enumeration control_mode; //

	enum GRID_FOLLOWING_TYPE
	{
		BALANCED_POWER = 0,
		POSITIVE_SEQUENCE = 1
	};
	enumeration grid_following_mode; //

	enum GRID_FORMING_TYPE
	{
		CONSTANT_DC_BUS = 0,
		PV_DC_BUS = 1
	};
	enumeration grid_forming_mode; //

	complex temp_current_val[3];
	TIMESTAMP inverter_start_time;
	bool inverter_first_step;
	int64 first_iteration_current_injection; //Initialization variable - mostly so SWING_PQ buses initalize properly for deltamode

	double GridForming_convergence_criterion;

	INV_DYN_STATE curr_state; ///< The current state of the inverter in deltamode

	double value_Frequency; //Value storage for current frequency value

	complex phaseA_V_Out; //voltage
	complex phaseB_V_Out;
	complex phaseC_V_Out;
	complex phaseA_I_Out; // current
	complex phaseB_I_Out;
	complex phaseC_I_Out;
	complex power_val[3];		   //power
	complex VA_Out;				   // complex output power
	complex value_Circuit_V_PS;	   // Positive sequence voltage of three phase terminal voltages
	complex value_Circuit_I_PS[3]; // Positive sequence current of three phase terminal currents, assume no negative or zero sequence, Phase A equals to positive sequence

	double node_nominal_voltage; // Nominal voltage

	double E_mag; //internal voltage magnitude, used for grid-forming control

	bool frequency_watt; // Boolean value indicating whether the f/p droop curve is included in the inverter or not
	bool volt_var;		 // Boolean value indicating whether the volt-var droop curve is included in the inverter or not

	// voltages and currents in dq frame, used for grid-following control
	double ugd_pu[3];
	double ugq_pu[3];
	double igd_pu[3];
	double igq_pu[3];
	double igd_ref[3];
	double igq_ref[3];
	double ugd_pu_PS; // positive sequence voltage value in dq frame
	double ugq_pu_PS; // positive sequence voltage value in dq frame

	// used for grid-following control
	double igd_PI[3];
	double igq_PI[3];
	double ed_pu[3]; // internal votlage in dq frame
	double eq_pu[3]; // internal votlage in dq frame

	double S_base;	 // S_base is the rated caspacity
	double V_base;	 // Vbase is the rated Line to ground voltage
	double Vdc_base; // rated dc bus voltage
	double Idc_base; // rated dc current
	double Z_base;	 // Zbase is the reated impedance
	double I_base;	 // Ibase is the rated current
	double P_out_pu; // P_out_pu is the per unit value of VA_OUT.Re()
	double Q_out_pu; // Q_out_pu is the per-unit value of VA_Out.Im()
	double Tp;		 // Tp is the time constant of low pass filter, it is per-unit value
	double Tq;		 // Tq is the time constant of low pass filter, it is per-unit value
	double Tv;		 // Tv is the time constant of low pass filter,

	double V_ref; // V_ref is the voltage reference obtained from Q-V droop
	double Vset;  // Vset is the voltage set point in grid-forming inverter, usually 1 pu

	double kpv;			  // proportional gain and integral gain of voltage loop
	double kiv;			  //
	double mq;			  // mq is the Q-V droop gain, usually 0.05 pu
	double E_max;		  // E_max and E_min are the maximum and minimum of the output of voltage controller
	double E_min;		  //
	double delta_w_droop; // delta mega from P-f droop
	double delta_w_Pmax;  //
	double delta_w_Pmin;  //
	double Pset;		  // power set point in P-f droop
	double mp;			  // P-f droop gain, usually 3.77 rad/s/pu
	double kppmax;		  // proportional and integral gains for Pmax controller
	double kipmax;
	double w_lim; // w_lim is the saturation limit
	double Pmax;  // Pmax and Pmin are the maximum limit and minimum limit of Pmax controller and Pmin controller
	double Pmin;
	double w_ref;		 // w_ref is the rated frequency, usually 376.99 rad/s
	double f_nominal;	 // rated frequency, 60 Hz
	double Angle[3];	 // Phase angle of the internal voltage
	complex e_source[3]; // e_source[i] is the complex value of internal voltage
	double e_source_Re[3];
	double e_source_Im[3];
	complex I_source[3]; // I_source[i] is the complex value when using current source representation
	double I_source_Re[3];
	double I_source_Im[3];
	double pCircuit_V_Avg_pu; //pCircuit_V_Avg_pu refers to the average value of three phase voltages, it is per-unit value
	double Xfilter;			  // Rfilter and Xfilter are the per-unit values of inverter filter, they are per-unit values
	double Rfilter;
	double freq; // freq is the frequency obtained from the P-f droop controller

	double kpPLL;	// kiPLL and kiPLL are the proportional and integral gains of PLL
	double kiPLL;	//
	double fPLL[3]; // frequency measured by PLL
	double Pref;	// Pref and Qref are the refrences for P and Q
	double Qref;
	double kpc; // kpc and kic are the PI gains of the current loop
	double kic;
	double F_current; // feed forward term gain in current loop

	double Tpf;			  // Tpf is the time constant of low pass filter in frequency-watt
	double Tqf;			  // Tqf is the time constant of low pass filter in volt-var
	double Tvf;			  // Tvf is the time constant of low pass filter in volt-var
	double Tff;			  // Tff is the time constant of low pass filter in frequency measurement
	double Tif;			  // Tif is the low pass filter when using current source representation
	double Vset0;		  // Vset0 is the voltage set point in volt-var in grid-following
	double Pref_droop_pu; // Power reference in frequency-watt
	double Pref_max;	  // Pref_max and Pref_min are the upper and lower limits of power references
	double Pref_min;	  //
	double V_Avg_pu;	  // average voltage in volt-var
	double Qref_droop_pu; // Q reference in volt-var
	double Qref_max;	  // Qref_max and Qref_min are the upper and lower limits of Q references
	double Qref_min;	  //
	double Rp;			  // p-f droop gain in frequency-watt
	double Rq;			  // Q-V droop gain in volt-var

	double m_Vdc;	   //modulation index when using grid-forming PV inverter, it is per unit value
	double Vdc_min_pu; // Tha minimum dc bus voltage that the PV grid-forming inverter can run. It is also the maximum point voltage of PV panel, it is per unit value
	double I_dc_pu;	   // equivalent current at the dc side, which is calculated from the ac side, it is per unit value
	double I_PV_pu;	   // current from the PV panel
	double C_pu;	   // Capacitance of dc bus, it is per unit value

	double kpVdc; //proportional gain of Vdc_min controller
	double kiVdc; //integral gain of Vdc_min controller
	double kdVdc; // derivative gain of Vdc_min controller

	double delta_w_Vdc_min; //variable in the Vdc_min controller

	/* required implementations */
	inverter_dyn(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	STATUS pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS post_deltaupdate(complex *useful_value, unsigned int mode_pass);
	STATUS updateCurrInjection(int64 iteration_count);
	STATUS init_dynamics(INV_DYN_STATE *curr_time);
	STATUS DC_object_register(OBJECT *DC_object);

public:
	static CLASS *oclass;
	static inverter_dyn *defaults;
	static CLASS *plcass;
};

#endif // GLD_GENERATORS_INVERTER_DYN_H_
