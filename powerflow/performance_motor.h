// $Id: performance_motor.cpp
//	Copyright (C) 2020 Battelle Memorial Institute

#ifndef _performance_motor_H
#define _performance_motor_H

#include "node.h"

EXPORT SIMULATIONMODE interupdate_performance_motor(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

class performance_motor : public node
{
public:

protected:

private:
	double last_time_val;
	double curr_time_val;

	typedef enum {
		statusRUNNING=0,		///< Motor is running
		statusSTALLED=1,		///< Motor is stalled
		statusTRIPPED=2,		///< Motor is tripped
		statusOFF = 3			///< Motor is off
	} MOTOR_STATUS;		
	enumeration motor_status;

	typedef enum {
		modeSPIM=0,		///<Single-phase induction motor
		modeTPIM=1		///<Three-phase induction motor
	} MOTOR_OPERATION_MODE;
	enumeration motor_op_mode;

    gld::complex PowerVal;

    double delta_f_val;

    double Vbrk;
    double Vstallbrk;
    double Vstall;
    double Vrst;

    double P_val;
    double Q_val;
    double P_0;
    double Q_0;
    double K_p1;
    double K_q1;
    double K_p2;
    double K_q2;
    double N_p1;
    double N_q1;
    double N_p2;
    double N_q2;
    double CmpKpf;
    double CmpKqf;
    double CompPF;
    gld::complex stall_impedance_pu;
    double Tstall;
    double reconnect_time;

    double Pinit;
    double Vinit;

    double stall_time_tracker;
    double reconnect_time_tracker;

    double P_base;

    gld::complex stall_admittance;

    void update_motor_equations(void);


public:
	int create(void);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
	performance_motor(MODULE *mod);
	inline performance_motor(CLASS *cl=oclass):node(cl){};
	int init(OBJECT *parent);
	int isa(char *classname);
	static CLASS *pclass;
	static CLASS *oclass;
};

#endif
