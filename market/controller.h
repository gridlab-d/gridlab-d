/** $Id: controller.h 1182 2009-09-09 22:08:36Z mhauer $
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
	int isa(char *classname);
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
		SM_WATERHEATER,
	} SIMPLE_MODE;
	SIMPLE_MODE simplemode;
	
	typedef enum {
		BM_OFF,
		BM_ON,
	} BIDMODE;
	BIDMODE bidmode;

	typedef enum {
		CN_RAMP,
		CN_DOUBLE_RAMP,
	} CONTROLMODE;
	CONTROLMODE control_mode;
	
	typedef enum {
		RM_DEADBAND,
		RM_SLIDING,
	} RESOLVEMODE;
	RESOLVEMODE resolve_mode;

	typedef enum{
		TM_INVALID=0,
		TM_OFF=1,
		TM_HEAT=2,
		TM_COOL=3,
	} THERMOSTATMODE;
	THERMOSTATMODE thermostat_mode, last_mode;

	double kT_L, kT_H;
	char target[33];
	char setpoint[33];
	char demand[33];
	char total[33];
	char load[33];
	char state[33];
	char32 avg_target;
	char32 std_target;
	OBJECT *pMarket;
	auction *market;
	int64 lastbid_id;
	int64 lastmkt_id;
	double last_p;
	double last_q;
	double set_temp;
	int may_run;

	// new stuff
	double ramp_low, ramp_high;
	double dPeriod;
	int64 period;
	double slider_setting;
	double slider_setting_heat;
	double slider_setting_cool;
	double range_low;
	double range_high;
	double heat_range_high;
	double heat_range_low;
	double heat_ramp_high;
	double heat_ramp_low;
	double cool_range_high;
	double cool_range_low;
	double cool_ramp_high;
	double cool_ramp_low;
	char32 heating_setpoint;
	char32 cooling_setpoint;
	char32 heating_demand;
	char32 cooling_demand;
	char32 heating_total;
	char32 cooling_total;
	char32 heating_load;
	char32 cooling_load;
	char32 heating_state;
	char32 cooling_state;
	char32 deadband;

	double setpoint0;
	double heating_setpoint0;
	double cooling_setpoint0;

private:
	TIMESTAMP next_run;
	double *pMonitor;
	double *pSetpoint;
	double *pDemand;
	double *pTotal;
	double *pLoad;
	double *pAvg;
	double *pStd;
	enumeration *pState;
	void cheat();
	void fetch(double **prop, char *name, OBJECT *parent);
	int dir;
	double min, max;
	double T_lim, k_T;
	double heat_min, heat_max;
	double cool_min, cool_max;
	double *pDeadband;
	double *pHeatingSetpoint;
	double *pCoolingSetpoint;
	double *pHeatingDemand;
	double *pCoolingDemand;
	double *pHeatingTotal;
	double *pCoolingTotal;
	double *pHeatingLoad;
	double *pCoolingLoad;
	enumeration *pHeatingState;
	enumeration *pCoolingState;
	double *pAuxState;
	double *pHeatState;
	double *pCoolState;
	char32 heat_state;
	char32 cool_state;
	char32 aux_state;
};

#endif // _controller_H

// EOF
