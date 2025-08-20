/** $Id: appliance.h 4738 2014-07-03 00:55:39Z dchassin $
    Copyright (C) 2012 Battelle Northwest
 **/

#ifndef _APPLIANCE_H
#define _APPLIANCE_H

#include "residential_enduse.h"

class appliance : public residential_enduse
{
private:
	GL_STRUCT(Eigen::MatrixXcd,power);
	GL_STRUCT(Eigen::MatrixXcd,impedance);
	GL_STRUCT(Eigen::MatrixXcd,current);
	GL_STRUCT(Eigen::MatrixXd,duration);
	GL_STRUCT(Eigen::MatrixXd,transition);
	GL_STRUCT(Eigen::MatrixXd,heatgain);
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
