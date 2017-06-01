/** $Id: gen.cpp 4738 2014-07-03 00:55:39Z dchassin $
	@file gen.cpp
	@defgroup gen Template for a new object class
	@ingroup MODULENAME

	You can add an object class to a module using the \e add_class
	command:
	<verbatim>
	linux% add_class CLASS
	</verbatim>

	You must be in the module directory to do this.

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "gen.h"
#include "bus.h"
#include "matpower.h"

CLASS *gen::oclass = NULL;
gen *gen::defaults = NULL;

#ifdef OPTIONAL
/* TODO: define this to allow the use of derived classes */
CLASS *PARENTgen::pclass = NULL;
#endif

/* TODO: remove passes that aren't needed */
static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN;

/* TODO: specify which pass the clock advances */
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
gen::gen(MODULE *module)
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
		oclass = gl_register_class(module,"gen",sizeof(gen),passconfig);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
			/* TODO: add your published properties here */
                        //PT_int16, "GEN_BUS", PADDR(GEN_BUS), PT_DESCRIPTION, "bus number",
                        PT_double, "PG[MW]", PADDR(PG), PT_DESCRIPTION, "real power output",
                        PT_double, "QG[MVAr]", PADDR(QG), PT_DESCRIPTION, "reactive power output",
                        PT_double, "QMAX[MVAr]", PADDR(QMAX), PT_DESCRIPTION, "maximum reactive power output",
                        PT_double, "QMIN[MVAr]", PADDR(QMIN), PT_DESCRIPTION, "minimum reactive power output",
                        PT_double, "VG", PADDR(VG), PT_DESCRIPTION, "voltage magnitude setpoint",
                        PT_double, "MBASE", PADDR(MBASE), PT_DESCRIPTION, "total MVA base of machine, defaults to baseMVA",
                        PT_enumeration, "GEN_STATUS", PADDR(GEN_STATUS), PT_DESCRIPTION, "machine status, >0 = machine in-service, <=0 = machine out-of-service",
				PT_KEYWORD, "In", 1,
				PT_KEYWORD, "Out", 0,
                        PT_double, "PMAX[MW]", PADDR(PMAX), PT_DESCRIPTION, "maximum real power output",
                        PT_double, "PMIN[MW]", PADDR(PMIN), PT_DESCRIPTION, "minimum real power output",
                        PT_double, "PC1[MW]", PADDR(PC1), PT_DESCRIPTION, "lower real power output of PQ capability curve",
                        PT_double, "PC2[MW]", PADDR(PC2), PT_DESCRIPTION, "upper real power output of PQ capability curve",
                        PT_double, "QC1MIN[MVAr]", PADDR(QC1MIN), PT_DESCRIPTION, "minimum reactive power output at PC1",
                        PT_double, "QC1MAX[MVAr]", PADDR(QC1MAX), PT_DESCRIPTION, "maximum reactive power output at PC1",
			PT_double, "QC2MIN[MVAr]", PADDR(QC2MIN), PT_DESCRIPTION, "minimum reactive power output at PC2",
                        PT_double, "QC2MAX[MVAr]", PADDR(QC2MAX), PT_DESCRIPTION, "maximum reactive power output at PC2",
                        PT_double, "RAMP_AGC", PADDR(RAMP_AGC),PT_DESCRIPTION,"ramp rate for load following/AGC",
                        PT_double, "RAMP_10[MW]", PADDR(RAMP_10), PT_DESCRIPTION, "ramp rate for 10 min reserves",
                        PT_double, "RAMP_30[MW]", PADDR(RAMP_30), PT_DESCRIPTION , "ramp rate for 30 min reserves",
                        PT_double, "RAMP_Q[MW]", PADDR(RAMP_Q), PT_DESCRIPTION, "ramp rate for reactive power",
                        PT_double, "APF", PADDR(APF), PT_DESCRIPTION, "area participation factor",
			PT_double, "MU_PMAX", PADDR(MU_PMAX), PT_DESCRIPTION, "Kuhn-Tucker multiplier on upper P_g limit (mu/MW)",
			PT_double, "MU_PMIN", PADDR(MU_PMIN),
PT_DESCRIPTION, "Kuhn-Tucker multiplier on lower P_g limit (mu/MW)",		
			PT_double, "MU_QMAX", PADDR(MU_QMAX), PT_DESCRIPTION, "Kuhn-Tucker multiplier on upper Q_g limit (mu/MW)",
			PT_double, "MU_QMIN", PADDR(MU_QMIN), PT_DESCRIPTION, "Kuhn-Tucker multiplier on lower Q_g limit (mu/MW)",
			PT_double, "Price", PADDR(Price), PT_DESCRIPTION, "generator operation price in US dollar",

// Cost model
			PT_enumeration, "MODEL", PADDR(MODEL), PT_DESCRIPTION, "cost model",
				PT_KEYWORD, "Piecewise Linear", 1,
				PT_KEYWORD, "Polynomial", 2,
                                                                // 1 = piecewise linear
                                                                // 2 = polynomial
                        PT_double, "STARTUP", PADDR(STARTUP), PT_DESCRIPTION, "start up cost in US dollars",
                        PT_double, "SHUTDOWN", PADDR(SHUTDOWN), PT_DESCRIPTION, "shutdown cost in US dollars",
                        //PT_int16, "NCOST", PADDR(NCOST),PT_DESCRIPTION, "number of cost coeff for poly cost function or number of data points for piecewise linear",
                        /*Only support model 2 right now -- LYZ @ Jan 11th, 2012*/
                        PT_char1024, "COST", PADDR(COST), PT_DESCRIPTION, "n+1 coeff of n-th order polynomial cost, starting with highest order",
                        NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(this,0,sizeof(gen));
		/* TODO: set the default values of all properties here */
	}
}

/* Object creation is called once for each object that is created by the core */
int gen::create(void)
{
	memcpy(this,defaults,sizeof(gen));
	/* TODO: set the context-free initial value of properties, such as random distributions */
	MU_PMAX = 0;
	MU_PMIN = 0;
	MU_QMAX = 0;
	MU_QMIN = 0;
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int gen::init(OBJECT *parent)
{
	/* TODO: set the context-dependent initial value of properties */
	
	// Get the bus id (BUS_I) from bus object (parent) and set generator id (GEN_BUS)
/*	
	OBJECT *obj_gen = OBJECTHDR(this);
	gen *tempgen = OBJECTDATA(obj_gen,gen);

	bus *tempbus = OBJECTDATA(parent,bus);
	setObjectValue_Double(obj_gen,"GEN_BUS",tempbus->BUS_I);

	//printf("Gen_id %d,bus_id %d\n",tempgen->GEN_BUS,tempbus->BUS_I);
*/
	return 1; /* return 1 on success, 0 on failure */
}

/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP gen::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP gen::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement bottom-up behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP gen::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_gen(OBJECT **obj)
{
	try
	{
		*obj = gl_create_object(gen::oclass);
		if (*obj!=NULL)
			return OBJECTDATA(*obj,gen)->create();
	}
	catch (char *msg)
	{
		gl_error("create_gen: %s", msg);
	}
	return 0;
}

EXPORT int init_gen(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,gen)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("init_gen(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return 0;
}

EXPORT TIMESTAMP sync_gen(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	gen *my = OBJECTDATA(obj,gen);
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
		gl_error("sync_gen(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return TS_INVALID;
}
