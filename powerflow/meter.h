// $Id: meter.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _METER_H
#define _METER_H

#include "powerflow.h"
#include "node.h"

EXPORT SIMULATIONMODE interupdate_meter(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

class meter : public node
{
public:
	complex measured_voltage[3];	///< measured voltage
	complex measured_voltageD[3];	///< measured voltage - Line-to-Line
	complex measured_current[3];	///< measured current
	double measured_real_energy;	///< metered real energy consumption
	double measured_reactive_energy;///< metered reactive energy consumption
	complex measured_power;			///< metered power
	double measured_demand;			///< metered demand (peak of power)
	double measured_real_power;		///< metered real power
	double last_measured_real_power; ///< previous metered real power
	double measured_reactive_power; ///< metered reactive power
	complex indiv_measured_power[3];///< metered power on each phase
	bool meter_interrupted;			///< Reliability flag - goes active if the customer is in an "interrupted" state
	bool meter_interrupted_secondary;	///< Reliability flag - goes active if the customer is in an "secondary interrupted" state - i.e., momentary
	bool meter_NR_servered;			///< Flag for NR solver, server mode (not standalone), and SWING designation
	TIMESTAMP next_time;
	TIMESTAMP dt;
	TIMESTAMP last_t;

#ifdef SUPPORT_OUTAGES
	int16 sustained_count;	//reliability sustained event counter
	int16 momentary_count;	//reliability momentary event counter
	int16 total_count;		//reliability total event counter
	int16 s_flag;			//reliability flag that gets set if the meter experienced more than n sustained interruptions
	int16 t_flag;			//reliability flage that gets set if the meter experienced more than n events total
	complex pre_load;		//the load prior to being interrupted
#endif

	double hourly_acc;
	double previous_monthly_bill;	//Total monthly bill for the previous month
	double previous_monthly_energy;	//Total monthly energy for the previous month
	double monthly_bill;			//Accumulator for the current month's bill
	double monthly_fee;				//Once a month flat fee for customer hook-up
	double monthly_energy;			//Accumulator for the current month's energy
	complex meter_power_consumption; //Represents standby power or losses assosciated with electric meters
	int16 no_phases;

	typedef enum {
		BM_NONE,
		BM_UNIFORM,
		BM_TIERED,
		BM_HOURLY,
		BM_TIERED_RTP
	} BILLMODE;
	//BILLMODE bill_mode;
	enumeration bill_mode;
	OBJECT *power_market;
	PROPERTY *price_prop;
	int32 bill_day;					//Day bill is to be processed (assumed to occur at midnight of that day)
	int last_bill_month;			//Keeps track of which month we are looking at
	double price, last_price;					//Standard uniform pricing
	double price_base, last_price_base;
	double tier_price[3], tier_energy[3], last_tier_price[3];  //Allows for additional tiers of pricing over the standard price in TIERED

	double process_bill(TIMESTAMP t1);
	int check_prices();

	void BOTH_meter_sync_fxn(void);

	SIMULATIONMODE inter_deltaupdate_meter(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

private:
	double previous_energy_total;  // Used to track what the meter reading was the previous month

public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	meter(MODULE *mod);
	inline meter(CLASS *cl=oclass):node(cl){};
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0);
	int isa(char *classname);
};

#endif // _METER_H

