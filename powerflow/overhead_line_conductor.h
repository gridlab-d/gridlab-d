// $Id: overhead_line_conductor.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _OVERHEADLINECONDUCTOR_H
#define _OVERHEADLINECONDUCTOR_H

#include "powerflow.h"
#include "line.h"

class overhead_line_conductor : public powerflow_library
{
public:
	static CLASS *pclass;
    static CLASS *oclass;

	/* get_name acquires the name of an object or 'unnamed' if non set */
	inline const char *get_name(void) const { static char tmp[64]; OBJECT *obj=OBJECTHDR(this); return obj->name?obj->name:(sprintf(tmp,"%s:%d",obj->oclass->name,obj->id)>0?tmp:"(unknown)");};
	/* get_id acquires the object's id */
	inline unsigned int get_id(void) const {return OBJECTHDR(this)->id;};

public:
	double geometric_mean_radius;
	double resistance;
	double cable_diameter;
	LINERATINGS winter, summer;
	
	overhead_line_conductor(MODULE *mod);
	inline overhead_line_conductor(CLASS *cl=oclass):powerflow_library(cl){};
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
};

#endif
