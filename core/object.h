/** $Id: object.h 1207 2009-01-12 22:47:29Z d3p988 $
	Copyright (C) 2008 Battelle Memorial Institute	@file object.h
	@addtogroup object
 @{
 **/

#ifndef _OBJECT_H
#define _OBJECT_H

#include "complex.h"
#include "timestamp.h"
#include "class.h"
#include "aggregate.h"
#include "exception.h"
#include "globals.h"
#include "random.h"
#include "threadpool.h"

/* this must match property_type list in object.c */
typedef unsigned int OBJECTRANK; /**< Object rank number */
typedef unsigned short OBJECTSIZE; /** Object data size */
typedef unsigned int OBJECTNUM; /** Object id number */
typedef char * OBJECTNAME; /** Object name */

/* object flags */
#define OF_NONE		0x0000 /**< Object flag; none set */
#define OF_HASPLC	0x0001 /**< Object flag; external PLC is attached, disables local PLC */
#define OF_LOCKED	0x0002 /**< Object flag; data write pending, reread recommended after lock clears */
#define OF_RECALC	0x0008 /**< Object flag; recalculation of derived values is needed */
#define OF_FOREIGN	0x0010 /**< Object flag; indicates that object was created in a DLL and memory cannot be freed by core */
#define OF_SKIPSAFE	0x0020 /**< Object flag; indicates that skipping updates is safe */
#define OF_RERANK	0x4000 /**< Internal use only */

typedef struct s_object_list {
	OBJECTNUM id; /**< object id number; globally unique */
	CLASS *oclass; /**< object class; determine structure of object data */
	struct s_object_list *next; /**< next object in list */
	struct s_object_list *parent; /**< object's parent; determines rank */
	OBJECTRANK rank; /**< object's rank */
	TIMESTAMP clock; /**< object's private clock */
	TIMESTAMP valid_to;	/**< object's valid-until time */
	double latitude, longitude; /**< object's geo-coordinates */
	TIMESTAMP in_svc, /**< time at which object begin's operating */
		out_svc; /**< time at which object ceases operating */
	OBJECTNAME name;
	int tp_affinity; /**< threadpool processor affinity */
	unsigned long flags; /**< object flags */
	/* IMPORTANT: flags must be last */
} OBJECT; /**< Object header structure */

/* this is the callback table for modules
 * the table is initialized in module.cpp
 */
typedef struct s_callbacks {
	TIMESTAMP *global_clock;
	int (*output_verbose)(char *format, ...);
	int (*output_message)(char *format, ...);
	int (*output_warning)(char *format, ...);
	int (*output_error)(char *format, ...);
	int (*output_debug)(char *format, ...);
	int (*output_test)(char *format, ...);
	CLASS *(*register_class)(MODULE *,CLASSNAME,unsigned int,PASSCONFIG);
	struct {
		OBJECT *(*single)(CLASS*);
		OBJECT *(*array)(CLASS*,unsigned int);
		OBJECT *(*foreign)(OBJECT *);
	} create;
	int (*define_map)(CLASS*,...);
	CLASS *(*class_getname)(char*);
	CLASS *(*class_getobjnum)(OBJECTTYPE);
	struct {
		FUNCTION *(*define)(CLASS*,FUNCTIONNAME,FUNCTIONADDR);
		FUNCTIONADDR (*get)(char*,char*);
	} function;
	int (*define_enumeration_member)(CLASS*,char*,char*,enumeration);
	int (*define_set_member)(CLASS*,char*,char*,unsigned long);
	int (*set_dependent)(OBJECT*,OBJECT*);
	int (*set_parent)(OBJECT*,OBJECT*);
	int (*set_rank)(OBJECT*,unsigned int);
	struct {
		PROPERTY *(*get_property)(OBJECT*,PROPERTYNAME);
		int (*set_value_by_addr)(OBJECT *, void*, char*,PROPERTY*);
		int (*get_value_by_addr)(OBJECT *, void*, char*, int size,PROPERTY*);
		int (*set_value_by_name)(OBJECT *, char*, char*);
		int (*get_value_by_name)(OBJECT *, char*, char*, int size);
		OBJECT *(*get_reference)(OBJECT *, char*);
		char *(*get_unit)(OBJECT *, char *);
		void *(*get_addr)(OBJECT *, char *);
	} properties;
	struct {
		struct s_findlist *(*objects)(struct s_findlist *,...);
		OBJECT *(*next)(struct s_findlist *,OBJECT *obj);
		struct s_findlist *(*copy)(struct s_findlist *);
		void (*add)(struct s_findlist*, OBJECT*);
		void (*del)(struct s_findlist*, OBJECT*);
		void (*clear)(struct s_findlist*);
	} find;
	PROPERTY *(*find_property)(CLASS *, PROPERTYNAME);
	void *(*malloc)(size_t);
	void (*free)(void*);
	struct s_aggregate *(*create_aggregate)(char *aggregator, char *group_expression);
	//struct s_aggregate *(*create_aggregate_zwei)(char *aggregator, struct s_aggregate *aggr);
	double (*run_aggregate)(struct s_aggregate *aggregate);
	void *(*get_module_var)(MODULE *module, char *varname);
	int (*depends)(char *name, unsigned char major, unsigned char minor, unsigned short build);
	struct {
		double (*uniform)(double a, double b);
		double (*normal)(double m, double s);
		double (*bernoulli)(double p);
		double (*pareto)(double m, double a);
		double (*lognormal)(double m, double s);
		double (*sampled)(unsigned int n, double *x);
		double (*exponential)(double l);
		RANDOMTYPE (*type)(char *name);
		double (*value)(RANDOMTYPE type, ...);
		double (*pseudo)(RANDOMTYPE type, unsigned int *state, ...);
	} random;
	int (*object_isa)(OBJECT *obj, char *type);
	DELEGATEDTYPE* (*register_type)(CLASS *oclass, char *type,int (*from_string)(void*,char*),int (*to_string)(void*,char*,int));
	int (*define_type)(CLASS*,DELEGATEDTYPE*,...);
	struct {
		TIMESTAMP (*mkdatetime)(DATETIME *dt);
		int (*strdatetime)(DATETIME *t, char *buffer, int size);
		double (*timestamp_to_days)(TIMESTAMP t);
		double (*timestamp_to_hours)(TIMESTAMP t);
		double (*timestamp_to_minutes)(TIMESTAMP t);
		double (*timestamp_to_seconds)(TIMESTAMP t);
		int (*local_datetime)(TIMESTAMP ts, DATETIME *dt);
		TIMESTAMP (*convert_to_timestamp)(char *value);
	} time;
	int (*unit_convert)(char *from, char *to, double *value);
	int (*unit_convert_ex)(UNIT *pFrom, UNIT *pTo, double *pValue);
	UNIT *(*unit_find)(char *unit_name);
	struct {
		EXCEPTIONHANDLER *(*create_exception_handler)();
		void (*delete_exception_handler)(EXCEPTIONHANDLER *ptr);
		void (*throw_exception)(char *msg, ...);
		char *(*exception_msg)(void);
	} exception;
	struct {
		GLOBALVAR *(*create)(char *name, ...);
		STATUS (*setvar)(char *def,...);
		char *(*getvar)(char *name, char *buffer, int size);
		GLOBALVAR *(*find)(char *name);
	} global;
#ifndef NOLOCKS
	int64 *lock_count, *lock_spin;
#endif
	struct {
		char *(*find_file)(char *name, char *path, int mode);
	} file;
	struct s_objvar_struct {
		complex *(*complex_var)(OBJECT *obj, PROPERTY *prop);
		int16 *(*int16_var)(OBJECT *obj, PROPERTY *prop);
		int32 *(*int32_var)(OBJECT *obj, PROPERTY *prop);
		int64 *(*int64_var)(OBJECT *obj, PROPERTY *prop);
		double *(*double_var)(OBJECT *obj, PROPERTY *prop);
		char *(*string_var)(OBJECT *obj, PROPERTY *prop);
		OBJECT *(*object_var)(OBJECT *obj, PROPERTY *prop);
	} objvar;
	struct s_objvar_name_struct {
		complex *(*complex_var)(OBJECT *obj, char *name);
		int16 *(*int16_var)(OBJECT *obj, char *name);
		int32 *(*int32_var)(OBJECT *obj, char *name);
		int64 *(*int64_var)(OBJECT *obj, char *name);
		double *(*double_var)(OBJECT *obj, char *name);
		char *(*string_var)(OBJECT *obj, char *name);
	} objvarname;
	struct {
		int (*string_to_property)(PROPERTY *prop, void *addr, char *value);
		int (*property_to_string)(PROPERTY *prop, void *addr, char *value, int size);
	} convert;
	MODULE *(*module_find)(char *name);
	OBJECT *(*get_object)(char *name);
	int (*get_oflags)(KEYWORD **extflags);
	unsigned int (*object_count)(void);
} CALLBACKS; /**< core callback function table */

#ifdef __cplusplus
extern "C" {
#endif

OBJECT *object_create_single(CLASS *oclass);
OBJECT *object_create_array(CLASS *oclass, unsigned int n_objects);
OBJECT *object_create_foreign(OBJECT *obj);
OBJECT *object_remove_by_id(OBJECTNUM id);
int object_init(OBJECT *obj);
int object_set_dependent(OBJECT *obj, OBJECT *dependent);
int object_set_parent(OBJECT *obj, OBJECT *parent);
void *object_get_addr(OBJECT *obj, char *name);
PROPERTY *object_get_property(OBJECT *obj, PROPERTYNAME name);
int object_set_value_by_name(OBJECT *obj, PROPERTYNAME name, char *value);
int object_set_value_by_addr(OBJECT *obj, void *addr, char *value, PROPERTY *prop);
int object_set_double_by_name(OBJECT *obj, PROPERTYNAME name, double value);
int object_set_complex_by_name(OBJECT *obj, PROPERTYNAME name, complex value);
int object_get_value_by_name(OBJECT *obj, PROPERTYNAME name, char *value, int size);
int object_get_value_by_addr(OBJECT *obj, void *addr, char *value, int size, PROPERTY *prop);
OBJECT *object_get_reference(OBJECT *obj, char *name);
int object_isa(OBJECT *obj, char *type);
OBJECTNAME object_set_name(OBJECT *obj, OBJECTNAME name);
OBJECT *object_find_name(OBJECTNAME name);

int object_get_oflags(KEYWORD **extflags);

TIMESTAMP object_sync(OBJECT *obj, TIMESTAMP to,PASSCONFIG pass);
PROPERTY *object_get_property(OBJECT *obj, PROPERTYNAME name);
OBJECT *object_get_object(OBJECT *obj, PROPERTY *prop);
int16 *object_get_int16(OBJECT *obj, PROPERTY *prop);
int16 *object_get_int16_by_name(OBJECT *obj, char *name);
int32 *object_get_int32(OBJECT *obj, PROPERTY *prop);
int32 *object_get_int32_by_name(OBJECT *obj, char *name);
int64 *object_get_int64(OBJECT *obj, PROPERTY *prop);
int64 *object_get_int64_by_name(OBJECT *obj, char *name);
double *object_get_double(OBJECT *pObj, PROPERTY *prop);
double *object_get_double_by_name(OBJECT *pObj, char *name);
complex *object_get_complex(OBJECT *pObj, PROPERTY *prop);
complex *object_get_complex_by_name(OBJECT *pObj, char *name);
double *object_get_double_quick(OBJECT *pObj, PROPERTY *prop);
complex *object_get_complex_quick(OBJECT *pObj, PROPERTY *prop);
char *object_get_string(OBJECT *pObj, PROPERTY *prop);
char *object_get_string_by_name(OBJECT *obj, char *name);
FUNCTIONADDR object_get_function(CLASSNAME classname, FUNCTIONNAME functionname);
char *object_property_to_string(OBJECT *obj, char *name);
char *object_get_unit(OBJECT *obj, char *name);
int object_set_rank(OBJECT *obj, OBJECTRANK rank);

OBJECT *object_find_by_id(OBJECTNUM id);
OBJECT *object_get_first();
OBJECT *object_get_next(OBJECT *obj);
unsigned int object_get_count(void);
int object_dump(char *buffer, int size, OBJECT *obj);
int object_saveall(FILE *fp);
int object_saveall_xml(FILE *fp);

char *object_name(OBJECT *obj);
int convert_from_latitude(double,void*,int);
int convert_from_longitude(double,void*,int);
double convert_to_latitude(char *buffer);
double convert_to_longitude(char *buffer);

PROPERTY *object_flag_property(void);
PROPERTY *object_access_property(void);

#ifdef __cplusplus
}
#endif


#define object_size(X) ((X)?(X)->size:-1) /**< get the size of the object X */
#define object_id(X) ((X)?(X)->id:-1) /**< get the id of the object X */
#define object_parent(X) ((X)?(X)->parent:NULL) /**< get the parent of the object */
#define object_rank(X) ((X)?(X)->name:-1) /**< get the rank of the object */

#define OBJECTDATA(X,T) ((T*)((X)?((X)+1):NULL)) /**< get the object data structure */
#define GETADDR(O,P) (O?((void*)((char*)(O+1)+(unsigned int64)((P)->addr))):NULL) /**< get the addr of an object's property */
#define OBJECTHDR(X) ((X)?(((OBJECT*)X)-1):NULL) /**< get the header from the object's data structure */
#define MYPARENT ((((OBJECT*)this)-1)->parent) /**< get the parent from the object's data structure */

#endif

/** @} **/
