// $Id: power_metrics.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2009 Battelle Memorial Institute

#ifndef _POWER_METRICS_H
#define _POWER_METRICS_H

#include "powerflow.h"
#include "powerflow_library.h"
#include "fault_check.h"

class power_metrics : public powerflow_library
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:

	power_metrics(MODULE *mod);
	inline power_metrics(CLASS *cl=oclass):powerflow_library(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);
	void calc_SAIFI(void);
	void calc_SAIDI(void);
	void calc_CAIDI(void);
	void calc_ASAI(void);
	void calc_MAIFI(void);
	double SAIFI;
	double SAIDI;
	double CAIDI;
	double ASAI;
	double MAIFI;
	double SAIFI_int;
	double SAIDI_int;
	double CAIDI_int;
	double ASAI_int;
	double MAIFI_int;
	double Extra_PF_Data;
	void perform_rel_calcs(int number_int, int number_int_secondary, int total_cust, TIMESTAMP rest_time_val, TIMESTAMP base_time_val);
	void reset_metrics_variables(bool annual_metrics);
	void check_fault_check(void);
	int num_cust_interrupted;
	int num_cust_momentary_interrupted;
	int num_cust_total;
	TIMESTAMP outage_length;
	double outage_length_normalized;
	double stat_base_time_value;
	OBJECT *rel_metrics;	//Link to reliability metrics object
private:
	double SAIFI_num;
	double SAIDI_num;
	double ASAI_num;
	double MAIFI_num;
	double SAIFI_num_int;
	double SAIDI_num_int;
	double ASAI_num_int;
	double MAIFI_num_int;
	fault_check *fault_check_object_lnk;	//Link to fault_check object - so it only has to be mapped once
};

EXPORT int calc_pfmetrics(OBJECT *callobj, OBJECT *calcobj, int number_int, int number_int_secondary, int total_customers, TIMESTAMP rest_time_val, TIMESTAMP base_time_val);
EXPORT int reset_pfinterval_metrics(OBJECT *callobj, OBJECT *calcobj);
EXPORT int reset_pfannual_metrics(OBJECT *callobj, OBJECT *calcobj);
EXPORT void *init_pf_reliability_extra(OBJECT *myhdr, OBJECT *callhdr);
EXPORT int logfile_extra(OBJECT *myhdr, char *BufferArray);

#endif // _POWER_METRICS_H
