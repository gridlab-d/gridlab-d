/** $Id: dryer.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file dryer.h
	@addtogroup dryer
	@ingroup residential

 @{
 **/

#ifndef _DRYER_H
#define _DRYER_H

#include "residential.h"
#include "residential_enduse.h"

class dryer : public residential_enduse
{
public:
	typedef enum {	DRYER_STOPPED=0,		///< motor is stopped
			DRYER_STALLED=1,						///< motor is stalled
			DRYER_TRIPPED=2,						///< motor is tripped
			DRYER_MOTOR_COIL_ONLY=3,				///<only the coil and motor are operating
			DRYER_MOTOR_ONLY=4,						///< only the motor is running (air fluff)
			DRYER_CONTROL_ONLY=5					///< only the controls are running
	};										///< control state



	double circuit_split;				///< -1=100% negative, 0=balanced, +1=100% positive
	bool is_240;						///< load connected at 220 
	
	bool control_check;					///< logic checks

	bool new_running_state;
	bool motor_only_check1;
	bool motor_only_check2;
	bool motor_only_check3;
	bool motor_only_check4;
	bool motor_only_check5;
	bool motor_only_check6;

	bool motor_coil_only_check1;
	bool motor_coil_only_check2;
	bool motor_coil_only_check3;
	bool motor_coil_only_check4;
	bool motor_coil_only_check5;
	bool motor_coil_only_check6;

	bool dryer_on;
	bool dryer_check;
	bool dryer_ready;



	double motor_power;					///< installed dryer motor power [W] (default = random uniform between 150-350 W)
	double coil_power[1];				///< installed heating coil power [W] (default = random uniform between 4500-6000 W, 0 for gas)
	double controls_power;
	double daily_dryer_demand;			///< amount of demand added per hour (units/hr)
	double enduse_queue;				///< accumulated demand (units)
	double cycle_duration;				///< typical cycle runtime (s)
	double cycle_time;					///< remaining time in main cycle (s)
	double state_time;					///< remaining time in current state (s)
	
	double dryer_run_prob;
	double next_t;
	double dryer_turn_on;
	double queue_min;
	double queue_max;

	//****Changes by Niru
	TIMESTAMP next_change_time;			///< time when the current change changes to the next state (s)
	//***changes stop here

	double stall_voltage;				///< voltage at which the motor stalls
	double start_voltage;				///< voltage at which motor can start
	complex stall_impedance;			///< impedance of motor when stalled
	double trip_delay;					///< stalled time before thermal trip
	double reset_delay;					///< trip time before thermal reset and restart
	double heat_fraction;				///< internal gain fraction of installed power
	
	TIMESTAMP start_time;
	
	double energy_baseline;				///< amount of energy needed to dry clothes in baseline mode
	double energy_used;					///< amount of energy used in current cycle to dry clothes
	double actual_dryer_demand;
	double pulse_interval[10];			///< length of time of each pulse section for a dryer operation
	
	double energy_needed;				///< total energy needed to dry clothes (used for switching modes)
	double total_power;					///< instaneous power draw of the unit
	double motor_on_off;				///< boolean logic to track the state of dryer
	double motor_coil_on_off;


	TIMESTAMP time_state;				///< time in current state

	TIMESTAMP return_time;

	enumeration state;

public:
	static CLASS *oclass, *pclass;
	static dryer *defaults;

	dryer(MODULE *module);
	~dryer();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP last_t;
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	double update_state(double dt);		///< updates the load struct and returns the time until expected state change

};

#endif // _DRYER_H

/**@}**/