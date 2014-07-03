/** $Id: appliance.h 4738 2014-07-03 00:55:39Z dchassin $
    Copyright (C) 2012 Battelle Northwest
 **/

#ifndef _APPLIANCE_H
#define _APPLIANCE_H

#include "residential_enduse.h"

class appliance : public residential_enduse
{
private:
	GL_STRUCT(complex_array,power);
	GL_STRUCT(complex_array,impedance);
	GL_STRUCT(complex_array,current);
	GL_STRUCT(double_array,duration);
	GL_STRUCT(double_array,transition);
	GL_STRUCT(double_array,heatgain);
private:
	TIMESTAMP next_t;
	unsigned int n_states;
	unsigned int state;
	double *transition_probabilities;
private:
	void update_next_t(void);
	void update_power(void);
	void update_state(void);
public:
	appliance(MODULE *module);
	~appliance();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	int precommit(TIMESTAMP t1);
	inline TIMESTAMP presync(TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t1) { return TS_NEVER; };
	inline TIMESTAMP postsync(TIMESTAMP t1) { return TS_NEVER; };
	int prenotify(PROPERTY *prop, char *value){ return 1;} ;
	int postnotify(PROPERTY *prop, char *value);
public:
	static CLASS *oclass, *pclass;
	static appliance *defaults;
};

#endif
