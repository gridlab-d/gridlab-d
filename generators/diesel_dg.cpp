/** $Id: diesel_dg.cpp,v 1.2 2008/02/12 00:28:08 d3g637 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file diesel_dg.cpp
	@defgroup diesel_dg Diesel gensets
	@ingroup generators

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
//#include "C:\_FY08\Projects-SVN\source\residential\solvers.h"

#include "diesel_dg.h"
#include "lock.h"

CLASS *diesel_dg::oclass = NULL;
diesel_dg *diesel_dg::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
diesel_dg::diesel_dg(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"diesel_dg",sizeof(diesel_dg),passconfig);
		if (oclass==NULL)
			throw "unable to register class diesel_dg";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			PT_enumeration,"Gen_mode",PADDR(Gen_mode),
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"CONSTANTE",CONSTANTE,
				PT_KEYWORD,"CONSTANTPQ",CONSTANTPQ,
			PT_enumeration,"Gen_status",PADDR(Gen_status),
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"OFFLINE",OFFLINE,
				PT_KEYWORD,"ONLINE",ONLINE,	
			PT_double, "Rated_kVA[kVA]", PADDR(Rated_kVA),
			PT_double, "Rated_kV[kV]", PADDR(Rated_kV),
			PT_double, "Rs", PADDR(Rs),
			PT_double, "Xs", PADDR(Xs),
			PT_double, "Rg", PADDR(Rg),
			PT_double, "Xg", PADDR(Xg),
			PT_complex, "voltage_A[V]", PADDR(voltage_A),
			PT_complex, "voltage_B[V]", PADDR(voltage_B),
			PT_complex, "voltage_C[V]", PADDR(voltage_C),
			PT_complex, "current_A[A]", PADDR(current_A),
			PT_complex, "current_B[A]", PADDR(current_B),
			PT_complex, "current_C[A]", PADDR(current_C),
			PT_complex, "EfA[V]", PADDR(EfA),
			PT_complex, "EfB[V]", PADDR(EfB),
			PT_complex, "EfC[V]", PADDR(EfC),
			PT_complex, "power_A[VA]", PADDR(power_A),
			PT_complex, "power_B[VA]", PADDR(power_B),
			PT_complex, "power_C[VA]", PADDR(power_C),
			PT_complex, "power_A_sch[VA]", PADDR(power_A_sch),
			PT_complex, "power_B_sch[VA]", PADDR(power_B_sch),
			PT_complex, "power_C_sch[VA]", PADDR(power_C_sch),
			PT_complex, "EfA_sch[V]", PADDR(EfA_sch),
			PT_complex, "EfB_sch[V]", PADDR(EfB_sch),
			PT_complex, "EfC_sch[V]", PADDR(EfC_sch),
			PT_int16,"SlackBus",PADDR(SlackBus),
			PT_set, "phases", PADDR(phases),
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(this,0,sizeof(diesel_dg));
		/* TODO: set the default values of all properties here */
	}
}

/* Object creation is called once for each object that is created by the core */
int diesel_dg::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int diesel_dg::init(OBJECT *parent)
{
	/* TODO: set the context-dependent initial value of properties */
	double ZB, SB, EB;
	complex tst;
	int i;
	OBJECT *obj = OBJECTHDR(this);

	if (Gen_mode==UNKNOWN)
	{
		throw("Generator control mode is not specified");
	}
	if (Gen_status==0)
	{
		//OBJECT *obj = OBJECTHDR(this);
		throw("Generator is out of service!");
	}
	else
		{
//			if (Rated_kW!=0.0)  SB = Rated_kW/sqrt(1-Rated_pf*Rated_pf);
			if (Rated_kVA!=0.0)  SB = Rated_kVA*1000.0/3;
			if (Rated_kV!=0.0)  EB = Rated_kV*1000.0/sqrt(3.0);
			if (SB!=0.0)  ZB = EB*EB/SB;
			else throw("Generator power capacity not specified!");
			double Real_Rs = Rs * ZB; 
			double Real_Xs = Xs * ZB;
			double Real_Rg = Rg * ZB; 
			double Real_Xg = Xg * ZB;
			tst = complex(Real_Rg,Real_Xg);
			AMx[0][0] = complex(Real_Rs,Real_Xs) + tst;
			AMx[1][1] = complex(Real_Rs,Real_Xs) + tst;
			AMx[2][2] = complex(Real_Rs,Real_Xs) + tst;
		//	AMx[0][0] = AMx[1][1] = AMx[2][2] = complex(Real_Rs+Real_Rg,Real_Xs+Real_Xg);
			AMx[0][1] = AMx[0][2] = AMx[1][0] = AMx[1][2] = AMx[2][0] = AMx[2][1] = tst;
		}

	struct {
		complex **var;
		char *varname;
	} map[] = {
		// local object name,	meter object name
		{&pCircuit_V,			"voltage_A"}, // assumes 23 and 31 follow immediately in memory
		{&pLine_I,				"current_A"}, // assumes 2 and 3(N) follow immediately in memory
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
	};

	if (parent!=NULL && strcmp(parent->oclass->name,"meter")==0)
	{
		// attach meter variables to each circuit
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
			*(map[i].var) = get_complex(parent,map[i].varname);
	}
	else
	{
		GL_THROW("Meter object was not found, please specify a meter parent for diesel_dg:%d.",obj->id);
		/* TROUBLESHOOT
		A meter object is the only way that a generator object can attach to powerflow or other solver
		systems.  Please create a parent meter object for the generator to attach to the system.
		*/
	}

	return 1; /* return 1 on success, 0 on failure */
}

complex *diesel_dg::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}


/* Presync is called when the clock needs to advance on the first top-down pass */
/*TIMESTAMP diesel_dg::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	current_A = current_B = current_C = 0.0;

	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	//return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
//}**/

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP diesel_dg::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
//	complex PA,QA,PB,QB,PC,QC;
//	double Rated_kVar;
	if (SlackBus==1) Gen_mode = CONSTANTE;

	OBJECT *obj = OBJECTHDR(this);
/*
	voltage_A = OBJECTDATA(obj,node)->voltage_A;
	voltage_B = OBJECTDATA(obj,node)->voltage_B;
	voltage_C = OBJECTDATA(obj,node)->voltage_C;

*/
    
	if (obj != NULL)
		LOCK_OBJECT(obj);

	voltage_A = pCircuit_V[0];
	voltage_B = pCircuit_V[1];
	voltage_C = pCircuit_V[2];

	if (obj != NULL)
		UNLOCK_OBJECT(obj);

	if (Gen_mode==CONSTANTE)
	{
		current_A += AMx[0][0]*(voltage_A-EfA) + AMx[0][1]*(voltage_B-EfB) + AMx[0][2]*(voltage_C-EfC);
		current_B += AMx[1][0]*(voltage_A-EfA) + AMx[1][1]*(voltage_B-EfB) + AMx[1][2]*(voltage_C-EfC);
		current_C += AMx[2][0]*(voltage_A-EfA) + AMx[2][1]*(voltage_B-EfB) + AMx[2][2]*(voltage_C-EfC);
 //       PA = Re(voltage_A * phaseA_I_in); 
//		QA = Im(voltage_A * phaseA_I_in); 
	
	}else if (Gen_mode==CONSTANTPQ){
		current_A = - (~(complex(power_A_sch)/voltage_A));
		current_B = - (~(complex(power_B_sch)/voltage_B));
		current_C = - (~(complex(power_C_sch)/voltage_C));
	}

	if (obj->parent != NULL)
		LOCK_OBJECT(obj->parent);

	pLine_I[0] = current_A;
	pLine_I[1] = current_B;
	pLine_I[2] = current_C;

	if (obj->parent != NULL)
		UNLOCK_OBJECT(obj->parent);

	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement bottom-up behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP diesel_dg::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	power_A = -voltage_A * (~current_A);
	power_B = -voltage_B * (~current_B);
	power_C = -voltage_C * (~current_C);

	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement post-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_diesel_dg(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(diesel_dg::oclass);
		if (*obj!=NULL)
		{
			diesel_dg *my = OBJECTDATA(*obj,diesel_dg);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	} 
	catch (char *msg) 
	{
		gl_error("create_diesel_dg: %s", msg);
	}
	return 0;
}

EXPORT int init_diesel_dg(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,diesel_dg)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("init_diesel_dg(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return 0;
}

EXPORT TIMESTAMP sync_diesel_dg(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	diesel_dg *my = OBJECTDATA(obj,diesel_dg);
	try
	{
		switch (pass) {
//		case PC_PRETOPDOWN:
//			t2 = my->presync(obj->clock,t1);
//			break;
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
	catch (char *msg)
	{
		gl_error("sync_diesel_dg(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return t2;
}
