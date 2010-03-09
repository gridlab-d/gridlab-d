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

#ifdef _DEBUG
static unsigned int enduse_magic = 0x8c3d7762;
#endif

int enduse_create(enduse *data)
{
	memset(data,0,sizeof(enduse));
	data->next = enduse_list;
	enduse_list = data;

	// check the power factor
	data->power_factor = 1.0;
	data->heatgain_fraction = 1.0;

#ifdef _DEBUG
	data->magic = enduse_magic;
#endif
	return 1;
}

int enduse_init(enduse *e)
{
#ifdef _DEBUG
	if (e->magic!=enduse_magic)
		throw_exception("enduse '%s' magic number bad", e->name);
#endif

	e->t_last = TS_ZERO;

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

TIMESTAMP enduse_sync(enduse *e, PASSCONFIG pass, TIMESTAMP t1)
{
#ifdef _DEBUG
	if (e->magic!=enduse_magic)
		throw_exception("enduse '%s' magic number bad", e->name);
#endif

	if (pass==PC_PRETOPDOWN)// && t1>e->t_last)
	{
		if (e->t_last>TS_ZERO)
		{
			double dt = (double)(t1-e->t_last)/(double)3600;
			e->energy.r += e->total.r * dt;
			e->energy.i += e->total.i * dt;
			if(dt > 0.0)
				e->heatgain = 0; /* heat is a dt thing, so dt=0 -> Q*dt = 0 */
		}
		e->t_last = t1;
	}
	else if(pass==PC_BOTTOMUP)
	{
		if (e->shape && e->shape->type != MT_UNKNOWN) // shape driven -> use fractions
		{
			// non-electric load
			if (e->config&EUC_HEATLOAD)
			{
				e->heatgain = e->shape->load;
			}

			// electric load
			else
			{
				double P = e->voltage_factor>0 ? e->shape->load * (e->power_fraction + e->current_fraction + e->impedance_fraction) : 0.0;
				e->total.r = P;
				if (fabs(e->power_factor)<1)
					e->total.i = (e->power_factor<0?-1:1)*P*sqrt(1/(e->power_factor*e->power_factor)-1);
				else
					e->total.i = 0;

				// beware: these are misnomers (they are e->constant_power, e->constant_current, ...)
				e->power.r = e->total.r * e->power_fraction; e->power.i = e->total.i * e->power_fraction;
				e->current.r = e->total.r * e->current_fraction; e->current.i = e->total.i * e->current_fraction;
				e->admittance.r = e->total.r * e->impedance_fraction; e->admittance.i = e->total.i * e->impedance_fraction;
			}
		}
		else if (e->voltage_factor > 0 && !(e->config&EUC_HEATLOAD)) // no shape electric - use ZIP component directly
		{
			e->total.r = e->power.r + e->current.r + e->admittance.r;
			e->total.i = e->power.i + e->current.i + e->admittance.i;
		}
		else
		{
			/* don't touch anything */
		}

		// non-electric load
		if (e->config&EUC_HEATLOAD)
		{
			e->heatgain *= e->heatgain_fraction;
		}

		// electric load
		else
		{
			if (e->total.r > e->demand.r) e->demand = e->total;
			if(e->heatgain_fraction > 0.0)
				e->heatgain = e->total.r * e->heatgain_fraction * 3412.1416 /* Btu/h/kW */;
		}

		e->t_last = t1;
	}
	return (e->shape && e->shape->type != MT_UNKNOWN) ? e->shape->t2 : TS_NEVER;
}

TIMESTAMP enduse_syncall(TIMESTAMP t1)
{
	enduse *e;
	TIMESTAMP t2 = TS_NEVER;
	for (e=enduse_list; e!=NULL; e=e->next)
	{
		TIMESTAMP t3 = enduse_sync(e,PC_PRETOPDOWN,t1);
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
#define OUTPUT_NZ(X) if (e->X!=0) len+=sprintf(string+len,"%s" #X ": %f", len>0?"; ":"", e->X)
#define OUTPUT(X) len+=sprintf(string+len,"%s"#X": %f", len>0?"; ":"", e->X);
	OUTPUT_NZ(impedance_fraction);
	OUTPUT_NZ(current_fraction);
	OUTPUT_NZ(power_fraction);
	OUTPUT(power_factor);
	OUTPUT(power.r);
	OUTPUT_NZ(power.i);
	return len;
}

int enduse_publish(CLASS *oclass, PROPERTYADDR struct_address, char *prefix)
{
	enduse *this=NULL; // temporary enduse structure used for mapping variables
	int result = 0;
	struct s_map_enduse{
		PROPERTYTYPE type;
		char *name;
		char *addr;
		char *description;
		int flags;
	}*p, prop_list[]={
		{PT_complex, "energy[kVAh]", (char *)PADDR(energy), "the total energy consumed since the last meter reading"},
		{PT_complex, "power[kVA]", (char *)PADDR(total), "the total power consumption of the load"},
		{PT_complex, "peak_demand[kVA]", (char *)PADDR(demand), "the peak power consumption since the last meter reading"},
		{PT_double, "heatgain[Btu/h]", (char *)PADDR(heatgain), "the heat transferred from the enduse to the parent"},
		{PT_double, "heatgain_fraction[pu]", (char *)PADDR(heatgain_fraction), "the fraction of the heat that goes to the parent"},
		{PT_double, "current_fraction[pu]", (char *)PADDR(current_fraction),"the fraction of total power that is constant current"},
		{PT_double, "impedance_fraction[pu]", (char *)PADDR(impedance_fraction), "the fraction of total power that is constant impedance"},
		{PT_double, "power_fraction[pu]", (char *)PADDR(power_fraction), "the fraction of the total power that is constant power"},
		{PT_double, "power_factor", (char *)PADDR(power_factor), "the power factor of the load"},
		{PT_complex, "constant_power[kVA]", (char *)PADDR(power), "the constant power portion of the total load"},
		{PT_complex, "constant_current[kVA]", (char *)PADDR(current), "the constant current portion of the total load"},
		{PT_complex, "constant_admittance[kVA]", (char *)PADDR(admittance), "the constant admittance portion of the total load"},
		{PT_double, "voltage_factor[pu]", (char *)PADDR(voltage_factor), "the voltage change factor"},
		{PT_double, "breaker_amps[A]", (char *)PADDR(breaker_amps), "the rated breaker amperage"},
		{PT_set, "configuration", (char *)PADDR(config), "the load configuration options"},
			{PT_KEYWORD, "IS220", (set)EUC_IS220},
			//{PT_KEYWORD, "NONE",(set)0},
	}, *last=NULL;

	// publish the enduse load itself
	PROPERTY *prop = property_malloc(PT_enduse,oclass,strcmp(prefix,"")==0?"load":prefix,struct_address,NULL);
	prop->description = "the enduse load description";
	prop->flags = 0;
	class_add_property(oclass,prop);

	for (p=prop_list;p<prop_list+sizeof(prop_list)/sizeof(prop_list[0]);p++)
	{
		char name[256], lastname[256];

		if(prefix == NULL || strcmp(prefix,"")==0)
		{
			strcpy(name,p->name);
		}
		else
		{
			//strcpy(name,prefix);
			//strcat(name, ".");
			//strcat(name, p->name);
			sprintf(name,"%s.%s",prefix,p->name);
		}

		if (p->type<_PT_LAST)
		{
			prop = property_malloc(p->type,oclass,name,p->addr+(int64)struct_address,NULL);
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
				if (!class_define_enumeration_member(oclass,lastname,p->name,p->type))
				{
					output_error("unable to publish enumeration member '%s' of enduse '%s'", p->name,last->name);
					/* TROUBLESHOOT
					The enduse_publish structure is not defined correctly.  This is an internal error and cannot be corrected by
					users.  Contact technical support and report this problem.
					 */
					return -result;
				}
				break;
			case PT_set:
				if (!class_define_set_member(oclass,lastname,p->name,(int64)p->addr))
				{
					output_error("unable to publish set member '%s' of enduse '%s'", p->name,last->name);
					/* TROUBLESHOOT
					The enduse_publish structure is not defined correctly.  This is an internal error and cannot be corrected by
					users.  Contact technical support and report this problem.
					 */
					return -result;
				}
				break;
			default:
				output_error("PT_KEYWORD not supported after property '%s %s' in enduse_publish", class_get_property_typename(last->type), last->name);
				/* TROUBLESHOOT
				The enduse_publish structure is not defined correctly.  This is an internal error and cannot be corrected by
				users.  Contact technical support and report this problem.
				 */
				return -result;
			}
		}
		else
		{
			output_error("property type '%s' not recognized in enduse_publish", class_get_property_typename(last->type));
			/* TROUBLESHOOT
				The enduse_publish structure is not defined correctly.  This is an internal error and cannot be corrected by
				users.  Contact technical support and report this problem.
			*/
			return -result;
		}

		last = p;
		strcpy(lastname,name);
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
		output_error("convert_to_enduse(string='%-.64s...', ...) input string is too long (max is 1023)",string);
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

