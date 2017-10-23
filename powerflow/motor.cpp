// $Id: motor.cpp 1182 2016-08-15 jhansen $
//	Copyright (C) 2008 Battelle Memorial Institute

#include <stdlib.h>	
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "motor.h"

//////////////////////////////////////////////////////////////////////////
// capacitor CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* motor::oclass = NULL;
CLASS* motor::pclass = NULL;

/**
* constructor.  Class registration is only called once to 
* register the class with the core. Include parent class constructor (node)
*
* @param mod a module structure maintained by the core
*/

static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

motor::motor(MODULE *mod):node(mod)
{
	if(oclass == NULL)
	{
		pclass = node::oclass;
		
		oclass = gl_register_class(mod,"motor",sizeof(motor),passconfig|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class motor";
		else
			oclass->trl = TRL_PRINCIPLE;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "node",
			PT_double, "base_power[W]", PADDR(Pbase),PT_DESCRIPTION,"base power",
			PT_double, "n", PADDR(n),PT_DESCRIPTION,"ratio of stator auxiliary windings to stator main windings",
			PT_double, "Rds[ohm]", PADDR(Rds),PT_DESCRIPTION,"d-axis resistance",
			PT_double, "Rqs[ohm]", PADDR(Rqs),PT_DESCRIPTION,"q-asis resistance",
			PT_double, "Rr[ohm]", PADDR(Rr),PT_DESCRIPTION,"rotor resistance",
			PT_double, "Xm[ohm]", PADDR(Xm),PT_DESCRIPTION,"magnetizing reactance",
			PT_double, "Xr[ohm]", PADDR(Xr),PT_DESCRIPTION,"rotor reactance",
			PT_double, "Xc_run[ohm]", PADDR(Xc1),PT_DESCRIPTION,"running capacitor reactance",
			PT_double, "Xc_start[ohm]", PADDR(Xc2),PT_DESCRIPTION,"starting capacitor reactance",
			PT_double, "Xd_prime[ohm]", PADDR(Xd_prime),PT_DESCRIPTION,"d-axis reactance",
			PT_double, "Xq_prime[ohm]", PADDR(Xq_prime),PT_DESCRIPTION,"q-axis reactance",
			PT_double, "A_sat", PADDR(Asat),PT_DESCRIPTION,"flux saturation parameter, A",
			PT_double, "b_sat", PADDR(bsat),PT_DESCRIPTION,"flux saturation parameter, b",
			PT_double, "H", PADDR(H),PT_DESCRIPTION,"moment of inertia",
			PT_double, "To_prime[s]", PADDR(To_prime),PT_DESCRIPTION,"rotor time constant",
			PT_double, "capacitor_speed[%]", PADDR(cap_run_speed_percentage),PT_DESCRIPTION,"percentage speed of nominal when starting capacitor kicks in",
			PT_double, "trip_time[s]", PADDR(trip_time),PT_DESCRIPTION,"time motor can stay stalled before tripping off ",
			PT_double, "reconnect_time[s]", PADDR(reconnect_time),PT_DESCRIPTION,"time before tripped motor reconnects",
			PT_double, "mechanical_torque", PADDR(Tmech),PT_DESCRIPTION,"mechanical torque applied to the motor",
			PT_double, "iteration_count", PADDR(interation_count),PT_DESCRIPTION,"maximum number of iterations for steady state model",
			PT_double, "delta_mode_voltage_trigger[%]", PADDR(DM_volt_trig_per),PT_DESCRIPTION,"percentage voltage of nominal when delta mode is triggered",
			PT_double, "delta_mode_rotor_speed_trigger[%]", PADDR(DM_speed_trig_per),PT_DESCRIPTION,"percentage speed of nominal when delta mode is triggered",
			PT_double, "delta_mode_voltage_exit[%]", PADDR(DM_volt_exit_per),PT_DESCRIPTION,"percentage voltage of nominal to exit delta mode",
			PT_double, "delta_mode_rotor_speed_exit[%]", PADDR(DM_speed_exit_per),PT_DESCRIPTION,"percentage speed of nominal to exit delta mode",
			PT_double, "maximum_speed_error", PADDR(speed_error),PT_DESCRIPTION,"maximum speed error for the steady state model",
			PT_double, "wr", PADDR(wr),PT_DESCRIPTION,"rotor speed",
			PT_enumeration,"motor_status",PADDR(motor_status),PT_DESCRIPTION,"the current status of the motor",
				PT_KEYWORD,"RUNNING",(enumeration)statusRUNNING,
				PT_KEYWORD,"STALLED",(enumeration)statusSTALLED,
				PT_KEYWORD,"TRIPPED",(enumeration)statusTRIPPED,
				PT_KEYWORD,"OFF",(enumeration)statusOFF,
			PT_int32,"motor_status_number",PADDR(motor_status),PT_DESCRIPTION,"the current status of the motor as an integer",
			PT_enumeration,"motor_override",PADDR(motor_override),PT_DESCRIPTION,"override function to dictate if motor is turned off or on",
				PT_KEYWORD,"ON",(enumeration)overrideON,
				PT_KEYWORD,"OFF",(enumeration)overrideOFF,
			PT_enumeration,"motor_operation_type",PADDR(motor_op_mode),PT_DESCRIPTION,"current operation type of the motor - deltamode related",
				PT_KEYWORD,"SINGLE-PHASE",(enumeration)modeSPIM,
				PT_KEYWORD,"THREE-PHASE",(enumeration)modeTPIM,
			PT_enumeration,"triplex_connection_type",PADDR(triplex_connection_type),PT_DESCRIPTION,"Describes how the motor will connect to the triplex devices",
				PT_KEYWORD,"TRIPLEX_1N",(enumeration)TPNconnected1N,
				PT_KEYWORD,"TRIPLEX_2N",(enumeration)TPNconnected2N,
				PT_KEYWORD,"TRIPLEX_12",(enumeration)TPNconnected12,

			// These model parameters are published as hidden
			PT_double, "wb[rad/s]", PADDR(wb),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"base speed",
			PT_double, "ws", PADDR(ws),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"system speed",
			PT_complex, "psi_b", PADDR(psi_b),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"backward rotating flux",
			PT_complex, "psi_f", PADDR(psi_f),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"forward rotating flux",
			PT_complex, "psi_dr", PADDR(psi_dr),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Rotor d axis flux",
			PT_complex, "psi_qr", PADDR(psi_qr),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Rotor q axis flux",
			PT_complex, "Ids", PADDR(Ids),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"time before tripped motor reconnects",
			PT_complex, "Iqs", PADDR(Iqs),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"time before tripped motor reconnects",
			PT_complex, "If", PADDR(If),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"forward current",
			PT_complex, "Ib", PADDR(Ib),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"backward current",
			PT_complex, "Is", PADDR(Is),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor current",
			PT_complex, "Ss", PADDR(Ss),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor power",
			PT_double, "electrical_torque", PADDR(Telec),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"electrical torque",
			PT_complex, "Vs", PADDR(Vs),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor voltage",
			PT_bool, "motor_trip", PADDR(motor_trip),PT_DESCRIPTION,"boolean variable to check if motor is tripped",
			PT_double, "trip", PADDR(trip),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"current time in tripped state",
			PT_double, "reconnect", PADDR(reconnect),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"current time since motor was tripped",

			//**** YUAN'S THREE-PHASE INDUCTION MOTOR VARIABLES COULD GO HERE ****//
			//************** Begin_Yuan's TPIM model ********************//
			PT_double, "rs[pu]", PADDR(rs),PT_DESCRIPTION,"stator resistance",
			PT_double, "rr[pu]", PADDR(rr),PT_DESCRIPTION,"rotor resistance",
			PT_double, "lm[pu]", PADDR(lm),PT_DESCRIPTION,"magnetizing reactance",
			PT_double, "lls[pu]", PADDR(lls),PT_DESCRIPTION,"stator leakage reactance",
			PT_double, "llr[pu]", PADDR(llr),PT_DESCRIPTION,"rotor leakage reactance",
			PT_double, "TPIM_rated_mechanical_Load_torque", PADDR(TLrated),PT_DESCRIPTION,"rated mechanical load torque applied to three-phase induction motor",
			PT_double, "friction_coefficient", PADDR(Kfric),PT_DESCRIPTION,"coefficient of speed-dependent torque",
			PT_enumeration,"TPIM_initial_status",PADDR(TPIM_initial_status),PT_DESCRIPTION,"initial status of three-phase induction motor: RUNNING or STATIONARY",
				PT_KEYWORD,"RUNNING",(enumeration)initialRUNNING,
				PT_KEYWORD,"STATIONARY",(enumeration)initialSTATIONARY,
			// share declarations of interation_count, DM_volt_trig_per, DM_speed_trig_per, DM_volt_exit_per, DM_speed_exit_per, speed_error with SPIM model
			// share declarations of motor_status, motor_override and motor_op_mode with SPIM model
			// share declarations of motor_trip

//			PT_double, "trip_time[s]", PADDR(trip_time),PT_DESCRIPTION,"time motor can stay stalled before tripping off ",
//			PT_double, "reconnect_time[s]", PADDR(reconnect_time),PT_DESCRIPTION,"time before tripped motor reconnects",

			// These model parameters are published as hidden
			// share declarations of wb with SPIM
			PT_double, "wsyn", PADDR(wsyn),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"system frequency in pu",
			PT_complex, "phips", PADDR(phips),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"positive sequence stator flux",
			PT_complex, "phins_cj", PADDR(phins_cj),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"conjugate of negative sequence stator flux",
			PT_complex, "phipr", PADDR(phipr),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"positive sequence rotor flux",
			PT_complex, "phinr_cj", PADDR(phinr_cj),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"conjugate of negative sequence rotor flux",
			PT_double, "omgr0", PADDR(omgr0),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"dc component of rotor speed",
			PT_double, "TL", PADDR(TL),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"actually applied mechanical torque",
			PT_complex, "Ias", PADDR(Ias),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor phase-a stator current",
			PT_complex, "Ibs", PADDR(Ibs),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor phase-b stator current",
			PT_complex, "Ics", PADDR(Ics),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor phase-c stator current",
			PT_complex, "Smt", PADDR(Smt),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor complex power",
			PT_complex, "Vas", PADDR(Vas),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor phase-a stator-to-ground voltage",
			PT_complex, "Vbs", PADDR(Vbs),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor phase-b stator-to-ground voltage",
			PT_complex, "Vcs", PADDR(Vcs),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor phase-c stator-to-ground voltage",
			PT_complex, "Ips", PADDR(Ips),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"positive sequence stator current",
			PT_complex, "Ipr", PADDR(Ipr),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"positive sequence rotor current",
			PT_complex, "Ins_cj", PADDR(Ins_cj),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"conjugate of negative sequence stator current",
			PT_complex, "Inr_cj", PADDR(Inr_cj),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"conjugate of negative sequence rotor current",
			PT_double, "Ls", PADDR(Ls),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"stator synchronous reactance",
			PT_double, "Lr", PADDR(Lr),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"rotor synchronous reactance",
			PT_double, "sigma1", PADDR(sigma1),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"intermediate variable 1 associated with synch. react.",
			PT_double, "sigma2", PADDR(sigma2),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"intermediate variable 2 associated with synch. react.",
//			PT_bool, "motor_trip", PADDR(motor_trip),PT_DESCRIPTION,"boolean variable to check if motor is tripped",
//			PT_double, "trip", PADDR(trip),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"current time in tripped state",
//			PT_double, "reconnect", PADDR(reconnect),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"current time since motor was tripped",

			//************** End_Yuan's TPIM model ********************//

			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"delta_linkage_node", (FUNCTIONADDR)delta_linkage)==NULL)
			GL_THROW("Unable to publish motor delta_linkage function");
		if (gl_publish_function(oclass,	"delta_freq_pwr_object", (FUNCTIONADDR)delta_frequency_node)==NULL)
			GL_THROW("Unable to publish motor deltamode function");
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_motor)==NULL)
			GL_THROW("Unable to publish motor deltamode function");
		if (gl_publish_function(oclass,	"pwr_object_swing_swapper", (FUNCTIONADDR)swap_node_swing_status)==NULL)
			GL_THROW("Unable to publish motor swing-swapping function");
		if (gl_publish_function(oclass,	"attach_vfd_to_pwr_object", (FUNCTIONADDR)attach_vfd_to_node)==NULL)
			GL_THROW("Unable to publish motor VFD attachment function");
    }
}

int motor::create()
{
	int result = node::create();
	last_cycle = 0;

	// Default parameters
	motor_override = overrideON;  // share the variable with TPIM
	motor_trip = 0;  // share the variable with TPIM
	Pbase = 3500;                    
	n = 1.22;              
	Rds =0.0365;           
	Rqs = 0.0729;          
	Rr =0.0486;             
	Xm=2.28;               
	Xr=2.33;               
	Xc1 = -2.779;           
	Xc2 = -0.7;            
	Xd_prime = 0.1033;      
	Xq_prime =0.1489;       
	bsat = 0.7212;  
	Asat = 5.6;
	H=0.04;
	To_prime =0.1212;   
	trip_time = 10;        
	reconnect_time = 300;
	interation_count = 1000;  // share the variable with TPIM
	Tmech = 1.0448;
	cap_run_speed_percentage = 50;
	DM_volt_trig_per = 80; // share the variable with TPIM
	DM_speed_trig_per = 80;  // share the variable with TPIM
	DM_volt_exit_per = 95; // share the variable with TPIM
	DM_speed_exit_per = 95; // share the variable with TPIM
	speed_error = 1e-10; // share the variable with TPIM

	// initial parameter for internal model
	trip = 0;
	reconnect = 0;
	psi_b = complex(0,0);
    psi_f = complex(0,0);
    psi_dr = complex(0,0); 
    psi_qr = complex(0,0); 
    Ids = complex(0,0);
    Iqs = complex(0,0);  
    If = complex(0,0);
    Ib = complex(0,0);
    Is = complex(0,0);
    Ss = complex(0,0);
    Telec = 0; 
    wr = 0;

    //Mode initialization
    motor_op_mode = modeSPIM; // share the variable with TPIM

    /************ YUAN ***************/
    //Three-phase induction motor parameters would go here
    //************** Begin_Yuan's TPIM model ********************//
    rs = 0.01;  // pu
    lls = 0.1;  //  pu
    lm = 3.0;  // pu
    rr = 0.005;  // pu
    llr = 0.08;  // pu

    //************* Parameters are for 3000 W motor - reconciled variable with SPIM - figure out how to trigger this
    //PbTPIM = 3000; // W
    //Hs = 0.1;  // s	-- this needs to be reconciled as well
    Kfric = 0.0;  // pu
	TL = 0.95;  // pu
	TLrated = 0.95;  //pu
	phips = complex(0,0); // pu
	phins_cj = complex(0,0); // pu
	phipr = complex(0,0); // pu
	phinr_cj = complex(0,0); // pu
	omgr0 = 0.97; // pu
	Ias = complex(0,0); // pu
	Ibs = complex(0,0); // pu
	Ics = complex(0,0); // pu
	Smt = complex(0,0); // VA
	Vas = complex(0,0); // pu
	Vbs = complex(0,0); // pu
	Vcs = complex(0,0); // pu
	Ips = complex(0,0); // pu
	Ipr = complex(0,0); // pu
	Ins_cj = complex(0,0); // pu
	Inr_cj = complex(0,0); // pu
	Ls = 0.0; // pu
	Lr = 0.0; // pu
	sigma1 = 0.0; // pu
	sigma2 = 0.0; // pu
	wref = 0.0; // pu
	wsyn = 1.0; // pu
	TPIM_initial_status = initialRUNNING;

    //************** End_Yuan's TPIM model ********************//

	triplex_connected = false;	//By default, not connected to triplex
	triplex_connection_type = TPNconnected12;	//If it does end up being triplex, we default it to L1-L2

	return result;
}

int motor::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	int result = node::init(parent);

	// Check what phases are connected on this motor
	double num_phases = 0;
	num_phases = num_phases+has_phase(PHASE_A);
	num_phases = num_phases+has_phase(PHASE_B);
	num_phases = num_phases+has_phase(PHASE_C);
	
	// error out if we have more than one phase
	if (num_phases == 1)
	{
		motor_op_mode = modeSPIM;
	}
	else if (num_phases == 3)
	{
		motor_op_mode = modeTPIM;
	}
	else
	{
		GL_THROW("motor:%s -- only single-phase or three-phase motors are support",(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The motor only supports single-phase and three-phase motors at this time.  Please use one of these connection types.
		*/
	}

	// determine the specific phase this motor is connected to
	// --- @TODO: Implement support for split phase

	if (motor_op_mode == modeSPIM)
	{
		if (has_phase(PHASE_S))
		{
			connected_phase = -1;		//Arbitrary
			triplex_connected = true;	//Flag us as triplex
		}
		else	//Three-phase
		{
			//Affirm the triplex flag is not set
			triplex_connected = false;

			if (has_phase(PHASE_A)) {
				connected_phase = 0;
			}
			if (has_phase(PHASE_B)) {
				connected_phase = 1;
			}
			if (has_phase(PHASE_C)) {
				connected_phase = 2;
			}
		}
	}
	else	//Three-phase diagnostics
	{
		/***** YUAN ********/
		//Any error checking or initial coding would go here
		//************** Begin_Yuan's TPIM model ********************//
		if(has_phase(PHASE_A) == 0 || has_phase(PHASE_B) == 0 || has_phase(PHASE_C) == 0) {
			GL_THROW("At least 1 phase connection is missing for the TPIM model");
		}
		//************** End_Yuan's TPIM model ********************//

	}

	//Parameters
	if ((triplex_connected == true) && (triplex_connection_type == TPNconnected12))
	{
		Ibase = Pbase/(2.0*nominal_voltage);	//To reflect LL connection
	}
	else	//"Three-phase" or "normal" triplex
	{
		Ibase = Pbase/nominal_voltage;
	}

	wb=2*PI*nominal_frequency;
	cap_run_speed = (cap_run_speed_percentage*wb)/100;
	DM_volt_trig = (DM_volt_trig_per)/100;
	DM_speed_trig = (DM_speed_trig_per*wb)/100;
	DM_volt_exit = (DM_volt_exit_per)/100;
	DM_speed_exit = (DM_speed_exit_per*wb)/100;

	//************** Begin_Yuan's TPIM model ********************//
	IbTPIM = Pbase / nominal_voltage / 3.0;  // A, I guess nominal_voltage is line-to-ground here, right ?
	Ls = lls + lm; // pu
	Lr = llr + lm; // pu
	sigma1 = Ls - lm * lm / Lr; // pu
	sigma2 = Lr - lm * lm / Ls; // pu

	if (motor_op_mode == modeTPIM && TPIM_initial_status == initialSTATIONARY){
		DM_volt_trig_per = 110;  // percent
		DM_speed_trig_per = 110;
		DM_volt_exit_per = 110;
		DM_speed_exit_per = 110;
		omgr0 = 0.0; // pu
		TL = 0.0;  // pu
	}
	else if (motor_op_mode == modeTPIM && TPIM_initial_status == initialRUNNING){
		omgr0 = sqrt(TLrated); // pu
		TL = TLrated;
	}

	omgr0_prev = omgr0;
	//************** End_Yuan's TPIM model ********************//

	return result;
}

int motor::isa(char *classname)
{
	return strcmp(classname,"motor")==0 || node::isa(classname);
}

TIMESTAMP motor::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	// we need to initialize the last cycle variable
	if (last_cycle == 0) {
		last_cycle = t1;
	}
	else if ((double)t1 > last_cycle) {
		delta_cycle = (double)t1-last_cycle;
	}
	
	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::presync(t1);
	return result;
}

TIMESTAMP motor::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	// update voltage and frequency
	updateFreqVolt();

	if (motor_op_mode == modeSPIM)
	{
		if((double)t1 == last_cycle) { // if time did not advance, load old values
			SPIMreinitializeVars();
		}
		else if((double)t1 > last_cycle){ // time advanced, time to update varibles
			SPIMupdateVars();
		}
		else {
			gl_error("current time is less than previous time");
		}

		// update protection
		SPIMUpdateProtection(delta_cycle);

		if (motor_override == overrideON && ws > 1 && Vs.Mag() > 0.1 && motor_trip == 0) { // motor is currently connected and grid conditions are not "collapsed"
			// run the steady state solver
			SPIMSteadyState(t1);

			// update current draw
			if (triplex_connected==true)
			{
				//See which type of triplex
				if (triplex_connection_type == TPNconnected1N)
				{
					pre_rotated_current[0] = Is*Ibase;
				}
				else if (triplex_connection_type == TPNconnected2N)
				{
					pre_rotated_current[1] = Is*Ibase;
				}
				else	//Assume it is 12 now
				{
					pre_rotated_current[2] = Is*Ibase;
				}
			}
			else	//"Three-phase" connection
			{
				pre_rotated_current[connected_phase] = Is*Ibase;
			}
		}
		else { // motor is currently disconnected
			if (triplex_connected==true)
			{
				//See which type of triplex
				if (triplex_connection_type == TPNconnected1N)
				{
					pre_rotated_current[0] = complex(0.0,0.0);
				}
				else if (triplex_connection_type == TPNconnected2N)
				{
					pre_rotated_current[1] = complex(0.0,0.0);
				}
				else	//Assume it is 12 now
				{
					pre_rotated_current[2] = complex(0.0,0.0);
				}

				//Set off
				SPIMStateOFF();
			}
			else	//"Three-phase" connection
			{
				pre_rotated_current[connected_phase] = 0;
				SPIMStateOFF();
			}
		}

		// update motor status
		SPIMUpdateMotorStatus();
	}

	//************** Begin_Yuan's TPIM model ********************//
	else	//Must be three-phase
	{
		/**** YUAN ****/
		//Three-phase steady-state code would go here
		if((double)t1 == last_cycle) { // if time did not advance, load old values
			TPIMreinitializeVars();
		}
		else if((double)t1 > last_cycle){ // time advanced, time to update variables
			TPIMupdateVars();
		}
		else {
			gl_error("current time is less than previous time");
		}

		// update protection
		TPIMUpdateProtection(delta_cycle);

		if (motor_override == overrideON && wsyn > 0.1 &&  Vas.Mag() > 0.1 && Vbs.Mag() > 0.1 && Vcs.Mag() > 0.1 &&
			motor_trip == 0) { // motor is currently connected and grid conditions are not "collapsed"
			// This needs to be re-visited to determine the lower bounds of wsyn and Vas, Vbs and Vcs

			// run the steady state solver
			TPIMSteadyState(t1);

			// update current draw -- might need to be pre_rotated_current
			pre_rotated_current[0] = Ias*IbTPIM; // A
			pre_rotated_current[1] = Ibs*IbTPIM; // A
			pre_rotated_current[2] = Ics*IbTPIM; // A
		}
		else { // motor is currently disconnected
			pre_rotated_current[0] = 0; // A
			pre_rotated_current[1] = 0; // A
			pre_rotated_current[2] = 0; // A
			TPIMStateOFF();
		}

		// update motor status
		TPIMUpdateMotorStatus();
	}
	//************** End_Yuan's TPIM model ********************//

	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::sync(t1);

	if ((double)t1 > last_cycle) {	
		last_cycle = (double)t1;
	}

	// figure out if we need to enter delta mode on the next pass
	if (motor_op_mode == modeSPIM)
	{
		if ((Vs.Mag() < DM_volt_trig || wr < DM_speed_trig) && deltamode_inclusive)
		{
			// we should not enter delta mode if the motor is tripped or not close to reconnect
			if (motor_trip == 1 && reconnect < reconnect_time-1) {
				return result;
			}

			// we are not tripped and the motor needs to enter delta mode to capture the dynamics
			schedule_deltamode_start(t1);
			return t1;
		}
		else
		{
			return result;
		}
	}
	//************** Begin_Yuan's TPIM model ********************//
	else	//Must be three-phase
	{
		/******* YUAN ********/
		//This may need to be updated for three-phase
		if ( (Vas.Mag() < DM_volt_trig_per / 100 || Vbs.Mag() < DM_volt_trig_per / 100 ||
				Vcs.Mag() < DM_volt_trig_per / 100 || omgr0 < DM_speed_trig_per / 100)
				&& deltamode_inclusive)
		{
			// we should not enter delta mode if the motor is tripped or not close to reconnect
			if (motor_trip == 1 && reconnect < reconnect_time-1) {
				return result;
			}

			// we are not tripped and the motor needs to enter delta mode to capture the dynamics
			schedule_deltamode_start(t1);
			return t1;
		}
		else
		{
			return result;
		}
	}
	//************** End_Yuan's TPIM model ********************//
}

TIMESTAMP motor::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::postsync(t1);
	return result;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////

//Module-level call
SIMULATIONMODE motor::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	OBJECT *hdr = OBJECTHDR(this);
	STATUS return_status_val;

	// make sure to capture the current time
	curr_delta_time = gl_globaldeltaclock;

	// I need the time delta in seconds
	double deltaTime = (double)dt/(double)DT_SECOND;

	//mostly for GFA functionality calls
	if ((iteration_count_val==0) && (interupdate_pos == false) && (fmeas_type != FM_NONE)) 
	{
		//Update frequency calculation values (if needed)
		memcpy(&prev_freq_state,&curr_freq_state,sizeof(FREQM_STATES));
	}

	//In the first call we need to initilize the dynamic model
	if ((delta_time==0) && (iteration_count_val==0) && (interupdate_pos == false))	//First run of new delta call
	{
		//Call presync-equivalent items
		NR_node_presync_fxn(0);

		if (fmeas_type != FM_NONE) {
			//Initialize dynamics
			init_freq_dynamics();
		}

		if (motor_op_mode == modeSPIM)
		{
			// update voltage and frequency
			updateFreqVolt();

			// update previous values for the model
			SPIMupdateVars();

			SPIMSteadyState(curr_delta_time);

			// update motor status
			SPIMUpdateMotorStatus();
		}

		//************** Begin_Yuan's TPIM model ********************//
		else	//Three-phase
		{
			/******* YUAN *********/
			//This would be where the three-phase deltamode/dynamic code would go -- first run/deltamode initialization stuff
			// update voltage and frequency
			updateFreqVolt();

			// update previous values for the model
			TPIMupdateVars();

			TPIMSteadyState(curr_delta_time);

			// update motor status
			TPIMUpdateMotorStatus();
		}
		//************** End_Yuan's TPIM model ********************//

	}//End first pass and timestep of deltamode (initial condition stuff)

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Call presync-equivalent items
		if (delta_time>0) {
			NR_node_presync_fxn(0);

			// update voltage and frequency
			updateFreqVolt();
		}

		if (motor_op_mode == modeSPIM)
		{
			// if deltaTime is not small enough we will run into problems
			if (deltaTime > 0.0003) {
				gl_warning("Delta time for the SPIM model needs to be lower than 0.0003 seconds");
			}

			if(curr_delta_time == last_cycle) { // if time did not advance, load old values
				SPIMreinitializeVars();
			}
			else if(curr_delta_time > last_cycle){ // time advanced, time to update varibles
				SPIMupdateVars();
			}
			else {
				gl_error("current time is less than previous time");
			}

			// update protection
			if (delta_time>0) {
				SPIMUpdateProtection(deltaTime);
			}

			if (motor_override == overrideON && ws > 1 && Vs.Mag() > 0.1 && motor_trip == 0) { // motor is currently connected and grid conditions are not "collapsed"
				// run the dynamic solver
				SPIMDynamic(curr_delta_time, deltaTime);

				// update current draw
				if (triplex_connected == true)
				{
					//See which type of triplex
					if (triplex_connection_type == TPNconnected1N)
					{
						pre_rotated_current[0] = Is*Ibase;
					}
					else if (triplex_connection_type == TPNconnected2N)
					{
						pre_rotated_current[1] = Is*Ibase;
					}
					else	//Assume it is 12 now
					{
						pre_rotated_current[2] = Is*Ibase;
					}
				}
				else	//"Three-phase" connection
				{
					pre_rotated_current[connected_phase] = Is*Ibase;
				}
			}
			else { // motor is currently disconnected
				if (triplex_connected == true)
				{
					//See which type of triplex
					if (triplex_connection_type == TPNconnected1N)
					{
						pre_rotated_current[0] = complex(0.0,0.0);
					}
					else if (triplex_connection_type == TPNconnected2N)
					{
						pre_rotated_current[1] = complex(0.0,0.0);
					}
					else	//Assume it is 12 now
					{
						pre_rotated_current[2] = complex(0.0,0.0);
					}

					//Set off
					SPIMStateOFF();
				}
				else	//"Three-phase" connection
				{
					pre_rotated_current[connected_phase] = 0;
					SPIMStateOFF();
				}
			}

			// update motor status
			SPIMUpdateMotorStatus();
		}

		//************** Begin_Yuan's TPIM model ********************//
		else 	//Must be three-phase
		{
			/*************** YUAN ***********************/
			//This is where other three-phase code would go (predictor step?)
			// if deltaTime is not small enough we will run into problems
			if (deltaTime > 0.0005) {
				gl_warning("Delta time for the TPIM model needs to be lower than 0.0005 seconds");
			}

			if(curr_delta_time == last_cycle) { // if time did not advance, load old values
				TPIMreinitializeVars();
			}
			else if(curr_delta_time > last_cycle){ // time advanced, time to update varibles
				TPIMupdateVars();
			}
			else {
				gl_error("current time is less than previous time");
			}

			// update protection
			if (delta_time>0) {
				TPIMUpdateProtection(deltaTime);
			}


			if (motor_override == overrideON && wsyn > 0.1 && Vas.Mag() > 0.1 && Vbs.Mag() > 0.1 && Vcs.Mag() > 0.1 &&
					motor_trip == 0) { // motor is currently connected and grid conditions are not "collapsed"
				// run the dynamic solver
				TPIMDynamic(curr_delta_time, deltaTime);

				// update current draw -- pre_rotated_current
				pre_rotated_current[0] = Ias*IbTPIM; // A
				pre_rotated_current[1] = Ibs*IbTPIM; // A
				pre_rotated_current[2] = Ics*IbTPIM; // A
			}
			else { // motor is currently disconnected
				pre_rotated_current[0] = 0; // A
				pre_rotated_current[1] = 0; // A
				pre_rotated_current[2] = 0; // A;
				TPIMStateOFF();
			}

			// update motor status
			TPIMUpdateMotorStatus();
		}
		//************** End_Yuan's TPIM model ********************//


		//Call sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		if (curr_delta_time > last_cycle) {
			last_cycle = curr_delta_time;
		}

		return SM_DELTA;
	}
	else // After powerflow call
	{
		//Perform postsync-like updates on the values
		BOTH_node_postsync_fxn(hdr);
	
		//Frequency measurement stuff
		if (fmeas_type != FM_NONE)
		{
			return_status_val = calc_freq_dynamics(deltaTime);

			//Check it
			if (return_status_val == FAILED)
			{
				return SM_ERROR;
			}
		}//End frequency measurement desired
		//Default else -- don't calculate it

		if (motor_op_mode == modeSPIM)
		{
			// figure out if we need to exit delta mode on the next pass
			if (Vs.Mag() > DM_volt_exit && wr > DM_speed_exit)
			{
				// we return to steady state if the voltage and speed is good
				return SM_EVENT;
			} else if (motor_trip == 1 && reconnect < reconnect_time-1) {
				// we return to steady state if the motor is tripped
				return SM_EVENT;
			} else if (motor_override == overrideOFF) {
				//We're off at the moment, so assume back to event driven mode
				return SM_EVENT;
			}

			//Default - stay in deltamode
			return SM_DELTA;
		}

		//************** Begin_Yuan's TPIM model ********************//
		else	//Must be three-phase
		{
			/**** YUAN *****/
			//This may need to be revisited - check to see if it should stay in deltamode or not
			// figure out if we need to exit delta mode on the next pass
			if (Vas.Mag() > DM_volt_exit_per/100 && Vbs.Mag() > DM_volt_exit_per/100 && Vcs.Mag() > DM_volt_exit_per/100
					&& omgr0 > DM_speed_exit_per/100)
			{
				return SM_EVENT;
			}
			else if (motor_trip == 1 && reconnect < reconnect_time-1) {
				// we return to steady state if the motor is tripped
				return SM_EVENT;
			}
			else if (motor_override == overrideOFF) {
				//We're off at the moment, so assume back to event driven mode
				return SM_EVENT;
			}


			//Default - stay in deltamode
			return SM_DELTA;
		}
		//************** End_Yuan's TPIM model ********************//
	}
}

// function to update motor frequency and voltage
void motor::updateFreqVolt() {
	// update voltage and frequency
	if (motor_op_mode == modeSPIM)
	{
		if ( (SubNode == CHILD) || (SubNode == DIFF_CHILD) ) // if we have a parent, reference the voltage and frequency of the parent
		{
			node *parNode = OBJECTDATA(SubNodeParent,node);
			if (triplex_connected == true)
			{
				//See which type of triplex
				if (triplex_connection_type == TPNconnected1N)
				{
					Vs = parNode->voltage[0]/parNode->nominal_voltage;
					ws = parNode->curr_freq_state.fmeas[0]*2*PI;
				}
				else if (triplex_connection_type == TPNconnected2N)
				{
					Vs = parNode->voltage[1]/parNode->nominal_voltage;
					ws = parNode->curr_freq_state.fmeas[1]*2*PI;
				}
				else	//Assume it is 12 now
				{
					Vs = parNode->voltaged[0]/(2.0*parNode->nominal_voltage);	//To reflect LL connection
					ws = parNode->curr_freq_state.fmeas[0]*2*PI;
				}
			}
			else	//"Three-phase" connection
			{
				Vs = parNode->voltage[connected_phase]/parNode->nominal_voltage;
				ws = parNode->curr_freq_state.fmeas[connected_phase]*2*PI;
			}
		}
		else // No parent, use our own voltage
		{
			if (triplex_connected == true)
			{
				//See which type of triplex
				if (triplex_connection_type == TPNconnected1N)
				{
					Vs = voltage[0]/nominal_voltage;
					ws = curr_freq_state.fmeas[0]*2*PI;
				}
				else if (triplex_connection_type == TPNconnected2N)
				{
					Vs = voltage[1]/nominal_voltage;
					ws = curr_freq_state.fmeas[1]*2*PI;
				}
				else	//Assume it is 12 now
				{
					Vs = voltaged[0]/(2.0*nominal_voltage);	//To reflect LL connection
					ws = curr_freq_state.fmeas[0]*2*PI;
				}
			}
			else 	//"Three-phase" connection
			{
				Vs = voltage[connected_phase]/nominal_voltage;
				ws = curr_freq_state.fmeas[connected_phase]*2*PI;
			}
		}
	}
	//************** Begin_Yuan's TPIM model ********************//
	else //TPIM model
	{
		if ( (SubNode == CHILD) || (SubNode == DIFF_CHILD) ) // if we have a parent, reference the voltage and frequency of the parent
		{
			node *parNode = OBJECTDATA(SubNodeParent,node);
			// obtain 3-phase voltages
			Vas = parNode->voltage[0]/parNode->nominal_voltage;
			Vbs = parNode->voltage[1]/parNode->nominal_voltage;
			Vcs = parNode->voltage[2]/parNode->nominal_voltage;
			// obtain frequency in pu, take the average of 3-ph frequency ?
			wsyn = (parNode->curr_freq_state.fmeas[0]+ parNode->curr_freq_state.fmeas[1] + parNode->curr_freq_state.fmeas[2]) / 60.0 / 3.0;
		}
		else // No parent, use our own voltage
		{
			Vas = voltage[0]/nominal_voltage;
			Vbs = voltage[1]/nominal_voltage;
			Vcs = voltage[2]/nominal_voltage;
			wsyn = (curr_freq_state.fmeas[0] + curr_freq_state.fmeas[1] + curr_freq_state.fmeas[2]) / 60.0 / 3.0;
		}
	}
	//************** End_Yuan's TPIM model ********************//
}

// function to update the previous values for the motor model
void motor::SPIMupdateVars() {
	wr_prev = wr;
	Telec_prev = Telec; 
	psi_dr_prev = psi_dr;
	psi_qr_prev = psi_qr;
	psi_f_prev = psi_f; 
	psi_b_prev = psi_b;
	Iqs_prev = Iqs;
	Ids_prev =Ids;
	If_prev = If;
	Ib_prev = Ib;
	Is_prev = Is;
	Ss_prev = Ss;
	reconnect_prev = reconnect;
	motor_trip_prev = motor_trip;
	trip_prev = trip;
	psi_sat_prev = psi_sat;
}


//************** Begin_Yuan's TPIM model ********************//
void motor::TPIMupdateVars() {
	phips_prev = phips;
	phins_cj_prev = phins_cj;
	phipr_prev = phipr;
	phinr_cj_prev = phinr_cj;
	omgr0_prev = omgr0;
	Ips_prev = Ips;
	Ipr_prev = Ipr;
	Ins_cj_prev = Ins_cj;
	Inr_cj_prev = Inr_cj;

	reconnect_prev = reconnect;
	motor_trip_prev = motor_trip;
	trip_prev = trip;

}
//************** End_Yuan's TPIM model ********************//


// function to reinitialize values for the motor model
void motor::SPIMreinitializeVars() {
	wr = wr_prev;
	Telec = Telec_prev; 
	psi_dr = psi_dr_prev;
	psi_qr = psi_qr_prev;
	psi_f = psi_f_prev; 
	psi_b = psi_b_prev;
	Iqs = Iqs_prev;
	Ids =Ids_prev;
	If = If_prev;
	Ib = Ib_prev;
	Is = Is_prev;
	Ss = Ss_prev;
	reconnect = reconnect_prev;
	motor_trip = motor_trip_prev;
	trip = trip_prev;
	psi_sat = psi_sat_prev;
}


//************** Begin_Yuan's TPIM model ********************//
void motor::TPIMreinitializeVars() {
	phips = phips_prev;
	phins_cj = phins_cj_prev;
	phipr = phipr_prev;
	phinr_cj = phinr_cj_prev;
	omgr0 = omgr0_prev;
	Ips = Ips_prev;
	Ipr = Ipr_prev;
	Ins_cj = Ins_cj_prev;
	Inr_cj = Inr_cj_prev;

	reconnect = reconnect_prev;
	motor_trip = motor_trip_prev;
	trip = trip_prev;

}
//************** End_Yuan's TPIM model ********************//


// function to update the status of the motor
void motor::SPIMUpdateMotorStatus() {
	if (motor_override == overrideOFF || ws <= 1 || Vs.Mag() <= 0.1)
		motor_status = statusOFF;
	else if (wr > 1) {
		motor_status = statusRUNNING;
	}
	else if (motor_trip == 1) {
		motor_status = statusTRIPPED;
	}
	else {
		motor_status = statusSTALLED;
	}	
}

//************** Begin_Yuan's TPIM model ********************//
void motor::TPIMUpdateMotorStatus() {
	if (motor_override == overrideOFF || wsyn <= 0.1 ||
			Vas.Mag() <= 0.1 || Vbs.Mag() <= 0.1 || Vcs.Mag() <= 0.1) {
		motor_status = statusOFF;
	}
	else if (omgr0 > 0.0) {
		motor_status = statusRUNNING;
	}
	else if (motor_trip == 1) {
		motor_status = statusTRIPPED;
	}
	else {
		motor_status = statusSTALLED;
	}
}
//************** End_Yuan's TPIM model ********************//


// function to update the protection of the motor
void motor::SPIMUpdateProtection(double delta_time) {	
	if (motor_override == overrideON) { 
		if (wr < 1 && motor_trip == 0 && ws > 1 && Vs.Mag() > 0.1) { // conditions for counting up the time trip timer
			trip = trip + delta_time; // count up by the time since last pass
		}
		else if ( (wr >= 1 && motor_trip == 0) || ws <= 1 || Vs.Mag() <= 0.1) { // conditions for counting down the time trip timer
			trip = trip - (delta_time / (reconnect_time/trip_time)) < 0 ? 0 : trip - (delta_time / (reconnect_time/trip_time)); // count down by the time since last cycle scaled by the difference in trip and reconnect times
		}
	}
	else { // motor is off so it is "cooling" down
		trip = trip - (delta_time / (reconnect_time/trip_time)) < 0 ? 0 : trip - (delta_time / (reconnect_time/trip_time)); // count down by the time since last cycle scaled by the difference in trip and reconnect times
	}

	if (motor_trip == 1) {
		reconnect = reconnect + delta_time; // count up by the time since last pass
	}

	if (trip >= trip_time) { // check if we should trip the motor
		motor_trip = 1;
		trip = 0;
	}
	if (reconnect >= reconnect_time) { // check if we should reconnect the motor
		motor_trip = 0;
		reconnect = 0;
	}
}

//************** Begin_Yuan's TPIM model ********************//
// function to update the protection of the motor
void motor::TPIMUpdateProtection(double delta_time) {
	if (motor_override == overrideON) {
		if (omgr0 < 0.01 && motor_trip == 0 && wsyn > 0.1 && Vas.Mag() > 0.1 && Vbs.Mag() > 0.1 && Vcs.Mag() > 0.1) { // conditions for counting up the time trip timer
			trip = trip + delta_time; // count up by the time since last pass
		}
		else if ( (omgr0 >= 0.01 && motor_trip == 0) || wsyn <= 0.1 || Vas.Mag() <= 0.1 || Vbs.Mag() <= 0.1 || Vcs.Mag() <= 0.1) { // conditions for counting down the time trip timer
			trip = trip - (delta_time / (reconnect_time/trip_time)) < 0 ? 0 : trip - (delta_time / (reconnect_time/trip_time)); // count down by the time since last cycle scaled by the difference in trip and reconnect times
		}
	}
	else { // motor is off so it is "cooling" down
		trip = trip - (delta_time / (reconnect_time/trip_time)) < 0 ? 0 : trip - (delta_time / (reconnect_time/trip_time)); // count down by the time since last cycle scaled by the difference in trip and reconnect times
	}

	if (motor_trip == 1) {
		reconnect = reconnect + delta_time; // count up by the time since last pass
	}

	if (trip >= trip_time) { // check if we should trip the motor
		motor_trip = 1;
		trip = 0;
	}
	if (reconnect >= reconnect_time) { // check if we should reconnect the motor
		motor_trip = 0;
		reconnect = 0;
	}
}
//************** End_Yuan's TPIM model ********************//

// function to ensure that internal model states are zeros when the motor is OFF
void motor::SPIMStateOFF() {
	psi_b = complex(0,0);
    psi_f = complex(0,0);
    psi_dr = complex(0,0); 
    psi_qr = complex(0,0); 
    Ids = complex(0,0);
    Iqs = complex(0,0);  
    If = complex(0,0);
    Ib = complex(0,0);
    Is = complex(0,0);
    Ss = complex(0,0);
    Telec = 0; 
    wr = 0;
}

//************** Begin_Yuan's TPIM model ********************//
void motor::TPIMStateOFF() {
	phips = complex(0,0);
	phins_cj = complex(0,0);
	phipr = complex(0,0);
	phinr_cj = complex(0,0);
	omgr0 = 0;
	Ips = complex(0,0);
	Ipr = complex(0,0);
	Ins_cj = complex(0,0);
	Inr_cj = complex(0,0);
}
//************** End_Yuan's TPIM model ********************//


// Function to calculate the solution to the steady state SPIM model
void motor::SPIMSteadyState(TIMESTAMP t1) {
	double wr_delta = 1;
    psi_sat = 1;
	double psi = -1;
    double count = 1;
	double Xc = -1;

	while (fabs(wr_delta) > speed_error && count < interation_count) {
        count++;
        
        //Kick in extra capacitor if we droop in speed
		if (wr < cap_run_speed) {
           Xc = Xc2;
		}
		else {
           Xc = Xc1; 
		}

		TF[0] = (complex(0,1)*Xd_prime*ws)/wb + Rds; 
		TF[1] = 0;
		TF[2] = (complex(0,1)*Xm*ws)/(Xr*wb);
		TF[3] = (complex(0,1)*Xm*ws)/(Xr*wb);
		TF[4] = 0;
		TF[5] = (complex(0,1)*Xc*wb)/ws + (complex(0,1)*Xq_prime*ws)/wb + Rqs;
		TF[6] = -(Xm*n*ws)/(Xr*wb);
		TF[7] = (Xm*n*ws)/(Xr*wb);
		TF[8] = Xm/2;
		TF[9] =  -(complex(0,1)*Xm*n)/2;
		TF[10] = (complex(0,1)*wr - complex(0,1)*ws)*To_prime -psi_sat;
		TF[11] = 0;
		TF[12] = Xm/2;
		TF[13] = (complex(0,1)*Xm*n)/2;
		TF[14] = 0;
		TF[15] = (complex(0,1)*wr + complex(0,1)*ws)*-To_prime -psi_sat ;

		// big matrix solving winding currents and fluxes
		invertMatrix(TF,ITF);
		Ids = ITF[0]*Vs.Mag() + ITF[1]*Vs.Mag();
		Iqs = ITF[4]*Vs.Mag() + ITF[5]*Vs.Mag();
		psi_f = ITF[8]*Vs.Mag() + ITF[9]*Vs.Mag();
		psi_b = ITF[12]*Vs.Mag() + ITF[13]*Vs.Mag();
		If = (Ids-(complex(0,1)*n*Iqs))*0.5;
		Ib = (Ids+(complex(0,1)*n*Iqs))*0.5;

        //electrical torque 
		Telec = (Xm/Xr)*2*(If.Im()*psi_f.Re() - If.Re()*psi_f.Im() - Ib.Im()*psi_b.Re() + Ib.Re()*psi_b.Im()); 

        //calculate speed deviation 
        wr_delta = Telec-Tmech;

        //Calculate saturated flux
        psi = sqrt(pow(psi_f.Re(),2)+pow(psi_f.Im(),2)+pow(psi_b.Re(),2)+pow(psi_b.Im(),2));
		if(psi<=bsat) {
            psi_sat = 1;
		}
		else {
            psi_sat = 1 + Asat*(pow(psi-bsat,2));
		}

        //update the rotor speed
		if (wr+wr_delta > 0) {
			wr = wr+wr_delta;
		}
		else {
			wr = 0;
		}
	}
    
    psi_dr = psi_f + psi_b;
    psi_qr = complex(0,1)*psi_f + complex(0,-1)*psi_b;
    // system current and power equations
    Is = (Ids + Iqs)*complex_exp(Vs.Arg());
    Ss = Vs*~Is;
}


//************** Begin_Yuan's TPIM model ********************//
void motor::TPIMSteadyState(TIMESTAMP t1) {
		double omgr0_delta = 1;
		double count = 1;
		complex alpha = complex(0,0);
		complex A1, A2, A3, A4;
		complex B1, B2, B3, B4;
		complex C1, C2, C3, C4;
		complex D1, D2, D3, D4;
		complex E1, E2, E3, E4;
		complex Vap;
		complex Van;
		// complex Vaz;

		alpha = complex_exp(2.0/3.0 * PI);

        Vap = (Vas + alpha * Vbs + alpha * alpha * Vcs) / 3.0;
        Van = (Vas + alpha * alpha * Vbs + alpha * Vcs) / 3.0;
        // Vaz = 1.0/3.0 * (Vas + Vbs + Vcs);

        // printf("Enter steady state: \n");

        if (TPIM_initial_status == initialRUNNING){

			while (fabs(omgr0_delta) > speed_error && count < interation_count) {
				count++;

				// pre-calculate complex coefficients of the 4 linear equations associated with flux state variables
				A1 = -(complex(0,1) * (wref + wsyn) + rs / sigma1) ;
				B1 =  0.0 ;
				C1 =  rs / sigma1 * lm / Lr ;
				D1 =  0.0 ;
				E1 = Vap ;

				A2 = 0.0;
				B2 = -(complex(0,1) * (wref - wsyn) + rs / sigma1) ;
				C2 = 0.0 ;
				D2 = rs / sigma1 * lm / Lr ;
				E2 = ~Van ;  // Does ~ represent conjugate of complex ?

				A3 = rr / sigma2 * lm / Ls ;
				B3 =  0.0 ;
				C3 =  -(complex(0,1) * (wref + wsyn - omgr0) + rr / sigma2) ;
				D3 =  0.0 ;
				E3 =  0.0 ;

				A4 = 0.0 ;
				B4 =  rr / sigma2 * lm / Ls ;
				C4 = 0.0 ;
				D4 = -(complex(0,1) * (wref - wsyn - omgr0) + rr / sigma2) ;
				E4 =  0.0 ;

				// solve the 4 linear equations to obtain 4 flux state variables
				phips = (B1*C2*D3*E4 - B1*C2*D4*E3 - B1*C3*D2*E4 + B1*C3*D4*E2 + B1*C4*D2*E3
					- B1*C4*D3*E2 - B2*C1*D3*E4 + B2*C1*D4*E3 + B2*C3*D1*E4 - B2*C3*D4*E1
					- B2*C4*D1*E3 + B2*C4*D3*E1 + B3*C1*D2*E4 - B3*C1*D4*E2 - B3*C2*D1*E4
					+ B3*C2*D4*E1 + B3*C4*D1*E2 - B3*C4*D2*E1 - B4*C1*D2*E3 + B4*C1*D3*E2
					+ B4*C2*D1*E3 - B4*C2*D3*E1 - B4*C3*D1*E2 + B4*C3*D2*E1)/
					(A1*B2*C3*D4 - A1*B2*C4*D3 - A1*B3*C2*D4 + A1*B3*C4*D2 + A1*B4*C2*D3 -
					A1*B4*C3*D2 - A2*B1*C3*D4 + A2*B1*C4*D3 + A2*B3*C1*D4 - A2*B3*C4*D1 -
					A2*B4*C1*D3 + A2*B4*C3*D1 + A3*B1*C2*D4 - A3*B1*C4*D2 - A3*B2*C1*D4 +
					A3*B2*C4*D1 + A3*B4*C1*D2 - A3*B4*C2*D1 - A4*B1*C2*D3 + A4*B1*C3*D2 +
					A4*B2*C1*D3 - A4*B2*C3*D1 - A4*B3*C1*D2 + A4*B3*C2*D1) ;

				phins_cj =  -(A1*C2*D3*E4 - A1*C2*D4*E3 - A1*C3*D2*E4 + A1*C3*D4*E2 + A1*C4*D2*E3
					- A1*C4*D3*E2 - A2*C1*D3*E4 + A2*C1*D4*E3 + A2*C3*D1*E4 - A2*C3*D4*E1
					- A2*C4*D1*E3 + A2*C4*D3*E1 + A3*C1*D2*E4 - A3*C1*D4*E2 - A3*C2*D1*E4
					+ A3*C2*D4*E1 + A3*C4*D1*E2 - A3*C4*D2*E1 - A4*C1*D2*E3 + A4*C1*D3*E2
					+ A4*C2*D1*E3 - A4*C2*D3*E1 - A4*C3*D1*E2 + A4*C3*D2*E1)/
					(A1*B2*C3*D4 - A1*B2*C4*D3 - A1*B3*C2*D4 + A1*B3*C4*D2 + A1*B4*C2*D3 -
					A1*B4*C3*D2 - A2*B1*C3*D4 + A2*B1*C4*D3 + A2*B3*C1*D4 - A2*B3*C4*D1 -
					A2*B4*C1*D3 + A2*B4*C3*D1 + A3*B1*C2*D4 - A3*B1*C4*D2 - A3*B2*C1*D4 +
					A3*B2*C4*D1 + A3*B4*C1*D2 - A3*B4*C2*D1 - A4*B1*C2*D3 + A4*B1*C3*D2 +
					A4*B2*C1*D3 - A4*B2*C3*D1 - A4*B3*C1*D2 + A4*B3*C2*D1) ;

				phipr = (A1*B2*D3*E4 - A1*B2*D4*E3 - A1*B3*D2*E4 + A1*B3*D4*E2 + A1*B4*D2*E3
					- A1*B4*D3*E2 - A2*B1*D3*E4 + A2*B1*D4*E3 + A2*B3*D1*E4 - A2*B3*D4*E1
					- A2*B4*D1*E3 + A2*B4*D3*E1 + A3*B1*D2*E4 - A3*B1*D4*E2 - A3*B2*D1*E4
					+ A3*B2*D4*E1 + A3*B4*D1*E2 - A3*B4*D2*E1 - A4*B1*D2*E3 + A4*B1*D3*E2
					+ A4*B2*D1*E3 - A4*B2*D3*E1 - A4*B3*D1*E2 + A4*B3*D2*E1)/
					(A1*B2*C3*D4 - A1*B2*C4*D3 - A1*B3*C2*D4 + A1*B3*C4*D2 + A1*B4*C2*D3 -
					A1*B4*C3*D2 - A2*B1*C3*D4 + A2*B1*C4*D3 + A2*B3*C1*D4 - A2*B3*C4*D1 -
					A2*B4*C1*D3 + A2*B4*C3*D1 + A3*B1*C2*D4 - A3*B1*C4*D2 - A3*B2*C1*D4 +
					A3*B2*C4*D1 + A3*B4*C1*D2 - A3*B4*C2*D1 - A4*B1*C2*D3 + A4*B1*C3*D2 +
					A4*B2*C1*D3 - A4*B2*C3*D1 - A4*B3*C1*D2 + A4*B3*C2*D1) ;

				phinr_cj = -(A1*B2*C3*E4 - A1*B2*C4*E3 - A1*B3*C2*E4 + A1*B3*C4*E2 + A1*B4*C2*E3
					- A1*B4*C3*E2 - A2*B1*C3*E4 + A2*B1*C4*E3 + A2*B3*C1*E4 - A2*B3*C4*E1
					- A2*B4*C1*E3 + A2*B4*C3*E1 + A3*B1*C2*E4 - A3*B1*C4*E2 - A3*B2*C1*E4
					+ A3*B2*C4*E1 + A3*B4*C1*E2 - A3*B4*C2*E1 - A4*B1*C2*E3 + A4*B1*C3*E2
					+ A4*B2*C1*E3 - A4*B2*C3*E1 - A4*B3*C1*E2 + A4*B3*C2*E1)/
					(A1*B2*C3*D4 - A1*B2*C4*D3 - A1*B3*C2*D4 + A1*B3*C4*D2 + A1*B4*C2*D3 -
					A1*B4*C3*D2 - A2*B1*C3*D4 + A2*B1*C4*D3 + A2*B3*C1*D4 - A2*B3*C4*D1 -
					A2*B4*C1*D3 + A2*B4*C3*D1 + A3*B1*C2*D4 - A3*B1*C4*D2 - A3*B2*C1*D4 +
					A3*B2*C4*D1 + A3*B4*C1*D2 - A3*B4*C2*D1 - A4*B1*C2*D3 + A4*B1*C3*D2 +
					A4*B2*C1*D3 - A4*B2*C3*D1 - A4*B3*C1*D2 + A4*B3*C2*D1) ;

				Ips = (phips - phipr * lm / Lr) / sigma1;  // pu
				Ipr = (phipr - phips * lm / Ls) / sigma2;  // pu
				Ins_cj = (phins_cj - phinr_cj * lm / Lr) / sigma1; // pu
				Inr_cj = (phinr_cj - phins_cj * lm / Ls) / sigma2; // pu
				Telec = (~phips * Ips + ~phins_cj * Ins_cj).Im() ;  // pu

				// iteratively compute speed increment to make sure Telec matches TL during steady state mode
				// if it does not match, then update current and Telec using new omgr0
				omgr0_delta = ( Telec - TL ) / interation_count;

				// printf("omgr0 = %f, omgr0_delta = %f, Telec = %f, TL = %f \n", omgr0, omgr0_delta, Telec, TL);

				//update the rotor speed to make sure electrical torque traces mechanical torque
				if (omgr0 + omgr0_delta > 0) {
					omgr0 = omgr0 + omgr0_delta;
				}
				else {
					omgr0 = 0;
				}

			}  // End while

        } // End if TPIM_initial_status == initialRUNNING

        else // must be TPIM_initial_status == initialSTATIONARY
        {
        	omgr0 = 0.0;
        	TL = 0.0;

			// pre-calculate complex coefficients of the 4 linear equations associated with flux state variables
			A1 = -(complex(0,1) * (wref + wsyn) + rs / sigma1) ;
			B1 =  0.0 ;
			C1 =  rs / sigma1 * lm / Lr ;
			D1 =  0.0 ;
			E1 = Vap ;

			A2 = 0.0;
			B2 = -(complex(0,1) * (wref - wsyn) + rs / sigma1) ;
			C2 = 0.0 ;
			D2 = rs / sigma1 * lm / Lr ;
			E2 = ~Van ;  // Does ~ represent conjugate of complex ?

			A3 = rr / sigma2 * lm / Ls ;
			B3 =  0.0 ;
			C3 =  -(complex(0,1) * (wref + wsyn - omgr0) + rr / sigma2) ;
			D3 =  0.0 ;
			E3 =  0.0 ;

			A4 = 0.0 ;
			B4 =  rr / sigma2 * lm / Ls ;
			C4 = 0.0 ;
			D4 = -(complex(0,1) * (wref - wsyn - omgr0) + rr / sigma2) ;
			E4 =  0.0 ;

			// solve the 4 linear equations to obtain 4 flux state variables
			phips = (B1*C2*D3*E4 - B1*C2*D4*E3 - B1*C3*D2*E4 + B1*C3*D4*E2 + B1*C4*D2*E3
				- B1*C4*D3*E2 - B2*C1*D3*E4 + B2*C1*D4*E3 + B2*C3*D1*E4 - B2*C3*D4*E1
				- B2*C4*D1*E3 + B2*C4*D3*E1 + B3*C1*D2*E4 - B3*C1*D4*E2 - B3*C2*D1*E4
				+ B3*C2*D4*E1 + B3*C4*D1*E2 - B3*C4*D2*E1 - B4*C1*D2*E3 + B4*C1*D3*E2
				+ B4*C2*D1*E3 - B4*C2*D3*E1 - B4*C3*D1*E2 + B4*C3*D2*E1)/
				(A1*B2*C3*D4 - A1*B2*C4*D3 - A1*B3*C2*D4 + A1*B3*C4*D2 + A1*B4*C2*D3 -
				A1*B4*C3*D2 - A2*B1*C3*D4 + A2*B1*C4*D3 + A2*B3*C1*D4 - A2*B3*C4*D1 -
				A2*B4*C1*D3 + A2*B4*C3*D1 + A3*B1*C2*D4 - A3*B1*C4*D2 - A3*B2*C1*D4 +
				A3*B2*C4*D1 + A3*B4*C1*D2 - A3*B4*C2*D1 - A4*B1*C2*D3 + A4*B1*C3*D2 +
				A4*B2*C1*D3 - A4*B2*C3*D1 - A4*B3*C1*D2 + A4*B3*C2*D1) ;

			phins_cj =  -(A1*C2*D3*E4 - A1*C2*D4*E3 - A1*C3*D2*E4 + A1*C3*D4*E2 + A1*C4*D2*E3
				- A1*C4*D3*E2 - A2*C1*D3*E4 + A2*C1*D4*E3 + A2*C3*D1*E4 - A2*C3*D4*E1
				- A2*C4*D1*E3 + A2*C4*D3*E1 + A3*C1*D2*E4 - A3*C1*D4*E2 - A3*C2*D1*E4
				+ A3*C2*D4*E1 + A3*C4*D1*E2 - A3*C4*D2*E1 - A4*C1*D2*E3 + A4*C1*D3*E2
				+ A4*C2*D1*E3 - A4*C2*D3*E1 - A4*C3*D1*E2 + A4*C3*D2*E1)/
				(A1*B2*C3*D4 - A1*B2*C4*D3 - A1*B3*C2*D4 + A1*B3*C4*D2 + A1*B4*C2*D3 -
				A1*B4*C3*D2 - A2*B1*C3*D4 + A2*B1*C4*D3 + A2*B3*C1*D4 - A2*B3*C4*D1 -
				A2*B4*C1*D3 + A2*B4*C3*D1 + A3*B1*C2*D4 - A3*B1*C4*D2 - A3*B2*C1*D4 +
				A3*B2*C4*D1 + A3*B4*C1*D2 - A3*B4*C2*D1 - A4*B1*C2*D3 + A4*B1*C3*D2 +
				A4*B2*C1*D3 - A4*B2*C3*D1 - A4*B3*C1*D2 + A4*B3*C2*D1) ;

			phipr = (A1*B2*D3*E4 - A1*B2*D4*E3 - A1*B3*D2*E4 + A1*B3*D4*E2 + A1*B4*D2*E3
				- A1*B4*D3*E2 - A2*B1*D3*E4 + A2*B1*D4*E3 + A2*B3*D1*E4 - A2*B3*D4*E1
				- A2*B4*D1*E3 + A2*B4*D3*E1 + A3*B1*D2*E4 - A3*B1*D4*E2 - A3*B2*D1*E4
				+ A3*B2*D4*E1 + A3*B4*D1*E2 - A3*B4*D2*E1 - A4*B1*D2*E3 + A4*B1*D3*E2
				+ A4*B2*D1*E3 - A4*B2*D3*E1 - A4*B3*D1*E2 + A4*B3*D2*E1)/
				(A1*B2*C3*D4 - A1*B2*C4*D3 - A1*B3*C2*D4 + A1*B3*C4*D2 + A1*B4*C2*D3 -
				A1*B4*C3*D2 - A2*B1*C3*D4 + A2*B1*C4*D3 + A2*B3*C1*D4 - A2*B3*C4*D1 -
				A2*B4*C1*D3 + A2*B4*C3*D1 + A3*B1*C2*D4 - A3*B1*C4*D2 - A3*B2*C1*D4 +
				A3*B2*C4*D1 + A3*B4*C1*D2 - A3*B4*C2*D1 - A4*B1*C2*D3 + A4*B1*C3*D2 +
				A4*B2*C1*D3 - A4*B2*C3*D1 - A4*B3*C1*D2 + A4*B3*C2*D1) ;

			phinr_cj = -(A1*B2*C3*E4 - A1*B2*C4*E3 - A1*B3*C2*E4 + A1*B3*C4*E2 + A1*B4*C2*E3
				- A1*B4*C3*E2 - A2*B1*C3*E4 + A2*B1*C4*E3 + A2*B3*C1*E4 - A2*B3*C4*E1
				- A2*B4*C1*E3 + A2*B4*C3*E1 + A3*B1*C2*E4 - A3*B1*C4*E2 - A3*B2*C1*E4
				+ A3*B2*C4*E1 + A3*B4*C1*E2 - A3*B4*C2*E1 - A4*B1*C2*E3 + A4*B1*C3*E2
				+ A4*B2*C1*E3 - A4*B2*C3*E1 - A4*B3*C1*E2 + A4*B3*C2*E1)/
				(A1*B2*C3*D4 - A1*B2*C4*D3 - A1*B3*C2*D4 + A1*B3*C4*D2 + A1*B4*C2*D3 -
				A1*B4*C3*D2 - A2*B1*C3*D4 + A2*B1*C4*D3 + A2*B3*C1*D4 - A2*B3*C4*D1 -
				A2*B4*C1*D3 + A2*B4*C3*D1 + A3*B1*C2*D4 - A3*B1*C4*D2 - A3*B2*C1*D4 +
				A3*B2*C4*D1 + A3*B4*C1*D2 - A3*B4*C2*D1 - A4*B1*C2*D3 + A4*B1*C3*D2 +
				A4*B2*C1*D3 - A4*B2*C3*D1 - A4*B3*C1*D2 + A4*B3*C2*D1) ;

			Ips = (phips - phipr * lm / Lr) / sigma1;  // pu
			Ipr = (phipr - phips * lm / Ls) / sigma2;  // pu
			Ins_cj = (phins_cj - phinr_cj * lm / Lr) / sigma1; // pu
			Inr_cj = (phinr_cj - phins_cj * lm / Ls) / sigma2; // pu
			Telec = (~phips * Ips + ~phins_cj * Ins_cj).Im() ;  // pu
        }

		// system current and power equations
        Ias = Ips + ~Ins_cj ;// pu
        Ibs = alpha * alpha * Ips + alpha * ~Ins_cj ; // pu
        Ics = alpha * Ips + alpha * alpha * ~Ins_cj ; // pu

		Smt = (Vap * ~Ips + Van * Ins_cj) * Pbase; // VA

}
//************** End_Yuan's TPIM model ********************//


// Function to calculate the solution to the steady state SPIM model
void motor::SPIMDynamic(double curr_delta_time, double dTime) {
	double psi = -1;
	double Xc = -1;
    
    //Kick in extra capacitor if we droop in speed
	if (wr < cap_run_speed) {
       Xc = Xc2;
	}
	else {
       Xc = Xc1; 
	}

    // Flux equation
	psi_b = (Ib*Xm) / ((complex(0,1)*(ws+wr)*To_prime)+psi_sat);
	psi_f = psi_f + ( If*(Xm/To_prime) - (complex(0,1)*(ws-wr) + psi_sat/To_prime)*psi_f )*dTime;   

    //Calculate saturated flux
	psi = sqrt(psi_f.Re()*psi_f.Re() + psi_f.Im()*psi_f.Im() + psi_b.Re()*psi_b.Re() + psi_b.Im()*psi_b.Im());
	if(psi<=bsat) {
        psi_sat = 1;
	}
	else {
        psi_sat = 1 + Asat*((psi-bsat)*(psi-bsat));
	}   

	// Calculate d and q axis fluxes
	psi_dr = psi_f + psi_b;
	psi_qr = complex(0,1)*psi_f + complex(0,-1)*psi_b;

	// d and q-axis current equations
	Ids = (-(complex(0,1)*(ws/wb)*(Xm/Xr)*psi_dr) + Vs.Mag()) / ((complex(0,1)*(ws/wb)*Xd_prime)+Rds);  
	Iqs = (-(complex(0,1)*(ws/wb)*(n*Xm/Xr)*psi_qr) + Vs.Mag()) / ((complex(0,1)*(ws/wb)*Xq_prime)+(complex(0,1)*(wb/ws)*Xc)+Rqs); 

	// f and b current equations
	If = (Ids-(complex(0,1)*n*Iqs))*0.5;
	Ib = (Ids+(complex(0,1)*n*Iqs))*0.5;

	// system current and power equations
	Is = (Ids + Iqs)*complex_exp(Vs.Arg());
	Ss = Vs*~Is;

    //electrical torque 
	Telec = (Xm/Xr)*2*(If.Im()*psi_f.Re() - If.Re()*psi_f.Im() - Ib.Im()*psi_b.Re() + Ib.Re()*psi_b.Im()); 

	// speed equation 
	wr = wr + (((Telec-Tmech)*wb)/(2*H))*dTime;

    // speeds below 0 should be avioded
	if (wr < 0) {
		wr = 0;
	}
}


//************** Begin_Yuan's TPIM model ********************//
void motor::TPIMDynamic(double curr_delta_time, double dTime) {
	complex alpha = complex(0,0);
	complex Vap;
	complex Van;

	// variables related to predictor step
	complex A1p, C1p;
	complex B2p, D2p;
	complex A3p, C3p;
	complex B4p, D4p;
	complex dphips_prev_dt ;
	complex dphins_cj_prev_dt ;
	complex dphipr_prev_dt ;
	complex dphinr_cj_prev_dt ;
	complex domgr0_prev_dt ;

	// variables related to corrector step
	complex A1c, C1c;
	complex B2c, D2c;
	complex A3c, C3c;
	complex B4c, D4c;
	complex dphips_dt ;
	complex dphins_cj_dt ;
	complex dphipr_dt ;
	complex dphinr_cj_dt ;
	complex domgr0_dt ;

	alpha = complex_exp(2.0/3.0 * PI);

    Vap = (Vas + alpha * Vbs + alpha * alpha * Vcs) / 3.0;
    Van = (Vas + alpha * alpha * Vbs + alpha * Vcs) / 3.0;

    TPIMupdateVars();

    // Apply mechanical torque
    if(omgr0 >= 1.0){
    	TL = TLrated;
    }

    //*** Predictor Step ***//
    // predictor step 1 - calculate coefficients
    A1p = -(complex(0,1) * (wref + wsyn) + rs / sigma1) ;
    C1p =  rs / sigma1 * lm / Lr ;

    B2p = -(complex(0,1) * (wref - wsyn) + rs / sigma1) ;
    D2p = rs / sigma1 *lm / Lr ;

    A3p = rr / sigma2 * lm / Ls ;
    C3p =  -(complex(0,1) * (wref + wsyn - omgr0_prev) + rr / sigma2) ;

    B4p =  rr / sigma2 * lm / Ls ;
    D4p = -(complex(0,1) * (wref - wsyn - omgr0_prev) + rr / sigma2) ;

    // predictor step 2 - calculate derivatives
    dphips_prev_dt =  ( Vap + A1p * phips_prev + C1p * phipr_prev ) * wb;  // pu/s
    dphins_cj_prev_dt = ( ~Van + B2p * phins_cj_prev + D2p * phinr_cj_prev ) * wb; // pu/s
    dphipr_prev_dt  =  ( C3p * phipr_prev + A3p * phips_prev ) * wb; // pu/s
    dphinr_cj_prev_dt = ( D4p * phinr_cj_prev  + B4p * phins_cj_prev ) * wb; // pu/s
    domgr0_prev_dt =  ( (~phips_prev * Ips_prev + ~phins_cj_prev * Ins_cj_prev).Im() - TL - Kfric * omgr0_prev ) / (2.0 * H); // pu/s


    // predictor step 3 - integrate for predicted state variable
    phips = phips_prev +  dphips_prev_dt * dTime;
    phins_cj = phins_cj_prev + dphins_cj_prev_dt * dTime;
    phipr = phipr_prev + dphipr_prev_dt * dTime ;
    phinr_cj = phinr_cj_prev + dphinr_cj_prev_dt * dTime ;
    omgr0 = omgr0_prev + domgr0_prev_dt.Re() * dTime ;

    // predictor step 4 - update outputs using predicted state variables
    Ips = (phips - phipr * lm / Lr) / sigma1;  // pu
    Ipr = (phipr - phips * lm / Ls) / sigma2;  // pu
    Ins_cj = (phins_cj - phinr_cj * lm / Lr) / sigma1 ; // pu
    Inr_cj = (phinr_cj - phins_cj * lm / Ls) / sigma2; // pu


    //*** Corrector Step ***//
    // assuming no boundary variable (e.g. voltage) changes during each time step,
    // so predictor and corrector steps are placed in the same class function

    // corrector step 1 - calculate coefficients using predicted state variables
    A1c = -(complex(0,1) * (wref + wsyn) + rs / sigma1) ;
    C1c =  rs / sigma1 * lm / Lr ;

    B2c = -(complex(0,1) * (wref - wsyn) + rs / sigma1) ;
    D2c = rs / sigma1 *lm / Lr ;

    A3c = rr / sigma2 * lm / Ls ;
    C3c =  -(complex(0,1) * (wref + wsyn - omgr0) + rr / sigma2) ;  // This coeff. is different from predictor

    B4c =  rr / sigma2 * lm / Ls ;
    D4c = -(complex(0,1) * (wref - wsyn - omgr0) + rr / sigma2) ; // This coeff. is different from predictor

    // corrector step 2 - calculate derivatives
    dphips_dt =  ( Vap + A1c * phips + C1c * phipr ) * wb;
    dphins_cj_dt = ( ~Van + B2c * phins_cj + D2c * phinr_cj ) * wb ;
    dphipr_dt  =  ( C3c * phipr + A3c * phips ) * wb;
    dphinr_cj_dt = ( D4c * phinr_cj  + B4c * phins_cj ) * wb;
    domgr0_dt =  1.0/(2.0 * H) * ( (~phips * Ips + ~phins_cj * Ins_cj).Im() - TL - Kfric * omgr0 );

    // corrector step 3 - integrate
    phips = phips_prev +  (dphips_prev_dt + dphips_dt) * dTime/2.0;
    phins_cj = phins_cj_prev + (dphins_cj_prev_dt + dphins_cj_dt) * dTime/2.0;
    phipr = phipr_prev + (dphipr_prev_dt + dphipr_dt) * dTime/2.0 ;
    phinr_cj = phinr_cj_prev + (dphinr_cj_prev_dt + dphinr_cj_dt) * dTime/2.0 ;
    omgr0 = omgr0_prev + (domgr0_prev_dt + domgr0_dt).Re() * dTime/2.0 ;

	if (omgr0 < 0) { // speeds below 0 should be avoided
		omgr0 = 0;
	}

    // corrector step 4 - update outputs
    Ips = (phips - phipr * lm / Lr) / sigma1;  // pu
    Ipr = (phipr - phips * lm / Ls) / sigma2;  // pu
    Ins_cj = (phins_cj - phinr_cj * lm / Lr) / sigma1; // pu
    Inr_cj = (phinr_cj - phins_cj * lm / Ls) / sigma2; // pu
    Telec = (~phips * Ips + ~phins_cj * Ins_cj).Im() ;  // pu

	// update system current and power equations
    Ias = Ips + ~Ins_cj ;// pu
    Ibs = alpha * alpha * Ips + alpha * ~Ins_cj ; // pu
    Ics = alpha * Ips + alpha * alpha * ~Ins_cj ; // pu

	Smt = (Vap * ~Ips + Van * Ins_cj) * Pbase; // VA

}
//************** End_Yuan's TPIM model ********************//


//Function to perform exp(j*val) (basically a complex rotation)
complex motor::complex_exp(double angle)
{
	complex output_val;

	//exp(jx) = cos(x)+j*sin(x)
	output_val = complex(cos(angle),sin(angle));

	return output_val;
}

// Function to do the inverse of a 4x4 matrix
int motor::invertMatrix(complex TF[16], complex ITF[16])
{
    complex inv[16], det;
    int i;

    inv[0] = TF[5]  * TF[10] * TF[15] - TF[5]  * TF[11] * TF[14] - TF[9]  * TF[6]  * TF[15] + TF[9]  * TF[7]  * TF[14] +TF[13] * TF[6]  * TF[11] - TF[13] * TF[7]  * TF[10];
    inv[4] = -TF[4]  * TF[10] * TF[15] + TF[4]  * TF[11] * TF[14] + TF[8]  * TF[6]  * TF[15] - TF[8]  * TF[7]  * TF[14] - TF[12] * TF[6]  * TF[11] + TF[12] * TF[7]  * TF[10];
    inv[8] = TF[4]  * TF[9] * TF[15] - TF[4]  * TF[11] * TF[13] - TF[8]  * TF[5] * TF[15] + TF[8]  * TF[7] * TF[13] + TF[12] * TF[5] * TF[11] - TF[12] * TF[7] * TF[9];
    inv[12] = -TF[4]  * TF[9] * TF[14] + TF[4]  * TF[10] * TF[13] +TF[8]  * TF[5] * TF[14] - TF[8]  * TF[6] * TF[13] - TF[12] * TF[5] * TF[10] + TF[12] * TF[6] * TF[9];
    inv[1] = -TF[1]  * TF[10] * TF[15] + TF[1]  * TF[11] * TF[14] + TF[9]  * TF[2] * TF[15] - TF[9]  * TF[3] * TF[14] - TF[13] * TF[2] * TF[11] + TF[13] * TF[3] * TF[10];
    inv[5] = TF[0]  * TF[10] * TF[15] - TF[0]  * TF[11] * TF[14] - TF[8]  * TF[2] * TF[15] + TF[8]  * TF[3] * TF[14] + TF[12] * TF[2] * TF[11] - TF[12] * TF[3] * TF[10];
    inv[9] = -TF[0]  * TF[9] * TF[15] + TF[0]  * TF[11] * TF[13] + TF[8]  * TF[1] * TF[15] - TF[8]  * TF[3] * TF[13] - TF[12] * TF[1] * TF[11] + TF[12] * TF[3] * TF[9];
    inv[13] = TF[0]  * TF[9] * TF[14] - TF[0]  * TF[10] * TF[13] - TF[8]  * TF[1] * TF[14] + TF[8]  * TF[2] * TF[13] + TF[12] * TF[1] * TF[10] - TF[12] * TF[2] * TF[9];
    inv[2] = TF[1]  * TF[6] * TF[15] - TF[1]  * TF[7] * TF[14] - TF[5]  * TF[2] * TF[15] + TF[5]  * TF[3] * TF[14] + TF[13] * TF[2] * TF[7] - TF[13] * TF[3] * TF[6];
    inv[6] = -TF[0]  * TF[6] * TF[15] + TF[0]  * TF[7] * TF[14] + TF[4]  * TF[2] * TF[15] - TF[4]  * TF[3] * TF[14] - TF[12] * TF[2] * TF[7] + TF[12] * TF[3] * TF[6];
    inv[10] = TF[0]  * TF[5] * TF[15] - TF[0]  * TF[7] * TF[13] - TF[4]  * TF[1] * TF[15] + TF[4]  * TF[3] * TF[13] + TF[12] * TF[1] * TF[7] - TF[12] * TF[3] * TF[5];
    inv[14] = -TF[0]  * TF[5] * TF[14] + TF[0]  * TF[6] * TF[13] + TF[4]  * TF[1] * TF[14] - TF[4]  * TF[2] * TF[13] - TF[12] * TF[1] * TF[6] + TF[12] * TF[2] * TF[5];
    inv[3] = -TF[1] * TF[6] * TF[11] + TF[1] * TF[7] * TF[10] + TF[5] * TF[2] * TF[11] - TF[5] * TF[3] * TF[10] - TF[9] * TF[2] * TF[7] + TF[9] * TF[3] * TF[6];
    inv[7] = TF[0] * TF[6] * TF[11] - TF[0] * TF[7] * TF[10] - TF[4] * TF[2] * TF[11] + TF[4] * TF[3] * TF[10] + TF[8] * TF[2] * TF[7] - TF[8] * TF[3] * TF[6];
    inv[11] = -TF[0] * TF[5] * TF[11] + TF[0] * TF[7] * TF[9] + TF[4] * TF[1] * TF[11] - TF[4] * TF[3] * TF[9] - TF[8] * TF[1] * TF[7] + TF[8] * TF[3] * TF[5];
    inv[15] = TF[0] * TF[5] * TF[10] - TF[0] * TF[6] * TF[9] - TF[4] * TF[1] * TF[10] + TF[4] * TF[2] * TF[9] + TF[8] * TF[1] * TF[6] - TF[8] * TF[2] * TF[5];

    det = TF[0] * inv[0] + TF[1] * inv[4] + TF[2] * inv[8] + TF[3] * inv[12];

    if (det == 0)
        return 0;

    det = complex(1,0) / det;

    for (i = 0; i < 16; i++)
        ITF[i] = inv[i] * det;

    return 1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: motor
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_motor(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(motor::oclass);
		if (*obj!=NULL)
		{
			motor *my = OBJECTDATA(*obj,motor);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(motor);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_motor(OBJECT *obj)
{
	try {
		motor *my = OBJECTDATA(obj,motor);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(motor);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_motor(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	TIMESTAMP t1 = TS_INVALID;
	motor *my = OBJECTDATA(obj,motor);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t1 = my->presync(obj->clock,t0);
			break;
		case PC_BOTTOMUP:
			t1 = my->sync(obj->clock,t0);
			break;
		case PC_POSTTOPDOWN:
			t1 = my->postsync(obj->clock,t0);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass == clockpass)
			obj->clock = t1;
	}
	SYNC_CATCHALL(motor);
	return t1;
}

/**
* Allows the core to discover whether obj is a subtype of this class.
*
* @param obj a pointer to this object
* @param classname the name of the object the core is testing
*
* @return 0 if obj is a subtype of this class
*/
EXPORT int isa_motor(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,motor)->isa(classname);
	} else {
		return 0;
	}
}

/** 
* DELTA MODE
*/
EXPORT SIMULATIONMODE interupdate_motor(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	motor *my = OBJECTDATA(obj,motor);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_motor(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

/**@}*/
