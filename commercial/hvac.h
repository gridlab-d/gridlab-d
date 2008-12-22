/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
**/


#ifndef _HVAC_H
#define _HVAC_H

#include "gridlabd.h"

class hvac {
public:
private:
public:
	hvac(void);
	~hvac(void);
	void create();
	TIMESTAMP sync(TIMESTAMP t0);
	void pre_update(void);
	void post_update(void);
};

#endif
