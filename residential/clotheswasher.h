/** $Id: dishwasher.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file dishwasher.h
	@addtogroup dishwasher
	@ingroup residential

 @{
 **/

#ifndef _CLOTHESWASHER_H
#define _CLOTHESWASHER_H

#include "gridlabd.h"
#include "residential.h"
#include "residential_enduse.h"
#include "fsm.h"

#define CW_OFF 0x00
#define CW_CONTROL 0x01
#define CW_MOTOR 0x02
#define CW_HEAT 0x04
#define CW_STALL 0x08
#define CW_PHASE1 0x10
#define CW_PHASE2 0x20
#define CW_PHASE3 0x30

class clotheswasher : public residential_enduse
{
	typedef enum {
		HOTWASH		= 0x00,
		WARMWASH	= 0x01,
		COLDWASH    = 0x02;
	} CLOTHESWASHERWASHTEMP;
	typedef enum {
		HOTRINSE	= 0x00,
		WARMRINSE	= 0x01,
		COLDRINSE   = 0x02;
	} CLOTHESWASHERRINSETEMP;
	typedef enum {
		QUICK	= 0x00,
		NORMAL	= 0x01,
		HEAVY	= 0x02,
	} CLOTHESWASHERMODE;
	typedef enum {
		OFF,
		CONTROLSTART,
		PUMPWASHLOWHEATCOLD,
		PUMPWASHLOWHEATWARM,
		PUMPWASHLOWHEATHOT,
		WASHAGITATION,
		PUMPWASHMEDIUM,
		CONTROLRINSE,
		PUMPRINSELOWHEATCOLD,
		PUMPRINSELOWHEATWARM,
		PUMPRINSELOWHEATHOT,
		RINSEAGITATION,
		PUMPRINSEMEDIUM,
		POSTRINSEAGITATION,
		CONTROLRINSEHIGH,
		PUMPRINSEHIGH,
		POSTRINSEHIGHAGITATION,
		CONTROLEND,
	} CLOTHESWASHERSTATE;

	//TOFIX
public:
	GL_BITFLAGS(enumeration,controlmode); ///< controller mode
	GL_STRUCT(double_array,state_duration); ///< state duration
	GL_STRUCT(complex_array,state_power); ///< power for each state
	GL_STRUCT(double_array,state_heatgain); ///< heatgain for each state
	GL_STRUCT(statemachine,state_machine); ///< state machine rules
	GL_BITFLAGS(enumeration,washtemp); ///< dry option selector
	GL_BITFLAGS(enumeration,rinsetemp); ///< wash option selector
	GL_BITFLAGS(enumeration,washmode); ///< wash option selector
	GL_STRUCT(complex,pump_power); ///< power used by pump
	GL_STRUCT(complex,coil_power_wet); ///< power used by coils when wet (100C)
	//GL_STRUCT(complex,coil_power_dry); ///< power used by coils when dry (>>100C)
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
	static clotheswasher *defaults;

	clotheswasher(MODULE *module);
	~clotheswasher();
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

#endif // _CLOTHESWASHER_H

/**@}**/
