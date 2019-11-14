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

	typedef enum {
			TPIM_A=0,		///< Type A three-phase motor
			TPIM_B=1,		///< Type B three-phase motor
			TPIM_C=2		///< Type C three-phase motor
	} TPIM_TYPE;
	enumeration TPIM_type;

	typedef enum {
			SPIM_C = 0, 	///< Single phase motor with constant torque
			SPIM_S = 1, 	///< Single phase motor with speed dependent torque
			SPIM_T = 2,     ///< Single phase motor with triangular torque
	} SPIM_TYPE;
	enumeration SPIM_type;

	void SPIMupdateVars(); // function to update the previous values for the motor model
	void updateFreqVolt(); // function to update frequency and voltage for the motor
	void SPIMUpdateMotorStatus(); // function to update the status of the motor
	void SPIMStateOFF(double dTime); // function to ensure that internal model states are zeros when the motor is OFF
	void SPIMreinitializeVars(); // function to reinitialize values for the motor model
	void SPIMUpdateProtection(double delta_time); // function to update the protection of the motor
	void SPIMSteadyState(TIMESTAMP t1); // steady state model for the SPIM motor
	void SPIMDynamic(double dTime); // dynamic phasor model for the SPIM motor
	complex complex_exp(double angle);
	int invertMatrix(complex TF[16], complex ITF[16]);

	void UpdateProtection(double delta_time); // function to update the protection of both SPIM and TPIM motor

	//TPIM functions
	void TPIMupdateVars(); // function to update the previous values for the motor model
	void TPIMUpdateMotorStatus(); // function to update the status of the motor
	void TPIMStateOFF(); // function to ensure that internal model states are zeros when the motor is OFF
	void TPIMreinitializeVars(); // function to reinitialize values for the motor model
	void TPIMUpdateProtection(double delta_time); // function to update the protection of the motor
	void TPIMSteadyState(TIMESTAMP t1); // steady state model for the TPIM motor
	void TPIMDynamic(double curr_delta_time, double dTime); // dynamic phasor model for the TPIM motor

	// Protection functions
	void motorCheckTrip(double delta_time, double_array* motorProtection, double* timerList, bool& tripStatus); // function to check the trip status under each protection
	void SPIM_CheckThermalTrip(double delta_time, double_array* motorProtection, bool &tripStatus); // function to check thermal trip for SPIM
	void motorCheckReconnect(double delta_time, double_array* motorProtection, double& reconnectTimer, bool& reconnectStatus); // function to check the reconnect status under each protection

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

	// Triangular torque items - Zhigang Chu
	double t_DLD; // For triangular torque motors, indicate at what time it starts to have triangle torque
	double theta; // Rotor angle for triangular torque motors [rad]
	double T_av; // Angle dependent load coefficient for triangular torque motors
	double avRatio; // Ratio between angle dependent load coefficient and speed dependent one
					// T_av = avRatio * Tmech (aka T_speed)
	double R_stall; // Stall resistance of SPIM
	double temperature_SPIM;
	double Tth; // Thermal time constant
	
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

	typedef enum {
		PROT_MODE_SIMPLE=0,		//Basic over-voltage and UV contactor protection
		PROT_MODE_ADVANCED=1	//More complicated "4-type" protection
	} TYPE_OF_PROTECTION_MODE;
	enumeration motor_protection_type_mode;

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
    complex motor_elec_power;
    double Telec;
	double Tmech;
	double Tmech_eff;
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
    complex motor_elec_power_prev;
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

	complex Vs;
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
	complex phips;
	complex phins_cj;
	complex phipr;
	complex phinr_cj;
	double wr_pu;
	complex Ias;
	complex Ibs;
	complex Ics;
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

	complex phips_prev;
	complex phins_cj_prev;
	complex phipr_prev;
	complex phinr_cj_prev;
	double wr_pu_prev;
	complex Ips_prev;
	complex Ipr_prev;
	complex Ins_cj_prev;
	complex Inr_cj_prev;

	// Protection parameters

public:

	// double array defining trip volt and time to be entered by users
	GL_STRUCT(double_array,relayProtectionTrip); // Volt-time curve of electronic relay protection (Protection 1)
	GL_STRUCT(double_array,overLoadProtectionTrip); // Volt-time curve of over load protection (Protection 2)
	GL_STRUCT(double_array,thermalProtectionTrip); // Volt-time curve of thermal protection (Protection 3)
	GL_STRUCT(double_array,contactorProtectionTrip); // Volt-time curve of contactor protection (Protection 4)
	GL_STRUCT(double_array,emsProtectionTrip); // Volt-time curve of EMS protection (Protection 5)

	// double array defining reconnect volt and time to be entered by users
	GL_STRUCT(double_array,relayProtectionReconnect); // Volt-time curve of electronic relay protection
	GL_STRUCT(double_array,contactorProtectionReconnect); // Volt-time curve of contactor protection
	GL_STRUCT(double_array,emsProtectionReconnect); // Volt-time curve of EMS protection

private:
	// Array of timer to determine whether has arrived at tripping time
	double *relayTimerList;
	double *overLoadTimerList;
	double *thermalTimerList;
	double *contactorTimerList;
	double *emsTimerList;

	// Array of previous timer to record the values in last cycle, incase duplicated additions to timer at the same cycle
	double *relayTimerList_prev;
	double *overLoadTimerList_prev;
	double *thermalTimerList_prev;
	double *contactorTimerList_prev;
	double *emsTimerList_prev;

	// Reconnect timer to determine whether has arrived at reconnection time
	double relayTimerReconnect;
	double contactorTimerReconnect;
	double emsTimerReconnect;

	// Reconnect timer to record the values in last cycle, incase duplicated additions to timer at the same cycle
	double relayTimerReconnect_prev;
	double contactorTimerReconnect_prev;
	double emsTimerReconnect_prev;

	// boolean flag indicating the protection type is adopted or not
	bool hasRelayProtection;
	bool hasOverLoadProtection;
	bool hasThermalProtection;
	bool hasContactorProtection;
	bool hasEMSProtection;

	// boolean flag indicating the protection has determined to trip or not
	bool relayTrip;
	bool overLoadTrip;
	bool thermalTrip;
	bool contactorTrip;
	bool emsTrip;

	// boolean flag indicating the protection type has determined to reconnect or not
	bool relayReconnect;
	bool contactorReconnect;
	bool emsReconnect;

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
	static motor *defaults;
};

#endif
