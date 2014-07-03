// $Id: line_spacing.h 4738 2014-07-03 00:55:39Z dchassin $
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

	double distance_AtoE;	// distance bewteen cable A and the earth in feet
	double distance_BtoE;	// distance between cable B and the earth in feet
	double distance_CtoE;	// distance between cable C and the earth in feet
	double distance_NtoE;	// distance between cable Neutral and the earth in feet
	
	line_spacing(MODULE *mod);
	inline line_spacing(CLASS *cl=oclass):powerflow_library(cl){};
	int create(void);
	int isa(char *classname);
};

#endif // _LINE_H
