/** $Id: zipload.h $
	Copyright (C) 2009 Battelle Memorial Institute
	@file zipload.h
	@addtogroup ZIPload
	@ingroup residential

 @{
 **/

#ifndef _ZIPLOAD_H
#define _ZIPLOAD_H
#include "residential.h"
#include "residential_enduse.h"

class ZIPload : public residential_enduse
{
public:
	double base_power;			///< Base real power of the system
	double power_pf;			///< power factor of constant power load
	double current_pf;			///< power factor of constant current load
	double impedance_pf;		///< power factor of constant impedance load
	bool is_240;				///< load connected at 220 
	double breaker_val;			///< Amperage limit for connected breaker

public:
	static CLASS *oclass, *pclass;
	static ZIPload *defaults;

	ZIPload(MODULE *module);
	~ZIPload();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _ZIPLOAD_H

/**@}**/
