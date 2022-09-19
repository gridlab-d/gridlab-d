/** $Id: switch_object.cpp,v 1.6 2008/02/06 19:15:26 natet Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file switch_object.cpp
	@addtogroup powerflow switch_object
	@ingroup powerflow

	Implements a switch object.

 @{
 **/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "switch_object.h"

//////////////////////////////////////////////////////////////////////////
// switch_object CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* switch_object::oclass = nullptr;
CLASS* switch_object::pclass = nullptr;

switch_object::switch_object(MODULE *mod) : link_object(mod)
{
	if(oclass == nullptr)
	{
		pclass = link_object::oclass;

		oclass = gl_register_class(mod,"switch",sizeof(switch_object),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass== nullptr)
			throw "unable to register class switch_object";
		else
			oclass->trl = TRL_QUALIFIED;

        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
			PT_enumeration, "phase_A_state", PADDR(phase_A_state),PT_DESCRIPTION,"Defines the current state of the phase A switch",
				PT_KEYWORD, "OPEN", (enumeration)SW_OPEN,
				PT_KEYWORD, "CLOSED", (enumeration)SW_CLOSED,
			PT_enumeration, "phase_B_state", PADDR(phase_B_state),PT_DESCRIPTION,"Defines the current state of the phase B switch",
				PT_KEYWORD, "OPEN", (enumeration)SW_OPEN,
				PT_KEYWORD, "CLOSED", (enumeration)SW_CLOSED,
			PT_enumeration, "phase_C_state", PADDR(phase_C_state),PT_DESCRIPTION,"Defines the current state of the phase C switch",
				PT_KEYWORD, "OPEN", (enumeration)SW_OPEN,
				PT_KEYWORD, "CLOSED", (enumeration)SW_CLOSED,
			PT_enumeration, "operating_mode", PADDR(switch_banked_mode),PT_DESCRIPTION,"Defines whether the switch operates in a banked or per-phase control mode",
				PT_KEYWORD, "INDIVIDUAL", (enumeration)INDIVIDUAL_SW,
				PT_KEYWORD, "BANKED", (enumeration)BANKED_SW,
			PT_complex, "switch_impedance[Ohm]",PADDR(switch_impedance_value), PT_DESCRIPTION,"Impedance value of the swtich when closed",
			PT_double, "switch_resistance[Ohm]",PADDR(switch_impedance_value.Re()), PT_DESCRIPTION,"Resistance portion of impedance value of the switch when it is closed.",
			PT_double, "switch_reactance[Ohm]", PADDR(switch_impedance_value.Im()), PT_DESCRIPTION, "Reactance portion of impedance value of the switch when it is closed.",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

			if (gl_publish_function(oclass,"change_switch_state",(FUNCTIONADDR)change_switch_state)==nullptr)
				GL_THROW("Unable to publish switch state change function");
			if (gl_publish_function(oclass,"reliability_operation",(FUNCTIONADDR)reliability_operation)==nullptr)
				GL_THROW("Unable to publish switch reliability operation function");
			if (gl_publish_function(oclass,	"create_fault", (FUNCTIONADDR)create_fault_switch)==nullptr)
				GL_THROW("Unable to publish fault creation function");
			if (gl_publish_function(oclass,	"fix_fault", (FUNCTIONADDR)fix_fault_switch)==nullptr)
				GL_THROW("Unable to publish fault restoration function");
            if (gl_publish_function(oclass,	"clear_fault", (FUNCTIONADDR)clear_fault_switch)==nullptr)
                GL_THROW("Unable to publish fault clearing function");
			if (gl_publish_function(oclass,	"change_switch_faults", (FUNCTIONADDR)switch_fault_updates)==nullptr)
				GL_THROW("Unable to publish switch fault correction function");
			if (gl_publish_function(oclass,	"change_switch_state_toggle", (FUNCTIONADDR)change_switch_state_toggle)==nullptr)
				GL_THROW("Unable to publish switch toggle function");

			//Publish deltamode functions
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_switch)==nullptr)
				GL_THROW("Unable to publish switch deltamode function");

			//Publish restoration-related function (current update)
			if (gl_publish_function(oclass,	"update_power_pwr_object", (FUNCTIONADDR)updatepowercalc_link)==nullptr)
				GL_THROW("Unable to publish switch external power calculation function");
			if (gl_publish_function(oclass,	"check_limits_pwr_object", (FUNCTIONADDR)calculate_overlimit_link)==nullptr)
				GL_THROW("Unable to publish switch external power limit calculation function");
			if (gl_publish_function(oclass,	"perform_current_calculation_pwr_link", (FUNCTIONADDR)currentcalculation_link)==nullptr)
				GL_THROW("Unable to publish switch external current calculation function");

			//Other
			if (gl_publish_function(oclass, "pwr_object_kmldata", (FUNCTIONADDR)switch_object_kmldata) == nullptr)
				GL_THROW("Unable to publish switch kmldata function");
    }
}

int switch_object::isa(char *classname)
{
	return strcmp(classname,"switch")==0 || link_object::isa(classname);
}

int switch_object::create()
{
	int result = link_object::create();

	prev_full_status = 0x00;		//Flag as all open initially
	switch_banked_mode = BANKED_SW;	//Assume operates in banked mode normally
	phase_A_state = SW_CLOSED;			//All switches closed by default
	phase_B_state = SW_CLOSED;
	phase_C_state = SW_CLOSED;

	prev_SW_time = 0;

	phased_switch_status = 0x00;	//Reset variable
	faulted_switch_phases = 0x00;	//No faults at onset
	prefault_banked = false;		//Condition of the switch prior to a fault - individual mode has to be enacted for reliability

	event_schedule = nullptr;
	eventgen_obj = nullptr;
	fault_handle_call = nullptr;
	local_switching = false;
	event_schedule_map_attempt = false;	//Haven't tried to map yet
	
	//Simplification variables
	switch_impedance_value = gld::complex(-1.0,-1.0);
	switch_admittance_value = gld::complex(0.0,0.0);

	return result;
}

int switch_object::init(OBJECT *parent)
{
	double phase_total, switch_total;
	char indexa, indexb;
	set phase_from, phase_to;

	OBJECT *obj = OBJECTHDR(this);

	//Special flag moved to be universal for all solvers - mainly so phase checks catch it now
	SpecialLnk = SWITCH;

	int result = link_object::init(parent);

	//Check for deferred
	if (result == 2)
		return 2;	//Return the deferment - no sense doing everything else!

	//Grab phase information
	phase_from = node_phase_information(from);
	phase_to = node_phase_information(to);

	//Just blanket check them - none would be valid
	if (((phase_from | phase_to | phases) & PHASE_S) == PHASE_S)
	{
		GL_THROW("switch:%d - %s - triplex/S-phase connections unsupported at this time",get_id(),get_name());
		/*  TROUBLESHOOT
		A switch was placed connected to a triplex/split-phase node, or was flagged as triplex/split-phase itself.  This is not
		supported at this time - all switching must be done on the standard three-phase portion of the system.
		*/
	}

	//secondary init stuff - should have been done, but we'll be safe
	//Basically zero everything
	if (solver_method==SM_FBS)
	{
		for (indexa=0; indexa<3; indexa++)
		{
			for (indexb=0; indexb<3; indexb++)
			{
				//These have to be zeroed (nature of switch - mostly to zero, but also are zero-impedance in FBS)
				b_mat[indexa][indexb] = 0.0;
				c_mat[indexa][indexb] = 0.0;
				B_mat[indexa][indexb] = 0.0;

				//Paranoid initialization
				a_mat[indexa][indexb] = 0.0;
				A_mat[indexa][indexb] = 0.0;
				d_mat[indexa][indexb] = 0.0;
			}
		}

		//Generic warning for switches
		gl_warning("switch objects may not behave properly under FBS!");
		/*  TROUBLESHOOT
		Due to the nature of the forward-backward sweep algorithm, and open
		switch may induce the desired behavior on the system.  If open, it should prevent
		current flow and set the "to" end voltage to zero.  However, this may cause problems
		in many powerflow conditions or not properly solve.  Consider using the
		Newton-Raphson solver in conjunction with the reliability package, or rearrange your system.
		*/
	}
	else
	{
		//Flagged it as special (we'll forgo inversion processes on this)

		//Index out "voltage_ratio" matrix (same as A_mat, basically)
		//and From_Y - just because
		for (indexa=0; indexa<3; indexa++)
		{
			for (indexb=0; indexb<3; indexb++)
			{
				From_Y[indexa][indexb] = 0.0;
				a_mat[indexa][indexb] = 0.0;
			}
		}
	}//End NR mode

	//TODO: Reconcile the switch_resistance with switch_impedance

	//See if we're NR - if so, populate the admittance value
	if (solver_method == SM_NR)
	{
		//See if anything was set - otherwise, use the default value - resistance
		if(switch_impedance_value.Re() < 0.0)
		{
			switch_impedance_value.SetReal(default_resistance);
		}

		//Do the same for reactance - be consistent
		if (switch_impedance_value.Im() < 0.0)
		{
			switch_impedance_value.SetImag(-1.0*default_resistance);
		}

		//Now check values again - resistance
		if (switch_impedance_value.Re() == 0.0)
		{
			gl_warning("Switch:%s switch_resistance has been set to zero. This will result singular matrix. Setting to the global default.",obj->name);
			/*  TROUBLESHOOT
			Under Newton-Raphson solution method the impedance matrix cannot be a singular matrix for the inversion process.
			Change the value of switch_resistance to something small but larger that zero.
			*/

			switch_impedance_value.SetReal(default_resistance);
		}

		//React
		if (switch_impedance_value.Im() == 0.0)
		{
			gl_warning("Switch:%s switch_reactance has been set to zero. This will result singular matrix. Setting to the global default.",obj->name);
			/*  TROUBLESHOOT
			Under Newton-Raphson solution method the impedance matrix cannot be a singular matrix for the inversion process.
			Change the value of switch_reactance to something small but larger that zero.
			*/

			switch_impedance_value.SetImag(default_resistance);
		}

		//Calculate the admittance
		switch_admittance_value = gld::complex(1.0,0.0)/switch_impedance_value;
	}
	else
	{
		//FBS defaults to lossless - just how it was set up
		//See if anything was set - otherwise, use the default value - resistance
		if(switch_impedance_value.Re() < 0.0)
		{
			switch_impedance_value.SetReal(0.0);
		}

		//Do the same for reactance - be consistent
		if (switch_impedance_value.Im() < 0.0)
		{
			switch_impedance_value.SetImag(0.0);
		}
	}

	//Make adjustments based on BANKED vs. INDIVIDUAL - replication of later code
	if (switch_banked_mode == BANKED_SW)
	{
		//Set reliability-related flag
		prefault_banked = true;

		//Banked mode is operated by majority rule.  If the majority are a different state, all become that state
		phase_total = (double)(has_phase(PHASE_A) + has_phase(PHASE_B) + has_phase(PHASE_C));	//See how many switches we have

		switch_total = 0.0;
		if (has_phase(PHASE_A) && (phase_A_state == SW_CLOSED))
			switch_total += 1.0;

		if (has_phase(PHASE_B) && (phase_B_state == SW_CLOSED))
			switch_total += 1.0;

		if (has_phase(PHASE_C) && (phase_C_state == SW_CLOSED))
			switch_total += 1.0;

		switch_total /= phase_total;

		if (switch_total > 0.5)	//In two switches, a stalemate occurs.  We'll consider this a "maintain status quo" state
		{
			//Initial check, make sure stays open
			if (status == LS_OPEN)
			{
				phase_A_state = phase_B_state = phase_C_state = SW_CLOSED;	//Set all to open - phase checks will sort it out
			}
			else
			{
				phase_A_state = phase_B_state = phase_C_state = SW_CLOSED;	//Set all to closed - phase checks will sort it out
			}
		}
		else	//Minority or stalemate
		{
			if (switch_total == 0.5)	//Stalemate
			{
				if (status == LS_OPEN)	//These check assume phase_X_state will be manipulated, not status
				{
					phase_A_state = phase_B_state = phase_C_state = SW_OPEN;
				}
				else	//Closed
				{
					phase_A_state = phase_B_state = phase_C_state = SW_CLOSED;
				}
			}
			else	//Not stalemate - open all
			{
				status = LS_OPEN;
				phase_A_state = phase_B_state = phase_C_state = SW_OPEN;	//Set all to open - phase checks will sort it out
			}
		}

		if (status==LS_OPEN)
		{
			if (solver_method == SM_NR)
			{
				From_Y[0][0] = gld::complex(0.0,0.0);
				From_Y[1][1] = gld::complex(0.0,0.0);
				From_Y[2][2] = gld::complex(0.0,0.0);

				//Impedance, just because (used in some secondary calculations)
				b_mat[0][0] = gld::complex(0.0,0.0);
				b_mat[1][1] = gld::complex(0.0,0.0);
				b_mat[2][2] = gld::complex(0.0,0.0);

				//Confirm a_mat is zerod too, just for portability
				a_mat[0][0] = gld::complex(0.0,0.0);
				a_mat[1][1] = gld::complex(0.0,0.0);
				a_mat[2][2] = gld::complex(0.0,0.0);

				d_mat[0][0] = gld::complex(0.0,0.0);
				d_mat[1][1] = gld::complex(0.0,0.0);
				d_mat[2][2] = gld::complex(0.0,0.0);
			}
			else	//Assumed FBS (GS not considered)
			{
				A_mat[0][0] = gld::complex(0.0,0.0);
				A_mat[1][1] = gld::complex(0.0,0.0);
				A_mat[2][2] = gld::complex(0.0,0.0);

				d_mat[0][0] = gld::complex(0.0,0.0);
				d_mat[1][1] = gld::complex(0.0,0.0);
				d_mat[2][2] = gld::complex(0.0,0.0);

				B_mat[0][0] = gld::complex(0.0,0.0);
				B_mat[1][1] = gld::complex(0.0,0.0);
				B_mat[2][2] = gld::complex(0.0,0.0);

				b_mat[0][0] = gld::complex(0.0,0.0);
				b_mat[1][1] = gld::complex(0.0,0.0);
				b_mat[2][2] = gld::complex(0.0,0.0);
			}

			phase_A_state = phase_B_state = phase_C_state = SW_OPEN;	//All open
			prev_full_status = 0x00;								//Confirm here
		}
		else
		{
			if (has_phase(PHASE_A))
			{
				if (solver_method == SM_NR)
				{
					From_Y[0][0] = switch_admittance_value;
					b_mat[0][0] = switch_impedance_value;
					a_mat[0][0] = 1.0;
					d_mat[0][0] = 1.0;
				}
				else
				{
					A_mat[0][0] = 1.0;
					d_mat[0][0] = 1.0;
					B_mat[0][0] = switch_impedance_value;
					b_mat[0][0] = switch_impedance_value;
				}

				phase_A_state = SW_CLOSED;							//Flag as closed
				prev_full_status |= 0x04;
			}

			if (has_phase(PHASE_B))
			{
				if (solver_method == SM_NR)
				{
					From_Y[1][1] = switch_admittance_value;
					b_mat[1][1] = switch_impedance_value;
					a_mat[1][1] = 1.0;
					d_mat[1][1] = 1.0;
				}
				else
				{
					A_mat[1][1] = 1.0;
					d_mat[1][1] = 1.0;
					B_mat[1][1] = switch_impedance_value;
					b_mat[1][1] = switch_impedance_value;
				}

				phase_B_state = SW_CLOSED;							//Flag as closed
				prev_full_status |= 0x02;
			}

			if (has_phase(PHASE_C))
			{
				if (solver_method == SM_NR)
				{
					From_Y[2][2] = switch_admittance_value;
					b_mat[2][2] = switch_impedance_value;
					a_mat[2][2] = 1.0;
					d_mat[2][2] = 1.0;
				}
				else
				{
					A_mat[2][2] = 1.0;
					d_mat[2][2] = 1.0;
					B_mat[2][2] = switch_impedance_value;
					b_mat[2][2] = switch_impedance_value;
				}

				phase_C_state = SW_CLOSED;							//Flag as closed
				prev_full_status |= 0x01;
			}
		}
	}//End banked mode
	else	//Individual mode
	{
		if (status==LS_OPEN)	//Take this as all should be open
		{
			if (solver_method == SM_NR)
			{
				From_Y[0][0] = gld::complex(0.0,0.0);
				From_Y[1][1] = gld::complex(0.0,0.0);
				From_Y[2][2] = gld::complex(0.0,0.0);

				//Impedance, just because
				b_mat[0][0] = gld::complex(0.0,0.0);
				b_mat[1][1] = gld::complex(0.0,0.0);
				b_mat[2][2] = gld::complex(0.0,0.0);

				//Ensure voltage ratio set too (technically not needed)
				a_mat[0][0] = 0.0;
				a_mat[1][1] = 0.0;
				a_mat[2][2] = 0.0;

				d_mat[0][0] = 0.0;
				d_mat[1][1] = 0.0;
				d_mat[2][2] = 0.0;
			}
			else	//Assumed FBS (GS not considered)
			{
				A_mat[0][0] = gld::complex(0.0,0.0);
				A_mat[1][1] = gld::complex(0.0,0.0);
				A_mat[2][2] = gld::complex(0.0,0.0);

				d_mat[0][0] = gld::complex(0.0,0.0);
				d_mat[1][1] = gld::complex(0.0,0.0);
				d_mat[2][2] = gld::complex(0.0,0.0);

				B_mat[0][0] = gld::complex(0.0,0.0);
				B_mat[1][1] = gld::complex(0.0,0.0);
				B_mat[2][2] = gld::complex(0.0,0.0);

				b_mat[0][0] = gld::complex(0.0,0.0);
				b_mat[1][1] = gld::complex(0.0,0.0);
				b_mat[2][2] = gld::complex(0.0,0.0);
			}

			phase_A_state = phase_B_state = phase_C_state = SW_OPEN;	//All open
			prev_full_status = 0x00;								//Confirm here
		}
		else	//LS_CLOSED - handle individually
		{
			if (has_phase(PHASE_A))
			{
				if (phase_A_state == SW_CLOSED)
				{
					if (solver_method == SM_NR)
					{
						From_Y[0][0] = switch_admittance_value;
						b_mat[0][0] = switch_impedance_value;
						a_mat[0][0] = 1.0;
						d_mat[0][0] = 1.0;
					}
					else
					{
						A_mat[0][0] = 1.0;
						d_mat[0][0] = 1.0;
						B_mat[0][0] = switch_impedance_value;
						b_mat[0][0] = switch_impedance_value;
					}
					prev_full_status |= 0x04;
				}
				else	//Must be open
				{
					if (solver_method == SM_NR)
					{
						From_Y[0][0] = gld::complex(0.0,0.0);
						b_mat[0][0] = gld::complex(0.0,0.0);
						a_mat[0][0] = gld::complex(0.0,0.0);
						d_mat[0][0] = gld::complex(0.0,0.0);
					}
					else
					{
						A_mat[0][0] = gld::complex(0.0,0.0);
						d_mat[0][0] = gld::complex(0.0,0.0);
						B_mat[0][0] = gld::complex(0.0,0.0);
						b_mat[0][0] = gld::complex(0.0,0.0);
					}
					prev_full_status &=0xFB;
				}
			}

			if (has_phase(PHASE_B))
			{
				if (phase_B_state == SW_CLOSED)
				{
					if (solver_method == SM_NR)
					{
						From_Y[1][1] = switch_admittance_value;
						b_mat[1][1] = switch_impedance_value;
						a_mat[1][1] = 1.0;
						d_mat[1][1] = 1.0;
					}
					else
					{
						A_mat[1][1] = 1.0;
						d_mat[1][1] = 1.0;
						B_mat[1][1] = switch_impedance_value;
						b_mat[1][1] = switch_impedance_value;
					}
					prev_full_status |= 0x02;
				}
				else	//Must be open
				{
					if (solver_method == SM_NR)
					{
						From_Y[1][1] = gld::complex(0.0,0.0);
						b_mat[1][1] = gld::complex(0.0,0.0);
						a_mat[1][1] = gld::complex(0.0,0.0);
						d_mat[1][1] = gld::complex(0.0,0.0);
					}
					else
					{
						A_mat[1][1] = gld::complex(0.0,0.0);
						d_mat[1][1] = gld::complex(0.0,0.0);
						B_mat[1][1] = gld::complex(0.0,0.0);
						b_mat[1][1] = gld::complex(0.0,0.0);
					}
					prev_full_status &=0xFD;
				}
			}

			if (has_phase(PHASE_C))
			{
				if (phase_C_state == SW_CLOSED)
				{
					if (solver_method == SM_NR)
					{
						From_Y[2][2] = switch_admittance_value;
						b_mat[2][2] = switch_impedance_value;
						a_mat[2][2] = 1.0;
						d_mat[2][2] = 1.0;
					}
					else
					{
						A_mat[2][2] = 1.0;
						d_mat[2][2] = 1.0;
						B_mat[2][2] = switch_impedance_value;
						b_mat[2][2] = switch_impedance_value;
					}
					prev_full_status |= 0x01;
				}
				else	//Must be open
				{
					if (solver_method == SM_NR)
					{
						From_Y[2][2] = gld::complex(0.0,0.0);
						b_mat[2][2] = gld::complex(0.0,0.0);
						a_mat[2][2] = gld::complex(0.0,0.0);
						d_mat[2][2] = gld::complex(0.0,0.0);
					}
					else
					{
						A_mat[2][2] = gld::complex(0.0,0.0);
						d_mat[2][2] = gld::complex(0.0,0.0);
						B_mat[2][2] = gld::complex(0.0,0.0);
						b_mat[2][2] = gld::complex(0.0,0.0);
					}
					prev_full_status &=0xFE;
				}
			}
		}
	}//end individual

	//Store switch status - will get updated as things change later
	phased_switch_status = prev_full_status;
	prev_status = status;

	return result;
}

//Functionalized switch sync call -- before link call -- for deltamode functionality
void switch_object::BOTH_switch_sync_pre(unsigned char *work_phases_pre, unsigned char *work_phases_post)
{
	if ((solver_method == SM_NR) && (event_schedule != nullptr))	//NR-reliability-related stuff
	{
		if (!meshed_fault_checking_enabled)
		{
			//Store our phases going in
			*work_phases_pre = NR_branchdata[NR_branch_reference].phases & 0x07;
		
			//Call syncing function
			*work_phases_post = switch_expected_sync_function();

			//Store our phases going out
			*work_phases_post &= 0x07;
		}
		else	//Meshed checking, simplified operations
		{
			//Call syncing function
			switch_sync_function();
		}
	}
	else    //Normal execution
	{
		if (solver_method == SM_NR && local_switching) {
			prev_full_status = phased_switch_status;
		}
		//Call syncing function
		switch_sync_function();

		//Set phases the same
		*work_phases_pre = 0x00;
		*work_phases_post = 0x00;
	}
	gl_verbose ("  BOTH_switch_sync_pre leaving:%s:%s", work_phases_pre, work_phases_post);
}

//Functionalized switch sync call -- after link call -- for deltamode functionality
void switch_object::NR_switch_sync_post(unsigned char *work_phases_pre, unsigned char *work_phases_post, OBJECT *obj, TIMESTAMP *t0, TIMESTAMP *t2)
{
	unsigned char work_phases, work_phases_closed; //work_phases_pre, work_phases_post, work_phases_closed;
	char fault_val[9];
	int working_protect_phases[3];
	int result_val, impl_fault, indexval;
	bool fault_mode;
	TIMESTAMP temp_time;

	gl_verbose ("  NR_switch_sync_post:%s:%s", work_phases_pre, work_phases_post);
	//Overall encompassing check -- meshed handled differently
	if (!meshed_fault_checking_enabled)
	{
		//See if we're in the proper cycle - NR only for now
		if (*work_phases_pre != *work_phases_post)
		{
			//Initialize the variable
			working_protect_phases[0] = 0;
			working_protect_phases[1] = 0;
			working_protect_phases[2] = 0;

			//Find out what changed
			work_phases = (*work_phases_pre ^ *work_phases_post) & 0x07;

			//See if this transition is a "fault-open" or a "fault-close"
			work_phases_closed = work_phases & *work_phases_post;

			//See how it looks
			if (work_phases_closed == work_phases)	//It's a close
			{
				fault_mode = true;
				//work_phases = (~work_phases) & 0x07;
			}
			else	//It's an open
			{
				fault_mode = false;
				//Work phases is already in the proper format
			}

			//Set up fault type
			fault_val[0] = 'S';
			fault_val[1] = 'W';
			fault_val[2] = '-';

			//Default fault - none - will cause a failure if not caught
			impl_fault = -1;

			//Determine who opened and store the time
			switch (work_phases)
			{
			case 0x00:	//No switches opened !??
				GL_THROW("switch:%s supposedly opened, but doesn't register the right phases",obj->name);
				/*  TROUBLESHOOT
				A switch reported changing to an open status.  However, it did not appear to fully propogate this
				condition.  Please try again.  If the error persists, please submit your code and a bug report
				via the trac website.
				*/
				break;
			case 0x01:	//Phase C action
				fault_val[3] = 'C';
				fault_val[4] = '\0';
				impl_fault = 20;
				working_protect_phases[2] = 1;
				break;
			case 0x02:	//Phase B action
				fault_val[3] = 'B';
				fault_val[4] = '\0';
				impl_fault = 19;
				working_protect_phases[1] = 1;
				break;
			case 0x03:	//Phase B and C action
				fault_val[3] = 'B';
				fault_val[4] = 'C';
				fault_val[5] = '\0';
				impl_fault = 22;
				working_protect_phases[1] = 1;
				working_protect_phases[2] = 1;
				break;
			case 0x04:	//Phase A action
				fault_val[3] = 'A';
				fault_val[4] = '\0';
				impl_fault = 18;
				working_protect_phases[0] = 1;
				break;
			case 0x05:	//Phase A and C action
				fault_val[3] = 'A';
				fault_val[4] = 'C';
				fault_val[5] = '\0';
				impl_fault = 23;
				working_protect_phases[0] = 1;
				working_protect_phases[2] = 1;
				break;
			case 0x06:	//Phase A and B action
				fault_val[3] = 'A';
				fault_val[4] = 'B';
				fault_val[5] = '\0';
				impl_fault = 21;
				working_protect_phases[0] = 1;
				working_protect_phases[1] = 1;
				break;
			case 0x07:	//All three went
				fault_val[3] = 'A';
				fault_val[4] = 'B';
				fault_val[5] = 'C';
				fault_val[6] = '\0';
				impl_fault = 24;
				working_protect_phases[0] = 1;
				working_protect_phases[1] = 1;
				working_protect_phases[2] = 1;
				break;
			default:
				GL_THROW("switch:%s supposedly opened, but doesn't register the right phases",obj->name);
				//Defined above
			}//End switch

			//See if we're a close an not in any previous faulting
			if (fault_mode)
			{
				//Loop through the phases
				for (indexval=0; indexval<3; indexval++)
				{
					//See if we need to be "our own protector"
					if ((protect_locations[indexval] == -1) && (working_protect_phases[indexval] == 1))
					{
						//Flag us as our own device -- otherwise restoration gets a little odd
						protect_locations[indexval] = NR_branch_reference;
					}
				}
			}

			if (event_schedule != nullptr)	//Function was mapped - go for it!
			{
				//Call the function
				if (fault_mode)	//Restoration - make fail time in the past
				{
					if (mean_repair_time != 0)
						temp_time = 50 + (TIMESTAMP)(mean_repair_time);
					else
						temp_time = 50;

					//Call function
					result_val = ((int (*)(OBJECT *, OBJECT *, char *, TIMESTAMP, TIMESTAMP, int, bool))(*event_schedule))(*eventgen_obj,obj,fault_val,(*t0-50),temp_time,impl_fault,fault_mode);
				}
				else	//Failing - normal
				{
					result_val = ((int (*)(OBJECT *, OBJECT *, char *, TIMESTAMP, TIMESTAMP, int, bool))(*event_schedule))(*eventgen_obj,obj,fault_val,*t0,TS_NEVER,-1,fault_mode);
				}

				//Make sure it worked
				if (result_val != 1)
				{
					GL_THROW("Attempt to change switch:%s failed in a reliability manner",obj->name);
					/*  TROUBLESHOOT
					While attempting to propagate a changed switch's impacts, an error was encountered.  Please
					try again.  If the error persists, please submit your code and a bug report via the trac website.
					*/
				}

				//Ensure we don't go anywhere yet
				*t2 = *t0;

			}	//End fault object present with reliability
			else    //No object, just fail us out - save the iterations
			{
				gl_warning("No fault_check object present - Newton-Raphson solver may fail!");
				/*  TROUBLESHOOT
				A switch changed and created an open link.  If the system is not meshed, the Newton-Raphson
				solver will likely fail.  Theoretically, this should be a quick fail due to a singular matrix.
				However, the system occasionally gets stuck and will exhaust iteration cycles before continuing.
				If the fuse is blowing and the NR solver still iterates for a long time, this may be the case.
				*/
			}
		}//End NR call
	}//end not meshed checking mode
	//defaulted else -- meshed checking, but we don't do anything for that (link handles all)

}

TIMESTAMP switch_object::presync(TIMESTAMP t0)
{
	int implemented_fault = 0;
	char256 fault_type;
	bool closing = true;
	TIMESTAMP repair_time = TS_NEVER;
	OBJECT *protect_obj = nullptr;
	unsigned char phase_changes = 0x00;

	if (local_switching) {
		if (switch_banked_mode == BANKED_SW) { // if any phase state has changed, they all follow the leader
			if (phase_A_state != (prev_full_status & 0x04)) {
				phase_B_state = phase_C_state = phase_A_state;
			} else if (phase_B_state != (prev_full_status & 0x02)) {
				phase_A_state = phase_C_state = phase_B_state;
			}
			if (phase_C_state != (prev_full_status & 0x01)) {
				phase_A_state = phase_B_state = phase_C_state;
			}
		}
		phased_switch_status = 0x00;
		if (phase_A_state == SW_CLOSED) phased_switch_status |= 0x04;
		if (phase_B_state == SW_CLOSED) phased_switch_status |= 0x02;
		if (phase_C_state == SW_CLOSED) phased_switch_status |= 0x01;
		phase_changes = phased_switch_status ^ prev_full_status;
		gl_verbose ("switch_object::presync:%s:%ld:%d:%d:%d", get_name(), t0, prev_full_status, phased_switch_status, phase_changes);
		if (phased_switch_status != prev_full_status)	{
			if (phased_switch_status < prev_full_status) closing = false;
			if (phase_changes == 0x04)	{
				strcpy (fault_type, "SW-A");
				implemented_fault = 18;
			} else if (phase_changes == 0x02) {
				strcpy (fault_type, "SW-B");
				implemented_fault = 19;
			} else if (phase_changes == 0x01) {
				strcpy (fault_type, "SW-C");
				implemented_fault = 20;
			} else if (phase_changes == 0x06) {
				strcpy (fault_type, "SW-AB");
				implemented_fault = 21;
			} else if (phase_changes == 0x03) {
				strcpy (fault_type, "SW-BC");
				implemented_fault = 22;
			} else if (phase_changes == 0x05) {
				strcpy (fault_type, "SW-AC");
				implemented_fault = 23;
			} else if (phase_changes == 0x07) {
				strcpy (fault_type, "SW-ABC");
				implemented_fault = 24;
			} else {
				strcpy (fault_type, "None");
				implemented_fault = 0;
			}
			if (closing) {
				link_fault_off (&implemented_fault, fault_type);
			} else {
				link_fault_on (&protect_obj, fault_type, &implemented_fault, &repair_time);
			}
		}
	}
	// Call the ancestor's presync
	TIMESTAMP result = link_object::presync(t0);
	return result;
}

TIMESTAMP switch_object::sync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	unsigned char work_phases_pre, work_phases_post;
	gl_verbose ("switch_object::sync:%s:%ld:%d:%d:%d", get_name(), t0, phase_A_state, phase_B_state, phase_C_state);
	//Try to map the event_schedule function address, if we haven't tried yet
	if (!event_schedule_map_attempt)
	{
		//First check to see if a fault_check object even exists
		if (fault_check_object != nullptr)
		{
			//It exists, good start! - now see if the proper variable is populated!
			eventgen_obj = get_object(fault_check_object, "eventgen_object");

			//See if it worked - if not, assume it doesn't exist
			if (*eventgen_obj != nullptr)
			{
				//It's not null, map up the scheduler function
				event_schedule = (FUNCTIONADDR)(gl_get_function(*eventgen_obj,"add_event"));
								
				//Make sure it was found
				if (event_schedule == nullptr)
				{
					gl_warning("Unable to map add_event function in eventgen:%s",*(*eventgen_obj)->name);
					/*  TROUBLESHOOT
					While attempting to map the "add_event" function from an eventgen object, the function failed to be
					found.  Ensure the target object in fault_check is an eventgen object and this function exists.  If
					the error persists, please submit your code and a bug report via the trac website.
					*/
				}
			} else { // no event generator, but we might want regular power flow with faults
				//See if the fault check is in a particular state
				gld_property *flt_chk_property;
				enumeration flt_check_property_value;

				flt_chk_property = new gld_property(fault_check_object,"check_mode");

				if (!flt_chk_property->is_valid() || !flt_chk_property->is_enumeration())
				{
					GL_THROW("switch:%d - %s - Failure to map fault_check property",obj->id,(obj->name?obj->name:"Unnamed"));
					/*  TROUBLESHOOT
					While attempting to map the "check_mode" property of the fault_check object, an error was encountered.
					Please check your GLM and try again.  If the error persists, please submit your model and a description
					to the issues system.
					*/
				}

				//Pull the property
				flt_check_property_value = flt_chk_property->get_enumeration();

				//Check it
				if (flt_check_property_value == 4)	//fault_check::SWITCHING
				{
					local_switching = true;
				}

				//Clean up the object
				delete flt_chk_property;
			}
		}
		event_schedule_map_attempt = true;
	}
	// update time variable
	if (prev_SW_time != t0)	//New timestep
		prev_SW_time = t0;

	//Call functionalized "pre-link" sync items
	BOTH_switch_sync_pre(&work_phases_pre, &work_phases_post);

	//Call overlying link sync
	gl_verbose ("  calling link_object::sync:%d:%d:%d", phase_A_state, phase_B_state, phase_C_state);
	TIMESTAMP t2=link_object::sync(t0);

	if (solver_method == SM_NR)
	{
		//Call functionalized "post-link" sync items
		NR_switch_sync_post(&work_phases_pre, &work_phases_post, obj, &t0, &t2);
	}
	if (t2==TS_NEVER)
		return(t2);
	else
		return(-t2);	//Soft limit it
}

//Function to perform actual switch sync calls (changes, etc.) - functionalized since essentially used in
//reliability calls as well, so need to make sure the two call points are consistent
void switch_object::switch_sync_function(void)
{
	unsigned char pres_status;
	double phase_total, switch_total;
	OBJECT *obj = OBJECTHDR(this);
	int result_val;

	gl_verbose ("  switch_sync_function:%s:%d:%d:%d:%d:%d", get_name(), status, prev_status, phase_A_state, phase_B_state, phase_C_state);
	pres_status = 0x00;	//Reset individual status indicator - assumes all start open

	//See which mode we are operating in
	if (!meshed_fault_checking_enabled)	//"Normal" mode
	{
		if (switch_banked_mode == BANKED_SW)	//Banked mode
		{
			if (status == prev_status)
			{
				//Banked mode is operated by majority rule.  If the majority are a different state, all become that state
				phase_total = (double)(has_phase(PHASE_A) + has_phase(PHASE_B) + has_phase(PHASE_C));	//See how many switches we have

				switch_total = 0.0;
				if (has_phase(PHASE_A) && (phase_A_state == SW_CLOSED))
					switch_total += 1.0;

				if (has_phase(PHASE_B) && (phase_B_state == SW_CLOSED))
					switch_total += 1.0;

				if (has_phase(PHASE_C) && (phase_C_state == SW_CLOSED))
					switch_total += 1.0;

				switch_total /= phase_total;

				if (switch_total > 0.5)	//In two switches, a stalemate occurs.  We'll consider this a "maintain status quo" state
				{
					status = LS_CLOSED;	//If it wasn't here, it is now
					phase_A_state = phase_B_state = phase_C_state = SW_CLOSED;	//Set all to closed - phase checks will sort it out
				}
				else	//Minority or stalemate
				{
					if (switch_total == 0.5)	//Stalemate
					{
						if (status == LS_OPEN)	//These check assume phase_X_state will be manipulated, not status
						{
							phase_A_state = phase_B_state = phase_C_state = SW_OPEN;
						}
						else	//Closed
						{
							phase_A_state = phase_B_state = phase_C_state = SW_CLOSED;
						}
					}
					else	//Not stalemate - open all
					{
						status = LS_OPEN;
						phase_A_state = phase_B_state = phase_C_state = SW_OPEN;	//Set all to open - phase checks will sort it out
					}
				}
			}//End status is same
			else	//Not the same - force the inputs
			{
				if (status==LS_OPEN)
					phase_A_state = phase_B_state = phase_C_state = SW_OPEN;	//Flag all as open
				else
					phase_A_state = phase_B_state = phase_C_state = SW_CLOSED;	//Flag all as closed
			}//End not same

			if (status==LS_OPEN)
			{
				if (has_phase(PHASE_A))
				{
					if (solver_method == SM_NR)
					{
						From_Y[0][0] = gld::complex(0.0,0.0);	//Update admittance
						b_mat[0][0] = gld::complex(0.0,0.0);
						a_mat[0][0] = 0.0;					//Update the voltage ratio matrix as well (for power calcs)
						d_mat[0][0] = 0.0;	
						NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove this bit
					}
					else	//Assume FBS
					{
						A_mat[0][0] = gld::complex(0.0,0.0);
						d_mat[0][0] = gld::complex(0.0,0.0);
						B_mat[0][0] = gld::complex(0.0,0.0);
						b_mat[0][0] = gld::complex(0.0,0.0);
					}
				}

				if (has_phase(PHASE_B))
				{
					if (solver_method == SM_NR)
					{
						From_Y[1][1] = gld::complex(0.0,0.0);	//Update admittance
						b_mat[1][1] = gld::complex(0.0,0.0);
						a_mat[1][1] = 0.0;					//Update the voltage ratio matrix as well (for power calcs)
						d_mat[1][1] = 0.0;	
						NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove this bit
					}
					else
					{
						A_mat[1][1] = gld::complex(0.0,0.0);
						d_mat[1][1] = gld::complex(0.0,0.0);
						B_mat[1][1] = gld::complex(0.0,0.0);
						b_mat[1][1] = gld::complex(0.0,0.0);
					}
				}

				if (has_phase(PHASE_C))
				{
					if (solver_method == SM_NR)
					{
						From_Y[2][2] = gld::complex(0.0,0.0);	//Update admittance
						b_mat[2][2] = gld::complex(0.0,0.0);
						a_mat[2][2] = 0.0;					//Update the voltage ratio matrix as well (for power calcs)
						d_mat[2][2] = 0.0;
						NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove this bit
					}
					else
					{
						A_mat[2][2] = gld::complex(0.0,0.0);
						d_mat[2][2] = gld::complex(0.0,0.0);
						B_mat[2][2] = gld::complex(0.0,0.0);
						b_mat[2][2] = gld::complex(0.0,0.0);
					}
				}
			}//end open
			else					//Must be closed then
			{
				if (has_phase(PHASE_A))
				{
					pres_status |= 0x04;				//Flag as closed

					if (solver_method == SM_NR)
					{
						From_Y[0][0] = switch_admittance_value;	//Update admittance
						b_mat[0][0] = switch_impedance_value;
						a_mat[0][0] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
						d_mat[0][0] = 1.0;
						NR_branchdata[NR_branch_reference].phases |= 0x04;	//Ensure we're set
					}
					else	//Assumed FBS
					{
						A_mat[0][0] = 1.0;
						d_mat[0][0] = 1.0;
						B_mat[0][0] = switch_impedance_value;
						b_mat[0][0] = switch_impedance_value;
					}
				}

				if (has_phase(PHASE_B))
				{
					pres_status |= 0x02;				//Flag as closed

					if (solver_method == SM_NR)
					{
						From_Y[1][1] = switch_admittance_value;	//Update admittance
						b_mat[1][1] = switch_impedance_value;
						a_mat[1][1] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
						d_mat[1][1] = 1.0;
						NR_branchdata[NR_branch_reference].phases |= 0x02;	//Ensure we're set
					}
					else
					{
						A_mat[1][1] = 1.0;
						d_mat[1][1] = 1.0;
						B_mat[1][1] = switch_impedance_value;
						b_mat[1][1] = switch_impedance_value;
					}
				}

				if (has_phase(PHASE_C))
				{
					pres_status |= 0x01;				//Flag as closed

					if (solver_method == SM_NR)
					{
						From_Y[2][2] = switch_admittance_value;	//Update admittance
						b_mat[2][2] = switch_impedance_value;
						a_mat[2][2] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
						d_mat[2][2] = 1.0;
						NR_branchdata[NR_branch_reference].phases |= 0x01;	//Ensure we're set
					}
					else
					{
						A_mat[2][2] = 1.0;
						d_mat[2][2] = 1.0;
						B_mat[2][2] = switch_impedance_value;
						b_mat[2][2] = switch_impedance_value;
					}
				}
			}//end closed
		}//End banked mode
		else	//Individual mode
		{
			if (status == LS_OPEN)	//Fully opened means all go open
			{
				phase_A_state = phase_B_state = phase_C_state = SW_OPEN;	//All open

				if (solver_method == SM_NR)
				{
					From_Y[0][0] = gld::complex(0.0,0.0);
					From_Y[1][1] = gld::complex(0.0,0.0);
					From_Y[2][2] = gld::complex(0.0,0.0);

					b_mat[0][0] = gld::complex(0.0,0.0);
					b_mat[1][1] = gld::complex(0.0,0.0);
					b_mat[2][2] = gld::complex(0.0,0.0);

					a_mat[0][0] = gld::complex(0.0,0.0);
					a_mat[1][1] = gld::complex(0.0,0.0);
					a_mat[2][2] = gld::complex(0.0,0.0);

					d_mat[0][0] = gld::complex(0.0,0.0);
					d_mat[1][1] = gld::complex(0.0,0.0);
					d_mat[2][2] = gld::complex(0.0,0.0);
		
					NR_branchdata[NR_branch_reference].phases &= 0xF0;		//Remove all our phases
				}
				else //Assumed FBS
				{
					A_mat[0][0] = gld::complex(0.0,0.0);
					A_mat[1][1] = gld::complex(0.0,0.0);
					A_mat[2][2] = gld::complex(0.0,0.0);

					d_mat[0][0] = gld::complex(0.0,0.0);
					d_mat[1][1] = gld::complex(0.0,0.0);
					d_mat[2][2] = gld::complex(0.0,0.0);

					B_mat[0][0] = gld::complex(0.0,0.0);
					B_mat[1][1] = gld::complex(0.0,0.0);
					B_mat[2][2] = gld::complex(0.0,0.0);

					b_mat[0][0] = gld::complex(0.0,0.0);
					b_mat[1][1] = gld::complex(0.0,0.0);
					b_mat[2][2] = gld::complex(0.0,0.0);
				}
			}
			else	//Closed means a phase-by-phase basis
			{
				if (has_phase(PHASE_A))
				{
					if (phase_A_state == SW_CLOSED)
					{
						pres_status |= 0x04;

						if (solver_method == SM_NR)
						{
							From_Y[0][0] = switch_admittance_value;
							b_mat[0][0] = switch_impedance_value;
							NR_branchdata[NR_branch_reference].phases |= 0x04;	//Ensure we're set
							a_mat[0][0] = 1.0;
							d_mat[0][0] = 1.0;
						}
						else	//Assumed FBS
						{
							A_mat[0][0] = 1.0;
							d_mat[0][0] = 1.0;
							B_mat[0][0] = switch_impedance_value;
							b_mat[0][0] = switch_impedance_value;
						}
					}
					else	//Must be open
					{
						if (solver_method == SM_NR)
						{
							From_Y[0][0] = gld::complex(0.0,0.0);
							b_mat[0][0] = gld::complex(0.0,0.0);
							NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Make sure we're removed
							a_mat[0][0] = 0.0;
							d_mat[0][0] = 0.0;
						}
						else
						{
							A_mat[0][0] = gld::complex(0.0,0.0);
							d_mat[0][0] = gld::complex(0.0,0.0);
							B_mat[0][0] = gld::complex(0.0,0.0);
							b_mat[0][0] = gld::complex(0.0,0.0);
						}
					}
				}

				if (has_phase(PHASE_B))
				{
					if (phase_B_state == SW_CLOSED)
					{
						pres_status |= 0x02;

						if (solver_method == SM_NR)
						{
							From_Y[1][1] = switch_admittance_value;
							b_mat[1][1] = switch_impedance_value;
							NR_branchdata[NR_branch_reference].phases |= 0x02;	//Ensure we're set
							a_mat[1][1] = 1.0;
							d_mat[1][1] = 1.0;
						}
						else
						{
							A_mat[1][1] = 1.0;
							d_mat[1][1] = 1.0;
							B_mat[1][1] = switch_impedance_value;
							b_mat[1][1] = switch_impedance_value;
						}
					}
					else	//Must be open
					{
						if (solver_method == SM_NR)
						{
							From_Y[1][1] = gld::complex(0.0,0.0);
							b_mat[1][1] = gld::complex(0.0,0.0);
							NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Make sure we're removed
							a_mat[1][1] = 0.0;
							d_mat[1][1] = 0.0;
						}
						else
						{
							A_mat[1][1] = gld::complex(0.0,0.0);
							d_mat[1][1] = gld::complex(0.0,0.0);
							B_mat[1][1] = gld::complex(0.0,0.0);
							b_mat[1][1] = gld::complex(0.0,0.0);
						}
					}
				}

				if (has_phase(PHASE_C))
				{
					if (phase_C_state == SW_CLOSED)
					{
						pres_status |= 0x01;

						if (solver_method == SM_NR)
						{
							From_Y[2][2] = switch_admittance_value;
							b_mat[2][2] = switch_impedance_value;
							NR_branchdata[NR_branch_reference].phases |= 0x01;	//Ensure we're set
							a_mat[2][2] = 1.0;
							d_mat[2][2] = 1.0;
						}
						else
						{
							A_mat[2][2] = 1.0;
							d_mat[2][2] = 1.0;
							B_mat[2][2] = switch_impedance_value;
							b_mat[2][2] = switch_impedance_value;
						}
					}
					else	//Must be open
					{
						if (solver_method == SM_NR)
						{
							From_Y[2][2] = gld::complex(0.0,0.0);
							b_mat[2][2] = gld::complex(0.0,0.0);
							NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Make sure we're removed
							a_mat[2][2] = 0.0;
							d_mat[2][2] = 0.0;
						}
						else
						{
							A_mat[2][2] = gld::complex(0.0,0.0);
							d_mat[2][2] = gld::complex(0.0,0.0);
							B_mat[2][2] = gld::complex(0.0,0.0);
							b_mat[2][2] = gld::complex(0.0,0.0);
						}
					}
				}
			}
		}//end individual mode

		//Check status before running sync (since it will clear it)
		//NR locking
		if (solver_method == SM_NR)
		{
			if ((status != prev_status) || (pres_status != prev_full_status))
			{
				LOCK_OBJECT(NR_swing_bus);	//Lock SWING since we'll be modifying this
				NR_admit_change = true;	//Flag an admittance change
				UNLOCK_OBJECT(NR_swing_bus);	//Finished
			}
		}

		prev_full_status = pres_status;	//Update the status flags
	}//End of "Normal" operation
	else	//Meshed fault checking -- phases are handled externally by link.cpp, just like other objects, but only with an eventgen call
	{
		if (status != prev_status)
		{
			//Check and see if we've mapped the function first
			if (fault_handle_call == nullptr)
			{
				//Make sure a fault_check object exists
				if (fault_check_object != nullptr)
				{
					//Get the link
					fault_handle_call = (FUNCTIONADDR)(gl_get_function(fault_check_object,"reliability_alterations"));

					//Make sure it worked
					if (fault_handle_call == nullptr)
					{
						GL_THROW("switch:%d - %s - Failed to map the topology update function!",obj->id,(obj->name ? obj->name : "Unnamed"));
						/*  TROUBLESHOOT
						While attempting to map up the topology reconfigure function, an error was encountered.  Please try again.
						If the error persists, please submit your file and an issue report.
						*/
					}
				}
				//Default else -- it will just fail
			}
			//Default else -- already mapped

			//Loop through and handle phases appropriate
			if (status == LS_CLOSED)
			{
				//Check phases
				if (has_phase(PHASE_A))
				{
					//Handle the phase - theoretically done elsewhere, but double check
					NR_branchdata[NR_branch_reference].phases |= 0x04;
					From_Y[0][0] = switch_admittance_value;
					b_mat[0][0] = switch_impedance_value;
					a_mat[0][0] = 1.0;
					d_mat[0][0] = 1.0;
					phase_A_state = SW_CLOSED;
				}

				if (has_phase(PHASE_B))
				{
					NR_branchdata[NR_branch_reference].phases |= 0x02;
					From_Y[1][1] = switch_admittance_value;
					b_mat[1][1] = switch_impedance_value;
					a_mat[1][1] = 1.0;
					d_mat[1][1] = 1.0;
					phase_B_state = SW_CLOSED;
				}

				if (has_phase(PHASE_C))
				{
					NR_branchdata[NR_branch_reference].phases |= 0x01;
					From_Y[2][2] = switch_admittance_value;
					b_mat[2][2] = switch_impedance_value;
					a_mat[2][2] = 1.0;
					d_mat[2][2] = 1.0;
					phase_C_state = SW_CLOSED;
				}

				//Call the reconfiguration function
				if (fault_handle_call != nullptr)
				{
					//Call the topology update
					result_val = ((int (*)(OBJECT *, int, bool))(*fault_handle_call))(fault_check_object,NR_branch_reference,true);

					//Make sure it worked
					if (result_val != 1)
					{
						GL_THROW("switch:%d - %s - Topology update fail!",obj->id,(obj->name ? obj->name : "Unnamed"));
						/*  TROUBLESHOOT
						While attempting to update the topology of the system after a switch action, an error occurred.
						Please try again.  If the error persists, please submit your file and an issue report.
						*/
					}
				}
				//Default else -- not mapped
			}
			else	//Must be open
			{
				//Handle phases
				NR_branchdata[NR_branch_reference].phases &= 0xF8;

				//Zero it all, just because
				From_Y[0][0] = gld::complex(0.0,0.0);
				From_Y[1][1] = gld::complex(0.0,0.0);
				From_Y[2][2] = gld::complex(0.0,0.0);

				b_mat[0][0] = gld::complex(0.0,0.0);
				b_mat[1][1] = gld::complex(0.0,0.0);
				b_mat[2][2] = gld::complex(0.0,0.0);
	
				a_mat[0][0] = gld::complex(0.0,0.0);
				a_mat[1][1] = gld::complex(0.0,0.0);
				a_mat[2][2] = gld::complex(0.0,0.0);

				d_mat[0][0] = gld::complex(0.0,0.0);
				d_mat[1][1] = gld::complex(0.0,0.0);
				d_mat[2][2] = gld::complex(0.0,0.0);

				//Call the reconfiguration function
				//Call the reconfiguration function
				if (fault_handle_call != nullptr)
				{
					//Call the topology update
					result_val = ((int (*)(OBJECT *, int, bool))(*fault_handle_call))(fault_check_object,NR_branch_reference,false);

					//Make sure it worked
					if (result_val != 1)
					{
						GL_THROW("switch:%d - %s - Topology update fail!",obj->id,(obj->name ? obj->name : "Unnamed"));
						//Defined above
					}
				}
				//Default else -- not mapped

				//Set individual flags
				phase_A_state = phase_B_state = phase_C_state = SW_OPEN;
			}

			//Update status
			prev_status = status;

			//Flag an update
			LOCK_OBJECT(NR_swing_bus);	//Lock SWING since we'll be modifying this
			NR_admit_change = true;	//Flag an admittance change
			UNLOCK_OBJECT(NR_swing_bus);	//Finished
		}
		//defaulted else - do nothing, something must have handled it externally
	}//End of meshed checking
}

//Function to replicate sync_switch_function, but not call anything (just for reliability checks)
unsigned char switch_object::switch_expected_sync_function(void)
{
	unsigned char phases_out;
	double phase_total, switch_total;
	SWITCHSTATE temp_A_state, temp_B_state, temp_C_state;
	enumeration temp_status;

	gl_verbose ("  switch_expected_sync_function:%d:%d:%d", phase_A_state, phase_B_state, phase_C_state);
	if (solver_method==SM_NR)	//Newton-Raphson checks
	{
		//Store current phases
		phases_out = NR_branchdata[NR_branch_reference].phases;
		temp_A_state = (SWITCHSTATE)phase_A_state;
		temp_B_state = (SWITCHSTATE)phase_B_state;
		temp_C_state = (SWITCHSTATE)phase_C_state;
		temp_status = status;

		if (switch_banked_mode == BANKED_SW)	//Banked mode
		{
			if (temp_status == prev_status)
			{
				//Banked mode is operated by majority rule.  If the majority are a different state, all become that state
				phase_total = (double)(has_phase(PHASE_A) + has_phase(PHASE_B) + has_phase(PHASE_C));	//See how many switches we have

				switch_total = 0.0;
				if (has_phase(PHASE_A) && (temp_A_state == SW_CLOSED))
					switch_total += 1.0;

				if (has_phase(PHASE_B) && (temp_B_state == SW_CLOSED))
					switch_total += 1.0;

				if (has_phase(PHASE_C) && (temp_C_state == SW_CLOSED))
					switch_total += 1.0;

				switch_total /= phase_total;

				if (switch_total > 0.5)	//In two switches, a stalemate occurs.  We'll consider this a "maintain status quo" state
				{
					temp_status = LS_CLOSED;	//If it wasn't here, it is now
					temp_A_state = temp_B_state = temp_C_state = SW_CLOSED;	//Set all to closed - phase checks will sort it out
				}
				else	//Minority or stalemate
				{
					if (switch_total == 0.5)	//Stalemate
					{
						if (temp_status == LS_OPEN)	//These check assume phase_X_state will be manipulated, not status
						{
							temp_A_state = temp_B_state = temp_C_state = SW_OPEN;
						}
						else	//Closed
						{
							temp_A_state = temp_B_state = temp_C_state = SW_CLOSED;
						}
					}
					else	//Not stalemate - open all
					{
						temp_status = LS_OPEN;
						temp_A_state = temp_B_state = temp_C_state = SW_OPEN;	//Set all to open - phase checks will sort it out
					}
				}
			}//End status is same
			else	//Not the same - force the inputs
			{
				if (temp_status==LS_OPEN)
					temp_A_state = temp_B_state = temp_C_state = SW_OPEN;	//Flag all as open
				else
					temp_A_state = temp_B_state = temp_C_state = SW_CLOSED;	//Flag all as closed
			}//End not same

			if (temp_status==LS_OPEN)
			{
				if (has_phase(PHASE_A))
				{
					phases_out &= 0xFB;	//Remove this bit
				}

				if (has_phase(PHASE_B))
				{
					phases_out &= 0xFD;	//Remove this bit
				}

				if (has_phase(PHASE_C))
				{
					phases_out &= 0xFE;	//Remove this bit
				}
			}//end open
			else					//Must be closed then
			{
				if (has_phase(PHASE_A))
				{
					phases_out |= 0x04;	//Ensure we're set
				}

				if (has_phase(PHASE_B))
				{
					phases_out |= 0x02;	//Ensure we're set
				}

				if (has_phase(PHASE_C))
				{
					phases_out |= 0x01;	//Ensure we're set
				}
			}//end closed
		}//End banked mode
		else	//Individual mode
		{
			if (temp_status == LS_OPEN)	//Fully opened means all go open
			{
				temp_A_state = temp_B_state = temp_C_state = SW_OPEN;	//All open
				phases_out &= 0xF0;		//Remove all our phases
			}
			else	//Closed means a phase-by-phase basis
			{
				if (has_phase(PHASE_A))
				{
					if (temp_A_state == SW_CLOSED)
					{
						phases_out |= 0x04;	//Ensure we're set
					}
					else	//Must be open
					{
						phases_out &= 0xFB;	//Make sure we're removed
					}
				}

				if (has_phase(PHASE_B))
				{
					if (temp_B_state == SW_CLOSED)
					{
						phases_out |= 0x02;	//Ensure we're set
					}
					else	//Must be open
					{
						phases_out &= 0xFD;	//Make sure we're removed
					}
				}

				if (has_phase(PHASE_C))
				{
					if (temp_C_state == SW_CLOSED)
					{
						phases_out |= 0x01;	//Ensure we're set
					}
					else	//Must be open
					{
						phases_out &= 0xFE;	//Make sure we're removed
					}
				}
			}
		}//end individual mode
	}//end SM_NR

	//Return the phases
	return phases_out;
}

//Function to externally set switch status - mainly for "out of step" updates under NR solver
//where admittance needs to be updated
void switch_object::set_switch(bool desired_status)
{
	gl_verbose ("set_switch:%s:%d", get_name(), desired_status);
	status = desired_status;	//Change the status
	//Check solver method - Only works for NR right now
	if (solver_method == SM_NR)
	{
		if (status != prev_status)	//Something's changed
		{
			//Copied from sync
			if (status==LS_OPEN)
			{
				if (has_phase(PHASE_A))
				{
					From_Y[0][0] = gld::complex(0.0,0.0);	//Update admittance
					b_mat[0][0] = gld::complex(0.0,0.0);
					a_mat[0][0] = 0.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[0][0] = 0.0;
					NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Ensure we're not set
					phase_A_state = SW_OPEN;				//Open this phase
				}

				if (has_phase(PHASE_B))
				{
					From_Y[1][1] = gld::complex(0.0,0.0);	//Update admittance
					b_mat[1][1] = gld::complex(0.0,0.0);
					a_mat[1][1] = 0.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[1][1] = 0.0;
					NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Ensure we're not set
					phase_B_state = SW_OPEN;				//Open this phase
				}

				if (has_phase(PHASE_C))
				{
					From_Y[2][2] = gld::complex(0.0,0.0);	//Update admittance
					b_mat[2][2] = gld::complex(0.0,0.0);
					a_mat[2][2] = 0.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[2][2] = 0.0;
					NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Ensure we're not set
					phase_C_state = SW_OPEN;				//Open this phase
				}
			}//end open
			else					//Must be closed then
			{
				if (has_phase(PHASE_A))
				{
					From_Y[0][0] = switch_admittance_value;	//Update admittance
					b_mat[0][0] = switch_impedance_value;
					a_mat[0][0] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[0][0] = 1.0;
					NR_branchdata[NR_branch_reference].phases |= 0x04;	//Ensure we're set
					phase_A_state = SW_CLOSED;				//Close this phase
				}

				if (has_phase(PHASE_B))
				{
					From_Y[1][1] = switch_admittance_value;	//Update admittance
					b_mat[1][1] = switch_impedance_value;
					a_mat[1][1] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[1][1] = 1.0;
					NR_branchdata[NR_branch_reference].phases |= 0x02;	//Ensure we're set
					phase_B_state = SW_CLOSED;				//Close this phase
				}

				if (has_phase(PHASE_C))
				{
					From_Y[2][2] = switch_admittance_value;	//Update admittance
					b_mat[2][2] = switch_impedance_value;
					a_mat[2][2] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[2][2] = 1.0;
					NR_branchdata[NR_branch_reference].phases |= 0x01;	//Ensure we're set
					phase_C_state = SW_CLOSED;				//Close this phase
				}
			}//end closed

			//Lock the swing first
			LOCK_OBJECT(NR_swing_bus);

			//Flag an update
			NR_admit_change = true;	//Flag an admittance change

			//Unlock swing
			UNLOCK_OBJECT(NR_swing_bus);

			//Update prev_status
			prev_status = status;
		}
		//defaulted else - no change, so nothing needs to be done
	}
	else
	{
		gl_warning("Switch status updated, but no other changes made.");
		/*  TROUBLESHOOT
		When changed under solver methods other than NR, only the switch status
		is changed.  The solver handles other details in a specific step, so no
		other changes are performed.
		*/
	}
}

//Function to externally set switch status - mainly for "out of step" updates under NR solver
//where admittance needs to be updated - this function provides individual switching ability
//0 = open, 1 = closed, 2 = don't care (leave as was)
void switch_object::set_switch_full(char desired_status_A, char desired_status_B, char desired_status_C)
{
	gl_verbose ("set_switch_full:%s:%d:%d:%d", get_name(), desired_status_A, desired_status_B, desired_status_C);
	if (desired_status_A == 0)
		phase_A_state = SW_OPEN;
	else if (desired_status_A == 1)
		phase_A_state = SW_CLOSED;
	//defaulted else - do nothing, leave it as it is

	if (desired_status_B == 0)
		phase_B_state = SW_OPEN;
	else if (desired_status_B == 1)
		phase_B_state = SW_CLOSED;
	//defaulted else - do nothing, leave it as it is

	if (desired_status_C == 0)
		phase_C_state = SW_OPEN;
	else if (desired_status_C == 1)
		phase_C_state = SW_CLOSED;
	//defaulted else - do nothing, leave it as it is

	//Call syncing function (does all that used to occur here)
	switch_sync_function();
}


//Function to externally set switch status - mainly for "out of step" updates under NR solver
//where admittance needs to be updated - this function tracks switching position for reliability faults
//Prevents system from magically "turning back on" a single islanded switch when removed as a downstream element
void switch_object::set_switch_full_reliability(unsigned char desired_status)
{
	unsigned char desA, desB, desC, phase_change;

	gl_verbose ("set_switch_full_reliability:%s:%d:%d:%d", get_name(), phased_switch_status, int(desired_status), local_switching);

	//Determine what to change
	phase_change = desired_status ^ (~faulted_switch_phases);

	//Figure out what phase configuration we want to change
	if ((phase_change & 0x04) == 0x04)	//Phase A
	{
		//Determine change direction
		if ((desired_status & 0x04) == 0x04)	//Putting it back in
		{
			//Switch A desired on - set it appropriately
			if ((phased_switch_status & 0x04) == 0x04)	//Switch A was on when this was faulted
			{
				desA=1;	//Desired a close - close it
			}
			else	//Switch A was off, make it so
			{
				desA=0;	//Desired open
			}

			faulted_switch_phases &= 0xFB;	//Remove it from the "fault-removed" phasing
		}
		else	//Removing it (faulting it)
		{
			if (phase_A_state == SW_CLOSED)
			{
				phased_switch_status |= 0x04;	//Flag it as being "was closed"
			}
			else	//Must be open
			{
				phased_switch_status &= 0xFB;	//Ensure it is flagged as such
			}

			desA=0;	//Desired an open - open it
			faulted_switch_phases |= 0x04;	//Add it into the "fault-removed" phasing
		}
	}
	else
		desA=2;	//indifferent - no change
	
	if ((phase_change & 0x02) == 0x02)	//Phase B
	{
		//Determine change direction
		if ((desired_status & 0x02) == 0x02)	//Putting it back in
		{
			//Switch B desired on - set it appropriately
			if ((phased_switch_status & 0x02) == 0x02)	//Switch B was on when this was faulted
			{
				desB=1;	//Desired a close - close it
			}
			else	//Switch B was off, make it so
			{
				desB=0;	//Desired open
			}

			faulted_switch_phases &= 0xFD;	//Remove it from the "fault-removed" phasing
		}
		else	//Removing it (faulting it)
		{
			if (phase_B_state == SW_CLOSED)
			{
				phased_switch_status |= 0x02;	//Flag it as being "was closed"
			}
			else	//Must be open
			{
				phased_switch_status &= 0xFD;	//Ensure it is flagged as such
			}

			desB=0;	//Desired an open - open it
			faulted_switch_phases |= 0x02;	//Add it into the "fault-removed" phasing
		}
	}
	else
		desB=2;	//indifferent - no change

	if ((phase_change & 0x01) == 0x01)	//Phase C
	{
		//Determine change direction
		if ((desired_status & 0x01) == 0x01)	//Putting it back in
		{
			//Switch C desired on - set it appropriately
			if ((phased_switch_status & 0x01) == 0x01)	//Switch C was on when this was faulted
			{
				desC=1;	//Desired a close - close it
			}
			else	//Switch C was off, make it so
			{
				desC=0;	//Desired open
			}

			faulted_switch_phases &= 0xFE;	//Remove it from the "fault-removed" phasing
		}
		else	//Removing it (faulting it)
		{
			if (phase_C_state == SW_CLOSED)
			{
				phased_switch_status |= 0x01;	//Flag it as being "was closed"
			}
			else	//Must be open
			{
				phased_switch_status &= 0xFE;	//Ensure it is flagged as such
			}

			desC=0;	//Desired an open - open it
			faulted_switch_phases |= 0x01;	//Add it into the "fault-removed" phasing
		}
	}
	else
		desC=2;	//indifferent - no change

	//Kludge
	if (!meshed_fault_checking_enabled)
	{
		//If a fault is present and we are banked, make sure we aren't any more
		if ((faulted_switch_phases != 0x00) && prefault_banked && (switch_banked_mode == BANKED_SW))
		{
			//Put into individual mode
			switch_banked_mode = INDIVIDUAL_SW;
		}
		else if ((faulted_switch_phases == 0x00) && prefault_banked && (switch_banked_mode == INDIVIDUAL_SW))
		{
			//Put back into banked mode
			switch_banked_mode = BANKED_SW;
		}
		//defaulted elses - either not in banked mode, or it is set appropriately
	}
	//Default else, leave it as it was

	//Perform the setting
	set_switch_full(desA,desB,desC);

}

//Retrieve the address of an object
OBJECT **switch_object::get_object(OBJECT *obj, const char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==nullptr || p->ptype!=PT_object)
		return nullptr;
	return (OBJECT**)GETADDR(obj,p);
}

//Function to adjust "faulted phases" block - in case something has tried to restore itself
void switch_object::set_switch_faulted_phases(unsigned char desired_status)
{
	gl_verbose ("set_switch_faulted_phases:%d", desired_status);
	//Remove from the fault tracker
	phased_switch_status |= desired_status;
}

//Function to pull phase information from the from/to node, using API - for triplex check at this point
set switch_object::node_phase_information(OBJECT *obj)
{
	gld_property *temp_phase_property;
	set phase_information;

	//Map it
	temp_phase_property = new gld_property(obj,"phases");

	//Make sure it worked
	if (!temp_phase_property->is_valid() || !temp_phase_property->is_set())
	{
		GL_THROW("switch:%d - %s - unable to map phase property of connecting node",get_id(),get_name());
		/*  TROUBLESHOOT
		While attempting to map the phases of the from or to node of a switch-based object, an error occurred.
		Please try again.  If the error persists, please submit a bugfix ticket.
		*/
	}

	//Pull the value
	phase_information = temp_phase_property->get_set();

	//Clean up
	delete temp_phase_property;

	//Send it back
	return phase_information;
}

//Module-level deltamode call
SIMULATIONMODE switch_object::inter_deltaupdate_switch(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	OBJECT *hdr = OBJECTHDR(this);
	TIMESTAMP t0_val, t2_val;
	unsigned char work_phases_pre, work_phases_post;

	//Initialize - just set to random values, not used here
	t0_val = TS_NEVER;
	t2_val = TS_NEVER;

	if (!interupdate_pos)	//Before powerflow call
	{
		//Switch sync item - pre-items
		BOTH_switch_sync_pre(&work_phases_pre,&work_phases_post);

		//Link sync/status update stuff
		NR_link_sync_fxn();

		//Switch sync item - post items
		NR_switch_sync_post(&work_phases_pre,&work_phases_post,hdr,&t0_val,&t2_val);
		
		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}
	else	//After the call
	{
		//Call postsync
		BOTH_link_postsync_fxn();
		
		return SM_EVENT;	//Links always just want out
	}
}//End module deltamode

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: switch_object
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT TIMESTAMP commit_switch_object(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	if (solver_method==SM_FBS)
	{
		switch_object *plink = OBJECTDATA(obj,switch_object);
		plink->calculate_power();
	}
	return TS_NEVER;
}
EXPORT int create_switch(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(switch_object::oclass);
		if (*obj!=nullptr)
		{
			switch_object *my = OBJECTDATA(*obj,switch_object);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(switch_object);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_switch(OBJECT *obj)
{
	try {
		switch_object *my = OBJECTDATA(obj,switch_object);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(switch_object);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_switch(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		switch_object *pObj = OBJECTDATA(obj,switch_object);
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
	SYNC_CATCHALL(switch_object);
}

EXPORT int isa_switch(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,switch_object)->isa(classname);
}

//Function to change switch states
EXPORT int change_switch_state(OBJECT *thisobj, unsigned char phase_change, bool state)
{
	char desA, desB, desC;

	//Map the switch
	switch_object *swtchobj = OBJECTDATA(thisobj,switch_object);
	gl_verbose ("  change_switch_state:%d:%d:%d", phase_change, state, swtchobj->switch_banked_mode);

	if ((swtchobj->switch_banked_mode == switch_object::BANKED_SW) || meshed_fault_checking_enabled)	//Banked mode - all become "state", just cause
	{
		swtchobj->set_switch(state);
	}
	else	//Must be individual
	{
		//Figure out what we need to call
		if ((phase_change & 0x04) == 0x04)
		{
			if (state)
				desA=1;	//Close it
			else
				desA=0;	//Open it
		}
		else	//Nope, no A
			desA=2;		//I don't care

		//Phase B
		if ((phase_change & 0x02) == 0x02)
		{
			if (state)
				desB=1;	//Close it
			else
				desB=0;	//Open it
		}
		else	//Nope, no B
			desB=2;		//I don't care

		//Phase C
		if ((phase_change & 0x01) == 0x01)
		{
			if (state)
				desC=1;	//Close it
			else
				desC=0;	//Open it
		}
		else	//Nope, no A
			desC=2;		//I don't care

		//Perform the switching!
		swtchobj->set_switch_full(desA,desB,desC);
	}//End individual adjustments

	return 1;	//This will always succeed...because I say so!
}

//Function to change switch states
EXPORT int change_switch_state_toggle(OBJECT *thisobj)
{
	FUNCTIONADDR funadd = nullptr;
	int ext_result;

	gl_verbose ("  change_switch_state_toggle");
	//Map the switch
	switch_object *swtchobj = OBJECTDATA(thisobj,switch_object);

	//See the current state
	if (swtchobj->status == LS_OPEN)
	{
		swtchobj->set_switch(true);
	}
	else //Must be closed
	{
		swtchobj->set_switch(false);
	}

	//Call the fault_check routine to update topology to reflect our change

	//Map the function
	funadd = (FUNCTIONADDR)(gl_get_function(fault_check_object,"reliability_alterations"));

	//Make sure it was found
	if (funadd == nullptr)
	{
		gl_error("Unable to update topology for switching action");
		/*  TROUBLESHOOT
		While attempting to map the reliability function to manipulate topology, an error occurred.
		Please try again.  If the bug persists, please submit your code and a bug report via the
		ticketing system.
		*/

		return 0;
	}

	//Perform topological check -- just pass a general node
	ext_result = ((int (*)(OBJECT *, int, bool))(*funadd))(fault_check_object,0,false);

	//Make sure it worked
	if (ext_result != 1)
	{
		gl_error("Unable to update topology for switching action");
		//defined above

		return 0;
	}

	return 1;	//This will always succeed...because I say so!
}

//Reliability interface - generalized switch operation so switches and other opjects can be similarly
EXPORT int reliability_operation(OBJECT *thisobj, unsigned char desired_phases)
{
	//Map the switch
	switch_object *swtchobj = OBJECTDATA(thisobj,switch_object);

	swtchobj->set_switch_full_reliability(desired_phases);

	return 1;	//This will always succeed...because I say so!
}

EXPORT int create_fault_switch(OBJECT *thisobj, OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time)
{
	int retval;

	//Link to ourselves
	switch_object *thisswitch = OBJECTDATA(thisobj,switch_object);

	//Try to fault up
	retval = thisswitch->link_fault_on(protect_obj, fault_type, implemented_fault,repair_time);

	return retval;
}
EXPORT int fix_fault_switch(OBJECT *thisobj, int *implemented_fault, char *imp_fault_name)
{
	int retval;

	//Link to ourselves
	switch_object *thisswitch = OBJECTDATA(thisobj,switch_object);

	//Clear the fault
	retval = thisswitch->link_fault_off(implemented_fault, imp_fault_name);

	//Clear the fault type
	*implemented_fault = -1;

	return retval;
}

EXPORT int clear_fault_switch(OBJECT *thisobj, int *implemented_fault, char *imp_fault_name)
{
	int retval;

	//Link to ourselves
	switch_object *thisswitch = OBJECTDATA(thisobj,switch_object);

	//Clear the fault
	retval = thisswitch->clear_fault_only(implemented_fault, imp_fault_name);

	//Clear the fault type
	*implemented_fault = -1;

	return retval;
}

EXPORT int switch_fault_updates(OBJECT *thisobj, unsigned char restoration_phases)
{
	gl_verbose ("  switch_fault_updates:%d", int(restoration_phases));
	//Link to ourselves
	switch_object *thisswitch = OBJECTDATA(thisobj,switch_object);

	//Call the update
	thisswitch->set_switch_faulted_phases(restoration_phases);

	return 1;	//We magically always succeed
}

//Deltamode export
EXPORT SIMULATIONMODE interupdate_switch(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	switch_object *my = OBJECTDATA(obj,switch_object);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_switch(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_link(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

//KML Export
EXPORT int switch_object_kmldata(OBJECT *obj,int (*stream)(const char*,...))
{
	switch_object *n = OBJECTDATA(obj, switch_object);
	int rv = 1;

	rv = n->kmldata(stream);

	return rv;
}

int switch_object::kmldata(int (*stream)(const char*,...))
{
	int phase[3] = {has_phase(PHASE_A),has_phase(PHASE_B),has_phase(PHASE_C)};
	enumeration state[3] = {phase_A_state, phase_B_state, phase_C_state};

	// switch state
	stream("<TR><TH ALIGN=LEFT>Status</TH>");
	for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
	{
		if ( phase[i] )
			stream("<TD ALIGN=CENTER COLSPAN=2 STYLE=\"font-family:courier;\"><NOBR>%s</NOBR></TD>", state[i]?"CLOSED":"OPEN");
		else
			stream("<TD ALIGN=CENTER COLSPAN=2 STYLE=\"font-family:courier;\">&mdash;</TD>");
	}
	stream("</TR>\n");

	// control input
	gld_global run_realtime("run_realtime");
	gld_global server("hostname");
	gld_global port("server_portnum");
	if ( run_realtime.get_bool() )
	{
		stream("<TR><TH ALIGN=LEFT>Control</TH>");
		for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
		{
			if ( phase[i] )
				stream("<TD ALIGN=CENTER COLSPAN=2 STYLE=\"font-family:courier;\"><FORM ACTION=\"http://%s:%d/kml/%s\" METHOD=GET><INPUT TYPE=SUBMIT NAME=\"switchA\" VALUE=\"%s\" /></FORM></TD>",
						(const char*)server.get_string(), port.get_int16(), (const char*)get_name(), state[i] ? "OPEN" : "CLOSE");
			else
				stream("<TD ALIGN=CENTER COLSPAN=2 STYLE=\"font-family:courier;\">&mdash;</TD>");
		}
		stream("</TR>\n");
	}
	return 2;
}

/**@}**/
