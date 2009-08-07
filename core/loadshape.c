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

static void sync_analog(loadshape *ls, double dt)
{
	if (ls->params.analog.energy>0)

		/* load is based on fixed energy scale */
		ls->load = ls->schedule->value * ls->params.analog.energy * ls->schedule->fraction * ls->dPdV;
	else if (ls->params.analog.power>0)

		/* load is based on fixed power scale */
		ls->load = ls->schedule->value * ls->params.analog.power * ls->dPdV;
	else

		/* load is based on direct value (no scale) */
		ls->load = ls->schedule->value * ls->dPdV;
}

static void sync_pulsed(loadshape *ls, double dt)
{
	/* load is off or waiting to choose a state */
	if (ls->r >= 0)
	{
		/* if load should turn on within the next second */
		if (ls->r!=0 && ls->q >= ls->d[0] - ls->r/3600)
		{
			/* turn load on */
			ls->q = 1;
#ifdef _DEBUG
			output_debug("loadshape %s: turns on", ls->schedule->name);
#endif
			goto TurnOn;
		}
TurnOff:
		/* load is off */
		ls->s = 0;

		/* no load when off */
		ls->load = 0;

		/* if a value is given in the schedule */
		if (ls->schedule->value>0) 
		{
			/* calculate the decay rate of the queue */
			ls->r = ls->schedule->value*ls->params.pulsed.scalar/ls->params.pulsed.energy;
			if (ls->r<=0)
				output_warning("loadshape %s: r not positive while load is off!", ls->schedule->name);
		}
		else
		{
			/* the queue doesn't change (no decay) */
			ls->r = 0;
			output_warning("loadshape %s: state machine suspended because schedule has zero value", ls->schedule->name);
		}
	}

	/* load is on */
	else
	{
		/* the load will turn off within the next second */
		if (ls->r!=0 && ls->q <= ls->d[1] - ls->r/3600)
		{
			/* turn the load off */
			ls->q = 0;
#ifdef _DEBUG
			output_debug("loadshape %s: turns off", ls->schedule->name);
#endif
			goto TurnOff;
		}
TurnOn:	
		/* the load is on */
		ls->s = 1;

		/* fixed power pulse */
		if (ls->params.pulsed.pulsetype==MPT_POWER)
		{
			/* load has fixed power */
			ls->load = ls->params.pulsed.pulsevalue * ls->dPdV;
			
			/* rate is based on energy and load */
			ls->r = -1/(ls->params.pulsed.energy/ls->load);
			if (ls->r>=0)
				output_warning("loadshape %s: r not negative while load is on!", ls->schedule->name);
		}
		else if (ls->params.pulsed.pulsevalue!=0)
		{
			/* load has fixed duration so power is energy/duration */
			ls->load = ls->params.pulsed.energy / ls->params.pulsed.pulsevalue *3600 * ls->dPdV;
			
			/* rate is based on time */
			ls->r = -3600/ls->params.pulsed.pulsevalue;
			if (ls->r>=0)
				output_warning("loadshape %s: r not negative while load is on!", ls->schedule->name);
		}
		else
		{
			/* can't have load now */
			output_warning("loadshape %s: load value is zero in 'on' state", ls->schedule->name);
		}
	}
}

static void sync_modulated(loadshape *ls, double dt)
{
	/* load is off or waiting to choose a state */
	if (ls->r >= 0)
	{
		if (ls->r!=0 && ls->q >= ls->d[0] - ls->r/3600)
		{
			ls->q = 1;
#ifdef _DEBUG
			output_debug("loadshape %s: turns on", ls->schedule->name);
#endif
			goto TurnOn;
		}
TurnOff:
		ls->s = 0;
		ls->load = 0;

		// amplitude modulation
		if (ls->schedule->value>0) 
		{
			if (ls->params.modulated.modulation==MMT_AMPLITUDE) 
			{
				// AM off time
				double period = ls->schedule->duration / ls->params.modulated.scalar;
				double duty_cycle = (ls->params.modulated.pulsetype==MPT_TIME) 
					? ls->params.modulated.pulsevalue / period 
					: ls->params.modulated.energy * 3600 / ls->params.modulated.pulsevalue / period;
				ls->r = 1 / (period - duty_cycle * period);
			}

			// pulse-width modulation
			else if (ls->params.modulated.modulation==MMT_PULSEWIDTH) 
			{
				// PWM off time
				double power = (ls->params.modulated.pulsetype==MPT_TIME)
					? ls->params.modulated.energy * 3600 / ls->params.modulated.pulsevalue
					: ls->params.modulated.pulsevalue;
				double period = ls->schedule->duration / ls->params.modulated.scalar;
				double ton = ls->schedule->value * ls->params.modulated.scalar / ls->params.modulated.energy / ls->params.modulated.scalar;
				ls->r = 1 / (period - ton);
			}

			// frequency modulation
			else if (ls->params.modulated.modulation==MMT_FREQUENCY) 
			{
				double ton = ls->params.modulated.pulsevalue;
				double power = ls->params.modulated.pulsevalue;
				double period, toff;
				if (ls->params.modulated.pulsetype==MPT_TIME)
					power = ls->params.modulated.energy * ls->params.modulated.scalar / ton * 3600;
				else
					ton = ls->params.modulated.energy * ls->params.modulated.scalar / power * 3600;
				period = ls->schedule->duration /  ls->params.modulated.scalar;
				toff = period /  ls->params.modulated.scalar - ton;
				ls->r = 1/toff;
			}
			else
				output_warning("loadshape %s: modulation type is not determined!", ls->schedule->name);
		}
		else
		{
			ls->r = 0;
			output_warning("loadshape %s: state machine suspended because schedule has zero value", ls->schedule->name);
		}
	}

	/* load is on */
	else
	{
		/* ready to turn off */
		if (ls->r!=0 && ls->q <= ls->d[1] - ls->r/3600)
		{
			ls->q = 0;
#ifdef _DEBUG
			output_debug("loadshape %s: turns off", ls->schedule->name);
#endif
			goto TurnOff;
		}
TurnOn:	
		ls->s = 1;

		// amplitude modulation
		if (ls->params.modulated.modulation==MMT_AMPLITUDE) 
		{
			// AM on time
			double period = ls->schedule->duration / ls->params.modulated.scalar;
			double duty_cycle = (ls->params.modulated.pulsetype==MPT_TIME) 
					? ls->params.modulated.pulsevalue / period 
					: ls->params.modulated.energy * 3600 / ls->params.modulated.pulsevalue / period;
			ls->r  = 1 / (duty_cycle * period);
			ls->load = ls->schedule->value * ls->params.modulated.scalar;
		}

		// pulse-width modulation
		else if (ls->params.modulated.modulation==MMT_PULSEWIDTH) 
		{
			// PWM on time
			double power = (ls->params.modulated.pulsetype==MPT_TIME)
				? ls->params.modulated.energy * 3600 / ls->params.modulated.pulsevalue
				: ls->params.modulated.pulsevalue;
			double pulsecount = ls->params.modulated.energy / power * ls->schedule->duration / 3600;
			double period = ls->schedule->duration / pulsecount;
			double ton = ls->schedule->value * ls->params.modulated.scalar / ls->params.modulated.energy / pulsecount;
			ls->r = 1 / ton;
			ls->load = power;
		}

		// frequency modulation
		else if (ls->params.modulated.modulation==MMT_FREQUENCY) // frequency modulation
		{
			double ton = ls->params.modulated.pulsevalue;
			double power = ls->params.modulated.pulsevalue;
			if (ls->params.modulated.pulsetype==MPT_TIME)
				power = ls->params.modulated.energy * ls->params.modulated.scalar / ton * 3600;
			else
				ton = ls->params.modulated.energy * ls->params.modulated.scalar / power * 3600;
			ls->r = 1/ton;
		}
		else
			output_warning("loadshape %s: modulation type is not determined!", ls->schedule->name);
	}
}

static void sync_queued(loadshape *ls, double dt)
{
	double queue_value = (ls->d[1] - ls->d[0]);
	if (ls->params.modulated.pulsetype==MPT_POWER)
		ls->load = ls->s * ls->params.modulated.pulsevalue * ls->dPdV; 
	else /* MPT_TIME */
		ls->load = ls->s * ls->params.modulated.energy / ls->params.modulated.pulsevalue / ls->params.modulated.scalar * ls->dPdV;		

#define duration ((ls->params.modulated.energy*queue_value)/ ls->load)

	/* update s and r */
	if (ls->q > ls->d[0])
	{
		ls->s = 1;

		ls->r = -1/duration;
		
	}
	else if (ls->q < ls->d[1])
	{
		ls->s = 0;
		ls->r = 1/random_exponential(ls->schedule->value*ls->params.pulsed.scalar*queue_value);
	}
	/* else state remains unchanged */

	/* udpate q */ 
	ls->q += ls->r * dt;
#undef duration
	
}

/** Create a loadshape
	@return 1 on success, 0 on failure
 **/
int loadshape_create(void *data)
{
	loadshape *s = (loadshape*)data;
	memset(data,0,sizeof(loadshape));
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
	/* no schedule -> don't touch the loadshape */
	if (ls->schedule==NULL)
		return;

	switch (ls->type) {
	case MT_ANALOG:
		break;
	case MT_PULSED:
		ls->d[MS_OFF] = 1; /* scalar determine how many pulses per period are emitted */
		ls->d[MS_ON] = 0;
		ls->q = random_uniform(0,1);
		sync_pulsed(ls,0);
		break;
	case MT_MODULATED:
		ls->d[MS_OFF] = 1; /* scalar determine how many pulses per period are emitted */
		ls->d[MS_ON] = 0;
		break;
	case MT_QUEUED:
		ls->d[MS_OFF] = ls->params.queued.q_off;
		ls->d[MS_ON] = ls->params.queued.q_on;
		if (ls->s == 0) /* load is off */
		{
			/* recalculate time to next on */
			ls->r = 1/random_exponential(ls->schedule->value * ls->params.pulsed.scalar * (ls->params.queued.q_on - ls->params.queued.q_off));
		}
		break;
	default:
		break;
	}

	if (ls->schedule->duration==0)
	{
		ls->load = 0;
		ls->t2=TS_NEVER;
		return;
	}
}

int loadshape_init(loadshape *ls) /**< load shape */
{
	/* no schedule -> nothing to initialized */
	if (ls->schedule==NULL)
		return 0;

	/* some sanity checks */
	switch (ls->type) {
	case MT_ANALOG:
		if (ls->params.analog.energy<0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) analog energy must be a positive number",ls->schedule->name);
			return 1;
		}
		else if (ls->params.analog.power<0)
		{
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) analog power must be a positive number",ls->schedule->name);
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
		if (ls->params.modulated.modulation<=MMT_UNKNOWN || ls->params.modulated.modulation>MMT_FREQUENCY)
		{
			char *modulation[] = {"unknown","amplitude","pulsewidth","frequency"};
			output_error("loadshape_init(loadshape *ls={schedule->name='%s',...}) modulation type %s is invalid",ls->schedule->name,modulation[ls->params.modulated.modulation]);
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
	
	/* establish the initial parameters */
	loadshape_recalc(ls);

	/* randomize the initial state */
	if (ls->q==0) ls->q = ls->d[0]<ls->d[1] ? random_uniform(ls->d[0], ls->d[1]) : random_uniform(ls->d[1], ls->d[0]); ; 

	/* initial power per-unit factor */
	if (ls->dPdV==0) ls->dPdV = 1.0;

	return 0;
}

TIMESTAMP loadshape_sync(loadshape *ls, TIMESTAMP t1)
{
#ifdef _DEBUG
//	int ps = ls->s;
#endif

	/* if the clock is running and the loadshape is driven by a schedule */
	if (ls->schedule!=NULL && t1 > ls->t0)
	{
		double dt = ls->t0>0 ? (double)(t1 - ls->t0)/3600 : 0.0;

		/* do not change anything if the duration of the schedule is invalid */
		if (ls->schedule->duration<=0)
		{
			ls->t0 = t1;
			return TS_NEVER;
		}

		switch (ls->type) {
		case MT_ANALOG:

			sync_analog(ls, dt);

			/* time to next event determined by schedule */
			ls->t2 = ls->schedule->next_t;
			break;

		case MT_PULSED:

			/* udpate q */ 
			ls->q += ls->r * dt;

			sync_pulsed(ls, dt);

#ifdef _DEBUG
			if (ls->s==0 && ls->r<0)
			{
				output_error("loadshape %s: state inconsistent (s=on, r<0)!", ls->schedule->name);
				return ls->t2 = TS_NEVER;
			}
			else if (ls->s==1 && ls->r>0)
			{
				output_error("loadshape %s: state inconsistent (s=off, r>0)!", ls->schedule->name);
				return ls->t2 = TS_NEVER;
			}
#endif

			/* time to next event */
			ls->t2 = ls->r!=0 ? t1 + (TIMESTAMP)(( ls->d[ls->s] - ls->q) / ls->r * 3600) : TS_NEVER;
#ifdef _DEBUG
			{
				char buf[64];
				output_debug("value = %5.3f, q = %5.3f, r = %+5.3f, %s", ls->schedule->value, ls->q, ls->r, convert_from_timestamp(ls->t2,buf,sizeof(buf))?buf:"(error)");
			}
#endif
			/* choose sooner of schedule change or state change */
			if (ls->schedule->next_t < ls->t2) ls->t2 = ls->schedule->next_t;
			break;

		case MT_MODULATED:

			/* udpate q */ 
			ls->q += ls->r * dt;

			sync_modulated(ls, dt);

			/* time to next event */
			ls->t2 = ls->r!=0 ? t1 + (TIMESTAMP)(( ls->d[ls->s] - ls->q) / ls->r * 3600) + 1 : TS_NEVER;

			/* choose sooner of schedule change or state change */
			if (ls->schedule->next_t < ls->t2) ls->t2 = ls->schedule->next_t;
			break;

		case MT_QUEUED:


			/* udpate q */ 
			ls->q += ls->r * dt;

			sync_queued(ls, dt);

			/* time to next event */
			ls->t2 = ls->r!=0 ? t1 + (TIMESTAMP)(( ls->d[ls->s] - ls->q) / ls->r * 3600) + 1 : TS_NEVER;

			/* choose sooner of schedule change or state change */
			if (ls->schedule->next_t < ls->t2) ls->t2 = ls->schedule->next_t;
			break;
			

		default:
			break;
		}
	}
#ifdef _DEBUG
//	output_debug("loadshape %s: load = %.3f", ls->schedule->name, ls->load);
//	output_debug("loadshape %s: t2-t1 = %d", ls->schedule->name, ls->t2 - global_clock);
#endif
	ls->t0 = t1;
	return ls->t2>0?ls->t2:TS_NEVER;
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
	char *modulation[] = {"unknown","amplitude","pulsewidth","frequency"};
	loadshape *ls = (loadshape*)data;
	switch (ls->type) {
	case MT_ANALOG:
		if (ls->params.analog.energy>0)
			return sprintf(string,"type: analog; schedule: %s; energy: %g kWh",	ls->schedule->name, ls->params.analog.energy);
		else if (ls->params.analog.power>0)
			return sprintf(string,"type: analog; schedule: %s; power: %g kW",	ls->schedule->name, ls->params.analog.power);
		else
			return sprintf(string,"type: analog; schedule: %s", ls->schedule->name);
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
			return sprintf(string,"type: modulated; schedule: %s; energy: %g kWh; count: %d; duration: %g s; modulation: %s",
			ls->schedule->name, ls->params.modulated.energy, ls->params.modulated.scalar, ls->params.modulated.pulsevalue,modulation[ls->params.modulated.modulation]);
		else if (ls->params.pulsed.pulsetype==MPT_POWER)
			return sprintf(string,"type: modulated; schedule: %s; energy: %g kWh; count: %d; power: %g kW; modulation: %s",
			ls->schedule->name, ls->params.modulated.energy, ls->params.modulated.scalar, ls->params.modulated.pulsevalue,modulation[ls->params.modulated.modulation]);
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
			{
				if (!convert_unit_double(value,"kW",&ls->params.analog.power))
				{
					output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert power to unit kW",string);
					return 0;
				}
			}
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
		else if (strcmp(param,"stdev")==0)
		{
			double dev = atof(value);
			double err = random_triangle(-3,3);
			if (ls->type==MT_ANALOG)
			{
				if (ls->params.analog.energy!=0) 
				{
					if (!convert_unit_double(value,"kWh",&dev))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert stdev to unit kWh",string);
						return 0;
					}
					ls->params.analog.energy += dev*err;
				}
				else
				{
					if (!convert_unit_double(value,"kW",&dev))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert stdev to unit kW",string);
						return 0;
					}
					ls->params.analog.power += dev*err;
				}
			}
			else if (ls->type==MT_PULSED)
			{
				if (ls->params.pulsed.pulsetype == MPT_TIME)
				{
					if (!convert_unit_double(value,"s",&dev))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert stdev to unit s",string);
						return 0;
					}
					ls->params.pulsed.pulsevalue += dev*err;
					}
				else if (ls->params.pulsed.pulsetype == MPT_POWER)
				{
					if (!convert_unit_double(value,"kW",&dev))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert stdev to unit kW",string);
						return 0;
					}
					ls->params.pulsed.pulsevalue += dev*err;
				}
			}
			else if (ls->type==MT_MODULATED)
			{
				if (ls->params.modulated.pulsetype == MPT_TIME)
				{
					if (!convert_unit_double(value,"s",&dev))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert stdev to unit s",string);
						return 0;
					}
					ls->params.modulated.pulsevalue += dev*err;
				}
				else if (ls->params.modulated.pulsetype == MPT_POWER)
				{
					if (!convert_unit_double(value,"kW",&dev))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert stdev to unit kW",string);
						return 0;
					}
					ls->params.modulated.pulsevalue += dev*err;
				}
			}
			else if (ls->type==MT_QUEUED)
			{
				if (ls->params.queued.pulsetype == MPT_TIME)
				{
					if (!convert_unit_double(value,"s",&dev))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert stdev to unit s",string);
						return 0;
					}
					ls->params.queued.pulsevalue += dev*err;
				}
				else if (ls->params.queued.pulsetype == MPT_POWER)
				{
					if (!convert_unit_double(value,"kW",&dev))
					{
						output_error("convert_to_loadshape(string='%-.64s...', ...) unable to convert stdev to unit kW",string);
						return 0;
					}
					ls->params.queued.pulsevalue += dev*err;
				}
			}
		}
		else if (strcmp(param,"modulation")==0)
		{
			if (ls->type==MT_ANALOG)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) modulation is not used by analog loadshapes",string);
			else if (ls->type==MT_PULSED)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) modulation is not used by pulsed loadshapes",string);
			else if (ls->type==MT_MODULATED)
			{
				if (strcmp(value,"amplitude")==0)
					ls->params.modulated.modulation = MMT_AMPLITUDE;
				else if (strcmp(value,"pulsewidth")==0)
					ls->params.modulated.modulation = MMT_PULSEWIDTH;
				else if (strcmp(value,"frequency")==0)
					ls->params.modulated.modulation = MMT_FREQUENCY;
				else
				{
					output_error("convert_to_loadshape(string='%-.64s...', ...) '%s' is not a recognized modulation",string,value);
					return 0;
				}
			}
			else if (ls->type==MT_QUEUED)
				output_warning("convert_to_loadshape(string='%-.64s...', ...) modulation is not used by queued loadshapes",string);
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

