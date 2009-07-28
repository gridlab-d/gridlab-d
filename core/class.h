/** $Id: class.h 1207 2009-01-12 22:47:29Z d3p988 $
	Copyright (C) 2008 Battelle Memorial Institute
	@file class.h
	@addtogroup class
 @{
 **/

#ifndef _CLASS_H
#define _CLASS_H

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "platform.h"
#include "complex.h"
#include "unit.h"

typedef struct s_class_list CLASS;

/* Valid GridLAB data types */
typedef char char1024[1025]; /**< strings up to 1024 characters */
typedef char char256[257]; /**< strings up to 256 characters */
typedef char char32[33]; /**< strings up to 32 characters */
typedef char char8[9]; /** string up to 8 characters */
typedef char int8; /** 8-bit integers */
typedef short int16; /** 16-bit integers */
typedef int int32; /* 32-bit integers */
typedef long enumeration; /* enumerations (any one of a list of values) */
typedef struct s_object_list* object; /* GridLAB objects */
typedef unsigned int64 set; /* sets (each of up to 64 values may be defined) */
typedef double triplet[3];
typedef complex triplex[3];
#ifdef REAL4
typedef float real; 
#else
typedef double real;
#endif

#ifndef __cplusplus
#ifndef true
typedef unsigned char bool;
#define true (1)
#define false (0)
#endif
#endif


/* delegated types allow module to keep all type operations private
 * this includes convert operations and allocation/deallocation
 */
typedef struct s_delegatedtype
{
	char32 type; /**< the name of the delegated type */
	CLASS *oclass; /**< the class implementing the delegated type */
	int (*from_string)(void *addr, char *value); /**< the function that converts from a string to the data */
	int (*to_string)(void *addr, char *value, int size); /**< the function that converts from the data to a string */
} DELEGATEDTYPE; /**< type delegation specification */
typedef struct s_delegatedvalue
{
	char *data; /**< the data that is delegated */
	DELEGATEDTYPE *type; /**< the delegation specification to use */
} DELEGATEDVALUE; /**< a delegation entry */
typedef DELEGATEDVALUE* delegated; /* delegated data type */

/* int64 is already defined in platform.h */
typedef enum {_PT_FIRST=-1, 
	PT_void, /**< the type has no data */
	PT_double, /**< the data is a double-precision float */
	PT_complex, /**< the data is a complex value */
	PT_enumeration, /**< the data is an enumeration */
	PT_set, /**< the data is a set */
	PT_int16, /**< the data is a 16-bit integer */
	PT_int32, /**< the data is a 32-bit integer */
	PT_int64, /**< the data is a 64-bit integer */
	PT_char8, /**< the data is \p NULL -terminated string up to 8 characters in length */
	PT_char32, /**< the data is \p NULL -terminated string up to 32 characters in length */ 
	PT_char256, /**< the data is \p NULL -terminated string up to 256 characters in length */
	PT_char1024, /**< the data is \p NULL -terminated string up to 1024 characters in length */
	PT_object, /**< the data is a pointer to a GridLAB object */
	PT_delegated, /**< the data is delegated to a module for implementation */
	PT_bool, /**< the data is a true/false value, implemented as a C++ bool */
	PT_timestamp, /**< timestamp value */
	PT_double_array, /**< the data is a fixed length double[] */
	PT_complex_array, /**< the data is a fixed length complex[] */
/*	PT_object_array, */ /**< the data is a fixed length array of object pointers*/
	PT_float,	/**< Single-precision float	*/
	PT_real,	/**< Single or double precision float ~ allows double values to be overriden */
	PT_loadshape,	/**< Loadshapes are state machines driven by schedules */
	PT_enduse,		/**< Enduse load data */
#ifdef USE_TRIPLETS
	PT_triple, /**< triplet of doubles (not supported) */
	PT_triplex, /**< triplet of complexes (not supported) */
#endif
	_PT_LAST, 
	/* never put these before _PT_LAST they have special uses */
	PT_AGGREGATE, /* internal use only */
	PT_KEYWORD, /* used to add an enum/set keyword definition */
	PT_ACCESS, /* used to specify property access rights */
	PT_SIZE, /* used to setup arrayed properties */
	PT_FLAGS, /* used to indicate property flags next */
	PT_INHERIT, /* used to indicate that properties from a parent class are to be published */
	PT_UNITS, /* used to indicate that property has certain units (which following immediately as a string) */
	PT_DESCRIPTION, /* used to provide helpful description of property */
	PT_EXTEND, /* used to enlarge class size by the size of the current property being mapped */
	PT_EXTENDBY, /* used to enlarge class size by the size provided in the next argument */
	PT_DEPRECATED, /* used to flag a property that is deprecated */
} PROPERTYTYPE; /**< property types */
typedef char CLASSNAME[64]; /**< the name a GridLAB class */
typedef void* PROPERTYADDR; /**< the offset of a property from the end of the OBJECT header */
typedef char PROPERTYNAME[64]; /**< the name of a property */
typedef char FUNCTIONNAME[64]; /**< the name of a function (not used) */
typedef enum {
	PA_PUBLIC, /**< property is public (visible, saved, and loaded) */
	PA_REFERENCE, /**< property is FYI (visible and saved, but not loaded */
	PA_PROTECTED, /**< property is semipublic (visible, but not saved or loaded) */
	PA_PRIVATE, /**< property is nonpublic (not-visible, saved or loaded) */
} PROPERTYACCESS; /**< property access rights */
typedef int64 (*FUNCTIONADDR)(void*,...); /** the entry point of a module function */

/* pass configuration */
typedef unsigned char PASSCONFIG; /**< the pass configuration */
#define PC_NOSYNC 0x00					/**< used when the class requires no synchronization */
#define PC_PRETOPDOWN 0x01				/**< used when the class requires synchronization on the first top-down pass */
#define PC_BOTTOMUP	0x02				/**< used when the class requires synchronization on the bottom-up pass */
#define PC_POSTTOPDOWN 0x04				/**< used when the class requires synchronization on the second top-down pass */
#define PC_FORCE_NAME 0x20				/**< used to indicate the this class must define names for all its objects */
#define PC_PARENT_OVERRIDE_OMIT 0x40	/**< used to ignore parent's use of PC_UNSAFE_OVERRIDE_OMIT */
#define PC_UNSAFE_OVERRIDE_OMIT 0x80	/**< used to flag that omitting overrides is unsafe */ 

typedef enum {
	NM_PREUPDATE = 0, /**< notify module before property change */
	NM_POSTUPDATE = 1, /**< notify module after property change */
	NM_RESET = 2,/**< notify module of system reset event */
} NOTIFYMODULE; /**< notification message types */

typedef struct s_keyword {
	char name[32];
	unsigned long value;
	struct s_keyword *next;
} KEYWORD;

typedef unsigned long PROPERTYFLAGS;
#define PF_RECALC	0x0001 /**< property has a recalc trigger (only works if recalc_<class> is exported) */
#define PF_CHARSET	0x0002 /**< set supports single character keywords (avoids use of |) */
#define PF_DEPRECATED 0x8000 /**< set this flag to indicate that the property is deprecated (warning will be displayed anytime it is used */

typedef struct s_property_map {
	CLASS *oclass; /**< class implementing the property */
	PROPERTYNAME name; /**< property name */
	PROPERTYTYPE ptype; /**< property type */
	unsigned long size; /**< property array size */
	PROPERTYACCESS access; /**< property access flags */
	UNIT *unit; /**< property unit, if any; \p NULL if none */
	PROPERTYADDR addr; /**< property location, offset from OBJECT header */
	DELEGATEDTYPE *delegation; /**< property delegation, if any; \p NULL if none */
	KEYWORD *keywords; /**< keyword list, if any; \p NULL if none (only for set and enumeration types)*/
	char *description; /**< description of property */
	struct s_property_map *next; /**< next property in property list */
	PROPERTYFLAGS flags; /**< property flags (e.g., PF_RECALC) */
} PROPERTY; /**< property definition item */

typedef struct s_function_map {
	CLASS *oclass;
	FUNCTIONNAME name;
	FUNCTIONADDR addr;
	struct s_function_map *next;
} FUNCTION;

/* Set operations */
#define SET_MASK 0xffff
#define SET_ADD(set,value) (set = set | value )
#define SET_DEL(set,value) (set = (value^SET_MASK)&set) /*SET_HAS(set,value)?set^value:set) */
#define SET_CLEAR(set) (set = 0)
#define SET_HAS(set,value) (set & value)

typedef enum {CLASSVALID=0xc44d822e} CLASSMAGIC; /* this is used to uniquely identify classes */

#define _MODULE_DEFINED_
typedef struct s_module_list MODULE;
struct s_class_list {
	CLASSMAGIC magic;
	CLASSNAME name;
	unsigned int size;
	MODULE *module; 
	PROPERTY *pmap;
	FUNCTION *fmap;
	FUNCTIONADDR create;
	FUNCTIONADDR init;
	FUNCTIONADDR sync;
	FUNCTIONADDR commit;
	FUNCTIONADDR notify;
	FUNCTIONADDR isa;
	FUNCTIONADDR plc;
	PASSCONFIG passconfig;
	FUNCTIONADDR recalc;
	CLASS *parent;			/**< parent class from which properties should be inherited */
	struct {
		int32 numobjs;
		int64 clocks;
		int32 count;
	} profiler;
	struct s_class_list *next;
}; /* CLASS */

#ifdef __cplusplus
extern "C" {
#endif

PROPERTY *class_get_first_property(CLASS *oclass);
PROPERTY *class_get_next_property(PROPERTY *prop);
PROPERTY *class_prop_in_class(CLASS *oclass, PROPERTY *prop);
PROPERTY *class_find_property(CLASS *oclass, PROPERTYNAME name);
void class_add_property(CLASS *oclass, PROPERTY *prop);
PROPERTY *class_add_extended_property(CLASS *oclass, char *name, PROPERTYTYPE ptype, char *unit);
PROPERTYTYPE class_get_propertytype_from_typename(char *name);
int class_string_to_property(PROPERTY *prop, void *addr, char *value);
int class_property_to_string(PROPERTY *prop, void *addr, char *value, int size);
CLASS *class_get_first_class(void);
CLASS *class_get_last_class(void);
CLASS *class_get_class_from_classname(char *name);
CLASS *class_get_class_from_classname_in_module(char *name, MODULE *mod);
char *class_get_property_typename(PROPERTYTYPE type);
int class_saveall(FILE *fp);
int class_saveall_xml(FILE *fp);
unsigned int class_get_count(void);
void class_profiles(void);
int class_get_xsd(CLASS *oclass, char *buffer, size_t len);

CLASS *class_register(MODULE *module,CLASSNAME classname,unsigned int datasize,PASSCONFIG passconfig);
int class_define_map(CLASS *oclass, ...);
int class_define_enumeration_member(CLASS *oclass,char *property_name,char *member,enumeration value);
int class_define_set_member(CLASS *oclass,char *property_name,char *member,unsigned long value);
FUNCTION *class_define_function(CLASS *oclass, FUNCTIONNAME functionname, FUNCTIONADDR call);
FUNCTIONADDR class_get_function(char *classname, char *functionname);
DELEGATEDTYPE *class_register_type(CLASS *oclass, char *type,int (*from_string)(void*,char*),int (*to_string)(void*,char*,int));
int class_define_type(CLASS *oclass, DELEGATEDTYPE *delegation, ...);

unsigned long property_size(PROPERTY *prop);
int property_create(PROPERTY *prop, void *addr);
PROPERTY *property_malloc(PROPERTYTYPE proptype, CLASS *oclass, char *name, void *addr, DELEGATEDTYPE *delegation);

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
