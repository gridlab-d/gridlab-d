/** $Id: capbank.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 **/

#ifndef _CAPBANK_H
#define _CAPBANK_H

#include "link.h"

class capbank : public link {
public:
	double KVARrated;
	double Vrated;
	enum {CAPS_IN=0, CAPS_OUT=1} state;
	OBJECT *CTlink;
	OBJECT *PTnode;
	double VARopen;
	double VARclose;
	double Vopen;
	double Vclose;
public:
	static CLASS *oclass;
	static capbank *defaults;
	static CLASS *pclass;
public:
	capbank(MODULE *mod);
	int create();
	TIMESTAMP sync(TIMESTAMP t0);
};

GLOBAL CLASS *capbank_class INIT(NULL);

#endif
