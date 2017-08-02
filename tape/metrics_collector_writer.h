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
#include <json/json.h> //jsoncpp library

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

private:

	int write_line(TIMESTAMP);

private:

	Json::Value metrics_writer_billing_meters;	// Final output dictionary for triplex_meters and non-swing meters
	Json::Value metrics_writer_houses;	// Final output dictionary for houses
	Json::Value metrics_writer_inverters;	// Final output dictionary for inverters
	Json::Value metrics_writer_feeder_information;	// Final output dictionary for feeder_information
	Json::Value metrics_writer_capacitors;
	Json::Value metrics_writer_regulators;

	Json::Value ary_billing_meters;  // array storage for billing meter metrics
	Json::Value ary_houses;          // array storage for house (and water heater) metrics
	Json::Value ary_inverters;       // array storage for inverter metrics
	Json::Value ary_feeders;         // array storage for feeder information metrics
	Json::Value ary_capacitors;
	Json::Value ary_regulators;

	char256 filename_billing_meter;
	char256 filename_inverter;
	char256 filename_house;
	char256 filename_substation;
	char256 filename_capacitor;
	char256 filename_regulator;

	TIMESTAMP startTime;
	TIMESTAMP final_write;
	TIMESTAMP next_write;
	TIMESTAMP last_write;
	bool interval_write;

	char* parent_string;

	FINDLIST *metrics_collectors;

	int interval_length;			//integer averaging length (seconds)
};

#endif // C++

#endif // _METRICS_COLLECTOR_WRITER_H_

// EOF
