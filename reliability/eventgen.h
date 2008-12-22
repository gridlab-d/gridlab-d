/** $Id: eventgen.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file eventgen.h

 @{
 **/

#ifndef _eventgen_H
#define _eventgen_H

#include <stdarg.h>
#include "gridlabd.h"

class eventgen {
private:
	char1024 old_values;		/**< previous value of target properties */
	FINDLIST *objectlist;		/**< list of object targeted */
	OBJECT *event_object;		/**< object to which event occurred */
public:
	double frequency;			/**< frequency of events (/y) */
	double duration;			/**< duration of event (s) */
	char256 group;				/**< object group aggregation */
	char256 targets;			/**< properties to alter during event (comma separated names) */
	char256 values;				/**< values to use for properties during event (comma separated values) */
public:
	/* required implementations */
	eventgen(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static eventgen *defaults;

};

#endif

/**@}*/
