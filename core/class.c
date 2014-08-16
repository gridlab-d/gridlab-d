/** $Id: class.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file class.c
	@addtogroup class Classes of objects
	@ingroup core
 **/

#include "exec.h"
#include "property.h"
#include "class.h"
#include "output.h"
#include "convert.h"
#include "module.h"
#include "exception.h"
#include "timestamp.h"
#include "loadshape.h"
#include "enduse.h"
#include "stream.h"
#include "random.h"

#if defined WIN32 && ! defined MINGW
	#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
	#define _WIN32_WINNT 0x0400
	#include <windows.h>
	#ifndef DLEXT
		#define DLEXT ".dll"
	#endif
	#define DLLOAD(P) LoadLibrary(P)
	#define DLSYM(H,S) GetProcAddress((HINSTANCE)H,S)
	#define snprintf _snprintf
#else /* ANSI */
#ifndef MINGW
	#include "dlfcn.h"
#endif
	#ifndef DLEXT
		#define DLEXT ".so"
	#endif
#ifndef MINGW
	#define DLLOAD(P) dlopen(P,RTLD_LAZY)
#else
	#define DLLOAD(P) dlopen(P)
#endif
	#define DLSYM(H,S) dlsym(H,S)
#endif

static unsigned int class_count = 0;

/* defined in property.c */
extern struct s_property_specs property_type[_PT_LAST];

/* object class list */
static CLASS *first_class = NULL; /**< first class in class list */
static CLASS *last_class = NULL; /**< last class in class list */

/** Get the first property in a class's property list.
	All subsequent properties that have the same class
	can be scanned.  Be careful not to scan off the end
	of the list onto the next class.  The iterator
	should look like this

	@code
	PROPERTY *p;
	for (p=class_get_first_property(oclass);
		p!=NULL && p->otype==oclass->type;
		p=p->next)
	{
	// your code goes here
	}
	@endcode

	@return a pointer to first PROPERTY in the CLASS definition,
	or \p NULL is none defined
 **/
PROPERTY *class_get_first_property(CLASS *oclass) /**< the object class */
{
	if (oclass==NULL)
		throw_exception("class_get_first_property(CLASS *oclass=NULL): oclass is NULL");
		/* TROUBLESHOOT
			A call to <code>class_get_first_property()</code> was made with a NULL pointer.
			This is a bug and should be reported.
		 */
	return oclass->pmap;
}

/** Get the next property of within the current class
	@return a pointer to the PROPERTY, or \p NULL if there are no properties left
 **/
PROPERTY *class_get_next_property(PROPERTY *prop)
{
	if (prop->next && prop->oclass==prop->next->oclass)
		return prop->next;
	else
		return NULL;
}

/** Search class hierarchy for a property
	@return property pointer if found, NULL if not in class hierarchy
 **/
PROPERTY *class_prop_in_class(CLASS *oclass, PROPERTY *prop)
{
	if(oclass == prop->oclass)
	{
		return prop;
	}
	else
	{
		if(oclass->parent != NULL)
		{
			return class_prop_in_class(oclass->parent, prop);
		}
		else
		{
			return NULL;
		}
	}
}

#if 0
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
#endif

/* though improbable, this is to prevent more complicated, specifically crafted
	inheritence loops.  these should be impossible if a class_register call is
	immediately followed by a class_define_map call. -d3p988 */
PROPERTY *class_find_property_rec(CLASS *oclass, 
                                  PROPERTYNAME name, 
                                  CLASS *pclass)
{
	PROPERTY *prop;
	for (prop=oclass->pmap; prop!=NULL && prop->oclass==oclass; prop=prop->next)
	{
		if (strcmp(name,prop->name)==0)
			return prop;
	}
	if (oclass->parent==pclass)
	{
		output_error("class_find_property_rec(CLASS *oclass='%s', PROPERTYNAME name='%s', CLASS *pclass='%s') causes an infinite class inheritance loop", oclass->name, name, pclass->name);
		/*	TROUBLESHOOT
			A class has somehow specified itself as a parent class, either directly or indirectly.
			This means there is a problem with the module that publishes the class.
		 */
		return NULL;
	}
	else if (oclass->parent!=NULL)
		return class_find_property_rec(oclass->parent,name, pclass);
	else
		return NULL;

}

static PROPERTY *find_header_property(CLASS *oclass, 
                                      PROPERTYNAME name)
{
	PROPERTY *prop = NULL;
	return prop;
}

/** Find the named property in the class

	@return a pointer to the PROPERTY, or \p NULL if the property is not found.
 **/
PROPERTY *class_find_property(CLASS *oclass,     /**< the object class */
                              PROPERTYNAME name) /**< the property name */
{
	PROPERTY *prop = find_header_property(oclass,name);
	if ( prop ) return prop;

	if(oclass == NULL)
		return NULL;

	for (prop=oclass->pmap; prop!=NULL && prop->oclass==oclass; prop=prop->next)
	{
		if (strcmp(name,prop->name)==0)
		{
			if (prop->flags&PF_DEPRECATED && !(prop->flags&PF_DEPRECATED_NONOTICE) && !global_suppress_deprecated_messages)
			{
				output_warning("class_find_property(CLASS *oclass='%s', PROPERTYNAME name='%s': property is deprecated", oclass->name, name);
				/* TROUBLESHOOT
					You have done a search on a property that has been flagged as deprecated and will most likely not be supported soon.
					Correct the usage of this property to get rid of this message.
				 */
				if (global_suppress_repeat_messages)
					prop->flags |= ~PF_DEPRECATED_NONOTICE;
			}
			return prop;
		}
	}
	if (oclass->parent==oclass)
	{
		output_error("class_find_property(oclass='%s', name='%s') causes an infinite class inheritance loop", oclass->name, name);
		/*	TROUBLESHOOT
			A class has somehow specified itself as a parent class, either directly or indirectly.
			This means there is a problem with the module that publishes the class.
		 */
		return NULL;
	}
	else if (oclass->parent!=NULL)
		return class_find_property_rec(oclass->parent,name, oclass);
	else
		return NULL;
}

/** Add a property to a class
 **/
void class_add_property(CLASS *oclass,  /**< the class to which the property is to be added */
                        PROPERTY *prop) /**< the property to be added */
{
	PROPERTY *last = oclass->pmap;
	while (last!=NULL && last->next!=NULL)
		last = last->next;
	if (last==NULL)
		oclass->pmap = prop;
	else
		last->next = prop;
}

/** Add an extended property to a class 
    @return the property pointer
 **/
PROPERTY *class_add_extended_property(CLASS *oclass,      /**< the class to which the property is to be added */
                                      char *name,         /**< the name of the property */
                                      PROPERTYTYPE ptype, /**< the type of the property */
                                      char *unit)         /**< the unit of the property */
{
	PROPERTY *prop = malloc(sizeof(PROPERTY));
	UNIT *pUnit = NULL;

	TRY {
		if (unit)
			pUnit = unit_find(unit);
	} CATCH (char *msg) {
		// will get picked up later
	} ENDCATCH;

	if (prop==NULL)
		throw_exception("class_add_extended_property(oclass='%s', name='%s', ...): memory allocation failed", oclass->name, name);
		/* TROUBLESHOOT
			The system has run out of memory.  Try making the model smaller and trying again.
		 */
	if (ptype<=_PT_FIRST || ptype>=_PT_LAST)
		throw_exception("class_add_extended_property(oclass='%s', name='%s', ...): property type is invalid", oclass->name, name);
		/* TROUBLESHOOT
			The function was called with a property type that is not recognized.  This is a bug that should be reported.
		 */
	if (unit!=NULL && pUnit==NULL)
		throw_exception("class_add_extended_property(oclass='%s', name='%s', ...): unit '%s' is not found", oclass->name, name, unit);
		/* TROUBLESHOOT
			The function was called with unit that is defined in units file <code>.../etc/unitfile.txt</code>.  Try using a defined unit or adding
			the desired unit to the units file and try again.
		 */
	memset(prop, 0, sizeof(PROPERTY));
	prop->access = PA_PUBLIC;
	prop->addr = (void*)(int64)oclass->size;
	prop->size = 0;
	prop->delegation = NULL;
	prop->flags = PF_EXTENDED;
	prop->keywords = NULL;
	prop->description = NULL;
	prop->unit = pUnit;
	strncpy(prop->name,name,sizeof(prop->name));
	prop->next = NULL;
	prop->oclass = oclass;
	prop->ptype = ptype;
	prop->width = property_type[ptype].size;

	oclass->size += property_type[ptype].size;

	class_add_property(oclass,prop);
	return prop;
}

/** Get the last registered class
    @return the last class registered
 **/
CLASS *class_get_last_class(void)
{
	return last_class;
}

/** Get the number of registered class
	@return the number of classes registered
 **/
unsigned int class_get_count(void)
{
	return class_count;
}

/** Get the name of a property from its type
	@return a pointer to a string containing the name of the property type
 **/
char *class_get_property_typename(PROPERTYTYPE type) /**< the property type */
{
	if (type<=_PT_FIRST || type>=_PT_LAST)
		return "##UNDEF##";
	else
		return property_type[type].name;
}

/** Get the name of a property from its type
	@return a pointer to a string containing the name of the property type
 **/
char *class_get_property_typexsdname(PROPERTYTYPE type) /**< the property type */
{
	if (type<=_PT_FIRST || type>=_PT_LAST)
		return "##UNDEF##";
	else
		return property_type[type].xsdname;
}


/** Get the type of a property from its \p name
	@return the property type
 **/
PROPERTYTYPE class_get_propertytype_from_typename(char *name) /**< a string containing the name of the property type */
{
	int i;
	for (i=0; i<sizeof(property_type)/sizeof(property_type[0]); i++)
	{
		if (strcmp(property_type[i].name,name)==0)
			return i;
	}
	return PT_void;
}

/** Convert a string to a property of the given type
    @return non-zero on success, 0 on failure
 **/
int class_string_to_propertytype(PROPERTYTYPE type, 
                                 void *addr, 
                                 char *value)
{
	if (type > _PT_FIRST && type < _PT_LAST)
		return (*property_type[type].string_to_data)(value,addr,NULL);
	else
		return 0;
}
/** Convert a string value to property data.
	The \p addr must be the physical address in memory.
	@return the number of value read from the \p value string; 0 on failure
 **/
int class_string_to_property(PROPERTY *prop, /**< the type of the property at the \p addr */
                             void *addr,     /**< the address of the property's data */
                             char *value)    /**< the string from which the data is read */
{
	if (prop->ptype > _PT_FIRST && prop->ptype < _PT_LAST)
		return (*property_type[prop->ptype].string_to_data)(value,addr,prop);
	else
		return 0;
}

/** Convert a property value to a string.
	The \p addr must be the physical address in memory.
	@return the number character written to \p value
 **/
int class_property_to_string(PROPERTY *prop, /**< the property type */
                             void *addr,     /**< the address of the property's data */
                             char *value,    /**< the value buffer to which the string is to be written */
                             int size)       /**< the maximum number of characters that can be written to the \p value buffer*/
{
	int rv = 0;
	if (prop->ptype==PT_delegated)
	{
		output_error("unable to convert from delegated property value");
		/*	TROUBLESHOOT
			Property delegation is not yet fully implemented, so you should never get this error.
			If you do, there is a problem with the system that is causing it to become unstable.
		 */
		return 0;
	}
	else if (prop->ptype>_PT_FIRST && prop->ptype<_PT_LAST){
		// note, need to append unit type
		rv = (*property_type[prop->ptype].data_to_string)(value,size,addr,prop);
		if(rv > 0 && prop->unit != 0)
		{
			strcat(value+rv," ");
			strcat(value+rv+1,prop->unit->name);
			rv += (int)(1+strlen(prop->unit->name));
		}
	}
	else
	{
		rv = 0;
	}
	return rv;
}


/** Register an object class
	@return the object class; \p NULL on error \p errno:
	- \p E2BIG: class name too long
	- \p ENOMEM: memory allocation failed

 **/
CLASS *class_register(MODULE *module,        /**< the module that implements the class */
                      CLASSNAME name,        /**< the class name */
                      unsigned int size,     /**< the size of the data block */
                      PASSCONFIG passconfig) /**< the passes for which \p sync should be called */
{
	CLASS *oclass = class_get_class_from_classname(name);

	/* check the property list */
	int a = sizeof(property_type);
	int b = sizeof(property_type[0]);
	int c = _PT_LAST - _PT_FIRST - 1;

	if (_PT_LAST-_PT_FIRST-1!=sizeof(property_type)/sizeof(property_type[0]))
	{
		output_fatal("property_type[] in class.c has an incorrect number of members (%i vs %i)", a/b, c);
		/* TROUBLESHOOT
			This error occurs when an improper definition of a class is used.  This is not usually
			caused by an error in a GLM file but is most likely caused by a bug in a module
			or incorrectly defined class.
		 */
		exit(XC_EXCEPTION);
	}
	if (oclass!=NULL)
	{
		if(strcmp(oclass->module->name, module->name) == 0){
			output_error("module %s cannot register class %s, it is already registered by module %s", module->name,name,oclass->module->name);
			/*	TROUBLESHOOT
				This error is caused by an attempt to define a new class which is already
				defined in the module or namespace given.  This is generally caused by
				bug in a module or an incorrectly defined class.
			 */
			return NULL;
		} else {
			output_verbose("module %s is registering a 2nd class %s, previous one in module %s", module->name, name, oclass->module->name);
		}
	}
	if (strlen(name)>=sizeof(oclass->name) )
	{
		errno = E2BIG;
		return 0;
	}
	oclass = (CLASS*)malloc(sizeof(CLASS));
	if (oclass==NULL)
	{
		errno = ENOMEM;
		return 0;
	}
	memset(oclass,0,sizeof(CLASS));
	oclass->magic = CLASSVALID;
	oclass->id = 	class_count++;
	oclass->module = module;
	strncpy(oclass->name,name,sizeof(oclass->name));
	oclass->size = size;
	oclass->passconfig = passconfig;
	oclass->profiler.numobjs=0;
	oclass->profiler.count=0;
	oclass->profiler.clocks=0;
	if (first_class==NULL)
		first_class = oclass;
	else
		last_class->next = oclass;
	last_class = oclass;
	output_verbose("class %s registered ok", name);
	return oclass;
}

/** Get the first registered class
	@return a pointer to the first registered CLASS,
	or \p NULL if none registered.
 **/
CLASS *class_get_first_class(void)
{
	return first_class;
}

/** Get the class from the class name and a module pointer.
	@return a pointer to the class registered to that module
	having that \p name, or \p NULL if no match found.
 **/
CLASS *class_get_class_from_classname_in_module(char *name, MODULE *mod){
	CLASS *oclass = NULL;
	if(name == NULL) return NULL;
	if(mod == NULL) return NULL;
	for (oclass=first_class; oclass!=NULL; oclass=oclass->next)
	{
		if(oclass->module == (MODULE *)mod)
			if(strcmp(oclass->name,name)==0)
				return oclass;
	}
	return NULL;
}

/** Get the number of runtime classes defined
    @return the number of runtime classes defined
 **/
size_t class_get_runtimecount(void)
{
	CLASS *oclass;
	size_t count = 0;
	for ( oclass=first_class ; oclass!=NULL ; oclass=oclass->next )
	{
		if ( oclass->has_runtime )
			count++;
	}
	return count;
}

/** Get the first runtime class defined
    @return the pointer to the first runtime class
 **/
CLASS *class_get_first_runtime(void)
{
	CLASS *oclass;
	for ( oclass=first_class ; oclass!=NULL ; oclass=oclass->next )
	{
		if ( oclass->has_runtime )
			return oclass;
	}
	return NULL;
}

/** Get the next runtime class defined
    @return the point to the next runtime class
 **/
CLASS *class_get_next_runtime(CLASS *oclass) /**< the class to search from */
{
	oclass=oclass->next;
	while ( oclass!=NULL )
	{
		if ( oclass->has_runtime )
			return oclass;
		oclass=oclass->next;
	}
	return NULL;
}
/** get count of extended properties */
size_t class_get_extendedcount(CLASS *oclass)
{
	PROPERTY *prop;
	size_t count = 0;
	for ( prop=oclass->pmap ; prop!=NULL ; prop=prop->next )
	{
		if ( prop->flags&PF_EXTENDED ) count++;
	}
	return count;
}

/** Get the class from the class name.
	@return a pointer to the class having that \p name,
	or \p NULL if no match found.
 **/
CLASS *class_get_class_from_classname(char *name) /**< a pointer to a \p NULL -terminated string containing the class name */
{
	CLASS *oclass = NULL;
	MODULE *mod = NULL;
	char *ptr = NULL;
	char temp[1024]; /* we get access violations when name is from another DLL. -mh */
	strcpy(temp, name);
	ptr = strchr(temp, '.');
	if(ptr != NULL){	/* check module for the class */
		ptr[0] = 0;
		++ptr;
		mod = module_find(temp);
		if(mod == NULL){
			output_verbose("could not search for '%s.%s', module not loaded", name, ptr);
			return NULL;
		}
		for (oclass=first_class; oclass!=NULL; oclass=oclass->next)
		{
			if(oclass->module == mod)
				if(strcmp(oclass->name,ptr)==0)
					return oclass;
		}
		return NULL;
	}
	for (oclass=first_class; oclass!=NULL; oclass=oclass->next)
	{
		if (strcmp(oclass->name,name)==0)
			return oclass;
	}
	return NULL;
}

/** Define one or more class properties.

	The variable argument list must be \p NULL -terminated.
	Each property declaration begins with a PROPERTYTYPE value,
	followed by a \e char* pointing to the name of the
	property, followed the offset from the end of the
	OBJECT header's address (or the absolution address of the data
	if PT_SIZE is used).  If the property name includes units in square
	brackets, they will be separated from the name and added to
	the property's definition, provided the are defined in the
	file \b unitfile.txt.

	You may use the flag PT_INHERIT to specify a parent class from which
	to inherit published properties.  If this is used, you may add the
	PC_UNSAFE_OVERRIDE_OMIT flag in class_register to force child classes
	to use all the pass configured.

	You may use special flags to customize the property registration:
	- \p PT_ACCESS will set special access rights (see PROPERTYACCESS)
	- \p PT_SIZE will cause memory to be allocated for the value.  If
	  \p addr is non-NULL, the data is points to will be converted to
	  the property.
	- \p PT_FLAGS will allow you set property flags (see PROPERTYFLAGS)
	- \p PT_INHERIT will allow you to include the parent classes properties
	- \p PT_UNITS will allow you to specify the units (instead of using [] syntax)
	- \p PT_EXTEND will expand the size of the class by the size of the property being mapped
	- \p PT_EXTENDBY will expand the size of the class by the unsigned int provided in the next argument

	@return a count of variables mapped; <=0 indicates not all mapped
	(-count successfully mapped before it failed), errno:
	- \p E2BIG: variable name too long to store
	- \p ENOMEM: memory allocation failed
	- \p ENOENT: \p oclass type not found
	- \p EINVAL: keyword is invalid
 **/
int class_define_map(CLASS *oclass, /**< the object class */
                     ...) /**< definition arguments (see remarks) */
{
	va_list arg;
	PROPERTYTYPE proptype;
	int count=0;
	PROPERTY *prop=NULL;
	va_start(arg,oclass);
	errno = 0;
	while ((proptype=va_arg(arg,PROPERTYTYPE))!=0)
	{
		if (proptype>_PT_LAST)
		{
			if (proptype==PT_INHERIT)
			{
				if (oclass->parent!=NULL)
				{
					errno = EINVAL;
					output_error("class_define_map(oclass='%s',...): PT_INHERIT unexpected; class already inherits properties from class %s", oclass->name, oclass->parent);
					/* TROUBLESHOOT
						This error is caused by an attempt to incorrectly specify a class that
						inherits variables from more than one other class.  This is almost
						always caused by a bug in the module's constructor for that class.
					 */
					goto Error;
				}
				else
				{
					char *classname = va_arg(arg,char*);
					PASSCONFIG no_override;
					oclass->parent = class_get_class_from_classname_in_module(classname,oclass->module);
					if (oclass->parent==NULL)
					{
						errno = EINVAL;
						output_error("class_define_map(oclass='%s',...): parent property class name '%s' is not defined", oclass->name, classname);
						/*	TROUBLESHOOT
							A class is trying to inherit properties from another class that has not been defined.
							This is usually caused by a problem in the module(s) that publishes the classes.  Either
							the child class is in not specifying the parent class correctly, or the parent class is
							not created before the child class.  If the child class depends on a class implemented
							in another module, the module publishing the child class must load the module publishing
							the parent class before attempting to use it.
						 */
						goto Error;
					}
					if (oclass->parent == oclass){
						errno = EINVAL;
						output_error("class_define_map(oclass='%s',...): parent property class name '%s' attempting to inherit from self!", oclass->name, classname);
						/*	TROUBLESHOOT
							A class is attempting to directly inherit properties from itself.  This is caused by
							a problem with the module that publishes the class.
						 */
						goto Error;
					}
					no_override = ~(~oclass->parent->passconfig|oclass->passconfig); /* parent bool-implies child (p->q=~p|q) */
					if (oclass->parent->passconfig&PC_UNSAFE_OVERRIDE_OMIT
							&& !(oclass->passconfig&PC_PARENT_OVERRIDE_OMIT)
							&& no_override&PC_PRETOPDOWN)
						output_warning("class_define_map(oclass='%s',...): class '%s' suppresses parent class '%s' PRETOPDOWN sync behavior by omitting override", oclass->name, oclass->name, oclass->parent->name);
						/*	TROUBLESHOOT
							A class is suppressing the <i>presync</i> event implemented by its parent
							even though the parent is published with a flag that indicates this is unsafe.  Presumably
							this is deliberate, but the warning is given just in case it's not intended.
						 */
					if (oclass->parent->passconfig&PC_UNSAFE_OVERRIDE_OMIT
							&& !(oclass->passconfig&PC_PARENT_OVERRIDE_OMIT)
							&& no_override&PC_BOTTOMUP)
						output_warning("class_define_map(oclass='%s',...): class '%s' suppresses parent class '%s' BOTTOMUP sync behavior by omitting override", oclass->name, oclass->name, oclass->parent->name);
						/*	TROUBLESHOOT
							A class is suppressing the <i>sync</i> event implemented by its parent
							even though the parent is published with a flag that indicates this is unsafe.  Presumably
							this is deliberate, but the warning is given just in case it's not intended.
						 */
					if (oclass->parent->passconfig&PC_UNSAFE_OVERRIDE_OMIT
							&& !(oclass->passconfig&PC_PARENT_OVERRIDE_OMIT)
							&& no_override&PC_POSTTOPDOWN)
						output_warning("class_define_map(oclass='%s',...): class '%s' suppresses parent class '%s' POSTTOPDOWN sync behavior by omitting override", oclass->name, oclass->name, oclass->parent->name);
						/*	TROUBLESHOOT
							A class is suppressing the <i>postsync</i> event implemented by its parent
							even though the parent is published with a flag that indicates this is unsafe.  Presumably
							this is deliberate, but the warning is given just in case it's not intended.
						 */
					if (oclass->parent->passconfig&PC_UNSAFE_OVERRIDE_OMIT
							&& !(oclass->passconfig&PC_PARENT_OVERRIDE_OMIT)
							&& no_override&PC_UNSAFE_OVERRIDE_OMIT)
						output_warning("class_define_map(oclass='%s',...): class '%s' does not assert UNSAFE_OVERRIDE_OMIT when parent class '%s' does", oclass->name, oclass->name, oclass->parent->name);
						/*	TROUBLESHOOT
							A class is not asserting that it is unsafe to suppress synchronization behavior
							but its parent does assert that this is unsafe.  This permits stealth omission
							by any classes that inherits behavior from this class and the warning is given
							in case this is not intended.
						 */
					count++;
				}
			}

			/* this test will catch use of PT_? tokens outside the context of a property */
			else if (prop==NULL)
			{
				errno = EINVAL;
				output_error("class_define_map(oclass='%s',...): expected keyword missing after '%s'", oclass->name, class_get_property_typename(proptype));
				/*	TROUBLESHOOT
					The structure of class is being published with some special properties that only work in the context
					of a published variable.  This is caused by a problem with the module that publishes the class.
				 */
				goto Error;
			}
			else if (proptype==PT_KEYWORD && prop->ptype==PT_enumeration)
			{
				char *keyword = va_arg(arg,char*);
				int32 keyvalue = va_arg(arg,int32);
				if (!class_define_enumeration_member(oclass,prop->name,keyword,keyvalue))
				{
					errno = EINVAL;
					output_error("class_define_map(oclass='%s',...): property keyword '%s' could not be defined as value %d", oclass->name, keyword,keyvalue);
					/*	TROUBLESHOOT
						An attempt to define an <i>enumeration</i> property is using a value that cannot be used, either because it is
						already being used, or because it is outside of range of allowed values for that property.  That is caused
						by a problem in the module that publishes the class.
					 */
					goto Error;
				}
			}
			else if (proptype==PT_KEYWORD && prop->ptype==PT_set)
			{
				char *keyword = va_arg(arg,char*);
				unsigned int64 keyvalue = va_arg(arg, int64);
				if (!class_define_set_member(oclass,prop->name,keyword,keyvalue))
				{
					errno = EINVAL;
					output_error("class_define_map(oclass='%s',...): property keyword '%s' could not be defined as value %d", oclass->name, keyword,keyvalue);
					/*	TROUBLESHOOT
						An attempt to define an <i>set</i> property is using a value that cannot be used, either because it is
						already being used, or because it is outside of range of allowed values for that property.  That is caused
						by a problem in the module that publishes the class.
					 */
					goto Error;
				}
			}
			else if (proptype==PT_ACCESS)
			{
				PROPERTYACCESS pa = va_arg(arg,PROPERTYACCESS);
				switch (pa) {
				case PA_PUBLIC:
				case PA_PROTECTED:
				case PA_PRIVATE:
				case PA_REFERENCE:
				case PA_HIDDEN:
					prop->access = pa;
					break;
				default:
					errno = EINVAL;
					output_error("class_define_map(oclass='%s',...): unrecognized property access code (value=%d is not valid)", oclass->name, pa);
					/*	TROUBLESHOOT
						A class is attempting specify a type variable access that is unknown.  This is caused by a problem
						in the module that publishes the class.
					 */
					goto Error;
					break;
				}
			}
			else if (proptype==PT_SIZE)
			{
				prop->size = va_arg(arg,uint32);
				if (prop->size<1)
				{
					errno = EINVAL;
					output_error("class_define_map(oclass='%s',...): property size must be greater than 0", oclass->name, proptype);
					/*	TROUBLESHOOT
						A class is attempting to define a repeated variable (such as an array) that contains less than 1 item.
						This is caused by a problem in the module that published the class.
					 */
					goto Error;
				}
			}
			else if (proptype==PT_EXTEND)
			{
				oclass->size += property_type[prop->ptype].size;
			}
			else if (proptype==PT_EXTENDBY)
			{
				oclass->size += va_arg(arg,unsigned int);
			}
			else if (proptype==PT_FLAGS)
			{
				prop->flags |= va_arg(arg,unsigned int);
			}
			else if (proptype==PT_DEPRECATED)
			{
				prop->flags |= PF_DEPRECATED;
			}
			else if (proptype==PT_UNITS)
			{
				char *unitspec = va_arg(arg,char*);
				TRY {
					if ((prop->unit = unit_find(unitspec))==NULL)
						throw_exception("unable to define unit '%s'", unitspec); 
				} CATCH (const char *msg) {
						output_error("class_define_map(oclass='%s',...): property %s unit '%s' is not recognized: %s",oclass->name, prop->name,unitspec,msg);
						/*	TROUBLESHOOT
							A class is attempting to publish a variable using a unit that is not defined.
							This is caused by an incorrect unit specification in a variable publication (in C++) or declaration (in GLM).
							Units are defined in the unit file located in the GridLAB-D <b>etc</b> folder.
							This error immediately follows a throw event with the same message.
						 */
				} ENDCATCH;
			}
			else if (proptype==PT_DESCRIPTION)
			{
				prop->description = va_arg(arg,char*);
			}
			else if(proptype == PT_HAS_NOTIFY || proptype == PT_HAS_NOTIFY_OVERRIDE)
			{
				char notify_fname[128];
				sprintf(notify_fname, "notify_%s_%s", prop->oclass->name, prop->name);
				prop->notify = (FUNCTIONADDR)DLSYM(prop->oclass->module->hLib, notify_fname);
				if(prop->notify == 0){
					errno = EINVAL;
					output_error("Unable to find function '%s' in %s module", notify_fname, prop->oclass->module->name);
					goto Error;
				}
				if(proptype == PT_HAS_NOTIFY_OVERRIDE){
					prop->notify_override = true;
				} else {
					prop->notify_override = false;
				}
			}
			else
			{
				char tcode[32];
				char *ptypestr=class_get_property_typename(proptype);
				sprintf(tcode,"%d",proptype);
				if (strcmp(ptypestr,"##UNDEF##")==0)
					ptypestr = tcode;
				errno = EINVAL;
				output_error("class_define_map(oclass='%s',...): unrecognized extended property (PROPERTYTYPE=%s)", oclass->name, ptypestr?ptypestr:tcode);
				/*	TROUBLESHOOT
					A property extension given in a published class specification uses a property type (PT_*) is that not valid.
					This is caused by a problem in the module that publishes the class.
				 */
				goto Error;
			}
		}
		else if (proptype==PT_enduse) /// @todo class_define_map support for enduse has to be done explicitly until structures are supported
		{
			char *name = va_arg(arg,char*);
			PROPERTYADDR addr = va_arg(arg,PROPERTYADDR);
			if (enduse_publish(oclass,addr,name)<=0)
			{
				output_error("class_define_map(oclass='%s',...): substructure of property '%s' substructure could not be published", oclass->name, prop->name);
				/*	TROUBLESHOOT
					A class is publishing a property that has a substructure, which couldn't be published.
					This must be corrected in the code the declares the property or publishes the class.
				 */
				errno = E2BIG;
				goto Error;
			}
		}
		else
		{
			DELEGATEDTYPE *delegation=(proptype==PT_delegated?va_arg(arg,DELEGATEDTYPE*):NULL);
			char *name = va_arg(arg,char*);
			PROPERTYADDR addr = va_arg(arg,PROPERTYADDR);
			if (prop!=NULL && strlen(name)>=sizeof(prop->name))
			{
				output_error("class_define_map(oclass='%s',...): property name '%s' is too big", oclass->name, name);
				/*	TROUBLESHOOT
					A class is publishing a property using a name that is too big for the system.
					Property names are limited in length.
					This must be corrected in the code the declares the property or publishes the class.
				 */
				errno = E2BIG;
				goto Error;
			}
			if (strcmp(name,"parent")==0){
				output_error("class_define_map(oclass='%s',...): property name '%s' conflicts with built-in property", oclass->name, name);
				/*	TROUBLESHOOT
					A class is attempting to publish a variable with a name normally reserved for object headers.
					This is not allowed.  If the class is implemented in a module, this is problem with the module.
					If the class is declared in a GLM file, you must correct the problem to avoid unpredictable
					simulation behavior.
				 */
				goto Error;
			} else if (strcmp(name,"rank")==0) {
				output_error("class_define_map(oclass='%s',...): property name '%s' conflicts with built-in property", oclass->name, name);
				/* no need to repeat troubleshoot message */
				goto Error;
			} else if (strcmp(name,"clock")==0) {
				output_error("class_define_map(oclass='%s',...): property name '%s' conflicts with built-in property", oclass->name, name);
				/* no need to repeat troubleshoot message */
				goto Error;
			} else if (strcmp(name,"valid_to")==0) {
				output_error("class_define_map(oclass='%s',...): property name '%s' conflicts with built-in property", oclass->name, name);
				/* no need to repeat troubleshoot message */
				goto Error;
			} else if (strcmp(name,"latitude")==0) {
				output_error("class_define_map(oclass='%s',...): property name '%s' conflicts with built-in property", oclass->name, name);
				/* no need to repeat troubleshoot message */
				goto Error;
			} else if (strcmp(name,"longitude")==0) {
				output_error("class_define_map(oclass='%s',...): property name '%s' conflicts with built-in property", oclass->name, name);
				/* no need to repeat troubleshoot message */
				goto Error;
			} else if (strcmp(name,"in_svc")==0) {
				output_error("class_define_map(oclass='%s',...): property name '%s' conflicts with built-in property", oclass->name, name);
				/* no need to repeat troubleshoot message */
				goto Error;
			} else if (strcmp(name,"out_svc")==0) {
				output_error("class_define_map(oclass='%s',...): property name '%s' conflicts with built-in property", oclass->name, name);
				/* no need to repeat troubleshoot message */
				goto Error;
			} else if (strcmp(name,"name")==0) {
				output_error("class_define_map(oclass='%s',...): property name '%s' conflicts with built-in property", oclass->name, name);
				/* no need to repeat troubleshoot message */
				goto Error;
			} else if (strcmp(name,"flags")==0) {
				output_error("class_define_map(oclass='%s',...): property name '%s' conflicts with built-in property", oclass->name, name);
				/* no need to repeat troubleshoot message */
				goto Error;
			}
			prop = property_malloc(proptype,oclass,name,addr,delegation);
			if (prop==NULL)
				goto Error;

			/* attach to property list */
			class_add_property(oclass,prop);
			count++;

			/* save property types in case extended property comes up */
			if (prop->ptype>_PT_LAST)
				prop = NULL;
		}
	}
	va_end(arg);
	return count;
Error:
	if (prop!=NULL)
		output_error("class_define_map(oclass='%s',...): processed up to '%s' before encountering error", oclass->name, prop->name);
	return -count;
}

/** Define an enumeration member
	@return 0 on failure, 1 on success
 **/
int class_define_enumeration_member(CLASS *oclass, /**< pointer to the class which implements the enumeration */
                                    char *property_name, /**< property name of the enumeration */
                                    char *member, /**< member name to define */
                                    enumeration value) /**< enum value to associate with the name */
{
	PROPERTY *prop = class_find_property(oclass,property_name);
	KEYWORD *key = (KEYWORD*)malloc(sizeof(KEYWORD));
	if (prop==NULL || key==NULL) return 0;
	key->next = prop->keywords;
	strncpy(key->name,member,sizeof(key->name));
	key->value = value;
	prop->keywords = key;
	return 1;
}

/** Define a set member
 **/
int class_define_set_member(CLASS *oclass, /**< pointer to the class which implements the set */
                            char *property_name, /**< property name of the set */
                            char *member, /**< member name to define */
                            unsigned int64 value) /**< set value to associate with the name */
{
	PROPERTY *prop = class_find_property(oclass,property_name);
	KEYWORD *key = (KEYWORD*)malloc(sizeof(KEYWORD));
	if (prop==NULL || key==NULL) return 0;
	if (prop->keywords==NULL)
		prop->flags |= PF_CHARSET; /* enable single character keywords until a long keyword is defined */
	key->next = prop->keywords;
	strncpy(key->name,member,sizeof(key->name));
	key->name[sizeof(key->name)-1]='\0'; /* null terminate name in case is was too long for strncpy */
	if (strlen(key->name)>1 && (prop->flags&PF_CHARSET)) /* long keyword detected */
		prop->flags ^= PF_CHARSET; /* disable single character keywords */
	key->value = value;
	prop->keywords = key;
	return 1;
}

/**	Define a class function.
	@return A structure with the function pointer and the function's published name.
 */
FUNCTION *class_define_function(CLASS *oclass, FUNCTIONNAME functionname, FUNCTIONADDR call)
{
	FUNCTION *func, *tempfunc;
	if (class_get_function(oclass->name,functionname)!=NULL)
	{
		output_error("class_define_function(CLASS *class={name='%s',...}, FUNCTIONNAME functionname='%s', ...) the function name has already been defined", oclass->name, functionname);
		/* TROUBLESHOOT
			The function in question has already been defined for the class.
			Only one function of any given name is permitted in each class.
			Remove or correct the duplicate function declaration and try again.
		 */
		errno = 0;
		return NULL;
	}

	func = (FUNCTION*)malloc(sizeof(FUNCTION));
	if (func==NULL)
	{
		errno = ENOMEM;
		return NULL;
	}
	func->addr = call;
	strcpy(func->name,functionname);
	func->next = NULL;
	func->oclass = oclass;
	if (oclass->fmap==NULL)
		oclass->fmap = func;
	else if (oclass->fmap->next == NULL)
		oclass->fmap->next = func;
	else	//More than one attached
	{
		tempfunc = oclass->fmap;
		while (tempfunc->next != NULL)
			tempfunc = tempfunc->next;

		tempfunc->next = func;
	}

	return func;
}

/* Get the entry point of a class function
 */
FUNCTIONADDR class_get_function(char *classname, char *functionname)
{
	CLASS *oclass = class_get_class_from_classname(classname);
	FUNCTION *func;
	for (func=oclass->fmap; func!=NULL && func->oclass==oclass; func=func->next)
	{
		if (strcmp(functionname,func->name)==0)
			return func->addr;
	}
	errno = ENOENT;
	return NULL;
}

/** Save all class information to a stream in \b glm format
	@return the number of characters written to the stream
 **/
int class_saveall(FILE *fp) /**< a pointer to the stream FILE structure */
{
	unsigned count=0;
	count += fprintf(fp,"\n########################################################\n");
	count += fprintf(fp,"# classes\n");
	{	CLASS	*oclass;
		for (oclass=class_get_first_class(); oclass!=NULL; oclass=oclass->next)
		{
			PROPERTY *prop;
			FUNCTION *func;
			count += fprintf(fp,"class %s {\n",oclass->name);
			if (oclass->parent)
				count += fprintf(fp,"\tparent %s;\n", oclass->parent->name);
			for (func=oclass->fmap; func!=NULL && func->oclass==oclass; func=func->next)
				count += fprintf(fp, "\tfunction %s();\n", func->name);
			for (prop=oclass->pmap; prop!=NULL && prop->oclass==oclass; prop=prop->next)
			{
				char *propname = class_get_property_typename(prop->ptype);
				if (propname!=NULL)
					count += fprintf(fp,"\t%s %s;\n", propname, prop->name);
			}
			count += fprintf(fp,"}\n");
		}
	}
	return count;
}

/** Save all class information to a stream in \b xml format
	@return the number of characters written to the stream
 **/
int class_saveall_xml(FILE *fp) /**< a pointer to the stream FILE structure */
{
	unsigned count=0;
	count += fprintf(fp,"\t<classes>\n");
	{	CLASS	*oclass;
		for (oclass=class_get_first_class(); oclass!=NULL; oclass=oclass->next)
		{
			PROPERTY *prop;
			FUNCTION *func;
			count += fprintf(fp,"\t\t<class name=\"%s\">\n",oclass->name);
			if (oclass->parent)
				count += fprintf(fp,"\t\t<parent>%s</parent>\n", oclass->parent->name);
			for (func=oclass->fmap; func!=NULL && func->oclass==oclass; func=func->next)
				count += fprintf(fp, "\t\t<function>%s</function>\n", func->name);
			for (prop=oclass->pmap; prop!=NULL && prop->oclass==oclass; prop=prop->next)
			{
				char *propname = class_get_property_typename(prop->ptype);
				if (propname!=NULL)
					count += fprintf(fp,"\t\t\t<property type=\"%s\">%s</property>\n", propname, prop->name);
			}
			count += fprintf(fp,"\t\t</class>\n");
		}
	}
	count += fprintf(fp,"\t</classes>\n");
	return count;
}

/** Generate profile information for the classes used
 **/
void class_profiles(void)
{
	CLASS *cl;
	int64 total=0;
	int count=0, i=0, hits;
	CLASS **index;
	output_profile("Model profiler results");
	output_profile("======================\n");
	output_profile("Class            Time (s) Time (%%) msec/obj");
	output_profile("---------------- -------- -------- --------");
	for (cl=first_class; cl!=NULL; cl=cl->next)
	{
		total+=cl->profiler.clocks;
		count++;
	}
	if(0 == count){
		return;	// short-circuit
	}
	index = (CLASS**)malloc(sizeof(CLASS*)*count);
	if(0 == index){
		// error
		return;
	}
	for (cl=first_class; cl!=NULL; cl=cl->next)
		index[i++]=cl;
	hits=-1;
	while (hits!=0)
	{
		hits=0;
		for (i=0; i<count-1; i++)
		{
			if (index[i]->profiler.clocks<index[i+1]->profiler.clocks)
			{
				CLASS *tmp = index[i];
				index[i]=index[i+1];
				index[i+1]=tmp;
				hits++;
			}
		}
	}
	for (i=0; i<count; i++)
	{
		cl = index[i];
		if (cl->profiler.clocks>0)
		{
			double ts = (double)cl->profiler.clocks/CLOCKS_PER_SEC;
			double tp = (double)cl->profiler.clocks/total*100;
			double mt = ts/cl->profiler.numobjs*1000;
			output_profile("%-16.16s %7.3f %8.1f%% %8.1f", cl->name, ts,tp,mt);
		}
		else
			break;
	}
	free(index);
	index = NULL;
	output_profile("================ ======== ======== ========");
	output_profile("%-16.16s %7.3f %8.1f%% %8.1f\n",
		"Total", (double)total/CLOCKS_PER_SEC,100.0,1000*(double)total/CLOCKS_PER_SEC/object_get_count());

}

/** Register a type delegation for a property
	@return a pointer DELEGATEDTYPE struct if successful, \p NULL if delegation failed

	Type delegation is used to transform data to string and string to data conversion
	to routines implemented in a module, instead of in the core.   This allows custom
	data type to be implemented, including enumerations, sets, and special objects.
 **/
DELEGATEDTYPE *class_register_type(CLASS *oclass, /**< the object class */
                                   char *type, /**< the property type */
                                   int (*from_string)(void*,char*), /**< the converter from string to data */
                                   int (*to_string)(void*,char*,int)) /**< the converter from data to string */
{
	DELEGATEDTYPE *dt = (DELEGATEDTYPE*)malloc(sizeof(DELEGATEDTYPE));
	if (dt!=NULL)
	{
		dt->oclass = oclass;
		strncpy(dt->type,type,sizeof(dt->type));
		dt->from_string = from_string;
		dt->to_string = to_string;
	}
	else
		output_error("unable to register delegated type (memory allocation failed)");
		/*	TROUBLESHOOT
			Property delegation is not supported yet so this should never happen.
			This is most likely caused by a lack of memory or an unstable system.
		 */
	return dt;
}

/* this is not supported */
int class_define_type(CLASS *oclass, DELEGATEDTYPE *delegation, ...)
{
	output_error("delegated types not supported using class_define_type (use class_define_map instead)");
	/*	TROUBLESHOOT
			Property delegation is not supported yet so this should never happen.
			This is most likely caused by a lack of memory or an unstable system.
	 */
	return 0;
}

static int check = 0;  /* there must be a better way to do this, but this works. -MH */

/**	Writes a formatted string into a temporary buffer prior and verifies that enough space exists prior to writing to the destination.
	@return the number of characters written to the buffer
 **/
static int buffer_write(char *buffer, /**< buffer into which string is written */
                        size_t len,   /**< size of the buffer into which the string is written */
                        char *format, /**< format of string to write into buffer, followed by the variable arguments */
                        ...)
{
	char temp[1025];
	unsigned int count = 0;
	va_list ptr;

	if(buffer == NULL)
		return 0;
	if(len < 1)
		return 0;
	if(check == 0)
		return 0;

	va_start(ptr,format);
	count = vsprintf(temp, format, ptr);
	va_end(ptr);

	if(count < len){
		strncpy(buffer, temp, count);
		return count;
	} else {
		check = 0;
		return 0;
	}
}

/** Generate the XSD snippet of a class
	@return the number of characters written to the buffer
 **/
int class_get_xsd(CLASS *oclass, /**< a pointer to the class to convert to XSD */
				  char *buffer, /**< a pointer to the first character in the buffer */
				  size_t len) /**< the size of the buffer */
{
	size_t n=0;
	PROPERTY *prop;
	int i;
	CLASS *oc = oclass;
	extern KEYWORD oflags[];
	struct {
		char *name;
		char *type;
		KEYWORD *keys;
	} attribute[]={
		{"id", "integer",NULL},
		{"parent", "string",NULL},
		{"rank", "integer",NULL},
		{"clock", "string",NULL},
		{"valid_to", "string",NULL},
		{"latitude", "string",NULL},
		{"longitude", "string",NULL},
		{"in_svc", "string",NULL},
		{"out_svc", "string",NULL},
		{"flags", "string",oflags},
	};
	check = 1;
	n += buffer_write(buffer+n, len-n, "<xs:element name=\"%s\">\n", oclass->name);
	n += buffer_write(buffer+n, len-n, "\t<xs:complexType>\n");
	n += buffer_write(buffer+n, len-n, "\t\t<xs:all>\n");
	for (i=0; i < sizeof(attribute) / sizeof(attribute[0]); i++)
	{
		n += buffer_write(buffer+n, len-n, "\t\t\t<xs:element name=\"%s\">\n", attribute[i].name);
		n += buffer_write(buffer+n, len-n, "\t\t\t\t<xs:simpleType>\n");
		if (attribute[i].keys==NULL){
			n += buffer_write(buffer+n, len-n, "\t\t\t\t\t<xs:restriction base=\"xs:%s\"/>\n", attribute[i].type);
		}
		else
		{
			KEYWORD *key;
			n += buffer_write(buffer+n, len-n, "\t\t\t\t\t<xs:restriction base=\"xs:string\">\n");
			n += buffer_write(buffer+n, len-n, "\t\t\t\t\t\t<xs:pattern value=\"");
			for (key=attribute[i].keys; key!=NULL; key=key->next){
				n += buffer_write(buffer+n, len-n, "%s%s", key==attribute[i].keys?"":"|", key->name);
			}
			n += buffer_write(buffer+n, len-n, "\"/>\n");
			n += buffer_write(buffer+n, len-n, "\t\t\t\t\t</xs:restriction>\n");
		}
		n += buffer_write(buffer+n, len-n, "\t\t\t\t</xs:simpleType>\n");
		n += buffer_write(buffer+n, len-n, "\t\t\t</xs:element>\n");
	}
	for(; oc != 0; oc = oc->parent){
		for (prop=oc->pmap; prop!=NULL && prop->oclass==oc; prop=prop->next)
		{
			char *proptype=class_get_property_typexsdname(prop->ptype);
			if (prop->unit!=NULL){
				n += buffer_write(buffer+n, len-n, "\t\t\t\t<xs:element name=\"%s\" type=\"xs:string\"/>\n", prop->name);
			} else {
				n += buffer_write(buffer+n, len-n, "\t\t\t<xs:element name=\"%s\">\n", prop->name);
				n += buffer_write(buffer+n, len-n, "\t\t\t\t<xs:simpleType>\n");
				n += buffer_write(buffer+n, len-n, "\t\t\t\t\t<xs:restriction base=\"xs:%s\">\n", proptype==NULL?"string":proptype);
				if (prop->keywords!=NULL)
				{
					KEYWORD *key;
					n += buffer_write(buffer+n, len-n, "\t\t\t\t\t<xs:pattern value=\"");
					for (key=prop->keywords; key!=NULL; key=key->next){
						n += buffer_write(buffer+n, len-n, "%s%s", key==prop->keywords?"":"|", key->name);
					}
					n += buffer_write(buffer+n, len-n, "\"/>\n");
				}
				n += buffer_write(buffer+n, len-n, "\t\t\t\t\t</xs:restriction>\n");
				n += buffer_write(buffer+n, len-n, "\t\t\t\t</xs:simpleType>\n");
				n += buffer_write(buffer+n, len-n, "\t\t\t</xs:element>\n");
			}
		}
	}
	n += buffer_write(buffer+n, len-n, "\t\t</xs:all>\n");
	n += buffer_write(buffer+n, len-n, "\t</xs:complexType>\n");
	n += buffer_write(buffer+n, len-n, "</xs:element>\n");
	buffer[n] = 0;
	if(check == 0){
		printf("class_get_xsd() overflowed.\n");
		buffer[0] = 0;
		return 0;
	}
	return (int)n;
}


/**@}**/
