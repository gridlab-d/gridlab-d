/** $Id: controller2.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file controller2.h
	@addtogroup controller2
	@ingroup market

 **/

#ifndef _controller2_H
#define _controller2_H

#include <stdarg.h>
#include "auction.h"
#include "gridlabd.h"

class controller2 {
public:
	controller2(MODULE *);
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
		CM_CND=2,	// cumulative normal distribution
		CM_THIRD=3,
		CM_DUTYCYCLE=4,
		CM_PROBOFF=5,	// probabilistic shut-off
	} CONTROLMODE;
	enumeration control_mode;
	// sensitivity
	double sensitivity;					///<
	int64 period;
	// observation tuple
	double observation;					///<
	double obs_mean;					///<
	double obs_stdev;					///<
	// expected value
	double expectation;					///<
	void *expectation_addr;				///<
	PROPERTY *expectation_prop;			///<
	char32 expectation_propname;		///<
	OBJECT *expectation_obj;			///<
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
	PROPERTY *output_setpoint_prop;		///< setpoint property
	char32 output_setpoint_propname;	///< setpoint property name
	OBJECT *observable;					///< observed object
	void *observation_addr;				///< observation value addr
	PROPERTY *observation_prop;			///< observation property
	char32 observation_propname;		///< observation prop name
	void *observation_mean_addr;		///< observation mean value addr
	PROPERTY *observation_mean_prop;
	char32 observation_mean_propname;	///< observation mean property name
	void *observation_stdev_addr;		///< observation standard deviation value addr
	PROPERTY *observation_stdev_prop;
	char32 observation_stdev_propname;	///< observation standard devation property name
	TIMESTAMP last_cycle_time;			///< last operation cycle time
	int32 cycle_length;					///< operation cycle time delays
	/* setpoint calculations */
	/* actuator calculations */
	double ramp_high, ramp_low;
	double range_high, range_low;
	double prob_off;
	double first_setpoint;
private:
	int calc_ramp(TIMESTAMP t0, TIMESTAMP t1);
	int calc_cnd(TIMESTAMP t0, TIMESTAMP t1);
	int calc_dutycycle(TIMESTAMP t0, TIMESTAMP t1);
	int calc_proboff(TIMESTAMP t0, TIMESTAMP t1);
	int orig_setpoint;
	int64 last_cycle;
};

#endif // _controller2_H
