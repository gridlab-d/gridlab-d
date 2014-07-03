/* transform.h
 */

#ifndef _TRANSFORM_H
#define _TRANSFORM_H

#include "schedule.h"

/* data structure to bind a variable with a property definition */
typedef struct s_variable {
	void *addr;
	PROPERTY *prop;
} GLDVAR; 

/* prototype of external transform function */
typedef int (*TRANSFORMFUNCTION)(int,GLDVAR*,int,GLDVAR*);

/* list of supported transform sources */
typedef enum {
	XS_UNKNOWN	= 0x00, 
	XS_DOUBLE	= 0x01, 
	XS_COMPLEX	= 0x02, 
	XS_LOADSHAPE= 0x04, 
	XS_ENDUSE	= 0x08, 
	XS_SCHEDULE = 0x10,
	XS_ALL		= 0x1f,
} TRANSFORMSOURCE;

/* list of supported transform function types */
typedef enum {
	XT_LINEAR	= 0x00,
	XT_EXTERNAL = 0x01,
} TRANSFORMFUNCTIONTYPE;

/* transform data structure */
typedef struct s_transform {
	double *source;	///< source vector of the function input
	TRANSFORMSOURCE source_type; ///< data type of source
	struct s_object_list *target_obj; ///< object of the target
	struct s_property_map *target_prop; ///< property of the target
	TRANSFORMFUNCTIONTYPE function_type; ///< function type (linear, external, etc.)
	union {
		struct { // used only by linear transforms
			void *source_addr; ///< pointer to the source
			SCHEDULE *source_schedule; ///< schedule associated with the source
			double *target; ///< target of the function output
			double scale; ///< scalar (linear transform only)
			double bias; ///< constant (linear transform only)
		};
		struct { // used only by external transforms
			TRANSFORMFUNCTION function; /// function pointer
			int retval; /// last return value
			int nlhs; /// number of lhs values
			GLDVAR *plhs; /// vector of lhs value pointers
			int nrhs; /// number of rhs values
			GLDVAR *prhs; /// vector of rhs value pointers
		};
	};
	struct s_transform *next; ///* next item in linked list
} TRANSFORM;

#ifdef __cplusplus
extern "C" {
#endif

int transform_add_external(struct s_object_list *target_obj, struct s_property_map *target_prop, char *function, struct s_object_list *source_obj, struct s_property_map *source_prop);
int transform_add_linear(TRANSFORMSOURCE stype, double *source, void *target, double scale, double bias, struct s_object_list *obj, struct s_property_map *prop, SCHEDULE *s);
TRANSFORM *transform_getnext(TRANSFORM *xform);
TIMESTAMP transform_syncall(TIMESTAMP t, TRANSFORMSOURCE source);

GLDVAR *gldvar_create(unsigned int dim);
int gldvar_isset(GLDVAR *var, unsigned int n);
void gldvar_set(GLDVAR *var, unsigned int n, void *addr, PROPERTY *prop);
void gldvar_unset(GLDVAR *var, unsigned int n);
void *gldvar_getaddr(GLDVAR *var, unsigned int n);
void *gldvar_getprop(GLDVAR *var, unsigned int n);
PROPERTYTYPE gldvar_gettype(GLDVAR *var, unsigned int n);
char *gldvar_getname(GLDVAR *var, unsigned int n);
char *gldvar_getstring(GLDVAR *var, unsigned int n, char *buffer, int size);
UNIT *gldvar_getunits(GLDVAR *var, unsigned int n);

#ifdef __cplusplus
}
#endif

#endif
