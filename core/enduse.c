/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file loadshape.c
	@addtogroup loadshape
**/

#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

#include "platform.h"
#include "output.h"
#include "loadshape.h"
#include "exception.h"
#include "convert.h"
#include "globals.h"
#include "random.h"
#include "schedule.h"
#include "enduse.h"
#include "gridlabd.h"

static enduse *enduse_list = NULL;

int enduse_create(void *data)
{
	enduse *e = (enduse*)data;
	e->shape = NULL;
	e->next = enduse_list;
	enduse_list = e;
	return 1;
}

int enduse_init(enduse *e)
{
	double sum = fabs(e->current_fraction) + fabs(e->impedance_fraction) + fabs(e->power_fraction);

	/* TODO initialize enduse data */
	if (sum==0)
	{
		output_error("enduse_init(...) sum of zip fractions is zero");
		return 1;
	}

	return 0;
}

int enduse_initall(void)
{
	enduse *e;
	for (e=enduse_list; e!=NULL; e=e->next)
	{
		if (enduse_init(e)==1)
			return FAILED;
	}
	return SUCCESS;
}

TIMESTAMP enduse_sync(enduse *e, PASSCONFIG pass, TIMESTAMP t0, TIMESTAMP t1)
{
	if (pass==PC_PRETOPDOWN && t1>t0)
	{
		double dt = (double)(t1-t0)/(double)3600;
		e->energy.r += e->power.i * dt;
		e->energy.i += e->power.i * dt;
	}
	else if(pass==PC_BOTTOMUP)
	{
		e->power.r = e->shape->load * (e->power_fraction + (e->current_fraction + e->impedance_fraction/e->voltage_factor )/e->voltage_factor) ;
		e->power.i = e->shape->load * e->power_factor;
		if (e->power.r > e->demand.r) e->demand = e->power;
		e->heatgain = e->power.r * e->heatgain_fraction;
	}
	return TS_NEVER;
}

TIMESTAMP enduse_syncall(TIMESTAMP t1)
{
	enduse *e;
	TIMESTAMP t2 = TS_NEVER;
	for (e=enduse_list; e!=NULL; e=e->next)
	{
		TIMESTAMP t3 = enduse_sync(e,PC_PRETOPDOWN,global_clock,t1);
		if (t3<t2) t2 = t3;
	}
	return t2;
}

int convert_from_enduse(char *string,int size,void *data, PROPERTY *prop)
{
/*
	loadshape *shape;
	complex power;
	complex energy;
	complex demand;
	double impedance_fraction;
	double current_fraction;
	double power_fraction;
	double power_factor;
	struct s_enduse *next;
*/
	enduse *e = (enduse*)data;
	int len = 0;
#define OUTPUT_NZ(X) if (e->X!=0) len+=sprintf(string+len,"%s" #X ": %g", len>0?"; ":"", e->X)
#define OUTPUT(X) len+=sprintf(string+len,"%s"#X": %g", len>0?"; ":"", e->X);
	OUTPUT_NZ(impedance_fraction);
	OUTPUT_NZ(current_fraction);
	OUTPUT_NZ(power_fraction);
	OUTPUT(power_factor);
	OUTPUT(shape);
	return len;
}

int enduse_publish(CLASS *oclass, int struct_address, char *prefix)
{
	enduse e;

	struct s_map_enduse{
		char *prop_name;
		int publish_addr;
		PROPERTYTYPE prop_type;
	}*p, prop_list[]={
		{"current_fraction", ((&e.current_fraction - &e) + struct_address), PT_double},
		{"demand[kVA]", ((&e.demand - &e) + struct_address), PT_complex},
		{"energy[kVA]", ((&e.energy - &e) + struct_address), PT_complex},
		{"heatgain",  ((&e.heatgain - &e) + struct_address), PT_double},
		{"heatgain_fraction", ((&e.heatgain_fraction - &e) + struct_address), PT_double},
		{"impedance_fraction", ((&e.impedance_fraction - &e) + struct_address), PT_double},
		{"power[kVA]", ((&e.power - &e) + struct_address), PT_complex},
		{"power_factor", ((&e.power_factor - &e) + struct_address), PT_double},
		{"power_fraction", ((&e.power_fraction - &e) + struct_address), PT_double},		
		{"voltage_factor", ((&e.voltage_factor - &e) + struct_address), PT_double},
		{"flags", ((&e.flags - &e) + struct_address), PT_set},
		{"is220", EUF_IS220, PT_KEYWORD},
	};
	
	int result = 0;	

	for (p=prop_list;p<prop_list+sizeof(prop_list)/sizeof(prop_list[0]);p++)
	{
		char *prop_name;
				
		if(prefix == NULL)
		{
			prop_name = p->prop_name;
		}
		else
		{
			prop_name = strcat(prefix, ".", p->prop_name);
		}
				
		result = class_define_map(oclass,  p->prop_type, prop_name, p->publish_addr, NULL);
		if(result<1)
			output_error("unable to publish properties in %s",__FILE__);
	}

	return result;
}

int convert_to_enduse(char *string, void *data, PROPERTY *prop)
{
	enduse *e = (enduse*)data;
	char buffer[1024];
	char *token = NULL;

	/* check string length before copying to buffer */
	if (strlen(string)>sizeof(buffer)-1)
	{
		output_error("convert_to_loadshape(string='%-.64s...', ...) input string is too long (max is 1023)",string);
		return 0;
	}
	strcpy(buffer,string);

	/* parse tuples separate by semicolon*/
	while ((token=strtok(token==NULL?buffer:NULL,";"))!=NULL)
	{
		/* colon separate tuple parts */
		char *param = token;
		char *value = strchr(token,':');

		/* isolate param and token and eliminte leading whitespaces */
		while (isspace(*param) || iscntrl(*param)) param++;		
		if (value==NULL)
			value="1";
		else
			*value++ = '\0'; /* separate value from param */
		while (isspace(*value) || iscntrl(*value)) value++;

		// parse params
		if (strcmp(param,"current_fraction")==0)
			e->current_fraction = atof(value);
		else if (strcmp(param,"impedance_fraction")==0)
			e->impedance_fraction = atof(value);
		else if (strcmp(param,"power_fraction")==0)
			e->power_fraction = atof(value);
		else if (strcmp(param,"power_factor")==0)
			e->power_factor = atof(value);
		else if (strcmp(param,"loadshape")==0)
		{
			PROPERTY *pref = class_find_property(prop->oclass,value);
			if (pref==NULL)
			{
				output_warning("convert_to_enduse(string='%-.64s...', ...) loadshape '%s' not found in class '%s'",string,value,prop->oclass->name);
				return 0;
			}
			e->shape = (loadshape*)((char*)e - (int64)(prop->addr) + (int64)(pref->addr));
		}
		else
		{
			output_error("convert_to_enduse(string='%-.64s...', ...) parameter '%s' is not valid",string,param);
			return 0;
		}
	}

	/* reinitialize the loadshape */
	if (enduse_init((enduse*)data))
		return 0;

	/* everything converted ok */
	return 1;
}

int enduse_test(void)
{
	int failed = 0;
	int ok = 0;
	int errorcount = 0;
	char ts[64];

	/* tests */
	struct s_test {
		char *name;
	} *p, test[] = {
		"TODO",
	};

	output_test("\nBEGIN: enduse tests");
	for (p=test;p<test+sizeof(test)/sizeof(test[0]);p++)
	{
	}

	/* report results */
	if (failed)
	{
		output_error("endusetest: %d enduse tests failed--see test.txt for more information",failed);
		output_test("!!! %d enduse tests failed, %d errors found",failed,errorcount);
	}
	else
	{
		output_verbose("%d enduse tests completed with no errors--see test.txt for details",ok);
		output_test("endusetest: %d schedule tests completed, %d errors found",ok,errorcount);
	}
	output_test("END: enduse tests");
	return failed;
}

