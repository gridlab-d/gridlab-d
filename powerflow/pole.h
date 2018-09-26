// powerflow/pole.h
// Copyright (C) 2018, Stanford University

#ifndef _POLE_H
#define _POLE_H

#include "node.h"
#include "pole_configuration.h"

class pole : public node
{
public:
	typedef struct s_wiredata {
		double height;
		double diameter;
	} WIREDATA;
	typedef enum e_wiretype {
		WT_PHASEA, WT_PHASEB, WT_PHASEC, WT_NEUTRAL, WT_PHONE, WT_TV, _N_WIRETYPES,
	} CONDUCTOR;
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	enum {PT_WOOD=0, PT_STEEL=1, PT_CONCRETE=2};
	enumeration pole_type;
	double tilt_angle;
	double tilt_direction;
	object weather;
	object configuration;
	double equipment_area;		// (see Section E)
	double equipment_height;	// (see Section E)
private:
	double ice_thickness;
	double wind_loading;
	double resisting_moment; 	// (see Section B)
	double pole_moment;		// (see Section D)
	double equipment_moment;	// (see Section E)
	double wire_load;		// (see Section F)
	double wire_moment;		// (see Section F)
	double wind_pressure;		// (see Section D)
	double cable_height;		// (see Section F)
	object cable_configuration;
private:
	pole_configuration *config;
	double last_wind_speed;
	double *wind_speed;
	double *wind_direction;
	double *wind_gust;
	WIREDATA wire_data[_N_WIRETYPES];
public:
	pole(MODULE *);
	int isa(char *);
	int create(void);
	int init(OBJECT *);
	TIMESTAMP presync(TIMESTAMP);
	TIMESTAMP sync(TIMESTAMP);
	TIMESTAMP postsync(TIMESTAMP);
};

#endif // _POLE_H

