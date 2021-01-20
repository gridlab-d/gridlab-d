#ifndef GLD_GENERATORS_energy_storage_H_
#define GLD_GENERATORS_energy_storage_H_

#include "generators.h"

EXPORT SIMULATIONMODE interupdate_energy_storage(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS dc_object_update_energy_storage(OBJECT *us_obj, OBJECT *calling_obj, bool init_mode);

class energy_storage : public gld_object
{
private:
	bool deltamode_inclusive; //Boolean for deltamode calls - pulled from object flags
	bool first_sync_delta_enabled;

	//Inverter connections
	gld_property *inverter_voltage_property;
	gld_property *inverter_current_property;
	gld_property *inverter_power_property;

	//Default voltage and current values, if ran "headless"
	double default_voltage_array;
	double default_current_array;
	double default_power_array;

public:
	//Example published/keeper values - can be changed
	double ES_DC_Voltage;
	double ES_DC_Current;
	double ES_DC_Power_Val;

	/* required implementations */
	energy_storage(MODULE *module);
	int create(void);
	int init(OBJECT *parent);

	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS energy_storage_dc_update(OBJECT *calling_obj, bool init_mode);

public:
	static CLASS *oclass;
	static energy_storage *defaults;
};

#endif // GLD_GENERATORS_energy_storage_H_
