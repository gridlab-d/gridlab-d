/** $Id: range.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file range.h
	@addtogroup range
	@ingroup residential

 @{
 **/
#ifndef _range_H
#define _range_H

#include "residential.h"
#include "residential_enduse.h"

class range : public residential_enduse {
private:
	double standby_load;	///< typical power loss through thermal jacket losses (UA 2, 60 to 140 degF, 160 BTU/hr, 47W, 411kWh/year, ~10% energy star guesstimate)
public:

		typedef enum e_state {	
						
			CT_STOPPED=1,				///<cooktop is stopped
			CT_STAGE_1_ONLY=2,			///<cooktop is running with stage 1 settings
			CT_STAGE_2_ONLY=3,			///<cooktop is running with stage 2 settings
			CT_STAGE_3_ONLY=4,			///<cooktop is running with stage 3 settings
			CT_STALLED=5,				///<cooktop is stalled
			CT_TRIPPED=6,				///<cooktop is tripped
	} STATE;							///<control state



	typedef enum {
		ONENODE,	///< range model uses a single zone
		NONE,		///< range model zoning isn't defined
	} WHMODEL;	///< range model currently in use
	typedef enum {
		DEPLETING,	///< heat in the oven is dropping fast
		RECOVERING, ///< heat in the is rising fast
		STABLE,		///< heat in the relatively stable
	} WHQFLOW;		///< heat flow in the oven
	typedef enum {
		INSIDE,		///< oven located in conditioned space
		GARAGE,		///< oven located in unconditioned space
	} WHLOCATION;
	typedef enum {
		FULL,		///< heat in the oven is full
		PARTIAL,	///< heat in the oven is partial
		EMPTY,		///< heat in the oven is empty
	} WHQSTATE; ///<
	typedef enum {
		ELECTRIC,	///< oven heats with an electric resistance element
		GASHEAT		///< oven heats with natural gas
	} HEATMODE;		///<

	// One of our main return values...
	double time_to_transition;		///< time until next transition [in seconds]

	// Basic characteristics defined at creation...
	double Tset_curtail;			///< lower limit before we cancel curtailment [F]
	double Tinlet;					///< default will be set to 60 degF
	enumeration location;			///< location of oven (inside or garage) [enum]
	enumeration heat_mode;				///< method of heating the food (gas or electric) [enum]

	// Characteristics calculated from basics at creation...
	double area;					///< oven cross-sectional area [ft^2]
	double height;					///< oven height [ft]
	double Ton;						///< cut-in temperature [F]
	double Toff;					///< cut-out temperature [F]
	double Cw;						///< thermal mass of the the food item [Btu/F]

	double dt1;
	double dt2;

	double enduse_demand_cooktop;	///< amount of demand added per hour (units/hr)
	        
	double heat_fraction;		    ///< fraction of the plugload that is transferred as heat (default = 0.90)
	
	double cooktop_energy_used;		///<amount of energy used in current cycle
	double cycle_duration_cooktop;
	double cycle_time_cooktop;
	double cooktop_interval[3];     ///<length of time of each setting section for cooktop operation
	double total_time;				///<tootal cooktop operating time
	double cooktop_coil_power[3];   ///<installed heating coil power [W]
	double enduse_queue_cooktop;	///< accumulated demand (units)

	double queue_min;               ///<minimum accumulated demand
	double queue_max;               ///<maximum accumulated demand
	double cooktop_run_prob;		///<probability of turning on cooktop

	double time_cooktop_operation;
	double time_cooktop_setting;
	bool cooktop_check;
	

	double total_power_oven;        ///<total power usage in oven
	double total_power_cooktop;		///<total power usage in cooktop
	double total_power_range;		///<total power usage in range

	double state_time;					///< remaining time in current state (s)

	double stall_voltage;				///< voltage at which the motor stalls
	double start_voltage;				///< voltage at which motor can start
	complex stall_impedance;			///< impedance of motor when stalled
	double trip_delay;					///< stalled time before thermal trip
	double reset_delay;					///< trip time before thermal reset and restart
	double cooktop_energy_baseline;     ///<amount of energy needed to 
	double cooktop_energy_needed;       ///<total energy needed to cook or warm up food with cooktop

	bool cooktop_coil_1_check;			///<logic check to see if coil 1 is activated
	bool cooktop_coil_2_check;			///<logic check to see if coil 2 is activated
	bool oven_check;
	bool remainon;

	enumeration state_cooktop;

	double TSTAT_PRECISION;

	// The primary values we compute in our simultation...
	double h;						///< boundary between hot and cold the food layers [ft from top of the container]
	double Tlower;					///< temperature in lower zone of the oven (for 2-zone model) [F]
	double Tlower_old;
	double Tupper;					///< temperature in upper zone of the oven (for 2-zone model) [F]
	double Tupper_old;
	double Tw;						///< oven temperature [F]
	double Tw_old;					///< previous oven temperature, for internal_gains

	// Convenience values (some pre-computed here and there for efficiency)...
	bool heat_needed;				///< need to maintain this bit of state because of Tstat deadband...
	double is_range_on;		///< Simple logic for determining state of range 1-on, 0-off
public:
	double oven_volume;					///< oven size [gal]
	double oven_UA;						///< oven UA [BTU/hr-F]
	double oven_diameter;				///< oven diameter [ft]
	double oven_demand;				///< food draw rate [gpm]
	double oven_demand_old;			///< previous food demand, needed for temperature change (reflects heat loss from hot the food draw)
	double heating_element_capacity;	///< rated Q of (each) heating element, input in W, converted to[Btu/hr]
	double oven_setpoint;				///< setpoint T of heating element [F]
	double thermostat_deadband;			///< deadband around Tset (half above, half below) [F]
	double *pTair;
	double *pTout;
	double specificheat_food;
	double food_density;
	double enduse_queue_oven;
	double oven_run_prob;
	double enduse_demand_oven;

	double time_oven_setting;
	double time_oven_operation;

	double actual_load;
	double prev_load;
	complex range_actual_power;	///< the actual power draw of the object after accounting for voltage
	

public:
	static CLASS *oclass, *pclass;
	static range *defaults;
	
	range(MODULE *mod);
	~range(void);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	void thermostat(TIMESTAMP t0, TIMESTAMP t1);					// Thermostat plc control code - determines whether to heat...
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	int commit();


public:
	enumeration current_model;				///< Discerns which range model we need to use
	enumeration load_state;					///

	enumeration range_state(void);				// Are we full, partial, or empty?
	void set_time_to_transition(void);					//< Sets timeToTransition...
	enumeration set_current_model_and_load_state(void);	// set the model and state for each cycle
	void update_T_and_or_h(double);				// Reset Tw and or h...
	double update_state(double,TIMESTAMP);

	double dhdt(double h);								// Calculates dh/dt...
	double actual_kW(void);								// Actual heat from heating element...
	double new_time_1node(double T0, double T1);		// Calcs time to transition...
	double new_temp_1node(double T0, double delta_t);	// Calcs temp after transition...
	
	double get_Tambient(enumeration water_heater_location);		// ambient T [F] -- either an indoor house temperature or a garage temperature, probably...
	typedef enum {MODEL_NOT_1ZONE=0, MODEL_NOT_2ZONE=1} WRONGMODEL;
	void wrong_model(enumeration msg);
};

#endif

/** $Id: range.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2011 Battelle Memorial Institute
	@file range.h
	@addtogroup range
	@ingroup residential

 @{
 **/
/**@}**/
