/** $Id: occupantload.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file occupantload.h
	@addtogroup occupantload
	@ingroup residential

 @{
 **/

#ifndef _OCCUPANTLOAD_H
#define _OCCUPANTLOAD_H
#include "residential.h"
#include "residential_enduse.h"

class occupantload : public residential_enduse
{

public:
	int number_of_occupants;			///< number of occupants of the house
	double occupancy_fraction;			///< represents occupancy schedule
	double heatgain_per_person;			///< sensible+latent loads (400 BTU) default based on DOE-2

public:
	static CLASS *oclass, *pclass;
	static occupantload *defaults;

	occupantload(MODULE *module);
	~occupantload();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _OCCUPANTLOAD_H

/**@}**/
