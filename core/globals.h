/** $Id: globals.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file globals.h
	@addtogroup globals
 @{
 **/
#ifndef _GLOBALS_H
#define _GLOBALS_H

#include "version.h"
#include "class.h"

#ifdef _MAIN_C
#define GLOBAL 
#define INIT(A) = A
#else
#define GLOBAL extern
#define INIT(A)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FALSE
#define FALSE (0)
#define TRUE (!FALSE)
#endif

typedef enum {FAILED=FALSE, SUCCESS=TRUE} STATUS;

typedef struct s_globalvar {
	PROPERTY *prop;
	struct s_globalvar *next;
	uint32 flags;
	void (*callback)(char *); // this function will be called whenever the globalvar is set
} GLOBALVAR;

STATUS global_init(void);
GLOBALVAR *global_getnext(GLOBALVAR *previous);
GLOBALVAR *global_find(char *name);
GLOBALVAR *global_create(char *name, ...);
STATUS global_setvar(char *def,...);
char *global_getvar(char *name, char *buffer, int size);
void global_dump(void);

/* MAJOR and MINOR version */
GLOBAL unsigned global_version_major INIT(REV_MAJOR); /**< The software's major version */
GLOBAL unsigned global_version_minor INIT(REV_MINOR); /**< The software's minor version */

GLOBAL char global_command_line[1024]; /**< The current command-line */
GLOBAL char global_environment[1024] INIT("batch"); /**< The processing environment in use */
GLOBAL int global_quiet_mode INIT(FALSE); /**< The quiet mode flag */
GLOBAL int global_warn_mode INIT(TRUE); /**< The warning mode flag */
GLOBAL int global_debug_mode INIT(FALSE); /**< Enables the debugger */
GLOBAL int global_test_mode INIT(FALSE); /**< The test mode flag */
GLOBAL int global_verbose_mode INIT(FALSE); /**< The verbose mode flag */
GLOBAL int global_debug_output INIT(FALSE); /**< Enables debug output */
GLOBAL int global_keep_progress INIT(FALSE); /**< Flag to keep progress reports */
GLOBAL unsigned global_iteration_limit INIT(100); /**< The global iteration limit */
GLOBAL char global_workdir[1024] INIT("."); /**< The current working directory */
GLOBAL char global_dumpfile[1024] INIT("gridlabd.xml"); /**< The dump file name */
GLOBAL char global_savefile[1024] INIT(""); /**< The save file name */
GLOBAL int global_dumpall INIT(FALSE);	/**< Flags all modules to dump data after run complete */
GLOBAL int global_runchecks INIT(FALSE); /**< Flags module check code to be called after initialization */
/** @todo Set the threadcount to zero to automatically use the maximum system resources (tickets 180) */
GLOBAL int global_threadcount INIT(1); /**< the maximum thread limit, zero means automagically determine best thread count */
GLOBAL int global_profiler INIT(0); /**< Flags the profiler to process class performance data */
GLOBAL int global_pauseatexit INIT(0); /**< Enable a pause for user input after exit */
GLOBAL char global_testoutputfile[1024] INIT("test.txt"); /**< Specifies the test output file */
GLOBAL int global_xml_encoding INIT(8);  /**< Specifies XML encoding (default is 8) */
GLOBAL char global_pidfile[1024] INIT(""); /**< Specifies that a process id file should be created */
GLOBAL unsigned char global_no_balance INIT(FALSE);
GLOBAL char global_kmlfile[1024] INIT(""); /**< Specifies KML file to dump */
GLOBAL char global_modelname[1024] INIT(""); /**< Name of the current model */
GLOBAL char global_execdir[1024] INIT(""); /**< Path to folder containing installed application files */
GLOBAL bool global_strictnames INIT(true); /**< Enforce strict global naming (prevents globals from being implicitly created by assignment) */
GLOBAL bool global_xmlstrict INIT(false); /**< Causes XML I/O to use strict XML data structures */
GLOBAL int global_relax_naming_rules INIT(0); /**< Causes the error to relax to a warning when object names start with numbers or special characters */
GLOBAL char global_urlbase[1024] /**< default urlbase used for online resources */
#ifdef _DEBUG
	INIT("./");
#else
	INIT("http://www.gridlabd.org/"); 
#endif
GLOBAL unsigned int global_randomseed INIT(0); /**< random number seed (default 0 means true randomization, non-zero means deterministic random state) */
GLOBAL char global_include[1024] 
#ifdef WIN32
	INIT(""); 
#else
	INIT("");
#endif /**< include path for models and code headers */
GLOBAL int global_gdb INIT(0); /**< select gdb debugger */
GLOBAL char global_trace[1024] INIT(""); /**< comma separate list of runtime calls that will be traced */
GLOBAL int global_gdb_window INIT(0); /**< start gdb in a separate window */
GLOBAL int global_process_id INIT(0); /**< the main process id */
GLOBAL char global_execname[1024] INIT(""); /**< the main program full path */
GLOBAL char global_tmp[1024] /**< location for temp files */
#ifdef WIN32
							INIT("C:\\WINDOWS\\TEMP"); 
#else
							INIT("/tmp"); 
#endif
GLOBAL int global_force_compile INIT(0); /** flag to force recompile of GLM file even when up to date */
GLOBAL int global_nolocks INIT(0); /** flag to disable memory locking */
GLOBAL int global_forbid_multiload INIT(0); /** flag to disable multiple GLM file loads */
GLOBAL int global_skipsafe INIT(0); /** flag to allow skipping of safe syncs (see OF_SKIPSAFE) */
typedef enum {DF_ISO=0, DF_US=1, DF_EURO=2} DATEFORMAT;
GLOBAL int global_dateformat INIT(DF_ISO); /** date format (ISO=0, US=1, EURO=2) */

#include "timestamp.h"
#include "realtime.h"

GLOBAL TIMESTAMP global_clock INIT(TS_ZERO); /**< The main clock timestamp */
GLOBAL TIMESTAMP global_starttime INIT(0); /**< The simulation starting time */
GLOBAL TIMESTAMP global_stoptime INIT(TS_NEVER); /**< The simulation stop time */

GLOBAL char global_double_format[32] INIT("%+lg"); /**< the format to use when processing real numbers */
GLOBAL char global_complex_format[256] INIT("%+lg%+lg%c"); /**< the format to use when processing complex numbers */
GLOBAL char global_object_format[32] INIT("%s:%d"); 
GLOBAL char global_object_scan[32] INIT("%[^:]:%d"); /**< the format to use when scanning for object ids */

GLOBAL int global_minimum_timestep INIT(1); /**< the minimum timestep allowed */
GLOBAL int global_maximum_synctime INIT(60); /**< the maximum time allotted to any single sync call */

GLOBAL char global_platform[8] /**< the host operating platform */

#ifdef WIN32
	INIT("WINDOWS");
#else
	INIT("LINUX");
#endif

GLOBAL int global_suppress_repeat_messages INIT(1); /**< flag that allows repeated messages to be suppressed */
GLOBAL int global_suppress_deprecated_messages INIT(0); /**< flag to suppress output notice of deprecated properties usage */

GLOBAL int global_run_realtime INIT(0); /**< flag to force simulator into realtime mode */

#ifdef _DEBUG
GLOBAL char global_sync_dumpfile[1024] INIT(""); /**< enable sync event dump file */
#endif

GLOBAL int global_streaming_io_enabled INIT(0); /**< flag to enable compact streams instead of XML or GLM */

GLOBAL int global_nondeterminism_warning INIT(0); /**< flag to enable nondeterminism warning (use of rand when multithreading */
GLOBAL int global_compileonly INIT(0); /**< flag to enable compile-only option (does not actually start the simulation) */

GLOBAL int global_server_portnum INIT(6267); /**< port used in server mode (assigned by IANA Dec 2010) */
GLOBAL char global_browser[1024] /**< default browser to use for GUI */
#ifdef WIN32
	INIT("iexplore"); 
#else
	INIT("firefox"); 
#endif
GLOBAL int global_server_quit_on_close INIT(0); /** server will quit when connection is closed */

GLOBAL int technology_readiness_level INIT(0); /**< the TRL of the model (see http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Technology_Readiness_Levels) */

GLOBAL int global_show_progress INIT(1);

/* checkpoint globals */
typedef enum {CPT_NONE=0, CPT_WALL=1, CPT_SIM=2} CHECKPOINTTYPE;
GLOBAL int global_checkpoint_type INIT(CPT_NONE); /**< checkpoint type determines whether and how checkpoints are used */
GLOBAL char global_checkpoint_file[1024] INIT(""); /**< checkpoint file name is base name used for checkpoint save files */
GLOBAL int global_checkpoint_seqnum INIT(0); /**< checkpoint sequence file number */
GLOBAL int global_checkpoint_interval INIT(0); /** checkpoint interval (default is 3600 for CPT_WALL and 86400 for CPT_SIM */
GLOBAL int global_checkpoint_keepall INIT(0); /** determines whether all checkpoint files are kept, non-zero keeps files, zero delete all but last */

/* version check */
GLOBAL int global_check_version INIT(0); /**< check version flag */

#ifdef __cplusplus
}
#endif

#undef GLOBAL
#undef INIT

#endif /* _GLOBAL_H */
/**@}**/
