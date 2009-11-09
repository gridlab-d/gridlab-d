/** $Id: restoration.cpp 2009-11-09 15:00:00Z d3x593 $
	Copyright (C) 2009 Battelle Memorial Institute
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
using namespace std;

#include "restoration.h"
#include "solver_nr.h"

//////////////////////////////////////////////////////////////////////////
// restoration CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* restoration::oclass = NULL;
CLASS* restoration::pclass = NULL;

restoration::restoration(MODULE *mod) : powerflow_object(mod)
{
	if(oclass == NULL)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod,"restoration",sizeof(restoration),PC_PRETOPDOWN|PC_POSTTOPDOWN);
		if(oclass == NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
		
		if(gl_publish_variable(oclass,
			PT_char1024,"configuration_file",PADDR(configuration_file),
			PT_int32,"reconfig_attempts",PADDR(reconfig_attempts),
			PT_int32,"reconfig_iteration_limit",PADDR(reconfig_iter_limit),
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int restoration::isa(char *classname)
{
	return strcmp(classname,"restoration")==0;
}

int restoration::create(void)
{
	int result = powerflow_object::create();
	*configuration_file = NULL;
	prev_time = 0;
	phases = PHASE_A;	//Arbitrary - set so powerflow_object shuts up (library doesn't grant us access to presynch/postsynch)

	reconfig_attempts = 0;
	reconfig_iter_limit = 0;
	attempt_reconfigure=false;

	return result;
}

int restoration::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	FILE *CONFIGINFO;

	if (solver_method == SM_NR)
	{
		if (restoration_object == NULL)
		{
			restoration_object = obj;	//Set up the global
			
			if (*configuration_file == NULL)	//Make sure an input has been specified
			{
				GL_THROW("Please specify a restoration configuration file.");
				/*  TROUBLESHOOT
				The restoration object requires an input file that determines how
				reconfiguration will occur.  Please specify this file to proceed.
				*/
			}

			//Perform file open and close - make sure the file we specified exists
			CONFIGINFO = fopen(configuration_file,"r");		//Open the file for reading (not sure how you'll want to actually do this)

			//Check it
			if (CONFIGINFO==NULL)
			{
				GL_THROW("Configuration file could not be opened!");
				/*  TROUBLESHOOT
				The configuration file specified for the restoration object does not appear
				to exist, or it can not be opened.  Make sure no other program is using this file
				and that it is in the same folder as the model GLM file.
				*/
			}

			//close the handle
			fclose(CONFIGINFO);

			//Check limits - post warnings as necessary
			if (reconfig_attempts==0)
			{
				gl_warning("Infinite reconfigurations set.");
				/*  TROUBLESHOOT
				The restoration object can have the number of reconfiguration per timestep set via the
				reconfig_attempts property.  If not set, or set to 0, the system will perform reconfigurations
				until the overall powerflow iteration limit is reached (or the system solves and moves to a new
				timestep).
				*/
			}

			if (reconfig_iter_limit==0)
			{
				gl_warning("Infinite reconfiguration iterations set.");
				/*  TROUBLESHOOT
				The restoration object can have the number of iterations powerflow can perform before trying another
				reconfiguration set.  This is set via teh reconfig_iteration_limit property.  If not set, or set to 0, the
				system will perform iterations on this reconfiguration until the overall powerflow iteration limit is reached,
				or the system solves and moves to a new reconfiguration.
				*/
			}

			//Set our rank higher than swing.  We need to go off before the swing bus, just in case we are reconfiguring things
			gl_set_rank(obj,6);	//6 = swing+1
		}
		else
		{
			GL_THROW("Only one restoration object is permitted per model file");
			/*  TROUBLESHOOT
			The restoration object manipulates the configuration of the system.  In order
			to prevent possible conflicts, only one restoration object is permitted per
			model.
			*/
		}
	}
	else
	{
		GL_THROW("Restoration control is only valid for the Newton-Raphson solver at this time.");
		/*  TROUBLESHOOT
		The restoration object only works with the Newton-Raphson powerflow solver at this time.
		Other solvers may be integrated at a future date.
		*/
	}

	return powerflow_object::init(parent);
}

TIMESTAMP restoration::presync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t1 = powerflow_object::presync(t0);
	bool NeedReconfiguration;
	FILE *CONFIGINFO;

	if (prev_time!=t0)	//New timestep - reset tracking variables
	{
		reconfig_number = 0;	//Reset number of reconfigurations that have occurred
		reconfig_iterations=0;	//Reset number of iterations that have occurred on this timestep
	}

	if ((attempt_reconfigure==true) && (NR_cycle==true))	//Solution cycle (1 off, since swing toggles it after we've run)
	{
		reconfig_number++;	//Increment the number of reconfigurations we've tried

		//Testing code - not sure how all of this will be handled

			/* --------- File read testing ----------- */
			char1024 testtext;

			CONFIGINFO = fopen(configuration_file,"r");		//Open the file for reading (not sure how you'll want to actually do this)

			//Just read in one line
			fscanf(CONFIGINFO,"%s",testtext);

			//close the handle
			fclose(CONFIGINFO);

			//print the contents, just to see if it worked
			printf(testtext);
			printf("\n");

			/* ----------- End file read testing ----------*/

			//Check voltages
			NeedReconfiguration=VoltageCheck();


			/* ---------- Array access testing ------------ */

			//print first connectivity for now - this is just an example of obtaining connectivity (for check)
			printf("Connect Pre - %d\n",Connectivity_Matrix[0][0]);

			/* ---------- End array access testing ----------*/

		/*  NOTES:
		NR_busdata is the same pointer array the NR solver uses.  Any changes to NR_busdata are reflected on the actual objects (i.e., don't change anything)

		bus names are available in NR_busdata[busindex].name
		branch names are available in NR_branchdata[branchindex].name

		Switches are read/controlled via the NR_branchdata[branchindex].status field.

		Connectivity matrix is stored in Connectivity_Matrix.  It is a 2D array of (# nodes) x (# nodes) size.  Connections are based on row/column pairs.
		e.g., if Connectivity_Matrix[2][4] had a non-zero value, it means the bus in NR_busdata[2] is connected to the bus of NR_busdata[4] via some connection.

		Connectivity_Matrix values:
		0 - no connection
		1 - "normal" line (overhead line, underground line, triplex line, transformer)
		2 - fuse - if you want to handle fuses, additional functionality for testing if they are blown or not is probably necessary
		3 - switch - open/closed status/control is handled in NR_branchdata[branchindex].status

		--- End Notes ---- */
	}

	return t1;
}

TIMESTAMP restoration::postsync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::postsync(t0);
	unsigned char ConvergenceCheck;

	bool reconfig_check = false;	//Temp working variable - will be replaced once logic is in place

	if (prev_time==0)	//First run, make sure we come back - otherwise this won't do anything
	{
		NR_retval = t0;	//Override global return, make sure we stay here once
		ret_time = t0;	//Set our tracking variable as well
		attempt_reconfigure=true;	//Set up so it will try to do a reconfiguration

		/* Testing Code - remove when done - example of traversing connectivity matrix */
			//Print the connectivity matrix for the sake of doing so
			FILE *FPCONNECT = fopen("connectout.txt","w");

			int conval;
			unsigned int indexx, indexy;
			for (indexx=0; indexx<NR_bus_count; indexx++)
			{
				for (indexy=0; indexy<NR_bus_count; indexy++)
				{
					conval=Connectivity_Matrix[indexx][indexy];

					if (conval != 0)
					{
						fprintf(FPCONNECT,NR_busdata[indexx].name);
						fprintf(FPCONNECT," - ");
						fprintf(FPCONNECT,NR_busdata[indexy].name);
						
						if (conval == 1)
							fprintf(FPCONNECT," - Normal\n");
						else if (conval==2)
							fprintf(FPCONNECT," - fuse\n");
						else if (conval==3)
							fprintf(FPCONNECT," - switch\n");
						else
							fprintf(FPCONNECT," - Uh oh\n");
					}
				}
			}
			fclose(FPCONNECT);

		/* End testing code */
	}

	if (NR_cycle==false)	//solution cycle
	{
		reconfig_iterations++;	//Increment the iterations counter

		if (NR_retval != t0)	//Convergence!
		{
			ConvergenceCheck = 0x04;

			//Reset the counter - we're either at the limit or below it, so this doesn't matter
			reconfig_iterations=0;
		}
		else
		{
			ConvergenceCheck = 0x00;
		}

		if ((reconfig_iterations>=reconfig_iter_limit) && (reconfig_iter_limit!=0))	//See if we've met the iteration limit (and it's nonzero)
		{
			ConvergenceCheck |= 0x02;
		}

		if ((reconfig_number>=reconfig_attempts) && (reconfig_attempts!=0))	//See if we've met the reconfiguration limit (and it's nonzero)
		{
			ConvergenceCheck |= 0x01;
		}
	}

	if ((prev_time!=0) && (NR_cycle==false))	//solution phase - check our results and see if we need to reloop or not
	{
		//Sample check for convergence, change as required
		switch(ConvergenceCheck) {
			case 0x00:	//Not converged, iteration limit not reached, reconfigure limit not reached
			case 0x01:	//Not converged, iteration limit not reached, reconfigure limit reached
				{
					ret_time = t0;	//Proceed as we were (keep iterating)
					break;
				}
			case 0x02:	//Not converged, iteration limit reached, reconfigure limit not reached
				{
					ret_time = t0;				//Keep us here
					attempt_reconfigure=true;	//Flag for a reconfiguration
					reconfig_iterations = 0;	//Reset the counter
					break;
				}
			case 0x03:	//Not converged, iteration limit reached, reconfigure limit reached
				{
					//We're hosed
					GL_THROW("Reconfiguration iteration and reconfiguration limits reached.");
					/*  TROUBLESHOOT
					The restoration object has reached the maximum number of reconfigurations per
					timestep.  The system failed to reach a suitable configuration, so the solver
					has halted.
					*/
					break;
				}
			case 0x04:	//Converged, iteration limit not reached, reconfigure limit not reached
				{
					//Check it
					reconfig_check = VoltageCheck();

					//check to see if we want to perform a reconfiguration, or let us go on our way
					if (reconfig_check)
					{
						ret_time = t0;			//We need to stay here, update as such
						NR_admit_change = true;	//Flag NR to perform an admittance update on next solver pass (not sure if this is absolutely needed or not yet)
						attempt_reconfigure = true;	//Flag to perform a reconfiguration
						reconfig_iterations = 0;	//Reset the iteration counter
					}
					else
					{
						ret_time = tret;	//Let us proceed
					}
					break;
				}
			case 0x05:	//Converged, iteration limit not reached, reconfigure limit reached
				{
					ret_time = tret;	//Converged and our last attempt, call it good and proceed
					break;
				}
			case 0x06:	//Converged, iteration limit reached, reconfigure limit not reached
				{
					//Check it
					reconfig_check = VoltageCheck();

					//check to see if we want to perform a reconfiguration, or let us go on our way
					if (reconfig_check)
					{
						ret_time = t0;			//We need to stay here, update as such
						NR_admit_change = true;	//Flag NR to perform an admittance update on next solver pass (not sure if this is absolutely needed or not yet)
						attempt_reconfigure = true;	//Flag to perform a reconfiguration
						reconfig_iterations = 0;	//Reset the iteration counter
					}
					else
					{
						ret_time = tret;	//Let us proceed
					}

					break;
				}
			case 0x07:	//Converged, iteration limit reached, reconfigure limit reached
				{
					ret_time = tret;	//Converged and our last attempt, call it good and proceed
					break;
				}
		}//Switch end
	}
		
	//Update previous timestep info
	prev_time = t0;

	return ret_time;	//Return where we want to go.  If >t0, NR will force us back anyways
}

//Function to create the connectivity matrix (called by swing during NR initializations)
void restoration::CreateConnectivity(void)
{
	unsigned int indexx,indexy;

	//First dimension allocation
	Connectivity_Matrix = (int**)gl_malloc(NR_bus_count * sizeof(int *));
	if (Connectivity_Matrix==NULL)
	{
		GL_THROW("Restoration: memory allocation failure for connectivity table");
		/*	TROUBLESHOOT
		This is a bug.  GridLAB-D failed to allocate memory for the connectivity table for the
		restoration device.  Please submit this bug and your code.
		*/
	}

	//Second dimension allocation
	for (indexx=0; indexx<NR_bus_count; indexx++)
	{
		Connectivity_Matrix[indexx] = (int*)malloc(NR_bus_count * sizeof(int));
		if (Connectivity_Matrix[indexx]==NULL)
		{
			GL_THROW("Restoration: memory allocation failure for connectivity table");
			/*	TROUBLESHOOT
			This is a bug.  GridLAB-D failed to allocate memory for the connectivity table for the
			restoration device.  Please submit this bug and your code.
			*/
		}
	}

	//Zero everything
	for (indexx=0; indexx<NR_bus_count; indexx++)
	{
		for (indexy=0; indexy<NR_bus_count; indexy++)
		{
			Connectivity_Matrix[indexx][indexy] = 0;
		}
	}
}

//Function to populate connectivity matrix entries (called during NR link initializations
void restoration::PopulateConnectivity(int frombus, int tobus, OBJECT *linkingobj)
{
	int link_type;

	//Determine what type of indicator we'll put in
	if (gl_object_isa(linkingobj,"switch","powerflow"))
	{
		link_type = CONN_SWITCH;
	}
	else if (gl_object_isa(linkingobj,"fuse","powerflow"))
	{
		link_type = CONN_FUSE;
	}
	else	//Must be a "normal" line then
	{
		link_type = CONN_LINE;
	}

	//Populate the connectivity matrix - both directions
	Connectivity_Matrix[frombus][tobus] = link_type;
	Connectivity_Matrix[tobus][frombus] = link_type;
}

//Function to check voltages and determine if a reconfiguration is necessary
bool *restoration::VoltageCheck(void)
{
	/* ------------ Array access testing ------------- */
		//Voltages would be checked here, just like presynch.  Replication of presync print for testing

		//print first voltage for now - this is just an example of obtaining the voltages (for check)
		printf("Volt Check Post - %f %f\n",NR_busdata[0].V[1].Re(),NR_busdata[0].V[1].Im());

	/* ---------- End array access testing ------------ */

	//Set to always return false right now
	return false;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: restoration
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_restoration(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(restoration::oclass);
		if (*obj!=NULL)
		{
			restoration *my = OBJECTDATA(*obj,restoration);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("%s %s (id=%d): %s", (*obj)->name?(*obj)->name:"unnamed", (*obj)->oclass->name, (*obj)->id, msg);
		return 0;
	}
	return 1;
}

EXPORT int init_restoration(OBJECT *obj, OBJECT *parent)
{
	try {
			return OBJECTDATA(obj,restoration)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("%s %s (id=%d): %s", obj->name?obj->name:"unnamed", obj->oclass->name, obj->id, msg);
		return 0;
	}
	return 1;
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_restoration(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	restoration *pObj = OBJECTDATA(obj,restoration);
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
	}
	catch (char *msg)
	{
		gl_error("restoration %s (%s:%d): %s", obj->name, obj->oclass->name, obj->id, msg);
	}
	catch (...)
	{
		gl_error("restoration %s (%s:%d): unknown exception", obj->name, obj->oclass->name, obj->id);
	}
	return TS_INVALID; /* stop the clock */
}

EXPORT int isa_restoration(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,restoration)->isa(classname);
}

/**@}**/
