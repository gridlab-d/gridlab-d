// $Id: line_configuration.h 4738 2014-07-03 00:55:39Z dchassin $
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
	gld::complex impedance11;	// Used when defining the z-bus matrix explicitely.
	gld::complex impedance12;	// For ABC phasing, 1-A, 2-B, 3-C; For triplex 1-ph1, 2-ph2, 3-0
	gld::complex impedance13;
	gld::complex impedance21;
	gld::complex impedance22;
	gld::complex impedance23;
	gld::complex impedance31;
	gld::complex impedance32;
	gld::complex impedance33;
	double  capacitance11;	// Used when defining the y-bus matrix explicitely.
	double  capacitance12;	// For ABC phasing, 1-A, 2-B, 3-C; For triplex 1-ph1, 2-ph2, 3-0
	double  capacitance13;
	double  capacitance21;
	double  capacitance22;
	double  capacitance23;
	double  capacitance31;
	double  capacitance32;
	double  capacitance33;
	LINERATINGS winter, summer;
	
	line_configuration(MODULE *mod);
	inline line_configuration(CLASS *cl=oclass):powerflow_library(cl){};
	int isa(char *classname);
	int create(void);

};

#endif // _LINECONFIGURATION_H
