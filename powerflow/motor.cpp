// $Id: motor.cpp 1182 2016-08-15 jhansen $
//	Copyright (C) 2008 Battelle Memorial Institute

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>	

#include <iostream>

#include "motor.h"

//////////////////////////////////////////////////////////////////////////
// capacitor CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* motor::oclass = nullptr;
CLASS* motor::pclass = nullptr;

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
	if(oclass == nullptr)
	{
		pclass = node::oclass;
		
		oclass = gl_register_class(mod,"motor",sizeof(motor),passconfig|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class motor";
		else
			oclass->trl = TRL_PRINCIPLE;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "node",
			PT_double, "base_power[W]", PADDR(Pbase),PT_DESCRIPTION,"base power",
			PT_double, "n", PADDR(n),PT_DESCRIPTION,"ratio of stator auxiliary windings to stator main windings",
			PT_double, "Rds[ohm]", PADDR(Rds),PT_DESCRIPTION,"d-axis resistance - single-phase model",
			PT_double, "Rqs[ohm]", PADDR(Rqs),PT_DESCRIPTION,"q-asis resistance - single-phase model",
			PT_double, "Rs[ohm]", PADDR(Rs),PT_DESCRIPTION,"stator resistance - three-phase model",
			PT_double, "Rr[ohm]", PADDR(Rr),PT_DESCRIPTION,"rotor resistance",
			PT_double, "Xm[ohm]", PADDR(Xm),PT_DESCRIPTION,"magnetizing reactance",
			PT_double, "Xr[ohm]", PADDR(Xr),PT_DESCRIPTION,"rotor reactance",
			PT_double, "Xs[ohm]", PADDR(Xs),PT_DESCRIPTION,"stator leakage reactance - three-phase model",
			PT_double, "Xc_run[ohm]", PADDR(Xc1),PT_DESCRIPTION,"running capacitor reactance - single-phase model",
			PT_double, "Xc_start[ohm]", PADDR(Xc2),PT_DESCRIPTION,"starting capacitor reactance - single-phase model",
			PT_double, "Xd_prime[ohm]", PADDR(Xd_prime),PT_DESCRIPTION,"d-axis reactance - single-phase model",
			PT_double, "Xq_prime[ohm]", PADDR(Xq_prime),PT_DESCRIPTION,"q-axis reactance - single-phase model",
			PT_double, "A_sat", PADDR(Asat),PT_DESCRIPTION,"flux saturation parameter, A - single-phase model",
			PT_double, "b_sat", PADDR(bsat),PT_DESCRIPTION,"flux saturation parameter, b - single-phase model",
			PT_double, "H[s]", PADDR(H),PT_DESCRIPTION,"inertia constant",
			PT_double, "J[kg*m^2]", PADDR(Jm),PT_DESCRIPTION,"moment of inertia",
			PT_double, "number_of_poles", PADDR(pf),PT_DESCRIPTION,"number of poles",
			PT_double, "To_prime[s]", PADDR(To_prime),PT_DESCRIPTION,"rotor time constant",
			PT_double, "capacitor_speed[%]", PADDR(cap_run_speed_percentage),PT_DESCRIPTION,"percentage speed of nominal when starting capacitor kicks in",
			PT_double, "trip_time[s]", PADDR(trip_time),PT_DESCRIPTION,"time motor can stay stalled before tripping off ",
            PT_enumeration,"uv_relay_install",PADDR(uv_relay_install),PT_DESCRIPTION,"is under-voltage relay protection installed on this motor",
				PT_KEYWORD,"INSTALLED",(enumeration)uv_relay_INSTALLED,
				PT_KEYWORD,"UNINSTALLED",(enumeration)uv_relay_UNINSTALLED,
            PT_double, "uv_relay_trip_time[s]", PADDR(uv_relay_trip_time),PT_DESCRIPTION,"time low-voltage condition must exist for under-voltage protection to trip ",
            PT_double, "uv_relay_trip_V[pu]", PADDR(uv_relay_trip_V),PT_DESCRIPTION,"pu minimum voltage before under-voltage relay trips",
            PT_enumeration,"contactor_state",PADDR(contactor_state),PT_DESCRIPTION,"the current status of the motor",
				PT_KEYWORD,"OPEN",(enumeration)contactorOPEN,
				PT_KEYWORD,"CLOSED",(enumeration)contactorCLOSED,
            PT_double, "contactor_open_Vmin[pu]", PADDR(contactor_open_Vmin),PT_DESCRIPTION,"pu voltage at which motor contactor opens",
            PT_double, "contactor_close_Vmax[pu]", PADDR(contactor_close_Vmax),PT_DESCRIPTION,"pu voltage at which motor contactor recloses",
			PT_double, "reconnect_time[s]", PADDR(reconnect_time),PT_DESCRIPTION,"time before tripped motor reconnects",

			//Reconcile torque and speed, primarily
			PT_double, "mechanical_torque[pu]", PADDR(Tmech),PT_DESCRIPTION,"mechanical torque applied to the motor",
			PT_double, "mechanical_torque_state_var[pu]", PADDR(Tmech_eff),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Internal state variable torque - three-phase model",

			PT_enumeration, "torque_usage_method", PADDR(motor_torque_usage_method), PT_DESCRIPTION, "Approach for using Tmech on both types",
				PT_KEYWORD, "DIRECT", (enumeration)modelDirectTorque,
				PT_KEYWORD, "SPEEDFOUR", (enumeration)modelSpeedFour,	//Fixed at 0.85 + 0.15*speed^4

			PT_int32, "iteration_count", PADDR(iteration_count),PT_DESCRIPTION,"maximum number of iterations for steady state model",
			PT_double, "delta_mode_voltage_trigger[%]", PADDR(DM_volt_trig_per),PT_DESCRIPTION,"percentage voltage of nominal when delta mode is triggered",
			PT_double, "delta_mode_rotor_speed_trigger[%]", PADDR(DM_speed_trig_per),PT_DESCRIPTION,"percentage speed of nominal when delta mode is triggered",
			PT_double, "delta_mode_voltage_exit[%]", PADDR(DM_volt_exit_per),PT_DESCRIPTION,"percentage voltage of nominal to exit delta mode",
			PT_double, "delta_mode_rotor_speed_exit[%]", PADDR(DM_speed_exit_per),PT_DESCRIPTION,"percentage speed of nominal to exit delta mode",
			PT_double, "maximum_speed_error", PADDR(speed_error),PT_DESCRIPTION,"maximum speed error for transitioning modes",
			PT_double, "rotor_speed[rad/s]", PADDR(wr),PT_DESCRIPTION,"rotor speed",
			PT_enumeration,"motor_status",PADDR(motor_status),PT_DESCRIPTION,"the current status of the motor",
				PT_KEYWORD,"RUNNING",(enumeration)statusRUNNING,
				PT_KEYWORD,"STALLED",(enumeration)statusSTALLED,
				PT_KEYWORD,"TRIPPED",(enumeration)statusTRIPPED,
				PT_KEYWORD,"OFF",(enumeration)statusOFF,
			PT_int32,"motor_status_number",PADDR(motor_status),PT_DESCRIPTION,"the current status of the motor as an integer",
			PT_enumeration,"desired_motor_state",PADDR(motor_override),PT_DESCRIPTION,"Should the motor be on or off",
				PT_KEYWORD,"ON",(enumeration)overrideON,
				PT_KEYWORD,"OFF",(enumeration)overrideOFF,

			PT_object,"connected_house",PADDR(mtr_house_pointer),PT_DESCRIPTION,"house object to monitor the XXX property to determine if the motor is running",

			PT_enumeration,"connected_house_assumed_mode", PADDR(connected_house_assumed_mode), PT_DESCRIPTION, "Assumed operation mode of connected_house object",
				PT_KEYWORD,"NONE",(enumeration)house_mode_NONE,
				PT_KEYWORD,"COOLING",(enumeration)house_mode_COOLING,
				PT_KEYWORD,"HEATING",(enumeration)house_mode_HEATING,

			PT_enumeration,"motor_operation_type",PADDR(motor_op_mode),PT_DESCRIPTION,"current operation type of the motor - deltamode related",
				PT_KEYWORD,"SINGLE-PHASE",(enumeration)modeSPIM,
				PT_KEYWORD,"THREE-PHASE",(enumeration)modeTPIM,
			PT_enumeration,"triplex_connection_type",PADDR(triplex_connection_type),PT_DESCRIPTION,"Describes how the motor will connect to the triplex devices",
				PT_KEYWORD,"TRIPLEX_1N",(enumeration)TPNconnected1N,
				PT_KEYWORD,"TRIPLEX_2N",(enumeration)TPNconnected2N,
				PT_KEYWORD,"TRIPLEX_12",(enumeration)TPNconnected12,

			// These model parameters are published as hidden
			PT_double, "wb[rad/s]", PADDR(wbase),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"base speed",
			PT_double, "ws[rad/s]", PADDR(ws),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"system speed",
			PT_complex, "psi_b", PADDR(psi_b),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"backward rotating flux",
			PT_complex, "psi_f", PADDR(psi_f),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"forward rotating flux",
			PT_complex, "psi_dr", PADDR(psi_dr),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Rotor d axis flux",
			PT_complex, "psi_qr", PADDR(psi_qr),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"Rotor q axis flux",
			PT_complex, "Ids", PADDR(Ids),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"time before tripped motor reconnects",
			PT_complex, "Iqs", PADDR(Iqs),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"time before tripped motor reconnects",
			PT_complex, "If", PADDR(If),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"forward current",
			PT_complex, "Ib", PADDR(Ib),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"backward current",
			PT_complex, "Is", PADDR(Is),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor current",
			PT_complex, "electrical_power[VA]", PADDR(motor_elec_power),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor power",
			PT_double, "electrical_torque[pu]", PADDR(Telec),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"electrical torque",
			PT_complex, "Vs", PADDR(Vs),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor voltage",
			PT_bool, "motor_trip", PADDR(motor_trip),PT_DESCRIPTION,"boolean variable to check if motor is tripped",
			PT_double, "trip", PADDR(trip),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"current time in tripped state",
			PT_double, "reconnect", PADDR(reconnect),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"current time since motor was tripped",


			// These model parameters are published as hidden
			PT_complex, "phips", PADDR(phips),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"positive sequence stator flux",
			PT_complex, "phins_cj", PADDR(phins_cj),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"conjugate of negative sequence stator flux",
			PT_complex, "phipr", PADDR(phipr),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"positive sequence rotor flux",
			PT_complex, "phinr_cj", PADDR(phinr_cj),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"conjugate of negative sequence rotor flux",
			PT_double, "per_unit_rotor_speed[pu]", PADDR(wr_pu),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"rotor speed in per-unit",
			PT_complex, "Ias[pu]", PADDR(Ias),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor phase-a stator current",
			PT_complex, "Ibs[pu]", PADDR(Ibs),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor phase-b stator current",
			PT_complex, "Ics[pu]", PADDR(Ics),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor phase-c stator current",
			PT_complex, "Vas[pu]", PADDR(Vas),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor phase-a stator-to-ground voltage",
			PT_complex, "Vbs[pu]", PADDR(Vbs),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor phase-b stator-to-ground voltage",
			PT_complex, "Vcs[pu]", PADDR(Vcs),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"motor phase-c stator-to-ground voltage",
			PT_complex, "Ips", PADDR(Ips),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"positive sequence stator current",
			PT_complex, "Ipr", PADDR(Ipr),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"positive sequence rotor current",
			PT_complex, "Ins_cj", PADDR(Ins_cj),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"conjugate of negative sequence stator current",
			PT_complex, "Inr_cj", PADDR(Inr_cj),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"conjugate of negative sequence rotor current",
			PT_double, "Ls", PADDR(Ls),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"stator synchronous reactance",
			PT_double, "Lr", PADDR(Lr),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"rotor synchronous reactance",
			PT_double, "sigma1", PADDR(sigma1),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"intermediate variable 1 associated with synch. react.",
			PT_double, "sigma2", PADDR(sigma2),PT_ACCESS,PA_HIDDEN,PT_DESCRIPTION,"intermediate variable 2 associated with synch. react.",

			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_motor)==nullptr)
			GL_THROW("Unable to publish motor deltamode function");
		if (gl_publish_function(oclass,	"pwr_object_swing_swapper", (FUNCTIONADDR)swap_node_swing_status)==nullptr)
			GL_THROW("Unable to publish motor swing-swapping function");
		if (gl_publish_function(oclass,	"attach_vfd_to_pwr_object", (FUNCTIONADDR)attach_vfd_to_node)==nullptr)
			GL_THROW("Unable to publish motor VFD attachment function");
		if (gl_publish_function(oclass, "pwr_object_swing_status_check", (FUNCTIONADDR)node_swing_status) == nullptr)
			GL_THROW("Unable to publish motor swing-status check function");
		if (gl_publish_function(oclass, "pwr_object_shunt_update", (FUNCTIONADDR)node_update_shunt_values) == nullptr)
			GL_THROW("Unable to publish motor shunt update function");
    }
}

int motor::create()
{
	int result = node::create();
	last_cycle = 0;

	//Flag variable
	Tmech = -1.0;
	Tmech_eff = 0.0;

	// Default parameters
	motor_override = overrideON;  // share the variable with TPIM
	motor_trip = 0;  // share the variable with TPIM
	Pbase = -999;
	n = 1.22;              
	Rds =0.0365;           
	Rqs = 0.0729;          
	Rr =-999;
	Xm=-999;
	Xr=-999;
	Xc1 = -2.779;           
	Xc2 = -0.7;            
	Xd_prime = 0.1033;      
	Xq_prime =0.1489;       
	bsat = 0.7212;  
	Asat = 5.6;
	H=-999;
	Jm=-999;
	To_prime =0.1212;   
	trip_time = 10;        
	reconnect_time = 300;
	iteration_count = 1000;  // share the variable with TPIM
	cap_run_speed_percentage = 50;
	DM_volt_trig_per = 80; // share the variable with TPIM
	DM_speed_trig_per = 80;  // share the variable with TPIM
	DM_volt_exit_per = 95; // share the variable with TPIM
	DM_speed_exit_per = 95; // share the variable with TPIM
	speed_error = 1e-10; // share the variable with TPIM

	wbase=2.0*PI*nominal_frequency;

	// initial parameter for internal model
	trip = 0;
	reconnect = 0;
	psi_b = gld::complex(0.0,0.0);
    psi_f = gld::complex(0.0,0.0);
    psi_dr = gld::complex(0.0,0.0); 
    psi_qr = gld::complex(0.0,0.0); 
    Ids = gld::complex(0.0,0.0);
    Iqs = gld::complex(0.0,0.0);  
    If = gld::complex(0.0,0.0);
    Ib = gld::complex(0.0,0.0);
    Is = gld::complex(0.0,0.0);
    motor_elec_power = gld::complex(0.0,0.0);
    Telec = 0; 
    wr = 0;
    
    // Randomized contactor transition values
    //  Used to determine at what voltages the contactor
    //  will open and closed and votlage drops too low.
    uv_relay_install = uv_relay_UNINSTALLED; //Relay not enabled by default.
    uv_relay_time = 0; //counter for how long we've been in under-voltage condition
    uv_relay_trip_time = 0.02; //20ms default
    uv_relay_trip_V = 0.0;
    uv_lockout = 0;
    
    contactor_open_Vmin = 0.0;
    contactor_close_Vmax = 0.01;
    contactor_state = contactorCLOSED;

    //Mode initialization
    motor_op_mode = modeSPIM; // share the variable with TPIM

	//House connection capability
	mtr_house_pointer = nullptr;
	mtr_house_state_pointer = nullptr;

	//Set to none initially - if this isn't set, this will just get ignored
	connected_house_assumed_mode = house_mode_NONE;

    //Three-phase induction motor parameters, 500 HP, 2.3kV
    pf = 4;
    Rs= 0.262;
    Xs= 1.206;
    rs_pu = -999;  // pu
    lls = -999;  //  pu
    lm = -999;  // pu
    rr_pu = -999;  // pu
    llr = -999;  // pu

    // Parameters are for 3000 W motor
    Kfric = 0.0;  // pu
	phips = gld::complex(0.0,0.0); // pu
	phins_cj = gld::complex(0.0,0.0); // pu
	phipr = gld::complex(0.0,0.0); // pu
	phinr_cj = gld::complex(0.0,0.0); // pu
	wr_pu = 0.97; // pu
	Ias = gld::complex(0.0,0.0); // pu
	Ibs = gld::complex(0.0,0.0); // pu
	Ics = gld::complex(0.0,0.0); // pu
	Vas = gld::complex(0.0,0.0); // pu
	Vbs = gld::complex(0.0,0.0); // pu
	Vcs = gld::complex(0.0,0.0); // pu
	Ips = gld::complex(0.0,0.0); // pu
	Ipr = gld::complex(0.0,0.0); // pu
	Ins_cj = gld::complex(0.0,0.0); // pu
	Inr_cj = gld::complex(0.0,0.0); // pu
	Ls = 0.0; // pu
	Lr = 0.0; // pu
	sigma1 = 0.0; // pu
	sigma2 = 0.0; // pu
	ws_pu = 1.0; // pu

	triplex_connected = false;	//By default, not connected to triplex
	triplex_connection_type = TPNconnected12;	//If it does end up being triplex, we default it to L1-L2

	//Torque model element - by default, just pull in value from property
	motor_torque_usage_method = modelDirectTorque;

	return result;
}

int motor::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	int result;
	bool temp_house_motor_state;
	double temp_house_capacity_info, temp_house_cop;
	enumeration temp_house_type;
	gld_property *temp_gld_property;
	gld_wlock *test_rlock = nullptr;

	//See if we have a house connection defined -- if so, do this after that initializes (to get data)
	if (mtr_house_pointer != nullptr)
	{
		//Check it's status
		if ((mtr_house_pointer->flags & OF_INIT) != OF_INIT)
		{
			char objname[256];
			gl_verbose("motor:%d - %s ::init(): deferring initialization on %s", obj->id,(obj->name ? obj->name : "Unnamed"),gl_name(mtr_house_pointer, objname, 255));
			return 2; // defer
		}
	}

	//Now run node init, as necessary
	result = node::init(parent);

	// Check what phases are connected on this motor
	int num_phases = 0;
	if (has_phase(PHASE_A))
		num_phases++;

	if (has_phase(PHASE_B))
		num_phases++;

	if (has_phase(PHASE_C))
		num_phases++;
	
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
		GL_THROW("motor:%s -- only single-phase or three-phase motors are supported",(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The motor only supports single-phase and three-phase motors at this time.  Please use one of these connection types.
		*/
	}

	// determine the specific phase this motor is connected to
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
			else if (has_phase(PHASE_B)) {
				connected_phase = 1;
			}
			else {	//Phase C, by default
				connected_phase = 2;
			}
		}
	}
	//Default else -- three-phase diagnostics (none needed right now)

	//Map the connected house
	if (mtr_house_pointer != nullptr)
	{
		//Make sure it is a house first, just for giggles
		if (!gl_object_isa(mtr_house_pointer,"house","residential"))
		{
			GL_THROW("motor:%s -- connected_house must point toward a residential:house object",(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The motor connected_house field only supports connections to a residential:house object at this time.
			*/
		}

		//Make sure our mode is SPIM and triplexy - if not, failure
		if (!triplex_connected)
		{
			GL_THROW("motor:%s -- When using the house-connected mode, the motor must be a triplex device",(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			In the house-tied mode, the motor must be a triplex-connected motor.  Ideally, it should be connected to the same triplex device
			as the house.  Three-phase implementations may be supported at a future date.
			*/
		}

		//See if a mode is assumed - if not, throw an error, "just because"
		if (connected_house_assumed_mode == house_mode_NONE)
		{
			GL_THROW("motor:%s -- When using the house-connected mode, an expected type of operation must be specified",(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			If using the motor in a "house_connected" mode, an assumption on if it is running as a heating or cooling motor must be assumed.  This 
			is used to extract the motor size from the house.  If left to "NONE", this error will persist.
			*/
		}

		//Implies it is set, determine which one we are and pull the appropriate properties
		if (connected_house_assumed_mode == house_mode_COOLING)
		{
			//Make sure it is a compatible type!
			temp_gld_property = new gld_property(mtr_house_pointer,"cooling_system_type");

			//Make sure it worked
			if (!temp_gld_property->is_valid() || !temp_gld_property->is_enumeration())
			{
				GL_THROW("motor:%s -- Unable to map house property",(obj->name ? obj->name : "Unnamed"));
				//Defined below
			}

			//Pull the value
			temp_gld_property->getp<enumeration>(temp_house_type,*test_rlock);

			//Delete the connection
			delete temp_gld_property;

			//Check to see if it is valid
			if ((temp_house_type != 2) && (temp_house_type != 3))	//Not a normal AC or heat pump
			{
				GL_THROW("motor:%s - Mapped house does not support the type of heating/cooling mode expected!",(obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the motor to a house object, the house does not have the appropriate heating or cooling system
				to connect the motor.  If assuming COOLING, the house cooling_system_type must be ELECTRIC or HEAT_PUMP.  If assuming
				HEATING, the house heating_system_type must be HEAT_PUMP.  Select the appropriate mode and try again.
				*/
			}

			//Map to design cooling capacity
			temp_gld_property = new gld_property(mtr_house_pointer,"design_cooling_capacity");

			//Make sure it worked
			if (!temp_gld_property->is_valid() || !temp_gld_property->is_double())
			{
				GL_THROW("motor:%s -- Unable to map house property",(obj->name ? obj->name : "Unnamed"));
				//Defined below
			}

			//Pull the value
			temp_house_capacity_info = temp_gld_property->get_double();

			//Clear the mapping
			delete temp_gld_property;

			//Map to the cooling COP
			temp_gld_property = new gld_property(mtr_house_pointer,"cooling_COP");

			//Make sure it worked
			if (!temp_gld_property->is_valid() || !temp_gld_property->is_double())
			{
				GL_THROW("motor:%s -- Unable to map house property",(obj->name ? obj->name : "Unnamed"));
				//Defined below
			}

			//Pull the value
			temp_house_cop = temp_gld_property->get_double();

			//Clear the property
			delete temp_gld_property;
		}
		else if (connected_house_assumed_mode == house_mode_HEATING)
		{
			//Make sure it is a compatible type!
			temp_gld_property = new gld_property(mtr_house_pointer,"heating_system_type");

			//Make sure it worked
			if (!temp_gld_property->is_valid() || !temp_gld_property->is_enumeration())
			{
				GL_THROW("motor:%s -- Unable to map house property",(obj->name ? obj->name : "Unnamed"));
				//Defined below
			}

			//Pull the value
			temp_gld_property->getp<enumeration>(temp_house_type,*test_rlock);

			//Delete the connection
			delete temp_gld_property;

			//Check to see if it is valid
			if (temp_house_type != 3)	//Must be a heat pump
			{
				GL_THROW("motor:%s - Mapped house does not support the type of heating/cooling mode expected!",(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Map to design heating capacity
			temp_gld_property = new gld_property(mtr_house_pointer,"design_heating_capacity");

			//Make sure it worked
			if (!temp_gld_property->is_valid() || !temp_gld_property->is_double())
			{
				GL_THROW("motor:%s -- Unable to map house property",(obj->name ? obj->name : "Unnamed"));
				//Defined below
			}

			//Pull the value
			temp_house_capacity_info = temp_gld_property->get_double();

			//Clear the mapping
			delete temp_gld_property;

			//Map to the heating COP
			temp_gld_property = new gld_property(mtr_house_pointer,"heating_COP");

			//Make sure it worked
			if (!temp_gld_property->is_valid() || !temp_gld_property->is_double())
			{
				GL_THROW("motor:%s -- Unable to map house property",(obj->name ? obj->name : "Unnamed"));
				//Defined below
			}

			//Pull the value
			temp_house_cop = temp_gld_property->get_double();

			//Clear the property
			delete temp_gld_property;
		}
		//Default else - would be NONE, but we should never get here with that!

		//Perform the conversion to get a power amount for this motor (based on house)
		Pbase = temp_house_capacity_info / temp_house_cop / 3.4120;


		//Set the flag on the house
		temp_gld_property = new gld_property(mtr_house_pointer,"external_motor_attached");

		//Make sure it worked
		if (!temp_gld_property->is_valid() || !temp_gld_property->is_bool())
		{
			GL_THROW("motor:%s -- Unable to map house external motor property",(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to map the external_motor_attached property of the house to set motor state, an error occurred.  Please try again.
			If the error persists, please submit your code and a bug report via the issues tracker.
			*/
		}

		//Set the flag and push it
		temp_house_motor_state = true;
		temp_gld_property->setp<bool>(temp_house_motor_state,*test_rlock);

		//Now that it is done, kill it
		delete temp_gld_property;

		//Map up the property
		mtr_house_state_pointer = new gld_property(mtr_house_pointer,"compressor_on");

		//Make sure it worked
		if (!mtr_house_state_pointer->is_valid() || !mtr_house_state_pointer->is_bool())
		{
			GL_THROW("motor:%s -- Unable to map house property",(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to map a property of the house needed to operate the motor externally, an error occurred.  Please try again.
			If the error persists, please submit your code and a bug report via the issues tracker.
			*/
		}

		//Check the initial state - pull the value (not sure it is actually set yet)
		mtr_house_state_pointer->getp<bool>(temp_house_motor_state,*test_rlock);

		//Determine our state
		if (temp_house_motor_state)
		{
			motor_override = overrideON;
		}
		else
		{
			motor_override = overrideOFF;
		}
	}

	//Check the initial torque conditions
	if (Tmech == -1.0)
	{
		//See which one we are
		if (motor_op_mode == modeSPIM)
		{
			if (motor_torque_usage_method==modelDirectTorque)
			{
				Tmech = 1.0448;
			}
			else	//Assumes the speed^4 fraction
			{
				Tmech = 1.0;	//Assumes starts at nominal speed
			}
		}
		else	//Assume 3 phase
		{
			if (motor_torque_usage_method==modelDirectTorque)
			{
				Tmech = 0.95;
			}
			else	//Assumes the speed^4 fraction
			{
				Tmech = 1.0;	//Assumes starts at nominal speed
			}

			//Check mode
			if (motor_status != statusOFF)
			{
				Tmech_eff = Tmech;
			}
		}
	}
	//Default else - user specified it


	//Check default rated power
	if (Pbase == -999)
	{
		//See which one we are
		if (motor_op_mode == modeSPIM)
		{
			Pbase = 3500;
		}
		else	//Assume 3 phase
		{
			Pbase = 372876;
		}
	}
	//Default else - user specified it

	//Check default magnetizing inductance
	if (Xm == -999)
	{
		//See which one we are
		if (motor_op_mode == modeSPIM)
		{
			Xm = 2.28;
		}
		else	//Assume 3 phase
		{
			Xm = 56.02;
		}
	}
	//Default else - user specified it

	//Check default rotor inductance
	if (Xr == -999)
	{
		//See which one we are
		if (motor_op_mode == modeSPIM)
		{
			Xr = 2.33;
		}
		else	//Assume 3 phase
		{
			Xr = 1.206;
		}
	}
	//Default else - user specified it

    //Check default rotor resistance
	if (Rr == -999)
	{
		//See which one we are
		if (motor_op_mode == modeSPIM)
		{
			Rr = 0.0486;
		}
		else	//Assume 3 phase
		{
			Rr = 0.187;
		}
	}
	//Default else - user specified it

	// Moment of inertia and/or inertia constant
	if ((Jm == -999) && (H == -999)) // none specified
	{
		//See which one we are
		if (motor_op_mode == modeSPIM)
		{
			Jm = 0.0019701;
			H = 0.04;
		}
		else	//Assume 3 phase
		{
			Jm = 11.06;
			H = 0.52694;
		}
	}
	else if ((Jm != -999) && (H == -999)) // Jm but not H specified; calculate H
	{
		H = 1/2*pow(2/pf,2)*Jm*pow(wbase,2)/Pbase;
	}
	else if ((Jm == -999) && (H != -999)) // H but not Jm specified
	{
		Jm = H /(1/2*pow(2/pf,2)*pow(wbase,2)/Pbase); // no need to calculate Jm, only H is needed
	}
	else if ((Jm != -999) && (H != -999)) // both Jm and H specified; priority to H
	{
		gl_warning("motor:%s -- both H and J were specified, H will be used, J is ignored",(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		Both H and J were specified, H will be used, J is ignored.
		*/
	}

	// Per unit parameters
	Zbase = 3*pow(nominal_voltage,2)/Pbase;
	rs_pu = Rs/Zbase;  // pu
	lls = Xs/Zbase;  //  pu
	rr_pu = Rr/Zbase;  // pu
	llr = Xr/Zbase;  // pu
	lm = Xm/Zbase;  // pu

	//Parameters
	if (motor_op_mode == modeSPIM)
	{
		if (triplex_connected && (triplex_connection_type == TPNconnected12))
		{
			Ibase = Pbase/(2.0*nominal_voltage);	//To reflect LL connection
		}
		else	//"Three-phase" or "normal" triplex
		{
			Ibase = Pbase/nominal_voltage;
		}
	}
	else	//Three-phase
	{
		Ibase = Pbase/nominal_voltage/3.0;
	}


	cap_run_speed = (cap_run_speed_percentage*wbase)/100;
	DM_volt_trig = (DM_volt_trig_per)/100;
	DM_speed_trig = (DM_speed_trig_per*wbase)/100;
	DM_volt_exit = (DM_volt_exit_per)/100;
	DM_speed_exit = (DM_speed_exit_per*wbase)/100;

	//TPIM Items
	Ls = lls + lm; // pu
	Lr = llr + lm; // pu
	sigma1 = Ls - lm * lm / Lr; // pu
	sigma2 = Lr - lm * lm / Ls; // pu

	if ((motor_op_mode == modeTPIM) && (motor_status != statusRUNNING)){
		wr_pu = 0.0; // pu
		wr = 0.0;
	}
	else if ((motor_op_mode == modeTPIM) && (motor_status == statusRUNNING)){
		wr_pu = 1.0; // pu
		wr = wbase;
	}

	wr_pu_prev = wr_pu;
    
    // Checking contactor open and close min and max voltages
    if (contactor_open_Vmin > contactor_close_Vmax){
        GL_THROW("motor:%s -- contactor_open_Vmin must be less or equal to than contactor_close_Vmax",(obj->name ? obj->name : "Unnamed"));
    }

    
    // Checking under-voltage relay input parameters
    if (uv_relay_trip_time < 0 ){
        GL_THROW("motor:%s -- uv_relay_trip_time must be greater than or equal to 0",(obj->name ? obj->name : "Unnamed"));
    }
    if (uv_relay_trip_V < 0 || uv_relay_trip_V > 1){
        GL_THROW("motor:%s -- uv_relay_trip_V must be greater than or equal to 0 and less than or equal to 1",(obj->name ? obj->name : "Unnamed"));
    }

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
	bool temp_house_motor_state;
	gld_wlock *test_rlock = nullptr;

	// update voltage and frequency
	updateFreqVolt();

	if (motor_op_mode == modeSPIM)
	{
		//See if we're in "house-check mode"
		if (mtr_house_state_pointer != nullptr)
		{
			//Pull the updated state
			mtr_house_state_pointer->getp<bool>(temp_house_motor_state,*test_rlock);

			//Set the motor state
			if (temp_house_motor_state)
			{
				motor_override = overrideON;
			}
			else
			{
				motor_override = overrideOFF;
			}
		}//End crude house coupling check

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
			if (triplex_connected)
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
			if (triplex_connected)
			{
				//See which type of triplex
				if (triplex_connection_type == TPNconnected1N)
				{
					pre_rotated_current[0] = gld::complex(0.0,0.0);
				}
				else if (triplex_connection_type == TPNconnected2N)
				{
					pre_rotated_current[1] = gld::complex(0.0,0.0);
				}
				else	//Assume it is 12 now
				{
					pre_rotated_current[2] = gld::complex(0.0,0.0);
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
	else	//Must be three-phase
	{
		//Three-phase steady-state code
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

		if (motor_override == overrideON && ws_pu > 0.1 &&  Vas.Mag() > 0.1 && Vbs.Mag() > 0.1 && Vcs.Mag() > 0.1 &&
			motor_trip == 0) { // motor is currently connected and grid conditions are not "collapsed"
			// This needs to be re-visited to determine the lower bounds of ws_pu and Vas, Vbs and Vcs

			// run the steady state solver
			TPIMSteadyState(t1);

			// update current draw -- might need to be pre_rotated_current
			pre_rotated_current[0] = Ias*Ibase; // A
			pre_rotated_current[1] = Ibs*Ibase; // A
			pre_rotated_current[2] = Ics*Ibase; // A
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

	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::sync(t1);

	if ((double)t1 > last_cycle) {	
		last_cycle = (double)t1;
	}

	// figure out if we need to enter delta mode on the next pass
	if (motor_op_mode == modeSPIM)
	{
		if (((Vs.Mag() < DM_volt_trig) || (wr < DM_speed_trig)) && deltamode_inclusive)
		{
			// we should not enter delta mode if the motor is tripped or not close to reconnect
			if ((motor_trip == 1 && reconnect < reconnect_time-1)  || (motor_override == overrideOFF)) {
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
	else	//Must be three-phase
	{
		//This may need to be updated for three-phase
		if ( ((Vas.Mag() < DM_volt_trig) || (Vbs.Mag() < DM_volt_trig) ||
				(Vcs.Mag() < DM_volt_trig) || (wr < DM_speed_trig))
				&& deltamode_inclusive)
		{
			// we should not enter delta mode if the motor is tripped or not close to reconnect
			if ((motor_trip == 1 && reconnect < reconnect_time-1) || (motor_override == overrideOFF)) {
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
	bool temp_house_motor_state;
	gld_wlock *test_rlock = nullptr;
	double deltat;

	// make sure to capture the current time
	curr_delta_time = gl_globaldeltaclock;

	// I need the time delta in seconds
	deltat = (double)dt/(double)DT_SECOND;

	//Update time tracking variable - mostly for GFA functionality calls
	if ((iteration_count_val==0) && !interupdate_pos) //Only update timestamp tracker on first iteration
	{
		//Update tracking variable
		prev_time_dbl = gl_globaldeltaclock;

		//Update frequency calculation values (if needed)
		if (fmeas_type != FM_NONE)
		{
			//See which pass
			if (delta_time == 0)
			{
				//Initialize dynamics - first run of new delta call
				init_freq_dynamics(deltat);
			}
			else
			{
				//Copy the tracker value
				memcpy(&prev_freq_state,&curr_freq_state,sizeof(FREQM_STATES));
			}
		}
	}

	//In the first call we need to initilize the dynamic model
	if ((delta_time==0) && (iteration_count_val==0) && !interupdate_pos)	//First run of new delta call
	{
		//Call presync-equivalent items
		NR_node_presync_fxn(0);

		if (motor_op_mode == modeSPIM)
		{
			//See if we're in "house-check mode"
			if (mtr_house_state_pointer != nullptr)
			{
				//Pull the updated state
				mtr_house_state_pointer->getp<bool>(temp_house_motor_state,*test_rlock);

				//Set the motor state
				if (temp_house_motor_state)
				{
					motor_override = overrideON;
				}
				else
				{
					motor_override = overrideOFF;
				}
			}//End crude house coupling check

			// update voltage and frequency
			updateFreqVolt();

			// update previous values for the model
			SPIMupdateVars();

			SPIMSteadyState(curr_delta_time);

			// update motor status
			SPIMUpdateMotorStatus();
		}
		else	//Three-phase
		{
			//first run/deltamode initialization stuff
			// update voltage and frequency
			updateFreqVolt();

			// update previous values for the model
			TPIMupdateVars();

			TPIMSteadyState(curr_delta_time);

			// update motor status
			TPIMUpdateMotorStatus();
		}
	}//End first pass and timestep of deltamode (initial condition stuff)

	if (!interupdate_pos)	//Before powerflow call
	{
		//Call presync-equivalent items
		if (delta_time>0) {
			NR_node_presync_fxn(0);

			// update voltage and frequency
			updateFreqVolt();
		}

		if (motor_op_mode == modeSPIM)
		{
			//See if we're in "house-check mode"
			if (mtr_house_state_pointer != nullptr)
			{
				//Pull the updated state
				mtr_house_state_pointer->getp<bool>(temp_house_motor_state,*test_rlock);

				//Set the motor state
				if (temp_house_motor_state)
				{
					motor_override = overrideON;
				}
				else
				{
					motor_override = overrideOFF;
				}
			}//End crude house coupling check

			// if deltaTime is not small enough we will run into problems
			if (deltat > 0.0003) {
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
				SPIMUpdateProtection(deltat);
			}

			if (motor_override == overrideON && ws > 1 && Vs.Mag() > 0.1 && motor_trip == 0) { // motor is currently connected and grid conditions are not "collapsed"
				// run the dynamic solver
				SPIMDynamic(curr_delta_time, deltat);

				// update current draw
				if (triplex_connected)
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
				if (triplex_connected)
				{
					//See which type of triplex
					if (triplex_connection_type == TPNconnected1N)
					{
						pre_rotated_current[0] = gld::complex(0.0,0.0);
					}
					else if (triplex_connection_type == TPNconnected2N)
					{
						pre_rotated_current[1] = gld::complex(0.0,0.0);
					}
					else	//Assume it is 12 now
					{
						pre_rotated_current[2] = gld::complex(0.0,0.0);
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
		else 	//Must be three-phase
		{
			// if deltat is not small enough we will run into problems
			if (deltat > 0.0005) {
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
				TPIMUpdateProtection(deltat);
			}


			if (motor_override == overrideON && ws_pu > 0.1 && Vas.Mag() > 0.1 && Vbs.Mag() > 0.1 && Vcs.Mag() > 0.1 &&
					motor_trip == 0) { // motor is currently connected and grid conditions are not "collapsed"
				// run the dynamic solver
				TPIMDynamic(curr_delta_time, deltat);

				// update current draw -- pre_rotated_current
				pre_rotated_current[0] = Ias*Ibase; // A
				pre_rotated_current[1] = Ibs*Ibase; // A
				pre_rotated_current[2] = Ics*Ibase; // A
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
			return_status_val = calc_freq_dynamics(deltat);

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
			if ((Vs.Mag() > DM_volt_exit) && (wr > DM_speed_exit))
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
		else	//Must be three-phase
		{
			// figure out if we need to exit delta mode on the next pass
			if ((Vas.Mag() > DM_volt_exit) && (Vbs.Mag() > DM_volt_exit) && (Vcs.Mag() > DM_volt_exit)
					&& (wr > DM_speed_exit) && ((fabs(wr_pu-wr_pu_prev)*wbase) <= speed_error))
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
	}
}

// function to update motor frequency and voltage
void motor::updateFreqVolt() {
	// update voltage and frequency
	if (motor_op_mode == modeSPIM)
	{
		if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) != 0) // if we have a parent, reference the voltage and frequency of the parent
		{
			node *parNode = OBJECTDATA(SubNodeParent,node);
			if (triplex_connected)
			{
				//See which type of triplex
				if (triplex_connection_type == TPNconnected1N)
				{
					Vs = parNode->voltage[0]/parNode->nominal_voltage;
					ws = parNode->curr_freq_state.fmeas[0]*2.0*PI;
				}
				else if (triplex_connection_type == TPNconnected2N)
				{
					Vs = parNode->voltage[1]/parNode->nominal_voltage;
					ws = parNode->curr_freq_state.fmeas[1]*2.0*PI;
				}
				else	//Assume it is 12 now
				{
					Vs = parNode->voltaged[0]/(2.0*parNode->nominal_voltage);	//To reflect LL connection
					ws = parNode->curr_freq_state.fmeas[0]*2.0*PI;
				}
			}
			else	//"Three-phase" connection
			{
				Vs = parNode->voltage[connected_phase]/parNode->nominal_voltage;
				ws = parNode->curr_freq_state.fmeas[connected_phase]*2.0*PI;
			}
		}
		else // No parent, use our own voltage
		{
			if (triplex_connected)
			{
				//See which type of triplex
				if (triplex_connection_type == TPNconnected1N)
				{
					Vs = voltage[0]/nominal_voltage;
					ws = curr_freq_state.fmeas[0]*2.0*PI;
				}
				else if (triplex_connection_type == TPNconnected2N)
				{
					Vs = voltage[1]/nominal_voltage;
					ws = curr_freq_state.fmeas[1]*2.0*PI;
				}
				else	//Assume it is 12 now
				{
					Vs = voltaged[0]/(2.0*nominal_voltage);	//To reflect LL connection
					ws = curr_freq_state.fmeas[0]*2.0*PI;
				}
			}
			else 	//"Three-phase" connection
			{
				Vs = voltage[connected_phase]/nominal_voltage;
				ws = curr_freq_state.fmeas[connected_phase]*2.0*PI;
			}
		}

		//Make the per-unit value
		ws_pu = ws/wbase;
	}
	else //TPIM model
	{
		if ((SubNode & (SNT_CHILD | SNT_DIFF_CHILD)) != 0) // if we have a parent, reference the voltage and frequency of the parent
		{
			node *parNode = OBJECTDATA(SubNodeParent,node);
			// obtain 3-phase voltages
			Vas = parNode->voltage[0]/parNode->nominal_voltage;
			Vbs = parNode->voltage[1]/parNode->nominal_voltage;
			Vcs = parNode->voltage[2]/parNode->nominal_voltage;
			// obtain frequency in pu, take the average of 3-ph frequency ?
			ws_pu = (parNode->curr_freq_state.fmeas[0]+ parNode->curr_freq_state.fmeas[1] + parNode->curr_freq_state.fmeas[2]) / nominal_frequency / 3.0;
		}
		else // No parent, use our own voltage
		{
			Vas = voltage[0]/nominal_voltage;
			Vbs = voltage[1]/nominal_voltage;
			Vcs = voltage[2]/nominal_voltage;
			ws_pu = (curr_freq_state.fmeas[0] + curr_freq_state.fmeas[1] + curr_freq_state.fmeas[2]) / nominal_frequency / 3.0;
		}

		//Make the non-per-unit value
		ws = ws_pu * wbase;
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
	motor_elec_power_prev = motor_elec_power;
	reconnect_prev = reconnect;
	motor_trip_prev = motor_trip;
	trip_prev = trip;
	psi_sat_prev = psi_sat;
}


//TPIM state variable updates - transition them
void motor::TPIMupdateVars() {
	phips_prev = phips;
	phins_cj_prev = phins_cj;
	phipr_prev = phipr;
	phinr_cj_prev = phinr_cj;
	wr_pu_prev = wr_pu;
	Ips_prev = Ips;
	Ipr_prev = Ipr;
	Ins_cj_prev = Ins_cj;
	Inr_cj_prev = Inr_cj;

	reconnect_prev = reconnect;
	motor_trip_prev = motor_trip;
	trip_prev = trip;

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
	motor_elec_power = motor_elec_power_prev;
	reconnect = reconnect_prev;
	motor_trip = motor_trip_prev;
	trip = trip_prev;
	psi_sat = psi_sat_prev;
}

//TPIM initalization routine
void motor::TPIMreinitializeVars() {
	phips = phips_prev;
	phins_cj = phins_cj_prev;
	phipr = phipr_prev;
	phinr_cj = phinr_cj_prev;
	wr_pu = wr_pu_prev;
	Ips = Ips_prev;
	Ipr = Ipr_prev;
	Ins_cj = Ins_cj_prev;
	Inr_cj = Inr_cj_prev;

	reconnect = reconnect_prev;
	motor_trip = motor_trip_prev;
	trip = trip_prev;

}

// function to update the status of the motor
void motor::SPIMUpdateMotorStatus() {
	if (motor_override == overrideOFF || ws <= 1 || Vs.Mag() <= 0.1)
	{
		motor_status = statusOFF;
	}
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

//TPIM status update
void motor::TPIMUpdateMotorStatus() {
	if (motor_override == overrideOFF || ws_pu <= 0.1 ||
			Vas.Mag() <= 0.1 || Vbs.Mag() <= 0.1 || Vcs.Mag() <= 0.1) {
		motor_status = statusOFF;
	}
	else if (wr_pu > 0.0) {
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
        
        // Under-voltage protection relay timer
        if (uv_lockout == 0) {
            if (Vs.Mag() <= uv_relay_trip_V && Vs.Mag() > 0.01 && uv_relay_install == uv_relay_INSTALLED) { //  This particular AC motor does have an under-voltage relay that will trip. Assumes voltages less than 0.01 pu indicates a disconnected state and shouldn't be considered under-voltage.
                if (uv_relay_time <= uv_relay_trip_time) { //In under-voltage state and accumulating time
                    uv_relay_time = uv_relay_time + delta_time;
                    uv_lockout = 0;
                }
                else { //relay time has accumlated and trips the unit open, permanently
                    motor_override = overrideOFF;
                    uv_lockout = 1;
                }
            } else { //Either the voltage is high enough or this motor doesn't have under-voltage protection
                uv_relay_time = 0;
            }
        }
        
        // Main contactor opening due to low-voltage condition (coil doesn't have enough magnetic force to hold contacts together)
        
        if (Vs.Mag() <=  contactor_open_Vmin && uv_lockout == 0 && contactor_state == contactorCLOSED) { //
            //std::cout << "Under voltage, contactor open: " << Vs.Mag() << "\n";
            motor_override = overrideOFF;
            contactor_state = contactorOPEN;
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
    // Main contactor closing (magnetic force able to hold contacts together) once voltage gets high enough
    if (Vs.Mag() >=  contactor_close_Vmax && uv_lockout == 0 && contactor_state == contactorOPEN ){//
        //std::cout << "Voltage OK, contactor closed: " << Vs.Mag() << "\n";
        motor_override = overrideON;
        contactor_state = contactorCLOSED;
    }
}

// TPIM thermal protection
void motor::TPIMUpdateProtection(double delta_time) {
	if (motor_override == overrideON) {
		if (wr_pu < 0.01 && motor_trip == 0 && ws_pu > 0.1 && Vas.Mag() > 0.1 && Vbs.Mag() > 0.1 && Vcs.Mag() > 0.1) { // conditions for counting up the time trip timer
			trip = trip + delta_time; // count up by the time since last pass
		}
		else if ( (wr_pu >= 0.01 && motor_trip == 0) || ws_pu <= 0.1 || Vas.Mag() <= 0.1 || Vbs.Mag() <= 0.1 || Vcs.Mag() <= 0.1) { // conditions for counting down the time trip timer
			trip = trip - (delta_time / (reconnect_time/trip_time)) < 0 ? 0 : trip - (delta_time / (reconnect_time/trip_time)); // count down by the time since last cycle scaled by the difference in trip and reconnect times
		}
        
        // Under-voltage protection relay timer
        if (uv_lockout == 0) {
            if (((Vas.Mag() <= uv_relay_trip_V && Vas.Mag() > 0.001) || (Vbs.Mag() <= uv_relay_trip_V && Vbs.Mag() > 0.001) || (Vcs.Mag() <= uv_relay_trip_V) && Vcs.Mag() > 0.001) && uv_relay_install == uv_relay_INSTALLED) { //  This particular AC motor does have an under-voltage relay that will trip
                if (uv_relay_time <= uv_relay_trip_time) { //In under-voltage state and accumulating time
                    //std::cout << "Under voltage: " << Vas.Mag() << "\t" << Vbs.Mag() << "\t" << Vcs.Mag()<< "\n";
                    uv_relay_time = uv_relay_time + delta_time;
                    uv_lockout = 0;
                }
                else { //relay time has accumlated and trips the unit open, permanently
                    //std::cout << "Under voltage protection relay trip.\n";
                    motor_override = overrideOFF;
                    uv_lockout = 1;
                }
            } else { //Either the voltage is high enough or this motor doesn't have under-voltage protection
                //std::cout << "Voltage OK: " << Vas.Mag() << "\t" << Vbs.Mag() << "\t" << Vcs.Mag()<< "\n";
                uv_relay_time = 0;
            }
        }
        
        // Main contactor opening due to low-voltage condition (coil doesn't have enough magnetic force to hold contacts together)
        if ((Vas.Mag() <=  contactor_open_Vmin || Vbs.Mag() <=  contactor_open_Vmin || Vcs.Mag() <=  contactor_open_Vmin) && uv_lockout == 0  && contactor_state == contactorCLOSED) { //
            //std::cout << "Under voltage, contactor open: " << Vas.Mag() << "\t" << Vbs.Mag() << "\t" << Vcs.Mag()<< "\n";
            motor_override = overrideOFF;
            contactor_state = contactorOPEN;
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
    
    // Main contactor closing (magnetic force able to hold contacts together) once voltage gets high enough
    if ((Vas.Mag() >=  contactor_close_Vmax && Vbs.Mag() >=  contactor_close_Vmax && Vcs.Mag() >=  contactor_close_Vmax ) && uv_lockout == 0 && contactor_state == contactorOPEN ){//
        //std::cout << "Voltage OK, contactor closed: " << Vas.Mag() << "\t" << Vbs.Mag() << "\t" << Vcs.Mag()<< "\n";
        motor_override = overrideON;
        contactor_state = contactorCLOSED;
    }
}

// function to ensure that internal model states are zeros when the motor is OFF
void motor::SPIMStateOFF() {
	psi_b = gld::complex(0.0,0.0);
    psi_f = gld::complex(0.0,0.0);
    psi_dr = gld::complex(0.0,0.0); 
    psi_qr = gld::complex(0.0,0.0); 
    Ids = gld::complex(0.0,0.0);
    Iqs = gld::complex(0.0,0.0);  
    If = gld::complex(0.0,0.0);
    Ib = gld::complex(0.0,0.0);
    Is = gld::complex(0.0,0.0);
    motor_elec_power = gld::complex(0.0,0.0);
    Telec = 0.0; 
    wr = 0.0;
	wr_pu = 0.0;
}

//TPIM "zero-stating" item
void motor::TPIMStateOFF() {
	phips = gld::complex(0.0,0.0);
	phins_cj = gld::complex(0.0,0.0);
	phipr = gld::complex(0.0,0.0);
	phinr_cj = gld::complex(0.0,0.0);
	wr = 0.0;
	wr_pu = 0.0;
	Ips = gld::complex(0.0,0.0);
	Ipr = gld::complex(0.0,0.0);
	Ins_cj = gld::complex(0.0,0.0);
	Inr_cj = gld::complex(0.0,0.0);
	motor_elec_power = gld::complex(0.0,0.0);
	Telec = 0.0;
	Tmech_eff = 0.0;
}

// Function to calculate the solution to the steady state SPIM model
void motor::SPIMSteadyState(TIMESTAMP t1) {
	double wr_delta = 1;
    psi_sat = 1;
	double psi = -1;
    int32 count = 1;
	double Xc = -1;

	while (fabs(wr_delta) > speed_error && count < iteration_count) {
        count++;
        
        //Kick in extra capacitor if we droop in speed
		if (wr < cap_run_speed) {
           Xc = Xc2;
		}
		else {
           Xc = Xc1; 
		}

		TF[0] = (gld::complex(0.0,1.0)*Xd_prime*ws_pu) + Rds; 
		TF[1] = 0;
		TF[2] = (gld::complex(0.0,1.0)*Xm*ws_pu)/Xr;
		TF[3] = (gld::complex(0.0,1.0)*Xm*ws_pu)/Xr;
		TF[4] = 0;
		TF[5] = (gld::complex(0.0,1.0)*Xc)/ws_pu + (gld::complex(0.0,1.0)*Xq_prime*ws_pu) + Rqs;
		TF[6] = -(Xm*n*ws_pu)/Xr;
		TF[7] = (Xm*n*ws_pu)/Xr;
		TF[8] = Xm/2;
		TF[9] =  -(gld::complex(0.0,1.0)*Xm*n)/2;
		TF[10] = (gld::complex(0.0,1.0)*wr - gld::complex(0.0,1.0)*ws)*To_prime -psi_sat;
		TF[11] = 0;
		TF[12] = Xm/2;
		TF[13] = (gld::complex(0.0,1.0)*Xm*n)/2;
		TF[14] = 0;
		TF[15] = (gld::complex(0.0,1.0)*wr + gld::complex(0.0,1.0)*ws)*-To_prime -psi_sat ;

		// big matrix solving winding currents and fluxes
		invertMatrix(TF,ITF);
		Ids = ITF[0]*Vs.Mag() + ITF[1]*Vs.Mag();
		Iqs = ITF[4]*Vs.Mag() + ITF[5]*Vs.Mag();
		psi_f = ITF[8]*Vs.Mag() + ITF[9]*Vs.Mag();
		psi_b = ITF[12]*Vs.Mag() + ITF[13]*Vs.Mag();
		If = (Ids-(gld::complex(0.0,1.0)*n*Iqs))*0.5;
		Ib = (Ids+(gld::complex(0.0,1.0)*n*Iqs))*0.5;

        //electrical torque 
		Telec = (Xm/Xr)*2.0*(If.Im()*psi_f.Re() - If.Re()*psi_f.Im() - Ib.Im()*psi_b.Re() + Ib.Re()*psi_b.Im()); 

		//See which model we're using
		if (motor_torque_usage_method==modelSpeedFour)
		{
			//Compute updated mechanical torque
			Tmech = 0.85 + 0.15*(wr_pu*wr_pu*wr_pu*wr_pu);
		}
		//Default else - direct method, so just read (in case player/else changes it)

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
			wr_pu = wr/wbase;
		}
		else {
			wr = 0;
			wr_pu = 0.0;
		}
	}
    
    psi_dr = psi_f + psi_b;
    psi_qr = gld::complex(0.0,1.0)*psi_f + gld::complex(0,-1)*psi_b;
    // system current and power equations
    Is = (Ids + Iqs)*complex_exp(Vs.Arg());
    motor_elec_power = (Vs*~Is) * Pbase;
}


//TPIM steady state function
void motor::TPIMSteadyState(TIMESTAMP t1) {
		double omgr0_delta = 1;
		int32 count = 1;
		gld::complex alpha = gld::complex(0.0,0.0);
		gld::complex A1, A2, A3, A4;
		gld::complex B1, B2, B3, B4;
		gld::complex C1, C2, C3, C4;
		gld::complex D1, D2, D3, D4;
		gld::complex E1, E2, E3, E4;
		gld::complex Vap;
		gld::complex Van;
		// gld::complex Vaz;

		alpha = complex_exp(2.0/3.0 * PI);

        Vap = (Vas + alpha * Vbs + alpha * alpha * Vcs) / 3.0;
        Van = (Vas + alpha * alpha * Vbs + alpha * Vcs) / 3.0;
        // Vaz = 1.0/3.0 * (Vas + Vbs + Vcs);

        // printf("Enter steady state: \n");

        if (motor_status == statusRUNNING){

			while (fabs(omgr0_delta) > speed_error && count < iteration_count) {
				count++;

				// pre-calculate complex coefficients of the 4 linear equations associated with flux state variables
				A1 = -(gld::complex(0.0,1.0) * ws_pu + rs_pu / sigma1) ;
				B1 =  0.0 ;
				C1 =  rs_pu / sigma1 * lm / Lr ;
				D1 =  0.0 ;
				E1 = Vap ;

				A2 = 0.0;
				B2 = -(gld::complex(0.0,-1.0) * ws_pu + rs_pu / sigma1) ;
				C2 = 0.0 ;
				D2 = rs_pu / sigma1 * lm / Lr ;
				E2 = ~Van ;

				A3 = rr_pu / sigma2 * lm / Ls ;
				B3 =  0.0 ;
				C3 =  -(gld::complex(0.0,1.0) * (ws_pu - wr_pu) + rr_pu / sigma2) ;
				D3 =  0.0 ;
				E3 =  0.0 ;

				A4 = 0.0 ;
				B4 =  rr_pu / sigma2 * lm / Ls ;
				C4 = 0.0 ;
				D4 = -(gld::complex(0.0,1.0) * (-ws_pu - wr_pu) + rr_pu / sigma2) ;
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

				//See which model we're using
				if (motor_torque_usage_method==modelSpeedFour)
				{
					//Compute updated mechanical torque
					Tmech = 0.85 + 0.15*(wr_pu*wr_pu*wr_pu*wr_pu);

					//Assign it in, too
					Tmech_eff = Tmech;
				}
				//Default else - direct method, so just read (in case player/else changes it)

				// iteratively compute speed increment to make sure Telec matches Tmech during steady state mode
				// if it does not match, then update current and Telec using new wr_pu
				omgr0_delta = ( Telec - Tmech_eff ) / ((double)iteration_count);

				//update the rotor speed to make sure electrical torque traces mechanical torque
				if (wr_pu + omgr0_delta > -10) {
					wr_pu = wr_pu + omgr0_delta;
					wr = wr_pu * wbase;
				}
				else {
					wr_pu = -10;
					wr = wr_pu * wbase;
				}

			}  // End while

        } // End if TPIM is assumed running
        else // must be the TPIM stalled or otherwise not running
        {
        	wr_pu = 0.0;
			wr = 0.0;

			// pre-calculate complex coefficients of the 4 linear equations associated with flux state variables
			A1 = -(gld::complex(0.0,1.0) * ws_pu + rs_pu / sigma1) ;
			B1 =  0.0 ;
			C1 =  rs_pu / sigma1 * lm / Lr ;
			D1 =  0.0 ;
			E1 = Vap ;

			A2 = 0.0;
			B2 = -(gld::complex(0.0,-1.0) * ws_pu + rs_pu / sigma1) ;
			C2 = 0.0 ;
			D2 = rs_pu / sigma1 * lm / Lr ;
			E2 = ~Van ;

			A3 = rr_pu / sigma2 * lm / Ls ;
			B3 =  0.0 ;
			C3 =  -(gld::complex(0.0,1.0) * (ws_pu - wr_pu) + rr_pu / sigma2) ;
			D3 =  0.0 ;
			E3 =  0.0 ;

			A4 = 0.0 ;
			B4 =  rr_pu / sigma2 * lm / Ls ;
			C4 = 0.0 ;
			D4 = -(gld::complex(0.0,1.0) * (-ws_pu - wr_pu) + rr_pu / sigma2) ;
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

		motor_elec_power = (Vap * ~Ips + Van * Ins_cj) * Pbase; // VA

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
	psi_b = (Ib*Xm) / ((gld::complex(0.0,1.0)*(ws+wr)*To_prime)+psi_sat);
	psi_f = psi_f + ( If*(Xm/To_prime) - (gld::complex(0.0,1.0)*(ws-wr) + psi_sat/To_prime)*psi_f )*dTime;   

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
	psi_qr = gld::complex(0.0,1.0)*psi_f + gld::complex(0,-1)*psi_b;

	// d and q-axis current equations
	Ids = (-(gld::complex(0.0,1.0)*ws_pu*(Xm/Xr)*psi_dr) + Vs.Mag()) / ((gld::complex(0.0,1.0)*ws_pu*Xd_prime)+Rds);  
	Iqs = (-(gld::complex(0.0,1.0)*ws_pu*(n*Xm/Xr)*psi_qr) + Vs.Mag()) / ((gld::complex(0.0,1.0)*ws_pu*Xq_prime)+(gld::complex(0.0,1.0)/ws_pu*Xc)+Rqs); 

	// f and b current equations
	If = (Ids-(gld::complex(0.0,1.0)*n*Iqs))*0.5;
	Ib = (Ids+(gld::complex(0.0,1.0)*n*Iqs))*0.5;

	// system current and power equations
	Is = (Ids + Iqs)*complex_exp(Vs.Arg());
	motor_elec_power = (Vs*~Is) * Pbase;

    //electrical torque 
	Telec = (Xm/Xr)*2*(If.Im()*psi_f.Re() - If.Re()*psi_f.Im() - Ib.Im()*psi_b.Re() + Ib.Re()*psi_b.Im()); 

	//See which model we're using
	if (motor_torque_usage_method==modelSpeedFour)
	{
		//Compute updated mechanical torque
		Tmech = 0.85 + 0.15*(wr_pu*wr_pu*wr_pu*wr_pu);
	}
	//Default else - direct method, so just read (in case player/else changes it)

	// speed equation 
	wr = wr + (((Telec-Tmech)*wbase)/(2*H))*dTime;

    // speeds below 0 should be avioded
	if (wr < 0) {
		wr = 0;
	}

	//Get the per-unit version
	wr_pu = wr / wbase;
}

//Dynamic updates for TPIM
void motor::TPIMDynamic(double curr_delta_time, double dTime) {
	gld::complex alpha = gld::complex(0.0,0.0);
	gld::complex Vap;
	gld::complex Van;

	// variables related to predictor step
	gld::complex A1p, C1p;
	gld::complex B2p, D2p;
	gld::complex A3p, C3p;
	gld::complex B4p, D4p;
	gld::complex dphips_prev_dt ;
	gld::complex dphins_cj_prev_dt ;
	gld::complex dphipr_prev_dt ;
	gld::complex dphinr_cj_prev_dt ;
	gld::complex domgr0_prev_dt ;

	// variables related to corrector step
	gld::complex A1c, C1c;
	gld::complex B2c, D2c;
	gld::complex A3c, C3c;
	gld::complex B4c, D4c;
	gld::complex dphips_dt ;
	gld::complex dphins_cj_dt ;
	gld::complex dphipr_dt ;
	gld::complex dphinr_cj_dt ;
	gld::complex domgr0_dt ;

	alpha = complex_exp(2.0/3.0 * PI);

    Vap = (Vas + alpha * Vbs + alpha * alpha * Vcs) / 3.0;
    Van = (Vas + alpha * alpha * Vbs + alpha * Vcs) / 3.0;

    TPIMupdateVars();

	//See which model we're using
	if (motor_torque_usage_method==modelSpeedFour)
	{
		//Compute updated mechanical torque
		Tmech = 0.85 + 0.15*(wr_pu*wr_pu*wr_pu*wr_pu);

		//Assign it in
		Tmech_eff = Tmech;
	}
	else
	{
		//Do the old routine - only engage if above 1.0
		if (wr_pu >= 1.0)
		{
			Tmech_eff = Tmech;
		}
	}

    //*** Predictor Step ***//
    // predictor step 1 - calculate coefficients
    A1p = -(gld::complex(0.0,1.0) * ws_pu + rs_pu / sigma1) ;
    C1p =  rs_pu / sigma1 * lm / Lr ;

    B2p = -(gld::complex(0.0,-1.0) * ws_pu + rs_pu / sigma1) ;
    D2p = rs_pu / sigma1 *lm / Lr ;

    A3p = rr_pu / sigma2 * lm / Ls ;
    C3p =  -(gld::complex(0.0,1.0) * (ws_pu - wr_pu_prev) + rr_pu / sigma2) ;

    B4p =  rr_pu / sigma2 * lm / Ls ;
    D4p = -(gld::complex(0.0,1.0) * (-ws_pu - wr_pu_prev) + rr_pu / sigma2) ;

    // predictor step 2 - calculate derivatives
    dphips_prev_dt =  ( Vap + A1p * phips_prev + C1p * phipr_prev ) * wbase;  // pu/s
    dphins_cj_prev_dt = ( ~Van + B2p * phins_cj_prev + D2p * phinr_cj_prev ) * wbase; // pu/s
    dphipr_prev_dt  =  ( C3p * phipr_prev + A3p * phips_prev ) * wbase; // pu/s
    dphinr_cj_prev_dt = ( D4p * phinr_cj_prev  + B4p * phins_cj_prev ) * wbase; // pu/s
    domgr0_prev_dt =  ( (~phips_prev * Ips_prev + ~phins_cj_prev * Ins_cj_prev).Im() - Tmech_eff - Kfric * wr_pu_prev ) / (2.0 * H); // pu/s


    // predictor step 3 - integrate for predicted state variable
    phips = phips_prev +  dphips_prev_dt * dTime;
    phins_cj = phins_cj_prev + dphins_cj_prev_dt * dTime;
    phipr = phipr_prev + dphipr_prev_dt * dTime ;
    phinr_cj = phinr_cj_prev + dphinr_cj_prev_dt * dTime ;
    wr_pu = wr_pu_prev + domgr0_prev_dt.Re() * dTime ;
	wr = wr_pu * wbase;

    // predictor step 4 - update outputs using predicted state variables
    Ips = (phips - phipr * lm / Lr) / sigma1;  // pu
    Ipr = (phipr - phips * lm / Ls) / sigma2;  // pu
    Ins_cj = (phins_cj - phinr_cj * lm / Lr) / sigma1 ; // pu
    Inr_cj = (phinr_cj - phins_cj * lm / Ls) / sigma2; // pu


    //*** Corrector Step ***//
    // assuming no boundary variable (e.g. voltage) changes during each time step,
    // so predictor and corrector steps are placed in the same class function

    // corrector step 1 - calculate coefficients using predicted state variables
    A1c = -(gld::complex(0.0,1.0) * ws_pu + rs_pu / sigma1) ;
    C1c =  rs_pu / sigma1 * lm / Lr ;

    B2c = -(gld::complex(0.0,-1.0) * ws_pu + rs_pu / sigma1) ;
    D2c = rs_pu / sigma1 *lm / Lr ;

    A3c = rr_pu / sigma2 * lm / Ls ;
    C3c =  -(gld::complex(0.0,1.0) * (ws_pu - wr_pu) + rr_pu / sigma2) ;  // This coeff. is different from predictor

    B4c =  rr_pu / sigma2 * lm / Ls ;
    D4c = -(gld::complex(0.0,1.0) * (-ws_pu - wr_pu) + rr_pu / sigma2) ; // This coeff. is different from predictor

    // corrector step 2 - calculate derivatives
    dphips_dt =  ( Vap + A1c * phips + C1c * phipr ) * wbase;
    dphins_cj_dt = ( ~Van + B2c * phins_cj + D2c * phinr_cj ) * wbase ;
    dphipr_dt  =  ( C3c * phipr + A3c * phips ) * wbase;
    dphinr_cj_dt = ( D4c * phinr_cj  + B4c * phins_cj ) * wbase;
    domgr0_dt =  1.0/(2.0 * H) * ( (~phips * Ips + ~phins_cj * Ins_cj).Im() - Tmech_eff - Kfric * wr_pu );

    // corrector step 3 - integrate
    phips = phips_prev +  (dphips_prev_dt + dphips_dt) * dTime/2.0;
    phins_cj = phins_cj_prev + (dphins_cj_prev_dt + dphins_cj_dt) * dTime/2.0;
    phipr = phipr_prev + (dphipr_prev_dt + dphipr_dt) * dTime/2.0 ;
    phinr_cj = phinr_cj_prev + (dphinr_cj_prev_dt + dphinr_cj_dt) * dTime/2.0 ;
    wr_pu = wr_pu_prev + (domgr0_prev_dt + domgr0_dt).Re() * dTime/2.0 ;
	wr = wr_pu * wbase;

	if (wr_pu < -10.0) { // speeds below -10 should be avoided
		wr_pu = -10.0;
		wr = wr_pu * wbase;
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

	motor_elec_power = (Vap * ~Ips + Van * Ins_cj) * Pbase; // VA

}

//Function to perform exp(j*val) (basically a complex rotation)
gld::complex motor::complex_exp(double angle)
{
	gld::complex output_val;

	//exp(jx) = cos(x)+j*sin(x)
	output_val = gld::complex(cos(angle),sin(angle));

	return output_val;
}

// Function to do the inverse of a 4x4 matrix
int motor::invertMatrix(gld::complex TF[16], gld::complex ITF[16])
{
    gld::complex inv[16], det;
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

    det = gld::complex(1,0) / det;

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
		if (*obj!=nullptr)
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
