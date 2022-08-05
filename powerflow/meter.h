// $Id: meter.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _METER_H
#define _METER_H

#include "powerflow.h"
#include "node.h"

EXPORT SIMULATIONMODE interupdate_meter(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

//KML export
EXPORT int meter_kmldata(OBJECT *obj,int (*stream)(const char*,...));

class meter : public node
{
public:
	gld::complex measured_voltage[3];	///< measured voltage
	gld::complex measured_voltageD[3];	///< measured voltage - Line-to-Line
	double measured_real_max_voltage_in_interval[3];	///< measured real max voltage in a time interval
	double measured_real_max_voltageD_in_interval[3];	///< measured real max voltage in a time interval - Line-to-Line
	double measured_real_min_voltage_in_interval[3];	///< measured real min voltage in a time interval
	double measured_real_min_voltageD_in_interval[3];	///< measured real min voltage in a time interval - Line-to-Line
	double measured_avg_voltage_mag_in_interval[3];	///< measured avg voltage magnitude in a time interval
	double measured_avg_voltageD_mag_in_interval[3];	///< measured avg voltage magnitude in a time interval - Line-to-Line
	double measured_reactive_max_voltage_in_interval[3];	///< measured reactive max voltage in a time interval
	double measured_reactive_max_voltageD_in_interval[3];	///< measured reactive max voltage in a time interval - Line-to-Line
	double measured_reactive_min_voltage_in_interval[3];	///< measured reactive min voltage in a time interval
	double measured_reactive_min_voltageD_in_interval[3];	///< measured reactive min voltage in a time interval - Line-to-Line
	double measured_real_avg_power_in_interval;	///< metered real part of average power over the last interval
	double measured_reactive_avg_power_in_interval;	///< metered reactive part of average power over the last interval
	double measured_real_max_power_in_interval;	///< maximum real power over the last interval
	double measured_reactive_max_power_in_interval;	///< maximum real power over the last interval
	double measured_real_min_power_in_interval;	///< minimum real power over the last interval
	double measured_reactive_min_power_in_interval;	///< minimum real power over the last interval
    double measured_real_avg_power_in_interval_3ph[3];	///< metered real part of average power over the last interval
    double measured_reactive_avg_power_in_interval_3ph[3];	///< metered reactive part of average power over the last interval
    double measured_real_max_power_in_interval_3ph[3];	///< maximum real power over the last interval
    double measured_reactive_max_power_in_interval_3ph[3];	///< maximum real power over the last interval
    double measured_real_min_power_in_interval_3ph[3];	///< minimum real power over the last interval
    double measured_reactive_min_power_in_interval_3ph[3];	///< minimum real power over the last interval
	double measured_min_max_avg_timestep; // Period of timestep for min/max/average calculations

	gld::complex measured_current[3];	///< measured current
	double measured_real_energy;	///< metered real energy consumption
	double measured_real_energy_delta;	///< metered real energy consumption over last interval
	double measured_reactive_energy;///< metered reactive energy consumption
	double measured_reactive_energy_delta;///< metered reactive energy consumption over last interval
    double measured_energy_delta_timestep; // Period of timestep for real and reactive delta energy calculation
	gld::complex measured_power;			///< metered power
	double measured_demand;			///< metered demand (peak of power)
	double measured_real_power;		///< metered real power
	double last_measured_real_power; ///< previous metered real power
	double last_measured_real_power_3ph[3]; ///< previous metered real power, all three phases
	double measured_reactive_power; ///< metered reactive power
	double last_measured_reactive_power; ///< previously measured reactive power
	double last_measured_reactive_power_3ph[3]; ///< previously measured reactive power, all three phases
	gld::complex indiv_measured_power[3];///< metered power on each phase
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
	gld::complex pre_load;		//the load prior to being interrupted
#endif

	double hourly_acc;
	double previous_monthly_bill;	//Total monthly bill for the previous month
	double previous_monthly_energy;	//Total monthly energy for the previous month
	double monthly_bill;			//Accumulator for the current month's bill
	double monthly_fee;				//Once a month flat fee for customer hook-up
	double monthly_energy;			//Accumulator for the current month's energy
	gld::complex meter_power_consumption; //Represents standby power or losses assosciated with electric meters
	int16 no_phases;

	typedef enum {
		BM_NONE,
		BM_UNIFORM,
		BM_TIERED,
		BM_HOURLY,
		BM_TIERED_RTP,
		BM_TIERED_TOU
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

	int check_prices();

	void BOTH_meter_sync_fxn(void);

	SIMULATIONMODE inter_deltaupdate_meter(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

private:
	double process_bill(TIMESTAMP t1);
	double previous_energy_total;  // Used to track what the meter reading was the previous month
    double last_measured_real_energy;
    double last_measured_reactive_energy;
    gld::complex last_measured_max_voltage_mag[3];
    gld::complex last_measured_max_voltageD_mag[3];
    gld::complex last_measured_min_voltage_mag[3];
    gld::complex last_measured_min_voltageD_mag[3];
    gld::complex last_measured_voltage[3];
    gld::complex last_measured_voltageD[3];
    double last_measured_avg_voltage_mag[3];
    double last_measured_avg_voltageD_mag[3];

	double last_measured_max_real_power;
	double last_measured_min_real_power;
	double last_measured_max_reactive_power;
	double last_measured_min_reactive_power;
	double last_measured_avg_real_power;
	double last_measured_avg_reactive_power;

    double last_measured_max_real_power_3ph[3];
    double last_measured_min_real_power_3ph[3];
    double last_measured_max_reactive_power_3ph[3];
    double last_measured_min_reactive_power_3ph[3];
    double last_measured_avg_real_power_3ph[3];
    double last_measured_avg_reactive_power_3ph[3];

    int voltage_avg_count;
    TIMESTAMP last_delta_timestamp;
    TIMESTAMP last_stat_timestamp;
    TIMESTAMP start_timestamp;
    TIMESTAMP interval_dt;

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
	int kmldata(int (*stream)(const char*,...));
};

#endif // _METER_H

