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
#include "meter.h"
#include "regulator.h"
#include "triplex_meter.h"
#include "switch_object.h"

CLASS* link_object::oclass = NULL;
CLASS* link_object::pclass = NULL;

/**
* constructor.  Class registration is only called once to register the class with the core.
* @param mod the module struct that this class is registering in
*/
link_object::link_object(MODULE *mod) : powerflow_object(mod)
{
	// first time init
	if (oclass==NULL)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod,"link",sizeof(link_object),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class link";
		else
			oclass->trl = TRL_PROVEN;
		
		if(gl_publish_variable(oclass,
			PT_INHERIT, "powerflow_object",
			PT_enumeration, "status", PADDR(status),PT_DESCRIPTION,"",
				PT_KEYWORD, "CLOSED", (enumeration)LS_CLOSED,
				PT_KEYWORD, "OPEN", (enumeration)LS_OPEN,
			PT_object, "from",PADDR(from),PT_DESCRIPTION,"from_node - source node",
			PT_object, "to", PADDR(to),PT_DESCRIPTION,"to_node - load node",
			PT_complex, "power_in[VA]", PADDR(power_in),PT_DESCRIPTION,"power flow in (w.r.t from node)",
			PT_complex, "power_out[VA]", PADDR(power_out),PT_DESCRIPTION,"power flow out (w.r.t to node)",
			PT_double, "power_out_real[W]", PADDR(power_out.Re()),PT_DESCRIPTION,"power flow out (w.r.t to node), real",
			PT_complex, "power_losses[VA]", PADDR(power_loss),PT_DESCRIPTION,"power losses",
			PT_complex, "power_in_A[VA]", PADDR(indiv_power_in[0]),PT_DESCRIPTION,"power flow in (w.r.t from node), phase A",
			PT_complex, "power_in_B[VA]", PADDR(indiv_power_in[1]),PT_DESCRIPTION,"power flow in (w.r.t from node), phase B",
			PT_complex, "power_in_C[VA]", PADDR(indiv_power_in[2]),PT_DESCRIPTION,"power flow in (w.r.t from node), phase C",
			PT_complex, "power_out_A[VA]", PADDR(indiv_power_out[0]),PT_DESCRIPTION,"power flow out (w.r.t to node), phase A",
			PT_complex, "power_out_B[VA]", PADDR(indiv_power_out[1]),PT_DESCRIPTION,"power flow out (w.r.t to node), phase B",
			PT_complex, "power_out_C[VA]", PADDR(indiv_power_out[2]),PT_DESCRIPTION,"power flow out (w.r.t to node), phase C",
			PT_complex, "power_losses_A[VA]", PADDR(indiv_power_loss[0]),PT_DESCRIPTION,"power losses, phase A",
			PT_complex, "power_losses_B[VA]", PADDR(indiv_power_loss[1]),PT_DESCRIPTION,"power losses, phase B",
			PT_complex, "power_losses_C[VA]", PADDR(indiv_power_loss[2]),PT_DESCRIPTION,"power losses, phase C",
			PT_complex, "current_out_A[A]", PADDR(read_I_out[0]),PT_DESCRIPTION,"current flow out of link (w.r.t. to node), phase A",
			PT_complex, "current_out_B[A]", PADDR(read_I_out[1]),PT_DESCRIPTION,"current flow out of link (w.r.t. to node), phase B",
			PT_complex, "current_out_C[A]", PADDR(read_I_out[2]),PT_DESCRIPTION,"current flow out of link (w.r.t. to node), phase C",
			PT_complex, "current_in_A[A]", PADDR(read_I_in[0]),PT_DESCRIPTION,"current flow to link (w.r.t from node), phase A",
			PT_complex, "current_in_B[A]", PADDR(read_I_in[1]),PT_DESCRIPTION,"current flow to link (w.r.t from node), phase B",
			PT_complex, "current_in_C[A]", PADDR(read_I_in[2]),PT_DESCRIPTION,"current flow to link (w.r.t from node), phase C",
			PT_complex, "fault_current_in_A[A]", PADDR(If_in[0]),PT_DESCRIPTION,"fault current flowing in, phase A",
			PT_complex, "fault_current_in_B[A]", PADDR(If_in[1]),PT_DESCRIPTION,"fault current flowing in, phase B",
			PT_complex, "fault_current_in_C[A]", PADDR(If_in[2]),PT_DESCRIPTION,"fault current flowing in, phase C",
			PT_complex, "fault_current_out_A[A]", PADDR(If_out[0]),PT_DESCRIPTION,"fault current flowing out, phase A",
			PT_complex, "fault_current_out_B[A]", PADDR(If_out[1]),PT_DESCRIPTION,"fault current flowing out, phase B",
			PT_complex, "fault_current_out_C[A]", PADDR(If_out[2]),PT_DESCRIPTION,"fault current flowing out, phase C",
			PT_set, "flow_direction", PADDR(flow_direction),PT_DESCRIPTION,"flag used for describing direction of the flow of power",
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
			PT_double, "continuous_rating[A]", PADDR(link_rating[0]), PT_DESCRIPTION, "Continuous rating for this link object (set individual line segments",
			PT_double, "emergency_rating[A]", PADDR(link_rating[1]), PT_DESCRIPTION, "Emergency rating for this link object (set individual line segments",
			PT_double, "inrush_convergence_value[V]", PADDR(inrush_tol_value), PT_DESCRIPTION, "Tolerance, as change in line voltage drop between iterations, for deltamode in-rush completion",
			NULL) < 1 && errno) GL_THROW("unable to publish link properties in %s",__FILE__);

			//Publish deltamode functions
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_link)==NULL)
				GL_THROW("Unable to publish link deltamode function");
			
			//Publish restoration-related function (current update)
			if (gl_publish_function(oclass,	"update_power_pwr_object", (FUNCTIONADDR)updatepowercalc_link)==NULL)
				GL_THROW("Unable to publish link external power calculation function");
			if (gl_publish_function(oclass,	"check_limits_pwr_object", (FUNCTIONADDR)calculate_overlimit_link)==NULL)
				GL_THROW("Unable to publish link external power limit calculation function");
	}
}

int link_object::isa(char *classname)
{
	return strcmp(classname,"link")==0 || powerflow_object::isa(classname);
}

int link_object::create(void)
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

	protect_locations[0] = protect_locations[1] = protect_locations[2] = -1;	//Initalize cleared

	current_in[0] = current_in[1] = current_in[2] = complex(0,0);

	link_limits[0][0] = link_limits[0][1] = link_limits[0][2] = link_limits[1][0] = link_limits[1][1] = link_limits[1][2] = NULL;
	
	link_rating[0][0] = link_rating[0][1] = link_rating[0][2] = 1000;	//Replicates current defaults of line objects
	link_rating[1][0] = link_rating[1][1] = link_rating[1][2] = 2000;

	check_link_limits = false;

	mean_repair_time = 0.0;

	current_accumulated = false;

	deltamode_inclusive = false;	//By default, we don't support deltamode

	link_recalc_fxn = NULL;	//Initialize frequency dependence calculation

	ahrlstore = NULL;	//Initialize history terms
	bhrlstore = NULL;
	ahmstore = NULL;
	bhmstore = NULL;
	chrcstore = NULL;
	LinkHistTermL = NULL;
	LinkHistTermCf = NULL;
	LinkHistTermCt = NULL;
	LinkCapShuntTerm = NULL;	/******* DEBUG NOTE - Consider deleting this and useing YBase_Pri and YBase_Sec linked to it for LinkCap */
	YBase_Full = NULL;
	YBase_Pri = NULL;
	YBase_Sec = NULL;
	inrush_computations_needed = false;	//By default, we behave like an ordinary deltamode link
	inrush_vdiffmag_prev[0] = inrush_vdiffmag_prev[1] = inrush_vdiffmag_prev[2] = 0.0;
	deltamode_prev_time = -1.0; 
	inrush_tol_value = 0.0001;	//0.1 mV - arbitrary

	//****************** DEBUG
	//Saturation-based items -- probably need to be moved, but putting here since me=lazy
	D_sat = 0.0;
	A_phi = complex(0.0,0.0);
	B_phi = complex(0.0,0.0);
	hphi = NULL;
	saturation_calculated_vals = NULL;

	return result;
}

int link_object::init(OBJECT *parent)
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

	//Make sure they don't match -- odd test, but still needs to be done
	if (from == to)
	{
		GL_THROW("link:%d - %s - FROM and TO objects are the same object!",obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The from-node and to-node of a link are the same object. This is effectively a loop-back on itself and is not allowed.
		Please choose a different from or to node.
		*/
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
		{
			if (gl_object_isa(to->parent,"link","powerflow"))
			{
				gl_warning("%s (id:%d) already has a link parented - this could give invalid answers in FBS!",to->name,to->id);
				/*  TROUBLESHOOT
				While connecting the different objects, a link's 'to' end already has another link as a parent.  This is
				indicative of a loop or other 'non-radial' condition on the system.  The exception to this is if an open
				switch is connected at that node.  If it is not an open switch, Forward Backward-Sweep (FBS) will not
				give a proper solution in this condition.  Even if it is an open switch, be aware that FBS could still
				give an invalid answer, depending on topological conditions.  If it is an open switch, consider removing
				the open switch from your GLM to ensure proper answers.  If it is another type of link or you do not wish
				to remove the open switch, consider using the Newton-Raphson (NR) solver instead.
				*/
			}
			else if (gl_object_isa(to->parent,"node","powerflow"))
			{
				gl_warning("%s (id:%d) is parented to a node, but has a link running into it!",to->name,to->id);
				/*  TROUBLESHOOT
				While connecting the different objects, a link's 'to' end already has a parent of another node.  This may
				cause invalid answers on the system an an inaccurate powerflow.  Please check the node's connection and
				adjust your system, if necessary.  You may also consider using the Newton-Raphson solver (NR), which will not
				this issue.
				*/
			}
			else	//Business as normal
			{
				/* promote this object if necessary */
				gl_set_rank(obj,to->rank+1);
			}
		}
		break;
	case SM_GS: /* Gauss-Seidel */
		{
			GL_THROW("Gauss_Seidel is a deprecated solver and has been removed");
			/*  TROUBLESHOOT 
			The Gauss-Seidel solver implementation has been removed as of version 3.0.
			It was never fully functional and has not been updated in a couple versions.  The source
			code still exists in older repositories, so if you have an interest in that implementation, please
			try an older subversion number.
			*/

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

			if ((phase_f_test != (phases_test & ~(PHASE_S))) || (phase_t_test != phases_test) || ((phase_t_test & PHASE_S) != PHASE_S))	//Phase mismatch on the line
				GL_THROW("transformer:%d (split phase) - %s has a phase mismatch at one or both ends",obj->id,obj->name);
				/*  TROUBLESHOOT
				A line has been configured to carry a certain set of phases.  Either the input node or output
				node is not providing a source/sink for these different conductors.  The To and From nodes must
				have at least the phases of the line connecting them. Center-tap transformers specifically require
				at least a single phase (A, B, or C) on the primary side, and the same phase and phase S on the
				secondary side.
				*/
		}
		else if (SpecialLnk==DELTAGWYE)	//D-Gwye then
		{
			phase_t_test &= ~(PHASE_N | PHASE_D);	//Pull off the neutral and Delta phase portion of from node (no idea if ABCD or ABCN would be convention)
			phase_f_test &= ~(PHASE_N | PHASE_D);	//Pull off the neutral and Delta phase portion of to node

			if ((phase_f_test != (phases & ~(PHASE_N | PHASE_D))) || (phase_t_test != (phases & ~(PHASE_N | PHASE_D))))	//Phase mismatch on the line
				GL_THROW("transformer:%d (delta-grounded wye) - %s has a phase mismatch at one or both ends",obj->id,obj->name);
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

	//See if limits are enabled - if so, populate them
	if (use_link_limits==TRUE)
	{
		//See what kind of link we are - if not in this list, ignore it
		if (gl_object_isa(obj,"transformer","powerflow") || gl_object_isa(obj,"underground_line","powerflow") || gl_object_isa(obj,"overhead_line","powerflow") || gl_object_isa(obj,"triplex_line","powerflow"))	//Default line or xformer
		{
			//link us to local property (individual object populate)
			link_limits[0][0] = &link_rating[0][0];
			link_limits[0][1] = &link_rating[0][1];
			link_limits[0][2] = &link_rating[0][2];
			link_limits[1][0] = &link_rating[1][0];
			link_limits[1][1] = &link_rating[1][1];
			link_limits[1][2] = &link_rating[1][2];

			//Set flag to check
			check_link_limits = true;

			//See if the limits are zero and toss some warnings
			if (*link_limits[0][0] == 0.0 || *link_limits[0][1] == 0.0 || *link_limits[0][2] == 0.0)
			{
				gl_warning("continuous_rating for link:%s is zero - this may lead to odd warning messages about line limits with nonsense values",obj->name);
				/*  TROUBLESHOOT
				The value for continuous_rating is set to zero for the particular link object.  This may cause warnings related to line ratings
				that have nonsense values like '1.#J%'.  To resolve these warnings, please put a valid number in for continuous_rating.  Load order
				may also contribute to this problem.  Ensure all configurations are first in the GLM files.
				*/
			}

			if (*link_limits[1][0] == 0.0 || *link_limits[1][1] == 0.0 || *link_limits[1][2] == 0.0)
			{
				gl_warning("emergency_rating for link:%s is zero - this may lead to odd warning messages about line limits with nonsense values",obj->name);
				/*  TROUBLESHOOT
				The value for emergency_rating is set to zero for the particular link object.  This may cause warnings related to line ratings
				that have nonsense values like '1.#J%'.  To resolve these warnings, please put a valid number in for emergency_rating.
				Load order may also contribute to this problem.  Ensure all configurations are first in the GLM files.
				*/
			}
		}
		else
		{
			//Defaulted else - don't check - (switch, fuse, regulator, sectionalizer, recloser)

			//Set flag, just to be safe
			check_link_limits = false;
		}
	}

	//Deltamode checks - init, so no locking yet
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		//Flag the variable for later use
		deltamode_inclusive = true;

		//Increment the counter for allocation
		pwr_object_count++;

		//Check for in-rush capabilities
		if (enable_inrush_calculations==true)
		{
			//See if we're a normal line
			if (SpecialLnk == NORMAL)
			{
				//Allocate the terms -- Inductance -- fully allocate, to stay consistent
				LinkHistTermL = (complex *)gl_malloc(6*sizeof(complex));

				//Check it
				if (LinkHistTermL == NULL)
				{
					GL_THROW("Link:%s failed to allocate space for deltamode inrush history term",obj->name?obj->name:"unnamed");
					/*  TROUBLESHOOT
					While attempting to allocate memory for a link object to dynamics-required in-rush calculation
					terms, an error occurred.  Please try again.  If the error persists, please submit your code and
					a bug report to the ticketing system.
					*/
				}

				//Zero everything, to be safe
				LinkHistTermL[0] = complex(0.0,0.0);
				LinkHistTermL[1] = complex(0.0,0.0);
				LinkHistTermL[2] = complex(0.0,0.0);
				LinkHistTermL[3] = complex(0.0,0.0);
				LinkHistTermL[4] = complex(0.0,0.0);
				LinkHistTermL[5] = complex(0.0,0.0);

				//Allocate the "constant" terms
				//Ahrl
				//Allocate the terms -- inductance ahrl constant
				ahrlstore = (complex *)gl_malloc(9*sizeof(complex));

				//Check it
				if (ahrlstore == NULL)
				{
					GL_THROW("Link:%s failed to allocate space for deltamode inrush history term",obj->name?obj->name:"unnamed");
					//Defined above
				}

				//Zero everything, to be safe -- from side
				ahrlstore[0] = complex(0.0,0.0);
				ahrlstore[1] = complex(0.0,0.0);
				ahrlstore[2] = complex(0.0,0.0);
				ahrlstore[3] = complex(0.0,0.0);
				ahrlstore[4] = complex(0.0,0.0);
				ahrlstore[5] = complex(0.0,0.0);
				ahrlstore[6] = complex(0.0,0.0);
				ahrlstore[7] = complex(0.0,0.0);
				ahrlstore[8] = complex(0.0,0.0);

				//Bhrl
				//Allocate the terms -- inductance bhrl constant
				bhrlstore = (complex *)gl_malloc(9*sizeof(complex));

				//Check it
				if (bhrlstore == NULL)
				{
					GL_THROW("Link:%s failed to allocate space for deltamode inrush history term",obj->name?obj->name:"unnamed");
					//Defined above
				}

				//Zero everything, to be safe -- from side
				bhrlstore[0] = complex(0.0,0.0);
				bhrlstore[1] = complex(0.0,0.0);
				bhrlstore[2] = complex(0.0,0.0);
				bhrlstore[3] = complex(0.0,0.0);
				bhrlstore[4] = complex(0.0,0.0);
				bhrlstore[5] = complex(0.0,0.0);
				bhrlstore[6] = complex(0.0,0.0);
				bhrlstore[7] = complex(0.0,0.0);
				bhrlstore[8] = complex(0.0,0.0);

				//Allocate the capacitance history term -- if it is enabled
				if (use_line_cap == true)	//Capacitance enabled
				{
					//Allocate the terms -- Capacitance -- fully allocate, to stay consistent -- from side
					LinkHistTermCf = (complex *)gl_malloc(6*sizeof(complex));

					//Check it
					if (LinkHistTermCf == NULL)
					{
						GL_THROW("Link:%s failed to allocate space for deltamode inrush history term",obj->name?obj->name:"unnamed");
						//Defined above
					}

					//Allocate the terms -- Capacitance -- fully allocate, to stay consistent -- to side
					LinkHistTermCt = (complex *)gl_malloc(6*sizeof(complex));

					//Check it
					if (LinkHistTermCt == NULL)
					{
						GL_THROW("Link:%s failed to allocate space for deltamode inrush history term",obj->name?obj->name:"unnamed");
						//Defined above
					}

					//Zero everything, to be safe -- from side
					LinkHistTermCf[0] = complex(0.0,0.0);
					LinkHistTermCf[1] = complex(0.0,0.0);
					LinkHistTermCf[2] = complex(0.0,0.0);
					LinkHistTermCf[3] = complex(0.0,0.0);
					LinkHistTermCf[4] = complex(0.0,0.0);
					LinkHistTermCf[5] = complex(0.0,0.0);

					//Zero everything, to be safe -- to side
					LinkHistTermCt[0] = complex(0.0,0.0);
					LinkHistTermCt[1] = complex(0.0,0.0);
					LinkHistTermCt[2] = complex(0.0,0.0);
					LinkHistTermCt[3] = complex(0.0,0.0);
					LinkHistTermCt[4] = complex(0.0,0.0);
					LinkHistTermCt[5] = complex(0.0,0.0);

					//Chrc
					//Allocate the terms -- capacitance chrc constant
					chrcstore = (double *)gl_malloc(9*sizeof(double));

					//Check it
					if (chrcstore == NULL)
					{
						GL_THROW("Link:%s failed to allocate space for deltamode inrush history term",obj->name?obj->name:"unnamed");
						//Defined above
					}

					//Allocate the current calculation term
					LinkCapShuntTerm = (complex *)gl_malloc(9*sizeof(complex));

					//Check it
					if (LinkCapShuntTerm == NULL)
					{
						GL_THROW("Link:%s failed to allocate space for deltamode inrush history term",obj->name?obj->name:"unnamed");
						//Defined above
					}

					//Zero everything, to be safe -- from side
					chrcstore[0] = 0.0;
					chrcstore[1] = 0.0;
					chrcstore[2] = 0.0;
					chrcstore[3] = 0.0;
					chrcstore[4] = 0.0;
					chrcstore[5] = 0.0;
					chrcstore[6] = 0.0;
					chrcstore[7] = 0.0;
					chrcstore[8] = 0.0;

					LinkCapShuntTerm[0] = complex(0.0,0.0);
					LinkCapShuntTerm[1] = complex(0.0,0.0);
					LinkCapShuntTerm[2] = complex(0.0,0.0);
					LinkCapShuntTerm[3] = complex(0.0,0.0);
					LinkCapShuntTerm[4] = complex(0.0,0.0);
					LinkCapShuntTerm[5] = complex(0.0,0.0);
					LinkCapShuntTerm[6] = complex(0.0,0.0);
					LinkCapShuntTerm[7] = complex(0.0,0.0);
					LinkCapShuntTerm[8] = complex(0.0,0.0);
				}
				//Default else -- capacitance not enabled
			}//End "normal" (so a line
			else if (SpecialLnk == WYEWYE)	//Wye-wye transformer
			{
				//Allocate the terms -- Main transformer -- fully allocate, to stay consistent
				LinkHistTermL = (complex *)gl_malloc(12*sizeof(complex));

				//Check it
				if (LinkHistTermL == NULL)
				{
					GL_THROW("Link:%s failed to allocate space for deltamode inrush history term",obj->name?obj->name:"unnamed");
					//Defined above
				}

				//Zero everything, to be safe
				LinkHistTermL[0] = complex(0.0,0.0);
				LinkHistTermL[1] = complex(0.0,0.0);
				LinkHistTermL[2] = complex(0.0,0.0);
				LinkHistTermL[3] = complex(0.0,0.0);
				LinkHistTermL[4] = complex(0.0,0.0);
				LinkHistTermL[5] = complex(0.0,0.0);
				LinkHistTermL[6] = complex(0.0,0.0);
				LinkHistTermL[7] = complex(0.0,0.0);
				LinkHistTermL[8] = complex(0.0,0.0);
				LinkHistTermL[9] = complex(0.0,0.0);
				LinkHistTermL[10] = complex(0.0,0.0);
				LinkHistTermL[11] = complex(0.0,0.0);

				//"C" terms (magnetization) will be allocated within transformer

				//Allocate the "constant" terms
				//Ahrl
				//Allocate the terms -- inductance ahrl constant
				ahrlstore = (complex *)gl_malloc(36*sizeof(complex));

				//Check it
				if (ahrlstore == NULL)
				{
					GL_THROW("Link:%s failed to allocate space for deltamode inrush history term",obj->name?obj->name:"unnamed");
					//Defined above
				}

				//Zero everything, to be safe -- from side
				ahrlstore[0] = ahrlstore[9]  = ahrlstore[18] = ahrlstore[27] = complex(0.0,0.0);
				ahrlstore[1] = ahrlstore[10] = ahrlstore[19] = ahrlstore[28] = complex(0.0,0.0);
				ahrlstore[2] = ahrlstore[11] = ahrlstore[20] = ahrlstore[29] = complex(0.0,0.0);
				ahrlstore[3] = ahrlstore[12] = ahrlstore[21] = ahrlstore[30] = complex(0.0,0.0);
				ahrlstore[4] = ahrlstore[13] = ahrlstore[22] = ahrlstore[31] = complex(0.0,0.0);
				ahrlstore[5] = ahrlstore[14] = ahrlstore[23] = ahrlstore[32] = complex(0.0,0.0);
				ahrlstore[6] = ahrlstore[15] = ahrlstore[24] = ahrlstore[33] = complex(0.0,0.0);
				ahrlstore[7] = ahrlstore[16] = ahrlstore[25] = ahrlstore[34] = complex(0.0,0.0);
				ahrlstore[8] = ahrlstore[17] = ahrlstore[26] = ahrlstore[35] = complex(0.0,0.0);
				
				//Bhrl
				//Allocate the terms -- inductance bhrl constant
				bhrlstore = (complex *)gl_malloc(36*sizeof(complex));

				//Check it
				if (bhrlstore == NULL)
				{
					GL_THROW("Link:%s failed to allocate space for deltamode inrush history term",obj->name?obj->name:"unnamed");
					//Defined above
				}

				//Zero everything, to be safe
				bhrlstore[0] = bhrlstore[9]  = bhrlstore[18] = bhrlstore[27] = complex(0.0,0.0);
				bhrlstore[1] = bhrlstore[10] = bhrlstore[19] = bhrlstore[28] = complex(0.0,0.0);
				bhrlstore[2] = bhrlstore[11] = bhrlstore[20] = bhrlstore[29] = complex(0.0,0.0);
				bhrlstore[3] = bhrlstore[12] = bhrlstore[21] = bhrlstore[30] = complex(0.0,0.0);
				bhrlstore[4] = bhrlstore[13] = bhrlstore[22] = bhrlstore[31] = complex(0.0,0.0);
				bhrlstore[5] = bhrlstore[14] = bhrlstore[23] = bhrlstore[32] = complex(0.0,0.0);
				bhrlstore[6] = bhrlstore[15] = bhrlstore[24] = bhrlstore[33] = complex(0.0,0.0);
				bhrlstore[7] = bhrlstore[16] = bhrlstore[25] = bhrlstore[34] = complex(0.0,0.0);
				bhrlstore[8] = bhrlstore[17] = bhrlstore[26] = bhrlstore[35] = complex(0.0,0.0);

				//Ahm
				//Allocate the terms -- inductance ahrl constant
				ahmstore = (complex *)gl_malloc(18*sizeof(complex));

				//Check it
				if (ahmstore == NULL)
				{
					GL_THROW("Link:%s failed to allocate space for deltamode inrush history term",obj->name?obj->name:"unnamed");
					//Defined above
				}

				//Zero everything, to be safe -- from side
				ahmstore[0] = ahmstore[9]  = complex(0.0,0.0);
				ahmstore[1] = ahmstore[10] = complex(0.0,0.0);
				ahmstore[2] = ahmstore[11] = complex(0.0,0.0);
				ahmstore[3] = ahmstore[12] = complex(0.0,0.0);
				ahmstore[4] = ahmstore[13] = complex(0.0,0.0);
				ahmstore[5] = ahmstore[14] = complex(0.0,0.0);
				ahmstore[6] = ahmstore[15] = complex(0.0,0.0);
				ahmstore[7] = ahmstore[16] = complex(0.0,0.0);
				ahmstore[8] = ahmstore[17] = complex(0.0,0.0);
				
				//Bhrl
				//Allocate the terms -- inductance bhrl constant
				bhmstore = (complex *)gl_malloc(18*sizeof(complex));

				//Check it
				if (bhmstore == NULL)
				{
					GL_THROW("Link:%s failed to allocate space for deltamode inrush history term",obj->name?obj->name:"unnamed");
					//Defined above
				}

				//Zero everything, to be safe
				bhmstore[0] = bhmstore[9]  = complex(0.0,0.0);
				bhmstore[1] = bhmstore[10] = complex(0.0,0.0);
				bhmstore[2] = bhmstore[11] = complex(0.0,0.0);
				bhmstore[3] = bhmstore[12] = complex(0.0,0.0);
				bhmstore[4] = bhmstore[13] = complex(0.0,0.0);
				bhmstore[5] = bhmstore[14] = complex(0.0,0.0);
				bhmstore[6] = bhmstore[15] = complex(0.0,0.0);
				bhmstore[7] = bhmstore[16] = complex(0.0,0.0);
				bhmstore[8] = bhmstore[17] = complex(0.0,0.0);
			}//End transformer WYE-WYE
			//Default else - something we don't support
		}
		//Default else -- normal, so no in-rush
	}	//End deltamode items -- like in-rush

	//If frequency dependence is enabled, try to map the recalc function
	if (enable_frequency_dependence == true)
	{
		//Try to map the function.  If it fails, just assume it has no frequency dependence
		link_recalc_fxn = (FUNCTIONADDR)(gl_get_function(obj,"recalc_distribution_line"));
	}

	// KML support
	set_latitude((from->latitude+to->latitude)/2);
	set_longitude((from->longitude+to->longitude)/2);

	return 1;
}

node *link_object::get_from(void) const
{
	node *from;
	get_flow(&from,NULL);
	return from;
}
node *link_object::get_to(void) const
{
	node *to;
	get_flow(NULL,&to);
	return to;
}
set link_object::get_flow(node **fn, node **tn) const
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

//Presync portion of NR code - functionalized for deltamode
void link_object::NR_link_presync_fxn(void)
{
	OBJECT *obj = OBJECTHDR(this);
	int ret_value;
	bool force_link_update;
	double curr_delta_time;
	bool require_inrush_update, transf_from_stdy_state;
	complex work_matrix_A[6][6], work_matrix_B[6][6], work_matrix_C[6][6];
	complex work_matrix_D[3][3],work_matrix_E[3][3],work_matrix_F[3][3];
	complex work_matrix_G[6][6], work_matrix_H[6][6], work_matrix_I[6][6];
	complex work_vector_A[6], work_vector_B[6], work_vector_C[6];
	complex work_vector_D[3];
	complex temp_value_A, temp_value_B;
	char jindex, kindex;
	FUNCTIONADDR transformer_calc_function;	

	//See if a frequency dependence is desired -- if so, update it
	if (enable_frequency_dependence == true)
	{
		if (link_recalc_fxn != NULL)	//See if a function was mapped -- perform an update
		{
			//Make the call to update impedance values
			ret_value = ((int (*)(OBJECT *))(*link_recalc_fxn))(obj);

			//Make sure it worked -- not sure how this happens right now (all return 1), but just in case
			if (ret_value != 1)
			{
				GL_THROW("Updating the frequency-dependent terms of link:%d,%s failed!",obj->id,obj->name?obj->name:"unnamed");
				/*  TROUBLESHOOT
				While attempting to call the recalc function for a frequency-dependent link object, an error was returned.
				Please check your parameters and try again.
				*/
			}

			//Flag an admittance change, since we may have adjusted something
			NR_admit_change = true;

			//Force an update of the link
			force_link_update = true;
		}
		else	//No frequency dependency mapped, just treat like normal
		{
			//Set the flag
			force_link_update = false;
		}
	}//End frequency dependence check
	else	//Treat like normal
	{
		force_link_update = false;
	}
	
	//Deltamode catch and check
	if ((enable_inrush_calculations == true) && ((SpecialLnk==NORMAL) || (SpecialLnk==WYEWYE)))	//Lines are only supported at the moment -- maybe flag this different in the future?
	{
		//See if we're in deltamode or not
		if (deltatimestep_running > 0)
		{
			//Get current deltamode timestep
			curr_delta_time = gl_globaldeltaclock;

			//***************** IN RUSH STUFFF *************************/
			//See if we're a "normal line"
			if (SpecialLnk == NORMAL)
			{
				//Set flag
				require_inrush_update = true;

				//See if we're a different timestep
				if (curr_delta_time != deltamode_prev_time)
				{
					//See if we need to try and initialize differently
					if (deltamode_prev_time < 0)
					{
						//Default flag - assume we're newly energized (full-in-rush)
						transf_from_stdy_state = false;

						//Check phases - see if we have voltages - Phase A
						if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)
						{
							if ((NR_busdata[NR_branchdata[NR_branch_reference].from].V[0].Mag() > 0.0) && (NR_busdata[NR_branchdata[NR_branch_reference].to].V[0].Mag() > 0.0))
							{
								transf_from_stdy_state=true;
							}
							//Default else -- keep it false
						}
						//No phase A

						//Check phases - see if we have voltages - Phase B
						if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)
						{
							if ((NR_busdata[NR_branchdata[NR_branch_reference].from].V[1].Mag() > 0.0) && (NR_busdata[NR_branchdata[NR_branch_reference].to].V[1].Mag() > 0.0))
							{
								transf_from_stdy_state=true;
							}
							//Default else -- keep it false
						}
						//No phase B

						//Check phases - see if we have voltages - Phase C
						if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)
						{
							if ((NR_busdata[NR_branchdata[NR_branch_reference].from].V[2].Mag() > 0.0) && (NR_busdata[NR_branchdata[NR_branch_reference].to].V[2].Mag() > 0.0))
							{
								transf_from_stdy_state=true;
							}
							//Default else -- keep it false
						}
						//No phase C
					}//End init run into in-rush
					else
					{
						//Set the transformer flag for "normal"
						transf_from_stdy_state = false;
					}
					
					//Update the tracker
					deltamode_prev_time = curr_delta_time;

					//Copy the old history terms in first
					LinkHistTermL[3] = LinkHistTermL[0];	//Inductance
					LinkHistTermL[4] = LinkHistTermL[1];
					LinkHistTermL[5] = LinkHistTermL[2];

					if (use_line_cap == true)	//Only enable if capacitance is on
					{
						LinkHistTermCf[3] = LinkHistTermCf[0];	//"from" capacitance
						LinkHistTermCf[4] = LinkHistTermCf[1];
						LinkHistTermCf[5] = LinkHistTermCf[2];

						LinkHistTermCt[3] = LinkHistTermCt[0];	//"to" capacitance
						LinkHistTermCt[4] = LinkHistTermCt[1];
						LinkHistTermCt[5] = LinkHistTermCt[2];
					}

					//Determine how we are calculating values
					if (transf_from_stdy_state == false)	//"normal" update
					{
						//Calculate the updated history terms - hrl = ahrl*(vfromprev-vtoprev)-bhrl*hrlprev
						//Cheating references to voltages
						LinkHistTermL[0] = (NR_busdata[NR_branchdata[NR_branch_reference].from].V[0] - NR_busdata[NR_branchdata[NR_branch_reference].to].V[0]) * ahrlstore[0] +
										   (NR_busdata[NR_branchdata[NR_branch_reference].from].V[1] - NR_busdata[NR_branchdata[NR_branch_reference].to].V[1]) * ahrlstore[1] +
										   (NR_busdata[NR_branchdata[NR_branch_reference].from].V[2] - NR_busdata[NR_branchdata[NR_branch_reference].to].V[2]) * ahrlstore[2] -
										   bhrlstore[0] * LinkHistTermL[3] -
										   bhrlstore[1] * LinkHistTermL[4] -
										   bhrlstore[2] * LinkHistTermL[5];

						LinkHistTermL[1] = (NR_busdata[NR_branchdata[NR_branch_reference].from].V[0] - NR_busdata[NR_branchdata[NR_branch_reference].to].V[0]) * ahrlstore[3] +
										   (NR_busdata[NR_branchdata[NR_branch_reference].from].V[1] - NR_busdata[NR_branchdata[NR_branch_reference].to].V[1]) * ahrlstore[4] +
										   (NR_busdata[NR_branchdata[NR_branch_reference].from].V[2] - NR_busdata[NR_branchdata[NR_branch_reference].to].V[2]) * ahrlstore[5] -
										   bhrlstore[3] * LinkHistTermL[3] -
										   bhrlstore[4] * LinkHistTermL[4] -
										   bhrlstore[5] * LinkHistTermL[5];

						LinkHistTermL[2] = (NR_busdata[NR_branchdata[NR_branch_reference].from].V[0] - NR_busdata[NR_branchdata[NR_branch_reference].to].V[0]) * ahrlstore[6] +
										   (NR_busdata[NR_branchdata[NR_branch_reference].from].V[1] - NR_busdata[NR_branchdata[NR_branch_reference].to].V[1]) * ahrlstore[7] +
										   (NR_busdata[NR_branchdata[NR_branch_reference].from].V[2] - NR_busdata[NR_branchdata[NR_branch_reference].to].V[2]) * ahrlstore[8] -
										   bhrlstore[6] * LinkHistTermL[3] -
										   bhrlstore[7] * LinkHistTermL[4] -
										   bhrlstore[8] * LinkHistTermL[5];

						if (use_line_cap == true)	//Only do if capacitance is enabled
						{
							//Calculate the updated history terms - hrcf = chrc*vfromprev - hrcfprev
							LinkHistTermCf[0] = NR_busdata[NR_branchdata[NR_branch_reference].from].V[0] * chrcstore[0] +
												NR_busdata[NR_branchdata[NR_branch_reference].from].V[1] * chrcstore[1] +
												NR_busdata[NR_branchdata[NR_branch_reference].from].V[2] * chrcstore[2] -
												LinkHistTermCf[3];

							LinkHistTermCf[1] = NR_busdata[NR_branchdata[NR_branch_reference].from].V[0] * chrcstore[3] +
												NR_busdata[NR_branchdata[NR_branch_reference].from].V[1] * chrcstore[4] +
												NR_busdata[NR_branchdata[NR_branch_reference].from].V[2] * chrcstore[5] -
												LinkHistTermCf[4];

							LinkHistTermCf[2] = NR_busdata[NR_branchdata[NR_branch_reference].from].V[0] * chrcstore[6] +
												NR_busdata[NR_branchdata[NR_branch_reference].from].V[1] * chrcstore[7] +
												NR_busdata[NR_branchdata[NR_branch_reference].from].V[2] * chrcstore[8] -
												LinkHistTermCf[5];

							//Calculate the updated history terms - hrcf = chrc*vfromprev - hrcfprev
							LinkHistTermCt[0] = NR_busdata[NR_branchdata[NR_branch_reference].to].V[0] * chrcstore[0] +
												NR_busdata[NR_branchdata[NR_branch_reference].to].V[1] * chrcstore[1] +
												NR_busdata[NR_branchdata[NR_branch_reference].to].V[2] * chrcstore[2] -
												LinkHistTermCt[3];

							LinkHistTermCt[1] = NR_busdata[NR_branchdata[NR_branch_reference].to].V[0] * chrcstore[3] +
												NR_busdata[NR_branchdata[NR_branch_reference].to].V[1] * chrcstore[4] +
												NR_busdata[NR_branchdata[NR_branch_reference].to].V[2] * chrcstore[5] -
												LinkHistTermCt[4];

							LinkHistTermCt[2] = NR_busdata[NR_branchdata[NR_branch_reference].to].V[0] * chrcstore[6] +
												NR_busdata[NR_branchdata[NR_branch_reference].to].V[1] * chrcstore[7] +
												NR_busdata[NR_branchdata[NR_branch_reference].to].V[2] * chrcstore[8] -
												LinkHistTermCt[5];
						}//End capacitance update
					}//End "normal" run
					else	//First run from steady state
					{
						//Calculate the updated history terms - hrl = inv(1+brhl)*ahrl*(vfrom-vto)

						//Calculate the intermediate, inverse term
						for (jindex=0; jindex<3; jindex++)
						{
							for (kindex=0; kindex<3; kindex++)
							{
								if (jindex==kindex)
								{
									work_matrix_D[jindex][kindex] = complex(1.0,0.0) + bhrlstore[jindex*3+kindex];
								}
								else
								{
									work_matrix_D[jindex][kindex] = bhrlstore[jindex*3+kindex];
								}
							}
						}

						//Invert it
						lu_matrix_inverse(&work_matrix_D[0][0],&work_matrix_E[0][0],3);

						//Multiply it by the ahstore values
						lmatrix_mult(&work_matrix_E[0][0],ahrlstore,&work_matrix_F[0][0],3);

						//Compute the voltage difference
						work_vector_D[0] = NR_busdata[NR_branchdata[NR_branch_reference].from].V[0] - NR_busdata[NR_branchdata[NR_branch_reference].to].V[0];
						work_vector_D[1] = NR_busdata[NR_branchdata[NR_branch_reference].from].V[1] - NR_busdata[NR_branchdata[NR_branch_reference].to].V[1];
						work_vector_D[2] = NR_busdata[NR_branchdata[NR_branch_reference].from].V[2] - NR_busdata[NR_branchdata[NR_branch_reference].to].V[2];

						//Multiply the above term by this and store it into LinkHistL
						lmatrix_vmult(&work_matrix_F[0][0],&work_vector_D[0],LinkHistTermL,3);

						//Update capacitance terms
						if (use_line_cap == true)	//Only do if capacitance is enabled
						{
							//Calculate the updated history terms - hrcf = chrc*vfromprev/2.0
							LinkHistTermCf[0] = (NR_busdata[NR_branchdata[NR_branch_reference].from].V[0] * chrcstore[0] +
												NR_busdata[NR_branchdata[NR_branch_reference].from].V[1] * chrcstore[1] +
												NR_busdata[NR_branchdata[NR_branch_reference].from].V[2] * chrcstore[2]) / complex(2.0,0.0);

							LinkHistTermCf[1] = (NR_busdata[NR_branchdata[NR_branch_reference].from].V[0] * chrcstore[3] +
												NR_busdata[NR_branchdata[NR_branch_reference].from].V[1] * chrcstore[4] +
												NR_busdata[NR_branchdata[NR_branch_reference].from].V[2] * chrcstore[5]) / complex(2.0,0.0);

							LinkHistTermCf[2] = (NR_busdata[NR_branchdata[NR_branch_reference].from].V[0] * chrcstore[6] +
												NR_busdata[NR_branchdata[NR_branch_reference].from].V[1] * chrcstore[7] +
												NR_busdata[NR_branchdata[NR_branch_reference].from].V[2] * chrcstore[8]) / complex(2.0,0.0);

							//Calculate the updated history terms - hrcf = chrc*vfromprev - hrcfprev
							LinkHistTermCt[0] = (NR_busdata[NR_branchdata[NR_branch_reference].to].V[0] * chrcstore[0] +
												NR_busdata[NR_branchdata[NR_branch_reference].to].V[1] * chrcstore[1] +
												NR_busdata[NR_branchdata[NR_branch_reference].to].V[2] * chrcstore[2]) / complex(2.0,0.0);

							LinkHistTermCt[1] = (NR_busdata[NR_branchdata[NR_branch_reference].to].V[0] * chrcstore[3] +
												NR_busdata[NR_branchdata[NR_branch_reference].to].V[1] * chrcstore[4] +
												NR_busdata[NR_branchdata[NR_branch_reference].to].V[2] * chrcstore[5]) / complex(2.0,0.0);

							LinkHistTermCt[2] = (NR_busdata[NR_branchdata[NR_branch_reference].to].V[0] * chrcstore[6] +
												NR_busdata[NR_branchdata[NR_branch_reference].to].V[1] * chrcstore[7] +
												NR_busdata[NR_branchdata[NR_branch_reference].to].V[2] * chrcstore[8]) / complex(2.0,0.0);
						}//End capacitance update
					}//End first run of in-rush from steady-state

					//See how we're applying the update
					if (use_line_cap == true)	//Only do if capacitance is enabled
					{
						//Compute the values and post them to the appropriate nodes
						NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[0] += LinkHistTermL[0] + LinkHistTermCf[0];
						NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[1] += LinkHistTermL[1] + LinkHistTermCf[1];
						NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[2] += LinkHistTermL[2] + LinkHistTermCf[2];

						NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[0] -= LinkHistTermL[0] - LinkHistTermCt[0];
						NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[1] -= LinkHistTermL[1] - LinkHistTermCt[1];
						NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[2] -= LinkHistTermL[2] - LinkHistTermCt[2];
					}//End capacitance enabled
					else	//Not enabled
					{
						//Compute the values and post them to the appropriate nodes
						NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[0] += LinkHistTermL[0];
						NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[1] += LinkHistTermL[1];
						NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[2] += LinkHistTermL[2];

						NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[0] -= LinkHistTermL[0];
						NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[1] -= LinkHistTermL[1];
						NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[2] -= LinkHistTermL[2];
					}
				}
				//Defaulted else -- not a new time, so don't update it
			}//End "normal line"
			else if (SpecialLnk == WYEWYE)	//Wye-wye transformer	-- shell code, this doesn't work
			{
				//Set flag
				require_inrush_update = true;

				//See if we're a different timestep
				if (curr_delta_time != deltamode_prev_time)
				{
					/************************* FIGURE OUT A BETTER WAY TO DO THIS -- DOING NOW FOR DEBUGGING *******************/
					if (deltamode_prev_time < 0)
					{
						//Default flag - assume we're newly energized (full-in-rush)
						transf_from_stdy_state = false;

						//Check phases - see if we have voltages - Phase A
						if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)
						{
							if ((NR_busdata[NR_branchdata[NR_branch_reference].from].V[0].Mag() > 0.0) && (NR_busdata[NR_branchdata[NR_branch_reference].to].V[0].Mag() > 0.0))
							{
								transf_from_stdy_state=true;
							}
							//Default else -- keep it false
						}
						//No phase A

						//Check phases - see if we have voltages - Phase B
						if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)
						{
							if ((NR_busdata[NR_branchdata[NR_branch_reference].from].V[1].Mag() > 0.0) && (NR_busdata[NR_branchdata[NR_branch_reference].to].V[1].Mag() > 0.0))
							{
								transf_from_stdy_state=true;
							}
							//Default else -- keep it false
						}
						//No phase B

						//Check phases - see if we have voltages - Phase C
						if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)
						{
							if ((NR_busdata[NR_branchdata[NR_branch_reference].from].V[2].Mag() > 0.0) && (NR_busdata[NR_branchdata[NR_branch_reference].to].V[2].Mag() > 0.0))
							{
								transf_from_stdy_state=true;
							}
							//Default else -- keep it false
						}
						//No phase C

						//Null it first, for giggles
						transformer_calc_function = NULL;

						//Find the transformer function
						transformer_calc_function = (FUNCTIONADDR)(gl_get_function(obj,"recalc_transformer_matrices"));

						//See if it worked
						if (transformer_calc_function == NULL)
						{
							GL_THROW("Link:%s - failed to map transformer update function", obj->name ? obj->name : "Unnamed");
							//Below
						}

						//Now call it to populate the matrices -- do it via a function, just because (likely easiest way)
						ret_value = ((int (*)(OBJECT *))(*transformer_calc_function))(obj);

						//Make sure it worked
						if (ret_value != 1)
						{
							GL_THROW("Link:%s - failed update transformer matrices", obj->name ? obj->name : "Unnamed");
							//Below
						}
					}
					else
					{
						//Set the transformer flag for "normal"
						transf_from_stdy_state = false;
					}
					/**************************************************************/

					//Update the tracker
					deltamode_prev_time = curr_delta_time;

					//Copy the old history terms in first - main transformer portion
					LinkHistTermL[6] =  LinkHistTermL[0];
					LinkHistTermL[7] =  LinkHistTermL[1];
					LinkHistTermL[8] =  LinkHistTermL[2];
					LinkHistTermL[9] =  LinkHistTermL[3];
					LinkHistTermL[10] = LinkHistTermL[4];
					LinkHistTermL[11] = LinkHistTermL[5];

					//Copy the old history terms for the primary magnetization - if applicable
					if (LinkHistTermCf != NULL)
					{
						LinkHistTermCf[3] = LinkHistTermCf[0];
						LinkHistTermCf[4] = LinkHistTermCf[1];
						LinkHistTermCf[5] = LinkHistTermCf[2];
					}

					//Copy the old history terms for the secondary magnetization - if applicable
					if (LinkHistTermCt != NULL)
					{
						LinkHistTermCt[3] = LinkHistTermCt[0];
						LinkHistTermCt[4] = LinkHistTermCt[1];
						LinkHistTermCt[5] = LinkHistTermCt[2];
					}

					//Update the phi term too, if appropriate
					if (hphi != NULL)
					{
						hphi[6] = hphi[0];
						hphi[7] = hphi[1];
						hphi[8] = hphi[2];
						hphi[9] = hphi[3];
						hphi[10] = hphi[4];
						hphi[11] = hphi[5];
					}

					//Calculate the updated history terms - htr = ahtr*vprev-bhtr*hrlprev

					//Copy voltage terms into common vector
					work_vector_A[0] = NR_busdata[NR_branchdata[NR_branch_reference].from].V[0];
					work_vector_A[1] = NR_busdata[NR_branchdata[NR_branch_reference].from].V[1];
					work_vector_A[2] = NR_busdata[NR_branchdata[NR_branch_reference].from].V[2];
					work_vector_A[3] = NR_busdata[NR_branchdata[NR_branch_reference].to].V[0];
					work_vector_A[4] = NR_busdata[NR_branchdata[NR_branch_reference].to].V[1];
					work_vector_A[5] = NR_busdata[NR_branchdata[NR_branch_reference].to].V[2];

					if (transf_from_stdy_state == false)
					{
						//Perform the multiply of ahrl*vprev - put into LinkHistTerm first parts
						lmatrix_vmult(ahrlstore, &work_vector_A[0], LinkHistTermL, 6);

						//Perform the multiply of bhrlstore*hprev
						lmatrix_vmult(bhrlstore, &LinkHistTermL[6], &work_vector_C[0], 6);

						//Subtrack to form the new LinkHistTerm
						for (jindex=0; jindex<6; jindex++)
						{
							LinkHistTermL[jindex] -= work_vector_C[jindex];
						}

						//Calculate the updated history terms - htr_pri = ahtrm*vpri_prev-bhtrm*htr_pri_prev - if applicable
						if (LinkHistTermCf != NULL)
						{
							//Perform the multiply of ahtrm*vpri_prev - put into LinkHistTerm first parts
							lmatrix_vmult(ahmstore, &work_vector_A[0], LinkHistTermCf, 3);

							//Perform the multiply of bhhtrmstore*hprev_pri
							lmatrix_vmult(bhmstore, &LinkHistTermCf[3], &work_vector_C[0], 3);

							//Subtrack to form the new LinkHistTerm
							for (jindex=0; jindex<3; jindex++)
							{
								LinkHistTermCf[jindex] -= work_vector_C[jindex];
							}
						}

						//Calculate the updated history terms - htr_sec = ahtrm*vsec_prev-bhtrm*htr_sec_prev - if applicable
						if (LinkHistTermCt != NULL)
						{
							//Perform the multiply of ahtrm*vsec_prev - put into LinkHistTerm first parts
							lmatrix_vmult(&ahmstore[9], &work_vector_A[3], LinkHistTermCt, 3);

							//Perform the multiply of bhhtrmstore*hprev_sec
							lmatrix_vmult(&bhmstore[9], &LinkHistTermCt[3], &work_vector_C[0], 3);

							//Subtrack to form the new LinkHistTerm
							for (jindex=0; jindex<3; jindex++)
							{
								LinkHistTermCt[jindex] -= work_vector_C[jindex];
							}
						}

						//Update phi history term, if appropriate
						if (hphi != NULL)
						{
							//Compute the "constant" first -- save some multiplies
							temp_value_A = complex(1.0,0.0) + B_phi;
							temp_value_A = temp_value_A * A_phi;

							//hphi = A_phi*I(8)*Voltage_vals + B_phi*I(8)*phi_prev;
							for (jindex=0; jindex<6; jindex++)
							{
								hphi[jindex] = work_vector_A[jindex] * temp_value_A;
								hphi[jindex] += hphi[jindex+6] * B_phi;
							}
						}
					}//End normal update
					else	//Flag was true, so this transformer isn't cold-starting
					{
						//Updated htrafo for "initialization" - htrafo = inv(I+bhtrstor)*ahtrstore*V
						//Form up the "I+bhtrstore" portion
						for (jindex=0; jindex<6; jindex++)
						{
							for (kindex=0; kindex<6; kindex++)
							{
								if (jindex==kindex)
								{
									work_matrix_A[jindex][kindex] = complex(1.0,0.0) + bhrlstore[jindex*6+kindex];
								}
								else
								{
									work_matrix_A[jindex][kindex] = bhrlstore[jindex*6+kindex];
								}
							}
						}

						//Invert it
						lu_matrix_inverse(&work_matrix_A[0][0],&work_matrix_B[0][0],6);

						//Multiply it by the ahstore values
						lmatrix_mult(&work_matrix_B[0][0],ahrlstore,&work_matrix_C[0][0],6);

						//Multiply by the voltage and put it into the LinkHist term
						lmatrix_vmult(&work_matrix_C[0][0],&work_vector_A[0],LinkHistTermL,6);

						//Do the "from/primary" multiply, if relevant
						//htr_pri = inv(I + bhtr)*ahtr*vpri
						if (LinkHistTermCf != NULL)
						{
							//Form up the initial "I+bhtr" portion
							for (jindex=0; jindex<3; jindex++)
							{
								for (kindex=0; kindex<3; kindex++)
								{
									if (jindex==kindex)	//Diagonal, add I
									{
										work_matrix_D[jindex][kindex] = complex(1.0,0.0) + bhmstore[jindex*3+kindex];
									}
									else	//Normal "copy"
									{
										work_matrix_D[jindex][kindex] = bhmstore[jindex*3+kindex];
									}
								}
							}

							//Invert it
							lu_matrix_inverse(&work_matrix_D[0][0],&work_matrix_E[0][0],3);

							//Multply it by the ahmstore values
							lmatrix_mult(&work_matrix_E[0][0],ahmstore,&work_matrix_F[0][0],3);

							//Multiply it by the voltage and store it
							lmatrix_vmult(&work_matrix_F[0][0],&work_vector_A[0], LinkHistTermCf, 3);
						}//End from/primary magnetization items

						//Do the "to/secondary" multiply, if relevant
						//htr_sec = inv(I + bhtr)*ahtr*vsec
						if (LinkHistTermCt != NULL)
						{
							//Form up the initial "I+bhtr" portion
							for (jindex=0; jindex<3; jindex++)
							{
								for (kindex=0; kindex<3; kindex++)
								{
									if (jindex==kindex)	//Diagonal, add I
									{
										work_matrix_D[jindex][kindex] = complex(1.0,0.0) + bhmstore[jindex*3+kindex+9];
									}
									else	//Normal "copy"
									{
										work_matrix_D[jindex][kindex] = bhmstore[jindex*3+kindex+9];
									}
								}
							}

							//Invert it
							lu_matrix_inverse(&work_matrix_D[0][0],&work_matrix_E[0][0],3);

							//Multply it by the ahmstore values
							lmatrix_mult(&work_matrix_E[0][0],&ahmstore[9],&work_matrix_F[0][0],3);

							//Multiply it by the voltage and store it
							lmatrix_vmult(&work_matrix_F[0][0],&work_vector_A[3], LinkHistTermCt, 3);
						}//End to/secondary magnetization items

						//Update phi history term, if appropriate
						//hphi = A_phi*(1+B_phi)/(1-B_phi)*Voltage_vals
						if (hphi != NULL)
						{
							//Compute the "constant" first -- save some multiplies
							temp_value_A = complex(1.0,0.0) + B_phi;
							temp_value_B = complex(1.0,0.0) - B_phi;
							temp_value_A = temp_value_A / temp_value_B * A_phi;

							//hphi = A_phi*(1+B_phi)/(1-B_phi)*I(8)*Voltage_vals;
							for (jindex=0; jindex<6; jindex++)
							{
								hphi[jindex] = work_vector_A[jindex] * temp_value_A;
							}
						}//End hphi update
					}//End transformer was flagged as coming from being active

					//Compute the values and post them to the appropriate nodes
					NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[0] += LinkHistTermL[0];
					NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[1] += LinkHistTermL[1];
					NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[2] += LinkHistTermL[2];

					NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[0] += LinkHistTermL[3];
					NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[1] += LinkHistTermL[4];
					NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[2] += LinkHistTermL[5];

					//Now apply any appropriate magentization values -- depends on how they were set up (if present)
					if (LinkHistTermCf != NULL)	//Primary needs accumulated
					{
						NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[0] += LinkHistTermCf[0];
						NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[1] += LinkHistTermCf[1];
						NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[2] += LinkHistTermCf[2];
					}
					//Default else -- no primary

					if (LinkHistTermCt != NULL)	//Secondary needs accumulated
					{
						NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[0] += LinkHistTermCt[0];
						NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[1] += LinkHistTermCt[1];
						NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[2] += LinkHistTermCt[2];
					}
				}
				//Defaulted else -- not a new time, so don't update it
			}//End Wye-Wye transformer
			else	//Something unsupported
			{
				require_inrush_update = false;	//deflag
			}
		}//End deltamode active
		else	//in-rush capable, but not enabled
		{
			//Deflag us
			require_inrush_update = false;

			//Set the tracking time variable, just in case we've popped in and out
			deltamode_prev_time = -1.0;

			if (SpecialLnk == NORMAL)
			{
				//Zero the various working matrices, on priniciple
				/********** TODO - Like loads, this may be something that postupdate can do ********************/
				LinkHistTermL[0] = complex(0.0,0.0);
				LinkHistTermL[1] = complex(0.0,0.0);
				LinkHistTermL[2] = complex(0.0,0.0);
				LinkHistTermL[3] = complex(0.0,0.0);
				LinkHistTermL[4] = complex(0.0,0.0);
				LinkHistTermL[5] = complex(0.0,0.0);

				//Zero capacitance, if they exist
				if (use_line_cap == true)
				{
					LinkHistTermCf[0] = complex(0.0,0.0);
					LinkHistTermCf[1] = complex(0.0,0.0);
					LinkHistTermCf[2] = complex(0.0,0.0);
					LinkHistTermCf[3] = complex(0.0,0.0);
					LinkHistTermCf[4] = complex(0.0,0.0);
					LinkHistTermCf[5] = complex(0.0,0.0);

					LinkHistTermCt[0] = complex(0.0,0.0);
					LinkHistTermCt[1] = complex(0.0,0.0);
					LinkHistTermCt[2] = complex(0.0,0.0);
					LinkHistTermCt[3] = complex(0.0,0.0);
					LinkHistTermCt[4] = complex(0.0,0.0);
					LinkHistTermCt[5] = complex(0.0,0.0);
				}
				//Default else -- no capacitance
			}
			else if (SpecialLnk == WYEWYE)
			{
				//Zero the various working matrices, on priniciple
				/********** TODO - Like loads, this may be something that postupdate can do ********************/
				LinkHistTermL[0] = complex(0.0,0.0);
				LinkHistTermL[1] = complex(0.0,0.0);
				LinkHistTermL[2] = complex(0.0,0.0);
				LinkHistTermL[3] = complex(0.0,0.0);
				LinkHistTermL[4] = complex(0.0,0.0);
				LinkHistTermL[5] = complex(0.0,0.0);
				LinkHistTermL[6] = complex(0.0,0.0);
				LinkHistTermL[7] = complex(0.0,0.0);
				LinkHistTermL[8] = complex(0.0,0.0);
				LinkHistTermL[9] = complex(0.0,0.0);
				LinkHistTermL[10] = complex(0.0,0.0);
				LinkHistTermL[11] = complex(0.0,0.0);

				//Zero primary and secondary magnetization history terms
				if (LinkHistTermCf != NULL)
				{
					LinkHistTermCf[0] = complex(0.0,0.0);
					LinkHistTermCf[1] = complex(0.0,0.0);
					LinkHistTermCf[2] = complex(0.0,0.0);
					LinkHistTermCf[3] = complex(0.0,0.0);
					LinkHistTermCf[4] = complex(0.0,0.0);
					LinkHistTermCf[5] = complex(0.0,0.0);
				}

				if (LinkHistTermCt != NULL)
				{
					LinkHistTermCt[0] = complex(0.0,0.0);
					LinkHistTermCt[1] = complex(0.0,0.0);
					LinkHistTermCt[2] = complex(0.0,0.0);
					LinkHistTermCt[3] = complex(0.0,0.0);
					LinkHistTermCt[4] = complex(0.0,0.0);
					LinkHistTermCt[5] = complex(0.0,0.0);
				}

				//Phi as well
				if (hphi != NULL)
				{
					for (jindex=0; jindex<12; jindex++)
					{
						hphi[jindex] = complex(0.0,0.0);
					}
				}
			}
			//Default else -- shouldn't ever get here anyways, but just zero stuffs

			//Compute the values and post them to the appropriate nodes
			NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[0] = complex(0.0,0.0);
			NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[1] = complex(0.0,0.0);
			NR_busdata[NR_branchdata[NR_branch_reference].from].BusHistTerm[2] = complex(0.0,0.0);

			NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[0] = complex(0.0,0.0);
			NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[1] = complex(0.0,0.0);
			NR_busdata[NR_branchdata[NR_branch_reference].to].BusHistTerm[2] = complex(0.0,0.0);

			//Clear the saturation terms too, if they exist - may get duplicated, but meh
			if (NR_busdata[NR_branchdata[NR_branch_reference].from].BusSatTerm != NULL)	//From side
			{
				NR_busdata[NR_branchdata[NR_branch_reference].from].BusSatTerm[0] = complex(0.0,0.0);
				NR_busdata[NR_branchdata[NR_branch_reference].from].BusSatTerm[1] = complex(0.0,0.0);
				NR_busdata[NR_branchdata[NR_branch_reference].from].BusSatTerm[2] = complex(0.0,0.0);
			}

			if (NR_busdata[NR_branchdata[NR_branch_reference].to].BusSatTerm != NULL)	//To side
			{
				NR_busdata[NR_branchdata[NR_branch_reference].to].BusSatTerm[0] = complex(0.0,0.0);
				NR_busdata[NR_branchdata[NR_branch_reference].to].BusSatTerm[1] = complex(0.0,0.0);
				NR_busdata[NR_branchdata[NR_branch_reference].to].BusSatTerm[2] = complex(0.0,0.0);
			}
		}//End deltamode not active
	}//End inrush enabled
	else	//Not in in-rush or deltamode - flag to not
	{
		require_inrush_update = false;
	}

	current_accumulated = false;	//Reset the flag

	if ((status != prev_status) || (force_link_update == true))	//Something's changed, update us
	{
		complex Ylinecharge[3][3];
		complex Y[3][3];
		complex Yc[3][3];
		complex Ylefttemp[3][3];
		complex Yto[3][3];
		complex Yfrom[3][3];
		double invratio, workingvalue;

		//Create initial admittance matrix - use code from GS below - store in From_Y (for now)
		for (jindex=0; jindex<3; jindex++)
			for (kindex=0; kindex<3; kindex++)
				Y[jindex][kindex] = 0.0;

		// compute admittance - invert b matrix - special circumstances given different methods
		if ((SpecialLnk!=NORMAL) && (SpecialLnk!=SPLITPHASE))
		{
			;	//Just skip over all of this nonsense
		}
		else if (has_phase(PHASE_S)) //Triplexy
		{
			//Find the determinant
			complex detvalue = b_mat[0][0]*b_mat[1][1] - b_mat[0][1]*b_mat[1][0];

			//Store the value/compute the inversion
			Y[0][0] = b_mat[1][1] / detvalue;
			Y[0][1] = b_mat[0][1] * -1.0 / detvalue;
			Y[1][0] = b_mat[1][0] * -1.0 / detvalue;
			Y[1][1] = b_mat[0][0] / detvalue;
		}
		else if ((SpecialLnk==NORMAL) && (require_inrush_update == true))
		{
			;	//Skip us as well -- impedance matrix is manipulated, so no sense inverting it twice
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

		if (SpecialLnk!=NORMAL)	//Handle transformers and "special" devices slightly different
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
				//Check to see if in-rush is enabled and WYEWYE
				if ((enable_inrush_calculations == true) && (SpecialLnk == WYEWYE))
				{
					//Null it first, for giggles
					transformer_calc_function = NULL;

					//Find the transformer function
					transformer_calc_function = (FUNCTIONADDR)(gl_get_function(obj,"recalc_transformer_matrices"));

					//See if it worked
					if (transformer_calc_function == NULL)
					{
						GL_THROW("Link:%s - failed to map transformer update function", obj->name ? obj->name : "Unnamed");
						/*  TROUBLESHOOT
						While attempting to find the function to update a transformer's inrush matrices, an error was encountered.
						Please try again.  If the error persists, please submit your code and a bug report via the ticketing system.
						*/
					}

					//Now call it to populate the matrices -- do it via a function, just because (likely easiest way)
					ret_value = ((int (*)(OBJECT *))(*transformer_calc_function))(obj);

					//Make sure it worked
					if (ret_value != 1)
					{
						GL_THROW("Link:%s - failed update transformer matrices", obj->name ? obj->name : "Unnamed");
						/*  TROUBLESHOOT
						While attempting to perform the update for the transformer in-rush matrices, and error was encountered.
						Please try again and ensure in-rush computations are enabled.  If the error persists, please submit your code
						and a bug report via the ticketing system.
						*/
					}
				}//End WYEWYE transformer inrush update
				else	//No in-rush or not WYE-WYE, just go like normal
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
				}//End non-inrush transformer
			}//End other transformers
		}
		else 				//Simple lines
		{
			//Compute the inductance portions for inrush
			if ((enable_inrush_calculations == true) && (require_inrush_update==true))
			{
				//Zero working matrices - reuse existing, just to annoy later coders
				for (jindex=0; jindex<3; jindex++)
				{
					for (kindex=0; kindex<3; kindex++)
					{
						Ylefttemp[jindex][kindex] = complex(0.0,0.0);
						Yfrom[jindex][kindex] = complex(0.0,0.0);
						Yto[jindex][kindex] = complex(0.0,0.0);
					}
				}

				//If capacitance is enabled, copy the original b_mat before we break it
				if (use_line_cap == true)
				{
					equalm(b_mat,Yc);	//Copy to Yc temporarily
				}

				//Form intermediate inductance values and constants
				for (jindex=0; jindex<3; jindex++)
				{
					for (kindex=0; kindex<3; kindex++)
					{
						//Extract the imaginary part (should be only part) and de-phasor it - Yshunt/(2*pi*f)*2/dt
						workingvalue = b_mat[jindex][kindex].Im() / (PI * current_frequency * deltatimestep_running);

						//Put into the other working matrix (zh)
						Ylefttemp[jindex][kindex] = b_mat[jindex][kindex] - complex(workingvalue,0.0);

						//Put back into the impedance matrix
						b_mat[jindex][kindex] += complex(workingvalue,0.0);
					}
				}

				//Inversion sequence to get Y (admittance)
				if (has_phase(PHASE_A) && !has_phase(PHASE_B) && !has_phase(PHASE_C)) //only A
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

				//Form the bhrl term - Y*Zh = bhrl
				multiply(Y,Ylefttemp,Yfrom);

				//Compute the ahrl term - Y(Zh*Y - I)
				multiply(Ylefttemp,Y,Yto);	//Zh*Y
				Yto[0][0]-=1.0;	// -I
				Yto[1][1]-=1.0;
				Yto[2][2]-=1.0;
				multiply(Y,Yto,Ylefttemp);	//Y(Zh*Y-I)

				//Loop and store them - translate due to mallocing to be safe
				for (jindex=0; jindex<3; jindex++)
				{
					for (kindex=0; kindex<3; kindex++)
					{
						//ahrl
						ahrlstore[jindex*3+kindex] = Ylefttemp[jindex][kindex];

						//bhrl
						bhrlstore[jindex*3+kindex] = Yfrom[jindex][kindex];
					}
				}

				//Restore b_mat for capacitance
				if (use_line_cap == true)
				{
					equalm(Yc,b_mat);
				}
			}//End in-rush-enabled calculation updates -- impedance/inductance portion

			if (use_line_cap == true)	//Capacitance included
			{
				if (has_phase(PHASE_S))	//Triplex exclusion
				{
					//Just copy the output directly in
					equalm(Y,From_Y);

					//Do the same for the "shunt" term, even though it is the same here
					for (jindex=0; jindex<3; jindex++)
					{
						for (kindex=0; kindex<3; kindex++)
						{
							YSfrom[jindex*3+kindex]=Y[jindex][kindex];
						}
					}
				}//End triplex capacitance code
				else	//Other lines
				{
					//Make "transfer matrices" admittance-less
					equalm(Y,From_Y);

					//Zero out Y first, just in case - mainly if something above used it
					for (jindex=0; jindex<3; jindex++)
					{
						for (kindex=0; kindex<3; kindex++)
						{
							Y[jindex][kindex]=complex(0.0,0.0);
						}
					}

					//Replicate the inversion, again, but with the "less modified" version - Stored from Yc=b_mat earlier
					//Inversion sequence to get Y (admittance)
					if (has_phase(PHASE_A) && !has_phase(PHASE_B) && !has_phase(PHASE_C)) //only A
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

					//Compute total self admittance - include line charging capacitance
					//Basically undo a_mat = I + 1/2 Zabc*Yabc
					equalm(a_mat,Ylinecharge);
					Ylinecharge[0][0]-=1;
					Ylinecharge[1][1]-=1;
					Ylinecharge[2][2]-=1;
					multiply(2.0,Ylinecharge,Ylefttemp);
					multiply(Y,Ylefttemp,Ylinecharge);

					//Split back in half for application to each side
					multiply(0.5,Ylinecharge,Ylefttemp);

					//See how this is being handled
					if ((enable_inrush_calculations == true) && (require_inrush_update == true))
					{
						//Update constant terms - shunt is the same for capacitance
						for (jindex=0; jindex<3; jindex++)
						{
							for (kindex=0; kindex<3; kindex++)
							{
								//Extract the imaginary part (should be only part) and de-phasor it - Yshunt/(2*pi*f)*2/dt
								workingvalue = Ylefttemp[jindex][kindex].Im() / (PI * current_frequency * deltatimestep_running);

								//Put into the "shunt" matrix
								Ylefttemp[jindex][kindex] += complex(workingvalue,0.0);

								//Copy this value into the final storage matrix too
								LinkCapShuntTerm[jindex*3+kindex] = Ylefttemp[jindex][kindex];

								//Create chrcstore while we're in here
								chrcstore[jindex*3+kindex] = 2.0 * workingvalue;
							}
						}
					}
					//Defaulted else -- In-rush not enabled, just continue like normal

					//Combine back into the full shunt term
					addition(Ylefttemp,From_Y,Yc);

					//Now parse into the new storage structure (manual to ensure things are placed right)
					for (jindex=0; jindex<3; jindex++)
					{
						for (kindex=0; kindex<3; kindex++)
						{
							YSfrom[jindex*3+kindex]=Yc[jindex][kindex];
						}
					}
				}//End non-triplex lines
			}
			else	//Normal execution
			{
				//Just post the admittance straight in - line charging doesn't exist anyways
				equalm(Y,From_Y);
			}
		}
		
		//Update status variable
		prev_status = status;
	}
}

//Presync call
TIMESTAMP link_object::presync(TIMESTAMP t0)
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
			int resultval;
			bool *temp_empty;
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
				if (SpecialLnk!=NORMAL)	//Transformer, send more - may not need all 4, but put them there anyways
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
					//See if capacitance is enabled, map a secondary matrix
					if (use_line_cap == true)
					{
						//Allocate a matrix to store the secondary information (could use To_Y, but the may be confusing)
						YSfrom = (complex *)gl_malloc(9*sizeof(complex));
						if (YSfrom == NULL)
						{
							GL_THROW("NR: Memory allocation failure for line matrix.");
							/*  TROUBLESHOOT
							This is a bug.  Newton-Raphson tries to allocate memory for an additional
							needed matrix when dealing with lines that have capacitance included.
							This failed.  Please submit your code and a bug report on the trac site.
							*/
						}

						//Link up appropriately
						NR_branchdata[NR_branch_reference].Yfrom = &From_Y[0][0];
						NR_branchdata[NR_branch_reference].Yto = &From_Y[0][0];
						NR_branchdata[NR_branch_reference].YSfrom = YSfrom;
						NR_branchdata[NR_branch_reference].YSto = YSfrom;

					}
					else	//No capacitance - proceed like normal
					{
						NR_branchdata[NR_branch_reference].Yfrom = &From_Y[0][0];
						NR_branchdata[NR_branch_reference].Yto = &From_Y[0][0];
						NR_branchdata[NR_branch_reference].YSfrom = &From_Y[0][0];
						NR_branchdata[NR_branch_reference].YSto = &From_Y[0][0];
					}
				}

				//Link the name
				NR_branchdata[NR_branch_reference].name = obj->name;

				//Link to ourselves
				NR_branchdata[NR_branch_reference].obj = obj;

				//Link to our limit checking function -- seems a little silly to do it this way, but allows external calls
				NR_branchdata[NR_branch_reference].limit_check = (FUNCTIONADDR)(gl_get_function(obj,"check_limits_pwr_object"));

				//Make sure it worked, for now
				if (NR_branchdata[NR_branch_reference].limit_check == NULL)
				{
					GL_THROW("Unable to map limit checking function for link:%s",obj->name ? obj->name : "Unnamed");
					/*  TROUBLESHOOT
					While attempting to map the power/current checking function for a link object, the mapping failed.
					Please try again.  If the error persists, please submit your code and a bug report via the ticketing
					system.
					*/
				}

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

			//Link up the deltamode functions
			//Populate individual object references into deltamode, if needed
			if ((deltamode_inclusive==true) && (enable_subsecond_models == true))
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
					the object supports deltamode.  The object simply may not support delta mode.  If the error persists,
					please submit your code and a bug report via the
					trac website.
					*/
				}

				//Null out the frequency function - links don't do anything with it anyways
				delta_freq_functions[temp_pwr_object_current] = NULL;

				//See if we're an appropriate transformer and in-rush enabled
				if ((enable_inrush_calculations == true) && (gl_object_isa(obj,"transformer","powerflow")))
				{
					//Map the function for the transformer
					NR_branchdata[NR_branch_reference].ExtraDeltaModeFunc = (FUNCTIONADDR)(gl_get_function(obj,"recalc_deltamode_saturation"));

					//Make sure it worked
					if (NR_branchdata[NR_branch_reference].ExtraDeltaModeFunc == NULL)
					{
						gl_warning("Failure to map deltamode saturation function for device:%s",obj->name ? obj->name : "Unnamed");
						/*  TROUBLESHOOT
						Attempts to map up the saturation function of a specific device (transformer) failed.
						Please try again.  If the error persists, please submit your code and a bug report
						via the ticking website.
						*/
					}

					//Set "flag"
					temp_empty = NULL;

					//Call the function once, right now
					resultval = ((int (*)(OBJECT *,bool *))(*NR_branchdata[NR_branch_reference].ExtraDeltaModeFunc))(obj,temp_empty);

					//Check the results 
					if (resultval == -1)	//Not a supported transformer type
					{
						//Null the function to prevent it from being called
						NR_branchdata[NR_branch_reference].ExtraDeltaModeFunc = NULL;
					}
					else if (resultval > 0)	//Successful init and saturation needed - allocate some vectors
					{
						//1 = primary, 2 = secondary, 3 = both
						if ((resultval == 1) || (resultval == 3))	//Primary winding needed -- allocate node value
						{
							//See if it has already been allocated
							if (NR_busdata[NR_branchdata[NR_branch_reference].from].BusSatTerm == NULL)
							{
								//Allocate three spots -- always assume three for now
								NR_busdata[NR_branchdata[NR_branch_reference].from].BusSatTerm = (complex *)gl_malloc(3*sizeof(complex));

								//Check it
								if (NR_busdata[NR_branchdata[NR_branch_reference].from].BusSatTerm == NULL)
								{
									GL_THROW("Failed to allocate saturation current vector in %s",from->name ? from->name : "Unnamed");
									/*  TROUBLESHOOT
									Failed to allocate the saturation current "holding" matrix for a node.  Please try again.
									If the error persists, please submit your code and a bug report via the ticketing website.
									*/
								}
								else	//Must have worked -- zero them
								{
									NR_busdata[NR_branchdata[NR_branch_reference].from].BusSatTerm[0] = complex(0.0,0.0);
									NR_busdata[NR_branchdata[NR_branch_reference].from].BusSatTerm[1] = complex(0.0,0.0);
									NR_busdata[NR_branchdata[NR_branch_reference].from].BusSatTerm[2] = complex(0.0,0.0);
								}
							}//End allocation routine
							//Default else -- assume someone else go to it
						}//End primary side needed
						
						if ((resultval == 2) || (resultval == 3))	//Secondary winding needed, allocate
						{
							//See if it has already been allocated
							if (NR_busdata[NR_branchdata[NR_branch_reference].to].BusSatTerm == NULL)
							{
								//Allocate three spots -- always assume three for now
								NR_busdata[NR_branchdata[NR_branch_reference].to].BusSatTerm = (complex *)gl_malloc(3*sizeof(complex));

								//Check it
								if (NR_busdata[NR_branchdata[NR_branch_reference].to].BusSatTerm == NULL)
								{
									GL_THROW("Failed to allocate saturation current vector in %s",to->name ? to->name : "Unnamed");
									//Defined above
								}
								else	//Must have worked -- zero them
								{
									NR_busdata[NR_branchdata[NR_branch_reference].to].BusSatTerm[0] = complex(0.0,0.0);
									NR_busdata[NR_branchdata[NR_branch_reference].to].BusSatTerm[1] = complex(0.0,0.0);
									NR_busdata[NR_branchdata[NR_branch_reference].to].BusSatTerm[2] = complex(0.0,0.0);
								}
							}//End allocation routine
							//Default else -- assume someone else go to it
						}//End secondary side needed
					}
					else	//Somehow got a failure - catch it, even though I don't know how we get here
					{
						GL_THROW("Error while performing first execution of transformer saturation map in %s",obj->name ? obj->name : "Unnamed");
						/*  TROUBLESHOOT
						While attempting to do a first execution of the transformer saturation function, something
						unexpected occurred.  Please try again.  If the error persists, please submit your code
						and a bug report via the ticketing website.
						*/
					}
				}//End in-rush and "special"
				else	//Nope, just null it
				{
					NR_branchdata[NR_branch_reference].ExtraDeltaModeFunc = NULL;
				}
			}//End deltamode populations
			else	//Not deltamode - null out things not used
			{
				NR_branchdata[NR_branch_reference].ExtraDeltaModeFunc = NULL;
			}
		}//End init loop

		//Call the presync items that are common to deltamode implementations
		NR_link_presync_fxn();

		//Update time variable if necessary
		if (prev_LTime != t0)
		{
			//Check to see if a SC has occured and zero out the fault current - putting in here so reiterations don't break it
			if(If_in[0] != 0 || If_in[1] != 0 || If_in[2] != 0){
				If_in[0] = 0;
				If_in[1] = 0;
				If_in[2] = 0;
				If_out[0] = 0;
				If_out[1] = 0;
				If_out[2] = 0;
			}

			prev_LTime=t0;
		}
	}//End NR Solver

	return t1;
}

TIMESTAMP link_object::sync(TIMESTAMP t0)
{
#ifdef SUPPORT_OUTAGES
	node *fNode;
	node *tNode;
	fNode=OBJECTDATA(from,node);
	tNode=OBJECTDATA(to,node);
#endif
	OBJECT *obj = OBJECTHDR(this);

	if (is_closed())
	{
		if (solver_method==SM_FBS)
		{
			node *f;
			node *t;
			set reverse = get_flow(&f,&t);

#ifdef SUPPORT_OUTAGES
			tNode->condition=fNode->condition;
#endif
			/* compute currents */
			READLOCK_OBJECT(to);
			complex tc[] = {t->current_inj[0], t->current_inj[1], t->current_inj[2]};
			UNLOCK_OBJECT(to);

			complex i0, i1, i2;

			current_in[0] = i0 = 
				c_mat[0][0] * t->voltage[0] +
				c_mat[0][1] * t->voltage[1] +
				c_mat[0][2] * t->voltage[2] +
				d_mat[0][0] * tc[0] +
				d_mat[0][1] * tc[1] +
				d_mat[0][2] * tc[2];
			current_in[1] = i1 = 
				c_mat[1][0] * t->voltage[0] +
				c_mat[1][1] * t->voltage[1] +
				c_mat[1][2] * t->voltage[2] +
				d_mat[1][0] * tc[0] +
				d_mat[1][1] * tc[1] +
				d_mat[1][2] * tc[2];
			current_in[2] = i2 = 
				c_mat[2][0] * t->voltage[0] +
				c_mat[2][1] * t->voltage[1] +
				c_mat[2][2] * t->voltage[2] +
				d_mat[2][0] * tc[0] +
				d_mat[2][1] * tc[1] +
				d_mat[2][2] * tc[2];

			WRITELOCK_OBJECT(from);
			f->current_inj[0] += i0;
			f->current_inj[1] += i1;
			f->current_inj[2] += i2;
			WRITEUNLOCK_OBJECT(from);
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

//Functionalized postsync events -- made so deltamode can call
void link_object::BOTH_link_postsync_fxn(void)
{
	double temp_power_check;
	bool over_limit;

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
	if (has_phase(PHASE_S))
		calculate_power_splitphase();
	else
		calculate_power();

	//Perform limit check
	perform_limit_checks(&temp_power_check, &over_limit);
}

//Functionalized limit checking, mostly for restoration calls
void link_object::perform_limit_checks(double *over_limit_value, bool *over_limits)
{
	double temp_power_check, temp_current_diff;
	node *nTo;

	//Default values
	*over_limit_value = 0.0;
	*over_limits = false;

	//Set it to zero
	temp_power_check = 0.0;

	//Check to see if limits need to be checked
	if ((use_link_limits==true) && (check_link_limits==true))
	{
		//See what we are - if we're a transformer, we're looking at power
		if ((SpecialLnk != NORMAL) && (SpecialLnk != SWITCH) && (SpecialLnk != REGULATOR))
		{
			//Check power - rating is in kVA - just use power_out (tends to be a little more accurate
			temp_power_check = power_out.Mag() / 1000.0;

			if (temp_power_check > *link_limits[0][0])
			{
				//Exceeded rating - no emergency ratings for transformers, at this time
				gl_warning("transformer:%s is at %.2f%% of its rated power value",OBJECTHDR(this)->name,(temp_power_check/(*link_limits[0][0])*100.0));
				/*  TROUBLESHOOT
				The total power passing through a transformer is above its kVA rating.
				*/

				//Add in the value
				*over_limit_value = (temp_power_check - (power_out.Mag()/1000.0))*1000.0;

				//Flag as over
				*over_limits = true;
			}
		}//End transformers
		else	//Must be a line - that's the only other option right now
		{
			//Make sure it isn't triplex first
			if (has_phase(PHASE_S))
			{
				if (read_I_out[0].Mag() > *link_limits[0][0])
				{
					//Exceeded continuous - check emergency
					if (read_I_out[0].Mag() > *link_limits[1][0])
					{
						//Exceeded emergency
						gl_warning("Line:%s is at %.2f%% of its emergency rating on phase 1!",OBJECTHDR(this)->name,(read_I_out[0].Mag()/(*link_limits[1][0])*100.0));
						/*  TROUBLESHOOT
						Phase 1 on the line has exceeded the emergency rating associated with it.
						*/
					}
					else	//Just continuous exceed
					{
						gl_warning("Line:%s is at %.2f%% of its continuous rating on phase 1!",OBJECTHDR(this)->name,(read_I_out[0].Mag()/(*link_limits[0][0])*100.0));
						/*  TROUBLESHOOT
						Phase 1 on the line has exceeded the continuous rating associated with it.
						*/
					}

					//Get this as a power value, for accumulation
					nTo = OBJECTDATA(to,node);

					//Find "overcurrent"
					temp_current_diff = read_I_out[0].Mag() - *link_limits[0][0];

					//Compute the power - crude, but meh
					temp_power_check = (nTo->voltage[0].Mag())*temp_current_diff;

					//Accumulate it
					*over_limit_value += temp_power_check;

					//Flag as over
					*over_limits = true;

				}//End Phase 1 check

				if (read_I_out[1].Mag() > *link_limits[0][1])
				{
					//Exceeded continuous - check emergency
					if (read_I_out[1].Mag() > *link_limits[1][1])
					{
						//Exceeded emergency
						gl_warning("Line:%s is at %.2f%% of its emergency rating on phase 2!",OBJECTHDR(this)->name,(read_I_out[1].Mag()/(*link_limits[1][1])*100.0));
						/*  TROUBLESHOOT
						Phase 1 on the line has exceeded the emergency rating associated with it.
						*/
					}
					else	//Just continuous exceed
					{
						gl_warning("Line:%s is at %.2f%% of its continuous rating on phase 2!",OBJECTHDR(this)->name,(read_I_out[1].Mag()/(*link_limits[0][1])*100.0));
						/*  TROUBLESHOOT
						Phase 1 on the line has exceeded the continuous rating associated with it.
						*/
					}

					//Get this as a power value, for accumulation
					nTo = OBJECTDATA(to,node);

					//Find "overcurrent"
					temp_current_diff = read_I_out[1].Mag() - *link_limits[0][1];

					//Compute the power - crude, but meh
					temp_power_check = (nTo->voltage[1].Mag())*temp_current_diff;

					//Accumulate it
					*over_limit_value += temp_power_check;

					//Flag as over
					*over_limits = true;

				}//End Phase 2 check
			}//End triplex line check
			else	//"Normal" line
			{
				//Check individual currents (line ratings in Amps)
				if (has_phase(PHASE_A))
				{
					if (read_I_out[0].Mag() > *link_limits[0][0])
					{
						//Exceeded continuous - check emergency
						if (read_I_out[0].Mag() > *link_limits[1][0])
						{
							//Exceeded emergency
							gl_warning("Line:%s is at %.2f%% of its emergency rating on phase A!",OBJECTHDR(this)->name,(read_I_out[0].Mag()/(*link_limits[1][0])*100.0));
							/*  TROUBLESHOOT
							Phase A on the line has exceeded the emergency rating associated with it.
							*/
						}
						else	//Just continuous exceed
						{
							gl_warning("Line:%s is at %.2f%% of its continuous rating on phase A!",OBJECTHDR(this)->name,(read_I_out[0].Mag()/(*link_limits[0][0])*100.0));
							/*  TROUBLESHOOT
							Phase A on the line has exceeded the continuous rating associated with it.
							*/
						}

						//Get this as a power value, for accumulation
						nTo = OBJECTDATA(to,node);

						//Find "overcurrent"
						temp_current_diff = read_I_out[0].Mag() - *link_limits[0][0];

						//Compute the power - crude, but meh
						temp_power_check = (nTo->voltage[0].Mag())*temp_current_diff;

						//Accumulate it
						*over_limit_value += temp_power_check;

						//Flag as over
						*over_limits = true;

					}//End Phase A check
				}//End has Phase A

				if (has_phase(PHASE_B))
				{
					if (read_I_out[1].Mag() > *link_limits[0][1])
					{
						//Exceeded continuous - check emergency
						if (read_I_out[1].Mag() > *link_limits[1][1])
						{
							//Exceeded emergency
							gl_warning("Line:%s is at %.2f%% of its emergency rating on phase B!",OBJECTHDR(this)->name,(read_I_out[1].Mag()/(*link_limits[1][1])*100.0));
							/*  TROUBLESHOOT
							Phase B on the line has exceeded the emergency rating associated with it.
							*/
						}
						else	//Just continuous exceed
						{
							gl_warning("Line:%s is at %.2f%% of its continuous rating on phase B!",OBJECTHDR(this)->name,(read_I_out[1].Mag()/(*link_limits[0][1])*100.0));
							/*  TROUBLESHOOT
							Phase B on the line has exceeded the continuous rating associated with it.
							*/
						}

						//Get this as a power value, for accumulation
						nTo = OBJECTDATA(to,node);

						//Find "overcurrent"
						temp_current_diff = read_I_out[1].Mag() - *link_limits[0][1];

						//Compute the power - crude, but meh
						temp_power_check = (nTo->voltage[1].Mag())*temp_current_diff;

						//Accumulate it
						*over_limit_value += temp_power_check;

						//Flag as over
						*over_limits = true;

					}//End Phase B check
				}//End has Phase B

				if (has_phase(PHASE_C))
				{
					if (read_I_out[2].Mag() > *link_limits[0][2])
					{
						//Exceeded continuous - check emergency
						if (read_I_out[2].Mag() > *link_limits[1][2])
						{
							//Exceeded emergency
							gl_warning("Line:%s is at %.2f%% of its emergency rating on phase C!",OBJECTHDR(this)->name,(read_I_out[2].Mag()/(*link_limits[1][2])*100.0));
							/*  TROUBLESHOOT
							Phase C on the line has exceeded the emergency rating associated with it.
							*/
						}
						else	//Just continuous exceed
						{
							gl_warning("Line:%s is at %.2f%% of its continuous rating on phase C!",OBJECTHDR(this)->name,(read_I_out[2].Mag()/(*link_limits[0][2])*100.0));
							/*  TROUBLESHOOT
							Phase C on the line has exceeded the continuous rating associated with it.
							*/
						}

						//Get this as a power value, for accumulation
						nTo = OBJECTDATA(to,node);

						//Find "overcurrent"
						temp_current_diff = read_I_out[2].Mag() - *link_limits[0][2];

						//Compute the power - crude, but meh
						temp_power_check = (nTo->voltage[2].Mag())*temp_current_diff;

						//Accumulate it
						*over_limit_value += temp_power_check;

						//Flag as over
						*over_limits = true;

					}//End Phase C check
				}//End has Phase C
			}//End "normal line" check
		}//End must be a line check
	}//End Limit checks
}

TIMESTAMP link_object::postsync(TIMESTAMP t0)
{
	TIMESTAMP TRET=TS_NEVER;
	//double temp_power_check;

	if ((solver_method==SM_FBS))
	{
		node *f;
		node *t; //@# make else/if statement for solver method NR; & set current_out->to t->node current_inj;
		set reverse = get_flow(&f,&t);

		// update published current_out values;
		READLOCK_OBJECT(to);
		complex tc[] = {t->current_inj[0], t->current_inj[1], t->current_inj[2]};
		READUNLOCK_OBJECT(to);

		read_I_out[0] = tc[0];
		read_I_out[1] = tc[1];

		if (has_phase(PHASE_S) && (voltage_ratio != 1.0))	//Implies SPCT
			read_I_out[2] = -tc[1] - tc[0];	//Implies ground at TP Node, so I_n is full neutral + ground
		else
			read_I_out[2] = tc[2];
		
		if (!is_open())
		{
			/* compute and update voltages */
			complex v0 = 
				A_mat[0][0] * f->voltage[0] +
				A_mat[0][1] * f->voltage[1] + // 
				A_mat[0][2] * f->voltage[2] - //@todo current inj; flowing from t node
				B_mat[0][0] * tc[0] - // current injection put into link from end mode
				B_mat[0][1] * tc[1] -
				B_mat[0][2] * tc[2];
			complex v1 = 
				A_mat[1][0] * f->voltage[0] +
				A_mat[1][1] * f->voltage[1] +
				A_mat[1][2] * f->voltage[2] -
				B_mat[1][0] * tc[0] -
				B_mat[1][1] * tc[1] -
				B_mat[1][2] * tc[2];
			complex v2 = 
				A_mat[2][0] * f->voltage[0] +
				A_mat[2][1] * f->voltage[1] +
				A_mat[2][2] * f->voltage[2] -
				B_mat[2][0] * tc[0] -
				B_mat[2][1] * tc[1] -
				B_mat[2][2] * tc[2];

			WRITELOCK_OBJECT(to);
			t->voltage[0] = v0;
			t->voltage[1] = v1;
			t->voltage[2] = v2;
			WRITEUNLOCK_OBJECT(to);

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
		/* Zeroing code - TODO: Figure out how to make this work properly
		else //Assumes open here
		{
			//Zero all output voltages - radial assumption
			LOCKED(to,t->voltage[0] = 0.0);
			LOCKED(to,t->voltage[1] = 0.0);
			LOCKED(to,t->voltage[2] = 0.0);

			//Zero output current too, since t->current_inj isn't valid to us no matter what
			read_I_out[0] = 0.0;
			read_I_out[1] = 0.0;
			read_I_out[2] = 0.0;
		}
		*/
#endif

	}//End FBS

	//Call functionalized postsync items
	BOTH_link_postsync_fxn();

	return TRET;
}

int link_object::kmlinit(int (*stream)(const char*,...))
{
	// TODO: move line styles into line objects
	stream("<Style id=\"overhead_line_r\"><LineStyle><color>7f00ffff</color><width>4</width></LineStyle><PolyStyle><color>7fff0000</color></PolyStyle></Style>\n");
	stream("<Style id=\"overhead_line_g\"><LineStyle><color>7f00ffff</color><width>4</width></LineStyle><PolyStyle><color>7f00ff00</color></PolyStyle></Style>\n");
	stream("<Style id=\"overhead_line_b\"><LineStyle><color>7f00ffff</color><width>4</width></LineStyle><PolyStyle><color>7f0000ff</color></PolyStyle></Style>\n");
	stream("<Style id=\"overhead_line_k\"><LineStyle><color>7f00ffff</color><width>4</width></LineStyle><PolyStyle><color>7f000000</color></PolyStyle></Style>\n");
	stream("<Style id=\"underground_line_r\"><LineStyle><color>3f00ffff</color><width>4</width></LineStyle><PolyStyle><color>3fff0000</color></PolyStyle></Style>\n");
	stream("<Style id=\"underground_line_g\"><LineStyle><color>3f00ffff</color><width>4</width></LineStyle><PolyStyle><color>3f00ff00</color></PolyStyle></Style>\n");
	stream("<Style id=\"underground_line_b\"><LineStyle><color>3f00ffff</color><width>4</width></LineStyle><PolyStyle><color>3f0000ff</color></PolyStyle></Style>\n");
	stream("<Style id=\"underground_line_k\"><LineStyle><color>3f00ffff</color><width>4</width></LineStyle><PolyStyle><color>3f000000</color></PolyStyle></Style>\n");

	gld_global host("hostname");
	gld_global port("server_portnum");
#define STYLE(X) stream("<Style id=\"" #X "_g\"><IconStyle><Icon><href>http://%s:%u/rt/" #X "_g.png</href></Icon></IconStyle></Style>\n", (const char*)host.get_string(), port.get_int32());\
		stream("<Style id=\"" #X "_r\"><IconStyle><Icon><href>http://%s:%u/rt/" #X "_r.png</href></Icon></IconStyle></Style>\n", (const char*)host.get_string(), port.get_int32());\
		stream("<Style id=\"" #X "_b\"><IconStyle><Icon><href>http://%s:%u/rt/" #X "_b.png</href></Icon></IconStyle></Style>\n", (const char*)host.get_string(), port.get_int32());\
		stream("<Style id=\"" #X "_k\"><IconStyle><Icon><href>http://%s:%u/rt/" #X "_k.png</href></Icon></IconStyle></Style>\n", (const char*)host.get_string(), port.get_int32());
	STYLE(regulator);
	STYLE(switch);
	STYLE(transformer);
	return 0;
}

int link_object::kmldump(int (*stream)(const char*,...))
{
	OBJECT *obj = OBJECTHDR(this);
	stream("    <Placemark>\n");
	if (obj->name)
		stream("      <name>%s</name>\n", obj->name);
	else
		stream("      <name>%s ==> %s</name>\n", from->name?from->name:"unnamed", to->name?to->name:"unnamed");
	stream("      <description>\n");
	stream("        <![CDATA[\n");
	stream("          <TABLE><TR>\n");
	stream("<CAPTION>%s #%d</CAPTION>\n<TR><TD WIDTH=\"25%\" ALIGN=CENTER>Property<HR></TD>"
			"<TH WIDTH=\"25%\" COLSPAN=2 ALIGN=CENTER><NOBR>Phase A</NOBR><HR></TH>"
			"<TH WIDTH=\"25%\" COLSPAN=2 ALIGN=CENTER><NOBR>Phase B</NOBR><HR></TH>"
			"<TH WIDTH=\"25%\" COLSPAN=2 ALIGN=CENTER><NOBR>Phase C</NOBR><HR></TH></TR>\n", get_oclass()->get_name(), get_id());

	int status = 2; // green
#define HANDLE_EX(X,Y)if ( gl_object_isa(my(),Y) ) status = ((X*)this)->kmldata(stream); else
#define HANDLE(X) HANDLE_EX(X,#X)
	HANDLE_EX(switch_object,"switch")
	HANDLE(regulator)
	HANDLE(triplex_meter)
	HANDLE(meter)
	{
		// values
		node *pFrom = OBJECTDATA(from,node);
		node *pTo = OBJECTDATA(to,node);
		int phase[3] = {has_phase(PHASE_A),has_phase(PHASE_B),has_phase(PHASE_C)};
		complex flow[3];
		complex current[3];
		int i;
		for (i=0; i<3; i++)
		{
			if (phase[i])
			{
				if ( indiv_power_in[i].Mag() > indiv_power_out[i].Mag() )
				{
					flow[i] = indiv_power_out[i]/1000;
					current[i] = current_out[i];
				}
				else
				{
					flow[i] = indiv_power_in[i]/1000;
					current[i] = current_in[i];
				}
			}
		}

		// flow
		stream("<TR><TH ALIGN=LEFT>Flow</TH>");
		for (i=0; i<3; i++)
		{
			if (phase[i])
				stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.1f</NOBR></TD><TD ALIGN=LEFT>kW</TD>",
					flow[i].Re());
			else
				stream("<TD>&nbsp;</TD>");
		}
		stream("</TR>");
		stream("<TR><TH ALIGN=LEFT>&nbsp;</TH>");
		for (i=0; i<3; i++)
		{
			if (phase[i])
				stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.1f</NOBR></TD><TD ALIGN=LEFT>kVAR</TD>",
					flow[i].Im());
			else
				stream("<TD>&nbsp;</TD>");
		}
		stream("</TR>");

		// current
		stream("<TR><TH ALIGN=LEFT>Current</TH>");
		for (i=0; i<3; i++)
		{
			if (phase[i])
				stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.1f</NOBR></TD><TD ALIGN=LEFT>A</TD>\n",
					current[i].Mag());
			else
				stream("<TD>&nbsp;</TD>\n");
		}
		stream("</TR>");

		// loss
		stream("<TR><TH ALIGN=LEFT>Loss</TH>");
		for (i=0; i<3; i++)
		{
			if (phase[i])
				stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.1f</NOBR></TD><TD ALIGN=LEFT>kW</TD>\n",
						indiv_power_loss[i].Re()/1000);
			else
				stream("<TD>&nbsp;</TD>\n");
		}
		stream("</TR>");
		stream("<TR><TH ALIGN=LEFT>&nbsp;</TH>");
		for (i=0; i<3; i++)
		{
			if (phase[i])
				stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.1f</NOBR></TD><TD ALIGN=LEFT>kVAR</TD>\n",
						indiv_power_loss[i].Im()/1000);
			else
				stream("<TD>&nbsp;</TD>\n");
		}
		stream("</TR>");

	}

	stream("</TABLE>\n");
	stream("        ]]>\n");
	stream("      </description>\n");
	if ( fabs(from->latitude-to->latitude)<1e-4 && fabs(from->longitude-to->longitude)<1e-4 )
	{
		stream("<styleUrl>#%s_g</styleUrl>>\n",obj->oclass->name);
		stream("<Point>\n");
		stream("<coordinates>%f,%f</coordinates>\n",get_longitude(),get_latitude());
		stream("</Point>\n");
	}
	else
	{
		stream("      <styleUrl>#%s_g</styleUrl>>\n",obj->oclass->name);
		stream("      <coordinates>%f,%f</coordinates>\n", (from->longitude+to->longitude)/2,(from->latitude+to->latitude)/2);
		stream("      <LineString>\n");
		stream("        <extrude>0</extrude>\n");
		stream("        <tessellate>0</tessellate>\n");
		stream("        <altitudeMode>relative</altitudeMode>\n");
		stream("        <coordinates>%f,%f,50 %f,%f,50</coordinates>\n", // TODO read line height from object library
			from->longitude,from->latitude,to->longitude,to->latitude);
		stream("      </LineString>\n");
	}

	stream("    </Placemark>\n");
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
		*obj = gl_create_object(link_object::oclass);
		if (*obj!=NULL)
		{
			link_object *my = OBJECTDATA(*obj,link_object);
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
		link_object *my = OBJECTDATA(obj,link_object);
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
		link_object *pObj = OBJECTDATA(obj,link_object);
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
	return OBJECTDATA(obj,link_object)->isa(classname);
}

//Deltamode export
EXPORT SIMULATIONMODE interupdate_link(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	link_object *my = OBJECTDATA(obj,link_object);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_link(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_link(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

//Externally callable power update -- for restoration code, primarily
EXPORT int updatepowercalc_link(OBJECT *obj)
{
	link_object *my = OBJECTDATA(obj,link_object);

	//Call the current update -- do it as a "self call"
	my->CurrentCalculation(-1);

	//Now update the power value
	my->calculate_power();

	//We always succeed, because we're just that good.
	return 1;
}

//Externally callable function -- do power update and check against limits
//Basically does the same thing as updatepowercalc_link, but then checks it too
EXPORT int calculate_overlimit_link(OBJECT *obj, double *overload_value, bool *overloaded)
{
	link_object *my = OBJECTDATA(obj,link_object);
	
	//Set some default values - assume we start "not loaded"
	*overload_value = 0.0;
	*overloaded = false;
	
	//Do another overall line limits check -- restoration does this, but just in case something else does, as well
	if (use_link_limits == true)
	{
		//Call the current update -- do it as a "self call"
		my->CurrentCalculation(-1);

		//Now update the power value
		my->calculate_power();

		//Compute the overload amount, if any
		my->perform_limit_checks(overload_value,overloaded);
	}

	//Assume success
	return 1;
}

/**
* CurrentCalculation will perform the Newton-Raphson accumulation of
* current injections - functionalized for Master/Slave use
* nodecall is set by calling nodes - indicates the NR reference (for locking issues)
*
* This function may supercede "calc_currents" for the restoration module - quite honestly, I don't know if
*      that function even calculates proper values anymore.
*
* Locking not needed on various fnode/tnode voltage reads - rank separation prevents contention
*     For normal operation, only from nodes need locking - this is an assumption holdover from FBS days - it may need revisiting
*/
int link_object::CurrentCalculation(int nodecall)
{
	if (current_accumulated==false)	//Only update if we haven't done so yet
	{
		//Reset the deltamode-oriented flag, just because - will stay by exception
		inrush_computations_needed = false;

		if (is_closed())	//Only compute this if the overall link is "in service"
		{
			//Solve current equations to get current injections
			node *fnode = NULL;
			node *tnode = NULL;
			node *ofnode = NULL;
			OBJECT *fobjval = NULL;
			OBJECT *tobjval = NULL;
			complex vtemp[6];
			complex itemp[3];
			double vmagtemp[3];
			complex current_temp[6];
			complex shunt_current_val[6];
			complex invsquared;
			int jindex;
			bool flock;	//Flags to know if from side needs locking

			//Check our in-pass - see who called us - children will never call us, so checking "from" explicitly is unnecessary (error it)
			if (nodecall==-1)	//We called ourselves, full locking needed
			{
				flock=true;
			}
			else if (nodecall==NR_branchdata[NR_branch_reference].from)
			{
				flock=false;	//From node called us, so trying lock it would be futile
			}
			else if (nodecall==NR_branchdata[NR_branch_reference].to)
			{
				flock=true;		//To node called us, so lock the from to make sure nobody squashes us
			}
			else	//Defaulted else catch - better be a good reason ourselves or someone not connected to us is calling us
			{
				gl_error("Unexpected link current update call from %s",NR_busdata[nodecall].name);
				/*  TROUBLESHOOT
				While attempting to perform the current injection updates for a link, an unexpected object
				called the current update.  Please try again.  If the error persists, please submit your code
				and a bug report via the trac website.
				*/
				return 0;	//Failure
			}

			//Populate fnode and tnode differently - bypasses issue with children getting current accumulations they shouldn't
			fobjval = NR_busdata[NR_branchdata[NR_branch_reference].from].obj;
			tobjval = NR_busdata[NR_branchdata[NR_branch_reference].to].obj;

			//Map these into the old framework - will bypass childed objects this way
			fnode = OBJECTDATA(fobjval,node);
			tnode = OBJECTDATA(tobjval,node);

			//See if the from node is childed - it will have a different name address if it is
			if ((&from->name) != (&fobjval->name))
			{
				//Link up the "other from" node - 
				ofnode = OBJECTDATA(from,node);
			}
			else
				ofnode = NULL;	//Ensure it's blanked

			if ((SpecialLnk == DELTADELTA) || (SpecialLnk == WYEWYE))
			{
				//See if we're in deltamode and in-rush enabled
				if ((deltatimestep_running > 0) && (enable_inrush_calculations == true) && (SpecialLnk == WYEWYE))
				{
					//Pull in the voltages
					vtemp[0] = fnode->voltage[0];
					vtemp[1] = fnode->voltage[1];
					vtemp[2] = fnode->voltage[2];
					vtemp[3] = tnode->voltage[0];
					vtemp[4] = tnode->voltage[1];
					vtemp[5] = tnode->voltage[2];

					//Perform Itrafo = YTstore*V-htrafo
					lmatrix_vmult(YBase_Full, &vtemp[0], &current_temp[0], 6);

					//Subtract history aspects
					for (jindex=0; jindex<6; jindex++)
					{
						current_temp[jindex] -= LinkHistTermL[jindex];
					}

					//Calcualte "shunt" components, as necessary
					//From side
					if (LinkHistTermCf != NULL)
					{
						//Perform ishunt_f = YMfrom*V-hfrom
						lmatrix_vmult(YBase_Pri, &vtemp[0], &shunt_current_val[0], 3);

						//Subtract history aspects
						for (jindex=0; jindex<3; jindex++)
						{
							shunt_current_val[jindex] -= LinkHistTermCf[jindex];
						}
					}
					else	//Not there, just zero
					{
						shunt_current_val[0] = complex(0.0,0.0);
						shunt_current_val[1] = complex(0.0,0.0);
						shunt_current_val[2] = complex(0.0,0.0);
					}

					//To side
					if (LinkHistTermCt != NULL)
					{
						//Perform ishunt_t = YMTo*V-hTo
						lmatrix_vmult(YBase_Sec, &vtemp[3], &shunt_current_val[3], 3);

						//Subtract history aspects
						for (jindex=0; jindex<3; jindex++)
						{
							shunt_current_val[3+jindex] -= LinkHistTermCt[jindex];
						}
					}
					else	//Not there, just zero
					{
						shunt_current_val[3] = complex(0.0,0.0);
						shunt_current_val[4] = complex(0.0,0.0);
						shunt_current_val[5] = complex(0.0,0.0);
					}

					//See if saturation exists
					if (saturation_calculated_vals != NULL)
					{
						//Add in appropriate areas
						for (jindex=0; jindex<6; jindex++)
						{
							shunt_current_val[jindex] += saturation_calculated_vals[jindex];
						}
					}

					//Apply adjustments
					current_in[0] = current_temp[0] + shunt_current_val[0];
					current_in[1] = current_temp[1] + shunt_current_val[1];
					current_in[2] = current_temp[2] + shunt_current_val[2];

					//Output is backwards, plus needs other components negated (full backwards - injection into to node, so KCL)
					current_out[0] = -current_temp[3] - shunt_current_val[3];
					current_out[1] = -current_temp[4] - shunt_current_val[4];
					current_out[2] = -current_temp[5] - shunt_current_val[5];

					//Compute the difference, for convergence checks (semi-silly, but should still work)
					//Just prim - secondary of each phase, but if the difference doesn't move, should be solved (right?)
					vtemp[0] -= vtemp[3];
					vtemp[1] -= vtemp[4];
					vtemp[2] -= vtemp[5];

					//Get the mags
					vmagtemp[0] = vtemp[0].Mag();
					vmagtemp[1] = vtemp[1].Mag();
					vmagtemp[2] = vtemp[2].Mag();

					//Final determination of "convergence" or not
					if (fabs(vmagtemp[0]-inrush_vdiffmag_prev[0]) > inrush_tol_value)
						inrush_computations_needed = true;

					if (fabs(vmagtemp[1]-inrush_vdiffmag_prev[1]) > inrush_tol_value)
						inrush_computations_needed = true;

					if (fabs(vmagtemp[2]-inrush_vdiffmag_prev[2]) > inrush_tol_value)
						inrush_computations_needed = true;

					//Store the new "previous" values
					inrush_vdiffmag_prev[0] = vmagtemp[0];
					inrush_vdiffmag_prev[1] = vmagtemp[1];
					inrush_vdiffmag_prev[2] = vmagtemp[2];

					//************************************ Question ****************************************/
					//This may be different than how GLD does this -- is in the curr+shunt and out curr - shunt?
				}//End WYEWYE in-rush calculations
				else	//Not a WYEWYE or not an in-rush running version
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

					//See if our nature requires a lock
					if (flock)
					{
						//Lock the from side for current dispersion
						WRITELOCK_OBJECT(fobjval);
					}

					//Current in is just the same
					fnode->current_inj[0] += current_in[0];
					fnode->current_inj[1] += current_in[1];
					fnode->current_inj[2] += current_in[2];

					//If we locked our from node, be sure to let it go
					if (flock)
					{
						//Unlock the from side so others can play
						WRITEUNLOCK_OBJECT(fobjval);
					}

					//Replicate to the "original parent" if needed
					if (ofnode != NULL)
					{
						//See if our nature requires a lock
						if (flock)
						{
							//Lock the from side for current dispersion
							WRITELOCK_OBJECT(from);
						}

						//Apply current injection updates to child as well
						ofnode->current_inj[0] += current_in[0];
						ofnode->current_inj[1] += current_in[1];
						ofnode->current_inj[2] += current_in[2];

						//If we locked our from node, be sure to let it go
						if (flock)
						{
							//Unlock the from side so others can play
							WRITEUNLOCK_OBJECT(from);
						}
					}
				}//End not in-rush calculations (normal)
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

				//See if our nature requires a lock
				if (flock)
				{
					//Lock the from side for current dispersion
					WRITELOCK_OBJECT(fobjval);
				}

				//Current in is just the same
				fnode->current_inj[0] += current_in[0];
				fnode->current_inj[1] += current_in[1];
				fnode->current_inj[2] += current_in[2];

				//If we locked our from node, be sure to let it go
				if (flock)
				{
					//Unlock the from side so others can play
					WRITEUNLOCK_OBJECT(fobjval);
				}

				//Replicate to the "original parent" if needed
				if (ofnode != NULL)
				{
					//See if our nature requires a lock
					if (flock)
					{
						//Lock the from side for current dispersion
						WRITELOCK_OBJECT(from);
					}

					//Apply current injection updates to child
					ofnode->current_inj[0] += current_in[0];
					ofnode->current_inj[1] += current_in[1];
					ofnode->current_inj[2] += current_in[2];

					//If we locked our from node, be sure to let it go
					if (flock)
					{
						//Unlock the from side so others can play
						WRITEUNLOCK_OBJECT(from);
					}
				}

			}//End regulators
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

				//See if our nature requires a lock
				if (flock)
				{
					//Lock the from side for current dispersion
					WRITELOCK_OBJECT(fobjval);
				}

				//Current in is just the same
				fnode->current_inj[0] += current_in[0];
				fnode->current_inj[1] += current_in[1];
				fnode->current_inj[2] += current_in[2];

				//If we locked our from node, be sure to let it go
				if (flock)
				{
					//Unlock the from side so others can play
					WRITEUNLOCK_OBJECT(fobjval);
				}

				//Replicate to the "original parent" if needed
				if (ofnode != NULL)
				{
					//See if our nature requires a lock
					if (flock)
					{
						//Lock the from side for current dispersion
						WRITELOCK_OBJECT(from);
					}

					//Apply updates to child object
					ofnode->current_inj[0] += current_in[0];
					ofnode->current_inj[1] += current_in[1];
					ofnode->current_inj[2] += current_in[2];

					//If we locked our from node, be sure to let it go
					if (flock)
					{
						//Unlock the from side so others can play
						WRITEUNLOCK_OBJECT(from);
					}
				}
			}//end delta-GWYE
			else if (SpecialLnk == SPLITPHASE)	//Split phase, center tapped xformer
			{
				if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//A
				{
					current_in[0] = itemp[0] = 
						fnode->voltage[0]*b_mat[2][2]+
						tnode->voltage[0]*b_mat[2][0]+
						tnode->voltage[1]*b_mat[2][1];

					//See if our nature requires a lock
					if (flock)
					{
						//Lock the from side for current dispersion
						WRITELOCK_OBJECT(fobjval);
					}

					//Accumulate injection
					fnode->current_inj[0] += itemp[0];

					//If we locked our from node, be sure to let it go
					if (flock)
					{
						//Unlock the from side so others can play
						WRITEUNLOCK_OBJECT(fobjval);
					}

					//Replicate to the "original parent" if needed
					if (ofnode != NULL)
					{
						//See if our nature requires a lock
						if (flock)
						{
							//Lock the from side for current dispersion
							WRITELOCK_OBJECT(from);
						}

						//Apply update to child
						ofnode->current_inj[0] += itemp[0];

						//If we locked our from node, be sure to let it go
						if (flock)
						{
							//Unlock the from side so others can play
							WRITEUNLOCK_OBJECT(from);
						}
					}

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
					current_in[1] = itemp[0] = 
						fnode->voltage[1]*b_mat[2][2] +
						tnode->voltage[0]*b_mat[2][0] +
						tnode->voltage[1]*b_mat[2][1];

					//See if our nature requires a lock
					if (flock)
					{
						//Lock the from side for current dispersion
						WRITELOCK_OBJECT(fobjval);
					}

					//Accumulate the injection
					fnode->current_inj[1] += itemp[0];

					//If we locked our from node, be sure to let it go
					if (flock)
					{
						//Unlock the from side so others can play
						WRITEUNLOCK_OBJECT(fobjval);
					}

					//Replicate to the "original parent" if needed
					if (ofnode != NULL)
					{
						//See if our nature requires a lock
						if (flock)
						{
							//Lock the from side for current dispersion
							WRITELOCK_OBJECT(from);
						}

						//Apply updates to child
						ofnode->current_inj[1] += itemp[0];

						//If we locked our from node, be sure to let it go
						if (flock)
						{
							//Unlock the from side so others can play
							WRITEUNLOCK_OBJECT(from);
						}
					}

					//calculate current out
					current_out[0] = fnode->voltage[1]*b_mat[0][2] +
									 tnode->voltage[0]*b_mat[0][0] +
									 tnode->voltage[1]*b_mat[0][1];

					current_out[1] = fnode->voltage[1]*b_mat[1][2] +
									 tnode->voltage[0]*b_mat[1][0] +
									 tnode->voltage[1]*b_mat[1][1];

				}
				else if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//C
				{
					current_in[2] = itemp[0] = 
						fnode->voltage[2]*b_mat[2][2] +
						tnode->voltage[0]*b_mat[2][0] +
						tnode->voltage[1]*b_mat[2][1];

					//See if our nature requires a lock
					if (flock)
					{
						//Lock the from side for current dispersion
						WRITELOCK_OBJECT(fobjval);
					}

					//Accumulate the injection
					fnode->current_inj[2] += itemp[0];

					//If we locked our from node, be sure to let it go
					if (flock)
					{
						//Unlock the from side so others can play
						WRITEUNLOCK_OBJECT(fobjval);
					}

					//Replicate to the "original parent" if needed
					if (ofnode != NULL)
					{
						//See if our nature requires a lock
						if (flock)
						{
							//Lock the from side for current dispersion
							WRITELOCK_OBJECT(from);
						}

						//Apply updates to child
						ofnode->current_inj[2] += itemp[0];

						//If we locked our from node, be sure to let it go
						if (flock)
						{
							//Unlock the from side so others can play
							WRITEUNLOCK_OBJECT(from);
						}
					}

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

				//See if our nature requires a lock
				if (flock)
				{
					//Lock the from side for current dispersion
					WRITELOCK_OBJECT(fobjval);
				}

				//Current in values go to the injection
				fnode->current_inj[0] += current_in[0];
				fnode->current_inj[1] += current_in[1];

				//If we locked our from node, be sure to let it go
				if (flock)
				{
					//Unlock the from side so others can play
					WRITEUNLOCK_OBJECT(fobjval);
				}

				//Replicate to the "original parent" if needed
				if (ofnode != NULL)
				{
					//See if our nature requires a lock
					if (flock)
					{
						//Lock the from side for current dispersion
						WRITELOCK_OBJECT(from);
					}

					//Apply current injections to child
					ofnode->current_inj[0] += current_in[0];
					ofnode->current_inj[1] += current_in[1];

					//If we locked our from node, be sure to let it go
					if (flock)
					{
						//Unlock the from side so others can play
						WRITEUNLOCK_OBJECT(from);
					}
				}
			}//End split phase line
			else
			{
				//See if we're in deltamode and in-rush enabled
				if ((deltatimestep_running > 0) && (enable_inrush_calculations == true) && (SpecialLnk == NORMAL))
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

					//See if line capacitance is enabled
					if (use_line_cap == true)
					{
						//Compute shunt current values
						if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//A
						{
							shunt_current_val[0] = LinkCapShuntTerm[0] * fnode->voltage[0] + 
												   LinkCapShuntTerm[1] * fnode->voltage[1] + 
												   LinkCapShuntTerm[2] * fnode->voltage[2] -
												   LinkHistTermCf[0];

							shunt_current_val[3] = LinkCapShuntTerm[0] * tnode->voltage[0] + 
												   LinkCapShuntTerm[1] * tnode->voltage[1] + 
												   LinkCapShuntTerm[2] * tnode->voltage[2] -
												   LinkHistTermCt[0];
						}
						else	//Nope, zero these
						{
							shunt_current_val[0] = complex(0.0,0.0);
							shunt_current_val[3] = complex(0.0,0.0);
						}

						//Compute shunt current values
						if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//B
						{
							shunt_current_val[1] = LinkCapShuntTerm[3] * fnode->voltage[0] + 
												   LinkCapShuntTerm[4] * fnode->voltage[1] + 
												   LinkCapShuntTerm[5] * fnode->voltage[2] -
												   LinkHistTermCf[1];

							shunt_current_val[4] = LinkCapShuntTerm[3] * tnode->voltage[0] + 
												   LinkCapShuntTerm[4] * tnode->voltage[1] + 
												   LinkCapShuntTerm[5] * tnode->voltage[2] -
												   LinkHistTermCt[1];
						}
						else	//Nope, zero these
						{
							shunt_current_val[1] = complex(0.0,0.0);
							shunt_current_val[4] = complex(0.0,0.0);
						}

						//Compute shunt current values
						if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//C
						{
							shunt_current_val[2] = LinkCapShuntTerm[6] * fnode->voltage[0] + 
												   LinkCapShuntTerm[7] * fnode->voltage[1] + 
												   LinkCapShuntTerm[8] * fnode->voltage[2] -
												   LinkHistTermCf[2];

							shunt_current_val[5] = LinkCapShuntTerm[6] * tnode->voltage[0] + 
												   LinkCapShuntTerm[7] * tnode->voltage[1] + 
												   LinkCapShuntTerm[8] * tnode->voltage[2] -
												   LinkHistTermCt[2];
						}
						else	//Nope, zero these
						{
							shunt_current_val[2] = complex(0.0,0.0);
							shunt_current_val[5] = complex(0.0,0.0);
						}
					}
					else	//Just zero the values out - 0-2 = from, 3-5 = to
					{
						shunt_current_val[0] = shunt_current_val[1] = shunt_current_val[2] = complex(0.0,0.0);
						shunt_current_val[3] = shunt_current_val[4] = shunt_current_val[5] = complex(0.0,0.0);
					}

					//See if phases are valid
					if ((NR_branchdata[NR_branch_reference].phases & 0x04) == 0x04)	//A
					{
						current_out[0] = From_Y[0][0]*vtemp[0]+
										From_Y[0][1]*vtemp[1]+
										From_Y[0][2]*vtemp[2] - 
										LinkHistTermL[0];

						//Update our vmag value
						vmagtemp[0] = vtemp[0].Mag();
					}
					else
					{
						current_out[0] = 0.0;
						vmagtemp[0] = 0.0;
					}

					if ((NR_branchdata[NR_branch_reference].phases & 0x02) == 0x02)	//B
					{
						current_out[1] = From_Y[1][0]*vtemp[0]+
										From_Y[1][1]*vtemp[1]+
										From_Y[1][2]*vtemp[2] -
										LinkHistTermL[1];

						//Update our vmag value
						vmagtemp[1] = vtemp[1].Mag();
					}
					else
					{
						current_out[1] = 0.0;
						vmagtemp[1] = 0.0;
					}

					if ((NR_branchdata[NR_branch_reference].phases & 0x01) == 0x01)	//C
					{
						current_out[2] = From_Y[2][0]*vtemp[0]+
									From_Y[2][1]*vtemp[1]+
									From_Y[2][2]*vtemp[2] - 
									LinkHistTermL[2];

						//Update our vmag value
						vmagtemp[2] = vtemp[2].Mag();
					}
					else
					{
						current_out[2] = 0.0;
						vmagtemp[2] = 0.0;
					}

					//Now calculate current_in
					current_in[0] = current_out[0] + shunt_current_val[0];
					current_in[1] = current_out[1] + shunt_current_val[1];
					current_in[2] = current_out[2] + shunt_current_val[2];

					//Adjust current out
					current_out[0] -= shunt_current_val[3];
					current_out[1] -= shunt_current_val[4];
					current_out[2] -= shunt_current_val[5];

					//Final determination of "convergence" or not
					if (fabs(vmagtemp[0]-inrush_vdiffmag_prev[0]) > inrush_tol_value)
						inrush_computations_needed = true;

					if (fabs(vmagtemp[1]-inrush_vdiffmag_prev[1]) > inrush_tol_value)
						inrush_computations_needed = true;

					if (fabs(vmagtemp[2]-inrush_vdiffmag_prev[2]) > inrush_tol_value)
						inrush_computations_needed = true;

					//Store the new "previous" values
					inrush_vdiffmag_prev[0] = vmagtemp[0];
					inrush_vdiffmag_prev[1] = vmagtemp[1];
					inrush_vdiffmag_prev[2] = vmagtemp[2];

					//************************************ Question ****************************************/
					//This may be different than how GLD does this -- is in the curr+shunt and out curr - shunt?
				}
				else	//Normal line -- compute like usual
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

					//Now calculate current_in
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
				}//End "normal" calculation

				//See if our nature requires a lock
				if (flock)
				{
					//Lock the from side for current dispersion
					WRITELOCK_OBJECT(fobjval);
				}

				//Current in is just the same
				fnode->current_inj[0] += current_in[0];
				fnode->current_inj[1] += current_in[1];
				fnode->current_inj[2] += current_in[2];

				//If we locked our from node, be sure to let it go
				if (flock)
				{
					//Unlock the from side so others can play
					WRITEUNLOCK_OBJECT(fobjval);
				}

				//Replicate to the "original parent" if needed
				if (ofnode != NULL)
				{
					//See if our nature requires a lock
					if (flock)
					{
						//Lock the from side for current dispersion
						WRITELOCK_OBJECT(from);
					}

					//Apply current injections to childed object
					ofnode->current_inj[0] += current_in[0];
					ofnode->current_inj[1] += current_in[1];
					ofnode->current_inj[2] += current_in[2];

					//If we locked our from node, be sure to let it go
					if (flock)
					{
						//Unlock the from side so others can play
						WRITEUNLOCK_OBJECT(from);
					}
				}
			}//End "normal" lines
		}//End is closed
		else	//Open, so no current
		{
			current_in[0] = current_in[1] = current_in[2] = 0.0;
			current_out[0] = current_out[1] = current_out[2] = 0.0;
		}

		//Flag us as done
		current_accumulated = true;
	}//End current not accumulated yet

	return 1;	//Assume it's always successful now
}

//Module-level deltamode call
SIMULATIONMODE link_object::inter_deltaupdate_link(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	//OBJECT *hdr = OBJECTHDR(this);

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Link presync stuff
		NR_link_presync_fxn();
		
		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}
	else	//After the call
	{
		//Call postsync
		BOTH_link_postsync_fxn();

		//See if we're in-rush-desired.  If so, check that we're ready to leave
		if (enable_inrush_calculations == true)
		{
			//See if we lack confidence to exit
			if (inrush_computations_needed == false)	//We're happy/don't care, onward!
			{
				return SM_EVENT;
			}
			else	//Must still want some computations
			{
				return SM_DELTA;
			}
		}
		else	//Standard deltamode
		{
			return SM_EVENT;	//Links always just want out
		}
	}
}//End module deltamode

//Power calculations
void link_object::calculate_power_splitphase()
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
		READLOCK_OBJECT(to);
		complex tc[] = {t->current_inj[0], t->current_inj[1], t->current_inj[2]};
		READUNLOCK_OBJECT(to);

		if (SpecialLnk!=SPLITPHASE) 
		{
			//Original code - attempted to split so single phase matches three phase in terms of output capabilities
			//power_in = (f->voltage[0]*~current_in[0] - f->voltage[1]*~current_in[1] + f->voltage[2]*~current_in[2]).Mag();
			//power_out = (t->voltage[0]*~t->current_inj[0] - t->voltage[1]*~t->current_inj[1] + t->voltage[2]*~t->current_inj[2]).Mag();
				
			//Follows convention of three phase calculations below
			indiv_power_in[0] = f->voltage[0]*~current_in[0];
			indiv_power_in[1] = complex(-1.0) * f->voltage[1]*~current_in[1];
			indiv_power_in[2] = f->voltage[2]*~current_in[2];

			indiv_power_out[0] = t->voltage[0]*~tc[0];
			indiv_power_out[1] = complex(-1.0) * t->voltage[1]*~tc[1];
			indiv_power_out[2] = t->voltage[2]*~tc[2];
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

			indiv_power_out[0] = t->voltage[0]*~tc[0];
			indiv_power_out[1] = complex(-1.0) * t->voltage[1]*~tc[1];
			indiv_power_out[2] = t->voltage[2]*~tc[2];
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

void link_object::set_flow_directions(void)
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

void link_object::calculate_power()
{
		node *f = OBJECTDATA(from, node);
		node *t = OBJECTDATA(to, node);

		//Extra catch here, in case something calls the wrong one
		if (has_phase(PHASE_S))
		{
			calculate_power_splitphase();
			return;
		}

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

			READLOCK_OBJECT(to);
			complex tc[] = {t->current_inj[0], t->current_inj[1], t->current_inj[2]};
			READUNLOCK_OBJECT(to);

			indiv_power_out[0] = t->voltage[0]*~tc[0];
			indiv_power_out[1] = t->voltage[1]*~tc[1];
			indiv_power_out[2] = t->voltage[2]*~tc[2];
		}

		power_in = indiv_power_in[0] + indiv_power_in[1] + indiv_power_in[2];
		power_out = indiv_power_out[0] + indiv_power_out[1] + indiv_power_out[2];

		//Calculate overall losses
		if ((SpecialLnk != DELTAGWYE) && (SpecialLnk != DELTADELTA))
		{
			//Figure out losses - fix for reverse flow capabilities
			for (int i=0; i<3; i++)
			{
				indiv_power_loss[i] = indiv_power_in[i] - indiv_power_out[i];
				if (indiv_power_loss[i].Re() < 0)
					indiv_power_loss[i].Re() = -indiv_power_loss[i].Re();
			}


			power_loss = indiv_power_loss[0] + indiv_power_loss[1] + indiv_power_loss[2];
		}
		else	//Delta-GWye is a little different
		{
			//Just set to NaN to flag that they mean nothing - this will probably upset some platform
			indiv_power_loss[0] = NaN;
			indiv_power_loss[1] = NaN;
			indiv_power_loss[2] = NaN;

			//Calculate overall losses
			power_loss = power_in - power_out;
		}
	
		set_flow_directions();
}

//Retrieve value of a double
double *link_object::get_double(OBJECT *obj, char *name)
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
int link_object::link_fault_on(OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time, void *Extra_Data)
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
	double type_fault;
	bool switch_val;
	complex C_mat[7][7];
	int64 pf_resultval;
	bool pf_badcompute;
	NR_MESHFAULT_IMPEDANCE pf_mesh_fault_values;
	NRSOLVERMODE pf_solvermode;
	complex pf_mesh_fault_impedance_matrix[3][3];
	complex CI_mat[3][3];
	complex CV_mat[3][3];

	//Check to see which mode we are in
	if (meshed_fault_checking_enabled == false)	//"Normal" mode
	{
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
			//See if we're in mesh-searching mode or not
			if (meshed_fault_checking_enabled == true)
			{
				//Determine which scenario we're in
				if ((fault_type[3] == 'A') && (fault_type[4] == '\0'))	//Phase A occurance
				{
					if (has_phase(PHASE_A))
					{
						if ((NR_branchdata[NR_branch_reference].origphases & 0x04) == 0x04)	//make sure phase A is active (no previous fault)
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
						if ((NR_branchdata[NR_branch_reference].origphases & 0x02) == 0x02)	//make sure phase B is active (no previous fault)
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
						if ((NR_branchdata[NR_branch_reference].origphases & 0x01) == 0x01)	//make sure phase C is active (no previous fault)
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
					temp_phases = NR_branchdata[NR_branch_reference].origphases & 0x07;

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
					temp_phases = NR_branchdata[NR_branch_reference].origphases & 0x07;

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
			}
			else	//Normal method
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
			}//end of normal reliability mode

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
				type_fault = 1;
				fault_current_calc(C_mat, phase_remove, type_fault);
			} else if(*implemented_fault == 2){ //SLG-B -> Ifa=Ifc=Vbx=Vxg=0
				C_mat[3][0]=C_mat[4][2]=C_mat[5][4]=C_mat[6][6]=complex(1,0);
				type_fault = 2;
				fault_current_calc(C_mat, phase_remove, type_fault);
			} else if(*implemented_fault == 3){ //SLG-C -> Ifa=Ifb=Vcx=Vxg=0
				C_mat[3][0]=C_mat[4][1]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
				type_fault = 3;
				fault_current_calc(C_mat, phase_remove, type_fault);
			} else if(*implemented_fault == 4){ //DLG-AB -> Ifc=Vax=Vbx=Vxg=0
				C_mat[3][2]=C_mat[4][3]=C_mat[5][4]=C_mat[6][6]=complex(1,0);
				type_fault = 4;
				fault_current_calc(C_mat, phase_remove, type_fault);
			} else if(*implemented_fault == 5){ //DLG-BC -> Ifa=Vbx=Vcx=Vxg=0
				C_mat[3][0]=C_mat[4][4]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
				type_fault = 5;
				fault_current_calc(C_mat, phase_remove, type_fault);
			} else if(*implemented_fault == 6){ //DLG-CA -> Ifb=Vax=Vcx=Vxg=0
				C_mat[3][1]=C_mat[4][3]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
				type_fault = 6;
				fault_current_calc(C_mat, phase_remove, type_fault);
			} else if(*implemented_fault == 7){ //LL-AB -> Ifa+Ifb=Ifc=Vax=Vbx=0
				C_mat[3][0]=C_mat[3][1]=C_mat[4][2]=C_mat[5][3]=C_mat[6][4]=complex(1,0);
				type_fault = 7;
				fault_current_calc(C_mat, phase_remove, type_fault);
			} else if(*implemented_fault == 8){ //LL-BC -> Ifb+Ifc=Ifa=Vbx=Vcx=0
				C_mat[3][1]=C_mat[3][2]=C_mat[4][0]=C_mat[5][4]=C_mat[6][5]=complex(1,0);
				type_fault = 8;
				fault_current_calc(C_mat, phase_remove, type_fault);
			} else if(*implemented_fault == 9){ //LL-CA -> Ifa+Ifc=Ifb=Vax=Vcx=0
				C_mat[3][0]=C_mat[3][2]=C_mat[4][1]=C_mat[5][3]=C_mat[6][5]=complex(1,0);
				type_fault = 9;
				fault_current_calc(C_mat, phase_remove, type_fault);
			} else if(*implemented_fault == 10){ //TLG-ABC -> Vax=Vbx=Vcx=Vxg=0
				C_mat[3][3]=C_mat[4][4]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
				type_fault = 10;
				fault_current_calc(C_mat, phase_remove, type_fault);
			} else if(*implemented_fault == 32){ //TLL-ABC -> Ifa+Ifb+Ifc=Vax=Vbx=Vcx=0
				C_mat[3][0]=C_mat[3][1]=C_mat[3][2]=C_mat[4][3]=C_mat[5][4]=C_mat[6][5]=complex(1,0);
				type_fault = 11;
				fault_current_calc(C_mat, phase_remove, type_fault);
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
					if (protect_locations[phaseidx] == -1)
					{
						GL_THROW("Attempted to restore a device that never appears to have been faulted!");
						/*  TROUBLESHOOT
						While attempting to restore a device, it was found to have never been in a fault.
						It is unclear how this occurs, but it should not and should be fixed.
						*/
					}
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
	}//End "normal" fault operations mode
	else	//Meshed checking -- handle differently
	{
		// set defaults for C_mat
		for(int n=0; n<7; n++){
			for(int m=0; m<7; m++) {
				C_mat[n][m]=complex(0,0);
			}
		}

		for(int n=0; n<3; n++){
			for(int m=0; m<3; m++) {
				CI_mat[n][m]=complex(0,0);
				CV_mat[n][m]=complex(0,0);
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

		//Call the solver for the impedance grab - do this prior to the code below removing phases
		//******************* NOTE -- This may have issues with "non-eventgen" items, like switch opens or fuse blows -- test! *****/
		if ((enable_mesh_fault_current == true) && (prev_LTime != 0))	//Make sure we're in mesh mode and not the first timestep
		{
			//Init variables
			pf_badcompute = false;
			pf_mesh_fault_values.return_code = 0;
			pf_mesh_fault_values.z_matrix = &pf_mesh_fault_impedance_matrix[0][0];

			//Just go with "static" powerflow
			pf_solvermode=PF_NORMAL;

			//Populate the mesh fault reference - Pull the "TO" node
			//***** NOTE -- Better approach?? ****/
			pf_mesh_fault_values.NodeRefNum = NR_branchdata[NR_branch_reference].to;

			//Call the powerflow/impednace creater
			pf_resultval = solver_nr(NR_bus_count, NR_busdata, NR_branch_count, NR_branchdata, &NR_powerflow, pf_solvermode, &pf_mesh_fault_values, &pf_badcompute);

			//Check the output
			if ((pf_badcompute == true) || (pf_mesh_fault_values.return_code != 1) || (pf_resultval <= 0))
			{
				GL_THROW("link:%d - %s -- Mesh-based fault impedance update failure",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to obtain the mesh fault method impedance, an error occurred.  Look for an earlier message for the explicit reason.
				*/
			}
			//Default else -- all was happy, and onward we go!
		}//End mesh fault current impedance pull

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
					if ((NR_branchdata[NR_branch_reference].origphases & 0x04) == 0x04)	//make sure phase A is active (no previous fault)
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
					if ((NR_branchdata[NR_branch_reference].origphases & 0x02) == 0x02)	//make sure phase B is active (no previous fault)
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
					if ((NR_branchdata[NR_branch_reference].origphases & 0x01) == 0x01)	//make sure phase C is active (no previous fault)
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
				temp_phases = NR_branchdata[NR_branch_reference].origphases & 0x07;

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
				temp_phases = NR_branchdata[NR_branch_reference].origphases & 0x07;

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

			//See which fault current mode we are leveraging
			if (enable_mesh_fault_current == true)	//Mesh fault current
			{
				if (prev_LTime != 0)	//Make sure we're not the very first pass of a timestep - theoretically, the old method hates this too
				{
					//set up the remaining 4 fault specific equations in C_mat before calculating the fault current
					if(*implemented_fault == 1){ //SLG-A -> Ifb=Ifc=Vax=Vxg=0
						if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x03) { //three-phase: ABC. check if BC is in phases
							CI_mat[1][1] = CI_mat[2][2]=complex(1,0);
							CI_mat[0][0] = fault_Z*-1.0;
							CV_mat[0][0] = complex(1,0);
							type_fault = 133;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x02) { //two phase: AB. Check for B phases
							CI_mat[0][1] = complex(1,0);
							CI_mat[1][0] = fault_Z*-1.0;
							CV_mat[1][0] = complex(1,0);
							type_fault = 1221;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x01) { //two phase: AC. Check for C in phases
							CI_mat[0][1] = complex(1,0);
							CI_mat[1][0] = fault_Z*-1.0;
							CV_mat[1][0] = complex(1,0);
							type_fault = 1222;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x00) { //single-phase: only A. There should be nothing in phases
							CI_mat[0][0] = fault_Z*-1.0;
							CV_mat[0][0] = complex(1,0);
							type_fault = 111;
						}
						mesh_fault_current_calc(pf_mesh_fault_impedance_matrix,CV_mat,CI_mat,NR_busdata[NR_swing_bus_reference].V,type_fault);

					} else if(*implemented_fault == 2){ //SLG-B -> Ifa=Ifc=Vbx=Vxg=0
						if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x05) { //three-phase: ABC. check if AC is in phases
							CI_mat[0][0] = CI_mat[2][2]=complex(1,0);
							CI_mat[1][1] = fault_Z*-1.0;
							CV_mat[1][1] = complex(1,0);
							type_fault = 233;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x04) { //two phase: AB. Check for A in phases
							CI_mat[0][1] = complex(1,0);
							CI_mat[1][0] = fault_Z*-1.0;
							CV_mat[1][0] = complex(1,0);
							type_fault = 2221;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x01) { //two phase: BC. Check for C in phases
							CI_mat[0][1] = complex(1,0);
							CI_mat[1][0] = fault_Z*-1.0;
							CV_mat[1][0] = complex(1,0);
							type_fault = 2222;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x00) { //single-phase: only B. There should be nothing in phases
							CI_mat[0][0] = fault_Z*-1.0;
							CV_mat[0][0] = complex(1,0);
							type_fault = 211;
						}
						mesh_fault_current_calc(pf_mesh_fault_impedance_matrix,CV_mat,CI_mat,NR_busdata[NR_swing_bus_reference].V,type_fault);

					} else if(*implemented_fault == 3){ //SLG-C -> Ifa=Ifb=Vcx=Vxg=0
						if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x06) { //three-phase: ABC. check if AB is in phases
							CI_mat[0][0] = CI_mat[1][1]=complex(1,0);
							CI_mat[2][2] = fault_Z*-1.0;
							CV_mat[2][2] = complex(1,0);
							type_fault = 333;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x04) { //two phase: AC. Check for A in phases
							CI_mat[0][1] = complex(1,0);
							CI_mat[1][0] = fault_Z*-1.0;
							CV_mat[1][0] = complex(1,0);
							type_fault = 3221;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x02) { //two phase: BC. Check for B in phases
							CI_mat[0][1] = complex(1,0);
							CI_mat[1][0] = fault_Z*-1.0;
							CV_mat[1][0] = complex(1,0);
							type_fault = 3222;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x00) { //single-phase: only C. There should be nothing in phases
							CI_mat[0][0] = fault_Z*-1.0;
							CV_mat[0][0] = complex(1,0);
							type_fault = 311;
						}
						mesh_fault_current_calc(pf_mesh_fault_impedance_matrix,CV_mat,CI_mat,NR_busdata[NR_swing_bus_reference].V,type_fault);

					} else if(*implemented_fault == 4){ //DLG-AB -> Ifc=Vax=Vbx=Vxg=0
						if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x01) { //three-phase: ABC. check if C is in phases
							CI_mat[0][2] = complex(1,0);
							CI_mat[1][0] = CI_mat[2][1] = (fault_Z + ground_Z)*-1.0;
							CI_mat[1][1] = CI_mat[2][0] = ground_Z*-1.0;
							CV_mat[1][0] = CV_mat[2][1] = complex(1,0);
							type_fault = 433;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x00) { //two phase: AB. There shold be nothing in phases
							CI_mat[0][0] = CI_mat[1][1] =  (fault_Z + ground_Z)*-1.0;
							CI_mat[0][1] = CI_mat[1][0] =  ground_Z*-1.0;
							CV_mat[0][0] = CV_mat[1][1] = complex(1,0);
							type_fault = 422;
						}
						mesh_fault_current_calc(pf_mesh_fault_impedance_matrix,CV_mat,CI_mat,NR_busdata[NR_swing_bus_reference].V,type_fault);

					} else if(*implemented_fault == 5){ //DLG-BC -> Ifa=Vbx=Vcx=Vxg=0
						if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x04) { //three-phase: ABC. check if A is in phases
							CI_mat[0][0] = complex(1,0);
							CI_mat[1][1] = CI_mat[2][2] = (fault_Z + ground_Z)*-1.0;
							CI_mat[1][2] = CI_mat[2][1] = ground_Z*-1.0;
							CV_mat[1][1] = CV_mat[2][2] = complex(1,0);
							type_fault = 533;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x00) { //two phase: BC. There shold be nothing in phases
							CI_mat[0][0] = CI_mat[1][1] =  (fault_Z + ground_Z)*-1.0;
							CI_mat[0][1] = CI_mat[1][0] =  ground_Z*-1.0;
							CV_mat[0][0] = CV_mat[1][1] = complex(1,0);
							type_fault = 522;
						}
						mesh_fault_current_calc(pf_mesh_fault_impedance_matrix,CV_mat,CI_mat,NR_busdata[NR_swing_bus_reference].V,type_fault);
						
					} else if(*implemented_fault == 6){ //DLG-CA -> Ifb=Vax=Vcx=Vxg=0
						if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x02) { //three-phase: ABC. check if B is in phases
							CI_mat[0][1] = complex(1,0);
							CI_mat[1][2] = CI_mat[2][0] = (fault_Z + ground_Z)*-1.0;
							CI_mat[1][0] = CI_mat[2][2] = ground_Z*-1.0;
							CV_mat[1][2] = CV_mat[2][0] = complex(1,0);
							type_fault = 633;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x00) { //two phase: CA. There shold be nothing in phases
							CI_mat[0][0] = CI_mat[1][1] =  (fault_Z + ground_Z)*-1.0;
							CI_mat[0][1] = CI_mat[1][0] =  ground_Z*-1.0;
							CV_mat[0][0] = CV_mat[1][1] = complex(1,0);
							type_fault = 622;
						}
						mesh_fault_current_calc(pf_mesh_fault_impedance_matrix,CV_mat,CI_mat,NR_busdata[NR_swing_bus_reference].V,type_fault);
						
					} else if(*implemented_fault == 7){ //LL-AB -> Ifa+Ifb=Ifc=Vax=Vbx=0
						if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x01) { //three-phase: ABC. check if C is in phases
							CI_mat[0][2] = CI_mat[1][0] = CI_mat[1][1]=complex(1,0);
							CI_mat[2][0] = fault_Z*-1.0;
							CV_mat[2][0] = complex(1,0);
							CV_mat[2][1] = complex(-1,0);
							type_fault = 733;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x00) { //two phase: AB. There shold be nothing in phases
							CI_mat[0][0] = CI_mat[0][1] =  complex(1,0);
							CI_mat[1][0] = fault_Z*-1.0;
							CV_mat[1][0] = complex(1,0);
							CV_mat[1][1] = complex(-1,0);
							type_fault = 722;
						}
						mesh_fault_current_calc(pf_mesh_fault_impedance_matrix,CV_mat,CI_mat,NR_busdata[NR_swing_bus_reference].V,type_fault);
						
					} else if(*implemented_fault == 8){ //LL-BC -> Ifb+Ifc=Ifa=Vbx=Vcx=0
						if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x04) { //three-phase: ABC. check if A is in phases
							CI_mat[0][0] = CI_mat[1][1] = CI_mat[1][2]=complex(1,0);
							CI_mat[2][1] = fault_Z*-1.0;
							CV_mat[2][1] = complex(1,0);
							CV_mat[2][2] = complex(-1,0);
							type_fault = 833;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x00) { //two phase: BC. There shold be nothing in phases
							CI_mat[0][0] = CI_mat[0][1] =  complex(1,0);
							CI_mat[1][0] = fault_Z*-1.0;
							CV_mat[1][0] = complex(1,0);
							CV_mat[1][1] = complex(-1,0);
							type_fault = 822;
						}
						mesh_fault_current_calc(pf_mesh_fault_impedance_matrix,CV_mat,CI_mat,NR_busdata[NR_swing_bus_reference].V,type_fault);
						
					} else if(*implemented_fault == 9){ //LL-CA -> Ifa+Ifc=Ifb=Vax=Vcx=0
						if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x02) { //three-phase: ABC. check if B is in phases
							CI_mat[0][1] = CI_mat[1][0] = CI_mat[1][2]=complex(1,0);
							CI_mat[2][2] = fault_Z*-1.0;
							CV_mat[2][2] = complex(1,0);
							CV_mat[2][0] = complex(-1,0);
							type_fault = 933;
						}
						else if ((NR_branchdata[NR_branch_reference].phases & 0x07) == 0x00) { //two phase: CA. There shold be nothing in phases
							CI_mat[0][0] = CI_mat[0][1] =  complex(1,0);
							CI_mat[1][0] = fault_Z*-1.0;
							CV_mat[1][0] = complex(1,0);
							CV_mat[1][1] = complex(-1,0);
							type_fault = 922;
						}
						mesh_fault_current_calc(pf_mesh_fault_impedance_matrix,CV_mat,CI_mat,NR_busdata[NR_swing_bus_reference].V,type_fault);
						
					} else if(*implemented_fault == 10){ //TLG-ABC -> Vax=Vbx=Vcx=Vxg=0
						//C_mat[3][3]=C_mat[4][4]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
						CI_mat[0][0] = CI_mat[1][1] = CI_mat[2][2] = (fault_Z + ground_Z)*-1.0;
						CI_mat[0][1] = CI_mat[0][2] = CI_mat[1][0] = CI_mat[1][2] = CI_mat[2][0] = CI_mat[2][1] = ground_Z*-1.0;
						CV_mat[0][0] = CV_mat[1][1] = CV_mat[2][2] = complex(1,0);
						type_fault = 1033;
						mesh_fault_current_calc(pf_mesh_fault_impedance_matrix,CV_mat,CI_mat,NR_busdata[NR_swing_bus_reference].V,type_fault);
						//fault_current_calc(C_mat, phase_remove, type_fault);
					} else if(*implemented_fault == 32){ //TLL-ABC -> Ifa+Ifb+Ifc=Vax=Vbx=Vcx=0
						//C_mat[3][0]=C_mat[3][1]=C_mat[3][2]=C_mat[4][3]=C_mat[5][4]=C_mat[6][5]=complex(1,0);
						CI_mat[0][0] = CI_mat[0][1] = CI_mat[0][2] = complex(1,0);
						CI_mat[1][0] = CI_mat[2][1] = fault_Z*-1.0;
						CI_mat[1][1] = CI_mat[2][2] = fault_Z;
						CV_mat[1][0] = CV_mat[2][1] = complex(1,0);
						CV_mat[1][1] = CV_mat[2][2] = complex(-1,0);
						type_fault = 1133;
						mesh_fault_current_calc(pf_mesh_fault_impedance_matrix,CV_mat,CI_mat,NR_busdata[NR_swing_bus_reference].V,type_fault);
						//fault_current_calc(C_mat, phase_remove, type_fault);
					}
				}//End not very first pass
				else	//Is the first pass, just inform fault current doesn't exit
				{
					gl_warning("link:%d - %s -- fault current not available on very first time loop",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
					/*  TROUBLESHOOT
					Due to the nature of the mesh-fault current calculation algorithm, it can not calculate a fault current on the very first iteration
					of the system.  Please place your fault time at least one second in the future and try again.
					*/
				}
			}//End mesh-fault current method
			else	//"Old method" for fault calculation - radial
			{
				//set up the remaining 4 fault specific equations in C_mat before calculating the fault current
				if(*implemented_fault == 1){ //SLG-A -> Ifb=Ifc=Vax=Vxg=0
					C_mat[3][1]=C_mat[4][2]=C_mat[5][3]=C_mat[6][6]=complex(1,0);
					type_fault = 1;
					fault_current_calc(C_mat, phase_remove, type_fault);
				} else if(*implemented_fault == 2){ //SLG-B -> Ifa=Ifc=Vbx=Vxg=0
					C_mat[3][0]=C_mat[4][2]=C_mat[5][4]=C_mat[6][6]=complex(1,0);
					type_fault = 2;
					fault_current_calc(C_mat, phase_remove, type_fault);
				} else if(*implemented_fault == 3){ //SLG-C -> Ifa=Ifb=Vcx=Vxg=0
					C_mat[3][0]=C_mat[4][1]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
					type_fault = 3;
					fault_current_calc(C_mat, phase_remove, type_fault);
				} else if(*implemented_fault == 4){ //DLG-AB -> Ifc=Vax=Vbx=Vxg=0
					C_mat[3][2]=C_mat[4][3]=C_mat[5][4]=C_mat[6][6]=complex(1,0);
					type_fault = 4;
					fault_current_calc(C_mat, phase_remove, type_fault);
				} else if(*implemented_fault == 5){ //DLG-BC -> Ifa=Vbx=Vcx=Vxg=0
					C_mat[3][0]=C_mat[4][4]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
					type_fault = 5;
					fault_current_calc(C_mat, phase_remove, type_fault);
				} else if(*implemented_fault == 6){ //DLG-CA -> Ifb=Vax=Vcx=Vxg=0
					C_mat[3][1]=C_mat[4][3]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
					type_fault = 6;
					fault_current_calc(C_mat, phase_remove, type_fault);
				} else if(*implemented_fault == 7){ //LL-AB -> Ifa+Ifb=Ifc=Vax=Vbx=0
					C_mat[3][0]=C_mat[3][1]=C_mat[4][2]=C_mat[5][3]=C_mat[6][4]=complex(1,0);
					type_fault = 7;
					fault_current_calc(C_mat, phase_remove, type_fault);
				} else if(*implemented_fault == 8){ //LL-BC -> Ifb+Ifc=Ifa=Vbx=Vcx=0
					C_mat[3][1]=C_mat[3][2]=C_mat[4][0]=C_mat[5][4]=C_mat[6][5]=complex(1,0);
					type_fault = 8;
					fault_current_calc(C_mat, phase_remove, type_fault);
				} else if(*implemented_fault == 9){ //LL-CA -> Ifa+Ifc=Ifb=Vax=Vcx=0
					C_mat[3][0]=C_mat[3][2]=C_mat[4][1]=C_mat[5][3]=C_mat[6][5]=complex(1,0);
					type_fault = 9;
					fault_current_calc(C_mat, phase_remove, type_fault);
				} else if(*implemented_fault == 10){ //TLG-ABC -> Vax=Vbx=Vcx=Vxg=0
					C_mat[3][3]=C_mat[4][4]=C_mat[5][5]=C_mat[6][6]=complex(1,0);
					type_fault = 10;
					fault_current_calc(C_mat, phase_remove, type_fault);
				} else if(*implemented_fault == 32){ //TLL-ABC -> Ifa+Ifb+Ifc=Vax=Vbx=Vcx=0
					C_mat[3][0]=C_mat[3][1]=C_mat[3][2]=C_mat[4][3]=C_mat[5][4]=C_mat[6][5]=complex(1,0);
					type_fault = 11;
					fault_current_calc(C_mat, phase_remove, type_fault);
				}
			}//End older, radial fault calculation method
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
					if (protect_locations[phaseidx] == -1)
					{
						GL_THROW("Attempted to restore a device that never appears to have been faulted!");
						/*  TROUBLESHOOT
						While attempting to restore a device, it was found to have never been in a fault.
						It is unclear how this occurs, but it should not and should be fixed.
						*/
					}
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
	}//End "Msehed mode" checks
}

//Function to remove enacted fault on link - use same list as above (link_fault_on)
int link_object::link_fault_off(int *implemented_fault, char *imp_fault_name, void *Extra_Data)
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

	//Check our operations mode
	if (meshed_fault_checking_enabled == false)	//Normal mode
	{
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
					if (protect_locations[phaseidx] == -1)
					{
						GL_THROW("An attempt to restore something that was never faulted has occurred!");
						/*  TROUBLESHOOT
						While attempting to restore a device, an unknown state was encountered where it
						had somehow started faulted and was not caught.  If you have any switches in your
						system, consider starting them closed first.
						*/
					}
					else if (protect_locations[phaseidx] == -99)
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
							if (meshed_fault_checking_enabled == true)
							{
								if ((NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases & 0x07) != 0x00)
								{
									NR_branchdata[protect_locations[phaseidx]].phases &= (NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases & 0x07);
								}
								else if ((NR_busdata[NR_branchdata[protect_locations[phaseidx]].to].phases & 0x07) != 0x00)
								{
									NR_branchdata[protect_locations[phaseidx]].phases &= (NR_busdata[NR_branchdata[protect_locations[phaseidx]].to].phases & 0x07);
								}
								//Default else, busted
							}
							else	//Normal, radial mode
							{
								NR_branchdata[protect_locations[phaseidx]].phases &= (NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases & 0x07);
							}

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

						//Clear "far" device lockout
						NR_branchdata[protect_locations[phaseidx]].faultphases &= ~(work_phases);	//Remove the phase lock
					}

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
	}//End "normal" operations mode
	else	//Must be crazy mesh checking mode
	{
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
					if (protect_locations[phaseidx] == -1)
					{
						GL_THROW("An attempt to restore something that was never faulted has occurred!");
						/*  TROUBLESHOOT
						While attempting to restore a device, an unknown state was encountered where it
						had somehow started faulted and was not caught.  If you have any switches in your
						system, consider starting them closed first.
						*/
					}
					else if (protect_locations[phaseidx] == -99)
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

							//Just call the restoration -- fault_check should pick up any abnormalities, or so the theory goes

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

							//Shouldn't even need to alter the flags
							////Flag one of our ends as active, just in case
							//if ((NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases & 0x07) != 0x00)
							//{
							//	NR_branchdata[protect_locations[phaseidx]].phases &= (NR_busdata[NR_branchdata[protect_locations[phaseidx]].from].phases & 0x07);
							//}
							//else if ((NR_busdata[NR_branchdata[protect_locations[phaseidx]].to].phases & 0x07) != 0x00)
							//{
							//	NR_branchdata[protect_locations[phaseidx]].phases &= (NR_busdata[NR_branchdata[protect_locations[phaseidx]].to].phases & 0x07);
							//}
							////Default else, busted

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

						//Clear "far" device lockout
						NR_branchdata[protect_locations[phaseidx]].faultphases &= ~(work_phases);	//Remove the phase lock
					}

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
	}//End meshed checking mode
}
//adding function to calculate the fault current seen at the swing bus then distribute that current down to fault path to the faulted line
//right now it is assumed that all faults occur at the to end of the faulted link object.

//This function currently computes SLG and LL faults for any phase on a three-phase node/line
//Therefore the inputs to this function are:
	//Zth[3][3]  = Thevenins equivalent impedance. It is a 3x3 matrix
	//CV[3][3]   = A 3x3 CV matrix calculated before this function is called
	//CI[3][3]   = A 3x3 CI matrix calculated before this function is called
	//VSth = Swing-node voltage. It is a 3x1 matrix
	//fault_type = Currently it is not used but it might come to some good use when adding all faults.

void link_object::mesh_fault_current_calc(complex Zth[3][3],complex CV[3][3],complex CI[3][3],complex *VSth,double fault_type)
	{
		int phaseCheck;
		phaseCheck = 0;

		if ((fault_type == 133) || (fault_type == 233) || (fault_type == 333) || (fault_type == 433) || (fault_type == 533) || (fault_type == 633) || (fault_type == 733) || (fault_type == 833) || (fault_type == 933) || (fault_type == 1033) || (fault_type == 1133)) {
			phaseCheck = 3;
		}
		else if ((fault_type == 1221) || (fault_type == 1222) ||(fault_type == 2221) || (fault_type == 2222) ||(fault_type == 3221) || (fault_type == 3222) ||(fault_type == 422) || (fault_type == 522) || (fault_type == 622) || (fault_type == 722) || (fault_type == 822) || (fault_type == 922)) {
			phaseCheck = 2;
		}
		else if ((fault_type == 111) || (fault_type == 211) || (fault_type == 311)) {
			phaseCheck = 1;
		}

		//populate CF[0-2][0-2] with Zth
		//populate CF[0-2][3-5] with Identity matrix
		//populate CF[3-5][0-2] with CI matrix
		//populate CF[3-5][3-5] with CV matrix
		//CF[0][4] = CF[0][5] = CF[1][3] = CF[1][5] = CF[2][3] = CF[2][4] = complex(0,0) since they are the zeroes of identity
		if (phaseCheck == 1)
			{
				complex CF[2][2];
                                if (fault_type == 111) {
					CF[0][0] = Zth[0][0]; //Zth
				} else if (fault_type == 211) {
					CF[0][0] = Zth[1][1]; //Zth
				} else if (fault_type == 311) {
					CF[0][0] = Zth[2][2]; //Zth
				}
				CF[0][1] = complex(1,0); //I
				CF[1][0] = CI[0][0]; //CI
				CF[1][1] = CV[0][0]; //CV

				//Populate VFrhs[0-2][0] with Swing-node/thevenin voltage
				complex VFrhs[2];
				VFrhs[0] = VSth[0];
				VFrhs[1] = complex(0,0);

				//calculate the fault current and fault voltage
				//invert CF matrix and save the output to CFinv
				complex CFinv[2][2];
				complex IVf[2];
				lu_matrix_inverse(&CF[0][0],&CFinv[0][0],2);

				//Following the formula: IVf (Fault current, voltage matrix) = CFinv*VFrhs.
				//IVf[0][0] is Fault Current value and IVf[1][0] is Fault Voltage value
				lmatrix_vmult(&CFinv[0][0],&VFrhs[0],&IVf[0],2);

				//Populate fault current into the output
                                if (fault_type == 111) {
					If_out[0] = IVf[0];
					//Assume lines for now -- input is the same (transformers will be different)
					If_in[0] = If_out[0];
				} else if (fault_type == 211) {
					If_out[1] = IVf[0];
					//Assume lines for now -- input is the same (transformers will be different)
					If_in[1] = If_out[1];
				} else if (fault_type == 311) {
					If_out[2] = IVf[0];
					//Assume lines for now -- input is the same (transformers will be different)
					If_in[2] = If_out[2];
				}

			}
		else if (phaseCheck == 2)
			{
				complex CF[4][4];

				CF[0][2] = complex(1,0); //I
				CF[1][3] = complex(1,0); //I

				CF[2][0] = CI[0][0]; //CI
				CF[2][1] = CI[0][1]; //CI
				CF[3][0] = CI[1][0]; //CI
				CF[3][1] = CI[1][1]; //CI

				CF[2][2] = CV[0][0]; //CV
				CF[2][3] = CV[0][1]; //CV
				CF[3][2] = CV[1][0]; //CV
				CF[3][3] = CV[1][1]; //CV

				//Populate VFrhs[0-2][0] with Swing-node/thevenin voltage
				complex VFrhs[4];
				if ((fault_type == 422) || (fault_type == 722) || (fault_type == 1221) || (fault_type == 2221)) //AB
					{
						CF[0][0] = Zth[0][0]; //Zth
						CF[0][1] = Zth[0][1]; //Zth
						CF[1][0] = Zth[1][0]; //Zth
						CF[1][1] = Zth[1][1]; //Zth

						VFrhs[0] = VSth[0];
						VFrhs[1] = VSth[1];
					}
				else if ((fault_type == 522) || (fault_type == 822) || (fault_type == 2222) || (fault_type == 3222)) //BC
					{
						CF[0][0] = Zth[1][1]; //Zth
						CF[0][1] = Zth[1][2]; //Zth
						CF[1][0] = Zth[2][1]; //Zth
						CF[1][1] = Zth[2][2]; //Zth

						VFrhs[0] = VSth[1];
						VFrhs[1] = VSth[2];
					}
				else if ((fault_type == 622) || (fault_type == 922) || (fault_type == 1222) || (fault_type == 3221)) //CA
					{
						CF[0][0] = Zth[0][0]; //Zth
						CF[0][1] = Zth[0][2]; //Zth
						CF[1][0] = Zth[2][0]; //Zth
						CF[1][1] = Zth[2][2]; //Zth

						VFrhs[0] = VSth[0];
						VFrhs[1] = VSth[2];
					}
				VFrhs[2] = complex(0,0);
				VFrhs[3] = complex(0,0);

				//calculate the fault current and fault voltage
				//invert CF matrix and save the output to CFinv
				complex CFinv[4][4];
				complex IVf[4];
				lu_matrix_inverse(&CF[0][0],&CFinv[0][0],4);

				//Following the formula: IVf (Fault current, voltage matrix) = CFinv*VFrhs.
				//IVf[0-1][0] is Fault Current value and IVf[2-3][0] is Fault Voltage value
				lmatrix_vmult(&CFinv[0][0],&VFrhs[0],&IVf[0],4);

				//Populate fault current into the output
				if ((fault_type == 422) || (fault_type == 722) || (fault_type == 1221) || (fault_type == 2221)) //AB
					{
						If_out[0] = IVf[0];
						If_out[1] = IVf[1];

						//Assume lines for now -- input is the same (transformers will be different)
						If_in[0] = If_out[0];
						If_in[1] = If_out[1];
					}
				else if ((fault_type == 522) || (fault_type == 822) || (fault_type == 2222) || (fault_type == 3222)) //BC
					{
						If_out[1] = IVf[0];
						If_out[2] = IVf[1];

						//Assume lines for now -- input is the same (transformers will be different)
						If_in[1] = If_out[1];
						If_in[2] = If_out[2];
					}
				else if ((fault_type == 622) || (fault_type == 922) || (fault_type == 1222) || (fault_type == 3221)) //CA
					{
						If_out[0] = IVf[0];
						If_out[2] = IVf[1];

						//Assume lines for now -- input is the same (transformers will be different)
						If_in[0] = If_out[0];
						If_in[2] = If_out[2];
					}
			}
		else if (phaseCheck == 3)
			{
				complex CF[6][6];

				CF[0][0] = Zth[0][0]; //Zth
				CF[0][1] = Zth[0][1]; //Zth
				CF[1][0] = Zth[1][0]; //Zth
				CF[1][1] = Zth[1][1]; //Zth
				CF[0][2] = Zth[0][2]; //Zth
				CF[1][2] = Zth[1][2]; //Zth
				CF[2][0] = Zth[2][0]; //Zth
				CF[2][1] = Zth[2][1]; //Zth
				CF[2][2] = Zth[2][2]; //Zth

				CF[0][3] = complex(1,0); //I
				CF[1][4] = complex(1,0); //I
				CF[2][5] = complex(1,0); //I

				CF[3][0] = CI[0][0]; //CI
				CF[3][1] = CI[0][1]; //CI
				CF[4][0] = CI[1][0]; //CI
				CF[4][1] = CI[1][1]; //CI
				CF[3][2] = CI[0][2]; //CI
				CF[4][2] = CI[1][2]; //CI
				CF[5][0] = CI[2][0]; //CI
				CF[5][1] = CI[2][1]; //CI
				CF[5][2] = CI[2][2]; //CI

				CF[3][3] = CV[0][0]; //CV
				CF[3][4] = CV[0][1]; //CV
				CF[4][3] = CV[1][0]; //CV
				CF[4][4] = CV[1][1]; //CV
				CF[3][5] = CV[0][2]; //CV
				CF[4][5] = CV[1][2]; //CV
				CF[5][3] = CV[2][0]; //CV
				CF[5][4] = CV[2][1]; //CV
				CF[5][5] = CV[2][2]; //CV

				//Populate VFrhs[0-2][0] with Swing-node/thevenin voltage
				complex VFrhs[6];
				VFrhs[0] = VSth[0];
				VFrhs[1] = VSth[1];
				VFrhs[2] = VSth[2];
				VFrhs[3] = complex(0,0);
				VFrhs[4] = complex(0,0);
				VFrhs[5] = complex(0,0);

				//calculate the fault current and fault voltage
				//invert CF matrix and save the output to CFinv
				complex CFinv[6][6];
				complex IVf[6];
				lu_matrix_inverse(&CF[0][0],&CFinv[0][0],6);

				//Following the formula: IVf (Fault current, voltage matrix) = CFinv*VFrhs.
				//IVf[0-2][0] is Fault Current value and IVf[3-5][0] is Fault Voltage value
				lmatrix_vmult(&CFinv[0][0],&VFrhs[0],&IVf[0],6);

				//Populate fault current into the output
				If_out[0] = IVf[0];
				If_out[1] = IVf[1];
				If_out[2] = IVf[2];

				//Assume lines for now -- input is the same (transformers will be different)
				If_in[0] = If_out[0];
				If_in[1] = If_out[1];
				If_in[2] = If_out[2];
			}
		else
			{
				GL_THROW("Invalid phase information. Expects one-phase, two-phase, or three-phase");
			}
}

void link_object::fault_current_calc(complex C[7][7],unsigned int removed_phase, double fault_type)
{
	int temp_branch_fc, temp_node, current_branch, temp_connection_type;;
	unsigned int temp_table_loc;
	unsigned char temp_branch_phases;
	char *temp_branch_name;
	OBJECT *temp_transformer, **temp_transformer_configuration;
	PROPERTY *temp_trans_config, *temp_con_typ;
	double temp_v_ratio;
	complex double_phase_det;
	complex V_sb[3];
	complex V_sys[3];
	complex Z_thevenin[3][3];
	complex Z_sys[3][3];
	complex Y_thevenin[3][3];
	complex Y_temp[3][3];
	complex Z_temp[3][3];
	complex C_inv[7][7];
	complex A_t[3][3];
    complex d_t[3][3];
	complex IP[7];
	complex L[7][7];
	complex U[7][7];
	complex zz[7];
	complex xx[7];
	complex det;

	// zero Z_thevenin !
	Z_thevenin[0][0]=Z_thevenin[0][1]=Z_thevenin[0][2]=Z_thevenin[1][0]=Z_thevenin[1][1]=Z_thevenin[1][2]=Z_thevenin[2][0]=Z_thevenin[2][1]=Z_thevenin[2][2]=0;

	temp_branch_fc = NR_branch_reference;
	temp_node = NR_branchdata[temp_branch_fc].from;
	NR_branchdata[temp_branch_fc].fault_link_below = -1; //-1 indicates that temp_branch_fc is the faulted link object 

	
	// loop that traces the SC path to the swing bus
	while (NR_busdata[temp_node].type != 2)
	{
		//Pull from bus of current link
		current_branch = temp_branch_fc;
		
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
			While attempting to induce a safety reaction to a fault, a progression through the
			links of the system failed.  Please try again.  If the bug persists, please submit your
			*/
		}
		temp_node = NR_branchdata[temp_branch_fc].from;
	}
	
	//Grab the swing bus voltage
	V_sb[0] = NR_busdata[temp_node].V[0];
	V_sb[1] = NR_busdata[temp_node].V[1];
	V_sb[2] = NR_busdata[temp_node].V[2];

	//Travel back down to the faulted node adding up all the device impedances in the fault path
	while(NR_branchdata[temp_branch_fc].fault_link_below != -1){
		temp_branch_phases = NR_branchdata[temp_branch_fc].phases & 0x07;
		if(NR_branchdata[temp_branch_fc].lnk_type == 4){//transformer
			temp_branch_name = NR_branchdata[temp_branch_fc].name;//get the name of the transformer object
			temp_transformer = gl_get_object(temp_branch_name);//get the transformer object
			if(gl_object_isa(temp_transformer, "transformer", "powerflow")){ // tranformer
				temp_trans_config = gl_get_property(temp_transformer,"configuration");//get pointer to the configuration property
				temp_transformer_configuration = gl_get_object_prop(temp_transformer, temp_trans_config);//get the transformer configuration object
				temp_con_typ = gl_get_property(*temp_transformer_configuration, "connect_type");//get pointer to the connection type
				temp_connection_type = *(int *)gl_get_enum(*temp_transformer_configuration, temp_con_typ);//get connection type
				if(temp_connection_type == 1){//WYE_WYE or DELTA-DELTA transformer
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
					A_t[0][0] = A_t[1][1] = A_t[2][2] = d_t[0][0] = d_t[1][1] = d_t[2][2] = 1/NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_low = A_t * Z_high * d_t

					equalm(Z_thevenin,Z_sys);
					multiply(A_t,Z_sys,Z_thevenin);
					equalm(Z_thevenin,Z_sys);
					multiply(Z_sys,d_t,Z_thevenin);
					addition(Z_thevenin,Z_temp,Z_thevenin);
					V_sb[0] = V_sb[0]*A_t[0][0];
					V_sb[1] = V_sb[1]*A_t[1][1];
					V_sb[2] = V_sb[2]*A_t[2][2];
				} else if(temp_connection_type == 3){//Delta grounded WYE transformer
					if(temp_branch_phases == 0x07){
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

						A_t[0][0] = 1/NR_branchdata[temp_branch_fc].v_ratio;
						A_t[0][2] = -1/NR_branchdata[temp_branch_fc].v_ratio;
						A_t[1][0] = -1/NR_branchdata[temp_branch_fc].v_ratio;
						A_t[1][1] = 1/NR_branchdata[temp_branch_fc].v_ratio;
						A_t[2][0] = -1/NR_branchdata[temp_branch_fc].v_ratio;
						A_t[2][2] = 1/NR_branchdata[temp_branch_fc].v_ratio;

						d_t[0][0] = 1/NR_branchdata[temp_branch_fc].v_ratio;
						d_t[0][1] = -1/NR_branchdata[temp_branch_fc].v_ratio;
						d_t[1][1] = 1/NR_branchdata[temp_branch_fc].v_ratio;
						d_t[1][2] = -1/NR_branchdata[temp_branch_fc].v_ratio;
						d_t[2][0] = -1/NR_branchdata[temp_branch_fc].v_ratio;
						d_t[2][2] = 1/NR_branchdata[temp_branch_fc].v_ratio;

						equalm(Z_thevenin,Z_sys);
						multiply(A_t,Z_sys,Z_thevenin);
						equalm(Z_thevenin,Z_sys);
						multiply(Z_sys,d_t,Z_thevenin);
						addition(Z_thevenin,Z_temp,Z_thevenin);

						V_sys[0] = V_sb[0];
						V_sys[1] = V_sb[1];
						V_sys[2] = V_sb[2];
						V_sb[0] = A_t[0][0]*V_sys[0] + A_t[0][1]*V_sys[1] + A_t[0][2]*V_sys[2];
						V_sb[1] = A_t[1][0]*V_sys[0] + A_t[1][1]*V_sys[1] + A_t[1][2]*V_sys[2];
						V_sb[2] = A_t[2][0]*V_sys[0] + A_t[2][1]*V_sys[1] + A_t[2][2]*V_sys[2];
					} else {
					gl_warning("Delta-grounded WYE transformers with less than three phases aren't handled. Ignoring object. Fault current is not accurate.");
					}
				} else if(temp_connection_type == 4){// Single phase transformer
					gl_warning("Single phase transformers are not supported for fault analysis at this time. Ignoring object. Fault current is not accurate.");
				} else {//split-phase transformer
					gl_warning("split-phase transformers are not supported for fault analysis at this time. Ignoring object. Fault current is not accurate.");
				}
			} else if (gl_object_isa(temp_transformer, "regulator", "powerflow")){ // regulator right now assumed to have all taps in neutral.
				gl_warning("regulators are neglected from the fault calculation");
			} else {
				GL_THROW("link object is a type 4 but is not a transformer or a regulator!");
			}
		} else {//line or safety device
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
		temp_branch_fc = NR_branchdata[temp_branch_fc].fault_link_below;
	}

	//include the faulted link's impedance in the equivalent system impedance
	temp_branch_phases = removed_phase | NR_branchdata[temp_branch_fc].phases;
	if(NR_branchdata[temp_branch_fc].lnk_type == 4){//transformer
		temp_branch_name = NR_branchdata[temp_branch_fc].name;//get the name of the transformer object
		temp_transformer = gl_get_object(temp_branch_name);//get the transformer object
		if(gl_object_isa(temp_transformer, "transformer", "powerflow")){ // tranformer
			temp_trans_config = gl_get_property(temp_transformer,"configuration");//get pointer to the configuration property
			temp_transformer_configuration = gl_get_object_prop(temp_transformer, temp_trans_config);//get the transformer configuration object
			temp_con_typ = gl_get_property(*temp_transformer_configuration, "connect_type");//get pointer to the connection type
			temp_connection_type = *(int *)gl_get_enum(*temp_transformer_configuration, temp_con_typ);//get connection type
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
				A_t[0][0] = A_t[1][1] = A_t[2][2] = d_t[0][0] = d_t[1][1] = d_t[2][2] = 1/NR_branchdata[temp_branch_fc].v_ratio;//calculate the transfer matrix A_t such that Z_low = A_t * Z_high * d_t

				equalm(Z_thevenin,Z_sys);
				multiply(A_t,Z_sys,Z_thevenin);
				equalm(Z_thevenin,Z_sys);
				multiply(Z_sys,d_t,Z_thevenin);
				addition(Z_thevenin,Z_temp,Z_thevenin);
				V_sb[0] = V_sb[0]*A_t[0][0];
				V_sb[1] = V_sb[1]*A_t[1][1];
				V_sb[2] = V_sb[2]*A_t[2][2];
			} else if(temp_connection_type == 3){//Delta grounded WYE transformer
				if(temp_branch_phases == 0x07){
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

					A_t[0][0] = 1/NR_branchdata[temp_branch_fc].v_ratio;
					A_t[0][2] = -1/NR_branchdata[temp_branch_fc].v_ratio;
					A_t[1][0] = -1/NR_branchdata[temp_branch_fc].v_ratio;
					A_t[1][1] = 1/NR_branchdata[temp_branch_fc].v_ratio;
					A_t[2][0] = -1/NR_branchdata[temp_branch_fc].v_ratio;
					A_t[2][2] = 1/NR_branchdata[temp_branch_fc].v_ratio;

					d_t[0][0] = 1/NR_branchdata[temp_branch_fc].v_ratio;
					d_t[0][1] = -1/NR_branchdata[temp_branch_fc].v_ratio;
					d_t[1][1] = 1/NR_branchdata[temp_branch_fc].v_ratio;
					d_t[1][2] = -1/NR_branchdata[temp_branch_fc].v_ratio;
					d_t[2][0] = -1/NR_branchdata[temp_branch_fc].v_ratio;
					d_t[2][2] = 1/NR_branchdata[temp_branch_fc].v_ratio;

					equalm(Z_thevenin,Z_sys);
					multiply(A_t,Z_sys,Z_thevenin);
					equalm(Z_thevenin,Z_sys);
					multiply(Z_sys,d_t,Z_thevenin);
					addition(Z_thevenin,Z_temp,Z_thevenin);

					V_sys[0] = V_sb[0];
					V_sys[1] = V_sb[1];
					V_sys[2] = V_sb[2];
					V_sb[0] = A_t[0][0]*V_sys[0] + A_t[0][1]*V_sys[1] + A_t[0][2]*V_sys[2];
					V_sb[1] = A_t[1][0]*V_sys[0] + A_t[1][1]*V_sys[1] + A_t[1][2]*V_sys[2];
					V_sb[2] = A_t[2][0]*V_sys[0] + A_t[2][1]*V_sys[1] + A_t[2][2]*V_sys[2];
				} else {
				gl_warning("Delta-grounded WYE transformers with less than three phases aren't handled. Ignoring object. Fault current is not accurate.");
				}
			} else if(temp_connection_type == 4){// Single phase transformer
				gl_warning("Single phase transformers are not supported for fault analysis at this time. Ignoring object. Fault current is not accurate.");
			} else {//split-phase transformer
				gl_warning("split-phase transformers are not supported for fault analysis at this time. Ignoring object. Fault current is not accurate.");
			}
		} else if (gl_object_isa(temp_transformer, "regulator", "powerflow")){ // regulator right now assumed to have all taps in neutral.
			gl_warning("regulators are neglected from the fault calculation");
		} else {
			GL_THROW("link object is a type 4 but is not a transformer or a regulator!");
		}
	} else {//line or safety device
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

	if(fault_type == 1){//SLG-A
		det = C[1][4]*C[2][5] - C[1][5]*C[1][5];
		if(det.Mag() <= 1e-4)
			GL_THROW("Distribution system is singular. Unable to solve for SC current.");
		C_inv[0][0] = complex(1,0);
		C_inv[0][1] = (C[0][5]*C[1][5] - C[0][4]*C[2][5])/det;
		C_inv[0][2] = (C[0][4]*C[1][5] - C[0][5]*C[1][4])/det;
	} else if(fault_type == 2){//SLG-B
		det = C[0][3]*C[2][5] - C[0][5]*C[0][5];
		if(det.Mag() <= 1e-4)
			GL_THROW("Distribution system is singular. Unable to solve for SC current.");
		C_inv[1][1] = complex(1,0);
		C_inv[1][0] = (C[0][5]*C[1][5] - C[0][4]*C[2][5])/det;
		C_inv[1][2] = (C[0][4]*C[0][5] - C[0][3]*C[1][5])/det;
	} else if(fault_type == 3){//SLG-C
		det = C[0][3]*C[1][4] - C[0][4]*C[0][4];
		if(det.Mag() <= 1e-4)
			GL_THROW("Distribution system is singular. Unable to solve for SC current.");
		C_inv[2][2] = complex(1,0);
		C_inv[2][0] = (C[0][4]*C[1][5] - C[0][5]*C[1][4])/det;
		C_inv[2][1] = (C[0][4]*C[0][5] - C[0][3]*C[1][5])/det;
	} else if(fault_type == 4){//DLG-AB
		det = C[2][5];
		if(det.Mag() <= 1e-4)
			GL_THROW("Distribution system is singular. Unable to solve for SC current.");
		C_inv[0][0] = C_inv[1][1] = complex(1,0);
		C_inv[0][2] = -C[0][5]/det;
		C_inv[1][2] = -C[1][5]/det;
	} else if(fault_type == 5){//DLG-BC
		det = C[0][3];
		if(det.Mag() <= 1e-4)
			GL_THROW("Distribution system is singular. Unable to solve for SC current.");
		C_inv[1][1] = C_inv[2][2] = complex(1,0);
		C_inv[1][0] = -C[0][4]/det;
		C_inv[2][0] = -C[0][5]/det;
	} else if(fault_type == 6){//DLG-CA
		det = C[1][4];
		if(det.Mag() <= 1e-4)
			GL_THROW("Distribution system is singular. Unable to solve for SC current.");
		C_inv[0][0] = C_inv[2][2] = complex(1,0);
		C_inv[0][1] = -C[0][4]/det;
		C_inv[2][1] = -C[1][5]/det;
	} else if(fault_type == 7){//LL-AB
		det= C[0][5]*C[0][5] + complex(2,0)*C[0][5]*C[1][5] + C[1][5]*C[1][5] - C[0][3]*C[2][5] - complex(2,0)*C[0][4]*C[2][5] - C[1][4]*C[2][5];
		if(det.Mag() <= 1e-4)
			GL_THROW("Distribution system is singular. Unable to solve for SC current.");
		C_inv[0][0] = (C[1][5]*C[1][5] + C[0][5]*C[1][5] - C[0][4]*C[2][5] - C[1][4]*C[2][5])/det;
		C_inv[0][1] = -(C[0][5]*C[0][5] + C[1][5]*C[0][5] - C[0][3]*C[2][5] - C[0][4]*C[2][5])/det;
		C_inv[0][2] = (C[0][4]*C[0][5] + C[0][5]*C[1][4] - C[0][4]*C[1][5] - C[0][3]*C[1][5])/det;
		C_inv[1][1] = -C_inv[0][1];
		C_inv[1][2] = -C_inv[0][2];
		C_inv[1][0] = -C_inv[0][0];
	} else if(fault_type == 8){//LL-BC
		det= C[0][4]*C[0][4] + complex(2,0)*C[0][4]*C[0][5] + C[0][5]*C[0][5] - C[0][3]*C[1][4] - complex(2,0)*C[0][3]*C[1][5] - C[0][3]*C[2][5];
		if(det.Mag() <= 1e-4)
			GL_THROW("Distribution system is singular. Unable to solve for SC current.");
		C_inv[1][0] = (C[0][4]*C[1][5] - C[0][5]*C[1][4] - C[0][5]*C[1][5] + C[0][4]*C[2][5])/det;
		C_inv[1][1] = (C[0][5]*C[0][5] + C[0][4]*C[0][5] - C[0][3]*C[1][5] - C[0][3]*C[2][5])/det;
		C_inv[1][2] = -(C[0][4]*C[0][4] + C[0][5]*C[0][4] - C[0][3]*C[1][4] - C[0][3]*C[1][5])/det;
		C_inv[2][0] = -C_inv[1][0];
		C_inv[2][2] = -C_inv[1][2];
		C_inv[2][1] = -C_inv[1][1];
	} else if(fault_type == 9){//LL-CA
		det= -(C[0][4]*C[0][4] + complex(2,0)*C[0][4]*C[1][5] + C[1][5]*C[1][5] - C[0][3]*C[1][4] - complex(2,0)*C[0][5]*C[1][4] - C[1][4]*C[2][5]);
		if(det.Mag() <= 1e-4)
			GL_THROW("Distribution system is singular. Unable to solve for SC current.");
		C_inv[0][0] = -(C[1][5]*C[1][5] + C[0][4]*C[1][5] - C[0][5]*C[1][4] - C[1][4]*C[2][5])/det;
		C_inv[0][1] = -(C[0][4]*C[0][5] - C[0][3]*C[1][5] - C[0][5]*C[1][5] + C[0][4]*C[2][5])/det;
		C_inv[0][2] = (C[0][4]*C[0][4] + C[1][5]*C[0][4] - C[0][3]*C[1][4] - C[0][5]*C[1][4])/det;
		C_inv[2][1] = -C_inv[0][1];
		C_inv[2][2] = -C_inv[0][2];
		C_inv[2][0] = -C_inv[0][0];
	} else if(fault_type == 10){//TLG
		C_inv[0][0] = C_inv[1][1] = C_inv[2][2] = complex(1,0);
	} else if(fault_type == 11){//TLL
		det = C[0][3] + complex(2,0)*C[0][4] + complex(2,0)*C[0][5] + C[1][4] + complex(2,0)*C[1][5] + C[2][5];
		if(det.Mag() <= 1e-4)
			GL_THROW("Distribution system is singular. Unable to solve for SC current.");
		C_inv[0][0] = (C[0][4] + C[0][5] + C[1][4] + complex(2,0)*C[1][5] + C[2][5])/det;
		C_inv[0][1] = C_inv[0][2] = -C[0][6]/det;
		C_inv[1][0] = C_inv[1][2] = -C[1][6]/det;
		C_inv[1][1] = (C[0][3] + C[0][4] + complex(2,0)*C[0][5] + C[1][5] + C[2][5])/det;
		C_inv[2][0] = C_inv[2][1] = -C[2][6]/det;
		C_inv[2][2] = (C[0][3] + complex(2,0)*C[0][4] + C[0][5] + C[1][4] + C[1][5])/det;
	}
	//decompose the C matrix
//	lu_decomp(C, L, U);
//	//solve A*x = b using forward and backward substitution, L*z = b and U*x = z 
//	forward_sub(L, IP, zz);
//	back_sub(U, zz, xx);

	//pass the fault current back down to the faulted object
	NR_branchdata[temp_branch_fc].If_to[0] = C_inv[0][0]*IP[0] + C_inv[0][1]*IP[1] + C_inv[0][2]*IP[2];
	NR_branchdata[temp_branch_fc].If_to[1] = C_inv[1][0]*IP[0] + C_inv[1][1]*IP[1] + C_inv[1][2]*IP[2];
	NR_branchdata[temp_branch_fc].If_to[2] = C_inv[2][0]*IP[0] + C_inv[2][1]*IP[1] + C_inv[2][2]*IP[2];
	temp_node = NR_branchdata[temp_branch_fc].from;
	
	while (NR_busdata[temp_node].type != 2)
	{
		if(NR_branchdata[temp_branch_fc].lnk_type == 4){//transformer
			temp_branch_name = NR_branchdata[temp_branch_fc].name;//get the name of the transformer object
			temp_transformer = gl_get_object(temp_branch_name);//get the transformer object
			if(gl_object_isa(temp_transformer, "transformer", "powerflow")){ // tranformer
				temp_trans_config = gl_get_property(temp_transformer,"configuration");//get pointer to the configuration property
				temp_transformer_configuration = gl_get_object_prop(temp_transformer, temp_trans_config);//get the transformer configuration object
				temp_con_typ = gl_get_property(*temp_transformer_configuration, "connect_type");//get pointer to the connection type
				temp_connection_type = *(int *)gl_get_enum(*temp_transformer_configuration, temp_con_typ);//get connection type
				if(temp_connection_type == 1 || temp_connection_type == 2){//WYE_WYE or DELTA-DELTA transformer
					if(NR_branchdata[temp_branch_fc].fault_link_below != -1){
						NR_branchdata[temp_branch_fc].If_to[0] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[0];
						NR_branchdata[temp_branch_fc].If_to[1] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[1];
						NR_branchdata[temp_branch_fc].If_to[2] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[2];
					}
					temp_v_ratio = NR_branchdata[temp_branch_fc].v_ratio;
					NR_branchdata[temp_branch_fc].If_from[0] = NR_branchdata[temp_branch_fc].If_to[0]/temp_v_ratio;
					NR_branchdata[temp_branch_fc].If_from[1] = NR_branchdata[temp_branch_fc].If_to[1]/temp_v_ratio;
					NR_branchdata[temp_branch_fc].If_from[2] = NR_branchdata[temp_branch_fc].If_to[2]/temp_v_ratio;
				} else if(temp_connection_type == 3){//Delta grounded WYE transformer
					gl_warning("Delta-grounded WYE transformers are not supported for fault analysis at this time. Fault current is not accurate.");
				} else if(temp_connection_type == 4){// Single phase transformer
					gl_warning("Single phase transformers are not supported for fault analysis at this time. Fault current is not accurate.");
				} else {//split-phase transformer
					gl_warning("split-phase transformers are not supported for fault analysis at this time. Fault current is not accurate.");
				}
			} else if (gl_object_isa(temp_transformer, "regulator", "powerflow")){ // regulator right now assumed to have all taps in neutral.
				if(NR_branchdata[temp_branch_fc].fault_link_below != -1){
					NR_branchdata[temp_branch_fc].If_to[0] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[0];
					NR_branchdata[temp_branch_fc].If_to[1] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[1];
					NR_branchdata[temp_branch_fc].If_to[2] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[2];
				}
				NR_branchdata[temp_branch_fc].If_from[0] = NR_branchdata[temp_branch_fc].If_to[0];
				NR_branchdata[temp_branch_fc].If_from[1] = NR_branchdata[temp_branch_fc].If_to[1];
				NR_branchdata[temp_branch_fc].If_from[2] = NR_branchdata[temp_branch_fc].If_to[2];
				gl_warning("regulators are neglected from the fault calculation");
			} else {
				GL_THROW("link object is a type 4 but is not a transformer or a regulator!");
			}
		} else {
			if(NR_branchdata[temp_branch_fc].fault_link_below != -1){
				NR_branchdata[temp_branch_fc].If_to[0] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[0];
				NR_branchdata[temp_branch_fc].If_to[1] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[1];
				NR_branchdata[temp_branch_fc].If_to[2] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[2];
			}
			NR_branchdata[temp_branch_fc].If_from[0] = NR_branchdata[temp_branch_fc].If_to[0];
			NR_branchdata[temp_branch_fc].If_from[1] = NR_branchdata[temp_branch_fc].If_to[1];
			NR_branchdata[temp_branch_fc].If_from[2] = NR_branchdata[temp_branch_fc].If_to[2];
		}
		for (temp_table_loc=0; temp_table_loc<NR_busdata[temp_node].Link_Table_Size; temp_table_loc++)
		{
			//See if node is a to end - assumes radial phase progressions (i.e., no phase AB and phase C running into a node to form node ABC)
			if (NR_branchdata[NR_busdata[temp_node].Link_Table[temp_table_loc]].to == temp_node)	//This node is a to end
			{
				//Go up to the next level
				temp_branch_fc = NR_busdata[temp_node].Link_Table[temp_table_loc];
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
	}

	//update the fault current variables in link object connected to the swing bus
	if(NR_branchdata[temp_branch_fc].lnk_type == 4){//transformer
		temp_branch_name = NR_branchdata[temp_branch_fc].name;//get the name of the transformer object
		temp_transformer = gl_get_object(temp_branch_name);//get the transformer object
		if(gl_object_isa(temp_transformer, "transformer", "powerflow")){ // tranformer
			temp_trans_config = gl_get_property(temp_transformer,"configuration");//get pointer to the configuration property
			temp_transformer_configuration = gl_get_object_prop(temp_transformer, temp_trans_config);//get the transformer configuration object
			temp_con_typ = gl_get_property(*temp_transformer_configuration, "connect_type");//get pointer to the connection type
			temp_connection_type = *(int *)gl_get_enum(*temp_transformer_configuration, temp_con_typ);//get connection type
			if(temp_connection_type == 1 || temp_connection_type == 2){//WYE_WYE or DELTA-DELTA transformer
				if(NR_branchdata[temp_branch_fc].fault_link_below != -1){
					NR_branchdata[temp_branch_fc].If_to[0] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[0];
					NR_branchdata[temp_branch_fc].If_to[1] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[1];
					NR_branchdata[temp_branch_fc].If_to[2] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[2];
				}
				temp_v_ratio = NR_branchdata[temp_branch_fc].v_ratio;
				NR_branchdata[temp_branch_fc].If_from[0] = NR_branchdata[temp_branch_fc].If_to[0]/temp_v_ratio;
				NR_branchdata[temp_branch_fc].If_from[1] = NR_branchdata[temp_branch_fc].If_to[1]/temp_v_ratio;
				NR_branchdata[temp_branch_fc].If_from[2] = NR_branchdata[temp_branch_fc].If_to[2]/temp_v_ratio;
			} else if(temp_connection_type == 3){//Delta grounded WYE transformer
				gl_warning("Delta-grounded WYE transformers are not supported for fault analysis at this time. Fault current is not accurate.");
			} else if(temp_connection_type == 4){// Single phase transformer
				gl_warning("Single phase transformers are not supported for fault analysis at this time. Fault current is not accurate.");
			} else {//split-phase transformer
				gl_warning("split-phase transformers are not supported for fault analysis at this time. Fault current is not accurate.");
			}
		} else if (gl_object_isa(temp_transformer, "regulator", "powerflow")){ // regulator right now assumed to have all taps in neutral.
			if(NR_branchdata[temp_branch_fc].fault_link_below != -1){
				NR_branchdata[temp_branch_fc].If_to[0] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[0];
				NR_branchdata[temp_branch_fc].If_to[1] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[1];
				NR_branchdata[temp_branch_fc].If_to[2] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[2];
			}
			NR_branchdata[temp_branch_fc].If_from[0] = NR_branchdata[temp_branch_fc].If_to[0];
			NR_branchdata[temp_branch_fc].If_from[1] = NR_branchdata[temp_branch_fc].If_to[1];
			NR_branchdata[temp_branch_fc].If_from[2] = NR_branchdata[temp_branch_fc].If_to[2];
			gl_warning("regulators are neglected from the fault calculation");
		} else {
			GL_THROW("link object is a type 4 but is not a transformer or a regulator!");
		}
	} else {
		if(NR_branchdata[temp_branch_fc].fault_link_below != -1){
			NR_branchdata[temp_branch_fc].If_to[0] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[0];
			NR_branchdata[temp_branch_fc].If_to[1] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[1];
			NR_branchdata[temp_branch_fc].If_to[2] = NR_branchdata[NR_branchdata[temp_branch_fc].fault_link_below].If_from[2];
		}
		NR_branchdata[temp_branch_fc].If_from[0] = NR_branchdata[temp_branch_fc].If_to[0];
		NR_branchdata[temp_branch_fc].If_from[1] = NR_branchdata[temp_branch_fc].If_to[1];
		NR_branchdata[temp_branch_fc].If_from[2] = NR_branchdata[temp_branch_fc].If_to[2];
	}	
}

////////////////////////////////////////////////////////
// Larger matrix operations -- mostly for transformers
////////////////////////////////////////////////////////

//Inverse handled via lu_matrix_inverse routine - generic

//Performs a general matrix addition for square matrices of size "size"
//Computes matrix_in_A + matrix_in_B = matrix_out
//Insure all matrices are preallocated
void link_object::lmatrix_add(complex *matrix_in_A, complex *matrix_in_B, complex *matrix_out, int matsize)
{
	//Variables
	int jindex, kindex;
	OBJECT *obj = OBJECTHDR(this);

	//Initial check - make sure nothing NULL has been passed
	if ((matrix_in_A == NULL) || (matrix_in_B == NULL) || (matrix_out == NULL))
	{
		GL_THROW("link:%d-%s attempted to do a large matrix operation with an unallocated input or output matrix",obj->id,(obj->name ? obj->name : "unnamed"));
		//Defined above
	}

	//Do a negative/zero size check, just to stop stupid people
	if (matsize<1)
	{
		GL_THROW("link:%d-%s attempted to add matrices of an invalid size",obj->id,(obj->name ? obj->name : "unnamed"));
		/*  TROUBLESHOOT
		While attempting to perform a "large matrix" addition operation, but a negative or zero size was passed.  Please
		fix this and try again.
		*/
	}

	//Loop and do the add - simple
	for (jindex=0; jindex<matsize; jindex++)
	{
		for (kindex=0; kindex<matsize; kindex++)
		{
			matrix_out[jindex*matsize+kindex] = matrix_in_A[jindex*matsize+kindex] + matrix_in_B[jindex*matsize+kindex];
		}
	}
}

//Performs a general matrix multiplication for square matrices of size "size"
//Computes matrix_in_A * matrix_in_B = matrix_out
//Insure all matrices are preallocated
void link_object::lmatrix_mult(complex *matrix_in_A, complex *matrix_in_B, complex *matrix_out, int matsize)
{
	//Variables
	int jindex, kindex, lindex;
	OBJECT *obj = OBJECTHDR(this);

	//Initial check - make sure nothing NULL has been passed
	if ((matrix_in_A == NULL) || (matrix_in_B == NULL) || (matrix_out == NULL))
	{
		GL_THROW("link:%d-%s attempted to do a large matrix operation with an unallocated input or output matrix",obj->id,(obj->name ? obj->name : "unnamed"));
		//Defined above
	}

	//Do a negative/zero size check, just to stop stupid people
	if (matsize<1)
	{
		GL_THROW("link:%d-%s attempted to multiply matrices of an invalid size",obj->id,(obj->name ? obj->name : "unnamed"));
		/*  TROUBLESHOOT
		While attempting to perform a "large matrix" multiplication operation, but a negative or zero size was passed.  Please
		fix this and try again.
		*/
	}

	//Perform the matrix mulitplication
	for (jindex=0; jindex<matsize; jindex++)
	{
		for (kindex=0; kindex<matsize; kindex++)
		{
			//Zero the entry, since we'll accumulate into it
			matrix_out[jindex*matsize+kindex] = complex(0.0,0.0);

			for (lindex=0; lindex<matsize; lindex++)
			{
				matrix_out[jindex*matsize+kindex] += matrix_in_A[jindex*matsize+lindex] * matrix_in_B[lindex*matsize+kindex];
			}//End lindex
		}//End kindex
	}//End jindex
}

//Performs a general matrix-vector multiplication for square matrices of size "matsize"
//Computes matrix_in * vector_in = vector_out
//Insure all matrices/vector are preallocated
void link_object::lmatrix_vmult(complex *matrix_in, complex *vector_in, complex *vector_out, int matsize)
{
	//Variables
	int jindex, kindex;
	OBJECT *obj = OBJECTHDR(this);

	//Initial check - make sure nothing NULL has been passed
	if ((matrix_in == NULL) || (vector_in == NULL) || (vector_out == NULL))
	{
		GL_THROW("link:%d-%s attempted to do a large matrix operation with an unallocated input or output matrix",obj->id,(obj->name ? obj->name : "unnamed"));
		//Defined above
	}

	//Do a negative/zero size check, just to stop stupid people
	if (matsize<1)
	{
		GL_THROW("link:%d-%s attempted to multiply matrices of an invalid size",obj->id,(obj->name ? obj->name : "unnamed"));
		//Define elsewhere
	}

	//Perform the matrix mulitplication
	for (jindex=0; jindex<matsize; jindex++)
	{
		//Zero the entry, since we'll accumulate into it
		vector_out[jindex] = complex(0.0,0.0);

		//Multiply across the columns/rows
		for (kindex=0; kindex<matsize; kindex++)
		{
			vector_out[jindex] += matrix_in[jindex*matsize+kindex] * vector_in[kindex];
		}//End kindex
	}//End jindex
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

//Perform LU decomposition
void lu_decomp(complex *a, complex *l, complex *u, int size_val)
{
	int k, n, m, s;
	complex sum;
	// make sure L and U contain zeroes
	for(n=0; n<size_val; n++){
		for(m=0;m<size_val;m++){
			l[n*size_val+m] = complex(0,0);
			u[n*size_val+m] = complex(0,0);
		}
	}
	// for loop to decompose a in to lower triangular matrix l and upper triangular matirx u
	for(k=0; k<size_val; k++){
		l[k*size_val+k] = complex(1,0);
		for(m=k; m<size_val; m++){
			if(k==0){
				u[k*size_val+m] = a[k*size_val+m];
			}else{
				sum = complex(0,0);
				for(s=0; s<k; s++){
					sum += l[k*size_val+s]*u[s*size_val+m];
				}
				u[k*size_val+m] = a[k*size_val+m] - sum;
			}
		}// end U loop
		for(n=k+1; n<size_val; n++){
			if(k==0){
				l[n*size_val+k] = a[n*size_val+k]/u[k*size_val+k];
			}else{
				sum = complex(0,0);
				for(s=0; s<k; s++){
					sum += l[n*size_val+s]*u[s*size_val+k];
				}
				l[n*size_val+k] = (a[n*size_val+k] - sum)/u[k*size_val+k];
			}			
		}// end L loop
	}// end decomp loop
}

//Performs forward substitution for LU decomp inversion method
void forward_sub(complex *l, complex *b, complex *z, int size_val)// inputs l and b compute output z
{
	int n, m;
	z[0] = b[0];
	for(n=1; n<size_val; n++){
		for(m=0; m<n; m++){
			b[n] = b[n] - (l[n*size_val+m]*z[m]);
		}
		z[n] = b[n];
	}
}

//Performs backward substitution for LU decomp inversion method
void back_sub(complex *u, complex *z, complex *x, int size_val)// inputs u and z comput output x
{
	int n, m;
	x[(size_val-1)] = z[(size_val-1)]/u[(size_val*size_val-1)];
	for(n=(size_val-2); n>-1; n--){
		for(m=n+1; m<size_val; m++){
			z[n] = z[n] - (u[n*size_val+m]*x[m]); 
		}
		x[n] = z[n]/u[n*size_val+n];
	}
}

//Perform an inversion calculation using the LU decomposition as a basis
//Assumes all matrices prealloced in advance
//output_mat = inv(input_mat)
//size_val represents the square dimension -- 6x6 matrix = 6
void lu_matrix_inverse(complex *input_mat, complex *output_mat, int size_val)
{
	complex *l_mat, *u_mat, *b_vec, *z_vec, *x_vec;
	int sq_size,loop_val_x, loop_val_y;

	//Get overall size (save a multiply, save something)
	sq_size = size_val*size_val;

	//Allocate them on the heap, for now
	l_mat = (complex *)gl_malloc(sq_size*sizeof(complex));

	//Check it
	if (l_mat == NULL)
	{
		GL_THROW("link: error allocated space for LU matrix inversion");
		/*  TROUBLESHOOT
		While attempting to allocate space to perform a LU-decomp-based
		matrix inversion, an error occurred.  Please try again.  If the error
		persists, please submit you code and a bug report via the ticketing website.
		*/
	}

	u_mat = (complex *)gl_malloc(sq_size*sizeof(complex));

	//Check it
	if (u_mat == NULL)
	{
		GL_THROW("link: error allocated space for LU matrix inversion");
		//Defined above
	}

	b_vec = (complex *)gl_malloc(size_val*sizeof(complex));

	//Check it
	if (b_vec == NULL)
	{
		GL_THROW("link: error allocated space for LU matrix inversion");
		//Defined above
	}

	z_vec = (complex *)gl_malloc(size_val*sizeof(complex));

	//Check it
	if (z_vec == NULL)
	{
		GL_THROW("link: error allocated space for LU matrix inversion");
		//Defined above
	}

	x_vec = (complex *)gl_malloc(size_val*sizeof(complex));

	//Check it
	if (x_vec == NULL)
	{
		GL_THROW("link: error allocated space for LU matrix inversion");
		//Defined above
	}

	//Do the LU decomp
	lu_decomp(input_mat,l_mat,u_mat,size_val);

	//Loop for the inverse
	for (loop_val_x=0; loop_val_x<size_val; loop_val_x++)
	{
		//Form "solution" vector
		for (loop_val_y=0; loop_val_y<size_val; loop_val_y++)
		{
			if (loop_val_x == loop_val_y)
			{
				b_vec[loop_val_y] = complex(1.0,0.0);
			}
			else
			{
				b_vec[loop_val_y] = complex(0.0,0.0);
			}
		}//End solution vector formulation

		//Perform forward substitution
		forward_sub(l_mat,b_vec,z_vec,size_val);

		//Perform backward substitution
		back_sub(u_mat,z_vec,x_vec,size_val);

		//Now copy it in
		for (loop_val_y=0; loop_val_y<size_val; loop_val_y++)
		{
			output_mat[loop_val_y*size_val+loop_val_x] = x_vec[loop_val_y];
		}
	}//End column loop

	//Free up all the heap variables
	gl_free(l_mat);
	gl_free(u_mat);
	gl_free(b_vec);
	gl_free(z_vec);
	gl_free(x_vec);
}


/**@}*/

