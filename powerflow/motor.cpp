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
			PT_enumeration,"motor_override",PADDR(motor_override),PT_DESCRIPTION,"override funtion to dictate if motor is turned off or on",
				PT_KEYWORD,"ON",(enumeration)overrideON,
				PT_KEYWORD,"OFF",(enumeration)overrideOFF,

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

			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"delta_linkage_node", (FUNCTIONADDR)delta_linkage)==NULL)
			GL_THROW("Unable to publish motor delta_linkage function");
		if (gl_publish_function(oclass,	"delta_freq_pwr_object", (FUNCTIONADDR)delta_frequency_node)==NULL)
			GL_THROW("Unable to publish motor deltamode function");
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_motor)==NULL)
			GL_THROW("Unable to publish motor deltamode function");
    }
}

int motor::create()
{
	int result = node::create();
	last_cycle = 0;

	// Default parameters
	motor_override = overrideON;
	motor_trip = 0;
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
	interation_count = 1000;
	Tmech = 1.0448;
	cap_run_speed_percentage = 50;
	DM_volt_trig_per = 80; 
	DM_speed_trig_per = 80;
	DM_volt_exit_per = 95; 
	DM_speed_exit_per = 95;
	speed_error = 1e-10;

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

	return result;
}

int motor::init(OBJECT *parent)
{
	int result = node::init(parent);

	Ibase = Pbase/nominal_voltage; 
	wb=2*PI*nominal_frequency;
	cap_run_speed = (cap_run_speed_percentage*wb)/100;
	DM_volt_trig = (DM_volt_trig_per)/100;
	DM_speed_trig = (DM_speed_trig_per*wb)/100;
	DM_volt_exit = (DM_volt_exit_per)/100;
	DM_speed_exit = (DM_speed_exit_per*wb)/100;

	// Check what phases are connected on this motor
	double num_phases = 0;
	num_phases = num_phases+has_phase(PHASE_A);
	num_phases = num_phases+has_phase(PHASE_B);
	num_phases = num_phases+has_phase(PHASE_C);
	
	// error out if we have more than one phase
	if (num_phases > 1) {
		GL_THROW("More than one phase is specified for a single phase motor");	
	}

	// determine the specific phase this motor is connected to
	// ----------- TO DO!!!!! ------------------
	// --- Implement support for split phase 
	// --- Support 3-phase motors            

	if (has_phase(PHASE_A)) {
		connected_phase = 0;	
	}
	if (has_phase(PHASE_B)) {
		connected_phase = 1;	
	}
	if (has_phase(PHASE_C)) {
		connected_phase = 2;	
	}

	OBJECT *obj = OBJECTHDR(this);
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
	if (t1 == 946713696) {
		double a;
		a = 1;
	}
	
	// update voltage and frequency
	updateFreqVolt();

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
		current[connected_phase] = Is*Ibase;
	}
	else { // motor is currently disconnected
		current[connected_phase] = 0;
		SPIMStateOFF();
	}

	// update motor status
	SPIMUpdateMotorStatus();

	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::sync(t1);

	if ((double)t1 > last_cycle) {	
		last_cycle = (double)t1;
	}

	// figure out if we need to enter delta mode on the next pass 
	if ((Vs.Mag() < DM_volt_trig || wr < DM_speed_trig) && deltamode_inclusive)
	{
		schedule_deltamode_start(t1);
		return t1;
	}
	else 
	{
		return result;
	}
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

		// update voltage and frequency
		updateFreqVolt();

		// update previous values for the model
		SPIMupdateVars();

		SPIMSteadyState(curr_delta_time);

		// update motor status
		SPIMUpdateMotorStatus();

	}//End first pass and timestep of deltamode (initial condition stuff)

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Call presync-equivalent items
		if (delta_time>0) {
			NR_node_presync_fxn(0);

			// update voltage and frequency
			updateFreqVolt();
		}

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
			current[connected_phase] = Is*Ibase;
		}
		else { // motor is currently disconnected
			current[connected_phase] = 0;
			SPIMStateOFF();
		}

		// update motor status
		SPIMUpdateMotorStatus();

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

		// figure out if we need to exit delta mode on the next pass 
		if (Vs.Mag() > DM_volt_exit && wr > DM_speed_exit)
		{
			return SM_EVENT;
		}
			return SM_DELTA;
		}
}

// function to update motor frequency and voltage
void motor::updateFreqVolt() {
	// update voltage and frequency
	if ( (SubNode == CHILD) || (SubNode == DIFF_CHILD) ) // if we have a parent, reference the voltage and frequency of the parent
	{
		node *parNode = OBJECTDATA(SubNodeParent,node);
		Vs = parNode->voltage[connected_phase]/parNode->nominal_voltage;
		ws = parNode->curr_freq_state.fmeas[connected_phase]*2*PI;
	}
	else // No parent, use our own voltage
	{
		Vs = voltage[connected_phase]/nominal_voltage;
		ws = curr_freq_state.fmeas[connected_phase]*2*PI;
	}
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