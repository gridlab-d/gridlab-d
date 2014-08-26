/** $Id: object.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file object.c
	@addtogroup object Objects
	@ingroup core
	
	Object functions support object operations.  Objects have two parts, an #OBJECTHDR
	block followed by an #OBJECTDATA block.  The #OBJECTHDR contains all the common
	object information, such as it's id and clock.  The #OBJECTDATA contains all the
	data implemented by the module that created the object.

	@code
	OBJECTHDR		size						size of the OBJECTDATA block
					id							unique id of the object
					oclass						class the implements the OBJECTDATA
					next						pointer to the next OBJECTHDR
					parent						pointer to parent's OBJECTHDR
					rank						object's rank (less than parent's rank)
					clock						object's sync clock
					latitude, longitude			object's geo-coordinates
					in_svc, out_svc				object's activation/deactivation dates
					flags						object flags (e.g., external PLC active)
	OBJECTDATA		(varies; defined by oclass)
	@endcode
 @{
 **/

#include <float.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>

#ifdef WIN32
#define isnan _isnan  /* map isnan to appropriate function under Windows */
#endif

#include "object.h"
#include "convert.h"
#include "output.h"
#include "globals.h"
#include "module.h"
#include "lock.h"

/* object list */
static OBJECTNUM next_object_id = 0;
static OBJECTNUM deleted_object_count = 0;
static OBJECT *first_object = NULL;
static OBJECT *last_object = NULL;
static OBJECTNUM object_array_size = 0;
static OBJECT **object_array = NULL;

/* {name, val, next} */
KEYWORD oflags[] = {
	/* "name", value, next */
	{"NONE", OF_NONE, oflags + 1},
	{"HASPLC", OF_HASPLC, oflags + 2},
	{"LOCKED", OF_LOCKED, oflags + 3},
	{"RERANKED", OF_RERANK, oflags + 4},
	{"RECALC", OF_RECALC, oflags + 5},
	{"DELTAMODE", OF_DELTAMODE, NULL},
};

/* WARNING: untested. -d3p988 30 Jan 08 */
int object_get_oflags(KEYWORD **extflags){
	int flag_size = sizeof(oflags);
	
	*extflags = module_malloc(flag_size);
	
	if(extflags == NULL){
		output_error("object_get_oflags: malloc failure");
		errno = ENOMEM;
		return -1;
	}
	
	memcpy(*extflags, oflags, flag_size);
	
	return flag_size / sizeof(KEYWORD); /* number of items written */
}

PROPERTY *object_flag_property(){
	static PROPERTY flags = {0, "flags", PT_set, 1, 8, PA_PUBLIC, NULL, (void*)-4, NULL, oflags, NULL};
	
	return &flags;
}

KEYWORD oaccess[] = {
	/* "name", value, next */
	{"PUBLIC", PA_PUBLIC, oaccess + 1},
	{"REFERENCE", PA_REFERENCE, oaccess + 2},
	{"PROTECTED", PA_PROTECTED, oaccess + 3},
	{"PRIVATE", PA_PRIVATE, NULL},
};

PROPERTY *object_access_property(){
	static PROPERTY flags = {0, "access", PT_enumeration, 1, 8, PA_PUBLIC, NULL, (void*) -4, NULL, oaccess, NULL};
	
	return &flags;
}

/* prototypes */
void object_tree_delete(OBJECT *obj, OBJECTNAME name);

/** Get the number of objects defined 

	@return the number of objects in the model
 **/
unsigned int object_get_count(){
	return next_object_id - deleted_object_count;
}

/** Get a named property of an object.  

	Note that you must use object_get_value_by_name to retrieve the value of
	the property.  If part is specified, failed searches for given name will
	be parsed for a part.

	@return a pointer to the PROPERTY structure
 **/
PROPERTY *object_get_property(OBJECT *obj, /**< a pointer to the object */
							  PROPERTYNAME name, /**< the name of the property */
							  PROPERTYSTRUCT *pstruct) /** buffer in which to store part info, if found */
{ 
	if(obj == NULL){
		return NULL;
	} else {
		char *part;
		PROPERTYNAME root;
		PROPERTY *prop = class_find_property(obj->oclass, name);
		PROPERTYSPEC *spec;
		if ( pstruct ) { pstruct->prop=prop; pstruct->part[0]='\0'; }
		if ( prop ) return prop;

		/* property not found, but part structure was not requested either */
		if ( pstruct==NULL ) return NULL;

		/* possible part specified, so search for it */
		strcpy(root,name);
		part = strrchr(root,'.');
		if ( !part ) return NULL; /* no part, no result */
		
		/* part is apparently valid */
		*part++='\0';

		/* check the root */
		prop = class_find_property(obj->oclass, root);
		if ( !prop ) return NULL; /* root isn't valid either */

		/* check part directly (note this fails if the part is valid but the double is NaN) */
		spec = property_getspec(prop->ptype);
		if ( spec->get_part==NULL || spec->get_part(obj,part)==QNAN ) return NULL;
		
		/* part is valid */
		pstruct->prop = prop;
		strncpy(pstruct->part,part,sizeof(pstruct->part));

		return prop;
	}
}

/**	This will build (or rebuild) an array with all the instantiated GridLab-D objects
	placed at indices that correspond to their internal object ID.
	
	@return the number of objects instantiated when the call was made
*/
int object_build_object_array(){
	unsigned int tcount = object_get_count();
	unsigned int i = 0;
	OBJECT *optr = object_get_first();
	
	if(object_array != NULL){
		free(object_array);
		object_array = NULL;
	}
	
	object_array = malloc(sizeof(OBJECT *) * tcount);
	
	if(object_array == NULL){
		return 0;
	}
	
	object_array_size = tcount;
	
	for(i = 0; i < tcount; ++i){
		object_array[i] = optr;
		optr = optr->next;
	}
	
	return object_array_size;
}


PROPERTY *object_prop_in_class(OBJECT *obj, PROPERTY *prop){
	if(prop == NULL){
		return NULL;
	}
	
	if(obj != NULL){
		return class_prop_in_class(obj->oclass, prop);
	} else {
		return NULL;
	}
}

/** Find an object by its id number
	@return a pointer the object
 **/
OBJECT *object_find_by_id(OBJECTNUM id){ /**< object id number */
	OBJECT *obj;
	
	if(object_get_count() == object_array_size){
		if(id < object_array_size){
			return object_array[id];
		} else {
			return NULL;
		}
	} else {
		/* this either fails or sets object_array_size to object_get_count() */
		if(object_build_object_array())
			return object_find_by_id(id);
	}
	
	for(obj = first_object; obj != NULL; obj = obj->next){
		if(obj->id == id){
			return obj; /* "break"*/
		}
	}
	
	return NULL;
}


/** Get the name of an object. 

	@return a pointer to the object name string
 **/
char *object_name(OBJECT *obj, char *oname, int size){ /**< a pointer to the object */
	//static char32 oname="(invalid)";
	
	convert_from_object(oname, size, &obj, NULL);
	
	return oname;
}

/** Get the unit of an object, if any
 **/
char *object_get_unit(OBJECT *obj, char *name)
{
	static UNIT *dimless = NULL;
	unsigned int unitlock = 0;
	PROPERTY *prop = object_get_property(obj, name,NULL);
	
	if(prop == NULL){
		char *buffer = (char *)malloc(64);
		memset(buffer, 0, 64);
		throw_exception("property '%s' not found in object '%s'", name, object_name(obj, buffer, 63));
		/* TROUBLESHOOT
			The property for which the unit was requested does not exist.  
			Depending on where this occurs it's either a bug or an error in the model.
			Try fixing your model and try again.  If the problem persists, report it.
		 */
	}
	
	rlock(&unitlock);
	if(dimless == NULL){
		runlock(&unitlock);
		wlock(&unitlock);
		dimless=unit_find("1");
		wunlock(&unitlock);
	}
	else 
		runlock(&unitlock);
	
	if(prop->unit != NULL){
		return prop->unit->name;
	} else {
		return dimless->name;
	}
}

/** Create a single object.
	@return a pointer to object header, \p NULL of error, set \p errno as follows:
	- \p EINVAL type is not valid
	- \p ENOMEM memory allocation failed
 **/
OBJECT *object_create_single(CLASS *oclass){ /**< the class of the object */
	/* @todo support threadpool during object creation by calling this malloc from the appropriate thread */
	OBJECT *obj = 0;
	static int tp_next = 0;
	static int tp_count = 0;
	PROPERTY *prop;
	int sz = sizeof(OBJECT);

	if(tp_count == 0){
		tp_count = processor_count();
	}

	if(oclass == NULL){
		throw_exception("object_create_single(CLASS *oclass=NULL): class is NULL");
		/* TROUBLESHOOT
			An attempt to create an object was given a NULL pointer for the class.  
			This is most likely a bug and should be reported.
		 */
	}
	if ( oclass->passconfig&PC_ABSTRACTONLY )
	{
		throw_exception("object_create_single(CLASS *oclass='%s'): abstract class '%s' cannot be instantiated", oclass->name);
		/* TROUBLESHOOT
			An attempt to create an object using an abstract class was detected.
			Some classes may only be inherited but cannot be used directly in models.
		*/
	}

	obj = (OBJECT*)malloc(sz + oclass->size);

	if(obj == NULL){
		throw_exception("object_create_single(CLASS *oclass='%s'): memory allocation failed", oclass->name);
		/* TROUBLESHOOT
			The system has run out of memory and is unable to create the object requested.  Try freeing up system memory and try again.
		 */
	}

	memset(obj, 0, sz + oclass->size);

	tp_next %= tp_count;

	obj->id = next_object_id++;
	obj->oclass = oclass;
	obj->next = NULL;
	obj->name = NULL;
	obj->parent = NULL;
	obj->rank = 0;
	obj->clock = 0;
	obj->latitude = QNAN;
	obj->longitude = QNAN;
	obj->in_svc = TS_ZERO;
	obj->in_svc_micro = 0;
	obj->in_svc_double = (double)obj->in_svc;
	obj->out_svc = TS_NEVER;
	obj->out_svc_micro = 0;
	obj->out_svc_double = (double)obj->out_svc;
	obj->space = object_current_namespace();
	obj->flags = OF_NONE;
	obj->rng_state = randwarn(NULL);
	obj->heartbeat = 0;

	for (prop=obj->oclass->pmap; prop!=NULL; prop=(prop->next?prop->next:(prop->oclass->parent?prop->oclass->parent->pmap:NULL)))
		property_create(prop,(void*)((char *)(obj+1)+(int64)(prop->addr)));
	
	if(first_object == NULL){
		first_object = obj;
	} else {
		last_object->next = obj;
	}
	
	last_object = obj;
	oclass->profiler.numobjs++;
	
	return obj;
}

/** Create a foreign object.

	@return a pointer to object header, \p NULL of error, set \p errno as follows:
	- \p EINVAL type is not valid
	- \p ENOMEM memory allocation failed

	To use this function you must set \p obj->oclass before calling.  All other properties 
	will be cleared and you must set them after the call is completed.
 **/
OBJECT *object_create_foreign(OBJECT *obj) /**< a pointer to the OBJECT data structure */
{	

	if(obj == NULL){
		throw_exception("object_create_foreign(OBJECT *obj=NULL): object is NULL");
		/* TROUBLESHOOT
			An attempt to create an object was given a NULL pointer for the class.  
			This is most likely a bug and should be reported.
		 */
	}
	
	if(obj->oclass == NULL){
		throw_exception("object_create_foreign(OBJECT *obj=<new>): object->oclass is NULL");
		/* TROUBLESHOOT
			The system has run out of memory and is unable to create the object requested.  Try freeing up system memory and try again.
		 */
	}
	
	if(obj->oclass->magic!=CLASSVALID)
		throw_exception("object_create_foreign(OBJECT *obj=<new>): obj->oclass is not really a class");
		/* TROUBLESHOOT
			An attempt to create an object was given a class that most likely is not a class.  
			This is most likely a bug and should be reported.
		 */

	memset(obj->synctime,0,sizeof(obj->synctime));

	obj->id = next_object_id++;
	obj->next = NULL;
	obj->name = NULL;
	obj->parent = NULL;
	obj->rank = 0;
	obj->clock = 0;
	obj->latitude = QNAN;
	obj->longitude = QNAN;
	obj->in_svc = TS_ZERO;
	obj->in_svc_micro = 0;
	obj->in_svc_double = (double)obj->in_svc;
	obj->out_svc = TS_NEVER;
	obj->out_svc_micro = 0;
	obj->out_svc_double = (double)obj->out_svc;
	obj->flags = OF_FOREIGN;
	
	if(first_object == NULL){
		first_object = obj;
	} else {
		last_object->next = obj;
	}
	
	last_object = obj;
	obj->oclass->profiler.numobjs++;
	
	return obj;
}

/** Stream fixup object
 **/
void object_stream_fixup(OBJECT *obj, char *classname, char *objname)
{
	obj->oclass = class_get_class_from_classname(classname);
	obj->name = (char*)malloc(strlen(objname)+1);
	strcpy(obj->name,objname);
	obj->next = NULL;
	if ( first_object==NULL )
		first_object = obj;
	else
		last_object->next = obj;
	last_object = obj;
}

/** Create multiple objects.
	
	@return Same as create_single, but returns the first object created.
 **/
OBJECT *object_create_array(CLASS *oclass, /**< a pointer to the CLASS structure */
							unsigned int n_objects){ /**< the number of objects to create */
	OBJECT *first = NULL;
	
	while(n_objects-- > 0){
		OBJECT *obj = object_create_single(oclass);
		
		if(obj == NULL){
			return NULL;
		} else if(first == NULL){
			first = obj;
		}
	}
	return first;
}

/** Removes a single object.
	@return Returns the object after the one that was removed.
 **/
OBJECT *object_remove_by_id(OBJECTNUM id){
	//output_error("object_remove_by_id not yet supported");
	OBJECT *target = object_find_by_id(id);
	OBJECT *prev = NULL;
	OBJECT *next = NULL;
	
	if(target != NULL){
		char name[64] = "";
		
		if(first_object == target){
			first_object = target->next;
		} else {
			for(prev = first_object; (prev->next != NULL) && (prev->next != target); prev = prev->next){
				; /* find the object that points to the item being removed */
			}
		}
		
		object_tree_delete(target, target->name ? target->name : (sprintf(name, "%s:%d", target->oclass->name, target->id), name));
		next = target->next;
		prev->next = next;
		target->oclass->profiler.numobjs--;
		free(target);
		target = NULL;
		deleted_object_count++;
	}
	
	return next;
}

/** Get the address of a property value
	@return \e void pointer to the data; \p NULL is not found
 **/
void *object_get_addr(OBJECT *obj, /**< object to look in */
					  char *name){ /**< name of property to find */
	PROPERTY *prop;
	if(obj == NULL)
		return NULL;

	prop = class_find_property(obj->oclass,name);
	
	if(prop != NULL && prop->access != PA_PRIVATE){
		return (void *)((char *)(obj + 1) + (int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	} else {
		errno = ENOENT;
		return NULL;
	}
}

OBJECT **object_get_object(OBJECT *obj, PROPERTY *prop)
{
	int64 o = (int64)obj;
	int64 s = (int64)sizeof(OBJECT);
	int64 a = (int64)(prop->addr);
	int64 i = o + s + a;
	
	if(object_prop_in_class(obj, prop) && prop->ptype == PT_object && prop->access != PA_PRIVATE){
		return (OBJECT **)i;
	} else {	
		errno = ENOENT;
		return NULL;
	}
}

OBJECT **object_get_object_by_name(OBJECT *obj, char *name)
{
	PROPERTY *prop = class_find_property(obj->oclass, name);
	
	if(prop != NULL && prop->access != PA_PRIVATE && prop->ptype == PT_object){
		return (OBJECT **)((char *)obj + sizeof(OBJECT) + (int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	} else {
		errno = ENOENT;
		return NULL;
	}
}

enumeration *object_get_enum(OBJECT *obj, PROPERTY *prop){
	if(object_prop_in_class(obj, prop) && prop->ptype == PT_enumeration && prop->access != PA_PRIVATE){
		return (enumeration *)((char *)(obj) + sizeof(OBJECT) + (int64)(prop->addr));
	} else {
		errno = ENOENT;
		return NULL;
	}
}

enumeration *object_get_enum_by_name(OBJECT *obj, char *name){
	PROPERTY *prop = class_find_property(obj->oclass, name);

	if(prop != NULL && prop->access != PA_PRIVATE){
		return (enumeration *)((char *)(obj) + sizeof(OBJECT) + (int64)(prop->addr));
	} else {
		errno = ENOENT;
		return NULL;
	}
}

set *object_get_set(OBJECT *obj, PROPERTY *prop){
	if(object_prop_in_class(obj, prop) && prop->ptype == PT_set && prop->access != PA_PRIVATE){
		return (set *)((char *)(obj) + sizeof(OBJECT) + (int64)(prop->addr));
	} else {
		errno = ENOENT;
		return NULL;
	}
}

set *object_get_set_by_name(OBJECT *obj, char *name){
	PROPERTY *prop = class_find_property(obj->oclass, name);

	if(prop != NULL && prop->access != PA_PRIVATE){
		return (set *)((char *)(obj) + sizeof(OBJECT) + (int64)(prop->addr));
	} else {
		errno = ENOENT;
		return NULL;
	}
}

/* Get the pointer to the value of a 16-bit integer property. 
 * Returns NULL if the property is not found or if the value the right type.
 */
int16 *object_get_int16(OBJECT *obj, PROPERTY *prop)
{
	if(object_prop_in_class(obj, prop) && prop->ptype == PT_int16 && prop->access != PA_PRIVATE){
		return (int16 *)((char *)obj+sizeof(OBJECT)+(int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	} else {
		errno = ENOENT;
		return NULL;
	}
}

int16 *object_get_int16_by_name(OBJECT *obj, char *name)
{
	PROPERTY *prop = class_find_property(obj->oclass, name);
	
	if(prop != NULL && prop->access != PA_PRIVATE){
		return (int16 *)((char *)obj + sizeof(OBJECT) + (int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	} else {
		errno = ENOENT;
		return NULL;
	}
}

/* Get the pointer to the value of a 32-bit integer property. 
 * Returns NULL if the property is not found or if the value the right type.
 */
int32 *object_get_int32(OBJECT *obj, PROPERTY *prop)
{
	if(object_prop_in_class(obj, prop) && prop->ptype == PT_int32 && prop->access != PA_PRIVATE){
		return (int32 *)((char *)obj + sizeof(OBJECT) + (int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	} else {
		errno = ENOENT;
		return NULL;
	}
}

int32 *object_get_int32_by_name(OBJECT *obj, char *name)
{
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop!=NULL && prop->access != PA_PRIVATE)
		return (int32 *)((char*)obj+sizeof(OBJECT)+(int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	errno = ENOENT;
	return NULL;
}

/* Get the pointer to the value of a 64-bit integer property. 
 * Returns NULL if the property is not found or if the value the right type.
 */
int64 *object_get_int64(OBJECT *obj, PROPERTY *prop)
{
	if(object_prop_in_class(obj, prop) && prop->ptype == PT_int64 && prop->access != PA_PRIVATE){
		return (int64 *)((char *)obj + sizeof(OBJECT) + (int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	} else {
		errno = ENOENT;
		return NULL;
	}
}

int64 *object_get_int64_by_name(OBJECT *obj, char *name)
{
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop!=NULL && prop->access != PA_PRIVATE)
		return (int64 *)((char*)obj+sizeof(OBJECT)+(int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	errno = ENOENT;
	return NULL;
}

/* Get the pointer to the value of a double property. 
 * Returns NULL if the property is not found or if the value the right type.
 */
double *object_get_double_quick(OBJECT *obj, PROPERTY *prop)
{	/* no checks */
	return (double*)((char*)obj+sizeof(OBJECT)+(int64)(prop->addr));
}

double *object_get_double(OBJECT *obj, PROPERTY *prop)
{
	if(object_prop_in_class(obj, prop) && (prop->ptype==PT_double||prop->ptype==PT_random) && prop->access != PA_PRIVATE)
		return (double*)((char*)obj+sizeof(OBJECT)+(int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	errno = ENOENT;
	return NULL;
}

double *object_get_double_by_name(OBJECT *obj, char *name)
{
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop!=NULL && prop->access != PA_PRIVATE)
		return (double *)((char*)obj+sizeof(OBJECT)+(int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	errno = ENOENT;
	return NULL;
}

/* Get the pointer to the value of a complex property. 
 * Returns NULL if the property is not found or if the value the right type.
 */
complex *object_get_complex_quick(OBJECT *obj, PROPERTY *prop)
{	/* no checks */
	return (complex*)((char*)obj+sizeof(OBJECT)+(int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
}

complex *object_get_complex(OBJECT *obj, PROPERTY *prop)
{
	if(object_prop_in_class(obj, prop) && prop->ptype==PT_complex && prop->access != PA_PRIVATE)
		return (complex*)((char*)obj+sizeof(OBJECT)+(int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	errno = ENOENT;
	return NULL;
}

complex *object_get_complex_by_name(OBJECT *obj, char *name)
{
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop!=NULL && prop->access != PA_PRIVATE)
		return (complex *)((char*)obj+sizeof(OBJECT)+(int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	errno = ENOENT;
	return NULL;
}

char *object_get_string(OBJECT *obj, PROPERTY *prop){
	if(object_prop_in_class(obj, prop) && prop->ptype >= PT_char8 && prop->ptype <= PT_char1024 && prop->access != PA_PRIVATE)
		return (char *)((char*)obj+sizeof(OBJECT)+(int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	errno = ENOENT;
	return NULL;
}

/* Get the pointer to the value of a string property. 
 * Returns NULL if the property is not found or if the value the right type.
 */
char *object_get_string_by_name(OBJECT *obj, char *name)
{
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop!=NULL && prop->access != PA_PRIVATE)
		return ((char*)obj+sizeof(OBJECT)+(int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	errno = ENOENT;
	return NULL;
}

/* this function finds the property associated with the addr of an object member */
static PROPERTY *get_property_at_addr(OBJECT *obj, void *addr)
{
	PROPERTY *prop = NULL;
	int64 offset = (int)((char*)addr - (char*)(obj+1));

	/* reuse last result if possible */
	if(prop!=NULL && object_prop_in_class(obj, prop) && (int64)(prop->addr) == offset && prop->access != PA_PRIVATE)  /* warning: cast from pointer to integer of different size */
		return prop;

	/* scan through properties of this class and stop when no more properties or class changes */
	for (prop=obj->oclass->pmap; prop!=NULL; prop=(prop->next->oclass==prop->oclass?prop->next:NULL))
	{
		if((int64)(prop->addr)==offset) /* warning: cast from pointer to integer of different size */
			if(prop->access != PA_PRIVATE)
				return prop;
			else {
				output_error("trying to get the private property %s in %s", prop->name, obj->oclass->name);
				/*	TROUBLESHOOT
					The specified property was published by its object as private.  Though it may be read at the end of the simulation
					and by other classes in the module, other modules do not have permission to access that property.
				*/
				return 0;
			}
	}
	return NULL;
}

/** Set a property value by reference to its physical address
	@return the character written to the buffer
 **/
int object_set_value_by_addr(OBJECT *obj, /**< the object to alter */
							 void *addr, /**< the address of the property */
							 char *value, /**< the value to set */
							 PROPERTY *prop) /**< the property to use or NULL if unknown */
{
	int result=0;
	if(prop==NULL && (prop=get_property_at_addr(obj,addr))==NULL) 
		return 0;
	if(prop->access != PA_PUBLIC){
		output_error("trying to set the value of non-public property %s in %s", prop->name, obj->oclass->name);
		/*	TROUBLESHOOT
			The specified property was published by its object as private.  It may not be modified by other modules.
		*/
		return 0;
	}

	/* set the recalc bit if the property has a recalc trigger */
	if(prop->flags&PF_RECALC) obj->flags |= OF_RECALC;

	/* dispatch notifiers */
	if(obj->oclass->notify){
		if(obj->oclass->notify(obj,NM_PREUPDATE,prop,value) == 0){
			output_error("preupdate notify failure on %s in %s", prop->name, obj->name ? obj->name : "an unnamed object");
		}
	}
	// this happens BEFORE the value is set, so that we can avoid values that would
	//	put the object into an invalid state.  Also to adjust related values with
	//	zero lag.
	if(prop->notify){
		if(prop->notify(obj, value) == 0){
			output_error("property notify_%s_%s failure in %s", obj->oclass->name, prop->name, (obj->name ? obj->name : "an unnamed object"));
		}
	}
	if(prop->notify_override != true){
		result = class_string_to_property(prop,addr,value);
	}
	if(obj->oclass->notify){
		if(obj->oclass->notify(obj,NM_POSTUPDATE,prop,value) == 0){
			output_error("postupdate notify failure on %s in %s", prop->name, obj->name ? obj->name : "an unnamed object");
		}
	}
	return result;
}

static int set_header_value(OBJECT *obj, char *name, char *value)
{
	unsigned int temp_microseconds;
	TIMESTAMP tval;
	double tval_double;

	if(strcmp(name,"name")==0)
	{
		if(obj->name!=NULL)
		{
			output_error("object %s:d name already set to %s", obj->oclass->name, obj->id, obj->name);
			/*	TROUBLESHOOT
				Object definitions within model files may only have name specifiers once.  They may not be named multiple
				times within the object file.
			*/
			return FAILED;
		}
		else
		{
			object_set_name(obj,value);
			return SUCCESS;
		}
	}
	else if(strcmp(name,"parent")==0)
	{
		OBJECT *parent=object_find_name(value);
		if(parent==NULL && strcmp(value,"")!=0)
		{
			output_error("object %s:%d parent %s not found", obj->oclass->name, obj->id, value);
			return FAILED;
		}
		else if(object_set_parent(obj,parent)==FAILED && strcmp(value,"")!=0)
		{
			output_error("object %s:%d cannot use parent %s", obj->oclass->name, obj->id, value);
			return FAILED;
		}
		else
			return SUCCESS;
	}
	else if(strcmp(name,"rank")==0)
	{
		if(object_set_rank(obj,atoi(value))<0)
		{
			output_error("object %s:%d rank '%s' is invalid", obj->oclass->name, obj->id, value);
			return FAILED;
		}
		else
			return SUCCESS;
	}
	else if(strcmp(name,"clock")==0)
	{
		if((obj->clock = convert_to_timestamp(value))==TS_INVALID)
		{
			output_error("object %s:%d clock timestamp '%s' is invalid", obj->oclass->name, obj->id, value);
			return FAILED;
		}
		else
			return SUCCESS;
	}
	else if(strcmp(name,"valid_to")==0)
	{
		if((obj->valid_to = convert_to_timestamp(value))==TS_INVALID)
		{
			output_error("object %s:%d valid_to timestamp '%s' is invalid", obj->oclass->name, obj->id, value);
			return FAILED;
		}
		else
			return SUCCESS;
	}
	else if(strcmp(name,"latitude")==0)
	{
		if((obj->latitude = convert_to_latitude(value))==QNAN)
		{
			output_error("object %s:%d latitude '%s' is invalid", obj->oclass->name, obj->id, value);
			/*	TROUBLESHOOT
				Latitudes are expected to be in the format of X{N,S, }Y'Z".  The seconds may be excluded.
				If the seconds are excluded, minutes may be excluded.  If minutes are excluded, the
				north/south indicator may be excluded.  There must be only one space between the degree
				value and the minute value.  Negative degrees are valid.
			*/
			return FAILED;
		}
		else 
			return SUCCESS;
	}
	else if(strcmp(name,"longitude")==0)
	{
		if((obj->longitude = convert_to_longitude(value))==QNAN)
		{
			output_error("object %s:d longitude '%s' is invalid", obj->oclass->name, obj->id, value);
			/*	TROUBLESHOOT
				Latitudes are expected to be in the format of X{E,W, }Y'Z".  The seconds may be excluded.
				If the seconds are excluded, minutes may be excluded.  If minutes are excluded, the
				north/south indicator may be excluded.  There must be only one space between the degree
				value and the minute value.  Negative degrees are valid.
			*/
			return FAILED;
		}
		else 
			return SUCCESS;
	}
	else if(strcmp(name,"in_svc")==0)
	{
		tval = convert_to_timestamp_delta(value,&temp_microseconds,&tval_double);

		if(tval == TS_INVALID)
		{
			output_error("object %s:%d in_svc timestamp '%s' is invalid", obj->oclass->name, obj->id, value);
			return FAILED;
		}
		else if ((tval > obj->out_svc) || ((tval == obj->out_svc) && (temp_microseconds >= obj->out_svc_micro)))
		{
			output_error("object %s:%d in_svc timestamp '%s' overlaps out_svc timestamp", obj->oclass->name, obj->id, value);
			return FAILED;
		}
		else
		{
			obj->in_svc = tval;
			obj->in_svc_micro = temp_microseconds;
			obj->in_svc_double = tval_double;
			return SUCCESS;
		}
	}
	else if(strcmp(name,"out_svc")==0)
	{
		tval = convert_to_timestamp_delta(value,&temp_microseconds,&tval_double);

		if(tval == TS_INVALID)
		{
			output_error("object %s:%d out_svc timestamp '%s' is invalid", obj->oclass->name, obj->id, value);
			return FAILED;
		}
		else if ((tval < obj->in_svc) || ((tval == obj->in_svc) && (temp_microseconds <= obj->in_svc_micro)))
		{
			output_error("object %s:%d out_svc timestamp '%s' overlaps in_svc timestamp", obj->oclass->name, obj->id, value);
			return FAILED;
		}
		else
		{
			obj->out_svc = tval;
			obj->out_svc_micro = temp_microseconds;
			obj->out_svc_double = tval_double;
			return SUCCESS;
		}
	}
	else if(strcmp(name,"flags")==0)
	{
		/* flags should be ignored */
		return SUCCESS;
	}
	else if ( strcmp(name,"heartbeat")==0 )
	{
		TIMESTAMP t = convert_to_timestamp(value);
		if(t == TS_INVALID)
		{
			output_error("object %s:%d out_svc timestamp '%s' is invalid", obj->oclass->name, obj->id, value);
			return FAILED;
		}
		else
		{
			obj->heartbeat = t;
			return SUCCESS;
		}
	}
	else {
		output_error("object %s:%d called set_header_value() for invalid field '%s'", name);
		/*	TROUBLESHOOT
			The valid header fields are "name", "parent", "rank", "clock", "valid_to", "latitude",
			"longitude", "in_svc", "out_svc", "heartbeat", and "flags".
		*/
		return FAILED;
	}
	/* should never get here */
}

/** Set a property value by reference to its name
	@return the number of characters written to the buffer
 **/
int object_set_value_by_name(OBJECT *obj, /**< the object to change */
							 PROPERTYNAME name, /**< the name of the property to change */
							 char *value) /**< the value to set */
{
	void *addr;
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop==NULL)
	{
		if(set_header_value(obj,name,value)==FAILED)
		{
			errno = ENOENT;
			return 0;
		}
		else
		{
			size_t len = strlen(value);
			return len>0?(int)len:1; /* empty string is not necessarily wrong */
		}
	}
	if(prop->access != PA_PUBLIC){
		output_error("trying to set the value of non-public property %s in %s", prop->name, obj->oclass->name);
		/*	TROUBLESHOOT
			The specified property was published by its object as private.  It may not be modified by other modules.
		*/
		return 0;
	}
	addr = (void*)((char *)(obj+1)+(int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	return object_set_value_by_addr(obj,addr,value,prop);
}


/* Set a property value by reference to its name
 */
int object_set_int16_by_name(OBJECT *obj, PROPERTYNAME name, int16 value)
{
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop==NULL)
	{
		errno = ENOENT;
		return 0;
	}
	if(prop->access != PA_PUBLIC){
		output_error("trying to set the value of non-public property %s in %s", prop->name, obj->oclass->name);
		/*	TROUBLESHOOT
			The specified property was published by its object as private.  It may not be modified by other modules.
		*/
		return 0;
	}
	if(prop->ptype != PT_int16){
		output_error("property '%s' of '%s' is cannot be set like an int16", prop->name, obj->oclass->name);
		/*	TROUBLESHOOT
			The specified property was not an int16.
		*/
		return 0;
	}
	*(int16 *)((char *)(obj+1)+(int64)(prop->addr)) = value; /* warning: cast from pointer to integer of different size */
	return 1;
}

/* Set a property value by reference to its name
 */
int object_set_int32_by_name(OBJECT *obj, PROPERTYNAME name, int32 value)
{
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop==NULL)
	{
		errno = ENOENT;
		return 0;
	}
	if(prop->access != PA_PUBLIC){
		output_error("trying to set the value of non-public property %s in %s", prop->name, obj->oclass->name);
		/*	TROUBLESHOOT
			The specified property was published by its object as private.  It may not be modified by other modules.
		*/
		return 0;
	}
	*(int32 *)((char *)(obj+1)+(int64)(prop->addr)) = value; /* warning: cast from pointer to integer of different size */
	return 1;
}

/* Set a property value by reference to its name
 */
int object_set_int64_by_name(OBJECT *obj, PROPERTYNAME name, int64 value)
{
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop==NULL)
	{
		errno = ENOENT;
		return 0;
	}
	if(prop->access != PA_PUBLIC){
		output_error("trying to set the value of non-public property %s in %s", prop->name, obj->oclass->name);
		/*	TROUBLESHOOT
			The specified property was published by its object as private.  It may not be modified by other modules.
		*/
		return 0;
	}
	*(int64 *)((char *)(obj+1)+(int64)(prop->addr)) = value; /* warning: cast from pointer to integer of different size */
	return 1;
}

/* Set a property value by reference to its name
 */
int object_set_double_by_name(OBJECT *obj, PROPERTYNAME name, double value)
{
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop==NULL)
	{
		errno = ENOENT;
		return 0;
	}
	if(prop->access != PA_PUBLIC){
		output_error("trying to set the value of non-public property %s in %s", prop->name, obj->oclass->name);
		/*	TROUBLESHOOT
			The specified property was published by its object as private.  It may not be modified by other modules.
		*/
		return 0;
	}
	*(double*)((char *)(obj+1)+(int64)(prop->addr)) = value; /* warning: cast from pointer to integer of different size */
	return 1;
}

/* Set a property value by reference to its name
 */
int object_set_complex_by_name(OBJECT *obj, PROPERTYNAME name, complex value)
{
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop==NULL)
	{
		errno = ENOENT;
		return 0;
	}
	if(prop->access != PA_PUBLIC){
		output_error("trying to set the value of non-public property %s in %s", prop->name, obj->oclass->name);
		/*	TROUBLESHOOT
			The specified property was published by its object as private.  It may not be modified by other modules.
		*/
		return 0;
	}
	*(complex*)((char *)(obj+1)+(int64)(prop->addr)) = value; /* warning: cast from pointer to integer of different size */
	return 1;
}

/** Get a property value by reference to its physical address
	@return the number of characters written to the buffer; 0 if failed
 **/
int object_get_value_by_addr(OBJECT *obj, /**< the object from which to get the data */
							 void *addr, /**< the addr of the data to get */
							 char *value, /**< the buffer to which to write the result */
							 int size, /**< the size of the buffer */
							 PROPERTY *prop) /**< the property to use or NULL if unknown */
{
	prop = prop ? prop : get_property_at_addr(obj,addr);
	if(prop->access == PA_PRIVATE){
		output_error("trying to read the value of private property %s in %s", prop->name, obj->oclass->name);
		/*	TROUBLESHOOT
			The specified property was published by its object as private.  It may not be modified by other modules.
		*/
		return 0;
	}
	return class_property_to_string(prop,addr,value,size);
}

/** Get a value by reference to its property name
	@return the number of characters written to the buffer; 0 if failed
 **/
int object_get_value_by_name(OBJECT *obj, PROPERTYNAME name, char *value, int size)
{
	char temp[1024];
	char *buffer;
	if(value == 0){
		output_error("object_get_value_by_name: 'value' is a null pointer");
		return 0;
	}
	if(size < 1){
		output_error("object_get_value_by_name: invalid buffer size of %i", size);
		return 0;
	}
	buffer = object_property_to_string(obj,name, temp, 1023);
	if(buffer==NULL)
		return 0;
	
	strncpy(value,buffer,size);
	return 1;
}

/** Get a reference to another object 
 **/
OBJECT *object_get_reference(OBJECT *obj, char *name){
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop == NULL || prop->access == PA_PRIVATE || prop->ptype != PT_object)
	{
		if(prop == NULL){
			;
		}
		errno = EINVAL;
		return NULL;
	} else {
		return *(OBJECT**)((char*)obj + sizeof(OBJECT) + (int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	}
}

/** Get the first object in the model
	@return a pointer to the first OBJECT
 **/
OBJECT *object_get_first()
{
	return first_object;
}

/** Get the next object in the model
	@return a pointer to the OBJECT after \p obj
 **/
OBJECT *object_get_next(OBJECT *obj){ /**< the object from which to start */
	if(obj != NULL){
		return obj->next;
	} else {
		return NULL;
	}
}


/*	Set the rank of the object (internal use only) 
	This function keeps track of which object initiated
	the request to prevent looping.  This will prevent
	an object_set_parent call from creating a parent loop.
 */
static int set_rank(OBJECT *obj, OBJECTRANK rank, OBJECT *first)
{
	OBJECTRANK parent_rank = -1;
	if(obj == NULL){
		output_error("set_rank called for a null object");
		return -1;
	}
	if(rank >= object_get_count()){
		char b[74];
		output_error("%s: set_rank wigging out, rank > object count", object_name(first, b, 64));
		/*	TROUBLESHOOT
			As a sanity check, the rank of an object should not exceed the number of objects in the model.  If the model
			is deliberately playing with the ranks, please either reduce the manual rank adjustment, or add a number of
			"harmless" objects to inflate the number of objects in the model.
		 */
		return -1;
	}
	if(obj==first)
	{
		char b[64];
		output_error("%s: set_rank failed, parent loopback has occurred", object_name(first, b, 63));
		return -1;
	}
	if(obj->flags & OF_RERANK){
		char b[64];
		output_error("%s: object flaged as already re-ranked", object_name(obj, b, 63));
		return -1;
	} else {
		obj->flags |= OF_RERANK;
	}
	if(rank >= obj->rank)
		obj->rank = rank+1;
	if(obj->parent != NULL)
	{
		parent_rank = set_rank(obj->parent,obj->rank,first?first:obj);
		if(parent_rank == -1)
			return -1;
	}
	obj->flags &= ~OF_RERANK;
	return obj->rank;
}

/** Set the rank of an object but forcing it's parent
	to increase rank if necessary.
	@return object rank; -1 if failed 
 **/
int object_set_rank(OBJECT *obj, /**< the object to set */
					OBJECTRANK rank) /**< the object */
{
	/* prevent rank from decreasing */
	if(obj == NULL)
		return 0;
	if(rank<=obj->rank)
		return obj->rank;
	return set_rank(obj,rank,NULL);
}

/** Set the parent of an object
	@obj	the object that is setting a new parent
	@parent	the new parent for obj.  May be null, removing the object's parent.
	@return the rank of the object after parent was set, must be equal to or greater than original rank and greater than parent's rank.
 **/
int object_set_parent(OBJECT *obj, /**< the object to set */
					  OBJECT *parent) /**< the new parent of the object */
{
	if(obj == NULL){
		output_error("object_set_parent was called with a null pointer");
		return -1;
	}
	if(obj == parent){
		char b[64];
		output_error("object %s tried to set itself as its parent", object_name(obj, b, 63));
		return -1;
	}
	obj->parent = parent;
	if(parent!=NULL)
		return set_rank(parent,obj->rank,NULL);
	return obj->rank;
}

/** Set the dependent of an object.  This increases
	to the rank as though the parent was set, but does
	not affect the parent.
	@return the rank of the object after the dependency was set
 **/
int object_set_dependent(OBJECT *obj, /**< the object to set */
						 OBJECT *dependent) /**< the dependent object */
{
	if(obj == NULL){
		output_error("object_set_dependent was called with a null pointer");
		return -1;
	}
	if(dependent == NULL){
		char b[64];
		output_error("object %s tried to set a null object as a dependent", object_name(obj, b, 63));
		return -1;
	}
	if(obj == dependent)
		return -1;
	
	return set_rank(dependent,obj->rank,NULL);
}

/* Convert the value of an object property to a string
 */
char *object_property_to_string(OBJECT *obj, char *name, char *buffer, int sz)
{
	//static char buffer[4096];
	void *addr;
	PROPERTY *prop = class_find_property(obj->oclass,name);
	if(prop==NULL)
	{
		errno = ENOENT;
		return NULL;
	}
	addr = GETADDR(obj,prop); /* warning: cast from pointer to integer of different size */
	if(prop->ptype==PT_delegated)
		return prop->delegation->to_string(addr,buffer,sz)?buffer:NULL;
	else if(class_property_to_string(prop,addr,buffer,sz))
	{
		
		return buffer;
	}
	else
		return "";
}

void object_profile(OBJECT *obj, OBJECTPROFILEITEM pass, clock_t t)
{
	if ( global_profiler==1 )
	{
		clock_t dt = exec_clock()-t;
		obj->synctime[pass] += dt;
		wlock(&obj->oclass->profiler.lock);
		obj->oclass->profiler.count++;
		obj->oclass->profiler.clocks += dt;
		wunlock(&obj->oclass->profiler.lock);
	}
}

TIMESTAMP _object_sync(OBJECT *obj, /**< the object to synchronize */
					  TIMESTAMP ts, /**< the desire clock to sync to */
					  PASSCONFIG pass) /**< the pass configuration */
{
	CLASS *oclass = obj->oclass;
	register TIMESTAMP plc_time=TS_NEVER, sync_time;
	TIMESTAMP effective_valid_to = min(obj->clock+global_skipsafe,obj->valid_to);
	int autolock = obj->oclass->passconfig&PC_AUTOLOCK;

	/* check skipsafe */
	if(global_skipsafe>0 && (obj->flags&OF_SKIPSAFE) && ts<effective_valid_to)

		/* return valid_to time if skipping */
		return effective_valid_to;

	/* check sync */
	if(oclass->sync==NULL)
	{
		char buffer[64];
		char buffer2[64];
		char *passname = (pass==PC_PRETOPDOWN?"PC_PRETOPDOWN":(pass==PC_BOTTOMUP?"PC_BOTTOMUP":(pass==PC_POSTTOPDOWN?"PC_POSTTOPDOWN":"<unknown>")));
		output_fatal("object_sync(OBJECT *obj='%s', TIMESTAMP ts='%s', PASSCONFIG pass=%s): int64 sync_%s(OBJECT*,TIMESTAMP,PASSCONFIG) is not implemented in module %s", object_name(obj, buffer2, 63), convert_from_timestamp(ts,buffer,sizeof(buffer))?buffer:"<invalid>", passname, oclass->name, oclass->module->name);
		/*	TROUBLESHOOT
			The indicated sync function is not implemented by the class given.  
			This happens when the PASSCONFIG flag indicates a particular sync
			(presync/sync/postsync) needs to be called, but the class does not
			actually implement it.  This is a problem with the module that
			implements the class.
		 */
		return TS_INVALID;
	}

#ifndef WIN32
	/* setup lockup alarm */
	alarm(global_maximum_synctime);
#endif

	/* call recalc if recalc bit is set */
	if( (obj->flags&OF_RECALC) && obj->oclass->recalc!=NULL)
	{
		if (autolock) wlock(&obj->lock);
		oclass->recalc(obj);
		if (autolock) wunlock(&obj->lock);
		obj->flags &= ~OF_RECALC;
	}

	/* call PLC code on bottom-up, if any */
	if( !(obj->flags&OF_HASPLC) && oclass->plc!=NULL && pass==PC_BOTTOMUP )
	{
		if (autolock) wlock(&obj->lock);
		plc_time = oclass->plc(obj,ts);
		if (autolock) wunlock(&obj->lock);
	}

	/* call sync */
	if (autolock) wlock(&obj->lock);
	sync_time = (*obj->oclass->sync)(obj,ts,pass);
	if (autolock) wunlock(&obj->lock);
	if(absolute_timestamp(plc_time)<absolute_timestamp(sync_time))
		sync_time = plc_time;

	/* compute valid_to time */
	if(sync_time>TS_MAX)
		obj->valid_to = TS_NEVER;
	else
		obj->valid_to = sync_time; // NOTE, this can be negative

#ifndef WIN32
	/* clear lockup alarm */
	alarm(0);
#endif

	return obj->valid_to;
}
/** Synchronize an object.  The timestamp given is the desired increment.

	If an object is called on multiple passes (see PASSCONFIG) it is 
	customary to update the clock only after the last pass is completed.

	For the sake of speed this function assumes that the sync function 
	is properly defined in the object class structure.

	@return  the time of the next event for this object.
 */
TIMESTAMP object_sync(OBJECT *obj, /**< the object to synchronize */
					  TIMESTAMP ts, /**< the desire clock to sync to */
					  PASSCONFIG pass) /**< the pass configuration */
{
	clock_t t=exec_clock();
	TIMESTAMP t2=TS_NEVER;
	//TIMESTAMP t_start = ts;
	//TIMESTAMP abs_t2 = ts;
	//char namestr[65];
	//int itr = global_iteration_limit;
	//if(obj->clock == 0 || obj->valid_to >= ts){
	//	// object has not started, or object's previous t2 is equal to or greater the clock + minimum timestep
	//	return _object_sync(obj, ts, pass);
	//} else {
	//	// object started AND it had a state change before the next minimum timestep
	//	t_start = obj->clock; // where the object was
	//	if(obj->valid_to > 0){
	//		abs_t2 = obj->valid_to; // where the object wanted to advance to
	//	} else {
	//		abs_t2 = -obj->valid_to; // soft timestep
	//	}
	//	
	//	do {
	//		/* don't call sync beyond valid horizon */
	//		t2 = _object_sync(obj,(abs_t2<(obj->valid_to>0?obj->valid_to:TS_NEVER)?abs_t2:obj->valid_to),pass);
	//		if(t2 < 0){
	//			abs_t2 = -t2;
	//		} else {
	//			abs_t2 = t2;
	//		}
	//		if(abs_t2 == t_start){
	//			--itr;
	//			if(itr == 0){
	//				output_fatal("mini-iteration limit reached for object %s in object_sync", object_name(obj, namestr, 64));
	//				return TS_INVALID;
	//			}
	//		}
	//		if(abs_t2 < t_start){
	//			output_error("return time is less than current time!");
	//			return TS_INVALID;
	//		} else {
	//			t_start = abs_t2;
	//			itr = global_iteration_limit;
	//		}
	//		
	//	} while (t2>0 && ts>(t2<0?-t2:t2) && t2<TS_NEVER);
	//	return t2;
	//}
	do {
		/* don't call sync beyond valid horizon */
		t2 = _object_sync(obj,(ts<(obj->valid_to>0?obj->valid_to:TS_NEVER)?ts:obj->valid_to),pass);	
	} while (t2>0 && ts>(t2<0?-t2:t2) && t2<TS_NEVER);

	/* do profiling, if needed */
	if ( global_profiler==1 )
	{
		switch (pass) {
		case PC_PRETOPDOWN: object_profile(obj,OPI_PRESYNC,t);break;
		case PC_BOTTOMUP: object_profile(obj,OPI_SYNC,t);break;
		case PC_POSTTOPDOWN: object_profile(obj,OPI_POSTSYNC,t);break;
		default: break;
		}
	}

	return t2;
}

TIMESTAMP object_heartbeat(OBJECT *obj)
{
	clock_t t=exec_clock();
	TIMESTAMP t1 = obj->oclass->heartbeat ? obj->oclass->heartbeat(obj) : TS_NEVER;
	object_profile(obj,OPI_HEARTBEAT,t);
	return t1;
}

/** Initialize an object.  This should not be called until
	all objects that are needed are created

	@return 1 on success; 0 on failure
 **/
int object_init(OBJECT *obj) /**< the object to initialize */
{
	clock_t t=exec_clock();
	int rv = 1;
	obj->clock = global_starttime;
	if(obj->oclass->init != NULL)
		rv = (int)(*(obj->oclass->init))(obj, obj->parent);
	object_profile(obj,OPI_INIT,t);
	return rv;
}

/** Run events that should only occur at the start of a timestep.
	The input timestamp is that of the new time that is being syncronized to.

	This function should not affect other objects, and should not rely on
	calculations that are performed by other objects in precommit, since there
	is no order

	The return value is if the function successfully completed.
 **/
STATUS object_precommit(OBJECT *obj, TIMESTAMP t1)
{
	clock_t t=exec_clock();
	STATUS rv = SUCCESS;
	if(obj->oclass->precommit != NULL){
		rv = (STATUS)(*(obj->oclass->precommit))(obj, t1);
	}
	if(rv == 1){ // if 'old school' or no precommit callback,
		rv = SUCCESS;
	}
	object_profile(obj,OPI_PRECOMMIT,t);
	return rv;
}

TIMESTAMP object_commit(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	clock_t t=exec_clock();
	TIMESTAMP rv = 1;
	if(obj->oclass->commit != NULL){
		rv = (TIMESTAMP)(*(obj->oclass->commit))(obj, t1, t2);
	}
	if(rv == 1){ // if 'old school' or no commit callback,
		rv =TS_NEVER;
	} 
	object_profile(obj,OPI_COMMIT,t);
	return rv;
}

/**	Finalize is the last function callback that is made for an object.  It
	provides an opportunity to close files, collate answers, come to
	conclusions, and destroy network objects.

	The return value is if the function successfully completed.
 **/
STATUS object_finalize(OBJECT *obj)
{
	clock_t t=exec_clock();
	STATUS rv = SUCCESS;
	if(obj->oclass->finalize != NULL){
		rv = (STATUS)(*(obj->oclass->finalize))(obj);
	}
	if(rv == 1){ // if 'old school' or no finalize callback,
		rv = SUCCESS;
	}
	object_profile(obj,OPI_FINALIZE,t);
	return rv;
}

/** Tests the type of an object
 **/
int object_isa(OBJECT *obj, /**< the object to test */
			   char *type){ /**< the type of test */
	if(obj == 0){
		return 0;
	}
	if(strcmp(obj->oclass->name,type) == 0){
		return 1;
	} else if(obj->oclass->isa){
		return (int)obj->oclass->isa(obj, type);
	} else {
		return 0;
	}
}

/** Dump an object to a buffer
	@return the number of characters written to the buffer
 **/
int object_dump(char *outbuffer, /**< the destination buffer */
				int size, /**< the size of the buffer */
				OBJECT *obj){ /**< the object to dump */
	char buffer[65536];
	char tmp[256];
	char tmp2[1024];
	int count = 0;
	PROPERTY *prop = NULL;
	CLASS *pclass = NULL;
	if(size>sizeof(buffer)){
		size = sizeof(buffer);
	}
	
	count += sprintf(buffer + count, "object %s:%d {\n", obj->oclass->name, obj->id);

	/* dump internal properties */
	if(obj->parent != NULL){
		count += sprintf(buffer + count, "\tparent = %s:%d (%s)\n", obj->parent->oclass->name, obj->parent->id, obj->parent->name != NULL ? obj->parent->name : "");
	} else {
		count += sprintf(buffer + count, "\troot object\n");
	}
	if(obj->name != NULL){
		count += sprintf(buffer + count, "\tname %s\n", obj->name);
	}

	count += sprintf(buffer + count, "\trank = %d;\n", obj->rank);
	count += sprintf(buffer + count, "\tclock = %s (%" FMT_INT64 "d);\n", convert_from_timestamp(obj->clock, tmp, sizeof(tmp)) > 0 ? tmp : "(invalid)", obj->clock);

	if(!isnan(obj->latitude)){
		count += sprintf(buffer + count, "\tlatitude = %s;\n", convert_from_latitude(obj->latitude, tmp, sizeof(tmp)) ? tmp : "(invalid)");
	}
	if(!isnan(obj->longitude)){
		count += sprintf(buffer + count, "\tlongitude = %s;\n", convert_from_longitude(obj->longitude, tmp, sizeof(tmp)) ? tmp : "(invalid)");
	}
	count += sprintf(buffer + count, "\tflags = %s;\n", convert_from_set(tmp, sizeof(tmp), &(obj->flags), object_flag_property()) ? tmp : "(invalid)");

	/* dump properties */
	for(prop = obj->oclass->pmap; prop != NULL && prop->oclass == obj->oclass; prop = prop->next){
		char *value = object_property_to_string(obj, prop->name, tmp2, 1023);
		if(value != NULL){
			count += sprintf(buffer + count, "\t%s %s = %s;\n", prop->ptype == PT_delegated ? prop->delegation->type : class_get_property_typename(prop->ptype), prop->name, value);
			if(count > size){
				throw_exception("object_dump(char *buffer=%x, int size=%d, OBJECT *obj=%s:%d) buffer overrun", outbuffer, size, obj->oclass->name, obj->id);
				/* TROUBLESHOOT
					The buffer used to dump objects has overflowed.  This can only be fixed by increasing the size of the buffer and recompiling.
					If you do not have access to source code, please report this problem so that it can be corrected in the next build.
				 */
			}
		}
	}

	/* dump inherited properties */
	pclass = obj->oclass;
	while((pclass = pclass->parent) != NULL){
		for(prop = pclass->pmap; prop != NULL && prop->oclass == pclass; prop = prop->next){
			char *value = object_property_to_string(obj, prop->name, tmp2, 1023);
			if(value != NULL){
				count += sprintf(buffer + count, "\t%s %s = %s;\n", prop->ptype == PT_delegated ? prop->delegation->type : class_get_property_typename(prop->ptype), prop->name, value);
				if(count > size){
					throw_exception("object_dump(char *buffer=%x, int size=%d, OBJECT *obj=%s:%d) buffer overrun", outbuffer, size, obj->oclass->name, obj->id);
					/* TROUBLESHOOT
						The buffer used to dump objects has overflowed.  This can only be fixed by increasing the size of the buffer and recompiling.
						If you do not have access to source code, please report this problem so that it can be corrected in the next build.
					 */
				}
			}
		}
	}

	count += sprintf(buffer+count,"}\n");
	if(count < size && count < sizeof(buffer)){
		strncpy(outbuffer, buffer, count+1);
		return count;
	} else {
		output_error("buffer too small in object_dump()!");
		return 0;
	}
	
}

/** Save an object to the buffer provided
    @return the number of bytes written to the buffer, 0 on error, with errno set
 **/
static int object_save_x(char *temp, int size, OBJECT *obj, CLASS *oclass)
{
	char buffer[1024];
	PROPERTY *prop;
	int count = sprintf(temp, "\t// %s properties\n", oclass->name);
	for ( prop=oclass->pmap; prop!=NULL && prop->oclass==oclass; prop=prop->next )
	{
		char *value = object_property_to_string(obj, prop->name, buffer, 1023);
		if ( value!=NULL )
		{
			if ( prop->ptype==PT_timestamp)  // timestamps require single quotes
				count += sprintf(temp+count, "\t%s '%s';\n", prop->name, value);
			else if ( strcmp(value,"")==0 || ( strpbrk(value," \t") && prop->unit==NULL ) ) // double quotes needed empty strings and when white spaces are present in non-real values
				count += sprintf(temp+count, "\t%s \"%s\";\n", prop->name, value);
			else
				count += sprintf(temp+count, "\t%s %s;\n", prop->name, value);
		}
	}
	return count;
}
int object_save(char *buffer, int size, OBJECT *obj)
{
	char temp[65536];
	char32 oname="";
	CLASS *pclass;
	int count = sprintf(temp,"object %s:%d {\n\n\t// header properties\n", obj->oclass->name, obj->id);

	output_debug("saving object %s:%d", obj->oclass->name, obj->id);

	/* dump header properties */
	if(obj->parent != NULL){
		convert_from_object(oname, sizeof(oname), &obj->parent, NULL);
		count += sprintf(temp+count, "\tparent %s;\n", oname);
	}

	count += sprintf(temp+count, "\trank %d;\n", obj->rank);
	if(obj->name != NULL){
		count += sprintf(temp+count, "\tname %s;\n", obj->name);
	}
	count += sprintf(temp+count,"\tclock %s;\n", convert_from_timestamp(obj->clock, buffer, sizeof(buffer)) > 0 ? buffer : "(invalid)");
	if( !isnan(obj->latitude) ){
		count += sprintf(temp+count, "\tlatitude %s;\n", convert_from_latitude(obj->latitude, buffer, sizeof(buffer)) ? buffer : "(invalid)");
	}
	if( !isnan(obj->longitude) ){
		count += sprintf(temp+count, "\tlongitude %s;\n", convert_from_longitude(obj->longitude, buffer, sizeof(buffer)) ? buffer : "(invalid)");
	}
	count += sprintf(temp+count, "\tflags %s;\n", convert_from_set(buffer, sizeof(buffer), &(obj->flags), object_flag_property()) ? buffer : "(invalid)");

	/* dump class-defined properties */
	for ( pclass=obj->oclass->parent ; pclass!=NULL ; pclass=pclass->parent )
		count += object_save_x(temp+count,size-count,obj,pclass);
	count += object_save_x(temp+count,size-count,obj,obj->oclass);
	count += sprintf(temp+count,"}\n");
	if ( count>=sizeof(temp) )
		output_warning("object_save(char *buffer=%p, int size=%d, OBJECT *obj={%s:%d}: buffer overflow", buffer, size, obj->oclass->name, obj->id);
	if ( count<size )
	{
		strcpy(buffer,temp);
		return count;
	}
	else
	{
		errno = ENOMEM;
		return 0;
	}
}

/** Save all the objects in the model to the stream \p fp in the \p .GLM format
	@return the number of bytes written, 0 on error, with errno set.
 **/
int object_saveall(FILE *fp) /**< the stream to write to */
{
	unsigned count = 0;
	char buffer[1024];

	count += fprintf(fp, "\n########################################################\n");
	count += fprintf(fp, "# objects\n");
	{
		OBJECT *obj;
		for (obj = first_object; obj != NULL; obj = obj->next){
			PROPERTY *prop = NULL;
			char32 oname = "(unidentified)";
			count += fprintf(fp, "object %s:%d {\n", obj->oclass->name, obj->id);

			/* dump internal properties */
			if(obj->parent != NULL){
				convert_from_object(oname, sizeof(oname), &obj->parent, NULL);
				count += fprintf(fp, "\tparent %s;\n", oname);
			} else {
				count += fprintf(fp,"\troot;\n");
			}
			count += fprintf(fp, "\trank %d;\n", obj->rank);
			if(obj->name != NULL){
				count += fprintf(fp, "\tname %s;\n", obj->name);
			}
			count += fprintf(fp,"\tclock %s;\n", convert_from_timestamp(obj->clock, buffer, sizeof(buffer)) > 0 ? buffer : "(invalid)");
			if(!isnan(obj->latitude)){
				count += fprintf(fp, "\tlatitude %s;\n", convert_from_latitude(obj->latitude, buffer, sizeof(buffer)) ? buffer : "(invalid)");
			}
			if(!isnan(obj->longitude)){
				count += fprintf(fp, "\tlongitude %s;\n", convert_from_longitude(obj->longitude, buffer, sizeof(buffer)) ? buffer : "(invalid)");
			}
			count += fprintf(fp, "\tflags %s;\n", convert_from_set(buffer, sizeof(buffer), &(obj->flags), object_flag_property()) ? buffer : "(invalid)");

			/* dump properties */
			for(prop = obj->oclass->pmap; prop != NULL && prop->oclass == obj->oclass; prop = prop->next){
				char *value = object_property_to_string(obj, prop->name, buffer, 1023);
				if(value != NULL){
					count += fprintf(fp, "\t%s %s;\n", prop->name, value);
				}
			}
			count += fprintf(fp,"}\n");
		}
	}
	return count;	
}

/** Save all the objects in the model to the stream \p fp in the \p .XML format
	@return the number of bytes written, 0 on error, with errno set.
 **/
int object_saveall_xml(FILE *fp){ /**< the stream to write to */
	unsigned count = 0;
	char buffer[1024];
	PROPERTY *prop = NULL;
	OBJECT *obj = NULL;
	CLASS *oclass = NULL;

	for(obj = first_object; obj != NULL; obj = obj->next){
		char32 oname = "(unidentified)";
		convert_from_object(oname, sizeof(oname), &obj, NULL); /* what if we already have a name? -mh */
		if((oclass == NULL) || (obj->oclass != oclass)){
			oclass = obj->oclass;
		}
		count += fprintf(fp, "\t\t<object type=\"%s\" id=\"%i\" name=\"%s\">\n", obj->oclass->name, obj->id, oname);

		/* dump internal properties */
		if(obj->parent != NULL){
			convert_from_object(oname, sizeof(oname), &obj->parent, NULL);
			count += fprintf(fp,"\t\t\t<parent>\n"); 
			count += fprintf(fp, "\t\t\t\t%s\n", oname);
			count += fprintf(fp,"\t\t\t</parent>\n");
		} else {
			count += fprintf(fp,"\t\t\t<parent>root</parent>\n");
		}
		count += fprintf(fp,"\t\t\t<rank>%d</rank>\n", obj->rank);
		count += fprintf(fp,"\t\t\t<clock>\n", obj->clock);
		count += fprintf(fp,"\t\t\t\t <timestamp>%s</timestamp>\n", convert_from_timestamp(obj->clock,buffer, sizeof(buffer)) > 0 ? buffer : "(invalid)");
		count += fprintf(fp,"\t\t\t</clock>\n");
		/* why do latitude/longitude have 2 values?  I currently only store as float in the schema... */
		if(!isnan(obj->latitude)){
			count += fprintf(fp, "\t\t\t<latitude>%lf %s</latitude>\n", obj->latitude, convert_from_latitude(obj->latitude, buffer, sizeof(buffer)) ? buffer : "(invalid)");
		}
		if(!isnan(obj->longitude)){
			count += fprintf(fp, "\t\t\t<longitude>%lf %s</longitude>\n", obj->longitude, convert_from_longitude(obj->longitude, buffer, sizeof(buffer)) ? buffer : "(invalid)");
		}

		/* dump inherited properties */
		if(oclass->parent != NULL){
			for (prop = oclass->parent->pmap; prop != NULL && prop->oclass == oclass->parent; prop = prop->next){
				char *value = object_property_to_string(obj, prop->name, buffer, 1023);
				if(value != NULL){
					count += fprintf(fp, "\t\t\t<%s>%s</%s>\n", prop->name, value, prop->name);
				}
			}
		}

		/* dump properties */
		for(prop = oclass->pmap; prop != NULL && prop->oclass == oclass; prop = prop->next){
			char *value = object_property_to_string(obj, prop->name, buffer, 1023);
			if(value!=NULL){
				count += fprintf(fp, "\t\t\t<%s>%s</%s>\n", prop->name, value, prop->name);
			}
		}
		count += fprintf(fp, "\t\t</object>\n");
	}

	count += fprintf(fp, "\t</objects>\n");
	return count;	
}

int object_saveall_xml_old(FILE *fp);

int object_saveall_xml_old(FILE *fp){ /**< the stream to write to */
	unsigned count = 0;
	char buffer[1024];

	count += fprintf(fp,"\t<objects>\n");
	{
		OBJECT *obj;
		CLASS *oclass = NULL;

		for (obj = first_object; obj != NULL; obj = obj->next){
			PROPERTY *prop = NULL;
			char32 oname = "(unidentified)";

			convert_from_object(oname, sizeof(oname), &obj, NULL);
			
			if(oclass == NULL || obj->oclass != oclass){
				oclass = obj->oclass;
			}
			count += fprintf(fp, "\t\t<object>\n");
			count += fprintf(fp, "\t\t\t<name>%s</name> \n", oname);
			count += fprintf(fp, "\t\t\t<class>%s</class> \n", obj->oclass->name);
			count += fprintf(fp, "\t\t\t<id>%d</id>\n", obj->id);

			/* dump internal properties */
			if(obj->parent != NULL){
				convert_from_object(oname, sizeof(oname), &obj->parent, NULL);
				count += fprintf(fp, "\t\t\t<parent>\n"); 
				count += fprintf(fp, "\t\t\t\t<name>%s</name>\n", oname);
				count += fprintf(fp, "\t\t\t\t<class>%s</class>\n", obj->parent->oclass->name);
				count += fprintf(fp, "\t\t\t\t<id>%d</id>\n", obj->parent->id);
				count += fprintf(fp, "\t\t\t</parent>\n");
			} else {
				count += fprintf(fp,"\t\t\t<parent>root</parent>\n");
			}
			count += fprintf(fp, "\t\t\t<rank>%d</rank>\n", obj->rank);
			count += fprintf(fp, "\t\t\t<clock>\n", obj->clock);
			count += fprintf(fp, "\t\t\t\t <timestamp>%s</timestamp>\n", (convert_from_timestamp(obj->clock, buffer, sizeof(buffer)) > 0) ? buffer : "(invalid)");
			count += fprintf(fp, "\t\t\t</clock>\n");
				/* why do latitude/longitude have 2 values?  I currently only store as float in the schema... */
			if(!isnan(obj->latitude)){
				count += fprintf(fp, "\t\t\t<latitude>%lf %s</latitude>\n" ,obj->latitude, convert_from_latitude(obj->latitude, buffer, sizeof(buffer)) ? buffer : "(invalid)");
			}
			if(!isnan(obj->longitude)) {
				count += fprintf(fp, "\t\t\t<longitude>%lf %s</longitude>\n", obj->longitude, convert_from_longitude(obj->longitude, buffer, sizeof(buffer)) ? buffer : "(invalid)");
			}

			/* dump properties */
			count += fprintf(fp, "\t\t\t<properties>\n");
			for (prop = oclass->pmap; prop != NULL && prop->oclass == oclass; prop = prop->next){
				char *value = object_property_to_string(obj, prop->name, buffer, 1023);

				if(value != NULL){
					count += fprintf(fp, "\t\t\t\t<property>\n");
					count += fprintf(fp, "\t\t\t\t\t<type>%s</type> \n", prop->name);
					count += fprintf(fp, "\t\t\t\t\t<value>%s</value> \n", value);
					count += fprintf(fp, "\t\t\t\t</property>\n");
				}
			}
			count += fprintf(fp, "\t\t\t</properties>\n");
			count += fprintf(fp, "\t\t</object>\n");
		}
	}
	count += fprintf(fp,"\t</objects>\n");
	return count;	
}

int convert_from_latitude(double v, void *buffer, int bufsize){
	double d = floor(fabs(v));
	double r = fabs(v) - d;
	double m = floor(r * 60.0);
	double s = (r - (double)m / 60.0) * 3600.0;
	char ns = (v < 0) ? 'S' : 'N';

	if(isnan(v)){
		return 0;
	}
	
	return sprintf(buffer, "%.0f%c%.0f:%.2fs", d, ns, m, s);
}

int convert_from_longitude(double v, void *buffer, int bufsize){
	double d = floor(fabs(v));
	double r = fabs(v)-d;
	double m = floor(r*60);
	double s = (r - (double)m/60.0)*3600;
	char ns = (v < 0) ? 'W' : 'E';

	if(isnan(v)){
		return 0;
	}

	return sprintf(buffer, "%.0f%c%.0f'%.2f\"", d, ns, m, s);
}

double convert_to_latitude(char *buffer){
	int32 d, m = 0;
	double s = 0;
	char ns;
	
	if(sscanf(buffer, "%d%c%d'%lf\"", &d, &ns, &m, &s) > 1){	
		double v = (double)d + (double)m / 60.0 + s / 3600.0;
		if(v >= 0.0 || v <= 90.0){
			if(ns == 'S'){
				return -v;
			} else if(ns == 'N'){
				return v;
			}
		}
	}

	return QNAN;
}

double convert_to_longitude(char *buffer){
	int32 d, m = 0;
	double s = 0;
	char ns;

	if(sscanf(buffer, "%d%c%d'%lf\"", &d, &ns, &m, &s) > 1){	
		double v = (double)d + (double)m / 60.0 + s / 3600.0;
		if(v >= 0.0 || v <= 90.0){
			if(ns == 'W'){
				return -v;
			} else if(ns=='E'){
				return v;
			}
		}
	}

	return QNAN;
}

/***************************************************************************
 OBJECT NAME TREE
 ***************************************************************************/

typedef struct s_objecttree {
	char name[64];
	OBJECT *obj;
	struct s_objecttree *before, *after;
	int balance; /* unused */
} OBJECTTREE;

static OBJECTTREE *top=NULL;

void debug_traverse_tree(OBJECTTREE *tree){
	if(tree == NULL){
		tree = top;
		if(top == NULL){
			return;
		}
	}
	if(tree->before != NULL){
		debug_traverse_tree(tree->before);
	}
	output_test("%s", tree->name);
	if(tree->after != NULL){
		debug_traverse_tree(tree->after);
	}
}

/* returns the height of the tree */
int tree_get_height(OBJECTTREE *tree){
	if(tree == NULL){
		return 0;
	} else {
		int left = tree_get_height(tree->before);
		int right = tree_get_height(tree->after);
		if(left > right)
			return left+1;
		else return right+1;
	}
}

/* returns the node to point to instead of tree */
void rotate_tree_right(OBJECTTREE **tree){ /* move one object from left to right */
	OBJECTTREE *root, *pivot, *child;
	root = *tree;
	pivot = root->before;
	child = root->before->after;
	*tree = pivot;
	pivot->after = root;
	root->before = child;
	root->balance += 2;
	pivot->balance += 1;
}

/* returns the node to point to instead of tree */
void rotate_tree_left(OBJECTTREE **tree){ /* move one object from right to left */
	OBJECTTREE *root, *pivot, *child;
	root = *tree;
	pivot = root->after;
	child = root->after->before;
	*tree = pivot;
	pivot->before = root;
	root->after = child;
	root->balance -= 2;
	pivot->balance -= 1;
}

/*  Rebalance the tree to make searching more efficient
	It's a good idea to this after the tree is built
 */
int object_tree_rebalance(OBJECTTREE *tree) /* AVL logic */
{
	/* currently being done during insertions & deletions */
	return 0;
}

/*	Add an item to the tree
	returns the "correct" root node for the subtree that an object was added to.
 */
static int addto_tree(OBJECTTREE **tree, OBJECTTREE *item){
	int rel = strcmp((*tree)->name, item->name);
	int right = 0, left = 0, ir = 0, il = 0, rv = 0, height = 0;

	// find location to insert new object
	if(rel > 0){
		if((*tree)->before == NULL){
			(*tree)->before = item;
		} else {
			rv = addto_tree(&((*tree)->before), item);
			if(global_no_balance){
				return rv + 1;
			}
		}
	} else if(rel<0) {
		if((*tree)->after == NULL) {
			(*tree)->after = item;
		} else {
			rv = addto_tree(&((*tree)->after),item);
			if(global_no_balance){
				return rv + 1;
			}
		}
	} else {
		return (*tree)->obj==item->obj;
	}

	// check balance
	right = tree_get_height((*tree)->after);
	left = tree_get_height((*tree)->before);
	(*tree)->balance = right - left;

	// rotations needed?
	if((*tree)->balance > 1){
		if((*tree)->after->balance < 0){ /* inner left is heavy */
			rotate_tree_right(&((*tree)->after));
		}
		rotate_tree_left(tree);	//	was left/right
	} else if((*tree)->balance < -1){
		if((*tree)->before->balance > 0){ /* inner right is heavy */
			rotate_tree_left(&((*tree)->before));
		}
		rotate_tree_right(tree);
	}
	return tree_get_height(*tree); /* verify after rotations */
}

/*	Add an object to the object tree.  Throws exceptions on memory errors.
	Returns a pointer to the object tree item if successful, NULL on failure (usually because name already used)
 */
static OBJECTTREE *object_tree_add(OBJECT *obj, OBJECTNAME name){
	OBJECTTREE *item = (OBJECTTREE*)malloc(sizeof(OBJECTTREE));

	if(item == NULL) {
		output_fatal("object_tree_add(obj='%s:%d', name='%s'): memory allocation failed (%s)", obj->oclass->name, obj->id, name, strerror(errno));
		return NULL;
		/* TROUBLESHOOT
			The memory required to add this object to the object index is not available.  Try freeing up system memory and try again.
		 */
	}
	
	item->obj = obj;
	item->balance = 0;
	strncpy(item->name, name, sizeof(item->name));
	item->before = item->after = NULL;

	if(top == NULL){
		top = item;
		return top;
	} else {
		if(addto_tree(&top, item) != 0){
			return item;
		} else {
			return NULL;
		}
	}
}

/*	Finds a name in the tree
 */
static OBJECTTREE **findin_tree(OBJECTTREE *tree, OBJECTNAME name)
{
	OBJECTTREE **temptree = NULL;

	if(tree == NULL){
		return NULL;
	} else {
		int rel = strcmp(tree->name, name);
		if(rel > 0){
			if(tree->before != NULL){
				if(strcmp(tree->before->name, name) == 0){
					return &(tree->before);
				} else {
					return findin_tree(tree->before, name);
				}
			} else {
				return NULL;
			}
		} else if(rel<0) {
			if(tree->after != NULL){
				if(strcmp(tree->after->name, name) == 0){
					return &(tree->after);
				} else {
					return findin_tree(tree->after, name);
				}
			} else {
				return NULL;
			}
		} else {
			return (temptree = &tree);
		}
	}
}

/*	Deletes a name from the tree
	WARNING: removing a tree entry does NOT free() its object!
 */
void object_tree_delete(OBJECT *obj, OBJECTNAME name)
{
	OBJECTTREE **item = findin_tree(top,name);
	OBJECTTREE *temp = NULL, **dtemp = NULL;

	if(item != NULL && strcmp((*item)->name, name)!=0){
		if((*item)->after == NULL && (*item)->before == NULL){ /* no children -- nuke */
			free(*item);
			*item = NULL;
		} else if((*item)->after != NULL && (*item)->before != NULL){ /* two children -- find a replacement */
			dtemp = &((*item)->before);
			while(temp->after != NULL)
				dtemp = &(temp->after);
			temp = (*dtemp)->before;
			(*dtemp)->before = (*item)->before;
			(*dtemp)->after = (*item)->after;
			free(*item);
			*item = *dtemp;
			*dtemp = temp;
			/* replace item with the rightmost left element.*/

		} else if((*item)->after == NULL || (*item)->before == NULL){ /* one child -- promotion time! */
			if((*item)->after != NULL){
				temp = (*item)->after;
				free(*item);
				*item = temp;
			} else if((*item)->before != NULL){
				temp = (*item)->before;
				free(*item);
				*item = temp;
			} else {
				output_fatal("unexpected branch result in object_tree_delete");
				/*	TROUBLESHOOT
					This should never happen and if it does, the system has become unstable and the problem should be reported.
				 */
			}
		}

		/* throw_exception("object_tree_delete(obj=%s:%d, name='%s'): rename of objects in tree is not supported yet", obj->oclass->name, obj->id, name); */
	}
}

/** Find an object from a name.  This only works for named objects.  See object_set_name().
	@return a pointer to the OBJECT structure
 **/
OBJECT *object_find_name(OBJECTNAME name){
	OBJECTTREE **item = NULL;

	item = findin_tree(top, name);
	
	if(item != NULL && *item != NULL){
		return (*item)->obj;
	} else {
		/* normal operation, remain silent */
		return NULL;
	}
}

int object_build_name(OBJECT *obj, char *buffer, int len){
	char b[256];
	char *ptr = 0;
	int L; // to not confuse l and 1 visually

	if(obj == 0){
		return 0;
	}
	if(buffer == 0){
		return 0;
	}

	if(obj->name){
		L = (int)strlen(obj->name);
		ptr = obj->name;
	} else {
		sprintf(b, "%s %i", obj->oclass->name, obj->id);
		L = (int)strlen(b);
		ptr = b;
	}

	if(L > len){
		output_error("object_build_name(): unable to build name for '%s', input buffer too short", ptr);
		return 0;
	} else {
		strcpy(buffer, ptr);
		return L;
	}

	output_error("object_build_name(): control unexpectedly reached end of method");
	return 0; // shouldn't reach this
}

/** Sets the name of an object.  This is useful if the internal name cannot be relied upon, 
	as when multiple modules are being used.
	Throws an exception when a memory error occurs or when the name is already taken by another object.
 **/
OBJECTNAME object_set_name(OBJECT *obj, OBJECTNAME name){
	OBJECTTREE *item = NULL;

	if((isalpha(name[0]) != 0) || (name[0] == '_')){
		; // good
	} else {
		if(global_relax_naming_rules == 0){
			output_error("object name '%s' invalid, names must start with a letter or an underscore", name);
			return NULL;
		} else {
			output_warning("object name '%s' does not follow strict naming rules and may not link correctly during load time", name);
		}
	}
	if(obj->name != NULL){
		object_tree_delete(obj,name);
	}
	
	if(name != NULL){
		if(object_find_name(name) != NULL){
			output_error("An object named '%s' already exists!", name);
			/*	TROUBLESHOOT
				GridLab-D prohibits two objects from using the same name, to prevent
				ambiguous object look-ups.
			*/
			return NULL;
		}
		item = object_tree_add(obj,name);
		if(item != NULL){
			obj->name = item->name;
		}
	}
	
	if(item != NULL){
		return item->name;
	} else {
		return NULL;
	}
}

/** Convenience method use by the testing framework.  
	This should only be exposed there.
 **/
void remove_objects(){ 
	OBJECT* obj1;

	obj1 = first_object;
	while(obj1 != NULL){
		first_object = obj1->next;
		obj1->oclass->profiler.numobjs--;
		free(obj1);
		obj1 = first_object;
	}

	next_object_id = 0;
}

/*****************************************************************************************************
 * name space support
 *****************************************************************************************************/
NAMESPACE *current_namespace = NULL;
static int _object_namespace(NAMESPACE *space,char *buffer,int size)
{
	int n=0;
	if(space==NULL)
		return 0;
	n += _object_namespace(space->next,buffer,size);
	if(buffer[0]!='\0') 
	{
		strcat(buffer,"::"); 
		n++;
	}
	strcat(buffer,space->name);
	n+=(int)strlen(space->name);
	return n;
}
/** Get the full namespace of current space
	@return the full namespace spec
 **/
void object_namespace(char *buffer, int size)
{
	strcpy(buffer,"");
	_object_namespace(current_namespace,buffer,size);
}

/** Get full namespace of object's space
	@return 1 if in subspace, 0 if global namespace
 **/
int object_get_namespace(OBJECT *obj, char *buffer, int size)
{
	strcpy(buffer,"");
	_object_namespace(obj->space,buffer,size);
	return obj->space!=NULL;
}

/** Get the current namespace
    @return pointer to namespace or NULL is global
 **/
NAMESPACE *object_current_namespace()
{
	return current_namespace;
}

/** Opens a new namespace within the current name space
	@return 1 on success, 0 on failure
 **/
int object_open_namespace(char *space)
{
	NAMESPACE *ns = malloc(sizeof(NAMESPACE));
	if(ns==NULL)
	{
		throw_exception("object_open_namespace(char *space='%s'): memory allocation failure", space);
		/* TROUBLESHOOT
			The memory required to create the indicated namespace is not available.  Try freeing up system memory and try again.
		 */
		return 0;
	}
	strncpy(ns->name,space,sizeof(ns->name));
	ns->next = current_namespace;
	current_namespace = ns;
	return 1;
}

/** Closes the current namespace
    @return 1 on success, 0 on failure
 **/
int object_close_namespace()
{
	if(current_namespace==NULL)
	{
		throw_exception("object_close_namespace(): no current namespace to close");
		/* TROUBLESHOOT
			An attempt to close a namespace was made while there was no open namespace.
			This is an internal error and should be reported.
		 */
		return 0;
	}
	current_namespace = current_namespace->next;
	return 1;
}

/** Makes the namespace active
    @return 1 on success, 0 on failure
 **/
int object_select_namespace(char *space)
{
	output_error("namespace selection not yet supported");
	return 0;
}

/** Locate the object and property corresponding the address of data
	@return 1 on success, 0 on failure
	Sets the pointers to the object and the property that matches
 **/
int object_locate_property(void *addr, OBJECT **pObj, PROPERTY **pProp)
{
	OBJECT *obj;
	for (obj=first_object; obj!=NULL; obj=obj->next)
	{
		if ((int64)addr>(int64)obj && (int64)addr<(int64)(obj+1)+(int64)obj->oclass->size)
		{
			int offset = (int)((int64)addr - (int64)(obj+1));
			PROPERTY *prop; 
			for (prop=obj->oclass->pmap; prop!=NULL && prop->oclass==obj->oclass; prop=prop->next)
			{
				if ((int64)prop->addr == offset)
				{
					*pObj = obj;
					*pProp = prop;
					return SUCCESS;
				}
			}
		}
	}
	return FAILED;
}

/** Forecast create 
    The specifications for a forecast are as follows
	"option: value; [option: value; [...]]" where
	options is as follows:
	'timestep' - identifies the timestep of the forecast
	'length' - identifies the number of values in the forecast
	'property' - identifies the property this forecast applies to
	'external' - identifies the external function call to use to update the forecast
	
	The external function is specified as 'libname/functionname', the function 'functionname'
	call must be in the DLL/SO/DYLIB file 'libfile' and have the following 
	call prototype
		TIMESTAMP functioname(OBJECT *obj, FORECAST *fc);
	where the return value is the new forecast start time or TZ_INVALID is the forecast 
	could not be updated (in which case the existing forecast 'fc' is not changed).

	if 'external' is not defined, then the forecast is expected to be updated
	during the object presync operation.  It is up to the class implementation of
	presync to suppress update of the forecast when 'external' is set.
		
 **/
FORECAST *forecast_create(OBJECT *obj, char *specs)
{
	//FORECAST *f;
	FORECAST *fc;

	/* crate forecast entity */
	fc = malloc(sizeof(FORECAST));
	if ( fc==NULL ) 
		throw_exception("forecast_create(): memory allocation failed");
		/* TROUBLESHOOT
		   The forecast_create function could not allocate memory for 
		   the FORECAST entity.  This is probably due to a lack of system
		   memory or a problem with the memory allocation system.  Free up system
		   memory, reducing the complexity and/or size of the model and try again.
		 */
	memset(fc,0,sizeof(FORECAST));

	/* add to current list of forecasts */
	fc->next = obj->forecast;
	obj->forecast = fc;

	/* extract forecast description */
	/* TODO */
	output_warning("forecast_create(): description parsing not implemented");

	/* copy the description */
	strncpy(fc->specification,specs,sizeof(fc->specification));

	return fc;
}

/** Forecast find
 **/
FORECAST *forecast_find(OBJECT *obj, char *name)
{
	FORECAST *fc;
	for ( fc=obj->forecast; fc!=NULL; fc=fc->next )
	{
		if (fc->propref && strcmp(fc->propref->name,name)==0)
			return fc;
	}
	return NULL;
}

/** Forecast read
 **/
double forecast_read(FORECAST *fc, TIMESTAMP ts)
{
	int64 n;

	/* prevent use of zero or negative timesteps */
	if ( fc->timestep<=0 )
		return QNAN;

	/* time request is before start of forecast */
	if ( ts < fc->starttime)
		return QNAN;

	/* compute offset to data entry */
	n = ( ts - fc->starttime ) / fc->timestep;

	/* time of request is after end of forecast */
	if ( n >= fc->n_values )
		return QNAN;

	if ( fc->values )
		return fc->values[n];
	else
		return QNAN;
}

/** Forecast save
 **/
void forecast_save(FORECAST *fc, TIMESTAMP ts, int32 tstep, int n_values, double *data)
{
	fc->starttime = ts;
	fc->timestep = tstep;
	if ( fc->n_values != n_values )
	{
		if ( fc->values ) free(fc->values);
		fc->values = malloc( n_values * sizeof(double) );
		if ( fc->values == NULL ) 
			throw_exception("forecast_save(): memory allocation failed");
			/* TROUBLESHOOT
			   The forecast_create function could not allocate memory for 
			   the FORECAST entity.  This is probably due to a lack of system
			   memory or a problem with the memory allocation system.  Free up system
			   memory, reducing the complexity and/or size of the model and try again.
			 */
		fc->n_values = n_values;
	}
	memcpy(fc->values,data,n_values*sizeof(double));
}

/** threadsafe remote object read **/
void *object_remote_read(void *local, /**< local memory for data (must be correct size for property) */
						 OBJECT *obj, /**< object from which to get data */
						 PROPERTY *prop) /**< property from which to get data */
{
	int size = property_size(prop);
	void *addr = ((char*)obj)+(size_t)(prop->addr);
	
	/* single host */
	if ( global_multirun_mode==MRM_STANDALONE)
	{
		/* single thread */
		if ( global_threadcount==1 )
		{
			/* no lock or fetch required */
			memcpy(local,addr,size);
			return local;
		}

		/* multithread */
		else 
		{
			rlock(&obj->lock);
			memcpy(local,addr,size);
			runlock(&obj->lock);
			return local;
		}
	}
	else
	{
		/* @todo remote object read for multihost */
		return NULL;
	}
}

/** threadsafe remote object write **/
void object_remote_write(void *local, /** local memory for data */
						 OBJECT *obj, /** object to which data is written */
						 PROPERTY *prop) /**< property to which data is written */
{
	int size = property_size(prop);
	void *addr = ((char*)obj)+(size_t)(prop->addr);
	
	/* single host */
	if ( global_multirun_mode==MRM_STANDALONE)
	{
		/* single thread */
		if ( global_threadcount==1 )
		{
			/* no lock or fetch required */
			memcpy(addr,local,size);
		}

		/* multithread */
		else 
		{
			wlock(&obj->lock);
			memcpy(addr,local,size);
			wunlock(&obj->lock);
		}
	}
	else
	{
		/* @todo remote object write for multihost */
	}
}

double object_get_part(void *x, char *name)
{
	OBJECT *obj = (OBJECT*)x;
	char root[64], part[64];

	if ( strcmp(name,"id")==0 ) return (double)(obj->id);
	if ( strcmp(name,"rng_state")==0 ) return (double)(obj->rng_state);
	if ( strcmp(name,"latitude")==0 ) return obj->latitude;
	if ( strcmp(name,"longitude")==0 ) return obj->longitude;
	if ( strcmp(name,"schedule_skew")==0 ) return (double)(obj->schedule_skew);

	if ( sscanf(name,"%[^. ].%s",root,part)==2 ) // has part
	{
		struct {
			char *name;
			TIMESTAMP *addr;
		} *p, map[]={
			{"clock",&(obj->clock)},
			{"valid_to",&(obj->valid_to)},
			{"in_svc",&(obj->in_svc)},
			{"out_svc",&(obj->out_svc)},
			{"heartbeat",&(obj->heartbeat)},
		};
		for ( p=map ; p<map+sizeof(map); p++ ) {
			if ( strcmp(p->name,root)==0 )
				return timestamp_get_part(p->addr,part);
		}
	}
	return QNAN;
}

/** @} **/
