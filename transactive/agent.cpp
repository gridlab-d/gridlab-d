// transactive/agent.cpp
// Copyright (C) 2016, Stanford University
// Author: dchassin@slac.stanford.edu

#include "transactive.h"

// transactive/agent.cpp
// Copyright (C) 2016, Stanford University
// Author: dchassin@slac.stanford.edu

#include "transactive.h"

/** $Id: assert.cpp 4738 2014-07-03 00:55:39Z dchassin $

   General purpose assert objects

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include "assert.h"

EXPORT_CREATE(agent);
EXPORT_INIT(agent);
EXPORT_COMMIT(agent);
EXPORT_METHOD(agent,sell);
EXPORT_METHOD(agent,buy);

CLASS *agent::oclass = NULL;
agent *agent::defaults = NULL;

agent::agent(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"agent",sizeof(agent),PC_AUTOLOCK|PC_OBSERVER);
		if (oclass==NULL)
			throw "unable to register class agent";
		else
			oclass->trl = TRL_CONCEPT;
		defaults = this;
		if ( gl_publish_variable(oclass,
				PT_enumeration, "type", get_type_offset(), PT_DESCRIPTION, "type of transactive agent",
					PT_KEYWORD, "DEVICE", (enumeration)AT_DEVICE,
					PT_KEYWORD, "BROKER", (enumeration)AT_BROKER,
					PT_KEYWORD, "MARKET", (enumeration)AT_MARKET,
				PT_method, "sell", get_sell_offset(), PT_DESCRIPTION, "sell bid (quantity, price, start, end)",
				PT_method, "buy", get_buy_offset(), PT_DESCRIPTION, "buy bid (quantity, price, start, end)",
				NULL) < 1 )
			throw "unable to publish agent properties";
		memset(this,0,sizeof(agent));
		// TODO default defaults
	}
}

int agent::create(void)
{
	memcpy(this,defaults,sizeof(*this));
	// TODO static defaults
	return 1; /* return 1 on success, 0 on failure */
}

int agent::init(OBJECT *parent)
{
	gld_property connect(parent,"connect");
	if ( !connect.is_valid() )
		warning("parent does not implement 'connect' method");
	connect.call(get_name());

	return 1;
}

TIMESTAMP agent::commit(TIMESTAMP t1, TIMESTAMP t2)
{
	// TODO
	return TS_NEVER;
}

int agent::sell(char *buffer, size_t len)
{
	printf("agent::sell(char *buffer='%s', size_t len=%d)\n", buffer, len);
	if ( len == 0 )
	{
		return 1;
	}
	else
		return 0; // no outgoing message
}

int agent::buy(char *buffer, size_t len)
{
	printf("agent::buy(char *buffer='%s', size_t len=%d)\n", buffer, len);
	if ( len == 0 )
	{
		return 1;
	}
	else
		return 0; // no outgoing message
}

