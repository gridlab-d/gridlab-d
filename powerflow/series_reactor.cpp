/** $Id: series_reactor.cpp,v 1.6 2009/10/23 07:40:00 d3x593 Exp $
	Copyright (C) 2009 Battelle Memorial Institute
	@file series_reactor.cpp
	@addtogroup powerflow series_reactor
	@ingroup powerflow

	Implements a a series reactor object with specifiable
	impedances.  This is a static reactor, so values will only
	be computed once.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "series_reactor.h"
#include "node.h"

//////////////////////////////////////////////////////////////////////////
// series_reactor CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* series_reactor::oclass = NULL;
CLASS* series_reactor::pclass = NULL;

series_reactor::series_reactor(MODULE *mod) : link_object(mod)
{
	if(oclass == NULL)
	{
		pclass = link_object::oclass;

		oclass = gl_register_class(mod,"series_reactor",sizeof(series_reactor),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class series_reactor";
		else
			oclass->trl = TRL_PROVEN;

        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
			PT_complex, "phase_A_impedance[Ohm]",PADDR(phase_A_impedance),PT_DESCRIPTION,"Series impedance of reactor on phase A",
			PT_double, "phase_A_resistance[Ohm]",PADDR(phase_A_impedance.Re()),PT_DESCRIPTION,"Resistive portion of phase A's impedance",
			PT_double, "phase_A_reactance[Ohm]",PADDR(phase_A_impedance.Im()),PT_DESCRIPTION,"Reactive portion of phase A's impedance",
			PT_complex, "phase_B_impedance[Ohm]",PADDR(phase_B_impedance),PT_DESCRIPTION,"Series impedance of reactor on phase B",
			PT_double, "phase_B_resistance[Ohm]",PADDR(phase_B_impedance.Re()),PT_DESCRIPTION,"Resistive portion of phase B's impedance",
			PT_double, "phase_B_reactance[Ohm]",PADDR(phase_B_impedance.Im()),PT_DESCRIPTION,"Reactive portion of phase B's impedance",
			PT_complex, "phase_C_impedance[Ohm]",PADDR(phase_C_impedance),PT_DESCRIPTION,"Series impedance of reactor on phase C",
			PT_double, "phase_C_resistance[Ohm]",PADDR(phase_C_impedance.Re()),PT_DESCRIPTION,"Resistive portion of phase C's impedance",
			PT_double, "phase_C_reactance[Ohm]",PADDR(phase_C_impedance.Im()),PT_DESCRIPTION,"Reactive portion of phase C's impedance",
			PT_double, "rated_current_limit[A]",PADDR(rated_current_limit),PT_DESCRIPTION,"Rated current limit for the reactor",
            NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_link)==NULL)
			GL_THROW("Unable to publish series reactor deltamode function");
    }
}

int series_reactor::isa(char *classname)
{
	return strcmp(classname,"series_reactor")==0 || link_object::isa(classname);
}

int series_reactor::create()
{
	int result = link_object::create();
	phase_A_impedance = phase_B_impedance = phase_C_impedance = 0.0;

	return result;
}

int series_reactor::init(OBJECT *parent)
{
	int result = link_object::init(parent);

	a_mat[0][0] = d_mat[0][0] = A_mat[0][0] = (has_phase(PHASE_A)) ? 1.0 : 0.0;
	a_mat[1][1] = d_mat[1][1] = A_mat[1][1] = (has_phase(PHASE_B)) ? 1.0 : 0.0;
	a_mat[2][2] = d_mat[2][2] = A_mat[2][2] = (has_phase(PHASE_C)) ? 1.0 : 0.0;

	if (solver_method==SM_FBS)
	{
		c_mat[0][0] = 0.0;
		c_mat[1][1] = 0.0;
		c_mat[2][2] = 0.0;

		//Pass in impedance values
		b_mat[0][0] = B_mat[0][0] = phase_A_impedance;
		b_mat[1][1] = B_mat[1][1] = phase_B_impedance;
		b_mat[2][2] = B_mat[2][2] = phase_C_impedance;

	}
	else
	{
		//Flag it as special (we'll forgo inversion processes on this)
		SpecialLnk = SWITCH;

		//Initialize off-diagonals just in case
		From_Y[0][1] = From_Y[0][2] = From_Y[1][0] = 0.0;
		From_Y[1][2] = From_Y[2][0] = From_Y[2][1] = 0.0;

		//See if it has a particular phase, if so populate it.  If not
		//and has zero impedance, put a "default" value in its place
		if (has_phase(PHASE_A))
		{
			if (phase_A_impedance==0.0)
				From_Y[0][0] = complex(1e4,1e4);
			else
				From_Y[0][0] = complex(1.0,0.0)/phase_A_impedance;
		}
		else
			From_Y[0][0] = 0.0;		//Should already be 0, but let's be paranoid

		if (has_phase(PHASE_B))
		{
			if (phase_B_impedance==0.0)
				From_Y[1][1] = complex(1e4,1e4);
			else
				From_Y[1][1] = complex(1.0,0.0)/phase_B_impedance;
		}
		else
			From_Y[1][1] = 0.0;		//Should already be 0, but let's be paranoid

		if (has_phase(PHASE_C))
		{
			if (phase_C_impedance==0.0)
				From_Y[2][2] = complex(1e4,1e4);
			else
				From_Y[2][2] = complex(1.0,0.0)/phase_C_impedance;
		}
		else
			From_Y[2][2] = 0.0;		//Should already be 0, but let's be paranoid
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: series_reactor
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT TIMESTAMP commit_series_reactor(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	if (solver_method==SM_FBS)
	{
		series_reactor *plink = OBJECTDATA(obj,series_reactor);
		plink->calculate_power();
	}
	return TS_NEVER;
}
EXPORT int create_series_reactor(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(series_reactor::oclass);
		if (*obj!=NULL)
		{
			series_reactor *my = OBJECTDATA(*obj,series_reactor);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(series_reactor);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_series_reactor(OBJECT *obj)
{
	try {
		series_reactor *my = OBJECTDATA(obj,series_reactor);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(series_reactor);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_series_reactor(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		series_reactor *pObj = OBJECTDATA(obj,series_reactor);
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
	SYNC_CATCHALL(series_reactor);
}

EXPORT int isa_series_reactor(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,series_reactor)->isa(classname);
}

/**@}**/
