// $Id: line_spacing.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _LINESPACING_H
#define _LINESPACING_H

#include "powerflow.h"
#include "powerflow_library.h"
#include "line.h"

class line_spacing : public powerflow_library
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	double distance_AtoB;   // distance between cables A and B in feet
	double distance_BtoC;   // distance between cables B and C in feet
	double distance_AtoC;   // distance between cables A and C in feet
	double distance_AtoN;   // distance between cables A and Neutral in feet
	double distance_BtoN;   // distance between cables B and Neutral in feet
	double distance_CtoN;   // distance between cables C and Neutral in feet
	
	line_spacing(MODULE *mod);
	inline line_spacing(CLASS *cl=oclass):powerflow_library(cl){};
	int create(void);
	int isa(char *classname);
};

#endif // _LINE_H
