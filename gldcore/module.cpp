/** $Id: module.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file module.cpp
	@addtogroup modules Runtime modules

 @{
 **/

/* absolutely nothing must be placed before this per feature_test_macros(7) man page */
#ifndef WIN32
#ifndef __APPLE__
#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <features.h>
#endif
#endif

#include "version.h"

#if defined WIN32
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#endif
#include <cmath>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "http_client.h"

#if defined(_WIN32) && !defined(__MINGW32__)
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0400
#include <windows.h>
#ifndef DLEXT
#define DLEXT ".dll"
#endif
#define DLLOAD(P) LoadLibrary(P)
#define DLSYM(H,S) (void *)GetProcAddress((HINSTANCE)H,S)
#define snprintf _snprintf
#else /* ANSI */
#include "dlfcn.h"
#ifndef DLEXT
#ifdef __MINGW32__
#define DLEXT ".dll"
#else
#define DLEXT ".so"
#endif
#endif
#define DLLOAD(P) dlopen(P,RTLD_LAZY)
#define DLSYM(H,S) dlsym(H,S)
#endif

#if !defined(HAVE_CONFIG_H) || defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif

#if HAVE_SCHED_H
#include <sched.h>
#endif

#include <cerrno>

#include "platform.h"
#include "globals.h"
#include "output.h"
#include "module.h"
#include "find.h"
#include "gldrandom.h"
#include "test_callbacks.h"
#include "exception.h"
#include "unit.h"
#include "interpolate.h"
#include "lock.h"
#include "schedule.h"
#include "exec.h"
#include "stream.h"
#include "transform.h"

#include "console.h"

#include "matlab.h"

int get_exe_path(char *buf, int len, void *mod){	/* void for GetModuleFileName, a windows func */
	int rv = 0, i = 0;
	if(buf == nullptr)
		return 0;
	if(len < 1)
		return 0;
#if defined WIN32 && ! defined __MINGW32__
	rv = GetModuleFileName((HMODULE) mod, buf, len);
	if(rv){
		for(i = rv; ((buf[i] != '/') && (buf[i] != '\\') && (i >= 0)); --i){
			buf[i] = 0;
			--rv;
		}
	}
#else /* POSIX */
	if(mod == nullptr){ /* "/bin/gridlabd"?*/
		;
	} else {
		;
	}
#endif
	return rv;
}

int module_get_exe_path(char *buf, int len){
	return get_exe_path(buf, len, nullptr);
}

int module_get_path(char *buf, int len, MODULE *mod){
	return get_exe_path(buf, len, mod->hLib);
}

void dlload_error(const char *filename)
{
#ifndef __MINGW32__
#if defined WIN32
	LPTSTR error;
	LPTSTR end;
	DWORD result = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &error, 0, nullptr);
	if (!result)
		error = TEXT("[FormatMessage failed]");
	else for (end = error + strlen(error) - 1; end >= error && isspace(*end); end--)
		*end = 0;
#else
	char *error = dlerror();
#endif
#else
	const char *error = "unknown error";
#endif
	output_debug("%s: %s (LD_LIBRARY_PATH=%s)", filename, error,getenv("LD_LIBRARY_PATH"));
#if defined WIN32 && ! defined __MINGW32__
	if (result)
		LocalFree(error);
#endif
}

/* MALLOC/FREE - GL threadsafe versions */
static unsigned int malloc_lock = 0;

void *module_malloc(size_t size)
{
	void *ptr;
	wlock(&malloc_lock);
	ptr = (void*)malloc(size);
	wunlock(&malloc_lock);
	return ptr;
}
void module_free(void *ptr)
{
	wlock(&malloc_lock);
	free(ptr);
	wunlock(&malloc_lock);
}

/* these are the core functions available to loadable modules
 * the structure is defined in object.h */
#define MAGIC 0x012BB0B9
int64 lock_count;
int64 lock_spin;
s_callbacks::s_callbacks() throw() {
    global_clock = &::global_clock;
    global_delta_curr_clock = &::global_delta_curr_clock;
    global_exit_code = &::global_exit_code;
    global_stoptime = &::global_stoptime;
    output_verbose = ::output_verbose;
    output_message = ::output_message;
    output_warning = ::output_warning;
    output_error = ::output_error;
    output_fatal = ::output_fatal;
    output_debug = ::output_debug;
    output_test = ::output_test;
    register_class = class_register;
    create.single = object_create_single;
    create.array = object_create_array;
    create.foreign = object_create_foreign;
    define_map = class_define_map;
    loadmethod = class_add_loadmethod;
    class_getfirst = class_get_first_class;
    class_getname = class_get_class_from_classname;
    class_add_extended_property = ::class_add_extended_property;
    function.define = class_define_function;
    function.get = class_get_function;
    define_enumeration_member = class_define_enumeration_member;
    define_set_member = class_define_set_member;
    object.get_first = object_get_first;
    object.set_dependent = object_set_dependent;
    object.set_parent = object_set_parent;
    object.set_rank = object_set_rank;
    properties.get_property = object_get_property;
    properties.set_value_by_addr = object_set_value_by_addr;
    properties.get_value_by_addr = object_get_value_by_addr;
    properties.set_value_by_name = object_set_value_by_name;
    properties.get_value_by_name = object_get_value_by_name;
    properties.get_reference = object_get_reference;
    properties.get_unit = object_get_unit;
    properties.get_addr = object_get_addr;
    properties.set_value_by_type = class_string_to_propertytype;
    properties.compare_basic = property_compare_basic;
    properties.get_compare_op = property_compare_op;
    properties.get_part = property_get_part;
    properties.get_spec = property_getspec;
    find.objects = find_objects;
    find.next = find_next;
    find.copy = findlist_copy;
    find.add = findlist_add;
    find.del = findlist_del;
    find.clear = findlist_clear;
    find_property = class_find_property;
    malloc = module_malloc;
    free = module_free;
    aggregate.create = aggregate_mkgroup;
    aggregate.refresh = aggregate_value;
    module.getvar = module_getvar_addr;
    module.getfirst = module_get_first;
    module.depends = module_depends;
    module.find_transform_function = module_find_transform_function;
    random.uniform = random_uniform;
    random.normal = random_normal;
    random.bernoulli = random_bernoulli;
    random.pareto = random_pareto;
    random.lognormal = random_lognormal;
    random.sampled = random_sampled;
    random.exponential = random_exponential;
    random.type = random_type;
    random.value = random_value;
    random.pseudo = pseudorandom_value;
    random.triangle = random_triangle;
    random.beta = random_beta;
    random.gamma = random_gamma;
    random.weibull = random_weibull;
    random.rayleigh = random_rayleigh;
    object_isa = ::object_isa;
    register_type = class_register_type;
    define_type = class_define_type;
    time.mkdatetime = mkdatetime;
    time.strdatetime = strdatetime;
    time.timestamp_to_days = timestamp_to_days;
    time.timestamp_to_hours = timestamp_to_hours;
    time.timestamp_to_minutes = timestamp_to_minutes;
    time.timestamp_to_seconds = timestamp_to_seconds;
    time.local_datetime = local_datetime;
    time.local_datetime_delta = local_datetime_delta;
    time.convert_to_timestamp = convert_to_timestamp;
    time.convert_to_timestamp_delta = convert_to_timestamp_delta;
    time.convert_from_timestamp = convert_from_timestamp;
    time.convert_from_deltatime_timestamp = convert_from_deltatime_timestamp;
    unit_convert = ::unit_convert;
    unit_convert_ex = ::unit_convert_ex;
    unit_find = ::unit_find;
    exception.create_exception_handler = create_exception_handler;
    exception.delete_exception_handler = delete_exception_handler;
    exception.throw_exception = throw_exception;
    exception.exception_msg = exception_msg;
    global.create = global_create;
    global.setvar = global_setvar;
    global.getvar = global_getvar;
    global.find = global_find;
    lock.read = rlock;
    lock.write = wlock;
    unlock.read = runlock;
    unlock.write = wunlock;
    file.find_file = find_file;
    objvar.bool_var = object_get_bool;
    objvar.complex_var = object_get_complex;
    objvar.enum_var = object_get_enum;
    objvar.set_var = object_get_set;
    objvar.int16_var = object_get_int16;
    objvar.int32_var = object_get_int32;
    objvar.int64_var = object_get_int64;
    objvar.double_var = object_get_double;
    objvar.string_var = object_get_string;
    objvar.object_var = object_get_object;
    objvarname.bool_var = object_get_bool_by_name;
    objvarname.complex_var = object_get_complex_by_name;
    objvarname.enum_var =  object_get_enum_by_name;
    objvarname.set_var =  object_get_set_by_name;
    objvarname.int16_var = object_get_int16_by_name;
    objvarname.int32_var = object_get_int32_by_name;
    objvarname.int64_var = object_get_int64_by_name;
    objvarname.double_var = object_get_double_by_name;
    objvarname.string_var = object_get_string_by_name;
    objvarname.object_var = object_get_object_by_name;
    convert.string_to_property = class_string_to_property;
    convert.property_to_string = class_property_to_string;
    module_find = ::module_find;
    get_object = object_find_name;
    object_find_by_id = ::object_find_by_id;
    name_object = object_build_name;
    get_oflags = object_get_oflags;
    object_count = object_get_count;
    schedule.create = schedule_create;
    schedule.index = schedule_index;
    schedule.value = schedule_value;
    schedule.dtnext = schedule_dtnext;
    schedule.find = schedule_find_byname;
    schedule.getfirst = schedule_getfirst;
    loadshape.create = loadshape_create;
    loadshape.init = loadshape_init;
    enduse.create = enduse_create;
    enduse.sync = enduse_sync;
    interpolate.linear = interpolate_linear;
    interpolate.quadratic = interpolate_quadratic;
    forecast.create = forecast_create;
    forecast.find = forecast_find;
    forecast.read = forecast_read;
    forecast.save = forecast_save;
    remote.readobj = object_remote_read;
    remote.writeobj = object_remote_write;
    remote.readvar = global_remote_read;
    remote.writevar = global_remote_write;
    objlist.create = objlist_create;
    objlist.search = objlist_search;
    objlist.destroy = objlist_destroy;
    objlist.add = objlist_add;
    objlist.del = objlist_del;
    objlist.size = objlist_size;
    objlist.get = objlist_get;
    objlist.apply = objlist_apply;
    geography.latitude.to_string = convert_from_latitude;
    geography.latitude.from_string = convert_to_latitude;
    geography.longitude.to_string = convert_from_longitude;
    geography.longitude.from_string = convert_to_longitude;
    http.read = http_read;
    http.free = http_delete_result;
    transform.getnext = transform_getnext;
    transform.add_linear = transform_add_linear;
    transform.add_external = transform_add_external;
    transform.apply = transform_apply;
    randomvar.getnext = randomvar_getnext;
    randomvar.getspec = randomvar_getspec;
    version.major = version_major;
    version.minor = version_minor;
    version.patch = version_patch;
    version.build = version_build;
    version.branch = version_branch;
    magic = MAGIC;
}

extern CALLBACKS* callbacks = new s_callbacks;

CALLBACKS *module_callbacks(void) { return callbacks; }


static MODULE *first_module = nullptr;
static MODULE *last_module = nullptr;
static size_t module_count = 0;
size_t module_getcount(void) { return module_count; }

/** Load a runtime module
	@return a pointer to the MODULE structure
	\p nullptr on failure, errno set to:
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
	char tpath[1024];
#ifdef _WIN32
	char from='/', to='\\';
#else
	char from='\\', to='/';
#endif
	char *p = nullptr;
	void *hLib = nullptr;
	LIBINIT init = nullptr;
	int *pMajor = nullptr, *pMinor = nullptr;
	CLASS *previous = nullptr;
	CLASS *c;

	if ( callbacks->magic != MAGIC )
	{
		output_fatal("callback function table alignment error (magic number position mismatch)");
		return nullptr;
	}
#ifdef NEVER /* this shouldn't ever be necessary but sometimes for debugging purposes it is helpful */
	/* if LD_LIBRARY_PATH is not set, default to current directory */
	if (getenv("LD_LIBRARY_PATH")==nullptr)
	{
		putenv("LD_LIBRARY_PATH=.");
		output_verbose("Setting default LD_LIBRARY_DEFAULT to current directory");
	}
#endif

	if (mod!=nullptr)
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
	fmod = strstr(buffer,"::");
	if (fmod!=nullptr && strcmp(fmod, file) != 0)
	{
		char *modname = strtok(nullptr,"::");
		MODULE *parent_mod = module_find(fmod);
		if(parent_mod == nullptr)
			parent_mod = module_load(fmod, 0, nullptr);
		previous = class_get_last_class();
		if(parent_mod != nullptr && parent_mod->subload != nullptr)
		{	/* if we've defined a subload routine and already loaded the parent module*/
			MODULE *child_mod;
			if(module_find(fmod) == nullptr)
				module_load(fmod, 0, nullptr);
			child_mod = parent_mod->subload(modname, &mod, (previous ? &(previous->next) : &previous), argc, argv);
			if(child_mod == nullptr)
			{	/* failure */
				output_error("module_load(file='%s::%s'): subload failed", fmod, modname);
				/* TROUBLESHOOT
				   A module is unable to load a submodule require for operation.
				   Check that the indicated submodule is installed and try again.
				 */
				return nullptr;
			}
			if (mod != nullptr)
			{	/* if we want to register another module */
				last_module->next = mod;
				last_module = mod;
				mod->oclass = previous ? previous->next : class_get_first_class();
			}
			return last_module;
		} else {
			struct {
				const char *name;
				LOADER loader;
			} fmap[] = {
				{"matlab",nullptr},
				{"java",load_java_module},
				{"python",load_python_module},
				{nullptr,nullptr} /* DO NOT DELETE THIS TERMINATOR ENTRY */
			}, *p;
			for (p=fmap; p->name!=nullptr; p++)
			{
				if (strcmp(p->name, fmod)==0)
				{
					static char *args[1];
					isforeign = true;
					if (p->loader!=nullptr)
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
			if (p==nullptr)
			{
				output_error("module_load(file='%s',...): foreign module type %s not recognized or supported", fmod);
				return nullptr;
			}
		}
	}

	/* create a new module entry */
	mod = (MODULE *)malloc(sizeof(MODULE));
	if (mod==nullptr)
	{
		output_verbose("%s(%d): module '%s' memory allocation failed", __FILE__, __LINE__, file);
		errno=ENOMEM;
		return nullptr;
	}
	else
		output_verbose("%s(%d): module '%s' memory allocated", __FILE__, __LINE__, file);

	/* locate the module */
	snprintf(pathname, sizeof(pathname), "%s" DLEXT, file);

	if(find_file(pathname, nullptr, X_OK|R_OK, tpath,sizeof(tpath)) == nullptr)
	{
		output_verbose("unable to locate %s in GLPATH, using library loader instead", pathname);
		strncpy(tpath,pathname,sizeof(tpath));
	}
	else
	{
#ifndef WIN32
		/* if the path is a relative path */
		struct stat buf;
		if (tpath[0]!='/' && stat(tpath,&buf)==0) 
		{
			char buffer[1024];

			/* add ./ to the beginning of the path */
			sprintf(buffer,"./%s", tpath);
			strcpy(tpath,buffer);
		}
#endif
		output_verbose("full path to library '%s' is '%s'", file, tpath);
	}

	/* convert path delims based on OS preference */
	for (p=strchr(tpath,from); p!=nullptr; p=strchr(p,from))
		*p=to;

	/* ok, let's do it */
	hLib = DLLOAD(tpath);
	if (hLib==nullptr)
	{
#if defined(_WIN32) && ! defined(__MINGW32__)
		if ( GetLastError()==193 ) /* invalid exe format -- happens when wrong version of MinGW is used */
		{
			output_error("module '%s' load failed - invalid DLL format",file);
			/* TROUBLESHOOT
			   GridLAB-D and MinGW are not compatible.  Most likely the 32-bit version of 
			   MinGW is installed on a 64-bit machine running the 64-bit version of GridLAB-D.
			   Try installing MinGW64 instead.
			 */
			errno = ENOEXEC;
		}
		else
		{
			output_error("%s(%d): module '%s' load failed - %s (error code %d)", __FILE__, __LINE__, file, strerror(errno), GetLastError());
			errno = ENOENT;
		}
#else
		output_error("%s(%d): module '%s' load failed - %s", __FILE__, __LINE__, file, dlerror());
		output_debug("%s(%d): path to module is '%s'", __FILE__, __LINE__, tpath);
#endif
		dlload_error(pathname);
		errno = ENOENT;
		free(mod);
		mod = nullptr;
		return nullptr;
	}
	else
	{
		output_verbose("%s(%d): module '%s' loaded ok", __FILE__, __LINE__, file);
	}

	/* get the initialization function */
	init = (LIBINIT)DLSYM(hLib,"init");
	if (init==nullptr)
	{
		output_error("%s(%d): module '%s' does not export init()", __FILE__, __LINE__, file);
		dlload_error(pathname);
		errno = ENOEXEC;
		free(mod);
		mod = nullptr;
		return nullptr;
	}
	else
	{
		output_verbose("%s(%d): module '%s' exports init()", __FILE__, __LINE__, file);
	}

	/* connect the module's exported data & functions */
	mod->hLib = (void*)hLib;
	pMajor = (int*)DLSYM(hLib, "gld_major");
	pMinor = (int*)DLSYM(hLib, "gld_minor");
	mod->major = pMajor?*pMajor:0;
	mod->minor = pMinor?*pMinor:0;
	mod->import_file = (int(*)(const char*))DLSYM(hLib,"import_file");
	mod->export_file = (int(*)(const char*))DLSYM(hLib,"export_file");
	mod->setvar = (int(*)(const char*,char*))DLSYM(hLib,"setvar");
	mod->getvar = (void*(*)(const char*,char*,unsigned int))DLSYM(hLib,"getvar");
	mod->check = (int(*)())DLSYM(hLib,"check");
	/* deltamode */
	mod->deltadesired = (unsigned long(*)(DELTAMODEFLAGS*))DLSYM(hLib,"deltamode_desired");
	mod->preupdate = (unsigned long(*)(void*,int64,unsigned int64))DLSYM(hLib,"preupdate");
	mod->interupdate = (SIMULATIONMODE(*)(void*,int64,unsigned int64,unsigned long, unsigned int))DLSYM(hLib,"interupdate");
	mod->deltaClockUpdate = (SIMULATIONMODE(*)(void *, double, unsigned long, SIMULATIONMODE))DLSYM(hLib,"deltaClockUpdate");
	mod->postupdate = (STATUS(*)(void*,int64,unsigned int64))DLSYM(hLib,"postupdate");
	/* clock  update */
	mod->clockupdate = (TIMESTAMP(*)(TIMESTAMP*))DLSYM(hLib,"clock_update");
	mod->cmdargs = (int(*)(int,char**))DLSYM(hLib,"cmdargs");
	mod->kmldump = (int(*)(int(*)(const char*,...),OBJECT*))DLSYM(hLib,"kmldump");
	mod->subload = (MODULE *(*)(char *, MODULE **, CLASS **, int, char **))DLSYM(hLib, "subload");
	mod->test = (void(*)(int,char*[]))DLSYM(hLib,"test");
	mod->stream = (STREAMCALL)DLSYM(hLib,"stream");
	mod->globals = nullptr;
	mod->term = (void(*)(void))DLSYM(hLib,"term");
	strcpy(mod->name,file);
	mod->next = nullptr;

	/* check the module version before trying to initialize */
	if ( mod->major!=REV_MAJOR || mod->minor!=REV_MINOR )
	{
		output_error("Module version %d.%d mismatch from core version %d.%d", mod->major, mod->minor, REV_MAJOR, REV_MINOR);
		return nullptr;
	}

	/* call the initialization function */
	errno = 0;
	mod->oclass = (*init)(callbacks,(void*)mod,argc,argv);
	if ( mod->oclass==nullptr && errno!=0 )
		return nullptr;

	/* connect intrinsic functions */
	for (c=mod->oclass; c!=nullptr; c=c->next) {
		char fname[1024];
		struct {
			FUNCTIONADDR *func;
			const char *name;
			int optional;
		} map[] = {
			{&c->create,"create",       false},
			{&c->init,"init",           true},
			{&c->precommit,"precommit", true},
			{&c->sync,"sync",           true},
			{&c->commit,"commit",       true},
			{&c->finalize,"finalize",   true},
			{&c->notify,"notify",       true},
			{&c->isa,"isa",             true},
			{&c->plc,"plc",             true},
			{&c->recalc,"recalc",       true},
			{&c->update,"update",       true},
			{&c->heartbeat,"heartbeat",true},
		};
		int i;
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
		{
			snprintf(fname, sizeof(fname) ,"%s_%s",map[i].name,isforeign?fmod:c->name);
			if ((*(map[i].func) = (FUNCTIONADDR)DLSYM(hLib,fname))==nullptr && !map[i].optional)
			{
				output_fatal("intrinsic %s is not defined in class %s", fname,file);
				/*	TROUBLESHOOT
					A required intrinsic function was not found.  Please review and modify the class definition.
				 */
				errno=EINVAL;
				return nullptr;
			}
			else
				if(!map[i].optional)
					output_verbose("%s(%d): module '%s' intrinsic %s found", __FILE__, __LINE__, file, fname);
		}
	}

	/* attach to list of known modules */
	if (first_module==nullptr)
	{
		mod->id = 0;
		first_module = mod;
	}
	else
	{
		last_module->next = mod;
		mod->id = last_module->id + 1;
	}
	last_module = mod;
	module_count++;

	/* register the module stream, if any */
	if ( mod->stream!=nullptr )
		stream_register(mod->stream);

	return last_module;
}

#ifdef _WIN32
#include <winnt.h>
static bool _checkimg(const char *fname)
{
	FILE *fh = fopen(fname,"r");
	if ( fh!=nullptr )
	{
		struct _IMAGE_DOS_HEADER dh;
		// access DLL image
		fclose(fh);
	}
	return true;
}
#endif

static void _module_list (char *path)
{
	struct stat info;
	static int count = 0;
	int gsrm = global_suppress_repeat_messages;
#ifdef _WIN32
	char search[1024];
	HANDLE hFind;
	WIN32_FIND_DATA sFind;
#else
	DIR *dir;
	struct dirent *ent;
#endif
	if(path == nullptr){
		return;
	}
	/* access directory */
	if ( stat(path,&info)!=0 )
		return;
	
	/* write header if necessary */
	if ( (count++%25) == 0 )
	{
		output_message("Module name              Version Location");
		output_message("------------------------ ------- ----------------------------------------");
	}

	/* open directory */
#ifdef _WIN32
	sprintf(search,"%s\\*.dll",path);
	hFind=FindFirstFile(search,&sFind);
	if ( hFind==INVALID_HANDLE_VALUE )
		return;
	do {
#else
	if ( (dir=opendir(path))==nullptr )
		return;
	while((ent = readdir(dir)) != nullptr) {
#endif

	/* iterate files list */
		char fname[1024];
		char *ext;
		void *hLib = nullptr;
		int *pMajor = nullptr, *pMinor = nullptr;
		LIBINIT init = nullptr;
		global_suppress_repeat_messages = 0;
#ifdef _WIN32
		strcpy(fname,sFind.cFileName);
		/* check image */
		if ( !_checkimg(fname) ) continue;
#else
		// TODO Posix version
		/* isolate so files only */
		strcpy(fname, ent->d_name);
		ext = strrchr(fname,'.');
		if ( ext==nullptr ) continue; /* no extension */
		if ( strcmp(ext,".so")!=0 ) continue; /* not the right extension */
#endif
		/* access DLL */
		hLib = DLLOAD(fname);
		if ( hLib==nullptr ) continue;
		if ( DLSYM(hLib,"init")==nullptr ) continue;
		pMajor = (int*)DLSYM(hLib, "major");
		pMinor = (int*)DLSYM(hLib, "minor");
		if ( pMajor==nullptr || pMinor==nullptr ) continue;

		/* TODO print info */
		output_message("%-24.24s %5d.%d %s", fname, *pMajor, *pMinor, path);
#ifdef _WIN32
		global_suppress_repeat_messages = gsrm;
	} while ( FindNextFile(hFind,&sFind) );
	FindClose(hFind);
#else
	}
	closedir(dir);
#endif
}

void module_list(void)
{
	char *glpath = getenv("GLPATH");
	char *gridlabd = getenv("GRIDLABD");
	char *tokPath = nullptr;
	char *tokPathPtr = nullptr;
#ifdef _WIN32
	const char *pathDelim = ";";
#else
	const char *pathDelim = ":";
#endif

	_module_list(global_workdir);
	_module_list(global_execdir);
	if(glpath != nullptr){
        char *glPath = static_cast<char *>(malloc(sizeof(char) * (unsigned) strlen(glpath)));
		strncpy(glPath, glpath, (unsigned)strlen(glpath));
		tokPath = strtok_r(glPath, pathDelim, &tokPathPtr);
		while (tokPath != nullptr){
			_module_list(tokPath);
			tokPath = strtok_r(nullptr, pathDelim, &tokPathPtr);
		}
		tokPathPtr = nullptr;
		free(glPath);
	}
	if(gridlabd != nullptr){
        char *gridLabD = static_cast<char *>(malloc(sizeof(char) * (unsigned) strlen(gridlabd)));
		strncpy(gridLabD, gridlabd, (unsigned)strlen(gridlabd));
		tokPath = strtok_r(gridLabD, pathDelim, &tokPathPtr);
		while (tokPath != nullptr){
			_module_list(tokPath);
			tokPath = strtok_r(nullptr, pathDelim, &tokPathPtr);
		}
		free(gridLabD);
	}
}
int module_setvar(MODULE *mod, const char *varname, char *value)
{
	char modvarname[2048];
	sprintf(modvarname,"%s::%s",mod->name,varname);
	return global_setvar(modvarname,value)==SUCCESS;
}

void* module_getvar(MODULE *mod, const char *varname, char *value, unsigned int size)
{
	char modvarname[2048];
	sprintf(modvarname,"%s::%s",mod->name,varname);
	return global_getvar(modvarname,value,size);
}

void* module_getvar_old(MODULE *mod, const char *varname, char *value, unsigned int size)
{
	if (mod->getvar!=nullptr)
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

double* module_getvar_addr(MODULE *mod, const char *varname)
{
	char modvarname[2048];
	GLOBALVAR *var;
	sprintf(modvarname,"%s::%s",mod->name,varname);
	var = global_find(modvarname);
	if (var!=nullptr)
        return static_cast<double *>(var->prop->addr);
	else
		return nullptr;
}

int module_saveall(FILE *fp)
{
	MODULE *mod;
	int count=0;
	CLASS *oclass = nullptr;
	char varname[1024];
	char buffer[1024];
	count += fprintf(fp,"\n////////////////////////////////////////////////////////\n");
	count += fprintf(fp,"// modules\n");
	for (mod=first_module; mod!=nullptr; mod=mod->next)
	{
		varname[0] = '\0';
		oclass = nullptr;

		count += fprintf(fp,"module %s {\n",mod->name);
		if (mod->major>0 || mod->minor>0)
			count += fprintf(fp,"\tmajor %d;\n\tminor %d;\n",mod->major,mod->minor);
		for (oclass=mod->oclass; oclass!=nullptr ; oclass=oclass->next)
		{
			if (oclass->module==mod)
				count += fprintf(fp,"\tclass %s;\n",oclass->name);
		}

		while (module_getvar(mod,varname,buffer,sizeof(buffer)))
		{
			count += fprintf(fp,"\t%s %s;\n",varname,buffer);
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
	GLOBALVAR *gvptr = nullptr;
	char1024 buffer;

	for (mod = first_module; mod != nullptr; mod = mod->next){
		char tname[2048];
		size_t tlen;
		gvptr = global_getnext(nullptr);
		sprintf(tname, "%s::", mod->name);
		tlen = strlen(tname);
		count += fprintf(fp, "\t<module type=\"%s\" ", mod->name);
		if (mod->major > 0){
			count += fprintf(fp, "major=\"%d\" minor=\"%d\">\n", mod->major, mod->minor);
		} else {
			count += fprintf(fp, ">\n");
		}
		count += fprintf(fp, "\t\t<properties>\n");
		while(gvptr != nullptr){
			if(strncmp(tname, gvptr->prop->name, tlen) == 0){
				count += fprintf(fp, "\t\t\t<%s>%s</%s>\n", gvptr->prop->name+tlen, class_property_to_string(gvptr->prop,(void*)gvptr->prop->addr,buffer.get_string(),1024)>0 ? buffer.get_string() : "...", gvptr->prop->name+tlen);
			} // else we have a module::prop name
			gvptr = global_getnext(gvptr);
		}
		count += fprintf(fp, "\t\t</properties>\n");
		module_saveobj_xml(fp, mod);	/* insert objects w/in module tag */
		count += fprintf(fp,"\t</module>\n");
	}
	return count;
}

#if defined WIN32 && ! defined __MINGW32__
#define isnan _isnan  /* map isnan to appropriate function under Windows */
#endif

int module_saveobj_xml(FILE *fp, MODULE *mod){ /**< the stream to write to */
	unsigned count = 0;
	char buffer[1024];
	PROPERTY *prop = nullptr;
	OBJECT *obj;
	CLASS *oclass=nullptr;
	CLASS *pclass = nullptr;

	for(obj = object_get_first(); obj != nullptr; obj = obj->next){
		char128 oname = "(unidentified)";
		if(obj->oclass->module != mod){
			continue;
		}

		if(obj->name != nullptr){
			strcpy(oname, obj->name);
		} else {
			sprintf(oname, "%s:%i", obj->oclass->name, obj->id);
		}
		if ((oclass == nullptr) || (obj->oclass != oclass))
			oclass = obj->oclass;
		count += fprintf(fp,"\t\t<object type=\"%s\" id=\"%i\" name=\"%s\">\n", obj->oclass->name, obj->id, oname.get_string());

		/* dump internal properties */
		if (obj->parent!=nullptr){
			if(obj->parent->name != nullptr){
				strcpy(oname, obj->parent->name);
			} else {
				sprintf(oname, "%s:%i", obj->parent->oclass->name, obj->parent->id);
			}
			count += fprintf(fp,"\t\t\t<parent>%s</parent>\n", oname.get_string());
		} else {
			count += fprintf(fp,"\t\t\t<parent>root</parent>\n");
		}
		count += fprintf(fp,"\t\t\t<rank>%d</rank>\n", obj->rank);
		count += fprintf(fp,"\t\t\t<clock>%lld\n", obj->clock); //TODO: Review if obj->clock is needed to be printed?
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
		for (prop=oclass->pmap;prop!=nullptr && prop->oclass==oclass;prop=prop->next)
		{
			char *value = nullptr;
			if((prop->access != PA_PUBLIC) && (prop->access != PA_REFERENCE))
				continue;
			value = object_property_to_string(obj,prop->name, buffer, 1023);
			if (value!=nullptr){
				count += fprintf(fp, "\t\t\t<%s>%s</%s>\n", prop->name, value, prop->name);
			}
		}
		pclass = oclass->parent;
		while(pclass != nullptr){ /* inherited properties */
			for (prop=pclass->pmap;prop!=nullptr && prop->oclass==pclass;prop=prop->next){
				char *value = object_property_to_string(obj,prop->name, buffer, 1023);
				if (value!=nullptr){
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
	for (mod=first_module; mod!=nullptr; mod=mod->next)
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
		for (oclass=mod->oclass; oclass!=nullptr ; oclass=oclass->next)
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
		while (module_getvar(mod,varname,nullptr,0))
		{
			char32 value;
			if (module_getvar(mod,varname,value,sizeof(value)))
			{	/* TODO: support other types (ticket #46) */
				count += fprintf(fp,"\t\t\t\t<property> \n");
				count += fprintf(fp,"\t\t\t\t\t <type>double</type>\n");//TODO: Is varname.get_string() to be printed? Currently hardcoded as double.
				count += fprintf(fp,"\t\t\t\t\t <name>%s</name>\n", value.get_string());
				count += fprintf(fp,"\t\t\t\t</property> \n");
			}
		}
		count += fprintf(fp,"\t\t\t</properties>\n");
		count += fprintf(fp,"\t\t</module>\n");
	}
	count += fprintf(fp,"\t</modules>\n");
	return count;
}

MODULE *module_find(const char *modname)
{
	MODULE *mod = nullptr;
	for (mod=first_module; mod!=nullptr; mod=mod->next)
	{
		if (strcmp(mod->name,modname)==0)
			break;
	}
	return mod;

}

int module_import(MODULE *mod, const char *filename)
{
	if (mod->import_file == nullptr)
	{
		errno = ENOENT;
		return 0;
	}
	return (*mod->import_file)(filename);
}

int module_export(MODULE *mod, const char *filename)
{
	if (mod->export_file == nullptr)
	{
		errno = ENOENT;
		return 0;
	}
	return (*mod->export_file)(filename);
}

int module_save(MODULE *mod, const char *filename)
{
	if (mod->export_file == nullptr)
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
	for (mod=first_module; mod!=nullptr; mod=mod->next)
	{
		if (mod->export_file!=nullptr)
			count += module_save(mod,nullptr);
	}
	return count;
}

int module_checkall(void)
{
	MODULE *mod;
	int count=0;
	for (mod=first_module; mod!=nullptr; mod=mod->next)
			count += module_check(mod);
	return count;
}

int module_check(MODULE *mod)
{
	if (mod->check==nullptr)
		return 0;
	return (*mod->check)();
}

void module_libinfo(const char *module_name)
{
	MODULE *mod = module_load(module_name,0,nullptr);
	if (mod!=nullptr)
	{
		CLASS *c;
		PROPERTY *p;
		GLOBALVAR *v=nullptr;
		output_raw("Module name....... %s\n", mod->name);
		output_raw("Major version..... %d\n", mod->major);
		output_raw("Minor version..... %d\n", mod->minor);
		output_raw("Classes........... ");
		for (c=mod->oclass; c!=nullptr; c=c->next)
			output_raw("%s%s", c->name, c->next!=nullptr?", ":"");
		output_raw("\n");
		output_raw("Implementations... ");
		if (mod->cmdargs!=nullptr) output_raw("cmdargs ");
		if (mod->getvar!=nullptr) output_raw("getvar ");
		if (mod->setvar!=nullptr) output_raw("setvar ");
		if (mod->import_file!=nullptr) output_raw("import_file ");
		if (mod->export_file!=nullptr) output_raw("export_file ");
		if (mod->check!=nullptr) output_raw("check ");
		if (mod->kmldump!=nullptr) output_raw("kmldump ");
		if (mod->stream!=nullptr) output_raw("stream ");
		output_raw("\nGlobals........... ");
		for (p=mod->globals; p!=nullptr; p=p->next)
			output_raw("%s ", p->name);
		while ((v=global_getnext(v))!=nullptr)
		{
			if (strncmp(v->prop->name,module_name,strlen(module_name))==0)
			{
				char *vn = strstr(v->prop->name,"::");
				if (vn!=nullptr)
					output_raw("%s ", vn+2);
			}
		}
		output_raw("\n");
	}
	else
		output_error("Module %s load failed", module_name);
}

int module_cmdargs(int argc, char **argv)
{
	MODULE *mod;
	for (mod=first_module; mod!=nullptr; mod=mod->next)
	{
		if (mod!=nullptr && mod->cmdargs!=nullptr)
			return (*(mod->cmdargs))(argc,argv);
	}
	return 0;
}

int module_depends(const char *name, unsigned char major, unsigned char minor, unsigned short build)
{
	MODULE *mod;
	for (mod=first_module; mod!=nullptr; mod=mod->next)
	{
		if (strcmp(mod->name,name)==0)
			if( major>0 && mod->major>0 )
				if( mod->major==major && mod->minor>=minor )
					return 1; // version matched
				else
					return 0; // version mismatched
			else
				return 1; // indifferent to version
	}
	return module_load(name,0,nullptr)!=nullptr;
}

MODULE *module_get_next(MODULE*module)
{
	return module->next;
}

void module_termall(void)
{
	MODULE *mod;
	for (mod=first_module; mod!=nullptr; mod=mod->next)
	{
		if ( mod->term ) mod->term();
	}
}


/***************************************************************************
 * EXTERNAL COMPILER SUPPORT
 ***************************************************************************/

#include <sys/stat.h>
#include <cctype>

#ifdef _WIN32
#ifdef X64
#define CC "gcc"
#define CCFLAGS "-DWIN32 -DX64"
#define LDFLAGS "" /* "--export-all-symbols,--add-stdcall,--add-stdcall-alias,--subsystem,windows,--enable-runtime-pseudo-reloc,-no-undefined" */
#else // !X64
#define CC "gcc"
#define CCFLAGS "-DWIN32"
#define LDFLAGS "" /* "--export-all-symbols,--add-stdcall,--add-stdcall-alias,--subsystem,windows,--enable-runtime-pseudo-reloc,-no-undefined" */
#endif // X64
#define fstat _fstat
#define stat _stat
#else // !WIN32
#define CC "/usr/bin/gcc"
#ifdef __APPLE__
#define CCFLAGS "-DMACOSX"
#define LDFLAGS "-dylib"
#else // !__APPLE__
#define CCFLAGS "-DLINUX -fPIC"
#define LDFLAGS "" /* --export-all-symbols" */
#endif // __APPLE__
#endif // WIN32

static int cc_verbose=0;
static int cc_debug=0;
static int cc_clean=0;
static int cc_keepwork=0;

/** Get file modify time
    @return modification time in seconds of epoch, 0 on missing file or fstat failure
 **/
static time_t file_modtime(char *file) /**< file name to query */
{
	FILE *fp = fopen(file,"r");
	if (fp)
	{
		struct stat status;
		if ( fstat(fileno(fp),&status)==0 )
			return status.st_mtime;
	}
	return 0;
}

/** Execute a command using formatted strings
    @return command return code
 **/
static int execf(const char *format, /**< format string  */
				 ...) /**< parameters  */
{
	char command[4096];
	int rc;
	va_list ptr;
	va_start(ptr,format);
	vsprintf(command,format,ptr); /* note the lack of check on buffer overrun */
	va_end(ptr);
	if (cc_verbose || global_verbose_mode ) output_message(command);
	else output_debug("command: %s",command);
	rc = system(command);
	output_debug("return code=%d",rc);
	return rc;
}

/** Compile C source code into a dynamic link library 
    @return 0 on success
 **/
int module_compile(const char *name,	/**< name of library */
				   const char *code,	/**< listing of source code */
				   int flags,	/**< compile options (see MC_?) */
				   const char *prefix, /**< file prefix */
				   const char *source,/**< source file (for context) */
				   int line)	/**< source line (for context) */
{
	char cfile[1024];
	char ofile[1024];
	char afile[1024];
    char *cc = const_cast<char *>(getenv("CC") ? getenv("CC") : CC);
    char *ccflags = const_cast<char *>(getenv("CCFLAGS") ? getenv("CCFLAGS") : CCFLAGS);
    char *ldflags = const_cast<char *>(getenv("LDFLAGS") ? getenv("LDFLAGS") : LDFLAGS);
	int rc;
	size_t codesize = strlen(code), len;
	FILE *fp;
	char srcfile[1024];
	char mopt[8] = "";
	const char *libs = "-lstdc++";
#ifdef _WIN32
	snprintf(mopt,sizeof(mopt),"-m%d",sizeof(void*)*8);
	libs = "";
#endif

	/* normalize source file name */
	if ( source )
	{
		char *c, *p=srcfile;
		strcpy(srcfile,source);
		for ( c=srcfile; *c!='\0'; c++ )
		{
			switch (*c) {
				case '\\': *p++='/'; break;
				default: *p++=*c; break;
			}
		}
		*c='\0';
		source = srcfile;
	}


	/* set flags */
	cc_verbose = (flags&MC_VERBOSE);
	cc_debug = (flags&MC_DEBUG);
	cc_clean = (flags&MC_CLEAN);
	cc_keepwork = (flags&MC_KEEPWORK);

	/* construct the file names */
	snprintf(cfile,sizeof(cfile),"%s.c",name);
	snprintf(ofile,sizeof(ofile),"%s.o",name);
	snprintf(afile,sizeof(afile),"%s" DLEXT,name);

	/* create the C source file */
	if ( (fp=fopen(cfile,"wt"))==nullptr)
	{
		output_error("unable to open '%s' for writing", cfile);
		return -1;
	}

	/* store prefix code */
	fprintf(fp,"/* automatically generated code\nSource: %s(%d)\n */\n%s\n",source,line,prefix?prefix:"");

	/* store file/line reference */
	if (source!=nullptr) fprintf(fp,"#line %d \"%s\"\n",line,source);

	/* write C source code */
	if ( (len=fwrite(code,1,codesize,fp))<codesize )
	{
		output_error("unable to write code to '%s' (%d of %d bytes written)", cfile, len, codesize);
		return -1;
	}

	/* C file done */
	fclose(fp);

	/* compile the code */
	if ( (rc=execf("%s %s %s -c \"%s\" -o \"%s\" ", cc, mopt, ccflags, cfile, ofile))!=0 )
		return rc;

	/* create needed DLL files on windows */
	if ( (rc=execf("%s %s %s%s -shared \"%s\" -o \"%s\"", cc, mopt, ((ldflags[0]==0)?"":"-Wl,"), ldflags, ofile,afile))!=0 )
		return rc;

#ifdef LINUX
	/* address SE textrel_shlib_t issue */
	if (global_getvar("control_textrel_shlib_t",tbuf,63)!=nullptr)
	{
		/* SE linux needs the new module marked as relocatable (textrel_shlib_t) */
		execf("chcon -t textrel_shlib_t '%s'", afile);
	}
#endif

	if ( !cc_keepwork )
	{
		unlink(cfile);
		unlink(ofile);
	}

	return 0;
}

/***************************************************************************
 * EXTERN SUPPORT
 ***************************************************************************/

typedef struct s_exfnmap {
	char *fname;
	char *libname;
	void *lib;
	void *call;
	struct s_exfnmap *next;
} EXTERNALFUNCTION;
EXTERNALFUNCTION *external_function_list = nullptr;

/* saves mapping - fctname will be stored in new malloc copy, libname must already be a copy in heap */
static int add_external_function(char *fctname, char *libname, void *lib)
{
	if ( module_get_transform_function(fctname)==nullptr )
	{
		int ordinal;
		char function[1024];
        EXTERNALFUNCTION *item = static_cast<EXTERNALFUNCTION *>(malloc(sizeof(EXTERNALFUNCTION)));
		if ( item==nullptr )
		{
			output_error("add_external_function(char *fn='%s',lib='%s',...): memory allocation failed", fctname, libname);
			return 0;
		}
        item->fname = static_cast<char *>(malloc(strlen(fctname) + 1));
		if ( item->fname==nullptr )
		{
			output_error("add_external_function(char *fn='%s',lib='%s',...): memory allocation failed", fctname, libname);
			return 0;
		}
		item->libname = libname;
		item->lib = lib;
		item->next = external_function_list;
		external_function_list = item;

		/* this is to address a frequent MinGW/MSVC incompatibility with DLLs */
		if ( sscanf(fctname,"%[^@]@%d",function,&ordinal)==2)
		{
#ifdef _WIN32
			item->call = DLSYM(lib,(LPCSTR)(long long)ordinal);
#else
			item->call = DLSYM(lib,function);
#endif
			strcpy(item->fname,function);
		}
		else
		{
			item->call = DLSYM(lib,fctname);
			strcpy(item->fname,fctname);
		}
		if ( item->call )
			output_debug("external function '%s' added from library '%s' (lib=%8x)", item->fname, libname, (int64)lib);
		else
			output_warning("external function '%s' not found in library '%s'", fctname, libname);
		return 1;
	}
	else
	{
		output_warning("external function '%s' is already defined", fctname);
		return 1;
	}
}

/* loads the DLL and maps the comma separate function list */
int module_load_function_list(char *libname, char *fnclist)
{
	char libpath[1024];
    char *static_name = static_cast<char *>(malloc(strlen(libname) + 1));
	void *lib;
	char *s, *e;
	
	if ( static_name==0 ) return 0; // malloc failed
	strcpy(static_name,libname); // use this copy to map functions

	/* load the library */
	if ( strchr(libname,'/')==nullptr )
		snprintf(libpath,sizeof(libpath),"./%s" DLEXT, libname);
	else
		snprintf(libpath,sizeof(libpath),"%s" DLEXT, libname);

	lib = DLLOAD(libpath);
#ifdef WIN32
	errno = GetLastError();
#endif
	if (lib==nullptr)
	{
#ifdef _WIN32
		LPTSTR error;
		LPTSTR end;
		DWORD result = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				nullptr, GetLastError(), 0,
				(LPTSTR) &error, 0, nullptr);
		if (!result)
			error = TEXT(const_cast<char*>("[FormatMessage failed]"));
		else for (end = error + strlen(error) - 1; end >= error && isspace(*end); end--)
			*end = 0;
#else
		char *error = dlerror();
#endif
		output_error("unable to load external library '%s': %s (errno=%d)", libpath, error, errno);
		return 0;
	}
	output_debug("loaded external function library '%s' ok",libname);

	/* map the functions */
	for ( s=fnclist; *s!='\0' ; s++ )
	{
		if ( !isspace(*s) && *s!=',' ) // start of a name
		{
			// span valid characters
			char c;
			for ( e=s; !isspace(*s) && *e!=',' && *e!='\0'; e++ );
			c = *e; *e = '\0';
			add_external_function(s,static_name,lib);
			s = e;
			if ( c=='\0' ) break;
		}
	}

	return 1; // ok
}

/* gets an external function from a module to use as a transform function */
TRANSFORMFUNCTION module_get_transform_function(const char *function)
{
	EXTERNALFUNCTION *item;
	for ( item=external_function_list; item!=nullptr ; item=item->next )
	{
		if ( strcmp(item->fname,function)==0 )
            return reinterpret_cast<TRANSFORMFUNCTION>(item->call);
	}
	errno = ENOENT;
	return nullptr;
}

const char *module_find_transform_function(TRANSFORMFUNCTION function)
{
	EXTERNALFUNCTION *item;
	for ( item=external_function_list; item!=nullptr ; item=item->next )
	{
        if (strcmp(static_cast<const char *>(item->call), reinterpret_cast<const char *>(function)) == 0)
			return item->fname;
	}
	errno = ENOENT;
	return nullptr;
}

#include "class.h"

void module_profiles(void)
{
	if ( global_mt_analysis>0 )
	{
		OBJECT *obj;
		unsigned int n_ranks = 0;
		struct s_rankdata {
			int64 t_presync, t_sync, t_postsync;
			int64 n_presync, n_sync, n_postsync;
			double total;
		} *rankdata;
		unsigned int n, r;

		output_profile("Multithreading analysis");
		output_profile("=======================\n");
		
		/* analysis assumes data was collected during a single threaded run */
		if ( global_threadcount>1 )
		{
			output_profile("thread count must be 1 to complete analysis");
			return;
		}

		/* determine number of ranks used */
		for ( obj=object_get_first(); obj!=nullptr ; obj=obj->next )
		{
			if ( n_ranks < obj->rank + 1 )
				n_ranks = obj->rank + 1;
		}
		n_ranks;

		/* allocate working buffers */
		rankdata = (struct s_rankdata*)malloc(n_ranks*sizeof(struct s_rankdata));
		memset(rankdata,0,n_ranks*sizeof(struct s_rankdata));

		/* gather rank data */
		for ( obj=object_get_first(); obj!=nullptr ; obj=obj->next )
		{
			struct s_rankdata *rank = &rankdata[obj->rank];
			if ( obj->oclass->passconfig&PC_PRETOPDOWN )
			{
				rank->t_presync += obj->synctime[0];
				rank->n_presync++;
			}
			if ( obj->oclass->passconfig&PC_BOTTOMUP )
			{
				rank->t_sync += obj->synctime[1];
				rank->n_sync++;
			}
			if ( obj->oclass->passconfig&PC_POSTTOPDOWN )
			{
				rank->t_postsync += obj->synctime[2];
				rank->n_postsync++;
			}
		}

		for ( n=1 ; n<=(unsigned int)global_mt_analysis ; n*=2 )
		{
			static double total1 = 0;
			double total = 0;
			for ( r=0 ; r<n_ranks ; r++ )
			{
				struct s_rankdata *rank = &rankdata[r];
				rank->total = rank->n_presync==0 ? 0 : (double)rank->t_presync/(double)global_ms_per_second/(double)rank->n_presync * (double)( rank->n_presync/n + rank->n_presync%n );
				rank->total += rank->n_sync==0 ? 0 : (double)rank->t_sync/(double)global_ms_per_second/(double)rank->n_sync * (double)( rank->n_sync/n + rank->n_sync%n );
				rank->total += rank->n_postsync==0 ? 0 : (double)rank->t_postsync/(double)global_ms_per_second/(double)rank->n_postsync * (double)( rank->n_postsync/n + rank->n_postsync%n );
				total += rank->total;
			}
			if ( n==1 ) 
			{
				total1 = total;
				output_profile("%2d thread model time    %.1f s (actual time)", n, total);
			}
			else
				output_profile("%2d thread model time    %.1f s (%+.0f%% est.)", n, total,(total-total1)/total1*100);
		}
		output_profile("");
	}
}

/***************************************************************************
 * THREAD SCHEDULER
 *
 * Note: This is added in module.c because module is the only core element that
 *       has the WINAPI.  Someday this should be split off into a separate file.
 *
 * This is an early prototype of processor/thread scheduling.  For now it
 * only supports a single-threaded run, and it allocates a single processor
 * exclusively to a single gridlabd run.
 *
 * @todo thread scheduling needs the following enhancements:
 *
 * 1. Support multithreaded operation, which means that available processors
 *    need to be assigned to threads, not processes.
 *
 * 2. Support waiting until processors become available instead of overloading.
 *
 ***************************************************************************/

#ifdef _WIN32
/* WIN32 requires use of the compatibility kill implementation */
#include "signal.h"
extern int kill(pid_t,int); /* defined in kill.c */
#else
#include <csignal>
#ifdef MACOSX
#include <mach/mach_init.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>

struct thread_affinity_policy policy;
#else /* linux */
#include <sched.h>
#endif

#endif

#include "gui.h"

static unsigned char procs[65536]; /* processor map */
static unsigned char n_procs=0; /* number of processors in map */

#define MAPNAME "gridlabd-pmap-3" /* TODO: change the pmap number each time the structure changes */
typedef struct s_gldprocinfo {
	unsigned int lock;		/* field lock */
	pid_t pid;			/* process id */
	TIMESTAMP progress;		/* current simtime */
	TIMESTAMP starttime;		/* sim starttime */
	TIMESTAMP stoptime;		/* sim stoptime */
	enumeration status;		/* current status */
	char1024 model;			/* model name */
	time_t start;			/* wall time of start */
} GLDPROCINFO;
static GLDPROCINFO *process_map = nullptr; /* global process map */

typedef struct {
	unsigned short n_procs; /* number of processors used by this process */
	unsigned short *list; /* list of processors assigned to this process */
} MYPROCINFO;
static MYPROCINFO *my_proc=nullptr; /* processors assigned to this process */
#define PROCERR ((unsigned short)-1)

static unsigned int show_progress = 1; /* flag to toggle progress/runtime display */

unsigned short sched_get_cpuid(unsigned short n)
{
	if ( my_proc==nullptr || my_proc->list==nullptr || n>=my_proc->n_procs )
		return PROCERR;
	return my_proc->list[n];
}
pid_t sched_get_procid()
{
	unsigned short cpuid = sched_get_cpuid(0);
	if(PROCERR == cpuid){
		output_warning("proc_map %x, myproc not assigned", process_map, sched_get_cpuid(0));
		return 0;
	}
	output_debug("proc_map %x, myproc %ui", process_map, sched_get_cpuid(0));
	return process_map[cpuid].pid;
}

void sched_lock(unsigned short proc)
{
	if ( process_map )
		wlock(&process_map[proc].lock);
}

void sched_unlock(unsigned short proc)
{
	if ( process_map )
		wunlock(&process_map[proc].lock);
}

/** update the process info **/
void sched_update(TIMESTAMP clock, enumeration status)
{
	int t;
	if ( my_proc==nullptr || my_proc->list==nullptr ) return;
	for ( t=0 ; t<my_proc->n_procs ; t++ )
	{
		int n = my_proc->list[t];
		sched_lock(n);
		process_map[n].status = status;
		process_map[n].progress = clock;
		process_map[n].starttime = global_starttime;
		process_map[n].stoptime = global_stoptime;
		sched_unlock(n);
	}
}
int sched_isdefunct(pid_t pid)
{
	/* signal 0 only checks process existence */
	if(pid != 0)
		return kill(pid,0)==-1;
	else
		return 0;
}

/** Unschedule process
 **/
void sched_finish(void)
{
	int t;
	if ( my_proc==nullptr || my_proc->list==nullptr ) return;
	for ( t=0 ; t<my_proc->n_procs ; t++ )
	{
		int n = my_proc->list[t];
		sched_lock(n);
		process_map[n].status = MLS_DONE;
		sched_unlock(n);
	}
}

/** Clear process map
 **/
void sched_clear(void)
{
	if ( process_map!=nullptr )
	{
		unsigned int n;
		for ( n=0 ; n<n_procs ; n++ )
		{
			if (sched_isdefunct(process_map[n].pid) )
			{
				sched_lock(n);
				process_map[n].pid = 0;
				sched_unlock(n);
			}
		}
	}
}
void sched_pkill(pid_t pid)
{
	if ( process_map!=nullptr && process_map[pid].pid!=0 )
	{
		kill(process_map[pid].pid, SIGINT);
	}
}

static char HEADING_R[] = "PROC PID   RUNTIME    STATE   CLOCK                   MODEL" ;
static char HEADING_P[] = "PROC PID   PROGRESS   STATE   CLOCK                   MODEL" ;
int sched_getinfo(int n,char *buf, size_t sz)
{
	const char *status;
	char ts[64];
	struct tm *tm;
	time_t ptime;
	int width = 80, namesize;
	static char *name=nullptr;
	char *HEADING = show_progress ? HEADING_P : HEADING_R;
	size_t HEADING_SZ = strlen(HEADING);
#ifdef _WIN32
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	if ( console )
	{
		CONSOLE_SCREEN_BUFFER_INFO cbsi;
		GetConsoleScreenBufferInfo(console,&cbsi);
		width = cbsi.dwSize.X-1;
	}
#else
	struct winsize ws;
	if ( ioctl(1,TIOCGWINSZ,&ws)!=-1 )
		width = ws.ws_col-1;
#endif
	namesize = (int)(width - (strstr(HEADING,"MODEL")-HEADING));
	if ( namesize<8 ) namesize=8;
	if ( namesize>1024 ) namesize=1024;
	if ( name!=nullptr ) free(name);
	name = (char*)malloc(namesize+1);

	if ( n==-1 ) /* heading string */
	{
		strncpy(buf,HEADING,sz);
		return (int)strlen(buf);		
	}
	else if ( n==-2 ) /* heading underlines */
	{
		for ( n=0 ; n<width ; n++ )
		{
			if ( n>0 && n<HEADING_SZ-1 && HEADING[n]==' ' && HEADING[n+1]!=' ' )
				buf[n] = ' ';
			else
				buf[n]='-';
		}
		buf[n]='\0';
		return width;
	}
	else if ( n==-3 ) /* bottom underline */
	{	for ( n=0 ; n<width ; n++ )
			buf[n] = '-';
		buf[n]='\0';
		return width;
	}

	if ( process_map==nullptr )
		return -1;

	sched_lock(n);
	ptime = (time_t)process_map[n].progress;
	tm = gmtime(&ptime);
	switch ( process_map[n].status ) {
	case MLS_INIT: status = "Init"; break;
	case MLS_RUNNING: status = "Running"; break;
	case MLS_PAUSED: status = "Paused"; break;
	case MLS_DONE: status = "Done"; break;
	case MLS_LOCKED: status = "Locked"; break;
	default: status = "Unknown"; break;
	}
	if ( process_map[n].pid!=0 )
	{
		char *modelname = process_map[n].model;
		int len;
		char t[64]=" - ";
		int is_defunct = 0;
		sched_unlock(n);
		is_defunct = sched_isdefunct(process_map[n].pid);
		sched_lock(n);
		if ( process_map[n].start>0 && process_map[n].status!=MLS_DONE && !is_defunct )
		{
			if ( !show_progress )
			{
				time_t s = (time(nullptr)-process_map[n].start);
				int h = 0;
				int m = 0;
	
				/* compute elapsed time */
				h = (int)(s/3600); s=s%3600;
				m = (int)(s/60); s=s%60;
				if ( h>0 ) sprintf(t,"%4d:%02d:%02d",h,m,(int)s);
				else if ( m>0 ) sprintf(t,"     %2d:%02d",m,(int)s);
				else sprintf(t,"       %2ds", (int)s);
			}
			else if ( process_map[n].stoptime!=TS_NEVER )
			{
				sprintf(t,"%.0f%%",100.0*(process_map[n].progress - process_map[n].starttime)/(process_map[n].stoptime-process_map[n].starttime));
			}
		}

		/* check for defunct process */
		if ( sched_isdefunct(process_map[n].pid) )
			status = "Defunct";

		/* format clock (without localization) */
		strftime(ts,sizeof(ts),"%Y-%m-%d %H:%M:%S UTC",tm);

		/* truncate path if match with cwd */
		if ( strnicmp(global_workdir,modelname,strlen(global_workdir))==0 )
		{
			modelname+=strlen(global_workdir);
			if ( modelname[0]=='/' || modelname[0]=='\\' ) modelname++; /* truncate remaining / */
		}
		
		/* rewrite model name to fit length limit */
		len = (int)strlen(modelname);
		if ( len<namesize )
			strcpy(name,modelname);
		else
		{
			/* remove the middle */
			int mid=namesize/2 - 3;
			strncpy(name,modelname,mid+1);
			strcpy(name+mid+1," ... ");
			strcat(name,modelname+len-mid);
		}

		/* print info */
		sz = sprintf(buf,"%4d %5d %10s %-7s %-23s %s", n, process_map[n].pid, t, status, process_map[n].progress==TS_ZERO?"INIT":ts, name);
	}
	else
		sz = sprintf(buf,"%4d   -", n);
	sched_unlock(n);
	return (int)sz;
}

void sched_print(int flags) /* flag=0 for single listing, flag=1 for continuous listing */
{
	char line[1024];
	int width = 80, namesize;
	static char *name=nullptr;
	char *HEADING = show_progress ? HEADING_P : HEADING_R;
#ifdef _WIN32
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	if ( console )
	{
		CONSOLE_SCREEN_BUFFER_INFO cbsi;
		GetConsoleScreenBufferInfo(console,&cbsi);
		width = cbsi.dwSize.X-1;
	}
#else
	struct winsize ws;
	if ( ioctl(1,TIOCGWINSZ,&ws)!=-1 )
		width = ws.ws_col;
#endif
	namesize = (int)(width - (strstr(HEADING,"MODEL")-HEADING));
	if ( namesize<8 ) namesize=8;
	if ( namesize>1024 ) namesize=1024;
	if ( name!=nullptr ) free(name);
	name = (char*)malloc(namesize+1);
	if ( process_map!=nullptr )
	{
		unsigned int n;
		int first=1;
		if ( flags==1 )
		{
			sched_getinfo(-1,line,sizeof(line));
			printf("%s\n",line);
			sched_getinfo(-2,line,sizeof(line));
			printf("%s\n",line);
		}
		for ( n=0 ; n<n_procs ; n++ )
		{
			if ( process_map[n].pid!=0 || flags==1 )
			{
				if ( sched_getinfo(n,line,sizeof(line))>0 )
					printf("%s\n",line);
				else
					printf("%4d (error)\n",n);
			}
		}
	}
}

MYPROCINFO *sched_allocate_procs(unsigned int n_threads, pid_t pid)
{
	int t;

#if defined WIN32
	int cpu;

	/* get process info */
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS,FALSE,pid);
	if ( hProc==nullptr )
	{
		unsigned long  err = GetLastError();
		output_warning("unable to access current process info, err code %d--job not added to process map", err);
		return nullptr;
	}
#elif defined MACOSX
	int cpu;
#elif defined DYN_PROC_AFFINITY
	cpu_set_t *cpuset = CPU_ALLOC(n_procs);
#elif defined HAVE_CPU_SET_T && defined HAVE_CPU_SET_MACROS
	cpu_set_t *cpuset = malloc(sizeof(cpu_set_t));
	CPU_ZERO(cpuset);
#endif

	if ( n_threads==0 ) n_threads = n_procs;
    my_proc = static_cast<MYPROCINFO *>(malloc(sizeof(MYPROCINFO)));
    my_proc->list = static_cast<unsigned short *>(malloc(sizeof(unsigned short) * n_threads));
	my_proc->n_procs = n_threads;
	for ( t=0 ; t<(int)n_threads ; t++ )
	{
		int n;
		for ( n=0 ; n<n_procs ; n++ )
		{
			sched_lock(n);
			if ( process_map[n].pid==0 )
				break;
			sched_unlock(n);
		}
		if ( n==n_procs )
		{
			goto Error;
			/** @todo wait for a processor to free up before running */
		}
		my_proc->list[t] = n;
		process_map[n].pid = pid;
		strncpy(process_map[n].model,global_modelname,sizeof(process_map[n].model)-1);
		process_map[n].start = time(nullptr);
		sched_unlock(n);

#ifdef _WIN32
		// TODO add this cpu to affinity
		cpu = n;
#elif defined MACOSX
		// TODO add this cpu to affinity
		cpu = n;
#elif defined DYN_PROC_AFFINITY /* linux */
		CPU_SET_S(n,CPU_ALLOC_SIZE(n_procs),cpuset);	
#elif defined HAVE_CPU_SET_T && defined HAVE_CPU_SET_MACROS
		CPU_SET(n,cpuset);
#endif
	}
#ifdef _WIN32
	// TODO set mp affinity
	if ( global_threadcount==1 && SetProcessAffinityMask(hProc,(DWORD_PTR)(1<<cpu))==0 )
	{
		unsigned long  err = GetLastError();
		output_error("unable to set current process affinity mask, err code %d", err);
	}
	CloseHandle(hProc);
#elif defined MACOSX
	// TODO set mp affinity
	if ( global_threadcount==1 )
	{
		policy.affinity_tag = cpu;
		if (thread_policy_set(mach_thread_self(), THREAD_AFFINITY_POLICY, reinterpret_cast<thread_policy_t>(&policy), THREAD_AFFINITY_POLICY_COUNT) != KERN_SUCCESS )
			output_warning("unable to set thread policy: %s", strerror(errno));
	}
#elif defined DYN_PROC_AFFINITY
	if (sched_setaffinity(pid,CPU_ALLOC_SIZE(n_procs),cpuset) )
		output_warning("unable to set current process affinity mask: %s", strerror(errno));
#elif defined HAVE_SCHED_SETAFFINITY
	if (sched_setaffinity(pid,sizeof(cpu_set_t),cpuset) )
		output_warning("unable to set current process affinity mask: %s", strerror(errno));
#endif
	return my_proc;
Error:
	if ( my_proc!=nullptr )
	{
		if ( my_proc->list!=nullptr ) free(my_proc->list);
		free(my_proc);
	}
#ifdef _WIN32
	CloseHandle(hProc);
#endif
	return nullptr;
}

/** Initialize the processor scheduling system

    This function sets up the processor scheduling system
	that is responsible to keep thread from migrating once
	they are committed to a particular processor.
 **/
#ifdef _WIN32
void sched_init(int readonly)
{
	static int has_run = 0;
	SYSTEM_INFO info;
	pid_t pid = (unsigned short)GetCurrentProcessId();
	HANDLE hMap;
	unsigned long mapsize;

	if(has_run == 0){
		has_run = 1;
	} else {
		output_verbose("sched_init(): second call, short-circuiting gracefully");
		return;
	}

	/* get total number of processors */
	GetSystemInfo(&info);
	n_procs = (unsigned char)info.dwNumberOfProcessors;
	mapsize = sizeof(GLDPROCINFO)*n_procs;

	/* get global process map */
	hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, MAPNAME);
	if ( hMap==nullptr )
	{
		/** @todo implement locking before creating the global process map */

		/* create global process map */
		hMap = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, mapsize, MAPNAME);
		if ( hMap==nullptr )
		{
			output_warning("unable to create global process map, error code %d--job not added to process map", GetLastError());
			return;
		}
	}

	/* access global process map */
	process_map = (GLDPROCINFO*) MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS,0,0,mapsize);
	if ( process_map==nullptr )
	{
		output_warning("unable to access global process map, error code %d--job not added to process map", GetLastError());
		return;
	}

	/* automatic cleanup of defunct jobs */
	if ( global_autoclean )
		sched_clear();

	/* readonly means don't record this job */
	if ( readonly ) return;

	/* find an available processor */
	my_proc = sched_allocate_procs(global_threadcount,pid);
	if ( my_proc==nullptr )
	{
		output_warning("no processor available to avoid overloading--job not added to process map");
		return;
	}
	atexit(sched_finish);
}

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>

void sched_init(int readonly)
{
	static int has_run = 0;
	const char *mfile = "/tmp/" MAPNAME;
	unsigned long mapsize;
	int fd = open(mfile,O_CREAT,0666);
	key_t shmkey = ftok(mfile,sizeof(GLDPROCINFO));
	pid_t pid = getpid();
	int shmid;
	int n;

	/* get total number of processors */
#ifndef DYN_PROC_AFFINITY
	n_procs = sysconf(_SC_NPROCESSORS_ONLN)>1024 ? 1024 : sysconf(_SC_NPROCESSORS_ONLN);
#else
	n_procs = sysconf(_SC_NPROCESSORS_ONLN);
#endif
	mapsize = sizeof(GLDPROCINFO)*n_procs;

	if(has_run == 0){
		has_run = 1;
	} else {
		output_verbose("sched_init(): second call, short-circuiting gracefully");
		return;
	}

	/* check key */
	if ( shmkey==-1 )
	{
		output_error("error generating key to global process map: %s", strerror(errno));
		return;
	}
	else
		close(fd);

	/* get global process map */
	shmid = shmget(shmkey,mapsize,IPC_CREAT|0666);
	if ( shmid<0 ) 
	{
		output_error("unable to create global process map: %s", strerror(errno));
		switch ( errno ) {
		case EACCES: output_error("access to global process map %s is denied", mfile); break;
		/* TROUBLESHOOT
		   Access to the process map is denied.  Consult with the system administrator
		   to obtain access to the process map.
		 */
		case EEXIST: output_error("global process map already exists"); break;
		case EINVAL: output_error("size of existing process map is not %d bytes or map size exceed system limit for shared memory", mapsize); break;
		/* TROUBLESHOOT
		   The process map changed size or it is too big for the limits on shared memory.
		   Reboot your system to clear the old process map.  If the problem persists,
		   consult your system's manuals to learn how to increase the size of shared memory.
		 */
		case ENOENT: output_error("process map %s not found", mfile); break;
		case ENOMEM: output_error("process map too big"); break;
		/* TROUBLESHOOT
		   The process map is too big for the resources available.
		   Try freeing up resources.  If the problem persists,
		   consult your system's manuals to learn how to reserve more shared memory.
		 */
		case ENOSPC: output_error("shared memory limit exceeded (need %.1fkB)",mapsize/1000.0); break;
		/* TROUBLESHOOT
		   The process map is too big for the limits on shared memory.
		   Consult your system's manuals to learn how to increase the size of shared memory.
		 */
		default: output_error("unknown shmget error");
		}
		return;
	}

	/* access global process map */
	process_map = (GLDPROCINFO*) shmat(shmid,nullptr,0);
	if ( process_map==nullptr )
	{
		output_error("unable to access global process map: %s", strerror(errno));
		return;
	}

	/* automatic cleanup of defunct jobs */
	if ( global_autoclean )
		sched_clear();

	/* readonly means don't record this job */
	if ( readonly )
		return;

	/* find an available processor */
	my_proc = sched_allocate_procs(global_threadcount,pid);
	if ( my_proc==nullptr )
	{
		output_warning("no processor available to avoid overloading--job not added to process map");
		return;
	}
	atexit(sched_finish);
}
#endif

/*********************************************************************
 * INTERACTIVE PROCESS CONTROLLER
 *********************************************************************/

typedef struct s_args {
	size_t n; /* number of args found */
	size_t a; /* size of arg list */
	char **arg; /* argument list */
} ARGS;
ARGS* get_args(char *line)
{
	int n=0;
	char *p, *tag;
	enum {P_INIT, P_SPACE, P_TEXT, P_QUOTE, P_QUOTES} state = P_INIT;
	ARGS *args = (ARGS*)malloc(sizeof(ARGS));

	/* prepare args structure */
	if ( args==nullptr )
	{
		output_fatal("memory allocation error");
		return nullptr;
	}
	memset(args,0,sizeof(args));

	/* determine maximum number of args needed */
	for ( p=line ; *p!='\0' ; p++ )
		if ( isspace(*p) ) n++;
	args->a = n;
	args->arg = (char**)malloc(sizeof(char*)*n);

	/* extract arguments */
	for ( p=line ; *p!='\0' ; p++ )
	{
		switch (state) {
		case P_INIT:
		case P_SPACE:			
			if ( !isspace(*p) )
			{
				state = P_TEXT;
				tag = p;
			}
			break;
		case P_TEXT:
			if ( isspace(*p) )
			{
				int len = (int)(p-tag);
				args->arg[args->n] = (char*)malloc(sizeof(char)*(len+1));
				strncpy(args->arg[args->n],tag,len);
				args->arg[args->n][len] = '\0';
				args->n++;
				state = P_SPACE;
			}
			else if ( *p=='\'' )
				state = P_QUOTE;
			else if ( *p=='"' )
				state = P_QUOTES;
			break;
		case P_QUOTE:
			if ( *p=='\'')
				state = P_TEXT;
			break;
		case P_QUOTES:
			if ( *p=='"')
				state = P_TEXT;
			break;
		default:
			output_fatal("get_args(char *line='%s'): unknown parser state '%d'", line, state);
			break;
		}
	}
	return args;
}
void free_args(ARGS *args)
{
	unsigned int n;
	for ( n=0 ; n<args->n ; n++ )
		free(args->arg[n]);
	free(args->arg);
	free(args);
}

static int sched_stop = 0;
#ifdef _WIN32
BOOL WINAPI sched_signal(DWORD type)
{
	if ( type==CTRL_C_EVENT )
	{
#else
void sched_signal(int sig)
{
#endif
		/* purge input stream */
		//while ( !feof(stdin) ) getchar();

		/* print a friendly message */
		printf("\n*** SIGINT ***\n");

		/* stop processing */
		sched_stop = 1;
#ifdef _WIN32
		return true;
	}
	return FALSE;
#endif
}

void sched_continuous(void)
{
#ifdef HAVE_CURSES
	char message[1024]="Ready.";
	int sel=0;
	unsigned int refresh_mod=10;
	unsigned int refresh_count=0;

	sched_init(1);

	initscr();
	cbreak();
	echo();
	intrflush(stdscr,true);
	keypad(stdscr,true);
	refresh();
	halfdelay(1);

	while ( sel>=0 && !sched_stop )
	{
		int c;
		char ts[64];
		struct tm *tb;
		time_t now = time(nullptr);
		if ( refresh_count++%refresh_mod==0 )
		{
			int n;
			char line[1024];
			clear();
			mvprintw(0,0,"GridLAB-D Process Control - Version %d.%d.%d-%d (%s)",REV_MAJOR,REV_MINOR,REV_PATCH,version_build(),version_branch());
			sched_getinfo(-1,line,sizeof(line));
			mvprintw(2,0,"%s",line);
			sched_getinfo(-2,line,sizeof(line));
			mvprintw(3,0,"%s",line);
			for ( n=0 ; n<n_procs ; n++ )
			{
				if ( sched_getinfo(n,line,sizeof(line))<0 )
					sprintf(message,"ERROR: unable to read process %d", n);
				if ( n==sel ) attron(A_BOLD);
				mvprintw(n+4,0,"%s",line);
				if ( n==sel ) attroff(A_BOLD);
			}
			sched_getinfo(-3,line,sizeof(line));
			mvprintw(n_procs+5,0,"%s",line);
			tb = localtime(&now);
			strftime(ts,sizeof(ts),"%Y/%m/%d %H:%M:%S",tb);
			mvprintw(n_procs+7,0,"%s: %s",ts,message);
			mvprintw(n_procs+8,0,"C to clear defunct, Up/Down to select, R/P to display runtime/progress, K to kill, Q to quit: ");
		}
		c = wgetch(stdscr);
		switch (c) {
		case KEY_UP:
			if ( sel>0 ) sel--;
			sprintf(message,"Process %d selected", sel);
			refresh_count=0;
			break;
		case KEY_DOWN:
			if ( sel<n_procs-1 ) sel++;
			refresh_count=0;
			sprintf(message,"Process %d selected", sel);
			break;
		case 'q':
		case 'Q':
			sel = -1;
			break;
		case 'k':
		case 'K':
			sched_pkill(sel);
			sprintf(message,"Kill signal sent to process %d",sel);
			refresh_count=0;
			break;
		case 'c':
		case 'C':
			sched_clear();
			sprintf(message,"Defunct processes cleared ok");
			refresh_count=0;
			break;
		case 'r':
		case 'R':
			show_progress = 0;
			sprintf(message,"Runtime display selected");
			refresh_count = 0;
			break;
		case 'p':
		case 'P':
			show_progress = 1;
			sprintf(message,"Progress display selected");
			refresh_count = 0;
			break;
		default:
			break;
		}
	}
	endwin();
#else
	output_error("unable to show screen without curses library");
#endif
}

void sched_controller(void)
{
	char command[1024];
	ARGS *last = nullptr;

	sched_continuous();
	return;

	global_suppress_repeat_messages = 0;
#ifdef _WIN32
	if ( !SetConsoleCtrlHandler(sched_signal,true) )
		output_warning("unable to suppress console Ctrl-C handler");
#else
	signal(SIGINT,sched_signal);
#endif

	printf("Gridlabd process controller starting");
	while ( sched_stop==0 )
	{
		ARGS *args;
		sched_stop = 0;
		while ( printf("\ngridlabd>> "), fgets(command,sizeof(command),stdin)==nullptr );
 		args = get_args(command);
		if ( args->n==0 ) { free_args(args); args=nullptr; }
		if ( args==nullptr && last!=nullptr ) { args=last; printf("gridlabd>> %s\n", last->arg[0]); }
		if ( args!=nullptr )
		{
			char *cmd = args->arg[0];
			size_t argc = args->n - 1;
			char **argv = args->arg + 1;
			if ( strnicmp(cmd,"quit",strlen(cmd))==0 )
				exit(XC_SUCCESS);
			else if ( strnicmp(cmd,"exit",strlen(cmd))==0 )
				exit(argc>0 ? atoi(argv[0]) : 0);
			else if ( strnicmp(cmd,"list",strlen(cmd))==0 )
				sched_print(0);
			else if ( strnicmp(cmd,"continuous",strlen(cmd))==0)
				sched_continuous();
			else if ( strnicmp(cmd,"clear",strlen(cmd))==0 )
				sched_clear();
			else if ( strnicmp(cmd,"kill",strlen(cmd))==0 )
			{
				if ( argc>0 )
					sched_pkill(atoi(argv[0]));
				else
					output_error("missing process id");
			}
			else if ( strnicmp(cmd,"help",strlen(cmd))==0 )
			{
				printf("Process controller help:\n");
				printf("  clear     clear process map\n");
				printf("  exit <n>  exit with status <n>\n");
				printf("  kill <n>  kill process <n>\n");
				printf("  list      list process map\n");
				printf("  quit      exit with status 0\n");
			}
			else
				output_error("command '%s' not found",cmd);
			if ( last!=nullptr && last!=args )
				free_args(last);
			last = args;
		}
	}
}


/**@}*/
