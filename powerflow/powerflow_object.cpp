/** $Id: powerflow_object.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file powerflow_object.cpp
	@addtogroup powerflow_object Powerflow objects (abstract)
	@ingroup powerflow

	The powerflow_object class is an abstract class that implements 
	basic elements of powerflow solutions.  There are two critical
	pieces of information:

	-# \p phases indicates the phases which are connected by this object.
		This may be any feasible combination of the phase flags 
		- \p PHASE_A for phase A connections
		- \p PHASE_B for phase B connections
		- \p PHASE_C for phase C connections
		- \p PHASE_N for neutral connections
		- \p PHASE_D for delta connections (implies A, B, C);
		- \p PHASE_A for split connections (requires one of A, B, or C);
		- \p PHASE_ABC for three phase connections
		- \p PHASE_ABCN for three phase connections with separate neutral
	-# \p reference_bus indicates the bus for the reference frequency and voltage at which the solution is to be
		computed.  The current solution method does not change the frequency
		nor does it consider the frequency in the powerflow calculated.  The
		parameter is only to be used to adjust frequency-sensitive loads, such
		as Grid-Friendly<sup>TM</sup> devices.

	In addition, each powerflow_object defined must specify what it's parent object
	is for the purposes of establishing the evaluation order through the ranks.
	The reference bus is also propagated using the rank order.

	The powerflow_object provides optional support for outage conditions when
	compiled with the SUPPORT_OUTAGES flag is defined in powerflow_object.h.
	When enabled the following parameters are also defined

	-# \p condition indicates the device's operating condition with respect
		to abnormal phase configuration that would require special consideration
		during the powerflow solution.  This can be any combination of
		two of the following
		- \p OC_NORMAL for no abnormal configuration (excludes all others)
		- \p OC_PHASE_A_CONTACT phase A in contact
		- \p OC_PHASE_B_CONTACT phase B in contact
		- \p OC_PHASE_C_CONTACT phase C in contact
		- \p OC_NEUTRAL_CONTACT neutral in contact
		- \p OC_SPLIT_1_CONTACT split line 1 in contact
		- \p OC_SPLIT_2_CONTACT split line 2 in contact
		- \p OC_SPLIT_N_CONTACT split line N in contact
		- \p OC_GROUND_CONTACT ground in contact
		- \p OC_PHASE_A_OPEN phase A is open
		- \p OC_PHASE_B_OPEN phase B is open
		- \p OC_PHASE_C_OPEN phase C is open
		- \p OC_NEUTRAL_OPEN neutral is open
		- \p OC_SPLIT_1_OPEN split line 1 is open
		- \p OC_SPLIT_2_OPEN split line 2 is open
		- \p OC_SPLIT_N_OPEN split neutral is open
		- \p OC_GROUND_OPEN ground connection is open
	-# \p solution indicates whether special solution methods are required.  The
		base powerflow_object only recognized the normal solution method (#PS_NORMAL),
		but each object may add its own identifiers, so long as it does not conflict
		any identifiers in the object inheritance hierarchy.  When #SUPPORT_OUTAGES is
		defined, the flag #PS_OUTAGE is also defined and can be used to engage an
		alternate solution method.

	@par Outage support

	When <code>SUPPORT_OUTAGES</code> is defined, the \p postsync pass will examine whether 
	\p condition and \p solution are matched.  If there is an inconsistency
	(i.e., \p <code>condition != OC_NORMAL &&</code> \p <code>solution == PS_NORMAL</code> or
	\p <code>condition == OC_NORMAL &&</code> \p <code>solution != PS_NORMAL</code>), then
	they will be reconciled and the solver will iterate again. 
	Object derived from powerflow_object can inspect the value of \p condition
	to determine whether they can use the normal powerflow solution method, or
	an alternative method that is appropriate for abnormal conditions.
	Be aware that this may result in a change of state in some other object
	that may cause the solver to reiterate. It is highly recommended that state 
	changes to OC_NORMAL have a non-zero delay built in (i.e., allow the 
	clock to advanced).	Failure to do so may result in non-convergence of the solver.

 @{
 **/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "powerflow.h"
#include "node.h"
#include "link.h"

//////////////////////////////////////////////////////////////////////////
// powerflow_object CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* powerflow_object::oclass = NULL;
CLASS* powerflow_object::pclass = NULL;

powerflow_object::powerflow_object(MODULE *mod)
{	
	if (oclass==NULL)
	{
#ifdef SUPPORT_OUTAGES
		oclass = gl_register_class(mod,"powerflow_object",sizeof(powerflow_object),PC_ABSTRACTONLY|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
#else
		oclass = gl_register_class(mod,"powerflow_object",sizeof(powerflow_object),PC_ABSTRACTONLY|PC_NOSYNC|PC_AUTOLOCK);
#endif
		if (oclass==NULL)
			throw "unable to register class powerflow_object";
		else
			oclass->trl = TRL_PROVEN;

		if(gl_publish_variable(oclass,
			PT_set, "phases", PADDR(phases),
				PT_KEYWORD, "G", (set)GROUND,
				PT_KEYWORD, "S", (set)PHASE_S,
				PT_KEYWORD, "N", (set)PHASE_N,
				PT_KEYWORD, "D", (set)PHASE_D,
				PT_KEYWORD, "C", (set)PHASE_C,
				PT_KEYWORD, "B", (set)PHASE_B,
				PT_KEYWORD, "A", (set)PHASE_A,
			PT_double,"nominal_voltage[V]",PADDR(nominal_voltage),
#ifdef SUPPORT_OUTAGES
			PT_set, "condition", PADDR(condition),
				PT_KEYWORD, "OPEN", (set)OC_OPEN,
				PT_KEYWORD, "CONTACT", (set)OC_CONTACT,
				PT_KEYWORD, "NORMAL", (set)OC_NORMAL,
				PT_KEYWORD, "SN", (set)PHASE_SN,
				PT_KEYWORD, "S2", (set)PHASE_S2,
				PT_KEYWORD, "S1", (set)PHASE_S1,
				PT_KEYWORD, "G", (set)GROUND,
				PT_KEYWORD, "N", (set)PHASE_N,
				PT_KEYWORD, "C", (set)PHASE_C,
				PT_KEYWORD, "B", (set)PHASE_B,
				PT_KEYWORD, "A", (set)PHASE_A,
			PT_enumeration, "solution", PADDR(solution),
				PT_KEYWORD, "NORMAL", PS_NORMAL,
				PT_KEYWORD, "OUTAGE", PS_OUTAGE,
#endif
         	NULL) < 1) GL_THROW("unable to publish powerflow_object properties in %s",__FILE__);

		// set defaults
	}
}

powerflow_object::powerflow_object(CLASS *oclass)
{
	gl_create_foreign((OBJECT*)this);
}

int powerflow_object::isa(char *classname)
{
	return strcmp(classname,"powerflow_object")==0;
}

int powerflow_object::create(void)
{
	OBJECT *obj = OBJECTHDR(this);
	phases = NO_PHASE;
	nominal_voltage = 0.0;

	//Deltamode override flag
	if (all_powerflow_delta == true)
		obj->flags |= OF_DELTAMODE;

#ifdef SUPPORT_OUTAGES
	condition = OC_NORMAL;
	solution = PS_NORMAL;
#endif
	return 1;
}

int powerflow_object::init(OBJECT *parent)
{
	/* unspecified phase inherits from parent, if any */
	if (parent && gl_object_isa(parent,"powerflow_object"))
	{
		powerflow_object *pParent = OBJECTDATA(parent,powerflow_object);
		if (phases==NO_PHASE)
			phases = pParent->phases;
	}

	/* no phase info */
	if (phases==0)
		throw "phases not specified";

	/* split connection must connect to a phase */
	if (has_phase(PHASE_S) && !(has_phase(PHASE_A) || has_phase(PHASE_B) || has_phase(PHASE_C)))
		throw "split connection is missing A,B, or C phase connection";

	/* split connection must connect to only one phase */
	if (has_phase(PHASE_S) && (
		(has_phase(PHASE_A) && has_phase(PHASE_B)) ||
		(has_phase(PHASE_B) && has_phase(PHASE_C)) ||
		(has_phase(PHASE_C) && has_phase(PHASE_A))))
		throw "split connection is connected to two phases simultaneously";

	/* split connection is not permitted with delta connection */
	if (has_phase(PHASE_S) && has_phase(PHASE_D))
		throw "split and delta connection cannot occur simultaneously";

	/* split connection is not permitted on neutral */
	if (has_phase(PHASE_N) && has_phase(PHASE_S)) 
	{
		gl_warning("neutral phase ignored on split connection.");
		phases ^= PHASE_N;
	}

	return 1;
}

TIMESTAMP powerflow_object::presync(TIMESTAMP t0)
{
	/* nothing to do */
	return TS_NEVER;
}

TIMESTAMP powerflow_object::sync(TIMESTAMP t0)
{
	/* nothing to do */
	return TS_NEVER;
}

TIMESTAMP powerflow_object::postsync(TIMESTAMP t0)
{
#ifdef SUPPORT_OUTAGES
	OBJECT *obj = OBJECTHDR(this);
	if (condition!=OC_NORMAL && solution==PS_NORMAL)
	{
		char buffer[1024]="???";
		gl_get_value_by_name(obj,"condition",buffer,sizeof(buffer));
		gl_debug("powerflow_object %s (%s:%d): abnormal condition detected (condition=%s), switching solver to OUTAGE method", obj->name, obj->oclass->name, obj->id, buffer);
		solution = PS_OUTAGE;
		return t0;
	}
	else if (condition==OC_NORMAL && solution==PS_OUTAGE)
	{
		gl_debug("powerflow_object %s (%s:%d): normal condition restored, switching solver to NORMAL method", obj->name, obj->oclass->name, obj->id);
		solution = PS_NORMAL;
		return t0;
	}
#endif
	return TS_NEVER;
}

int powerflow_object::kmldump(FILE *fp)
{
	return 1; /* 1 means output default if it wasn't handled */
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: powerflow_object
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_powerflow_object(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(powerflow_object::oclass);
		if (*obj!=NULL)
		{
			powerflow_object *my = OBJECTDATA(*obj,powerflow_object);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(powerflow_object);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_powerflow_object(OBJECT *obj)
{
	try {
		powerflow_object *my = OBJECTDATA(obj,powerflow_object);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(powerflow_object);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_powerflow_object(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	powerflow_object *pObj = OBJECTDATA(obj,powerflow_object);
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
	SYNC_CATCHALL(powerflow_object);
}

EXPORT int isa_powerflow_object(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,powerflow_object)->isa(classname);
}

/**@}**/
