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

EXPORT SIMULATIONMODE interupdate_switch(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

class switch_object : public link_object
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
	inline switch_object(CLASS *cl=oclass):link_object(cl){};
	int isa(char *classname);

	void set_switch(bool desired_status);
	void set_switch_full(char desired_status_A, char desired_status_B, char desired_status_C);	//Used to set individual phases - 0 = open, 1 = closed, 2 = don't care (retain current)
	void set_switch_full_reliability(unsigned char desired_status);
	void set_switch_faulted_phases(unsigned char desired_status);
	void switch_sync_function(void);			//Functionalized since it exists in two spots - no sense having to update two pieces of code
	unsigned char switch_expected_sync_function(void);	//Function to determined expected results of sync - used for reliability
	OBJECT **get_object(OBJECT *obj, char *name);	//Function to pull object property - reliability use

	void BOTH_switch_sync_pre(unsigned char *work_phases_pre, unsigned char *work_phases_post);
	void NR_switch_sync_post(unsigned char *work_phases_pre, unsigned char *work_phases_post, OBJECT *obj, TIMESTAMP *t0, TIMESTAMP *t2);

	//Deltamode call
	SIMULATIONMODE inter_deltaupdate_switch(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

	enumeration switch_banked_mode;
	TIMESTAMP prev_SW_time;	//Used to track switch opens/closes in NR.  Zeros end voltage on first run for other

	unsigned char phased_switch_status;	//Used to track individual phase switch position - mainly for reliability - use LSB - x0_XABC
	unsigned char faulted_switch_phases;	//Used for phase faulting tracking - mainly for reliabiilty - replicated NR functionality so FBS can use it later
	bool prefault_banked;				//Flag used to indicate if a switch was in banked mode pre-fault.  Needs to be swapped out to properly work
	enumeration phase_A_state;				///< Defines the current state of the phase A switch
	enumeration phase_B_state;				///< Defines the current state of the phase B switch
	enumeration phase_C_state;				///< Defines the current state of the phase C switch

	double switch_resistance;
private:
	OBJECT **eventgen_obj;					//Reliability variable - link to eventgen object
	FUNCTIONADDR event_schedule;			//Reliability variable - links to "add_event" function in eventgen
	bool event_schedule_map_attempt;		//Flag to see if we've tried to map the event_schedule variable, or not
};

EXPORT int change_switch_state(OBJECT *thisobj, unsigned char phase_change, bool state);
EXPORT int reliability_operation(OBJECT *thisobj, unsigned char desired_phases);
EXPORT int create_fault_switch(OBJECT *thisobj, OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time, void *Extra_Data);
EXPORT int fix_fault_switch(OBJECT *thisobj, int *implemented_fault, char *imp_fault_name, void* Extra_Data);
EXPORT int switch_fault_updates(OBJECT *thisobj, unsigned char restoration_phases);

#endif // SWITCH_OBJECT_H
/**@}**/
