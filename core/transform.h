/* transform.h
 */

#ifndef _TRANSFORM_H
#define _TRANSFORM_H

#include "schedule.h"

typedef enum {
	XS_UNKNOWN	= 0x00, 
	XS_DOUBLE	= 0x01, 
	XS_COMPLEX	= 0x02, 
	XS_LOADSHAPE= 0x04, 
	XS_ENDUSE	= 0x08, 
	XS_SCHEDULE = 0x10,
	XS_ALL		= 0x1f,
	} TRANSFORMSOURCE;
typedef struct s_transform {
	double *source;
	TRANSFORMSOURCE source_type;
	void *source_addr;
	SCHEDULE *source_schedule;
	double *target;
	struct s_object_list *target_obj;
	struct s_property_map *target_prop;
	double scale;
	double bias;
	struct s_transform *next;
} TRANSFORM;

int transform_add(TRANSFORMSOURCE stype, double *source, double *target, double scale, double bias, struct s_object_list *obj, struct s_property_map *prop, SCHEDULE *s);
TRANSFORM *transform_getnext(TRANSFORM *xform);
TIMESTAMP transform_syncall(TIMESTAMP t, TRANSFORMSOURCE restrict);

#endif
