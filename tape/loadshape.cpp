/** $Id: loadshape.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file loadshape.cpp
	@addtogroup loadshape
	@ingroup tape


 @{
 **/

#if 0

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <cctype>

#include "loadshape.h"

//////////////////////////////////////////////////////////////////////////
// loadshape CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* loadshape::oclass = NULL;
loadshape *loadshape::defaults = NULL;

loadshape::loadshape(MODULE *module) 
{

	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"loadshape",sizeof(loadshape),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_int32, "interval", PADDR(interval),
			PT_int32, "limit", PADDR(limit),
			PT_enumeration, "integral", PADDR(integral),
				PT_KEYWORD, "NONE", LSI_NONE,
				PT_KEYWORD, "TIME", LSI_TIME,
			PT_enumeration, "period", PADDR(period),
				PT_KEYWORD, "NOT_USED", LSP_NOT_USED,
				PT_KEYWORD, "DAILY", LSP_DAILY,
				PT_KEYWORD, "HOURLY", LSP_HOURLY,
				PT_KEYWORD, "WEEKLY", LSP_WEEKLY,
				PT_KEYWORD, "MONTHLY", LSP_MONTHLY,
				PT_KEYWORD, "QUARTERLY", LSP_QUARTERLY,
				PT_KEYWORD, "YEARLY", LSP_YEARLY,
			PT_int32, "period_ex", PADDR(period_ex),
			PT_int32, "samples", PADDR(samples),
			PT_int32, "sample_rate", PADDR(sample_rate),
			PT_enumeration, "sample_mode", PADDR(sample_mode),
				PT_KEYWORD, "NOT_USED", LSS_NOT_USED,
				PT_KEYWORD, "EVERY_MINUTE", LSS_EVERY_MINUTE,
				PT_KEYWORD, "EVERY_HOUR", LSS_EVERY_HOUR,
				PT_KEYWORD, "EVERY_DAY", LSS_EVERY_DAY,
			PT_enumeration, "output_mode", PADDR(output_mode),
				PT_KEYWORD, "CSV", LSO_CSV,
				PT_KEYWORD, "SHAPE", LSO_SHAPE,
				PT_KEYWORD, "PLOT", LSO_PLOT,
				PT_KEYWORD, "PNG", LSO_PNG,
				PT_KEYWORD, "JPG", LSO_JPG,
			PT_enumeration, "state", PADDR(state), PT_ACCESS, PA_REFERENCE,
				PT_KEYWORD, "INIT", TS_INIT,
				PT_KEYWORD, "OPEN", TS_OPEN,
				PT_KEYWORD, "DONE", TS_DONE,
				PT_KEYWORD, "ERROR", TS_ERROR,
			PT_char256, "output_name", PADDR(output_name),
			PT_char256, "property", PADDR(prop),
			PT_char256, "selection", PADDR(selection),
			PT_char1024, "group", PADDR(group),

			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
		memset(this, 0, sizeof(loadshape));
		interval = -1;
		limit = -1;
		
	}
}

int loadshape::create() 
{
	// copy the defaults
	memcpy(this,defaults,sizeof(loadshape));
	return 1;
}

/*	Loadshape objects are considered non-critical to the functioning of the simulation
 *	 as a whole.  In the event that a loadshape fails to initialize, it will enter
 *	 an error state and return TS_NEVER until the simulation ends.
 */
int loadshape::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	
	state = TS_INIT;

	gl_error("loadshape is under development and will not contribute to the simulation results");

	if(prop[0] == 0){
		gl_error("loadshapes require a target property!");
		state = TS_ERROR;
		return 1;
	}

	if(parent == NULL && group[0] == 0){
		gl_error("loadshapes require either a parent object or a target group");
		state = TS_ERROR;
		return 1;
	}

	if(parent != NULL && group[0] != 0){
		gl_warning("loadshape has both parent & group ~ using the parent and not the group");
	}

	/* find sample rate, in seconds */
	if(sample_mode != LSS_NOT_USED){
		switch(sample_mode){
			case LSS_EVERY_MINUTE:
				sample_len = 60;
				break;
			case LSS_EVERY_HOUR:
				sample_len = 3600;
				break;
			case LSS_EVERY_DAY:
				sample_len = 86400;
				break;
			default:
				gl_error("Invalid sample mode!");
				return 0;
		}
	} else if(samples > 0){
		sample_len = 0; /* defer */
	} else if(sample_rate > 0){
		sample_len = sample_rate;
	}

	/* find period length, in seconds */
	if(period != LSP_NOT_USED){
		if(period_ex > 0){ /* using 'period_ex' */
			period_len = period_ex;
		} else {
			sprintf(errmsg, "No period length data found");
			gl_error(errmsg);
			state = TS_ERROR;
			return 1;
		}
	} else if(period_ex > 0){
		if(sample_len > 0){
			period_len = period_ex * sample_len;
		} else {
			sprintf(errmsg, "Cannot use explicit period length (in samples) without defining the sample length with sample_mode or sample_rate");
			gl_error(errmsg);
			state = TS_ERROR;
			return 1;
		}
	}

	/* if defered sample length... */
	if(samples > 0 && period_len > 0){
		if(period_len % samples > 0){
			sprintf(errmsg, "Cannot create %i equal sample periods given a period length of %i seconds!", samples, period_len);
			gl_error(errmsg);
			state = TS_ERROR;
			return 1;
		}
		sample_len = period_len / samples;
	}

	/* generate schedule groupings ... adapt schedule object or migrate schedule to core? */
	if(selection[0] != 0){
		sel_list = parse_cron(selection);
		if(sel_list == NULL){
			gl_error("Unable to parse the selection list ~ ignoring & continuing ungrouped");
		}
	}

	return 1;
}

TIMESTAMP loadshape::presync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

TIMESTAMP loadshape::sync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

int loadshape::commit(){
	return 1; /* success */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT void new_loadshape(MODULE *mod){
	new loadshape(mod);
}

EXPORT int create_loadshape(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(loadshape::oclass);
	if (*obj!=NULL)
	{
		loadshape *my = OBJECTDATA(*obj,loadshape);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_loadshape(OBJECT *obj)
{
	loadshape *my = OBJECTDATA(obj,loadshape);
	return my->init(obj->parent);
}

EXPORT TIMESTAMP sync_loadshape(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	loadshape *my = OBJECTDATA(obj, loadshape);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	
	try {
		switch (pass) 
		{
			case PC_PRETOPDOWN:
				t1 = my->presync(obj->clock, t0);
				break;

			case PC_BOTTOMUP:
				t1 = my->sync(obj->clock, t0);
				obj->clock = t0;
				break;

			default:
				gl_error("house::sync- invalid pass configuration");
				t1 = TS_INVALID; // serious error in exec.c
		}
	} 
	catch (char *msg)
	{
		gl_error("house::sync exception caught: %s", msg);
		t1 = TS_INVALID;
	}
	catch (...)
	{
		gl_error("house::sync exception caught: no info");
		t1 = TS_INVALID;
	}

	obj->clock = t0;
	return t1;
}

EXPORT int commit_loadshape(OBJECT *obj){
	loadshape *my = OBJECTDATA(obj,loadshape);
	return my->commit();
}

/**@}**/

#endif // zero
