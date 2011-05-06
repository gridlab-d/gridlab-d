#ifndef _PROPERTY_H
#define _PROPERTY_H

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "platform.h"
#include "complex.h"
#include "unit.h"

// also in object.h
typedef struct s_class_list CLASS;

// also in class.h
#ifndef FADDR
#define FADDR
typedef int64 (*FUNCTIONADDR)(void*,...); /** the entry point of a module function */
#endif

/* Valid GridLAB data types */
typedef char char1024[1025]; /**< strings up to 1024 characters */
typedef char char256[257]; /**< strings up to 256 characters */
typedef char char32[33]; /**< strings up to 32 characters */
typedef char char8[9]; /** string up to 8 characters */

#ifdef HAVE_STDINT_H
#include <stdint.h>
typedef int8_t    int8;     /* 8-bit integers */
typedef int16_t   int16;    /* 16-bit integers */
typedef int32_t   int32;    /* 32-bit integers */
typedef uint32_t  uint32;   /* unsigned 32-bit integers */
typedef uint64_t  uint64;   /* unsigned 64-bit integers */
typedef uint64    set;      /* sets (each of up to 64 values may be defined) */
#else /* no HAVE_STDINT_H */
typedef char int8; /** 8-bit integers */
typedef short int16; /** 16-bit integers */
typedef int int32; /* 32-bit integers */
typedef unsigned int64 set; /* sets (each of up to 64 values may be defined) */
typedef unsigned int uint32; /* unsigned 32-bit integers */
typedef unsigned int64 uint64;
#endif /* HAVE_STDINT_H */
typedef uint32 enumeration; /* enumerations (any one of a list of values) */
typedef struct s_object_list* object; /* GridLAB objects */
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
	PT_HAS_NOTIFY, /* used to indicate that a notify function exists for the specified property */
	PT_HAS_NOTIFY_OVERRIDE, /* as PT_HAS_NOTIFY, but instructs the core not to set the property to the value being set */
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

typedef struct s_keyword {
	char name[32];
	uint64 value;
	struct s_keyword *next;
} KEYWORD;

typedef uint32 PROPERTYFLAGS;
#define PF_RECALC	0x0001 /**< property has a recalc trigger (only works if recalc_<class> is exported) */
#define PF_CHARSET	0x0002 /**< set supports single character keywords (avoids use of |) */
#define PT_EXTENDED 0x0004 /**< indicates that the property was added at runtime */
#define PF_DEPRECATED 0x8000 /**< set this flag to indicate that the property is deprecated (warning will be displayed anytime it is used */
#define PF_DEPRECATED_NONOTICE 0x04000 /**< set this flag to indicate that the property is deprecated but no reference warning is desired */

typedef struct s_property_map {
	CLASS *oclass; /**< class implementing the property */
	PROPERTYNAME name; /**< property name */
	PROPERTYTYPE ptype; /**< property type */
	uint32 size; /**< property array size */
	uint32 width; /**< property byte size, copied from array in class.c */
	PROPERTYACCESS access; /**< property access flags */
	UNIT *unit; /**< property unit, if any; \p NULL if none */
	PROPERTYADDR addr; /**< property location, offset from OBJECT header */
	DELEGATEDTYPE *delegation; /**< property delegation, if any; \p NULL if none */
	KEYWORD *keywords; /**< keyword list, if any; \p NULL if none (only for set and enumeration types)*/
	char *description; /**< description of property */
	struct s_property_map *next; /**< next property in property list */
	PROPERTYFLAGS flags; /**< property flags (e.g., PF_RECALC) */
	FUNCTIONADDR notify;
	bool notify_override;
} PROPERTY; /**< property definition item */

PROPERTY *property_malloc(PROPERTYTYPE, CLASS *, char *, void *, DELEGATEDTYPE *);
uint32 property_size(PROPERTY *);
uint32 property_size_by_types(PROPERTYTYPE);
int property_create(PROPERTY *, void *);

#endif //_PROPERTY_H

// EOF
