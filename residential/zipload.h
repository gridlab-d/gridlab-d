/** $Id: zipload.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file zipload.h
	@addtogroup ZIPload
	@ingroup residential

 @{
 **/

#ifndef _ZIPLOAD_H
#define _ZIPLOAD_H
#include "residential.h"
#include "residential_enduse.h"

class ZIPload : public residential_enduse
{
public:
	double base_power;			///< Base real power of the system
	double power_pf;			///< power factor of constant power load
	double current_pf;			///< power factor of constant current load
	double impedance_pf;		///< power factor of constant impedance load
	bool is_240;				///< load connected at 220 
	double breaker_val;			///< Amperage limit for connected breaker
	gld::complex actual_power;		///< Actual load after adjusted for voltage factors

	bool demand_response_mode;	///< Activates equilibrium dynamic representation of demand response 
	int64 N;					///< Number of devices to model - base power is per device 
	int16 L;					///< Range of the thermostat's control operation 
	double N_off;				///< Number of devices that are off 
	double N_on;					///< Number of devices that are on 
	double roff;				///< rate at which devices cool down 
	double ron;					///< rate at which devices heat up 
	double t; 					///< total cycle time of a thermostatic device 
	int16 x;					///< temperature of the device's controlled media (eg air temp or water temp) 
	double phi;					///< duty cycle of the device 
	double eta;					///< consumer demand rate that prematurely turns on a device or population 
	double nominal_power;
	int64 next_time, last_time; ///< used to keep track of time in "special" modes - DR, duty-cycle
	double duty_cycle;			///< effective duty cycle of device
	double last_duty_cycle;
	double period;				///< period at which duty cycle is applied
	double phase;				///< phase of the duty cycle in terms of 0-1
	double multiplier;			///< static multiplier to modify base power ( load = base_power * multiplier )
	double recovery_duty_cycle; ///< duty cycle during recovery interval
	bool heatgain_only;			///< Activates a heat only mode - no load electric load is assigned to the load
	 

	typedef struct {
		double *on;
		double *off;
		int16 nbins;
	} DRMODEL;

	DRMODEL drm;

private:
	int first_pass;
	int shared_init(OBJECT *parent);

public:
	static CLASS *oclass, *pclass;
	static ZIPload *defaults;

	ZIPload(MODULE *module);
	~ZIPload();
	int create();
	int init(OBJECT *parent);
	int checkpoint_init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _ZIPLOAD_H

/**@}**/
