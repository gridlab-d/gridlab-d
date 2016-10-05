// $Id: underground_line_conductor.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _UNDERGROUNDLINECONDUCTOR_H
#define _UNDERGROUNDLINECONDUCTOR_H

#include "line.h"

class underground_line_conductor : public powerflow_library
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	double outer_diameter;
	double conductor_gmr;
	double conductor_diameter;
	double conductor_resistance;
	double neutral_gmr;
	double neutral_diameter;
	double neutral_resistance;
	int16  neutral_strands;
	double insulation_rel_permitivitty;
	double shield_gmr;
	double shield_resistance;
	double shield_thickness;
	double shield_diameter;
	LINERATINGS winter, summer;
	
	underground_line_conductor(MODULE *mod);
	inline underground_line_conductor(CLASS *cl=oclass):powerflow_library(cl){};
	int isa(char *classname);
	int create(void);
	int init(OBJECT *parent);
};

#endif // _UNDERGROUNDLINECONDUCTOR_H
