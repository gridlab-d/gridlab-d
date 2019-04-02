/** $Id: controller_ccsi.cpp 1182 2009-09-09 22:08:36Z mhauer $
	Copyright (C) 2009 Battelle Memorial Institute
	@file auction_ccsi.cpp
	@defgroup controller_ccsi Transactive controller_ccsi, OlyPen experiment style
	@ingroup market

 **/

#include "controller_ccsi.h"

CLASS* controller_ccsi::oclass = NULL;
controller_ccsi *controller_ccsi::defaults = NULL;

controller_ccsi::controller_ccsi(MODULE *module){
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,const_cast<char*>("controller_ccsi"),sizeof(controller_ccsi),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class controller_ccsi";
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
				PT_KEYWORD, "DOUBLE_PRICE", (enumeration)CN_DOUBLE_PRICE,
			PT_enumeration, "resolve_mode", PADDR(resolve_mode),
				PT_KEYWORD, "DEADBAND", (enumeration)RM_DEADBAND,
				PT_KEYWORD, "SLIDING", (enumeration)RM_SLIDING,
			PT_double, "slider_setting",PADDR(slider_setting),
			PT_double, "slider_setting_heat", PADDR(slider_setting_heat),
			PT_double, "slider_setting_cool", PADDR(slider_setting_cool),
			PT_char32, "override", PADDR(re_override),
			PT_double, "parent_unresponsive_load", PADDR(parent_unresponsive_load), PT_DESCRIPTION, "the uncontrolled load of parent.",
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
			PT_double, "BidPrice", PADDR(Pbid),
			PT_double, "AvgTemperature", PADDR(Temp12),			
			// PROXY PROPERTIES
			PT_double, "proxy_average", PADDR(proxy_avg),
			PT_double, "proxy_standard_deviation", PADDR(proxy_std),
			PT_int64, "proxy_market_id", PADDR(proxy_mkt_id),
			PT_double, "proxy_clear_price[$]", PADDR(proxy_clear_price),
			PT_double, "proxy_price_cap", PADDR(proxy_price_cap),
			PT_char32, "proxy_market_unit", PADDR(proxy_mkt_unit),
			PT_double, "proxy_initial_price", PADDR(proxy_init_price),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(this,0,sizeof(controller_ccsi));
	}
}

int controller_ccsi::create(){
	memcpy(this, defaults, sizeof(controller_ccsi));
	sprintf(avg_target.get_string(), "avg24");
	sprintf(std_target.get_string(), "std24");
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
	use_predictive_bidding = FALSE;
	Qh_count = 0.0;
	Qh_average = 0.0;
	controller_bid.bid_id = -1;
	controller_bid.market_id = -1;
	controller_bid.price = 0;
	controller_bid.quantity = 0;
	controller_bid.state = BS_UNKNOWN;
	controller_bid.rebid = false;
	controller_bid.bid_accepted = true;
	bid_id = -1;
	return 1;
}

/** provides some easy default inputs for the transactive controller_ccsi,
	 and some examples of what various configurations would look like.
 **/
void controller_ccsi::cheat(){
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
			cool_range_low = 5;
			cool_range_high = 5;
			break;
		default:
			break;
	}
}


/** convenience shorthand
 **/
void controller_ccsi::fetch_double(double **prop, const char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_double_by_name(parent, const_cast<char*>(name));
	if(*prop == nullptr){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "controller_ccsi:%i", hdr->id);
		if(*name == static_cast<char>(0))
			sprintf(msg, "%s: controller_ccsi unable to find property: name is NULL", namestr);
		else
			sprintf(msg, "%s: controller_ccsi unable to find %s", namestr, name);
		throw(msg);
	}
}

void controller_ccsi::fetch_int64(int64 **prop, const char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_int64_by_name(parent, const_cast<char*>(name));
	if(*prop == nullptr){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "controller_ccsi:%i", hdr->id);
		if(*name == static_cast<char>(0))
			sprintf(msg, "%s: controller_ccsi unable to find property: name is NULL", namestr);
		else
			sprintf(msg, "%s: controller_ccsi unable to find %s", namestr, name);
		throw(std::runtime_error(msg));
	}
}

void controller_ccsi::fetch_enum(enumeration **prop, const char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_enum_by_name(parent, const_cast<char*>(name));
	if(*prop == nullptr){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "controller:%i", hdr->id);
		if(*name == static_cast<char>(0)){
			sprintf(msg, "%s: controller unable to find property: name is NULL", namestr);
		}
		else
		{
			sprintf(msg, "%s: controller unable to find %s", namestr, name);
		}
		throw(std::runtime_error(msg));
	}
}

double controller_ccsi::calcTemp1(double Tair, double Deadband, enumeration State){
	double halfband = Deadband * 0.5;
	double temp1 = 0.0;

	if(State != 0){
		temp1 = Tair + halfband;
	} else {
		temp1 = Tair - halfband;
	}

	return temp1;
}

double controller_ccsi::calcTemp2(double Ua, double Hm, double Ca, double Cm, double MassInternalGainFraction, double MassSolarGainFraction, double Qi, double Qs, double Qh, double Tout, double Tair, double Tmass, double Deadband, enumeration State){
	double Qh_estimate = 0.0;
	if (Qh < 0.0){
		Qh_estimate = Qh;
		if (Qh_count > 0.0){
			Qh_average = (Qh_average*Qh_count + Qh)/(Qh_count + 1.0);
			Qh_count = Qh_count + 1.0;
		} else {
			Qh_average = Qh;
			Qh_count = 1.0;
		}
	} else {
		Qh_estimate = Qh_average;
	}
	double Qa_OFF = ((1 - MassInternalGainFraction)*Qi) + ((1 - MassSolarGainFraction)*Qs);
	double Qa_ON = Qh + ((1 - MassInternalGainFraction)*Qi) + ((1 - MassSolarGainFraction)*Qs);
	double Qm = (MassInternalGainFraction*Qi) + (MassSolarGainFraction*Qs);
	double A_ETP[2][2];
	A_ETP[0][0] = A_ETP[0][1] = A_ETP[1][0] = A_ETP[1][1] = 0.0;
	double B_ETP_ON[2];
	B_ETP_ON[0] = B_ETP_ON[1] = 0.0;
	double B_ETP_OFF[2];
	B_ETP_ON[0] = B_ETP_ON[1] = 0.0;
	double x[2];
	x[0] = Tair;
	x[1] = Tmass;
	double L[2];
	L[0] = 1.0;
	L[1] = 0.0;
//	double T = dPeriod/3600.0;
	double T = (bid_delay+dPeriod)/3600.0;
	double AEI[2][2];//inv(A_ETP)
	AEI[0][0] = AEI[0][1] = AEI[1][0] = AEI[1][1] = 0.0;
	double LAEI[2];//L*inv(A_ETP)
	LAEI[0] = LAEI[1] = 0.0;
	double AET[2][2];//A_ETP*T
	AET[0][0] = AET[0][1] = AET[1][0] = AET[1][1] = 0.0;
	double eAET[2][2];//expm(A_ETP*T)
	eAET[0][0] = eAET[0][1] = eAET[1][0] = eAET[1][1] = 0.0;
	double LT[2];//L*inv(A_ETP)*expm(A_ETP*T)
	LT[0] = LT[1] = 0.0;
	double AEx[2];//A_ETP*x
	AEx[0] = AEx[1] = 0.0;
	double AxB[2];//A_ETP*x + B_ETP_ON/OFF
	AxB[0] = AxB[1] = 0.0;
	double LAxB = 0.0;//L*inv(A_ETP)*expm(A_ETP*T)*(A_ETP*x + B_ETP_ON/OFF)
	double LAIB = 0.0;//L*inv(A_ETP)*B_ETP_ON/OFF
	double temp2 = 0.0;//L*inv(A_ETP)*expm(A_ETP*T)*(A_ETP*x + B_ETP_ON/OFF) - L*inv(A_ETP)*B_ETP_ON/OFF +/- halfband
	//Calculate A_ETP, B_ETP_ON, and B_ETP_OFF
	if (Ca != 0.0){
		A_ETP[0][0] = -1.0*(Ua + Hm)/Ca;
		A_ETP[0][1] = Hm/Ca;
		B_ETP_ON[0] = (Ua*Tout/Ca) + (Qa_ON/Ca);
		B_ETP_OFF[0] = (Ua*Tout/Ca) + (Qa_OFF/Ca);
	}
	if (Cm != 0.0){
		A_ETP[1][0] = Hm/Cm;
		A_ETP[1][1] = -1.0*Hm/Cm;
		B_ETP_ON[1] = Qm/Cm;
		B_ETP_OFF[1] = Qm/Cm;
	}
	//Calculate inverse of A_ETP
	double detA = 0.0;
	if(((A_ETP[0][0]*A_ETP[1][1]) - (A_ETP[0][1]*A_ETP[1][0])) != 0.0){
		detA = ((A_ETP[0][0]*A_ETP[1][1]) - (A_ETP[0][1]*A_ETP[1][0]));
		AEI[0][0] = A_ETP[1][1]/detA;
		AEI[0][1] = -1*A_ETP[0][1]/detA;
		AEI[1][1] = A_ETP[0][0]/detA;
		AEI[1][0] = -1*A_ETP[1][0]/detA;
	} else {
		if(State == 0){
			return Tair - (Deadband/2.0);
		} else {
			return Tair + (Deadband/2.0);
		}
	}
	//Calculate exp(A_ETP*T)
	AET[0][0] = A_ETP[0][0]*T;
	AET[0][1] = A_ETP[0][1]*T;
	AET[1][0] = A_ETP[1][0]*T;
	AET[1][1] = A_ETP[1][1]*T;
	if(AET[0][1] == 0.0 && AET[1][0] == 0.0){//diagonal matrix
		eAET[0][0] = exp(AET[0][0]);
		eAET[0][1] = eAET[1][0] = 0.0;
		eAET[1][1] = exp(AET[1][1]);
	} else if(AET[1][0] == 0.0){//upper triangular matrix
		if(fabs(AET[0][0] - AET[1][1]) <= 1e-37){//nilpotent
			eAET[0][0] = exp(AET[0][0]);
			eAET[0][1] = exp(AET[0][0])*AET[0][1];
			eAET[1][0] = 0.0;
			eAET[1][1] = exp(AET[0][0]);
		} else {
			eAET[0][0] = exp(AET[0][0]);
			eAET[0][1] = (AET[0][1]*(exp(AET[0][0]) - exp(AET[1][1])))/(AET[0][0] - AET[1][1]);
			eAET[1][0] = 0.0;
			eAET[1][1] = exp(AET[1][1]);
		}
	} else {
		double discr = (AET[0][0] - AET[1][1])*(AET[0][0] - AET[1][1]) + (4.0*AET[0][1]*AET[1][0]);
		double pre = exp((AET[0][0] + AET[1][1])/2.0);
		double g = 0.0;
		if(fabs(discr) <= 1e-37){
			eAET[0][0] = pre*(1.0 + ((AET[0][0] - AET[1][1])/2.0));
			eAET[0][1] = pre*AET[0][1];
			eAET[1][0] = pre*AET[1][0];
			eAET[1][1] = pre*(1.0 - ((AET[0][0] - AET[1][1])/2.0));
		} else if(discr > 1e-37){
			g = 0.5*sqrt(discr);
			eAET[0][0] = pre*(cosh(g) + ((AET[0][0] - AET[1][1])*sinh(g)/(2.0*g)));
			eAET[0][1] = pre*AET[0][1]*sinh(g)/g;
			eAET[1][0] = pre*AET[1][0]*sinh(g)/g;
			eAET[1][1] = pre*(cosh(g) - ((AET[0][0] - AET[1][1])*sinh(g)/(2.0*g)));
		} else {
			g = 0.5*sqrt(fabs(discr));
			eAET[0][0] = pre*(cos(g) + ((AET[0][0] - AET[1][1])*sin(g)/(2.0*g)));
			eAET[0][1] = pre*AET[0][1]*sin(g)/g;
			eAET[1][0] = pre*AET[1][0]*sin(g)/g;
			eAET[1][1] = pre*(cos(g) - ((AET[0][0] - AET[1][1])*sin(g)/(2.0*g)));
		}
	}
	//Calculate L*inv(A_ETP)
	LAEI[0] = (L[0]*AEI[0][0]) + (L[1]*AEI[1][0]);
	LAEI[1] = (L[0]*AEI[0][1]) + (L[1]*AEI[1][1]);
	//Calculate L*inv(A_ETP)expm(A_ETP*T)
	LT[0] = (LAEI[0]*eAET[0][0]) + (LAEI[1]*eAET[1][0]);
	LT[1] = (LAEI[0]*eAET[0][1]) + (LAEI[1]*eAET[1][1]);
	//Calculate A_ETP*x
	AEx[0] = (A_ETP[0][0]*x[0]) + (A_ETP[0][1]*x[1]);
	AEx[1] = (A_ETP[1][0]*x[0]) + (A_ETP[1][1]*x[1]);
	//Calculate A_ETP*x + B_ETP_ON/OFF
	if(State == 0){
		AxB[0] = AEx[0] + B_ETP_OFF[0];
		AxB[1] = AEx[1] + B_ETP_OFF[1];
	} else {
		AxB[0] = AEx[0] + B_ETP_ON[0];
		AxB[1] = AEx[1] + B_ETP_ON[1];
	}
	//Calculate L*inv(A_ETP)expm(A_ETP*T)(A_ETP*x + B_ETP_ON/OFF)
	LAxB = (LT[0]*AxB[0]) + (LT[1]*AxB[1]);
	//Calculate L*inv(A_ETP)*B_ETP_ON/OFF
	if(State == 0){
		LAIB = (LAEI[0]*B_ETP_OFF[0]) + (LAEI[1]*B_ETP_OFF[1]);
	} else {
		LAIB = (LAEI[0]*B_ETP_ON[0]) + (LAEI[1]*B_ETP_ON[1]);
	}
	//Calculate L*inv(A_ETP)expm(A_ETP*T)(A_ETP*x + B_ETP_ON/OFF) - L*inv(A_ETP)*B_ETP_ON/OFF +/- halfband
	if(State == 0){
		temp2 = LAxB - LAIB - (Deadband/2.0);
	} else {
		temp2 = LAxB - LAIB + (Deadband/2.0);
	}
	return temp2;
}
/** initialization process
 **/
int controller_ccsi::init(OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	char tname[32];
	char *namestr = (hdr->name ? hdr->name : tname);
	double *pInitPrice = NULL;
	bid_offset = 1e-9;
	sprintf(tname, "controller_ccsi:%i", hdr->id);

	cheat();

	if(parent == NULL){
		gl_error("%s: controller_ccsi has no parent, therefore nothing to control", namestr);
		return 0;
	}
	if(bidmode != BM_PROXY){
		pMarket = gl_get_object((char *)(&pMkt));
		if(pMarket == NULL){
			gl_error("%s: controller_ccsi has no market, therefore no price signals", namestr);
			return 0;
		}

		if((pMarket->flags & OF_INIT) != OF_INIT){
			gl_verbose("controller_ccsi::init(): deferring initialization on %s", pMkt);
			return 2; // defer
		}

		if(gl_object_isa(pMarket, "auction_ccsi")){
			gl_set_dependent(hdr, pMarket);
		}

		if(dPeriod == 0.0){
			double *pPeriod = NULL;
			fetch_double(&pPeriod, "period", pMarket);
			period = *pPeriod;
		} else {
			period = (TIMESTAMP)floor(dPeriod + 0.5);
		}

		fetch_double(&pAvg, (char *)(&avg_target), pMarket);
		fetch_double(&pStd, (char *)(&std_target), pMarket);
		fetch_int64(&pMarketId, "market_id", pMarket);
		fetch_double(&pClearedPrice, "current_market.clearing_price", pMarket);
		fetch_double(&pPriceCap, "price_cap", pMarket);
		fetch_enum(&pMarginMode, "margin_mode", pMarket);
		fetch_double(&pMarginalFraction, "current_market.marginal_quantity_frac", pMarket);
		gld_property marketunit(pMarket, "unit");
		gld_string mku;
		mku = marketunit.get_string();
		strncpy(market_unit, mku.get_buffer(), 31);
		submit = (FUNCTIONADDR)(gl_get_function(pMarket, "submit_bid_state_ccsi"));
		if(submit == NULL){
			gl_error("Unable to find function, submit_bid_state_ccsi(), for object %s.", pMkt);
			return 0;
		}
		fetch_double(&pInitPrice, "init_price", pMarket);
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
		pAvg = &(this->proxy_avg);
		pStd = &(this->proxy_std);
		pMarketId = &(this->proxy_mkt_id);
		pClearedPrice = &(this->proxy_clear_price);
		pPriceCap = &(this->proxy_price_cap);
		strncpy(market_unit, proxy_mkt_unit, 31);
		submit = (FUNCTIONADDR)(gl_get_function(pMarket, "submit_bid_state_ccsi"));
		if(submit == NULL){
			gl_error("Unable to find function, submit_bid_state_ccsi(), for object %s.", pMkt);
			return 0;
		}
		pInitPrice = &(this->proxy_init_price);
	}

	if(bid_delay < 0){
		bid_delay = -bid_delay;
	}
	if(bid_delay > period){
		gl_warning("Bid delay is greater than the controller_ccsi period. Resetting bid delay to 0.");
		bid_delay = 0;
	}

	if(target[0] == 0){
		GL_THROW("controller_ccsi: %i, target property not specified", hdr->id);
	}
	if(setpoint[0] == 0 && (control_mode == CN_RAMP || control_mode == CN_DOUBLE_PRICE)){
		GL_THROW("controller_ccsi: %i, setpoint property not specified", hdr->id);;
	}
	if(demand[0] == 0 && (control_mode == CN_RAMP || control_mode == CN_DOUBLE_PRICE)){
		GL_THROW("controller_ccsi: %i, demand property not specified", hdr->id);
	}
	if(deadband[0] == 0 && ((use_predictive_bidding == TRUE && control_mode == CN_RAMP) || control_mode == CN_DOUBLE_PRICE)){
		GL_THROW("controller_ccsi: %i, deadband property not specified", hdr->id);
	}
	if(total[0] == 0){
		GL_THROW("controller_ccsi: %i, total property not specified", hdr->id);
	}
	if(load[0] == 0){
		GL_THROW("controller_ccsi: %i, load property not specified", hdr->id);
	}

	if(heating_setpoint[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller_ccsi: %i, heating_setpoint property not specified", hdr->id);;
	}
	if(heating_demand[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller_ccsi: %i, heating_demand property not specified", hdr->id);
	}

	if(cooling_setpoint[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller_ccsi: %i, cooling_setpoint property not specified", hdr->id);;
	}
	if(cooling_demand[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller_ccsi: %i, cooling_demand property not specified", hdr->id);
	}

	if(deadband[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller_ccsi: %i, deadband property not specified", hdr->id);
	}

	fetch_double(&pMonitor, target, parent);
	if(control_mode == CN_RAMP){
		fetch_double(&pSetpoint, setpoint, parent);
		fetch_double(&pDemand, demand, parent);
		fetch_double(&pTotal, total, parent);
		fetch_double(&pLoad, load, parent);
		if(use_predictive_bidding == TRUE){
			fetch_double(&pDeadband, (char *)(&deadband), parent);
		}
	} else if(control_mode == CN_DOUBLE_RAMP){
		sprintf(aux_state, "is_AUX_on");
		sprintf(heat_state, "is_HEAT_on");
		sprintf(cool_state, "is_COOL_on");
		fetch_double(&pHeatingSetpoint, (char *)(&heating_setpoint), parent);
		fetch_double(&pHeatingDemand, (char *)(&heating_demand), parent);
		fetch_double(&pHeatingTotal, total, parent);
		fetch_double(&pHeatingLoad, total, parent);
		fetch_double(&pCoolingSetpoint, (char *)(&cooling_setpoint), parent);
		fetch_double(&pCoolingDemand, (char *)(&cooling_demand), parent);
		fetch_double(&pCoolingTotal, total, parent);
		fetch_double(&pCoolingLoad, load, parent);
		fetch_double(&pDeadband, (char *)(&deadband), parent);
		fetch_double(&pAuxState, (char *)(&aux_state), parent);
		fetch_double(&pHeatState, (char *)(&heat_state), parent);
		fetch_double(&pCoolState, (char *)(&cool_state), parent);
	} else if(control_mode == CN_DOUBLE_PRICE){
		fetch_double(&pSetpoint, setpoint, parent);
		fetch_double(&pDemand, demand, parent);
		fetch_double(&pTotal, total, parent);
		fetch_double(&pLoad, load, parent);
		fetch_double(&pDeadband, deadband.get_string(), parent);
		fetch_double(&pUa, "UA", parent);
		fetch_double(&pHm, "mass_heat_coeff", parent);
		fetch_double(&pCa, "air_heat_capacity", parent);
		fetch_double(&pCm, "mass_heat_capacity", parent);
		fetch_double(&pMassInternalGainFraction, "mass_internal_gain_fraction", parent);
		fetch_double(&pMassSolarGainFraction, "mass_solar_gain_fraction", parent);
		fetch_double(&pQi, "Qi", parent);
		fetch_double(&pQs, "solar_gain", parent);
		fetch_double(&pQh, "heat_cool_gain", parent);
		fetch_double(&pTout, "outdoor_temperature", parent);
		fetch_double(&pTmass, "mass_temperature", parent);
		fetch_double(&pRated_cooling_capacity, "design_cooling_capacity", parent);
		fetch_double(&pCooling_COP, "cooling_COP", parent);
	}
	if(bid_id == -1){
		controller_bid.bid_id = (int64)hdr->id;
		bid_id = (int64)hdr->id;
	} else {
		controller_bid.bid_id = bid_id;
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
			gl_warning("%s: controller_ccsi has no price ramp", namestr);
			/* occurs given no price variation, or no control width (use a normal thermostat?) */
		}
		if(ramp_low * ramp_high < 0){
			gl_warning("%s: controller_ccsi price curve is not injective and may behave strangely");
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
	init_time = gl_globalclock;
	time_off = TS_NEVER;
	if(sliding_time_delay < 0 )
		dtime_delay = 21600; // default sliding_time_delay of 6 hours
	else
		dtime_delay = (int64)sliding_time_delay;

	if(state[0] != 0){
		// grab state pointer
		pState = gl_get_enum_by_name(parent, state);
		last_pState = 0;
		if(pState == 0){
			gl_error("state property name \'%s\' is not published by parent class", state);
			return 0;
		}
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
	if(re_override[0] != 0){
		pOverride = gl_get_enum_by_name(parent, re_override);
	}
	if((pOverride == 0) && (use_override == OU_ON)){
		gl_error("use_override is ON but no valid override property name is given");
		return 0;
	}

	if(control_mode == CN_RAMP || control_mode == CN_DOUBLE_PRICE){
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
	last_p = *pInitPrice;
	lastmkt_id = 0;
	return 1;
}


int controller_ccsi::isa(char *classname)
{
	return strcmp(classname,"controller_ccsi")==0;
}


TIMESTAMP controller_ccsi::presync(TIMESTAMP t0, TIMESTAMP t1){
	double deadband_shift = 0.0;
	double shift_direction = 0.0;
	OBJECT *hdr = OBJECTHDR(this);

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

	if((control_mode == CN_RAMP || control_mode == CN_DOUBLE_PRICE) && setpoint0 == -1)
		setpoint0 = *pSetpoint;
	if(control_mode == CN_DOUBLE_RAMP && heating_setpoint0 == -1)
		heating_setpoint0 = *pHeatingSetpoint;
	if(control_mode == CN_DOUBLE_RAMP && cooling_setpoint0 == -1)
		cooling_setpoint0 = *pCoolingSetpoint;


	if(control_mode == CN_RAMP || control_mode == CN_DOUBLE_PRICE){
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
	} else if(control_mode == CN_DOUBLE_RAMP){
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

	if(control_mode == CN_DOUBLE_PRICE){ 
		// if market has updated, continue onwards
		if(*pMarketId != lastmkt_id){// && (*pAvg == 0.0 || *pStd == 0.0 || setpoint0 == 0.0)){

			lastmkt_id = *pMarketId;
			lastbid_id = -1; // clear last bid id, refers to an old market
			// update using last price
			// T_set,a = T_set + (P_clear - P_avg) * | T_lim - T_set | / (k_T * stdev24)

			clear_price = *pClearedPrice;
			controller_bid.rebid = false;
			if(fabs(*pStd) < bid_offset){
				set_temp = setpoint0;
			} else if(clear_price < *pAvg && range_low != 0){
				set_temp = setpoint0 + (clear_price - *pAvg) * fabs(range_low) / (ramp_low * *pStd);
			} else if(clear_price > *pAvg && range_high != 0){
				set_temp = setpoint0 + (clear_price - *pAvg) * fabs(range_high) / (ramp_high * *pStd); 
			} else {
				set_temp = setpoint0; 
			}

			if((use_override == OU_ON) && (pOverride != 0)){
				if(clear_price <= last_p){
					// if we're willing to pay as much as, or for more than the offered price, then run.
					*pOverride = 1;
				} else {
					*pOverride = -1;
				}
			}

			// clip
			if(set_temp > max){
				set_temp = max;
			} else if(set_temp < min){
				set_temp = min;
			}

			*pSetpoint = set_temp;
			//gl_verbose("controller_ccsi::postsync(): temp %f given p %f vs avg %f",set_temp, market->next.price, market->avg24);
		}
	}

	if(control_mode == CN_RAMP){ 
		// if market has updated, continue onwards
		if(*pMarketId != lastmkt_id){// && (*pAvg == 0.0 || *pStd == 0.0 || setpoint0 == 0.0)){
			lastmkt_id = *pMarketId;
			lastbid_id = -1; // clear last bid id, refers to an old market
			// update using last price
			// T_set,a = T_set + (P_clear - P_avg) * | T_lim - T_set | / (k_T * stdev24)
			
			clear_price = *pClearedPrice;
			controller_bid.rebid = false;
			if(use_predictive_bidding == TRUE){
				if((dir > 0 && clear_price < last_p) || (dir < 0 && clear_price > last_p)){
					shift_direction = -1;
				} else if((dir > 0 && clear_price >= last_p) || (dir < 0 && clear_price <= last_p)){
					shift_direction = 1;
				} else {
					shift_direction = 0;
				}
				deadband_shift = *pDeadband * 0.5;
			}
			if(fabs(*pStd) < bid_offset){
				set_temp = setpoint0;
			} else if(clear_price < *pAvg && range_low != 0){
				set_temp = setpoint0 + (clear_price - *pAvg) * fabs(range_low) / (ramp_low * *pStd) + deadband_shift*shift_direction;
			} else if(clear_price > *pAvg && range_high != 0){
				set_temp = setpoint0 + (clear_price - *pAvg) * fabs(range_high) / (ramp_high * *pStd) + deadband_shift*shift_direction;
			} else {
				set_temp = setpoint0 + deadband_shift*shift_direction;
			}

			if((use_override == OU_ON) && (pOverride != 0)){
				if(clear_price <= last_p){
					// if we're willing to pay as much as, or for more than the offered price, then run.
					*pOverride = 1;
				} else {
					*pOverride = -1;
				}
			}

			// clip
			if(set_temp > max){
				set_temp = max;
			} else if(set_temp < min){
				set_temp = min;
			}

			*pSetpoint = set_temp;
			//gl_verbose("controller_ccsi::postsync(): temp %f given p %f vs avg %f",set_temp, market->next.price, market->avg24);
		}
	}



	return TS_NEVER;
}

TIMESTAMP controller_ccsi::sync(TIMESTAMP t0, TIMESTAMP t1){
	double bid = -1.0;
	int64 no_bid = 0; // flag gets set when the current temperature drops in between the the heating setpoint and cooling setpoint curves
	double demand = 0.0;
	double rampify = 0.0;
	double deadband_shift = 0.0;
	double shift_direction = 0.0;
	double shift_setpoint = 0.0;
	double prediction_ramp = 0.0;
	double prediction_range = 0.0;
	double midpoint = 0.0;
	OBJECT *hdr = OBJECTHDR(this);
	char mktname[1024];
	char ctrname[1024];
	if(t1 == next_run && *pMarketId == lastmkt_id && bidmode == BM_PROXY){
		/*
		 * This case is only true when dealing with co-simulation with FNCS.
		 */
		return TS_NEVER;
	}

	/* short circuit if the state variable doesn't change during the specified interval */
	if((t1 < next_run) && (*pMarketId == lastmkt_id)){
		if(t1 <= next_run - bid_delay){
			if(use_predictive_bidding == TRUE && (((control_mode == CN_RAMP || control_mode == CN_DOUBLE_PRICE) && last_setpoint != setpoint0) || (control_mode == CN_DOUBLE_RAMP && (last_heating_setpoint != heating_setpoint0 || last_cooling_setpoint != cooling_setpoint0)))) {
				;
			} else {// check to see if we have changed states
				if(pState == 0){
					return next_run;
				} else if(*pState == last_pState){
					return next_run;
				}
			}
		} else {
			return next_run;
		}
	}
	
	if(use_predictive_bidding == TRUE){
		deadband_shift = *pDeadband * 0.5;
	}
/*
	if(control_mode == CN_RAMP){
		// if market has updated, continue onwards
		if(*pMarketId != lastmkt_id){// && (*pAvg == 0.0 || *pStd == 0.0 || setpoint0 == 0.0)){
			lastmkt_id = *pMarketId;
			lastbid_id = -1; // clear last bid id, refers to an old market
			// update using last price
			// T_set,a = T_set + (P_clear - P_avg) * | T_lim - T_set | / (k_T * stdev24)

			clear_price = *pClearedPrice;
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
			if(fabs(*pStd) < bid_offset){
				set_temp = setpoint0;
			} else if(clear_price < *pAvg && range_low != 0){
				set_temp = setpoint0 + (clear_price - *pAvg) * fabs(range_low) / (ramp_low * *pStd) + deadband_shift*shift_direction;
			} else if(clear_price > *pAvg && range_high != 0){
				set_temp = setpoint0 + (clear_price - *pAvg) * fabs(range_high) / (ramp_high * *pStd) + deadband_shift*shift_direction;
			} else {
				set_temp = setpoint0 + deadband_shift*shift_direction;
			}

			if((use_override == OU_ON) && (pOverride != 0)){
				if(clear_price <= last_p){
					// if we're willing to pay as much as, or for more than the offered price, then run.
					*pOverride = 1;
				} else {
					*pOverride = -1;
				}
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

		if(dir > 0){
			if(use_predictive_bidding == TRUE){
				if(*pState == 0 && *pMonitor > (max - deadband_shift)){
					bid = *pPriceCap;
				} else if(*pState != 0 && *pMonitor < (min + deadband_shift)){
					bid = 0.0;
					no_bid = 1;
				} else if(*pState != 0 && *pMonitor > max){
					bid = *pPriceCap;
				} else if(*pState == 0 && *pMonitor < min){
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(*pMonitor > max){
					bid = *pPriceCap;
				} else if (*pMonitor < min){
					bid = 0.0;
					no_bid = 1;
				}
			}
		} else if(dir < 0){
			if(use_predictive_bidding == TRUE){
				if(*pState == 0 && *pMonitor < (min + deadband_shift)){
					bid = *pPriceCap;
				} else if(*pState != 0 && *pMonitor > (max - deadband_shift)){
					bid = 0.0;
					no_bid = 1;
				} else if(*pState != 0 && *pMonitor < min){
					bid = *pPriceCap;
				} else if(*pState == 0 && *pMonitor > max){
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(*pMonitor < min){
					bid = *pPriceCap;
				} else if (*pMonitor > max){
					bid = 0.0;
					no_bid = 1;
				}
			}
		} else if(dir == 0){
			if(use_predictive_bidding == TRUE){
				if(direction == 0.0) {
					gl_error("the variable direction did not get set correctly.");
				} else if((*pMonitor > max + deadband_shift || (*pState != 0 && *pMonitor > min - deadband_shift)) && direction > 0){
					bid = *pPriceCap;
				} else if((*pMonitor < min - deadband_shift || (*pState != 0 && *pMonitor < max + deadband_shift)) && direction < 0){
					bid = *pPriceCap;
				} else {
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(*pMonitor < min){
					bid = *pPriceCap;
				} else if(*pMonitor > max){
					bid = 0.0;
					no_bid = 1;
				} else {
					bid = *pAvg;
				}
			}
		}

		// calculate bid price
		if(*pMonitor > setpoint0){
			k_T = ramp_high;
			T_lim = range_high;
		} else if(*pMonitor < setpoint0) {
			k_T = ramp_low;
			T_lim = range_low;
		} else {
			k_T = 0.0;
			T_lim = 0.0;
		}
		
		
		if(bid < 0.0 && *pMonitor != setpoint0) {
			bid = *pAvg + ( (fabs(*pStd) < bid_offset) ? 0.0 : (*pMonitor - setpoint0) * (k_T * *pStd) / fabs(T_lim) );
		} else if(*pMonitor == setpoint0) {
			bid = *pAvg;
		}

		// bid the response part of the load
		double residual = *pTotal;
		// WARNING ~ bid ID check will not work properly
		//KEY bid_id = (KEY)(lastmkt_id == *pMarketId ? lastbid_id : -1);
		// override
		//bid_id = -1;
		if(*pDemand > 0 && no_bid != 1){
			last_p = bid;
			last_q = *pDemand;
			if(0 != strcmp(market_unit, "")){
				if(0 == gl_convert("kW", market_unit, &(last_q))){
					gl_error("unable to convert bid units from 'kW' to '%s'", market_unit.get_string());
					return TS_INVALID;
				}
			}
			//lastbid_id = market->submit(OBJECTHDR(this), -last_q, last_p, bid_id, (BIDDERSTATE)(pState != 0 ? *pState : 0));
			controller_bid.market_id = lastmkt_id;
			controller_bid.price = last_p;
			controller_bid.quantity = -last_q;
			if(pState != 0){
				if (*pState > 0) {
					controller_bid.state = BS_ON;
				} else {
					controller_bid.state = BS_OFF;
				}
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)gl_name(pMarket, mktname, 1024), "submit_bid_state_ccsi", "auction_ccsi", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			} else {
				controller_bid.state = BS_UNKNOWN;
				((void (*)(char *, char *, char *, char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), (char *)gl_name(pMarket, mktname, 1024), "submit_bid_state_ccsi", "auction_ccsi", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			}
			if(controller_bid.bid_accepted == false){
				return TS_INVALID;
			}
			residual -= *pLoad;

		} else {
			last_p = 0;
			last_q = 0;
			gl_verbose("%s's is not bidding", hdr->name);
		}
		if(residual < -0.001) {
			gl_warning("controller_ccsi:%d: residual unresponsive load is negative! (%.1f kW)", hdr->id, residual);
		}

	} else if (control_mode == CN_DOUBLE_RAMP){
*/
	if (control_mode == CN_DOUBLE_RAMP){

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
		if(cool_min - heat_max < *pDeadband){
			switch(resolve_mode){
				case RM_DEADBAND:
					midpoint = (heat_max + cool_min) / 2;
					if(midpoint - *pDeadband/2 < heating_setpoint0 || midpoint + *pDeadband/2 > cooling_setpoint0) {
						gl_error("The midpoint between the max heating setpoint and the min cooling setpoint must be half a deadband away from each base setpoint");
						return TS_INVALID;
					} else {
						heat_max = midpoint - *pDeadband/2;
						cool_min = midpoint + *pDeadband/2;
					}
					break;
				case RM_SLIDING:
					if(heat_max > cooling_setpoint0 - *pDeadband) {
						gl_error("the max heating setpoint must be a full deadband less than the cooling_base_setpoint");
						return TS_INVALID;
					}

					if(cool_min < heating_setpoint0 + *pDeadband) {
						gl_error("The min cooling setpoint must be a full deadband greater than the heating_base_setpoint");
						return TS_INVALID;
					}
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
		// if the market has updated,
		if(lastmkt_id != *pMarketId){
			lastmkt_id = *pMarketId;
			lastbid_id = -1;
			// retrieve cleared price
			clear_price = *pClearedPrice;
			controller_bid.rebid = false;
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
			if(fabs(*pStd) < bid_offset){
				*pCoolingSetpoint = cooling_setpoint0;
				*pHeatingSetpoint = heating_setpoint0;
			} else {
				if(clear_price > *pAvg){
					*pCoolingSetpoint = cooling_setpoint0 + (clear_price - *pAvg) * fabs(cool_range_high) / (cool_ramp_high * *pStd) + deadband_shift*shift_direction;
					//*pHeatingSetpoint = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_range_high) / (heat_ramp_high * *pStd);
					*pHeatingSetpoint = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_range_low) / (heat_ramp_low * *pStd) + deadband_shift*shift_direction;
				} else if(clear_price < *pAvg){
					*pCoolingSetpoint = cooling_setpoint0 + (clear_price - *pAvg) * fabs(cool_range_low) / (cool_ramp_low * *pStd) + deadband_shift*shift_direction;
					//*pHeatingSetpoint = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_range_low) / (heat_ramp_low * *pStd);
					*pHeatingSetpoint = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_range_high) / (heat_ramp_high * *pStd) + deadband_shift*shift_direction;
				} else {
					*pCoolingSetpoint = cooling_setpoint0 + deadband_shift*shift_direction;
					*pHeatingSetpoint = heating_setpoint0 + deadband_shift*shift_direction;
				}
			}

			// apply overrides
			if((use_override == OU_ON)){
				if(last_q != 0.0){
					if(clear_price == last_p && clear_price != *pPriceCap){
						if(*pMarginMode == AM_DENY_CCSI){
							*pOverride = -1;
						} else if(*pMarginMode == AM_PROB_CCSI){
							double r = gl_random_uniform(RNGSTATE,0, 1.0);
							if(r < *pMarginalFraction){
								*pOverride = 1;
							} else {
								*pOverride = -1;
							}
						}
					} else if(*pClearedPrice <= last_p){
						*pOverride = 1;
					} else {
						*pOverride = -1;
					}
				} else { // equality
					*pOverride = 0; // 'normal operation'
				}
			}

			//clip
			if(*pCoolingSetpoint > cool_max)
				*pCoolingSetpoint = cool_max;
			if(*pCoolingSetpoint < cool_min)
				*pCoolingSetpoint = cool_min;
			if(*pHeatingSetpoint > heat_max)
				*pHeatingSetpoint = heat_max;
			if(*pHeatingSetpoint < heat_min)
				*pHeatingSetpoint = heat_min;

			lastmkt_id = *pMarketId;


		}
		
		// submit bids
		double previous_q = last_q; //store the last value, in case we need it
		last_p = 0.0;
		last_q = 0.0;
		
		// We have to cool
		if(*pMonitor > cool_max){
			last_p = *pPriceCap;
			last_q = *pCoolingDemand;
		}
		// We have to heat
		else if(*pMonitor < heat_min){
			last_p = *pPriceCap;
			last_q = *pHeatingDemand;
		}
		// We're floating in between heating and cooling
		else if(*pMonitor > heat_max && *pMonitor < cool_min){
			last_p = 0.0;
			last_q = 0.0;
		}
		// We might heat, if the price is right
		else if(*pMonitor <= heat_max && *pMonitor >= heat_min){
			double ramp, range;
			ramp = (*pMonitor > heating_setpoint0 ? heat_ramp_high : heat_ramp_low);
			range = (*pMonitor > heating_setpoint0 ? heat_range_high : heat_range_low);
			if(*pMonitor != heating_setpoint0){
				last_p = *pAvg + ( (fabs(*pStd) < bid_offset) ? 0.0 : (*pMonitor - heating_setpoint0) * ramp * (*pStd) / fabs(range) );
			} else {
				last_p = *pAvg;
			}
			last_q = *pHeatingDemand;
		}
		// We might cool, if the price is right
		else if(*pMonitor <= cool_max && *pMonitor >= cool_min){
			double ramp, range;
			ramp = (*pMonitor > cooling_setpoint0 ? cool_ramp_high : cool_ramp_low);
			range = (*pMonitor > cooling_setpoint0 ? cool_range_high : cool_range_low);
			if(*pMonitor != cooling_setpoint0){
				last_p = *pAvg + ( (fabs(*pStd) < bid_offset) ? 0 : (*pMonitor - cooling_setpoint0) * ramp * (*pStd) / fabs(range) );
			} else {
				last_p = *pAvg;
			}
			last_q = *pCoolingDemand;
		}

		if(last_p > *pPriceCap)
			last_p = *pPriceCap;
		if(last_p < -*pPriceCap)
			last_p = -*pPriceCap;
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
			if (pState != 0 ) {
				if (*pState > 0) {
					controller_bid.state = BS_ON;
				} else {
					controller_bid.state = BS_OFF;
				}
				((void (*)(char *, char *, const char *, const char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), pMkt, "submit_bid_state_ccsi", "auction_ccsi", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			} else {
				controller_bid.state = BS_UNKNOWN;
				((void (*)(char *, char *, const char *, const char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), pMkt, "submit_bid_state_ccsi", "auction_ccsi", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			}
			if(controller_bid.bid_accepted == false){
				return TS_INVALID;
			}
		}
		else
		{
			if (last_pState != *pState)
			{
				KEY bid = (KEY)(lastmkt_id == *pMarketId ? lastbid_id : -1);
				double my_bid = -*pPriceCap;
				if (*pState != 0)
					my_bid = last_p;
				if (*pState > 0) {
					controller_bid.state = BS_ON;
				} else {
					controller_bid.state = BS_OFF;
				}
				((void (*)(char *, char *, const char *, const char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), pMkt, "submit_bid_state_ccsi", "auction_ccsi", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				if(controller_bid.bid_accepted == false){
					return TS_INVALID;
				}
				controller_bid.rebid = true;
			}
		}
	}

	if (pState != 0)
		last_pState = *pState;

	char timebuf[128];
	gl_printtime(t1,timebuf,127);
	//gl_verbose("controller_ccsi:%i::sync(): bid $%f for %f kW at %s",hdr->id,last_p,last_q,timebuf);
	//return postsync(t0, t1);	
	return TS_NEVER;
}

TIMESTAMP controller_ccsi::postsync(TIMESTAMP t0, TIMESTAMP t1){
	TIMESTAMP rv = next_run - bid_delay;
	double bid = -1.0;
	double bid1 = -1.0;
	double bid2 = -1.0;
	int64 no_bid = 0; // flag gets set when the current temperature drops in between the the heating setpoint and cooling setpoint curves
	int64 no_bid1 = 0;
	int64 no_bid2 = 0;
	double demand = 0.0;
	double rampify = 0.0;
	double deadband_shift = 0.0;
	double shift_direction = 0.0;
	double shift_setpoint = 0.0;
	double prediction_ramp = 0.0;
	double prediction_range = 0.0;
	double midpoint = 0.0;
	OBJECT *hdr = OBJECTHDR(this);
	char mktname[1024];
	char ctrname[1024];

	if(use_predictive_bidding == TRUE){
		deadband_shift = *pDeadband * 0.5;
	}

	if(control_mode == CN_DOUBLE_PRICE){
		if (t1 == next_run - bid_delay) {
			double doubleTemp[2];
			doubleTemp[0] = 0.0;
			doubleTemp[1] = 0.0;
			doubleTemp[0] = calcTemp1(*pMonitor, *pDeadband, *pState);
			doubleTemp[1] = calcTemp2(*pUa, *pHm, *pCa, *pCm, *pMassInternalGainFraction, *pMassSolarGainFraction, *pQi, *pQs, *pQh, *pTout, *pMonitor, *pTmass, *pDeadband, *pState);
			Temp12 = (doubleTemp[0]+doubleTemp[1])/2;

			//Find bidprice 1
			if(dir > 0){
				if(use_predictive_bidding == TRUE){
					if(*pState == 0 && Temp12 > (max - deadband_shift)){
						bid1 = *pPriceCap;
					} else if(*pState != 0 && Temp12 < (min + deadband_shift)){
						bid1 = 0.0;
						no_bid1 = 1;
					} else if(*pState != 0 && Temp12 > max){
						bid1 = *pPriceCap;
					} else if(*pState == 0 && Temp12 < min){
						bid1 = 0.0;
						no_bid1 = 1;
					}
				} else {
					if(Temp12 > max){
						bid1 = *pPriceCap;
					} else if (Temp12 < min){
						bid1 = 0.0;
						no_bid1 = 1;
					}
				}
			} else if(dir < 0){
				if(use_predictive_bidding == TRUE){
					if(*pState == 0 && Temp12 < (min + deadband_shift)){
						bid1 = *pPriceCap;
					} else if(*pState != 0 && Temp12 > (max - deadband_shift)){
						bid1 = 0.0;
						no_bid1 = 1;
					} else if(*pState != 0 && Temp12 < min){
						bid1 = *pPriceCap;
					} else if(*pState == 0 && Temp12 > max){
						bid1 = 0.0;
						no_bid1 = 1;
					}
				} else {
					if(Temp12 < min){
						bid1 = *pPriceCap;
					} else if (Temp12 > max){
						bid1 = 0.0;
						no_bid1 = 1;
					}
				}
			} else if(dir == 0){
				if(use_predictive_bidding == TRUE){
					if(direction == 0.0) {
						gl_error("the variable direction did not get set correctly.");
					} else if((Temp12 > max + deadband_shift || (*pState != 0 && Temp12 > min - deadband_shift)) && direction > 0){
						bid1 = *pPriceCap;
					} else if((Temp12 < min - deadband_shift || (*pState != 0 && Temp12 < max + deadband_shift)) && direction < 0){
						bid1 = *pPriceCap;
					} else {
						bid1 = 0.0;
						no_bid1 = 1;
					}
				} else {
					if(Temp12 < min){
						bid1 = *pPriceCap;
					} else if(Temp12 > max){
						bid1 = 0.0;
						no_bid1 = 1;
					} else {
						bid1 = *pAvg;
					}
				}
			}

			// calculate bid price
			if(Temp12 > setpoint0){
				k_T = ramp_high;
				T_lim = range_high;
			} else if(Temp12 < setpoint0) {
				k_T = ramp_low;
				T_lim = range_low;
			} else {
				k_T = 0.0;
				T_lim = 0.0;
			}
						
			if(bid1 < 0.0 && Temp12 != setpoint0) {
				bid1 = *pAvg + ( (fabs(*pStd) < bid_offset) ? 0.0 : (Temp12 - setpoint0) * (k_T * *pStd) / fabs(T_lim) );
			} else if(Temp12 == setpoint0) {
				bid1 = *pAvg;
			}
			bid = bid1;
			Pbid = bid;
			
			if(no_bid1 == 1){
				no_bid = 1;
			} else {
				no_bid = 0;
			}

			// bid the response part of the load
			double residual = *pTotal;
			/* WARNING ~ bid ID check will not work properly */
			//KEY bid_id = (KEY)(lastmkt_id == *pMarketId ? lastbid_id : -1);
			//if(*pDemand > 0 && no_bid != 1){
			if (last_q <= 0){
				if (*pDemand >0){
					last_q = *pDemand;	
				}
				else {
					last_q = *pRated_cooling_capacity*0.001/(3.4120*(*pCooling_COP));
				}
			}
			else {
				if (*pDemand>0) {
					last_q = *pDemand;
				}
			}

			last_p = bid;

			if (!(last_q > 0 && *pDemand <= 0)) { // only do unit conversion if value has changed from the previous; otherwise unit is W already
				if(0 != strcmp(market_unit, "")){
					if(0 == gl_convert("kW", market_unit, &(last_q))){
						gl_error("unable to convert bid units from 'kW' to '%s'", market_unit);
						return TS_INVALID;
					}
				}
			}
			//lastbid_id = market->submit(OBJECTHDR(this), -last_q, last_p, bid_id, (BIDDERSTATE)(pState != 0 ? *pState : 0));
			controller_bid.market_id = lastmkt_id;
			controller_bid.price = last_p;
			controller_bid.quantity = -last_q;
			if(pState != 0){
				if (*pState > 0) {
					controller_bid.state = BS_ON;
				} else {
					controller_bid.state = BS_OFF;
				}
				((void (*)(char *, char *, const char *, const char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), pMkt, "submit_bid_state_ccsi", "auction_ccsi", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			} else {
				controller_bid.state = BS_UNKNOWN;
				((void (*)(char *, char *, const char *, const char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), pMkt, "submit_bid_state_ccsi", "auction_ccsi", (void *)&controller_bid, (size_t)sizeof(controller_bid));
				controller_bid.rebid = true;
			}
			if(controller_bid.bid_accepted == false){
				return TS_INVALID;
			}
			residual -= *pLoad;

			/*} else {
				last_p = 0;
				last_q = 0;
				gl_verbose("%s's is not bidding", hdr->name);
			}*/


			if(residual < -0.001)
				gl_warning("controller_ccsi:%d: residual unresponsive load is negative! (%.1f kW)", hdr->id, residual);

			parent_unresponsive_load=*pTotal-*pLoad;

			if(0 != strcmp(market_unit, "")){
				if(0 == gl_convert("kW", market_unit, &(parent_unresponsive_load))){
					gl_error("unable to convert parent unresponsive load units from 'kW' to '%s'", market_unit);
					return TS_INVALID;
				}
			}
		}
	}


	if(control_mode == CN_RAMP){
		if (t1 == next_run - bid_delay) {
			if(dir > 0){
				if(use_predictive_bidding == TRUE){
					if(*pState == 0 && *pMonitor > (max - deadband_shift)){
						bid = *pPriceCap;
					} else if(*pState != 0 && *pMonitor < (min + deadband_shift)){
						bid = 0.0;
						no_bid = 1;
					} else if(*pState != 0 && *pMonitor > max){
						bid = *pPriceCap;
					} else if(*pState == 0 && *pMonitor < min){
						bid = 0.0;
						no_bid = 1;
					}
				} else {
					if(*pMonitor > max){
						bid = *pPriceCap;
					} else if (*pMonitor < min){
						bid = 0.0;
						no_bid = 1;
					}
				}
			} else if(dir < 0){
				if(use_predictive_bidding == TRUE){
					if(*pState == 0 && *pMonitor < (min + deadband_shift)){
						bid = *pPriceCap;
					} else if(*pState != 0 && *pMonitor > (max - deadband_shift)){
						bid = 0.0;
						no_bid = 1;
					} else if(*pState != 0 && *pMonitor < min){
						bid = *pPriceCap;
					} else if(*pState == 0 && *pMonitor > max){
						bid = 0.0;
						no_bid = 1;
					}
				} else {
					if(*pMonitor < min){
						bid = *pPriceCap;
					} else if (*pMonitor > max){
						bid = 0.0;
						no_bid = 1;
					}
				}
			} else if(dir == 0){
				if(use_predictive_bidding == TRUE){
					if(direction == 0.0) {
						gl_error("the variable direction did not get set correctly.");
					} else if((*pMonitor > max + deadband_shift || (*pState != 0 && *pMonitor > min - deadband_shift)) && direction > 0){
						bid = *pPriceCap;
					} else if((*pMonitor < min - deadband_shift || (*pState != 0 && *pMonitor < max + deadband_shift)) && direction < 0){
						bid = *pPriceCap;
					} else {
						bid = 0.0;
						no_bid = 1;
					}
				} else {
					if(*pMonitor < min){
						bid = *pPriceCap;
					} else if(*pMonitor > max){
						bid = 0.0;
						no_bid = 1;
					} else {
						bid = *pAvg;
					}
				}
			}

			// calculate bid price
			if(*pMonitor > setpoint0){
				k_T = ramp_high;
				T_lim = range_high;
			} else if(*pMonitor < setpoint0) {
				k_T = ramp_low;
				T_lim = range_low;
			} else {
				k_T = 0.0;
				T_lim = 0.0;
			}
			
			
			if(bid < 0.0 && *pMonitor != setpoint0) {
				bid = *pAvg + ( (fabs(*pStd) < bid_offset) ? 0.0 : (*pMonitor - setpoint0) * (k_T * *pStd) / fabs(T_lim) );
			} else if(*pMonitor == setpoint0) {
				bid = *pAvg;
			}

			// bid the response part of the load
			double residual = *pTotal;
			/* WARNING ~ bid ID check will not work properly */
			//KEY bid_id = (KEY)(lastmkt_id == *pMarketId ? lastbid_id : -1);
			// override
			//bid_id = -1;
			if(*pDemand > 0 && no_bid != 1){
				last_p = bid;
				last_q = *pDemand;
				if(0 != strcmp(market_unit, "")){
					if(0 == gl_convert("kW", market_unit, &(last_q))){
						gl_error("unable to convert bid units from 'kW' to '%s'", market_unit);
						return TS_INVALID;
					}
				}
				//lastbid_id = market->submit(OBJECTHDR(this), -last_q, last_p, bid_id, (BIDDERSTATE)(pState != 0 ? *pState : 0));
				//lastbid_id = market->submit(OBJECTHDR(this), -last_q, last_p, bid_id, (BIDDERSTATE)(pState != 0 ? *pState : 0));
				controller_bid.market_id = lastmkt_id;
				controller_bid.price = last_p;
				controller_bid.quantity = -last_q;
				if(pState != 0){
					if (*pState > 0) {
						controller_bid.state = BS_ON;
					} else {
						controller_bid.state = BS_OFF;
					}
					((void (*)(char *, char *, const char *, const char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), pMkt, "submit_bid_state_ccsi", "auction_ccsi", (void *)&controller_bid, (size_t)sizeof(controller_bid));
					controller_bid.rebid = true;
				} else {
					controller_bid.state = BS_UNKNOWN;
					((void (*)(char *, char *, const char *, const char *, void *, size_t))(*submit))((char *)gl_name(hdr, ctrname, 1024), pMkt, "submit_bid_state_ccsi", "auction_ccsi", (void *)&controller_bid, (size_t)sizeof(controller_bid));
					controller_bid.rebid = true;
				}
				if(controller_bid.bid_accepted == false){
					return TS_INVALID;
				}
				residual -= *pLoad;

			} else {
				last_p = 0;
				last_q = 0;
				gl_verbose("%s's is not bidding", hdr->name);
			}
			if(residual < -0.001) {
				gl_warning("controller_ccsi:%d: residual unresponsive load is negative! (%.1f kW)", hdr->id, residual);
			}

			parent_unresponsive_load=*pTotal-*pLoad;

			if(0 != strcmp(market_unit, "")){
				if(0 == gl_convert("kW", market_unit, &(parent_unresponsive_load))){
					gl_error("unable to convert parent unresponsive load units from 'kW' to '%s'", market_unit);
					return TS_INVALID;
				}
			}
		}

	}

	if(last_setpoint != setpoint0 && (control_mode == CN_RAMP || control_mode == CN_DOUBLE_PRICE)){
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
		if(*pHeatState == 1 || *pAuxState == 1){
			thermostat_mode = TM_HEAT;
			if(last_mode == TM_OFF)
				time_off = TS_NEVER;
		} else if(*pCoolState == 1){
			thermostat_mode = TM_COOL;
			if(last_mode == TM_OFF)
				time_off = TS_NEVER;
		} else if(*pHeatState == 0 && *pAuxState == 0 && *pCoolState == 0){
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

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_controller_ccsi(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(controller_ccsi::oclass);
		if (*obj!=NULL)
		{
			controller_ccsi *my = OBJECTDATA(*obj,controller_ccsi);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(controller_ccsi);
}

EXPORT int init_controller_ccsi(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
		{
			return OBJECTDATA(obj,controller_ccsi)->init(parent);
		}
		else
			return 0;
	}
	INIT_CATCHALL(controller_ccsi);
}

EXPORT int isa_controller_ccsi(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,controller_ccsi)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_controller_ccsi(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	controller_ccsi *my = OBJECTDATA(obj,controller_ccsi);
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
	SYNC_CATCHALL(controller_ccsi);
}

// EOF
