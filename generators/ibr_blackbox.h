#ifndef GLD_GENERATORS_IBR_BLACKBOX_H_
#define GLD_GENERATORS_IBR_BLACKBOX_H_

#include <vector>

#include "cblock.h"

#include "generators.h"

#ifdef HAVE_FRUGALLY
#include "fdeep/fdeep.hpp"
#endif

EXPORT int isa_ibr_blackbox(OBJECT *obj, char *classname);
EXPORT STATUS preupdate_ibr_blackbox(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time);
EXPORT SIMULATIONMODE interupdate_ibr_blackbox(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
// EXPORT STATUS postupdate_ibr_blackbox(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass);
EXPORT STATUS ibr_blackbox_NR_current_injection_update(OBJECT *obj, int64 iteration_count, bool *converged_failure);

//ibr_blackbox class
class ibr_blackbox : public gld_object
{
private:
	bool deltamode_inclusive; //Boolean for deltamode calls - pulled from object flags
	bool first_sync_delta_enabled;
	char first_iter_counter;
	bool deltamode_exit_iteration_met;

	double prev_timestamp_dbl;
	double last_QSTS_GF_Update;

	gld_property *pIGenerated[3];		//Link to direct current injections to powerflow at bus-level

	gld::complex value_IGenerated[3];		//Value/accumulator for IGenerated values
	gld::complex prev_value_IGenerated[3];	//Tracking variable for grid following "QSTS exit" and delta injections

	bool parent_is_a_meter;		 //Boolean to indicate if the parent object is a meter/triplex_meter
	bool parent_is_single_phase; //Boolean to indicate if the parent object is single-phased (main or triplexed)
	bool parent_is_triplex;		//Boolean to indicate if the parent object was a triplex device - minimal usage
	enumeration attached_bus_type;	//Determines attached bus type - mostly for VSI and grid-forming functionality

	FUNCTIONADDR swing_test_fxn;	//Function to map to swing testing function, if needed

	gld_property *pCircuit_V[3];   ///< pointer to the three L-N voltage fields
	gld_property *pMeterStatus;	   ///< Pointer to service_status variable on meter parent

	//Default or "connecting point" values for powerflow interactions
	gld::complex value_Circuit_V[3];	   ///< value holder for the three L-N voltage fields
	enumeration value_MeterStatus; ///< value holder for service_status variable on meter parent

	SIMULATIONMODE desired_simulation_mode; //deltamode desired simulation mode after corrector pass - prevents starting iterations again

	//Map functions
	gld_property *map_complex_value(OBJECT *obj, const char *name);
	gld_property *map_double_value(OBJECT *obj, const char *name);
	void pull_complex_powerflow_values(void);
	void push_complex_powerflow_values(bool update_voltage);

	//Frugally deep model adds
	std::unique_ptr<fdeep::model> bb_model = {nullptr};
	fdeep::tensors result_output;
	std::vector<std::vector<std::vector<float>>> simulation_data;

public:
	set phases;				 /**< device phases (see PHASE codes) */
	gld::complex terminal_current_val[3];
	gld::complex terminal_current_val_pu[3];
	double dc_input_voltage;
	double dc_output_current;
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

	double Pref;	// Pref and Qref are the refrences for P and Q
	double Qref;

	double Vset0;		  // Vset0 is the voltage set point in volt-var in grid-following

	/* required implementations */
	ibr_blackbox(MODULE *module);
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

public:
	static CLASS *oclass;
	static ibr_blackbox *defaults;
	static CLASS *plcass;
};

#endif // GLD_GENERATORS_IBR_BLACKBOX_H_
