/** $Id: evcharger_det.h
	Copyright (C) 2019 Battelle Memorial Institute

 **/

#ifndef _EVCHARGER_DET_H
#define _EVCHARGER_DET_H

#include "residential.h"
#include "residential_enduse.h"

EXPORT SIMULATIONMODE interupdate_evcharger_det(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);

class evcharger_det : public residential_enduse
{
public:
	typedef enum {
			VL_UNKNOWN=0,					///< defines vehicle state unknown
			VL_HOME=1,						///< Vehicle is at home
			VL_WORK=2,						///< Vehicle is at work
			VL_WORK_TO_HOME=3,				///< Vehicle is commuting from work to home
			VL_HOME_TO_WORK=4				///< Vehicle is commuting from home to work
	} VEHICLELOCATION;

	typedef struct  {
		double travel_distance;			///< Distance the vehicle travels on any trip from home - assumed work dist = travel_distance/2
		double WorkArrive;				///< Time when vehicle arrives at work HHMM
		double WorkDuration;			///< Seconds a vehicle remains at work
		double WorkHomeDuration;		///< Seconds a vehicle is traveling between work and home
		double HomeArrive;				///< Time when vehicle arrives at home HHMM
		double HomeDuration;			///< Seconds a vehicle remains at home
		double HomeWorkDuration;		///< Seconds a vehicle is traveling between home and work
		enumeration Location;			///< Vehicle current locational state
		double battery_capacity;		///< Current capacity of the battery (kW-hours)
		double battery_SOC;				///< Battery state of charge
		double battery_size;			///< Maximum battery size
		double MaxChargeRate;			///< Maximum current rate for the charger (Watts)
		double ChargeEfficiency;		///< Efficiency of the charger (power in to battery power)
		double mileage_efficiency;		///< Powertrain battery use - miles/kWh
		double next_state_change;	///< Timestamp of next transition (home->work, work->home)
	} VEHICLEDATA;

	double DesiredChargeRate;				///< Current commanded charge rate of the vehicle
	double ActualChargeRate;				///< Implemented charge rate (possibly ramp-limited)
	double RealizedChargeRate;				///< Actual charge rate implmented, based on grid conditions
	double max_overload_currentPU;			///< Maximum charger current allowed for the EV

	double variation_mean;			///< Mean of normal variation of schedule variation
	double variation_std_dev;		///< Standard deviation of normal variation of schedule times

	double variation_trip_mean;		///< Mean of normal variation of trip length
	double variation_trip_std_dev;	///< Standard deviation of normal variation of trip length

	double mileage_classification;	///< Mileage classification of electric vehicle

	bool Work_Charge_Available;		///< Variable to indicate if any SOC is recovered while the vehicle is "at work"

	char1024 NHTSDataFile;			///< Path to NHTS travel data
	unsigned int VehicleLocation;	///< Index of NHTS to this vehicle's data

	VEHICLEDATA CarInformation;		///< Structure of information for this particular PHEV/EV

	bool enable_J2894_checks;		///< Enable SAE J2894-specified limitations on when the charger can operate
	double J2894_voltage_high_threshold_values[2][2];	//Time/level of high-end J2894 voltage thresholds
	double J2894_voltage_low_threshold_values[2][2];	//Time/level of low-end J2894 voltage thresholds
	double J2894_off_threshold;			/// Time for the charger to remain disconnected after a violation
	double J2894_ramp_limit;			/// Ramp-rate limit for how fast the EV can go to full power
	bool ev_charger_enabled_state;		///< Indicates if the ev_charger is online and available to use, or tripped offline

	static CLASS *oclass, *pclass;

	evcharger_det(MODULE *module);
	~evcharger_det();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	double sync_ev_function(double curr_time);
	double check_J2894_values(double diff_time,double tstep_value,bool *floored_trip);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);

private:
	TIMESTAMP glob_min_timestep;	///< Variable for storing minimum timestep value - if it exists 
	double glob_min_timestep_dbl;	///< Double version of the above - just to prevent multiple castings
	bool off_nominal_time;			///< Flag to indicate a minimum timestep is present

	double prev_time_dbl;			///< Tracking variable

	bool deltamode_inclusive;	//Boolean for deltamode calls - pulled from object flags
	bool deltamode_registered;	//Boolean for deltamode registration -- basically a "first run" flag

	double J2894_voltage_high_accumulators[2];	//Timing tracking variable for J2894/ANSI C84.1 high voltage levels
	double J2894_voltage_low_accumulators[2];	//Timing tracking variable for J2894/ANSI C84.1 low voltage levels
	bool J2894_voltage_high_state[2];	//Tracking variable for J2894/ANSI C84.1 high voltage levels - indicates if condition hit
	bool J2894_voltage_low_state[2];	//Tracking variable for J2894/ANSI C84.1 low voltage levels - indicates if condition hit

	double J2894_off_accumulator;				//Timing tracking variable for J2894/ANSI C84.1-induced outage
	bool J2894_is_ramp_constrained;				//Variable to indicate if a ramping limit is being hit
	double expected_voltage_base;	///< Basis variable for ramping information

	double max_overload_charge_current;	///< Maximum "electronics" charge rate - converted from PU - in amps

	typedef enum {
		J2894_NONE=0,
		J2894_SURGE_OVER_VOLT=1,
		J2894_HIGH_OVER_VOLT=2,
		J2894_OVER_VOLT=3,
		J2894_UNDER_VOLT=4,
		J2894_LOW_UNDER_VOLT=5,
		J2894_EXTREME_UNDER_VOLT=6,
		J2894_UNKNOWN=7		//Catch-all - mostly for when QSTS trips it on a partial second
	} J2894_TRIP_REASON;	//J2894 Tripped-violation indicator

	enumeration J2894_trip_method;
};

#endif // _EVCHARGER_DET_H

/**@}**/
