/** $Id: module.c 1212 2009-01-17 01:42:30Z d3m998 $
	Copyright (C) 2008 Battelle Memorial Institute
	@file module.cpp
	@addtogroup modules Runtime modules

 @{
 **/

#if defined WIN32 && ! defined MINGW
#include <io.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <math.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#include "interpolate.h"
#ifndef MINGW
#include "lock.h"
#endif
#include "schedule.h"

#include "matlab.h"

int get_exe_path(char *buf, int len, void *mod){	/* void for GetModuleFileName, a windows func */
	int rv = 0, i = 0;
	if(buf == NULL)
		return 0;
	if(len < 1)
		return 0;
#if defined WIN32 && ! defined MINGW
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
#ifndef MINGW
#if defined WIN32
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
#else
	char *error = "unknown error";
#endif
	output_debug("%s: %s (LD_LIBRARY_PATH=%s)", filename, error,getenv("LD_LIBRARY_PATH"));
#if defined WIN32 && ! defined MINGW
	if (result)
		LocalFree(error);
#endif
}

/* MALLOC/FREE - GL threadsafe versions */
static int malloc_lock = 0;
void *module_malloc(size_t size)
{
	void *ptr;
	lock(&malloc_lock);
	ptr = (void*)malloc(size);
	unlock(&malloc_lock);
	return ptr;
}
void module_free(void *ptr)
{
	lock(&malloc_lock);
	free(ptr);
	unlock(&malloc_lock);
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
	{class_define_function,class_get_function},
	class_define_enumeration_member,
	class_define_set_member,
	object_set_dependent,
	object_set_parent,
	object_set_rank,
	{object_get_property, object_set_value_by_addr,object_get_value_by_addr, object_set_value_by_name,object_get_value_by_name,object_get_reference,object_get_unit,object_get_addr,class_string_to_propertytype},
	{find_objects,find_next,findlist_copy,findlist_add,findlist_del,findlist_clear},
	class_find_property,
	module_malloc,
	module_free,
	aggregate_mkgroup,
	aggregate_value,
	module_getvar_addr,
	module_depends,
	{random_uniform, random_normal, random_bernoulli, random_pareto, random_lognormal, random_sampled, random_exponential, random_type, random_value, pseudorandom_value, random_triangle, random_beta, random_gamma, random_weibull, random_rayleigh},
	object_isa,
	class_register_type,
	class_define_type,
	{mkdatetime,strdatetime,timestamp_to_days,timestamp_to_hours,timestamp_to_minutes,timestamp_to_seconds,local_datetime,convert_to_timestamp,convert_from_timestamp},
	unit_convert, unit_convert_ex, unit_find,
	{create_exception_handler,delete_exception_handler,throw_exception,exception_msg},
	{global_create, global_setvar, global_getvar, global_find},
#ifndef NOLOCKS
	&lock_count, &lock_spin,
#endif
	{find_file},
	{object_get_complex, object_get_enum, object_get_int16, object_get_int32, object_get_int64, object_get_double, object_get_string, object_get_object},
	{object_get_complex_by_name, object_get_enum_by_name, object_get_int16_by_name, object_get_int32_by_name, object_get_int64_by_name,
		object_get_double_by_name, object_get_string_by_name, object_get_object_by_name},
	{class_string_to_property, class_property_to_string,},
	module_find,
	object_find_name,
	object_build_name,
	object_get_oflags,
	object_get_count,
	{schedule_create, schedule_index, schedule_value, schedule_dtnext, schedule_find_byname},
	{loadshape_create,loadshape_init},
	{enduse_create,enduse_sync},
	{interpolate_linear, interpolate_quadratic},
	{forecast_create, forecast_find, forecast_read, forecast_save},
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
	char tpath[1024];
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

#ifdef NEVER /* this shouldn't ever be necessary but sometimes for debugging purposes it is helpful */
	/* if LD_LIBRARY_PATH is not set, default to current directory */
	if (getenv("LD_LIBRARY_PATH")==NULL)
	{
		putenv("LD_LIBRARY_PATH=.");
		output_verbose("Setting default LD_LIBRARY_DEFAULT to current directory");
	}
#endif

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
	snprintf(pathname, 1024, "%s" DLEXT, file);

	if(find_file(pathname, NULL, X_OK|R_OK, tpath,sizeof(tpath)) == NULL)
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
	for (p=strchr(tpath,from); p!=NULL; p=strchr(p,from))
		*p=to;

	/* ok, let's do it */
	hLib = DLLOAD(tpath);
	if (hLib==NULL)
	{
#if defined WIN32 && ! defined MINGW
		output_error("%s(%d): module '%s' load failed - %s (error code %d)", __FILE__, __LINE__, file, strerror(errno), GetLastError());
#else
		output_error("%s(%d): module '%s' load failed - %s", __FILE__, __LINE__, file, dlerror());
		output_debug("%s(%d): path to module is '%s'", __FILE__, __LINE__, tpath);
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
	mod->import_file = (int(*)(const char*))DLSYM(hLib,"import_file");
	mod->export_file = (int(*)(const char*))DLSYM(hLib,"export_file");
	mod->setvar = (int(*)(const char*,char*))DLSYM(hLib,"setvar");
	mod->getvar = (void*(*)(const char*,char*,unsigned int))DLSYM(hLib,"getvar");
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
	if (mod->oclass==NULL)
		return NULL;

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
			{&c->commit,"commit",TRUE},
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
				/*	TROUBLESHOOT
					A required intrinsic function was not found.  Please review and modify the class definition.
				 */
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

int module_setvar(MODULE *mod, const char *varname, char *value)
{
	char modvarname[1024];
	sprintf(modvarname,"%s::%s",mod->name,varname);
	return global_setvar(modvarname,value)==SUCCESS;
}

void* module_getvar(MODULE *mod, const char *varname, char *value, unsigned int size)
{
	char modvarname[1024];
	sprintf(modvarname,"%s::%s",mod->name,varname);
	return global_getvar(modvarname,value,size);
}

void* module_getvar_old(MODULE *mod, const char *varname, char *value, unsigned int size)
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

double* module_getvar_addr(MODULE *mod, const char *varname)
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
			if(strncmp(tname, gvptr->prop->name, tlen) == 0){
				count += fprintf(fp, "\t\t\t<%s>%s</%s>\n", gvptr->prop->name+tlen, class_property_to_string(gvptr->prop,(void*)gvptr->prop->addr,buffer,1024)>0 ? buffer : "...", gvptr->prop->name+tlen);
			} // else we have a module::prop name
			gvptr = global_getnext(gvptr);
		}
		count += fprintf(fp, "\t\t</properties>\n");
		module_saveobj_xml(fp, mod);	/* insert objects w/in module tag */
		count += fprintf(fp,"\t</module>\n");
	}
	return count;
}

#if defined WIN32 && ! defined MINGW
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
		if ((oclass == NULL) || (obj->oclass != oclass))
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
		for (prop=oclass->pmap;prop!=NULL && prop->oclass==oclass;prop=prop->next)
		{
			char *value = NULL;
			if((prop->access != PA_PUBLIC) && (prop->access != PA_REFERENCE))
				continue;
			value = object_property_to_string(obj,prop->name, buffer, 1023);
			if (value!=NULL){
				count += fprintf(fp, "\t\t\t<%s>%s</%s>\n", prop->name, value, prop->name);
			}
		}
		pclass = oclass->parent;
		while(pclass != NULL){ /* inherited properties */
			for (prop=pclass->pmap;prop!=NULL && prop->oclass==pclass;prop=prop->next){
				char *value = object_property_to_string(obj,prop->name, buffer, 1023);
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

int module_import(MODULE *mod, const char *filename)
{
	if (mod->import_file == NULL)
	{
		errno = ENOENT;
		return 0;
	}
	return (*mod->import_file)(filename);
}

int module_save(MODULE *mod, const char *filename)
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

void module_libinfo(const char *module_name)
{
	MODULE *mod = module_load(module_name,0,NULL);
	if (mod!=NULL)
	{
		CLASS *c;
		PROPERTY *p;
		GLOBALVAR *v=NULL;
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
		while ((v=global_getnext(v))!=NULL)
		{
			if (strncmp(v->prop->name,module_name,strlen(module_name))==0)
			{
				char *vn = strstr(v->prop->name,"::");
				if (vn!=NULL)
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
