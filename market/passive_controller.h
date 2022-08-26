/** $Id: passive_controller.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file passive_controller.h
	@addtogroup passive_controller
	@ingroup market

 **/

#ifndef _passive_controller_H
#define _passive_controller_H

#include <stdarg.h>

#include "auction.h"
#include "gridlabd.h"

class passive_controller : public gld_object {
public:
	passive_controller(MODULE *);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
public:
	// control mode
	typedef enum {
		CM_NONE=0,
		CM_RAMP=1,
		CM_DUTYCYCLE=4,
		CM_PROBOFF=5,	// probabilistic shut-off
		CM_ELASTICITY_MODEL=6, //For the elasticity model to work
		CM_DLC=7,
		CM_PFC=8,
	} CONTROLMODE;
	enumeration control_mode;
	typedef enum {
		DLC_OFF=0, // deactivates the load during high prices
		DLC_CYCLING=1, // cycles the load at a specific rate during high prices
	} DLCMODE;
	enumeration dlc_mode;
	// distribution type, for probabilistic control
	typedef enum {
		PDT_NORMAL,
		PDT_EXPONENTIAL,
		PDT_UNIFORM
	} DISTRIBUTIONTYPE;
	enumeration distribution_type;
	double comfort_level;
	// sensitivity
	double sensitivity;					///<
	int64 period;
	double dPeriod;
	// observation tuple
	double observation;					///<
	double obs_mean;					///<
	double obs_stdev;					///<
	// expected value
	double expectation;					///<
	void *expectation_addr;				///<
	PROPERTY *expectation_property;			///<
	char32 expectation_propname;		///<
	OBJECT *expectation_object;			///<
	// input from series controllers
	int32 input_state;					///< storage for any binary control input
	double input_setpoint;				///< storage for any finite control input
	bool input_chained;					///< flag to determine if there are any attached controllers
	// binary on/off output
	int output_state;					///< binary output value
	void *output_state_addr;			///< binary output value addr
	PROPERTY *output_state_prop;		///< binary output property
	char32 output_state_propname;		///< binary output property name
	double output_setpoint;				///< setpoint output value
	void *output_setpoint_addr;			///< setpoint value addr
	PROPERTY *output_setpoint_property;	///< setpoint property
	char32 output_setpoint_propname;	///< setpoint property name
	OBJECT *observation_object;					///< observed object
	void *observation_addr;				///< observation value addr
	PROPERTY *observation_prop;			///< observation property
	char32 observation_propname;		///< observation prop name
	void *observation_mean_addr;		///< observation mean value addr
	PROPERTY *observation_mean_prop;
	char32 observation_mean_propname;	///< observation mean property name
	void *observation_stdev_addr;		///< observation standard deviation value addr
	PROPERTY *stdev_observation_property;
	char32 observation_stdev_propname;	///< observation standard devation property name
	TIMESTAMP last_cycle_time;			///< last operation cycle time
	int32 cycle_length;					///< operation cycle time delays
	/* setpoint calculations */
	/* actuator calculations */
	double ramp_high, ramp_low;
	double range_high, range_low;
	double prob_off;
	double base_setpoint;
	bool pool_pump_model;				///< Boolean flag for turning on the pool pump model version of duty cycle control
	double base_duty_cycle;
	double cycle_on, cycle_off;			///< used to specify the number of seconds of on and off during the cycling dlc mode
	bool first_period;					///< boolean for determing whether we need to "seed" the dlc cycling mode
	
	bool zipLoadParent;
	double critical_day;
	double old_critical_day;
	double dailyElasticity;
	double subElasticityFirstSecond;
	double subElasticityFirstThird;
	int32 secondTierHours;
	int32 thirdTierHours;
	int32 firstTierHours;

	double firstTierPrice;
	double secondTierPrice;
	double thirdTierPrice;

	double oldFirstTierPrice;
	double oldSecondTierPrice;
	double oldThirdTierPrice;

	double oldPriceRatioThirdFirst;
	double newPriceRatioThirdFirst;
	double oldPriceRatioSecondFirst;
	double newPriceRatioSecondFirst;
	double criticalPriceMultiplier;
	double peakPriceMultiplier;
	

	bool linearizeElasticity;	
	double priceDiffThird;
	double priceDiffSecond;
	double priceDiffFirst;

	int32 elasticityPeriod;	
	bool check_two_tier_cpp;

	//to be deleted later
	double dailyElasticityMultiplier;
	double criticalpeakPriceMultiplier_test;
	double peakPriceMultiplier_test;
	//end of delete

	double *tier_prices;
	double price_offset;
	double *cleared_load;
	double *criticalPeakLoad;
	double *peakLoad;
	double *offPeakLoad;
	double predicted_load;
	double totalClearedLoad;
	double totalOffPeakLoad;
	double totalPeakLoad;
	double averagePeakLoad;
	double totalCriticalPeakLoad;

	enduse *current_load_enduse;
	
	int ArraySize;
	int ThirdTierArraySize;
	int SecondTierArraySize;
	int FirstTierArraySize;

	int ArrayIndex;
	int ThirdTierArrayIndex;
	int SecondTierArrayIndex;
	int FirstTierArrayIndex;

	int first_run;
	TIMESTAMP starttime;
	TIMESTAMP returnTime;

	int trigger_time_under;
	int trigger_time_over;
	int release_time_under;
	int release_time_over;
	double release_freq_under;
	double release_freq_over;
	int time_in_trigger_under;
	int time_in_trigger_over;
	int time_in_release_under;
	int time_in_release_over;
	double trigger_freq_under;
	double trigger_freq_over;
	double frequency;
	typedef enum {
		OVER_FREQUENCY=0,
		UNDER_FREQUENCY=1,
		OVER_UNDER_FREQUENCY=2,
	} PFCMODE;
	enumeration PFC_mode;	
	typedef enum {
		FREE=0,
		TRIGGERED_OFF=1,
		TRIGGERED_ON=2,
		FORCED_OFF=3,	
		FORCED_ON=4,
		RELEASED_OFF=5,
		RELEASED_ON=6,
	} PFCSTATE;
	enumeration PFC_state;
	int state_observed;					///< binary output value
	void *state_observed_addr;			///< binary output value addr
	PROPERTY *state_observed_prop;		///< binary output property
	char32 state_observed_propname;		///< binary output property name
	char32 power_observed_propname;
	TIMESTAMP supervisor_next_run;
	int bid_delay;
	supervisory_control *market;
	double voltage_lockout;
	int voltage_lockout_state;
	int voltage_lockout_time;
	int time_in_voltage_lockout;
	BIDINFO controller_bid;
private:
	void fetch_double(double **prop, const char *name, OBJECT *parent);
	void fetch_int(int **prop, const char *name, OBJECT *parent);
	int calc_ramp(TIMESTAMP t0, TIMESTAMP t1);
	int calc_dutycycle(TIMESTAMP t0, TIMESTAMP t1);
	int calc_proboff(TIMESTAMP t0, TIMESTAMP t1);
	int calc_elasticity(TIMESTAMP t0, TIMESTAMP t1);	
	int calc_dlc(TIMESTAMP t0, TIMESTAMP t1);
	int calc_pfc(TIMESTAMP t0, TIMESTAMP t1);
	int orig_setpoint;
	double *supervisorPeriod;
	int *supervisorMarketId;
	double *stateParent;
	double *powerParent;
	double *ratedPowerParent;
	double powerParentConverted;
	double *voltageParent;
	double *nomVoltageParent;
	int64 last_cycle;
};

#endif // _passive_controller_H
