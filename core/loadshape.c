/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file loadshape.c
	@addtogroup loadshape
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
    The arguments depend on the loadshape machine type:
	
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

	@return a pointer to the new state machine on success, or NULL on failure
 **/
loadshape *loadshape_create(SCHEDULE *s,	/**< the schedule that govern this machine */
						MACHINETYPE t,	/**< the type of machine (see #MACHINETYPE) */
						...)			/**< the machine specification (depends on #MACHINETYPE) */
{
	va_list arg;
	loadshape *machine = malloc(sizeof(loadshape));
	if (machine==NULL)
	{
		output_error("loadshape_create(SCHEDULE *s={name: %s; ...}, ...): memory allocation error", s->name);
		/* TROUBLESHOOT
			An attempt to create a schedule state machine failed because of insufficient system resources.  Free some system memory and try again.
		 */
		return NULL;
	}	
	va_start(arg,t);
	switch (t) {
	case MT_ANALOG:
		machine->params.analog.energy = va_arg(arg,double);
		if (machine->params.analog.energy<0)
		{
			output_error("loadshape_create(SCHEDULE *s={name: %s; ...}, MACHINETYPE t=MT_ANALOG, double energy=%g): scheduled energy is negative", s->name, machine->params.analog.energy);
			/* TROUBLESHOOT
				The machine uses a schedule that has a negative energy.  Specify a zero or position scheduled energy and try again.
			 */
			goto Error;
		}
		break;
	case MT_PULSED:
		machine->params.pulsed.energy = va_arg(arg,double);
		if (machine->params.analog.energy<0)
		{
			output_error("loadshape_create(SCHEDULE *s={name: %s; ...}, MACHINETYPE t=MT_PULSED, double energy=%g): scheduled energy is negative", s->name, machine->params.analog.energy);
			/* TROUBLESHOOT
				The machine uses a schedule that has a negative energy.  Specify a zero or position scheduled energy and try again.
			 */
			goto Error;
		}
		machine->params.pulsed.scalar = va_arg(arg,double);
		machine->params.pulsed.pulsetype = va_arg(arg,MACHINEPULSETYPE);
		machine->params.pulsed.pulsevalue = va_arg(arg,double);
		break;
	case MT_MODULATED:
		machine->params.modulated.energy = va_arg(arg,double);
		if (machine->params.analog.energy<0)
		{
			output_error("loadshape_create(SCHEDULE *s={name: %s; ...}, MACHINETYPE t=MT_MODULATED, double energy=%g): scheduled energy is negative", s->name, machine->params.analog.energy);
			/* TROUBLESHOOT
				The machine uses a schedule that has a negative energy.  Specify a zero or position scheduled energy and try again.
			 */
			goto Error;
		}
		machine->params.modulated.scalar = va_arg(arg,double);
		machine->params.modulated.pulsetype = va_arg(arg,MACHINEPULSETYPE);
		machine->params.modulated.pulsevalue = va_arg(arg,double);
		break;
	case MT_QUEUED:
		machine->params.queued.energy = va_arg(arg,double);
		if (machine->params.analog.energy<0)
		{
			output_error("loadshape_create(SCHEDULE *s={name: %s; ...}, MACHINETYPE t=MT_QUEUED, double energy=%g): scheduled energy is negative", s->name, machine->params.analog.energy);
			/* TROUBLESHOOT
				The machine uses a schedule that has a negative energy.  Specify a zero or position scheduled energy and try again.
			 */
			goto Error;
		}
		machine->params.queued.scalar = va_arg(arg,double);
		machine->params.queued.pulsetype = va_arg(arg,MACHINEPULSETYPE);
		machine->params.queued.pulsevalue = va_arg(arg,double);
		machine->params.queued.q_on = va_arg(arg,double);
		machine->params.queued.q_off = va_arg(arg,double);
		break;
	default:
		output_error("loadshape_create(SCHEDULE *s={name: %s; ...}, MACHINETYPE t=%d, ...): memory allocation error", s->name);
		/* TROUBLESHOOT
			The machine type is not defined.  Specify a defined machine type (see MACHINETYPE) and try again.
		 */
		goto Error;
	}
	va_end(arg);
	machine->next = loadshape_list;
	loadshape_list = machine;
	return machine;
Error:
	free(machine);
	return NULL;
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

	/* normalize the schedule */
	schedule_normalize(ls->schedule,FALSE);

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

	/* Add to list of active loadshapes only if not already in list 
	   Note: this is the most efficient way to do it, but it avoids having
	   to implement support for creators for all core properties
	 */
	/* search loadshape list for this load shape */
	for (ls=loadshape_list; ls!=NULL; ls=ls->next)
	{
		if (ls==(loadshape*)data)
			break;
	}

	/* if not found in list */
	if (ls==NULL)
	{
		ls = (loadshape*)data;
		ls->next = loadshape_list;
		loadshape_list = ls;
	}
	
	/* reinitialize the loadshape */
	else if (loadshape_init((loadshape*)data)==FAILED)
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

	/* first do schedule tests */
	struct s_test {
		char *name, *def;
		char *t1, *t2;
		int normalized;
		double value;
	} *p, test[] = {
		/* schedule name	schedule definition							sync time				next time expected		normalize	value expected */
		{"empty",			"", 										"2000/01/01 00:00:00",	"NEVER",				0,			0.0},
		{"halfday-binary",	"* 12-23 * * *",							"2001/02/03 01:30:00",	"2001/02/03 12:00:00",	0,			0.0},
		{"halfday-binary",	NULL,										"2002/03/05 13:45:00",	"2002/03/06 00:00:00",	0,			1.0},
		{"halfday-bimodal",	"* 0-11 * * * 0.25; * 12-23 * * * 0.75;",	"2003/04/07 01:15:00",	"2003/04/07 12:00:00",	0,			0.25},
		{"halfday-bimodal",	"* 0-11 * * * 0.25; * 12-23 * * * 0.75;",	"2004/05/09 00:00:00",	"2004/05/09 12:00:00",	0,			0.25},
		{"halfday-bimodal",	NULL,										"2005/06/11 13:20:00",	"2005/06/12 00:00:00",	0,			0.75},
		{"halfday-bimodal",	NULL,										"2006/07/13 12:00:00",	"2006/07/14 00:00:00",	0,			0.75},
		{"halfday-bimodal",	NULL,										"2007/08/15 00:00:00",	"2007/08/15 12:00:00",	0,			0.25},
		{"quarterday-normal", "* 0-5 * * *; * 12-17 * * *;",			"2008/09/17 00:00:00",	"2008/09/17 06:00:00",	1,			0.5},
		{"quarterday-normal", NULL,										"2009/10/19 06:00:00",	"2009/10/19 12:00:00",	1,			0.0},
		{"quarterday-normal", NULL,										"2010/11/21 12:00:00",	"2010/11/21 18:00:00",	1,			0.5},
		{"quarterday-normal", NULL,										"2011/12/23 18:00:00",	"2011/12/24 00:00:00",	1,			0.0},
	};

	for (p=test;p<test+sizeof(test)/sizeof(test[0]);p++)
	{
		TIMESTAMP t1 = convert_to_timestamp(p->t1);
		int errors=0;
		SCHEDULE *s = schedule_create(p->name, p->def);
		output_test("Schedule %s { %s } sync to %s...", p->name, p->def, convert_from_timestamp(t1,ts,sizeof(ts))?ts:"???");
		if (s==NULL)
		{
			output_test(" ! schedule %s { %s } create failed", p->name, p->def);
			errors++;
		}
		else
		{
			TIMESTAMP t2;
			if (p->normalized) 
				schedule_normalize(s,FALSE);
			t2 = s?schedule_sync(s,t1):TS_NEVER;
			if (s->value!=p->value)
			{
				output_test(" ! expected value %lg but found %lg", s->name, p->value, s->value);
				errors++;
			}
			if (t2!=convert_to_timestamp(p->t2))
			{
				output_test(" ! expected next time %s but found %s", p->t2, convert_from_timestamp(t2,ts,sizeof(ts))?ts:"???");
				errors++;
			}
		}
		if (errors==0)
		{
			output_test("   test passed");
			ok++;
		}
		else
			failed++;
		errorcount+=errors;
	}

	/* TODO loadshape tests */

	/* report results */
	if (failed)
	{
		output_error("loadshapetest: %d schedule tests failed--see test.txt for more information",failed);
		output_test("!!! %d schedule tests failed, %d errors found",failed,errorcount);
	}
	else
	{
		output_message("%d schedule tests completed with 0 errors--see test.txt for more information",ok);
		output_test("loadshapetest: %d schedule tests completed, %d errors found",ok,errorcount);
	}
	return failed;
}

