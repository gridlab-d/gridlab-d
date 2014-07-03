// $Id: volt_var_control.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2010 Battelle Memorial Institute

#ifndef _VOLT_VAR_CONTROL_H
#define _VOLT_VAR_CONTROL_H

#include "powerflow.h"
#include "regulator.h"
#include "capacitor.h"

typedef enum {
		STANDBY=0,		//CVVC is off
		ACTIVE=1		//CVVC is on
		} VOLTVARSTATE;

class volt_var_control : public powerflow_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	enumeration control_method;		///< Control method for IVVC controller (On or off)
	double cap_time_delay;				//Default delay for capacitors
	double reg_time_delay;				//Default delay for regulator
	double desired_pf;					//Desired power factor for the system
	double d_max;						//Scaling constant for capacitor switching on - typically 0.3 - 0.6
	double d_min;						//Scaling constant for capacitor switching off - typically 0.1 - 0.4
	OBJECT *substation_lnk_obj;			//Power factor measurement link
	char1024 regulator_list;			//List of regulators, separated by commas
	char1024 capacitor_list;			//List of controllable capacitors, separated by commas
	char1024 measurement_list;			//List of available voltage measurement points, separated by commas with the regulator index between
	char1024 minimum_voltage_txt;		//List of minimum allowable voltage of the system
	char1024 maximum_voltage_txt;		//List of maximum allowable voltage of the system
	char1024 desired_voltage_txt;		//List of desired operating point voltages
	char1024 max_vdrop_txt;				//List of maximum allowed voltage drops
	char1024 vbw_low_txt;				//List of bandwidth (deadband) for low loading
	char1024 vbw_high_txt;				//List of bandwidth (deadband) for high loading
	set pf_phase;						//Phases for power factor monitoring to occur
	double react_pwr;					//Reactive power quantity at the substation
	double curr_pf;						//Current pf at the substation
	bool pf_signed;						//Flag to indicate if a signed pf value should be maintained, or just a "deadband around 1"

	volt_var_control(MODULE *mod);
	volt_var_control(CLASS *cl=oclass):powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);

	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);

private:
	VOLTVARSTATE prev_mode;						//Previous mode of Volt-VAr controller - used to transitions
	link_object *substation_link;						//Object to obtain power-factor measurments
	regulator **pRegulator_list;				//Regulators
	regulator_configuration **pRegulator_configs;	//Regulator configurations
	capacitor **pCapacitor_list;				//Capacitors to monitor
	double *Capacitor_size;						//Size of various capacitors
	double *minimum_voltage;					//Minimum allowable voltage of the system
	double *maximum_voltage;					//Maximum allowable voltage of the system
	double *desired_voltage;					//Desired operating point voltages
	double *max_vdrop;							//Maximum allowed voltage drops
	double *vbw_low;							//Bandwidth (deadband) for low loading
	double *vbw_high;							//Bandwidth (deadband) for high loading

	node ***pMeasurement_list;					//Measurement points - they are assumed to be nodes at some level
	node **RegToNodes;							//To Node (Load side) of regulators - for voltage VO measurements
	int num_caps;								//Number of capacitors under our control
	int num_regs;								//Number of regulators under our control
	int *num_meas;								//Number of voltage measurements to monitor
	bool Regulator_Change;						//Flag to indicate a regulator change is in progress - used to hold off VAr-based switching
	double *reg_step_up;						//Regulator step for upper taps
	double *reg_step_down;						//Regulator step for lower taps (may be same as reg_step_up)
	double *RegUpdateTimes;						//Regulator progression times (differential)
	TIMESTAMP *TRegUpdate;						//Regulator update times to proceed towards (absolute)
	double *CapUpdateTimes;						//Capacitor progression times (differential)
	TIMESTAMP TCapUpdate;						//Capacitor time to proceed towards
	bool TUpdateStatus;							//Status variable for TUpdate - used for transitioning between Active/Standby states
	bool first_cycle;
	regulator_configuration::Control_enum *PrevRegState;	//Previous state of the regulators
	capacitor::CAPCONTROL *PrevCapState;					//Previous state of the capacitors
	TIMESTAMP prev_time;
	void size_sorter(double *cap_size, int *cap_Index, int cap_num, double *temp_cap_size, int *temp_cap_Index);					//Capacitor size sorting function (recursive)
	char *dbl_token(char *start_token, double *dbl_val);	//Function to parse a comma-separated list to get the next double (or the last double)
	char *obj_token(char *start_token, OBJECT **obj_val);	//Function to parse a comma-separated list to get the next object (or the last object)
};

#endif // _VOLT_VAR_CONTROL_H
