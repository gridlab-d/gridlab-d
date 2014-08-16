/** $Id: property.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2011 Battelle Memorial Institute
	@file property.c
	@addtogroup property Properties of objects
	@ingroup core
	
	GridLAB-D classes contain properties,
	which are supported by the functions in this module

 @{
 **/

#include <math.h>

#include "class.h"
#include "output.h"
#include "convert.h"
#include "module.h"
#include "exception.h"
#include "timestamp.h"
#include "loadshape.h"
#include "enduse.h"
#include "stream.h"
#include "instance.h"
#include "linkage.h"
#include "compare.h"
#include "stream.h"
#include "exec.h"

/* IMPORTANT: this list must match PROPERTYTYPE enum in property.h */
PROPERTYSPEC property_type[_PT_LAST] = {
	{"void", "string", 0, 0, convert_from_void,convert_to_void},
	{"double", "decimal", sizeof(double), 24, convert_from_double,convert_to_double,NULL,stream_double,{TCOPS(double)},},
	{"complex", "string", sizeof(complex), 48, convert_from_complex,convert_to_complex,NULL,NULL,{TCOPS(double)},complex_get_part},
	{"enumeration", "string", sizeof(int32), 32, convert_from_enumeration,convert_to_enumeration,NULL,NULL,{TCOPS(uint64)},},
	{"set", "string", sizeof(int64), 32, convert_from_set,convert_to_set,NULL,NULL,{TCOPS(uint64)},},
	{"int16", "integer", sizeof(int16), 6, convert_from_int16,convert_to_int16,NULL,NULL,{TCOPS(uint16)},},
	{"int32", "integer", sizeof(int32), 12, convert_from_int32,convert_to_int32,NULL,NULL,{TCOPS(uint32)},},
	{"int64", "integer", sizeof(int64), 24, convert_from_int64,convert_to_int64,NULL,NULL,{TCOPS(uint64)},},
	{"char8", "string", sizeof(char8), 8, convert_from_char8,convert_to_char8,NULL,NULL,{TCOPS(string)},},
	{"char32", "string", sizeof(char32), 32, convert_from_char32,convert_to_char32,NULL,NULL,{TCOPS(string)},},
	{"char256", "string", sizeof(char256), 256, convert_from_char256,convert_to_char256,NULL,NULL,{TCOPS(string)},},
	{"char1024", "string", sizeof(char1024), 1024, convert_from_char1024,convert_to_char1024,NULL,NULL,{TCOPS(string)},},
	{"object", "string", sizeof(OBJECT*), sizeof(OBJECTNAME), convert_from_object,convert_to_object,NULL,NULL,{TCOPB(object)},object_get_part},
	{"delegated", "string", (unsigned int)-1, 0, convert_from_delegated, convert_to_delegated},
	{"bool", "string", sizeof(bool), 6, convert_from_boolean, convert_to_boolean,NULL,NULL,{TCOPB(bool)},},
	{"timestamp", "string", sizeof(int64), 24, convert_from_timestamp_stub, convert_to_timestamp_stub,NULL,NULL,{TCOPS(uint64)},timestamp_get_part},
	{"double_array", "string", sizeof(double_array), 0, convert_from_double_array, convert_to_double_array,double_array_create,NULL,{TCNONE},double_array_get_part},
	{"complex_array", "string", sizeof(complex_array), 0, convert_from_complex_array, convert_to_complex_array,complex_array_create,NULL,{TCNONE},complex_array_get_part},
	{"real", "decimal", sizeof(real), 24, convert_from_real, convert_to_real},
	{"float", "decimal", sizeof(float), 24, convert_from_float, convert_to_float},
	{"loadshape", "string", sizeof(loadshape), 0, convert_from_loadshape, convert_to_loadshape, loadshape_create,NULL,{TCOPS(double)},},
	{"enduse", "string", sizeof(enduse), 0, convert_from_enduse, convert_to_enduse, enduse_create,NULL,{TCOPS(double)},enduse_get_part},
	{"randomvar", "string", sizeof(randomvar), 24, convert_from_randomvar, convert_to_randomvar, randomvar_create,NULL,{TCOPS(double)},random_get_part},
};

PROPERTYSPEC *property_getspec(PROPERTYTYPE ptype)
{
	return &(property_type[ptype]);
}

/** Check whether the properties as defined are mapping safely to memory
    @return 0 on failure, 1 on success
 **/
int property_check(void)
{
	PROPERTYTYPE ptype;
	int status = 1;
	for ( ptype=_PT_FIRST+1 ; ptype<_PT_LAST ; ptype++ )
	{
		size_t sz = 0;
		switch (ptype) {
		case PT_double: sz = sizeof(double); break;
		case PT_complex: sz = sizeof(complex); break;
		case PT_enumeration: sz = sizeof(enumeration); break;
		case PT_set: sz = sizeof(set); break;
		case PT_int16: sz = sizeof(int16); break;
		case PT_int32: sz = sizeof(int32); break;
		case PT_int64: sz = sizeof(int64); break;
		case PT_char8: sz = sizeof(char8); break;
		case PT_char32: sz = sizeof(char32); break;
		case PT_char256: sz = sizeof(char256); break;
		case PT_char1024: sz = sizeof(char1024); break;
		case PT_object: sz = sizeof(OBJECT*); break;
		case PT_bool: sz = sizeof(bool); break;
		case PT_timestamp: sz = sizeof(TIMESTAMP); break;
		case PT_double_array: sz = sizeof(double_array); break;
		case PT_complex_array: sz = sizeof(complex_array); break;
		case PT_real: sz = sizeof(real); break;
		case PT_float: sz = sizeof(float); break;
		case PT_loadshape: sz = sizeof(loadshape); break;
		case PT_enduse: sz = sizeof(enduse); break;
		case PT_random: sz = sizeof(randomvar); break;
		default: break;
		}
		output_verbose("property_check of %s: declared size is %d, actual size is %d", property_type[ptype].name, property_type[ptype].size, sz);
		if ( sz>0 && property_type[ptype].size<sz )
		{
			status = 0;
			output_error("declared size of property %s smaller than actual size in memory on this platform (declared %d, actual %d)", property_type[ptype].name, property_type[ptype].size, sz);
		}
		else if ( sz>0 && property_type[ptype].size!=sz )
		{
			output_warning("declared size of property %s does not match actual size in memory on this platform (declared %d, actual %d)", property_type[ptype].name, property_type[ptype].size, sz);
		}
	}
	return status;
}

PROPERTY *property_malloc(PROPERTYTYPE proptype, CLASS *oclass, char *name, void *addr, DELEGATEDTYPE *delegation)
{
	char unitspec[1024];
	PROPERTY *prop = (PROPERTY*)malloc(sizeof(PROPERTY));

	if (prop==NULL)
	{
		output_error("property_malloc(oclass='%s',...): memory allocation failed", oclass->name, name);
		/*	TROUBLESHOOT
			This means that the system has run out of memory while trying to define a class.  Trying freeing
			up some memory by unloading applications or configuring your system so it has more memory.
		 */
		errno = ENOMEM;
		goto Error;
	}
	memset(prop, 0, sizeof(PROPERTY));
	prop->ptype = proptype;
	prop->size = 0;
	prop->width = property_type[proptype].size;
	prop->access = PA_PUBLIC;
	prop->oclass = oclass;
	prop->flags = 0;
	prop->keywords = NULL;
	prop->description = NULL;
	prop->unit = NULL;
	prop->notify = 0;
	prop->notify_override = false;
	if (sscanf(name,"%[^[][%[^]]]",prop->name,unitspec)==2)
	{
		/* detect when a unit is associated with non-double/complex property */
		if (prop->ptype!=PT_double && prop->ptype!=PT_complex)
			output_error("property_malloc(oclass='%s',...): property %s cannot have unit '%s' because it is not a double or complex value",oclass->name, prop->name,unitspec);
			/*	TROUBLESHOOT
				Only <b>double</b> and <b>complex</b> properties can have units.  
				Either change the type of the property or remove the unit specification from the property's declaration.
			 */

		/* verify that the requested unit exists or can be derived */
		else 
		{
			if ((prop->unit = unit_find(unitspec))==NULL)
				throw_exception("property_malloc(oclass='%s',...): property %s unit '%s' is not recognized",oclass->name, prop->name,unitspec);
				/*	TROUBLESHOOT
					A class is attempting to publish a variable using a unit that is not defined.  
					This is caused by an incorrect unit specification in a variable publication (in C++) or declaration (in GLM).
					Units are defined in the unit file located in the GridLAB-D <b>etc</b> folder.  
				 */
		}
	}
	prop->addr = addr;
	prop->delegation = delegation;
	prop->next = NULL;

	/* check for already existing property by same name */
	if (oclass!=NULL && class_find_property(oclass,prop->name))
		output_warning("property_malloc(oclass='%s',...): property name '%s' is defined more than once", oclass->name, prop->name);
		/*	TROUBLESHOOT
			A class is attempting to publish a variable more than once.  
			This is caused by an repeated specification for a variable publication (in C++) or declaration (in GLM).
		 */
	return prop;
Error:
	free(prop);
	return NULL;
}

/** Get the size of a single instance of a property
	@return the size in bytes of the a property
 **/
uint32 property_size(PROPERTY *prop)
{
	if (prop && prop->ptype>_PT_FIRST && prop->ptype<_PT_LAST)
		return property_type[prop->ptype].size;
	else
		return 0;
}

uint32 property_size_by_type(PROPERTYTYPE type)
{
	return property_type[type].size;
}

int property_create(PROPERTY *prop, void *addr)
{
	if (prop && prop->ptype>_PT_FIRST && prop->ptype<_PT_LAST)
	{
		if (property_type[prop->ptype].create)
			return property_type[prop->ptype].create(addr);
		//memset(addr,0,(prop->size==0?1:prop->size)*property_type[prop->ptype].size);
		memset(addr,0,property_type[prop->ptype].size);
		return 1;
	}
	else
		return 0;
}

size_t property_minimum_buffersize(PROPERTY *prop)
{
	size_t size = property_type[prop->ptype].csize;
	if ( size>0 )
		return size;
	switch (prop->ptype) {
	/* @todo dynamic sizing */
	default:
		return 0;
	}
	return 0;
}

PROPERTYCOMPAREOP property_compare_op(PROPERTYTYPE ptype, char *opstr)
{
	int n;
	for ( n=0; n<_TCOP_LAST; n++)
		if (strcmp(property_type[ptype].compare[n].str,opstr)==0)
			return n;
	return TCOP_ERR;
}


bool property_compare_basic(PROPERTYTYPE ptype, PROPERTYCOMPAREOP op, void *x, void *a, void *b, char *part)
{
	if ( part==NULL && property_type[ptype].compare[op].fn!=NULL )
		return property_type[ptype].compare[op].fn(x,a,b);
	else if ( property_type[ptype].get_part!=NULL )
	{
		double d = property_type[ptype].get_part ? property_type[ptype].get_part(x,part) : QNAN ;
		if ( isfinite(d) )
			// parts always double (for now)
			return property_type[PT_double].compare[op].fn((void*)&d,a,b);
		else
		{
			output_error("part %s is not defined for type %s", part, property_type[ptype].name);
			return 0;
		}
	}
	else // no comparison possible
	{
		output_error("property type '%s' does not support comparison operations or parts", property_type[ptype].name);
		return 0;
	}
}

PROPERTYTYPE property_get_type(char *name)
{
	PROPERTYTYPE ptype;
	for ( ptype = _PT_FIRST+1 ; ptype<_PT_LAST ; ptype++ )
	{
		if ( strcmp(property_type[ptype].name,name)==0)
			return ptype;
	}
	return PT_void;
}

double property_get_part(OBJECT *obj, PROPERTY *prop, char *part)
{
	PROPERTYSPEC *spec = property_getspec(prop->ptype);
	if ( spec && spec->get_part )
	{
		return spec->get_part(GETADDR(obj,prop),part);
	}
	else
		return QNAN;
}

/*********************************************************
 * PROPERTY PARTS
 *********************************************************/
double complex_get_part(void *x, char *name)
{
	complex *c = (complex*)x;
	if ( strcmp(name,"real")==0) return c->r;
	if ( strcmp(name,"imag")==0) return c->i;
	if ( strcmp(name,"mag")==0) return complex_get_mag(*c);
	if ( strcmp(name,"arg")==0) return complex_get_arg(*c);
	if ( strcmp(name,"ang")==0) return (complex_get_arg(*c)*180/PI);
	return QNAN;
}

/*********************************************************
 * DOUBLE ARRAYS
 *********************************************************/
int double_array_create(double_array*a)
{
	int n;
	a->n = a->m = 0;
	a->max = 1;
	a->x = (double***)malloc(sizeof(double**)*a->max);
	a->f = (unsigned char*)malloc(sizeof(unsigned char)*a->max);
	if ( a->x==NULL || a->f==NULL )
		return 0;
	memset(a->x,0,sizeof(double**)*a->max);
	memset(a->f,0,sizeof(unsigned char)*a->max);
	return 1;
}
double get_double_array_value(double_array*a,unsigned int n, unsigned int m)
{
	if ( a->n>n && a->m>m )
		return *(a->x[n][m]);
	else
		throw_exception("get_double_array_value(double_array*a='n=%d,m=%d,...',unsigned int n=%d,unsigned int m=%d): array index out of range",a->n,a->m,n,m);
}
void set_double_array_value(double_array*a,unsigned int n, unsigned int m, double x)
{
	if ( a->n>n && a->m>m )
		*(a->x[n][m])=x;
	else
		throw_exception("get_double_array_value(double_array*a='n=%d,m=%d,...',unsigned int n=%d,unsigned int m=%d): array index out of range",a->n,a->m,n,m);
}
double *get_double_array_ref(double_array*a,unsigned int n, unsigned int m)
{
	if ( a->n>n && a->m>m )
		return a->x[n][m];
	else
		throw_exception("get_double_array_value(double_array*a='n=%d,m=%d,...',unsigned int n=%d,unsigned int m=%d): array index out of range",a->n,a->m,n,m);
}
double double_array_get_part(void *x, char *name)
{
	int n,m;
	if (sscanf(name,"%d.%d",&n,&m)==2)
	{
		double_array *a = (double_array*)x;
		if ( n<a->n && m<a->m && a->x[n][m]!=NULL )
			return *(a->x[n][m]);
	}
	return QNAN;
}

/*********************************************************
 * COMPLEX ARRAYS
 *********************************************************/
int complex_array_create(complex_array *a)
{
	int n;
	a->n = a->m = 0;
	a->max = 1;
	a->x = (complex***)malloc(sizeof(complex**)*a->max);
	a->f = (unsigned char*)malloc(sizeof(unsigned char)*a->max);
	if ( a->x==NULL || a->f==NULL )
		return 0;
	memset(a->x,0,sizeof(complex**)*a->max);
	memset(a->f,0,sizeof(unsigned char)*a->max);
	return 1;
}
complex *get_complex_array_value(complex_array *a,unsigned int n, unsigned int m)
{
	if ( a->n>n && a->m>m )
		return a->x[n][m];
	else
		throw_exception("get_complex_array_value(complex_array*a='n=%d,m=%d,...',unsigned int n=%d,unsigned int m=%d): array index out of range",a->n,a->m,n,m);
}
void set_complex_array_value(complex_array *a,unsigned int n, unsigned int m, complex *x)
{
	if ( a->n>n && a->m>m )
		*(a->x[n][m]) = *x;
	else
		throw_exception("get_complex_array_value(complex_array*a='n=%d,m=%d,...',unsigned int n=%d,unsigned int m=%d): array index out of range",a->n,a->m,n,m);
}
complex *get_complex_array_ref(complex_array *a,unsigned int n, unsigned int m)
{
	if ( a->n>n && a->m>m )
		return a->x[n][m];
	else
		throw_exception("get_complex_array_value(complex_array*a='n=%d,m=%d,...',unsigned int n=%d,unsigned int m=%d): array index out of range",a->n,a->m,n,m);
}
double complex_array_get_part(void *x, char *name)
{
	int n,m;
	char subpart[32];
	if (sscanf(name,"%d.%d.%31s",&n,&m,subpart)==2)
	{
		complex_array *a = (complex_array*)x;
		if ( n<a->n && m<a->m && a->x[n][m]!=NULL )
		{
			if ( strcmp(subpart,"real")==0 ) return a->x[n][m]->r;
			else if ( strcmp(subpart,"imag")==0 ) return a->x[n][m]->i;
			else return QNAN;
		}
	}
	return QNAN;
}



// EOF
