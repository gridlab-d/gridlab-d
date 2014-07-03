/** $Id: house_a.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file house.h
	@addtogroup house
	@ingroup residential
 
 @{
 **/

#ifndef _HOUSE_H
#define _HOUSE_H

#include "residential.h"
#include "residential_enduse.h"

typedef enum {duration=0,energy=1} TYPEOFEVENT;
typedef enum {analog=0, PWM=1, queued=2} TYPEOFPULSE;

#define ENDUSELOAD enduse

class house : public residential_enduse { /* inherits due to HVAC being an enduse */
public:
	/// Get voltage on a circuit
	/// @return voltage (or 0 if breaker is open)
	inline complex V(CIRCUIT *c) ///< pointer to circuit 
	{ return c->status==BRK_CLOSED ? *(c->pV) : complex(0,0);};
	complex *pCircuit_V; ///< pointer to the three voltages on three lines
	complex *pLine_I; ///< pointer to the three current on three lines
	complex *pLine12; ///< pointer to the load across lines 1 & 2

public:  //definitions
	typedef enum {	HEAT, ///< heating mode
					AUX, ///< supplemental heating
					COOL, ///< cooling mode
					OFF  ///< off
				}HCMODE; ///< heating/cooling mode
	typedef enum {
		ELECTRIC,	///< tank heats with an electric resistance element
		GASHEAT		///< tank heats with natural gas
	} HEATMODE;		///<

public:
	// published variables
	double floor_area;				///< house floor area (ft^2)
	double envelope_UA;				///< envelope UA (BTU.sq.ft/hr.ft2)
	double glazing_shgc;			///< glazing SHGC
	double window_wall_ratio;		///< window-wall ratio
	double gross_wall_area;			///< gross wall area (sq.ft)
	double ceiling_height;			///< ceiling height
	double aspect_ratio;			///< aspect ratio
	double thermostat_deadband;		///< thermostat deadband (degF)
	double airchange_per_hour;		///< house air changes per hour (ach)
	double heating_setpoint;		///< heating setpoint (degF)
	double cooling_setpoint;		///< cooling setpoint (degF)
	double solar_aperture[N_SOLAR_SURFACES];		///< Future: Solar aperture(WWR) by orientation
	double design_heating_capacity;	///< space heating capacity (BTUh/sf)
	double design_cooling_capacity;	///< space cooling capacity (BTUh/sf)
	double heating_COP;				///< space heating COP
	double cooling_COP;				///< space cooling COP
	double over_sizing_factor;		///< Future: equipment over sizing factor
	double outside_temp;

	double volume;					///< house air volume
	double air_mass;				///< mass of air (lbs)
	double air_thermal_mass;		///< thermal mass of air (BTU/F)
	double solar_load;				///< solar load (BTU/h)
	double house_content_heat_transfer_coeff; ///< mass UA
	double COP_coeff;				///< equipment cop coefficient (scalar)
	double air_density;				///< air density
	double air_heat_capacity;		///< heat capacity of air
	double house_content_thermal_mass; ///< house thermal mass (BTU/F)

	double *pTout;
	double *pRhout;
	double *pSolar;

	double Tair;
	double Tout;
	double Tsolar;
	double Tmaterials;

	enumeration heat_cool_mode;			///< heating cooling mode at t1
	enumeration heat_mode;				///< method of heating the water (gas or electric) [enum]
	double hvac_rated_power;
	double hvac_rated_capacity;
	double rated_heating_capacity;
	double rated_cooling_capacity;

	PANEL panel; ///< main house panel

//	double internal_gain;
//	double hvac_kWh_use;
	enduse tload; /* total load */
	//enduse load;	// inherited from res_enduse
	CIRCUIT *hvac_circuit;
private:
	double A11;
	double A12;
	double A21;
	double A22;

	double B11;
	double B12;
	double B21;
	double B22;

	double s1;
	double s2;

	double M_inv11;
	double M_inv12;
	double M_inv21;
	double M_inv22;

	double BB11;
	double BB21;
	double BB12;
	double BB22;

public:
	static CLASS *oclass, *pclass;
	house( MODULE *module);
	~house();

	int create();
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_hvac_load(TIMESTAMP t1, double nHours);
	TIMESTAMP sync_thermal(TIMESTAMP t1, double nHours);
	TIMESTAMP sync_billing(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_thermostat(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_panel(TIMESTAMP t0, TIMESTAMP t1);

	int init(OBJECT *parent);
	int init_climate(void);

	CIRCUIT *attach(ENDUSELOAD *target, double limit, int is220=false);

// access methods
public:
	double get_floor_area () { return floor_area; };
	double get_Tout () { return *pTout; };
	double get_Tair () { return Tair; };
	double get_Tsolar(int hour, int month, double Tair, double Tout);
	int set_Eigen_values();
	complex *get_complex(OBJECT *obj, char *name);
};

#endif

/**@}**/
