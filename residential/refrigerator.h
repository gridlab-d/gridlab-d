/** $Id: refrigerator.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file refrigerator.h
	@addtogroup refrigerator
	@ingroup residential

 @{
 **/

#ifndef _REFRIGERATOR_H
#define _REFRIGERATOR_H

#include "residential.h"
#include "residential_enduse.h"

class refrigerator : public residential_enduse {
private:
	double *pTout;
public:
	
	typedef enum e_state {	
			RS_DEFROST=1,			
			RS_COMPRESSSOR_OFF_NORMAL=2,		
			RS_COMPRESSSOR_ON_NORMAL=3,		
			RS_COMPRESSSOR_ON_LONG=4,
	} REFRIGERATOR_STATE;	


public:
	double size;  ///< refrigerator volume (cf) 
	double rated_capacity;  ///< rated capacity (Btu/h)
	double thermostat_deadband;  ///< refrigerator thermostat hysterisys (degF)
	double UA;
	double UAr;		///< UA of Refrigerator compartment
	double UAf;		///< UA of the food-air
	double Tair;	///< Refirgerator air temperature (degF)
	double Tout;	///< House air temperature
	double Tset;	///< Refrigerator control set point temperature (degF)
	double Cf;		///< heat capapcity of the food
	double Qr;		///< heat rate from the cooling system
	double COPcoef;	///< compressor COP
	double energy_used;	
	double energy_needed;	
	double cycle_time;
	double total_power;	
	bool is_240;
	//bool new_running_state;
	bool initial_state;
	bool check_icemaking;
	double ice_making_no;
	double ice_making;
	int32 return_time;
	int door_return_time;
	TIMESTAMP start_time;

	bool icemaker_running;
	int32 check_DO;
	int32 DO_random_opening;
	
	int32 FF_Door_Openings;
	int DO_Thershold;
	int last_dr_mode;
	double defrostDelayed;
	double compressor_defrost_time;
	
	bool long_compressor_cycle_due;
	double long_compressor_cycle_time;
	double long_compressor_cycle_power;
	double long_compressor_cycle_energy;
	double long_compressor_cycle_threshold;
	
	double refrigerator_power;
	bool run_defrost;
	bool check_defrost;
	int32 no_of_defrost;

	double compressor_off_normal_energy;
	double compressor_off_normal_power; 
	double compressor_on_normal_energy; 
	double compressor_on_normal_power; 
	double defrost_energy; 
	double defrost_power; 
	double icemaking_energy; 
	double icemaking_power; 	
	
	double sweatheater_power;
	double delay_defrost_time;

	double ice_making_probability;
	int hourly_door_opening;
	int door_open_time;
	int door_next_open_time;
	int door_time;
	double door_opening_criterion;
	int daily_door_opening;
	double door_opening_power;
	double door_opening_energy;
	int *next_door_openings;
	bool door_open;
	bool door_to_open;
	bool door_energy_calc;
	int door_array_size;
	double total_compressor_time;

	bool new_running_state;
	
	double Tevent;	///< Temperature we will switch the motor on or off.  Available for SmartGrid PLC code to nudge.

	typedef enum {  DM_UNKNOWN=0,						///< mode is unknown
			DM_LOW=1,							///< low signal
			DM_NORMAL=2,						///< normal signal
			DM_HIGH=3,							///< high demand response signal
			DM_CRITICAL=4,						///< critical demand response signal 
	};
	enumeration dr_mode;

	typedef enum {  
			DC_TIMED=1,						///< For the timed option
			DC_DOOR_OPENINGS=2,				///< For the door openings option
			DC_COMPRESSOR_TIME=3,				///< For the compressor time option		
	};
	enumeration defrost_criterion;

	double dr_mode_double; ///temporary variable for testing

	enumeration state;
	TIMESTAMP last_time, next_time;
	double* ice_making_time; 

public:

	static CLASS *oclass, *pclass;
	refrigerator(MODULE *module);
	~refrigerator();
	
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	void thermostat(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	double update_refrigerator_state(double dt, TIMESTAMP t1);		
};

#endif // _REFRIGERATOR_H

/**@}**/
