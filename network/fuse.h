/** $Id: fuse.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 **/

#ifndef _FUSE_H
#define _FUSE_H

#include "link.h"

class fuse : public link {
public:
	double TimeConstant;
	double SetCurrent;
	double SetBase;
	double SetScale;
	double SetCurve;
	double TresetAvg;
	double TresetStd;
	enum {FS_GOOD=1, FS_BLOWN=2, FS_FAULT=3} State;
private:
	TIMESTAMP Tstate;
	TIMESTAMP Treset;
public:
	static CLASS *oclass;
	static fuse *defaults;
	static CLASS *pclass;
public:
	fuse(MODULE *mod);
	int create();
	TIMESTAMP sync(TIMESTAMP t0);
};

GLOBAL CLASS *fuse_class INIT(NULL);

#endif
