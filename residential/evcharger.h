/** $Id: evcharger.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file evcharger.h
	@addtogroup evcharger
	@ingroup residential

 @{
 **/

#ifndef _EVCHARGER_H
#define _EVCHARGER_H

#include "residential.h"
#include "residential_enduse.h"

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

class evcharger : public residential_enduse
{
	typedef enum {
		VS_UNKNOWN=-1,				///< vehicle state is unknown
		VS_HOME=0,					///< vehicle is at home (charging is active)
		VS_WORK=1,					///< vehicle is at work (charging is possible)
	// these are not yet supported
	//	VS_SHORTTRIP=2,				///< vehicle is on short trip (<50 miles -- discharging to 25%)
	//	VS_LONGTRIP=3,				///< vehicle is on long trip (>50 miles -- discharged to 25%)
	} VEHICLESTATE;
	typedef enum {
		CT_LOW=0,					///< low power charger (120V/15A)
		CT_MEDIUM=1,				///< med power charger (240V/30A)
		CT_HIGH=2,					///< high power charger (240V/70A)
	} CHARGERTYPE;
	typedef enum {
		VT_ELECTRIC=0,				///< vehicle is pure electric (no long trips)
		VT_HYBRID=1,				///< vehicle is hybrid (long trip possible)
	} VEHICLETYPE;
private:
	EVDEMAND *pDemand;				///< ref demand profile for this vehicle

public:
	double heat_fraction;			///< fraction of the evcharger that is transferred as heat (default = 0.90)
	enumeration charger_type;		///< EV charger power
	enumeration vehicle_type;		///< vehicle type
	enumeration vehicle_state;		///< current state of EV
	double charging_efficiency;		///< Efficiency of input power to battery power
	struct {
		double home;				///< probability of coming home
		double work;				///< probability of leaving at work
	// these are not yet supported
	//	double shorttrip;			///< probability of leaving on a short trip
	//	double longtrip;			///< probability of leaving on a long trip
	} demand;					///< vehicle demand characteristics
	struct {
		double work;				///< distance to work
	// these are not yet supported
	//	double shorttrip;			///< typical distance for short trip
	//	double longtrip;			///< typical distance for long trip
	} distance;					///< trip distances
	bool charge_at_work;		///< allow charging at work
	double capacity;			///< maximum capacity of battery
	double charge;				///< current charge state of vehicle
	double mileage;				///< vehicle mileage (miles/kW)
	double power_factor;		///< charge power factor
	double charge_throttle;		///< charge throttle (0-1)
	double mileage_classification;	///< Mileage classification of PHEV - e.g. 33 for a PHEV33 (33 miles on electric)
	char1024 demand_profile;	///< filename of demand profile

public:
	static CLASS *oclass, *pclass;
	static evcharger *defaults;

	evcharger(MODULE *module);
	~evcharger();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	double update_state(double dt=0.0);
	void load_demand_profile();
};

#endif // _EVCHARGER_H

/**@}**/
