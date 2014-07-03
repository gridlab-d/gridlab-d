
#ifndef _ENDUSE_H
#define _ENDUSE_H

#include "class.h"
#include "object.h"
#include "timestamp.h"
#include "loadshape.h"

#define EUC_IS220 0x0001 ///< enduse flag to indicate that the voltage is line-to-line, not line-to-neutral
#define EUC_HEATLOAD 0x0002 ///< enduse flag to indicate that the load drives the heatgain instead of the total power

typedef enum {
	EUMT_MOTOR_A, /**< 3ph induction motors driving constant torque loads */
	EUMT_MOTOR_B, /**< induction motors driving high inertia speed-squares torque loads */
	EUMT_MOTOR_C, /**< induction motors driving low inertia loads speed-squared torque loads */
	EUMT_MOTOR_D, /**< 1ph induction motors driving constant torque loads */
	_EUMT_COUNT, /* must be last */
} EUMOTORTYPE;
typedef enum {
	EUET_ELECTRONIC_A, /**< simple power electronics (no backfeed) */
	EUET_ELECTRONIC_B, /**< advanced power electronics (w/ backfeed) */
	_EUET_COUNT, /* must be last */
} EUELECTRONICTYPE;

typedef struct s_motor {
	complex power;		/**< motor power when running */
	complex impedance;	/**< motor impedance when stalled */
	double inertia;		/**< motor inertia in seconds */
	double v_stall;		/**< motor stall voltage (pu) */
	double v_start;		/**< motor start voltage (pu) */
	double v_trip;		/**< motor trip voltage (pu) */
	double t_trip;		/**< motor thermal trip time in seconds */
	/* TODO add slip data (0 for synchronous motors) */
} EUMOTOR;
typedef struct s_electronic {
	complex power;		/**< load power when running */
	double inertia;		/**< load "inertia" */
	double v_trip;		/**< load "trip" voltage (pu) */
	double v_start;		/**< load "start" voltage (pu) */
} EUELECTRONIC;

typedef struct s_enduse {
	/* the output value must be first for transform to stream */
	/* meter values */
	complex total;				/* total power in kW */
	complex energy;				/* total energy in kWh */
	complex demand;				/* maximum power in kW (can be reset) */

	/* circuit configuration */	
	set config;					/* end-use configuration */
	double breaker_amps;		/* breaker limit (if any) */

	/* zip values */
	complex admittance;			/* constant impedance oprtion of load in kW */
	complex current;			/* constant current portion of load in kW */
	complex power;				/* constant power portion of load in kW */

	/* composite load data */
	EUMOTOR motor[_EUMT_COUNT];				/* motor loads (A-D) */
	EUELECTRONIC electronic[_EUET_COUNT];	/* electronic loads (S/D) */

	/* loading */
	double impedance_fraction;	/* constant impedance fraction (pu load) */
	double current_fraction;	/* constant current fraction (pu load) */
	double power_fraction;		/* constant power fraction (pu load)*/
	double power_factor;		/* power factor */
	double voltage_factor;		/* voltage factor (pu nominal) */

	/* heat */
	double heatgain;			/* internal heat from load (Btu/h) */
	double cumulative_heatgain;  /* internal cumulative heat gain from load (Btu) */ 
	double heatgain_fraction;	/* fraction of power that goes to internal heat (pu Btu/h) */

	/* misc info */
	char *name;
	loadshape *shape;
	TIMESTAMP t_last;			/* last time of update */

	// added for backward compatibility with res ENDUSELOAD
	// @todo these are obsolete and must be retrofitted with the above values
	struct s_object_list *end_obj;

	struct s_enduse *next;
#ifdef _DEBUG
	unsigned int magic;
#endif
} enduse;

int enduse_create(enduse *addr);
int enduse_init(enduse *e);
int enduse_initall(void);
TIMESTAMP enduse_sync(enduse *e, PASSCONFIG pass, TIMESTAMP t1);
TIMESTAMP enduse_syncall(TIMESTAMP t1);
int convert_to_enduse(char *string, void *data, PROPERTY *prop);
int convert_from_enduse(char *string,int size,void *data, PROPERTY *prop);
int enduse_publish(CLASS *oclass, PROPERTYADDR struct_address, char *prefix);
int enduse_test(void);

double enduse_get_part(void *c, char *name);

#endif
