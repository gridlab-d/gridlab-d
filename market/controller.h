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
		TC_DOUBLE = 1,
		TC_THREEPART = 2
	} CONTROLTYPE;
	CONTROLTYPE type;
	char32 target1;
	char32 target2;
	char32 monitor;
	char32 demand1;
	char32 demand2;
	double ramp1_low, ramp1_high;		// k values
	double ramp2_low, ramp2_high;
	double range1_low, range1_high;
	double range2_low, range2_high;
	double min1, max1;					// for convenience
	double min2, max2;
	double base1, base2;
	double set1, set2;
	double bid_price, bid_quantity;
	int32 lastbid_id;
	int64 lastmkt_id;
	int16 may_run;
	int64 market_id;
	auction *market;
private:
	double *pTarget1, *pTarget2;
	const double *pDemand1, *pDemand2; // we only read the demanded power
	const double *pMonitor; // we only read the monitored value
	TIMESTAMP next_run;

	double transact(const double *, double *, double, double, double, double, double, double, double, double);
	double threepart();
};


#endif