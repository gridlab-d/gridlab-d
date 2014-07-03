/** $Id: series_reactor.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file series_reactor.h

 @{
 **/

#ifndef SERIES_REACTOR_H
#define SERIES_REACTOR_H

#include "powerflow.h"
#include "link.h"

class series_reactor : public link_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;

public:
	int create(void);
	int init(OBJECT *parent);
	series_reactor(MODULE *mod);
	inline series_reactor(CLASS *cl=oclass):link_object(cl){};
	int isa(char *classname);
	
	complex phase_A_impedance;	//Phase A impedance of the reactor
	complex phase_B_impedance;	//Phase B impedance of the reactor
	complex phase_C_impedance;	//Phase C impedance of the reactor
	double rated_current_limit;	//Current rating for the series reactor
};

#endif // SERIES_REACTOR_H
/**@}**/
