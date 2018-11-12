// powernet/house.cpp
// Copyright (C) 2016, Stanford University
// Author: dchassin@slac.stanford.edu

#include "powernet.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

EXPORT_CREATE(house);
EXPORT_INIT(house);
EXPORT_SYNC(house);
EXPORT_COMMIT(house);
EXPORT_METHOD(house,connect);

CLASS *house::oclass = NULL;
house *house::defaults = NULL;



house::house(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"house",sizeof(house),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK|PC_OBSERVER);
		if (oclass==NULL)
			throw "unable to register class house";
		else
			oclass->trl = TRL_CONCEPT;
		defaults = this;
		if ( gl_publish_variable(oclass,
				PT_object, "weather", get_weather_offset(), PT_DESCRIPTION, "weather object reference",
				PT_double, "Uair[W/degC]", get_Uair_offset(), PT_DESCRIPTION, "air-outdoor conductance",
				PT_double, "Cair[kJ/degC]", get_Cair_offset(), PT_DESCRIPTION, "air heat capacity",
				PT_double, "Umass[W/degC]", get_Umass_offset(), PT_DESCRIPTION, "air-mass conductance",
				PT_double, "Cmass[kJ/degC]", get_Cmass_offset(), PT_DESCRIPTION, "mass heat capacity",
				PT_double, "Qheat[W]", get_Qheat_offset(), PT_DESCRIPTION, "heating system capacity",
				PT_double, "Qaux[W]", get_Qaux_offset(), PT_DESCRIPTION, "auxiliary heating system capacity",
				PT_double, "Qcool[W]", get_Qcool_offset(), PT_DESCRIPTION, "cooling system capacity",
				PT_double, "Qint[W]", get_Qint_offset(), PT_DESCRIPTION, "internal heat gain",
				PT_double, "Tair[degC]", get_Tair_offset(), PT_DESCRIPTION, "indoor air temperature",
				PT_double, "Tmass[degC]", get_Tmass_offset(), PT_DESCRIPTION, "indoor mass temperature",
				PT_double, "COPheat[pu]", get_COPheat_offset(), PT_DESCRIPTION, "heating COP",
				PT_double, "COPcool[pu]", get_COPcool_offset(), PT_DESCRIPTION, "cooling COP",
				PT_method, "connect", get_connect_offset(), PT_DESCRIPTION, "method to connect agent",
				NULL) < 1 )
			throw "unable to publish house properties";
		memset(this,0,sizeof(house));
		// TODO default defaults
	}
}

int house::create(void)
{
	loads = new list<ENDUSELOAD*>;
	memcpy(this,defaults,sizeof(*this));
	return 1; /* return 1 on success, 0 on failure */
}

int house::init(OBJECT *parent)
{
	// TODO dynamic defaults
	return 1;
}

int house::connect(char *buffer, size_t len)
{
	printf("house::connect(char *buffer='%s', size_t len=%d)\n", buffer, len);
	if ( len==0 )
	{
		message msg(buffer);
		const char *action = msg.get("action");
		const char *source = msg.get("object");
		if ( action==NULL || strcmp(action,"agent")!=0 )
		{
			error("house::connect(char *buffer='%s', size_t len=%d): action is not valid (action='%s')", buffer, len, action);
			return 0;
		}
		if ( source==NULL )
		{
			error("house::connect(char *buffer='%s', size_t len=%d): source is not found", buffer, len);
			return 0;
		}
		gld_object *agent = find_object((char*)source);
		if ( agent==NULL || !agent->is_valid() )
		{
			error("house::connect(char *buffer='%s', size_t len=%d): agent is not valid", buffer, len);
			return 0;
		}
		sell = gld_property(agent,"sell");
		if ( !sell.is_valid() )
		{
			error("house::connect(char *buffer='%s', size_t len=%d): agent does not implement 'sell' method", buffer, len);
			return 0;
		}
		return 1;
	}
	else
		return 0;
}
TIMESTAMP house::presync(TIMESTAMP t1)
{
	if ( sell.is_valid() )
	{
		gld_clock now;
		message msg;
		msg.add("id",get_name());
		msg.add("quantity","%g kW",10);
		msg.add("price","%g $/MWh", 15);
		msg.add("start","%ld s",now.get_timestamp());
		msg.add("duration","%d s",300);
		char buffer[1024];
		msg.write(buffer,sizeof(1024));
		sell.call(buffer);
	}
	return TS_NEVER;
}

TIMESTAMP house::sync(TIMESTAMP t1)
{
	return TS_NEVER;
}

TIMESTAMP house::postsync(TIMESTAMP t1)
{
	return TS_NEVER;
}

TIMESTAMP house::commit(TIMESTAMP t1, TIMESTAMP t2)
{
	// TODO
	return TS_NEVER;
}

ENDUSELOAD *house::add_load(OBJECT *obj, bool is220)
{
	ENDUSELOAD *load = new ENDUSELOAD;
	memset(load,0,sizeof(ENDUSELOAD));
	if ( is220 )
		load->circuit = s_enduseload::PC_BOTH;
	else
		load->circuit = ( gl_random_bernoulli(RNGSTATE,0.5) ? s_enduseload::PC_ONE : s_enduseload::PC_TWO );
	return load;
}
