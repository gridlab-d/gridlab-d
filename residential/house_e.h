/** $Id: house_e.h,v 1.44 2008/02/12 06:14:47 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file house_e.h
	@addtogroup house_e
	@ingroup residential

 @{
 **/

#ifndef _HOUSE_E_H
#define _HOUSE_E_H

#include "residential.h"
#include "complex.h"
#include "enduse.h"
#include "loadshape.h"

class house_e {
public:
	PANEL panel; ///< main house_e panel
	/// Get voltage on a circuit
	/// @return voltage (or 0 if breaker is open)
	inline complex V(CIRCUIT *c) ///< pointer to circuit 
	{ return c->status==BRK_CLOSED ? *(c->pV) : complex(0,0);};
	complex *pCircuit_V; ///< pointer to the three voltages on three lines
	complex *pLine_I; ///< pointer to the three current on three lines
	complex *pLine12; ///< pointer to the load across lines 1 & 2

public:
	// building design variables
	double floor_area;				///< house_e floor area (ft^2)
	double envelope_UA;				///< envelope UA (BTU.sq.ft/hr.ft2)
	double glazing_shgc;			///< glazing SHGC
	double window_wall_ratio;		///< window-wall ratio
	double gross_wall_area;			///< gross wall area (sq.ft)
	double ceiling_height;			///< ceiling height
	double aspect_ratio;			///< building footprint aspect ratio
	double solar_aperture[N_SOLAR_SURFACES];		///< Future: Solar aperture(WWR) by orientation
	double house_content_heat_transfer_coeff; ///< mass UA
	double COP_coeff;				///< equipment cop coefficient (scalar)
	double air_density;				///< air density
	double air_heat_capacity;		///< heat capacity of air
	double house_content_thermal_mass; ///< house thermal mass (BTU/F)

	// system design variables
	double thermostat_deadband;		///< thermostat deadband (degF)
	double airchange_per_hour;		///< house_e air changes per hour (ach)
	double heating_setpoint;		///< heating setpoint (degF)
	double cooling_setpoint;		///< cooling setpoint (degF)
	double design_heating_capacity;	///< space heating capacity (BTUh/sf)
	double design_cooling_capacity;	///< space cooling capacity (BTUh/sf)
	double heating_COP;				///< space heating COP
	double cooling_COP;				///< space cooling COP
	double over_sizing_factor;		///< Future: equipment over sizing factor
	double rated_heating_capacity;	///< reated heating capacity of the system (BTUh/sf; varies w.r.t Tout),
	double rated_cooling_capacity;	///< rated cooling capacity of the system (BTUh/sf; varies w.r.t Tout)

	typedef enum {
		ST_GAS	= 0x01,	///< flag to indicate gas heating is used
		ST_AC	= 0x02,	///< flag to indicate the air-conditioning is used
		ST_AIR	= 0x04,	///< flag to indicate central air is used
		ST_VAR	= 0x08,	///< flag to indicate the variable speed system is used
	} SYSTEMTYPE; ///< flags for system type options
	set system_type;				///< system type

	// derived/calculated variables
	double volume;					///< house_e air volume
	double air_mass;				///< mass of air (lbs)
	double air_thermal_mass;		///< thermal mass of air (BTU/F)
	double solar_load;				///< solar load (BTU/h)
	double cooling_design_temperature, heating_design_temperature, design_peak_solar, design_internal_gains;

	/* enduse loads */
#define IEU_CLOTHESWASHER	0x0001	///< flag to indicate clotheswasher enduse is implicitly defined
#define IEU_DISHWASHER		0x0002	///< flag to indicate dishwasher enduse is implicitly defined
#define IEU_DRYER			0x0004	///< flag to indicate dryer enduse is implicitly defined
#define IEU_EVCHARGER		0x0008	///< flag to indicate eletric vehicle charger enduse is implicitly defined
#define IEU_FREEZER			0x0010	///< flag to indicate freezer enduse is implicitly defined
#define IEU_LIGHTS			0x0020	///< flag to indicate lights enduse is implicitly defined
#define IEU_MICROWAVE		0x0040	///< flag to indicate microwave enduse is implicitly defined
#define IEU_PLUGS			0x0080	///< flag to indicate plugs enduse is implicitly defined
#define IEU_RANGE			0x0100	///< flag to indicate range enduse is implicitly defined
#define IEU_REFRIGERATOR	0x0200	///< flag to indicate refrigerator enduse is implicitly defined
#define IEU_WATERHEATER		0x0400	///< flag to indicate waterheater enduse is implicitly defined
	/* TODO add more enduses here */
	set implicit_enduses;	///< flags to indicate which enduses are implicitly defined

	double Rroof, Rwall, Rfloor, Rwindows;

	double *pTout;	// pointer to outdoor temperature (see climate)
	double *pRhout;	// pointer to outdoor humidity (see climate)
	double *pSolar;	// pointer to solar radiation array (see climate)

	// electric enduse loads
	enduse clotheswasher;
	enduse dishwasher;
	enduse dryer;
	enduse evcharger;
	enduse freezer;
	enduse lights;
	enduse microwave;
	enduse plugs;
	enduse range;
	enduse refrigerator;
	enduse waterheater;

	// no electric load enduse gains
	enduse occupants;

	double Tair;
	double Tmaterials;
	double outside_temperature;

	typedef enum {	
		SM_UNKNOWN	=0,			///< unknown mode
		SM_OFF		=1,			///< off
		SM_HEAT		=2,			///< heating mode
		SM_AUX		=3,			///< supplemental heating
		SM_COOL		=4,			///< cooling mode
	} SYSTEMMODE;				///< system mode
	SYSTEMMODE system_mode;			///< system mode at t1
	double system_rated_power;		///< rated power of the system
	double system_rated_capacity;	///< rated capacity of the system

	enduse system;  /* hvac system load */
	enduse total; /* total load */

private:

	// internal variables used to track state of house */
	double dTair;
	double c1,c2,c3,c4,c5,c6,c7,r1,r2,k1,k2,Teq, Tevent;
	static bool warn_control;
	static double warn_low_temp, warn_high_temp;
	bool check_start;

public:
	static CLASS *oclass;
	static house_e *defaults;
	house_e( MODULE *module);
	~house_e();

	int create();
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_billing(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_thermostat(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_panel(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_enduses(TIMESTAMP t0, TIMESTAMP t1);
	void update_system(double dt=0);
	void update_model(double dt=0);
	void check_controls(void);

	int init(OBJECT *parent);
	int init_climate(void);

	CIRCUIT *attach(OBJECT *obj, double limit, int is220=false, enduse *pEnduse=NULL);
	void attach_implicit_enduses(void);

// access methods
public:
	inline double get_floor_area () { return floor_area; };
	inline double get_Tout () { return *pTout; };
	inline double get_Tair () { return Tair; };

	complex *get_complex(OBJECT *obj, char *name);
};

inline double sgn(double x) 
{
	if (x<0) return -1;
	if (x>0) return 1;
	return 0;
}

#endif

/**@}**/
