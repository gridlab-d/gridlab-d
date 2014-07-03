/** $Id: microwave.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file microwave.h
	@addtogroup microwave
	@ingroup residential

 @{
 **/

#ifndef _MICROWAVE_H
#define _MICROWAVE_H
#include "residential.h"
#include "residential_enduse.h"

class microwave : public residential_enduse
{
public:
	double circuit_split;		///< -1=100% negative, 0=balanced, +1=100% positive
	double installed_power;		///< installed wattage [W] (default = random normal between 700W and 1200W)
	double standby_power;		///< standby power [W] (usually 10W)
	enum {	OFF=0,					///< microwave is off
			ON=1,					///< microwave is on
	} state;					///< microwave state
	double cycle_time;
private:
	double runtime;				///< current runtime (expected time in ON state)
	double last_runtime;
	double state_time;			///< time in current state
	double prev_demand;			///< previous demand
	TIMESTAMP cycle_start, cycle_on, cycle_off;
public:
	static CLASS *oclass, *pclass;
	static microwave *defaults;

	microwave(MODULE *module);
	~microwave();
	int create();
	void init_noshape();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	double update_state(double dt=0.0);
	TIMESTAMP update_state_cycle(TIMESTAMP t0, TIMESTAMP t1);
};

#endif // _MICROWAVE_H

/**@}**/
