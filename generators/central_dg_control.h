/** $Id: central_dg_control.h,v 1.0 2008/07/16
	Copyright (C) 2013 Battelle Memorial Institute
	@file central_dg_control.h
	@addtogroup central_dg_control

 @{  
 **/

#ifndef _central_dg_control_H
#define _central_dg_control_H

#include <stdarg.h>

#include "generators.h"

EXPORT STATUS postupdate_central_dg_control(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass);

//Inverter state variable structure


//inverter extends power_electronics
class central_dg_control
{
private:
	gld_property *pPower_Meas[3];
protected:
	/* TODO: put unpublished but inherited variables */
public:
	//char32 inverter_group;
	//char32 battery_group;
	//char32 solar_group;
	char32 controlled_objects;
	inverter **inverter_set;
	inverter ***battery_inverter_set;
	inverter ***solar_inverter_set;
	battery **battery_set;
	solar **solar_set;
	OBJECT *feederhead_meter;
	int controlled_count;
	int inverter_count;
	int battery_count;
	int solar_count;
	int battery_inverter_count;
	int solar_inverter_count;
	typedef enum CONTROL_MODE {NO_SETTING=-1, NO_CONTROL=0, CONSTANT_PF=1, PEAK_SHAVING=2} control_mode;
	control_mode control_mode_setting[4];
	control_mode active_control_mode;
	double P[3];
	double P_3p;
	double Q[3];
	double Q_3p;
	gld::complex S_3p;
	double P_disp_3p;
	double Q_disp_3p;
	double P_gen[3];
	double P_gen_3p;
	double Q_gen[3];
	double Q_gen_3p;
	double P_gen_solar[3];
	double P_gen_solar_3p;
	double Q_gen_solar[3];
	double Q_gen_solar_3p;
	double P_gen_battery[3];
	double P_gen_battery_3p;
	double Q_gen_battery[3];
	double Q_gen_battery_3p;
	double pf_meas_3p;
	double pf_low;
	double pf_high;
	double S_peak;
	double all_inverter_S_rated;
	double all_battery_S_rated;
	double all_solar_S_rated;

public:
	/* required implementations */
	central_dg_control(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static central_dg_control *defaults;
	static CLASS *plcass;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif
