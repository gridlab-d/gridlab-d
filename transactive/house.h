// transactive/house.h
// Copyright (C) 2016, Stanford University
// Author: dchassin@slac.stanford.edu

#ifndef _HOUSE_H
#define _HOUSE_H

#include "gridlabd.h"
#include "../powerflow/triplex_meter.h"

using namespace arma;
DECL_METHOD(house,connect);

class house : public gld_object {
public:
	typedef enum {SM_OFF=0, SM_HEAT=1, SM_AUX=2, SM_COOL=3, N_SYSTEMMODES} SYSTEMMODE;
	GL_ATOMIC(double,UA);
	GL_ATOMIC(double,CA);
	GL_ATOMIC(double,UM);
	GL_ATOMIC(double,CM);
	GL_ATOMIC(double_array,QH); // two modes (SM_HEAT and SM_AUX)
	GL_ATOMIC(double_array,QC); // one mode (SM_COOL)
	GL_ATOMIC(double_array,COP); // three modes (SM_HEAT, SM_AUX, SM_COOL)
	GL_ATOMIC(double,QI);
	GL_ATOMIC(double,TA);
	GL_ATOMIC(double,TM);
	GL_ATOMIC(enumeration,system_mode); // current mode
	GL_METHOD(house,connect);
private:
	mat::fixed<2,2> A; // state transition matrix
	vec::fixed<2> B[N_SYSTEMMODES]; // input matrix (follows system_mode)
	rowvec::fixed<2> C; // output matrix
	double u; // input
	vec::fixed<2> x; // state
	double y; // output
	gld_property sell;
public:
	/* required implementations */
	house(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t1);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);

public:
	static CLASS *oclass;
	static house *defaults;
};

#endif // _HOUSE_H
