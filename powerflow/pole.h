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
public:
	pole(MODULE *);
	int create(void);
	int init(OBJECT *);
	TIMESTAMP presync(TIMESTAMP);
	TIMESTAMP sync(TIMESTAMP);
	TIMESTAMP postsync(TIMESTAMP);
public:
	int isa(char *);
};

#endif // _POLE_H

