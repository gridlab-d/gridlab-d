// $Id: series_compensator.h 2018-12-06 $
//	Copyright (C) 2018 Battelle Memorial Institute

#ifndef _SERIES_COMPENSATOR_H
#define _SERIES_COMPENSATOR_H

#include "powerflow.h"
#include "link.h"

EXPORT SIMULATIONMODE interupdate_series_compensator(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

class series_compensator : public link_object
{
public:
	double series_compensator_resistance;

	double turns_ratio[3];
	double vset_value[3];
	double voltage_iter_tolerance;

	//Deltamode function
	SIMULATIONMODE inter_deltaupdate_series_compensator(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
	void sercom_prePre_fxn(void);	//Functionalized "presync before link::presync" calls
	void sercom_postPre_fxn(void);	//Functionalized "presync after link::presync" calls
	int sercom_postPost_fxn(unsigned char pass_value);	//Functionalized "postsync after link::postsync" calls

private:
	gld_property *ToNode_voltage[3];	//Pointer for API to map to voltages to read
	double prev_turns_ratio[3];
	complex val_ToNode_voltage[3];
	double val_ToNode_voltage_pu[3];
	double val_FromNode_nominal_voltage;
	
public:
	static CLASS *oclass;
	static CLASS *pclass;
	
public:
	series_compensator(MODULE *mod);
	inline series_compensator(CLASS *cl=oclass):link_object(cl){};
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	int isa(char *classname);
};

#endif // _SERIES_COMPENSATOR_H
