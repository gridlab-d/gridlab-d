/** $Id: recloser.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2011 Battelle Memorial Institute
	@file recloser.h
 @{
 **/

#ifndef RECLOSER_H
#define RECLOSER_H

#include "powerflow.h"
#include "switch_object.h"

class recloser : public switch_object// public switch_object ?
{
public:
	static CLASS *oclass;
	static CLASS *pclass;

public:
	int create(void);
	int init(OBJECT *parent);
	recloser(MODULE *mod);
	inline recloser(CLASS *cl=oclass):switch_object(cl){};
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0);

	double retry_time;
	double ntries;
	double curr_tries;
	int64 return_time;

private:
	TIMESTAMP prev_rec_time;
};

EXPORT double change_recloser_state(OBJECT *thisobj, unsigned char phase_change, bool state);
EXPORT int recloser_reliability_operation(OBJECT *thisobj, unsigned char desired_phases);
EXPORT int recloser_fault_updates(OBJECT *thisobj, unsigned char restoration_phases);

#endif // RECLOSER_H
/**@}**/