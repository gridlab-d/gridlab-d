/** $Id: metrics.h 1182 2008-12-22 22:08:36Z dchassin $
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

//#define CLEARBUF(BUF,N) ((char*)(memset((BUF),0,((N)>>3)+1)))
//#define MAKEBUF(N) CLEARBUF((char*)(malloc(((N)>>3)+1)),N)
//#define BITSET(BUF,N) (((BUF)[(N)>>3])|=(1<<((N)&7)))
//#define BITCLR(BUF,N) (((BUF)[(N)>>3])&=(~(1<<((N)&7))))
//#define COUNTBITS(BUF,N) (0 /* TODO */)
//
//typedef struct s_event {
//	TIMESTAMP event_time;
//	//IEEE 1366-2003 data collected
//	int32 Ni;	// number of interrupted customers for each sustaind interruption
//	int32 Nmi;	// number of interrupted customers for each momentary interruption
//	double ri;	// restoration time for each interruption event
//	struct s_event *next;
//} EVENT;
//
//typedef struct s_totals {
//	int16 period;
//	int16 Nevents;	// number of event that occurred during the reporting period
//	int16 IMe;		// number of momentary interruption events
//	int32 CI;		// customers interrupted = Sum[Ni]
//	double CMI;		// customer minutes interrupted = Sum[ri*Ni]
//	int32 CN;		// customers who experienced at least one sustained interruption
//	int32 CNT;		// total number of customers who have experienced more than n sustained and momentary interruptions
//	int32 CNK;		// total number of customers who have experienced more than n sustained interruptions
//	double CMIE;	// total number of customer momentary interruption events = Sum[IMe*Nmi]
//	double IMi;		// total number of customer momentary interruptions
//	double LT;		// total connected kVA load served
//	double Li;		// connection kVA load interrupted for each interruption event
//	double LDI;		// load minutes interrupted
//} TOTALS;

typedef struct s_indices {
	char256 MetricName;	//Name of metric from metric_of_interest
	double *MetricLoc;	//Pointer to where this metric is calculated
} INDEXARRAY;

typedef struct s_custarray {
	OBJECT *CustomerObj;	//Object pointer to the customer
	bool *CustInterrupted;	//Pointer to customer "interrupted" flag
} CUSTARRAY;

class metrics {
//declaring the reliability indexes
public:
private:
	//EVENT *eventlist;
	//FINDLIST *customers;
	//FINDLIST *interrupted;
	//FILE *fp;
	//TOTALS totals;
	//bool first_year;
	
	int num_indices;			//Number of indices we are finding
	INDEXARRAY *CalcIndices;	//Storage/informational array for metrics of interest
	TIMESTAMP metric_interval;	//TIMESTAMP version of metric_interval
	TIMESTAMP next_metric_interval;	//Tracking variable used to determine when to do a "line dump" of variables
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
public:
	char report_file[1024];	// the file to which the report is written
public:
	// required implementations
	metrics(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	char1024 customer_group;
	OBJECT *module_metrics_obj;
	char1024 metrics_oi;
	double metric_interval_dbl;
	void event_ended(OBJECT *event_obj,OBJECT *fault_obj,TIMESTAMP event_start_time,TIMESTAMP event_end_time,char *fault_type,int number_customers_int);
	int get_interrupted_count(void);
public:
	static CLASS *oclass;
	static metrics *defaults;
public:
	//inline EVENT *get_event() { return eventlist;};
	//inline void add_event(EVENT *p) { p->next=eventlist; eventlist=p;};
	//inline void del_event() {if (eventlist) eventlist=eventlist->next;};
	//OBJECT *start_event(EVENT *pEvent, FINDLIST *objectlist,const char *targets,char *event_values,char *old_values, int size);
	//void end_event(OBJECT* obj, const char *targets, char *old_values);
};

#endif

/**@}*/
