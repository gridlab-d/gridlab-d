/** $Id: globals.c 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file globals.c
	@addtogroup globals Global variables
	@ingroup core

	The GridLAB-D core maintains a group of global variables that can be accessed
	by both core functions and runtime modules using the core API.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "output.h"
#include "globals.h"
#include "module.h"
#include "lock.h"

static GLOBALVAR *global_varlist = NULL, *lastvar = NULL;

static KEYWORD df_keys[] = {
	{"ISO", DF_ISO, df_keys+1},
	{"US", DF_US, df_keys+2},
	{"EURO", DF_EURO, NULL},
};
static KEYWORD trl_keys[] = {
	{"PRINCIPLE",	TRL_PRINCIPLE, trl_keys+1},
	{"CONCEPT",		TRL_CONCEPT, trl_keys+2},
	{"PROOF",		TRL_PROOF, trl_keys+3},
	{"STANDALONE",	TRL_STANDALONE, trl_keys+4},
	{"INTEGRATED",	TRL_INTEGRATED, trl_keys+5},
	{"DEMONSTRATED",TRL_DEMONSTRATED, trl_keys+6},
	{"PROTOTYPE",	TRL_PROTOTYPE, trl_keys+7},
	{"QUALIFIED",	TRL_QUALIFIED, trl_keys+8},
	{"PROVEN",		TRL_PROVEN, trl_keys+9},
	{"UNKNOWN",		TRL_UNKNOWN, NULL},
};
static KEYWORD cpt_keys[] = {
	{"NONE", CPT_NONE, cpt_keys+1},
	{"WALL", CPT_WALL, cpt_keys+2},
	{"SIM",  CPT_SIM,  NULL},
};

static struct s_varmap {
	char *name;
	PROPERTYTYPE type;
	void *addr;
	PROPERTYACCESS access;
	KEYWORD *keys;
	void (*callback)(char *name);
} map[] = {
	/** @todo make this list the authorative list and retire the global_* list (ticket #25) */
	{"version.major", PT_int32, &global_version_major, PA_REFERENCE},
	{"version.minor", PT_int32, &global_version_minor, PA_REFERENCE},
	{"command_line", PT_char1024, &global_command_line, PA_REFERENCE},
	{"environment", PT_char1024, &global_environment, PA_REFERENCE},
	{"quiet", PT_int32, &global_quiet_mode, PA_PUBLIC},
	{"warn", PT_int32, &global_warn_mode, PA_PUBLIC},
	{"debugger", PT_int32, &global_debug_mode, PA_PUBLIC},
	{"gdb", PT_int32, &global_gdb, PA_PUBLIC},
	{"debug", PT_int32, &global_debug_output, PA_PUBLIC},
	{"test", PT_int32, &global_debug_mode, PA_PUBLIC},
	{"verbose", PT_int32, &global_verbose_mode, PA_PUBLIC},
	{"iteration_limit", PT_int32, &global_iteration_limit, PA_PUBLIC},
	{"workdir", PT_char1024, &global_workdir, PA_REFERENCE},
	{"dumpfile", PT_char1024, &global_dumpfile, PA_PUBLIC},
	{"savefile", PT_char1024, &global_savefile, PA_PUBLIC},
	{"dumpall", PT_int32, &global_dumpall, PA_PUBLIC},
	{"runchecks", PT_int32, &global_runchecks, PA_PUBLIC},
	{"threadcount", PT_int32, &global_threadcount, PA_PUBLIC},
	{"profiler", PT_int32, &global_profiler, PA_PUBLIC},
	{"pauseatexit", PT_int32, &global_pauseatexit, PA_PUBLIC},
	{"testoutputfile", PT_char1024, &global_testoutputfile, PA_PUBLIC},
	{"xml_encoding", PT_int32, &global_xml_encoding, PA_PUBLIC},
	{"clock", PT_timestamp, &global_clock, PA_PUBLIC},
	{"starttime", PT_timestamp, &global_starttime, PA_PUBLIC},
	{"stoptime", PT_timestamp, &global_stoptime, PA_PUBLIC},
	{"double_format", PT_char32, &global_double_format, PA_PUBLIC},
	{"complex_format", PT_char256, &global_complex_format, PA_PUBLIC},
	{"object_format", PT_char32, &global_object_format, PA_PUBLIC},
	{"object_scan", PT_char32, &global_object_scan, PA_PUBLIC},
	{"object_tree_balance", PT_bool, &global_no_balance, PA_PUBLIC},
	{"kmlfile", PT_char1024, &global_kmlfile, PA_PUBLIC},
	{"modelname", PT_char1024, &global_modelname, PA_REFERENCE},
	{"execdir",PT_char1024, &global_execdir, PA_REFERENCE},
	{"strictnames", PT_bool, &global_strictnames, PA_PUBLIC},
	{"website", PT_char1024, &global_urlbase, PA_PUBLIC}, /** @todo deprecate use of 'website' */
	{"urlbase", PT_char1024, &global_urlbase, PA_PUBLIC},
	{"randomseed", PT_int32, &global_randomseed, PA_PUBLIC,NULL,(void(*)(char*))random_init},
	{"include", PT_char1024, &global_include, PA_REFERENCE},
	{"trace", PT_char1024, &global_trace, PA_PUBLIC},
	{"gdb_window", PT_int32, &global_gdb_window, PA_PUBLIC},
	{"tmp", PT_char1024, &global_tmp, PA_PUBLIC},
	{"force_compile", PT_int32, &global_force_compile, PA_PUBLIC},
	{"nolocks", PT_int32, &global_nolocks, PA_PUBLIC},
	{"skipsafe", PT_int32, &global_skipsafe, PA_PUBLIC},
	{"dateformat", PT_enumeration, &global_dateformat, PA_PUBLIC, df_keys},
	{"minimum_timestep", PT_int32, &global_minimum_timestep, PA_PUBLIC},
	{"platform",PT_char8, global_platform, PA_REFERENCE},
	{"suppress_repeat_messages",PT_int32, &global_suppress_repeat_messages, PA_PUBLIC},
	{"maximum_synctime",PT_int32, &global_maximum_synctime, PA_PUBLIC},
	{"run_realtime",PT_int32, &global_run_realtime, PA_PUBLIC},
	{"no_deprecate",PT_int16, &global_suppress_deprecated_messages, PA_PUBLIC},
#ifdef _DEBUG
	{"sync_dumpfile",PT_char1024, &global_sync_dumpfile, PA_PUBLIC},
#endif
	{"streaming_io",PT_int32, &global_streaming_io_enabled, PA_PROTECTED},
	{"compileonly",PT_int32, &global_compileonly, PA_PROTECTED},
	{"relax_naming_rules",PT_int32,&global_relax_naming_rules, PA_PUBLIC},
	{"browser", PT_char1024, &global_browser, PA_PUBLIC},
	{"server_portnum",PT_int32,&global_server_portnum, PA_PUBLIC},
	{"server_quit_on_close",PT_int32,&global_server_quit_on_close, PA_PUBLIC},
	{"technology_readiness_level", PT_enumeration, &technology_readiness_level, PA_PUBLIC, trl_keys},
	{"show_progress",PT_int32,&global_show_progress,PA_PUBLIC},
	{"checkpoint_type", PT_enumeration, &global_checkpoint_type, PA_PUBLIC, cpt_keys},
	{"checkpoint_file", PT_char1024, &global_checkpoint_file, PA_PUBLIC},
	{"checkpoint_seqnum", PT_int32, &global_checkpoint_seqnum, PA_PUBLIC},
	{"checkpoint_interval", PT_int32, &global_checkpoint_interval, PA_PUBLIC},
	{"checkpoint_keepall", PT_int32, &global_checkpoint_keepall, PA_PUBLIC},
	{"check_version", PT_int32, &global_check_version, PA_PUBLIC},
	/* add new global variables here */
};

#ifdef WIN32
#	define TMP "C:\\WINDOWS\\TEMP"
#	define PATHSEP "\\"
#	define HOMEVAR "HOMEPATH"
#	define USERVAR "USERNAME"
#	define snprintf _snprintf
#else
#	define TMP "/tmp"
#	define PATHSEP "/"
#	define HOMEVAR "HOME"
#	define USERVAR "USER"
#endif

static void buildtmp(void)
{
	char *tmp, *home, *user;

	if ((tmp = getenv("GLTEMP"))) {
		snprintf(global_tmp, sizeof(global_tmp), "%s", tmp);
		return;
	}
	if (home = getenv(HOMEVAR)) {
#ifdef WIN32
		char *drive;
		if (!(drive = getenv("HOMEDRIVE")))
			drive = "";
		snprintf(global_tmp, sizeof(global_tmp),
				"%s%s\\Local Settings\\Temp\\gridlabd", drive, home);
#else
		snprintf(global_tmp, sizeof(global_tmp), "%s/.gridlabd/tmp", home);
#endif
		return;
	}
	if (!(tmp = getenv("TMP")) && !(tmp = getenv("TEMP")))
		tmp = TMP;
	user = getenv(USERVAR);
	snprintf(global_tmp, sizeof(global_tmp), "%s%s%s" PATHSEP "gridlabd",
			tmp, (user ? PATHSEP : ""), (user ? user : ""));
}

/** Register global variables
	@return SUCCESS or FAILED
 **/
STATUS global_init(void)
{
	unsigned int i;

	buildtmp();

	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++){
		struct s_varmap *p = &(map[i]);
		GLOBALVAR *var = global_create(p->name, p->type, p->addr, PT_ACCESS, p->access, NULL);
		if(var == NULL){
			output_error("global_init(): global variable '%s' registration failed", p->name);
			/* TROUBLESHOOT
				The global variable initialization process was unable to register
				the indicated global variable.  This error usually follows a more
				detailed explanation of the error.  Follow the troubleshooting for
				that message and try again.
			*/
		} else {
			var->prop->keywords = p->keys;
			var->callback = p->callback;
		}
	}
	return SUCCESS;
}

/** Find a global variable
	@return a pointer to the GLOBALVAR struct if found, NULL if not found
 **/
GLOBALVAR *global_find(char *name) /**< name of global variable to find */
{
	GLOBALVAR *var = NULL;
	for(var = global_getnext(NULL); var != NULL; var = global_getnext(var)){
		if(strcmp(var->prop->name, name) == 0){
			return var;
		}
	}
	return NULL;
}

/** Get global variable list

	This function retrieves global variable names.  The first time it should be called with
	a NULL pointer to retrieve the first variable name.  Subsequent calls should return the pointer
	to the previous variable name (and not a pointer to a copy).

	@return a pointer to the first character in the next variable name, or NULL of none found.
 **/
GLOBALVAR *global_getnext(GLOBALVAR *previous){ /**< a pointer to the previous variable name (NULL for first) */
	if(previous == NULL){
		return global_varlist;
	} else {
		return previous->next;
	}
}

/** Creates a user-defined global variable
	@return a pointer to GLOBALVAR struct or NULL is failed

	User defined variables can be created using a syntax similar to
	that of class_define_map().  However
	- addresses are absolute rather	than relative to the object
	- size specifies the number of instances of the type located
	  at the address

	@todo this does not support module globals but needs to (no ticket)

 **/
GLOBALVAR *global_create(char *name, ...){
	va_list arg;
	PROPERTY *prop = NULL, *lastprop = NULL;
	PROPERTYTYPE proptype;
	GLOBALVAR *var = NULL;

	/* don't create duplicate entries */
	if(global_find(name) != NULL){
		errno = EINVAL;
		output_error("tried to create global variable '%s' a second time", name);
		/* TROUBLESHOOT
			An attempt to create a global variable failed because the
			indicated variable already exists.  Find out what is attempting
			to create the variable and fix the problem and try again.
		*/
		return NULL;
	}

	/* allocate the global var definition */
	var = (GLOBALVAR *)malloc(sizeof(GLOBALVAR));
	memset(var,0,sizeof(GLOBALVAR));
	
	if(var == NULL){
		errno = ENOMEM;
		throw_exception("global_create(char *name='%s',...): unable to allocate memory for global variable", name);
		/* TROUBLESHOOT
			There is insufficient memory to allocate for the global variable.  Try freeing up memory and try again.
		 */
		return NULL;
	}

	var->prop = NULL;
	var->next = NULL;

	/* read the property args */
	va_start(arg, name);

	while ((proptype = va_arg(arg,PROPERTYTYPE)) != 0){
		if(proptype > _PT_LAST){
			if(prop == NULL){
				throw_exception("global_create(char *name='%s',...): property keyword not specified after an enumeration property definition", name);
			} else if(proptype == PT_KEYWORD && prop->ptype == PT_enumeration) {
				char *keyword = va_arg(arg, char *);
				int32 keyvalue = va_arg(arg, int32);
				KEYWORD *key = (KEYWORD *)malloc(sizeof(KEYWORD));
				if(key == NULL){
					throw_exception("global_create(char *name='%s',...): property keyword could not be stored", name);
					/* TROUBLESHOOT
						The memory needed to store the property's keyword is not available.  Try freeing up memory and try again.
					 */
				}
				key->next = prop->keywords;
				strncpy(key->name, keyword, sizeof(key->name));
				key->value = keyvalue;
				prop->keywords = key;
			} else if(proptype == PT_KEYWORD && prop->ptype == PT_set){
				char *keyword = va_arg(arg, char *);
				unsigned int64 keyvalue = va_arg(arg, int64); /* uchars are promoted to int by GCC */
				KEYWORD *key = (KEYWORD *)malloc(sizeof(KEYWORD));
				if(key == NULL){
					throw_exception("global_create(char *name='%s',...): property keyword could not be stored", name);
					/* TROUBLESHOOT
						The memory needed to store the property's keyword is not available.  Try freeing up memory and try again.
					 */
				}
				key->next = prop->keywords;
				strncpy(key->name, keyword, sizeof(key->name));
				key->value = keyvalue;
				prop->keywords = key;
			} else if(proptype == PT_ACCESS){
				PROPERTYACCESS pa = va_arg(arg, PROPERTYACCESS);
				switch (pa){
					case PA_PUBLIC:
					case PA_REFERENCE:
					case PA_PROTECTED:
					case PA_PRIVATE:
						prop->access = pa;
						break;
					default:
						errno = EINVAL;
						throw_exception("global_create(char *name='%s',...): unrecognized property access code (PROPERTYACCESS=%d)", name, pa);
						/* TROUBLESHOOT
							The specific property access code is not recognized.  Correct the access code and try again.
						 */
						break;
				}
			} else if(proptype == PT_SIZE){
				prop->size = va_arg(arg, uint32);
				if(prop->addr == 0){
					if (prop->size > 0){
						prop->addr = (PROPERTYADDR)malloc(prop->size * property_size(prop));
					} else {
						throw_exception("global_create(char *name='%s',...): property size must be greater than 0 to allocate memory", name);
						/* TROUBLESHOOT
							The size of the property must be positive.
						 */
					}
				}
			} else if(proptype == PT_UNITS){
				char *unitspec = va_arg(arg, char *);
				if((prop->unit = unit_find(unitspec)) == NULL){
					output_warning("global_create(char *name='%s',...): property %s unit '%s' is not recognized",name, prop->name,unitspec);
					/* TROUBLESHOOT
						The property definition uses a unit that is not found.  Check the unit and try again.  
						If you wish to define a new unit, try adding it to <code>.../etc/unitfile.txt</code>.
					 */
				}
			} else if (proptype == PT_DESCRIPTION) {
				prop->description = va_arg(arg,char*);
			} else if (proptype == PT_DEPRECATED) {
				prop->flags |= PF_DEPRECATED;
			} else {
				throw_exception("global_create(char *name='%s',...): property extension code not recognized (PROPERTYTYPE=%d)", name, proptype);
				/* TROUBLESHOOT
					The property extension code used is not valid.  This is probably a bug and should be reported.
				 */
			}
		} else {

			PROPERTYADDR addr = va_arg(arg,PROPERTYADDR);
			if(strlen(name) >= sizeof(prop->name)){
				throw_exception("global_create(char *name='%s',...): property name '%s' is too big to store", name, name);
				/* TROUBLESHOOT
					The property name cannot be longer than the size of the internal buffer used to store it (currently this is 63 characters).
					Use a shorter name and try again.
				 */
			}

			prop = property_malloc(proptype,NULL,name,addr,NULL);
			if (prop==NULL)
				throw_exception("global_create(char *name='%s',...): property '%s' could not be stored", name, name);
			if (var->prop==NULL)
				var->prop = prop;
			strncpy(var->prop->name, name, sizeof(var->prop->name));

			/* link map to oclass if not yet done */
			if (lastprop!=NULL)
				lastprop->next = prop;
			else
				lastprop = prop;

			strncpy(var->prop->name, name, sizeof(var->prop->name));

			/* save enum property in case keywords come up */
			if (prop->ptype>_PT_LAST)
				prop = NULL;
		}
	}
	va_end(arg);

	if (lastvar==NULL)
		/* first variable */
		global_varlist = lastvar = var;
	else
	{	/* not first */
		lastvar->next = var;
		lastvar = var;
	}

	return var;
}

/** Sets a user-defined global variable
	@return STATUS value

	User defined global variables can be set using a syntax similar
	to that of putenv().  The definition must be of the form \p "NAME=VALUE".
	If there is no '=' in the first argument, then it is used as a name
	and the second argument is read as a pointer to a string the contains
	the new value.
 **/
STATUS global_setvar(char *def, ...) /**< the definition */
{
	char name[65]="", value[1024]="";
	if (sscanf(def,"%[^=]=%[^\r\n]",name,value)<2)
	{
		va_list ptr;
		char *v;
		va_start(ptr,def);
		v = va_arg(ptr,char*);
		va_end(ptr);
		if (v!=NULL) 
		{
			strncpy(value,v,sizeof(value));
			if (strcmp(value,v)!=0)
				output_error("global_setvar(char *name='%s',...): value is too long to store");
				/* TROUBLESHOOT
					An attempt to set a global variable failed because the value of the variable
					was too long.
				 */
		}
	}
	if (strcmp(name,"")!=0) /* something was defined */
	{
		GLOBALVAR *var = global_find(name);
		static int globalvar_lock = 0;
		int retval;
		if (var==NULL)
		{
			if (global_strictnames)
			{
				output_error("strict naming prevents implicit creation of %s", name);
				/* TROUBLESHOOT
					The global_strictnames variable prevents the system from implicitly
					creating a new variable just by setting a value.  Try using the
					name of a named variable or remove the strict naming setting
					by setting global_strictnames=0.
				 */
				return FAILED;
			}

			/** @todo autotype global variables when creating them (ticket #26) */
			var = global_create(name,PT_char1024,NULL,PT_SIZE,1,PT_ACCESS,PA_PUBLIC,NULL);
		}
		lock(&globalvar_lock);
		retval = class_string_to_property(var->prop,(void*)var->prop->addr,value);
		unlock(&globalvar_lock);
		if (retval==0){
			output_error("global_setvar(): unable to set %s to %s",name,value);
			/* TROUBLESHOOT
				The input value was not convertable into the desired type for the input
				variable.  Check the input range, review the input file, and adjust
				the input value appropriately.
			 */
			return FAILED;
		}
		else if (var->callback) 
			var->callback(var->prop->name);

		return SUCCESS;
	}
	else
	{
		output_error("global variable definition '%s' not formatted correctedly", def);
		/* TROUBLESHOOT
			A request to set a global variable was not formatted properly.  Use the
			proper format, i.e. name=value, and try again.
		 */
		return FAILED;
	}
}

/** Get the value of a global variable in a safer fashion
	@return a \e char * pointer to the buffer holding the buffer where we wrote the data,
		\p NULL if insufficient buffer space or if the \p name was not found.

	This function searches global, user-defined, and module variables for a match.
**/
char *global_getvar(char *name, char *buffer, int size){
	//static char local_buff[1024];
	char temp[1024];
	int len = 0;
	GLOBALVAR *var = NULL;
	if(buffer == NULL){
		output_error("global_getvar: buffer not supplied");
		return 0;
	}
	if(name == NULL){
		output_error("global_getvar: variable name not supplied");
		return NULL;
	}
	if(size < 1){
		output_error("global_getvar: invalid buffer size");
		return NULL; /* user error ... could force it, but that's asking for trouble. */
	}
	var = global_find(name);
	if(var == NULL)
		return NULL;
	len = class_property_to_string(var->prop, (void *)var->prop->addr, temp, sizeof(temp));
	if(len < size){ /* if we have enough space, copy to the supplied buffer */
		strncpy(buffer, temp, len+1);
		return buffer; /* wrote buffer, return ptr for printf funcs */
	}
	return NULL; /* NULL if insufficient buffer space */
}

void global_dump(void)
{
	GLOBALVAR *var=NULL;
	while ((var=global_getnext(var))!=NULL)
	{
		char buffer[1024];
		if (class_property_to_string(var->prop, (void*)var->prop->addr,buffer,sizeof(buffer)))
			output_message("%s=%s;", var->prop->name, buffer);
	}
}

/**@}**/
