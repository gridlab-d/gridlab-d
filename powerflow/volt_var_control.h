// $Id: volt_var_control object 8/24/2009

#ifndef _VOLT_VAR_CONTROL_H
#define _VOLT_VAR_CONTROL_H

#include "powerflow.h"
#include "node.h"
#include "link.h"
#include "capacitor.h"
#include "regulator.h"
#include "transformer.h"

class volt_var_control : public node
{
public:
	// All of your user or other module accesible variables go here
	double qualification_time;
private:
	// Any "hidden" variables go here

public:
	static CLASS *pclass;
	static CLASS *oclass;

public:
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0);
	//TIMESTAMP postsync(TIMESTAMP t0);
	volt_var_control(MODULE *mod);
	inline volt_var_control(CLASS *cl=oclass):node(cl){};
};

#endif