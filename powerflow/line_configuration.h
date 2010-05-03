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
	complex impedance11;	// Used when defining the z-bus matrix explicitely.
	complex impedance12;	// For ABC phasing, 1-A, 2-B, 3-C; For triplex 1-ph1, 2-ph2, 3-0
	complex impedance13;
	complex impedance21;
	complex impedance22;
	complex impedance23;
	complex impedance31;
	complex impedance32;
	complex impedance33;
	
	line_configuration(MODULE *mod);
	inline line_configuration(CLASS *cl=oclass):powerflow_library(cl){};
	int isa(char *classname);
	int create(void);

};

#endif // _LINECONFIGURATION_H
