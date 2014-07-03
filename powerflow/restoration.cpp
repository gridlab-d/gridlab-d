/** $Id: restoration.cpp 4738 2014-07-03 00:55:39Z dchassin $
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
		if (oclass==NULL)
			throw "unable to register class restoration";
		else
			oclass->trl = TRL_PROTOTYPE;

		if(gl_publish_variable(oclass,
			PT_int32,"reconfig_attempts",PADDR(reconfig_attempts),PT_DESCRIPTION,"Number of reconfigurations/timestep to try before giving up",
			PT_int32,"reconfig_iteration_limit",PADDR(reconfig_iter_limit),PT_DESCRIPTION,"Number of iterations to let PF go before flagging this as a bad reconfiguration",
			PT_bool,"populate_tree",PADDR(populate_tree),PT_DESCRIPTION,"Flag to populate Parent/Child tree structure",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int restoration::isa(char *classname)
{
	return strcmp(classname,"restoration")==0;
}

int restoration::create(void)
{
	prev_time = 0;

	reconfig_attempts = 0;
	reconfig_iter_limit = 0;
	populate_tree = false;


	return 1;
}

int restoration::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);

	if (solver_method == SM_NR)
	{
		if (restoration_object == NULL)
		{
			restoration_object = obj;	//Set up the global
			
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
		Connectivity_Matrix[indexx] = (int*)gl_malloc(NR_bus_count * sizeof(int));
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

//Function to calculate the magnitude of node voltage in per unit
double restoration::VoltagePerUnit(int node_id, int node_phase) // inputs are node_id and phase
{
		double MagV_PU;
		OBJECT *TempObj;			//Variables for nominal voltage example.  May be changed/removed
		node *TempNode;
		double ex_nom_volts;
			//Get the object information
			TempObj = gl_get_object(NR_busdata[node_id].name);

			//Associate this with a node object
			TempNode = OBJECTDATA(TempObj,node);

			//Extract the nominal voltage value
			ex_nom_volts = TempNode->nominal_voltage;
			MagV_PU = NR_busdata[node_id].V[node_phase].Mag()/ex_nom_volts; 
			return MagV_PU;
}


//Function to check if the per phase node voltage is in the permitted range
bool restoration::VoltageCheck(double MagV_PU)
{
	if ((MagV_PU > 0.927) && (MagV_PU < 1.05))
	{
			return true;
	}
	else 
	{
		return false;
	}
}

//Function to find the feeder_id for each node, starting from the initiating node for each feeder. Using Depth first search 
void restoration :: Feeder_id_Mark(int initial_node, int feeder)
{
	unsigned int indexx, indexbr;
	unsigned int conval;
		visited_flag[initial_node] = true;
	//	printf("%s  feeder %d \n",NR_busdata[initial_node].name, feeder );   
		feeder_id[initial_node] = feeder;
		for(indexx=0;indexx<NR_bus_count;indexx++)
		{
		conval = Connectivity_Matrix[initial_node][indexx];
				if (conval != 0)
					{
						if ((conval != 3) && (visited_flag[indexx]==false))  // If it is not a switch (it could be a line, fuse, transformer or voltage regulator)
							Feeder_id_Mark(indexx,feeder); 
						else if ((conval == 3) && (visited_flag[indexx]==false))  // If it is a switch
						{ 
								for (indexbr=0; indexbr<NR_branch_count; indexbr++)
								{
									if ((NR_branchdata[indexbr].from == initial_node) && (NR_branchdata[indexbr].to == indexx) && (*NR_branchdata[indexbr].status) == 1) // If switch is closed										{
										Feeder_id_Mark(indexx,feeder);
								}
						}
			} /*End of if  (conval != 0) which indicates that a link is exisiting)*/
		} /*End of bus node traversion*/
} /*End of Feeder_id_Mark()*/

int restoration :: Search_sec_switches(int initial_node)
{
	bool encounter_tie_switch; // flag if encounter a tie switch on the searching path
	int  temp_node_id, temp_branch_id;
	unsigned int indexx, indexy, sec_switch_sum;
	encounter_tie_switch = false;
	sec_switch_sum = 0;
	for( indexx = 0; indexx < NR_bus_count; indexx ++)
	{
		if (NR_busdata[initial_node].Parent_Node != -1) 
		{
			temp_node_id = NR_busdata[initial_node].Parent_Node ;
			if (Connectivity_Matrix[initial_node][temp_node_id] == 3) // if there is a sectionaling switch between parent node and inital_node, push it back to the candidate sectionaling switch array
				{		 
					for ( indexy = 0; indexy < NR_busdata[temp_node_id].Link_Table_Size; indexy ++)
					{
					    temp_branch_id = NR_busdata[temp_node_id].Link_Table[indexy];
						if ((NR_branchdata[temp_branch_id].from == initial_node)  || (NR_branchdata[temp_branch_id].to == initial_node))
						{
							candidate_sec_switch[sec_switch_sum] = temp_branch_id;
							break;
						}
					}
					 sec_switch_sum++;
				}
			for ( indexy = 0; indexy < NR_busdata[temp_node_id].Link_Table_Size; indexy ++)
					{
						temp_branch_id = NR_busdata[temp_node_id].Link_Table[indexy]; //Branch data temp_branch;
						{
							 if ((feeder_id[NR_branchdata[temp_branch_id].from]) != (feeder_id[NR_branchdata[temp_branch_id].to])) // if there is a tie-switch 
							 {
								  encounter_tie_switch = true;
							 }
						}
					}
			if (encounter_tie_switch == true)
				{ 
					break;
				}
		 initial_node = temp_node_id;
		}
		else  if (NR_busdata[initial_node].Parent_Node == -1)
			break;
	}
	return sec_switch_sum; 
} // end of function

// function to check if two nodes are connected in the system
bool restoration::connected_path(unsigned int node_int, unsigned int node_end, unsigned int last_node)
{
	unsigned int index;
	bool both_handled, from_val, end_flag;
	int branch_val, node_val;
	BRANCHDATA temp_branch;
	restoration *rest_object;

	end_flag = false;
	visited_flag[last_node] = true;
	//Loop through the connectivity and populate appropriately
	for (index=0; index<NR_busdata[node_int].Link_Table_Size; index++)	//parse through our connected link
	{
		visited_flag[node_int] = true;
		both_handled = false;	//Reset flag

		temp_branch = NR_branchdata[NR_busdata[node_int].Link_Table[index]];	//Get connecting link information

		//See which end we are, and if the other end has been handled
		if (temp_branch.from == node_int)	//We're the from
		{
			from_val = true;	//Flag us as the from end (so we don't have to check it again later)

			if ((visited_flag[temp_branch.to]==true) || (temp_branch.to == sub_transmission_node))
             		both_handled=true;	//Handled
			}
			else	//Must be the to
			{
				from_val = false;	//Flag us as the to end (so we don't have to check it again later)

				if  ((visited_flag[temp_branch.from]==true) || (temp_branch.from == sub_transmission_node))	//Handled
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

				//Recursion!!
				if (branch_val == node_end)
				{
					end_flag = true;
					break;
				}
				printf("path node: %s:\n", NR_busdata[branch_val].name);
				connected_path(branch_val, node_end, last_node);
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
					if (branch_val == node_end)
					{
						end_flag = true;
						break;
					}
					//Recursion!!
					printf("path node: %s:\n", NR_busdata[branch_val].name);
					connected_path(branch_val, node_end,last_node );
				}
			} // end of else
		}//End both not handled (work to be done)
	}//End link table loop
		return end_flag;
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
	double max_volt_error, min_V_system;
	int64 pf_result;
	int fidx, tidx, pidx;
	double cont_rating;
	bool pf_bad_computations, pf_valid_solution, fc_all_supported, good_solution, rating_exceeded, separation_oos, system_restored; // bool separation_oos indicates if the switching operations that separate the out-of-service area are requried.
	complex temp_current[3];
	unsigned int indexx, indexy, indexz, num_unsupported, indexbr, tempbr, temp_num_switch, temp_idx;
	unsigned int tie_switch_number,sec_switch_number;
	int conval;
	int temp_feeder_id, temp_branch_id, initial_search_node;
	VECTARRAY candidate_tie_switch;
	VECTARRAY candidate_switch_operation;
    VECTARRAY temp_switch;
	int array_expected_size;
	NRSOLVERMODE powerflow_type = PF_NORMAL;
  
	if (t0 != prev_time)	//Perform new timestep updates
	{
		reconfig_number = 0;	//Reset number of reconfigurations that have occurred
		reconfig_switch_number = 0; // Reset number of switching operations that have occurred
		reconfig_iterations=0;	//Reset number of pf iterations that have occurred 
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
	
	FILE *FPCONNECT = fopen("connectout.txt","w");
    printf("***********A fault is detected in the system.****************\n");
	/* Mark the feeder_id for each node */
	feeder_id = (int*)gl_malloc(NR_bus_count * sizeof(int)); 
	visited_flag = (bool*)gl_malloc(NR_bus_count * sizeof(bool)); 
	// Initiate to mark the feeder_id for all the node is -1;
	for (indexx=0; indexx<NR_bus_count; indexx++)
	{
		feeder_id[indexx] = -1;
	}

	// Find all the initiating node for each feeder 
	for(indexx=0;indexx<NR_bus_count;indexx++)
	{
		conval = Connectivity_Matrix[0][indexx];
		if (conval == 4)
		{
				indexy = indexx; // Find the sub-transmission node in the system
				sub_transmission_node = indexy;
				break;
		}
	}

		// Store the initial node for each feeder and mark the feeder id for each node 
	feeder_number = 0;
	for (indexx =0; indexx< NR_bus_count; indexx++)
	{
		conval = Connectivity_Matrix[indexy][indexx];
		if (( conval  == 4 ) && ( indexx != 0 ))
		{
			feeder_number += 1;
		}
	} // count the total number of feeders
	feeder_initial_node = (int*)gl_malloc(feeder_number * sizeof(int)); // feeder_inital_node record the initial node connecting to the the subtransmission node for each feeder;
	temp_feeder_id = 0;
	indexbr = 0;
	for (indexx =0; indexx< NR_bus_count; indexx++)
	{
		conval = Connectivity_Matrix[indexy][indexx];
		if (( conval  == 4 ) && ( indexx != 0 ))
		{
			Connectivity_Matrix[indexy][indexx] = 0; 
			Connectivity_Matrix[indexx][indexy] = 0; // temporary disconnect the link between feeder initiating node and sub-transmission node
			feeder_initial_node[indexbr] = indexx;
	//		printf("initial_node at feeder: %d name : %s\n", indexbr, NR_busdata[indexx].name);
			indexbr+=1;
			feeder_id[indexx] = temp_feeder_id;
			// initially mark all the nodes in the system as unvisited;
			for (indexz =0; indexz< NR_bus_count; indexz++)
			{
				visited_flag[indexz] = false; 
			}
			Feeder_id_Mark(indexx, temp_feeder_id); // Mark the feeder_id for each node
			Connectivity_Matrix[indexy][indexx] = conval; 
			Connectivity_Matrix[indexx][indexy] = conval; // recovery the link between the feeder initiating node and sub-transmission node
			temp_feeder_id++;
		} //  end of if	
	} // end of for NR_bus_count traverse

	// Calculate the total number of unsupported node
	num_unsupported = 0;
	for(indexx=0; indexx < NR_bus_count;indexx++)
	{
		if ((FaultyObject->Supported_Nodes[indexx][0] == 0) || (FaultyObject->Supported_Nodes[indexx][1] == 0) || (FaultyObject->Supported_Nodes[indexx][2] == 0))	//Any fault is treated as 3-phase fault
		num_unsupported++; 
	} // end of for

	// Save the node id of unsupported node in the array of "unsupported_node" 
	unsupported_node = (int*)gl_malloc(num_unsupported * sizeof(int));
	indexy = 0;
	for(indexx=0;indexx < NR_bus_count; indexx++)
	{
		if ((FaultyObject->Supported_Nodes[indexx][0] == 0) || (FaultyObject->Supported_Nodes[indexx][1] == 0) || (FaultyObject->Supported_Nodes[indexx][2] == 0))	//Any fault is treated as 3-phase fault
		{
			unsupported_node[indexy] = indexx;
			NR_busdata[indexx].Child_Node_idx = 0; // For the unsupported node, the total number of child node is zero;
			NR_busdata[indexx].Parent_Node = -1; // For the unsupported node, the parent_node is not existing. It be set to -1;
			printf("unsupported node : %s\n", NR_busdata[indexx].name);
			indexz = indexx;
			indexy++;
		} // end of if
	} // end of for
	fprintf(FPCONNECT, "The number of unsupported node in the system is ");
	fprintf(FPCONNECT, "%d .\n", indexy);

	printf("*****************************************************************\n");
	/* Searching for all the tie switches and sectionalizing switch in the system
    Searching for the tie switches connecting to the out-of-service area */

	// Firstly, Searching for all the tie switches and sectionalizing switch in the system
	tie_switch_number = 0;
	sec_switch_number = 0;
  
   for ( indexbr = 0; indexbr < NR_branch_count; indexbr ++) // count the total number of tie-switches and sec-switches in the system
   {
	   indexx = ( NR_branchdata[indexbr]. from);
       indexy = ( NR_branchdata[indexbr]. to);
	   conval = Connectivity_Matrix[indexx][indexy];
	   if ( conval == CONN_SWITCH)
	   {
		   if  ((*NR_branchdata[indexbr].status) == 1)
			   sec_switch_number++;
		   else 
			   tie_switch_number++;
	   } 
   } // end of NR_branch travesion

	tie_switch = (int*)gl_malloc(tie_switch_number * sizeof(int));
    sec_switch = (int*)gl_malloc(sec_switch_number * sizeof(int));
    tie_switch_number = 0; 
    sec_switch_number = 0;

	// Searching for all the tie-switches and sectionaling switches in the system, and save them in the array of sec_switch and tie_switch respectively
   for ( indexbr = 0; indexbr < NR_branch_count; indexbr ++) // count the total number of tie-switches and sec-switches in the system
   {
	   indexx = ( NR_branchdata[indexbr]. from);
       indexy = ( NR_branchdata[indexbr]. to);
	   conval = Connectivity_Matrix[indexx][indexy];
	   if ( conval == CONN_SWITCH)
	   {
		   if  ((*NR_branchdata[indexbr].status) == 1)
		   {
				sec_switch[sec_switch_number] = indexbr;
   			    sec_switch_number++;
		   }
		   else
		   {
    			tie_switch[tie_switch_number] = indexbr;	
				tie_switch_number++;
		   }
	   } 
   } // end of NR_branch travesion

	printf(" The total number of tie-switch in the system is %d.\n", tie_switch_number); //  to be removed
	printf(" The total number of sectionalizing switch in the system is %d.\n", sec_switch_number); // to be removed
		
	// Secondly, searching for the tie-switches connecting to the out-of-service area		
		
	unsigned int temp_num;

	candidate_tie_switch.Data = (int*)gl_malloc(tie_switch_number*sizeof(int)); 
	if (candidate_tie_switch.Data == NULL)
		GL_THROW("Candidate tie switch memory allocation failed");

	candidate_tie_switch.DataLength = tie_switch_number;

	//Initialize it
	for (indexbr = 0; indexbr < tie_switch_number; indexbr++)
		candidate_tie_switch.Data[indexbr] = -1;

	temp_idx = 0;	//Reset pointer
	candidate_tie_switch.IdxVar = 0;

	for (indexbr = 0; indexbr < tie_switch_number; indexbr ++)
	{
		temp_num = tie_switch[indexbr];
		for (indexx = 0; indexx < num_unsupported; indexx++)
		{
			if ( (NR_branchdata[temp_num].from == unsupported_node[indexx])  ||  (NR_branchdata[temp_num].to == unsupported_node[indexx] ))
			{
				candidate_tie_switch.Data[temp_idx] = temp_num;// save the candidate switch in the list
				temp_idx++;
				break;
			}
		}
	}

	//Store how many elements we actually needed (original statements below implies it may not always use full values)
	candidate_tie_switch.IdxVar = temp_idx;

	// if there is not a single tie-switch connecting to the out-of-service area, no restoration strategy can be found. Printing the notice.
	
	if (candidate_tie_switch.Data[0] == -1)
	{
		printf(" There is not a single tie-switch connecting to the out-of-service area. \n");
		printf(" The system can not be restored. \n");
		reconfig_attempts = 0 ;
	}

	//Allocate the candidate_switch_operation array - initial guess on size, this will likely need to be refined when there is more time to examine the algorithm (vector removal code)
	array_expected_size = tie_switch_number*(tie_switch_number + 6*sec_switch_number + candidate_tie_switch.IdxVar);
	candidate_switch_operation.Data = (int*)gl_malloc(array_expected_size*sizeof(int));
	if (candidate_switch_operation.Data == NULL)
		GL_THROW("candidate switch operation data malloc failed");

	candidate_switch_operation.DataLength = array_expected_size;
	candidate_switch_operation.IdxVar = 0;
  
	temp_switch.DataLength = (2*tie_switch_number+1)*candidate_tie_switch.IdxVar+1;
	temp_switch.Data = (int*)gl_malloc(temp_switch.DataLength*sizeof(int));
	if (temp_switch.Data == NULL)
		GL_THROW("temp_switch malloc failed.");

	temp_switch.IdxVar = 0;

	/* Candidate solution searching and checking phase*/
   while (reconfig_switch_number < tie_switch_number)
	{
		reconfig_switch_number++;
		// When the number of  tie switching operation is just one , which is closing the tie switch
        if ( reconfig_switch_number == 1) 
		{
			tempbr = candidate_tie_switch.IdxVar;
			for ( indexx = 0; indexx < tempbr; indexx++)
			{
				candidate_switch_operation.Data[candidate_switch_operation.IdxVar] = candidate_tie_switch.Data[indexx]; //  candidate_switch_operation store the switching operations when reconfig_switch_number is one.
				candidate_switch_operation.IdxVar++;
			}
		}

		// When the number of  tie switching operation is two
       /* firstly, searching for the tie switch operations*/
		if ( reconfig_switch_number == 2)
		{
			temp_num_switch = 0; // temp_num_switch records the number of tie-switching operation pairs
			for (indexx = 0; indexx < candidate_tie_switch.IdxVar; indexx++)
			{
				  tempbr = candidate_tie_switch.Data[indexx]; // for each switch connecting to the out-of-service area
				  for ( indexy = 0; indexy < tie_switch_number; indexy ++ )
				  {
					  indexz = tie_switch[indexy];
					  if ( feeder_id[NR_branchdata[tempbr].from] == -1)   // find one end of the tie-switch that in the out-of-service area
					  {
                          temp_feeder_id = feeder_id[(NR_branchdata[tempbr].to)];
					  }
					  else if ( feeder_id[NR_branchdata[tempbr].to] == -1) // find one end of the tie-switch that in the out-of-service area
					  {
						  temp_feeder_id = feeder_id[(NR_branchdata[tempbr].from)];
					  }
					  if  (((feeder_id[NR_branchdata[indexz].from]) ==  temp_feeder_id) || ((feeder_id[NR_branchdata[indexz].to]) ==  temp_feeder_id))
					  {						   
							if ( indexz != tempbr)		
							{
							temp_switch.Data[temp_switch.IdxVar] = tempbr;
							temp_switch.IdxVar++;
							temp_switch.Data[temp_switch.IdxVar] = indexz;
							temp_switch.IdxVar++;
							temp_num_switch+=2;
							}
					  }
				  } // end of for indexy
			}// end of for candidate_tie_switch traverse

		temp_switch.Data[temp_switch.IdxVar] = -1; // -1 is flag to indicate the following operationgs.
		temp_switch.IdxVar++;
		temp_num_switch++;

 // Searching for the switching operation pairs that are both connecting to the out-of-service area directly
		for (indexx =0; indexx < candidate_tie_switch.IdxVar; indexx++)
		{
			for (indexy = indexx+1; indexy < candidate_tie_switch.IdxVar; indexy++)
			{
				temp_switch.Data[temp_switch.IdxVar] = candidate_tie_switch.Data[indexx];
				temp_switch.IdxVar++;
				temp_num_switch++;
				temp_switch.Data[temp_switch.IdxVar] = candidate_tie_switch.Data[indexy];
				temp_switch.IdxVar++;
				temp_num_switch++;
			}
		}

		candidate_sec_switch = (int*)gl_malloc(sec_switch_number * sizeof(int)); 
		
		separation_oos = false;
		for ( indexx =0; indexx < temp_num_switch; indexx ++)
		{ 
			temp_branch_id = temp_switch.Data[indexx];
			if ((temp_branch_id == -1) && ( temp_branch_id != temp_switch.Data[(temp_switch.IdxVar-1)]))
			{
				separation_oos = true;
				indexx ++;
			}
			else if ((temp_branch_id == -1) && ( temp_branch_id == temp_switch.Data[(temp_switch.IdxVar-1)]))
			{
				break;
			}

			if (separation_oos == false)
			{
				if ( feeder_id[NR_branchdata[temp_branch_id].from] == -1)
				{
					temp_feeder_id = feeder_id[NR_branchdata[temp_branch_id].to] ;
				}
				else if  ( feeder_id[NR_branchdata[temp_branch_id].to] == -1)
				{
					temp_feeder_id = feeder_id[NR_branchdata[temp_branch_id].from];
				}
				indexx++;
				temp_branch_id = temp_switch.Data[indexx];
				if ( feeder_id[NR_branchdata[temp_branch_id].from] == temp_feeder_id )
				{
					initial_search_node =  (NR_branchdata[temp_branch_id].from);
				}
				else if ( feeder_id[NR_branchdata[temp_branch_id].to] == temp_feeder_id )
				{
					initial_search_node = (NR_branchdata[temp_branch_id].to);
				}
				indexy = Search_sec_switches(initial_search_node); // indexy return the number of sectionaling switches are found for the tie-switch pair
				if (indexy > 0)
				{
					for ( indexz = indexy; indexz > 0; indexz--)
					{
						candidate_switch_operation.Data[candidate_switch_operation.IdxVar] = temp_switch.Data[(indexx-1)];
						candidate_switch_operation.IdxVar++;
						candidate_switch_operation.Data[candidate_switch_operation.IdxVar]= temp_switch.Data[indexx];
						candidate_switch_operation.IdxVar++;
						candidate_switch_operation.Data[candidate_switch_operation.IdxVar] = candidate_sec_switch[indexz-1]; // candidate_switch_operation store the switching operations
						candidate_switch_operation.IdxVar++;
					}
				}
			}
			if  (separation_oos ==true)
			{
					for ( indexbr = 0; indexbr < sec_switch_number; indexbr++)
					{
						if ((feeder_id[NR_branchdata[sec_switch[indexbr]].from] == -1) && (feeder_id[NR_branchdata[sec_switch[indexbr]].to] == -1))
						{
						candidate_switch_operation.Data[candidate_switch_operation.IdxVar] = temp_switch.Data[indexx];
						candidate_switch_operation.IdxVar++;
						candidate_switch_operation.Data[candidate_switch_operation.IdxVar] = temp_switch.Data[(indexx+1)];
						candidate_switch_operation.IdxVar++;
						candidate_switch_operation.Data[candidate_switch_operation.IdxVar] = sec_switch[indexbr]; // candidate_switch_operation store the switching operations
						candidate_switch_operation.IdxVar++;
						}
					}
					indexx++;
			} // end of else (separation_oss == true)
		}
        
		tempbr	= temp_switch.IdxVar;

		if (tempbr > candidate_tie_switch.DataLength)	//It's bigger - realloc - hopefully we won't need to do this
		{
			gl_free(candidate_tie_switch.Data);
			candidate_tie_switch.Data=(int*)gl_malloc(tempbr*sizeof(int));
			if (candidate_tie_switch.Data == NULL)
				GL_THROW("Malloc of candidate_tie_switch failed");

			candidate_tie_switch.DataLength = tempbr;
			candidate_tie_switch.IdxVar = 0;
		}
		else	//Size is ok
			candidate_tie_switch.IdxVar = 0;

		for (indexx = 0; indexx < tempbr; indexx++)
		{
			temp_switch.IdxVar--;	//Put to "current" location
			candidate_tie_switch.Data[candidate_tie_switch.IdxVar] = temp_switch.Data[temp_switch.IdxVar];// copy the tie-switching operations into the candidate_tie_switch 
			candidate_tie_switch.IdxVar++;
		}
	} // end of  reconfig_switch_number == 2

  /* Change the switch status in the candidate list, if the candidate switch is closed, open this switch. Or if the candidate switch
	is open, then close this switch*/
	while (candidate_switch_operation.IdxVar != 0) // If there is a candidate switch in the solution list
		{
			for ( indexx = 0; indexx < (2*reconfig_switch_number -1); indexx++)
			{
			candidate_switch_operation.IdxVar--;	//Always is at "next" spot, so we need to go down
			temp_branch_id = candidate_switch_operation.Data[candidate_switch_operation.IdxVar];// Get the last candidate switch in the list
			temp_switch.Data[temp_switch.IdxVar] = temp_branch_id;
			temp_switch.IdxVar++;


							//Get the switch's object linking
							tempobj = gl_get_object(NR_branchdata[temp_branch_id].name);

							//Now pull it into switch properties
							SwitchDevice = OBJECTDATA(tempobj,switch_object);

					if  ((feeder_id[NR_branchdata[temp_branch_id].from]) != (feeder_id[NR_branchdata[temp_branch_id].to])) //  if it is a tie switch
								{
								//Close it
								SwitchDevice->set_switch(true);	//True = closed, false = open
								}
					else  //
								{
								//Open it
								SwitchDevice->set_switch(false);	//True = closed, false = open
								}  

			}

			//Check to see if everything is supported again - not sure if this will be necessary or not
			FaultyObject->support_check(0,populate_tree);		//Start a new support check
			//See if anything is unsupported
			fc_all_supported = true;
			for (indexx=0; indexx<NR_bus_count; indexx++)
			{
				if ((FaultyObject->Supported_Nodes[indexx][0] == 0) || (FaultyObject->Supported_Nodes[indexx][0] == 0) || (FaultyObject->Supported_Nodes[indexx][0] == 0))	//Look for any phase fault - ignores single phase
				{
					fc_all_supported = false;	//Flag as a defect
//					printf("unsupported node:%s.\n",NR_busdata[indexx].name);
					gl_verbose("Reconfiguration attempt failed - unsupported nodes were still located");
					/* TROUBLESHOOT
					After performing the reconfiguration logic, some nodes are still unsupported.  This is considered a failed
					reconfiguration, so another will be attempted.
					*/
//					break;						//Only takes one failure to need to get out of here
				}
			}

			//Set the pf solution flag
			pf_valid_solution = false;

		//Re-solve powerflow at this point - assuming the system appears valid
			while ((reconfig_iterations < reconfig_iter_limit) && (fc_all_supported==true))	//Only let powerflow iterate so many times
			{
				//Reset tracking variables
				good_solution = false;
				min_V = (double*)gl_malloc(feeder_number * sizeof(double));
				node_id_minV = (int*)gl_malloc(feeder_number *sizeof(int));
				for (indexx = 0; indexx < feeder_number; indexx ++)  // give the initiating value of the min_V at each feeder
				{
					min_V[indexx] = 1.000;
					node_id_minV[indexx] = 0;
				}

				pf_bad_computations = false;	//Reset singularity checker



				//Perform NR solver
				pf_result = solver_nr(NR_bus_count, NR_busdata, NR_branch_count, NR_branchdata, &NR_powerflow, powerflow_type, &pf_bad_computations);
				//De-flag any admittance changes (so other iterations don't take longer
				LOCK_OBJECT(NR_swing_bus);	//Lock SWING since we'll be modifying this
				NR_admit_change = false;
				UNLOCK_OBJECT(NR_swing_bus);	//Finished locking

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
			bool VoltageCheckResult;
			unsigned int indexx, indexy, indexz; temp_feeder_id;
			double tempV;
			bool temp_flag;
			for (indexx=0; indexx <NR_bus_count; indexx++)
			{
				for (indexy=0; indexy<3; indexy ++)
				{
				    temp_feeder_id = feeder_id[indexx];
					tempV = VoltagePerUnit(indexx, indexy);
					for ( indexz = 0; indexz < feeder_number; indexz ++)
					{
						if (( tempV != 0) && ( temp_feeder_id == indexz))
						{
							if (tempV < min_V[indexz])
							{
								min_V[indexz] = tempV;           // record the value of min_V at each feeder 
								node_id_minV[indexz] = indexx; // record the min_V node id at each feeder
							}
							if (tempV > max_V)
							{
								max_V = tempV;
								node_id_maxV = indexx;  // record the value of max_V and max_V node id in the system
							}
						}
					}// end of tempV! = 0 and feeder_id is located
				} // end of loop of indexy
        } // end of loop of indexx
           
			    
			min_V_system = 1.000;
			for ( indexz = 0; indexz < feeder_number; indexz ++)	 
			{
				if ( min_V[indexz] < min_V_system)
				{
					min_V_system = min_V[indexz];
					indexx = indexz; 
				}
			}

			temp_flag = VoltageCheck(min_V_system);
	        if ( temp_flag == false)
			{
				fprintf(FPCONNECT, "\n\n");
				fprintf(FPCONNECT, "The following restoration plan is failed :");
				fprintf(FPCONNECT, "The node_id of minimum voltage in the system is ");
				fprintf(FPCONNECT, "%s", NR_busdata[node_id_minV[indexx]].name);
				fprintf(FPCONNECT, ".\n");
				fprintf(FPCONNECT, "The minimum voltage in the system is ");
				fprintf(FPCONNECT, "%f", min_V_system);
				fprintf(FPCONNECT, "of the nominal voltage. ");
				tempbr = temp_switch.IdxVar;
				for ( indexz = 0; indexz <tempbr; indexz++)
				{
			 		temp_branch_id = temp_switch.Data[indexz];
					indexx = ((NR_branchdata[temp_branch_id]).from); 
					indexy = ((NR_branchdata[temp_branch_id]).to);  		
					 if ((*NR_branchdata[temp_branch_id].status) == 1)   // The switch is closed
						 {
	//						fprintf(FPCONNECT, "close the switch between  %s and  %s \n", NR_busdata[indexx].name, NR_busdata[indexy].name);
						 }
					 else if ((*NR_branchdata[temp_branch_id].status) == 0)   // The switch is open	
						 {
	//						fprintf(FPCONNECT, "open the switch between  %s and  %s \n", NR_busdata[indexx].name, NR_busdata[indexy].name);
						 }
				}
			}

//	for (indexx = 0; indexx < NR_bus_count; indexx++)
//	  {
//			tempV = VoltagePerUnit(indexx, 0);
//			fprintf(FPCONNECT,NR_busdata[indexx].name);
//			fprintf(FPCONNECT," - ");
//			fprintf(FPCONNECT,"%f", tempV);			
//			fprintf(FPCONNECT,"\n");
//	  }

	   VoltageCheckResult = temp_flag;	
	   good_solution =  VoltageCheckResult; ////VoltageCheckResult///////////////////////////////////////////////////////////////////////////////////////////////


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
			}//end valid pf solution checks

			//Check to see if we're done, or need to reconfigure again, and print out of the solutions( switching operations)
			if ((good_solution == true) && (pf_valid_solution == true))
			{
				tempbr = temp_switch.IdxVar;
				system_restored = true;	
				printf("\n The system is successfully restored by the following switching operations:\n");
				for ( indexz = 0; indexz <tempbr; indexz++)
				{
					temp_branch_id = temp_switch.Data[indexz];
					indexx = ((NR_branchdata[temp_branch_id]).from); 
					indexy = ((NR_branchdata[temp_branch_id]).to);  		
					 if ((*NR_branchdata[temp_branch_id].status) == 1)   // The switch is closed
						 {
							printf("---close the switch between bus %s and bus %s \n", NR_busdata[indexx].name, NR_busdata[indexy].name);
						 }
					 else if ((*NR_branchdata[temp_branch_id].status) == 0)   // The switch is open	
						 {
							printf("---open the switch between bus %s and bus %s \n", NR_busdata[indexx].name, NR_busdata[indexy].name);
						 }
				}
				for ( indexz = 0; indexz < feeder_number; indexz ++)	 
				{
					min_V_system= min_V[indexz]*100;
					printf("The minimum voltage at feeder %d is %5.2f  percent of norminal Voltage at node",indexz, min_V_system);
					printf("%s. \n",NR_busdata[node_id_minV[indexz]].name);						
				}
				break;
			}

			//  change the system to the original status, since the candidate solution is not a good solution
			tempbr = temp_switch.IdxVar;
			for ( indexz = 0; indexz < tempbr; indexz++)
			{
					temp_branch_id = temp_switch.Data[indexz];

							//Get the switch's object linking
							tempobj = gl_get_object(NR_branchdata[temp_branch_id].name);

							//Now pull it into switch properties
							SwitchDevice = OBJECTDATA(tempobj,switch_object);


					if  ((feeder_id[NR_branchdata[temp_branch_id].from]) != (feeder_id[NR_branchdata[temp_branch_id].to])) //  if it is a tie switch
								{
								//Open it
								SwitchDevice->set_switch(false);	//True = closed, false = open
								}
					else  //
								{
								//Close it
								SwitchDevice->set_switch(true);	//True = closed, false = open
								}  
			}
			temp_switch.IdxVar=0;
			reconfig_number++;	//Increment the reconfiguration counter
		}//end of while the candidate_switch_operations is not empty 
	if (system_restored == true)
		break;
	}//end reconfiguration while  reconfig_switch_number < tie_switch_number
     
	if (reconfig_number == reconfig_attempts ) 
	{
		GL_THROW("Feeder reconfiguration failed.");
		/*  TROUBLESHOOT
		Attempts at reconfiguring the feeder have failed.  An unsolvable condition was reached
		or the reconfig_attempts limit was set too low.  Please modify the test system or increase
		the value of reconfig_attempts.
		*/
	}

	//Remove dynamic allocation stuff - this will be handled better once the code is all finalized
	gl_free(candidate_tie_switch.Data);
	gl_free(candidate_switch_operation.Data);
	gl_free(temp_switch.Data);
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
		else
			return 0;
	}
	CREATE_CATCHALL(restoration);
}

EXPORT int init_restoration(OBJECT *obj, OBJECT *parent)
{
	try {
		return OBJECTDATA(obj,restoration)->init(parent);
	}
	INIT_CATCHALL(restoration);
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
