/*
 * metrics_collector.cpp
 *
 *  Created on: Jan 30, 2017
 *      Author: tang526
 */

#include "metrics_collector.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

CLASS *metrics_collector::oclass = NULL;
CLASS *metrics_collector::pclass = NULL;
metrics_collector *metrics_collector::defaults = NULL;
PROPERTY *metrics_collector::propTriplexNomV = NULL;
PROPERTY *metrics_collector::propTriplexV1 = NULL;
PROPERTY *metrics_collector::propTriplexV2 = NULL;
PROPERTY *metrics_collector::propTriplexV12 = NULL;
PROPERTY *metrics_collector::propTriplexPrice = NULL;
PROPERTY *metrics_collector::propTriplexBill = NULL;
PROPERTY *metrics_collector::propTriplexP = NULL;
PROPERTY *metrics_collector::propTriplexQ = NULL;
PROPERTY *metrics_collector::propMeterNomV = NULL;
PROPERTY *metrics_collector::propMeterVa = NULL;
PROPERTY *metrics_collector::propMeterVb = NULL;
PROPERTY *metrics_collector::propMeterVc = NULL;
PROPERTY *metrics_collector::propMeterVab = NULL;
PROPERTY *metrics_collector::propMeterVbc = NULL;
PROPERTY *metrics_collector::propMeterVca = NULL;
PROPERTY *metrics_collector::propMeterPrice = NULL;
PROPERTY *metrics_collector::propMeterBill = NULL;
PROPERTY *metrics_collector::propMeterP = NULL;
PROPERTY *metrics_collector::propMeterQ = NULL;
PROPERTY *metrics_collector::propHouseLoad = NULL;
PROPERTY *metrics_collector::propHouseHVAC = NULL;
PROPERTY *metrics_collector::propHouseAirTemp = NULL;
PROPERTY *metrics_collector::propHouseCoolSet = NULL;
PROPERTY *metrics_collector::propHouseHeatSet = NULL;
PROPERTY *metrics_collector::propHouseSystemMode = NULL;
PROPERTY *metrics_collector::propWaterLoad = NULL;
PROPERTY *metrics_collector::propWaterSetPoint = NULL;
PROPERTY *metrics_collector::propWaterDemand = NULL;
PROPERTY *metrics_collector::propWaterTemp = NULL;

PROPERTY *metrics_collector::propWaterLSetPoint = NULL;
PROPERTY *metrics_collector::propWaterUSetPoint = NULL;
PROPERTY *metrics_collector::propWaterLTemp = NULL;
PROPERTY *metrics_collector::propWaterUTemp = NULL;
PROPERTY *metrics_collector::propWaterElemLMode = NULL;
PROPERTY *metrics_collector::propWaterElemUMode = NULL;

PROPERTY *metrics_collector::propInverterS = NULL;
PROPERTY *metrics_collector::propCapCountA = NULL;
PROPERTY *metrics_collector::propCapCountB = NULL;
PROPERTY *metrics_collector::propCapCountC = NULL;
PROPERTY *metrics_collector::propRegCountA = NULL;
PROPERTY *metrics_collector::propRegCountB = NULL;
PROPERTY *metrics_collector::propRegCountC = NULL;
PROPERTY *metrics_collector::propSwingSubLoad = NULL;
PROPERTY *metrics_collector::propSwingMeterS = NULL;

PROPERTY *metrics_collector::propTransformerOverloaded = NULL;
PROPERTY *metrics_collector::propLineOverloaded = NULL;

PROPERTY *metrics_collector::propChargeRate = NULL;
PROPERTY *metrics_collector::propBatterySOC = NULL;

bool metrics_collector::log_set = true;  // if false, the first (some class) instance will print messages to console

void new_metrics_collector(MODULE *mod){
	new metrics_collector(mod);
}

metrics_collector::metrics_collector(MODULE *mod){
	if(oclass == NULL)
	{
#ifdef _DEBUG
		gl_debug("construction metrics_collector class");
#endif
		oclass = gl_register_class(mod, const_cast<char *>("metrics_collector"), sizeof(metrics_collector), PC_POSTTOPDOWN);
    if(oclass == NULL)
      GL_THROW(const_cast<char *>("unable to register object class implemented by %s"),__FILE__);

    if(gl_publish_variable(oclass,
			PT_double, "interval[s]", PADDR(interval_length_dbl), PT_DESCRIPTION, "Interval at which the metrics_collector output is stored in JSON format",NULL) < 1) 
			GL_THROW(const_cast<char *>("unable to publish properties in %s"), __FILE__);

		defaults = this;
		memset(this, 0, sizeof(metrics_collector));
  }
}

int metrics_collector::isa(char *classname){
	return (strcmp(classname, oclass->name) == 0);
}

int metrics_collector::create(){

	memcpy(this, defaults, sizeof(metrics_collector));

	time_array = NULL;
	real_power_array = NULL;
	reactive_power_array = NULL;
	voltage_vll_array = NULL;
	voltage_vln_array = NULL;
	voltage_unbalance_array = NULL;
	total_load_array = NULL;
	hvac_load_array = NULL;
	wh_load_array = NULL;
	air_temperature_array = NULL;
	set_cooling_array = NULL;
	set_heating_array = NULL;
	count_array = NULL;
	real_power_loss_array = NULL;
	reactive_power_loss_array = NULL;
	wh_setpoint_array = NULL;
	wh_demand_array = NULL;
	wh_temp_array = NULL;

	wh_l_setpoint_array = NULL;
	wh_u_setpoint_array = NULL;
	wh_l_temp_array = NULL;
	wh_u_temp_array = NULL;

	trans_overload_status_array = NULL;
	line_overload_status_array = NULL;

	charge_rate_array = NULL;
	battery_SOC_array = NULL;

	metrics = NULL;

	// Interval related
	interval_length = -1;
	curr_index = 0;
	interval_length_dbl=-1.0;

	strcpy (parent_name, "No name given");

	log_me = false;
	first_write = true;
	return 1;
}

int metrics_collector::init(OBJECT *parent){

	OBJECT *obj = OBJECTHDR(this);

//	int temp = strcmp(parent->oclass->name,"triplex_meter") != 0;
	if (parent == NULL)
	{
		gl_error("metrics_collector must have a parent (triplex meter, house, waterheater, inverter, substation, or meter)");
		/*  TROUBLESHOOT
		Check the parent object of the metrics_collector.
		*/
		return 0;
	}
	parent_string = const_cast<char *>("");
	// Find parent, if not defined, or if the parent is not a supported class, throw an exception
	if (gl_object_isa(parent, "triplex_meter"))	{
		parent_string = const_cast<char *>("triplex_meter");
		if (propTriplexNomV == NULL) propTriplexNomV = gl_get_property (parent, "nominal_voltage");
		if (propTriplexPrice == NULL) propTriplexPrice = gl_get_property (parent, "price");
		if (propTriplexBill == NULL) propTriplexBill = gl_get_property (parent, "monthly_bill");
		if (propTriplexP == NULL) propTriplexP = gl_get_property (parent, "measured_real_power");
		if (propTriplexQ == NULL) propTriplexQ = gl_get_property (parent, "measured_reactive_power");
		if (propTriplexV1 == NULL) propTriplexV1 = gl_get_property (parent, "voltage_1");
		if (propTriplexV2 == NULL) propTriplexV2 = gl_get_property (parent, "voltage_2");
		if (propTriplexV12 == NULL) propTriplexV12 = gl_get_property (parent, "voltage_12");
		if (!log_set) {
			log_set = log_me = true;
		}
	} else if (gl_object_isa(parent, "house")) {
		parent_string = const_cast<char*>("house");
		if (propHouseLoad == NULL) propHouseLoad = gl_get_property (parent, "total_load");
		if (propHouseHVAC == NULL) propHouseHVAC = gl_get_property (parent, "hvac_load");
		if (propHouseAirTemp == NULL) propHouseAirTemp = gl_get_property (parent, "air_temperature");
		if (propHouseCoolSet == NULL) propHouseCoolSet = gl_get_property (parent, "cooling_setpoint");
		if (propHouseHeatSet == NULL) propHouseHeatSet = gl_get_property (parent, "heating_setpoint");
		if (propHouseSystemMode == NULL) propHouseSystemMode = gl_get_property (parent, "system_mode");
	} else if (gl_object_isa(parent,"waterheater")) {
		parent_string = const_cast<char*>("waterheater");
		if (propWaterLoad == NULL) propWaterLoad = gl_get_property (parent, "actual_load");
		if (propWaterSetPoint == NULL) propWaterSetPoint = gl_get_property (parent, "tank_setpoint");
		if (propWaterDemand == NULL) propWaterDemand = gl_get_property (parent, "water_demand");
		if (propWaterTemp == NULL) propWaterTemp = gl_get_property (parent, "temperature");

		if (propWaterLSetPoint == NULL) propWaterLSetPoint = gl_get_property (parent, "lower_tank_setpoint");
		if (propWaterUSetPoint == NULL) propWaterUSetPoint = gl_get_property (parent, "upper_tank_setpoint");
		if (propWaterLTemp == NULL) propWaterLTemp = gl_get_property (parent, "lower_tank_temperature");
		if (propWaterUTemp == NULL) propWaterUTemp = gl_get_property (parent, "upper_tank_temperature");
		if (propWaterElemLMode == NULL) propWaterElemLMode = gl_get_property (parent, "lower_heating_element_state");
		if (propWaterElemUMode == NULL) propWaterElemUMode = gl_get_property (parent, "upper_heating_element_state");
	} else if (gl_object_isa(parent,"inverter")) {
		parent_string = const_cast<char*>("inverter");
		if (propInverterS == NULL) propInverterS = gl_get_property (parent, "VA_Out");
	} else if (gl_object_isa(parent,"capacitor")) {
		parent_string = const_cast<char*>("capacitor");
		if (propCapCountA == NULL) propCapCountA = gl_get_property (parent, "cap_A_switch_count");
		if (propCapCountB == NULL) propCapCountB = gl_get_property (parent, "cap_B_switch_count");
		if (propCapCountC == NULL) propCapCountC = gl_get_property (parent, "cap_C_switch_count");
	} else if (gl_object_isa(parent,"regulator")) {
		parent_string = const_cast<char*>("regulator");
		if (propRegCountA == NULL) propRegCountA = gl_get_property (parent, "tap_A_change_count");
		if (propRegCountB == NULL) propRegCountB = gl_get_property (parent, "tap_B_change_count");
		if (propRegCountC == NULL) propRegCountC = gl_get_property (parent, "tap_C_change_count");
	} else if (gl_object_isa(parent,"substation")) {  // must be a swing bus
		PROPERTY *pval = gl_get_property(parent,"bustype");
		if ((pval==NULL) || (pval->ptype!=PT_enumeration))
		{
			GL_THROW(const_cast<char *>("metrics_collector:%s failed to map bustype variable from %s"), obj->name ? obj->name : "unnamed", parent->name ? parent->name : "unnamed");
			/*  TROUBLESHOOT
			While attempting to set up the deltamode interfaces and calculations with powerflow, the required interface could not be mapped.
			Please check your GLM and try again.  If the error persists, please submit a trac ticket with your code.
			*/
		}
		//Map to the intermediate
		auto *meter_bustype = (enumeration*)GETADDR(parent,pval);
		// Check if the parent meter is a swing bus (2) or not
		if (*meter_bustype != 2) {
			gl_error("If a metrics_collector is attached to a substation, it must be a SWING bus");
			return 0;
		}
		parent_string = const_cast<char *>("swingbus");
		if (propSwingSubLoad == NULL) propSwingSubLoad = gl_get_property (parent,
                                                                          const_cast<char *>("distribution_load"));
	} else if (gl_object_isa(parent, const_cast<char *>("meter"))) {
		parent_string = const_cast<char *>("meter"); // unless it's a swing bus
		if (propMeterNomV == NULL) propMeterNomV = gl_get_property (parent, "nominal_voltage");
		if (propMeterPrice == NULL) propMeterPrice = gl_get_property (parent, "price");
		if (propMeterBill == NULL) propMeterBill = gl_get_property (parent, "monthly_bill");
		if (propMeterP == NULL) propMeterP = gl_get_property (parent, "measured_real_power");
		if (propMeterQ == NULL) propMeterQ = gl_get_property (parent, "measured_reactive_power");
		if (propMeterVa == NULL) propMeterVa = gl_get_property (parent, "voltage_A");
		if (propMeterVb == NULL) propMeterVb = gl_get_property (parent, "voltage_B");
		if (propMeterVc == NULL) propMeterVc = gl_get_property (parent, "voltage_C");
		if (propMeterVab == NULL) propMeterVab = gl_get_property (parent, "voltage_AB");
		if (propMeterVbc == NULL) propMeterVbc = gl_get_property (parent, "voltage_BC");
		if (propMeterVca == NULL) propMeterVca = gl_get_property (parent, "voltage_CA");
		PROPERTY *pval = gl_get_property(parent, const_cast<char *>("bustype"));
		if ((pval!=NULL) && (pval->ptype==PT_enumeration))
		{
			auto *meter_bustype = (enumeration*)GETADDR(parent,pval);
			if (*meter_bustype == 2) {
				parent_string = const_cast<char *>("swingbus");
				if (propSwingMeterS == NULL) propSwingMeterS = gl_get_property (parent,
                                                                                const_cast<char *>("measured_power"));
			}
		}
	} else if (gl_object_isa(parent, "transformer")) {
		parent_string = const_cast<char*>("transformer");
		if (propTransformerOverloaded == NULL) propTransformerOverloaded = gl_get_property (parent, "overloaded_status");
	} else if (gl_object_isa(parent, "line")) {
		parent_string = const_cast<char*>("line");
		if (propLineOverloaded == NULL) propLineOverloaded = gl_get_property (parent, "overloaded_status");
	} else if (gl_object_isa(parent, "evcharger_det")) {
		parent_string = const_cast<char*>("evcharger_det");
		if (propChargeRate == NULL) propChargeRate = gl_get_property (parent, "charge_rate");
		if (propBatterySOC == NULL) propBatterySOC = gl_get_property (parent, "battery_SOC");
	}
	else {
		gl_error("metrics_collector allows only these parents: triplex meter, house, waterheater, inverter, substation, meter, capacitor, regulator, transformer, line.");
		/*  TROUBLESHOOT
		Check the parent object of the metrics_collector.
		*/
		return 0;
	}

	// Create the structures for JSON outputs
	if (strcmp(parent_string, "triplex_meter") == 0)
	{
		if (parent->name != NULL) {
			strcpy (parent_name, parent->name);
		}
		metrics = (double *)gl_malloc(MTR_ARRAY_SIZE*sizeof(double));
		if (metrics == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate JSON metrics array"), obj->id);
		}
	}
	else if (strcmp(parent_string, "meter") == 0)
	{
		if (parent->name != NULL) {
			strcpy (parent_name, parent->name);
		}
		metrics = (double *)gl_malloc(MTR_ARRAY_SIZE*sizeof(double));
		if (metrics == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate JSON metrics array"), obj->id);
		}
	} 
	else if (strcmp(parent_string, "house") == 0)
	{
		if (parent->name != NULL) {
			strcpy (parent_name, parent->name);
		}
		metrics = (double *)gl_malloc(HSE_ARRAY_SIZE*sizeof(double));
		if (metrics == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate JSON metrics array"), obj->id);
		}
	}
	else if (strcmp(parent_string, "waterheater") == 0) 
	{
		if (parent->parent->name != NULL) {
			strcpy (parent_name, parent->parent->name);
		}
		metrics = (double *)gl_malloc(WH_ARRAY_SIZE*sizeof(double));
		if (metrics == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate JSON metrics array"), obj->id);
		}
		// Get the name of the waterheater for actual load
		char tname[32];
		sprintf(tname, "%i", parent->id);
		char *namestr = (parent->name ? parent->name : tname);
		sprintf(waterheaterName, "waterheater_%s_actual_load", namestr);
	}
	else if (strcmp(parent_string, "inverter") == 0)
	{
		if (parent->name != NULL) {
			strcpy (parent_name, parent->name);
		}
		metrics = (double *)gl_malloc(INV_ARRAY_SIZE*sizeof(double));
		if (metrics == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate JSON metrics array"), obj->id);
		}
	}
	else if (strcmp(parent_string, "capacitor") == 0)
	{
		if (parent->name != NULL) {
			strcpy (parent_name, parent->name);
		}
		metrics = (double *)gl_malloc(CAP_ARRAY_SIZE*sizeof(double));
		if (metrics == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate JSON metrics array"), obj->id);
		}
	}
	else if (strcmp(parent_string, "regulator") == 0)
	{
		if (parent->name != NULL) {
			strcpy (parent_name, parent->name);
		}
		metrics = (double *)gl_malloc(REG_ARRAY_SIZE*sizeof(double));
		if (metrics == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate JSON metrics array"), obj->id);
		}
	}
	else if (strcmp(parent_string, "swingbus") == 0)
	{
		// In this work, only when a metrics_collector is attached to a swing-bus, the feeder losses are recorded
		// Find all the link objects for collecting loss values
		link_objects = gl_find_objects(FL_NEW,FT_MODULE,SAME,"powerflow",FT_END); //find all link objects
		if(link_objects == NULL){
			gl_warning("No link objects were found.");
			/* TROUBLESHOOT
			No link objects were found in your .glm file. In this way, there is no losses calculated.
			*/
		}
		if (parent->name != NULL) {
			strcpy (parent_name, parent->name);
		}
		else {
			strcpy (parent_name, "Swing Bus Metrics");
		}
		metrics = (double *)gl_malloc(FDR_ARRAY_SIZE*sizeof(double));
		if (metrics == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate JSON metrics array"), obj->id);
		}
	}
	else if (strcmp(parent_string, "transformer") == 0)
	{
		if (parent->name != NULL) {
			strcpy (parent_name, parent->name);
		}
		metrics = (double *)gl_malloc(TRANS_OVERLOAD_ARRAY_SIZE*sizeof(double));
		if (metrics == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate JSON metrics array",obj->id);
		}
	}
	else if (strcmp(parent_string, "line") == 0)
	{
		if (parent->name != NULL) {
			strcpy (parent_name, parent->name);
		}
		metrics = (double *)gl_malloc(LINE_OVERLOAD_ARRAY_SIZE*sizeof(double));
		if (metrics == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate JSON metrics array",obj->id);
		}
	}
	else if (strcmp(parent_string, "evcharger_det") == 0)
	{
		if (parent->name != NULL) {
			strcpy (parent_name, parent->name);
		}
		metrics = (double *)gl_malloc(EVCHARGER_DET_ARRAY_SIZE*sizeof(double));
		if (metrics == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate JSON metrics array",obj->id);
		}
	}

	// Check valid metrics_collector output interval
	interval_length = (TIMESTAMP)(interval_length_dbl);
	vector_length = interval_length + 1;
	if(interval_length <= 0){
		gl_error("metrics_collector::init(): invalid interval of %i, must be greater than 0", interval_length);
		/* TROUBLESHOOT
			The metrics_collector interval must be a positive number of seconds.
		*/
		return 0;
	}

	// need the time sample points for every parent type
	time_array = (TIMESTAMP *)gl_malloc(vector_length*sizeof(TIMESTAMP));
	if (time_array == NULL)
	{
		GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate time array"), obj->id);
		/*  TROUBLESHOOT
		While attempting to allocate the array, an error was encountered.
		Please try again.  If the error persists, please submit a bug report via the Trac system.
		*/
	}

	// Allocate the arrays based on the parent type
	if ((strcmp(parent_string, "triplex_meter") == 0) || (strcmp(parent_string, "meter") == 0)) {
		// Allocate real power array
		real_power_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (real_power_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate real power array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate reactive power array
		reactive_power_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (reactive_power_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate reactive power array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate phase 1-2 voltage array
		voltage_vll_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (voltage_vll_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate phase 1-2 voltage array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate average voltage array
		voltage_vln_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (voltage_vln_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate average vln voltage array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate voltage unbalance array
		voltage_unbalance_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (voltage_unbalance_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate voltage unbalance array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Initialize the array
		for (curr_index=0; curr_index<vector_length; curr_index++)
		{
			real_power_array[curr_index] = 0.0;
			reactive_power_array[curr_index] = 0.0;
			voltage_vll_array[curr_index] = 0.0;
			voltage_vln_array[curr_index] = 0.0;
			voltage_unbalance_array[curr_index] = 0.0;
		}

	}
	// If parent is house
	else if (strcmp(parent_string, "house") == 0) {
		// Allocate total_load array
		total_load_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (total_load_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate total_load array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate hvac_load array
		hvac_load_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (hvac_load_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate hvac_load array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate air temperature array
		air_temperature_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (air_temperature_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate air_temperature array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate air temperature deviation from cooling setpointarray
		set_cooling_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (set_cooling_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate set_cooling_array array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate air temperature deviation from heating setpointarray
		set_heating_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (set_heating_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate set_heating_array array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Initialize the array
		for (curr_index=0; curr_index<vector_length; curr_index++)
		{
			total_load_array[curr_index] = 0.0;
			hvac_load_array[curr_index] = 0.0;
			air_temperature_array[curr_index] = 0.0;
			set_cooling_array[curr_index] = 0.0;
			set_heating_array[curr_index] = 0.0;
			system_mode = 0;
		}
	}
	// If parent is waterheater
	else if (strcmp(parent_string, "waterheater") == 0) {
		// Allocate hvac_load array
		wh_load_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (wh_load_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate wh_load array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate wh_setpoint array
		wh_setpoint_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (wh_setpoint_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate wh_setpoint array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate wh_demand array
		wh_demand_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (wh_demand_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate wh_demand array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate wh_temp array
		wh_temp_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (wh_temp_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate wh_temp array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate wh_l_setpoint_array array
		wh_l_setpoint_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (wh_l_setpoint_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate wh_l_setpoint array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate wh_u_setpoint_array array
		wh_u_setpoint_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (wh_u_setpoint_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate wh_u_setpoint array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate wh_l_temp_array array
		wh_l_temp_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (wh_l_temp_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate wh_l_temp array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate wh_u_setpoint_array array
		wh_u_temp_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (wh_u_temp_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate wh_u_temp array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		for (curr_index=0; curr_index<vector_length; curr_index++)
		{
			wh_load_array[curr_index] = 0.0;
			wh_setpoint_array[curr_index] = 0.0;
			wh_demand_array[curr_index] = 0.0;
			wh_temp_array[curr_index] = 0.0;

			wh_l_setpoint_array[curr_index] = 0.0;
			wh_u_setpoint_array[curr_index] = 0.0;
			wh_l_temp_array[curr_index] = 0.0;
			wh_u_temp_array[curr_index] = 0.0;
		}
		elem_l_mode = 0;
		elem_u_mode = 0;
	}
	// If parent is inverter
	else if (strcmp(parent_string, "inverter") == 0) {
		// Allocate real power array
		real_power_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (real_power_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate real power array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate reactive power array
		reactive_power_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (reactive_power_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate reactive power array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}
	}
	// If parent is meter
	else if (strcmp(parent_string, "swingbus") == 0) {
		// Allocate real power array
		real_power_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (real_power_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate real power array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate reactive power array
		reactive_power_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (reactive_power_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate reactive power array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}
		// Allocate real power loss array
		real_power_loss_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (real_power_loss_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate real power loss array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}

		// Allocate reactive power loss array
		reactive_power_loss_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (reactive_power_loss_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate reactive power loss array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}
	}
	else if ((strcmp(parent_string, "capacitor") == 0) || (strcmp(parent_string, "regulator") == 0)) {
		count_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (count_array == NULL)
		{
			GL_THROW(const_cast<char *>("metrics_collector %d::init(): Failed to allocate operation count array"), obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}
		for (int i = 0; i < vector_length; i++) count_array[i] = 0.0;
	}
	else if (strcmp(parent_string, "transformer") == 0) {
		trans_overload_status_array = (int *)gl_malloc(vector_length*sizeof(int));
		// Check
		if (trans_overload_status_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate overload status array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}
		for (int i = 0; i < vector_length; i++) trans_overload_status_array[i] = 0;
	}
	else if (strcmp(parent_string, "line") == 0) {
		line_overload_status_array = (int *)gl_malloc(vector_length*sizeof(int));
		// Check
		if (line_overload_status_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate overload status array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}
		for (int i = 0; i < vector_length; i++) line_overload_status_array[i] = 0;
	}
	else if (strcmp(parent_string, "evcharger_det") == 0) {
		charge_rate_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (charge_rate_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate overload status array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}
		battery_SOC_array = (double *)gl_malloc(vector_length*sizeof(double));
		// Check
		if (battery_SOC_array == NULL)
		{
			GL_THROW("metrics_collector %d::init(): Failed to allocate overload status array",obj->id);
			/*  TROUBLESHOOT
			While attempting to allocate the array, an error was encountered.
			Please try again.  If the error persists, please submit a bug report via the Trac system.
			*/
		}
		for (int i = 0; i < vector_length; i++) {
			charge_rate_array[i] = 0.0;
			battery_SOC_array[i] = 0.0;
		}
	}
	// else not possible come to this step
	else {
		gl_error("metrics_collector must have a triplex meter, meter, house, waterheater, inverter, swing-bus, transformer or line as its parent");
		/*  TROUBLESHOOT
		Check the parent object of the metrics_collector. The metrics_collector is only able to be childed via a
		triplex meter or a house or an inverter when connecting into powerflow systems.
		*/
		return 0;
	}


	// Initialize tracking variables
	curr_index = 0;

	// Update time variables
	start_time = gl_globalclock;
	next_write = start_time + interval_length;

	return 1;
}

TIMESTAMP metrics_collector::postsync(TIMESTAMP t0, TIMESTAMP t1) {
	// recalculate next_time, since we know commit() will fire
	if (next_write <= t1) {
		write_now = true;
	}
	if (t1 < next_write) {
		return next_write;
	}
	return t1 + interval_length;
}

int metrics_collector::commit(TIMESTAMP t1){
	OBJECT *obj = OBJECTHDR(this);
	// read the parameters for each time step
	if(0 == read_line(obj)){
		gl_error("metrics_collector::commit(): error when reading the values");
		return 0;
	}

	// check for write
	if (write_now) {
		if (0 == write_line(t1, obj)) {
			gl_error("metrics_collector::commit(): error when aggregating values over the interval");
			return 0;
		}
		write_now = false;
	}
	return 1;
}

/**
	@return 0 on failure, 1 on success
 **/
int metrics_collector::read_line(OBJECT *obj){
	// save the actual simulator time
	time_array[curr_index] = gl_globalclock;

	if (strcmp(parent_string, "triplex_meter") == 0) {
		real_power_array[curr_index]  = *gl_get_double(obj->parent, propTriplexP);
		reactive_power_array[curr_index] = *gl_get_double(obj->parent, propTriplexQ);

		// Get bill value, price unit given in triplex_meter is [$/kWh]
		price_parent = *gl_get_double(obj->parent, propTriplexPrice);
		bill_parent = *gl_get_double(obj->parent, propTriplexBill);

		// Get voltage values, s1 to ground, s2 to ground, s1 to s2
		double v1 = (*gl_get_complex(obj->parent, propTriplexV1)).Mag();  
		double v2 = (*gl_get_complex(obj->parent, propTriplexV2)).Mag();  
		double v12 = (*gl_get_complex(obj->parent, propTriplexV12)).Mag();

		// compliance with C84.1; unbalance defined as max deviation from average / average, here based on 1-N and 2-N
		double vavg = 0.5 * (v1 + v2);

		voltage_vll_array[curr_index] = fabs(v12);
		voltage_vln_array[curr_index] = vavg;
		voltage_unbalance_array[curr_index] =  0.5 * fabs(v1 - v2)/vavg;
	}
	else if (strcmp(parent_string, "meter") == 0)
	{
		real_power_array[curr_index] = *gl_get_double(obj->parent, propMeterP);
		reactive_power_array[curr_index] = *gl_get_double(obj->parent, propMeterQ);

		// Get bill value, price unit given is [$/kWh]
		price_parent = *gl_get_double(obj->parent, propMeterPrice);
		bill_parent = *gl_get_double(obj->parent, propMeterBill);

		// these can have one, two or three phases
		double va = (*gl_get_complex(obj->parent, propMeterVa)).Mag();   
		double vb = (*gl_get_complex(obj->parent, propMeterVb)).Mag();   
		double vc = (*gl_get_complex(obj->parent, propMeterVc)).Mag();
		double nph = 0.0;
		if (va > 0.0) nph += 1.0;
		if (vb > 0.0) nph += 1.0;
		if (vc > 0.0) nph += 1.0;
		if (nph < 1.0) nph = 1.0;
		double vavg = (va + vb + vc) / nph;
		double vmin = vavg;
		double vmax = vavg;
		if (va > 0.0) {
			if (va < vmin) vmin = va;
			if (va > vmax) vmax = va;
		}
		if (vb > 0.0) {
			if (vb < vmin) vmin = vb;
			if (vb > vmax) vmax = vb;
		}
		if (vc > 0.0) {
			if (vc < vmin) vmin = vc;
			if (vc > vmax) vmax = vc;
		}
		double vab = (*gl_get_complex(obj->parent, propMeterVab)).Mag();   
		double vbc = (*gl_get_complex(obj->parent, propMeterVbc)).Mag();   
		double vca = (*gl_get_complex(obj->parent, propMeterVca)).Mag();
		double vll = (vab + vbc + vca) / nph;
		// determine unbalance per C84.1
		double vdev = 0.0;
		double vdevph;
		if (vab > 0.0) {
			vdevph = fabs(vab - vll);
			if (vdevph > vdev) vdev = vdevph;
		}
		if (vbc > 0.0) {
			vdevph = fabs(vbc - vll);
			if (vdevph > vdev) vdev = vdevph;
		}
		if (vca > 0.0) {
			vdevph = fabs(vca - vll);
			if (vdevph > vdev) vdev = vdevph;
		}

		voltage_vll_array[curr_index] = vll;  // Vll
		voltage_vln_array[curr_index] = vavg;  // Vln
		voltage_unbalance_array[curr_index] = vdev / vll; // max deviation from Vll / average Vll
	} 
	else if (strcmp(parent_string, "house") == 0)
	{
		total_load_array[curr_index] = *gl_get_double(obj->parent, propHouseLoad);
		hvac_load_array[curr_index] = *gl_get_double(obj->parent, propHouseHVAC);
		air_temperature_array[curr_index] = *gl_get_double(obj->parent, propHouseAirTemp);
		set_cooling_array[curr_index] = *gl_get_double(obj->parent, propHouseCoolSet);
		set_heating_array[curr_index] = *gl_get_double(obj->parent, propHouseHeatSet);
		system_mode = *gl_get_enum(obj->parent, propHouseSystemMode);
	}
	else if (strcmp(parent_string, "waterheater") == 0) {
		wh_load_array[curr_index] = *gl_get_double(obj->parent, propWaterLoad);
		wh_setpoint_array[curr_index] = *gl_get_double(obj->parent, propWaterSetPoint);
		wh_demand_array[curr_index] = *gl_get_double(obj->parent, propWaterDemand);
		wh_temp_array[curr_index] = *gl_get_double(obj->parent, propWaterTemp);

		wh_l_setpoint_array[curr_index] = *gl_get_double(obj->parent, propWaterLSetPoint);
		wh_u_setpoint_array[curr_index] = *gl_get_double(obj->parent, propWaterUSetPoint);
		wh_l_temp_array[curr_index] = *gl_get_double(obj->parent, propWaterLTemp);
		wh_u_temp_array[curr_index] = *gl_get_double(obj->parent, propWaterUTemp);
		elem_l_mode = *gl_get_enum(obj->parent, propWaterElemLMode);
		elem_u_mode = *gl_get_enum(obj->parent, propWaterElemUMode);
	}
	else if (strcmp(parent_string, "inverter") == 0) {
		gld::complex VAOut = *gl_get_complex(obj->parent, propInverterS);
		real_power_array[curr_index] = (double)VAOut.Re();
		reactive_power_array[curr_index] = (double)VAOut.Im();
	}
	else if (strcmp(parent_string, "capacitor") == 0) {
		double opcount = *gl_get_double(obj->parent, propCapCountA)
			+ *gl_get_double(obj->parent, propCapCountB) + *gl_get_double(obj->parent, propCapCountC);
		count_array[curr_index] = opcount;
	}
	else if (strcmp(parent_string, "regulator") == 0) {
		double opcount = *gl_get_double(obj->parent, propRegCountA)
			+ *gl_get_double(obj->parent, propRegCountB) + *gl_get_double(obj->parent, propRegCountC);
		count_array[curr_index] = opcount;
	}
	else if (strcmp(parent_string, "swingbus") == 0) {
		// Get VAfeeder values
		gld::complex VAfeeder;
		if (gl_object_isa(obj->parent, const_cast<char *>("substation"))) {
			VAfeeder = *gl_get_complex(obj->parent, propSwingSubLoad);
		} else {
			VAfeeder = *gl_get_complex(obj->parent, propSwingMeterS);
		}
		real_power_array[curr_index] = (double)VAfeeder.Re();
		reactive_power_array[curr_index] = (double)VAfeeder.Im();
		// Feeder Losses calculation
		int index = 0;
		obj = NULL;
		gld::complex lossesSum = 0.0;
		while(obj = gl_find_next(link_objects,obj)){
			if(index >= link_objects->hit_count){
				break;
			}
			char * oclassName = obj->oclass->name;
			if (strcmp(oclassName, "overhead_line") == 0 || strcmp(oclassName, "underground_line") == 0 || 
					strcmp(oclassName, "triplex_line") == 0 || strcmp(oclassName, "transformer") == 0 || 
					strcmp(oclassName, "regulator") == 0 || strcmp(oclassName, "switch") == 0 || strcmp(oclassName, "fuse") == 0) {
				// Obtain the link data
				link_object *one_link = OBJECTDATA(obj,link_object);
				if(one_link == NULL){
					gl_error("Unable to map the object as link.");
					return 0;
				}
				gld::complex loss = one_link->power_loss;
				// real power losses should be positive
				if (loss.Re() < 0) {
					loss.Re() = -loss.Re();
				}
				lossesSum += loss;
			}
			index++;
		} 
		// Put the loss value into the array
		real_power_loss_array[curr_index] = (double)lossesSum.Re();
		reactive_power_loss_array[curr_index] = (double)lossesSum.Im();
	}
	else if (strcmp(parent_string, "transformer") == 0) {
		if (*gl_get_bool(obj->parent, propTransformerOverloaded)) {
		  trans_overload_status_array[curr_index] = 1;
		} else {
			trans_overload_status_array[curr_index] = 0;
		}
	}
	else if (strcmp(parent_string, "line") == 0) {
		if (*gl_get_bool(obj->parent, propLineOverloaded)) {
		  line_overload_status_array[curr_index] = 1;
		} else {
			line_overload_status_array[curr_index] = 0;
		}
	}
	else if (strcmp(parent_string, "evcharger_det") == 0) {
		charge_rate_array[curr_index] = *gl_get_double(obj->parent, propChargeRate);
		battery_SOC_array[curr_index] = *gl_get_double(obj->parent, propBatterySOC);
	}
	// else not possible come to this step
	else {
		gl_error("metrics_collector must have a triplex meter, house, waterheater, inverter, swing-bus, capacitor, regulator, transformer or line as its parent");
		/*  TROUBLESHOOT
		Check the parent object of the metrics_collector. The metrics_collector is only able to be childed via a
		triplex meter or a house or a waterheater or an inverter or a swing-bus when connecting into powerflow systems.
		*/
		return 0;
	}

	// Update index value
	++curr_index;
	if (curr_index == vector_length) {
		gl_error ("metrics_collector wrapped its buffer for aggregating data, %d %d %ld\n",
							curr_index, vector_length, gl_globalclock);
		/*  TROUBLESHOOT
		The metrics_collector allocates a buffer large enough to cover its aggregation 
		interval at 1-second time steps. Is the time step smaller than 1s? Program logic error?
		*/
	}
	return 1;
}

void metrics_collector::log_to_console(char *msg, TIMESTAMP t) {
	if (log_me)	{
		printf("** %s: t = %" FMT_INT64 "d, next_write = %" FMT_INT64 "d, curr_index = %i\n",
					 msg, t - start_time, next_write - start_time, curr_index);
		for (int j = 0; j < curr_index; j++) {
			printf("	%" FMT_INT64 "d", time_array[j] - start_time);
		}
		printf("\n");
		for (int j = 0; j < curr_index; j++) {
//			printf("	%g", real_power_array[j]);			
			printf("	%g", voltage_vll_array[j]);			
		}
		printf("\n");
	}
}

/**
	@return 1 on successful write, 0 on unsuccessful write, error, or when not ready
 **/

int metrics_collector::write_line(TIMESTAMP t1, OBJECT *obj){
//	log_to_console ("entering write_line", t1);
	bool bOverran = false;
	if (t1 > next_write) { // interpolate at the aggregation interval's actual end point
		bOverran = true;     
		copyHistories (curr_index - 1, curr_index);
		interpolateHistories (curr_index - 1, next_write);     
//		log_to_console ("adjusted for overrun", t1);
	}		

	if ((strcmp(parent_string, "triplex_meter") == 0) || (strcmp(parent_string, "meter") == 0)) {
		metrics[MTR_MIN_REAL_POWER] = findMin(real_power_array, curr_index);
		metrics[MTR_MAX_REAL_POWER] = findMax(real_power_array, curr_index);
		metrics[MTR_AVG_REAL_POWER] = findAverage(real_power_array, curr_index);

		metrics[MTR_MIN_REAC_POWER] = findMin(reactive_power_array, curr_index);
		metrics[MTR_MAX_REAC_POWER] = findMax(reactive_power_array, curr_index);
		metrics[MTR_AVG_REAC_POWER] = findAverage(reactive_power_array, curr_index);

		metrics[MTR_REAL_ENERGY] = findAverage(real_power_array, curr_index) * interval_length / 3600;
		metrics[MTR_REAC_ENERGY] = findAverage(reactive_power_array, curr_index) * interval_length / 3600;

//		metrics[MTR_BILL] = metrics[MTR_REAL_ENERGY] * price_parent / 1000; // price unit given is [$/kWh]
		metrics[MTR_BILL] = bill_parent;

		metrics[MTR_MIN_VLL] = findMin(voltage_vll_array, curr_index);
		metrics[MTR_MAX_VLL] = findMax(voltage_vll_array, curr_index);
		metrics[MTR_AVG_VLL] = findAverage(voltage_vll_array, curr_index);
#ifdef ALL_MTR_METRICS
		metrics[MTR_MIN_VLN] = findMin(voltage_vln_array, curr_index);
		metrics[MTR_MAX_VLN] = findMax(voltage_vln_array, curr_index);
		metrics[MTR_AVG_VLN] = findAverage(voltage_vln_array, curr_index);

		metrics[MTR_MIN_VUNB] = findMin(voltage_unbalance_array, curr_index);
		metrics[MTR_MAX_VUNB] = findMax(voltage_unbalance_array, curr_index);
		metrics[MTR_AVG_VUNB] = findAverage(voltage_unbalance_array, curr_index);
#endif
		// Voltage above/below ANSI C84 A/B Range
		double normVol = 1.0, aboveRangeA, belowRangeA, aboveRangeB, belowRangeB, belowOutage;
		if (strcmp(parent_string, "triplex_meter") == 0) {
			normVol = *gl_get_double(obj->parent, propTriplexNomV);
			belowOutage = normVol * 0.10000 * 2.0;
			aboveRangeA = normVol * 1.05000 * 2.0;
			belowRangeA = normVol * 0.95000 * 2.0;
			aboveRangeB = normVol * 1.05833 * 2.0;
			belowRangeB = normVol * 0.91667 * 2.0;
			if (log_me) {
				log_to_console (const_cast<char *>("checking violations"), t1);
				aboveRangeA = normVol * 1.040 * 2.0;
				aboveRangeB = normVol * 1.044 * 2.0;
			}
		} else {
			normVol = *gl_get_double(obj->parent, propMeterNomV);
			belowOutage = normVol * 0.10000 * (std::sqrt(3));
			aboveRangeA = normVol * 1.05000 * (std::sqrt(3));
			belowRangeA = normVol * 0.95000 * (std::sqrt(3));
			aboveRangeB = normVol * 1.05833 * (std::sqrt(3));
			belowRangeB = normVol * 0.91667 * (std::sqrt(3));
		}
		// Voltage above Range A
		struct vol_violation vol_Vio = findOutLimit(first_write, voltage_vll_array, true, aboveRangeA, curr_index);
		metrics[MTR_ABOVE_A_DUR] = vol_Vio.durationViolation;
#ifdef ALL_MTR_METRICS
		metrics[MTR_ABOVE_A_CNT] = vol_Vio.countViolation;
#endif
		// Voltage below Range A
		vol_Vio = findOutLimit(first_write, voltage_vll_array, false, belowRangeA, curr_index);
		metrics[MTR_BELOW_A_DUR] = vol_Vio.durationViolation;
#ifdef ALL_MTR_METRICS
		metrics[MTR_BELOW_A_CNT] = vol_Vio.countViolation;
#endif
		// Voltage above Range B
		vol_Vio = findOutLimit(first_write, voltage_vll_array, true, aboveRangeB, curr_index);
		metrics[MTR_ABOVE_B_DUR] = vol_Vio.durationViolation;
#ifdef ALL_MTR_METRICS
		metrics[MTR_ABOVE_B_CNT] = vol_Vio.countViolation;
#endif
		// Voltage below Range B
		vol_Vio = findOutLimit(first_write, voltage_vll_array, false, belowRangeB, curr_index);
		metrics[MTR_BELOW_B_DUR] = vol_Vio.durationViolation;
#ifdef ALL_MTR_METRICS
		metrics[MTR_BELOW_B_CNT] = vol_Vio.countViolation;
#endif
#ifdef ALL_MTR_METRICS
		// Voltage below 10% of the norminal voltage rating
		vol_Vio = findOutLimit(first_write, voltage_vll_array, false, belowOutage, curr_index);
		metrics[MTR_BELOW_10_DUR] = vol_Vio.durationViolation;
		metrics[MTR_BELOW_10_CNT] = vol_Vio.countViolation;
#endif
	}
	// If parent is house
	else if (strcmp(parent_string, "house") == 0) {
		metrics[HSE_MIN_TOTAL_LOAD] = findMin(total_load_array, curr_index);
		metrics[HSE_MAX_TOTAL_LOAD] = findMax(total_load_array, curr_index);
		metrics[HSE_AVG_TOTAL_LOAD] = findAverage(total_load_array, curr_index);

		metrics[HSE_MIN_HVAC_LOAD] = findMin(hvac_load_array, curr_index);
		metrics[HSE_MAX_HVAC_LOAD] = findMax(hvac_load_array, curr_index);
		metrics[HSE_AVG_HVAC_LOAD] = findAverage(hvac_load_array, curr_index);

		metrics[HSE_MIN_AIR_TEMP] = findMin(air_temperature_array, curr_index);
		metrics[HSE_MAX_AIR_TEMP] = findMax(air_temperature_array, curr_index);
		metrics[HSE_AVG_AIR_TEMP] = findAverage(air_temperature_array, curr_index);
		metrics[HSE_AVG_DEV_COOLING] = findAverage(set_cooling_array, curr_index);
		metrics[HSE_AVG_DEV_HEATING] = findAverage(set_heating_array, curr_index);
		metrics[HSE_SYSTEM_MODE] = (double) system_mode;
	}
	else if (strcmp(parent_string, "waterheater") == 0) {
		metrics[WH_MIN_ACTUAL_LOAD] = findMin(wh_load_array, curr_index);
		metrics[WH_MAX_ACTUAL_LOAD] = findMax(wh_load_array, curr_index);
		metrics[WH_AVG_ACTUAL_LOAD] = findAverage(wh_load_array, curr_index);
		metrics[WH_MIN_SETPOINT] = findMin(wh_setpoint_array, curr_index);
		metrics[WH_MAX_SETPOINT] = findMax(wh_setpoint_array, curr_index);
		metrics[WH_AVG_SETPOINT] = findAverage(wh_setpoint_array, curr_index);
		metrics[WH_MIN_DEMAND] = findMin(wh_demand_array, curr_index);
		metrics[WH_MAX_DEMAND] = findMax(wh_demand_array, curr_index);
		metrics[WH_AVG_DEMAND] = findAverage(wh_demand_array, curr_index);
		metrics[WH_MIN_TEMP] = findMin(wh_temp_array, curr_index);
		metrics[WH_MAX_TEMP] = findMax(wh_temp_array, curr_index);
		metrics[WH_AVG_TEMP] = findAverage(wh_temp_array, curr_index);

		metrics[WH_MIN_L_SETPOINT] = findMin(wh_l_setpoint_array, curr_index);
		metrics[WH_MAX_L_SETPOINT] = findMax(wh_l_setpoint_array, curr_index);
		metrics[WH_AVG_L_SETPOINT] = findAverage(wh_l_setpoint_array, curr_index);
		metrics[WH_MIN_U_SETPOINT] = findMin(wh_u_setpoint_array, curr_index);
		metrics[WH_MAX_U_SETPOINT] = findMax(wh_u_setpoint_array, curr_index);
		metrics[WH_AVG_U_SETPOINT] = findAverage(wh_u_setpoint_array, curr_index);
		metrics[WH_MIN_L_TEMP] = findMin(wh_l_temp_array, curr_index);
		metrics[WH_MAX_L_TEMP] = findMax(wh_l_temp_array, curr_index);
		metrics[WH_AVG_L_TEMP] = findAverage(wh_l_temp_array, curr_index);
		metrics[WH_MIN_U_TEMP] = findMin(wh_u_temp_array, curr_index);
		metrics[WH_MAX_U_TEMP] = findMax(wh_u_temp_array, curr_index);
		metrics[WH_AVG_U_TEMP] = findAverage(wh_u_temp_array, curr_index);
		metrics[WH_ELEM_L_MODE] = (double) elem_l_mode;
		metrics[WH_ELEM_U_MODE] = (double) elem_u_mode;
	}
	else if (strcmp(parent_string, "inverter") == 0) {
		metrics[INV_MIN_REAL_POWER] = findMin(real_power_array, curr_index);
		metrics[INV_MAX_REAL_POWER] = findMax(real_power_array, curr_index);
		metrics[INV_AVG_REAL_POWER] = findAverage(real_power_array, curr_index);

		metrics[INV_MIN_REAC_POWER] = findMin(reactive_power_array, curr_index);
		metrics[INV_MAX_REAC_POWER] = findMax(reactive_power_array, curr_index);
		metrics[INV_AVG_REAC_POWER] = findAverage(reactive_power_array, curr_index);
	}
	else if (strcmp(parent_string, "capacitor") == 0) {
		metrics[CAP_OPERATION_CNT] = findMax(count_array, curr_index);
	}
	else if (strcmp(parent_string, "regulator") == 0) {
		metrics[REG_OPERATION_CNT] = findMax(count_array, curr_index);
	}
	// If parent is transformer
	else if (strcmp(parent_string, "transformer") == 0) {
		metrics[TRANS_OVERLOAD_PERC] = countPerc(trans_overload_status_array, curr_index);
	}
	// If parent is line
	else if (strcmp(parent_string, "line") == 0) {
		metrics[LINE_OVERLOAD_PERC] = countPerc(line_overload_status_array, curr_index);
	}
	else if (strcmp(parent_string, "swingbus") == 0) {
		// real power data
		metrics[FDR_MIN_REAL_POWER] = findMin(real_power_array, curr_index);
		metrics[FDR_MAX_REAL_POWER] = findMax(real_power_array, curr_index);
		metrics[FDR_AVG_REAL_POWER] = findAverage(real_power_array, curr_index);
		metrics[FDR_MED_REAL_POWER] = findMedian(real_power_array, curr_index);
		// Reactive power data    
		metrics[FDR_MIN_REAC_POWER] = findMin(reactive_power_array, curr_index);
		metrics[FDR_MAX_REAC_POWER] = findMax(reactive_power_array, curr_index);
		metrics[FDR_AVG_REAC_POWER] = findAverage(reactive_power_array, curr_index);
		metrics[FDR_MED_REAC_POWER] = findMedian(reactive_power_array, curr_index);
		// Energy data
		metrics[FDR_REAL_ENERGY] = findAverage(real_power_array, curr_index) * interval_length / 3600;
		metrics[FDR_REAC_ENERGY] = findAverage(reactive_power_array, curr_index) * interval_length / 3600;
		// real power loss data
		metrics[FDR_MIN_REAL_LOSS] = findMin(real_power_loss_array, curr_index);
		metrics[FDR_MAX_REAL_LOSS] = findMax(real_power_loss_array, curr_index);
		metrics[FDR_AVG_REAL_LOSS] = findAverage(real_power_loss_array, curr_index);
		metrics[FDR_MED_REAL_LOSS] = findMedian(real_power_loss_array, curr_index);
		// Reactive power data    
		metrics[FDR_MIN_REAC_LOSS] = findMin(reactive_power_loss_array, curr_index);
		metrics[FDR_MAX_REAC_LOSS] = findMax(reactive_power_loss_array, curr_index);
		metrics[FDR_AVG_REAC_LOSS] = findAverage(reactive_power_loss_array, curr_index);
		metrics[FDR_MED_REAC_LOSS] = findMedian(reactive_power_loss_array, curr_index);
	}
	else if (strcmp(parent_string, "evcharger_det") == 0) {
		metrics[EV_MIN_CHARGE_RATE] = findMin(charge_rate_array, curr_index);
		metrics[EV_MAX_CHARGE_RATE] = findMax(charge_rate_array, curr_index);
		metrics[EV_AVG_CHARGE_RATE] = findAverage(charge_rate_array, curr_index);

		metrics[EV_MIN_BATTERY_SOC] = findMin(battery_SOC_array, curr_index);
		metrics[EV_MAX_BATTERY_SOC] = findMax(battery_SOC_array, curr_index);
		metrics[EV_AVG_BATTERY_SOC] = findAverage(battery_SOC_array, curr_index);
	}

	// wrap the arrays for next collection interval
	if (bOverran) {
		copyHistories(curr_index - 1, 0);
		copyHistories(curr_index, 1);
		curr_index = 2;
	} else {
		copyHistories(curr_index - 1, 0);
		curr_index = 1;
	}

	next_write = std::min(next_write + interval_length, gl_globalstoptime);
	first_write = false;
	return 1;
}

void metrics_collector::copyHistories (int from, int to) {
	time_array[to] = time_array[from];
	if (voltage_vll_array) voltage_vll_array[to] = voltage_vll_array[from];
	if (voltage_vln_array) voltage_vln_array[to] = voltage_vln_array[from];
	if (voltage_unbalance_array) voltage_unbalance_array[to] = voltage_unbalance_array[from];
	if (total_load_array) total_load_array[to] = total_load_array[from];
	if (hvac_load_array) hvac_load_array[to] = hvac_load_array[from];
	if (air_temperature_array) air_temperature_array[to] = air_temperature_array[from];
	if (set_cooling_array) set_cooling_array[to] = set_cooling_array[from];
	if (set_heating_array) set_heating_array[to] = set_heating_array[from];
	if (wh_load_array) wh_load_array[to] = wh_load_array[from];
	if (count_array) count_array[to] = count_array[from];
	if (real_power_array) real_power_array[to] = real_power_array[from];
	if (reactive_power_array) reactive_power_array[to] = reactive_power_array[from];
	if (real_power_loss_array) real_power_loss_array[to] = real_power_loss_array[from];
	if (reactive_power_loss_array) reactive_power_loss_array[to] = reactive_power_loss_array[from];
	if (trans_overload_status_array) trans_overload_status_array[to] = trans_overload_status_array[from];
	if (line_overload_status_array) line_overload_status_array[to] = line_overload_status_array[from];
	if (charge_rate_array) charge_rate_array[to] = charge_rate_array[from];
	if (battery_SOC_array) battery_SOC_array[to] = battery_SOC_array[from];
}

void metrics_collector::interpolate (double *a, int idx, double denom, double top) {
	a[idx] = a[idx-1] + (a[idx+1] - a[idx-1]) * top / denom;
}

void metrics_collector::interpolateHistories (int idx, TIMESTAMP t) {
	time_array[idx] = t;
	double denom = time_array[idx+1] - time_array[idx-1];
	double top = t - time_array[idx-1];
	if (voltage_vll_array) interpolate (voltage_vll_array, idx, denom, top);
	if (voltage_vln_array) interpolate (voltage_vln_array, idx, denom, top);
	if (voltage_unbalance_array) interpolate (voltage_unbalance_array, idx, denom, top);
	if (total_load_array) interpolate (total_load_array, idx, denom, top);
	if (hvac_load_array) interpolate (hvac_load_array, idx, denom, top);
	if (air_temperature_array) interpolate (air_temperature_array, idx, denom, top);
	if (set_cooling_array) interpolate (set_cooling_array, idx, denom, top);
	if (set_heating_array) interpolate (set_heating_array, idx, denom, top);
	if (wh_load_array) interpolate (wh_load_array, idx, denom, top);
	if (count_array) interpolate (count_array, idx, denom, top);
	if (real_power_array) interpolate (real_power_array, idx, denom, top);
	if (reactive_power_array) interpolate (reactive_power_array, idx, denom, top);
	if (real_power_loss_array) interpolate (real_power_loss_array, idx, denom, top);
	if (reactive_power_loss_array) interpolate (reactive_power_loss_array, idx, denom, top);
	if (charge_rate_array) interpolate (charge_rate_array, idx, denom, top);
	if (battery_SOC_array) interpolate (battery_SOC_array, idx, denom, top);
}

double metrics_collector::countPerc(int array[], int length) {
	double perc;
	double totalOnes = 0;
	for (int i = 0; i < length; i++) {
		if (array[i] == 1) totalOnes++;
	}
	perc = 100 * totalOnes/length;
	return perc;
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
	if (length < 2) return array[0];
	double sum = 0;
	double tlen = time_array[length-1] - time_array[0];
	for (int i = 1; i < length; i++) {
		double dA = 0.5 * (array[i] + array[i-1]) * (time_array[i] - time_array[i-1]);
		sum += dA;
	}
	return sum / tlen;
}

// helper class to find the median of a histogram, skipping explicit linear interpolation
struct HistBar {
	double height;  // aka frequency of observations, or # samples at this average array value
	double v1;
	double width;
	double avg;
	HistBar (TIMESTAMP t1, TIMESTAMP t2, double y1, double y2) {
		height = t2 - t1;
		v1 = y1;
		width = y2 - y1;
		avg = 0.5 * (y1 + y2);
	}
	bool operator < (const HistBar& bar) const {
		return avg < bar.avg;
	}
};

double metrics_collector::findMedian(double array[], int length) {
	std::vector<HistBar> Histogram;

	double nobs = 0.0;
	for (int i = 1; i < length; i++) {
		Histogram.push_back (HistBar (time_array[i-1], time_array[i], array[i-1], array[i]));
		nobs += Histogram[i-1].height;
	}

	std::sort(Histogram.begin(), Histogram.end());

	// find the median bar, and interpolate within it
	double cume = 0.0;
	for (int i = 0; i < length - 1; i++) {
		if ((cume + Histogram[i].height) >= 0.5 * nobs)	{ // found it
			return Histogram[i].v1 + (0.5 * nobs - cume) * Histogram[i].width / Histogram[i].height;

		}
		cume += Histogram[i].height;
	}
	return 0.0;
}

vol_violation metrics_collector::findOutLimit (bool firstCall, double array[], bool checkAbove, double limitVal, int length) {
	struct vol_violation result;
	double y1, y2, dt, t, slope;
	bool crossed;
	result.countViolation = 0;
	result.durationViolation = 0.0;

	for (int i = 1; i < length; i++) {
		dt = time_array[i] - time_array[i-1];
		y2 = array[i] - limitVal;   // work with distance above the limit
		y1 = array[i-1] - limitVal;
		crossed = false;
		if (checkAbove)	{
			if (y1 > 0.0 || y2 > 0.0) {
				if (y1 * y2 <= 0.0) crossed = true;
				if (firstCall) {
					result.countViolation += 1;
				} else if (y1 <= 0.0 && y2 > 0.0) {
					result.countViolation += 1;
				}
				if (crossed) {
					slope = (y2 - y1) / dt; // solve for t at the limit crossing
					t = -y1 / slope;
					assert (t >= 0.0 && t <= dt);
					if (y1 > 0.0) {  // add the violating sub-interval to the total duration
						result.durationViolation += t;
					} else {
						result.durationViolation += (dt - t);
					}
				} else { // this whole interval violates the limit
					result.durationViolation += dt;
				}
			}
		} else { // below; mirrors the checkAbove case
			if (y1 < 0.0 || y2 < 0.0) {
				if (y1 * y2 <= 0.0) crossed = true;
				if (firstCall) {
					result.countViolation += 1;
				} else if (y1 >= 0.0 && y2 < 0.0) {
					result.countViolation += 1;
				}
				if (crossed) {
					slope = (y2 - y1) / dt;
					t = -y1 / slope;
					assert (t >= 0.0 && t <= dt);
					if (y1 < 0.0) {
						result.durationViolation += t;
					} else {
						result.durationViolation += (dt - t);
					}
				} else {
					result.durationViolation += dt;
				}
			}
		}
		firstCall = false;
	}
	if (log_me) {
		printf("limit %g count %i duration %g\n", limitVal, result.countViolation, result.durationViolation);
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
    catch( const std::exception& ex){
        std::cerr << "Unhandled general exception in " << __func__ << ": " << ex.what() << std::endl;
    }
	return rv;
}

EXPORT int init_metrics_collector(OBJECT *obj){
    metrics_collector *my = OBJECTDATA(obj, metrics_collector);
    int rv = 0;
    try {
        rv = my->init(obj->parent);
    }
    catch (char *msg) {
        gl_error("init_metrics_collector: %s", msg);
    }
    catch (const char *msg) {
        gl_error("init_metrics_collector: %s", msg);
    }
    catch (const std::exception &ex) {
        std::cerr << "Unhandled general exception in " << __func__ << ": " << ex.what() << std::endl;
    }
	return rv;
}

EXPORT TIMESTAMP sync_metrics_collector(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass){
	TIMESTAMP rv = 0;
	metrics_collector *my = OBJECTDATA(obj, metrics_collector);
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
    catch(const std::exception& ex){
        std::cerr << "Unhandled general exception in " << __func__ << ": " << ex.what() << std::endl;
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
    catch (const std::exception& ex){
        std::cerr << "Unhandled general exception in " << __func__ << ": " << ex.what() << std::endl;
    }
	return rv;
}

EXPORT int isa_metrics_collector(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj, metrics_collector)->isa(classname);
}

// EOF








