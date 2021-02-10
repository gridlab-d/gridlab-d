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

	double prev_time;

public:
	//Example published/keeper values - can be changed
	double ES_DC_Voltage;  // unit V
	double ES_DC_Current;  // unit A
	double ES_DC_Power_Val; // unit w

	double Vbase_ES;  // Rated voltage of ES, unit V
	double Sbase_ES;  // Rated rating of ES, unit VA
	double Qbase_ES;  // Rated capacity of ES, unit Wh
	double SOC_0_ES;  // Initial state of charge
	double SOC_ES;    // State of charge
    double Q_ES;      // Energy of ES, unit Wh


	double E_ES_pu;   // No-load voltage, per unit
	double E0_ES_pu;  // Battery constant voltage, per unit
	double K_ES_pu;   // Polarisation voltage, per unit
    double A_ES_pu;   // Exponential zone amplitude, per unit
    double B_ES_pu;   // Exponential zone time constant inverse, per unit
    double V_ES_pu;   // ES terminal voltage, per unit
    double I_ES_pu;   // ES output current, per unit
    double R_ES_pu;   // ES internal resitance, per unit
    double P_ES_pu;   // ES ouptut active power, per unit



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
