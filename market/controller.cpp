/** $Id: controller.cpp 1182 2009-09-09 22:08:36Z mhauer $
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
		oclass = gl_register_class(module,"controller",sizeof(controller),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
			PT_enumeration, "simple_mode", PADDR(simplemode),
				PT_KEYWORD, "NONE", SM_NONE,
				PT_KEYWORD, "HOUSE_HEAT", SM_HOUSE_HEAT,
				PT_KEYWORD, "HOUSE_COOL", SM_HOUSE_COOL,
				PT_KEYWORD, "HOUSE_PREHEAT", SM_HOUSE_PREHEAT,
				PT_KEYWORD, "HOUSE_PRECOOL", SM_HOUSE_PRECOOL,
			PT_enumeration, "bid_mode", PADDR(bidmode),
				PT_KEYWORD, "ON", BM_ON,
				PT_KEYWORD, "OFF", BM_OFF,
			PT_double, "ramp_low", PADDR(kT_L), PT_DESCRIPTION, "the comfort response below the setpoint",
			PT_double, "ramp_high", PADDR(kT_H), PT_DESCRIPTION, "the comfort response above the setpoint",
			PT_double, "Tmin", PADDR(Tmin), PT_DESCRIPTION, "the setpoint limit on the low side",
			PT_double, "Tmax", PADDR(Tmax), PT_DESCRIPTION, "the setpoint limit on the high side",
			PT_char32, "target", PADDR(target), PT_DESCRIPTION, "the observed property (e.g., air temperature)",
			PT_char32, "setpoint", PADDR(setpoint), PT_DESCRIPTION, "the controlled property (e.g., heating setpoint)",
			PT_char32, "demand", PADDR(demand), PT_DESCRIPTION, "the controlled load when on",
			PT_char32, "load", PADDR(load), PT_DESCRIPTION, "the current controlled load",
			PT_char32, "total", PADDR(total), PT_DESCRIPTION, "the uncontrolled load (if any)",
			PT_object, "market", PADDR(pMarket), PT_DESCRIPTION, "the market to bid into",
			PT_char32, "state", PADDR(state), PT_DESCRIPTION, "the state property of the controlled load",
			PT_double, "bid_price", PADDR(last_p), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid price",
			PT_double, "bid_quant", PADDR(last_q), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid quantity",
			PT_double, "set_temp", PADDR(set_temp), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the reset value",
			PT_double, "base_setpoint", PADDR(setpoint0),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(controller));
	}
}

int controller::create(){
	memset(this, 0, sizeof(controller));
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
			kT_L = -2;
			kT_H = -2;
			Tmin = -5;
			Tmax = 0;
			dir = -1;
			break;
		case SM_HOUSE_COOL:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "cooling_setpoint");
			sprintf(demand, "cooling_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			kT_L = 2;
			kT_H = 2;
			Tmin = 0;
			Tmax = 5;
			dir = 1;
			break;
		case SM_HOUSE_PREHEAT:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "heating_setpoint");
			sprintf(demand, "heating_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			kT_L = -2;
			kT_H = -2;
			Tmin = -5;
			Tmax = 3;
			dir = -1;
			break;
		case SM_HOUSE_PRECOOL:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "cooling_setpoint");
			sprintf(demand, "cooling_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			kT_L = 2;
			kT_H = 2;
			Tmin = -3;
			Tmax = 5;
			dir = 1;
			break;
		default:
			break;
	}
}


/** convenience shorthand
 **/
void controller::fetch(double **prop, char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_double_by_name(parent, name);
	if(*prop == NULL){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		sprintf(tname, "controller:%i", hdr->id);
		GL_THROW("%s: controller unable to find %s", namestr, name);
	}
}

/** initialization process
 **/
int controller::init(OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	char tname[32];
	char *namestr = (hdr->name ? hdr->name : tname);
//	double high, low;

	sprintf(tname, "controller:%i", hdr->id);

	cheat();

	if(parent == NULL){
		gl_error("%s: controller has no parent, therefore nothing to control", namestr);
		return 0;
	}

	if(pMarket == NULL){
		gl_error("%s: controller has no market, therefore no price signals", namestr);
		return 0;
	}

	market = OBJECTDATA(pMarket, auction);

	if(target[0] == 0){
		GL_THROW("target property not specified");
	}
	if(setpoint[0] == 0){
		GL_THROW("setpoint property not specified");;
	}
	if(demand[0] == 0){
		GL_THROW("demand property not specified");
	}
	if(total[0] == 0){
		GL_THROW("total property not specified");
	}
	if(load[0] == 0){
		GL_THROW("load property not specified");
	}
	fetch(&pMonitor, target, parent);
	fetch(&pSetpoint, setpoint, parent);
	fetch(&pDemand, demand, parent);
	fetch(&pTotal, total, parent);
	fetch(&pLoad, load, parent);

	if(dir == 0){
		double high = kT_H * Tmax;
		double low = kT_L * Tmin;
		if(high > low){
			dir = 1;
		} else if(high < low){
			dir = -1;
		} else if((high == low) && (fabs(kT_H) > 0.001 || fabs(kT_L) > 0.001)){
			dir = 0;
			gl_warning("%s: controller has no price ramp", namestr);
			/* occurs given no price variation, or no control width (use a normal thermostat?) */
		}
		if(kT_L * kT_H < 0){
			gl_warning("%s: controller price curve is not injective and may behave strangely");
			/* TROUBLESHOOTING
				The price curve 'changes directions' at the setpoint, which may create odd
				conditions in a number of circumstances.
			 */
		}
	}

	setpoint0 = -1; // key to check first thing

	double period = market->period;
	next_run = gl_globalclock + (TIMESTAMP)(period - fmod(gl_globalclock+period,period));
	
	if(state[0] != 0){
		// grab state pointer
		pState = gl_get_enum_by_name(parent, state);
		if(pState == 0){
			gl_error("state property name \'%s\' is not published by parent class", state);
			return 0;
		}
	}

	return 1;
}

TIMESTAMP controller::presync(TIMESTAMP t0, TIMESTAMP t1){
	if(setpoint0 == -1){
		setpoint0 = *pSetpoint;
	}
	if(t0 == next_run){
		min = setpoint0 + Tmin;
		max = setpoint0 + Tmax;
	}
	return TS_NEVER;
}

TIMESTAMP controller::sync(TIMESTAMP t0, TIMESTAMP t1){
	double bid = -1.0;
	double demand = 0.0;
	OBJECT *hdr = OBJECTHDR(this);

	/* short circuit */
	if(t0 < next_run){
		return TS_NEVER;
	}

	if(dir > 0){
		if(*pMonitor > max){
			bid = 9999.0;
		} else if (*pMonitor < min){
			bid = 0.0;
		}
	} else if(dir < 0){
		if(*pMonitor < min){
			bid = 9999.0;
		} else if(*pMonitor > max){
			bid = 0.0;
		}
	} else if(dir == 0){
		if(*pMonitor < min){
			bid = 9999.0;
		} else if(*pMonitor > max){
			bid = 0.0;
		} else {
			bid = market->avg24; // override due to lack of "real" curve
		}
	}

	// calculate bid price
	if(*pMonitor > setpoint0){
		k_T = kT_H;
		T_lim = max;
	} else {
		k_T = kT_L;
		T_lim = min;
	}

	if(bid < 0.0)
		bid = market->avg24 + (*pMonitor - setpoint0) * (k_T * market->std24) / fabs(T_lim - setpoint0);

	// bid the response part of the load
	double residual = *pTotal;
	int bid_id = (lastbid_id == market->market_id ? lastbid_id : -1);
	if(bid > 0.0 && *pDemand > 0){
		last_p = bid;
		last_q = *pDemand;
		//lastbid_id = market->submit(OBJECTHDR(this), -last_q, last_p, bid_id, (BIDDERSTATE)(pState != 0 ? *pState : 0));
		if(pState != 0){
			lastbid_id = submit_bid_state(pMarket, hdr, -last_q, last_p, bid_id, (*pState > 0 ? 1 : 0));
		} else {
			lastbid_id = submit_bid(pMarket, hdr, -last_q, last_p, bid_id);
		}
		residual -= *pLoad;
	} else {
		last_p = 0;
		last_q = 0;
	}
	if (residual>0){
		//lastbid_id = market->submit(OBJECTHDR(this), -residual, 9999, bid_id, (BIDDERSTATE)(pState != 0 ? *pState : 0));
		if(pState != 0){
			lastbid_id = submit_bid_state(pMarket, hdr, -last_q, 9999.0, bid_id, (*pState > 0 ? 1 : 0));
		} else {
			lastbid_id = submit_bid(pMarket, hdr, -last_q, 9999.0, bid_id);
		}
	}
	else if(residual < 0)
		gl_warning("controller:%d: residual unresponsive load is negative! (%.1f kW)", hdr->id, residual);

	char timebuf[128];
	gl_printtime(t1,timebuf,127);
	//gl_verbose("controller:%i::sync(): bid $%f for %f kW at %s",hdr->id,last_p,last_q,timebuf);
	return TS_NEVER;
}

TIMESTAMP controller::postsync(TIMESTAMP t0, TIMESTAMP t1){

	if(t0 < next_run){
		return TS_NEVER;
	}

	next_run += market->period;

	if(market->market_id != lastmkt_id){
		lastmkt_id = market->market_id;
		if(market->avg24 == 0.0 || market->std24 == 0.0 || setpoint0 == 0.0){
			return TS_NEVER; /* not enough input data */
		}
		// update using last price
		// T_set,a = T_set + (P_clear - P_avg) * | T_lim - T_set | / (k_T * stdev24)

		if(market->next.price > last_p){ // if we lost the auction
			/* failed to win auction */
			may_run = 0;
			if(dir > 0){
				set_temp = max;
			} else {
				set_temp = min;
			}
		} else {
			set_temp = setpoint0 + (market->next.price - market->avg24) * fabs(T_lim - setpoint0) / (k_T * market->std24);
			may_run = 1;
		}

		// clip
		if(set_temp > max){
			set_temp = max;
		} else if(set_temp < min){
			set_temp = min;
		}

		*pSetpoint = set_temp;
		//gl_verbose("controller::postsync(): temp %f given p %f vs avg %f",set_temp, market->next.price, market->avg24);
		
	}
	return TS_NEVER;
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
	}
	catch (char *msg)
	{
		gl_error("create_controller: %s", msg);
	}
	return 1;
}

EXPORT int init_controller(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL){
			return OBJECTDATA(obj,controller)->init(parent);
		}
	}
	catch (char *msg)
	{
		char name[64];
		gl_error("init_controller(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
	}
	return 1;
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
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			obj->clock = t1;
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
	}
	catch (char *msg)
	{
		char name[64];
		gl_error("sync_controller(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
	}
	return t2;
}

// EOF
