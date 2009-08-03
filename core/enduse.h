
#ifndef _ENDUSE_H
#define _ENDUSE_H

#include "class.h"
#include "object.h"
#include "timestamp.h"
#include "loadshape.h"

#define EUC_IS220 0x0001 ///< enduse flag to indicate that the voltage is line-to-line, not line-to-neutral

typedef struct s_enduse {
	char *name;
	loadshape *shape;
	set config;					/* end-use configuration */
	complex total;				/* total power in kW */
	complex energy;				/* total energy in kWh */
	complex demand;				/* maximum power in kW (can be reset) */
	complex admittance;			/* constant impedance oprtion of load in kW */
	complex current;			/* constant current portion of load in kW */
	complex power;				/* constant power portion of load in kW */
	double impedance_fraction;	/* constant impedance fraction (pu load) */
	double current_fraction;	/* constant current fraction (pu load) */
	double power_fraction;		/* constant power fraction (pu load)*/
	double power_factor;		/* power factor */
	double voltage_factor;		/* voltage factor (pu nominal) */
	double heatgain;			/* internal heat from load (Btu/h) */
	double heatgain_fraction;	/* fraction of power that goes to internal heat (pu Btu/h) */
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
