/** $Id: battery.h,v 1.0 2008/07/18
	Copyright (C) 2008 Battelle Memorial Institute
	@file battery.h
	@addtogroup battery

 @{  
 **/

#ifndef _battery_H
#define _battery_H

#include <stdarg.h>

#include "generators.h"

EXPORT STATUS preupdate_battery(OBJECT *obj,TIMESTAMP t0, unsigned int64 delta_time);
EXPORT SIMULATIONMODE interupdate_battery(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS postupdate_battery(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass);


class battery : public gld_object
{
private:
	TIMESTAMP prev_time;
	int first_time_step;
	int number_of_phases_out;
	int prev_state; //1 is charging, 0 is nothing, -1 is discharging

	/* TODO: put private variables here */
	double n_series; //the number of cells in series
	double internal_battery_load; //the power out of the battery on the source side of the internal resistance
	double r_in; // the internal resistance of the battery as a function of soc
	double v_oc; // the open circuit voltage as a function of soc
	double v_t; // the terminal voltage as a function of soc and battery load.
	double p_br;

	bool deltamode_inclusive;	   //Boolean for deltamode calls - pulled from object flags
	bool first_run;
	bool enableDelta; // is true only if battery is use_internal_battery_model, and its parent inverter is FQM_CONSTANT_PQ mode

	gld_property *pCircuit_V[3];		//< pointer to the three voltages on three lines
	gld_property *pLine_I[3];			//< pointer to the three current on three lines
	gld_property *pLine12;			//< used in triplex metering
	gld_property *pPower;

	gld_property *pTout;
	gld_property *pSoc;
	gld_property *pBatteryLoad;
	gld_property *pSocReserve;
	gld_property *pRatedPower;

	//Inverter-related properies
	gld_property *peff; // parent inverter efficiency
	gld_property *pinverter_VA_Out; // inverter AC power output

	gld::complex value_Circuit_V[3];
	gld::complex value_Line_I[3];
	gld::complex value_Line12;

	double value_Tout;

	bool parent_is_meter;
	bool parent_is_triplex;
	bool parent_is_inverter;
	bool climate_object_found;

	void push_powerflow_currents(void);
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	set phases;	/**< device phases (see PHASE codes) */
	double power_transferred;
	enum GENERATOR_MODE {
		GM_UNKNOWN=0,
		GM_CONSTANT_V=1, 
		GM_CONSTANT_PQ, 
		GM_CONSTANT_PF, 
		GM_SUPPLY_DRIVEN, 
		GM_POWER_DRIVEN, 
		GM_VOLTAGE_CONTROLLED, 
		GM_POWER_VOLTAGE_HYBRID
	};
	enumeration gen_mode_v;  //operating mode of the generator


	//note battery panel will always operate under the SUPPLY_DRIVEN generator mode
	enum GENERATOR_STATUS {
		OFFLINE=1, 
		ONLINE=2
	};
	enumeration gen_status_v;

	enum ADDITIONAL_CONTROLS {
		AC_NONE=0,
		AC_LINEAR_TEMPERATURE=1
	};
	enumeration additional_controls;

	enum POWER_TYPE {
		DC=1, 
		AC=2
	};
	enumeration power_type_v;

	enum RFB_SIZE {
		SMALL=1, 
		MED_COMMERCIAL, 
		MED_HIGH_ENERGY, 
		LARGE, 
		HOUSEHOLD
	};
	enumeration rfb_size_v;

	enum BATTERY_STATE {
		BS_WAITING = 0,
		BS_CHARGING = 1,
		BS_DISCHARGING = 2,
		BS_FULL = 3,
		BS_EMPTY = 4,
		BS_CONFLICTED = 5,
	};
	enumeration battery_state;

		
	double power_set_high;
	double power_set_low;
	double power_set_high_highT;
	double power_set_low_highT;
	double voltage_set_high;
	double voltage_set_low;
	double deadband;
	double check_power;
	double pf;

	gld::complex last_current[3];
	double no_of_cycles;
	bool Iteration_Toggle;			// "Off" iteration tracker
	double parasitic_power_draw;
	double high_temperature;
	double low_temperature;
	double midpoint_temperature;
	double sensitivity;
	double check_power_high;
	double check_power_low;
	double B_scheduled_power;

	//Internal battery model parameters
	bool use_internal_battery_model;
	enum {LI_ION=1, LEAD_ACID};
	enumeration battery_type;
	double soc; //state of charge of the battery
	double bat_load; //current load of the battery
	double last_bat_load;
	double b_soc_reserve;

	TIMESTAMP state_change_time;


	//battery module parameters
	double v_max; //the maximum DC voltage of the battery in V
	double p_max; // the rated DC power the battery can supply or draw in W

	double e_max; // the battery's internal capacity in Wh
	double eta_rt; // the roundtrip efficiency of the battery at rated power.

	double deltat; // delta mode time interval in second
	unsigned int64 state_change_time_delta;
	double pre_soc; //store the soc value during iterations
	double Pout_delta; //Power output from parent inverter

	//*** LEGACY model parameters - ported from energy_storage before separation ***//
	double Rinternal;
	double Energy;
	double efficiency;

	double Max_P;//< maximum real power capacity in kW
	gld::complex V_Max;
	gld::complex I_Max;
	double E_Max;

	double base_efficiency;

	gld::complex V_Out;
	gld::complex I_Out;
	gld::complex VA_Out;
	gld::complex I_In;
	gld::complex V_Internal;
	gld::complex I_Internal;
	gld::complex VA_Internal;
	gld::complex I_Prev;
	double margin;
	double E_Next;

	bool recalculate;

	//*** End LEGACY model parameters ***//

public:
	/* required implementations */
	battery(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	//double timestamp_to_hours(TIMESTAMP t);
	TIMESTAMP rfb_event_time(TIMESTAMP t0, gld::complex power, double e);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);

	STATUS pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass);
	void update_soc(unsigned int64 delta_time);
	double check_state_change_time_delta(unsigned int64 delta_time, unsigned long dt);

	double calculate_efficiency(gld::complex voltage, gld::complex current);
	gld::complex calculate_v_terminal(gld::complex v, gld::complex i);

	gld_property *map_complex_value(OBJECT *obj, const char *name);
	gld_property *map_double_value(OBJECT *obj, const char *name);
public:
	static CLASS *oclass;
	static battery *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif
