// $Id: capacitor.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _CAPACITOR_H
#define _CAPACITOR_H

#include "powerflow.h"
#include "node.h"

class capacitor : public node
{
public:
	typedef enum {MANUAL=0, VAR=1, VOLT=2, VARVOLT=3} CAPCONTROL;
	typedef enum {OPEN=0, CLOSED=1} CAPSWITCH;
	typedef enum {BANK=0, INDIVIDUAL=1} CAPCONTROL2;

	set pt_phase;				// phase PT on
	set phases_connected;		// phases capacitors connected to
	double voltage_set_high;    // high voltage set point for voltage control (turn off)
	double voltage_set_low;     // low voltage set point for voltage control (turn on)
	double VAr_set_high;		// high VAR set point for VAR control (turn off)
	double VAr_set_low;			// low VAR set point for VAR control (turn on)
	double capacitor_A;			// Capacitance value for phase A or phase AB
	double capacitor_B;			// Capacitance value for phase B or phase BC
	double capacitor_C;			// Capacitance value for phase C or phase CA
	CAPCONTROL2 control_level;  // define bank or individual control
	CAPSWITCH switchA_state;	// capacitor A switch open or close
	CAPSWITCH switchB_state;	// capacitor B switch open or close
	CAPSWITCH switchC_state;	// capacitor C switch open or close
	OBJECT *RemoteNode;			// Remote node for sensing values used for control schemes
	double time_delay;          // control time delay
	double dwell_time;			// Time for system to remain constant before a state change will be passed

protected:
	CAPCONTROL control;			// control operation strategy; 0 - manual, 1 - VAr, 2- voltage, 3 - VAr primary,voltage backup.
	int64 time_to_change;       // time until state change
	int64 dwell_time_left;		// time until dwell interval is met
	int64 last_time;			// last time capacitor was checked
	double cap_nominal_voltage;	// Nominal voltage for the capacitor. Used for calculation of capacitance value.

public:
	int create(void);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	capacitor(MODULE *mod);
	inline capacitor(CLASS *cl=oclass):node(cl){};
	int init(OBJECT *parent);
	int isa(char *classname);

private:
	complex cap_value[3];		// Capacitor values translated to admittance
	CAPSWITCH switchA_state_Next;	// capacitor A switch open or close at next transition
	CAPSWITCH switchB_state_Next;	// capacitor B switch open or close at next transition
	CAPSWITCH switchC_state_Next;	// capacitor C switch open or close at next transition
	CAPSWITCH switchA_state_Req_Next;	// capacitor A switch open or close requested (dwell checking)
	CAPSWITCH switchB_state_Req_Next;	// capacitor A switch open or close requested (dwell checking)
	CAPSWITCH switchC_state_Req_Next;	// capacitor A switch open or close requested (dwell checking)
	CAPSWITCH switchA_state_Prev;	// capacitor A switch open or close at previous transition (used for manual control)
	CAPSWITCH switchB_state_Prev;	// capacitor B switch open or close at previous transition (used for manual control)
	CAPSWITCH switchC_state_Prev;	// capacitor C switch open or close at previous transition (used for manual control)
	double VArVals[3];				// VAr values recorded (due to nature of how it's recorded, it has to be in here)
	bool NotFirstIteration;			// Checks to see if this is the first iteration of the system.

public:
	static CLASS *pclass;
	static CLASS *oclass;
};

#endif // _CAPACITOR_H
