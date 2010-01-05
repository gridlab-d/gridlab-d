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
#include "fault_check.h"
#include "solver_nr.h"
#include "node.h"
#include "line.h"
#include "switch_object.h"

//////////////////////////////////////////////////////////////////////////
// restoration CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* restoration::oclass = NULL;
CLASS* restoration::pclass = NULL;

restoration::restoration(MODULE *mod) : powerflow_library(mod)
{
	if(oclass == NULL)
	{
		oclass = gl_register_class(mod,"restoration",sizeof(restoration),0x00);
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
	*configuration_file = NULL;
	prev_time = 0;

	reconfig_attempts = 0;
	reconfig_iter_limit = 0;

	return 1;
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
				GL_THROW("Infinite reconfigurations set.");
				/*  TROUBLESHOOT
				The restoration object can have the number of reconfiguration per timestep set via the
				reconfig_attempts property.  If not set, or set to 0, the system will perform reconfigurations
				forever and lock up the computer (if an acceptable solution is never met).
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

	return 1;
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
	else if (gl_object_isa(linkingobj,"transformer","powerflow") || gl_object_isa(linkingobj,"regulator","powerflow"))	//Regulators and transformers lumped together for now.  Both essentially the same thing
	{
		link_type = CONN_XFRM;
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
bool restoration::VoltageCheck(void)
{
	OBJECT *TempObj;			//Variables for nominal voltage example.  May be changed/removed
	node *TempNode;
	double ex_nom_volts;
	/* ------------ Array access testing ------------- */
		//Voltages would be checked here

		//This is an example of pulling the nominal voltage from the first item in NR_busdata (this is the swing bus, but the principle holds)
			//Get the object information
			TempObj = gl_get_object(NR_busdata[0].name);

			//Associate this with a node object
			TempNode = OBJECTDATA(TempObj,node);

			//Extract the nominal voltage value
			ex_nom_volts = TempNode->nominal_voltage;

		//print first voltage for now - this is just an example of obtaining the voltages (for check)
		printf("Volt Check Post - %f %f\n",NR_busdata[0].V[1].Re(),NR_busdata[0].V[1].Im());

	/* ---------- End array access testing ------------ */

	//Set to always return true right now - indicates all voltages passed
	return true;
}

//Function to perform the reconfiguration - functionalized so fault_check can call it before the NR solver goes off (and call the solver within)
void restoration::Perform_Reconfiguration(OBJECT *faultobj, TIMESTAMP t0)
{
	OBJECT *tempobj;
	DATETIME CurrTimeVal;
	switch_object *SwitchDevice;
	line *LineDevice;
	line_configuration *LineConfig;
	triplex_line_configuration *TripLineConfig;
	node *TempNode;
	overhead_line_conductor *OHConductor;
	underground_line_conductor *UGConductor;
	triplex_line_conductor *TLConductor;
	double max_volt_error;
	int64 pf_result;
	int fidx, tidx, pidx;
	double cont_rating;
	bool pf_bad_computations, pf_valid_solution, fc_all_supported, good_solution, rating_exceeded;
	complex temp_current[3];

	if (t0 != prev_time)	//Perform new timestep updates
	{
		reconfig_number = 0;	//Reset number of reconfigurations that have occurred
		prev_time = t0;			//Update time value
	}

	//Get the current time into a manageable structure - to be used later
	 gl_localtime(t0, &CurrTimeVal);

	//Pull out fault check object info
	fault_check *FaultyObject = OBJECTDATA(faultobj,fault_check);

	//Find the maximum voltage error for the NR solver - use the swing bus (since normal NR solver just does that)
	tempobj = gl_get_object(NR_busdata[0].name);	//Get object link - 0 should be swing bus
	TempNode = OBJECTDATA(tempobj,node);			//Get the node link

	max_volt_error = TempNode->nominal_voltage * default_maximum_voltage_error;	//Calculate the maximum voltage error

	//Now we go into two while loops so reconfigurations are attempted until a solution is found (or limits are reached)
	while (reconfig_number < reconfig_attempts)	//Reconfiguration count loop
	{
		//Reset tracking variables
		good_solution = false;
		reconfig_iterations=0;	//Reset number of iterations that have occurred on this attempt

		//If we've been called, there is a fault somewhere (fault_check has summoned us)
		/***********************

		This is where the list of candidate switching operations would be generated/found - likely based off of the Connectivity_Matrix

		***********************/

		/**********************
		
		This is where a switch status would be changed based on the reconfiguration.  For the purposes of an example, I will just find all
		switches in the system and close them

		***********************/

			//EXAMPLE CODE - Find all switches in the system and close them
			//Very crude, just find every switch (possibly even duplication since they exist twice in connectivity) and close them
			//May need refinement, but is only here to show a quick example
			unsigned int indexx, indexy, indexz;
			int conval;
			BRANCHDATA temp_branch;

			for (indexx=0; indexx<NR_bus_count; indexx++)
			{
				for (indexy=0; indexy<NR_bus_count; indexy++)
				{
					conval = Connectivity_Matrix[indexx][indexy];

					//See if it is a switch
					if (conval==CONN_SWITCH)
					{
						//Find the switch based on its ends (search the connecting nodes shorter link list
						for (indexz=0; indexz<NR_busdata[indexx].Link_Table_Size; indexz++)
						{
							temp_branch = NR_branchdata[NR_busdata[indexx].Link_Table[indexz]];	//Get current "link of interest" info

							if (((temp_branch.from==indexx) && (temp_branch.to==indexy)) || ((temp_branch.from==indexy) && (temp_branch.to==indexx)))	//Check both ways so we can fail out in a pickle
							{


								break;	//Found what we want, so no need to continue looping
							}
						}

						if (indexz==NR_busdata[indexx].Link_Table_Size)	//Made it all the way to the end :(
						{
							GL_THROW("A switch object could not be found in reconfiguration");	//This error indicates Connectivity_Matrix flagged a switch, but the node has no record of it
							//Troubleshoot to be added if kept in here
						}
						else	//Proceed with the closing
						{
							//Get the switch's object linking
							tempobj = gl_get_object(temp_branch.name);

							//Now pull it into switch properties
							SwitchDevice = OBJECTDATA(tempobj,switch_object);

							//Close it
							SwitchDevice->set_switch(true);	//True = closed, false = open
						}
					}//end it is a switch
				}//end column traversion
			}//end row traversion

			/*******************
			End Simple example
			*******************/

		//Check to see if everything is supported again - not sure if this will be necessary or not
		FaultyObject->support_check(0);		//Start a new support check

		//See if anything is unsupported
		fc_all_supported = true;
		for (indexx=0; indexx<NR_bus_count; indexx++)
		{
			if (FaultyObject->Supported_Nodes[indexx]==0)	//No Support
			{
				fc_all_supported = false;	//Flag as a defect
				gl_verbose("Reconfiguration attempt failed - unsupported nodes were still located");
				/* TROUBLESHOOT
				After performing the reconfiguration logic, some nodes are still unsupported.  This is considered a failed
				reconfiguration, so another will be attempted.
				*/
				break;						//Only takes one failure to need to get out of here
			}
		}

		//Set the pf solution flag
		pf_valid_solution = false;

		//Re-solve powerflow at this point - assuming the system appears valid
		while ((reconfig_iterations < reconfig_iter_limit) && (fc_all_supported==true))	//Only let powerflow iterate so many times
		{
			pf_bad_computations = false;	//Reset singularity checker

			//Perform NR solver
			pf_result = solver_nr(NR_bus_count, NR_busdata, NR_branch_count, NR_branchdata, max_volt_error, &pf_bad_computations);

			//De-flag any admittance changes (so other iterations don't take longer
			NR_admit_change = false;

			if (pf_bad_computations==true)	//Singular attempt, fail - not sure how it will get here with the fault_check call above, but added for completeness
			{
				pf_valid_solution = false;

				gl_verbose("Restoration attempt failed - singular system matrix.");
				/*  TROUBLESHOOT
				The restoration object has attempted a reconfiguration that did not restore the system.
				Another reconfiguration will be attempted.
				*/

				break;	//Get out of this while, no sense solving a singular matrix more than once
			}
			else if (pf_result<0)	//Failure to converge, but just numerical issues, not singular.  Do another pass
			{
				pf_valid_solution = false;	//Set it just to be safe

				reconfig_iterations++;	//Increment the powerflow iteration counter
			}
			else	//Must be a successful solution then
			{
				pf_valid_solution = true;

				break;	//Get out of the while, we've solved successfully
			}
		}//end while for pf iterations

		//See if it was a valid solution, if so, check system conditions (voltages/currents)
		if (pf_valid_solution==true)
		{
			//Check voltages - handled in the other function (not sure what we want to do here - presumably make sure it isn't too low or something
			good_solution = VoltageCheck();

			//Check branch currents, if something is a line (switches, xformers, etc. unhandled at this time)
			if (good_solution==true)	//Voltage passed, onward
			{
				for (indexx=0; indexx<NR_branch_count; indexx++)
				{
					//See if it is a line
					fidx=NR_branchdata[indexx].from;
					tidx=NR_branchdata[indexx].to;

					if (Connectivity_Matrix[fidx][tidx]==CONN_LINE)
					{
						cont_rating = 0.0;		//Reset rating variable to zero initially

						//Grab the object header
						tempobj = gl_get_object(NR_branchdata[indexx].name);

						//Get its line version
						LineDevice = OBJECTDATA(tempobj,line);

						//Calculate the current in for the deivce
						LineDevice->calc_currents(temp_current);

						//Find the conductor limits
						if (gl_object_isa(tempobj,"triplex_line","powerflow"))	//See if it is triplex, that gets handled slightly different
						{
							//Get the line configuration
							TripLineConfig = OBJECTDATA(LineDevice->configuration,triplex_line_configuration);

							//Grab the conductor (assumes 1 and 2 are the same - neutral is unchecked)
							TLConductor = OBJECTDATA(TripLineConfig->phaseA_conductor,triplex_line_conductor);

							//Grab the appropriate continuous rating - Summer is assumed to be April - September (6 months each)
							if ((CurrTimeVal.month >= 4) && (CurrTimeVal.month <=9))	//Summer
							{
								cont_rating = TLConductor->summer.continuous;
							}
							else	//Winter
							{
								cont_rating = TLConductor->winter.continuous;
							}
						}//end triplex
						else //Must be UG or OH
						{
							//Get the line configuration
							LineConfig = OBJECTDATA(LineDevice->configuration,line_configuration);

							//Pick a phase that exists.  Assumes all wires are the same on the line
							if ((LineDevice->phases & PHASE_A) == PHASE_A)
							{
								pidx=0;
							}
							else if ((LineDevice->phases & PHASE_B) == PHASE_B)
							{
								pidx=1;
							}
							else	//Must be C only
							{
								pidx=2;
							}

							//See which type of conductor we need to use
							if (gl_object_isa(tempobj,"overhead_line","powerflow"))
							{
								if (pidx==0)	//A
								{
									OHConductor = OBJECTDATA(LineConfig->phaseA_conductor,overhead_line_conductor);
								}
								else if (pidx==1)	//B
								{
									OHConductor = OBJECTDATA(LineConfig->phaseB_conductor,overhead_line_conductor);
								}
								else	//Must be C
								{
									OHConductor = OBJECTDATA(LineConfig->phaseC_conductor,overhead_line_conductor);
								}

								//Grab the appropriate continuous rating - Summer is assumed to be April - September (6 months each)
								if ((CurrTimeVal.month >= 4) && (CurrTimeVal.month <=9))	//Summer
								{
									cont_rating = OHConductor->summer.continuous;
								}
								else	//Winter
								{
									cont_rating = OHConductor->winter.continuous;
								}
							}//end OH
							else	//Must be UG
							{
								if (pidx==0)	//A
								{
									UGConductor = OBJECTDATA(LineConfig->phaseA_conductor,underground_line_conductor);
								}
								else if (pidx==1)	//B
								{
									UGConductor = OBJECTDATA(LineConfig->phaseB_conductor,underground_line_conductor);
								}
								else	//Must be C
								{
									UGConductor = OBJECTDATA(LineConfig->phaseC_conductor,underground_line_conductor);
								}

								//Grab the appropriate continuous rating - Summer is assumed to be April - September (6 months each)
								if ((CurrTimeVal.month >= 4) && (CurrTimeVal.month <=9))	//Summer
								{
									cont_rating = UGConductor->summer.continuous;
								}
								else	//Winter
								{
									cont_rating = UGConductor->winter.continuous;
								}
							}//end UG
						}//end UG/OH line

						//Reset rating variable
						rating_exceeded = false;

						if (cont_rating != 0.0)	//Zero rating is taken to mean ratings aren't going to be examined
						{
							//See if any measurements exceeded the ratings
							if ((LineDevice->phases & PHASE_S) == PHASE_S)	//Triplex line
							{
								rating_exceeded |= (temp_current[0].Mag() > cont_rating);
								rating_exceeded |= (temp_current[1].Mag() > cont_rating);
							}
							else	//Normal lines
							{
								if ((LineDevice->phases & PHASE_A) == PHASE_A)
								{
									rating_exceeded |= (temp_current[0].Mag() > cont_rating);
								}

								if ((LineDevice->phases & PHASE_B) == PHASE_B)
								{
									rating_exceeded |= (temp_current[1].Mag() > cont_rating);
								}

								if ((LineDevice->phases & PHASE_C) == PHASE_C)
								{
									rating_exceeded |= (temp_current[2].Mag() > cont_rating);
								}
							}
						}//end cont rating not 0.0

						if (rating_exceeded == true)	//Exceeded a limit
						{
							good_solution = false;	//Flag as bad solution
							break;					//Get us out of the branch current checks (only takes 1 bad one)
						}
					}//end it is a line
				}//end branch current checks
			}//end voltage checks passed
			//Defaulted else - voltage check didn't pass
		}//end valid pf solution checks

		//Check to see if we're done, or need to reconfigure again
		if ((good_solution == true) && (pf_valid_solution == true))
		{
			break;	//Get us out
		}

		reconfig_number++;	//Increment the reconfiguration counter
	}//end reconfiguration while loop

	if (reconfig_number == reconfig_attempts)
	{
		GL_THROW("Feeder reconfiguration failed.");
		/*  TROUBLESHOOT
		Attempts at reconfiguring the feeder have failed.  An unsolvable condition was reached
		or the reconfig_attempts limit was set too low.  Please modify the test system or increase
		the value of reconfig_attempts.
		*/
	}
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
	return 0;
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
EXPORT TIMESTAMP sync_restoration(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	return TS_NEVER;
}
EXPORT int isa_restoration(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,restoration)->isa(classname);
}

/**@}**/
