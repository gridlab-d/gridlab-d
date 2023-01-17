#ifndef GLD_GENERATORS_IBR_SKELETON_H_
#define GLD_GENERATORS_IBR_SKELETON_H_

#include <vector>

#include "cblock.h"

#include "generators.h"

EXPORT int isa_ibr_skeleton(OBJECT *obj, char *classname);
EXPORT STATUS preupdate_ibr_skeleton(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time);
EXPORT SIMULATIONMODE interupdate_ibr_skeleton(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS postupdate_ibr_skeleton(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass);
EXPORT STATUS ibr_skeleton_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure);
EXPORT STATUS ibr_skeleton_DC_object_register(OBJECT *this_obj, OBJECT *DC_obj);

//Alias the currents
#define phaseA_I_Out terminal_current_val[0]
#define phaseB_I_Out terminal_current_val[1]
#define phaseC_I_Out terminal_current_val[2]

typedef struct
{
	FUNCTIONADDR fxn_address;
	OBJECT *dc_object;
} DC_OBJ_FXNS_IBR;

//ibr_skeleton class
class ibr_skeleton : public gld_object
{
private:
	bool deltamode_inclusive; //Boolean for deltamode calls - pulled from object flags
	bool first_sync_delta_enabled;
	char first_iter_counter;
	bool deltamode_exit_iteration_met;

	double prev_timestamp_dbl;
	double last_QSTS_GF_Update;

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
	std::vector<DC_OBJ_FXNS_IBR> dc_interface_objects;
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


	gld_property *pFrequency;			//Pointer to frequency value for checking 1547 compliance
	double value_Frequency;				//Value storage for current frequency value
	double ieee_1547_delta_return;			//Deltamode tracker - made global for "off-cycle" checks
	double prev_time_dbl_IEEE1547;		//Time tracker for IEEE 1547 update checks

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

	gld::complex I_out_PU_temp[3];  //This is mainly used for current limiting function of a grid-forming inverter

	gld::complex power_val[3];		   //power
	gld::complex VA_Out;				   // complex output power
	gld::complex value_Circuit_V_PS;	   // Positive sequence voltage of three phase terminal voltages
	gld::complex value_Circuit_I_PS[3]; // Positive sequence current of three phase terminal currents, assume no negative or zero sequence, Phase A equals to positive sequence

	double node_nominal_voltage; // Nominal voltage

	double mdc;	  // only used when dc bus dynamic is enabled, make sure that the modulation index is enough

	double Pref_droop_pu_prev; // The value of Pref in last simulation step, note it is only defined in the predictor pass
	double Qref_droop_pu_prev; // The value of Qref in last simulation step, note it is only defined in the predictor pass

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

	double Tif;			  // Tif is the low pass filter when using current source representation
	double Vset0;		  // Vset0 is the voltage set point in volt-var in grid-following
	double Pref_droop_pu; // Power reference in frequency-watt
	double Pref_max;	  // Pref_max and Pref_min are the upper and lower limits of power references
	double Pref_min;	  //
	double V_Avg_pu;	  // average voltage in volt-var
	double Qref_droop_pu; // Q reference in volt-var
	double Qref_max;	  // Qref_max and Qref_min are the upper and lower limits of Q references
	double Qref_min;	  //

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
	ibr_skeleton(MODULE *module);
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
	STATUS init_dynamics();
	STATUS DC_object_register(OBJECT *DC_object);

public:
	static CLASS *oclass;
	static ibr_skeleton *defaults;
	static CLASS *plcass;
};

#endif // GLD_GENERATORS_IBR_SKELETON_H_
