/** $Id: microwave.h,v 1.8 2008/02/09 23:48:51 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file microwave.h
	@addtogroup microwave
	@ingroup residential

 @{
 **/

#ifndef _MICROWAVE_H
#define _MICROWAVE_H
#include "residential.h"

class microwave  
{
private:
	complex *pVoltage;

public:
	double circuit_split;		///< -1=100% negative, 0=balanced, +1=100% positive
	double installed_power;		///< installed wattage [W] (default = random normal between 700W and 1200W)
	double standby_power;		///< standby power [W] (usually 10W)
	double demand;				///< fraction of time microwave oven is ON (schedule driven)
	double power_factor;		///< power factor (default = 0.95)
	double heat_fraction;		///< fraction of heat that is internal gain
	ENDUSELOAD load;			///< enduse load structure
	enum {	OFF=0,					///< microwave is off
			ON=1,					///< microwave is on
	} state;					///< microwave state
private:
	double runtime;				///< current runtime (expected time in ON state)
	double state_time;			///< time in current state
	double prev_demand;			///< previous demand

public:
	static CLASS *oclass;
	static microwave *defaults;

	microwave(MODULE *module);
	~microwave();
	int create();
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	double update_state(double dt=0.0);
};

#endif // _MICROWAVE_H

/**@}**/
