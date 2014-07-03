/** $Id: central_dg_control.h,v 1.0 2008/07/16
	Copyright (C) 2013 Battelle Memorial Institute
	@file central_dg_control.h
	@addtogroup central_dg_control

 @{  
 **/

#ifndef _central_dg_control_H
#define _central_dg_control_H

#include <stdarg.h>
#include "gridlabd.h"
#include "generators.h"

EXPORT STATUS postupdate_central_dg_control(OBJECT *obj, complex *useful_value, unsigned int mode_pass);

//Inverter state variable structure


//inverter extends power_electronics
class central_dg_control
{
private:
	
	complex *FreqPower;					//Link to bus "frequency-power weighted" accumulation
	complex *TotalPower;
	double PO_prev_it;
	double QO_prev_it;
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
	bool pf_is_low;
	bool pf_is_high;
	bool P_is_high;
	double P[3];
	double P_3p;
	double Q[3];
	double Q_3p;
	complex S_3p;
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
	double pf_meas[3];
	double pf_meas_3p;
	double pf_low;
	double pf_high;
	double S_peak;
	double **inverter_P_a_set;
	double **inverter_P_b_set;
	double **inverter_P_c_set;
	double **inverter_Q_a_set;
	double **inverter_Q_b_set;
	double **inverter_Q_c_set;
	double **inverter_S_rated_set;
	double **battery_qs;
	double *inverter_P_a_disp_set;
	double *inverter_P_b_disp_set;
	double *inverter_P_c_disp_set;
	double *inverter_Q_a_disp_set;
	double *inverter_Q_b_disp_set;
	double *inverter_Q_c_disp_set;
	double all_inverter_S_rated;
	double all_battery_S_rated;
	double all_solar_S_rated;

	

	enum INVERTER_TYPE {TWO_PULSE=0, SIX_PULSE=1, TWELVE_PULSE=2, PWM=3, FOUR_QUADRANT = 4};
	enumeration inverter_type_v;
	
	
	complex VA_Out;
				//Tracking variable for previous "new time" run


public:
	/* required implementations */
	bool *get_bool(OBJECT *obj, char *name);
	central_dg_control(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS post_deltaupdate(complex *useful_value, unsigned int mode_pass);
	int *get_enum(OBJECT *obj, char *name);
public:
	static CLASS *oclass;
	static central_dg_control *defaults;
	static CLASS *plcass;
	complex *get_complex(OBJECT *obj, char *name);
	double *get_double(OBJECT *obj, char *name);
	double fmin(double a, double b);
	double fmin(double a, double b, double c);
	double fmax(double a, double b);
	double fmax(double a, double b, double c);
	complex complex_exp(double angle);
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
/** $Id: central_dg_control.h,v 1.0 2008/06/16
	@file central_dg_control.h
	@addtogroup central_dg_control
	@ingroup MODULENAME

 @{  
 **/

#ifndef _inverter_H
#define _inverter_H

#include <stdarg.h>
#include "gridlabd.h"

class central_dg_control {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
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
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMPP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
