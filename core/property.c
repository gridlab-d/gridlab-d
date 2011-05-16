/** $Id: property.c 1182 2011-05-06 22:08:36Z mhauer $
	Copyright (C) 2011 Battelle Memorial Institute
	@file property.c
	@addtogroup property Properties of objects
	@ingroup core
	
	GridLAB-D classes contain properties,
	which are supported by the functions in this module

 @{
 **/

#include "class.h"
#include "output.h"
#include "convert.h"
#include "module.h"
#include "exception.h"
#include "timestamp.h"
#include "loadshape.h"
#include "enduse.h"
#include "stream.h"

/* IMPORTANT: this list must match PROPERTYTYPE enum in class.h */
/* ALSO IMPORTANT: this list was copied from class.c */
static struct s_property_specs { /**<	the property type conversion specifications.
										It is critical that the order of entries in this list must match 
										the order of entries in the enumeration #PROPERTYTYPE 
								  **/
	char *name; /**< the property type name */
	unsigned int size; /**< the size of 1 instance */
	int (*data_to_string)(char *,int,void*,PROPERTY*); /**< the function to convert from data to a string */
	int (*string_to_data)(char *,void*,PROPERTY*); /**< the function to convert from a string to data */
	int (*create)(void*); /**< the function used to create the property, if any */
	int (*stream_in)(FILE*,void*,PROPERTY*); /**< the function to read data from a stream */
	int (*stream_out)(FILE*,void*,PROPERTY*); /**< the function to write data to a stream */
} property_type[] = {
	{"void", 0, convert_from_void,convert_to_void},
	{"double", sizeof(double), convert_from_double,convert_to_double,NULL,stream_in_double,stream_out_double},
	{"complex", sizeof(complex), convert_from_complex,convert_to_complex},
	{"enumeration",sizeof(int32), convert_from_enumeration,convert_to_enumeration},
	{"set",sizeof(int64), convert_from_set,convert_to_set},
	{"int16", sizeof(int16), convert_from_int16,convert_to_int16},
	{"int32", sizeof(int32), convert_from_int32,convert_to_int32},
	{"int64", sizeof(int64), convert_from_int64,convert_to_int64},
	{"char8", sizeof(char8), convert_from_char8,convert_to_char8},
	{"char32", sizeof(char32), convert_from_char32,convert_to_char32},
	{"char256", sizeof(char256), convert_from_char256,convert_to_char256},
	{"char1024", sizeof(char1024), convert_from_char1024,convert_to_char1024},
	{"object", sizeof(OBJECT*), convert_from_object,convert_to_object},
	{"delegated", (unsigned int)-1, convert_from_delegated, convert_to_delegated},
	{"bool", sizeof(unsigned int), convert_from_boolean, convert_to_boolean},
	{"timestamp", sizeof(int64), convert_from_timestamp_stub, convert_to_timestamp_stub},
	{"double_array", sizeof(double), convert_from_double_array, convert_to_double_array},
	{"complex_array", sizeof(complex), convert_from_complex_array, convert_to_complex_array},
	{"real", sizeof(real), convert_from_real, convert_to_real},
	{"float", sizeof(float), convert_from_float, convert_to_float},
	{"loadshape", sizeof(loadshape), convert_from_loadshape, convert_to_loadshape, loadshape_create},
	{"enduse",sizeof(enduse), convert_from_enduse, convert_to_enduse, enduse_create},
};

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
	if (sscanf(name,"%[^[][%[%%A-Za-z0-9*/^]]",prop->name,unitspec)==2)
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
			TRY {
				if ((prop->unit = unit_find(unitspec))==NULL)
					output_error("property_malloc(oclass='%s',...): property %s unit '%s' is not recognized",oclass->name, prop->name,unitspec);
					/*	TROUBLESHOOT
						A class is attempting to publish a variable using a unit that is not defined.  
						This is caused by an incorrect unit specification in a variable publication (in C++) or declaration (in GLM).
						Units are defined in the unit file located in the GridLAB-D <b>etc</b> folder.  
					 */
			} CATCH (char *msg) {
					output_error("property_malloc(oclass='%s',...): property %s unit '%s' is not recognized",oclass->name, prop->name,unitspec);
					/*	TROUBLESHOOT
						A class is attempting to publish a variable using a unit that is not defined.  
						This is caused by an incorrect unit specification in a variable publication (in C++) or declaration (in GLM).
						Units are defined in the unit file located in the GridLAB-D <b>etc</b> folder.  
					 */
			} ENDCATCH;
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

// EOF
