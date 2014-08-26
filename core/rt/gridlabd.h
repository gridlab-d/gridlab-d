/** $Id: gridlabd.h 4738 2014-07-03 00:55:39Z dchassin $
    Copyright (C) 2008 Battelle Memorial Institute
	@file rt/gridlabd.h
	@defgroup runtime Runtime Class API

	The Runtime Class API is a programming library for inline C++ embedded
	within GLM files.

 @{
 **/
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <memory.h>
#include <float.h>
#include <string.h>

#ifdef _WINDOWS
#define isfinite _finite
#endif

# if __WORDSIZE == 64
#define int64 long int /**< standard version of 64-bit integers */
#else
#define int64 long long /**< standard version of 64-bit integers */
#endif

typedef enum {I='i',J='j',A='d'} CNOTATION; /**< complex number notation to use */
#define CNOTATION_DEFAULT J /* never set this to A */
#define PI 3.1415926535897932384626433832795
#define E 2.71828182845905

#define _NO_CPPUNIT

/* only cpp code may actually do complex math */
class complex {
private:
	double r; /**< the real part */
	double i; /**< the imaginary part */
	CNOTATION f; /**< the default notation to use */
public:
	/** Construct a complex number with zero magnitude */
	inline complex() /**< create a zero complex number */
	{
		r = 0;
		i = 0;
		f = CNOTATION_DEFAULT;
	};
	inline complex(double re) /**< create a complex number with only a real part */
	{
		r = re;
		i = 0;
		f = CNOTATION_DEFAULT;
	};
	inline complex(double re, double im, CNOTATION nf=CNOTATION_DEFAULT) /**< create a complex number with both real and imaginary parts */
	{
		r = re;
		i = im;
		f = nf;
	};

	/* assignment operations */
	inline complex &operator = (complex x) /**< complex assignment */
	{
		r = x.r;
		i = x.i;
		f = x.f;
		return *this;
	};
	inline complex &operator = (double x) /**< double assignment */
	{
		r = x;
		i = 0;
		f = CNOTATION_DEFAULT;
		return *this;
	};

	/* access operations */
	inline double & Re(void) /**< access to real part */
	{
		return r;
	};
	inline double & Im(void) /**< access to imaginary part */
	{
		return i;
	};
	inline CNOTATION & Notation(void) /**< access to notation */
	{
		return f;
	};
	inline double Mag(void) const /**< compute magnitude */
	{
		return sqrt(r*r+i*i);
	};
	inline double Mag(double m)  /**< set magnitude */
	{
		double old = sqrt(r*r+i*i);
		r *= m/old;
		i *= m/old;
		return m;
	};
	inline double Arg(void) const /**< compute angle */
	{
		if (r==0)
		{
			if (i>0)
				return PI/2;
			else if (i==0)
				return 0;
			else
				return -PI/2;
		}
		else if (r>0)
			return atan(i/r);
		else
			return PI+atan(i/r);
	};
	inline double Arg(double a)  /**< set angle */
	{
		SetPolar(Mag(),a,f);
		return a;
	};
	inline complex Log(void) const /**< compute log */
	{
		return complex(log(Mag()),Arg(),f);
	};
	inline void SetReal(double v) /**< set real part */
	{
		r = v;
	};
	inline void SetImag(double v) /**< set imaginary part */
	{
		i = v;
	};
	inline void SetNotation(CNOTATION nf) /**< set notation */
	{
		f = nf;
	}
	inline void SetRect(double rp, double ip, CNOTATION nf=CNOTATION_DEFAULT) /**< set rectangular value */
	{
		r = rp;
		i = ip;
		f = nf;
	};
	inline void SetPolar(double m, double a, CNOTATION nf=A) /**< set polar values */
	{
		r = (m*cos(a));
		i = (m*sin(a));
		f = nf;
	};

#if 0
	//inline operator const double (void) const /**< cast real part to double */
	//{
	//	return r;
	//};
#endif

	inline complex operator - (void) /**< change sign */
	{
		return complex(-r,-i,f);
	};
	inline complex operator ~ (void) /**< complex conjugate */
	{
		return complex(r,-i,f);
	};

	/* reflexive math operations */
	inline complex &operator += (double x) /**< add a double to the real part */
	{
		r += x;
		return *this;
	};
	inline complex &operator -= (double x) /**< subtract a double from the real part */
	{
		r -= x;
		return *this;
	};
	inline complex &operator *= (double x) /**< multiply a double to real part */
	{
		r *= x;
		i *= x;
		return *this;
	};
	inline complex &operator /= (double x) /**< divide into the real part */
	{
		r /= x;
		i /= x;
		return *this;
	};
	inline complex &operator ^= (double x) /**< raise to a real power */
	{
		double lm = log(Mag()), a = Arg(), b = exp(x*lm), c = x*a;
		r = (b*cos(c));
		i = (b*sin(c));
		return *this;
	};
	inline complex &operator += (complex x) /**< add a complex number */
	{
		r += x.r;
		i += x.i;
		return *this;
	};
	inline complex &operator -= (complex x)  /**< subtract a complex number */
	{
		r -= x.r;
		i -= x.i;
		return *this;
	};
	inline complex &operator *= (complex x)  /**< multip[le by a complex number */
	{
		double pr=r;
		r = pr * x.r - i * x.i;
		i = pr * x.i + i * x.r;
		return *this;
	};
	inline complex &operator /= (complex y)  /**< divide by a complex number */
	{
		double xr=r;
		double a = y.r*y.r+y.i*y.i;
		r = (xr*y.r+i*y.i)/a;
		i = (i*y.r-xr*y.i)/a;
		return *this;
	};
	inline complex &operator ^= (complex x) /**< raise to a complex power */
	{
		double lm = log(Mag()), a = Arg(), b = exp(x.r*lm-x.i*a), c = x.r*a+x.i*lm;
		r = (b*cos(c));
		i = (b*sin(c));
		return *this;
	};

	/* binary math operations */
	inline complex operator + (double y) /**< double sum */
	{
		complex x(*this);
		return x+=y;
	};
	inline complex operator - (double y) /**< double subtract */
	{
		complex x(*this);
		return x-=y;
	};
	inline complex operator * (double y) /**< double multiply */
	{
		complex x(*this);
		return x*=y;
	};
	inline complex operator / (double y) /**< double divide */
	{
		complex x(*this);
		return x/=y;
	};
	inline complex operator ^ (double y) /**< double power */
	{
		complex x(*this);
		return x^=y;
	};
	inline complex operator + (complex y) /**< complex sum */
	{
		complex x(*this);
		return x+=y;
	};
	inline complex operator - (complex y) /**< complex subtract */
	{
		complex x(*this);
		return x-=y;
	};
	inline complex operator * (complex y) /**< complex multiply */
	{
		complex x(*this);
		return x*=y;
	};
	inline complex operator / (complex y) /**< complex divide */
	{
		complex x(*this);
		return x/=y;
	};
	inline complex operator ^ (complex y) /**< complex power */
	{
		complex x(*this);
		return x^=y;
	};

	/** set power factor */
	inline complex SetPowerFactor(double mag, /**< magnitude of power */
		double pf, /**< power factor */
		CNOTATION n=J) /** notation */
	{
		SetPolar(mag/pf, acos(pf),n);
		return *this;
	}


	/* comparison */
	inline bool IsZero(double err=0.0) /**< zero test */
	{
		return Mag()<=err;
	}

	/* magnitude comparisons */
	inline bool operator == (double m)	{ return Mag()==m; };
	inline bool operator != (double m)	{ return Mag()!=m; };
	inline bool operator < (double m)	{ return Mag()<m; };
	inline bool operator <= (double m)	{ return Mag()<=m; };
	inline bool operator > (double m)	{ return Mag()>m; };
	inline bool operator >= (double m)	{ return Mag()>=m; };

	/* angle comparisons */
	inline complex operator == (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)==0.0;};
	inline complex operator != (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)!=0.0;};
	inline complex operator < (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)<PI;};
	inline complex operator <= (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)<=PI;};
	inline complex operator > (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)>PI;};
	inline complex operator >= (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)>=PI;};
	inline bool IsFinite(void) { return isfinite(r) && isfinite(i); };
};

#ifdef REAL4
typedef float real;
#else
typedef double real;
#endif

typedef int64 TIMESTAMP;
#define TS_TPS ((int64)1)	/* ticks per second time resolution - this needs to be compatible with TS_SECOND is timestamp.h */
#define TS_REALNOW ((TIMESTAMP)(time(NULL))*TS_TPS)
#define TS_SIMNOW (*(callback->global_clock))
#define TS_ZERO ((int64)0)
#define Ts_INIT ((int64)3600*12) /* 12 hours after time zero to avoid negative timezone values */
#define TS_MAX (32482080000LL) /* roughly 3000 CE, any date beyond this should be interpreted as TS_NEVER */
#define TS_INVALID ((int64)-1)
#define TS_NEVER ((int64)(((unsigned int64)-1)>>1))
#define MINYEAR 1970
#define MAXYEAR 2969
/* Deltamode declarations */
typedef enum {
	SM_INIT			= 0x00, /**< initial state of simulation */
	SM_EVENT		= 0x01, /**< event driven simulation mode */
	SM_DELTA		= 0x02, /**< finite difference simulation mode */
	SM_DELTA_ITER	= 0x03, /**< Iteration of finite difference simulation mode */
	SM_ERROR		= 0xff, /**< simulation mode error */
} SIMULATIONMODE; /**< simulation mode values */
typedef enum {
	DMF_NONE		= 0x00,	/**< no flags */
	DMF_SOFTEVENT	= 0x01,/**< event is soft */
} DELTAMODEFLAGS; /**< delta mode flags */
typedef unsigned int64 DELTAT; /**< stores cumulative delta time values in ns */
typedef unsigned long DT; /**< stores incremental delta time values in ns */
#define DT_INFINITY (0xfffffffe)
#define DT_INVALID  (0xffffffff)
#define DT_SECOND 1000000000

inline TIMESTAMP gl_timestamp(double t) { return (TIMESTAMP)(t*TS_TPS);}
inline double gl_seconds(TIMESTAMP t) { return (double)t/(double)TS_TPS;}
inline double gl_minutes(TIMESTAMP t) { return (double)t/(double)(TS_TPS*60);}
inline double gl_hours(TIMESTAMP t) { return (double)t/(double)(TS_TPS*60*60);}
inline double gl_days(TIMESTAMP t) { return (double)t/(double)(TS_TPS*60*60*24);}
inline double gl_weeks(TIMESTAMP t) { return (double)t/(double)(TS_TPS*60*60*24*7);}

typedef union {
	struct {
		unsigned int calendar:4;
		unsigned int minute:20;
	};
	unsigned int index;
} SCHEDULEINDEX;

typedef enum {
	RT_INVALID=-1,	/**< used to flag bad random types */
	RT_DEGENERATE,	/**< degenerate distribution (Dirac delta function); double only_value */
	RT_UNIFORM,		/**< uniform distribution; double minimum_value, double maximum_value */
	RT_NORMAL,		/**< normal distribution; double arithmetic_mean, double arithmetic_stdev */
	RT_LOGNORMAL,	/**< log-normal distribution; double geometric_mean, double geometric_stdev */
	RT_BERNOULLI,	/**< Bernoulli distribution; double probability_of_observing_1 */
	RT_PARETO,		/**< Pareto distribution; double minimum_value, double gamma_scale */
	RT_EXPONENTIAL, /**< exponential distribution; double coefficient, double k_scale */
	RT_SAMPLED,		/**< sampled distribution; unsigned number_of_samples, double samples[n_samples] */
	RT_RAYLEIGH,	/**< Rayleigh distribution; double sigma */
	RT_WEIBULL,		/**< Weibull distribution; double lambda, double k */
	RT_GAMMA,		/**< Gamma distribution; double alpha, double beta */
	RT_BETA,		/**< Beta distribution; double alpha, double beta */
	RT_TRIANGLE,	/**< Triangle distribution; double a, double b */
} RANDOMTYPE;

typedef char char1024[1025]; /**< strings up to 1024 characters */
typedef char char256[257]; /**< strings up to 256 characters */
typedef char char32[33]; /**< strings up to 32 characters */
typedef char char8[9]; /** string up to 8 characters */
typedef char int8; /** 8-bit integers */
typedef short int16; /** 16-bit integers */
typedef int int32; /* 32-bit integers */
typedef unsigned int uint32; /* 32-bit integers */
typedef int32 enumeration; /* enumerations (any one of a list of values) */
typedef TIMESTAMP timestamp;
typedef struct s_object_list* object; /* GridLAB objects */
typedef unsigned int64 set; /* sets (each of up to 64 values may be defined) */
typedef double triplet[3];
typedef complex triplex[3];
typedef struct s_randomvar {
	double value;				/**< current value */
	unsigned int state;			/**< RNG state */
	RANDOMTYPE type;			/**< RNG distribution */
	double a, b;				/**< RNG distribution parameters */
	double low, high;			/**< RNG truncations limits */
	unsigned int update_rate;	/**< RNG refresh rate in seconds */
	unsigned int flags;			/**< RNG flags */
	/* internal parameters */
	struct s_randomvar *next;
} randomvar;

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
	PT_bool, /**< the data is a true/false value */
	PT_timestamp, /**< timestamp value */
	PT_double_array, /**< the data is a fixed length double[] */
	PT_complex_array, /**< the data is a fixed length complex[] */
/*	PT_object_array, */ /**< the data is a fixed length array of object pointers*/
	PT_float,	/**< Single-precision float	*/
	PT_real,	/**< Single or double precision float ~ allows double values to be overriden */
	PT_loadshape,	/**< Loadshapes are state machines driven by schedules */
	PT_enduse,		/**< Enduse load data */
	PT_random,		/**< Randomized number */
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
	PT_DEPRECATED,
	PT_HAS_NOTIFY,
	PT_HAS_NOTIFY_OVERRIDE,
} PROPERTYTYPE; /**< property types */
typedef char CLASSNAME[64]; /**< the name a GridLAB class */
typedef unsigned int OBJECTRANK; /**< Object rank number */
typedef unsigned short OBJECTSIZE; /** Object data size */
typedef unsigned int OBJECTNUM; /** Object id number */
typedef char * OBJECTNAME; /** Object name */
typedef char FUNCTIONNAME[64]; /**< the name of a function (not used) */
typedef void* PROPERTYADDR; /**< the offset of a property from the end of the OBJECT header */
typedef char PROPERTYNAME[64]; /**< the name of a property */
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
typedef int64 (*FUNCTIONADDR)(void*,...); /** the entry point of a module function */
typedef struct s_unit UNIT;
typedef struct s_delegatedtype DELEGATEDTYPE;
typedef struct s_delegatedvalue DELEGATEDVALUE;
typedef DELEGATEDVALUE* delegated; /* delegated data type */
typedef struct s_object_list OBJECT;
typedef struct s_module_list MODULE;
typedef struct s_class_list CLASS;
typedef struct s_property_map PROPERTY;
typedef struct s_function_map FUNCTION;
typedef struct s_schedule SCHEDULE;
typedef struct s_loadshape_core loadshape;
typedef struct s_enduse enduse;

#define MAXBLOCKS 4
#define MAXVALUES 64

/* pass configuration */
typedef unsigned long PASSCONFIG; /**< the pass configuration */
#define PC_NOSYNC 0x00					/**< used when the class requires no synchronization */
#define PC_PRETOPDOWN 0x01				/**< used when the class requires synchronization on the first top-down pass */
#define PC_BOTTOMUP	0x02				/**< used when the class requires synchronization on the bottom-up pass */
#define PC_POSTTOPDOWN 0x04				/**< used when the class requires synchronization on the second top-down pass */
#define PC_FORCE_NAME 0x20				/**< used to indicate the this class must define names for all its objects */
#define PC_PARENT_OVERRIDE_OMIT 0x40	/**< used to ignore parent's use of PC_UNSAFE_OVERRIDE_OMIT */
#define PC_UNSAFE_OVERRIDE_OMIT 0x80	/**< used to flag that omitting overrides is unsafe */
#define PC_ABSTRACTONLY 0x100 /**< used to flag that the class should never be instantiated itself, only inherited classes should */

#ifndef FALSE
#define FALSE (0)
#define TRUE (!FALSE)
#endif

typedef enum {FAILED=FALSE, SUCCESS=TRUE} STATUS;

typedef struct s_globalvar {
	PROPERTY *prop;
	struct s_globalvar *next;
	uint32 flags;
	void (*callback)(char*);
	unsigned int lock;
} GLOBALVAR;

typedef enum {
	NM_PREUPDATE = 0, /**< notify module before property change */
	NM_POSTUPDATE = 1, /**< notify module after property change */
	NM_RESET = 2,/**< notify module of system reset event */
} NOTIFYMODULE; /**< notification message types */

#include "setjmp.h"
typedef struct s_exception_handler {
	int id; /**< the exception handler id */
	jmp_buf buf; /**< the \p jmpbuf containing the context for the exception handler */
	char msg[1024]; /**< the message thrown */
	struct s_exception_handler *next; /**< the next exception handler */
} EXCEPTIONHANDLER; /**< the exception handler structure */

typedef struct s_keyword KEYWORD;

typedef uint32 PROPERTYFLAGS;
#define PF_RECALC	0x0001 /**< property has a recalc trigger (only works if recalc_<class> is exported) */
#define PF_CHARSET	0x0002 /**< set supports single character keywords (avoids use of |) */

struct s_property_map {
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
	PROPERTY *next; /**< next property in property list */
	PROPERTYFLAGS flags; /**< property flags (e.g., PF_RECALC) */
	FUNCTIONADDR notify;
	bool notify_override;
}; /**< property definition item */

typedef struct s_property_struct {
	PROPERTY *prop;
	PROPERTYNAME part;
} PROPERTYSTRUCT;

struct s_unit {
	char name[64];		/**< the name of the unit */
	double c,e,h,k,m,s,a,b; /**< the unit parameters */
	int prec; /** the precision of the unit definition */
	UNIT *next; /**< the next unit is the unit list */
}; /**< the UNIT structure */
struct s_delegatedtype {
	char32 type; /**< the name of the delegated type */
	CLASS *oclass; /**< the class implementing the delegated type */
	int (*from_string)(void *addr, char *value); /**< the function that converts from a string to the data */
	int (*to_string)(void *addr, char *value, int size); /**< the function that converts from the data to a string */
}; /**< type delegation specification */
struct s_delegatedvalue {
	char *data; /**< the data that is delegated */
	DELEGATEDTYPE *type; /**< the delegation specification to use */
}; /**< a delegation entry */
struct s_keyword {
	char name[32];
	uint32 value;
	struct s_keyword *next;
};
struct s_module_list {
	void *hLib;
	unsigned int id;
	char name[1024];
	CLASS *oclass;
	unsigned short major;
	unsigned short minor;
	void* (*getvar)(char *varname,char *value,unsigned int size);
	int (*setvar)(char *varname,char *value);
	int (*import_file)(char *file);
	int (*export_file)(char *file);
	int (*check)();
	/* deltamode */
	unsigned long (*deltadesired)(DELTAMODEFLAGS*);
	unsigned long (*preupdate)(void*,int64,unsigned int64);
	SIMULATIONMODE (*interupdate)(void*,int64,unsigned int64,unsigned long,unsigned int);
	STATUS (*postupdate)(void*,int64,unsigned int64);
#ifndef _NO_CPPUNIT
	int (*module_test)(void *callbacks,int argc,char* argv[]);
#endif
	int (*cmdargs)(int,char**);
	int (*kmldump)(FILE*fp,OBJECT*);
	void (*test)(int argc, char *argv[]);
	MODULE *(*subload)(char *, MODULE **, CLASS **, int, char **);
	PROPERTY *globals;
	void (*term)(void);
	size_t (*stream)(FILE *fp, int flags);
	MODULE *next;
};

/* Technology readiness levels (see http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Technology_Readiness_Levels) */
typedef enum {
	TRL_UNKNOWN			= 0,
	TRL_PRINCIPLE		= 1,
	TRL_CONCEPT			= 2,
	TRL_PROOF			= 3,
	TRL_STANDALONE		= 4,
	TRL_INTEGRATED		= 5,
	TRL_DEMONSTRATED	= 6,
	TRL_PROTOTYPE		= 7,
	TRL_QUALIFIED		= 8,
	TRL_PROVEN			= 9,
} TECHNOLOGYREADINESSLEVEL;

typedef enum {CLASSVALID=0xc44d822e} CLASSMAGIC; ///< this is used to uniquely identify class structure

struct s_class_list {
	CLASSMAGIC magic;
	int id;
	CLASSNAME name;
	unsigned int size;
	MODULE *module;
	PROPERTY *pmap;
	FUNCTION *fmap;
	FUNCTIONADDR create;
	FUNCTIONADDR init;
	FUNCTIONADDR precommit;
	FUNCTIONADDR sync;
	FUNCTIONADDR commit;
	FUNCTIONADDR finalize;
	FUNCTIONADDR notify;
	FUNCTIONADDR isa;
	FUNCTIONADDR plc;
	PASSCONFIG passconfig;
	FUNCTIONADDR recalc;
	FUNCTIONADDR update;	/**< deltamode related */
	FUNCTIONADDR heartbeat;
	CLASS *parent;			/**< parent class from which properties should be inherited */
	struct {
		unsigned int lock;
		int32 numobjs;
		int64 clocks;
		int32 count;
	} profiler;
	TECHNOLOGYREADINESSLEVEL trl; // technology readiness level (1-9, 0=unknown)
	bool has_runtime;	///< flag indicating that a runtime dll, so, or dylib is in use
	char runtime[1024]; ///< name of file containing runtime dll, so, or dylib
	CLASS *next;
};

typedef char FULLNAME[1024]; /** Full object name (including space name) */
typedef struct s_namespace {
	FULLNAME name;
	struct s_namespace *next;
} NAMESPACE; ///< Namespaces are used to disambiguate class and object names

typedef struct s_forecast {
	char1024 specification; /**< forecast specification (see forecasting docs for details) */
	PROPERTY *propref; /**< property the forecast relates to */
	int n_values; /**< number of values in the forecast */
	TIMESTAMP starttime; /**< the start time of the forecast */
	int32 timestep; /**< number of seconds per forecast timestep */
	double *values; /**< values of the forecast (NULL if no forecast) */
	TIMESTAMP (*external)(void *obj, void *fc); /**< external forecast update call */
	struct s_forecast *next; /**< next forecast data block (NULL for last) */
} FORECAST; /**< Forecast data block */

typedef enum {
	OPI_PRESYNC,
	OPI_SYNC,
	OPI_POSTSYNC,
	OPI_INIT,
	OPI_HEARTBEAT,
	OPI_PRECOMMIT,
	OPI_COMMIT,
	OPI_FINALIZE,
	/* add profile items here */
	_OPI_NUMITEMS,
} OBJECTPROFILEITEM;
struct s_object_list {
	OBJECTNUM id; /**< object id number; globally unique */
	CLASS *oclass; /**< object class; determine structure of object data */
	OBJECTNAME name;
	char32 groupid;
	OBJECT *next; /**< next object in list */
	OBJECT *parent; /**< object's parent; determines rank */
	OBJECTRANK rank; /**< object's rank */
	TIMESTAMP clock; /**< object's private clock */
	TIMESTAMP valid_to;	/**< object's valid-until time */
	TIMESTAMP schedule_skew; /**< time skew applied to schedule operations involving this object */
	FORECAST *forecast; /**< forecast data block */
	double latitude, longitude; /**< object's geo-coordinates */
	TIMESTAMP in_svc, /**< time at which object begin's operating */
		out_svc; /**< time at which object ceases operating */
	unsigned int in_svc_micro,	/**< Microsecond portion of in_svc */
		out_svc_micro;	/**< Microsecond portion of out_svc */
	double in_svc_double;	/**< Double value representation of in service time */
	double out_svc_double;	/**< Double value representation of out of service time */
	clock_t synctime[_OPI_NUMITEMS]; /**< total time used by this object */
	NAMESPACE *space; /**< namespace of object */
	unsigned int lock; /**< object lock */
	unsigned int rng_state; /**< random number generator state */
	TIMESTAMP heartbeat; /**< heartbeat call interval (in sim-seconds) */
	uint32 flags; /**< object flags */
	/* IMPORTANT: flags must be last */
}; /**< Object header structure */

struct s_function_map {
	CLASS *oclass; ///< class to which this function applies
	FUNCTIONNAME name; ///< the name of the function
	FUNCTIONADDR addr; ///< the call address of the function
	FUNCTION *next; ///< the next function in the function map
}; ///< The function map structure

typedef struct s_datetime { ///< The s_datetime structure
	unsigned short year; /**< year (1970 to 2970 is allowed) */
	unsigned short month; /**< month (1-12) */
	unsigned short day; /**< day (1 to 28/29/30/31) */
	unsigned short hour; /**< hour (0-23) */
	unsigned short minute; /**< minute (0-59) */
	unsigned short second; /**< second (0-59) */
	unsigned int nanosecond; /**< nsecond (0-999999999) */
	unsigned short is_dst; /**< 0=std, 1=dst */
	char tz[5]; /**< ptr to tzspec timezone id */
	unsigned short weekday; /**< 0=Sunday */
	unsigned short yearday; /**< 0=Jan 1 */
	TIMESTAMP timestamp; /**< GMT timestamp */
	int tzoffset; /**< time zone offset in seconds (-43200 - 43200) */
} DATETIME; ///< A typedef for struct s_datetime

struct s_schedule {
	char name[64];						/**< the name of the schedule */
	char definition[65536];				/**< the definition string of the schedule */
	char blockname[MAXBLOCKS][64];		/**< the name of each block */
	unsigned char block;				/**< the last block used (4 max) */
	unsigned char index[14][366*24*60];	/**< the schedule index (enough room for all 14 annual calendars to 1 minute resolution) */
	uint32 dtnext[14][366*24*60];/**< the time until the next schedule change (in minutes) */
	double data[MAXBLOCKS*MAXVALUES];	/**< the list of values used in each block */
	unsigned int weight[MAXBLOCKS*MAXVALUES];	/**< the weight (in minutes) associate with each value */
	double sum[MAXBLOCKS];				/**< the sum of values for each block -- used to normalize */
	double abs[MAXBLOCKS];				/**< the sum of the absolute values for each block -- used to normalize */
	unsigned int count[MAXBLOCKS];		/**< the number of values given in each block */
	unsigned int minutes[MAXBLOCKS];	/**< the total number of minutes associate with each block */
	TIMESTAMP next_t;					/**< the time of the next schedule event */
	double value;						/**< the current scheduled value */
	double duration;					/**< the duration of the current scheduled value */
	int flags;							/**< the schedule flags (see SF_*) */
	SCHEDULE *next;	/* next schedule in list */
};

typedef enum {
	MT_UNKNOWN=0,
	MT_ANALOG,		/**< machine output an analog signal */
	MT_PULSED,		/**< machine outputs pulses of fixed area with varying frequency to match value */
	MT_MODULATED,	/**< machine outputs pulses of fixed frequency with varying area to match value */
	MT_QUEUED,		/**< machine accrues values and output pulses of fixed area with varying frequency */
} MACHINETYPE; /** type of machine */
typedef enum {
	MPT_UNKNOWN=0,
	MPT_TIME,		/**< pulses are of fixed time (either total period or on-time duration); power is energy/duration */
	MPT_POWER,		/**< pulses are of fixed power; duration is energy/power */
} MACHINEPULSETYPE; /**< the type of pulses generated by the machine */

struct s_loadshape_core {
	double load;		/**< the actual load magnitude */

	/* machine specification */
	SCHEDULE *schedule;	/**< the schedule driving this machine */
	MACHINETYPE type;	/**< the type of this machine */
	union {
		struct {
			double energy;		/**< the total energy used over the shape */
		} analog;
		struct {
			double energy;		/**< the total energy used over the shape */
			double scalar;		/**< the number of pulses over the shape */
			MACHINEPULSETYPE pulsetype;	/**< the fixed part of the pulse (time or power) */
			double pulsevalue;	/**< the value of the fixed part of the pulse */
		} pulsed;
		struct {
			double energy;		/**< the total energy used over the shape */
			double scalar;		/**< the number of pulses over the shape */
			MACHINEPULSETYPE pulsetype;	/**< the fixed part of the pulse (time or power) */
			double pulsevalue;	/**< the value of the fixed part of the pulse */
		} modulated;
		struct {
			double energy;		/**< the total energy used over the shape */
			double scalar;		/**< the number of pulses over the shape */
			MACHINEPULSETYPE pulsetype;	/**< the fixed part of the pulse (time or power) */
			double pulsevalue;	/**< the value of the fixed part of the pulse */
			double q_on, q_off;	/**< the queue thresholds (in units of 1 pulse) */
		} queued;
	} params;	/**< the machine parameters (depends on #type) */

	/* internal machine parameters */
	double r;			/**< the state rate */
	double re[2];		/**< the state rate stdevs (not used yet) */
	double d[2];		/**< the state transition thresholds */
	double de[2];		/**< the state transition threshold stdevs (not used yet) */
	double dPdV;		/**< the voltage sensitivity of the load */

	/* state variables */
	double q;			/**< the internal state of the machine */
	unsigned int s;		/**< the current state of the machine (0 or 1) */
	TIMESTAMP t0;	/**< time of last update (in seconds since epoch) */
	TIMESTAMP t2;	/**< time of next update (in seconds since epoch) */

	loadshape *next;	/* next loadshape in list */
};

struct s_enduse {
	loadshape *shape;
	complex power;				/* power in kW */
	complex energy;				/* total energy in kWh */
	complex demand;				/* maximum power in kW (can be reset) */
	double impedance_fraction;	/* constant impedance fraction (pu load) */
	double current_fraction;	/* constant current fraction (pu load) */
	double power_fraction;		/* constant power fraction (pu load)*/
	double power_factor;		/* power factor */
	double voltage_factor;		/* voltage factor (pu nominal) */
	double heatgain;			/* internal heat from load (Btu/h) */
	double heatgain_fraction;	/* fraction of power that goes to internal heat (pu Btu/h) */

	enduse *next;
};

/* object flags */
#define OF_NONE		0x0000 /**< Object flag; none set */
#define OF_HASPLC	0x0001 /**< Object flag; external PLC is attached, disables local PLC */
#define OF_LOCKED	0x0002 /**< Object flag; data write pending, reread recommended after lock clears */
#define OF_RECALC	0x0008 /**< Object flag; recalculation of derived values is needed */
#define OF_FOREIGN	0x0010 /**< Object flag; indicates that object was created in a DLL and memory cannot be freed by core */
#define OF_SKIPSAFE	0x0020 /**< Object flag; indicates that skipping updates is safe */
#define OF_FORECAST 0x0040 /**< Object flag; inidcates that the object has a valid forecast available */
#define OF_DEFERRED	0x0080	/**< Object flag; indicates that the object started to be initialized, but requested deferral */
#define OF_INIT		0x0100	/**< Object flag; indicates that the object has been successfully initialized */
#define OF_RERANK	0x4000 /**< Internal use only */

/******************************************************************************
 * Memory locking support
 */

#if defined(USE_RUNTIME_LOCKING)
#if defined(WIN32) && !defined(__GNUC__)
	#include <intrin.h>
	#pragma intrinsic(_InterlockedCompareExchange)
	#pragma intrinsic(_InterlockedIncrement)
	#define atomic_compare_and_swap(dest, comp, xchg) (_InterlockedCompareExchange((int32 *) dest, xchg, comp) == comp)
	#define atomic_increment(ptr) _InterlockedIncrement((int32 *) ptr)
	#ifndef inline
		#define inline __inline
	#endif
#elif __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1050
	#include <libkern/OSAtomic.h>
	#define atomic_compare_and_swap(dest, comp, xchg) OSAtomicCompareAndSwap32Barrier(comp, xchg, (int32_t *) dest)
	#define atomic_increment(ptr) OSAtomicIncrement32Barrier((int32_t *) ptr)
#else
	#define atomic_compare_and_swap __sync_bool_compare_and_swap
	#define atomic_increment(ptr) __sync_add_and_fetch(ptr, 1)
#endif


static inline void lock(unsigned int *lock)
{
	unsigned int value;

	do {
		value = *lock;
	} while ((value & 1) || !atomic_compare_and_swap(lock, value, value + 1));
}

static inline void unlock(unsigned int *lock)
{
	atomic_increment(lock);
}

#define LOCK(lock) lock(lock) /**< Locks an item */
#define UNLOCK(lock) unlock(lock) /**< Unlocks an item */
#define LOCK_OBJECT(obj) lock(&((obj)->lock)) /**< Locks an object */
#define UNLOCK_OBJECT(obj) unlock(&((obj)->lock)) /**< Unlocks an object */
#define LOCKED(obj,command) (LOCK_OBJECT(obj),(command),UNLOCK_OBJECT(obj))

#else

#define LOCK(lock)
#define UNLOCK(lock)
#define LOCK_OBJECT(obj)
#define UNLOCK_OBJECT(obj)
#define LOCKED(obj, command)

#endif

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

typedef struct s_callbacks {
	TIMESTAMP *global_clock;
	double *global_delta_curr_clock;
	TIMESTAMP *global_stoptime;
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
		OBJECT *(*foreign)(OBJECT *obj);
	} create;
	int (*define_map)(CLASS*,...);
	CLASS *(*class_getfirst)(void);
	CLASS *(*class_getname)(char*);
	struct {
		FUNCTION *(*define)(CLASS*,FUNCTIONNAME,FUNCTIONADDR);
		FUNCTIONADDR (*get)(char*,char*);
	} function;
	int (*define_enumeration_member)(CLASS*,char*,char*,enumeration);
	int (*define_set_member)(CLASS*,char*,char*,uint32);
	struct {
		OBJECT *(*get_first)(void);
		int (*set_dependent)(OBJECT*,OBJECT*);
		int (*set_parent)(OBJECT*,OBJECT*);
		int (*set_rank)(OBJECT*,unsigned int);
	} object;
	struct {
		PROPERTY *(*get_property)(OBJECT*,PROPERTYNAME,PROPERTYSTRUCT*);
		int (*set_value_by_addr)(OBJECT *, void*, char*,PROPERTY*);
		int (*get_value_by_addr)(OBJECT *, void*, char*, int size,PROPERTY*);
		int (*set_value_by_name)(OBJECT *, char*, char*);
		int (*get_value_by_name)(OBJECT *, char*, char*, int size);
		OBJECT *(*get_reference)(OBJECT *, char*);
		char *(*get_unit)(OBJECT *, char *);
		void *(*get_addr)(OBJECT *, char *);
		int (*set_value_by_type)(PROPERTYTYPE,void *data,char *);
		bool (*compare_basic)(PROPERTYTYPE ptype, PROPERTYCOMPAREOP op, void* x, void* a, void* b, char *part);
		PROPERTYCOMPAREOP (*get_compare_op)(PROPERTYTYPE ptype, char *opstr);
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
	struct {
		struct s_aggregate *(*create)(char *aggregator, char *group_expression);
		double (*refresh)(struct s_aggregate *aggregate);
	} aggregate;
	struct {
		void *(*getvar)(MODULE *module, char *varname);
		MODULE *(*getfirst)(void);
		int (*depends)(char *name, unsigned char major, unsigned char minor, unsigned short build);
	} module;
	struct {
		double (*uniform)(unsigned int *rng, double a, double b);
		double (*normal)(unsigned int *rng, double m, double s);
		double (*bernoulli)(unsigned int *rng, double p);
		double (*pareto)(unsigned int *rng, double m, double a);
		double (*lognormal)(unsigned int *rng, double m, double s);
		double (*sampled)(unsigned int *rng, unsigned int n, double *x);
		double (*exponential)(unsigned int *rng, double l);
		RANDOMTYPE (*type)(char *name);
		double (*value)(RANDOMTYPE type, ...);
		double (*pseudo)(RANDOMTYPE type, unsigned int *state, ...);
		double (*triangle)(unsigned int *rng, double a, double b);
		double (*beta)(unsigned int *rng, double a, double b);
		double (*gamma)(unsigned int *rng, double a);
		double (*weibull)(unsigned int *rng, double a, double b);
		double (*rayleigh)(unsigned int *rng, double a);
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
		TIMESTAMP (*convert_to_timestamp_delta)(const char *value, unsigned int *nanoseconds, double *dbl_time_value);
		int (*convert_from_timestamp)(TIMESTAMP ts, char *buffer, int size);
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
	struct {
		void (*read)(unsigned int *);
		void (*write)(unsigned int *);
	} lock, unlock;
	struct {
		char *(*find_file)(char *name, char *path, int mode);
	} file;
	struct s_objvar_struct {
		complex *(*complex_var)(OBJECT *obj, PROPERTY *prop);
		enumeration *(*enum_var)(OBJECT *obj, PROPERTY *prop);
		set *(*set_var)(OBJECT *obj, PROPERTY *prop);
		int16 *(*int16_var)(OBJECT *obj, PROPERTY *prop);
		int32 *(*int32_var)(OBJECT *obj, PROPERTY *prop);
		int64 *(*int64_var)(OBJECT *obj, PROPERTY *prop);
		double *(*double_var)(OBJECT *obj, PROPERTY *prop);
		char *(*string_var)(OBJECT *obj, PROPERTY *prop);
		OBJECT *(*object_var)(OBJECT *obj, PROPERTY *prop);
	} objvar;
	struct s_objvar_name_struct {
		complex *(*complex_var)(OBJECT *obj, char *name);
		enumeration *(*enum_var)(OBJECT *obj, char *name);
		set *(*set_var)(OBJECT *obj, char *name);
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
	int (*name_object)(OBJECT *obj, char *buffer, int len);
	int (*get_oflags)(KEYWORD **extflags);
	unsigned int (*object_count)(void);
	struct {
		SCHEDULE *(*create)(char *name, char *definition);
		SCHEDULEINDEX (*index)(SCHEDULE *sch, TIMESTAMP ts);
		double (*value)(SCHEDULE *sch, SCHEDULEINDEX index);
		int32 (*dtnext)(SCHEDULE *sch, SCHEDULEINDEX index);
		SCHEDULE *(*find)(char *name);
	} schedule;
	struct {
		int (*create)(loadshape *s);
		int (*init)(loadshape *s);
	} loadshape;
	struct {
		int (*create)(struct s_enduse *e);
		TIMESTAMP (*sync)(enduse *e, PASSCONFIG pass, TIMESTAMP t0, TIMESTAMP t1);
	} enduse;
	struct {
		double (*linear)(double t, double x0, double y0, double x1, double y1);
		double (*quadratic)(double t, double x0, double y0, double x1, double y1, double x2, double y2);
	} interpolate;
	struct {
		FORECAST *(*create)(OBJECT *obj, char *specs);
		FORECAST *(*find)(OBJECT *obj, char *name);
		double (*read)(FORECAST *fc, TIMESTAMP *ts);
		void (*save)(FORECAST *fc, TIMESTAMP *ts, int32 tstep, int n_values, double *data);
	} forecast;
	struct {
		void *(*readobj)(void *local, OBJECT *obj, PROPERTY *prop);
		void (*writeobj)(void *local, OBJECT *obj, PROPERTY *prop);
		void *(*readvar)(void *local, GLOBALVAR *var);
		void (*writevar)(void *local, GLOBALVAR *var);
	} remote;
	struct {
		struct s_objlist *(*create)(CLASS *oclass, PROPERTY *match_property, char *match_part, char *match_op, void *match_value1, void *match_value2);
		struct s_objlist *(*search)(char *group);
		void (*destroy)(struct s_objlist *list);
		size_t (*add)(struct s_objlist *list, PROPERTY *match_property, char *match_part, char *match_op, void *match_value1, void *match_value2);
		size_t (*del)(struct s_objlist *list, PROPERTY *match_property, char *match_part, char *match_op, void *match_value1, void *match_value2);
		size_t (*size)(struct s_objlist *list);
		struct s_object_list *(*get)(struct s_objlist *list,size_t n);
		int (*apply)(struct s_objlist *list, void *arg, int (*function)(struct s_object_list *,void *,int pos));
	} objlist;
	struct {
		struct {
			int (*to_string)(double v, char *buffer, size_t size);
			double (*from_string)(char *buffer);
		} latitude, longitude;
	} geography;
	struct {
		void (*read)(char *url, int maxlen);
		void (*free)(void *result);
	} http;
} CALLBACKS; /**< core callback function table */

extern CALLBACKS *callback;

typedef FUNCTIONADDR function;

#define gl_verbose (*callback->output_verbose) ///< Send a printf-style message to the verbose stream
#define gl_output (*callback->output_message) ///< Send a printf-style message to the output stream
#define gl_warning (*callback->output_warning) ///< Send a printf-style message to the warning stream
#define gl_error (*callback->output_error) ///< Send a printf-style message to the error stream
#ifdef _DEBUG
#define gl_debug (*callback->output_debug) ///< Send a printf-style message to the debug stream
#else
#define gl_debug
#endif
#define gl_testmsg (*callback->output_test) ///< Send a printf-style message to the testmsg stream

#define gl_globalclock (*(callback->global_clock)) ///< Get the current value of the global clock

/** Link to double precision deltamode clock (offset by global_clock) **/
#define gl_globaldeltaclock (*(callback->global_delta_curr_clock))

/** Link to stop time of the simulation **/
#define gl_globalstoptime (*(callback->global_stoptime))

/// Get the name of an object
/// @return a pointer to a static buffer containing the object's name
inline char* gl_name(OBJECT *my, char *buffer, size_t size)
{
	char temp[256];
	if(my == NULL || buffer == NULL) return NULL;
	if (my->name==NULL)
		sprintf(temp,"%s:%d", my->oclass->name, my->id);
	else
		sprintf(temp,"%s", my->name);
	if(size < strlen(temp))
		return NULL;
	strcpy(buffer, temp);
	return buffer;
}

/// Throw an exception using printf-style arguments
/// @return Does not return
inline void gl_throw(char *msg, ...) ///< printf-style argument list
{
	va_list ptr;
	va_start(ptr,msg);
	static char buffer[1024];
	vsprintf(buffer,msg,ptr);
	throw buffer;
	va_end(ptr);
}

/// Get the string value of global variable
/// @return A pointer to a static buffer containing the value
inline char *gl_global_getvar(char *name, char *buffer, int size) ///< pointer to string containing the name of the global variable
{
	return callback->global.getvar(name, buffer, size);
}

/// Create an object in the core
/// @return A pointer to the object
inline OBJECT *gl_create_object(CLASS *oclass) ///< a pointer to the class of the object to be created
{
	return (*callback->create.single)(oclass);
}

inline OBJECT *gl_create_foreign(OBJECT *obj) {return (*callback->create.foreign)(obj);};

/// Set the parent of an object
/// @return the rank of the object after parent is set
inline int gl_set_parent(OBJECT *obj, ///< the object whose parent is being set
						 OBJECT *parent) ///< the parent that is being set
{
	return (*callback->object.set_parent)(obj,parent);
}

/// Promote an object to a higher rank
/// @return the old rank of the object
inline int gl_set_rank(OBJECT* obj, ///< the object whose rank is being set
					   unsigned int rank) ///< the new rank of the object
{
	return (*callback->object.set_rank)(obj,rank);
}

/// Get a pointer to the data of an object property (by name)
/// @return a pointer to the data
inline void *gl_get_addr(OBJECT *obj, ///< the object whose property is sought
						 char *name) ///< the name of the property being sought
{
	return callback->properties.get_addr(obj,name);
}

/// Get the typed value of a property
/// @return nothing
template <class T> inline void gl_get_value(OBJECT *obj, ///< the object whose property value is being obtained
											char *propname, ///< the name of the property being obtained
											T &value) ///< a reference to the local value where the property's value is being copied
{
	T *ptr = (T*)gl_get_addr(obj,propname);
	char buffer[256];
	// @todo it would be a good idea to check the property type here
	if (ptr==NULL)
		gl_throw("property %s not found in object %s", propname, gl_name(obj, buffer, 255));
	value = *ptr;
}

/// Set the typed value of a property
/// @return nothing
template <class T> inline void gl_set_value(OBJECT *obj, ///< the object whose property value is being obtained
											char *propname, ///< the name of the property being obtained
											T &value) ///< a reference to the local value where the property's value is being copied
{
	T *ptr = (T*)gl_get_addr(obj,propname);
	char buffer[256];
	// @todo it would be a good idea to check the property type here
	if (ptr==NULL)
		gl_throw("property %s not found in object %s", propname, gl_name(obj, buffer, 255));
	*ptr = value;
}

inline FUNCTIONADDR gl_get_function(OBJECT *obj, char *fname) { return callback->function.get(obj->oclass->name,fname);};
inline FUNCTIONADDR gl_get_function(CLASS *oclass, char *fname) { return callback->function.get(oclass->name,fname);};
inline FUNCTIONADDR gl_get_function(char *classname, char *fname) { return callback->function.get(classname,fname);};

inline double gl_random_uniform(const double lo, const double hi) { return callback->random.uniform(NULL,lo,hi);};
inline double gl_random_normal(const double mu, const double sigma) { return callback->random.normal(NULL,mu,sigma);};
inline double gl_random_lognormal(const double gmu, const double gsigma) { return callback->random.lognormal(NULL,gmu,gsigma);};
inline double gl_random_exponential(const double lambda) { return callback->random.exponential(NULL,lambda);};
inline double gl_random_pareto(const double m, const double k) { return callback->random.pareto(NULL,m,k);};
inline double gl_random_bernoulli(double p) { return callback->random.bernoulli(NULL,p);};
inline double gl_random_sampled(unsigned int n, double *x) { return callback->random.sampled(NULL,n,x);};
inline double gl_random_triangle(double a, double b) { return callback->random.triangle(NULL,a,b);};
inline double gl_random_beta(double a, double b) { return callback->random.beta(NULL,a,b);};
inline double gl_random_gamma(double a) { return callback->random.gamma(NULL,a);};
inline double gl_random_weibull(double a, double b) { return callback->random.weibull(NULL,a,b);};
inline double gl_random_rayleigh(double a) { return callback->random.rayleigh(NULL,a);};

inline bool gl_object_isa(OBJECT *obj, char *type) { return callback->object_isa(obj,type)==1;};
inline DATETIME *gl_localtime(TIMESTAMP ts,DATETIME *dt) { return callback->time.local_datetime(ts,dt)?dt:NULL;};
inline TIMESTAMP gl_mkdatetime(DATETIME *dt) { return callback->time.mkdatetime(dt);};
inline TIMESTAMP gl_mkdatetime(short year, short month, short day, short hour=0, short minute=0, short second=0, char *tz=NULL, unsigned int nsec=0)
{
	DATETIME dt;
	dt.year = year;
	dt.month = month;
	dt.day = day;
	dt.hour = hour;
	dt.minute = minute;
	dt.second = second;
	dt.nanosecond = nsec;
	strncpy(dt.tz,tz,sizeof(dt.tz));
	return callback->time.mkdatetime(&dt);
};

inline int gl_unit_convert(char *from, char *to, double &value) { return callback->unit_convert(from, to, &value);};
inline int gl_unit_convert(UNIT *from, UNIT *to, double &value) { return callback->unit_convert_ex(from, to, &value);};
inline UNIT *gl_unit_find(char *name) { return callback->unit_find(name);};
inline char *gl_find_file(char *name, char *path, int mode) { return callback->file.find_file(name,path,mode); };

#define gl_printtime (*callback->time.convert_from_timestamp)

inline char *gl_strftime(DATETIME *dt, char *buffer, int size) { return callback->time.strdatetime(dt,buffer,size)?buffer:NULL;};
inline char *gl_strftime(TIMESTAMP ts, char *buffer, int size)
{
	//static char buffer[64];
	DATETIME dt;
	if(buffer == 0){
		callback->output_error("gl_strftime: buffer is a null pointer");
		return 0;
	}
	if(size < 15){
		callback->output_error("gl_strftime: buffer size is too small");
		return 0;
	}
	if(gl_localtime(ts,&dt)){
		return gl_strftime(&dt,buffer,size);
	} else {
		strncpy(buffer,"(invalid time)", size);
	}
	return buffer;
}

inline void trace(char *fn, OBJECT *obj)
{
	callback->output_message("TRACE: %s(%s[%s:%d])", fn, obj->name?obj->name:"<unnamed>", obj->oclass->name,obj->id);
}

inline void *gl_malloc(size_t size) { return callback->malloc(size);};
inline void gl_free(void *ptr) { return callback->free(ptr);};

#define gl_find_objects (*callback->find.object)
inline OBJECT *gl_find_next(struct s_findlist *list,OBJECT *obj) { return callback->find.next(list,obj);};
inline struct s_findlist *gl_find_copy(struct s_findlist *list) { return callback->find.copy(list);};
inline void gl_find_add(struct s_findlist *list, OBJECT *obj) { callback->find.add(list,obj);};
inline void gl_find_del(struct s_findlist *list, OBJECT *obj) { callback->find.del(list,obj);};
inline void gl_find_clear(struct s_findlist *list) { callback->find.clear(list);};

inline SCHEDULE *gl_schedule_create(char *name, char *definition)
{
	return callback->schedule.create(name,definition);
}

inline SCHEDULEINDEX gl_schedule_index(SCHEDULE *sch, TIMESTAMP ts)
{
	return callback->schedule.index(sch,ts);
}

inline double gl_schedule_value(SCHEDULE *sch, SCHEDULEINDEX index)
{
	return callback->schedule.value(sch,index);
}

inline int32 gl_schedule_dtnext(SCHEDULE *sch, SCHEDULEINDEX index)
{
	return callback->schedule.dtnext(sch,index);
}

inline TIMESTAMP gl_enduse_sync(enduse *e, TIMESTAMP t1)
{
	return callback->enduse.sync(e,PC_BOTTOMUP,*(callback->global_clock),t1);
}


/**@}**/
