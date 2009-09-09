/** $Id: auction.h 1182 2009-09-09 22:08:36Z mhauer $
	Copyright (C) 2009 Battelle Memorial Institute
	@file controller.h
	@addtogroup controller
	@ingroup market

 **/

#ifndef _controller_H
#define _controller_H

#include <stdarg.h>
#include "auction.h"
#include "gridlabd.h"

class controller {
public:
	controller(MODULE *);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	static CLASS *oclass;
public:
	typedef enum{
		TC_SINGLE = 0,
		TC_DOUBLE = 1
	} CONTROLTYPE;
	CONTROLTYPE type;
	char32 target;
	char32 target2;
	char32 monitor;
	double ramp_low, ramp_high;		// k values
	double ramp2_low, ramp2_high;
	double min1, max1;
	double min2, max2;
	double base1, base2;
	double bid_price, bid_quantity;
	int64 market_id;
	auction *market;
private:
	double *pTarget1, *pTarget2;
	double *pMonitor;
};


#endif