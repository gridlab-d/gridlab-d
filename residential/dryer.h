/** $Id: dryer.h,v 1.9 2008/02/09 00:05:09 d3j168 Exp $
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
	complex dryer_actual_power;			///< actual complex power drawn from power system
	double circuit_split;				///< -1=100% negative, 0=balanced, +1=100% positive
	double motor_power;					///< installed dryer motor power [W] (default = random uniform between 150-350 W)
	double coil_power;					///< installed heating coil power [W] (default = random uniform between 3500-5000 W, 0 for gas)
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
	enum {	STOPPED=0,						///< motor is stopped
			RUNNING=1,						///< motor is running
			STALLED=2,						///< motor is stalled
			TRIPPED=3,						///< motor is tripped
	} state;							///< control state

public:
	static CLASS *oclass, *pclass;
	static dryer *defaults;

	dryer(MODULE *module);
	~dryer();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	double update_state(double dt=0.0);		///< updates the load struct and returns the time until expected state change

};

#endif // _DRYER_H

/**@}**/
