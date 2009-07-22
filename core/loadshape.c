/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file loadshape.c
	@addtogroup loadshape

	@par MT_ANALOG 
	<code>loadshape_create(s,MT_ANALOG,double energy)</code>
	where #energy is the total energy used over the schedule definition

	@par MT_PULSED
	<code>loadshape_create(s,MT_PULSED,double energy, double scalar, MACHINEPULSETYPE pulsetype, double pulsevalue)</code>
	where #energy is the total energy used over the schedule definition, #scalar is the number of pulses generated over the schedule
	definition, #pulsetype is the type of pulse generated (fixed duration or fixed power), and #pulsevalue is the value of the fixed part
	of the pulse.

	@par MT_MODULATED
	<code>loadshape_create(s,MT_PULSED,double energy, double scalar, MACHINEPULSETYPE pulsetype, double pulsevalue)</code>
	where #energy is the total energy used over the schedule definition, #scalar is the number of pulses generated over the schedule
	definition, #pulsetype is the type of pulse generated (fixed duration or fixed power), and #pulsevalue is the value of the fixed part
	of the pulse.

	@par MT_QUEUED
	<code>loadshape_create(s,MT_PULSED,double energy, double scalar, MACHINEPULSETYPE pulsetype, double pulsevalue, double q_on, double q_off)</code>
	where #energy is the total energy used over the schedule definition, #scalar is the number of pulses generated over the schedule
	definition, #pulsetype is the type of pulse generated (fixed duration or fixed power), #pulsevalue is the value of the fixed part
	of the pulse, #q_on is the pulse queue value at which the pulses are started, and #q_off is the pulse queue value at which the pulses are stopped.

**/

#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "platform.h"
#include "output.h"
#include "loadshape.h"
#include "exception.h"
#include "convert.h"
#include "globals.h"
#include "random.h"
#include "schedule.h"

static loadshape *loadshape_list = NULL;
/** Create a loadshape
	@return 1 on success, 0 on failure
 **/
int loadshape_create(void *data)
{
	loadshape *s = (loadshape*)data;
	s->schedule = NULL;
	s->next = loadshape_list;
	loadshape_list = s;
	return 1;
}

int loadshape_initall(void)
{
	loadshape *ls;
	for (ls=loadshape_list; ls!=NULL; ls=ls->next)
	{
		if (loadshape_init(ls)==1)
			return FAILED;
	}
	return SUCCESS;
}

void loadshape_recalc(loadshape *ls)
{
	if (ls->schedule->duration==0)
	{
		ls->load = 0;
		ls->t2=TS_NEVER;
		return;
	}
	switch (ls->type) {
	case MT_ANALOG:
		break;
	case MT_PULSED:
		ls->d[0] = 1/ls->params.pulsed.scalar; /* scalar determine how many pulses per period are emitted */
		ls->d[1] = 0;
		if (ls->s == 0) /* load is off */
			/* recalculate time to next on */
			ls->r = 1/random_exponential(ls->schedule->value * ls->params.pulsed.scalar);
		break;
	case MT_MODULATED:
		ls->d[0] = 1/ls->params.modulated.scalar; /* scalar determine how many pulses per period are emitted */
		ls->d[1] = 0;
		break;
	case MT_QUEUED:
		break;
	default:
		break;
	}
}

int loadshape_init(loadshape *ls) /**< load shape */
{
	/* check the schedule */
	if (ls->schedule==NULL)
	{
			output_error("loadshape_init(...) schedule is not specified");
			return 1;
	}

	/* some sanity checks */
	switch (ls->type) {
	case MT_ANALOG:
		if (ls->params.analog.energy<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) analog energy must be a positive number",ls->schedule->name);
			return 1;
		}
		break;
	case MT_PULSED:
		if (ls->params.pulsed.energy<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) pulsed energy must be a positive number",ls->schedule->name);
			return 1;
		}
		if (ls->params.pulsed.pulsetype==MPT_UNKNOWN)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) pulse type could not be inferred because either duration or power is missing",ls->schedule->name);
			return 1;
		}
		if (ls->params.pulsed.pulsetype==MPT_TIME && ls->params.pulsed.pulsevalue<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) pulse duration must be a positive number",ls->schedule->name);
			return 1;
		}
		if (ls->params.pulsed.pulsetype==MPT_POWER && ls->params.pulsed.pulsevalue<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) pulse power must be a positive number",ls->schedule->name);
			return 1;
		}
		if (ls->params.pulsed.scalar<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) pulse count must be a positive number",ls->schedule->name);
			return 1;
		}
		break;
	case MT_MODULATED:
		if (ls->params.modulated.energy<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) modulated energy must be a positive number",ls->schedule->name);
			return 1;
		}
		if (ls->params.modulated.pulsetype==MT_UNKNOWN)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) modulated pulse type could not be inferred because either duration or power is missing",ls->schedule->name);
			return 1;
		}
		if (ls->params.modulated.pulsetype==MPT_TIME && ls->params.modulated.pulsevalue<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) modulated pulse period must be a positive number",ls->schedule->name);
			return 1;
		}
		if (ls->params.modulated.pulsetype==MPT_POWER && ls->params.modulated.pulsevalue<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) modulated pulse power must be a positive number",ls->schedule->name);
			return 1;
		}
		if (ls->params.modulated.scalar<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) modulated pulse count must be a positive number",ls->schedule->name);
			return 1;
		}
		break;
	case MT_QUEUED:
		if (ls->params.queued.energy<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) queue energy must be a positive number",ls->schedule->name);
			return 1;
		}
		if (ls->params.queued.pulsetype==MT_UNKNOWN)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) queue pulse type could not be inferred because either duration or power is missing",ls->schedule->name);
			return 1;
		}
		if (ls->params.queued.pulsetype==MPT_TIME && ls->params.queued.pulsevalue<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) queue pulse duration must be a positive number",ls->schedule->name);
			return 1;
		}
		if (ls->params.queued.pulsetype==MPT_POWER && ls->params.queued.pulsevalue<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) queue pulse power must be a positive number",ls->schedule->name);
			return 1;
		}
		if (ls->params.queued.scalar<=0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) queue pulse count must be a positive number",ls->schedule->name);
			return 1;
		}
		if (ls->params.queued.q_on<=ls->params.queued.q_off)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) queue q_on threshold must be greater than q_off threshold (q_off=%f, q_on=%f)",ls->schedule->name,ls->params.queued.q_off,ls->params.queued.q_on);
			return 1;
		}
		break;
	default:
		output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) load shape type is invalid",ls->schedule->name);
		return 1;
		break;
	}
	
	/* randomize the initial state */
	if (ls->q==0) ls->q = random_uniform(ls->d[0], ls->d[1]); 

	/* initial power per-unit factor */
	if (ls->dPdV==0) ls->dPdV = 1.0;

	/* establish the initial parameters */
	loadshape_recalc(ls);

	return 0;
}

TIMESTAMP loadshape_sync(loadshape *ls, TIMESTAMP t1)
{
	/* if the clock is running */
	if (t1 > ls->t0)
	{
		double dt = (double)(t1 - ls->t0);
		switch (ls->type) {
		case MT_ANALOG:
			/* update load */
			ls->load = ls->schedule->value * ls->params.analog.energy / ls->schedule->duration * ls->dPdV;
			break;
		case MT_PULSED:
			/* update s and r */
			if (ls->q < ls->d[0])
			{
				/* turn load on - rate is based on duration/power given */
				ls->s = 1;
				if (ls->params.pulsed.pulsetype==MPT_POWER)

					/* fixed power */
					ls->load = ls->params.pulsed.pulsevalue * ls->dPdV;
				else

					/* fixed duration */
					ls->load = ls->params.pulsed.energy / ls->params.pulsed.pulsevalue * ls->dPdV;

				/* read is based on load */
				ls->r = -1/ls->load;

			}
			else if (ls->q > ls->d[1])
			{
				/* turn load off - rate is based of exponential distribution based on schedule value */
				ls->s = 0;
				ls->r = 1/random_exponential(ls->schedule->value * ls->params.pulsed.scalar);
			}
			/* else state remains unchanged */

			/* udpate q */ 
			ls->q += ls->r * dt;

			/* update load */
			break;
		case MT_MODULATED:
			/* update s and r */
			if (ls->q < ls->d[0])
			{
				ls->s = 1;
				ls->r = 1/(double)(ls->schedule->next_t-t1)/60;
			}
			else if (ls->q > ls->d[1])
			{
				ls->s = 0;
				ls->r = -1/(double)(ls->schedule->next_t-t1)/60;
			}
			/* else state remains unchanged */

			/* udpate q */ 
			ls->q += ls->r * dt;

			/* update load */
			#define duration (ls->params.modulated.energy / ls->params.modulated.pulsevalue / ls->schedule->value)
			if (ls->params.modulated.pulsetype==MPT_POWER)
				ls->load = ls->s * ls->params.modulated.energy / ls->params.modulated.pulsevalue / duration * ls->dPdV; 
			else /* MPT_TIME */
				ls->load = ls->s * ls->params.modulated.energy / duration / ls->params.modulated.scalar * ls->dPdV;
			break;
		case MT_QUEUED:
			/* update s and r */
			if (ls->q < ls->d[0])
			{
				ls->s = 1;
				ls->r = 1/(double)(ls->schedule->next_t-t1)/60;
			}
			else if (ls->q > ls->d[1])
			{
				ls->s = 0;
				ls->r = -1/(double)(ls->schedule->next_t-t1)/60;
			}
			/* else state remains unchanged */

			/* udpate q */ 
			ls->q += ls->r * dt;

			/* update load */
			break;
		default:
			break;
		}
	}
	return ls->t2;
}

TIMESTAMP loadshape_syncall(TIMESTAMP t1)
{
	loadshape *s;
	TIMESTAMP t2 = TS_NEVER;
	for (s=loadshape_list; s!=NULL; s=s->next)
	{
		TIMESTAMP t3 = loadshape_sync(s,t1);
		if (t3<t2) t2 = t3;
	}
	return t2;
}

int convert_from_loadshape(char *string,int size,void *data, PROPERTY *prop)
{
	loadshape *ls = (loadshape*)data;
	switch (ls->type) {
	case MT_ANALOG:
		return sprintf(string,"type: analog; schedule: %s; energy: %g kWh",
			ls->schedule->name, ls->params.analog.energy);
		break;
	case MT_PULSED:
		if (ls->params.pulsed.pulsetype==MPT_TIME)
			return sprintf(string,"type: pulsed; schedule: %s; energy: %g kWh; count: %d; duration: %g s",
			ls->schedule->name, ls->params.pulsed.energy, ls->params.pulsed.scalar, ls->params.pulsed.pulsevalue);
		else if (ls->params.pulsed.pulsetype==MPT_POWER)
			return sprintf(string,"type: pulsed; schedule: %s; energy: %g kWh; count: %d; power: %g kW",
			ls->schedule->name, ls->params.pulsed.energy, ls->params.pulsed.scalar, ls->params.pulsed.pulsevalue);
		else
		{
			output_error("convert_from_loadshape(...,data={schedule->name='%s',...},prop={name='%s',...}) has an invalid pulsetype", ls->schedule->name, prop->name);
			return 0;
		}
		break;
	case MT_MODULATED:
		if (ls->params.pulsed.pulsetype==MPT_TIME)
			return sprintf(string,"type: modulated; schedule: %s; energy: %g kWh; count: %d; duration: %g s",
			ls->schedule->name, ls->params.modulated.energy, ls->params.modulated.scalar, ls->params.modulated.pulsevalue);
		else if (ls->params.pulsed.pulsetype==MPT_POWER)
			return sprintf(string,"type: modulated; schedule: %s; energy: %g kWh; count: %d; power: %g kW",
			ls->schedule->name, ls->params.modulated.energy, ls->params.modulated.scalar, ls->params.modulated.pulsevalue);
		else
		{
			output_error("convert_from_loadshape(...,data={schedule->name='%s',...},prop={name='%s',...}) has an invalid pulsetype", ls->schedule->name, prop->name);
			return 0;
		}
		break;
	case MT_QUEUED:
		if (ls->params.pulsed.pulsetype==MPT_TIME)
			return sprintf(string,"type: queue; schedule: %s; energy: %g kWh; count: %d; duration: %g s; q_on: %g; q_off: %g",
			ls->schedule->name, ls->params.queued.energy, ls->params.queued.scalar, ls->params.queued.pulsevalue, ls->params.queued.q_on, ls->params.queued.q_off);
		else if (ls->params.pulsed.pulsetype==MPT_POWER)
			return sprintf(string,"type: queued; schedule: %s; energy: %g kWh; count: %d; power: %g kW; q_on: %g; q_off: %g",
			ls->schedule->name, ls->params.queued.energy, ls->params.queued.scalar, ls->params.queued.pulsevalue, ls->params.queued.q_on, ls->params.queued.q_off);
		else
		{
			output_error("convert_from_loadshape(...,data={schedule->name='%s',...},prop={name='%s',...}) has an invalid pulsetype", ls->schedule->name, prop->name);
			return 0;
		}
		break;
	}
	return 1;
} 
int convert_to_loadshape(char *string, void *data, PROPERTY *prop)
{
	loadshape *ls = (loadshape*)data;
	char buffer[1024];
	char *token = NULL;

	/* check string length before copying to buffer */
	if (strlen(string)>sizeof(buffer)-1)
	{
		output_error("convert_to_loadshape(string='%-.64s...', ...) input string is too long (max is 1023)",string);
		return 0;
	}
	strcpy(buffer,string);

	/* initial values */
	ls->type = MT_UNKNOWN;

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
		if (strcmp(param,"type")==0)
		{
			if (strcmp(value,"analog")==0)
			{
				ls->type = MT_ANALOG;
				ls->params.analog.energy = 0.0;
			}
			else if (strcmp(value,"pulsed")==0)
			{
				ls->type = MT_PULSED;
				ls->params.pulsed.energy = 0.0;
				ls->params.pulsed.pulsetype = MPT_UNKNOWN;
				ls->params.pulsed.pulsetype = 0.0;
				ls->params.pulsed.scalar = 0,0;
			}
			else if (strcmp(value,"modulated")==0)
			{
				ls->type = MT_MODULATED;
				ls->params.modulated.energy = 0.0;
				ls->params.modulated.pulsetype = MPT_UNKNOWN;
				ls->params.modulated.pulsetype = 0.0;
				ls->params.modulated.scalar = 0,0;
			}
			else if (strcmp(value,"queued")==0)
			{
				ls->type = MT_QUEUED;
				ls->params.queued.energy = 0.0;
				ls->params.queued.pulsetype = MPT_UNKNOWN;
				ls->params.queued.pulsetype = 0.0;
				ls->params.queued.scalar = 0,0;
				ls->params.queued.q_on = 0,0;
				ls->params.queued.q_off = 0,0;
			}
			else
			{
				output_error("convert_to_loadshape(string='%-.64s...', ...) type '%s' is invalid",string,value);
				return 0;
			}
		}
		else if (strcmp(param,"schedule")==0)
		{
			SCHEDULE *s = schedule_find_byname(value);
			if (s==NULL)
			{
				output_error("convert_to_loadshape(string='%-.64s...', ...) schedule '%s' does not exist",string,value);
				return 0;
			}
			ls->schedule = s;
		}
		else if (strcmp(param,"energy")==0)
		{
			int result;
			if (ls->type==MT_ANALOG)
				result=convert_unit_double(value,"kWh",&ls->params.analog.energy);
			else if (ls->type==MT_PULSED)
				result=convert_unit_double(value,"kWh",&ls->params.pulsed.energy);
			else if (ls->type==MT_MODULATED)
				result=convert_unit_double(value,"kWh",&ls->params.modulated.energy);
			else if (ls->type==MT_QUEUED)
				result=convert_unit_double(value,"kWh",&ls->params.queued.energy);
			else
			{
				output_error("convert_to_loadshape(string='%-.64s...', ...) unable to parse energy before type is specified",string);
				return 0;
			}
			if (result==0)
			{
				output_error("convert_to_loadshape(string='%-.64s...', ...) unit of energy parameter '%s' is incompatible with kWh",string, value);
				return 0;
			}
		}
		else if (strcmp(param,"count")==0)
		{
			if (ls->type==MT_ANALOG)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) count is not used by analog loadshapes",string);
			else if (ls->type==MT_PULSED)
				ls->params.pulsed.scalar = atof(value);
			else if (ls->type==MT_MODULATED)
				ls->params.modulated.scalar = atof(value);
			else if (ls->type==MT_QUEUED)
				ls->params.queued.scalar = atof(value);
			else
			{
				output_error("convert_to_loadshape(string='%-.64s...', ...) unable to parse count before type is specified",string);
				return 0;
			}
		}
		else if (strcmp(param,"duration")==0)
		{
			if (ls->type==MT_ANALOG)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) duration is not used by analog loadshapes",string);
			else if (ls->type==MT_PULSED)
			{
				if (ls->params.pulsed.pulsetype==MPT_POWER)
					output_warning("convert_to_loadshape(string='%-.64s...', ...) duration ignored because power has already been specified and is mutually exclusive",string);
				else
				{
					ls->params.pulsed.pulsetype = MPT_TIME;
					if (!convert_unit_double(value,"s",&ls->params.pulsed.pulsevalue))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert duration to seconds",string);
						return 0;
					}
				}
			}
			else if (ls->type==MT_MODULATED)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) duration is not used by modulated loadshapes",string);
			else if (ls->type==MT_QUEUED)
			{
				if (ls->params.queued.pulsetype==MPT_POWER)
					output_warning("convert_to_loadshape(string='%-.64s...', ...) duration ignored because power has already been specified and is mutually exclusive",string);
				else
				{
					ls->params.queued.pulsetype = MPT_TIME;
					if (!convert_unit_double(value,"s",&ls->params.queued.pulsevalue))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert duration to seconds",string);
						return 0;
					}
				}
			}
			else
			{
				output_error("convert_to_loadshape(string='%-.64s...', ...) unable to parse duration before type is specified",string);
				return 0;
			}
		}
		else if (strcmp(param,"period")==0)
		{
			if (ls->type==MT_ANALOG)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) period is not used by analog loadshapes",string);
			else if (ls->type==MT_PULSED)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) duration is not used by modulated loadshapes",string);
			else if (ls->type==MT_MODULATED)
			{
				if (ls->params.modulated.pulsetype==MPT_POWER)
					output_warning("convert_to_loadshape(string='%-.64s...', ...) period ignored because power has already been specified and is mutually exclusive",string);
				else
				{
					ls->params.modulated.pulsetype = MPT_TIME;
					if (!convert_unit_double(value,"s",&ls->params.modulated.pulsevalue))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert period to seconds",string);
						return 0;
					}
				}
			}
			else if (ls->type==MT_QUEUED)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) duration is not used by modulated loadshapes",string);
			else
			{
				output_error("convert_to_loadshape(string='%-.64s...', ...) unable to parse duration before type is specified",string);
				return 0;
			}
		}
		else if (strcmp(param,"power")==0)
		{
			if (ls->type==MT_ANALOG)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) power is not used by analog loadshapes",string);
			else if (ls->type==MT_PULSED)
				if (ls->params.pulsed.pulsetype==MPT_TIME)
					output_warning("convert_to_loadshape(string='%-.64s...', ...) power ignored because duration has already been specified and is mutually exclusive",string);
				else
				{
					ls->params.pulsed.pulsetype = MPT_POWER;
					if (!convert_unit_double(value,"kW",&ls->params.pulsed.pulsevalue))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert power to unit kW",string);
						return 0;
					}
				}
			else if (ls->type==MT_MODULATED)
				if (ls->params.modulated.pulsetype==MPT_TIME)
					output_warning("convert_to_loadshape(string='%-.64s...', ...) power ignored because period has already been specified and is mutually exclusive",string);
				else
				{
					ls->params.modulated.pulsetype = MPT_POWER;
					if (!convert_unit_double(value,"kW",&ls->params.modulated.pulsevalue))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert power to unit kW",string);
						return 0;
					}
				}
			else if (ls->type==MT_QUEUED)
				if (ls->params.queued.pulsetype==MPT_TIME)
					output_warning("convert_to_loadshape(string='%-.64s...', ...) power ignored because duration has already been specified and is mutually exclusive",string);
				else
				{
					ls->params.queued.pulsetype = MPT_POWER;
					if (!convert_unit_double(value,"kW",&ls->params.queued.pulsevalue))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert power to unit kW",string);
						return 0;
					}
				}
			else
			{
				output_error("convert_to_loadshape(string='%-.64s...', ...) unable to parse count before type is specified",string);
				return 0;
			}
		}
		else if (strcmp(param,"q_on")==0)
		{
			if (ls->type==MT_ANALOG)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) q_on is not used by analog loadshapes",string);
			else if (ls->type==MT_PULSED)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) q_on is not used by pulsed loadshapes",string);
			else if (ls->type==MT_MODULATED)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) q_on is not used by modulated loadshapes",string);
			else if (ls->type==MT_QUEUED)
				ls->params.queued.q_on = atof(value);
		}
		else if (strcmp(param,"q_off")==0)
		{
			if (ls->type==MT_ANALOG)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) q_off is not used by analog loadshapes",string);
			else if (ls->type==MT_PULSED)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) q_off is not used by pulsed loadshapes",string);
			else if (ls->type==MT_MODULATED)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) q_off is not used by modulated loadshapes",string);
			else if (ls->type==MT_QUEUED)
				ls->params.queued.q_off = atof(value);
		}
		else
		{
			output_error("convert_to_loadshape(string='%-.64s...', ...) parameter '%s' is not valid",string,param);
			return 0;
		}
	}

	/* reinitialize the loadshape */
	if (loadshape_init((loadshape*)data))
		return 0;

	/* everything converted ok */
	return 1;
} 

int loadshape_test(void)
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

	output_test("\nBEGIN: loadshape tests");
	for (p=test;p<test+sizeof(test)/sizeof(test[0]);p++)
	{
	}

	/* TODO now ok to do loadshape tests */

	/* report results */
	if (failed)
	{
		output_error("loadshapetest: %d loadshape tests failed--see test.txt for more information",failed);
		output_test("!!! %d loadshape tests failed, %d errors found",failed,errorcount);
	}
	else
	{
		output_verbose("%d loadshape tests completed with no errors--see test.txt for details",ok);
		output_test("loadshapetest: %d loadshape tests completed, %d errors found",ok,errorcount);
	}
	output_test("END: loadshape tests");
	return failed;
}

