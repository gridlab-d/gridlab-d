// $Id: fuse.cpp 1186 2009-01-02 18:15:30Z dchassin $
/**	Copyright (C) 2008 Battelle Memorial Institute

	@file fuse.cpp
	@addtogroup powerflow_fuse Fuse
	@ingroup powerflow
	
	@todo fuse do not reclose ever once blown, implement fuse restoration scheme 
	(e.g., scale of hours with circuit outage)
	
	@{
*/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "fuse.h"

//initialize pointers
CLASS* fuse::oclass = NULL;
CLASS* fuse::pclass = NULL;

//////////////////////////////////////////////////////////////////////////
// fuse CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

fuse::fuse(MODULE *mod) : link_object(mod)
{
	if(oclass == NULL)
	{
		pclass = link_object::oclass;
		
		oclass = gl_register_class(mod,"fuse",sizeof(fuse),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class fuse";
		else
			oclass->trl = TRL_QUALIFIED;
        
        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
			PT_enumeration, "phase_A_status", PADDR(phase_A_state),
				PT_KEYWORD, "BLOWN", (enumeration)BLOWN,
				PT_KEYWORD, "GOOD", (enumeration)GOOD,
			PT_enumeration, "phase_B_status", PADDR(phase_B_state),
				PT_KEYWORD, "BLOWN", (enumeration)BLOWN,
				PT_KEYWORD, "GOOD", (enumeration)GOOD,
			PT_enumeration, "phase_C_status", PADDR(phase_C_state),
				PT_KEYWORD, "BLOWN", (enumeration)BLOWN,
				PT_KEYWORD, "GOOD", (enumeration)GOOD,
			PT_enumeration, "repair_dist_type", PADDR(restore_dist_type),
				PT_KEYWORD, "NONE", (enumeration)NONE,
				PT_KEYWORD, "EXPONENTIAL", (enumeration)EXPONENTIAL,
			PT_double, "current_limit[A]", PADDR(current_limit),
			PT_double, "mean_replacement_time[s]",PADDR(mean_replacement_time),	//Retains compatibility with older files
			PT_double, "fuse_resistance[Ohm]",PADDR(fuse_resistance), PT_DESCRIPTION,"The resistance value of the fuse when it is not blown.",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		if (gl_publish_function(oclass,"change_fuse_state",(FUNCTIONADDR)change_fuse_state)==NULL)
			GL_THROW("Unable to publish fuse state change function");
		if (gl_publish_function(oclass,"reliability_operation",(FUNCTIONADDR)fuse_reliability_operation)==NULL)
			GL_THROW("Unable to publish fuse reliability operation function");
		if (gl_publish_function(oclass,	"create_fault", (FUNCTIONADDR)create_fault_fuse)==NULL)
			GL_THROW("Unable to publish fault creation function");
		if (gl_publish_function(oclass,	"fix_fault", (FUNCTIONADDR)fix_fault_fuse)==NULL)
			GL_THROW("Unable to publish fault restoration function");
		if (gl_publish_function(oclass,	"change_fuse_faults", (FUNCTIONADDR)fuse_fault_updates)==NULL)
			GL_THROW("Unable to publish fuse fault correction function");

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_link)==NULL)
			GL_THROW("Unable to publish fuse deltamode function");
    }
}

int fuse::isa(char *classname)
{
	return strcmp(classname,"fuse")==0 || link_object::isa(classname);
}

int fuse::create()
{
	int result = link_object::create();

	prev_full_status = 0x00;		//Flag as all open initially
	phase_A_state = GOOD;			//All fuses good by default
	phase_B_state = GOOD;
	phase_C_state = GOOD;

	fix_time[0] = TS_NEVER;			//All fix times are NEVER!
	fix_time[1] = TS_NEVER;
	fix_time[2] = TS_NEVER;

	phased_fuse_status = 0x00;	//Reset variable
	faulted_fuse_phases = 0x00;	//No faults at onset

	current_limit = 9999.0;			//Big current!
	mean_replacement_time = 0.0;	//Flag so it gets populated
	restore_dist_type = NONE;		//Defaults to no distribution

	current_current_values[0] = current_current_values[1] = current_current_values[2] = 0.0;	//No current by default

	prev_fuse_time = 0;	//Init tracker

	event_schedule = NULL;
	eventgen_obj = NULL;
	event_schedule_map_attempt = false;	//Haven't tried to map yet
	fuse_resistance = -1.0;

	return result;
}

/**
* Object initialization is called once after all object have been created
*
* @param parent a pointer to this object's parent
* @return 1 on success, 0 on error
*/
int fuse::init(OBJECT *parent)
{
	char jindex, kindex;
	OBJECT *obj = OBJECTHDR(this);

	if ((phases & PHASE_S) == PHASE_S)
	{
		GL_THROW("fuses cannot be placed on triplex circuits");
		/*  TROUBLESHOOT
		Fuses do not currently support triplex circuits.  Please place the fuse higher upstream in the three-phase power
		area or utilize another object (such as a circuit breaker in a house model) to limit the current flow.
		*/
	}

	//Special flag moved to be universal for all solvers - mainly so phase checks catch it now
	SpecialLnk = SWITCH;

	int result = link_object::init(parent);

	//Check current limit
	if (current_limit < 0.0)
	{
		GL_THROW("fuse:%s has a negative current limit value!",obj->name);
		/*  TROUBLESHOOT
		The fuse has a negative value current limit specified.  Please specify a positive
		value for the current and try again.
		*/
	}

	if (current_limit == 0.0)
	{
		current_limit = 9999.0;	//Set to arbitrary large value

		gl_warning("fuse:%s has a zero current limit - set to 9999.9 Amps",obj->name);
		/*  TROUBLESHOOT
		A fuse somehow had a current limit of 0.0 Amps set.  This is invalid, so a placeholder
		value of 9999.0 Amps is used.  Please adjust this value accordingly.
		*/
	}

	if (mean_replacement_time<=0.0)	//Make sure the time makes sense
	{
		gl_warning("Fuse:%s has a negative or 0 mean replacement time - defaulting to 1 hour",obj->name);
		/*  TROUBLESHOOT
		A fuse has a negative or zero time specified for mean_replacement_time.  The value has therefore been
		overridden to 1 hour.  If this is unacceptable, please change the value in your GLM file.
		*/
		mean_replacement_time = 3600.0;
	}

	//Set mean_repair_time to the same value
	mean_repair_time = mean_replacement_time;

	if (solver_method==SM_FBS)
	{
		gl_warning("Fuses only work for the attached node in the FBS solver, not any deeper.");
		/*  TROUBLESHOOT
		Under the Forward-Back sweep method, fuses can only affect their directly attached downstream node.
		Due to the nature of the FBS algorithm, nodes further downstream (especially constant current loads)
		will cause an oscillatory nature in the voltage and current injections, so they will no longer be accurate.
		Either ignore these values or figure out a way to work around this limitation (player objects).
		*/
	}
	//check the fuse resistance value to see that it is not zero
	if (solver_method == SM_NR){
		if(fuse_resistance == 0.0){
			gl_warning("Fuse:%s fuse_resistance has been set to zero. This will result singular matrix. Setting to the global default.",obj->name);
			/*  TROUBLESHOOT
			Under Newton-Raphson solution method the impedance matrix cannot be a singular matrix for the inversion process.
			Change the value of fuse_resistance to something small but larger that zero.
			*/
		}
		if(fuse_resistance < 0.0){
			fuse_resistance = default_resistance;
		}
	}

	//Initialize matrices
	for (jindex=0;jindex<3;jindex++)
	{
		for (kindex=0;kindex<3;kindex++)
		{
			a_mat[jindex][kindex] = d_mat[jindex][kindex] = A_mat[jindex][kindex] = 0.0;
			c_mat[jindex][kindex] = 0.0;
			B_mat[jindex][kindex] = b_mat[jindex][kindex] = 0.0;
		}
	}

	a_mat[0][0] = d_mat[0][0] = A_mat[0][0] = (is_closed() && has_phase(PHASE_A) ? 1.0 : 0.0);
	a_mat[1][1] = d_mat[1][1] = A_mat[1][1] = (is_closed() && has_phase(PHASE_B) ? 1.0 : 0.0);
	a_mat[2][2] = d_mat[2][2] = A_mat[2][2] = (is_closed() && has_phase(PHASE_C) ? 1.0 : 0.0);

	if (solver_method==SM_FBS)
	{
		b_mat[0][0] = c_mat[0][0] = B_mat[0][0] = 0.0;
		b_mat[1][1] = c_mat[1][1] = B_mat[1][1] = 0.0;
		b_mat[2][2] = c_mat[2][2] = B_mat[2][2] = 0.0;
	}
	else if (solver_method==SM_NR)
	{
		//Flagged it as special (we'll forgo inversion processes on this)

		//Initialize off-diagonals just in case
		From_Y[0][1] = From_Y[0][2] = From_Y[1][0] = 0.0;
		From_Y[1][2] = From_Y[2][0] = From_Y[2][1] = 0.0;


		if (status==LS_OPEN)	//Take this as all should be open
		{
			From_Y[0][0] = complex(0.0,0.0);
			From_Y[1][1] = complex(0.0,0.0);
			From_Y[2][2] = complex(0.0,0.0);

			phase_A_state = phase_B_state = phase_C_state = BLOWN;	//All open
			prev_full_status = 0x00;								//Confirm here
		}
		else	//LS_CLOSED - handle individually
		{
			if (has_phase(PHASE_A))
			{
				if (phase_A_state == GOOD)
				{
					From_Y[0][0] = complex(1/fuse_resistance,1/fuse_resistance);
					prev_full_status |= 0x04;
				}
				else	//Must be open
				{
					From_Y[0][0] = complex(0.0,0.0);
					prev_full_status &=0xFB;
				}
			}

			if (has_phase(PHASE_B))
			{
				if (phase_B_state == GOOD)
				{
					From_Y[1][1] = complex(1/fuse_resistance,1/fuse_resistance);
					prev_full_status |= 0x02;
				}
				else	//Must be open
				{
					From_Y[1][1] = complex(0.0,0.0);
					prev_full_status &=0xFD;
				}
			}

			if (has_phase(PHASE_C))
			{
				if (phase_C_state == GOOD)
				{
					From_Y[2][2] = complex(1/fuse_resistance,1/fuse_resistance);
					prev_full_status |= 0x01;
				}
				else	//Must be open
				{
					From_Y[2][2] = complex(0.0,0.0);
					prev_full_status &=0xFE;
				}
			}
		}
	}
	else
	{
		GL_THROW("Fuses are not supported by this solver method");
		/*  TROUBLESHOOT
		Fuses are currently only supported in Forward-Back Sweep and
		Newton-Raphson Solvers.  Using them in other solvers is
		untested and may provide erroneous answers (if any at all).
		*/
	}

	//Store fuse status - will get updated as things change later
	phased_fuse_status = prev_full_status;

	return result;
}

TIMESTAMP fuse::sync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	unsigned char work_phases;
	bool fuse_blew;
	TIMESTAMP replacement_time;
	TIMESTAMP replacement_duration;
	TIMESTAMP t2;
	char fault_val[9];
	int result_val;

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
	if (prev_fuse_time != t0)	//New timestep
		prev_fuse_time = t0;

	//Code below only applies to NR right now - FBS legacy code has no sync values
	//May need to be appropriately adjusted once FBS supports reliability
	if (solver_method == SM_NR)
	{
		//Put any fuses back in service, if they're ready
		if (((fix_time[0] <= t0) || (fix_time[1] <= t0) || (fix_time[2] <= t0)) && (event_schedule == NULL))	//Only needs to be done if reliability isn't present
		{
			//Bring the phases back that are necessary
			if ((fix_time[0] <= t0) && ((NR_branchdata[NR_branch_reference].origphases & 0x04) == 0x04))	//Phase A ready and had a phase A
			{
				//Update status
				phase_A_state = GOOD;

				//Pop in the variables for the reliability update (if it exists)
				fix_time[0] = TS_NEVER;	//Reset variables
			}

			if ((fix_time[1] <= t0) && ((NR_branchdata[NR_branch_reference].origphases & 0x02) == 0x02))	//Phase B ready and had a phase B
			{
				//Update status
				phase_B_state = GOOD;

				//Pop in the variables for the reliability update (if it exists)
				fix_time[1] = TS_NEVER;	//Reset variables
			}

			if ((fix_time[2] <= t0) && ((NR_branchdata[NR_branch_reference].origphases & 0x01) == 0x01))	//Phase C ready and had a phase C
			{
				//Update status
				phase_C_state = GOOD;

				//Pop in the variables for the reliability update (if it exists)
				fix_time[2] = TS_NEVER;	//Reset variables
			}
		}//End back in service

		//Call syncing function
		fuse_sync_function();

		//Call overlying link sync
		t2=link_object::sync(t0);

		//Always execute check code now
		//Start with no assumed outages
		fuse_blew = false;
		work_phases = 0x00;
		replacement_time = 0;

		//Check them
		if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//Phase A valid - check it
		{
			//Link::sync is where current in is calculated.  Convert the values
			current_current_values[0] = current_in[0].Mag();

			if ((current_current_values[0] > current_limit) && (phase_A_state == GOOD))
			{
				phase_A_state = BLOWN;	//Blow the fuse
				gl_warning("Phase A of fuse:%s just blew!",obj->name);
				/*  TROUBLESHOOT
				The current through phase A of the fuse just exceeded the maximum rated value.
				Use a larger value, or otherwise change your system and try again.
				*/

				fuse_blew = true;		//Flag a change
				work_phases |= 0x04;	//Flag A change

				//See if an update is needed (it's A and first, so yes, but just to be generic)
				if (replacement_time == 0)
				{
					//Get length of outage
					if (restore_dist_type == EXPONENTIAL)
					{
						//Update mean repair time
						mean_repair_time = gl_random_exponential(RNGSTATE,1.0/mean_replacement_time);
						replacement_duration = (TIMESTAMP)(mean_repair_time);
					}
					else
					{
						//Update mean repair time - fuse always overrides link
						mean_repair_time = mean_replacement_time;
						replacement_duration = (TIMESTAMP)(mean_repair_time);
					}

					//Figure out when it is
					replacement_time = prev_fuse_time + replacement_duration;
				}
			}
			//Else is leave as is - either blown, or reliability hit it
		}

		if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//Phase B valid - check it
		{
			//Link::sync is where current in is calculated.  Convert the values
			current_current_values[1] = current_in[1].Mag();

			if ((current_current_values[1] > current_limit) && (phase_B_state == GOOD))
			{
				phase_B_state = BLOWN;	//Blow the fuse

				gl_warning("Phase B of fuse:%s just blew!",obj->name);
				/*  TROUBLESHOOT
				The current through phase B of the fuse just exceeded the maximum rated value.
				Use a larger value, or otherwise change your system and try again.
				*/

				fuse_blew = true;		//Flag a change
				work_phases |= 0x02;	//Flag B change

				//See if an update is needed
				if (replacement_time == 0)
				{
					//Get length of outage
					if (restore_dist_type == EXPONENTIAL)
					{
						//Update mean repair time
						mean_repair_time = gl_random_exponential(RNGSTATE,1.0/mean_replacement_time);
						replacement_duration = (TIMESTAMP)(mean_repair_time);
					}
					else
					{
						//Update mean repair time - fuse always overrides link
						mean_repair_time = mean_replacement_time;
						replacement_duration = (TIMESTAMP)(mean_repair_time);
					}

					//Figure out when it is
					replacement_time = prev_fuse_time + replacement_duration;
				}
			}
			//Else is leave as is - either blown, or reliability hit it
		}

		if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//Phase C valid - check it
		{
			//Link::sync is where current in is calculated.  Convert the values
			current_current_values[2] = current_in[2].Mag();

			if ((current_current_values[2] > current_limit) && (phase_C_state == GOOD))
			{
				phase_C_state = BLOWN;	//Blow the fuse

				gl_warning("Phase C of fuse:%s just blew!",obj->name);
				/*  TROUBLESHOOT
				The current through phase C of the fuse just exceeded the maximum rated value.
				Use a larger value, or otherwise change your system and try again.
				*/

				fuse_blew = true;		//Flag a change
				work_phases |= 0x01;	//Flag C change

				//See if an update is needed
				if (replacement_time == 0)
				{
					//Get length of outage
					if (restore_dist_type == EXPONENTIAL)
					{
						//Update mean repair time
						mean_repair_time = gl_random_exponential(RNGSTATE,1.0/mean_replacement_time);
						replacement_duration = (TIMESTAMP)(mean_repair_time);
					}
					else
					{
						//Update mean repair time - fuse always overrides link
						mean_repair_time = mean_replacement_time;
						replacement_duration = (TIMESTAMP)(mean_repair_time);
					}

					//Figure out when it is
					replacement_time = prev_fuse_time + replacement_duration;
				}
			}
			//Else is leave as is - either blown, or reliability hit it
		}

		if (fuse_blew == true)
		{
			//Set up fault type
			fault_val[0] = 'F';
			fault_val[1] = 'U';
			fault_val[2] = 'S';
			fault_val[3] = '-';

			//Determine who blew and store the time (assumes fuses can be replaced in parallel)
			switch (work_phases)
			{
			case 0x00:	//No fuses blown !??
				GL_THROW("fuse:%s supposedly blew, but doesn't register the right phases",obj->name);
				/*  TROUBLESHOOT
				A fuse reported an over-current condition and blew the appropriate link.  However, it did not appear
				to fully propogate this condition.  Please try again.  If the error persists, please submit your code
				and a bug report via the trac website.
				*/
				break;
			case 0x01:	//Phase C blew
				fix_time[2] = replacement_time;
				fault_val[4] = 'C';
				fault_val[5] = '\0';
				break;
			case 0x02:	//Phase B blew
				fix_time[1] = replacement_time;
				fault_val[4] = 'B';
				fault_val[5] = '\0';
				break;
			case 0x03:	//Phase B and C blew
				fix_time[1] = replacement_time;
				fix_time[2] = replacement_time;
				fault_val[4] = 'B';
				fault_val[5] = 'C';
				fault_val[6] = '\0';
				break;
			case 0x04:	//Phase A blew
				fix_time[0] = replacement_time;
				fault_val[4] = 'A';
				fault_val[5] = '\0';
				break;
			case 0x05:	//Phase A and C blew
				fix_time[0] = replacement_time;
				fix_time[2] = replacement_time;
				fault_val[4] = 'A';
				fault_val[5] = 'C';
				fault_val[6] = '\0';
				break;
			case 0x06:	//Phase A and B blew
				fix_time[0] = replacement_time;
				fix_time[1] = replacement_time;
				fault_val[4] = 'A';
				fault_val[5] = 'B';
				fault_val[6] = '\0';
				break;
			case 0x07:	//All three went
				fix_time[0] = replacement_time;
				fix_time[1] = replacement_time;
				fix_time[2] = replacement_time;
				fault_val[4] = 'A';
				fault_val[5] = 'B';
				fault_val[6] = 'C';
				fault_val[7] = '\0';
				break;
			default:
				GL_THROW("fuse:%s supposedly blew, but doesn't register the right phases",obj->name);
				//Defined above
			}//End switch

			if (event_schedule != NULL)	//Function was mapped - go for it!
			{
				//Call the function
				result_val = ((int (*)(OBJECT *, OBJECT *, char *, TIMESTAMP, TIMESTAMP, int, bool))(*event_schedule))(*eventgen_obj,obj,fault_val,t0,0,-1,false);

				//Make sure it worked
				if (result_val != 1)
				{
					GL_THROW("Attempt to blow fuse:%s failed in a reliability manner",obj->name);
					/*  TROUBLESHOOT
					While attempting to propagate a blown fuse's impacts, an error was encountered.  Please
					try again.  If the error persists, please submit your code and a bug report via the trac website.
					*/
				}

				//Ensure we don't go anywhere yet
				t2 = t0;

			}	//End fault object present
			else	//No object, just fail us out - save the iterations
			{
				gl_warning("No fault_check object present - Newton-Raphson solver may fail!");
				/*  TROUBLESHOOT
				A fuse blew and created an open link.  If the system is not meshed, the Newton-Raphson
				solver will likely fail.  Theoretically, this should be a quick fail due to a singular matrix.
				However, the system occasionally gets stuck and will exhaust iteration cycles before continuing.
				If the fuse is blowing and the NR solver still iterates for a long time, this may be the case.
				*/
			}
		}
	}//End NR-only reliability calls
	else	//FBS
		t2 = link_object::sync(t0);

	if (t2==TS_NEVER)
		return(t2);
	else
		return(-t2);	//Soft limit it
}

TIMESTAMP fuse::postsync(TIMESTAMP t0)
{
	OBJECT *hdr = OBJECTHDR(this);
	char jindex;
	unsigned char goodphases = 0x00;
	TIMESTAMP Ret_Val[3], t1;

	//FBS legacy code
	if (solver_method == SM_FBS)
	{
		//All actual checks and updates are handled in the COMMIT time-step in the fuse_check function below.
		//See which phases we need to check
		if ((phases & PHASE_A) == PHASE_A)	//Check A
		{
			if (phase_A_state == GOOD)	//Only bother if we are in service
			{
				Ret_Val[0] = TS_NEVER;		//We're still good, so we don't care when we come back
				goodphases |= 0x04;			//Mark as good
			}
			else						//We're blown
			{
				if (t0 == fix_time[0])
					Ret_Val[0] = t0 + 1;		//Jump us up 1 second so COMMIT can happen
				else
					Ret_Val[0] = fix_time[0];		//Time until we should be fixed
			}
		}
		else
			Ret_Val[0] = TS_NEVER;		//No phase A, make us really big

		//See which phases we need to check
		if ((phases & PHASE_B) == PHASE_B)	//Check B
		{
			if (phase_B_state == GOOD)	//Only bother if we are in service
			{
				Ret_Val[1] = TS_NEVER;		//We're still good, so we don't care when we come back
				goodphases |= 0x02;			//Mark as good
			}
			else						//We're blown
			{
				if (t0 == fix_time[1])
					Ret_Val[1] = t0 + 1;		//Jump us up 1 second so COMMIT can happen
				else
					Ret_Val[1] = fix_time[1];		//Time until we should be fixed
			}
		}
		else
			Ret_Val[1] = TS_NEVER;		//No phase A, make us really big


		//See which phases we need to check
		if ((phases & PHASE_C) == PHASE_C)	//Check C
		{
			if (phase_C_state == GOOD)	//Only bother if we are in service
			{
				Ret_Val[2] = TS_NEVER;		//We're still good, so we don't care when we come back
				goodphases |= 0x01;			//Mark as good
			}
			else						//We're blown
			{
				if (t0 == fix_time[2])
					Ret_Val[2] = t0 + 1;		//Jump us up 1 second so COMMIT can happen
				else
					Ret_Val[2] = fix_time[2];		//Time until we should be fixed
			}
		}
		else
			Ret_Val[2] = TS_NEVER;		//No phase A, make us really big

		//Normal link update
		t1 = link_object::postsync(t0);
		
		//Find the minimum timestep and return it
		for (jindex=0;jindex<3;jindex++)
		{
			if (Ret_Val[jindex] < t1)
				t1 = Ret_Val[jindex];
		}
	}
	else	//Other solvers
	{
		t1 = link_object::postsync(t0);
	}

	if (t1 != TS_NEVER)
		return -t1;  //Return that minimum, but don't push simulation forward.
	else
		return TS_NEVER;
}

//Function to perform actual fuse sync calls (changes, etc.) - functionalized since essentially used in
//reliability calls as well, so need to make sure the two call points are consistent
void fuse::fuse_sync_function(void)
{
	unsigned char pres_status;

	if (solver_method==SM_NR)	//Newton-Raphson checks
	{
		pres_status = 0x00;	//Reset individual status indicator - assumes all start open

		if (status == LS_OPEN)	//Fully opened means all go open
		{
			From_Y[0][0] = complex(0.0,0.0);
			From_Y[1][1] = complex(0.0,0.0);
			From_Y[2][2] = complex(0.0,0.0);

			phase_A_state = phase_B_state = phase_C_state = BLOWN;	//All open
			NR_branchdata[NR_branch_reference].phases &= 0xF0;		//Remove all our phases
		}
		else	//Closed means a phase-by-phase basis
		{
			if (has_phase(PHASE_A))
			{
				if (phase_A_state == GOOD)
				{
					From_Y[0][0] = complex(1/fuse_resistance,1/fuse_resistance);
					pres_status |= 0x04;
					NR_branchdata[NR_branch_reference].phases |= 0x04;	//Ensure we're set
				}
				else	//Must be open
				{
					From_Y[0][0] = complex(0.0,0.0);
					NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Make sure we're removed
				}
			}

			if (has_phase(PHASE_B))
			{
				if (phase_B_state == GOOD)
				{
					From_Y[1][1] = complex(1/fuse_resistance,1/fuse_resistance);
					pres_status |= 0x02;
					NR_branchdata[NR_branch_reference].phases |= 0x02;	//Ensure we're set
				}
				else	//Must be open
				{
					From_Y[1][1] = complex(0.0,0.0);
					NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Make sure we're removed
				}
			}

			if (has_phase(PHASE_C))
			{
				if (phase_C_state == GOOD)
				{
					From_Y[2][2] = complex(1/fuse_resistance,1/fuse_resistance);
					pres_status |= 0x01;
					NR_branchdata[NR_branch_reference].phases |= 0x01;	//Ensure we're set
				}
				else	//Must be open
				{
					From_Y[2][2] = complex(0.0,0.0);
					NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Make sure we're removed
				}
			}
		}

		//Check status before running sync (since it will clear it)
		if ((status != prev_status) || (pres_status != prev_full_status))
		{
			LOCK_OBJECT(NR_swing_bus);	//Lock SWING since we'll be modifying this
			NR_admit_change = true;	//Flag an admittance change
			UNLOCK_OBJECT(NR_swing_bus);	//Finished
		}

		prev_full_status = pres_status;	//Update the status flags
	}//end SM_NR
}

//Function to externally set fuse status - mainly for "out of step" updates under NR solver
//where admittance needs to be updated - this function provides individual switching ability
//0 = blown, 1 = good, 2 = don't care (leave as was)
void fuse::set_fuse_full(char desired_status_A, char desired_status_B, char desired_status_C)
{
	if (desired_status_A == 0)
		phase_A_state = BLOWN;
	else if (desired_status_A == 1)
		phase_A_state = GOOD;
	//defaulted else - do nothing, leave it as it is

	if (desired_status_B == 0)
		phase_B_state = BLOWN;
	else if (desired_status_B == 1)
		phase_B_state = GOOD;
	//defaulted else - do nothing, leave it as it is

	if (desired_status_C == 0)
		phase_C_state = BLOWN;
	else if (desired_status_C == 1)
		phase_C_state = GOOD;
	//defaulted else - do nothing, leave it as it is

	//Call syncing function (does all that used to occur here)
	fuse_sync_function();

}

//Function to externally set fuse status - mainly for "out of step" updates under NR solver
//where admittance needs to be updated - this function tracks fuse status for reliability faults
//Prevents system from magically "turning back on" a single islanded fuse when removed as a downstream element
void fuse::set_fuse_full_reliability(unsigned char desired_status)
{
	unsigned char desA, desB, desC, phase_change;

	//Determine what to change
	phase_change = desired_status ^ (~faulted_fuse_phases);

	//Figure out what phase configuration we want to change
	if ((phase_change & 0x04) == 0x04)	//Phase A
	{
		//Determine change direction
		if ((desired_status & 0x04) == 0x04)	//Putting it back in
		{
			//Fuse A desired on - set it appropriately
			if ((phased_fuse_status & 0x04) == 0x04)	//Fuse A was on when this was faulted
			{
				desA=1;	//Desired a close - close it
			}
			else	//Fuse A was off, make it so
			{
				desA=0;	//Desired open
			}

			faulted_fuse_phases &= 0xFB;	//Remove it from the "fault-removed" phasing
		}
		else	//Removing it (faulting it)
		{
			if (phase_A_state == GOOD)
			{
				phased_fuse_status |= 0x04;	//Flag it as being "was closed"
			}
			else	//Must be open
			{
				phased_fuse_status &= 0xFB;	//Ensure it is flagged as such
			}

			desA=0;	//Desired an open - open it
			faulted_fuse_phases |= 0x04;	//Add it into the "fault-removed" phasing
		}
	}
	else
		desA=2;	//indifferent - no change
	
	if ((phase_change & 0x02) == 0x02)	//Phase B
	{
		//Determine change direction
		if ((desired_status & 0x02) == 0x02)	//Putting it back in
		{
			//Fuse B desired on - set it appropriately
			if ((phased_fuse_status & 0x02) == 0x02)	//Fuse B was on when this was faulted
			{
				desB=1;	//Desired a close - close it
			}
			else	//Fuse B was off, make it so
			{
				desB=0;	//Desired open
			}

			faulted_fuse_phases &= 0xFD;	//Remove it from the "fault-removed" phasing
		}
		else	//Removing it (faulting it)
		{
			if (phase_B_state == GOOD)
			{
				phased_fuse_status |= 0x02;	//Flag it as being "was closed"
			}
			else	//Must be open
			{
				phased_fuse_status &= 0xFD;	//Ensure it is flagged as such
			}

			desB=0;	//Desired an open - open it
			faulted_fuse_phases |= 0x02;	//Add it into the "fault-removed" phasing
		}
	}
	else
		desB=2;	//indifferent - no change

	if ((phase_change & 0x01) == 0x01)	//Phase C
	{
		//Determine change direction
		if ((desired_status & 0x01) == 0x01)	//Putting it back in
		{
			//Fuse C desired on - set it appropriately
			if ((phased_fuse_status & 0x01) == 0x01)	//Fuse C was on when this was faulted
			{
				desC=1;	//Desired a close - close it
			}
			else	//Fuse C was off, make it so
			{
				desC=0;	//Desired open
			}

			faulted_fuse_phases &= 0xFE;	//Remove it from the "fault-removed" phasing
		}
		else	//Removing it (faulting it)
		{
			if (phase_C_state == GOOD)
			{
				phased_fuse_status |= 0x01;	//Flag it as being "was closed"
			}
			else	//Must be open
			{
				phased_fuse_status &= 0xFE;	//Ensure it is flagged as such
			}

			desC=0;	//Desired an open - open it
			faulted_fuse_phases |= 0x01;	//Add it into the "fault-removed" phasing
		}
	}
	else
		desC=2;	//indifferent - no change

	//Perform the setting
	set_fuse_full(desA,desB,desC);

}

//Retrieve the address of an object
OBJECT **fuse::get_object(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_object)
		return NULL;
	return (OBJECT**)GETADDR(obj,p);
}

//Function to adjust "faulted phases" block - in case something has tried to restore itself
void fuse::set_fuse_faulted_phases(unsigned char desired_status)
{
	//Remove from the fault tracker
	phased_fuse_status |= desired_status;
}

/**
* Fuse checking function
* Hold over code from previous functionality - lets FBS work as-is, for now.
*
* functionalized so don't have to change 4 entries in 3 different sets every time
*
* @param phase_to_check - the current phase to check fusing action for
* @param fcurr - array of from (line input) currents
*/
void fuse::fuse_check(set phase_to_check, complex *fcurr)
{
	char indexval;
	char phase_verbose;
	unsigned char work_phase;
	FUSESTATE *valstate;
	TIMESTAMP *fixtime;
	OBJECT *hdr = OBJECTHDR(this);

	if (phase_to_check == PHASE_A)
	{
		indexval = 0;
		valstate = (FUSESTATE*)&phase_A_state;
		phase_verbose='A';
		fixtime = &fix_time[0];
	}
	else if (phase_to_check == PHASE_B)
	{
		indexval = 1;
		valstate = (FUSESTATE*)&phase_B_state;
		phase_verbose='B';
		fixtime = &fix_time[1];
	}
	else if (phase_to_check == PHASE_C)
	{
		indexval = 2;
		valstate = (FUSESTATE*)&phase_C_state;
		phase_verbose='C';
		fixtime = &fix_time[2];
	}
	else
	{
		GL_THROW("Unknown phase to check in fuse:%d",OBJECTHDR(this)->id);
		/*  TROUBLESHOOT
		An invalid phase was specified for the phase check in a fuse.  Please
		check your code and continue.  If it persists, submit your code and a bug
		using the trac website.
		*/
	}

	//See which phases we need to check
	if ((phases & phase_to_check) == phase_to_check)	//Check phase
	{
		work_phase = 0x04 >> indexval;	//Working variable, primarily for NR

		if (*valstate == GOOD)	//Only bother if we are in service
		{
			//Check both directions, that way if we are reverse flowed it doesn't matter
			if (fcurr[indexval].Mag() > current_limit)	//We've exceeded the limit
			{
				*valstate = BLOWN;	//Trip us

				//Set us up appropriately
				A_mat[indexval][indexval] = d_mat[indexval][indexval] = 0.0;

				//Get an update time
				*fixtime = prev_fuse_time + (int64)(3600*gl_random_exponential(RNGSTATE,1.0/mean_replacement_time));

				//Announce it for giggles
				gl_warning("Phase %c of fuse:%d (%s) just blew",phase_verbose,hdr->id,hdr->name);
			}
			else	//Still good
			{
				//Ensure matrices are up to date in case someone manually set things
				A_mat[indexval][indexval] = d_mat[indexval][indexval] = 1.0;
			}
		}
		else						//We're blown
		{
			if (*fixtime <= prev_fuse_time)	//Technician has arrived and replaced us!!
			{
				//Fix us
				A_mat[indexval][indexval] = d_mat[indexval][indexval] = 1.0;

				*valstate = GOOD;
				*fixtime = TS_NEVER;	//Update the time check just in case

				//Send an announcement for giggles
				gl_warning("Phase %c of fuse:%d (%s) just returned to service",phase_verbose,hdr->id,hdr->name);
			}
			else //Still driving there or on break, no fixed yet
			{
				//Ensure matrices are up to date in case someone manually blew us (or a third, off state is implemented)
				A_mat[indexval][indexval] = d_mat[indexval][indexval] = 0.0;
			}
		}
	}
}

/**
* Routine to see if a fuse has been blown
* Legacy functionality for FBS code
*
* @param parent a pointer to this object's parent
* @return 1 on success, 0 on error
*/
int fuse::fuse_state(OBJECT *parent)
{
	this->fuse_check(PHASE_A,current_in);
	this->fuse_check(PHASE_B,current_in);
	this->fuse_check(PHASE_C,current_in);

	return 1;	//Not sure how we'd ever fail.  If I come up with a reason, we'll check
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: fuse
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_fuse(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(fuse::oclass);
		if (*obj!=NULL)
		{
			fuse *my = OBJECTDATA(*obj,fuse);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	} 
	CREATE_CATCHALL(fuse);
}

EXPORT int init_fuse(OBJECT *obj)
{
	try {
		fuse *my = OBJECTDATA(obj,fuse);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(fuse);
}

//Commit timestep - after all iterations are done
EXPORT TIMESTAMP commit_fuse(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	fuse *fsr = OBJECTDATA(obj,fuse);
	try
	{
		if (solver_method==SM_FBS)
		{
			link_object *plink = OBJECTDATA(obj,link_object);
			plink->calculate_power();
			
			return (fsr->fuse_state(obj->parent) ? TS_NEVER : 0);
		}
		else
			return TS_NEVER;
	}
	catch (const char *msg)
	{
		gl_error("%s (fuse:%d): %s", fsr->get_name(), fsr->get_id(), msg);
		return 0; 
	}
}

EXPORT TIMESTAMP sync_fuse(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		fuse *pObj = OBJECTDATA(obj,fuse);
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
	SYNC_CATCHALL(fuse);
}

/**
* Allows the core to discover whether obj is a subtype of this class.
*
* @param obj a pointer to this object
* @param classname the name of the object the core is testing
*
* @return true (1) if obj is a subtype of this class
*/
EXPORT int isa_fuse(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,fuse)->isa(classname);
}

//Function to change fuse states
EXPORT int change_fuse_state(OBJECT *thisobj, unsigned char phase_change, bool state)
{
	char desA, desB, desC;

	//Map the Fuse
	fuse *fuseobj = OBJECTDATA(thisobj,fuse);

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
	fuseobj->set_fuse_full(desA,desB,desC);

	return 1;	//This will always succeed...because I say so!
}

//Reliability interface - generalized fuse operation so fuses and other opjects can be similarly
EXPORT int fuse_reliability_operation(OBJECT *thisobj, unsigned char desired_phases)
{
	//Map the fuse
	fuse *fuseobj = OBJECTDATA(thisobj,fuse);

	fuseobj->set_fuse_full_reliability(desired_phases);

	return 1;	//This will always succeed...because I say so!
}

EXPORT int create_fault_fuse(OBJECT *thisobj, OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time, void *Extra_Data)
{
	int retval;

	//Link to ourselves
	fuse *thisfuse = OBJECTDATA(thisobj,fuse);

	//Try to fault up
	retval = thisfuse->link_fault_on(protect_obj, fault_type, implemented_fault,repair_time,Extra_Data);

	return retval;
}
EXPORT int fix_fault_fuse(OBJECT *thisobj, int *implemented_fault, char *imp_fault_name, void *Extra_Data)
{
	int retval;

	//Link to ourselves
	fuse *thisfuse = OBJECTDATA(thisobj,fuse);

	//Clear the fault
	retval = thisfuse->link_fault_off(implemented_fault, imp_fault_name, Extra_Data);

	//Clear the fault type
	*implemented_fault = -1;

	return retval;
}

EXPORT int fuse_fault_updates(OBJECT *thisobj, unsigned char restoration_phases)
{
	//Link to ourselves
	fuse *thisfuse = OBJECTDATA(thisobj,fuse);

	//Call the update
	thisfuse->set_fuse_faulted_phases(restoration_phases);

	return 1;	//We magically always succeed
}

/**@}*/
