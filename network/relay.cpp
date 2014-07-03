/** $Id: relay.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file relay.cpp
	@author David Chassin
	@addtogroup relay Relay
	@ingroup network

	The primary relay time is determined by	\f$T_p=TimeDial\left(A_p+\frac{B_p}{M_p^{C_p}-1}\right)\f$.
	
	The reset time is determined by	\f$T_r=TimeDial\left(\frac{B_r}{1-M_r^{C_r}}\right)\f$.

	Five relay curves are supported:
	\f[
		\begin{array}{ c c c c c c }
			Coeff & U_1 & U_2 & U_3 & U_4 & U_5 \\
			Ap & 0.0226 & 0.1800 & 0.0963 & 0.0352 & 0.00262 \\
			Bp & 0.0104 & 5.9500 & 3.8800 & 5.6700 & 0.00342 \\
			Cp & 0.0200 & 2.0000 & 2.0000 & 2.0000 & 0.02000 \\
			Br & 1.0800 & 5.9500 & 3.8800 & 5.6700 & 0.32300 \\
			Cr & 2.0000 & 2.0000 & 2.0000 & 2.0000 & 2.00000
		\end{array}					
	\f]

	The current curves are implemented according to \e SEL-551 \e Instruction \e Manual 
	\htmlonly (<a href="http://www.google.com/search?hl=en&q=SEL-551+Manual">Google that</a>) \endhtmlonly

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"

/* Coefficient				U1			U2			U3			U4			U5		*/
static const double Ap[] = {0.0226,		0.1800,		0.0963,		0.0352,		0.00262	};
static const double Bp[] = {0.0104,		5.9500,		3.8800,		5.6700,		0.00342	};
static const double Cp[] = {0.0200,		2.0000,		2.0000,		2.0000,		0.02000	};
static const double Br[] = {1.0800,		5.9500,		3.8800,		5.6700,		0.32300	};
static const double Cr[] = {2.0000,		2.0000,		2.0000,		2.0000,		2.00000	};


//////////////////////////////////////////////////////////////////////////
// relay CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* relay::oclass = NULL;
CLASS* relay::pclass = NULL;
relay *relay::defaults = NULL;

CLASS *relay_class = (NULL);

relay::relay(MODULE *mod) : link(mod)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		relay_class = oclass = gl_register_class(mod,"relay",sizeof(relay),PC_BOTTOMUP);
		if (oclass==NULL)
			throw "unable to register class relay";
		else
			oclass->trl = TRL_PROOF;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_enumeration,"Curve",PADDR(Curve),
				PT_KEYWORD,"FC_U1",FC_U1,
				PT_KEYWORD,"FC_U2",FC_U2,
				PT_KEYWORD,"FC_U3",FC_U3,
				PT_KEYWORD,"FC_U4",FC_U4,
				PT_KEYWORD,"FC_U5",FC_U5,
			PT_double, "TimeDial", PADDR(TimeDial),
			PT_double, "SetCurrent", PADDR(SetCurrent),
			PT_enumeration,"State",PADDR(State),
				PT_KEYWORD,"FS_CLOSED",FS_CLOSED,
				PT_KEYWORD,"FS_TRIPPED",FS_TRIPPED,
				PT_KEYWORD,"FS_RECLOSED",FS_RECLOSED,
				PT_KEYWORD,"FS_LOCKOUT",FS_LOCKOUT,
				PT_KEYWORD,"FS_FAULT",FS_FAULT,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
	}
}

int relay::create() 
{
	int result = link::create();
	memcpy(this,defaults,sizeof(*this));
	return result;
}

TIMESTAMP relay::sync(TIMESTAMP t0) 
{
	link::sync(t0);

	double M = I.Mag()/SetCurrent;
	switch (State) {
	case FS_CLOSED:
		if (M>1)
		{	// compute trip time
			Tp = TimeDial * (Ap[Curve]+Bp[Curve]/(pow(M,Cp[Curve])-1));
			TIMESTAMP t1 = Tstate + (TIMESTAMP)(Tp*TS_SECOND);
			if (t1<=t0)
			{
				State=FS_TRIPPED;
				Tstate = t0;
				t1 = TS_NEVER;
			}
			return t1;
		}
		else if (M<=1)
		{	// reset timer
			Tstate = 0;
			return TS_NEVER;
		}
		break;
	case FS_TRIPPED:
		if (M<1)
		{
			// compute reset time
			Tr = TimeDial *(Br[Curve]/(1-pow(M,Cr[Curve])));
			TIMESTAMP t1 = Tstate +(TIMESTAMP)(Tr*TS_SECOND);
			if (t1<=t0)
			{
				State = FS_RECLOSED;
				Tstate = t0;
				return TS_NEVER;
			}
			else
				return t1;
		}
		break;
	case FS_RECLOSED:
		// TODO:
		break;
	case FS_LOCKOUT:
		// TODO:
		break;
	case FS_FAULT:
		// TODO:
		break;
	default:
		break;
	}

	return TS_NEVER;
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: relay
//////////////////////////////////////////////////////////////////////////

EXPORT int create_relay(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(relay_class);
	if (*obj!=NULL)
	{
		relay *my = OBJECTDATA(*obj,relay);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT TIMESTAMP sync_relay(OBJECT *obj, TIMESTAMP t0)
{
	TIMESTAMP t1 = OBJECTDATA(obj,relay)->sync(t0);
	obj->clock = t0;
	return t1;
}

/**@}*/
