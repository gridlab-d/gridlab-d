/** $Id: clotheswasher.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file clotheswasher.h
	@addtogroup clotheswasher
	@ingroup residential

 @{
 **/

#ifndef _clotheswasher_H
#define _clotheswasher_H

#include "residential.h"
#include "residential_enduse.h"

class clotheswasher : public residential_enduse
{
public:
	double circuit_split;				///< -1=100% negative, 0=balanced, +1=100% positive
	double motor_power;					///< installed clotheswasher motor power [W] (default = random uniform between 500 - 750 W)
	double enduse_demand;				///< amount of demand added per hour (units/hr)
	double enduse_queue;				///< accumulated demand (units)
	double cycle_duration;				///< typical cycle runtime (s)
	double cycle_time;					///< remaining time in main cycle (s)
	double state_time;					///< remaining time in current state (s)
	double stall_voltage;				///< voltage at which the motor stalls
	double start_voltage;				///< voltage at which motor can start
	complex stall_impedance;			///< impedance of motor when stalled
	double trip_delay;					///< stalled time before thermal trip
	double reset_delay;					///< trip time before thermal reset and restart
	double heat_fraction;				///< internal gain fraction of installed power
	TIMESTAMP time_state;				///< time in current state
	bool starttime;
	typedef enum {
		STOPPED=0,						///< motor is stopped
		RUNNING=1,						///< motor is running
		STALLED=2,						///< motor is stalled
		TRIPPED=3,						///< motor is tripped
		PREWASH=4,
		WASH=5,
		SPIN1=6,
		SPIN2=7,
		SPIN3=8,
		SPIN4=9
	};
	enumeration state;							///< control state

public:
	static CLASS *oclass, *pclass;
	static clotheswasher *defaults;

	double Is_on;

	bool new_running_state;
	bool critical_cycle;
	double clothesWasherPower;
	double normal_perc;
	double perm_press_perc;

	double NORMAL_PREWASH_POWER;
	double NORMAL_WASH_POWER;
	double NORMAL_SPIN_LOW_POWER;	
	double NORMAL_SPIN_MEDIUM_POWER;	
	double NORMAL_SPIN_HIGH_POWER;	
	double NORMAL_SMALLWASH_POWER;	

	double NORMAL_PREWASH_ENERGY;
	double NORMAL_WASH_ENERGY;
	double NORMAL_SPIN_LOW_ENERGY;	
	double NORMAL_SPIN_MEDIUM_ENERGY;	
	double NORMAL_SPIN_HIGH_ENERGY;	
	double NORMAL_SMALLWASH_ENERGY;	

	double PERMPRESS_PREWASH_POWER;
	double PERMPRESS_WASH_POWER;
	double PERMPRESS_SPIN_LOW_POWER;	
	double PERMPRESS_SPIN_MEDIUM_POWER;	
	double PERMPRESS_SPIN_HIGH_POWER;	
	double PERMPRESS_SMALLWASH_POWER;	

	double PERMPRESS_PREWASH_ENERGY;
	double PERMPRESS_WASH_ENERGY;
	double PERMPRESS_SPIN_LOW_ENERGY;	
	double PERMPRESS_SPIN_MEDIUM_ENERGY;	
	double PERMPRESS_SPIN_HIGH_ENERGY;	
	double PERMPRESS_SMALLWASH_ENERGY;

	double GENTLE_PREWASH_POWER;
	double GENTLE_WASH_POWER;
	double GENTLE_SPIN_LOW_POWER;	
	double GENTLE_SPIN_MEDIUM_POWER;	
	double GENTLE_SPIN_HIGH_POWER;	
	double GENTLE_SMALLWASH_POWER;	

	double GENTLE_PREWASH_ENERGY;
	double GENTLE_WASH_ENERGY;
	double GENTLE_SPIN_LOW_ENERGY;	
	double GENTLE_SPIN_MEDIUM_ENERGY;	
	double GENTLE_SPIN_HIGH_ENERGY;	
	double GENTLE_SMALLWASH_ENERGY;

	double queue_min;
	double queue_max;

	double clotheswasher_run_prob;

	enum {  SPIN_LOW=0,						///< low power spin
			SPIN_MEDIUM=1,					///< medium power spin
			SPIN_HIGH=2,					///< high power spin
			SPIN_WASH=3,							///< wash mode
			SMALLWASH=4,					///< small wash modes in between
	};
	enumeration spin_mode;

	enum {  NORMAL=0,						///< Normal wash
			PERM_PRESS=1,					///< Permenant press wash
			GENTLE=2,						///< Gentle wash
	};
	enumeration wash_mode;

	clotheswasher(MODULE *module);
	~clotheswasher();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	double update_state(double dt);		///< updates the load struct and returns the time until expected state change

};

#endif // _clotheswasher_H

/**@}**/
