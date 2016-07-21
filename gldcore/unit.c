/** $Id: unit.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file unit.c
	@addtogroup unit Unit management
	@ingroup core
	
	The unit manager converts values from one unit to another.

	The unit manager reads the file \p unitfile.txt. The file loaded 
	is the first found according the following search sequence:
	-	The current directory
	-	The same directory as the executable \p gridlabd.exe
	-	The path in the \p GLPATH environment variable.

	Units are based on 6 physical/economic constants:
	@verbatim
	#c=2.997925e8 ; speed of light (m/s)
	#e=1.602189246e-19 ; electron charge (C)
	#h=6.62617636e-24 ; Plank's constant (kg.m.m/s)
	#k=1.38066244e-13 ; Boltzman's constant (kg.m.m/s.s.K)
	#m=9.10953447e-31 ; electron rest mass (kg)
	#s=1.233270e4 ; average price of gold in 1990 ($/kg - 383.59$/oz)
	@endverbatim

	From these are derived all the fundamental SI units:
	@verbatim
	m=-1,0,1,0,-1,0,4.121487e01,0,7
	kg=0,0,0,0,1,0,1.09775094e30,0,10
	s=-2,0,1,0,-1,0,1.235591e10,0,7
	A=2,1,-1,0,1,0,5.051397e08,0,7
	K=2,0,0,-1,1,0,1.686358e00,0,7
	cd=4,0,-1,0,2,0,1.447328E+00,0,7
	@endverbatim

	An addition currency unit is defined because it's really useful:
	@verbatim
	1990$=0,0,0,0,1,1,1.097751e30,0,7
	@endverbatim

	Based on these units, all the other units are derived using simple formulae, e.g.:
	@verbatim
	; Length
	cm=0.01 m
	mm=0.001 m
	km=1000 m
	in=2.54 cm
	ft=12 in
	yd=3 ft
	mile=5280 ft
	@endverbatim

	@section Theory of Operation

	The theory behind the unit management system is based on the fact that every physical quantity
	can be expressed in terms of a linear combination of exponents of the five basic physical constants
	(to which a currency constant is added to allow cost-based units).  This is best understood by
	considering the equation

	\f[ quantity = value * unit \f]

	where

	\f[ unit = c^k_c * e^k_e * h^k_h * k^k_k * m^k_m * s^k_s \f]

	where c, e, h, k, m, and s are the constants defined above, and the k terms are the unit terms defined for that unit.

	Thus, two values can only be added (or subtracted) only if their units are the same (i.e., unit is a common factor), 
	and when two values are multiplied (or divided), the exponential terms of the units are added (or subtracted).

	For example, 1 second is defined as 

	\f[ 1.235591x10^{10} h/mc^2 \f]

 @{
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#include "platform.h"
#include "output.h"
#include "unit.h"
#include "exception.h"
#include "globals.h"
#include "find.h"
#include "lock.h"

/* fundamental physical/economic constants */
static double c = 2.997925e8;		/**< m/s */
static double e = 1.602189246e-19;	/**< C */
static double h = 6.62617636e-34;	/**< J/Hz */
static double k = 1.38066244e-23;	/**< J/K */
static double m = 0.910953447e-30;	/**< kg */
static double s = 1.233270e4;		/**< 1990$/kgAu */
static int precision = 10;			/**, 10 digits max precision */
static UNIT *unit_list = NULL;
static char filepath[1024] = "unitfile.txt";
static int linenum = 0;

typedef struct s_unitscalar UNITSCALAR;
struct s_unitscalar {
	char name[3];
	unsigned char len;
	int scalar;
	UNITSCALAR *next;
};
UNITSCALAR *scalar_list = NULL;

/* unit_find_raw is both used to detect the presence and absence of units in the list.
	not finding a given unit is normal behavior, and this function should run silently.
*/
UNIT *unit_find_raw(char *unit){
	UNIT *p;
	/* scan list for existing entry */
	for (p = unit_list; p != NULL; p = p->next){
		if (strcmp(p->name,unit) == 0){
			return p;
		}
	}
	return NULL;
}
UNIT *unit_primary(char *name,double c,double e,double h,double k,double m,double s,double a,double b,int prec);

/* locate an underived unit */
UNIT *unit_find_underived(char *unit)
{
	UNIT *p;
	UNITSCALAR *s;

	/* scan list for existing entry */
	p = unit_find_raw(unit);
	if(p != NULL){
		/* success, quick out */
		return p;
	}

	/* check to see if scalar is being used */
	for(s = scalar_list; s != NULL; s = s->next){
		if(strncmp(unit, s->name, s->len) == 0){
			break;
		}
	}
	if(s == NULL){ /* normal behavior */
		return NULL;
	}

	/* scan for base unit */
	p = unit_find_raw(unit+s->len);
	if(p == NULL){
		output_error("compound unit '%s' could not be correctly identified", unit);
		/*	TROUBLESHOOT
			The given unit had a recognized scalar prefix, but the underlying base unit was not identified.   This
			will cascade a null pointer to the calling method.
		*/
		return NULL;
	}

	/* add as new derived unit */
	return unit_primary(unit, p->c, p->e, p->h, p->k, p->m, p->s, p->a * pow(10, s->scalar), p->b, p->prec);
}

/* define a unit scalar */
int unit_scalar(char *name,int scalar)
{
	UNITSCALAR *ptr = (UNITSCALAR *)malloc(sizeof(UNITSCALAR));
	if (ptr == NULL){
		throw_exception("%s(%d): scalar definitinon of '%s' failed: %s", filepath, linenum, name, strerror(errno));
		/*	TROUBLESHOOT
			This is actually triggered by a malloc failure, and the rest of the system is likely unstable.
		*/
	}
	strncpy(ptr->name, name, sizeof(ptr->name));
	ptr->scalar = scalar;
	ptr->len = (unsigned char)strlen(ptr->name);
	ptr->next = scalar_list;
	scalar_list = ptr;
	return 1;
}


/* define a constant */
int unit_constant(char name, double value)
{
	char buffer[2] = {name,'\0'};
	if(unit_find_underived(buffer) != NULL){
		throw_exception("%s(%d): constant definition of '%s' failed; unit already defined", filepath, linenum, buffer);
		/*	TROUBLESHOOT
			The value of the specified scalar name has already been used as a unit and cannot be used as a constant.  Please remove or comment
			out the offending line from the unit file.
		*/
	}
	switch (name) {
		case 'c':
			c=value;
			break;
		case 'e':
			e=value;
			break;
		case 'h':
			h=value;
			break;
		case 'k':
			k=value;
			break;
		case 'm':
			m=value;
			break;
		case 's':
			s=value;
			break;
		default:
			throw_exception("%s(%d): constant '%c' is not valid", filepath,linenum,name);
			/*	TROUBLESHOOT
				The specified constant name does not conform to the internally recognized constants.
			*/
			return 0;
	}
	return 1;
}

/* define a primary unit */
UNIT *unit_primary(char *name, double c, double e, double h, double k, double m, double s, double a, double b, int prec){
	UNIT *p = unit_find_raw(name);
#if 0
	){
		throw_exception("raw unit '%s' was not found", name);
		/*	TROUBLESHOOT
			The requested unit was not found by the unit subsystem.  There is a good chance that this function will
			end up triggering a null pointer exception if not properly handled.
		*/
	}
	if
#endif
	if(p != NULL && (c != p->c || e != p->e || h != p->h || k != p->k || m != p->m || s != p->s || a != p->a || b != p->b || prec != p->prec)){
		throw_exception("%s(%d): primary definition of '%s' failed; unit already defined with a different parameter set", filepath, linenum, name);
		/*	TROUBLESHOOT
			Since the specified unit was defined, the values of the universal constants OR the global precision have
			changed.  No units should be created until after all of the constants have been specified, nor should those
			constants be modified between unit definitions.
		*/
	}
	p = (UNIT *)malloc(sizeof(UNIT));
	if(p == NULL)
	{
		throw_exception("%s(%d): memory allocation failed",filepath,linenum);
		/* TROUBLESHOOT
			The unit conversion system was unable to allocate memory needed to define the unit located at the specified file at the given line.
			Try freeing up system memory and try again.
		 */
		return 0;
	}
	strncpy(p->name, name, sizeof(p->name) - 1);
	p->c = c;
	p->e = e;
	p->h = h;
	p->k = k;
	p->m = m;
	p->s = s;
	p->a = a;
	p->b = b;
	p->prec = prec;
	p->next = unit_list;
	unit_list = p;
	return p;
}

/* determine the precision of a term */
int unit_precision(char *term)
{
	char* p;
	char mant[256];

	/* get mantissa */
	if ((p=strchr(term,'e')) != NULL || (p=strchr(term,'E')) != NULL) {

		/* has exponent */
		strncpy(mant, term, p-term);
	} else {
		strcpy(mant, term);
	}

	return (int)(strlen(mant) - (strchr(mant,'.') != NULL ? 1 : 0));
}

/* define a derived unit */
int unit_derived(char *name,char *derivation)
{
	double c = 0, e = 0, h = 0, k = 0, m = 0, s = 0, a = 0, b = 0;
	int prec = 0;
	double scalar = 1.0;
	char lastOp = '\0', nextOp = '\0';
	UNIT *lastUnit = NULL;
	UNIT local;
	char *p = derivation;
	
	if (unit_find_raw(name) != NULL){
		throw_exception("%s(%d): derived definition of '%s' failed; unit already defined", filepath, linenum, name);
		/*	TROUBLESHOOT
			The specified derived unit has already been defined within the unit file.  Please review the unit definition file and remove the offending line.
		*/
	}
	
	/* extract scalar */
	if (sscanf(p, "%lf", &scalar) == 1){
		/* advance pointer */
		if ((p = strchr(p,' ')) != NULL){
			p++;
		}
		/* reset pointer */
		else {
			p = derivation;
		}
	}

	/* scan for terms */
	while (*p != '\0')
	{
		char term[32];
		UNIT *pUnit;

		/* extract operation */
		if (sscanf(p,"%[^-*/^+]",term)!=1){
			throw_exception("%s(%d): unable to read unit '%s'", filepath,linenum,p);
			/*	TROUBLESHOOT
				The unit definition contains a character that prevents it from being parsed.  No unit definitions should contain
				the characters ^, -, *, /, or +.
			*/
		}

		/* non-exponential ops */
		if (nextOp != '^'){
			/* find unit */
			pUnit = unit_find_underived(term);

			if (pUnit == NULL){
				double val;
				if (sscanf(term,"%lf",&val) == 1){
					if (nextOp =='+' || nextOp == '-'){ /* bias operation */
						UNIT bias = {"", 0, 0, 0, 0, 0, 0, 0, val, unit_precision(term)};
						local = bias;
					} else { /* scale operation */
						UNIT offset = {"", 0, 0, 0, 0, 0, 0, val, 0, unit_precision(term)};
						local = offset;
					}
					pUnit = &local;
				} else {
					throw_exception("%s(%d): unable to find or derive unit '%s'", filepath, linenum, p);
					/*	TROUBLESHOOT
						The specified term was neither found nor derived, and likely does not exist in the unit subsystem at
						the point the exception is raised.  Review the unit file and either move the offending unit down the
						file, or verify that the base unit exists.
					*/
				}
			}
		}

		/* advance to next term */
		p += strlen(term);

		/* add this term to result */
		switch(nextOp){
			case '\0':
				/* first term */
				if (a == 0)
				{
					c = pUnit->c;
					e = pUnit->e;
					h = pUnit->h;
					k = pUnit->k;
					m = pUnit->m;
					s = pUnit->s;
					a = pUnit->a * scalar;
					b = pUnit->b;
					prec = pUnit->prec;
					nextOp = '*'; /* in case ^ follows first term */
				} /* else last term */
				break;
			case '*':
				c += pUnit->c;
				e += pUnit->e;
				h += pUnit->h;
				k += pUnit->k;
				m += pUnit->m;
				s += pUnit->s;
				a *= pUnit->a;
				prec = min(prec,pUnit->prec);
				break;
			case '/':
				c -= pUnit->c;
				e -= pUnit->e;
				h -= pUnit->h;
				k -= pUnit->k;
				m -= pUnit->m;
				s -= pUnit->s;
				a /= pUnit->a;
				prec = min(prec, pUnit->prec);
				break;
			case '+':
				b += pUnit->b;
				break;
			case '-':
				b -= pUnit->b;
				break;
			case '^':
				if (lastUnit!=NULL){
					int repeat = 0;
					if (sscanf(term, "%d", &repeat) != 1 || repeat < 2){
						throw_exception("%s(%d): syntax error at '%s', exponent must be greater than 1", filepath,linenum,term);
						/*	TROUBLESHOOT
							The unit was defined akin to "m^2", but the exponent was an integer value less than 1,
							which is nonsensical for a physical simulation
						*/
					}
					repeat--;
					switch(lastOp) {
						case '*':
							c += lastUnit->c * repeat;
							e += lastUnit->e * repeat;
							h += lastUnit->h * repeat;
							k += lastUnit->k * repeat;
							m += lastUnit->m * repeat;
							s += lastUnit->s * repeat;
							a *= pow(lastUnit->a, repeat);
							prec = min(prec, lastUnit->prec);
							break;
						case '/':
							c -= lastUnit->c * repeat;
							e -= lastUnit->e * repeat;
							h -= lastUnit->h * repeat;
							k -= lastUnit->k * repeat;
							m -= lastUnit->m * repeat;
							s -= lastUnit->s * repeat;
							a /= pow(lastUnit->a, repeat);
							prec = min(prec, lastUnit->prec);
							break;
						default:
							throw_exception("%s(%d): ^ not allowed after '%c' at '%s'", filepath, linenum, lastOp, term);
							/* TROUBLESHOOT
								The unit file used an invalid syntax to define a unit.  Correct the syntax and try again.
							 */
							return 0;
					}
				} // endif (lastUnit != NULL)
				break;
			default:
				throw_exception("%s(%d): '%c' is not recognized at '%s'", filepath,linenum,lastOp,term);
				/* TROUBLESHOOT
					The unit file used an invalid syntax to define a unit.  Correct the syntax and try again.
				 */
				return 0;
				break;
			// endswitch nextOp
		}
		lastOp = nextOp;
		if((nextOp = *p) != '\0'){
			p++;
		}
		lastUnit = pUnit;
	}

	return (unit_primary(name, c, e, h, k, m, s, a, b, prec) != NULL);
}

/** Initialize the unit manager 
 **/
void unit_init(void)
{
	static int tried=0, trylock=0;
	char *glpath = getenv("GLPATH");
	FILE *fp = NULL;
	char tpath[1024];

	/* try only once */
	wlock(&trylock);
	if (tried)
	{
		wunlock(&trylock);
		return;
	}
	else
		tried = 1;
	
	if(find_file(filepath, NULL, R_OK, tpath,sizeof(tpath)) != NULL)
		fp = fopen(tpath, "r");

	/* locate unit file on GLPATH if not found locally */
	if (fp == NULL && glpath != NULL){
		char envbuf[1024];
		char *dir;
		strcpy(envbuf, glpath);
		dir = strtok(envbuf, ";");
		while(dir != NULL){
			strcpy(filepath, dir);
			strcat(filepath, "/unitfile.txt");
			fp = fopen(filepath, "r");
			if (fp != NULL){
				break;
			}
			dir = strtok(NULL, ";");
		}
	}

	/* unit file not found */
	if (fp == NULL)
	{
		output_error("%s: %s (GLPATH=%s)", filepath, strerror(errno), glpath);
		/*	TROUBLESHOOT
			The unit subsystem was not able to locate the unit file in the working directoy or in the directories
			specified in GLPATH.
		*/
		wunlock(&trylock);
		return;
	}

	/* scan file */
	while (!feof(fp) && !ferror(fp))
	{
		char buffer[1024], *p;
		if (fgets(buffer, sizeof(buffer), fp) == NULL){
			break;
		}
		linenum++;

		/* wipe comments */
		p = strchr(buffer,';');
		if (p != NULL){
			*p = '\0';
		}

		/* remove trailing whitespace */
		p = buffer + strlen(buffer) - 1;
		while (iswspace(*p) && p>buffer){
			*p-- = '\0';
		}

		/* ignore blank lines or lines starting with white space*/
		if (buffer[0] == '\0' || iswspace(buffer[0])){
			continue;
		}
		
		/* constant definition */
		if (buffer[0] == '#') {
			double value = 0.0;
			char name;
			if (sscanf(buffer + 1,"%c=%lf", &name, &value) != 2){
				output_error("%s(%d): constant definition '%s' is not valid", filepath, linenum, buffer);
			} else {
				unit_constant(name, value);
			}
			continue;
		}

		/* scalar definition */
		if (buffer[0] == '*'){
			char name[8];
			int scalar;
			if (sscanf(buffer+1, "%7[^=]=%d", name, &scalar) == 2){
				unit_scalar(name, scalar);
			} else {
				output_error("%s(%d): scalar '%s' is not valid", filepath, linenum, buffer);
			}
			continue;
		}

		/* primary definition */
		{
			char name[256];
			double c, e, h, k, m, s, a, b;
			int prec;
			if (sscanf(buffer, "%[^=]=%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d",name, &c, &e, &h, &k, &m, &s, &a, &b, &prec) == 10){
				unit_primary(name, c, e, h, k, m, s, a, b, prec);
				continue;
			}
		}

		/* derived definition */
		{
			char name[256];
			char derivation[256];
			if (sscanf(buffer, "%[^=]=%[-+/*^0-9. %A-Za-z]", name, derivation) == 2){
				unit_derived(name, derivation);
				continue;
			}
		}

		/* not recognized */
		throw_exception("%s(%d): unit '%s' is not recognized", filepath, linenum, buffer);
		/*	TROUBLESHOOT
			The specified unit was not recognized as a constant, a primary unit, or a derived unit.
		*/
	}

	/* check status */
	if (ferror(fp)){
		throw_exception("unitfile: %s", strerror(errno));
		/*	TROUBLESHOOT
			An error occured while reading the unit file.
		*/
	} else {
		output_verbose("%s loaded ok", filepath);
	}

	/* done */
	fclose(fp);
	
	wunlock(&trylock);
	return;
}

/** Convert a value from one unit to another
	@return 1 if successful, 0 if failed
 **/
int unit_convert(char *from, char *to, double *pValue)
{
	if (strcmp(from,to)==0)
		return 1;
	else
	{
		UNIT *pFrom = unit_find(from);
		UNIT *pTo = unit_find(to);
		if (pFrom != NULL){
			if(pTo != NULL){
				return unit_convert_ex(pFrom, pTo, pValue);
			} else {
				output_error("could not find 'to' unit %s for unit_convert", to);
				/*	TROUBLESHOOT
					The specified unit name was not found by the unit system.  Verify that it is a valid unit name.
				*/
			}
		} else {
			output_error("could not find 'from' unit %s for unit_convert", from);
			/*	TROUBLESHOOT
				The specified unit name was not found by the unit system.  Verify that it is a valid unit name.
			*/
		}
		return 0;
	}
}

/** Convert a value from one unit to another
	@return 1 if successful, 0 if failed
 **/
int unit_convert_ex(UNIT *pFrom, UNIT *pTo, double *pValue)
{
	if(pFrom == NULL || pTo == NULL || pValue == NULL){
		output_error("could not run unit_convert_ex due to null arguement");
		/*	TROUBLESHOOT
			An error occured earlier in processing that caused a null pointer to be used as an arguement.  Review
			other error messages for details, but either a property was not found, or a unit definition was not
			found.
		*/
		return 0;
	}
	if (pTo->c == pFrom->c && pTo->e == pFrom->e && pTo->h == pFrom->h && pTo->k == pFrom->k && pTo->m == pFrom->m && pTo->s == pFrom->s)
	{
		*pValue = (*pValue - pFrom->b) * (pFrom->a / pTo->a) + pTo->b;
		return 1;
	} else {
		output_error("could not convert units from %s to %s, mismatched constant values", pFrom->name, pTo->name);
		return 0;
	}
}

/** Convert a complex value from one unit to another
	@return 1 if successful, 0 if failed
 **/
int unit_convert_complex(UNIT *pFrom, UNIT *pTo, complex *pValue)
{
	if(pFrom == NULL || pTo == NULL || pValue == NULL){
		output_error("could not run unit_convert_complex due to null arguement");
		/*	TROUBLESHOOT
			An error occured earlier in processing that caused a null pointer to be used as an arguement.  Review
			other error messages for details, but either a property was not found, or a unit definition was not
			found.
		*/
		return 0;
	}
	
	if (pTo->c == pFrom->c && pTo->e == pFrom->e && pTo->h == pFrom->h && pTo->k == pFrom->k && pTo->m == pFrom->m && pTo->s == pFrom->s){
		pValue->r = (pValue->r - pFrom->b) * (pFrom->a / pTo->a) + pTo->b;
		pValue->i = (pValue->i - pFrom->b) * (pFrom->a / pTo->a) + pTo->b;
		return 1;
	} else {
		output_error("could not convert units from %s to %s, mismatched constant values", pFrom->name, pTo->name);
		return 0;
	}
}

/** Find a unit
	@return a pointer to the UNIT structure
 **/
UNIT *unit_find(char *unit) /**< the name of the unit */
{
	UNIT *p;
	int rv = 0;

	TRY {
		/* first time */
		if (unit_list==NULL) unit_init();
	} CATCH (char *msg) {
		output_error("unit_find(char *unit='%s'): %s", unit,msg);
	}
	ENDCATCH;

	/* scan list for existing entry */
	p = unit_find_raw(unit);
	if (p != NULL){
		return p;
	}
	
	/* derive entry if possible */
	TRY {
		rv = unit_derived(unit, unit);
	} CATCH (char *msg) {
		output_error("unit_find(char *unit='%s'): %s", unit,msg);
	}
	ENDCATCH;

	//if (unit_derived(unit,unit)){
	if(rv){
		return unit_list;
	} else {
		output_error("could not find unit \'%s\'", unit);
		return NULL;
	}
}

/** Test the unit manager
	@return the number of failed tests
 **/
int unit_test(void)
{
	typedef struct {double value; char *unit;} VALUE;
	typedef struct {VALUE from; VALUE to; double precision;} TEST;
#ifndef PI
#define PI 3.141592635
#endif
	TEST test[] = {
		{1, "ratio",	1, "unit"},
		{1, "%",		0.01, "unit"},
		{1, "pu",		1, "1/unit"},
		{1, "/%",		1, "1/%"},

		{PI, "unit",	3.141592635, "unit"},
		{2*PI, "rad",	1, "unit"},
		{360, "deg",	1, "unit"},
		{400, "grad",	1, "unit"},
		{4, "quad",		1, "unit"},
		{4*PI, "sr",	1, "unit"},

		{1, "R",		5.0/9.0, "K", 0.001},
		{0, "degC",		273.14, "K", 0.001},
		{0, "degF",		459.65, "R", 0.001},
		{212, "degF",		100.0, "degC", 0.01},
		{32, "degF",		0.0, "degC", 0.01},
		{20, "degC",		68.0, "degF", 0.01},
		{77, "degF",		25.0, "degC", 0.01},
		{1, "kg",		1000, "g"},
		{1, "N",		1, "m*kg/s^2"},
		{1, "J",		1, "N*m"},
		
		{1, "min",		60,"s"},
		{1, "h",		60,"min"},
		{1, "day",		24,"h"},
		{1, "yr",		365,"day"},
		{1, "syr",		365.24,"day"},

		{1, "cm",		0.01, "m"},
		{1, "mm",		0.1, "cm"},
		{1, "km",		1000,"m"},
		{1, "in",		2.54, "cm"},
		{1, "ft",		12, "in"},
		{1, "yd",		3, "ft"},
		{1, "mile",		5280, "ft"},

		{1, "sf",		1,"ft^2"},
		{1, "sy",		1,"yd^2"},

		{1, "cf",		1,"ft^3"},
		{1, "cy",		1,"yd^3"},
		{1, "l",		0.001,"m^3"},
		{1, "gal",		3.785412, "l"},

		{2.2046, "lb",	1, "kg"},
		{1, "tonne",	1000, "kg"},
		
		{1, "mph",		1, "mile/h"},
		{1, "fps",		1, "ft/s"},
		{1, "fpm",		1, "ft/min"},
		{1, "gps",		1, "gal/s"},
		{1, "gpm",		1, "gal/min"},
		{1, "gph",		1, "gal/h"},
		{1, "cfm",		1, "ft^3/min"},
		{1, "ach",		1, "1/h"},
		
		{1, "Hz",		1, "1/s"},
		
		{1, "W",		1, "J/s"},
		{1, "kW",		1000, "W"},
		{1, "MW",		1000, "kW"},
		{1, "Wh",		1, "W*h"},
		{1, "kWh",		1000, "Wh"},
		{1, "MWh",		1000, "kWh"},
		{1, "Btu",		0.293, "Wh"},
		{1, "kBtu",		1000, "Btu"},
		{1, "MBtu",		1000, "kBtu"},
		{1, "ton",		12000, "Btu/h"},
		{1, "tons",		1, "ton*s"},
		{1, "tonh",		1, "ton*h"},
		{1, "hp",		746, "W"},
		{1, "W",		1, "V*A"},
		{1, "C",		1, "A*s"},
		{1, "F",		1, "C/V"},
		{1, "Ohm",		1,"V/A"},
		{1, "H",		1,"Ohm*s"},
	};
	int n, failed = 0, succeeded = 0;
	output_test("\nBEGIN: units tests");
	for (n = 0; n < sizeof(test)/sizeof(test[0]); n++){
		double v = test[n].from.value;
		if (test[n].precision == 0)
			test[n].precision = 1e-4;

		/* forward test */
		if (!unit_convert(test[n].from.unit, test[n].to.unit, &v)) {
			output_test("FAILED: conversion from %s to %s not possible", test[n].from.unit,test[n].to.unit);
			failed++;
		} else if (fabs(v-test[n].to.value) > test[n].precision) {
			output_test("FAILED: incorrect unit conversion %g %s -> %g %s (got %g %s instead)", 
				test[n].from.value,test[n].from.unit,
				test[n].to.value,test[n].to.unit,
				v, test[n].to.unit);
			failed++;
		} else if (!unit_convert(test[n].to.unit,test[n].from.unit, &v)) { 	/* reverse test */
			output_test("FAILED: conversion from %s to %s not possible", test[n].to.unit, test[n].from.unit);
			failed++;
		} else if (fabs(v - test[n].from.value) > test[n].precision) {
			output_test("FAILED: incorrect unit conversion %g %s -> %g %s (got %g %s instead)", 
				test[n].to.value,test[n].to.unit,
				test[n].from.value,test[n].from.unit,
				v, test[n].from.unit);
			failed++;
		} else {
			output_test("SUCCESS: %g %s = %g %s", 				
					test[n].from.value, test[n].from.unit,
					test[n].to.value, test[n].to.unit);
			succeeded++;
		}
	}
	output_test("END: %d units tested", n);
	output_verbose("units tested: %d ok, %d failed (see '%s' for details).", succeeded, failed, global_testoutputfile);
	return failed;
}

/** @} **/
