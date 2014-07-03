/** $Id: thermal_storage.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file lights.h
	@addtogroup residential_lights

 @{
 **/

#ifndef _THERMAL_STORAGE_H
#define _THERMAL_STORAGE_H

#include "residential.h"
#include "residential_enduse.h"

typedef enum {
		INTERNAL=0,		//Use internal schedule
		EXTERNAL=1		//Use published variables for schedule
} THERMAL_SCHEDULE_TYPE;

class thermal_storage : public residential_enduse {
public:
	double total_capacity;			///< Scaled total capacity [Btu]
	double stored_capacity;			///< Current level of stored capacity [Btu/hr]
	double recharge_power;			///< Scaled power usage based on outdoor air temp to recharge [kW]
	double recharge_power_factor;	///< Power factor associated with recharging operation
	double discharge_power;			///< Scaled power usage to cool the air [kW]
	double discharge_power_factor;	///< Power factor associated with discharging operation
	double discharge_rate;			///< Based on design cooling from house [Btu/hr]
	bool recharge;					///< set 1 if recharging, 0 not
	double recharge_time;			///< recharge cycle set on by schedule, can not overlap with discharge schedule
	double discharge_time;			///< discharge cycle set on by schedule, can not overlap with recharge schedule
	double state_of_charge;			///< Thermal storage state of charge variable
	double k;						///< Coefficient of thermal conductivity [W/m/K]
	double water_capacity;			///< Amount of water contained in the system for use as ice storage [m^3]
	double surface_area;			///< Surface area of the water capacity stored in cube [m^2]
	TIMESTAMP last_timestep;		///< Last time the simulation stopped to calculate [sec]
	TIMESTAMP next_timestep;		///< Next time thermal_storage will need to run [delta sec]

	//double *design_cooling_capacity;
	double *outside_temperature;
	double *thermal_storage_available;
	double *thermal_storage_active;

	SCHEDULE *recharge_schedule_vals;
	SCHEDULE *discharge_schedule_vals;

	enumeration recharge_schedule_type;		///< Determines if charging should occur via internal schedule, or an external property
	enumeration discharge_schedule_type;	///< Determines if discharging should occur via internal schedule, or an external property

private:
	double *recharge_time_ptr;
	double *discharge_time_ptr;

public:
	static CLASS *oclass, *pclass;
	thermal_storage(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _THERMAL_STORAGE_H

/**@}**/
