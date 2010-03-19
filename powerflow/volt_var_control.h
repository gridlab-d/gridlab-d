// $Id: volt_var_control.h 2010-02-26 15:00:00Z d3x593 $
//	Copyright (C) 2010 Battelle Memorial Institute

#ifndef _VOLT_VAR_CONTROL_H
#define _VOLT_VAR_CONTROL_H

#include "powerflow.h"
#include "regulator.h"
#include "capacitor.h"

typedef enum {
		STANDBY=0,		//IVVC is off
		ACTIVE=1		//IVVC is on
		} VOLTVARSTATE;

class volt_var_control : public powerflow_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	VOLTVARSTATE control_method;		//Control method for IVVC controller (On or off)
	double cap_time_delay;				//Default delay for capacitors
	double reg_time_delay;				//Default delay for regulator
	double desired_pf;					//Desired power factor for the system
	double d_max;						//Scaling constant for capacitor switching on - typically 0.3 - 0.6
	double d_min;						//Scaling constant for capacitor switching off - typically 0.1 - 0.4
	OBJECT *substation_lnk_obj;			//Power factor measurement link
	OBJECT *feeder_regulator_obj;		//Feeder level regulator
	char1024 capacitor_list;			//List of controllable capacitors, separated by commas
	char1024 measurement_list;			//List of available voltage measurement points, separated by commas
	double minimum_voltage;				//Minimum allowable voltage of the system
	double maximum_voltage;				//Maximum allowable voltage of the system
	double desired_voltage;				//Desired operating point voltage
	double max_vdrop;					//Maximum allowed voltage drop
	double vbw_low;						//Bandwidth (deadband) for low loading
	double vbw_high;					//Bandwidth (deadband) for high loading
	set pf_phase;						//Phases for power factor monitoring to occur
	double react_pwr;					//Reactive power quantity at the substation
	double curr_pf;						//Current pf at the substation

	volt_var_control(MODULE *mod);
	volt_var_control(CLASS *cl=oclass):powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);

	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);

private:
	VOLTVARSTATE prev_mode;						//Previous mode of Volt-VAr controller - used to transitions
	link *substation_link;						//Object to obtain power-factor measurments
	regulator *feeder_regulator;				//Feeder regulator link
	regulator_configuration *feeder_reg_config;	//Feeder regulator configuration
	capacitor **pCapacitor_list;				//Capacitors to monitor
	double *Capacitor_size;						//Size of various capacitors
	node **pMeasurement_list;					//Measurement points - they are assumed to be nodes at some level
	node *RegToNode;							//To Node (Load side) of regulator - for voltage VO measurements
	int num_caps;								//Number of capacitors under our control
	int num_meas;								//Number of voltage measurements to monitor
	bool Regulator_Change;						//Flag to indicate a regulator change is in progress - used to hold off VAr-based switching
	double reg_step_up;							//Feeder regulator step for upper taps
	double reg_step_down;						//Feeder regulator step for lower taps (may be same as reg_step_up)
	TIMESTAMP TRegUpdate;						//Regulator update time to proceed towards
	TIMESTAMP TCapUpdate;						//Capacitor time to proceed towards
	bool TUpdateStatus;							//Status variable for TUpdate - used for transitioning between Active/Standby states
	bool first_cycle;
	regulator_configuration::Control_enum PrevRegState;	//Previous state of the regulator
	capacitor::CAPCONTROL *PrevCapState;				//Previous state of the capacitors
	TIMESTAMP prev_time;
	void size_sorter(double *cap_size, int *cap_Index, int cap_num, double *temp_cap_size, int *temp_cap_Index);					//Capacitor size sorting function (recursive)
};

#endif // _VOLT_VAR_CONTROL_H
