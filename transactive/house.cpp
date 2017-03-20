// transactive/house.cpp
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
				PT_double, "UA[W/degC]", get_UA_offset(), PT_DESCRIPTION, "air-outdoor conductance",
				PT_double, "CA[kJ/degC]", get_CA_offset(), PT_DESCRIPTION, "air heat capacity",
				PT_double, "UM[W/degC]", get_UM_offset(), PT_DESCRIPTION, "air-mass conductance",
				PT_double, "CM[kJ/degC]", get_CM_offset(), PT_DESCRIPTION, "mass heat capacity",
				PT_double, "QH[W]", get_QH_offset(), PT_DESCRIPTION, "heating system capacity",
				PT_double, "QC[W]", get_QC_offset(), PT_DESCRIPTION, "cooling system capacity",
				PT_double, "QI[W]", get_QI_offset(), PT_DESCRIPTION, "internal heat gain",
				PT_double, "TA[degC]", get_TA_offset(), PT_DESCRIPTION, "indoor air temperature",
				PT_double, "TM[degC]", get_TM_offset(), PT_DESCRIPTION, "mass temperature",
				PT_double, "COP[pu]", get_COP_offset(), PT_DESCRIPTION, "heating/cooling ",
				PT_method, "connect", get_connect_offset(), PT_DESCRIPTION, "method to connect agent",
				NULL) < 1 )
			throw "unable to publish house properties";
		memset(this,0,sizeof(house));
		// TODO default defaults
	}
}

int house::create(void)
{
	memcpy(this,defaults,sizeof(*this));
	A.zeros();
	size_t n;
	for ( n = 0 ; n < N_SYSTEMMODES ; n++ )
		B[n].zeros();
	C.zeros();
	u = 0;
	y = 0;
	x(0) = TA;
	x(1) = TM;
	QH.grow_to(1,2);
	QC.grow_to(1,1);
	COP.grow_to(1,3);
	return 1; /* return 1 on success, 0 on failure */
}

int house::init(OBJECT *parent)
{
	A(0,0) = -(UA+UM)/CA;	A(1,0) = +UM/CA;
	A(0,1) = +UM/CM;		A(1,1) = -UM/CM;

	B[SM_OFF](0) = 0;
	B[SM_OFF](1) = 0;

	B[SM_HEAT](0) = QH[0][0]/CA;
	B[SM_HEAT](1) = 0;

	B[SM_AUX](0) = QH[0][1]/CA;
	B[SM_AUX](1) = 0;

	B[SM_COOL](0) = -QC[0][0]/CA;
	B[SM_COOL](1) = 0;

	C(0) = 1;
	C(1) = 0;

	x(0) = TA;
	x(1) = TM;

	// TODO dynamic defaults
	return 1;
}

int house::connect(char *buffer, size_t len)
{
	printf("house::connect(char *buffer='%s', size_t len=%d)\n", buffer, len);
	if ( len==0 )
	{
		gld_object *agent = find_object(buffer);
		if ( agent==NULL || !agent->is_valid() )
		{
			error("house::connect(char *buffer='%s', size_t len=%d): agent is not valid");
			return 0;
		}
		sell = gld_property(agent,"sell");
		if ( !sell.is_valid() )
		{
			error("house::connect(char *buffer='%s', size_t len=%d): agent does not implement 'sell' method");
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
		sell.call("house_1: 10kW @ 15$/MWh at 1478291400s for 300s");
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
