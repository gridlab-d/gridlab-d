
#ifndef _ENDUSE_H
#define _ENDUSE_H

#include "class.h"
#include "object.h"
#include "timestamp.h"
#include "loadshape.h"

#define EUC_IS220 0x0001 ///< enduse flag to indicate that the voltage is line-to-line, not line-to-neutral
#define EUC_HEATLOAD 0x0002 ///< enduse flag to indicate that the load drives the heatgain instead of the total power

typedef struct s_enduse {
	char *name;
	loadshape *shape;
#ifndef __cplusplus
	struct {
#endif
		set config;					/* end-use configuration */
		double breaker_amps;		/* breaker limit (if any) */
#ifndef __cplusplus
	}; /* circuit configuration */
	struct {
#endif
		complex total;				/* total power in kW */
		complex energy;				/* total energy in kWh */
		complex demand;				/* maximum power in kW (can be reset) */
#ifndef __cplusplus
	}; /* meter values */
	struct {
#endif
		complex admittance;			/* constant impedance oprtion of load in kW */
		complex current;			/* constant current portion of load in kW */
		complex power;				/* constant power portion of load in kW */
#ifndef __cplusplus
	}; /* zip values */
	struct {
#endif
		double impedance_fraction;	/* constant impedance fraction (pu load) */
		double current_fraction;	/* constant current fraction (pu load) */
		double power_fraction;		/* constant power fraction (pu load)*/
		double power_factor;		/* power factor */
		double voltage_factor;		/* voltage factor (pu nominal) */
#ifndef __cplusplus
	}; /* loading */
	struct {
#endif
		double heatgain;			/* internal heat from load (Btu/h) */
		double heatgain_fraction;	/* fraction of power that goes to internal heat (pu Btu/h) */
#ifndef __cplusplus
	}; /* heat */
#endif
	TIMESTAMP t_last;			/* last time of update */

	// added for backward compatibility with res ENDUSELOAD
	// @todo these are obsolete and must be retrofitted with the above values
	struct s_object_list *end_obj;

	struct s_enduse *next;
} enduse;

int enduse_create(void *addr);
int enduse_init(enduse *e);
int enduse_initall(void);
TIMESTAMP enduse_sync(enduse *e, PASSCONFIG pass, TIMESTAMP t1);
TIMESTAMP enduse_syncall(TIMESTAMP t1);
int convert_to_enduse(char *string, void *data, PROPERTY *prop);
int convert_from_enduse(char *string,int size,void *data, PROPERTY *prop);
int enduse_publish(CLASS *oclass, PROPERTYADDR struct_address, char *prefix);
int enduse_test(void);

#endif
