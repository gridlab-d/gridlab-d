/* transform.h
 */

#ifndef _TRANSFORM_H
#define _TRANSFORM_H

#include "schedule.h"

typedef int (*TRANSFORMFUNCTION)(int,void*,int,void*);
typedef enum {
	XS_UNKNOWN	= 0x00, 
	XS_DOUBLE	= 0x01, 
	XS_COMPLEX	= 0x02, 
	XS_LOADSHAPE= 0x04, 
	XS_ENDUSE	= 0x08, 
	XS_SCHEDULE = 0x10,
	XS_ALL		= 0x1f,
	} TRANSFORMSOURCE;
typedef enum {
	XT_LINEAR	= 0x00,
	XT_EXTERNAL = 0x01,
} TRANSFORMFUNCTIONTYPE;
typedef struct s_transform {
	double *source;	///< source vector of the function input
	TRANSFORMSOURCE source_type; ///< data type of source
	void *source_addr; ///< pointer to the source
	SCHEDULE *source_schedule; ///< schedule associated with the source
	double *target; ///< target of the function output
	struct s_object_list *target_obj; ///< object of the target
	struct s_property_map *target_prop; ///< property of the target
	TRANSFORMFUNCTIONTYPE function_type; ///< function type (linear, external, etc.)
	union {
		struct {
			double scale; ///< scalar (linear transform only)
			double bias; ///< constant (linear transform only)
		};
		struct {
			TRANSFORMFUNCTION function; /// function pointer
			int retval; /// last return value
			int nlhs; /// number of lhs values
			void *plhs; /// vector of lhs value pointers
			int nrhs; /// number of rhs values
			void *prhs; /// vector of rhs value pointers
		};
	};
	struct s_transform *next; ///* next item in linked list
} TRANSFORM;

int transform_add_function(TRANSFORMSOURCE stype, double *source, double *target, char *function, struct s_object_list *obj, struct s_property_map *prop, SCHEDULE *s);
int transform_add(TRANSFORMSOURCE stype, double *source, double *target, double scale, double bias, struct s_object_list *obj, struct s_property_map *prop, SCHEDULE *s);
TRANSFORM *transform_getnext(TRANSFORM *xform);
TIMESTAMP transform_syncall(TIMESTAMP t, TRANSFORMSOURCE restrict);

#endif
