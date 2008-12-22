/** $Id: evcharger.h,v 1.9 2008/02/09 23:48:51 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file evcharger.h
	@addtogroup evcharger
	@ingroup residential

 @{
 **/

#ifndef _EVCHARGER_H
#define _EVCHARGER_H

#include "residential.h"

#define MAXDAYTYPES 2
#define WEEKDAY 0
#define WEEKEND 1

#define MAXEVENTS 2
#define DEPART 0
#define ARRIVE 1

typedef struct s_evdemand {
	char name[1024];
	unsigned short n_daytypes;
	double home[MAXDAYTYPES][MAXEVENTS][24];
	double work[MAXDAYTYPES][MAXEVENTS][24];
	double shorttrip[MAXDAYTYPES][MAXEVENTS][24];
	double longtrip[MAXDAYTYPES][MAXEVENTS][24];
} EVDEMAND;

class evcharger  
{
private:
	complex *pVoltage;			///< ref voltage the charge is getting
	EVDEMAND *pDemand;			///< ref demand profile for this vehicle
public:
	ENDUSELOAD load;			///< enduse load structure
	double heat_fraction;		///< fraction of the evcharger that is transferred as heat (default = 0.90)
	enum {
		LOW=0,						///< low power charger (120V/15A)
		MEDIUM=1,					///< med power charger (240V/30A)
		HIGH=2,						///< high power charger (240V/70A)
	} charger_type;				///< EV charger power
	enum {
		ELECTRIC=0,					///< vehicle is pure electric (no long trips)
		HYBRID=1,					///< vehicle is hybrid (long trip possible)
	} vehicle_type;				///< vehicle type
	enum {
		HOME=0,						///< vehicle is at home (charging is active)
		WORK=1,						///< vehicle is at work (charging is possible)
		SHORTTRIP=2,				///< vehicle is on short trip (<50 miles -- discharging to 25%)
		LONGTRIP=3,					///< vehicle is on long trip (>50 miles -- discharged to 25%)
	} state;					///< current state of EV
	struct {
		double home;				///< probability of coming home
		double work;				///< probability of leaving at work
		double shorttrip;			///< probability of leaving on a short trip
		double longtrip;			///< probability of leaving on a long trip
	} demand;					///< vehicle demand characteristics
	struct {
		double work;				///< distance to work
		double shorttrip;			///< typical distance for short trip
		double longtrip;			///< typical distance for long trip
	} distance;					///< trip distances
	bool charge_at_work;		///< allow charging at work
	double capacity;			///< maximum capacity of battery
	double charge;				///< current charge state of vehicle
	double mileage;				///< vehicle mileage (miles/kW)
	double power_factor;		///< charge power factor
	double charge_throttle;		///< charge throttle (0-1)
	char1024 demand_profile;	///< filename of demand profile

public:
	static CLASS *oclass;
	static evcharger *defaults;

	evcharger(MODULE *module);
	~evcharger();
	int create();
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	double update_state(double dt=0.0);
	void load_demand_profile();
};

#endif // _EVCHARGER_H

/**@}**/
