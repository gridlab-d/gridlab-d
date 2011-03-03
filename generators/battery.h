/** $Id: battery.h,v 1.0 2008/07/18
	Copyright (C) 2008 Battelle Memorial Institute
	@file battery.h
	@addtogroup battery

 @{  
 **/

#ifndef _battery_H
#define _battery_H

#include <stdarg.h>
#include "../powerflow/powerflow_object.h"
//#include "../powerflow/node.h"

#include "gridlabd.h"
#include "energy_storage.h"


class battery : public energy_storage
{
private:
	double number_of_phases_out;
	TIMESTAMP prev_time;
	int first_time_step;
	
	int prev_state; //1 is charging, 0 is nothing, -1 is discharging

	/* TODO: put private variables here */
	//complex AMx[3][3];//generator impedance matrix

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
	} gen_mode_v;  //operating mode of the generator 

	//note battery panel will always operate under the SUPPLY_DRIVEN generator mode
	enum GENERATOR_STATUS {
		OFFLINE=1, 
		ONLINE=2
	} gen_status_v;

	enum ADDITIONAL_CONTROLS {
		AC_NONE=0,
		AC_LINEAR_TEMPERATURE=1
	} additional_controls;

	enum POWER_TYPE {
		DC=1, 
		AC=2
	} power_type_v;

	enum RFB_SIZE {
		SMALL=1, 
		MED_COMMERCIAL, 
		MED_HIGH_ENERGY, 
		LARGE, 
		HOUSEHOLD
	} rfb_size_v;

	enum BATTERY_STATE {
		BS_WAITING = 0,
		BS_CHARGING = 1,
		BS_DISCHARGING = 2,
		BS_FULL = 3,
		BS_EMPTY = 4,
		BS_CONFLICTED = 5,
	} battery_state;

		
	complex *pCircuit_V;		//< pointer to the three voltages on three lines
	complex *pLine_I;			//< pointer to the three current on three lines
	complex *pLine12;			//< used in triplex metering
	complex *pPower;
	double power_set_high;
	double power_set_low;
	double power_set_high_highT;
	double power_set_low_highT;
	double voltage_set_high;
	double voltage_set_low;
	double deadband;
	double check_power;
	double pf;
	//double lockout_time;
	//int lockout_flag;
	//TIMESTAMP next_time;
	complex last_current[3];
	double no_of_cycles;
	bool Iteration_Toggle;			// "Off" iteration tracker
	bool *NR_mode;			//Toggle for NR solving cycle.  If not NR, just goes to fals
	double *pTout;
	double *pSolar;
	double parasitic_power_draw;
	double high_temperature;
	double low_temperature;
	double midpoint_temperature;
	double sensitivity;
	double check_power_high;
	double check_power_low;
	double B_scheduled_power;

public:
	/* required implementations */
	battery(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	//double timestamp_to_hours(TIMESTAMP t);
	TIMESTAMP rfb_event_time(TIMESTAMP t0, complex power, double e);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);

	bool *get_bool(OBJECT *obj, char *name);
	double calculate_efficiency(complex voltage, complex current);
	complex *get_complex(OBJECT *obj, char *name);
	complex calculate_v_terminal(complex v, complex i);

public:
	static CLASS *oclass;
	static battery *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
/** $Id: battery.h,v 1.0 2008/07/18
	@file battery.h
	@addtogroup battery
	@ingroup MODULENAME

 @{  
 **/

#ifndef _battery_H
#define _battery_H

#include <stdarg.h>
#include "gridlabd.h"

class battery {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
public:
	/* required implementations */
	battery(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static battery *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
