#ifndef GLD_GENERATORS_INVERTER_DYN_H_
#define GLD_GENERATORS_INVERTER_DYN_H_

#include <vector>

#include "generators.h"

EXPORT int isa_inverter_dyn(OBJECT *obj, char *classname);
EXPORT STATUS preupdate_inverter_dyn(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time);
EXPORT SIMULATIONMODE interupdate_inverter_dyn(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS postupdate_inverter_dyn(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass);
EXPORT STATUS inverter_dyn_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure);
EXPORT STATUS inverter_dyn_DC_object_register(OBJECT *this_obj, OBJECT *DC_obj);

//Alias the currents
#define phaseA_I_Out terminal_current_val[0]
#define phaseB_I_Out terminal_current_val[1]
#define phaseC_I_Out terminal_current_val[2]

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

	// state variables of the Qmax and Qmin control
	double ddelta_V_Qmax_ini;
	double delta_V_Qmax_ini;
	double ddelta_V_Qmin_ini;
	double delta_V_Qmin_ini;


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
	bool deltamode_exit_iteration_met;

	double prev_timestamp_dbl;
	double last_QSTS_GF_Update;

	INV_DYN_STATE pred_state; ///< The predictor state of the inverter in delamode
	INV_DYN_STATE next_state; ///< The next state of the inverter in delamode

	gld_property *pIGenerated[3];		//Link to direct current injections to powerflow at bus-level
	gld::complex generator_admittance[3][3]; //Generator admittance matrix converted from sequence values
	gld::complex filter_admittance;			//Filter admittance value - mostly separate to make single-phase easier
	gld::complex value_IGenerated[3];		//Value/accumulator for IGenerated values
	gld::complex prev_value_IGenerated[3];	//Tracking variable for grid following "QSTS exit"

	//Comaptibility variables - used to be in power_electronics
	bool parent_is_a_meter;		 //Boolean to indicate if the parent object is a meter/triplex_meter
	bool parent_is_single_phase; //Boolean to indicate if the parent object is single-phased (main or triplexed)
	bool parent_is_triplex;		//Boolean to indicate if the parent object was a triplex device - minimal usage
	enumeration attached_bus_type;	//Determines attached bus type - mostly for VSI and grid-forming functionality

	FUNCTIONADDR swing_test_fxn;	//Function to map to swing testing function, if needed

	gld_property *pCircuit_V[3];   ///< pointer to the three L-N voltage fields
	gld_property *pLine_I[3];	   ///< pointer to the three current fields
	gld_property *pLine_unrotI[3]; ///< pointer to the three pre-rotated current fields
	gld_property *pPower[3];	   ///< pointer to power value on meter parent
	gld_property *pMeterStatus;	   ///< Pointer to service_status variable on meter parent
	gld_property *pSOC;            ///< Pointer to battery SOC


	//Default or "connecting point" values for powerflow interactions
	gld::complex value_Circuit_V[3];	   ///< value holder for the three L-N voltage fields
	gld::complex value_Line_I[3];	   ///< value holder for the three current fields
	gld::complex value_Line_unrotI[3];  ///< value holder for the three pre-rotated current fields
	gld::complex value_Power[3];		   ///< value holder for power value on meter parent
	enumeration value_MeterStatus; ///< value holder for service_status variable on meter parent
	gld::complex value_Meter_I[3];	   ///< value holder for meter measured current on three lines

	gld_property *pbus_full_Y_mat; //Link to the full_Y bus variable -- used for Norton equivalents
	gld_property *pGenerated;	   //Link to pGenerated value - used for Norton equivalents

	SIMULATIONMODE desired_simulation_mode; //deltamode desired simulation mode after corrector pass - prevents starting iterations again

	//Vector for DC object "update" functions
	std::vector<DC_OBJ_FXNS> dc_interface_objects;
	double P_DC;
	double V_DC;
	double I_DC;
	double SOC;


	// DC object "update" of steady-state
	double pvc_Pmax;

	//Convergence check item for grid-forming voltage
	gld::complex e_droop_prev[3];

	//Map functions
	gld_property *map_complex_value(OBJECT *obj, const char *name);
	gld_property *map_double_value(OBJECT *obj, const char *name);
	void pull_complex_powerflow_values(void);
	void reset_complex_powerflow_accumulators(void);
	void push_complex_powerflow_values(bool update_voltage);

	// Check limit func
	void check_and_update_VA_Out(OBJECT *obj);

	// Update current func
	void update_iGen(gld::complex);

	//*** IEEE 1547 functionality ***//
	bool Reconnect_Warn_Flag;		//Flag to warn on a 1547 recovery that the behavior is not fully validated

	bool enable_1547_compliance;	//Flag to enable IEEE 1547-2003/2014 condition checking
	double IEEE1547_reconnect_time;			//Time after a 1547 violation clears before we reconnect
	bool inverter_1547_status;		//Flag to indicate if we are online, or "curtailed" due to 1547 mapping

	enum IEEE_1547_STATUS {IEEE1547_NONE=0, IEEE1547_2003=1, IEEE1547A_2014=2,IEEE1547_2018=3};
	enumeration ieee_1547_version;

	//1547(a) frequency
	double IEEE1547_over_freq_high_band_setpoint;	//OF2 set point for IEEE 1547a-2014
	double IEEE1547_over_freq_high_band_delay;		//OF2 clearing time for IEEE1547a-2014
	double IEEE1547_over_freq_high_band_viol_time;	//OF2 violation accumulator
	double IEEE1547_over_freq_low_band_setpoint;	//OF1 set point for IEEE 1547a-2014
	double IEEE1547_over_freq_low_band_delay;		//OF1 clearing time for IEEE 1547a-2014
	double IEEE1547_over_freq_low_band_viol_time;	//OF1 violation accumulator
	double IEEE1547_under_freq_high_band_setpoint;	//UF2 set point for IEEE 1547a-2014
	double IEEE1547_under_freq_high_band_delay;		//UF2 clearing time for IEEE1547a-2014
	double IEEE1547_under_freq_high_band_viol_time;	//UF2 violation accumulator
	double IEEE1547_under_freq_low_band_setpoint;	//UF1 set point for IEEE 1547a-2014
	double IEEE1547_under_freq_low_band_delay;		//UF1 clearing time for IEEE 1547a-2014
	double IEEE1547_under_freq_low_band_viol_time;	//UF1 violation accumulator

	//1547 voltage(a) voltage
	double IEEE1547_under_voltage_lowest_voltage_setpoint;	//Lowest voltage threshold for undervoltage
	double IEEE1547_under_voltage_middle_voltage_setpoint;	//Middle-lowest voltage threshold for undervoltage
	double IEEE1547_under_voltage_high_voltage_setpoint;	//High value of low voltage threshold for undervoltage
	double IEEE1547_over_voltage_low_setpoint;				//Lowest voltage value for overvoltage
	double IEEE1547_over_voltage_high_setpoint;				//High voltage value for overvoltage
	double IEEE1547_under_voltage_lowest_delay;				//Lowest voltage clearing time for undervoltage
	double IEEE1547_under_voltage_middle_delay;				//Middle-lowest voltage clearing time for undervoltage
	double IEEE1547_under_voltage_high_delay;				//Highest voltage clearing time for undervoltage
	double IEEE1547_over_voltage_low_delay;					//Lowest voltage clearing time for overvoltage
	double IEEE1547_over_voltage_high_delay;				//Highest voltage clearing time for overvoltage
	double IEEE1547_under_voltage_lowest_viol_time;			//Lowest low voltage threshold violation accumulator
	double IEEE1547_under_voltage_middle_viol_time;			//Middle low voltage threshold violation accumulator
	double IEEE1547_under_voltage_high_viol_time;			//Highest low voltage threshold violation accumulator
	double IEEE1547_over_voltage_low_viol_time;				//Lowest high voltage threshold violation accumulator
	double IEEE1547_over_voltage_high_viol_time;			//Highest high voltage threshold violation accumulator

	enum IEEE1547TRIPSTATUS {
		IEEE_1547_NOTRIP=0,		/**< No trip reason */
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

	double IEEE1547_out_of_violation_time_total;	//Tracking variable to see how long we've been "outside of bad conditions" to re-enable the inverter
	gld_property *pFrequency;			//Pointer to frequency value for checking 1547 compliance
	double value_Frequency;				//Value storage for current frequency value
	double ieee_1547_delta_return;			//Deltamode tracker - made global for "off-cycle" checks
	double prev_time_dbl_IEEE1547;		//Time tracker for IEEE 1547 update checks

	STATUS initalize_IEEE_1547_checks(void);
	double perform_1547_checks(double timestepvalue);

public:
	set phases;				 /**< device phases (see PHASE codes) */
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
		DYNAMIC_DC_BUS = 1
	};
	enumeration grid_forming_mode; //

	enum P_F_DROOP_SETTING_TYPE
	{
		FSET_MODE = 0,
		PSET_MODE = 1
	};
	enumeration P_f_droop_setting_mode; //


	gld::complex terminal_current_val[3];
	gld::complex terminal_current_val_pu[3];
	TIMESTAMP inverter_start_time;
	bool inverter_first_step;
	bool first_deltamode_init;
	int64 first_iteration_current_injection; //Initialization variable - mostly so SWING_PQ buses initalize properly for deltamode

	double GridForming_freq_convergence_criterion;
	double GridForming_volt_convergence_criterion;
	double GridFollowing_curr_convergence_criterion;

	INV_DYN_STATE curr_state; ///< The current state of the inverter in deltamode


	gld::complex I_out_PU_temp[3];  //This is mainly used for current limiting function of a grid-forming inverter

	gld::complex power_val[3];		   //power
	gld::complex VA_Out;				   // complex output power
	gld::complex value_Circuit_V_PS;	   // Positive sequence voltage of three phase terminal voltages
	gld::complex value_Circuit_I_PS[3]; // Positive sequence current of three phase terminal currents, assume no negative or zero sequence, Phase A equals to positive sequence

	double node_nominal_voltage; // Nominal voltage

	double E_mag; //internal voltage magnitude, used for grid-forming control
	double mdc;	  // only used when dc bus dynamic is enabled, make sure that the modulation index is enough

	bool frequency_watt; // Boolean value indicating whether the f/p droop curve is included in the inverter or not
	bool checkRampRate_real; // check the active power ramp rate
	bool volt_var;		 // Boolean value indicating whether the volt-var droop curve is included in the inverter or not
	bool checkRampRate_reactive; // check the reactive power ramp rate

	double rampUpRate_real; // unit: pu/s
	double rampDownRate_real; // unit: pu/s
	double rampUpRate_reactive; // unit: pu/s
	double rampDownRate_reactive; // unit: pu/s
	double Pref_droop_pu_prev; // The value of Pref in last simulation step, note it is only defined in the predictor pass
	double Qref_droop_pu_prev; // The value of Qref in last simulation step, note it is only defined in the predictor pass

	// voltages and currents in dq frame, used for grid-following control
	double ugd_pu[3];
	double ugq_pu[3];
	double igd_pu[3];
	double igq_pu[3];
	double igd_ref[3];
	double igq_ref[3];
	double igd_ref_max;  //Upper limit for igd_ref
	double igd_ref_min;  //Lower limit for igd_ref
	double igq_ref_max;  //Upper limit for igq_ref
	double igq_ref_min;  //Lower limit for igq_ref

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
	double delta_w_Pmax;  // output of the Pmax controller
	double delta_w_Pmin;  //
	double delta_V_Qmax; // output of the Qmax controller
	double delta_V_Qmin;
	double Pset;		  // power set point in P-f droop
	double mp;			  // P-f droop gain, usually 3.77 rad/s/pu
	double P_f_droop;     // p-f droop gain, per unit, usually 0.01
	double kppmax;		  // proportional and integral gains for Pmax and Pmin controllers
	double kipmax;
	double kpqmax;        // proportional and integral gains for Qmax and Qmin controllers
	double kiqmax;
	double w_lim; // w_lim is the saturation limit
	double V_lim; // the saturation limit of the Qmax and Qmin controllers
	double Pmax;  // Pmax and Pmin are the maximum limit and minimum limit of Pmax controller and Pmin controller
	double Pmin;
	double Qmax;  // Qmax and Qmin are the limits for the Qmax and Qmin controllers
	double Qmin;
	double Imax;  // The maximum output current of a grid-forming inverter
	double w_ref;		 // w_ref is the rated frequency, usually 376.99 rad/s
	double f_nominal;	 // rated frequency, 60 Hz
	double Angle[3];	 // Phase angle of the internal voltage
	gld::complex e_source[3]; // e_source[i] is the complex value of internal voltage
	gld::complex e_source_pu[3]; // e_source[i] is the complex per-unit value of internal voltage
	gld::complex e_droop[3]; // e_droop is the complex value of the inverter internal voltage given by the grid-forming droop control
	gld::complex e_droop_pu[3]; // e_droop is the complex value of the inverter internal voltage given by the grid-forming droop control
	double e_source_Re[3];
	double e_source_Im[3];
	gld::complex I_source[3]; // I_source[i] is the complex value when using current source representation
	double I_source_Re[3];
	double I_source_Im[3];
	double pCircuit_V_Avg_pu; //pCircuit_V_Avg_pu refers to the average value of three phase voltages, it is per-unit value
	double Xfilter;			  // Rfilter and Xfilter are the per-unit values of inverter filter, they are per-unit values
	double Rfilter;
	double freq; // freq is the frequency obtained from the P-f droop controller
	double fset; // frequency set point is for isochronous mode of grid-forming inverters

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
	double db_UF;         // dead band for frequency-watt, UF for under-frequency
	double db_OF;         // dead band for frequency-watt, OF for over-frequency
	double Rq;			  // Q-V droop gain in volt-var
	double db_UV;         // dead band for volt-var, UV for under-voltage
	double db_OV;         // dead band for volt-var, OV for over-voltage

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
	int isa(char *classname);

	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	STATUS pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass);
	STATUS updateCurrInjection(int64 iteration_count,bool *converged_failure);
	STATUS init_dynamics(INV_DYN_STATE *curr_time);
	STATUS DC_object_register(OBJECT *DC_object);

public:
	static CLASS *oclass;
	static inverter_dyn *defaults;
	static CLASS *plcass;
};

#endif // GLD_GENERATORS_INVERTER_DYN_H_
