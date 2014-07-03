/** $Id: controller.h 4738 2014-07-03 00:55:39Z dchassin $
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

class controller : public gld_object {
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
		SM_DOUBLE_RAMP,
	} SIMPLE_MODE;
	enumeration simplemode;
	
	typedef enum {
		BM_OFF,
		BM_ON,
	} BIDMODE;
	enumeration bidmode;

	typedef enum {
		CN_RAMP,
		CN_DOUBLE_RAMP,
	} CONTROLMODE;
	enumeration control_mode;
	
	typedef enum {
		RM_DEADBAND,
		RM_SLIDING,
	} RESOLVEMODE;
	enumeration resolve_mode;

	typedef enum{
		TM_INVALID=0,
		TM_OFF=1,
		TM_HEAT=2,
		TM_COOL=3,
	} THERMOSTATMODE;
	enumeration thermostat_mode, last_mode, previous_mode;

	typedef enum {
		OU_OFF=0,
		OU_ON=1
	} OVERRIDE_USE;
	enumeration use_override;

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
	KEY lastbid_id;
	KEY lastmkt_id;
	double last_p;
	double last_q;
	double set_temp;
	int may_run;

	// new stuff
	double clear_price;
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
	char32 re_override;

	double setpoint0;
	double heating_setpoint0;
	double cooling_setpoint0;
	double sliding_time_delay;
	int bid_delay;

	bool use_predictive_bidding;
	double last_setpoint;
	double last_heating_setpoint;
	double last_cooling_setpoint;


private:
	TIMESTAMP next_run;
	TIMESTAMP init_time;
	double *pMonitor;
	double *pSetpoint;
	double *pDemand;
	double *pTotal;
	double *pLoad;
	double *pAvg;
	double *pStd;
	enumeration *pState;
	enumeration last_pState;
	void cheat();
	void fetch(double **prop, char *name, OBJECT *parent);
	int dir, direction;
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
	int64 dtime_delay;
	TIMESTAMP time_off;
	bool use_market_period;

	enumeration *pOverride;
};

#endif // _controller_H

// EOF
