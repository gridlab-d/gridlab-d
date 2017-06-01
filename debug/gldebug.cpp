/** $Id: gldebug.cpp 4738 2014-07-03 00:55:39Z dchassin $

   General purpose debug objects

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>
#include <signal.h>

#define CONSOLE /* enables console/curses support */

#include "gldebug.h"

EXPORT_CREATE_C(debug,g_debug);
EXPORT_INIT_C(debug,g_debug);
EXPORT_PRECOMMIT_C(debug,g_debug);
EXPORT_COMMIT_C(debug,g_debug);
EXPORT_SYNC_C(debug,g_debug);
EXPORT_FINALIZE_C(debug,g_debug);

gld_global global_debug;
bool debug_window = FALSE;

////////////////////////////////////////////////////////////////////////////////
// notification handler
static NOTIFICATION *first_notice = NULL;
bool add_notification(OBJECT *target, PROPERTY *prop, OBJECT *recipient, int (*call)(NOTIFICATION*))
{
	NOTIFICATION *item = (NOTIFICATION*)malloc(sizeof(NOTIFICATION));
	if ( item==NULL ) return false;
	item->recipient = recipient;
	item->target = target;
	item->prop = prop;
	item->call = call;
	item->next = first_notice;
	first_notice = item;
	return true;
}
EXPORT int notify_debug(OBJECT *obj, int type, PROPERTY *prop, char *value)
{
	NOTIFICATION *notice;
	for ( notice=first_notice ; notice!=NULL ; notice=notice->next )
	{
		// check if this is the notice of interest
		if ( notice->target==obj && notice->prop==prop )
		{
			g_debug *debug = OBJECTDATA(notice->recipient,g_debug);
			debug->notify(notice);
			return notice->chain ? notice->chain(obj,type,prop,value) : 1;
		}
	}
	return 1;
}
////////////////////////////////////////////////////////////////////////////////
// class implementation
CLASS *g_debug::oclass = NULL;
g_debug *g_debug::defaults = NULL;

unsigned long g_debug::history_next = 0;
unsigned long g_debug::command_num = 0;
unsigned long g_debug::history_size = 5;
char **g_debug::history = NULL;

bool break_on = false;
void break_enable(void)
{
	break_on = true;
}
void break_handler(int sig)
{
	if ( !break_on )
	{
		printf("\n*** SIGNAL %d RECEIVED ***\n", sig);
		break_enable();
	}
	else
		printf("\n*** SIGNAL %d IGNORED ***\nUse 'quit' to stop debugger\n", sig);
	signal(SIGINT,break_handler);
}
void break_disable(void)
{
	break_on = false;
}

g_debug::g_debug(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"debug",sizeof(g_debug),PC_BOTTOMUP|PC_PRETOPDOWN|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class debug";
		else
			oclass->trl = TRL_PROVEN;

		defaults = this;
		if (gl_publish_variable(oclass,
			PT_enumeration,"type",get_type_offset(),PT_DESCRIPTION,"debug point type (none or break)",
				PT_KEYWORD,"NONE",(enumeration)DBT_NONE,
				PT_KEYWORD,"BREAK",(enumeration)DBT_BREAK,
			PT_enumeration,"status",get_status_offset(),PT_DESCRIPTION,"debug point status (active or inactive)",
				PT_KEYWORD,"INACTIVE",(enumeration)DBS_INACTIVE,
				PT_KEYWORD,"ACTIVE",(enumeration)DBS_ACTIVE,
			PT_set,"event",get_event_offset(),PT_DESCRIPTION,"debug event (init, sync, etc.)",
				PT_KEYWORD,"NONE",(set)DBE_NONE,
				PT_KEYWORD,"INIT",(set)DBE_INIT,
				PT_KEYWORD,"PRECOMMIT",(set)DBE_PRECOMMIT,
				PT_KEYWORD,"PRESYNC",(set)DBE_PRESYNC,
				PT_KEYWORD,"SYNC",(set)DBE_SYNC,
				PT_KEYWORD,"POSTSYNC",(set)DBE_POSTSYNC,
				PT_KEYWORD,"COMMIT",(set)DBE_COMMIT,
				PT_KEYWORD,"FINALIZE",(set)DBE_FINALIZE,
				PT_KEYWORD,"ALL",(set)DBE_ALL,
			PT_timestamp, "stoptime", get_stoptime_offset(), PT_DESCRIPTION,"the time at which to stop",
			PT_char1024, "target", get_target_offset(),PT_DESCRIPTION,"the target property to test",
			PT_char32, "part", get_part_offset(),PT_DESCRIPTION,"the target property part to test",
			PT_enumeration,"relation",get_relation_offset(),PT_DESCRIPTION,"the relation to use for the test",
				PT_KEYWORD,"==",TCOP_EQ,
				PT_KEYWORD,"<",TCOP_LT,
				PT_KEYWORD,"<=",TCOP_LE,
				PT_KEYWORD,">",TCOP_GT,
				PT_KEYWORD,">=",TCOP_GE,
				PT_KEYWORD,"!=",TCOP_NE,
				PT_KEYWORD,"inside",TCOP_IN,
				PT_KEYWORD,"outside",TCOP_NI,
			PT_char1024, "value", get_value_offset(),PT_DESCRIPTION,"the value to compare with for binary tests",
			PT_char1024, "within", get_value2_offset(),PT_DESCRIPTION,"the bounds within which the value must bed compared",
			PT_char1024, "lower", get_value_offset(),PT_DESCRIPTION,"the lower bound to compare with for interval tests",
			PT_char1024, "upper", get_value2_offset(),PT_DESCRIPTION,"the upper bound to compare with for interval tests",
			PT_set,"options", get_options_offset(), PT_DESCRIPTION,"debugging options",
				PT_KEYWORD,"DETAILS",(set)DBO_DETAILS,
			NULL)<1)
		{
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}
		memset(this,0,sizeof(g_debug));
		relation=TCOP_EQ;
		signal(SIGINT,break_handler);
		history = (char**)malloc(sizeof(char*)*history_size);
		memset(history,0,sizeof(char*)*history_size);
		if ( !global_debug.get("debug") )
			throw "unable to get global 'debug'";
		gld_global("debug::window",PT_bool,&debug_window);
	}
}

int g_debug::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	return 1; /* return 1 on success, 0 on failure */
}

int g_debug::init(OBJECT *parent)
{
	char buffer[64];
	strcpy(buffer,get_name());
	if ( get_parent() )
		message("%s init(OBJECT *parent={name:'%s';...})",buffer,get_parent()->get_name());
	else
		message("%s init(OBJECT *parent=NULL)",buffer);

	// find target
	if ( get_parent()!=NULL )
	{
		char pname[1024];
		strcpy(pname, get_parent()->get_name());
		target_property.set_object(get_parent());
		target_property.set_property(target);
		if ( !target_property.is_valid() )
		{
			gl_error("%s: target '%s' of parent '%s' is not valid", get_name(), target.get_string(), pname);
			return FAILED;
		}
		gl_verbose("%s: target '%s.%s' is ok", get_name(), pname, target.get_string());
		message("initial value of '%s.%s' is '%s'", pname, target.get_string(), (const char*)target_property.get_string());
		message("initial debug type is '%s'", (const char*)gld_property(my(),"type").get_string());
	}
	else
	{
		gl_error("%s: parent is not set", get_name());
		return FAILED;
	}

	// check events
	if ( get_event()==DBE_NONE )
		gl_warning("%s: no debug event given", get_name());
	else
	{
		gld_property event_p(my(),"event");
		message("events = '%s'", (const char*)event_p.get_string());
	}

	if ( get_event()&DBE_INIT )
	{
		message("%s INIT event", get_name());
		return get_command() ? SUCCESS : FAILED;
	}
	return SUCCESS;
}

int g_debug::precommit(TIMESTAMP t)
{
	if ( stop_condition(DBE_PRECOMMIT) )
	{
		message("%s PRECOMMIT event",get_name());
		return get_command() ? SUCCESS : FAILED;
	}
	return SUCCESS;
}

TIMESTAMP g_debug::presync(TIMESTAMP t)
{
	if ( stop_condition(DBE_PRESYNC) )
	{
		message("%s PRESYNC event",get_name());
		return get_command() ? TS_NEVER : TS_INVALID;
	}
	return TS_NEVER;
}

TIMESTAMP g_debug::sync(TIMESTAMP t)
{
	if ( stop_condition(DBE_SYNC) )
	{
		message("%s SYNC event",get_name());
		return get_command() ? TS_NEVER : TS_INVALID;
	}
	return TS_NEVER;
}

TIMESTAMP g_debug::postsync(TIMESTAMP t)
{
	if ( stop_condition(DBE_POSTSYNC) )
	{
		message("%s POSTSYNC event",get_name());
		return get_command() ? TS_NEVER : TS_INVALID;
	}
	return TS_NEVER;
}

TIMESTAMP g_debug::commit(TIMESTAMP t1, TIMESTAMP t2)
{
	if ( stop_condition(DBE_COMMIT) )
	{
		message("%s COMMIT event",get_name());
		return get_command() ? TS_NEVER : TS_INVALID;
	}
	return TS_NEVER;
}

int g_debug::finalize(void)
{
	if ( stop_condition(DBE_FINALIZE) )
	{
		message("%s FINALIZE event",get_name());
		return get_command() ? SUCCESS : FAILED;
	}
	return SUCCESS;
}

int g_debug::notify(NOTIFICATION *notice)
{
	// TODO
	return 1;
}


bool g_debug::stop_condition(DEBUGEVENT context)
{
	status = DBS_INACTIVE;

	// stoptime satisfied
	if ( ( stoptime==TS_ZERO || stoptime>=gl_globalclock ) && context&event )
	{
		message("stoptime condition met at %s", (const char *)gld_clock(stoptime).get_string());
		status = DBS_ACTIVE;
	}

	// target
	gld_property target_prop(get_parent(),target);
	if ( strcmp(part,"")==0 && target_prop.compare(relation,value,value2) )
		status = DBS_ACTIVE;
	if ( strcmp(part,"")!=0 && target_prop.compare(relation,value,value2,part) )
		status = DBS_ACTIVE;

	return status==DBS_ACTIVE;
}

size_t g_debug::message(char *fmt, ...)
{
	if ( global_debug.get_bool() )
	{
		size_t len;
		va_list ptr;
		va_start(ptr,fmt);
		if ( debug_window )
		{
			char msg[1024];
			len = vsprintf(msg,fmt,ptr);
			console_message("%s",msg);
		}
		else
		{
			char buffer[64];
			gld_clock now;
			len = fprintf(stdout,"\nDEBUG [%s] %s %u%s ",
				now.to_string(buffer,sizeof(buffer))>0?buffer:"???", get_name(), command_num,fmt?":":">");
			if ( fmt!=NULL )
			{
				len += vfprintf(stdout,fmt,ptr);
			}
		}
		va_end(ptr);
		return len;
	}
	else
		return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// COMMAND LINE
////////////////////////////////////////////////////////////////////////////////////////////////
bool g_debug::get_command(void)
{
	// check is enabled
	if ( global_debug.get_bool()==false )
		return true;

	if ( debug_window==TRUE && console_get_command() )
		return true;

	bool run = true;
	break_enable();
	while ( run )
	{
		char buffer[1024]="";
		message();
		fgets(buffer,sizeof(buffer),stdin); 
		char cmd[64];
		char args[1024]="";
		char *last = (history[(history_next-1)%history_size]==NULL) ? NULL : history[(history_next-1)%history_size];

		// repeat history commands
		if ( buffer[0]=='!' )
		{
			if ( buffer[1]=='!' ) // last command
				strcpy(buffer,last);
			else if ( strlen(buffer)>2 ) // include \n
			{ // older command
				int n = atoi(buffer+1)%history_size;
				if ( history[n]!=NULL )
				{
					strcpy(buffer,history[n]);
					message(buffer);
				}
				else
				{
					message("no such command in history");
					continue;
				}
			}
			else
			{
				cmd_history();
				continue;
			}
		}

		// process commands
		if ( sscanf(buffer,"%s %[^\n]",cmd,args)>0 )
		{
			size_t len = strlen(cmd);

			// process commands
			if ( strnicmp(cmd,"print",len)==0 )
				run = cmd_print(args);
			else if ( strnicmp(cmd,"break",len)==0 )
				run = cmd_break(args);
			else if ( strnicmp(cmd,"watch",len)==0 )
				run = cmd_watch(args);
			else if ( strnicmp(cmd,"list",len)==0 )
				run = cmd_list(args);
			else if ( strnicmp(cmd,"help",len)==0 )
				run = cmd_help(args);
			else if ( strnicmp(cmd,"info",len)==0 )
				run = cmd_info(args);
			else if ( strnicmp(cmd,"history",len)==0)
				run = cmd_history(args);
			else if ( strnicmp(cmd,"signal",len)==0 )
				cmd_signal(args);
			else if ( strnicmp(cmd,"continue",len)==0 )
				run = false;
			else if ( strnicmp(cmd,"details",len)==0 )
				run = cmd_details(args);
			else if ( strnicmp(cmd,"run",len)==0 )
			{
				run = false;
				gld_global debug("debug");
				debug.from_string("0");
			}
			else if ( strnicmp(cmd,"quit",len)==0 )
				exit(0);
			else if ( strnicmp(cmd,"exit",len)==0 )
				exit(atoi(args));
			else if ( strnicmp(cmd,"abort",len)==0 )
				abort();
			else
				message("command '%s' not found",cmd);
			if ( last==NULL || strcmp(last,buffer)!=0 )
			{
				if ( history[history_next]!=NULL ) free(history[history_next]);
				history[history_next] = (char*)malloc(strlen(buffer)+1);
				strcpy(history[history_next++],buffer);
				if ( history_next==history_size )
					history_next = 0;
				command_num++;
			}
		}
	}
	break_disable();
	return true;
}
bool g_debug::cmd_help(char *args)
{
	if ( strcmp(args,"")==0 )
	{
		gl_output("Available command (type 'command help' for more or 'info command' for details):");
		gl_output("  Abort      Calls abort()");
		gl_output("  Break      Controls breakpoint events");
		gl_output("  Continue   Continues until next debug event");
		gl_output("  Details    Controls details in lists");
		gl_output("  Exit       Calls exit(n)");
		gl_output("  History    Lists command history");
		gl_output("  Info       Opens a browser");
		gl_output("  List       Lists objects");
		gl_output("  Print      Prints properties");
		gl_output("  Run        Runs until next Ctrl-C");
		gl_output("  Signal     Raises a signal");
		gl_output("  Quit       Quits immediately");
		gl_output("  Watch      Controls watchpoint events");
		gl_output("  !!         Repeats last command");
		gl_output("  !<n>       Repeats <n>th command");
	}
	else
	{
		gl_output("Try entering '%s help'", args);
	}
	return true;
}
bool g_debug::cmd_info(char *args)
{
	char cmd[1024];
	gld_global browser("browser");
	gld_global infourl("infourl");
#ifdef WIN32
	sprintf(cmd,"start %s %s%s", (const char*)browser.get_string(), (const char*)infourl.get_string(), args);
#elif defined(MACOSX)
	sprintf(cmd,"open -a %s %s%s", (const char*)browser.get_string(), (const char*)infourl.get_string(), args);
#else
	sprintf(cmd,"%s '%s%s' & ps -p $! >/dev/null", (const char*)browser.get_string(), (const char*)infourl.get_string(), args);
#endif
	gl_verbose("Starting browser using command [%s]", cmd);
	if (system(cmd)!=0)
		gl_error("unable to start browser");
	else
		gl_verbose("starting interface");
	return true;
}
bool g_debug::cmd_details(char *args)
{
	if ( stricmp(args,"help")==0 )
	{
		gl_output("Syntax: details [on|off]");
		gl_output("Details provide extra info for 'list' command");
		return true;
	}

	if ( stricmp(args,"on")==0 )
		set_options_bits(DBO_DETAILS);
	else if ( stricmp(args,"off")==0 )
		clr_options_bits(DBO_DETAILS);
	else if ( strcmp(args,"")==0 )
		printf("details are %s", get_options(DBO_DETAILS)?"on":"off");
	else
		gl_error("'%s' is an invalid details option", args);
	return true;
}
bool g_debug::cmd_break(char *args)
{
	return true;
}
bool g_debug::cmd_watch(char *args)
{
	return true;
}
bool g_debug::cmd_list(char *args)
{
	if ( stricmp(args,"help")==0 )
	{
		gl_output("Syntax: list [class|search-spec]");
		gl_output("List objects, limited to class or search-spec");
		gl_output("See 'group' for more info.");
		return true;
	}

	gld_objlist list;
	if ( strlen(args)==0 || strchr(args,'=')!=NULL ) // full search
		list.set(args);
	else if ( strlen(args)>0 ) // just class name
	{
		char buf[1024];
		sprintf(buf,"class=%s",args);
		list.set(buf);
	}
	if ( !list.is_valid() )
	{
		gl_error("list spec '%s' is not valid", args);
		return true;
	}

	int n;
	for ( n=0; n<list.get_size() ; n++ )
	{
		gld_object *obj = get_object(list.get(n));
		gld_class *oclass = obj->get_oclass();

		// name info
		char oname[256];
		strcpy(oname,obj->get_name());
		
		// service status
		char svc = 'A';
		if ( obj->get_in_svc()>gl_globalclock ) svc = 'P';
		else if ( obj->get_out_svc()<gl_globalclock ) svc = 'R';

		// pass status
		char pretopdown = '-';
		char bottomup = '-';
		char posttopdown = '-';

		// lock status
		char lock = obj->get_flags()&OF_LOCKED ? 'l' : '-';

		// PLC code
		char plc = obj->get_flags()&OF_HASPLC ? 'x' : '-';

		// rank
		unsigned int rank = obj->get_rank();

		// clock
		char buffer[64]="-";
		if ( obj->get_clock()>0 )
		{
			gld_clock ts(obj->get_clock());
			ts.to_string(buffer,sizeof(buffer));
		}

		// parent name
		gld_object *parent = obj->get_parent();
		char pname[256] = "";
		if ( parent ) strcpy(pname, parent->get_name());

		// details
		char details[256] = "";
		if ( get_options()&DBO_DETAILS )
		{
			char valid_to[64]="-";
			if ( obj->get_valid_to()>0 )
				gld_clock(obj->get_valid_to()).to_string(valid_to,sizeof(valid_to));
			gld_class *oclass = obj->get_oclass();
			gld_module *module = oclass->get_module();
			sprintf(details,"%s %c%c%c%c%c%c %s/%s/%d ", valid_to,
				obj->get_flags()&OF_RECALC?'c':'-', 
				obj->get_flags()&OF_RERANK?'r':'-', 
				obj->get_flags()&OF_FOREIGN?'f':'-',
				obj->get_flags()&OF_SKIPSAFE?'s':'-',
				obj->get_flags()&OF_FORECAST?'F':'-',
				obj->get_flags()&OF_INIT?'I':(obj->get_flags()&OF_DEFERRED?'i':'-'),
				module ? module->get_name() : "[runtime]", 
				oclass->get_name(), 
				obj->get_id());
		}

		printf("%c%c%c%c%c%c %4d %-24s %-16s %-16s %s\n", 
			svc, pretopdown, bottomup, posttopdown, lock, plc, 
			rank, buffer, oname, pname, details);
	}
	return true;
}
bool g_debug::cmd_print(char *args)
{
	if ( stricmp(args,"help")==0 )
	{
		gl_output("Syntax: print [global|property|name.property]");
		return true;
	}

	// parent property
	if ( args[0]=='.' )
	{
		OBJECT *obj = my();
		while ( *args=='.' )
		{
			OBJECT *parent = obj->parent;
			if ( parent==NULL )
			{
				gld_string name(get_object(obj)->get_name());
				message("object '%s' has no parent", (const char*)name);
				return true;
			}
			obj = parent;
			args++;
		}

		// no args means print whole object
		if ( strcmp(args,"")==0 )
		{
			// TODO
			return true;
		}

		// print only request args
		else
		{
			gld_property parent(obj,args);
			if ( !parent.is_valid() )
			{
				message("property '%s' is not found in object '%s'", args, get_object(obj)->get_name());
				return true;
			}
			gld_string result = parent.get_string();
			if ( result )
				message("%s = '%s'",args,(const char*)result);
			else
				message("no result");
			return true;
		}
	}
	else if ( args[0]==':' )
	{
		args++;

		// try a global property
		gld_property global(args); 
		if ( global.is_valid() )
		{
			gld_string result = global.get_string();
			if ( result )
				message("%s = '%s'",args,(const char*)result);
			else
				gl_debug("no result");
			return true;
		}
		else
		{
			message("'%s' is not a valid global", args);
			return false;
		}
	}
	else if ( strcmp(args,"")==0 )
	{
		// TODO no args means print whole object
		return true;
	}
	// try a local property
	else
	{
		gld_property local(my(),args);
		if ( !local.is_valid() )
		{
			message("property '%s' is not found", args);
			return true;
		}
		gld_string result = local.get_string();
		if ( result )
			message("%s = '%s'",args,(const char*)result);
		else
			message("no result");
		return true;
	}
}
bool g_debug::cmd_signal(char *args)
{
	if ( stricmp(args,"help")==0 )
	{
		gl_output("Syntax: signal signame");
		gl_output("Sends a signal");
		return true;
	}

	struct {
		char *name;
		unsigned int sig;
	} map[] = {
		{"ABRT",SIGABRT},
		{"FPE",SIGFPE},
		{"ILL",SIGILL},
		{"INT",SIGINT},
		{"SEGV",SIGSEGV},
		{"TERM",SIGTERM},
	};
	int n;
	for ( n=0 ; n<sizeof(map)/sizeof(map[0]) ; n++ )
	{
		if ( strcmp(args,map[n].name)==0 )
		{
			message("signaling %d... ", map[n].sig);
			raise(map[n].sig);
			return true;
		}
	}
	n = atoi(args);
	if ( n!=0 )
		raise(n);
	else
		message("no such signal");
	return true;
}
bool g_debug::cmd_history(char *args)
{
	if ( stricmp(args,"help")==0 )
	{
		gl_output("Syntax: history [n]");
		gl_output("Displays command history");
		return true;
	}


	int n = atoi(args);
	int start = command_num - history_size;
	for ( n=start<0?0:start ; (n%history_size)!=history_next ; n++ ) 
	{
		if ( history[n%history_size]!=NULL ) 
			printf("%4d: %s", n, history[n%history_size]);
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// COMMAND WINDOW
////////////////////////////////////////////////////////////////////////////////////////////////
static bool console_initialized = false;
static gld_global major, minor, patch, build, branch, modelname;
static gld_object *selected_object = NULL;
static int selected_item=0;
unsigned long console_object_input_handler(unsigned int input);
unsigned long console_data_input_handler(unsigned int input);
unsigned long console_message_input_handler(unsigned int input);

typedef struct {
	unsigned long (*input_handler)(unsigned int input);
	char left[256];
	char center[256];
	char right[256];
} WINDOWHEADER;

typedef struct {
	unsigned long (*input_handler)(unsigned int input);
	gld_object *selected_object;
} WINDOWLIST;

typedef struct {
	unsigned long (*input_handler)(unsigned int input);
	unsigned int selected_item;
} WINDOWDATA;

typedef struct {
	int len;
	char **list;
	int next;
	int first;
} CONSOLEMESSAGEBUFFER;
typedef struct {
	unsigned long (*input_handler)(unsigned int input);
	CONSOLEMESSAGEBUFFER buffer;
} WINDOWMESSAGES;
typedef enum {WH_PANE, WH_HBAR, WH_VBAR} CONSOLEHIGHLIGHT;

typedef struct {
	unsigned int height, width;
	unsigned int hpos, vpos;
	unsigned int n_panes, pane;
	unsigned int tabstops; // bit mask of panes that are tab stops
	CONSOLEHIGHLIGHT highlight;
	unsigned int nmsg;
	WINDOWHEADER hdr;
	WINDOWLIST list;
	WINDOWDATA data;
	WINDOWMESSAGES msgs;
} CONSOLEWINDOW;
static CONSOLEWINDOW debugwnd = { 
	0,0,	// height,width not determined yet
	-10,	// hpos
	32,		// vpos
	3,		// number of panes in tab sequence
	1,		// selected pane
	0x0e,	// panes that are tab stops (panes 1,2, & 3)
	WH_PANE,// current highlight
	5,		// size of message buffer
	{NULL}, // header input handler
	{console_object_input_handler},
	{console_data_input_handler},
	{console_message_input_handler},
};

unsigned long console_object_input_handler(unsigned int input)
{
	return 0;
}
unsigned long console_data_input_handler(unsigned int input)
{
	return 0;
}
unsigned long console_message_input_handler(unsigned int input)
{
	return 0;
}


int g_debug::console_message(const char *fmt, ...)
{
	// initialize message buffer
	if ( debugwnd.msgs.buffer.len==0 )
	{
		debugwnd.msgs.buffer.len = debugwnd.nmsg;
		debugwnd.msgs.buffer.list = (char**)malloc(sizeof(char*)*debugwnd.nmsg);
		memset(debugwnd.msgs.buffer.list,0,sizeof(char*)*debugwnd.nmsg);
		debugwnd.msgs.buffer.next = 0;
	}
	
	// format contents
	char buffer[1024];
	va_list ptr;
	va_start(ptr,fmt);
	int len = vsprintf(buffer,fmt,ptr);
	va_end(ptr);

	// add message to list
	int pos = debugwnd.msgs.buffer.next;
	if ( debugwnd.msgs.buffer.list[pos]!=NULL )
		free(debugwnd.msgs.buffer.list[pos]);
	char *msg = (char*)malloc(len+1);
	debugwnd.msgs.buffer.list[pos] = msg;
	strcpy(msg,buffer);
	debugwnd.msgs.buffer.next = (debugwnd.msgs.buffer.next+1)%debugwnd.nmsg;

	// adjust first message position only if within panel height of next
	if ( -(debugwnd.msgs.buffer.next-debugwnd.msgs.buffer.first)%debugwnd.nmsg==debugwnd.hpos )
		debugwnd.msgs.buffer.first = (debugwnd.msgs.buffer.first+1)%debugwnd.nmsg;
	return pos;
}

void g_debug::console_show_objlist(gld_object *parent, int level)
{
	unsigned int i;
	gld_object *obj=NULL;
	OBJECT *parent_object = parent?parent->my():NULL;
	for ( i=0,obj=gld_object::get_first(); i<debugwnd.height+debugwnd.hpos-2 && obj!=NULL ; i++,obj=obj->get_next() )
	{
		OBJECT *this_object = obj->my();
		if ( parent_object==this_object->parent )
		{
			bool is_selected = ( obj==selected_object && debugwnd.pane==0 );
			if ( is_selected ) attron(A_BOLD);
			mvprintw(i+2,level,"%-.*s",debugwnd.vpos-level,obj->get_name());
			if ( is_selected ) attroff(A_BOLD);
			console_show_objlist(obj,level+1);
		}
	}
}

bool g_debug::show_line(int row, const char *label, const char *fmt, ...)
{
	static int last_header_row = 0;
	if ( last_header_row==0 || row==2 ) 
		last_header_row=row;

	// ignore lines outside pane
	if ( row<2 || row>(int)(debugwnd.height+debugwnd.hpos-1) ) return false;

	// determine whether selected (i.e., bold)
	bool is_selected = (row-2==selected_item && debugwnd.pane==1 );
	if ( is_selected ) attron(A_BOLD);
	
	// heading item
	if ( fmt==NULL ) 
	{
		int i=0;
		for ( i=last_header_row+1 ; i<row ; i++ )
			mvprintw(i,debugwnd.vpos+1,"|");
		mvprintw(row,debugwnd.vpos+1, "+- %s", label);
		last_header_row = row;
	}
	
	// data item
	else
	{
		va_list ptr;
		va_start(ptr,fmt);
		char buffer[1024];
		vsprintf(buffer,fmt,ptr);
		va_end(ptr);
		mvprintw(row,debugwnd.vpos+1,"  |- %-12s %s", label,strcmp(buffer,"\"\"")==0?"":buffer);
	}
	if ( is_selected && debugwnd.pane==1 ) attroff(A_BOLD);
	return true;
}
void g_debug::console_show_object(void)
{
	OBJECT *obj = my();
	unsigned int row=2, col=debugwnd.vpos+1, tab=48;
	char buffer[1024];
	sprintf(buffer,"Header %s",get_oclass()->get_name()); 
	show_line(row,buffer);
	show_line(++row,"id","%d",get_id());
	show_line(++row,"group","%s",get_groupid());
	show_line(++row,"parent","%s",get_parent()?get_parent()->get_name():"");
	show_line(++row,"rank","%d",get_rank());
	show_line(++row,"clock","%s",(const char*)gld_clock(get_clock()).get_string());
	show_line(++row,"valid","%s",(const char*)gld_clock(get_valid_to()).get_string());
	show_line(++row,"skew","%s",(const char*)gld_clock(get_schedule_skew()).get_string());
//	show_line(++row,"forecast","");
	show_line(++row,"latitude","%.6f",callback->geography.latitude.to_string(get_latitude(),buffer,sizeof(buffer))?buffer:"(NA)");
	show_line(++row,"longitude","%.6f",callback->geography.longitude.to_string(get_longitude(),buffer,sizeof(buffer))?buffer:"(NA)");
	show_line(++row,"in","%s",(const char*)gld_clock(get_in_svc()).get_string());
	show_line(++row,"out","%s",(const char*)gld_clock(get_out_svc()).get_string());
//	show_line(++row,"time","");
//	show_line(++row,"space","");
	show_line(++row,"lock","%u",get_lock());
	show_line(++row,"rng_state","%u",get_rng_state());
	show_line(++row,"heartbeat","%llu s",get_heartbeat());
	show_line(++row,"flags","0x%08x",get_flags());
	sprintf(buffer,"Body %s",get_oclass()->get_name()); 
	show_line(++row,buffer);
	gld_property prop(my());
	for ( ; prop.is_valid() ; prop.set_property(prop.get_next()) )
	{
		if ( row<debugwnd.height-2 )
			show_line(++row,prop.get_name(),(const char*)prop.get_string());
	}
}

void g_debug::console_show_messages(void)
{
	int msg;
	unsigned int row=debugwnd.height+debugwnd.hpos+1;
	char blank[1024];
	memset(blank,' ',sizeof(blank));
	blank[debugwnd.width]='\0';
	for ( msg=debugwnd.msgs.buffer.first ; msg!=debugwnd.msgs.buffer.next && row<=debugwnd.height ; msg=(msg+1)%debugwnd.nmsg, row++ )
	{
		mvprintw(row,0,"%s",blank);
		mvprintw(row,0,"[%d] %s",msg,debugwnd.msgs.buffer.list[msg]);
	}
}

bool g_debug::console_get_command(void)
{
	if ( !console_initialized )
	{
		console_load();
		initscr();
		cbreak();
		echo();
		intrflush(stdscr,TRUE);
		keypad(stdscr,TRUE);
		refresh();
		halfdelay(1);
		major.get("version.major");
		minor.get("version.minor");
		patch.get("version.patch");
		build.get("version.build");
		branch.get("version.branch");
		modelname.get("modelname");
		console_initialized = true;
		console_message("Ready.");
	}
	bool run = true;
	typedef enum {
		R_NONE		= 0x00, 
		R_CLOCK		= 0x01, 
		R_OBJLIST	= 0x02, 
		R_OBJECT	= 0x04, 
		R_PROPERTY	= 0x08, 
		R_MESSAGES	= 0x10,
		R_PANES		= R_OBJLIST|R_OBJECT,
		R_ALL		= 0xffff 
	} REFRESHFLAGS;
	unsigned long refresh_flags = R_ALL;
	unsigned long refresh_count = 0;
	selected_object = this;
	while ( run ) 
	{
		if ( refresh_count++%10==0 )
			refresh_flags|=R_CLOCK;

		if ( debugwnd.width!=getwidth() || debugwnd.height!=getheight() )
			refresh_flags = R_ALL;

		if ( refresh_flags==R_ALL ) 
		{
			debugwnd.width = getwidth();
			debugwnd.height = getheight();
			clear();

			// horizontal bars
			char sbar[1024];
			char dbar[1024];
			unsigned int i;
			for ( i=0 ; i<debugwnd.width ; i++ ) 
			{
				sbar[i]='-';
				dbar[i]='=';
			}
			sbar[i] = dbar[i] = '\0';

			// status bar split
			if ( debugwnd.highlight==WH_HBAR ) attron(A_BOLD);
			mvprintw(debugwnd.height+debugwnd.hpos,0,"%s",sbar);
			if ( debugwnd.highlight==WH_HBAR ) attroff(A_BOLD);

			// vertical bar split
			if ( debugwnd.highlight==WH_VBAR ) attron(A_BOLD);
			for ( i=2 ; i<debugwnd.height+debugwnd.hpos ; i++ )
				mvprintw(i,debugwnd.vpos,"%c",'|');
			if ( debugwnd.highlight==WH_VBAR ) attroff(A_BOLD);

			// header bar data
			gld_string m = modelname.get_string();
			gld_string b = branch.get_string();
			mvprintw(0,0,"GridLAB-D Debug Console - Version %d.%d.%d-%d (%s)",
				major.get_int32(), minor.get_int32(), patch.get_int32(), build.get_int32(), 
				b.get_buffer());
			mvprintw(0,(int)(debugwnd.width/2-m.get_length()/2-2)," [%s] ",m.get_buffer());

			// header bar split
			mvprintw(1,0,dbar);
		}
		
		// clock
		if ( refresh_flags&R_CLOCK )
		{
			gld_clock now;
			gld_string n = now.get_string();
			mvprintw(0,(int)(debugwnd.width-n.get_length()-1)," %s",n.get_buffer());
		}

		// entries in navigation panel
		if ( refresh_flags&R_OBJLIST )
			console_show_objlist();

		// entries in browsing panel
		if ( refresh_flags&R_OBJECT )
			console_show_object();

		// message buffer
		if ( refresh_flags&R_MESSAGES )
			console_show_messages();

		refresh_flags = R_NONE;
		int c = wgetch(stdscr);
		switch (c) {
		case 'N':
			run = false;
			break;
		case 'R':
			refresh_flags = R_ALL;
			break;
		case 'Q':
		case 'X':
			if ( console_confirm("Are you sure you want to exit (Y,N)? [N] ") )
			{
				console_save();
				clear();
				exit(1);
			}
			break;
		case 191: // forward slash
			console_help();
			refresh_flags = R_ALL;
			break;

		case 'H': // select the horizontal line
			if ( debugwnd.highlight!=WH_HBAR )
				debugwnd.highlight = WH_HBAR;
			else
				debugwnd.highlight = WH_PANE;
			refresh_flags = R_ALL;
			break;
		case 'V': // select the vertical line
			if ( debugwnd.highlight!=WH_VBAR )
				debugwnd.highlight = WH_VBAR;
			else
				debugwnd.highlight = WH_PANE;
			refresh_flags = R_ALL;
			break;
		case KEY_DOWN:
			if ( debugwnd.highlight==WH_PANE )
			{
				if ( selected_item<(int)(debugwnd.height+debugwnd.hpos-3) ) selected_item++;
				refresh_flags = R_OBJECT;
			}
			else if ( debugwnd.highlight==WH_HBAR )
			{
				if ( debugwnd.hpos<-1 )
				{
					debugwnd.hpos++;
					refresh_flags = R_ALL;
				}
			}
			break;
		case KEY_UP:
			if ( debugwnd.highlight==WH_PANE )
			{
				if ( selected_item<(int)(debugwnd.height+debugwnd.hpos-3) ) selected_item--;
				refresh_flags = R_OBJECT;
			}
			else if ( debugwnd.highlight==WH_HBAR )
			{
				if ( debugwnd.height+debugwnd.hpos>4 )
				{
					debugwnd.hpos--;
					refresh_flags = R_ALL;
				}
			}
			break;
		case KEY_LEFT:
			if ( debugwnd.highlight==WH_VBAR )
			{
				if ( debugwnd.vpos>4 )
				{
					debugwnd.vpos--;
					refresh_flags = R_ALL;
				}
			}
			break;
		case KEY_RIGHT:
			if ( debugwnd.highlight==WH_VBAR )
			{
				debugwnd.vpos++;
				refresh_flags = R_ALL;
			}
			break;		
		case '\t':
			debugwnd.pane = (debugwnd.pane+1)%debugwnd.n_panes;
			refresh_flags = R_PANES;
			break;
		default:
			// all other keys handled by panes
			switch ( debugwnd.pane )
			{
			case 1: 
				refresh_flags = debugwnd.list.input_handler(c);
				break;
			case 2:
				refresh_flags = debugwnd.data.input_handler(c);
				break;
			default:
				break;
			}			
			break;
		}
	}
	return true;
}

bool g_debug::console_confirm(const char *msg)
{
	console_message("%s",msg);
	console_show_messages();
	while ( 1 )
	{
		int c = wgetch(stdscr);
		if ( c=='Y' ) return true;
		if ( c=='N' || c=='\n' ) return false;
	}
}

static const char *helpdata[] = {
	// page 0
	"Welcome to the GridLAB-D Debugger Help Pages\n"
	"\n"
	"Contents:\n"
	" [1]   Basic commands\n"
	" [2]   Debugging principles\n"
	" [3]   Navigation\n"
	" [4]   Window setup\n"
	"\n"
	"Q to Quit help, Left/Right to navigate pages.\n",

	// page 1
	"Basic commands\n"
	"\n"
	" Q       Quit and save current layout\n"
	" X       Exit without saving current layout\n",

	// page 2
	"Debugging principles\n"
	"\n"
	"TODO",

	// page 3
	"Navigation\n"
	"\n"
	" Tab     Move to the next page\n"
	" Up      Move the cursor up to the previous item displayed\n"
	" Down    Move the cursor down to the next item displayed\n"
	" Left    Move the cursor to the parent item (closes the child item) (if any)\n"
	" Right   Move the cursor to the first child item (if any)\n"
	" Enter   Begin editing the current item\n"
	"\n",

	// page 4
	"Window Setup\n"
	"\n"
	" H      Toggles horizontal bar position control (using Up/Down arrows)\n"
	" V      Toggles vertical bar position control (using Up/Down arrows)\n",
};

void g_debug::console_help(unsigned int page, unsigned int row)
{
	unsigned int n_pages = sizeof(helpdata)/sizeof(helpdata[0]);
Again:
	clear();
	mvprintw(0,0,helpdata[page]);

	unsigned int done=0;
	while ( !done )
	{
		unsigned int c = wgetch(stdscr);
		if ( c=='Q') done=1;
		else if ( c==KEY_LEFT && page>0 )
		{
			page--; 
			goto Again;
		}
		else if ( c==KEY_RIGHT && page<n_pages-1 )
		{
			page++;
			goto Again;
		}
	}
	return;
}

void g_debug::console_load(void)
{
	FILE *fp = fopen("gridlabd_debug.cfg","r");
	if ( fp==NULL ) return;
	fscanf(fp,"%d,%d,%d,%d",
		&debugwnd.height, &debugwnd.width,
		&debugwnd.hpos, &debugwnd.vpos);
	fclose(fp);
}

void g_debug::console_save(void)
{
	FILE *fp = fopen("gridlabd_debug.cfg","w");
	if ( fp==NULL ) return;
	fprintf(fp,"%d,%d,%d,%d",
		debugwnd.height, debugwnd.width,
		debugwnd.hpos, debugwnd.vpos);
	fclose(fp);
}
