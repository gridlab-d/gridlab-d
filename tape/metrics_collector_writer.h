/*
 * metrics_collector_writer.h
 *
 *  Created on: Jan 30, 2017
 *      Author: tang526
 */

#ifndef _METRICS_COLLECTOR_WRITER_H_
#define _METRICS_COLLECTOR_WRITER_H_

#include "tape.h"
#include "metrics_collector.h"
//#include <json/json.h> //jsoncpp library

#include <iostream>
#include <fstream>
#include <cstring>
#include <string.h>
using namespace std;

EXPORT void new_metrics_collector_writer(MODULE *);

#ifdef __cplusplus

class metrics_collector_writer{
public:
	static metrics_collector_writer *defaults;
	static CLASS *oclass, *pclass;

	metrics_collector_writer(MODULE *);
	int create();
	int init(OBJECT *);
	int isa(char *);
	TIMESTAMP postsync(TIMESTAMP, TIMESTAMP);

	int commit(TIMESTAMP);

public:
	char256 filename;
	double interval_length_dbl;			//Metrics output interval length
	TIMESTAMP next_time;
	TIMESTAMP dt;
	TIMESTAMP last_t;

private:

	int write_line(TIMESTAMP);

private:
	Json::Value metrics_writer_Output;	// Final output dictionary
	Json::Value metrics_writer_metrics;	// Final output dictionary

	Json::Value metrics_writer_billing_meters;	// Final output dictionary for triplex_meters and non-swing meters
	Json::Value metrics_writer_houses;	// Final output dictionary for houses
	Json::Value metrics_writer_inverters;	// Final output dictionary for inverters
	Json::Value metrics_writer_feeder_information;	// Final output dictionary for feeder_information

	Json::Value ary_billing_meters;  // array storage for billing meter metrics
	Json::Value ary_houses;          // array storage for house (and water heater) metrics
	Json::Value ary_inverters;       // array storage for inverter metrics
	Json::Value ary_feeders;         // array storage for feeder information metrics

	char256 filename_billing_meter;
	char256 filename_inverter;
	char256 filename_house;
	char256 filename_substation;

	FINDLIST *items;
	PROPERTY *prop_ptr;
	int obj_count;
	int write_count;
	TIMESTAMP startTime;
	TIMESTAMP final_write;
	TIMESTAMP next_write;
	TIMESTAMP last_write;
	TIMESTAMP last_flush;
	TIMESTAMP write_interval;
	TIMESTAMP flush_interval;
	int32 write_ct;
	TAPESTATUS tape_status; // TS_INIT/OPEN/DONE/ERROR
	char *prev_line_buffer;
	char *line_buffer;
	size_t line_size;
	bool interval_write;

	char* parent_string;

	FINDLIST *metrics_collectors;

	// Parameters related to triplex_meter and billing meter objects
	double *real_power_array;		//array storing real power measured at the meter or triplex_meter
	double *reactive_power_array;		//array storing reactive power measured at the meter or triplex_meter
	double *voltage_mag_array;		//array storing voltage12 or vLL measured at the meter or triplex_meter
	double *voltage_average_mag_array;		//array storing voltage12/2 or vLN measured at the meter or triplex_meter
	double *voltage_unbalance_array;		//array storing (voltage[0]-voltage[1])/(voltage12/2) measured at the triplex_meter, or (vmax - vmin)/vavg at the primary meter

	int interval_length;			//integer averaging length (seconds)

	bool firstWrite;

	TIMESTAMP perform_average_time;	//Timestamp to perform the average
	TIMESTAMP last_update_time;		//Last time array updated
	int curr_avg_pos_index;			//Index for current position of averaging array

};

#endif // C++

#endif // _METRICS_COLLECTOR_WRITER_H_

// EOF
