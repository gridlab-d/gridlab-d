/** $Id $
	Copyright (C) 2008 Battelle Memorial Institute
	@file residential_enduse.cpp
	@addtogroup residential_enduse Residential enduses
	@ingroup residential

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "residential.h"
#include "residential_enduse.h"

//////////////////////////////////////////////////////////////////////////
// CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* residential_enduse::oclass = NULL;

// the constructor registers the class and properties and sets the defaults
residential_enduse::residential_enduse(MODULE *mod)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gld_class::create(mod,"residential_enduse",sizeof(residential_enduse),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the residential_enduse class failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_loadshape,"shape",PADDR(shape),
			PT_enduse,"",PADDR(load),
			PT_enumeration,"override",PADDR(re_override),
				PT_KEYWORD,"ON",(enumeration)OV_ON,
				PT_KEYWORD,"NORMAL",(enumeration)OV_NORMAL,
				PT_KEYWORD,"OFF",(enumeration)OV_OFF,
			PT_enumeration, "power_state", PADDR(power_state),
				PT_KEYWORD, "UNKNOWN", (enumeration)PS_UNKNOWN,
				PT_KEYWORD, "OFF", (enumeration)PS_OFF,
				PT_KEYWORD, "ON", (enumeration)PS_ON,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the residential_enduse properties failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */
	}
}

// create is called every time a new object is loaded
int residential_enduse::create(bool connect_shape) 
{
	// attach loadshape 
	load.end_obj = my();
	if (connect_shape) load.shape = &shape;
	load.breaker_amps = 20;
	load.config = 0;
	load.heatgain_fraction = 1.0; /* power has no effect on heat loss */
	re_override = OV_NORMAL;
	power_state = PS_UNKNOWN;
	return 1;
}

int residential_enduse::init(OBJECT *parent)
{
	set_flags(get_flags()|OF_SKIPSAFE);
	gld_object *pParent = OBJECTDATA(parent,gld_object);

	//	pull parent attach_enduse and attach the enduseload
	if ( pParent!=NULL && pParent->is_valid() )
	{
		ATTACHFUNCTION attach = (ATTACHFUNCTION)pParent->get_function("attach_enduse");
		if ( attach )
			pCircuit = (*attach)(parent, &load, load.breaker_amps, (load.config&EUC_IS220)!=0);
		else
			gl_warning("%s (%s:%d) parent %s (%s:%d) does not export attach_enduse function so voltage response cannot be modeled", get_name(), get_oclass()->get_name(), get_id(), pParent->get_name(), pParent->get_oclass()->get_name(), pParent->get_id());
			/* TROUBLESHOOT
				Enduses must have a voltage source from a parent object that exports an attach_enduse function.  
				The residential_enduse object references a parent object that does not conform with this requirement.
				Fix the parent reference and try again.
			 */
	}

	if (load.shape!=NULL) {
		if (load.shape->schedule==NULL)
		{
			gl_verbose("%s (%s:%d) schedule is not specified so the load may be inactive", get_name(), get_oclass()->get_name(), get_id());
			/* TROUBLESHOOT
				The residential_enduse object requires a schedule that defines how
				the load behaves.  Omitting this schedule effectively shuts the enduse
				load off and this is not typically intended.
			 */
		}
	}

	return 1;
}

int residential_enduse::isa(char *classname){
	return strcmp(classname,"residential_enduse")==0;
}

TIMESTAMP residential_enduse::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	gl_debug("%s shape load = %8g", get_name(), gl_get_loadshape_value(&shape));
	if (load.voltage_factor>1.2 || load.voltage_factor<0.8)
		gl_verbose("%s voltage is out of normal +/- 20%% range of nominal (vf=%.2f)", get_name(), load.voltage_factor);
		/* TROUBLESHOOT
		   The voltage on the enduse circuit is outside the expected range for that enduse.
		   This is usually caused by an impropely configure circuit (e.g., 110V on 220V or vice versa).
		   Fix the circuit configuration for that enduse and try again.
		 */
	return shape.t2>t1 ? shape.t2 : TS_NEVER; 
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_residential_enduse(OBJECT **obj, OBJECT *parent)
{
	try {
		*obj = gl_create_object(residential_enduse::oclass);
		if (*obj!=NULL)
		{
			residential_enduse *my = OBJECTDATA(*obj,residential_enduse);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(residential_enduse);
}

EXPORT int init_residential_enduse(OBJECT *obj)
{
	try {
		residential_enduse *my = OBJECTDATA(obj,residential_enduse);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(residential_enduse);
}

EXPORT int isa_residential_enduse(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,residential_enduse)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_residential_enduse(OBJECT *obj, TIMESTAMP t1)
{
	residential_enduse *my = OBJECTDATA(obj,residential_enduse);
	try {
		TIMESTAMP t2 = my->sync(obj->clock, t1);
		obj->clock = t1;
		return t2;
	}
	SYNC_CATCHALL(residential_enduse);
}

/**@}**/
