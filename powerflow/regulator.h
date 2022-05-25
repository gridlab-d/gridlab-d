// $Id: regulator.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _REGULATOR_H
#define _REGULATOR_H

#include "powerflow.h"
#include "link.h"
#include "regulator_configuration.h"

EXPORT SIMULATIONMODE interupdate_regulator(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

//KML export
EXPORT int regulator_kmldata(OBJECT *obj,int (*stream)(const char*,...));

class regulator : public link_object
{
public:
	double VtapChange;
	double tapChangePer;
	double Vlow;
	double Vhigh;
	gld::complex V2[3], Vcomp[3];
	int16 tap[3];
	int16 prev_tap[3];
	gld::complex volt[3];
	gld::complex D_mat[3][3];
	gld::complex W_mat[3][3];
	gld::complex curr[3];
	gld::complex check_voltage[3];//Voltage that is being checked against if internal. If externally obtained (through cosim), this is the voltage value
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
	typedef enum {
		msg_INTERNAL,
		msg_EXTERNAL,
	} msg_mode;
	enumeration msgmode;
	TIMESTAMP prev_time;
	int16 prev_tap_A;
	int16 prev_tap_B;
	int16 prev_tap_C;

	double regulator_resistance;

	//Deltamode function
	SIMULATIONMODE inter_deltaupdate_regulator(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
	void reg_prePre_fxn(double curr_time_value);	//Functionalized "presync before link::presync" calls
	void reg_postPre_fxn(void);	//Functionalized "presync after link::presync" calls
	double reg_postPost_fxn(double curr_time_value);	//Functionalized "postsync after link::postsync" calls

protected:
	double mech_t_next[3];	 //next time step after tap change
	double dwell_t_next[3];	 //wait to advance only after sensing over/under voltage for a certain dwell_time
	double next_time;		 //final return for next time step
	int16 mech_flag[3];		 //indicates whether a state change is okay due to mechanical tap changes
	int16 dwell_flag[3];	 //indicates whether a state change is okay due to dwell time limitations
	int16 first_run_flag[3]; //keeps the system from blowing up on bad initial tap position guess
	void get_monitored_voltage();  //Function to calculate check_voltage depending on mode
	bool toggle_reverse_flow[3];
	bool toggle_reverse_flow_banked;
	int16 reverse_flow_tap[3];

private:
	bool offnominal_time;	//Used to detect off-nominal timesteps and perform an exception for them
	bool iteration_flag;	//Iteration toggler - to maintain logic from previous NR implementation, to a degree
	bool new_reverse_flow_action[3];
	bool deltamode_reiter_request;	// Flag to replicate a reiteration request from presync/postsync, since that is handled different in deltamode - used to match QSTS
	gld_property *RNode_voltage[3];	// Pointer for API to map to RNode voltage values
	gld_property *ToNode_voltage[3];	//Pointer for API to map to voltages to read
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
	int kmldata(int (*stream)(const char*,...));
};

#endif // _REGULATOR_H
