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

	return result;
}

int fault_check::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);

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

	return powerflow_object::init(parent);
}

TIMESTAMP fault_check::sync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::sync(t0);
	restoration *rest_obj;
	unsigned int index;
		
	if (prev_time == 0)	//First run - see if restoration exists (we need it for now)
	{
		if (restoration_object==NULL)
		{
			GL_THROW("Restoration object not detected!");	//Put down here because the variable may not be populated in time for init
			/*  TROUBLESHOOT
			The fault_check object requires a restoration object to be in place in the system.  The restoration object
			does not necessarily have to be reconfiguring anything, it just needs to be present and running for
			information.
			*/
		}

		//Create our node reference vector (this is done once for now, may need to change in future)
		Supported_Nodes = (int*)gl_malloc(NR_bus_count*sizeof(int));
		if (Supported_Nodes == NULL)
		{
			GL_THROW("fault_check: node status vector allocation failure");
			/*  TROUBLESHOOT
			The fault_check object has failed to allocate the node information vector.  Please try
			again and if the problem persists, submit your code and a bug report via the trac website.
			*/
		}
	}

	if (NR_cycle==false)	//Getting ready to do a solution, we should see check for "unsupported" systems
	{
		//Map to the restoration object
		rest_obj = OBJECTDATA(restoration_object,restoration);

		//Call the connectivity check
		support_check(0,rest_obj->populate_tree);

		//Extra code - may be replaced or handled elsewhere
		//Go through the list now - if something is unsupported, call a reconfiguration

		for (index=0; index<NR_bus_count; index++)
		{
			if (Supported_Nodes[index]==0)
			{
				rest_obj->Perform_Reconfiguration(OBJECTHDR(this),t0);	//Request a reconfiguration

				break;	//Get us out of this loop, only need to detect a solitary failure
			}
		}
	}

	//Update previous timestep info
	prev_time = t0;

	return tret;	//Return where we want to go.  If >t0, NR will force us back anyways
}

void fault_check::search_links(unsigned int node_int)
{
	unsigned int index;
	bool both_handled, from_val;
	int branch_val, node_val;
	BRANCHDATA temp_branch;
	restoration *rest_object;

	//Loop through the connectivity and populate appropriately
	for (index=0; index<NR_busdata[node_int].Link_Table_Size; index++)	//parse through our connected link
	{
		both_handled = false;	//Reset flag

		temp_branch = NR_branchdata[NR_busdata[node_int].Link_Table[index]];	//Get connecting link information

		//See which end we are, and if the other end has been handled
		if (temp_branch.from == node_int)	//We're the from
		{
			from_val = true;	//Flag us as the from end (so we don't have to check it again later)

			if (Supported_Nodes[temp_branch.to]==1)	//Handled
				both_handled=true;
		}
		else	//Must be the to
		{
			from_val = false;	//Flag us as the to end (so we don't have to check it again later)

			if (Supported_Nodes[temp_branch.from]==1)	//Handled
				both_handled=true;
		}

		//If not handled, proceed with logic-ness
		if (both_handled==false)
		{
			//Link to the restoration object for now.  This may later become self-contained.
			rest_object = OBJECTDATA(restoration_object,restoration);

			//Figure out the indexing so we can tell what we are
			if (from_val)	//From end
				node_val = temp_branch.to;
			else
				node_val = temp_branch.from;

			//Populate the parent/child tree if wanted
			if (rest_object->populate_tree == true)
			{
				if (from_val)	//we're the from end
				{
					//This means we are the to end's parent - just write this.  If mesh or multi-sourced, this will just get overwritten
					NR_busdata[temp_branch.to].Parent_Node = temp_branch.from;

					//This also means the to end is our child
					if (NR_busdata[temp_branch.from].Child_Node_idx < NR_busdata[temp_branch.from].Link_Table_Size)	//Still in a valid range
					{
						NR_busdata[temp_branch.from].Child_Nodes[NR_busdata[temp_branch.from].Child_Node_idx] = temp_branch.to;	//Store the to value
						NR_busdata[temp_branch.from].Child_Node_idx++;	//Increment the pointer
					}
					else
					{
						GL_THROW("NR: Too many children were attempted to be populated!");
						/*  TROUBLESHOOT
						While populating the tree structure for the restoration module, a memory allocation limit
						was exceeded.  Please ensure no new nodes have been introduced into the system and try again.  If
						the error persists, please submit you code and a bug report using the trac website.
						*/
					}
				}
				//To end, we don't want to do anything (should all be handled from from end)
			}

			if (rest_object->Connectivity_Matrix[node_int][node_val]!=3)	//Not a Switch
			{
				//This means it is a fuse or link, which for now we handle the same

				//Flag us
				if (from_val)	//From end
				{
					branch_val = temp_branch.to;
				}
				else	//To end
				{
					branch_val = temp_branch.from;
				}

				//Set support flag
				Supported_Nodes[branch_val]=1;

				//Recursion!!
				search_links(branch_val);
			}
			else	//We must be a switch
			{
				if (*temp_branch.status==1)	//We're closed!
				{
					//Flag us
					if (from_val)	//From end
					{
						branch_val = temp_branch.to;
					}
					else	//To end
					{
						branch_val = temp_branch.from;
					}

					//Set support flag
					Supported_Nodes[branch_val]=1;

					//Recursion!!
					search_links(branch_val);
				}
			}

		}//End both not handled (work to be done)
	}//End link table loop
}

void fault_check::support_check(unsigned int swing_node_int,bool rest_pop_tree)
{
	unsigned int index;

	//Reset the node status list
	for (index=0; index<NR_bus_count; index++)
		Supported_Nodes[index] = 0;

	//See if we want the tree structure populated
	if (rest_pop_tree == true)
	{
		for (index=0; index<NR_bus_count; index++)
			NR_busdata[index].Child_Node_idx = 0;	//Zero the child object index
	}

	//Swing node has support
	Supported_Nodes[swing_node_int] = 1;

	//Call the node link-erator (node support check) - call it on the swing, the details are handled inside
	search_links(swing_node_int);
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
	catch (char *msg)
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
	catch (char *msg)
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
