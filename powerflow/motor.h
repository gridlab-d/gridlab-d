// $Id: motor.cpp 1182 2016-08-15 jhansen $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _motor_H
#define _motor_H

#include "node.h"

EXPORT SIMULATIONMODE interupdate_motor(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

class motor : public node
{
public:

protected:

private:
	complex TF[16];
	complex ITF[16];
	double last_cycle;
	double curr_delta_time;
	bool triplex_connected;

	typedef enum {
		TPNconnected1N=0,		///< Connected 120 V L1 to neutral
		TPNconnected2N=1,		///< Connected 120 V L2 to neutral
		TPNconnected12=2		///< Connected 240 V L1 to L2
	} MOTOR_TRIPLEX_CONNECTION;
	enumeration triplex_connection_type;

	void SPIMupdateVars(); // function to update the previous values for the motor model
	void updateFreqVolt(); // function to update frequency and voltage for the motor
	void SPIMUpdateMotorStatus(); // function to update the status of the motor
	void SPIMStateOFF(); // function to ensure that internal model states are zeros when the motor is OFF
	void SPIMreinitializeVars(); // function to reinitialize values for the motor model
	void SPIMUpdateProtection(double delta_time); // function to update the protection of the motor
	void SPIMSteadyState(TIMESTAMP t1); // steady state model for the SPIM motor
	void SPIMDynamic(double curr_delta_time, double dTime); // dynamic phasor model for the SPIM motor
	complex complex_exp(double angle);
	int invertMatrix(complex TF[16], complex ITF[16]);

	//************** Begin_Yuan's TPIM model ********************//
	void TPIMupdateVars(); // function to update the previous values for the motor model
	void TPIMUpdateMotorStatus(); // function to update the status of the motor
	void TPIMStateOFF(); // function to ensure that internal model states are zeros when the motor is OFF
	void TPIMreinitializeVars(); // function to reinitialize values for the motor model
	void TPIMUpdateProtection(double delta_time); // function to update the protection of the motor
	void TPIMSteadyState(TIMESTAMP t1); // steady state model for the TPIM motor
	void TPIMDynamic(double curr_delta_time, double dTime); // dynamic phasor model for the TPIM motor
	//************** End_Yuan's TPIM model ********************//

	double delta_cycle;
	double Pbase;
	double Ibase;
	double wb;
	double n; 
	double Rds;
	double Rqs;
	double Rr;
	double Xm;
	double Xr;
	double Xc1; 
	double Xc2;
	double Xd_prime;
	double Xq_prime;
	double bsat;
	double Asat;
	double H;
	double To_prime;
	double cap_run_speed_percentage;
	double cap_run_speed; 
	double trip_time;
	double reconnect_time;
	double interation_count;
	double DM_volt_trig_per;
	double DM_speed_trig_per;
	double DM_volt_trig;
	double DM_speed_trig;
	double DM_volt_exit_per;
	double DM_speed_exit_per;
	double DM_volt_exit;
	double DM_speed_exit;
	double speed_error;
	
	typedef enum {
		statusRUNNING=0,		///< Motor is running
		statusSTALLED=1,		///< Motor is stalled
		statusTRIPPED=2,		///< Motor is tripped
		statusOFF = 3			///< Motor is off
	} MOTOR_STATUS;		
	enumeration motor_status;

	typedef enum {
		overrideON=0,			///< Motor is connected
		overrideOFF = 1			///< Motor is disconnected
	} MOTOR_OVERRIDE;		
	enumeration motor_override;

	typedef enum {
		modeSPIM=0,		///<Single-phase induction motor
		modeTPIM=1		///<Three-phase induction motor
	} MOTOR_OPERATION_MODE;
	enumeration motor_op_mode;

	double trip;
	double reconnect;
	bool motor_trip;
	complex psi_b;
    complex psi_f;
    complex psi_dr; 
    complex psi_qr; 
    complex Ids;
    complex Iqs;  
    complex If;
    complex Ib;
    complex Is;
    complex Ss;
    double Telec;
	double Tmech;
    double wr;
	double psi_sat;

	double trip_prev;
	double reconnect_prev;
	int	motor_trip_prev;
	complex psi_b_prev;
    complex psi_f_prev;
    complex psi_dr_prev; 
    complex psi_qr_prev; 
    complex Ids_prev;
    complex Iqs_prev;  
    complex If_prev;
    complex Ib_prev;
    complex Is_prev;
    complex Ss_prev;
    double Telec_prev;
	double Tmech_prev;
    double wr_prev;
	double psi_sat_prev;

	complex Vs;
	double ws;
	int connected_phase;

	//************** Begin_Yuan's TPIM model ********************//
	double rs;
	double rr;
	double lm;
	double IbTPIM; // base current for TPIM model
	double lls;
	double llr;
	double TLrated; // rated mechanical torque applied to three phase induction motor
	double TL;  // actually applied mechanical torque
	double Kfric;
	complex phips;
	complex phins_cj;
	complex phipr;
	complex phinr_cj;
	double omgr0;
	complex Ias;
	complex Ibs;
	complex Ics;
	complex Smt;
	complex Vas;
	complex Vbs;
	complex Vcs;
	complex Ips;
	complex Ipr;
	complex Ins_cj;
	complex Inr_cj;
	double Ls;
	double Lr;
	double sigma1;
	double sigma2;
	double wref;
	double wsyn;

	complex phips_prev;
	complex phins_cj_prev;
	complex phipr_prev;
	complex phinr_cj_prev;
	double omgr0_prev;
	complex Ips_prev;
	complex Ipr_prev;
	complex Ins_cj_prev;
	complex Inr_cj_prev;

	typedef enum {
		initialRUNNING=0,			///< Motor starts running from established speed and torque
		initialSTATIONARY = 1			///< Motor starts up from zero speed and torque
	} TPIM_INITIAL_STATUS;
	enumeration TPIM_initial_status;

	//************** End_Yuan's TPIM model ********************//


public:
	int create(void);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
	motor(MODULE *mod);
	inline motor(CLASS *cl=oclass):node(cl){};
	int init(OBJECT *parent);
	int isa(char *classname);
	static CLASS *pclass;
	static CLASS *oclass;
};

#endif
