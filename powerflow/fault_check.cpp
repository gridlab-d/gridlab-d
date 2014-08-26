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
		oclass = gl_register_class(mod,"fault_check",sizeof(fault_check),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class fault_check";
		else
			oclass->trl = TRL_DEMONSTRATED;
		
		if(gl_publish_variable(oclass,
			PT_enumeration, "check_mode", PADDR(fcheck_state),PT_DESCRIPTION,"Frequency of fault checks",
				PT_KEYWORD, "SINGLE", (enumeration)SINGLE,
				PT_KEYWORD, "ONCHANGE", (enumeration)ONCHANGE,
				PT_KEYWORD, "ALL", (enumeration)ALLT,
			PT_char1024,"output_filename",PADDR(output_filename),PT_DESCRIPTION,"Output filename for list of unsupported nodes",
			PT_bool,"reliability_mode",PADDR(reliability_mode),PT_DESCRIPTION,"General flag indicating if fault_check is operating under faulting or restoration mode -- reliability set this",
			PT_bool,"strictly_radial",PADDR(reliability_search_mode),PT_DESCRIPTION,"Flag to indicate if a system is known to be strictly radial -- uses radial assumptions for reliability alterations",
			PT_object,"eventgen_object",PADDR(rel_eventgen),PT_DESCRIPTION,"Link to generic eventgen object to handle unexpected faults",
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

	reliability_search_mode = true;	//By default (and for speed), assumes the system is truly, strictly radial

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
	

	//Make sure the eventgen_object is an actual eventgen object.
	if(rel_eventgen != NULL){
		if(!gl_object_isa(rel_eventgen,"eventgen")){
			gl_error("fault_check:%s %s is not an eventgen object. Please specify the name of an eventgen object.",obj->name,rel_eventgen);
			return 0;
			/*  TROUBLESHOOT
			The property eventgen_object was given the name of an object that is not an eventgen object.
			Please provide the name of an eventgen object located in your file.
			*/
		}
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
	unsigned char phases_adjust;
	bool perform_check, override_output;
		
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
		Supported_Nodes = (unsigned int**)gl_malloc(NR_bus_count*sizeof(unsigned int*));
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
			Supported_Nodes[index] = (unsigned int*)gl_malloc(3*sizeof(unsigned int));

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

			//Populate mesh-based items if necessary too
			if (reliability_search_mode == false)
			{
				Alteration_Links = (char*)gl_malloc(NR_branch_count*sizeof(char));

				//Check it
				if (Alteration_Links == NULL)
				{
					GL_THROW("fault_check: link alteration status vector allocation failure");
					/*  TROUBLESHOOT
					The fault_check object has failed to allocate the node alteration information vector.  Please try
					again and if the problem persists, submit your code and a bug report via the trac website.
					*/
				}

				//Do the same for the tracking variables - nodes
				Altered_Node_Track = (s_obj_supp_check*)gl_malloc(NR_bus_count*sizeof(s_obj_supp_check));

				//Check it
				if (Altered_Node_Track == NULL)
				{
					GL_THROW("fault_check: node alteration status vector allocation failure");
					//Defined above
				}

				//Do the same for the tracking variables -- links
				Altered_Link_Track = (s_obj_supp_check*)gl_malloc(NR_branch_count*sizeof(s_obj_supp_check));

				//Check it
				if (Altered_Link_Track == NULL)
				{
					GL_THROW("fault_check: link alteration status vector allocation failure");
					//Defined above
				}

				//Initialize the two altered list trackers
				for (index=0; index<NR_bus_count; index++)
				{
					Altered_Node_Track[index].altering_object = -9999;
					Altered_Node_Track[index].phases_altered = 0x00;
					Altered_Node_Track[index].next = NULL;
				}

				for (index=0; index<NR_branch_count; index++)
				{
					Altered_Link_Track[index].altering_object = -9999;
					Altered_Link_Track[index].phases_altered = 0x00;
					Altered_Link_Track[index].next = NULL;
				}

				//Populate the phases field too
				valid_phases = (unsigned char*)gl_malloc(NR_bus_count*sizeof(unsigned char));

				//Check it
				if (valid_phases == NULL)
				{
					GL_THROW("fault_check: node alteration status vector allocation failure");
					//Defined above
				}
			}//End additional allocations

			//Apply the general reset
			reset_alterations_check();
		}//End reliability mode allocing
	}//end 0th run

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
			if (reliability_search_mode == true)
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
			}
			else	//Mesh, maybe
			{
				//Call the connectivity check
				support_check_mesh(0,-9999,&phases_adjust);

				//If the first run and we just ran, see if anything started "bad"
				if (prev_time == 0)
				{
					support_search_links_mesh(-99,false,&phases_adjust);
				}

				override_output = output_check_supported_mesh();	//See if anything changed

				//See if anything broke
				if (override_output == true)
				{
					if (output_filename[0] != '\0')	//See if there's an output
					{
						write_output_file(t0);	//Write it
					}

					//See what mode we are in
					if (reliability_mode == false)
					{
						GL_THROW("Unsupported phase on a possibly meshed node");
						/*  TROUBLESHOOT
						An unsupported connection was found on the system.  Since reconfiguration is not enabled,
						the solver will possibly fail here on the next iteration, so the system is broken early.
						*/
					}
					else	//Must be in reliability mode
					{
						gl_warning("Unsupported phase on a possibly meshed node");
						/*  TROUBLESHOOT
						An unsupported connection was found on the system.  Since reliability is enabled,
						the solver will simply ignore the unsupported components for now.
						*/
					}
				}
			}
		}//no restoration
	}//End check

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
										else if ((rest_object ->Connectivity_Matrix[temp_branch.from][ temp_branch.to] == 3) && (*temp_branch.status == 1)) //  if the branch is a closed switch connection
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

//Mesh searching function -- checks to see how something was removed
void fault_check::search_links_mesh(int node_int, int buslink_fault, unsigned char fault_phases)
{
	unsigned char phases_altered;
	unsigned int index, device_value, node_value;
	unsigned char temp_phases;
	bool return_status;

	//Loop through our connected nodes
	for (index=0; index<NR_busdata[node_int].Link_Table_Size; index++)
	{
		//Pull link index -- just for readabiiity
		device_value = NR_busdata[node_int].Link_Table[index];

		//Get our opposite end reference
		if (node_int == NR_branchdata[device_value].from)
		{
			//Extract our index
			node_value = NR_branchdata[device_value].to;
		}
		else	//Must be the TO side, so check from
		{
			//Extract our index
			node_value = NR_branchdata[device_value].from;
		}

		//Get initial phasing information - the ones that are available
		temp_phases = NR_branchdata[device_value].phases;

		//See if we're the oringally "altered" bus -- mainly to keep us from overriding any reliability actions
		if (device_value != buslink_fault)
		{
			//See if this particular link was removed by the currently occurring fault
			return_status = altered_device_search(&Altered_Link_Track[device_value],buslink_fault,fault_phases,&phases_altered,false);

			//If it was removed by this, allow that portion potential re-instation
			if (return_status == true)
			{
				temp_phases |= (NR_branchdata[device_value].origphases & phases_altered);
			}
			//Default else, no adjustments
		}//End no the faulted bus
		//Default else -- our phase manipulations were hopefully handled elsewhere

		//Populate the phase information
		valid_phases[node_value] |= (valid_phases[node_int] & temp_phases);
	}//End of node link table traversion
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

//Mesh-capable version of support check -- by default, it doesn't support restoration object
void fault_check::support_check_mesh(int swing_node_int,int buslink_fault,unsigned char *temp_phases)
{
	//unsigned char temp_phases;
	s_obj_supp_check *temp_check;
	unsigned int indexa, indexb;

	//Reset the node status list
	reset_support_check();

	//Swing node has support - if the phase exists (changed for complete faults)
	valid_phases[swing_node_int] = NR_busdata[swing_node_int].phases & 0x07;

	//Figure out what the phases altered was for the "fault" we're looking at -- mainly to pass this information
	//Make sure it wasn't a swing fault or a "non-existant" fault
	if ((buslink_fault != -99) && (buslink_fault != -9999))
	{
		//Check first entry
		if (Altered_Link_Track[buslink_fault].altering_object ==  buslink_fault)	//If it is ourself
		{
			//Extract the data
			*temp_phases = Altered_Link_Track[buslink_fault].phases_altered;
		}
		else	//No match on first entry
		{
			//Make sure there is at least one more
			if (Altered_Link_Track[buslink_fault].next != NULL)
			{
				//Set an initial default, just in case we progress out
				*temp_phases = 0x80;

				//Map to the next entry
				temp_check = Altered_Link_Track[buslink_fault].next;

				//Check it
				if (temp_check->altering_object == buslink_fault)
				{
					//Extract it
					*temp_phases = temp_check->phases_altered;
				}

				//Progress
				while (temp_check->next != NULL)
				{
					//Progress inward
					temp_check = temp_check->next;

					if (temp_check->altering_object == buslink_fault)
					{
						//Extract it
						*temp_phases = temp_check->phases_altered;

						//Get out
						break;
					}
				}//End traversion

				//Got to the end, which means we were never found, add us in (mainly to prevent later errors)
				*temp_phases = ((NR_branchdata[buslink_fault].origphases ^ NR_branchdata[buslink_fault].phases) & 0x07);

				//Store us too - even if we don't do anything, prevents errors later
				altered_device_add(&Altered_Link_Track[buslink_fault],buslink_fault,*temp_phases,*temp_phases);
			}
			else	//Nope -- might be first entry, just flag it
			{
				if (Altered_Link_Track[buslink_fault].altering_object == -9999)	//No faults, so we're the first in -- record us
				{
					*temp_phases = ((NR_branchdata[buslink_fault].origphases ^ NR_branchdata[buslink_fault].phases) & 0x07);

					//Store us too - even if we don't do anything, prevents errors later
					altered_device_add(&Altered_Link_Track[buslink_fault],buslink_fault,*temp_phases,*temp_phases);
				}
				else	//Somethign odd, just flag us generic and see what happens (shouldn't really get here)
				{
					//Just set a semi empty value -- nothing to find or compare
					*temp_phases = 0xFF;
				}
			}
		}//End not match on first entry
	}//End not a swing or generic fault
	else if (buslink_fault == -99)	//It is a SWING fault
	{
		//Check and see if we're a removal or a re-instation
		if (NR_busdata[0].origphases != NR_busdata[0].phases)
		{
			//Figure out what was removed
			*temp_phases = ((NR_busdata[0].origphases ^ NR_busdata[0].phases) & 0x07);
		}
		else
		{
			//Read our information -- assumes a SWING fault is ALWAYS the only one affecting the SWING
			*temp_phases = Altered_Node_Track[0].phases_altered;
		}
	}
	else	//Left over, just a generic "check"
	{
		*temp_phases = 0xFF;
	}

	//Call the node link-erator (node support check) - call it on the swing, the details are handled inside
	//Supports possibly meshed topology - brute force method
	for (indexa=0; indexa<NR_bus_count; indexa++)
	{
		for (indexb=0; indexb<NR_bus_count; indexb++)
		{
			//Call the search link on individual nodes
			search_links_mesh(indexb,buslink_fault,*temp_phases);
		}
	}
}

void fault_check::reset_support_check(void)
{
	unsigned int index;

	//Reset the node - 0 = unsupported, 1 = supported (not populated here), 2 = N/A (no phase there)
	for (index=0; index<NR_bus_count; index++)
	{
		if (reliability_search_mode == true)	//Strictly radial
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
		else	//Maybe-sort-not-really radial
		{
			//Set main variable
			valid_phases[index] = 0x00;
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
		if (reliability_search_mode == true)
		{
			//Put the phases into an easier to read format
			phase_outs = 0x00;

			if (Supported_Nodes[index][0] == 0)	//Check A
				phase_outs |= 0x04;

			if (Supported_Nodes[index][1] == 0)	//Check B
				phase_outs |= 0x02;

			if (Supported_Nodes[index][2] == 0)	//Check C
				phase_outs |= 0x01;
		}
		else	//Mesh mode
		{
			phase_outs = ((~(valid_phases[index] & NR_busdata[index].origphases)) & (0x07 & NR_busdata[index].origphases));
		}

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
	unsigned char phases_adjust;

	//Check the mode
	if (reliability_search_mode == true)	//Strickly radial assumption, continue as always
	{
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
	else	//Not assumed to be strictly radial, or just being safe -- check EVERYTHING
	{
		//Reset the alteration matrix
		reset_alterations_check();

		//Call a support check -- reset handled inside
		//Always assumed to NOT be in "restoration object" mode
		support_check_mesh(0,baselink_int,&phases_adjust);

		//Now loop through and remove those components that are not supported anymore -- start from SWING, just because we have to start somewhere
		support_search_links_mesh(baselink_int,rest_mode,&phases_adjust);
	}
}

//Recursive function to traverse powerflow and alter phases as necessary
//Checks against support found, not an assumed radial traversion (little slower, but more thorough)
//Based on support_search_links below
void fault_check::support_search_links_mesh(int baselink_int, bool impact_mode,unsigned char *phases_adjust)
{
	unsigned int indexval, index;
	int device_index;
	unsigned char temp_phases, work_phases, remove_phases, add_phases_orig, add_phases;
	bool remove_status;

	//First things first -- figure out how to flag ourselves
	if (impact_mode == false)	//Removal mode
	{
		if (baselink_int != -99)	//Not a SWING-related fault, so actually do something (SWING just proceeds in)
		{
			//Figure out which phases mismatch the FROM/TO ends of this link - 
			temp_phases = ((valid_phases[NR_branchdata[baselink_int].from] ^ valid_phases[NR_branchdata[baselink_int].to]) & 0x07);

			//Cast by our original phases, in case something else broke us first
			remove_phases = temp_phases & NR_branchdata[baselink_int].origphases;

			//Create our entry
			altered_device_add(&Altered_Link_Track[baselink_int],baselink_int,remove_phases,*phases_adjust);

			//Flag us as handled
			Alteration_Links[baselink_int] = 1;
		}//Not a SWING fault
		else	//Swing node -- add it in
		{
			//See what phases were removed from us
			remove_phases = ((NR_busdata[0].origphases ^ NR_busdata[0].phases) & 0x07);

			//Add this to the node tracker
			altered_device_add(&Altered_Node_Track[0],baselink_int,remove_phases,*phases_adjust);

			//Flag us as handled
			Alteration_Nodes[0] = 1;
		}
		//Default else, just proceed
	}
	else	//Restoration mode
	{
		if (baselink_int != -99)	//Not a SWING-related fault, so actually do somethign (SWING just proceeds in)
		{
			//Remove our entry -- Search not necessary, we should be part of our own fault!
			//Note that current reliability operations will get confused if you induce multiple "faults" explicitly on the same
			//device -- this will need to be fixed if future implementations require it
			//Mainly comes as a limitation that reliability is fixing the fault outside of this object, so when it checks for
			//"restoration", it can get confused
			
			//Go forth and see what we find
			remove_status = altered_device_search(&Altered_Link_Track[baselink_int],baselink_int,*phases_adjust,&add_phases_orig,true);

			//Make sure it worked.  If it didn't, something bad happened
			if (remove_status == false)
			{
				GL_THROW("An error was encountered trying to remove the faulted state from the original faulting object!");
				/*  TROUBLESHOOT
				While attempting to restore the original "faulting" device from the faulted state, an error occurred.
				Please try again.  If the error persists, please submit your file and a description via the trac system.
				*/
			}

			//Flag us as handled
			Alteration_Links[baselink_int] = 1;
		}//Not a SWING fault
		else	//Are a swing node, get the information back
		{
			//Go forth and see what we find
			remove_status = altered_device_search(&Altered_Node_Track[0],baselink_int,*phases_adjust,&add_phases_orig,true);

			//Make sure it worked.  If it didn't, something bad happened
			if (remove_status == false)
			{
				GL_THROW("An error was encountered trying to remove the faulted state from the original faulting object!");
				//Defined above
			}

			//Flag us as handled
			Alteration_Nodes[0] = 1;
		}
	}
		
	//Traverse the bus list
	for (indexval=0; indexval<NR_bus_count; indexval++)
	{
		//Check our location in the support check -- see if we're supported or not and alter as necessary
		if (impact_mode == true)	//Restoration
		{
			//Mask out our phases -- see what is available
			work_phases = NR_busdata[indexval].phases & 0x07;

			//See if any change is needed
			if (work_phases != valid_phases[indexval])
			{
				//Figure out what is being added back in
				work_phases = NR_busdata[indexval].origphases & valid_phases[indexval];

				//Figure out the change
				add_phases = ((NR_busdata[indexval].phases ^ work_phases) & 0x07);

				//See if we're triplex
				if ((NR_busdata[indexval].origphases & 0x80) == 0x80)
				{
					add_phases |= (NR_busdata[indexval].origphases & 0xE0);	//SP, House?, To SPCT - flagged on
				}
				else if (work_phases == 0x07)	//Fully connected, we can pass D and diff conns
				{
					add_phases |= (NR_busdata[indexval].origphases & 0x18);	//D
				}

				//Apply the change to the TO node
				NR_busdata[indexval].phases |= add_phases;

				//Remove our reference in the altered list
				remove_status = altered_device_search(&Altered_Node_Track[indexval],baselink_int,*phases_adjust,&add_phases,true);

				//Make sure it worked, just to be safe.  It should have, but you never know
				if (remove_status == false)
				{
					GL_THROW("Failed to remove faulted entry information after object has been restored!");
					/*  TROUBLESHOOT
					While attempting to remove the information for a particular fault from an object's database,
					an error was encountered.  Please try again.  If the error persists, please submit your model
					and a bug report via the trac website.
					*/
				}

				//Loop through our links and adjust anyone necessary
				for (index=0; index<NR_busdata[indexval].Link_Table_Size; index++)	//parse through our connected link
				{
					//Extract the branchdata reference
					device_index = NR_busdata[indexval].Link_Table[index];

					//See if it has already been handled -- no sense in duplicating items
					if (Alteration_Links[device_index] == 0)	//Not handled
					{
						//See which phases are even allowed
						temp_phases = (valid_phases[NR_branchdata[device_index].from] & valid_phases[NR_branchdata[device_index].to]);

						//See how these compare with what we can do
						add_phases = NR_branchdata[device_index].origphases & temp_phases;

						//Make sure it's non-zero
						if (add_phases != 0x00)	//We can add something!
						{
							//See if we were original triplex-oriented
							if ((NR_branchdata[device_index].origphases & 0x80) == 0x80)
							{
								add_phases |= (NR_branchdata[device_index].origphases & 0xE0);	//Mask in SPCT-type flags
							}

							//Restore components - USBs are typically node oriented, so they aren't explicitly included here
							NR_branchdata[device_index].phases |= add_phases;

							//Remove our reference in the altered list
							remove_status = altered_device_search(&Altered_Link_Track[device_index],baselink_int,*phases_adjust,&add_phases,true);

							//Make sure it worked, just to be safe.  It should have, but you never know
							if (remove_status == false)
							{
								GL_THROW("Failed to remove faulted entry information after object has been restored!");
								//Defined above
							}
						}//End addition of phases
						//Default else, nothing new supported here

						//Flag this branch as handled
						Alteration_Links[device_index] = 1;
					
						//Functionalized version of modifier
						special_object_alteration_handle(device_index);
					}//End unhandled
					//Defaulted else, already handled, next item in the list
				}//End list traversion
			}//End change needed
			//Default else, no change

			//Flag us as handled -- whether we changed or not
			Alteration_Nodes[indexval] = 1;
		}//End restoration mode
		else	//Removal mode
		{
			//Mask out our phases -- see what is available to remove
			work_phases = NR_busdata[indexval].phases & 0x07;

			//See if any change is needed
			if (work_phases != valid_phases[indexval])
			{
				//Mask out the work_phases by the "supported" ones
				work_phases = work_phases & valid_phases[indexval];

				//Figure out what was just removed -- assumes it was a removal
				remove_phases = ((NR_busdata[indexval].phases ^ work_phases) & 0x07);

				//Add our noded link in
				altered_device_add(&Altered_Node_Track[indexval],baselink_int,remove_phases,*phases_adjust);

				//Loop through our link table -- if we don't have a support phase, they shouldn't be valid either
				for (index=0; index<NR_busdata[indexval].Link_Table_Size; index++)	//parse through our connected link
				{
					//Extract the branchdata reference
					device_index = NR_busdata[indexval].Link_Table[index];

					//See if it has already been handled -- no sense in duplicating items
					if (Alteration_Links[device_index] == 0)	//Not handled
					{
						//See if we are split-phase
						if ((NR_branchdata[device_index].phases & 0x80) == 0x80)
						{
							//Just remove it all
							NR_branchdata[device_index].phases = 0x00;
						}
						else	//Not split-phase, normal - continue
						{
							//Remove components - USBs are typically node oriented, so they aren't included here
							NR_branchdata[device_index].phases &= work_phases;
						}

						//Add our details to the fault tracking system
						altered_device_add(&Altered_Link_Track[device_index],baselink_int,remove_phases,*phases_adjust);

						//Flag as handled
						Alteration_Links[device_index] = 1;
					
						//Functionalized version of modifier
						special_object_alteration_handle(device_index);
					}//Link not handled
					else	//Link already handled, proceed
					{
						continue;	//Probably really don't need this explicitly specified, but meh
					}
				}//End link table traversion

				//See if the node is Triplex & valie - if so, bring the flag in.  If not, clear it
				if (((NR_busdata[indexval].phases & 0x80) == 0x80) && (work_phases != 0x00))
				{
					work_phases |= 0xE0;	//SP, House?, To SPCT - flagged on
				}
				else if (work_phases == 0x07)	//Fully connected, we can pass D and diff conns
				{
					work_phases |= 0x18;	//House?, D
				}

				//Apply the change to the TO node
				NR_busdata[indexval].phases &= work_phases;

				//Flag this node as handled
				Alteration_Nodes[indexval] = 1;
			}//End phases don't match - action necessary
			//Defaulted else, continue
		}//End removal mode
	}//End bus traversion list

	//Should be done -- links should have been caught by respective bus values
}

//Code to check the special link devices and modify as needed
void fault_check::special_object_alteration_handle(int branch_idx)
{
	int return_val;
	unsigned char temp_phases;
	OBJECT *temp_obj;
	FUNCTIONADDR funadd = NULL;

	//See if we're a switch - if so, call the appropriate function
	if (NR_branchdata[branch_idx].lnk_type == 2)
	{
		//Find this object
		temp_obj = gl_get_object(NR_branchdata[branch_idx].name);

		//Make sure it worked
		if (temp_obj == NULL)
		{
			GL_THROW("Failed to find switch object:%s for reliability manipulation",NR_branchdata[branch_idx].name);
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
			GL_THROW("Failed to find reliability manipulation method on object %s",NR_branchdata[branch_idx].name);
			/*  TROUBLESHOOT
			While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
			was not located.  Ensure this object type supports special actions and try again.  If the problem persists, please submit a bug
			report and your code to the trac website.
			*/
		}

		//Call the update
		temp_phases = (NR_branchdata[branch_idx].phases & 0x07);
		return_val = ((int (*)(OBJECT *, unsigned char))(*funadd))(temp_obj,temp_phases);

		if (return_val == 0)	//Failed :(
		{
			GL_THROW("Failed to handle reliability manipulation on %s",NR_branchdata[branch_idx].name);
			/*  TROUBLESHOOT
			While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
			failed to execute properly.  If the problem persists, please submit a bug report and your code to the trac website.
			*/
		}
	}
	else if (NR_branchdata[branch_idx].lnk_type == 3)	//See if we're a fuse
	{
		//Find this object
		temp_obj = gl_get_object(NR_branchdata[branch_idx].name);

		//Make sure it worked
		if (temp_obj == NULL)
		{
			GL_THROW("Failed to find fuse object:%s for reliability manipulation",NR_branchdata[branch_idx].name);
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
			GL_THROW("Failed to find reliability manipulation method on object %s",NR_branchdata[branch_idx].name);
			/*  TROUBLESHOOT
			While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
			was not located.  Ensure this object type supports special actions and try again.  If the problem persists, please submit a bug
			report and your code to the trac website.
			*/
		}

		//Call the update
		temp_phases = (NR_branchdata[branch_idx].phases & 0x07);
		return_val = ((int (*)(OBJECT *, unsigned char))(*funadd))(temp_obj,temp_phases);

		if (return_val == 0)	//Failed :(
		{
			GL_THROW("Failed to handle reliability manipulation on %s",NR_branchdata[branch_idx].name);
			/*  TROUBLESHOOT
			While attempting to handle special reliability actions on a "special" device (switch, recloser, etc.), the function required
			failed to execute properly.  If the problem persists, please submit a bug report and your code to the trac website.
			*/
		}
	}//end fuse
	else if (NR_branchdata[branch_idx].lnk_type == 6)	//Recloser
	{
		//Find this object
		temp_obj = gl_get_object(NR_branchdata[branch_idx].name);

		//Make sure it worked
		if (temp_obj == NULL)
		{
			GL_THROW("Failed to find recloser object:%s for reliability manipulation",NR_branchdata[branch_idx].name);
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
			GL_THROW("Failed to find reliability manipulation method on object %s",NR_branchdata[branch_idx].name);
			//defined above
		}

		//Call the update
		temp_phases = (NR_branchdata[branch_idx].phases & 0x07);
		return_val = ((int (*)(OBJECT *, unsigned char))(*funadd))(temp_obj,temp_phases);

		if (return_val == 0)	//Failed :(
		{
			GL_THROW("Failed to handle reliability manipulation on %s",NR_branchdata[branch_idx].name);
			//Define above
		}
	}//end recloser
	else if (NR_branchdata[branch_idx].lnk_type == 5)	//Sectionalizer
	{
		//Find this object
		temp_obj = gl_get_object(NR_branchdata[branch_idx].name);

		//Make sure it worked
		if (temp_obj == NULL)
		{
			GL_THROW("Failed to find sectionalizer object:%s for reliability manipulation",NR_branchdata[branch_idx].name);
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
			GL_THROW("Failed to find reliability manipulation method on object %s",NR_branchdata[branch_idx].name);
			//Defined above
		}

		//Call the update
		temp_phases = (NR_branchdata[branch_idx].phases & 0x07);
		return_val = ((int (*)(OBJECT *, unsigned char))(*funadd))(temp_obj,temp_phases);

		if (return_val == 0)	//Failed :(
		{
			GL_THROW("Failed to handle reliability manipulation on %s",NR_branchdata[branch_idx].name);
			//Defined above
		}
	}//end sectionalizer
}

//Functionalized entry to find end of linked-list and add entry in
void fault_check::altered_device_add(s_obj_supp_check *base_item, int fault_cause, unsigned char phases_alter, unsigned char phases_fault)
{
	s_obj_supp_check *temp_check, *temp_check_new;

	//See if we're the first one or not
	if ((base_item->next == NULL) && (base_item->altering_object == -9999))
	{
		//Just add us in -- already allocated, so not needed
		base_item->altering_object = fault_cause;
		base_item->phases_altered = phases_alter;
		base_item->fault_phases = phases_fault;
	}
	else	//Find the end!
	{
		//Copy pointer
		temp_check = base_item;

		//Progress
		while (temp_check->next != NULL)
		{
			temp_check = temp_check->next;
		}

		//Found the null, create a new one
		temp_check_new = (s_obj_supp_check*)gl_malloc(sizeof(s_obj_supp_check));

		//Make sure it worked
		if (temp_check_new == NULL)
		{
			GL_THROW("Failed to allocate storage location for new faulting location");
			/*  TROUBLESHOOT
			While attempting to allocate a new entry of the faulting device list, an error
			was encountered.  Please try again.  If the error persists, please post your model
			and any relevant details to the trac system.
			*/
		}

		//Store the details
		temp_check_new->altering_object = fault_cause;
		temp_check_new->phases_altered = phases_alter;
		temp_check_new->fault_phases = phases_fault;
		temp_check_new->next = NULL;

		//Link us in
		temp_check->next = temp_check_new;
	}
}

//Functionalized entry to see if a particular fault record is part of this entry
//If appropriately flagged, will remove it too
bool fault_check::altered_device_search(s_obj_supp_check *base_item, int fault_case, unsigned char phases_fault, unsigned char *phases_alter, bool remove_mode)
{
	bool ignore_phases;
	s_obj_supp_check *temp_check_prev, *temp_check_next;

	if (phases_fault == 0xFF)
	{
		ignore_phases = true;		//Special mode - extract what the "fault" was
	}
	else
	{
		ignore_phases = false;	//Normal mode, just check us
	}

	//See if there's more than one entry
	if (base_item->next == NULL)	//Single entry, so it better be us
	{
		if ((base_item->altering_object == fault_case) && ((base_item->fault_phases == phases_fault) || ignore_phases))	//Match!
		{
			//Export our alteration
			*phases_alter = base_item->phases_altered;

			if (remove_mode == true)	//Clean us up
			{
				//Deflag everything
				base_item->altering_object = -9999;
				base_item->phases_altered = 0;
				base_item->fault_phases = 0x00;

			}
			//Default else -- don't do anything

			//Success!
			return true;
		}
		else	//No match, indicate as much
		{
			return false;
		}
	}//End only the base entry
	else	//More than one -- transition the list
	{
		//See if we're the first one
		if ((base_item->altering_object == fault_case) && ((base_item->fault_phases == phases_fault) || ignore_phases))	//We are
		{
			//Export values
			*phases_alter = base_item->phases_altered;

			//Check for alteration modes
			if (remove_mode == true)	//Clean us out
			{
				//Get our next object
				temp_check_next = base_item->next;

				//Copy our values
				base_item->altering_object = temp_check_next->altering_object;
				base_item->phases_altered = temp_check_next->phases_altered;
				base_item->fault_phases = temp_check_next->fault_phases;
				base_item->next = temp_check_next->next;

				//Now free up the next item
				gl_free(temp_check_next);
			}
			//Defaulted else -- nothing to do 

			//Return that it was found
			return true;
		}//End first entry matches
		else	//Nope, proceed in
		{
			//Copy us
			temp_check_prev = base_item;

			//Progress inward
			while (temp_check_prev->next != NULL)
			{
				//Extract us
				temp_check_next = temp_check_prev->next;

				//Check us
				if ((temp_check_next->altering_object == fault_case) && ((temp_check_next->fault_phases == phases_fault) || ignore_phases))	//Found it
				{
					//Export items
					*phases_alter = temp_check_next->phases_altered;

					//See if we're removing it or not
					if (remove_mode == true)	//Remove is
					{
						//Update our referrer to the next update
						temp_check_prev->next = temp_check_next->next;

						//Remove us
						gl_free(temp_check_next);
					}
					//default else, don't do anything

					//Indicate it was found
					return true;
				}
				else	//Not found, next
				{
					temp_check_prev = temp_check_next;
				}
			}

			//If we made it this far out, we completely progressed and didn't find it, return false
			return false;
		}//End not first entry traversion

		//Blocking -- should never get here, but if we did, we failed
		return false;
	}//End more than one object

	//If we made it this far, we failed -- not found in here -- should never get here
	return false;
}

//Recursive function to traverse powerflow and alter phases as necessary
//Based on search_links code from above
void fault_check::support_search_links(int node_int, int node_start, bool impact_mode)
{
	unsigned int index;
	bool both_handled, from_val;
	int branch_val;
	BRANCHDATA temp_branch;
	unsigned char work_phases, phase_restrictions;

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

			//Functionalized version of modifier
			special_object_alteration_handle(NR_busdata[node_int].Link_Table[index]);

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

	//If we're in "special" mode, reset the branches too
	if (reliability_search_mode == false)
	{
		for (index=0; index<NR_branch_count; index++)
		{
			Alteration_Links[index] = 0;	//Flag as untouched
		}
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

//Function to see if an unsupported node exists, mainly for file output
bool fault_check::output_check_supported_mesh(void)
{
	unsigned int index;

	for (index=0; index<NR_bus_count; index++)
	{
		if ((NR_busdata[index].phases ^ NR_busdata[index].origphases) != 0x00) 
		{
			//Change found, get us out of here!
			return true;
		}
	}

	//If we made it here, no change was found, just exit
	return false;
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
