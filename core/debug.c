/** $Id: debug.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file debug.c
	@author David P. Chassin
	@addtogroup debug Debugger
	@ingroup core

	The debugger supports two methods of interrupting the simulation.  Breakpoints
	halt the simulator and start the debugger whenever a situation arises that
	matches the breakpoint criterion.  For example, a breakpoint on the bottom-up
	pass will stop the simulation every time an object sync is called during a
	bottom-up pass.

	Watchpoints are different from breakpoints in that the debugger is only stopped
	when the value being watched changes.  For example, a breakpoint on node:12 voltage
	would only stop the simulation when that value is changed.  In contrast, a breakpoint
	on node:12 would stop each time node:12 is sync'd.

	While the debugger is running \p help will print a list of all the available commands.

	@section debug_start Getting started

	To start the debugger you must include the \p --debug option on the command-line.  Note
	that while the debugger is running, the system will only operate in single-threaded
	mode.

	Each time the debugger stops to prompt for input, it displays the current simulation
	time and simulator status.  The status include which pass is currently running
	(see PASSCONFIG), which rank is being processed, which object is about to be updated,
	and which iteration is being run (if the time has not advanced yet).

	@verbatim
	DEBUG: time 2000-01-01 00:00:00 PST
	DEBUG: pass BOTTOMUP, rank 0, object link:14, iteration 1
	GLD>
	@endverbatim

	Debugging commands may be abbreviated to the extent that they are unambiguous.  For
	example \p b may be used instead of \p break, but \p wa must be used for \p watch to
	distinguish it from \p where.

	@subsection debug_list Listing objects

	To obtain a list of objects loaded, you may use the \p list command:

	@verbatim
	GLD> list
	A-b---    2 INIT                     node:0           ROOT
	A-b---    1 INIT                     node:1           node:0
	A-b---    1 INIT                     node:2           node:0
	A-b---    1 INIT                     node:3           node:0
	A-b---    0 INIT                     link:4           node:1
	A-b---    0 INIT                     link:5           node:2
	A-b---    0 INIT                     link:6           node:2
	A-b---    0 INIT                     link:7           node:3
	A-b---    0 INIT                     link:8           node:3	
	GLD>
	@endverbatim

	You may limit the list to only the object of a particular class:

	@verbatim
	GLD> list node
	A-b---    2 INIT                     node:0           ROOT
	A-b---    1 INIT                     node:1           node:0
	A-b---    1 INIT                     node:2           node:0
	A-b---    1 INIT                     node:3           node:0
	GLD>
	@endverbatim

	The first column contains flags indicating the status of the 
	object.  In the first character:
	- \p A indicates the object is \e active (operating)
	- \p P indicates the object is \e planned (not yet operating)
	- \p R indicates the object is \e retired (no longer operating)
	In the second character:
	- \p - indicates that the object is not called on the PRETOPDOWN pass
	- \p t indicates that the object has yet to be called on the PRETOPDOWN pass
	- \p T indicates that the object has already been called on the PRETOPDOWN pass
	In the third character:
	- \p - indicates that the object is not called on the BOTTOMUP pass
	- \p b indicates that the object has yet to be called on the BOTTOMUP pass
	- \p B indicates that the object has already been called on the BOTTOMUP pass
	In the fourth character:
	- \p - indicates that the object is not called on the POSTTOPDOWN pass
	- \p t indicates that the object has yet to be called on the POSTTOPDOWN pass
	- \p T indicates that the object has already been called on the POSTTOPDOWN pass
	In the fifth character:
	- \p - indicates the object is unlocked
	- \p l indicates the object is locked
	In the sixth character:
	- \p - indicates the object's native PLC code is enabled
	- \p x indicates the object's natice PLC code is disabled

	The second field is the object's rank.

	The third field is the object's internal clock (or \p INIT) if the object
	has not yet been sync'd.

	The fourth field is the name (\p class:\p id) of the object.

	The fifth field is the name of the object's parent (or \p ROOT) if is has none.

	@subsection debug_print

	To inspect the properties of an object, you can use the \p print command.  With
	no option, the current object is printed:

	@verbatim
	GLD> print
	DEBUG: object link:5 {
			parent = node:2
			rank = 0;
			clock = 0 (0);
			complex Y = +10-1j;
			complex I = +0+0j;
			double B = +0;
			object from = node:0;
			object to = node:2;
	}
	GLD>	
	@endverbatim

	When an object name (\p class:\p id) is provided, that object is printed:

	@verbatim
	GLD> print node:0
	DEBUG: object node:0 {
			root object
			rank = 2;
			clock = 0 (0);
			latitude = 49N12'34.0";
			longitude = 121W15'48.3";
			complex V = +1-0d;
			complex S = +0+0j;
			double G = +0;
			double B = +0;
			double Qmax_MVAR = +0;
			double Qmin_MVAR = +0;
			enumeration type = 3;
			int16 bus_id = 0;
			char32 name = Feeder;
			int16 flow_area_num = 1;
			complex Vobs = +0+0d;
			double Vstdev = +0;
	}
	GLD>
	@endverbatim

	@subsection debug_script Scripting commands

	You can run a script containing debug commands using the \p script command:

	@verbatim
	GLD> sys copy con: test.scr
	wa node:0
	run
	^Z
			1 file(s) copied.
	GLD> script test.scr
	DEBUG: resuming simulation, Ctrl-C interrupts
	DEBUG: watchpoint 0 stopped on object node:0
	DEBUG: object node:0 {
			root object
			rank = 2;
			clock = 2000-01-01 00:00:00 PST (946713600);
			latitude = 49N12'34.0";
			longitude = 121W15'48.3";
			complex V = +1-0d;
			complex S = +0.522519+0.0522519j;
			double G = +0;
			double B = +0;
			double Qmax_MVAR = +0;
			double Qmin_MVAR = +0;
			enumeration type = 3;
			int16 bus_id = 0;
			char32 name = Feeder;
			int16 flow_area_num = 1;
			complex Vobs = +0+0d;
			double Vstdev = +0;
	}

	DEBUG: watchpoint 1 stopped on object node:0
	DEBUG: object node:0 {
			root object
			rank = 2;
			clock = 2000-01-01 00:00:00 PST (946713600);
			latitude = 49N12'34.0";
			longitude = 121W15'48.3";
			complex V = +1-0d;
			complex S = +0.522519+0.0522519j;
			double G = +0;
			double B = +0;
			double Qmax_MVAR = +0;
			double Qmin_MVAR = +0;
			enumeration type = 3;
			int16 bus_id = 0;
			char32 name = Feeder;
			int16 flow_area_num = 1;
			complex Vobs = +0+0d;
			double Vstdev = +0;
	}

	DEBUG: time 2000-01-01 00:00:00 PST
	DEBUG: pass BOTTOMUP, rank 2, object node:0, iteration 5
	GLD>	
	@endverbatim
 @{ strsignal
 **/

#include <signal.h>
#include <ctype.h>
#include <string.h>

#include "platform.h"
#include "output.h"
#include "exec.h"
#include "class.h"
#include "convert.h"
#include "object.h"
#include "index.h"
#include "realtime.h"
#include "module.h"
#include "debug.h"
#include "kill.h"

#ifndef _MAX_PATH
#define _MAX_PATH 1024
#endif

/* these are defined in exec.c */
extern const PASSCONFIG passtype[];
extern int iteration_counter;

/* these are the signals that will trigger the debugger to start */
int siglist[] = {SIGABRT, SIGINT, SIGFPE, SIGSEGV, SIGTERM};

static char *get_objname(OBJECT *obj)
{
	static char buf[1024];
	if (obj->name) return obj->name;
	sprintf(buf,"%s:%d", obj->oclass->name, obj->id);
	return buf;
}

/*****************************************************************************************/
static int debug_active = 1; /**< flag indicating that the debugger is currently active */
static int error_caught = 0; /**< flag indicating that the debugger has seen an error */
static int watch_sync = 0; /**< flag indicating that sync times should be reported */
static int sigint_caught = 0; /**< flag indicating that \p SIGINT has been caught */
static int sigterm_caught = 0; /**< flag indicating that \p SIGTERM has been caught */
static int list_details = 0; /**< flag indicating that listing includes details */
static int list_unnamed = 1; /**< flag indicating that listing includes unnamed objects */
static int list_inactive = 1; /**< flag indicating that listing includes inactive objects */
static int list_sync = 1; /**< flag indicating that listing includes objects that have syncs */

static STATUS exec(char *format,...)
{
	char cmd[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(cmd,format,ptr);
	va_end(ptr);
	output_debug("Running '%s'", cmd);
	return system(cmd)==0?SUCCESS:FAILED;
}

void debug_notify_error(void)
{
	error_caught = 1;
}

/** The exec_sighandler() function is called whenever a \p SIGINT signal
	is caught by the main application.  When \p global_debug_mode
	is 0, this causes the main simulation loop to halt.  When
	\p global_debug_mode is non-zero, it causes the debugger to
	be activated before the beginning of the next sync call is made.
 **/
void exec_sighandler(int sig) /**< the signal number, see \p <signal.h> */ 
{
	output_debug("signal %s caught", strsignal(sig));
	if (sig==SIGTERM)
	{
		output_error("received SIGTERM; exiting now");
		/* TROUBLESHOOT
			A signal from the operating system was caught, which halts
			the current simulation.  This is not normal, but may be intentional.
			If it is not intentional, make sure that the signal is not being
			raised accidentally by another application.
		 */
		exit(XC_SIGNAL|SIGTERM);
	}
	else if (sig==SIGABRT)
	{
		output_error("received SIGABRT; exiting now");
		/* TROUBLESHOOT
			A signal from the operating system was caught, which aborts
			the current simulation.   This is not normal, but may be intentional.
			If it is not intentional, make sure that the signal is not being
			raised accidentally by another application.
		 */
		exit(XC_SIGNAL|SIGABRT);
	}
	else if (sig==SIGINT)
	{
		sigint_caught=1;
		if (global_debug_mode)
		{
			if (debug_active==0)
			{
				output_debug("debugger activated");
				debug_active=1;
			}
			else
			{
				output_debug("debugger activation failed, signal processing disabled");
				debug_active=1;
				signal(sig,NULL);
			}
		}
		else
			exec_setexitcode(XC_SIGINT);
	}
	else
		output_error("ignored signal %d", sig);
		/* TROUBLESHOOT
			A signal was received that is not handled.  This is not serious, but
			may indicate that a behavior was expected by another application that
			GridLAB-D does not support.  If this is unexpected, make sure that 
			the signal is not being	raised accidentally by another application.
		*/
}

#ifdef WIN32
char *strsignal(int sig)
{
	switch(sig) {
	case SIGABRT: return "SIGABRT";
	case SIGFPE: return "SIGFPE";
	case SIGILL: return "SIGILL";
	case SIGINT: return "SIGINT";
	case SIGSEGV: return "SIGSEGV";
	case SIGTERM: return "SIGTERM";
	default: return "SIGUNKNOWN";
	}
}
#endif

typedef enum {DBG_QUIT,DBG_RUN,DBG_NEXT} DEBUGCMD; /**< commands that are automatically repeated when a \p NULL command is entered */
typedef enum {BP_MODULE, BP_CLASS, BP_OBJECT, BP_PASS, BP_RANK, BP_TIME, BP_CLOCK, BP_ERROR} BREAKPOINTTYPE; /**< breakpoint types that are supported */
typedef struct s_breakpoint {
	BREAKPOINTTYPE type; /**< the breakpoint type for this entry */
	int enabled; /**< flag where the breakpoint is enabled */
	int num; /**< breakpoint id number */
	union {
		MODULE *mod; /**< criteria for a module breakpoint */
		CLASS *oclass; /**< criteria for a class breakpoint */
		OBJECT *obj; /**< criteria for an object breakpoint */
		int64 pass; /**< criteria for a pass number breakpoint */
		int64 rank; /**< criteria for a rank number breakpoint */
		TIMESTAMP ts; /**< criteria for a timestamp breakpoint */
		int64 data; /**< type-neutral criteria */ 
	}; /**< breakpoint criteria */
	struct s_breakpoint *next; /**< the next breakpoint in the breakpoint list */
} BREAKPOINT; /**< the structure for a breakpoint entry */
static BREAKPOINT *first_breakpoint=NULL, /**< pointer to the first breakpoint */
	*last_breakpoint=NULL; /**< pointer to the last breakpoint */
static int breakpoint_count=0; /**< the number of breakpoints defined so far */

/** This function adds a breakpoint to the list of breakpoints that examined by the debugger 
 **/
static int exec_add_breakpoint(BREAKPOINTTYPE type, /**< the breakpoint type */
							   int64 data) /**< the breakpoint type-neutral data */
{
	BREAKPOINT *bp = (BREAKPOINT*)malloc(sizeof(BREAKPOINT));
	if (bp==NULL)
	{
		output_error("exec_add_breakpoint() - memory allocation failed");
		/* TROUBLESHOOT
			The <b>add breakpoint</b> debugger command failed because the
			system ran out of memory.  Follow the usual procedure for freeing
			up memory before trying again.
		 */
		return 0;
	}
	bp->type = type;
	bp->data = data;
	bp->enabled = 1;
	bp->num = breakpoint_count++;
	bp->next = NULL;
	if (last_breakpoint!=NULL) 
		last_breakpoint->next=bp;
	else 
		first_breakpoint=bp;
	last_breakpoint = bp;
	return 1;
}

typedef struct s_watchpoint {
	int enabled; /**< flag whether the watchpoint is enabled */
	int num; /**< the watchpoint id number */
	OBJECT *obj; /**< the object watched, if any */
	PROPERTY *prop; /**< the property being watched, if any */
	char buffer[4096]; /**< the current value being watched */
	struct s_watchpoint *next; /**< the next watchpoint in the watchpoint list */
} WATCHPOINT; /**< the structure for a watchpoint entry */
static WATCHPOINT *first_watchpoint=NULL, /**< a pointer to the first watchpoint */
	*last_watchpoint=NULL; /**< a pointer to the last watchpoint */
static int watchpoint_count=0; /**< the number of watchpoints defined so far */

/** This function adds a watchpoint to the watchpoint list 
 **/
static int exec_add_watchpoint(OBJECT *obj, /**< the object being watched */
							   PROPERTY *prop) /**< the property being watched */
{
	WATCHPOINT *wp = (WATCHPOINT*)malloc(sizeof(WATCHPOINT));
	char buffer[1024];
	if (wp==NULL)
	{
		output_error("exec_add_watchpoint() - memory allocation failed");
		/* TROUBLESHOOT
			The <b>add breakpoint</b> debugger command failed because the
			system ran out of memory.  Follow the usual procedure for freeing
			up memory before trying again.
		 */
		return 0;
	}
	wp->enabled = 1;
	wp->num = watchpoint_count++;
	wp->obj = obj;
	wp->prop = prop;
	if (prop==NULL)
		object_dump(wp->buffer,sizeof(wp->buffer),obj);
	else
		strcpy(wp->buffer,object_property_to_string(obj,prop->name, buffer, 1023));
	wp->next = NULL;
	if (last_watchpoint!=NULL) 
		last_watchpoint->next=wp;
	else 
		first_watchpoint=wp;
	last_watchpoint = wp;
	return 1;
}

static void list_object(OBJECT *obj, PASSCONFIG pass)
{
	char details[132] = "";
	char buf1[64],buf2[64],buf3[64];
	if (list_unnamed==0 && obj->name==NULL)
		return;
	if (list_inactive==0 && (global_clock<obj->in_svc || global_clock>obj->out_svc))
		return;
	if (list_sync==0 && obj->oclass->sync==NULL)
		return;
	if (list_details)
	{
		char valid_to[64] = "";
		convert_from_timestamp(obj->valid_to,valid_to,sizeof(valid_to));
		sprintf(details,"%s %c%c%c %s/%s/%d ", valid_to,
			obj->flags&OF_RECALC?'c':'-', obj->flags&OF_RERANK?'r':'-', obj->flags&OF_FOREIGN?'f':'-',
			obj->oclass->module->name, obj->oclass->name, obj->id);
	}
	output_message("%1s%1s%1s%1s%1s%1s %4d %-24s %-16s %-16s %s", 
		global_clock<obj->in_svc?"P":(global_clock<obj->out_svc?"A":"R"), /* service status: P=planned, A=active, R=retired */
		(obj->oclass->passconfig&PC_PRETOPDOWN)?(pass<PC_PRETOPDOWN?"t":"T"):"-", /* pre-topdown sync status */
		(obj->oclass->passconfig&PC_BOTTOMUP)?(pass<PC_BOTTOMUP?"b":"B"):"-", /* bottom-up sync status */
		(obj->oclass->passconfig&PC_POSTTOPDOWN)?(pass<PC_POSTTOPDOWN?"t":"T"):"-", /* post-topdown sync status */
		(obj->flags&OF_LOCKED)==OF_LOCKED?"l":"-", /* object lock status */
		(obj->flags&OF_HASPLC)==OF_HASPLC?"x":"-", /* object PLC status */
		obj->rank, 
		obj->clock>0?(convert_from_timestamp(obj->clock,buf3,sizeof(buf3))?buf3:"(error)"):"INIT",
		obj->name?obj->name:convert_from_object(buf1,sizeof(buf1),&obj,NULL)?buf1:"(error)",
		obj->parent?(obj->parent->name?obj->parent->name:convert_from_object(buf2,sizeof(buf2),&(obj->parent),NULL)?buf2:"(error)"):"ROOT",
		details);
}

FILE *load_fp; /**< the file from which commands are read (see \p script) */
int load_from_file = 0; /** flag whether commands are being read from a \p script file (see #load_fp) */

/** The function executes the next debug command provided by the user or read from a file
 **/
DEBUGCMD exec_debug_cmd(struct sync_data *data, /**< the current sync status of the mail loop */
						int pass, /**< the current pass on the main loop */
						int index, /**< the rank index */
						OBJECT *obj) /**< the current object being processed */
{
	static DEBUGCMD last = DBG_NEXT;
	char tmp[64];
	char* read_val;
	
	output_debug("time %s\r", convert_from_timestamp(global_clock,tmp,sizeof(tmp))?tmp:"(invalid)");
	output_debug("pass %s, rank %d, object %s, iteration %d", 
		passtype[pass]==PC_PRETOPDOWN?"PRETOPDOWN":(passtype[pass]==PC_BOTTOMUP?"BOTTOMUP":(passtype[pass]==PC_POSTTOPDOWN?"POSTTOPDOWN":"(invalid)")),
		index, get_objname(obj),
		global_iteration_limit-iteration_counter+1);
	{
		extern void (*notify_error)(void);
		if (notify_error != NULL)
			output_debug("output errors are trapped");
	}
	while (1)
	{
		char cmd[32]=""; 
		char buffer[1024]="";
		int i;
Retry:
		/* add signals to signal handler */
		for (i=0; i<sizeof(siglist)/sizeof(siglist[0]); i++)
			signal(siglist[i],exec_sighandler);

		if(load_from_file == 0){
			fprintf(stdout,"GLD> ");
			sigint_caught=0;
			fflush(stdout);
		
			gets(buffer);	/* "gets() is dangerous and should not be used" -gcc */
			output_verbose("debug command '%s'", buffer);
		}
		else{ /* Load from file */
			char* nl;
			read_val = fgets(buffer,1024,load_fp);
			
			/* check the returned value.  If NULL, drop out of file load mode */
			if(read_val == NULL)
			{
				load_from_file = 0;
				fclose(load_fp);
				goto Retry;
			}
			/* Strip out the newline characters */
			nl = strrchr(buffer,'\r');
			if(nl)
				*nl = '\0';
			nl = strrchr(buffer,'\n');
			if(nl)
				*nl = '\0';
		}
		sscanf(buffer,"%s",cmd);
		if (sigint_caught==1)
		{
			fprintf(stdout,"<Ctrl-C>\n");
			goto Retry;
		}
		if (strncmp(cmd,"quit",max(1,strlen(cmd)))==0)
			return DBG_QUIT;
		else if (strncmp(cmd,"run",max(1,strlen(cmd)))==0 || (strlen(cmd)==0&&last==DBG_RUN))
		{
			output_debug("resuming simulation, Ctrl-C interrupts");
			debug_active=0;
			return last = DBG_RUN;
		}
		else if (strncmp(cmd,"next",max(1,strlen(cmd)))==0 || (strlen(cmd)==0&&last==DBG_NEXT))
		{
			return last = DBG_NEXT;
		}
		else if (strncmp(cmd,"module",max(1,strlen(cmd)))==0)
		{
			char modname[128];
			
			if(sscanf(buffer,"%*s %s", modname)){
				output_message("Loading module %s...", modname);
				module_load(modname, 0, NULL);
			} else {
				MODULE *mod = module_get_first();
				output_message("Loaded modules:");
				while(mod != NULL){
					output_message(" * %s v%i.%i", mod->name, mod->major, mod->minor);
					mod = mod->next;
				}
			}
		}
		else if (strncmp(cmd,"namespace",max(2,strlen(cmd)))==0)
		{
			char space[1024];
			if (sscanf(buffer,"%*s %s", space)==0)
			{
				object_namespace(space,sizeof(space));
				output_debug("%s",space[0]=='\0'?space:"(global)");
			}
			else if (!object_select_namespace(space))
				output_debug("unable to select namespace '%s'", space);
		}
		else if (strncmp(cmd,"list",max(1,strlen(cmd)))==0)
		{
			char lclass[256]="";
			OBJECT *obj = object_get_first();
			sscanf(buffer,"%*s %s", lclass);
			/* output_message("%-16.16s %4s %6s %-16.16s %-32.32s",	"Object name","Rank","Status","Parent","Clock");
			   output_message("%-16.16s %4s %6s %-16.16s %-32.32s",	"----------------","----","------","----------------","--------------------------------");
			 */
			/* NOTE ~ this has been copy/paste-ed for "find" as well! */
			for (obj=object_get_first(); obj!=NULL; obj=obj->next) 
			{
				if (lclass[0]=='\0' || strcmp(lclass,obj->oclass->name)==0)
					list_object(obj,pass);
			}
		}
		else if (strncmp(cmd,"details",max(1,strlen(cmd)))==0)
		{
			char cmd[1024];
			int n = sscanf(buffer,"%*s %[^\0]", cmd);
			if (n==1)
			{
				if (strcmp(cmd,"on")==0)
					list_details = 1;
				else if (strcmp(cmd,"off")==0)
					list_details = 0;
				else
					output_error("'%s' is an invalid option to details command", cmd);
					/* TROUBLESHOOT
						The <b>details</b> debugger command was given an option
						that isn't valid.  Check the syntax and try again.
					 */
			}
			else if (n==0)
				output_debug("Details are %s", list_details?"on":"off");
			else
				output_error("details command syntax error");
				/* TROUBLESHOOT
					The <b>details</b> debugger command support only one option,
					but somehow more than one option was read.  Check the syntax
					and try again.
				 */
		}
		else if (strncmp(cmd,"inactive",max(1,strlen(cmd)))==0)
		{
			char cmd[1024];
			int n = sscanf(buffer,"%*s %[^\0]", cmd);
			if (n==1)
			{
				if (strcmp(cmd,"on")==0)
					list_inactive = 1;
				else if (strcmp(cmd,"off")==0)
					list_inactive = 0;
				else
					output_error("'%s' is an invalid option to inactive command", cmd);
					/* TROUBLESHOOT
						The <b>inactive</b> debugger command was given an option
						that isn't valid.  Check the syntax and try again.
					 */
			}
			else if (n==0)
				output_debug("Inactive objects are %s", list_details?"on":"off");
			else
				output_error("inactive command syntax error");
				/* TROUBLESHOOT
					The <b>inactive</b> debugger command support only one option,
					but somehow more than one option was read.  Check the syntax
					and try again.
				 */
		}
		else if (strncmp(cmd,"unnamed",max(1,strlen(cmd)))==0)
		{
			char cmd[1024];
			int n = sscanf(buffer,"%*s %[^\0]", cmd);
			if (n==1)
			{
				if (strcmp(cmd,"on")==0)
					list_unnamed = 1;
				else if (strcmp(cmd,"off")==0)
					list_unnamed = 0;
				else
					output_error("'%s' is an invalid option to unnamed command", cmd);
					/* TROUBLESHOOT
						The <b>unnamed</b> debugger command was given an option
						that isn't valid.  Check the syntax and try again.
					 */
			}
			else if (n==0)
				output_debug("Unnamed objects are %s", list_details?"on":"off");
			else
				output_error("unnamed command syntax error");
				/* TROUBLESHOOT
					The <b>unnamed</b> debugger command support only one option,
					but somehow more than one option was read.  Check the syntax
					and try again.
				 */
		}
		else if (strncmp(cmd,"nsync",max(2,strlen(cmd)))==0)
		{
			char cmd[1024];
			int n = sscanf(buffer,"%*s %[^\0]", cmd);
			if (n==1)
			{
				if (strcmp(cmd,"on")==0)
					list_sync = 1;
				else if (strcmp(cmd,"off")==0)
					list_sync = 0;
				else
					output_error("'%s' is an invalid nsync command", cmd);
					/* TROUBLESHOOT
						The <b>nsync</b> debugger command was given an option
						that isn't valid.  Check the syntax and try again.
					 */
			}
			else if (n==0)
				output_debug("Sync objects are %s", list_details?"on":"off");
			else
				output_error("nsync command syntax error");
				/* TROUBLESHOOT
					The <b>nsync</b> debugger command support only one option,
					but somehow more than one option was read.  Check the syntax
					and try again.
				 */
		}
		else if (strncmp(cmd,"script",max(1,strlen(cmd)))==0)
		{
			char load_filename[_MAX_PATH+32]; /* Made it a little longer than max_path to be safe */
			if(sscanf(buffer,"%*s %[^\x20]",load_filename) <=0) /* use \n since we are using fgets to load buffer */
			{
				output_error("missing file name for script command");
				/* TROUBLESHOOT
					The <b>script</b> command requires a filename, but non was
					provided in the command.  Identify the file and try again.
				 */
			}
			load_fp = fopen(load_filename,"r");
			if(load_fp == NULL)
			{
				output_error("unable to open file %s for reading",load_filename);
				/* TROUBLESHOOT
					The file specified is not available.  Make sure the filename
					is correct and has the required access rights and try again.
				 */
			}
			load_from_file = 1;
		}
		else if (strncmp(cmd,"system",max(2,strlen(cmd)))==0)
		{
			char cmd[1024];
			if (sscanf(buffer,"%*s %[^\0]", cmd)==1)
				system(cmd);
#ifdef WIN32
			else if (getenv("COMSPEC")!=NULL)
				system(getenv("COMSPEC"));
			else
				system("cmd");
#else
			else if (getenv("SHELL")!=NULL)
				system(getenv("SHELL"));
			else
				system("/bin/sh");
#endif
		}
		else if (strncmp(cmd,"break",max(1,strlen(cmd)))==0)
		{
			char bptype[256]="";
			char bpval[256]="";
			if (sscanf(buffer,"%*s %s %[^\0]", bptype, bpval)==0)
			{
				/* display all breakpoints */
				BREAKPOINT *bp;
				if (last_breakpoint==NULL)
					output_debug("no breakpoints");
				for (bp=first_breakpoint; bp!=NULL; bp=bp->next)
				{
					char tmp[64];
					switch (bp->type) {
					case BP_MODULE:
						output_debug("breakpoint %2d - module %s %s", bp->num, bp->mod->name, bp->enabled?"":"(disabled)");
						break;
					case BP_CLASS:
						output_debug("breakpoint %2d - class %s %s", bp->num, bp->oclass->name, bp->enabled?"":"(disabled)");
						break;
					case BP_OBJECT:
						output_debug("breakpoint %2d object %s %s", bp->num, get_objname(bp->obj), bp->enabled?"":"(disabled)");
						break;
					case BP_PASS:
						output_debug("breakpoint %2d pass %s %s", bp->num, (int)bp->pass==PC_PRETOPDOWN?"pretopdown":((int)bp->pass==PC_BOTTOMUP?"bottomup":((int)bp->pass==PC_POSTTOPDOWN?"posttopdown":"(invalid)")), bp->enabled?"":"(disabled)");
						break;
					case BP_RANK:
						output_debug("breakpoint %2d rank %d %s", bp->num, (int)bp->rank, bp->enabled?"":"(disabled)");
						break;
					case BP_TIME:
						output_debug("breakpoint %2d time %s (%"FMT_INT64"d) %s", bp->num, convert_from_timestamp(bp->ts,tmp,sizeof(tmp))?tmp:"(invalid)", bp->ts, bp->enabled?"":"(disabled)");
						break;
					case BP_CLOCK:
						output_debug("breakpoint %2d clock %s", bp->num, bp->enabled?"":"(disabled)");
						break;
					case BP_ERROR:
						output_debug("breakpoint %2d on error", bp->num, bp->enabled?"":"(disabled)");
						break;
					default:
						break;
					}
				}
			}
			else if (strncmp(bptype,"error",strlen(bptype))==0)
			{
				if (!exec_add_breakpoint(BP_ERROR,0))
					output_error("unable to add error breakpoint");
					/* TROUBLESHOOT
						The <b>break error</b> could not be completed because of a 
						problem with the internal operation needed to perform it.
						Follow the guidance for the message which precedes this message
						and try again.
					 */
				error_caught = 0;
				output_notify_error(debug_notify_error);
			}
			else if (strncmp(bptype,"clock",strlen(bptype))==0)
			{
				if (!exec_add_breakpoint(BP_CLOCK,global_clock))
					output_error("unable to add clock breakpoint");
					/* TROUBLESHOOT
						The <b>break clock</b> could not be completed because of a 
						problem with the internal operation needed to perform it.
						Follow the guidance for the message which precedes this message
						and try again.
					 */
			}
			else if (strncmp(bptype,"object",strlen(bptype))==0)
			{	/* create object breakpoint */
				OBJECT *obj = object_find_name(bpval);
				if (obj!=NULL || convert_to_object(bpval,&obj,NULL))
				{
					if (!exec_add_breakpoint(BP_OBJECT,(int64)obj)) /* warning: cast from pointer to integer of different size */
						output_error("unable to add object breakpoint");
						/* TROUBLESHOOT
							The <b>break <i>object</i></b> could not be completed because of a 
							problem with the internal operation needed to perform it.
							Follow the guidance for the message which precedes this message
							and try again.
						 */
				}
				else
					output_error("object %s does not exist",bpval);
					/* TROUBLESHOOT
						The <b>break error</b> refers to an object that does not
						exist.  Find the correct object name and try again.
					 */
			}
			else if (strncmp(bptype,"module",strlen(bptype))==0)
			{	/* create module breakpoint */
				MODULE *mod = module_find(bpval);
				if (mod!=NULL)
				{
					if (!exec_add_breakpoint(BP_MODULE,(int64)mod)) /* warning: cast from pointer to integer of different size */
						output_error("unable to add module breakpoint");
						/* TROUBLESHOOT
							The <b>break module</b> could not be completed because of a 
							problem with the internal operation needed to perform it.
							Follow the guidance for the message which precedes this message
							and try again.
						 */
				}
				else
					output_error("module %s does not exist",bpval);
					/* TROUBLESHOOT
						The <b>break module</b> refers to a module that does not exist.
						Identify the correct name of the module and try again.
					 */
			}
			else if (strncmp(bptype,"class",strlen(bptype))==0)
			{	/* create class breakpoint */
				CLASS *oclass = class_get_class_from_classname(bpval);
				if (oclass!=NULL)
				{
					if (!exec_add_breakpoint(BP_CLASS,(int64)oclass)) /* warning: cast from pointer to integer of different size */
						output_error("unable to add class breakpoint");
						/* TROUBLESHOOT
							The <b>break error</b> could not be completed because of a 
							problem with the internal operation needed to perform it.
							Follow the guidance for the message which precedes this message
							and try again.
						 */
				}
				else
					output_debug("class %s does not exist",bpval);
			}
			else if (strncmp(bptype,"pass",strlen(bptype))==0)
			{	/* create pass breakpoint */
				int pass;
				if (strnicmp(bpval,"pretopdown",max(2,strlen(bpval)))==0) pass=PC_PRETOPDOWN;
				else if (strnicmp(bpval,"bottomup",strlen(bpval))==0) pass=PC_BOTTOMUP;
				else if (strnicmp(bpval,"posttopdown",max(2,strlen(bpval)))==0) pass=PC_POSTTOPDOWN;
				else
				{
					output_error("undefined pass type for add breakpoint");
					/* TROUBLESHOOT
						The <b>break pass</b> specifies a pass type that isn't
						valid.  Only <b>pretopdown</b>, <b>bottomup</b>, and <b>posttopdown</b>
						are supported.
					 */
					continue;
				}
				if (!exec_add_breakpoint(BP_PASS,(int64)pass))
					output_error("unable to add pass breakpoint");
				/* TROUBLESHOOT
					The <b>break pass</b> could not be completed because of a 
					problem with the internal operation needed to perform it.
					Follow the guidance for the message which precedes this message
					and try again.
				 */
			}
			else if (strncmp(bptype,"rank",strlen(bptype))==0)
			{	/* create rank breakpoint */
				if (!exec_add_breakpoint(BP_RANK,(int64)atoi(bpval)))
					output_error("unable to add rank breakpoint");
					/* TROUBLESHOOT
						The <b>break rank</b> specifies a rank that isn't
						valid.  Check the command syntax and try again.
					 */
			}
			else if (strncmp(bptype,"time",strlen(bptype))==0)
			{	/* create time breakpoint */
				TIMESTAMP ts = convert_to_timestamp(bpval);
				if (ts==TS_NEVER)
					output_error("invalid time");
					/* TROUBLESHOOT
						The <b>break time</b> specifies a timestamp that isn't
						valid.  Check the command syntax and try again.
					 */
				else if (!exec_add_breakpoint(BP_TIME,(int64)ts))
					output_error("unable to add timestamp breakpoint");
					/* TROUBLESHOOT
						The <b>break time</b> could not be completed because of a 
						problem with the internal operation needed to perform it.
						Follow the guidance for the message which precedes this message
						and try again.
					 */
			}
			else if (strncmp(bptype,"disable",strlen(bptype))==0)
			{
				int n=-1;
				BREAKPOINT *bp;
				if (strcmp(bpval,"")!=0)
				{
					if (isdigit(bpval[0]))
						n=atoi(bpval);
					else
					{
						output_error("invalid breakpoint number");
						/* TROUBLESHOOT
							The <b>break disable</b> command specified a breakpoint number
							that doesn't exist.  Check the list of breakpoints and try again.
						 */
						continue;
					}
				}
				for (bp=first_breakpoint; bp!=NULL; bp=bp->next)
				{
					if (bp->num==n || n==-1)
						bp->enabled = 0;
				}
			}
			else if (strncmp(bptype,"enable",strlen(bptype))==0)
			{
				int n=-1;
				BREAKPOINT *bp;
				if (strcmp(bpval,"")!=0)
				{
					if (isdigit(bpval[0]))
						n=atoi(bpval);
					else
					{
						output_error("invalid breakpoint number");
						/* repeat of last troubleshooting */
						continue;
					}
				}
				for (bp=first_breakpoint; bp!=NULL; bp=bp->next)
				{
					if (bp->num==n || n==-1)
						bp->enabled = 1;
				}
			}
			else
				output_error("%s is not a recognized breakpoint subcommand", bptype);
				/* TROUBLESHOOT
					The <b>break</b> subcommand isn't recognized.  
					Check the command syntax and try again.
				 */
		}
		else if (strncmp(cmd,"watch",max(2,strlen(cmd)))==0)
		{
			char wptype[256]="";
			char wpval[256]="";
			OBJECT *obj;
			if (sscanf(buffer,"%*s %s %[^\0]", wptype, wpval)==0)
			{
				/* display all watchpoints */
				WATCHPOINT *wp;
				if (last_watchpoint==NULL)
					output_debug("no watchpoints");
				for (wp=first_watchpoint; wp!=NULL; wp=wp->next)
				{
					if (wp->prop==NULL)
						output_debug("watchpoint %d - object %s %s", wp->num, get_objname(wp->obj), wp->enabled?"":"(disabled)");
					else
						output_debug("watchpoint %d - object %s %s - property %s", wp->num, get_objname(wp->obj), wp->enabled?"":"(disabled)", wp->prop->name);
				}
			}
			else if (isdigit(wptype[0]))
			{
				int n=atoi(wptype);
				WATCHPOINT *wp;
				for (wp=first_watchpoint; wp!=NULL; wp=wp->next)
				{
					if (wp->num==n)
						output_debug("watchpoint %d - object %s %s\n%s", 
							wp->num, get_objname(wp->obj), wp->enabled?"":"(disabled)",wp->buffer);
				}
			}
			else if (strcmp(wptype,"sync")==0)
			{
				watch_sync = !watch_sync;
				output_debug("sync watch is %s",watch_sync?"enabled":"disabled");
			}
			else if (strncmp(wptype,"disable",strlen(wptype))==0)
			{
				int n=-1;
				WATCHPOINT *wp;
				if (strcmp(wpval,"")!=0)
				{
					if (isdigit(wpval[0]))
						n=atoi(wpval);
					else
					{
						output_error("invalid watchpoint number");
						/* TROUBLESHOOT
							The <b>watch</b> command refers to a watchpoint that doesn't exist.
							Check the list of watchpoints and try again.
						 */
						continue;
					}
				}
				for (wp=first_watchpoint; wp!=NULL; wp=wp->next)
				{
					if (wp->num==n || n==-1)
						wp->enabled = 0;
				}
			}
			else if (strncmp(wptype,"enable",strlen(wptype))==0)
			{
				int n=-1;
				WATCHPOINT *wp;
				if (strcmp(wpval,"")!=0)
				{
					if (isdigit(wpval[0]))
						n=atoi(wpval);
					else
					{
						output_error("invalid watchpoint number");
						/* repeat of last troubleshoot */
						continue;
					}
				}
				for (wp=first_watchpoint; wp!=NULL; wp=wp->next)
				{
					if (wp->num==n || n==-1)
						wp->enabled = 1;
				}
			}
			else if ((obj=object_find_name(wptype)) || convert_to_object(wptype,&obj,NULL))
			{
				if (strcmp(wpval,"")!=0)
				{
					PROPERTY *prop = object_get_property(obj,wpval,NULL);
					if (prop==NULL)
						output_error("object %s does not have a property named '%s'", wptype,wpval);
					/* TROUBLESHOOT
						The <b>watch</b> command refers to an object property that isn't defined.
						Check the list of properties for that class of object and try again.
					 */
					else if (!exec_add_watchpoint(obj,prop))
						output_error("unable to add object watchpoint");
					/* TROUBLESHOOT
						The <b>watch</b> could not be completed because of a 
						problem with the internal operation needed to perform it.
						Follow the guidance for the message that precedes this message
						and try again.
					 */
				}
				else if (!exec_add_watchpoint(obj,NULL))
					output_error("unable to add object watchpoint");
				/* repeat last TROUBLESHOOT */
			}
			else
				output_error("%s is not recognized", wptype);		
			/* TROUBLESHOOT
				The <b>watch</b> subcommand isn't recognized.  
				Check the command syntax and try again.
			 */
		}
		else if (strncmp(cmd,"where",max(1,strlen(cmd)))==0)
		{
			char ts[64];
			output_debug("Global clock... %s (%" FMT_INT64 "d)", convert_from_timestamp(global_clock,ts,sizeof(ts))?ts:"(invalid)",global_clock);
			output_debug("Hard events.... %d", data->hard_event);
			output_debug("Sync status.... %s", data->status==FAILED?"FAILED":"SUCCESS");
			output_debug("Step to time... %s (%" FMT_INT64 "d)", convert_from_timestamp(data->step_to,ts,sizeof(ts))?ts:"(invalid)",data->step_to);
			output_debug("Pass........... %d", pass);
			output_debug("Rank........... %d", index);
			output_debug("Object......... %s",get_objname(obj));
		}
		else if (strncmp(cmd,"print",max(1,strlen(cmd)))==0)
		{
			char tmp[4096];
			if (sscanf(buffer,"%*s %s",tmp)==0)
			{
				if (object_dump(tmp,sizeof(tmp),obj))
					output_debug("%s",tmp);
				else
					output_error("object dump failed");
				/* TROUBLESHOOT
					The <b>print</b> was unable to complete the dump of the specified object
					because of an internal error.  						
					Follow the guidance for the message that precedes this message and try again.
				 */
			}
			else
			{
				OBJECT *obj = object_find_name(tmp);
				if (obj!=NULL || convert_to_object(tmp,&obj,NULL))
				{
					if (object_dump(tmp,sizeof(tmp),obj))
						output_debug("%s",tmp);
					else
						output_error("object dump failed");
					/* repeat last TROUBLESHOOT */
				}
				else
					output_error("object %s undefined", tmp);
				/* TROUBLESHOOT
					The <b>print</b> command refers to an object that doesn't exist.
					Check the list of objects and try again.
				 */
			}
		}
		else if (strncmp(cmd,"globals",max(1,strlen(cmd)))==0)
		{
			GLOBALVAR *var;
			for (var=global_getnext(NULL); var!=NULL; var=global_getnext(var))
			{
				char buffer[256];
				char *val = global_getvar(var->prop->name, buffer, 255);
				if (val!=NULL && strlen(val)>64)
					strcpy(val+28,"...");
				output_message("%-32.32s: \"%s\"", var->prop->name, val==NULL?"(error)":val);
			}
		}
		else if (strncmp(cmd,"set",max(1,strlen(cmd)))==0)
		{
			char256 objname;
			char256 propname;
			char256 value;
			int scanct = 0;
			value[0] = '\0';
			if ((scanct=sscanf(buffer,"%*s %s %s %s",objname,propname,value))>=2)
			{
				OBJECT *obj;
				if (strcmp(objname,"global")==0)
				{
					if (global_setvar(propname,value)==FAILED)
						output_error("unable to set global value %s", propname);
						/* TROUBLESHOOT
							The <b>set</b> debug command was unable to set the specified global variable because
							of an internal error. 
							Follow the guidance for the message that precedes this message
							and try again.
						 */
				}
				else if ((obj=object_find_name(objname)) || convert_to_object(objname,&obj,NULL))
				{
					if (!object_set_value_by_name(obj,propname,value))
						output_error("unable to set value of object %s property %s", objname,propname);
						/* TROUBLESHOOT
							The <b>set</b> debug command was unable to set the value given could not be
							converted to the type required for the property.  Check the command
							syntax and try again.
						 */
					else if (!object_get_property(obj,propname,NULL))
						output_error("invalid property for object %s",objname);
						/* TROUBLESHOOT
							This error should not occur and in harmless.  
							If it does occur, please report the problem to the core development team.
						 */
				}
				else
					output_error("invalid object or value");
					/* TROUBLESHOOT
						The <b>set</b> debug command was unable to set the specified property
						either because the object could not be found, or the value given could not be
						converted to the type required for the property.  Check the command
						syntax and try again.
					 */
			}
			else{
				output_error("set syntax error (%i items)", scanct);
				/* TROUBLESHOOT
					The <b>set</b> syntax doesn't have the correct number of arguments.
					Check the command syntax and try again.
				 */
			}
		}
		else if (strncmp(cmd,"find ",max(1,strlen(cmd)))==0)
		{
			FINDLIST *fl = NULL;
			OBJECT *obj = NULL;
			output_debug("running search with \"%s\"", buffer+5);
			fl = find_objects(FL_GROUP, buffer+5);
			obj = find_first(fl);
			//while(obj != NULL){
			//	output_debug(" * %s (%s)", get_objname(obj), obj->oclass->name);
			//	obj = find_next(fl, obj);
			for (obj=find_first(fl); obj!=NULL; obj=find_next(fl, obj)) 
				list_object(obj,pass);
		}
		else if (strncmp(cmd,"gdb",3)==0)
		{
			if (exec("gdb --quiet %s --pid=%d",global_execname,global_process_id)<=0)
				output_debug("unable to start gdb");
		}
		else if (strncmp(cmd,"help",max(1,strlen(cmd)))==0)
		{
			output_debug("Summary of debug commands\n"
				"   break             prints all breakpoints\n"
				"         clock       stop the simulator on clock advance\n"
				"         error       stop the simulator is an error occurs\n"
				"         object <X>  stop the simulator on sync of object X\n"
				"         module <X>  stop the simulator on sync of module X\n"
				"         class <X>   stop the simulator on sync of class X\n"
				"         pass <X>    stop the simulator on the start of pass X\n"
				"         rank <X>    stop the simulation on the start of rank X\n"
				"         time <X>    stop the simulation at the time time X\n"
				"         disable     disables all breakpoints\n"
				"         enable      enables all breakpoints\n"
				"         disable <#> disables breakpoint #\n"
				"         enable <#>  enables breakpoint #\n"
				"   details on|off    enables/disables details in list\n"
				"   globals           print global variables\n"
				"   inactive on|off   enables/disables listing of inactive objects\n"
				"   list              list objects\n"
				"        <X>          list objects of class X\n"
				"   module            list loaded modules\n"
				"          <X>        load module X\n"
				"   next              advances to the next object sync\n"
				"   print <X>         prints object X\n"
				"   quit              quits the simulator\n"
				"   run               runs the simulator until the next break point\n"
				"   set <X> <Y> <val> set object X property Y to <val>\n"
				"   script <filename> read a script file containing debugger commands\n"
				"   nsync on|off      enables/disables listing of non-synchronizing objects\n"
				"   system            opens a shell on the current OS\n"
				"          <command>  executes the command on the current OS\n"
				"   unnamed on|off    enables/disables listing of unnamed objects\n"
				"   watch             prints all watchpoints\n"
				"         <#>         prints watchpoint #\n"
				"         <X>         stop the simulation if object X changes\n"
				"         <X> <Y>     stop the simulation if object X property Y changes\n"
				"         disable     disables all watchpoints #\n"
				"         disable <#> disables watchpoint #\n"
				"         enable      enables all watchpoint #\n"
				"         enable <#>  enables watchpoint #\n"
				"         sync        toggles sync watch\n"
				"   where             displays the current context\n"
				);
		}
		else if (strcmp(buffer,"")!=0)
			output_error("invalid debug command, try 'help'");
			/* TROUBLESHOOT
				The debug command given is not recognized.  Use the <b>help</b>
				to check the command syntax and try again.
			 */
		else
			return last;
	}
}

/** This is the main debugger processing loop
 **/
int exec_debug(struct sync_data *data, /**< the current sync status of the mail loop */
			   int pass, /**< the current pass on the main loop */
			   int index, /**< the rank index */
			   OBJECT *obj) /**< the current object being processed */
{
	TIMESTAMP this_t;
#ifdef WIN32
	static int firstcall=1;
	if (firstcall)
	{
		firstcall=0;

		/* disable repeated message trap */
		global_suppress_repeat_messages = 0;
	}
#endif

	global_debug_output = 1; // force it on
	global_suppress_repeat_messages = 0; // force it off

	if (debug_active==0)
	{
		BREAKPOINT *bp;
		WATCHPOINT *wp;
		static char timebuf[64]="";
		char buffer[64];

		/* only output time update if it differs from last one */
		if (convert_from_timestamp(global_clock,buffer,sizeof(buffer)) && strcmp(buffer,timebuf)!=0)
		{
			output_raw("DEBUG: global_clock = '%s' (%"FMT_INT64"d)\r", buffer,global_clock);
			strcpy(timebuf,buffer);
		}

		/* check watchpoints */
		for (wp=first_watchpoint; wp!=NULL; wp=wp->next)
		{
			if (wp->enabled==0)
				continue;
			if (obj==wp->obj)
			{
				if (wp->prop==NULL)
				{
					char tmp[4096];
					if (object_dump(tmp,sizeof(tmp),obj) && strcmp(tmp,wp->buffer)!=0)
					{
						output_debug("watchpoint %d stopped on object %s", wp->num, get_objname(obj));
						output_debug("%s",tmp);
						strcpy(wp->buffer,tmp);
						debug_active=1;
					}
				}
				else
				{
					char temp[1024];
					char *tmp = object_property_to_string(obj,wp->prop->name, temp, 1023);
					if (tmp!=NULL && strcmp(tmp,wp->buffer)!=0)
					{
						output_debug("watchpoint %d stopped on object %s property %s", wp->num, get_objname(obj), wp->prop->name);
						output_debug("from... %s",wp->buffer);
						output_debug("to..... %s",tmp);
						strcpy(wp->buffer,tmp);
						debug_active=1;
					}
				}
			}
		}

		/* check breakpoints */
		for (bp=first_breakpoint; bp!=NULL; bp=bp->next)
		{
			if (bp->enabled==0)
				continue;
			if (bp->type==BP_ERROR && error_caught)
			{
				output_debug("breakpoint %d (error) stopped on object %s", bp->num, get_objname(obj));
				error_caught=0;
				debug_active=1;
			}
			if (bp->type==BP_MODULE && bp->mod==obj->oclass->module)
			{
				output_debug("breakpoint %d (module %s) stopped on object %s", bp->num, bp->mod->name, get_objname(obj));
				debug_active=1;
			}
			else if (bp->type==BP_CLASS && bp->oclass==obj->oclass)
			{
				output_debug("breakpoint %d (class %s) stopped on object %s", bp->num, bp->oclass->name, get_objname(obj));
				debug_active=1;
			}
			else if (bp->type==BP_OBJECT && bp->obj==obj)
			{
				char buf1[1024];
				strcpy(buf1,get_objname(bp->obj));
				output_debug("breakpoint %d (object %s) stopped on object %s", bp->num, buf1, get_objname(obj));
				debug_active=1;
			}
			else if (bp->type==BP_RANK && bp->rank==(int64)obj->rank)
			{
				output_debug("breakpoint %d (rank %d) stopped on object %s", bp->num, (int)bp->rank, get_objname(obj));
				debug_active=1;
			}
			else if (bp->type==BP_PASS && bp->pass==(int64)pass)
			{
				output_debug("breakpoint %d (pass %d) stopped on object %s", bp->num, (int)bp->pass, get_objname(obj));
				debug_active=1;
			}
			else if (bp->type==BP_TIME && global_clock>=bp->ts)
			{
				char tmp[64];
				output_debug("breakpoint %d (time %s (%d)) stopped on object %s:%" FMT_INT64 "d", bp->num, convert_from_timestamp(bp->ts,tmp,sizeof(tmp))?tmp:"(invalid)", bp->ts, obj->oclass->name, obj->id);
				debug_active=1;
			}
			else if (bp->type==BP_CLOCK && global_clock!=bp->ts)
			{
				char tmp[64];
				output_debug("breakpoint %d (clock %s (%d)) stopped on clock update", bp->num, convert_from_timestamp(bp->ts,tmp,sizeof(tmp))?tmp:"(invalid)", bp->ts);
				bp->ts = global_clock; /* now use this time as bp */
				debug_active=1;
			}
		}
	}
	if (debug_active==1)
	{
		DEBUGCMD dbgcmd = exec_debug_cmd(data,pass,index,obj);
		switch (dbgcmd) {
		case DBG_QUIT: return FAILED;
		case DBG_RUN: break;
		case DBG_NEXT: break;
		default: 
			output_error("invalid debug state");
			/* TROUBLESHOOT
				The debugger has entered a state the indicates the system
				has become unstable.  Please submit a problem report to the
				core development and include both the input and output files,
				including the XML dump file.
			 */
			return FAILED;
			break;
		}
	}

	/* check in and out-of-service dates */
	if (global_clock<obj->in_svc)
	{
		if (debug_active) output_debug("%s not in service yet", get_objname(obj));
		this_t = obj->in_svc; /* yet to go in service */
	}
	else if (global_clock<=obj->out_svc)
	{
		this_t = object_sync(obj, global_clock, passtype[pass]);
		if (debug_active || watch_sync) 
		{
			char buffer[64]="(error)";
			convert_from_timestamp(this_t<0?-this_t:this_t,buffer,sizeof(buffer));
			output_debug("%s next sync %s %s", get_objname(obj),buffer,this_t<0?"(soft sync)":"");
		}
	}
	else 
	{
		if (debug_active) output_debug("%s out of service", get_objname(obj));
		this_t = TS_NEVER; /* already out of service */
	}

	/* check for "soft" event (events that are ignored when stopping) */
	if (this_t < -1)
		this_t = -this_t;
	else if (this_t != TS_NEVER)
		data->hard_event++;  /* this counts the number of hard events */

	/* check for stopped clock */
	if (this_t < global_clock) {
		char b[64];
		output_error("%s: object %s stopped its clock (debug)!", simtime(), object_name(obj, b, 63));
		/* TROUBLESHOOT
			This indicates that one of the objects in the simulator has encountered a
			state where it cannot calculate the time to the next state.  This usually
			is caused by a bug in the module that implements that object's class.
		 */
		if (error_caught)
			data->status = SUCCESS;	/* this will let the debugger handle the error */
		else
			data->status = FAILED; /* the debugger isn't going to handle the error so make exec handle it */
	} else {
		/* check for iteration limit approach */
		if (iteration_counter == 2 && this_t == global_clock) {
			char b[64];
			output_verbose("%s: object %s iteration limit imminent",
								simtime(), object_name(obj, b, 63));
		}
		else if (iteration_counter == 1 && this_t == global_clock) {
			output_error("convergence iteration limit reached for object %s (debug)", get_objname(obj));
			/* TROUBLESHOOT
				This indicates that the core's solver was unable to determine
				a steady state for all objects for any time horizon.  Identify
				the object that is causing the convergence problem and contact
				the developer of the module that implements that object's class.
			 */
		}

		/* manage minimum timestep */
		if (global_minimum_timestep>1 && this_t>global_clock && this_t<TS_NEVER)
			this_t = ((this_t/global_minimum_timestep)+1)*global_minimum_timestep;

		/* if this event precedes next step, next step is now this event */
		if (data->step_to > this_t)
			data->step_to = this_t;
		data->status = SUCCESS;
	}
	return data->status;
}

/**@}*/
