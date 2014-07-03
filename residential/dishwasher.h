/** $Id: dishwasher.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file dishwasher.h
	@addtogroup dishwasher
	@ingroup residential

 @{
 **/

#ifndef _dishwasher_H
#define _dishwasher_H

#include "residential.h"
#include "residential_enduse.h"

class dishwasher : public residential_enduse
{
public:
	typedef enum {	dishwasher_STOPPED=0,		///< motor is stopped
			dishwasher_STALLED=1,						///< motor is stalled
			dishwasher_TRIPPED=2,						///< motor is tripped
			dishwasher_COIL_ONLY=3,				//<only the coil and motor are operating
			dishwasher_MOTOR_COIL_ONLY=4,	
			dishwasher_MOTOR_ONLY=5,						///< only the motor is running (air fluff)
			dishwasher_CONTROL_ONLY=6,
			dishwasher_HEATEDDRY_ONLY=7
	};										///< control state
	enumeration state;



	double circuit_split;				///< -1=100% negative, 0=balanced, +1=100% positive
	bool is_240;						///< load connected at 220 
	
	bool control_check1;
	bool control_check2;
	bool control_check3;
	bool control_check4;
	bool control_check5;
	bool control_check6;
	bool control_check7;
	bool control_check8;
	bool control_check9;
	bool control_check10;
	bool control_check11;
	bool control_check12;
	bool control_check_temp;

	bool new_running_state;

	bool motor_only_check1;
	bool motor_only_check2;
	bool motor_only_check3;
	bool motor_only_check4;
	bool motor_only_check5;
	bool motor_only_check6;
	bool motor_only_check7;
	bool motor_only_check8;
	bool motor_only_check9;


	bool motor_only_temp1;
	bool motor_only_temp2;
	bool motor_only_25_repeat_one;

	bool motor_coil_only_check1;
	bool motor_coil_only_check2;


	bool heateddry_check1;
	bool heateddry_check2;

	bool coil_only_check1;
	bool coil_only_check2;
	bool coil_only_check3;
	bool Heateddry_option_check;


	double zero_power;
	double motor_power;					///< installed dishwasher motor power [W] (default = random uniform between 150-350 W)
	double coil_power[3];					///< installed heating coil power [W] (default = random uniform between 3500-5000 W, 0 for gas)
	double controls_power;
	double dishwasher_demand;				///< amount of demand added per hour (units/hr)
	double enduse_queue;				///< accumulated demand (units)
	double cycle_duration;				///< typical cycle runtime (s)
	double cycle_time;					///< remaining time in main cycle (s)
	double state_time;					///< remaining time in current state (s)
	double count_motor_only;
	double count_motor_only1;
	double count_motor_only_25;
	double count_coil_only;
	double count_motor_only_68;
	double daily_dishwasher_demand;
	double dishwasher_run_prob;
	double queue_min;
	double queue_max;

	double count_control_only;
	double count_control_only1;

	//****Changes by Niru
	TIMESTAMP next_change_time;			///< time when the current change changes to the next state (s)
	//***changes stop here

	double stall_voltage;				///< voltage at which the motor stalls
	double start_voltage;				///< voltage at which motor can start
	complex stall_impedance;			///< impedance of motor when stalled
	double trip_delay;					///< stalled time before thermal trip
	double reset_delay;					///< trip time before thermal reset and restart
	double heat_fraction;				///< internal gain fraction of installed power
	
	//double load_relative_humidity;		///< relative amount of water in load - will be used to determine cycle length
	//double slope_of_rh_vs_time;			///< slope of how fast water is removed from load as function of time
	double energy_baseline;				///< amount of energy needed to dry clothes in baseline mode
	double energy_used;					///< amount of energy used in current cycle to dry clothes
	double actual_dishwasher_demand;
	double pulse_interval[19];			///< length of time of each pulse section for a dishwasher operation
	
	//double time_baseline;				///< length of time needed to run baseline at nominal voltage
	//double time_eco;					///< length of time needed to run eco at nominal voltage
	double energy_needed;				///< total energy needed to dry clothes (used for switching modes)
	double total_power;					///< instaneous power draw of the unit
	double motor_on_off;				///< boolean logic to track the state of dishwasher
	double motor_coil_on_off;
	double both_coils_on_off;
	/*double response_criticalmode;
	double response_highmode;*/

	TIMESTAMP time_state;				///< time in current state

	TIMESTAMP return_time;

public:
	static CLASS *oclass, *pclass;
	static dishwasher *defaults;

	dishwasher(MODULE *module);
	~dishwasher();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP last_t;
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	double update_state(double dt); //, TIMESTAMP t1=0);		///< updates the load struct and returns the time until expected state change

};

#endif // _dishwasher_H

/**@}**/
