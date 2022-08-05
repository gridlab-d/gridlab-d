/*
 * controllerdg.h
 *
 *  Created on: Sep 21, 2016
 *      Author: tang526
 */

#ifndef _controller_dg_H_
#define _controller_dg_H_

#include <stdarg.h>

#include "gridlabd.h"
#include "generators.h"

EXPORT SIMULATIONMODE interupdate_controller_dg(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS postupdate_controller_dg(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass);

//controller state variable structure
typedef struct {
	double w_measured; // transient speed of each generator in p.u.
	double x;		   //State variable
	double Pref_ctrl;  //Reference real power output/bias (per-unit) for controller_dg
	double wref_ctrl;  //Reference frequency/bias for conrtoller_dg

	double Vset_ref; // Vset reference for each DG, values obtained at the begining of the delta mode
	double x_QV;
	double Vset_ctrl;

	} CTRL_VARS;

//central controller variable structure
typedef struct {
	CTRL_VARS *curr_state;  //Current state of all variables
	CTRL_VARS *next_state;  //Next state of all variables
} CTRL_Gen;

//DG Variables
typedef struct {
	OBJECT *obj;
	gld_property *Pref_prop;
	gld_property *Vset_prop;
	gld_property *omega_prop;
	double Pref;
	double Vset;
	double omega;
	double omega_ref;
	OBJECT *parent;
} DG_VARS;

//Node variables
typedef struct {
	OBJECT *obj;
	gld_property *voltage_A_prop;
	gld_property *voltage_B_prop;
	gld_property *voltage_C_prop;
	gld::complex voltage_A;
	gld::complex voltage_B;
	gld::complex voltage_C;
	double nominal_voltage;
} NODE_VARS;

//Switch variables
typedef struct {
	OBJECT *obj;
	gld_property *status_prop;
	gld_property *power_in_prop;
	gld_property *power_out_A_prop;
	gld_property *power_out_B_prop;
	gld_property *power_out_C_prop;
	gld::complex power_in;
	gld::complex power_out_A;
	gld::complex power_out_B;
	gld::complex power_out_C;
	enumeration status_val;
} SWITCH_VARS;

class controller_dg: public gld_object
{
private:
	bool first_run;
	bool flag_switchOn;
	unsigned int64 controlTime;

	FINDLIST *switches;
	DG_VARS *pDG;
	FINDLIST *dgs;

	CTRL_Gen **ctrlGen; 		   // Pointer to all the controls of each generator
	NODE_VARS *GenPobj;

	NODE_VARS *Switch_Froms;
	SWITCH_VARS *pSwitchObjs;

	double *prev_Pref_val;		   //Previous value of x - used for delta-exiting convergence check
	double *prev_Vset_val;		   // Previous value of Vset - used for delta-exiting convergence check

	int dgswitchFound; 		   // Index for storing found switches that connected to the generators

	CTRL_VARS predictor_vals;	   //Predictor pass values of variables
	CTRL_VARS corrector_vals;	   //Corrector pass values of variables

	bool deltamode_inclusive;	   //Boolean for deltamode calls - pulled from object flags
	double *mapped_freq_variable;  //Mapping to frequency variable in powerflow module - deltamode updates

	gld_property *map_complex_value(OBJECT *obj, const char *name);
	gld_property *map_double_value(OBJECT *obj, const char *name);
public:
	char32 controlled_dgs;

	double omega_ref;
	double nominal_voltage;

	double kp;
	double ki;
	double gain;

	double kp_QV;
	double ki_QV;
	double gain_QV;

	//Convergence criteria
	double controller_Pref_convergence_criterion;

public:
	static CLASS *oclass;
	static controller_dg *defaults;
	STATUS apply_dynamics(CTRL_VARS *curr_time, CTRL_VARS *curr_delta, double deltaT, int index);
	STATUS init_dynamics(CTRL_VARS *curr_time, int index);

public:
	/* required implementations */
	controller_dg(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	//STATUS deltaupdate(unsigned int64 dt, unsigned int iteration_count_val);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass);

};

#endif /* GENERATORS_CONTROLLER_DG_H_ */
