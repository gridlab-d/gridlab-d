/** $Id: switch_object.cpp,v 1.6 2008/02/06 19:15:26 natet Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file switch_object.cpp
	@addtogroup powerflow switch_object
	@ingroup powerflow

	Implements a switch object.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "switch_object.h"

//////////////////////////////////////////////////////////////////////////
// switch_object CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* switch_object::oclass = NULL;
CLASS* switch_object::pclass = NULL;

switch_object::switch_object(MODULE *mod) : link_object(mod)
{
	if(oclass == NULL)
	{
		pclass = link_object::oclass;

		oclass = gl_register_class(mod,"switch",sizeof(switch_object),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class switch_object";
		else
			oclass->trl = TRL_QUALIFIED;

        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
			PT_enumeration, "phase_A_state", PADDR(phase_A_state),PT_DESCRIPTION,"Defines the current state of the phase A switch",
				PT_KEYWORD, "OPEN", (enumeration)OPEN,
				PT_KEYWORD, "CLOSED", (enumeration)CLOSED,
			PT_enumeration, "phase_B_state", PADDR(phase_B_state),PT_DESCRIPTION,"Defines the current state of the phase B switch",
				PT_KEYWORD, "OPEN", (enumeration)OPEN,
				PT_KEYWORD, "CLOSED", (enumeration)CLOSED,
			PT_enumeration, "phase_C_state", PADDR(phase_C_state),PT_DESCRIPTION,"Defines the current state of the phase C switch",
				PT_KEYWORD, "OPEN", (enumeration)OPEN,
				PT_KEYWORD, "CLOSED", (enumeration)CLOSED,
			PT_enumeration, "operating_mode", PADDR(switch_banked_mode),PT_DESCRIPTION,"Defines whether the switch operates in a banked or per-phase control mode",
				PT_KEYWORD, "INDIVIDUAL", (enumeration)INDIVIDUAL_SW,
				PT_KEYWORD, "BANKED", (enumeration)BANKED_SW,
			PT_double, "switch_resistance[Ohm]",PADDR(switch_resistance), PT_DESCRIPTION,"The resistance value of the switch when it is not blown.",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

			if (gl_publish_function(oclass,"change_switch_state",(FUNCTIONADDR)change_switch_state)==NULL)
				GL_THROW("Unable to publish switch state change function");
			if (gl_publish_function(oclass,"reliability_operation",(FUNCTIONADDR)reliability_operation)==NULL)
				GL_THROW("Unable to publish switch reliability operation function");
			if (gl_publish_function(oclass,	"create_fault", (FUNCTIONADDR)create_fault_switch)==NULL)
				GL_THROW("Unable to publish fault creation function");
			if (gl_publish_function(oclass,	"fix_fault", (FUNCTIONADDR)fix_fault_switch)==NULL)
				GL_THROW("Unable to publish fault restoration function");
			if (gl_publish_function(oclass,	"change_switch_faults", (FUNCTIONADDR)switch_fault_updates)==NULL)
				GL_THROW("Unable to publish switch fault correction function");

			//Publish deltamode functions
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_switch)==NULL)
				GL_THROW("Unable to publish switch deltamode function");

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
	phase_A_state = CLOSED;			//All switches closed by default
	phase_B_state = CLOSED;
	phase_C_state = CLOSED;

	prev_SW_time = 0;

	phased_switch_status = 0x00;	//Reset variable
	faulted_switch_phases = 0x00;	//No faults at onset
	prefault_banked = false;		//Condition of the switch prior to a fault - individual mode has to be enacted for reliability

	event_schedule = NULL;
	eventgen_obj = NULL;
	event_schedule_map_attempt = false;	//Haven't tried to map yet
	
	switch_resistance = -1.0;

	return result;
}

int switch_object::init(OBJECT *parent)
{
	double phase_total, switch_total;
	char indexa, indexb;

	OBJECT *obj = OBJECTHDR(this);

	//Special flag moved to be universal for all solvers - mainly so phase checks catch it now
	SpecialLnk = SWITCH;

	int result = link_object::init(parent);

	//secondary init stuff - should have been done, but we'll be safe
	//Basically zero everything
	if (solver_method==SM_FBS)
	{
		for (indexa=0; indexa<3; indexa++)
		{
			for (indexb=0; indexb<3; indexb++)
			{
				//These have to be zeroed (nature of switch)
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

		if(switch_resistance == 0.0){
			gl_warning("Switch:%s switch_resistance has been set to zero. This will result singular matrix. Setting to the global default.",obj->name);
			/*  TROUBLESHOOT
			Under Newton-Raphson solution method the impedance matrix cannot be a singular matrix for the inversion process.
			Change the value of switch_resistance to something small but larger that zero.
			*/
		}
		if(switch_resistance < 0.0){
			switch_resistance = default_resistance;
		}
	}//End NR mode

	//Make adjustments based on BANKED vs. INDIVIDUAL - replication of later code
	if (switch_banked_mode == BANKED_SW)
	{
		//Set reliability-related flag
		prefault_banked = true;

		//Banked mode is operated by majority rule.  If the majority are a different state, all become that state
		phase_total = (double)(has_phase(PHASE_A) + has_phase(PHASE_B) + has_phase(PHASE_C));	//See how many switches we have

		switch_total = 0.0;
		if (has_phase(PHASE_A) && (phase_A_state == CLOSED))
			switch_total += 1.0;

		if (has_phase(PHASE_B) && (phase_B_state == CLOSED))
			switch_total += 1.0;

		if (has_phase(PHASE_C) && (phase_C_state == CLOSED))
			switch_total += 1.0;

		switch_total /= phase_total;

		if (switch_total > 0.5)	//In two switches, a stalemate occurs.  We'll consider this a "maintain status quo" state
		{
			//Initial check, make sure stays open
			if (status == LS_OPEN)
			{
				phase_A_state = phase_B_state = phase_C_state = CLOSED;	//Set all to open - phase checks will sort it out
			}
			else
			{
				phase_A_state = phase_B_state = phase_C_state = CLOSED;	//Set all to closed - phase checks will sort it out
			}
		}
		else	//Minority or stalemate
		{
			if (switch_total == 0.5)	//Stalemate
			{
				if (status == LS_OPEN)	//These check assume phase_X_state will be manipulated, not status
				{
					phase_A_state = phase_B_state = phase_C_state = OPEN;
				}
				else	//Closed
				{
					phase_A_state = phase_B_state = phase_C_state = CLOSED;
				}
			}
			else	//Not stalemate - open all
			{
				status = LS_OPEN;
				phase_A_state = phase_B_state = phase_C_state = OPEN;	//Set all to open - phase checks will sort it out
			}
		}

		if (status==LS_OPEN)
		{
			if (solver_method == SM_NR)
			{
				From_Y[0][0] = complex(0.0,0.0);
				From_Y[1][1] = complex(0.0,0.0);
				From_Y[2][2] = complex(0.0,0.0);

				//Confirm a_mat is zerod too, just for portability
				a_mat[0][0] = complex(0.0,0.0);
				a_mat[1][1] = complex(0.0,0.0);
				a_mat[2][2] = complex(0.0,0.0);

				d_mat[0][0] = complex(0.0,0.0);
				d_mat[1][1] = complex(0.0,0.0);
				d_mat[2][2] = complex(0.0,0.0);
			}
			else	//Assumed FBS (GS not considered)
			{
				A_mat[0][0] = complex(0.0,0.0);
				A_mat[1][1] = complex(0.0,0.0);
				A_mat[2][2] = complex(0.0,0.0);

				d_mat[0][0] = complex(0.0,0.0);
				d_mat[1][1] = complex(0.0,0.0);
				d_mat[2][2] = complex(0.0,0.0);
			}

			phase_A_state = phase_B_state = phase_C_state = OPEN;	//All open
			prev_full_status = 0x00;								//Confirm here
		}
		else
		{
			if (has_phase(PHASE_A))
			{
				if (solver_method == SM_NR)
				{
					From_Y[0][0] = complex(1/switch_resistance,1/switch_resistance);
					a_mat[0][0] = 1.0;
					d_mat[0][0] = 1.0;
				}
				else
				{
					A_mat[0][0] = 1.0;
					d_mat[0][0] = 1.0;
				}

				phase_A_state = CLOSED;							//Flag as closed
				prev_full_status |= 0x04;
			}

			if (has_phase(PHASE_B))
			{
				if (solver_method == SM_NR)
				{
					From_Y[1][1] = complex(1/switch_resistance,1/switch_resistance);
					a_mat[1][1] = 1.0;
					d_mat[1][1] = 1.0;
				}
				else
				{
					A_mat[1][1] = 1.0;
					d_mat[1][1] = 1.0;
				}

				phase_B_state = CLOSED;							//Flag as closed
				prev_full_status |= 0x02;
			}

			if (has_phase(PHASE_C))
			{
				if (solver_method == SM_NR)
				{
					From_Y[2][2] = complex(1/switch_resistance,1/switch_resistance);
					a_mat[2][2] = 1.0;
					d_mat[2][2] = 1.0;
				}
				else
				{
					A_mat[2][2] = 1.0;
					d_mat[2][2] = 1.0;
				}

				phase_C_state = CLOSED;							//Flag as closed
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
				From_Y[0][0] = complex(0.0,0.0);
				From_Y[1][1] = complex(0.0,0.0);
				From_Y[2][2] = complex(0.0,0.0);

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
				A_mat[0][0] = complex(0.0,0.0);
				A_mat[1][1] = complex(0.0,0.0);
				A_mat[2][2] = complex(0.0,0.0);

				d_mat[0][0] = complex(0.0,0.0);
				d_mat[1][1] = complex(0.0,0.0);
				d_mat[2][2] = complex(0.0,0.0);
			}

			phase_A_state = phase_B_state = phase_C_state = OPEN;	//All open
			prev_full_status = 0x00;								//Confirm here
		}
		else	//LS_CLOSED - handle individually
		{
			if (has_phase(PHASE_A))
			{
				if (phase_A_state == CLOSED)
				{
					if (solver_method == SM_NR)
					{
						From_Y[0][0] = complex(1/switch_resistance,1/switch_resistance);
						a_mat[0][0] = 1.0;
						d_mat[0][0] = 1.0;
					}
					else
					{
						A_mat[0][0] = 1.0;
						d_mat[0][0] = 1.0;
					}
					prev_full_status |= 0x04;
				}
				else	//Must be open
				{
					if (solver_method == SM_NR)
					{
						From_Y[0][0] = complex(0.0,0.0);
						a_mat[0][0] = complex(0.0,0.0);
						d_mat[0][0] = complex(0.0,0.0);
					}
					else
					{
						A_mat[0][0] = complex(0.0,0.0);
						d_mat[0][0] = complex(0.0,0.0);
					}
					prev_full_status &=0xFB;
				}
			}

			if (has_phase(PHASE_B))
			{
				if (phase_B_state == CLOSED)
				{
					if (solver_method == SM_NR)
					{
						From_Y[1][1] = complex(1/switch_resistance,1/switch_resistance);
						a_mat[1][1] = 1.0;
						d_mat[1][1] = 1.0;
					}
					else
					{
						A_mat[1][1] = 1.0;
						d_mat[1][1] = 1.0;
					}
					prev_full_status |= 0x02;
				}
				else	//Must be open
				{
					if (solver_method == SM_NR)
					{
						From_Y[1][1] = complex(0.0,0.0);
						a_mat[1][1] = complex(0.0,0.0);
						d_mat[1][1] = complex(0.0,0.0);
					}
					else
					{
						A_mat[1][1] = complex(0.0,0.0);
						d_mat[1][1] = complex(0.0,0.0);
					}
					prev_full_status &=0xFD;
				}
			}

			if (has_phase(PHASE_C))
			{
				if (phase_C_state == CLOSED)
				{
					if (solver_method == SM_NR)
					{
						From_Y[2][2] = complex(1/switch_resistance,1/switch_resistance);
						a_mat[2][2] = 1.0;
						d_mat[2][2] = 1.0;
					}
					else
					{
						A_mat[2][2] = 1.0;
						d_mat[2][2] = 1.0;
					}
					prev_full_status |= 0x01;
				}
				else	//Must be open
				{
					if (solver_method == SM_NR)
					{
						From_Y[2][2] = complex(0.0,0.0);
						a_mat[2][2] = complex(0.0,0.0);
						d_mat[2][2] = complex(0.0,0.0);
					}
					else
					{
						A_mat[2][2] = complex(0.0,0.0);
						d_mat[2][2] = complex(0.0,0.0);
					}
					prev_full_status &=0xFE;
				}
			}
		}
	}//end individual

	//Store switch status - will get updated as things change later
	phased_switch_status = prev_full_status;

	return result;
}

//Functionalized switch sync call -- before link call -- for deltamode functionality
void switch_object::BOTH_switch_sync_pre(unsigned char *work_phases_pre, unsigned char *work_phases_post)
{
	//unsigned char work_phases, work_phases_pre, work_phases_post, work_phases_closed;

	if ((solver_method == SM_NR) && (event_schedule != NULL))	//NR-reliability-related stuff
	{
		//Store our phases going in
		*work_phases_pre = NR_branchdata[NR_branch_reference].phases & 0x07;
	
		//Call syncing function
		*work_phases_post = switch_expected_sync_function();

		//Store our phases going out
		*work_phases_post &= 0x07;
	}
	else	//Normal execution
	{
		//Call syncing function
		switch_sync_function();

		//Set phases the same
		*work_phases_pre = 0x00;
		*work_phases_post = 0x00;
	}
}

//Functionalized switch sync call -- after link call -- for deltamode functionality
void switch_object::NR_switch_sync_post(unsigned char *work_phases_pre, unsigned char *work_phases_post, OBJECT *obj, TIMESTAMP *t0, TIMESTAMP *t2)
{
	unsigned char work_phases, work_phases_closed; //work_phases_pre, work_phases_post, work_phases_closed;
	char fault_val[9];
	int result_val, impl_fault;
	bool fault_mode;
	TIMESTAMP temp_time;

	//See if we're in the proper cycle - NR only for now
	if (*work_phases_pre != *work_phases_post)
	{
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
			break;
		case 0x02:	//Phase B action
			fault_val[3] = 'B';
			fault_val[4] = '\0';
			impl_fault = 19;
			break;
		case 0x03:	//Phase B and C action
			fault_val[3] = 'B';
			fault_val[4] = 'C';
			fault_val[5] = '\0';
			impl_fault = 22;
			break;
		case 0x04:	//Phase A action
			fault_val[3] = 'A';
			fault_val[4] = '\0';
			impl_fault = 18;
			break;
		case 0x05:	//Phase A and C action
			fault_val[3] = 'A';
			fault_val[4] = 'C';
			fault_val[5] = '\0';
			impl_fault = 23;
			break;
		case 0x06:	//Phase A and B action
			fault_val[3] = 'A';
			fault_val[4] = 'B';
			fault_val[5] = '\0';
			impl_fault = 21;
			break;
		case 0x07:	//All three went
			fault_val[3] = 'A';
			fault_val[4] = 'B';
			fault_val[5] = 'C';
			fault_val[6] = '\0';
			impl_fault = 24;
			break;
		default:
			GL_THROW("switch:%s supposedly opened, but doesn't register the right phases",obj->name);
			//Defined above
		}//End switch

		if (event_schedule != NULL)	//Function was mapped - go for it!
		{
			//Call the function
			if (fault_mode == true)	//Restoration - make fail time in the past
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

		}	//End fault object present
		else	//No object, just fail us out - save the iterations
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
}

TIMESTAMP switch_object::sync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	unsigned char work_phases_pre, work_phases_post;

	//Try to map the event_schedule function address, if we haven't tried yet
	if (event_schedule_map_attempt == false)
	{
		//First check to see if a fault_check object even exists
		if (fault_check_object != NULL)
		{
			//It exists, good start! - now see if the proper variable is populated!
			eventgen_obj = get_object(fault_check_object, "eventgen_object");

			//See if it worked - if not, assume it doesn't exist
			if (*eventgen_obj != NULL)
			{
				//It's not null, map up the scheduler function
				event_schedule = (FUNCTIONADDR)(gl_get_function(*eventgen_obj,"add_event"));
								
				//Make sure it was found
				if (event_schedule == NULL)
				{
					gl_warning("Unable to map add_event function in eventgen:%s",*(*eventgen_obj)->name);
					/*  TROUBLESHOOT
					While attempting to map the "add_event" function from an eventgen object, the function failed to be
					found.  Ensure the target object in fault_check is an eventgen object and this function exists.  If
					the error persists, please submit your code and a bug report via the trac website.
					*/
				}
			}
			//Defaulted elses - just leave things as is :(
		}
		//Defaulted else - doesn't exist, so leave function address empty

		//Flag the attempt as having occurred
		event_schedule_map_attempt = true;
	}

	//Update time variable
	if (prev_SW_time != t0)	//New timestep
		prev_SW_time = t0;

	//Call functionalized "pre-link" sync items
	BOTH_switch_sync_pre(&work_phases_pre, &work_phases_post);

	//Call overlying link sync
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

	pres_status = 0x00;	//Reset individual status indicator - assumes all start open

	if (switch_banked_mode == BANKED_SW)	//Banked mode
	{
		if (status == prev_status)
		{
			//Banked mode is operated by majority rule.  If the majority are a different state, all become that state
			phase_total = (double)(has_phase(PHASE_A) + has_phase(PHASE_B) + has_phase(PHASE_C));	//See how many switches we have

			switch_total = 0.0;
			if (has_phase(PHASE_A) && (phase_A_state == CLOSED))
				switch_total += 1.0;

			if (has_phase(PHASE_B) && (phase_B_state == CLOSED))
				switch_total += 1.0;

			if (has_phase(PHASE_C) && (phase_C_state == CLOSED))
				switch_total += 1.0;

			switch_total /= phase_total;

			if (switch_total > 0.5)	//In two switches, a stalemate occurs.  We'll consider this a "maintain status quo" state
			{
				status = LS_CLOSED;	//If it wasn't here, it is now
				phase_A_state = phase_B_state = phase_C_state = CLOSED;	//Set all to closed - phase checks will sort it out
			}
			else	//Minority or stalemate
			{
				if (switch_total == 0.5)	//Stalemate
				{
					if (status == LS_OPEN)	//These check assume phase_X_state will be manipulated, not status
					{
						phase_A_state = phase_B_state = phase_C_state = OPEN;
					}
					else	//Closed
					{
						phase_A_state = phase_B_state = phase_C_state = CLOSED;
					}
				}
				else	//Not stalemate - open all
				{
					status = LS_OPEN;
					phase_A_state = phase_B_state = phase_C_state = OPEN;	//Set all to open - phase checks will sort it out
				}
			}
		}//End status is same
		else	//Not the same - force the inputs
		{
			if (status==LS_OPEN)
				phase_A_state = phase_B_state = phase_C_state = OPEN;	//Flag all as open
			else
				phase_A_state = phase_B_state = phase_C_state = CLOSED;	//Flag all as closed
		}//End not same

		if (status==LS_OPEN)
		{
			if (has_phase(PHASE_A))
			{
				if (solver_method == SM_NR)
				{
					From_Y[0][0] = complex(0.0,0.0);	//Update admittance
					a_mat[0][0] = 0.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[0][0] = 0.0;	
					NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove this bit
				}
				else	//Assume FBS
				{
					A_mat[0][0] = 0.0;
					d_mat[0][0] = 0.0;
				}
			}

			if (has_phase(PHASE_B))
			{
				if (solver_method == SM_NR)
				{
					From_Y[1][1] = complex(0.0,0.0);	//Update admittance
					a_mat[1][1] = 0.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[1][1] = 0.0;	
					NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove this bit
				}
				else
				{
					A_mat[1][1] = 0.0;
					d_mat[1][1] = 0.0;
				}
			}

			if (has_phase(PHASE_C))
			{
				if (solver_method == SM_NR)
				{
					From_Y[2][2] = complex(0.0,0.0);	//Update admittance
					a_mat[2][2] = 0.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[2][2] = 0.0;
					NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove this bit
				}
				else
				{
					A_mat[2][2] = 0.0;
					d_mat[2][2] = 0.0;
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
					From_Y[0][0] = complex(1/switch_resistance,1/switch_resistance);	//Update admittance
					a_mat[0][0] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[0][0] = 1.0;
					NR_branchdata[NR_branch_reference].phases |= 0x04;	//Ensure we're set
				}
				else	//Assumed FBS
				{
					A_mat[0][0] = 1.0;
					d_mat[0][0] = 1.0;
				}
			}

			if (has_phase(PHASE_B))
			{
				pres_status |= 0x02;				//Flag as closed

				if (solver_method == SM_NR)
				{
					From_Y[1][1] = complex(1/switch_resistance,1/switch_resistance);	//Update admittance
					a_mat[1][1] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[1][1] = 1.0;
					NR_branchdata[NR_branch_reference].phases |= 0x02;	//Ensure we're set
				}
				else
				{
					A_mat[1][1] = 1.0;
					d_mat[1][1] = 1.0;
				}
			}

			if (has_phase(PHASE_C))
			{
				pres_status |= 0x01;				//Flag as closed

				if (solver_method == SM_NR)
				{
					From_Y[2][2] = complex(1/switch_resistance,1/switch_resistance);	//Update admittance
					a_mat[2][2] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[2][2] = 1.0;
					NR_branchdata[NR_branch_reference].phases |= 0x01;	//Ensure we're set
				}
				else
				{
					A_mat[2][2] = 1.0;
					d_mat[2][2] = 1.0;
				}
			}
		}//end closed
	}//End banked mode
	else	//Individual mode
	{
		if (status == LS_OPEN)	//Fully opened means all go open
		{
			phase_A_state = phase_B_state = phase_C_state = OPEN;	//All open

			if (solver_method == SM_NR)
			{
				From_Y[0][0] = complex(0.0,0.0);
				From_Y[1][1] = complex(0.0,0.0);
				From_Y[2][2] = complex(0.0,0.0);
	
				NR_branchdata[NR_branch_reference].phases &= 0xF0;		//Remove all our phases
			}
			else //Assumed FBS
			{
				A_mat[0][0] = 0.0;
				A_mat[1][1] = 0.0;
				A_mat[2][2] = 0.0;

				d_mat[0][0] = 0.0;
				d_mat[1][1] = 0.0;
				d_mat[2][2] = 0.0;
			}
		}
		else	//Closed means a phase-by-phase basis
		{
			if (has_phase(PHASE_A))
			{
				if (phase_A_state == CLOSED)
				{
					pres_status |= 0x04;

					if (solver_method == SM_NR)
					{
						From_Y[0][0] = complex(1/switch_resistance,1/switch_resistance);
						NR_branchdata[NR_branch_reference].phases |= 0x04;	//Ensure we're set
						a_mat[0][0] = 1.0;
						d_mat[0][0] = 1.0;
					}
					else	//Assumed FBS
					{
						A_mat[0][0] = 1.0;
						d_mat[0][0] = 1.0;
					}
				}
				else	//Must be open
				{
					if (solver_method == SM_NR)
					{
						From_Y[0][0] = complex(0.0,0.0);
						NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Make sure we're removed
						a_mat[0][0] = 0.0;
						d_mat[0][0] = 0.0;
					}
					else
					{
						A_mat[0][0] = 0.0;
						d_mat[0][0] = 0.0;
					}
				}
			}

			if (has_phase(PHASE_B))
			{
				if (phase_B_state == CLOSED)
				{
					pres_status |= 0x02;

					if (solver_method == SM_NR)
					{
						From_Y[1][1] = complex(1/switch_resistance,1/switch_resistance);
						NR_branchdata[NR_branch_reference].phases |= 0x02;	//Ensure we're set
						a_mat[1][1] = 1.0;
						d_mat[1][1] = 1.0;
					}
					else
					{
						A_mat[1][1] = 1.0;
						d_mat[1][1] = 1.0;
					}
				}
				else	//Must be open
				{
					if (solver_method == SM_NR)
					{
						From_Y[1][1] = complex(0.0,0.0);
						NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Make sure we're removed
						a_mat[1][1] = 0.0;
						d_mat[1][1] = 0.0;
					}
					else
					{
						A_mat[1][1] = 0.0;
						d_mat[1][1] = 0.0;
					}
				}
			}

			if (has_phase(PHASE_C))
			{
				if (phase_C_state == CLOSED)
				{
					pres_status |= 0x01;

					if (solver_method == SM_NR)
					{
						From_Y[2][2] = complex(1/switch_resistance,1/switch_resistance);
						NR_branchdata[NR_branch_reference].phases |= 0x01;	//Ensure we're set
						a_mat[2][2] = 1.0;
						d_mat[2][2] = 1.0;
					}
					else
					{
						A_mat[2][2] = 1.0;
						d_mat[2][2] = 1.0;
					}
				}
				else	//Must be open
				{
					if (solver_method == SM_NR)
					{
						From_Y[2][2] = complex(0.0,0.0);
						NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Make sure we're removed
						a_mat[2][2] = 0.0;
						d_mat[2][2] = 0.0;
					}
					else
					{
						A_mat[2][2] = 0.0;
						d_mat[2][2] = 0.0;
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
}

//Function to replicate sync_switch_function, but not call anything (just for reliability checks)
unsigned char switch_object::switch_expected_sync_function(void)
{
	unsigned char phases_out;
	double phase_total, switch_total;
	SWITCHSTATE temp_A_state, temp_B_state, temp_C_state;
	enumeration temp_status;

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
				if (has_phase(PHASE_A) && (temp_A_state == CLOSED))
					switch_total += 1.0;

				if (has_phase(PHASE_B) && (temp_B_state == CLOSED))
					switch_total += 1.0;

				if (has_phase(PHASE_C) && (temp_C_state == CLOSED))
					switch_total += 1.0;

				switch_total /= phase_total;

				if (switch_total > 0.5)	//In two switches, a stalemate occurs.  We'll consider this a "maintain status quo" state
				{
					temp_status = LS_CLOSED;	//If it wasn't here, it is now
					temp_A_state = temp_B_state = temp_C_state = CLOSED;	//Set all to closed - phase checks will sort it out
				}
				else	//Minority or stalemate
				{
					if (switch_total == 0.5)	//Stalemate
					{
						if (temp_status == LS_OPEN)	//These check assume phase_X_state will be manipulated, not status
						{
							temp_A_state = temp_B_state = temp_C_state = OPEN;
						}
						else	//Closed
						{
							temp_A_state = temp_B_state = temp_C_state = CLOSED;
						}
					}
					else	//Not stalemate - open all
					{
						temp_status = LS_OPEN;
						temp_A_state = temp_B_state = temp_C_state = OPEN;	//Set all to open - phase checks will sort it out
					}
				}
			}//End status is same
			else	//Not the same - force the inputs
			{
				if (temp_status==LS_OPEN)
					temp_A_state = temp_B_state = temp_C_state = OPEN;	//Flag all as open
				else
					temp_A_state = temp_B_state = temp_C_state = CLOSED;	//Flag all as closed
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
				temp_A_state = temp_B_state = temp_C_state = OPEN;	//All open
				phases_out &= 0xF0;		//Remove all our phases
			}
			else	//Closed means a phase-by-phase basis
			{
				if (has_phase(PHASE_A))
				{
					if (temp_A_state == CLOSED)
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
					if (temp_B_state == CLOSED)
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
					if (temp_C_state == CLOSED)
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
					From_Y[0][0] = complex(0.0,0.0);	//Update admittance
					a_mat[0][0] = 0.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[0][0] = 0.0;
					NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Ensure we're not set
					phase_A_state = OPEN;				//Open this phase
				}

				if (has_phase(PHASE_B))
				{
					From_Y[1][1] = complex(0.0,0.0);	//Update admittance
					a_mat[1][1] = 0.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[1][1] = 0.0;
					NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Ensure we're not set
					phase_B_state = OPEN;				//Open this phase
				}

				if (has_phase(PHASE_C))
				{
					From_Y[2][2] = complex(0.0,0.0);	//Update admittance
					a_mat[2][2] = 0.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[2][2] = 0.0;
					NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Ensure we're not set
					phase_C_state = OPEN;				//Open this phase
				}
			}//end open
			else					//Must be closed then
			{
				if (has_phase(PHASE_A))
				{
					From_Y[0][0] = complex(1/switch_resistance,1/switch_resistance);	//Update admittance
					a_mat[0][0] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[0][0] = 1.0;
					NR_branchdata[NR_branch_reference].phases |= 0x04;	//Ensure we're set
					phase_A_state = CLOSED;				//Close this phase
				}

				if (has_phase(PHASE_B))
				{
					From_Y[1][1] = complex(1/switch_resistance,1/switch_resistance);	//Update admittance
					a_mat[1][1] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[1][1] = 1.0;
					NR_branchdata[NR_branch_reference].phases |= 0x02;	//Ensure we're set
					phase_B_state = CLOSED;				//Close this phase
				}

				if (has_phase(PHASE_C))
				{
					From_Y[2][2] = complex(1/switch_resistance,1/switch_resistance);	//Update admittance
					a_mat[2][2] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
					d_mat[2][2] = 1.0;
					NR_branchdata[NR_branch_reference].phases |= 0x01;	//Ensure we're set
					phase_C_state = CLOSED;				//Close this phase
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
	if (desired_status_A == 0)
		phase_A_state = OPEN;
	else if (desired_status_A == 1)
		phase_A_state = CLOSED;
	//defaulted else - do nothing, leave it as it is

	if (desired_status_B == 0)
		phase_B_state = OPEN;
	else if (desired_status_B == 1)
		phase_B_state = CLOSED;
	//defaulted else - do nothing, leave it as it is

	if (desired_status_C == 0)
		phase_C_state = OPEN;
	else if (desired_status_C == 1)
		phase_C_state = CLOSED;
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
			if (phase_A_state == CLOSED)
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
			if (phase_B_state == CLOSED)
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
			if (phase_C_state == CLOSED)
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

	//If a fault is present and we are banked, make sure we aren't any more
	if ((faulted_switch_phases != 0x00) && (prefault_banked == true) && (switch_banked_mode == BANKED_SW))
	{
		//Put into individual mode
		switch_banked_mode = INDIVIDUAL_SW;
	}
	else if ((faulted_switch_phases == 0x00) && (prefault_banked == true) && (switch_banked_mode == INDIVIDUAL_SW))
	{
		//Put back into banked mode
		switch_banked_mode = BANKED_SW;
	}
	//defaulted elses - either not in banked mode, or it is set appropriately

	//Perform the setting
	set_switch_full(desA,desB,desC);

}

//Retrieve the address of an object
OBJECT **switch_object::get_object(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_object)
		return NULL;
	return (OBJECT**)GETADDR(obj,p);
}

//Function to adjust "faulted phases" block - in case something has tried to restore itself
void switch_object::set_switch_faulted_phases(unsigned char desired_status)
{
	//Remove from the fault tracker
	phased_switch_status |= desired_status;
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

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Link presync stuff
		NR_link_presync_fxn();

		//Switch sync item - pre-items
		BOTH_switch_sync_pre(&work_phases_pre,&work_phases_post);

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
		if (*obj!=NULL)
		{
			switch_object *my = OBJECTDATA(*obj,switch_object);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(switch);
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
	INIT_CATCHALL(switch);
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
	SYNC_CATCHALL(switch);
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

	if (swtchobj->switch_banked_mode == switch_object::BANKED_SW)	//Banked mode - all become "state", just cause
	{
		swtchobj->set_switch(state);
	}
	else	//Must be individual
	{
		//Figure out what we need to call
		if ((phase_change & 0x04) == 0x04)
		{
			if (state==true)
				desA=1;	//Close it
			else
				desA=0;	//Open it
		}
		else	//Nope, no A
			desA=2;		//I don't care

		//Phase B
		if ((phase_change & 0x02) == 0x02)
		{
			if (state==true)
				desB=1;	//Close it
			else
				desB=0;	//Open it
		}
		else	//Nope, no B
			desB=2;		//I don't care

		//Phase C
		if ((phase_change & 0x01) == 0x01)
		{
			if (state==true)
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

//Reliability interface - generalized switch operation so switches and other opjects can be similarly
EXPORT int reliability_operation(OBJECT *thisobj, unsigned char desired_phases)
{
	//Map the switch
	switch_object *swtchobj = OBJECTDATA(thisobj,switch_object);

	swtchobj->set_switch_full_reliability(desired_phases);

	return 1;	//This will always succeed...because I say so!
}

EXPORT int create_fault_switch(OBJECT *thisobj, OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time, void *Extra_Data)
{
	int retval;

	//Link to ourselves
	switch_object *thisswitch = OBJECTDATA(thisobj,switch_object);

	//Try to fault up
	retval = thisswitch->link_fault_on(protect_obj, fault_type, implemented_fault,repair_time,Extra_Data);

	return retval;
}
EXPORT int fix_fault_switch(OBJECT *thisobj, int *implemented_fault, char *imp_fault_name, void *Extra_Data)
{
	int retval;

	//Link to ourselves
	switch_object *thisswitch = OBJECTDATA(thisobj,switch_object);

	//Clear the fault
	retval = thisswitch->link_fault_off(implemented_fault, imp_fault_name, Extra_Data);

	//Clear the fault type
	*implemented_fault = -1;

	return retval;
}

EXPORT int switch_fault_updates(OBJECT *thisobj, unsigned char restoration_phases)
{
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
/**@}**/
