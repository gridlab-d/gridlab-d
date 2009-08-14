/** $Id: relay.cpp 1186 2009-01-02 18:15:30Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file relay.cpp
	@addtogroup powerflow_relay Relay
	@ingroup powerflow

	Implements a relay object.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "relay.h"
#include "node.h"

//////////////////////////////////////////////////////////////////////////
// relay CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* relay::oclass = NULL;
CLASS* relay::pclass = NULL;

relay::relay(MODULE *mod) : link(mod)
{
	if(oclass == NULL)
	{
		pclass = link::oclass;
		
		oclass = gl_register_class(mod,"relay",sizeof(relay),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
        if(oclass == NULL)
            GL_THROW("unable to register object class implemented by %s",__FILE__);
        
        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
			PT_double, "time_to_change[s]", PADDR(time_to_change),
			PT_double, "recloser_delay[s]", PADDR(recloser_delay),
			PT_int16, "recloser_tries", PADDR(recloser_tries),
			PT_int16, "recloser_limit", PADDR(recloser_limit),
            NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int relay::isa(char *classname)
{
	return strcmp(classname,"relay")==0 || link::isa(classname);
}

int relay::create()
{
	int result = link::create();
	recloser_delay = 0;
	recloser_tries = 0;
	recloser_limit = 0;
	return result;
}

int relay::init(OBJECT *parent)
{
	int result = link::init(parent);

	if (recloser_limit == 0)
	{
		recloser_limit = 5;
		gl_warning("Recloser:%d tries limit was not specified, defaulted to 5 tries",OBJECTHDR(this)->id);
		/*  TROUBLESHOOT
		The recloser did not have a specified value for the maximum number of recloser tries.  5 was selected as a default value. 
		*/
	}

	if (recloser_delay == 0)
	{
		recloser_delay = 3;
		gl_warning("Recloser:%d reclose delay was not specified, defaulted to 3 seconds",OBJECTHDR(this)->id);
		/*  TROUBLESHOOT
		The recloser did not have a specified value for the recloser try delay.  3 was selected as a default value. 
		*/
	}


	if (recloser_delay<=1.0)
		throw "recloser delay must be at least 1 second";
	
	a_mat[0][0] = d_mat[0][0] = A_mat[0][0] = (link::is_closed() && has_phase(PHASE_A) ? 1.0 : 0.0);
	a_mat[1][1] = d_mat[1][1] = A_mat[1][1] = (link::is_closed() && has_phase(PHASE_B) ? 1.0 : 0.0);
	a_mat[2][2] = d_mat[2][2] = A_mat[2][2] = (link::is_closed() && has_phase(PHASE_C) ? 1.0 : 0.0);

	b_mat[0][0] = c_mat[0][0] = B_mat[0][0] = 0.0;
	b_mat[1][1] = c_mat[1][1] = B_mat[1][1] = 0.0;
	b_mat[2][2] = c_mat[2][2] = B_mat[2][2] = 0.0;

	return result;
}

TIMESTAMP relay::sync(TIMESTAMP t0)
{
	TIMESTAMP t1 = TS_NEVER;
	node *f;
	node *t;
	set reverse = get_flow(&f,&t);
#ifdef SUPPORT_OUTAGES
	set trip = (f->is_contact_any() || t->is_contact_any());

	/* perform relay operation if any line contact has occurred */
	if (status==LS_CLOSED && trip)
	{
		status = LS_OPEN;

		/* schedule recloser operation */
		t1 = t0+(TIMESTAMP)(recloser_delay*TS_SECOND);
	}

	/* recloser time has arrived */
	else if (status==LS_OPEN && t0>=reclose_time)
	{
		/* still have contact */
		if (trip)
		{
			/* reschedule automatic recloser if retries permits */ 
			if (recloser_limit>recloser_tries)
			{
				recloser_tries++;
				t1 = t0+(TIMESTAMP)(recloser_delay*TS_SECOND);
			}

			/* automatic retries exhausted, manual takes an average of an hour */
			else
			{
				t1 = t0+(TIMESTAMP)(gl_random_exponential(3600)*TS_SECOND);
			}
		}
		else
			status = LS_CLOSED;
	}
#endif

	TIMESTAMP t2=link::sync(t0);

	return t1<t2?t1:t2;
}

TIMESTAMP relay::postsync(TIMESTAMP t0)
{
	return link::postsync(t0);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: relay
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_relay(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(relay::oclass);
		if (*obj!=NULL)
		{
			relay *my = OBJECTDATA(*obj,relay);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_relay: %s", msg);
	}
	return 0;
}

EXPORT int commit_relay(OBJECT *obj)
{
	if (solver_method==SM_FBS)
	{
		relay *plink = OBJECTDATA(obj,relay);
		plink->calculate_power();
	}
	return 1;
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_relay(OBJECT *obj)
{
	relay *my = OBJECTDATA(obj,relay);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		GL_THROW("%s (relay:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_relay(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	relay *pObj = OBJECTDATA(obj,relay);
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
	} catch (const char *error) {
		GL_THROW("%s (relay:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return 0; 
	} catch (...) {
		GL_THROW("%s (relay:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
		return 0;
	}
}

EXPORT int isa_relay(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,relay)->isa(classname);
}

/**@}**/
