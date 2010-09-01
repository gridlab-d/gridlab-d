/** $Id: plugload.h,v 1.9 2008/02/09 23:48:51 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file plugload.h
	@addtogroup plugload
	@ingroup residential

 @{
 **/

#ifndef _PLUGLOAD_H
#define _PLUGLOAD_H
#include "residential.h"
#include "residential_enduse.h"

class plugload : public residential_enduse
{
public:
	double circuit_split;		///< -1=100% negative, 0=balanced, +1=100% positive
	double demand;				///< fraction of time plugloads are ON (schedule driven)
	double heat_fraction;		///< fraction of the plugload that is transferred as heat (default = 0.90)
	complex plugs_actual_power;	///< actual power demand as a function of voltage

public:
	static CLASS *oclass, *pclass;
	static plugload *defaults;

	plugload(MODULE *module);
	~plugload();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _PLUGLOAD_H

/**@}**/
