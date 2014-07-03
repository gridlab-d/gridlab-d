/** $Id: node.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file node.cpp
	@addtogroup node Network node (bus)
	@ingroup network
	
	The node object implements the general node solution elements
	of the Gauss-Seidel solver.

	The types of nodes are supported
	- \p PQ buses are nodes that have both constant real and reactive power injections
	- \p PV buses are for nodes that have constant real power injection but can control reactive power injection
	- \p SWING buses are nodes that are designated to absorb the residual error and for generators that can control both real and reactive power injections

	Three bus types are supported, \e PQ, \e PV, and \e SWING:

	@section PQ PQ bus

	The \e PQ bus is the most commonly found bus type in electric network models.  PQ buses are nodes
	where both the real power (\e P) and reactive power (\e Q) are given.  In these cases, the
	updated voltage at for a node is found from an existing (non-zero) voltage using

	\f[ \widetilde V \leftarrow \frac{P - \jmath Q}{\widetilde{YY} \left( \widetilde V^* - \Sigma \widetilde{YV} \right)} \f]

	where

	\f[ \widetilde{YY} \leftarrow \Sigma \widetilde Y + G + \jmath B \f]

	@section PV PV bus

	The \e PV bus is the next most common bus type in electric network models.  PV buses are nodes
	where the real power (\e P) is given, but the reactive power (\e Q) must be determined at each iteration.
	In these cases, the updated voltage for a node is found by calculating

	\f[ Q \leftarrow - \Im \left[ \widetilde V^* \left( \widetilde{YY} \widetilde V + \Sigma \widetilde{YV} \right) \right] \f]

	If \e Q exceeds the \e Q limits [\p Qmin, \p Qmax] then the angle is fixed to the correspond limit and the bus
	is solved as a \e PQ bus.

	@section SWING SWING bus

	The \e SWING bus occurs at least one in any given island of a network models.  Large models may have
	more the one \e SWING bus, particularly if areas of the network are only lightly coupled by relatively
	high impedance links.  \e SWING bus nodes are nodes where both the real power (\e P) and reactive power (\e Q)
	must be determined at each iteration.  In these cases, the updated voltage for a node is found by

	\f[ \widetilde S \leftarrow \left[ \widetilde V^* \left( \widetilde{YY} \widetilde V + \Sigma \widetilde{YV} \right) \right] \f]

	where

	\f[ P + \jmath Q \leftarrow \widetilde S \f]
 @{
 **/

// #define HYBRID // enables hybrid solver that can be used to perform state estimation (DPC 10/07: this is not fully functional)

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"



//////////////////////////////////////////////////////////////////////////
// node CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

CLASS* node::oclass = NULL;
CLASS* node::pclass = NULL;
node *node::defaults = NULL;

CLASS *node_class = (NULL);
OBJECT *last_node = (NULL);

node::node(MODULE *mod) 
{

	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		node_class = oclass = gl_register_class(mod,"node",sizeof(node),PC_PRETOPDOWN|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if (oclass==NULL)
			throw "unable to register class node";
		else
			oclass->trl = TRL_STANDALONE;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_complex, "V", PADDR(V),
			PT_complex, "S", PADDR(S),
			PT_double, "G", PADDR(G),
			PT_double, "B", PADDR(B),
			PT_double, "Qmax_MVAR", PADDR(Qmax_MVAR),
			PT_double, "Qmin_MVAR", PADDR(Qmin_MVAR),
			PT_enumeration,"type",PADDR(type),
				PT_KEYWORD,"PQ",PQ,
				PT_KEYWORD,"PQV",PQV,
				PT_KEYWORD,"PV",PV,
				PT_KEYWORD,"SWING",SWING,
			PT_int16, "flow_area_num", PADDR(flow_area_num),
			PT_double, "base_kV", PADDR(base_kV),
#ifdef HYBRID
			PT_complex, "Vobs", PADDR(Vobs),
			PT_double, "Vstdev", PADDR(Vstdev),
#endif
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		memset(this,0,sizeof(node));
		defaults = this;
		type = PQ;
		V = complex(1,0,A);
		S = complex(0,0,J);
		flow_area_num=1;
		loss_zone_num=1;
		Vobs = complex(0,0,A); /* default observation is 0+0j */
	}
}

node::~node()
{
	while (linklist!=NULL)
	{
		LINKLIST *next = linklist->next;
		delete linklist;
		linklist = next;
	}
}

int node::create() 
{
	memcpy(this,defaults,sizeof(*this));
	return 1;
}

void node::attach(link *pLink)
{
	LINKLIST *item = new LINKLIST;
	if (item==NULL)
		throw "node::attach() - memory allocation failed";
	item->data = pLink;
	item->next = linklist;
	linklist = item;
}

int node::init(OBJECT *parent)
{
	// check that parent is swing bus
	OBJECT *hdr = OBJECTHDR(this);
	node *swing = parent?OBJECTDATA(parent,node):this;
	OBJECT *swing_hdr = OBJECTHDR(swing);
	if (swing_hdr->oclass!=hdr->oclass || swing->type!=SWING)
	{
		gl_error("node %s:%d parent is not a swing bus",hdr->oclass->name,hdr->id);
		return 0;
	}
#ifdef HYBRID
	// add observation residual
	if (Vstdev>0)
		swing->add_obs_residual(this);

	// add injection error
	swing->add_inj_residual(this);
#endif

	YVs=complex(0,0);
	Ys=complex(0,0);
	return 1;
}

#ifdef HYBRID
void node::add_inj_residual(node *pNode)
{
	if (pNode!=this)
	{
		pNode->Sr2 = (~(~pNode->V*((pNode->Ys + complex(pNode->G,pNode->B))*pNode->V + pNode->YVs)) - pNode->S).Mag();
		Sr2 += pNode->Sr2*pNode->Sr2;
		n_inj++;
	}
}

void node::del_inj_residual(node *pNode)
{
	if (pNode!=this)
	{
		Sr2 -= pNode->Sr2*pNode->Sr2;
		n_inj--;
	}
}

double node::get_inj_residual(void) const
{
	if (Sr2>0 && n_inj>0)
		return sqrt(Sr2/n_inj);
	else
		return Sr2;
}

void node::add_obs_residual(node *pNode)
{
	double r = (pNode->V - pNode->Vobs).Mag() / pNode->Vstdev;
	r*=r;
	if (pNode!=this)
	{
		pNode->r2 = r;
		pNode->n_obs = 1;
	}
	r2 += r;
	n_obs++;
}

void node::del_obs_residual(node *pNode)
{
	if (pNode!=this)
		r2 -= pNode->r2;
	else
	{
		double r = (pNode->V - pNode->Vobs).Mag() / pNode->Vstdev;
		r2 -= r*r;
	}
	n_obs--;
}

double node::get_obs_residual(void) const
{
	if (r2>0 && n_obs>0)
		return sqrt(r2/n_obs);
	else
		return r2;
}

double node::get_obs_probability(void) const
{
	if (r2<0)
		return 1;
	double pr = exp(-0.5*r2); /// @todo there should be a 1/sqrt(2*pi) coeff on the observability probability, yet it works. (network, low priority)
	if (pr>1)
		gl_warning("node:%d observation probability exceeds 1!", OBJECTHDR(this)->id);
	return pr;
}
#endif

TIMESTAMP node::presync(TIMESTAMP t0)
{
	// reset the accumulators
	Ys = 0;
	YVs = 0;
	return TS_NEVER;
}

TIMESTAMP node::postsync(TIMESTAMP t0) 
{
	OBJECT *hdr = OBJECTHDR(this);
	node *swing = hdr->parent?OBJECTDATA(hdr->parent,node):this;
	complex dV(0.0);
	complex YY = Ys + complex(G,B);
	// copy values that might get updated while we work on this object
	complex old_YVs = YVs;
#ifdef HYBRID
	swing->del_inj_residual(this);
#endif
	if (!YY.IsZero() || type==SWING)
	{
		switch (type) {
		case PV:
			S.Im() = ((~V*(YY*V-old_YVs)).Im());
			if (Qmin_MVAR<Qmax_MVAR && S.Im()<Qmin_MVAR) 
				S.Im() = Qmin_MVAR;
			else if (Qmax_MVAR>Qmin_MVAR && S.Im()>Qmax_MVAR) 
				S.Im() = Qmax_MVAR;
			//else
			{
				complex Vnew = (-(~S/~V) + old_YVs) / YY;
				Vnew.SetPolar(V.Mag(),Vnew.Arg());
#ifdef HYBRID
				if (Vstdev>0)
				{
					double pr = swing->get_obs_probability();
					swing->del_obs_residual(this);
					dV = Vobs*(1-pr) + (Vnew)*pr - V;
					V += dV;
					swing->add_obs_residual(this);
				}
				else
#endif
				{
					dV = Vnew - V;
					V = Vnew;
				}
				break;
			}
			/* continue with PQ solution */
		case PQ:
			if (!V.IsZero())
			{
				complex Vnew = (-(~S/~V) + old_YVs) / YY;
#ifdef HYBRID
				if (Vstdev>0) // need to consider observation
				{
					double pr = swing->get_obs_probability();
					swing->del_obs_residual(this);
					dV = Vobs*(1-pr) + (Vnew)*pr - V;
					V += dV;
					swing->add_obs_residual(this);
				}
				else // no observation 
#endif
				{
					dV = (Vnew - V)*acceleration_factor;
					V += dV;
				}
				V.Notation() = A;
			}
			break;
		case SWING:
			S = ~(~V*(YY*V - YVs));
			S.Notation() = J;
			break;
		default:
			/* unknown type fails */
			gl_error("invalid bus type");
			return TS_ZERO;
		}
	}
#ifdef HYBRID
	swing->add_inj_residual(this);
#endif

#ifdef _DEBUG
	// node debugging
	if (debug_node>0)
	{
		OBJECT* obj = OBJECTHDR(this);
		static int first=-1;
		if (first==-1) first = obj->id;
		if (obj->id==first)
		{
			printf("\n");
			printf("Node           Type  V                 Vobs              Stdev    Power             G        B        dV       Pr{Vobs} r2     Sr2\n");
			printf("============== ===== ================= ================= ======== ================= ======== ======== ======== ======== ====== ======\n");
		}
		if (((debug_node&1)==1 && dV.Mag()>convergence_limit )  // only on dV
#ifdef HYBRID
			|| ((debug_node&2)==2 && Vstdev>0 ) // only on observation
			|| ((debug_node&4)==4 && get_inj_residual()>0.001)// non-zero power residual
#endif
			)
		{
			printf("%2d (%-9.9s) %5s %+8.4f%+8.3fd %+8.4f%+8.3fd %8.5f %+8.4f%+8.4fj %+8.5f ", 
				obj->id, obj->name, type==SWING?"SWING":(type==PQ?"PQ   ":"PV"),
				V.Mag(),V.Arg()*180/3.1416, 
				Vobs.Mag(), Vobs.Arg()*180/3.1416, Vstdev,
				S.Re(), S.Im(), 
				G, B,
				dV.Mag());
#ifdef HYBRID
			printf("%+8.5f %8.5f ", get_obs_probability(), r2);
			if (Vstdev>0)
				printf("%8.5f %6.3f %6.3f\n", get_obs_probability(), r2, get_inj_residual());
			else
				printf("   --      --   %6.3f\n",get_inj_residual());
#else
			printf("\n");
#endif
		}
	}
#endif // _DEBUG

	// send dV through all links
	LINKLIST *item;
	for (item=linklist; item!=NULL; item=item->next)
		item->data->apply_dV(hdr,dV);

	if (dV.Mag()>convergence_limit)
		return t0; /* did not converge, hold the clock */
	else
		return TS_NEVER; /* converged, no further updates needed */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: node
//////////////////////////////////////////////////////////////////////////

EXPORT int create_node(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(node_class);
	if (*obj!=NULL)
	{
		last_node = *obj;
		node *my = OBJECTDATA(*obj,node);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_node(OBJECT *obj)
{
	return OBJECTDATA(obj,node)->init(obj->parent);
}

EXPORT TIMESTAMP sync_node(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	TIMESTAMP t1;
	try {
		switch (pass)
		{
		case PC_PRETOPDOWN:
			return OBJECTDATA(obj,node)->presync(t0);
		case PC_POSTTOPDOWN:
			t1 = OBJECTDATA(obj,node)->postsync(t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid passconfig";
		}
	}
	catch (char *msg) 
	{
		gl_error("%s(%s:%d): %s", obj->name?obj->name:"(anon)", obj->oclass->name, obj->id, msg);
		return TS_INVALID;
	}
}

/**@}*/
