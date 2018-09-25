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
	double ice_thickness; 		// (see Chart 1)
	double wind_loading; 		// (see Chart 1)
	double temperature; 		// (see Chart 1)
	double overload_factor; 	// (see Chart 2)
	double strength_factor; 	// (see Chart 3)
	double pole_length; 		// (see Chart 4)
	double ground_depth;		// (see Section A)
	double ground_diameter; 	// (see Chart 4)
	double top_diameter; 		// (see Chart 4)
	double fiber_strength; 		// (see Chart 5)
	double resisting_moment; 	// (see Section B)
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

