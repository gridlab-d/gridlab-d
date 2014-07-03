/** $Id: waterheater.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file waterheater.h
	@addtogroup waterheater
	@ingroup residential

 @{
 **/

#ifndef _WATERHEATER_H
#define _WATERHEATER_H

#include "residential.h"
#include "residential_enduse.h"

class waterheater : public residential_enduse {
private:
	double standby_load;	///< typical power loss through thermal jacket losses (UA 2, 60 to 140 degF, 160 BTU/hr, 47W, 411kWh/year, ~10% energy star guesstimate)
public:
	typedef enum {
		ONENODE,	///< tank model uses a single zone
		TWONODE,	///< tank model uses two zones
		NONE,		///< tank model zoning isn't defined
	} WHMODEL;	///< tank model currently in use
	typedef enum {
		DEPLETING,	///< tank heat is dropping fast
		RECOVERING, ///< tank heat is rising fast
		STABLE,		///< tank heat is relatively stable
	} WHQFLOW;		///< tank heat flow
	typedef enum {
		INSIDE,		///< tank located in conditioned space
		GARAGE,		///< tank located in unconditioned space
	} WHLOCATION;
	typedef enum {
		FULL,		///< tank heat is full
		PARTIAL,	///< tank heat is partial
		EMPTY,		///< tank heat is empty
	} WHQSTATE; ///<
	typedef enum {
		ELECTRIC,	///< tank heats with an electric resistance element
		GASHEAT		///< tank heats with natural gas
	} HEATMODE;		///<

	// One of our main return values...
	double time_to_transition;		///< time until next transition [in seconds]

	// Basic characteristics defined at creation...
	double Tset_curtail;			///< lower limit before we cancel curtailment [F]
	double Tinlet;					///< default will be set to 60 degF
	enumeration location;			///< location of tank (inside or garage) [enum]
	enumeration heat_mode;				///< method of heating the water (gas or electric) [enum]

	// Characteristics calculated from basics at creation...
	double area;					///< tank cross-sectional area [ft^2]
	double height;					///< tank height [ft]
	double Ton;						///< cut-in temperature [F]
	double Toff;					///< cut-out temperature [F]
	double Cw;						///< thermal mass of water [Btu/F]

	// The primary values we compute in our simultation...
	double h;						///< boundary between hot and cold water layers [ft from top of tank]
	double Tlower;					///< temperature in lower zone of tank (for 2-zone model) [F]
	double Tlower_old;
	double Tupper;					///< temperature in upper zone of tank (for 2-zone model) [F]
	double Tupper_old;
	double Twater;					///< temperature of whole tank (for 1-node model) [F]
	double Tw;						///< water temperature [F]
	double Tw_old;					///< previous water temperature, for internal_gains

	// Convenience values (some pre-computed here and there for efficiency)...
	bool heat_needed;				///< need to maintain this bit of state because of Tstat deadband...
	double is_waterheater_on;		///< Simple logic for determining state of waterheater 1-on, 0-off
public:
	double tank_volume;					///< tank size [gal]
	double tank_UA;						///< tank UA [BTU/hr-F]
	double tank_diameter;				///< tank diameter [ft]
	double water_demand;				///< water draw rate [gpm]
	double water_demand_old;			///< previous water demand, needed for temperature change (reflects heat loss from hot water draw)
	double heating_element_capacity;	///< rated Q of (each) heating element, input in W, converted to[Btu/hr]
	double tank_setpoint;				///< setpoint T of heating element [F]
	double thermostat_deadband;			///< deadband around Tset (half above, half below) [F]
	double *pTair;
	double *pTout;

	double gas_fan_power;		///< fan power draw when a gas waterheater is burning fuel
	double gas_standby_power;	///< standby power draw when a gas waterheater is NOT burning fuel

	double actual_load;
	double prev_load;
	complex waterheater_actual_power;	///< the actual power draw of the object after accounting for voltage
	

public:
	static CLASS *oclass, *pclass;
	static waterheater *defaults;
	
	waterheater(MODULE *mod);
	~waterheater(void);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	void thermostat(TIMESTAMP t0, TIMESTAMP t1);					// Thermostat plc control code - determines whether to heat...
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP commit();


public:
	enumeration current_model;				///< Discerns which water heater model we need to use
	enumeration load_state;					///< Are we filling or draining the tank [enum]

	enumeration tank_state(void);				// Are we full, partial, or empty?
	void set_time_to_transition(void);					//< Sets timeToTransition...
	enumeration set_current_model_and_load_state(void);	// set the model and state for each cycle
	void update_T_and_or_h(double);						// Reset Tw and or h...

	double dhdt(double h);								// Calculates dh/dt...
	double actual_kW(void);								// Actual heat from heating element...
	double new_time_1node(double T0, double T1);		// Calcs time to transition...
	double new_temp_1node(double T0, double delta_t);	// Calcs temp after transition...
	double new_time_2zone(double h0, double h1);		// Calcs time to transition...
	double new_h_2zone(double h0, double delta_t);      // Calcs h after transition...

	double get_Tambient(enumeration water_heater_location);		// ambient T [F] -- either an indoor house temperature or a garage temperature, probably...
	typedef enum {MODEL_NOT_1ZONE=0, MODEL_NOT_2ZONE=1} WRONGMODEL;
	void wrong_model(WRONGMODEL msg);
};

#endif

/**@}**/
