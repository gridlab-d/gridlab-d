// $Id: performance_motor.cpp
//	Copyright (C) 2020 Battelle Memorial Institute

#include <stdlib.h>	
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "performance_motor.h"

//@TODO: Still needs to have the stalling and reconnecting portion done.  Right now, will return to service immediately

CLASS* performance_motor::oclass = NULL;
CLASS* performance_motor::pclass = NULL;

/**
* constructor.  Class registration is only called once to 
* register the class with the core. Include parent class constructor (node)
*
* @param mod a module structure maintained by the core
*/

static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;


performance_motor::performance_motor(MODULE *mod):node(mod)
{
	if(oclass == NULL)
	{
		pclass = node::oclass;
		
		oclass = gl_register_class(mod,"performance_motor",sizeof(performance_motor),passconfig|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class performance_motor";
		else
			oclass->trl = TRL_PRINCIPLE;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "node",

			PT_enumeration,"motor_status",PADDR(motor_status),PT_DESCRIPTION,"the current status of the motor",
				PT_KEYWORD,"RUNNING",(enumeration)statusRUNNING,
				PT_KEYWORD,"STALLED",(enumeration)statusSTALLED,
				PT_KEYWORD,"TRIPPED",(enumeration)statusTRIPPED,
				PT_KEYWORD,"OFF",(enumeration)statusOFF,
			PT_int32,"motor_status_number",PADDR(motor_status),PT_DESCRIPTION,"the current status of the motor as an integer",

            PT_double,"Pbase[W]",PADDR(P_base),
            PT_complex,"power_value[W]",PADDR(PowerVal),
            PT_double,"delta_f_val",PADDR(delta_f_val),
            PT_double,"Vbrk",PADDR(Vbrk),
            PT_double,"Vstallbrk",PADDR(Vstallbrk),
            PT_double,"Vstall",PADDR(Vstall),
			PT_double,"Vrst",PADDR(Vrst),
            PT_double,"P_val",PADDR(P_val),
            PT_double,"Q_val",PADDR(Q_val),
            PT_double,"P_0",PADDR(P_0),
            PT_double,"Q_0",PADDR(Q_0),
            PT_double,"K_p1",PADDR(K_p1),
            PT_double,"K_q1",PADDR(K_q1),
            PT_double,"K_p2",PADDR(K_p2),
            PT_double,"K_q2",PADDR(K_q2),
            PT_double,"N_p1",PADDR(N_p1),
            PT_double,"N_q1",PADDR(N_q1),
            PT_double,"N_p2",PADDR(N_p2),
            PT_double,"N_q2",PADDR(N_q2),
            PT_double,"CmpKpf",PADDR(CmpKpf),
            PT_double,"CmpKqf",PADDR(CmpKqf),
            PT_complex,"stall_impedance[pu]",PADDR(stall_impedance_pu),
            PT_double,"Tstall[s]",PADDR(Tstall),
            PT_double,"reconnect_time[s]",PADDR(reconnect_time),
            PT_double,"Pinit",PADDR(Pinit),
            PT_double,"Vinit",PADDR(Vinit),
            PT_double,"stall_time_tracker[s]",PADDR(stall_time_tracker),
            PT_double,"reconnect_time_tracker[s]",PADDR(reconnect_time_tracker),
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_performance_motor)==NULL)
			GL_THROW("Unable to publish motor deltamode function");
    }
}

int performance_motor::create()
{
	int result = node::create();

	last_time_val = 0.0;
	curr_time_val = 0.0;

    PowerVal = complex(0.0,0.0);

    delta_f_val = 0.0;

    Vstallbrk = 0.0;

    Vbrk = 0.86;
    Vstall = 0.70;
	Vrst = 0.70;
    P_val = 0.0;
    Q_val = 0.0;
    P_0 = 0.0;
    Q_0 = 0.0;
    K_p1 = 0.0;
    K_q1 = 6.0;
    K_p2 = 12.0;
    K_q2 = 11.0;
    N_p1 = 1.0;
    N_q1 = 2.0;
    N_p2 = 3.2;
    N_q2 = 2.5;
    CmpKpf = 1.0;
    CmpKqf = -3.3;
    stall_impedance_pu = complex(0.124,0.114);
    Tstall = 0.033;
    reconnect_time = 0.4;

    CompPF = 0.97;
    Pinit = 1.0;
    Vinit = 1.0;

    stall_time_tracker = 0.0;
    reconnect_time_tracker = 0.0;

    P_base = 2690;  //Watts
    stall_admittance = complex(0.0,0.0);

	return result;
}

int performance_motor::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	int result;
    double Z_base;
	gld_property *temp_gld_property;
	gld_wlock *test_rlock;

	//Now run node init, as necessary
	result = node::init(parent);

	// Check what phases are connected on this motor
	int num_phases = 0;
	if (has_phase(PHASE_A)==true)
		num_phases++;

	if (has_phase(PHASE_B)==true)
		num_phases++;

	if (has_phase(PHASE_C)==true)
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

    //Compute the impedance base
    if (motor_op_mode == modeSPIM)
    {
        Z_base = (4.0 * nominal_voltage * nominal_voltage)/P_base;  //Triplex is 12, so has to be double nominal
    }
    else    //Must be three-phase
    {
        Z_base = (3.0 * nominal_voltage * nominal_voltage)/P_base;
    }

    //Convert the impedance to an admittance and call it good
    stall_admittance = complex(1.0,0.0)/(stall_impedance_pu * Z_base);

    //Compute the initial values
    P_0 = Pinit - K_p1*pow((Vinit - Vbrk),N_p1);
    Q_0 = Pinit*(sqrt(1.0 - CompPF*CompPF)/CompPF) - K_p1*pow((1.0 - Vbrk),N_p1);
    Vstallbrk = Vbrk + pow((K_p1/K_p2),(1.0/(N_p2-N_p1)));

    //Update time variable
	curr_time_val = (double)gl_globalclock;

	return result;
}

int performance_motor::isa(char *classname)
{
	return strcmp(classname,"performance_motor")==0 || node::isa(classname);
}

TIMESTAMP performance_motor::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	//Zero our accumulators, just because
    shunt[0] = shunt[1] = shunt[2] = complex(0.0,0.0);
    power[0] = power[1] = power[2] = complex(0.0,0.0);

	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::presync(t1);
	return result;
}

TIMESTAMP performance_motor::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	//Call the motor update
	update_motor_equations();

	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::sync(t1);

    return result;
}

TIMESTAMP performance_motor::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	//Must be at the bottom, or the new values will be calculated after the fact
	TIMESTAMP result = node::postsync(t1);
	return result;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////

//Module-level call
SIMULATIONMODE performance_motor::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	OBJECT *hdr = OBJECTHDR(this);
	STATUS return_status_val;
	bool temp_house_motor_state;
	gld_wlock *test_rlock;

	// make sure to capture the current time
	curr_time_val = gl_globaldeltaclock;

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
		if (fmeas_type != FM_NONE) {
			//Initialize dynamics
			init_freq_dynamics();
		}
	}//End first pass and timestep of deltamode (initial condition stuff)

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Zero all the accumulators
		shunt[0] = shunt[1] = shunt[2] = complex(0.0,0.0);
		power[0] = power[1] = power[2] = complex(0.0,0.0);

		//Call presync-equivalent items
		NR_node_presync_fxn(0);

        //Call the motor update
		update_motor_equations();

		//Call sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

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

	}

    return SM_EVENT;
}

//Overall motor equation function
void performance_motor::update_motor_equations(void)
{
    double V_current;

    //See which mode we're in - use this to compute "the voltage", since it's a positive sequence model
    if (motor_op_mode == modeSPIM)
    {
        V_current = voltaged[0].Mag()/(2.0 * nominal_voltage);    //V_12
    }
    else    //Must be TPIM, but elimination!
    {
        //Just do an average magnitude
        V_current = (voltage[0].Mag() + voltage[1].Mag() + voltage[2].Mag())/(3.0 * nominal_voltage);
    }

    //Zero the current output, just because
    P_val = 0.0;
    Q_val = 0.0;
    PowerVal = complex(0.0,0.0);

    //Check our state
    if (motor_status == statusRUNNING)
    {
        //See which range we're in
        if (V_current > Vbrk)
        {
            P_val = (P_0 + K_p1*pow((V_current - Vbrk),N_p1))*(1.0 + CmpKpf*delta_f_val);
            Q_val = (Q_0 + K_q1*pow((V_current - Vbrk),N_q1))*(1.0 + CmpKqf*delta_f_val);
        }
        else if ((V_current < Vbrk) && (V_current > Vstallbrk))
        {
            P_val = (P_0 + K_p2*pow((V_current - Vbrk),N_p2))*(1.0 + CmpKpf*delta_f_val);
            Q_val = (Q_0 + K_q2*pow((V_current - Vbrk),N_q2))*(1.0 + CmpKqf*delta_f_val);
        }
        else    //We somehow are stalled, change and set the power
        {
            motor_status = statusSTALLED;

            //Zero our values, just because

            //See what kind of motor we are
            if (motor_op_mode == modeSPIM)
            {
                //Assumes no children - base functionality
                shunt[2] = stall_admittance;
            }
            else    //Must be three-phase
            {
                //Post it equally
                shunt[0] = stall_admittance;
                shunt[1] = stall_admittance;
                shunt[2] = stall_admittance;
            }
            //Others are already zerod
        }

        //General post here, just because (better than duplicating)
        //Convert the values into power values - posting it that way "just because"
        PowerVal = complex(P_val,Q_val) * P_base;

        //Post it, as appropriate
        if (motor_op_mode == modeSPIM)
        {
            //Post direction
            power[2] = PowerVal;
        }
        else    //Three-phase
        {
            power[0] = PowerVal / 3.0;
            power[1] = PowerVal / 3.0;
            power[2] = PowerVal / 3.0;
        }
    }
    else if (motor_status == statusSTALLED)
    {
        //See what kind of motor we are
        if (motor_op_mode == modeSPIM)
        {
            //Assumes no children - base functionality
            shunt[2] = stall_admittance;
        }
        else    //Must be three-phase
        {
            //Post it equally
            shunt[0] = stall_admittance;
            shunt[1] = stall_admittance;
            shunt[2] = stall_admittance;
        }
        //Others are already zerod

		//If goes above Vrest, restart - this will delay one timestep, but meh - crude implementation
		if (V_current > Vrst)
		{
			motor_status = statusRUNNING;
		}
    }
    else if (motor_status == statusTRIPPED)
    {
        //Stuff here?
    }
    else    //Must be off
    {
        //Other stuff here?
    }
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
EXPORT int create_performance_motor(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(performance_motor::oclass);
		if (*obj!=NULL)
		{
			performance_motor *my = OBJECTDATA(*obj,performance_motor);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(performance_motor);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_performance_motor(OBJECT *obj)
{
	try {
		performance_motor *my = OBJECTDATA(obj,performance_motor);
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
EXPORT TIMESTAMP sync_performance_motor(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	TIMESTAMP t1 = TS_INVALID;
	performance_motor *my = OBJECTDATA(obj,performance_motor);
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
	SYNC_CATCHALL(performance_motor);
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
EXPORT int isa_performance_motor(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,performance_motor)->isa(classname);
	} else {
		return 0;
	}
}

/** 
* DELTA MODE
*/
EXPORT SIMULATIONMODE interupdate_performance_motor(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	performance_motor *my = OBJECTDATA(obj,performance_motor);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_performance_motor(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

/**@}*/
