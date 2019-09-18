/*
 * metrics_collector_writer.cpp
 *
 *  Created on: Jan 30, 2017
 *      Author: tang526
 */

#include "metrics_collector_writer.h"

CLASS *metrics_collector_writer::oclass = NULL;

void new_metrics_collector_writer(MODULE *mod){
	new metrics_collector_writer(mod);
}

metrics_collector_writer::metrics_collector_writer(MODULE *mod){
	if(oclass == NULL)
	{
		oclass = gl_register_class(mod,"metrics_collector_writer",sizeof(metrics_collector_writer),PC_POSTTOPDOWN);
		if (oclass==NULL)
			throw "unable to register class metrics_collector_writer";

		if(gl_publish_variable(oclass,
			PT_char256,"filename",PADDR(filename),PT_DESCRIPTION,"the JSON formatted output file name",
			PT_double, "interval[s]", PADDR(interval_length_dbl), PT_DESCRIPTION, "Interval at which the metrics_collector_writer output is stored in JSON format",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int metrics_collector_writer::isa(char *classname){
	return (strcmp(classname, oclass->name) == 0);
}

int metrics_collector_writer::create(){
	return 1;
}

int metrics_collector_writer::init(OBJECT *parent){

	OBJECT *obj = OBJECTHDR(this);
	FILE *fn = NULL;
	int index = 0;
	char time_str[64];
	DATETIME dt;

	// check for filename
	if(0 == filename[0]){
		// if no filename, auto-generate based on ID
		sprintf(filename, "%256s-%256i-metrics_collector_output.json", oclass->name, obj->id);
		gl_warning("metrics_collector_writer::init(): no filename defined, auto-generating '%s'", filename.get_string());
		/* TROUBLESHOOT
			group_recorder requires a filename.  If none is provided, a filename will be generated
			using a combination of the classname and the core-assigned ID number.
		*/
	}

	// Write seperate json files for meters, triplex_meters, inverters, capacitors, regulators, houses, substation_meter:
	filename_billing_meter = "billing_meter_";
	strcat(filename_billing_meter, filename);
	filename_inverter = "inverter_";
	strcat(filename_inverter, filename);
	filename_capacitor = "capacitor_";
	strcat(filename_capacitor, filename);
	filename_regulator = "regulator_";
	strcat(filename_regulator, filename);
	filename_house = "house_";
	strcat(filename_house, filename);
	filename_substation = "substation_";
	strcat(filename_substation, filename);

	// Check valid metrics_collector output interval
	interval_length = (int64)(interval_length_dbl);
	if(interval_length <= 0){
		gl_error("metrics_collector_writer::init(): invalid interval of %i, must be greater than 0", interval_length);
		/* TROUBLESHOOT
			The metrics_collector interval must be a positive number of seconds.
		*/
		return 0;
	}

	// Find the metrics_collector objects
	metrics_collectors = gl_find_objects(FL_NEW,FT_CLASS,SAME,"metrics_collector",FT_END); //find all metrics_collector objects
	if(metrics_collectors == NULL){
		gl_error("No metrics_collector objects were found.");
		return 0;
		/* TROUBLESHOOT
		No metrics_collector objects were found in your .glm file. if you defined a metrics_collector_writer object, make sure you have metrics_collector objects defined also.
		*/
	}

	// Go through each metrics_coolector object, and check its time interval given
	obj = NULL;
	while(obj = gl_find_next(metrics_collectors,obj)){
		if(index >= metrics_collectors->hit_count){
			break;
		}
		// Obtain the object data
		metrics_collector *temp_metrics_collector = OBJECTDATA(obj,metrics_collector);
		if(temp_metrics_collector == NULL){
			gl_error("Unable to map object as metrics_collector object.");
			return 0;
		}
		// Obtain the object time interval and compare
		if (temp_metrics_collector->interval_length_dbl != interval_length_dbl) {
			gl_error("Currently the time interval of the metrics_collector should be the same as the metrics_collector_writer");
			return 0;
		}
	}

	// Update time variables
	startTime = gl_globalclock;
	last_write = gl_globalclock;
	next_write = gl_globalclock + interval_length;
	final_write = gl_globalstoptime;

	// Copied from recorder object
	if(0 == gl_localtime(startTime, &dt)){
		gl_error("metrics_collector_writer::init(): error when converting the starting time");
		/* TROUBLESHOOT
			Unprintable timestamp.
		 */
		return 0;
	}
	if(0 == gl_strtime(&dt, time_str, sizeof(time_str) ) ){
		gl_error("metrics_collector_writer::init(): error when writing the starting time as a string");
		/* TROUBLESHOOT
			Error printing the timestamp.
		 */
		return 0;
	}

	// Write start time for each metrics
	metrics_writer_billing_meters["StartTime"] = time_str;
	metrics_writer_houses["StartTime"] = time_str;
	metrics_writer_inverters["StartTime"] = time_str;
	metrics_writer_capacitors["StartTime"] = time_str;
	metrics_writer_regulators["StartTime"] = time_str;
	metrics_writer_feeder_information["StartTime"] = time_str;

	// Write metadata for each file; these indices MUST match assignments below
	Json::Value jsn;
	Json::Value meta;
	int idx = 0;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "Wh"; meta["real_energy"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VARh"; meta["reactive_energy"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "USD"; meta["bill"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "V"; meta["voltage12_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "V"; meta["voltage12_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "V"; meta["voltage12_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "V"; meta["voltage_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "V"; meta["voltage_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "V"; meta["voltage_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "V"; meta["voltage_unbalance_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "V"; meta["voltage_unbalance_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "V"; meta["voltage_unbalance_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "s"; meta["above_RangeA_Duration"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "";  meta["above_RangeA_Count"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "s"; meta["below_RangeA_Duration"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "";  meta["below_RangeA_Count"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "s"; meta["above_RangeB_Duration"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "";  meta["above_RangeB_Count"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "s"; meta["below_RangeB_Duration"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "";  meta["below_RangeB_Count"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "s"; meta["below_10_percent_NormVol_Duration"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "";  meta["below_10_percent_NormVol_Count"] = jsn;
	metrics_writer_billing_meters["Metadata"] = meta;
	ary_billing_meters.resize(idx);

	meta.clear();
	idx = 0;
	jsn["index"] = idx++; jsn["units"] = "kW"; meta["total_load_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "kW"; meta["total_load_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "kW"; meta["total_load_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "kW"; meta["hvac_load_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "kW"; meta["hvac_load_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "kW"; meta["hvac_load_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "degF"; meta["air_temperature_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "degF"; meta["air_temperature_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "degF"; meta["air_temperature_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "degF"; meta["air_temperature_deviation_cooling"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "degF"; meta["air_temperature_deviation_heating"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "kW"; meta["waterheater_load_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "kW"; meta["waterheater_load_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "kW"; meta["waterheater_load_avg"] = jsn;
	metrics_writer_houses["Metadata"] = meta;
	ary_houses.resize(idx);

	meta.clear();
	idx = 0;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_avg"] = jsn;
	metrics_writer_inverters["Metadata"] = meta;
	ary_inverters.resize(idx);

	meta.clear();
	idx = 0;
	jsn["index"] = idx++; jsn["units"] = ""; meta["operation_count"] = jsn;
	metrics_writer_capacitors["Metadata"] = meta;
	ary_capacitors.resize(idx);

	meta.clear();
	idx = 0;
	jsn["index"] = idx++; jsn["units"] = ""; meta["operation_count"] = jsn;
	metrics_writer_regulators["Metadata"] = meta;
	ary_regulators.resize(idx);

	meta.clear();
	idx = 0;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_median"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_median"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "Wh"; meta["real_energy"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VARh"; meta["reactive_energy"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_losses_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_losses_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_losses_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "W"; meta["real_power_losses_median"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_losses_min"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_losses_max"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_losses_avg"] = jsn;
	jsn["index"] = idx++; jsn["units"] = "VAR"; meta["reactive_power_losses_median"] = jsn;
	metrics_writer_feeder_information["Metadata"] = meta;
	ary_feeders.resize(idx);

	return 1;
}

TIMESTAMP metrics_collector_writer::postsync(TIMESTAMP t0, TIMESTAMP t1){

	// recalculate next_time, since we know commit() will fire
	if(next_write <= t1){
		interval_write = true;
		last_write = t1;
		next_write = min(t1 + interval_length, final_write);
	}

	// the interval recorders have already returned t1+interval_length, earlier in the sequence.
	return TS_NEVER;
}

int metrics_collector_writer::commit(TIMESTAMP t1){
	OBJECT *obj = OBJECTHDR(this);

	// if periodic interval, check for write
	if(interval_write){
		if(0 == write_line(t1)){
			gl_error("metrics_collector_writer::commit(): error when writing the values to JSON format");
			return 0;
		}
		interval_write = false;
	}

	return 1;
}

/**
	@return 1 on successful write, 0 on unsuccessful write, error, or when not ready
 **/
int metrics_collector_writer::write_line(TIMESTAMP t1){
	char time_str[64];
	time_t now = time(NULL);
	int index = 0;

	double *metrics;
	Json::Value metrics_writer_Output_time;
	Json::Value metrics_writer_Output_data;
	// metrics JSON value
	Json::Value billing_meter_objects;
	Json::Value house_objects;
	Json::Value inverter_objects;
	Json::Value capacitor_objects;
	Json::Value regulator_objects;
	Json::Value feeder_information;

	// Write Time -> represents the time from the StartTime
	metrics_writer_Output_time["Time"] = (Json::Int64)(t1 - startTime); // in seconds
	int writeTime = t1 - startTime; // in seconds
	sprintf(time_str, "%d", writeTime);

//	printf("write_line at %d seconds, final %ld, now %ld\n", writeTime, final_write, t1);

	// Go through each metrics_collector object, and check its time interval given
	OBJECT *obj = NULL;
	while(obj = gl_find_next(metrics_collectors,obj)){
		if(index >= metrics_collectors->hit_count){
			break;
		}

		// Obtain the object data
		metrics_collector *temp_metrics_collector = OBJECTDATA(obj,metrics_collector);
		if(temp_metrics_collector == NULL){
			gl_error("Unable to map object as metrics_collector object.");
			return 0;
		}

		// Check each metrics_collector parent type
		if ((strcmp(temp_metrics_collector->parent_string, "triplex_meter") == 0) ||
				(strcmp(temp_metrics_collector->parent_string, "meter") == 0)) {
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
			ary_billing_meters[idx++] = metrics[MTR_MIN_VLN];
			ary_billing_meters[idx++] = metrics[MTR_MAX_VLN];
			ary_billing_meters[idx++] = metrics[MTR_AVG_VLN];
			ary_billing_meters[idx++] = metrics[MTR_MIN_VUNB];
			ary_billing_meters[idx++] = metrics[MTR_MAX_VUNB];
			ary_billing_meters[idx++] = metrics[MTR_AVG_VUNB];
			ary_billing_meters[idx++] = metrics[MTR_ABOVE_A_DUR];
			ary_billing_meters[idx++] = metrics[MTR_ABOVE_A_CNT];
			ary_billing_meters[idx++] = metrics[MTR_BELOW_A_DUR];
			ary_billing_meters[idx++] = metrics[MTR_BELOW_A_CNT];
			ary_billing_meters[idx++] = metrics[MTR_ABOVE_B_DUR];
			ary_billing_meters[idx++] = metrics[MTR_ABOVE_B_CNT];
			ary_billing_meters[idx++] = metrics[MTR_BELOW_B_DUR];
			ary_billing_meters[idx++] = metrics[MTR_BELOW_B_CNT];
			ary_billing_meters[idx++] = metrics[MTR_BELOW_10_DUR];
			ary_billing_meters[idx++] = metrics[MTR_BELOW_10_CNT];
			string key = temp_metrics_collector->parent_name;
			billing_meter_objects[key] = ary_billing_meters;
		} // End of recording metrics_collector data attached to one triplex_meter or primary meter
		else if (strcmp(temp_metrics_collector->parent_string, "house") == 0) {
			metrics = temp_metrics_collector->metrics;
			string key = temp_metrics_collector->parent_name;
			// Update an existing house object if an earlier waterheater created it (TODO - look for speedup)
			if (house_objects.isMember(key)) {
				int idx = 0;
				house_objects[key][idx++] = metrics[HSE_MIN_TOTAL_LOAD];
				house_objects[key][idx++] = metrics[HSE_MAX_TOTAL_LOAD];
				house_objects[key][idx++] = metrics[HSE_AVG_TOTAL_LOAD];
				house_objects[key][idx++] = metrics[HSE_MIN_HVAC_LOAD];
				house_objects[key][idx++] = metrics[HSE_MAX_HVAC_LOAD];
				house_objects[key][idx++] = metrics[HSE_AVG_HVAC_LOAD];
				house_objects[key][idx++] = metrics[HSE_MIN_AIR_TEMP];
				house_objects[key][idx++] = metrics[HSE_MAX_AIR_TEMP];
				house_objects[key][idx++] = metrics[HSE_AVG_AIR_TEMP];
				house_objects[key][idx++] = metrics[HSE_AVG_DEV_COOLING];
				house_objects[key][idx++] = metrics[HSE_AVG_DEV_HEATING];
				// leave the earlier waterheater metric values untouched
			} else { // insert a new house with zero waterheater metric values
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
				for (int j = 0; j < WH_ARRAY_SIZE; j++) {
					ary_houses[idx++] = 0.0;
				}
				house_objects[key] = ary_houses;
			}
		} // End of recording metrics_collector data attached to one house
		else if (strcmp(temp_metrics_collector->parent_string, "waterheater") == 0) {
			metrics = temp_metrics_collector->metrics;
			string key = temp_metrics_collector->parent_name;
			if (house_objects.isMember(key)) { // already made this house
				int idx = HSE_ARRAY_SIZE; // start of the waterheater metrics - TODO speedups
				house_objects[key][idx++] = metrics[WH_MIN_ACTUAL_LOAD];
				house_objects[key][idx++] = metrics[WH_MAX_ACTUAL_LOAD];
				house_objects[key][idx++] = metrics[WH_AVG_ACTUAL_LOAD];
			} else { // make a new house, but with only the waterheater metrics non-zero
				int idx = 0;
				for (int j = 0; j < HSE_ARRAY_SIZE; j++) {
					ary_houses[idx++] = 0.0;
				}
				ary_houses[idx++] = metrics[WH_MIN_ACTUAL_LOAD]; 
				ary_houses[idx++] = metrics[WH_MAX_ACTUAL_LOAD]; 
				ary_houses[idx++] = metrics[WH_AVG_ACTUAL_LOAD]; 
				house_objects[key] = ary_houses;
			}
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
		}
		else if (strcmp(temp_metrics_collector->parent_string, "regulator") == 0) {
			metrics = temp_metrics_collector->metrics;
			int idx = 0;
			ary_regulators[idx++] = metrics[REG_OPERATION_CNT];
			string key = temp_metrics_collector->parent_name;
			regulator_objects[key] = ary_regulators;
		}
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
			feeder_information[key] = ary_feeders;
		} // End of recording metrics_collector data attached to the swing-bus meter
		index++;
	}


	// Rewrite the metrics to be separate 2-d ones
	metrics_writer_billing_meters[time_str] = billing_meter_objects;
	metrics_writer_houses[time_str] = house_objects;
	metrics_writer_inverters[time_str] = inverter_objects;
	metrics_writer_capacitors[time_str] = capacitor_objects;
	metrics_writer_regulators[time_str] = regulator_objects;
	metrics_writer_feeder_information[time_str] = feeder_information;

	if (final_write <= t1) {
		// Start write to file
		Json::StyledWriter writer;

		// Open file for writing
		ofstream out_file;

		// Write seperate JSON files for each object
		out_file.open (filename_billing_meter);
		out_file << writer.write(metrics_writer_billing_meters) <<  endl;
		out_file.close();

		out_file.open (filename_house);
		out_file << writer.write(metrics_writer_houses) <<  endl;
		out_file.close();

		out_file.open (filename_inverter);
		out_file << writer.write(metrics_writer_inverters) <<  endl;
		out_file.close();

		out_file.open (filename_capacitor);
		out_file << writer.write(metrics_writer_capacitors) <<  endl;
		out_file.close();

		out_file.open (filename_regulator);
		out_file << writer.write(metrics_writer_regulators) <<  endl;
		out_file.close();

		out_file.open (filename_substation);
		out_file << writer.write(metrics_writer_feeder_information) <<  endl;
		out_file.close();
	}

	return 1;
}


EXPORT int create_metrics_collector_writer(OBJECT **obj, OBJECT *parent){
	int rv = 0;
	try {
		*obj = gl_create_object(metrics_collector_writer::oclass);
		if(*obj != NULL){
			metrics_collector_writer *my = OBJECTDATA(*obj, metrics_collector_writer);
			gl_set_parent(*obj, parent);
			rv = my->create();
		}
	}
	catch (char *msg){
		gl_error("create_metrics_collector_writer: %s", msg);
	}
	catch (const char *msg){
		gl_error("create_metrics_collector_writer: %s", msg);
	}
	catch (...){
		gl_error("create_metrics_collector_writer: unexpected exception caught");
	}
	return rv;
}

EXPORT int init_metrics_collector_writer(OBJECT *obj){
	metrics_collector_writer *my = OBJECTDATA(obj, metrics_collector_writer);
	int rv = 0;
	try {
		rv = my->init(obj->parent);
	}
	catch (char *msg){
		gl_error("init_metrics_collector_writer: %s", msg);
	}
	catch (const char *msg){
		gl_error("init_metrics_collector_writer: %s", msg);
	}
	return rv;
}

EXPORT TIMESTAMP sync_metrics_collector_writer(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass){
	metrics_collector_writer *my = OBJECTDATA(obj, metrics_collector_writer);
	TIMESTAMP rv = 0;
	try {
		switch(pass){
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
	}
	catch(char *msg){
		gl_error("sync_metrics_collector_writer: %s", msg);
	}
	catch(const char *msg){
		gl_error("sync_metrics_collector_writer: %s", msg);
	}
	return rv;
}

EXPORT int commit_metrics_collector_writer(OBJECT *obj){
	int rv = 0;
	metrics_collector_writer *my = OBJECTDATA(obj, metrics_collector_writer);
	try {
		rv = my->commit(obj->clock);
	}
	catch (char *msg){
		gl_error("commit_metrics_collector_writer: %s", msg);
	}
	catch (const char *msg){
		gl_error("commit_metrics_collector_writer: %s", msg);
	}
	return rv;
}

EXPORT int isa_metrics_collector_writer(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj, metrics_collector_writer)->isa(classname);
}

// EOF








