/** $Id: enduse.c 4738 2014-07-03 00:55:39Z dchassin $
 	Copyright (C) 2008 Battelle Memorial Institute
	@file loadshape.c
	@addtogroup loadshape
**/

#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <pthread.h>

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
static unsigned int n_enduses = 0;

double enduse_get_part(void *x, char *name)
{
	enduse *e = (enduse*)x;
#define _DO_DOUBLE(X,Y) if ( strcmp(name,Y)==0) return e->X;
#define _DO_COMPLEX(X,Y) \
	if ( strcmp(name,Y".real")==0) return e->X.r; \
	if ( strcmp(name,Y".imag")==0) return e->X.i; \
	if ( strcmp(name,Y".mag")==0) return complex_get_mag(e->X); \
	if ( strcmp(name,Y".arg")==0) return complex_get_arg(e->X); \
	if ( strcmp(name,Y".ang")==0) return complex_get_arg(e->X)*180/PI; 
#define DO_DOUBLE(X) _DO_DOUBLE(X,#X)
#define DO_COMPLEX(X) _DO_COMPLEX(X,#X)
	DO_COMPLEX(total);
	DO_COMPLEX(energy);
	DO_COMPLEX(demand);
	DO_DOUBLE(breaker_amps);
	DO_COMPLEX(admittance);
	DO_COMPLEX(current);
	DO_COMPLEX(power);
	DO_DOUBLE(impedance_fraction);
	DO_DOUBLE(current_fraction);
	DO_DOUBLE(power_fraction);
	DO_DOUBLE(power_factor);
	DO_DOUBLE(voltage_factor);
	DO_DOUBLE(heatgain);
	DO_DOUBLE(heatgain_fraction);
#define DO_MOTOR(X) \
	_DO_COMPLEX(motor[EUMT_MOTOR_##X].power,"motor"#X".power"); \
	_DO_COMPLEX(motor[EUMT_MOTOR_##X].impedance,"motor"#X".impedance"); \
	_DO_DOUBLE(motor[EUMT_MOTOR_##X].inertia,"motor"#X".inertia"); \
	_DO_DOUBLE(motor[EUMT_MOTOR_##X].v_stall,"motor"#X".v_stall"); \
	_DO_DOUBLE(motor[EUMT_MOTOR_##X].v_start,"motor"#X".v_start"); \
	_DO_DOUBLE(motor[EUMT_MOTOR_##X].v_trip,"motor"#X".v_trip"); \
	_DO_DOUBLE(motor[EUMT_MOTOR_##X].t_trip,"motor"#X".t_trip");
	DO_MOTOR(A);
	DO_MOTOR(B);
	DO_MOTOR(C);
	DO_MOTOR(D);
#define DO_ELECTRONIC(X) \
	_DO_COMPLEX(electronic[EUMT_MOTOR_##X].power,"electronic"#X".power"); \
	_DO_DOUBLE(electronic[EUMT_MOTOR_##X].inertia,"electronic"#X".inertia"); \
	_DO_DOUBLE(electronic[EUMT_MOTOR_##X].v_trip,"electronic"#X".v_trip"); \
	_DO_DOUBLE(electronic[EUMT_MOTOR_##X].v_start,"electronic"#X".v_start");
	DO_ELECTRONIC(A);
	DO_ELECTRONIC(B);
	return QNAN;
}

#ifdef _DEBUG
static unsigned int enduse_magic = 0x8c3d7762;
#endif

int enduse_create(enduse *data)
{
	memset(data,0,sizeof(enduse));
	data->next = enduse_list;
	enduse_list = data;
	n_enduses++;

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
			e->cumulative_heatgain += e->heatgain * dt;
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

typedef struct s_endusesyncdata {
	unsigned int n;
	pthread_t pt;
	bool ok;
	enduse *e;
	unsigned int ne;
	TIMESTAMP t0;
	unsigned int ran;
} ENDUSESYNCDATA;

static pthread_cond_t start_ed = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t startlock_ed = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t done_ed = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t donelock_ed = PTHREAD_MUTEX_INITIALIZER;
static TIMESTAMP next_t1_ed, next_t2_ed;
static unsigned int donecount_ed;
static unsigned int run = 0;

clock_t enduse_synctime = 0;

void *enduse_syncproc(void *ptr)
{
	ENDUSESYNCDATA *data = (ENDUSESYNCDATA*)ptr;
	enduse *e;
	unsigned int n;
	TIMESTAMP t2;

	// begin processing loop
	while (data->ok) 
	{
		// lock access to start condition
		pthread_mutex_lock(&startlock_ed);

		// wait for thread start condition
		while (data->t0==next_t1_ed && data->ran==run) 
			pthread_cond_wait(&start_ed,&startlock_ed);
		
		// unlock access to start count
		pthread_mutex_unlock(&startlock_ed);

		// process the list for this thread
		t2 = TS_NEVER;
		for ( e=data->e, n=0 ; e!=NULL, n<data->ne ; e=e->next, n++ )
		{
			TIMESTAMP t = enduse_sync(e, PC_PRETOPDOWN, next_t1_ed);
			if (t<t2) t2 = t;
		}

		// signal completed condition
		data->t0 = next_t1_ed;
		data->ran++;

		// lock access to done condition
		pthread_mutex_lock(&donelock_ed);

		// signal thread is done for now
		donecount_ed--;
		if ( t2<next_t2_ed ) next_t2_ed = t2;

		// signal change in done condition
		pthread_cond_broadcast(&done_ed);

		// unlock access to done count
		pthread_mutex_unlock(&donelock_ed);
	}
	pthread_exit((void*)0);
	return (void*)0;
}

TIMESTAMP enduse_syncall(TIMESTAMP t1)
{
	static unsigned int n_threads_ed=0;
	static ENDUSESYNCDATA *thread_ed = NULL;
	TIMESTAMP t2 = TS_NEVER;
	clock_t ts = exec_clock();
	
	// skip enduse_syncall if there's no enduse in the glm
	if (n_enduses == 0)
		return TS_NEVER;

	// number of threads desired
	if (n_threads_ed==0)
	{
		enduse *e;
		int n_items, en = 0;

		output_debug("enduse_syncall setting up for %d enduses", n_enduses);

		// determine needed threads
		n_threads_ed = global_threadcount;
		if (n_threads_ed>1)
		{
			unsigned int n;
			if (n_enduses<n_threads_ed*4)
				n_threads_ed = n_enduses/4;

			// only need 1 thread if n_enduses is less than 4
			if (n_threads_ed == 0)
				n_threads_ed = 1;

			// determine enduses per thread
			n_items = n_enduses/n_threads_ed;
			n_threads_ed = n_enduses/n_items;
			if (n_threads_ed*n_items<n_enduses) // not enough slots yet
				n_threads_ed++; // add one underused thread

			output_debug("enduse_syncall is using %d of %d available threads", n_threads_ed,global_threadcount);
			output_debug("enduse_syncall is assigning %d enduses per thread", n_items);

			// allocate thread list
			thread_ed = (ENDUSESYNCDATA*)malloc(sizeof(ENDUSESYNCDATA)*n_threads_ed);
			memset(thread_ed,0,sizeof(ENDUSESYNCDATA)*n_threads_ed);

			// assign starting enduse for each thread
			for (e=enduse_list; e!=NULL; e=e->next)
			{
				if (thread_ed[en].ne==n_items)
					en++;
				if (thread_ed[en].ne==0)
					thread_ed[en].e = e;
				thread_ed[en].ne++;
			}

			// create threads
			for (n=0; n<n_threads_ed; n++)
			{
				thread_ed[n].ok = true;
				if (pthread_create(&(thread_ed[n].pt),NULL,enduse_syncproc,&(thread_ed[n]))!=0)
				{
					output_fatal("enduse_sync thread creation failed");
					thread_ed[n].ok = false;
				}
				else 
					thread_ed[n].n = n;
			}
		}
	}

	// no threading required
	if (n_threads_ed<2)
	{
		// process list directly
		enduse *e;
		for (e=enduse_list; e!=NULL; e=e->next)
		{
			TIMESTAMP t3 = enduse_sync(e, PC_PRETOPDOWN, t1);
			if (t3<t2) t2 = t3;
		}
		next_t2_ed = t2;
	}
	else 
	{
		// lock access to done count
		pthread_mutex_lock(&donelock_ed);

		// initialize wait count
		donecount_ed = n_threads_ed;

		// lock access to start condition
		pthread_mutex_lock(&startlock_ed);

		// update start condition
		next_t1_ed = t1;
		next_t2_ed = TS_NEVER;
		run++;

		// signal all the threads
		pthread_cond_broadcast(&start_ed);

		// unlock access to start count
		pthread_mutex_unlock(&startlock_ed);

		// begin wait 
		while (donecount_ed>0)
			pthread_cond_wait(&done_ed,&donelock_ed);
		output_debug("passed donecount==0 condition");

		// unclock done count
		pthread_mutex_unlock(&donelock_ed);

		// process results from all threads
		if (next_t2_ed<t2) t2=next_t2_ed;
	}

	enduse_synctime += exec_clock() - ts;
	return t2;

	/*enduse *e;
	TIMESTAMP t2 = TS_NEVER;
	clock_t start = exec_clock();
	for (e=enduse_list; e!=NULL; e=e->next)
	{
		TIMESTAMP t3 = enduse_sync(e,PC_PRETOPDOWN,t1);
		if (t3<t2) t2 = t3;
	}
	enduse_synctime += exec_clock() - start;
	return t2;*/
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
		{PT_double, "cumulative_heatgain[Btu]", (char *)PADDR(cumulative_heatgain), "the cumulative heatgain from the enduse to the parent"},
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

	/* use structure conversion if opens with { */
	if ( string[0]=='{')
	{
		PROPERTY eus[] = {
			{NULL,"total",PT_complex,0,0,PA_PUBLIC,"kVA",(char*)(&e->total)-(char*)e,NULL,NULL,NULL,eus+1},
			{NULL,"energy",PT_complex,0,0,PA_PUBLIC,"kVA",(char*)(&e->total)-(char*)e,NULL,NULL,NULL,eus+2},
			{NULL,"demand",PT_complex,0,0,PA_PUBLIC,"kVA",(char*)(&e->total)-(char*)e,NULL,NULL,NULL,NULL},
		};
		return convert_to_struct(string,data,&eus);
	}

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

