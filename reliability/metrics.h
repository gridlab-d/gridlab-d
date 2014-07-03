/** $Id: metrics.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file metrics.h
	@addtogroup metrics
	@ingroup MODULENAME

 @{
 **/

#ifndef _metrics_H
#define _metrics_H

#include <stdarg.h>
#include "gridlabd.h"

typedef struct s_indices {
	char256 MetricName;			//Name of metric from metric_of_interest
	double *MetricLoc;			//Pointer to where this metric is calculated
	double *MetricLocInterval;	//Pointer to where the interval metric is calculated (if it has one)
} INDEXARRAY;

typedef struct s_custarray {
	OBJECT *CustomerObj;	//Object pointer to the customer
	bool *CustInterrupted;	//Pointer to customer "interrupted" flag
	bool *CustInterrupted_Secondary;	//Pointer to secondary customer "interrupted" flag - may or may not be used
} CUSTARRAY;

class metrics : public gld_object {
//declaring the reliability indexes
public:
private:
	int num_indices;			//Number of indices we are finding
	INDEXARRAY *CalcIndices;	//Storage/informational array for metrics of interest
	TIMESTAMP metric_interval;	//TIMESTAMP version of metric_interval
	TIMESTAMP report_interval;	//TIMESTAMP version of report_interval
	TIMESTAMP next_metric_interval;	//Tracking variable used to determine when to do an interval dump of variables
	TIMESTAMP next_report_interval;	//Tracking variable used to determine when to do next "line dump" of variables
	TIMESTAMP next_annual_interval;	//Tracking variable used to determine when next year is (assumes 8760 model)
	int metric_interval_event_count;	//Event count in the current "metric" interval
	int annual_interval_event_count;	//Even count in the current "annual" interval
	bool metric_equal_annual;			//Flag to see if annual and "metric interval" are the same length
	int CustomerCount;		//Number of candidate objects (customers) found
	CUSTARRAY *Customers;	//Array of candidate objects (customers)
	FUNCTIONADDR reset_interval_func;	//Pointer to metric "interval" reset
	FUNCTIONADDR reset_annual_func;		//Pointer to metric annual reset
	FUNCTIONADDR compute_metrics;		//Pointer to metric computation function

	TIMESTAMP curr_time;	//Time tracking variable
	
	double *get_metric(OBJECT *obj, char *name);	//Function to extract address of double value (metric)
	bool *get_outage_flag(OBJECT *obj, char *name);	//Function to extract address of outage flag
public:
	static bool report_event_log;

	char report_file[1024];	// the file to which the report is written
	bool secondary_interruptions_count;	//Flag to get secondary interruptions count - if supported.  If one "customer" supports it, they all better

	// required implementations
	metrics(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	char1024 customer_group;
	OBJECT *module_metrics_obj;
	char1024 metrics_oi;
	double metric_interval_dbl;
	double report_interval_dbl;
	void *Extra_Data;		//Pointer to extra data array - if needed
	void event_ended(OBJECT *event_obj,OBJECT *fault_obj,OBJECT *faulting_obj,TIMESTAMP event_start_time,TIMESTAMP event_end_time,char *fault_type,char *impl_fault,int number_customers_int);
	void event_ended_sec(OBJECT *event_obj,OBJECT *fault_obj,OBJECT *faulting_obj,TIMESTAMP event_start_time,TIMESTAMP event_end_time,char *fault_type,char *impl_fault,int number_customers_int, int number_customers_int_secondary);
	int get_interrupted_count(void);
	void get_interrupted_count_secondary(int *in_outage, int *in_outage_secondary);
	void write_metrics(void);

	static CLASS *oclass;
	static metrics *defaults;
};

#endif

/**@}*/
