/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file statemachine.c
	@addtogroup statemachine
**/

#include <stdlib.h>
#include <stdarg.h>

#include "platform.h"
#include "output.h"
#include "statemachine.h"
#include "exception.h"

/** Create a schedule state machine
    The arguments depend on the machine type:
	
	@par MT_ANALOG 
	<code>schedule_fsm_create(s,MT_ANALOG,double energy)</code>
	where #energy is the total energy used over the schedule definition

	@par MT_PULSED
	<code>schedule_fsm_create(s,MT_PULSED,double energy, double scalar, MACHINEPULSETYPE pulsetype, double pulsevalue)</code>
	where #energy is the total energy used over the schedule definition, #scalar is the number of pulses generated over the schedule
	definition, #pulsetype is the type of pulse generated (fixed duration or fixed power), and #pulsevalue is the value of the fixed part
	of the pulse.

	@par MT_MODULATED
	<code>schedule_fsm_create(s,MT_PULSED,double energy, double scalar, MACHINEPULSETYPE pulsetype, double pulsevalue)</code>
	where #energy is the total energy used over the schedule definition, #scalar is the number of pulses generated over the schedule
	definition, #pulsetype is the type of pulse generated (fixed duration or fixed power), and #pulsevalue is the value of the fixed part
	of the pulse.

	@par MT_QUEUED
	<code>schedule_fsm_create(s,MT_PULSED,double energy, double scalar, MACHINEPULSETYPE pulsetype, double pulsevalue, double q_on, double q_off)</code>
	where #energy is the total energy used over the schedule definition, #scalar is the number of pulses generated over the schedule
	definition, #pulsetype is the type of pulse generated (fixed duration or fixed power), #pulsevalue is the value of the fixed part
	of the pulse, #q_on is the pulse queue value at which the pulses are started, and #q_off is the pulse queue value at which the pulses are stopped.

	@return a pointer to the new state machine on success, or NULL on failure
 **/
SCHEDULE_FSM *schedule_fsm_create(SCHEDULE *s,	/**< the schedule that govern this machine */
						MACHINETYPE t,	/**< the type of machine (see #MACHINETYPE) */
						...)			/**< the machine specification (depends on #MACHINETYPE) */
{
	va_list arg;
	SCHEDULE_FSM *machine = malloc(sizeof(SCHEDULE_FSM));
	if (machine==NULL)
	{
		output_error("schedule_fsm_create(SCHEDULE *s={name: %s; ...}, ...): memory allocation error", s->name);
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
			output_error("schedule_fsm_create(SCHEDULE *s={name: %s; ...}, MACHINETYPE t=MT_ANALOG, double energy=%g): scheduled energy is negative", s->name, machine->params.analog.energy);
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
			output_error("schedule_fsm_create(SCHEDULE *s={name: %s; ...}, MACHINETYPE t=MT_PULSED, double energy=%g): scheduled energy is negative", s->name, machine->params.analog.energy);
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
			output_error("schedule_fsm_create(SCHEDULE *s={name: %s; ...}, MACHINETYPE t=MT_MODULATED, double energy=%g): scheduled energy is negative", s->name, machine->params.analog.energy);
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
			output_error("schedule_fsm_create(SCHEDULE *s={name: %s; ...}, MACHINETYPE t=MT_QUEUED, double energy=%g): scheduled energy is negative", s->name, machine->params.analog.energy);
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
		output_error("schedule_fsm_create(SCHEDULE *s={name: %s; ...}, MACHINETYPE t=%d, ...): memory allocation error", s->name);
		/* TROUBLESHOOT
			The machine type is not defined.  Specify a defined machine type (see MACHINETYPE) and try again.
		 */
		goto Error;
	}
	va_end(arg);
	return machine;
Error:
	free(machine);
	return NULL;
}

int schedule_fsm_init(SCHEDULE_FSM *m, /**< machine */
					  complex *v, /**< reference voltage to use for power calcs */
					  complex *zip0, /**< analog: peak end-use ZIP fractions; others: ZIP values when machine is off */
					  complex *zip1) /**< ZIP values when machine is on, NULL for analog machines */
{
	m->zip[0][0] = &zip0[0];
	m->zip[0][1] = &zip0[1];
	m->zip[0][2] = &zip0[2];
	if (m->type!=MT_ANALOG)
	{
		m->zip[1][0] = &zip1[0];
		m->zip[1][1] = &zip1[1];
		m->zip[1][2] = &zip1[2];
	}
}

TIMESTAMP schedule_fsm_sync(SCHEDULE_FSM *m, TIMESTAMP t1)
{
	if (t1>m->t0)
	{
		double dt = (double)(t1-m->t0);
		double energy, value, interval;
		if (m->type == MT_ANALOG)
		{
			energy = m->params.analog.energy;
			value = m->schedule->value;
			interval = m->schedule->duration;
			m->s = 0; /* always in state 0 */
		}
		else
		{
			switch (m->s) {
			case 0:
				// TODO
				break;
			case 1:
				// TODO
				break;
			default:
				break;
			}
			m->q += m->r[m->s]*dt;
		}

		/* if reference voltage is given */
		if (m->v!=NULL)
		{
			/* power is sensitive to voltage */
			complex v = *(m->v);

			/* power = energy/interval x value x ZIPfractions */
			// TODO: can't do this until complex is supported in C
			//m->power = energy/interval * value * ((v/(m->zip[m->s][0]) + m->zip[m->s][1])/(~v) + m->zip[m->s][2]);
			throw_exception("complex ops not supported in C yet");
		}
		else
		{
			/* power = energy/interval x value x nominal_power (ignores constant impedance and current portions)*/
			// TODO: can't do this until complex is supported in C
			//m->power = energy/interval * value * ((v/(m->zip[m->s][0]) + m->zip[m->s][1])/(~v) + m->zip[m->s][2]);
			//m->power = energy/interval * value * m->zip[m->s][2];
		}
	}
}