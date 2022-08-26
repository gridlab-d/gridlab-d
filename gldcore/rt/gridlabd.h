/** $Id$
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
#include <math.h>

#ifdef _WINDOWS
#define isfinite _finite
#endif

# if __WORDSIZE == 64
#define int64 long int /**< standard version of 64-bit integers */
#else
#define int64 long long /**< standard version of 64-bit integers */
#endif

typedef enum {I='i',J='j',A='d', R='r'} CNOTATION; /**< complex number notation to use */
#define CNOTATION_DEFAULT J /* never set this to A */
#define PI 3.1415926535897932384626433832795
#define GLD_E 2.71828182845905

#define _NO_CPPUNIT

/* only cpp code may actually do complex math */
namespace gld {
    class complex {
    private:
        double r; /**< the real part */
        double i; /**< the imaginary part */
        CNOTATION f; /**< the default notation to use */
    public:
        /** Construct a complex number with zero magnitude */
        complex() /**< create a zero complex number */
        {
            r = 0;
            i = 0;
            f = CNOTATION_DEFAULT;
        };
        complex(double re) /**< create a complex number with only a real part */
        {
            r = re;
            i = 0;
            f = CNOTATION_DEFAULT;
        };
        complex(double re, double im, CNOTATION nf=CNOTATION_DEFAULT) /**< create a complex number with both real and imaginary parts */
        {
            f = nf;
            r = re;
            i = im;
        };

        /* assignment operations */
        complex &operator = (complex x) /**< complex assignment */
        {
            r = x.r;
            i = x.i;
            f = x.f;
            return *this;
        };
        complex &operator = (double x) /**< double assignment */
        {
            r = x;
            i = 0;
            f = CNOTATION_DEFAULT;
            return *this;
        };

        /* access operations */
        double & Re(void) /**< access to real part */
        {
            return r;
        };
        double & Im(void) /**< access to imaginary part */
        {
            return i;
        };
        CNOTATION & Notation(void) /**< access to notation */
        {
            return f;
        };
        double Mag(void) const /**< compute magnitude */
        {
            return sqrt(r*r+i*i);
        };
        double Mag(double m)  /**< set magnitude */
        {
            double old = sqrt(r*r+i*i);
            r *= m/old;
            i *= m/old;
            return m;
        };
        double Arg(void) const /**< compute angle */
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
        double Arg(double a)  /**< set angle */
        {
            SetPolar(Mag(),a,f);
            return a;
        };
        complex Log(void) const /**< compute log */
        {
            return complex(log(Mag()),Arg(),f);
        };
        void SetReal(double v) /**< set real part */
        {
            r = v;
        };
        void SetImag(double v) /**< set imaginary part */
        {
            i = v;
        };
        void SetNotation(CNOTATION nf) /**< set notation */
        {
            f = nf;
        }
        void SetRect(double rp, double ip, CNOTATION nf=CNOTATION_DEFAULT) /**< set rectangular value */
        {
            r = rp;
            i = ip;
            f = nf;
        };
        void SetPolar(double m, double a, CNOTATION nf=A) /**< set polar values */
        {
            r = (m*cos(a));
            i = (m*sin(a));
            f = nf;
        };

#if 0
        //operator const double (void) const /**< cast real part to double */
	//{
	//	return r;
	//};
#endif

        complex operator - (void) /**< change sign */
        {
            return complex(-r,-i,f);
        };
        complex operator ~ (void) /**< complex conjugate */
        {
            return complex(r,-i,f);
        };

        /* reflexive math operations */
        complex &operator += (double x) /**< add a double to the real part */
        {
            r += x;
            return *this;
        };
        complex &operator -= (double x) /**< subtract a double from the real part */
        {
            r -= x;
            return *this;
        };
        complex &operator *= (double x) /**< multiply a double to both parts */
        {
            r *= x;
            i *= x;
            return *this;
        };
        complex &operator /= (double x) /**< divide into both parts */
        {
            r /= x;
            i /= x;
            return *this;
        };
        complex &operator ^= (double x) /**< raise to a real power */
        {
            double lm = log(Mag()), a = Arg(), b = exp(x*lm), c = x*a;
            r = (b*cos(c));
            i = (b*sin(c));
            return *this;
        };
        complex &operator += (complex x) /**< add a complex number */
        {
            r += x.r;
            i += x.i;
            return *this;
        };
        complex &operator -= (complex x)  /**< subtract a complex number */
        {
            r -= x.r;
            i -= x.i;
            return *this;
        };
        complex &operator *= (complex x)  /**< multip[le by a complex number */
        {
            double pr=r;
            r = pr * x.r - i * x.i;
            i = pr * x.i + i * x.r;
            return *this;
        };
        complex &operator /= (complex y)  /**< divide by a complex number */
        {
            double xr=r;
            double a = y.r*y.r+y.i*y.i;
            r = (xr*y.r+i*y.i)/a;
            i = (i*y.r-xr*y.i)/a;
            return *this;
        };
        complex &operator ^= (complex x) /**< raise to a complex power */
        {
            double lm = log(Mag()), a = Arg(), b = exp(x.r*lm-x.i*a), c = x.r*a+x.i*lm;
            r = (b*cos(c));
            i = (b*sin(c));
            return *this;
        };

        /* binary math operations */
        complex operator + (double y) /**< double sum */
        {
            complex x(*this);
            return x+=y;
        };
        complex operator - (double y) /**< double subtract */
        {
            complex x(*this);
            return x-=y;
        };
        complex operator * (double y) /**< double multiply */
        {
            complex x(*this);
            return x*=y;
        };
        complex operator / (double y) /**< double divide */
        {
            complex x(*this);
            return x/=y;
        };
        complex operator ^ (double y) /**< double power */
        {
            complex x(*this);
            return x^=y;
        };
        complex operator + (complex y) /**< complex sum */
        {
            complex x(*this);
            return x+=y;
        };
        complex operator - (complex y) /**< complex subtract */
        {
            complex x(*this);
            return x-=y;
        };
        complex operator * (complex y) /**< complex multiply */
        {
            complex x(*this);
            return x*=y;
        };
        complex operator / (complex y) /**< complex divide */
        {
            complex x(*this);
            return x/=y;
        };
        complex operator ^ (complex y) /**< complex power */
        {
            complex x(*this);
            return x^=y;
        };

        /** set power factor */
        complex SetPowerFactor(double mag, /**< magnitude of power */
                               double pf, /**< power factor */
                               CNOTATION n=J) /** notation */
        {
            SetPolar(mag/pf, acos(pf),n);
            return *this;
        }


        /* comparison */
        bool IsZero(double err=0.0) /**< zero test */
        {
            return Mag()<=err;
        }

        /* magnitude comparisons */
        bool operator == (double m)	{ return Mag()==m; };
        bool operator != (double m)	{ return Mag()!=m; };
        bool operator < (double m)	{ return Mag()<m; };
        bool operator <= (double m)	{ return Mag()<=m; };
        bool operator > (double m)	{ return Mag()>m; };
        bool operator >= (double m)	{ return Mag()>=m; };

        /* angle comparisons */
        bool operator == (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)==0.0;};
        bool operator != (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)!=0.0;};
        bool operator < (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)<PI;};
        bool operator <= (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)<=PI;};
        bool operator > (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)>PI;};
        bool operator >= (complex y)	{ return fmod(y.Arg()-Arg(),2*PI)>=PI;};
        bool IsFinite(void) { return isfinite(r) && isfinite(i); };
    };
}
using gld::complex; // TODO: GLD Runtime compile does not properly handle namespaces.

#ifdef REAL4
typedef float real;
#else
typedef double real;
#endif

typedef int64 TIMESTAMP;
typedef int EXITCODE;
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
typedef gld::complex triplex[3];

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
};
using randomvar_struct = s_randomvar; // Correct behavior
using randomvar = s_randomvar; // behavior for GLM runtime compilation, which does not handle underscore

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
	PT_complex_array, /**< the data is a fixed length gld::complex[] */
/*	PT_object_array, */ /**< the data is a fixed length array of object pointers*/
	PT_float,	/**< Single-precision float	*/
	PT_real,	/**< Single or double precision float ~ allows double values to be overriden */
	PT_loadshape,	/**< Loadshapes are state machines driven by schedules */
	PT_enduse,		/**< Enduse load data */
	PT_random,		/**< Randomized number */
	PT_method,		/**< Method interface */
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

typedef int (*METHODCALL)(void *obj, char *string, int size); /**< the function that read and writes a string */

struct s_property_map {
	CLASS *oclass; /**< class implementing the property */
	PROPERTYNAME name; /**< property name */
	PROPERTYTYPE ptype; /**< property type */
	uint32 size; /**< property array size */
	uint32 width; /**< property byte size, copied from array in class.c */
	PROPERTYACCESS access; /**< property access flags */
	UNIT *unit; /**< property unit, if any; \p NULL if none */
	PROPERTYADDR addr; /**< property location, offset from OBJECT header; OBJECT header itself for methods */
	DELEGATEDTYPE *delegation; /**< property delegation, if any; \p NULL if none */
	KEYWORD *keywords; /**< keyword list, if any; \p NULL if none (only for set and enumeration types)*/
	char *description; /**< description of property */
	PROPERTY *next; /**< next property in property list */
	PROPERTYFLAGS flags; /**< property flags (e.g., PF_RECALC) */
	FUNCTIONADDR notify;
	METHODCALL method; /**< method call, addr must be 0 */
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

typedef struct s_loadmethod {
	char *name;
	int (*call)(void*,char*);
	struct s_loadmethod *next;
} LOADMETHOD;

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
	LOADMETHOD loadmethods;
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
	unsigned int child_count; /**< number of object that have this object as a parent */
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

typedef enum {
	EUMT_MOTOR_A, /**< 3ph induction motors driving constant torque loads */
	EUMT_MOTOR_B, /**< induction motors driving high inertia speed-squares torque loads */
	EUMT_MOTOR_C, /**< induction motors driving low inertia loads speed-squared torque loads */
	EUMT_MOTOR_D, /**< 1ph induction motors driving constant torque loads */
	_EUMT_COUNT, /* must be last */
} EUMOTORTYPE;
typedef enum {
    EUET_ELECTRONIC_A, /**< simple power electronics (no backfeed) */
    EUET_ELECTRONIC_B, /**< advanced power electronics (w/ backfeed) */
    _EUET_COUNT, /* must be last */
} EUELECTRONICTYPE;
typedef struct s_motor {
    gld::complex power;		/**< motor power when running */
    gld::complex impedance;	/**< motor impedance when stalled */
    double inertia;		/**< motor inertia in seconds */
    double v_stall;		/**< motor stall voltage (pu) */
    double v_start;		/**< motor start voltage (pu) */
    double v_trip;		/**< motor trip voltage (pu) */
    double t_trip;		/**< motor thermal trip time in seconds */
    /* TODO add slip data (0 for synchronous motors) */
} EUMOTOR;
typedef struct s_electronic {
    gld::complex power;		/**< load power when running */
    double inertia;		/**< load "inertia" */
    double v_trip;		/**< load "trip" voltage (pu) */
    double v_start;		/**< load "start" voltage (pu) */
} EUELECTRONIC;

typedef struct s_enduse {
	/* the output value must be first for transform to stream */
	/* meter values */
	gld::complex total;				/* total power in kW */
	gld::complex energy;				/* total energy in kWh */
	gld::complex demand;				/* maximum power in kW (can be reset) */

	/* circuit configuration */	
	set config;					/* end-use configuration */
	double breaker_amps;		/* breaker limit (if any) */

	/* zip values */
	gld::complex admittance;			/* constant impedance oprtion of load in kW */
	gld::complex current;			/* constant current portion of load in kW */
	gld::complex power;				/* constant power portion of load in kW */

	/* composite load data */
	EUMOTOR motor[_EUMT_COUNT];				/* motor loads (A-D) */
	EUELECTRONIC electronic[_EUET_COUNT];	/* electronic loads (S/D) */

	/* loading */
	double impedance_fraction;	/* constant impedance fraction (pu load) */
	double current_fraction;	/* constant current fraction (pu load) */
	double power_fraction;		/* constant power fraction (pu load)*/
	double power_factor;		/* power factor */
	double voltage_factor;		/* voltage factor (pu nominal) */

	/* heat */
	double heatgain;			/* internal heat from load (Btu/h) */
	double cumulative_heatgain;  /* internal cumulative heat gain from load (Btu) */ 
	double heatgain_fraction;	/* fraction of power that goes to internal heat (pu Btu/h) */

	/* misc info */
	char *name;
	loadshape *shape;
	TIMESTAMP t_last;			/* last time of update */

	// added for backward compatibility with res ENDUSELOAD
	// @todo these are obsolete and must be retrofitted with the above values
	struct s_object_list *end_obj;

        struct s_enduse *next;
#ifdef _DEBUG
    unsigned int magic;
#endif
} enduse;


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
#if defined(_WIN32) && !defined(__GNUC__)
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
	int (*string_to_data)(const char *,void*,PROPERTY*); /**< the function to convert from a string to data */
	int (*create)(void*); /**< the function used to create the property, if any */
	size_t (*stream)(FILE*,int,void*,PROPERTY*); /**< the function to read data from a stream */
	struct {
		PROPERTYCOMPAREOP op;
		char str[16];
		PROPERTYCOMPAREFUNCTION* fn;
		int trinary;
	} compare[_TCOP_LAST]; /**< the list of comparison operators available for this type */
	double (*get_part)(void*,char *name); /**< the function to get a part of a property */
	// @todo for greater generality this should be implemented as a linked list
} PROPERTYSPEC;

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
		};
	};
	struct s_transform *next; ///* next item in linked list
} TRANSFORM;

typedef class s_callbacks {
public:
    s_callbacks() throw();

    TIMESTAMP *global_clock;
    double *global_delta_curr_clock;
    TIMESTAMP *global_stoptime;
    EXITCODE *global_exit_code;
    int (*output_verbose)(const char *format, ...);
    int (*output_message)(const char *format, ...);
    int (*output_warning)(const char *format, ...);
    int (*output_error)(const char *format, ...);
    int (*output_fatal)(const char *format, ...);
    int (*output_debug)(const char *format, ...);
    int (*output_test)(const char *format, ...);
    CLASS *(*register_class)(MODULE *,const CLASSNAME,unsigned int,PASSCONFIG);
    struct {
        OBJECT *(*single)(CLASS*);
        OBJECT *(*array)(CLASS*,unsigned int);
        OBJECT *(*foreign)(OBJECT *);
    } create;
    int (*define_map)(CLASS*,...);
    int (*loadmethod)(CLASS*,const char*,int (*call)(void*,char*));
    CLASS *(*class_getfirst)(void);
    CLASS *(*class_getname)(const char*);
    PROPERTY *(*class_add_extended_property)(CLASS *,char *,PROPERTYTYPE,char *);
    struct {
        FUNCTION *(*define)(CLASS*,const FUNCTIONNAME,FUNCTIONADDR);
        FUNCTIONADDR (*get)(char*,const char*);
    } function;
    int (*define_enumeration_member)(CLASS*,const char*,const char*,enumeration);
    int (*define_set_member)(CLASS*,const char*,const char*,unsigned int64);
    struct {
        OBJECT *(*get_first)(void);
        int (*set_dependent)(OBJECT*,OBJECT*);
        int (*set_parent)(OBJECT*,OBJECT*);
        int (*set_rank)(OBJECT*, OBJECTRANK);
    } object;
    struct {
        PROPERTY *(*get_property)(OBJECT*,const PROPERTYNAME,PROPERTYSTRUCT*);
        int (*set_value_by_addr)(OBJECT *, void*, char*,PROPERTY*);
        int (*get_value_by_addr)(OBJECT *, void*, char*, int size,PROPERTY*);
        int (*set_value_by_name)(OBJECT *, char*, char*);
        int (*get_value_by_name)(OBJECT *, const char*, char*, int size);
        OBJECT *(*get_reference)(OBJECT *, char*);
        char *(*get_unit)(OBJECT *, const char *);
        void *(*get_addr)(OBJECT *, const char *);
        int (*set_value_by_type)(PROPERTYTYPE,void *data,char *);
        bool (*compare_basic)(PROPERTYTYPE ptype, PROPERTYCOMPAREOP op, void* x, void* a, void* b, char *part);
        PROPERTYCOMPAREOP (*get_compare_op)(PROPERTYTYPE ptype, char *opstr);
        double (*get_part)(OBJECT*,PROPERTY*,char*);
        PROPERTYSPEC *(*get_spec)(PROPERTYTYPE);
    } properties;
    struct {
        struct s_findlist *(*objects)(struct s_findlist *,...);
        OBJECT *(*next)(struct s_findlist *,OBJECT *obj);
        struct s_findlist *(*copy)(struct s_findlist *);
        void (*add)(struct s_findlist*, OBJECT*);
        void (*del)(struct s_findlist*, OBJECT*);
        void (*clear)(struct s_findlist*);
    } find;
    PROPERTY *(*find_property)(CLASS *, const PROPERTYNAME);
    void *(*malloc)(size_t);
    void (*free)(void*);
    struct {
        struct s_aggregate *(*create)(char *aggregator, char *group_expression);
        double (*refresh)(struct s_aggregate *aggregate);
    } aggregate;
    struct {
        double *(*getvar)(MODULE *module, const char *varname);
        MODULE *(*getfirst)(void);
        int (*depends)(const char *name, unsigned char major, unsigned char minor, unsigned short build);
        const char *(*find_transform_function)(TRANSFORMFUNCTION function);
    } module;
    struct {
        double (*uniform)(unsigned int *rng, double a, double b);
        double (*normal)(unsigned int *rng, double m, double s);
        double (*bernoulli)(unsigned int *rng, double p);
        double (*pareto)(unsigned int *rng, double m, double a);
        double (*lognormal)(unsigned int *rng,double m, double s);
        double (*sampled)(unsigned int *rng,unsigned int n, double *x);
        double (*exponential)(unsigned int *rng,double l);
        RANDOMTYPE (*type)(char *name);
        double (*value)(RANDOMTYPE type, ...);
        double (*pseudo)(RANDOMTYPE type, unsigned int *state, ...);
        double (*triangle)(unsigned int *rng,double a, double b);
        double (*beta)(unsigned int *rng,double a, double b);
        double (*gamma)(unsigned int *rng,double a, double b);
        double (*weibull)(unsigned int *rng,double a, double b);
        double (*rayleigh)(unsigned int *rng,double a);
    } random;
    int (*object_isa)(OBJECT *obj, const char *type);
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
        int (*local_datetime_delta)(double ts, DATETIME *dt);
        TIMESTAMP (*convert_to_timestamp)(const char *value);
        TIMESTAMP (*convert_to_timestamp_delta)(const char *value, unsigned int *microseconds, double *dbl_time_value);
        int (*convert_from_timestamp)(TIMESTAMP ts, char *buffer, int size);
        int (*convert_from_deltatime_timestamp)(double ts_v, char *buffer, int size);
    } time;
    int (*unit_convert)(const char *from, const char *to, double *value);
    int (*unit_convert_ex)(UNIT *pFrom, UNIT *pTo, double *pValue);
    UNIT *(*unit_find)(const char *unit_name);
    struct {
        EXCEPTIONHANDLER *(*create_exception_handler)();
        void (*delete_exception_handler)(EXCEPTIONHANDLER *ptr);
        void (*throw_exception)(const char *msg, ...);
        char *(*exception_msg)(void);
    } exception;
    struct {
        GLOBALVAR *(*create)(const char *name, ...);
        STATUS (*setvar)(const char *def,...);
        char *(*getvar)(const char *name, char *buffer, int size);
        GLOBALVAR *(*find)(const char *name);
    } global;
    struct {
        void (*read)(unsigned int *);
        void (*write)(unsigned int *);
    } lock, unlock;
    struct {
        char *(*find_file)(const char *name, char *path, int mode, char *buffer, int len);
    } file;
    struct s_objvar_struct {
        bool *(*bool_var)(OBJECT *obj, PROPERTY *prop);
        gld::complex *(*complex_var)(OBJECT *obj, PROPERTY *prop);
        enumeration *(*enum_var)(OBJECT *obj, PROPERTY *prop);
        set *(*set_var)(OBJECT *obj, PROPERTY *prop);
        int16 *(*int16_var)(OBJECT *obj, PROPERTY *prop);
        int32 *(*int32_var)(OBJECT *obj, PROPERTY *prop);
        int64 *(*int64_var)(OBJECT *obj, PROPERTY *prop);
        double *(*double_var)(OBJECT *obj, PROPERTY *prop);
        char *(*string_var)(OBJECT *obj, PROPERTY *prop);
        OBJECT **(*object_var)(OBJECT *obj, PROPERTY *prop);
    } objvar;
    struct s_objvar_name_struct {
        bool *(*bool_var)(OBJECT *obj, const char *name);
        gld::complex *(*complex_var)(OBJECT *obj, const char *name);
        enumeration *(*enum_var)(OBJECT *obj, const char *name);
        set *(*set_var)(OBJECT *obj, const char *name);
        int16 *(*int16_var)(OBJECT *obj, const char *name);
        int32 *(*int32_var)(OBJECT *obj, const char *name);
        int64 *(*int64_var)(OBJECT *obj, const char *name);
        double *(*double_var)(OBJECT *obj, const char *name);
        char *(*string_var)(OBJECT *obj, const char *name);
        OBJECT **(*object_var)(OBJECT *obj, const char *name);
    } objvarname;
    struct {
        int (*string_to_property)(PROPERTY *prop, void *addr, char *value);
        int (*property_to_string)(PROPERTY *prop, void *addr, char *value, int size);
    } convert;
    MODULE *(*module_find)(const char *name);
    OBJECT *(*get_object)(const char *name);
    OBJECT *(*object_find_by_id)(OBJECTNUM);
    int (*name_object)(OBJECT *obj, char *buffer, int len);
    int (*get_oflags)(KEYWORD **extflags);
    unsigned int (*object_count)(void);
    struct {
        SCHEDULE *(*create)(const char *name, const char *definition);
        SCHEDULEINDEX (*index)(SCHEDULE *sch, TIMESTAMP ts);
        double (*value)(SCHEDULE *sch, SCHEDULEINDEX index);
        int32 (*dtnext)(SCHEDULE *sch, SCHEDULEINDEX index);
        SCHEDULE *(*find)(const char *name);
        SCHEDULE *(*getfirst)(void);
    } schedule;
    struct {
        int (*create)(struct s_loadshape *s);
        int (*init)(struct s_loadshape *s);
    } loadshape;
    struct {
        int (*create)(struct s_enduse *e);
        TIMESTAMP (*sync)(struct s_enduse *e, PASSCONFIG pass, TIMESTAMP t1);
    } enduse;
    struct {
        double (*linear)(double t, double x0, double y0, double x1, double y1);
        double (*quadratic)(double t, double x0, double y0, double x1, double y1, double x2, double y2);
    } interpolate;
    struct {
        FORECAST *(*create)(OBJECT *obj, char *specs); /**< create a forecast using the specifications and append it to the object's forecast block */
        FORECAST *(*find)(OBJECT *obj, char *name); /**< find the forecast for the named property, if any */
        double (*read)(FORECAST *fc, TIMESTAMP ts); /**< read the forecast value for the time ts */
        void (*save)(FORECAST *fc, TIMESTAMP ts, int32 tstep, int n_values, double *data);
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
        void* (*read)(const char *url, int maxlen);
        void (*free)(void *result);
    } http;
    struct {
        TRANSFORM *(*getnext)(TRANSFORM*);
        int (*add_linear)(TRANSFORMSOURCE,double*,void*,double,double,OBJECT*,PROPERTY*,SCHEDULE*);
        int (*add_external)(OBJECT*,PROPERTY*,const char*,OBJECT*,PROPERTY*);
        int64 (*apply)(TIMESTAMP,TRANSFORM*,double*);
    } transform;
    struct {
        randomvar_struct *(*getnext)(randomvar_struct*);
        size_t (*getspec)(char *, size_t, const randomvar_struct *);
    } randomvar;
    struct {
        unsigned int (*major)(void);
        unsigned int (*minor)(void);
        unsigned int (*patch)(void);
        unsigned int (*build)(void);
        const char * (*branch)(void);
    } version;
    long unsigned int magic; /* used to check structure alignment */
} CALLBACKS; /**< core callback function table */

extern CALLBACKS *callback;

typedef FUNCTIONADDR function;

#define gl_verbose (*callback->output_verbose) ///< Send a printf-style message to the verbose stream
#define gl_output (*callback->output_message) ///< Send a printf-style message to the output stream
#define gl_warning (*callback->output_warning) ///< Send a printf-style message to the warning stream
#define gl_error (*callback->output_error) ///< Send a printf-style message to the error stream
#define gl_fatal (*callback->output_fatal) ///< Send a printf-style message to the error stream
#define gl_debug (*callback->output_debug) ///< Send a printf-style message to the debug stream
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
inline double gl_random_gamma(double a, double b) { return callback->random.gamma(NULL,a,b);};
inline double gl_random_weibull(double a, double b) { return callback->random.weibull(NULL,a,b);};
inline double gl_random_rayleigh(double a) { return callback->random.rayleigh(NULL,a);};

inline bool gl_object_isa(OBJECT *obj, char *type) { return callback->object_isa(obj,type)==1;};
inline DATETIME *gl_localtime(TIMESTAMP ts,DATETIME *dt) { return callback->time.local_datetime(ts,dt)?dt:NULL;};
inline DATETIME *gl_localtime_delta(double ts,DATETIME *dt) { return callback->time.local_datetime_delta(ts,dt)?dt:NULL;};
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
inline char *gl_find_file(const char *name, char *path, int mode, char* buffer, int len) { return callback->file.find_file(name,path,mode,buffer,len); };

#define gl_printtime (*callback->time.convert_from_timestamp)
#define gl_printtimedelta (*callback->time.convert_from_deltatime_timestamp)

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
	return callback->enduse.sync(e,PC_BOTTOMUP,t1); //*(callback->global_clock));
}

// DOUBLE ARRAY IMPLEMENTATION
#define BYREF 0x01
#include <math.h>
static int64 _qnan = 0xffffffffffffffffLL;
#define QNAN (*(double*)&_qnan)

class double_array;
class double_vector {
private:
	double **data;
public:
	double_vector(double **x) 
	{ 
		data=x; 
	};
	double &operator[] (const size_t n) 
	{ 
		if ( data[n]==NULL ) data[n]=new double; 
		return *data[n]; 
	};
	const double operator[] (const size_t n) const
	{
		if ( data[n]==NULL ) data[n]=new double; 
		return *data[n];
	}
};
class double_array {
private:
	size_t n, m; /** n rows, m cols */
	size_t max; /** current allocation size max x max */
	unsigned int *refs; /** reference count **/
	double ***x; /** pointer to 2D array of pointers to double values */
	unsigned char *f; /** pointer to array of flags: bit0=byref, */
	const char *name;
	friend class double_vector;
private:
	inline void exception(const char *msg,...) const
	{ 
		static char buf[1024]; 
		va_list ptr;
		va_start(ptr,msg);
		sprintf(buf,"%s", name?name:""); 
		vsprintf(buf+strlen(buf), msg, ptr); 
		throw (const char*)buf;
		va_end(ptr);
	};
	inline void set_rows(const size_t i) { n=i; };
	inline void set_cols(const size_t i) { m=i; };
	inline void set_flag(const size_t r, size_t c, const unsigned char b) {f[r*m+c]|=b;};
	inline void clr_flag(const size_t r, size_t c, const unsigned char b) {f[r*m+c]&=~b;};
	inline bool tst_flag(const size_t r, size_t c, const unsigned char b) const {return (f[r*m+c]&b)==b;};
	double &my(const size_t r, const size_t c) 
	{ 
		if ( x[r][c]==NULL ) x[r][c] = new double;
		return (*x[r][c]); 
	};
public:
	inline double_vector operator[] (const size_t n) { return double_vector(x[n]); }
	inline const double_vector operator[] (const size_t n) const { return double_vector(x[n]); }
	double_array(const size_t rows=0, const size_t cols=0, double **data=NULL)
	{
		refs = new unsigned int;
		*refs = 0;
		m = n = max = 0;
		x = NULL;
		f = NULL;
		if ( rows>0 )
			grow_to(rows,cols);
		for ( size_t r=0 ; r<rows ; r++ )
		{
			for ( size_t c=0 ; c<cols ; c++ )
			{
				set_at(r,c, ( data!=NULL ? data[r][c] : 0.0 ) );
			}
		}
	}
	double_array(const double_array &a)
	{
		n = a.n;
		m = a.m;
		max = a.max;
		refs = a.refs;
		x = a.x;
		f = a.f;
		name = a.name;
		(*refs)++;
	}
    ~double_array(void) {
        if (x != nullptr && (*refs)-- == 0) {
//            size_t r, c;
            if (n > 0)
                for (auto r = 0; r < n; r++) {
                    if (x[r] != nullptr) {
                        if (m > 0)
                            for (auto c = 0; c < m; c++) {
                                if (x[r][c] != nullptr && tst_flag(r, c, BYREF))
                                    free(x[r][c]);
                            }
                        free(x[r]);
                    }
                }
            free(x);
            delete refs;
        }
    }
public:
	void set_name(const char *v) { name = v; }; 
	inline const char *get_name(void) const { return name; };
	void copy_name(const char *v) { char *s=(char*)malloc(strlen(v)+1); strcpy(s,v); name=(const char*)s; };
	inline const size_t get_rows(void) const { return n; };
	inline const size_t get_cols(void) const { return m; };
	inline const size_t get_max(void) const { return max; };
	void set_max(const size_t size) 
	{
		if ( size<=max ) exception(".set_max(%u): cannot shrink double_array",size);
		size_t r;
		double ***z = (double***)malloc(sizeof(double**)*size);
		// create new rows
		for ( r=0 ; r<max ; r++ )
		{
			if ( x[r]!=NULL )
			{
				double **y = (double**)malloc(sizeof(double*)*size);
				if ( y==NULL ) exception(".set_max(%u): unable to expand double_array",size);
				memcpy(y,x[r],sizeof(double*)*max);
				memset(y+max,0,sizeof(double*)*(size-max));
				free(x[r]);
				z[r] = y;
			}
			else
				z[r] = NULL;
		}
		memset(z+max,0,sizeof(double**)*(size-max));
		free(x);
		x = z;
		unsigned char *nf = (unsigned char*)malloc(sizeof(unsigned char)*size);
		if ( f!=NULL )
		{
			memcpy(nf,f,max);
			memset(nf+max,0,size-max);
			free(f);
		}
		else
			memset(nf,0,size);
		f = nf;
		max=size; 
	};
	void grow_to(const size_t r, const size_t c) 
	{ 
		size_t s = (max<1?1:max);
		while ( c>=s || r>=s ) s*=2; 
		if ( s>max ) set_max(s);

		// add rows
		while ( n<r ) 
		{
			if ( x[n]==NULL ) 
			{
				x[n] = (double**)malloc(sizeof(double*)*max);
				memset(x[n],0,sizeof(double*)*max);
			}
			n++;
		}

		// add columns
		if ( m<c )
		{
			size_t i;
			for ( i=0 ; i<n ; i++ )
			{
				double **y = (double**)malloc(sizeof(double*)*c);
				if ( x[i]!=NULL )
				{
					memcpy(y,x[i],sizeof(double**)*m);
					free(x[i]);
				}
				memset(y+m,0,sizeof(double**)*(c-m));
				x[i] = y;
			}
			m=c;
		}
	};
	void grow_to(const size_t c) { grow_to(n>0?n:1,c); };
	void grow_to(const double_array &y) { grow_to(y.get_rows(),y.get_cols()); };
	void check_valid(const size_t r, const size_t c) const { if ( !is_valid(r,c) ) exception(".check_value(%u,%u): invalid (r,c)",r,c); };
	inline void check_valid(const size_t c) const { check_valid(0,c); };
	bool is_valid(const size_t r, const size_t c) const { return r<n && c<m; };
	inline bool is_valid(const size_t c) const { return is_valid(0,c); };
	bool is_nan(const size_t r, const size_t c)  const
	{
		check_valid(r,c);
		return ! ( x[r][c]!=NULL && isfinite(*(x[r][c])) ); 
	};
	inline bool is_nan(const size_t c) const { return is_nan(0,c); };
	bool is_empty(void) const { return n==0 && m==0; };
	void clr_at(const size_t r, const size_t c) 
	{ 
		check_valid(r,c);
		if ( tst_flag(r,c,BYREF) )
			free(x[r][c]); 
		x[r][c]=NULL; 
	};
	inline void clr_at(const size_t c) { return clr_at(0,c); };
	/// make a new matrix (row major)
	double **copy_matrix(void) 
	{   
		double **y = new double*[n];
		unsigned int r;
		for ( r=0 ; r<n ; r++ )
		{
			y[r] = new double[m];
			unsigned int c;
			for ( c=0 ; c<m ; c++ )
				y[r][c] = *(x[r][c]);
		}
		return y;               
	};
	/// free a matrix
	void free_matrix(double **y)
	{
		unsigned int r;
		for ( r=0 ; r<n ; r++ )
			delete [] y[r];
		delete [] y;
	};
	/// vector copy (row major)
	double *copy_vector(double *y=NULL)
	{
		if ( y==NULL ) y=new double[m*n];
		unsigned i=0;
		unsigned int r, c;
		for ( r=0 ; r<n ; r++ )
		{
			for ( c=0 ; c<m ; c++ )
				y[i++] = *(x[r][c]);
		}
		return y;
	}
	void transpose(void) {
		double ***xt = new double**[n];
		size_t i;
		for ( i=0 ; i<m ; i++ )
		{
			xt[i] = new double*[n];
			size_t j;
			for ( j=0 ; j<n ; j++ )
				xt[i][j] = x[j][i];
		}
		for ( i=0 ; i<n ; i++ )
			delete [] x[i];
		delete [] x;
		x = xt;
		size_t t=m; m=n; n=t;
	};
	inline double *get_addr(const size_t r, const size_t c) { return x[r][c]; };
	inline double *get_addr(const size_t c) { return get_addr(0,c); };
	double get_at(const size_t r, const size_t c) { return is_nan(r,c) ? QNAN : *(x[r][c]) ; };
	inline double get_at(const size_t c) { return get_at(0,c); };
	inline double &get(const size_t r, const size_t c) { return *x[r][c]; };
	inline double &get(const size_t c) { return get(0,c); };
	inline void set_at(const size_t c, const double v) { set_at(0,c,v); };
	void set_at(const size_t r, const size_t c, const double v) 
	{ 
		check_valid(r,c);
		if ( x[r][c]==NULL ) 
			x[r][c]=(double*)malloc(sizeof(double)); 
		*(x[r][c]) = v; 
	};
	inline void set_at(const size_t c, double *v) { set_at(0,c,v); };
	void set_at(const size_t r, const size_t c, double *v) 
	{ 
		check_valid(r,c);
		if ( v==NULL ) 
		{
			if ( x[r][c]!=NULL ) 
				clr_at(r,c);
		}
		else 
		{
			set_flag(r,c,BYREF);
			x[r][c] = v; 
		}
	};
	void set_ident(void)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) = (r==c) ? 1 : 0;
		}
	};
	void dump(size_t r1=0, size_t r2=-1, size_t c1=0, size_t c2=-1)
	{
		if ( r2==-1 ) r2 = n-1;
		if ( c2==-1 ) c2 = m-1;
		if ( r2<r1 || c2<c1 ) exception(".dump(%u,%u,%u,%u): invalid (r,c)", r1,r2,c1,c2);
		size_t r,c;
		fprintf(stderr,"double_array %s = {\n",name?name:"unnamed"); 
		for ( r=r1 ; r<=n ; r++ )
		{
			for ( c=c1 ; c<=m ; c++ )
				fprintf(stderr," %8g", my(r,c));
			fprintf(stderr,"\n");
		}
		fprintf(stderr," }\n");
	}
	void operator= (const double y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) = y;
		}
	};
	double_array &operator= (const double_array &y)
	{
		size_t r,c;
		grow_to(y);
		for ( r=0 ; r<y.get_rows() ; r++ )
		{
			for ( c=0 ; c<y.get_cols() ; c++ )
				my(r,c) = y[r][c];
		}
		return *this;
	};
	double_array &operator+= (const double &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) += y;
		}
		return *this;
	}
	double_array &operator+= (const double_array &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) += y[r][c];
		}
		return *this;
	};
	double_array &operator-= (const double &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) -= y;
		}
		return *this;
	};
	double_array &operator-= (const double_array &y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) -= y[r][c];
		}
		return *this;
	};
	double_array &operator *= (const double y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) *= y;
		}
		return *this;
	};
	double_array &operator /= (const double y)
	{
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<get_cols() ; c++ )
				my(r,c) /= y;
		}
		return *this;
	};
	// binary operators
	double_array operator+ (const double y)
	{
		double_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) + y;
		return a;
	}
	double_array operator- (const double y)
	{
		double_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) - y;
		return a;
	}
	double_array operator* (const double y)
	{
		double_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) * y;
		return a;
	}
	double_array operator/ (const double y)
	{
		double_array a(get_rows(),get_cols());
		size_t r,c;
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<get_cols() ; c++ )
				a[r][c] = my(r,c) / y;
		return a;
	}
	double_array operator + (const double_array &y)
	{
		size_t r,c;
		if ( get_rows()!=y.get_rows() || get_cols()!=y.get_cols() )
			exception("+%s: size mismatch",y.get_name());
		double_array a(get_rows(),get_cols());
		a.set_name("(?+?)");
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<y.get_cols() ; c++ )
				a[r][c] = my(r,c) + y[r][c];
		return a;
	};
	double_array operator - (const double_array &y)
	{
		size_t r,c;
		if ( get_rows()!=y.get_rows() || get_cols()!=y.get_cols() )
			exception("-%s: size mismatch",y.get_name());
		double_array a(get_rows(),get_cols());
		a.set_name("(?-?)");
		for ( r=0 ; r<get_rows() ; r++ )
			for ( c=0 ; c<y.get_cols() ; c++ )
				a[r][c] = my(r,c) - y[r][c];
		return a;
	};
	double_array operator * (const double_array &y)
	{
		size_t r,c,k;
		if ( get_cols()!=y.get_rows() )
			exception("*%s: size mismatch",y.get_name());
		double_array a(get_rows(),y.get_cols());
		a.set_name("(?*?)");
		for ( r=0 ; r<get_rows() ; r++ )
		{
			for ( c=0 ; c<y.get_cols() ; c++ )
			{	
				double b = 0;
				for ( k=0 ; k<get_cols() ; k++ )
					b += my(r,k) * y[k][c];
				a[r][c] = b;
			}
		}
		return a;
	};
	void extract_row(double*y,const size_t r)
	{
		size_t c;
		for ( c=0 ; c<m ; c++ )
			y[c] = my(r,c);
	}
	void extract_col(double*y,const size_t c)
	{
		size_t r;
		for ( r=0 ; r<n ; r++ )
			y[r] = my(r,c);
	}
};
///////////////////////////////////////////////////////////////////////////////////
// Extended runtime API for 3.0
#define GLAPI3 // enable gl class to support extended API
class gl_core {
private:
	OBJECT *my;
public:
	inline gl_core(OBJECT*obj) : my(obj) {};
public:
	inline void error(char *fmt,...)
	{
		char buffer[1024];
		char tmp[256];
		va_list ptr;
		va_start(ptr,fmt);
		vsprintf(buffer,fmt,ptr);
		(*callback->output_error)("%s: %s", gl_name(my,tmp,sizeof(tmp)),buffer);
		va_end(ptr);
	};
	inline void warning(char *fmt,...)
	{
		char buffer[1024];
		char tmp[256];
		va_list ptr;
		va_start(ptr,fmt);
		vsprintf(buffer,fmt,ptr);
		(*callback->output_warning)("%s: %s", gl_name(my,tmp,sizeof(tmp)),buffer);
		va_end(ptr);
	};};
/**@}**/
