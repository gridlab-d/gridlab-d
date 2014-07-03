/** $Id: fuse.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file fuse.cpp
	@addtogroup fuse Fuse
	@ingroup network
	
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"


//////////////////////////////////////////////////////////////////////////
// fuse CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* fuse::oclass = NULL;
CLASS* fuse::pclass = NULL;
fuse *fuse::defaults = NULL;
CLASS *fuse_class = (NULL);

fuse::fuse(MODULE *mod) : link(mod)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		fuse_class = oclass = gl_register_class(mod,"fuse",sizeof(fuse),PC_BOTTOMUP);
		if (oclass==NULL)
			throw "unable to register class fuse";
		else
			oclass->trl = TRL_STANDALONE;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_double, "TimeConstant", PADDR(TimeConstant),
			PT_double, "SetCurrent", PADDR(SetCurrent),
			PT_double, "SetBase", PADDR(SetBase),
			PT_double, "SetScale", PADDR(SetScale),
			PT_double, "SetCurve", PADDR(SetCurve),
			PT_double, "TresetAvg", PADDR(TresetAvg),
			PT_double, "TresetStd", PADDR(TresetStd),
			PT_enumeration,"State",PADDR(State),
				PT_KEYWORD,"FS_GOOD",FS_GOOD,
				PT_KEYWORD,"FS_BLOWN",FS_BLOWN,
				PT_KEYWORD,"FS_FAULT",FS_FAULT,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
		Y = 100;			// no effective resistance
		TimeConstant = 1.0;	// equivalent to SEL551 TimeDial setting
		SetCurrent = 1000;	// amps above which fusing occurs
		SetBase = 0.05;		// roughly like a SEL551 U3 relay
		SetScale = 4.0;		// roughly like a SEL551 U3 relay
		SetCurve = 2;		// roughly like a SEL551 U3 relay
		TresetAvg = 3600*4;	// about 4 hours avg
		TresetStd = 3600;	// about 1 hours stdev
		State = FS_GOOD;	// fuse state
	}
}

int fuse::create() 
{
	int result = link::create();
	memcpy(this,defaults,sizeof(*this));
	return result;
}

TIMESTAMP fuse::sync(TIMESTAMP t0) 
{
	link::sync(t0);

	double M = I.Mag()/SetCurrent;
	switch (State) {
	case FS_GOOD:
		Y=100; // fuse is good
		if (M>1)
		{	// compute fuse time
			double t = TimeConstant * (SetBase+SetScale/(pow(M,SetCurve)-1));
			TIMESTAMP t1 = Tstate + (TIMESTAMP)(t*TS_SECOND);
			if (t1<=t0)
			{	// fuse blows now
				State=FS_BLOWN;
				Tstate = t0;
				Treset = t0 + (TIMESTAMP)(gl_random_normal(RNGSTATE,TresetAvg,TresetStd)*TS_SECOND);
				return TS_NEVER;
			}
			else // fuse blows soon
			{
				Tstate = t1;
				return t1;
			}
		}
		else if (M<=1)
		{	// fuse doesn't blow; reset timer
			/// @todo it would be better to model how much damage the fuse incurred (network, low priority) (ticket #128)
			Tstate = t0;
			return TS_NEVER;
		}
		break;
	case FS_BLOWN:
		Y=0; // fuse is gone
		if (Treset<=t0)
		{
			Tstate = t0;
			State = FS_GOOD;
		}
		break;
	case FS_FAULT:
		break;
	default:
		break;
	}

	return TS_NEVER;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: fuse
//////////////////////////////////////////////////////////////////////////

EXPORT int create_fuse(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(fuse_class);
	if (*obj!=NULL)
	{
		fuse *my = OBJECTDATA(*obj,fuse);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT TIMESTAMP sync_fuse(OBJECT *obj, TIMESTAMP t0)
{
	TIMESTAMP t1 = OBJECTDATA(obj,fuse)->sync(t0);
	obj->clock = t0;
	return t1;
}


/**@}*/
