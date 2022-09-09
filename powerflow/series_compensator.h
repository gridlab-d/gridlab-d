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

	double turns_ratio[3];
	double vset_value[3];  //voltage reference
	double vset_value_0[3]; //voltage set points changed by the player
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

    bool frequency_open_loop_control; // Boolean value indicating whether the frequency bang-bang control of the series compensator is enabled or not;

    bool t_delay_low_flag;   //status: frequency measurement, under-frequency event
    bool t_hold_low_flag;    //status: frequency controlled enabled, hold, under-frequency event
    bool t_recover_low_flag; //status: voltage gradually goes back to normal, under-frequency event
    bool t_delay_high_flag;  //status: frequency measurement, over-frequency event
    bool t_hold_high_flag;   //status: frequency controlled enabled, hold, over-frequency event
    bool t_recover_high_flag;//status: voltage gradually goes back to normal, over-frequency event


    double t_count_low_delay; // counter during under-frequency event, frequency measurement
    double t_count_low_hold; //  counter during under-frequency event, hold
    double t_count_high_delay; // counter during over-frequency event, frequency measurement
    double t_count_high_hold; //  counter during over-frequency event, hold

    double t_delay;  // Time delay for frequency measurement
    double t_hold;   // Time that the regulator stays at the new voltage set point
    double recover_rate; // The rate the voltage set point recovers to nominal voltage set point. unit pu/s.
    double frequency_low; // the low frequency set point to enable frequency control
    double frequency_high; //the high frequency set point to enable frequency control
    double V_error; //



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
	gld::complex val_ToNode_voltage[3];
	gld::complex val_FromNode_voltage[3];
	double val_ToNode_voltage_pu[3];
	double val_FromNode_voltage_pu[3];
	double val_FromNode_nominal_voltage;
	double val_FromNode_frequency; // measured frequency
	
	SERIES_STATE pred_state;	///< The predictor state of the inverter in deltamode
	SERIES_STATE next_state;	///< The next state of the inverter in deltamode
	SERIES_STATE curr_state;  ///< The current state of the inverter in deltamode

	int deltamode_return_val;	///< Value for deltamode iterations to return when not iterating

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
