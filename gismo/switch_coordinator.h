/** $Id: line_sensor.h 4738 2014-07-03 00:55:39Z dchassin $

 Switch coordinator

 **/

#ifndef _SWITCHCOORDINATOR_H
#define _SWITCHCOORDINATOR_H

#include "gridlabd.h"
#include "../powerflow/switch_object.h"

class switch_coordinator : public gld_object {
public:
	enum {SCS_IDLE=0, SCS_ARMED=1, SCS_ACTIVE=2} SWITCHCOORDINATIONSTATUS;
public:
	GL_ATOMIC(enumeration,status);
	GL_ATOMIC(set,armed);
private:
	unsigned int n_switches;
	unsigned int64 states;
	gld_property *index[64];
public:
	/* required implementations */
	switch_coordinator(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int precommit(TIMESTAMP t1);
	TIMESTAMP presync(TIMESTAMP t1);
	inline TIMESTAMP postsync(TIMESTAMP t1) { return TS_NEVER; };
	inline TIMESTAMP sync(TIMESTAMP t1) { return TS_NEVER; };
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);
	int notify_status(char *value);
	int connect(char *name);

public:
	static CLASS *oclass;
	static switch_coordinator *defaults;
};

#endif // _LINESENSOR_H
