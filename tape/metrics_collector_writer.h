/*
 * metrics_collector_writer.h
 *
 *  Created on: Jan 30, 2017
 *      Author: tang526
 *      Modifed d3j331 - Mitch Pelton
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
#include <algorithm>

#ifdef HAVE_HDF5
#include <memory>
#include "H5Cpp.h"
#endif

#define MAX_METRIC_NAME_LENGTH 48
#define MAX_METRIC_VALUE_LENGTH 24

using namespace std;

const string m_json("json");
const string m_h5("h5");

const string m_billing_meter("billing_meter");
const string m_inverter("inverter");
const string m_capacitor("capacitor");
const string m_regulator("regulator");
const string m_house("house");
const string m_feeder("substation");

const string m_transformer("transformer");

const string m_index("index");
const string m_units("units");
const string m_metadata("Metadata");
const string m_starttime("StartTime");
const string m_time("time");
const string m_name("name");
const string m_value("value");

const string m_real_power_min ("real_power_min");
const string m_real_power_max ("real_power_max");
const string m_real_power_avg ("real_power_avg");
const string m_real_power_median ("real_power_median");
const string m_real_power_losses_min ("real_power_losses_min");
const string m_real_power_losses_max ("real_power_losses_max");
const string m_real_power_losses_avg ("real_power_losses_avg");
const string m_real_power_losses_median ("real_power_losses_median");

const string m_reactive_power_min ("reactive_power_min");
const string m_reactive_power_max ("reactive_power_max");
const string m_reactive_power_avg ("reactive_power_avg");
const string m_reactive_power_median ("reactive_power_median");
const string m_reactive_power_losses_min ("reactive_power_losses_min");
const string m_reactive_power_losses_max ("reactive_power_losses_max");
const string m_reactive_power_losses_avg ("reactive_power_losses_avg");
const string m_reactive_power_losses_median ("reactive_power_losses_median");

const string m_real_energy ("real_energy");
const string m_reactive_energy ("reactive_energy");
const string m_bill ("bill");

const string m_voltage12_min ("voltage12_min");
const string m_voltage12_max ("voltage12_max");
const string m_voltage12_avg ("voltage12_avg");
const string m_above_RangeA_Duration ("above_RangeA_Duration");
const string m_below_RangeA_Duration ("below_RangeA_Duration");
const string m_above_RangeB_Duration ("above_RangeB_Duration");
const string m_below_RangeB_Duration ("below_RangeB_Duration");

const string m_voltage_min ("voltage_min");
const string m_voltage_max ("voltage_max");
const string m_voltage_avg ("voltage_avg");
const string m_voltage_unbalance_min ("voltage_unbalance_min");
const string m_voltage_unbalance_max ("voltage_unbalance_max");
const string m_voltage_unbalance_avg ("voltage_unbalance_avg");
const string m_above_RangeA_Count ("above_RangeA_Count");
const string m_below_RangeA_Count ("below_RangeA_Count");
const string m_above_RangeB_Count ("above_RangeB_Count");
const string m_below_RangeB_Count ("below_RangeB_Count");
const string m_below_10_percent_NormVol_Duration ("below_10_percent_NormVol_Duration");
const string m_below_10_percent_NormVol_Count ("below_10_percent_NormVol_Count");

const string m_total_load_min ("total_load_min");
const string m_total_load_max ("total_load_max");
const string m_total_load_avg ("total_load_avg");
const string m_hvac_load_min ("hvac_load_min");
const string m_hvac_load_max ("hvac_load_max");
const string m_hvac_load_avg ("hvac_load_avg");
const string m_air_temperature_min ("air_temperature_min");
const string m_air_temperature_max ("air_temperature_max");
const string m_air_temperature_avg ("air_temperature_avg");
const string m_air_temperature_deviation_cooling ("air_temperature_deviation_cooling");
const string m_air_temperature_deviation_heating ("air_temperature_deviation_heating");
const string m_waterheater_load_min ("waterheater_load_min");
const string m_waterheater_load_max ("waterheater_load_max");
const string m_waterheater_load_avg ("waterheater_load_avg");

const string m_operation_count ("operation_count");

const string m_trans_overload_perc ("trans_overload_perc");

#ifdef HAVE_HDF5
typedef struct {
	char name[MAX_METRIC_NAME_LENGTH]; 
	char value[MAX_METRIC_VALUE_LENGTH]; 
} hMetadata;

typedef struct _BillingMeter{
    int time; 
    char name[MAX_METRIC_NAME_LENGTH]; 
	double real_power_min;
	double real_power_max;
	double real_power_avg;
	double reactive_power_min;
	double reactive_power_max;
	double reactive_power_avg;
	double real_energy;
	double reactive_energy;
	double bill;
	double voltage12_min;
	double voltage12_max;
	double voltage12_avg;
	double above_RangeA_Duration;
	double below_RangeA_Duration;
	double above_RangeB_Duration;
	double below_RangeB_Duration;
#ifdef ALL_MTR_METRICS
	double voltage_min;
	double voltage_max;
	double voltage_avg;
	double voltage_unbalance_min;
	double voltage_unbalance_max;
	double voltage_unbalance_avg;
	double above_RangeA_Count;
	double below_RangeA_Count;
	double above_RangeB_Count;
	double below_RangeB_Count;
	double below_10_percent_NormVol_Duration;
	double below_10_percent_NormVol_Count;
#endif
} BillingMeter;

typedef struct _House {
    int time; 
    char name[MAX_METRIC_NAME_LENGTH]; 
	double total_load_min;
	double total_load_max;
	double total_load_avg;
	double hvac_load_min;
	double hvac_load_max;
	double hvac_load_avg;
	double air_temperature_min;
	double air_temperature_max;
	double air_temperature_avg;
	double air_temperature_deviation_cooling;
	double air_temperature_deviation_heating;
	double waterheater_load_min;
	double waterheater_load_max;
	double waterheater_load_avg;
} House;

typedef struct _Inverter {
    int time; 
    char name[MAX_METRIC_NAME_LENGTH]; 
	double real_power_min;
	double real_power_max;
	double real_power_avg;
	double reactive_power_min;
	double reactive_power_max;
	double reactive_power_avg;
} Inverter;

typedef struct _Capacitor {
    int time; 
    char name[MAX_METRIC_NAME_LENGTH]; 
	double operation_count;
} Capacitor;

typedef struct _Regulator {
    int time; 
    char name[MAX_METRIC_NAME_LENGTH]; 
	double operation_count;
} Regulator;

typedef struct _Feeder {
    int time; 
    char name[MAX_METRIC_NAME_LENGTH]; 
	double real_power_min;
	double real_power_max;
	double real_power_avg;
	double real_power_median;
	double reactive_power_min;
	double reactive_power_max;
	double reactive_power_avg;
	double reactive_power_median;
	double real_energy;
	double reactive_energy;
	double real_power_losses_min;
	double real_power_losses_max;
	double real_power_losses_avg;
	double real_power_losses_median;
	double reactive_power_losses_min;
	double reactive_power_losses_max;
	double reactive_power_losses_avg;
	double reactive_power_losses_median;
} Feeder;

typedef struct _Transformer {
  int time;
	char name[MAX_METRIC_NAME_LENGTH];
	double trans_overload_perc;
} Transformer;
#endif

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
	char8 extension;
	char8 alternate;
	double interval_length_dbl;			//Metrics output interval length
	double interim_length_dbl;			//Metrics output interim length

private:

	int write_line(TIMESTAMP);
	void writeMetadata(Json::Value& meta, Json::Value& metadata, char* time_str, char256 filename_house);
	void writeJsonFile (char256 filename, Json::Value& metrics);

#ifdef HAVE_HDF5
	// Functions to setup dataset structures
	void hdfMetadata ();
	void hdfBillingMeter ();
	void hdfHouse ();
	void hdfInverter ();
	void hdfCapacitor ();
	void hdfRegulator ();
	void hdfFeeder ();
	void hdfTransformer ();

	// Functions to write dataset to a file
	void hdfWrite(char256 filename, H5::CompType* mtype, void *ptr, int structKind, int size);
	void hdfMetadataWrite(Json::Value& meta, char* time_str, char256 filename);
	void hdfBillingMeterWrite (size_t objs, Json::Value& metrics);
	void hdfHouseWrite (size_t objs, Json::Value& metrics);
	void hdfInverterWrite (size_t objs, Json::Value& metrics);
	void hdfCapacitorWrite (size_t objs, Json::Value& metrics);
	void hdfRegulatorWrite (size_t objs, Json::Value& metrics);
	void hdfFeederWrite (size_t objs, Json::Value& metrics);
	void hdfTransformerWrite (size_t objs, Json::Value& metrics);
#endif

private:

	Json::Value metrics_writer_billing_meters;      // Final output dictionary for triplex_meters and non-swing meters
	Json::Value metrics_writer_houses;              // Final output dictionary for houses and water heater
	Json::Value metrics_writer_inverters;           // Final output dictionary for inverters
	Json::Value metrics_writer_feeders;             // Final output dictionary for feeders
	Json::Value metrics_writer_capacitors;          // Final output dictionary for capacitors
	Json::Value metrics_writer_regulators;          // Final output dictionary for regulators

	Json::Value metrics_writer_transformers;        // Final output dictionary for transformers

	Json::Value ary_billing_meters;  // array storage for billing meter metrics
	Json::Value ary_houses;          // array storage for house (and water heater) metrics
	Json::Value ary_inverters;       // array storage for inverter metrics
	Json::Value ary_feeders;         // array storage for feeder metrics
	Json::Value ary_capacitors;      // array storage for capacitors metrics
	Json::Value ary_regulators;      // array storage for regulators metrics

	Json::Value ary_transformers;    // array storage for tranformers metrics

#ifdef HAVE_HDF5
	H5::CompType* mtype_metadata;
	H5::CompType* mtype_billing_meters;
	H5::CompType* mtype_houses;
	H5::CompType* mtype_inverters;
	H5::CompType* mtype_feeders;
	H5::CompType* mtype_capacitors;
	H5::CompType* mtype_regulators;
	H5::CompType* mtype_transformers;
#endif

	char256 filename_billing_meter;
	char256 filename_inverter;
	char256 filename_house;
	char256 filename_feeder;
	char256 filename_capacitor;
	char256 filename_regulator;
	char256 filename_transformer;

	TIMESTAMP startTime;
	TIMESTAMP final_write;
	TIMESTAMP next_write;
	TIMESTAMP last_write;
	bool interval_write;

	char* parent_string;

	FINDLIST *metrics_collectors;

	int interval_length;			//integer averaging length (seconds)
	int interim_length;				//integer interim length (seconds to write out intervals and then clear)
	int interim_cnt;				//integer interim count (count to write out intervals)
	int line_cnt;					//integer write line count (count how many write_line in a interim write)
};

#endif // C++

#endif // _METRICS_COLLECTOR_WRITER_H_

// EOF
