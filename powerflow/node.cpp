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
				PT_KEYWORD, "PQ", (enumeration)PQ,
				PT_KEYWORD, "PV", (enumeration)PV,
				PT_KEYWORD, "SWING", (enumeration)SWING,
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

			PT_complex, "current_AB[A]", PADDR(current_dy[0]),PT_DESCRIPTION,"bus current delta-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_BC[A]", PADDR(current_dy[1]),PT_DESCRIPTION,"bus current delta-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_CA[A]", PADDR(current_dy[2]),PT_DESCRIPTION,"bus current delta-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_AN[A]", PADDR(current_dy[3]),PT_DESCRIPTION,"bus current wye-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_BN[A]", PADDR(current_dy[4]),PT_DESCRIPTION,"bus current wye-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_CN[A]", PADDR(current_dy[5]),PT_DESCRIPTION,"bus current wye-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_AB[VA]", PADDR(power_dy[0]),PT_DESCRIPTION,"bus power delta-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_BC[VA]", PADDR(power_dy[1]),PT_DESCRIPTION,"bus power delta-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_CA[VA]", PADDR(power_dy[2]),PT_DESCRIPTION,"bus power delta-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_AN[VA]", PADDR(power_dy[3]),PT_DESCRIPTION,"bus power wye-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_BN[VA]", PADDR(power_dy[4]),PT_DESCRIPTION,"bus power wye-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_CN[VA]", PADDR(power_dy[5]),PT_DESCRIPTION,"bus power wye-connected injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "shunt_AB[S]", PADDR(power_dy[0]),PT_DESCRIPTION,"bus shunt delta-connected admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_BC[S]", PADDR(power_dy[1]),PT_DESCRIPTION,"bus shunt delta-connected admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_CA[S]", PADDR(power_dy[2]),PT_DESCRIPTION,"bus shunt delta-connected admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_AN[S]", PADDR(power_dy[3]),PT_DESCRIPTION,"bus shunt wye-connected admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_BN[S]", PADDR(power_dy[4]),PT_DESCRIPTION,"bus shunt wye-connected admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_CN[S]", PADDR(power_dy[5]),PT_DESCRIPTION,"bus shunt wye-connected admittance, this an accumulator only, not a output or input variable",

			PT_double, "mean_repair_time[s]",PADDR(mean_repair_time), PT_DESCRIPTION, "Time after a fault clears for the object to be back in service",

			PT_enumeration, "service_status", PADDR(service_status),PT_DESCRIPTION,"In and out of service flag",
				PT_KEYWORD, "IN_SERVICE", (enumeration)ND_IN_SERVICE,
				PT_KEYWORD, "OUT_OF_SERVICE", (enumeration)ND_OUT_OF_SERVICE,
			PT_double, "service_status_double", PADDR(service_status_dbl),PT_DESCRIPTION,"In and out of service flag - type double - will indiscriminately override service_status - useful for schedules",
			PT_double, "previous_uptime[min]", PADDR(previous_uptime),PT_DESCRIPTION,"Previous time between disconnects of node in minutes",
			PT_double, "current_uptime[min]", PADDR(current_uptime),PT_DESCRIPTION,"Current time since last disconnect of node in minutes",
			PT_bool, "Norton_dynamic", PADDR(dynamic_norton),PT_DESCRIPTION,"Flag to indicate a Norton-equivalent connection -- used for generators and deltamode",

			PT_object, "topological_parent", PADDR(TopologicalParent),PT_DESCRIPTION,"topological parent as per GLM configuration",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

			if (gl_publish_function(oclass,	"delta_linkage_node", (FUNCTIONADDR)delta_linkage)==NULL)
				GL_THROW("Unable to publish node delta_linkage function");
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_node)==NULL)
				GL_THROW("Unable to publish node deltamode function");
			if (gl_publish_function(oclass,	"delta_freq_pwr_object", (FUNCTIONADDR)delta_frequency_node)==NULL)
				GL_THROW("Unable to publish node deltamode function");
    
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

	full_Y = NULL;		//Not used by default
	full_Y_all = NULL;	//Not used by default
	DynVariable = NULL;	//Not used by default

	memset(voltage,0,sizeof(voltage));
	memset(voltaged,0,sizeof(voltaged));
	memset(current,0,sizeof(current));
	memset(power,0,sizeof(power));
	memset(shunt,0,sizeof(shunt));

	current_dy[0] = current_dy[1] = current_dy[2] = complex(0.0,0.0);
	current_dy[3] = current_dy[4] = current_dy[5] = complex(0.0,0.0);
	power_dy[0] = power_dy[1] = power_dy[2] = complex(0.0,0.0);
	power_dy[3] = power_dy[4] = power_dy[5] = complex(0.0,0.0);
	shunt_dy[0] = shunt_dy[1] = shunt_dy[2] = complex(0.0,0.0);
	shunt_dy[3] = shunt_dy[4] = shunt_dy[5] = complex(0.0,0.0);

	prev_voltage_value = NULL;	//NULL the pointer, just for the sake of doing so
	prev_power_value = NULL;	//NULL the pointer, again just for the sake of doing so
	node_type = NORMAL_NODE;	//Assume we're nothing special by default
	current_accumulated = false;
	deltamode_inclusive = false;	//Begin assuming we aren't delta-enabled
	dynamic_norton = false;			//By default, no one needs the Norton equivalent posting

	return result;
}

int node::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);

	if (solver_method==SM_NR)
	{
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
			
			//Do the same for substations
			gl_free(bus_list);
			bus_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"substation",FT_END);

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
			
			//Do the same for elec_frequency
			gl_free(bus_list);
			bus_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"elec_frequency",FT_END);

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
			//See if it is a node/load/meter/substation
			if (!(gl_object_isa(obj->parent,"load","powerflow") | gl_object_isa(obj->parent,"node","powerflow") | gl_object_isa(obj->parent,"meter","powerflow") | gl_object_isa(obj->parent,"substation","powerflow")))
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
							parNode->NR_number_child_nodes[0]++;	//Increment the counter of child nodes - we'll alloc and link them later

							//Update the pointer to our parent's NR pointer (so links can go there appropriately)
							NR_subnode_reference = &(parNode->NR_node_reference);

							//Zero out last child power vector (used for updates)
							last_child_power[0][0] = last_child_power[0][1] = last_child_power[0][2] = complex(0,0);
							last_child_power[1][0] = last_child_power[1][1] = last_child_power[1][2] = complex(0,0);
							last_child_power[2][0] = last_child_power[2][1] = last_child_power[2][2] = complex(0,0);
							last_child_current12 = 0.0;

							//Do the same for the delta/wye explicit portions
							last_child_power_dy[0][0] = last_child_power_dy[0][1] = last_child_power_dy[0][2] = complex(0.0,0.0);
							last_child_power_dy[1][0] = last_child_power_dy[1][1] = last_child_power_dy[1][2] = complex(0.0,0.0);
							last_child_power_dy[2][0] = last_child_power_dy[2][1] = last_child_power_dy[2][2] = complex(0.0,0.0);
							last_child_power_dy[3][0] = last_child_power_dy[3][1] = last_child_power_dy[3][2] = complex(0.0,0.0);
							last_child_power_dy[4][0] = last_child_power_dy[4][1] = last_child_power_dy[4][2] = complex(0.0,0.0);
							last_child_power_dy[5][0] = last_child_power_dy[5][1] = last_child_power_dy[5][2] = complex(0.0,0.0);
						}
					}

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

				//Do the same for the delta/wye explicit portions
				last_child_power_dy[0][0] = last_child_power_dy[0][1] = last_child_power_dy[0][2] = complex(0.0,0.0);
				last_child_power_dy[1][0] = last_child_power_dy[1][1] = last_child_power_dy[1][2] = complex(0.0,0.0);
				last_child_power_dy[2][0] = last_child_power_dy[2][1] = last_child_power_dy[2][2] = complex(0.0,0.0);
				last_child_power_dy[3][0] = last_child_power_dy[3][1] = last_child_power_dy[3][2] = complex(0.0,0.0);
				last_child_power_dy[4][0] = last_child_power_dy[4][1] = last_child_power_dy[4][2] = complex(0.0,0.0);
				last_child_power_dy[5][0] = last_child_power_dy[5][1] = last_child_power_dy[5][2] = complex(0.0,0.0);
			}//end no issues phase

			//Make sure nominal voltages match
			if (nominal_voltage != parNode->nominal_voltage)
			{
				gl_warning("Node:%s does not have the same nominal voltage as its parent - copying voltage from parent.",(obj->name ? obj->name : "unnamed"));
				/*  TROUBLESHOOT
				A node object was parented to another node object with a different nominal_voltage set.  The childed object will now
				take the parent nominal_voltage.  If this is not intended, please add a transformer between these nodes.
				*/

				nominal_voltage = parNode->nominal_voltage;
			}
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
				Newton-Raphson failed to automatically assign a swing bus to the node (unchilded nodes are referenced
				to the swing bus).  This should have been detected by this point and represents a bug in the solver.
				Please submit a bug report detailing how you obtained this message.
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

			if(default_resistance <= 0.0){
				gl_error("INIT: The global default_resistance was less than or equal to zero. default_resistance must be greater than zero.");
				return 0;
			}
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
		// Store the topological parent before anyone overwrites it
		TopologicalParent = obj->parent;

		if (obj->parent != NULL)
		{
			if((gl_object_isa(obj->parent,"load","powerflow") | gl_object_isa(obj->parent,"node","powerflow") | gl_object_isa(obj->parent,"meter","powerflow") | gl_object_isa(obj->parent,"substation","powerflow")))	//Parent is another node
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

				//Make sure nominal voltages match
				if (nominal_voltage != parNode->nominal_voltage)
				{
					gl_warning("Node:%s does not have the same nominal voltage as its parent - copying voltage from parent.",(obj->name ? obj->name : "unnamed"));
					//Define above

					nominal_voltage = parNode->nominal_voltage;
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
		voltaged[0] = voltage[0] + voltage[1];	//12
		voltaged[1] = voltage[1] - voltage[2];	//2N
		voltaged[2] = voltage[0] - voltage[2];	//1N -- odd order
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

	//Deltamode checks - init, so no locking yet
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		//Flag the variable for later use
		deltamode_inclusive = true;

		//Increment the counter for allocation
		pwr_object_count++;

		//If we're SWING, map the variable for the extra function as well
		if (bustype == SWING)
		{
			//Assign the function variable for deltamode
			deltamode_extra_function = (int64)(&(delta_extra_function));
		}

		//Check out parent and toss some warnings
		if (TopologicalParent != NULL)
		{
			if ((TopologicalParent->flags & OF_DELTAMODE) != OF_DELTAMODE)
			{
				gl_warning("Object %s (node:%d) is flagged for deltamode, but it's parent is not.  This may lead to incorrect answers!",obj->name?obj->name:"Unknown",obj->id);
				/*  TROUBLESHOOT
				A childed node's parent is not flagged for deltamode.  This may lead to some erroneous errors in the powerflow.  Please apply the
				flags DELTAMODE property to the parent, or utilize the all_powerflow_delta module-level flag to fix this.
				*/
			}
		}
	}

	return result;
}

//Functionalized presync pass routines for NR solver
//Put in place so deltamode can call it and properly udpate
void node::NR_node_presync_fxn(void)
{
	//Zero the accumulators for later (meters and such)
	current_inj[0] = current_inj[1] = current_inj[2] = 0.0;

	//Reset flag
	current_accumulated = false;

	if ((SubNode==DIFF_PARENT))	//Differently connected parent - zero our accumulators
	{
		//Zero them.  Row 1 is power, row 2 is admittance, row 3 is current
		Extra_Data[0] = Extra_Data[1] = Extra_Data[2] = 0.0;

		Extra_Data[3] = Extra_Data[4] = Extra_Data[5] = 0.0;

		Extra_Data[6] = Extra_Data[7] = Extra_Data[8] = 0.0;
	}

	//Uncomment us eventually, like when houses work in deltamode
	////If we're a parent and "have house", zero our accumulator
	//if ((SubNode==PARENT) && (house_present==true))
	//{
	//	nom_res_curr[0] = nom_res_curr[1] = nom_res_curr[2] = 0.0;
	//}

}

//Full node presync function
TIMESTAMP node::presync(TIMESTAMP t0)
{
	unsigned int index_val;
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t1 = powerflow_object::presync(t0); 
	TIMESTAMP temp_time_value;
	node *temp_par_node = NULL;

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

		//Deltamode check - let every object do it, for giggles
		if (enable_subsecond_models == true)
		{
			if (solver_method != SM_NR)
			{
				GL_THROW("deltamode simulations only support powerflow in Newton-Raphson (NR) mode at this time");
				/*  TROUBLESHOOT
				deltamode and dynamics simulations will only work with the powerflow module in Newton-Raphson mode.
				Please swap to that solver and try again.
				*/
			}
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

			//initialize the bustype - will be used for detection
			for (index_val=0; index_val<NR_bus_count; index_val++)
				NR_busdata[index_val].type = -1;

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

			//Initialize the from - will be used for detection
			for (index_val=0; index_val<NR_branch_count; index_val++)
				NR_branchdata[index_val].from = -1;

			//Allocate deltamode stuff as well
			if (enable_subsecond_models==true)	//If enabled in powerflow, swing bus rules them all
			{
				//Make sure no one else has done it yet
				if ((pwr_object_current == -1) || (delta_objects==NULL))
				{
					//Allocate the deltamode object array
					delta_objects = (OBJECT**)gl_malloc(pwr_object_count*sizeof(OBJECT*));

					//Make sure it worked
					if (delta_objects == NULL)
					{
						GL_THROW("Failed to allocate deltamode objects array for powerflow module!");
						/*  TROUBLESHOOT
						While attempting to create a reference array for powerflow module deltamode-enabled
						objects, an error was encountered.  Please try again.  If the error persists, please
						submit your code and a bug report via the trac website.
						*/
					}

					//Allocate the function reference list as well
					delta_functions = (FUNCTIONADDR*)gl_malloc(pwr_object_count*sizeof(FUNCTIONADDR));

					//Make sure it worked
					if (delta_functions == NULL)
					{
						GL_THROW("Failed to allocate deltamode objects function array for powerflow module!");
						/*  TROUBLESHOOT
						While attempting to create a reference array for powerflow module deltamode-enabled
						objects, an error was encountered.  Please try again.  If the error persists, please
						submit your code and a bug report via the trac website.
						*/
					}

					//Allocate "frequency" function references
					delta_freq_functions = (FUNCTIONADDR*)gl_malloc(pwr_object_count*sizeof(FUNCTIONADDR));

					//Make sure it worked
					if (delta_freq_functions == NULL)
					{
						GL_THROW("Failed to allocate deltamode objects function array for powerflow module!");
						//Defined above
					}

					//Initialize index
					pwr_object_current = 0;
				}

				//See if we are the swing bus AND our flag is enabled, otherwise warn
				if ((bustype==SWING) && (deltamode_inclusive == false))
				{
					gl_warning("SWING bus:%s is not flagged for deltamode",obj->name);
					/*  TROUBLESHOOT
					Deltamode is enabled for the overall powerflow module, but not for the SWING node
					of the powerflow.  This means the overall powerflow is not being calculated or included
					in any deltamode calculations.  If this is not a desired run method, please enable the
					deltamode_inclusive flag on the SWING node.
					*/
				}
			}

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

		//Comment us out eventually, when houses work in deltamode
		//If we're a parent and "have house", zero our accumulator
		if ((SubNode==PARENT) && (house_present==true))
		{
			nom_res_curr[0] = nom_res_curr[1] = nom_res_curr[2] = 0.0;
		}

		//Populate individual object references into deltamode, if needed
		if ((deltamode_inclusive==true) && (enable_subsecond_models == true) && (prev_NTime==0))
		{
			int temp_pwr_object_current;

			//Check limits first
			if (pwr_object_current>=pwr_object_count)
			{
				GL_THROW("Too many objects tried to populate deltamode objects array in the powerflow module!");
				/*  TROUBLESHOOT
				While attempting to populate a reference array of deltamode-enabled objects for the powerflow
				module, an attempt was made to write beyond the allocated array space.  Please try again.  If the
				error persists, please submit a bug report and your code via the trac website.
				*/
			}

			//Lock the SWING bus and get us a value
			if ( NR_swing_bus!=obj) WRITELOCK_OBJECT(NR_swing_bus);	//Lock Swing for flag

				//Get the value
				temp_pwr_object_current = pwr_object_current;

				//Increment
				pwr_object_current++;

			//Unlock
			if ( NR_swing_bus!=obj) WRITEUNLOCK_OBJECT(NR_swing_bus);	//Lock Swing for flag

			//Add us into the list
			delta_objects[temp_pwr_object_current] = obj;

			//Map up the function
			delta_functions[temp_pwr_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"interupdate_pwr_object"));

			//Make sure it worked
			if (delta_functions[temp_pwr_object_current] == NULL)
			{
				gl_warning("Failure to map deltamode function for device:%s",obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  This error may simply be an indication that the object of interest
				does not support deltamode.  If the error persists and the object does, please submit your code and
				a bug report via the trac website.
				*/
			}

			//Map up the frequency function
			delta_freq_functions[temp_pwr_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"delta_freq_pwr_object"));

			//Make sure it worked
			if (delta_freq_functions[temp_pwr_object_current] == NULL)
			{
				//Make sure we didn't already warn out -- it's an indication we're not delta-compliant and the "overall" flag is hitting us
				if (delta_functions[temp_pwr_object_current] != NULL)
				{
					gl_warning("Failure to map deltamode function for devices:%s",obj->name);
					//Defined above - assumes they exist in pairs
				}
			}

			//Do any additional parent/child mappings for deltamode -- if necessary
			if (((SubNode==CHILD) || (SubNode==DIFF_CHILD)) && (dynamic_norton==true))
			{
				//Map our parent
				temp_par_node = OBJECTDATA(SubNodeParent,node);

				//Make sure it worked, for giggles
				if (temp_par_node == NULL)
				{
					GL_THROW("node:%s - failed to map parent object for childed node",obj->name);
					/*  TROUBLESHOOT
					While attempting to link to the parent node, an error occurred.  Please try again.
					If the error persists, please submit your code and a bug report via the trac website.
					*/
				}

				//See if our parent has been allocated yet or not
				if (temp_par_node->full_Y == NULL)
				{
					//Lock our parent
					LOCK_OBJECT(SubNodeParent);

					//Flag our parent, just to make sure things work properly
					temp_par_node->dynamic_norton = true;

					//Do allocations
					temp_par_node->full_Y = (complex *)gl_malloc(9*sizeof(complex));

					//Check it
					if (temp_par_node->full_Y==NULL)
					{
						GL_THROW("Node:%s failed to allocate space for the a deltamode variable",SubNodeParent->name);
						/*  TROUBLESHOOT
						While attempting to allocate memory for a dynamics-required (deltamode) variable, an error
						occurred. Please try again.  If the error persists, please submit your code and a bug
						report via the trac website.
						*/
					}

					//Zero it, just to be safe (gens will accumulate into it)
					temp_par_node->full_Y[0] = temp_par_node->full_Y[1] = temp_par_node->full_Y[2] = complex(0.0,0.0);
					temp_par_node->full_Y[3] = temp_par_node->full_Y[4] = temp_par_node->full_Y[5] = complex(0.0,0.0);
					temp_par_node->full_Y[6] = temp_par_node->full_Y[7] = temp_par_node->full_Y[8] = complex(0.0,0.0);

					//Allocate another matrix for admittance - this will have the full value
					temp_par_node->full_Y_all = (complex *)gl_malloc(9*sizeof(complex));

					//Check it
					if (temp_par_node->full_Y_all==NULL)
					{
						GL_THROW("Node:%s failed to allocate space for the a deltamode variable",SubNodeParent->name);
						//Defined above
					}

					//Zero it, just to be safe (gens will accumulate into it)
					temp_par_node->full_Y_all[0] = temp_par_node->full_Y_all[1] = temp_par_node->full_Y_all[2] = complex(0.0,0.0);
					temp_par_node->full_Y_all[3] = temp_par_node->full_Y_all[4] = temp_par_node->full_Y_all[5] = complex(0.0,0.0);
					temp_par_node->full_Y_all[6] = temp_par_node->full_Y_all[7] = temp_par_node->full_Y_all[8] = complex(0.0,0.0);

					//Do the same for a dynamics contribution (just 4x1 for now for normal nodes)
					//0-2 represent ABC current,3 represents overall power, 4 represents power frequency weighting,
					//5 represents overall output power

					temp_par_node->DynVariable = (complex *)gl_malloc(6*sizeof(complex));

					//Check it
					if (temp_par_node->DynVariable==NULL)
					{
						GL_THROW("Node:%s failed to allocate space for the a deltamode variable",SubNodeParent->name);
						//Defined above
					}

					//Zero them, for consistency
					temp_par_node->DynVariable[0] = temp_par_node->DynVariable[1] = temp_par_node->DynVariable[2] = complex(0.0,0.0);
					temp_par_node->DynVariable[3] = temp_par_node->DynVariable[4] = temp_par_node->DynVariable[5] = complex(0.0,0.0);

					//Unlock our parent
					UNLOCK_OBJECT(SubNodeParent);
				}
				//Default else - it's mapped

				//Link the local pointers
				full_Y = temp_par_node->full_Y;
				full_Y_all = temp_par_node->full_Y_all;
				DynVariable = temp_par_node->DynVariable;

				//No need to do NR mappings - we don't get hit anyways
			}//End child norton equivalent postings code
		}//end deltamode allocations

		//Call NR presync function
		NR_node_presync_fxn();

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
	
	return t1;
}

//Functionalized sync pass routines for NR solver
//Put in place so deltamode can call it and properly udpate
void node::NR_node_sync_fxn(OBJECT *obj)
{
	int loop_index_var;

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
					voltaged[0] = voltage[0] + voltage[1];	//12
					voltaged[1] = voltage[1] - voltage[2];	//2N
					voltaged[2] = voltage[0] - voltage[2];	//1N -- unsure why odd

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

		if (SubNode==CHILD)
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

				//Do the same for explicit delta/wye portions
				for (loop_index_var=0; loop_index_var<6; loop_index_var++)
				{
					ParToLoad->power_dy[loop_index_var] += power_dy[loop_index_var];
					ParToLoad->shunt_dy[loop_index_var] += shunt_dy[loop_index_var];
					ParToLoad->current_dy[loop_index_var] += current_dy[loop_index_var];
				}

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

				//Do the same for the explicit delta/wye loads - last_child_power is set up as columns of ZIP, not ABC
				for (loop_index_var=0; loop_index_var<6; loop_index_var++)
				{
					ParToLoad->power_dy[loop_index_var] += power_dy[loop_index_var] - last_child_power_dy[loop_index_var][0];
					ParToLoad->shunt_dy[loop_index_var] += shunt_dy[loop_index_var] - last_child_power_dy[loop_index_var][1];
					ParToLoad->current_dy[loop_index_var] += current_dy[loop_index_var] - last_child_power_dy[loop_index_var][2];
				}

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

			//Do the same for delta/wye explicit loads
			for (loop_index_var=0; loop_index_var<6; loop_index_var++)
			{
				last_child_power_dy[loop_index_var][0] = power_dy[loop_index_var];
				last_child_power_dy[loop_index_var][1] = shunt_dy[loop_index_var];
				last_child_power_dy[loop_index_var][2] = current_dy[loop_index_var];
			}

			if (has_phase(PHASE_S))		//Triplex extra current update
				last_child_current12 = current12;
		}

		//Accumulations for "differently connected nodes" is still basically the same for delta/wye combination loads
		if (SubNode==DIFF_CHILD)
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

			//Import power and "load" characteristics for explicit delta/wye portions
			for (loop_index_var=0; loop_index_var<6; loop_index_var++)
			{
				ParToLoad->power_dy[loop_index_var] += power_dy[loop_index_var];
				ParToLoad->shunt_dy[loop_index_var] += shunt_dy[loop_index_var];
				ParToLoad->current_dy[loop_index_var] += current_dy[loop_index_var];
			}

			//Finished, unlock parent
			UNLOCK_OBJECT(SubNodeParent);

			//Update our tracking variable
			for (loop_index_var=0; loop_index_var<6; loop_index_var++)
			{
				last_child_power_dy[loop_index_var][0] = power_dy[loop_index_var];
				last_child_power_dy[loop_index_var][1] = shunt_dy[loop_index_var];
				last_child_power_dy[loop_index_var][2] = current_dy[loop_index_var];
			}
		}//End differently connected child
	}//end not uninitialized
}

TIMESTAMP node::sync(TIMESTAMP t0)
{
	TIMESTAMP t1 = powerflow_object::sync(t0);
	OBJECT *obj = OBJECTHDR(this);
	complex delta_current[3];
	complex power_current[3];
	complex delta_shunt[3];
	complex delta_shunt_curr[3];
	complex dy_curr_accum[3];
	
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

		//Handle explicit delta-wye connections now -- no triplex
		if (!(has_phase(PHASE_S)))
		{
			//Convert delta connected power to appropriate line current
			delta_current[0]= (voltageAB.IsZero()) ? 0 : ~(power_dy[0]/voltageAB);
			delta_current[1]= (voltageBC.IsZero()) ? 0 : ~(power_dy[1]/voltageBC);
			delta_current[2]= (voltageCA.IsZero()) ? 0 : ~(power_dy[2]/voltageCA);

			power_current[0]=delta_current[0]-delta_current[2];
			power_current[1]=delta_current[1]-delta_current[0];
			power_current[2]=delta_current[2]-delta_current[1];

			//Convert delta connected load to appropriate line current
			delta_shunt[0] = voltageAB*shunt_dy[0];
			delta_shunt[1] = voltageBC*shunt_dy[1];
			delta_shunt[2] = voltageCA*shunt_dy[2];

			delta_shunt_curr[0] = delta_shunt[0]-delta_shunt[2];
			delta_shunt_curr[1] = delta_shunt[1]-delta_shunt[0];
			delta_shunt_curr[2] = delta_shunt[2]-delta_shunt[1];

			//Convert delta-current into a phase current - reuse temp variable
			delta_current[0]=current_dy[0]-current_dy[2];
			delta_current[1]=current_dy[1]-current_dy[0];
			delta_current[2]=current_dy[2]-current_dy[1];

			//Accumulate
			dy_curr_accum[0] = delta_current[0] + power_current[0] + delta_shunt_curr[0];
			dy_curr_accum[1] = delta_current[1] + power_current[1] + delta_shunt_curr[1];
			dy_curr_accum[2] = delta_current[2] + power_current[2] + delta_shunt_curr[2];

			//Wye-connected portions
			dy_curr_accum[0] += (voltageA.IsZero() || (power_dy[3].IsZero() && shunt_dy[3].IsZero())) ? current_dy[3] : current_dy[3] + ~(power_dy[3]/voltageA) + voltageA*shunt_dy[3];
			dy_curr_accum[1] += (voltageB.IsZero() || (power_dy[4].IsZero() && shunt_dy[4].IsZero())) ? current_dy[4] : current_dy[4] + ~(power_dy[4]/voltageB) + voltageB*shunt_dy[4];
			dy_curr_accum[2] += (voltageC.IsZero() || (power_dy[5].IsZero() && shunt_dy[5].IsZero())) ? current_dy[5] : current_dy[5] + ~(power_dy[5]/voltageC) + voltageC*shunt_dy[5];
				
			//Accumulate in to final portion
			current_inj[0] += dy_curr_accum[0];
			current_inj[1] += dy_curr_accum[1];
			current_inj[2] += dy_curr_accum[2];

		}//End delta/wye explicit

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
			//Call NR sync function items
			NR_node_sync_fxn(obj);

			if ((NR_curr_bus==NR_bus_count) && (bustype==SWING))	//Only run the solver once everything has populated
			{
				bool bad_computation=false;
				NRSOLVERMODE powerflow_type;
				
				//Depending on operation mode, call solver appropriately
				if (deltamode_inclusive)	//Dynamics mode, solve the static in a way that generators are handled right
				{
					if (NR_dyn_first_run==true)	//If it is the first run, perform the initialization powerflow
					{
						powerflow_type = PF_DYNINIT;
						NR_dyn_first_run = false;	//Deflag us for future powerflow solutions
					}
					else	//After first run - call the "dynamic" version of the powerflow solver (SWING bus different)
					{
						powerflow_type = PF_DYNCALC;
					}
				}//End deltamode
				else	//Normal mode
				{
					powerflow_type = PF_NORMAL;
				}

				int64 result = solver_nr(NR_bus_count, NR_busdata, NR_branch_count, NR_branchdata, &NR_powerflow, powerflow_type, &bad_computation);

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

				//See where we wanted to go
				return NR_retval;
			}
			else if (NR_curr_bus==NR_bus_count)	//Population complete, we're not swing, let us go (or we never go on)
				return t1;
			else	//Population of data busses is not complete.  Flag us for a go-around, they should be ready next time
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
				{
					//See if we're a disconnected node
					if (NR_node_reference == -1)
					{
						//Output an error (but don't fail) - SWING bus will cause the explicit failure
						gl_error("Unconnected node - %s id:%d",obj->name?obj->name:"Unknown",obj->id);
						/*  TROUBLESHOOT
						While parsing a GLM, the Newton-Raphson powerflow solver encountered a node
						that does not appear to be connected anywhere.  This will cause problems with the
						solver.  Please verify that this node is supposed to be islanded.
						*/
					}

					return t0;
				}
			}
			break;
		}
	default:
		GL_THROW("unsupported solver method");
		/*	TROUBLESHOOT
		An invalid powerflow solver was specified.  Currently acceptable values are FBS for forward-back
		sweep (Kersting's method) and NR for Newton-Raphson.
		*/
		break;
	}
	return t1;
}

//Functionalized postsync pass routines for NR solver and generic items
//Put in place so deltamode can call it and properly udpate
void node::BOTH_node_postsync_fxn(OBJECT *obj)
{
	/* check for voltage control requirement */
	if (require_voltage_control==TRUE)
	{
		/* PQ bus must have a source */
		if ((busflags&NF_HASSOURCE)==0 && bustype==PQ)
			voltage[0] = voltage[1] = voltage[2] = complex(0,0);
	}

	//Update appropriate "other" voltages
	if (has_phase(PHASE_S))
	{	// split-tap voltage diffs are different
		voltaged[0] = voltage[0] + voltage[1];	//V12
		voltaged[1] = voltage[1] - voltage[2];	//V2N
		voltaged[2] = voltage[0] - voltage[2];	//V1N -- not sure why these are backwards
	}
	else
	{	// compute 3phase voltage differences
		voltaged[0] = voltage[0] - voltage[1];	//AB
		voltaged[1] = voltage[1] - voltage[2];	//BC
		voltaged[2] = voltage[2] - voltage[0];	//CA
	}
	
	//This code performs the new "flattened" NR calculations.
	if (solver_method == SM_NR)
	{
		int result = NR_current_update(true,false);

		//Make sure it worked, just to be thorough
		if (result != 1)
		{
			GL_THROW("Attempt to update current/power on node:%s failed!",obj->name);
			//Defined elsewhere
		}
	}
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
	//Call NR-related and some "common" node postsync routines
	BOTH_node_postsync_fxn(obj);

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

int node::kmldump(int (*stream)(const char*,...))
{
	OBJECT *obj = OBJECTHDR(this);
	if (isnan(obj->latitude) || isnan(obj->longitude))
		return 0;
	stream("<Placemark>\n");
	if (obj->name)
		stream("<name>%s</name>\n", obj->name, obj->oclass->name, obj->id);
	else
		stream("<name>%s %d</name>\n", obj->oclass->name, obj->id);
	stream("<description>\n");
	stream("<![CDATA[\n");
	stream("<TABLE>\n");
	stream("<TR><TD WIDTH=\"25%\">%s&nbsp;%d<HR></TD><TH WIDTH=\"25%\" ALIGN=CENTER>Phase A<HR></TH><TH WIDTH=\"25%\" ALIGN=CENTER>Phase B<HR></TH><TH WIDTH=\"25%\" ALIGN=CENTER>Phase C<HR></TH></TR>\n", obj->oclass->name, obj->id);

	// voltages
	stream("<TR><TH ALIGN=LEFT>Voltage</TH>");
	double vscale = primary_voltage_ratio*sqrt((double) 3.0)/(double) 1000.0;
	if (has_phase(PHASE_A))
		stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kV&nbsp;&nbsp;<BR>%.3f&nbsp;deg&nbsp;</TD>",
			voltageA.Mag()*vscale,voltageA.Arg()*180/3.1416);
	else
		stream("<TD></TD>");
	if (has_phase(PHASE_B))
		stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kV&nbsp;&nbsp;<BR>%.3f&nbsp;deg&nbsp;</TD>",
			voltageB.Mag()*vscale,voltageB.Arg()*180/3.1416);
	else
		stream("<TD></TD>");
	if (has_phase(PHASE_C))
		stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kV&nbsp;&nbsp;<BR>%.3f&nbsp;deg&nbsp;</TD>",
			voltageC.Mag()*vscale,voltageC.Arg()*180/3.1416);
	else
		stream("<TD></TD>");

	// supply
	/// @todo complete KML implement of supply (ticket #133)

	// demand
	if (gl_object_isa(obj,"load"))
	{
		load *pLoad = OBJECTDATA(obj,load);
		stream("<TR><TH ALIGN=LEFT>Load</TH>");
		if (has_phase(PHASE_A))
		{
			complex load_A = ~voltageA*pLoad->constant_current[0] + pLoad->powerA;
			stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kW&nbsp;&nbsp;<BR>%.3f&nbsp;kVAR</TD>",
				load_A.Re(),load_A.Im());
		}
		else
			stream("<TD></TD>");
		if (has_phase(PHASE_B))
		{
			complex load_B = ~voltageB*pLoad->constant_current[1] + pLoad->powerB;
			stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kW&nbsp;&nbsp;<BR>%.3f&nbsp;kVAR</TD>",
				load_B.Re(),load_B.Im());
		}
		else
			stream("<TD></TD>");
		if (has_phase(PHASE_C))
		{
			complex load_C = ~voltageC*pLoad->constant_current[2] + pLoad->powerC;
			stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kW&nbsp;&nbsp;<BR>%.3f&nbsp;kVAR</TD>",
				load_C.Re(),load_C.Im());
		}
		else
			stream("<TD></TD>");
	}
	stream("</TR>\n");
	stream("</TABLE>\n");
	stream("]]>\n");
	stream("</description>\n");
	stream("<Point>\n");
	stream("<coordinates>%f,%f</coordinates>\n",obj->longitude,obj->latitude);
	stream("</Point>\n");
	stream("</Placemark>\n");
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
					//See what the difference is - if it is above the convergence limit, send an NR update
					diff_val = voltage[0] - prev_voltage_value[0];

					if (diff_val.Mag() >= maximum_voltage_error)	//Outside of range, so force a new iteration
					{
						NR_retval = gl_globalclock;
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
					//See what the difference is - if it is above the convergence limit, send an NR update
					diff_val = voltage[1] - prev_voltage_value[1];

					if (diff_val.Mag() >= maximum_voltage_error)	//Outside of range, so force a new iteration
					{
						NR_retval = gl_globalclock;
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
					//See what the difference is - if it is above the convergence limit, send an NR update
					diff_val = voltage[2] - prev_voltage_value[2];

					if (diff_val.Mag() >= maximum_voltage_error)	//Outside of range, so force a new iteration
					{
						NR_retval = gl_globalclock;
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
	node *temp_par_node = NULL;

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

	//Populate explicit power
	NR_busdata[NR_node_reference].S_dy = &power_dy[0];

	//Populate explicit admittance
	NR_busdata[NR_node_reference].Y_dy = &shunt_dy[0];

	//Populate explicit current
	NR_busdata[NR_node_reference].I_dy = &current_dy[0];

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

	//Per unit values - populate nominal voltage on a whim
	NR_busdata[NR_node_reference].kv_base = nominal_voltage;
	NR_busdata[NR_node_reference].mva_base = -1.0;

	//Set the matrix value to -1 to know it hasn't been set (probably not necessary)
	NR_busdata[NR_node_reference].Matrix_Loc = -1;

	//Populate original phases
	NR_busdata[NR_node_reference].origphases = NR_busdata[NR_node_reference].phases;

	//Populate our tracking variable
	prev_phases = NR_busdata[NR_node_reference].phases;

	//Populate dynamic mode flag address
	NR_busdata[NR_node_reference].dynamics_enabled = &deltamode_inclusive;

	//Allocate full admittance matrix, if desired (for now, not) -- only if something has requested it
	if ((deltamode_inclusive==true) && (dynamic_norton==true))
	{
		//Check our status - shouldn't be necessary, but let's be paranoid
		if ((SubNode!=CHILD) && (SubNode!=DIFF_CHILD))	//We're stand-alone or a parent
		{
			//Make sure no pesky children have already allocated us
			if (full_Y == NULL)
			{
				//Allocate it
				full_Y = (complex *)gl_malloc(9*sizeof(complex));

				//Check it
				if (full_Y==NULL)
				{
					GL_THROW("Node:%s failed to allocate space for the a deltamode variable",me->name);
					/*  TROUBLESHOOT
					While attempting to allocate memory for a dynamics-required (deltamode) variable, an error
					occurred. Please try again.  If the error persists, please submit your code and a bug
					report via the trac website.
					*/
				}

				//Zero it, just to be safe (gens will accumulate into it)
				full_Y[0] = full_Y[1] = full_Y[2] = complex(0.0,0.0);
				full_Y[3] = full_Y[4] = full_Y[5] = complex(0.0,0.0);
				full_Y[6] = full_Y[7] = full_Y[8] = complex(0.0,0.0);

				//Allocate another matrix for admittance - this will have the full value
				full_Y_all = (complex *)gl_malloc(9*sizeof(complex));

				//Check it
				if (full_Y_all==NULL)
				{
					GL_THROW("Node:%s failed to allocate space for the a deltamode variable",me->name);
					//Defined above
				}

				//Zero it, just to be safe (gens will accumulate into it)
				full_Y_all[0] = full_Y_all[1] = full_Y_all[2] = complex(0.0,0.0);
				full_Y_all[3] = full_Y_all[4] = full_Y_all[5] = complex(0.0,0.0);
				full_Y_all[6] = full_Y_all[7] = full_Y_all[8] = complex(0.0,0.0);

				//Do the same for a dynamics contribution (just 4x1 for now for normal nodes)
				//0-2 represent ABC current,3 represents overall power, 4 represents power frequency weighting,
				//5 represents overall output power

				DynVariable = (complex *)gl_malloc(6*sizeof(complex));

				//Check it
				if (DynVariable==NULL)
				{
					GL_THROW("Node:%s failed to allocate space for the a deltamode variable",me->name);
					//Defined above
				}

				//Zero them, for consistency
				DynVariable[0] = DynVariable[1] = DynVariable[2] = complex(0.0,0.0);
				DynVariable[3] = DynVariable[4] = DynVariable[5] = complex(0.0,0.0);
			}
		}//End we're a parent

		//Map all relevant variables to the NR structure
		NR_busdata[NR_node_reference].full_Y = full_Y;
		NR_busdata[NR_node_reference].full_Y_all = full_Y_all;
		NR_busdata[NR_node_reference].DynCurrent = DynVariable;
		NR_busdata[NR_node_reference].PGenTotal = &DynVariable[3];
	}
	else	//Ensure it is empty
	{
		NR_busdata[NR_node_reference].full_Y = NULL;
		NR_busdata[NR_node_reference].full_Y_all = NULL;
		NR_busdata[NR_node_reference].DynCurrent = NULL;
		NR_busdata[NR_node_reference].PGenTotal = NULL;
	}

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
	int temp_result, loop_index;
	OBJECT *obj = OBJECTHDR(this);
	complex temp_current_inj[3];
	complex delta_shunt[3];
	complex delta_current[3];

	//Don't do anything if we've already been "updated"
	if (current_accumulated==false)
	{
		if (postpass)	//Perform what used to be in post-synch - used to childed object items - occurred on "false" pass, so goes before all
		{
			if (SubNode==CHILD)	//Remove child contributions
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

				//Remove power and "load" characteristics for explicit delta/wye values
				for (loop_index=0; loop_index<6; loop_index++)
				{
					ParToLoad->power_dy[loop_index] -= last_child_power_dy[loop_index][0];		//Power
					ParToLoad->shunt_dy[loop_index] -= last_child_power_dy[loop_index][1];		//Shunt
					ParToLoad->current_dy[loop_index] -= last_child_power_dy[loop_index][2];	//Current
				}

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

				//Zero the last power accumulators
				for (loop_index=0; loop_index<6; loop_index++)
				{
					last_child_power_dy[loop_index][0] = complex(0.0);	//Power
					last_child_power_dy[loop_index][1] = complex(0.0);	//Shunt
					last_child_power_dy[loop_index][2] = complex(0.0);	//Current
				}

				//Current 12 if we are triplex
				if (has_phase(PHASE_S))
					last_child_current12 = 0.0;
			}
			else if (SubNode==DIFF_CHILD)	//Differently connected 
			{
				node *ParToLoad = OBJECTDATA(SubNodeParent,node);

				if (!parentcall)	//We weren't called by our parent, so lock us to create sibling rivalry!
				{
					//Lock the parent for writing
					LOCK_OBJECT(SubNodeParent);
				}

				//Remove power and "load" characteristics for explicit delta/wye values
				for (loop_index=0; loop_index<6; loop_index++)
				{
					ParToLoad->power_dy[loop_index] -= last_child_power_dy[loop_index][0];		//Power
					ParToLoad->shunt_dy[loop_index] -= last_child_power_dy[loop_index][1];		//Shunt
					ParToLoad->current_dy[loop_index] -= last_child_power_dy[loop_index][2];	//Current
				}

				if (!parentcall)	//Wasn't a parent call - unlock us so our siblings get a shot
				{
					//Unlock the parent now that it is done
					UNLOCK_OBJECT(SubNodeParent);
				}

				//Zero the last power accumulators
				for (loop_index=0; loop_index<6; loop_index++)
				{
					last_child_power_dy[loop_index][0] = complex(0.0);	//Power
					last_child_power_dy[loop_index][1] = complex(0.0);	//Shunt
					last_child_power_dy[loop_index][2] = complex(0.0);	//Current
				}
			}

			if ((SubNode==CHILD) || (SubNode==DIFF_CHILD))	//Child Voltage Updates
			{
				node *ParStealLoad = OBJECTDATA(SubNodeParent,node);

				//Steal our paren't voltages as well
				//this will either be parent called or a child "no way it can change" rank read - no lock needed
				voltage[0] = ParStealLoad->voltage[0];
				voltage[1] = ParStealLoad->voltage[1];
				voltage[2] = ParStealLoad->voltage[2];

				//Compute "delta" voltages, so current injections upward are correct
				if (has_phase(PHASE_S))
				{
					//Compute the delta voltages
					voltaged[0] = voltage[0] + voltage[1];	//12
					voltaged[1] = voltage[1] - voltage[2];	//2N
					voltaged[2] = voltage[0] - voltage[2];	//1N -- unsure why it is odd
				}
				else	//"Normal" three phase - compute normal deltas
				{
					//Compute the delta voltages
					voltaged[0] = voltage[0] - voltage[1];
					voltaged[1] = voltage[1] - voltage[2];
					voltaged[2] = voltage[2] - voltage[0];
				}
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

		//Explicit delta-wye portions (do both) -- make sure not triplex though
		if (!(has_phase(PHASE_S)))
		{
			//Do delta-connected portions

			//Convert delta connected impedance
			delta_shunt[0] = voltaged[0]*shunt_dy[0];
			delta_shunt[1] = voltaged[1]*shunt_dy[1];
			delta_shunt[2] = voltaged[2]*shunt_dy[2];

			//Convert delta connected power
			delta_current[0]= (voltaged[0]==0) ? complex(0,0) : ~(power_dy[0]/voltaged[0]);
			delta_current[1]= (voltaged[1]==0) ? complex(0,0) : ~(power_dy[1]/voltaged[1]);
			delta_current[2]= (voltaged[2]==0) ? complex(0,0) : ~(power_dy[2]/voltaged[2]);

			//Put into accumulator
			temp_current_inj[0] += delta_shunt[0]-delta_shunt[2] + delta_current[0]-delta_current[2] + current_dy[0]-current_dy[2];
			temp_current_inj[1] += delta_shunt[1]-delta_shunt[0] + delta_current[1]-delta_current[0] + current_dy[1]-current_dy[0];
			temp_current_inj[2] += delta_shunt[2]-delta_shunt[1] + delta_current[2]-delta_current[1] + current_dy[2]-current_dy[1];

			//Now put in Wye components
			temp_current_inj[0] += ((voltage[0]==0) ? complex(0,0) : ~(power_dy[3]/voltage[0])) + voltage[0]*shunt_dy[3] + current_dy[3];
			temp_current_inj[1] += ((voltage[1]==0) ? complex(0,0) : ~(power_dy[4]/voltage[1])) + voltage[1]*shunt_dy[4] + current_dy[4];
			temp_current_inj[2] += ((voltage[2]==0) ? complex(0,0) : ~(power_dy[5]/voltage[2])) + voltage[2]*shunt_dy[5] + current_dy[5];
		}//End both delta/wye

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

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Module-level call
SIMULATIONMODE node::inter_deltaupdate_node(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	//unsigned char pass_mod;
	OBJECT *hdr = OBJECTHDR(this);

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Call presync-equivalent items
		NR_node_presync_fxn();

		//Call sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}
	else	//After the call
	{
		//Perform postsync-like updates on the values
		BOTH_node_postsync_fxn(hdr);

		//No control required at this time - powerflow defers to the whims of other modules
		//Code below implements predictor/corrector-type logic, even though it effectively does nothing
		return SM_EVENT;

		////Do deltamode-related logic
		//if (bustype==SWING)	//We're the SWING bus, control our destiny (which is really controlled elsewhere)
		//{
		//	//See what we're on
		//	pass_mod = iteration_count_val - ((iteration_count_val >> 1) << 1);

		//	//Check pass
		//	if (pass_mod==0)	//Predictor pass
		//	{
		//		return SM_DELTA_ITER;	//Reiterate - to get us to corrector pass
		//	}
		//	else	//Corrector pass
		//	{
		//		//As of right now, we're always ready to leave
		//		//Other objects will dictate if we stay (powerflow is indifferent)
		//		return SM_EVENT;
		//	}//End corrector pass
		//}//End SWING bus handling
		//else	//Normal bus
		//{
		//	return SM_EVENT;	//Normal nodes want event mode all the time here - SWING bus will
		//						//control the reiteration process for pred/corr steps
		//}
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF OTHER EXPORT FUNCTIONS
//////////////////////////////////////////////////////////////////////////
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

//Deltamode export
EXPORT SIMULATIONMODE interupdate_node(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	node *my = OBJECTDATA(obj,node);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_node(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_node(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

//Function to easily access complex bus deltamode values
//0 = full_Y - exposed bus admittance model
//1 - PGenTotal - total amount of generation on that bus (for current gen)
//2 - DeltaCurrents - currents calculated from updated powerflow solution
//3 - full_Y_all - exposed Ybus self admittance area
//4 - FreqPower - Frequeny-power weighting, for "nominal" update at end
//5 - TotalPower - Accumulated power, but not powerflow derived (differs from PGenTotal above)
EXPORT complex *delta_linkage(OBJECT *obj, unsigned char mapvar)
{
	complex *testval;
	node *my = OBJECTDATA(obj,node);

	if (mapvar==0)	//Bus admittance - full_Y
	{
		testval = my->full_Y;
	}
	else if (mapvar==1)	//Generation total - PGenTotal
	{
		testval = &(my->DynVariable[3]);
	}
	else if (mapvar==2)	//Dynamic current values - DeltaCurrents
	{
		testval = &(my->DynVariable[0]);
	}
	else if (mapvar==3)	//Self admittance, but fully Y-bus form
	{
		testval = my->full_Y_all;
	}
	else if (mapvar==4)	//Frequency-power variable
	{
		testval = &(my->DynVariable[4]);
	}
	else if (mapvar==5)	//Total power variable
	{
		testval = &(my->DynVariable[5]);
	}
	else	//Unknown - fail out
	{
		testval = NULL;
	}

	return testval;
}

//Function to extract and post the accumulated power and "frequency power" for
//updating "system frequency" when transiting back to standard mode
//Return SUCCESS/FAILED
EXPORT STATUS delta_frequency_node(OBJECT *obj, complex *powerval, complex *freqpowerval)
{
	//Map us
	node *my = OBJECTDATA(obj,node);

	//Null check to see if they even are needed
	if ((my->DynVariable != NULL) && ((my->SubNode!=CHILD) && (my->SubNode!=DIFF_CHILD)))
	{
		//Accumulate values
		*powerval += my->DynVariable[5];
		*freqpowerval += my->DynVariable[4];
	}
	//Default else - not there, so ignore us

	//Always succeed, for now
	return SUCCESS;
}

/**@}*/
