/*
 * metrics_collector_writer.cpp
 *
 *  Created on: Jan 30, 2017,  Author: tang526
 *  Modified on: June, 2019,   Author: Mitch Pelton
 *  Modified on: December, 2019, Author: Laurentiu Marinovici
 */

#include "metrics_collector_writer.h"

CLASS *metrics_collector_writer::oclass = NULL;

void new_metrics_collector_writer(MODULE *mod) {
	new metrics_collector_writer(mod);
}

metrics_collector_writer::metrics_collector_writer(MODULE *mod) {
	if (oclass == NULL) {
		oclass = gl_register_class(mod, const_cast<char *>("metrics_collector_writer"), sizeof(metrics_collector_writer), PC_POSTTOPDOWN);
		if (oclass==NULL)
			throw "unable to register class metrics_collector_writer";

		if (gl_publish_variable(oclass,
				PT_char256, "filename", PADDR(filename), PT_DESCRIPTION, "the JSON formatted output file name",
				PT_char8, "extension", PADDR(extension), PT_DESCRIPTION, "the file formatted type (JSON, H5)",
				PT_char8, "alternate", PADDR(alternate), PT_DESCRIPTION, "the alternate file name convention",
				PT_char8, "allextensions", PADDR(allextensions), PT_DESCRIPTION, "write all file extensions",
				PT_double, "interim[s]", PADDR(interim_length_dbl), PT_DESCRIPTION, "Interim at which metrics_collector_writer output is written",
				PT_double, "interval[s]", PADDR(interval_length_dbl), PT_DESCRIPTION, "Interval at which the metrics_collector_writer output is stored in JSON format",
				NULL) < 1)
			GL_THROW(const_cast<char *>("unable to publish properties in %s"), __FILE__);
	}
}

int metrics_collector_writer::isa(char *classname) {
	return (strcmp(classname, oclass->name) == 0);
}

int metrics_collector_writer::create() {
	return 1;
}

int metrics_collector_writer::init(OBJECT *parent) {
	OBJECT *obj = OBJECTHDR(this);
	FILE *fn = NULL;
	int index = 0;
	char time_str[64];

	// check for filename
	if (0 == filename[0]) {
		// if no filename, auto-generate based on ID
		sprintf(filename, "%256s-%256i-metrics_collector_output", oclass->name, obj->id);
		gl_warning("metrics_collector_writer::init(): no filename defined, auto-generating '%s'", filename.get_string());
		/*
		 requires a filename.  If none is provided, a filename will be generated
		 using a combination of the classname and the core-assigned ID number.
		 */
	}

	// check for extension
	if (0 == extension[0]) {
		// if no filename, auto-generate based on ID
		sprintf(extension, "json");
		gl_warning("metrics_collector_writer::init(): no extension defined, auto-generating '%s'", extension.get_string());
		/*	requires a extension.  If none is provided, a .json will be used	*/
	} else {
		if (!(strcmp(extension, m_json.c_str()) == 0) && !(strcmp(extension, m_h5.c_str()) == 0)) {
			sprintf(extension, "json");
			gl_warning("metrics_collector_writer::init(): bad extension defined, auto-generating '%s'", extension.get_string());
		}
#ifndef HAVE_HDF5
		else {
			if (strcmp(extension, m_h5.c_str()) == 0)
				gl_warning("metrics_collector_writer::init(): H5 extension defined, but HDF library not found");
			sprintf(extension, "json");
			gl_warning("metrics_collector_writer::init(): bad extension defined, auto-generating '%s'", extension.get_string());
		}
#endif
	}

	both = false;
	// Check valid metrics_collector_writer output alternate path
	if (0 == alternate[0]) {
		// if no alternate file naming flag
		gl_warning("metrics_collector_writer::init(): no option to set alternate metrics file name give, so going with default, as if alternate no");
		sprintf(alternate, "no");
	} else {
		if (!(strcmp(alternate, "no") == 0) && !(strcmp(alternate, "yes") == 0)) {
			gl_warning("metrics_collector_writer::init(): bad alternate option given. Should be either no or yes. Default = no");
			sprintf(alternate, "no");
		} else {
			if (strcmp(alternate, "yes") == 0) {
				if (!(0 == allextensions[0])) {
					if (!(strcmp(allextensions, "no") == 0) && !(strcmp(allextensions, "yes") == 0)) {
						gl_warning("metrics_collector_writer::init(): bad allextensions option given. Should be either no or yes. Default = no");
					} else {
						if (strcmp(allextensions, "yes") == 0) {
							both = true;
							sprintf(extension, "json");
						}
					}
				}
			}
		}
	}

	// Check valid metrics_collector output interval
	interval_length = (int64) (interval_length_dbl);
	if (interval_length <= 0) {
		gl_error("metrics_collector_writer::init(): invalid interval of %i, must be greater than 0", interval_length);
		/*	The metrics_collector interval must be a positive number of seconds.	*/
		return 0;
	}

	// Check valid metrics_collector output interim interval write out and clear
	new_day = true;
	day_cnt = 1;
	line_cnt = 1;
	interim_cnt = 1;
	interim_length = (int64) (interim_length_dbl);
	if (interim_length <= 0) {
		interim_length = 86400;
		gl_warning("metrics_collector_writer::init(): no interim length defined, setting to %i seconds", interim_length);
	}

	// Find the metrics_collector objects
	metrics_collectors = gl_find_objects(FL_NEW, FT_CLASS, SAME, "metrics_collector", FT_END); //find all metrics_collector objects
	if (metrics_collectors == NULL) {
		gl_error("metrics_collector_writer::init(): no metrics_collector objects were found.");
		return 0;
		/* TROUBLESHOOT
		 * No metrics_collector objects were found in your .glm file.
		 * if you defined a metrics_collector_writer object, make sure you have metrics_collector objects defined also.
		 */
	}

	// Go through each metrics_collector object, and check its time interval given
	obj = NULL;
	while (obj = gl_find_next(metrics_collectors, obj)) {
		if (index >= metrics_collectors->hit_count) {
			break;
		}
		// Obtain the object data
		metrics_collector *temp_metrics_collector = OBJECTDATA(obj, metrics_collector);
		if (temp_metrics_collector == NULL) {
			gl_error("metrics_collector_writer::init(): unable to map object as metrics_collector object.");
			return 0;
		}
		// Obtain the object time interval and compare
		if (temp_metrics_collector->interval_length_dbl != interval_length_dbl) {
			gl_error("metrics_collector_writer::init(): currently the time interval of the metrics_collector should be the same as the metrics_collector_writer");
			return 0;
		}
	}

	// Update time variables
	startTime = gl_globalclock;
	next_write = gl_globalclock + interval_length;
	final_write = gl_globalstoptime;

	// Copied from recorder object
	if (0 == gl_localtime(startTime, &dt)) {
		gl_error("metrics_collector_writer::init(): error when converting the starting time");
		/* TROUBLESHOOT
		 Unprintable timestamp.
		 */
		return 0;
	}
	if (0 == gl_strtime(&dt, time_str, sizeof(time_str))) {
		gl_error("metrics_collector_writer::init(): error when writing the starting time as a string");
		/* TROUBLESHOOT
		 Error printing the timestamp.
		 */
		return 0;
	}

	// Write separate json files for meters, triplex_meters, inverters, capacitors, regulators, houses, feeders, transformers, lines, evchargers:

	if (strcmp(alternate, "no") == 0) {
		snprintf(filename_billing_meter, sizeof(filename_billing_meter)-1, "%s_%s", m_billing_meter.c_str(), filename.get_string());
		snprintf(filename_inverter, sizeof(filename_inverter)-1, "%s_%s", m_inverter.c_str(), filename.get_string());
		snprintf(filename_capacitor, sizeof(filename_capacitor)-1, "%s_%s", m_capacitor.c_str(), filename.get_string());
		snprintf(filename_regulator, sizeof(filename_regulator)-1, "%s_%s", m_regulator.c_str(), filename.get_string());
		snprintf(filename_house, sizeof(filename_house)-1, "%s_%s", m_house.c_str(), filename.get_string());
		snprintf(filename_feeder, sizeof(filename_feeder)-1, "%s_%s", m_feeder.c_str(), filename.get_string());
		snprintf(filename_transformer, sizeof(filename_transformer)-1, "%s_%s", m_transformer.c_str(), filename.get_string());
		snprintf(filename_line, sizeof(filename_line)-1, "%s_%s", m_line.c_str(), filename.get_string());
		snprintf(filename_evchargerdet, sizeof(filename_evchargerdet)-1, "%s_%s", m_evchargerdet.c_str(), filename.get_string());

	} else {
		snprintf(filename_billing_meter, sizeof(filename_billing_meter)-1, "%s%s", filename.get_string(), m_billing_meter.c_str());
		snprintf(filename_inverter, sizeof(filename_inverter)-1, "%s%s", filename.get_string(), m_inverter.c_str());
		snprintf(filename_capacitor, sizeof(filename_capacitor)-1, "%s%s", filename.get_string(), m_capacitor.c_str());
		snprintf(filename_regulator, sizeof(filename_regulator)-1, "%s%s", filename.get_string(), m_regulator.c_str());
		snprintf(filename_house, sizeof(filename_house)-1, "%s%s", filename.get_string(), m_house.c_str());
		snprintf(filename_feeder, sizeof(filename_feeder)-1, "%s%s", filename.get_string(), m_feeder.c_str());
		snprintf(filename_transformer, sizeof(filename_transformer)-1, "%s%s", filename.get_string(), m_transformer.c_str());
		snprintf(filename_line, sizeof(filename_line)-1, "%s%s", filename.get_string(), m_line.c_str());
		snprintf(filename_evchargerdet, sizeof(filename_evchargerdet)-1, "%s%s", filename.get_string(), m_evchargerdet.c_str());
	}
#ifdef HAVE_HDF5
	//prepare dataset for HDF5 if needed
	if ((strcmp(extension, m_h5.c_str()) == 0) || both) {
		H5::Exception::dontPrint();
		try {
			hdfMetadata();
			hdfBillingMeter();
			hdfHouse();
			hdfInverter();
			hdfCapacitor();
			hdfRegulator();
			hdfFeeder();
			hdfTransformer();
			hdfLine();
			hdfEvChargerDet();
		}
		// catch failure caused by the H5File operations
		catch( H5::FileIException error ){
			error.printErrorStack();
			return -1;
		}
		// catch failure caused by the DataSet operations
		catch( H5::DataSetIException error ){
			error.printErrorStack();
			return -1;
		}
		// catch failure caused by the DataSpace operations
		catch( H5::DataSpaceIException error ){
			error.printErrorStack();
			return -1;
		}
		catch( const std::exception& ex){
			std::cerr << "Unhandled general exception in HDF5(" << __func__ << "): " << ex.what() << std::endl;
		}
	}
#endif

	// Write metadata for each file; these indices MUST match assignments below
	Json::Value jsn;
	Json::Value meta;
	Json::Value metadata;

	int idx = 0;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "Wh";	meta[m_real_energy] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VARh";	meta[m_reactive_energy] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "USD";	meta[m_bill] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "V";		meta[m_voltage12_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "V";		meta[m_voltage12_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "V";		meta[m_voltage12_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "s";		meta[m_above_RangeA_Duration] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "s";		meta[m_below_RangeA_Duration] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "s";		meta[m_above_RangeB_Duration] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "s";		meta[m_below_RangeB_Duration] = jsn;
#ifdef ALL_MTR_METRICS
	jsn[m_index] = idx++;	jsn[m_units] = "V";		meta[m_voltage_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "V";		meta[m_voltage_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "V";		meta[m_voltage_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "V";		meta[m_voltage_unbalance_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "V";		meta[m_voltage_unbalance_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "V";		meta[m_voltage_unbalance_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "";		meta[m_above_RangeA_Count] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "";		meta[m_below_RangeA_Count] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "";		meta[m_above_RangeB_Count] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "";		meta[m_below_RangeB_Count] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "s";		meta[m_below_10_percent_NormVol_Duration] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "";		meta[m_below_10_percent_NormVol_Count] = jsn;
#endif
	ary_billing_meters.resize(idx);
	writeMetadata(meta, metadata, time_str, filename_billing_meter);

	idx = 0;
	jsn[m_index] = idx++;	jsn[m_units] = "kW";	meta[m_total_load_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "kW";	meta[m_total_load_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "kW";	meta[m_total_load_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "kW";	meta[m_hvac_load_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "kW";	meta[m_hvac_load_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "kW";	meta[m_hvac_load_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_air_temperature_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_air_temperature_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_air_temperature_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_air_temperature_setpoint_cooling] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_air_temperature_setpoint_heating] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "";		meta[m_system_mode] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "kW";	meta[m_waterheater_load_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "kW";	meta[m_waterheater_load_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "kW";	meta[m_waterheater_load_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_waterheater_setpoint_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_waterheater_setpoint_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_waterheater_setpoint_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "gpm";	meta[m_waterheater_demand_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "gpm";	meta[m_waterheater_demand_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "gpm";	meta[m_waterheater_demand_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_waterheater_temp_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_waterheater_temp_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_waterheater_temp_avg] = jsn;

	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_wh_lower_setpoint_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_wh_lower_setpoint_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_wh_lower_setpoint_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_wh_upper_setpoint_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_wh_upper_setpoint_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_wh_upper_setpoint_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_wh_lower_temp_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_wh_lower_temp_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_wh_lower_temp_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_wh_upper_temp_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_wh_upper_temp_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "degF";	meta[m_wh_upper_temp_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "";		meta[m_wh_lower_elem_state] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "";		meta[m_wh_upper_elem_state] = jsn;

	ary_houses.resize(idx);
	writeMetadata(meta, metadata, time_str, filename_house);

	idx = 0;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_avg] = jsn;
	ary_inverters.resize(idx);
	writeMetadata(meta, metadata, time_str, filename_inverter);

	idx = 0;
	jsn[m_index] = idx++;	jsn[m_units] = "";		meta[m_operation_count] = jsn;
	ary_capacitors.resize(idx);
	writeMetadata(meta, metadata, time_str, filename_capacitor);

	idx = 0;
	jsn[m_index] = idx++;	jsn[m_units] = "";		meta[m_operation_count] = jsn;
	ary_regulators.resize(idx);
	writeMetadata(meta, metadata, time_str, filename_regulator);

	idx = 0;
	jsn[m_index] = idx++;	jsn[m_units] = "%";		meta[m_trans_overload_perc] = jsn;
	ary_transformers.resize(idx);
	writeMetadata(meta, metadata, time_str, filename_transformer);

	idx = 0;
	jsn[m_index] = idx++;	jsn[m_units] = "%";		meta[m_line_overload_perc] = jsn;
	ary_lines.resize(idx);
	writeMetadata(meta, metadata, time_str, filename_line);

	idx = 0;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_median] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_median] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "Wh";	meta[m_real_energy] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VARh";	meta[m_reactive_energy] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_losses_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_losses_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_losses_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_real_power_losses_median] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_losses_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_losses_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_losses_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "VAR";	meta[m_reactive_power_losses_median] = jsn;
	ary_feeders.resize(idx);
	writeMetadata(meta, metadata, time_str, filename_feeder);

	idx = 0;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_charge_rate_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_charge_rate_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "W";		meta[m_charge_rate_avg] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "%";	meta[m_battery_SOC_min] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "%";	meta[m_battery_SOC_max] = jsn;
	jsn[m_index] = idx++;	jsn[m_units] = "%";	meta[m_battery_SOC_avg] = jsn;
	ary_evchargerdets.resize(idx);
	writeMetadata(meta, metadata, time_str, filename_evchargerdet);

	return 1;
}

void metrics_collector_writer::writeMetadata(Json::Value &meta,
		Json::Value &metadata, char *time_str, char256 filename) {
	if (strcmp(extension, m_json.c_str()) == 0) {
		Json::StreamWriterBuilder builder;
		builder["indentation"] = "";
		ofstream out_file;

		metadata[m_starttime] = time_str;
		metadata[m_metadata] = meta;
		string FileName(filename);
		if (strcmp(alternate, "yes") == 0)
			FileName.append("." + m_json);
		out_file.open (FileName);
		out_file << Json::writeString(builder, metadata);
		out_file.close();
#ifdef HAVE_HDF5
		if (both) {
			hdfMetadataWrite(meta, time_str, filename);
		}
#endif
	}
#ifdef HAVE_HDF5
	else {
		hdfMetadataWrite(meta, time_str, filename);
	}
#endif
	metadata.clear();
	meta.clear();
}

TIMESTAMP metrics_collector_writer::postsync(TIMESTAMP t0, TIMESTAMP t1) {
//	cout << "postsync t0-" << t0 << " t1-" << t1 << endl;
	// recalculate next_time, since we know commit() will fire
	if (!interval_write) {
		if (next_write == t1) {
			interval_write = true;
//			cout << "postsync write " << next_write << endl;
		}
		return next_write;
	}
	// the interval recorders have already returned t1+interval_length, earlier in the sequence.
	return TS_NEVER;
}

int metrics_collector_writer::commit(TIMESTAMP t1) {
//	cout << "commit t1 " << t1 << endl;
	// if periodic interval, check for write
	if (interval_write) {
//		cout << "commit write " << t1 << endl;
		if (0 == write_line(t1)) {
			gl_error("metrics_collector_writer::commit(): error when writing the values");
			return 0;
		}
		interval_write = false;
	}
	return 1;
}

/**
 @return 1 on successful write, 0 on unsuccessful write, error, or when not ready
 **/
int metrics_collector_writer::write_line(TIMESTAMP t1) {
	char time_str[64];
	time_t now = time(NULL);
	int index = 0;

	double *metrics;
	// metrics JSON value
	Json::Value billing_meter_objects;
	Json::Value house_objects;
	Json::Value inverter_objects;
	Json::Value capacitor_objects;
	Json::Value regulator_objects;
	Json::Value feeder_objects;
	Json::Value transformer_objects;
	Json::Value line_objects;
	Json::Value evchargerdet_objects;

	// Write Time -> represents the time from the StartTime
	writeTime = t1 - startTime; // in seconds
	sprintf(time_str, "%d", writeTime);

//	cout << "write_line at " << writeTime << " seconds, final " << final_write << ", now " << t1 << endl;

	// Go through each metrics_collector object, and check its time interval given
	OBJECT *obj = NULL;
	while (obj = gl_find_next(metrics_collectors, obj)) {
		if (index >= metrics_collectors->hit_count) {
			break;
		}

		// Obtain the object data
		metrics_collector *temp_metrics_collector = OBJECTDATA(obj, metrics_collector);
		if (temp_metrics_collector == NULL) {
			gl_error("Unable to map object as metrics_collector object.");
			return 0;
		}

		// Check each metrics_collector parent type
		if ((strcmp(temp_metrics_collector->parent_string, "triplex_meter") == 0)
				|| (strcmp(temp_metrics_collector->parent_string, "meter") == 0)) {
			metrics = temp_metrics_collector->metrics;
			int idx = 0;
			ary_billing_meters[idx++] = metrics[MTR_MIN_REAL_POWER];
			ary_billing_meters[idx++] = metrics[MTR_MAX_REAL_POWER];
			ary_billing_meters[idx++] = metrics[MTR_AVG_REAL_POWER];
			ary_billing_meters[idx++] = metrics[MTR_MIN_REAC_POWER];
			ary_billing_meters[idx++] = metrics[MTR_MAX_REAC_POWER];
			ary_billing_meters[idx++] = metrics[MTR_AVG_REAC_POWER];
			ary_billing_meters[idx++] = metrics[MTR_REAL_ENERGY];
			ary_billing_meters[idx++] = metrics[MTR_REAC_ENERGY];
			// TODO - verify the fixed charge is included
			ary_billing_meters[idx++] = metrics[MTR_BILL]; // Price unit given is $/kWh
			ary_billing_meters[idx++] = metrics[MTR_MIN_VLL];
			ary_billing_meters[idx++] = metrics[MTR_MAX_VLL];
			ary_billing_meters[idx++] = metrics[MTR_AVG_VLL];
			ary_billing_meters[idx++] = metrics[MTR_ABOVE_A_DUR];
			ary_billing_meters[idx++] = metrics[MTR_BELOW_A_DUR];
			ary_billing_meters[idx++] = metrics[MTR_ABOVE_B_DUR];
			ary_billing_meters[idx++] = metrics[MTR_BELOW_B_DUR];
#ifdef ALL_MTR_METRICS
			ary_billing_meters[idx++] = metrics[MTR_MIN_VLN];
			ary_billing_meters[idx++] = metrics[MTR_MAX_VLN];
			ary_billing_meters[idx++] = metrics[MTR_AVG_VLN];
			ary_billing_meters[idx++] = metrics[MTR_MIN_VUNB];
			ary_billing_meters[idx++] = metrics[MTR_MAX_VUNB];
			ary_billing_meters[idx++] = metrics[MTR_AVG_VUNB];
			ary_billing_meters[idx++] = metrics[MTR_ABOVE_A_CNT];
			ary_billing_meters[idx++] = metrics[MTR_BELOW_A_CNT];
			ary_billing_meters[idx++] = metrics[MTR_ABOVE_B_CNT];
			ary_billing_meters[idx++] = metrics[MTR_BELOW_B_CNT];
			ary_billing_meters[idx++] = metrics[MTR_BELOW_10_DUR];
			ary_billing_meters[idx++] = metrics[MTR_BELOW_10_CNT];
#endif
			string key = temp_metrics_collector->parent_name;
			billing_meter_objects[key] = ary_billing_meters;
		} // End of recording metrics_collector data attached to one triplex_meter or primary meter
		else if (strcmp(temp_metrics_collector->parent_string, "house") == 0) {
			metrics = temp_metrics_collector->metrics;
			string key = temp_metrics_collector->parent_name;
			int idx = 0;
			ary_houses[idx++] = metrics[HSE_MIN_TOTAL_LOAD];
			ary_houses[idx++] = metrics[HSE_MAX_TOTAL_LOAD];
			ary_houses[idx++] = metrics[HSE_AVG_TOTAL_LOAD];
			ary_houses[idx++] = metrics[HSE_MIN_HVAC_LOAD];
			ary_houses[idx++] = metrics[HSE_MAX_HVAC_LOAD];
			ary_houses[idx++] = metrics[HSE_AVG_HVAC_LOAD];
			ary_houses[idx++] = metrics[HSE_MIN_AIR_TEMP];
			ary_houses[idx++] = metrics[HSE_MAX_AIR_TEMP];
			ary_houses[idx++] = metrics[HSE_AVG_AIR_TEMP];
			ary_houses[idx++] = metrics[HSE_AVG_DEV_COOLING];
			ary_houses[idx++] = metrics[HSE_AVG_DEV_HEATING];
			ary_houses[idx++] = metrics[HSE_SYSTEM_MODE];
			if (!house_objects.isMember(key)) { // already made this house
				for (int j = 0; j < WH_ARRAY_SIZE; j++) {
					ary_houses[idx++] = 0.0;
				}
			}
			house_objects[key] = ary_houses;
		} // End of recording metrics_collector data attached to one house
		else if (strcmp(temp_metrics_collector->parent_string, "waterheater") == 0) {
			metrics = temp_metrics_collector->metrics;
			string key = temp_metrics_collector->parent_name;
			int idx = 0;
			if (house_objects.isMember(key)) { // already made this house
				idx = HSE_ARRAY_SIZE; // start of the waterheater metrics
			} else {
				for (int j = 0; j < HSE_ARRAY_SIZE; j++) {
					ary_houses[idx++] = 0.0;
				}
			}
			ary_houses[idx++] = metrics[WH_MIN_ACTUAL_LOAD];
			ary_houses[idx++] = metrics[WH_MAX_ACTUAL_LOAD];
			ary_houses[idx++] = metrics[WH_AVG_ACTUAL_LOAD];
			ary_houses[idx++] = metrics[WH_MIN_SETPOINT];
			ary_houses[idx++] = metrics[WH_MAX_SETPOINT];
			ary_houses[idx++] = metrics[WH_AVG_SETPOINT];
			ary_houses[idx++] = metrics[WH_MIN_DEMAND];
			ary_houses[idx++] = metrics[WH_MAX_DEMAND];
			ary_houses[idx++] = metrics[WH_AVG_DEMAND];
			ary_houses[idx++] = metrics[WH_MIN_TEMP];
			ary_houses[idx++] = metrics[WH_MAX_TEMP];
			ary_houses[idx++] = metrics[WH_AVG_TEMP];

			ary_houses[idx++] = metrics[WH_MIN_L_SETPOINT];
			ary_houses[idx++] = metrics[WH_MAX_L_SETPOINT];
			ary_houses[idx++] = metrics[WH_AVG_L_SETPOINT];
			ary_houses[idx++] = metrics[WH_MIN_U_SETPOINT];
			ary_houses[idx++] = metrics[WH_MAX_U_SETPOINT];
			ary_houses[idx++] = metrics[WH_AVG_U_SETPOINT];
			ary_houses[idx++] = metrics[WH_MIN_L_TEMP];
			ary_houses[idx++] = metrics[WH_MAX_L_TEMP];
			ary_houses[idx++] = metrics[WH_AVG_L_TEMP];
			ary_houses[idx++] = metrics[WH_MIN_U_TEMP];
			ary_houses[idx++] = metrics[WH_MAX_U_TEMP];
			ary_houses[idx++] = metrics[WH_AVG_U_TEMP];
			ary_houses[idx++] = metrics[WH_ELEM_L_MODE];
			ary_houses[idx++] = metrics[WH_ELEM_U_MODE];

			house_objects[key] = ary_houses;

		} // End of recording metrics_collector data attached to one waterheater
		else if (strcmp(temp_metrics_collector->parent_string, "inverter") == 0) {
			metrics = temp_metrics_collector->metrics;
			int idx = 0;
			ary_inverters[idx++] = metrics[INV_MIN_REAL_POWER];
			ary_inverters[idx++] = metrics[INV_MAX_REAL_POWER];
			ary_inverters[idx++] = metrics[INV_AVG_REAL_POWER];
			ary_inverters[idx++] = metrics[INV_MIN_REAC_POWER];
			ary_inverters[idx++] = metrics[INV_MAX_REAC_POWER];
			ary_inverters[idx++] = metrics[INV_AVG_REAC_POWER];
			string key = temp_metrics_collector->parent_name;
			inverter_objects[key] = ary_inverters;
		} // End of recording metrics_collector data attached to one inverter
		else if (strcmp(temp_metrics_collector->parent_string, "capacitor") == 0) {
			metrics = temp_metrics_collector->metrics;
			int idx = 0;
			ary_capacitors[idx++] = metrics[CAP_OPERATION_CNT];
			string key = temp_metrics_collector->parent_name;
			capacitor_objects[key] = ary_capacitors;
		} // End of recording metrics_collector data attached to one capacitor
		else if (strcmp(temp_metrics_collector->parent_string, "regulator") == 0) {
			metrics = temp_metrics_collector->metrics;
			int idx = 0;
			ary_regulators[idx++] = metrics[REG_OPERATION_CNT];
			string key = temp_metrics_collector->parent_name;
			regulator_objects[key] = ary_regulators;
		} // End of recording metrics_collector data attached to one regulator
		else if (strcmp(temp_metrics_collector->parent_string, "transformer") == 0) {
			metrics = temp_metrics_collector->metrics;
			int idx = 0;
			ary_transformers[idx++] = metrics[TRANS_OVERLOAD_PERC];
			string key = temp_metrics_collector->parent_name;
			transformer_objects[key] = ary_transformers;
		} // End of recording metrics_collector data attached to one transformer
		else if (strcmp(temp_metrics_collector->parent_string, "line") == 0) {
			metrics = temp_metrics_collector->metrics;
			int idx = 0;
			ary_lines[idx++] = metrics[LINE_OVERLOAD_PERC];
			string key = temp_metrics_collector->parent_name;
			line_objects[key] = ary_lines;
		} // End of recording metrics_collector data attached to one line
		else if (strcmp(temp_metrics_collector->parent_string, "swingbus") == 0) {
			metrics = temp_metrics_collector->metrics;
			int idx = 0;
			ary_feeders[idx++] = metrics[FDR_MIN_REAL_POWER];
			ary_feeders[idx++] = metrics[FDR_MAX_REAL_POWER];
			ary_feeders[idx++] = metrics[FDR_AVG_REAL_POWER];
			ary_feeders[idx++] = metrics[FDR_MED_REAL_POWER];
			ary_feeders[idx++] = metrics[FDR_MIN_REAC_POWER];
			ary_feeders[idx++] = metrics[FDR_MAX_REAC_POWER];
			ary_feeders[idx++] = metrics[FDR_AVG_REAC_POWER];
			ary_feeders[idx++] = metrics[FDR_MED_REAC_POWER];
			ary_feeders[idx++] = metrics[FDR_REAL_ENERGY];
			ary_feeders[idx++] = metrics[FDR_REAC_ENERGY];
			ary_feeders[idx++] = metrics[FDR_MIN_REAL_LOSS];
			ary_feeders[idx++] = metrics[FDR_MAX_REAL_LOSS];
			ary_feeders[idx++] = metrics[FDR_AVG_REAL_LOSS];
			ary_feeders[idx++] = metrics[FDR_MED_REAL_LOSS];
			ary_feeders[idx++] = metrics[FDR_MIN_REAC_LOSS];
			ary_feeders[idx++] = metrics[FDR_MAX_REAC_LOSS];
			ary_feeders[idx++] = metrics[FDR_AVG_REAC_LOSS];
			ary_feeders[idx++] = metrics[FDR_MED_REAC_LOSS];
			string key = temp_metrics_collector->parent_name;
			feeder_objects[key] = ary_feeders;
		} // End of recording metrics_collector data attached to the swing-bus/substation/feeder meter
		else if (strcmp(temp_metrics_collector->parent_string, "evcharger_det") == 0) {
			metrics = temp_metrics_collector->metrics;
			int idx = 0;
			ary_evchargerdets[idx++] = metrics[EV_MIN_CHARGE_RATE];
			ary_evchargerdets[idx++] = metrics[EV_MAX_CHARGE_RATE];
			ary_evchargerdets[idx++] = metrics[EV_AVG_CHARGE_RATE];
			ary_evchargerdets[idx++] = metrics[EV_MIN_BATTERY_SOC];
			ary_evchargerdets[idx++] = metrics[EV_MAX_BATTERY_SOC];
			ary_evchargerdets[idx++] = metrics[EV_AVG_BATTERY_SOC];
			string key = temp_metrics_collector->parent_name;
			evchargerdet_objects[key] = ary_evchargerdets;
		} // End of recording metrics_collector data attached to the swing-bus/substation/feeder meter
		index++;
	}

	// Rewrite the metrics to be separate 2-d ones
	metrics_writer_billing_meters[time_str] = billing_meter_objects;
	metrics_writer_houses[time_str] = house_objects;
	metrics_writer_inverters[time_str] = inverter_objects;
	metrics_writer_capacitors[time_str] = capacitor_objects;
	metrics_writer_regulators[time_str] = regulator_objects;
	metrics_writer_feeders[time_str] = feeder_objects;
	metrics_writer_transformers[time_str] = transformer_objects;
	metrics_writer_lines[time_str] = line_objects;
	metrics_writer_evchargerdets[time_str] = evchargerdet_objects;

	if (writeTime == (interim_length * interim_cnt) || final_write - startTime <= writeTime) {
        gl_debug("metrics collected -> %d\n", index);
        gl_debug("interim write time -> %d\n", writeTime);
		/*
		 cout << "meterics collected -> " << index << endl;
		 cout << "interim write time -> " << writeTime << endl;
		 cout << "final_write -> " << final_write-startTime << endl;
		 cout << "interim_length -> " << interim_length << endl;
		 cout << "interim_cnt -> " << interim_cnt << endl;
		 cout << m_billing_meter << "size ->" << billing_meter_objects.size() << endl;
		 cout << m_house << "size -> " << house_objects.size() << endl;
		 cout << m_inverter << "size -> " << inverter_objects.size() << endl;
		 cout << m_capacitor << "size -> " << capacitor_objects.size() << endl;
		 cout << m_regulator << "size -> " << regulator_objects.size() << endl;
		 cout << m_feeder << "size -> " << feeder_objects.size() << endl;
		 cout << m_transformer << "size -> " << transformer_objects.size() << endl;
		 cout << m_line << "size -> " << line_objects.size() << endl;
		 cout << m_evchargerdet << "size -> " << evchargerdet_objects.size() << endl;
		 */
		if (strcmp(extension, m_json.c_str()) == 0) {
			writeJsonFile(filename_billing_meter, metrics_writer_billing_meters);
			writeJsonFile(filename_house, metrics_writer_houses);
			writeJsonFile(filename_inverter, metrics_writer_inverters);
			writeJsonFile(filename_capacitor, metrics_writer_capacitors);
			writeJsonFile(filename_regulator, metrics_writer_regulators);
			writeJsonFile(filename_feeder, metrics_writer_feeders);
			writeJsonFile(filename_transformer, metrics_writer_transformers);
			writeJsonFile(filename_line, metrics_writer_lines);
			writeJsonFile(filename_evchargerdet, metrics_writer_evchargerdets);
		}
#ifdef HAVE_HDF5
		if ((strcmp(extension, m_h5.c_str()) == 0) || both) {
			hdfBillingMeterWrite(billing_meter_objects.size(), metrics_writer_billing_meters);
			hdfHouseWrite(house_objects.size(), metrics_writer_houses);
			hdfInverterWrite(inverter_objects.size() , metrics_writer_inverters);
			hdfCapacitorWrite(capacitor_objects.size(), metrics_writer_capacitors);
			hdfRegulatorWrite(regulator_objects.size(), metrics_writer_regulators);
			hdfFeederWrite(feeder_objects.size(), metrics_writer_feeders);
			hdfTransformerWrite(transformer_objects.size(), metrics_writer_transformers);
			hdfLineWrite(line_objects.size(), metrics_writer_lines);
			hdfEvChargerDetWrite(evchargerdet_objects.size(), metrics_writer_evchargerdets);
		}
#endif
		new_day = false;
		interim_cnt++;
		if ((86400 * day_cnt) < (interim_cnt * interim_length)) {
			new_day = true;
			day_cnt++;
		}
		line_cnt = 0;
	}
	next_write = t1 + interval_length;
	line_cnt++;
	return 1;
}

// Write seperate JSON files for each object
void metrics_collector_writer::writeJsonFile (char256 filename, Json::Value& metrics) {
	Json::StreamWriterBuilder builder;
	builder["indentation"] = "";
	long pos = 0;
	long offset = 1;
	ofstream out_file;

	string FileName(filename);
	if (strcmp(alternate, "yes") == 0)
		FileName.append("." + m_json);
	out_file.open (FileName, ofstream::in | ofstream::ate);
	pos = out_file.tellp();
	out_file << Json::writeString(builder, metrics);
	out_file.seekp(pos-offset);
	out_file << ", ";
	out_file.close();
	if (!both) {
		metrics.clear();
	}
}

#ifdef HAVE_HDF5
void metrics_collector_writer::hdfMetadata () {
	// defining the datatype to pass HDF55
	mtype_metadata = std::make_unique<H5::CompType>(sizeof(hMetadata));
	mtype_metadata->insertMember(m_name, HOFFSET(hMetadata, name), H5::StrType(H5::PredType::C_S1, MAX_METRIC_NAME_LENGTH));
	mtype_metadata->insertMember(m_value, HOFFSET(hMetadata, value), H5::StrType(H5::PredType::C_S1, MAX_METRIC_VALUE_LENGTH));
}

void metrics_collector_writer::hdfBillingMeter () {
	// defining the datatype to pass HDF55
	mtype_billing_meters = std::make_unique<H5::CompType>(sizeof(BillingMeter));
	mtype_billing_meters->insertMember(m_time, HOFFSET(BillingMeter, time), H5::PredType::NATIVE_INT64);
	mtype_billing_meters->insertMember(m_date, HOFFSET(BillingMeter, date), H5::StrType(H5::PredType::C_S1, MAX_METRIC_VALUE_LENGTH));
	mtype_billing_meters->insertMember(m_name, HOFFSET(BillingMeter, name), H5::StrType(H5::PredType::C_S1, MAX_METRIC_NAME_LENGTH));
	mtype_billing_meters->insertMember(m_real_power_min, HOFFSET(BillingMeter, real_power_min), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_real_power_max, HOFFSET(BillingMeter, real_power_max), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_real_power_avg, HOFFSET(BillingMeter, real_power_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_reactive_power_min, HOFFSET(BillingMeter, reactive_power_min), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_reactive_power_max, HOFFSET(BillingMeter, reactive_power_max), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_reactive_power_avg, HOFFSET(BillingMeter, reactive_power_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_real_energy, HOFFSET(BillingMeter, real_energy), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_reactive_energy, HOFFSET(BillingMeter, reactive_energy), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_bill, HOFFSET(BillingMeter, bill), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_voltage12_min, HOFFSET(BillingMeter, voltage12_min), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_voltage12_max, HOFFSET(BillingMeter, voltage12_max), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_voltage12_avg, HOFFSET(BillingMeter, voltage12_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_above_RangeA_Duration, HOFFSET(BillingMeter, above_RangeA_Duration), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_below_RangeA_Duration, HOFFSET(BillingMeter, below_RangeA_Duration), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_above_RangeB_Duration, HOFFSET(BillingMeter, above_RangeB_Duration), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_below_RangeB_Duration, HOFFSET(BillingMeter, below_RangeB_Duration), H5::PredType::NATIVE_DOUBLE);
#ifdef ALL_MTR_METRICS
	mtype_billing_meters->insertMember(m_voltage_min, HOFFSET(BillingMeter, voltage_min), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_voltage_max, HOFFSET(BillingMeter, voltage_max), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_voltage_avg, HOFFSET(BillingMeter, voltage_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_voltage_unbalance_min, HOFFSET(BillingMeter, voltage_unbalance_min), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_voltage_unbalance_max, HOFFSET(BillingMeter, voltage_unbalance_max), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_voltage_unbalance_avg, HOFFSET(BillingMeter, voltage_unbalance_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_above_RangeA_Count, HOFFSET(BillingMeter, above_RangeA_Count), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_below_RangeA_Count, HOFFSET(BillingMeter, below_RangeA_Count), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_above_RangeB_Count, HOFFSET(BillingMeter, above_RangeB_Count), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_below_RangeB_Count, HOFFSET(BillingMeter, below_RangeB_Count), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_below_10_percent_NormVol_Duration, HOFFSET(BillingMeter, below_10_percent_NormVol_Duration), H5::PredType::NATIVE_DOUBLE);
	mtype_billing_meters->insertMember(m_below_10_percent_NormVol_Count, HOFFSET(BillingMeter, below_10_percent_NormVol_Count), H5::PredType::NATIVE_DOUBLE);
#endif
}

void metrics_collector_writer::hdfHouse () {
	// defining the datatype to pass HDF55
	mtype_houses = std::make_unique<H5::CompType>(sizeof(House));
	mtype_houses->insertMember(m_time, HOFFSET(House, time), H5::PredType::NATIVE_INT64);
	mtype_houses->insertMember(m_date, HOFFSET(House, date), H5::StrType(H5::PredType::C_S1, MAX_METRIC_VALUE_LENGTH));
	mtype_houses->insertMember(m_name, HOFFSET(House, name), H5::StrType(H5::PredType::C_S1, MAX_METRIC_NAME_LENGTH));
	mtype_houses->insertMember(m_total_load_min, HOFFSET(House, total_load_min), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_total_load_max, HOFFSET(House, total_load_max), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_total_load_avg, HOFFSET(House, total_load_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_hvac_load_min, HOFFSET(House, hvac_load_min), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_hvac_load_max, HOFFSET(House, hvac_load_max), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_hvac_load_avg, HOFFSET(House, hvac_load_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_air_temperature_min, HOFFSET(House, air_temperature_min), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_air_temperature_max, HOFFSET(House, air_temperature_max), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_air_temperature_avg, HOFFSET(House, air_temperature_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_air_temperature_setpoint_cooling, HOFFSET(House, air_temperature_setpoint_cooling), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_air_temperature_setpoint_heating, HOFFSET(House, air_temperature_setpoint_heating), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_system_mode, HOFFSET(House, system_mode), H5::PredType::NATIVE_INT);
	mtype_houses->insertMember(m_waterheater_load_min, HOFFSET(House, waterheater_load_min), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_waterheater_load_max, HOFFSET(House, waterheater_load_max), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_waterheater_load_avg, HOFFSET(House, waterheater_load_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_waterheater_setpoint_min, HOFFSET(House, waterheater_setpoint_min), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_waterheater_setpoint_max, HOFFSET(House, waterheater_setpoint_max), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_waterheater_setpoint_avg, HOFFSET(House, waterheater_setpoint_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_waterheater_demand_min, HOFFSET(House, waterheater_demand_min), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_waterheater_demand_max, HOFFSET(House, waterheater_demand_max), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_waterheater_demand_avg, HOFFSET(House, waterheater_demand_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_waterheater_temp_min, HOFFSET(House, waterheater_temp_min), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_waterheater_temp_max, HOFFSET(House, waterheater_temp_max), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_waterheater_temp_avg, HOFFSET(House, waterheater_temp_avg), H5::PredType::NATIVE_DOUBLE);

	mtype_houses->insertMember(m_wh_lower_setpoint_min, HOFFSET(House, wh_lower_setpoint_min), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_wh_lower_setpoint_max, HOFFSET(House, wh_lower_setpoint_max), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_wh_lower_setpoint_avg, HOFFSET(House, wh_lower_setpoint_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_wh_upper_setpoint_min, HOFFSET(House, wh_upper_setpoint_min), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_wh_upper_setpoint_max, HOFFSET(House, wh_upper_setpoint_max), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_wh_upper_setpoint_avg, HOFFSET(House, wh_upper_setpoint_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_wh_lower_temp_min, HOFFSET(House, wh_lower_temp_min), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_wh_lower_temp_max, HOFFSET(House, wh_lower_temp_max), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_wh_lower_temp_avg, HOFFSET(House, wh_lower_temp_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_wh_upper_temp_min, HOFFSET(House, wh_upper_temp_min), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_wh_upper_temp_max, HOFFSET(House, wh_upper_temp_max), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_wh_upper_temp_avg, HOFFSET(House, wh_upper_temp_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_houses->insertMember(m_wh_lower_elem_state, HOFFSET(House, wh_lower_elem_state), H5::PredType::NATIVE_INT);
	mtype_houses->insertMember(m_wh_upper_elem_state, HOFFSET(House, wh_upper_elem_state), H5::PredType::NATIVE_INT);
}

void metrics_collector_writer::hdfInverter () {
	// defining the datatype to pass HDF55
	mtype_inverters = std::make_unique<H5::CompType>(sizeof(Inverter));
	mtype_inverters->insertMember(m_time, HOFFSET(Inverter, time), H5::PredType::NATIVE_INT64);
	mtype_inverters->insertMember(m_date, HOFFSET(Inverter, date), H5::StrType(H5::PredType::C_S1, MAX_METRIC_VALUE_LENGTH));
	mtype_inverters->insertMember(m_name, HOFFSET(Inverter, name), H5::StrType(H5::PredType::C_S1, MAX_METRIC_NAME_LENGTH));
	mtype_inverters->insertMember(m_real_power_min, HOFFSET(Inverter, real_power_min), H5::PredType::NATIVE_DOUBLE);
	mtype_inverters->insertMember(m_real_power_max, HOFFSET(Inverter, real_power_max), H5::PredType::NATIVE_DOUBLE);
	mtype_inverters->insertMember(m_real_power_avg, HOFFSET(Inverter, real_power_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_inverters->insertMember(m_reactive_power_min, HOFFSET(Inverter, reactive_power_min), H5::PredType::NATIVE_DOUBLE);
	mtype_inverters->insertMember(m_reactive_power_max, HOFFSET(Inverter, reactive_power_max), H5::PredType::NATIVE_DOUBLE);
	mtype_inverters->insertMember(m_reactive_power_avg, HOFFSET(Inverter, reactive_power_avg), H5::PredType::NATIVE_DOUBLE);
}

void metrics_collector_writer::hdfCapacitor () {
	// defining the datatype to pass HDF55
	mtype_capacitors = std::make_unique<H5::CompType>(sizeof(Capacitor));
	mtype_capacitors->insertMember(m_time, HOFFSET(Capacitor, time), H5::PredType::NATIVE_INT64);
	mtype_capacitors->insertMember(m_date, HOFFSET(Capacitor, date), H5::StrType(H5::PredType::C_S1, MAX_METRIC_VALUE_LENGTH));
	mtype_capacitors->insertMember(m_name, HOFFSET(Capacitor, name), H5::StrType(H5::PredType::C_S1, MAX_METRIC_NAME_LENGTH));
	mtype_capacitors->insertMember(m_operation_count, HOFFSET(Capacitor, operation_count), H5::PredType::NATIVE_DOUBLE);
}

void metrics_collector_writer::hdfRegulator () {
	// defining the datatype to pass HDF55
	mtype_regulators = std::make_unique<H5::CompType>(sizeof(Regulator));
	mtype_regulators->insertMember(m_time, HOFFSET(Regulator, time), H5::PredType::NATIVE_INT64);
	mtype_regulators->insertMember(m_date, HOFFSET(Regulator, date), H5::StrType(H5::PredType::C_S1, MAX_METRIC_VALUE_LENGTH));
	mtype_regulators->insertMember(m_name, HOFFSET(Regulator, name), H5::StrType(H5::PredType::C_S1, MAX_METRIC_NAME_LENGTH));
	mtype_regulators->insertMember(m_operation_count, HOFFSET(Regulator, operation_count), H5::PredType::NATIVE_DOUBLE);
}

void metrics_collector_writer::hdfFeeder () {
	// defining the datatype to pass HDF55
	mtype_feeders = std::make_unique<H5::CompType>(sizeof(Feeder));
	mtype_feeders->insertMember(m_time, HOFFSET(Feeder, time), H5::PredType::NATIVE_INT64);
	mtype_feeders->insertMember(m_date, HOFFSET(Feeder, date), H5::StrType(H5::PredType::C_S1, MAX_METRIC_VALUE_LENGTH));
	mtype_feeders->insertMember(m_name, HOFFSET(Feeder, name), H5::StrType(H5::PredType::C_S1, MAX_METRIC_NAME_LENGTH));
	mtype_feeders->insertMember(m_real_power_min, HOFFSET(Feeder, real_power_min), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_real_power_max, HOFFSET(Feeder, real_power_max), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_real_power_avg, HOFFSET(Feeder, real_power_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_real_power_median, HOFFSET(Feeder, real_power_median), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_reactive_power_min, HOFFSET(Feeder, reactive_power_min), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_reactive_power_max, HOFFSET(Feeder, reactive_power_max), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_reactive_power_avg, HOFFSET(Feeder, reactive_power_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_reactive_power_median, HOFFSET(Feeder, reactive_power_median), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_real_energy, HOFFSET(Feeder, real_energy), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_reactive_energy, HOFFSET(Feeder, reactive_energy), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_real_power_losses_min, HOFFSET(Feeder, real_power_losses_min), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_real_power_losses_max, HOFFSET(Feeder, real_power_losses_max), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_real_power_losses_avg, HOFFSET(Feeder, real_power_losses_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_real_power_losses_median, HOFFSET(Feeder, real_power_losses_median), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_reactive_power_losses_min, HOFFSET(Feeder, reactive_power_losses_min), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_reactive_power_losses_max, HOFFSET(Feeder, reactive_power_losses_max), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_reactive_power_losses_avg, HOFFSET(Feeder, reactive_power_losses_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_feeders->insertMember(m_reactive_power_losses_median, HOFFSET(Feeder, reactive_power_losses_median), H5::PredType::NATIVE_DOUBLE);
}

void metrics_collector_writer::hdfTransformer () {
	// defining the datatype to pass HDF55
	mtype_transformers = std::make_unique<H5::CompType>(sizeof(Transformer));
	mtype_transformers->insertMember(m_time, HOFFSET(Transformer, time), H5::PredType::NATIVE_INT64);
	mtype_transformers->insertMember(m_date, HOFFSET(Transformer, date), H5::StrType(H5::PredType::C_S1, MAX_METRIC_VALUE_LENGTH));
	mtype_transformers->insertMember(m_name, HOFFSET(Transformer, name), H5::StrType(H5::PredType::C_S1, MAX_METRIC_NAME_LENGTH));
	mtype_transformers->insertMember(m_trans_overload_perc, HOFFSET(Transformer, trans_overload_perc), H5::PredType::NATIVE_DOUBLE);
}

void metrics_collector_writer::hdfLine () {
	// defining the datatype to pass HDF55
	mtype_lines = std::make_unique<H5::CompType>(sizeof(Line));
	mtype_lines->insertMember(m_time, HOFFSET(Line, time), H5::PredType::NATIVE_INT64);
	mtype_lines->insertMember(m_date, HOFFSET(Line, date), H5::StrType(H5::PredType::C_S1, MAX_METRIC_VALUE_LENGTH));
	mtype_lines->insertMember(m_name, HOFFSET(Line, name), H5::StrType(H5::PredType::C_S1, MAX_METRIC_NAME_LENGTH));
	mtype_lines->insertMember(m_line_overload_perc, HOFFSET(Line, line_overload_perc), H5::PredType::NATIVE_DOUBLE);
}

void metrics_collector_writer::hdfEvChargerDet () {
	// defining the datatype to pass HDF55
	mtype_evchargerdets = std::make_unique<H5::CompType>(sizeof(EVChargerDet));
	mtype_evchargerdets->insertMember(m_time, HOFFSET(EVChargerDet, time), H5::PredType::NATIVE_INT64);
	mtype_evchargerdets->insertMember(m_date, HOFFSET(EVChargerDet, date), H5::StrType(H5::PredType::C_S1, MAX_METRIC_VALUE_LENGTH));
	mtype_evchargerdets->insertMember(m_name, HOFFSET(EVChargerDet, name), H5::StrType(H5::PredType::C_S1, MAX_METRIC_NAME_LENGTH));
	mtype_evchargerdets->insertMember(m_charge_rate_min, HOFFSET(EVChargerDet, charge_rate_min), H5::PredType::NATIVE_DOUBLE);
	mtype_evchargerdets->insertMember(m_charge_rate_max, HOFFSET(EVChargerDet, charge_rate_max), H5::PredType::NATIVE_DOUBLE);
	mtype_evchargerdets->insertMember(m_charge_rate_avg, HOFFSET(EVChargerDet, charge_rate_avg), H5::PredType::NATIVE_DOUBLE);
	mtype_evchargerdets->insertMember(m_battery_SOC_min, HOFFSET(EVChargerDet, battery_SOC_min), H5::PredType::NATIVE_DOUBLE);
	mtype_evchargerdets->insertMember(m_battery_SOC_max, HOFFSET(EVChargerDet, battery_SOC_max), H5::PredType::NATIVE_DOUBLE);
	mtype_evchargerdets->insertMember(m_battery_SOC_avg, HOFFSET(EVChargerDet, battery_SOC_avg), H5::PredType::NATIVE_DOUBLE);
}

void metrics_collector_writer::hdfWrite(char256 filename, const std::unique_ptr<H5::CompType>& mtype, void* ptr, int structKind, int size) {
	H5::Exception::dontPrint();
	cout << "Metric" << structKind;

	try {
		if (new_day) {
			// preparation of a dataset and a file.
			hsize_t dim[1] = {size};
			hsize_t maxdims[1] = {H5S_UNLIMITED};
			int rank = sizeof(dim) / sizeof(hsize_t);
			H5::DataSpace space(rank, dim, maxdims);

			// Modify dataset creation property to enable chunking
			hsize_t chunk_dims[1] = {size};
			auto plist = std::make_unique<H5::DSetCreatPropList>();
			plist->setChunk(1, chunk_dims);
			// Set ZLIB (DEFLATE) Compression using level.
			// To use SZIP compression comment out this line.
			plist->setDeflate(9);
			// Uncomment these lines to set SZIP Compression
			// unsigned szip_options_mask = H5_SZIP_NN_OPTION_MASK;
			// unsigned szip_pixels_per_block = 16;
			// plist->setSzip(szip_options_mask, szip_pixels_per_block);

			string FileName(filename);
			if (strcmp(alternate, "yes") == 0)
				FileName.append("." + m_h5);
			auto file = std::make_unique<H5::H5File>(FileName, H5F_ACC_RDWR);
			string DatasetName(m_index);
			DatasetName.append(to_string(day_cnt));

			switch (structKind) {
				case 1:
				len_billing_meters = size;
				set_billing_meters = std::make_unique<H5::DataSet>(file->createDataSet(DatasetName, *mtype, space, *plist));
				set_billing_meters->write(((std::vector <BillingMeter> *)ptr)->data(), *mtype);
				set_billing_meters->flush(H5F_SCOPE_GLOBAL);
				break;
				case 2:
				len_houses = size;
				set_houses = std::make_unique<H5::DataSet>(file->createDataSet(DatasetName, *mtype, space, *plist));
				set_houses->write(((std::vector <House> *)ptr)->data(), *mtype);
				set_houses->flush(H5F_SCOPE_GLOBAL);
				break;
				case 3:
				len_inverters = size;
				set_inverters = std::make_unique<H5::DataSet>(file->createDataSet(DatasetName, *mtype, space, *plist));
				set_inverters->write(((std::vector <Inverter> *)ptr)->data(), *mtype);
				set_inverters->flush(H5F_SCOPE_GLOBAL);
				break;
				case 4:
				len_capacitors = size;
				set_capacitors = std::make_unique<H5::DataSet>(file->createDataSet(DatasetName, *mtype, space, *plist));
				set_capacitors->write(((std::vector <Capacitor> *)ptr)->data(), *mtype);
				set_capacitors->flush(H5F_SCOPE_GLOBAL);
				break;
				case 5:
				len_regulators = size;
				set_regulators = std::make_unique<H5::DataSet>(file->createDataSet(DatasetName, *mtype, space, *plist));
				set_regulators->write(((std::vector <Regulator> *)ptr)->data(), *mtype);
				set_regulators->flush(H5F_SCOPE_GLOBAL);
				break;
				case 6:
				len_feeders = size;
				set_feeders = std::make_unique<H5::DataSet>(file->createDataSet(DatasetName, *mtype, space, *plist));
				set_feeders->write(((std::vector <Feeder> *)ptr)->data(), *mtype);
				set_feeders->flush(H5F_SCOPE_GLOBAL);
				break;
				case 7:
				len_transformers = size;
				set_transformers = std::make_unique<H5::DataSet>(file->createDataSet(DatasetName, *mtype, space, *plist));
				set_transformers->write(((std::vector <Transformer> *)ptr)->data(), *mtype);
				set_transformers->flush(H5F_SCOPE_GLOBAL);
				break;
				case 8:
				len_lines = size;
				set_lines = std::make_unique<H5::DataSet>(file->createDataSet(DatasetName, *mtype, space, *plist));
				set_lines->write(((std::vector <Line> *)ptr)->data(), *mtype);
				set_lines->flush(H5F_SCOPE_GLOBAL);
				break;
				case 9:
				len_evchargerdets = size;
				set_evchargerdets = std::make_unique<H5::DataSet>(file->createDataSet(DatasetName, *mtype, space, *plist));
				set_evchargerdets->write(((std::vector <EVChargerDet> *)ptr)->data(), *mtype);
				set_evchargerdets->flush(H5F_SCOPE_GLOBAL);
				break;
			}

		} else {
			H5::DataSet *dataset;
			hsize_t dim[1];
			hsize_t dims[1] = {size};
			hsize_t maxdims[1] = {H5S_UNLIMITED};
			hsize_t offset[1];
			switch (structKind) {
				case 1:
				offset[0] = len_billing_meters;
				dim[0] = len_billing_meters + size;
				len_billing_meters += size;
				dataset = set_billing_meters.get();
				break;
				case 2:
				offset[0] = len_houses;
				dim[0] = len_houses + size;
				len_houses += size;
				dataset = set_houses.get();
				break;
				case 3:
				offset[0] = len_inverters;
				dim[0] = len_inverters + size;
				len_inverters += size;
				dataset = set_inverters.get();
				break;
				case 4:
				offset[0] = len_capacitors;
				dim[0] = len_capacitors + size;
				len_capacitors += size;
				dataset = set_capacitors.get();
				break;
				case 5:
				offset[0] = len_regulators;
				dim[0] = len_regulators + size;
				len_regulators += size;
				dataset = set_regulators.get();
				break;
				case 6:
				offset[0] = len_feeders;
				dim[0] = len_feeders + size;
				len_feeders += size;
				dataset = set_feeders.get();
				break;
				case 7:
				offset[0] = len_transformers;
				dim[0] = len_transformers + size;
				len_transformers += size;
				dataset = set_transformers.get();
				break;
				case 8:
				offset[0] = len_lines;
				dim[0] = len_lines + size;
				len_lines += size;
				dataset = set_lines.get();
				break;
				case 9:
				offset[0] = len_evchargerdets;
				dim[0] = len_evchargerdets + size;
				len_evchargerdets += size;
				dataset = set_evchargerdets.get();
				break;
			}
			// Extend the dataset.
			if (size) {
				dataset->extend(dim);
				// Select a hyperslab in extended portion of the dataset.
				auto filespace = std::make_unique<H5::DataSpace>(dataset->getSpace ());
				filespace->selectHyperslab(H5S_SELECT_SET, dims, offset);
				// Define memory space.
				int rank = sizeof(dim) / sizeof(hsize_t);
				H5::DataSpace memspace(rank, dims, maxdims);
				switch (structKind) {
					case 1: dataset->write(((std::vector <BillingMeter> *)ptr)->data(), *mtype, memspace, *filespace); break;
					case 2: dataset->write(((std::vector <House> *)ptr)->data(), *mtype, memspace, *filespace); break;
					case 3: dataset->write(((std::vector <Inverter> *)ptr)->data(), *mtype, memspace, *filespace); break;
					case 4: dataset->write(((std::vector <Capacitor> *)ptr)->data(), *mtype, memspace, *filespace); break;
					case 5: dataset->write(((std::vector <Regulator> *)ptr)->data(), *mtype, memspace, *filespace); break;
					case 6: dataset->write(((std::vector <Feeder> *)ptr)->data(), *mtype, memspace, *filespace); break;
					case 7: dataset->write(((std::vector <Transformer> *)ptr)->data(), *mtype, memspace, *filespace); break;
					case 8: dataset->write(((std::vector <Line> *)ptr)->data(), *mtype, memspace, *filespace); break;
					case 9: dataset->write(((std::vector <EVChargerDet> *)ptr)->data(), *mtype, memspace, *filespace); break;
				}
				dataset->flush(H5F_SCOPE_GLOBAL);// - Flushes the entire virtual file
			}
		}
	}
	// catch failure caused by the H5 operations
	catch( H5::PropListIException error)
	{	error.printErrorStack();}
	catch( H5::FileIException error )
	{	error.printErrorStack();}
	catch( H5::DataSetIException error )
	{	error.printErrorStack();}
	catch( H5::DataSpaceIException error )
	{	error.printErrorStack();}
	catch( const std::exception& ex){
		std::cerr << "Unhandled general exception in HDF5(" << __func__ << "): " << ex.what() << std::endl;
	}
	cout << " Done" << endl;
}

void metrics_collector_writer::hdfMetadataWrite(Json::Value& meta, char* time_str, char256 filename) {
	H5::Exception::dontPrint();
	try {
		hMetadata tbl[meta.size() + 1];
		int idx = 0;
		strncpy(tbl[idx].name, m_starttime.c_str(), MAX_METRIC_NAME_LENGTH);
		strncpy(tbl[idx].value, time_str, MAX_METRIC_VALUE_LENGTH);
		for (auto const& id : meta.getMemberNames()) {
			idx++;
			strncpy(tbl[idx].name, id.c_str(), MAX_METRIC_NAME_LENGTH);
			string value = meta[id][m_units].asString();
			strncpy(tbl[idx].value, value.c_str(), MAX_METRIC_VALUE_LENGTH);
		}

		// preparation of a dataset and a file.
		hsize_t dim[1] = {sizeof(tbl) / sizeof(hMetadata)};
		int rank = sizeof(dim) / sizeof(hsize_t);
		H5::DataSpace space(rank, dim);

		// Modify dataset creation property to enable chunking
		hsize_t chunk_dims[1] = {idx};
		auto plist = std::make_unique<H5::DSetCreatPropList>();
		plist->setChunk(1, chunk_dims);
		// Set ZLIB (DEFLATE) Compression using level.
		// To use SZIP compression comment out this line.
		plist->setDeflate(9);
		// Uncomment these lines to set SZIP Compression
		// unsigned szip_options_mask = H5_SZIP_NN_OPTION_MASK;
		// unsigned szip_pixels_per_block = 16;
		// plist->setSzip(szip_options_mask, szip_pixels_per_block);
		string FileName(filename);
		if (strcmp(alternate, "yes") == 0)
		FileName.append("." + m_h5);
		auto file = std::make_unique<H5::H5File>(FileName, H5F_ACC_TRUNC);
		string DatasetName(m_metadata);
		auto dataset = std::make_unique<H5::DataSet>(file->createDataSet(DatasetName, *mtype_metadata, space, *plist));
		dataset->write(tbl, *mtype_metadata);
	}
	// catch failure caused by the H5 operations
	catch( H5::PropListIException error)
	{	error.printErrorStack();}
	catch( H5::FileIException error )
	{	error.printErrorStack();}
	catch( H5::DataSetIException error )
	{	error.printErrorStack();}
	catch( H5::DataSpaceIException error )
	{	error.printErrorStack();}
	catch( const std::exception& ex){
		std::cerr << "Unhandled general exception in HDF5(" << __func__ << "): " << ex.what() << std::endl;
	}
}

std::vector<std::string> sortIds(std::vector<std::string> v)
{
	std::sort(v.begin(), v.end(), [](std::string a, std::string b) {
				return std::stol(a) < std::stol(b);
			});
	return v;
};

void metrics_collector_writer::maketime(double time, char *buffer, int size) {
	DATETIME new_dt;
	memcpy(&new_dt, &dt, sizeof(DATETIME));
	gl_localtime_delta(time+startTime, &new_dt);
	gl_strtime(&new_dt, buffer, size);
}

void metrics_collector_writer::hdfBillingMeterWrite (size_t objs, Json::Value& metrics) {
	std::vector <BillingMeter> tbl;
	tbl.reserve(line_cnt*objs);
	int idx = 0;
	for (auto const& id : sortIds(metrics.getMemberNames())) {
		Json::Value name = metrics[id];
		for (auto const& uid : name.getMemberNames()) {
			tbl.push_back(BillingMeter());
			Json::Value mtr = name[uid];
			tbl[idx].time = stol(id);
			maketime(stod(id), tbl[idx].date, MAX_METRIC_VALUE_LENGTH);
			strncpy(tbl[idx].name, uid.c_str(), MAX_METRIC_NAME_LENGTH);
			tbl[idx].real_power_min = mtr[MTR_MIN_REAL_POWER].asDouble();
			tbl[idx].real_power_max = mtr[MTR_MAX_REAL_POWER].asDouble();
			tbl[idx].real_power_avg = mtr[MTR_AVG_REAL_POWER].asDouble();
			tbl[idx].reactive_power_min = mtr[MTR_MIN_REAC_POWER].asDouble();
			tbl[idx].reactive_power_max = mtr[MTR_MAX_REAC_POWER].asDouble();
			tbl[idx].reactive_power_avg = mtr[MTR_AVG_REAC_POWER].asDouble();
			tbl[idx].real_energy = mtr[MTR_REAL_ENERGY].asDouble();
			tbl[idx].reactive_energy = mtr[MTR_REAC_ENERGY].asDouble();
			tbl[idx].bill = mtr[MTR_BILL].asDouble();
			tbl[idx].voltage12_min = mtr[MTR_MIN_VLL].asDouble();
			tbl[idx].voltage12_max = mtr[MTR_MAX_VLL].asDouble();
			tbl[idx].voltage12_avg = mtr[MTR_AVG_VLL].asDouble();
			tbl[idx].above_RangeA_Duration = mtr[MTR_ABOVE_A_DUR].asDouble();
			tbl[idx].below_RangeA_Duration = mtr[MTR_BELOW_A_DUR].asDouble();
			tbl[idx].above_RangeB_Duration = mtr[MTR_ABOVE_B_DUR].asDouble();
			tbl[idx].below_RangeB_Duration = mtr[MTR_BELOW_B_DUR].asDouble();
#ifdef ALL_MTR_METRICS
			tbl[idx].voltage_min = mtr[MTR_MIN_VLN].asDouble();
			tbl[idx].voltage_max = mtr[MTR_MAX_VLN].asDouble();
			tbl[idx].voltage_avg = mtr[MTR_AVG_VLN].asDouble();
			tbl[idx].voltage_unbalance_min = mtr[MTR_MIN_VUNB].asDouble();
			tbl[idx].voltage_unbalance_max = mtr[MTR_MAX_VUNB].asDouble();
			tbl[idx].voltage_unbalance_avg = mtr[MTR_AVG_VUNB].asDouble();
			tbl[idx].above_RangeA_Count = mtr[MTR_ABOVE_A_CNT].asDouble();
			tbl[idx].below_RangeA_Count = mtr[MTR_BELOW_A_CNT].asDouble();
			tbl[idx].above_RangeB_Count = mtr[MTR_ABOVE_B_CNT].asDouble();
			tbl[idx].below_RangeB_Count = mtr[MTR_BELOW_B_CNT].asDouble();
			tbl[idx].below_10_percent_NormVol_Duration = mtr[MTR_BELOW_10_DUR].asDouble();
			tbl[idx].below_10_percent_NormVol_Count = mtr[MTR_BELOW_10_CNT].asDouble();
#endif
			idx++;
		}
	}
	hdfWrite(filename_billing_meter, mtype_billing_meters, &tbl, 1, idx);
	metrics.clear();
}

void metrics_collector_writer::hdfHouseWrite (size_t objs, Json::Value& metrics) {
	std::vector <House> tbl;
	tbl.reserve(line_cnt*objs);
	int idx = 0;
	for (auto const& id : sortIds(metrics.getMemberNames())) {
		Json::Value name = metrics[id];
		for (auto const& uid : name.getMemberNames()) {
			tbl.push_back(House());
			Json::Value mtr = name[uid];
			tbl[idx].time = stol(id);
			maketime(stod(id), tbl[idx].date, MAX_METRIC_VALUE_LENGTH);
			strncpy(tbl[idx].name, uid.c_str(), MAX_METRIC_NAME_LENGTH);;
			tbl[idx].total_load_min = mtr[HSE_MIN_TOTAL_LOAD].asDouble();
			tbl[idx].total_load_max = mtr[HSE_MAX_TOTAL_LOAD].asDouble();
			tbl[idx].total_load_avg = mtr[HSE_AVG_TOTAL_LOAD].asDouble();
			tbl[idx].hvac_load_min = mtr[HSE_MIN_HVAC_LOAD].asDouble();
			tbl[idx].hvac_load_max = mtr[HSE_MAX_HVAC_LOAD].asDouble();
			tbl[idx].hvac_load_avg = mtr[HSE_AVG_HVAC_LOAD].asDouble();
			tbl[idx].air_temperature_min = mtr[HSE_MIN_AIR_TEMP].asDouble();
			tbl[idx].air_temperature_max = mtr[HSE_MAX_AIR_TEMP].asDouble();
			tbl[idx].air_temperature_avg = mtr[HSE_AVG_AIR_TEMP].asDouble();
			tbl[idx].air_temperature_setpoint_cooling = mtr[HSE_AVG_DEV_COOLING].asDouble();
			tbl[idx].air_temperature_setpoint_heating = mtr[HSE_AVG_DEV_HEATING].asDouble();
			tbl[idx].system_mode = mtr[HSE_SYSTEM_MODE].asInt();
			tbl[idx].waterheater_load_min = mtr[HSE_SYSTEM_MODE + 1].asDouble();
			tbl[idx].waterheater_load_max = mtr[HSE_SYSTEM_MODE + 2].asDouble();
			tbl[idx].waterheater_load_avg = mtr[HSE_SYSTEM_MODE + 3].asDouble();
			tbl[idx].waterheater_setpoint_min = mtr[HSE_SYSTEM_MODE + 4].asDouble();
			tbl[idx].waterheater_setpoint_max = mtr[HSE_SYSTEM_MODE + 5].asDouble();
			tbl[idx].waterheater_setpoint_avg = mtr[HSE_SYSTEM_MODE + 6].asDouble();
			tbl[idx].waterheater_demand_min = mtr[HSE_SYSTEM_MODE + 7].asDouble();
			tbl[idx].waterheater_demand_max = mtr[HSE_SYSTEM_MODE + 8].asDouble();
			tbl[idx].waterheater_demand_avg = mtr[HSE_SYSTEM_MODE + 9].asDouble();
			tbl[idx].waterheater_temp_min = mtr[HSE_SYSTEM_MODE + 10].asDouble();
			tbl[idx].waterheater_temp_max = mtr[HSE_SYSTEM_MODE + 11].asDouble();
			tbl[idx].waterheater_temp_avg = mtr[HSE_SYSTEM_MODE + 12].asDouble();


			tbl[idx].wh_lower_setpoint_min = mtr[HSE_SYSTEM_MODE + 13].asDouble();
			tbl[idx].wh_lower_setpoint_max = mtr[HSE_SYSTEM_MODE + 14].asDouble();
			tbl[idx].wh_lower_setpoint_avg = mtr[HSE_SYSTEM_MODE + 15].asDouble();
			tbl[idx].wh_upper_setpoint_min = mtr[HSE_SYSTEM_MODE + 16].asDouble();
			tbl[idx].wh_upper_setpoint_max = mtr[HSE_SYSTEM_MODE + 17].asDouble();
			tbl[idx].wh_upper_setpoint_avg = mtr[HSE_SYSTEM_MODE + 18].asDouble();
			tbl[idx].wh_lower_temp_min = mtr[HSE_SYSTEM_MODE + 19].asDouble();
			tbl[idx].wh_lower_temp_max = mtr[HSE_SYSTEM_MODE + 20].asDouble();
			tbl[idx].wh_lower_temp_avg = mtr[HSE_SYSTEM_MODE + 21].asDouble();
			tbl[idx].wh_upper_temp_min = mtr[HSE_SYSTEM_MODE + 22].asDouble();
			tbl[idx].wh_upper_temp_max = mtr[HSE_SYSTEM_MODE + 23].asDouble();
			tbl[idx].wh_upper_temp_avg = mtr[HSE_SYSTEM_MODE + 24].asDouble();
			tbl[idx].wh_lower_elem_state = mtr[HSE_SYSTEM_MODE + 25].asInt();
			tbl[idx].wh_upper_elem_state = mtr[HSE_SYSTEM_MODE + 26].asInt();




			idx++;
		}
	}
	hdfWrite(filename_house, mtype_houses, &tbl, 2, idx);
	metrics.clear();
}

void metrics_collector_writer::hdfInverterWrite (size_t objs, Json::Value& metrics) {
	std::vector <Inverter> tbl;
	tbl.reserve(line_cnt*objs);
	int idx = 0;
	for (auto const& id : sortIds(metrics.getMemberNames())) {
		Json::Value name = metrics[id];
		for (auto const& uid : name.getMemberNames()) {
			tbl.push_back(Inverter());
			Json::Value mtr = name[uid];
			tbl[idx].time = stol(id);
			maketime(stod(id), tbl[idx].date, MAX_METRIC_VALUE_LENGTH);
			strncpy(tbl[idx].name, uid.c_str(), MAX_METRIC_NAME_LENGTH);;
			tbl[idx].real_power_min = mtr[INV_MIN_REAL_POWER].asDouble();
			tbl[idx].real_power_max = mtr[INV_MAX_REAL_POWER].asDouble();
			tbl[idx].real_power_avg = mtr[INV_AVG_REAL_POWER].asDouble();
			tbl[idx].reactive_power_min = mtr[INV_MIN_REAC_POWER].asDouble();
			tbl[idx].reactive_power_max = mtr[INV_MAX_REAC_POWER].asDouble();
			tbl[idx].reactive_power_avg = mtr[INV_AVG_REAC_POWER].asDouble();
			idx++;
		}
	}
	hdfWrite(filename_inverter, mtype_inverters, &tbl, 3, idx);
	metrics.clear();
}

void metrics_collector_writer::hdfCapacitorWrite (size_t objs, Json::Value& metrics) {
	std::vector <Capacitor> tbl;
	tbl.reserve(line_cnt*objs);
	int idx = 0;
	for (auto const& id : sortIds(metrics.getMemberNames())) {
		Json::Value name = metrics[id];
		for (auto const& uid : name.getMemberNames()) {
			tbl.push_back(Capacitor());
			Json::Value mtr = name[uid];
			tbl[idx].time = stol(id);
			maketime(stod(id), tbl[idx].date, MAX_METRIC_VALUE_LENGTH);
			strncpy(tbl[idx].name, uid.c_str(), MAX_METRIC_NAME_LENGTH);;
			tbl[idx].operation_count = mtr[CAP_OPERATION_CNT].asDouble();
			idx++;
		}
	}
	hdfWrite(filename_capacitor, mtype_capacitors, &tbl, 4, idx);
	metrics.clear();
}

void metrics_collector_writer::hdfRegulatorWrite (size_t objs, Json::Value& metrics) {
	std::vector <Regulator> tbl;
	tbl.reserve(line_cnt*objs);
	int idx = 0;
	for (auto const& id : sortIds(metrics.getMemberNames())) {
		Json::Value name = metrics[id];
		for (auto const& uid : name.getMemberNames()) {
			tbl.push_back(Regulator());
			Json::Value mtr = name[uid];
			tbl[idx].time = stol(id);
			maketime(stod(id), tbl[idx].date, MAX_METRIC_VALUE_LENGTH);
			strncpy(tbl[idx].name, uid.c_str(), MAX_METRIC_NAME_LENGTH);;
			tbl[idx].operation_count = mtr[REG_OPERATION_CNT].asDouble();
			idx++;
		}
	}
	hdfWrite(filename_regulator, mtype_regulators, &tbl, 5, idx);
	metrics.clear();
}

void metrics_collector_writer::hdfFeederWrite (size_t objs, Json::Value& metrics) {
	std::vector <Feeder> tbl;
	tbl.reserve(line_cnt*objs);
	int idx = 0;
	for (auto const& id : sortIds(metrics.getMemberNames())) {
		Json::Value name = metrics[id];
		for (auto const& uid : name.getMemberNames()) {
			tbl.push_back(Feeder());
			Json::Value mtr = name[uid];
			tbl[idx].time = stol(id);
			maketime(stod(id), tbl[idx].date, MAX_METRIC_VALUE_LENGTH);
			strncpy(tbl[idx].name, uid.c_str(), MAX_METRIC_NAME_LENGTH);;
			tbl[idx].real_power_min = mtr[FDR_MIN_REAL_POWER].asDouble();
			tbl[idx].real_power_max = mtr[FDR_MAX_REAL_POWER].asDouble();
			tbl[idx].real_power_avg = mtr[FDR_AVG_REAL_POWER].asDouble();
			tbl[idx].real_power_median = mtr[FDR_MED_REAL_POWER].asDouble();
			tbl[idx].reactive_power_min = mtr[FDR_MIN_REAC_POWER].asDouble();
			tbl[idx].reactive_power_max = mtr[FDR_MAX_REAC_POWER].asDouble();
			tbl[idx].reactive_power_avg = mtr[FDR_AVG_REAC_POWER].asDouble();
			tbl[idx].reactive_power_median = mtr[FDR_MED_REAC_POWER].asDouble();
			tbl[idx].real_energy = mtr[FDR_REAL_ENERGY].asDouble();
			tbl[idx].reactive_energy = mtr[FDR_REAC_ENERGY].asDouble();
			tbl[idx].real_power_losses_min = mtr[FDR_MIN_REAL_LOSS].asDouble();
			tbl[idx].real_power_losses_max = mtr[FDR_MAX_REAL_LOSS].asDouble();
			tbl[idx].real_power_losses_avg = mtr[FDR_AVG_REAL_LOSS].asDouble();
			tbl[idx].real_power_losses_median = mtr[FDR_MED_REAL_LOSS].asDouble();
			tbl[idx].reactive_power_losses_min = mtr[FDR_MIN_REAC_LOSS].asDouble();
			tbl[idx].reactive_power_losses_max = mtr[FDR_MAX_REAC_LOSS].asDouble();
			tbl[idx].reactive_power_losses_avg = mtr[FDR_AVG_REAC_LOSS].asDouble();
			tbl[idx].reactive_power_losses_median = mtr[FDR_MED_REAC_LOSS].asDouble();
			idx++;
		}
	}
	hdfWrite(filename_feeder, mtype_feeders, &tbl, 6, idx);
	metrics.clear();
}

void metrics_collector_writer::hdfTransformerWrite (size_t objs, Json::Value& metrics) {
	std::vector <Transformer> tbl;
	tbl.reserve(line_cnt*objs);
	int idx = 0;
	for (auto const& id : sortIds(metrics.getMemberNames())) {
		Json::Value name = metrics[id];
		for (auto const& uid : name.getMemberNames()) {
			tbl.push_back(Transformer());
			Json::Value mtr = name[uid];
			tbl[idx].time = stol(id);
			maketime(stod(id), tbl[idx].date, MAX_METRIC_VALUE_LENGTH);
			strncpy(tbl[idx].name, uid.c_str(), MAX_METRIC_NAME_LENGTH);;
			tbl[idx].trans_overload_perc = mtr[TRANS_OVERLOAD_PERC].asDouble();
			idx++;
		}
	}
	hdfWrite(filename_transformer, mtype_transformers, &tbl, 7, idx);
	metrics.clear();
}

void metrics_collector_writer::hdfLineWrite (size_t objs, Json::Value& metrics) {
	std::vector <Line> tbl;
	tbl.reserve(line_cnt*objs);
	int idx = 0;
	for (auto const& id : sortIds(metrics.getMemberNames())) {
		Json::Value name = metrics[id];
		for (auto const& uid : name.getMemberNames()) {
			tbl.push_back(Line());
			Json::Value mtr = name[uid];
			tbl[idx].time = stol(id);
			maketime(stod(id), tbl[idx].date, MAX_METRIC_VALUE_LENGTH);
			strncpy(tbl[idx].name, uid.c_str(), MAX_METRIC_NAME_LENGTH);;
			tbl[idx].line_overload_perc = mtr[LINE_OVERLOAD_PERC].asDouble();
			idx++;
		}
	}
	hdfWrite(filename_line, mtype_lines, &tbl, 8, idx);
	metrics.clear();
}

void metrics_collector_writer::hdfEvChargerDetWrite (size_t objs, Json::Value& metrics) {
	std::vector <EVChargerDet> tbl;
	tbl.reserve(line_cnt*objs);
	int idx = 0;
	for (auto const& id : sortIds(metrics.getMemberNames())) {
		Json::Value name = metrics[id];
		for (auto const& uid : name.getMemberNames()) {
			tbl.push_back(EVChargerDet());
			Json::Value mtr = name[uid];
			tbl[idx].time = stol(id);
			maketime(stod(id), tbl[idx].date, MAX_METRIC_VALUE_LENGTH);
			strncpy(tbl[idx].name, uid.c_str(), MAX_METRIC_NAME_LENGTH);;
			tbl[idx].charge_rate_min = mtr[EV_MIN_CHARGE_RATE].asDouble();
			tbl[idx].charge_rate_max = mtr[EV_MAX_CHARGE_RATE].asDouble();
			tbl[idx].charge_rate_avg = mtr[EV_AVG_CHARGE_RATE].asDouble();
			tbl[idx].battery_SOC_min = mtr[EV_MIN_BATTERY_SOC].asDouble();
			tbl[idx].battery_SOC_max = mtr[EV_MAX_BATTERY_SOC].asDouble();
			tbl[idx].battery_SOC_avg = mtr[EV_AVG_BATTERY_SOC].asDouble();
			idx++;
		}
	}
	hdfWrite(filename_evchargerdet, mtype_evchargerdets, &tbl, 9, idx);
	metrics.clear();
}
#endif // HAVE_HDF5

EXPORT int create_metrics_collector_writer(OBJECT **obj, OBJECT *parent) {
	int rv = 0;
	try {
		*obj = gl_create_object(metrics_collector_writer::oclass);
		if (*obj != NULL) {
			metrics_collector_writer *my = OBJECTDATA(*obj,
					metrics_collector_writer);
			gl_set_parent(*obj, parent);
			rv = my->create();
		}
	} catch (char *msg) {
		gl_error("create_metrics_collector_writer: %s", msg);
	} catch (const char *msg) {
		gl_error("create_metrics_collector_writer: %s", msg);
	} catch (...) {
		gl_error("create_metrics_collector_writer: unexpected exception caught");
	}
	return rv;
}

EXPORT int init_metrics_collector_writer(OBJECT *obj) {
	metrics_collector_writer *my = OBJECTDATA(obj, metrics_collector_writer);
	int rv = 0;
	try {
		rv = my->init(obj->parent);
	} catch (char *msg) {
		gl_error("init_metrics_collector_writer: %s", msg);
	} catch (const char *msg) {
		gl_error("init_metrics_collector_writer: %s", msg);
	}
	return rv;
}

EXPORT TIMESTAMP sync_metrics_collector_writer(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass) {
	metrics_collector_writer *my = OBJECTDATA(obj, metrics_collector_writer);
	TIMESTAMP rv = 0;
	try {
		switch (pass) {
		case PC_PRETOPDOWN:
			rv = TS_NEVER;
			break;
		case PC_BOTTOMUP:
			rv = TS_NEVER;
			break;
		case PC_POSTTOPDOWN:
			rv = my->postsync(obj->clock, t0);
			obj->clock = t0;
			break;
		default:
			throw "invalid pass request";
		}
	} catch (char *msg) {
		gl_error("sync_metrics_collector_writer: %s", msg);
	} catch (const char *msg) {
		gl_error("sync_metrics_collector_writer: %s", msg);
	}
	return rv;
}

EXPORT int commit_metrics_collector_writer(OBJECT *obj) {
	int rv = 0;
	metrics_collector_writer *my = OBJECTDATA(obj, metrics_collector_writer);
	try {
		rv = my->commit(obj->clock);
	} catch (char *msg) {
		gl_error("commit_metrics_collector_writer: %s", msg);
	} catch (const char *msg) {
		gl_error("commit_metrics_collector_writer: %s", msg);
	}
	return rv;
}

EXPORT int isa_metrics_collector_writer(OBJECT *obj, char *classname) {
	return OBJECTDATA(obj, metrics_collector_writer)->isa(classname);
}

// EOF

