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
	gld::complex TF[16];
	gld::complex ITF[16];
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
	gld::complex complex_exp(double angle);
	int invertMatrix(gld::complex TF[16], gld::complex ITF[16]);

	//TPIM functions
	void TPIMupdateVars(); // function to update the previous values for the motor model
	void TPIMUpdateMotorStatus(); // function to update the status of the motor
	void TPIMStateOFF(); // function to ensure that internal model states are zeros when the motor is OFF
	void TPIMreinitializeVars(); // function to reinitialize values for the motor model
	void TPIMUpdateProtection(double delta_time); // function to update the protection of the motor
	void TPIMSteadyState(TIMESTAMP t1); // steady state model for the TPIM motor
	void TPIMDynamic(double curr_delta_time, double dTime); // dynamic phasor model for the TPIM motor

	double delta_cycle;
	double Pbase;
	double Ibase;
	double wbase;
	double ws_pu;
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
	double Jm;
	double To_prime;
	double cap_run_speed_percentage;
	double cap_run_speed; 
	double trip_time;
	double reconnect_time;
	int32 iteration_count;
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

	OBJECT *mtr_house_pointer;				///< Pointer to house object to monitor
	gld_property *mtr_house_state_pointer;	///< Property map for house to monitor

	typedef enum {
		modeSPIM=0,		///<Single-phase induction motor
		modeTPIM=1		///<Three-phase induction motor
	} MOTOR_OPERATION_MODE;
	enumeration motor_op_mode;

	typedef enum {
		modelDirectTorque=0,	///< Torque value in Tmech taken directly
		modelSpeedFour=1		///< Torque is set at 0.85 + .15*speed^4
	} MOTOR_TORQUE_MODEL;
	enumeration motor_torque_usage_method;

	typedef enum {
		house_mode_NONE=0,		//Default - basically ignore it
		house_mode_COOLING=1,	//Expected to be cooling (heat pump or specific AC)
		house_mode_HEATING=2	//Expected to be heating (heat pump)
	} HOUSE_MODE_USAGE;
	enumeration connected_house_assumed_mode;

	double trip;
	double reconnect;
	bool motor_trip;
	gld::complex psi_b;
    gld::complex psi_f;
    gld::complex psi_dr; 
    gld::complex psi_qr; 
    gld::complex Ids;
    gld::complex Iqs;  
    gld::complex If;
    gld::complex Ib;
    gld::complex Is;
    gld::complex motor_elec_power;
    double Telec;
	double Tmech;
	double Tmech_eff;
    double wr;
	double psi_sat;

	double trip_prev;
	double reconnect_prev;
	int	motor_trip_prev;
	gld::complex psi_b_prev;
    gld::complex psi_f_prev;
    gld::complex psi_dr_prev; 
    gld::complex psi_qr_prev; 
    gld::complex Ids_prev;
    gld::complex Iqs_prev;  
    gld::complex If_prev;
    gld::complex Ib_prev;
    gld::complex Is_prev;
    gld::complex motor_elec_power_prev;
    double Telec_prev;
	double Tmech_prev;
    double wr_prev;
	double psi_sat_prev;

    // Under voltage protection
    double uv_relay_rand;
    double uv_relay_time;
    double uv_relay_trip_time;
    double uv_relay_trip_V;
    typedef enum {
		uv_relay_INSTALLED=0,
		uv_relay_UNINSTALLED=1
	} UV_RELAY_INSTALLED;
    enumeration uv_relay_install;
    int uv_lockout;
    typedef enum {
		contactorOPEN=0,
		contactorCLOSED=1
	} CONTACTOR_STATE;
	enumeration contactor_state;
    double contactor_open_rand;
    double contactor_close_rand;
    double contactor_open_Vmin;
    double contactor_close_Vmax;

	gld::complex Vs;
	double ws;
	int connected_phase;

	//TPIM variables
	double Zbase;
	double Rs;
	double Xs;
	double pf;
	double rs_pu;
	double rr_pu;
	double lm;
	double lls;
	double llr;
	double TL_pu;  // actually applied mechanical torque
	double Kfric;
	gld::complex phips;
	gld::complex phins_cj;
	gld::complex phipr;
	gld::complex phinr_cj;
	double wr_pu;
	gld::complex Ias;
	gld::complex Ibs;
	gld::complex Ics;
	gld::complex Vas;
	gld::complex Vbs;
	gld::complex Vcs;
	gld::complex Ips;
	gld::complex Ipr;
	gld::complex Ins_cj;
	gld::complex Inr_cj;
	double Ls;
	double Lr;
	double sigma1;
	double sigma2;

	gld::complex phips_prev;
	gld::complex phins_cj_prev;
	gld::complex phipr_prev;
	gld::complex phinr_cj_prev;
	double wr_pu_prev;
	gld::complex Ips_prev;
	gld::complex Ipr_prev;
	gld::complex Ins_cj_prev;
	gld::complex Inr_cj_prev;

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
