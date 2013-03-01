/** $Id: node.cpp 1215 2009-01-22 00:54:37Z d3x593 $
	Copyright (C) 2008 Battelle Memorial Institute
	@file node.cpp
	@addtogroup powerflow_node Node
	@ingroup powerflow_object
	
	The node is one of the major components of the method used for solving a 
	powerflow network.  In essense the distribution network can be seen as a 
	series of nodes and links.  Nodes primary responsibility is to act as an
	aggregation point for the links that are attached to it, and to hold the
	current and voltage values that will be used in the matrix calculations
	done in the link.
	
	Three types of nodes are defined in this file.  Nodes are simply a basic 
	object that exports the voltages for each phase.  Triplex nodes export
	voltages for 3 lines; line1_voltage, line2_voltage, lineN_voltage.

	@par Voltage control

	When the global variable require_voltage_control is set to \p TRUE,
	the bus type is used to determine how voltage control is implemented.
	Voltage control is only performed when the bus has no link that
	considers it a to node.  When the flag \#NF_HASSOURCE is cleared, then
	the following is in effect:

	- \#SWING buses are considered infinite sources and the voltage will
	  remain fixed regardless of conditions.
	- \#PV buses are considered constrained sources and the voltage will
	  fall to zero if there is insufficient voltage support at the bus.
	  A generator object is required to provide the voltage support explicitly.
	  The voltage will be set to zero if the generator does not provide
	  sufficient support.
	- \#PQ buses will always go to zero voltage.

	@par Fault support

	The following conditions are used to describe a fault impedance \e X (e.g., 1e-6), 
	between phase \e x and neutral or group, or between phases \e x and \e y, and leaving
	phase \e z unaffected at iteration \e t:
	- \e phase-to-phase contact at the node
		- \e Forward-sweep: \f$ V_x(t) = V_y(t) = \frac{V_x(t-1) + V_y(t-1)}{2} \f$
		- \e Back-sweep: \f$ I_x(t+1) = -I_y(t+1) = \frac{V_x(t)-V_y(t)}{X} \f$;
    - \e phase-to-ground/neutral contact at the node
		- \e Forward-sweep: \f$ V_x(t) = \frac{1}{2} V_x(t-1)  \f$
		- \e Back-sweep: \f$ I_x(t+1) = \frac{V_x(t)}{X} \f$;
	
	@{
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "solver_nr.h"
#include "restoration.h"
#include "node.h"
#include "transformer.h"

//Library imports items - for external LU solver - stolen from somewhere else in GridLAB-D (tape, I believe)
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0400
#include <windows.h>
#ifndef DLEXT
#define DLEXT ".dll"
#endif
#define DLLOAD(P) LoadLibrary(P)
#define DLSYM(H,S) GetProcAddress((HINSTANCE)H,S)
#define snprintf _snprintf
#else /* ANSI */
#include "dlfcn.h"
#ifndef DLEXT
#define DLEXT ".so"
#else
#endif
#define DLLOAD(P) dlopen(P,RTLD_LAZY)
#define DLSYM(H,S) dlsym(H,S)
#endif

CLASS *node::oclass = NULL;
CLASS *node::pclass = NULL;

unsigned int node::n = 0; 

node::node(MODULE *mod) : powerflow_object(mod)
{
	if(oclass == NULL)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod,"node",sizeof(node),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class node";
		else
			oclass->trl = TRL_PROVEN;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "powerflow_object",
			PT_enumeration, "bustype", PADDR(bustype),PT_DESCRIPTION,"defines whether the node is a PQ, PV, or SWING node",
				PT_KEYWORD, "PQ", PQ,
				PT_KEYWORD, "PV", PV,
				PT_KEYWORD, "SWING", SWING,
			PT_set, "busflags", PADDR(busflags),PT_DESCRIPTION,"flag indicates node has a source for voltage, i.e. connects to the swing node",
				PT_KEYWORD, "HASSOURCE", (set)NF_HASSOURCE,
			PT_object, "reference_bus", PADDR(reference_bus),PT_DESCRIPTION,"reference bus from which frequency is defined",
			PT_double,"maximum_voltage_error[V]",PADDR(maximum_voltage_error),PT_DESCRIPTION,"convergence voltage limit or convergence criteria",

			PT_complex, "voltage_A[V]", PADDR(voltageA),PT_DESCRIPTION,"bus voltage, Phase A to ground",
			PT_complex, "voltage_B[V]", PADDR(voltageB),PT_DESCRIPTION,"bus voltage, Phase B to ground",
			PT_complex, "voltage_C[V]", PADDR(voltageC),PT_DESCRIPTION,"bus voltage, Phase C to ground",
			PT_complex, "voltage_AB[V]", PADDR(voltageAB),PT_DESCRIPTION,"line voltages, Phase AB",
			PT_complex, "voltage_BC[V]", PADDR(voltageBC),PT_DESCRIPTION,"line voltages, Phase BC",
			PT_complex, "voltage_CA[V]", PADDR(voltageCA),PT_DESCRIPTION,"line voltages, Phase CA",
			PT_complex, "current_A[A]", PADDR(currentA),PT_DESCRIPTION,"bus current injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_B[A]", PADDR(currentB),PT_DESCRIPTION,"bus current injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_C[A]", PADDR(currentC),PT_DESCRIPTION,"bus current injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_A[VA]", PADDR(powerA),PT_DESCRIPTION,"bus power injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_B[VA]", PADDR(powerB),PT_DESCRIPTION,"bus power injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_C[VA]", PADDR(powerC),PT_DESCRIPTION,"bus power injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "shunt_A[S]", PADDR(shuntA),PT_DESCRIPTION,"bus shunt admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_B[S]", PADDR(shuntB),PT_DESCRIPTION,"bus shunt admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_C[S]", PADDR(shuntC),PT_DESCRIPTION,"bus shunt admittance, this an accumulator only, not a output or input variable",
			PT_bool, "NR_mode", PADDR(NR_mode),PT_DESCRIPTION,"boolean for solution methodology, not a input or output object",
			PT_double, "mean_repair_time[s]",PADDR(mean_repair_time), PT_DESCRIPTION, "Time after a fault clears for the object to be back in service",

			PT_enumeration, "service_status", PADDR(service_status),PT_DESCRIPTION,"In and out of service flag",
				PT_KEYWORD, "IN_SERVICE", ND_IN_SERVICE,
				PT_KEYWORD, "OUT_OF_SERVICE", ND_OUT_OF_SERVICE,
			PT_double, "service_status_double", PADDR(service_status_dbl),PT_DESCRIPTION,"In and out of service flag - type double - will indiscriminately override service_status - useful for schedules",
			PT_double, "previous_uptime[min]", PADDR(previous_uptime),PT_DESCRIPTION,"Previous time between disconnects of node in minutes",
			PT_double, "current_uptime[min]", PADDR(current_uptime),PT_DESCRIPTION,"Current time since last disconnect of node in minutes",

			PT_object, "topological_parent", PADDR(TopologicalParent),PT_DESCRIPTION,"topological parent as per GLM configuration",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

int node::isa(char *classname)
{
	return strcmp(classname,"node")==0 || powerflow_object::isa(classname);
}

int node::create(void)
{
	int result = powerflow_object::create();

#ifdef SUPPORT_OUTAGES
	condition=OC_NORMAL;
#endif

	n++;

	bustype = PQ;
	busflags = NF_HASSOURCE;
	busphasesIn = 0;
	busphasesOut = 0;
	reference_bus = NULL;
	nominal_voltage = 0.0;
	maximum_voltage_error = 0.0;
	frequency = nominal_frequency;
	fault_Z = 1e-6;
	prev_NTime = 0;
	SubNode = NONE;
	SubNodeParent = NULL;
	TopologicalParent = NULL;
	NR_subnode_reference = NULL;
	Extra_Data=NULL;
	NR_link_table = NULL;
	NR_connected_links[0] = NR_connected_links[1] = 0;
	NR_number_child_nodes[0] = NR_number_child_nodes[1] = 0;
	NR_child_nodes = NULL;

	NR_node_reference = -1;	//Newton-Raphson bus index, set to -1 initially
	house_present = false;	//House attachment flag
	nom_res_curr[0] = nom_res_curr[1] = nom_res_curr[2] = 0.0;	//Nominal house current variables

	prev_phases = 0x00;

	mean_repair_time = 0.0;

	// Only used in capacitors, at this time, but put into node for future functionality (maybe with reliability?)
	service_status = ND_IN_SERVICE;
	service_status_dbl = -1.0;	//Initial flag is to ignore it.  As soon as this changes, it overrides service_status
	last_disconnect = 0;		//Will get set in init
	previous_uptime = -1.0;		///< Flags as not initialized
	current_uptime = -1.0;		///< Flags as not initialized

	memset(voltage,0,sizeof(voltage));
	memset(voltaged,0,sizeof(voltaged));
	memset(current,0,sizeof(current));
	memset(power,0,sizeof(power));
	memset(shunt,0,sizeof(shunt));

	prev_voltage_value = NULL;	//NULL the pointer, just for the sake of doing so
	prev_power_value = NULL;	//NULL the pointer, again just for the sake of doing so

	current_accumulated = false;

	return result;
}

int node::init(OBJECT *parent)
{
	if (solver_method==SM_NR)
	{
		OBJECT *obj = OBJECTHDR(this);
		char ext_lib_file_name[1025];
		char extpath[1024];
		CALLBACKS **cbackval = NULL;
		bool ExtLinkFailure;

		// Store the topological parent before anyone overwrites it
		TopologicalParent = obj->parent;

		//Check for a swing bus if we haven't found one already
		if (NR_swing_bus == NULL)
		{
			OBJECT *temp_obj = NULL;
			node *list_node;
			unsigned int swing_count = 0;
			FINDLIST *bus_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"node",FT_END);

			//Parse the findlist
			while(temp_obj=gl_find_next(bus_list,temp_obj))
			{
				list_node = OBJECTDATA(temp_obj,node);

				if (list_node->bustype==SWING)
				{
					NR_swing_bus=temp_obj;
					swing_count++;
				}
			}

			//Do the same for meters
			gl_free(bus_list);
			bus_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"meter",FT_END);

			//Reset temp_obj
			temp_obj = NULL;

			//Parse the findlist
			while(temp_obj=gl_find_next(bus_list,temp_obj))
			{
				list_node = OBJECTDATA(temp_obj,node);

				if (list_node->bustype==SWING)
				{
					NR_swing_bus=temp_obj;
					swing_count++;
				}
			}

			//Do the same for loads
			gl_free(bus_list);
			bus_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"load",FT_END);

			//Reset temp_obj
			temp_obj = NULL;

			//Parse the findlist
			while(temp_obj=gl_find_next(bus_list,temp_obj))
			{
				list_node = OBJECTDATA(temp_obj,node);

				if (list_node->bustype==SWING)
				{
					NR_swing_bus=temp_obj;
					swing_count++;
				}
			}

			//Do the same for triplex nodes
			gl_free(bus_list);
			bus_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"triplex_node",FT_END);

			//Reset temp_obj
			temp_obj = NULL;

			//Parse the findlist
			while(temp_obj=gl_find_next(bus_list,temp_obj))
			{
				list_node = OBJECTDATA(temp_obj,node);

				if (list_node->bustype==SWING)
				{
					NR_swing_bus=temp_obj;
					swing_count++;
				}
			}

			//And one more time for triplex meters
			gl_free(bus_list);
			bus_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"triplex_meter",FT_END);

			//Reset temp_obj
			temp_obj = NULL;

			//Parse the findlist
			while(temp_obj=gl_find_next(bus_list,temp_obj))
			{
				list_node = OBJECTDATA(temp_obj,node);

				if (list_node->bustype==SWING)
				{
					NR_swing_bus=temp_obj;
					swing_count++;
				}
			}

			//Free the buslist
			gl_free(bus_list);

			if (swing_count==0)
			{
				GL_THROW("NR: no swing bus found");
				/*	TROUBLESHOOT
				No swing bus was located in the test system.  Newton-Raphson requires at least one node
				be designated "bustype SWING".
				*/
			}

			if (swing_count>1)
			{
				GL_THROW("NR: more than one swing bus found!");
				/*	TROUBLESHOOT
				More than one swing bus was found.  Newton-Raphson currently only supports one swing bus
				at this time.  Please split the system into individual files or consider merging the system to
				only one swing bus.
				*/
			}
		}//end swing bus search

		//Check for parents to see if they are a parent/childed load
		if (obj->parent!=NULL) 	//Has a parent, let's see if it is a node and link it up 
		{						//(this will break anything intentionally done this way - e.g. switch between two nodes)
			//See if it is a node/load/meter
			if (!(gl_object_isa(obj->parent,"load","powerflow") | gl_object_isa(obj->parent,"node","powerflow") | gl_object_isa(obj->parent,"meter","powerflow")))
				GL_THROW("NR: Parent is not a node, load or meter!");
				/*  TROUBLESHOOT
				A Newton-Raphson parent-child connection was attempted on a non-node.  The parent object must be a node, load, or meter object in the 
				powerflow module for this connection to be successful.
				*/

			node *parNode = OBJECTDATA(obj->parent,node);

			//See if it is a swing, just to toss a warning
			if (parNode->bustype==SWING)
			{
				gl_warning("Node:%s is parented to a swing node and will get folded into it.",obj->name);
				/*  TROUBLESHOOT
				When a node has the parent field populated, it is assumed this is for a "zero-length" line connection.
				If a swing node is specified, it will assume the node is linked to the swing node via a zero-length
				connection.  If this is undesired, remove the parenting to the swing node.
				*/
			}

			//Phase variable
			set p_phase_to_check, c_phase_to_check;

			//N-less version
			p_phase_to_check = (parNode->phases & (~(PHASE_N)));
			c_phase_to_check = (phases & (~(PHASE_N)));

			//Make sure our phases align, otherwise become angry
			if ((parNode->phases!=phases) && (p_phase_to_check != c_phase_to_check))
			{
				//Create D-less and N-less versions of both for later comparisons
				p_phase_to_check = (parNode->phases & (~(PHASE_D | PHASE_N)));
				c_phase_to_check = (phases & (~(PHASE_D | PHASE_N)));

				//May not necessarily be a failure, let's investiage
				if ((p_phase_to_check & c_phase_to_check) != c_phase_to_check)	//Our parent is lacking, fail
				{
					GL_THROW("NR: Parent and child node phases for nodes %s and %s do not match!",obj->parent->name,obj->name);
					//Defined above
				}
				else					//We should be successful, but let's flag ourselves appropriately
				{	//Essentially a replication of the no-phase section with more check
					if ((parNode->SubNode==CHILD) | (parNode->SubNode==DIFF_CHILD) | ((obj->parent->parent!=NR_swing_bus) && (obj->parent->parent!=NULL)))	//Our parent is another child
					{
						GL_THROW("NR: Grandchildren are not supported at this time!");
						/*  TROUBLESHOOT
						Parent-child connections in Newton-Raphson may not go more than one level deep.  Grandchildren
						(a node parented to a node parented to a node) are unsupported at this time.  Please rearrange your
						parent-child connections appropriately, figure out a different way of performing the required connection,
						or, if your system is radial, consider using forward-back sweep.
						*/
					}
					else	//Our parent is unchilded (or has the swing bus as a parent)
					{
						//Check and see if the parent or child is a delta - if so, proceed as normal
						if (((phases & PHASE_D) == PHASE_D) || ((parNode->phases & PHASE_D) == PHASE_D))
						{
							//Set appropriate flags (store parent name and flag self & parent)
							SubNode = DIFF_CHILD;
							SubNodeParent = obj->parent;
							
							parNode->SubNode = DIFF_PARENT;
							parNode->SubNodeParent = obj;	//This may get overwritten if we have multiple children, so try not to use it anywhere mission critical.
						parNode->NR_number_child_nodes[0]++;	//Increment the counter of child nodes - we'll alloc and link them later

							//Update the pointer to our parent's NR pointer (so links can go there appropriately)
							NR_subnode_reference = &(parNode->NR_node_reference);

							//Allocate and point our properties up to the parent node
							if (parNode->Extra_Data == NULL)	//Make sure someone else hasn't allocated it for us
							{
								parNode->Extra_Data = (complex *)gl_malloc(9*sizeof(complex));
								if (parNode->Extra_Data == NULL)
								{
									GL_THROW("NR: Memory allocation failure for differently connected load.");
									/*  TROUBLESHOOT
									This is a bug.  Newton-Raphson tried to allocate memory for other necessary
									information to handle a parent-child relationship with differently connected loads.
									Please submit your code and a bug report using the trac website.
									*/
								}
							}
						}
						else	//None are delta, so handle partial phasing a little better
						{
							//Replicate "normal phasing" code below

							//Parent node check occurred above as part of this logic chain, so forgot from copy below
							
							//Set appropriate flags (store parent name and flag self & parent)
							SubNode = CHILD;
							SubNodeParent = obj->parent;
							
							parNode->SubNode = PARENT;
							parNode->SubNodeParent = obj;	//This may get overwritten if we have multiple children, so try not to use it anywhere mission critical.

							//Update the pointer to our parent's NR pointer (so links can go there appropriately)
							NR_subnode_reference = &(parNode->NR_node_reference);

							//Zero out last child power vector (used for updates)
							last_child_power[0][0] = last_child_power[0][1] = last_child_power[0][2] = complex(0,0);
							last_child_power[1][0] = last_child_power[1][1] = last_child_power[1][2] = complex(0,0);
							last_child_power[2][0] = last_child_power[2][1] = last_child_power[2][2] = complex(0,0);
							last_child_current12 = 0.0;
						}
					}

					//Give us no parent now, and let it go to SWING (otherwise generic check fails)
					obj->parent=NULL;

					//Flag our index as a child as well, as yet another catch
					NR_node_reference = -99;

					//Adjust our rank appropriately
					//Ranking - similar to GS
					gl_set_rank(obj,3);				//Put us below normal nodes (but above links)
													//This way load postings should propogate during sync (bottom-up)

				}
			}
			else		//Phase compatible, no issues
			{
				if ((parNode->SubNode==CHILD) | (parNode->SubNode==DIFF_CHILD) | ((obj->parent->parent!=NR_swing_bus) && (obj->parent->parent!=NULL)))	//Our parent is another child
				{
					GL_THROW("NR: Grandchildren are not supported at this time!");
					//Defined above
				}
				else	//Our parent is unchilded (or has the swing bus as a parent)
				{
					//Set appropriate flags (store parent name and flag self & parent)
					SubNode = CHILD;
					SubNodeParent = obj->parent;
					
					parNode->SubNode = PARENT;
					parNode->SubNodeParent = obj;	//This may get overwritten if we have multiple children, so try not to use it anywhere mission critical.
					parNode->NR_number_child_nodes[0]++;	//Increment the counter of child nodes - we'll alloc and link them later

					//Update the pointer to our parent's NR pointer (so links can go there appropriately)
					NR_subnode_reference = &(parNode->NR_node_reference);
				}

				//Give us no parent now, and let it go to SWING (otherwise generic check fails)
				obj->parent=NULL;

				//Flag our index as a child as well, as yet another catch
				NR_node_reference = -99;

				//Adjust our rank appropriately
				//Ranking - similar to GS
				gl_set_rank(obj,3);				//Put us below normal nodes (but above links)
												//This way load postings should propogate during sync (bottom-up)

				//Zero out last child power vector (used for updates)
				last_child_power[0][0] = last_child_power[0][1] = last_child_power[0][2] = complex(0,0);
				last_child_power[1][0] = last_child_power[1][1] = last_child_power[1][2] = complex(0,0);
				last_child_power[2][0] = last_child_power[2][1] = last_child_power[2][2] = complex(0,0);
				last_child_current12 = 0.0;
			}//end no issues phase
		}
		else	//Non-childed node gets the count updated
		{
			NR_bus_count++;		//Update global bus count for NR solver

			//Update ranking
			//Ranking - similar to GS
			if (bustype==SWING)
			{
				gl_set_rank(obj,6);
			}
			else	
			{		
				gl_set_rank(obj,4);
			}
		}

		if ((obj->parent==NULL) && (bustype!=SWING)) // need to find a swing bus to be this node's parent
		{
			gl_set_parent(obj,NR_swing_bus);
		}

		/* Make sure we aren't the swing bus */
		if (bustype!=SWING)
		{
			/* still no swing bus found */
			if (obj->parent==NULL)
				GL_THROW("NR: no swing bus found or specified");
				/*	TROUBLESHOOT
				Newton-Raphson failed to automatically assign a swing bus to the node.  This should
				have been detected by this point and represents a bug in the solver.  Please submit
				a bug report detailing how you obtained this message.
				*/
		}

		//Use SWING node to hook in the solver - since it's called by SWING, might as well
		if (bustype==SWING)
		{
			if (LUSolverName[0]=='\0')	//Empty name, default to superLU
			{
				matrix_solver_method=MM_SUPERLU;	//This is the default, but we'll set it here anyways
			}
			else	//Something is there, see if we can find it
			{
				//Initialize the global
				LUSolverFcns.dllLink = NULL;
				LUSolverFcns.ext_init = NULL;
				LUSolverFcns.ext_alloc = NULL;
				LUSolverFcns.ext_solve = NULL;
				LUSolverFcns.ext_destroy = NULL;

#ifdef WIN32
				snprintf(ext_lib_file_name, 1024, "solver_%s" DLEXT,LUSolverName.get_string());
#else
				snprintf(ext_lib_file_name, 1024, "lib_solver_%s" DLEXT,LUSolverName.get_string());
#endif

				if (gl_findfile(ext_lib_file_name, NULL, 0|4, extpath,sizeof(extpath))!=NULL)	//Link up
				{
					//Link to the library
					LUSolverFcns.dllLink = DLLOAD(extpath);

					//Make sure it worked
					if (LUSolverFcns.dllLink==NULL)
					{
						gl_warning("Failure to load solver_%s as a library, defaulting to superLU",LUSolverName.get_string());
						/*  TROUBLESHOOT
						While attempting to load the DLL for an external LU solver, the file did not appear to meet
						the libary specifications.  superLU is being used instead.  Please check the library file and
						try again.
						*/
						
						//Set to superLU
						matrix_solver_method = MM_SUPERLU;
					}
					else	//Found it right
					{
						//Set tracking flag - if anything fails, just revert to superLU and leave
						ExtLinkFailure = false;

						//Link the callback
						cbackval = (CALLBACKS **)DLSYM(LUSolverFcns.dllLink, "callback");
						
						//I don't know what this does
						if(cbackval)
							*cbackval = callback;
						
						//Now link functions - Init
						LUSolverFcns.ext_init = DLSYM(LUSolverFcns.dllLink,"LU_init");
						
						//Make sure it worked
						if (LUSolverFcns.ext_init == NULL)
						{
							gl_warning("LU_init of external solver solver_%s not found, defaulting to superLU",LUSolverName.get_string());
							/*  TROUBLESHOOT
							While attempting to link the LU_init routine of an external LU matrix solver library, the routine
							failed to be found.  Check the external library and try again.  At this failure, powerflow will revert
							to the superLU solver for Newton-Raphson.
							*/

							//Flag the failure
							ExtLinkFailure = true;
						}

						//Now link functions - alloc
						LUSolverFcns.ext_alloc = DLSYM(LUSolverFcns.dllLink,"LU_alloc");

						//Make sure it worked
						if (LUSolverFcns.ext_alloc == NULL)
						{
							gl_warning("LU_init of external solver solver_%s not found, defaulting to superLU",LUSolverName.get_string());
							/*  TROUBLESHOOT
							While attempting to link the LU_init routine of an external LU matrix solver library, the routine
							failed to be found.  Check the external library and try again.  At this failure, powerflow will revert
							to the superLU solver for Newton-Raphson.
							*/

							//Flag the failure
							ExtLinkFailure = true;
						}

						//Now link functions - solve
						LUSolverFcns.ext_solve = DLSYM(LUSolverFcns.dllLink,"LU_solve");

						//Make sure it worked
						if (LUSolverFcns.ext_solve == NULL)
						{
							gl_warning("LU_init of external solver solver_%s not found, defaulting to superLU",LUSolverName.get_string());
							/*  TROUBLESHOOT
							While attempting to link the LU_init routine of an external LU matrix solver library, the routine
							failed to be found.  Check the external library and try again.  At this failure, powerflow will revert
							to the superLU solver for Newton-Raphson.
							*/

							//Flag the failure
							ExtLinkFailure = true;
						}

						//Now link functions - destroy
						LUSolverFcns.ext_destroy = DLSYM(LUSolverFcns.dllLink,"LU_destroy");

						//Make sure it worked
						if (LUSolverFcns.ext_destroy == NULL)
						{
							gl_warning("LU_init of external solver solver_%s not found, defaulting to superLU",LUSolverName.get_string());
							/*  TROUBLESHOOT
							While attempting to link the LU_init routine of an external LU matrix solver library, the routine
							failed to be found.  Check the external library and try again.  At this failure, powerflow will revert
							to the superLU solver for Newton-Raphson.
							*/

							//Flag the failure
							ExtLinkFailure = true;
						}


						//If any failed, just revert to superLU (probably shouldn't even check others after a failure, but meh)
						if (ExtLinkFailure)
						{
							//Someone failed, just use superLU
							matrix_solver_method=MM_SUPERLU;
						}
						else
						{
							gl_verbose("External solver solver_%s found, utilizing for NR",LUSolverName.get_string());
							/*  TROUBLESHOOT
							An external LU matrix solver library was specified and found, so NR will be calculated
							using that instead of superLU.
							*/

							//Flag as an external solver
							matrix_solver_method=MM_EXTERN;
						}
					}//End found proper DLL
				}//end found external and linked
				else	//Not found, just default to superLU
				{
					gl_warning("The external solver solver_%s could not be found, defaulting to superLU",LUSolverName.get_string());
					/*  TROUBLESHOOT
					While attempting to link an external LU matrix solver library, the file could not be found.  Ensure it is
					in the proper GridLAB-D folder and is named correctly.  If the error persists, please submit your code and
					a bug report via the trac website.
					*/

					//Flag as superLU
					matrix_solver_method=MM_SUPERLU;
				}//end failed to find/load
			}//end somethign was attempted
		}//End matrix solver if

		if (mean_repair_time < 0.0)
		{
			gl_warning("node:%s has a negative mean_repair_time, set to 1 hour",obj->name);
			/*  TROUBLESHOOT
			A node object had a mean_repair_time that is negative.  This ia not a valid setting.
			The value has been changed to 0.  Please set the variable to an appropriate variable
			*/

			mean_repair_time = 0.0;	//Set to zero by default
		}
	}
	else if (solver_method==SM_GS)
	{
		GL_THROW("Gauss_Seidel is a deprecated solver and has been removed");
		/*  TROUBLESHOOT 
		The Gauss-Seidel solver implementation has been removed as of version 3.0.
		It was never fully functional and has not been updated in a couple versions.  The source
		code still exists in older repositories, so if you have an interest in that implementation, please
		try an older subversion number.
		*/
	}
	else if (solver_method == SM_FBS)	//Forward back sweep
	{
		OBJECT *obj = OBJECTHDR(this);

		// Store the topological parent before anyone overwrites it
		TopologicalParent = obj->parent;

		if (obj->parent != NULL)
		{
			if((gl_object_isa(obj->parent,"load","powerflow") | gl_object_isa(obj->parent,"node","powerflow") | gl_object_isa(obj->parent,"meter","powerflow")))	//Parent is another node
			{
				node *parNode = OBJECTDATA(obj->parent,node);

				//Phase variable
				set p_phase_to_check, c_phase_to_check;

				//Create D-less and N-less versions of both for later comparisons
				p_phase_to_check = (parNode->phases & (~(PHASE_D | PHASE_N)));
				c_phase_to_check = (phases & (~(PHASE_D | PHASE_N)));

				//Make sure our phases align, otherwise become angry
				if ((p_phase_to_check & c_phase_to_check) != c_phase_to_check)	//Our parent is lacking, fail
				{
					GL_THROW("Parent and child node phases are incompatible for nodes %s and %s.",obj->parent->name,obj->name);
					/*  TROUBLESHOOT
					A child object does not have compatible phases with its parent.  The parent needs to at least have the phases of
					the child object.  Please check your connections and try again.
					*/
				}
			}//No else here, may be a line due to FBS implementation, so we don't want to fail on that
		}
	}
	else
		GL_THROW("unsupported solver method");
		/*Defined below*/


	/* initialize the powerflow base object */
	int result = powerflow_object::init(parent);

	/* Ranking stuff for GS parent/child relationship - needs to be rethought - premise of loads/nodes above links MUST remain true */
	if (solver_method==SM_GS)
	{
		GL_THROW("Gauss_Seidel is a deprecated solver and has been removed");
		/*  TROUBLESHOOT 
		The Gauss-Seidel solver implementation has been removed as of version 3.0.
		It was never fully functional and has not been updated in a couple versions.  The source
		code still exists in older repositories, so if you have an interest in that implementation, please
		try an older subversion number.
		*/
	}

	/* unspecified phase inherits from parent, if any */
	if (nominal_voltage==0 && parent)
	{
		powerflow_object *pParent = OBJECTDATA(parent,powerflow_object);
		if (gl_object_isa(parent,"transformer"))
		{
			transformer *pTransformer = OBJECTDATA(parent,transformer);
			transformer_configuration *pConfiguration = OBJECTDATA(pTransformer->configuration,transformer_configuration);
			nominal_voltage = pConfiguration->V_secondary;
		}
		else
			nominal_voltage = pParent->nominal_voltage;
	}

	/* make sure the sync voltage limit is positive */
	if (maximum_voltage_error<0)
		throw "negative maximum_voltage_error is invalid";
		/*	TROUBLESHOOT
		A negative maximum voltage error was specified.  This can not be checked.  Specify a
		positive maximum voltage error, or omit this value to let the powerflow solver automatically
		calculate it for you.
		*/

	/* set sync_v_limit to default if needed */
	if (maximum_voltage_error==0.0)
		maximum_voltage_error =  nominal_voltage * default_maximum_voltage_error;

	/* make sure fault_Z is positive */
	if (fault_Z<=0)
		throw "negative fault impedance is invalid";
		/*	TROUBLESHOOT
		The node of interest has a negative or zero fault impedance value.  Specify this value as a
		positive number to enable proper solver operation.
		*/

	/* check that nominal voltage is set */
	if (nominal_voltage<=0)
		throw "nominal_voltage is not set";
		/*	TROUBLESHOOT
		The powerflow solver has detected that a nominal voltage was not specified or is invalid.
		Specify this voltage as a positive value via nominal_voltage to enable the solver to work.
		*/

	/* update geographic degree */
	if (k>1)
	{
		if (geographic_degree>0)
			geographic_degree = n/(1/(geographic_degree/n) + log((double)k));
		else
			geographic_degree = n/log((double)k);
	}

	/* set source flags for SWING and PV buses */
	if (bustype==SWING || bustype==PV)
		busflags |= NF_HASSOURCE;

	//Pre-zero non existant phases - deltas handled by phase (for open delta)
	if (has_phase(PHASE_S))	//Single phase
	{
		if (has_phase(PHASE_A))
		{
			if (voltage[0] == 0)
			{
				voltage[0].SetPolar(nominal_voltage,0.0);
			}
			if (voltage[1] == 0)
			{
				voltage[1].SetPolar(nominal_voltage,0.0);
			}
		}
		else if (has_phase(PHASE_B))
		{
			if (voltage[0] == 0)
			{
				voltage[0].SetPolar(nominal_voltage,-PI*2/3);
			}
			if (voltage[1] == 0)
			{
				voltage[1].SetPolar(nominal_voltage,-PI*2/3);
			}
		}
		else if (has_phase(PHASE_C))
		{
			if (voltage[0] == 0)
			{
				voltage[0].SetPolar(nominal_voltage,PI*2/3);
			}
			if (voltage[1] == 0)
			{
				voltage[1].SetPolar(nominal_voltage,PI*2/3);
			}
		}
		else
			throw("Please specify which phase (A,B,or C) the triplex node is attached to.");

		voltage[2] = complex(0,0);	//Ground always assumed it seems
	}
	else if (has_phase(PHASE_A|PHASE_B|PHASE_C))	//three phase
	{
		if (voltage[0] == 0)
		{
			voltage[0].SetPolar(nominal_voltage,0.0);
		}
		if (voltage[1] == 0)
		{
			voltage[1].SetPolar(nominal_voltage,-2*PI/3);
		}
		if (voltage[2] == 0)
		{
			voltage[2].SetPolar(nominal_voltage,2*PI/3);
		}
	}
	else	//Not three phase - check for individual phases and zero them if they aren't already
	{
		if (!has_phase(PHASE_A))
			voltage[0]=0.0;
		else if (voltage[0] == 0)
			voltage[0].SetPolar(nominal_voltage,0);

		if (!has_phase(PHASE_B))
			voltage[1]=0.0;
		else if (voltage[1] == 0)
			voltage[1].SetPolar(nominal_voltage,-2*PI/3);

		if (!has_phase(PHASE_C))
			voltage[2]=0.0;
		else if (voltage[2] == 0)
			voltage[2].SetPolar(nominal_voltage,2*PI/3);
	}

	if (has_phase(PHASE_D) & voltageAB==0)
	{	// compute 3phase voltage differences
		voltageAB = voltageA - voltageB;
		voltageBC = voltageB - voltageC;
		voltageCA = voltageC - voltageA;
	}
	else if (has_phase(PHASE_S))
	{
		//Compute differential voltages (1-2, 1-N, 2-N)
		voltaged[0] = voltage[0] + voltage[1];
		voltaged[1] = voltage[0] - voltage[2];
		voltaged[2] = voltage[1] - voltage[2];
	}

	//Populate last_voltage with initial values - just in case
	if (solver_method == SM_NR)
	{
		last_voltage[0] = voltage[0];
		last_voltage[1] = voltage[1];
		last_voltage[2] = voltage[2];
	}

	//Initialize uptime variables
	last_disconnect = gl_globalclock;	//Set to current clock

	return result;
}

TIMESTAMP node::presync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t1 = powerflow_object::presync(t0); 
	TIMESTAMP temp_time_value;

	//Determine the flag state - see if a schedule is overriding us
	if (service_status_dbl>-1.0)
	{
		if (service_status_dbl == 0.0)
		{
			service_status = ND_OUT_OF_SERVICE;
		}
		else if (service_status_dbl == 1.0)
		{
			service_status = ND_IN_SERVICE;
		}
		else	//Unknown, toss an error to stop meddling kids
		{
			GL_THROW("Invalid value for service_status_double");
			/*  TROUBLESHOOT
			service_status_double was set to a value other than 0 or 1.  This variable
			is meant as a convenience for schedules to drive the service_status variable, so
			IN_SERVICE=1 and OUT_OF_SERVICE=0 are the only valid states.  Please fix the value
			put into this variable.
			*/
		}
	}

	//Handle disconnect items - only do at new times
	if (prev_NTime!=t0)
	{
		if (service_status == ND_IN_SERVICE)
		{
			//See when the last update was
			if ((last_disconnect == prev_NTime) && (current_uptime == -1.0))	//Just reconnected
			{
				//Store one more
				last_disconnect = t0;
			}

			//Update uptime value
			temp_time_value = t0 - last_disconnect;
			current_uptime = ((double)(temp_time_value))/60.0;	//Put into minutes
		}
		else //Must be out of service
		{
			if (last_disconnect != prev_NTime)	//Just disconnected
			{
				temp_time_value = t0 - last_disconnect;
				previous_uptime = ((double)(temp_time_value))/60.0;	//Put into minutes - automatically shift into prev
			}
			//Default else - already out of service
			current_uptime = -1.0;	//Flag current
			last_disconnect = t0;	//Update tracking variable
		}//End out of service
	}//End disconnect uptime counter

	//Initial phase check - moved so all methods check now
	if (prev_NTime==0)	//Should only be the very first run
	{
		set phase_to_check;
		phase_to_check = (phases & (~(PHASE_D | PHASE_N)));
		
		//See if everything has a source
		if (((phase_to_check & busphasesIn) != phase_to_check) && (busphasesIn != 0))	//Phase mismatch (and not top level node)
		{
			GL_THROW("node:%d (%s) has more phases leaving than entering",obj->id,obj->name);
			/* TROUBLESHOOT
			A node has more phases present than it has sources coming in.  Under the Forward-Back sweep algorithm,
			the system should be strictly radial.  This scenario implies either a meshed system or unconnected
			phases between the from and to nodes of a connected line.  Please adjust the phases appropriately.  Also
			be sure no open switches are the sole connection for a phase, else this will fail as well.  In a few NR
			circumstances, this can also be seen if the "from" and "to" nodes are in reverse order - the "from" node 
			of a link object should be nearest the SWING node, or the node with the most phases - this error check
			will be updated in future versions.
			*/
		}

		//Special FBS code
		if ((solver_method==SM_FBS) && (FBS_swing_set==false))
		{
			//We're the first one in, we must be the SWING - set us as such
			bustype=SWING;

			//Deflag us
			FBS_swing_set=true;
		}
	}

	if (solver_method==SM_NR)
	{
		//Zero the accumulators for later (meters and such)
		current_inj[0] = current_inj[1] = current_inj[2] = 0.0;

		//Reset flag
		current_accumulated = false;

		//If we're the swing, toggle tracking variable
		if (bustype==SWING)
			NR_cycle = !NR_cycle;

		if (prev_NTime==0)	//First run, if we are a child, make sure no one linked us before we knew that
		{
			if (((SubNode == CHILD) || (SubNode == DIFF_CHILD)) && (NR_connected_links>0))
			{
				node *parNode = OBJECTDATA(SubNodeParent,node);

				WRITELOCK_OBJECT(SubNodeParent);	//Lock

				parNode->NR_connected_links[0] += NR_connected_links[0];

				//Zero our accumulator, just in case (used later)
				NR_connected_links[0] = 0;

				//Check and see if we're a house-triplex.  If so, flag our parent so NR works
				if (house_present==true)
				{
					parNode->house_present=true;
				}

				WRITEUNLOCK_OBJECT(SubNodeParent);	//Unlock
			}

			//See if we need to alloc our child space
			if (NR_number_child_nodes[0]>0)	//Malloc it up
			{
				NR_child_nodes = (node**)gl_malloc(NR_number_child_nodes[0] * sizeof(node*));

				//Make sure it worked
				if (NR_child_nodes == NULL)
				{
					GL_THROW("NR: Failed to allocate child node pointing space");
					/*  TROUBLESHOOT
					While attempting to allocate memory for child node connections, something failed.
					Please try again.  If the error persists, please submit your code and a bug report
					via the trac website.
					*/
				}

				//Ensure the pointer is zerod
				NR_number_child_nodes[1] = 0;
			}

			//Rank wise, children should be firing after the above malloc is done - link them beasties up
			if ((SubNode == CHILD) || (SubNode == DIFF_CHILD))
			{
				//Link the parental
				node *parNode = OBJECTDATA(SubNodeParent,node);

				WRITELOCK_OBJECT(SubNodeParent);	//Lock

				//Make sure there's still room
				if (parNode->NR_number_child_nodes[1]>=parNode->NR_number_child_nodes[0])
				{
					gl_error("NR: %s tried to parent to a node that has too many children already",obj->name);
					/*  TROUBLESHOOT
					While trying to link a child to its parent, a memory overrun was encountered.  Please try
					again.  If the error persists, please submit you code and a bug report via the trac website.
					*/

					WRITEUNLOCK_OBJECT(SubNodeParent);	//Unlock

					return TS_INVALID;
				}
				else	//There's space
				{
					//Link us
					parNode->NR_child_nodes[parNode->NR_number_child_nodes[1]] = OBJECTDATA(obj,node);

					//Accumulate the index
					parNode->NR_number_child_nodes[1]++;
				}

				WRITEUNLOCK_OBJECT(SubNodeParent);	//Unlock
			}
		}

		if (NR_busdata==NULL || NR_branchdata==NULL)	//First time any NR in (this should be the swing bus doing this)
		{
			if ( NR_swing_bus!=obj) WRITELOCK_OBJECT(NR_swing_bus);	//Lock Swing for flag

			NR_busdata = (BUSDATA *)gl_malloc(NR_bus_count * sizeof(BUSDATA));
			if (NR_busdata==NULL)
			{
				gl_error("NR: memory allocation failure for bus table");
				/*	TROUBLESHOOT
				This is a bug.  GridLAB-D failed to allocate memory for the bus table for Newton-Raphson.
				Please submit this bug and your code.
				*/

				//Unlock the swing bus
				if ( NR_swing_bus!=obj) WRITEUNLOCK_OBJECT(NR_swing_bus);

				return TS_INVALID;
			}
			NR_curr_bus = 0;	//Pull pointer off flag so other objects know it's built

			NR_branchdata = (BRANCHDATA *)gl_malloc(NR_branch_count * sizeof(BRANCHDATA));
			if (NR_branchdata==NULL)
			{
				gl_error("NR: memory allocation failure for branch table");
				/*	TROUBLESHOOT
				This is a bug.  GridLAB-D failed to allocate memory for the link table for Newton-Raphson.
				Please submit this bug and your code.
				*/

				//Unlock the swing bus
				if ( NR_swing_bus!=obj) WRITEUNLOCK_OBJECT(NR_swing_bus);

				return TS_INVALID;
			}
			NR_curr_branch = 0;	//Pull pointer off flag so other objects know it's built

			//Unlock the swing bus
			if ( NR_swing_bus!=obj) WRITEUNLOCK_OBJECT(NR_swing_bus);

			//Populate the connectivity matrix if a restoration object is present
			if (restoration_object != NULL)
			{
				restoration *Rest_Module = OBJECTDATA(restoration_object,restoration);
				Rest_Module->CreateConnectivity();
			}

			if (bustype==SWING)
			{
				NR_populate();		//Force a first population via the swing bus.  Not really necessary, but I'm saying it has to be this way.
				NR_admit_change = true;	//Ensure the admittance update variable is flagged
			}
			else
			{
				GL_THROW("NR: An order requirement has been violated");
				/*  TROUBLESHOOT
				When initializing the solver, the swing bus should be initialized first for
				Newton-Raphson.  If this does not happen, unexpected results can occur.  Try moving
				the SWING bus to the top of your file.  If the bug persists, submit your code and
				a bug report via the trac website.
				*/
			}
		}//End busdata and branchdata null (first in)

		if ((SubNode==DIFF_PARENT) && (NR_cycle==false))	//Differently connected parent - zero our accumulators
		{
			//Zero them.  Row 1 is power, row 2 is admittance, row 3 is current
			Extra_Data[0] = Extra_Data[1] = Extra_Data[2] = 0.0;

			Extra_Data[3] = Extra_Data[4] = Extra_Data[5] = 0.0;

			Extra_Data[6] = Extra_Data[7] = Extra_Data[8] = 0.0;
		}

		//If we're a parent and "have house", zero our accumulator
		if ((SubNode==PARENT) && (house_present==true))
		{
			nom_res_curr[0] = nom_res_curr[1] = nom_res_curr[2] = 0.0;
		}
	}//end solver_NR call
	else if (solver_method==SM_FBS)
	{
#ifdef SUPPORT_OUTAGES
		if (condition!=OC_NORMAL)	//We're in an abnormal state
		{
			voltage[0] = voltage[1] = voltage[2] = 0.0;	//Zero the voltages
			condition = OC_NORMAL;	//Clear the flag in case we're a switch
		}
#endif
		/* reset the current accumulator */
		current_inj[0] = current_inj[1] = current_inj[2] = complex(0,0);

		/* record the last sync voltage */
		last_voltage[0] = voltage[0];
		last_voltage[1] = voltage[1];
		last_voltage[2] = voltage[2];

		/* get frequency from reference bus */
		if (reference_bus!=NULL)
		{
			node *pRef = OBJECTDATA(reference_bus,node);
			frequency = pRef->frequency;
		}
	}

	//Update NR_mode flag, as necessary (used to be only in triplex devices)
	//Put at bottom so SWING update is seen (otherwise SWING is out of step)
	if (solver_method == SM_NR)
		NR_mode = NR_cycle;		//COpy NR_cycle into NR_mode for houses
	else
		NR_mode = false;		//Just put as false for other methods
	
	return t1;
}

TIMESTAMP node::sync(TIMESTAMP t0)
{
	TIMESTAMP t1 = powerflow_object::sync(t0);
	OBJECT *obj = OBJECTHDR(this);
	
	//Generic time keeping variable - used for phase checks (GS does this explicitly below)
	if (t0!=prev_NTime)
	{
		//Update time tracking variable
		prev_NTime=t0;
	}

	switch (solver_method)
	{
	case SM_FBS:
		{
		if (phases&PHASE_S)
		{	// Split phase
			complex temp_inj[2];
			complex adjusted_curr[3];
			complex temp_curr_val[3];

			if (house_present)
			{
				//Update phase adjustments
				adjusted_curr[0].SetPolar(1.0,voltage[0].Arg());	//Pull phase of V1
				adjusted_curr[1].SetPolar(1.0,voltage[1].Arg());	//Pull phase of V2
				adjusted_curr[2].SetPolar(1.0,voltaged[0].Arg());	//Pull phase of V12

				//Update these current contributions
				temp_curr_val[0] = nom_res_curr[0]/(~adjusted_curr[0]);		//Just denominator conjugated to keep math right (rest was conjugated in house)
				temp_curr_val[1] = nom_res_curr[1]/(~adjusted_curr[1]);
				temp_curr_val[2] = nom_res_curr[2]/(~adjusted_curr[2]);
			}
			else
			{
				temp_curr_val[0] = temp_curr_val[1] = temp_curr_val[2] = 0.0;	//No house present, just zero em
			}

#ifdef SUPPORT_OUTAGES
			if (voltage[0]!=0.0)
			{
#endif
			complex d1 = (voltage1.IsZero() || (power1.IsZero() && shunt1.IsZero())) ? (current1 + temp_curr_val[0]) : (current1 + ~(power1/voltage1) + voltage1*shunt1 + temp_curr_val[0]);
			complex d2 = ((voltage1+voltage2).IsZero() || (power12.IsZero() && shunt12.IsZero())) ? (current12 + temp_curr_val[2]) : (current12 + ~(power12/(voltage1+voltage2)) + (voltage1+voltage2)*shunt12 + temp_curr_val[2]);
			
			current_inj[0] += d1;
			temp_inj[0] = current_inj[0];
			current_inj[0] += d2;

#ifdef SUPPORT_OUTAGES
			}
			else
			{
				temp_inj[0] = 0.0;
				//WRITELOCK_OBJECT(obj);
				current_inj[0]=0.0;
				//UNLOCK_OBJECT(obj);
			}

			if (voltage[1]!=0)
			{
#endif
			d1 = (voltage2.IsZero() || (power2.IsZero() && shunt2.IsZero())) ? (-current2 - temp_curr_val[1]) : (-current2 - ~(power2/voltage2) - voltage2*shunt2 - temp_curr_val[1]);
			d2 = ((voltage1+voltage2).IsZero() || (power12.IsZero() && shunt12.IsZero())) ? (-current12 - temp_curr_val[2]) : (-current12 - ~(power12/(voltage1+voltage2)) - (voltage1+voltage2)*shunt12 - temp_curr_val[2]);

			current_inj[1] += d1;
			temp_inj[1] = current_inj[1];
			current_inj[1] += d2;
			
#ifdef SUPPORT_OUTAGES
			}
			else
			{
				temp_inj[0] = 0.0;
				//WRITELOCK_OBJECT(obj);
				current_inj[1] = 0.0;
				//UNLOCK_OBJECT(obj);
			}
#endif

			if (obj->parent!=NULL && gl_object_isa(obj->parent,"triplex_line","powerflow")) {
				link_object *plink = OBJECTDATA(obj->parent,link_object);
				complex d = plink->tn[0]*current_inj[0] + plink->tn[1]*current_inj[1];
				current_inj[2] += d;
			}
			else {
				complex d = ((voltage1.IsZero() || (power1.IsZero() && shunt1.IsZero())) ||
								   (voltage2.IsZero() || (power2.IsZero() && shunt2.IsZero()))) 
									? currentN : -(temp_inj[0] + temp_inj[1]);
				current_inj[2] += d;
			}
		}
		else if (has_phase(PHASE_D)) 
		{   // 'Delta' connected load
			
			//Convert delta connected power to appropriate line current
			complex delta_current[3];
			complex power_current[3];
			complex delta_shunt[3];
			complex delta_shunt_curr[3];

			delta_current[0]= (voltageAB.IsZero()) ? 0 : ~(powerA/voltageAB);
			delta_current[1]= (voltageBC.IsZero()) ? 0 : ~(powerB/voltageBC);
			delta_current[2]= (voltageCA.IsZero()) ? 0 : ~(powerC/voltageCA);

			power_current[0]=delta_current[0]-delta_current[2];
			power_current[1]=delta_current[1]-delta_current[0];
			power_current[2]=delta_current[2]-delta_current[1];

			//Convert delta connected load to appropriate line current
			delta_shunt[0] = voltageAB*shuntA;
			delta_shunt[1] = voltageBC*shuntB;
			delta_shunt[2] = voltageCA*shuntC;

			delta_shunt_curr[0] = delta_shunt[0]-delta_shunt[2];
			delta_shunt_curr[1] = delta_shunt[1]-delta_shunt[0];
			delta_shunt_curr[2] = delta_shunt[2]-delta_shunt[1];

			//Convert delta-current into a phase current - reuse temp variable
			delta_current[0]=current[0]-current[2];
			delta_current[1]=current[1]-current[0];
			delta_current[2]=current[2]-current[1];

#ifdef SUPPORT_OUTAGES
			for (char kphase=0;kphase<3;kphase++)
			{
				if (voltaged[kphase]==0.0)
				{
					//WRITELOCK_OBJECT(obj);
					current_inj[kphase] = 0.0;
					//UNLOCK_OBJECT(obj);
				}
				else
				{
					//WRITELOCK_OBJECT(obj);
					current_inj[kphase] += delta_current[kphase] + power_current[kphase] + delta_shunt_curr[kphase];
					//UNLOCK_OBJECT(obj);
				}
			}
#else
			complex d[] = {
				delta_current[0] + power_current[0] + delta_shunt_curr[0],
				delta_current[1] + power_current[1] + delta_shunt_curr[1],
				delta_current[2] + power_current[2] + delta_shunt_curr[2]};
			current_inj[0] += d[0];
			current_inj[1] += d[1];
			current_inj[2] += d[2];
#endif
		}
		else 
		{	// 'WYE' connected load

#ifdef SUPPORT_OUTAGES
			for (char kphase=0;kphase<3;kphase++)
			{
				if (voltage[kphase]==0.0)
				{
					//WRITELOCK_OBJECT(obj);
					current_inj[kphase] = 0.0;
					//UNLOCK_OBJECT(obj);
				}
				else
				{
					complex d = ((voltage[kphase]==0.0) || ((power[kphase] == 0) && shunt[kphase].IsZero())) ? current[kphase] : current[kphase] + ~(power[kphase]/voltage[kphase]) + voltage[kphase]*shunt[kphase];
					//WRITELOCK_OBJECT(obj);
					current_inj[kphase] += d;
					//UNLOCK_OBJECT(obj);
				}
			}
#else
			complex d[] = {
				(voltageA.IsZero() || (powerA.IsZero() && shuntA.IsZero())) ? currentA : currentA + ~(powerA/voltageA) + voltageA*shuntA,
				(voltageB.IsZero() || (powerB.IsZero() && shuntB.IsZero())) ? currentB : currentB + ~(powerB/voltageB) + voltageB*shuntB,
				(voltageC.IsZero() || (powerC.IsZero() && shuntC.IsZero())) ? currentC : currentC + ~(powerC/voltageC) + voltageC*shuntC,
			};
			current_inj[0] += d[0];
			current_inj[1] += d[1];
			current_inj[2] += d[2];
#endif
		}

#ifdef SUPPORT_OUTAGES
	if (is_open_any())
		throw "unable to handle node open phase condition";

	if (is_contact_any())
	{
		/* phase-phase contact */
		if (is_contact(PHASE_A|PHASE_B|PHASE_C))
			voltageA = voltageB = voltageC = (voltageA + voltageB + voltageC)/3;
		else if (is_contact(PHASE_A|PHASE_B))
			voltageA = voltageB = (voltageA + voltageB)/2;
		else if (is_contact(PHASE_B|PHASE_C))
			voltageB = voltageC = (voltageB + voltageC)/2;
		else if (is_contact(PHASE_A|PHASE_C))
			voltageA = voltageC = (voltageA + voltageC)/2;

		/* phase-neutral/ground contact */
		if (is_contact(PHASE_A|PHASE_N) || is_contact(PHASE_A|GROUND))
			voltageA /= 2;
		if (is_contact(PHASE_B|PHASE_N) || is_contact(PHASE_B|GROUND))
			voltageB /= 2;
		if (is_contact(PHASE_C|PHASE_N) || is_contact(PHASE_C|GROUND))
			voltageC /= 2;
	}
#endif

		// if the parent object is another node
		if (obj->parent!=NULL && gl_object_isa(obj->parent,"node","powerflow"))
		{
			node *pNode = OBJECTDATA(obj->parent,node);

			//Check to make sure phases are correct - ignore Deltas and neutrals (load changes take care of those)
			if (((pNode->phases & phases) & (!(PHASE_D | PHASE_N))) == (phases & (!(PHASE_D | PHASE_N))))
			{
				// add the injections on this node to the parent
				WRITELOCK_OBJECT(obj->parent);
				pNode->current_inj[0] += current_inj[0];
				pNode->current_inj[1] += current_inj[1];
				pNode->current_inj[2] += current_inj[2];
				WRITEUNLOCK_OBJECT(obj->parent);
			}
			else
				GL_THROW("Node:%d's parent does not have the proper phase connection to be a parent.",obj->id);
				/*  TROUBLESHOOT
				A parent-child relationship was attempted when the parent node does not contain the phases
				of the child node.  Ensure parent nodes have at least the phases of the child object.
				*/
		}

		break;
		}
	case SM_NR:
		{
			//Reliability check - sets and removes voltages (theory being previous answer better than starting at 0)
			unsigned char phase_checks_var;

			//See if we've been initialized or not
			if (NR_node_reference!=-1)
			{
				//Make sure we're a real boy - if we're not, do nothing (we'll steal mommy's or daddy's voltages in postsync)
				if ((SubNode!=CHILD) && (SubNode!=DIFF_CHILD))
				{
					//Figre out what has changed
					phase_checks_var = ((NR_busdata[NR_node_reference].phases ^ prev_phases) & 0x8F);

					if (phase_checks_var != 0x00)	//Something's changed
					{
						//See if it is a triplex
						if ((NR_busdata[NR_node_reference].origphases & 0x80) == 0x80)
						{
							//See if A, B, or C appeared, or disappeared
							if ((NR_busdata[NR_node_reference].phases & 0x80) == 0x00)	//No phases means it was just removed
							{
								//Store V1 and V2
								last_voltage[0] = voltage[0];
								last_voltage[1] = voltage[1];
								last_voltage[2] = voltage[2];

								//Clear them out
								voltage[0] = 0.0;
								voltage[1] = 0.0;
								voltage[2] = 0.0;
							}
							else	//Put back in service
							{
								voltage[0] = last_voltage[0];
								voltage[1] = last_voltage[1];
								voltage[2] = last_voltage[2];
							}

							//Recalculated V12, V1N, V2N in case a child uses them
							voltaged[0] = voltage[0] + voltage[1];
							voltaged[1] = voltage[0] - voltage[2];
							voltaged[2] = voltage[0] - voltage[2];
						}//End triplex
						else	//Nope
						{
							//Find out changes, and direction
							if ((phase_checks_var & 0x04) == 0x04)	//Phase A change
							{
								//See which way
								if ((prev_phases & 0x04) == 0x04)	//Means A just disappeared
								{
									last_voltage[0] = voltage[0];	//Store the last value
									voltage[0] = 0.0;				//Put us to zero, so volt_dump is happy
								}
								else	//A just came back
								{
									voltage[0] = last_voltage[0];	//Read in the previous values
								}
							}//End Phase A change

							//Find out changes, and direction
							if ((phase_checks_var & 0x02) == 0x02)	//Phase B change
							{
								//See which way
								if ((prev_phases & 0x02) == 0x02)	//Means B just disappeared
								{
									last_voltage[1] = voltage[1];	//Store the last value
									voltage[1] = 0.0;				//Put us to zero, so volt_dump is happy
								}
								else	//A just came back
								{
									voltage[1] = last_voltage[1];	//Read in the previous values
								}
							}//End Phase B change

							//Find out changes, and direction
							if ((phase_checks_var & 0x01) == 0x01)	//Phase C change
							{
								//See which way
								if ((prev_phases & 0x01) == 0x01)	//Means C just disappeared
								{
									last_voltage[2] = voltage[2];	//Store the last value
									voltage[2] = 0.0;				//Put us to zero, so volt_dump is happy
								}
								else	//A just came back
								{
									voltage[2] = last_voltage[2];	//Read in the previous values
								}
							}//End Phase C change

							//Recalculated VAB, VBC, and VCA, in case a child uses them
							voltaged[0] = voltage[0] - voltage[1];
							voltaged[1] = voltage[1] - voltage[2];
							voltaged[2] = voltage[2] - voltage[0];
						}//End not triplex

						//Assign current value in
						prev_phases = NR_busdata[NR_node_reference].phases;
					}//End Phase checks for reliability
				}//End normal node

				if ((SubNode==CHILD) && (NR_cycle==false))
				{
					//Post our loads up to our parent
					node *ParToLoad = OBJECTDATA(SubNodeParent,node);

					if (gl_object_isa(SubNodeParent,"load","powerflow"))	//Load gets cleared at every presync, so reaggregate :(
					{
						//Lock the parent for accumulation
						LOCK_OBJECT(SubNodeParent);

						//Import power and "load" characteristics
						ParToLoad->power[0]+=power[0];
						ParToLoad->power[1]+=power[1];
						ParToLoad->power[2]+=power[2];

						ParToLoad->shunt[0]+=shunt[0];
						ParToLoad->shunt[1]+=shunt[1];
						ParToLoad->shunt[2]+=shunt[2];

						ParToLoad->current[0]+=current[0];
						ParToLoad->current[1]+=current[1];
						ParToLoad->current[2]+=current[2];

						//All done, unlock
						UNLOCK_OBJECT(SubNodeParent);
					}
					else if (gl_object_isa(SubNodeParent,"node","powerflow"))	//"parented" node - update values - This has to go to the bottom
					{												//since load/meter share with node (and load handles power in presync)
						//Lock the parent for accumulation
						LOCK_OBJECT(SubNodeParent);

						//Import power and "load" characteristics
						ParToLoad->power[0]+=power[0]-last_child_power[0][0];
						ParToLoad->power[1]+=power[1]-last_child_power[0][1];
						ParToLoad->power[2]+=power[2]-last_child_power[0][2];

						ParToLoad->shunt[0]+=shunt[0]-last_child_power[1][0];
						ParToLoad->shunt[1]+=shunt[1]-last_child_power[1][1];
						ParToLoad->shunt[2]+=shunt[2]-last_child_power[1][2];

						ParToLoad->current[0]+=current[0]-last_child_power[2][0];
						ParToLoad->current[1]+=current[1]-last_child_power[2][1];
						ParToLoad->current[2]+=current[2]-last_child_power[2][2];

						if (has_phase(PHASE_S))	//Triplex gets another term as well
						{
							ParToLoad->current12 +=current12-last_child_current12;
						}

						//See if we have a house!
						if (house_present==true)	//Add our values into our parent's accumulator!
						{
							ParToLoad->nom_res_curr[0] += nom_res_curr[0];
							ParToLoad->nom_res_curr[1] += nom_res_curr[1];
							ParToLoad->nom_res_curr[2] += nom_res_curr[2];
						}

						//Unlock the parent now that we are done
						UNLOCK_OBJECT(SubNodeParent);
					}
					else
					{
						GL_THROW("NR: Object %d is a child of something that it shouldn't be!",obj->id);
						/*  TROUBLESHOOT
						A Newton-Raphson object is childed to something it should not be (not a load, node, or meter).
						This should have been caught earlier and is likely a bug.  Submit your code and a bug report using the trac website.
						*/
					}

					//Update previous power tracker
					last_child_power[0][0] = power[0];
					last_child_power[0][1] = power[1];
					last_child_power[0][2] = power[2];

					last_child_power[1][0] = shunt[0];
					last_child_power[1][1] = shunt[1];
					last_child_power[1][2] = shunt[2];

					last_child_power[2][0] = current[0];
					last_child_power[2][1] = current[1];
					last_child_power[2][2] = current[2];

					if (has_phase(PHASE_S))		//Triplex extra current update
						last_child_current12 = current12;
				}

				if ((SubNode==DIFF_CHILD) && (NR_cycle==false))	//Differently connected nodes
				{
					//Post our loads up to our parent - in the appropriate fashion
					node *ParToLoad = OBJECTDATA(SubNodeParent,node);

					//Lock the parent for accumulation
					LOCK_OBJECT(SubNodeParent);

					//Update post them.  Row 1 is power, row 2 is admittance, row 3 is current
					ParToLoad->Extra_Data[0] += power[0];
					ParToLoad->Extra_Data[1] += power[1];
					ParToLoad->Extra_Data[2] += power[2];

					ParToLoad->Extra_Data[3] += shunt[0];
					ParToLoad->Extra_Data[4] += shunt[1];
					ParToLoad->Extra_Data[5] += shunt[2];

					ParToLoad->Extra_Data[6] += current[0];
					ParToLoad->Extra_Data[7] += current[1];
					ParToLoad->Extra_Data[8] += current[2];

					//Finished, unlock parent
					UNLOCK_OBJECT(SubNodeParent);
				}

				if (NR_cycle==true)	//Accumulation cycle, compute our current injections
				{
					//This code will become irrelevant once NR is "flattened" - for now, it just repeats calculations
					int resultval = NR_current_update(false,false);

					//Make sure it worked, just to be thorough
					if (resultval != 1)
					{
						GL_THROW("Attempt to update current/power on node:%s failed!",obj->name);
						/*  TROUBLESHOOT
						While attempting to update the current on a node object, an error was encountered.  Please try again.  If the error persists,
						please submit your code and a bug report via the trac website.
						*/
					}
				}//End NR true
			}//end uninitialized

			if ((NR_curr_bus==NR_bus_count) && (bustype==SWING))	//Only run the solver once everything has populated
			{
				if (NR_cycle==false)	//Solving pass
				{
					bool bad_computation=false;

					int64 result = solver_nr(NR_bus_count, NR_busdata, NR_branch_count, NR_branchdata, &bad_computation);

					//De-flag the change - no contention should occur
					NR_admit_change = false;

					if (bad_computation==true)
					{
						GL_THROW("Newton-Raphson method is unable to converge to a solution at this operation point");
						/*  TROUBLESHOOT
						Newton-Raphson has failed to complete even a single iteration on the powerflow.  This is an indication
						that the method will not solve the system and may have a singularity or other ill-favored condition in the
						system matrices.
						*/
					}
					else if (result<0)	//Failure to converge, but we just let it stay where we are for now
					{
						gl_verbose("Newton-Raphson failed to converge, sticking at same iteration.");
						/*  TROUBLESHOOT
						Newton-Raphson failed to converge in the number of iterations specified in NR_iteration_limit.
						It will try again (if the global iteration limit has not been reached).
						*/
						NR_retval=t0;
					}
					else
						NR_retval=t1;

					return t0;		//Stay here whether we like it or not (second pass needs to complete)
				}
				else	//Accumulating pass, let us go where we wanted
					return NR_retval;
			}
			else if (NR_curr_bus==NR_bus_count)	//Population complete, we're not swing, let us go (or we never go on)
				return t1;
			else	//Population of data busses is not complete.  Flag us for a go-around, they should be ready next time
			{
				if (NR_cycle==false)
					return t0;
				else	//True cycle - everything should have been populated by now
				{
					if (bustype==SWING)	//Only error on swing - if errors with others, seems to be upset.  Too lazy to track down why.
					{
						GL_THROW("All nodes were not properly populated");
						/*  TROUBLESHOOT
						The NR solver is still waiting to initialize an object on the second pass.  Everything should have
						initialized on the first pass.  Look for orphaned node objects that do not have a line attached and
						try again.  If the error persists, please submit your code and a bug report via the trac website.
						*/
					}
					else
						return t0;
				}
			}
			break;
		}
	default:
		GL_THROW("unsupported solver method");
		/*	TROUBLESHOOT
		An invalid powerflow solver was specified.  Currently acceptable values are FBS for forward-back
		sweep (Kersting's method), GS for Gauss-Seidel, and NR for Newton-Raphson.
		*/
		break;
	}
	return t1;
}

TIMESTAMP node::postsync(TIMESTAMP t0)
{
	TIMESTAMP t1 = powerflow_object::postsync(t0);
	TIMESTAMP RetValue=t1;
	OBJECT *obj = OBJECTHDR(this);

#ifdef SUPPORT_OUTAGES
	if (condition!=OC_NORMAL)	//Zero all the voltages, just in case
	{
		voltage[0] = voltage[1] = voltage[2] = 0.0;
	}

	if (is_contact_any())
	{
		complex dVAB = voltageA - voltageB;
		complex dVBC = voltageB - voltageC;
		complex dVCA = voltageC - voltageA;

		/* phase-phase contact */
		//WRITELOCK_OBJECT(obj);
		if (is_contact(PHASE_A|PHASE_B|PHASE_C))
			/** @todo calculate three-way contact fault current */
			throw "three-way contact not supported yet";
		else if (is_contact(PHASE_A|PHASE_B))
			current_inj[0] = - current_inj[1] = dVAB/fault_Z;
		else if (is_contact(PHASE_B|PHASE_C))
			current_inj[1] = - current_inj[2] = dVBC/fault_Z;
		else if (is_contact(PHASE_A|PHASE_C))
			current_inj[2] = - current_inj[0] = dVCA/fault_Z;

		/* phase-neutral/ground contact */
		if (is_contact(PHASE_A|PHASE_N) || is_contact(PHASE_A|GROUND))
			current_inj[0] = voltageA / fault_Z;
		if (is_contact(PHASE_B|PHASE_N) || is_contact(PHASE_B|GROUND))
			current_inj[1] = voltageB / fault_Z;
		if (is_contact(PHASE_C|PHASE_N) || is_contact(PHASE_C|GROUND))
			current_inj[2] = voltageC / fault_Z;
		//UNLOCK_OBJECT(obj);
	}

	/* record the power in for posterity */
	//kva_in = (voltageA*~current[0] + voltageB*~current[1] + voltageC*~current[2])/1000; /*...or not.  Note sure how this works for a node*/

#endif
	//This code performs the new "flattened" NR calculations.
	//Once properly "flattened", this code won't need the cycle indicator
	if ((solver_method == SM_NR) && (NR_cycle==false))
	{
		int result = NR_current_update(true,false);

		//Make sure it worked, just to be thorough
		if (result != 1)
		{
			GL_THROW("Attempt to update current/power on node:%s failed!",obj->name);
			//Defined elsewhere
		}
	}

	/* check for voltage control requirement */
	if (require_voltage_control==TRUE)
	{
		/* PQ bus must have a source */
		if ((busflags&NF_HASSOURCE)==0 && bustype==PQ)
			voltage[0] = voltage[1] = voltage[2] = complex(0,0);
	}

	//Update appropriate "other" voltages
	if (phases&PHASE_S) 
	{	// split-tap voltage diffs are different
		voltage12 = voltage1 + voltage2;
		voltage1N = voltage1 - voltageN;
		voltage2N = voltage2 - voltageN;
	}
	else
	{	// compute 3phase voltage differences
		voltageAB = voltageA - voltageB;
		voltageBC = voltageB - voltageC;
		voltageCA = voltageC - voltageA;
	}

	if (solver_method==SM_FBS)
	{
		// if the parent object is a node
		if (obj->parent!=NULL && (gl_object_isa(obj->parent,"node","powerflow")))
		{
			// copy the voltage from the parent - check for mismatch handled earlier
			node *pNode = OBJECTDATA(obj->parent,node);
			voltage[0] = pNode->voltage[0];
			voltage[1] = pNode->voltage[1];
			voltage[2] = pNode->voltage[2];

			//Re-update our Delta or single-phase equivalents since we now have a new voltage
			//Update appropriate "other" voltages
			if (phases&PHASE_S) 
			{	// split-tap voltage diffs are different
				voltage12 = voltage1 + voltage2;
				voltage1N = voltage1 - voltageN;
				voltage2N = voltage2 - voltageN;
			}
			else
			{	// compute 3phase voltage differences
				voltageAB = voltageA - voltageB;
				voltageBC = voltageB - voltageC;
				voltageCA = voltageC - voltageA;
			}
		}
	}

#ifdef SUPPORT_OUTAGES
	/* check the voltage status for loads */
	if (phases&PHASE_S) // split-phase node
	{
		double V1pu = voltage1.Mag()/nominal_voltage;
		double V2pu = voltage2.Mag()/nominal_voltage;
		if (V1pu<0.8 || V2pu<0.8)
			status=UNDERVOLT;
		else if (V1pu>1.2 || V2pu>1.2)
			status=OVERVOLT;
		else
			status=NOMINAL;
	}
	else // three-phase node
	{
		double VApu = voltageA.Mag()/nominal_voltage;
		double VBpu = voltageB.Mag()/nominal_voltage;
		double VCpu = voltageC.Mag()/nominal_voltage;
		if (VApu<0.8 || VBpu<0.8 || VCpu<0.8)
			status=UNDERVOLT;
		else if (VApu>1.2 || VBpu>1.2 || VCpu>1.2)
			status=OVERVOLT;
		else
			status=NOMINAL;
	}
#endif
	if (solver_method==SM_FBS)
	{
		/* compute the sync voltage change */
		double sync_V = (last_voltage[0]-voltage[0]).Mag() + (last_voltage[1]-voltage[1]).Mag() + (last_voltage[2]-voltage[2]).Mag();
		
		/* if the sync voltage limit is defined and the sync voltage is larger */
		if (sync_V > maximum_voltage_error){

			/* request another pass */
			RetValue=t0;
		}
	}

	/* the solution is satisfactory */
	return RetValue;
}

int node::kmldump(FILE *fp)
{
	OBJECT *obj = OBJECTHDR(this);
	if (isnan(obj->latitude) || isnan(obj->longitude))
		return 0;
	fprintf(fp,"<Placemark>\n");
	if (obj->name)
		fprintf(fp,"<name>%s</name>\n", obj->name, obj->oclass->name, obj->id);
	else
		fprintf(fp,"<name>%s %d</name>\n", obj->oclass->name, obj->id);
	fprintf(fp,"<description>\n");
	fprintf(fp,"<![CDATA[\n");
	fprintf(fp,"<TABLE>\n");
	fprintf(fp,"<TR><TD WIDTH=\"25%\">%s&nbsp;%d<HR></TD><TH WIDTH=\"25%\" ALIGN=CENTER>Phase A<HR></TH><TH WIDTH=\"25%\" ALIGN=CENTER>Phase B<HR></TH><TH WIDTH=\"25%\" ALIGN=CENTER>Phase C<HR></TH></TR>\n", obj->oclass->name, obj->id);

	// voltages
	fprintf(fp,"<TR><TH ALIGN=LEFT>Voltage</TH>");
	double vscale = primary_voltage_ratio*sqrt((double) 3.0)/(double) 1000.0;
	if (has_phase(PHASE_A))
		fprintf(fp,"<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kV&nbsp;&nbsp;<BR>%.3f&nbsp;deg&nbsp;</TD>",
			voltageA.Mag()*vscale,voltageA.Arg()*180/3.1416);
	else
		fprintf(fp,"<TD></TD>");
	if (has_phase(PHASE_B))
		fprintf(fp,"<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kV&nbsp;&nbsp;<BR>%.3f&nbsp;deg&nbsp;</TD>",
			voltageB.Mag()*vscale,voltageB.Arg()*180/3.1416);
	else
		fprintf(fp,"<TD></TD>");
	if (has_phase(PHASE_C))
		fprintf(fp,"<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kV&nbsp;&nbsp;<BR>%.3f&nbsp;deg&nbsp;</TD>",
			voltageC.Mag()*vscale,voltageC.Arg()*180/3.1416);
	else
		fprintf(fp,"<TD></TD>");

	// supply
	/// @todo complete KML implement of supply (ticket #133)

	// demand
	if (gl_object_isa(obj,"load"))
	{
		load *pLoad = OBJECTDATA(obj,load);
		fprintf(fp,"<TR><TH ALIGN=LEFT>Load</TH>");
		if (has_phase(PHASE_A))
		{
			complex load_A = ~voltageA*pLoad->constant_current[0] + pLoad->powerA;
			fprintf(fp,"<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kW&nbsp;&nbsp;<BR>%.3f&nbsp;kVAR</TD>",
				load_A.Re(),load_A.Im());
		}
		else
			fprintf(fp,"<TD></TD>");
		if (has_phase(PHASE_B))
		{
			complex load_B = ~voltageB*pLoad->constant_current[1] + pLoad->powerB;
			fprintf(fp,"<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kW&nbsp;&nbsp;<BR>%.3f&nbsp;kVAR</TD>",
				load_B.Re(),load_B.Im());
		}
		else
			fprintf(fp,"<TD></TD>");
		if (has_phase(PHASE_C))
		{
			complex load_C = ~voltageC*pLoad->constant_current[2] + pLoad->powerC;
			fprintf(fp,"<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kW&nbsp;&nbsp;<BR>%.3f&nbsp;kVAR</TD>",
				load_C.Re(),load_C.Im());
		}
		else
			fprintf(fp,"<TD></TD>");
	}
	fprintf(fp,"</TR>\n");
	fprintf(fp,"</TABLE>\n");
	fprintf(fp,"]]>\n");
	fprintf(fp,"</description>\n");
	fprintf(fp,"<Point>\n");
	fprintf(fp,"<coordinates>%f,%f</coordinates>\n",obj->longitude,obj->latitude);
	fprintf(fp,"</Point>\n");
	fprintf(fp,"</Placemark>\n");
	return 0;
}

//Notify function
//NOTE: The NR-based notify stuff may no longer be needed after NR is "flattened", since it will
//      effectively be like FBS at that point.
int node::notify(int update_mode, PROPERTY *prop, char *value)
{
	complex diff_val;

	if (solver_method == SM_NR)
	{
		//Only even bother with voltage updates if we're properly populated
		if (prev_voltage_value != NULL)
		{
			//See if there is a voltage update - phase A
			if (strcmp(prop->name,"voltage_A")==0)
			{
				if (update_mode==NM_PREUPDATE)
				{
					//Store the last value
					prev_voltage_value[0] = voltage[0];
				}
				else if (update_mode==NM_POSTUPDATE)
				{
					//Handle the logics
					if (NR_cycle==false)	//Beginning of true pass - accumulator pass (toggle hasn't occurred)
					{
						//See what the difference is - if it is above the convergence limit, send an NR update
						diff_val = voltage[0] - prev_voltage_value[0];

						if (diff_val.Mag() >= maximum_voltage_error)	//Outside of range, so force a new iteration
						{
							NR_retval = gl_globalclock;
						}
					}
				}
			}

			//See if there is a voltage update - phase B
			if (strcmp(prop->name,"voltage_B")==0)
			{
				if (update_mode==NM_PREUPDATE)
				{
					//Store the last value
					prev_voltage_value[1] = voltage[1];
				}
				else if (update_mode==NM_POSTUPDATE)
				{
					//Handle the logics
					if (NR_cycle==false)	//Beginning of true pass - accumulator pass (toggle hasn't occurred)
					{
						//See what the difference is - if it is above the convergence limit, send an NR update
						diff_val = voltage[1] - prev_voltage_value[1];

						if (diff_val.Mag() >= maximum_voltage_error)	//Outside of range, so force a new iteration
						{
							NR_retval = gl_globalclock;
						}
					}
				}
			}

			//See if there is a voltage update - phase C
			if (strcmp(prop->name,"voltage_C")==0)
			{
				if (update_mode==NM_PREUPDATE)
				{
					//Store the last value
					prev_voltage_value[2] = voltage[2];
				}
				else if (update_mode==NM_POSTUPDATE)
				{
					//Handle the logics
					if (NR_cycle==false)	//Beginning of true pass - accumulator pass (toggle hasn't occurred)
					{
						//See what the difference is - if it is above the convergence limit, send an NR update
						diff_val = voltage[2] - prev_voltage_value[2];

						if (diff_val.Mag() >= maximum_voltage_error)	//Outside of range, so force a new iteration
						{
							NR_retval = gl_globalclock;
						}
					}
				}
			}
		}
	}//End NR
	//Default else - FBS's iteration methods aren't sensitive to this

	return 1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: node
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_node(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(node::oclass);
		if (*obj!=NULL)
		{
			node *my = OBJECTDATA(*obj,node);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(node);
}

// Commit function
EXPORT TIMESTAMP commit_node(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	node *pNode = OBJECTDATA(obj,node);
	try {
		// This zeroes out all of the unused phases at each node in the FBS method
		if (solver_method==SM_FBS)
		{
			if (pNode->has_phase(PHASE_A)) {
			//leave it
			}
			else
				pNode->voltage[0] = complex(0,0);

			if (pNode->has_phase(PHASE_B)) {
				//leave it
			}
			else
				pNode->voltage[1] = complex(0,0);

			if (pNode->has_phase(PHASE_C)) {
				//leave it
			}
			else
				pNode->voltage[2] = complex(0,0);
			
		}
		return TS_NEVER;
	}
	catch (char *msg)
	{
		gl_error("%s (node:%d): %s", pNode->get_name(), pNode->get_id(), msg);
		return 0; 
	}

}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_node(OBJECT *obj)
{
	try {
		node *my = OBJECTDATA(obj,node);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(node);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_node(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		node *pObj = OBJECTDATA(obj,node);
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
	SYNC_CATCHALL(node);
}

/**
* NR_populate is called by link objects during their first presync if a node is not
* "initialized".  This function "initializes" the node into the Newton-Raphson data
* structure NR_busdata
*
*/
int node::NR_populate(void)
{
		//Object header for names
		OBJECT *me = OBJECTHDR(this);

		//Lock the SWING for global operations
		if ( NR_swing_bus!=me ) LOCK_OBJECT(NR_swing_bus);

		NR_node_reference = NR_curr_bus;	//Grab the current location and keep it as our own
		NR_curr_bus++;					//Increment the current bus pointer for next variable
		if ( NR_swing_bus!=me ) UNLOCK_OBJECT(NR_swing_bus);	//All done playing with globals, unlock the swing so others can proceed

		//Quick check to see if there problems
		if (NR_node_reference == -1)
		{
			GL_THROW("NR: bus:%s failed to grab a unique bus index value!",me->name);
			/*  TROUBLESHOOT
			While attempting to gain a unique bus id for the Newton-Raphson solver, an error
			was encountered.  This may be related to a parallelization effort.  Please try again.
			If the error persists, please submit your code and a bug report via the trac website.
			*/
		}

		//Bus type
		NR_busdata[NR_node_reference].type = (int)bustype;

		//Interim check to make sure it isn't a PV bus, since those aren't supported yet - this will get removed when that functionality is put in place
		if (NR_busdata[NR_node_reference].type==1)
		{
			GL_THROW("NR: bus:%s is a PV bus - these are not yet supported.",me->name);
			/*  TROUBLESHOOT
			The Newton-Raphson solver implemented does not currently support the PV bus type.
			*/
		}

		//Populate phases
		NR_busdata[NR_node_reference].phases = 128*has_phase(PHASE_S) + 8*has_phase(PHASE_D) + 4*has_phase(PHASE_A) + 2*has_phase(PHASE_B) + has_phase(PHASE_C);

		//Link our name in
		NR_busdata[NR_node_reference].name = me->name;

		//Link us in as well
		NR_busdata[NR_node_reference].obj = me;

		//Link our maximum error vlaue
		NR_busdata[NR_node_reference].max_volt_error = maximum_voltage_error;

		//Populate voltage
		NR_busdata[NR_node_reference].V = &voltage[0];
		
		//Populate power
		NR_busdata[NR_node_reference].S = &power[0];

		//Populate admittance
		NR_busdata[NR_node_reference].Y = &shunt[0];

		//Populate current
		NR_busdata[NR_node_reference].I = &current[0];

		//Allocate our link list
		NR_busdata[NR_node_reference].Link_Table = (int *)gl_malloc(NR_connected_links[0]*sizeof(int));
		
		if (NR_busdata[NR_node_reference].Link_Table == NULL)
		{
			GL_THROW("NR: Failed to allocate link table for node:%d",me->id);
			/*  TROUBLESHOOT
			While attempting to allocate memory for the linking table for NR, memory failed to be
			allocated.  Make sure you have enough memory and try again.  If this problem happens a second
			time, submit your code and a bug report using the trac website.
			*/
		}

		//If a restoration object is present, create space for an equivalent table
		if (restoration_object != NULL)
		{
			if (NR_connected_links[0] != 1)	//Make sure we aren't an only child
			{
				NR_busdata[NR_node_reference].Child_Nodes = (int *)gl_malloc((NR_connected_links[0]-1)*sizeof(int));

				if (NR_busdata[NR_node_reference].Child_Nodes == NULL)
				{
					GL_THROW("NR: Failed to allocate child node table for node:%d - %s",me->id,me->name);
					/*  TROUBLESHOOT
					While attempting to allocate memory for a tree table for the restoration module, memory failed to be allocated.
					Make sure you ahve enough memory and try again.  If the problem persists, please submit your code and a bug report
					to the trac website.
					*/
				}

				//Initialize the children and the parent
				NR_busdata[NR_node_reference].Parent_Node = -1;

				for (unsigned int index=0; index<(NR_connected_links[0]-1); index++)
					NR_busdata[NR_node_reference].Child_Nodes[index] = -1;

				NR_busdata[NR_node_reference].Child_Node_idx = 0;	//Initialize the population index
			}
			else	//Only child, ensure we set it as zero
			{
				if (bustype == SWING)	//Swing bus isn't an only child, it's a single parent
				{
					NR_busdata[NR_node_reference].Child_Nodes = (int *)gl_malloc(sizeof(int));	//Just allocate one spot in this case

					if (NR_busdata[NR_node_reference].Child_Nodes == NULL)
					{
						GL_THROW("NR: Failed to allocate child node table for node:%d - %s",me->id,me->name);
						//Defined above
					}

					NR_busdata[NR_node_reference].Child_Nodes[0] = -1;	//Initialize it to -1 - flags as unpopulated

					NR_busdata[NR_node_reference].Parent_Node = 0;	//We're our own parent (the implications are astounding)
				}
				else	//Normal only child
				{
					NR_busdata[NR_node_reference].Child_Nodes = 0;
				}
			}
		}

		//Populate our size
		NR_busdata[NR_node_reference].Link_Table_Size = NR_connected_links[0];

		//See if we're a triplex
		if (has_phase(PHASE_S))
		{
			if (house_present)	//We're a proud parent of a house!
			{
				NR_busdata[NR_node_reference].house_var = &nom_res_curr[0];	//Separate storage area for nominal house currents
				NR_busdata[NR_node_reference].phases |= 0x40;					//Flag that we are a house-attached node
			}

			NR_busdata[NR_node_reference].extra_var = &current12;	//Stored in a separate variable and this is the easiest way for me to get it
		}
		else if (SubNode==DIFF_PARENT)	//Differently connected load/node (only can't be S)
		{
			NR_busdata[NR_node_reference].extra_var = Extra_Data;
			NR_busdata[NR_node_reference].phases |= 0x10;			//Special flag for a phase mismatch being present
		}

		//Per unit values
		NR_busdata[NR_node_reference].kv_base = -1.0;
		NR_busdata[NR_node_reference].mva_base = -1.0;

		//Set the matrix value to -1 to know it hasn't been set (probably not necessary)
		NR_busdata[NR_node_reference].Matrix_Loc = -1;

		//Populate original phases
		NR_busdata[NR_node_reference].origphases = NR_busdata[NR_node_reference].phases;

		//Populate our tracking variable
		prev_phases = NR_busdata[NR_node_reference].phases;

		return 0;
}

//Computes "load" portions of current injection
//postpass is set to true for the "postsync" power update - it does extra child node items needed
//parentcall is set when a parent object has called this update - mainly for locking purposes
//NOTE: Once NR gets collapsed into single pass form, the "postpass" flag will probably be irrelevant and could be removed
//      Once "flattened", this function shoud only be called by postsync, so why flag what always is true?
int node::NR_current_update(bool postpass, bool parentcall)
{
	unsigned int table_index;
	link_object *temp_link;
	int temp_result;
	OBJECT *obj = OBJECTHDR(this);
	complex temp_current_inj[3];

	//Don't do anything if we've already been "updated"
	if (current_accumulated==false)
	{
		if (postpass)	//Perform what used to be in post-synch - used to childed object items - occurred on "false" pass, so goes before all
		{
			if ((SubNode==CHILD) && (NR_cycle==false))	//Remove child contributions
			{
				node *ParToLoad = OBJECTDATA(SubNodeParent,node);

				if (!parentcall)	//We weren't called by our parent, so lock us to create sibling rivalry!
				{
					//Lock the parent for writing
					LOCK_OBJECT(SubNodeParent);
				}

				//Remove power and "load" characteristics
				ParToLoad->power[0]-=last_child_power[0][0];
				ParToLoad->power[1]-=last_child_power[0][1];
				ParToLoad->power[2]-=last_child_power[0][2];

				ParToLoad->shunt[0]-=last_child_power[1][0];
				ParToLoad->shunt[1]-=last_child_power[1][1];
				ParToLoad->shunt[2]-=last_child_power[1][2];

				ParToLoad->current[0]-=last_child_power[2][0];
				ParToLoad->current[1]-=last_child_power[2][1];
				ParToLoad->current[2]-=last_child_power[2][2];

				if (has_phase(PHASE_S))	//Triplex slightly different
					ParToLoad->current12-=last_child_current12;

				if (!parentcall)	//Wasn't a parent call - unlock us so our siblings get a shot
				{
					//Unlock the parent now that it is done
					UNLOCK_OBJECT(SubNodeParent);
				}

				//Update previous power tracker - if we haven't really converged, things will mess up without this
				//Power
				last_child_power[0][0] = last_child_power[0][1] = last_child_power[0][2] = 0.0;

				//Shunt
				last_child_power[1][0] = last_child_power[1][1] = last_child_power[1][2] = 0.0;

				//Current
				last_child_power[2][0] = last_child_power[2][1] = last_child_power[2][2] = 0.0;

				//Current 12 if we are triplex
				if (has_phase(PHASE_S))
					last_child_current12 = 0.0;
			}

			if (((SubNode==CHILD) || (SubNode==DIFF_CHILD)) && (NR_cycle==false))	//Child Voltage Updates
			{
				node *ParStealLoad = OBJECTDATA(SubNodeParent,node);

				//Steal our paren't voltages as well
				//this will either be parent called or a child "no way it can change" rank read - no lock needed
				voltage[0] = ParStealLoad->voltage[0];
				voltage[1] = ParStealLoad->voltage[1];
				voltage[2] = ParStealLoad->voltage[2];
			}
		}//End postsynch equivalent pass

		//Handle relvant children first
		if (NR_number_child_nodes[0]>0)	//We have children
		{
			for (table_index=0; table_index<NR_number_child_nodes[0]; table_index++)
			{
				//Call their update - By this call's nature, it is being called by a parent here
				temp_result = NR_child_nodes[table_index]->NR_current_update(postpass,true);

				//Make sure it worked, just to be thorough
				if (temp_result != 1)
				{
					GL_THROW("Attempt to update current on child of node:%s failed!",obj->name);
					/*  TROUBLESHOOT
					While attempting to update the current on a childed node object, an error was encountered.  Please try again.  If the error persists,
					please submit your code and a bug report via the trac website.
					*/
				}
			}//End FOR child table
		}//End we have children

		//Handle our "self" - do this in a "temporary fashion" for children problems
		temp_current_inj[0] = temp_current_inj[1] = temp_current_inj[2] = complex(0.0,0.0);

		if (has_phase(PHASE_D))	//Delta connection
		{
			complex delta_shunt[3];
			complex delta_current[3];

			//Convert delta connected impedance
			delta_shunt[0] = voltageAB*shunt[0];
			delta_shunt[1] = voltageBC*shunt[1];
			delta_shunt[2] = voltageCA*shunt[2];

			//Convert delta connected power
			delta_current[0]= (voltaged[0]==0) ? complex(0,0) : ~(power[0]/voltageAB);
			delta_current[1]= (voltaged[1]==0) ? complex(0,0) : ~(power[1]/voltageBC);
			delta_current[2]= (voltaged[2]==0) ? complex(0,0) : ~(power[2]/voltageCA);

			complex d[] = {
				delta_shunt[0]-delta_shunt[2] + delta_current[0]-delta_current[2] + current[0]-current[2],
				delta_shunt[1]-delta_shunt[0] + delta_current[1]-delta_current[0] + current[1]-current[0],
				delta_shunt[2]-delta_shunt[1] + delta_current[2]-delta_current[1] + current[2]-current[1],
			};

			temp_current_inj[0] = d[0];	
			temp_current_inj[1] = d[1];
			temp_current_inj[2] = d[2];
		}
		else if (has_phase(PHASE_S))	//Split phase node
		{
			complex vdel;
			complex temp_current[3];
			complex temp_store[3];
			complex temp_val[3];

			//Find V12 (just in case)
			vdel=voltage[0] + voltage[1];

			//Find contributions
			//Start with the currents (just put them in)
			temp_current[0] = current[0];
			temp_current[1] = current[1];
			temp_current[2] = current12; //current12 is not part of the standard current array

			//Now add in power contributions
			temp_current[0] += voltage[0] == 0.0 ? 0.0 : ~(power[0]/voltage[0]);
			temp_current[1] += voltage[1] == 0.0 ? 0.0 : ~(power[1]/voltage[1]);
			temp_current[2] += vdel == 0.0 ? 0.0 : ~(power[2]/vdel);

			if (house_present)	//House present
			{
				//Update phase adjustments
				temp_store[0].SetPolar(1.0,voltage[0].Arg());	//Pull phase of V1
				temp_store[1].SetPolar(1.0,voltage[1].Arg());	//Pull phase of V2
				temp_store[2].SetPolar(1.0,vdel.Arg());		//Pull phase of V12

				//Update these current contributions
				temp_val[0] = nom_res_curr[0]/(~temp_store[0]);		//Just denominator conjugated to keep math right (rest was conjugated in house)
				temp_val[1] = nom_res_curr[1]/(~temp_store[1]);
				temp_val[2] = nom_res_curr[2]/(~temp_store[2]);

				//Now add it into the current contributions
				temp_current[0] += temp_val[0];
				temp_current[1] += temp_val[1];
				temp_current[2] += temp_val[2];
			}//End house-attached splitphase

			//Last, but not least, admittance/impedance contributions
			temp_current[0] += shunt[0]*voltage[0];
			temp_current[1] += shunt[1]*voltage[1];
			temp_current[2] += shunt[2]*vdel;

			//Convert 'em to line currents
			complex d[] = {
				temp_current[0] + temp_current[2],
				-temp_current[1] - temp_current[2],
			};

			temp_current_inj[0] = d[0];
			temp_current_inj[1] = d[1];

			//Get information
			if ((Triplex_Data != NULL) && ((Triplex_Data[0] != 0.0) || (Triplex_Data[1] != 0.0)))
			{
				/* normally the calc would not be inside the lock, but it's reflexive so that's ok */
				temp_current_inj[2] = Triplex_Data[0]*temp_current_inj[0] + Triplex_Data[1]*temp_current_inj[1];
			}
			else
			{
				/* normally the calc would not be inside the lock, but it's reflexive so that's ok */
				temp_current_inj[2] = ((voltage1.IsZero() || (power1.IsZero() && shunt1.IsZero())) ||
								   (voltage2.IsZero() || (power2.IsZero() && shunt2.IsZero()))) 
									? currentN : -((temp_current_inj[0]-temp_current[2])+(temp_current_inj[1]-temp_current[2]));
			}
		}
		else					//Wye connection
		{
			complex d[] = {
				//PQP needs power converted to current
				//PQZ needs load currents calculated as well
				//Update load current values if PQI
				((voltage[0]==0) ? complex(0,0) : ~(power[0]/voltage[0])) + voltage[0]*shunt[0] + current[0],
				((voltage[1]==0) ? complex(0,0) : ~(power[1]/voltage[1])) + voltage[1]*shunt[1] + current[1],
				((voltage[2]==0) ? complex(0,0) : ~(power[2]/voltage[2])) + voltage[2]*shunt[2] + current[2],
			};
			temp_current_inj[0] = d[0];			
			temp_current_inj[1] = d[1];
			temp_current_inj[2] = d[2];
		}

		//If we are a child, apply our current injection directly up to our parent - links accumulate afterwards now because they bypass child relationships
		if ((SubNode==CHILD) || (SubNode==DIFF_CHILD))
		{
			node *ParLoadObj=OBJECTDATA(SubNodeParent,node);

			if (ParLoadObj->current_accumulated==false)	//Locking not needed here - if parent hasn't accumulated yet, it is the one that called us (rank split)
			{
				ParLoadObj->current_inj[0] += temp_current_inj[0];	//This ensures link-related injections are not counted
				ParLoadObj->current_inj[1] += temp_current_inj[1];
				ParLoadObj->current_inj[2] += temp_current_inj[2];
			}

			//Update our accumulator as well, otherwise things break
			current_inj[0] += temp_current_inj[0];
			current_inj[1] += temp_current_inj[1];
			current_inj[2] += temp_current_inj[2];
		}
		else	//Not a child, just put this in our accumulator - we're already locked, so no need to separately lock
		{
			current_inj[0] += temp_current_inj[0];
			current_inj[1] += temp_current_inj[1];
			current_inj[2] += temp_current_inj[2];
		}

		//Handle our links - "child" contributions are bypassed into their parents due to NR structure, so this order works
		if ((SubNode!=CHILD) && (SubNode!=DIFF_CHILD))	//Make sure we aren't children as well, since we'll get NULL pointers and make everyone upset
		{
			for (table_index=0; table_index<NR_busdata[NR_node_reference].Link_Table_Size; table_index++)
			{
				//Extract that link
				temp_link = OBJECTDATA(NR_branchdata[NR_busdata[NR_node_reference].Link_Table[table_index]].obj,link_object);

				//Make sure it worked
				if (temp_link == NULL)
				{
					GL_THROW("Attemped to update current for object:%s, which is not a link!",NR_branchdata[NR_busdata[NR_node_reference].Link_Table[table_index]].name);
					/*  TROUBLESHOOT
					The current node object tried to update the current injections for what it thought was a link object.  This does
					not appear to be true.  Try your code again.  If the error persists, please submit your code and a bug report to the
					trac website.
					*/
				}

				//Call a lock on that link - just in case multiple nodes call it at once
				WRITELOCK_OBJECT(NR_branchdata[NR_busdata[NR_node_reference].Link_Table[table_index]].obj);

				//Call its update - tell it who is asking so it knows what to lock
				temp_result = temp_link->CurrentCalculation(NR_node_reference);

				//Unlock the link
				WRITEUNLOCK_OBJECT(NR_branchdata[NR_busdata[NR_node_reference].Link_Table[table_index]].obj);

				//See if it worked, just in case this gets added in the future
				if (temp_result != 1)
				{
					GL_THROW("Attempt to update current on link:%s failed!",NR_branchdata[NR_busdata[NR_node_reference].Link_Table[table_index]].name);
					/*  TROUBLESHOOT
					While attempting to update the current on link object, an error was encountered.  Please try again.  If the error persists,
					please submit your code and a bug report via the trac website.
					*/
				}
			}//End link traversion
		}//End not children

		//Flag us as done
		current_accumulated = true;
	}//End current already handled

	return 1;	//Always successful
}


EXPORT int isa_node(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,node)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT int notify_node(OBJECT *obj, int update_mode, PROPERTY *prop, char *value){
	node *n = OBJECTDATA(obj, node);
	int rv = 1;
	
	rv = n->notify(update_mode, prop, value);
	
	return rv;
}
/**@}*/
