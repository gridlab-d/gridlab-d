/** $Id: energy_storage.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file energy_storage.cpp
	@defgroup energy_storage
	@ingroup generators

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "generators.h"

#include "energy_storage.h"
#include "gridlabd.h"



#define HOUR 3600 * TS_SECOND

CLASS *energy_storage::oclass = NULL;
energy_storage *energy_storage::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;


energy_storage::energy_storage(){};


/* Class registration is only called once to register the class with the core */
energy_storage::energy_storage(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"energy_storage",sizeof(energy_storage),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class energy_storage";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			PT_enumeration,"generator_mode",PADDR(gen_mode_v),
				PT_KEYWORD,"UNKNOWN",(enumeration)UNKNOWN,
				PT_KEYWORD,"CONSTANT_V",(enumeration)CONSTANT_V,
				PT_KEYWORD,"CONSTANT_PQ",(enumeration)CONSTANT_PQ,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)CONSTANT_PF,
				PT_KEYWORD,"SUPPLY_DRIVEN",(enumeration)SUPPLY_DRIVEN, //PV must operate in this mode

			PT_enumeration,"generator_status",PADDR(gen_status_v),
				PT_KEYWORD,"OFFLINE",(enumeration)OFFLINE,
				PT_KEYWORD,"ONLINE",(enumeration)ONLINE,	

			PT_enumeration,"power_type",PADDR(power_type_v),
				PT_KEYWORD,"AC",(enumeration)AC,
				PT_KEYWORD,"DC",(enumeration)DC,


			PT_double, "Rinternal", PADDR(Rinternal),
			PT_double, "V_Max[V]", PADDR(V_Max), 
			PT_complex, "I_Max[A]", PADDR(I_Max),
			PT_double, "E_Max", PADDR(E_Max),
			PT_double, "Energy",PADDR(Energy),
			PT_double, "efficiency", PADDR(efficiency),
		

			PT_double, "Rated_kVA[kVA]", PADDR(Rated_kVA),
			PT_complex, "V_Out[V]", PADDR(V_Out),
			PT_complex, "I_Out[A]", PADDR(I_Out),
			PT_complex, "VA_Out[VA]", PADDR(VA_Out),
			PT_complex, "V_In[V]", PADDR(V_In),
			PT_complex, "I_In[A]", PADDR(I_In),
			PT_complex, "V_Internal[V]", PADDR(V_Internal),
			PT_complex, "I_Internal[A]",PADDR(I_Internal),
			PT_complex, "I_Prev[A]", PADDR(I_Prev),

			//resistasnces and max P, Q

			PT_set, "phases", PADDR(phases),

				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;




		memset(this,0,sizeof(energy_storage));
		/* TODO: set the default values of all properties here */
	}
}

/* Object creation is called once for each object that is created by the core */
int energy_storage::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}



/*
double energy_storage::timestamp_to_hours(TIMESTAMP t)
{
	return (double)t/HOUR;
}
*/


/* Object initialization is called once after all object have been created */
int energy_storage::init(OBJECT *parent)
{

	gl_verbose("energy_storage init: about to exit");
	return 1; /* return 1 on success, 0 on failure */
}


double energy_storage::calculate_efficiency(complex voltage, complex current){
	return 1;
}


complex energy_storage::calculate_v_terminal(complex v, complex i){
	return v - (i * Rinternal);
}


complex *energy_storage::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}




/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP energy_storage::presync(TIMESTAMP t0, TIMESTAMP t1)
{

	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP energy_storage::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	
return TS_NEVER;
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP energy_storage::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_energy_storage(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(energy_storage::oclass);
		if (*obj!=NULL)
		{
			energy_storage *my = OBJECTDATA(*obj,energy_storage);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(energy_storage);
}

EXPORT int init_energy_storage(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,energy_storage)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(energy_storage);
}

EXPORT TIMESTAMP sync_energy_storage(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	energy_storage *my = OBJECTDATA(obj,energy_storage);
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
	}
	SYNC_CATCHALL(energy_storage);
	return t2;
}
