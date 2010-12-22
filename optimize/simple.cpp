/** $Id: auction.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file auction.cpp
	@defgroup auction Template for a new object class
	@ingroup market

	The auction object implements the basic auction. 

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "gridlabd.h"
#include "simple.h"

CLASS *simple::oclass = NULL;
simple *simple::defaults = NULL;

static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_POSTTOPDOWN;

/* Class registration is only called once to register the class with the core */
simple::simple(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"simple",sizeof(simple),passconfig);
		if (oclass==NULL)
		{
			static char msg[256];
			sprintf(msg, "unable to register object class implemented by %s", __FILE__);
			throw msg;
		}

		if (gl_publish_variable(oclass,
			PT_char1024, "objective", PADDR(objective), PT_DESCRIPTION, "Optimization objective value",
			PT_char1024, "variable", PADDR(variable), PT_DESCRIPTION, "Optimization decision variables",
			PT_char1024, "constraint", PADDR(constraint), PT_DESCRIPTION, "Optimization constraints",
			PT_double, "delta", PADDR(delta), PT_DESCRIPTION, "Change applied to decision variable",
			PT_double, "epsilon", PADDR(epsilon), PT_DESCRIPTION, "Precision of objective value",
			PT_int32, "trials", PADDR(trials), PT_DESCRIPTION, "Limits on number of trials",
			NULL)<1)
		{
			static char msg[256];
			sprintf(msg, "unable to publish properties in %s",__FILE__);
			throw msg;
		}

		defaults = this;
		memset(this,0,sizeof(simple));
	}
}


int simple::isa(char *classname)
{
	return strcmp(classname,"simple")==0;
}

/* Object creation is called once for each object that is created by the core */
int simple::create(void)
{
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int simple::init(OBJECT *parent)
{
	OBJECT *my = OBJECTHDR(this);
	char buffer[1024];
	struct {
		char *name;
		char *param;
		double **var;
		CONSTRAINTOP *op;
		double *value;
	} map[] = {
		{"objective",	objective,	&pObjective},
		{"variable",	variable,	&pVariable},
		{"constraint",	constraint,	&pConstraint, &(constrain.op), &(constrain.value)},
	};
	int n;
	for ( n=0 ; n<sizeof(map)/sizeof(map[0]) ; n++ )
	{
		char oname[1024];
		char pname[1024];
		OBJECT *obj;
		PROPERTY *prop;
		if (strcmp(map[n].param,"")!=0 && sscanf(map[n].param,"%[^.:].%s",oname,pname)!=2)
		{
			gl_error("%s could not be parsed, expected term in the form 'objectname'.'propertyname'", map[n].name);
			return 0;
		}
		
		// find the object
		obj = gl_get_object(oname);
		if ( obj==NULL )
		{
			gl_error("object '%s' could not be found", oname);
			return 0;
		}

		// set the object's rank
		gl_set_rank(my,obj->rank+1);

		// if not a constraint
		if ( map[n].op==NULL )
		{
			// get property
			prop = gl_get_property(obj,pname);
			if ( prop==NULL )
			{
				gl_error("property '%s' could not be found in object '%s'", pname, oname);
				return 0;
			}
			if ( prop->ptype!=PT_double )
			{
				gl_error("property '%s' in object '%s' is not a double", pname, oname);
				return 0;
			}
			*(map[n].var) = (double*)gl_get_addr(obj,pname);;
		}

		// parse constraint
		else {
			// TODO
		}
	}

	// check delta and epsilon 
	if ( delta<=0 )
	{
		gl_error("Decision variable 'delta' in simple optimizer object '%s' must be a positive value", gl_name(my,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}
	if ( epsilon<=0 )
	{
		gl_error("Objective 'epsilon' in simple optimizer object '%s' must be a positive value", gl_name(my,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}

	// read the current values
	if ( pObjective!=NULL )
		last_y = *pObjective;
	else
	{
		gl_error("The property 'objective' must be set in simple optimizer object '%s'", gl_name(my,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}
	if ( pVariable!=NULL)
		next_x = last_x = *pVariable;
	else
	{
		gl_error("The property 'variable' must be set in simple optimizer object '%s'", gl_name(my,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}

	// check the trial limit
	if ( trials<=0 )
	{
		gl_error("The trial limit 'trials' in simple optimizer object '%s' must be a positive integer value", gl_name(my,buffer,sizeof(buffer))?buffer:"???");
		return 0;
	}

	return 1; /* return 1 on success, 0 on failure */
}

	/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP simple::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	// if more tries needed 
	switch (pass) {
	case 0:
		last_x = next_x;
		*pVariable = next_x - delta;
		break;
	case 1:
		*pVariable = next_x;
		break;
	case 2:
		*pVariable = next_x + delta;
		break;
	default:
		break;
	}
	return TS_NEVER; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP simple::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *my = OBJECTHDR(this);
	char buffer[1024];

	// trial limit reached or objective cannot be calculated
	if ( trial>trials )
	{
		gl_error("The trial limit of %d in simple optimizer object '%s' has been reached", trials, gl_name(my,buffer,sizeof(buffer))?buffer:"???");
		return TS_INVALID; /* stop */
	}
	if ( isnan(*pObjective) || !isfinite(*pObjective) ) 
	{
		gl_error("The objective '%s' in simple optimizer object '%s' is infinite or indeterminate", objective, gl_name(my,buffer,sizeof(buffer))?buffer:"???");
		return TS_INVALID; /* stop */
	}

	switch ( pass ) {
	case 0:
		// zero order values
		last_y = *pObjective;
		pass = 1;
		return t1; // need another pass
	case 1:
		// first order values
		last_dy = (*pObjective - last_y)/delta;
		last_y = *pObjective;
		pass = 2;
		return t1; // need another pass
	case 2: 
	{	// second order values
		double dy = (*pObjective - last_y)/delta;
		double ddy = (dy - last_dy)/delta;
		dy = (dy+last_dy)/2;
		gl_verbose("y' = %.4f", dy);
		gl_verbose("y\" = %.4f", ddy);
		if ( fabs(dy)<epsilon )
		{
			pass = 0;
			return TS_NEVER; // done
		}
		if ( ddy==0 )
		{
			gl_error("The objective '%s' in simple optimizer object '%s' does not appear to have a non-zero second derivative", objective, gl_name(my,buffer,sizeof(buffer))?buffer:"???");
			return TS_INVALID;
		}
		next_x = last_x - dy/ddy;
		*pVariable = last_x;
		gl_verbose("x <- %.4f", next_x);
		pass = 0;
		trials++;
		return t1;
	} 
	default:
		pass=0;
		return TS_NEVER;
	}
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_simple(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(simple::oclass);
		if (*obj!=NULL)
		{
			simple *my = OBJECTDATA(*obj,simple);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_simple: %s", msg);
		return 0;
	}
	catch (const char *msg)
	{
		gl_error("create_simple: %s", msg);
		return 0;
	}
	return 1;
}

EXPORT int init_simple(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,simple)->init(parent);
	}
	catch (char *msg)
	{
		char name[64];
		gl_error("init_simple(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
		return 0;
	}
	catch (const char *msg)
	{
		char name[64];
		gl_error("init_simple(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
		return 0;
	}
	return 1;
}

EXPORT int isa_simple(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,simple)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_simple(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	simple *my = OBJECTDATA(obj,simple);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
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
		char name[64];
		gl_error("sync_simple(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
		t2 = TS_INVALID;
	}
	catch (const char *msg)
	{
		char name[64];
		gl_error("sync_simple(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
		t2 = TS_INVALID;
	}
	return t2;
}

