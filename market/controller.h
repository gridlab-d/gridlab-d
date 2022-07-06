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
		BM_PROXY,
	} BIDMODE;
	enumeration bidmode;

	typedef enum {
		CN_RAMP,
		CN_DOUBLE_RAMP,
		CN_DEV_LEVEL,
		CN_DOUBLE_PRICE
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
	char pMkt[33];
	char pMkt2[33];
	OBJECT *pMarket;
	OBJECT *pMarket2;
	auction *market;
	KEY lastbid_id;
	KEY lastmkt_id;
	KEY lastbid_id2;
	KEY lastmkt_id2;
	double last_p;
	double last_q;
	double P_ON,P_OFF,P_ONLOCK,P_OFFLOCK;  //Ebony
	double mu0, mu1; //Ebony
	double set_temp;
	int may_run;

	// new stuff
	double clear_price;
	double clear_price2;
	double ramp_low, ramp_high;
	double dPeriod,dPeriod2;
	int64 period,period2;
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
	char32 thermostat_state;

	double setpoint0;
	double heating_setpoint0;
	double cooling_setpoint0;
	double sliding_time_delay;
	int bid_delay,bid_delay2;

	bool use_predictive_bidding;
	double last_setpoint;
	double last_heating_setpoint;
	double last_cooling_setpoint;
	int64 bid_id;

private:
	TIMESTAMP next_run;
	TIMESTAMP last_run;
	TIMESTAMP fast_reg_run;		//Ebony
	TIMESTAMP init_time;
	gld_property *pMonitor;
	gld_property *pSetpoint;
	gld_property *pDemand;
	gld_property *pTotal;
	gld_property *pLoad;
	gld_property *pAvg;
	gld_property *pStd;
	gld_property *pMarginalFraction;
	double *pAvg2;	//Second Market
	double *pStd2;	//Second Market
	gld_property powerstate_prop;
	gld_keyword *PS_OFF, *PS_ON, *PS_UNKNOWN;
	enumeration last_pState;
	void cheat();
	int fetch_property(gld_property **prop, const char *propName, OBJECT *obj);
	int dir, direction;
	int dir2, direction2;
	int market_flag;
	int last_override;
	int locked;
	double r1;
	double min, max;
	double T_lim, k_T;
	double fast_reg_signal;
	int reg_period;
	double heat_min, heat_max;
	double cool_min, cool_max;
	gld_property *pDeadband;
	gld_property *pHeatingSetpoint;
	gld_property *pCoolingSetpoint;
	gld_property *pHeatingDemand;
	gld_property *pCoolingDemand;
	gld_property *pHeatingTotal;
	gld_property *pCoolingTotal;
	gld_property *pHeatingLoad;
	gld_property *pCoolingLoad;
	enumeration *pHeatingState;
	enumeration *pCoolingState;
	enumeration *pThermostatState;
	gld_property *pAuxState;
	gld_property *pHeatState;
	gld_property *pCoolState;
	char32 heat_state;
	char32 cool_state;
	char32 aux_state;
	int64 dtime_delay;
	TIMESTAMP time_off;
	bool use_market_period;
	int last_market;  //ebony
	int engaged;   //ebony
	double is_engaged;
	double delta_u;
	double P_ON_init;
	double P_total_init;
	double u_last;
	BIDINFO controller_bid;
	BIDINFO controller_bid2;
	FUNCTIONADDR submit;
	FUNCTIONADDR submit2;

	//Double-price CCSI functionality
	double calcTemp1_double_price(double Tair, double Deadband, enumeration State);
	double calcTemp2_double_price(gld_property *pUa, gld_property *pHm, gld_property *pCa, gld_property *pCm, gld_property *pMGF, gld_property *pMSFG, gld_property *pQi, gld_property *pQs, gld_property *pQh, gld_property *pTout, double Tair, gld_property *pTmass, double Deadband, enumeration State);

	gld_property override_prop;
	gld_keyword *OV_NORMAL, *OV_ON, *OV_OFF;
	enumeration *pOverride2;
	gld_property *pMarketId;
	gld_property *pMarketId2;
	gld_property *pClearedPrice;
	gld_property *pClearedPrice2;
	gld_property *pPriceCap;
	gld_property *pPriceCap2;
	GL_STRING(char32,marketunit);
	char market_unit[32];
	char market_unit2[32];
	gld_property *pMarginMode;
	static controller *defaults;
	int dev_level_ctrl(TIMESTAMP t0, TIMESTAMP t1);
	gld_property *pClearedQuantity;
	gld_property *pClearedQuantity2;
	gld_property *pSellerTotalQuantity;
	gld_property *pSellerTotalQuantity2;
	gld_property *pClearingType;
	gld_property *pClearingType2;
	gld_property *pMarginalFraction2;

	// DOUBLE PRICE variables
	gld_property *pUa;
	gld_property *pHm;
	gld_property *pCa;
	gld_property *pCm;
	gld_property *pMassInternalGainFraction;
	gld_property *pMassSolarGainFraction;
	gld_property *pQi;
	gld_property *pQs;
	gld_property *pQh;
	gld_property *pTout;
	gld_property *pTmass;
	gld_property *pRated_cooling_capacity;
	gld_property *pCooling_COP;

	//PROXY PROPERTIES
	double proxy_avg;
	double proxy_std;
	int64 proxy_mkt_id;
	int64 proxy_mkt_id2;
	double proxy_clear_price;
	double proxy_clear_price2;
	double proxy_price_cap;
	double proxy_price_cap2;
	char proxy_mkt_unit[32];
	char proxy_mkt_unit2[32];
	double proxy_init_price;
	enumeration proxy_margin_mode;
	double proxy_marginal_fraction;
	double proxy_clearing_quantity;
	double proxy_clearing_quantity2;
	double proxy_seller_total_quantity;
	double proxy_seller_total_quantity2;
	enumeration proxy_clearing_type;
	enumeration proxy_clearing_type2;
	double proxy_marginal_fraction2;
};

#endif // _controller_H

// EOF
