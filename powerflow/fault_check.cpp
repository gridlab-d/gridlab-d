/** $Id: fault_check.cpp 2009-11-09 15:00:00Z d3x593 $
	Copyright (C) 2009 Battelle Memorial Institute
**/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "fault_check.h"

//////////////////////////////////////////////////////////////////////////
// fault_check CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* fault_check::oclass = nullptr;
CLASS* fault_check::pclass = nullptr;

fault_check::fault_check(MODULE *mod) : powerflow_object(mod)
{
	if(oclass == nullptr)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod,"fault_check",sizeof(fault_check),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class fault_check";
		else
			oclass->trl = TRL_DEMONSTRATED;
		
		if(gl_publish_variable(oclass,
			PT_enumeration, "check_mode", PADDR(fcheck_state),PT_DESCRIPTION,"Frequency of fault checks",
				PT_KEYWORD, "SINGLE", (enumeration)SINGLE,
				PT_KEYWORD, "ONCHANGE", (enumeration)ONCHANGE,
				PT_KEYWORD, "ALL", (enumeration)ALLT,
				PT_KEYWORD, "SINGLE_DEBUG", (enumeration)SINGLE_DEBUG,
				PT_KEYWORD, "SWITCHING", (enumeration)SWITCHING,
			PT_char1024,"output_filename",PADDR(output_filename),PT_DESCRIPTION,"Output filename for list of unsupported nodes",
			PT_bool,"reliability_mode",PADDR(reliability_mode),PT_DESCRIPTION,"General flag indicating if fault_check is operating under faulting or restoration mode -- reliability set this",
			PT_bool,"strictly_radial",PADDR(reliability_search_mode),PT_DESCRIPTION,"Flag to indicate if a system is known to be strictly radial -- uses radial assumptions for reliability alterations",
			PT_bool,"full_output_file",PADDR(full_print_output),PT_DESCRIPTION,"Flag to indicate if the output_filename report contains both supported and unsupported nodes -- if false, just does unsupported",
			PT_bool,"grid_association",PADDR(grid_association_mode),PT_DESCRIPTION,"Flag to indicate if multiple, distinct grids are allowed in a GLM, or if anything not attached to the master swing is removed",
			PT_object,"eventgen_object",PADDR(rel_eventgen),PT_DESCRIPTION,"Link to generic eventgen object to handle unexpected faults",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
			if (gl_publish_function(oclass,"reliability_alterations",(FUNCTIONADDR)powerflow_alterations)==nullptr)
				GL_THROW("Unable to publish remove from service function");
			if (gl_publish_function(oclass,"handle_sectionalizer",(FUNCTIONADDR)handle_sectionalizer)==nullptr)
				GL_THROW("Unable to publish sectionalizer special function");
			if (gl_publish_function(oclass,"island_removal_function",(FUNCTIONADDR)powerflow_disable_island)==nullptr)
				GL_THROW("Unable to publish island deletion function");
			if (gl_publish_function(oclass,"rescan_topology",(FUNCTIONADDR)powerflow_rescan_topo)==nullptr)
				GL_THROW("Unable to publish the topology rescan function");
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

	full_print_output = false;	//By default, only output unsupported nodes

	reliability_mode = false;	//By default, we find errors and fail - if true, dumps log, but continues on its merry way

	reliability_search_mode = true;	//By default (and for speed), assumes the system is truly, strictly radial

	rel_eventgen = nullptr;		//No object linked by default

	restoration_fxn = nullptr;		//No restoration function mapped by default

	grid_association_mode = false;	//By default, we go to normal "Highlander" grid (there can be only one!)

	force_reassociation = false;	//By default, don't need to reassociate

	return result;
}

int fault_check::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	FILE *FPoint;

	//Register us in the global - so faults know who they gonna call
	if (fault_check_object == nullptr)	//Make sure we're the only one
	{
		fault_check_object = obj;	//Link us up!
	}
	else
	{
		//Make sure the existing one is not us - just in case the deferred init fired
		if (fault_check_object != obj)
		{
			GL_THROW("Only one fault_check object is supported at this time");
			/*  TROUBLESHOOT
			At this time, a .GLM file can only contain one fault_check object.  Future implementations
			may change this.  Please restrict yourself to one fault_check object in the mean time.
			*/
		}
		//Default else - it's set and it is us, so just proceed.
	}

	if (solver_method == SM_NR)
	{
		//Swing tracking variable - we have to be one below the SWING, so make sure it settled
		if (NR_swing_rank_set)
		{
			gl_set_rank(obj,(NR_expected_swing_rank-1));	//swing-1
		}
		else
		{
			//Not set yet, deferred init
			return 2;
		}
	}
	else
	{
		GL_THROW("Fault checking object is only valid for the Newton-Raphson solver at this time.");
		/*  TROUBLESHOOT
		The fault_check object only works with the Newton-Raphson powerflow solver at this time.
		Other solvers may be integrated at a future date.
		*/
	}

	//Make sure the eventgen_object is an actual eventgen object.
	if(rel_eventgen != nullptr){
		if(!gl_object_isa(rel_eventgen,"eventgen")){
			gl_error("fault_check:%s %s is not an eventgen object. Please specify the name of an eventgen object.",obj->name,rel_eventgen->name);
			return 0;
			/*  TROUBLESHOOT
			The property eventgen_object was given the name of an object that is not an eventgen object.
			Please provide the name of an eventgen object located in your file.
			*/
		}

		//Assume this means reliability is a go!
		reliability_mode = true;
	}

	//By default, kill the file - open and close it
	if (output_filename[0] != '\0')	//Make sure it isn't empty
	{
		FPoint = fopen(output_filename,"wt");
		fclose(FPoint);
	}

	//See if grid association is on - if so, force teh mesh searching method
	if (grid_association_mode)
	{
		if (reliability_search_mode)
		{
			gl_warning("fault_check:%s was forced into meshed search mode due to multiple grids being desired",obj->name ? obj->name : "Unnamed");
			/*  TROUBLESHOOT
			While starting up a fault_check object, the grid_association flag was set.  This flag requires the meshed topology checking routines
			to be active, so that capability was enabled.  If this is undesired, please deactivate grid_association capability.
			*/

			reliability_search_mode = false;
		}
	}

	//Do a powerflow multi_island check
	if (NR_island_fail_method && !grid_association_mode)
	{
		gl_error("fault_check:%s - powerflow::NR_island_fail_method is set, but grid_association is not!",(obj->name?obj->name:"Unnamed"));
		/*  TROUBLESHOOT
		The powerflow directive to handle individual island failures (NR_island_fail_method) is set to true, but grid_association is not
		enabled in fault_check.  This won't work.  Correct one of them.
		*/ 
		return 0;
	}

	//Set powerflow global flag for checking things
	if (!reliability_search_mode)
	{
		meshed_fault_checking_enabled = true;	//Let other powerflow objects know we're a mesh-based check
	}

	//See if we're in "special mode" and set the flag
	if (fcheck_state == SINGLE_DEBUG)
	{
		//Set the powerflow global -- all checks will be off of this (so no crazy player manipulations allowed)
		fault_check_override_mode = true;
	}

	return powerflow_object::init(parent);
}

TIMESTAMP fault_check::sync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::sync(t0);
	unsigned int index;
	int return_val;
	bool perform_check, override_output;

	gl_strftime (t0, time_buf, TIME_BUF_SIZE);
	gl_verbose ("*** fault_check::sync:%s:%ld", time_buf, t0);

	if (prev_time == 0)	//First run - see if restoration exists (we need it for now)
	{
		allocate_alterations_values(reliability_mode);

		//Override tret to force a reiteration - mostly to catch "initially islanded nodes" in single runs
		tret = t0;
	}

	if (fault_check_override_mode)	//Special mode -- do a single topology check and then fail
	{
		perform_check = true;
	}
	else if ((fcheck_state == SINGLE) && (prev_time == 0))	//Single only occurs on first iteration
	{
		perform_check = true;	//Flag for a check
	}//end single check
	else if ((fcheck_state == ONCHANGE) && NR_admit_change)	//Admittance change has been flagged
	{
		perform_check = true;	//Flag the check
	}//end onchange check
	else if ((fcheck_state == SWITCHING) && NR_admit_change)	//Admittance change has been flagged
	{
		perform_check = true;
	}
	else if (fcheck_state == ALLT)	//Must be every iteration then
	{
		perform_check = true;	//Flag the check
	}
	else if (force_reassociation)
	{
		perform_check = true;	//Flag the check - easist way to force the reassociation
	}
	else	//Doesn't fit one of the above
	{
		perform_check = false;	//Flag not-a-check
	}

	if (perform_check)	//Each "check" is identical - split out here to avoid 3x replication
	{
		//See if restoration is present - if not, proceed without it
		if ((restoration_object != nullptr) && !fault_check_override_mode)
		{
			//See if we've linked our function yet or not
			if (restoration_fxn == nullptr)
			{
				//Map it
				restoration_fxn = (FUNCTIONADDR)(gl_get_function(restoration_object,"perform_restoration"));

				//Check it
				if (restoration_fxn == nullptr)
				{
					GL_THROW("Unable to map restoration function");
					/*  TROUBLESHOOT
					While attempting to map the restoration configuration function, an error occurred.  Please
					try again.  If the error persists, please submit your code and a bug report via the ticketing system.
					*/
				}
			}//End restoration function map

			//Mandate "mesh mode" for this, just because I say so
			if (reliability_search_mode)	//Radial
			{
				gl_warning("fault_check interaction with restoration requires meshed checking mode - enabling this now");
				/*  TROUBLESHOOT
				The restoration algorithm is improved by using the meshed mode of topology checking for faults.  This has
				been automatically enabled for the object.  If this is undesired, do not use fault_check with restoration.
				*/

				//Set it to mesh
				reliability_search_mode = false;
			}

			//Call restoration -- fault_checks will occur as part of this
			return_val = ((int (*)(OBJECT *,int))(*restoration_fxn))(restoration_object,-99);

			//Make sure it worked
			if (return_val != 1)
			{
				GL_THROW("Restoration failed to execute properly");
				/*  TROUBLESHOOT
				While attempting to call the restoration/reconfiguration, an error was encountered in the function.
				Please look for other error messages from restoration itself.
				*/
			}

			//Now break into normal check, just to do one last one to make sure things work
		}//End restoration object not null

		//Perform the appropriate check -- occurs one more time after restoration (if it was called)
		if (reliability_search_mode)
		{
			//Call the connectivity check
			support_check(0);

			if (fcheck_state != SWITCHING)  //Parse the list - see if anything is broken
			{
				for (index=0; index<NR_bus_count; index++)
				{
					if ((Supported_Nodes[index][0] == 0) || (Supported_Nodes[index][1] == 0) || (Supported_Nodes[index][2] == 0))
					{
						if (output_filename[0] != '\0')	//See if there's an output
						{
							write_output_file(t0,0);	//Write it
						}

						//See what mode we are in - only check reliability mode if we aren't in grid association
						if (!reliability_mode && !fault_check_override_mode && !grid_association_mode)
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
		}
		else	//Mesh, maybe
		{
			//Do the grid association check (if needed)
			//****************** NOTE - is there a better way to do this with islands now -- need to do twice (pre/post) to catch stragglers! ****//
			if (grid_association_mode)
			{
				associate_grids();
			}

			//Overall check -- catches the first run, but we'll keep it here anyways
			//Internal check, for random "islands"
			if (NR_curr_bus != NR_bus_count)
			{
				GL_THROW("fault_check: Incomplete initialization detected - this will cause issues");
				/*  TROUBLESHOOT
				The fault_check object's topology checking routine tried to run, but detected a system that
				was not completely initialized.  Look for other output message relating to completely isolated
				and unconnected nodes.  These must be fixed (and islanded via a switch) before the system can proceed.
				*/
			}
			else	//Populated, onward!
			{
				//Call the connectivity check
				support_check_mesh();
			}

			//Now do an alteration pass - update (in case something changed)
			support_search_links_mesh();

			//Do the grid association check (if needed)
			if (grid_association_mode)
			{
				associate_grids();
			}

			override_output = output_check_supported_mesh();	//See if anything changed

			//If full output, do an initial dump
			if ((prev_time == 0) && full_print_output && (fcheck_state != SWITCHING))
			{
				//Do a write if one wasn't going to happen - don't override the flag
				if (!override_output && (output_filename[0] != '\0'))
				{
					write_output_file(t0,0);	//Write it
				}
				//Default else, we were going to write anyways
			}
			//Default else - not a first timestep or the mode of interest, so just go like normal

			//See if anything broke
			if (override_output && fcheck_state != SWITCHING)
			{
				if (output_filename[0] != '\0')	//See if there's an output
				{
					write_output_file(t0,0);	//Write it
				}

				//See what mode we are in - only "fail" if standard/legacy checking
				if (!reliability_mode && !fault_check_override_mode && !grid_association_mode)
				{
					GL_THROW("Unsupported phase on a possibly meshed node");
					/*  TROUBLESHOOT
					An unsupported connection was found on the system.  Since reconfiguration is not enabled,
					the solver will possibly fail here on the next iteration, so the system is broken early.
					*/
				}
				else	//Must be in reliability mode or one of the proper handling modes
				{
					gl_warning("Unsupported phase on a possibly meshed node");
					/*  TROUBLESHOOT
					An unsupported connection was found on the system.  Since reliability is enabled,
					the solver will simply ignore the unsupported components for now.
					*/
				}
			}
		}
	}//End check

	//Update previous timestep info
	prev_time = t0;

	//See if we were super-special mode
	if (fault_check_override_mode)
	{
		gl_error("fault_check:%d %s -- Special debug mode utilized, simulation terminating",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The special "SINGLE_DEBUG" mode has been activated in fault_check.  This performs a topology check ignoring some of the phase checking
		built into powerflow.  This is meant only for debugging topological issues, so the simulation is terminated prematurely, regardless of if
		any issues were found or not.
		*/
		return TS_INVALID;
	}
	else	//Standard mode
	{
		return tret;	//Return where we want to go.  If >t0, NR will force us back anyways
	}
}


void fault_check::search_links(int node_int)
{
	unsigned int index, indexb;
	bool both_handled, from_val, proceed_in;
	int branch_val;
	BRANCHDATA temp_branch;
	unsigned char work_phases;

	gl_verbose ("  fault_check::search_links:%s", NR_busdata[node_int].name);
	//Loop through the connectivity and populate appropriately
	for (index=0; index<NR_busdata[node_int].Link_Table_Size; index++)	//parse through our connected link
	{
		temp_branch = NR_branchdata[NR_busdata[node_int].Link_Table[index]];	//Get connecting link information

		proceed_in = false;			//Flag that we need to go the next link in

		//Check for no phase or open condition
		if (((temp_branch.phases & 0x07) != 0x00) && (*temp_branch.status == LS_CLOSED))
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
					if (!both_handled)
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
void fault_check::search_links_mesh(int node_int)
{
	unsigned int index, device_value, node_value;
	unsigned char temp_phases, temp_compare_phases, result_phases;

	gl_verbose ("  fault_check::search_links_mesh:%s", NR_busdata[node_int].name);
	//Check our entry mode -- if grid association mode, do this as a recursion
	if (!grid_association_mode)	//Nope, do "normally"
	{
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
			if (*NR_branchdata[device_value].status == LS_CLOSED)
			{
				temp_phases = NR_branchdata[device_value].phases;
			}
			else
			{
				temp_phases = 0x00;
			}

			//See if either end has any valid phases
			if ((valid_phases[node_int] != 0x00) || (valid_phases[node_value] != 0x00))
			{
				//Are we a switch
				if ((NR_branchdata[device_value].lnk_type == 4) || (NR_branchdata[device_value].lnk_type == 5) || (NR_branchdata[device_value].lnk_type == 6))
				{
					if (*NR_branchdata[device_value].status == 1)
					{
						temp_phases |= NR_branchdata[device_value].origphases & 0x07;
					}
				}
				else if (NR_branchdata[device_value].lnk_type == 3)	//Fuse
				{
					//See if it is "base closed"
					if (*NR_branchdata[device_value].status == 1)
					{
						//In-service -- see which phases are active and create a mask
						result_phases = (((~NR_branchdata[device_value].faultphases) & 0x07) | 0xF8);

						//Mask out the original
						temp_phases |= NR_branchdata[device_value].origphases & result_phases;
					}
					else	//Full open - just ignore it
					{
						temp_phases = 0x00;
					}
				}
				else
				{
					//Make sure enabled - the copy possible phases
					if (*NR_branchdata[device_value].status == LS_CLOSED)
					{
						temp_phases |= NR_branchdata[device_value].origphases & 0x07;
					}
					else
					{
						temp_phases = 0x00;
					}
				}
			}

			//Populate the phase information
			valid_phases[node_value] |= (valid_phases[node_int] & temp_phases);
		}//End of node link table traversion
	}//End not grid association mode
	else	//Grid association mode
	{
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
			if (*NR_branchdata[device_value].status == LS_CLOSED)
			{
				temp_phases = NR_branchdata[device_value].phases;
			}
			else
			{
				temp_phases = 0x00;
			}

			//See if either end has any valid phases
			if ((valid_phases[node_int] != 0x00) || (valid_phases[node_value] != 0x00))
			{
				//Are we a switch
				if ((NR_branchdata[device_value].lnk_type == 4) || (NR_branchdata[device_value].lnk_type == 5) || (NR_branchdata[device_value].lnk_type == 6))
				{
					if (*NR_branchdata[device_value].status == 1)
					{
						temp_phases |= NR_branchdata[device_value].origphases & 0x07;
					}
				}
				else if (NR_branchdata[device_value].lnk_type == 3)	//Fuse
				{
					//See if it is "base closed"
					if (*NR_branchdata[device_value].status == 1)
					{
						//In-service -- see which phases are active and create a mask
						result_phases = (((~NR_branchdata[device_value].faultphases) & 0x07) | 0xF8);

						//Mask out the original
						temp_phases |= NR_branchdata[device_value].origphases & result_phases;
					}
					else	//Full open - just ignore it
					{
						temp_phases = 0x00;
					}
				}
				else
				{
					//Make sure enabled - the copy possible phases
					if (*NR_branchdata[device_value].status == LS_CLOSED)
					{
						temp_phases |= NR_branchdata[device_value].origphases & 0x07;
					}
					else
					{
						temp_phases = 0x00;
					}
				}
			}

			//Check our "contributions" against the other end - use this to determine recursion
			temp_compare_phases = (valid_phases[node_value] | (valid_phases[node_int] & temp_phases));

			//See if it is the same
			if (valid_phases[node_value] != temp_compare_phases)
			{
				//Populate the phase information - store what we just did (no point doing twice)
				valid_phases[node_value] = temp_compare_phases;

				//Recurse in, do this node now
				search_links_mesh(node_value);
			}
			//Default else -- they match, so don't bother
		}//End of node link table traversion
	}//End grid association mode
}

void fault_check::support_check(int swing_node_int)
{
	unsigned int index;
	unsigned char phase_vals;

	gl_verbose ("  fault_check::support_check:%s", NR_busdata[swing_node_int].name);
	//Reset the node status list
	reset_support_check();

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
void fault_check::support_check_mesh(void)
{
	unsigned int indexa, indexb;

	gl_verbose ("  fault_check::support_check_mesh");
	//Reset the node status list
	reset_support_check();

	if (!grid_association_mode)	//Not needing to do grid association, use normal, inefficient method
	{
		//Swing node has support - if the phase exists (changed for complete faults)
		valid_phases[0] = NR_busdata[0].phases & 0x07;

		//Call the node link-erator (node support check) - call it on the swing, the details are handled inside
		//Supports possibly meshed topology - brute force method
		for (indexa=0; indexa<NR_bus_count; indexa++)
		{
			for (indexb=0; indexb<NR_bus_count; indexb++)
			{
				//Call the search link on individual nodes
				search_links_mesh(indexb);
			}
		}
	}
	else	//Grid association mode, do slightly different
	{
		//Traverse the whole bus list, just in case (since may be altered in the future)
		for (indexa=0; indexa<NR_bus_count; indexa++)
		{
			//See if we're a SWING node
			if ((NR_busdata[indexa].type == 2) || ((NR_busdata[indexa].type == 3) && NR_busdata[indexa].swing_functions_enabled) || ((*NR_busdata[indexa].busflag & NF_ISSOURCE) == NF_ISSOURCE))	//SWING node, of some form
			{
				//Check and see if we've apparently been "sourced" before
				if ((NR_busdata[indexa].phases & 0x07) != valid_phases[indexa])	//We don't match, so something came to us, but not the other way
				{
					//Extract our phases
					valid_phases[indexa] |= (NR_busdata[indexa].phases & 0x07);

					//Call the support check
					search_links_mesh(indexa);
				}
				//Default else -- we were already handled
			}
			//Default else -- not a swing
		}
	}
}

void fault_check::reset_support_check(void)
{
	unsigned int index;

	gl_verbose ("  fault_check::reset_support_check");
	//Reset the node - 0 = unsupported, 1 = supported (not populated here), 2 = N/A (no phase there)
	for (index=0; index<NR_bus_count; index++)
	{
		if (reliability_search_mode)	//Strictly radial
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

void fault_check::write_output_file(TIMESTAMP tval, double tval_delta)
{
	unsigned int index, ret_value;
	DATETIME temp_time;
	bool headerwritten = false;
	bool supportheaderwritten = false;
	bool deltamodeflag;
	char phase_outs;
	char deltaprint_buffer[64];
	FILE *FPOutput;

	//Do a deltamode check - fault check is not really privy to what mode we're in -- it doesn't care
	if (tval == 0)	//Deltamode flag
	{
		deltamodeflag=true;
	}
	else	//Nope
	{
		deltamodeflag=false;
	}

	//open the file
	FPOutput = fopen(output_filename,"at");

	for (index=0; index<NR_bus_count; index++)	//Loop through all bus values - find the unsupported section
	{
		if (reliability_search_mode)
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
			if (!headerwritten)
			{
				if (!fault_check_override_mode)
				{
					if (deltamodeflag)
					{
						//Convert the current time to an output
						ret_value = gl_printtimedelta(tval_delta,deltaprint_buffer,64);

						//Write it
						fprintf(FPOutput,"Unsupported at timestamp %0.9f - %s =\n\n",tval_delta,deltaprint_buffer);
					}
					else	//Event-based mode
					{
						//Convert timestamp so readable
						gl_localtime(tval,&temp_time);

						fprintf(FPOutput,"Unsupported at timestamp %lld - %04d-%02d-%02d %02d:%02d:%02d =\n\n",tval,temp_time.year,temp_time.month,temp_time.day,temp_time.hour,temp_time.minute,temp_time.second);
					}
				}
				else	//Special debug mode
				{
					//Write it
					fprintf(FPOutput,"Special debug topology check -- phase checks bypassed -- Unsupported nodes list\n\n");
				}

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

	//See if we made it here without writing a header -- if we're in special mode, indicate as much
	if (!headerwritten && fault_check_override_mode)
	{
		fprintf(FPOutput,"Special debug topology check -- phase checks bypassed -- No unsupported nodes found\n\n");
	}

	//Check and see if we want supported nodes too
	if (full_print_output)
	{
		for (index=0; index<NR_bus_count; index++)	//Loop through all bus values - find the unsupported section
		{
			phase_outs = (NR_busdata[index].phases & 0x07);

			if (phase_outs != 0x00)	//Supported
			{
				//See which mode we are in
				if (!fault_check_override_mode)	//Standard mode
				{
					//See if the header's been written
					if (!headerwritten)
					{
						if (deltamodeflag)
						{
							//Convert the current time to an output
							ret_value = gl_printtimedelta(tval_delta,deltaprint_buffer,64);

							//Write it
							fprintf(FPOutput,"Supported at timestamp %0.9f - %s =\n",tval_delta,deltaprint_buffer);

							//Check the mode -- see if we need an island summary
							if (!grid_association_mode)
							{
								fprintf(FPOutput,"\n");	//Extra line break
							}
							else //Write an island count
							{
								//Write it the island summary
								fprintf(FPOutput,"Number detected islands = %d\n\n",NR_islands_detected);
							}
						}
						else	//Event-based mode
						{
							//Convert timestamp so readable
							gl_localtime(tval,&temp_time);

							fprintf(FPOutput,"Supported at timestamp %lld - %04d-%02d-%02d %02d:%02d:%02d =\n",tval,temp_time.year,temp_time.month,temp_time.day,temp_time.hour,temp_time.minute,temp_time.second);

							if (!grid_association_mode)
							{
								fprintf(FPOutput,"\n");	//Extra line break
							}
							else
							{
								//Print the island count estimate
								fprintf(FPOutput,"Number detected islands = %d\n\n",NR_islands_detected);
							}
						}

						headerwritten = true;	//Flag it as written
						supportheaderwritten = true;	//Flag intermediate as written too
					}
					else if (!supportheaderwritten)	//Tiemstamp written, but not second header
					{
						if (!grid_association_mode)
						{
							fprintf(FPOutput,"\nSupported Nodes\n");
						}
						else
						{
							fprintf(FPOutput,"\nSupported Nodes -- %d Islands detected\n",NR_islands_detected);
						}

						supportheaderwritten = true;	//Flag intermediate as written too
					}
				}
				else	//Special debug mode
				{
					//See if anything is written
					if (!headerwritten)
					{
						if (!grid_association_mode)
						{
							//Write it
							fprintf(FPOutput,"Special debug topology check -- phase checks bypassed -- Supported nodes list\n\n");
						}
						else
						{
							//Write it
							fprintf(FPOutput,"Special debug topology check -- phase checks bypassed -- Supported nodes list -- %d Islands detected\n\n",NR_islands_detected);
						}

						//Set flags, just in case
						headerwritten = true;
						supportheaderwritten = true;
					}
					else if (!supportheaderwritten)
					{
						if (!grid_association_mode)
						{
							fprintf(FPOutput,"\nSupported Nodes\n");
						}
						else
						{
							fprintf(FPOutput,"\nSupported Nodes -- %d Islands detected\n",NR_islands_detected);
						}

						supportheaderwritten = true;	//Flag intermediate as written too
					}
					//Default else -- they've both bee written
				}

				//Figure out the "supported" structure
				switch (phase_outs) {
					case 0x01:	//Only C
						{
							fprintf(FPOutput,"Phase C on node %s",NR_busdata[index].name);
							break;
						}
					case 0x02:	//Only B
						{
							fprintf(FPOutput,"Phase B on node %s",NR_busdata[index].name);
							break;
						}
					case 0x03:	//B and C supported
						{
							fprintf(FPOutput,"Phases B and C on node %s",NR_busdata[index].name);
							break;
						}
					case 0x04:	//Only A
						{
							fprintf(FPOutput,"Phase A on node %s",NR_busdata[index].name);
							break;
						}
					case 0x05:	//A and C supported
						{
							fprintf(FPOutput,"Phases A and C on node %s",NR_busdata[index].name);
							break;
						}
					case 0x06:	//A and B supported
						{
							fprintf(FPOutput,"Phases A and B on node %s",NR_busdata[index].name);
							break;
						}
					case 0x07:	//All three supported
						{
							fprintf(FPOutput,"Phases A, B, and C on node %s",NR_busdata[index].name);
							break;
						}
					default:	//How'd we get here?
						{
							GL_THROW("Error parsing supported phases on node %s",NR_busdata[index].name);
							/*  TROUBLESHOOT
							The output file writing routine for the fault_check object encountered a problem
							determining which phases are supported on the indicated node.  Please try again.
							If the error persists, submit your code and a bug report using the trac website.
							*/
						}
				}//End case

				//Common write portions
				//Print extra information, if needed
				if (grid_association_mode)
				{
					fprintf(FPOutput," - Island %d",(NR_busdata[index].island_number+1));
				}

				//See if we're a SWING-enabled bus
				if (NR_busdata[index].swing_functions_enabled)
				{
					fprintf(FPOutput," - SWING-enabled\n");
				}
				else
				{
					fprintf(FPOutput,"\n");
				}
			}//end supported
		}//end secondary bus bus traversion
	}//End proper reliability mode

	fprintf(FPOutput,"\n");	//Put some blank lines between time entries

	//Close the file
	fclose(FPOutput);
}

//Function to traverse powerflow and remove/restore now unsupported objects' phases (so NR solves happy)
void fault_check::support_check_alterations(int baselink_int, bool rest_mode)
{
	int base_bus_val, return_val;
	double delta_ts_value;
	TIMESTAMP event_ts_value;

	if (prev_time == 0)	//First run - see if restoration exists (we need it for now)
	{
		allocate_alterations_values(true);
	}

	//Check the mode
	if (reliability_search_mode)	//Strictly radial assumption, continue as always
	{
		//See if the "faulting branch" is the swing node
		if ((baselink_int == -99) || (baselink_int == -77))	//Swing or initial-type fault
			base_bus_val = 0;	//Swing is always 0th position
		else	//Not swing, see what our to side is
		{
			gl_verbose ("  fault_check::support_check_alterations:%s:%d", NR_branchdata[baselink_int].name, rest_mode);
			//Get to side
			base_bus_val = NR_branchdata[baselink_int].to;

			//Assuming radial, now make the system happy by removing/restoring unsupported phases
			if (rest_mode)	//Restoration
				NR_busdata[base_bus_val].phases |= (NR_branchdata[baselink_int].phases & 0x07);
			else	//Removal mode
				NR_busdata[base_bus_val].phases &= (NR_branchdata[baselink_int].phases & 0x07);
		}

		//Reset the alteration matrix
		reset_alterations_check();

		//First node is handled, one way or another
		Alteration_Nodes[base_bus_val] = 1;

		//See if the FROM side of our newly restored greatness is supported.  If it isn't, there's no point in proceeding
		if (rest_mode)	//Restoration
		{
			gl_verbose("fault_check: alterations support check called restoration on bus %s with phases %d",NR_busdata[base_bus_val].name, int(NR_busdata[base_bus_val].phases));

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
			gl_verbose("fault_check: alterations support check called removal on bus %s with phases %d",NR_busdata[base_bus_val].name, int(NR_busdata[base_bus_val].phases));

			//Recurse our way in - adjusted version of original search_links function above (but no storage, because we don't care now)
			support_search_links(base_bus_val, base_bus_val, rest_mode);
		}
	}//End strictly radial assumption
	else	//Not assumed to be strictly radial, or just being safe -- check EVERYTHING
	{
		//See if we should even go in first
		if (!restoration_checks_active)
		{
			//Now see if the restoration object and function are mapped
			if ((restoration_object != nullptr) && (restoration_fxn != nullptr))
			{
				//Call restoration -- fault_checks will occur as part of this
				return_val = ((int (*)(OBJECT *,int))(*restoration_fxn))(restoration_object,baselink_int);

				//Make sure it worked
				if (return_val != 1)
				{
					GL_THROW("Restoration failed to execute properly");
					/*  TROUBLESHOOT
					While attempting to call the restoration/reconfiguration, an error was encountered in the function.
					Please look for other error messages from restoration itself.
					*/
				}
			}
			//Default else -- not mapped, so do nothing
		}//End restoration checks active
		//Default else -- no restoration or we're in "exploratory" mode

		//Reset the alteration matrix
		reset_alterations_check();

		//Do the grid association check (if needed)
		//****************** NOTE - is there a better way to do this with islands now -- need to do twice (pre/post) to catch stragglers! ****//
		if (grid_association_mode)
		{
			associate_grids();
		}

		//Call a support check -- reset handled inside
		//Always assumed to NOT be in "restoration object" mode
		support_check_mesh();

		//Now loop through and remove those components that are not supported anymore -- start from SWING, just because we have to start somewhere
		support_search_links_mesh();

		//Do the grid association check (if needed)
		if (grid_association_mode)
		{
			associate_grids();
		}
	}//End not strictly radial assumption

	//Determine if an output is desired and if we're not in restoration check mode (otherwise, it may flood the output log)
	if (!restoration_checks_active && (output_filename[0] != '\0'))
	{
		//See if we're a deltamode write or not
		if (deltatimestep_running > 0.0)
		{
			//Get current delta time
			delta_ts_value = gl_globaldeltaclock;

			//Zero event time
			event_ts_value = 0;
		}
		else	//Event mode - write that instead
		{
			//Get event time
			event_ts_value = gl_globalclock;

			//Zero the delta time
			delta_ts_value = 0.0;
		}

		//Perform the actual write -- may duplicate times (if an event calls after sync, but shows what may be changing from an event)
		write_output_file(event_ts_value,delta_ts_value);
	}//End write an output
	//Default else -- no output (none desired, or in restoration calls)
}

//Recursive function to traverse powerflow and alter phases as necessary
//Checks against support found, not an assumed radial traversion (little slower, but more thorough)
//Based on support_search_links below
void fault_check::support_search_links_mesh(void)
{
	unsigned int indexval, index;
	int device_index;
	unsigned char temp_phases, work_phases;

	//Traverse the bus list - figure out what has support or not
	for (indexval=0; indexval<NR_bus_count; indexval++)
	{
		//Check our location in the support check -- see if we're supported or not and alter as necessary

		//Mask out our phases -- see what is available - mostly just to avoid alterations if nothing changed
		temp_phases = NR_busdata[indexval].phases & 0x07;

		//See if any change is needed
		if (temp_phases != valid_phases[indexval])
		{
			//Figure out what should be valid - base phase values
			work_phases = NR_busdata[indexval].origphases & valid_phases[indexval];

			//See if we have any valid phases
			if (work_phases != 0x00)
			{
				//See if we're triplex
				if ((NR_busdata[indexval].origphases & 0x80) == 0x80)
				{
					work_phases |= (NR_busdata[indexval].origphases & 0xE0);	//SP, House?, To SPCT - flagged on
				}
				else if (work_phases == 0x07)	//Fully connected, we can pass D and diff conns
				{
					work_phases |= (NR_busdata[indexval].origphases & 0x18);	//D
				}
			}
			//Default else - we're completely valid phase-less, so just set to no phases

			//Apply the change to the node
			NR_busdata[indexval].phases = work_phases;

			//Loop through our links and adjust anyone necessary
			for (index=0; index<NR_busdata[indexval].Link_Table_Size; index++)	//parse through our connected link
			{
				//Extract the branchdata reference
				device_index = NR_busdata[indexval].Link_Table[index];

				//See if it has already been handled -- no sense in duplicating items
				if (Alteration_Links[device_index] == 0)	//Not handled
				{
					//See which phases are even allowed - initial mask of from/to valid phases
					temp_phases = (valid_phases[NR_branchdata[device_index].from] & valid_phases[NR_branchdata[device_index].to]);

					//See how these compare with what we can do
					work_phases = NR_branchdata[device_index].origphases & temp_phases;

					//Make sure it's non-zero
					if (work_phases != 0x00)	//We have some valid phasing
					{
						//See if we were original triplex-oriented
						if ((NR_branchdata[device_index].origphases & 0x80) == 0x80)
						{
							work_phases |= (NR_branchdata[device_index].origphases & 0xE0);	//Mask in SPCT-type flags
						}
					}//End valid phases
					//Default else - it is 0x00, so no phases present

					//Set us - if closed
					if (*NR_branchdata[device_index].status == LS_CLOSED)
					{
						NR_branchdata[device_index].phases = work_phases;
					}
					else
					{
						NR_branchdata[device_index].phases = 0x00;	//Open, so reset phases
					}

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
	}//End bus traversion list
	//Should be done -- links should have been caught by respective bus values
}

//Code to check the special link devices and modify as needed
void fault_check::special_object_alteration_handle(int branch_idx)
{
	int return_val;
	unsigned char temp_phases;
	OBJECT *temp_obj = nullptr;
	FUNCTIONADDR funadd = nullptr;

	gl_verbose ("  fault_check::special_object_alteration_handle:%s", NR_branchdata[branch_idx].name);
	//See which mode we're in -- bypass if needed (might be always)
	if (!meshed_fault_checking_enabled)
	{
		//See if we're a switch - if so, call the appropriate function
		if (NR_branchdata[branch_idx].lnk_type == 4)
		{
			//Find this object
			temp_obj = NR_branchdata[branch_idx].obj;

			//Make sure it worked
			if (temp_obj == nullptr)
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
			if (funadd==nullptr)
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
			temp_obj = NR_branchdata[branch_idx].obj;

			//Make sure it worked
			if (temp_obj == nullptr)
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
			if (funadd==nullptr)
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
			temp_obj = NR_branchdata[branch_idx].obj;

			//Make sure it worked
			if (temp_obj == nullptr)
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
			if (funadd==nullptr)
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
			temp_obj = NR_branchdata[branch_idx].obj;

			//Make sure it worked
			if (temp_obj == nullptr)
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
			if (funadd==nullptr)
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
	}//End "normal" reliability operations
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

	gl_verbose ("  fault_check::support_search_links:%s:%s:%d", NR_busdata[node_int].name, NR_busdata[node_start].name, impact_mode);
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

		if ((node_int == node_start) && !from_val)	//We're the TO side of the base node, Oh Noes!
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
		if (!both_handled)
		{
			//Figure out the indexing so we can tell what we are
			if (from_val)	//From end
			{
				branch_val = temp_branch.to;

				if (!impact_mode)	//Removal time
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

						//Check our status
						if (*NR_branchdata[NR_busdata[node_int].Link_Table[index]].status == LS_CLOSED)
						{
							phase_restrictions &= (NR_branchdata[NR_busdata[node_int].Link_Table[index]].origphases & 0x07);	//Mask this with what we used to be
						}
						else
						{
							phase_restrictions = 0x00;	//Nothing available
						}

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

				if (!impact_mode)	//Removal time
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

						//Check our status
						if (*NR_branchdata[NR_busdata[node_int].Link_Table[index]].status == LS_CLOSED)
						{
							phase_restrictions &= (NR_branchdata[NR_busdata[node_int].Link_Table[index]].origphases & 0x07);	//Mask this with what we used to be
						}
						else
						{
							phase_restrictions = 0x00;	//Nothing available
						}

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

	gl_verbose ("  fault_check::reset_alterations_check");
	//Do a check for initialization
	if (Alteration_Nodes == nullptr)
	{
		allocate_alterations_values(true);
	}

	//Reset the node - just whether the algorithm has looked at it or not
	for (index=0; index<NR_bus_count; index++)
	{
		Alteration_Nodes[index] = 0;	//Flag as untouched
	}

	//If we're in "special" mode, reset the branches too
	if (!reliability_search_mode)
	{
		for (index=0; index<NR_branch_count; index++)
		{
			Alteration_Links[index] = 0;	//Flag as untouched
		}
	}
}

//FUnctionalized version of the allocation routine, mainly so it can be called by
//events that start before fault_check's presync has occurred.
void fault_check::allocate_alterations_values(bool reliability_mode_bool)
{
	unsigned int index;
	
	gl_verbose ("  fault_check::allocate_alterations_values:%d", reliability_mode_bool);
	//Make sure we haven't been allocated before
	if (Supported_Nodes == nullptr)
	{
		if (restoration_object==nullptr && fcheck_state != SWITCHING)
		{
			gl_verbose("Restoration object not detected!");	//Put down here because the variable may not be populated in time for init
			/*  TROUBLESHOOT
			The fault_check object can use a restoration object in the system.  If the restoration object is present,
			unsuccessful node support checks will call the reconfiguration.
			*/
		}

		//Create our node reference vector - one for each bus
		Supported_Nodes = (unsigned int**)gl_malloc(NR_bus_count*sizeof(unsigned int*));
		
		if (Supported_Nodes == nullptr)
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

			if (Supported_Nodes[index] == nullptr)
			{
				GL_THROW("fault_check: node status vector allocation failure");
				//Defined above
			}
		}

		//Replicate the above if reliability is running, or we're in mesh mode
		if (reliability_mode_bool || !reliability_search_mode)
		{
			//Create our node reference vector - one for each bus
			Alteration_Nodes = (char*)gl_malloc(NR_bus_count*sizeof(char));
			if (Alteration_Nodes == nullptr)
			{
				GL_THROW("fault_check: node alteration status vector allocation failure");
				/*  TROUBLESHOOT
				The fault_check object has failed to allocate the node alteration information vector.  Please try
				again and if the problem persists, submit your code and a bug report via the trac website.
				*/
			}

			//Populate mesh-based items if necessary too
			if (!reliability_search_mode)
			{
				Alteration_Links = (char*)gl_malloc(NR_branch_count*sizeof(char));

				//Check it
				if (Alteration_Links == nullptr)
				{
					GL_THROW("fault_check: link alteration status vector allocation failure");
					/*  TROUBLESHOOT
					The fault_check object has failed to allocate the node alteration information vector.  Please try
					again and if the problem persists, submit your code and a bug report via the trac website.
					*/
				}
			}//End additional allocations

			//Populate the phases field too
			valid_phases = (unsigned char*)gl_malloc(NR_bus_count*sizeof(unsigned char));

			//Check it
			if (valid_phases == nullptr)
			{
				GL_THROW("fault_check: node alteration status vector allocation failure");
				//Defined above
			}

			//Apply the general reset
			reset_alterations_check();
		}//End reliability mode allocing
	}//End allocations actually needed
}

//Function to progress downwards and flag momentary interruptions
void fault_check::momentary_activation(int node_int)
{
	unsigned int index;
	OBJECT *tmp_obj;
	bool *momentary_flag;
	PROPERTY *pval;

	//See if we are a meter or triplex meter
	tmp_obj = NR_busdata[node_int].obj;

	//Make sure it worked
	if (tmp_obj == nullptr)
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
		if (pval == nullptr)
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

//Multiple grid checking items - allocate/reset array
void fault_check::reset_associated_grid(void)
{
	unsigned int indexval;

	//Loop through and reset the bus indicators
	for (indexval=0; indexval<NR_bus_count; indexval++)
	{
		NR_busdata[indexval].island_number = -1;	//Starts as "not associated"
	}

	//Reset the links too, just for giggles
	for (indexval=0; indexval<NR_branch_count; indexval++)
	{
		NR_branchdata[indexval].island_number = -1;	//Start as "not associated" as well
	}
}

//Multiple grid checking items - search for SWING nodes as the search entry points, then populate
void fault_check::associate_grids(void)
{
	unsigned int indexval;
	int grid_counter;
	STATUS stat_return_val;

	//Call the reset/allocation routine
	reset_associated_grid();

	//Set the counter
	grid_counter = 0;

	//Parse the busdata list to find these - enter as we go
	//Do a full traverse, since this may swap to "just has a source" later
	for (indexval=0; indexval<NR_bus_count; indexval++)
	{
		//See if we're a SWING node
		if (NR_busdata[indexval].type == 2)	//SWING bus
		{
			//See if we're already flagged
			if (NR_busdata[indexval].island_number == -1)	//We're still unparsed
			{
				//See if we have phases
				if ((NR_busdata[indexval].phases & 0x07) != 0x00)
				{
					//Call the associater routine
					search_associated_grids(indexval,grid_counter);

					//Increment the counter, when we're done
					grid_counter++;
				}
				//Default else -- no phases, so pretend it still doesn't exist
			}
			//Default else, we've already been hit, skip out
		}
		//Default else, keep going to look for one
	}

	//Second loop - SWING_PQ check
	//********* TODO: Better/more efficient way to do this!?!? **********/
	for (indexval=0; indexval<NR_bus_count; indexval++)
	{
		//See if we're a SWING_PQ node
		if (NR_busdata[indexval].type == 3)	//SWING_PQ bus
		{
			//See if we're already flagged
			if (NR_busdata[indexval].island_number == -1)	//We're still unparsed
			{
				//See if we have phases
				if ((NR_busdata[indexval].phases & 0x07) != 0x00)
				{
					//Flag us as a swing - to be safe
					NR_busdata[indexval].swing_functions_enabled = true;

					//Flag us as the topological entry point
					NR_busdata[indexval].swing_topology_entry = true;

					//Call the associater routine
					search_associated_grids(indexval,grid_counter);

					//Increment the counter, when we're done
					grid_counter++;
				}
				//Default else -- no phases, so pretend it still doesn't exist
			}
			else	//Deflag us as a swing
			{
				NR_busdata[indexval].swing_functions_enabled = false;

				//Leave the "swing_topology_entry" flag as-is - used as flag to determine if a SWING_PQ
				//with a generator suddenly became a main source
			}
			//Default else, we've already been hit, skip out
		}
		//Default else, keep going to look for one
	}

	//Third loop -- look for NF_ISSOURCE flags - anything attached to this, should be active
	for (indexval=0; indexval<NR_bus_count; indexval++)
	{
		//See if we're a source-flagged node
		if ((*NR_busdata[indexval].busflag & NF_ISSOURCE) == NF_ISSOURCE)	//Source flagged
		{
			//See if we're already flagged
			if (NR_busdata[indexval].island_number == -1)	//We're still unparsed
			{
				//See if we have phases
				if ((NR_busdata[indexval].phases & 0x07) != 0x00)
				{
					//Call the associater routine
					search_associated_grids(indexval,grid_counter);

					//Increment the counter, when we're done
					grid_counter++;
				}
				//Default else -- no phases, so pretend it still doesn't exist
			}
			//Default else, we've already been hit, skip out
		}
		//Default else, keep going to look for one
	}

	//See if it is different from the "existing count"
	if (NR_islands_detected != grid_counter)
	{
		//See if we need to free anything first - basically, see if it already exists
		if ((NR_powerflow.island_matrix_values != nullptr) && (NR_islands_detected != 0))
		{
			stat_return_val = NR_array_structure_free(&NR_powerflow,NR_islands_detected);

			//Make sure it worked
			if (stat_return_val == FAILED)
			{
				GL_THROW("fault_check: Failed to free up a multi-island NR solver array properly");
				/*  TROUBLESHOOT
				While attempting to free up one of the multi-island solver variable arrays, an error
				was encountered.  Please try again.  If the error persists, please submit your code and
				a bug report via the ticketing/issues system.
				*/
			}
		}

		//Now allocate new ones
		stat_return_val = NR_array_structure_allocate(&NR_powerflow,grid_counter);

		//Make sure it worked
		if (stat_return_val == FAILED)
		{
			GL_THROW("fault_check: Failed to allocate a multi-island NR solver array properly");
			/*  TROUBLESHOOT
			While attempting to allocate a multi-island solver variable array, an error was encountered.
			Please try again.  If the error persists, please submit your code and a bug report via the
			ticketing/issue system.
			*/
		}

		//Update the overall tracker
		NR_islands_detected = grid_counter;

		//Force an NR update too, just in case
		NR_admit_change = true;
	}
	//Default else - the size is still fine (no need to update the value

	//Deflag the "force a reassociation" flag
	force_reassociation = false;
}

//Multiple grid checking items - the actual crawler
//This is recursively called
void fault_check::search_associated_grids(unsigned int node_int, int grid_counter)
{
	unsigned int index;
	int node_ref;

	//Loop through the connection table for this node
	for (index=0; index<NR_busdata[node_int].Link_Table_Size; index++)
	{
		//See which end of the link we are
		if (NR_branchdata[NR_busdata[node_int].Link_Table[index]].from == node_int)	//From end
		{
			//Set the node-ref - must be other end
			node_ref = NR_branchdata[NR_busdata[node_int].Link_Table[index]].to;
		}
		else	//Must be the to-end
		{
			//Set the node-ref, it must be us
			node_ref = NR_branchdata[NR_busdata[node_int].Link_Table[index]].from;
		}

		//We're theoretically coming from a "powered node", so see if it has any phase alignment to proceed
		//Only do "in service" items, so go on current phases, not original phases
		if (((NR_busdata[node_int].phases & 0x07) & (NR_branchdata[NR_busdata[node_int].Link_Table[index]].phases & 0x07)) != 0x00)
		{
			//See if the other side has been handled
			if (NR_busdata[node_ref].island_number == -1)
			{
				//Set the appropriate side
				NR_busdata[node_ref].island_number = grid_counter;

				//Also flag us, as the link, to be associated with this island
				NR_branchdata[NR_busdata[node_int].Link_Table[index]].island_number = grid_counter;

				//Recurse in
				search_associated_grids(node_ref,grid_counter);
			}
			else if (NR_busdata[node_ref].island_number != grid_counter)
			{
				GL_THROW("fault_check: duplicate grid assignment on node %s!",NR_busdata[node_ref].name);
				/*  TROUBLESHOOT
				While mapping the associated grid/swing node for a system, a condition was encountered where
				a node tried to belong to two different systems.  This should not have occurred.  Please submit
				your code and a bug report via the ticketing system.
				*/
			}
			else if (NR_branchdata[NR_busdata[node_int].Link_Table[index]].island_number != grid_counter)	//This line is unassigned
			{
				//See if it is fully unassigned
				if (NR_branchdata[NR_busdata[node_int].Link_Table[index]].island_number == -1)
				{
					//Just got missed by the iterations, so associate us (ends already handled, so no need to recurse)
					NR_branchdata[NR_busdata[node_int].Link_Table[index]].island_number = grid_counter;
				}
				else
				{
					//Somehow is associated with a different grid, even though the from/to are the same!
					GL_THROW("fault_check: invalid grid assignment on line %s!",NR_branchdata[NR_busdata[node_int].Link_Table[index]].name);
					/*  TROUBLESHOOT
					While mapping the associated grid/swing node for a line in the system, a condition was encountered where
					the line belonged to a grid/island it shouldn't.  This should not have occurred.  Please submit
					your code and a bug report via the ticketing system.
					*/
				}
			}
			//Default else -- already handled as this grid
		}
		//Default else, not a match, so next
	}
}

//Function to remove a divergent island
STATUS fault_check::disable_island(int island_number)
{
	int index_value;
	TIMESTAMP curr_time_val_TS;
	double curr_time_val_DBL;
	FILE *FPOutput;
	DATETIME temp_time;
	char deltaprint_buffer[64];
	unsigned int ret_value;
	bool deltamodeflag;

	//Preliminary check - see if we're even in the right mode
	if (!grid_association_mode)
	{
		gl_error("fault_check: an island removal call was made, but grid_association is set to false!");
		/*  TROUBLESHOOT
		The powerflow is attempting to run in multi-islanded mode with individual island handling, but
		fault_check is not set up properly.  Please set `grid_association true;` in the fault_check and
		try again.
		*/

		return FAILED;
	}

	//Loop through the buses -- remove if it is in this island (keep SWING functions affected though)
	for (index_value=0; index_value < NR_bus_count; index_value++)
	{
		//See if we're in the island
		if (NR_busdata[index_value].island_number == island_number)
		{
			//Just trim it off
			NR_busdata[index_value].phases &= 0xF8;

			//De-associate us too
			NR_busdata[index_value].island_number = -1;

			//Empty the valid phases property
			valid_phases[index_value] = 0x00;
		}
		//Default else -- next bus
	}

	//Do the same for branches, just so we don't get confused
	for (index_value=0; index_value < NR_branch_count; index_value++)
	{
		//Check our association
		if (NR_branchdata[index_value].island_number == island_number)
		{
			//Trim the phases 
			NR_branchdata[index_value].phases &= 0xF8;

			//De-associate us
			NR_branchdata[index_value].island_number = -1;
		}
		//Default else -- next branch
	}

	//If there's an output file, log it in there too (since this is through an "unconventional" channel)
	if (output_filename[0] != '\0')	//See if there's an output
	{
		//See which one to call/populate
		if (deltatimestep_running > 0.0)	//Deltamode
		{
			curr_time_val_TS = 0;
			curr_time_val_DBL = gl_globaldeltaclock;
			deltamodeflag = true;
		}
		else	//Steady state
		{
			curr_time_val_TS = gl_globalclock;
			curr_time_val_DBL = 0.0;
			deltamodeflag = false;
		}

		//Put a message in the output file, before the topology change (just to be clear)
		//open the file
		FPOutput = fopen(output_filename,"at");

		if (deltamodeflag == true)
		{
			//Convert the current time to an output
			ret_value = gl_printtimedelta(curr_time_val_DBL,deltaprint_buffer,64);

			//Write it
			fprintf(FPOutput,"Island %d removed from powerflow at timestamp %0.9f - %s =\n\n",(island_number+1),curr_time_val_DBL,deltaprint_buffer);
		}
		else	//Event-based mode
		{
			//Convert timestamp so readable
			gl_localtime(curr_time_val_TS,&temp_time);

			fprintf(FPOutput,"Island %d removed from powerflow at timestamp %lld - %04d-%02d-%02d %02d:%02d:%02d =\n\n",(island_number+1),curr_time_val_TS,temp_time.year,temp_time.month,temp_time.day,temp_time.hour,temp_time.minute,temp_time.second);
		}

		//Close the file
		fclose(FPOutput);

		write_output_file(curr_time_val_TS,curr_time_val_DBL);	//Write it
	}

	//Flag a forced reiteration for the next time something can (not now though, we may be halfway through an NR solver loop)
	force_reassociation = true;

	//Output it - no longer as a verbose (easy to miss)
	gl_warning("fault_check: Removed island %d from the powerflow",(island_number+1));
	/*  TROUBLESHOOT
	One of the islands of teh system was removed (by the multi-island capability).  This may have been due to a 
	divergent powerflow, or some specifically-command action.  If this was not intended, check your file and try again.
	*/

	//Not sure how we'd fail, at this point
	return SUCCESS;
}

//Function to force a rescan - used for island "rejoining" portions
STATUS fault_check::rescan_topology(int bus_that_called_reset)
{
	//Make sure this wasn't somehow called while solver_NR is working
	if (NR_solver_working)
	{
		//If it was, just return a failure -- let the calling object deal with it
		return FAILED;
	}

	//Call the "topology re-evaluation" function
	support_check_alterations(bus_that_called_reset,true);

	//If we made it this far, we win!
	return SUCCESS;
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
		if (*obj!=nullptr)
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
	while (!loop_complete)
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
					tmp_obj = NR_branchdata[branch_val].obj;

					//Make sure it worked
					if (tmp_obj == nullptr)
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
					if (Pval == nullptr)
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
					if ((NR_busdata[node_val].type == 2) || ((NR_busdata[node_val].type == 3) && NR_busdata[node_val].swing_functions_enabled))
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
		if ((index>NR_busdata[node_val].Link_Table_Size) && !proper_exit)	//Full traversion - then failure
		{
			result_val = 0.0;		//Encode as a failure
			loop_complete = true;	//Flag loop as over
		}
	}//End while

	return result_val;
}

//Function to remove a divergent island from the powerflow, so it isn't handled anymore
EXPORT STATUS powerflow_disable_island(OBJECT *thisobj, int island_number)
{
	//Fault check object link
	fault_check *fltyobj = OBJECTDATA(fault_check_object,fault_check);

	//Call the function
	return fltyobj->disable_island(island_number);
}

//Function to prompt a topology rescan, likely when a swing comes back into service after failing
EXPORT STATUS powerflow_rescan_topo(OBJECT *thisobj,int bus_that_called_reset)
{
	//Fault check object link
	fault_check *fltyobj = OBJECTDATA(fault_check_object,fault_check);

	//Call the function
	return fltyobj->rescan_topology(bus_that_called_reset);
}
/**@}**/
