/*
 * metrics_collector_writer.cpp
 *
 *  Created on: Jan 30, 2017
 *      Author: tang526
 */

#include "metrics_collector_writer.h"

CLASS *metrics_collector_writer::oclass = NULL;
//CLASS *metrics_collector_writer::pclass = NULL;
//metrics_collector_writer *metrics_collector_writer::defaults = NULL;


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

	firstWrite = true;

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

	// Write seperate json files for triplex_meters, inverters, houses, substation_meter:
	filename_triplex_meter = "triplex_meter_";
	strcat(filename_triplex_meter, filename);
	filename_inverter = "inverter_";
	strcat(filename_inverter, filename);
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

	// Put starting time into the metrics_writer_Output dictionary
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
	metrics_writer_Output["StartTime"] = time_str;
	metrics_writer_triplex_meters["StartTime"] = time_str;
	metrics_writer_houses["StartTime"] = time_str;
	metrics_writer_inverters["StartTime"] = time_str;
	metrics_writer_feeder_information["StartTime"] = time_str;

	return 1;
}

TIMESTAMP metrics_collector_writer::postsync(TIMESTAMP t0, TIMESTAMP t1){

	// recalculate next_time, since we know commit() will fire
	if(next_write <= t1){
		interval_write = true;
		last_write = t1;
		next_write = t1 + interval_length;
	}

	// the interval recorders have already return'ed out, earlier in the sequence.
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
	DATETIME dt;
	time_t now = time(NULL);
	int index = 0;
	// Temperary JSON Value

	Json::Value metrics_Output_temp;
	Json::Value metrics_writer_Output_time;
	Json::Value metrics_writer_Output_data;
	// metrics JSON value
	Json::Value triplex_meter_objects;
	Json::Value house_objects;
	Json::Value inverter_objects;
	Json::Value feeder_information;

	// Write Time -> represents the time from the StartTime
	metrics_writer_Output_time["Time"] = t1 - startTime; // in seconds
	int writeTime = t1 - startTime; // in seconds
	sprintf(time_str, "%d", writeTime);

	// Go through each metrics_coolector object, and check its time interval given
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
		if (strcmp(temp_metrics_collector->parent_string, "triplex_meter") == 0) {
			// It is a metrics_collector attached to a triplex_meter
			metrics_Output_temp = temp_metrics_collector->metrics_Output;
			// Put value into the dictionary
			Json::Value temp_objects;
			temp_objects["Id"] = metrics_Output_temp["Parent_id"];
			temp_objects["Parent_name"] = metrics_Output_temp["Parent_name"];
			// Real power data
			temp_objects["real_power_min"] = metrics_Output_temp["min_real_power"];
			temp_objects["real_power_max"] = metrics_Output_temp["max_real_power"];
			temp_objects["real_power_avg"] = metrics_Output_temp["avg_real_power"];
			temp_objects["real_power_median"] = metrics_Output_temp["median_real_power"];
			// Reactive power data
			temp_objects["reactive_power_min"] = metrics_Output_temp["min_reactive_power"];
			temp_objects["reactive_power_max"] = metrics_Output_temp["max_reactive_power"];
			temp_objects["reactive_power_avg"] = metrics_Output_temp["avg_reactive_power"];
			temp_objects["reactive_power_median"] = metrics_Output_temp["median_reactive_power"];
			// Energy data
			temp_objects["real_energy"] = metrics_Output_temp["real_energy"];
			temp_objects["reactive_energy"] = metrics_Output_temp["reactive_energy"];
			// Bill
			temp_objects["bill"] = metrics_Output_temp["real_energy"].asDouble() * temp_metrics_collector->price_parent / 1000; // Price unit given is $/kWh
			// Phase 1 to 2 voltage data
			temp_objects["voltage_min"] = metrics_Output_temp["min_voltage"];
			temp_objects["voltage_max"] = metrics_Output_temp["max_voltage"];
			temp_objects["voltage_avg"] = metrics_Output_temp["avg_voltage"];
			// Phase 1 to 2 average voltage data
			temp_objects["voltage12_avg_min"] = metrics_Output_temp["min_voltage_average"];
			temp_objects["voltage12_avg_max"] = metrics_Output_temp["max_voltage_average"];
			temp_objects["voltage12_avg_avg"] = metrics_Output_temp["avg_voltage_average"];
			// Voltage unbalance data
			temp_objects["voltage_unbalance_min"] = metrics_Output_temp["min_voltage_unbalance"];
			temp_objects["voltage_unbalance_max"] = metrics_Output_temp["max_voltage_unbalance"];
			temp_objects["voltage_unbalance_avg"] = metrics_Output_temp["avg_voltage_unbalance"];
			// Voltage above/below ANSI C84 A/B Range
			temp_objects["above_RangeA_Duration"] = metrics_Output_temp["above_RangeA_Duration"];
			temp_objects["above_RangeA_Count"] = metrics_Output_temp["above_RangeA_Count"];
			temp_objects["below_RangeA_Duration"] = metrics_Output_temp["below_RangeA_Duration"];
			temp_objects["below_RangeA_Count"] = metrics_Output_temp["below_RangeA_Count"];
			temp_objects["above_RangeB_Duration"] = metrics_Output_temp["above_RangeB_Duration"];
			temp_objects["above_RangeB_Count"] = metrics_Output_temp["above_RangeB_Count"];
			temp_objects["below_RangeB_Duration"] = metrics_Output_temp["below_RangeB_Duration"];
			temp_objects["below_RangeB_Count"] = metrics_Output_temp["below_RangeB_Count"];
			// Voltage below 10% of the norminal voltage rating
			temp_objects["below_10_percent_NormVol_Duration"] = metrics_Output_temp["below_10_percent_NormVol_Duration"];
			temp_objects["below_10_percent_NormVol_Count"] = metrics_Output_temp["below_10_percent_NormVol_Count"];

			// Put the temp_objects into the triplex_meter_objects
			triplex_meter_objects.append(temp_objects);

		} // End of recording metrics_collector data attached to one triplex_meter
		else if (strcmp(temp_metrics_collector->parent_string, "house") == 0) {
			// It is a metrics_collector attached to a triplex_meter
			metrics_Output_temp = temp_metrics_collector->metrics_Output;
			// Put value into the dictionary
			Json::Value temp_objects;
			// Search if there is this house object exsiting already (waterheater may come before house for example)
			Json::Value::iterator it;
			for (it = house_objects.begin(); it != house_objects.end(); ++it) {
				if ((*it)["Id"].asInt() == metrics_Output_temp["Parent_id"].asInt()) {
					// total_load data
					(*it)["house_total_load_min"] = metrics_Output_temp["min_house_total_load"];
					(*it)["house_total_load_max"] = metrics_Output_temp["max_house_total_load"];
					(*it)["house_total_load_avg"] = metrics_Output_temp["avg_house_total_load"];
					(*it)["house_total_load_median"] = metrics_Output_temp["median_house_total_load"];
					// hvac_load data
					(*it)["house_hvac_load_min"] = metrics_Output_temp["min_house_hvac_load"];
					(*it)["house_hvac_load_max"] = metrics_Output_temp["max_house_hvac_load"];
					(*it)["house_hvac_load_avg"] = metrics_Output_temp["avg_house_hvac_load"];
					(*it)["house_hvac_load_median"] = metrics_Output_temp["median_house_hvac_load"];
					// hvac_load data
					(*it)["house_air_temperatured_min"] = metrics_Output_temp["min_house_air_temperature"];
					(*it)["house_air_temperature_max"] = metrics_Output_temp["max_house_air_temperature"];
					(*it)["house_air_temperature_avg"] = metrics_Output_temp["avg_house_air_temperature"];
					(*it)["house_air_temperature_median"] = metrics_Output_temp["median_house_air_temperature"];
					break;
				}
			}

			// If there is n0 metrics_collector placed at the house object, or placed after the one for the waterheater
			if (it == house_objects.end()) {
				temp_objects["Id"] = metrics_Output_temp["Parent_id"];
				temp_objects["Parent_name"] = metrics_Output_temp["Parent_name"];
				// total_load data
				temp_objects["house_total_load_min"] = metrics_Output_temp["min_house_total_load"];
				temp_objects["house_total_load_max"] = metrics_Output_temp["max_house_total_load"];
				temp_objects["house_total_load_avg"] = metrics_Output_temp["avg_house_total_load"];
				temp_objects["house_total_load_median"] = metrics_Output_temp["median_house_total_load"];
				// hvac_load data
				temp_objects["house_hvac_load_min"] = metrics_Output_temp["min_house_hvac_load"];
				temp_objects["house_hvac_load_max"] = metrics_Output_temp["max_house_hvac_load"];
				temp_objects["house_hvac_load_avg"] = metrics_Output_temp["avg_house_hvac_load"];
				temp_objects["house_hvac_load_median"] = metrics_Output_temp["median_house_hvac_load"];
				// hvac_load data
				temp_objects["house_air_temperatured_min"] = metrics_Output_temp["min_house_air_temperature"];
				temp_objects["house_air_temperature_max"] = metrics_Output_temp["max_house_air_temperature"];
				temp_objects["house_air_temperature_avg"] = metrics_Output_temp["avg_house_air_temperature"];
				temp_objects["house_air_temperature_median"] = metrics_Output_temp["median_house_air_temperature"];
				// Put temp_objects into the house_objects list since there is not any
				house_objects.append(temp_objects);
			}


		} // End of recording metrics_collector data attached to one house
		else if (strcmp(temp_metrics_collector->parent_string, "waterheater") == 0) {
			// It is a metrics_collector attached to a triplex_meter
			metrics_Output_temp = temp_metrics_collector->metrics_Output;
			// Temporary values
			Json::Value temp_objects;
			char tempName[64]; // Create temperary name to store object key names
			// Put the temp_objects into the house_objects
			Json::Value::iterator it;
			for (it = house_objects.begin(); it != house_objects.end(); ++it) {
				if ((*it)["Id"].asInt() == metrics_Output_temp["Parent_id"].asInt()) {
					// Put values into the house object
					// min value
					sprintf(tempName, "%s_min", temp_metrics_collector->waterheaterName);
					(*it)[tempName] = metrics_Output_temp["min_waterheater_actual_load"];
					// max value
					sprintf(tempName, "%s_max", temp_metrics_collector->waterheaterName);
					(*it)[tempName] = metrics_Output_temp["max_waterheater_actual_load"];
					// avg value
					sprintf(tempName, "%s_avg", temp_metrics_collector->waterheaterName);
					(*it)[tempName] = metrics_Output_temp["avg_waterheater_actual_load"];
					// median value
					sprintf(tempName, "%s_median", temp_metrics_collector->waterheaterName);
					(*it)[tempName] = metrics_Output_temp["median_waterheater_actual_load"];
					break;
				}
			}

			// If there is n0 metrics_collector placed at the house object, or placed after the one for the waterheater
			if (it == house_objects.end()) {
				temp_objects["Id"] = metrics_Output_temp["Parent_id"];
				temp_objects["Parent_name"] = metrics_Output_temp["Parent_name"];
				// min value
				sprintf(tempName, "%s_min", temp_metrics_collector->waterheaterName);
				temp_objects[tempName] = metrics_Output_temp["min_waterheater_actual_load"];
				// max value
				sprintf(tempName, "%s_max", temp_metrics_collector->waterheaterName);
				temp_objects[tempName] = metrics_Output_temp["max_waterheater_actual_load"];
				// avg value
				sprintf(tempName, "%s_avg", temp_metrics_collector->waterheaterName);
				temp_objects[tempName] = metrics_Output_temp["avg_waterheater_actual_load"];
				// median value
				sprintf(tempName, "%s_median", temp_metrics_collector->waterheaterName);
				temp_objects[tempName] = metrics_Output_temp["median_waterheater_actual_load"];
				// Put temp_objects into the house_objects list since there is not any
				house_objects.append(temp_objects);
			}

		} // End of recording metrics_collector data attached to one waterheater
		else if (strcmp(temp_metrics_collector->parent_string, "inverter") == 0) {
			// It is a metrics_collector attached to a triplex_meter
			metrics_Output_temp = temp_metrics_collector->metrics_Output;
			// Put value into the dictionary
			Json::Value temp_objects;
			temp_objects["Id"] = metrics_Output_temp["Parent_id"];
			temp_objects["Parent_name"] = metrics_Output_temp["Parent_name"];
			// Real power data
			temp_objects["inverter_real_power_min"] = metrics_Output_temp["min_inverter_real_power"];
			temp_objects["inverter_real_power_max"] = metrics_Output_temp["max_inverter_real_power"];
			temp_objects["inverter_real_power_avg"] = metrics_Output_temp["avg_inverter_real_power"];
			temp_objects["inverter_real_power_median"] = metrics_Output_temp["median_inverter_real_power"];
			// Reactive power data
			temp_objects["inverter_reactive_power_min"] = metrics_Output_temp["min_inverter_reactive_power"];
			temp_objects["inverter_reactive_power_max"] = metrics_Output_temp["max_inverter_reactive_power"];
			temp_objects["inverter_reactive_power_avg"] = metrics_Output_temp["avg_inverter_reactive_power"];
			temp_objects["inverter_reactive_power_median"] = metrics_Output_temp["median_inverter_reactive_power"];

			// Put the temp_objects into the triplex_meter_objects
			inverter_objects.append(temp_objects);

		} // End of recording metrics_collector data attached to one inverter
		else if (strcmp(temp_metrics_collector->parent_string, "meter") == 0) {
			// It is a metrics_collector attached to a meter
			metrics_Output_temp = temp_metrics_collector->metrics_Output;
			// Gather feeder overall information, e.g. losses, power, energy
			// Real power data
			feeder_information["feeder_real_power_min"] = metrics_Output_temp["min_feeder_real_power"];
			feeder_information["feeder_real_power_max"] = metrics_Output_temp["max_feeder_real_power"];
			feeder_information["feeder_real_power_avg"] = metrics_Output_temp["avg_feeder_real_power"];
			feeder_information["feeder_real_power_median"] = metrics_Output_temp["median_feeder_real_power"];
			// Reactive power data
			feeder_information["feeder_reactive_power_min"] = metrics_Output_temp["min_feeder_reactive_power"];
			feeder_information["feeder_reactive_power_max"] = metrics_Output_temp["max_feeder_reactive_power"];
			feeder_information["feeder_reactive_power_avg"] = metrics_Output_temp["avg_feeder_reactive_power"];
			feeder_information["feeder_reactive_power_median"] = metrics_Output_temp["median_feeder_reactive_power"];
			// Energy data
			feeder_information["feeder_real_energy"] = metrics_Output_temp["real_energy"];
			feeder_information["feeder_reactive_energy"] = metrics_Output_temp["reactive_energy"];
			// Real power loss data
			feeder_information["feeder_real_power_losses_min"] = metrics_Output_temp["min_feeder_real_power_loss"];
			feeder_information["feeder_real_power_losses_max"] = metrics_Output_temp["max_feeder_real_power_loss"];
			feeder_information["feeder_real_power_losses_avg"] = metrics_Output_temp["avg_feeder_real_power_loss"];
			feeder_information["feeder_real_power_losses_median"] = metrics_Output_temp["median_feeder_real_power_loss"];
			// Reactive power loss data
			feeder_information["feeder_reactive_power_losses_min"] = metrics_Output_temp["min_feeder_reactive_power_loss"];
			feeder_information["feeder_reactive_power_losses_max"] = metrics_Output_temp["max_feeder_reactive_power_loss"];
			feeder_information["feeder_reactive_power_losses_avg"] = metrics_Output_temp["avg_feeder_reactive_power_loss"];
			feeder_information["feeder_reactive_power_losses_median"] = metrics_Output_temp["median_feeder_reactive_power_loss"];

		} // End of recording metrics_collector data attached to the swing-bus meter


		index++;
	}


	// Rewrite the metrics to be seperate 2-d ones
	metrics_writer_triplex_meters[time_str]["triplex_meters"] = triplex_meter_objects;
	metrics_writer_houses[time_str]["houses"] = house_objects;
	metrics_writer_inverters[time_str]["inverters"] = inverter_objects;
	metrics_writer_feeder_information[time_str]["feeder_information"] = feeder_information;

	if (final_write <= t1) {
		// Start write to file
		Json::StyledWriter writer;

		// Open file for writing
		ofstream out_file;

		// Write seperate JSON files for each object
		// triplex_meter
		out_file.open (filename_triplex_meter);
		out_file << writer.write(metrics_writer_triplex_meters) <<  endl;
		out_file.close();
		// house
		out_file.open (filename_house);
		out_file << writer.write(metrics_writer_houses) <<  endl;
		out_file.close();
		// inverter
		out_file.open (filename_inverter);
		out_file << writer.write(metrics_writer_inverters) <<  endl;
		out_file.close();
		// feeder information
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








