/*
 * metrics_collector.cpp
 *
 *  Created on: Jan 30, 2017
 *      Author: tang526
 */

#include "metrics_collector.h"

#include <iostream>
#include <fstream>
#include <string.h>
#include <algorithm>
using namespace std;

CLASS *metrics_collector::oclass = NULL;
CLASS *metrics_collector::pclass = NULL;
metrics_collector *metrics_collector::defaults = NULL;



void new_metrics_collector(MODULE *mod){
	new metrics_collector(mod);
}

metrics_collector::metrics_collector(MODULE *mod){
	if(oclass == NULL)
		{
#ifdef _DEBUG
		gl_debug("construction metrics_collector class");
#endif
		oclass = gl_register_class(mod,"metrics_collector",sizeof(metrics_collector), PC_POSTTOPDOWN);
        if(oclass == NULL)
            GL_THROW("unable to register object class implemented by %s",__FILE__);

        if(gl_publish_variable(oclass,
			PT_double, "interval[s]", PADDR(interval_length_dbl), PT_DESCRIPTION, "Interval at which the metrics_collector output is stored in JSON format",

			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		defaults = this;
		memset(this, 0, sizeof(metrics_collector));
    }
}

int metrics_collector::isa(char *classname){
	return (strcmp(classname, oclass->name) == 0);
}

int metrics_collector::create(){

	memcpy(this, defaults, sizeof(metrics_collector));

	// Give default values to parameters related to triplex_meter
	real_power_array = NULL;
	reactive_power_array = NULL;
	voltage_mag_array = NULL;
	voltage_average_mag_array = NULL;
	voltage_unbalance_array = NULL;

	// Interval related
	interval_length = -1;
	perform_average_time = 0;
	last_update_time = 0;
	curr_avg_pos_index = 0;
	interval_length_dbl=-1.0;


	return 1;
}

int metrics_collector::init(OBJECT *parent){

	OBJECT *obj = OBJECTHDR(this);
	Json::Value paramters;

	int temp = strcmp(parent->oclass->name,"triplex_meter") != 0;
	// Find parent, if not defined, or if the parent is not a triplex_meter/house/inverter, throw an exception
	if	((parent != NULL && strcmp(parent->oclass->name,"triplex_meter") != 0) && (parent != NULL && strcmp(parent->oclass->name,"house") != 0) && (parent != NULL && strcmp(parent->oclass->name,"waterheater") != 0) && (parent != NULL && strcmp(parent->oclass->name,"inverter") != 0) && (parent != NULL && strcmp(parent->oclass->name,"meter") != 0))
	{
		gl_error("metrics_collector must have a triplex meter or house or waterheater or inverter or Swing-bus meter as it's parent");
		/*  TROUBLESHOOT
		Check the parent object of the metrics_collector. The metrics_collector is only able to be childed via a
		triplex meter or a house or a waterheater or an inverter or a swing-bus meter when connecting into powerflow systems.
		*/
		return 0;
	}

	// If its parent is a triplex_meter
	if (parent!=NULL && gl_object_isa(parent,"triplex_meter"))
	{
		parent_string = "triplex_meter";

		// Create the JSON dictionary for the outputs
		metrics_Output["Parent_id"] = parent->id;
		if (parent->name != NULL) {
			metrics_Output["Parent_name"] = parent->name;
		}
		else {
			metrics_Output["Parent_name"] = "No name given";
		}

	}
	// If its parent is a house
	else if (parent!=NULL && gl_object_isa(parent,"house"))
	{
		parent_string = "house";

		// Create the JSON dictionary for the outputs
		metrics_Output["Parent_id"] = parent->id;
		if (parent->name != NULL) {
			metrics_Output["Parent_name"] = parent->name;
		}
		else {
			metrics_Output["Parent_name"] = "No name given";
		}
	}
	// If its parent is a waterheater
	else if (parent!=NULL && gl_object_isa(parent,"waterheater"))
	{
		parent_string = "waterheater";

		// Create the JSON dictionary for the outputs
		metrics_Output["Parent_id"] = parent->parent->id; // For waterheater, write its outputs inside the house object
		if (parent->parent->name != NULL) {
			metrics_Output["Parent_name"] = parent->parent->name;
		}
		else {
			metrics_Output["Parent_name"] = "No name given";
		}

		// Get the name of the waterheater for actual load
		char tname[32];
		sprintf(tname, "%i", parent->id);
		char *namestr = (parent->name ? parent->name : tname);
		sprintf(waterheaterName, "waterheater_%s_actual_load", namestr);

	}
	// If its parent is a inverter
	else if (parent!=NULL && gl_object_isa(parent,"inverter"))
	{
		parent_string = "inverter";

		// Create the JSON dictionary for the outputs
		metrics_Output["Parent_id"] = parent->id;
		if (parent->name != NULL) {
			metrics_Output["Parent_name"] = parent->name;
		}
		else {
			metrics_Output["Parent_name"] = "No name given";
		}
	}
	// If its parent is a meter - this is only allowed for the Swing-bus type meter
	else if (parent != NULL && gl_object_isa(parent,"meter"))
	{
		parent_string = "meter";

		// Obtain the parent data
		//Map the flag
		PROPERTY *pval = gl_get_property(parent,"bustype");
		//Check it
		if ((pval==NULL) || (pval->ptype!=PT_enumeration))
		{
			GL_THROW("metrics_collector:%s failed to map bustype variable from %s",obj->name?obj->name:"unnamed",parent->name?parent->name:"unnamed");
			/*  TROUBLESHOOT
			While attempting to set up the deltamode interfaces and calculations with powerflow, the required interface could not be mapped.
			Please check your GLM and try again.  If the error persists, please submit a trac ticket with your code.
			*/
		}
		//Map to the intermediate
		enumeration *meter_bustype = (enumeration*)GETADDR(parent,pval);
		// Check if the parent meter is a swing bus (2) or not
		if (*meter_bustype != 2) {
			gl_error("If a metrics_collector is attached to a meter, it must be a SWING bus meter");
			return 0;
		}

		// In this work, only when a metrics_collector is attached to a swing-bus meter, the feeder losses are recorded
		// Find all the link objects for collecting loss values
		link_objects = gl_find_objects(FL_NEW,FT_MODULE,SAME,"powerflow",FT_END); //find all link objects
		if(link_objects == NULL){
			gl_warning("No link objects were found.");
			/* TROUBLESHOOT
			No link objects were found in your .glm file. In this way, there is no losses calculated.
			*/
		}
	}

	// Check valid metrics_collector output interval
	interval_length = (int64)(interval_length_dbl);
	if(interval_length <= 0){
		gl_error("metrics_collector::init(): invalid interval of %i, must be greater than 0", interval_length);
		/* TROUBLESHOOT
			The metrics_collector interval must be a positive number of seconds.
		*/
		return 0;
	}

	// Allocate the arrays based on the parent type
	if (strcmp(parent_string, "triplex_meter") == 0) {
		// Allocate real power array
		real_power_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (real_power_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated real power array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate reactive power array
		reactive_power_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (reactive_power_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated reactive power array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate phase 1-2 voltage array
		voltage_mag_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (voltage_mag_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated phase 1-2 voltage array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate average voltage array
		voltage_average_mag_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (voltage_average_mag_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated average voltage array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate voltage unbalance array
		voltage_unbalance_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (voltage_unbalance_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated voltage unbalance array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Initialize the array
		for (curr_avg_pos_index=0; curr_avg_pos_index<interval_length; curr_avg_pos_index++)
		{
			real_power_array[curr_avg_pos_index] = 0.0;
			reactive_power_array[curr_avg_pos_index] = 0.0;
			voltage_mag_array[curr_avg_pos_index] = 0.0;
			voltage_average_mag_array[curr_avg_pos_index] = 0.0;
			voltage_unbalance_array[curr_avg_pos_index] = 0.0;
		}

	}
	// If parent is house
	else if (strcmp(parent_string, "house") == 0) {
		// Allocate total_load array
		total_load_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (total_load_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated total_load array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate hvac_load array
		hvac_load_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (hvac_load_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated hvac_load array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate hvac_load array
		air_temperature_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (air_temperature_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated air_temperature array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Initialize the array
		for (curr_avg_pos_index=0; curr_avg_pos_index<interval_length; curr_avg_pos_index++)
		{
			total_load_array[curr_avg_pos_index] = 0.0;
			hvac_load_array[curr_avg_pos_index] = 0.0;
			air_temperature_array[curr_avg_pos_index] = 0.0;
		}
	}
	// If parent is waterheater
	else if (strcmp(parent_string, "waterheater") == 0) {
		// Allocate hvac_load array
		actual_load_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (actual_load_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated actual_load array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}
	}
	// If parent is inverter
	else if (strcmp(parent_string, "inverter") == 0) {
		// Allocate real power array
		real_power_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (real_power_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated real power array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate reactive power array
		reactive_power_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (reactive_power_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated reactive power array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}
	}
	// If parent is meter
	else if (strcmp(parent_string, "meter") == 0) {
		// Allocate real power array
		real_power_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (real_power_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated real power array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate reactive power array
		reactive_power_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (reactive_power_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated reactive power array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}
		// Allocate real power loss array
		real_power_loss_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (real_power_loss_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated real power loss array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate reactive power loss array
		reactive_power_loss_array = (double *)gl_malloc(interval_length*sizeof(double));
		// Check
		if (reactive_power_loss_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocated reactive power loss array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}
	}
	// else not possible come to this step
	else {
		gl_error("metrics_collector must have a triplex meter or house or waterheater or inverter or swing-bus meter as it's parent");
		/*  TROUBLESHOOT
		Check the parent object of the metrics_collector. The metrics_collector is only able to be childed via a
		triplex meter or a house or an inverter when connecting into powerflow systems.
		*/
		return 0;
	}


	// Initialize tracking variables
	curr_avg_pos_index = 0;

	// Update time variables
	last_write = gl_globalclock;
	next_write = gl_globalclock + interval_length;

	return 1;
}

TIMESTAMP metrics_collector::postsync(TIMESTAMP t0, TIMESTAMP t1){

	// recalculate next_time, since we know commit() will fire
	if(next_write <= t1){
		interval_write = true;
		last_write = t1;
		next_write = t1 + interval_length;
	}

//	// Return to the next write interval
//	return next_write;
	// Force to return to 1 sec later
	return t1+1;
}

int metrics_collector::commit(TIMESTAMP t1){
	OBJECT *obj = OBJECTHDR(this);

	// if periodic interval, check for write
	if(interval_write){
		if (curr_avg_pos_index != 0) {
			gl_error("metrics_collector::commit(): error when trying to write accumulated output during one interval, however curr_avg_pos_index indicates values are not completely obtained");
			return 0;
		}
		if(0 == write_line(t1, obj)){
			gl_error("metrics_collector::commit(): error when writing the values to JSON format");
			return 0;
		}
		interval_write = false;
	}

	// read the parameters for each time step
	if(0 == read_line(obj)){
		gl_error("metrics_collector::commit(): error when reading the values");
		return 0;
	}

	return 1;
}

/**
	@return 0 on failure, 1 on success
 **/
int metrics_collector::read_line(OBJECT *obj){

	// Check curr_avg_pos_index value
	if (curr_avg_pos_index < 0 || curr_avg_pos_index >= interval_length) {
		gl_error("metrics_collector::curr_avg_pos_index value exceeds the limit");
		return 0;
	}

	// If parent is triplex_meter
	if (strcmp(parent_string, "triplex_meter") == 0) {

		// Get power values
		double realPower = *gl_get_double_by_name(obj->parent, "measured_real_power");
		real_power_array[curr_avg_pos_index] = realPower;
		double reactivePower = *gl_get_double_by_name(obj->parent, "measured_reactive_power");
		reactive_power_array[curr_avg_pos_index] = reactivePower;

		// Get bill value
		price_parent = *gl_get_double_by_name(obj->parent, "price");

		// Get voltage values
		double v1 = *gl_get_double_by_name(obj->parent, "voltage_1"); // property of triplex_node: bus voltage, phase 1 to ground
		double v2 = *gl_get_double_by_name(obj->parent, "voltage_2"); // property of triplex_node: bus voltage, phase 2 to ground
		double v12 = *gl_get_double_by_name(obj->parent, "voltage_12"); // property of triplex_node: bus voltage, phase 1 to 2

		voltage_mag_array[curr_avg_pos_index] = fabs(v12);
		voltage_average_mag_array[curr_avg_pos_index] = fabs(v12/2);
		voltage_unbalance_array[curr_avg_pos_index] = fabs((v1 - v2)/(v12/2));

		// Update index value
		curr_avg_pos_index++;
		if (curr_avg_pos_index == interval_length) {
			curr_avg_pos_index = 0; // Reset index to 0 for the next interval records
		}
	}
	// If parent is house
	else if (strcmp(parent_string, "house") == 0) {
		// Get load values
		double totalload = *gl_get_double_by_name(obj->parent, "total_load");
		total_load_array[curr_avg_pos_index] = totalload;
		double hvacload = *gl_get_double_by_name(obj->parent, "hvac_load");
		hvac_load_array[curr_avg_pos_index] = hvacload;
		// Get air temperature values
		double airTemperature = *gl_get_double_by_name(obj->parent, "air_temperature");
		air_temperature_array[curr_avg_pos_index] = airTemperature;

		// Update index value
		curr_avg_pos_index++;
		if (curr_avg_pos_index == interval_length) {
			curr_avg_pos_index = 0; // Reset index to 0 for the next interval records
		}
	}
	// If parent is waterheater
	else if (strcmp(parent_string, "waterheater") == 0) {
		// Get load values
		double actualload = *gl_get_double_by_name(obj->parent, "actual_load");
		actual_load_array[curr_avg_pos_index] = actualload;
		// Update index value
		curr_avg_pos_index++;
		if (curr_avg_pos_index == interval_length) {
			curr_avg_pos_index = 0; // Reset index to 0 for the next interval records
		}

	}
	// If parent is inverter
	else if (strcmp(parent_string, "inverter") == 0) {
		// Get VA_Out values
		complex VAOut = *gl_get_complex_by_name(obj->parent, "VA_Out");
		real_power_array[curr_avg_pos_index] = (double)VAOut.Re();
		reactive_power_array[curr_avg_pos_index] = (double)VAOut.Im();
		// Update index value
		curr_avg_pos_index++;
		if (curr_avg_pos_index == interval_length) {
			curr_avg_pos_index = 0; // Reset index to 0 for the next interval records
		}

	}
	// If parent is meter
	else if (strcmp(parent_string, "meter") == 0) {
		// Get VAfeeder values
		complex VAfeeder = *gl_get_complex_by_name(obj->parent, "measured_power");
		real_power_array[curr_avg_pos_index] = (double)VAfeeder.Re();
		reactive_power_array[curr_avg_pos_index] = (double)VAfeeder.Im();
		// Update index value
		curr_avg_pos_index++;
		if (curr_avg_pos_index == interval_length) {
			curr_avg_pos_index = 0; // Reset index to 0 for the next interval records
		}

		// Get feeder loss values
		// Losses calculation
		int index = 0;
		obj = NULL;
		complex lossesSum = 0.0;
		while(obj = gl_find_next(link_objects,obj)){
			if(index >= link_objects->hit_count){
				break;
			}

			char * oclassName = obj->oclass->name;
			if (strcmp(oclassName, "overhead_line") == 0 || strcmp(oclassName, "underground_line") == 0 || strcmp(oclassName, "triplex_line") == 0 || strcmp(oclassName, "transformer") == 0 || strcmp(oclassName, "regulator") == 0 || strcmp(oclassName, "switch") == 0 || strcmp(oclassName, "fuse") == 0) {

				// Obtain the link data
				link_object *one_link = OBJECTDATA(obj,link_object);
				if(one_link == NULL){
					gl_error("Unable to map the object as link.");
					return 0;
				}

				// Calculate loss
				complex loss = one_link->power_loss;

				// real power losses should be positive
				if (loss.Re() < 0) {
					loss.Re() = -loss.Re();
				}

				lossesSum += loss;

			}

			// Increase the index value
			index++;
		}
		// Put the loss value into the array
		real_power_loss_array[curr_avg_pos_index] = (double)lossesSum.Re();
		reactive_power_loss_array[curr_avg_pos_index] = (double)lossesSum.Im();

	}
	// else not possible come to this step
	else {
		gl_error("metrics_collector must have a triplex meter or house or waterheater or inverter or swing-bus meter as it's parent");
		/*  TROUBLESHOOT
		Check the parent object of the metrics_collector. The metrics_collector is only able to be childed via a
		triplex meter or a house or a waterheater or an inverter or a swing-bus meter when connecting into powerflow systems.
		*/
		return 0;
	}

	return 1;
}


/**
	@return 1 on successful write, 0 on unsuccessful write, error, or when not ready
 **/

int metrics_collector::write_line(TIMESTAMP t1, OBJECT *obj){
	// In the metrics_collector object, values are rearranged in write_line into dictionary
	// Writing to JSON output file is executed in metrics_collector_writer object
	char time_str[64];
	DATETIME dt;

	if (strcmp(parent_string, "triplex_meter") == 0) {
		// Rearranging the arrays of data, and put into the dictionary
		// Real power data
		metrics_Output["min_real_power"] = findMin(real_power_array, interval_length);
		metrics_Output["max_real_power"] = findMax(real_power_array, interval_length);
		metrics_Output["avg_real_power"] = findAverage(real_power_array, interval_length);
		metrics_Output["median_real_power"] = findMedian(real_power_array, interval_length);

		// Reactive power data
		metrics_Output["min_reactive_power"] = findMin(reactive_power_array, interval_length);
		metrics_Output["max_reactive_power"] = findMax(reactive_power_array, interval_length);
		metrics_Output["avg_reactive_power"] = findAverage(reactive_power_array, interval_length);
		metrics_Output["median_reactive_power"] = findMedian(reactive_power_array, interval_length);

		// Energy data
		metrics_Output["real_energy"] = findAverage(real_power_array, interval_length) * interval_write / 3600;
		metrics_Output["reactive_energy"] = findAverage(reactive_power_array, interval_length) * interval_write / 3600;

		// Bill

		// Phase 1 to 2 voltage data
		metrics_Output["min_voltage"] = findMin(voltage_mag_array, interval_length);
		metrics_Output["max_voltage"] = findMax(voltage_mag_array, interval_length);
		metrics_Output["avg_voltage"] = findAverage(voltage_mag_array, interval_length);

		// Phase 1 to 2 average voltage data
		metrics_Output["min_voltage_average"] = findMin(voltage_average_mag_array, interval_length);
		metrics_Output["max_voltage_average"] = findMax(voltage_average_mag_array, interval_length);
		metrics_Output["avg_voltage_average"] = findAverage(voltage_average_mag_array, interval_length);

		// Voltage unbalance data
		metrics_Output["min_voltage_unbalance"] = findMin(voltage_unbalance_array, interval_length);
		metrics_Output["max_voltage_unbalance"] = findMax(voltage_unbalance_array, interval_length);
		metrics_Output["avg_voltage_unbalance"] = findAverage(voltage_unbalance_array, interval_length);

		// Voltage above/below ANSI C84 A/B Range
		double normVol = *gl_get_double_by_name(obj->parent, "nominal_voltage");
		double aboveRangeA = normVol* 1.05;
		double belowRangeA = normVol* 0.95;
		double aboveRangeB = normVol* 1.058;
		double belowRangeB = normVol* 0.917;
		// Voltage above Range A
		struct vol_violation vol_Vio = findOutLimit(voltage_mag_array, true, aboveRangeA, interval_length);
		metrics_Output["above_RangeA_Duration"] = vol_Vio.durationViolation;
		metrics_Output["above_RangeA_Count"] = vol_Vio.countViolation;
		// Voltage below Range A
		vol_Vio = findOutLimit(voltage_mag_array, false, belowRangeA, interval_length);
		metrics_Output["below_RangeA_Duration"] = vol_Vio.durationViolation;
		metrics_Output["below_RangeA_Count"] = vol_Vio.countViolation;
		// Voltage above Range B
		vol_Vio = findOutLimit(voltage_mag_array, true, aboveRangeB, interval_length);
		metrics_Output["above_RangeB_Duration"] = vol_Vio.durationViolation;
		metrics_Output["above_RangeB_Count"] = vol_Vio.countViolation;
		// Voltage below Range B
		vol_Vio = findOutLimit(voltage_mag_array, false, belowRangeB, interval_length);
		metrics_Output["below_RangeB_Duration"] = vol_Vio.durationViolation;
		metrics_Output["below_RangeB_Count"] = vol_Vio.countViolation;

		// Voltage below 10% of the norminal voltage rating
		double belowNorVol10 = normVol * 0.1;
		vol_Vio = findOutLimit(voltage_mag_array, false, belowNorVol10, interval_length);
		metrics_Output["below_10_percent_NormVol_Duration"] = vol_Vio.durationViolation;
		metrics_Output["below_10_percent_NormVol_Count"] = vol_Vio.countViolation;

	}
	// If parent is house
	else if (strcmp(parent_string, "house") == 0) {
		// Rearranging the arrays of data, and put into the dictionary
		// total_load data
		metrics_Output["min_house_total_load"] = findMin(total_load_array, interval_length);
		metrics_Output["max_house_total_load"] = findMax(total_load_array, interval_length);
		metrics_Output["avg_house_total_load"] = findAverage(total_load_array, interval_length);
		metrics_Output["median_house_total_load"] = findMedian(total_load_array, interval_length);

		// hvac_load data
		metrics_Output["min_house_hvac_load"] = findMin(hvac_load_array, interval_length);
		metrics_Output["max_house_hvac_load"] = findMax(hvac_load_array, interval_length);
		metrics_Output["avg_house_hvac_load"] = findAverage(hvac_load_array, interval_length);
		metrics_Output["median_house_hvac_load"] = findMedian(hvac_load_array, interval_length);

		// air_temperature data
		metrics_Output["min_house_air_temperature"] = findMin(air_temperature_array, interval_length);
		metrics_Output["max_house_air_temperature"] = findMax(air_temperature_array, interval_length);
		metrics_Output["avg_house_air_temperature"] = findAverage(air_temperature_array, interval_length);
		metrics_Output["median_house_air_temperature"] = findMedian(air_temperature_array, interval_length);

	}
	// If parent is waterheater
	else if (strcmp(parent_string, "waterheater") == 0) {
		// Rearranging the arrays of data, and put into the dictionary
		// actual_load data
		metrics_Output["min_waterheater_actual_load"] = findMin(actual_load_array, interval_length);
		metrics_Output["max_waterheater_actual_load"] = findMax(actual_load_array, interval_length);
		metrics_Output["avg_waterheater_actual_load"] = findAverage(actual_load_array, interval_length);
		metrics_Output["median_waterheater_actual_load"] = findMedian(actual_load_array, interval_length);

	}
	// If parent is inverter
	else if (strcmp(parent_string, "inverter") == 0) {
		// Rearranging the arrays of data, and put into the dictionary
		// real power data
		metrics_Output["min_inverter_real_power"] = findMin(real_power_array, interval_length);
		metrics_Output["max_inverter_real_power"] = findMax(real_power_array, interval_length);
		metrics_Output["avg_inverter_real_power"] = findAverage(real_power_array, interval_length);
		metrics_Output["median_inverter_real_power"] = findMedian(real_power_array, interval_length);
		// Reactive power data
		metrics_Output["min_inverter_reactive_power"] = findMin(reactive_power_array, interval_length);
		metrics_Output["max_inverter_reactive_power"] = findMax(reactive_power_array, interval_length);
		metrics_Output["avg_inverter_reactive_power"] = findAverage(reactive_power_array, interval_length);
		metrics_Output["median_inverter_reactive_power"] = findMedian(reactive_power_array, interval_length);

	}
	// If parent is meter
	else if (strcmp(parent_string, "meter") == 0) {
		// Rearranging the arrays of data, and put into the dictionary
		// real power data
		metrics_Output["min_feeder_real_power"] = findMin(real_power_array, interval_length);
		metrics_Output["max_feeder_real_power"] = findMax(real_power_array, interval_length);
		metrics_Output["avg_feeder_real_power"] = findAverage(real_power_array, interval_length);
		metrics_Output["median_feeder_real_power"] = findMedian(real_power_array, interval_length);
		// Reactive power data
		metrics_Output["min_feeder_reactive_power"] = findMin(reactive_power_array, interval_length);
		metrics_Output["max_feeder_reactive_power"] = findMax(reactive_power_array, interval_length);
		metrics_Output["avg_feeder_reactive_power"] = findAverage(reactive_power_array, interval_length);
		metrics_Output["median_feeder_reactive_power"] = findMedian(reactive_power_array, interval_length);
		// Energy data
		metrics_Output["real_energy"] = findAverage(real_power_array, interval_length) * interval_write / 3600;
		metrics_Output["reactive_energy"] = findAverage(reactive_power_array, interval_length) * interval_write / 3600;
		// real power loss data
		metrics_Output["min_feeder_real_power_loss"] = findMin(real_power_loss_array, interval_length);
		metrics_Output["max_feeder_real_power_loss"] = findMax(real_power_loss_array, interval_length);
		metrics_Output["avg_feeder_real_power_loss"] = findAverage(real_power_loss_array, interval_length);
		metrics_Output["median_feeder_real_power_loss"] = findMedian(real_power_loss_array, interval_length);
		// Reactive power loss data
		metrics_Output["min_feeder_reactive_power_loss"] = findMin(reactive_power_loss_array, interval_length);
		metrics_Output["max_feeder_reactive_power_loss"] = findMax(reactive_power_loss_array, interval_length);
		metrics_Output["avg_feeder_reactive_power_loss"] = findAverage(reactive_power_loss_array, interval_length);
		metrics_Output["median_feeder_reactive_power_loss"] = findMedian(reactive_power_loss_array, interval_length);

	}

	return 1;
}

double metrics_collector::findMax(double array[], int length) {
	double max = array[0];

	for (int i = 1; i < length; i++) {
		if (array[i] > max) {
			max = array[i];
		}
	}

	return max;
}

double metrics_collector::findMin(double array[], int length) {
	double min = array[0];

	for (int i = 1; i < length; i++) {
		if (array[i] < min) {
			min = array[i];
		}
	}

	return min;
}

double metrics_collector::findAverage(double array[], int length) {
	double avg = 0;

	for (int i = 0; i < length; i++) {
		avg += array[i];
	}

	avg = double(avg) / length;

	return avg;
}

double metrics_collector::findMedian(double array[], int length) {
	double median;

	std::sort(&array[0], &array[length]);

	median = length % 2 ? array[length / 2] : (array[length / 2 - 1] + array[length / 2]) / 2;

	return median;
}

vol_violation metrics_collector::findOutLimit(double array[], bool checkAbove, double limitVal, int length) {
	struct vol_violation result;
	int count = 0;
	double durationTime = 0.0;
	int pastVal;

	// Check the first index value
	if (array[0] > limitVal) {
		pastVal = 1;
	}
	else if (array[0] == limitVal) {
		pastVal = 0;
		count++;
	}
	else {
		pastVal = -1;
	}

	// Check length
	if (length <= 1) {
		result.countViolation = 0;
		result.durationViolation = 0.0;
		return result;
	}

	// Loop through the array serachinig for voltage violation time steps
	for (int i = 1; i < length; i++) {
		// If the value is out of the limit
		if (array[i] > limitVal) {
			// If at last time step, the value was out of the limit also -> record duration
			if (pastVal == 1) {
				durationTime++;  // count the duration
			}
			// If at last time step, the value was at the limit -> record duration
			else if (pastVal == 0) {
				durationTime++;
				pastVal = 1;
			}
			// If at last time step, the value was within the limit -> record duration
			else {
				count++; // Went across the limit line
				durationTime += 0.5; // Assume that the duration above the limit between the last and current time step is half time interval
				pastVal = 1;
			}
		}
		else if (array[i] == limitVal) {
			// If at last time step, the value was out of the limit -> record the duration time
			if (pastVal == 1) {
				durationTime++;  // count the duration
				count++;
				pastVal = 0;
			}
			// If at last time step, the value was at the limit also -> does not count anything
			else if (pastVal == 0) {
				// Do nothing
			}
			// If at last time step, the value was within the limit -> count
			else {
				count++; // Went across the limit line
				pastVal = 0;
			}
		}
		else {
			// If at last time step, the value was out of the limit -> record the itersection and the duration time
			if (pastVal == 1) {
				durationTime += 0.5;  //Assume that the duration above the limit between the last and current time step is half time interval
				count++;
				pastVal = -1;
			}
			// If at last time step, the value was at the limit -> do nothing
			else if (pastVal == 0) {
				pastVal = -1;
			}
			// If at last time step, the value was within the limit also -> do nothing
			else {
				// Do nothing
			}
		}
	}

	// Update the time steps array with the count size
	if (checkAbove) {
		result.countViolation = count;
		result.durationViolation = durationTime;
	}
	else {
		result.countViolation = count;
		result.durationViolation = (length - 1) - durationTime;
	}

	return result;
}

EXPORT int create_metrics_collector(OBJECT **obj, OBJECT *parent){
	int rv = 0;
	try {
		*obj = gl_create_object(metrics_collector::oclass);
		if(*obj != NULL){
			metrics_collector *my = OBJECTDATA(*obj, metrics_collector);
			gl_set_parent(*obj, parent);
			rv = my->create();
		}
	}
	catch (char *msg){
		gl_error("create_metrics_collector: %s", msg);
	}
	catch (const char *msg){
		gl_error("create_metrics_collector: %s", msg);
	}
	catch (...){
		gl_error("create_metrics_collector: unexpected exception caught");
	}
	return rv;
}

EXPORT int init_metrics_collector(OBJECT *obj){
	metrics_collector *my = OBJECTDATA(obj, metrics_collector);
	int rv = 0;
	try {
		rv = my->init(obj->parent);
	}
	catch (char *msg){
		gl_error("init_metrics_collector: %s", msg);
	}
	catch (const char *msg){
		gl_error("init_metrics_collector: %s", msg);
	}
	return rv;
}

EXPORT TIMESTAMP sync_metrics_collector(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass){
	metrics_collector *my = OBJECTDATA(obj, metrics_collector);
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
		gl_error("sync_metrics_collector: %s", msg);
	}
	catch(const char *msg){
		gl_error("sync_metrics_collector: %s", msg);
	}
	return rv;
}

EXPORT int commit_metrics_collector(OBJECT *obj){
	int rv = 0;
	metrics_collector *my = OBJECTDATA(obj, metrics_collector);
	try {
		rv = my->commit(obj->clock);
	}
	catch (char *msg){
		gl_error("commit_metrics_collector: %s", msg);
	}
	catch (const char *msg){
		gl_error("commit_metrics_collector: %s", msg);
	}
	return rv;
}

EXPORT int isa_metrics_collector(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj, metrics_collector)->isa(classname);
}

// EOF








