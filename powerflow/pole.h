// powerflow/pole.h
// Copyright (C) 2018, Stanford University

#ifndef _POLE_H
#define _POLE_H

#include "node.h"

class pole : public node
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	enum {PT_WOOD=0, PT_STEEL=1, PT_CONCRETE=2};
	enumeration pole_type;
	double tilt_angle;
	double tilt_direction;
	object weather;
public: // wood pole model parameters
	double design_ice_thickness; 		// (see Chart 1)
	double design_wind_loading; 		// (see Chart 1)
	double design_temperature; 		// (see Chart 1)
	// overload factors (see Chart 2)
	double overload_factor_vertical; 	
	double overload_factor_transverse_general; 	
	double overload_factor_transverse_crossing; 	
	double overload_factor_transverse_wire; 	
	double overload_factor_longitudinal_general; 	
	double overload_factor_longitudinal_deadend; 	
	// strength factors (see Chart 3)
	double strength_factor_250b_wood; 	
	double strength_factor_250b_support; 	
	double strength_factor_250c_wood; 	
	double strength_factor_250c_support; 	
	double cable_diameter;		// (see Section F)
	double ice_thickness;		// (see Section F)
	double pole_length; 		// (see Chart 4)
	double pole_depth;		// (see Section A)
	double ground_diameter; 	// (see Chart 4)
	double top_diameter; 		// (see Chart 4)
	double fiber_strength; 		// (see Chart 5)
	double equipment_area;		// (see Section E)
	double equipment_height;	// (see Section E)
	double resisting_moment; 	// (see Section B)
	double pole_moment;		// (see Section D)
	double equipment_moment;	// (see Section E)
	double wire_load;		// (see Section F)
	double wire_moment;		// (see Section F)
	double wind_pressure;		// (see Section D)
	double cable_height;		// (see Section F)
	object cable_configuration;
private:
	double *wind_speed;
	double *wind_direction;
	double *wind_gust;
public:
	pole(MODULE *);
	int isa(char *);
	int create(void);
	int init(OBJECT *);
	TIMESTAMP presync(TIMESTAMP);
	TIMESTAMP sync(TIMESTAMP);
	TIMESTAMP postsync(TIMESTAMP);
public:
	double get_pole_diameter(double height);
};

#endif // _POLE_H

