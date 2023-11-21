/* SA: commented out
#ifndef GLD_GENERATORS_energy_storage_H_
#define GLD_GENERATORS_energy_storage_H_*/

// SA: Renaming the directive checks
#ifndef GLD_GENERATORS_DC_LINK_H_
#define GLD_GENERATORS_DC_LINK_H_


#include "generators.h"


/* SA: commented out the function names foe energy_stoarge and added dc_link
EXPORT SIMULATIONMODE interupdate_energy_storage(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS dc_object_update_energy_storage(OBJECT *us_obj, OBJECT *calling_obj, bool init_mode); */

EXPORT SIMULATIONMODE interupdate_dc_link(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS dc_object_update_dc_link(OBJECT *us_obj, OBJECT *calling_obj, bool init_mode);


/* SA: commented out class energy_storage : public gld_object and added class dc_link : public gld_object
class energy_storage : public gld_object */
class dc_link : public gld_object
{
private:

	// SA: Adding the inverter object pointers
	OBJECT *inv1;
	OBJECT *inv2;

	// SA: Adding inverter connections --- for inverter1
	gld_property *inverter1_voltage_property;
	gld_property *inverter1_current_property;
	gld_property *inverter1_power_property;

	// SA: Adding inverter connections --- for inverter2
	gld_property *inverter2_voltage_property;
	gld_property *inverter2_current_property;
	gld_property *inverter2_power_property;

	bool deltamode_inclusive; //Boolean for deltamode calls - pulled from object flags

	/*  SA: commented out*
	//Inverter connections
	gld_property *inverter_voltage_property;
	gld_property *inverter_current_property;
	gld_property *inverter_power_property;

	//Default voltage and current values, if ran "headless"
	double default_voltage_array;
	double default_current_array;
	double default_power_array; */

	double prev_time; 

public:
	// SA: Adding variables defination
	double inv1_current;
	double inv1_voltage;
	double inv1_power;
	double inv2_current;
	double inv2_voltage;
	double inv2_power;

	double deltat; // Added as a global variable

	/*SA: commented out
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
    double P_ES_pu;   // ES ouptut active power, per unit  */

	//++++++++++++++++++++++ HM +++++++++++++++++++++++++++++

	double Vc;  // DC-Link capacitor voltage in volts
	double Vco;    // Initial DC-Link capacitor voltagae in volts
	double Vc_pu;  // DC-Link capacitor voltage in pu
	double Vco_pu;    // Initial DC-Link capacitor voltagae in pu
	double Vbase;  // Rated DC-Link voltage
	double dVc;  // Differntial Capacitor voltage
	double C;  // DC-Link Capacitance

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/* required implementations */
	// SA: commented energy_storage(MODULE *module); and added dc_link(MODULE *module);
	//energy_storage(MODULE *module);
	dc_link(MODULE *module);
	int create(void);
	int init(OBJECT *parent);

	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	/* SA: commented out STATUS energy_storage_dc_update(OBJECT *calling_obj, bool init_mode); and added STATUS dc_link_dc_update(OBJECT *calling_obj, bool init_mode);
	STATUS energy_storage_dc_update(OBJECT *calling_obj, bool init_mode); */
	STATUS dc_link_dc_update(OBJECT *calling_obj, bool init_mode);

public:
	static CLASS *oclass;
	/* SA: commented out static energy_storage *defaults; and added static dc_link *defaults;
	static energy_storage *defaults; */
	static dc_link *defaults;
};

#endif // GLD_GENERATORS_energy_storage_H_
