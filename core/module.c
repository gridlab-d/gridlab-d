/** $Id: module.c 1212 2009-01-17 01:42:30Z d3m998 $
	Copyright (C) 2008 Battelle Memorial Institute
	@file module.cpp
	@addtogroup modules Runtime modules

	Runtime modules are implemented as dynamic libraries that
	are loaded as needed.  The model loaded determines whether
	a runtime module is needed by specifying a \p module block.

	Adding a module in Windows can be done using the "Add GridLAB module"
	wizard.  In Linux it can be done using the "add_gridlab_module" script.
	This document is provided for completeness and to provide details that
	may be necessary should the scripts not function as required.

	@todo Write the Add module wizard (Win32) and script (Linux) (ticket #47)

	*************************************************************************
	@addtogroup module_design Designing a GridLAB module

	The exact approach to use in building a GridLAB module is not clear cut.
	In general a module is a solver that can compute the steady state of
	a collection of objects given a specific boundary condition.  For example,
	a power flow solver makes sense as a module because the steady state
	of a flow network can be directly computed.  However, a market clearing
	system and a load simulation doesn't make sense because the market is
	influenced not only by demand from loads, but also by supply.  As a general
	rule, if a set of simultaneous equations can be solved to obtain the state
	of a system, the system is suitable for implementation as a module.

	Modules must be able to implement a least three capabilities:
	<ol>
	<li> they must be able to create objects on demand (see create)

	<li> they must be able to initialize objects on demand (see init)

	<li> they must be able to compute the state of individual objects
	at a specified date and time on demand (see sync)
	</ol>

	In addition, modules generally should be able to implement the following

	- compute the states of objects with some degree of parallelism

	- load and save data in various exchange formats

	- inform the GridLAB core which objects data members are exposed to
	other modules

	- handle notification events of data members about to be changed or just changed

	- determine whether an object is a subtype of another object

	- verify that a collection of objects form a self-consistent and correct model

	It turns out that implementing these capabilities is not as easy as it
	at first seems.  In particular, the synchronization has typically been one
	of the most challenging concepts for programmers to understand.  Given
	the amount of time spend in sync calls, it is recommended that
	considerable time and effort be put into its design.

	<b>Basic Synchronization</b>

	An object's sync method actually performs two essential functions. First,
	it updates the state of an object to a designated point in time, and second
	it lets the core know when the object is next expected to change state. This
	is vital for the core to know because the core's clock will be advanced
	to the time of the next expected state change, and all objects will be
	synchronized to that time.

	In general a sync() function should be aware of three times:

	- \f$ t_0 \f$ is the time from which the object is being advanced.  This
	is not the current time, because it is presumed that the object has not
	yet advanced to the current time and this is why sync() is being called.

	- \f$ t_1 \f$ is the time to which the object is being advanced.  Think
	of this as \e now from the object's perspective. This is usually the
	current time from the core's perspective (but don't assume it \e always is).

	- \f$ t_2 \f$ is the time at which the object expects to have its next
	state change.  This time must be computed by the object during or immediately
	after the state is advanced to \f$ t_1 \f$.  This is the time returned to the
	core should the sync() call succeed.

	If no state change is ever anticipated
	then \f$ t_2 = \f$ #TS_NEVER is returned, indicating that barring any changes to its
	boundary condition, the object is in steady state.

	If an object's sync() method determines that
	the object is not yet in steady state (i.e., the module has not converged), then
	\f$ t_2 = t_1 \f$ is returned.

	If an object's sync() method determines that it cannot update to \f$ t_1 \f$ as required,
	the simulation has failed. It can either throw an exception using GL_THROW() or
	return \f$ t_2 < t_1 \f$ to indicate the time at which the problem is believed to have occurred.

	The time window \f$ t_1 - t_0 \f$ is the past window and the sync() method
	must implement all behaviors during that time as though they have already occurred.

	The time window \f$ t_2 - t_1 \f$ is the future window and the sync function
	must not implement behaviors in this window yet, as they have not yet occurred.

	It is a non-trivial fact that if all objects in all modules in GridLAB model
	return \f$ t_2 = \f$ #TS_NEVER, then the simulation stops successfully.  This
	is because the system has completely settle to a steady state and there is
	nothing further to compute.

	<b>Control Code</b>

	One very important aspect of synchronization behavior is how control code
	is handled when object behavior goes beyond the mere physics of its
	response to its boundary condition.  It is quite easy to implement
	control code that is integrated with the physical model.  However, this
	would prevent users from altering the control code without altering the
	source code of the object's implementation.

	To address this problem, objects can implement default control code that
	is disabled if a \ref plc "PLC object" is attached later.  The ability to alter
	control code should be made available when implemented for any object for which
	this is a realistic possibility, which is very nearly always.

	To implement default \ref machine "PLC code" for an object, the module must expose a
	plc() method that will be called immediately before sync() is called, but
	only if not external PLC method is provided in the model.  This plc() method
	may be written as though it was integrated with the physics implemented in sync(),
	but the physics must be able to update even when the default PLC code is not run.

	*************************************************************************
	@addtogroup module_build Building a GridLAB module

	A GridLAB module must be a Windows DLL or a Linux SO.
	The compile options that are typically required are
	- Include path: ../core
	- Runtime library: Multi-threaded Debug DLL (/MDd)

	For example the command-line options used by the climate module is
	\verbatim
	compiler: /Od /I "../core" /I "../test/cppunit2/include"
		/D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL"
		/D "_CRT_SECURE_NO_DEPRECATE" /D "_WINDLL" /D "_MBCS"
		/Gm /EHsc /RTC1 /MDd /Fo"Win32\Debug\climate\"
		/Fd"Win32\Debug\climate\vc80.pdb" /W3 /nologo /c /Wp64
		/ZI /TP /errorReport:prompt

	linker: /OUT:"Win32\Debug\climate.dll" /INCREMENTAL /NOLOGO
	/LIBPATH:"..\test\cppunit2\lib" /DLL /MANIFEST
	/MANIFESTFILE:"Win32\Debug\climate\climate.dll.intermediate.manifest"
	/DEBUG /PDB:"c:\projects\GridlabD\source\VS2005\Win32\Debug\climate.pdb"
	/SUBSYSTEM:WINDOWS /MACHINE:X86 /ERRORREPORT:PROMPT cppunitd.lib
	kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib
	advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib
	\endverbatim

	The \p main.cpp file contains the code to load and unload both Windows DLL
	and Linux shared-object libraries:
	\code
	// the version can be used to make sure the right module is loaded
	#define MAJOR 0 // TODO: set the major version of your module here
	#define MINOR 0 // TODO: set the minor version of your module here

	#define DLMAIN
	#include <stdlib.h>
	#include "gridlabd.h"
	EXPORT int do_kill(void*);
	EXPORT int major=MAJOR, minor=MINOR;

	#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
	#include <windows.h>
	BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID)
	{
		switch (ul_reason_for_call)
		{
			case DLL_PROCESS_ATTACH:
			case DLL_THREAD_ATTACH:
				break;
			case DLL_THREAD_DETACH:
			case DLL_PROCESS_DETACH:
				do_kill(hModule);
				break;
		}
		return TRUE;
	}
	#else // !WIN32
	CDECL int dllinit() __attribute__((constructor));
	CDECL int dllkill() __attribute__((destructor));
	CDECL int dllinit() { return 0;}
	CDECL int dllkill() { do_kill(NULL);}
	#endif // !WIN32
	\endcode

	The \p init.cpp file contains the code needed to initialize a module
	after it is loaded:
	\code
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>
	#include "gridlabd.h"
	#include "myclass.h"
	EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
	{
		// set the GridLAB core API callback table
		callback = fntable;

		// TODO: register each object class by creating its first instance
		new myclass(module);

		// always return the first class registered
		return myclass::oclass;
	}
	CDECL int do_kill()
	{
		// if anything needs to be deleted or freed, this is a good time to do it
		return 0;
	}
	\endcode

	You can also implement \code EXPORT int check(void) \endcode to allow the core
	to request a check of the objects that are implemented by the module.  This is
	particularly useful to perform topology checks for network models.

	If you implement the \code EXPORT int import_file(char *file) \endcode this will
	permit users to use the \p import command in the model definition.

	If you implement the \code EXPORT int save(char *file) \endcode this will permit
	users to request saves in formats other than \p .glm or \p .xml.

	The \code EXPORT int setvar(char *varname, char *value) \endcode and the
	\code EXPORT void* getvar(char *varname, char *value, unsigned int size) \endcode
	are implemented when you wish to allow the user to alter any of the module's global
	variables.  See \p network/varmap.c for an example.

	The \code EXPORT int module_test(TEST_CALLBACKS *callbacks,int argc, char* argv[]) \endcode
	function implements the module's unit test code.  See "Unit testing" for
	more information.

	The \code EXPORT MODULE *subload(char *modname, MODULE **pMod, CLASS **cptr, int argc, char **argv) \endcode
	function is used to load modules written in foreign languages.  Look at the gldjava project
	for examples of how this is used.  This function needs to manually set the function pointers
	for any classes registered by subload-ed modules.  A module subload is triggered with
	"module X::Y".

	The implementation of each class will require two files for each object class
	be included in your module's source code.  The header file will usually include
	the following:
	\code
	#ifndef _MYCLASS_H
	#define _MYCLASS_H
	#include "gridlabd.h"
	class myclass {
	public:
		// TODO: add your published variables here using GridLAB types (see PROPERTY)
		double myDouble; // just an example
	private:
		// TODO: add any unpublished variables here (any type is ok)
		double *pDouble; // just an example
	public:
		static CLASS *oclass;
		static myclass *defaults;
	public:
		myclass(MODULE *module);
		int create(void);
		int init(OBJECT *parent);
		TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	};
	#endif
	\endcode

	The implementation file should include the following:
	\code
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>
	#include "myclass.h"
	CLASS *myclass::oclass = NULL;
	myclass *myclass::defaults = NULL;
	myclass::myclass(MODULE *module)
	{
		if (oclass==NULL)
		{
			oclass = gl_register_class(module,"myclass",PC_BOTTOMUP);
			if (gl_publish_variable(oclass,
				// TODO: publish your variables here
				PT_double, "my_double", PADDR(myDouble), // just an example
				NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			defaults = this;
			// TODO: initialize the default values that apply to all objects of this class
			myDouble = 1.23; // just an example
			pDouble = NULL; // just an example
		}
	}
	void myclass::create(void)
	{
		memcpy(this,defaults,sizeof(*this));
		// TODO: initialize the defaults values that do not depend on relatioships with other objects
	}
	int myclass::init(OBJECT *parent)
	{
		// find the data in the parent object
		myclass *pMyClass = OBJECTDATA(parent,myclass); // just an example

		// TODO: initialize the default values that depend on relationships with other objects
		pDouble = &(pMyClass->double); // just an example

		// return 1 on success, 0 on failure
		return 1;
	}
	TIMESTAMP myclass::sync(TIMESTAMP t0, TIMESTAMP t1)
	{
		// TODO: update the state of the object
		myDouble = myDouble*1.01; // just an example

		// TODO: compute time to next state change
		return (TIMESTAMP)(t1 + myDouble/TS_SECOND); // just an example
	}

	//////////////////////////////////////////////////////////////////////////
	// IMPLEMENTATION OF CORE LINKAGE
	//////////////////////////////////////////////////////////////////////////

	/// Create a myclass object
	EXPORT int create_myclass(OBJECT **obj, ///< a pointer to the OBJECT*
							  OBJECT *parent) ///< a pointer to the parent OBJECT
	{
		*obj = gl_create_object(myclass::oclass,sizeof(myclass));
		if (*obj!=NULL)
		{
			myclass *my = OBJECTDATA(*obj,myclass);
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		return 0;
	}

	/// Initialize the object
	EXPORT int init_myclass(OBJECT *obj, ///< a pointer to the OBJECT
							OBJECT *parent) ///< a pointer to the parent OBJECT
	{
		if (obj!=NULL)
		{
			myclass *my = OBJECTDATA(obj,myclass);
			return my->init(parent);
		}
		return 0;
	}

	/// Synchronize the object
	EXPORT TIMESTAMP sync_myclass(OBJECT *obj, ///< a pointer to the OBJECT
								  TIMESTAMP t1) ///< the time to which the OBJECT's clock should advance
	{
		TIMESTAMP t2 = OBJECTDATA(obj,myclass)->sync(obj->clock,t1);
		obj->clock = t1;
		return t2;
	}
	\endcode

	There are a number of useful extended capabilities that can be added.  These include

	\code int notify_myclass(OBJECT *obj, NOTIFYMODULE msg) \endcode
	can be implemented to receive notification messages before and after a variable is changed by
	the core (NM_PREUPDATE / NM_POSTUPDATE) and when the core needs the module to reset (NM_RESET)

	\code int isa_myclass(OBJECT *obj, char *classname) \endcode
	can be implemented to allow the core to discover whether an object is a subtype of
	another class.

	\code int plc_myclass(OBJECT *obj, TIMESTAMP t1) \endcode
	can be implemented to create default PLC code that can be overridden by attaching a
	child plc object.

	\code EXPORT int recalc_myclass(OBJECT *obj) \endcode
	can be implemented to create a recalculation triggered based on changes to properties made
	through object_set_value_by_addr() and related functions.  A property can be made to
	trigger recalculation calls by adding PT_FLAGS, PF_RECALC after the property publish specification, e.g.,
	\code (gl_publish_variable(oclass, PT_double, "resistance[Ohm]", PADDR(resistance), PT_FLAGS, PF_RECALC, NULL); \endcode
	will cause recalc() to be called after resistance is changed.  The recalc calls occur right before the PLC code 
	is run during sync() events.

	A Linux GridLAB module must be a shared object library.

 @{
 **/

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <math.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* The following hack is required to stringize LIBEXT as passed in from
 * Makefile and used by snprintf below to construct the library name. */
#define _STR(x) #x
#define STR(x) _STR(x)

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
	#define _WIN32_WINNT 0x0400
	#include <windows.h>
	#define LIBPREFIX
	#ifndef LIBEXT
		#define LIBEXT .dll
	#endif
	#define DLLOAD(P) LoadLibrary(P)
	#define DLSYM(H,S) GetProcAddress((HINSTANCE)H,S)
	#define snprintf _snprintf
#else /* ANSI */
	#include "dlfcn.h"
	#define LIBPREFIX "lib"
	#ifndef LIBEXT
		#define LIBEXT .so
	#else
	#endif
	#define DLLOAD(P) dlopen(P,RTLD_LAZY)
	#define DLSYM(H,S) dlsym(H,S)
#endif

#if !defined(HAVE_CONFIG_H) || defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif

#include <errno.h>
#include "globals.h"
#include "output.h"
#include "module.h"
#include "find.h"
#include "random.h"
#include "test_callbacks.h"
#include "exception.h"
#include "unit.h"
#include "lock.h"

#include "matlab.h"

int get_exe_path(char *buf, int len, void *mod){	/* void for GetModuleFileName, a windows func */
	int rv = 0, i = 0;
	if(buf == NULL)
		return 0;
	if(len < 1)
		return 0;
#ifdef WIN32
	rv = GetModuleFileName((HMODULE) mod, buf, len);
	if(rv){
		for(i = rv; ((buf[i] != '/') && (buf[i] != '\\') && (i >= 0)); --i){
			buf[i] = 0;
			--rv;
		}
	}
#else /* POSIX */
	if(mod == NULL){ /* "/bin/gridlabd"?*/
		;
	} else {
		;
	}
#endif
	return rv;
}

int module_get_exe_path(char *buf, int len){
	return get_exe_path(buf, len, NULL);
}

int module_get_path(char *buf, int len, MODULE *mod){
	return get_exe_path(buf, len, mod->hLib);
}

void dlload_error(const char *filename)
{
#ifdef WIN32
	LPTSTR error;
	LPTSTR end;
	DWORD result = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &error, 0, NULL);
	if (!result)
		error = TEXT("[FormatMessage failed]");
	else for (end = error + strlen(error) - 1; end >= error && isspace(*end); end--)
		*end = 0;
#else
	char *error = dlerror();
#endif
	output_debug("%s: %s (LD_LIBRARY_PATH=%s)", filename, error,getenv("LD_LIBRARY_PATH"));
#ifdef WIN32
	if (result)
		LocalFree(error);
#endif
}

/* these are the core functions available to loadable modules
 * the structure is defined in object.h */
int64 lock_count;
int64 lock_spin;
static CALLBACKS callbacks = {
	&global_clock,
	output_verbose,
	output_message,
	output_warning,
	output_error,
	output_debug,
	output_test,
	class_register,
	{object_create_single,object_create_array,object_create_foreign},
	class_define_map,
	class_get_class_from_classname,
	class_get_class_from_objecttype,
	{class_define_function,class_get_function},
	class_define_enumeration_member,
	class_define_set_member,
	object_set_dependent,
	object_set_parent,
	object_set_rank,
	{object_get_property, object_set_value_by_addr,object_get_value_by_addr, object_set_value_by_name,object_get_value_by_name,object_get_reference,object_get_unit,object_get_addr},
	{find_objects,find_next,findlist_copy,findlist_add,findlist_del,findlist_clear},
	malloc,
	free,
	aggregate_mkgroup,
	aggregate_value,
	module_getvar_addr,
	module_depends,
	{random_uniform, random_normal, random_bernoulli, random_pareto, random_lognormal, random_sampled, random_exponential, random_type, random_value, pseudorandom_value},
	object_isa,
	class_register_type,
	class_define_type,
	{mkdatetime,strdatetime,timestamp_to_days,timestamp_to_hours,timestamp_to_minutes,timestamp_to_seconds,local_datetime,convert_to_timestamp},
	unit_convert, unit_convert_ex, unit_find,
	{create_exception_handler,delete_exception_handler,throw_exception,exception_msg},
	{global_create, global_setvar, global_getvar, global_find},
#ifndef NOLOCKS
	&lock_count, &lock_spin,
#endif
	{find_file},
	{object_get_complex, object_get_int16, object_get_int32, object_get_int64, object_get_double, object_get_string, object_get_object},
	{object_get_complex_by_name, object_get_int16_by_name, object_get_int32_by_name, object_get_int64_by_name,
		object_get_double_by_name, object_get_string_by_name},
	{class_string_to_property, class_property_to_string,},
	module_find,
	object_find_name,
	object_get_oflags,
	object_get_count,
};

MODULE *first_module = NULL;
MODULE *last_module = NULL;

/** Load a runtime module
	@return a pointer to the MODULE structure
	\p NULL on failure, errno set to:
    - \p ENOEXEC to indicate init() not defined in module
    - \p EINVAL to indicate call to init failed
    - \p ENOENT to indicate class not defined by module
 **/
typedef MODULE *(*LOADER)(const char *, int, char *[]);
MODULE *module_load(const char *file, /**< module filename, searches \p PATH */
							   int argc, /**< count of arguments in \p argv */
							   char *argv[]) /**< arguments passed from the command line */
{
	/* check for already loaded */
	MODULE *mod = module_find((char *)file);
	char buffer[FILENAME_MAX+1];
	char *fmod;
	bool isforeign = false;
	char pathname[1024];
	char *tpath = NULL;
#ifdef WIN32
	char from='/', to='\\';
#else
	char from='\\', to='/';
#endif
	char *p = NULL;
	void *hLib = NULL;
	LIBINIT init = NULL;
	int *pMajor = NULL, *pMinor = NULL;
	CLASS *previous = NULL;
	CLASS *c;

	if (mod!=NULL)
	{
		output_verbose("%s(%d): module '%s' already loaded", __FILE__, __LINE__, file);
		return mod;
	}
	else
	{
		output_verbose("%s(%d): module '%s' not yet loaded", __FILE__, __LINE__, file);
	}

	/* check for foreign modules */
	strcpy(buffer,file);
	fmod = strtok(buffer,"::");
	if (fmod!=NULL && strcmp(fmod, file) != 0)
	{
		char *modname = strtok(NULL,"::");
		MODULE *parent_mod = module_find(fmod);
		if(parent_mod == NULL)
			parent_mod = module_load(fmod, 0, NULL);
		previous = class_get_last_class();
		if(parent_mod != NULL && parent_mod->subload != NULL)
		{	/* if we've defined a subload routine and already loaded the parent module*/
			MODULE *child_mod;
			if(module_find(fmod) == NULL)
				module_load(fmod, 0, NULL);
			child_mod = parent_mod->subload(modname, &mod, (previous ? &(previous->next) : &previous), argc, argv);
			if(child_mod == NULL)
			{	/* failure */
				output_error("module_load(file='%s::%s'): subload failed", fmod, modname);
				return NULL;
			}
			if (mod != NULL)
			{	/* if we want to register another module */
				last_module->next = mod;
				last_module = mod;
				mod->oclass = previous ? previous->next : class_get_first_class();
			}
			return last_module;
		} else {
			struct {
				char *name;
				LOADER loader;
			} fmap[] = {
				{"matlab",NULL},
				{"java",load_java_module},
				{"python",load_python_module},
				{NULL,NULL} /* DO NOT DELETE THIS TERMINATOR ENTRY */
			}, *p;
			for (p=fmap; p->name!=NULL; p++)
			{
				if (strcmp(p->name, fmod)==0)
				{
					static char *args[1];
					isforeign = true;
					if (p->loader!=NULL)
						/* use external loader */
						return p->loader(modname,argc,argv);

					/* use a module with command args */
					argv = args;
					argc=1;
					argv[0] = modname;
					file=buffer;
					break;
				}
			}
			if (p==NULL)
			{
				output_error("module_load(file='%s',...): foreign module type %s not recognized or supported", fmod);
				return NULL;
			}
		}
	}

	/* create a new module entry */
	mod = (MODULE *)malloc(sizeof(MODULE));
	if (mod==NULL)
	{
		output_verbose("%s(%d): module '%s' memory allocation failed", __FILE__, __LINE__, file);
		errno=ENOMEM;
		return NULL;
	}
	else
		output_verbose("%s(%d): module '%s' memory allocated", __FILE__, __LINE__, file);

	/* locate the module */
	snprintf(pathname, 1024, LIBPREFIX "%s" STR(LIBEXT), file);
	tpath = find_file(pathname, NULL, X_OK|R_OK);
	if(tpath == NULL) 
	{
		output_verbose("unable to locate %s in GLPATH, using library loader instead", pathname);
		tpath=pathname;
	}
	else
		output_verbose("full path to library '%s' is '%s'", file, tpath);

	/* convert path delims based on OS preference */
	for (p=strchr(tpath,from); p!=NULL; p=strchr(p,from))
		*p=to;

	/* ok, let's do it */
	hLib = DLLOAD(tpath);
	if (hLib==NULL)
	{
#ifdef WIN32
		output_verbose("%s(%d): module '%s' load failed - %s (error code %d)", __FILE__, __LINE__, file, strerror(errno), GetLastError());
#else
		output_verbose("%s(%d): module '%s' load failed - %s", __FILE__, __LINE__, file, dlerror());
#endif
		dlload_error(pathname);
		errno = ENOENT;
		free(mod);
		return NULL;
	}
	else
		output_verbose("%s(%d): module '%s' loaded ok", __FILE__, __LINE__, file);

	/* get the initialization function */
	init = (LIBINIT)DLSYM(hLib,"init");
	if (init==NULL)
	{
		output_error("%s(%d): module '%s' does not export init()", __FILE__, __LINE__, file);
		dlload_error(pathname);
		errno = ENOEXEC;
		free(mod);
		return NULL;
	}
	else
		output_verbose("%s(%d): module '%s' exports init()", __FILE__, __LINE__, file);

	/* connect the module's exported data & functions */
	mod->hLib = (void*)hLib;
	pMajor = (int*)DLSYM(hLib, "major");
	pMinor = (int*)DLSYM(hLib, "minor");
	mod->major = pMajor?*pMajor:0;
	mod->minor = pMinor?*pMinor:0;
	mod->import_file = (int(*)(char*))DLSYM(hLib,"import_file");
	mod->export_file = (int(*)(char*))DLSYM(hLib,"export_file");
	mod->setvar = (int(*)(char*,char*))DLSYM(hLib,"setvar");
	mod->getvar = (void*(*)(char*,char*,unsigned int))DLSYM(hLib,"getvar");
	mod->check = (int(*)())DLSYM(hLib,"check");
#ifndef _NO_CPPUNIT
	mod->module_test = (int(*)(TEST_CALLBACKS*,int,char*[]))DLSYM(hLib,"module_test");
#endif
	mod->cmdargs = (int(*)(int,char**))DLSYM(hLib,"cmdargs");
	mod->kmldump = (int(*)(FILE*,OBJECT*))DLSYM(hLib,"kmldump");
	mod->subload = (MODULE *(*)(char *, MODULE **, CLASS **, int, char **))DLSYM(hLib, "subload");
	mod->test = (void(*)(int,char*[]))DLSYM(hLib,"test");
	mod->globals = NULL;
	strcpy(mod->name,file);
	mod->next = NULL;

	/* call the initialization function */
	mod->oclass = (*init)(&callbacks,(void*)mod,argc,argv);

	/* connect intrinsic functions */
	for (c=mod->oclass; c!=NULL; c=c->next) {
		char fname[1024];
		struct {
			FUNCTIONADDR *func;
			char *name;
			int optional;
		} map[] = {
			{&c->create,"create",FALSE},
			{&c->init,"init",TRUE},
			{&c->sync,"sync",TRUE},
			{&c->notify,"notify",TRUE},
			{&c->isa,"isa",TRUE},
			{&c->plc,"plc",TRUE},
			{&c->recalc,"recalc",TRUE},
		};
		int i;
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
		{
			snprintf(fname, 1024,"%s_%s",map[i].name,isforeign?fmod:c->name);
			if ((*(map[i].func) = (FUNCTIONADDR)DLSYM(hLib,fname))==NULL && !map[i].optional)
			{
				output_fatal("intrinsic %s is not defined in class %s", fname,file);
				errno=EINVAL;
				return NULL;
			}
			else
				if(!map[i].optional)
					output_verbose("%s(%d): module '%s' intrinsic %s found", __FILE__, __LINE__, file, fname);
		}
	}

	/* attach to list of known modules */
	if (first_module==NULL)
		first_module = mod;
	else
		last_module->next = mod;
	last_module = mod;
	return last_module;
}

int module_setvar(MODULE *mod, char *varname, char *value)
{
	char modvarname[1024];
	sprintf(modvarname,"%s::%s",mod->name,varname);
	return global_setvar(modvarname,value)==SUCCESS;
}

void* module_getvar(MODULE *mod, char *varname, char *value, unsigned int size)
{
	char modvarname[1024];
	sprintf(modvarname,"%s::%s",mod->name,varname);
	return global_getvar(modvarname,value,size);
}

void* module_getvar_old(MODULE *mod, char *varname, char *value, unsigned int size)
{
	if (mod->getvar!=NULL)
	{
		if (strcmp(varname,"major")==0)
		{
			sprintf(value,"%d",mod->major);
			return value;
		}
		else if (strcmp(varname,"minor")==0)
		{
			sprintf(value,"%d",mod->minor);
			return value;
		}
		else
			return (*mod->getvar)(varname,value,size);
	}
	else
		return 0;
}

double* module_getvar_addr(MODULE *mod, char *varname)
{
	char modvarname[1024];
	GLOBALVAR *var;
	sprintf(modvarname,"%s::%s",mod->name,varname);
	var = global_find(modvarname);
	if (var!=NULL)
		return var->prop->addr;
	else
		return NULL;
}

int module_saveall(FILE *fp)
{
	MODULE *mod;
	int count=0;
	CLASS *oclass = NULL;
	char32 varname;
	count += fprintf(fp,"\n########################################################\n");
	count += fprintf(fp,"# modules\n");
	for (mod=first_module; mod!=NULL; mod=mod->next)
	{
		varname[0] = '\0';
		oclass = NULL;

		count += fprintf(fp,"module %s {\n",mod->name);
		if (mod->major>0 || mod->minor>0)
			count += fprintf(fp,"\tmajor %d;\n\tminor %d;\n",mod->major,mod->minor);
		for (oclass=mod->oclass; oclass!=NULL ; oclass=oclass->next)
		{
			if (oclass->module==mod)
				count += fprintf(fp,"\tclass %s;\n",oclass->name);
		}

		while (module_getvar(mod,varname,NULL,0))
		{
			char32 value;
			if (module_getvar(mod,varname,value,sizeof(value)))
				count += fprintf(fp,"\t%s %s;\n",varname,value);
		}
		count += fprintf(fp,"}\n");
	}
	return count;
}

int module_saveall_xml(FILE *fp){
	MODULE *mod;
	int count = 0;
	char32 varname = "";
	char32 value = "";
	GLOBALVAR *gvptr = NULL;
	char1024 buffer;

	for (mod = first_module; mod != NULL; mod = mod->next){
		char tname[67];
		size_t tlen;
		gvptr = global_getnext(NULL);
		sprintf(tname, "%s::", mod->name);
		tlen = strlen(tname);
		count += fprintf(fp, "\t<module type=\"%s\" ", mod->name);
		if (mod->major > 0){
			count += fprintf(fp, "major=\"%d\" minor=\"%d\">\n", mod->major, mod->minor);
		} else {
			count += fprintf(fp, ">\n");
		}
		count += fprintf(fp, "\t\t<properties>\n");
		while(gvptr != NULL){
			if(strncmp(tname, gvptr->name, tlen) == 0){
				count += fprintf(fp, "\t\t\t<%s>%s</%s>\n", gvptr->name+tlen, class_property_to_string(gvptr->prop,(void*)gvptr->prop->addr,buffer,1024)>0 ? buffer : "...", gvptr->name+tlen);
			} // else we have a module::prop name
			gvptr = global_getnext(gvptr);
		}
		count += fprintf(fp, "\t\t</properties>\n");
		module_saveobj_xml(fp, mod);	/* insert objects w/in module tag */
		count += fprintf(fp,"\t</module>\n");
	}
	return count;
}

#ifdef WIN32
#define isnan _isnan  /* map isnan to appropriate function under Windows */
#endif

int module_saveobj_xml(FILE *fp, MODULE *mod){ /**< the stream to write to */
	unsigned count = 0;
	char buffer[1024];
	PROPERTY *prop = NULL;
	OBJECT *obj;
	CLASS *oclass=NULL;
	CLASS *pclass = NULL;

	for(obj = object_get_first(); obj != NULL; obj = obj->next){
		char32 oname = "(unidentified)";
		if(obj->oclass->module != mod){
			continue;
		}
		
		if(obj->name != NULL){
			strcpy(oname, obj->name);
		} else {
			sprintf(oname, "%s:%i", obj->oclass->name, obj->id);
		}
		if ((oclass == NULL) || (obj->oclass->type != oclass->type))
			oclass = obj->oclass;
		count += fprintf(fp,"\t\t<object type=\"%s\" id=\"%i\" name=\"%s\">\n", obj->oclass->name, obj->id, oname);

		/* dump internal properties */
		if (obj->parent!=NULL){
			if(obj->parent->name != NULL){
				strcpy(oname, obj->parent->name);
			} else {
				sprintf(oname, "%s:%i", obj->parent->oclass->name, obj->parent->id);
			}
			count += fprintf(fp,"\t\t\t<parent>%s</parent>\n", oname);
		} else {
			count += fprintf(fp,"\t\t\t<parent>root</parent>\n");
		}
		count += fprintf(fp,"\t\t\t<rank>%d</rank>\n", obj->rank);
		count += fprintf(fp,"\t\t\t<clock>\n", obj->clock);
		count += fprintf(fp,"\t\t\t\t <timestamp>%s</timestamp>\n", convert_from_timestamp(obj->clock,buffer,sizeof(buffer))>0?buffer:"(invalid)");
		count += fprintf(fp,"\t\t\t</clock>\n");
		/* why do latitude/longitude have 2 values?  I currently only store as float in the schema... -dc */
		if (!isnan(obj->latitude))
			count += fprintf(fp,"\t\t\t<latitude>%s</latitude>\n", convert_from_latitude(obj->latitude,buffer,sizeof(buffer))?buffer:"(invalid)");
		else
			count += fprintf(fp, "\t\t\t<latitude>NONE</latitude>\n");
		if (!isnan(obj->longitude))
			count += fprintf(fp,"\t\t\t<longitude>%s</longitude>\n",convert_from_longitude(obj->longitude,buffer,sizeof(buffer))?buffer:"(invalid)");
		else
			count += fprintf(fp,"\t\t\t<longitude>NONE</longitude>\n");

		/* dump properties */
		for (prop=oclass->pmap;prop!=NULL && prop->otype==oclass->type;prop=prop->next)
		{
			char *value = NULL;
			if((prop->access != PA_PUBLIC) && (prop->access != PA_REFERENCE))
				continue;
			value = object_property_to_string(obj,prop->name);
			if (value!=NULL){
				count += fprintf(fp, "\t\t\t<%s>%s</%s>\n", prop->name, value, prop->name);
			}
		}
		pclass = oclass->parent;
		while(pclass != NULL){ /* inherited properties */
			for (prop=pclass->pmap;prop!=NULL && prop->otype==pclass->type;prop=prop->next){
				char *value = object_property_to_string(obj,prop->name);
				if (value!=NULL){
					count += fprintf(fp, "\t\t\t<%s>%s</%s>\n", prop->name, value, prop->name);
				}
			}
			pclass = pclass->parent;
		}
		count += fprintf(fp,"\t\t</object>\n");
	}
	return count;
}

MODULE *module_get_first(void)
{
	return first_module;
}

int module_saveall_xml_old(FILE *fp);

int module_saveall_xml_old(FILE *fp)
{
	MODULE *mod;
	int count=0;
	count += fprintf(fp,"\t<modules>\n");
	for (mod=first_module; mod!=NULL; mod=mod->next)
	{
		CLASS *oclass;
		char32 varname="";
		count += fprintf(fp,"\t\t<module> \n");
		count += fprintf(fp,"\t\t\t<name>%s</name>\n",mod->name);
		if(mod->major > 0)
			count += fprintf(fp,"\t\t\t<major>%d</major>\n",mod->major );
		if(mod->minor > 0)
			count += fprintf(fp,"\t\t\t<minor>%d</minor>\n",mod->minor);
		count += fprintf(fp,"\t\t\t<classes>\n");
		for (oclass=mod->oclass; oclass!=NULL ; oclass=oclass->next)
		{
			if (oclass->module==mod){
				count += fprintf(fp, "\t\t\t\t<class> \n");
				count += fprintf(fp, "\t\t\t\t\t<classname>%s</classname>\n", oclass->name);
				count += fprintf(fp, "\t\t\t\t\t<module name=\"%s\" />\n", mod->name);
				count += fprintf(fp, "\t\t\t\t</class>\n");
			}
		}
		count += fprintf(fp,"\t\t\t</classes>\n");
		count += fprintf(fp,"\t\t\t<properties>\n");
		while (module_getvar(mod,varname,NULL,0))
		{
			char32 value;
			if (module_getvar(mod,varname,value,sizeof(value)))
			{	/* TODO: support other types (ticket #46) */
				count += fprintf(fp,"\t\t\t\t<property> \n");
				count += fprintf(fp,"\t\t\t\t\t <type>double</type>\n", varname);
				count += fprintf(fp,"\t\t\t\t\t <name>%s</name>\n", value);
				count += fprintf(fp,"\t\t\t\t</property> \n");
			}
		}
		count += fprintf(fp,"\t\t\t</properties>\n");
		count += fprintf(fp,"\t\t</module>\n");
	}
	count += fprintf(fp,"\t</modules>\n");
	return count;
}

MODULE *module_find(char *modname)
{
	MODULE *mod = NULL;
	for (mod=first_module; mod!=NULL; mod=mod->next)
	{
		if (strcmp(mod->name,modname)==0)
			break;
	}
	return mod;

}

int module_import(MODULE *mod, char *filename)
{
	if (mod->import_file == NULL)
	{
		errno = ENOENT;
		return 0;
	}
	return (*mod->import_file)(filename);
}

int module_save(MODULE *mod, char *filename)
{
	if (mod->export_file == NULL)
	{
		errno = ENOENT;
		return 0;
	}
	return (*mod->export_file)(filename);
}

int module_dumpall(void)
{
	MODULE *mod;
	int count=0;
	for (mod=first_module; mod!=NULL; mod=mod->next)
	{
		if (mod->export_file!=NULL)
			count += module_save(mod,NULL);
	}
	return count;
}

int module_checkall(void)
{
	MODULE *mod;
	int count=0;
	for (mod=first_module; mod!=NULL; mod=mod->next)
			count += module_check(mod);
	return count;
}

int module_check(MODULE *mod)
{
	if (mod->check==NULL)
		return 0;
	return (*mod->check)();
}

void module_libinfo(char *module_name)
{
	MODULE *mod = module_load(module_name,0,NULL);
	if (mod!=NULL)
	{
		CLASS *c;
		PROPERTY *p;
		output_raw("Module name....... %s\n", mod->name);
		output_raw("Major version..... %d\n", mod->major);
		output_raw("Minor version..... %d\n", mod->minor);
		output_raw("Classes........... ");
		for (c=mod->oclass; c!=NULL; c=c->next)
			output_raw("%s%s", c->name, c->next!=NULL?", ":"");
		output_raw("\n");
		output_raw("Implementations... ");
		if (mod->cmdargs!=NULL) output_raw("cmdargs ");
		if (mod->getvar!=NULL) output_raw("getvar ");
		if (mod->setvar!=NULL) output_raw("setvar ");
		if (mod->import_file!=NULL) output_raw("import_file ");
		if (mod->export_file!=NULL) output_raw("export_file ");
		if (mod->check!=NULL) output_raw("check ");
		if (mod->kmldump!=NULL) output_raw("kmldump ");
#ifndef _NO_CPPUNIT
		if (mod->module_test!=NULL) output_raw("module_test ");
#endif
		output_raw("\nGlobals........... ");
		for (p=mod->globals; p!=NULL; p=p->next)
			output_raw("%s ", p->name);
		output_raw("\n");
	}
	else
		output_error("Module %s load failed", module_name);
}

int module_cmdargs(int argc, char **argv)
{
	MODULE *mod;
	for (mod=first_module; mod!=NULL; mod=mod->next)
	{
		if (mod!=NULL && mod->cmdargs!=NULL)
			return (*(mod->cmdargs))(argc,argv);
	}
	return 0;
}

int module_depends(char *name, unsigned char major, unsigned char minor, unsigned short build)
{
	MODULE *mod;
	for (mod=first_module; mod!=NULL; mod=mod->next)
	{
		if (strcmp(mod->name,name)==0)
			if(mod->major > 0)
				if(mod->major==major && mod->minor>=minor)
					return 1;
	}
	return 0;
}


/**@}*/
