// $Id: triplex_meter.h 942 2008-09-19 20:03:17Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TRIPLEXMETER_H
#define _TRIPLEXMETER_H

#include "powerflow.h"
#include "triplex_node.h"

EXPORT SIMULATIONMODE interupdate_triplex_meter(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

class triplex_meter : public triplex_node
{
public:
	complex measured_voltage[3];	///< measured voltage
	complex measured_current[3];	///< measured current
	double measured_real_energy;	///< metered real energy consumption
	double measured_reactive_energy;///< metered reactive energy consumption
	complex measured_power;			///< metered power
	complex indiv_measured_power[3]; ///< individual phase power
	double measured_demand;			///< metered demand (peak of power)
	double measured_real_power;		///< metered real power
	double last_measured_real_power; ///< previous instance's metered real power
	double measured_reactive_power; ///< metered reactive power
	complex tpmeter_power_consumption; ///< power consumed by meter operation
	bool tpmeter_interrupted;		///< Reliability flag - goes active if the customer is in an "interrupted" state
	bool tpmeter_interrupted_secondary;	///< Reliability flag - goes active if the customer is in a "secondary interrupted" state - i.e., momentary
	TIMESTAMP next_time;
	TIMESTAMP dt;
	TIMESTAMP last_t;

#ifdef SUPPORT_OUTAGES
	int16 sustained_count;	///< reliability sustained event counter
	int16 momentary_count;	///< reliability momentary event counter
	int16 total_count;		///< reliability total event counter
	int16 s_flag;			///< reliability flag that gets set if the meter experienced more than n sustained interruptions
	int16 t_flag;			///< reliability flage that gets set if the meter experienced more than n events total
	complex pre_load;		///< the load prior to being interrupted
#endif

	double hourly_acc;
	double previous_monthly_bill;	///< Total monthly bill for the previous month
	double previous_monthly_energy;	///< Total monthly energy for the previous month
	double monthly_bill;			///< Accumulator for the current month's bill
	double monthly_fee;				///< Once a month flat fee for customer hook-up
	double monthly_energy;			///< Accumulator for the current month's energy
	typedef enum {
		BM_NONE,
		BM_UNIFORM,
		BM_TIERED,
		BM_HOURLY,
		BM_TIERED_RTP
	} BILLMODE;						///< Designates the bill mode to be used
	enumeration bill_mode;				///< Designates the bill mode to be used
	OBJECT *power_market;			///< Designates the auction object where prices are read from for bill mode
	PROPERTY *price_prop;			///< Designates what property to observe in the auction object for price
	int32 bill_day;					///< Day bill is to be processed (assumed to occur at midnight of that day)
	int last_bill_month;			///< Keeps track of which month we are looking at
	double price, last_price;		///< Standard uniform pricing
	double price_base, last_price_base; ///< Prices used in rtp pricing
	double tier_price[3], tier_energy[3], last_tier_price[3];  ///< Allows for additional tiers of pricing over the standard price in TIERED

	double process_bill(TIMESTAMP t1);	///< function for processing current bill
	int check_prices();				///< checks to make sure current prices are valid

private:
	double previous_energy_total;  ///< Used to track what the meter reading was the previous month

public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	triplex_meter(MODULE *mod);
	inline triplex_meter(CLASS *cl=oclass):triplex_node(cl){};
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	int isa(char *classname);

	SIMULATIONMODE inter_deltaupdate_triplex_meter(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
};

#endif // _TRIPLEXMETER_H

