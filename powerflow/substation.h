// $Id: substation.h
//	Copyright (C) 2009 Battelle Memorial Institute

#ifndef _SUBSTATION_H
#define _SUBSTATION_H

#include "powerflow.h"
#include "node.h"

class substation : public node
{
protected:
	TIMESTAMP last_t;

public:
	complex distribution_voltage[3];	///< measured voltage
	complex distribution_current[3];	///< measured current
	double distribution_energy;			///< metered energy
	complex distribution_power;			///< metered power
	double distribution_demand;			///< metered demand (peak of power)
	double Network_Node_Base_Power;		///< base MVA for network node
	double Network_Node_Base_Voltage;	///< base Voltage for network node

public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	substation(MODULE *mod);
	inline substation(CLASS *cl=oclass):node(cl){};
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	int isa(char *classname);
};

#endif // _SUBSTATION_H

