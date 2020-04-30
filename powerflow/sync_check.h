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

private:
	bool deltamode_inclusive; //Boolean for deltamode calls - pulled from object flags
	bool reg_dm_flag;

private: /* Published Variables*/
	bool arm_sync;

	double frequency_tolerance_pu;
	double voltage_tolerance_pu;
	double metrics_period_sec;

private: /* Init Funcs */
	bool data_sanity_check(OBJECT *par = NULL);
	void reg_deltamode();
	void init_sensors();

private: /* Funcs for Deltamode */
	void get_measurements();
};

#endif // GLD_POWERFLOW_SYNC_CHECK_H_
