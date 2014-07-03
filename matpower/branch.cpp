/** $Id: branch.cpp 4738 2014-07-03 00:55:39Z dchassin $
	@file branch.cpp
	@defgroup branch Template for a new object class
	@ingroup MODULENAME

	You can add an object class to a module using the \e add_class
	command:
	<verbatim>
	linux% add_class CLASS
	</verbatim>

	You must be in the module directory to do this.
	author: Kyle Anderson (GridSpice Project),  kyle.anderson@stanford.edu
	Released in Open Source to Gridlab-D Project


 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "branch.h"

CLASS *branch::oclass = NULL;
branch *branch::defaults = NULL;

#ifdef OPTIONAL
/* TODO: define this to allow the use of derived classes */
CLASS *PARENTbranch::pclass = NULL;
#endif

/* TODO: remove passes that aren't needed */
static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN;

/* TODO: specify which pass the clock advances */
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
branch::branch(MODULE *module)
#ifdef OPTIONAL
/* TODO: include this if you are deriving this from a superclass */
: SUPERCLASS(module)
#endif
{
#ifdef OPTIONAL
	/* TODO: include this if you are deriving this from a superclass */
	pclass = SUPERCLASS::oclass;
#endif
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"branch",sizeof(branch),passconfig);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
             // PT_int16, "F_BUS", PADDR(F_BUS),PT_DESCRIPTION, "from bus number",
              //PT_int16, "T_BUS", PADDR(T_BUS),PT_DESCRIPTION, "to bus number",
	      PT_char1024, "from", PADDR(from), PT_DESCRIPTION,"from bus name",
	      PT_char1024, "to", PADDR(to), PT_DESCRIPTION, "to bus name",
              PT_double, "BR_R", PADDR(BR_R), PT_DESCRIPTION, "resistance (p.u.)",
              PT_double, "BR_X", PADDR(BR_X), PT_DESCRIPTION, "reactance (p.u.)",
              PT_double, "BR_B", PADDR(BR_B),  PT_DESCRIPTION,"total line charging susceptance (p.u.)",
              PT_double, "RATE_A", PADDR(RATE_A), PT_DESCRIPTION, "MVA rating A (long term rating)",
              PT_double, "RATE_B", PADDR(RATE_B), PT_DESCRIPTION, "MVA rating B (short term rating)",
              PT_double, "RATE_C", PADDR(RATE_C), PT_DESCRIPTION, "MVA rating C (emergency rating)",
              PT_double, "TAP", PADDR(TAP), PT_DESCRIPTION, "transformer off nominal turns ratio ( taps at from bus, impedance at to bus, i.e. if r=x=0, tap = \frac{|V_f|}{|V_t|}",
              PT_double, "SHIFT", PADDR(SHIFT), PT_DESCRIPTION, "transformer phase shift angle (degrees)",
              PT_enumeration,	 "BR_STATUS", PADDR(BR_STATUS), PT_DESCRIPTION, "initial branch status, 1 = in-service,0=out-of-service",
			PT_KEYWORD, "In", 1,
			PT_KEYWORD, "Out", 0,
              PT_double, "ANGMIN", PADDR(ANGMIN), PT_DESCRIPTION, "minimum angle difference, \theta_f - \theta_t (degrees)",
              PT_double, "ANGMAX", PADDR(ANGMAX), PT_DESCRIPTION, "maximum angle difference, \theta_f - \theta_t (degrees)",
		PT_double, "PF[MW]", PADDR(PF), PT_DESCRIPTION, "real power injected at from bus end (MW)",
		PT_double, "QF[MVAr]", PADDR(QF), PT_DESCRIPTION, "reactive power injected at from bus end (MVAr)",
		PT_double, "PT[MW]", PADDR(PT), PT_DESCRIPTION, "real power injected at to bus end (MW)",
		PT_double, "QT[MVAr]", PADDR(QT), PT_DESCRIPTION, "reactive poewr injected at to bus end (MVAr)",
		PT_double, "MU_SF", PADDR(MU_SF), PT_DESCRIPTION, "Kuhn-Tucker multiplier on MVA limit at from bus (mu/MVA)",
		PT_double, "MU_ST", PADDR(MU_ST), PT_DESCRIPTION, "Kuhn-Tucker multiplier on MVA limit at to bus (mu/MVA)",
		PT_double, "MU_ANGMIN", PADDR(MU_ANGMIN), PT_DESCRIPTION, "Kuhn-Tucker multiplier lower angle difference limit (mu/degree)",
		PT_double, "MU_ANGMAX", PADDR(MU_ANGMAX), PT_DESCRIPTION, "Kuhn-Tucker multiplier upper angle difference limit (mu/degree)",
           
              NULL)<1 && errno) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(this,0,sizeof(branch));
		/* TODO: set the default values of all properties here */
	}
}

/* Object creation is called once for each object that is created by the core */
int branch::create(void)
{
	memcpy(this,defaults,sizeof(branch));
	/* TODO: set the context-free initial value of properties, such as random distributions */
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int branch::init(OBJECT *parent)
{
	/* TODO: set the context-dependent initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}

/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP branch::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP branch::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement bottom-up behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP branch::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_branch(OBJECT **obj)
{
	try
	{
		*obj = gl_create_object(branch::oclass);
		if (*obj!=NULL)
			return OBJECTDATA(*obj,branch)->create();
	}
	catch (char *msg)
	{
		gl_error("create_branch: %s", msg);
	}
	return 0;
}

EXPORT int init_branch(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,branch)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("init_branch(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return 0;
}

EXPORT TIMESTAMP sync_branch(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	branch *my = OBJECTDATA(obj,branch);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;
		return t2;
	}
	catch (char *msg)
	{
		gl_error("sync_branch(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return TS_INVALID;
}
