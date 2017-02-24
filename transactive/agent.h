// transactive/agent.h
// Copyright (C) 2016, Stanford University
// Author: dchassin@slac.stanford.edu

#ifndef _AGENT_H
#define _AGENT_H

#include "gridlabd.h"
#include "../powerflow/triplex_meter.h"

DECL_METHOD(agent,sell);
DECL_METHOD(agent,buy);

class agent : public gld_object {
public:
	enum {AT_DEVICE=0, AT_BROKER=1, AT_MARKET=2	} AGENTTYPE;
	GL_ATOMIC(enumeration,type);
	GL_METHOD(agent,sell);
	GL_METHOD(agent,buy);

private:
	// TODO

public:
	/* required implementations */
	agent(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);
	int postnotify(PROPERTY *prop, char *value);
	inline int prenotify(PROPERTY *prop, char *value) { return 1; };

public:
	static CLASS *oclass;
	static agent *defaults;
};

#endif // _AGENT_H
