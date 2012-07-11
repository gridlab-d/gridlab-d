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

#ifdef HAVE_STDINT_H
#include <stdint.h>
typedef int8_t    int8;     /* 8-bit integers */
typedef int16_t   int16;    /* 16-bit integers */
typedef int32_t   int32;    /* 32-bit integers */
typedef uint16_t  uint16;	/* unsigned 16-bit integers */
typedef uint32_t  uint32;   /* unsigned 32-bit integers */
typedef uint64_t  uint64;   /* unsigned 64-bit integers */
#else /* no HAVE_STDINT_H */
typedef char int8; /** 8-bit integers */
typedef short int16; /** 16-bit integers */
typedef int int32; /* 32-bit integers */
typedef unsigned short uint16;
typedef unsigned int uint32; /* unsigned 32-bit integers */
typedef unsigned int64 uint64;
#endif /* HAVE_STDINT_H */

/* Valid GridLAB data types */
typedef char char1024[1025]; /**< strings up to 1024 characters */
typedef char char256[257]; /**< strings up to 256 characters */
typedef char char32[33]; /**< strings up to 32 characters */
typedef char char8[9]; /** string up to 8 characters */
typedef uint64 set;      /* sets (each of up to 64 values may be defined) */
typedef uint32 enumeration; /* enumerations (any one of a list of values) */
typedef struct s_object_list* object; /* GridLAB objects */
typedef double triplet[3];
typedef complex triplex[3];

#ifdef __cplusplus
#include <math.h>

class double_array {
private:
	unsigned int n, m; /** n rows, m cols */
	unsigned int max; /** current allocation size max x max */
	double ***x; /** pointer to 2D array of pointers to double values */
	inline void set_n(unsigned int i) { n=i; };
	inline void set_m(unsigned int i) { m=i; };
public:
	inline unsigned int get_n(void) { return n; };
	inline unsigned int get_m(void) { return m; };
	inline unsigned int get_max(void) { return max; };
	inline void set_max(unsigned int size) 
	{
		if ( size<=max ) throw "cannot shrink double_array";
		unsigned int r;
		double ***z = (double***)malloc(sizeof(double***)*size);
		// create new rows
		for ( r=0 ; r<max ; r++ )
		{
			if ( x[r]!=NULL )
			{
				double **y = (double**)malloc(sizeof(double**)*size);
				if ( y==NULL ) throw "unable to expand double_array";
				memcpy(y,x[r],sizeof(double**)*max);
				memset(y+max,0,sizeof(double**)*(size-max));
				free(x[r]);
				z[r] = y;
			}
			else
				z[r] = NULL;
		}
		memset(z+max,0,sizeof(double***)*(size-max));
		free(x);
		x = z;
		max=size; /* TODO resize */ 
	};
	inline void grow_to(unsigned int c, unsigned int r) 
	{ 
		if ( c>=max || r>=max ) set_max(max*2);
		// add rows
		while ( n<=r ) 
		{
			if ( x[n]==NULL ) 
			{
				x[n] = (double**)malloc(sizeof(double**)*max);
				memset(x[n],0,sizeof(double**)*max);
			}
			n++;
		}
		if (m<=c) m=c+1; 
	};
	inline bool is_valid(unsigned int c, unsigned int r) { return r<n && c<m; };
	inline bool is_nan(unsigned int c, unsigned int r) { return ! ( is_valid(c,r) && x[r][c]!=NULL && isfinite(*(x[r][c])) ); };
	inline void clr_at(unsigned int c, unsigned int r) 
	{ 
		if ( is_valid(c,r) ) 
		{ 
			free(x[r][c]); x[r][c]=NULL; 
		} 
	};
	inline double get_at(unsigned int c, unsigned int r) { return is_nan(c,r) ? QNAN : *(x[r][c]) ; };
	inline void set_at(unsigned int c, unsigned int r, double v) 
	{ 
		if ( is_valid(c,r) ) 
		{ 
			if ( x[r][c]==NULL ) 
				x[r][c]=(double*)malloc(sizeof(double)); 
			else 
				*(x[r][c]) = v; 
		} 
	};
	inline void set_at(unsigned int c, unsigned int r, double *v) 
	{ 
		if ( is_valid(c,r) ) 
		{ 
			if ( v==NULL ) 
			{
				if ( x[r][c]!=NULL ) 
					clr_at(c,r);
			}
			else 
			{
				if ( x[r][c]==NULL ) 
					x[r][c]=(double*)malloc(sizeof(double)); 
				*(x[r][c]) = *v; 
			}
		} 
	} ;
};
class complex_array {
public:
	unsigned int n, m;
	unsigned int max; /** current allocation size max x max */
	complex ***x; /** pointer to 2D array of pointers to complex values */
};
#else
typedef struct {
	/* KEEP THIS CONSISTENT WITH "class double_array" */
	unsigned int n, m; /** n rows, m cols */
	unsigned int max; /** current allocation size max x max */
	double ***x; /** pointer to 2D array of pointers to double values */
} double_array;
typedef struct {
	/* KEEP THIS CONSISTENT WITH "class complex_array" */
	unsigned int n, m;
	unsigned int max; /** current allocation size max x max */
	complex ***x; /** pointer to 2D array of pointers to complex values */
} complex_array;
#endif

/* ADD NEW CORE TYPES HERE */

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
	PT_random,		/**< Randomized number */
	/* add new property types here - don't forget to add them also to rt/gridlabd.h and property.c */
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

/* property access rights (R/W apply to modules only, core always has all rights) */
#define PA_N 0x00 /**< no access permitted */
#define PA_R 0x01 /**< read access--modules can read the property */
#define PA_W 0x02 /**< write access--modules can write the property */
#define PA_S 0x04 /**< save access--property is saved to output */
#define PA_L 0x08 /**< load access--property is loaded from input */
typedef enum {
	PA_PUBLIC = (PA_R|PA_W|PA_S|PA_L), /**< property is public (readable, writable, saved, and loaded) */
	PA_REFERENCE = (PA_R|PA_W|PA_S), /**< property is FYI (readable, writable and saved, but not loaded */
	PA_PROTECTED = (PA_R), /**< property is semipublic (readable, but not saved or loaded) */
	PA_PRIVATE = (PA_N), /**< property is nonpublic (not visible, saved or loaded) */
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

typedef enum { 
	TCOP_EQ=0, 
	TCOP_LE=1, 
	TCOP_GE=2, 
	TCOP_NE=3, 
	TCOP_LT=4,
	TCOP_GT=5,
	TCOP_IN=6,
	TCOP_NI=7,
	_TCOP_LAST,
	TCOP_NOP,
	TCOP_ERR=-1
} PROPERTYCOMPAREOP;
typedef int PROPERTYCOMPAREFUNCTION(void*,void*,void*);

typedef struct s_property_specs { /**<	the property type conversion specifications.
								It is critical that the order of entries in this list must match 
								the order of entries in the enumeration #PROPERTYTYPE 
						  **/
	char *name; /**< the property type name */
	char *xsdname;
	unsigned int size; /**< the size of 1 instance */
	unsigned int csize; /**< the minimum size of a converted instance (not including '\0' or unit, 0 means a call to property_minimum_buffersize() is necessary) */ 
	int (*data_to_string)(char *,int,void*,PROPERTY*); /**< the function to convert from data to a string */
	int (*string_to_data)(char *,void*,PROPERTY*); /**< the function to convert from a string to data */
	int (*create)(void*); /**< the function used to create the property, if any */
	int (*stream_in)(FILE*,void*,PROPERTY*); /**< the function to read data from a stream */
	int (*stream_out)(FILE*,void*,PROPERTY*); /**< the function to write data to a stream */
	struct {
		PROPERTYCOMPAREOP op;
		char str[16];
		PROPERTYCOMPAREFUNCTION* fn;
		int trinary;
	} compare[_TCOP_LAST]; /**< the list of comparison operators available for this type */
	double (*get_part)(void*,char *name); /**< the function to get a part of a property */
	// @todo for greater generality this should be implemented as a linked list
} PROPERTYSPEC;
PROPERTYSPEC *property_getspec(PROPERTYTYPE ptype);

#ifdef __cplusplus
extern "C" {
#endif

PROPERTY *property_malloc(PROPERTYTYPE, CLASS *, char *, void *, DELEGATEDTYPE *);
uint32 property_size(PROPERTY *);
uint32 property_size_by_type(PROPERTYTYPE);
size_t property_minimum_buffersize(PROPERTY *);
int property_create(PROPERTY *, void *);
bool property_compare_basic(PROPERTYTYPE ptype, PROPERTYCOMPAREOP op, void *x, void *a, void *b, char *part);
PROPERTYCOMPAREOP property_compare_op(PROPERTYTYPE ptype, char *opstr);
PROPERTYTYPE property_get_type(char *name);

/* double array */
int double_array_create(double_array*a);
double get_double_array_value(double_array*,unsigned int n, unsigned int m);
void set_double_array_value(double_array*,unsigned int n, unsigned int m, double x);
double *get_double_array_ref(double_array*,unsigned int n, unsigned int m);
double double_array_get_part(void *x, char *name);


#ifdef __cplusplus
}
#endif

#endif //_PROPERTY_H

// EOF
