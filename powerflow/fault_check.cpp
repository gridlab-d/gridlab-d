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
		if (oclass==NULL)
			throw "unable to register class fault_check";
		else
			oclass->trl = TRL_DEMONSTRATED;
		
		if(gl_publish_variable(oclass,
			PT_enumeration, "check_mode", PADDR(fcheck_state),PT_DESCRIPTION,"Frequency of fault checks",
				PT_KEYWORD, "SINGLE", SINGLE,
				PT_KEYWORD, "ONCHANGE", ONCHANGE,
				PT_KEYWORD, "ALL", ALLT,
			PT_char1024,"output_filename",PADDR(output_filename),
			PT_bool,"reliability_mode",PADDR(reliability_mode),
			PT_object,"eventgen_object",PADDR(rel_eventgen),
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
			if (gl_publish_function(oclass,"reliability_alterations",(FUNCTIONADDR)powerflow_alterations)==NULL)
				GL_THROW("Unable to publish remove from service function");
			if (gl_publish_function(oclass,"handle_sectionalizer",(FUNCTIONADDR)handle_sectionalizer)==NULL)
				GL_THROW("Unable to publish sectionalizer special function");
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

	reliability_mode = false;	//By default, we find errors and fail - if true, dumps log, but continues on its merry way

	rel_eventgen = NULL;		//No object linked by default

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

	//Register us in the global - so faults know who they gonna call
	if (fault_check_object == NULL)	//Make sure we're the only one
	{
		fault_check_object = obj;	//Link us up!
	}
	else
	{
		GL_THROW("Only one fault_check object is supported at this time");
		/*  TROUBLESHOOT
		At this time, a .GLM file can only contain one fault_check object.  Future implementations
		may change this.  Please restrict yourself to one fault_check object in the mean time.
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
		Supported_Nodes = (int**)gl_malloc(NR_bus_count*sizeof(int*));
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

		//Replicate the above if reliability is running
		if (reliability_mode == true)
		{
			//Create our node reference vector - one for each bus
			Alteration_Nodes = (char*)gl_malloc(NR_bus_count*sizeof(char));
			if (Alteration_Nodes == NULL)
			{
				GL_THROW("fault_check: node alteration status vector allocation failure");
				/*  TROUBLESHOOT
				The fault_check object has failed to allocate the node alteration information vector.  Please try
				again and if the problem persists, submit your code and a bug report via the trac website.
				*/
			}
		}//End reliability mode allocing
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

						//See what mode we are in
						if (reliability_mode == false)
						{
							GL_THROW("Unsupported phase on node %s",NR_busdata[index].name);
							/*  TROUBLESHOOT
							An unsupported connection was found on the indicated bus in the system.  Since reconfiguration
							is not enabled, the solver will fail here on the next iteration, so the system is broken early.
							*/
						}
						else	//Must be in reliability mode
						{
							gl_warning("Unsupported phase on node %s",NR_busdata[index].name);
							/*  TROUBLESHOOT
							An unsupported connection was found on the indicated bus in the system.  Since reliability
							is enabled, the solver will simply ignore the unsupported components for now.
							*/
						}
						break;	//Only need to do this once
					}
				}
			}//no restoration
		}//End check
	}//End NR_cycle == false

	//Update previous timestep info
	prev_time = t0;

	return tret;	//Return where we want to go.  If >t0, NR will force us back anyways
}


void fault_check::search_links(int node_int)
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

void fault_check::support_check(int swing_node_int,bool rest_pop_tree)
{
	unsigned int index;
	unsigned char phase_vals;

	//Reset the node status list
	reset_support_check();

	//See if we want the tree structure populated
	if (rest_pop_tree == true)
	{
		for (index=0; index<NR_bus_count; index++)
			NR_busdata[index].Child_Node_idx = 0;	//Zero the child object index
	}

	//Swing node has support - if the phase exists (changed for complete faults)
	for (index=0; index<3; index++)
	{
		phase_vals = 0x04 >> index;	//Set up phase value

		if ((NR_busdata[swing_node_int].phases & phase_vals) == phase_vals)	//Has this phase
			Supported_Nodes[swing_node_int][index] = 1;	//Flag it as supported
	}

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
	DATETIME temp_time;
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
				//Convert timestamp so readable
				gl_localtime(tval,&temp_time);

				fprintf(FPOutput,"Unsupported at timestamp %lld - %04d-%02d-%02d %02d:%02d:%02d =\n\n",tval,temp_time.year,temp_time.month,temp_time.day,temp_time.hour,temp_time.minute,temp_time.second);
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

	fprintf(FPOutput,"\n");	//Put some blank lines between time entries

	//Close the file
	fclose(FPOutput);
}

//Function to traverse powerflow and remove/restore now unsupported objects' phases (so NR solves happy)
void fault_check::support_check_alterations(int baselink_int, bool rest_mode)
{
	int base_bus_val;

	//See if the "faulting branch" is the swing node
	if (baselink_int == -99)	//Swing
		base_bus_val = 0;	//Swing is always 0th position
	else	//Not swing, see what our to side is
	{
		//Get to side
		base_bus_val = NR_branchdata[baselink_int].to;

		//Assuming radial, now make the system happy by removing/restoring unsupported phases
		if (rest_mode == true)	//Restoration
			NR_busdata[base_bus_val].phases |= (NR_branchdata[baselink_int].phases & 0x07);
		else	//Removal mode
			NR_busdata[base_bus_val].phases &= (NR_branchdata[baselink_int].phases & 0x07);
	}

	//Reset the alteration matrix
	reset_alterations_check();

	//First node is handled, one way or another
	Alteration_Nodes[base_bus_val] = 1;

	//See if the FROM side of our newly restored greatness is supported.  If it isn't, there's no point in proceeding
	if (rest_mode == true)	//Restoration
	{
		gl_verbose("Alterations support check called restoration on bus %s",NR_busdata[base_bus_val].name);

		if ((NR_busdata[base_bus_val].phases & 0x07) != 0x00)	//We have phase, means OK above us
		{
			//Recurse our way in - adjusted version of original search_links function above (but no storage, because we don't care now)
			support_search_links(base_bus_val, base_bus_val, rest_mode);
		}
		else
			return;	//No phases above us, so must be higher level fault.  No sense proceeding then
	}
	else	//Destructive!
	{
		gl_verbose("Alterations support check called removal on bus %s",NR_busdata[base_bus_val].name);

		//Recurse our way in - adjusted version of original search_links function above (but no storage, because we don't care now)
		support_search_links(base_bus_val, base_bus_val, rest_mode);
	}
}

//Recursive function to traverse powerflow and alter phases as necessary
//Based on search_links code from above
void fault_check::support_search_links(int node_int, int node_start, bool impact_mode)
{
	unsigned int index;
	bool both_handled, from_val;
	int branch_val, return_val;
	BRANCHDATA temp_branch;
	unsigned char work_phases, phase_restrictions,temp_phases;
	OBJECT *temp_obj;
	FUNCTIONADDR funadd = NULL;

	//Loop through the connectivity and populate appropriately
	for (index=0; index<NR_busdata[node_int].Link_Table_Size; index++)	//parse through our connected link
	{
		temp_branch = NR_branchdata[NR_busdata[node_int].Link_Table[index]];	//Get connecting link information

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

		if ((node_int == node_start) && (from_val == false))	//We're the TO side of the base node, Oh Noes!
		{
			Alteration_Nodes[temp_branch.from] = 1;	//Flag us to prevent future issues (not sure how they'd happen)
			continue;	//Nothing to do with this link, so I hereby render this iteration useless and proceed to skip it
		}
		else	//FROM side of any, or not the TO side as the base node
		{
			//See if both sides of this link are already set - if so, don't bother going back in
			if ((Alteration_Nodes[temp_branch.to]==1) && (Alteration_Nodes[temp_branch.from]==1))
				both_handled=true;
		}

		//If not handled, proceed with logic-ness
		if (both_handled==false)
		{
			//Figure out the indexing so we can tell what we are
			if (from_val)	//From end
			{
				branch_val = temp_branch.to;

				if (impact_mode == false)	//Removal time
				{
					//Make sure our FROM end is valid first - just in case
					if (Alteration_Nodes[temp_branch.from] == 1)
					{
						//Remove our phase portions - determine by our FROM end
						work_phases = NR_busdata[temp_branch.from].phases & 0x07;

						//See if we are split-phase
						if ((NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x80) == 0x80)
						{
							//See if any support exists
							if ((NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & work_phases) == 0x00)	//no longer any support
							{
								//Just remove it all
								NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases = 0x00;
							}
							//Defaulted else - do nothing
						}
						else	//Not split-phase, normal - continue
						{
							//Remove components - USBs are typically node oriented, so they aren't included here
							NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases &= work_phases;
						}

						//Now apply the phases on the TO end of this branch - first off, get base phases - D is omitted (D unsupported for now)
						work_phases = NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x07;

						//See if the line is a SPCT or Triplex - if so, bring the flag in.  If not, clear it
						if ((NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x80) == 0x80)
						{
							work_phases |= 0xE0;	//SP, House?, To SPCT - flagged on
						}
						else if (work_phases == 0x07)	//Fully connected, we can pass D and diff conns
						{
							work_phases |= 0x18;	//House?, D
						}

						//Apply the change to the TO node
						NR_busdata[temp_branch.to].phases &= work_phases;
					}//End FROM end is valid
					else	//FROM end not valid - hope we get hit by something else later
					{
						continue;
					}
				}//end FROM removal
				else	//Restoration time
				{
					//Make sure our FROM end is valid first - just in case
					if (Alteration_Nodes[temp_branch.from] == 1)
					{
						//Now see if we can even proceed - if we are a fault blocked area, then go no lower
						phase_restrictions = ~(NR_branchdata[NR_busdata[node_int].Link_Table[index]].faultphases & 0x07);	//Get unrestricted

						phase_restrictions &= (NR_branchdata[NR_busdata[node_int].Link_Table[index]].origphases & 0x07);	//Mask this with what we used to be

						if (phase_restrictions == 0x00)	//No phases are available below here, go to next
						{
							continue;
						}
						else	//At least one phase is valid, proceed
						{
							//Restore our phase portions - determine by our FROM end and restrictions
							work_phases = NR_busdata[temp_branch.from].phases & phase_restrictions;

							if ((temp_branch.origphases & 0x80) == 0x80)	//See if we were split phase - if so and no phases are present, remove that too for good measure
							{
								if (work_phases != 0x00)
									work_phases |= (NR_branchdata[NR_busdata[node_int].Link_Table[index]].origphases & 0xE0);	//Mask in SPCT-type flags
							}

							//Restore components - USBs are typically node oriented, so they aren't explicitly included here
							NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases |= work_phases;

							//Now apply the phases on the TO end of this branch - first off, get base phases
							work_phases = NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x07;

							//See if the line is a SPCT or Triplex - if so, bring the flag in.  If not, clear it
							if ((NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x80) == 0x80)
							{
								work_phases |= (NR_busdata[temp_branch.to].origphases & 0xE0);	//SP, House?, To SPCT - flagged on
							}
							else if (work_phases == 0x07)	//Fully connected, we can pass D and diff conns
							{
								work_phases |= (NR_busdata[temp_branch.to].origphases & 0x18);	//D
							}

							//Apply the change to the TO node
							NR_busdata[temp_branch.to].phases |= work_phases;
						}
					}//End FROM end is valid
					else	//FROM end not valid - hope we get hit by something else later
					{
						continue;
					}
				}//End TO end restoration
			}//End FROM end
			else	//To end
			{
				branch_val = temp_branch.from;

				if (impact_mode == false)	//Removal time
				{
					//Make sure our TO end is valid first - just in case
					if (Alteration_Nodes[temp_branch.to] == 1)	//Implies TO is done, but not FROM.  Basically indicates reverse flow or a mesh - not necessarily good (solver won't care)
					{
						//Remove our phase portions - determine by our TO end
						work_phases = NR_busdata[temp_branch.to].phases & 0x07;

						if ((temp_branch.phases & 0x80) == 0x80)	//See if we are split phase - if so and no phases are present, remove that too for good measure
						{
							if (work_phases != 0x00)
								work_phases |= 0xA0;	//Add in the split phase flag
						}

						//Remove components - USBs are typically node oriented, so they aren't included here
						NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases &= work_phases;

						//Now apply the phases on the FROM end of this branch - first off, get base phases - D is omitted (D unsupported for now)
						work_phases = NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x07;

						//See if the line is a SPCT or Triplex - if so, bring the flag in.  If not, clear it
						if ((NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x80) == 0x80)
						{
							work_phases |= 0xE0;	//SP, House?, To SPCT - flagged on
						}
						else if (work_phases == 0x07)	//Fully connected, we can pass D and diff conns
						{
							work_phases |= 0x18;	//House?, D
						}

						//Apply the change to the FROM node
						NR_busdata[temp_branch.from].phases &= work_phases;
					}//End TO end is valid
					else	//TO end not valid - hope we get hit by something else later
					{
						continue;
					}
				}//End removal - FROM end
				else	//Restoration time
				{
					//Make sure our TO end is valid first - just in case
					if (Alteration_Nodes[temp_branch.to] == 1)
					{
						//Now see if we can even proceed - if we are a fault blocked area, then go no lower
						phase_restrictions = ~(NR_branchdata[NR_busdata[node_int].Link_Table[index]].faultphases & 0x07);	//Get unrestricted

						phase_restrictions &= (NR_branchdata[NR_busdata[node_int].Link_Table[index]].origphases & 0x07);	//Mask this with what we used to be

						if (phase_restrictions == 0x00)	//No phases are available below here, go to next
						{
							continue;
						}
						else	//At least one phase is valid, proceed
						{
							//Restore our phase portions - determine by our TO end and restrictions
							work_phases = NR_busdata[temp_branch.to].phases & phase_restrictions;

							if ((temp_branch.origphases & 0x80) == 0x80)	//See if we were split phase - if so and no phases are present, remove that too for good measure
							{
								if (work_phases != 0x00)
									work_phases |= 0x80;	//Add in the split phase flag
							}

							//Restore components - USBs are typically node oriented, so they aren't explicitly included here
							NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases |= work_phases;

							//Now apply the phases on the TO end of this branch - first off, get base phases
							work_phases = NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x07;

							//See if the line is a SPCT or Triplex - if so, bring the flag in.  If not, clear it
							if ((NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x80) == 0x80)
							{
								work_phases |= (NR_busdata[temp_branch.from].origphases & 0xE0);	//SP, House?, To SPCT - flagged on
							}
							else if (work_phases == 0x07)	//Fully connected, we can pass D and diff conns
							{
								work_phases |= (NR_busdata[temp_branch.from].origphases & 0x18);	//House?, D
							}

							//Apply the change to the TO node
							NR_busdata[temp_branch.from].phases |= work_phases;
						}
					}//End TO end is valid
					else	//TO end not valid - hope we get hit by something else later
					{
						continue;
					}
				}//End restoration - FROM end
			}//End TO end

			//See if we're a switch - if so, call the appropriate function
			if (NR_branchdata[NR_busdata[node_int].Link_Table[index]].lnk_type == 2)
			{
				//Find this object
				temp_obj = gl_get_object(NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);

				//Make sure it worked
				if (temp_obj == NULL)
				{
					GL_THROW("Failed to find switch object:%s for reliability manipulation",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					/*  TROUBLESHOOT
					While attemping to map to an object for reliability-based modifications, GridLAB-D failed to find the object of interest.
					Please try again.  If they error persists, please submit your code and a bug report to the trac website.
					*/
				}

				//map the function
				funadd = (FUNCTIONADDR)(gl_get_function(temp_obj,"reliability_operation"));

				//make sure it worked
				if (funadd==NULL)
				{
					GL_THROW("Failed to find reliability manipulation method on object %s",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					/*  TROUBLESHOOT
					While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
					was not located.  Ensure this object type supports special actions and try again.  If the problem persists, please submit a bug
					report and your code to the trac website.
					*/
				}

				//Call the update
				temp_phases = (NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x07);
				return_val = ((int (*)(OBJECT *, unsigned char))(*funadd))(temp_obj,temp_phases);

				if (return_val == 0)	//Failed :(
				{
					GL_THROW("Failed to handle reliability manipulation on %s",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					/*  TROUBLESHOOT
					While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
					failed to execute properly.  If the problem persists, please submit a bug report and your code to the trac website.
					*/
				}
			}
			else if (NR_branchdata[NR_busdata[node_int].Link_Table[index]].lnk_type == 3)	//See if we're a fuse
			{
				//Find this object
				temp_obj = gl_get_object(NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);

				//Make sure it worked
				if (temp_obj == NULL)
				{
					GL_THROW("Failed to find fuse object:%s for reliability manipulation",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					/*  TROUBLESHOOT
					While attemping to map to an object for reliability-based modifications, GridLAB-D failed to find the object of interest.
					Please try again.  If they error persists, please submit your code and a bug report to the trac website.
					*/
				}

				//map the function
				funadd = (FUNCTIONADDR)(gl_get_function(temp_obj,"reliability_operation"));

				//make sure it worked
				if (funadd==NULL)
				{
					GL_THROW("Failed to find reliability manipulation method on object %s",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					/*  TROUBLESHOOT
					While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
					was not located.  Ensure this object type supports special actions and try again.  If the problem persists, please submit a bug
					report and your code to the trac website.
					*/
				}

				//Call the update
				temp_phases = (NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x07);
				return_val = ((int (*)(OBJECT *, unsigned char))(*funadd))(temp_obj,temp_phases);

				if (return_val == 0)	//Failed :(
				{
					GL_THROW("Failed to handle reliability manipulation on %s",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					/*  TROUBLESHOOT
					While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
					failed to execute properly.  If the problem persists, please submit a bug report and your code to the trac website.
					*/
				}
			}//end fuse
			else if (NR_branchdata[NR_busdata[node_int].Link_Table[index]].lnk_type == 6)	//Recloser
			{
				//Find this object
				temp_obj = gl_get_object(NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);

				//Make sure it worked
				if (temp_obj == NULL)
				{
					GL_THROW("Failed to find recloser object:%s for reliability manipulation",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					/*  TROUBLESHOOT
					While attemping to map to an object for reliability-based modifications, GridLAB-D failed to find the object of interest.
					Please try again.  If they error persists, please submit your code and a bug report to the trac website.
					*/
				}

				//map the function
				funadd = (FUNCTIONADDR)(gl_get_function(temp_obj,"recloser_reliability_operation"));

				//make sure it worked
				if (funadd==NULL)
				{
					GL_THROW("Failed to find reliability manipulation method on object %s",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					//defined above
				}

				//Call the update
				temp_phases = (NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x07);
				return_val = ((int (*)(OBJECT *, unsigned char))(*funadd))(temp_obj,temp_phases);

				if (return_val == 0)	//Failed :(
				{
					GL_THROW("Failed to handle reliability manipulation on %s",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					//Define above
				}
			}//end recloser
			else if (NR_branchdata[NR_busdata[node_int].Link_Table[index]].lnk_type == 5)	//Sectionalizer
			{
				//Find this object
				temp_obj = gl_get_object(NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);

				//Make sure it worked
				if (temp_obj == NULL)
				{
					GL_THROW("Failed to find sectionalizer object:%s for reliability manipulation",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					/*  TROUBLESHOOT
					While attemping to map to an object for reliability-based modifications, GridLAB-D failed to find the object of interest.
					Please try again.  If they error persists, please submit your code and a bug report to the trac website.
					*/
				}

				//map the function
				funadd = (FUNCTIONADDR)(gl_get_function(temp_obj,"sectionalizer_reliability_operation"));

				//make sure it worked
				if (funadd==NULL)
				{
					GL_THROW("Failed to find reliability manipulation method on object %s",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					//Defined above
				}

				//Call the update
				temp_phases = (NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x07);
				return_val = ((int (*)(OBJECT *, unsigned char))(*funadd))(temp_obj,temp_phases);

				if (return_val == 0)	//Failed :(
				{
					GL_THROW("Failed to handle reliability manipulation on %s",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					//Defined above
				}
			}//end sectionalizer

			//Flag us as handled
			Alteration_Nodes[branch_val] = 1;

			//Recursion!!
			support_search_links(branch_val,node_start,impact_mode);
		}//End both not handled (work to be done)
	}//End link table loop
}

//Function to reset "touched" alteration variable
void fault_check::reset_alterations_check(void)
{
	unsigned int index;

	//Reset the node - just whether the algorithm has looked at it or not
	for (index=0; index<NR_bus_count; index++)
	{
		Alteration_Nodes[index] = 0;	//Flag as untouched
	}
}

//Function to progress downwards and flag momentary interruptions
void fault_check::momentary_activation(int node_int)
{
	unsigned int index;
	OBJECT *tmp_obj;
	bool *momentary_flag;
	PROPERTY *pval;

	//See if we are a meter or triplex meter
	tmp_obj = gl_get_object(NR_busdata[node_int].name);

	//Make sure it worked
	if (tmp_obj == NULL)
	{
		GL_THROW("Failed to map node:%s during momentary interruption!",NR_busdata[node_int].name);
		/*  TROUBLESHOOT
		While mapping a node object for the momentary fault inducing process, the node failed to be found.
		Please try again and ensure the node exists.  If the error persists, please submit your code and a bug
		report via the trac website.
		*/
	}

	//See if we're a triplex meter or meter
	if ((gl_object_isa(tmp_obj,"triplex_meter","powerflow")) || (gl_object_isa(tmp_obj,"meter","powerflow")))
	{
		//Find the momentary interruptions flag and set it!
		pval = gl_get_property(tmp_obj,"customer_interrupted_secondary");

		//Make sure it worked
		if (pval == NULL)
		{
			GL_THROW("Failed to map momentary outage flag on node:%s",tmp_obj->name);
			/*  TROUBLESHOOT
			While attempting to map the momentary outage flag on a node, it failed to find said flag.
			Please try again. If the error persists, please submit your code and a bug report via the
			trac website.
			*/
		}

		//Map it
		momentary_flag = (bool*)GETADDR(tmp_obj,pval);

		//Flag it
		*momentary_flag = true;
	}

	//Loop through the link table
	for (index=0; index<NR_busdata[node_int].Link_Table_Size; index++)
	{
		//See if we're the from end of this link - if so, proceed
		if (NR_branchdata[NR_busdata[node_int].Link_Table[index]].from == node_int)
		{
			//Recurse our way in!
			momentary_activation(NR_branchdata[NR_busdata[node_int].Link_Table[index]].to);
		}
		//Defaulted else - must be the to end - we don't care
	}
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
		else
			return 0;
	}
	CREATE_CATCHALL(fault_check);
}

EXPORT int init_fault_check(OBJECT *obj, OBJECT *parent)
{
	try {
		fault_check *my = OBJECTDATA(obj,fault_check);
		return my->init(parent);
	}
	INIT_CATCHALL(fault_check);
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
	try {
		fault_check *pObj = OBJECTDATA(obj,fault_check);
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
	SYNC_CATCHALL(fault_check);
}

EXPORT int isa_fault_check(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,fault_check)->isa(classname);
}

//Function to remove/restore out of service items following a reliability fault
EXPORT int powerflow_alterations(OBJECT *thisobj, int baselink, bool rest_mode)
{
	fault_check *thsfltchk = OBJECTDATA(thisobj,fault_check);

	//Perform the removal
	thsfltchk->support_check_alterations(baselink,rest_mode);

	return 1;	
}

//Function to progress upstream from a sectionalizer and see if a recloser is present (and handle any future sectionalizers it encounters)
EXPORT double handle_sectionalizer(OBJECT *thisobj, int sectionalizer_number)
{
	double result_val;
	unsigned int index;
	int branch_val, node_val;
	bool loop_complete, proper_exit;
	double *rec_tries;
	PROPERTY *Pval;
	OBJECT *tmp_obj;
	fault_check *fltyobj;

	//Pull base branch
	node_val = NR_branchdata[sectionalizer_number].from;

	//Set flag
	loop_complete = false;

	//Big loop
	while (loop_complete == false)
	{
		//set tracking flag
		proper_exit = false;

		//Progress upstream until a recloser, or the SWING is encountered
		for (index=0; index<NR_busdata[node_val].Link_Table_Size; index++)	//parse through our connected link
		{
			//Pull the branch information
			branch_val = NR_busdata[node_val].Link_Table[index];

			//See if this node is a to - if so, proceed up.  If not, next search
			if (NR_branchdata[branch_val].to == node_val)
			{
				//It is! - see if it is a recloser
				if (NR_branchdata[branch_val].lnk_type == 6)
				{
					//Map the object
					tmp_obj = gl_get_object(NR_branchdata[branch_val].name);

					//Make sure it worked
					if (tmp_obj == NULL)
					{
						GL_THROW("Failure to map recloser object %s during sectionalizer handling",NR_branchdata[branch_val].name);
						/*  TROUBLESHOOT
						While mapping a recloser as part of a sectionalizer's reliability function, said recloser failed to be
						mapped correctly.  Please try again.  If the error persists, please submit your code and a bug report
						via the trac website.
						*/
					}

					//Increment its tries - map it first
					Pval = gl_get_property(tmp_obj,"number_of_tries");

					//Make sure it worked
					if (Pval == NULL)
					{
						GL_THROW("Failed to map recloser:%s retry count!",NR_branchdata[branch_val].name);
						/*  TROUBLESHOOT
						While attempting to map a reclosers attempt count, an error was encountered.  Please try again.
						If the error persists, please submit your code and a bug report via the trac website.
						*/
					}

					//Map it
					rec_tries = (double*)GETADDR(tmp_obj,Pval);

					//Increment it
					*rec_tries += 1;

					//Store this as the return result
					result_val = *rec_tries;

					//Propogate downstream and momentary flag objects
					//map the fault_check object - do as recursion and function is in space
					fltyobj = OBJECTDATA(fault_check_object,fault_check);

					//Call the function
					fltyobj->momentary_activation(NR_branchdata[branch_val].to);
					
					//Flag a proper exit
					proper_exit = true;

					//Loop is complete
					loop_complete = true;

					//Out of here we go
					break;
				
				}//Branch is a recloser
				else	//Not a recloser - onwards and upwards
				{
					//Map our from node for the next search
					node_val = NR_branchdata[branch_val].from;

					//Make sure the from node isn't a SWING - if it is, we're done
					if (NR_busdata[node_val].type == 2)
					{
						result_val = -1.0;		//Flag that a swing was found - failure :(
						loop_complete = true;	//No more looping
						proper_exit = true;		//Flag so don't get stuck
						break;					//Out of the FOR we go
					}
					else	//Normal node
					{
						proper_exit = true;		//Flag proper exiting (not loop limit) so next can proceed
						break;					//Pop out of this FOR loop and start the next
					}
				}//Branch isn't a recloser
			}
			else	//Not, so onward and upward!
			{
				continue;
			}
		}//end for loop
		
		//Generic check at end of FOR
		if ((index>NR_busdata[node_val].Link_Table_Size) && (proper_exit==false))	//Full traversion - then failure
		{
			result_val = 0.0;		//Encode as a failure
			loop_complete = true;	//Flag loop as over
		}
	}//End while

	return result_val;
}

/**@}**/
