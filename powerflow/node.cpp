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

#include "node.h"
#include "transformer.h"

CLASS *node::oclass = NULL;
CLASS *node::pclass = NULL;
int iter_counter = 0;

unsigned int node::n = 0; 

node::node(MODULE *mod) : powerflow_object(mod)
{
	if(oclass == NULL)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod,"node",sizeof(node),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if(oclass == NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
        
		if(gl_publish_variable(oclass,
			PT_INHERIT, "powerflow_object",
			PT_enumeration, "bustype", PADDR(bustype),
				PT_KEYWORD, "PQ", PQ,
				PT_KEYWORD, "PV", PV,
				PT_KEYWORD, "SWING", SWING,
			PT_set, "busflags", PADDR(busflags),
				PT_KEYWORD, "HASSOURCE", NF_HASSOURCE,
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
			PT_complex, "power_A[W]", PADDR(powerA),
			PT_complex, "power_B[W]", PADDR(powerB),
			PT_complex, "power_C[W]", PADDR(powerC),
			PT_complex, "shunt_A[S]", PADDR(shuntA),
			PT_complex, "shunt_B[S]", PADDR(shuntB),
			PT_complex, "shunt_C[S]", PADDR(shuntC),

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

	n++;

	bustype = PQ;
	busflags = NF_HASSOURCE;
	reference_bus = NULL;
	nominal_voltage = 0.0;
	maximum_voltage_error = 0.0;
	frequency = nominal_frequency;
	fault_Z = 1e-6;

	memset(voltage,0,sizeof(voltage));
	memset(voltaged,0,sizeof(voltaged));
	memset(current,0,sizeof(current));
	memset(power,0,sizeof(power));
	memset(shunt,0,sizeof(shunt));

	return result;
}

int node::init(OBJECT *parent)
{
	if (solver_method==SM_GS)
	{
		OBJECT *obj = OBJECTHDR(this);

		// check that a parent is defined */
		if (obj->parent!=NULL)	//Has a parent, let's see if it is a node and link it up
		{
			node *parNode = OBJECTDATA(obj->parent,node);

			OBJECT *newlink = gl_create_object(link::oclass);
			link *test = OBJECTDATA(newlink,link);
			test->from = OBJECTHDR(parNode);
			test->to = OBJECTHDR(this);
			test->phases = parNode->phases;
			test->b_mat[0][0] = test->b_mat[1][1] = test->b_mat[2][2] = 1.0; //fault_Z;
			test->a_mat[0][0] = test->a_mat[1][1] = test->a_mat[2][2] = 1.0;
			test->A_mat[0][0] = test->A_mat[1][1] = test->A_mat[2][2] = 1.0;
			test->B_mat[0][0] = test->B_mat[1][1] = test->B_mat[2][2] = 1.0;
			test->d_mat[0][0] = test->d_mat[1][1] = test->d_mat[2][2] = 1.0;
			test->voltage_ratio = 1.0;

			//int tester = 0;
			//tester++;

			//Give us no parent now, and let it go to SWING
			obj->parent=NULL;
		}
		
		if (obj->parent==NULL) // need to find a swing bus to be this node's parent
		{
			FINDLIST *buslist = gl_find_objects(FL_NEW,FT_CLASS,SAME,"node",AND,FT_PROPERTY,"bustype",SAME,"SWING",FT_END);
			if (buslist==NULL)
				throw "no swing bus found";
			if (buslist->hit_count>1)
				throw "more than one swing bus found, you must specify which is this node's parent";
			gl_set_parent(obj,gl_find_next(buslist,NULL));
		}

		/* Make sure we aren't the swing bus */
		if (this->bustype!=SWING)
		{
			/* still no swing bus found */
			if (obj->parent==NULL)
				throw "no swing bus found or specified";

			/* check that the parent is a node and that it's a swing bus */
			if (!gl_object_isa(obj->parent,"node"))
				throw "node parent is not a node itself";
			else 
			{
				node *pNode = OBJECTDATA(obj->parent,node);

				if (pNode->bustype!=SWING)	//Originally checks to see if is swing bus...nto sure how to handle this yet
					throw "node parent is not a swing bus";
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

	/* initialize the powerflow base object */
	int result = powerflow_object::init(parent);

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

	/* no nominal voltage */
	if (nominal_voltage==0)
		throw "nominal voltage is not specified";

	/* make sure the sync voltage limit is positive */
	if (maximum_voltage_error<0)
		throw "negative maximum_voltage_error is invalid";

	/* set sync_v_limit to default if needed */
	if (maximum_voltage_error==0.0)
		maximum_voltage_error =  nominal_voltage * default_maximum_voltage_error;

	/* make sure fault_Z is positive */
	if (fault_Z<=0)
		throw "negative fault impedance is invalid";

	/* check that nominal voltage is set */
	if (nominal_voltage<=0)
		throw "nominal_voltage is not set";

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

	if (has_phase(PHASE_A) & has_phase(PHASE_B) & has_phase(PHASE_C))
	{
		if (voltage[0] == 0)
		{
			voltage[0].SetPolar(nominal_voltage,0.0);
		}
		if (voltage[1] == 0)
		{
			voltage[1].SetPolar(nominal_voltage,120.0);
		}
		if (voltage[2] == 0)
		{
			voltage[2].SetPolar(nominal_voltage,-120.0);
		}
	}


	if (has_phase(PHASE_D) & voltageAB==0)
	{	// compute 3phase voltage differences
		voltageAB = voltageA - voltageB;
		voltageBC = voltageB - voltageC;
		voltageCA = voltageC - voltageA;
	}

	return result;
}

TIMESTAMP node::presync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t1 = powerflow_object::presync(t0); 

	if (solver_method==SM_FBS)
	{
		/* reset the accumulators for the coming pass (don't do this on bottom-up pass or else tape won't work */
		//current[0] = current[1] = current[2] = complex(0,0);
		//power[0] = power[1] = power[2] = complex(0,0);
		//shunt[0] = shunt[1] = shunt[2] = complex(0,0);

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

TIMESTAMP node::sync(TIMESTAMP t0)
{
	TIMESTAMP t1 = powerflow_object::sync(t0);
	OBJECT *obj = OBJECTHDR(this);

	switch (solver_method)
	{
	case SM_FBS:
		{
		// add power and shunt to current injection
		if (phases&PHASE_S)
		{	// Split phase
			current_inj[0] += (voltage1.IsZero() || (power1.IsZero() && shunt1.IsZero())) ? current1 : current1 + ~(power1/voltage1) + voltage1*shunt1;
			current_inj[1] += (voltage2.IsZero() || (power2.IsZero() && shunt2.IsZero())) ? current2 : current2 + ~(power2/voltage2) + voltage2*shunt2;
			current_inj[2] += -(current_inj[0]+current_inj[1]);
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
			
			current_inj[0] += (voltageAB.IsZero() || (powerA.IsZero() && delta_shunt_curr[0].IsZero())) ? currentA : currentA + power_current[0] + delta_shunt_curr[0];
			current_inj[1] += (voltageBC.IsZero() || (powerB.IsZero() && delta_shunt_curr[1].IsZero())) ? currentB : currentB + power_current[1] + delta_shunt_curr[1];
			current_inj[2] += (voltageCA.IsZero() || (powerC.IsZero() && delta_shunt_curr[2].IsZero())) ? currentC : currentC + power_current[2] + delta_shunt_curr[2];
		}
		else 
		{	// 'WYE' connected load
			current_inj[0] += (voltageA.IsZero() || (powerA.IsZero() && shuntA.IsZero())) ? currentA : currentA + ~(powerA/voltageA) + voltageA*shuntA;
			current_inj[1] += (voltageB.IsZero() || (powerB.IsZero() && shuntB.IsZero())) ? currentB : currentB + ~(powerB/voltageB) + voltageB*shuntB;
			current_inj[2] += (voltageC.IsZero() || (powerC.IsZero() && shuntC.IsZero())) ? currentC : currentC + ~(powerC/voltageC) + voltageC*shuntC;
		}

#ifdef SUPPORT_OUTAGES
	if (is_open_any())
		throw "unable to handle node open phase condition";

	if (is_contact_any())
	{
		/* phase-phase contact */
		if (is_contact(PHASE_A|PHASE_B|PHASE_C))
			voltage_A = voltage_B = voltage_C = (voltage_A + voltage_B + voltage_C)/3;
		else if (is_contact(PHASE_A|PHASE_B))
			voltage_A = voltage_B = (voltage_A + voltage_B)/2;
		else if (is_contact(PHASE_B|PHASE_C))
			voltage_B = voltage_C = (voltage_B + voltage_C)/2;
		else if (is_contact(PHASE_A|PHASE_C))
			voltage_A = voltage_C = (voltage_A + voltage_C)/2;

		/* phase-neutral/ground contact */
		if (is_contact(PHASE_A|PHASE_N) || is_contact(PHASE_A|GROUND))
			voltage_A /= 2;
		if (is_contact(PHASE_B|PHASE_N) || is_contact(PHASE_B|GROUND))
			voltage_B /= 2;
		if (is_contact(PHASE_C|PHASE_N) || is_contact(PHASE_C|GROUND))
			voltage_C /= 2;
	}
#endif

		// if the parent object is another node
		if (obj->parent!=NULL && gl_object_isa(obj->parent,"node"))
		{
			// add the injections on this node to the parent
			node *pNode = OBJECTDATA(obj->parent,node);
			LOCKED(obj->parent, pNode->current_inj[0] += current_inj[0]);
			LOCKED(obj->parent, pNode->current_inj[1] += current_inj[1]);
			LOCKED(obj->parent, pNode->current_inj[2] += current_inj[2]);
		}

		break;
		}
	case SM_GS:
		{
		OBJECT *hdr = OBJECTHDR(this);
		node *swing = hdr->parent?OBJECTDATA(hdr->parent,node):this;
		complex AccelFactor;
		complex Vnew[3];
		complex dV[3];
		LINKCONNECTED *linktable;
		OBJECT *linkupdate;
		link *linkref;
		char phasespresent;
		
		//Determine which phases are present for later
		phasespresent = 8*has_phase(PHASE_D)+4*has_phase(PHASE_A) + 2*has_phase(PHASE_B) + has_phase(PHASE_C);

		//Assign acceleration factor to a complex (it's picky)
		AccelFactor=acceleration_factor;
		
		//init dV as zero
		dV[0] = dV[1] = dV[2] = complex(0,0);

		switch (bustype) {
			case PV:	//PV bus only supports Y-Y connections at this time.  Easier to handle the oddity of power that way.
				{
				//Check for NaNs (single phase problem)
				if ((phasespresent!=7) & (phasespresent!=15) & (phasespresent!=8))	//Not three phase or delta
				{
					//Calculate reactive power - apply GS update (do phase iteratively)
					power[0].Im() = ((~voltage[0]*(Ys[0][0]*voltage[0]+Ys[0][1]*voltage[1]+Ys[0][2]*voltage[2]-YVs[0]+current[0]+voltage[0]*shunt[0])).Im());
					Vnew[0] = (-(~power[0]/~voltage[0]+current[0]+voltage[0]*shunt[0])+YVs[0]-voltage[1]*Ys[0][1]-voltage[2]*Ys[0][2])/Ys[0][0];
					Vnew[0] = (isnan(Vnew[0].Re())) ? complex(0,0) : Vnew[0];

					power[1].Im() = ((~voltage[1]*(Ys[1][0]*Vnew[0]+Ys[1][1]*voltage[1]+Ys[1][2]*voltage[2]-YVs[1]+current[1]+voltage[1]*shunt[1])).Im());
					Vnew[1] = (-(~power[1]/~voltage[1]+current[1]+voltage[1]*shunt[1])+YVs[1]-Vnew[0]*Ys[1][0]-voltage[2]*Ys[1][2])/Ys[1][1];
					Vnew[1] = (isnan(Vnew[1].Re())) ? complex(0,0) : Vnew[1];

					power[2].Im() = ((~voltage[2]*(Ys[2][0]*Vnew[0]+Ys[2][1]*Vnew[1]+Ys[2][2]*voltage[2]-YVs[2]+current[2]+voltage[2]*shunt[2])).Im());
					Vnew[2] = (-(~power[2]/~voltage[2]+current[2]+voltage[2]*shunt[2])+YVs[2]-Vnew[0]*Ys[2][0]-Vnew[1]*Ys[2][1])/Ys[2][2];
					Vnew[2] = (isnan(Vnew[2].Re())) ? complex(0,0) : Vnew[2];
				}
				else	//Three phase
				{
					//Calculate reactive power - apply GS update (do phase iteratively)
					power[0].Im() = ((~voltage[0]*(Ys[0][0]*voltage[0]+Ys[0][1]*voltage[1]+Ys[0][2]*voltage[2]-YVs[0]+current[0]+voltage[0]*shunt[0])).Im());
					Vnew[0] = (-(~power[0]/~voltage[0]+current[0]+voltage[0]*shunt[0])+YVs[0]-voltage[1]*Ys[0][1]-voltage[2]*Ys[0][2])/Ys[0][0];

					power[1].Im() = ((~voltage[1]*(Ys[1][0]*Vnew[0]+Ys[1][1]*voltage[1]+Ys[1][2]*voltage[2]-YVs[1]+current[1]+voltage[1]*shunt[1])).Im());
					Vnew[1] = (-(~power[1]/~voltage[1]+current[1]+voltage[1]*shunt[1])+YVs[1]-Vnew[0]*Ys[1][0]-voltage[2]*Ys[1][2])/Ys[1][1];

					power[2].Im() = ((~voltage[2]*(Ys[2][0]*Vnew[0]+Ys[2][1]*Vnew[1]+Ys[2][2]*voltage[2]-YVs[2]+current[2]+voltage[2]*shunt[2])).Im());
					Vnew[2] = (-(~power[2]/~voltage[2]+current[2]+voltage[2]*shunt[2])+YVs[2]-Vnew[0]*Ys[2][0]-Vnew[1]*Ys[2][1])/Ys[2][2];
				}

				//Apply correction - only use angles (magnitude is unaffected)
				Vnew[0].SetPolar(voltage[0].Mag(),Vnew[0].Arg());
				Vnew[1].SetPolar(voltage[1].Mag(),Vnew[1].Arg());
				Vnew[2].SetPolar(voltage[2].Mag(),Vnew[2].Arg());

				//Find step amount for convergence check
				dV[0]=Vnew[0]-voltage[0];
				dV[1]=Vnew[1]-voltage[1];
				dV[2]=Vnew[2]-voltage[2];

				//Apply update
				voltage[0]=voltage[0]+AccelFactor*dV[0];
				voltage[1]=voltage[1]+AccelFactor*dV[1];
				voltage[2]=voltage[2]+AccelFactor*dV[2];

				break;
				}
			case PQ:
				{
				//Check for NaNs (single phase problem)
				if ((phasespresent!=7) & (phasespresent!=15) & (phasespresent!=8))	//Not three phase or delta
				{
					//Update node voltage
					Vnew[0] = (-(~power[0]/~voltage[0]+current[0]+voltage[0]*shunt[0])+YVs[0]-voltage[1]*Ys[0][1]-voltage[2]*Ys[0][2])/Ys[0][0];
					Vnew[0] = (isnan(Vnew[0].Re())) ? complex(0,0) : Vnew[0];

					Vnew[1] = (-(~power[1]/~voltage[1]+current[1]+voltage[1]*shunt[1])+YVs[1]-Vnew[0]*Ys[1][0]-voltage[2]*Ys[1][2])/Ys[1][1];
					Vnew[1] = (isnan(Vnew[1].Re())) ? complex(0,0) : Vnew[1];

					Vnew[2] = (-(~power[2]/~voltage[2]+current[1]+voltage[2]*shunt[2])+YVs[2]-Vnew[0]*Ys[2][0]-Vnew[1]*Ys[2][1])/Ys[2][2];
					Vnew[2] = (isnan(Vnew[2].Re())) ? complex(0,0) : Vnew[2];
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

						//Update node voltage
						Vnew[0] = (-(power_current[0]+current[0]+delta_shunt_curr[0])+YVs[0]-voltage[1]*Ys[0][1]-voltage[2]*Ys[0][2])/Ys[0][0];
						Vnew[1] = (-(power_current[1]+current[1]+delta_shunt_curr[1])+YVs[1]-Vnew[0]*Ys[1][0]-voltage[2]*Ys[1][2])/Ys[1][1];
						Vnew[2] = (-(power_current[2]+current[2]+delta_shunt_curr[2])+YVs[2]-Vnew[0]*Ys[2][0]-Vnew[1]*Ys[2][1])/Ys[2][2];
					}
					else
					{
						//Update node voltage
						Vnew[0] = (-(~power[0]/~voltage[0]+current[0]+voltage[0]*shunt[0])+YVs[0]-voltage[1]*Ys[0][1]-voltage[2]*Ys[0][2])/Ys[0][0];
						Vnew[1] = (-(~power[1]/~voltage[1]+current[1]+voltage[1]*shunt[1])+YVs[1]-Vnew[0]*Ys[1][0]-voltage[2]*Ys[1][2])/Ys[1][1];
						Vnew[2] = (-(~power[2]/~voltage[2]+current[2]+voltage[2]*shunt[2])+YVs[2]-Vnew[0]*Ys[2][0]-Vnew[1]*Ys[2][1])/Ys[2][2];

					}
				}

				//Find step amount for convergence check
				dV[0]=Vnew[0]-voltage[0];
				dV[1]=Vnew[1]-voltage[1];
				dV[2]=Vnew[2]-voltage[2];

				//Apply update
				voltage[0]=voltage[0]+AccelFactor*dV[0];
				voltage[1]=voltage[1]+AccelFactor*dV[1];
				voltage[2]=voltage[2]+AccelFactor*dV[2];

				break;
				}
			case SWING:
				{
				//Nothing to do here :(
					iter_counter++;

					//if ((iter_counter % 1000)==1)
					//{
					//	printf("\nIteration %d\n",iter_counter);
					//}
				break;
				}
			default:
				{
				/* unknown type fails */
				gl_error("invalid bus type");
				return TS_ZERO;
				}
		}

		//Update YVs terms for connected nodes - exclude swing bus (or if nothing changed)
		if ((dV[0].Mag()!=0) | (dV[1].Mag()!=0) | (dV[2].Mag()!=0))
		{
			linktable = &nodelinks;

			while (linktable->next!=NULL)		//Parse through the linked list
			{
				linktable = linktable->next;

				linkupdate = linktable->connectedlink;

				linkref = OBJECTDATA(linkupdate,link);
				linkref->UpdateYVs(OBJECTHDR(this), dV);	//Call link YVs updating function
			}
		}

		if ((dV[0].Mag()>maximum_voltage_error) | (dV[1].Mag()>maximum_voltage_error) | (dV[2].Mag()>maximum_voltage_error))
			return t0;	//No convergence, loop again
		else
			return TS_NEVER;	//converged, no more loops needed
		break;
		}
	default:
		throw "unsupported solver method";
		break;
	}
	return t1;
}

TIMESTAMP node::postsync(TIMESTAMP t0)
{
	TIMESTAMP t1 = powerflow_object::postsync(t0);
	OBJECT *obj = OBJECTHDR(this);
	char phasespresent;

	//Determine which phases are present for later
	phasespresent = 8*has_phase(PHASE_D)+4*has_phase(PHASE_A) + 2*has_phase(PHASE_B) + has_phase(PHASE_C);

#ifdef SUPPORT_OUTAGES
	if (is_contact_any())
	{
		complex dVAB = voltage_A - voltage_B;
		complex dVBC = voltage_B - voltage_C;
		complex dVCA = voltage_C - voltage_A;

		/* phase-phase contact */
		if (is_contact(PHASE_A|PHASE_B|PHASE_C))
			/** @todo calculate three-way contact fault current */
			throw "three-way contact not supported yet";
		else if (is_contact(PHASE_A|PHASE_B))
			phaseA_I_in = - phaseB_I_in = dVAB/fault_Z;
		else if (is_contact(PHASE_B|PHASE_C))
			phaseB_I_in = - phaseC_I_in = dVBC/fault_Z;
		else if (is_contact(PHASE_A|PHASE_C))
			phaseC_I_in = - phaseA_I_in = dVCA/fault_Z;

		/* phase-neutral/ground contact */
		if (is_contact(PHASE_A|PHASE_N) || is_contact(PHASE_A|GROUND))
			phaseA_I_in = voltage_A / fault_Z;
		if (is_contact(PHASE_B|PHASE_N) || is_contact(PHASE_B|GROUND))
			phaseB_I_in = voltage_B / fault_Z;
		if (is_contact(PHASE_C|PHASE_N) || is_contact(PHASE_C|GROUND))
			phaseC_I_in = voltage_C / fault_Z;
	}

	/* record the power in for posterity */
	kva_in = (voltage_A*~phaseA_I_in + voltage_B*~phaseB_I_in + voltage_C*~phaseC_I_in)/1000;

#endif

	/* check for voltage control requirement */
	if (require_voltage_control==TRUE)
	{
		/* PQ bus must have a source */
		if ((busflags&NF_HASSOURCE)==0 && bustype==PQ)
			voltage[0] = voltage[1] = voltage[2] = complex(0,0);
	}

	if (solver_method==SM_FBS)
	{
		// if the parent object is a node
		if (obj->parent!=NULL && gl_object_isa(obj->parent,"node"))
		{
			// copy the voltage from the parent
			node *pNode = OBJECTDATA(obj->parent,node);
			LOCKED(obj, voltage[0] = pNode->voltage[0]);
			LOCKED(obj, voltage[1] = pNode->voltage[1]);
			LOCKED(obj, voltage[2] = pNode->voltage[2]);
		}
	}
	else //GS items - solver and misc. related
	{
		LINKCONNECTED *linkscanner;	//Temp variable for connected links list - used to find currents flowing out of node
		OBJECT *currlinkobj;	//Temp variable for link of interest
		link *currlink;			//temp Reference to link of interest

		// Commented out - not needed if only do initial calculation on new timesteps (screws things up)
		/*
		//Zero out admittance matrix - get ready for next passes
		Ys[0][0] = Ys[0][1] = Ys[0][2] = complex(0,0);
		Ys[1][0] = Ys[1][1] = Ys[1][2] = complex(0,0);
		Ys[2][0] = Ys[2][1] = Ys[2][2] = complex(0,0);

		//Zero out current accummulator
		YVs[0] = complex(0,0);
		YVs[1] = complex(0,0);
		YVs[2] = complex(0,0);
		*/

		//Zero current for below calcuations.  May mess with tape (will have values at end of Postsync)
		current_inj[0] = current_inj[1] = current_inj[2] = complex(0,0);

		//Calculate current if it has one
		if ((bustype==PQ) | (bustype==PV)) //PQ and PV busses need current updates
		{
			if ((phasespresent!=7) & (phasespresent!=15) & (phasespresent!=8))	//Not three phase or delta
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
					delta_current[0]= ~(power[0]/voltageAB);
					delta_current[1]= ~(power[1]/voltageBC);
					delta_current[2]= ~(power[2]/voltageCA);

					current_inj[0] += delta_shunt[0]-delta_shunt[2];	//calculate "load" current (line current) - PQZ
					current_inj[1] += delta_shunt[1]-delta_shunt[0];
					current_inj[2] += delta_shunt[2]-delta_shunt[1];

					current_inj[0] +=delta_current[0]-delta_current[2];	//Calculate "load" current (line current) - PQP
					current_inj[1] +=delta_current[1]-delta_current[0];
					current_inj[2] +=delta_current[2]-delta_current[1];
				}
				else					//Wye connection
				{
					current_inj[0] +=~(power[0]/voltage[0]);			//PQP needs power converted to current
					current_inj[1] +=~(power[1]/voltage[1]);
					current_inj[2] +=~(power[2]/voltage[2]);

					current_inj[0] +=voltage[0]*shunt[0];			//PQZ needs load currents calculated as well
					current_inj[1] +=voltage[1]*shunt[1];
					current_inj[2] +=voltage[2]*shunt[2];

				}

				current_inj[0] +=current[0];		//Update load current values if PQI
				current_inj[1] +=current[1];		
				current_inj[2] +=current[2];
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
			
			if (((currlink->from)->id) == (OBJECTHDR(this)->id))	//see if we are the from end
			{
				if  ((currlink->power_in) > (currlink->power_out))	//Make sure current flowing right direction
				{
					current_inj[0] += currlink->current_in[0];
					current_inj[1] += currlink->current_in[1];
					current_inj[2] += currlink->current_in[2];
				}
			}
			else						//must be the to link then
			{
				if ((currlink->power_in) < (currlink->power_out))	//Current is reversed, so this is a from to
				{
					current_inj[0] -= currlink->current_out[0];
					current_inj[1] -= currlink->current_out[1];
					current_inj[2] -= currlink->current_out[2];
				}
			}
		}
	}

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
			return t0;
		}
	}

	/* the solution is satisfactory */
	return t1;
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
	}
	catch (char *msg)
	{
		gl_error("create_node: %s", msg);
	}
	return 0;
}



/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_node(OBJECT *obj)
{
	node *my = OBJECTDATA(obj,node);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		GL_THROW("%s (node:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
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
	node *pObj = OBJECTDATA(obj,node);
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
		gl_error("node %s (%s:%d): %s", obj->name, obj->oclass->name, obj->id, msg);
	}
	catch (...)
	{
		gl_error("node %s (%s:%d): unknown exception", obj->name, obj->oclass->name, obj->id);
	}
	return TS_INVALID; /* stop the clock */
}

/**
* Attachlink is called by link objects during their creation.  It creates a linked list
* of links that are attached to the current node.
*
* @param obj the link that has an attachment to this node
*/
LINKCONNECTED *node::attachlink(OBJECT *obj) ///< object to attach
{
	// construct and id the new circuit
	LINKCONNECTED *lconnected = new LINKCONNECTED;
	if (lconnected==NULL)
	{
		gl_error("memory allocation failure");
		return 0;
		/* Note ~ this returns a null pointer, but iff the malloc fails.  If
		 * that happens, we're already in SEGFAULT sort of bad territory. */
	}
	
	lconnected->next = nodelinks.next;

	// attach to node list
	nodelinks.next = lconnected;

	// attach the link object identifier
	lconnected->connectedlink = obj;

	return 0;
}

EXPORT int isa_node(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,node)->isa(classname);
}

/**@}*/
