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
	typedef enum {
		SM_NONE,
		SM_HOUSE_HEAT,
		SM_HOUSE_COOL,
		SM_HOUSE_PREHEAT,
		SM_HOUSE_PRECOOL,
	} SIMPLE_MODE;
	SIMPLE_MODE simplemode;
	double kT_L, kT_H, Tmin, Tmax;
	char target[33];
	char setpoint[33];
	char demand[33];
	char total[33];
	OBJECT *pMarket;
	auction *market;
	int32 lastbid_id;
	int64 lastmkt_id;
	double last_p;
	double last_q;
	double set_temp;
	int may_run;
	double setpoint0;
private:
	TIMESTAMP next_run;
	double *pMonitor;
	double *pSetpoint;
	double *pDemand;
	double *pTotal;
	void cheat();
	void fetch(double **prop, char *name, OBJECT *parent);
	int dir;
	double min, max;
	double T_lim, k_T;
};

#endif // _controller_H

// EOF
