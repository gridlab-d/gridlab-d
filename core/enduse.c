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

int enduse_publish(CLASS *oclass, PROPERTYADDR struct_address, char *prefix)
{
	enduse *this=NULL; // temporary enduse structure used for mapping variables

	struct s_map_enduse{
		PROPERTYTYPE type;
		char *name;
		char *addr;
		char *description;
		int flags;
	}*p, prop_list[]={
		{PT_complex, "demand[kVA]",  PADDR(demand), "the peak power consumption since the last meter reading"},
		{PT_complex, "energy[kVAh]",  PADDR(energy), "the total energy consumed since the last meter reading"},
		{PT_complex, "total_power[kVA]",  PADDR(total), "the total power consumption of the load"},
		{PT_double, "heatgain[Btu/h]",   PADDR(heatgain), "the heat transferred from the enduse to the parent"},
		{PT_double, "heatgain_fraction[%]",  PADDR(heatgain_fraction), "the fraction of the heat that goes to the parent"},
		{PT_double, "current_fraction[%]", PADDR(current_fraction),"the fraction of total power that is constant current"}, 
		{PT_double, "impedance_fraction[%]",  PADDR(impedance_fraction), "the fraction of total power that is constant impedance"},
		{PT_double, "power_fraction[%]",  PADDR(power_fraction), "the fraction of the total power that is constant power"},		
		{PT_double, "power_factor",  PADDR(power_factor), "the power factor of the load"},
		{PT_complex, "constant_power[kVA]",  PADDR(power), "the constant power portion of the total load"},
		{PT_complex, "constant_current[kVA]", PADDR(current), "the constant current portion of the total load"},
		{PT_complex, "constant_admittance[kVA]", PADDR(current), "the constant admittance portion of the total load"},
		{PT_double, "voltage_factor[pu]",  PADDR(voltage_factor), "the voltage change factor"},
		{PT_set, "configuration",  PADDR(config), "the load configuration options"},
			{PT_KEYWORD, "IS220", EUC_IS220},

		// @todo retire these values asap
		{PT_complex, "total[kVA]",  PADDR(power), "the constant power portion of the total load",PF_DEPRECATED},
		{PT_complex, "power[kVA]",  PADDR(power), "the constant power portion of the total load",PF_DEPRECATED},
		{PT_complex, "current[kVA]", PADDR(current), "the constant current portion of the total load",PF_DEPRECATED},
		{PT_complex, "admittance[kVA]", PADDR(current), "the constant admittance portion of the total load",PF_DEPRECATED},
	}, *last=NULL;
	
	int result = 0;	
	for (p=prop_list;p<prop_list+sizeof(prop_list)/sizeof(prop_list[0]);p++)
	{
		char name[256];
				
		if(prefix == NULL || strcmp(prefix,"")==0)
		{
			strcpy(name,p->name);
		}
		else
		{
			strcpy(name,prefix);
			strcat(name, ".");
			strcat(name, p->name);
		}
		
		if (p->type<_PT_LAST)
		{
			PROPERTY *prop = property_malloc(p->type,oclass,name,p->addr+(int64)struct_address,NULL);
			prop->description = p->description;
			prop->flags = p->flags;
			class_add_property(oclass,prop);
			result++;
		}
		else if (last==NULL)
		{
			output_error("PT_KEYWORD not allowed unless it follows another property specification");
			/* TROUBLESHOOT
				The enduse_publish structure is not defined correctly.  This is an internal error and cannot be corrected by
				users.  Contact technical support and report this problem.
			 */
			return -result;
		}
		else if (p->type==PT_KEYWORD) {
			switch (last->type) {
			case PT_enumeration:
				if (!class_define_enumeration_member(oclass,last->name,p->name,p->type))
				{
					output_error("unable to publish enumeration member %s of enduse %s", p->name,last->name);
					/* TROUBLESHOOT
					The enduse_publish structure is not defined correctly.  This is an internal error and cannot be corrected by
					users.  Contact technical support and report this problem.
					 */
					return -result;
				}
				break;
			case PT_set:
				if (!class_define_set_member(oclass,last->name,p->name,(int64)p->addr))
				{
					output_error("unable to publish set member %s of enduse %s", p->name,last->name);
					/* TROUBLESHOOT
					The enduse_publish structure is not defined correctly.  This is an internal error and cannot be corrected by
					users.  Contact technical support and report this problem.
					 */
					return -result;
				}
				break;
			default:
				output_error("PT_KEYWORD not supported after property type %s in enduse_publish", class_get_property_typename(last->type));
				/* TROUBLESHOOT
				The enduse_publish structure is not defined correctly.  This is an internal error and cannot be corrected by
				users.  Contact technical support and report this problem.
				 */
				return -result;
			}
		}
		else
		{
			output_error("property type %s not recognized in enduse_publish", class_get_property_typename(last->type));
			/* TROUBLESHOOT
				The enduse_publish structure is not defined correctly.  This is an internal error and cannot be corrected by
				users.  Contact technical support and report this problem.
			*/
			return -result;
		}

		last = p;
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

