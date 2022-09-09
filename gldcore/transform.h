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
	XS_RANDOMVAR = 0x20,
	XS_ALL		= 0x1f,
} TRANSFORMSOURCE;

/* list of supported transform function types */
typedef enum {
	XT_LINEAR	= 0x00,
	XT_EXTERNAL = 0x01,
//	XT_DIFF		= 0x02, ///< transform is a finite difference
//	XT_SUM		= 0x03, ///< transform is a discrete sum
	XT_FILTER	= 0x04, ///< transform is a discrete-time filter
} TRANSFORMFUNCTIONTYPE;

/****************************************************************
 * Transfer function implementation
 ****************************************************************/

typedef struct s_transferfunction {
	char name[64];		///< transfer function name
	char domain[4];		///< domain variable name
	double timestep;	///< timestep (seconds)
	double timeskew;	///< timeskew (seconds)
	unsigned int n;		///< denominator order
	double *a;			///< denominator coefficients
	unsigned int m;		///< numerator order
	double *b;			///< numerator coefficients
	struct s_transferfunction *next;
} TRANSFERFUNCTION;

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
		struct { // used only by filter transforms
			TRANSFERFUNCTION *tf; ///< transfer function
			double *u; ///< u vector
			double *y; ///< y vector
			double *x; ///< x vector
			TIMESTAMP t2; ///< next sample time
			double t2_dbl;	///< next sample time for deltamode simulations
		};
	};
	struct s_transform *next; ///* next item in linked list
} TRANSFORM;

#ifdef __cplusplus
extern "C" {
#endif

int transform_add_external(struct s_object_list *target_obj, struct s_property_map *target_prop, const char *function, struct s_object_list *source_obj, struct s_property_map *source_prop);
int transform_add_linear(TRANSFORMSOURCE stype, double *source, void *target, double scale, double bias, struct s_object_list *obj, struct s_property_map *prop, SCHEDULE *s);
TRANSFORM *transform_getnext(TRANSFORM *xform);
TIMESTAMP transform_syncall(TIMESTAMP t, TRANSFORMSOURCE source, double *t1_dbl);
int64 transform_apply(TIMESTAMP t1, TRANSFORM *xform, double *source, double *dm_time);

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

int transform_add_filter(struct s_object_list *target_obj, struct s_property_map *target_prop, char *function, struct s_object_list *source_obj, struct s_property_map *source_prop);
int transfer_function_add(char *tfname, char *domain, double timestep, double timeskew, unsigned int n, double *a, unsigned int m, double *b);

int transform_saveall(FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
