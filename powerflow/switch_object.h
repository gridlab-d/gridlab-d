/** $Id: switch_object.h,v 1.5 2008/02/04 23:08:12 d3p988 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file switch_object.h

	@note care should be taken when working with this class because \e switch
	is a reserved word in C/C++ and \p switch_object is used whenever that
	can cause problems.
 @{
 **/

#ifndef SWITCH_OBJECT_H
#define SWITCH_OBJECT_H

#include "powerflow.h"
#include "link.h"

class switch_object : public link
{
public:
	static CLASS *oclass;
	static CLASS *pclass;

public:
	typedef enum {OPEN=0, CLOSED=1} SWITCHSTATE;
	typedef enum {INDIVIDUAL_SW=0, BANKED_SW=1} SWITCHBANK;
	unsigned char prev_full_status;	///Fully resolved status (ABC) - used for reliability and recalculation detection

	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0);
	switch_object(MODULE *mod);
	inline switch_object(CLASS *cl=oclass):link(cl){};
	int isa(char *classname);

	void set_switch(bool desired_status);
	void set_switch_full(char desired_status_A, char desired_status_B, char desired_status_C);	//Used to set individual phases - 0 = open, 1 = closed, 2 = don't care (retain current)

	TIMESTAMP prev_SW_time;	//Used to track switch opens/closes in NR.  Zeros end voltage on first run for other

	SWITCHBANK switch_banked_mode;

	unsigned char phased_switch_status;	//Used to track individual phase switch position - mainly for reliability - use LSB - x0_XABC
	SWITCHSTATE phase_A_state;
	SWITCHSTATE phase_B_state;
	SWITCHSTATE phase_C_state;
};

EXPORT int change_switch_state(OBJECT *thisobj, unsigned char phase_change, bool state);

#endif // SWITCH_OBJECT_H
/**@}**/
