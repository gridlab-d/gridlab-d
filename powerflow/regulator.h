// $Id: regulator.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _REGULATOR_H
#define _REGULATOR_H

#include "powerflow.h"
#include "link.h"
#include "regulator_configuration.h"

#define tap_A tap[0]
#define tap_B tap[1]
#define tap_C tap[2]

class regulator : public link_object
{
public:
	double VtapChange;
	double tapChangePer;
	double Vlow;
	double Vhigh;
	complex V2[3], Vcomp[3];
	int16 tap[3];
	int16 prev_tap[3];
	complex volt[3];
	complex D_mat[3][3];
	complex W_mat[3][3];
	complex curr[3];
	OBJECT *RemoteNode;		 //Remote node for sensing voltage values in REMOTE_NODE Control method
	double tap_A_change_count; //Counter for the number of times tap_A changes.
	double tap_B_change_count; //Counter for the number of times tap_B changes.
	double tap_C_change_count; //Counter for the number of times tap_C changes.
	int16 initial_tap_A;
	int16 initial_tap_B;
	int16 initial_tap_C;
	int16 tap_A_changed;
	int16 tap_B_changed;
	int16 tap_C_changed;

	TIMESTAMP prev_time;
	int16 prev_tap_A;
	int16 prev_tap_B;
	int16 prev_tap_C;

	double regulator_resistance;

protected:
	int64 mech_t_next[3];	 //next time step after tap change
	int64 dwell_t_next[3];	 //wait to advance only after sensing over/under voltage for a certain dwell_time
	int64 next_time;		 //final return for next time step
	int16 mech_flag[3];		 //indicates whether a state change is okay due to mechanical tap changes
	int16 dwell_flag[3];	 //indicates whether a state change is okay due to dwell time limitations
	int16 first_run_flag[3]; //keeps the system from blowing up on bad initial tap position guess
	complex check_voltage[3];//Voltage that is being checked against
	void get_monitored_voltage();  //Function to calculate check_voltage depending on mode

private:
	bool offnominal_time;	//Used to detect off-nominal timesteps and perform an exception for them
	bool iteration_flag;	//Iteration toggler - to maintain logic from previous NR implementation, to a degree
public:
	static CLASS *oclass;
	static CLASS *pclass;
	
public:
	OBJECT *configuration;
	regulator(MODULE *mod);
	inline regulator(CLASS *cl=oclass):link_object(cl){};
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	int isa(char *classname);
};

#endif // _REGULATOR_H
