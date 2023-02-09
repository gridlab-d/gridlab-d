#ifndef GLD_GENERATORS_IBR_SKELETON_H_
#define GLD_GENERATORS_IBR_SKELETON_H_

#include <vector>

#include "cblock.h"

#include "generators.h"

EXPORT int isa_ibr_skeleton(OBJECT *obj, char *classname);
EXPORT STATUS preupdate_ibr_skeleton(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time);
EXPORT SIMULATIONMODE interupdate_ibr_skeleton(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
// EXPORT STATUS postupdate_ibr_skeleton(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass);
EXPORT STATUS ibr_skeleton_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure);
// EXPORT STATUS ibr_skeleton_DC_object_register(OBJECT *this_obj, OBJECT *DC_obj);

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

	//Convergence check item for grid-forming voltage
	gld::complex e_droop_prev[3];

	//Map functions
	gld_property *map_complex_value(OBJECT *obj, const char *name);
	gld_property *map_double_value(OBJECT *obj, const char *name);
	void pull_complex_powerflow_values(void);
	void reset_complex_powerflow_accumulators(void);
	void push_complex_powerflow_values(bool update_voltage);

public:
	set phases;				 /**< device phases (see PHASE codes) */
	gld::complex terminal_current_val[3];
	gld::complex terminal_current_val_pu[3];
	TIMESTAMP inverter_start_time;
	bool inverter_first_step;
	bool first_deltamode_init;
	int64 first_iteration_current_injection; //Initialization variable - mostly so SWING_PQ buses initalize properly for deltamode

	gld::complex I_out_PU_temp[3];  //This is mainly used for current limiting function of a grid-forming inverter

	gld::complex power_val[3];		   //power
	gld::complex VA_Out;				   // complex output power
	gld::complex value_Circuit_V_PS;	   // Positive sequence voltage of three phase terminal voltages
	gld::complex value_Circuit_I_PS[3]; // Positive sequence current of three phase terminal currents, assume no negative or zero sequence, Phase A equals to positive sequence

	double node_nominal_voltage; // Nominal voltage

	double S_base;	 // S_base is the rated caspacity
	double V_base;	 // Vbase is the rated Line to ground voltage
	double Z_base;	 // Zbase is the reated impedance
	double I_base;	 // Ibase is the rated current
	double P_out_pu; // P_out_pu is the per unit value of VA_OUT.Re()
	double Q_out_pu; // Q_out_pu is the per-unit value of VA_Out.Im()

	double f_nominal;	 // rated frequency, 60 Hz

	double pCircuit_V_Avg_pu; //pCircuit_V_Avg_pu refers to the average value of three phase voltages, it is per-unit value
	double Xfilter;			  // Rfilter and Xfilter are the per-unit values of inverter filter, they are per-unit values
	double Rfilter;
        gld::complex filter_admittance;

	double Pref;	// Pref and Qref are the refrences for P and Q
	double Qref;

	double Vset0;		  // Vset0 is the voltage set point in volt-var in grid-following

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
	// STATUS post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass);
	STATUS updateCurrInjection(int64 iteration_count,bool *converged_failure);
	STATUS init_dynamics();
	// STATUS DC_object_register(OBJECT *DC_object);

public:
	static CLASS *oclass;
	static ibr_skeleton *defaults;
	static CLASS *plcass;
};

#endif // GLD_GENERATORS_IBR_SKELETON_H_
