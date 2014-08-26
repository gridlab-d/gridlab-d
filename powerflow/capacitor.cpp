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
		
		oclass = gl_register_class(mod,"capacitor",sizeof(capacitor),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class capacitor";
		else
			oclass->trl = TRL_PROVEN;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "node",
			PT_set, "pt_phase", PADDR(pt_phase),PT_DESCRIPTION,"Phase(s) that the PT is on, used as measurement points for control",
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "D",(set)PHASE_D,
				PT_KEYWORD, "N",(set)PHASE_N,
			PT_set, "phases_connected", PADDR(phases_connected),PT_DESCRIPTION,"phases capacitors connected to",
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "D",(set)PHASE_D,
				PT_KEYWORD, "N",(set)PHASE_N,
			PT_enumeration, "switchA", PADDR(switchA_state),PT_DESCRIPTION,"capacitor A switch open or close",
				PT_KEYWORD, "OPEN", (enumeration)OPEN,
				PT_KEYWORD, "CLOSED", (enumeration)CLOSED,
				PT_enumeration, "switchB", PADDR(switchB_state),PT_DESCRIPTION,"capacitor B switch open or close",
				PT_KEYWORD, "OPEN", (enumeration)OPEN,
				PT_KEYWORD, "CLOSED", (enumeration)CLOSED,
				PT_enumeration, "switchC", PADDR(switchC_state),PT_DESCRIPTION,"capacitor C switch open or close",
				PT_KEYWORD, "OPEN", (enumeration)OPEN,
				PT_KEYWORD, "CLOSED", (enumeration)CLOSED,
			PT_enumeration, "control", PADDR(control),PT_DESCRIPTION,"control operation strategy",
				PT_KEYWORD, "MANUAL", (enumeration)MANUAL,
				PT_KEYWORD, "VAR", (enumeration)VAR,
				PT_KEYWORD, "VOLT", (enumeration)VOLT,
				PT_KEYWORD, "VARVOLT", (enumeration)VARVOLT,
				PT_KEYWORD, "CURRENT", (enumeration)CURRENT,
			PT_double, "cap_A_switch_count", PADDR(cap_switchA_count),PT_DESCRIPTION,"number of switch operations on Phase A",
			PT_double, "cap_B_switch_count", PADDR(cap_switchB_count),PT_DESCRIPTION,"number of switch operations on Phase B",
			PT_double, "cap_C_switch_count", PADDR(cap_switchC_count),PT_DESCRIPTION,"number of switch operations on Phase C",
			PT_double, "voltage_set_high[V]", PADDR(voltage_set_high), PT_DESCRIPTION,"Turn off if voltage is above this set point",
			PT_double, "voltage_set_low[V]", PADDR(voltage_set_low), PT_DESCRIPTION,"Turns on if voltage is below this set point",
			PT_double, "VAr_set_high[VAr]", PADDR(VAr_set_high),PT_DESCRIPTION,"high VAR set point for VAR control (turn off)",
			PT_double, "VAr_set_low[VAr]", PADDR(VAr_set_low),PT_DESCRIPTION,"low VAR set point for VAR control (turn on)",
			PT_double, "current_set_low[A]", PADDR(current_set_low),PT_DESCRIPTION,"high current set point for current control mode (turn on)",
			PT_double, "current_set_high[A]", PADDR(current_set_high),PT_DESCRIPTION,"low current set point for current control mode (turn off)",
			PT_double, "capacitor_A[VAr]", PADDR(capacitor_A),PT_DESCRIPTION,"Capacitance value for phase A or phase AB",
			PT_double, "capacitor_B[VAr]", PADDR(capacitor_B),PT_DESCRIPTION,"Capacitance value for phase B or phase BC",
			PT_double, "capacitor_C[VAr]", PADDR(capacitor_C),PT_DESCRIPTION,"Capacitance value for phase C or phase CA",
			PT_double, "cap_nominal_voltage[V]", PADDR(cap_nominal_voltage),PT_DESCRIPTION,"Nominal voltage for the capacitor. Used for calculation of capacitance value",
			PT_double, "time_delay[s]", PADDR(time_delay),PT_DESCRIPTION,"control time delay",
			PT_double, "dwell_time[s]", PADDR(dwell_time),PT_DESCRIPTION,"Time for system to remain constant before a state change will be passed",
			PT_double, "lockout_time[s]", PADDR(lockout_time),PT_DESCRIPTION,"Time for capacitor to remain locked out from further switching operations (VARVOLT control)",
			PT_object, "remote_sense",PADDR(RemoteSensor),PT_DESCRIPTION,"Remote object for sensing values used for control schemes",
			PT_object, "remote_sense_B", PADDR(SecondaryRemote),PT_DESCRIPTION,"Secondary Remote object for sensing values used for control schemes (VARVOLT uses two)",
			PT_enumeration, "control_level", PADDR(control_level),PT_DESCRIPTION,"define bank or individual control",
				PT_KEYWORD, "BANK", (enumeration)BANK,
				PT_KEYWORD, "INDIVIDUAL", (enumeration)INDIVIDUAL, 
         	NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

			//Publish deltamode functions
			if (gl_publish_function(oclass,	"delta_linkage_node", (FUNCTIONADDR)delta_linkage)==NULL)
				GL_THROW("Unable to publish load delta_linkage function");
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_node)==NULL)
				GL_THROW("Unable to publish load deltamode function");
			if (gl_publish_function(oclass,	"delta_freq_pwr_object", (FUNCTIONADDR)delta_frequency_node)==NULL)
				GL_THROW("Unable to publish load deltamode function");
    }
}

int capacitor::create()
{
	int result = node::create();
		
	// Set up defaults
	switchA_state = OPEN;
	switchB_state = OPEN;
	switchC_state = OPEN;
	cap_switchA_count = -1;
	cap_switchB_count = -1;
	cap_switchC_count = -1;
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
	voltage_set_high = 0.0;
	voltage_set_low = 0.0;
	VAr_set_high = 0.0;
	VAr_set_low = 0.0;
	current_set_low = 0.0;
	current_set_high = 0.0;
	time_delay = 0.0;
	dwell_time = 0.0;
	lockout_time = 0.0;
	time_to_change = 0;
	dwell_time_left = 0;
	lockout_time_left_A = 0;
	lockout_time_left_B = 0;
	lockout_time_left_C = 0;
	last_time = 0;
	cap_nominal_voltage = 0.0;
	RemoteSensor = NULL;
	SecondaryRemote=NULL;
	RNode = NULL;
	RLink = NULL;
	VArVals[0] = VArVals[1] = VArVals[2] = 0.0;
	CurrentVals[0] = CurrentVals[1] = CurrentVals[2] = 0.0;

	NotFirstIteration=false;
	Iteration_Toggle = false;
	NR_cycle_cap = true;

	return result;
}

int capacitor::init(OBJECT *parent)
{
	int result = node::init();

	OBJECT *obj = OBJECTHDR(this);

	if ((control == VARVOLT) && (SecondaryRemote!=NULL))	//Something set in the secondary sensor location & VARVOLT scheme
	{
		if (RemoteSensor==NULL)	//But nothing in the first
		{
			GL_THROW("Please set the remote sensing location on capacitor:%d under \"remote_sense\"",obj->id);
			/*  TROUBLESHOOT
			Capacitor objects have two "remote sensor" fields that can be set.  The secondary sensor is only used in VAR-VOLT schemes
			and requires that the primary sensor is used first.
			*/
		}
		else	//It has something too - let's make sure they are right
		{
			if ((gl_object_isa(RemoteSensor,"node","powerflow")) && (gl_object_isa(SecondaryRemote,"link","powerflow")))	//Node in 1, link in 2
			{
				RNode = OBJECTDATA(RemoteSensor,node);
				RLink = OBJECTDATA(SecondaryRemote,link_object);
			}
			else if ((gl_object_isa(RemoteSensor,"link","powerflow")) && (gl_object_isa(SecondaryRemote,"node","powerflow")))	//link in 1, node in 2
			{
				RNode = OBJECTDATA(SecondaryRemote,node);
				RLink = OBJECTDATA(RemoteSensor,link_object);
			}
			else
			{
				GL_THROW("For two remote sensors, Capacitor:%d requires one link and one node object specified.",obj->id);
				/*  TROUBLESHOOT
				To use both remote sensor fields, the capacitor must in VAR-VOLT control mode.  Furthermore, these two sensors are used
				to determine voltage and power measurements elsewhere on the system, so a remote node and a remote link must be specified.
				*/
			}
		}
	}
	else if (((control==VARVOLT) || (control==CURRENT)) && (SecondaryRemote==NULL) && (RemoteSensor != NULL) && gl_object_isa(RemoteSensor,"link","powerflow"))	//VAR-VOLT scheme, one sensor defined
	{
		RLink = OBJECTDATA(RemoteSensor,link_object);
	}
	else if (SecondaryRemote != NULL)	//Should only be populated for VARVOLT scheme,
	{
		gl_warning("Capacitor:%d has a secondary sensor specified, but is not in VARVOLT control.  This will be ignored.",obj->id);
		/*  TROUBLESHOOT
		The secondary remote sensor field is only used in the VAR-VOLT control scheme.  When not under this control scheme, any inputs
		to this field are ignored.
		*/
	}

	if ((RemoteSensor != NULL) && (control != VARVOLT))	//Something is specified
	{
		if (gl_object_isa(RemoteSensor,"node","powerflow"))
		{
			RNode = OBJECTDATA(RemoteSensor,node);	//Get remote node information
		}
		else if (gl_object_isa(RemoteSensor,"link","powerflow"))
		{
			RLink = OBJECTDATA(RemoteSensor,link_object);	//Get remote link information
		}
	}

	if ((capacitor_A == 0.0) && (capacitor_B == 0.0) && (capacitor_C == 0.0))
		gl_error("Capacitor:%d does not have any capacitance values defined!",obj->id);
		/*  TROUBLESHOOT
		The capacitor does not have any actual capacitance values defined.  This results in the capacitor doing
		nothing at all and results in no change to the system.  Specify a value with capacitor_A, capacitor_B, or capacitor_C.
		*/

	//Define the capacitor nominal voltage if unspecified
	if (cap_nominal_voltage==0.0)
	{
		cap_nominal_voltage=nominal_voltage;
	}

	if ((cap_nominal_voltage==0.0) && (nominal_voltage==0.0))	//Check both just in csae, but if cap_nominal is 0 at this point, both probably are
		GL_THROW("Capcitor:%d does not have a node nominal or capacitor nominal voltage specified.",obj->id);
		/*  TROUBLESHOOT
		The capacitor needs the cap_nominal_voltage or nominal_voltage property set to calculate the resultant
		capacitance value from the power rating.  Please specify one or both of these values.
		*/

	//Calculate capacitor values as admittance - handling of Delta - Wye conversion will be handled later (if needed)
	cap_value[0] = complex(0,capacitor_A/(cap_nominal_voltage * cap_nominal_voltage));
	cap_value[1] = complex(0,capacitor_B/(cap_nominal_voltage * cap_nominal_voltage));
	cap_value[2] = complex(0,capacitor_C/(cap_nominal_voltage * cap_nominal_voltage));

	if ((control == VOLT) && ((voltage_set_high == 0) || (voltage_set_low == 0)))
		gl_warning("Capacitor:%d does not have one or both of its voltage set points set.",obj->id);
		/*  TROUBLESHOOT
		If the VOLT control schemes is active, you must specify the voltage set points for the band of operation.  Without these,
		the capacitor will not function and will effectively perform no action.
		*/

	if ((control == VARVOLT) && (voltage_set_high == 0))
		gl_warning("Capacitor:%d does not have its upper voltage limit set.",obj->id);
		/*  TROUBLESHOOT
		When under the VAR-VOLT control scheme, the voltage_set_high set point should be set to the upper voltage
		limit the capacitor will remain switched on for.
		*/

	if (((control == VAR) || (control == VARVOLT) ) && ((VAr_set_high == 0) || (VAr_set_low == 0)))
		gl_warning("Capacitor:%d does not have one or both of its VAr set points set.",obj->id);
		/*  TROUBLESHOOT
		If VAR or VARVOLT control schemes are active, you must specify the reactive power set points for the band of operation.  Without these,
		the capacitor will not function and will effectively perform no action.
		*/

	if ((control==CURRENT) && ((current_set_high == 0) || (current_set_low == 0)))
		gl_warning("Capacitor:%d does not have one or both of its current set points set.",obj->id);
		/*  TROUBLESHOOT
		If the CURRENT control scheme is active, you must specify the current set points for the band of operation.  Without these,
		the capacitor will not function and will effectively perform no action.
		*/

	if (((control == VAR) || (control == VARVOLT) || (control==CURRENT)) && (RLink == NULL))
		GL_THROW("VAR, VARVOLT, or CURRENT control on capacitor:%d requires a remote link to monitor.",obj->id);
		/*  TROUBLESHOOT
		For VAR, VARVOLT, or CURRENT control to work on the capacitor, a remote line must be specified to monitor reactive power flow.  Without this, no operations will
		occur within the capacitor.
		*/

	if (((control==VAR) || (control==VARVOLT)) && (VAr_set_low > VAr_set_high))	//Check limits
		GL_THROW("The lower VAr limit of capacitor:%d is larger than the high limit setpoint!",obj->id);
		/*  TROUBLESHOOT
		Under VAr controls, the lower VAr set point must be less than the upper VAr set point.  Please set these accordingly.
		*/

	if (((control==VOLT) || (control==VARVOLT)) && (voltage_set_low > voltage_set_high))	//Check limits
		GL_THROW("The lower voltage limit of capacitor:%d is larger than the high limit setpoint!",obj->id);
		/*  TROUBLESHOOT
		Under voltage controls, the lower voltage set point must be less than the upper voltage set point.  Please set these accordingly.
		*/

	if ((control==CURRENT) && (current_set_low > current_set_high))	//Check limits
		GL_THROW("The lower current limit of capacitor:%d is larger than the high limit setpoint!",obj->id);
		/*  TROUBLESHOOT
		Under CURRENT controls, the lower current set point must be less than the upper current set point.  Please set these accordingly.
		*/

	if ((control != MANUAL) && (time_delay == 0) && (dwell_time==0))
		gl_warning("Automatic controls can oscillate to the iteration limit with no time delays.  To prevent this, ensure your switching limits are reasonable.");
		/*  TROUBLESHOOT
		With no time delays set (time_delay or dwell_time), the capacitor performs everything instantaneously.  If the set points are too close together,
		the capacitor may oscillate until the convergence limit is reached.  Avoid this for proper answers.
		*/

	if ((control == VARVOLT) && (lockout_time==0))
		gl_warning("No lockout time specified for capacitor:%d's VAR-VOLT control scheme.  May switch excessively.",obj->id);
		/*  TROUBLESHOOT
		Without a lockout_time set, the capacitor will only turn off for the delays instituted in time_delay.
		*/

	if ((control != MANUAL) && ((pt_phase & (PHASE_A | PHASE_B | PHASE_C)) == 0))	//Nothing specified in pt_phases
		GL_THROW("Capacitor:%d is set to an automatic scheme, but is not monitoring any phases.",obj->id);
		/*  TROUBLESHOOT
		A capacitor is setup to use one of the automatic control schemes (VAR, VOLT, VARVOLT), but does not have a phase
		specified in pt_phase to monitor.  As such, the capacitor will not do anything.  Please specify phase(s) to monitor
		in pt_phase.
		*/

	if ((control == MANUAL) && (control_level == BANK) && ((pt_phase & (PHASE_A | PHASE_B | PHASE_C)) == 0))	//Manual bank control with no phases monitored
		gl_warning("Capacitor:%d is set to manual bank mode, but does not know which phase to monitor.",obj->id);
		/*  TROUBLESHOOT
		A capacitor is set to manual bank mode.  However, pt_phase is empty and the capacitor does not know which phase
		to monitor to switch all conencted phases.  Specify at least one phase in pt_phase to enable the bank control
		*/

	if ((phases_connected & (PHASE_A | PHASE_B | PHASE_C)) == 0)
	{
		gl_warning("No capacitor phase connection information is available for capacitor:%d.  Defaulting to the phases property.",obj->id);
		/*  TROUBLESHOOT
		The capacitor does not have any information specified about how the capacitors are actually connected.  The phases property of the
		capacitor will be utilized instead.  If this is incorrect, explicitly specify the phases in phases_connected.
		*/

		phases_connected = phases;	//Just use what the node has set
	}

	//Set transition variables to beginning value initially
	switchA_state_Req_Next = switchA_state_Prev = switchA_state_Next = (CAPSWITCH)switchA_state;
	switchB_state_Req_Next = switchB_state_Prev = switchB_state_Next = (CAPSWITCH)switchB_state;
	switchC_state_Req_Next = switchC_state_Prev = switchC_state_Next = (CAPSWITCH)switchC_state;

	//Perform phase checks - make sure what we want to look at actually exists
	if (((control!=MANUAL) && (control!=VOLT)) && ((RLink->phases & pt_phase) != pt_phase))	//VAR, VOLTVAR, CURRENT
	{
		GL_THROW("One of the monitored remote link phases for capacitor:%d does not exist",obj->id);
		/*  TROUBLESHOOT
		One of the phases specified in pt_phase does not exist on the link being monitored by the capacitor.
		This will cause either no results or erroneous results.  Ensure pt_phase and the phases property of the
		remote link are compatible.
		*/
	}
	else if (((control==VOLT) || (control==VARVOLT)) && (RNode != NULL) && ((RNode->phases & pt_phase) != pt_phase))	//RNode check
	{
		GL_THROW("One of the monitored remote node phases for capacitor:%d does not exist",obj->id);
		/*  TROUBLESHOOT
		One of the phases specified in pt_phase does not exist on the node being monitored by the capacitor.
		This will cause either no results or erroneous results.  Ensure pt_phase and the phases property of the
		remote node are compatible.
		*/
	}
	else if (((control==VOLT) || (control==VARVOLT)) && (RNode == NULL) && ((phases & pt_phase) != pt_phase))	//Self node check
	{
		GL_THROW("One of the monitored node phases for capacitor:%d does not exist",obj->id);
		/*  TROUBLESHOOT
		One of the phases specified in pt_phase does not exist on the capacitor node.
		This will cause either no results or erroneous results.  Ensure pt_phase and the phases property of the
		capacitor are compatible.
		*/
	}
	if(cap_switchA_count < 0)
		cap_switchA_count = 0;
	if(cap_switchB_count < 0)
		cap_switchB_count = 0;
	if(cap_switchC_count < 0)
		cap_switchC_count = 0;
	
	prev_time = gl_globalclock;
	
	prev_switchA_state = init_switchA_state = (CAPSWITCH)switchA_state;
	prev_switchB_state = init_switchB_state = (CAPSWITCH)switchB_state;
	prev_switchC_state = init_switchC_state = (CAPSWITCH)switchC_state;
	
	switchA_changed = switchB_changed = switchC_changed = 2;

	return result;
}

TIMESTAMP capacitor::sync(TIMESTAMP t0)
{
	if (service_status == ND_IN_SERVICE)
	{
		complex VoltVals[3];
		complex temp_shunt[3];
		bool Phase_Mismatch = false;
		TIMESTAMP result;

		//Update time trackers
		time_to_change -= (t0 - last_time);
		dwell_time_left -= (t0 - last_time);
		lockout_time_left_A -= (t0 - last_time);
		lockout_time_left_B -= (t0 - last_time);
		lockout_time_left_C -= (t0 - last_time);

		if (last_time!=t0)	//If we've transitioned, update the transition value
		{
			last_time = t0;
		}

		if (control==MANUAL)	//Manual requires slightly different scheme
		{
			if (switchA_state != switchA_state_Prev)
			{
				if ((control_level == BANK) && ((pt_phase & PHASE_A) != PHASE_A) && (time_delay==0))	//Special handling of no time delay
				{
					switchA_state = switchA_state_Next;
				}
				else
				{
					switchA_state_Next =(CAPSWITCH) switchA_state;
					switchA_state = switchA_state_Prev;
				}
				time_to_change=(int64)time_delay;	//Change detected on anything, so reset time delay
			}

			if (switchB_state != switchB_state_Prev)
			{
				if ((control_level == BANK) && ((pt_phase & PHASE_B) != PHASE_B) && (time_delay==0))
				{
					switchB_state = switchB_state_Next;
				}
				else
				{
					switchB_state_Next = (CAPSWITCH)switchB_state;
					switchB_state = switchB_state_Prev;
				}
				time_to_change=(int64)time_delay;	//Change detected on anything, so reset time delay
			}

			if (switchC_state != switchC_state_Prev)
			{
				if ((control_level == BANK) && ((pt_phase & PHASE_C) != PHASE_C) && (time_delay==0))	//Special handling of no time delay
				{
					switchC_state = switchC_state_Next;
				}
				else
				{
					switchC_state_Next = (CAPSWITCH)switchC_state;
					switchC_state = switchC_state_Prev;
				}
				time_to_change=(int64)time_delay;	//Change detected on anything, so reset time delay
			}
		}

		if ((time_to_change<=0) && NotFirstIteration)	//Only let us iterate if our time has changed
		{
			//Perform the previous settings first
			if ((phases_connected & (PHASE_A)) == PHASE_A)
				switchA_state=switchA_state_Next;
				
			if ((phases_connected & (PHASE_B)) == PHASE_B)
				switchB_state=switchB_state_Next;

			if ((phases_connected & (PHASE_C)) == PHASE_C)
				switchC_state=switchC_state_Next;

			//Update controls
			if ((control==VOLT) || (control==VARVOLT))
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
						LOCK_OBJECT(OBJECTHDR(RNode));
						VoltVals[0] = RNode->voltage[0];
						VoltVals[1] = RNode->voltage[1];
						VoltVals[2] = RNode->voltage[2];
						UNLOCK_OBJECT(OBJECTHDR(RNode));

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
						LOCK_OBJECT(OBJECTHDR(RNode));
						VoltVals[0] = RNode->voltaged[0];
						VoltVals[1] = RNode->voltaged[1];
						VoltVals[2] = RNode->voltaged[2];
						UNLOCK_OBJECT(OBJECTHDR(RNode));
					}
				}
			}
			else;	//MANUAL and VAR-VOLT (which do not need values or do nothing right now)
					//VAR handled in postsynch

			switch (control) {
				case MANUAL:  // manuals
					{
						switchA_state_Prev = switchA_state_Next;	//Prepare working variables for the transition that happened
						switchB_state_Prev = switchB_state_Next;
						switchC_state_Prev = switchC_state_Next;
					}
					break;
				case VARVOLT:  // VAr-Volt - same as VAR, but switching operations contingent (and interrupted) by voltage limit
				case VAR:  // VAr
					{
						//Power values only calculated as L-N (L-L not possible or really feasible)
						if ((pt_phase & PHASE_A) == PHASE_A)
						{
							if (VAr_set_low >= VArVals[0])
								switchA_state_Req_Next=OPEN;
							else if (VAr_set_high <= VArVals[0])
								switchA_state_Req_Next=CLOSED;
							else;
						}
							
						if ((pt_phase & PHASE_B) == PHASE_B)
						{
							if (VAr_set_low >= VArVals[1])
								switchB_state_Req_Next=OPEN;
							else if (VAr_set_high <= VArVals[1])
								switchB_state_Req_Next=CLOSED;
							else;
						}

						if ((pt_phase & PHASE_C) == PHASE_C)
						{
							if (VAr_set_low >= VArVals[2])
								switchC_state_Req_Next=OPEN;
							else if (VAr_set_high <= VArVals[2])
								switchC_state_Req_Next=CLOSED;
							else;
						}

						//VAR-VOLT portion of control scheme
						if (control == VARVOLT)	//Check all voltages.  If exceed limit, force capacitor open instead of closed (regardless of how it was)
						{
							bool lockout_state_change[3];
							lockout_state_change[0] = lockout_state_change[1] = lockout_state_change[2] = false;

							if ((pt_phase & PHASE_D) != (PHASE_D))// Line to Neutral connections
							{
								if (lockout_time_left_A <= 0)	//Not in lockout period
								{
									if (((pt_phase & PHASE_A) == PHASE_A) && (VoltVals[0].Mag() >= voltage_set_high))
									{
										switchA_state_Req_Next=OPEN;
										lockout_state_change[0] = true;
									}
								}
								else	//In lockout period
								{
									switchA_state_Req_Next=OPEN;
								}

								if (lockout_time_left_B <= 0)	//Not in lockout period
								{
									if (((pt_phase & PHASE_B) == PHASE_B) && (VoltVals[1].Mag() >= voltage_set_high))
									{
										switchB_state_Req_Next=OPEN;
										lockout_state_change[1] = true;
									}
								}
								else	//In lockout period
								{
									switchB_state_Req_Next=OPEN;
								}

								if (lockout_time_left_C <= 0)	//Not in lockout period
								{
									if (((pt_phase & PHASE_C) == PHASE_C) && (VoltVals[2].Mag() >= voltage_set_high))
									{
										switchC_state_Req_Next=OPEN;
										lockout_state_change[2] = true;
									}
								}
								else	//In lockout period
								{
									switchC_state_Req_Next=OPEN;
								}
							}
							else // Line to Line connections
							{
								if (lockout_time_left_A <= 0)	//Not in lockout period
								{
									if (((pt_phase & (PHASE_A | PHASE_B)) == (PHASE_A | PHASE_B)) && (VoltVals[0].Mag() >= voltage_set_high))
									{
										switchA_state_Req_Next=OPEN;
										lockout_state_change[0] = true;
									}
								}
								else	//In lockout period
								{
									switchA_state_Req_Next=OPEN;
								}

								if (lockout_time_left_B <= 0)	//Not in lockout period
								{
									if (((pt_phase & (PHASE_B | PHASE_C)) == (PHASE_B | PHASE_C)) && (VoltVals[1].Mag() >= voltage_set_high))
									{
										switchB_state_Req_Next=OPEN;
										lockout_state_change[1] = true;
									}
								}
								else	//In lockout period
								{
									switchB_state_Req_Next=OPEN;
								}

								if (lockout_time_left_C <= 0)	//Not in lockout period
								{
									if (((pt_phase & (PHASE_C | PHASE_A)) == (PHASE_C | PHASE_A)) && (VoltVals[2].Mag() >= voltage_set_high))
									{
										switchC_state_Req_Next=OPEN;
										lockout_state_change[2] = true;
									}
								}
								else	//In lockout period
								{
									switchC_state_Req_Next=OPEN;
								}
							}
							
							if ((control_level == BANK) && (lockout_state_change[0] | lockout_state_change[1] | lockout_state_change[2]))	//Bank control
							{
								lockout_time_left_A = lockout_time_left_B = lockout_time_left_C = (int64)lockout_time;	//Bank control, so lock 'em all
							}
							else if ((control_level == INDIVIDUAL) && (lockout_state_change[0] | lockout_state_change[1] | lockout_state_change[2]))	//Individual control
							{
								if (lockout_state_change[0]==true)
									lockout_time_left_A = (int64)lockout_time;

								if (lockout_state_change[1]==true)
									lockout_time_left_B = (int64)lockout_time;

								if (lockout_state_change[2]==true)
									lockout_time_left_C = (int64)lockout_time;
							}
							else;
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
				case CURRENT:
					{// I - only line currents (just consider the phase as it was, no conversions)
						if ((pt_phase & PHASE_A) == PHASE_A)
						{
							if (current_set_low >= CurrentVals[0])
								switchA_state_Req_Next=OPEN;
							else if (current_set_high <= CurrentVals[0])
								switchA_state_Req_Next=CLOSED;
							else;
						}
							
						if ((pt_phase & PHASE_B) == PHASE_B)
						{
							if (current_set_low >= CurrentVals[1])
								switchB_state_Req_Next=OPEN;
							else if (current_set_high <= CurrentVals[1])
								switchB_state_Req_Next=CLOSED;
							else;
						}

						if ((pt_phase & PHASE_C) == PHASE_C)
						{
							if (current_set_low >= CurrentVals[2])
								switchC_state_Req_Next=OPEN;
							else if (current_set_high <= CurrentVals[2])
								switchC_state_Req_Next=CLOSED;
							else;
						}
					}
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

					switchA_state = switchB_state = switchC_state = switchA_state_Next;			//Propogate the switching - if we made it this far, this should be done
				}
				else	//Other
				{
					if ((bank_A | bank_B | bank_C) == true)
						switchA_state_Next = switchB_state_Next = switchC_state_Next = CLOSED;	//Bank control, close them all
					else
						switchA_state_Next = switchB_state_Next = switchC_state_Next = OPEN;	//Bank control, open them all (this should never be an issue)

					switchA_state_Req_Next = switchB_state_Req_Next = switchC_state_Req_Next = switchA_state_Next; //Slight override, otherwise it oscillates
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
			complex numer;

			cap_temp[0] = cap_temp[1] = cap_temp[2] = 0.0;

			//Update values of capacitors that are closed
			if (((phases_connected & PHASE_A) == PHASE_A) && (switchA_state==CLOSED))
				cap_temp[0] = cap_value[0];

			if (((phases_connected & PHASE_B) == PHASE_B) && (switchB_state==CLOSED))
				cap_temp[1] = cap_value[1];

			if (((phases_connected & PHASE_C) == PHASE_C) && (switchC_state==CLOSED))
				cap_temp[2] = cap_value[2];

			//Convert to Wye equivalent - old
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

		if (NotFirstIteration==false)	//Force a reiteration on the very first pass, no matter what
		{
			result = t0;
			time_to_change = -1;
			NotFirstIteration=true;	//Deflag us
		}
		//Defaulted else, not the first iteration

		if ((solver_method == SM_NR) && (Phase_Mismatch == true))	//Kludgy test to get to reiteratte - DELETE ME OR FIX ME!
		{
			if ((Iteration_Toggle == true) && (NR_cycle_cap == true))
			{
				result = t0;
				NR_cycle_cap = false;
			}
		}

		if (result != TS_NEVER)
		{
			if (result==t0)
				return result;
			else
				return -result;
		}
		else
			return TS_NEVER;
	}
	else //Out-of-service
	{
		switchA_state = switchB_state = switchC_state = OPEN;
		//Perform actual switching operation
		if ((phases_connected & (PHASE_A)) == PHASE_A)
			shunt[0] = complex(0.0);
			
		if ((phases_connected & (PHASE_B)) == PHASE_B)
			shunt[1] = complex(0.0);

		if ((phases_connected & (PHASE_C)) == PHASE_C)
			shunt[2] = complex(0.0);

		TIMESTAMP result = node::sync(t0);

		if (result != TS_NEVER)
		{
			if (result==t0)
				return result;
			else
				return -result;
		}
		else
			return TS_NEVER;
	}
}

TIMESTAMP capacitor::postsync(TIMESTAMP t0)
{
	TIMESTAMP result;
	CAPSWITCH cap_A_test_state, cap_B_test_state, cap_C_test_state;
	complex VoltVals[3];

	if(prev_time < t0){
		Iteration_Toggle = false;
		NR_cycle_cap = true;
		prev_time = t0;
		init_switchA_state =(CAPSWITCH) prev_switchA_state;
		init_switchB_state =(CAPSWITCH) prev_switchB_state;
		init_switchC_state =(CAPSWITCH) prev_switchC_state;
		switchA_changed = 0;
		switchB_changed = 0;
		switchC_changed = 0;
		if(prev_switchA_state != switchA_state){
			prev_switchA_state = (CAPSWITCH)switchA_state;
			cap_switchA_count++;
			switchA_changed = 1;
		}
		if(prev_switchB_state != switchB_state){
			prev_switchB_state = (CAPSWITCH)switchB_state;
			cap_switchB_count++;
			switchB_changed = 1;
		}
		if(prev_switchC_state != switchC_state){
			prev_switchC_state = (CAPSWITCH)switchC_state;
			cap_switchC_count++;
			switchC_changed = 1;
		}
	}
	else
		Iteration_Toggle = true;

	if(prev_time == t0){
		if(switchA_changed == 0){
			if(prev_switchA_state != switchA_state){
				prev_switchA_state = (CAPSWITCH)switchA_state;
				cap_switchA_count++;
				switchA_changed = 1;
			}
		}
		if(switchA_changed == 1){
			if(init_switchA_state == switchA_state){
				prev_switchA_state = (CAPSWITCH)switchA_state;
				cap_switchA_count--;
				if(cap_switchA_count < 0){
					gl_error("Unusual control of the capacitor has resulted in a negative switch change count on phase A.");
					return TS_INVALID;
				}
				switchA_changed = 0;
			} else if(prev_switchA_state != switchA_state){
				prev_switchA_state = (CAPSWITCH)switchA_state;
			}
		}
		if(switchA_changed == 2){
			prev_switchA_state = (CAPSWITCH)switchA_state;
		}
		if(switchB_changed == 0){
			if(prev_switchB_state != switchB_state){
				prev_switchB_state = (CAPSWITCH)switchB_state;
				cap_switchB_count++;
				switchB_changed = 1;
			}
		}
		if(switchB_changed == 1){
			if(init_switchB_state == switchB_state){
				prev_switchB_state =(CAPSWITCH)switchB_state;
				cap_switchB_count--;
				if(cap_switchB_count < 0){
					gl_error("Unusual control of the capacitor has resulted in a negative switch change count on phase B.");
					return TS_INVALID;
				}
				switchB_changed = 0;
			} else if(prev_switchB_state != switchB_state){
				prev_switchB_state = (CAPSWITCH)switchB_state;
			}
		}
		if(switchB_changed == 2){
			prev_switchB_state = (CAPSWITCH)switchB_state;
		}
		if(switchC_changed == 0){
			if(prev_switchC_state != switchC_state){
				prev_switchC_state = (CAPSWITCH)switchC_state;
				cap_switchC_count++;
				switchC_changed = 1;
			}
		}
		if(switchC_changed == 1){
			if(init_switchC_state == switchC_state){
				prev_switchC_state = (CAPSWITCH)switchC_state;
				cap_switchC_count--;
				if(cap_switchC_count < 0){
					gl_error("Unusual control of the capacitor has resulted in a negative switch change count on phase C.");
					return TS_INVALID;
				}
				switchC_changed = 0;
			} else if(prev_switchC_state != switchC_state){
				prev_switchC_state = (CAPSWITCH)switchC_state;
			}
		}
		if(switchC_changed == 2){
			prev_switchC_state = (CAPSWITCH)switchC_state;
		}
	}

	//Perform sync update, just in case
	result=node::postsync(t0);

	if ((control==VAR) || (control==VARVOLT))	//Grab the power values from remote link
	{
		READLOCK_OBJECT(OBJECTHDR(RLink));

		//Force the link to do an update (will be ignored first run anyways (zero))
		RLink->calculate_power();

		//Takes all measurements from input side of link (output can not quite be up to date)
		VArVals[0] = RLink->indiv_power_in[0].Im();
		VArVals[1] = RLink->indiv_power_in[1].Im();
		VArVals[2] = RLink->indiv_power_in[2].Im();
		
		READUNLOCK_OBJECT(OBJECTHDR(RLink));

		//If NR, force us to do a reiteration if something changed - FBS handles this just due to the nature of how it solves
		//Basically copied out of actual logic above
		if (solver_method == SM_NR)
		{
			//Initialize
			cap_A_test_state = switchA_state_Req_Next;
			cap_B_test_state = switchB_state_Req_Next;
			cap_C_test_state = switchC_state_Req_Next;

			//Power values only calculated as L-N (L-L not possible or really feasible)
			if ((pt_phase & PHASE_A) == PHASE_A)
			{
				if (VAr_set_low >= VArVals[0])
					cap_A_test_state=OPEN;
				else if (VAr_set_high <= VArVals[0])
					cap_A_test_state=CLOSED;
				else;
			}

			//Calculate them for phase B
			if ((pt_phase & PHASE_B) == PHASE_B)
			{
				if (VAr_set_low >= VArVals[1])
					cap_B_test_state=OPEN;
				else if (VAr_set_high <= VArVals[1])
					cap_B_test_state=CLOSED;
				else;
			}

			//Calculate them for phase C
			if ((pt_phase & PHASE_C) == PHASE_C)
			{
				if (VAr_set_low >= VArVals[2])
					cap_C_test_state=OPEN;
				else if (VAr_set_high <= VArVals[2])
					cap_C_test_state=CLOSED;
				else;
			}

			//Check and see if anything changed.  If so, reiterate.
			if ((Iteration_Toggle == false) && (switchA_state_Req_Next != cap_A_test_state))
				result = t0;

			if ((Iteration_Toggle == false) && (switchB_state_Req_Next != cap_B_test_state))
				result = t0;

			if ((Iteration_Toggle == false) && (switchC_state_Req_Next != cap_C_test_state))
				result = t0;

			//See if we've already requested a reiteration
			if ((result != t0) && (control == VARVOLT))
			{
				//Update voltage values
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
						LOCK_OBJECT(OBJECTHDR(RNode));
						VoltVals[0] = RNode->voltage[0];
						VoltVals[1] = RNode->voltage[1];
						VoltVals[2] = RNode->voltage[2];
						UNLOCK_OBJECT(OBJECTHDR(RNode));

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
						LOCK_OBJECT(OBJECTHDR(RNode));
						VoltVals[0] = RNode->voltaged[0];
						VoltVals[1] = RNode->voltaged[1];
						VoltVals[2] = RNode->voltaged[2];
						UNLOCK_OBJECT(OBJECTHDR(RNode));
					}
				}

				//VAR-VOLT portion of control scheme
				//Check all voltages.  If exceed limit, force capacitor open instead of closed (regardless of how it was)
				bool lockout_state_change[3];
				lockout_state_change[0] = lockout_state_change[1] = lockout_state_change[2] = false;

				if ((pt_phase & PHASE_D) != (PHASE_D))// Line to Neutral connections
				{
					if (lockout_time_left_A <= 0)	//Not in lockout period
					{
						if (((pt_phase & PHASE_A) == PHASE_A) && (VoltVals[0].Mag() >= voltage_set_high))
						{
							cap_A_test_state=OPEN;
							lockout_state_change[0] = true;
						}
					}
					else	//In lockout period
					{
						cap_A_test_state=OPEN;
					}

					if (lockout_time_left_B <= 0)	//Not in lockout period
					{
						if (((pt_phase & PHASE_B) == PHASE_B) && (VoltVals[1].Mag() >= voltage_set_high))
						{
							cap_B_test_state=OPEN;
							lockout_state_change[1] = true;
						}
					}
					else	//In lockout period
					{
						cap_B_test_state=OPEN;
					}

					if (lockout_time_left_C <= 0)	//Not in lockout period
					{
						if (((pt_phase & PHASE_C) == PHASE_C) && (VoltVals[2].Mag() >= voltage_set_high))
						{
							cap_C_test_state=OPEN;
							lockout_state_change[2] = true;
						}
					}
					else	//In lockout period
					{
						cap_C_test_state=OPEN;
					}
				}
				else // Line to Line connections
				{
					if (lockout_time_left_A <= 0)	//Not in lockout period
					{
						if (((pt_phase & (PHASE_A | PHASE_B)) == (PHASE_A | PHASE_B)) && (VoltVals[0].Mag() >= voltage_set_high))
						{
							cap_A_test_state=OPEN;
							lockout_state_change[0] = true;
						}
					}
					else	//In lockout period
					{
						cap_A_test_state=OPEN;
					}

					if (lockout_time_left_B <= 0)	//Not in lockout period
					{
						if (((pt_phase & (PHASE_B | PHASE_C)) == (PHASE_B | PHASE_C)) && (VoltVals[1].Mag() >= voltage_set_high))
						{
							cap_B_test_state=OPEN;
							lockout_state_change[1] = true;
						}
					}
					else	//In lockout period
					{
						cap_B_test_state=OPEN;
					}

					if (lockout_time_left_C <= 0)	//Not in lockout period
					{
						if (((pt_phase & (PHASE_C | PHASE_A)) == (PHASE_C | PHASE_A)) && (VoltVals[2].Mag() >= voltage_set_high))
						{
							cap_C_test_state=OPEN;
							lockout_state_change[2] = true;
						}
					}
					else	//In lockout period
					{
						cap_C_test_state=OPEN;
					}
				}

				//Check VARVOLT updates to see if a reiteration is needed
				if ((Iteration_Toggle == false) && (switchA_state_Req_Next != cap_A_test_state))
					result = t0;

				if ((Iteration_Toggle == false) && (switchB_state_Req_Next != cap_B_test_state))
					result = t0;

				if ((Iteration_Toggle == false) && (switchC_state_Req_Next != cap_C_test_state))
					result = t0;

			}//end VARVOLT mode extra logic
		}//End NR solver logic
	}
	else if (control==CURRENT)	//Grab the current injection values
	{
		READLOCK_OBJECT(OBJECTHDR(RLink));

		//Pull off the output currents
		CurrentVals[0]=RLink->current_in[0].Mag();
		CurrentVals[1]=RLink->current_in[1].Mag();
		CurrentVals[2]=RLink->current_in[2].Mag();
		
		READUNLOCK_OBJECT(OBJECTHDR(RLink));

		//If NR, check logic to see if we need to request another pass
		if (solver_method == SM_NR)
		{
			//Initialize
			cap_A_test_state = switchA_state_Req_Next;
			cap_B_test_state = switchB_state_Req_Next;
			cap_C_test_state = switchC_state_Req_Next;

			// I - only line currents (just consider the phase as it was, no conversions)
			if ((pt_phase & PHASE_A) == PHASE_A)
			{
				if (current_set_low >= CurrentVals[0])
					cap_A_test_state=OPEN;
				else if (current_set_high <= CurrentVals[0])
					cap_A_test_state=CLOSED;
				else;
			}

			if ((pt_phase & PHASE_B) == PHASE_B)
			{
				if (current_set_low >= CurrentVals[1])
					cap_B_test_state=OPEN;
				else if (current_set_high <= CurrentVals[1])
					cap_B_test_state=CLOSED;
				else;
			}

			if ((pt_phase & PHASE_C) == PHASE_C)
			{
				if (current_set_low >= CurrentVals[2])
					cap_C_test_state=OPEN;
				else if (current_set_high <= CurrentVals[2])
					cap_C_test_state=CLOSED;
				else;
			}

			//Check capacitor changes - see if a reiteration is needed
			if ((Iteration_Toggle == false) && (switchA_state_Req_Next != cap_A_test_state))
				result = t0;

			if ((Iteration_Toggle == false) && (switchB_state_Req_Next != cap_B_test_state))
				result = t0;

			if ((Iteration_Toggle == false) && (switchC_state_Req_Next != cap_C_test_state))
				result = t0;
		}
		//Defaulted else - FBS seems to behave okay (forces an iteration anyways)
	}
	else if ((control==VOLT) && (solver_method == SM_NR))
	{
		//Initialize
		cap_A_test_state = switchA_state_Req_Next;
		cap_B_test_state = switchB_state_Req_Next;
		cap_C_test_state = switchC_state_Req_Next;

		//Update voltage values
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
				LOCK_OBJECT(OBJECTHDR(RNode));
				VoltVals[0] = RNode->voltage[0];
				VoltVals[1] = RNode->voltage[1];
				VoltVals[2] = RNode->voltage[2];
				UNLOCK_OBJECT(OBJECTHDR(RNode));

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
				LOCK_OBJECT(OBJECTHDR(RNode));
				VoltVals[0] = RNode->voltaged[0];
				VoltVals[1] = RNode->voltaged[1];
				VoltVals[2] = RNode->voltaged[2];
				UNLOCK_OBJECT(OBJECTHDR(RNode));
			}
		}

		if ((pt_phase & PHASE_D) != (PHASE_D))// Line to Neutral connections
		{
			if ((pt_phase & PHASE_A) == PHASE_A)
			{
				if (voltage_set_low >= VoltVals[0].Mag())
					cap_A_test_state=CLOSED;
				else if (voltage_set_high <= VoltVals[0].Mag())
					cap_A_test_state=OPEN;
				else;
			}
				
			if ((pt_phase & PHASE_B) == PHASE_B)
			{
				if (voltage_set_low >= VoltVals[1].Mag())
					cap_B_test_state=CLOSED;
				else if (voltage_set_high <= VoltVals[1].Mag())
					cap_B_test_state=OPEN;
				else;
			}

			if ((pt_phase & PHASE_C) == PHASE_C)
			{
				if (voltage_set_low >= VoltVals[2].Mag())
					cap_C_test_state=CLOSED;
				else if (voltage_set_high <= VoltVals[2].Mag())
					cap_C_test_state=OPEN;
				else;
			}
		}
		else // Line to Line connections
		{
			if ((pt_phase & (PHASE_A | PHASE_B)) == (PHASE_A | PHASE_B))
			{
				if (voltage_set_low >= VoltVals[0].Mag())	//VoltVals handled above to use L-L voltages instead
					cap_A_test_state=CLOSED;				//switchA assigned to AB connection (delta-connection in loads)
				else if (voltage_set_high <= VoltVals[0].Mag())
					cap_A_test_state=OPEN;
				else;
			}

			if ((pt_phase & (PHASE_B | PHASE_C)) == (PHASE_B | PHASE_C))
			{
				if (voltage_set_low >= VoltVals[1].Mag())	//VoltVals handled above to use L-L voltages instead
					cap_B_test_state=CLOSED;				//switchB assigned to BC connection (delta-connection in loads)
				else if (voltage_set_high <= VoltVals[1].Mag())
					cap_B_test_state=OPEN;
				else;
			}

			if ((pt_phase & (PHASE_C | PHASE_A)) == (PHASE_C | PHASE_A))
			{
				if (voltage_set_low >= VoltVals[2].Mag())	//VoltVals handled above to use L-L voltages instead
					cap_C_test_state=CLOSED;				//switchC assigned to CA connection (delta-connection in loads)
				else if (voltage_set_high <= VoltVals[2].Mag())
					cap_C_test_state=OPEN;
				else;
			}
		}

		//Check to see if the voltage update needs a reiteration
		if ((Iteration_Toggle == false) && (switchA_state_Req_Next != cap_A_test_state))
			result = t0;

		if ((Iteration_Toggle == false) && (switchB_state_Req_Next != cap_B_test_state))
			result = t0;

		if ((Iteration_Toggle == false) && (switchC_state_Req_Next != cap_C_test_state))
			result = t0;

	}	//End VOLT mode under NR extra logic

	return result;
}

//Function to toggle capacitor bank status
//Used by VVC to bypass private variable restrictions, as well as capacitor internal delays
void capacitor::toggle_bank_status(bool des_status){

	if (des_status==true)	//We want to go to a closed state
	{
		if ((phases_connected & PHASE_A)  == PHASE_A)
		{
			switchA_state = switchA_state_Next = switchA_state_Prev = CLOSED;
		}

		if ((phases_connected & PHASE_B)  == PHASE_B)
		{
			switchB_state = switchB_state_Next = switchB_state_Prev = CLOSED;
		}

		if ((phases_connected & PHASE_C)  == PHASE_C)
		{
			switchC_state = switchC_state_Next = switchC_state_Prev = CLOSED;
		}
	}
	else	//Open us up
	{
		if ((phases_connected & PHASE_A)  == PHASE_A)
		{
			switchA_state = switchA_state_Next = switchA_state_Prev = OPEN;
		}

		if ((phases_connected & PHASE_B)  == PHASE_B)
		{
			switchB_state = switchB_state_Next = switchB_state_Prev = OPEN;
		}

		if ((phases_connected & PHASE_C)  == PHASE_C)
		{
			switchC_state = switchC_state_Next = switchC_state_Prev = OPEN;
		}
	}

	//Eliminate any delay imposed
	time_to_change = dwell_time_left = 0;
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
		else
			return 0;
	}
	CREATE_CATCHALL(capacitor);
}



/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_capacitor(OBJECT *obj)
{
	try {
		capacitor *my = OBJECTDATA(obj,capacitor);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(capacitor);
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
	try {
		capacitor *pObj = OBJECTDATA(obj,capacitor);
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
	}
	SYNC_CATCHALL(capacitor);
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
