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
		oclass = gl_register_class(mod,"node",sizeof(node),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if (oclass==NULL)
			throw "unable to register class node";
		else
			oclass->trl = TRL_PROVEN;

		if(gl_publish_variable(oclass,
			PT_INHERIT, "powerflow_object",
			PT_enumeration, "bustype", PADDR(bustype),
				PT_KEYWORD, "PQ", PQ,
				PT_KEYWORD, "PV", PV,
				PT_KEYWORD, "SWING", SWING,
			PT_set, "busflags", PADDR(busflags),
				PT_KEYWORD, "HASSOURCE", (set)NF_HASSOURCE,
			PT_object, "reference_bus", PADDR(reference_bus),
			PT_double,"maximum_voltage_error[V]",PADDR(maximum_voltage_error),

			PT_complex, "voltage_A[V]", PADDR(voltageA),
			PT_complex, "voltage_B[V]", PADDR(voltageB),
			PT_complex, "voltage_C[V]", PADDR(voltageC),
			PT_complex, "voltage_AB[V]", PADDR(voltageAB),
			PT_complex, "voltage_BC[V]", PADDR(voltageBC),
			PT_complex, "voltage_CA[V]", PADDR(voltageCA),
			PT_complex, "current_A[A]", PADDR(currentA),
			PT_complex, "current_B[A]", PADDR(currentB),
			PT_complex, "current_C[A]", PADDR(currentC),
			PT_complex, "power_A[VA]", PADDR(powerA),
			PT_complex, "power_B[VA]", PADDR(powerB),
			PT_complex, "power_C[VA]", PADDR(powerC),
			PT_complex, "shunt_A[S]", PADDR(shuntA),
			PT_complex, "shunt_B[S]", PADDR(shuntB),
			PT_complex, "shunt_C[S]", PADDR(shuntC),
			PT_bool, "NR_mode", PADDR(NR_mode),
			PT_double, "mean_repair_time[s]",PADDR(mean_repair_time), PT_DESCRIPTION, "Time after a fault clears for the object to be back in service",

			PT_enumeration, "service_status", PADDR(service_status),
				PT_KEYWORD, "IN_SERVICE", ND_IN_SERVICE,
				PT_KEYWORD, "OUT_OF_SERVICE", ND_OUT_OF_SERVICE,

			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    
			//Defaults setup
			nodelinks.next=NULL;
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
	NR_subnode_reference = NULL;
	Extra_Data=NULL;
	NR_link_table = NULL;
	NR_connected_links[0] = NR_connected_links[1] = 0;
	YVs[0] = YVs[1] = YVs[2] = 0.0;

	GS_converged=false;

	NR_node_reference = -1;	//Newton-Raphson bus index, set to -1 initially
	house_present = false;	//House attachment flag
	nom_res_curr[0] = nom_res_curr[1] = nom_res_curr[2] = 0.0;	//Nominal house current variables

	prev_phases = 0x00;

	mean_repair_time = 0.0;

	// Only used in capacitors, at this time, but put into node for future functionality (maybe with reliability?)
	service_status = ND_IN_SERVICE;

	memset(voltage,0,sizeof(voltage));
	memset(voltaged,0,sizeof(voltaged));
	memset(current,0,sizeof(current));
	memset(power,0,sizeof(power));
	memset(shunt,0,sizeof(shunt));

	return result;
}

int node::init(OBJECT *parent)
{
	if (solver_method==SM_NR)
	{
		OBJECT *obj = OBJECTHDR(this);
		char1024 ext_lib_file_name;
		char extpath[1024];
		CALLBACKS **cbackval = NULL;
		bool ExtLinkFailure;

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
				connection.  If this is undesired, remove the parenting on the swing node.
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
				if ((phases & PHASE_D) && ((parNode->phases & (PHASE_A|PHASE_B|PHASE_C)) != (PHASE_A|PHASE_B|PHASE_C)))	//We're a delta
				{
					GL_THROW("NR: Parent and child node phases for nodes %s and %s do not match!",obj->parent->name,obj->name);
					/*	TROUBLESHOOT
					The implementation of parent-child connections in Newton-Raphson requires the child
					object have the same phases as the parent.  Match the phases appropriately.  For a delta-connected
					child, the parent must be delta connected or contain all three "normal" phases.
					*/
				}
				else if ((p_phase_to_check & c_phase_to_check) != c_phase_to_check)	//Our parent is lacking, fail
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
						//Set appropriate flags (store parent name and flag self & parent)
						SubNode = DIFF_CHILD;
						SubNodeParent = obj->parent;
						
						parNode->SubNode = DIFF_PARENT;
						parNode->SubNodeParent = obj;	//This may get overwritten if we have multiple children, so try not to use it anywhere mission critical.

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
				snprintf(ext_lib_file_name, 1024, "solver_%s" DLEXT,LUSolverName);
#else
				snprintf(ext_lib_file_name, 1024, "lib_solver_%s" DLEXT,LUSolverName);
#endif

				if (gl_findfile(ext_lib_file_name, NULL, 0|4, extpath,sizeof(extpath))!=NULL)	//Link up
				{
					//Link to the library
					LUSolverFcns.dllLink = DLLOAD(extpath);

					//Make sure it worked
					if (LUSolverFcns.dllLink==NULL)
					{
						gl_warning("Failure to load solver_%s as a library, defaulting to superLU",LUSolverName);
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
							gl_warning("LU_init of external solver solver_%s not found, defaulting to superLU",LUSolverName);
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
						if (LUSolverFcns.ext_init == NULL)
						{
							gl_warning("LU_init of external solver solver_%s not found, defaulting to superLU",LUSolverName);
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
						if (LUSolverFcns.ext_init == NULL)
						{
							gl_warning("LU_init of external solver solver_%s not found, defaulting to superLU",LUSolverName);
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
						if (LUSolverFcns.ext_init == NULL)
						{
							gl_warning("LU_init of external solver solver_%s not found, defaulting to superLU",LUSolverName);
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
							gl_verbose("External solver solver_%s found, utilizing for NR",LUSolverName);
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
					gl_warning("The external solver solver_%s could not be found, defaulting to superLU",LUSolverName);
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
		OBJECT *obj = OBJECTHDR(this);

		FINDLIST *buslist = gl_find_objects(FL_NEW,FT_CLASS,SAME,"node",AND,FT_PROPERTY,"bustype",SAME,"SWING",FT_END);
		if (buslist==NULL)
			throw "GS: no swing bus found";
			/*	TROUBLESHOOT
			No swing bus was located in the test system.  Gauss-Seidel requires at least one node
			be designated BUSTYPE SWING.
			*/
		if (buslist->hit_count>1)
			gl_warning("GS: more than one swing bus found, you must specify which is this node's parent");
			/*	TROUBLESHOOT
			More than one swing bus was found.  Gauss-Seidel requires you specify the swing
			bus this node is connected to via the parent property.
			*/

		OBJECT *SwingBusObj = gl_find_next(buslist,NULL);

		// check that a parent is defined
		if ((obj->parent!=NULL) && (OBJECTDATA(obj->parent,node)->bustype!=SWING))	//Has a parent that isn't a swing bus, let's see if it is a node and link it up 
		{																			//(this will break anything intentionally done this way - e.g. switch between two nodes)
			gl_warning("Parent/child implementation marginally tested, use at your own risk!");
			/*  TROUBLESHOOT
			The Gauss-Seidel parent-child connection type is only marginally tested.  It has not been fully tested and may cause
			unexpected problems in your model.  Use at your own risk.
			*/

			//See if it is a node/load/meter
			if (!(gl_object_isa(obj->parent,"load","powerflow") | gl_object_isa(obj->parent,"node","powerflow") | gl_object_isa(obj->parent,"meter","powerflow")))
				throw("GS: Parent is not a load or node!");
				/*  TROUBLESHOOT
				A Gauss-Seidel parent-child connection was attempted on a non-node.  The parent object must be a node, load, or meter object in the 
				powerflow module for this connection to be successful.
				*/

			node *parNode = OBJECTDATA(obj->parent,node);

			//Make sure our phases align, otherwise become angry
			if (parNode->phases!=phases)
				throw("GS: Parent and child node phases do not match!");
				/*	TROUBLESHOOT
				The implementation of parent-child connections in Gauss-Seidel requires the child
				object have the same phases as the parent.  Match the phases appropriately.
				*/

			if ((parNode->SubNode==CHILD_NOINIT) | ((obj->parent->parent!=SwingBusObj) && (obj->parent->parent!=NULL)))	//Our parent is another child
			{
				//Set appropriate flags (store parent name and flag self & parent)
				SubNode = CHILD_NOINIT;
				if (parNode->SubNode==CHILD_NOINIT)	//already parsed child object
					SubNodeParent = parNode->SubNodeParent;
				else	//Parented object that will be parsed soon, but hasn't yet
					SubNodeParent = obj->parent->parent;
			}
			else	//Our parent is unchilded
			{
				//Set appropriate flags (store parent name and flag self & parent)
				SubNode = CHILD_NOINIT;
				SubNodeParent = obj->parent;
				
				parNode->SubNode = PARENT;
				parNode->SubNodeParent = obj;
			}

			//Give us no parent now, and let it go to SWING (otherwise generic check fails)
			obj->parent=NULL;

			//Zero out last child power vector (used for updates)
			last_child_power[0][0] = last_child_power[0][1] = last_child_power[0][2] = complex(0,0);
			last_child_power[1][0] = last_child_power[1][1] = last_child_power[1][2] = complex(0,0);
			last_child_power[2][0] = last_child_power[2][1] = last_child_power[2][2] = complex(0,0);
			last_child_current12 = 0.0;

		} 
		
		if ((obj->parent==NULL) && (bustype!=SWING)) // need to find a swing bus to be this node's parent
		{
			gl_set_parent(obj,SwingBusObj);
		}

		/* Make sure we aren't the swing bus */
		if (bustype!=SWING)
		{
			/* still no swing bus found */
			if (obj->parent==NULL)
				throw "GS: no swing bus found or specified";
				/*	TROUBLESHOOT
				Gauss-Seidel failed to automatically assign a swing bus to the node.  This should
				have been detected by this point and represents a bug in the solver.  Please submit
				a bug report detailing how you obtained this message.
				*/

			/* check that the parent is a node and that it's a swing bus */
			if (!gl_object_isa(obj->parent,"node"))
				throw "GS: node parent is not a node itself";
				/*	TROUBLESHOOT
				A node-based object in the Gauss-Seidel solver has a non-node parent set.  The parent
				must be set to either a parent node in a parent-child connection or a swing bus of the
				system.
				*/

			else 
			{
				node *pNode = OBJECTDATA(obj->parent,node);

				if (pNode->bustype!=SWING)	//Originally checks to see if is swing bus...not sure how to handle this yet
					throw "GS: node parent is not a swing bus";
					/*	TROUBLESHOOT
					A node-based object has an invalid parent specified.  Unless in a parent-child connect, only swing
					buses can be specified as a parent object in the Gauss-Seidel solver.
					*/
			}
		}

		//Zero out admittance matrix - get ready for next passes
		Ys[0][0] = Ys[0][1] = Ys[0][2] = complex(0,0);
		Ys[1][0] = Ys[1][1] = Ys[1][2] = complex(0,0);
		Ys[2][0] = Ys[2][1] = Ys[2][2] = complex(0,0);

		//Zero out current accummulator
		YVs[0] = complex(0,0);
		YVs[1] = complex(0,0);
		YVs[2] = complex(0,0);
	}
	else if (solver_method == SM_FBS)	//Forward back sweep
	{
		OBJECT *obj = OBJECTHDR(this);

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
		OBJECT *obje = OBJECTHDR(this);

		if (bustype==SWING)
		{
			gl_set_rank(obje,5);
		}
		else if (SubNode!=CHILD_NOINIT)
		{
			gl_set_rank(obje,1);
		}
		else	//We are a child, put us right above links (rank 1)
		{		//This puts us to execute before all nodes/loads in sync, but still before links in postsync
			gl_set_rank(obje,1);
		}
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

	//Pre-zero non existant phases
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
	else if ((has_phase(PHASE_A|PHASE_B|PHASE_C)) || (has_phase(PHASE_D)))	//three phase or delta
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
		else
			voltage[0].SetPolar(nominal_voltage,0);

		if (!has_phase(PHASE_B))
			voltage[1]=0.0;
		else
			voltage[2].SetPolar(nominal_voltage,-2*PI/3);

		if (!has_phase(PHASE_C))
			voltage[2]=0.0;
		else
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

	return result;
}

TIMESTAMP node::presync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t1 = powerflow_object::presync(t0); 

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
			be sure no open switches are the sole connection for a phase, else this will fail as well.
			*/
		}
	}

	if (solver_method==SM_NR)
	{
		//Zero the accumulators for later (meters and such)
		current_inj[0] = current_inj[1] = current_inj[2] = 0.0;

		//If we're the swing, toggle tracking variable
		if (bustype==SWING)
			NR_cycle = !NR_cycle;

		if (prev_NTime==0)	//First run, if we are a child, make sure no one linked us before we knew that
		{
			if (((SubNode == CHILD) || (SubNode == DIFF_CHILD)) && (NR_connected_links>0))
			{
				node *parNode = OBJECTDATA(SubNodeParent,node);

				LOCK_OBJECT(SubNodeParent);	//Lock

				parNode->NR_connected_links[0] += NR_connected_links[0];

				//Check and see if we're a house-triplex.  If so, flag our parent so NR works
				if (house_present==true)
				{
					parNode->house_present=true;
				}

				UNLOCK_OBJECT(SubNodeParent);	//Unlock
			}
		}

		if (NR_busdata==NULL || NR_branchdata==NULL)	//First time any NR in (this should be the swing bus doing this)
		{
			LOCK_OBJECT(NR_swing_bus);	//Lock Swing for flag

			NR_busdata = (BUSDATA *)gl_malloc(NR_bus_count * sizeof(BUSDATA));
			if (NR_busdata==NULL)
			{
				gl_error("NR: memory allocation failure for bus table");
				/*	TROUBLESHOOT
				This is a bug.  GridLAB-D failed to allocate memory for the bus table for Newton-Raphson.
				Please submit this bug and your code.
				*/
				return 0;
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
				return 0;
			}
			NR_curr_branch = 0;	//Pull pointer off flag so other objects know it's built

			//Unlock the swing bus
			UNLOCK_OBJECT(NR_swing_bus);

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
		}

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
	}
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
	else if ((solver_method==SM_GS) && (prev_NTime!=t0))
	{
		//Zero out admittance and YVs terms of new cycle.  Rank should take care of all problems here. 
		YVs[0] = YVs[1] = YVs[2] = 0.0;
		
		Ys[0][0] = Ys[0][1] = Ys[0][2] = 0.0;
		Ys[1][0] = Ys[1][1] = Ys[1][2] = 0.0;
		Ys[2][0] = Ys[2][1] = Ys[2][2] = 0.0;
	}
	
	return t1;
}

TIMESTAMP node::sync(TIMESTAMP t0)
{
	TIMESTAMP t1 = powerflow_object::sync(t0);
	OBJECT *obj = OBJECTHDR(this);
	
	//Generic time keeping variable - used for phase checks (GS does this explicitly below)
	if ((t0!=prev_NTime) && (solver_method !=SM_GS))
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
			current_inj[0] += (voltage1.IsZero() || (power1.IsZero() && shunt1.IsZero())) ? (current1 + temp_curr_val[0]) : (current1 + ~(power1/voltage1) + voltage1*shunt1 + temp_curr_val[0]);
			temp_inj[0] = current_inj[0];
			current_inj[0] += ((voltage1+voltage2).IsZero() || (power12.IsZero() && shunt12.IsZero())) ? (current12 + temp_curr_val[2]) : (current12 + ~(power12/(voltage1+voltage2)) + (voltage1+voltage2)*shunt12 + temp_curr_val[2]);

#ifdef SUPPORT_OUTAGES
			}
			else
			{
				temp_inj[0] = 0.0;
				current_inj[0]=0.0;
			}

			if (voltage[1]!=0)
			{
#endif

			current_inj[1] += (voltage2.IsZero() || (power2.IsZero() && shunt2.IsZero())) ? (-current2 - temp_curr_val[1]) : (-current2 - ~(power2/voltage2) - voltage2*shunt2 - temp_curr_val[1]);
			temp_inj[1] = current_inj[1];
			current_inj[1] += ((voltage1+voltage2).IsZero() || (power12.IsZero() && shunt12.IsZero())) ? (-current12 - temp_curr_val[2]) : (-current12 - ~(power12/(voltage1+voltage2)) - (voltage1+voltage2)*shunt12 - temp_curr_val[2]);
			
#ifdef SUPPORT_OUTAGES
			}
			else
			{
				temp_inj[0] = 0.0;
				current_inj[1] = 0.0;
			}
#endif

			if (obj->parent!=NULL && gl_object_isa(obj->parent,"triplex_line","powerflow")) {
				link *plink = OBJECTDATA(obj->parent,link);
				current_inj[2] += plink->tn[0]*current_inj[0] + plink->tn[1]*current_inj[1];
			}
			else {
				current_inj[2] += ((voltage1.IsZero() || (power1.IsZero() && shunt1.IsZero())) ||
								   (voltage2.IsZero() || (power2.IsZero() && shunt2.IsZero()))) 
									? currentN : -(temp_inj[0] + temp_inj[1]);
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
					current_inj[kphase] = 0.0;
				}
				else
				{
					current_inj[kphase] += delta_current[kphase] + power_current[kphase] + delta_shunt_curr[kphase];
				}
			}
#else
			current_inj[0] += delta_current[0] + power_current[0] + delta_shunt_curr[0];
			current_inj[1] += delta_current[1] + power_current[1] + delta_shunt_curr[1];
			current_inj[2] += delta_current[2] + power_current[2] + delta_shunt_curr[2];
#endif
		}
		else 
		{	// 'WYE' connected load

#ifdef SUPPORT_OUTAGES
			for (char kphase=0;kphase<3;kphase++)
			{
				if (voltage[kphase]==0.0)
				{
					current_inj[kphase] = 0.0;
				}
				else
				{
					current_inj[kphase] += ((voltage[kphase]==0.0) || ((power[kphase] == 0) && shunt[kphase].IsZero())) ? current[kphase] : current[kphase] + ~(power[kphase]/voltage[kphase]) + voltage[kphase]*shunt[kphase];
				}
			}
#else
			current_inj[0] += (voltageA.IsZero() || (powerA.IsZero() && shuntA.IsZero())) ? currentA : currentA + ~(powerA/voltageA) + voltageA*shuntA;
			current_inj[1] += (voltageB.IsZero() || (powerB.IsZero() && shuntB.IsZero())) ? currentB : currentB + ~(powerB/voltageB) + voltageB*shuntB;
			current_inj[2] += (voltageC.IsZero() || (powerC.IsZero() && shuntC.IsZero())) ? currentC : currentC + ~(powerC/voltageC) + voltageC*shuntC;
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
				LOCKED(obj->parent, pNode->current_inj[0] += current_inj[0]);
				LOCKED(obj->parent, pNode->current_inj[1] += current_inj[1]);
				LOCKED(obj->parent, pNode->current_inj[2] += current_inj[2]);
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
	case SM_GS:
		{
		//node *swing = hdr->parent?OBJECTDATA(hdr->parent,node):this;
		complex Vtemp[3];
		complex YVsTemp[3];
		complex Vnew[3];
		complex dV[3];
		LINKCONNECTED *linktable=NULL;
		OBJECT *linkupdate;
		link *linkref;

		//Run parent/child update items
		if ((SubNode==CHILD_NOINIT) || (SubNode==CHILD))
		{
			GS_P_C_NodeChecks(t0, t1, obj, linktable);
		}

		//House keeping items - if time step has changed update variable and reset our convergence flag
		if (t0!=prev_NTime)
		{
				//Update time tracking variable
				prev_NTime=t0;

				//Reset convergence flag
				GS_converged=false;
		}

		//Pull voltages and admittance into temporary variable
		LOCK_OBJECT(obj);
		Vtemp[0]=voltage[0];
		Vtemp[1]=voltage[1];
		Vtemp[2]=voltage[2];
		YVsTemp[0] = YVs[0];
		YVsTemp[1] = YVs[1];
		YVsTemp[2] = YVs[2];
		UNLOCK_OBJECT(obj);

		//init dV as zero
		dV[0] = dV[1] = dV[2] = complex(0,0);

		if (SubNode!=CHILD) //If is a child node, don't do any updates (waste of computations)
		{
			switch (bustype) {
				case PV:	//PV bus only supports Y-Y connections at this time.  Easier to handle the oddity of power that way.
					{
					//Check for NaNs (single phase problem)
					if ((!has_phase(PHASE_A|PHASE_B|PHASE_C)) && (!has_phase(PHASE_D)))	//Not three phase or delta
					{
						//Calculate reactive power - apply GS update (do phase iteratively)
						power[0].Im() = ((~Vtemp[0]*(Ys[0][0]*Vtemp[0]+Ys[0][1]*Vtemp[1]+Ys[0][2]*Vtemp[2]-YVsTemp[0]+current[0]+Vtemp[0]*shunt[0])).Im());
						Vnew[0] = (-(~power[0]/~Vtemp[0]+current[0]+Vtemp[0]*shunt[0])+YVsTemp[0]-Vtemp[1]*Ys[0][1]-Vtemp[2]*Ys[0][2])/Ys[0][0];
						Vnew[0] = (isnan(Vnew[0].Re())) ? complex(0,0) : Vnew[0];

						//Apply acceleration to Vnew
						Vnew[0]=Vtemp[0]+(Vnew[0]-Vtemp[0])*acceleration_factor;

						power[1].Im() = ((~Vtemp[1]*(Ys[1][0]*Vnew[0]+Ys[1][1]*Vtemp[1]+Ys[1][2]*Vtemp[2]-YVsTemp[1]+current[1]+Vtemp[1]*shunt[1])).Im());
						Vnew[1] = (-(~power[1]/~Vtemp[1]+current[1]+Vtemp[1]*shunt[1])+YVsTemp[1]-Vnew[0]*Ys[1][0]-Vtemp[2]*Ys[1][2])/Ys[1][1];
						Vnew[1] = (isnan(Vnew[1].Re())) ? complex(0,0) : Vnew[1];

						//Apply acceleration to Vnew
						Vnew[1]=Vtemp[1]+(Vnew[1]-Vtemp[1])*acceleration_factor;

						power[2].Im() = ((~Vtemp[2]*(Ys[2][0]*Vnew[0]+Ys[2][1]*Vnew[1]+Ys[2][2]*Vtemp[2]-YVsTemp[2]+current[2]+Vtemp[2]*shunt[2])).Im());
						Vnew[2] = (-(~power[2]/~Vtemp[2]+current[2]+Vtemp[2]*shunt[2])+YVsTemp[2]-Vnew[0]*Ys[2][0]-Vnew[1]*Ys[2][1])/Ys[2][2];
						Vnew[2] = (isnan(Vnew[2].Re())) ? complex(0,0) : Vnew[2];

						//Apply acceleration to Vnew
						Vnew[2]=Vtemp[2]+(Vnew[2]-Vtemp[2])*acceleration_factor;
					}
					else	//Three phase
					{
						//Calculate reactive power - apply GS update (do phase iteratively)
						power[0].Im() = ((~Vtemp[0]*(Ys[0][0]*Vtemp[0]+Ys[0][1]*Vtemp[1]+Ys[0][2]*Vtemp[2]-YVsTemp[0]+current[0]+Vtemp[0]*shunt[0])).Im());
						Vnew[0] = (-(~power[0]/~Vtemp[0]+current[0]+Vtemp[0]*shunt[0])+YVsTemp[0]-Vtemp[1]*Ys[0][1]-Vtemp[2]*Ys[0][2])/Ys[0][0];

						//Apply acceleration to Vnew
						Vnew[0]=Vtemp[0]+(Vnew[0]-Vtemp[0])*acceleration_factor;

						power[1].Im() = ((~Vtemp[1]*(Ys[1][0]*Vnew[0]+Ys[1][1]*Vtemp[1]+Ys[1][2]*Vtemp[2]-YVsTemp[1]+current[1]+Vtemp[1]*shunt[1])).Im());
						Vnew[1] = (-(~power[1]/~Vtemp[1]+current[1]+Vtemp[1]*shunt[1])+YVsTemp[1]-Vnew[0]*Ys[1][0]-Vtemp[2]*Ys[1][2])/Ys[1][1];

						//Apply acceleration to Vnew
						Vnew[1]=Vtemp[1]+(Vnew[1]-Vtemp[1])*acceleration_factor;

						power[2].Im() = ((~Vtemp[2]*(Ys[2][0]*Vnew[0]+Ys[2][1]*Vnew[1]+Ys[2][2]*Vtemp[2]-YVsTemp[2]+current[2]+Vtemp[2]*shunt[2])).Im());
						Vnew[2] = (-(~power[2]/~Vtemp[2]+current[2]+Vtemp[2]*shunt[2])+YVsTemp[2]-Vnew[0]*Ys[2][0]-Vnew[1]*Ys[2][1])/Ys[2][2];

						//Apply acceleration to Vnew
						Vnew[2]=Vtemp[2]+(Vnew[2]-Vtemp[2])*acceleration_factor;
					}

					//Apply correction - only use angles (magnitude is unaffected)
					Vnew[0].SetPolar(Vtemp[0].Mag(),Vnew[0].Arg());
					Vnew[1].SetPolar(Vtemp[1].Mag(),Vnew[1].Arg());
					Vnew[2].SetPolar(Vtemp[2].Mag(),Vnew[2].Arg());

					//Find step amount for convergence check
					dV[0]=Vnew[0]-Vtemp[0];
					dV[1]=Vnew[1]-Vtemp[1];
					dV[2]=Vnew[2]-Vtemp[2];

					//Apply update
					Vtemp[0]=Vnew[0];
					Vtemp[1]=Vnew[1];
					Vtemp[2]=Vnew[2];

					break;
					}
				case PQ:
					{
					if (has_phase(PHASE_S)) //Split phase
					{
						//Update node Vtemp
						Vnew[0] = (-(~power[0]/~Vtemp[0]+current[0]+Vtemp[0]*shunt[0])+YVsTemp[0]-Vtemp[1]*Ys[0][1]-Vtemp[2]*Ys[0][2])/Ys[0][0];

						//Apply acceleration to Vnew
						Vnew[0]=Vtemp[0]+(Vnew[0]-Vtemp[0])*acceleration_factor;

						Vnew[1] = (-(~power[1]/~Vtemp[1]+current[1]+Vtemp[1]*shunt[1])+YVsTemp[1]-Vnew[0]*Ys[1][0]-Vtemp[2]*Ys[1][2])/Ys[1][1];

						//Apply acceleration to Vnew
						Vnew[1]=Vtemp[1]+(Vnew[1]-Vtemp[1])*acceleration_factor;

						//Vnew[2] = (-(~power[2]/~Vtemp[2]+current[2]+Vtemp[2]*shunt[2])+YVsTemp[2]-Vnew[0]*Ys[2][0]-Vnew[1]*Ys[2][1])/Ys[2][2];

						//Apply acceleration to Vnew
						//Vnew[2]=Vtemp[2]+(Vnew[2]-Vtemp[2])*acceleration_factor;
						Vnew[2] = Vtemp[2];
					}
					else if ((!has_phase(PHASE_A|PHASE_B|PHASE_C)) && (!has_phase(PHASE_D)))  //Not three phase or delta
					{
						//Update node Vtemp
						Vnew[0] = (-(~power[0]/~Vtemp[0]+current[0]+Vtemp[0]*shunt[0])+YVsTemp[0]-Vtemp[1]*Ys[0][1]-Vtemp[2]*Ys[0][2])/Ys[0][0];
						Vnew[0] = (isnan(Vnew[0].Re())) ? complex(0,0) : Vnew[0];

						//Apply acceleration to Vnew
						Vnew[0]=Vtemp[0]+(Vnew[0]-Vtemp[0])*acceleration_factor;

						Vnew[1] = (-(~power[1]/~Vtemp[1]+current[1]+Vtemp[1]*shunt[1])+YVsTemp[1]-Vnew[0]*Ys[1][0]-Vtemp[2]*Ys[1][2])/Ys[1][1];
						Vnew[1] = (isnan(Vnew[1].Re())) ? complex(0,0) : Vnew[1];

						//Apply acceleration to Vnew
						Vnew[1]=Vtemp[1]+(Vnew[1]-Vtemp[1])*acceleration_factor;

						Vnew[2] = (-(~power[2]/~Vtemp[2]+current[1]+Vtemp[2]*shunt[2])+YVsTemp[2]-Vnew[0]*Ys[2][0]-Vnew[1]*Ys[2][1])/Ys[2][2];
						Vnew[2] = (isnan(Vnew[2].Re())) ? complex(0,0) : Vnew[2];

						//Apply acceleration to Vnew
						Vnew[2]=Vtemp[2]+(Vnew[2]-Vtemp[2])*acceleration_factor;
					}
					else	//Three phase
					{
						if (has_phase(PHASE_D))	//Delta connected
						{
							complex delta_current[3];
							complex power_current[3];
							complex delta_shunt[3];
							complex delta_shunt_curr[3];

							//Convert delta connected power load
							delta_current[0]= (voltageAB.IsZero()) ? 0 : ~(powerA/voltageAB);
							delta_current[1]= (voltageBC.IsZero()) ? 0 : ~(powerB/voltageBC);
							delta_current[2]= (voltageCA.IsZero()) ? 0 : ~(powerC/voltageCA);

							power_current[0]=delta_current[0]-delta_current[2];
							power_current[1]=delta_current[1]-delta_current[0];
							power_current[2]=delta_current[2]-delta_current[1];

							//Convert delta connected impedance
							delta_shunt[0] = voltageAB*shuntA;
							delta_shunt[1] = voltageBC*shuntB;
							delta_shunt[2] = voltageCA*shuntC;

							delta_shunt_curr[0] = delta_shunt[0]-delta_shunt[2];
							delta_shunt_curr[1] = delta_shunt[1]-delta_shunt[0];
							delta_shunt_curr[2] = delta_shunt[2]-delta_shunt[1];

							//Convert delta connected current
							delta_current[0]=current[0]-current[2];
							delta_current[1]=current[1]-current[0];
							delta_current[2]=current[2]-current[1];

							//Update node Vtemp
							Vnew[0] = (-(power_current[0]+delta_current[0]+delta_shunt_curr[0])+YVsTemp[0]-Vtemp[1]*Ys[0][1]-Vtemp[2]*Ys[0][2])/Ys[0][0];

							//Apply acceleration to Vnew
							Vnew[0]=Vtemp[0]+(Vnew[0]-Vtemp[0])*acceleration_factor;

							Vnew[1] = (-(power_current[1]+delta_current[1]+delta_shunt_curr[1])+YVsTemp[1]-Vnew[0]*Ys[1][0]-Vtemp[2]*Ys[1][2])/Ys[1][1];

							//Apply acceleration to Vnew
							Vnew[1]=Vtemp[1]+(Vnew[1]-Vtemp[1])*acceleration_factor;
							
							Vnew[2] = (-(power_current[2]+delta_current[2]+delta_shunt_curr[2])+YVsTemp[2]-Vnew[0]*Ys[2][0]-Vnew[1]*Ys[2][1])/Ys[2][2];

							//Apply acceleration to Vnew
							Vnew[2]=Vtemp[2]+(Vnew[2]-Vtemp[2])*acceleration_factor;
						}
						else
						{
							//Update node Vtemp
							Vnew[0] = (-(~power[0]/~Vtemp[0]+current[0]+Vtemp[0]*shunt[0])+YVsTemp[0]-Vtemp[1]*Ys[0][1]-Vtemp[2]*Ys[0][2])/Ys[0][0];

							//Apply acceleration to Vnew
							Vnew[0]=Vtemp[0]+(Vnew[0]-Vtemp[0])*acceleration_factor;

							Vnew[1] = (-(~power[1]/~Vtemp[1]+current[1]+Vtemp[1]*shunt[1])+YVsTemp[1]-Vnew[0]*Ys[1][0]-Vtemp[2]*Ys[1][2])/Ys[1][1];

							//Apply acceleration to Vnew
							Vnew[1]=Vtemp[1]+(Vnew[1]-Vtemp[1])*acceleration_factor;

							Vnew[2] = (-(~power[2]/~Vtemp[2]+current[2]+Vtemp[2]*shunt[2])+YVsTemp[2]-Vnew[0]*Ys[2][0]-Vnew[1]*Ys[2][1])/Ys[2][2];

							//Apply acceleration to Vnew
							Vnew[2]=Vtemp[2]+(Vnew[2]-Vtemp[2])*acceleration_factor;
						}
					}

					//Find step amount for convergence check
					dV[0]=Vnew[0]-Vtemp[0];
					dV[1]=Vnew[1]-Vtemp[1];
					dV[2]=Vnew[2]-Vtemp[2];

					//Apply update
					Vtemp[0]=Vnew[0];
					Vtemp[1]=Vnew[1];
					Vtemp[2]=Vnew[2];

					break;
					}
				case SWING: 						//Nothing to do here :(
					{
					break;
					}
				default:
					{
					/* unknown type fails */
					gl_error("invalid bus type");
					/*	TROUBLESHOOT
					This is an error.  Ensure you have no specified an invalid bustype on nodes in the
					system.  Valid types are SWING, PQ, and PV.  Report this error as a bug if no invalid
					types are found.
					*/

					return TS_ZERO;
					}
			}

			//Update YVsTemp terms for connected nodes - exclude swing bus (or if nothing changed) or if child object
			if ((dV[0].Mag()!=0) | (dV[1].Mag()!=0) | (dV[2].Mag()!=0))
			{
				linktable = &nodelinks;
				OBJECT *datanode;
				char tempdir = 0;

				while (linktable->next!=NULL)		//Parse through the linked list
				{
					linktable = linktable->next;

					linkupdate = linktable->connectedlink;
					linkref = OBJECTDATA(linkupdate,link);

					if (obj==linktable->fnodeconnected)
					{
						tempdir = 1;	//We are the from node
						datanode = (linktable->tnodeconnected);
						linkref->UpdateYVs(datanode, tempdir, dV);	//Call link YVsTemp updating function as a from
					}
					else if (obj==linktable->tnodeconnected)
					{
						tempdir = 2;	//We are the to node
						datanode = (linktable->fnodeconnected);
						linkref->UpdateYVs(datanode, tempdir, dV);	//Call link YVsTemp updating function as a from
					}
				}
			}
		}

		//Update voltages and admittances back into proper place
		LOCK_OBJECT(obj);
		voltage[0] = Vtemp[0];
		voltage[1] = Vtemp[1];
		voltage[2] = Vtemp[2];
		//YVs[0] = YVsTemp[0];
		//YVs[1] = YVsTemp[1];
		//YVs[2] = YVsTemp[2];
		UNLOCK_OBJECT(obj);

		//See if we've converged
		if ((dV[0].Mag()>maximum_voltage_error) | (dV[1].Mag()>maximum_voltage_error) | (dV[2].Mag()>maximum_voltage_error))
		{
			GS_all_converged=false;			//Set to false
			GS_converged=false;				//Reflag us, in case we were converged and suddenly aren't
			return t0;	//No convergence, loop again
		}
		else	//We're done, let's check to make sure everyone else is
		{
			GS_converged=true;	//Flag us
			GS_all_converged=true;	//Flag global.  If we are last and it wasn't supposed to go off, will be caught in Post-Sync
			return t1;	//converged, no more loops needed
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

					current_inj[0] += delta_shunt[0]-delta_shunt[2];	//calculate "load" current (line current) - PQZ
					current_inj[1] += delta_shunt[1]-delta_shunt[0];
					current_inj[2] += delta_shunt[2]-delta_shunt[1];

					current_inj[0] +=delta_current[0]-delta_current[2];	//Calculate "load" current (line current) - PQP
					current_inj[1] +=delta_current[1]-delta_current[0];
					current_inj[2] +=delta_current[2]-delta_current[1];

					current_inj[0] +=current[0]-current[2];	//Calculate "load" current (line current) - PQI
					current_inj[1] +=current[1]-current[0];
					current_inj[2] +=current[2]-current[1];
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
					current_inj[0] += (temp_current[0] + temp_current[2]);
					current_inj[1] += (-temp_current[1] - temp_current[2]);

					//Get information
					if ((Triplex_Data != NULL) && ((Triplex_Data[0] != 0.0) || (Triplex_Data[1] != 0.0)))
					{
						current_inj[2] += Triplex_Data[0]*current_inj[0] + Triplex_Data[1]*current_inj[1];
					}
					else
					{
						current_inj[2] += ((voltage1.IsZero() || (power1.IsZero() && shunt1.IsZero())) ||
										   (voltage2.IsZero() || (power2.IsZero() && shunt2.IsZero()))) 
											? currentN : -((current_inj[0]-temp_current[2])+(current_inj[1]-temp_current[2]));
					}
				}
				else					//Wye connection
				{
					current_inj[0] += (voltage[0]==0) ? complex(0,0) : ~(power[0]/voltage[0]);			//PQP needs power converted to current
					current_inj[1] += (voltage[1]==0) ? complex(0,0) : ~(power[1]/voltage[1]);
					current_inj[2] += (voltage[2]==0) ? complex(0,0) : ~(power[2]/voltage[2]);

					current_inj[0] +=voltage[0]*shunt[0];			//PQZ needs load currents calculated as well
					current_inj[1] +=voltage[1]*shunt[1];
					current_inj[2] +=voltage[2]*shunt[2];

					current_inj[0] +=current[0];		//Update load current values if PQI
					current_inj[1] +=current[1];		
					current_inj[2] +=current[2];
				}

				//If we are a child, apply our current injection directly up to our parent (links should have accumulated before us)
				if ((SubNode==CHILD) || (SubNode==DIFF_CHILD))
				{
					node *ParLoadObj=OBJECTDATA(SubNodeParent,node);

					//Lock the parent object
					LOCK_OBJECT(SubNodeParent);

					ParLoadObj->current_inj[0] += current_inj[0];
					ParLoadObj->current_inj[1] += current_inj[1];
					ParLoadObj->current_inj[2] += current_inj[2];

					//Unlock it
					UNLOCK_OBJECT(SubNodeParent);
				}
			}
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
	}

	/* record the power in for posterity */
	//kva_in = (voltageA*~current[0] + voltageB*~current[1] + voltageC*~current[2])/1000; /*...or not.  Note sure how this works for a node*/

#endif
	if ((solver_method == SM_NR) && (SubNode==CHILD) && (NR_cycle==false))	//Remove child contributions
	{
		node *ParToLoad = OBJECTDATA(SubNodeParent,node);

		//Lock the parent for writing
		LOCK_OBJECT(SubNodeParent);

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

		//Unlock the parent now that it is done
		UNLOCK_OBJECT(SubNodeParent);

		if (has_phase(PHASE_S))	//Triplex slightly different
			ParToLoad->current12-=last_child_current12;

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

	if ((solver_method == SM_NR) && ((SubNode==CHILD) || (SubNode==DIFF_CHILD)) && (NR_cycle==false))	//Child Voltage Updates
	{
		node *ParStealLoad = OBJECTDATA(SubNodeParent,node);

		//Steal our paren't voltages as well
		voltage[0] = ParStealLoad->voltage[0];
		voltage[1] = ParStealLoad->voltage[1];
		voltage[2] = ParStealLoad->voltage[2];
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
		LOCKED(obj, voltage12 = voltage1 + voltage2);
		LOCKED(obj, voltage1N = voltage1 - voltageN);
		LOCKED(obj, voltage2N = voltage2 - voltageN);
	}
	else
	{	// compute 3phase voltage differences
		LOCKED(obj, voltageAB = voltageA - voltageB);
		LOCKED(obj, voltageBC = voltageB - voltageC);
		LOCKED(obj, voltageCA = voltageC - voltageA);
	}

	if (solver_method==SM_FBS)
	{
		// if the parent object is a node
		if (obj->parent!=NULL && (gl_object_isa(obj->parent,"node","powerflow")))
		{
			// copy the voltage from the parent - check for mismatch handled earlier
			node *pNode = OBJECTDATA(obj->parent,node);
			LOCKED(obj, voltage[0] = pNode->voltage[0]);
			LOCKED(obj, voltage[1] = pNode->voltage[1]);
			LOCKED(obj, voltage[2] = pNode->voltage[2]);

			//Re-update our Delta or single-phase equivalents since we now have a new voltage
			//Update appropriate "other" voltages
			if (phases&PHASE_S) 
			{	// split-tap voltage diffs are different
				LOCKED(obj, voltage12 = voltage1 + voltage2);
				LOCKED(obj, voltage1N = voltage1 - voltageN);
				LOCKED(obj, voltage2N = voltage2 - voltageN);
			}
			else
			{	// compute 3phase voltage differences
				LOCKED(obj, voltageAB = voltageA - voltageB);
				LOCKED(obj, voltageBC = voltageB - voltageC);
				LOCKED(obj, voltageCA = voltageC - voltageA);
			}
		}
	}
	else if (solver_method==SM_GS)//GS items - solver and misc. related
	{
		if (GS_all_converged)
		{
			if (GS_converged)
				RetValue=t1;
			else	//Not converged
			{
				GS_all_converged=false;
				GS_converged=false;
				
				RetValue=t0;
			}
		}
		else	//Not converged
			RetValue=t0;

		//Check to see if we think we're converged.  If so, do miscellaneous final calc
		if (RetValue==t1)
		{
			LINKCONNECTED *linkscanner;	//Temp variable for connected links list - used to find currents flowing out of node
			OBJECT *currlinkobj;	//Temp variable for link of interest
			link *currlink;			//temp Reference to link of interest

			//See if we are a child node - if so, steal our parent's voltage values
			if (SubNode==CHILD)
			{
				node *parNode = OBJECTDATA(SubNodeParent,node);

				voltage[0]=parNode->voltage[0];
				voltage[1]=parNode->voltage[1];
				voltage[2]=parNode->voltage[2];

				//Update single phase/delta computations as necessary
				if (phases&PHASE_S) 
				{	// split-tap voltage diffs are different
					LOCKED(obj, voltage12 = voltage1 + voltage2);
					LOCKED(obj, voltage1N = voltage1 - voltageN);
					LOCKED(obj, voltage2N = voltage2 - voltageN);
				}
				else
				{	// compute 3phase voltage differences
					LOCKED(obj, voltageAB = voltageA - voltageB);
					LOCKED(obj, voltageBC = voltageB - voltageC);
					LOCKED(obj, voltageCA = voltageC - voltageA);
				}
			}

			//Zero current for below calcuations.  May mess with tape (will have values at end of Postsync)
			current_inj[0] = current_inj[1] = current_inj[2] = complex(0,0);

			//Calculate current if it has one
			if ((bustype==PQ) | (bustype==PV)) //PQ and PV busses need current updates
			{
				if ((!has_phase(PHASE_A|PHASE_B|PHASE_C)) && (!has_phase(PHASE_D)))  //Not three phase or delta
				{
					current_inj[0] = (voltage[0]==0) ? complex(0,0) : ~(power[0]/voltage[0]);
					current_inj[1] = (voltage[1]==0) ? complex(0,0) : ~(power[1]/voltage[1]);
					current_inj[2] = (voltage[2]==0) ? complex(0,0) : ~(power[2]/voltage[2]);
				}
				else //Three phase
				{
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

						current_inj[0] += delta_shunt[0]-delta_shunt[2];	//calculate "load" current (line current) - PQZ
						current_inj[1] += delta_shunt[1]-delta_shunt[0];
						current_inj[2] += delta_shunt[2]-delta_shunt[1];

						current_inj[0] +=delta_current[0]-delta_current[2];	//Calculate "load" current (line current) - PQP
						current_inj[1] +=delta_current[1]-delta_current[0];
						current_inj[2] +=delta_current[2]-delta_current[1];

						current_inj[0] +=current[0]-current[2];	//Calculate "load" current (line current) - PQI
						current_inj[1] +=current[1]-current[0];
						current_inj[2] +=current[2]-current[1];
					}
					else					//Wye connection
					{
						current_inj[0] += (voltage[0]==0) ? complex(0,0) : ~(power[0]/voltage[0]);			//PQP needs power converted to current
						current_inj[1] += (voltage[1]==0) ? complex(0,0) : ~(power[1]/voltage[1]);
						current_inj[2] += (voltage[2]==0) ? complex(0,0) : ~(power[2]/voltage[2]);

						current_inj[0] +=voltage[0]*shunt[0];			//PQZ needs load currents calculated as well
						current_inj[1] +=voltage[1]*shunt[1];
						current_inj[2] +=voltage[2]*shunt[2];

						current_inj[0] +=current[0];		//Update load current values if PQI
						current_inj[1] +=current[1];		
						current_inj[2] +=current[2];
					}
				}
			}
			else	//Swing bus - just make sure it is zero
			{
				current_inj[0] = current_inj[1] = current_inj[2] = complex(0,0);	//Swing has no load current or anything else
			}

			//Now accumulate branch currents, so can see what "flows" passes through the node
			linkscanner = &nodelinks;

			while (linkscanner->next!=NULL)		//Parse through the linked list
			{
				linkscanner = linkscanner->next;

				currlinkobj = linkscanner->connectedlink;

				currlink = OBJECTDATA(currlinkobj,link);
				
				//Update branch currents - explicitly check to and from in case this is a parent structure
				if (((currlink->from)->id) == (obj->id))	//see if we are the from end
				{
					if  ((currlink->power_in.Mag()) > (currlink->power_out.Mag()))	//Make sure current flowing right direction
					{
						current_inj[0] += currlink->current_in[0];
						current_inj[1] += currlink->current_in[1];
						current_inj[2] += currlink->current_in[2];
					}
				}
				else if (((currlink->to)->id) == (obj->id))	//see if we are the to end
				{
					if ((currlink->power_in.Mag()) < (currlink->power_out.Mag()))	//Current is reversed, so this is a from to
					{
						current_inj[0] -= currlink->current_out[0];
						current_inj[1] -= currlink->current_out[1];
						current_inj[2] -= currlink->current_out[2];
					}
				}
			}

			if (SubNode==CHILD)	//Remove child contributions
			{
				node *ParToLoad = OBJECTDATA(SubNodeParent,node);

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

				//Update previous power tracker - if we haven't really converged, things will mess up without this
				//Power
				last_child_power[0][0] = last_child_power[0][1] = last_child_power[0][2] = 0.0;

				//Shunt
				last_child_power[1][0] = last_child_power[1][1] = last_child_power[1][2] = 0.0;

				//Current
				last_child_power[2][0] = last_child_power[2][1] = last_child_power[2][2] = 0.0;
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
* GS_P_C_NodeChecks performs Gauss-Seidel related parent/child update routines
* Subfunctioned to help clean up the sync code so it is more readable.
*/
void *node::GS_P_C_NodeChecks(TIMESTAMP t0, TIMESTAMP t1, OBJECT *obj, LINKCONNECTED *linktable)
{
	if (SubNode==CHILD_NOINIT)	//First run of a "parented" node
	{
		//Grab the linked list from the child object
		node *ParOfChildNode = OBJECTDATA(SubNodeParent,node);
		LINKCONNECTED *parlink = &ParOfChildNode->nodelinks;

		linktable = &nodelinks;

		if (linktable->next!=NULL)	//See if it was empty
		{
			//Need working variable for updating down/upstream links (those that refer to us)
			LINKCONNECTED *ltemp;
			node *tempnode;

			while (linktable->next!=NULL)		//Parse through the linked list
			{
				linktable = linktable->next;

				LINKCONNECTED *lnewconnected = (LINKCONNECTED *)gl_malloc(sizeof(LINKCONNECTED));
				if (lnewconnected==NULL)
				{
					gl_error("GS: memory allocation failure");
					/*	TROUBLESHOOT
					This is a bug.  GridLAB-D failed to allocate memory for link connections to this node.
					Please submit this bug and your code.
					*/
					return 0;
				}

				lnewconnected->next = parlink->next;

				// attach to node list
				parlink->next = lnewconnected;

				// attach the link object identifier
				lnewconnected->connectedlink = linktable->connectedlink;

				// attach to and from nodes - substitute us for the appropriate link
				if (linktable->fnodeconnected==obj)	//We were the from node, so now parent is
				{
					lnewconnected->fnodeconnected = SubNodeParent;
					lnewconnected->tnodeconnected = linktable->tnodeconnected;

					//Now go search that node and update its back links to our parent
					tempnode = OBJECTDATA(linktable->tnodeconnected,node);
					ltemp = &tempnode->nodelinks;

					while (ltemp->next!=NULL)		//Parse through the linked list
					{
						ltemp = ltemp->next;
						
						if (ltemp->fnodeconnected==obj)	//We were the from node, so now parent is from node
							ltemp->fnodeconnected=SubNodeParent;
						else if (ltemp->tnodeconnected==obj)	//We were somehow the to node...shouldn't happen, but just in case
							ltemp->tnodeconnected=SubNodeParent;
					}
				}
				else	//We were to node, so now parent is
				{
					lnewconnected->fnodeconnected = linktable->fnodeconnected;
					lnewconnected->tnodeconnected = SubNodeParent;

					//Now go search that node and update its back links to our parent
					tempnode = OBJECTDATA(linktable->fnodeconnected,node);
					ltemp = &tempnode->nodelinks;

					while (ltemp->next!=NULL)		//Parse through the linked list
					{
						ltemp = ltemp->next;
						
						if (ltemp->fnodeconnected==obj)	//We were the from node, so now parent is from node
							ltemp->fnodeconnected=SubNodeParent;
						else if (ltemp->tnodeconnected==obj)	//We were somehow the to node...shouldn't happen, but just in case
							ltemp->tnodeconnected=SubNodeParent;
					}
				}
			}

			//clear child Node's linked list so it doesn't try to update things
			nodelinks.next=NULL;
		}

		SubNode=CHILD;	//Flag as having been initialized, hence done
	}

	if ((SubNode==CHILD) && (prev_NTime!=t0))	//First run, additional housekeeping
	{
		node *ParNodeAdmit = OBJECTDATA(SubNodeParent,node);

		//Import Admittance and YVs terms
		ParNodeAdmit->Ys[0][0]+=Ys[0][0];
		ParNodeAdmit->Ys[0][1]+=Ys[0][1];
		ParNodeAdmit->Ys[0][2]+=Ys[0][2];
		ParNodeAdmit->Ys[1][0]+=Ys[1][0];
		ParNodeAdmit->Ys[1][1]+=Ys[1][1];
		ParNodeAdmit->Ys[1][2]+=Ys[1][2];
		ParNodeAdmit->Ys[2][0]+=Ys[2][0];
		ParNodeAdmit->Ys[2][1]+=Ys[2][1];
		ParNodeAdmit->Ys[2][2]+=Ys[2][2];

		ParNodeAdmit->YVs[0]+=YVs[0];
		ParNodeAdmit->YVs[1]+=YVs[1];
		ParNodeAdmit->YVs[2]+=YVs[2];
	}
	
	if (SubNode==CHILD)
	{
		node *ParToLoad = OBJECTDATA(SubNodeParent,node);

		if (gl_object_isa(SubNodeParent,"load"))	//Load gets cleared at every presync, so reaggregate :(
		{
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
		}
		else if (gl_object_isa(SubNodeParent,"node"))	//"parented" node - update values - This has to go to the bottom
		{												//since load/meter share with node (and load handles power in presync)
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
		}
		else
			throw("GS: Object %d is a child of something that it shouldn't be!",obj->id);
			/*  TROUBLESHOOT
			A Gauss-Seidel object is childed to something it should not be (not a load, node, or meter).
			This should have been caught earlier and is likely a bug.  Submit your code and a bug report.
			*/

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
	}
	return 0;
}
/**
* Attachlink is called by link objects during their creation.  It creates a linked list
* of links that are attached to the current node.
*
* @param obj the link that has an attachment to this node
*/
LINKCONNECTED *node::attachlink(OBJECT *obj) ///< object to attach
{
	//Pull link information
	link *templink = OBJECTDATA(obj,link);

	// construct and id the new circuit
	LINKCONNECTED *lconnected = (LINKCONNECTED *)gl_malloc(sizeof(LINKCONNECTED));
	if (lconnected==NULL)
	{
		gl_error("GS: memory allocation failure for link table");
		return 0;
		/* Note ~ this returns a null pointer, but iff the malloc fails.  If
		 * that happens, we're already in SEGFAULT sort of bad territory. */
	}
	
	lconnected->next = nodelinks.next;

	// attach to node list
	nodelinks.next = lconnected;

	// attach the link object identifier
	lconnected->connectedlink = obj;
	
	// attach to and from nodes
	lconnected->fnodeconnected = templink->from;
	lconnected->tnodeconnected = templink->to;
	
	return 0;
}
/**
* NR_populate is called by link objects during their first presync if a node is not
* "initialized".  This function "initializes" the node into the Newton-Raphson data
* structure NR_busdata
*
*/
int *node::NR_populate(void)
{
		//Object header for names
		OBJECT *me = OBJECTHDR(this);

		//Lock the SWING for global operations
		LOCK_OBJECT(NR_swing_bus);

		NR_node_reference = NR_curr_bus;	//Grab the current location and keep it as our own
		NR_curr_bus++;					//Increment the current bus pointer for next variable
		UNLOCK_OBJECT(NR_swing_bus);	//All done playing with globals, unlock the swing so others can proceed

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

		//Populate phases
		NR_busdata[NR_node_reference].phases = 128*has_phase(PHASE_S) + 8*has_phase(PHASE_D) + 4*has_phase(PHASE_A) + 2*has_phase(PHASE_B) + has_phase(PHASE_C);

		//Link our name in
		NR_busdata[NR_node_reference].name = me->name;

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

EXPORT int isa_node(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,node)->isa(classname);
	} else {
		return 0;
	}
}

/**@}*/
