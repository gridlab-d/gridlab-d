/** $Id: generator.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 **/

#ifndef _GENERATOR_H
#define _GENERATOR_H

class generator {
public:
	double Pdesired_MW;		// desired real power output
	double Qdesired_MVAR;	// desired reactive power output (if Q controlled)
	int32 Qcontrolled;		// flag true if reactive power is controlled (requires PQ bus)
	double Pmax_MW;			// maximum real power
	double Qmin_MVAR;		// maximum reactive power possible
	double Qmax_MVAR;		// minimum reactive power possible
	double QVa, QVb, QVc;	// voltage response parameters (Q = a*V^2 + b*V + c)
	enum {STOPPED=0, STANDBY=1, ONLINE=2, TRIPPED=3} state;
private:
	//int generator_state_to_string(void *addr, char *value, int size);
	//int generator_state_from_string(void *addr, char *value);
public:
	static CLASS *oclass;
	static generator *defaults;
	static CLASS *pclass;
public:
	generator(MODULE *mod);
	int create();
	int init(node *parent);
	TIMESTAMP sync(TIMESTAMP t0);
};

GLOBAL CLASS *generator_class INIT(NULL);
GLOBAL OBJECT *last_generator INIT(NULL);

#endif
