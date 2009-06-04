// $Id: capacitor.cpp 1182 2008-12-22 22:08:36Z dchassin $
/**	Copyright (C) 2008 Battelle Memorial Institute

	@file capacitor.cpp
	@addtogroup capacitor
	@ingroup powerflow
	@{
*/

#include <stdlib.h>	
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "capacitor.h"

//////////////////////////////////////////////////////////////////////////
// capacitor CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* capacitor::oclass = NULL;
CLASS* capacitor::pclass = NULL;

/**
* constructor.  Class registration is only called once to 
* register the class with the core. Include parent class constructor (node)
*
* @param mod a module structure maintained by the core
*/
capacitor::capacitor(MODULE *mod):node(mod)
{
	if(oclass == NULL)
	{
		pclass = node::oclass;
		
		oclass = gl_register_class(mod,"capacitor",sizeof(capacitor),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
        if(oclass == NULL)
            GL_THROW("unable to register object class implemented by %s",__FILE__);

		if(gl_publish_variable(oclass,
			PT_INHERIT, "node",
			PT_set, "pt_phase", PADDR(pt_phase),
				PT_KEYWORD, "A",PHASE_A,
				PT_KEYWORD, "B",PHASE_B,
				PT_KEYWORD, "C",PHASE_C,
				PT_KEYWORD, "D",PHASE_D,
				PT_KEYWORD, "N",PHASE_N,
			PT_set, "phases_connected", PADDR(phases_connected),
				PT_KEYWORD, "A",PHASE_A,
				PT_KEYWORD, "B",PHASE_B,
				PT_KEYWORD, "C",PHASE_C,
				PT_KEYWORD, "D",PHASE_D,
				PT_KEYWORD, "N",PHASE_N,
			PT_enumeration, "switchA", PADDR(switchA_state),
				PT_KEYWORD, "OPEN", OPEN,
				PT_KEYWORD, "CLOSED", CLOSED,
				PT_enumeration, "switchB", PADDR(switchB_state),
				PT_KEYWORD, "OPEN", OPEN,
				PT_KEYWORD, "CLOSED", CLOSED,
				PT_enumeration, "switchC", PADDR(switchC_state),
				PT_KEYWORD, "OPEN", OPEN,
				PT_KEYWORD, "CLOSED", CLOSED,
			PT_enumeration, "control", PADDR(control),
				PT_KEYWORD, "MANUAL", MANUAL,
				PT_KEYWORD, "VAR", VAR,
				PT_KEYWORD, "VOLT", VOLT,
				PT_KEYWORD, "VARVOLT", VARVOLT,
			PT_double, "voltage_set_high[V]", PADDR(voltage_set_high), 
			PT_double, "voltage_set_low[V]", PADDR(voltage_set_low),
			PT_double, "VAr_set_high[VAr]", PADDR(VAr_set_high),
			PT_double, "VAr_set_low[VAr]", PADDR(VAr_set_low),
			PT_double, "capacitor_A[VAr]", PADDR(capacitor_A),
			PT_double, "capacitor_B[VAr]", PADDR(capacitor_B),
			PT_double, "capacitor_C[VAr]", PADDR(capacitor_C),
			PT_double, "time_delay[s]", PADDR(time_delay),
			PT_double, "dwell_time[s]", PADDR(dwell_time),
			PT_object,"sense_node",PADDR(RemoteNode),
			PT_enumeration, "control_level", PADDR(control_level),
				PT_KEYWORD, "BANK", BANK,
				PT_KEYWORD, "INDIVIDUAL", INDIVIDUAL, 
         	NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int capacitor::create()
{
	int result = node::create();
		
	// Set up defaults
	switchA_state = OPEN;
	switchB_state = OPEN;
	switchC_state = OPEN;
	switchA_state_Next = OPEN;
	switchB_state_Next = OPEN;
	switchC_state_Next = OPEN;
	switchA_state_Prev = OPEN;
	switchB_state_Prev = OPEN;
	switchC_state_Prev = OPEN;
	switchA_state_Req_Next = OPEN;
	switchB_state_Req_Next = OPEN;
	switchC_state_Req_Next = OPEN;
	control = MANUAL;
	control_level = INDIVIDUAL;
	pt_phase = PHASE_A;
	voltage_set_high = 0.0;
	voltage_set_low = 0.0;
	VAr_set_high = 0.0;
	VAr_set_low = 0.0;
	time_delay = 0.0;
	dwell_time = 0.0;
	time_to_change = 0;
	dwell_time_left = 0;
	last_time = 0;

	return result;
}

int capacitor::init(OBJECT *parent)
{
	int result = node::init();

	OBJECT *obj = OBJECTHDR(this);

	if ((capacitor_A == 0.0) && (capacitor_A == 0.0) && (capacitor_A == 0.0))
		gl_error("Capacitor:%d does not have any capacitance values defined!",obj->id);
		/*  TROUBLESHOOT
		The capacitor does not have any actual capacitance values defined.  This results in the capacitor doing
		nothing at all and results in no change to the system.  Specify a value with capacitor_A, capacitor_B, or capacitor_C.
		*/

	//Calculate capacitor values as admittance - handling of Delta - Wye conversion will be handled later (if needed)
	cap_value[0] = complex(0,capacitor_A/(nominal_voltage * nominal_voltage));
	cap_value[1] = complex(0,capacitor_B/(nominal_voltage * nominal_voltage));
	cap_value[2] = complex(0,capacitor_C/(nominal_voltage * nominal_voltage));

	if (control==VAR)
		gl_warning("VAR control is implemented as a \"Best Guess\" implementation.  Use at your own risk.");

	return result;
}

TIMESTAMP capacitor::presync(TIMESTAMP t0)
{
	node *RNode = OBJECTDATA(RemoteNode,node);

	if (control==VAR)	//Grab the power values before they are zeroed out (mainly by FBS)
	{
		//Only grabbing L-N power.  No effective way to take L-N back to L-L power (no reference)
		if (RNode == NULL)	//L-N power
		{
			VArVals[0] = (voltage[0]*~current_inj[0]).Im();
			VArVals[1] = (voltage[1]*~current_inj[1]).Im();
			VArVals[2] = (voltage[2]*~current_inj[2]).Im();
		}
		else
		{
			VArVals[0] = (RNode->voltage[0]*~RNode->current_inj[0]).Im();
			VArVals[1] = (RNode->voltage[1]*~RNode->current_inj[1]).Im();
			VArVals[2] = (RNode->voltage[2]*~RNode->current_inj[2]).Im();
		}
	}

	return node::presync(t0);
}

TIMESTAMP capacitor::sync(TIMESTAMP t0)
{
	complex VoltVals[3];
	complex temp_shunt[3];
	node *RNode = OBJECTDATA(RemoteNode,node);
	bool Phase_Mismatch = false;
	TIMESTAMP result;

	//Update time trackers
	time_to_change -= (t0 - last_time);
	dwell_time_left -= (t0 - last_time);

	if (last_time!=t0)	//If we've transitioned, update the transition value
	{
		last_time = t0;
	}

	if (control==MANUAL)	//Manual requires slightly different scheme
	{
		if (switchA_state != switchA_state_Prev)
		{
			switchA_state_Next = switchA_state;
			switchA_state = switchA_state_Prev;
			time_to_change=(int64)time_delay;	//Change detected on anything, so reset time delay
		}

		if (switchB_state != switchB_state_Prev)
		{
			switchB_state_Next = switchB_state;
			switchB_state = switchB_state_Prev;
			time_to_change=(int64)time_delay;	//Change detected on anything, so reset time delay
		}

		if (switchC_state != switchC_state_Prev)
		{
			switchC_state_Next = switchC_state;
			switchC_state = switchC_state_Prev;
			time_to_change=(int64)time_delay;	//Change detected on anything, so reset time delay
		}
	}

	if (time_to_change<=0)	//Only let us iterate if our time has changed
	{
		//Perform the previous settings first
		if ((phases_connected & (PHASE_A)) == PHASE_A)
			switchA_state=switchA_state_Next;
			
		if ((phases_connected & (PHASE_B)) == PHASE_B)
			switchB_state=switchB_state_Next;

		if ((phases_connected & (PHASE_C)) == PHASE_C)
			switchC_state=switchC_state_Next;

		//Update controls
		if (control==VOLT)
		{
			if ((pt_phase & PHASE_D) != (PHASE_D))	//See if we are interested in L-N or L-L voltages
			{
				if (RNode == NULL)	//L-N voltages
				{
					VoltVals[0] = voltage[0];
					VoltVals[1] = voltage[1];
					VoltVals[2] = voltage[2];
				}
				else
				{
					VoltVals[0] = RNode->voltage[0];
					VoltVals[1] = RNode->voltage[1];
					VoltVals[2] = RNode->voltage[2];
				}
			}
			else				//L-L voltages
			{
				if (RNode == NULL)
				{
					VoltVals[0] = voltaged[0];
					VoltVals[1] = voltaged[1];
					VoltVals[2] = voltaged[2];
				}
				else
				{
					VoltVals[0] = RNode->voltaged[0];
					VoltVals[1] = RNode->voltaged[1];
					VoltVals[2] = RNode->voltaged[2];
				}
			}
		}
		else;	//MANUAL and VAR-VOLT (which do not need values or do nothing right now)
				//VAR handled in presynch

		switch (control) {
			case MANUAL:  // manual
				{
					switchA_state_Prev = switchA_state_Next;	//Prepare working variables for the transition that happened
					switchB_state_Prev = switchB_state_Next;
					switchC_state_Prev = switchC_state_Next;
				}
				break;
			case VAR:  // VAr
				{
					//Power values only calculated as L-N (L-L not possible), but logic left for both L-N and L-L until we determine the true scheme
					if ((pt_phase & PHASE_D) != (PHASE_D))// Line to Neutral connections
					{
						if ((pt_phase & PHASE_A) == PHASE_A)
						{
							if (VAr_set_low >= VArVals[0])
								switchA_state_Req_Next=CLOSED;
							else if (VAr_set_high <= VArVals[0])
								switchA_state_Req_Next=OPEN;
							else;
						}
							
						if ((pt_phase & PHASE_B) == PHASE_B)
						{
							if (VAr_set_low >= VArVals[1])
								switchB_state_Req_Next=CLOSED;
							else if (VAr_set_high <= VArVals[1])
								switchB_state_Req_Next=OPEN;
							else;
						}

						if ((pt_phase & PHASE_C) == PHASE_C)
						{
							if (VAr_set_low >= VArVals[2])
								switchC_state_Req_Next=CLOSED;
							else if (VAr_set_high <= VArVals[2])
								switchC_state_Req_Next=OPEN;
							else;
						}
					}
					else // Line to Line connections
					{
						if ((pt_phase & (PHASE_A | PHASE_B)) == (PHASE_A | PHASE_B))
						{
							if (VAr_set_low >= VArVals[0])	//VArVals handled above to use L-L power instead
								switchA_state_Req_Next=CLOSED;				//switchA assigned to AB connection (delta-connection in loads)
							else if (VAr_set_high <= VArVals[0])
								switchA_state_Req_Next=OPEN;
							else;
						}

						if ((pt_phase & (PHASE_B | PHASE_C)) == (PHASE_B | PHASE_C))
						{
							if (VAr_set_low >= VArVals[1])	//VArVals handled above to use L-L power instead
								switchB_state_Req_Next=CLOSED;				//switchB assigned to BC connection (delta-connection in loads)
							else if (VAr_set_high <= VArVals[1])
								switchB_state_Req_Next=OPEN;
							else;
						}

						if ((pt_phase & (PHASE_C | PHASE_A)) == (PHASE_C | PHASE_A))
						{
							if (VAr_set_low >= VArVals[2])	//VArVals handled above to use L-L power instead
								switchC_state_Req_Next=CLOSED;				//switchC assigned to CA connection (delta-connection in loads)
							else if (VAr_set_high <= VArVals[2])
								switchC_state_Req_Next=OPEN;
							else;
						}
					}
				}
				break;
			case VOLT:
				{// V
					if ((pt_phase & PHASE_D) != (PHASE_D))// Line to Neutral connections
					{
						if ((pt_phase & PHASE_A) == PHASE_A)
						{
							if (voltage_set_low >= VoltVals[0].Mag())
								switchA_state_Req_Next=CLOSED;
							else if (voltage_set_high <= VoltVals[0].Mag())
								switchA_state_Req_Next=OPEN;
							else;
						}
							
						if ((pt_phase & PHASE_B) == PHASE_B)
						{
							if (voltage_set_low >= VoltVals[1].Mag())
								switchB_state_Req_Next=CLOSED;
							else if (voltage_set_high <= VoltVals[1].Mag())
								switchB_state_Req_Next=OPEN;
							else;
						}

						if ((pt_phase & PHASE_C) == PHASE_C)
						{
							if (voltage_set_low >= VoltVals[2].Mag())
								switchC_state_Req_Next=CLOSED;
							else if (voltage_set_high <= VoltVals[2].Mag())
								switchC_state_Req_Next=OPEN;
							else;
						}
					}
					else // Line to Line connections
					{
						if ((pt_phase & (PHASE_A | PHASE_B)) == (PHASE_A | PHASE_B))
						{
							if (voltage_set_low >= VoltVals[0].Mag())	//VoltVals handled above to use L-L voltages instead
								switchA_state_Req_Next=CLOSED;				//switchA assigned to AB connection (delta-connection in loads)
							else if (voltage_set_high <= VoltVals[0].Mag())
								switchA_state_Req_Next=OPEN;
							else;
						}

						if ((pt_phase & (PHASE_B | PHASE_C)) == (PHASE_B | PHASE_C))
						{
							if (voltage_set_low >= VoltVals[1].Mag())	//VoltVals handled above to use L-L voltages instead
								switchB_state_Req_Next=CLOSED;				//switchB assigned to BC connection (delta-connection in loads)
							else if (voltage_set_high <= VoltVals[1].Mag())
								switchB_state_Req_Next=OPEN;
							else;
						}

						if ((pt_phase & (PHASE_C | PHASE_A)) == (PHASE_C | PHASE_A))
						{
							if (voltage_set_low >= VoltVals[2].Mag())	//VoltVals handled above to use L-L voltages instead
								switchC_state_Req_Next=CLOSED;				//switchC assigned to CA connection (delta-connection in loads)
							else if (voltage_set_high <= VoltVals[2].Mag())
								switchC_state_Req_Next=OPEN;
							else;
						}
					}
				}
				break;
			case VARVOLT:  // VAr, V
				/// @todo implement capacity varvolt control closed (ticket #192)
				break;
			default:
				break;
		}

		//Checking of dwell times to see if we can transition the next state or not
		if (((switchA_state_Prev != switchA_state_Req_Next) || (switchB_state_Prev != switchB_state_Req_Next) || (switchC_state_Prev != switchC_state_Req_Next)) && (control != MANUAL))
		{
			dwell_time_left = (int64)dwell_time;			//Reset counter
			switchA_state_Prev = switchA_state_Req_Next;	//Reset trackers
			switchB_state_Prev = switchB_state_Req_Next;
			switchC_state_Prev = switchC_state_Req_Next;
		}

		if ((dwell_time_left <= 0) && (control!=MANUAL))	//Time criteria met, but not manual (this doesn't exist in MANUAL configuration)
		{
			switchA_state_Next = switchA_state_Req_Next;	//Transition things, this should reset the time_to_change timer as well
			switchB_state_Next = switchB_state_Req_Next;
			switchC_state_Next = switchC_state_Req_Next;
		}

		if (control_level == BANK)	//Check the "sense" phases.  If any are closed, close all of them
		{
			bool bank_A, bank_B, bank_C;

			if ((pt_phase & PHASE_D) != (PHASE_D))// Line to Neutral connections
			{
					bank_A = (((pt_phase & PHASE_A) == PHASE_A) && (switchA_state_Next == CLOSED));
					bank_B = (((pt_phase & PHASE_B) == PHASE_B) && (switchB_state_Next == CLOSED));
					bank_C = (((pt_phase & PHASE_C) == PHASE_C) && (switchC_state_Next == CLOSED));
			}
			else							//Line-Line connections
			{
					bank_A = (((pt_phase & (PHASE_A | PHASE_B)) == (PHASE_A | PHASE_B)) && (switchA_state_Next == CLOSED));
					bank_B = (((pt_phase & (PHASE_B | PHASE_C)) == (PHASE_B | PHASE_C)) && (switchB_state_Next == CLOSED));
					bank_C = (((pt_phase & (PHASE_C | PHASE_A)) == (PHASE_C | PHASE_A)) && (switchC_state_Next == CLOSED));
			}

			if (control==MANUAL)
			{
				if ((bank_A | bank_B | bank_C) == true)
					switchA_state_Next = switchB_state_Next = switchC_state_Next = CLOSED;	//Bank control, close them all
				else
					switchA_state_Next = switchB_state_Next = switchC_state_Next = OPEN;	//Bank control, open them all (this should never be an issue)

				switchA_state_Prev = switchB_state_Prev = switchC_state_Prev = switchA_state_Next; //Slight override, otherwise it oscillates
			}
			else	//Other
			{
				if ((bank_A | bank_B | bank_C) == true)
					switchA_state_Next = switchB_state_Next = switchC_state_Next = CLOSED;	//Bank control, close them all
				else
					switchA_state_Next = switchB_state_Next = switchC_state_Next = OPEN;	//Bank control, open them all (this should never be an issue)

				switchA_state_Req_Next = switchB_state_Req_Next = switchC_state_Req_Next = switchA_state_Next; //Slight override, otherwise it oscillates
				switchA_state_Prev = switchB_state_Prev = switchC_state_Prev = switchA_state_Next;
			}
		}


		//See what our new delay needs to be
		if ((switchA_state != switchA_state_Next) | (switchB_state != switchB_state_Next) | (switchC_state != switchC_state_Next))	//Not same
			time_to_change=(int64)time_delay;
		else
			time_to_change=-1;	//Flag value to pass normal result, not a modified version

	}

	//Put in appropriate values.  If we are a mismatch, convert things appropriately first
	//Check based on connection input (so could check Wye values and then switch in a delta connected CAP
	if ((((phases_connected & PHASE_D) != PHASE_D) && ((phases & PHASE_D) != PHASE_D)) || ((phases_connected & PHASE_D) == PHASE_D) && ((phases & PHASE_D) == PHASE_D))	//Correct connection
	{
		temp_shunt[0] = cap_value[0];
		temp_shunt[1] = cap_value[1];
		temp_shunt[2] = cap_value[2];
	}
	else if (((phases_connected & PHASE_D) != PHASE_D) && ((phases & PHASE_D) == PHASE_D))	//Delta connected node, but Wye connected Cap
	{
		GL_THROW("Capacitor:%d is Wye-connected on a Delta-connected node.  This is not supported at this time.",OBJECTHDR(this)->id);
		/*  TROUBLESHOOT
		Wye-connected capacitors on a delta-connected node are not supported at this time.  They may be added in a future release when
		the functionality is needed.
		*/
	}
	else if (((phases_connected & PHASE_D) == PHASE_D) && ((phases & PHASE_D) != PHASE_D))	//Wye connected node, but Delta connected Cap
	{
		complex cap_temp[3];

		cap_temp[0] = cap_temp[1] = cap_temp[2] = 0.0;

		//Update values of capacitors that are closed
		if (((phases_connected & PHASE_A) == PHASE_A) && (switchA_state==CLOSED))
			cap_temp[0] = cap_value[0];

		if (((phases_connected & PHASE_B) == PHASE_B) && (switchB_state==CLOSED))
			cap_temp[1] = cap_value[1];

		if (((phases_connected & PHASE_C) == PHASE_C) && (switchC_state==CLOSED))
			cap_temp[2] = cap_value[2];

		//Convert to Wye equivalent
		temp_shunt[0] = (voltaged[0]*cap_temp[0] - voltaged[2]*cap_temp[2]) / voltage[0];
		temp_shunt[1] = (voltaged[1]*cap_temp[1] - voltaged[0]*cap_temp[0]) / voltage[1];
		temp_shunt[2] = (voltaged[2]*cap_temp[2] - voltaged[1]*cap_temp[1]) / voltage[2];

		Phase_Mismatch = true;	//Flag us as an exception.  Otherwise values are wrong.
	}
	else	//No case should exist here, so if it does, scream about it.
		GL_THROW("Unable to determine connection for capacitor:%d",OBJECTHDR(this)->id);
		/*  TROUBLESHOOT
		The capacitor object encountered a connection it was unable to decipher.  Please submit this as
		a bug report with your code.
		*/

	if (Phase_Mismatch==true)	//On/offs are handled above, all must be turned on for here! (Delta-Wye/Wye-Delta type conversions)
	{
		shunt[0] = temp_shunt[0];
		shunt[1] = temp_shunt[1];
		shunt[2] = temp_shunt[2];
	}
	else	//Matched connection cases, implement "normally"
	{
		//Perform actual switching operation
		if ((phases_connected & (PHASE_A)) == PHASE_A)
			shunt[0] = switchA_state==CLOSED ? temp_shunt[0] : complex(0.0);
			
		if ((phases_connected & (PHASE_B)) == PHASE_B)
			shunt[1] = switchB_state==CLOSED ? temp_shunt[1] : complex(0.0);

		if ((phases_connected & (PHASE_C)) == PHASE_C)
			shunt[2] = switchC_state==CLOSED ? temp_shunt[2] : complex(0.0);
	}

	//Perform our inherited class sync
	result = node::sync(t0);

	if (dwell_time_left>0)		//Dwelling in progress, flag us to iterate when it should be done
		result = t0 + dwell_time_left;	//If nothing changes in the system, it will reiterate at the dwell time.  Otherwise, it will come in earlier
	else if (time_to_change>0)	//Change in progress, flag us to iterate when it should be done
		result = t0 + time_to_change;
	else;

	return result;
}

int capacitor::isa(char *classname)
{
	return strcmp(classname,"capacitor")==0 || node::isa(classname);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: capacitor
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_capacitor(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(capacitor::oclass);
		if (*obj!=NULL)
		{
			capacitor *my = OBJECTDATA(*obj,capacitor);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_capacitor: %s", msg);
	}
	return 0;
}



/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_capacitor(OBJECT *obj)
{
	capacitor *my = OBJECTDATA(obj,capacitor);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		GL_THROW("%s (capacitor:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_capacitor(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	capacitor *pObj = OBJECTDATA(obj,capacitor);
	try {
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(t0);
		case PC_BOTTOMUP:
			return pObj->sync(t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
	} catch (const char *error) {
		GL_THROW("%s (capacitor:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return 0; 
	} catch (...) {
		GL_THROW("%s (capacitor:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
		return 0;
	}
}

/**
* Allows the core to discover whether obj is a subtype of this class.
*
* @param obj a pointer to this object
* @param classname the name of the object the core is testing
*
* @return 0 if obj is a subtype of this class
*/
EXPORT int isa_capacitor(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,capacitor)->isa(classname);
}

/**@}*/
