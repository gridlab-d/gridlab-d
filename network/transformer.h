/** $Id: transformer.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 **/

#ifndef _TRANSFORMER_H
#define _TRANSFORMER_H

#include "link.h"

class transformer : public link {
public:
	enum {TT_YY=0, TT_YD=1, TT_DY=2, TT_DD=3} Type;
	double Sbase;
	double Vbase;
	double Zpu;
	double Vprimary;
	double Vsecondary;
private:
	complex Z[3];
	complex a[3];
	complex b[3];
	complex d[3];
	double TurnsRatio;
	double TransformerRatio;
public:
	static CLASS *oclass;
	static transformer *defaults;
	static CLASS *pclass;
public:
	transformer(MODULE *mod);
	int create();
	TIMESTAMP sync(TIMESTAMP t0);
};

GLOBAL CLASS *transformer_class INIT(NULL);

#endif
