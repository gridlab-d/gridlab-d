/** $Id: link.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 **/


#ifndef _LINK_H
#define _LINK_H

#include "node.h"

class link {
public:
	complex Y; /* impedance, per unit */
	complex I; /* current, per unit */
	double B; /* line charging, per unit */
	OBJECT* from;	/* from node */
	OBJECT* to;		/* to node */
	double turns_ratio; /* off-nominal turns ratio */

	complex Yeff; /* effective impedance */
	complex Yc; /* effective admittance (including turns ratio) */
	double c; /* turn ratio (inverse) */

public:
	static CLASS *oclass;
	static link *defaults;
	static CLASS *pclass;
public:
	link(MODULE *mod);
	int create();
	TIMESTAMP sync(TIMESTAMP t0);
	int init(node *parent);
	void apply_dV(OBJECT *source, complex dV);
};

GLOBAL CLASS *link_class INIT(NULL);
GLOBAL OBJECT *last_link INIT(NULL);

#endif
