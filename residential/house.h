/** $Id: house.h,v 1.44 2008/02/12 06:14:47 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file house.h
	@addtogroup house
	@ingroup residential

 @{
 **/

#ifndef _HOUSE_H
#define _HOUSE_H

#include "residential.h"

typedef enum {	BRK_OPEN=0,		///< breaker open
				BRK_CLOSED=1,	///< breaker closed
				BRK_FAULT=-1,	///< breaker faulted
} BREAKERSTATUS; ///< breaker state
typedef enum {	X12=0,	///< circuit from line 1 to line 2    (240V)
				X23=1,	///< circuit from line 2 to line 3(N) (120V)
				X13=2,	///< circuit from line 1 to line 3(N) (120V)
} CIRCUITTYPE; ///< circuit type

typedef struct s_load {
	/* NOTE: total must be first and must be published as "enduse_load[kW]" */
	complex total;		///< total load at voltage given (kW)
	complex power;		///< constant power load (kW)
	complex current;	///< constant current load (A)
	complex admittance;	///< constant admittance load (1/Ohm)
	double energy;		///< energy usage (accumulated kWh)
	double heatgain;	///< internal heat gain rate (kW)
} ENDUSELOAD;	///< End-use load struct that must be included in end-use for circuits to read load

typedef struct s_circuit {
	CIRCUITTYPE type;	///< circuit type
	ENDUSELOAD *pLoad;	///< pointer to the load struct
	complex *pV; ///< pointer to circuit voltage
	double max_amps; ///< maximum breaker amps
	int id; ///< circuit id
	BREAKERSTATUS status; ///< breaker status
	TIMESTAMP reclose; ///< time at which breaker is reclosed
	unsigned short tripsleft; ///< the number of trips left before breaker faults
	OBJECT *enduse; ///< the enduse that is using this circuit (must use ENDUSELOAD struct)
	struct s_circuit *next; ///< next circuit in list
} CIRCUIT; ///< circuit definition
typedef struct s_panel {
	double max_amps; ///< maximum panel amps
	BREAKERSTATUS status; ///< panel breaker status
	TIMESTAMP reclose; ///< time at which breaker is reclosed
	CIRCUIT *circuits; ///< pointer to first circuit in circuit list
} PANEL; ///< panel definition

#define N_SOLAR_SURFACES 9
typedef enum {HORIZONTAL, NORTH, NORTH_EAST, EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, WEST, NORTH_WEST} ORIENTATION;

class house {
private:
	PANEL panel; ///< main house panel
	/// Get voltage on a circuit
	/// @return voltage (or 0 if breaker is open)
	inline complex V(CIRCUIT *c) ///< pointer to circuit 
	{ return c->status==BRK_CLOSED ? *(c->pV) : complex(0,0);};
	complex *pCircuit_V; ///< pointer to the three voltages on three lines
	complex *pLine_I; ///< pointer to the three current on three lines

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

	double volume;					///< house air volume
	double air_mass;				///< mass of air (lbs)
	double air_thermal_mass;		///< thermal mass of air (BTU/F)
	double solar_load;				///< solar load (BTU/h)
	double house_content_heat_transfer_coeff; ///< mass UA
	double COP_coeff;				///< equipment cop coefficient (scalar)
	double air_density;				///< air density
	double air_heat_capacity;		///< heat capacity of air
	double house_content_thermal_mass; ///< house thermal mass (BTU/F)

	double cooling_design_temperature, heating_design_temperature, design_peak_solar, design_internal_gains;

	double Rroof, Rwall, Rfloor, Rwindows;

	double *pTout;
	double vTout;
	double *pRhout;
	double *pSolar;

	double Tair;
	double Tsolar;
	double Tmaterials;
		
	HCMODE heat_cool_mode;			///< heating cooling mode at t1
	HEATTYPE heat_type;				///< method of heating the water (gas or electric) [enum]
	double hvac_rated_power;
	double hvac_rated_capacity;
	double rated_heating_capacity;
	double rated_cooling_capacity;

	double hvac_kWh_use;
	ENDUSELOAD load;  /* hvac load */
	ENDUSELOAD tload; /* total load */

private:

	double dTair;
	double c1,c2,c3,c4,c5,c6,c7,r1,r2,k1,k2,Teq, Tevent;
	static bool warn_control;
	static double warn_low_temp, warn_high_temp;

public:
	static CLASS *oclass;
	static house *defaults;
	house( MODULE *module);
	~house();

	int create();
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_billing(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_thermostat(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync_panel(TIMESTAMP t0, TIMESTAMP t1);
	void update_hvac(double dt=0);
	void update_model(double dt=0);
	void check_controls(void);

	int init(OBJECT *parent);
	int init_climate(void);

	CIRCUIT *attach(OBJECT *obj, double limit, int is220=false);

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
