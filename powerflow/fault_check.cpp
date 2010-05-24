/** $Id: fault_check.cpp 2009-11-09 15:00:00Z d3x593 $
	Copyright (C) 2009 Battelle Memorial Institute
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>

using namespace std;

#include "fault_check.h"
#include "restoration.h"
#include "solver_nr.h"

//////////////////////////////////////////////////////////////////////////
// fault_check CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* fault_check::oclass = NULL;
CLASS* fault_check::pclass = NULL;

fault_check::fault_check(MODULE *mod) : powerflow_object(mod)
{
	if(oclass == NULL)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod,"fault_check",sizeof(fault_check),PC_BOTTOMUP);
		if(oclass == NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
		
		if(gl_publish_variable(oclass,
			PT_enumeration, "check_mode", PADDR(fcheck_state),PT_DESCRIPTION,"Frequency of fault checks",
				PT_KEYWORD, "SINGLE", SINGLE,
				PT_KEYWORD, "ONCHANGE", ONCHANGE,
				PT_KEYWORD, "ALL", ALLT,
			PT_char1024,"output_filename",PADDR(output_filename),
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int fault_check::isa(char *classname)
{
	return strcmp(classname,"fault_check")==0;
}

int fault_check::create(void)
{
	int result = powerflow_object::create();
	prev_time = 0;
	phases = PHASE_A;	//Arbitrary - set so powerflow_object shuts up (library doesn't grant us access to synch)

	fcheck_state = SINGLE;	//Default to only one check
	output_filename[0] = '\0';	//Empty file name

	return result;
}

int fault_check::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	FILE *FPoint;

	if (solver_method == SM_NR)
	{
		//Set the rank to be 1 below swing - lets it execute before NR on synch
		gl_set_rank(obj,5);	//swing-1
	}
	else
	{
		GL_THROW("Fault checking object is only valid for the Newton-Raphson solver at this time.");
		/*  TROUBLESHOOT
		The fault_check object only works with the Newton-Raphson powerflow solver at this time.
		Other solvers may be integrated at a future date.
		*/
	}

	//By default, kill the file - open and close it
	if (output_filename[0] != '\0')	//Make sure it isn't empty
	{
		FPoint = fopen(output_filename,"wt");
		fclose(FPoint);
	}

	return powerflow_object::init(parent);
}

TIMESTAMP fault_check::sync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::sync(t0);
	restoration *rest_obj;
	unsigned int index;
	bool perform_check;
		
	if (prev_time == 0)	//First run - see if restoration exists (we need it for now)
	{
		if (restoration_object==NULL)
		{
			gl_warning("Restoration object not detected!");	//Put down here because the variable may not be populated in time for init
			/*  TROUBLESHOOT
			The fault_check object can use a restoration object in the system.  If the restoration object is present,
			unsuccessful node support checks will call the reconfiguration.
			*/
		}

		//Create our node reference vector - one for each bus
		Supported_Nodes = (int**)gl_malloc(NR_bus_count*sizeof(int));
		if (Supported_Nodes == NULL)
		{
			GL_THROW("fault_check: node status vector allocation failure");
			/*  TROUBLESHOOT
			The fault_check object has failed to allocate the node information vector.  Please try
			again and if the problem persists, submit your code and a bug report via the trac website.
			*/
		}

		//Now create the per-phase reference vector
		for (index=0; index<NR_bus_count; index++)
		{
			Supported_Nodes[index] = (int*)gl_malloc(3*sizeof(int));

			if (Supported_Nodes[index] == NULL)
			{
				GL_THROW("fault_check: node status vector allocation failure");
				//Defined above
			}
		}
	}//end 0th run

	if (NR_cycle==false)	//Getting ready to do a solution, we should see check for "unsupported" systems
	{
		if ((fcheck_state == SINGLE) && (prev_time == 0))	//Single only occurs on first iteration
		{
			perform_check = true;	//Flag for a check
		}//end single check
		else if ((fcheck_state == ONCHANGE) && (NR_admit_change == true))	//Admittance change has been flagged
		{
			perform_check = true;	//Flag the check
		}//end onchange check
		else if (fcheck_state == ALLT)	//Must be every iteration then
		{
			perform_check = true;	//Flag the check
		}
		else	//Doesn't fit one of the above
		{
			perform_check = false;	//Flag not-a-check
		}

		if (perform_check)	//Each "check" is identical - split out here to avoid 3x replication
		{
			//See if restoration is present - if not, proceed without it
			if (restoration_object != NULL)
			{
				//Map to the restoration object
				rest_obj = OBJECTDATA(restoration_object,restoration);

				//Call the connectivity check
				support_check(0,rest_obj->populate_tree);

				//Go through the list now - if something is unsupported, call a reconfiguration
				for (index=0; index<NR_bus_count; index++)
				{
					if ((Supported_Nodes[index][0] == 0) || (Supported_Nodes[index][1] == 0) || (Supported_Nodes[index][2] == 0))
					{
						if (output_filename[0] != '\0')	//See if there's an output
						{
							write_output_file(t0);	//Write it
						}

						gl_warning("Unsupported phase on node %s",NR_busdata[index].name);	//Only reports the first one

						rest_obj->Perform_Reconfiguration(OBJECTHDR(this),t0);	//Request a reconfiguration

						break;	//Get us out of this loop, only need to detect a solitary failure
					}
				}
			}
			else	//No restoration
			{
				//Call the connectivity check
				support_check(0,false);

				//Parse the list - see if anything is broken
				for (index=0; index<NR_bus_count; index++)
				{
					if ((Supported_Nodes[index][0] == 0) || (Supported_Nodes[index][1] == 0) || (Supported_Nodes[index][2] == 0))
					{
						if (output_filename[0] != '\0')	//See if there's an output
						{
							write_output_file(t0);	//Write it
						}

						GL_THROW("Unsupported phase on node %s",NR_busdata[index].name);
						/*  TROUBLESHOOT
						An unsupported connection was found on the indicated bus in the system.  Since reconfiguration
						is not enabled, the solver will fail here on the next iteration, so the system is broken early.
						*/
					}
				}
			}//no restoration
		}//End check
	}//End NR_cycle == false

	//Update previous timestep info
	prev_time = t0;

	return tret;	//Return where we want to go.  If >t0, NR will force us back anyways
}


void fault_check::search_links(unsigned int node_int)
{
	unsigned int index, temp_child_idx, indexb;
	bool both_handled, from_val, first_resto, proceed_in;
	int branch_val;
	BRANCHDATA temp_branch;
	restoration *rest_object;
	unsigned char work_phases;

	//Loop through the connectivity and populate appropriately
	for (index=0; index<NR_busdata[node_int].Link_Table_Size; index++)	//parse through our connected link
	{
		temp_branch = NR_branchdata[NR_busdata[node_int].Link_Table[index]];	//Get connecting link information

		first_resto = false;		//Flag that restoration hasn't gone off
		proceed_in = false;			//Flag that we need to go the next link in

		//Check for no phase condition
		if ((temp_branch.phases & 0x07) != 0x00)
		{
			for (indexb=0; indexb<3; indexb++)	//Handle phases
			{
				work_phases = 0x04 >> indexb;	//Pull off the phase reference

				if ((temp_branch.phases & work_phases) == work_phases)	//We are of the proper phase
				{
					both_handled = false;	//Reset flag

					//See which end we are, and if the other end has been handled
					if (temp_branch.from == node_int)	//We're the from
					{
						from_val = true;	//Flag us as the from end (so we don't have to check it again later)
					}
					else	//Must be the to
					{
						from_val = false;	//Flag us as the to end (so we don't have to check it again later)
					}

					//See if both sides of this link are already set - if so, don't bother going back in
					if ((Supported_Nodes[temp_branch.to][indexb]==1) && (Supported_Nodes[temp_branch.from][indexb]==1))
						both_handled=true;

					//If not handled, proceed with logic-ness
					if (both_handled==false)
					{
						//Figure out the indexing so we can tell what we are
						if (from_val)	//From end
						{
							branch_val = temp_branch.to;

							//See if our FROM end is properly supported (paranoia check)
							if (Supported_Nodes[temp_branch.from][indexb] == 1)
							{
								proceed_in = true;		//Flag to progress when done
							}
							else
							{
								continue;		//Somehow is checking, but neither of our ends are set - skip us for now
							}
						}
						else	//To end
						{
							branch_val = temp_branch.from;

							//See if our TO end is properly supported (paranoia check)
							if (Supported_Nodes[temp_branch.to][indexb] == 1)
							{
								proceed_in = true;		//Flag to progress when done
							}
							else
							{
								continue;		//Somehow is checking, but neither of our ends are set - skip us for now
							}
						}

						//Pull out the link to the restoration object, if it exists
						if (restoration_object != NULL)
						{
							//Link to the restoration object
							rest_object = OBJECTDATA(restoration_object,restoration);

							//Populate the parent/child tree if wanted
							if ((rest_object->populate_tree == true) && (first_resto == false))
							{
								first_resto = true;		//Flag that first pass has happened on this branch

								if (from_val)	//we're the from end
								{
									//This means we are the to end's parent - just write this.  If mesh or multi-sourced, this will just get overwritten
									NR_busdata[temp_branch.to].Parent_Node = temp_branch.from;

									//This also means the to end is our child
									temp_child_idx = NR_busdata[temp_branch.from].Link_Table_Size;
									if (NR_busdata[temp_branch.from].Child_Node_idx < temp_child_idx)	//Still in a valid range
									{
										if (rest_object ->Connectivity_Matrix[temp_branch.from][ temp_branch.to] != 3)	// if the branch is not a switch connection
										{
											NR_busdata[temp_branch.from].Child_Nodes[NR_busdata[temp_branch.from].Child_Node_idx] = temp_branch.to;	//Store the to value
											NR_busdata[temp_branch.from].Child_Node_idx++;	//Increment the pointer
										}
										else if ((rest_object ->Connectivity_Matrix[temp_branch.from][ temp_branch.to] == 3) && (*temp_branch.status == true)) //  if the branch is a closed switch connection
										{
											NR_busdata[temp_branch.from].Child_Nodes[NR_busdata[temp_branch.from].Child_Node_idx] = temp_branch.to;	//Store the to value
											NR_busdata[temp_branch.from].Child_Node_idx++;	//Increment the pointer
										}
										else	// if the branch is open switch, the child is not populated for this branch
										{
											 temp_child_idx -= 1;
										}
									}
									else
									{
										GL_THROW("NR: Too many children were attempted to be populated!");
										/*  TROUBLESHOOT
										While populating the tree structure for the restoration module, a memory allocation limit
										was exceeded.  Please ensure no new nodes have been introduced into the system and try again.  If
										the error persists, please submit your code and a bug report using the trac website.
										*/
									}
								}
								//To end, we don't want to do anything (should all be handled from from end)
							}//Populate tree
						}//End restoration object present

						//Flag us as connected
						Supported_Nodes[branch_val][indexb] = 1;

					}//End both not handled (work to be done)
				}//End are a proper phase
			}//End phase testloop

			if (proceed_in)
			{
				//Recursion!!
				search_links(branch_val);
			}
		}//End has phases test loop
	}//End link table loop
}

void fault_check::support_check(unsigned int swing_node_int,bool rest_pop_tree)
{
	unsigned int index;

	//Reset the node status list
	reset_support_check();

	//See if we want the tree structure populated
	if (rest_pop_tree == true)
	{
		for (index=0; index<NR_bus_count; index++)
			NR_busdata[index].Child_Node_idx = 0;	//Zero the child object index
	}

	//Swing node has support - assume it is three phase (all three don't necessarily have to be used)
	Supported_Nodes[swing_node_int][0] = Supported_Nodes[swing_node_int][1] = Supported_Nodes[swing_node_int][2] = 1;

	//Call the node link-erator (node support check) - call it on the swing, the details are handled inside
	search_links(swing_node_int);
}

void fault_check::reset_support_check(void)
{
	unsigned int index;

	//Reset the node - 0 = unsupported, 1 = supported (not populated here), 2 = N/A (no phase there)
	for (index=0; index<NR_bus_count; index++)
	{
		if ((NR_busdata[index].origphases & 0x04) == 0x04)	//Phase A
		{
			Supported_Nodes[index][0] = 0;	//Flag as unsupported initially
		}
		else
		{
			Supported_Nodes[index][0] = 2;	//N/A phase
		}

		if ((NR_busdata[index].origphases & 0x02) == 0x02)	//Phase B
		{
			Supported_Nodes[index][1] = 0;	//Flag as unsupported initially
		}
		else
		{
			Supported_Nodes[index][1] = 2;	//N/A phase
		}

		if ((NR_busdata[index].origphases & 0x01) == 0x01)	//Phase C
		{
			Supported_Nodes[index][2] = 0;	//Flag as unsupported initially
		}
		else
		{
			Supported_Nodes[index][2] = 2;	//N/A phase
		}
	}
}

void fault_check::write_output_file(TIMESTAMP tval)
{
	unsigned int index;
	bool headerwritten = false;
	char phase_outs;

	//open the file
	FILE *FPOutput = fopen(output_filename,"at");

	for (index=0; index<NR_bus_count; index++)	//Loop through all bus values - find the unsupported section
	{
		//Put the phases into an easier to read format
		phase_outs = 0x00;

		if (Supported_Nodes[index][0] == 0)	//Check A
			phase_outs |= 0x04;

		if (Supported_Nodes[index][1] == 0)	//Check B
			phase_outs |= 0x02;

		if (Supported_Nodes[index][2] == 0)	//Check C
			phase_outs |= 0x01;

		if (phase_outs != 0x00)	//Anything unsupported?
		{
			//See if the header's been written
			if (headerwritten == false)
			{
				fprintf(FPOutput,"Unsupported at timestamp %lld:\n\n",tval);
				headerwritten = true;	//Flag it as written
			}

			//Figure out the "unsupported" structure
			switch (phase_outs) {
				case 0x01:	//Only C
					{
						fprintf(FPOutput,"Phase C on node %s\n",NR_busdata[index].name);
						break;
					}
				case 0x02:	//Only B
					{
						fprintf(FPOutput,"Phase B on node %s\n",NR_busdata[index].name);
						break;
					}
				case 0x03:	//B and C unsupported
					{
						fprintf(FPOutput,"Phases B and C on node %s\n",NR_busdata[index].name);
						break;
					}
				case 0x04:	//Only A
					{
						fprintf(FPOutput,"Phase A on node %s\n",NR_busdata[index].name);
						break;
					}
				case 0x05:	//A and C unsupported
					{
						fprintf(FPOutput,"Phases A and C on node %s\n",NR_busdata[index].name);
						break;
					}
				case 0x06:	//A and B unsupported
					{
						fprintf(FPOutput,"Phases A and B on node %s\n",NR_busdata[index].name);
						break;
					}
				case 0x07:	//All three unsupported
					{
						fprintf(FPOutput,"Phases A, B, and C on node %s\n",NR_busdata[index].name);
						break;
					}
				default:	//How'd we get here?
					{
						GL_THROW("Error parsing unsupported phases on node %s",NR_busdata[index].name);
						/*  TROUBLESHOOT
						The output file writing routine for the fault_check object encountered a problem
						determining which phases are unsupported on the indicated node.  Please try again.
						If the error persists, submit your code and a bug report using the trac website.
						*/
					}
			}//End case
		}//end unsupported
	}//end bus traversion

	fprintf(FPOutput,"\n\n\n");	//Put some blank lines between time entries

	//Close the file
	fclose(FPOutput);
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: fault_check
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_fault_check(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(fault_check::oclass);
		if (*obj!=NULL)
		{
			fault_check *my = OBJECTDATA(*obj,fault_check);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("%s %s (id=%d): %s", (*obj)->name?(*obj)->name:"unnamed", (*obj)->oclass->name, (*obj)->id, msg);
		return 0;
	}
	return 1;
}

EXPORT int init_fault_check(OBJECT *obj, OBJECT *parent)
{
	try {
			return OBJECTDATA(obj,fault_check)->init(parent);
	}
	catch (const char *msg)
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
EXPORT TIMESTAMP sync_fault_check(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	fault_check *pObj = OBJECTDATA(obj,fault_check);
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
	catch (const char *msg)
	{
		gl_error("fault_check %s (%s:%d): %s", obj->name, obj->oclass->name, obj->id, msg);
	}
	catch (...)
	{
		gl_error("fault_check %s (%s:%d): unknown exception", obj->name, obj->oclass->name, obj->id);
	}
	return TS_INVALID; /* stop the clock */
}

EXPORT int isa_fault_check(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,fault_check)->isa(classname);
}

/**@}**/
