/** $Id: double_controller.cpp 1182 2010-1-25 22:08:36Z mhauer $
	Copyright (C) 2010 Battelle Memorial Institute
	@file double_controller.cpp
	@defgroup double_controller Two-setpoint transactive controller, modified OlyPen experiment style for combined heating & cooling logic
	@ingroup market

 **/


#include "double_controller.h"

CLASS *double_controller::oclass = NULL;

double_controller::double_controller(MODULE *module){
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"double_controller",sizeof(double_controller),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
			PT_enumeration, "thermostat_mode", PADDR(thermostat_mode),
				PT_KEYWORD, "INVALID", TM_INVALID,
				PT_KEYWORD, "OFF", TM_OFF,
				PT_KEYWORD, "HEAT", TM_HEAT,
				PT_KEYWORD, "COOL", TM_COOL,
			PT_enumeration, "last_mode", PADDR(last_mode), PT_ACCESS, PA_REFERENCE,
				PT_KEYWORD, "INVALID", TM_INVALID,
				PT_KEYWORD, "OFF", TM_OFF,
				PT_KEYWORD, "HEAT", TM_HEAT,
				PT_KEYWORD, "COOL", TM_COOL,
			PT_enumeration, "resolve_mode", PADDR(resolve_mode),
				PT_KEYWORD, "NONE", RM_NONE,
				PT_KEYWORD, "DEADBAND", RM_DEADBAND,
				PT_KEYWORD, "STICKY", RM_STICKY,
			PT_enumeration, "setup_mode", PADDR(setup_mode),
				PT_KEYWORD, "NONE", ST_NONE,
				PT_KEYWORD, "HOUSE", ST_HOUSE,
			PT_int64, "last_mode_timer", PADDR(last_mode_timer),
			PT_double, "cool_ramp_low", PADDR(cool_ramp_low),
			PT_double, "cool_ramp_high", PADDR(cool_ramp_high),
			PT_double, "cool_range_low", PADDR(cool_range_low),
			PT_double, "cool_range_high", PADDR(cool_range_high),
			PT_double, "cool_set_base", PADDR(cool_set_base),
			PT_double, "cool_setpoint", PADDR(cool_setpoint),
			PT_double, "heat_ramp_low", PADDR(heat_ramp_low),
			PT_double, "heat_ramp_high", PADDR(heat_ramp_high),
			PT_double, "heat_range_low", PADDR(heat_range_low),
			PT_double, "heat_range_high", PADDR(heat_range_high),
			PT_double, "heat_set_base", PADDR(heat_set_base),
			PT_double, "heat_setpoint", PADDR(heat_setpoint),
			PT_char32, "temperature_name", PADDR(temperature_name),
			PT_char32, "cool_setpoint_name", PADDR(cool_setpoint_name),
			PT_char32, "cool_demand_name", PADDR(cool_demand_name),
			PT_char32, "heat_setpoint_name", PADDR(heat_setpoint_name),
			PT_char32, "heat_demand_name", PADDR(heat_demand_name),
			PT_char32, "load_name", PADDR(load_name),
			PT_char32, "total_load_name", PADDR(total_load_name),
			PT_object, "market", PADDR(pMarket), PT_DESCRIPTION, "the market to bid into",
			PT_double, "market_period", PADDR(market_period),
			PT_double, "bid_price", PADDR(last_price), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid price",
			PT_double, "bid_quant", PADDR(last_quant), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid quantity",
			PT_char32, "load", PADDR(load), PT_DESCRIPTION, "the current controlled load",
			PT_char32, "total", PADDR(total), PT_DESCRIPTION, "the uncontrolled load (if any)",
			PT_double, "last_price", PADDR(last_price),
			PT_double, "cool_bid", PADDR(cool_bid),
			PT_double, "heat_bid", PADDR(heat_bid),
			
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(double_controller));
	}
}

int double_controller::create(){
	memset(this, 0, sizeof(double_controller));
	return 1;
}

void double_controller::cheat(){
	switch(setup_mode){
		case ST_NONE:
			break;
		case ST_HOUSE:
			strcpy(temperature_name, "air_temperature");
			strcpy(deadband_name, "thermostat_deadband");
			strcpy(cool_setpoint_name, "cooling_setpoint");
			strcpy(cool_demand_name, "cooling_demand");
			strcpy(heat_setpoint_name, "heating_setpoint");
			strcpy(heat_demand_name, "heating_demand");
			strcpy(load_name, "hvac_load");
			strcpy(total_load_name, "total_load");
			break;
		default:
			break;
	}
}

void double_controller::fetch(double **value, char *name, OBJECT *parent, PROPERTY **prop, char *goal){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_property(parent, name);
	if(*name == 0){
		GL_THROW("%s property not specified", goal);
	}
	if(*prop == NULL){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		sprintf(tname, "double_controller:%i", hdr->id);
		GL_THROW("%s: double_controller unable to find property \'%s\'", namestr, name);
	} else {
		*value = gl_get_double(parent, *prop);
		if(*value == 0){
			char tname[32];
			char *namestr = (hdr->name ? hdr->name : tname);
			sprintf(tname, "double_controller:%i", hdr->id);
			GL_THROW("%s: property \'%s\' is not a double", namestr, name);
		}
	}
}

int double_controller::init(OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	char tname[32];
	char *namestr = (hdr->name ? hdr->name : tname);

	sprintf(tname, "double_controller:%i", hdr->id);

	cheat();

	if(parent == NULL){
		gl_error("%s: double_controller has no parent, therefore nothing to control", namestr);
		return 0;
	}

	if(pMarket == NULL){
		gl_error("%s: double_controller has no market, therefore no price signals", namestr);
		return 0;
	}

	//market = OBJECTDATA(pMarket, auction);
	pAvg24 =	gl_get_double_by_name(pMarket, "avg24");
	pStd24 =	gl_get_double_by_name(pMarket, "std24");
	pPeriod =	gl_get_double_by_name(pMarket, "period");
	pNextP = 	gl_get_double_by_name(pMarket, "next.P");
	pMarketID = gl_get_int64_by_name(pMarket, "market_id");

	if(pAvg24 == 0){
		gl_error("%s: double_controller market has no avg24 (and is not a market)", namestr);
		return 0;
	}
	if(pStd24 == 0){
		gl_error("%s: double_controller market has no std24 (and is not a market)", namestr);
		return 0;
	}if(pPeriod == 0){
		gl_error("%s: double_controller market has no period (and is not a market)", namestr);
		return 0;
	}if(pNextP == 0){
		gl_error("%s: double_controller market has no next.P (and is not a market)", namestr);
		return 0;
	}if(pMarketID == 0){
		gl_error("%s: double_controller market has no market_id (and is not a market)", namestr);
		return 0;
	}
	fetch(&temperature_ptr, temperature_name, parent, &temperature_prop, "temperature_name");
	fetch(&load_ptr, load_name, parent, &load_prop, "load_name");
	fetch(&total_load_ptr, total_load_name, parent, &total_load_prop, "total_load_name");
	fetch(&cool_setpoint_ptr, cool_setpoint_name, parent, &cool_setpoint_prop, "cool_setpoint_name");
	fetch(&cool_demand_ptr, cool_demand_name, parent, &cool_demand_prop, "cool_demand_prop");
	fetch(&heat_setpoint_ptr, heat_setpoint_name, parent, &heat_setpoint_prop, "heat_setpoint_name");
	fetch(&heat_demand_ptr, heat_demand_name, parent, &heat_demand_prop, "heat_demand_name");

	if(heat_ramp_low > 0 || heat_ramp_high > 0){
		gl_error("%s: heating ramp values must be non-positive", namestr);
		return 0;
	}
	if(cool_ramp_low < 0 || cool_ramp_high < 0){
		gl_error("%s: cooling ramp values must be non-negative", namestr);
		return 0;
	}
	if(cool_range_low > 0 || heat_range_low > 0){
		gl_error("%s: heating and cooling minimum temperature ranges must be non-positive", namestr);
		return 0;
	}
	if(cool_range_high < 0 || heat_range_high < 0){
		gl_error("%s: heating and cooling maximum temperature ranges must be non-negative", namestr);
		return 0;
	}

	next_run = 0;

	return 1;
}

TIMESTAMP double_controller::presync(TIMESTAMP t0, TIMESTAMP t1){
	if(thermostat_mode == TM_INVALID)
		thermostat_mode = TM_OFF;

	if(next_run == 0){
		cool_set_base = *cool_setpoint_ptr;
		heat_set_base = *heat_setpoint_ptr;
	}
	if(t0 == next_run){
		cool_temp_min = cool_set_base - cool_range_low;
		cool_temp_max = cool_set_base + cool_range_high;
		heat_temp_min = heat_set_base - heat_range_low;
		heat_temp_max = heat_set_base + heat_range_high;
		temperature = *temperature_ptr;
	}
	if(t1 > t0){
		if(thermostat_mode != TM_INVALID && thermostat_mode != TM_OFF){
			last_mode = thermostat_mode;
		}
	}
	return TS_NEVER;
}

TIMESTAMP double_controller::sync(TIMESTAMP t0, TIMESTAMP t1){
	double bid = -1.0;
	double demand = 0.0;
	double heat_ramp, cool_ramp;
	double new_limit;
	OBJECT *hdr = OBJECTHDR(this);

	/* short circuit if we've run recently */
	if(t1 < next_run){
		return next_run; // soft event
		//return TS_NEVER;
	}

	heat_limit = heat_temp_max;
	cool_limit = cool_temp_min;

	// determine bids

	// default behavior for resolving bids given a temperature input
	if(temperature > cool_temp_max){
		cool_bid = 9999.0;
	} else if(temperature < cool_temp_min){
		cool_bid = 0.0;
	} else {
		cool_ramp = (temperature > cool_set_base ? cool_ramp_high : cool_ramp_low);
		cool_limit = (temperature > cool_set_base ? cool_temp_max : cool_temp_min);
		cool_bid = *pAvg24 + (temperature - cool_set_base) * (cool_ramp * *pStd24) / fabs(cool_limit - cool_set_base);
	}
	// heating bid
	if(temperature > heat_temp_max){
		heat_bid = 0.0;
	} else if(temperature < heat_temp_max){
		heat_bid = 9999.0;
	} else {
		heat_ramp = (temperature > heat_set_base ? heat_ramp_high : heat_ramp_low);
		heat_limit = (temperature > heat_set_base ? heat_temp_max : heat_temp_min);
		heat_bid = *pAvg24 + (temperature - heat_set_base) * (heat_ramp * *pStd24) / fabs(heat_limit - heat_set_base);
	}

	// clip setpoints and bids
	switch(resolve_mode){
		case RM_DEADBAND:
			// forces a deadband at the midpoint between the two setpoint limits
			if(cool_limit - heat_limit < deadband){
				new_limit = (cool_limit + heat_limit) / 2;
				heat_limit = new_limit - deadband/2;
				cool_limit = new_limit + deadband/2;
				if(temperature > heat_limit){
					heat_bid = 0.0;
				}
				if(temperature < cool_limit){
					cool_bid = 0.0;
				}
			}
			break;
		case RM_STICKY:
			break;
		case RM_NONE:
		default:
			// if the two limits are close, clip whichever one has not been used recently (else cooling)
			// if new limit would not bid at current temperature, update accordingly
			if(last_mode == TM_OFF || last_mode == TM_COOL){
				if(cool_limit - heat_limit < deadband){
					new_limit = cool_limit - deadband;
					if(temperature > new_limit){
						heat_bid = 0.0;
					}
				}
			} else if(last_mode == TM_HEAT){
				if(cool_limit - heat_limit < deadband){
					new_limit = heat_limit - deadband;
					if(temperature < new_limit){
						cool_bid = 0.0;
					}
				}
			} 
			break;
	}

	// if this turns on, it will be...
	if(heat_bid > cool_bid){
		thermostat_mode = TM_HEAT;
	} else if(cool_bid > heat_bid){
		thermostat_mode = TM_COOL;
	} else if(cool_bid == heat_bid){
		thermostat_mode = TM_OFF; // equally demanded, or both == 0.0
	}

	// determine quantity
	if(cool_bid > 0.0){
		cool_demand = *cool_demand_ptr;
	} else {
		cool_demand = 0.0;
	} 

	if(heat_bid > 0.0){
		heat_demand = *heat_demand_ptr;
	} else {
		heat_demand = 0.0;
	}

	// post bid
	if(bid_mode == BM_ON){
		int bid_id = (lastbid_id == *pMarketID ? lastbid_id : -1);
		if(thermostat_mode == TM_HEAT){
			bid_price = heat_bid;
			bid_quant = heat_demand;
		} else if(thermostat_mode == TM_COOL){
			bid_price = cool_bid;
			bid_quant = cool_demand;
		} else if(thermostat_mode == TM_OFF){
			bid_price = 0.0;
			bid_quant = 0.0;
		}
		if(state_ptr != 0){
			lastbid_id = submit_bid_state(pMarket, hdr, -bid_quant, bid_price, bid_id, (*state_ptr > 0 ? 1 : 0));
		} else {
			lastbid_id = submit_bid(pMarket, hdr, -bid_quant, bid_price, bid_id);
		}
	} else {
		bid_price = 0.0;
		bid_quant = 0.0;
	}

	//return next_run > 0 ? next_run : TS_NEVER;
	return TS_NEVER;
}

TIMESTAMP double_controller::postsync(TIMESTAMP t0, TIMESTAMP t1){
	if(t0 < next_run){
		return next_run;
		//return TS_NEVER;
	}

	next_run = (TIMESTAMP)(*pPeriod) + t1;
	
	// update price & determine auction results
	if(*pMarketID != lastmkt_id){
		lastmkt_id = *pMarketID;
		if(*pAvg24 == 0.0 || *pStd24 == 0.0 || cool_set_base == 0.0 || heat_set_base == 0.0){
			//return next_run; // not enough input data
			return TS_NEVER;
		}

		if(bid_mode == BM_ON){
			// compare current bid to market results
			if(*pNextP > bid_price){ // market price greater than our bid
				cool_setpoint = cool_temp_max;
				heat_setpoint = heat_temp_min;
			} else {
				if(cool_bid > 0.0){
					double ramp = (temperature > cool_set_base ? cool_ramp_high : cool_ramp_low);
					cool_setpoint = cool_set_base + (*pNextP - *pAvg24) * fabs(cool_limit - cool_set_base) / (ramp * *pStd24);
					if(cool_setpoint > cool_temp_max){
						cool_setpoint = cool_temp_max;
					} else if(cool_setpoint < cool_limit){
						cool_setpoint = cool_limit;
					}
				} else {
					cool_setpoint = cool_temp_max;
				}
				if(heat_bid > 0.0){
					double ramp = (temperature > heat_set_base ? heat_ramp_high : heat_ramp_low);
					heat_setpoint = heat_set_base + (*pNextP - *pAvg24) * fabs(heat_limit - heat_set_base) / (ramp * *pStd24);
					if(heat_setpoint < heat_temp_min){
						heat_setpoint = heat_temp_min;
					} else if(heat_setpoint > heat_limit){
						heat_setpoint = heat_limit;
					}
				} else {
					heat_setpoint = heat_temp_min;
				}
			}
		} else { // determine setpoints from current price
			;
		}
	}

	// clip setpoints
	// post setpoints

	return next_run > 0 ? next_run : TS_NEVER;
	//return TS_NEVER;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_double_controller(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(double_controller::oclass);
		if (*obj!=NULL)
		{
			double_controller *my = OBJECTDATA(*obj,double_controller);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_double_controller: %s", msg);
	}
	return 1;
}

EXPORT int init_double_controller(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL){
			return OBJECTDATA(obj,double_controller)->init(parent);
		}
	}
	catch (char *msg)
	{
		char name[64];
		gl_error("init_double_controller(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
	}
	return 1;
}

EXPORT TIMESTAMP sync_double_controller(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	double_controller *my = OBJECTDATA(obj,double_controller);
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
		gl_error("sync_double_controller(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
	}
	return t2;
}

// EOF
