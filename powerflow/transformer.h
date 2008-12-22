// $Id: transformer.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TRANSFORMER_H
#define _TRANSFORMER_H

#include "powerflow.h"
#include "link.h"
#include "transformer_configuration.h"

class transformer : public link
{
public:
	OBJECT *configuration;

	int create(void);
	int init(OBJECT *parent);

	static CLASS *oclass;
	static CLASS *pclass;
	int isa(char *classname);

	transformer(MODULE *mod);
	inline transformer(CLASS *cl=oclass):link(cl){};
};

#endif // _TRANSFORMER_H
