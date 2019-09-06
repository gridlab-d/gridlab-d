// $Id: series_compensator.h 2018-12-06 $
//	Copyright (C) 2018 Battelle Memorial Institute

#ifndef _SERIES_COMPENSATOR_H
#define _SERIES_COMPENSATOR_H

#include "powerflow.h"
#include "link.h"

EXPORT SIMULATIONMODE interupdate_series_compensator(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

//Alternative PI version Dynamic control of series compensator state variable structure
typedef struct {


// state variables of controller
    double dn_ini_StateVal[3]; //
    double n_ini_StateVal[3]; // output of integrator
    double n_StateVal[3];   //output of PI controller

} SERIES_STATE;


class series_compensator : public link_object
{
public:
	double series_compensator_resistance;

	TIMESTAMP series_compensator_start_time;
	bool series_compensator_first_step;

	double voltage_deadband; // used for initialization of the series compensator

	double turns_ratio[3];
	double vset_value[3];  //voltage reference
	double voltage_iter_tolerance;
	double n_max[3]; //maximum turn ratio
	double n_min[3]; //minimum turn ratio
	double n_max_ext[3]; //maximum turn ratio, defined by user
	double n_min_ext[3]; //minimum turn ratio, defined by user
	double kp;   // propotional gain
	double ki;  // integrator gain
	double V_bypass_max_pu; //the upper limit voltage to bypass compensator
	double V_bypass_min_pu; // the lower limit voltage to bypass compensator

	double kpf; //proportional gain for frequency regulation
	double delta_f; //frequency deviation
	double f_db_max; // frequency dead band max
	double f_db_min; // frequency dead band min
	double delta_Vmax; //upper limit of the frequency regulation output
	double delta_Vmin; //lower limit of the frequency regulation output
	double delta_V; // frequency regulation output

	bool frequency_regulation;   // Boolean value indicating whether the frequency regulation of the series compensator is enabled or not;

	typedef enum {ST_NORMAL=0, ST_BYPASS=1} PHASE_COMP_STATE;
	enumeration phase_states[3];

	//Deltamode function
	SIMULATIONMODE inter_deltaupdate_series_compensator(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
	void sercom_prePre_fxn(void);	//Functionalized "presync before link::presync" calls
	void sercom_postPre_fxn(void);	//Functionalized "presync after link::presync" calls
	int sercom_postPost_fxn(unsigned char pass_value, double deltat);	//Functionalized "postsync after link::postsync" calls

private:
	gld_property *ToNode_voltage[3];	//Pointer for API to map to voltages to read
	gld_property *FromNode_voltage[3];
	gld_property *FromNode_frequency;
	double prev_turns_ratio[3];
	complex val_ToNode_voltage[3];
	complex val_FromNode_voltage[3];
	double val_ToNode_voltage_pu[3];
	double val_FromNode_voltage_pu[3];
	double val_FromNode_nominal_voltage;
	double val_FromNode_frequency; // measured frequency
	
	SERIES_STATE pred_state;	///< The predictor state of the inverter in deltamode
	SERIES_STATE next_state;	///< The next state of the inverter in deltamode
	SERIES_STATE curr_state;  ///< The current state of the inverter in deltamode

public:
	static CLASS *oclass;
	static CLASS *pclass;
	
public:
	series_compensator(MODULE *mod);
	inline series_compensator(CLASS *cl=oclass):link_object(cl){};
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	int isa(char *classname);
};

#endif // _SERIES_COMPENSATOR_H
