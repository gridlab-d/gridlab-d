// $Id: triplex_line_configuration.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TRIPLEXLINECONFIGURATION_H
#define _TRIPLEXLINECONFIGURATION_H

#include "line.h"

class triplex_line_configuration : public line_configuration
{
public:
	double ins_thickness;
	double diameter;
	
public:
	static CLASS *oclass;
	static CLASS *pclass;
	
	triplex_line_configuration(MODULE *mod);
	inline triplex_line_configuration(CLASS *cl=oclass):line_configuration(cl){};
	int isa(char *classname);
	int create(void);	
};

#endif // _TRIPLEXLINECONFIGURATION_H
