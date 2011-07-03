/** $Id: link.cpp 1211 2009-01-17 00:45:28Z d3x593 $
	Copyright (C) 2008 Battelle Memorial Institute
	@file link.cpp
	@addtogroup powerflow_link Link
	@ingroup powerflow_object

	@par Fault support

	The following conditions are used to describe a fault impedance \e X (e.g., 1e-6), 
	between phase \e x and neutral or group, or between phases \e x and \e y, and leaving
	phase \e z unaffected at iteration \e t:

	- \e phase-to-phase contact on the link
		- \e Forward-sweep: \f$ 
			V_x(t) = V_y(t) = 0, 
			A = \left( \begin{array}{ccc}
				0 & 0 & 0 \\
				0 & 0 & 0 \\
				0 & 0 & 1 \end{array} \right) , 
			B = \left( \begin{array}{ccc}
				0 & 0 & 0 \\
				0 & 0 & 0 \\
				0 & 0 & p \end{array} \right)  
			\f$
		  where \e p is the previous value
		  when \e x = phase A and \e y = phase B;
		- \e Back-sweep: \f$ 
			  c = \left( \begin{array}{ccc}
				0 & 0 & 0 \\
				0 & 0 & 0 \\
				0 & 0 & 0 \end{array} \right) ,
			  d = \left( \begin{array}{ccc}
				\frac{1}{X} & 0 & 0 \\
				0 & \frac{1}{X} & 0 \\
				0 & 0 & p \end{array} \right) 
		  \f$ when \e x = phase A and \e y = phase B;
    - \e phase-to-ground contact on the link
		- \e Forward-sweep: \f$ 
			V_x(t) = 0, 
			A = \left( \begin{array}{ccc}
				0 & 0 & 0 \\
				0 & 1 & 0 \\
				0 & 0 & 1 \end{array} \right) , 
			B = \left( \begin{array}{ccc}
				0 & 0 & 0 \\
				0 & p & p \\
				0 & p & p \end{array} \right)  
			\f$
		  where \e p is the previous value
		  when \e x = phase A;
		- \e Back-sweep: \f$ 
			  c = \left( \begin{array}{ccc}
				0 & 0 & 0 \\
				0 & 0 & 0 \\
				0 & 0 & 0 \end{array} \right) ,
			  d = \left( \begin{array}{ccc}
				\frac{1}{X} & 0 & 0 \\
				0 & p & 0 \\
				0 & 0 & p \end{array} \right) 
		  \f$ when \e x = phase A;
	- \e phase-to-neutral contact on the link
		- \e Forward-sweep: \f$ 
			V_x(t) = -V_N, 
			A = \left( \begin{array}{ccc}
				0 & 0 & 0 \\
				0 & 1 & 0 \\
				0 & 0 & 1 \end{array} \right) , 
			B = \left( \begin{array}{ccc}
				0 & 0 & 0 \\
				0 & p & p \\
				0 & p & p \end{array} \right)  
			\f$
		  where \e p is the previous value
		  when \e x = phase A;
		- \e Back-sweep: \f$ 
			  c = \left( \begin{array}{ccc}
				0 & 0 & 0 \\
				0 & 0 & 0 \\
				0 & 0 & 0 \end{array} \right) ,
			  d = \left( \begin{array}{ccc}
				\frac{1}{X} & 0 & 0 \\
				0 & p & 0 \\
				0 & 0 & p \end{array} \right) 
		  \f$ when \e x = phase A;


	@{
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "link.h"
#include "node.h"
#include "restoration.h"

CLASS* link::oclass = NULL;
CLASS* link::pclass = NULL;

/**
* constructor.  Class registration is only called once to register the class with the core.
* @param mod the module struct that this class is registering in
*/
link::link(MODULE *mod) : powerflow_object(mod)
{
	// first time init
	if (oclass==NULL)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod,"link",sizeof(link),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if (oclass==NULL)
			throw "unable to register class link";
		else
			oclass->trl = TRL_PROVEN;
		
		if(gl_publish_variable(oclass,
			PT_INHERIT, "powerflow_object",
			PT_enumeration, "status", PADDR(status),
				PT_KEYWORD, "CLOSED", LS_CLOSED,
				PT_KEYWORD, "OPEN", LS_OPEN,
			PT_object, "from",PADDR(from),
			PT_object, "to", PADDR(to),
			PT_complex, "power_in[VA]", PADDR(power_in),
			PT_complex, "power_out[VA]", PADDR(power_out),
			PT_double, "power_out_real[W]", PADDR(power_out.Re()),
			PT_complex, "power_losses[VA]", PADDR(power_loss),
			PT_complex, "power_in_A[VA]", PADDR(indiv_power_in[0]),
			PT_complex, "power_in_B[VA]", PADDR(indiv_power_in[1]),
			PT_complex, "power_in_C[VA]", PADDR(indiv_power_in[2]),
			PT_complex, "power_out_A[VA]", PADDR(indiv_power_out[0]),
			PT_complex, "power_out_B[VA]", PADDR(indiv_power_out[1]),
			PT_complex, "power_out_C[VA]", PADDR(indiv_power_out[2]),
			PT_complex, "power_losses_A[VA]", PADDR(indiv_power_loss[0]),
			PT_complex, "power_losses_B[VA]", PADDR(indiv_power_loss[1]),
			PT_complex, "power_losses_C[VA]", PADDR(indiv_power_loss[2]),
			PT_complex, "current_out_A[A]", PADDR(read_I_out[0]),
			PT_complex, "current_out_B[A]", PADDR(read_I_out[1]),
			PT_complex, "current_out_C[A]", PADDR(read_I_out[2]),
			PT_complex, "current_in_A[A]", PADDR(read_I_in[0]),
			PT_complex, "current_in_B[A]", PADDR(read_I_in[1]),
			PT_complex, "current_in_C[A]", PADDR(read_I_in[2]),
			PT_complex, "fault_current_in_A[A]", PADDR(If_in[0]),
			PT_complex, "fault_current_in_B[A]", PADDR(If_in[1]),
			PT_complex, "fault_current_in_C[A]", PADDR(If_in[2]),
			PT_complex, "fault_current_out_A[A]", PADDR(If_out[0]),
			PT_complex, "fault_current_out_B[A]", PADDR(If_out[1]),
			PT_complex, "fault_current_out_C[A]", PADDR(If_out[2]),
			PT_set, "flow_direction", PADDR(flow_direction),
				PT_KEYWORD, "UNKNOWN", (set)FD_UNKNOWN,
				PT_KEYWORD, "AF", (set)FD_A_NORMAL,
				PT_KEYWORD, "AR", (set)FD_A_REVERSE,
				PT_KEYWORD, "AN", (set)FD_A_NONE,
				PT_KEYWORD, "BF", (set)FD_B_NORMAL,
				PT_KEYWORD, "BR", (set)FD_B_REVERSE,
				PT_KEYWORD, "BN", (set)FD_B_NONE,
				PT_KEYWORD, "CF", (set)FD_C_NORMAL,
				PT_KEYWORD, "CR", (set)FD_C_REVERSE,
				PT_KEYWORD, "CN", (set)FD_C_NONE,
			PT_double, "mean_repair_time[s]",PADDR(mean_repair_time), PT_DESCRIPTION, "Time after a fault clears for the object to be back in service",
			NULL) < 1 && errno) GL_THROW("unable to publish link properties in %s",__FILE__);
	}
}

int link::isa(char *classname)
{
	return strcmp(classname,"link")==0 || powerflow_object::isa(classname);
}

int link::create(void)
{
	int result = powerflow_object::create();

#ifdef SUPPORT_OUTAGES
	condition=OC_NORMAL;
#endif
	
	from = NULL;
	to = NULL;
	status = LS_CLOSED;
	prev_status = LS_OPEN;	//Set different to status so it performs a calculation on the first run
	power_in = 0;
	power_out = 0;
	power_loss = 0;
	indiv_power_in[0] = indiv_power_in[1] = indiv_power_in[2] = 0.0;
	indiv_power_out[0] = indiv_power_out[1] = indiv_power_out[2] = 0.0;
	indiv_power_loss[0] = indiv_power_loss[1] = indiv_power_loss[2] = 0.0;
	flow_direction = FD_UNKNOWN;
	voltage_ratio = 1.0;
	SpecialLnk = NORMAL;
	prev_LTime=0;
	NR_branch_reference=-1;
	If_in[0] = If_in[1] = If_in[2] = complex(0,0);
	If_out[0] = If_out[1] = If_out[2] = complex(0,0);

	protect_locations[0] = protect_locations[1] = protect_locations[2] = 0;

	current_in[0] = current_in[1] = current_in[2] = complex(0,0);

	mean_repair_time = 0.0;

	return result;
}

int link::init(OBJECT *parent)
{
	OBJECT *obj = GETOBJECT(this);

	powerflow_object::init(parent);

	set phase_f_test, phase_t_test, phases_test;
	node *fNode = OBJECTDATA(from,node);
	node *tNode = OBJECTDATA(to,node);

	/* check link from node */
	if (from==NULL)
		throw "link from node is not specified";
		/*  TROUBLESHOOT
		The from node for a line or link is not connected to anything.
		*/
	if (to==NULL)
		throw "link to node is not specified";
		/*  TROUBLESHOOT
		The to node for a line or link is not connected to anything.
		*/
	
	if (mean_repair_time < 0.0)
	{
		gl_warning("link:%s has a negative mean_repair_time, set to 1 hour",obj->name);
		/*  TROUBLESHOOT
		A link object had a mean_repair_time that is negative.  This ia not a valid setting.
		The value has been changed to 0.  Please set the variable to an appropriate variable
		*/

		mean_repair_time = 0.0;	//Set to zero by default
	}

	/* adjust ranks according to method in use */
	switch (solver_method) {
	case SM_FBS: /* forward backsweep method only */
		if (obj->parent==NULL)
		{
			/* make 'from' object parent of this object */
			if (gl_object_isa(from,"node")) 
			{
				if(gl_set_parent(obj, from) < 0)
					throw "error when setting parent";
					/*  TROUBLESHOOT
					An error has occurred while setting the parent field of a link.  Please
					submit a bug report and your code so this error can be diagnosed further.
					*/
			} 
			else 
				throw "link from reference not a node";
				/*  TROUBLESHOOT
				The from connection of a link is not a node-based object.  Ensure that the from object is a
				node, load, or meter object.  If not, adjust your model accordingly.
				*/
		}
		else
			/* promote 'from' object if necessary */
			gl_set_rank(from,obj->rank+1);
		
		if (to->parent==NULL)
		{
			/* make this object parent to 'to' object */
			if (gl_object_isa(to,"node"))
			{
				if(gl_set_parent(to, obj) < 0)
					throw "error when setting parent";
					//Defined above
			} 
			else 
				throw "link to reference not a node";
				//Defined above
		}
		else
			/* promote this object if necessary */
			gl_set_rank(obj,to->rank+1);

		break;
	case SM_GS: /* Gauss-Seidel */
		{
		if (obj->parent==NULL) 
			/* automatically use from as parent node */
			obj->parent = from;

		node *fNode = OBJECTDATA(from,node);
		node *tNode = OBJECTDATA(to,node);
		
		if (fNode!=NULL)
			fNode->attachlink(OBJECTHDR(this));

		if (tNode!=NULL)
			tNode->attachlink(OBJECTHDR(this));

		break;
		}
	case SM_NR:
	{
		NR_branch_count++;		//Update global value of link count

		gl_set_rank(obj,1);	//Links go to rank 1

		//Link to the tn constants if we are a triplex (parent/child relationship gets lost)
		//Also link other end of line to from, so we can steal its currents later
		if (gl_object_isa(obj,"triplex_line","powerflow"))
		{
			node *tnode = OBJECTDATA(to,node);

			tnode->Triplex_Data=&tn[0];
		}

		//Increment connected links ends for nodes for creating their "link list"
		if ((fNode->SubNode == CHILD) || (fNode->SubNode == DIFF_CHILD))
		{
			//Parent/child connected node, so increment our parent's counter
			node *pfNode = OBJECTDATA(fNode->SubNodeParent,node);
			pfNode->NR_connected_links[0]++;
		}
		else
		{
			fNode->NR_connected_links[0]++;
		}

		if ((tNode->SubNode == CHILD) || (tNode->SubNode == DIFF_CHILD))
		{
			//Parent/child connected node, so increment our parent's counter
			node *ptNode = OBJECTDATA(tNode->SubNodeParent,node);
			ptNode->NR_connected_links[0]++;
		}
		else
		{
			tNode->NR_connected_links[0]++;
		}

		break;
	}
	default:
		throw "unsupported solver method";
		//Defined elsewhere
		break;
	}

	//Simple Phase checks
	phases_test = (phases & (~(PHASE_N | PHASE_D)));	//N and D phases are stripped off for now.  Redundant to D-Gwye tests.
	phase_f_test = (fNode->phases & phases_test);		//Will need to be handled differently if in the future explicit N/G phases are introduced.
	phase_t_test = (tNode->phases & phases_test);	
																		
	if ((SpecialLnk==DELTAGWYE) | (SpecialLnk==SPLITPHASE) | (SpecialLnk==SWITCH))	//Delta-Gwye and Split-phase transformers will cause problems, handle special.
	{																				//Switches/fuses have the potential, so put them in here too.
		if (SpecialLnk==SPLITPHASE)
		{
			phase_f_test &= ~(PHASE_S);	//Pull off the single phase portion of from node

			if ((phase_f_test != (phases_test & ~(PHASE_S))) || (phase_t_test != phases_test))	//Phase mismatch on the line
				GL_THROW("line:%d - %s has a phase mismatch at one or both ends",obj->id,obj->name);
				/*  TROUBLESHOOT
				A line has been configured to carry a certain set of phases.  Either the input node or output
				node is not providing a source/sink for these different conductors.  The To and From nodes must
				have at least the phases of the line connecting them.
				*/
		}
		else if (SpecialLnk==DELTAGWYE)	//D-Gwye then
		{
			phase_t_test &= ~(PHASE_N | PHASE_D);	//Pull off the neutral and Delta phase portion of from node (no idea if ABCD or ABCN would be convention)
			phase_f_test &= ~(PHASE_N | PHASE_D);	//Pull off the neutral and Delta phase portion of to node

			if ((phase_f_test != (phases & ~(PHASE_N | PHASE_D))) || (phase_t_test != (phases & ~(PHASE_N | PHASE_D))))	//Phase mismatch on the line
				GL_THROW("line:%d - %s has a phase mismatch at one or both ends",obj->id,obj->name);
				//Defined above
		}
		else	//Must be a switch then
		{
			//Do an initial check just to make sure the connection is physically possible
			if ((phase_f_test != phases_test) || (phase_t_test != phases_test))	//Phase mismatch on the line
				GL_THROW("switch:%d - %s has a phase mismatch at one or both ends",obj->id,obj->name);
				//Defined above

			//Now only update if we are closed
			if (status==LS_CLOSED)
			{
				//Set up the phase test on the to node to make sure all are hit (only do to node)
				tNode->busphasesIn |= phases_test;
				fNode->busphasesOut |= phases_test;
			}
		}
	}
	else												//Everything else
	{
		if ((phase_f_test != phases_test) || (phase_t_test != phases_test))	//Phase mismatch on the line
			GL_THROW("line:%d - %s has a phase mismatch at one or both ends",obj->id,obj->name);
			//Defined above

		//Set up the phase test on the to node to make sure all are hit (only do to node)
		tNode->busphasesIn |= phases_test;
		fNode->busphasesOut |= phases_test;
	}
	
	if (nominal_voltage==0)
	{
		node *pFrom = OBJECTDATA(from,node);
		nominal_voltage = pFrom->nominal_voltage;
	}

	/* no nominal voltage */
	if (nominal_voltage==0)
		throw "nominal voltage is not specified";

	/* record this link on the nodes' incidence counts */
	OBJECTDATA(from,node)->k++;
	OBJECTDATA(to,node)->k++;
	return 1;
}

node *link::get_from(void) const
{
	node *from;
	get_flow(&from,NULL);
	return from;
}
node *link::get_to(void) const
{
	node *to;
	get_flow(NULL,&to);
	return to;
}
set link::get_flow(node **fn, node **tn) const
{
	node *f = OBJECTDATA(from, node);
	node *t = OBJECTDATA(to, node);
	set reverse = 0;
	reverse |= (f->voltage[0].Mag()<t->voltage[0].Mag())?PHASE_A:0;
	reverse |= (f->voltage[1].Mag()<t->voltage[1].Mag())?PHASE_B:0;
	reverse |= (f->voltage[2].Mag()<t->voltage[2].Mag())?PHASE_C:0;
	if (fn!=NULL) *fn=f;
	if (tn!=NULL) *tn=t;
	/* someday perhaps, reversal will be supported */
	return reverse;
}

TIMESTAMP link::presync(TIMESTAMP t0)
{
	TIMESTAMP t1 = powerflow_object::presync(t0); 

	if (solver_method==SM_NR)
	{
		if (prev_LTime==0)	//First run, build up the pointer matrices
		{
			node *fnode = OBJECTDATA(from,node);
			node *tnode = OBJECTDATA(to,node);
			unsigned int *LinkTableLoc = NULL;
			unsigned int TempTableIndex;
			unsigned char working_phase;
			char *temp_phase;
			int IndVal = 0;
			OBJECT *obj = OBJECTHDR(this);

			if (fnode==NULL || tnode==NULL)
				return TS_NEVER;

			if ((NR_curr_bus!=-1) && (NR_curr_branch!=-1))	//Ensure we've been initialized
			{
				//Lock fnode so multi-links don't have issues
				LOCK_OBJECT(from);

				//Check our end nodes first - update them if necessary
				if (fnode->NR_node_reference==-1)	//Uninitialized node
				{
					fnode->NR_populate();
				}
				else if (fnode->NR_node_reference==-99)	//Child node
				{
					node *ParFromNode = OBJECTDATA(fnode->SubNodeParent,node);

					if (ParFromNode->NR_node_reference==-1)	//Uninitialized node
					{
						ParFromNode->NR_populate();
					}
				}

				//Unlock the from node
				UNLOCK_OBJECT(from);

				//Lock To object for multi-links
				LOCK_OBJECT(to);

				if (tnode->NR_node_reference==-1)	//Unitialized node
				{
					tnode->NR_populate();
				}
				else if (tnode->NR_node_reference==-99)	//Child node
				{
					node *ParToNode = OBJECTDATA(tnode->SubNodeParent,node);

					if (ParToNode->NR_node_reference==-1)	//Uninitialized node
					{
						ParToNode->NR_populate();
					}
				}

				//Unlock to node
				UNLOCK_OBJECT(to);

				//Now populate the link's information
				//Lock the Swing bus and get us a unique array location
				LOCK_OBJECT(NR_swing_bus);

				NR_branch_reference = NR_curr_branch;	//Get an index and store it as our own
				NR_curr_branch++;					//Increment it for next one

				UNLOCK_OBJECT(NR_swing_bus);	//Release the swing bus for others

				//See if we worked - not sure how it would get here otherwise, but meh
				if (NR_branch_reference == -1)
				{
					GL_THROW("NR: branch:%s failed to grab a unique bus index value!",obj->name);
					/*  TROUBLESHOOT
					While attempting to gain a unique branch id for the Newton-Raphson solver, an error
					was encountered.  This may be related to a parallelization effort.  Please try again.
					If the error persists, please submit your code and a bug report via the trac website.
					*/
				}


				//Start with admittance matrix
				if ((voltage_ratio != 1.0) || (SpecialLnk!=NORMAL))	//Transformer, send more - may not need all 4, but put them there anyways
				{
					//See if we're a switch (if so, we don't need all the hoopla)
					if (SpecialLnk==SWITCH)	//Just like normal lines
					{
						NR_branchdata[NR_branch_reference].Yfrom = &From_Y[0][0];
						NR_branchdata[NR_branch_reference].Yto = &From_Y[0][0];
						NR_branchdata[NR_branch_reference].YSfrom = &From_Y[0][0];
						NR_branchdata[NR_branch_reference].YSto = &From_Y[0][0];

						//Populate the status variable while we are in here
						NR_branchdata[NR_branch_reference].status = &status;
					}
					else
					{
						//Create them
						YSfrom = (complex *)gl_malloc(9*sizeof(complex));
						if (YSfrom == NULL)
							GL_THROW("NR: Memory allocation failure for transformer matrices.");
							/*  TROUBLESHOOT
							This is a bug.  Newton-Raphson tries to allocate memory for two other
							needed matrices when dealing with transformers.  This failed.  Please submit
							your code and a bug report on the trac site.
							*/

						YSto = (complex *)gl_malloc(9*sizeof(complex));
						if (YSto == NULL)
							GL_THROW("NR: Memory allocation failure for transformer matrices.");
							//defined above

						NR_branchdata[NR_branch_reference].Yfrom = &From_Y[0][0];
						NR_branchdata[NR_branch_reference].Yto = &To_Y[0][0];
						NR_branchdata[NR_branch_reference].YSfrom = YSfrom;
						NR_branchdata[NR_branch_reference].YSto = YSto;
					}
				}
				else						//Simple line, they are all the same anyways
				{
					NR_branchdata[NR_branch_reference].Yfrom = &From_Y[0][0];
					NR_branchdata[NR_branch_reference].Yto = &From_Y[0][0];
					NR_branchdata[NR_branch_reference].YSfrom = &From_Y[0][0];
					NR_branchdata[NR_branch_reference].YSto = &From_Y[0][0];
				}

				//Link the name
				NR_branchdata[NR_branch_reference].name = obj->name;

				//Populate phases property
				NR_branchdata[NR_branch_reference].phases = 128*has_phase(PHASE_S) + 4*has_phase(PHASE_A) + 2*has_phase(PHASE_B) + has_phase(PHASE_C);
				
				//Populate original phases property
				NR_branchdata[NR_branch_reference].origphases = NR_branchdata[NR_branch_reference].phases;

				//Zero fault phases - presumably nothing is broken right now
				NR_branchdata[NR_branch_reference].faultphases = 0x00;
				
				//link If_in and If_out
				NR_branchdata[NR_branch_reference].If_from = &If_in[0];
				NR_branchdata[NR_branch_reference].If_to = &If_out[0];

				if (SpecialLnk == SWITCH)	//If we're a fuse or switch, make sure our "open" phases are correct
				{
					working_phase = 0xF0;	//Start with mask for all USB

					//Update initial stati as necessary as well - do for fuses and switches (both encoded the same SpecialLnk)
					if (gl_object_isa(obj,"switch","powerflow"))
					{
						temp_phase = (char*)GETADDR(obj,gl_get_property(obj,"phase_A_state"));

						if (*temp_phase == 1)
							working_phase |= 0x04;

						temp_phase = (char*)GETADDR(obj,gl_get_property(obj,"phase_B_state"));

						if (*temp_phase == 1)
							working_phase |= 0x02;

						temp_phase = (char*)GETADDR(obj,gl_get_property(obj,"phase_C_state"));

						if (*temp_phase == 1)
							working_phase |= 0x01;
					}
					else if (gl_object_isa(obj,"fuse","powerflow"))
					{
						temp_phase = (char*)GETADDR(obj,gl_get_property(obj,"phase_A_status"));

						if (*temp_phase == 0)
							working_phase |= 0x04;

						temp_phase = (char*)GETADDR(obj,gl_get_property(obj,"phase_B_status"));

						if (*temp_phase == 0)
							working_phase |= 0x02;

						temp_phase = (char*)GETADDR(obj,gl_get_property(obj,"phase_C_status"));

						if (*temp_phase == 0)
							working_phase |= 0x01;
					}
					else	//Not sure how we'll get here, just make normal phase
						working_phase |= 0x0F;

					//Update phase value
					NR_branchdata[NR_branch_reference].phases &= working_phase;
				}

				//If we're a SPCT transformer, add in the "special" phase flag for our to node
				if (SpecialLnk==SPLITPHASE)
				{
					//Set the branch phases
					NR_branchdata[NR_branch_reference].phases |= 0x20;

					if (tnode->NR_node_reference == -99)	//To node is a child
					{
						//Set the To node phases
						//Lock the to node
						LOCK_OBJECT(tnode->SubNodeParent);

						NR_busdata[*tnode->NR_subnode_reference].phases |= 0x20;
						NR_busdata[*tnode->NR_subnode_reference].origphases |= 0x20;	//Make sure reliability gets updated right too!

						//Unlock to node
						UNLOCK_OBJECT(tnode->SubNodeParent);

					}
					else	//Must be a "normal" node
					{
						//Set the To node phases
						//Lock the to node
						LOCK_OBJECT(to);

						NR_busdata[tnode->NR_node_reference].phases |= 0x20;
						NR_busdata[tnode->NR_node_reference].origphases |= 0x20;	//Make sure reliability gets updated right too!

						//Unlock to node
						UNLOCK_OBJECT(to);
					}
				}

				//Populate to/from indices
				if (fnode->NR_node_reference == -99)	//Child node
				{
					NR_branchdata[NR_branch_reference].from = *fnode->NR_subnode_reference;	//Index value

					//Pull out index
					IndVal = *fnode->NR_subnode_reference;

					//Get the parent information
					node *parFnode = OBJECTDATA(fnode->SubNodeParent,node);
					LinkTableLoc = parFnode->NR_connected_links;

					//Lock the parent object while we steal location information, and give ourselves a unique value
					LOCK_OBJECT(fnode->SubNodeParent);
					
					TempTableIndex = LinkTableLoc[1];	//Get our index in the table
					LinkTableLoc[1]++;					//Increment the index for the next guy

					//Unlock us now
					UNLOCK_OBJECT(fnode->SubNodeParent);
				}
				else
				{
					NR_branchdata[NR_branch_reference].from = fnode->NR_node_reference;	//From reference
					IndVal = fnode->NR_node_reference;								//Find the FROM busdata index
					LinkTableLoc = fnode->NR_connected_links;						//Locate the counter table

					//Lock the from object while we steal location information, and give ourselves a unique value
					LOCK_OBJECT(from);
					
					TempTableIndex = LinkTableLoc[1];	//Get our index in the table
					LinkTableLoc[1]++;					//Increment the index for the next guy

					//Unlock us now
					UNLOCK_OBJECT(from);
				}

				//If we are OK, populate the list entry
				if (TempTableIndex >= LinkTableLoc[0])	//Update for intermediate value
				{
					GL_THROW("NR: An extra link tried to connected to node %s",NR_busdata[IndVal].name);
					/*  TROUBLESHOOT
					During the initialization state, a link tried to connect to a node
					that's link list is already full.  This is a bug.  Submit your code and
					a bug report using the trac website
					*/
				}
				else					//We're OK - populate in our parent's list
				{
					NR_busdata[IndVal].Link_Table[TempTableIndex] = NR_branch_reference;	//Populate that value
				}

				if (tnode->NR_node_reference == -99)	//Child node
				{
					NR_branchdata[NR_branch_reference].to = *tnode->NR_subnode_reference;

					//Pull out index
					IndVal = *tnode->NR_subnode_reference;

					//Get the parent information
					node *parTnode = OBJECTDATA(tnode->SubNodeParent,node);
					LinkTableLoc = parTnode->NR_connected_links;

					//Lock the parent object while we steal location information, and give ourselves a unique value
					LOCK_OBJECT(tnode->SubNodeParent);
					
					TempTableIndex = LinkTableLoc[1];	//Get our index in the table
					LinkTableLoc[1]++;					//Increment the index for the next guy

					//Unlock us now
					UNLOCK_OBJECT(tnode->SubNodeParent);
				}
				else
				{
					NR_branchdata[NR_branch_reference].to = tnode->NR_node_reference;	//To reference
					IndVal = tnode->NR_node_reference;								//Find the TO busdata index
					LinkTableLoc = tnode->NR_connected_links;						//Locate the counter table

					//Lock the to object while we steal location information, and give ourselves a unique value
					LOCK_OBJECT(to);
					
					TempTableIndex = LinkTableLoc[1];	//Get our index in the table
					LinkTableLoc[1]++;					//Increment the index for the next guy

					//Unlock us now
					UNLOCK_OBJECT(to);
				}

				//If we are OK, populate the list entry
				if (TempTableIndex >= LinkTableLoc[0])
				{
					GL_THROW("NR: An extra link tried to connected to node %s",NR_busdata[IndVal].name);
					//Defined above
				}
				else					//We're OK - populate in our parent's list
				{
					NR_busdata[IndVal].Link_Table[TempTableIndex] = NR_branch_reference;	//Populate that value
				}

				//Populate voltage ratio
				NR_branchdata[NR_branch_reference].v_ratio = voltage_ratio;

				//Populate the connectivity matrix with our to/from/type information if restoration exists
				if (restoration_object != NULL)
				{
					restoration *Rest_Object = OBJECTDATA(restoration_object,restoration);
					Rest_Object->PopulateConnectivity(NR_branchdata[NR_branch_reference].from,NR_branchdata[NR_branch_reference].to,obj);
				}
			}
			else
			{
				GL_THROW("A link was called before NR was initialized by a node.");
				/*	TROUBLESHOOT
				This is a bug.  The Newton-Raphson solver method relies on a node being called first.  If GridLAB-D
				made it this far, you should have a swing bus defined and it should be called before any other objects.
				Please submit your code and a bug report for this problem.
				*/
			}

			//Figure out what type of link we are and populate accordingly
			if ((gl_object_isa(obj,"transformer","powerflow")) || (gl_object_isa(obj,"regulator","powerflow")))	//Tranformer check
			{
				NR_branchdata[NR_branch_reference].lnk_type = 4;
			}
			else	//Normal-ish
			{
				if (has_phase(PHASE_S))	//Triplex
				{
					NR_branchdata[NR_branch_reference].lnk_type = 1;
				}
				else if (gl_object_isa(obj,"recloser","powerflow"))	//Recloser - do before switch since a recloser IS a switch
				{
					NR_branchdata[NR_branch_reference].lnk_type = 6;
				}
				else if (gl_object_isa(obj,"sectionalizer","powerflow"))	//Sectionalizer - do before switch since a sectionalizer IS a switch
				{
					NR_branchdata[NR_branch_reference].lnk_type = 5;
				}
				else if (gl_object_isa(obj,"switch","powerflow"))	//We're a switch
				{
					NR_branchdata[NR_branch_reference].lnk_type = 2;
				}
				else if (gl_object_isa(obj,"fuse","powerflow"))
				{
					NR_branchdata[NR_branch_reference].lnk_type = 3;
				}
				else	//Must be a plain old ugly overhead or underground line
				{
					NR_branchdata[NR_branch_reference].lnk_type = 0;
				}
			}//end link type population
		}//End init loop

		if (status != prev_status)	//Something's changed, update us
		{
			complex Ylinecharge[3][3];
			complex Y[3][3];
			complex Yc[3][3];
			complex Ylefttemp[3][3];
			complex Yto[3][3];
			complex Yfrom[3][3];
			double invratio;
			char jindex, kindex;

			//Create initial admittance matrix - use code from GS below - store in From_Y (for now)
			for (jindex=0; jindex<3; jindex++)
				for (kindex=0; kindex<3; kindex++)
					Y[jindex][kindex] = 0.0;
			

			// compute admittance - invert b matrix - special circumstances given different methods
			if ((SpecialLnk==SWITCH) || (SpecialLnk==REGULATOR))
			{
				;	//Just skip over all of this nonsense
			}
			else if (has_phase(PHASE_S)) //Triplexy
			{
				//Put it straight in
				equalm(b_mat,Y);
			}
			else if (has_phase(PHASE_A) && !has_phase(PHASE_B) && !has_phase(PHASE_C)) //only A
				Y[0][0] = complex(1.0) / b_mat[0][0];
			else if (!has_phase(PHASE_A) && has_phase(PHASE_B) && !has_phase(PHASE_C)) //only B
				Y[1][1] = complex(1.0) / b_mat[1][1];
			else if (!has_phase(PHASE_A) && !has_phase(PHASE_B) && has_phase(PHASE_C)) //only C
				Y[2][2] = complex(1.0) / b_mat[2][2];
			else if (has_phase(PHASE_A) && !has_phase(PHASE_B) && has_phase(PHASE_C)) //has A & C
			{
				complex detvalue = b_mat[0][0]*b_mat[2][2] - b_mat[0][2]*b_mat[2][0];

				Y[0][0] = b_mat[2][2] / detvalue;
				Y[0][2] = b_mat[0][2] * -1.0 / detvalue;
				Y[2][0] = b_mat[2][0] * -1.0 / detvalue;
				Y[2][2] = b_mat[0][0] / detvalue;
			}
			else if (has_phase(PHASE_A) && has_phase(PHASE_B) && !has_phase(PHASE_C)) //has A & B
			{
				complex detvalue = b_mat[0][0]*b_mat[1][1] - b_mat[0][1]*b_mat[1][0];

				Y[0][0] = b_mat[1][1] / detvalue;
				Y[0][1] = b_mat[0][1] * -1.0 / detvalue;
				Y[1][0] = b_mat[1][0] * -1.0 / detvalue;
				Y[1][1] = b_mat[0][0] / detvalue;
			}
			else if (!has_phase(PHASE_A) && has_phase(PHASE_B) && has_phase(PHASE_C))	//has B & C
			{
				complex detvalue = b_mat[1][1]*b_mat[2][2] - b_mat[1][2]*b_mat[2][1];

				Y[1][1] = b_mat[2][2] / detvalue;
				Y[1][2] = b_mat[1][2] * -1.0 / detvalue;
				Y[2][1] = b_mat[2][1] * -1.0 / detvalue;
				Y[2][2] = b_mat[1][1] / detvalue;
			}
			else if ((has_phase(PHASE_A) && has_phase(PHASE_B) && has_phase(PHASE_C)) || (has_phase(PHASE_D))) //has ABC or D (D=ABC)
				inverse(b_mat,Y);
			// defaulted else - No phases (e.g., the line does not exist) - just = 0

			if ((voltage_ratio!=1) | (SpecialLnk!=NORMAL))	//Handle transformers slightly different
			{
				invratio=1.0/voltage_ratio;

				if (SpecialLnk==DELTAGWYE)	//Delta-Gwye implementation
				{
					complex tempImped;

					//Pre-admittancized matrix
					equalm(b_mat,Yto);

					//Store value into YSto
					for (jindex=0; jindex<3; jindex++)
					{
						for (kindex=0; kindex<3; kindex++)
						{
							YSto[jindex*3+kindex]=Yto[jindex][kindex];
						}
					}

					//Adjust for To_Y
					multiply(Yto,c_mat,To_Y);

					//Scale to other size
					multiply(invratio,Yto,Ylefttemp);
					multiply(invratio,Ylefttemp,Yfrom);

					//Store value into YSfrom
					for (jindex=0; jindex<3; jindex++)
					{
						for (kindex=0; kindex<3; kindex++)
						{
							YSfrom[jindex*3+kindex]=Yfrom[jindex][kindex];
						}
					}

					//Adjust for From_Y
					multiply(B_mat,Yto,From_Y);
				}
				else if (SpecialLnk==SPLITPHASE)	//Split phase
				{
					//Yto - same for all
					YSto[0] = b_mat[0][0];
					YSto[1] = b_mat[0][1];
					YSto[3] = b_mat[1][0];
					YSto[4] = b_mat[1][1];
					YSto[2] = YSto[5] = YSto[6] = YSto[7] = YSto[8] = 0.0;

					if (has_phase(PHASE_A))		//A connected
					{
						//To_Y
						To_Y[0][0] = -b_mat[0][2];
						To_Y[1][0] = -b_mat[1][2];
						To_Y[0][1] = To_Y[0][2] = To_Y[1][1] = 0.0;
						To_Y[1][2] = To_Y[2][0] = To_Y[2][1] = To_Y[2][2] = 0.0;

						//Yfrom
						YSfrom[0] = b_mat[2][2];
						YSfrom[1] = YSfrom[2] = YSfrom[3] = YSfrom[4] = 0.0;
						YSfrom[5] = YSfrom[6] = YSfrom[7] = YSfrom[8] = 0.0;

						//From_Y
						From_Y[0][0] = -b_mat[2][0];
						From_Y[0][1] = -b_mat[2][1];
						From_Y[0][2] = From_Y[1][0] = From_Y[1][1] = 0.0;
						From_Y[1][2] = From_Y[2][0] = From_Y[2][1] = From_Y[2][2] = 0.0;
					}
					else if (has_phase(PHASE_B))	//B connected
					{
						//To_Y
						To_Y[0][1] = -b_mat[0][2];
						To_Y[1][1] = -b_mat[1][2];
						To_Y[0][0] = To_Y[0][2] = To_Y[1][0] = 0.0;
						To_Y[1][2] = To_Y[2][0] = To_Y[2][1] = To_Y[2][2] = 0.0;

						//Yfrom
						YSfrom[4] = b_mat[2][2];
						YSfrom[0] = YSfrom[1] = YSfrom[2] = YSfrom[3] = 0.0;
						YSfrom[5] = YSfrom[6] = YSfrom[7] = YSfrom[8] = 0.0;

						//From_Y
						From_Y[1][0] = -b_mat[2][0];
						From_Y[1][1] = -b_mat[2][1];
						From_Y[0][0] = From_Y[0][1] = From_Y[0][2] = 0.0;
						From_Y[1][2] = From_Y[2][0] = From_Y[2][1] = From_Y[2][2] = 0.0;
					}
					else if (has_phase(PHASE_C))	//C connected
					{
						//To_Y
						To_Y[0][2] = -b_mat[0][2];
						To_Y[1][2] = -b_mat[1][2];
						To_Y[0][0] = To_Y[0][1] = To_Y[1][0] = 0.0;
						To_Y[1][1] = To_Y[2][0] = To_Y[2][1] = To_Y[2][2] = 0.0;

						//Yfrom
						YSfrom[8] = b_mat[2][2];
						YSfrom[0] = YSfrom[1] = YSfrom[2] = YSfrom[3] = 0.0;
						YSfrom[4] = YSfrom[5] = YSfrom[6] = YSfrom[7] = 0.0;

						//From_Y
						From_Y[2][0] = -b_mat[2][0];
						From_Y[2][1] = -b_mat[2][1];
						From_Y[0][0] = From_Y[0][1] = From_Y[0][2] = 0.0;
						From_Y[1][0] = From_Y[1][1] = From_Y[1][2] = From_Y[2][2] = 0.0;
					}
					else
						GL_THROW("NR: Unknown phase configuration on split-phase transformer");
						/*  TROUBLESHOOT
						An unknown phase configuration has been entered for a split-phase,
						center-tapped transformer.  The Newton-Raphson solver does not know how to
						handle it.  Fix the phase and try again.
						*/
									
				}
				else if ((SpecialLnk==SWITCH) || (SpecialLnk==REGULATOR))
				{
					;	//More nothingness (all handled inside switch/regulator itself)
				}
				else	//Other xformers
				{
					//Pre-admittancized matrix
					equalm(b_mat,Yto);

					//Store value into YSto
					for (jindex=0; jindex<3; jindex++)
					{
						for (kindex=0; kindex<3; kindex++)
						{
							YSto[jindex*3+kindex]=Yto[jindex][kindex];
						}
					}

					multiply(invratio,Yto,Ylefttemp);		//Scale from admittance by turns ratio
					multiply(invratio,Ylefttemp,Yfrom);

					//Store value into YSfrom
					for (jindex=0; jindex<3; jindex++)
					{
						for (kindex=0; kindex<3; kindex++)
						{
							YSfrom[jindex*3+kindex]=Yfrom[jindex][kindex];
						}
					}

					multiply(invratio,Yto,To_Y);		//Incorporate turns ratio information into line's admittance matrix.
					multiply(voltage_ratio,Yfrom,From_Y); //Scales voltages to same "level" for GS //uncomment me
				}
			}
			else 				//Simple lines
			{
				//Compute total self admittance - include line charging capacitance
				equalm(a_mat,Ylinecharge);
				Ylinecharge[0][0]-=1;
				Ylinecharge[1][1]-=1;
				Ylinecharge[2][2]-=1;
				multiply(2,Ylinecharge,Ylefttemp);
				multiply(Y,Ylefttemp,Ylinecharge);

				addition(Ylinecharge,Y,From_Y);
				//equalm(From_Y,To_Y);
			}
			
			//Update status variable
			prev_status = status;
		}

		//Update time variable if necessary
		if (prev_LTime != t0)
		{
			prev_LTime=t0;
		}
	}
	else if ((solver_method==SM_GS) & (is_closed()) & (prev_LTime!=t0))	//Initial YVs calculations
	{
		node *fnode = OBJECTDATA(from,node);
		node *tnode = OBJECTDATA(to,node);
		if (fnode==NULL || tnode==NULL)
			return TS_NEVER;
		
		complex Ytot[3][3];
		complex Ylinecharge[3][3];
		complex Y[3][3];
		complex Yc[3][3];
		complex Ifrom[3];
		complex Ito[3];
		complex Ys[3][3];
		complex Yto[3][3];
		complex Yfrom[3][3];
		complex Ylefttemp[3][3];
		complex Yrighttemp[3][3];
		complex outcurrent[3];
		complex tempvar[3];
		double invratio;
		char jindex, kindex;
		char zerosum = 0;

		//Update t0 variable
		prev_LTime=t0;

		//Reset global convergence variable
		GS_all_converged=false;

		for (jindex=0;jindex<3;jindex++)	//Check to see if b_mat is all zeros (0 length line)
		{
			for (kindex=0;kindex<3;kindex++)
			{
				if (b_mat[jindex][kindex]==0)
				{
					zerosum++;
				}
			}
		}

		if ((zerosum==9)) // && (fnode->bustype!=2)) //Zero length line & from is not the swing bus
		{
			//put some values in the matrices first - these need to be fixed to get accurate power flow...but it's length 0!?!
			b_mat[0][0] = b_mat[1][1] = b_mat[2][2] = fault_Z;
			d_mat[0][0] = d_mat[1][1] = d_mat[2][2] = 1.0;
			A_mat[0][0] = A_mat[1][1] = A_mat[2][2] = 1.0;
			B_mat[0][0] = B_mat[1][1] = B_mat[2][2] = fault_Z;

			//Remove ourselves from from and to node's linked lists, otherwise it may reference itself
			
			//Grab the linked list from the from object
			LINKCONNECTED *parlink = &fnode->nodelinks;

			LINKCONNECTED *prevlink = (LINKCONNECTED *)gl_malloc(sizeof(LINKCONNECTED));
			if (prevlink==NULL)
			{
				gl_error("GS: memory allocation failure zero length");
				/*  TROUBLESHOOT
				This is a bug.  Gauss-Seidel attempted to allocate memory for a zero-length substitution and failed.
				Please submit a bug report and your model files.
				*/
				return 0;
			}

			while (parlink->next!=NULL)		//Parse through the linked list
			{
				prevlink = parlink;
				parlink = parlink->next;
				
				if ((parlink->fnodeconnected==from) & (parlink->tnodeconnected==to)) //this is us
				{
					prevlink->next=parlink->next;
				}
			}
			
			prevlink = NULL;
			parlink = &tnode->nodelinks;

			if (parlink->next->next!=NULL)	//Was the only line in, so just get rid of us
			{
				while (parlink->next!=NULL)
				{
					prevlink = parlink;
					parlink = parlink->next;
					
					if ((parlink->fnodeconnected==from) & (parlink->tnodeconnected==to)) //this is us
					{
						prevlink->next=parlink->next;
					}
				}
			}
			else
				parlink->next=NULL;

			prevlink=NULL;
			gl_free(prevlink);

			OBJECT *obj = OBJECTHDR(this);

			//Essentially setting from as parent - follow basic idea of node:init

			//See if it is a node/load/meter
			if (!(gl_object_isa(from,"load") | gl_object_isa(from,"node") | gl_object_isa(from,"meter")))
				GL_THROW("GS: Attempt to substitue 0 length line %d failed: from is not a node device!",obj->id);
				/*  TROUBLESHOOT
				The from end of a line is not a node, load, or meter.  Gauss-Seidel has failed to do a zero line
				substitution as a result.  Check the from connection of the link.
				*/

			//Make sure our phases align, otherwise become angry
			if (fnode->phases!=tnode->phases)
				GL_THROW("GS: Attempt to substitue 0 length line %d failed: endpoint phases do not match!",obj->id);
				/*  TROUBLESHOOT
				Gauss-Seidel attempted to substitute a 0 length line with a parent-child relationship.  However, the parent and child
				phases do not explicitly match.  Set them the same to enable the parent child relationship.
				*/

			//Additional check not needed in node - make sure To isn't a parent (no easy way to fix this, if multiple children gets wonky)
			if (tnode->SubNode==(SUBNODETYPE)3)
			{
				//Reference its child
				node *SubNodeObj = OBJECTDATA(tnode->SubNodeParent,node);
				gl_warning("0 Length Line %d has child-linked object as the end.  If more than one child existed, earlier children have been lost!",obj->id);
				/*  TROUBLESHOOT
				The end link of a system was attached to a node that already had a parent-child relationship.  If more than one child was connected to
				this end node, other children may have lost their connection and no longer be connected to the system.
				*/
			
				//Now have to handle based on what the from node of the line is
				if (fnode->SubNode==(SUBNODETYPE)2)	//From node is another child
				{
					GL_THROW("GS: Attempt to substitute 0 length line %d failed: Would result in great-grandchilren nesting which is unsupported in GS!",obj->id);
					/*  TROUBLESHOOT
					Gauss-Seidel is only set to implement "grandchildren" relationships.  That is, parent-child connections
					may only be nested two-deep.  Any further nesting is not supported.
					*/
				}
				else	//From node is unchilded
				{
					//Set appropriate flags (store parent name and flag from & to)
					tnode->SubNode = (SUBNODETYPE)2;
					tnode->SubNodeParent = from;
					
					fnode->SubNode = (SUBNODETYPE)3;
					fnode->SubNodeParent = to;

					//Set to's subnode (or the one we found) to from node as well
					SubNodeObj->SubNodeParent = from;
				}
			}
			else	//Normal To node
			{
				if (fnode->SubNode==(SUBNODETYPE)2)	//From node is another child
				{
					//Set appropriate flags (store parent name and flag)
					tnode->SubNode = (SUBNODETYPE)2;
					tnode->SubNodeParent = fnode->SubNodeParent;
				}
				else	//From node is unchilded
				{
					//Set appropriate flags (store parent name and flag from & to)
					tnode->SubNode = (SUBNODETYPE)2;
					tnode->SubNodeParent = from;
					
					fnode->SubNode = (SUBNODETYPE)3;
					fnode->SubNodeParent = to;
				}
			}

			for (jindex=0;jindex<3;jindex++)	//zero out load tracking matrix (just in case) (do both in case to was parent too)
			{
				for (kindex=0;kindex<3;kindex++)
				{
					fnode->last_child_power[jindex][kindex]=0.0;
					tnode->last_child_power[jindex][kindex]=0.0;
				}
			}

		}
		else	//normal b_mat (or at least, not all zeros)
		{
			//Ensure have zeroed out the Y at first
			for (jindex=0;jindex<3;jindex++)
			{
				for (kindex=0;kindex<3;kindex++)
				{
					Y[jindex][kindex] = 0.0;
				}
			}

			// compute admittance - invert b matrix - special circumstances given different methods
			if (has_phase(PHASE_S)) //Triplexy
			{
				inverse(b_mat,Y);
			}
			else if (has_phase(PHASE_A) && !has_phase(PHASE_B) && !has_phase(PHASE_C)) //only A
				Y[0][0] = complex(1.0) / b_mat[0][0];
			else if (!has_phase(PHASE_A) && has_phase(PHASE_B) && !has_phase(PHASE_C)) //only B
				Y[1][1] = complex(1.0) / b_mat[1][1];
			else if (!has_phase(PHASE_A) && !has_phase(PHASE_B) && has_phase(PHASE_C)) //only C
				Y[2][2] = complex(1.0) / b_mat[2][2];
			else if (has_phase(PHASE_A) && !has_phase(PHASE_B) && has_phase(PHASE_C)) //has A & C
			{
				complex detvalue = b_mat[0][0]*b_mat[2][2] - b_mat[0][2]*b_mat[2][0];

				Y[0][0] = b_mat[2][2] / detvalue;
				Y[0][2] = b_mat[0][2] * -1.0 / detvalue;
				Y[2][0] = b_mat[2][0] * -1.0 / detvalue;
				Y[2][2] = b_mat[0][0] / detvalue;
			}
			else if (has_phase(PHASE_A) && has_phase(PHASE_B) && !has_phase(PHASE_C)) //has A & B
			{
				complex detvalue = b_mat[0][0]*b_mat[1][1] - b_mat[0][1]*b_mat[1][0];

				Y[0][0] = b_mat[1][1] / detvalue;
				Y[0][1] = b_mat[0][1] * -1.0 / detvalue;
				Y[1][0] = b_mat[1][0] * -1.0 / detvalue;
				Y[1][1] = b_mat[0][0] / detvalue;
			}
			else if (!has_phase(PHASE_A) && has_phase(PHASE_B) && has_phase(PHASE_C))	//has B & C
			{
				complex detvalue = b_mat[1][1]*b_mat[2][2] - b_mat[1][2]*b_mat[2][1];

				Y[1][1] = b_mat[2][2] / detvalue;
				Y[1][2] = b_mat[1][2] * -1.0 / detvalue;
				Y[2][1] = b_mat[2][1] * -1.0 / detvalue;
				Y[2][2] = b_mat[1][1] / detvalue;
			}
			else if ((has_phase(PHASE_A) && has_phase(PHASE_B) && has_phase(PHASE_C)) || (has_phase(PHASE_D))) //has ABC or D (D=ABC)
				inverse(b_mat,Y);
			// defaulted else - No phases (e.g., the line does not exist) - just = 0

			//Compute total self admittance - include line charging capacitance
			equalm(a_mat,Ylinecharge);
			Ylinecharge[0][0]-=1;
			Ylinecharge[1][1]-=1;
			Ylinecharge[2][2]-=1;
			multiply(2,Ylinecharge,Ylefttemp);
			multiply(Y,Ylefttemp,Ylinecharge);

			addition(Ylinecharge,Y,Ytot);
			
			if ((voltage_ratio!=1) | (SpecialLnk!=NORMAL))	//Handle transformers slightly different
			{
				invratio=1.0/voltage_ratio;

				if (SpecialLnk==DELTAGWYE)	//Delta-Gwye implementation
				{
					complex tempImped;

					//Pre-admittancized matrix
					equalm(b_mat,Yto);

					//Adjust for To_Y
					multiply(Yto,c_mat,To_Y);

					//Scale to other size
					multiply(invratio,Yto,Ylefttemp);
					multiply(invratio,Ylefttemp,Yfrom);

					//Adjust for From_Y
					multiply(B_mat,Yto,From_Y);

					//Fix what I broke
					for(jindex=0;jindex<3;jindex++)
					{
						for(kindex=0;kindex<3;kindex++)
						{
							c_mat[jindex][kindex]=0.0;
							B_mat[jindex][kindex]=0.0;
						}
					}

					tempImped = complex(1.0) / b_mat[0][0];
					B_mat[0][0] = B_mat[1][1] = B_mat[2][2] = tempImped;

					//Calculate YVs terms
					Ifrom[0]=From_Y[0][0]*tnode->voltage[0]+
							 From_Y[0][1]*tnode->voltage[1]+
							 From_Y[0][2]*tnode->voltage[2];
					Ifrom[1]=From_Y[1][0]*tnode->voltage[0]+
							 From_Y[1][1]*tnode->voltage[1]+
							 From_Y[1][2]*tnode->voltage[2];
					Ifrom[2]=From_Y[2][0]*tnode->voltage[0]+
							 From_Y[2][1]*tnode->voltage[1]+
							 From_Y[2][2]*tnode->voltage[2];
					Ito[0] = To_Y[0][0]*fnode->voltage[0]+
							 To_Y[0][1]*fnode->voltage[1]+
							 To_Y[0][2]*fnode->voltage[2];
					Ito[1] = To_Y[1][0]*fnode->voltage[0]+
							 To_Y[1][1]*fnode->voltage[1]+
							 To_Y[1][2]*fnode->voltage[2];
					Ito[2] = To_Y[2][0]*fnode->voltage[0]+
							 To_Y[2][1]*fnode->voltage[1]+
							 To_Y[2][2]*fnode->voltage[2];

				}
				else if (SpecialLnk==REGULATOR)	//Regulator
				{
					GL_THROW("GS: Regulator not implemented in Gauss-Seidel Solver yet!");
					/*  TROUBLESHOOT
					Regulators are not coded into the Gauss-Seidel solver at this time.  Please
					use a different solver method or find a way to remove the regulator from your model.
					*/

					equalm(b_mat,Yto);	//Initial code.  Very untested
					equalm(c_mat,Yfrom);

					equalm(Yto,To_Y);

					To_Y[0][0] *= B_mat[0][0];
					To_Y[0][1] *= B_mat[0][1];
					To_Y[0][2] *= B_mat[0][2];
					To_Y[1][0] *= B_mat[1][0];
					To_Y[1][1] *= B_mat[1][1];
					To_Y[1][2] *= B_mat[1][2];
					To_Y[2][0] *= B_mat[2][0];
					To_Y[2][1] *= B_mat[2][1];
					To_Y[2][2] *= B_mat[2][2];

					equalm(Yfrom,From_Y);

					From_Y[0][0] = (B_mat[0][0]==0.0) ? 0.0 : From_Y[0][0]*B_mat[0][0];
					From_Y[0][1] = (B_mat[0][1]==0.0) ? 0.0 : From_Y[0][1]*B_mat[0][1];
					From_Y[0][2] = (B_mat[0][2]==0.0) ? 0.0 : From_Y[0][2]*B_mat[0][2];
					From_Y[1][0] = (B_mat[1][0]==0.0) ? 0.0 : From_Y[1][0]*B_mat[1][0];
					From_Y[1][1] = (B_mat[1][1]==0.0) ? 0.0 : From_Y[1][1]*B_mat[1][1];
					From_Y[1][2] = (B_mat[1][2]==0.0) ? 0.0 : From_Y[1][2]*B_mat[1][2];
					From_Y[2][0] = (B_mat[2][0]==0.0) ? 0.0 : From_Y[2][0]*B_mat[2][0];
					From_Y[2][1] = (B_mat[2][1]==0.0) ? 0.0 : From_Y[2][1]*B_mat[2][1];
					From_Y[2][2] = (B_mat[2][2]==0.0) ? 0.0 : From_Y[2][2]*B_mat[2][2];

					//Zero the used matrices
					b_mat[0][0] = b_mat[0][1] = b_mat[0][2] = complex(0,0);
					b_mat[1][0] = b_mat[1][1] = b_mat[1][2] = complex(0,0);
					b_mat[2][0] = b_mat[2][1] = b_mat[2][2] = complex(0,0);

					equalm(b_mat,c_mat);
					equalm(b_mat,B_mat);

					Ifrom[0]=From_Y[0][0]*tnode->voltage[0]+
							 From_Y[0][1]*tnode->voltage[1]+
							 From_Y[0][2]*tnode->voltage[2];
					Ifrom[1]=From_Y[1][0]*tnode->voltage[0]+
							 From_Y[1][1]*tnode->voltage[1]+
							 From_Y[1][2]*tnode->voltage[2];
					Ifrom[2]=From_Y[2][0]*tnode->voltage[0]+
							 From_Y[2][1]*tnode->voltage[1]+
							 From_Y[2][2]*tnode->voltage[2];
					Ito[0] = To_Y[0][0]*fnode->voltage[0]+
							 To_Y[0][1]*fnode->voltage[1]+
							 To_Y[0][2]*fnode->voltage[2];
					Ito[1] = To_Y[1][0]*fnode->voltage[0]+
							 To_Y[1][1]*fnode->voltage[1]+
							 To_Y[1][2]*fnode->voltage[2];
					Ito[2] = To_Y[2][0]*fnode->voltage[0]+
							 To_Y[2][1]*fnode->voltage[1]+
							 To_Y[2][2]*fnode->voltage[2];
				}
				else if (SpecialLnk==SPLITPHASE)	//Split phase - non working
				{
					equalm(b_mat,Yto);
					equalm(c_mat,Yfrom);

					for (jindex=0;jindex<3;jindex++)
					{
						for (kindex=0;kindex<3;kindex++)
						{
							c_mat[jindex][kindex] = 0.0;
						}
					}

/*					Old version - works?? - fixing assignments so can generalize
					equalm(b_mat,Yto);
					Yto[2][0] = Yto[2][1] = 0.0;

					equalm(b_mat,Ylefttemp);
					Ylefttemp[1][0] = Ylefttemp[1][1];
					Ylefttemp[1][1] = 0.0;
					multiply(invratio,Ylefttemp,To_Y);

					equalm(c_mat,Yfrom);
					equalm(c_mat,From_Y);
					Yfrom[0][0] = Yfrom[2][2];
					Yfrom[2][2] = Yfrom[0][1] = 0.0;

					c_mat[0][0] = c_mat[0][1] = c_mat[2][2] = 0.0;*/

					//multiply(invratio,Yfrom,From_Y);

					Ifrom[0]=From_Y[0][0]*tnode->voltage[0]+
							 From_Y[0][1]*tnode->voltage[1]+
							 From_Y[0][2]*tnode->voltage[2];
					Ifrom[1]=From_Y[1][0]*tnode->voltage[0]+
							 From_Y[1][1]*tnode->voltage[1]+
							 From_Y[1][2]*tnode->voltage[2];
					Ifrom[2]=From_Y[2][0]*tnode->voltage[0]+
							 From_Y[2][1]*tnode->voltage[1]+
							 From_Y[2][2]*tnode->voltage[2];
					Ito[0] = To_Y[0][0]*fnode->voltage[0]+
							 To_Y[0][1]*fnode->voltage[1]+
							 To_Y[0][2]*fnode->voltage[2];
					Ito[1] = To_Y[1][0]*fnode->voltage[0]+
							 To_Y[1][1]*fnode->voltage[1]+
							 To_Y[1][2]*fnode->voltage[2];
					Ito[2] = To_Y[2][0]*fnode->voltage[0]+
							 To_Y[2][1]*fnode->voltage[1]+
							 To_Y[2][2]*fnode->voltage[2];

				}
				else	//Other xformers
				{
					//Pre-admittancized matrix
					equalm(b_mat,Yto);

					multiply(invratio,Yto,Ylefttemp);		//Scale from admittance by turns ratio
					multiply(invratio,Ylefttemp,Yfrom);

					multiply(invratio,Yto,To_Y);		//Incorporate turns ratio information into line's admittance matrix.
					multiply(voltage_ratio,Yfrom,From_Y); //Scales voltages to same "level" for GS //uncomment me

					Ifrom[0]=From_Y[0][0]*tnode->voltage[0]+
							 From_Y[0][1]*tnode->voltage[1]+
							 From_Y[0][2]*tnode->voltage[2];
					Ifrom[1]=From_Y[1][0]*tnode->voltage[0]+
							 From_Y[1][1]*tnode->voltage[1]+
							 From_Y[1][2]*tnode->voltage[2];
					Ifrom[2]=From_Y[2][0]*tnode->voltage[0]+
							 From_Y[2][1]*tnode->voltage[1]+
							 From_Y[2][2]*tnode->voltage[2];
					Ito[0] = To_Y[0][0]*fnode->voltage[0]+
							 To_Y[0][1]*fnode->voltage[1]+
							 To_Y[0][2]*fnode->voltage[2];
					Ito[1] = To_Y[1][0]*fnode->voltage[0]+
							 To_Y[1][1]*fnode->voltage[1]+
							 To_Y[1][2]*fnode->voltage[2];
					Ito[2] = To_Y[2][0]*fnode->voltage[0]+
							 To_Y[2][1]*fnode->voltage[1]+
							 To_Y[2][2]*fnode->voltage[2];


				}

				LOCK_OBJECT(from);
				addition(fnode->Ys,Yfrom,fnode->Ys);
				fnode->YVs[0] += Ifrom[0];
				fnode->YVs[1] += Ifrom[1];
				fnode->YVs[2] += Ifrom[2];
				UNLOCK_OBJECT(from);

				LOCK_OBJECT(to);
				addition(tnode->Ys,Yto,tnode->Ys);
				tnode->YVs[0] += Ito[0];
				tnode->YVs[1] += Ito[1];
				tnode->YVs[2] += Ito[2];
				UNLOCK_OBJECT(to);
			
			}
			else					//Simple lines
			{
				//Compute "currents"
				//Individual phases
				Ifrom[0]=Ytot[0][0]*tnode->voltage[0]+
						 Ytot[0][1]*tnode->voltage[1]+
						 Ytot[0][2]*tnode->voltage[2];
				Ifrom[1]=Ytot[1][0]*tnode->voltage[0]+
						 Ytot[1][1]*tnode->voltage[1]+
						 Ytot[1][2]*tnode->voltage[2];
				Ifrom[2]=Ytot[2][0]*tnode->voltage[0]+
						 Ytot[2][1]*tnode->voltage[1]+
						 Ytot[2][2]*tnode->voltage[2];
				Ito[0] = Ytot[0][0]*fnode->voltage[0]+
						 Ytot[0][1]*fnode->voltage[1]+
						 Ytot[0][2]*fnode->voltage[2];
				Ito[1] = Ytot[1][0]*fnode->voltage[0]+
						 Ytot[1][1]*fnode->voltage[1]+
						 Ytot[1][2]*fnode->voltage[2];
				Ito[2] = Ytot[2][0]*fnode->voltage[0]+
						 Ytot[2][1]*fnode->voltage[1]+
						 Ytot[2][2]*fnode->voltage[2];
			
				//Update admittance tracking - separate matrices (transformer support)
				equalm(Ytot,To_Y);
				equalm(Ytot,From_Y);

				//Update from node YVs
				LOCK_OBJECT(from);
				addition(fnode->Ys,Ytot,fnode->Ys);
				fnode->YVs[0] += Ifrom[0];
				fnode->YVs[1] += Ifrom[1];
				fnode->YVs[2] += Ifrom[2];
				UNLOCK_OBJECT(from);

				//Update to node YVs
				LOCK_OBJECT(to);
				addition(tnode->Ys,Ytot,tnode->Ys);
				tnode->YVs[0] += Ito[0];
				tnode->YVs[1] += Ito[1];
				tnode->YVs[2] += Ito[2];
				UNLOCK_OBJECT(to);
			}
		}
	}

	return t1;
}

TIMESTAMP link::sync(TIMESTAMP t0)
{
#ifdef SUPPORT_OUTAGES
	node *fNode;
	node *tNode;
	fNode=OBJECTDATA(from,node);
	tNode=OBJECTDATA(to,node);
#endif

	if (is_closed())
	{
		if ((solver_method==SM_NR) && (NR_cycle==true))	//Accumulation cycle
		{
			//Solve current equations to get current injections
			node *fnode = OBJECTDATA(from,node);
			node *tnode = OBJECTDATA(to,node);
			complex vtemp[3];
			complex itemp[3];
			complex current_temp[3];
			complex invsquared;

			if ((voltage_ratio!=1.0) && (SpecialLnk != DELTAGWYE) && (SpecialLnk != SPLITPHASE) && (SpecialLnk != REGULATOR))
			{
				invsquared = 1.0 / (voltage_ratio * voltage_ratio);
				//(-a*Vout+Vin)
				vtemp[0] = fnode->voltage[0]-
						   A_mat[0][0]*tnode->voltage[0]-
						   A_mat[0][1]*tnode->voltage[1]-
						   A_mat[0][2]*tnode->voltage[2];

				vtemp[1] = fnode->voltage[1]-
						   A_mat[1][0]*tnode->voltage[0]-
						   A_mat[1][1]*tnode->voltage[1]-
						   A_mat[1][2]*tnode->voltage[2];

				vtemp[2] = fnode->voltage[2]-
						   A_mat[2][0]*tnode->voltage[0]-
						   A_mat[2][1]*tnode->voltage[1]-
						   A_mat[2][2]*tnode->voltage[2];

				//Put across admittance
				itemp[0] = b_mat[0][0]*vtemp[0]+
						   b_mat[0][1]*vtemp[1]+
						   b_mat[0][2]*vtemp[2];

				itemp[1] = b_mat[1][0]*vtemp[0]+
						   b_mat[1][1]*vtemp[1]+
						   b_mat[1][2]*vtemp[2];

				itemp[2] = b_mat[2][0]*vtemp[0]+
						   b_mat[2][1]*vtemp[1]+
						   b_mat[2][2]*vtemp[2];

				//Scale the "b_mat" value by the inverse (make it high-side impedance)
				//Post values based on phases (reliability related)
				if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//A
					current_in[0] = itemp[0]*invsquared;
				else
					current_in[0] = 0.0;

				if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//B
					current_in[1] = itemp[1]*invsquared;
				else
					current_in[1] = 0.0;

				if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//C
					current_in[2] = itemp[2]*invsquared;
				else
					current_in[2] = 0.0;

				//Calculate current out
				if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//A
				{
					current_out[0] = A_mat[0][0]*current_in[0]+
									 A_mat[0][1]*current_in[1]+
									 A_mat[0][2]*current_in[2];

					//Apply additional change
					if (a_mat[0][0] != 0)
					{
						current_out[0] -= tnode->voltage[0]/a_mat[0][0]*voltage_ratio;
					}
				}
				else
					current_out[0] = 0.0;

				if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//B
				{
					current_out[1] = A_mat[1][0]*current_in[0]+
									 A_mat[1][1]*current_in[1]+
									 A_mat[1][2]*current_in[2];

					//Apply additional update
					if (a_mat[1][1] != 0)
					{
						current_out[1] -= tnode->voltage[1]/a_mat[1][1]*voltage_ratio;
					}
				}
				else
					current_out[1] = 0.0;

				if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//C
				{
					current_out[2] = A_mat[2][0]*current_in[0]+
									 A_mat[2][1]*current_in[1]+
									 A_mat[2][2]*current_in[2];

					//Apply additional update
					if (a_mat[2][2] != 0)
					{
						current_out[2] -= tnode->voltage[2]/a_mat[2][2]*voltage_ratio;
					}
				}
				else
					current_out[2] = 0.0;

				//Lock from object for current update
				LOCK_OBJECT(from);

				//Current in is just the same
				fnode->current_inj[0] += current_in[0];
				fnode->current_inj[1] += current_in[1];
				fnode->current_inj[2] += current_in[2];

				//Now unlock it
				UNLOCK_OBJECT(from);
			}//end normal transformers
			else if (SpecialLnk == REGULATOR)
			{
				//(-a*Vout+Vin)
				vtemp[0] = fnode->voltage[0]-
						   a_mat[0][0]*tnode->voltage[0]-
						   a_mat[0][1]*tnode->voltage[1]-
						   a_mat[0][2]*tnode->voltage[2];

				vtemp[1] = fnode->voltage[1]-
						   a_mat[1][0]*tnode->voltage[0]-
						   a_mat[1][1]*tnode->voltage[1]-
						   a_mat[1][2]*tnode->voltage[2];

				vtemp[2] = fnode->voltage[2]-
						   a_mat[2][0]*tnode->voltage[0]-
						   a_mat[2][1]*tnode->voltage[1]-
						   a_mat[2][2]*tnode->voltage[2];

				if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//A
				{
					current_out[0] = From_Y[0][0]*vtemp[0]+
									 From_Y[0][1]*vtemp[1]+
									 From_Y[0][2]*vtemp[2];
				}
				else
					current_out[0] = 0.0;

				if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//B
				{
					current_out[1] = From_Y[1][0]*vtemp[0]+
									 From_Y[1][1]*vtemp[1]+
									 From_Y[1][2]*vtemp[2];
				}
				else
					current_out[1] = 0.0;

				if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//C
				{
					current_out[2] = From_Y[2][0]*vtemp[0]+
									From_Y[2][1]*vtemp[1]+
									From_Y[2][2]*vtemp[2];
				}
				else
					current_out[2] = 0.0;

				//Calculate current_in based on current_out (backwards, isn't it?)
				if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//A
				{
					current_in[0] = d_mat[0][0]*current_out[0]+
									d_mat[0][1]*current_out[1]+
									d_mat[0][2]*current_out[2];
				}
				else
					current_in[0] = 0.0;

				if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//B
				{
					current_in[1] = d_mat[1][0]*current_out[0]+
									d_mat[1][1]*current_out[1]+
									d_mat[1][2]*current_out[2];
				}
				else
					current_in[1] = 0.0;

				if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//C
				{
					current_in[2] = d_mat[2][0]*current_out[0]+
									d_mat[2][1]*current_out[1]+
									d_mat[2][2]*current_out[2];
				}
				else
					current_in[2] = 0.0;

				//Lock from object for current update
				LOCK_OBJECT(from);

				//Current in is just the same
				fnode->current_inj[0] += current_in[0];
				fnode->current_inj[1] += current_in[1];
				fnode->current_inj[2] += current_in[2];

				//Unlock it now
				UNLOCK_OBJECT(from);
			}
			else if (SpecialLnk == DELTAGWYE)
			{
				vtemp[0]=fnode->voltage[0]*a_mat[0][0]+
						 fnode->voltage[1]*a_mat[0][1]+
						 fnode->voltage[2]*a_mat[0][2]-
						 tnode->voltage[0];

				vtemp[1]=fnode->voltage[0]*a_mat[1][0]+
						 fnode->voltage[1]*a_mat[1][1]+
						 fnode->voltage[2]*a_mat[1][2]-
						 tnode->voltage[1];

				vtemp[2]=fnode->voltage[0]*a_mat[2][0]+
						 fnode->voltage[1]*a_mat[2][1]+
						 fnode->voltage[2]*a_mat[2][2]-
						 tnode->voltage[2];

				//Get low side current (current out) - for now, oh grand creator (me) mandates D-GWye are three phase or nothing
				if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x07)	//ABC
				{
					current_out[0] = vtemp[0] * b_mat[0][0];
					current_out[1] = vtemp[1] * b_mat[1][1];
					current_out[2] = vtemp[2] * b_mat[2][2];

					//Translate back to high-side
					current_in[0] = d_mat[0][0]*current_out[0]+
									d_mat[0][1]*current_out[1]+
									d_mat[0][2]*current_out[2];

					current_in[1] = d_mat[1][0]*current_out[0]+
									d_mat[1][1]*current_out[1]+
									d_mat[1][2]*current_out[2];

					current_in[2] = d_mat[2][0]*current_out[0]+
									d_mat[2][1]*current_out[1]+
									d_mat[2][2]*current_out[2];
				}
				else
				{
					current_out[0] = 0.0;
					current_out[1] = 0.0;
					current_out[2] = 0.0;

					current_in[0] = 0.0;
					current_in[1] = 0.0;
					current_in[2] = 0.0;
				}

				//Lock the from object for current injection update
				LOCK_OBJECT(from);

				//Current in is just the same
				fnode->current_inj[0] += current_in[0];
				fnode->current_inj[1] += current_in[1];
				fnode->current_inj[2] += current_in[2];

				//Done, release it
				UNLOCK_OBJECT(from);

			}//end delta-GWYE
			else if (SpecialLnk == SPLITPHASE)	//Split phase, center tapped xformer
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//A
				{
					itemp[0] = fnode->voltage[0]*b_mat[2][2]+
							   tnode->voltage[0]*b_mat[2][0]+
							   tnode->voltage[1]*b_mat[2][1];

					current_in[0] = itemp[0];

					//Lock from node for current injection
					LOCK_OBJECT(from);

					fnode->current_inj[0] += itemp[0];

					//Unlock it
					UNLOCK_OBJECT(from);

					//calculate current out
					current_out[0] = fnode->voltage[0]*b_mat[0][2]+
									 tnode->voltage[0]*b_mat[0][0]+
									 tnode->voltage[1]*b_mat[0][1];

					current_out[1] = fnode->voltage[0]*b_mat[1][2]+
									 tnode->voltage[0]*b_mat[1][0]+
									 tnode->voltage[1]*b_mat[1][1];
				}
				else if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//B
				{
					itemp[0] = fnode->voltage[1]*b_mat[2][2]+
							   tnode->voltage[0]*b_mat[2][0]+
							   tnode->voltage[1]*b_mat[2][1];

					current_in[1] = itemp[0];

					//Lock from for current injection update
					LOCK_OBJECT(from);

					fnode->current_inj[1] += itemp[0];

					//Unlock it now
					UNLOCK_OBJECT(from);

					//calculate current out
					current_out[0] = fnode->voltage[1]*b_mat[0][2]+
									 tnode->voltage[0]*b_mat[0][0]+
									 tnode->voltage[1]*b_mat[0][1];

					current_out[1] = fnode->voltage[1]*b_mat[1][2]+
									 tnode->voltage[0]*b_mat[1][0]+
									 tnode->voltage[1]*b_mat[1][1];

				}
				else if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//C
				{
					itemp[0] = fnode->voltage[2]*b_mat[2][2]+
							   tnode->voltage[0]*b_mat[2][0]+
							   tnode->voltage[1]*b_mat[2][1];

					current_in[2] = itemp[0];

					//Lock from object for current injection update
					LOCK_OBJECT(from);

					fnode->current_inj[2] += itemp[0];

					//Unlock it
					UNLOCK_OBJECT(from);

					//calculate current out
					current_out[0] = fnode->voltage[2]*b_mat[0][2]+
									 tnode->voltage[0]*b_mat[0][0]+
									 tnode->voltage[1]*b_mat[0][1];

					current_out[1] = fnode->voltage[2]*b_mat[1][2]+
									 tnode->voltage[0]*b_mat[1][0]+
									 tnode->voltage[1]*b_mat[1][1];
				}
				else	//No phases valid
				{
					current_out[0] = 0.0;
					current_out[1] = 0.0;
					//2 is generalized below - no sense doing twice
					current_in[0] = 0.0;
					current_in[1] = 0.0;
					current_in[2] = 0.0;
				}

				//Find neutral
				current_out[2] = -(current_out[0] + current_out[1]);

			}//end split-phase, center tapped xformer
			else if (has_phase(PHASE_S))	//Split-phase line
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x80) == 0x80)	//Split-phase valid
				{
					//(-a*Vout+Vin)
					vtemp[0] = fnode->voltage[0]-
							   a_mat[0][0]*tnode->voltage[0]-
							   a_mat[0][1]*tnode->voltage[1];

					vtemp[1] = fnode->voltage[1]-
							   a_mat[1][0]*tnode->voltage[0]-
							   a_mat[1][1]*tnode->voltage[1];

					current_in[0] = From_Y[0][0]*vtemp[0]+
									From_Y[0][1]*vtemp[1];

					current_in[1] = From_Y[1][0]*vtemp[0]+
									From_Y[1][1]*vtemp[1];

					//Calculate neutral current
					current_in[2] = tn[0]*current_in[0] + tn[1]*current_in[1];
				}
				else	//Not valid
				{
					current_in[0] = 0.0;
					current_in[1] = 0.0;
					current_in[2] = 0.0;
				}

				//Cuurent out is the same as current in for triplex (simple lines)
				current_out[0] = current_in[0];
				current_out[1] = current_in[1];
				current_out[2] = current_in[2];

				//Lock from object for current injection update
				LOCK_OBJECT(from);

				//Current in is just the same
				fnode->current_inj[0] += current_in[0];
				fnode->current_inj[1] += current_in[1];

				//Unlock it
				UNLOCK_OBJECT(from);
			}
			else
			{
				//(-a*Vout+Vin)
				vtemp[0] = fnode->voltage[0]-
						   a_mat[0][0]*tnode->voltage[0]-
						   a_mat[0][1]*tnode->voltage[1]-
						   a_mat[0][2]*tnode->voltage[2];

				vtemp[1] = fnode->voltage[1]-
						   a_mat[1][0]*tnode->voltage[0]-
						   a_mat[1][1]*tnode->voltage[1]-
						   a_mat[1][2]*tnode->voltage[2];

				vtemp[2] = fnode->voltage[2]-
						   a_mat[2][0]*tnode->voltage[0]-
						   a_mat[2][1]*tnode->voltage[1]-
						   a_mat[2][2]*tnode->voltage[2];

				//See if phases are valid
				if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//A
				{
					current_in[0] = From_Y[0][0]*vtemp[0]+
									From_Y[0][1]*vtemp[1]+
									From_Y[0][2]*vtemp[2];
				}
				else
					current_in[0] = 0.0;

				if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//B
				{
					current_in[1] = From_Y[1][0]*vtemp[0]+
									From_Y[1][1]*vtemp[1]+
									From_Y[1][2]*vtemp[2];
				}
				else
					current_in[1] = 0.0;

				if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//C
				{
					current_in[2] = From_Y[2][0]*vtemp[0]+
								From_Y[2][1]*vtemp[1]+
								From_Y[2][2]*vtemp[2];
				}
				else
					current_in[2] = 0.0;

				//Current out is the same as current in for lines/simple devices
				current_out[0] = current_in[0];
				current_out[1] = current_in[1];
				current_out[2] = current_in[2];

				//Lock from object for current update
				LOCK_OBJECT(from);

				//Current in is just the same
				fnode->current_inj[0] += current_in[0];
				fnode->current_inj[1] += current_in[1];
				fnode->current_inj[2] += current_in[2];

				//Unlock it now
				UNLOCK_OBJECT(from);
			}

		}
		else if (solver_method==SM_FBS)
		{
			node *f;
			node *t;
			set reverse = get_flow(&f,&t);

#ifdef SUPPORT_OUTAGES
			tNode->condition=fNode->condition;
#endif
			/* compute currents */
			complex i;
			current_in[0] = 
			i = c_mat[0][0] * t->voltage[0] +
				c_mat[0][1] * t->voltage[1] +
				c_mat[0][2] * t->voltage[2] +
				d_mat[0][0] * t->current_inj[0] +
				d_mat[0][1] * t->current_inj[1] +
				d_mat[0][2] * t->current_inj[2];
			LOCKED(from, f->current_inj[0] += i);
			current_in[1] = 
			i = c_mat[1][0] * t->voltage[0] +
				c_mat[1][1] * t->voltage[1] +
				c_mat[1][2] * t->voltage[2] +
				d_mat[1][0] * t->current_inj[0] +
				d_mat[1][1] * t->current_inj[1] +
				d_mat[1][2] * t->current_inj[2];
			LOCKED(from, f->current_inj[1] += i);
			current_in[2] = 
			i = c_mat[2][0] * t->voltage[0] +
				c_mat[2][1] * t->voltage[1] +
				c_mat[2][2] * t->voltage[2] +
				d_mat[2][0] * t->current_inj[0] +
				d_mat[2][1] * t->current_inj[1] +
				d_mat[2][2] * t->current_inj[2];
			LOCKED(from, f->current_inj[2] += i);
		}
	}
#ifdef SUPPORT_OUTAGES
	else if (is_open_any())
	{
		;//Nothing special in here yet
	}
	else if (is_contact_any())
		throw "unable to handle link contact condition";

	if (solver_method==SM_FBS)
	{
		if (voltage_ratio==1.0)	//Only do this for lines
		{
			/* compute for consequences of open link conditions -- Only supports 3-phase fault at the moment */
			if (has_phase(PHASE_A))	a_mat[0][0] = d_mat[0][0] = A_mat[0][0] = is_open() ? 0.0 : 1.0;
			if (has_phase(PHASE_B))	a_mat[1][1] = d_mat[1][1] = A_mat[1][1] = is_open() ? 0.0 : 1.0;
			if (has_phase(PHASE_C))	a_mat[2][2] = d_mat[2][2] = A_mat[2][2] = is_open() ? 0.0 : 1.0;
		}
		if (tNode->condition!=OC_NORMAL)
			tNode->condition=OC_NORMAL;		//Clear the flags just in case
	}


#endif

	return TS_NEVER;
}

TIMESTAMP link::postsync(TIMESTAMP t0)
{
	TIMESTAMP TRET=TS_NEVER;

	if ((solver_method==SM_FBS))
	{
		node *f;
		node *t; //@# make else/if statement for solver method NR; & set current_out->to t->node current_inj;
		set reverse = get_flow(&f,&t);

		// update published current_out values;
		read_I_out[0] = t->current_inj[0];
		read_I_out[1] = t->current_inj[1];

		if (has_phase(PHASE_S) && (voltage_ratio != 1.0))	//Implies SPCT
			read_I_out[2] = -t->current_inj[1] - t->current_inj[0];	//Implies ground at TP Node, so I_n is full neutral + ground
		else
			read_I_out[2] = t->current_inj[2];
		
		if (!is_open())
		{
			/* compute and update voltages */
			complex v;
			v = A_mat[0][0] * f->voltage[0] +
				A_mat[0][1] * f->voltage[1] + // 
				A_mat[0][2] * f->voltage[2] - //@todo current inj; flowing from t node
				B_mat[0][0] * t->current_inj[0] - // current injection put into link from end mode
				B_mat[0][1] * t->current_inj[1] -
				B_mat[0][2] * t->current_inj[2];
			LOCKED(to, t->voltage[0] = v);
			v = A_mat[1][0] * f->voltage[0] +
				A_mat[1][1] * f->voltage[1] +
				A_mat[1][2] * f->voltage[2] -
				B_mat[1][0] * t->current_inj[0] -
				B_mat[1][1] * t->current_inj[1] -
				B_mat[1][2] * t->current_inj[2];
			LOCKED(to, t->voltage[1] = v);
			v = A_mat[2][0] * f->voltage[0] +
				A_mat[2][1] * f->voltage[1] +
				A_mat[2][2] * f->voltage[2] -
				B_mat[2][0] * t->current_inj[0] -
				B_mat[2][1] * t->current_inj[1] -
				B_mat[2][2] * t->current_inj[2];
			LOCKED(to, t->voltage[2] = v);

#ifdef SUPPORT_OUTAGES		
			t->condition=f->condition;
		}
		else if (is_open()) //open
		{
			t->condition=!OC_NORMAL;
		}

		/* propagate voltage source flag from to-bus to from-bus */
		if (t->bustype==node::PQ)
		{
			/* keep a copy of the old flags on the to-bus */
			set of = t->busflags&NF_HASSOURCE;

			/* if the admittance is non-zero */
			if ((a_mat[0][0].Mag()>0 || a_mat[1][1].Mag()>0 || a_mat[2][2].Mag()>0))
			{
				/* the source-flag of the from-bus is copied to the to-bus */
				LOCKED(to, t->busflags |= (f->busflags&NF_HASSOURCE));
			}
			else
			{
				/* otherwise the source flag of the to-bus is cleared */
				LOCKED(to, t->busflags &= ~NF_HASSOURCE);
			}

			/* if the to-bus flags has changed */
			if ((t->busflags&NF_HASSOURCE)!=of)

				/* force the solver to make another pass */
				TRET = t0;
		}
#else
		}
#endif

	}
	else if ((!is_open()) && (solver_method==SM_GS) && GS_all_converged)
	{
		node *fnode = OBJECTDATA(from,node);
		node *tnode = OBJECTDATA(to,node);
		complex current_temp[3];
		complex Binv[3][3];
		char jindex, kindex;

		for (jindex=0; jindex<3; jindex++)
			for (kindex=0; kindex<3; kindex++)
				Binv[jindex][kindex] = 0.0;

		// invert B matrix - special circumstances given different methods
		if (has_phase(PHASE_A) && !has_phase(PHASE_B) && !has_phase(PHASE_C)) //only A
			Binv[0][0] = complex(1.0) / B_mat[0][0];
		else if (!has_phase(PHASE_A) && has_phase(PHASE_B) && !has_phase(PHASE_C)) //only B
			Binv[1][1] = complex(1.0) / B_mat[1][1];
		else if (!has_phase(PHASE_A) && !has_phase(PHASE_B) && has_phase(PHASE_C)) //only C
			Binv[2][2] = complex(1.0) / B_mat[2][2];
		else if (has_phase(PHASE_A) && !has_phase(PHASE_B) && has_phase(PHASE_C)) //has A & C
		{
			complex detvalue = B_mat[0][0]*B_mat[2][2] - B_mat[0][2]*B_mat[2][0];

			Binv[0][0] = B_mat[2][2] / detvalue;
			Binv[0][2] = B_mat[0][2] * -1.0 / detvalue;
			Binv[2][0] = B_mat[2][0] * -1.0 / detvalue;
			Binv[2][2] = B_mat[0][0] / detvalue;
		}
		else if (has_phase(PHASE_A) && has_phase(PHASE_B) && !has_phase(PHASE_C)) //has A & B
		{
			complex detvalue = B_mat[0][0]*B_mat[1][1] - B_mat[0][1]*B_mat[1][0];

			Binv[0][0] = B_mat[1][1] / detvalue;
			Binv[0][1] = B_mat[0][1] * -1.0 / detvalue;
			Binv[1][0] = B_mat[1][0] * -1.0 / detvalue;
			Binv[1][1] = B_mat[0][0] / detvalue;
		}
		else if ((has_phase(PHASE_A) && has_phase(PHASE_B) && has_phase(PHASE_C)) || (has_phase(PHASE_D))) //has ABC or D (D=ABC)
			inverse(B_mat,Binv);
		// defaulted else - No phases (e.g., the line does not exist) - just = 0

		//Calculate output currents
		current_temp[0] = A_mat[0][0]*fnode->voltage[0]+
						  A_mat[0][1]*fnode->voltage[1]+
						  A_mat[0][2]*fnode->voltage[2]-
						  tnode->voltage[0];
		current_temp[1] = A_mat[1][0]*fnode->voltage[0]+
						  A_mat[1][1]*fnode->voltage[1]+
						  A_mat[1][2]*fnode->voltage[2]-
						  tnode->voltage[1];
		current_temp[2] = A_mat[2][0]*fnode->voltage[0]+
						  A_mat[2][1]*fnode->voltage[1]+
						  A_mat[2][2]*fnode->voltage[2]-
						  tnode->voltage[2];

		current_out[0] = Binv[0][0]*current_temp[0]+
						 Binv[0][1]*current_temp[1]+
						 Binv[0][2]*current_temp[2];
		current_out[1] = Binv[1][0]*current_temp[0]+
						 Binv[1][1]*current_temp[1]+
						 Binv[1][2]*current_temp[2];
		current_out[2] = Binv[2][0]*current_temp[0]+
						 Binv[2][1]*current_temp[1]+
						 Binv[2][2]*current_temp[2];

		//Calculate input currents
		current_in[0] = c_mat[0][0]*tnode->voltage[0]+
						c_mat[0][1]*tnode->voltage[1]+
						c_mat[0][2]*tnode->voltage[2]+
						d_mat[0][0]*current_out[0]+
						d_mat[0][1]*current_out[1]+
						d_mat[0][2]*current_out[2];
		current_in[1] = c_mat[1][0]*tnode->voltage[0]+
						c_mat[1][1]*tnode->voltage[1]+
						c_mat[1][2]*tnode->voltage[2]+
						d_mat[1][0]*current_out[0]+
						d_mat[1][1]*current_out[1]+
						d_mat[1][2]*current_out[2];
		current_in[2] = c_mat[2][0]*tnode->voltage[0]+
						c_mat[2][1]*tnode->voltage[1]+
						c_mat[2][2]*tnode->voltage[2]+
						d_mat[2][0]*current_out[0]+
						d_mat[2][1]*current_out[1]+
						d_mat[2][2]*current_out[2];

		//Compute magnitude of power output
		power_in = ((fnode->voltage[0]*~current_in[0]).Mag() + (fnode->voltage[1]*~current_in[1]).Mag() + (fnode->voltage[2]*~current_in[2]).Mag());
		power_out = ((tnode->voltage[0]*~current_out[0]).Mag() + (tnode->voltage[1]*~current_out[1]).Mag() + (tnode->voltage[2]*~current_out[2]).Mag());
	}
	 // updates published current_in variable
		read_I_in[0] = current_in[0];
		read_I_in[1] = current_in[1];
		read_I_in[2] = current_in[2];

	//Do the same for current out - if NR (FBS done above)
	if (solver_method == SM_NR)
	{
		//Populate the outputs now
		read_I_out[0] = current_out[0];
		read_I_out[1] = current_out[1];
		read_I_out[2] = current_out[2];
	}

	// This portion can be removed once tape/recorders are being updated in commit.
	if (solver_method == SM_FBS || (solver_method == SM_NR && NR_cycle == true))
	{			
		if (has_phase(PHASE_S))
			calculate_power_splitphase();
		else
			calculate_power();
	}

	return TRET;
}

int link::kmldump(FILE *fp)
{
	OBJECT *obj = OBJECTHDR(this);
	if (isnan(from->latitude) || isnan(to->latitude) || isnan(from->longitude) || isnan(to->longitude))
		return 0;
	fprintf(fp,"    <Placemark>\n");
	if (obj->name)
		fprintf(fp,"      <name>%s</name>\n", obj->name);
	else
		fprintf(fp,"      <name>%s ==> %s</name>\n", from->name?from->name:"unnamed", to->name?to->name:"unnamed");
	fprintf(fp,"      <description>\n");
	fprintf(fp,"        <![CDATA[\n");
	fprintf(fp,"          <TABLE><TR>\n");
	fprintf(fp,"<TR><TD WIDTH=\"25%\">%s&nbsp;%d<HR></TD><TH WIDTH=\"25%\" ALIGN=CENTER>Phase A<HR></TH><TH WIDTH=\"25%\" ALIGN=CENTER>Phase B<HR></TH><TH WIDTH=\"25%\" ALIGN=CENTER>Phase C<HR></TH></TR>\n", obj->oclass->name, obj->id);

	// values
	node *pFrom = OBJECTDATA(from,node);
	node *pTo = OBJECTDATA(to,node);
	double vscale = primary_voltage_ratio*sqrt((double) 3.0)/(double) 1000.0;
	complex loss[3];
	complex flow[3];
	complex current[3];
	complex in[3] = {pFrom->voltageA/B_mat[0][0], pFrom->voltageB/B_mat[1][1], pFrom->voltageC/B_mat[2][2]};
	complex out[3] = {pTo->voltageA/B_mat[0][0], pTo->voltageB/B_mat[1][1], pTo->voltageC/B_mat[2][2]};
	complex Vfrom[3] = {pFrom->voltageA, pFrom->voltageB, pFrom->voltageC};
	complex Vto[3] = {pTo->voltageA,pTo->voltageB,pTo->voltageC};
	int phase[3] = {has_phase(PHASE_A),has_phase(PHASE_B),has_phase(PHASE_C)};
	int i;
	for (i=0; i<3; i++)
	{
		if (phase[i])
		{
			if (Vfrom[i].Re() > Vto[i].Re())
			{
				flow[i] = out[i]*Vto[i]*vscale;
				loss[i] = in[i]*Vfrom[i]*vscale - flow[i];
				current[i] = out[i];
			}
			else
			{
				flow[i] = in[i]*Vfrom[i]*vscale;
				loss[i] = out[i]*Vto[i]*vscale - flow[i];
				current[i] = in[i];
			}
		}
	}

	// flow
	fprintf(fp,"<TR><TH ALIGN=LEFT>Flow</TH>");
	for (i=0; i<3; i++)
	{
		if (phase[i])
			fprintf(fp,"<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;&nbsp;kW&nbsp;&nbsp;<BR>%.3f&nbsp;&nbsp;kVAR</TD>\n",
				flow[i].Re(), flow[i].Im());
		else
			fprintf(fp,"<TD></TD>\n");
	}
	fprintf(fp,"</TR>");

	// current
	fprintf(fp,"<TR><TH ALIGN=LEFT>Current</TH>");
	for (i=0; i<3; i++)
	{
		if (phase[i])
			fprintf(fp,"<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;&nbsp;Amps</TD>\n",
				current[i].Mag());
		else
			fprintf(fp,"<TD></TD>\n");
	}
	fprintf(fp,"</TR>");

	// loss
	fprintf(fp,"<TR><TH ALIGN=LEFT>Loss</TH>");
	for (i=0; i<3; i++)
	{
		if (phase[i])
			fprintf(fp,"<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.2f&nbsp;&nbsp;&nbsp;%%P&nbsp;&nbsp;<BR>%.2f&nbsp;&nbsp;&nbsp;%%Q&nbsp;&nbsp;</TD>\n",
				loss[i].Re()/flow[i].Re()*100,loss[i].Im()/flow[i].Im()*100);
		else
			fprintf(fp,"<TD></TD>\n");
	}
	fprintf(fp,"</TR>");
	fprintf(fp,"</TABLE>\n");
	fprintf(fp,"        ]]>\n");
	fprintf(fp,"      </description>\n");
	fprintf(fp,"      <styleUrl>#%s</styleUrl>>\n",obj->oclass->name);
	fprintf(fp,"      <coordinates>%f,%f</coordinates>\n",
		(from->longitude+to->longitude)/2,(from->latitude+to->latitude)/2);
	fprintf(fp,"      <LineString>\n");			
	fprintf(fp,"        <extrude>0</extrude>\n");
	fprintf(fp,"        <tessellate>0</tessellate>\n");
	fprintf(fp,"        <altitudeMode>relative</altitudeMode>\n");
	fprintf(fp,"        <coordinates>%f,%f,50 %f,%f,50</coordinates>\n",
		from->longitude,from->latitude,to->longitude,to->latitude);
	fprintf(fp,"      </LineString>\n");			
	fprintf(fp,"    </Placemark>\n");
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: link
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_link(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(link::oclass);
		if (*obj!=NULL)
		{
			link *my = OBJECTDATA(*obj,link);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(link);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_link(OBJECT *obj)
{
	try {
		link *my = OBJECTDATA(obj,link);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(link);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_link(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try 
	{
		link *pObj = OBJECTDATA(obj,link);
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
	SYNC_CATCHALL(link);
}

EXPORT int isa_link(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,link)->isa(classname);
}

/**
* UpdateYVs is called by node functions when their voltage changes to update the "current"
* for future nodes.
*
* @param snode is the object to update
* @param snodeside is the type the calling node is (1=from, 2=to)
* @param deltaV the voltage update to apply to YVs terms
*/
void link::calculate_power_splitphase()
{

	node *f = OBJECTDATA(from, node);
	node *t = OBJECTDATA(to, node);

	if (solver_method == SM_NR)
	{
		if (SpecialLnk!=SPLITPHASE) 
		{
			//Original code - attempted to split so single phase matches three phase in terms of output capabilities
			//power_in = (f->voltage[0]*~current_in[0] - f->voltage[1]*~current_in[1] + f->voltage[2]*~current_in[2]).Mag();
			//power_out = (t->voltage[0]*~t->current_inj[0] - t->voltage[1]*~t->current_inj[1] + t->voltage[2]*~t->current_inj[2]).Mag();
				
			//Follows convention of three phase calculations below
			indiv_power_in[0] = f->voltage[0]*~current_in[0];
			indiv_power_in[1] = complex(-1.0) * f->voltage[1]*~current_in[1];
			indiv_power_in[2] = f->voltage[2]*~current_in[2];

			indiv_power_out[0] = t->voltage[0]*~current_out[0];
			indiv_power_out[1] = complex(-1.0) * t->voltage[1]*~current_out[1];
			indiv_power_out[2] = t->voltage[2]*~current_out[2];
		}
		else  
		{
			//Original code - attempted to split so matches three phase below
			//power_in = (f->voltage[0]*~current_in[0]).Mag() + (f->voltage[1]*~current_in[1]).Mag() + (f->voltage[2]*~current_in[2]).Mag();
			//power_out = (t->voltage[0]*~t->current_inj[0] - t->voltage[1]*~t->current_inj[1] + t->voltage[2]*~t->current_inj[2]).Mag();
			//Follows convention of three phase calculations below
			indiv_power_in[0] = f->voltage[0]*~current_in[0];
			indiv_power_in[1] = f->voltage[1]*~current_in[1];
			indiv_power_in[2] = f->voltage[2]*~current_in[2];

			indiv_power_out[0] = t->voltage[0]*~current_out[0];
			indiv_power_out[1] = complex(-1.0) * t->voltage[1]*~current_out[1];
			indiv_power_out[2] = t->voltage[2]*~current_out[2];

		}
	}
	else
	{
		if (SpecialLnk!=SPLITPHASE) 
		{
			//Original code - attempted to split so single phase matches three phase in terms of output capabilities
			//power_in = (f->voltage[0]*~current_in[0] - f->voltage[1]*~current_in[1] + f->voltage[2]*~current_in[2]).Mag();
			//power_out = (t->voltage[0]*~t->current_inj[0] - t->voltage[1]*~t->current_inj[1] + t->voltage[2]*~t->current_inj[2]).Mag();
				
			//Follows convention of three phase calculations below
			indiv_power_in[0] = f->voltage[0]*~current_in[0];
			indiv_power_in[1] = complex(-1.0) * f->voltage[1]*~current_in[1];
			indiv_power_in[2] = f->voltage[2]*~current_in[2];

			indiv_power_out[0] = t->voltage[0]*~t->current_inj[0];
			indiv_power_out[1] = complex(-1.0) * t->voltage[1]*~t->current_inj[1];
			indiv_power_out[2] = t->voltage[2]*~t->current_inj[2];
		}
		else  
		{
			//Original code - attempted to split so matches three phase below
			//power_in = (f->voltage[0]*~current_in[0]).Mag() + (f->voltage[1]*~current_in[1]).Mag() + (f->voltage[2]*~current_in[2]).Mag();
			//power_out = (t->voltage[0]*~t->current_inj[0] - t->voltage[1]*~t->current_inj[1] + t->voltage[2]*~t->current_inj[2]).Mag();
			//Follows convention of three phase calculations below
			indiv_power_in[0] = f->voltage[0]*~current_in[0];
			indiv_power_in[1] = f->voltage[1]*~current_in[1];
			indiv_power_in[2] = f->voltage[2]*~current_in[2];

			indiv_power_out[0] = t->voltage[0]*~t->current_inj[0];
			indiv_power_out[1] = complex(-1.0) * t->voltage[1]*~t->current_inj[1];
			indiv_power_out[2] = t->voltage[2]*~t->current_inj[2];
		}
	}
	//Set direction flag.  Can be a little odd in split phase, since circulating currents.
	set_flow_directions();

	power_in = indiv_power_in[0] + indiv_power_in[1] + indiv_power_in[2];
	power_out = indiv_power_out[0] + indiv_power_out[1] + indiv_power_out[2];

	if (SpecialLnk!=SPLITPHASE) 
	{
		for (int i=0; i<3; i++)
		{
			indiv_power_loss[i] = indiv_power_in[i] - indiv_power_out[i];
			if (indiv_power_loss[i].Re() < 0)
				indiv_power_loss[i].Re() = -indiv_power_loss[i].Re();
		}
	}
	else
	{
		// A little different for split-phase transformers since it goes from one phase to three indiv. powers
		// We'll treat "power losses" in the ABC sense, not phase 123.
		int j;
		if (has_phase(PHASE_A))
			j = 0;
		else if (has_phase(PHASE_B))
			j = 1;
		else if (has_phase(PHASE_C))
			j = 2;
		for (int i=0; i<3; i++)
		{
			if (i == j)
			{
				indiv_power_loss[i] = indiv_power_in[j] - power_out;
				if (indiv_power_loss[i].Re() < 0)
					indiv_power_loss[i].Re() = -indiv_power_loss[i].Re();
			}
			else
				indiv_power_loss[i] = complex(0,0);
		}
	}
	//Calculate overall losses
	power_loss = indiv_power_loss[0] + indiv_power_loss[1] + indiv_power_loss[2];
}

void link::set_flow_directions(void)
{
	int i;
	flow_direction = FD_UNKNOWN; // clear the flows
	for (i=0; i<3; i++)
	{	
		static int shift[] = {0,4,8};
		double power_in = indiv_power_in[i].Mag();
		double power_out = indiv_power_out[i].Mag();
		if (power_in - power_out > ROUNDOFF)
			flow_direction |= ((int64)FD_A_NORMAL<<shift[i]);  // "Normal" flow direction
		else if (power_in - power_out < -ROUNDOFF)
			flow_direction |= ((int64)FD_A_REVERSE<<shift[i]);  // "Reverse" flow direction
		else
			flow_direction |= ((int64)FD_A_NONE<<shift[i]);  // "No" flow direction
	}
}

void link::calculate_power()
{
		node *f = OBJECTDATA(from, node);
		node *t = OBJECTDATA(to, node);

		if (solver_method == SM_NR)
		{
			if (SpecialLnk == SWITCH)
			{
				indiv_power_in[0] = (a_mat[0][0].Re() == 0.0) ? 0.0 : f->voltage[0]*~current_in[0];
				indiv_power_in[1] = (a_mat[1][1].Re() == 0.0) ? 0.0 : f->voltage[1]*~current_in[1];
				indiv_power_in[2] = (a_mat[2][2].Re() == 0.0) ? 0.0 : f->voltage[2]*~current_in[2];

				indiv_power_out[0] = (a_mat[0][0].Re() == 0.0) ? 0.0 : t->voltage[0]*~current_out[0];
				indiv_power_out[1] = (a_mat[1][1].Re() == 0.0) ? 0.0 : t->voltage[1]*~current_out[1];
				indiv_power_out[2] = (a_mat[2][2].Re() == 0.0) ? 0.0 : t->voltage[2]*~current_out[2];
			}
			else
			{
				indiv_power_in[0] = f->voltage[0]*~current_in[0];
				indiv_power_in[1] = f->voltage[1]*~current_in[1];
				indiv_power_in[2] = f->voltage[2]*~current_in[2];

				indiv_power_out[0] = t->voltage[0]*~current_out[0];
				indiv_power_out[1] = t->voltage[1]*~current_out[1];
				indiv_power_out[2] = t->voltage[2]*~current_out[2];
			}
		}
		else
		{
			indiv_power_in[0] = f->voltage[0]*~current_in[0];
			indiv_power_in[1] = f->voltage[1]*~current_in[1];
			indiv_power_in[2] = f->voltage[2]*~current_in[2];

			indiv_power_out[0] = t->voltage[0]*~t->current_inj[0];
			indiv_power_out[1] = t->voltage[1]*~t->current_inj[1];
			indiv_power_out[2] = t->voltage[2]*~t->current_inj[2];
		}

		power_in = indiv_power_in[0] + indiv_power_in[1] + indiv_power_in[2];
		power_out = indiv_power_out[0] + indiv_power_out[1] + indiv_power_out[2];

		//Figure out losses - fix for reverse flow capabilities
		for (int i=0; i<3; i++)
		{
			indiv_power_loss[i] = indiv_power_in[i] - indiv_power_out[i];
			if (indiv_power_loss[i].Re() < 0)
				indiv_power_loss[i].Re() = -indiv_power_loss[i].Re();
		}

		set_flow_directions();

		//Calculate overall losses
		power_loss = indiv_power_loss[0] + indiv_power_loss[1] + indiv_power_loss[2];
	
	
			//GS-esque (but fixed) method of power calculations
			//complex current_temp[3];
			//complex Binv[3][3];
			//complex binv[3][3];
			//node *fnode = OBJECTDATA(from,node);
			//node *tnode = OBJECTDATA(to,node);
			//char jindex, kindex;

			//for (jindex=0; jindex<3; jindex++)
			//{
			//	for (kindex=0; kindex<3; kindex++)
			//	{
			//		Binv[jindex][kindex] = 0.0;
			//		binv[jindex][kindex] = 0.0;
			//	}
			//}

			//// invert B matrix - special circumstances given different methods
			//if (has_phase(PHASE_A) && !has_phase(PHASE_B) && !has_phase(PHASE_C)) //only A
			//{
			//	Binv[0][0] = complex(1.0) / B_mat[0][0];
			//	binv[0][0] = complex(1.0) / b_mat[0][0];
			//}
			//else if (!has_phase(PHASE_A) && has_phase(PHASE_B) && !has_phase(PHASE_C)) //only B
			//{
			//	Binv[1][1] = complex(1.0) / B_mat[1][1];
			//	binv[1][1] = complex(1.0) / b_mat[1][1];
			//}
			//else if (!has_phase(PHASE_A) && !has_phase(PHASE_B) && has_phase(PHASE_C)) //only C
			//{
			//	Binv[2][2] = complex(1.0) / B_mat[2][2];
			//	binv[2][2] = complex(1.0) / b_mat[2][2];
			//}
			//else if (has_phase(PHASE_A) && !has_phase(PHASE_B) && has_phase(PHASE_C)) //has A & C
			//{
			//	complex detvalue = B_mat[0][0]*B_mat[2][2] - B_mat[0][2]*B_mat[2][0];

			//	Binv[0][0] = B_mat[2][2] / detvalue;
			//	Binv[0][2] = B_mat[0][2] * -1.0 / detvalue;
			//	Binv[2][0] = B_mat[2][0] * -1.0 / detvalue;
			//	Binv[2][2] = B_mat[0][0] / detvalue;

			//	//Forward direction calculation
			//	detvalue = b_mat[0][0]*b_mat[2][2] - b_mat[0][2]*b_mat[2][0];

			//	binv[0][0] = b_mat[2][2] / detvalue;
			//	binv[0][2] = b_mat[0][2] * -1.0 / detvalue;
			//	binv[2][0] = b_mat[2][0] * -1.0 / detvalue;
			//	binv[2][2] = b_mat[0][0] / detvalue;
			//}
			//else if (has_phase(PHASE_A) && has_phase(PHASE_B) && !has_phase(PHASE_C)) //has A & B
			//{
			//	complex detvalue = B_mat[0][0]*B_mat[1][1] - B_mat[0][1]*B_mat[1][0];

			//	Binv[0][0] = B_mat[1][1] / detvalue;
			//	Binv[0][1] = B_mat[0][1] * -1.0 / detvalue;
			//	Binv[1][0] = B_mat[1][0] * -1.0 / detvalue;
			//	Binv[1][1] = B_mat[0][0] / detvalue;

			//	//Forward direction calculation
			//	detvalue = b_mat[0][0]*b_mat[1][1] - b_mat[0][1]*b_mat[1][0];

			//	binv[0][0] = b_mat[1][1] / detvalue;
			//	binv[0][1] = b_mat[0][1] * -1.0 / detvalue;
			//	binv[1][0] = b_mat[1][0] * -1.0 / detvalue;
			//	binv[1][1] = b_mat[0][0] / detvalue;
			//}
			//else if ((has_phase(PHASE_A) && has_phase(PHASE_B) && has_phase(PHASE_C)) || (has_phase(PHASE_D))) //has ABC or D (D=ABC)
			//{
			//	inverse(B_mat,Binv);
			//	inverse(b_mat,binv);
			//}
			//// defaulted else - No phases (e.g., the line does not exist) - just = 0

			////Calculate output currents
			//current_temp[0] = A_mat[0][0]*fnode->voltage[0]+
			//				  A_mat[0][1]*fnode->voltage[1]+
			//				  A_mat[0][2]*fnode->voltage[2]-
			//				  tnode->voltage[0];
			//current_temp[1] = A_mat[1][0]*fnode->voltage[0]+
			//				  A_mat[1][1]*fnode->voltage[1]+
			//				  A_mat[1][2]*fnode->voltage[2]-
			//				  tnode->voltage[1];
			//current_temp[2] = A_mat[2][0]*fnode->voltage[0]+
			//				  A_mat[2][1]*fnode->voltage[1]+
			//				  A_mat[2][2]*fnode->voltage[2]-
			//				  tnode->voltage[2];

			//current_out[0] = Binv[0][0]*current_temp[0]+
			//				 Binv[0][1]*current_temp[1]+
			//				 Binv[0][2]*current_temp[2];
			//current_out[1] = Binv[1][0]*current_temp[0]+
			//				 Binv[1][1]*current_temp[1]+
			//				 Binv[1][2]*current_temp[2];
			//current_out[2] = Binv[2][0]*current_temp[0]+
			//				 Binv[2][1]*current_temp[1]+
			//				 Binv[2][2]*current_temp[2];

			////Calculate input currents
			//current_temp[0] = a_mat[0][0]*fnode->voltage[0]+
			//				  a_mat[0][1]*fnode->voltage[1]+
			//				  a_mat[0][2]*fnode->voltage[2]-
			//				  tnode->voltage[0];
			//current_temp[1] = a_mat[1][0]*fnode->voltage[0]+
			//				  a_mat[1][1]*fnode->voltage[1]+
			//				  a_mat[1][2]*fnode->voltage[2]-
			//				  tnode->voltage[1];
			//current_temp[2] = a_mat[2][0]*fnode->voltage[0]+
			//				  a_mat[2][1]*fnode->voltage[1]+
			//				  a_mat[2][2]*fnode->voltage[2]-
			//				  tnode->voltage[2];
			//
			//current_in[0] = binv[0][0]*current_temp[0]+
			//				binv[0][1]*current_temp[1]+
			//				binv[0][2]*current_temp[2];
			//current_in[1] = binv[1][0]*current_temp[0]+
			//				binv[1][1]*current_temp[1]+
			//				binv[1][2]*current_temp[2];
			//current_in[2] = binv[2][0]*current_temp[0]+
			//				binv[2][1]*current_temp[1]+
			//				binv[2][2]*current_temp[2];

			////Only three-phase portion of individual powers calculated right now
			//indiv_power_in[0] = fnode->voltage[0]*~current_in[0];
			//indiv_power_in[1] = fnode->voltage[1]*~current_in[1];
			//indiv_power_in[2] = fnode->voltage[2]*~current_in[2];

			//indiv_power_out[0] = tnode->voltage[0]*~current_out[0];
			//indiv_power_out[1] = tnode->voltage[1]*~current_out[1];
			//indiv_power_out[2] = tnode->voltage[2]*~current_out[2];

			//Only three-phase portion of individual powers calculated right now

}
void *link::UpdateYVs(OBJECT *snode, char snodeside, complex *deltaV)
{
	complex YVsNew[3];
	node *worknode = OBJECTDATA(snode,node);

	//Zero the accumulator
	YVsNew[0] = YVsNew[1] = YVsNew[2] = 0.0;

	//Calculate YVs update - gets done regardless of who we are
	if (snodeside==1)		//From node
	{
		if (deltaV[0]!=0.0)	//Non-zero
		{
			YVsNew[0] = To_Y[0][0]*deltaV[0];
			YVsNew[1] = To_Y[1][0]*deltaV[0];
			YVsNew[2] = To_Y[2][0]*deltaV[0];
		}

		if (deltaV[1]!=0.0)	//Non-zero
		{
			YVsNew[0] += To_Y[0][1]*deltaV[1];
			YVsNew[1] += To_Y[1][1]*deltaV[1];
			YVsNew[2] += To_Y[2][1]*deltaV[1];
		}

		if (deltaV[2]!=0.0)	//Non-zero
		{
			YVsNew[0] += To_Y[0][2]*deltaV[2];
			YVsNew[1] += To_Y[1][2]*deltaV[2];
			YVsNew[2] += To_Y[2][2]*deltaV[2];
		}
	}
	else if (snodeside==2)		//To node
	{
		if (deltaV[0]!=0.0)	//Non-zero
		{
			YVsNew[0] = From_Y[0][0]*deltaV[0];
			YVsNew[1] = From_Y[1][0]*deltaV[0];
			YVsNew[2] = From_Y[2][0]*deltaV[0];
		}

		if (deltaV[1]!=0.0)	//Non-zero
		{
			YVsNew[0] += From_Y[0][1]*deltaV[1];
			YVsNew[1] += From_Y[1][1]*deltaV[1];
			YVsNew[2] += From_Y[2][1]*deltaV[1];
		}

		if (deltaV[2]!=0.0)	//Non-zero
		{
			YVsNew[0] += From_Y[0][2]*deltaV[2];
			YVsNew[1] += From_Y[1][2]*deltaV[2];
			YVsNew[2] += From_Y[2][2]*deltaV[2];
		}
	}

	//Check to see if we are a subnode (fixes backpostings)
	if (worknode->SubNode!=1)
	{
		LOCK_OBJECT(snode);
		worknode->YVs[0] += YVsNew[0];
		worknode->YVs[1] += YVsNew[1];
		worknode->YVs[2] += YVsNew[2];
		UNLOCK_OBJECT(snode);
	}
	else	//Child update
	{
		OBJECT *newnode = worknode->SubNodeParent;
		node *newworknode = OBJECTDATA(newnode,node);

		LOCK_OBJECT(newnode);
		newworknode->YVs[0] += YVsNew[0];
		newworknode->YVs[1] += YVsNew[1];
		newworknode->YVs[2] += YVsNew[2];
		UNLOCK_OBJECT(newnode);
	}
	return 0;
}

//Function to calculate current values for use by restoration module
void link::calc_currents(complex *Current_Vals)
{
	node *fnode = OBJECTDATA(from,node);
	node *tnode = OBJECTDATA(to,node);
	complex vtemp[3];
	complex itemp[3];
	complex current_temp[3];

	//Zero the values, in case we are using the same temp variable every time
	Current_Vals[0] = Current_Vals[1] = Current_Vals[2] = 0.0;

	//Perform current_in calculation from sync pass above (just no propogation to current_inj)
	if (status==LS_CLOSED)
	{
		if ((voltage_ratio!=1.0) && (SpecialLnk != DELTAGWYE) && (SpecialLnk != SPLITPHASE))
		{
			//(-a*Vout+Vin)
			vtemp[0] = fnode->voltage[0]-
					   a_mat[0][0]*tnode->voltage[0]-
					   a_mat[0][1]*tnode->voltage[1]-
					   a_mat[0][2]*tnode->voltage[2];

			vtemp[1] = fnode->voltage[1]-
					   a_mat[1][0]*tnode->voltage[0]-
					   a_mat[1][1]*tnode->voltage[1]-
					   a_mat[1][2]*tnode->voltage[2];

			vtemp[2] = fnode->voltage[2]-
					   a_mat[2][0]*tnode->voltage[0]-
					   a_mat[2][1]*tnode->voltage[1]-
					   a_mat[2][2]*tnode->voltage[2];

			Current_Vals[0] = d_mat[0][0]*vtemp[0]+
							  d_mat[0][1]*vtemp[1]+
							  d_mat[0][2]*vtemp[2];

			Current_Vals[1] = d_mat[1][0]*vtemp[0]+
							  d_mat[1][1]*vtemp[1]+
							  d_mat[1][2]*vtemp[2];

			Current_Vals[2] = d_mat[2][0]*vtemp[0]+
							  d_mat[2][1]*vtemp[1]+
							  d_mat[2][2]*vtemp[2];

		}//end normal transformers
		else if (SpecialLnk == DELTAGWYE)
		{
			vtemp[0]=fnode->voltage[0]*a_mat[0][0]+
					 fnode->voltage[1]*a_mat[0][1]+
					 fnode->voltage[2]*a_mat[0][2]-
					 tnode->voltage[0];

			vtemp[1]=fnode->voltage[0]*a_mat[1][0]+
					 fnode->voltage[1]*a_mat[1][1]+
					 fnode->voltage[2]*a_mat[1][2]-
					 tnode->voltage[1];

			vtemp[2]=fnode->voltage[0]*a_mat[2][0]+
					 fnode->voltage[1]*a_mat[2][1]+
					 fnode->voltage[2]*a_mat[2][2]-
					 tnode->voltage[2];

			//Get low side current
			itemp[0] = vtemp[0] * b_mat[0][0];
			itemp[1] = vtemp[1] * b_mat[1][1];
			itemp[2] = vtemp[2] * b_mat[2][2];

			//Translate back to high-side
			Current_Vals[0] = d_mat[0][0]*itemp[0]+
							  d_mat[0][1]*itemp[1]+
							  d_mat[0][2]*itemp[2];

			Current_Vals[1] = d_mat[1][0]*itemp[0]+
							  d_mat[1][1]*itemp[1]+
							  d_mat[1][2]*itemp[2];

			Current_Vals[2] = d_mat[2][0]*itemp[0]+
							  d_mat[2][1]*itemp[1]+
							  d_mat[2][2]*itemp[2];

		}//end delta-GWYE
		else if (SpecialLnk == SPLITPHASE)	//Split phase, center tapped xformer
		{
			if (has_phase(PHASE_A))
			{
				itemp[0] = fnode->voltage[0]*b_mat[2][2]+
						   tnode->voltage[0]*b_mat[2][0]+
						   tnode->voltage[1]*b_mat[2][1];

				Current_Vals[0] = itemp[0];
			}
			else if (has_phase(PHASE_B))
			{
				itemp[0] = fnode->voltage[1]*b_mat[2][2]+
						   tnode->voltage[0]*b_mat[2][0]+
						   tnode->voltage[1]*b_mat[2][1];

				Current_Vals[1] = itemp[0];
			}
			else if (has_phase(PHASE_C))
			{
				itemp[0] = fnode->voltage[2]*b_mat[2][2]+
						   tnode->voltage[0]*b_mat[2][0]+
						   tnode->voltage[1]*b_mat[2][1];

				Current_Vals[2] = itemp[0];
			}

		}//end split-phase, center tapped xformer
		else if (has_phase(PHASE_S))	//Split-phase line
		{
			//(-a*Vout+Vin)
			vtemp[0] = fnode->voltage[0]-
					   a_mat[0][0]*tnode->voltage[0]-
					   a_mat[0][1]*tnode->voltage[1];

			vtemp[1] = fnode->voltage[1]-
					   a_mat[1][0]*tnode->voltage[0]-
					   a_mat[1][1]*tnode->voltage[1];

			Current_Vals[0] = From_Y[0][0]*vtemp[0]+
							  From_Y[0][1]*vtemp[1];

			Current_Vals[1] = From_Y[1][0]*vtemp[0]+
							  From_Y[1][1]*vtemp[1];
		}
		else
		{
			//(-a*Vout+Vin)
			vtemp[0] = fnode->voltage[0]-
					   a_mat[0][0]*tnode->voltage[0]-
					   a_mat[0][1]*tnode->voltage[1]-
					   a_mat[0][2]*tnode->voltage[2];

			vtemp[1] = fnode->voltage[1]-
					   a_mat[1][0]*tnode->voltage[0]-
					   a_mat[1][1]*tnode->voltage[1]-
					   a_mat[1][2]*tnode->voltage[2];

			vtemp[2] = fnode->voltage[2]-
					   a_mat[2][0]*tnode->voltage[0]-
					   a_mat[2][1]*tnode->voltage[1]-
					   a_mat[2][2]*tnode->voltage[2];

			Current_Vals[0] = From_Y[0][0]*vtemp[0]+
							  From_Y[0][1]*vtemp[1]+
							  From_Y[0][2]*vtemp[2];

			Current_Vals[1] = From_Y[1][0]*vtemp[0]+
							  From_Y[1][1]*vtemp[1]+
							  From_Y[1][2]*vtemp[2];

			Current_Vals[2] = From_Y[2][0]*vtemp[0]+
							  From_Y[2][1]*vtemp[1]+
							  From_Y[2][2]*vtemp[2];
		}
	}
}

//Retrieve value of a double
double *link::get_double(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_double)
		return NULL;
	return (double*)GETADDR(obj,p);
}

//Function to create faults on link
// Supported faults enumeration:
// 0 - No fault
// 1 - Single Line Ground (SLG) - phase A
// 2 - SLG - phase B
// 3 - SLG - phase C
// 4 - Double Line Ground (DLG) - phases A and B
// 5 - DLG - phases B and C
// 6 - DLG - phases C and A
// 7 - Line to Line (LL) - phases A and B
// 8 - LL - phases B and C
// 9 - LL - phases C and A
// 10 - Triple Line Ground (TLG) - phases A, B, and C
// 11 - Open Conductor (OC) - phase A
// 12 - OC - phase B
// 13 - OC - phase C
// 14 - Two Open Conductors (OC2) - phases A and B
// 15 - OC2 - phases B and C
// 16 - OC2 - phases C and A
// 17 - Triple Open Conductor (OC3) - phases A, B, and C
// 18 - SW-A - Switch action A
// 19 - SW-B - Switch action B
// 20 - SW-C - Switch action C
// 21 - SW-AB or SW-BA - Switch action AB
// 22 - SW-BC or SW-CB - Switch action BC
// 23 - SW-CA or SW-AC - Switch action CA
// 24 - SW-ABC - Switch action ABC
// 25 - FUS-A - Fuse action A
// 26 - FUS-B - Fuse action B
// 27 - FUS-C - Fuse action C
// 28 - FUS-AB or FUS-BA - Fuse action AB
// 29 - FUS-BC or FUS-CB - Fuse action BC
// 30 - FUS-AC or FUS-CA - Fuse action CA
// 31 - FUS-ABC - Fuse action ABC
// 32 - TLL - all lines fault - phases A, B, and C
int link::link_fault_on(OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time, void *Extra_Data)
{
	unsigned char phase_remove = 0x00;	//Default is no phases removed
	unsigned char rand_phases,temp_phases, work_phases;			//Working variable
	char numphase, phaseidx;
	double randval, ext_result_dbl;
	double tempphase[3];
	double *temp_double_val;
	bool safety_hit;
	int temp_branch, temp_node, ext_result;
	unsigned int temp_table_loc;
	OBJECT *objhdr = OBJECTHDR(this);
	OBJECT *tmpobj;
	FUNCTIONADDR funadd = NULL;
	double *Recloser_Counts;
	bool switch_val;
	complex C_mat[7][7];

	// set defaults for C_mat
	for(int n=0; n<7; n++){
		for(int m=0; m<7; m++) {
			C_mat[n][m]=complex(0,0);
		}
	}
	C_mat[0][0]=C_mat[1][1]=C_mat[2][2]=complex(1,0);
	

	//Default switch_val - special case
	switch_val = false;

	//Protective device set to NULL (should already be this way, but just in case)
	*protect_obj = NULL;

	//Default repair time is non-existant
	*repair_time = 0;

	//Link up recloser counts for manipulation
	Recloser_Counts = (double *)Extra_Data;

	//Initial check - faults only work for NR right now
	if (solver_method != SM_NR)
	{
		GL_THROW("Only the NR solver supports link faults!");
		/*  TROUBLESHOOT
		At this time, only the Newton-Raphson solution method supports faults for link objects.
		Please utilize this solver method, or check back at a later date.
		*/
	}

	if ((fault_type[0] == 'S') && (fault_type[1] == 'L') && (fault_type[2] == 'G'))	//SLG - single-line-ground fault
	{
		//See which phase we want to fault - skip [3] - it should be a dash (if it isn't, we ignore it)
		//First see if it is an 'X' - for random phase.  If so, pick one and proceed
		if (fault_type[4] == 'X')
		{
			//Reset counter
			numphase = 0;

			//See how many phases we get to work with 0 populate the phase array at the same time
			if (has_phase(PHASE_A))
			{
				tempphase[numphase] = 4;	//Flag for phase A - NR convention
				numphase++;					//Increment count
			}

			if (has_phase(PHASE_B))
			{
				tempphase[numphase] = 2;	//Flag for phase B - NR convention
				numphase++;					//Increment count
			}

			if (has_phase(PHASE_C))
			{
				tempphase[numphase] = 1;	//Flag for phase C - NR convention
				numphase++;					//Increment count
			}

			//Get a random value
			randval = gl_random_sampled(RNGSTATE,numphase,tempphase);

			//Put it back into proper format
			rand_phases = (unsigned char)(randval);
		}
		else	//Not a random - clear the variable, just in case
			rand_phases = 0x00;

		if ((fault_type[4] == 'A') || (rand_phases == 0x04))
		{
			if (has_phase(PHASE_A))
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//make sure phase A is active (no previous fault)
				{
					//Remove phase A
					NR_branchdata[NR_branch_reference].phases &= 0xFB;

					//Flag which phase we've removed
					phase_remove = 0x04;

					//Flag the fault type
					*implemented_fault = 1;
				}
				else	//Already in a fault - just flag us as none and go on our way
				{
					*implemented_fault = 0;
				}
			}//end has PHASE_A
			else
			{
				gl_warning("%s does not have a phase A to fault!",OBJECTHDR(this)->name);
				/*  TROUBLESHOOT
				A fault event was attempted on phase A of the link object. This object
				does not have a valid phase A to fault.
				*/
			}
		}//End A fault
		else if ((fault_type[4] == 'B') || (rand_phases == 0x02))
		{
			if (has_phase(PHASE_B))
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//make sure phase B is active (no previous fault)
				{
					//Remove phase B
					NR_branchdata[NR_branch_reference].phases &= 0xFD;

					//Flag which phase we've removed
					phase_remove = 0x02;

					//Flag the fault type
					*implemented_fault = 2;
				}
				else	//Already in a fault - just flag us as none and go on our way
				{
					*implemented_fault = 0;
				}
			}//end has PHASE_B
			else
			{
				gl_warning("%s does not have a phase B to fault!",OBJECTHDR(this)->name);
				/*  TROUBLESHOOT
				A fault event was attempted on phase B of the link object. This object
				does not have a valid phase B to fault.
				*/
			}
		}
		else if ((fault_type[4] == 'C') || (rand_phases == 0x01))
		{
			if (has_phase(PHASE_C))
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//make sure phase C is active (no previous fault)
				{
					//Remove phase C
					NR_branchdata[NR_branch_reference].phases &= 0xFE;

					//Flag which phase we've removed
					phase_remove = 0x01;

					//Flag the fault type
					*implemented_fault = 3;
				}
				else	//Already in a fault - just flag us as none and go on our way
				{
					*implemented_fault = 0;
				}
			}//end has PHASE_C
			else
			{
				gl_warning("%s does not have a phase C to fault!",OBJECTHDR(this)->name);
				/*  TROUBLESHOOT
				A fault event was attempted on phase C of the link object. This object
				does not have a valid phase C to fault.
				*/
			}
		}
		else	//Something else, fail
		{
			GL_THROW("Fault type %s for link objects has an invalid phase specification");
			/*  TROUBLESHOOT
			The phase specified for the link fault is not a valid A, B, or C value.  Please check your syntax
			and ensure the values are uppercase.
			*/
		}
	}//end SLG
	else if ((fault_type[0] == 'D') && (fault_type[1] == 'L') && (fault_type[2] == 'G'))	//DLG - double-line-ground fault
	{
		//Figure out who we want to alter - assume [3] is a -, so check [4]+
		work_phases = 0x00;

		if (fault_type[4] == 'X')	//Random, flag as such
			work_phases |= 0x08;
		
		if ((fault_type[4] == 'A') || (fault_type[5] == 'A'))	//A is desired
			work_phases |= 0x04;

		if ((fault_type[4] == 'B') || (fault_type[5] == 'B'))	//B is desired
			work_phases |= 0x02;

		if ((fault_type[4] == 'C') || (fault_type[5] == 'C'))	//C is desired
			work_phases |= 0x01;

		//See how many phases we have to deal with
		numphase = 0;

		if (has_phase(PHASE_A))
		{
			numphase++;					//Increment count
		}

		if (has_phase(PHASE_B))
		{
			numphase++;					//Increment count
		}

		if (has_phase(PHASE_C))
		{
			numphase++;					//Increment count
		}

		//Pull out what phases we have to play with
		temp_phases = NR_branchdata[NR_branch_reference].phases & 0x07;

		if (numphase == 0)
		{
			GL_THROW("No phases detected for %s!",objhdr->name);
			/*  TROUBLESHOOT
			No phases were detected for the link object specified.  This should have
			been caught much earlier.  Please submit your code and a bug report using the
			trac website.
			*/
		}//end 0 phase
		else if (numphase < 2)	//Single phase line (no zero phase this way)
		{
			//Refine our criteria
			if ((work_phases & 0x08) != 0x08)	//Not random case
				temp_phases &= work_phases;

			//defaulted else - if random case, only getting one phase out of this anywho (leave temp_phases as is)

			switch (temp_phases)
			{
			case 0x00:	//No phases available - something must have faulted
				*implemented_fault = 0;	//Flag as such
				break;
			case 0x01:	//Phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 3;								//Flag as a C SLG fault
				break;
			case 0x02:	//Phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 2;								//Flag as a B SLG fault
				break;
			case 0x04:	//Phase A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 1;								//Flag as a A SLG fault
				break;
			default:	//No other cases should exist
				GL_THROW("Fault type %s for link objects has an invalid phase specification");
				//Defined above
				break;
			}//end switch

			phase_remove = temp_phases;	//Flag phase removing

		}//end <2 phases (only 1 presumably)
		else if (numphase == 2)	//Only two phases present
		{
			//See how the two present coincide with the two we're asking for
			if ((work_phases & 0x08) != 0x08)	//Not random case, see what we have
				temp_phases &= work_phases;
			//Defaulted else - if random, only 1 choice exists, which is temp_phases (leave temp_phases as is)

			//Implement the appropriate fault
			switch (temp_phases)
			{
			case 0x00:	//No phases!
				*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
				break;
			case 0x01:	//Only phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 3;								//Flag as a C SLG fault
				break;
			case 0x02:	//Only phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 2;								//Flag as a B SLG fault
				break;
			case 0x03:	//B and C
				NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
				*implemented_fault = 5;								//Flag as a B & C fault
				break;
			case 0x04:	//Only A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 1;								//Flag as a A SLG fault
				break;
			case 0x05:	//A and C
				NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
				*implemented_fault = 6;								//Flag as A and C fault
				break;
			case 0x06:	//A and B
				NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
				*implemented_fault = 4;								//Flag as A and B fault
				break;
			default:	//Not sure how we'd ever get here
				GL_THROW("Unknown phase condition on two-phase fault of %s!",objhdr->name);
				/*  TROUBLESHOOT
				While attempting to implement a two-phase fault on a link object, an unknown phase
				configuration was encountered.  Please try again.  If the error persists, please submit
				your code and a bug report using the trac website.
				*/
				break;
			}//end switch

			phase_remove = temp_phases;	//Flag phase removing

		}//end 2 phases present
		else if (numphase == 3)	//All phases present
		{
			if ((work_phases & 0x08) == 0x08)	//Random condition
			{
				//See if we have all three available to fault (if one or more is already faulted, we won't care)
				rand_phases = 0;

				//Check and populate random array as well
				if ((temp_phases & 0x01) == 0x01)	//Check for C availability
				{
					tempphase[rand_phases] = 1;
					rand_phases++;
				}

				if ((temp_phases & 0x02) == 0x02)	//Check for B availability
				{
					tempphase[rand_phases] = 2;
					rand_phases++;
				}

				if ((temp_phases & 0x04) == 0x04)	//Check for A availability
				{
					tempphase[rand_phases] = 4;
					rand_phases++;
				}

				if (rand_phases <= 2)	//Two or fewer available, just mask us out
					work_phases = 0x07;	//This will pull the appropriate phase
				else	//Must be 3, right?
				{
					//Pick a random phase - this will be the one we DON'T fault
					randval = gl_random_sampled(RNGSTATE,numphase,tempphase);

					//Set temp_phases appropriate
					if (randval == 1)	//Leave phase C
						work_phases = 0x06;	//A and B fault
					else if (randval == 2)	//Leave phase B
						work_phases = 0x05;	//A and C fault
					else	//Must be 4	- leave phase A
						work_phases = 0x03;	//B and C fault
				}//end must be 3
			}//end random condition
			//Defaulted else - not random, so just use as normal mask

			//Determine phases to fault
			temp_phases &= work_phases;

			//Implement the appropriate fault
			switch (temp_phases)
			{
			case 0x00:	//No phases!
				*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
				break;
			case 0x01:	//Only phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 3;								//Flag as a C SLG fault
				break;
			case 0x02:	//Only phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 2;								//Flag as a B SLG fault
				break;
			case 0x03:	//B and C
				NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
				*implemented_fault = 5;								//Flag as a B & C fault
				break;
			case 0x04:	//Only A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 1;								//Flag as a A SLG fault
				break;
			case 0x05:	//A and C
				NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
				*implemented_fault = 6;								//Flag as A and C fault
				break;
			case 0x06:	//A and B
				NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
				*implemented_fault = 4;								//Flag as A and B fault
				break;
			default:	//Not sure how we'd ever get here
				GL_THROW("Unknown phase condition on two-phase fault of %s!",objhdr->name);
				//Defined above
				break;
			}//end switch

			phase_remove = temp_phases;	//Flag phase removing

		}//end all three present
		else	//Hmmm, how'd we get here?
		{
			GL_THROW("An invalid number of phases appears present for %s",objhdr->name);
			/*  TROUBLESHOOT
			An unexpected number of phases was found on the link object.  Please submit your
			code and a bug report using the trac website.
			*/
		}
	}//End DLG
	else if ((fault_type[0] == 'T') && (fault_type[1] == 'L') && (fault_type[2] == 'G'))	//TLG - triple-line-ground fault
	{
		//Let's see what phases we have to play with
		temp_phases = NR_branchdata[NR_branch_reference].phases & 0x07;

		//Case it out - if we want TLG, but don't have all three phases, just trip whatever we have
		switch (temp_phases)
		{
		case 0x00:	//No phases!
			*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
			break;
		case 0x01:	//Only phase C
			NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
			*implemented_fault = 3;								//Flag as a C only fault
			break;
		case 0x02:	//Only phase B
			NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
			*implemented_fault = 2;								//Flag as a B only fault
			break;
		case 0x03:	//B and C
			NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
			*implemented_fault = 5;								//Flag as a B & C fault
			break;
		case 0x04:	//Only A
			NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
			*implemented_fault = 1;								//Flag as a A only fault
			break;
		case 0x05:	//A and C
			NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
			*implemented_fault = 6;								//Flag as A and C fault
			break;
		case 0x06:	//A and B
			NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
			*implemented_fault = 4;								//Flag as A and B fault
			break;
		case 0x07:	//A, B, and C
			NR_branchdata[NR_branch_reference].phases &= 0xF8;	//Remove A, B, and C
			*implemented_fault = 10;							//Flag as all three fault
			break;
		default:	//Not sure how we'd ever get here
			GL_THROW("Unknown phase condition on three-phase fault of %s!",objhdr->name);
			/*  TROUBLESHOOT
			While attempting to implement a three-phase fault on a link object, an unknown phase
			configuration was encountered.  Please try again.  If the error persists, please submit
			your code and a bug report using the trac website.
			*/
			break;
		}//end phase cases

		phase_remove = temp_phases;	//Flag phase removing

	}//End TLG
	else if ((fault_type[0] == 'T') && (fault_type[1] == 'L') && (fault_type[2] == 'L'))	//TLL - triple-line-line fault
	{
		//Let's see what phases we have to play with
		temp_phases = NR_branchdata[NR_branch_reference].phases & 0x07;

		//Case it out - if we want TLG, but don't have all three phases, just trip whatever we have
		switch (temp_phases)
		{
		case 0x00:	//No phases!
			*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
			break;
		case 0x01:	//Only phase C
			NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
			*implemented_fault = 13;							//Flag as a C only OC fault
			break;
		case 0x02:	//Only phase B
			NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
			*implemented_fault = 12;							//Flag as a B only OC fault
			break;
		case 0x03:	//B and C
			NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
			*implemented_fault = 8;								//Flag as a B & C LL fault
			break;
		case 0x04:	//Only A
			NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
			*implemented_fault = 11;							//Flag as a A only OC fault
			break;
		case 0x05:	//A and C
			NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
			*implemented_fault = 9;								//Flag as A and C LL fault
			break;
		case 0x06:	//A and B
			NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
			*implemented_fault = 7;								//Flag as A and B LL fault
			break;
		case 0x07:	//A, B, and C
			NR_branchdata[NR_branch_reference].phases &= 0xF8;	//Remove A, B, and C
			*implemented_fault = 32;							//Flag as all three fault
			break;
		default:	//Not sure how we'd ever get here
			GL_THROW("Unknown phase condition on three-phase fault of %s!",objhdr->name);
			//Defined elsewhere
			break;
		}//end phase cases

		phase_remove = temp_phases;	//Flag phase removing

	}//End TLL
	else if ((fault_type[0] == 'L') && (fault_type[1] == 'L'))	//Line-line fault
	{
		//Figure out who we want to alter - assume [2] is a -, so check [3]+
		work_phases = 0x00;

		if (fault_type[3] == 'X')	//Random, flag as such
			work_phases |= 0x08;
		
		if ((fault_type[3] == 'A') || (fault_type[4] == 'A'))	//A is desired
			work_phases |= 0x04;

		if ((fault_type[3] == 'B') || (fault_type[4] == 'B'))	//B is desired
			work_phases |= 0x02;

		if ((fault_type[3] == 'C') || (fault_type[4] == 'C'))	//C is desired
			work_phases |= 0x01;

		//See how many phases we have to deal with
		numphase = 0;

		if (has_phase(PHASE_A))
		{
			numphase++;					//Increment count
		}

		if (has_phase(PHASE_B))
		{
			numphase++;					//Increment count
		}

		if (has_phase(PHASE_C))
		{
			numphase++;					//Increment count
		}

		//Pull out what phases we have to play with
		temp_phases = NR_branchdata[NR_branch_reference].phases & 0x07;

		if (numphase == 0)
		{
			GL_THROW("No phases detected for %s!",objhdr->name);
			//Defined above
		}//end 0 phase
		else if (numphase < 2)	//Single phase line (no zero phase this way)
		{
			//Refine our criteria
			if ((work_phases & 0x08) != 0x08)	//Not random case
				temp_phases &= work_phases;

			//defaulted else - if random case, only getting one phase out of this anywho (leave temp_phases as is)

			switch (temp_phases)
			{
			case 0x00:	//No phases available - something must have faulted
				*implemented_fault = 0;	//Flag as such
				break;
			case 0x01:	//Phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 13;							//Flag as a C OC fault
				break;
			case 0x02:	//Phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 12;							//Flag as a B OC fault
				break;
			case 0x04:	//Phase A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 11;							//Flag as a A OC fault
				break;
			default:	//No other cases should exist
				GL_THROW("Fault type %s for link objects has an invalid phase specification");
				//Defined above
				break;
			}//end switch

			phase_remove = temp_phases;	//Flag phase removing
		}//end <2 phases (only 1 presumably)
		else if (numphase == 2)	//Only two phases present
		{
			//See how the two present coincide with the two we're asking for
			if ((work_phases & 0x08) != 0x08)	//Not random case, see what we have
				temp_phases &= work_phases;
			//Defaulted else - if random, only 1 choice exists, which is temp_phases (leave temp_phases as is)

			//Implement the appropriate fault
			switch (temp_phases)
			{
			case 0x00:	//No phases!
				*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
				break;
			case 0x01:	//Only phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 13;							//Flag as a C OC fault
				break;
			case 0x02:	//Only phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 12;							//Flag as a B OC fault
				break;
			case 0x03:	//B and C
				NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
				*implemented_fault = 8;								//Flag as a B & C fault
				break;
			case 0x04:	//Only A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 11;							//Flag as a A OC fault
				break;
			case 0x05:	//A and C
				NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
				*implemented_fault = 9;								//Flag as A and C fault
				break;
			case 0x06:	//A and B
				NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
				*implemented_fault = 7;								//Flag as A and B fault
				break;
			default:	//Not sure how we'd ever get here
				GL_THROW("Unknown phase condition on two-phase fault of %s!",objhdr->name);
				//Defined above
				break;
			}//end switch

			phase_remove = temp_phases;	//Flag phase removing

		}//end 2 phases present
		else if (numphase == 3)	//All phases present
		{
			if ((work_phases & 0x08) == 0x08)	//Random condition
			{
				//See if we have all three available to fault (if one or more is already faulted, we won't care)
				rand_phases = 0;

				//Check and populate random array as well
				if ((temp_phases & 0x01) == 0x01)	//Check for C availability
				{
					tempphase[rand_phases] = 1;
					rand_phases++;
				}

				if ((temp_phases & 0x02) == 0x02)	//Check for B availability
				{
					tempphase[rand_phases] = 2;
					rand_phases++;
				}

				if ((temp_phases & 0x04) == 0x04)	//Check for A availability
				{
					tempphase[rand_phases] = 4;
					rand_phases++;
				}

				if (rand_phases <= 2)	//Two or fewer available, just mask us out
					work_phases = 0x07;	//This will pull the appropriate phase
				else	//Must be 3, right?
				{
					//Pick a random phase - this will be the one we DON'T fault
					randval = gl_random_sampled(RNGSTATE,numphase,tempphase);

					//Set temp_phases appropriate
					if (randval == 1)	//Leave phase C
						work_phases = 0x06;	//A and B fault
					else if (randval == 2)	//Leave phase B
						work_phases = 0x05;	//A and C fault
					else	//Must be 4	- leave phase A
						work_phases = 0x03;	//B and C fault
				}//end must be 3
			}//end random condition
			//Defaulted else - not random, so just use as normal mask

			//Determine phases to fault
			temp_phases &= work_phases;

			//Implement the appropriate fault
			switch (temp_phases)
			{
			case 0x00:	//No phases!
				*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
				break;
			case 0x01:	//Only phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 13;							//Flag as a C OC fault
				break;
			case 0x02:	//Only phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 12;							//Flag as a B OC fault
				break;
			case 0x03:	//B and C
				NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
				*implemented_fault = 8;								//Flag as a B & C fault
				break;
			case 0x04:	//Only A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 11;							//Flag as an A OC fault
				break;
			case 0x05:	//A and C
				NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
				*implemented_fault = 9;								//Flag as A and C fault
				break;
			case 0x06:	//A and B
				NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
				*implemented_fault = 7;								//Flag as A and B fault
				break;
			default:	//Not sure how we'd ever get here
				GL_THROW("Unknown phase condition on two-phase fault of %s!",objhdr->name);
				//Defined above
				break;
			}//end switch

			phase_remove = temp_phases;	//Flag phase removing

		}//end all three present
		else	//Hmmm, how'd we get here?
		{
			GL_THROW("An invalid number of phases appears present for %s",objhdr->name);
			//Defined above
		}
	}//End LL
	else if (((fault_type[0] == 'O') && (fault_type[1] == 'C') && (fault_type[2] == '-')) || ((fault_type[0] == 'O') && (fault_type[1] == 'C') && (fault_type[2] == '1')))	//Open conductor
	{
		//See which phase we want to fault - skip [3] if OC1, or [2] if OC - it should be a dash (if it isn't, we ignore it)
		if ((fault_type[0] == 'O') && (fault_type[1] == 'C') && (fault_type[2] == '-'))
			phaseidx=3;
		else	//Must be OC1- then
			phaseidx=4;

		//First see if it is an 'X' - for random phase.  If so, pick one and proceed
		if (fault_type[phaseidx] == 'X')
		{
			//Reset counter
			numphase = 0;

			//See how many phases we get to work with 0 populate the phase array at the same time
			if (has_phase(PHASE_A))
			{
				tempphase[numphase] = 4;	//Flag for phase A - NR convention
				numphase++;					//Increment count
			}

			if (has_phase(PHASE_B))
			{
				tempphase[numphase] = 2;	//Flag for phase B - NR convention
				numphase++;					//Increment count
			}

			if (has_phase(PHASE_C))
			{
				tempphase[numphase] = 1;	//Flag for phase C - NR convention
				numphase++;					//Increment count
			}

			//Get a random value
			randval = gl_random_sampled(RNGSTATE,numphase,tempphase);

			//Put it back into proper format
			rand_phases = (unsigned char)(randval);
		}
		else	//Not a random - clear the variable, just in case
			rand_phases = 0x00;

		if ((fault_type[phaseidx] == 'A') || (rand_phases == 0x04))
		{
			if (has_phase(PHASE_A))
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//make sure phase A is active (no previous fault)
				{
					//Remove phase A
					NR_branchdata[NR_branch_reference].phases &= 0xFB;

					phase_remove = 0x04;	//Flag phase being removed

					//Flag the fault type
					*implemented_fault = 11;
				}
				else	//Already in a fault - just flag us as none and go on our way
				{
					*implemented_fault = 0;
				}
			}//end has PHASE_A
			else
			{
				gl_warning("%s does not have a phase A to fault!",OBJECTHDR(this)->name);
				//Defined above
			}
		}//End A fault
		else if ((fault_type[phaseidx] == 'B') || (rand_phases == 0x02))
		{
			if (has_phase(PHASE_B))
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//make sure phase B is active (no previous fault)
				{
					//Remove phase B
					NR_branchdata[NR_branch_reference].phases &= 0xFD;

					//Flag phase being removed
					phase_remove = 0x02;

					//Flag the fault type
					*implemented_fault = 12;
				}
				else	//Already in a fault - just flag us as none and go on our way
				{
					*implemented_fault = 0;
				}
			}//end has PHASE_B
			else
			{
				gl_warning("%s does not have a phase B to fault!",OBJECTHDR(this)->name);
				//Defined above
			}
		}
		else if ((fault_type[phaseidx] == 'C') || (rand_phases == 0x01))
		{
			if (has_phase(PHASE_C))
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//make sure phase C is active (no previous fault)
				{
					//Remove phase C
					NR_branchdata[NR_branch_reference].phases &= 0xFE;

					//Flag phase being removed
					phase_remove = 0x01;

					//Flag the fault type
					*implemented_fault = 13;
				}
				else	//Already in a fault - just flag us as none and go on our way
				{
					*implemented_fault = 0;
				}
			}//end has PHASE_C
			else
			{
				gl_warning("%s does not have a phase C to fault!",OBJECTHDR(this)->name);
				//defined above
			}
		}
		else	//Something else, fail
		{
			GL_THROW("Fault type %s for link objects has an invalid phase specification");
			//Defined above
		}
	}//End OC1
	else if ((fault_type[0] == 'O') && (fault_type[1] == 'C') && (fault_type[2] == '2'))	//Double open-conductor fault
	{
		//Figure out who we want to alter - assume [3] is a -, so check [4]+
		work_phases = 0x00;

		if (fault_type[4] == 'X')	//Random, flag as such
			work_phases |= 0x08;
		
		if ((fault_type[4] == 'A') || (fault_type[5] == 'A'))	//A is desired
			work_phases |= 0x04;

		if ((fault_type[4] == 'B') || (fault_type[5] == 'B'))	//B is desired
			work_phases |= 0x02;

		if ((fault_type[4] == 'C') || (fault_type[5] == 'C'))	//C is desired
			work_phases |= 0x01;

		//See how many phases we have to deal with
		numphase = 0;

		if (has_phase(PHASE_A))
		{
			numphase++;					//Increment count
		}

		if (has_phase(PHASE_B))
		{
			numphase++;					//Increment count
		}

		if (has_phase(PHASE_C))
		{
			numphase++;					//Increment count
		}

		//Pull out what phases we have to play with
		temp_phases = NR_branchdata[NR_branch_reference].phases & 0x07;

		if (numphase == 0)
		{
			GL_THROW("No phases detected for %s!",objhdr->name);
			//Defined above
		}//end 0 phase
		else if (numphase < 2)	//Single phase line (no zero phase this way)
		{
			//Refine our criteria
			if ((work_phases & 0x08) != 0x08)	//Not random case
				temp_phases &= work_phases;

			//defaulted else - if random case, only getting one phase out of this anywho (leave temp_phases as is)

			switch (temp_phases)
			{
			case 0x00:	//No phases available - something must have faulted
				*implemented_fault = 0;	//Flag as such
				break;
			case 0x01:	//Phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 13;							//Flag as a C OC fault
				break;
			case 0x02:	//Phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 12;							//Flag as a B OC fault
				break;
			case 0x04:	//Phase A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 11;							//Flag as a A OC fault
				break;
			default:	//No other cases should exist
				GL_THROW("Fault type %s for link objects has an invalid phase specification");
				//Defined above
				break;
			}//end switch

			phase_remove = temp_phases;	//Flag phase removing

		}//end <2 phases (only 1 presumably)
		else if (numphase == 2)	//Only two phases present
		{
			//See how the two present coincide with the two we're asking for
			if ((work_phases & 0x08) != 0x08)	//Not random case, see what we have
				temp_phases &= work_phases;
			//Defaulted else - if random, only 1 choice exists, which is temp_phases (leave temp_phases as is)

			//Implement the appropriate fault
			switch (temp_phases)
			{
			case 0x00:	//No phases!
				*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
				break;
			case 0x01:	//Only phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 13;							//Flag as a C OC fault
				break;
			case 0x02:	//Only phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 12;							//Flag as a B OC fault
				break;
			case 0x03:	//B and C
				NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
				*implemented_fault = 15;							//Flag as a B & C fault
				break;
			case 0x04:	//Only A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 11;							//Flag as a A OC fault
				break;
			case 0x05:	//A and C
				NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
				*implemented_fault = 16;							//Flag as A and C fault
				break;
			case 0x06:	//A and B
				NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
				*implemented_fault = 14;							//Flag as A and B fault
				break;
			default:	//Not sure how we'd ever get here
				GL_THROW("Unknown phase condition on two-phase fault of %s!",objhdr->name);
				//Defined above
				break;
			}//end switch

			phase_remove = temp_phases;	//Flag phase removing

		}//end 2 phases present
		else if (numphase == 3)	//All phases present
		{
			if ((work_phases & 0x08) == 0x08)	//Random condition
			{
				//See if we have all three available to fault (if one or more is already faulted, we won't care)
				rand_phases = 0;

				//Check and populate random array as well
				if ((temp_phases & 0x01) == 0x01)	//Check for C availability
				{
					tempphase[rand_phases] = 1;
					rand_phases++;
				}

				if ((temp_phases & 0x02) == 0x02)	//Check for B availability
				{
					tempphase[rand_phases] = 2;
					rand_phases++;
				}

				if ((temp_phases & 0x04) == 0x04)	//Check for A availability
				{
					tempphase[rand_phases] = 4;
					rand_phases++;
				}

				if (rand_phases <= 2)	//Two or fewer available, just mask us out
					work_phases = 0x07;	//This will pull the appropriate phase
				else	//Must be 3, right?
				{
					//Pick a random phase - this will be the one we DON'T fault
					randval = gl_random_sampled(RNGSTATE,numphase,tempphase);

					//Set temp_phases appropriate
					if (randval == 1)	//Leave phase C
						work_phases = 0x06;	//A and B fault
					else if (randval == 2)	//Leave phase B
						work_phases = 0x05;	//A and C fault
					else	//Must be 4	- leave phase A
						work_phases = 0x03;	//B and C fault
				}//end must be 3
			}//end random condition
			//Defaulted else - not random, so just use as normal mask

			//Determine phases to fault
			temp_phases &= work_phases;

			//Implement the appropriate fault
			switch (temp_phases)
			{
			case 0x00:	//No phases!
				*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
				break;
			case 0x01:	//Only phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 13;							//Flag as a C OC fault
				break;
			case 0x02:	//Only phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 12;							//Flag as a B OC fault
				break;
			case 0x03:	//B and C
				NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
				*implemented_fault = 15;							//Flag as a B & C fault
				break;
			case 0x04:	//Only A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 11;							//Flag as a A OC fault
				break;
			case 0x05:	//A and C
				NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
				*implemented_fault = 16;							//Flag as A and C fault
				break;
			case 0x06:	//A and B
				NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
				*implemented_fault = 14;							//Flag as A and B fault
				break;
			default:	//Not sure how we'd ever get here
				GL_THROW("Unknown phase condition on two-phase fault of %s!",objhdr->name);
				//Defined above
				break;
			}//end switch

			phase_remove = temp_phases;	//Flag phase removing

		}//end all three present
		else	//Hmmm, how'd we get here?
		{
			GL_THROW("An invalid number of phases appears present for %s",objhdr->name);
			//Defined above
		}
	}//End OC2
	else if ((fault_type[0] == 'O') && (fault_type[1] == 'C') && (fault_type[2] == '3'))	//Triple open-conductor fault
	{
		//Let's see what phases we have to play with
		temp_phases = NR_branchdata[NR_branch_reference].phases & 0x07;

		//Case it out - if we want TLG, but don't have all three phases, just trip whatever we have
		switch (temp_phases)
		{
		case 0x00:	//No phases!
			*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
			break;
		case 0x01:	//Only phase C
			NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
			*implemented_fault = 13;							//Flag as a C only fault
			break;
		case 0x02:	//Only phase B
			NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
			*implemented_fault = 12;							//Flag as a B only fault
			break;
		case 0x03:	//B and C
			NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
			*implemented_fault = 15;							//Flag as a B & C fault
			break;
		case 0x04:	//Only A
			NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
			*implemented_fault = 11;							//Flag as a A only fault
			break;
		case 0x05:	//A and C
			NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
			*implemented_fault = 16;							//Flag as A and C fault
			break;
		case 0x06:	//A and B
			NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
			*implemented_fault = 14;							//Flag as A and B fault
			break;
		case 0x07:	//A, B, and C
			NR_branchdata[NR_branch_reference].phases &= 0xF8;	//Remove A, B, and C
			*implemented_fault = 17;							//Flag as all three fault
			break;
		default:	//Not sure how we'd ever get here
			GL_THROW("Unknown phase condition on three-phase fault of %s!",objhdr->name);
			//Defined above
			break;
		}//end phase cases

		phase_remove = temp_phases;	//Flag phase removing

	}//End OC3
	else if ((fault_type[0] == 'S') && (fault_type[1] == 'W') && (fault_type[2] == '-'))	//Switch operations - event or user induced (no random - so handled slightly different)
	{
		//Determine which scenario we're in
		if ((fault_type[3] == 'A') && (fault_type[4] == '\0'))	//Phase A occurance
		{
			if (has_phase(PHASE_A))
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//make sure phase A is active (no previous fault)
				{
					//Remove phase A
					NR_branchdata[NR_branch_reference].phases &= 0xFB;

					phase_remove = 0x04;	//Flag phase being removed

					//Flag the fault type
					*implemented_fault = 18;
				}
				else	//Already in a fault - just flag us as none and go on our way
				{
					*implemented_fault = 0;
				}
			}//end has PHASE_A
			else
			{
				gl_warning("%s does not have a phase A to fault!",OBJECTHDR(this)->name);
				//Defined above
			}
		}
		else if ((fault_type[3] == 'B') && (fault_type[4] == '\0'))	//Phase B occurance
		{
			if (has_phase(PHASE_B))
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//make sure phase B is active (no previous fault)
				{
					//Remove phase B
					NR_branchdata[NR_branch_reference].phases &= 0xFD;

					phase_remove = 0x02;	//Flag phase being removed

					//Flag the fault type
					*implemented_fault = 19;
				}
				else	//Already in a fault - just flag us as none and go on our way
				{
					*implemented_fault = 0;
				}
			}//end has PHASE_B
			else
			{
				gl_warning("%s does not have a phase B to fault!",OBJECTHDR(this)->name);
				//Defined above
			}
		}
		else if ((fault_type[3] == 'C') && (fault_type[4] == '\0'))	//Phase C occurance
		{
			if (has_phase(PHASE_C))
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//make sure phase C is active (no previous fault)
				{
					//Remove phase C
					NR_branchdata[NR_branch_reference].phases &= 0xFE;

					phase_remove = 0x01;	//Flag phase being removed

					//Flag the fault type
					*implemented_fault = 20;
				}
				else	//Already in a fault - just flag us as none and go on our way
				{
					*implemented_fault = 0;
				}
			}//end has PHASE_C
			else
			{
				gl_warning("%s does not have a phase C to fault!",OBJECTHDR(this)->name);
				//Defined above
			}
		}
		else if (fault_type[5] == '\0')	//Two Phase occurance
		{
			//Figure out who we want to alter - assume [2] is a -, so check [3]+
			work_phases = 0x00;

			if ((fault_type[3] == 'A') || (fault_type[4] == 'A'))	//A is desired
				work_phases |= 0x04;

			if ((fault_type[3] == 'B') || (fault_type[4] == 'B'))	//B is desired
				work_phases |= 0x02;

			if ((fault_type[3] == 'C') || (fault_type[4] == 'C'))	//C is desired
				work_phases |= 0x01;

			//See how many phases we have to deal with
			numphase = 0;

			if (has_phase(PHASE_A))
			{
				numphase++;					//Increment count
			}

			if (has_phase(PHASE_B))
			{
				numphase++;					//Increment count
			}

			if (has_phase(PHASE_C))
			{
				numphase++;					//Increment count
			}

			//Pull out what phases we have to play with
			temp_phases = NR_branchdata[NR_branch_reference].phases & 0x07;

			if (numphase == 0)
			{
				GL_THROW("No phases detected for %s!",objhdr->name);
				//Defined above
			}//end 0 phase
			else if (numphase < 2)	//Single phase line (no zero phase this way)
			{
				//See how the present coincide with the two we're asking for
				temp_phases &= work_phases;

				//Figure out what is going on
				switch (temp_phases)
				{
				case 0x00:	//No phases available - something must have faulted
					*implemented_fault = 0;	//Flag as such
					break;
				case 0x01:	//Phase C
					NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
					*implemented_fault = 20;							//Flag as a C switching
					break;
				case 0x02:	//Phase B
					NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
					*implemented_fault = 19;							//Flag as a B switching
					break;
				case 0x04:	//Phase A
					NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
					*implemented_fault = 18;							//Flag as a A switching
					break;
				default:	//No other cases should exist
					GL_THROW("Fault type %s for link objects has an invalid phase specification");
					//Defined above
					break;
				}//end switch

				phase_remove = temp_phases;	//Flag phase removing

			}//end <2 phases (only 1 presumably)
			else if (numphase == 2)	//Only two phases present
			{
				//See how the two present coincide with the two we're asking for
				temp_phases &= work_phases;

				//Implement the appropriate fault
				switch (temp_phases)
				{
				case 0x00:	//No phases!
					*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
					break;
				case 0x01:	//Only phase C
					NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
					*implemented_fault = 20;							//Flag as a C switching
					break;
				case 0x02:	//Only phase B
					NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
					*implemented_fault = 19;							//Flag as a B switching
					break;
				case 0x03:	//B and C
					NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
					*implemented_fault = 22;							//Flag as a B & C switching
					break;
				case 0x04:	//Only A
					NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
					*implemented_fault = 18;							//Flag as a A switching
					break;
				case 0x05:	//A and C
					NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
					*implemented_fault = 23;							//Flag as A and C switching
					break;
				case 0x06:	//A and B
					NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
					*implemented_fault = 21;							//Flag as A and B switching
					break;
				default:	//Not sure how we'd ever get here
					GL_THROW("Unknown phase condition on two-phase fault of %s!",objhdr->name);
					//Defined above
					break;
				}//end switch

				phase_remove = temp_phases;	//Flag phase removing

			}//end 2 phases present
			else if (numphase == 3)	//All phases present
			{
				//Determine phases to fault
				temp_phases &= work_phases;

				//Implement the appropriate fault
				switch (temp_phases)
				{
				case 0x00:	//No phases!
					*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
					break;
				case 0x01:	//Only phase C
					NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
					*implemented_fault = 20;							//Flag as a C switching
					break;
				case 0x02:	//Only phase B
					NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
					*implemented_fault = 19;							//Flag as a B switching
					break;
				case 0x03:	//B and C
					NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
					*implemented_fault = 22;							//Flag as a B & C fault
					break;
				case 0x04:	//Only A
					NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
					*implemented_fault = 18;							//Flag as a A switching
					break;
				case 0x05:	//A and C
					NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
					*implemented_fault = 23;							//Flag as A and C switching
					break;
				case 0x06:	//A and B
					NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
					*implemented_fault = 21;							//Flag as A and B switching
					break;
				default:	//Not sure how we'd ever get here
					GL_THROW("Unknown phase condition on two-phase fault of %s!",objhdr->name);
					//Defined above
					break;
				}//end switch

				phase_remove = temp_phases;	//Flag phase removing

			}//end all three present
			else	//Hmmm, how'd we get here?
			{
				GL_THROW("An invalid number of phases appears present for %s",objhdr->name);
				//Defined above
			}
		}//End Switch two phases
		else if (fault_type[6] == '\0')	//Three Phase occurance
		{
			//Let's see what phases we have to play with
			temp_phases = NR_branchdata[NR_branch_reference].phases & 0x07;

			//Case it out - if we want three-phase switching, but don't have all three phases, just switch whatever we have
			switch (temp_phases)
			{
			case 0x00:	//No phases!
				*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
				break;
			case 0x01:	//Only phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 20;							//Flag as a C only switching
				break;
			case 0x02:	//Only phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 19;							//Flag as a B only switching
				break;
			case 0x03:	//B and C
				NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
				*implemented_fault = 22;							//Flag as a B & C switching
				break;
			case 0x04:	//Only A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 18;							//Flag as a A only switching
				break;
			case 0x05:	//A and C
				NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
				*implemented_fault = 23;							//Flag as A and C switching
				break;
			case 0x06:	//A and B
				NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
				*implemented_fault = 21;							//Flag as A and B switching
				break;
			case 0x07:	//A, B, and C
				NR_branchdata[NR_branch_reference].phases &= 0xF8;	//Remove A, B, and C
				*implemented_fault = 24;							//Flag as all three switching
				break;
			default:	//Not sure how we'd ever get here
				GL_THROW("Unknown phase condition on three-phase fault of %s!",objhdr->name);
				//Defined above
				break;
			}//end phase cases

			phase_remove = temp_phases;	//Flag phase removing
		}//End three-phase occurance

		//Flag as special case
		switch_val = true;

	}//End switches
	else if ((fault_type[0] == 'F') && (fault_type[1] == 'U') && (fault_type[2] == 'S') && (fault_type[3] == '-') && (fault_type[5] == '\0'))	//Single phase fuse fault
	{
		//First see if it is an 'X' - for random phase.  If so, pick one and proceed
		if (fault_type[4] == 'X')
		{
			//Reset counter
			numphase = 0;

			//See how many phases we get to work with 0 populate the phase array at the same time
			if (has_phase(PHASE_A))
			{
				tempphase[numphase] = 4;	//Flag for phase A - NR convention
				numphase++;					//Increment count
			}

			if (has_phase(PHASE_B))
			{
				tempphase[numphase] = 2;	//Flag for phase B - NR convention
				numphase++;					//Increment count
			}

			if (has_phase(PHASE_C))
			{
				tempphase[numphase] = 1;	//Flag for phase C - NR convention
				numphase++;					//Increment count
			}

			//Get a random value
			randval = gl_random_sampled(RNGSTATE,numphase,tempphase);

			//Put it back into proper format
			rand_phases = (unsigned char)(randval);
		}
		else	//Not a random - clear the variable, just in case
			rand_phases = 0x00;

		if ((fault_type[4] == 'A') || (rand_phases == 0x04))
		{
			if (has_phase(PHASE_A))
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//make sure phase A is active (no previous fault)
				{
					//Remove phase A
					NR_branchdata[NR_branch_reference].phases &= 0xFB;

					phase_remove = 0x04;	//Flag phase being removed

					//Flag the fault type
					*implemented_fault = 25;
				}
				else	//Already in a fault - just flag us as none and go on our way
				{
					*implemented_fault = 0;
				}
			}//end has PHASE_A
			else
			{
				gl_warning("%s does not have a phase A to fault!",OBJECTHDR(this)->name);
				//Defined above
			}
		}//End A fault
		else if ((fault_type[4] == 'B') || (rand_phases == 0x02))
		{
			if (has_phase(PHASE_B))
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//make sure phase B is active (no previous fault)
				{
					//Remove phase B
					NR_branchdata[NR_branch_reference].phases &= 0xFD;

					//Flag phase being removed
					phase_remove = 0x02;

					//Flag the fault type
					*implemented_fault = 26;
				}
				else	//Already in a fault - just flag us as none and go on our way
				{
					*implemented_fault = 0;
				}
			}//end has PHASE_B
			else
			{
				gl_warning("%s does not have a phase B to fault!",OBJECTHDR(this)->name);
				//Defined above
			}
		}
		else if ((fault_type[4] == 'C') || (rand_phases == 0x01))
		{
			if (has_phase(PHASE_C))
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//make sure phase C is active (no previous fault)
				{
					//Remove phase C
					NR_branchdata[NR_branch_reference].phases &= 0xFE;

					//Flag phase being removed
					phase_remove = 0x01;

					//Flag the fault type
					*implemented_fault = 27;
				}
				else	//Already in a fault - just flag us as none and go on our way
				{
					*implemented_fault = 0;
				}
			}//end has PHASE_C
			else
			{
				gl_warning("%s does not have a phase C to fault!",OBJECTHDR(this)->name);
				//defined above
			}
		}
		else	//Something else, fail
		{
			GL_THROW("Fault type %s for link objects has an invalid phase specification");
			//Defined above
		}
	}//End single phase fuse fault
	else if ((fault_type[0] == 'F') && (fault_type[1] == 'U') && (fault_type[2] == 'S') && (fault_type[3] == '-') && (fault_type[6] == '\0'))	//Double fuse fault
	{
		//Figure out who we want to alter - assume [3] is a -, so check [4]+
		work_phases = 0x00;

		if (fault_type[4] == 'X')	//Random, flag as such
			work_phases |= 0x08;
		
		if ((fault_type[4] == 'A') || (fault_type[5] == 'A'))	//A is desired
			work_phases |= 0x04;

		if ((fault_type[4] == 'B') || (fault_type[5] == 'B'))	//B is desired
			work_phases |= 0x02;

		if ((fault_type[4] == 'C') || (fault_type[5] == 'C'))	//C is desired
			work_phases |= 0x01;

		//See how many phases we have to deal with
		numphase = 0;

		if (has_phase(PHASE_A))
		{
			numphase++;					//Increment count
		}

		if (has_phase(PHASE_B))
		{
			numphase++;					//Increment count
		}

		if (has_phase(PHASE_C))
		{
			numphase++;					//Increment count
		}

		//Pull out what phases we have to play with
		temp_phases = NR_branchdata[NR_branch_reference].phases & 0x07;

		if (numphase == 0)
		{
			GL_THROW("No phases detected for %s!",objhdr->name);
			//Defined above
		}//end 0 phase
		else if (numphase < 2)	//Single phase line (no zero phase this way)
		{
			//Refine our criteria
			if ((work_phases & 0x08) != 0x08)	//Not random case
				temp_phases &= work_phases;

			//defaulted else - if random case, only getting one phase out of this anywho (leave temp_phases as is)

			switch (temp_phases)
			{
			case 0x00:	//No phases available - something must have faulted
				*implemented_fault = 0;	//Flag as such
				break;
			case 0x01:	//Phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 27;							//Flag as a C fuse fault
				break;
			case 0x02:	//Phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 26;							//Flag as a B fuse fault
				break;
			case 0x04:	//Phase A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 25;							//Flag as a A fuse fault
				break;
			default:	//No other cases should exist
				GL_THROW("Fault type %s for link objects has an invalid phase specification");
				//Defined above
				break;
			}//end switch

			phase_remove = temp_phases;	//Flag phase removing

		}//end <2 phases (only 1 presumably)
		else if (numphase == 2)	//Only two phases present
		{
			//See how the two present coincide with the two we're asking for
			if ((work_phases & 0x08) != 0x08)	//Not random case, see what we have
				temp_phases &= work_phases;
			//Defaulted else - if random, only 1 choice exists, which is temp_phases (leave temp_phases as is)

			//Implement the appropriate fault
			switch (temp_phases)
			{
			case 0x00:	//No phases!
				*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
				break;
			case 0x01:	//Only phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 27;							//Flag as a C fuse fault
				break;
			case 0x02:	//Only phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 26;							//Flag as a B fuse fault
				break;
			case 0x03:	//B and C
				NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
				*implemented_fault = 29;							//Flag as a B & C fuse fault
				break;
			case 0x04:	//Only A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 25;							//Flag as a A fuse fault
				break;
			case 0x05:	//A and C
				NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
				*implemented_fault = 30;							//Flag as A and C fuse fault
				break;
			case 0x06:	//A and B
				NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
				*implemented_fault = 28;							//Flag as A and B fuse fault
				break;
			default:	//Not sure how we'd ever get here
				GL_THROW("Unknown phase condition on two-phase fault of %s!",objhdr->name);
				//Defined above
				break;
			}//end switch

			phase_remove = temp_phases;	//Flag phase removing

		}//end 2 phases present
		else if (numphase == 3)	//All phases present
		{
			if ((work_phases & 0x08) == 0x08)	//Random condition
			{
				//See if we have all three available to fault (if one or more is already faulted, we won't care)
				rand_phases = 0;

				//Check and populate random array as well
				if ((temp_phases & 0x01) == 0x01)	//Check for C availability
				{
					tempphase[rand_phases] = 1;
					rand_phases++;
				}

				if ((temp_phases & 0x02) == 0x02)	//Check for B availability
				{
					tempphase[rand_phases] = 2;
					rand_phases++;
				}

				if ((temp_phases & 0x04) == 0x04)	//Check for A availability
				{
					tempphase[rand_phases] = 4;
					rand_phases++;
				}

				if (rand_phases <= 2)	//Two or fewer available, just mask us out
					work_phases = 0x07;	//This will pull the appropriate phase
				else	//Must be 3, right?
				{
					//Pick a random phase - this will be the one we DON'T fault
					randval = gl_random_sampled(RNGSTATE,numphase,tempphase);

					//Set temp_phases appropriate
					if (randval == 1)	//Leave phase C
						work_phases = 0x06;	//A and B fuse fault
					else if (randval == 2)	//Leave phase B
						work_phases = 0x05;	//A and C fuse fault
					else	//Must be 4	- leave phase A
						work_phases = 0x03;	//B and C fuse fault
				}//end must be 3
			}//end random condition
			//Defaulted else - not random, so just use as normal mask

			//Determine phases to fault
			temp_phases &= work_phases;

			//Implement the appropriate fault
			switch (temp_phases)
			{
			case 0x00:	//No phases!
				*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
				break;
			case 0x01:	//Only phase C
				NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
				*implemented_fault = 27;							//Flag as a C fuse fault
				break;
			case 0x02:	//Only phase B
				NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
				*implemented_fault = 26;							//Flag as a B fuse fault
				break;
			case 0x03:	//B and C
				NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
				*implemented_fault = 29;							//Flag as a B & C fuse fault
				break;
			case 0x04:	//Only A
				NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
				*implemented_fault = 25;							//Flag as a A fuse fault
				break;
			case 0x05:	//A and C
				NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
				*implemented_fault = 30;							//Flag as A and C fuse fault
				break;
			case 0x06:	//A and B
				NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
				*implemented_fault = 28;							//Flag as A and B fuse fault
				break;
			default:	//Not sure how we'd ever get here
				GL_THROW("Unknown phase condition on two-phase fault of %s!",objhdr->name);
				//Defined above
				break;
			}//end switch

			phase_remove = temp_phases;	//Flag phase removing

		}//end all three present
		else	//Hmmm, how'd we get here?
		{
			GL_THROW("An invalid number of phases appears present for %s",objhdr->name);
			//Defined above
		}
	}//End two-phase fuse fault
	else if ((fault_type[0] == 'F') && (fault_type[1] == 'U') && (fault_type[2] == 'S') && (fault_type[3] == '-') && (fault_type[7] == '\0'))	//Three-phase fuse pop
	{
		//Let's see what phases we have to play with
		temp_phases = NR_branchdata[NR_branch_reference].phases & 0x07;

		//Case it out - if we want TLG, but don't have all three phases, just trip whatever we have
		switch (temp_phases)
		{
		case 0x00:	//No phases!
			*implemented_fault = 0;	//No fault - just get us out (something has already failed us)
			break;
		case 0x01:	//Only phase C
			NR_branchdata[NR_branch_reference].phases &= 0xFE;	//Remove C
			*implemented_fault = 27;							//Flag as a C only fuse fault
			break;
		case 0x02:	//Only phase B
			NR_branchdata[NR_branch_reference].phases &= 0xFD;	//Remove B
			*implemented_fault = 26;							//Flag as a B only fuse fault
			break;
		case 0x03:	//B and C
			NR_branchdata[NR_branch_reference].phases &= 0xFC;	//Remove B and C
			*implemented_fault = 29;							//Flag as a B & C fuse fault
			break;
		case 0x04:	//Only A
			NR_branchdata[NR_branch_reference].phases &= 0xFB;	//Remove A
			*implemented_fault = 25;							//Flag as a A only fuse fault
			break;
		case 0x05:	//A and C
			NR_branchdata[NR_branch_reference].phases &= 0xFA;	//Remove A and C
			*implemented_fault = 30;							//Flag as A and C fuse fault
			break;
		case 0x06:	//A and B
			NR_branchdata[NR_branch_reference].phases &= 0xF9;	//Remove A and B
			*implemented_fault = 28;							//Flag as A and B fuse fault
			break;
		case 0x07:	//A, B, and C
			NR_branchdata[NR_branch_reference].phases &= 0xF8;	//Remove A, B, and C
			*implemented_fault = 31;							//Flag as all three fuse fault
			break;
		default:	//Not sure how we'd ever get here
			GL_THROW("Unknown phase condition on three-phase fault of %s!",objhdr->name);
			//Defined above
			break;
		}//end phase cases

		phase_remove = temp_phases;	//Flag phase removing

	}//End three-phase fuse fault
	else	//Undetermined fault - fail us
	{
		GL_THROW("Fault type %s is not recognized for link objects!",fault_type);
		/*  TROUBLESHOOT
		The fault type specified in the eventgen object is invalid for link objects.  Please select the
		appropriate fault type and try again.  If this message has come up in error, please submit your code and a
		bug report using the trac website.
		*/

		return 0;	//Shouldn't get here, but just in case.
	}

	//Preliminary error check - see if switch faults or fuse faults are indeed being done on a fuse/switch
	if ((*implemented_fault>=18) && (*implemented_fault <= 24))	//Switch faults
	{
		if (NR_branchdata[NR_branch_reference].lnk_type != 2)	//Switch
		{
			GL_THROW("Event type %s was tried on a non-switch object!",fault_type);
			/*  TROUBLESHOOT
			A switch-related faulted was attempted on a device that is not a switch.
			Please specify a switch in the eventgen group and try again.  If the error
			persists, please submit your code and a bug report via the trac website.
			*/
		}
	}

	if ((*implemented_fault>=25) && (*implemented_fault <= 31))	//fuse faults
	{
		if (NR_branchdata[NR_branch_reference].lnk_type != 3)	//Fuse
		{
			GL_THROW("Event type %s was tried on a non-fuse object!",fault_type);
			/*  TROUBLESHOOT
			A fuse-related faulted was attempted on a device that is not a fuse.
			Please specify a fuse in the eventgen group and try again.  If the error
			persists, please submit your code and a bug report via the trac website.
			*/
		}
	}

	//See if we actually did anything - if we were already in a fault, we don't care
	if (*implemented_fault != 0)
	{
		LOCK_OBJECT(NR_swing_bus);	//Lock SWING since we'll be modifying this

		NR_admit_change = true;		//Flag an admittance update - this should trigger fault_check
		
		UNLOCK_OBJECT(NR_swing_bus);	//Release us

		//set up the remaining 4 fault specific equations in C_mat before calculating the fault current
		if(*implemented_fault == 1){ //SLG-A -> Ifb=Ifc=Vax=Vxg=0
			C_mat[3][1]=C_mat[4][2]=C_mat[5][3]=C_mat[6][6]=complex(1,0);
			fault_current_calc(C_mat, phase_remove);
		} else if(*implemented_fault == 2){ //SLG-B -> Ifa=Ifc=Vbx=Vxg=0
			C_mat[3][0]=C_mat[4][2]=C_mat[5][4]=C_mat[6][6]=complex(1,0);
			fault_current_calc(C_mat, phase_remove);
		} else if(*implemented_fault == 3){ //SLG-C -> Ifa=Ifb=Vcx=Vxg=0
			C_mat[3][0]=C_mat[4][1]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
			fault_current_calc(C_mat, phase_remove);
		} else if(*implemented_fault == 4){ //DLG-AB -> Ifc=Vax=Vbx=Vxg=0
			C_mat[3][2]=C_mat[4][3]=C_mat[5][4]=C_mat[6][6]=complex(1,0);
			fault_current_calc(C_mat, phase_remove);
		} else if(*implemented_fault == 5){ //DLG-BC -> Ifa=Vbx=Vcx=Vxg=0
			C_mat[3][0]=C_mat[4][4]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
			fault_current_calc(C_mat, phase_remove);
		} else if(*implemented_fault == 6){ //DLG-CA -> Ifb=Vax=Vcx=Vxg=0
			C_mat[3][1]=C_mat[4][3]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
			fault_current_calc(C_mat, phase_remove);
		} else if(*implemented_fault == 7){ //LL-AB -> Ifa+Ifb=Ifc=Vax=Vbx=0
			C_mat[3][0]=C_mat[3][1]=C_mat[4][2]=C_mat[5][3]=C_mat[6][4]=complex(1,0);
			fault_current_calc(C_mat, phase_remove);
		} else if(*implemented_fault == 8){ //LL-BC -> Ifb+Ifc=Ifa=Vbx=Vcx=0
			C_mat[3][1]=C_mat[3][2]=C_mat[4][0]=C_mat[5][4]=C_mat[6][5]=complex(1,0);
			fault_current_calc(C_mat, phase_remove);
		} else if(*implemented_fault == 9){ //LL-CA -> Ifa+Ifc=Ifb=Vax=Vcx=0
			C_mat[3][0]=C_mat[3][2]=C_mat[4][1]=C_mat[5][3]=C_mat[6][5]=complex(1,0);
			fault_current_calc(C_mat, phase_remove);
		} else if(*implemented_fault == 10){ //TLG-ABC -> Vax=Vbx=Vcx=Vxg=0
			C_mat[3][3]=C_mat[4][4]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
			fault_current_calc(C_mat, phase_remove);
		} else if(*implemented_fault == 32){ //TLL-ABC -> Ifa+Ifb+Ifc=Vax=Vbx=Vcx=0
			C_mat[3][0]=C_mat[3][1]=C_mat[3][2]=C_mat[4][3]=C_mat[5][4]=C_mat[6][5]=complex(1,0);
			fault_current_calc(C_mat, phase_remove);
		}
		// Calculate the fault current

		

		//Progress upward through the list until we hit a "safety" device (fuse or switch) - pop said device
		//If we somehow hit the swing bus, kill that entire phase

		//First off, see if we ARE a safety device.  That makes things easier
		//Safety devices classified as fuse or recloser - switches and sectionalizers only aid in mitigation, not actual faulting
		//Switches are included here in cases of where they are an intentionally induced action - sectionalizers have no such thing thus far
		if (NR_branchdata[NR_branch_reference].lnk_type == 3)	//Fuse
		{
			//Get the fuse
			tmpobj = gl_get_object(NR_branchdata[NR_branch_reference].name);

			if (tmpobj == NULL)
			{
				GL_THROW("An attempt to alter fuse %s failed.",NR_branchdata[NR_branch_reference].name);
				/*  TROUBLESHOOT
				While attempting to set the state of a fuse, an error occurred.  Please try again.  If the error persists,
				please submit a bug report and your code via the trac website.
				*/
			}

			funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_fuse_state"));
			
			//Make sure it was found
			if (funadd == NULL)
			{
				GL_THROW("Unable to change fuse state on %s",tmpobj->name);
				/*  TROUBLESHOOT
				While attempting to alter a fuse state, the proper fuse function was not found.
				If the problem persists, please submit a bug report and your code to the trac website.
				*/
			}

			//Update the fuse statii
			ext_result = ((int (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,phase_remove,false);

			//Make sure it worked
			if (ext_result != 1)
			{
				GL_THROW("An attempt to alter fuse %s failed.",NR_branchdata[NR_branch_reference].name);
				//defined above
			}

			//Retrieve the mean_repair_time
			temp_double_val = get_double(tmpobj,"mean_repair_time");

			//See if it worked
			if (temp_double_val == NULL)
			{
				gl_warning("Unable to map mean_repair_time from object:%s",tmpobj->name);
				/*  TROUBLESHOOT
				While attempting to access the mean_repair_time of the safety device, GridLAB-D encountered
				an error.  Ensure the object has this value.  If it does not, it will be ignored.  If it does
				exist and this warning appeared, please try again.  If the warning persists, please submit your
				code and a bug report via the trac website.
				*/
				*repair_time = 0;
			}
			else	//It did map - get the value
			{
				*repair_time = (TIMESTAMP)(*temp_double_val);
			}

			//Store the branch as an index in appropriate phases
			for (phaseidx=0; phaseidx < 3; phaseidx++)
			{
				temp_phases = 0x04 >> phaseidx;	//Figure out the phase we are on and if it is valid

				if ((phase_remove & temp_phases) == temp_phases)
				{
					protect_locations[phaseidx] = NR_branch_reference;	//Store ourselves
				}
			}

			//Update our fault phases so we aren't restored
			NR_branchdata[NR_branch_reference].faultphases |= phase_remove;

			//Remove the relevant phases 
			NR_branchdata[NR_branch_reference].phases &= ~(phase_remove);

			//Store the object handle
			*protect_obj=tmpobj;

			//Flag us as being protected
			safety_hit = true;

		}//end fuse
		else if (NR_branchdata[NR_branch_reference].lnk_type == 6)	//Recloser
		{
			//Get the recloser
			tmpobj = gl_get_object(NR_branchdata[NR_branch_reference].name);

			if (tmpobj == NULL)
			{
				GL_THROW("An attempt to alter recloser %s failed.",NR_branchdata[NR_branch_reference].name);
				/*  TROUBLESHOOT
				While attempting to set the state of a recloser, an error occurred.  Please try again.  If the error persists,
				please submit a bug report and your code via the trac website.
				*/
			}

			funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_recloser_state"));
			
			//Make sure it was found
			if (funadd == NULL)
			{
				GL_THROW("Unable to change recloser state on %s",tmpobj->name);
				/*  TROUBLESHOOT
				While attempting to alter a recloser state, the proper recloser function was not found.
				If the problem persists, please submit a bug report and your code to the trac website.
				*/
			}

			//Update the recloser statii - return is the number of attempts (max attempts in this case)
			ext_result_dbl = ((double (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,phase_remove,false);

			//Make sure it worked
			if (ext_result_dbl == 0)
			{
				GL_THROW("An attempt to alter recloser %s failed.",NR_branchdata[NR_branch_reference].name);
				//defined above
			}

			//Retrieve the mean_repair_time
			temp_double_val = get_double(tmpobj,"mean_repair_time");

			//See if it worked
			if (temp_double_val == NULL)
			{
				gl_warning("Unable to map mean_repair_time from object:%s",tmpobj->name);
				//Defined above
				*repair_time = 0;
			}
			else	//It did map - get the value
			{
				*repair_time = (TIMESTAMP)(*temp_double_val);
			}

			//Store the number of recloser actions
			*Recloser_Counts = ext_result_dbl;

			//Store the branch as an index in appropriate phases
			for (phaseidx=0; phaseidx < 3; phaseidx++)
			{
				temp_phases = 0x04 >> phaseidx;	//Figure out the phase we are on and if it is valid

				if ((phase_remove & temp_phases) == temp_phases)
				{
					protect_locations[phaseidx] = NR_branch_reference;	//Store ourselves
				}
			}

			//Update our fault phases so we aren't restored
			NR_branchdata[NR_branch_reference].faultphases |= phase_remove;

			//Remove the relevant phases 
			NR_branchdata[NR_branch_reference].phases &= ~(phase_remove);

			//Store the object handle
			*protect_obj=tmpobj;

			//Flag us as being protected
			safety_hit = true;

		}//End recloser
		else if ((NR_branchdata[NR_branch_reference].lnk_type == 2) && (switch_val == true))	//Switch induced fault - handle it
		{
			//Follows convention of safety devices above
			//Extra coding - basically what would have happened below when it was classified as a safety device
			//Get the switch
			tmpobj = gl_get_object(NR_branchdata[NR_branch_reference].name);

			if (tmpobj == NULL)
			{
				GL_THROW("An attempt to alter switch %s failed.",NR_branchdata[NR_branch_reference].name);
				/*  TROUBLESHOOT
				While attempting to set the state of a switch, an error occurred.  Please try again.  If the error persists,
				please submit a bug report and your code via the trac website.
				*/
			}

			funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_switch_state"));
			
			//Make sure it was found
			if (funadd == NULL)
			{
				GL_THROW("Unable to change switch state on %s",tmpobj->name);
				/*  TROUBLESHOOT
				While attempting to alter a switch state, the proper switch function was not found.
				If the problem persists, please submit a bug report and your code to the trac website.
				*/
			}

			//Update the switch statii
			ext_result = ((int (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,phase_remove,false);

			//Make sure it worked
			if (ext_result != 1)
			{
				GL_THROW("An attempt to alter switch %s failed.",NR_branchdata[NR_branch_reference].name);
				//defined above
			}

			//Retrieve the mean_repair_time
			temp_double_val = get_double(tmpobj,"mean_repair_time");

			//See if it worked
			if (temp_double_val == NULL)
			{
				gl_warning("Unable to map mean_repair_time from object:%s",tmpobj->name);
				//Defined above
				*repair_time = 0;
			}
			else	//It did map - get the value
			{
				*repair_time = (TIMESTAMP)(*temp_double_val);
			}

			//Store ourselves as our protective device
			for (phaseidx=0; phaseidx < 3; phaseidx++)
			{
				temp_phases = 0x04 >> phaseidx;	//Figure out the phase we are on and if it is valid

				if ((phase_remove & temp_phases) == temp_phases)
				{
					protect_locations[phaseidx] = NR_branch_reference;	//Store ourselves
				}
			}

			//Flag our fault phases
			NR_branchdata[NR_branch_reference].faultphases |= phase_remove;

			//Remove the relevant phases 
			NR_branchdata[NR_branch_reference].phases &= ~(phase_remove);

			//Store the object handle
			*protect_obj=tmpobj;

			safety_hit = true;	//We hit a protective device
		}
		else
		{
			safety_hit = false;	//Nope, let's find one
		}

		//Start up the loop - populate initial variable
		temp_branch = NR_branch_reference;

		while (safety_hit != true)
		{
			//Pull from bus of current link
			temp_node = NR_branchdata[temp_branch].from;

			//See if temp_node is a swing.  If so, remove the relevant phase and we're done
			if (NR_busdata[temp_node].type == 2)
			{
				//Store the swing as an index in appropriate phases
				for (phaseidx=0; phaseidx < 3; phaseidx++)
				{
					temp_phases = 0x04 >> phaseidx;	//Figure out the phase we are on and if it is valid

					if ((phase_remove & temp_phases) == temp_phases)
					{
						protect_locations[phaseidx] = -99;	//Flag for swing
					}
				}

				//Get the swing bus value
				tmpobj = gl_get_object(NR_busdata[temp_node].name);

				if (tmpobj == NULL)
				{
					GL_THROW("An attempt to find the swing node %s failed.",NR_busdata[temp_node].name);
					/*  TROUBLESHOOT
					While attempting to get the mean repair time for the swing bus, an error occurred.  Please try again.  If the error persists,
					please submit a bug report and your code via the trac website.
					*/
				}

				//Retrieve the mean_repair_time
				temp_double_val = get_double(tmpobj,"mean_repair_time");

				//See if it worked
				if (temp_double_val == NULL)
				{
					gl_warning("Unable to map mean_repair_time from object:%s",tmpobj->name);
					//Defined above
					*repair_time = 0;
				}
				else	//It did map - get the value
				{
					*repair_time = (TIMESTAMP)(*temp_double_val);
				}

				//Remove the required phases
				NR_busdata[temp_node].phases &= (~(phase_remove));

				//Update our fault phases so we aren't restored
				NR_branchdata[NR_branch_reference].faultphases |= phase_remove;

				//Store the object handle
				*protect_obj=tmpobj;

				safety_hit = true;	//Flag as a safety hit (assumes SWING bus is somehow protected on transmission side)

				break;	//Get us out of this loop
			}
			else	//Not a swing, find the first branch this node is a "to" value for
			{
				for (temp_table_loc=0; temp_table_loc<NR_busdata[temp_node].Link_Table_Size; temp_table_loc++)
				{
					//See if node is a to end - assumes radial phase progressions (i.e., no phase AB and phase C running into a node to form node ABC)
					if (NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].to == temp_node)	//This node is a to end
					{
						//See if we are of a "protective" device implementation
						if (NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].lnk_type == 6)	//Recloser
						{
							//Get the recloser
							tmpobj = gl_get_object(NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].name);

							if (tmpobj == NULL)
							{
								GL_THROW("An attempt to alter recloser %s failed.",NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].name);
								/*  TROUBLESHOOT
								While attempting to set the state of a recloser, an error occurred.  Please try again.  If the error persists,
								please submit a bug report and your code via the trac website.
								*/
							}

							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_recloser_state"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change recloser state on %s",tmpobj->name);
								/*  TROUBLESHOOT
								While attempting to alter a recloser state, the proper recloser function was not found.
								If the problem persists, please submit a bug report and your code to the trac website.
								*/
							}

							//Update the recloser statii - return is the number of attempts (max attempts in this case)
							ext_result_dbl = ((double (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,phase_remove,false);

							//Make sure it worked
							if (ext_result_dbl == 0)
							{
								GL_THROW("An attempt to alter recloser %s failed.",NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].name);
								//defined above
							}

							//Retrieve the mean_repair_time
							temp_double_val = get_double(tmpobj,"mean_repair_time");

							//See if it worked
							if (temp_double_val == NULL)
							{
								gl_warning("Unable to map mean_repair_time from object:%s",tmpobj->name);
								//Defined above
								*repair_time = 0;
							}
							else	//It did map - get the value
							{
								*repair_time = (TIMESTAMP)(*temp_double_val);
							}

							//Store the number of recloser actions
							*Recloser_Counts = ext_result_dbl;

							//Store the branch as an index in appropriate phases
							for (phaseidx=0; phaseidx < 3; phaseidx++)
							{
								temp_phases = 0x04 >> phaseidx;	//Figure out the phase we are on and if it is valid

								if ((phase_remove & temp_phases) == temp_phases)
								{
									protect_locations[phaseidx] = NR_busdata[temp_node].Link_Table[temp_table_loc];	//Store our location
								}
							}

							//Flag the remote object's appropriate phases
							NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].faultphases |= phase_remove;

							//Update our fault phases so we aren't restored
							NR_branchdata[NR_branch_reference].faultphases |= phase_remove;

							//Store the object handle
							*protect_obj=tmpobj;

							//Flag us as being protected
							safety_hit = true;

							//Break out of this pesky loop
							break;
						}//end recloser
						else if (NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].lnk_type == 5)	//Sectionalizer
						{
							//Get the sectionalizer
							tmpobj = gl_get_object(NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].name);

							if (tmpobj == NULL)
							{
								GL_THROW("An attempt to alter sectionalizer %s failed.",NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].name);
								/*  TROUBLESHOOT
								While attempting to set the state of a sectionalizer, an error occurred.  Please try again.  If the error persists,
								please submit a bug report and your code via the trac website.
								*/
							}

							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_sectionalizer_state"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change sectionalizer state on %s",tmpobj->name);
								/*  TROUBLESHOOT
								While attempting to alter a sectionalizer state, the proper sectionalizer function was not found.
								If the problem persists, please submit a bug report and your code to the trac website.
								*/
							}

							//Update the sectionalizer statii
							ext_result_dbl = ((double (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,phase_remove,false);

							//Make sure it worked
							if (ext_result_dbl == 0)
							{
								GL_THROW("An attempt to alter sectionalizer %s failed.",NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].name);
								//defined above
							}
							else if (ext_result_dbl < 0)	//Negative number means no upstream recloser was found
							{
								//Treat us as nothing special - just proceed up a branch - if no recloser, that means we need to hit a fuse...or the swing bus
								//Go up to the next level
								temp_branch = NR_busdata[temp_node].Link_Table[temp_table_loc];
								break;	//Out of this for we go!
							}
							else	//Positive number - recloser operated
							{
								//Retrieve the mean_repair_time
								temp_double_val = get_double(tmpobj,"mean_repair_time");

								//See if it worked
								if (temp_double_val == NULL)
								{
									gl_warning("Unable to map mean_repair_time from object:%s",tmpobj->name);
									//Defined above
									*repair_time = 0;
								}
								else	//It did map - get the value
								{
									*repair_time = (TIMESTAMP)(*temp_double_val);
								}

								//Store the number of recloser actions
								*Recloser_Counts = ext_result_dbl;

								//Store the branch as an index in appropriate phases
								for (phaseidx=0; phaseidx < 3; phaseidx++)
								{
									temp_phases = 0x04 >> phaseidx;	//Figure out the phase we are on and if it is valid

									if ((phase_remove & temp_phases) == temp_phases)
									{
										protect_locations[phaseidx] = NR_busdata[temp_node].Link_Table[temp_table_loc];	//Store our location
									}
								}

								//Flag the remote object's appropriate phases
								NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].faultphases |= phase_remove;

								//Update our fault phases so we aren't restored
								NR_branchdata[NR_branch_reference].faultphases |= phase_remove;

								//Store the object handle
								*protect_obj=tmpobj;

								//Flag us as being protected
								safety_hit = true;

								//Break out of this pesky loop
								break;
							}//End sectionalizer logic
						}//end sectionalizer
						else if (NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].lnk_type == 3)	//Fuse
						{
							//Get the fuse
							tmpobj = gl_get_object(NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].name);

							if (tmpobj == NULL)
							{
								GL_THROW("An attempt to alter fuse %s failed.",NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].name);
								/*  TROUBLESHOOT
								While attempting to set the state of a fuse, an error occurred.  Please try again.  If the error persists,
								please submit a bug report and your code via the trac website.
								*/
							}

							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_fuse_state"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change fuse state on %s",tmpobj->name);
								/*  TROUBLESHOOT
								While attempting to alter a fuse state, the proper fuse function was not found.
								If the problem persists, please submit a bug report and your code to the trac website.
								*/
							}

							//Update the fuse statii
							ext_result = ((int (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,phase_remove,false);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter fuse %s failed.",NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].name);
								//defined above
							}

							//Retrieve the mean_repair_time
							temp_double_val = get_double(tmpobj,"mean_repair_time");

							//See if it worked
							if (temp_double_val == NULL)
							{
								gl_warning("Unable to map mean_repair_time from object:%s",tmpobj->name);
								//Defined above
								*repair_time = 0;
							}
							else	//It did map - get the value
							{
								*repair_time = (TIMESTAMP)(*temp_double_val);
							}

							//Store the branch as an index in appropriate phases
							for (phaseidx=0; phaseidx < 3; phaseidx++)
							{
								temp_phases = 0x04 >> phaseidx;	//Figure out the phase we are on and if it is valid

								if ((phase_remove & temp_phases) == temp_phases)
								{
									protect_locations[phaseidx] = NR_busdata[temp_node].Link_Table[temp_table_loc];	//Store our location
								}
							}

							//Flag the remote objects removed phases
							NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].faultphases |= phase_remove;

							//Update our fault phases so we aren't restored
							NR_branchdata[NR_branch_reference].faultphases |= phase_remove;

							//Store the object handle
							*protect_obj=tmpobj;

							//Flag us as being protected
							safety_hit = true;

							//Break out of this pesky loop
							break;
						}//end fuse
						else if (NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].lnk_type == 4)	//Transformer - not really "protective" - we assume it explodes
						{
							//Get the transformer
							tmpobj = gl_get_object(NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].name);

							if (tmpobj == NULL)
							{
								GL_THROW("An attempt to alter transformer %s failed.",NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].name);
								/*  TROUBLESHOOT
								While attempting to set the state of a transformer, an error occurred.  Please try again.  If the error persists,
								please submit a bug report and your code via the trac website.
								*/
							}

							//Transformers are magical "all phases removed" devices - basically we're assuming catastrophic failure
							NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].phases &= 0xF0;

							//Store the branch as an index in appropriate phases - transformer becomes all, not matter what
							for (phaseidx=0; phaseidx < 3; phaseidx++)
							{
								protect_locations[phaseidx] = NR_busdata[temp_node].Link_Table[temp_table_loc];	//Store our location
							}

							//Retrieve the mean_repair_time
							temp_double_val = get_double(tmpobj,"mean_repair_time");

							//See if it worked
							if (temp_double_val == NULL)
							{
								gl_warning("Unable to map mean_repair_time from object:%s",tmpobj->name);
								//Defined above
								*repair_time = 0;
							}
							else	//It did map - get the value
							{
								*repair_time = (TIMESTAMP)(*temp_double_val);
							}

							//Flag remote transformer phases - all of them by default
							NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].faultphases = 0x07;

							//Update our fault phases so we aren't restored
							NR_branchdata[NR_branch_reference].faultphases |= phase_remove;

							//Store the object handle
							*protect_obj=tmpobj;

							//Protective device found!
							safety_hit = true;

							//Get out of this for loop
							break;
						}//end transformer
						else	//Normal or triplex line - proceed
						{
							//Go up to the next level
							temp_branch = NR_busdata[temp_node].Link_Table[temp_table_loc];
							break;	//Out of this for we go!
						}//end normal
					}//End it is a to-end
					//Default else - it must be a from, just proceed
				}//End link table for traversion

				//Make sure we didn't somehow reach the end
				if (temp_table_loc == NR_busdata[temp_node].Link_Table_Size)
				{
					GL_THROW("Error finding proper to reference for node %s",NR_busdata[temp_node].name);
					/*  TROUBLESHOOT
					While attempting to induce a safety reaction to a fault, a progression through the
					links of the system failed.  Please try again.  If the bug persists, please submit your
					code and a bug report using the trac website.
					*/
				}
			}//end not a swing bus
		}//end safety not hit while

		//Safety device enacted - now call fault_check function and let it remove all invalid objects
		//Map the function
		funadd = (FUNCTIONADDR)(gl_get_function(fault_check_object,"reliability_alterations"));
		
		//Make sure it was found
		if (funadd == NULL)
		{
			GL_THROW("Unable to update objects for reliability effects");
			/*  TROUBLESHOOT
			While attempting to update the powerflow to properly represent the new post-fault state, an error
			occurred.  If the problem persists, please submit a bug report and your code to the trac website.
			*/
		}

		//Update the device - find a valid phase
		for (phaseidx=0; phaseidx < 3; phaseidx++)
		{
			temp_phases = 0x04 >> phaseidx;	//Figure out the phase we are on and if it is valid

			if ((phase_remove & temp_phases) == temp_phases)
			{
				temp_branch = protect_locations[phaseidx];	//Pull protective device number
				break;		//Only need one to know where to start
			}
		}

		//Update powerflow - removal mode
		ext_result = ((int (*)(OBJECT *, int, bool))(*funadd))(fault_check_object,temp_branch,false);

		//Make sure it worked
		if (ext_result != 1)
		{
			GL_THROW("Unable to update objects for reliability effects");
			//defined above
		}

		if (temp_branch == -99)
			gl_verbose("Event %d induced on %s by using %s",*implemented_fault,objhdr->name,NR_busdata[0].name);
		else
			gl_verbose("Event %d induced on %s by using %s",*implemented_fault,objhdr->name,NR_branchdata[temp_branch].name);
	}//End a change has been flagged

	return 1;	//Successful
}

//Function to remove enacted fault on link - use same list as above (link_fault_on)
int link::link_fault_off(int *implemented_fault, char *imp_fault_name, void *Extra_Data)
{
	unsigned char phase_restore = 0x00;	//Default is no phases restored
	unsigned char temp_phases, temp_phases_B, work_phases;			//Working variable
	char phaseidx, indexval;
	int temp_node, ext_result;
	double ext_result_dbl;
	OBJECT *objhdr = OBJECTHDR(this);
	OBJECT *tmpobj;
	FUNCTIONADDR funadd = NULL;
	double *Recloser_Counts;
	bool switch_val;

	//Set up default switch variable - used to indicate special cases
	switch_val = false;

	//Link up recloser counts for manipulation
	Recloser_Counts = (double *)Extra_Data;

	//Less logic here - just undo what we did before - find the fault type and clear it out
	switch (*implemented_fault)
	{
		case 0:	//No fault - do nothing
			imp_fault_name[0] = 'N';
			imp_fault_name[1] = 'o';
			imp_fault_name[2] = 'n';
			imp_fault_name[3] = 'e';
			imp_fault_name[4] = '\0';
			break;
		case 1:	//SLG-A
			imp_fault_name[0] = 'S';
			imp_fault_name[1] = 'L';
			imp_fault_name[2] = 'G';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'A';
			imp_fault_name[5] = '\0';
			phase_restore = 0x04;	//Put A back in service
			break;
		case 2:	//SLG-B
			imp_fault_name[0] = 'S';
			imp_fault_name[1] = 'L';
			imp_fault_name[2] = 'G';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'B';
			imp_fault_name[5] = '\0';
			phase_restore = 0x02;	//Put B back in service
			break;
		case 3:	//SLG-C
			imp_fault_name[0] = 'S';
			imp_fault_name[1] = 'L';
			imp_fault_name[2] = 'G';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'C';
			imp_fault_name[5] = '\0';
			phase_restore = 0x01;	//Put C back in service
			break;
		case 4:	//DLG-AB
			imp_fault_name[0] = 'D';
			imp_fault_name[1] = 'L';
			imp_fault_name[2] = 'G';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'A';
			imp_fault_name[5] = 'B';
			imp_fault_name[6] = '\0';
			phase_restore = 0x06;	//Put A and B back in service
			break;
		case 5:	//DLG-BC
			imp_fault_name[0] = 'D';
			imp_fault_name[1] = 'L';
			imp_fault_name[2] = 'G';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'B';
			imp_fault_name[5] = 'C';
			imp_fault_name[6] = '\0';
			phase_restore = 0x03;	//Put B and C back in service
			break;
		case 6:	//DLG-CA
			imp_fault_name[0] = 'D';
			imp_fault_name[1] = 'L';
			imp_fault_name[2] = 'G';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'C';
			imp_fault_name[5] = 'A';
			imp_fault_name[6] = '\0';
			phase_restore = 0x05;	//Put C and A back in service
			break;
		case 7:	//LL-AB
			imp_fault_name[0] = 'L';
			imp_fault_name[1] = 'L';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'A';
			imp_fault_name[4] = 'B';
			imp_fault_name[5] = '\0';
			phase_restore = 0x06;	//Put A and B back in service
			break;
		case 8:	//LL-BC
			imp_fault_name[0] = 'L';
			imp_fault_name[1] = 'L';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'B';
			imp_fault_name[4] = 'C';
			imp_fault_name[5] = '\0';
			phase_restore = 0x03;	//Put B and C back in service
			break;
		case 9:	//LL-CA
			imp_fault_name[0] = 'L';
			imp_fault_name[1] = 'L';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'C';
			imp_fault_name[4] = 'A';
			imp_fault_name[5] = '\0';
			phase_restore = 0x05;	//Put C and A back in service
			break;
		case 10:	//TLG
			imp_fault_name[0] = 'T';
			imp_fault_name[1] = 'L';
			imp_fault_name[2] = 'G';
			imp_fault_name[3] = '\0';
			phase_restore = 0x07;	//Put A, B, and C back in service
			break;
		case 11:	//OC-A
			imp_fault_name[0] = 'O';
			imp_fault_name[1] = 'C';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'A';
			imp_fault_name[4] = '\0';
			phase_restore = 0x04;	//Put A back in service
			break;
		case 12:	//OC-B
			imp_fault_name[0] = 'O';
			imp_fault_name[1] = 'C';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'B';
			imp_fault_name[4] = '\0';
			phase_restore = 0x02;	//Put B back in service
			break;
		case 13:	//OC-C
			imp_fault_name[0] = 'O';
			imp_fault_name[1] = 'C';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'C';
			imp_fault_name[4] = '\0';
			phase_restore = 0x01;	//Put C back in service
			break;
		case 14:	//OC2-AB
			imp_fault_name[0] = 'O';
			imp_fault_name[1] = 'C';
			imp_fault_name[2] = '2';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'A';
			imp_fault_name[5] = 'B';
			imp_fault_name[6] = '\0';
			phase_restore = 0x06;	//Put A and B back in service
			break;
		case 15:	//OC2-BC
			imp_fault_name[0] = 'O';
			imp_fault_name[1] = 'C';
			imp_fault_name[2] = '2';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'B';
			imp_fault_name[5] = 'C';
			imp_fault_name[6] = '\0';
			phase_restore = 0x03;	//Put B and C back in service
			break;
		case 16:	//OC2-CA
			imp_fault_name[0] = 'O';
			imp_fault_name[1] = 'C';
			imp_fault_name[2] = '2';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'C';
			imp_fault_name[5] = 'A';
			imp_fault_name[6] = '\0';
			phase_restore = 0x05;	//Put C and A back in service
			break;
		case 17:	//OC3
			imp_fault_name[0] = 'O';
			imp_fault_name[1] = 'C';
			imp_fault_name[2] = '3';
			imp_fault_name[3] = '\0';
			phase_restore = 0x07;	//Put A, B, and C back in service
			break;
		case 18:	//SW-A
			imp_fault_name[0] = 'S';
			imp_fault_name[1] = 'W';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'A';
			imp_fault_name[4] = '\0';
			phase_restore = 0x04;	//Put A back in service
			switch_val = true;		//Flag as a switch action
			break;
		case 19:	//SW-B
			imp_fault_name[0] = 'S';
			imp_fault_name[1] = 'W';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'B';
			imp_fault_name[4] = '\0';
			phase_restore = 0x02;	//Put B back in service
			switch_val = true;		//Flag as a switch action
			break;
		case 20:	//SW-C
			imp_fault_name[0] = 'S';
			imp_fault_name[1] = 'W';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'C';
			imp_fault_name[4] = '\0';
			phase_restore = 0x01;	//Put C back in service
			switch_val = true;		//Flag as a switch action
			break;
		case 21:	//SW-AB
			imp_fault_name[0] = 'S';
			imp_fault_name[1] = 'W';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'A';
			imp_fault_name[4] = 'B';
			imp_fault_name[5] = '\0';
			phase_restore = 0x06;	//Put A and B back in service
			switch_val = true;		//Flag as a switch action
			break;
		case 22:	//SW-BC
			imp_fault_name[0] = 'S';
			imp_fault_name[1] = 'W';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'B';
			imp_fault_name[4] = 'C';
			imp_fault_name[5] = '\0';
			phase_restore = 0x03;	//Put B and C back in service
			switch_val = true;		//Flag as a switch action
			break;
		case 23:	//SW-CA
			imp_fault_name[0] = 'S';
			imp_fault_name[1] = 'W';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'C';
			imp_fault_name[4] = 'A';
			imp_fault_name[5] = '\0';
			phase_restore = 0x05;	//Put C and A back in service
			switch_val = true;		//Flag as a switch action
			break;
		case 24:	//SW-ABC
			imp_fault_name[0] = 'S';
			imp_fault_name[1] = 'W';
			imp_fault_name[2] = '-';
			imp_fault_name[3] = 'A';
			imp_fault_name[4] = 'B';
			imp_fault_name[5] = 'C';
			imp_fault_name[6] = '\0';
			phase_restore = 0x07;	//Put A, B, and C back in service
			switch_val = true;		//Flag as a switch action
			break;
		case 25:	//FUS-A
			imp_fault_name[0] = 'F';
			imp_fault_name[1] = 'U';
			imp_fault_name[2] = 'S';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'A';
			imp_fault_name[5] = '\0';
			phase_restore = 0x04;	//Put A back in service
			break;
		case 26:	//FUS-B
			imp_fault_name[0] = 'F';
			imp_fault_name[1] = 'U';
			imp_fault_name[2] = 'S';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'B';
			imp_fault_name[5] = '\0';
			phase_restore = 0x02;	//Put B back in service
			break;
		case 27:	//FUS-C
			imp_fault_name[0] = 'F';
			imp_fault_name[1] = 'U';
			imp_fault_name[2] = 'S';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'C';
			imp_fault_name[5] = '\0';
			phase_restore = 0x01;	//Put C back in service
			break;
		case 28:	//FUS-AB
			imp_fault_name[0] = 'F';
			imp_fault_name[1] = 'U';
			imp_fault_name[2] = 'S';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'A';
			imp_fault_name[5] = 'B';
			imp_fault_name[6] = '\0';
			phase_restore = 0x06;	//Put A and B back in service
			break;
		case 29:	//FUS-BC
			imp_fault_name[0] = 'F';
			imp_fault_name[1] = 'U';
			imp_fault_name[2] = 'S';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'B';
			imp_fault_name[5] = 'C';
			imp_fault_name[6] = '\0';
			phase_restore = 0x03;	//Put B and C back in service
			break;
		case 30:	//FUS-CA
			imp_fault_name[0] = 'F';
			imp_fault_name[1] = 'U';
			imp_fault_name[2] = 'S';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'C';
			imp_fault_name[5] = 'A';
			imp_fault_name[6] = '\0';
			phase_restore = 0x05;	//Put C and A back in service
			break;
		case 31:	//FUS-ABC
			imp_fault_name[0] = 'F';
			imp_fault_name[1] = 'U';
			imp_fault_name[2] = 'S';
			imp_fault_name[3] = '-';
			imp_fault_name[4] = 'A';
			imp_fault_name[5] = 'B';
			imp_fault_name[6] = 'C';
			imp_fault_name[7] = '\0';
			phase_restore = 0x07;	//Put A, B, and C back in service
			break;
		case 32:	//TLL
			imp_fault_name[0] = 'T';
			imp_fault_name[1] = 'L';
			imp_fault_name[2] = 'L';
			imp_fault_name[3] = '\0';
			phase_restore = 0x07;	//Put A, B, and C back in service
			break;
		default:	//Should never get here
			GL_THROW("%s - attempted to recover from unsupported fault!",objhdr->name);
			/*  TROUBLESHOOT
			The link object attempted to recover from an unknown fault type.  Please try again.  If the
			error persists, please submit your code and a bug report to the trac website.
			*/
			break;
	}//end switch

	//Flag an update if something changed (not a 0 state)
	if (*implemented_fault != 0)
	{
		temp_node = -1;	//Default value

		//Remove our restrictions for these phases
		for (phaseidx=0; phaseidx<3; phaseidx++)
		{
			work_phases = 0x04 >> phaseidx;	//Get check

			if ((phase_restore & work_phases) == work_phases)	//Valid phase to restore
			{
				//Store the value of protect_locations for later
				temp_node = protect_locations[phaseidx];

				//See if our "protective device" was the swing bus
				if (protect_locations[phaseidx] == -99)
				{
					//Theoretically, this phase should exist on the SWING.  Let's check to make sure
					if ((NR_busdata[0].origphases & work_phases) == work_phases)	//Valid phase
					{
						NR_busdata[0].phases |= work_phases;	//Just turn it back on - it's the SWING, it has to work
					}
					else
					{
						GL_THROW("A fault was induced on the SWING bus for an unsupported phase!");
						/*  TROUBLESHOOT
						Somehow, a fault was induced on a phase that should not exist on the system.  While
						attemtping to restore this fault, the SWING bus did not have the proper original phases,
						so this could not be restored.  Please submit your code and a bug report using the trac website.
						*/
					}
				}//End SWING
				else	//Not the SWING Bus
				{
					//See if we are of a "protective" device implementation
					if ((NR_branchdata[protect_locations[phaseidx]].lnk_type == 2) && (switch_val == true))	//Switch
					{
						//Get the switch
						tmpobj = gl_get_object(NR_branchdata[protect_locations[phaseidx]].name);

						if (tmpobj == NULL)
						{
							GL_THROW("An attempt to alter switch %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
							//Defined above
						}

						//Check and see what the supporting nodes are doing - if a switch is already in a fault, this causes issues
						temp_phases = ((NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases | NR_busdata[NR_branchdata[protect_locations[phaseidx]].to].phases) & 0x07);

						if ((phase_restore & temp_phases) == phase_restore)	//Support is available, one way or another
						{
							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_switch_state"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change switch state on %s",tmpobj->name);
								//Defined above
							}

							//Update the fuse statii
							ext_result = ((int (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,phase_restore,true);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter switch %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}
						}
						else if ((phase_restore & temp_phases) != 0x00)	//Some support is available
						{
							//Update fault phases
							temp_phases_B = (phase_restore & temp_phases);		//these are the phases to restore
							temp_phases = (phase_restore & (~temp_phases_B));	//These are the phases just to update

							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_switch_faults"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change switch state on %s",tmpobj->name);
								//Defined above - despite being slightly different
							}

							//Update the switch statii
							ext_result = ((int (*)(OBJECT *, unsigned char))(*funadd))(tmpobj,temp_phases);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter switch %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}

							//Restore other phase(s)
							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_switch_state"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change switch state on %s",tmpobj->name);
								//Defined above
							}

							//Update the fuse statii
							ext_result = ((int (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,temp_phases_B,true);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter switch %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}
						}
						else	//No support available
						{
							//Just update fault phases
							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_switch_faults"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change switch state on %s",tmpobj->name);
								//Defined above - despite being slightly different
							}

							//Update the switch statii
							ext_result = ((int (*)(OBJECT *, unsigned char))(*funadd))(tmpobj,phase_restore);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter switch %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}
						}

						//Modify our phases as appropriate
						NR_branchdata[protect_locations[phaseidx]].phases &= (NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases & 0x07);

						//Flag the remote object's appropriate phases
						NR_branchdata[protect_locations[phaseidx]].faultphases &= ~(phase_restore);

						//Remove the branch as an index in appropriate phases
						for (phaseidx=0; phaseidx < 3; phaseidx++)
						{
							temp_phases = 0x04 >> phaseidx;	//Figure out the phase we are on and if it is valid

							if ((phase_restore & temp_phases) == temp_phases)
							{
								protect_locations[phaseidx] = -1;	//Clear our location
							}
						}

						//Update our fault phases
						NR_branchdata[NR_branch_reference].faultphases &= ~(phase_restore);

						//Break out of this pesky loop
						break;
					}//end switch closure
					else if (NR_branchdata[protect_locations[phaseidx]].lnk_type == 6)	//recloser
					{
						//Get the recloser
						tmpobj = gl_get_object(NR_branchdata[protect_locations[phaseidx]].name);

						if (tmpobj == NULL)
						{
							GL_THROW("An attempt to alter recloser %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
							//Defined above
						}

						//Check and see what the supporting nodes are doing - if a recloser is already in a fault, this causes issues
						temp_phases = ((NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases | NR_busdata[NR_branchdata[protect_locations[phaseidx]].to].phases) & 0x07);

						if ((phase_restore & temp_phases) == phase_restore)	//Support is available, one way or another
						{
							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_recloser_state"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change recloser state on %s",tmpobj->name);
								//Defined above
							}

							//Update the recloser statii
							ext_result_dbl = ((double (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,phase_restore,true);

							//Make sure it worked
							if (ext_result_dbl != 1.0)
							{
								GL_THROW("An attempt to alter recloser %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}
						}
						else if ((phase_restore & temp_phases) != 0x00)	//Some support is available
						{
							//Update fault phases
							temp_phases_B = (phase_restore & temp_phases);		//these are the phases to restore
							temp_phases = (phase_restore & (~temp_phases_B));	//These are the phases just to update

							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_recloser_faults"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change recloser state on %s",tmpobj->name);
								//Defined above - despite being slightly different
							}

							//Update the sectionalizer statii
							ext_result = ((int (*)(OBJECT *, unsigned char))(*funadd))(tmpobj,temp_phases);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter recloser %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}

							//Restore other phase(s)
							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_recloser_state"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change recloser state on %s",tmpobj->name);
								//Defined above
							}

							//Update the recloser statii
							ext_result_dbl = ((double (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,phase_restore,true);

							//Make sure it worked
							if (ext_result_dbl != 1.0)
							{
								GL_THROW("An attempt to alter recloser %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}
						}
						else	//No support available
						{
							//Just update fault phases
							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_recloser_faults"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change sectionalizer recloser on %s",tmpobj->name);
								//Defined above - despite being slightly different
							}

							//Update the sectionalizer statii
							ext_result = ((int (*)(OBJECT *, unsigned char))(*funadd))(tmpobj,phase_restore);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter recloser %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}
						}

						//Modify our phases as appropriate
						NR_branchdata[protect_locations[phaseidx]].phases &= (NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases & 0x07);

						//Flag the remote object's appropriate phases
						NR_branchdata[protect_locations[phaseidx]].faultphases &= ~(phase_restore);

						//Remove the branch as an index in appropriate phases
						for (phaseidx=0; phaseidx < 3; phaseidx++)
						{
							temp_phases = 0x04 >> phaseidx;	//Figure out the phase we are on and if it is valid

							if ((phase_restore & temp_phases) == temp_phases)
							{
								protect_locations[phaseidx] = -1;	//Clear our location
							}
						}

						//Update our fault phases
						NR_branchdata[NR_branch_reference].faultphases &= ~(phase_restore);

						//Break out of this pesky loop
						break;
					}//end recloser closure
					else if (NR_branchdata[protect_locations[phaseidx]].lnk_type == 5)	//Sectionalizer
					{
						//Get the sectionalizer
						tmpobj = gl_get_object(NR_branchdata[protect_locations[phaseidx]].name);

						if (tmpobj == NULL)
						{
							GL_THROW("An attempt to alter sectionalizer %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
							//Defined above
						}

						//Check and see what the supporting nodes are doing - if a sectionalizer is already in a fault, this causes issues
						temp_phases = ((NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases | NR_busdata[NR_branchdata[protect_locations[phaseidx]].to].phases) & 0x07);

						if ((phase_restore & temp_phases) == phase_restore)	//Support is available, one way or another
						{
							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_sectionalizer_state"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change sectionalizer state on %s",tmpobj->name);
								//Defined above
							}

							//Update the sectionalizer statii
							ext_result_dbl = ((double (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,phase_restore,true);

							//Make sure it worked
							if (ext_result_dbl != 1.0)
							{
								GL_THROW("An attempt to alter sectionalizer %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}
						}
						else if ((phase_restore & temp_phases) != 0x00)	//Some support is available
						{
							//Update fault phases
							temp_phases_B = (phase_restore & temp_phases);		//these are the phases to restore
							temp_phases = (phase_restore & (~temp_phases_B));	//These are the phases just to update

							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_sectionalizer_faults"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change sectionalizer state on %s",tmpobj->name);
								//Defined above - despite being slightly different
							}

							//Update the sectionalizer statii
							ext_result = ((int (*)(OBJECT *, unsigned char))(*funadd))(tmpobj,temp_phases);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter sectionalizer %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}

							//Restore other phase(s)
							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_sectionalizer_state"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change sectionalizer state on %s",tmpobj->name);
								//Defined above
							}

							//Update the sectionalizer statii
							ext_result_dbl = ((double (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,phase_restore,true);

							//Make sure it worked
							if (ext_result_dbl != 1.0)
							{
								GL_THROW("An attempt to alter sectionalizer %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}
						}
						else	//No support available
						{
							//Just update fault phases
							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_sectionalizer_faults"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change sectionalizer state on %s",tmpobj->name);
								//Defined above - despite being slightly different
							}

							//Update the sectionalizer statii
							ext_result = ((int (*)(OBJECT *, unsigned char))(*funadd))(tmpobj,phase_restore);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter sectionalizer %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}
						}
						
						//Modify our phases as appropriate
						NR_branchdata[protect_locations[phaseidx]].phases &= (NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases & 0x07);

						//Flag the remote object's appropriate phases
						NR_branchdata[protect_locations[phaseidx]].faultphases &= ~(phase_restore);

						//Remove the branch as an index in appropriate phases
						for (phaseidx=0; phaseidx < 3; phaseidx++)
						{
							temp_phases = 0x04 >> phaseidx;	//Figure out the phase we are on and if it is valid

							if ((phase_restore & temp_phases) == temp_phases)
							{
								protect_locations[phaseidx] = -1;	//Clear our location
							}
						}

						//Update our fault phases
						NR_branchdata[NR_branch_reference].faultphases &= ~(phase_restore);

						//Break out of this pesky loop
						break;
					}//end sectionalizer closure
					else if (NR_branchdata[protect_locations[phaseidx]].lnk_type == 3)	//fuse
					{
						//Get the fuse
						tmpobj = gl_get_object(NR_branchdata[protect_locations[phaseidx]].name);

						if (tmpobj == NULL)
						{
							GL_THROW("An attempt to alter fuse %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
							//Defined above
						}

						//Check and see what the supporting nodes are doing - if a fuse is already in a fault, this causes issues
						temp_phases = ((NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases | NR_busdata[NR_branchdata[protect_locations[phaseidx]].to].phases) & 0x07);

						if ((phase_restore & temp_phases) == phase_restore)	//Support is available, one way or another
						{
							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_fuse_state"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change fuse state on %s",tmpobj->name);
								//Defined above
							}

							//Update the fuse statii
							ext_result = ((int (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,phase_restore,true);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter fuse %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}
						}
						else if ((phase_restore & temp_phases) != 0x00)	//Some support is available
						{
							//Update fault phases
							temp_phases_B = (phase_restore & temp_phases);		//these are the phases to restore
							temp_phases = (phase_restore & (~temp_phases_B));	//These are the phases just to update

							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_fuse_faults"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change fuse state on %s",tmpobj->name);
								//Defined above - despite being slightly different
							}

							//Update the fuse statii
							ext_result = ((int (*)(OBJECT *, unsigned char))(*funadd))(tmpobj,temp_phases);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter fuse %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}

							//Restore other phase(s)
							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_fuse_state"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change fuse state on %s",tmpobj->name);
								//Defined above
							}

							//Update the fuse statii
							ext_result = ((int (*)(OBJECT *, unsigned char, bool))(*funadd))(tmpobj,temp_phases_B,true);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter fuse %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}
						}
						else	//No support available
						{
							//Just update fault phases
							//Get function address
							funadd = (FUNCTIONADDR)(gl_get_function(tmpobj,"change_fuse_faults"));
							
							//Make sure it was found
							if (funadd == NULL)
							{
								GL_THROW("Unable to change fuse state on %s",tmpobj->name);
								//Defined above - despite being slightly different
							}

							//Update the fuse statii
							ext_result = ((int (*)(OBJECT *, unsigned char))(*funadd))(tmpobj,phase_restore);

							//Make sure it worked
							if (ext_result != 1)
							{
								GL_THROW("An attempt to alter fuse %s failed.",NR_branchdata[protect_locations[phaseidx]].name);
								//defined above
							}
						}

						//Modify our phases as appropriate
						NR_branchdata[protect_locations[phaseidx]].phases &= (NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases & 0x07);

						//Flag the remote object's appropriate phases
						NR_branchdata[protect_locations[phaseidx]].faultphases &= ~(phase_restore);

						//Remove the branch as an index in appropriate phases
						for (phaseidx=0; phaseidx < 3; phaseidx++)
						{
							temp_phases = 0x04 >> phaseidx;	//Figure out the phase we are on and if it is valid

							if ((phase_restore & temp_phases) == temp_phases)
							{
								protect_locations[phaseidx] = -1;	//Clear our location
							}
						}

						//Update our fault phases
						NR_branchdata[NR_branch_reference].faultphases &= ~(phase_restore);

						//Break out of this pesky loop
						break;
					}//End fuse closure
					else if (NR_branchdata[protect_locations[phaseidx]].lnk_type == 4)	//Transformer
					{
						//Transformers are special case - if the source is supported, all three phases are reinstated
						temp_phases = NR_branchdata[protect_locations[phaseidx]].origphases & 0x07;

						//Make sure we match
						if ((temp_phases & NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases) == temp_phases)
						{
							//Support is there, restore all relevant phases
							NR_branchdata[protect_locations[phaseidx]].faultphases &= ~(temp_phases);

							//Check for SPCT rating
							if ((NR_branchdata[protect_locations[phaseidx]].origphases & 0x80) == 0x80)	//SPCT
							{
								temp_phases |= 0x80;	//Apply SPCT flag
							}
							//Defaulted else - normal xformer

							//Pull the phases back into service
							NR_branchdata[protect_locations[phaseidx]].phases = (NR_branchdata[protect_locations[phaseidx]].origphases & temp_phases);
						}
						else	//Support is not there - clear the flag, but don't restore
						{
							NR_branchdata[protect_locations[phaseidx]].faultphases &= ~(temp_phases);
						}

						//Remove our fault for the appropriate phases
						temp_phases = NR_branchdata[protect_locations[phaseidx]].origphases & 0x07;
						NR_branchdata[NR_branch_reference].faultphases &= ~(temp_phases);

						//Remove fault indices - loop through transformer phases
						for (indexval=0; indexval<3; indexval++)
						{
							temp_phases = 0x04 >> indexval;

							if ((temp_phases & NR_branchdata[NR_branch_reference].faultphases) == 0x00)	//No fault here
								protect_locations[indexval] = -1;	//Clear flag
						}

						break;	//Get us out of phase loop - transformers handle all 3 phases at once
					}//End transformer restoration
					else
					{
						GL_THROW("Protective device %s invalid for restoration!",NR_branchdata[protect_locations[phaseidx]].name);
						/*  TROUBLESHOOT
						While attempting to restore a protective device, something besides a switch, fuse, or transformer was used.
						This should not have happened.  Please try again.  If the error persists, please submit your code and a bug
						report using the trac website.
						*/
					}
				}

				//Clear "far" device lockout
				NR_branchdata[protect_locations[phaseidx]].faultphases &= ~(work_phases);	//Remove the phase lock

				//Clear faulted device lockout
				protect_locations[phaseidx] = -1;

				//Clear faulted device protect_locations
				NR_branchdata[NR_branch_reference].faultphases &= ~(work_phases);	//Remove the phase lock from us as well
			}//End valid phase
			//Default else - not a phase we care about
		}//End phase loop

		//Perform rescan for validities
		//Safety device enacted - now call fault_check function and let it restore  objects
		//Map the function
		funadd = (FUNCTIONADDR)(gl_get_function(fault_check_object,"reliability_alterations"));
		
		//Make sure it was found
		if (funadd == NULL)
		{
			GL_THROW("Unable to update objects for reliability effects");
			//Defined above
		}

		//Make sure we have a valid value
		if (temp_node == -1)
		{
			GL_THROW("Attempts to map a fault location failed!");
			/*  TROUBLESHOOT
			While attempting to restore a fault's protective device, no location
			for the protective device was found.  If the problem persists, please submit
			your code and a bug report using the trac website.
			*/
		}

		//Update powerflow - removal mode
		ext_result = ((int (*)(OBJECT *, int, bool))(*funadd))(fault_check_object,temp_node,true);

		//Make sure it worked
		if (ext_result != 1)
		{
			GL_THROW("Unable to update objects for reliability effects");
			//defined above
		}

		LOCK_OBJECT(NR_swing_bus);	//Lock SWING since we'll be modifying this
		NR_admit_change = true;	
		UNLOCK_OBJECT(NR_swing_bus);	//Release us

		if (temp_node == -99)
			gl_verbose("Event %s removed from %s by restoring %s",imp_fault_name,objhdr->name,NR_busdata[0].name);
		else
			gl_verbose("Event %s removed from %s by restoring %s",imp_fault_name,objhdr->name,NR_branchdata[temp_node].name);
	}//End actual change

	return 1;
}
//adding function to calculate the fault current seen at the swing bus then distribut that current down to fault path to the faulted line
//right now it is assumed that all faults occur at the to end of the faulted link object.
void link::fault_current_calc(complex C[7][7],unsigned int removed_phase)
{
	int temp_branch_fc, temp_node, current_branch, temp_connection_type;;
	unsigned int temp_table_loc;
	unsigned char temp_branch_phases;
	char *temp_branch_name;
	OBJECT *temp_transformer, *temp_transformer_configuration;
	PROPERTY *temp_trans_config, *temp_con_typ;
	double temp_v_ratio;
	complex double_phase_det;
	complex V_sb[3];
	complex Z_thevenin[3][3];
	complex Y_thevenin[3][3];
	complex Y_temp[3][3];
	complex Z_temp[3][3];
	complex A_t[3][3];
	complex IP[7];
	complex L[7][7];
	complex U[7][7];
	complex zz[7];
	complex xx[7];

	// zero Z_thevenin !
	Z_thevenin[0][0]=Z_thevenin[0][1]=Z_thevenin[0][2]=Z_thevenin[1][0]=Z_thevenin[1][1]=Z_thevenin[1][2]=Z_thevenin[2][0]=Z_thevenin[2][1]=Z_thevenin[2][2]=0;

	temp_branch_fc = NR_branch_reference;
	temp_node = NR_branchdata[temp_branch_fc].from;
	NR_branchdata[temp_branch_fc].fault_link_below = -1; //-1 indicates that temp_branch_fc is the faulted link object 
	temp_branch_phases = removed_phase | NR_branchdata[temp_branch_fc].phases ;

	
	// loop that sums up all the link impedances from the swing bus to the faulted link object.
	while (NR_busdata[temp_node].type != 2)
	{
		//Pull from bus of current link
		current_branch = temp_branch_fc;
		//Pull link admittance data and reference Z_thevenin to primary side
		if(NR_branchdata[temp_branch_fc].lnk_type == 4){ //transformer or regulation
			temp_branch_name = NR_branchdata[temp_branch_fc].name;//get the name of the transformer object
			temp_transformer = gl_get_object(temp_branch_name);//get the transformer object
			if(gl_object_isa(temp_transformer, "transformer", "powerflow")){ // tranformer
				temp_trans_config = gl_get_property(temp_transformer,"configuration");//get pointer to the configuration property
				temp_transformer_configuration = gl_get_object_prop(temp_transformer, temp_trans_config);//get the transformer configuration object
				temp_con_typ = gl_get_property(temp_transformer_configuration, "connect_type");//get pointer to the connection type
				temp_connection_type = *(int *)gl_get_enum(temp_transformer_configuration, temp_con_typ);//get connection type
				if(temp_connection_type == 1 || temp_connection_type == 2){//WYE_WYE or DELTA-DELTA transformer
					if(temp_branch_phases == 0x07){//has all three phases
						Y_temp[0][0] = NR_branchdata[temp_branch_fc].YSto[0];
						Y_temp[0][1] = NR_branchdata[temp_branch_fc].YSto[1];
						Y_temp[0][2] = NR_branchdata[temp_branch_fc].YSto[2];
						Y_temp[1][0] = NR_branchdata[temp_branch_fc].YSto[3];
						Y_temp[1][1] = NR_branchdata[temp_branch_fc].YSto[4];
						Y_temp[1][2] = NR_branchdata[temp_branch_fc].YSto[5];
						Y_temp[2][0] = NR_branchdata[temp_branch_fc].YSto[6];
						Y_temp[2][1] = NR_branchdata[temp_branch_fc].YSto[7];
						Y_temp[2][2] = NR_branchdata[temp_branch_fc].YSto[8];
						inverse(Y_temp,Z_temp);//Z_temp holds the transformer impedance referenced to the to side
						A_t[0][0] = A_t[1][1] = A_t[2][2] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low
					} else if(temp_branch_phases == 0x06){//has phases A and B
						double_phase_det = (NR_branchdata[temp_branch_fc].YSto[0]*NR_branchdata[temp_branch_fc].YSto[4])-(NR_branchdata[temp_branch_fc].YSto[3]*NR_branchdata[temp_branch_fc].YSto[1]);
						Z_temp[0][0] = NR_branchdata[temp_branch_fc].YSto[4]/double_phase_det;
						Z_temp[0][1] = -NR_branchdata[temp_branch_fc].YSto[1]/double_phase_det;
						Z_temp[1][0] = -NR_branchdata[temp_branch_fc].YSto[3]/double_phase_det;
						Z_temp[1][1] = NR_branchdata[temp_branch_fc].YSto[0]/double_phase_det;
						A_t[0][0] = A_t[1][1] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low 
					} else if(temp_branch_phases == 0x05){//has phases A and C
						double_phase_det = (NR_branchdata[temp_branch_fc].YSto[0]*NR_branchdata[temp_branch_fc].YSto[8])-(NR_branchdata[temp_branch_fc].YSto[6]*NR_branchdata[temp_branch_fc].YSto[2]);
						Z_temp[0][0] = NR_branchdata[temp_branch_fc].YSto[8]/double_phase_det;
						Z_temp[0][2] = -NR_branchdata[temp_branch_fc].YSto[2]/double_phase_det;
						Z_temp[2][0] = -NR_branchdata[temp_branch_fc].YSto[6]/double_phase_det;
						Z_temp[2][2] = NR_branchdata[temp_branch_fc].YSto[0]/double_phase_det;
						A_t[0][0] = A_t[2][2] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low
					} else if(temp_branch_phases == 0x03){//has phases B and C
						double_phase_det = (NR_branchdata[temp_branch_fc].YSto[4]*NR_branchdata[temp_branch_fc].YSto[8])-(NR_branchdata[temp_branch_fc].YSto[7]*NR_branchdata[temp_branch_fc].YSto[5]);
						Z_temp[1][1] = NR_branchdata[temp_branch_fc].YSto[8]/double_phase_det;
						Z_temp[1][2] = -NR_branchdata[temp_branch_fc].YSto[5]/double_phase_det;
						Z_temp[2][1] = -NR_branchdata[temp_branch_fc].YSto[7]/double_phase_det;
						Z_temp[2][2] = NR_branchdata[temp_branch_fc].YSto[4]/double_phase_det;
						A_t[1][1] = A_t[2][2] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low
					} else if(temp_branch_phases == 0x04){//has phase A
						Z_temp[0][0] = complex(1,0)/NR_branchdata[temp_branch_fc].YSto[0];
						A_t[0][0] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low
					} else if(temp_branch_phases == 0x02){//has phase B
						Z_temp[1][1] = complex(1,0)/NR_branchdata[temp_branch_fc].YSto[4];
						A_t[1][1] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low
					} else if(temp_branch_phases == 0x01){//has phase C
						Z_temp[2][2] = complex(1,0)/NR_branchdata[temp_branch_fc].YSto[8];
						A_t[2][2] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low
					}
					addition(Z_thevenin,Z_temp,Z_temp);
					multiply(A_t,Z_temp,Z_thevenin);
				} else if(temp_connection_type == 3){//Delta grounded WYE transformer
					gl_warning("Delta-grounded WYE transformers are not supported for fault analysis at this time. Fault current is not accurate.");
				} else if(temp_connection_type == 4){// Single phase transformer
					gl_warning("Single phase transformers are not supported for fault analysis at this time. Fault current is not accurate.");
				} else {//split-phase transformer
					gl_warning("split-phase transformers are not supported for fault analysis at this time. Fault current is not accurate.");
				}
			} else if(gl_object_isa(temp_transformer, "regulator", "powerflow")){ // regulator
				gl_warning("regulators are not supported for fault analysis at this time. Fault current is not accurate.");
			} else {
				GL_THROW("link object is a type 4 but is not a transformer or a regulator!");
			}
		} else { //line or safety device 
			//convert the admittance matrix to the impedance matrix
			if(temp_branch_phases == 0x07){//has all three phases
				Y_temp[0][0] = NR_branchdata[temp_branch_fc].YSto[0];
				Y_temp[0][1] = NR_branchdata[temp_branch_fc].YSto[1];
				Y_temp[0][2] = NR_branchdata[temp_branch_fc].YSto[2];
				Y_temp[1][0] = NR_branchdata[temp_branch_fc].YSto[3];
				Y_temp[1][1] = NR_branchdata[temp_branch_fc].YSto[4];
				Y_temp[1][2] = NR_branchdata[temp_branch_fc].YSto[5];
				Y_temp[2][0] = NR_branchdata[temp_branch_fc].YSto[6];
				Y_temp[2][1] = NR_branchdata[temp_branch_fc].YSto[7];
				Y_temp[2][2] = NR_branchdata[temp_branch_fc].YSto[8];
				inverse(Y_temp,Z_temp);
			} else if(temp_branch_phases == 0x06){//has phases A and B
				double_phase_det = (NR_branchdata[temp_branch_fc].YSto[0]*NR_branchdata[temp_branch_fc].YSto[4])-(NR_branchdata[temp_branch_fc].YSto[3]*NR_branchdata[temp_branch_fc].YSto[1]);
				Z_temp[0][0] = NR_branchdata[temp_branch_fc].YSto[4]/double_phase_det;
				Z_temp[0][1] = -NR_branchdata[temp_branch_fc].YSto[1]/double_phase_det;
				Z_temp[1][0] = -NR_branchdata[temp_branch_fc].YSto[3]/double_phase_det;
				Z_temp[1][1] = NR_branchdata[temp_branch_fc].YSto[0]/double_phase_det;
			} else if(temp_branch_phases == 0x05){//has phases A and C
				double_phase_det = (NR_branchdata[temp_branch_fc].YSto[0]*NR_branchdata[temp_branch_fc].YSto[8])-(NR_branchdata[temp_branch_fc].YSto[6]*NR_branchdata[temp_branch_fc].YSto[2]);
				Z_temp[0][0] = NR_branchdata[temp_branch_fc].YSto[8]/double_phase_det;
				Z_temp[0][2] = -NR_branchdata[temp_branch_fc].YSto[2]/double_phase_det;
				Z_temp[2][0] = -NR_branchdata[temp_branch_fc].YSto[6]/double_phase_det;
				Z_temp[2][2] = NR_branchdata[temp_branch_fc].YSto[0]/double_phase_det;
			} else if(temp_branch_phases == 0x03){//has phases B and C
				double_phase_det = (NR_branchdata[temp_branch_fc].YSto[4]*NR_branchdata[temp_branch_fc].YSto[8])-(NR_branchdata[temp_branch_fc].YSto[7]*NR_branchdata[temp_branch_fc].YSto[5]);
				Z_temp[1][1] = NR_branchdata[temp_branch_fc].YSto[8]/double_phase_det;
				Z_temp[1][2] = -NR_branchdata[temp_branch_fc].YSto[5]/double_phase_det;
				Z_temp[2][1] = -NR_branchdata[temp_branch_fc].YSto[7]/double_phase_det;
				Z_temp[2][2] = NR_branchdata[temp_branch_fc].YSto[4]/double_phase_det;
			} else if(temp_branch_phases == 0x04){//has phase A
				Z_temp[0][0] = complex(1,0)/NR_branchdata[temp_branch_fc].YSto[0];
			} else if(temp_branch_phases == 0x02){//has phase B
				Z_temp[1][1] = complex(1,0)/NR_branchdata[temp_branch_fc].YSto[4];
			} else if(temp_branch_phases == 0x01){//has phase C
				Z_temp[2][2] = complex(1,0)/NR_branchdata[temp_branch_fc].YSto[8];
			}

			addition(Z_thevenin,Z_temp,Z_thevenin);
		}
		
		for (temp_table_loc=0; temp_table_loc<NR_busdata[temp_node].Link_Table_Size; temp_table_loc++)
		{
			//See if node is a to end - assumes radial phase progressions (i.e., no phase AB and phase C running into a node to form node ABC)
			if (NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].to == temp_node)	//This node is a to end
			{
				//Go up to the next level
				temp_branch_fc = NR_busdata[temp_node].Link_Table[temp_table_loc];
				NR_branchdata[temp_branch_fc].fault_link_below = current_branch;
				break;	//Out of this for we go!
			}//End it is a to-end
			//Default else - it must be a from, just proceed
		}//End link table for traversion

		//Make sure we didn't somehow reach the end
		if (temp_table_loc == NR_busdata[temp_node].Link_Table_Size)
		{
			GL_THROW("Error finding proper to reference for node %s",NR_busdata[temp_node].name);
			/*  TROUBLESHOOT
			While attempting to enduce a safety reaction to a fault, a progression through the
			links of the system failed.  Please try again.  If the bug persists, please submit your
			*/
		}
		temp_node = NR_branchdata[temp_branch_fc].from;
		temp_branch_phases = NR_branchdata[temp_branch_fc].phases & 0x07;
	}
	//include the last link's impedance in the thevenin impedance matrix
	if(NR_branchdata[temp_branch_fc].lnk_type == 4){ //transformer
		temp_branch_name = NR_branchdata[temp_branch_fc].name;//get the name of the transformer object
		temp_transformer = gl_get_object(temp_branch_name);//get the transformer object
		if(gl_object_isa(temp_transformer, "transformer", "powerflow")){ // tranformer
			temp_trans_config = gl_get_property(temp_transformer,"configuration");//get pointer to the configuration property
			temp_transformer_configuration = gl_get_object_prop(temp_transformer, temp_trans_config);//get the transformer configuration object
			temp_con_typ = gl_get_property(temp_transformer_configuration, "connect_type");//get pointer to the connection type
			temp_connection_type = *(int *)gl_get_enum(temp_transformer_configuration, temp_con_typ);//get connection type
			if(temp_connection_type == 1 || temp_connection_type == 2){//WYE_WYE or DELTA-DELTA transformer
				if(temp_branch_phases == 0x07){//has all three phases
					Y_temp[0][0] = NR_branchdata[temp_branch_fc].YSto[0];
					Y_temp[0][1] = NR_branchdata[temp_branch_fc].YSto[1];
					Y_temp[0][2] = NR_branchdata[temp_branch_fc].YSto[2];
					Y_temp[1][0] = NR_branchdata[temp_branch_fc].YSto[3];
					Y_temp[1][1] = NR_branchdata[temp_branch_fc].YSto[4];
					Y_temp[1][2] = NR_branchdata[temp_branch_fc].YSto[5];
					Y_temp[2][0] = NR_branchdata[temp_branch_fc].YSto[6];
					Y_temp[2][1] = NR_branchdata[temp_branch_fc].YSto[7];
					Y_temp[2][2] = NR_branchdata[temp_branch_fc].YSto[8];
					inverse(Y_temp,Z_temp);//Z_temp holds the transformer impedance referenced to the to side
					A_t[0][0] = A_t[1][1] = A_t[2][2] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low
				} else if(temp_branch_phases == 0x06){//has phases A and B
					double_phase_det = (NR_branchdata[temp_branch_fc].YSto[0]*NR_branchdata[temp_branch_fc].YSto[4])-(NR_branchdata[temp_branch_fc].YSto[3]*NR_branchdata[temp_branch_fc].YSto[1]);
					Z_temp[0][0] = NR_branchdata[temp_branch_fc].YSto[4]/double_phase_det;
					Z_temp[0][1] = -NR_branchdata[temp_branch_fc].YSto[1]/double_phase_det;
					Z_temp[1][0] = -NR_branchdata[temp_branch_fc].YSto[3]/double_phase_det;
					Z_temp[1][1] = NR_branchdata[temp_branch_fc].YSto[0]/double_phase_det;
					A_t[0][0] = A_t[1][1] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low 
				} else if(temp_branch_phases == 0x05){//has phases A and C
					double_phase_det = (NR_branchdata[temp_branch_fc].YSto[0]*NR_branchdata[temp_branch_fc].YSto[8])-(NR_branchdata[temp_branch_fc].YSto[6]*NR_branchdata[temp_branch_fc].YSto[2]);
					Z_temp[0][0] = NR_branchdata[temp_branch_fc].YSto[8]/double_phase_det;
					Z_temp[0][2] = -NR_branchdata[temp_branch_fc].YSto[2]/double_phase_det;
					Z_temp[2][0] = -NR_branchdata[temp_branch_fc].YSto[6]/double_phase_det;
					Z_temp[2][2] = NR_branchdata[temp_branch_fc].YSto[0]/double_phase_det;
					A_t[0][0] = A_t[2][2] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low
				} else if(temp_branch_phases == 0x03){//has phases B and C
					double_phase_det = (NR_branchdata[temp_branch_fc].YSto[4]*NR_branchdata[temp_branch_fc].YSto[8])-(NR_branchdata[temp_branch_fc].YSto[7]*NR_branchdata[temp_branch_fc].YSto[5]);
					Z_temp[1][1] = NR_branchdata[temp_branch_fc].YSto[8]/double_phase_det;
					Z_temp[1][2] = -NR_branchdata[temp_branch_fc].YSto[5]/double_phase_det;
					Z_temp[2][1] = -NR_branchdata[temp_branch_fc].YSto[7]/double_phase_det;
					Z_temp[2][2] = NR_branchdata[temp_branch_fc].YSto[4]/double_phase_det;
					A_t[1][1] = A_t[2][2] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low
				} else if(temp_branch_phases == 0x04){//has phase A
					Z_temp[0][0] = complex(1,0)/NR_branchdata[temp_branch_fc].YSto[0];
					A_t[0][0] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low
				} else if(temp_branch_phases == 0x02){//has phase B
					Z_temp[1][1] = complex(1,0)/NR_branchdata[temp_branch_fc].YSto[4];
					A_t[1][1] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low
				} else if(temp_branch_phases == 0x01){//has phase C
					Z_temp[2][2] = complex(1,0)/NR_branchdata[temp_branch_fc].YSto[8];
					A_t[2][2] = NR_branchdata[temp_branch_fc].v_ratio*NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_high = A_t * Z_low
				}
				addition(Z_thevenin,Z_temp,Z_temp);
				multiply(A_t,Z_temp,Z_thevenin);
				} else if(temp_connection_type == 3){//Delta grounded WYE transformer
					gl_warning("Delta-grounded WYE transformers are not supported for fault analysis at this time. Fault current is not accurate.");
				} else if(temp_connection_type == 4){// Single phase transformer
					gl_warning("Single phase transformers are not supported for fault analysis at this time. Fault current is not accurate.");
				} else {//split-phase transformer
					gl_warning("split-phase transformers are not supported for fault analysis at this time. Fault current is not accurate.");
				}
		} else if (gl_object_isa(temp_transformer, "regulator", "powerflow")){ // regulator
			gl_warning("regulators are not supported for fault analysis at this time. Fault current is not accurate.");
		} else {
			GL_THROW("link object is a type 4 but is not a transformer or a regulator!");
		}
	} else { //line or safety device 
		if(temp_branch_phases == 0x07){//has all three phases
			Y_temp[0][0] = NR_branchdata[temp_branch_fc].YSto[0];
			Y_temp[0][1] = NR_branchdata[temp_branch_fc].YSto[1];
			Y_temp[0][2] = NR_branchdata[temp_branch_fc].YSto[2];
			Y_temp[1][0] = NR_branchdata[temp_branch_fc].YSto[3];
			Y_temp[1][1] = NR_branchdata[temp_branch_fc].YSto[4];
			Y_temp[1][2] = NR_branchdata[temp_branch_fc].YSto[5];
			Y_temp[2][0] = NR_branchdata[temp_branch_fc].YSto[6];
			Y_temp[2][1] = NR_branchdata[temp_branch_fc].YSto[7];
			Y_temp[2][2] = NR_branchdata[temp_branch_fc].YSto[8];
			inverse(Y_temp,Z_temp);//Z_temp holds the transformer impedance referenced to the to side
		} else if(temp_branch_phases == 0x06){//has phases A and B
			double_phase_det = (NR_branchdata[temp_branch_fc].YSto[0]*NR_branchdata[temp_branch_fc].YSto[4])-(NR_branchdata[temp_branch_fc].YSto[3]*NR_branchdata[temp_branch_fc].YSto[1]);
			Z_temp[0][0] = NR_branchdata[temp_branch_fc].YSto[4]/double_phase_det;
			Z_temp[0][1] = -NR_branchdata[temp_branch_fc].YSto[1]/double_phase_det;
			Z_temp[1][0] = -NR_branchdata[temp_branch_fc].YSto[3]/double_phase_det;
			Z_temp[1][1] = NR_branchdata[temp_branch_fc].YSto[0]/double_phase_det;
		} else if(temp_branch_phases == 0x05){//has phases A and C
			double_phase_det = (NR_branchdata[temp_branch_fc].YSto[0]*NR_branchdata[temp_branch_fc].YSto[8])-(NR_branchdata[temp_branch_fc].YSto[6]*NR_branchdata[temp_branch_fc].YSto[2]);
			Z_temp[0][0] = NR_branchdata[temp_branch_fc].YSto[8]/double_phase_det;
			Z_temp[0][2] = -NR_branchdata[temp_branch_fc].YSto[2]/double_phase_det;
			Z_temp[2][0] = -NR_branchdata[temp_branch_fc].YSto[6]/double_phase_det;
			Z_temp[2][2] = NR_branchdata[temp_branch_fc].YSto[0]/double_phase_det;
		} else if(temp_branch_phases == 0x03){//has phases B and C
			double_phase_det = (NR_branchdata[temp_branch_fc].YSto[4]*NR_branchdata[temp_branch_fc].YSto[8])-(NR_branchdata[temp_branch_fc].YSto[7]*NR_branchdata[temp_branch_fc].YSto[5]);
			Z_temp[1][1] = NR_branchdata[temp_branch_fc].YSto[8]/double_phase_det;
			Z_temp[1][2] = -NR_branchdata[temp_branch_fc].YSto[5]/double_phase_det;
			Z_temp[2][1] = -NR_branchdata[temp_branch_fc].YSto[7]/double_phase_det;
			Z_temp[2][2] = NR_branchdata[temp_branch_fc].YSto[4]/double_phase_det;
		} else if(temp_branch_phases == 0x04){//has phase A
			Z_temp[0][0] = complex(1,0)/NR_branchdata[temp_branch_fc].YSto[0];
		} else if(temp_branch_phases == 0x02){//has phase B
			Z_temp[1][1] = complex(1,0)/NR_branchdata[temp_branch_fc].YSto[4];
		} else if(temp_branch_phases == 0x01){//has phase C
			Z_temp[2][2] = complex(1,0)/NR_branchdata[temp_branch_fc].YSto[8];
		}

		addition(Z_thevenin,Z_temp,Z_thevenin);
	}

	inverse(Z_thevenin,Y_thevenin);
	V_sb[0] = NR_busdata[temp_node].V[0];
	V_sb[1] = NR_busdata[temp_node].V[1];
	V_sb[2] = NR_busdata[temp_node].V[2];
	//calculate the full three phase to ground fault current at the swing bus
	IP[0] = Y_thevenin[0][0]*V_sb[0] + Y_thevenin[0][1]*V_sb[1] + Y_thevenin[0][2]*V_sb[2];
	IP[1] = Y_thevenin[1][0]*V_sb[0] + Y_thevenin[1][1]*V_sb[1] + Y_thevenin[1][2]*V_sb[2];
	IP[2] = Y_thevenin[2][0]*V_sb[0] + Y_thevenin[2][1]*V_sb[1] + Y_thevenin[2][2]*V_sb[2];
	IP[3] = 0;
	IP[4] = 0;
	IP[5] = 0;
	IP[6] = 0;
	//add the Y_thevenin matrix into the C matrix according to Kersting
	C[0][3] = Y_thevenin[0][0];
	C[0][4] = Y_thevenin[0][1];
	C[0][5] = Y_thevenin[0][2];
	C[0][6] = C[0][3]+C[0][4]+C[0][5];
	C[1][3] = Y_thevenin[1][0];
	C[1][4] = Y_thevenin[1][1];
	C[1][5] = Y_thevenin[1][2];
	C[1][6] = C[1][3]+C[1][4]+C[1][5];
	C[2][3] = Y_thevenin[2][0];
	C[2][4] = Y_thevenin[2][1];
	C[2][5] = Y_thevenin[2][2];
	C[2][6] = C[2][3]+C[2][4]+C[2][5];
	//decompose the C matrix
	lu_decomp(C, L, U);
	//solve A*x = b using forward and backward substitution, L*z = b and U*x = z 
	forward_sub(L, IP, zz);
	back_sub(U, zz, xx);

	//pass the fault current back down to the faulted object
	NR_branchdata[temp_branch_fc].If_from[0] = xx[0];
	NR_branchdata[temp_branch_fc].If_from[1] = xx[1];
	NR_branchdata[temp_branch_fc].If_from[2] = xx[2];

	while(NR_branchdata[temp_branch_fc].fault_link_below != -1){
		if(NR_branchdata[temp_branch_fc].lnk_type == 4){//transformer
			temp_branch_name = NR_branchdata[temp_branch_fc].name;//get the name of the transformer object
			temp_transformer = gl_get_object(temp_branch_name);//get the transformer object
			if(gl_object_isa(temp_transformer, "transformer", "powerflow")){ // tranformer
				temp_trans_config = gl_get_property(temp_transformer,"configuration");//get pointer to the configuration property
				temp_transformer_configuration = gl_get_object_prop(temp_transformer, temp_trans_config);//get the transformer configuration object
				temp_con_typ = gl_get_property(temp_transformer_configuration, "connect_type");//get pointer to the connection type
				temp_connection_type = *(int *)gl_get_enum(temp_transformer_configuration, temp_con_typ);//get connection type
				if(temp_connection_type == 1 || temp_connection_type == 2){//WYE_WYE or DELTA-DELTA transformer
					temp_v_ratio = NR_branchdata[temp_branch_fc].v_ratio;
					NR_branchdata[temp_branch_fc].If_to[0] = NR_branchdata[temp_branch_fc].If_from[0]*temp_v_ratio;
					NR_branchdata[temp_branch_fc].If_to[1] = NR_branchdata[temp_branch_fc].If_from[1]*temp_v_ratio;
					NR_branchdata[temp_branch_fc].If_to[2] = NR_branchdata[temp_branch_fc].If_from[2]*temp_v_ratio;

					NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[0] = NR_branchdata[temp_branch_fc].If_to[0];
					NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[1] = NR_branchdata[temp_branch_fc].If_to[1];
					NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[2] = NR_branchdata[temp_branch_fc].If_to[2];
				} else if(temp_connection_type == 3){//Delta grounded WYE transformer
					gl_warning("Delta-grounded WYE transformers are not supported for fault analysis at this time. Fault current is not accurate.");
				} else if(temp_connection_type == 4){// Single phase transformer
					gl_warning("Single phase transformers are not supported for fault analysis at this time. Fault current is not accurate.");
				} else {//split-phase transformer
					gl_warning("split-phase transformers are not supported for fault analysis at this time. Fault current is not accurate.");
				}
			} else if (gl_object_isa(temp_transformer, "regulator", "powerflow")){ // regulator
				gl_warning("regulators are not supported for fault analysis at this time. Fault current is not accurate.");
			} else {
				GL_THROW("link object is a type 4 but is not a transformer or a regulator!");
			}
		} else {
			NR_branchdata[temp_branch_fc].If_to[0] = NR_branchdata[temp_branch_fc].If_from[0];
			NR_branchdata[temp_branch_fc].If_to[1] = NR_branchdata[temp_branch_fc].If_from[1];
			NR_branchdata[temp_branch_fc].If_to[2] = NR_branchdata[temp_branch_fc].If_from[2];

			NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[0] = NR_branchdata[temp_branch_fc].If_to[0];
			NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[1] = NR_branchdata[temp_branch_fc].If_to[1];
			NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[2] = NR_branchdata[temp_branch_fc].If_to[2];
		}
		temp_branch_fc = NR_branchdata[temp_branch_fc].fault_link_below;
	}
	//calculate the fault current to .
	NR_branchdata[temp_branch_fc].If_to[0] = NR_branchdata[temp_branch_fc].If_from[0];
	NR_branchdata[temp_branch_fc].If_to[1] = NR_branchdata[temp_branch_fc].If_from[1];
	NR_branchdata[temp_branch_fc].If_to[2] = NR_branchdata[temp_branch_fc].If_from[2];
}
//////////////////////////////////////////////////////////////////////////
// MATRIX OPS FOR LINKS
//////////////////////////////////////////////////////////////////////////

void inverse(complex in[3][3], complex out[3][3])
{
	complex x = complex(1.0) / (in[0][0] * in[1][1] * in[2][2] -
                               in[0][0] * in[1][2] * in[2][1] -
                               in[0][1] * in[1][0] * in[2][2] +
                               in[0][1] * in[1][2] * in[2][0] +
                               in[0][2] * in[1][0] * in[2][1] -
                               in[0][2] * in[1][1] * in[2][0]);

	out[0][0] = x * (in[1][1] * in[2][2] - in[1][2] * in[2][1]);
	out[0][1] = x * (in[0][2] * in[2][1] - in[0][1] * in[2][2]);
	out[0][2] = x * (in[0][1] * in[1][2] - in[0][2] * in[1][1]);
	out[1][0] = x * (in[1][2] * in[2][0] - in[1][0] * in[2][2]);
	out[1][1] = x * (in[0][0] * in[2][2] - in[0][2] * in[2][0]);
	out[1][2] = x * (in[0][2] * in[1][0] - in[0][0] * in[1][2]);
	out[2][0] = x * (in[1][0] * in[2][1] - in[1][1] * in[2][0]);
	out[2][1] = x * (in[0][1] * in[2][0] - in[0][0] * in[2][1]);
	out[2][2] = x * (in[0][0] * in[1][1] - in[0][1] * in[1][0]);
}

void multiply(double a, complex b[3][3], complex c[3][3])
{
	#define MUL(i, j) c[i][j] = b[i][j] * a
	MUL(0, 0); MUL(0, 1); MUL(0, 2);
	MUL(1, 0); MUL(1, 1); MUL(1, 2);
	MUL(2, 0); MUL(2, 1); MUL(2, 2);
	#undef MUL
}

void multiply(complex a[3][3], complex b[3][3], complex c[3][3])
{
	#define MUL(i, j) c[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j]
	MUL(0, 0); MUL(0, 1); MUL(0, 2);
	MUL(1, 0); MUL(1, 1); MUL(1, 2);
	MUL(2, 0); MUL(2, 1); MUL(2, 2);
	#undef MUL
}

void subtract(complex a[3][3], complex b[3][3], complex c[3][3])
{
	#define SUB(i, j) c[i][j] = a[i][j] - b[i][j]
	SUB(0, 0); SUB(0, 1); SUB(0, 2);
	SUB(1, 0); SUB(1, 1); SUB(1, 2);
	SUB(2, 0); SUB(2, 1); SUB(2, 2);
	#undef SUB
}

void addition(complex a[3][3], complex b[3][3], complex c[3][3])
{
	#define ADD(i, j) c[i][j] = a[i][j] + b[i][j]
	ADD(0, 0); ADD(0, 1); ADD(0, 2);
	ADD(1, 0); ADD(1, 1); ADD(1, 2);
	ADD(2, 0); ADD(2, 1); ADD(2, 2);
	#undef ADD
}

void equalm(complex a[3][3], complex b[3][3])
{
	#define MEQ(i, j) b[i][j] = a[i][j]
	MEQ(0, 0); MEQ(0, 1); MEQ(0, 2);
	MEQ(1, 0); MEQ(1, 1); MEQ(1, 2);
	MEQ(2, 0); MEQ(2, 1); MEQ(2, 2);
	#undef MEQ
}
void lu_decomp(complex a[7][7], complex l[7][7], complex u[7][7])
{
	int k, n, m, s;
	complex sum;
	// make sure L and U contain zeroes
	for(n=0; n<7; n++){
		for(m=0;m<7;m++){
			l[n][m] = complex(0,0);
			u[n][m] = complex(0,0);
		}
	}
	// for loop to decompose a in to lower triangular matrix l and upper triangular matirx u
	for(k=0; k<7; k++){
		l[k][k] = complex(1,0);
		for(m=k; m<7; m++){
			if(k==0){
				u[k][m] = a[k][m];
			}else{
				sum = complex(0,0);
				for(s=0; s<k; s++){
					sum += l[k][s]*u[s][m];
				}
				u[k][m] = a[k][m] - sum;
			}
		}// end U loop
		for(n=k+1; n<7; n++){
			if(k==0){
				l[n][k] = a[n][k]/u[k][k];
			}else{
				sum = complex(0,0);
				for(s=0; s<k; s++){
					sum += l[n][s]*u[s][k];
				}
				l[n][k] = (a[n][k] - sum)/u[k][k];
			}			
		}// end L loop
	}// end decomp loop
}
void forward_sub(complex l[7][7], complex b[7], complex z[7])// inputs l and b compute output z
{
	int n, m;
	z[0] = b[0];
	for(n=1; n<7; n++){
		for(m=0; m<n; m++){
			b[n] = b[n] - (l[n][m]*z[m]);
		}
		z[n] = b[n];
	}
}
void back_sub(complex u[7][7], complex z[7], complex x[7])// inputs u and z comput output x
{
	int n, m;
	x[6] = z[6]/u[6][6];
	for(n=5; n>-1; n--){
		for(m=n+1; m<7; m++){
			z[n] = z[n] - (u[n][m]*x[m]); 
		}
		x[n] = z[n]/u[n][n];
	}
}
/**@}*/
