/** $Id: evcharger_det.h
	Copyright (C) 2012 Battelle Memorial Institute

 **/

#ifndef _EVCHARGER_DET_H
#define _EVCHARGER_DET_H

#include "residential.h"
#include "residential_enduse.h"

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
	TIMESTAMP next_state_change;	///< Timestamp of next transition (home->work, work->home)
} VEHICLEDATA;

class evcharger_det : public residential_enduse
{
public:
	
	double ChargeRate;				///< Current commanded charge rate of the vehicle

	double variation_mean;			///< Mean of normal variation of schedule variation
	double variation_std_dev;		///< Standard deviation of normal variation of schedule times

	double variation_trip_mean;		///< Mean of normal variation of trip length
	double variation_trip_std_dev;	///< Standard deviation of normal variation of trip length

	double mileage_classification;	///< Mileage classification of electric vehicle

	bool Work_Charge_Available;		///< Variable to indicate if any SOC is recovered while the vehicle is "at work"

	char1024 NHTSDataFile;			///< Path to NHTS travel data
	unsigned int VehicleLocation;	///< Index of NHTS to this vehicle's data

	VEHICLEDATA CarInformation;		///< Structure of information for this particular PHEV/EV

	static CLASS *oclass, *pclass;

	evcharger_det(MODULE *module);
	~evcharger_det();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

private:
	TIMESTAMP glob_min_timestep;	///< Variable for storing minimum timestep value - if it exists 
	bool off_nominal_time;			///< Flag to indicate a minimum timestep is present

	TIMESTAMP prev_time;			///< Tracking variable

};

#endif // _EVCHARGER_DET_H

/**@}**/
