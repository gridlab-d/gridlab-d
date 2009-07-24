/** $Id: house_e.h,v 1.44 2008/02/12 06:14:47 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file house_e.h
	@addtogroup house_e
	@ingroup residential

 @{
 **/

#ifndef _HOUSE_E_H
#define _HOUSE_E_H

#include "house_a.h"
#include "residential.h"
#include "complex.h"
#include "enduse.h"
#include "loadshape.h"

#define LST_DIRECT		0x00 ///< shape is absolute direct shape - scalar is multiplier
#define LST_NORMAL		0x01 ///< shape is given in normalized values (pu) - scalar is kW
#define LST_RELATIVE	0x02 ///< shape is given in relative values (/sf) - scalar is multiplier of floor_area
#define LST_SEASONAL	0x10 ///< shape has seasonal component (winter/summer)
#define LST_WEEKDAY		0x20 ///< shape has weekday component (weekend/weekday)

#define LS_SINGLE	0	///< single shape (no seasons, no daytypes)
#define LS_WINTER	0	///< winter shape (no daytypes)
#define LS_SUMMER	1	///< summer shape (no daytypes)
#define LS_WEEKDAY	0	///< weekday shape (no seasons)
#define LS_WEEKEND	1	///< weekend shape (no seasons)
#define LS_WINTERWEEKDAY 0	///< winter weekday shape
#define LS_WINTERWEEKEND 1	///< winter weekend shape
#define LS_SUMMERWEEKDAY 2	///< summer weekday shape
#define LS_SUMMERWEEKEND 3	///< summer weekend shape

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

public:  //definitions
	typedef enum {	
		HC_UNKNOWN=0, ///< unknown mode
		HC_OFF=1,  ///< off
		HC_HEAT=2, ///< heating mode
		HC_AUX=3, ///< supplemental heating
		HC_COOL=4, ///< cooling mode
	} HCMODE; ///< heating/cooling mode
	typedef enum {
		HT_UNKNOWN=0,
		HT_ELECTRIC=1,	///< tank heats with an electric resistance element
		HT_GASHEAT=2,	///< tank heats with natural gas
	} HEATTYPE;		///<
	 

public:
	// published variables
	double floor_area;				///< house_e floor area (ft^2)
	double envelope_UA;				///< envelope UA (BTU.sq.ft/hr.ft2)
	double glazing_shgc;			///< glazing SHGC
	double window_wall_ratio;		///< window-wall ratio
	double gross_wall_area;			///< gross wall area (sq.ft)
	double ceiling_height;			///< ceiling height
	double aspect_ratio;			///< aspect ratio
	double thermostat_deadband;		///< thermostat deadband (degF)
	double airchange_per_hour;		///< house_e air changes per hour (ach)
	double heating_setpoint;		///< heating setpoint (degF)
	double cooling_setpoint;		///< cooling setpoint (degF)
	double solar_aperture[N_SOLAR_SURFACES];		///< Future: Solar aperture(WWR) by orientation
	double design_heating_capacity;	///< space heating capacity (BTUh/sf)
	double design_cooling_capacity;	///< space cooling capacity (BTUh/sf)
	double heating_COP;				///< space heating COP
	double cooling_COP;				///< space cooling COP
	double over_sizing_factor;		///< Future: equipment over sizing factor

	double volume;					///< house_e air volume
	double air_mass;				///< mass of air (lbs)
	double air_thermal_mass;		///< thermal mass of air (BTU/F)
	double solar_load;				///< solar load (BTU/h)
	double house_content_heat_transfer_coeff; ///< mass UA
	double COP_coeff;				///< equipment cop coefficient (scalar)
	double air_density;				///< air density
	double air_heat_capacity;		///< heat capacity of air
	double house_content_thermal_mass; ///< house thermal mass (BTU/F)

	double cooling_design_temperature, heating_design_temperature, design_peak_solar, design_internal_gains;
	char256 implicit_enduses;		///< list of implicit end-use loads to enable (list will be parsed to attach LOADSHAPES)

	double Rroof, Rwall, Rfloor, Rwindows;

	double *pTout;
	double vTout;
	double *pRhout;
	double *pSolar;

	enduse clotheswasher;
	loadshape clotheswasher_shape;
	enduse dishwasher;
	loadshape dishwasher_shape;
	enduse dryer;
	loadshape dryer_shape;
	enduse freezer;
	loadshape freezer_shape;
	enduse light;
	loadshape light_shape;
	enduse microwave;
	loadshape microwave_shape;
	enduse occupants;
	loadshape occupants_shape;
	enduse plugs;
	loadshape plugs_shape;
	enduse refrigerator;
	loadshape refrigerator_shape;
	enduse waterheater;
	loadshape waterheater_shape;

	double Tair;
	double Tsolar;
	double Tmaterials;
	double outside_temp;

	HCMODE heat_cool_mode;			///< heating cooling mode at t1
	HEATTYPE heat_type;				///< method of heating the water (gas or electric) [enum]
	double hvac_rated_power;
	double hvac_rated_capacity;
	double rated_heating_capacity;
	double rated_cooling_capacity;

	double hvac_kWh_use;
	ENDUSELOAD HVAC_load;  /* hvac load */
	ENDUSELOAD tload; /* total load */

	/* implicit end-uses */
	LOADSHAPE *implicit_loads;
		
private:

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
	void update_hvac(double dt=0);
	void update_model(double dt=0);
	void check_controls(void);

	int init(OBJECT *parent);
	int init_climate(void);

	CIRCUIT *attach(OBJECT *obj, double limit, int is220=false, ENDUSELOAD *pLoad=NULL, LOADSHAPE *implicit_end_use=NULL);
	void attach_implicit_enduses(char *enduses=NULL);

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
