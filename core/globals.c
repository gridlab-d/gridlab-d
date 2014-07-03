/** $Id: globals.c 4738 2014-07-03 00:55:39Z dchassin $
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
	{"NONE", CPT_NONE, cpt_keys+1},	/**< no checkpoint done */
	{"WALL", CPT_WALL, cpt_keys+2},	/**< checkpoint on wall clock interval */
	{"SIM",  CPT_SIM,  NULL},		/**< checkpoint on simulation clock interval */
};

static KEYWORD rng_keys[] = {
	{"RNG2", RNG2, rng_keys+1},		/**< version 2 random number generator (stateless) */
	{"RNG3", RNG3, NULL,},			/**< version 3 random number generator (statefull) */
};

static KEYWORD mls_keys[] = {
	{"INIT", MLS_INIT, mls_keys+1},			/**< main loop hasn't started yet */
	{"RUNNING", MLS_RUNNING, mls_keys+2},	/**< main loop is running */
	{"PAUSED", MLS_PAUSED, mls_keys+3},		/**< main loop is paused */
	{"DONE", MLS_DONE, mls_keys+4},			/**< main loop is done */
	{"LOCKED", MLS_LOCKED, NULL},			/**< main loop is locked */
};

static KEYWORD mrm_keys[] = {
	{"STANDALONE", MRM_STANDALONE, mrm_keys+1}, /**< run is standalone */
	{"MASTER", MRM_MASTER, mrm_keys+2}, /**< run is a master of multirun */
	{"SLAVE", MRM_SLAVE, NULL}, /**< run is a slave of a multirun */
};

static KEYWORD mrc_keys[] = {
	{"NONE", MRC_NONE, mrc_keys+1},		/**< isn't actually connected upwards */
	{"MEMORY", MRC_MEM, mrc_keys+2},	/**< use shared mem or the like */
	{"SOCKET", MRC_SOCKET, NULL},		/**< use a socket */
};

static KEYWORD isc_keys[] = {
	{"CREATION", IS_CREATION, isc_keys+1},
	{"DEFERRED", IS_DEFERRED, isc_keys+2},
	{"BOTTOMUP", IS_BOTTOMUP, isc_keys+3},
	{"TOPDOWN", IS_TOPDOWN, NULL}
};

static KEYWORD mcf_keys[] = {
	{"NONE", MC_NONE, mcf_keys+1},		/**< no module compiler flags set */
	{"CLEAN", MC_CLEAN, mcf_keys+2},	/**< flag to rebuild everything (no reuse of previous work) */
	{"KEEPWORK", MC_KEEPWORK, mcf_keys+3},	/**< flag to keep everything (do not delete intermediate files) */
	{"DEBUG", MC_DEBUG, mcf_keys+4},	/**< flag to build with debugging turned on */
	{"VERBOSE", MC_VERBOSE, NULL},		/**< flag to output commands as they are executed */
};
static KEYWORD vo_keys[] = {
	{"NONE", VO_NONE, vo_keys+1},
	{"TSTD", VO_TSTSTD, vo_keys+2},
	{"TALL", VO_TSTALL, vo_keys+3},
	{"TRUN", VO_TSTRUN, vo_keys+4},
	{"TERR", VO_TSTERR, vo_keys+5},
	{"TEXC", VO_TSTEXC, vo_keys+6},
	{"TOPT", VO_TSTOPT, vo_keys+7},
	{"RALL", VO_RPTALL, vo_keys+8},
	{"RDIR", VO_RPTDIR, vo_keys+9},
	{"RGLM", VO_RPTGLM, NULL},
};
static KEYWORD so_keys[] = {
	{"NAMES", SO_NAMES, so_keys+1},
	{"POSITIONS", SO_GEOCOORDS, NULL},
};
static KEYWORD sm_keys[] = {
	{"INIT", SM_INIT, sm_keys+1},
	{"EVENT", SM_EVENT, sm_keys+2},
	{"DELTA", SM_DELTA, sm_keys+3},
	{"DELTA_ITER", SM_DELTA_ITER, sm_keys+4},
	{"ERROR", SM_ERROR, NULL},
};

static struct s_varmap {
	char *name;
	PROPERTYTYPE type;
	void *addr;
	PROPERTYACCESS access;
	char *description;
	KEYWORD *keys;
	void (*callback)(char *name);
} map[] = {
	/** @todo make this list the authorative list and retire the global_* list (ticket #25) */
	{"version.major", PT_int32, &global_version_major, PA_REFERENCE, "major version"},
	{"version.minor", PT_int32, &global_version_minor, PA_REFERENCE, "minor version"},
	{"version.patch", PT_int32, &global_version_patch, PA_REFERENCE, "patch number"},
	{"version.build", PT_int32, &global_version_build, PA_REFERENCE, "build number"},
	{"version.branch", PT_char32, &global_version_branch, PA_REFERENCE, "branch name"},
	{"command_line", PT_char1024, &global_command_line, PA_REFERENCE, "command line"},
	{"environment", PT_char1024, &global_environment, PA_PUBLIC, "operating environment"},
	{"quiet", PT_bool, &global_quiet_mode, PA_PUBLIC, "quiet output status flag"},
	{"warn", PT_bool, &global_warn_mode, PA_PUBLIC, "warning output status flag"},
	{"debugger", PT_bool, &global_debug_mode, PA_PUBLIC, "debugger enable flag"},
	{"gdb", PT_bool, &global_gdb, PA_PUBLIC, "gdb enable flag"},
	{"debug", PT_bool, &global_debug_output, PA_PUBLIC, "debug output status flag"},
	{"test", PT_bool, &global_debug_mode, PA_PUBLIC, "test enable flag"},
	{"verbose", PT_bool, &global_verbose_mode, PA_PUBLIC, "verbose enable flag"},
	{"iteration_limit", PT_int32, &global_iteration_limit, PA_PUBLIC, "iteration limit"},
	{"workdir", PT_char1024, &global_workdir, PA_REFERENCE, "working directory"},
	{"dumpfile", PT_char1024, &global_dumpfile, PA_PUBLIC, "dump filename"},
	{"savefile", PT_char1024, &global_savefile, PA_PUBLIC, "save filename"},
	{"dumpall", PT_bool, &global_dumpall, PA_PUBLIC, "dumpall enable flag"},
	{"runchecks", PT_bool, &global_runchecks, PA_PUBLIC, "runchecks enable flag"},
	{"threadcount", PT_int32, &global_threadcount, PA_PUBLIC, "number of threads to use while using multicore"},
	{"profiler", PT_bool, &global_profiler, PA_PUBLIC, "profiler enable flag"},
	{"pauseatexit", PT_bool, &global_pauseatexit, PA_PUBLIC, "pause at exit flag"},
	{"testoutputfile", PT_char1024, &global_testoutputfile, PA_PUBLIC, "filename for test output"},
	{"xml_encoding", PT_int32, &global_xml_encoding, PA_PUBLIC, "XML data encoding"},
	{"clock", PT_timestamp, &global_clock, PA_PUBLIC, "global clock"},
	{"starttime", PT_timestamp, &global_starttime, PA_PUBLIC, "simulation start time"},
	{"stoptime", PT_timestamp, &global_stoptime, PA_PUBLIC, "simulation stop time"},
	{"double_format", PT_char32, &global_double_format, PA_PUBLIC, "format for writing double values"},
	{"complex_format", PT_char256, &global_complex_format, PA_PUBLIC, "format for writing complex values"},
	{"object_format", PT_char32, &global_object_format, PA_PUBLIC, "format for writing anonymous object names"},
	{"object_scan", PT_char32, &global_object_scan, PA_PUBLIC, "format for reading anonymous object names"},
	{"object_tree_balance", PT_bool, &global_no_balance, PA_PUBLIC, "object index tree balancing enable flag"},
	{"kmlfile", PT_char1024, &global_kmlfile, PA_PUBLIC, "KML output file name"},
	{"modelname", PT_char1024, &global_modelname, PA_REFERENCE, "model name"},
	{"execdir",PT_char1024, &global_execdir, PA_REFERENCE, "directory where executable binary was found"},
	{"strictnames", PT_bool, &global_strictnames, PA_PUBLIC, "strict global name enable flag"},
	{"website", PT_char1024, &global_urlbase, PA_PUBLIC, "url base string (deprecated)"}, /** @todo deprecate use of 'website' */
	{"urlbase", PT_char1024, &global_urlbase, PA_PUBLIC, "url base string"},
	{"randomseed", PT_int32, &global_randomseed, PA_PUBLIC, "random number generator seed value", NULL,(void(*)(char*))random_init},
	{"include", PT_char1024, &global_include, PA_REFERENCE, "include folder path"},
	{"trace", PT_char1024, &global_trace, PA_PUBLIC, "trace function list"},
	{"gdb_window", PT_bool, &global_gdb_window, PA_PUBLIC, "gdb window enable flag"},
	{"tmp", PT_char1024, &global_tmp, PA_PUBLIC, "temporary folder name"},
	{"force_compile", PT_int32, &global_force_compile, PA_PUBLIC, "force recompile enable flag"},
	{"nolocks", PT_bool, &global_nolocks, PA_PUBLIC, "locking disable flag"},
	{"skipsafe", PT_bool, &global_skipsafe, PA_PUBLIC, "skip sync safe enable flag"},
	{"dateformat", PT_enumeration, &global_dateformat, PA_PUBLIC, "date format string", df_keys},
	{"init_sequence", PT_enumeration, &global_init_sequence, PA_PUBLIC, "initialization sequence control flag", isc_keys},
	{"minimum_timestep", PT_int32, &global_minimum_timestep, PA_PUBLIC, "minimum timestep"},
	{"platform",PT_char8, global_platform, PA_REFERENCE, "operating platform"},
	{"suppress_repeat_messages",PT_bool, &global_suppress_repeat_messages, PA_PUBLIC, "suppress repeated messages enable flag"},
	{"maximum_synctime",PT_int32, &global_maximum_synctime, PA_PUBLIC, "maximum sync time for deltamode"},
	{"run_realtime",PT_bool, &global_run_realtime, PA_PUBLIC, "realtime enable flag"},
	{"no_deprecate",PT_bool, &global_suppress_deprecated_messages, PA_PUBLIC, "suppress deprecated usage message enable flag"},
#ifdef _DEBUG
	{"sync_dumpfile",PT_char1024, &global_sync_dumpfile, PA_PUBLIC, "sync event dump file name"},
#endif
	{"streaming_io",PT_bool, &global_streaming_io_enabled, PA_PROTECTED, "streaming I/O enable flag"},
	{"compileonly",PT_bool, &global_compileonly, PA_PROTECTED, "compile only enable flag"},
	{"relax_naming_rules",PT_bool,&global_relax_naming_rules, PA_PUBLIC, "relax object naming rules enable flag"},
	{"browser", PT_char1024, &global_browser, PA_PUBLIC, "browser selection"},
	{"server_portnum",PT_int32,&global_server_portnum, PA_PUBLIC, "server port number"},
	{"server_quit_on_close",PT_bool,&global_server_quit_on_close, PA_PUBLIC, "server quit on connection closed enable flag"},
	{"autoclean",PT_bool,&global_autoclean, PA_PUBLIC, "autoclean enable flag"},
	{"technology_readiness_level", PT_enumeration, &technology_readiness_level, PA_PUBLIC, "technology readiness level", trl_keys},
	{"show_progress",PT_bool,&global_show_progress,PA_PUBLIC, "show progress enable flag"},
	{"checkpoint_type", PT_enumeration, &global_checkpoint_type, PA_PUBLIC, "checkpoint type usage flag", cpt_keys},
	{"checkpoint_file", PT_char1024, &global_checkpoint_file, PA_PUBLIC, "checkpoint file base name"},
	{"checkpoint_seqnum", PT_int32, &global_checkpoint_seqnum, PA_PUBLIC, "checkpoint sequence number"},
	{"checkpoint_interval", PT_int32, &global_checkpoint_interval, PA_PUBLIC, "checkpoint interval"},
	{"checkpoint_keepall", PT_bool, &global_checkpoint_keepall, PA_PUBLIC, "checkpoint file keep enable flag"},
	{"check_version", PT_bool, &global_check_version, PA_PUBLIC, "check version enable flag"},
	{"random_number_generator", PT_enumeration, &global_randomnumbergenerator, PA_PUBLIC, "random number generator version control flag", rng_keys},
	{"mainloop_state", PT_enumeration, &global_mainloopstate, PA_PUBLIC, "main sync loop state flag", mls_keys},
	{"pauseat", PT_timestamp, &global_mainlooppauseat, PA_PUBLIC, "pause at time"},
	{"infourl", PT_char1024, &global_infourl, PA_PUBLIC, "URL to use for obtaining online help"},
	{"hostname", PT_char1024, &global_hostname, PA_PUBLIC, "unused"},
	{"hostaddr", PT_char32, &global_hostaddr, PA_PUBLIC, "unused"},
	{"autostart_gui", PT_bool, &global_autostartgui, PA_PUBLIC, "automatic GUI start enable flag"},
	{"master", PT_char1024, &global_master, PA_PUBLIC, "master server hostname"},
	{"master_port", PT_int64, &global_master_port, PA_PUBLIC, "master server port number"},
	{"multirun_mode", PT_enumeration, &global_multirun_mode, PA_PUBLIC, "multirun enable flag", mrm_keys},
	{"multirun_conn", PT_enumeration, &global_multirun_connection, PA_PUBLIC, "unused", mrc_keys},
	{"signal_timeout", PT_int32, &global_signal_timeout, PA_PUBLIC, "unused"},
	{"slave_port", PT_int16, &global_slave_port, PA_PUBLIC, "unused"},
	{"slave_id", PT_int64, &global_slave_id, PA_PUBLIC, "unused"},
	{"return_code", PT_int32, &global_return_code, PA_REFERENCE, "unused"},
	{"module_compiler_flags", PT_set, &global_module_compiler_flags, PA_PUBLIC, "module compiler flags", mcf_keys},
	{"init_max_defer", PT_int32, &global_init_max_defer, PA_REFERENCE, "deferred initialization limit"},
	{"mt_analysis", PT_bool, &global_mt_analysis, PA_PUBLIC, "perform multithread profile optimization analysis"},
	{"inline_block_size", PT_int32, &global_inline_block_size, PA_PUBLIC, "inline code block size"},
	{"validate", PT_set, &global_validateoptions, PA_PUBLIC, "validation test options",vo_keys},
	{"sanitize", PT_set, &global_sanitizeoptions, PA_PUBLIC, "sanitize process options",so_keys},
	{"sanitize_prefix", PT_char8, &global_sanitizeprefix, PA_PUBLIC, "sanitized name prefix"},
	{"sanitize_index", PT_char1024, &global_sanitizeindex, PA_PUBLIC, "sanitization index file spec"},
	{"sanitize_offset", PT_char32, &global_sanitizeoffset, PA_PUBLIC, "sanitization lat/lon offset"},
	{"simulation_mode",PT_enumeration,&global_simulation_mode,PA_PUBLIC, "current time simulation type",sm_keys},
	{"deltamode_timestep",PT_int32,&global_deltamode_timestep,PA_PUBLIC, "uniform step size for deltamode simulations"},
	{"deltamode_maximumtime", PT_int64,&global_deltamode_maximumtime,PA_PUBLIC, "maximum time (ns) deltamode can run"},
	{"deltaclock", PT_int64, &global_deltaclock, PA_PUBLIC, "cumulative delta runtime with respect to the global clock"},
	{"delta_current_clock", PT_double, &global_delta_curr_clock, PA_PUBLIC, "Absolute delta time (global clock offset)"},
	{"deltamode_updateorder", PT_char1024, &global_deltamode_updateorder, PA_REFERENCE, "order in which modules are update in deltamode"},
	{"deltamode_iteration_limit", PT_int32, &global_deltamode_iteration_limit, PA_PUBLIC, "iteration limit for each delta timestep (object and interupdate)"},
	{"run_powerworld", PT_bool, &global_run_powerworld, PA_PUBLIC, "boolean that that says your system is set up correctly to run with PowerWorld"},
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
		GLOBALVAR *var = global_create(p->name, p->type, p->addr, PT_ACCESS, p->access, p->description?PT_DESCRIPTION:0, p->description, NULL);
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
					case PA_HIDDEN:
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

			/* link map to oclass if not yet done */
			if (lastprop!=NULL)
				lastprop->next = prop;
			else
				lastprop = prop;

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
			if ( var==NULL )
			{
				output_error("unable to implicitly create the global variable '%s'", name);
				/* TROUBLESHOOT
					An attempt to create the global variable indicated failed.  
					This is an internal error and should be reported to the software developers.
				 */
				return FAILED;
			}
		}
		wlock(&globalvar_lock);
		retval = class_string_to_property(var->prop,(void*)var->prop->addr,value);
		wunlock(&globalvar_lock);
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

static int guid_first=1;
char *global_guid(char *buffer, int size)
{
	if ( size>36 )
	{
		if ( guid_first )
		{
			srand(entropy_source());
			guid_first = 0;
		}
		sprintf(buffer,"%04x%04x-%04x-4%03x-%04x-%04x%04x%04x",
			rand()&0xffff,rand()&0xffff,rand()&0xffff,rand()&0x0fff,rand()&0xffff,rand()&0xffff,rand()&0xffff,rand()&0xffff);
		return buffer;
	}
	else
	{
		output_error("global_guid(...): buffer too small");
		return NULL;
	}
}
char *global_run(char *buffer, int size)
{
	static char value[37]="";
	if ( value[0]=='\0' )
		global_guid(value,sizeof(value));
	if ( size>36 )
	{
		strcpy(buffer,value);
		return buffer;
	}
	else
		return NULL;
}
char *global_now(char *buffer, int size)
{
	if ( size>32 )
	{
		time_t now = time(NULL);
		struct tm *tmbuf = gmtime(&now);
		strftime(buffer,size,"%Y%m%d-%H%M%S",tmbuf);
		return buffer;
	}
	else
	{
		output_error("global_now(...): buffer too small");
		return NULL;
	}
}
char *global_true(char *buffer, int size)
{
	if ( size>1 )
		return strcpy(buffer,"1");
	else
	{
		output_error("global_now(...): buffer too small");
		return NULL;
	}
}

char *global_seq(char *buffer, int size, char *name)
{
	char seq[64], opt[64]="";
	if ( sscanf(name,"%63[^:]:%63s",seq,opt)==2 )
	{
		if ( strcmp(opt,"INIT")==0 )
		{
			if ( global_find(seq)!=NULL )
			{
				output_warning("global_seq(char *name='%s'): sequence is already initialized", seq);
				return NULL;
			}
			else
			{
				int32 *addr = (int32*)malloc(sizeof(int32));
				GLOBALVAR *var = global_create(seq,PT_int32,addr,PT_ACCESS,PA_PUBLIC,NULL);
				*addr = 0;
				return global_getvar(seq,buffer,size);
			}
		}
		else if ( strcmp(opt,"INC")==0 )
		{
			GLOBALVAR *var = global_find(seq);
			int32 *addr;
			if ( var==NULL || var->prop->ptype!=PT_int32 )
			{
				output_error("global_seq(char *name='%s'): sequence name is missing or not an int32 variable",name);
				return NULL;
			}
			addr = (int32*)var->prop->addr;
			(*addr)++;
			output_debug("updating global sequence '%s' to value '%d'", seq, *addr); 
			return global_getvar(seq,buffer,size);
		}
		else
		{
			output_error("global_seq(..., char *name='%s'): sequence spec '%s' is invalid",name,opt);
			return NULL;
		}
	}
	else
	{
		output_error("global_seq(..., char *name='%s'): sequence spec is invalid",name);
		return NULL;
	}
}

int global_isdefined(char *name)
{
	return global_find(name)!=NULL;
}

int parameter_expansion(char *buffer, int size, char *spec)
{
	char name[64], value[64], pattern[64], op[64], string[64]="", yes[1024]="1", no[1024]="0";
	int offset, length;
	int32 number;

	/* ${name:-value} */
	if ( sscanf(spec,"%63[^:]:-%63s",name,value)==2 )
	{	
		if ( global_getvar(name,buffer,size)==NULL )
			strncpy(buffer,value,size);
		return 1;
	}

	/* ${name:=value} */
	if ( sscanf(spec,"%63[^:]:=%63s",name,value)==2 )
	{
		if ( !global_isdefined(name) )
			global_setvar(name,value);
		global_getvar(name,buffer,size);
		return 1;
	}

	/* ${name:+value} */
	if ( sscanf(spec,"%63[^:]:+%63s",name,value)==2 )
	{
		if ( !global_isdefined(name) )
			strcpy(buffer,"");
		else
			strncpy(buffer,value,size);
		return 1;
	}

	/* ${name:offset:length} */
	if ( sscanf(spec,"%63[^:]:%d:%d",name,&offset,&length)==3 )
	{
		char temp[1024];
		if ( global_getvar(name,temp,sizeof(temp)-1) )
		{
			strncpy(buffer,temp+offset,size);
			buffer[length]='\0';
			return 1;
		}
		else
			return 0;
	}

	/* ${name:offset} */
	if ( sscanf(spec,"%63[^:]:%d",name,&offset)==2 )
	{
		char temp[1024];
		if ( global_getvar(name,temp,sizeof(temp)-1) )
		{
			strncpy(buffer,temp+offset,size);
			return 1;
		}
		else
			return 0;
	}

	/* ${name/offset/length} */
	if ( sscanf(spec,"%63[^/]/%63[^/]/%63s",name,pattern,string)>=2 )
	{
		char temp[1024], *ptr;
		size_t start;
		if ( global_getvar(name,temp,sizeof(temp)-1)==NULL )
			return 0;
		strcpy(buffer,"");
		ptr = strstr(temp,pattern);
		if ( ptr!=NULL )
		{
			start = ptr - temp;
			strncpy(buffer,temp,size);
			strncpy(buffer+start,string,size-start);
			strncpy(buffer+start+strlen(string),temp+start+strlen(pattern),size-start-strlen(string));
		}
		return 1;
	}

	/* ${name//offset/length} */
	if ( sscanf(spec,"%63[^/]//%63[^/]/%63s",name,pattern,string)==2 )
	{
		char temp[1024], *ptr=NULL;
		size_t start;
		if ( global_getvar(name,temp,sizeof(temp)-1)==NULL )
			return 0;
		strcpy(buffer,"");
		while ( true )
		{
			ptr = strstr(temp,pattern);
			if ( ptr==NULL )
				break;
			start = ptr - temp;
			strncpy(buffer,temp,size);
			strncpy(buffer+start,string,size-start);
			strncpy(buffer+start+strlen(string),temp+start+strlen(pattern),size-start-strlen(string));
			strncpy(temp,buffer,sizeof(temp));
		}
		return 1;
	}

	/* ${++name} */
	if ( sscanf(spec,"++%63s",name)==1 )
	{
		GLOBALVAR *var = global_find(name);
		int32 *addr;
		if ( var==NULL || var->prop->ptype!=PT_int32 )
			return 0;
		addr = var->prop->addr;
		sprintf(buffer,"%d",++(*addr));
		return 1;
	}

	/* ${--name} */
	if ( sscanf(spec,"--%63s",name)==1 )
	{
		GLOBALVAR *var = global_find(name);
		int32 *addr;
		if ( var==NULL || var->prop->ptype!=PT_int32 )
			return 0;
		addr = var->prop->addr;
		sprintf(buffer,"%d",--(*addr));
		return 1;
	}

	/* ${++name} */
	if ( sscanf(spec,"%63[^-+]%63[-+]",name,op)==2 )
	{
		GLOBALVAR *var = global_find(name);
		int32 *addr;
		if ( var==NULL || var->prop->ptype!=PT_int32 )
			return 0;
		addr = var->prop->addr;
		sprintf(buffer,"%d",(*addr));
		if ( strcmp(op,"++")==0 ) { (*addr)++; return 1; }
		else if ( strcmp(op,"--")==0 ) { (*addr)--; return 1; }
	}

	/* ${name op value} */
	if ( sscanf(spec,"%63[^!<>=&|~]%63[!<>=&|~]%d?%1023[^:]:%1023s",name,op,&number,yes,no)>=3 )
	{
		GLOBALVAR *var = global_find(name);
		if ( var!=NULL && var->prop->ptype==PT_int32 )
		{
			int32 *addr = var->prop->addr;
			if ( strcmp(op,"==")==0 ) { strcpy(buffer,(*addr==number)?yes:no); return 1; }
			else if ( strcmp(op,"!=")==0 || strcmp(op,"<>")==0 ) { strcpy(buffer,(*addr!=number)?yes:no); return 1; }
			else if ( strcmp(op,"<=")==0 ) { strcpy(buffer,(*addr<=number)?yes:no); return 1; }
			else if ( strcmp(op,"<")==0 ) { strcpy(buffer,(*addr<number)?yes:no); return 1; }
			else if ( strcmp(op,">=")==0 ) { strcpy(buffer,(*addr>=number)?yes:no); return 1; }
			else if ( strcmp(op,">")==0 ) { strcpy(buffer,(*addr>number)?yes:no); return 1; }
			else if ( strcmp(op,"&")==0 ) { strcpy(buffer,((*addr)&number)?yes:no); return 1; }
			else if ( strcmp(op,"|~")==0 ) { strcpy(buffer,((*addr)|~number)?yes:no); return 1; }
		}
	}

	/* ${name op= value} */
	if ( sscanf(spec,"%63[^-+/*%&^|~=]%63[-+/*%&^|~=]%d",name,op,&number)==3 )
	{
		GLOBALVAR *var = global_find(name);
		if ( var!=NULL && var->prop->ptype==PT_int32 )
		{
			int32 *addr = var->prop->addr;
			sprintf(buffer,"%d",(*addr));
			if ( strcmp(op,"+=")==0 ) { sprintf(buffer,"%d",(*addr)+=number); return 1; }
			if ( strcmp(op,"-=")==0 ) { sprintf(buffer,"%d",(*addr)-=number); return 1; }
			if ( strcmp(op,"*=")==0 ) { sprintf(buffer,"%d",(*addr)*=number); return 1; }
			if ( strcmp(op,"/=")==0 ) { sprintf(buffer,"%d",(*addr)/=number); return 1; }
			if ( strcmp(op,"%=")==0 ) { sprintf(buffer,"%d",(*addr)%=number); return 1; }
			if ( strcmp(op,"&=")==0 ) { sprintf(buffer,"%d",(*addr)&=number); return 1; }
			if ( strcmp(op,"|=")==0 ) { sprintf(buffer,"%d",(*addr)|=number); return 1; }
			if ( strcmp(op,"^=")==0 ) { sprintf(buffer,"%d",(*addr)^=number); return 1; }
			if ( strcmp(op,"&=~")==0 ) { sprintf(buffer,"%d",(*addr)&=~number); return 1; }
			if ( strcmp(op,"|=~")==0 ) { sprintf(buffer,"%d",(*addr)|=~number); return 1; }
			if ( strcmp(op,"^=~")==0 ) { sprintf(buffer,"%d",(*addr)^=~number); return 1; }
		}
	}

	/* ${name=number} */
	if ( sscanf(spec,"%63[A-Za-z0-9_:.]=%d",name,&number)==2 )
	{
		GLOBALVAR *var = global_find(name);
		int32 *addr;
		if ( var==NULL )
		{
				addr = (int32*)malloc(sizeof(int32));
				var = global_create(name,PT_int32,addr,PT_ACCESS,PA_PUBLIC,NULL);
		}
		else
			addr = var->prop->addr;
		*addr = number;
		sprintf(buffer,"%d",number);
		return 1;
	}

	/* ${name=string} */
	if ( sscanf(spec,"%63[A-Za-z0-9_:.]=%s",name,string)==2 )
	{
		global_setvar(name,string);
		strncpy(buffer,string,size);
		return 1;
	}

	return 0;
}

/** Get the value of a global variable in a safer fashion
	@return a \e char * pointer to the buffer holding the buffer where we wrote the data,
		\p NULL if insufficient buffer space or if the \p name was not found.

	This function searches global, user-defined, and module variables for a match.
**/
char *global_getvar(char *name, char *buffer, int size)
{
	char temp[1024];
	int len = 0;
	GLOBALVAR *var = NULL;
	struct {
		char *name;
		char *(*call)(char *buffer,int size);
	} map[] = {
		{"GUID",global_guid},
		{"NOW",global_now},
		{"RUN",global_run},
#if defined WIN32
		{"WINDOWS",global_true},
#elif defined __APPLE__
		{"APPLE",global_true},
#else
		{"LINUX",global_true},
#endif
	};
	int i;	
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

	/* special variables names */
	for ( i=0 ; i<sizeof(map)/sizeof(map[0]) ; i++ )
	{
		if ( strcmp(name,map[i].name)==0 )
			return map[i].call(buffer,size);
	}

	/* sequences */
	if ( strncmp(name,"SEQ_",4)==0 && strchr(name,':')!=NULL )
		return global_seq(buffer,size,name);

	/* expansions */
	if ( parameter_expansion(buffer,size,name) )
		return buffer;

	var = global_find(name);
	if(var == NULL)
	{
		/* try parameter expansion */
		if ( parameter_expansion(buffer,size,name) )
			return buffer;
		else
			return NULL;
	}
	len = class_property_to_string(var->prop, (void *)var->prop->addr, temp, sizeof(temp));
	if(len < size){ /* if we have enough space, copy to the supplied buffer */
		strncpy(buffer, temp, len+1);
		return buffer; /* wrote buffer, return ptr for printf funcs */
	}
	return NULL; /* NULL if insufficient buffer space */
}

size_t global_getcount(void)
{
	size_t count = 0;
	GLOBALVAR *var = NULL;
	while ( (var=global_getnext(var))!=NULL )
		count++;
	return count;
}

void global_dump(void)
{
	GLOBALVAR *var=NULL;
	int old = global_suppress_repeat_messages;
	global_suppress_repeat_messages = 0;
	while ((var=global_getnext(var))!=NULL)
	{
		char buffer[1024];
		if (class_property_to_string(var->prop, (void*)var->prop->addr,buffer,sizeof(buffer)))
			output_message("%s=%s;", var->prop->name, buffer);
	}
	global_suppress_repeat_messages = old;
}

/** threadsafe remote global read **/
void *global_remote_read(void *local, /** local memory for data (must be correct size for global */
						 GLOBALVAR *var) /** global variable from which to get data */
{
	int size = property_size(var->prop);
	void *addr = var->prop->addr;
	
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
			rlock(&var->lock);
			memcpy(local,addr,size);
			runlock(&var->lock);
			return local;
		}
	}
	else
	{
		/* @todo remote object read for multihost */
		return NULL;
	}
}
/** threadsafe remote global write **/
void global_remote_write(void *local, /** local memory for data */
						 GLOBALVAR *var) /** global variable to which data is written */
{
	int size = property_size(var->prop);
	void *addr = var->prop->addr;
	
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
			wlock(&var->lock);
			memcpy(addr,local,size);
			wunlock(&var->lock);
		}
	}
	else
	{
		/* @todo remote object write for multihost */
	}
}

/**@}**/
