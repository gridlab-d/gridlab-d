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
				PT_KEYWORD, "WATERHEATER", SM_WATERHEATER,
			PT_enumeration, "bid_mode", PADDR(bidmode),
				PT_KEYWORD, "ON", BM_ON,
				PT_KEYWORD, "OFF", BM_OFF,
			PT_double, "ramp_low", PADDR(ramp_low), PT_DESCRIPTION, "the comfort response below the setpoint",
			PT_double, "ramp_high", PADDR(ramp_high), PT_DESCRIPTION, "the comfort response above the setpoint",
			PT_double, "Tmin", PADDR(Tmin), PT_DESCRIPTION, "the setpoint limit on the low side",
			PT_double, "Tmax", PADDR(Tmax), PT_DESCRIPTION, "the setpoint limit on the high side",
			PT_char32, "target", PADDR(target), PT_DESCRIPTION, "the observed property (e.g., air temperature)",
			PT_char32, "setpoint", PADDR(setpoint), PT_DESCRIPTION, "the controlled property (e.g., heating setpoint)",
			PT_char32, "demand", PADDR(demand), PT_DESCRIPTION, "the controlled load when on",
			PT_char32, "load", PADDR(load), PT_DESCRIPTION, "the current controlled load",
			PT_char32, "total", PADDR(total), PT_DESCRIPTION, "the uncontrolled load (if any)",
			PT_object, "market", PADDR(pMarket), PT_DESCRIPTION, "the market to bid into",
			PT_char32, "state", PADDR(state), PT_DESCRIPTION, "the state property of the controlled load",
			PT_char32, "avg_target", PADDR(avg_target),
			PT_char32, "std_target", PADDR(std_target),
			PT_double, "bid_price", PADDR(last_p), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid price",
			PT_double, "bid_quantity", PADDR(last_q), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid quantity",
			PT_double, "set_temp[degF]", PADDR(set_temp), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the reset value",
			PT_double, "base_setpoint[degF]", PADDR(setpoint0),
			// new stuff
			PT_int64, "period", PADDR(period),
			PT_enumeration, "control_mode", PADDR(control_mode),
				PT_KEYWORD, "RAMP", CN_RAMP,
				PT_KEYWORD, "DOUBLE_RAMP", CN_DOUBLE_RAMP,
			PT_enumeration, "resolve_mode", PADDR(resolve_mode),
				PT_KEYWORD, "DEADBAND", RM_DEADBAND,
				PT_KEYWORD, "SLIDING", RM_SLIDING,
			PT_double, "slider_setting_heat", PADDR(slider_setting_heat),
			PT_double, "slider_setting_cool", PADDR(slider_setting_cool),
			PT_double, "range_low[degF]", PADDR(range_low),
			PT_double, "range_high[degF]", PADDR(range_high),
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
			// redefinitions
			PT_char32, "average_target", PADDR(avg_target),
			PT_char32, "standard_deviation_target", PADDR(std_target),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(controller));
	}
}

int controller::create(){
	memset(this, 0, sizeof(controller));
	sprintf(avg_target, "avg24");
	sprintf(std_target, "std24");
	slider_setting_heat = -1.0;
	slider_setting_cool = -1.0;
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
			ramp_low = 2;
			ramp_high = 2;
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
			ramp_low = -2;
			ramp_high = -2;
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
			ramp_low = 2;
			ramp_high = 2;
			Tmin = -3;
			Tmax = 5;
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
			Tmin = 0;
			Tmax = 10;
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
		throw("%s: controller unable to find %s", namestr, name);
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

	gl_set_dependent(hdr, pMarket);
	market = OBJECTDATA(pMarket, auction);

	if(target[0] == 0){
		GL_THROW("controller: %i, target property not specified", hdr->id);
	}
	if(setpoint[0] == 0){
		GL_THROW("controller: %i, setpoint property not specified", hdr->id);;
	}
	if(demand[0] == 0){
		GL_THROW("controller: %i, demand property not specified", hdr->id);
	}
	if(total[0] == 0){
		GL_THROW("controller: %i, total property not specified", hdr->id);
	}
	if(load[0] == 0){
		GL_THROW("controller: %i, load property not specified", hdr->id);
	}

	fetch(&pMonitor, target, parent);
	if(control_mode == CN_RAMP){
		fetch(&pSetpoint, setpoint, parent);
		fetch(&pDemand, demand, parent);
		fetch(&pTotal, total, parent);
		fetch(&pLoad, load, parent);
	} else if(control_mode == CN_DOUBLE_RAMP){
		fetch(&pHeatingSetpoint, heating_setpoint, parent);
		fetch(&pHeatingDemand, heating_demand, parent);
		fetch(&pHeatingTotal, heating_total, parent);
		fetch(&pHeatingLoad, heating_load, parent);
		fetch(&pCoolingSetpoint, cooling_setpoint, parent);
		fetch(&pCoolingDemand, cooling_demand, parent);
		fetch(&pCoolingTotal, cooling_total, parent);
		fetch(&pCoolingLoad, cooling_load, parent);
	}
	fetch(&pAvg, avg_target, pMarket);
	fetch(&pStd, std_target, pMarket);
	fetch(&pDeadband, deadband, parent);
	

	if(dir == 0){
		double high = ramp_high * Tmax;
		double low = ramp_low * Tmin;
		if(high > low){
			dir = 1;
		} else if(high < low){
			dir = -1;
		} else if((high == low) && (fabs(ramp_high) > 0.001 || fabs(ramp_low) > 0.001)){
			dir = 0;
			gl_warning("%s: controller has no price ramp", namestr);
			/* occurs given no price variation, or no control width (use a normal thermostat?) */
		}
		if(ramp_low * ramp_high < 0){
			gl_warning("%s: controller price curve is not injective and may behave strangely");
			/* TROUBLESHOOTING
				The price curve 'changes directions' at the setpoint, which may create odd
				conditions in a number of circumstances.
			 */
		}
	}

	setpoint0 = -1; // key to check first thing

//	double period = market->period;
//	next_run = gl_globalclock + (TIMESTAMP)(period - fmod(gl_globalclock+period,period));
	next_run = gl_globalclock + (market->period - gl_globalclock%market->period);
	
	if(state[0] != 0){
		// grab state pointer
		pState = gl_get_enum_by_name(parent, state);
		if(pState == 0){
			gl_error("state property name \'%s\' is not published by parent class", state);
			return 0;
		}
	}

	if(heating_state[0] != 0){
		// grab state pointer
		pHeatingState = gl_get_enum_by_name(parent, heating_state);
		if(pState == 0){
			gl_error("heating_state property name \'%s\' is not published by parent class", state);
			return 0;
		}
	}

	if(state[0] != 0){
		// grab state pointer
		pCoolingState = gl_get_enum_by_name(parent, cooling_state);
		if(pState == 0){
			gl_error("cooling_state property name \'%s\' is not published by parent class", state);
			return 0;
		}
	}

	if(slider_setting < 0.0){
		gl_warning("slider_setting is negative, reseting to 0.0");
		slider_setting = 0.0;
	}
	if(slider_setting_heat < 0.0){
		gl_warning("slider_setting_heat is negative, reseting to 0.0");
		slider_setting_heat = 0.0;
	}
	if(slider_setting_cool < 0.0){
		gl_warning("slider_setting_cool is negative, reseting to 0.0");
		slider_setting_cool = 0.0;
	}
	if(slider_setting > 1.0){
		gl_warning("slider_setting is greater than 1.0, reseting to 1.0");
		slider_setting = 1.0;
	}
	if(slider_setting_heat > 1.0){
		gl_warning("slider_setting_heat is greater than 1.0, reseting to 1.0");
		slider_setting_heat = 1.0;
	}
	if(slider_setting_cool > 1.0){
		gl_warning("slider_setting_cool is greater than 1.0, reseting to 1.0");
		slider_setting_cool = 1.0;
	}

	return 1;
}


int controller::isa(char *classname)
{
	return strcmp(classname,"controller")==0;
}


TIMESTAMP controller::presync(TIMESTAMP t0, TIMESTAMP t1){
	if(slider_setting < 0.0)
		slider_setting = 0.0;
	if(slider_setting_heat < 0.0)
		slider_setting_heat = 0.0;
	if(slider_setting_cool < 0.0)
		slider_setting_cool = 0.0;
	if(slider_setting > 1.0)
		slider_setting = 1.0;
	if(slider_setting_heat > 1.0)
		slider_setting_heat = 1.0;
	if(slider_setting_cool > 1.0)
		slider_setting_cool = 1.0;

	if(setpoint0 == -1){
		setpoint0 = *pSetpoint;
		heating_setpoint0 = *pHeatingSetpoint;
		cooling_setpoint0 = *pCoolingSetpoint;
	}
	if(t0 == next_run){
		if(control_mode == CN_RAMP){
			min = setpoint0 + Tmin * slider_setting;
			max = setpoint0 + Tmax * slider_setting;
		} else if(control_mode == CN_DOUBLE_RAMP){
			cool_min = cooling_setpoint0 + cool_range_low * slider_setting_cool;
			cool_max = cooling_setpoint0 + cool_range_high * slider_setting_cool;
			heat_min = heating_setpoint0 + heat_range_low * slider_setting_heat;
			heat_max = heating_setpoint0 + heat_range_high * slider_setting_heat;
		}
	}
	return TS_NEVER;
}

TIMESTAMP controller::sync(TIMESTAMP t0, TIMESTAMP t1){
	double bid = -1.0;
	double demand = 0.0;
	double rampify = 0.0;
	OBJECT *hdr = OBJECTHDR(this);

	/* short circuit */
	if(t0 < next_run){
		return TS_NEVER;
	}

	if(control_mode == CN_RAMP){
		// if market has updated, continue onwards
		if(market->market_id != lastmkt_id && (*pAvg == 0.0 || *pStd == 0.0 || setpoint0 == 0.0)){
			double clear_price;
			lastmkt_id = market->market_id;
			// update using last price
			// T_set,a = T_set + (P_clear - P_avg) * | T_lim - T_set | / (k_T * stdev24)

			clear_price = market->current_frame.clearing_price;
			if(clear_price > last_p){ // if we lost the auction
				/* failed to win auction */
				may_run = 0;
				if(dir > 0){
					set_temp = max;
				} else {
					set_temp = min;
				}
			} else {
				//set_temp = (market->next.price - *pAvg) * fabs(T_lim) / (k_T * *pStd);
				if(*pStd == 0){
					set_temp = setpoint0;
				} else if(clear_price < *pAvg && range_low != 0){
					set_temp = (clear_price - *pAvg) * fabs(ramp_low) / ((dir ? range_low : range_high) * *pStd);
				} else if(clear_price > *pAvg && range_high != 0){
					set_temp = (clear_price - *pAvg) * fabs(ramp_high) / ((dir ? range_high : range_low) * *pStd);
				} else {
					set_temp = setpoint0;
				}
				
				may_run = 1;
			}

			// clip
			if(set_temp > max){
				set_temp = max;
			} else if(set_temp < min){
				set_temp = min;
			}

			*pSetpoint = setpoint0 + set_temp;
			//gl_verbose("controller::postsync(): temp %f given p %f vs avg %f",set_temp, market->next.price, market->avg24);	
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
				bid = *pAvg; // override due to lack of "real" curve
			}
		}

		// calculate bid price
		if(*pMonitor > setpoint0){
			k_T = ramp_high;
			T_lim = max;
		} else {
			k_T = ramp_low;
			T_lim = min;
		}

		if(bid < 0.0)
			bid = *pAvg + (*pMonitor - setpoint0) * (k_T * *pStd) / fabs(T_lim);

		// bid the response part of the load
		double residual = *pTotal;
		/* WARNING ~ bid ID check will not work properly */
		int64 bid_id = (KEY)(lastmkt_id == market->market_id ? lastbid_id : -1);
		// override
		//bid_id = -1;
		if(bid > 0.0 && *pDemand > 0){
			last_p = bid;
			last_q = *pDemand;
			//lastbid_id = market->submit(OBJECTHDR(this), -last_q, last_p, bid_id, (BIDDERSTATE)(pState != 0 ? *pState : 0));
			if(pState != 0){
				lastbid_id = submit_bid_state(pMarket, hdr, -last_q, last_p, (*pState > 0 ? 1 : 0), bid_id);
			} else {
				lastbid_id = submit_bid(pMarket, hdr, -last_q, last_p, bid_id);
			}
			residual -= *pLoad;
		} else {
			last_p = 0;
			last_q = 0;
		}
		if (residual>0){
			/* WARNING ~ bid ID check will not work properly */
			//lastbid_id = market->submit(OBJECTHDR(this), -residual, 9999, bid_id, (BIDDERSTATE)(pState != 0 ? *pState : 0));
			if(pState != 0){
				lastbid_id = submit_bid_state(pMarket, hdr, -last_q, 9999.0, (*pState > 0 ? 1 : 0), bid_id);
			} else {
				lastbid_id = submit_bid(pMarket, hdr, -last_q, 9999.0, bid_id);
			}
		}
		else if(residual < 0)
			gl_warning("controller:%d: residual unresponsive load is negative! (%.1f kW)", hdr->id, residual);
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

		// if the market has updated, 
		if(lastmkt_id < market->market_id){
			// retrieve cleared price
			double clear_price;
			clear_price = market->current_frame.clearing_price;
			if(clear_price == last_p){
				// determine what to do at the marginal price
				switch(market->clearing_type){
					case CT_SELLER:	// may need to curtail
						break;
					case CT_PRICE:	// should not occur
					case CT_NULL:	// q zero or logic error ~ should not occur
						gl_warning("clearing price and bid price are equal with a market clearing type that involves inequal prices");
						break;
					default:
						break;
				}
			} else if(clear_price > last_p){
				// cheapskates-R-us, did not win power
				may_run = 0;
				*pHeatingSetpoint = heat_min;
				*pCoolingSetpoint = cool_max;
				*pHeatingState = 0;
				*pCoolingState = 0;
			} else { // clear_price < last_p, we got a deal!
				may_run = 1;
				if(last_mode == TM_OFF || last_mode == TM_COOL){
					//set_temp = (clear_price - *pAvg) * fabs(ramp_high) / (range_high * *pStd);
					*pHeatingSetpoint = heat_min;
					*pHeatingState = 0;
					*pCoolingState = 1;
					if(*pStd == 0.0){
						*pCoolingSetpoint = set_temp = cooling_setpoint0;
					} else {
						if(clear_price > *pAvg){
							*pCoolingSetpoint = set_temp = cooling_setpoint0 + (clear_price - *pAvg) * fabs(cool_ramp_high) / (cool_range_high * *pStd);
						} else if(clear_price < *pAvg){
							*pCoolingSetpoint = set_temp = cooling_setpoint0 + (clear_price - *pAvg) * fabs(cool_ramp_low) / (cool_ramp_low * *pStd);
						} else {
							*pCoolingSetpoint = set_temp = cooling_setpoint0;
						}
					}
				} else if(last_mode == TM_HEAT){
					//set_temp = (clear_price - *pAvg) * fabs(ramp_high) / (range_high * *pStd);
					*pCoolingSetpoint = cool_max;
					*pCoolingState = 0;
					*pHeatingState = 1;
					if(*pStd == 0.0){
						*pHeatingSetpoint = set_temp = heating_setpoint0;
					} else {
						if(clear_price > *pAvg){
							*pHeatingSetpoint = set_temp = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_ramp_low) / (heat_range_high * *pStd);
						} else if(clear_price < *pAvg){
							*pHeatingSetpoint = set_temp = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_ramp_high) / (heat_range_low * *pStd);
						} else {
							*pHeatingSetpoint = set_temp = heating_setpoint0;
						}
					}
				}
			}
			// calculate setpoints
			lastmkt_id = market->market_id;
		}
		// find crossover
		double midpoint = 0.0;
		if(heat_max - cool_min > *pDeadband){
			switch(resolve_mode){
				case RM_DEADBAND:
					midpoint = (heat_max + cool_min) / 2;
					heat_max = midpoint - *pDeadband/2;
					cool_min = midpoint + *pDeadband/2;
					break;
				case RM_SLIDING:
					if(last_mode == TM_OFF || last_mode == TM_COOL){
						heat_max = cool_min - *pDeadband;
					} else if (last_mode == TM_HEAT){
						cool_min = heat_max + *pDeadband;
					}
					break;
				default:
					gl_error("unrecognized resolve_mode when double_ramp overlap resolution is needed");
					break;
			}
		}
		// submit bids
		last_p = 0.0;
		last_q = 0.0;
		if(*pMonitor > cool_max){
			last_p = market->pricecap;
			last_q = *pCoolingDemand;
			thermostat_mode = TM_COOL;
		} else if(*pMonitor < heat_min){
			last_p = market->pricecap;
			last_q = *pHeatingDemand;
			thermostat_mode = TM_HEAT;
		} else if(*pMonitor > heat_max && *pMonitor < cool_min){
			last_p = 0.0;
			last_q = 0.0;
			thermostat_mode = TM_OFF;
		} else if(*pMonitor < heat_max && *pMonitor > heat_min){
			double ramp, range;
			ramp = (*pMonitor > *pHeatingSetpoint ? heat_ramp_high : heat_ramp_low);
			range = (*pMonitor > *pHeatingSetpoint ? heat_range_high : heat_range_low);
			last_p = *pAvg + (*pMonitor - *pHeatingSetpoint) * ramp * (*pStd) / fabs(range);
			last_q = *pHeatingDemand;
			thermostat_mode = TM_HEAT;
		} else if(*pMonitor < cool_max && *pMonitor > cool_min){
			double ramp, range;
			ramp = (*pMonitor > *pCoolingSetpoint ? cool_ramp_high : cool_ramp_low);
			range = (*pMonitor > *pCoolingSetpoint ? cool_range_high : cool_range_low);
			last_p = *pAvg + (*pMonitor - *pCoolingSetpoint) * ramp * (*pStd) / fabs(range);
			last_q = *pCoolingDemand;
			thermostat_mode = TM_COOL;
		}
		if(last_q > 0){
			/* WARNING ~ bid ID check will not work properly */
			int64 bid = (KEY)(lastmkt_id == market->market_id ? lastbid_id : -1);
			lastbid_id = submit_bid(this->pMarket, OBJECTHDR(this), last_q, last_p, bid);
		}
	}
	char timebuf[128];
	gl_printtime(t1,timebuf,127);
	//gl_verbose("controller:%i::sync(): bid $%f for %f kW at %s",hdr->id,last_p,last_q,timebuf);
	//return postsync(t0, t1);
	return TS_NEVER;
}

TIMESTAMP controller::postsync(TIMESTAMP t0, TIMESTAMP t1){

	if(t0 < next_run){
		return TS_NEVER;
	}

	next_run += (TIMESTAMP)(market->period);

	
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
