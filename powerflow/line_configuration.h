// $Id: line_configuration.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _LINECONFIGURATION_H
#define _LINECONFIGURATION_H

#include "line.h"

class line_configuration : public powerflow_library
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	OBJECT *phaseA_conductor;
	OBJECT *phaseB_conductor;
	OBJECT *phaseC_conductor;
	OBJECT *phaseN_conductor;
	OBJECT *line_spacing;
	
	line_configuration(MODULE *mod);
	inline line_configuration(CLASS *cl=oclass):powerflow_library(cl){};
	int isa(char *classname);
	int create(void);

};

#endif // _LINECONFIGURATION_H
