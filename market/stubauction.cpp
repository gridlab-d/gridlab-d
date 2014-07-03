/** $Id: stubauction.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file stubauction.cpp
	@defgroup stubauction Template for a new object class
	@ingroup market

	The stubauction object implements the basic stubauction. 

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "gridlabd.h"
#include "stubauction.h"

CLASS *stubauction::oclass = NULL;
stubauction *stubauction::defaults = NULL;

static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_POSTTOPDOWN;

/* Class registration is only called once to register the class with the core */
stubauction::stubauction(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"stubauction",sizeof(stubauction),passconfig|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class stubauction";
		else
			oclass->trl = TRL_QUALIFIED;

		if (gl_publish_variable(oclass,
			PT_char32, "unit", PADDR(unit), PT_DESCRIPTION, "unit of quantity",
			PT_double, "period[s]", PADDR(period), PT_DESCRIPTION, "interval of time between market clearings",
			PT_double, "last.P", PADDR(last_price), PT_DESCRIPTION, "last cleared price", 
			PT_double, "current_market.clearing_price", PADDR(next_price),  PT_DESCRIPTION, "next cleared price",
			PT_double, "past_market.clearing_price", PADDR(last_price), PT_DESCRIPTION, "last cleared price", 
			PT_double, "next.P", PADDR(next_price),  PT_DESCRIPTION, "next cleared price",
			PT_double, "avg24", PADDR(avg24), PT_DESCRIPTION, "daily average of price",
			PT_double, "std24", PADDR(std24), PT_DESCRIPTION, "daily stdev of price",
			PT_double, "avg72", PADDR(avg72), PT_DESCRIPTION, "three day price average",
			PT_double, "std72", PADDR(std72), PT_DESCRIPTION, "three day price stdev",
			PT_double, "avg168", PADDR(avg168), PT_DESCRIPTION, "weekly average of price",
			PT_double, "std168", PADDR(std168), PT_DESCRIPTION, "weekly stdev of price",
			PT_int64, "market_id", PADDR(market_id), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "unique identifier of market clearing",
			PT_bool, "verbose", PADDR(verbose), PT_DESCRIPTION, "enable verbose stubauction operations",
			PT_enumeration,"control_mode",PADDR(control_mode),PT_DESCRIPTION,"the control mode to use for determining average and deviation calculations",
				PT_KEYWORD,"NORMAL",(enumeration)CON_NORMAL,
				PT_KEYWORD,"DISABLED",(enumeration)CON_DISABLED,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(this,0,sizeof(stubauction));
	}
}

/* Object creation is called once for each object that is created by the core */
int stubauction::create(void)
{
	memcpy(this,defaults,sizeof(stubauction));
	lasthr = thishr = -1;
	verbose = 0;
	control_mode = CON_NORMAL;
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int stubauction::init(OBJECT *parent)
{
	OBJECT *obj=OBJECTHDR(this);
	market_id = 0;
	if(period == 0) period = 300;
	clearat = nextclear();
	return 1; /* return 1 on success, 0 on failure */
}


int stubauction::isa(char *classname)
{
	return strcmp(classname,"stubauction")==0;
}


/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP stubauction::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	return TS_NEVER;
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP stubauction::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	int64 i = 0;
	int64 j = 0;
	DATETIME dt;
	
	retry = 0;

	if (t1>=clearat)
	{

		gl_localtime(clearat,&dt);
//		if (verbose) gl_output("   ...%s clearing process started at %s", gl_name(OBJECTHDR(this),myname,sizeof(myname)), gl_strtime(&dt,buffer,sizeof(buffer))?buffer:"unknown time");

		/* clear market */
		thishr = dt.hour;
		
		last_price = next_price;
		++market_id;

//		if(lasthr != thishr){
		if(t0 != t1 && 0 == t1 % 3600){
			/* add price/quantity to the history */
			prices[count%168] = next_price;
			++count;
			
			/* update the daily and weekly averages */
			if(control_mode == CON_NORMAL){
				avg168 = 0.0;
				for(i = 0; i < count && i < 168; ++i){
					avg168 += prices[i];
				}
				avg168 /= (count > 168 ? 168 : count);

				avg24 = 0.0;
				for(i = 1; i <= 24 && i <= count; ++i){
					j = (168 - i + count) % 168;
					avg24 += prices[j];
				}
				avg24 /= (count > 24 ? 24 : count);

				avg72 = 0.0;
				for(i = 1; i <= 72 && i <= count; ++i){
					j = (168 - i + count) % 168;
					avg72 += prices[j];
				}
				avg72 /= (count > 72 ? 72 : count);

				/* update the daily & weekly standard deviations */
				std168 = 0.0;
				for(i = 0; i < count && i < 168; ++i){
					std168 += prices[i] * prices[i];
				}
				std168 /= (count > 168 ? 168 : count);
				std168 -= avg168*avg168;
				std168 = sqrt(fabs(std168));

				std24 = 0.0;
				for(i = 1; i <= 24 && i <= count; ++i){
					j = (168 - i + count) % 168;
					std24 += prices[j] * prices[j];
				}
				std24 /= (count > 24 ? 24 : count);
				std24 -= avg24*avg24;
				std24 = sqrt(fabs(std24));

				std72 = 0.0;
				for(i = 1; i <= 72 && i <= count; ++i){
					j = (168 - i + count) % 168;
					std72 += prices[j] * prices[j];
				}
				std72 /= (count > 72 ? 72 : count);
				std72 -= avg72*avg72;
				std72 = sqrt(fabs(std72));

				retry = 1;
			}

			/* update reference hour */
			lasthr = thishr;
		}

		market_id++;

		clearat = nextclear();
		gl_localtime(clearat,&dt);
//		if (verbose) gl_output("   ...%s opens for clearing of market_id %d at %s", gl_name(OBJECTHDR(this),name,sizeof(name)), (int32)market_id, gl_strtime(&dt,buffer,sizeof(buffer))?buffer:"unknown time");
	}
	return (retry ? t1 : -clearat); /* soft return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

TIMESTAMP stubauction::nextclear(void) const
{
	return gl_globalclock + (TIMESTAMP)(period - fmod((double)gl_globalclock, period));
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_stubauction(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(stubauction::oclass);
		if (*obj!=NULL)
		{
			stubauction *my = OBJECTDATA(*obj,stubauction);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(stubauction);
}

EXPORT int init_stubauction(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,stubauction)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(stubauction);
}

EXPORT int isa_stubauction(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,stubauction)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_stubauction(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	stubauction *my = OBJECTDATA(obj,stubauction);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			//GL_THROW("invalid pass request (%d)", pass);
			gl_error("invalid pass request (%d)", pass);
			t2 = TS_INVALID;
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;
		return t2;
	}
	SYNC_CATCHALL(stubauction);
}

