/** $Id: controller.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file auction.cpp
	@defgroup controller Transactive controller, OlyPen experiment style
	@ingroup market

 **/

#include "controller.h"

CLASS* controller::oclass = NULL;

controller::controller(MODULE *module){
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"controller",sizeof(controller),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class controller";
		else
			oclass->trl = TRL_QUALIFIED;

		if (gl_publish_variable(oclass,
			PT_enumeration, "simple_mode", PADDR(simplemode),
				PT_KEYWORD, "NONE", (enumeration)SM_NONE,
				PT_KEYWORD, "HOUSE_HEAT", (enumeration)SM_HOUSE_HEAT,
				PT_KEYWORD, "HOUSE_COOL", (enumeration)SM_HOUSE_COOL,
				PT_KEYWORD, "HOUSE_PREHEAT", (enumeration)SM_HOUSE_PREHEAT,
				PT_KEYWORD, "HOUSE_PRECOOL", (enumeration)SM_HOUSE_PRECOOL,
				PT_KEYWORD, "WATERHEATER", (enumeration)SM_WATERHEATER,
				PT_KEYWORD, "DOUBLE_RAMP", (enumeration)SM_DOUBLE_RAMP,
			PT_enumeration, "bid_mode", PADDR(bidmode),
				PT_KEYWORD, "ON", (enumeration)BM_ON,
				PT_KEYWORD, "OFF", (enumeration)BM_OFF,
				PT_KEYWORD, "PROXY", (enumeration)BM_PROXY,
			PT_enumeration, "use_override", PADDR(use_override),
				PT_KEYWORD, "OFF", (enumeration)OU_OFF,
				PT_KEYWORD, "ON", (enumeration)OU_ON,
			PT_double, "ramp_low[degF]", PADDR(ramp_low), PT_DESCRIPTION, "the comfort response below the setpoint",
			PT_double, "ramp_high[degF]", PADDR(ramp_high), PT_DESCRIPTION, "the comfort response above the setpoint",
			PT_double, "range_low", PADDR(range_low), PT_DESCRIPTION, "the setpoint limit on the low side",
			PT_double, "range_high", PADDR(range_high), PT_DESCRIPTION, "the setpoint limit on the high side",
			PT_char32, "target", PADDR(target), PT_DESCRIPTION, "the observed property (e.g., air temperature)",
			PT_char32, "setpoint", PADDR(setpoint), PT_DESCRIPTION, "the controlled property (e.g., heating setpoint)",
			PT_char32, "demand", PADDR(demand), PT_DESCRIPTION, "the controlled load when on",
			PT_char32, "load", PADDR(load), PT_DESCRIPTION, "the current controlled load",
			PT_char32, "total", PADDR(total), PT_DESCRIPTION, "the uncontrolled load (if any)",
			PT_char32, "market", PADDR(pMkt), PT_DESCRIPTION, "the market to bid into",
			PT_char32, "state", PADDR(state), PT_DESCRIPTION, "the state property of the controlled load",
			PT_char32, "avg_target", PADDR(avg_target),
			PT_char32, "std_target", PADDR(std_target),
			PT_double, "bid_price", PADDR(last_p), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid price",
			PT_double, "bid_quantity", PADDR(last_q), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid quantity",
			PT_double, "set_temp[degF]", PADDR(set_temp), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the reset value",
			PT_double, "base_setpoint[degF]", PADDR(setpoint0),
			// new stuff
			PT_double, "market_price", PADDR(clear_price), PT_DESCRIPTION, "the current market clearing price seen by the controller.",
			PT_double, "period[s]", PADDR(dPeriod), PT_DESCRIPTION, "interval of time between market clearings",
			PT_enumeration, "control_mode", PADDR(control_mode),
				PT_KEYWORD, "RAMP", (enumeration)CN_RAMP,
				PT_KEYWORD, "DOUBLE_RAMP", (enumeration)CN_DOUBLE_RAMP,
				PT_KEYWORD, "DEV_LEVEL", (enumeration)CN_DEV_LEVEL,
			PT_enumeration, "resolve_mode", PADDR(resolve_mode),
				PT_KEYWORD, "DEADBAND", (enumeration)RM_DEADBAND,
				PT_KEYWORD, "SLIDING", (enumeration)RM_SLIDING,
			PT_double, "slider_setting",PADDR(slider_setting),
			PT_double, "slider_setting_heat", PADDR(slider_setting_heat),
			PT_double, "slider_setting_cool", PADDR(slider_setting_cool),
			PT_char32, "override", PADDR(re_override),
			// double ramp
			PT_double, "heating_range_high[degF]", PADDR(heat_range_high),
			PT_double, "heating_range_low[degF]", PADDR(heat_range_low),
			PT_double, "heating_ramp_high", PADDR(heat_ramp_high),
			PT_double, "heating_ramp_low", PADDR(heat_ramp_low),
			PT_double, "cooling_range_high[degF]", PADDR(cool_range_high),
			PT_double, "cooling_range_low[degF]", PADDR(cool_range_low),
			PT_double, "cooling_ramp_high", PADDR(cool_ramp_high),
			PT_double, "cooling_ramp_low", PADDR(cool_ramp_low),
			PT_double, "heating_base_setpoint[degF]", PADDR(heating_setpoint0),
			PT_double, "cooling_base_setpoint[degF]", PADDR(cooling_setpoint0),
			PT_char32, "deadband", PADDR(deadband),
			PT_char32, "heating_setpoint", PADDR(heating_setpoint),
			PT_char32, "heating_demand", PADDR(heating_demand),
			PT_char32, "cooling_setpoint", PADDR(cooling_setpoint),
			PT_char32, "cooling_demand", PADDR(cooling_demand),
			PT_double, "sliding_time_delay[s]", PADDR(sliding_time_delay), PT_DESCRIPTION, "time interval desired for the sliding resolve mode to change from cooling or heating to off",
			PT_bool, "use_predictive_bidding", PADDR(use_predictive_bidding),
			
			// DEV_LEVEL
			PT_double, "device_actively_engaged", PADDR(is_engaged),
			PT_int32, "cleared_market", PADDR(last_market),  
			PT_int32, "locked", PADDR(locked),
			PT_double,"p_ON",PADDR(P_ON),
			PT_double,"p_OFF",PADDR(P_OFF),
			PT_double,"p_ONLOCK",PADDR(P_ONLOCK),
			PT_double,"p_OFFLOCK",PADDR(P_OFFLOCK),
			PT_double,"delta_u",PADDR(delta_u),
			PT_char32, "regulation_market_on", PADDR(pMkt2), PT_DESCRIPTION, "the willing to change state from ON->OFF market to bid into for regulation case",
			PT_char32, "regulation_market_off", PADDR(pMkt), PT_DESCRIPTION, "the willing to change state from OFF->ON market to bid into for regulation case",
			PT_double, "fast_regulation_signal[s]", PADDR(fast_reg_signal), PT_DESCRIPTION, "the regulation signal that the controller compares against (i.e., how often is there a control action",
			PT_double, "market_price_on", PADDR(clear_price2), PT_DESCRIPTION, "the current market clearing price seen by the controller in ON->OFF regulation market",
			PT_double, "market_price_off", PADDR(clear_price), PT_DESCRIPTION, "the current market clearing price seen by the controller  in OFF->ON regulation market",
			PT_double, "period_on[s]", PADDR(dPeriod2), PT_DESCRIPTION, "interval of time between market clearings in ON->OFF regulation market",
			PT_double, "period_off[s]", PADDR(dPeriod), PT_DESCRIPTION, "interval of time between market clearings in  OFF->ON regulation market",
			PT_int32, "regulation_period", PADDR(reg_period), PT_DESCRIPTION, "fast regulation signal period",
			PT_double, "r1",PADDR(r1), PT_DESCRIPTION, "temporary diagnostic variable",
			PT_double, "mu0",PADDR(mu0), PT_DESCRIPTION, "temporary diagnostic variable",
			PT_double, "mu1",PADDR(mu1), PT_DESCRIPTION, "temporary diagnostic variable",

			// redefinitions
			PT_char32, "average_target", PADDR(avg_target),
			PT_char32, "standard_deviation_target", PADDR(std_target),
			PT_int64, "bid_id", PADDR(bid_id),
#ifdef _DEBUG
			PT_enumeration, "current_mode", PADDR(thermostat_mode),
				PT_KEYWORD, "INVALID", (enumeration)TM_INVALID,
				PT_KEYWORD, "OFF", (enumeration)TM_OFF,
				PT_KEYWORD, "HEAT", (enumeration)TM_HEAT,
				PT_KEYWORD, "COOL", (enumeration)TM_COOL,
			PT_enumeration, "dominant_mode", PADDR(last_mode),
				PT_KEYWORD, "INVALID", (enumeration)TM_INVALID,
				PT_KEYWORD, "OFF", (enumeration)TM_OFF,
				PT_KEYWORD, "HEAT", (enumeration)TM_HEAT,
				PT_KEYWORD, "COOL", (enumeration)TM_COOL,
			PT_enumeration, "previous_mode", PADDR(previous_mode),
				PT_KEYWORD, "INVALID", TM_INVALID,
				PT_KEYWORD, "OFF", (enumeration)TM_OFF,
				PT_KEYWORD, "HEAT", (enumeration)TM_HEAT,
				PT_KEYWORD, "COOL", (enumeration)TM_COOL,
			PT_double, "heat_max", PADDR(heat_max),
			PT_double, "cool_min", PADDR(cool_min),
#endif
			PT_int32, "bid_delay", PADDR(bid_delay),
			PT_char32, "thermostat_state", PADDR(thermostat_state), PT_DESCRIPTION, "The name of the thermostat state property within the object being controlled",
			// PROXY PROPERTIES
			PT_double, "proxy_average", PADDR(proxy_avg),
			PT_double, "proxy_standard_deviation", PADDR(proxy_std),
			PT_int64, "proxy_market_id", PADDR(proxy_mkt_id),
			PT_int64, "proxy_market2_id", PADDR(proxy_mkt_id2),
			PT_double, "proxy_clear_price[$]", PADDR(proxy_clear_price),
			PT_double, "proxy_clear_price2[$]", PADDR(proxy_clear_price2),
			PT_double, "proxy_price_cap", PADDR(proxy_price_cap),
			PT_double, "proxy_price_cap2", PADDR(proxy_price_cap2),
			PT_char32, "proxy_market_unit", PADDR(proxy_mkt_unit),
			PT_double, "proxy_initial_price", PADDR(proxy_init_price),
			PT_double, "proxy_marginal_fraction", PADDR(proxy_marginal_fraction),
			PT_double, "proxy_marginal_fraction2", PADDR(proxy_marginal_fraction2),
			PT_double, "proxy_clearing_quantity", PADDR(proxy_clearing_quantity),
			PT_double, "proxy_clearing_quantity2", PADDR(proxy_clearing_quantity2),
			PT_double, "proxy_seller_total_quantity", PADDR(proxy_seller_total_quantity),
			PT_double, "proxy_seller_total_quantity2", PADDR(proxy_seller_total_quantity2),
			PT_enumeration, "proxy_margin_mode", PADDR(proxy_margin_mode),
				PT_KEYWORD, "NORMAL", (enumeration)AM_NONE,
				PT_KEYWORD, "DENY", (enumeration)AM_DENY,
				PT_KEYWORD, "PROB", (enumeration)AM_PROB,
			PT_enumeration, "proxy_clearing_type", PADDR(proxy_clearing_type),
				PT_KEYWORD, "MARGINAL_SELLER", (enumeration)CT_SELLER,
				PT_KEYWORD, "MARGINAL_BUYER", (enumeration)CT_BUYER,
				PT_KEYWORD, "MARGINAL_PRICE", (enumeration)CT_PRICE,
				PT_KEYWORD, "EXACT", (enumeration)CT_EXACT,
				PT_KEYWORD, "FAILURE", (enumeration)CT_FAILURE,
				PT_KEYWORD, "NULL", (enumeration)CT_NULL,
			PT_enumeration, "proxy_clearing_type2", PADDR(proxy_clearing_type2),
				PT_KEYWORD, "MARGINAL_SELLER", (enumeration)CT_SELLER,
				PT_KEYWORD, "MARGINAL_BUYER", (enumeration)CT_BUYER,
				PT_KEYWORD, "MARGINAL_PRICE", (enumeration)CT_PRICE,
				PT_KEYWORD, "EXACT", (enumeration)CT_EXACT,
				PT_KEYWORD, "FAILURE", (enumeration)CT_FAILURE,
				PT_KEYWORD, "NULL", (enumeration)CT_NULL,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(controller));
	}
}

int controller::create(){
	memset(this, 0, sizeof(controller));
	sprintf((char *)(&avg_target), "avg24");
	sprintf((char *)(&std_target), "std24");
	slider_setting_heat = -0.001;
	slider_setting_cool = -0.001;
	slider_setting = -0.001;
	sliding_time_delay = -1;
	lastbid_id = -1;
	heat_range_low = -5;
	heat_range_high = 3;
	cool_range_low = -3;
	cool_range_high = 5;
	use_override = OU_OFF;
	period = 0;
	period2 = 0;
	use_predictive_bidding = FALSE;
	controller_bid.bid_id = -1;
	controller_bid.market_id = -1;
	controller_bid.price = 0;
	controller_bid.quantity = 0;
	controller_bid.state = BS_UNKNOWN;
	controller_bid.rebid = false;
	controller_bid.bid_accepted = true;
	controller_bid2.bid_id = -1;
	controller_bid2.market_id = -1;
	controller_bid2.price = 0;
	controller_bid2.quantity = 0;
	controller_bid2.state = BS_UNKNOWN;
	controller_bid2.rebid = false;
	controller_bid2.bid_accepted = true;
	bid_id = -1;
	return 1;
}

/** provides some easy default inputs for the transactive controller,
	 and some examples of what various configurations would look like.
 **/
void controller::cheat(){
	switch(simplemode){
		case SM_NONE:
			break;
		case SM_HOUSE_HEAT:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "heating_setpoint");
			sprintf(demand, "heating_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			ramp_low = -2;
			ramp_high = -2;
			range_low = -5;
			range_high = 0;
			dir = -1;
			break;
		case SM_HOUSE_COOL:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "cooling_setpoint");
			sprintf(demand, "cooling_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			ramp_low = 2;
			ramp_high = 2;
			range_low = 0;
			range_high = 5;
			dir = 1;
			break;
		case SM_HOUSE_PREHEAT:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "heating_setpoint");
			sprintf(demand, "heating_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			ramp_low = -2;
			ramp_high = -2;
			range_low = -5;
			range_high = 3;
			dir = -1;
			break;
		case SM_HOUSE_PRECOOL:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "cooling_setpoint");
			sprintf(demand, "cooling_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			ramp_low = 2;
			ramp_high = 2;
			range_low = -3;
			range_high = 5;
			dir = 1;
			break;
		case SM_WATERHEATER:
			sprintf(target, "temperature");
			sprintf(setpoint, "tank_setpoint");
			sprintf(demand, "heating_element_capacity");
			sprintf(total, "actual_load");
			sprintf(load, "actual_load");
			sprintf(state, "power_state");
			ramp_low = -2;
			ramp_high = -2;
			range_low = 0;
			range_high = 10;
			break;
		case SM_DOUBLE_RAMP:
			sprintf(target, "air_temperature");
			sprintf((char *)(&heating_setpoint), "heating_setpoint");
			sprintf((char *)(&heating_demand), "heating_demand");
			sprintf((char *)(&heating_total), "total_load");		// using total instead of heating_total
			sprintf((char *)(&heating_load), "hvac_load");			// using load instead of heating_load
			sprintf((char *)(&cooling_setpoint), "cooling_setpoint");
			sprintf((char *)(&cooling_demand), "cooling_demand");
			sprintf((char *)(&cooling_total), "total_load");		// using total instead of cooling_total
			sprintf((char *)(&cooling_load), "hvac_load");			// using load instead of cooling_load
			sprintf((char *)(&deadband), "thermostat_deadband");
			sprintf(load, "hvac_load");
			sprintf(total, "total_load");
			heat_ramp_low = -2;
			heat_ramp_high = -2;
			heat_range_low = -5;
			heat_range_high = 5;
			cool_ramp_low = 2;
			cool_ramp_high = 2;
			cool_range_low = -5;
			cool_range_high = 5;
			break;
		default:
			break;
	}
}


/** convenience shorthand
 **/
int controller::fetch_property(gld_property **prop, char *propName, OBJECT *obj){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = new gld_property(obj, propName);
	if(!(*prop)->is_valid()){
		gl_error("controller::fetch_property: controller %s can't find property %s in object %s.", hdr->name, propName, obj->name);
		return 0;
	} else {
		return 1;
	}
}

/** initialization process
 **/
int controller::init(OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	char tname[32];
	char *namestr = (hdr->name ? hdr->name : tname);
	gld_property *pInitPrice = NULL;

	sprintf(tname, "controller:%i", hdr->id);

	cheat();

	if(parent == NULL){
		gl_error("%s: controller has no parent, therefore nothing to control", namestr);
		return 0;
	}

	if(bidmode != BM_PROXY){
		pMarket = gl_get_object((char *)(&pMkt));
		if(pMarket == NULL){
			gl_error("%s: controller has no market, therefore no price signals", namestr);
			return 0;
		}

		if((pMarket->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("controller::init(): deferring initialization on %s", gl_name(pMarket, objname, 255));
			return 2; // defer
		}

		if(gl_object_isa(pMarket, "auction")){
			gl_set_dependent(hdr, pMarket);
		}

		if(dPeriod == 0.0){
			gld_property *pPeriod = NULL;
			if(fetch_property(&pPeriod, "period", pMarket) == 0){
				return 0;
			}
			pPeriod->getp(dPeriod);
		}
		period = (TIMESTAMP)floor(dPeriod + 0.5);

		if(control_mode == CN_DEV_LEVEL){
			pMarket2 = gl_get_object((char *)(&pMkt2));
			if(pMarket2 == NULL){
				gl_error("%s: controller has no second market, therefore no price signals from the second market", namestr);
				return 0;
			}

			if(gl_object_isa(pMarket2, "auction")){
				gl_set_dependent(hdr, pMarket2);
			} else {
				gl_error("controllers only work in the secnond market when attached to an 'auction' object");
				return 0;
			}

			if((pMarket2->flags & OF_INIT) != OF_INIT){
				char objname[256];
				gl_verbose("Second market: controller::init(): deferring initialization on %s", gl_name(pMarket2, objname, 255));
				return 2; // defer
			}
			if(dPeriod2 == 0.0){
				gld_property *pPeriod2 = NULL;
				if(fetch_property(&pPeriod2, "period", pMarket2) == 0) {
					return 0;
				}
				pPeriod2->getp(dPeriod2);
			}
			period2 = (TIMESTAMP)floor(dPeriod2 + 0.5);

		}
		if(fetch_property(&pAvg, (char *)(&avg_target), pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pStd, (char *)(&std_target), pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pMarketId, "market_id", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pClearedPrice, "current_market.clearing_price", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pPriceCap, "price_cap", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pMarginalFraction, "current_market.marginal_quantity_frac", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pMarginMode, "margin_mode", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pClearedQuantity, "current_market.clearing_quantity", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pSellerTotalQuantity, "current_market.seller_total_quantity", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pClearingType, "current_market.clearing_type", pMarket) == 0) {
			return 0;
		}
		gld_property *marketunit;
		if(fetch_property(&marketunit, "unit", pMarket) == 0) {
			return 0;
		}
		gld_string mku;
		mku = marketunit->get_string();
		strncpy(market_unit, mku.get_buffer(), 31);
		submit = (FUNCTIONADDR)(gl_get_function(pMarket, "submit_bid_state"));
		if(submit == NULL){
			char buf[256];
			gl_error("Unable to find function, submit_bid_state(), for object %s.", (char *)gl_name(pMarket, buf, 255));
			return 0;
		}
		if(fetch_property(&pInitPrice, "init_price", pMarket) == 0) {
			return 0;
		}
		if(control_mode == CN_DEV_LEVEL) {
			if(fetch_property(&pMarketId2, "market_id", pMarket2) ==0) {
				return 0;
			}
			if(fetch_property(&pClearedPrice2, "current_market.clearing_price", pMarket2) == 0) {
				return 0;
			}
			if(fetch_property(&pPriceCap2, "price_cap", pMarket2) == 0) {
				return 0;
			}
			if(fetch_property(&pMarginalFraction2, "current_market.marginal_quantity_frac", pMarket2) == 0) {
				return 0;
			}
			if(fetch_property(&pClearedQuantity2, "current_market.clearing_quantity", pMarket2) == 0) {
				return 0;
			}
			if(fetch_property(&pSellerTotalQuantity2, "current_market.seller_total_quantity", pMarket2) == 0) {
				return 0;
			}
			if(fetch_property(&pClearingType2, "current_market.clearing_type", pMarket2) == 0) {
				return 0;
			}
			gld_property *marketunit2;
			if(fetch_property(&pClearingType2, "unit", pMarket2) == 0) {
				return 0;
			}
			mku = marketunit2->get_string();
			strncpy(market_unit2, mku.get_buffer(), 31);
			submit2 = (FUNCTIONADDR)(gl_get_function(pMarket2, "submit_bid_state"));
			if(submit2 == NULL){
				char buf[256];
				gl_error("Unable to find function, submit_bid_state(), for object %s.", (char *)gl_name(pMarket2, buf, 255));
				return 0;
			}
		}
	} else {
		if (dPeriod == 0.0)
		{
			period =300;
		}
		else
		{
			period = (TIMESTAMP)floor(dPeriod + 0.5);
		}
		pMarket = OBJECTHDR(this);
		if(fetch_property(&pAvg, "proxy_average", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pStd, "proxy_standard_deviation", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pMarketId, "proxy_market_id", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pClearedPrice, "proxy_clear_price", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pPriceCap,"proxy_price_cap", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pMarginMode, "proxy_margin_mode", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pMarginalFraction, "proxy_marginal_fraction", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pClearedQuantity, "proxy_clearing_quantity", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pSellerTotalQuantity, "proxy_seller_total_quantity", pMarket) == 0) {
			return 0;
		}
		if(fetch_property(&pClearingType, "proxy_clearing_type", pMarket) == 0) {
			return 0;
		}
		strncpy(market_unit, proxy_mkt_unit, 31);
		submit = (FUNCTIONADDR)(gl_get_function(pMarket, "submit_bid_state"));
		if(submit == NULL){
			char buf[256];
			gl_error("Unable to find function, submit_bid_state(), for object %s.", (char *)gl_name(pMarket, buf, 255));
			return 0;
		}
		if(fetch_property(&pInitPrice, "proxy_initial_price", pMarket) == 0) {
			return 0;
		}
		if(control_mode == CN_DEV_LEVEL) {
			if(dPeriod2 == 0.0){
				period2 = 300;
			} else {
				period2 = (TIMESTAMP)floor(dPeriod2 + 0.5);
			}
			pMarket2 = OBJECTHDR(this);
			if(fetch_property(&pMarketId2, "proxy_market2_id", pMarket2) == 0) {
				return 0;
			}
			if(fetch_property(&pClearedPrice2, "proxy_clear_price2", pMarket2) == 0) {
				return 0;
			}
			if(fetch_property(&pPriceCap2, "proxy_price_cap2", pMarket2) == 0) {
				return 0;
			}
			if(fetch_property(&pMarginalFraction2, "proxy_marginal_fraction2", pMarket2) == 0) {
				return 0;
			}
			if(fetch_property(&pClearedQuantity2, "proxy_clearing_quantity2", pMarket2) ==0) {
				return 0;
			}
			if(fetch_property(&pSellerTotalQuantity2, "proxy_seller_total_quantity2", pMarket2) == 0) {
				return 0;
			}
			if(fetch_property(&pClearingType2, "proxy_clearing_type2", pMarket2) == 0) {
				return 0;
			}
			strncpy(market_unit2, proxy_mkt_unit2, 31);
			submit2 = (FUNCTIONADDR)(gl_get_function(pMarket2, "submit_bid_state"));
			if(submit2 == NULL){
				char buf[256];
				gl_error("Unable to find function, submit_bid_state(), for object %s.", (char *)gl_name(pMarket2, buf, 255));
				return 0;
			}
		}
	}

	if(bid_delay < 0){
		bid_delay = -bid_delay;
	}
	if(bid_delay > period){
		gl_warning("Bid delay is greater than the controller period. Resetting bid delay to 0.");
		bid_delay = 0;
	}

	if(control_mode == CN_DEV_LEVEL){
		if(bid_delay2 < 0){
			bid_delay2 = -bid_delay2;
		}
		if(bid_delay2 > period2){
			gl_warning("Bid delay is greater than the controller period. Resetting bid delay to 0.");
			bid_delay2 = 0;
		}
	}

	if(target[0] == 0){
		GL_THROW("controller: %i, target property not specified", hdr->id);
	}
	if(setpoint[0] == 0 && control_mode == CN_RAMP){
		GL_THROW("controller: %i, setpoint property not specified", hdr->id);
	}
	if(demand[0] == 0 && control_mode == CN_RAMP){
		GL_THROW("controller: %i, demand property not specified", hdr->id);
	}
	if(deadband[0] == 0 && use_predictive_bidding == TRUE && control_mode == CN_RAMP){
		GL_THROW("controller: %i, deadband property not specified", hdr->id);
	}
	
	if(bid_delay < 0){
		bid_delay = -bid_delay;
	}
	if(bid_delay > period){
		gl_warning("Bid delay is greater than the controller period. Resetting bid delay to 0.");
		bid_delay = 0;
	}

	if(setpoint[0] == 0 && control_mode == CN_DEV_LEVEL){
		GL_THROW("controller: %i, setpoint property not specified", hdr->id);
	}

	if(demand[0] == 0 && control_mode == CN_DEV_LEVEL){
		GL_THROW("controller: %i, demand property not specified", hdr->id);
	}

	if(deadband[0] == 0 && use_predictive_bidding == TRUE && control_mode == CN_DEV_LEVEL){
		GL_THROW("controller: %i, deadband property not specified", hdr->id);
	}

	if(total[0] == 0){
		GL_THROW("controller: %i, total property not specified", hdr->id);
	}
	if(load[0] == 0){
		GL_THROW("controller: %i, load property not specified", hdr->id);
	}

	if(heating_setpoint[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, heating_setpoint property not specified", hdr->id);
	}
	if(heating_demand[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, heating_demand property not specified", hdr->id);
	}

	if(cooling_setpoint[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, cooling_setpoint property not specified", hdr->id);
	}
	if(cooling_demand[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, cooling_demand property not specified", hdr->id);
	}

	if(deadband[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, deadband property not specified", hdr->id);
	}

	if(fetch_property(&pMonitor, (char *)(&target), parent) == 0) {
		return 0;
	}
	if(control_mode == CN_RAMP || control_mode == CN_DEV_LEVEL){
		if(fetch_property(&pSetpoint, (char *)(&setpoint), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pDemand, (char *)(&demand), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pTotal, (char *)(&total), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pLoad, (char *)(&load), parent) == 0) {
			return 0;
		}
		if(use_predictive_bidding == TRUE){
			if(fetch_property(&pDeadband, (char *)(&deadband), parent) == 0) {
				return 0;
			}
		}
	} else if(control_mode == CN_DOUBLE_RAMP){
		sprintf(aux_state, "is_AUX_on");
		sprintf(heat_state, "is_HEAT_on");
		sprintf(cool_state, "is_COOL_on");
		if(fetch_property(&pHeatingSetpoint, (char *)(&heating_setpoint), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pHeatingDemand, (char *)(&heating_demand), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pHeatingTotal, (char *)(&total), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pHeatingLoad, (char *)(&load), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pCoolingSetpoint, (char *)(&cooling_setpoint), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pCoolingDemand, (char *)(&cooling_demand), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pCoolingTotal, (char *)(&total), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pCoolingLoad, (char *)(&load), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pDeadband, (char *)(&deadband), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pAuxState, (char *)(&aux_state), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pHeatState, (char *)(&heat_state), parent) == 0) {
			return 0;
		}
		if(fetch_property(&pCoolState, (char *)(&cool_state), parent) == 0) {
			return 0;
		}
	}

	if(bid_id == -1){
		controller_bid.bid_id = (int64)hdr->id;
		bid_id = (int64)hdr->id;
	} else {
		controller_bid.bid_id = bid_id;
	}
	controller_bid2.bid_id = controller_bid.bid_id;
	if(thermostat_state[0] == 0){
		pThermostatState = NULL;
	} else {
		pThermostatState = gl_get_enum_by_name(parent, thermostat_state);
		if(pThermostatState == 0){
			gl_error("thermostat state property name \'%s\' is not published by parent class.", (char *)&thermostat_state);
			return 0;
		}
	}
	if(dir == 0){
		double high = ramp_high * range_high;
		double low = ramp_low * range_low;
		if(high > low){
			dir = 1;
		} else if(high < low){
			dir = -1;
		} else if((high == low) && (fabs(ramp_high) > 0.001 || fabs(ramp_low) > 0.001)){
			dir = 0;
			if(ramp_high > 0){
				direction = 1;
			} else {
				direction = -1;
			}
			gl_warning("%s: controller has no price ramp", namestr);
			/* occurs given no price variation, or no control width (use a normal thermostat?) */
		}
		if(ramp_low * ramp_high < 0){
			gl_warning("%s: controller price curve is not injective and may behave strangely");
			/* TROUBLESHOOT
				The price curve 'changes directions' at the setpoint, which may create odd
				conditions in a number of circumstances.
			 */
		}
	}
	if(setpoint0==0)
		setpoint0 = -1; // key to check first thing

	if(heating_setpoint0==0)
		heating_setpoint0 = -1;

	if(cooling_setpoint0==0)
		cooling_setpoint0 = -1;

//	double period = market->period;
//	next_run = gl_globalclock + (TIMESTAMP)(period - fmod(gl_globalclock+period,period));
	next_run = gl_globalclock;// + (market->period - gl_globalclock%market->period);
	last_run = next_run;
	init_time = gl_globalclock;
	time_off = TS_NEVER;
	if(sliding_time_delay < 0 )
		dtime_delay = 21600; // default sliding_time_delay of 6 hours
	else
		dtime_delay = (int64)sliding_time_delay;

	if(state[0] != 0){
		// grab state pointer
		powerstate_prop = gld_property(parent,state); // pState = gl_get_enum_by_name(parent, state);
		if ( !powerstate_prop.is_valid() )
		{
			gl_error("state property name '%s' is not published by parent object '%s'", state, get_object(parent)->get_name());
			return 0;
		}
		PS_OFF = powerstate_prop.find_keyword("OFF");
		PS_ON = powerstate_prop.find_keyword("ON");
		PS_UNKNOWN = powerstate_prop.find_keyword("UNKNOWN");
		if ( PS_OFF==NULL || PS_ON==NULL || PS_UNKNOWN==NULL )
		{
			gl_error("state property '%s' of object '%s' does not published all required keywords OFF, ON, and UNKNOWN", state,get_object(parent)->get_name());
		}
		last_pState = *PS_UNKNOWN;
	}

	if(heating_state[0] != 0){
		// grab state pointer
		pHeatingState = gl_get_enum_by_name(parent, heating_state);
		if(pHeatingState == 0){
			gl_error("heating_state property name \'%s\' is not published by parent class", (char *)(&heating_state));
			return 0;
		}
	}

	if(cooling_state[0] != 0){
		// grab state pointer
		pCoolingState = gl_get_enum_by_name(parent, cooling_state);
		if(pCoolingState == 0){
			gl_error("cooling_state property name \'%s\' is not published by parent class", (char *)(&cooling_state));
			return 0;
		}
	}
	// get override, if set
	if ( re_override[0] != 0 )
	{
		override_prop = gld_property(parent, re_override);
		if ( !override_prop.is_valid() )
		{
			gl_error("use_override property '%s' is not found in object '%s'", (const char*)re_override, get_object(parent)->get_name());
			return 0;
		}
		OV_OFF = override_prop.find_keyword("OFF");
		OV_ON = override_prop.find_keyword("ON");
		OV_NORMAL = override_prop.find_keyword("NORMAL");
		if ( OV_OFF==NULL || OV_ON==NULL || OV_NORMAL==NULL )
		{
			gl_error("the use_override property '%s' does not define the expected enumeration keywords NORMAL, ON, and OFF");
			return 0;
		}
	}
	if(use_override == OU_ON && bid_delay <= 0){
		bid_delay = 1;
	}

	if(control_mode == CN_RAMP){
		if(slider_setting < -0.001){
			gl_warning("slider_setting is negative, reseting to 0.0");
			slider_setting = 0.0;
		}
		if(slider_setting > 1.0){
			gl_warning("slider_setting is greater than 1.0, reseting to 1.0");
			slider_setting = 1.0;
		}
	}

	if(control_mode == CN_DEV_LEVEL){
		if(slider_setting < -0.001){
			gl_warning("slider_setting is negative, reseting to 0.0");
			slider_setting = 0.0;
		}
		if(slider_setting > 1.0){
			gl_warning("slider_setting is greater than 1.0, reseting to 1.0");
			slider_setting = 1.0;
		}
	}

	if(control_mode == CN_DOUBLE_RAMP){
		if(slider_setting_heat < -0.001){
			gl_warning("slider_setting_heat is negative, reseting to 0.0");
			slider_setting_heat = 0.0;
		}
		if(slider_setting_cool < -0.001){
			gl_warning("slider_setting_cool is negative, reseting to 0.0");
			slider_setting_cool = 0.0;
		}
		if(slider_setting_heat > 1.0){
			gl_warning("slider_setting_heat is greater than 1.0, reseting to 1.0");
			slider_setting_heat = 1.0;
		}
		if(slider_setting_cool > 1.0){
			gl_warning("slider_setting_cool is greater than 1.0, reseting to 1.0");
			slider_setting_cool = 1.0;
		}
		// get override, if set
	}
	//get initial clear price
	pInitPrice->getp(last_p);
	lastmkt_id = 0;
	market_flag = -1;
	engaged = 0;
	locked = 0;

	return 1;
}

int controller::isa(char *classname)
{
	return strcmp(classname,"controller")==0;
}

TIMESTAMP controller::presync(TIMESTAMP t0, TIMESTAMP t1){
	double setPoint = 0.0;
	double heatSetPoint = 0.0;
	double coolSetPoint = 0.0;
	if(control_mode == CN_RAMP || control_mode == CN_DEV_LEVEL) {
		pSetpoint->getp(setPoint);
	} else if(control_mode == CN_DOUBLE_RAMP) {
		pHeatingSetpoint->getp(heatSetPoint);
		pCoolingSetpoint->getp(coolSetPoint);
	}
	if(slider_setting < -0.001)
		slider_setting = 0.0;
	if(slider_setting_heat < -0.001)
		slider_setting_heat = 0.0;
	if(slider_setting_cool < -0.001)
		slider_setting_cool = 0.0;
	if(slider_setting > 1.0)
		slider_setting = 1.0;
	if(slider_setting_heat > 1.0)
		slider_setting_heat = 1.0;
	if(slider_setting_cool > 1.0)
		slider_setting_cool = 1.0;

	if(control_mode == CN_RAMP && setpoint0 == -1)
		setpoint0 = setPoint;
	if(control_mode == CN_DEV_LEVEL && setpoint0 == -1)
		setpoint0 = setPoint;
	if(control_mode == CN_DOUBLE_RAMP && heating_setpoint0 == -1)
		heating_setpoint0 = heatSetPoint;
	if(control_mode == CN_DOUBLE_RAMP && cooling_setpoint0 == -1)
		cooling_setpoint0 = coolSetPoint;

	if(control_mode == CN_RAMP){
		if (slider_setting == -0.001){
			min = setpoint0 + range_low;
			max = setpoint0 + range_high;
		} else if(slider_setting > 0){
			min = setpoint0 + range_low * slider_setting;
			max = setpoint0 + range_high * slider_setting;
			if(range_low != 0)
				ramp_low = 2 + (1 - slider_setting);
			else
				ramp_low = 0;
			if(range_high != 0)
				ramp_high = 2 + (1 - slider_setting);
			else
				ramp_high = 0;
		} else {
			min = setpoint0;
			max = setpoint0;
		}
	} 
	else if(control_mode == CN_DEV_LEVEL){
		if (slider_setting == -0.001){
			min = setpoint0 + range_low;
			max = setpoint0 + range_high;
		} else if(slider_setting > 0){
			min = setpoint0 + range_low * slider_setting;
			max = setpoint0 + range_high * slider_setting;
			if(range_low != 0)
				ramp_low = 2 + (1 - slider_setting);
			else
				ramp_low = 0;
			if(range_high != 0)
				ramp_high = 2 + (1 - slider_setting);
			else
				ramp_high = 0;
		} else {
			min = setpoint0;
			max = setpoint0;
		}
	}
	else if(control_mode == CN_DOUBLE_RAMP){
		if (slider_setting_cool == -0.001){
			cool_min = cooling_setpoint0 + cool_range_low;
			cool_max = cooling_setpoint0 + cool_range_high;
		} else if(slider_setting_cool > 0.0){
			cool_min = cooling_setpoint0 + cool_range_low * slider_setting_cool;
			cool_max = cooling_setpoint0 + cool_range_high * slider_setting_cool;
			if (cool_range_low != 0.0)
				cool_ramp_low = 2 + (1 - slider_setting_cool);
			else
				cool_ramp_low = 0;
			if (cool_range_high != 0.0)
				cool_ramp_high = 2 + (1 - slider_setting_cool);
			else
				cool_ramp_high = 0;
		} else {
			cool_min = cooling_setpoint0;
			cool_max = cooling_setpoint0;
		}
		if (slider_setting_heat == -0.001){
			heat_min = heating_setpoint0 + heat_range_low;
			heat_max = heating_setpoint0 + heat_range_high;
		} else if (slider_setting_heat > 0.0){
			heat_min = heating_setpoint0 + heat_range_low * slider_setting_heat;
			heat_max = heating_setpoint0 + heat_range_high * slider_setting_heat;
			if (heat_range_low != 0.0)
				heat_ramp_low = -2 - (1 - slider_setting_heat);
			else
				heat_ramp_low = 0;
			if (heat_range_high != 0)
				heat_ramp_high = -2 - (1 - slider_setting_heat);
			else
				heat_ramp_high = 0;
		} else {
			heat_min = heating_setpoint0;
			heat_max = heating_setpoint0;
		}
	}
	if((thermostat_mode != TM_INVALID && thermostat_mode != TM_OFF) || t1 >= time_off)
		last_mode = thermostat_mode;
	else if(thermostat_mode == TM_INVALID)
		last_mode = TM_OFF;// this initializes last mode to off

	if(thermostat_mode != TM_INVALID)
		previous_mode = thermostat_mode;
	else
		previous_mode = TM_OFF;
	if(override_prop.is_valid()){
		if(use_override == OU_OFF && override_prop.get_enumeration() != 0){
			override_prop.setp(OV_NORMAL->get_enumeration_value());
		}
	}

	return TS_NEVER;
}

TIMESTAMP controller::sync(TIMESTAMP t0, TIMESTAMP t1){
	double bid = -1.0;
	int64 no_bid = 0; // flag gets set when the current temperature drops in between the the heating setpoint and cooling setpoint curves
	double rampify = 0.0;
	extern double bid_offset;
	double deadband_shift = 0.0;
	double shift_direction = 0.0;
	double shift_setpoint = 0.0;
	double prediction_ramp = 0.0;
	double prediction_range = 0.0;
	double midpoint = 0.0;
	TIMESTAMP fast_reg_run;
	OBJECT *hdr = OBJECTHDR(this);
	char mktname[1024];
	char ctrname[1024];
	double avgP = 0.0;
	double stdP = 0.0;
	int64 marketId = 0;
	int64 market2Id = 0;
	double clrP = 0.0;
	double pCap = 0.0;
	double pCap2 = 0.0;
	double marginalFraction = 0.0;
	double marginalFraction2 = 0.0;
	enumeration marginMode = AM_NONE;
	double clrQ = 0.0;
	double clrQ2 = 0.0;
	double sellerTotalQ = 0.0;
	enumeration clrType = CT_EXACT;
	enumeration clrType2 = CT_EXACT;
	double monitor = 0.0;
	double demandP = 0.0;
	double loadP = 0.0;
	double dBand = 0.0;
	double heatDemand = 0.0;
	double coolDemand = 0.0;
	if(bidmode != BM_PROXY){
		pAvg->getp(avgP);
		pStd->getp(stdP);
		pMarketId->getp(marketId);
		pClearedPrice->getp(clrP);
		pPriceCap->getp(pCap);
		pMarginalFraction->getp(marginalFraction);
		pMarginMode->getp(marginMode);
		pClearedQuantity->getp(clrQ);
		pSellerTotalQuantity->getp(sellerTotalQ);
		pClearingType->getp(clrType);
	} else if(bidmode == BM_PROXY) {
		avgP = pAvg->get_double();
		stdP = pStd->get_double();
		marketId = pMarketId->get_integer();
		clrP = pClearedPrice->get_double();
		pCap = pPriceCap->get_double();
		marginalFraction = pMarginalFraction->get_double();
		marginMode = pMarginMode->get_enumeration();
		clrQ = pClearedQuantity->get_double();
		sellerTotalQ = pSellerTotalQuantity->get_double();
		clrType = pClearingType->get_enumeration();
	}
	pMonitor->getp(monitor);
	if(control_mode == CN_RAMP || control_mode == CN_DEV_LEVEL){
		pDemand->getp(demandP);
		pLoad->getp(loadP);
		if(use_predictive_bidding == true) {
			pDeadband->getp(dBand);
		}
	} else if(control_mode == CN_DOUBLE_RAMP) {
		pDeadband->getp(dBand);
		pHeatingDemand->getp(heatDemand);
		pCoolingDemand->getp(coolDemand);
	}
	if (control_mode == CN_DEV_LEVEL) {
		if(bidmode != BM_PROXY){
			pMarketId2->getp(market2Id);
			pPriceCap2->getp(pCap2);
			pMarginalFraction2->getp(marginalFraction2);
			pClearedQuantity2->getp(clrQ2);
			pClearingType2->getp(clrType2);
		} else if(bidmode == BM_PROXY){
			market2Id = pMarketId2->get_integer();
			pCap2 = pPriceCap2->get_double();
			marginalFraction2 = pMarginalFraction2->get_double();
			clrQ2 = pClearedQuantity2->get_double();
			clrType2 = pClearingType2->get_enumeration();
		}
		//printf("Reg signal is %f\n",fast_reg_signal);
		fast_reg_run = gl_globalclock + (TIMESTAMP)(reg_period - (gl_globalclock+reg_period) % reg_period);

		if (t1 == fast_reg_run-reg_period){
			if(dev_level_ctrl(t0, t1) != 0){
				GL_THROW("error occured when handling the device level controller");
			}
		}
	}

	if(t1 == next_run && marketId == lastmkt_id && bidmode == BM_PROXY){
		/*
		 * This case is only true when dealing with co-simulation with FNCS.
		 */
		return TS_NEVER;
	}

	/* short circuit if the state variable doesn't change during the specified interval */
	enumeration ps = -1; // ps==-1 means the powerstate is not found -- -1 should never be used
	if ( powerstate_prop.is_valid() )
		powerstate_prop.getp(ps);
	if((t1 < next_run) && (marketId == lastmkt_id)){
		if(t1 <= next_run - bid_delay){
			if(use_predictive_bidding == TRUE && ((control_mode == CN_RAMP && last_setpoint != setpoint0) || (control_mode == CN_DOUBLE_RAMP && (last_heating_setpoint != heating_setpoint0 || last_cooling_setpoint != cooling_setpoint0)))) {
				; // do nothing
			} else if(use_override == OU_ON && t1 == next_run - bid_delay){
				;
			} else {// check to see if we have changed states
				if ( !powerstate_prop.is_valid() ) {
					if(control_mode == CN_DEV_LEVEL)
						return fast_reg_run;
					else
						return next_run;
				} else if ( ps==last_pState ) { // *pState == last_pState)
					if(control_mode == CN_DEV_LEVEL)
						return fast_reg_run;
					else
						return next_run;
				}
			}
		} else {
			if(control_mode == CN_DEV_LEVEL)
				return fast_reg_run;
			else
				return next_run;
		}
	}

	if(control_mode == CN_DEV_LEVEL){

		if((t1 < next_run) && (market2Id == lastmkt_id2)){
			if(t1 <= next_run - bid_delay2){
				if(use_predictive_bidding == TRUE && (control_mode == CN_DEV_LEVEL && last_setpoint != setpoint0)) {
					;
				} else {// check to see if we have changed states
					if(!powerstate_prop.is_valid()){
						return fast_reg_run;
					} else if(ps==last_pState){
						return fast_reg_run;
					}
				}
			} else {
				return fast_reg_run;
			}
		}
	}
	
	if(use_predictive_bidding == TRUE){
		deadband_shift = dBand * 0.5;
	}

	if(control_mode == CN_RAMP){
		// if market has updated, continue onwards
		if(marketId != lastmkt_id){// && (*pAvg == 0.0 || *pStd == 0.0 || setpoint0 == 0.0)){
			lastmkt_id = marketId;
			lastbid_id = -1; // clear last bid id, refers to an old market
			// update using last price
			// T_set,a = T_set + (P_clear - P_avg) * | T_lim - T_set | / (k_T * stdev24)

			clear_price = clrP;
			controller_bid.rebid = false;

			if(use_predictive_bidding == TRUE){
				if((dir > 0 && clear_price < last_p) || (dir < 0 && clear_price > last_p)){
					shift_direction = -1;
				} else if((dir > 0 && clear_price >= last_p) || (dir < 0 && clear_price <= last_p)){
					shift_direction = 1;
				} else {
					shift_direction = 0;
				}
			}
			if(fabs(stdP) < bid_offset){
				set_temp = setpoint0;
			} else if(clear_price < avgP && range_low != 0){
				set_temp = setpoint0 + (clear_price - avgP) * fabs(range_low) / (ramp_low * stdP) + deadband_shift*shift_direction;
			} else if(clear_price > avgP && range_high != 0){
				set_temp = setpoint0 + (clear_price - avgP) * fabs(range_high) / (ramp_high * stdP) + deadband_shift*shift_direction;
			} else {
				set_temp = setpoint0 + deadband_shift*shift_direction;
			}

			if ( use_override==OU_ON && override_prop.is_valid() )
			{
				if ( clear_price<=last_p )
				{
					// if we're willing to pay as much as, or for more than the offered price, then run.
					override_prop.setp(OV_ON->get_enumeration_value()); // *pOverride = 1;
				} else {
					override_prop.setp(OV_OFF->get_enumeration_value()); // *pOverride = -1;
				}
			}

			// clip
			if(set_temp > max){
				set_temp = max;
			} else if(set_temp < min){
				set_temp = min;
			}

			pSetpoint->setp(set_temp);
			//gl_verbose("controller::postsync(): temp %f given p %f vs avg %f",set_temp, market->next.price, market->avg24);
		}
		
		if(dir > 0){
			if(use_predictive_bidding == TRUE){
				if ( ps == *PS_OFF && monitor > (max - deadband_shift)){
					bid = pCap;
				}
				else if ( ps != *PS_OFF && monitor < (min + deadband_shift)){
					bid = 0.0;
					no_bid = 1;
				}
				else if ( ps != *PS_OFF && monitor > max){
					bid = pCap;
				}
				else if ( ps == *PS_OFF && monitor < min){
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(monitor > max){
					bid = pCap;
				} else if (monitor < min){
					bid = 0.0;
					no_bid = 1;
				}
			}
		} else if(dir < 0){
			if(use_predictive_bidding == TRUE){
				if ( ps==*PS_OFF && monitor < (min + deadband_shift) )
				{
					bid = pCap;
				}
				else if ( ps != *PS_OFF && monitor > (max - deadband_shift) )
				{
					bid = 0.0;
					no_bid = 1;
				}
				else if ( ps != *PS_OFF && monitor < min)
				{
					bid = pCap;
				}
				else if ( ps == *PS_OFF && monitor > max)
				{
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(monitor < min){
					bid = pCap;
				} else if (monitor > max){
					bid = 0.0;
					no_bid = 1;
				}
			}
		} else if(dir == 0){
			if(use_predictive_bidding == TRUE){
				if(direction == 0.0) {
					gl_error("the variable direction did not get set correctly.");
				}
				else if ( (monitor > max + deadband_shift || (ps != *PS_OFF && monitor > min - deadband_shift)) && direction > 0 )
				{
					bid = pCap;
				}
				else if ( (monitor < min - deadband_shift || ( ps != *PS_OFF && monitor < max + deadband_shift)) && direction < 0 )
				{
					bid = pCap;
				} else {
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(monitor < min){
					bid = pCap;
				} else if(monitor > max){
					bid = 0.0;
					no_bid = 1;
				} else {
					bid = avgP;
				}
			}
		}

		// calculate bid price
		if(monitor > setpoint0){
			k_T = ramp_high;
			T_lim = range_high;
		} else if(monitor < setpoint0) {
			k_T = ramp_low;
			T_lim = range_low;
		} else {
			k_T = 0.0;
			T_lim = 0.0;
		}
		
		
		if(bid < 0.0 && monitor != setpoint0) {
			bid = avgP + ( (fabs(stdP) < bid_offset) ? 0.0 : (monitor - setpoint0) * (k_T * stdP) / fabs(T_lim) );
		} else if(monitor == setpoint0) {
			bid = avgP;
		}

		// bid the response part of the load
		double residual;
		pTotal->getp(residual);
		/* WARNING ~ bid ID check will not work properly */
		//KEY bid_id = (KEY)(lastmkt_id == *pMarketId ? lastbid_id : -1);
		// override
		//bid_id = -1;
		if(demandP > 0 && no_bid != 1){
			last_p = bid;
			last_q = demandP;
			if(0 != strcmp(market_unit, "")){
				if(0 == gl_convert("kW", market_unit, &(last_q))){
					gl_error("unable to convert bid units from 'kW' to '%s'", market_unit);
					return TS_INVALID;
				}
			}
			//lastbid_id = market->submit(OBJECTHDR(this), -last_q, last_p, bid_id, (BIDDERSTATE)(pState != 0 ? *pState : 0));
			controller_bid.market_id = lastmkt_id;
			controller_bid.price = last_p;
			controller_bid.quantity = -last_q;
			if( powerstate_prop.is_valid() ){
				if ( ps == *PS_ON ) {
					controller_bid.state = BS_ON;
				} else {
					controller_bid.state = BS_OFF;
				}
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			} else {
				controller_bid.state = BS_UNKNOWN;
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			}
			if(controller_bid.bid_accepted == false){
				return TS_INVALID;
			}
			residual -= loadP;

		} else {
			last_p = 0;
			last_q = 0;
			gl_verbose("%s's is not bidding", hdr->name);
		}
		if(residual < -0.001)
			gl_warning("controller:%d: residual unresponsive load is negative! (%.1f kW)", hdr->id, residual);
	} 
	else if(control_mode == CN_DEV_LEVEL){
		// if market has updated, continue onwards
		if(marketId != lastmkt_id && market2Id != lastmkt_id2){// && (*pAvg == 0.0 || *pStd == 0.0 || setpoint0 == 0.0)){
			controller_bid.rebid = false;
			controller_bid2.rebid = false;
			lastmkt_id = marketId;
			lastbid_id = -1; 
			lastmkt_id2 = market2Id;
			lastbid_id2 = -1;// clear last bid id, refers to an old market
			
			P_ONLOCK = 0;			//Ebony
			P_OFFLOCK = 0;			//Ebony
			last_run = next_run;	//Ebony
			engaged = 0;

			last_market = market_flag;

			// If the market failed with some sellers, let's go ahead and use what is available
			if (clrQ == 0) {
				P_OFF = sellerTotalQ;
			}
			else {
				P_OFF = clrQ;
			}

			if (clrQ2 == 0) {
				if(bidmode != BM_PROXY){
					pSellerTotalQuantity2->getp(P_ON);
				} else if(bidmode == BM_PROXY){
					P_ON = pSellerTotalQuantity2->get_double();
				}
			}
			else {
				P_ON = clrQ2;
			}

			// Choose whether we should be part of the control action or not.
			// We didn't actually bid in the last market
			if (last_q == 0) {
				engaged = 0;
			}
			// One of the markets failed
			else if(clrType == CT_FAILURE || clrType == CT_NULL || clrType2 == CT_FAILURE || clrType2 == CT_NULL) {
				engaged = 0;
			}
			// The cleared quantities weren't balanced - we could eventually allow some percentage of difference
			else if (clrQ != clrQ2) {
				engaged = 0;
			}
			// One of the markets didn't clear with any quantity
			else if (clrQ == 0) {
				engaged = 0;
			}
			// One of the markets didn't clear with any quantity
			else if (clrQ2 == 0) {
				engaged = 0;
			}
			// We bid into the OFF->ON market
			else if (market_flag == 0) {   
				clear_price = clrP;

				if (last_p < clear_price) { // Cleared at the right price
					engaged = 1;
				}
				else if (last_p == clear_price) { // We're a marginal unit, so randomize whether to engage or not
					double my_rand = gl_random_uniform(RNGSTATE,0, 1.0);
					if (my_rand <= marginalFraction) {
						engaged = 1;
					}
					else {
						engaged = 0;
					}
				}
				else {
					engaged = 0;
				}	
			}
			// We bid into the ON->OFF market
			else if (market_flag == 1) {
				if(bidmode != BM_PROXY){
					pClearedPrice2->getp(clear_price);
				} else if(bidmode == BM_PROXY) {
					clear_price = pClearedPrice2->get_double();
				}

				if (last_p < clear_price) { // Cleared at the right price
					engaged = 1;
				}
				else if (last_p == clear_price) { // We're a marginal unit, so randomize whether to engage or not
					double my_rand = gl_random_uniform(RNGSTATE,0, 1.0);
					if (my_rand <= marginalFraction2) {
						engaged = 1;
					}
					else {
						engaged = 0;
					}
				}
				else {
					engaged = 0;
				}	
			}
			else {
				engaged = 0;
			}

			if(use_predictive_bidding == TRUE){
				if((dir > 0 && clear_price < last_p) || (dir < 0 && clear_price > last_p)){
					shift_direction = -1;
				} else if((dir > 0 && clear_price >= last_p) || (dir < 0 && clear_price <= last_p)){
					shift_direction = 1;
				} else {
					shift_direction = 0;
				}
			}
		}

		if(dir > 0){
			if(use_predictive_bidding == TRUE){
				if(ps == *PS_OFF && monitor > (max - deadband_shift)){
					bid = pCap;
				} else if(ps != *PS_OFF && monitor < (min + deadband_shift)){
					bid = 0.0;
					no_bid = 1;
				} else if(ps != *PS_OFF && monitor > max){
					bid = pCap;
				} else if(ps == *PS_OFF && monitor < min){
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(monitor > max){
					bid = pCap;
				} else if (monitor < min){
					bid = 0.0;
					no_bid = 1;
				}
			}
		} else if(dir < 0){
			if(use_predictive_bidding == TRUE){
				if(ps == *PS_OFF && monitor < (min + deadband_shift)){
					bid = pCap;
				} else if(ps != *PS_OFF && monitor > (max - deadband_shift)){
					bid = 0.0;
					no_bid = 1;
				} else if(ps != *PS_OFF && monitor < min){
					bid = pCap;
				} else if(ps == *PS_OFF && monitor > max){
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(monitor < min){
					bid = pCap;
				} else if (monitor > max){
					bid = 0.0;
					no_bid = 1;
				}
			}
		} else if(dir == 0){
			if(use_predictive_bidding == TRUE){
				if(direction == 0.0) {
					gl_error("the variable direction did not get set correctly.");
				} else if((monitor > max + deadband_shift || (ps != *PS_OFF && monitor > min - deadband_shift)) && direction > 0){
					bid = pCap;
				} else if((monitor < min - deadband_shift || (ps != *PS_OFF && monitor < max + deadband_shift)) && direction < 0){
					bid = pCap;
				} else {
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(monitor < min){
					bid = pCap;
				} else if(monitor > max){
					bid = 0.0;
					no_bid = 1;
				} else {
					bid = avgP;
				}
			}
		}

		// calculate bid price
		if(monitor > setpoint0){
			k_T = ramp_high;
			T_lim = range_high;
		} else if(monitor < setpoint0) {
			k_T = ramp_low;
			T_lim = range_low;
		} else {
			k_T = 0.0;
			T_lim = 0.0;
		}
		
		
		if(bid < 0.0 && monitor != setpoint0) {
			bid = avgP + ( (fabs(stdP) < bid_offset) ? 0.0 : (monitor - setpoint0) * (k_T * stdP) / fabs(T_lim) );
		} else if(monitor == setpoint0) {
			bid = avgP;
		}

		//Let's dissallow negative or zero bidding in this, for now
		if (bid <= 0.0)
			bid = bid_offset;

		// WARNING ~ bid ID check will not work properly 
		//KEY bid_id = (KEY)(lastmkt_id == *pMarketId ? lastbid_id : -1);
		//KEY bid_id2 = (KEY)(lastmkt_id2 == *pMarketId2 ? lastbid_id2 : -1);

		if(demandP > 0 && no_bid != 1){
			last_p = bid;
			last_q = demandP;
			
			if(ps == *PS_ON){
				if(0 != strcmp(market_unit2, "")){
					if(0 == gl_convert("kW", market_unit2, &(last_q))){
						gl_error("unable to convert bid units from 'kW' to '%s'", market_unit2);
						return TS_INVALID;
					}
				}

				if (last_p > pCap2)
					last_p = pCap2;

				//lastbid_id2 = submit_bid(pMarket2, hdr, last_q, last_p, bid_id2);
				controller_bid2.market_id = lastmkt_id2;
				controller_bid2.price = last_p;
				controller_bid2.quantity = last_q;
				controller_bid2.state = BS_UNKNOWN;
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit2))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt2), "submit_bid_state", "auction", (void *)&controller_bid2, (size_t)sizeof(controller_bid2));
				controller_bid2.rebid = true;
				if(controller_bid2.bid_accepted == false){
					return TS_INVALID;
				}
				market_flag = 1;

				// We had already bid into the other market, so let's cancel that bid out
				if (controller_bid.rebid) {
					//lastbid_id = submit_bid(pMarket, hdr, 0, *pPriceCap, bid_id);
					controller_bid.market_id = lastmkt_id;
					controller_bid.price = pCap;
					controller_bid.quantity = last_q;
					controller_bid.state = BS_UNKNOWN;
					((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
					controller_bid.rebid = true;
					if(controller_bid.bid_accepted == false){
						return TS_INVALID;
					}
				}
			} else if (ps == *PS_OFF) {
				if(0 != strcmp(market_unit, "")){
					if(0 == gl_convert("kW", market_unit, &(last_q))){
						gl_error("unable to convert bid units from 'kW' to '%s'", market_unit);
						return TS_INVALID;
					}
				}

				if (last_p > pCap)
					last_p = pCap;

				//lastbid_id = submit_bid(pMarket, hdr, last_q, last_p, bid_id);
				controller_bid.market_id = lastmkt_id;
				controller_bid.price = last_p;
				controller_bid.quantity = last_q;
				controller_bid.state = BS_UNKNOWN;
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
				if(controller_bid.bid_accepted == false){
					return TS_INVALID;
				}
				market_flag = 0;

				// We had already bid into the other market, so let's cancel that bid out
				if (controller_bid2.rebid) {
					//lastbid_id2 = submit_bid(pMarket2, hdr, 0, *pPriceCap2, bid_id2);
					controller_bid2.market_id = lastmkt_id2;
					controller_bid2.price = pCap2;
					controller_bid2.quantity = last_q;
					controller_bid2.state = BS_UNKNOWN;
					((void (*)(char *, char *, char *, char *, void *, size_t))(*submit2))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt2), "submit_bid_state", "auction", (void *)&controller_bid2, (size_t)sizeof(controller_bid2));
					controller_bid2.rebid = true;
					if(controller_bid2.bid_accepted == false){
						return TS_INVALID;
					}
				}
			}
		} else {
			last_p = 0;
			last_q = 0;
			gl_verbose("%s's is not bidding", hdr->name);
		}

	}
	else if (control_mode == CN_DOUBLE_RAMP){
		/*
		double heat_range_high;
		double heat_range_low;
		double heat_ramp_high;
		double heat_ramp_low;
		double cool_range_high;
		double cool_range_low;
		double cool_ramp_high;
		double cool_ramp_low;
		*/

		DATETIME t_next;
		gl_localtime(t1,&t_next);

		// find crossover
		double midpoint = 0.0;
		if(cool_min - heat_max < dBand){
			switch(resolve_mode){
				case RM_DEADBAND:
					midpoint = (heat_max + cool_min) / 2;
					if(midpoint - dBand/2 < heating_setpoint0 || midpoint + dBand/2 > cooling_setpoint0) {
						gl_error("The midpoint between the max heating setpoint and the min cooling setpoint must be half a deadband away from each base setpoint");
						return TS_INVALID;
					} else {
						heat_max = midpoint - dBand/2;
						cool_min = midpoint + dBand/2;
					}
					break;
				case RM_SLIDING:
					if(heat_max > cooling_setpoint0 - dBand) {
						gl_error("the max heating setpoint must be a full deadband less than the cooling_base_setpoint");
						return TS_INVALID;
					}

					if(cool_min < heating_setpoint0 + dBand) {
						gl_error("The min cooling setpoint must be a full deadband greater than the heating_base_setpoint");
						return TS_INVALID;
					}
					if(last_mode == TM_OFF || last_mode == TM_COOL){
						heat_max = cool_min - dBand;
					} else if (last_mode == TM_HEAT){
						cool_min = heat_max + dBand;
					}
					break;
				default:
					gl_error("unrecognized resolve_mode when double_ramp overlap resolution is needed");
					break;
			}
		}
		// if the market has updated,
		if(lastmkt_id != marketId){
			lastmkt_id = marketId;
			lastbid_id = -1;
			// retrieve cleared price
			clear_price = clrP;
			controller_bid.rebid = false;
			if(clear_price == last_p){
				// determine what to do at the marginal price
				switch(clrType){
					case CT_SELLER:	// may need to curtail
						break;
					case CT_PRICE:	// should not occur
					case CT_NULL:	// q zero or logic error ~ should not occur
						// occurs during the zero-eth market.
						//gl_warning("clearing price and bid price are equal with a market clearing type that involves inequal prices");
						break;
					default:
						break;
				}
			}
			if(use_predictive_bidding == TRUE){
				if((thermostat_mode == TM_COOL && clear_price < last_p) || (thermostat_mode == TM_HEAT && clear_price > last_p)){
					shift_direction = -1;
				} else if((thermostat_mode == TM_COOL && clear_price >= last_p) || (thermostat_mode == TM_HEAT && clear_price <= last_p)){
					shift_direction = 1;
				} else {
					shift_direction = 0;
				}
			}
			may_run = 1;
			// calculate setpoints
			if(fabs(stdP) < bid_offset){
				pCoolingSetpoint->setp(cooling_setpoint0);
				pHeatingSetpoint->setp(heating_setpoint0);
			} else {
				double res;
				if(clear_price > avgP){
					res = cooling_setpoint0 + (clear_price - avgP) * fabs(cool_range_high) / (cool_ramp_high * stdP) + deadband_shift*shift_direction;
					pCoolingSetpoint->setp(res);
					//*pHeatingSetpoint = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_range_high) / (heat_ramp_high * *pStd);
					res = heating_setpoint0 + (clear_price - avgP) * fabs(heat_range_low) / (heat_ramp_low * stdP) + deadband_shift*shift_direction;
					pHeatingSetpoint->setp(res);
				} else if(clear_price < avgP){
					res = cooling_setpoint0 + (clear_price - avgP) * fabs(cool_range_low) / (cool_ramp_low * stdP) + deadband_shift*shift_direction;
					pCoolingSetpoint->setp(res);
					//*pHeatingSetpoint = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_range_low) / (heat_ramp_low * *pStd);
					res = heating_setpoint0 + (clear_price - avgP) * fabs(heat_range_high) / (heat_ramp_high * stdP) + deadband_shift*shift_direction;
					pHeatingSetpoint->setp(res);
				} else {
					res = cooling_setpoint0 + deadband_shift*shift_direction;
					pCoolingSetpoint->setp(res);
					res = heating_setpoint0 + deadband_shift*shift_direction;
					pHeatingSetpoint->setp(res);
				}
			}

			// apply overrides
			if((use_override == OU_ON)){
				if(last_q != 0.0){
					if(clear_price == last_p && clear_price != pCap){
						if ( marginMode==AM_DENY )
						{
							override_prop.setp(OV_OFF->get_enumeration_value()); // *pOverride = -1;
						} else if(marginMode == AM_PROB){
							double r = gl_random_uniform(RNGSTATE,0, 1.0);
							if ( r<marginalFraction )
							{
								override_prop.setp(OV_ON->get_enumeration_value()); // *pOverride = 1;
							}
							else
							{
								override_prop.setp(OV_OFF->get_enumeration_value()); // *pOverride = -1;
							}
						}
					} else if ( clrP<=last_p )
					{
						override_prop.setp(OV_ON->get_enumeration_value()); // *pOverride = 1;
					}
					else
					{
						override_prop.setp(OV_OFF->get_enumeration_value()); // *pOverride = -1;
					}
				}
				else // last_q==0
				{
					override_prop.setp(OV_OFF->get_enumeration_value()); // *pOverride = 0; // 'normal operation'
				}
			}

			//clip
			double heatSetpoint;
			double coolSetpoint;
			pHeatingSetpoint->getp(heatSetpoint);
			pCoolingSetpoint->getp(coolSetpoint);
			if(coolSetpoint > cool_max)
				pCoolingSetpoint->setp(cool_max);
			if(coolSetpoint < cool_min)
				pCoolingSetpoint->setp(cool_min);
			if(heatSetpoint > heat_max)
				pHeatingSetpoint->setp(heat_max);
			if(heatSetpoint < heat_min)
				pHeatingSetpoint->setp(heat_min);

			lastmkt_id = marketId;


		}
		
		// submit bids
		double previous_q = last_q; //store the last value, in case we need it
		last_p = 0.0;
		last_q = 0.0;
		
		// We have to cool
		if(monitor > cool_max && (pThermostatState == NULL || *pThermostatState == 1 || *pThermostatState == 3)){
			last_p = pCap;
			last_q = coolDemand;
		}
		// We have to heat
		else if(monitor < heat_min && (pThermostatState == NULL || *pThermostatState == 1 || *pThermostatState == 2)){
			last_p = pCap;
			last_q = heatDemand;
		}
		// We're floating in between heating and cooling
		else if(monitor > heat_max && monitor < cool_min){
			last_p = 0.0;
			last_q = 0.0;
		}
		// We might heat, if the price is right
		else if(monitor <= heat_max && monitor >= heat_min && (pThermostatState == NULL || *pThermostatState == 1 || *pThermostatState == 2)){
			double ramp, range;
			ramp = (monitor > heating_setpoint0 ? heat_ramp_high : heat_ramp_low);
			range = (monitor > heating_setpoint0 ? heat_range_high : heat_range_low);
			if(monitor != heating_setpoint0){
				last_p = avgP + ( (fabs(stdP) < bid_offset) ? 0.0 : (monitor - heating_setpoint0) * ramp * (stdP) / fabs(range) );
			} else {
				last_p = avgP;
			}
			last_q = heatDemand;
		}
		// We might cool, if the price is right
		else if(monitor <= cool_max && monitor >= cool_min && (pThermostatState == NULL || *pThermostatState == 1 || *pThermostatState == 3)){
			double ramp, range;
			ramp = (monitor > cooling_setpoint0 ? cool_ramp_high : cool_ramp_low);
			range = (monitor > cooling_setpoint0 ? cool_range_high : cool_range_low);
			if(monitor != cooling_setpoint0){
				last_p = avgP + ( (fabs(stdP) < bid_offset) ? 0 : (monitor - cooling_setpoint0) * ramp * (stdP) / fabs(range) );
			} else {
				last_p = avgP;
			}
			last_q = coolDemand;
		}

		if(last_p > pCap)
			last_p = pCap;
		if(last_p < -pCap)
			last_p = -pCap;
		if(0 != strcmp(market_unit, "")){
			if(0 == gl_convert("kW", market_unit, &(last_q))){
				gl_error("unable to convert bid units from 'kW' to '%s'", market_unit);
				return TS_INVALID;
			}
		}
		controller_bid.market_id = lastmkt_id;
		controller_bid.price = last_p;
		controller_bid.quantity = -last_q;

		if(last_q > 0.001){
			if( powerstate_prop.is_valid() ){
				if ( ps == *PS_ON ) {
					controller_bid.state = BS_ON;
				} else {
					controller_bid.state = BS_OFF;
				}
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			} else {
				controller_bid.state = BS_UNKNOWN;
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			}
			if(controller_bid.bid_accepted == false){
				return TS_INVALID;
			}
		}
		else
		{
			if (last_pState != ps)
			{
				KEY bid = (KEY)(lastmkt_id == marketId ? lastbid_id : -1);
				double my_bid = -pCap;
				if ( ps != *PS_OFF  )
					my_bid = last_p;

				if ( ps == *PS_ON ) {
					controller_bid.state = BS_ON;
				} else {
					controller_bid.state = BS_OFF;
				}
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)(&pMkt), "submit_bid_state", "auction", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				if(controller_bid.bid_accepted == false){
					return TS_INVALID;
				}
				controller_bid.rebid = true;
			}
		}
	}

	if ( powerstate_prop.is_valid() )
		last_pState = ps;

	char timebuf[128];
	gl_printtime(t1,timebuf,127);
	//gl_verbose("controller:%i::sync(): bid $%f for %f kW at %s",hdr->id,last_p,last_q,timebuf);
	//return postsync(t0, t1);

	if (control_mode == CN_DEV_LEVEL) {		
		return fast_reg_run;
	}

	return TS_NEVER;
}

TIMESTAMP controller::postsync(TIMESTAMP t0, TIMESTAMP t1){
	TIMESTAMP rv = next_run - bid_delay;
	if(last_setpoint != setpoint0 && control_mode == CN_RAMP){
		last_setpoint = setpoint0;
	}
	if(last_setpoint != setpoint0 && control_mode == CN_DEV_LEVEL){
		last_setpoint = setpoint0;
	}
	if(last_heating_setpoint != heating_setpoint0 && control_mode == CN_DOUBLE_RAMP){
		last_heating_setpoint = heating_setpoint0;
	}
	if(last_cooling_setpoint != cooling_setpoint0 && control_mode == CN_DOUBLE_RAMP){
		last_cooling_setpoint = cooling_setpoint0;
	}

	// Determine the system_mode the HVAC is in
	if(t1 < next_run-bid_delay){
		return next_run-bid_delay;
	}

	if(resolve_mode == RM_SLIDING){
		double auxState = 0;
		double heatState = 0;
		double coolState = 0;
		pAuxState->getp(auxState);
		pHeatState->getp(heatState);
		pCoolState->getp(coolState);
		if(heatState == 1 || auxState == 1){
			thermostat_mode = TM_HEAT;
			if(last_mode == TM_OFF)
				time_off = TS_NEVER;
		} else if(coolState == 1){
			thermostat_mode = TM_COOL;
			if(last_mode == TM_OFF)
				time_off = TS_NEVER;
		} else if(heatState == 0 && auxState == 0 && coolState == 0){
			thermostat_mode = TM_OFF;
			if(previous_mode != TM_OFF)
				time_off = t1 + dtime_delay;
		} else {
			gl_error("The HVAC is in two or more modes at once. This is impossible");
			if(resolve_mode == RM_SLIDING)
				return TS_INVALID; // If the HVAC is in two modes at once then the sliding resolve mode will have conflicting state input so stop the simulation.
		}
	}

	if (t1 - next_run < bid_delay){
		rv = next_run;
	}

	if(next_run == t1){
		next_run += (TIMESTAMP)(this->period);
		rv = next_run - bid_delay;
	}

	return rv;
}

int controller::dev_level_ctrl(TIMESTAMP t0, TIMESTAMP t1){
	if (engaged == 1)
		if (last_market == 1)
			is_engaged = 1;
		else
			is_engaged = -1;
	else
		is_engaged = 0;
	
	OBJECT *hdr = OBJECTHDR(this);
	double my_id = hdr->id;

	// Not sure if this is needed, but lets clean up the Override signal if we are entering a new market
	//  We'll catch the new signal one reg signal too late for now
	if(((t1-last_run) % int(dPeriod)) == 0) {
		override_prop.setp(OV_NORMAL->get_enumeration_value());
		last_override = 0;
	}
	// If engaged and during the first pass, check to see if we should override
	else if (engaged==1)	{ 
		if ( t0 < t1 ) {
			// Only re-calculate this stuff on the defined regulation time
			if(((t1-last_run) % reg_period) == 0){

				// First time through this market period, grab some of the initial data
				if (t1 <=last_run + reg_period) {
					P_ON_init = P_ON;
					delta_u = fast_reg_signal*P_ON_init;		
					locked = 0;
					override_prop.setp(OV_NORMAL->get_enumeration_value());
					last_override = 0;
					u_last = (1+fast_reg_signal)*P_ON_init;
				}
				else {
					delta_u = (1+fast_reg_signal)*P_ON_init - u_last;
					u_last = (1+fast_reg_signal)*P_ON_init;
				}

				// if we are part of the OFF->ON market
				if (delta_u > 0 && last_market == 0) {
					if (locked == 0) {
						if (P_OFF != 0)
							mu0 = delta_u/P_OFF;
						else
							mu0 = 0;
						mu1 = 0;

						r1 = gl_random_uniform(RNGSTATE,0, 1.0);

						if(r1 < mu0){
							override_prop.setp(OV_ON->get_enumeration_value());
							locked = 1;
						} 
						else {
							if (use_override == OU_ON)
								override_prop.setp(OV_OFF->get_enumeration_value()); //keep it in the OFF position
							else
								override_prop.setp(OV_NORMAL->get_enumeration_value());	 //else operate normally is probably not needed
						}
					}
					else if (use_override == OU_ON) {
						override_prop.setp(OV_ON->get_enumeration_value()); //keep it in the ON position
					}
				}
				// Ensure that it stays in the position we have decided on
				else if (last_market == 0) {
					mu0=0;
					if (P_ON != 0.0)
						mu1=-delta_u/P_ON;
					else
						mu1 = 0;
					if (use_override == OU_ON) {
						if (locked == 1)
							override_prop.setp(OV_ON->get_enumeration_value()); //keep it in the ON position
						else
							override_prop.setp(OV_OFF->get_enumeration_value()); //keep it in the OFF position
					}
				}
				// If we are part of the ON->OFF market
				else if (delta_u < 0 && last_market == 1) {
					if (locked == 0) {
						mu0=0;
						if (P_ON != 0.0)
							mu1=-delta_u/P_ON;
						else
							mu1 = 0;

						r1 = gl_random_uniform(RNGSTATE,0, 1.0);

						if(r1 < mu1){
							override_prop.setp(OV_OFF->get_enumeration_value());
							locked = 1;
						} else {
							if (use_override == OU_ON)
								override_prop.setp(OV_ON->get_enumeration_value()); // keep it in the ON position
							else
								override_prop.setp(OV_NORMAL->get_enumeration_value());	// operate normally is probably not needed
						}	
					}
					else if (use_override == OU_ON) {
						override_prop.setp(OV_OFF->get_enumeration_value()); //keep it in the OFF position
					}
				}
				else if (last_market == 1) {
					if (P_OFF != 0)
							mu0 = delta_u/P_OFF;
						else
							mu0 = 0;
						mu1 = 0;
					if (use_override == OU_ON) {
						if (locked == 1)
							override_prop.setp(OV_OFF->get_enumeration_value()); //keep it in the OFF position
						else
							override_prop.setp(OV_ON->get_enumeration_value()); //keep it in the ON position
					}
				}
				else {
					mu0 = 0;
					mu1 = 0;
					override_prop.setp(OV_NORMAL->get_enumeration_value());
				}

				last_override = override_prop.get_integer();

				P_OFFLOCK = P_OFFLOCK + mu1*P_ON;			
				P_ON = (1-mu1)*P_ON;
				P_ONLOCK = P_ONLOCK + mu0*P_OFF;
				P_OFF = (1-mu0)*P_OFF; 
			}
			else {
				override_prop.setp(last_override);
			}
		}
		else {
			override_prop.setp(last_override);
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_controller(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(controller::oclass);
		if (*obj!=NULL)
		{
			controller *my = OBJECTDATA(*obj,controller);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(controller);
}

EXPORT int init_controller(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
		{
			return OBJECTDATA(obj,controller)->init(parent);
		}
		else
			return 0;
	}
	INIT_CATCHALL(controller);
}

EXPORT int isa_controller(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,controller)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_controller(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	controller *my = OBJECTDATA(obj,controller);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			//obj->clock = t1;
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			//obj->clock = t1;
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			obj->clock = t1;
			break;
		default:
			gl_error("invalid pass request (%d)", pass);
			return TS_INVALID;
			break;
		}
		return t2;
	}
	SYNC_CATCHALL(controller);
}

// EOF
