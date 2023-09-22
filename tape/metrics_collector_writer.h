/*
 * metrics_collector_writer.h
 *
 *  Created on: Jan 30, 2017
 *      Author: tang526
 *      Modifed d3j331 - Mitch Pelton
 *      Modified on: December, 2019, Author: Laurentiu Marinovici
 */

#ifndef _METRICS_COLLECTOR_WRITER_H_
#define _METRICS_COLLECTOR_WRITER_H_


#include <json/json.h> //jsoncpp library
#include <iostream>
#include <fstream>
#include <cstring>
#include <string.h>
#include <algorithm>
#include <memory>

#include "tape.h"
#include "metrics_collector.h"

#ifdef HAVE_HDF5
#include "H5Cpp.h"
#endif

#define MAX_METRIC_NAME_LENGTH 48
#define MAX_METRIC_VALUE_LENGTH 28

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
const string m_line("line");
const string m_evchargerdet("evchargerdet");

const string m_index("index");
const string m_units("units");
const string m_metadata("Metadata");
const string m_starttime("StartTime");
const string m_time("time");
const string m_name("name");
const string m_date("date");
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
const string m_air_temperature_setpoint_cooling ("air_temperature_setpoint_cooling");
const string m_air_temperature_setpoint_heating ("air_temperature_setpoint_heating");
const string m_system_mode ("system_mode");
const string m_waterheater_load_min ("waterheater_load_min");
const string m_waterheater_load_max ("waterheater_load_max");
const string m_waterheater_load_avg ("waterheater_load_avg");
const string m_waterheater_setpoint_min ("waterheater_setpoint_min");
const string m_waterheater_setpoint_max ("waterheater_setpoint_max");
const string m_waterheater_setpoint_avg ("waterheater_setpoint_avg");
const string m_waterheater_demand_min ("waterheater_demand_min");
const string m_waterheater_demand_max ("waterheater_demand_max");
const string m_waterheater_demand_avg ("waterheater_demand_avg");
const string m_waterheater_temp_min ("waterheater_temp_min");
const string m_waterheater_temp_max ("waterheater_temp_max");
const string m_waterheater_temp_avg ("waterheater_temp_avg");

const string m_wh_lower_setpoint_min ("wh_lower_setpoint_min");
const string m_wh_lower_setpoint_max ("wh_lower_setpoint_max");
const string m_wh_lower_setpoint_avg ("wh_lower_setpoint_avg");
const string m_wh_upper_setpoint_min ("wh_upper_setpoint_min");
const string m_wh_upper_setpoint_max ("wh_upper_setpoint_max");
const string m_wh_upper_setpoint_avg ("wh_upper_setpoint_avg");
const string m_wh_lower_temp_min ("wh_lower_temp_min");
const string m_wh_lower_temp_max ("wh_lower_temp_max");
const string m_wh_lower_temp_avg ("wh_lower_temp_avg");
const string m_wh_upper_temp_min ("wh_upper_temp_min");
const string m_wh_upper_temp_max ("wh_upper_temp_max");
const string m_wh_upper_temp_avg ("wh_upper_temp_avg");
const string m_wh_lower_elem_state ("wh_lower_elem_state");
const string m_wh_upper_elem_state ("wh_upper_elem_state");

const string m_operation_count ("operation_count");

const string m_trans_overload_perc ("trans_overload_perc");
const string m_line_overload_perc ("line_overload_perc");

const string m_charge_rate_min ("charge_rate_min");
const string m_charge_rate_max ("charge_rate_max");
const string m_charge_rate_avg ("charge_rate_avg");
const string m_battery_SOC_min ("battery_SOC_min");
const string m_battery_SOC_max ("battery_SOC_max");
const string m_battery_SOC_avg ("battery_SOC_avg");

#ifdef HAVE_HDF5
typedef struct {
	char name[MAX_METRIC_NAME_LENGTH]; 
	char value[MAX_METRIC_VALUE_LENGTH]; 
} hMetadata;

typedef struct _BillingMeter{
	long int time; 
	char date[MAX_METRIC_VALUE_LENGTH]; 
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
	long int time; 
	char date[MAX_METRIC_VALUE_LENGTH]; 
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
	double air_temperature_setpoint_cooling;
	double air_temperature_setpoint_heating;
	int system_mode;
	double waterheater_load_min;
	double waterheater_load_max;
	double waterheater_load_avg;
	double waterheater_setpoint_min;
	double waterheater_setpoint_max;
	double waterheater_setpoint_avg;
	double waterheater_demand_min;
	double waterheater_demand_max;
	double waterheater_demand_avg;
	double waterheater_temp_min;
	double waterheater_temp_max;
	double waterheater_temp_avg;

	double wh_lower_setpoint_min;
	double wh_lower_setpoint_max;
	double wh_lower_setpoint_avg;
	double wh_upper_setpoint_min;
	double wh_upper_setpoint_max;
	double wh_upper_setpoint_avg;
	double wh_lower_temp_min;
	double wh_lower_temp_max;
	double wh_lower_temp_avg;
	double wh_upper_temp_min;
	double wh_upper_temp_max;
	double wh_upper_temp_avg;
	int wh_lower_elem_state;
	int wh_upper_elem_state;
} House;

typedef struct _Inverter {
	long int time; 
	char date[MAX_METRIC_VALUE_LENGTH]; 
	char name[MAX_METRIC_NAME_LENGTH]; 
	double real_power_min;
	double real_power_max;
	double real_power_avg;
	double reactive_power_min;
	double reactive_power_max;
	double reactive_power_avg;
} Inverter;

typedef struct _Capacitor {
	long int time; 
	char date[MAX_METRIC_VALUE_LENGTH]; 
	char name[MAX_METRIC_NAME_LENGTH]; 
	double operation_count;
} Capacitor;

typedef struct _Regulator {
	long int time; 
	char date[MAX_METRIC_VALUE_LENGTH]; 
	char name[MAX_METRIC_NAME_LENGTH]; 
	double operation_count;
} Regulator;

typedef struct _Feeder {
	long int time; 
	char date[MAX_METRIC_VALUE_LENGTH]; 
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
	long int time;
	char date[MAX_METRIC_VALUE_LENGTH]; 
	char name[MAX_METRIC_NAME_LENGTH];
	double trans_overload_perc;
} Transformer;

typedef struct _Line {
	long int time;
	char date[MAX_METRIC_VALUE_LENGTH]; 
	char name[MAX_METRIC_NAME_LENGTH];
	double line_overload_perc;
} Line;

typedef struct _EVChargerDet {
	long int time; 
	char date[MAX_METRIC_VALUE_LENGTH]; 
	char name[MAX_METRIC_NAME_LENGTH]; 
	double charge_rate_min;
	double charge_rate_max;
	double charge_rate_avg;
	double battery_SOC_min;
	double battery_SOC_max;
	double battery_SOC_avg;
} EVChargerDet;
#endif

EXPORT void new_metrics_collector_writer(MODULE *);

#ifdef __cplusplus

class metrics_collector_writer{
public:
	static metrics_collector_writer *defaults;
	static CLASS *oclass, *pclass;

	explicit metrics_collector_writer(MODULE *);
	int create();
	int init(OBJECT *);
	int isa(char *);
	TIMESTAMP postsync(TIMESTAMP, TIMESTAMP);

	int commit(TIMESTAMP);

public:
	bool both;
	char256 filename;
	char8 extension;
	char8 alternate;
	char8 allextensions;
	double interval_length_dbl;			//Metrics output interval length
	double interim_length_dbl;			//Metrics output interim length

private:

	int write_line(TIMESTAMP);
	void writeMetadata(Json::Value& meta, Json::Value& metadata, char* time_str, char256 filename_house);
	void writeJsonFile (char256 filename, Json::Value& metrics);
	void maketime(double time, char *buffer, int size);

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
	void hdfLine();
	void hdfEvChargerDet ();

	// Functions to write dataset to a file
	void hdfWrite(char256 filename, const std::unique_ptr<H5::CompType>& mtype, void *ptr, int structKind, int size);
	void hdfMetadataWrite(Json::Value& meta, char* time_str, char256 filename);
	void hdfBillingMeterWrite (size_t objs, Json::Value& metrics);
	void hdfHouseWrite (size_t objs, Json::Value& metrics);
	void hdfInverterWrite (size_t objs, Json::Value& metrics);
	void hdfCapacitorWrite (size_t objs, Json::Value& metrics);
	void hdfRegulatorWrite (size_t objs, Json::Value& metrics);
	void hdfFeederWrite (size_t objs, Json::Value& metrics);
	void hdfTransformerWrite (size_t objs, Json::Value& metrics);
	void hdfLineWrite (size_t objs, Json::Value& metrics);
	void hdfEvChargerDetWrite  (size_t objs, Json::Value& metrics);
#endif

private:

	Json::Value metrics_writer_billing_meters;      // Final output dictionary for triplex_meters and non-swing meters
	Json::Value metrics_writer_houses;              // Final output dictionary for houses and water heater
	Json::Value metrics_writer_inverters;           // Final output dictionary for inverters
	Json::Value metrics_writer_feeders;             // Final output dictionary for feeders
	Json::Value metrics_writer_capacitors;          // Final output dictionary for capacitors
	Json::Value metrics_writer_regulators;          // Final output dictionary for regulators
	Json::Value metrics_writer_transformers;        // Final output dictionary for transformers
	Json::Value metrics_writer_lines;               // Final output dictionary for lines
	Json::Value metrics_writer_evchargerdets;        // Final output dictionary for evcharger det

	Json::Value ary_billing_meters;  // array storage for billing meter metrics
	Json::Value ary_houses;          // array storage for house (and water heater) metrics
	Json::Value ary_inverters;       // array storage for inverter metrics
	Json::Value ary_feeders;         // array storage for feeder metrics
	Json::Value ary_capacitors;      // array storage for capacitors metrics
	Json::Value ary_regulators;      // array storage for regulators metrics
	Json::Value ary_transformers;    // array storage for tranformers metrics
	Json::Value ary_lines;           // array storage for lines metrics
	Json::Value ary_evchargerdets;    // array storage for evcharger det metrics

#ifdef HAVE_HDF5
	hsize_t len_billing_meters;
	hsize_t len_houses;
	hsize_t len_inverters;
	hsize_t len_feeders;
	hsize_t len_capacitors;
	hsize_t len_regulators;
	hsize_t len_transformers;
	hsize_t len_lines;
	hsize_t len_evchargerdets;

	std::unique_ptr<H5::DataSet> set_billing_meters;
	std::unique_ptr<H5::DataSet> set_houses;
	std::unique_ptr<H5::DataSet> set_inverters;
	std::unique_ptr<H5::DataSet> set_feeders;
	std::unique_ptr<H5::DataSet> set_capacitors;
	std::unique_ptr<H5::DataSet> set_regulators;
	std::unique_ptr<H5::DataSet> set_transformers;
	std::unique_ptr<H5::DataSet> set_lines;
	std::unique_ptr<H5::DataSet> set_evchargerdets;

	std::unique_ptr<H5::CompType> mtype_metadata;
	std::unique_ptr<H5::CompType> mtype_billing_meters;
	std::unique_ptr<H5::CompType> mtype_houses;
	std::unique_ptr<H5::CompType> mtype_inverters;
	std::unique_ptr<H5::CompType> mtype_feeders;
	std::unique_ptr<H5::CompType> mtype_capacitors;
	std::unique_ptr<H5::CompType> mtype_regulators;
	std::unique_ptr<H5::CompType> mtype_transformers;
	std::unique_ptr<H5::CompType> mtype_lines;
	std::unique_ptr<H5::CompType> mtype_evchargerdets;
#endif

	char256 filename_billing_meter;
	char256 filename_inverter;
	char256 filename_house;
	char256 filename_feeder;
	char256 filename_capacitor;
	char256 filename_regulator;
	char256 filename_transformer;
	char256 filename_line;
	char256 filename_evchargerdet;

	DATETIME dt;
	TIMESTAMP startTime;
	TIMESTAMP final_write;
	TIMESTAMP next_write;
	TIMESTAMP last_write;
	bool interval_write;
	bool new_day;
	
	char* parent_string;

	FINDLIST *metrics_collectors;

	int interval_length;			//integer averaging length (seconds)
	int interim_length;				//integer interim length (seconds to write out intervals and then clear)
	int interim_cnt;				//integer interim count (count to write out intervals)
	int line_cnt;					//integer write line count (count how many write_line in a interim write)
	int day_cnt;					//integer day count (count how many day tables are in written)
	int writeTime;					//represents the time from the StartTime
};

#endif // C++

#endif // _METRICS_COLLECTOR_WRITER_H_

// EOF
