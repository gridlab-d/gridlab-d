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
			PT_double, "capacitor_A[VAr]", PADDR(capacitor_A),
			PT_double, "capacitor_B[VAr]", PADDR(capacitor_B),
			PT_double, "capacitor_C[VAr]", PADDR(capacitor_C),
			PT_double, "time_delay[s]", PADDR(time_delay),
			PT_object,"Remote_Node",PADDR(RemoteNode),
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
	control = MANUAL;
	control_level = INDIVIDUAL;
	pt_phase = PHASE_A;
	voltage_set_high = 0.0;
	voltage_set_low = 0.0;
	var_close = 0.0;
	var_open = 0.0;
	volt_close = 0.0;
	volt_open = 0.0;
	pt_ratio = 60;
	time_delay = 0.0;
	time_to_change = 0;
	last_time = 0;

	//throw "capacitor implementation is not complete"; - removed while debugging
	return result;
}

int capacitor::init(OBJECT *parent)
{
	int result = node::init();

	//Calculate capacitor values as admittance
	cap_value[0] = complex(0,capacitor_A/(nominal_voltage * nominal_voltage));
	cap_value[1] = complex(0,capacitor_B/(nominal_voltage * nominal_voltage));
	cap_value[2] = complex(0,capacitor_C/(nominal_voltage * nominal_voltage));

	return result;
}

TIMESTAMP capacitor::sync(TIMESTAMP t0)
{
	complex VoltVals[3];
	node *RNode = OBJECTDATA(RemoteNode,node);
	TIMESTAMP result;

	//Update time tracker
	time_to_change -= (t0 - last_time);

	if (last_time!=t0)	//If we've transitioned, update the transition value
	{
		last_time = t0;
	}

	if (time_to_change<=0)	//Only let us iterate if our time has changed
	{
		//Perform the previous settings first
		if ((pt_phase & (PHASE_A)) == PHASE_A)
			switchA_state=switchA_state_Next;
			
		if ((pt_phase & (PHASE_B)) == PHASE_B)
			switchB_state=switchB_state_Next;

		if ((pt_phase & (PHASE_C)) == PHASE_C)
			switchC_state=switchC_state_Next;

		//Update controls
		if ((pt_phase & PHASE_N) == (PHASE_N))	//See if we are interested in L-N or L-L voltages
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

		switch (control) {
			case MANUAL:  // manual
				//May not really be anything to do in here - all handled in common set below.
				/// @todo implement capacity manual control closed (ticket #189)
				break;
			case VAR:  // VAr
				/// @todo implement capacity var control closed (ticket #190)
				break;
			case VOLT:
				{// V
				if ((pt_phase & PHASE_N) == (PHASE_N))// Line to Neutral connections
				{
					if ((pt_phase & (PHASE_A | PHASE_N)) == (PHASE_A | PHASE_N))
						if (voltage_set_low >= VoltVals[0].Mag())
							switchA_state_Next=CLOSED;
						else if (voltage_set_high <= VoltVals[0].Mag())
							switchA_state_Next=OPEN;
						else;
						
					if ((pt_phase & (PHASE_B | PHASE_N)) == (PHASE_B | PHASE_N))
						if (voltage_set_low >= VoltVals[1].Mag())
							switchB_state_Next=CLOSED;
						else if (voltage_set_high <= VoltVals[1].Mag())
							switchB_state_Next=OPEN;
						else;

					if ((pt_phase & (PHASE_C | PHASE_N)) == (PHASE_C | PHASE_N))
						if (voltage_set_low >= VoltVals[2].Mag())
							switchC_state_Next=CLOSED;
						else if (voltage_set_high <= VoltVals[2].Mag())
							switchC_state_Next=OPEN;
						else;
				}
				else // Line to Line connections
				{
					if ((pt_phase & (PHASE_A | PHASE_B)) == (PHASE_A | PHASE_B))
					   voltaged[0];
					if ((pt_phase & (PHASE_B | PHASE_C)) == (PHASE_B | PHASE_C))
					   voltaged[1];
					if ((pt_phase & (PHASE_C | PHASE_A)) == (PHASE_C | PHASE_A))
					voltaged[2];
				}
				/// @todo implement capacity volt control closed (ticket #191)
				break;
				}
			case VARVOLT:  // VAr, V
				/// @todo implement capacity varvolt control closed (ticket #192)
				break;
			default:
				break;
		}

		if (control_level == BANK)
		{
			if ((switchA_state_Next | switchB_state_Next | switchC_state_Next) == CLOSED)
				switchA_state_Next = switchB_state_Next = switchC_state_Next = CLOSED;	//Bank control, close them all
			else
				switchA_state_Next = switchB_state_Next = switchC_state_Next = OPEN;	//Bank control, open them all (this should never be an issue)
		}


		//See what our new delay needs to be
		if ((switchA_state != switchA_state_Next) | (switchB_state != switchB_state_Next) | (switchC_state != switchC_state_Next))	//Not same
			time_to_change=(int64)time_delay;
		else
			time_to_change=-1;	//Flag value to pass normal result, not a modified version

	}

	if ((pt_phase & (PHASE_A)) == PHASE_A)
		shunt[0] = switchA_state==CLOSED ? cap_value[0] : complex(0.0);
		
	if ((pt_phase & (PHASE_B)) == PHASE_B)
		shunt[1] = switchB_state==CLOSED ? cap_value[1] : complex(0.0);

	if ((pt_phase & (PHASE_C)) == PHASE_C)
		shunt[2] = switchC_state==CLOSED ? cap_value[2] : complex(0.0);


	result = node::sync(t0);

	if ((time_to_change!=-1) && ((result - t0) > time_to_change))
		result = t0 + time_to_change;

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
