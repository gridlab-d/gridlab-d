/** $Id: double_controller.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2010 Battelle Memorial Institute
	@file double_controller.cpp
	@defgroup double_controller Two-setpoint transactive controller, modified OlyPen experiment style for combined heating & cooling logic
	@ingroup market

 **/


#include "double_controller.h"

CLASS *double_controller::oclass = NULL;

double_controller::double_controller(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"double_controller",sizeof(double_controller),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class double_controller";
		else
			oclass->trl = TRL_QUALIFIED;

		if (gl_publish_variable(oclass,
			PT_enumeration, "thermostat_mode", PADDR(thermostat_mode),
				PT_KEYWORD, "INVALID", (enumeration)TM_INVALID,
				PT_KEYWORD, "OFF", (enumeration)TM_OFF,
				PT_KEYWORD, "HEAT", (enumeration)TM_HEAT,
				PT_KEYWORD, "COOL", (enumeration)TM_COOL,
			PT_enumeration, "last_mode", PADDR(last_mode), PT_ACCESS, PA_REFERENCE,
				PT_KEYWORD, "INVALID", (enumeration)TM_INVALID,
				PT_KEYWORD, "OFF", (enumeration)TM_OFF,
				PT_KEYWORD, "HEAT", (enumeration)TM_HEAT,
				PT_KEYWORD, "COOL", (enumeration)TM_COOL,
			PT_enumeration, "resolve_mode", PADDR(resolve_mode),
				PT_KEYWORD, "NONE", (enumeration)RM_NONE,
				PT_KEYWORD, "DEADBAND", (enumeration)RM_DEADBAND,
				PT_KEYWORD, "STICKY", (enumeration)RM_STICKY,
			PT_enumeration, "setup_mode", PADDR(setup_mode),
				PT_KEYWORD, "NONE", (enumeration)ST_NONE,
				PT_KEYWORD, "HOUSE", (enumeration)ST_HOUSE,
			PT_enumeration, "bid_mode", PADDR(bid_mode),
				PT_KEYWORD, "ON", (enumeration)BM_ON,
				PT_KEYWORD, "OFF", (enumeration)BM_OFF,
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
			PT_char32, "deadband_name", PADDR(deadband_name),
			PT_char32, "state_name", PADDR(state_name),
			PT_object, "market", PADDR(pMarket), PT_DESCRIPTION, "the market to bid into",
			PT_double, "market_period", PADDR(market_period),
			PT_double, "bid_price", PADDR(bid_price), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid price",
			PT_double, "bid_quant", PADDR(bid_quant), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid quantity",
			PT_char32, "load", PADDR(load), PT_DESCRIPTION, "the current controlled load",
			PT_char32, "total", PADDR(total), PT_DESCRIPTION, "the uncontrolled load (if any)",
			PT_double, "last_price", PADDR(last_price),
			PT_double, "temperature", PADDR(temperature),
			PT_double, "cool_bid", PADDR(cool_bid),
			PT_double, "heat_bid", PADDR(heat_bid),
			PT_double, "cool_demand", PADDR(cool_demand),
			PT_double, "heat_demand", PADDR(heat_demand),
			PT_double, "price", PADDR(price),
			PT_double, "avg_price", PADDR(avg_price),
			PT_double, "stdev_price", PADDR(stdev_price),
			
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(double_controller));
	}
}

int double_controller::create(){
	memset(this, 0, sizeof(double_controller));
	sprintf(avg_target, "avg24");
	sprintf(std_target, "std24");
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
			strcpy(state_name,"power_state");
			break;
		default:
			break;
	}
}

void double_controller::fetch(double **value, char *name, OBJECT *parent, PROPERTY **prop, char *goal){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_property(parent, name);
	if(*name == 0){
		GL_THROW("double_controller:%i, %s property not specified", hdr->id,goal);
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
	pPeriod =	gl_get_double_by_name(pMarket, "period");
	pNextP = 	gl_get_double_by_name(pMarket, "next.P");
	pMarketID = gl_get_int64_by_name(pMarket, "market_id");
	pAvg =		gl_get_double_by_name(pMarket, avg_target);
	pStd =		gl_get_double_by_name(pMarket, std_target);

	if(pAvg == 0){
		gl_error("%s: double_controller market has no %s average property", namestr, avg_target.get_string());
		return 0;
	}
	if(pStd == 0){
		gl_error("%s: double_controller market has no %s standard deviation property", namestr, std_target.get_string());
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
	fetch(&cool_setpoint_ptr, cool_setpoint_name, parent, &cool_setpoint_prop, "cool_setpoint_name");
	fetch(&heat_setpoint_ptr, heat_setpoint_name, parent, &heat_setpoint_prop, "heat_setpoint_name");
	if(bid_mode == BM_ON){
		fetch(&load_ptr, load_name, parent, &load_prop, "load_name");
		fetch(&total_load_ptr, total_load_name, parent, &total_load_prop, "total_load_name");
		fetch(&cool_demand_ptr, cool_demand_name, parent, &cool_demand_prop, "cool_demand_prop");
		fetch(&heat_demand_ptr, heat_demand_name, parent, &heat_demand_prop, "heat_demand_name");
	}
	
	if(deadband_name[0] != 0){
		deadband_ptr = gl_get_double_by_name(parent, deadband_name);
	}

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

	state_ptr = (enumeration*)gl_get_addr(parent,state_name);

	next_run = 0;

	return 1;
}


int double_controller::isa(char *classname)
{
	return strcmp(classname,"double_controller")==0;
}


TIMESTAMP double_controller::presync(TIMESTAMP t0, TIMESTAMP t1){
	if(thermostat_mode == TM_INVALID)
		thermostat_mode = TM_OFF;

	if(deadband_ptr != 0){
		
		deadband = *deadband_ptr;
	}

	if(next_run == 0){
		cool_setpoint = cool_set_base = *cool_setpoint_ptr;
		heat_setpoint = heat_set_base = *heat_setpoint_ptr;
	}
	if(t0 == next_run){
		// pos/neg is factored into the range
		cool_temp_min = cool_set_base + cool_range_low;
		cool_temp_max = cool_set_base + cool_range_high;
		heat_temp_min = heat_set_base + heat_range_low;
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
		if(*pStd != 0.0){
			cool_bid = *pAvg + (temperature - cool_set_base) * (cool_ramp * *pStd) / fabs(cool_limit - cool_set_base);
		} else {
			cool_bid = *pAvg;
		}
	}
	// heating bid
	if(temperature > heat_temp_max){
		heat_bid = 0.0;
	} else if(temperature < heat_temp_min){
		heat_bid = 9999.0;
	} else {
		heat_ramp = (temperature > heat_set_base ? heat_ramp_high : heat_ramp_low);
		heat_limit = (temperature > heat_set_base ? heat_temp_max : heat_temp_min);
		if(*pStd != 0.0){
			heat_bid = *pAvg + (temperature - heat_set_base) * (heat_ramp * *pStd) / fabs(heat_limit - heat_set_base);
		} else {
			heat_bid = *pAvg;
		}
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
			if(cool_bid > 0.0){ /* "cooling wins" theory */
				heat_bid = 0.0;
				heat_limit = heat_set_base;
			}
			break;
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
	} // end switch

	// if this turns on, it will be...
	if(heat_bid > cool_bid){
		thermostat_mode = TM_HEAT;
	} else if(cool_bid > heat_bid){
		thermostat_mode = TM_COOL;
	} else if(cool_bid == heat_bid){
		thermostat_mode = TM_OFF; // equally demanded, or both == 0.0
	}


	// determine quantity
	// specific to market bids
	if(cool_bid > 0.0 && bid_mode == BM_ON){
		cool_demand = *cool_demand_ptr;
	} else {
		cool_demand = 0.0;
	} 

	if(heat_bid > 0.0 && bid_mode == BM_ON){
		heat_demand = *heat_demand_ptr;
	} else {
		heat_demand = 0.0;
	}


	if(thermostat_mode == TM_HEAT){
		bid_price = heat_bid;
	} else if(thermostat_mode == TM_COOL){
		bid_price = cool_bid;
	} else if(thermostat_mode == TM_OFF){
		bid_price = (heat_bid > cool_bid ? heat_bid : cool_bid);
	}

	if(bid_mode == BM_ON){
		KEY bid_id = (lastbid_id == *pMarketID ? lastbid_id : -1);
		// override
		//bid_id = -1;

		if(thermostat_mode == TM_HEAT){
			bid_quant = heat_demand;
		} else if(thermostat_mode == TM_COOL){
			bid_quant = cool_demand;
		} else if(thermostat_mode == TM_OFF){
			bid_quant = 0.0;
		}

		if (bid_quant!=0)
		{
			if(state_ptr != 0){
				lastbid_id = submit_bid_state(pMarket, hdr, -bid_quant, bid_price, (*state_ptr > 0 ? 1 : 0), bid_id);
			} else {
				lastbid_id = submit_bid(pMarket, hdr, -bid_quant, bid_price, bid_id);
			}
		}
	}

	//return next_run > 0 ? next_run : TS_NEVER;
	return TS_NEVER;
}

TIMESTAMP double_controller::postsync(TIMESTAMP t0, TIMESTAMP t1){
	if(t0 < next_run){
		return next_run;
		//return TS_NEVER;
	}

	double cool_bound, heat_bound;

	next_run = (TIMESTAMP)(*pPeriod) + t1;
	
	// update price & determine auction results
	if(*pMarketID != lastmkt_id){
		lastmkt_id = *pMarketID;
		avg_price = 0.0;
		if(*pAvg == 0.0 || *pStd == 0.0 || cool_set_base == 0.0 || heat_set_base == 0.0){
			//return next_run; // not enough input data
			return TS_NEVER;
		}

		price = *pNextP;
		avg_price = *pAvg;
		stdev_price = *pStd;

		if(*pNextP > *pAvg){ // high prices un-cool and un-heat
			cool_bound = cool_temp_max;
			heat_bound = heat_temp_min;
		} else {
			cool_bound = cool_temp_min;
			heat_bound = heat_temp_max;
		}
		// compare current bid to market results
		if(bid_mode == BM_ON && *pNextP > bid_price){ // only if we care about 'beating the market'
			// market price greater than our bid
			cool_setpoint = cool_temp_max;
			heat_setpoint = heat_temp_min;
		} else {
			if(cool_bid > 0.0 || bid_mode == BM_OFF){
				double ramp = (temperature > cool_set_base ? cool_ramp_high : cool_ramp_low);
				double cool_offset = (*pNextP - *pAvg) * fabs(cool_bound - cool_set_base) / (ramp * *pStd);
				cool_setpoint = cool_set_base + cool_offset;
				if(cool_setpoint > cool_temp_max){
					cool_setpoint = cool_temp_max;
				} else if(cool_setpoint < cool_bound){
					cool_setpoint = cool_bound;
				}
			} else {
				cool_setpoint = cool_temp_max;
			}
			if(heat_bid > 0.0 || bid_mode == BM_OFF){
				double ramp = (temperature > heat_set_base ? heat_ramp_high : heat_ramp_low);
				double heat_offset = (*pNextP - *pAvg) * fabs(heat_bound - heat_set_base) / (ramp * *pStd);
				heat_setpoint = heat_set_base + heat_offset;
				if(heat_setpoint < heat_temp_min){
					heat_setpoint = heat_temp_min;
				} else if(heat_setpoint > heat_bound){
					heat_setpoint = heat_bound;
				}
			} else {
				heat_setpoint = heat_temp_min;
			}
		}
	}

	// verify that the setpoints make sense
	switch(resolve_mode){
		default:
		case RM_STICKY:
			if(last_mode == TM_OFF || last_mode == TM_COOL){
				if(cool_setpoint - heat_setpoint < deadband){
					heat_setpoint = cool_setpoint - deadband;
				}
			} else if(last_mode == TM_HEAT){
				if(cool_setpoint - heat_setpoint < deadband){
					cool_setpoint = heat_setpoint + deadband;
				}
			}
			break;
		case RM_DEADBAND:
			// make sure there's space between the two deadbands
			if(cool_setpoint - heat_setpoint < deadband){
				double mid = (cool_setpoint + heat_setpoint) / 2;
				cool_setpoint = mid + deadband/2;
				heat_setpoint = mid - deadband/2;
			}
			break;
	}

	// enforce base setpoints 
	if(cool_setpoint < heat_set_base){
		heat_setpoint = heat_set_base;
		cool_setpoint = heat_setpoint + deadband;
	} else if(heat_setpoint > cool_set_base){
		cool_setpoint = cool_set_base;
		heat_setpoint = cool_setpoint - deadband;
	}

	// post setpoints
	*cool_setpoint_ptr = cool_setpoint;
	*heat_setpoint_ptr = heat_setpoint;

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
		else
			return 0;
	}
	CREATE_CATCHALL(double_controller);
}

EXPORT int init_double_controller(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL){
			return OBJECTDATA(obj,double_controller)->init(parent);
		}
		else
			return 0;
	}
	INIT_CATCHALL(double_controller);
}

EXPORT int isa_double_controller(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,double_controller)->isa(classname);
	} else {
		return 0;
	}
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
		return t2;
	}
	SYNC_CATCHALL(double_controller);
}

// EOF
