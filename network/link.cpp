/** $Id: link.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file link.cpp
	@addtogroup link Network link (branch)
	@ingroup network
	
	The link object implements the general solution elements
	for branches using the Gauss-Seidel method.

	The following operations are performed on each branch during a bottom-up sync event.

	The effective admittance Y is calculated by including half of the line charging capacitance \p B [Kundur 1993, p. 258]:

		\f[ \widetilde{Y}_{eff} = \widetilde{Y} + \jmath \frac{B}{2} \f]
	
	The admittance coefficient is the inverse of the transformer turns ratio \p n, if any [Kundur 1993, p. 236]:
	
		\f[ c \leftarrow \frac{1}{n} \f]

	The effective line self-admittance is the product of the admittance coefficient and component admittance

		\f[ \widetilde{Y}_c = \widetilde{Y}_{eff} c \f]

	Add the self-admittance and the shunt admittances to the busses [Kundur 1993, p. 259]

		\f[ \Sigma \widetilde{Y}_{from} \leftarrow \Sigma \widetilde{Y}_{from} + \widetilde{Y}_c + \widetilde{Y}_c (c-1) \f]

		\f[ \Sigma \widetilde{Y}_{to} \leftarrow \Sigma \widetilde{Y}_{to} + \widetilde{Y}_c + \widetilde{Y}_{eff} (1-c) \f]

	Compute the line current injections on the busses

		\f[ \widetilde{I}_{from} \leftarrow \widetilde{V}_{to} Y c \f] \n

		\f[ \widetilde{I}_{to} \leftarrow \widetilde{V}_{from} Y c \f] \n

	Add the current injections to the busses

		\f[ \Sigma \widetilde{YV}_{from} \leftarrow \Sigma \widetilde{YV} - \widetilde{I}_{from} \f] \n

		\f[ \Sigma \widetilde{YV}_{to} \leftarrow \Sigma \widetilde{YV} - \widetilde{I}_{to} \f]

	Compute the line current (from -> to)

		\f[ \widetilde{I} \leftarrow \widetilde{I}_{from} - \widetilde{I}_{to} \f] \n

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"

//////////////////////////////////////////////////////////////////////////
// link CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

CLASS* link::oclass = NULL;
CLASS* link::pclass = NULL;
link *link::defaults = NULL;
CLASS *link_class = (NULL);
OBJECT *last_link = (NULL);

link::link(MODULE *mod) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		link_class = oclass = gl_register_class(mod,"link",sizeof(link),PC_BOTTOMUP|PC_UNSAFE_OVERRIDE_OMIT);
		if (oclass==NULL)
			throw "unable to register class link";
		else
			oclass->trl = TRL_STANDALONE;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_complex, "Y", PADDR(Y),
			PT_complex, "I", PADDR(I),
			PT_double, "B", PADDR(B),
			PT_object, "from", PADDR(from),
			PT_object, "to", PADDR(to),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
		Y = complex(0,0);
		B = 0.0;
		I = complex(0,0);
		from = NULL;
		to = NULL;
		turns_ratio = 1.0;
	}
}

int link::create() 
{
	memcpy(this,defaults,sizeof(*this));
	return 1;
}

int link::init(node *parent)
{
	node *f = OBJECTDATA(from,node);
	if (f==NULL)
		throw "from node not specified";
	f->attach(this);

	node *t = OBJECTDATA(to,node);
	if (t==NULL)
		throw "to node not specified";
	t->attach(this);

	// compute effective impedance
	Yeff = Y + complex(0,B/2);

	// off-nominal ratio [Kundur 1993, p.236]
	c = 1/turns_ratio; 

	// compute effective admittance
	Yc = Yeff*c;

	return 1;
}

TIMESTAMP link::sync(TIMESTAMP t0) 
{
	node *f = OBJECTDATA(from,node);
	node *t = OBJECTDATA(to,node);

	// Note: n!=1 <=> B==0
	// compute line currents (note from/to switched)
	complex Ifrom = t->V * Yc;
	complex Ito = f->V * Yc;

	// add to self admittance (contribution diagonal terms)
	// add to current injections (contribution to off-diagonal terms)
	complex Ys = Yc + Yc*(c-1);
	LOCK_OBJECT(from);
	f->Ys += Ys;
	f->YVs += Ifrom;
	UNLOCK_OBJECT(from);

	Ys = Yc + Yeff*(1-c);
	LOCK_OBJECT(to);
	t->YVs += Ito;
	t->Ys += Ys;
	UNLOCK_OBJECT(to);

	// compute current over line (from->to)
	I = Ito - Ifrom;


#ifdef _DEBUG
	// link debugging
	if (debug_link==1)
	{
		OBJECT* obj = OBJECTHDR(this);
		static int first=-1;
		if (first==-1) first = obj->id;
		if (obj->id==first)
		{
			printf("\n");
			printf("Link  From -> To             R        X        B        Y                 Ifrom             Ito               I               \n");
			printf("===== ====================== ======== ======== ======== ================= ================= ================= =================\n");
		}
		printf("%2d-%2d %-9.9s -> %-9.9s %+8.4f %+8.4f %+8.4f %+8.4f%+8.4fj %+8.4f%+8.4fj %+8.4f%+8.4fj %+8.4f%+8.4fj\n", 
			from->id, to->id, 
			from->name,to->name,
			(complex(1,0)/Y).Re(),(complex(1,0)/Y).Im(), B,
			Y.Re(), Y.Im(),
			Ifrom.Re(), Ifrom.Im(), 
			Ito.Re(), Ito.Im(), 
			I.Re(), I.Im());
	}
#endif // _DEBUG

	return TS_NEVER;
}

void link::apply_dV(OBJECT *source, complex dV)
{
	complex dI = dV * Y * c;
	OBJECT *obj;
	node *n;
	if (source==from)
		n = OBJECTDATA(obj=to,node);
	else if (source==to)
		n = OBJECTDATA(obj=from,node);
	else
		throw "apply_dV() - source is not valid";
	LOCKED(obj,n->YVs += dI);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: link
//////////////////////////////////////////////////////////////////////////

EXPORT int create_link(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(link_class);
	if (*obj!=NULL)
	{
		last_link = *obj;
		link *my = OBJECTDATA(*obj,link);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT TIMESTAMP sync_link(OBJECT *obj, TIMESTAMP t0)
{
	TIMESTAMP t1 = OBJECTDATA(obj,link)->sync(t0);
	obj->clock = t0;
	return t1;
}

EXPORT int init_link(OBJECT *obj)
{

	return OBJECTDATA(obj,link)->init(OBJECTDATA(obj->parent,node));
}


/**@}*/
