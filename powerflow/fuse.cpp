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

#include "node.h"
#include "fuse.h"

//initialize pointers
CLASS* fuse::oclass = NULL;
CLASS* fuse::pclass = NULL;

//////////////////////////////////////////////////////////////////////////
// fuse CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

fuse::fuse(MODULE *mod) : link(mod)
{
	if(oclass == NULL)
	{
		pclass = relay::oclass;
		
		oclass = gl_register_class(mod,"fuse",sizeof(fuse),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if (oclass==NULL)
			throw "unable to register class fuse";
		else
			oclass->trl = TRL_QUALIFIED;
        
        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
			PT_double, "current_limit[A]",PADDR(current_limit),
			PT_double, "mean_replacement_time[H]",PADDR(mean_replacement_time),
			PT_enumeration, "phase_A_status", PADDR(phase_A_status),
				PT_KEYWORD, "GOOD", GOOD,
				PT_KEYWORD, "BLOWN", BLOWN,
			PT_enumeration, "phase_B_status", PADDR(phase_B_status),
				PT_KEYWORD, "GOOD", GOOD,
				PT_KEYWORD, "BLOWN", BLOWN,
			PT_enumeration, "phase_C_status", PADDR(phase_C_status),
				PT_KEYWORD, "GOOD", GOOD,
				PT_KEYWORD, "BLOWN", BLOWN,
            NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int fuse::isa(char *classname)
{
	return strcmp(classname,"fuse")==0 || link::isa(classname);
}

int fuse::create()
{
	int result = link::create();
        
    // Set up defaults
	to = from = NULL;
	fix_time_A = TS_NEVER;
	fix_time_B = TS_NEVER;
	fix_time_C = TS_NEVER;
	Prev_Time = 0;
	current_limit = 0.0;
	mean_replacement_time = 0.0;
	phase_A_status = GOOD;
	phase_B_status = GOOD;
	phase_C_status = GOOD;

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
	int jindex, kindex;

	if ((phases & PHASE_S) == PHASE_S)
		GL_THROW("fuses cannot be placed on triplex circuits");
		/*  TROUBLESHOOT
		Fuses do not currently support triplex circuits.  Please place the fuse higher upstream in the three-phase power
		area or utilize another object (such as a circuit breaker in a house model) to limit the current flow.
		*/

	//Special flag moved to be universal for all solvers - mainly so phase checks catch it now
	SpecialLnk = SWITCH;

	int result = link::init(parent);

	if (current_limit<=0.0)
		GL_THROW("Fuse:%d has no or a negative current limit set.",OBJECTHDR(this)->id);
		/*  TROUBLESHOOT
		The fuse does not have a proper current limit set.  The value is in current magnitude, so specify a number
		above zero.
		*/

	if (mean_replacement_time<0.0)
		GL_THROW("Fuse:%d (%s) has a mean replacement time interval set as a negative value set.",OBJECTHDR(this)->id,OBJECTHDR(this)->name);
		/*  TROUBLESHOOT
		The fuse does not have a proper mean replacement time specified.  Without this, it will not function properly
		if blown.  Please specify a replacement time greater than 0.
		*/
	else if (mean_replacement_time==0.0)
	{
		gl_warning("Fuse:%d (%s) does not have a mean replacement time interval set, using default.",OBJECTHDR(this)->id,OBJECTHDR(this)->name);
		mean_replacement_time = 10000;
	}
	if (solver_method==SM_FBS)
		gl_warning("Fuses only work for the attached node in the FBS solver, not any deeper.");
		/*  TROUBLESHOOT
		Under the Forward-Back sweep method, fuses can only affect their directly attached downstream node.
		Due to the nature of the FBS algorithm, nodes further downstream (especially constant current loads)
		will cause an oscillatory nature in the voltage and current injections, so they will no longer be accurate.
		Either ignore these values or figure out a way to work around this limitation (player objects).
		*/

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

	if (solver_method==SM_FBS)
	{
		//Check to see which phases we have
		//Phase A
		if ((phases & PHASE_A) == PHASE_A)
		{
			a_mat[0][0] = d_mat[0][0] = A_mat[0][0] = 1.0;
		}

		//Phase B
		if ((phases & PHASE_B) == PHASE_B)
		{
			a_mat[1][1] = d_mat[1][1] = A_mat[1][1] = 1.0;
		}

		//Phase C
		if ((phases & PHASE_C) == PHASE_C)
		{
			a_mat[2][2] = d_mat[2][2] = A_mat[2][2] = 1.0;
		}
	}
	else if (solver_method==SM_NR)
	{
		//Flagged us as a switch, since we'll work the same way anyways

		//Check to see which phases we have
		//Phase A
		if (((phases & PHASE_A) == PHASE_A) && (phase_A_status == GOOD))
		{
			From_Y[0][0] = complex(1e4,1e4);	//Update admittance
			a_mat[0][0] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
		}

		//Phase B
		if (((phases & PHASE_B) == PHASE_B) && (phase_B_status == GOOD))
		{
			From_Y[1][1] = complex(1e4,1e4);	//Update admittance
			a_mat[1][1] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
		}

		//Phase C
		if (((phases & PHASE_C) == PHASE_C) && (phase_C_status == GOOD))
		{
			From_Y[2][2] = complex(1e4,1e4);	//Update admittance
			a_mat[2][2] = 1.0;					//Update the voltage ratio matrix as well (for power calcs)
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

	return result;
}


TIMESTAMP fuse::postsync(TIMESTAMP t0)
{
	OBJECT *hdr = OBJECTHDR(this);
	char jindex;
	unsigned char goodphases = 0x00;
	TIMESTAMP Ret_Val[3];

	//Update time variable
	if (Prev_Time != t0)	//New timestep
		Prev_Time = t0;

	//All actual checks and updates are handled in the COMMIT time-step in the fuse_check function below.
	//See which phases we need to check
	if ((phases & PHASE_A) == PHASE_A)	//Check A
	{
		if (phase_A_status == GOOD)	//Only bother if we are in service
		{
			Ret_Val[0] = TS_NEVER;		//We're still good, so we don't care when we come back
			goodphases |= 0x04;			//Mark as good
		}
		else						//We're blown
		{
			if (t0 == fix_time_A)
				Ret_Val[0] = t0 + 1;		//Jump us up 1 second so COMMIT can happen
			else
				Ret_Val[0] = fix_time_A;		//Time until we should be fixed
		}
	}
	else
		Ret_Val[0] = TS_NEVER;		//No phase A, make us really big

	//See which phases we need to check
	if ((phases & PHASE_B) == PHASE_B)	//Check B
	{
		if (phase_B_status == GOOD)	//Only bother if we are in service
		{
			Ret_Val[1] = TS_NEVER;		//We're still good, so we don't care when we come back
			goodphases |= 0x02;			//Mark as good
		}
		else						//We're blown
		{
			if (t0 == fix_time_B)
				Ret_Val[1] = t0 + 1;		//Jump us up 1 second so COMMIT can happen
			else
				Ret_Val[1] = fix_time_B;		//Time until we should be fixed
		}
	}
	else
		Ret_Val[1] = TS_NEVER;		//No phase A, make us really big


	//See which phases we need to check
	if ((phases & PHASE_C) == PHASE_C)	//Check C
	{
		if (phase_C_status == GOOD)	//Only bother if we are in service
		{
			Ret_Val[2] = TS_NEVER;		//We're still good, so we don't care when we come back
			goodphases |= 0x01;			//Mark as good
		}
		else						//We're blown
		{
			if (t0 == fix_time_C)
				Ret_Val[2] = t0 + 1;		//Jump us up 1 second so COMMIT can happen
			else
				Ret_Val[2] = fix_time_C;		//Time until we should be fixed
		}
	}
	else
		Ret_Val[2] = TS_NEVER;		//No phase A, make us really big

	if (solver_method == SM_NR)	//Update NR solver
	{
		//Mask out the USB of goodphases
		goodphases |= 0xF8;

		//Update our phases
		NR_branchdata[NR_branch_reference].phases &= goodphases;
	}

	//Normal link update
	TIMESTAMP t1 = link::postsync(t0);
	
	//Find the minimum timestep and return it
	for (jindex=0;jindex<3;jindex++)
	{
		if (Ret_Val[jindex] < t1)
			t1 = Ret_Val[jindex];
	}
	if (t1 != TS_NEVER)
		return -t1;  //Return that minimum, but don't push simulation forward.
	else
		return TS_NEVER;
}

/**
* Routine to see if a fuse has been blown
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
/**
* Fuse checking function
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
		valstate = &phase_A_status;
		phase_verbose='A';
		fixtime = &fix_time_A;
	}
	else if (phase_to_check == PHASE_B)
	{
		indexval = 1;
		valstate = &phase_B_status;
		phase_verbose='B';
		fixtime = &fix_time_B;
	}
	else if (phase_to_check == PHASE_C)
	{
		indexval = 2;
		valstate = &phase_C_status;
		phase_verbose='C';
		fixtime = &fix_time_C;
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
				if (solver_method==SM_FBS)
				{
					A_mat[indexval][indexval] = d_mat[indexval][indexval] = 0.0;
				}
				else if (solver_method==SM_NR)
				{
					From_Y[indexval][indexval] = complex(0.0,0.0);	//Update admittance
					a_mat[indexval][indexval] = 0.0;				//Update the voltage ratio matrix as well (for power calcs)
					NR_admit_change = true;							//Flag for an admittance update
					
					work_phase = !work_phase;	//Compliment us
					NR_branchdata[NR_branch_reference].phases &= work_phase;	//Remove this bit
				}

				//Get an update time
				*fixtime = Prev_Time + (int64)(3600*gl_random_exponential(1.0/mean_replacement_time));

				//Announce it for giggles
				gl_warning("Phase %c of fuse:%d (%s) just blew",phase_verbose,hdr->id,hdr->name);
			}
			else	//Still good
			{
				//Ensure matrices are up to date in case someone manually set things
				if (solver_method==SM_FBS)
				{
					A_mat[indexval][indexval] = d_mat[indexval][indexval] = 1.0;
				}
				else if (solver_method==SM_NR)
				{
					From_Y[indexval][indexval] = complex(1e4,1e4);	//Update admittance
					a_mat[indexval][indexval] = 1.0;				//Update the voltage ratio matrix as well (for power calcs)
					NR_branchdata[NR_branch_reference].phases |= work_phase;	//Make sure we're still flagged as valid
				}
			}
		}
		else						//We're blown
		{
			if (*fixtime <= Prev_Time)	//Technician has arrived and replaced us!!
			{
				//Fix us
				if (solver_method==SM_FBS)
				{
					A_mat[indexval][indexval] = d_mat[indexval][indexval] = 1.0;
				}
				else if (solver_method==SM_NR)
				{
					From_Y[indexval][indexval] = complex(1e4,1e4);	//Update admittance
					a_mat[indexval][indexval] = 1.0;				//Update the voltage ratio matrix as well (for power calcs)
					NR_admit_change = true;							//Flag for an admittance update
					NR_branchdata[NR_branch_reference].phases |= work_phase;	//Flag us back into service
				}

				*valstate = GOOD;
				*fixtime = TS_NEVER;	//Update the time check just in case

				//Send an announcement for giggles
				gl_warning("Phase %c of fuse:%d (%s) just returned to service",phase_verbose,hdr->id,hdr->name);
			}
			else //Still driving there or on break, no fixed yet
			{
				//Ensure matrices are up to date in case someone manually blew us (or a third, off state is implemented)
				if (solver_method==SM_FBS)
				{
					A_mat[indexval][indexval] = d_mat[indexval][indexval] = 0.0;
				}
				else if (solver_method==SM_NR)
				{
					From_Y[indexval][indexval] = complex(0.0,0.0);	//Update admittance
					a_mat[indexval][indexval] = 0.0;				//Update the voltage ratio matrix as well (for power calcs)

					work_phase = !work_phase;	//Compliment us
					NR_branchdata[NR_branch_reference].phases &= work_phase;	//Ensure this bit is removed

				}
			}
		}
	}
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
	} 
	catch (const char *msg)
	{
		gl_error("create_fuse: %s", msg);
	}
	return 0;
}

EXPORT int init_fuse(OBJECT *obj)
{
	fuse *my = OBJECTDATA(obj,fuse);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		GL_THROW("%s (fuse:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
}




//Commit timestep - after all iterations are done
EXPORT int commit_fuse(OBJECT *obj)
{
	fuse *fsr = OBJECTDATA(obj,fuse);
	try {
		if (solver_method==SM_FBS || solver_method==SM_NR)
		{
			link *plink = OBJECTDATA(obj,link);
			plink->calculate_power();
		}
		return fsr->fuse_state(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s (fuse:%d): %s", fsr->get_name(), fsr->get_id(), msg);
		return 0; 
	}

}


EXPORT TIMESTAMP sync_fuse(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	fuse *pObj = OBJECTDATA(obj,fuse);
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
		gl_error("%s (fuse:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return TS_INVALID; 
	} catch (...) {
		gl_error("%s (fuse:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
		return TS_INVALID;
	}
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

/**@}*/
