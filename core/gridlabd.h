/** $Id: gridlabd.h 1207 2009-01-12 22:47:29Z d3p988 $
	Copyright (C) 2008 Battelle Memorial Institute
	@file gridlabd.h
	@author David P. Chassin
	@addtogroup module_api Runtime module API
	@brief The GridLAB-D external module header file

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

	@todo Many of the module API macros would be better implemented as inline functions (ticket #9)
 @{
 **/

#ifndef _GRIDLABD_H
#define _GRIDLABD_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32
#define HAVE_LIBCPPUNIT
#endif

#ifdef __cplusplus
	#ifndef CDECL
		#define CDECL extern "C"
	#endif
#else
	#define CDECL
#endif

#ifdef WIN32
#ifndef EXPORT
#define EXPORT CDECL __declspec(dllexport)
#endif
#else
#define EXPORT CDECL
#endif

#include "schedule.h"
#include "object.h"
#include "find.h"
#include "random.h"

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
#define PUBLISH_STRUCT(C,T,N) {struct C _t;if (gl_publish_variable(C##_class,PT_##T,#N,(char*)&_t.N-(char*)&_t,NULL)<1) return NULL;}
/** The PUBLISH_CLASS macro is used to publish a member of a class (C++ only).
 **/
#define PUBLISH_CLASS(C,T,N) {class C _t;if (gl_publish_variable(C##_class,PT_##T,#N,(char*)&_t.N-(char*)&_t,NULL)<1) return NULL;}
/** The PUBLISH_CLASSX macro is used to publish a member of a class (C++ only) using a different name from the member name.
 **/
#define PUBLISH_CLASSX(C,T,N,V) {class C _t;if (gl_publish_variable(C##_class,PT_##T,V,(char*)&_t.N-(char*)&_t,NULL)<1) return NULL;}

/** The PUBLISH_CLASS_UNT macros is used to publish a member of a class (C++ only) including a unit specification.
**/
//#define PUBLISH_CLASS_UNIT(C,T,N,U) {class C _t;if (gl_publish_variable(C##_class,PT_##T,#N"["U"]",(char*)&_t.N-(char*)&_t,NULL)<1) return NULL;}
/** The PUBLISH_DELEGATED macro is used to publish a variable that uses a delegated type.

**/
#define PUBLISH_DELEGATED(C,T,N) {class C _t;if (gl_publish_variable(C##_class,PT_delegated,T,#N,(char*)&_t.N-(char*)&_t,NULL)<1) return NULL;}

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
#define gl_get_module_var (*callback->get_module_var)

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
	return (*callback->depends)(name,major,minor,build);
}
#else
#define gl_module_depends (*callback->depends)
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
{ return (*callback->set_dependent)(obj,dep);}
#else
#define gl_set_dependent (*callback->set_dependent)
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
{ return (*callback->set_parent)(obj,parent);}
#else
#define gl_set_parent (*callback->set_parent)
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
{ return (*callback->set_rank)(obj,rank);}
#else
#define gl_set_rank (*callback->set_rank)
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
								 PROPERTYNAME name) /**< the name of the property */
{ return (*callback->properties.get_property)(obj,name); }
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
						 PROPERTYNAME name)
{
	static char buffer[1024];
	memset(buffer,0,sizeof(buffer));
	return gl_get_value_by_name(obj,name,buffer,sizeof(buffer))>=0 ? buffer : NULL;
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
#define gl_create_aggregate (*callback->create_aggregate)

/** Evaluate an aggregate property
	@see aggregate_value()
 **/
#define gl_run_aggregate (*callback->run_aggregate)
/** @} **/

/******************************************************************************
 * Random number generation
 */
/**	@defgroup gridlabd_h_random Random numbers

	The random number library provides a variety of random number generations
	for different distributions.

	@{
 **/

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

/** Convert a string to a timestamp
	@see convert_to_timestamp()
 **/
#define gl_parsetime (*callback->time.convert_to_timestamp)

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
/**@}*/

#define gl_get_oflags (*callback->get_oflags)

#ifdef __cplusplus
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

inline SCHEDULE *gl_schedule_find(char *name)
{
	return callback->schedule.find(name);
}

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

inline long gl_schedule_dtnext(SCHEDULE *sch, SCHEDULEINDEX index)
{
	return callback->schedule.dtnext(sch,index);
}

inline int gl_enduse_create(enduse *e)
{
	return callback->enduse.create(e);
}

inline TIMESTAMP gl_enduse_sync(enduse *e, TIMESTAMP t1)
{
	return callback->enduse.sync(e,PC_BOTTOMUP,t1);
}

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

inline double gl_get_loadshape_value(loadshape *shape)
{
	if (shape)
		return shape->load;
	else
		return 0;
}

inline char *gl_strftime(DATETIME *dt, char *buffer, int size) { return callback->time.strdatetime(dt,buffer,size)?buffer:NULL;};
inline char *gl_strftime(TIMESTAMP ts)
{
	static char buffer[64];
	strcpy(buffer,"(invalid time)");
	DATETIME dt;
	if(gl_localtime(ts,&dt)){
		gl_strftime(&dt,buffer,sizeof(buffer));
	}
	return buffer;
}
#endif //__cplusplus


/**@}*/
/******************************************************************************
 * Interpolation routines
 */
/** @defgroup gridlabd_h_interpolation Interpolation routines
 @{
 **/

/** Linearly interpolate a value between two points

 **/
#define gl_lerp (*callback->interpolate.linear)
/**@}*/

/** Quadratically interpolate a value between two points

 **/
#define gl_qerp (*callback->interpolate.quadratic)
/**@}*/


/** @} **/
#endif
