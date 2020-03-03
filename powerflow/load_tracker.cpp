#include "load_tracker.h"
#include "powerflow.h"

CLASS* load_tracker::oclass = NULL;

load_tracker::load_tracker(MODULE *mod)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"load_tracker",sizeof(load_tracker),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
		else
			oclass->trl = TRL_PROVEN;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_object, "target",PADDR(target),PT_DESCRIPTION,"target object to track the load of",
			PT_char256, "target_property", PADDR(target_prop),PT_DESCRIPTION,"property on the target object representing the load",
			PT_enumeration, "operation", PADDR(operation),PT_DESCRIPTION,"operation to perform on complex property types",
				PT_KEYWORD,"REAL",(enumeration)REAL,
				PT_KEYWORD,"IMAGINARY",(enumeration)IMAGINARY,
				PT_KEYWORD,"MAGNITUDE",(enumeration)MAGNITUDE,
				PT_KEYWORD,"ANGLE",(enumeration)ANGLE, // If using, please specify in radians
			PT_double, "full_scale", PADDR(full_scale),PT_DESCRIPTION,"magnitude of the load at full load, used for feed-forward control",
			PT_double, "setpoint",PADDR(setpoint),PT_DESCRIPTION,"load setpoint to track to",
			PT_double, "deadband",PADDR(deadband),PT_DESCRIPTION,"percentage deadband",
			PT_double, "damping",PADDR(damping),PT_DESCRIPTION,"load setpoint to track to",
			PT_double, "output", PADDR(output),PT_DESCRIPTION,"output scaling value",
			PT_double, "feedback", PADDR(feedback),PT_DESCRIPTION,"the feedback signal, for reference purposes",
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

int load_tracker::isa(char *classname)
{
	return strcmp(classname,"load_tracker")==0;
}

int load_tracker::create()
{
	operation = MAGNITUDE;
	deadband = 1.0;
	damping = 0.0;
	return 1;
}

int load_tracker::init(OBJECT *parent)
{
	// Make sure we have a target object
	if (target==NULL)
	{
		GL_THROW("Target object not set");
		/* TROUBLESHOOT
		Please specify the name of the target object to be monitored.
		*/
	}

	// Make sure we have a target property
	PROPERTY* target_property = gl_get_property(target,target_prop.get_string());
	if (target_property==NULL)
	{
		GL_THROW("Unable to find property \"%s\" in object %s", target_prop.get_string(), target->name);
		/* TROUBLESHOOT
		Please specify an existing property of the target object to be monitored.
		*/
	}

	// Make sure it is a supported type
	switch (target_property->ptype)
	{
	case PT_double:
	case PT_complex:
	case PT_int16:
	case PT_int32:
	case PT_int64:
		break;
	default:
		GL_THROW("Unsupported property type.  Supported types are complex, double and integer");
		/* TROUBLESHOOT
		Please specify a property whose type is either a double, complex, int16, int32, or an int64.
		*/
	}
	type = target_property->ptype;

	// Get a pointer to the target property value
	switch (type)
	{
	case PT_double:
		pointer.d = gl_get_double_by_name(target, target_prop.get_string());
		break;
	case PT_complex:
		pointer.c = gl_get_complex_by_name(target, target_prop.get_string());
		break;
	case PT_int16:
		pointer.i16 = gl_get_int16_by_name(target, target_prop.get_string());
		break;
	case PT_int32:
		pointer.i32 = gl_get_int32_by_name(target, target_prop.get_string());
		break;
	case PT_int64:
		pointer.i64 = gl_get_int64_by_name(target, target_prop.get_string());
		break;
	}

	// The VALUEPOINTER is a union of pointers so we only need to check one of them....
	if (pointer.d == NULL)
	{
		GL_THROW("Unable to bind to property \"%s\" in object %s", target_prop.get_string(), target->name);
		/* TROUBLESHOOT
		The property given does not exist for the given property object. Please specify an existing property of the target object.
		*/
	}

	// Make sure we have a full_scale value
	if (full_scale == 0.0)
	{
		GL_THROW("The full_scale property must be non-zero");
		/* TROUBLESHOOT
		Please specify full_scale as a non-zero value.
		*/
	}

	// Check deadbank is OK
	if (deadband < 1.0 || deadband > 50.0)
	{
		GL_THROW("Deadband must be in the range 1% to 50%");
		/* TROUBLESHOOT
		Please specify deadband as a value between 1 and 50.
		*/
	}

	// Check damping is OK
	if (damping < 0.0)
	{
		GL_THROW("Damping must greater than or equal to 0.0");
		/* TROUBLESHOOT
		Please specify damping as a value of 0 or greater.
		*/
	}

	// Set the default output value based on feed-forward control
	output = setpoint / full_scale;

	return 1;
}

void load_tracker::update_feedback_variable()
{
	//Locking - lock pointed device
	READLOCK_OBJECT(target);
		switch (type)
		{
		case PT_double:
			feedback = *(pointer.d);
			break;
		case PT_complex:
			{
				switch (operation)
				{
				case REAL:
					feedback = pointer.c->Re();
					break;
				case IMAGINARY:
					feedback = pointer.c->Im();
					break;
				case MAGNITUDE:
					{
						feedback = pointer.c->Mag();
						if (pointer.c->Re() < 0.0)
						{
							feedback *= -1.0;
						}
					}
					break;
				case ANGLE:
					feedback = pointer.c->Arg();
					break;
				}
			}
			break;
		case PT_int16:
			feedback = (double)(*(pointer.i16));
			break;
		case PT_int32:
			feedback = (double)(*(pointer.i32));
			break;
		case PT_int64:
			feedback = (double)(*(pointer.i64));
			break;
		}
	//Unlock
	READUNLOCK_OBJECT(target);
}

TIMESTAMP load_tracker::presync(TIMESTAMP t0)
{
	// We only re-calculate the output variable in the
	// presync before the powerflow solve.  We are going to
	// check for output error after the 
	update_feedback_variable();

	if (setpoint == 0.0)
	{
		// set output to 0.0 and don't do anything else
		output = 0.0;
	}
	else if (feedback == 0.0)
	{
		// can't trust feedback, just use feed forward control
		output = setpoint / full_scale;
	}
	else
	{
		// NOTE: we assume a linear response to scaling.

		if (feedback < setpoint)
		{
			double percent_error = (setpoint-feedback)/full_scale;
			if (percent_error > (deadband/100.0))
			{
				output *= 1.0 + ((setpoint-feedback)/feedback) * (1.0/(1.0+damping));
			}
		}
		else
		{
			double percent_error = (feedback-setpoint)/full_scale;
			if (percent_error > (deadband/100.0))
			{
				output *= 1.0 - ((feedback-setpoint)/feedback) * (1.0/(1.0+damping));
			}
		}
	}

	/* nothing to do */
	return TS_NEVER;
}

TIMESTAMP load_tracker::sync(TIMESTAMP t0)
{
	/* nothing to do */
	return TS_NEVER;
}

TIMESTAMP load_tracker::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	// After both the powerflow solve has completed and the
	// measurments have been updated we check the output error
	// and see if we need to trigger another iteration.
	if ((solver_method == SM_FBS) || (solver_method == SM_NR && NR_admit_change == false))
	{
		update_feedback_variable();

		if (feedback < setpoint)
		{
			double percent_error = (setpoint-feedback)/full_scale;
			if (percent_error > (deadband/100.0))
			{
				//force a new iteration
				return t1;
			}
		}
		else
		{
			double percent_error = (feedback-setpoint)/full_scale;
			if (percent_error > (deadband/100.0))
			{
				//force a new iteration
				return t1;
			}
		}
	}

	/* nothing to do */
	return TS_NEVER;
}

EXPORT int isa_load_tracker(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,load_tracker)->isa(classname);
}

EXPORT int create_load_tracker(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(load_tracker::oclass);
		if (*obj!=NULL)
		{
			load_tracker *my = OBJECTDATA(*obj,load_tracker);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(load_tracker);
	return 0;
}

EXPORT int init_load_tracker(OBJECT *obj)
{
	try {
		load_tracker *my = OBJECTDATA(obj,load_tracker);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(load_tracker);
}

EXPORT TIMESTAMP sync_load_tracker(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	load_tracker *pObj = OBJECTDATA(obj,load_tracker);
	try {
		TIMESTAMP t1;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(t0);
		case PC_BOTTOMUP:
			return pObj->sync(t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(obj->clock,t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
		throw "invalid pass request";
	}
	SYNC_CATCHALL(load_tracker);
}