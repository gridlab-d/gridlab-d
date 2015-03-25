/** $Id: gridlabd.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file gridlabd.h
	@author David P. Chassin
	@addtogroup module_api C/C++ Module API
	@brief The GridLAB-D external C module header file

	The runtime module API links the GridLAB-D core to modules that are created to
	perform various modeling tasks.  The core interacts with each module according
	to a set script that determines which exposed module functions are called and
	when.  The general sequence of calls is as follows:
	- <b>Registration</b>: A module registers the object classes it implements and
	registers the variables that each class publishes.
	- <b>Creation</b>: The core calls object creation functions during the model
	load operation for each object that is created.  Basic initialization can be
	completed at this point.
	- <b>Definition</b>: The core sets the values of all published variables that have
	been specified in the model being loaded.  After this is completed, all references
	to other objects have been resolved.
	- <b>Validation</b>: The core gives the module an opportunity to check the model
	before initialization begins.  This gives the module an opportunity to validate
	the model and reject it or fix it if it fails to meet module-defined criteria.
	- <b>Initialization</b>: The core calls the final initialization procedure with
	the object's full context now defined.  Properties that are derived based on the
	object's context should be initialized only at this point.
	- <b>Synchronization</b>: This operation is performed repeatedly until every object
	reports that it no longer expects any state changes.  The return value from a
	synchronization operation is the amount time expected to elapse before the object's
	next state change.  The side effect of a synchronization is a change to one or
	more properties of the object, and possible an update to the property of another
	object.

	Note that object destruction is not supported at this time.

	 GridLAB-D modules usually require a number of functions to access data and interaction
	 with the core.  These include
	 - memory locking,
	 - memory exception handlers,
	 - variable publishers,
	 - output functions,
	 - management routines,
	 - module management,
	 - class registration,
	 - object management,
	 - property management,
	 - object search,
	 - random number generators, and
	 - time management.
 @{
 **/

#ifndef _GRIDLABD_H
#define _GRIDLABD_H

/* version info */
#define MAJOR 3
#define MINOR 0

/* permanently disable use of CPPUNIT */
#ifndef _NO_CPPUNIT
#define _NO_CPPUNIT
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32
#define HAVE_LIBCPPUNIT
#endif

#ifdef __cplusplus
	#ifndef CDECL
		/** Defines a function as a C-type function **/
		#define CDECL extern "C"
	#endif
#else
	#define CDECL
#endif

#ifdef WIN32
#ifndef EXPORT
/** Defines a function as exported to core **/
#define EXPORT CDECL __declspec(dllexport)
#endif
#else
#define EXPORT CDECL
#endif

#include <stdarg.h>
#include "platform.h"
#include "schedule.h"
#include "transform.h"
#include "object.h"
#include "find.h"
#include "random.h"
#define STREAM_MODULE
#include "stream.h"

#ifdef DLMAIN
#define EXTERN
#define INIT(X) =(X)
#else
#ifdef __cplusplus
#define EXTERN
#else
#define EXTERN extern
#endif /* __cplusplus */
#define INIT(X)
#endif
CDECL EXPORT EXTERN CALLBACKS *callback INIT(NULL);
#undef INIT
#undef EXTERN

#ifndef MODULENAME
#define MODULENAME(obj) (obj->oclass->module->name)
#endif

/******************************************************************************
 * Variable publishing
 */
/**	@defgroup gridlabd_h_publishing Publishing variables

	Modules must register public variables that are accessed by other modules, and the core by publishing them.

	The typical modules will register a class, and immediately publish the variables supported by the class:
	@code

	EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
	{
		extern CLASS* node_class; // defined globally in the module
		if (set_callback(fntable)==NULL)
		{
			errno = EINVAL;
			return NULL;
		}

		node_class = gl_register_class(module,"node",sizeof(node),PC_BOTTOMUP);
		PUBLISH_CLASS(node,complex,V);
		PUBLISH_CLASS(node,complex,S);

		return node_class; // always return the *first* class registered
	}
	@endcode

	@{
 **/
/** The PUBLISH_STRUCT macro is used to publish a member of a structure.
 **/
#define PUBLISH_STRUCT(C,T,N) {struct C *_t=NULL;if (gl_publish_variable(C##_class,PT_##T,#N,(char*)&(_t->N)-(char*)_t,NULL)<1) return NULL;}
/** The PUBLISH_CLASS macro is used to publish a member of a class (C++ only).
 **/
#define PUBLISH_CLASS(C,T,N) {class C *_t=NULL;if (gl_publish_variable(C##_class,PT_##T,#N,(char*)&(_t->N)-(char*)_t,NULL)<1) return NULL;}
/** The PUBLISH_CLASSX macro is used to publish a member of a class (C++ only) using a different name from the member name.
 **/
#define PUBLISH_CLASSX(C,T,N,V) {class C *_t=NULL;if (gl_publish_variable(C##_class,PT_##T,V,(char*)&(_t->N)-(char*)_t,NULL)<1) return NULL;}

/** The PUBLISH_CLASS_UNT macros is used to publish a member of a class (C++ only) including a unit specification.
**/
//#define PUBLISH_CLASS_UNIT(C,T,N,U) {class C _t;if (gl_publish_variable(C##_class,PT_##T,#N"["U"]",(char*)&_t.N-(char*)&_t,NULL)<1) return NULL;}
/** The PUBLISH_DELEGATED macro is used to publish a variable that uses a delegated type.

**/
#define PUBLISH_DELEGATED(C,T,N) {class C *_t=NULL;if (gl_publish_variable(C##_class,PT_delegated,T,#N,(char*)&(_t->N)-(char*)_t,NULL)<1) return NULL;}

/** The PUBLISH_ENUM(C,N,E) macro is used to define a keyword for an enumeration variable
 **/
//#define PUBLISH_ENUM(C,N,E) (*callback->define_enumeration_member)(C##_class,#N,#E,C::E)

/** The PUBLISH_SET(C,N,E) macro is used to define a keyword for a set variable
 **/
//#define PUBLISH_SET(C,N,E) (*callback->define_set_member)(C##_class,#N,#E,C::E)
/** @} **/

#define PADDR(X) ((char*)&(this->X)-(char*)this)

/******************************************************************************
 * Exception handling
 */
/**	@defgroup gridlabd_h_exception Exception handling

	Module exception handling is provided for modules implemented in C to perform exception handling,
	as well to allow C++ code to throw exceptions to the core's main exception handler.

	Typical use is like this:

	@code
	#include <errno.h>
	#include <string.h>
	GL_TRY {

		// block of code

		// exception
		if (errno!=0)
			GL_THROW("Error condition %d: %s", errno, strerror(errno));

		// more code

	} GL_CATCH(char *msg) {

		// exception handler

	} GL_ENDCATCH;
	@endcode

	Note: it is ok to use GL_THROW inside a C++ catch statement.  This behavior is defined
	(unlike using C++ throw inside C++ catch) because GL_THROW is implemented using longjmp().

	See \ref exception "Exception handling" for detail on the message format conventions.

	@{
 **/
/** You may create your own #GL_TRY block and throw exception using GL_THROW(Msg,...) within
	the block.  Declaring this block will change the behavior of GL_THROW(Msg,...) only
	within the block.  Calls to GL_THROW(Msg,...) within this try block will be transfer
	control to the GL_CATCH(Msg) block.
 **/
#define GL_TRY { EXCEPTIONHANDLER *_handler = (*callback->exception.create_exception_handler)(); if (_handler==NULL) (*callback->output_error)("%s(%d): module exception handler creation failed",__FILE__,__LINE__); else if (setjmp(_handler->buf)==0) {
/* TROUBLESHOOT
	This error is caused when the system is unable to implement an exception handler for a module.
	This is an internal error and should be reported to the module developer.
 */
/** The behavior of GL_THROW(Msg,...) differs depending on the situation:
	- Inside a #GL_TRY block, program flow is transfered to the GL_CATCH(Msg) block that follows.
	- Inside a GL_CATCH(Msg) block, GL_THROW(Msg,...) behavior is undefined (read \em bad).
	- Outside a #GL_TRY or GL_CATCH(Msg) block, control is transfered to the main core exception handler.
 **/
#ifdef __cplusplus
inline void GL_THROW(char *format, ...)
{
	static char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(buffer,format,ptr);
	va_end(ptr);
	throw (const char*) buffer;
}
#else
#define GL_THROW (*callback->exception.throw_exception)
/** The argument \p msg provides access to the exception message thrown.
	Otherwise, GL_CATCH(Msg) blocks function like all other code blocks.

	The behavior of GL_THROW(Msg) is not defined inside GL_CATCH(Msg) blocks.

	GL_CATCH blocks must always be terminated by a #GL_ENDCATCH statement.
 **/
#endif
#define GL_CATCH(Msg) } else {Msg = (*callback->exception.exception_msg)();
/** GL_CATCH(Msg) blocks must always be terminated by a #GL_ENDCATCH statement.
 **/
#define GL_ENDCATCH } (*callback->exception.delete_exception_handler)(_handler);}
/** @} **/

/******************************************************************************
 * Output functions
 */
/**	@defgroup gridlabd_h_output Output functions

	Module may output messages to stdout, stderr, and the core test record file
	using the output functions.

	See \ref output "Output function" for details on message format conventions.
	@{
 **/

#define gl_verbose (*callback->output_verbose)

/** Produces a message on stdout.
	@see output_message(char *format, ...)
 **/
#define gl_output (*callback->output_message)

/** Produces a warning message on stderr, but only when \b --warning is provided on the command line.
	@see output_warning(char *format, ...)
 **/
#define gl_warning (*callback->output_warning)

/** Produces an error message on stderr, but only when \b --quiet is not provided on the command line.
 	@see output_error(char *format, ...)
**/
#define gl_error (*callback->output_error)

#ifdef _DEBUG
/** Produces a debug message on stderr, but only when \b --debug is provided on the command line.
 	@see output_debug(char *format, ...)
 **/
#define gl_debug (*callback->output_debug)
#else
#define gl_debug
#endif

/** Produces a test message in the test record file, but only when \b --testfile is provided on the command line.
	@see output_testmsg(char *format, ...)
 **/
#define gl_testmsg (*callback->output_test)
/** @} **/

/******************************************************************************
 * Memory allocation
 */
/**	@defgroup gridlabd_h_memory Memory allocation functions

	The core is responsible for managing any memory that is shared.  Use these
	macros to manage the allocation of objects that are registered classes.

	@{
 **/
/** Allocate a block of memory from the core's heap.
	This is necessary for any memory that the core will have to manage.
	@see malloc()
 **/
#define gl_malloc (*callback->malloc)
/** @} **/

/******************************************************************************
 * Core access
 */
/**	@defgroup gridlabd_h_corelib Core access functions

	Most module function use core library functions and variables.
	Use these macros to access the core library and other global module variables.

	@{
 **/
/** Defines the callback table for the module.
	Callback function provide module with direct access to important core functions.
	@see struct s_callback
 **/
#define set_callback(CT) (callback=(CT))

/** Provides access to a global module variable.
	@see global_getvar(), global_setvar()
 **/
#define gl_get_module_var (*callback->module.getvar)

/** Provide file search function
	@see find_file()
 **/
#define gl_findfile (*callback->file.find_file)

#define gl_find_module (*callback->module_find)

#define gl_find_property (*callback->find_property)

/** Declare a module dependency.  This will automatically load
    the module if it is not already loaded.
	@return 1 on success, 0 on failure
 **/
#ifdef __cplusplus
inline int gl_module_depends(char *name, /**< module name */
							 unsigned char major=0, /**< major version, if any required (module must match exactly) */
							 unsigned char minor=0, /**< minor version, if any required (module must be greater or equal) */
							 unsigned short build=0) /**< build number, if any required (module must be greater or equal) */
{
	return (*callback->module.depends)(name,major,minor,build);
}
#else
#define gl_module_getfirst (*callback->module.getfirst)
#define gl_module_depends (*callback->module.depends)
#endif


/** @} **/

/******************************************************************************
 * Class registration
 */
/**	@defgroup gridlabd_h_class Class registration

	Class registration is used to make sure the core knows how objects are implemented
	in modules.  Use the class management macros to create and destroy classes.

	@{
 **/
/** Allow an object class to be registered with the core.
	Note that C file may publish structures, even they are not implemented as classes.
	@see class_register()
 **/
#define gl_register_class (*callback->register_class)
/** @} **/

/******************************************************************************
 * Object management
 */
/**	@defgroup gridlabd_h_object Object management

	Object management macros are create to allow modules to create, test,
	control ranks, and reveal members of objects and registered classes.

	@{
 **/
/** Creates an object by allocating on from core heap.
	@see object_create_single()
 */
#define gl_create_object (*callback->create.single)

/** Creates an array of objects on core heap.
	@see object_create_array()
 **/
#define gl_create_array (*callback->create.array)

/** Creates an array of objects on core heap.
	@see object_create_array()
 **/
#define gl_create_foreign (*callback->create.foreign)

/** Object type test

	Checks the type (and supertypes) of an object.

	@see object_isa()
 **/
#ifdef __cplusplus
inline bool gl_object_isa(OBJECT *obj, /**< object to test */
						  char *type,
						  char *modname=NULL) /**< type to test */
{	bool rv = (*callback->object_isa)(obj,type)!=0;
	bool mv = modname ? obj->oclass->module == (*callback->module_find)(modname) : true;
	return (rv && mv);}
#else
#define gl_object_isa (*callback->object_isa)
#endif

/** Declare an object property as publicly accessible.
	@see object_define_map()
 **/
#define gl_publish_variable (*callback->define_map)

/** Publishes an object function.  This is currently unused.
	@see object_define_function()
 **/
#ifdef __cplusplus
inline FUNCTION *gl_publish_function(CLASS *oclass, /**< class to which function belongs */
									 FUNCTIONNAME functionname, /**< name of function */
									 FUNCTIONADDR call) /**< address of function entry */
{ return (*callback->function.define)(oclass, functionname, call);}
inline FUNCTIONADDR gl_get_function(OBJECT *obj, char *name)
{ return obj?(*callback->function.get)(obj->oclass->name,name):NULL;}
#else
#define gl_publish_function (*callback->function.define)
#define gl_get_function (*callback->function.get)
#endif

/** Changes the dependency rank of an object.
	Normally dependency rank is determined by the object parent,
	but an object's rank may be increased using this call.
	An object's rank may not be decreased.
	@see object_set_rank(), object_set_parent()
 **/
#ifdef __cplusplus
inline int gl_set_dependent(OBJECT *obj, /**< object to set dependency */
							OBJECT *dep) /**< object dependent on */
{ return (*callback->object.set_dependent)(obj,dep);}
#else
#define gl_set_dependent (*callback->object.set_dependent)
#endif

/** Establishes the rank of an object relative to another object (it's parent).
	When an object is parent to another object, it's rank is always greater.
	Object of higher rank are processed first on top-down passes,
	and later on bottom-up passes.
	Objects of the same rank may be processed in parallel,
	if system resources make it possible.
	@see object_set_rank(), object_set_parent()
 **/
#ifdef __cplusplus
inline int gl_set_parent(OBJECT *obj, /**< object to set parent of */
						 OBJECT *parent) /**< parent object */
{ return (*callback->object.set_parent)(obj,parent);}
#else
#define gl_set_parent (*callback->object.set_parent)
#endif

/** Adjusts the rank of an object relative to another object (it's parent).
	When an object is parent to another object, it's rank is always greater.
	Object of higher rank are processed first on top-down passes,
	and later on bottom-up passes.
	Objects of the same rank may be processed in parallel,
	if system resources make it possible.
	@see object_set_rank(), object_set_parent()
 **/
#ifdef __cplusplus
inline int gl_set_rank(OBJECT *obj, /**< object to change rank */
					   int rank)	/**< new rank of object */
{ return (*callback->object.set_rank)(obj,rank);}
#else
#define gl_set_rank (*callback->object.set_rank)
#endif

/** @} **/

/******************************************************************************
 * Property management
 */
/**	@defgroup gridlabd_h_property Property management

	Use the property management functions to provide and gain access to published
	variables from other modules.  This include getting property information
	and unit conversion.

	@{
 **/
/** Create an object
	@see class_register()
 **/
#define gl_register_type (*callback->register_type)

/** Publish an delegate property type for a class
	@note This is not supported in Version 1.
 **/
#define gl_publish_delegate (*callback->define_type)

/** Get a property of an object
	@see object_get_property()
 **/
#ifdef __cplusplus
inline PROPERTY *gl_get_property(OBJECT *obj, /**< a pointer to the object */
								 PROPERTYNAME name, /**< the name of the property */
								 PROPERTYSTRUCT *part=NULL) /**< part info */
{ return (*callback->properties.get_property)(obj,name,part); }
#else
#define gl_get_property (*callback->properties.get_property)
#endif

/** Get the value of a property in an object
	@see object_get_value_by_addr()
 **/
#ifdef __cplusplus
inline int gl_get_value(OBJECT *obj, /**< the object from which to get the data */
						void *addr, /**< the addr of the data to get */
						char *value, /**< the buffer to which to write the result */
						int size, /**< the size of the buffer */
						PROPERTY *prop=NULL) /**< the property to use or NULL if unknown */
{ return (*callback->properties.get_value_by_addr)(obj,addr,value,size,prop);}
#else
#define gl_get_value (*callback->properties.get_value_by_addr)
#endif
#define gl_set_value_by_type (*callback->properties.set_value_by_type)

/** Set the value of a property in an object
	@see object_set_value_by_addr()
 **/
#ifdef __cplusplus
inline int gl_set_value(OBJECT *obj, /**< the object to alter */
						void *addr, /**< the address of the property */
						char *value, /**< the value to set */
						PROPERTY *prop) /**< the property to use or NULL if unknown */
{ return (*callback->properties.set_value_by_addr)(obj,addr,value,prop);}
#else
#define gl_set_value (*callback->properties.set_value_by_addr)
#endif

char* gl_name(OBJECT *my, char *buffer, size_t size);
#ifdef __cplusplus
/* 'stolen' from rt/gridlabd.h, something dchassin squirreled in. -mhauer */
/// Set the typed value of a property
/// @return nothing
template <class T> inline int gl_set_value(OBJECT *obj, ///< the object whose property value is being obtained
											PROPERTY *prop, ///< the name of the property being obtained
											T &value) ///< a reference to the local value where the property's value is being copied
{
	//T *ptr = (T*)gl_get_addr(obj,propname);
	char buffer[256];
	T *ptr = (T *)((char *)(obj + 1) + (int64)(prop->addr)); /* warning: cast from pointer to integer of different size */
	// @todo it would be a good idea to check the property type here
	if (ptr==NULL)
		GL_THROW("property %s not found in object %s", prop->name, gl_name(obj, buffer, 255));
	if(obj->oclass->notify){
		if(obj->oclass->notify(obj,NM_PREUPDATE,prop) == 0){
			gl_error("preupdate notify failure on %s in %s", prop->name, obj->name ? obj->name : "an unnamed object");
		}
	}
	*ptr = value;
	if(obj->oclass->notify){
		if(obj->oclass->notify(obj,NM_POSTUPDATE,prop) == 0){
			gl_error("postupdate notify failure on %s in %s", prop->name, obj->name ? obj->name : "an unnamed object");
		}
	}
	return 1;
}
#endif

/** Get a reference to another object
	@see object_get_reference()
 **/
#define gl_get_reference (*callback->properties.get_reference)

/** Get the value of a property in an object
	@see object_get_value_by_name()
 **/
#ifdef __cplusplus
inline int gl_get_value_by_name(OBJECT *obj,
								PROPERTYNAME name,
								char *value,
								int size)
{ return (*callback->properties.get_value_by_name)(obj,name,value,size);}
#else
#define gl_get_value_by_name (*callback->properties.get_value_by_name)
#endif

#ifdef __cplusplus
inline char *gl_getvalue(OBJECT *obj,
						 PROPERTYNAME name, char *buffer, int sz)
{
	return gl_get_value_by_name(obj,name,buffer,sz)>=0 ? buffer : NULL;
}
#endif

/** Set the value of a property in an object
	@see object_set_value_by_name()
 **/
#define gl_set_value_by_name (*callback->properties.set_value_by_name)

/** Get unit of property
	@see object_get_unit()
 **/
#define gl_get_unit (*callback->properties.get_unit)

/** Convert the units of a property using unit name
	@see unit_convert()
 **/
#define gl_convert (*callback->unit_convert)

/** Convert the units of a property using unit data
	@see unit_convert_ex()
 **/
#define gl_convert_ex (*callback->unit_convert_ex)

#define gl_find_unit (*callback->unit_find)

#define gl_get_object (*callback->get_object)

#define gl_name_object (*callback->name_object)

#define gl_get_object_count (*callback->object_count)

#define gl_get_object_prop (*callback->objvar.object_var)

/** Retrieve the complex value associated with the property
	@see object_get_complex()
**/
#define gl_get_complex (*callback->objvar.complex_var)

#define gl_get_complex_by_name (*callback->objvarname.complex_var)

#define gl_get_enum (*callback->objvar.enum_var)

#define gl_get_enum_by_name (*callback->objvarname.enum_var)

#define gl_get_set (*callback->objvar.set_var)

#define gl_get_set_by_name (*callback->objvarname.set_var)

#define gl_get_int16 (*callback->objvar.int16_var)

#define gl_get_int16_by_name (*callback->objvarname.int16_var)

#define gl_get_int32_by_name (*callback->objvarname.int32_var)

#define gl_get_int32 (*callback->objvar.int32_var)

#define gl_get_int64_by_name (*callback->objvarname.int64_var)

#define gl_get_int64 (*callback->objvar.int64_var)

#define gl_get_double_by_name (*callback->objvarname.double_var)

#define gl_get_double (*callback->objvar.double_var)

#define gl_get_string_by_name (*callback->objvarname.string_var)

#define gl_get_string (*callback->objvar.string_var)

#define gl_get_addr (*callback->properties.get_addr)

/** @} **/

/******************************************************************************
 * Object search
 */
/**	@defgroup gridlabd_h_search Object search

	Searches and navigates object lists.

	@{
 **/
/** Find one or more object
	@see find_objects()
 **/
#define gl_find_objects (*callback->find.objects)

/** Scan a list of found objects
	@see find_first(), find_next()
 **/
#define gl_find_next (*callback->find.next)

/** Duplicate a list of found objects
	@see find_copylist()
 **/
#define gl_findlist_copy (*callback->find.copy)
#define gl_findlist_add (*callback->find.add)
#define gl_findlist_del (*callback->find.del)
#define gl_findlist_clear (*callback->find.clear)
/** Release memory used by a find list
	@see free()
 **/
#define gl_free (*callback->free)

/** Create an aggregate property from a find list
	@see aggregate_mkgroup()
 **/
#define gl_create_aggregate (*callback->aggregate.create)

/** Evaluate an aggregate property
	@see aggregate_value()
 **/
#define gl_run_aggregate (*callback->aggregate.refresh)
/** @} **/

/******************************************************************************
 * Random number generation
 */
/**	@defgroup gridlabd_h_random Random numbers

	The random number library provides a variety of random number generations
	for different distributions.

	@{
 **/

#define RNGSTATE (&(OBJECTHDR(this))->rng_state)

/** Determine the distribution type to be used from its name
	@see RANDOMTYPE, random_type()
 **/
#define gl_randomtype (*callback->random.type)

/** Obtain an arbitrary random value using RANDOMTYPE
	@see RANDOMTYPE, random_value()
 **/
#define gl_randomvalue (*callback->random.value)

/** Obtain an arbitrary random value using RANDOMTYPE
	@see RANDOMTYPE, pseudorandom_value()
 **/
#define gl_pseudorandomvalue (*callback->random.pseudo)

/** Generate a uniformly distributed random number
	@see random_uniform()
 **/
#define gl_random_uniform (*callback->random.uniform)

/** Generate a normal distributed random number
	@see random_normal()
 **/
#define gl_random_normal (*callback->random.normal)

/** Generate a log normal distributed random number
	@see random_lognormal()
 **/
#define gl_random_lognormal (*callback->random.lognormal)

/** Generate a Bernoulli distributed random number
	@see random_bernoulli()
 **/
#define gl_random_bernoulli (*callback->random.bernoulli)

/** Generate a Pareto distributed random number
	@see random_pareto()
 **/
#define gl_random_pareto (*callback->random.pareto)

/** Generate a random number drawn uniformly from a sample
	@see random_sampled()
 **/
#define gl_random_sampled (*callback->random.sampled)

/** Generate an examponentially distributed random number
	@see random_exponential()
 **/
#define gl_random_exponential (*callback->random.exponential)
#define gl_random_triangle (*callback->random.triangle)
#define gl_random_gamma (*callback->random.gamma)
#define gl_random_beta (*callback->random.beta)
#define gl_random_weibull (*callback->random.weibull)
#define gl_random_rayleigh (*callback->random.rayleigh)
/** @} **/

/******************************************************************************
 * Timestamp handling
 */
/** @defgroup gridlabd_h_timestamp Time handling functions
 @{
 **/

#define gl_globalclock (*(callback->global_clock))

/** Link to double precision deltamode clock (offset by global_clock) **/
#define gl_globaldeltaclock (*(callback->global_delta_curr_clock))

/** Link to stop time of the simulation **/
#define gl_globalstoptime (*(callback->global_stoptime))

/** Convert a string to a timestamp
	@see convert_to_timestamp()
 **/
#define gl_parsetime (*callback->time.convert_to_timestamp)

#define gl_delta_parsetime (*callback->time.convert_to_timestamp_delta)

#define gl_printtime (*callback->time.convert_from_timestamp)

/** Convert a timestamp to a date/time structure
	@see mkdatetime()
 **/
#define gl_mktime (*callback->time.mkdatetime)

/** Convert a date/time structure to a string
	@see strdatetime()
 **/
#define gl_strtime (*callback->time.strdatetime)

/** Convert a timestamp to days
	@see timestamp_to_days()
 **/
#define gl_todays (*callback->time.timestamp_to_days)

/** Convert a timestamp to hours
	@see timestamp_to_hours()
 **/
#define gl_tohours (*callback->time.timestamp_to_hours)

/** Convert a timestamp to minutes
	@see timestamp_to_minutes()
 **/
#define gl_tominutes (*callback->time.timestamp_to_minutes)

/** Convert a timestamp to seconds
	@see timestamp_to_seconds()
 **/
#define gl_toseconds (*callback->time.timestamp_to_seconds)

/** Convert a timestamp to a local date/time structure
	@see local_datetime()
 **/
#define gl_localtime (*callback->time.local_datetime)

#ifdef __cplusplus
inline int gl_getweekday(TIMESTAMP t)
{
	DATETIME dt;
	gl_localtime(t, &dt);
	return dt.weekday;
}
inline int gl_gethour(TIMESTAMP t)
{
	DATETIME dt;
	gl_localtime(t, &dt);
	return dt.hour;
}
#endif
/**@}*/
/******************************************************************************
 * Global variables
 */
/** @defgroup gridlabd_h_globals Global variables
 @{
 **/

/** Create a new global variable
	@see global_create()
 **/
#define gl_global_create (*callback->global.create)

/** Set a global variable
	@see global_setvar()
 **/
#define gl_global_setvar (*callback->global.setvar)

/** Get a global variable
	@see global_getvar()
 **/
#define gl_global_getvar (*callback->global.getvar)

/** Find a global variable
	@see global_find()
 **/
#define gl_global_find (*callback->global.find)

#define gl_get_oflags (*callback->get_oflags)
/**@}*/

#ifdef __cplusplus
/******************************************************************************
 * Utilities
 */
/** @defgroup gridlabd_h_utility Utility functions
 @{
 **/

/** Clip a value \p x if outside the range (\p a, \p b)
	@return the clipped value of x
	@note \f clip() is only supported in C++
 **/
inline double clip(double x, /**< the value to clip **/
				   double a, /**< the lower limit of the range **/
				   double b) /**< the upper limit of the range **/
{
	if (x<a) return a;
	else if (x>b) return b;
	else return x;
}

/** Determine which bit is set in a bit pattern
	@return the bit number \e n; \p -7f is no bit found; \e -n if more than one bit found
 **/
inline char bitof(unsigned int64 x,/**< bit pattern to scan */
						   bool use_throw=false) /**< flag to use throw when more than one bit is set */
{
	char n=0;
	if (x==0)
	{
		if (use_throw)
			throw "bitof empty bit pattern";
		return -0x7f;
	}
	while ((x&1)==0)
	{
		x>>=1;
		n++;
	}
	if (x!=0)
	{
		if (use_throw)
			throw "bitof found more than one bit";
		else
			return -n;
	}
	return n;
}
/** Construct a proper object object
    @return a pointer to the struct buffer or NULL if failed
 **/
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

/** Find a schedule 
 **/
inline SCHEDULE *gl_schedule_find(char *name)
{
	return callback->schedule.find(name);
}
/** Create a schedule
 **/
inline SCHEDULE *gl_schedule_create(char *name, char *definition)
{
	return callback->schedule.create(name,definition);
}
/** Find the time index in a schedule
 **/
inline SCHEDULEINDEX gl_schedule_index(SCHEDULE *sch, TIMESTAMP ts)
{
	return callback->schedule.index(sch,ts);
}
/** Find the value at a time index in a schedule
 **/
inline double gl_schedule_value(SCHEDULE *sch, SCHEDULEINDEX index)
{
	return callback->schedule.value(sch,index);
}
/** Find the elapsed time until the value at an index changes
 **/
inline int32 gl_schedule_dtnext(SCHEDULE *sch, SCHEDULEINDEX index)
{
	return callback->schedule.dtnext(sch,index);
}
/** Create an enduse
 **/
inline int gl_enduse_create(enduse *e)
{
	return callback->enduse.create(e);
}
/** Synchronize an enduse
 **/
inline TIMESTAMP gl_enduse_sync(enduse *e, TIMESTAMP t1)
{
	return callback->enduse.sync(e,PC_BOTTOMUP,t1);
}
/** Create a loadshape 
 **/
inline loadshape *gl_loadshape_create(SCHEDULE *s)
{
	loadshape *ls = (loadshape*)malloc(sizeof(loadshape));
	memset(ls,0,sizeof(loadshape));
	if (0 == callback->loadshape.create(ls)){
		return NULL;
	}
	ls->schedule = s;
	return ls;
}
/** Get the current value of a loadshape
 **/
inline double gl_get_loadshape_value(loadshape *shape)
{
	if (shape)
		return shape->load;
	else
		return 0;
}
/** Format a DATETIME into a string buffer
 **/
inline char *gl_strftime(DATETIME *dt, char *buffer, int size) { return callback->time.strdatetime(dt,buffer,size)?buffer:NULL;};
/** Format a TIMESTAMP into a string buffer
 **/
inline char *gl_strftime(TIMESTAMP ts, char *buffer, int size)
{
	//static char buffer[64];
	DATETIME dt;
	if(buffer == 0){
		gl_error("gl_strftime: buffer is a null pointer");
		return 0;
	}
	if(size < 15){
		gl_error("gl_strftime: buffer size is too small");
		return 0;
	}
	if(gl_localtime(ts,&dt)){
		return gl_strftime(&dt,buffer,size);
	} else {
		strncpy(buffer,"(invalid time)", size);
	}
	return buffer;
}
/** Calculate the next power of 2 greater than a number
 **/
inline size_t nextpow2(register size_t x)
{
	if (x<0) return 0;
	x--;
	x|=x>>1;
	x|=x>>2;
	x|=x>>4;
	x|=x>>8;
	x|=x>>16;
	// won't work for anything over 2^30
	return x+1;
}

/**@}*/
#endif //__cplusplus


/******************************************************************************
 * Interpolation routines
 */
/** @defgroup gridlabd_h_interpolation Interpolation routines
 @{
 **/

/** Linearly interpolate a value between two points

 **/
#define gl_lerp (*callback->interpolate.linear)

/** Quadratically interpolate a value between two points

 **/
#define gl_qerp (*callback->interpolate.quadratic)
/**@}*/

/******************************************************************************
 * Forecasting routines
 */
/** @defgroup gridlabd_h_forecasting Forecasting routines
 @{
 **/

/** Create a forecast entity for an object
 **/
#define gl_forecast_create (*callback->forecast.create)

/** Find a forecast entity for an object
 **/
#define gl_forecast_find (*callback->forecast.find)

/** Read a forecast entity for an object
 **/
#define gl_forecast_read (*callback->forecast.read)

/** Save a forecast entity for an object
 **/
#define gl_forecast_save (*callback->forecast.save)
/**@}*/


/******************************************************************************
 * Init/Sync/Create catchall macros
 */
/** @defgroup gridlabd_h_catchall Init/Sync/Create catchall macros
 @{
 **/
///
/// Catchall for sync
///
#define SYNC_CATCHALL(C) catch (char *msg) { gl_error("sync_" #C "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg); return TS_INVALID; } catch (const char *msg) { gl_error("sync_" #C "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg); return TS_INVALID; } catch (...) { gl_error("sync_" #C "(obj=%d;%s): unhandled exception", obj->id, obj->name?obj->name:"unnamed"); return TS_INVALID; }
///
/// Catchall for init
///
#define INIT_CATCHALL(C) catch (char *msg) { gl_error("init_" #C "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg); return 0; } catch (const char *msg) { gl_error("init_" #C "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg); return 0; } catch (...) { gl_error("init_" #C "(obj=%d;%s): unhandled exception", obj->id, obj->name?obj->name:"unnamed"); return 0; }
///
/// Catchall for create
///
#define CREATE_CATCHALL(C) catch (char *msg) { gl_error("create_" #C ": %s", msg); return 0; } catch (const char *msg) { gl_error("create_" #C ": %s", msg); return 0; } catch (...) { gl_error("create_" #C ": unhandled exception"); return 0; }
#define I_CATCHALL(T,C) catch (char *msg) { gl_error(#T "_" #C ": %s", msg); return 0; } catch (const char *msg) { gl_error(#T "_" #C ": %s", msg); return 0; } catch (...) { gl_error(#T "_" #C ": unhandled exception"); return 0; }
#define T_CATCHALL(T,C) catch (char *msg) { gl_error(#T "_" #C "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg); return TS_INVALID; } catch (const char *msg) { gl_error(#T "_" #C "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg); return TS_INVALID; } catch (...) { gl_error(#T "_" #C "(obj=%d;%s): unhandled exception", obj->id, obj->name?obj->name:"unnamed"); return TS_INVALID; }
/**@}*/

/******************************************************************************
 * Remote data access
 */
/** @defgroup gridlabd_h_remote Remote data access
 @{
 **/

#ifdef __cplusplus
/** read remote object data **/
inline void *gl_read(void *local, /**< local memory for data (must be correct size for property) */
					 OBJECT *obj, /**< object from which to get data */
					 PROPERTY *prop) /**< property from which to get data */
{
	return callback->remote.readobj(local,obj,prop);
}
/** write remote object data **/
inline void gl_write(void *local, /** local memory for data */
					 OBJECT *obj, /** object to which data is written */
					 PROPERTY *prop) /**< property to which data is written */
{
	/* @todo */
	return callback->remote.writeobj(local,obj,prop);
}
/** read remote global data **/
inline void *gl_read(void *local, /** local memory for data (must be correct size for global */
					 GLOBALVAR *var) /** global variable from which to get data */
{
	/* @todo */
	return callback->remote.readvar(local,var);
}
/** write remote global data **/
inline void gl_write(void *local, /** local memory for data */
					 GLOBALVAR *var) /** global variable to which data is written */
{
	/* @todo */
	return callback->remote.writevar(local,var);
}
#endif
/**@}*/

// locking functions 
#ifdef __cplusplus
#define READLOCK(X) ::rlock(X); /**< Locks an item for reading (allows other reads but blocks write) */
#define WRITELOCK(X) ::wlock(X); /**< Locks an item for writing (blocks all operations) */
#define READUNLOCK(X) ::runlock(X); /**< Unlocks an read lock */
#define WRITEUNLOCK(X) ::wunlock(X); /**< Unlocks a write lock */

inline void rlock(unsigned int* lock) { callback->lock.read(lock); }
inline void wlock(unsigned int* lock) { callback->lock.write(lock); }
inline void runlock(unsigned int* lock) { callback->unlock.read(lock); }
inline void wunlock(unsigned int* lock) { callback->unlock.write(lock); }

#else
#define READLOCK(X) rlock(X); /**< Locks an item for reading (allows other reads but blocks write) */
#define WRITELOCK(X) wlock(X); /**< Locks an item for writing (blocks all operations) */
#define READUNLOCK(X) runlock(X); /**< Unlocks an read lock */
#define WRITEUNLOCK(X) wunlock(X); /**< Unlocks a write lock */
#endif
#define LOCK(X) WRITELOCK(X); /**< @todo this is deprecated and should not be used anymore */
#define UNLOCK(X) WRITEUNLOCK(X); /**< @todo this is deprecated and should not be used anymore */

#define READLOCK_OBJECT(X) READLOCK(&((X)->lock)) /**< Locks an object for reading */
#define WRITELOCK_OBJECT(X) WRITELOCK(&((X)->lock)) /**< Locks an object for writing */
#define READUNLOCK_OBJECT(X) READUNLOCK(&((X)->lock)) /**< Unlocks an object */
#define WRITEUNLOCK_OBJECT(X) WRITEUNLOCK(&((X)->lock)) /**< Unlocks an object */
#define LOCK_OBJECT(X) WRITELOCK_OBJECT(X); /**< @todo this is deprecated and should not be used anymore */
#define UNLOCK_OBJECT(X) WRITEUNLOCK_OBJECT(X); /**< @todo this is deprecated and should not be used anymore */

#define LOCKED(X,C) {WRITELOCK_OBJECT(X);(C);WRITEUNLOCK_OBJECT(X);} /**< @todo this is deprecated and should not be used anymore */

static unsigned long _nan[] = { 0xffffffff, 0x7fffffff, };
#ifdef WIN32
#define NaN (*(double*)&_nan)
#else// UNIX/LINUX
#include <math.h>
#define NaN NAN
#endif


#ifdef __cplusplus

/**************************************************************************************
 * GRIDLABD BASE CLASSES (Version 3.0 and later)
 * @defgroup gridlabd_h_classes Module API Classes
 * @{
 **************************************************************************************/

#include <ctype.h>
#include "module.h"
#include "class.h"
#include "property.h"

/// General string encapsulation
class gld_string {
private: // data
	typedef struct strbuf {
		unsigned int lock; // TODO implement locking
		size_t len;
		unsigned int nrefs;
		char *str;
	} STRBUF;
	STRBUF *buf;
public: // construction/destructor
	/// construct an empty string
	inline gld_string(void) : buf(NULL) { init(); };
	/// construct a linked string
	inline gld_string(gld_string&s) : buf(NULL) { init(); link(s); };
	/// construct a new string
	inline gld_string(const char *s) : buf(NULL) { init(); copy(s); };
	/// construct a new string of a particular length
	inline gld_string(const char *s, size_t n) : buf(NULL) { init(); copy(s,n); };
	/// destroy a string (or unlink from one)
	inline ~gld_string(void) { unlink(); };
public: // copy
	/// copy a string
	inline gld_string &operator=(const char *s) { copy(s); return *this; };
	/// link to a string
	inline gld_string &operator=(gld_string&s) { link(s); return *this; };
public: // casts
	/// cast to a pointer to the string buffer
	inline operator const char*(void) { return buf->str; };
	/// cast to the size of the string
	inline operator size_t(void) { return buf->len; };
	/// cast to a pointer to the string data
	inline operator STRBUF *(void) { return buf; };
private: // internals
	inline void init(void) { buf=(STRBUF*)malloc(sizeof(STRBUF)); memset(buf,0,sizeof(STRBUF)); }; 
	inline void lock(void) { if ( buf ) ::wlock(&buf->lock); };
	inline void unlock(void) { if ( buf ) ::wunlock(&buf->lock); };
	inline void fit(size_t n) { if ( buf==NULL || n>buf->len) alloc(n); };
	inline void alloc(size_t n) 
	{
		size_t len = nextpow2(n);
		if ( len<sizeof(NATIVE) ) 
			len=sizeof(NATIVE); 
		char *newstr=(char*)malloc(len);
		if ( buf->str!=NULL )
		{
			strcpy(newstr,buf->str);
			free(buf->str);
		}
		else
			buf->nrefs=1;
		buf->str = newstr;
		buf->len = len;
	};
	inline void copy(const char *s) { fit(strlen(s)+1); strcpy(buf->str,s); };
	inline void copy(const char *s, size_t n) { fit(n+1); strncpy(buf->str,s,n); };
	inline void link(gld_string&s) { unlink(); buf=(STRBUF*)s; buf->nrefs++;};
	inline void unlink() { if ( buf->nrefs<=1 ) {free(buf->str); free(buf);} else buf->nrefs--; };
public: // status accessors
	/// determine whether a string is valid
	inline bool is_valid(void) { return buf!=NULL; };
	/// determine whether a string is null
	inline bool is_null(void) { return is_valid() && buf->str==NULL; };
public: // read accessors
	/// get a pointer to the string buffer
	inline const char* get_buffer(void) { return buf ? buf->str : NULL; };
	/// get the size of the string buffer (as allocated)
	inline size_t get_size(void) { return buf ? buf->len : -1; };
	/// get the length of the string (up to the null termination)
	inline size_t get_length(void) { return buf && buf->str ? strlen(buf->str) : -1; };
public: // write accessors
	/// set the string
	inline void set_string(const char *s) { copy(s); };
	/// set the string buffer size
	inline void set_size(size_t n) { fit(n); };
	/// format the string (a la printf)
	inline size_t format(const char *fmt,...) { va_list ptr; va_start(ptr,fmt); int len=vsnprintf(buf->str,buf->len,fmt,ptr); va_end(ptr); return len;};
	/// format the string with a specific buffer size
	inline size_t format(size_t len,const char *fmt,...) { fit(len); va_list ptr; va_start(ptr,fmt); int rv=vsnprintf(buf->str,buf->len,fmt,ptr); va_end(ptr); return rv;};
public: // compare ops
	/// alphabetic before comparison
	inline bool operator<(const char*s) { return strcmp(buf->str,s)<0; };
	/// alphabetic before or same comparison
	inline bool operator<=(const char*s) { return strcmp(buf->str,s)<=0; };
	/// alphabetic same comparison
	inline bool operator==(const char*s) { return strcmp(buf->str,s)==0; };
	/// alphabetic after or same comparison
	inline bool operator>=(const char*s) { return strcmp(buf->str,s)>=0; };
	/// alphabetic after comparison
	inline bool operator>(const char*s) { return strcmp(buf->str,s)>0; };
	/// alphabetic differs comparison
	inline bool operator!=(const char*s) { return strcmp(buf->str,s)!=0; };
public: // manipulation
	/// trim left whitespace
	inline void trimleft(void) { if ( is_null() ) return; size_t n=0; while (buf->str[n]!='\0'&&isspace(*buf->str)) n++; strcpy(buf->str,buf->str+n); };
	/// trim right whitespace
	inline void trimright(void) { if ( is_null() ) return; size_t n=strlen(buf->str); while (n>0&&isspace(buf->str[n-1])) buf->str[--n]='\0'; };
	/// extract left string
	inline gld_string left(size_t n) { if ( is_null() ) return gld_string(); return gld_string(buf->str,n); };
	/// extract right string
	inline gld_string right(size_t n) { if ( is_null() ) return gld_string(); return gld_string(buf->str+buf->len-n); };
	/// extract mid string
	inline gld_string mid(size_t n, size_t m) { if ( is_null() ) return gld_string(); return gld_string(buf->str+buf->len-n,m); };
	/// locate substring
	inline size_t findstr(const char *s) { if ( is_null() ) return -1; char *p=strstr(buf->str,s); return p==NULL ? -1 : (p-buf->str); };
	/// find character
	inline size_t findchr(char c) { if ( is_null() ) return -1; char *p=strchr(buf->str,c); return p==NULL ? -1 : (p-buf->str); };
	inline size_t split(gld_string *&list, const char *delim=" ") 
	{
		// TODO
		if ( is_null() ) return 0;
		return 0;
	}
	inline gld_string merge(gld_string *&list, size_t n, const char *delim=" ")
	{
		// TODO
		return gld_string();
	}
};

/// Date/time encapsulation
class gld_clock {
private: // data
	DATETIME dt;
public: // constructors
	/// Clock constructor for current global clock
	gld_clock(void) { callback->time.local_datetime(*(callback->global_clock),&dt); }; 
	/// Clock constructor for an arbitrary TIMESTAMP
	gld_clock(TIMESTAMP ts) { if ( !callback->time.local_datetime(ts,&dt)) memset(&dt,0,sizeof(dt)); };
	/// Clock constructor for a time string
	gld_clock(char *str) { from_string(str); };
	/// Clock constructor for year, month, day, hour, minute, second, nanosecond values
	gld_clock(unsigned short y, unsigned short m=0, unsigned short d=0, unsigned short H=0, unsigned short M=0, unsigned short S=0, unsigned short int ms=0, char *tz=NULL, int dst=-1)
	{
		dt.year = y; dt.month=m; dt.day=d; dt.hour=H; dt.minute=M; dt.second=S; dt.nanosecond=ms;
		if ( dst>=0 ) dt.is_dst=dst;
		if ( tz!=NULL ) set_tz(tz); else callback->time.mkdatetime(&dt);
	}
public: // cast operators
	/// Cast to TIMESTAMP
	inline operator TIMESTAMP (void) { return dt.timestamp; };
public: // comparison operators
	/// Compare after TIMESTAMP
	inline bool operator > (TIMESTAMP t) { return dt.timestamp>t; };
	/// Compare after or same TIMESTAMP
	inline bool operator >= (TIMESTAMP t) { return dt.timestamp>=t; };
	/// Compare before TIMESTAMP
	inline bool operator < (TIMESTAMP t) { return dt.timestamp<t; };
	/// Compare before or same TIMESTAMP
	inline bool operator <= (TIMESTAMP t) { return dt.timestamp<=t; };
	/// Compare same TIMESTAMP
	inline bool operator == (TIMESTAMP t) { return dt.timestamp==t; };
	/// Compare different TIMESTAMP
	inline bool operator != (TIMESTAMP t) { return dt.timestamp!=t; };
	/// Check if valid TIMESTAMP
	inline bool is_valid(void) { return dt.timestamp>0; };
	/// Check if NEVER
	inline bool is_never(void) { return dt.timestamp==TS_NEVER; };
public: // read accessors
	/// Get the year
	inline unsigned short get_year(void) { return dt.year; };
	/// Get the month (0-11)
	inline unsigned short get_month(void) { return dt.month; };
	/// Get the day (1-31)
	inline unsigned short get_day(void) { return dt.day; };
	/// Get the hour (0-23)
	inline unsigned short get_hour(void) { return dt.hour; };
	/// Get the minute (0-59)
	inline unsigned short get_minute(void) { return dt.minute; };
	/// Get the second (0-59)
	inline unsigned short get_second(void) { return dt.second; };
	/// Get the nanosecond (0-999999)
	inline unsigned int get_nanosecond(void) { return dt.nanosecond; };
	/// Get the timezone spec
	inline char* get_tz(void) { return dt.tz; };
	/// Get the summer/daylight time flag
	inline bool get_is_dst(void) { return dt.is_dst?true:false; };
	/// Get the weekday (Sunday=0)
	inline unsigned short get_weekday(void) { return dt.weekday; };
	/// Get the day of the year (Jan 1=0)
	inline unsigned short get_yearday(void) { return dt.yearday; };
	/// Get the timezone offer (in seconds)
	inline int get_tzoffset(void) { return dt.tzoffset; };
	/// Get the TIMESTAMP value (UTC)
	inline TIMESTAMP get_timestamp(void) { return dt.timestamp; };
	/// Get the local TIMESTAMP value (ignoring DST)
	inline TIMESTAMP get_localtimestamp(void) { return dt.timestamp - dt.tzoffset; };
	/// Get the local TIMESTAMP value (including DST)
	inline TIMESTAMP get_localtimestamp_dst(bool force_dst=false) { return dt.timestamp - dt.tzoffset + (dt.is_dst||force_dst?3600:0); };
public: // write accessors
	/// Set the date
	inline TIMESTAMP set_date(unsigned short y, unsigned short m, unsigned short d) { dt.year=y; dt.month=m; dt.day=d; return callback->time.mkdatetime(&dt); };
	/// Set the time
	inline TIMESTAMP set_time(unsigned short H, unsigned short M, unsigned short S, unsigned long u=0, char *t=NULL, bool force_dst=false) { dt.hour=H; dt.minute=M; dt.second=S; dt.nanosecond=u; strncpy(dt.tz,t,sizeof(dt.tz)); if (force_dst) dt.is_dst=true; return callback->time.mkdatetime(&dt); };
	/// Set the date and time
	inline TIMESTAMP set_datetime(unsigned short y, unsigned short m, unsigned short d, unsigned short H, unsigned short M, unsigned short S, unsigned long u=0, char *t=NULL, bool force_dst=false) { dt.year=y; dt.month=m; dt.day=d; dt.hour=H; dt.minute=M; dt.second=S; dt.nanosecond=u; strncpy(dt.tz,t,sizeof(dt.tz)); if (force_dst) dt.is_dst=true; return callback->time.mkdatetime(&dt); };
	/// Set the year
	inline TIMESTAMP set_year(unsigned short y) { dt.year=y; return callback->time.mkdatetime(&dt); };
	/// Set the month (Jan=0)
	inline TIMESTAMP set_month(unsigned short m) { dt.month=m; return callback->time.mkdatetime(&dt); };
	/// Set the day (1-31)
	inline TIMESTAMP set_day(unsigned short d) { dt.day=d; return callback->time.mkdatetime(&dt); };
	/// Set the hour (0-23)
	inline TIMESTAMP set_hour(unsigned short h) { dt.hour=h; return callback->time.mkdatetime(&dt); };
	/// Set the minute (0-59)
	inline TIMESTAMP set_minute(unsigned short m) { dt.minute=m; return callback->time.mkdatetime(&dt); };
	/// Set the second (0-59)
	inline TIMESTAMP set_second(unsigned short s) { dt.second=s; return callback->time.mkdatetime(&dt); };
	/// Set the nanosecond (0-999999)
	inline TIMESTAMP set_nanosecond(unsigned int u) { dt.nanosecond=u; return callback->time.mkdatetime(&dt); };
	/// Set the timezone (see tzinfo.txt)
	inline TIMESTAMP set_tz(char* t) { strncpy(dt.tz,t,sizeof(dt.tz)); return callback->time.mkdatetime(&dt); };
	/// Set the DST flag
	inline TIMESTAMP set_is_dst(bool i) { dt.is_dst=i; return callback->time.mkdatetime(&dt); };
public: // special functions
	/// Convert from string
	inline bool from_string(char *str) { return callback->time.local_datetime(callback->time.convert_to_timestamp(str),&dt)?true:false; };
	/// Convert to string
	inline unsigned int to_string(char *str, int size) {return callback->time.convert_from_timestamp(dt.timestamp,str,size); };
	/// Extract the total number of days since 1/1/1970 0:00:00 UTC
	inline double to_days(TIMESTAMP ts=0) { return (dt.timestamp-ts)/86400.0 + dt.nanosecond*1e-9; };
	/// Extract the total number of hours since 1/1/1970 0:00:00 UTC
	inline double to_hours(TIMESTAMP ts=0) { return (dt.timestamp-ts)/3600.0 + dt.nanosecond*1e-9; };
	/// Extract the total number of minutes since 1/1/1970 0:00:00 UTC
	inline double to_minutes(TIMESTAMP ts=0) { return (dt.timestamp-ts)/60.0 + dt.nanosecond*1e-9; };
	/// Extract the total number of seconds since 1/1/1970 0:00:00 UTC
	inline double to_seconds(TIMESTAMP ts=0) { return dt.timestamp-ts + dt.nanosecond*1e-9; };
	/// Extract the total number of nanoseconds since 1/1/1970 0:00:00 UTC
	inline double to_nanoseconds(TIMESTAMP ts=0) { return (dt.timestamp-ts)*1e9 + dt.nanosecond; };
	/// Get the timestamp as a string
	inline gld_string get_string(const size_t sz=1024) 
	{
		gld_string res;
		char buf[1024];
		if ( sizeof(buf)<sz ) throw "get_string() over size limit";
		if ( to_string(buf,(int)sz)>=0 )
			res = buf;
		return res;
	};
};

/// Read lock container
class gld_rlock {
private: OBJECT *my;
	/// Constructor
public: gld_rlock(OBJECT *obj) : my(obj) {::rlock(&obj->lock);}; 
	/// Destructor
public: ~gld_rlock(void) {::runlock(&my->lock);};
};
/// Write lock container
class gld_wlock {
private: OBJECT *my;
		 /// Constructor
public: gld_wlock(OBJECT *obj) : my(obj) {::wlock(&obj->lock);}; 
		/// Destructor
public: ~gld_wlock(void) {::wunlock(&my->lock);};
};

class gld_class;
/// Module container
class gld_module {

private: // data
	MODULE core;

public: // constructors/casts
	/// Constructor
	inline gld_module(void) { MODULE *m = callback->module.getfirst(); if (m) core=*m; else throw "no modules loaded";};
	/// Cast to MODULE
	inline operator MODULE*(void) { return &core; };

public: // read accessors
	/// Get module name
	inline char* get_name(void) { return core.name; };
	/// Get module major version number
	inline unsigned short get_major(void) { return core.major; };
	/// Get module minor version number
	inline unsigned short get_minor(void) { return core.minor; };
	/// Get first class in module
	inline gld_class* get_first_class(void) { return (gld_class*)core.oclass; };

public: // write accessors

public: // iterators
	/// Check if last module loaded
	inline bool is_last(void) { return core.next==NULL; };
	/// Get next module loaded
	inline void get_next(void) { core = *(core.next); };
};

class gld_property;
class gld_function;
/// Class container
class gld_class {

private: // data
	CLASS core;

public: // constructors
	/// Constructor (blocker implementation)
	inline gld_class(void) { throw "gld_class constructor not permitted"; };
	/// Cast to CLASS
	inline operator CLASS*(void) { return &core; };

public: // read accessors
	/// Get class name
	inline char* get_name(void) { return core.name; };
	/// Get class size
	inline size_t get_size(void) { return core.size; };
	/// Get class parent
	inline gld_class* get_parent(void) { return (gld_class*)core.parent; };
	/// Get module that implements the class
	inline gld_module* get_module(void) { return (gld_module*)core.module; };
	/// Get the first property in the class
	inline gld_property* get_first_property(void) { return (gld_property*)core.pmap; };
	/// Get the next property in the class
	inline gld_property* get_next_property(PROPERTY*p) { PROPERTY *prop=(PROPERTY*)p->next; return ( prop && prop->oclass==&core ) ? (gld_property*)prop : NULL; };
	/// Get the first function in the class
	inline gld_function* get_first_function(void) { return (gld_function*)core.fmap; };
	/// Get the next function in the class
	inline gld_function* get_next_function(FUNCTION*f) { return (gld_function*)f->next; };
	/// Get TRL 
	inline TECHNOLOGYREADINESSLEVEL get_trl(void) { return core.trl; };

public: // write accessors
	/// Set TRL
	inline void set_trl(TECHNOLOGYREADINESSLEVEL t) { core.trl=t; };

public: // special functions
	/// Register a class	
	static inline CLASS *create(MODULE *m, char *n, size_t s, unsigned int f) { return callback->register_class(m,n,(unsigned int)s,f); };
	
public: // iterators
	/// Check if last class registered
	inline bool is_last(void) { return core.next==NULL; };
	/// Check if last class defined by this class' module
	inline bool is_module_last(void) { return core.next==NULL || core.module!=core.next->module; };
	/// Get the next class
	inline gld_class* get_next(void) { return (gld_class*)core.next; };
};

/// Function container
class gld_function {

private: // data
	FUNCTION core;

public: // constructors
	/// Construct a function (blocker implementation)
	inline gld_function(void) { throw "gld_function constructor not permitted"; };
	/// Cast to a FUNCTION pointer
	inline operator FUNCTION*(void) { return &core; };

public: // read accessors
	/// Get function name
	inline char *get_name(void) { return core.name; };
	/// Get function class
	inline gld_class* get_class(void) { return (gld_class*)core.oclass; };
	/// Get function address
	inline FUNCTIONADDR get_addr(void) { return core.addr; };

public: // write accessors

public: // iterators
	/// Check whether this is the last function defined
	inline bool is_last(void) { return core.next==NULL; };
	/// Get the next function in the list
	inline gld_function* get_next(void) { return (gld_function*)core.next; };
};

/// Built-in type container
class gld_type {

private: // data
	PROPERTYTYPE type;

public: // constructors/casts
	/// Constructor
	inline gld_type(PROPERTYTYPE t) : type(t) {};
	/// Cast to PROPERTYTYPE
	inline operator PROPERTYTYPE(void) { return type; };

public: // read accessors
	// TODO size,conversions,etc...

public: // write accessors

public: // iterators
	/// Get the first property type
	static inline PROPERTYTYPE get_first(void) { return PT_double; };
	/// Get the next property type
	inline PROPERTYTYPE get_next(void) { return (PROPERTYTYPE)(((int)type)+1); };
	/// Check whether this is the last property type
	inline bool is_last(void) { return (PROPERTYTYPE)(((int)type)+1)==_PT_LAST; }; 
};

/// Unit container
class gld_unit {

private: // data
	UNIT core;

public: // constructors/casts
	/// Construct empty unit container
	inline gld_unit(void) { memset(&core,0,sizeof(core)); };
	/// Construct a container for a named or derived unit
	inline gld_unit(char *name) { UNIT *unit=callback->unit_find(name); if (unit) memcpy(&core,unit,sizeof(UNIT)); else memset(&core,0,sizeof(UNIT)); };
	/// Cast to a UNIT structure
	inline operator UNIT*(void) { return &core; };

public: // read accessors
	/// Get the name or derivation of the unit
	inline char* get_name(void) { return core.name; };
	/// Get the C exponent
	inline double get_c(void) { return core.c; };
	/// Get the E exponent
	inline double get_e(void) { return core.e; };
	/// Get the H exponent
	inline double get_h(void) { return core.h; };
	/// Get the K exponent
	inline double get_k(void) { return core.k; };
	/// Get the M exponent
	inline double get_m(void) { return core.m; };
	/// Get the S exponent
	inline double get_s(void) { return core.s; };
	/// Get the A constant
	inline double get_a(void) { return core.a; };
	/// Get the B constant
	inline double get_b(void) { return core.b; };
	/// Get the unit precision
	inline int get_prec(void) { return core.prec; };
	/// Check whether the unit is valid
	inline bool is_valid(void) { return core.name[0]!='\0'; };

public: // write accessors
	/// Change the unit
	inline bool set_unit(char *name){ UNIT *unit=callback->unit_find(name); if (unit) {memcpy(&core,unit,sizeof(UNIT));return true;} else {memset(&core,0,sizeof(UNIT));return false;} };

public: // special functions
	/// Convert a value to another named or derived unit
	inline bool convert(char *name, double &value) { UNIT *unit=callback->unit_find(name); return unit&&(callback->unit_convert_ex(&core,unit,&value))?true:false; }
	/// Convert a value to another UNIT
	inline bool convert(UNIT *unit, double &value) { return callback->unit_convert_ex(&core,unit,&value)?true:false; }
	/// Convert a value to another gld_unit
	inline bool convert(gld_unit &unit, double &value) { return callback->unit_convert_ex(&core,(UNIT*)unit,&value)?true:false; }

public: // iterators
	/// Check whether this is the last defined unit
	inline bool is_last(void) { return core.next==NULL?true:false; };
	/// Get the next unit
	inline gld_unit* get_next(void) { return (gld_unit*)core.next; };
};

/// Keyword container
class gld_keyword {

private: // data
	KEYWORD core;

public: // constructors/casts
	/// Construct a key word
	inline gld_keyword(void) { throw "gld_keyword constructor not permitted"; };
	/// Cast to a keyword pointer
	inline operator KEYWORD* (void) { return &core; };

public: // read accessors
	/// Get the name of a keyword
	inline char* get_name(void) { return core.name; };
	/// Get the bit pattern for the keyword
	inline unsigned int64 get_value(void) { return core.value; };

public: // write accessors

public: // iterators
	/// Get the next keyword (NULL if last)
	inline gld_keyword* get_next(void) { return (gld_keyword*)core.next; };
};


// object data declaration/accessors
/// Define an atomic property
#define GL_ATOMIC(T,X) protected: T X; public: \
	static inline size_t get_##X##_offset(void) { return (char*)&(defaults->X)-(char*)defaults; }; \
	inline T get_##X(void) { return X; }; \
	inline gld_property get_##X##_property(void) { return gld_property(my(),#X); }; \
	inline T get_##X(gld_rlock&) { return X; }; \
	inline T get_##X(gld_wlock&) { return X; }; \
	inline void set_##X(T p) { X=p; }; \
	inline void set_##X(T p, gld_wlock&) { X=p; }; 
/// Define a structured property
#define GL_STRUCT(T,X) protected: T X; public: \
	static inline size_t get_##X##_offset(void) { return (char*)&(defaults->X)-(char*)defaults; }; \
	inline T get_##X(void) { gld_rlock _lock(my()); return X; }; \
	inline gld_property get_##X##_property(void) { return gld_property(my(),#X); }; \
	inline T get_##X(gld_rlock&) { return X; }; \
	inline T get_##X(gld_wlock&) { return X; }; \
	inline void set_##X(T p) { gld_wlock _lock(my()); X=p; }; \
	inline void set_##X(T p, gld_wlock&) { X=p; }; 
/// Define a string property
#define GL_STRING(T,X) 	protected: T X; public: \
	static inline size_t get_##X##_offset(void) { return (char*)&(defaults->X)-(char*)defaults; }; \
	inline char* get_##X(void) { gld_rlock _lock(my()); return X.get_string(); }; \
	inline gld_property get_##X##_property(void) { return gld_property(my(),#X); }; \
	inline char* get_##X(gld_rlock&) { return X.get_string(); }; \
	inline char* get_##X(gld_wlock&) { return X.get_string(); }; \
	inline char get_##X(size_t n) { gld_rlock _lock(my()); return X[n]; }; \
	inline char get_##X(size_t n, gld_rlock&) { return X[n]; }; \
	inline char get_##X(size_t n, gld_wlock&) { return X[n]; }; \
	inline void set_##X(char *p) { gld_wlock _lock(my()); strncpy(X,p,sizeof(X)); }; \
	inline void set_##X(char *p, gld_wlock&) { strncpy(X,p,sizeof(X)); }; \
	inline void set_##X(size_t n, char c) { gld_wlock _lock(my()); X[n]=c; }; \
	inline void set_##X(size_t n, char c, gld_wlock&) { X[n]=c; }; 
/// Define an array property
#define GL_ARRAY(T,X,S) protected: T X[S]; public: \
	static inline size_t get_##X##_offset(void) { return (char*)&(defaults->X)-(char*)defaults; }; \
	inline gld_property get_##X##_property(void) { return gld_property(my(),#X); }; \
	inline T* get_##X(void) { gld_rlock _lock(my()); return X; }; \
	inline T* get_##X(gld_rlock&) { return X; }; \
	inline T* get_##X(gld_wlock&) { return X; }; \
	inline T get_##X(size_t n) { gld_rlock _lock(my()); return X[n]; }; \
	inline T get_##X(size_t n, gld_rlock&) { return X[n]; }; \
	inline T get_##X(size_t n, gld_wlock&) { return X[n]; }; \
	inline void set_##X(T* p) { gld_wlock _lock(my()); memcpy(X,p,sizeof(X)); }; \
	inline void set_##X(T* p, gld_wlock&) { memcpy(X,p,sizeof(X)); }; \
	inline void set_##X(size_t n, T m) { gld_wlock _lock(my()); X[n]=m; }; \
	inline void set_##X(size_t n, T m, gld_wlock&) { X[n]=m; }; 
/// Define a bitflag property
#define GL_BITFLAGS(T,X) protected: T X; public: \
	static inline size_t get_##X##_offset(void) { return (char*)&(defaults->X)-(char*)defaults; }; \
	inline T get_##X(T mask=-1) { return X&mask; }; \
	inline gld_property get_##X##_property(void) { return gld_property(my(),#X); }; \
	inline T get_##X(gld_rlock&) { return X; }; \
	inline T get_##X(gld_wlock&) { return X; }; \
	inline void set_##X(T p) { X=p; }; \
	inline void set_##X##_bits(T p) { gld_rlock _lock(my()); (X)|=(p); }; \
	inline void clr_##X##_bits(T p) { gld_rlock _lock(my()); (X)&=~(p); }; \
	inline void set_##X(T p, gld_wlock&) { X=p; }; 

/// Set bits of a bitflag property
inline void setbits(unsigned long &flags, unsigned int bits) { flags|=bits; }; 
/// Clear bits of a bitflag property
inline void clearbits(unsigned long &flags, unsigned int bits) { flags&=~bits; }; 
/// Test bits of a bitflag property
inline bool hasbits(unsigned long flags, unsigned int bits) { return (flags&bits) ? true : false; };

/// Object container
class gld_object {
public:
	inline OBJECT *my() { return this?(((OBJECT*)this)-1):NULL; }
public:
	inline gld_object &operator=(gld_object&o) { exception("copy constructor is forbidden on gld_object"); };

public: // constructors
	inline static gld_object *find_object(char *n) { OBJECT *obj = callback->get_object(n); if (obj) return (gld_object*)(obj+1); else return NULL; };

public: // header read accessors (no locking)
	inline OBJECTNUM get_id(void) { return my()->id; };
	inline char* get_groupid(void) { return my()->groupid.get_string(); };
	inline gld_class* get_oclass(void) { return (gld_class*)my()->oclass; };
	inline gld_object* get_parent(void) { return my()->parent?OBJECTDATA(my()->parent,gld_object):NULL; };
	inline OBJECTRANK get_rank(void) { return my()->rank; };
	inline TIMESTAMP get_clock(void) { return my()->clock; };
	inline TIMESTAMP get_valid_to(void) { return my()->valid_to; };
	inline TIMESTAMP get_schedule_skew(void) { return my()->schedule_skew; };
	inline FORECAST* get_forecast(void) { return my()->forecast; };
	inline double get_latitude(void) { return my()->latitude; };
	inline double get_longitude(void) { return my()->longitude; };
	inline TIMESTAMP get_in_svc(void) { return my()->in_svc; };
	inline TIMESTAMP get_out_svc(void) { return my()->out_svc; };
	inline const char* get_name(void) { static char _name[sizeof(CLASS)+16]; return my()->name?my()->name:(sprintf(_name,"%s:%d",my()->oclass->name,my()->id),_name); };
	inline NAMESPACE* get_space(void) { return my()->space; };
	inline unsigned int get_lock(void) { return my()->lock; };
	inline unsigned int get_rng_state(void) { return my()->rng_state; };
	inline TIMESTAMP get_heartbeat(void) { return my()->heartbeat; };
	inline unsigned long get_flags(unsigned long mask=0xffffffff) { return (my()->flags)&mask; };

protected: // header write accessors (no locking)
	inline void set_clock(TIMESTAMP ts=0) { my()->clock=(ts?ts:gl_globalclock); };
	inline void set_heartbeat(TIMESTAMP dt) { my()->heartbeat=dt; };
	inline void set_forecast(FORECAST *fs) { my()->forecast=fs; };
	inline void set_latitude(double x) { my()->latitude=x; };
	inline void set_longitude(double x) { my()->longitude=x; };
	inline void set_flags(unsigned long flags) { my()->flags=flags; };
	inline void set_flags_bits(unsigned long bits) { my()->flags|=bits; };
	inline void unset_flags_bits(unsigned long bits) { my()->flags&=~bits; };

protected: // locking (self)
	inline void rlock(void) { ::rlock(&my()->lock); };
	inline void runlock(void) { ::runlock(&my()->lock); };
	inline void wlock(void) { ::wlock(&my()->lock); };
	inline void wunlock(void) { ::wunlock(&my()->lock); };
protected: // locking (others)
	inline void rlock(OBJECT *obj) { ::rlock(&obj->lock); };
	inline void runlock(OBJECT *obj) { ::runlock(&obj->lock); };
	inline void wlock(OBJECT *obj) { ::wlock(&obj->lock); };
	inline void wunlock(OBJECT *obj) { ::wunlock(&obj->lock); };

protected: // special functions
	inline bool operator == (gld_object *o) { return o!=NULL && my()==o->my(); };
	inline bool operator == (OBJECT *o) { return o!=NULL && my()==o; };

public: // member lookup functions
	inline PROPERTY* get_property(char *name, PROPERTYSTRUCT *pstruct=NULL) { return callback->properties.get_property(my(),name,pstruct); };
	inline FUNCTIONADDR get_function(char *name) { return (*callback->function.get)(my()->oclass->name,name); };

public: // external accessors
	template <class T> inline void getp(PROPERTY &prop, T &value) { rlock(); value=*(T*)(GETADDR(my(),&prop)); wunlock(); };
	template <class T> inline void setp(PROPERTY &prop, T &value) { wlock(); *(T*)(GETADDR(my(),&prop))=value; wunlock(); };
	template <class T> inline void getp(PROPERTY &prop, T &value, gld_rlock&) { value=*(T*)(GETADDR(my(),&prop)); };
	template <class T> inline void getp(PROPERTY &prop, T &value, gld_wlock&) { value=*(T*)(GETADDR(my(),&prop)); };
	template <class T> inline void setp(PROPERTY &prop, T &value, gld_wlock&) { *(T*)(GETADDR(my(),&prop))=value; };

public: // core interface
	inline int set_dependent(OBJECT *obj) { return callback->object.set_dependent(my(),obj); };
	inline int set_parent(OBJECT *obj) { return callback->object.set_parent(my(),obj); };
	inline int set_rank(unsigned int r) { return callback->object.set_rank(my(),r); };
	inline bool isa(char *type) { return callback->object_isa(my(),type) ? true : false; };
	inline bool is_valid(void) { return my()!=NULL && my()==OBJECTHDR(this); };

public: // iterators
	inline bool is_last(void) { return my()->next==NULL; };
	inline static gld_object *get_first(void) { OBJECT *o=callback->object.get_first(); return OBJECTDATA(o,gld_object);};
	inline gld_object* get_next(void) { return OBJECTDATA(my()->next,gld_object); };

public: // exceptions
	inline void exception(char *msg, ...) { static char buf[1024]; va_list ptr; va_start(ptr,msg); vsprintf(buf+sprintf(buf,"%s: ",get_name()),msg,ptr); va_end(ptr); throw (const char*)buf;};
};
/// Create a gld_object from an OBJECT
static inline gld_object* get_object(OBJECT*obj)
{
	return obj ? (gld_object*)(obj+1) : NULL;
}
/// Find a gld_object from an object name
static inline gld_object* get_object(char *n)
{
	OBJECT *obj = callback->get_object(n);
	return get_object(obj);
}

static PROPERTYSTRUCT nullpstruct;
/// Property container
class gld_property {

private: // data
	PROPERTYSTRUCT pstruct;
	OBJECT *obj;

public: // constructors/casts
	inline gld_property(void) : obj(NULL), pstruct(nullpstruct) {};
	inline gld_property(gld_object *o, char *n) : obj(o->my()), pstruct(nullpstruct)  
	{ 
		if (o) 
			callback->properties.get_property(o->my(),n,&pstruct); 
		else 
		{
			GLOBALVAR *v=callback->global.find(n); 
			pstruct.prop= (v?v->prop:NULL);
		} 
	};
	inline gld_property(OBJECT *o, char *n) : obj(o), pstruct(nullpstruct)  
	{ 
		if (o) 
			callback->properties.get_property(o,n,&pstruct); 
		else 
		{
			GLOBALVAR *v=callback->global.find(n); 
			pstruct.prop= (v?v->prop:NULL);
		} 
	};
	inline gld_property(OBJECT *o) : obj(o), pstruct(nullpstruct) { pstruct.prop=o->oclass->pmap; };
	inline gld_property(OBJECT *o, PROPERTY *p) : obj(o), pstruct(nullpstruct) { pstruct.prop=p; };
	inline gld_property(GLOBALVAR *v) : obj(NULL), pstruct(nullpstruct) { pstruct.prop=v->prop; };
	inline gld_property(char *n) : obj(NULL), pstruct(nullpstruct) 
	{ 
		GLOBALVAR *v=callback->global.find(n); 
		pstruct.prop = (v?v->prop:NULL);  
	};
	inline gld_property(char *m, char *n) : obj(NULL), pstruct(nullpstruct) 
	{ 
		char1024 vn; 
		sprintf(vn,"%s::%s",m,n); 
		GLOBALVAR *v=callback->global.find(vn); 
		pstruct.prop= (v?v->prop:NULL);  
	};
	inline operator PROPERTY*(void) { return pstruct.prop; };

public: // read accessors
	inline OBJECT *get_object(void) { return obj; };
	inline PROPERTY *get_property(void) { return pstruct.prop; };
	inline gld_class* get_class(void) { return (gld_class*)pstruct.prop->oclass; };
	inline char *get_name(void) { return pstruct.prop->name; };
	inline gld_type get_type(void) { return gld_type(pstruct.prop->ptype); };
	inline size_t get_size(void) { return (size_t)(pstruct.prop->size); };
	inline size_t get_width(void) { return (size_t)(pstruct.prop->width); };
	inline PROPERTYACCESS get_access(void) { return pstruct.prop->access; };
	inline gld_unit* get_unit(void) { return (gld_unit*)pstruct.prop->unit; };
	inline void* get_addr(void) { return obj?((void*)((char*)(obj+1)+(unsigned int64)(pstruct.prop->addr))):pstruct.prop->addr; };
	inline gld_keyword* get_first_keyword(void) { return (gld_keyword*)pstruct.prop->keywords; };
	inline char* get_description(void) { return pstruct.prop->description; };
	inline PROPERTYFLAGS get_flags(void) { return pstruct.prop->flags; };
	inline int to_string(char *buffer, int size) { return callback->convert.property_to_string(pstruct.prop,get_addr(),buffer,size); };
	inline gld_string get_string(const size_t sz=1024)
	{
		gld_string res;
		char buf[1024];
		if ( sizeof(buf)<sz ) throw "get_string() over size limit";
		if ( to_string(buf,(int)sz)>=0 )
			res = buf;
		return res;
	};
	inline int from_string(char *string) { return callback->convert.string_to_property(pstruct.prop,get_addr(),string); };
	inline char *get_partname(void) { return pstruct.part; };
	inline double get_part(char *part=NULL) { return callback->properties.get_part(obj,pstruct.prop,part?part:pstruct.part); };

public: // write accessors
	inline void set_object(OBJECT *o) { obj=o; };
	inline void set_object(gld_object *o) { obj=o->my(); };
	inline void set_property(char *n) { callback->properties.get_property(obj,n,&pstruct); };
	inline void set_property(PROPERTY *p) { pstruct.prop=p; };

public: // special operations
	inline bool is_valid(void) { return pstruct.prop!=NULL; }
	inline bool has_part(void) { return pstruct.part[0]!='\0'; };
	inline bool is_complex(void) { if(pstruct.prop->ptype == PT_complex) return true; return false;}
	inline bool is_double(void) { switch(pstruct.prop->ptype) { case PT_double: case PT_random: case PT_enduse: case PT_loadshape: return true; default: return false;} };
	inline bool is_integer(void) { switch(pstruct.prop->ptype) { case PT_int16: case PT_int32: case PT_int64: return true; default: return false;} };
	inline double get_double(void) { errno=0; switch(pstruct.prop->ptype) { case PT_double: case PT_random: case PT_enduse: case PT_loadshape: return has_part() ? get_part() : *(double*)get_addr(); default: errno=EINVAL; return NaN;} };
	inline int64 get_integer(void) { errno=0; switch(pstruct.prop->ptype) { case PT_int16: return (int64)*(int16*)get_addr(); case PT_int32: return (int64)*(int32*)get_addr(); case PT_int64: return *(int64*)get_addr(); default: errno=EINVAL; return 0;} };
	template <class T> inline void getp(T &value) { ::rlock(&obj->lock); value = *(T*)get_addr(); ::runlock(&obj->lock); };
	template <class T> inline void setp(T &value) { ::wlock(&obj->lock); *(T*)get_addr()=value; ::wunlock(&obj->lock); };
	template <class T> inline void getp(T &value, gld_rlock&) { value = *(T*)get_addr(); };
	template <class T> inline void getp(T &value, gld_wlock&) { value = *(T*)get_addr(); };
	template <class T> inline void setp(T &value, gld_wlock&) { *(T*)get_addr()=value; };
	inline gld_keyword* find_keyword(unsigned long value) { gld_keyword*k=get_first_keyword();while(k && k->get_value()!=value) {k=k->get_next();} return k; }; // TODO
	inline gld_keyword* find_keyword(char *name) 
	{ 
		gld_keyword *k = get_first_keyword();
		while ( k && strcmp(k->get_name(),name)!=0 ) 
			k=k->get_next();
		return k; 
	};
	inline bool compare(char *op, char *a, char *b=NULL, char *p=NULL) 
	{ 
		PROPERTYCOMPAREOP n = callback->properties.get_compare_op(pstruct.prop->ptype,op); 
		if (n==TCOP_ERR) throw "invalid property compare operation"; 
		return compare((enumeration)n,a,b,p); 
	};
	inline bool compare(enumeration op, char *a, char *b=NULL) 
	{ 
		char v1[1024], v2[1024]; 
		return callback->convert.string_to_property(pstruct.prop,(void*)v1,a)>0 && callback->properties.compare_basic(pstruct.prop->ptype,(PROPERTYCOMPAREOP)op,get_addr(),(void*)v1,(b&&callback->convert.string_to_property(pstruct.prop,(void*)v2,b)>0)?(void*)v2:NULL, NULL);
	};
	inline bool compare(enumeration op, char *a, char *b, char *p) 
	{
		double v1, v2; v1=atof(a); v2=b?atof(b):0;
		return callback->properties.compare_basic(pstruct.prop->ptype,(PROPERTYCOMPAREOP)op,get_addr(),(void*)&v1,b?(void*)&v2:NULL, p);
	};
	inline bool compare(enumeration op, double *a, double *b=NULL, char *p=NULL) 
	{ 
		return callback->properties.compare_basic(pstruct.prop->ptype,(PROPERTYCOMPAREOP)op,get_addr(),a,b,p);
	};
	inline bool compare(enumeration op, void *a, void *b=NULL) 
	{ 
		return callback->properties.compare_basic(pstruct.prop->ptype,(PROPERTYCOMPAREOP)op,get_addr(),a,b,NULL);
	};

public: // iterators
	inline bool is_last(void) { return pstruct.prop==NULL || pstruct.prop->next==NULL || pstruct.prop->oclass!=pstruct.prop->next->oclass; };
	inline PROPERTY* get_next(void) { return is_last() ? NULL : pstruct.prop->next; };

public: // comparators
	inline bool operator == (char* a) { return compare(TCOP_EQ,a,NULL); };
	inline bool operator <= (char* a) { return compare(TCOP_LE,a,NULL); };
	inline bool operator >= (char* a) { return compare(TCOP_GE,a,NULL); };
	inline bool operator != (char* a) { return compare(TCOP_NE,a,NULL); };
	inline bool operator < (char* a) { return compare(TCOP_LT,a,NULL); };
	inline bool operator > (char* a) { return compare(TCOP_GT,a,NULL); };
	inline bool inside(char* a, char* b) { return compare(TCOP_IN,a,b); };
	inline bool outside(char* a, char* b) { return compare(TCOP_NI,a,b); };

private: // exceptions
	inline void exception(char *msg, ...) 
	{ 
		static char buf[1024]; 
		va_list ptr; 
		va_start(ptr,msg); 
		vsprintf(buf+sprintf(buf,"%s.%s: ",OBJECTDATA(obj,gld_object)->get_name(),pstruct.prop->name),msg,ptr); 
		va_end(ptr); 
		throw (const char*)buf;
	};
};

/// Global variable container
class gld_global {

private: // data
	GLOBALVAR *var;

public: // constructors
	inline gld_global(void) : var(NULL) {};
	inline gld_global(GLOBALVAR *v) : var(v) {};
	inline gld_global(char *n) { var=callback->global.find(n); };
	inline gld_global(char *n, PROPERTYTYPE t, void *p) { var=callback->global.create(n,t,p,NULL); };

public: // read accessors
	inline operator GLOBALVAR*(void) { return var; };
	inline bool is_valid(void) { return var!=NULL; };
	inline PROPERTY* get_property(void) { if (!var) return NULL; return var->prop; };
	inline unsigned long get_flags(void) { if (!var) return -1; return var->flags; };
	inline size_t to_string(char *bp, size_t sz) { if (!var) return -1; gld_property p(var); return p.to_string(bp,(int)sz); };
	inline gld_string get_string(const size_t sz=1024)
	{
		gld_string res;
		char buf[1024];
		if ( sizeof(buf)<sz ) throw "get_string() over size limit";
		if ( to_string(buf,(int)sz)>=0 )
			res = buf;
		return res;
	};
	inline bool get_bool(void) { return *(bool*)(var->prop->addr); };
	inline int16 get_int16(void) { return *(int16*)(var->prop->addr); };
	inline int32 get_int32(void) { return *(int32*)(var->prop->addr); };
	inline int64 get_int64(void) { return *(int64*)(var->prop->addr); };
	inline double get_double(void) { return *(double*)(var->prop->addr); };
	inline complex get_complex(void) { return *(complex*)(var->prop->addr); };
	inline TIMESTAMP get_timestamp(void) { return *(TIMESTAMP*)(var->prop->addr); };

public: // write accessors
	inline size_t from_string(char *bp) { if (!var) return -1; gld_property p(var); return p.from_string(bp); };
	inline bool get(char *n) { var=callback->global.find(n); return var!=NULL; };
	inline bool create(char *n, PROPERTYTYPE t, void *p) { var=callback->global.create(n,t,p,NULL); return var!=NULL; };

public: // external accessors
	// TODO

public: // iterators
	inline bool is_last(void) { if (!var) return false; else return (var->next==NULL); };
	inline GLOBALVAR* get_next(void) { if (!var) return NULL; else return var->next; };
};

/// Aggregation container
class gld_aggregate {
private:
	AGGREGATION *aggr;
public:
	inline gld_aggregate(void) { aggr=NULL; };
	inline gld_aggregate(char *spec, char *group) { set_aggregate(spec,group); };
public:
	inline bool set_aggregate(char *spec, char *group) { aggr=callback->aggregate.create(spec,group); return aggr!=NULL; };
	inline bool is_valid(void) { return aggr!=NULL; };
	inline double get_value(void) { if (!aggr) throw "null aggregate"; return callback->aggregate.refresh(aggr); };
};

/// Object list container
class gld_objlist {
private:
	struct s_objlist *list;
public:
	inline operator OBJLIST*() { return list; };
public:
	inline gld_objlist(void) : list(NULL) {};
	inline gld_objlist(char *group) { list=callback->objlist.search(group); };
	inline gld_objlist(CLASS *c, PROPERTY *m, char *p, char *o, void *a, void *b=NULL) { list=callback->objlist.create(c,m,p,o,a,b); };
	inline gld_objlist(char *cn, char *mn, char *p, char *o, void *a, void *b=NULL) 
	{ 
		CLASS *c=callback->class_getname(cn); if (!c) exception("gld_objlist(): class '%s' is not found",cn); 
		PROPERTY *m=callback->find_property(c,mn); if (!m) exception("gld_objlist(): property '%s' is not found in class '%s'",mn,cn);
		list=callback->objlist.create(c,m,p,o,a,b); 
	};
	inline ~gld_objlist(void) { callback->objlist.destroy(list); };
public:
	inline size_t set(char *group) { if ( list ) callback->objlist.destroy(list); list=callback->objlist.search(group); return list?list->size:-1; };
	inline size_t add(PROPERTY *m, char *p, char *o, void *a, void *b=NULL) { return callback->objlist.add(list,m,p,o,a,b); };
	inline size_t del(PROPERTY *m, char *p, char *o, void *a, void *b=NULL) { return callback->objlist.add(list,m,p,o,a,b); };
	inline size_t add(char *cn, char *mn, char *p, char *o, void *a, void *b=NULL) 
	{
		CLASS *c=callback->class_getname(cn); if (!c) exception("gld_objlist(): class '%s' is not found",cn); 
		PROPERTY *m=callback->find_property(c,mn); if (!m) exception("gld_objlist(): property '%s' is not found in class '%s'",mn,cn);
		return callback->objlist.add(list,m,p,o,a,b); 
	};
	inline size_t del(char *cn, char *mn, char *p, char *o, void *a, void *b=NULL) 
	{ 
		CLASS *c=callback->class_getname(cn); if (!c) exception("gld_objlist(): class '%s' is not found",cn); 
		PROPERTY *m=callback->find_property(c,mn); if (!m) exception("gld_objlist(): property '%s' is not found in class '%s'",mn,cn);
		return callback->objlist.add(list,m,p,o,a,b); 
	};
public:
	inline bool is_valid(void) { return list!=NULL; };
	inline size_t get_size(void) { return list->size; };
	inline OBJECT *get(size_t n) { return list->objlist[n]; };
	inline int apply(void *arg, int (*function)(OBJECT *,void*,int)) { return callback->objlist.apply(list,arg,function);};
	inline void exception(char *msg, ...) { static char buf[1024]; va_list ptr; va_start(ptr,msg); vsprintf(buf,msg,ptr); va_end(ptr); throw (const char*)buf;};
};

/// Web data container
class gld_webdata {
private:
	struct s_http {
		struct {
			char *data;
			int size;
		} header, body; // keep consistent with struct s_http_result in core/http_client.h
		int status;
	} *result;
public:
	inline gld_webdata(void) {result=NULL;};
	inline gld_webdata(char *url, size_t maxlen=4096) {open(url,maxlen);};
	inline ~gld_webdata(void) {};
public:
	inline bool open(char *url, size_t maxlen=4096) { result = (struct s_http*)callback->http.read(url,(int)maxlen); return is_valid();};
	inline void close(void) { callback->http.free((void*)result);};
	inline bool is_valid(void) { return result!=NULL; };
	inline char *get_header(void) { return result->header.data;};
	inline size_t get_header_size(void) { return result->header.size; };
	inline char *get_body(void) { return result->body.data; };
	inline size_t get_body_size(void) { return result->body.size; };
	inline int get_status(void) { return result->status; };
};
////////////////////////////////////////////////////////////////////////////////////
// Module-Core Linkage Export Macros
////////////////////////////////////////////////////////////////////////////////////

#ifdef DLMAIN
EXPORT int do_kill(void*);
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
EXPORT int major=MAJOR, minor=MINOR; 
BOOL APIENTRY DllMain(HANDLE h, DWORD r) { if (r==DLL_PROCESS_DETACH) do_kill(h); return TRUE; }
#else // !WIN32
CDECL int dllinit() __attribute__((constructor));
CDECL int dllkill() __attribute__((destructor));
CDECL int dllinit() { return 0; }
CDECL int dllkill() { do_kill(NULL); }
#endif // !WIN32
#elif defined CONSOLE
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "console.h"
#endif // DLMAIN

#define EXPORT_CREATE_C(X,C) EXPORT int create_##X(OBJECT **obj, OBJECT *parent) \
{	try { *obj = gl_create_object(C::oclass); \
	if ( *obj != NULL ) { C *my = OBJECTDATA(*obj,C); \
		gl_set_parent(*obj,parent); return my->create(); \
	} else return 0; } CREATE_CATCHALL(X); }
/// Implement class create export
#define EXPORT_CREATE(X) EXPORT_CREATE_C(X,X)

#define EXPORT_INIT_C(X,C) EXPORT int init_##X(OBJECT *obj, OBJECT *parent) \
{	try { if (obj!=NULL) return OBJECTDATA(obj,C)->init(parent); else return 0; } \
	INIT_CATCHALL(X); }
/// Implement class init export
#define EXPORT_INIT(X) EXPORT_INIT_C(X,X)

#define EXPORT_COMMIT_C(X,C) EXPORT TIMESTAMP commit_##X(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2) \
{	C *my = OBJECTDATA(obj,C); try { return obj!=NULL ? my->commit(t1,t2) : TS_NEVER; } \
	T_CATCHALL(C,commit); }
/// Implement class commit export
#define EXPORT_COMMIT(X) EXPORT_COMMIT_C(X,X)

#define EXPORT_NOTIFY_C(X,C) EXPORT int notify_##X(OBJECT *obj, int notice, PROPERTY *prop, char *value) \
{	C *my = OBJECTDATA(obj,C); try { if ( obj!=NULL ) { \
	switch (notice) { \
	case NM_POSTUPDATE: return my->postnotify(prop,value); \
	case NM_PREUPDATE: return my->prenotify(prop,value); \
	default: return 0; } } else return 0; } \
	T_CATCHALL(X,commit); return 1; }
/// Implement class notify export
#define EXPORT_NOTIFY(X) EXPORT_NOTIFY_C(X,X)

#define EXPORT_SYNC_C(X,C) EXPORT TIMESTAMP sync_##X(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass) { \
	try { TIMESTAMP t1=TS_NEVER; C *p=OBJECTDATA(obj,C); \
	switch (pass) { \
	case PC_PRETOPDOWN: t1 = p->presync(t0); break; \
	case PC_BOTTOMUP: t1 = p->sync(t0); break; \
	case PC_POSTTOPDOWN: t1 = p->postsync(t0); break; \
	default: throw "invalid pass request"; break; } \
	if ( (obj->oclass->passconfig&(PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN)&(~pass)) <= pass ) obj->clock = t0; \
	return t1; } \
	SYNC_CATCHALL(X); }
/// Implement class sync export
#define EXPORT_SYNC(X) EXPORT_SYNC_C(X,X)

#define EXPORT_ISA_C(X,C) EXPORT int isa_##X(OBJECT *obj, char *name) { \
	return ( obj!=0 && name!=0 ) ? OBJECTDATA(obj,C)->isa(name) : 0; }
/// Implement class isa export
#define EXPORT_ISA(X) EXPORT_ISA_C(X,X)

#define EXPORT_PLC_C(X,C) EXPORT TIMESTAMP plc_##X(OBJECT *obj, TIMESTAMP t1) { \
	try { return OBJECTDATA(obj,C)->plc(t1); } \
	T_CATCHALL(plc,X); }
/// Implement class plc export
#define EXPORT_PLC(X) EXPORT_PLC_C(X,X)

// TODO add other linkages as needed
#define EXPORT_PRECOMMIT_C(X,C) EXPORT int precommit_##X(OBJECT *obj, TIMESTAMP t1) \
{	C *my = OBJECTDATA(obj,C); try { return obj!=NULL ? my->precommit(t1) : 0; } \
	T_CATCHALL(C,precommit); }
/// Implement class precommit export
#define EXPORT_PRECOMMIT(X) EXPORT_PRECOMMIT_C(X,X)

#define EXPORT_FINALIZE_C(X,C) EXPORT int finalize_##X(OBJECT *obj) \
{	C *my = OBJECTDATA(obj,C); try { return obj!=NULL ? my->finalize() : 0; } \
	T_CATCHALL(C,finalize); }
/// Implement class finalize export
#define EXPORT_FINALIZE(X) EXPORT_FINALIZE_C(X,X)

#define EXPORT_NOTIFY_C_P(X,C,P) EXPORT int notify_##X_##P(OBJECT *obj, char *value) \
{	C *my = OBJECTDATA(obj,C); try { if ( obj!=NULL ) { \
	return my->notify_##P(value); \
	} else return 0; } \
	T_CATCHALL(X,notify_##P); return 1; }
/// Implement property notify export
#define EXPORT_NOTIFY_PROP(X,P) EXPORT_NOTIFY_C_P(X,X,P)

#endif

/****************************************
 * GENERAL SOLVERS 
 ****************************************/
#ifdef USE_GLSOLVERS

#if defined WIN32 && ! defined MINGW
	#define _WIN32_WINNT 0x0400
	#undef int64 // wtypes.h also used int64
	#include <windows.h>
	#define int64 _int64
	#define PREFIX ""
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
	#define PREFIX ""
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

class glsolver {
public:
	int (*init)(void*);
	int (*solve)(void*);
	int (*set)(char*,...);
	int (*get)(char*,...);
private:
	inline void exception(char *fmt,...)
	{
		static char buffer[1024]="";
		va_list ptr;
		va_start(ptr,fmt);
		int len = vsprintf(buffer,fmt,ptr);
		va_end(ptr);
		if ( errno!=0 )
			sprintf(buffer+len," (%s)", strerror(errno));
		throw (const char*)buffer;
	};
public:
	inline glsolver(char *name, char *lib="glsolvers" DLEXT)
	{
		char path[1024];
		errno = 0;
		if ( callback->file.find_file(lib,NULL,X_OK,path,sizeof(path))!=NULL )
		{
			errno = 0;
			void* handle = DLLOAD(path);
			if ( handle==NULL )
				exception("glsolver(char *name='%s'): load of '%s' failed",name,path);
			else
			{
				char fname[64];
				struct {
					char *part;
					void **func;
				} map[] = {
					{"init", (void**)&init},
					{"solve", (void**)&solve},
					{"set", (void**)&set},
					{"get", (void**)&get},
				};
				int n;
				for ( n=0 ; n<sizeof(map)/sizeof(map[0]) ; n++ )
				{
					strcpy(fname,name);
					strcat(fname,"_");
					strcat(fname,map[n].part);
					errno = 0;
					*(map[n].func) = (void*)DLSYM(handle,fname);
					if ( *(map[n].func)==NULL )
						exception("glsolver(char *name='%s'): function '%s' not found in '%s'",name,fname,path);
				}
				errno = 0;
				if ( !(*init)(callback) )
					exception("glsolver(char *name='%s'): init failed",name);
			}
		}
		else
			exception("glsolver(char *name='%s'): solver library '%s' not found", name, lib);
	};
};

#endif // __cplusplus

/** @} **/
#endif

