// $Id: regulator_configuration.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _REGULATORCONFIGURATION_H
#define _REGULATORCONFIGURATION_H

#include "powerflow.h"
#include "powerflow_library.h"

class regulator_configuration : public powerflow_library
{
public:
	typedef enum {
		WYE_WYE=1,
		OPEN_DELTA_ABBC,
		OPEN_DELTA_BCAC,
		OPEN_DELTA_CABA,
		CLOSED_DELTA,
		CONNECT_TYPE_MAX
	} connect_type_enum;
				
	typedef enum {
		MANUAL=1,
		OUTPUT_VOLTAGE=2,
		REMOTE_NODE=3,
		LINE_DROP_COMP=4
	} Control_enum;

	typedef enum {
		INDIVIDUAL=1,
		BANK=2
	} control_level_enum;

	typedef enum {
		A = 1,
		B
	} Type_enum;


	//Split out for access in other objects, fixes some odd enum issues
	enumeration Control;
	enumeration control_level;
	enumeration Type;
	enumeration connect_type;

	/* get_name acquires the name of an object or 'unnamed' if non set */
	inline const char *get_name(void) const { static char tmp[64]; OBJECT *obj=OBJECTHDR(this); return obj->name?obj->name:(sprintf(tmp,"%s:%d",obj->oclass->name,obj->id)>0?tmp:"(unknown)");};
	/* get_id acquires the object's id */
	inline unsigned int get_id(void) const {return OBJECTHDR(this)->id;};
	
	double band_center;		// band center setting of regulator control
	double band_width;		// band width setting of regulator control
	double dwell_time;		// time delay setting of regulator control
	double time_delay;		// mechanical time delay between tap changes 
	int16 raise_taps;		// number of regulator raise taps
	int16 lower_taps;		// number of regulator lower taps
	double CT_ratio;		// primary rating of current transformer (x:5)
	double PT_ratio;		// potential transformer rating (x:1)
	double ldc_R_V[3];		// Line Drop Compensation R setting of regulator control (in volts)
	double ldc_X_V[3];		// Line Drop Compensation X setting of regulator control (in volts)
	set CT_phase;			// phase(s) monitored by CT
	set PT_phase;			// phase(s) monitored by PT
	double regulation;		// regulation of voltage regulator in %
	int16 tap_pos[3];

	#define ldc_R_V_A ldc_R_V[0]	// R for each phase
	#define ldc_R_V_B ldc_R_V[1]
	#define ldc_R_V_C ldc_R_V[2]
	#define ldc_X_V_A ldc_X_V[0]	// X for each phase
	#define ldc_X_V_B ldc_X_V[1]
	#define ldc_X_V_C ldc_X_V[2]
	#define tap_posA tap_pos[0]		// tap_pos of phase A
	#define tap_posB tap_pos[1]		// tap_pos of phase B
	#define tap_posC tap_pos[2]		// tap_pos of phase C

public:
	static CLASS *oclass;
	static CLASS *pclass;
	regulator_configuration(MODULE *mod);
	inline regulator_configuration(CLASS *cl=oclass):powerflow_library(cl){};
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
};


#endif // _REGULATORCONFIGURATION_H
