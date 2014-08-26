/** $Id: fuse.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2011 Battelle Memorial Institute
	@file fuse.h
 @{
 **/

#ifndef FUSE_H
#define FUSE_H

#include "powerflow.h"
#include "link.h"

class fuse : public link_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;

public:
	typedef enum {BLOWN=0, GOOD=1} FUSESTATE;
	typedef enum {NONE=0, EXPONENTIAL=1} MTTRDIST;

	enumeration restore_dist_type;
	unsigned char prev_full_status;	///Fully resolved status (ABC) - used for reliability and recalculation detection

	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);	//Legacy FBS coding in here - may not be needed in future
	fuse(MODULE *mod);
	inline fuse(CLASS *cl=oclass):link_object(cl){};
	int isa(char *classname);

	//Legacy FBS code - will change when reliability makes its way in there
	int fuse_state(OBJECT *parent);
	
	void set_fuse_full(char desired_status_A, char desired_status_B, char desired_status_C);	//Used to set individual phases - 0 = blown, 1 = good, 2 = don't care (retain current)
	void set_fuse_full_reliability(unsigned char desired_status);
	void set_fuse_faulted_phases(unsigned char desired_status);
	void fuse_sync_function(void);	//Functionalized since it exists in two spots - no sense having to update two pieces of code
	OBJECT **get_object(OBJECT *obj, char *name);	//Function to pull object property - reliability use

	double current_limit;		//Current limit for fuses blowing

	unsigned char phased_fuse_status;	//Used to track individual phase fuse status - mainly for reliability - use LSB - x0_XABC
	unsigned char faulted_fuse_phases;	//Used for phase faulting tracking - mainly for reliabiilty - replicated NR functionality so FBS can use it later
	enumeration phase_A_state;
	enumeration phase_B_state;
	enumeration phase_C_state;
	double mean_replacement_time;
	TIMESTAMP fix_time[3];
	double current_current_values[3];

	double fuse_resistance;
private:
	TIMESTAMP prev_fuse_time;				//Tracking variable
	OBJECT **eventgen_obj;					//Reliability variable - link to eventgen object
	FUNCTIONADDR event_schedule;			//Reliability variable - links to "add_event" function in eventgen
	bool event_schedule_map_attempt;		//Flag to see if we've tried to map the event_schedule variable, or not
	
	//Legacy FBS code
	void fuse_check(set phase_to_check, complex *fcurr);
};

EXPORT int change_fuse_state(OBJECT *thisobj, unsigned char phase_change, bool state);
EXPORT int fuse_reliability_operation(OBJECT *thisobj, unsigned char desired_phases);
EXPORT int create_fault_fuse(OBJECT *thisobj, OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time, void *Extra_Data);
EXPORT int fix_fault_fuse(OBJECT *thisobj, int *implemented_fault, char *imp_fault_name, void* Extra_Data);
EXPORT int fuse_fault_updates(OBJECT *thisobj, unsigned char restoration_phases);

#endif // FUSE_H
/**@}**/
