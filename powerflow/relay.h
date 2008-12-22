/** $Id: relay.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file relay.h

 @{
 **/

#ifndef RELAY_H
#define RELAY_H

#include "powerflow.h"
#include "link.h"

class relay : public link
{
private:
	TIMESTAMP reclose_time;
public:
	double time_to_change;	///< time for state to change
	double recloser_delay;	///< time delay before reclose (s)
	int16 recloser_tries;	///< number of times recloser tries
	
public:
	static CLASS *oclass;
	static CLASS *pclass;

public:
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	relay(MODULE *mod);
	inline relay(CLASS *cl=oclass):link(cl){};
	int isa(char *classname);

	friend class relay_tests;
};

#endif // RELAY_H
/**@}**/
