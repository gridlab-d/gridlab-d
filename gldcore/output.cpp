/** $Id: output.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file output.c
	@author David P. Chassin
	@addtogroup output Output functions
	@ingroup core
	
	Implements functions that send output to the current environment's console, 
	error stream, and test result string using printf style args.

	Debug, warning, errors, fatal messages should provide an indicate of the context in which
	the message is generated.

	- <b>Core messages</b> should be produced any time the core takes a off-normal path in
	the flow of the codes.  The severity of the situation determines which type of message
	it is and this is left to the programmers judgement, save to say that if the situation is not
	expected to seriously affect the result, it should be a warning; if the results are likely
	to be seriously affected, it should be an error; and if the result is hopelessly compromised
	it should be a fatal error.  
	\code
	output_fatal("module_load(file='%s', argc=%d, argv=['%s',...]): intrinsic %s is not defined in module", file, argc,argc>0?argv[0]:"",fname);
	\endcode

	- <b>Module messages</b> should be produced un the same guidelines as core messages, except that
	fatal messages are not supported for modules.  However, the context should provide the object, if known
	\code
	 gl_error("office:%d occupancy_schedule '%s' day '%c' is invalid", OBJECTHDR(this)->id, block, *p);
	\endcode

	@{
 **/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "output.h"
#include "globals.h"
#include "exception.h"
#include "lock.h"
#include "module.h"

static unsigned long output_lock = 0;
static char buffer[65536];
#define CHECK 0xcdcd
int overflow=CHECK;
int flush = 0;

static char prefix[16]="";
void output_prefix_enable(void)
{
	unsigned short cpuid, procid;
	sched_init(0);
	output_debug("reading cpuid()");
	cpuid = sched_get_cpuid(0);
	output_debug("reading procid()");
	procid = sched_get_procid();
	output_debug("sprintf'ing m/s name");
	switch ( global_multirun_mode ) {
	case MRM_STANDALONE:
		sprintf(prefix,"-%02d(%05d): ", cpuid, procid);
		break;
	case MRM_MASTER:
		flush = 1;
		sprintf(prefix,"M%02d(%05d): ", cpuid, procid);
		break;
	case MRM_SLAVE:
		flush = 1;
		sprintf(prefix,"S%02d(%05d): ", cpuid, procid);
		break;
	default:
		break;
	}
	output_debug("exiting output_prefix_enable");
}

/** output_redirect() changes where output message are sent 
	instead of the usual stdin, stdout streams
 **/
static struct s_redirection {
	FILE *output;
	FILE *error;
	FILE *warning;
	FILE *debug;
	FILE *verbose;
	FILE *profile;
	FILE *progress;
} redirect;
FILE* output_redirect_stream(char *name, FILE *fp)
{
	struct {
		char *name;
		FILE **file;
		char *defaultfile;
	} map[] = {
		{"output",&redirect.output,"gridlabd.out"},
		{"error",&redirect.error,"gridlabd.err"},
		{"warning",&redirect.warning,"gridlabd.wrn"},
		{"debug",&redirect.debug,"gridlabd.dbg"},
		{"verbose",&redirect.verbose,"gridlabd.inf"},
		{"profile",&redirect.profile,"gridlabd.pro"},
		{"progress",&redirect.progress,"gridlabd.prg"},
	};
	int i;
	for (i=0; i<sizeof(map)/sizeof(map[0]); i++) 
	{
		if (strcmp(name,map[i].name)==0)
		{
			char *mode = "w";
			FILE *oldfp = *(map[i].file);
			*(map[i].file) = fp;
#ifndef WIN32
			if (*(map[i].file))
				setlinebuf(*(map[i].file));
#endif
			return oldfp;
		}
	}
	return NULL;
}

void (*notify_error)(void) = NULL;
int output_notify_error(void (*notify)(void))
{
	notify_error = notify;
	return 0;
}

FILE* output_redirect(char *name, char *path)
{
	struct {
		char *name;
		FILE **file;
		char *defaultfile;
	} map[] = {
		{"output",&redirect.output,"gridlabd.out"},
		{"error",&redirect.error,"gridlabd.err"},
		{"warning",&redirect.warning,"gridlabd.wrn"},
		{"debug",&redirect.debug,"gridlabd.dbg"},
		{"verbose",&redirect.verbose,"gridlabd.inf"},
		{"profile",&redirect.profile,"gridlabd.pro"},
		{"progress",&redirect.progress,"gridlabd.prg"},
	};
	int i;
	for (i=0; i<sizeof(map)/sizeof(map[0]); i++) 
	{
		if (strcmp(name,map[i].name)==0)
		{
			char *mode = "w";
			if (*(map[i].file)!=NULL)
				fclose(*(map[i].file));
			
			/* test for append mode, path led with + */
			if (path != NULL && path[0]=='+')
			{	mode = "a";
				path++;
			}
			*(map[i].file) = fopen(path?path:map[i].defaultfile,"w");
#ifndef WIN32
			if (*(map[i].file))
				setlinebuf(*(map[i].file));
#endif
			return *(map[i].file);
		}
	}
	return NULL;
}

/**	prep_stream() sets the curr_stream objects to stdin, stdout, and stderr, since VS2005 is picky
	about non-constant initializers.
**/
static FILE *curr_stream[3] = {NULL, NULL, NULL};
static int stream_prep = 0;
static void prep_stream(){
	if(stream_prep)
		return;
	stream_prep = 1;
	if(curr_stream[FS_IN] == NULL){
		curr_stream[FS_IN] = stdin;
#ifdef DEBUG
		if (global_verbose_mode) printf("    ... prep_stream() set FS_IN to stdin\n");
#endif
	}
	if(curr_stream[FS_STD] == NULL){
		curr_stream[FS_STD] = stdout;
#ifdef DEBUG
		if (global_verbose_mode) printf("    ... prep_stream() set FS_STD to stdout\n");
#endif
	}
	if(curr_stream[FS_ERR] == NULL){
		curr_stream[FS_ERR] = stderr;
#ifdef DEBUG
		if (global_verbose_mode) printf("    ... prep_stream() set FS_ERR to stderr\n");
#endif
	}
	return;
}

int output_init(int argc,char *argv[])
{
	atexit(output_cleanup);
	return 1;
}

void output_cleanup(void)
{
	/* NULL purges buffers */
	output_verbose(NULL);
	output_warning(NULL);
	output_error(NULL);
	output_fatal(NULL);
	output_message(NULL);
	output_debug(NULL);
}

static int default_printstd(char *format,...)
{
	int count;
	va_list ptr;
	prep_stream();
	va_start(ptr,format);
	count = vfprintf(curr_stream[FS_STD],format,ptr);
	if ( flush ) fflush(curr_stream[FS_STD]);
	va_end(ptr);
	return count;
}

static int default_printerr(char *format,...)
{
	int count;
	va_list ptr;
	prep_stream();
	va_start(ptr,format);
	count = vfprintf(curr_stream[FS_ERR],format,ptr);
	va_end(ptr);
	fflush(curr_stream[FS_ERR]);
	return count;
}

FILE *output_set_stream(FILESTREAM fs, FILE *newfp){
	FILE *oldfp = curr_stream[fs];
	if(fs > FS_ERR)	/* input check */
		return NULL;
	if(newfp == NULL)
		return NULL;
	curr_stream[fs] = newfp;
	return oldfp;
}

static PRINTFUNCTION printstd=default_printstd, printerr=default_printerr;

/**	Sets stderr to stdout

	This was requested to keep all the output consistantly going to the same output
	for the Java GUI, since catching messages from both stderr and stdout was
	causing difficulties.
 **/
void output_both_stdout(){
	curr_stream[FS_STD] = stdout;
	curr_stream[FS_ERR] = stdout;
}

/** Set the stdout print function

	The default function sends normal message strings to the stdout stream.  Applications
	that need a handle such messages differently can request they be sent
	to the function \p call.
 **/
PRINTFUNCTION output_set_stdout(PRINTFUNCTION call) /**< The \b printf style function to call to send text to \e stdout */
{
	PRINTFUNCTION old = printstd;
	printstd = call;
	return old;
}

/** Set the stderr print function

	The default function sends error message strings to the stderr stream.  Applications
	that need a handle such messages differently can request they be sent
	to the function \p call.
 **/
PRINTFUNCTION output_set_stderr(PRINTFUNCTION call) /**< The \b printf style function to call to send text to \e stderr */
{
	PRINTFUNCTION old = printerr;
	printerr = call;
	return old;
}

static char time_context[256]="INIT";
void output_set_time_context(TIMESTAMP ts)
{
	convert_from_timestamp(ts,time_context,sizeof(time_context)-1);
}
void output_set_delta_time_context(TIMESTAMP ts, DELTAT delta_ts)
{
	convert_from_timestamp_delta(ts,delta_ts,time_context,sizeof(time_context)-1);
}
char *output_get_time_context(void)
{
	return time_context;
}

/** Output a fatal error message
	
	output_fatal() will produce output to the standard output stream.  
	Error messages are always preceded by the string "FATAL: " 
	and a newline is always appended to the message.
 **/
int output_fatal(const char *format,...) /**< \bprintf style argument list */
{
	/* check for repeated message */
	static char lastfmt[4096] = "";
	static int count=0;
	int result = 0;
	wlock(&output_lock);
	if (format!=NULL && strcmp(lastfmt,format)==0 && global_suppress_repeat_messages && !global_verbose_mode)
	{
		count++;
		goto Unlock;
	}
	else
	{
		va_list ptr;
		int len=0;
		strncpy(lastfmt,format?format:"",sizeof(lastfmt)-1);
		if (count>0 && global_suppress_repeat_messages && !global_verbose_mode)
		{
			len = sprintf(buffer,"last fatal error message was repeated %d times", count);
			count = 0;
			if(format == NULL) goto Output;
			else len += sprintf(buffer+len,"\n%sFATAL    [%s] : ",prefix, time_context);
		}
		else if (format==NULL)
			goto Unlock;
		va_start(ptr,format);
		vsprintf(buffer+len,format,ptr); /* note the lack of check on buffer overrun */
		va_end(ptr);
	}
Output:
	if (redirect.error)
		result = fprintf(redirect.error,"%sFATAL    [%s] : %s\n", prefix, time_context, buffer);
	else
		result = (*printerr)("%sFATAL    [%s] : %s\n", prefix, time_context, buffer);
Unlock:
	wunlock(&output_lock);
	return result;
}

/** Output an error message to the stdout stream using printf style argument processing
	
	output_error() will produce output to the standard output stream.  
	Error messages are always preceded by the string "ERROR: " 
	and a newline is always appended to the message.
 **/
int output_error(const char *format,...) /**< \bprintf style argument list */
{
	/* check for repeated message */
	static char lastfmt[4096] = "";
	static int count=0;
	int result = 0;
	wlock(&output_lock);
	if (format!=NULL && strcmp(lastfmt,format)==0 && global_suppress_repeat_messages && !global_verbose_mode)
	{
		count++;
		goto Unlock;
	}
	else
	{
		va_list ptr;
		int len=0;
		strncpy(lastfmt,format?format:"",sizeof(lastfmt)-1);
		if (count>0 && global_suppress_repeat_messages && !global_verbose_mode)
		{
			len = sprintf(buffer,"last error message was repeated %d times", count);
			count = 0;
			if(format == NULL) goto Output;
			else len += sprintf(buffer+len,"\n%sERROR    [%s] : ", prefix, time_context);
		}
		else if (format==NULL)
			goto Unlock;
		va_start(ptr,format);
		vsprintf(buffer+len,format,ptr); /* note the lack of check on buffer overrun */
		va_end(ptr);
	}
Output:

	if (notify_error!=NULL)
		(*notify_error)();

	if (redirect.error)
		result = fprintf(redirect.error,"%sERROR    [%s] : %s\n", prefix, time_context, buffer);
	else
		result = (*printerr)("%sERROR    [%s] : %s\n", prefix, time_context, buffer);
Unlock:
	wunlock(&output_lock);
	return result;
}

/** Output an error message to the stdout stream using printf style argument processing
	
	output_error() will produce output to the standard output stream.  
	Error messages are always preceded by the string "ERROR: " 
	and a newline is always appended to the message.
 **/
int output_error_raw(const char *format,...) /**< \bprintf style argument list */
{
	/* check for repeated message */
	static char lastfmt[4096] = "";
	static int count=0;
	int result = 0;
	wlock(&output_lock);
	if (format!=NULL && strcmp(lastfmt,format)==0 && global_suppress_repeat_messages && !global_verbose_mode)
	{
		count++;
		goto Unlock;
	}
	else
	{
		va_list ptr;
		int len=0;
		strncpy(lastfmt,format?format:"",sizeof(lastfmt)-1);
		if (count>0 && global_suppress_repeat_messages && !global_verbose_mode)
		{
			len = sprintf(buffer,"last error message was repeated %d times", count);
			count = 0;
			if(format == NULL) goto Output;
			else len += sprintf(buffer+len,"\n");
		}
		else if (format==NULL)
			goto Unlock;
		va_start(ptr,format);
		vsprintf(buffer+len,format,ptr); /* note the lack of check on buffer overrun */
		va_end(ptr);
	}
Output:

	if (notify_error!=NULL)
		(*notify_error)();

	if (redirect.error)
		result= fprintf(redirect.error,"%s%s\n", prefix, buffer);
	else
		result= (*printerr)("%s%s\n", prefix, buffer);
Unlock:
	wunlock(&output_lock);
	return result;
}

/** Output an test message to the stdout stream using printf style argument processing
	
	output_test() will produce output to the test output file defined by the 
	variable \p global_testoutputfile.
	A newline is always appended to the message. 
 **/
int output_test(const char *format,...) /**< \bprintf style argument list */
{
	static FILE *fp = NULL;
	char minor_b[32], major_b[32];
	char testoutputfilename[1024];
	char commandline[256];
	va_list ptr;

	int result = 0;
	wlock(&output_lock);

	if(format == NULL){
		goto Unlock;
	}

	va_start(ptr,format);
	vsprintf(buffer,format,ptr); /* note the lack of check on buffer overrun */
	va_end(ptr);

	if (fp==NULL)
	{
		time_t now = time(NULL);
		fp = fopen(global_getvar("testoutputfile", testoutputfilename, 1023), "w");
		if (fp==NULL)
		{
			/* can't write to output file, write to stderr instead */
			return (*printerr)("TEST: %s\n",buffer);
		}
		fprintf(fp,"GridLAB-D Version %s.%s\n", global_getvar("version.major", major_b, 32), global_getvar("version.minor", minor_b, 32));
		fprintf(fp,"Test results from run started %s", asctime(localtime(&now)));
		fprintf(fp,"Command line: %s\n", global_getvar("command_line", commandline, 255));
		if ( global_multirun_mode!=MRM_STANDALONE )
			fprintf(fp,"Instance: %s\n", prefix);
	}

	result = fprintf(fp,"%s\n", buffer);
Unlock:
	wunlock(&output_lock);
	return result;

}

/** Output a warning message to the stdout stream using printf style argument processing
	
	output_warning() will produce output to the standard output stream only when the
	\p global_warn_mode variable is not \b 0.  Warning messages are always preceded by
	the string "WARNING: " and a newline is always appended to the message.
 **/
int output_warning(const char *format,...) /**< \bprintf style argument list */
{
	if (global_warn_mode)
	{
		/* check for repeated message */
		static char lastfmt[4096] = "";
		static int count=0;
		int result = 0;
		wlock(&output_lock);
		if (format!=NULL && strcmp(lastfmt,format)==0 && global_suppress_repeat_messages && !global_verbose_mode)
		{
			count++;
			goto Unlock;
		}
		else
		{
			va_list ptr;
			int len=0;
			strncpy(lastfmt,format?format:"",sizeof(lastfmt)-1);
			if (count>0 && global_suppress_repeat_messages && !global_verbose_mode)
			{
				len = sprintf(buffer,"last warning message was repeated %d times", count);
				count = 0;
				if(format == NULL) goto Output;
				else len += sprintf(buffer+len,"\n%sWARNING  [%s] : ", prefix, time_context);
			}
			else if (format==NULL)
				goto Unlock;
			va_start(ptr,format);
			vsprintf(buffer+len,format,ptr); /* note the lack of check on buffer overrun */
			va_end(ptr);
		}
Output:
		if (redirect.warning)
			result = fprintf(redirect.warning,"%sWARNING  [%s] : %s\n", prefix, time_context, buffer);
		else
			result = (*printerr)("%sWARNING  [%s] : %s\n", prefix, time_context, buffer);
Unlock:
		wunlock(&output_lock);
		return result;
	}
	return 0;
}

/** Output a debug message to the stdout stream using printf style argument processing
	
	output_debug() will produce output to the standard output stream only when the
	\p global_debug_output variable is not \b 0.  Debug messages are always preceded by
	the string "DEBUG: " and a newline is always appended to the message.
 **/
int output_debug(const char *format,...) /**< \bprintf style argument list */
{
	if (global_debug_output)
	{
		/* check for repeated message */
		static char lastfmt[4096] = "";
		static int count=0;
		int result = 0;
		wlock(&output_lock);
		if (format!=NULL && strcmp(lastfmt,format)==0 && global_suppress_repeat_messages && !global_verbose_mode)
		{
			count++;
			goto Unlock;
		}
		else
		{
			va_list ptr;
			int len=0;
			strncpy(lastfmt,format?format:"",sizeof(lastfmt)-1);
			if (count>0 && global_suppress_repeat_messages && !global_verbose_mode)
			{
				len = sprintf(buffer,"last debug message was repeated %d times", count);
				count = 0;
				if(format == 0) goto Output;
				else len += sprintf(buffer+len,"\n%sDEBUG [%s] : ", prefix, time_context);
			}
			else if (format==NULL)
				goto Unlock;
			va_start(ptr,format);
			vsprintf(buffer+len,format,ptr); /* note the lack of check on buffer overrun */
			va_end(ptr);
		}
Output:
		if (redirect.debug)
			result = fprintf(redirect.debug,"%sDEBUG [%s] : %s\n", prefix, time_context, buffer);
		else
			result = (*printerr)("%sDEBUG [%s] : %s\n", prefix, time_context, buffer);
Unlock:
		wunlock(&output_lock);
		return result;
	}
	return 0;
}

/** Output a verbose message to the stdout stream using printf style argument processing
	
	output_verbose() will produce output to the standard output stream only when the
	\p global_verbose_mode variable is not \b 0.  Verbose message always have
	leading spaces and an ellipsis printed before the string, and 
	a newline is always appended to the message.
 **/
int output_verbose(const char *format,...) /**< \bprintf style argument list */
{
	if (global_verbose_mode)
	{
		/* check for repeated message */
		static char lastfmt[4096] = "";
		static int count=0;
		int result = 0;
		wlock(&output_lock);
		if (format!=NULL && strcmp(lastfmt,format)==0 && global_suppress_repeat_messages && !global_verbose_mode)
		{
			count++;
			goto Unlock;
		}
		else
		{
			va_list ptr;
			int len=0;
			strncpy(lastfmt,format?format:"",sizeof(lastfmt)-1);
			if (count>0 && global_suppress_repeat_messages && !global_verbose_mode)
			{
				len = sprintf(buffer,"%slast verbose message was repeated %d times\n   ... ", prefix, count);
				count = 0;
				if(format == 0) goto Output;
			}
			else if (format==NULL)
				goto Unlock;
			va_start(ptr,format);
			vsprintf(buffer+len,format,ptr); /* note the lack of check on buffer overrun */
			va_end(ptr);
		}
Output:
		if (redirect.verbose)
			result = fprintf(redirect.verbose,"%s%s\n", prefix, buffer);
		else
			result = (*printerr)("%s   ... %s\n", prefix, buffer);
Unlock:
		wunlock(&output_lock);
	return result;
	}
	return 0;
}
/** Output a message to the stdout stream using printf style argument processing
	
	output_message() will produce output to the standard output stream only when the
	\p global_quiet_mode variable is not \b 0.  A newline is always appended to the message.
 **/
int output_message(const char *format,...) /**< \bprintf style argument list */
{
	if (!global_quiet_mode)
	{
		/* check for repeated message */
		static char lastfmt[4096] = "";
		static int count=0;
		size_t sz = strlen(format?format:"");
		int result = 0;
		wlock(&output_lock);
		if (format!=NULL && strcmp(lastfmt,format)==0 && global_suppress_repeat_messages && !global_verbose_mode)
		{
			count++;
			goto Unlock;
		}
		else
		{
			va_list ptr;
			int len=0;
			strncpy(lastfmt,format?format:"",sizeof(lastfmt)-1);
			if (count>0 && global_suppress_repeat_messages && !global_verbose_mode)
			{
				len = sprintf(buffer,"%slast message was repeated %d times\n", prefix, count);
				count = 0;
				if(format == NULL) goto Output;
			}
			if (format==NULL)
				goto Unlock;
			va_start(ptr,format);
			vsprintf(buffer+len,format,ptr); /* note the lack of check on buffer overrun */
			va_end(ptr);
		}
Output:
		if (redirect.output)
			result = fprintf(redirect.output,"%s%s\n", prefix, buffer);
		else
			result = (*printstd)("%s%s\n", prefix, buffer);
Unlock:
		wunlock(&output_lock);
		return result;
	}
	return 0;
}

/** Output a profiler message
 **/
int output_profile(const char *format, ...) /**< /bprintf style argument list */
{
	char tmp[1024];
	va_list ptr;

	va_start(ptr,format);
	vsprintf(tmp,format,ptr);
	va_end(ptr);

	if (redirect.profile!=NULL)
		return fprintf(redirect.profile,"%s%s\n", prefix, tmp);
	else
		return (*printstd)("%s%s\n", prefix, tmp);
}

/** Output a progress report
 **/
int output_progress()
{
	char buffer[64];
	int res = 0;
	char *ts; 

	/* handle delta mode highres time */
	if ( global_simulation_mode==SM_DELTA )
	{
		DATETIME t;
		unsigned int64 secs = global_deltaclock/1000000000;
		local_datetime(global_clock+secs,&t);
		t.nanosecond = (unsigned int)(global_deltaclock-secs*1000000000);
		strdatetime(&t,buffer,sizeof(buffer));
		ts = buffer;
	}
	else
		ts = convert_from_timestamp(global_clock,buffer,sizeof(buffer))>0?buffer:"(invalid)";

	if (redirect.progress)
	{
		res = fprintf(redirect.progress,"%s\n",ts);
		fflush(redirect.progress);
	}
	else if (global_keep_progress)
		res = output_message("%sProcessing %s...", prefix, ts);
	else
	{
		static int len=0;
		int i=len, slen = (int)strlen(ts)+15;
		while (i--) putchar(' ');
		putchar('\r');
		if (slen>len) len=slen;
		res = output_raw("%sProcessing %s...\r", prefix, ts);
	}
	return res;
}

/** Output a raw string to the stdout stream using printf style argument processing
	
	output_raw() will produce output to the standard output stream only when the
	\p global_quiet_mode variable is not \b 0.  
 **/
int output_raw(const char *format,...) /**< \bprintf style argument list */
{
	if (!global_quiet_mode)
	{
		va_list ptr;
		int result = 0;
		wlock(&output_lock);

		va_start(ptr,format);
		vsprintf(buffer,format,ptr); /* note the lack of check on buffer overrun */
		va_end(ptr);

			if (redirect.output)
			{	int len = fprintf(redirect.output,"%s%s", prefix, buffer);
				fflush(redirect.output);
				result =  len;
			}
			else
				result = (*printerr)("%s%s",prefix, buffer);
		wunlock(&output_lock);
		return result;
	}
	return 0;
}

#include "module.h"

/** Output the XSD snippet of a class */
int output_xsd(char *spec)
{
	MODULE *mod = NULL;
	CLASS *oclass = NULL;
	char modulename[1024], classname[1024]="";
	char buffer[65536];
	/*if(sscanf(spec, "%[A-Za-z_0-9]::%[A-Za-z_0-9]:%s",modulename, submodulename, classname) == 3)
	{
		sprintf(jointname, "%s::%s", modulename, submodulename);
		mod = module_load(jointname, 0, NULL);
		if(mod == NULL){
			output_error("unable to load parent module %s", modulename);
			return 0;
		}
		
	}
	else if(sscanf(spec, "%[A-Za-z_0-9]::%[A-Za-z_0-9]",modulename, submodulename) == 2)
	{
		sprintf(jointname, "%s::%s", modulename, submodulename);
		mod = module_load(jointname, 0, NULL);
		if(mod == NULL){
			output_error("unable to load parent module %s", modulename);
			return 0;
		}
	}
	else */if (sscanf(spec,"%[A-Za-z_0-9]:%s",modulename,classname)<1)
	{
		output_error("improperly formatted XSD dump specification");
		return 0;
	}
	if (mod == NULL)
		mod = module_load(modulename,0,NULL);
	if (mod==NULL)
	{
		output_error("unable to find module '%s'", spec);
		return 0;
	}
	if (classname[0]!='\0' && (oclass=class_get_class_from_classname(classname))==NULL)
	{
		output_error("unable to find class '%s' in module '%s'", classname, modulename);
		return 0;
	}
	//if ((strlen(submodulename) > 1))
	//	strcpy(modulename, submodulename);
	output_message("<?xml version=\"1.0\" encoding=\"utf-%d\"?>",global_xml_encoding);
	output_message("<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" targetNamespace=\"http://www.w3.org/\" xmlns=\"http://www.w3.org/\" elementFormDefault=\"qualified\">\n");
	for (oclass=(classname[0]!='\0'?oclass:class_get_first_class()); oclass!=NULL; oclass=oclass->next) 
	{
		if (class_get_xsd(oclass,buffer,sizeof(buffer))<=0)
		{
			output_error("unable to convert class '%s' to XSD", oclass->name);
			return 0;
		}
		output_message(buffer);
		if (classname[0]!='\0')	/**  is this needed? -mh **/
			break;
	}
	output_message("</xs:schema>\n");
	return 0;
}

int output_xsl(char *fname, int n_mods, char *p_mods[])
{
	FILE *fp;

	/* load modules */
	while (n_mods-->0)
	{
		MODULE *mod = module_load(*p_mods,0,NULL);
		if (mod==NULL)
		{
			output_error("module %s not found", *p_mods);
			return FAILED;
		}
		p_mods++;
	}

	/* open the output file */
	fp = fopen(fname,"w");
	if (fp==NULL)
	{
		output_error("%s open failed: %s", fname, strerror(errno));
		return errno;
	}

	/* heading */
	fprintf(fp,"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
	fprintf(fp,"<!-- output by GridLAB-D -->\n");
	
	/* document */
	fprintf(fp,"<html xsl:version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" xmlns=\"http://www.w3.org/1999/xhtml\">\n");
	{
		fprintf(fp,"<xsl:for-each select=\"/gridlabd\">\n");
		{
			fprintf(fp,"<head>\n");
			{
				GLOBALVAR *stylesheet = global_find("stylesheet");
				fprintf(fp,"<title>GridLAB-D <xsl:value-of select=\"version.major\"/>.<xsl:value-of select=\"version.minor\"/> - <xsl:value-of select=\"modelname\"/></title>\n");
				if (stylesheet==NULL || stylesheet->prop->ptype!=PT_char1024) /* only char1024 is allowed */
					fprintf(fp,"<link rel=\"stylesheet\" href=\"%sgridlabd-%d_%d.css\" type=\"text/css\"/>\n",global_urlbase,global_version_major,global_version_minor);
				else
					fprintf(fp,"<link rel=\"stylesheet\" href=\"%s.css\" type=\"text/css\"/>\n",stylesheet->prop->addr);
			}
			fprintf(fp,"</head>\n");
			fprintf(fp,"<body>\n");
			{
				GLOBALVAR *var=NULL;
				MODULE *mod;
				
				fprintf(fp,"<H1><xsl:value-of select=\"modelname\"/></H1>\n");

				/* table of contents */
				fprintf(fp,"<H2>Table of Contents</H2>\n");
				fprintf(fp,"<OL TYPE=\"1\">\n");
				fprintf(fp,"<LI><A HREF=\"#global_variables\">Global variables</A></LI>\n");
				fprintf(fp,"<LI><A HREF=\"#solver_ranks\">Solver ranks</A></LI>\n");
				fprintf(fp,"<LI><A HREF=\"#modules\">Modules</A></LI><OL TYPE=\"a\">\n");
				for (mod=module_get_first(); mod!=NULL; mod=mod->next)
					fprintf(fp,"<LI><A HREF=\"#modules_%s\">%s</A></LI>\n",mod->name,mod->name);
				fprintf(fp,"</OL>\n");
				fprintf(fp,"<LI><A HREF=\"#output\">Output</A></LI>\n");
				fprintf(fp,"</OL>\n");

				/* global variable dump */
				fprintf(fp,"<H2><A NAME=\"global_variables\">GridLAB-D system variables</A></H2>\n");
				fprintf(fp,"<TABLE BORDER=\"1\">\n");
				while (var=global_getnext(var))
				{
					if (strstr(var->prop->name,"::"))
						continue; /* skip module globals (they'll get dumped later) */
					fprintf(fp,"<TR><TH>%s</TH><TD><xsl:value-of select=\"%s\"/></TD></TR>\n",var->prop->name,var->prop->name);
				}
				fprintf(fp,"</TABLE>\n");

				/* processing sequence dump */
				fprintf(fp,"<H2><A NAME=\"solver_ranks\">Solver ranks</A></H2>\n");
				fprintf(fp,"<TABLE BORDER=\"1\">\n");
					fprintf(fp,"<TR>");
						fprintf(fp,"<xsl:for-each select=\"sync-order/pass\">\n");
							fprintf(fp,"<TH>Pass <xsl:value-of select=\"name\"/></TH>\n");
						fprintf(fp,"</xsl:for-each>\n");
					fprintf(fp,"</TR>\n");
					fprintf(fp,"<TR>\n");
						fprintf(fp,"<xsl:for-each select=\"sync-order/pass\"><TD><DL>\n");
							fprintf(fp,"<xsl:for-each select=\"rank\">\n");
								fprintf(fp,"<xsl:sort select=\"ordinal\" data-type=\"number\" order=\"descending\"/>\n");
								fprintf(fp,"<DT>Rank <xsl:value-of select=\"ordinal\"/></DT>");
								fprintf(fp,"<xsl:for-each select=\"object\">\n");
								fprintf(fp,"<DD><a href=\"#{name}\"><xsl:value-of select=\"name\"/></a></DD>\n");
								fprintf(fp,"</xsl:for-each>\n");
							fprintf(fp,"</xsl:for-each>\n");
						fprintf(fp,"</DL></TD></xsl:for-each>\n");
					fprintf(fp,"</TR>\n");
				fprintf(fp,"</TABLE>\n");

				/* module object dumps */
				fprintf(fp,"<H2><A NAME=\"modules\">Modules</A></H2>\n");
				for (mod=module_get_first(); mod!=NULL; mod=mod->next)
				{
					CLASS *oclass;
					PROPERTY *prop;

					fprintf(fp,"<H3><A NAME=\"modules_%s\">%s</A></H3>", mod->name,mod->name);

					/* globals */
					fprintf(fp,"<TABLE BORDER=\"1\">\n");
					fprintf(fp,"<TR><TH>version.major</TH><TD><xsl:value-of select=\"%s/version.major\"/></TD></TR>",mod->name);
					fprintf(fp,"<TR><TH>version.minor</TH><TD><xsl:value-of select=\"%s/version.minor\"/></TD></TR>",mod->name);
					while (var=global_getnext(var))
					{
						if (strncmp(var->prop->name,mod->name,strlen(mod->name))==0)
						{
							char *name = var->prop->name + strlen(mod->name)+2; // offset name to after ::
							fprintf(fp,"<TR><TH>%s</TH><TD><xsl:value-of select=\"%s/%s\"/></TD></TR>",name,mod->name,name);
						}
					}
					fprintf(fp,"</TABLE>\n");

					/* object dump */
					for (oclass=mod->oclass; oclass!=NULL && oclass->module==mod; oclass=oclass->next)
					{
						CLASS *pclass = oclass;
						fprintf(fp,"<H4>%s objects</H4>", oclass->name);
						fprintf(fp,"<TABLE BORDER=\"1\">\n");
						fprintf(fp,"<TR><TH>Name</TH>");
						for (pclass=oclass; pclass!=NULL; pclass=pclass->parent)
							for (prop=class_get_first_property(pclass); prop!=NULL; prop=class_get_next_property(prop))
								fprintf(fp,"<TH>%s</TH>",prop->name);
						fprintf(fp,"</TR>\n");
						{
							fprintf(fp,"<xsl:for-each select=\"%s/%s_list/%s\">", mod->name,oclass->name,oclass->name);
							{
								fprintf(fp,"<TR><TD><a name=\"#{name}\"/><xsl:value-of select=\"name\"/> (#<xsl:value-of select=\"id\"/>)</TD>");
								for (pclass=oclass; pclass!=NULL; pclass=pclass->parent)
									for (prop=class_get_first_property(pclass); prop!=NULL; prop=class_get_next_property(prop))
									{
										if (prop->ptype==PT_object)
											fprintf(fp,"<TD><a href=\"#{%s}\"><xsl:value-of select=\"%s\"/></a></TD>",prop->name,prop->name);
										else
											fprintf(fp,"<TD><xsl:value-of select=\"%s\"/></TD>",prop->name);
									}
								fprintf(fp,"</TR>\n");
							}
							fprintf(fp,"</xsl:for-each>");
						}
						fprintf(fp,"</TABLE>\n");
					}
				}

				/* GLM output */
				fprintf(fp,"<H2><A NAME=\"output\">GLM Output</A></H2>\n");
				fprintf(fp,"<table border=\"1\" width=\"100%%\"><tr><td><pre>\n");
				{

					/* heading info */
					fprintf(fp,"# Generated by GridLAB-D <xsl:value-of select=\"version.major\"/>.<xsl:value-of select=\"version.minor\"/>\n");
					fprintf(fp,"# Command line..... <xsl:value-of select=\"command_line\"/>\n");
					fprintf(fp,"# Model name....... <xsl:value-of select=\"modelname\"/>\n");
					fprintf(fp,"# Start at......... <xsl:value-of select=\"starttime\"/>\n");
					fprintf(fp,"#\n");

					/* clock info */
					fprintf(fp,"clock {\n");
					fprintf(fp,"\ttimestamp '<xsl:value-of select=\"clock\"/>';\n");
					fprintf(fp,"\ttimezone <xsl:value-of select=\"timezone\"/>;\n");
					fprintf(fp,"}\n");

					/* module blocks */
					for (mod=module_get_first(); mod!=NULL; mod=mod->next)
					{
						GLOBALVAR *var=NULL;
						fprintf(fp,"<xsl:for-each select=\"%s\">", mod->name);
						{
							CLASS *oclass;

							fprintf(fp,"\n##############################################\n");
							fprintf(fp,"# %s module\n", mod->name);
							fprintf(fp,"module %s {\n", mod->name);
							fprintf(fp,"\tversion.major <xsl:value-of select=\"version.major\"/>;\n");
							fprintf(fp,"\tversion.minor <xsl:value-of select=\"version.minor\"/>;\n");
							while (var=global_getnext(var))
							{
								if (strncmp(var->prop->name,mod->name,strlen(mod->name))==0)
								{
									char *name = var->prop->name + strlen(mod->name)+2; // offset name to after ::
									fprintf(fp,"\t%s <xsl:value-of select=\"%s\"/>;\n",name,name);
								}
							}
							fprintf(fp,"}\n");

							for (oclass=mod->oclass; oclass!=NULL && oclass->module==mod; oclass=oclass->next)
							{
								fprintf(fp,"\n# %s::%s objects\n", mod->name, oclass->name);
								{
									fprintf(fp,"<xsl:for-each select=\"%s_list/%s\">", oclass->name,oclass->name);
									{
										PROPERTY *prop;
										fprintf(fp,"<a name=\"#GLM.{name}\"/>object %s:<xsl:value-of select=\"id\"/> {\n", oclass->name);
										fprintf(fp,"<xsl:if test=\"name!=''\">\tname \"<xsl:value-of select=\"name\"/>\";\n</xsl:if>");
										fprintf(fp,"<xsl:if test=\"parent!=''\">\tparent \"<a href=\"#GLM.{parent}\"><xsl:value-of select=\"parent\"/></a>\";\n</xsl:if>");
										fprintf(fp,"<xsl:if test=\"clock!=''\">\tclock '<xsl:value-of select=\"clock\"/>';\n</xsl:if>");
										fprintf(fp,"<xsl:if test=\"in_svc!=''\">\tin_svc '<xsl:value-of select=\"in_svc\"/>';\n</xsl:if>");
										fprintf(fp,"<xsl:if test=\"out_svc!=''\">\tout_svc '<xsl:value-of select=\"out_svc\"/>';\n</xsl:if>");
										fprintf(fp,"<xsl:if test=\"latitude!=''\">\tlatitude <xsl:value-of select=\"latitude\"/>;\n</xsl:if>");
										fprintf(fp,"<xsl:if test=\"longitude!=''\">\tlongitude <xsl:value-of select=\"longitude\"/>;\n</xsl:if>");
										fprintf(fp,"<xsl:if test=\"rank!=''\">\trank <xsl:value-of select=\"rank\"/>;\n</xsl:if>");
										for (prop=class_get_first_property(oclass); prop!=NULL; prop=class_get_next_property(prop))
										{
											if (prop->ptype==PT_object)
												fprintf(fp,"<xsl:if test=\"%s\">\t%s <a href=\"#GLM.{%s}\"><xsl:value-of select=\"%s\"/></a>;\n</xsl:if>",prop->name,prop->name,prop->name,prop->name);
											else
												fprintf(fp,"<xsl:if test=\"%s\">\t%s <xsl:value-of select=\"%s\"/>;\n</xsl:if>",prop->name,prop->name,prop->name);
										}
										fprintf(fp,"}\n");
									}
									fprintf(fp,"</xsl:for-each>");
								}
							}
						}
						fprintf(fp,"</xsl:for-each>");
					}
				}
				fprintf(fp,"</pre></td></tr></table>\n");
			}
			fprintf(fp,"</body>\n");
		}
		fprintf(fp,"</xsl:for-each>");
	}
	fprintf(fp,"</html>\n");

	fclose(fp);
	return 0;
}

/** @} **/
