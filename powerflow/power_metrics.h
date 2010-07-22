// $Id: power_metrics.h 2010-06-14 15:00:00Z d3x593 $
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
	double calc_SAIFI(void);
	double calc_SAIDI(void);
	double calc_CAIDI(void);
	double calc_ASAI(void);
	double SAIFI;
	double SAIDI;
	double CAIDI;
	double ASAI;
	void perform_rel_calcs(int number_int, int total_cust, TIMESTAMP rest_time_val, TIMESTAMP base_time_val);
	void reset_metrics_variables(bool annual_metrics);
	void check_fault_check(void);
	int num_cust_interrupted;
	int num_cust_total;
	TIMESTAMP outage_length;
	double outage_length_normalized;
private:
	double SAIFI_num;
	double SAIDI_num;
	double ASAI_num;
	fault_check *fault_check_object_lnk;	//Link to fault_check object - so it only has to be mapped once
};

EXPORT int calc_pfmetrics(OBJECT *callobj, OBJECT *calcobj, int number_int, int total_customers, TIMESTAMP rest_time_val, TIMESTAMP base_time_val);
EXPORT int reset_pfinterval_metrics(OBJECT *callobj, OBJECT *calcobj);
EXPORT int reset_pfannual_metrics(OBJECT *callobj, OBJECT *calcobj);

#endif // _POWER_METRICS_H
