/** $Id: dishwasher.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file dishwasher.h
	@addtogroup dishwasher
	@ingroup residential

 @{
 **/

#ifndef _DISHWASHER_H
#define _DISHWASHER_H

#include "gridlabd.h"
#include "residential.h"
#include "residential_enduse.h"
#include "fsm.h"

#define DW_OFF 0x00
#define DW_CONTROL 0x01
#define DW_MOTOR 0x02
#define DW_HEAT 0x04
#define DW_STALL 0x08
#define DW_PHASE1 0x10
#define DW_PHASE2 0x20
#define DW_PHASE3 0x30

class dishwasher : public residential_enduse
{
	typedef enum {
		AIR		= 0x00,
		HEAT	= 0x01,
	} DISHWASHERDRYMODE;
	typedef enum {
		QUICK	= 0x00,
		NORMAL	= 0x01,
		HEAVY	= 0x02,
	} DISHWASHERWASHMODE;
	typedef enum {
		OFF,
		CONTROLSTART,
		PUMPPREWASH,
		PUMPWASHQUICK,
		HEATWASHQUICK,
		PUMPWASHNORMAL,
		HEATWASHNORMAL,
		PUMPWASHHEAVY,
		HEATWASHHEAVY,
		CONTROLWASH,
		PUMPRINSE,
		HEATRINSE,
		CONTROLRINSE,
		HEATDRYON,
		HEATDRYOFF,
		CONTROLDRY,
		CONTROLEND,
	} DISHWASHERSTATE;
public:
	GL_BITFLAGS(enumeration,controlmode); ///< controller mode
	GL_STRUCT(double_array,state_duration); ///< state duration
	GL_STRUCT(complex_array,state_power); ///< power for each state
	GL_STRUCT(double_array,state_heatgain); ///< heatgain for each state
	GL_STRUCT(statemachine,state_machine); ///< state machine rules
	GL_BITFLAGS(enumeration,drymode); ///< dry option selector
	GL_BITFLAGS(enumeration,washmode); ///< wash option selector
	GL_STRUCT(complex,pump_power); ///< power used by pump
	GL_STRUCT(complex,coil_power_wet); ///< power used by coils when wet (100C)
	GL_STRUCT(complex,coil_power_dry); ///< power used by coils when dry (>>100C)
	GL_STRUCT(complex,control_power); ///< power used by controller
	GL_ATOMIC(double,state_queue); ///< accumulated demand (units)
	GL_ATOMIC(double,demand_rate); ///< dish load accumulation rate (units/day)
	GL_ATOMIC(double,hotwater_demand); ///< hotwater consumption (gpm)
	GL_ATOMIC(double,hotwater_temperature); ///< hotwater temperature (degF)
	GL_ATOMIC(double,hotwater_temperature_drop); ///< hotwater temperature drop from plumbing bus (degF)
	GL_STRUCT(double_array,state_setpoint); ///< temperature setpoints for each state
private:
	size_t n_states; ///< number states defined in controlmode

public:
	static CLASS *oclass, *pclass;
	static dishwasher *defaults;

	dishwasher(MODULE *module);
	~dishwasher();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP last_t;

	int precommit(TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t1);
	inline TIMESTAMP presync(TIMESTAMP t1);
	inline TIMESTAMP postsync(TIMESTAMP t1) { return TS_NEVER;};
	double update_state(double dt); //, TIMESTAMP t1=0);		///< updates the load struct and returns the time until expected state change
};

#endif // _DISHWASHER_H

/**@}**/
