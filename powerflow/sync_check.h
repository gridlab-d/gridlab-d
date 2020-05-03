// $Id: sync_check.h
//	Copyright (C) 2020 Battelle Memorial Institute

#ifndef GLD_POWERFLOW_SYNC_CHECK_H_
#define GLD_POWERFLOW_SYNC_CHECK_H_

#include "powerflow.h"

EXPORT SIMULATIONMODE interupdate_sync_check(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

class sync_check : public powerflow_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;

public:
	sync_check(MODULE *mod);
	sync_check(CLASS *cl = oclass) : powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent = NULL);
	int isa(char *classname);

	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);

	SIMULATIONMODE inter_deltaupdate_sync_check(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

private: /* Flags & Buffer */
	bool deltamode_inclusive; //Boolean for deltamode calls - pulled from object flags
	bool reg_dm_flag;

	bool metrics_flag;
	double t_sat;

	gld_property *temp_property_pointer;

private: /* Measurements, actuator, gld objs, & Nominal Values*/
	double freq_norm;
	double volt_norm;

	gld_object *swt_fm_node;
	gld_object *swt_to_node;

	double swt_fm_node_freq;
	double swt_to_node_freq;

	complex swt_fm_volt_A;
	complex swt_to_volt_A;

	gld_property *swt_prop_status;

private: /* Published Variables*/
	bool sc_enabled_flag;

	double frequency_tolerance_pu;
	double voltage_tolerance_pu;
	double metrics_period_sec;

private: /* Init Funcs */
	bool data_sanity_check(OBJECT *par = NULL);
	void reg_deltamode_check();
	void reg_deltamode();
	void init_norm_values(OBJECT *par = NULL);

private: /* Funcs for Deltamode */
	void update_measurements();
	void check_metrics();
	void check_excitation(unsigned long);
};

// Utility Funcs
// TODO: make a func for the get property process

#endif // GLD_POWERFLOW_SYNC_CHECK_H_
