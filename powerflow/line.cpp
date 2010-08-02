/** $Id: line.cpp 1186 2009-01-02 18:15:30Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file line.cpp
	@addtogroup line 
	@ingroup powerflow

	This file contains the definition for 4 types of lines, their configurations
	and the conductors associated with these lines.  The line is one of the major
	parts of the method used for solving a powerflow network.  In essense the
	distribution network can be seen as a series of nodes and links.  The links
	(or lines as in this file) do most of the work, solving a series of 3
	dimensional matrices, while the nodes (defined in node.cpp) are containers
	and aggregators, storing the values to be used in the equation solvers.
	
	In general, a line contains a configuration that contains a spacing object, and
	one or more conductor objects, exported as phaseA_conductor, phaseB_conductor, 
	phaseC_conductor, and phaseN_conductor.  The line spacing object hold 
	information about the distances between each of the phase conductors, while the 
	conductor objects contain the information on the specifics of the conductor 
	(resistance, diameter, etc...)
	
	Line itself is not exported, and cannot be used in a model.  Instead one of 3
	types of lines are exported for use in models; underground lines, overhead lines,
	and triplex (or secondary) lines.  Underground lines and overhead lines use the
	basic line configuration and line spacing objects, but have different conductor
	classes defined for them.  The overhead line conductor is the simplest of these
	with just a resistance and a geometric mean radius defined.  The underground
	line conductor is more complex, due to the fact that each phase conductor may
	have its own neutral.  
	
	Triplex lines are the third type of line exported.  A special triplex line
	configuration is also exported which presents 3 line conductors; l1_conductor,
	l2_conductor, and lN_conductor.  Also exported is a general insulation 
	thickness and the total diameter of the line.  The conductor for triplex lines
	is much the same as the overhead line conductor.
	
	All line objects export a phase property that is a set of phases and may be 
	set using the bitwise or operator (A|B|C for a 3 phase line).
	
	@{
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
using namespace std;

#include "node.h"
#include "line.h"

CLASS* line::oclass = NULL;
CLASS* line::pclass = NULL;
CLASS *line_class = NULL;

line::line(MODULE *mod) : link(mod) {
	if(oclass == NULL)
	{
		pclass = link::oclass;
		
		line_class = oclass = gl_register_class(mod,"line",sizeof(line),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
        if(oclass == NULL)
            GL_THROW("unable to register object class implemented by %s",__FILE__);
        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
			PT_object, "configuration",PADDR(configuration),
			PT_double, "length[ft]",PADDR(length),
			NULL) < 1) GL_THROW("unable to publish line properties in %s",__FILE__);
	}
}

int line::create()
{
	int result = link::create();

	configuration = NULL;
	length = 0;

	return result;
}

int line::init(OBJECT *parent)
{
	int result = link::init(parent);
	

	node *pFrom = OBJECTDATA(from,node);
	node *pTo = OBJECTDATA(to,node);

	/* check for node nominal voltage mismatch */
	if ((pFrom->nominal_voltage - pTo->nominal_voltage) > fabs(0.001*pFrom->nominal_voltage))
		throw "from and to node nominal voltage mismatch of greater than 0.1%";

	if (solver_method == SM_NR && length == 0.0)
		throw "Newton-Raphson method does not support zero length lines at this time";

	return result;
}

int line::isa(char *classname)
{
	return strcmp(classname,"line")==0 || link::isa(classname);
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: line
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/



/* This can be added back in after tape has been moved to commit
EXPORT int commit_line(OBJECT *obj)
{
	if (solver_method==SM_FBS)
	{
		line *plink = OBJECTDATA(obj,line);
		plink->calculate_power();
	}
	return 1;
}
*/
EXPORT int create_line(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(line::oclass);
		if (*obj!=NULL)
		{
			line *my = OBJECTDATA(*obj,line);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("create_line: %s", msg);
	}
	return 0;
}

EXPORT TIMESTAMP sync_line(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	line *pObj = OBJECTDATA(obj,line);
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
		gl_error("%s (line:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return TS_INVALID; 
	} catch (...) {
		gl_error("%s (line:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
		return TS_INVALID;
	}
}

EXPORT int init_line(OBJECT *obj)
{
	line *my = OBJECTDATA(obj,line);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s (line:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
}

EXPORT int isa_line(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,line)->isa(classname);
}

/**@}**/
