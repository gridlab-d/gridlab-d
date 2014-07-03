/** $Id: relay.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 **/

#ifndef _RELAY_H
#define _RELAY_H

#include "link.h"

class relay : public link {
public:
	enum {FC_U1=1,FC_U2=2,FC_U3=3,FC_U4=4,FC_U5=5} Curve;
	double TimeDial;
	double SetCurrent;
	enum {FS_CLOSED=1, FS_TRIPPED=2, FS_RECLOSED=3, FS_LOCKOUT=4, FS_FAULT=5} State;
private:
	double Tp;
	double Tr;
	double Mp;
	double Mr;
	TIMESTAMP Tstate;
public:
	static CLASS *oclass;
	static relay *defaults;
	static CLASS *pclass;
public:
	relay(MODULE *mod);
	int create();
	TIMESTAMP sync(TIMESTAMP t0);
};

GLOBAL CLASS *relay_class INIT(NULL);

#endif
